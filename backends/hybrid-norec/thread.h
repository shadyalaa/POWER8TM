#ifndef THREAD_H
#define THREAD_H 1


#include <pthread.h>
#include <stdlib.h>

#include <stm.h>
#include "norec.h"
#include "norec.h"

# define CACHE_LINE_SIZE 128

typedef struct padded_scalar {
    volatile unsigned long counter;
    char suffixPadding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t;

/*typedef struct padded_statistics {
    unsigned long commits;
    unsigned long aborts;
    char suffixPadding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t;*/

typedef struct padded_statistics {
    unsigned long tle_commits;
    unsigned long stm_commits;
    unsigned long conflicts;
    unsigned long self;
    unsigned long trans;
    unsigned long nontrans;
    unsigned long capacity;
    unsigned long other;
    unsigned long user;
    unsigned long persistent;
    unsigned long stm_aborts;
    char suffixPadding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t;

extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t stats_array[];

//extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t fallback_in_use;
extern __attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t fallback_in_use; 
extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t exists_sw;
extern __attribute__((aligned(CACHE_LINE_SIZE))) __thread unsigned short read_only_htm;

extern __thread unsigned int local_exec_mode;
extern __thread unsigned int local_thread_id;

/*extern void freeHTM(void* ptr);
extern void abortHTM(Thread* Self);
extern intptr_t sharedReadHTM(Thread* Self, intptr_t* addr);
extern void sharedWriteHTM(Thread* Self, intptr_t* addr, intptr_t val);
extern void freeSTM(void* ptr);
extern void abortSTM(Thread* Self);
extern intptr_t sharedReadSTM(Thread* Self, intptr_t* addr);
extern void sharedWriteSTM(Thread* Self, intptr_t* addr, intptr_t val);

extern __thread void (*freeFunPtr)(void* ptr);
extern __thread void (*abortFunPtr)(Thread* Self);
extern __thread intptr_t (*sharedReadFunPtr)(Thread* Self, intptr_t* addr);
extern __thread void (*sharedWriteFunPtr)(Thread* Self, intptr_t* addr, intptr_t val);*/

extern __thread unsigned long backoff;
extern __thread unsigned long cm_seed;

#ifndef REDUCED_TM_API


#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


#define THREAD_T                            pthread_t
#define THREAD_ATTR_T                       pthread_attr_t

#define THREAD_ATTR_INIT(attr)              pthread_attr_init(&attr)
#define THREAD_JOIN(tid)                    pthread_join(tid, (void**)NULL)
#define THREAD_CREATE(tid, attr, fn, arg)   pthread_create(&(tid), \
                                                           &(attr), \
                                                           (void* (*)(void*))(fn), \
                                                           (void*)(arg))

#define THREAD_LOCAL_T                      pthread_key_t
#define THREAD_LOCAL_INIT(key)              pthread_key_create(&key, NULL)
#define THREAD_LOCAL_SET(key, val)          pthread_setspecific(key, (void*)(val))
#define THREAD_LOCAL_GET(key)               pthread_getspecific(key)

#define THREAD_MUTEX_T                      pthread_mutex_t
#define THREAD_MUTEX_INIT(lock)             pthread_spin_init(&(lock), NULL)
#define THREAD_MUTEX_LOCK(lock)             pthread_mutex_lock(&(lock))
#define THREAD_MUTEX_UNLOCK(lock)           pthread_mutex_unlock(&(lock))

#define THREAD_COND_T                       pthread_cond_t
#define THREAD_COND_INIT(cond)              pthread_cond_init(&(cond), NULL)
#define THREAD_COND_SIGNAL(cond)            pthread_cond_signal(&(cond))
#define THREAD_COND_BROADCAST(cond)         pthread_cond_broadcast(&(cond))
#define THREAD_COND_WAIT(cond, lock)        pthread_cond_wait(&(cond), &(lock))

#  define THREAD_BARRIER_T                  barrier_t
#  define THREAD_BARRIER_ALLOC(N)           barrier_alloc()
#  define THREAD_BARRIER_INIT(bar, N)       barrier_init(bar, N)
#  define THREAD_BARRIER(bar, tid)          barrier_cross(bar)
#  define THREAD_BARRIER_FREE(bar)          barrier_free(bar)


typedef struct barrier {
    pthread_cond_t complete;
    pthread_mutex_t mutex;
    int count;
    int crossing;
} barrier_t;

barrier_t *barrier_alloc();

void barrier_free(barrier_t *b);

void barrier_init(barrier_t *b, int n);

void barrier_cross(barrier_t *b);

void thread_startup (long numThread);

void thread_start (void (*funcPtr)(void*), void* argPtr);

void thread_shutdown ();

void thread_barrier_wait();

long thread_getId();

long thread_getNumThread();

#ifdef __cplusplus
}
#endif

#endif

#endif /* THREAD_H */
