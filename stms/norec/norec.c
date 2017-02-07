#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include "platform.h"
//#include "norec.h"
#include "util.h"


__INLINE__ long ReadSetCoherent (Thread*);


enum norec_config {
    NOREC_INIT_WRSET_NUM_ENTRY = 1024,
    NOREC_INIT_RDSET_NUM_ENTRY = 8192,
    NOREC_INIT_LOCAL_NUM_ENTRY = 1024,
};


typedef int            BitMap;

/* Read set and write-set log entry */
typedef struct _AVPair {
    struct _AVPair* Next;
    struct _AVPair* Prev;
    volatile intptr_t* Addr;
    intptr_t Valu;
    long Ordinal;
} AVPair;

typedef struct _Log {
    AVPair* List;
    AVPair* put;        /* Insert position - cursor */
    AVPair* tail;       /* CCM: Pointer to last valid entry */
    AVPair* end;        /* CCM: Pointer to last entry */
    long ovf;           /* Overflow - request to grow */
    BitMap BloomFilter; /* Address exclusion fast-path test */
} Log;

struct _Thread {
    long UniqID;
    volatile long Retries;
    long Starts;
    long Aborts; /* Tally of # of aborts */
    long snapshot;
    unsigned long long rng;
    unsigned long long xorrng [1];
    Log rdSet;
    Log wrSet;
    volatile unsigned long cm_backoff;               
    volatile unsigned long cm_seed;  
    sigjmp_buf* envPtr;
};

typedef struct
{
    long value;
    char suffixPadding[128];
} aligned_type_t ;

__attribute__((aligned(128))) volatile aligned_type_t* LOCK;


static pthread_key_t    global_key_self;

void TxIncClock() {
    LOCK->value = LOCK->value + 2;
}

long  TxReadClock() {
    return LOCK->value;
}

#ifndef NOREC_CACHE_LINE_SIZE
#  define NOREC_CACHE_LINE_SIZE           (64)
#endif

__INLINE__ unsigned long long
MarsagliaXORV (unsigned long long x)
{
    if (x == 0) {
        x = 1;
    }
    x ^= x << 6;
    x ^= x >> 21;
    x ^= x << 7;
    return x;
}

__INLINE__ unsigned long long
MarsagliaXOR (unsigned long long* seed)
{
    unsigned long long x = MarsagliaXORV(*seed);
    *seed = x;
    return x;
}

__INLINE__ unsigned long long
TSRandom (Thread* Self)
{
    return MarsagliaXOR(&Self->rng);
}

__INLINE__ intptr_t
AtomicAdd (volatile intptr_t* addr, intptr_t dx)
{
    intptr_t v;
    for (v = *addr; CAS(addr, v, v+dx) != v; v = *addr) {}
    return (v+dx);
}

volatile long StartTally         = 0;
volatile long AbortTally         = 0;
volatile long ReadOverflowTally  = 0;
volatile long WriteOverflowTally = 0;
volatile long LocalOverflowTally = 0;
#define NOREC_TALLY_MAX          (((unsigned long)(-1)) >> 1)

#define FILTERHASH(a)                   ((UNS(a) >> 2) ^ (UNS(a) >> 5))
#define FILTERBITS(a)                   (1 << (FILTERHASH(a) & 0x1F))

__INLINE__ AVPair*
MakeList (long sz, Thread* Self)
{
    AVPair* ap = (AVPair*) malloc((sizeof(*ap) * sz) + NOREC_CACHE_LINE_SIZE);
    assert(ap);
    memset(ap, 0, sizeof(*ap) * sz);
    AVPair* List = ap;
    AVPair* Tail = NULL;
    long i;
    for (i = 0; i < sz; i++) {
        AVPair* e = ap++;
        e->Next    = ap;
        e->Prev    = Tail;
        e->Ordinal = i;
        Tail = e;
    }
    Tail->Next = NULL;

    return List;
}

 void FreeList (Log*, long) __attribute__ ((noinline));
/*__INLINE__*/ void
FreeList (Log* k, long sz)
{
    /* Free appended overflow entries first */
    AVPair* e = k->end;
    if (e != NULL) {
        while (e->Ordinal >= sz) {
            AVPair* tmp = e;
            e = e->Prev;
            free(tmp);
        }
    }

    /* Free continguous beginning */
    free(k->List);
}

__INLINE__ AVPair*
ExtendList (AVPair* tail)
{
    AVPair* e = (AVPair*)malloc(sizeof(*e));
    assert(e);
    memset(e, 0, sizeof(*e));
    tail->Next = e;
    e->Prev    = tail;
    e->Next    = NULL;
    e->Ordinal = tail->Ordinal + 1;
    return e;
}

__INLINE__ void
WriteBackForward (Log* k)
{
    AVPair* e;
    AVPair* End = k->put;
    for (e = k->List; e != End; e = e->Next) {
        *(e->Addr) = e->Valu;
    }
}

void
TxOnce ()
{
    LOCK = (aligned_type_t*) malloc(sizeof(aligned_type_t));
    LOCK->value = 0;

    pthread_key_create(&global_key_self, NULL); /* CCM: do before we register handler */

}


void
TxShutdown ()
{
    printf("NOREC system shutdown:\n"
           "  Starts=%li Aborts=%li\n"
           "  Overflows: R=%li W=%li L=%li\n"
           , StartTally, AbortTally,
           ReadOverflowTally, WriteOverflowTally, LocalOverflowTally);

    pthread_key_delete(global_key_self);

    MEMBARSTLD();
}


Thread*
TxNewThread ()
{
    Thread* t = (Thread*)malloc(sizeof(Thread));
    assert(t);

    return t;
}


void
TxFreeThread (Thread* t)
{
    AtomicAdd((volatile intptr_t*)((void*)(&ReadOverflowTally)),  t->rdSet.ovf);

    long wrSetOvf = 0;
    Log* wr;
    wr = &t->wrSet;
    {
        wrSetOvf += wr->ovf;
    }
    AtomicAdd((volatile intptr_t*)((void*)(&WriteOverflowTally)), wrSetOvf);

    AtomicAdd((volatile intptr_t*)((void*)(&StartTally)),         t->Starts);
    AtomicAdd((volatile intptr_t*)((void*)(&AbortTally)),         t->Aborts);

    FreeList(&(t->rdSet),     NOREC_INIT_RDSET_NUM_ENTRY);
    FreeList(&(t->wrSet),     NOREC_INIT_WRSET_NUM_ENTRY);

    free(t);
}

void
TxInitThread (Thread* t, long id)
{
    /* CCM: so we can access NOREC's thread metadata in signal handlers */
    pthread_setspecific(global_key_self, (void*)t);

    memset(t, 0, sizeof(*t));     /* Default value for most members */

    t->UniqID = id;
    t->rng = id + 1;
    t->xorrng[0] = t->rng;

    t->wrSet.List = MakeList(NOREC_INIT_WRSET_NUM_ENTRY, t);
    t->wrSet.put = t->wrSet.List;

    t->rdSet.List = MakeList(NOREC_INIT_RDSET_NUM_ENTRY, t);
    t->rdSet.put = t->rdSet.List;

    t->cm_seed = 123456789UL;
    t->cm_backoff=4;
}

__INLINE__ void
txReset (Thread* Self)
{
    Self->wrSet.put = Self->wrSet.List;
    Self->wrSet.tail = NULL;

    Self->wrSet.BloomFilter = 0;
    Self->rdSet.put = Self->rdSet.List;
    Self->rdSet.tail = NULL;

}

__INLINE__ void
txCommitReset (Thread* Self)
{
    txReset(Self);
    Self->Retries = 0;
    Self->cm_backoff=4;
}

// returns -1 if not coherent
__INLINE__ long
ReadSetCoherent (Thread* Self)
{
    long time;
    while (1) {
        time = LOCK->value;
	MEMBARSTLD();
        if ((time & 1) != 0) {
            continue;
        }

        Log* const rd = &Self->rdSet;
        AVPair* const EndOfList = rd->put;
        AVPair* e;

        for (e = rd->List; e != EndOfList; e = e->Next) {
            if (e->Valu != LDNF(e->Addr)) {
                return -1;
            }
        }

        if (LOCK->value == time)
            break;
    }
    return time;
}


__INLINE__ void
backoff (Thread* Self, long attempt)
{
    unsigned long long stall = TSRandom(Self) & 0xF;
    stall += attempt >> 2;
    stall *= 10;
    /* CCM: timer function may misbehave */
    volatile typeof(stall) i = 0;
    while (i++ < stall) {
        PAUSE();
    }
}


static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int result=0;
  unsigned long int upper, lower,tmp;
  __asm__ volatile(
                "0:                  \n"
                "\tmftbu   %0           \n"
                "\tmftb    %1           \n"
                "\tmftbu   %2           \n"
                "\tcmpw    %2,%0        \n"
                "\tbne     0b         \n"
                : "=r"(upper),"=r"(lower),"=r"(tmp)
                );
  result = upper;
  result = result<<32;
  result = result|lower;

  return(result);
}

__INLINE__ long
TryFastUpdate (Thread* Self)
{
    Log* const wr = &Self->wrSet;
    long ctr;
    //unsigned long long int begin = rdtsc();    
    while (CAS(&(LOCK->value), Self->snapshot, Self->snapshot + 1) != Self->snapshot) {
        long newSnap = ReadSetCoherent(Self);
        if (newSnap == -1) {
            return 0; //TxAbort(Self);
        }
        Self->snapshot = newSnap;
    }

    {
        WriteBackForward(wr); /* write-back the deferred stores */
    }
    MEMBARSTST(); /* Ensure the above stores are visible  */
    LOCK->value = Self->snapshot + 2;
    MEMBARSTLD();
    //unsigned long long int length = rdtsc() - begin;
    //printf("length of commit phase: %lu\n",length);
    return 1; /* success */
}

void
TxAbort (Thread* Self)
{
    Self->Retries++;
    Self->Aborts++;

    //if (Self->Retries > 3) { /* TUNABLE */
        //backoff(Self, Self->Retries);
    //}


    unsigned long wait;
    volatile int j;
    Self->cm_seed ^= (Self->cm_seed << 17);
    Self->cm_seed ^= (Self->cm_seed >> 13);
    Self->cm_seed ^= (Self->cm_seed << 5);
    wait = Self->cm_seed % Self->cm_backoff;
	//printf("MAX_BACKOFF is %ld, wait is %ld, seed is %ld and backoff is %ld\n",MAX_BACKOFF,wait,Self->cm_seed,Self->cm_backoff);
    for (j = 0; j < wait; j++);
    if (Self->cm_backoff < MAX_BACKOFF)
       Self->cm_backoff <<= 1;


    SIGLONGJMP(*Self->envPtr, 1);
    ASSERT(0);
}
//pthread_mutex_t cas_mutex;

void
TxStore (Thread* Self, volatile intptr_t* addr, intptr_t valu)
{

//  if(local_exec_mode==1){
//        *addr = valu;
//	return;
//   }

	//printf("writing to %p \t %ld\n",addr,valu);
    Log* k = &Self->wrSet;

    k->BloomFilter |= FILTERBITS(addr);

    AVPair* e = k->put;
    if (e == NULL) {
    	k->ovf++;
    	e = ExtendList(k->tail);
    	k->end = e;
    }
    k->tail    = e;
    k->put     = e->Next;
    e->Addr    = addr;
    e->Valu    = valu;
}


intptr_t
TxLoad (Thread* Self, volatile intptr_t* Addr)
{
  // if(local_exec_mode==1){
//	rot_readset[rs_counter] = (long)Addr;
//	rs_counter++;
//	return LDNF(Addr);
//   }
    intptr_t Valu; 
	//printf("trying to read @ %p\n",Addr);
    intptr_t msk = FILTERBITS(Addr);
    if ((Self->wrSet.BloomFilter & msk) == msk) {
        //printf("exists in WS: %p\n",Addr);
        Log* wr = &Self->wrSet;
        AVPair* e;
        for (e = wr->tail; e != NULL; e = e->Prev) {
            if (e->Addr == Addr) {
		//printf("found %d @ %p\n",e->Valu,Addr);
                return e->Valu;
            }
        }
    }

    MEMBARLDLD();
    Valu = LDNF(Addr);
//    printf("TxLoad \t %f @ %p\n",*Addr,Addr);

    while (LOCK->value != Self->snapshot) {
	//while((LOCK->value & 1)!=0) PAUSE(); 
	//MEMBARLDLD();
        long newSnap = ReadSetCoherent(Self);
        if (newSnap == -1) {
            TxAbort(Self);
        }
        Self->snapshot = newSnap;
	//printf("snapshot is %ld\n",Self->snapshot);
        MEMBARLDLD();
        Valu = LDNF(Addr);
    }

    Log* k = &Self->rdSet;
    AVPair* e = k->put;
    if (e == NULL) {
        k->ovf++;
        e = ExtendList(k->tail);
        k->end = e;
    }
    k->tail    = e;
    k->put     = e->Next;
    e->Addr = Addr;
    e->Valu = Valu;
//    printf("Return \t %f @ %p\n",Valu,Addr);
    return Valu;
}


void
TxStart (Thread* Self, sigjmp_buf* envPtr)
{
    txReset(Self);


    MEMBARLDLD();

    Self->envPtr= envPtr;



    Self->Starts++;

    do {
        Self->snapshot = LOCK->value;
	PAUSE(); 
    } while ((Self->snapshot & 1) != 0);

}

int
TxCommit (Thread* Self)
{
    /* Fast-path: Optional optimization for pure-readers */
    if (Self->wrSet.put == Self->wrSet.List)
    {
        txCommitReset(Self);
        return 1;
    }

    if (TryFastUpdate(Self)) {
        txCommitReset(Self);
        return 1;
    }

    TxAbort(Self);
    return 0;
}

int
TxCommitSTM (Thread* Self)
{
    /* Fast-path: Optional optimization for pure-readers */
    if (Self->wrSet.put == Self->wrSet.List)
    {
        txCommitReset(Self);
        return 1;
    }

    if (TryFastUpdate(Self)) {
        txCommitReset(Self);
        return 1;
    }
    return 0;
}

long TxValidate (Thread* Self) {
    if (Self->wrSet.put == Self->wrSet.List) {
        return -1;
    } else {
        long local_global_clock = LOCK->value;

        while (1) {
            Log* const rd = &Self->rdSet;
            AVPair* const EndOfList = rd->put;
            AVPair* e;

            for (e = rd->List; e != EndOfList; e = e->Next) {
                if (e->Valu != LDNF(e->Addr)) {
                    TxAbort(Self);
                }
            }

            long tmp = LOCK->value;
            if (local_global_clock == tmp) {
                return local_global_clock;
            } else {
                local_global_clock = tmp;
            }
        }
        return local_global_clock;
    }
}


long TxFinalize (Thread* Self, long clock) {
    if (Self->wrSet.put == Self->wrSet.List) {
        txCommitReset(Self);
        return 0;
    }

    if (LOCK->value != clock) {
        return 1;
    }

    Log* const wr = &Self->wrSet;
    WriteBackForward(wr); /* write-back the deferred stores */
    LOCK->value += LOCK->value + 2;

    return 0;
}

void TxResetAfterFinalize (Thread* Self) {
    txCommitReset(Self);
}
