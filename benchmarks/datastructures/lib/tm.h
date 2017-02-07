#ifndef TM_H
#define TM_H 1

#  include <stdio.h>


#ifndef REDUCED_TM_API

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */

#  include <assert.h>
#  include "memory.h"
#  include "thread.h"
#  include "types.h"
#  include "thread.h"
#  include <math.h>

#  define TM_ARG                        /* nothing */
#  define TM_ARG_ALONE                  /* nothing */
#  define TM_ARGDECL                    /* nothing */
#  define TM_ARGDECL_ALONE              /* nothing */
#  define TM_CALLABLE                   /* nothing */

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

#  define P_MALLOC(size)                malloc(size)
#  define P_FREE(ptr)                   free(ptr)
#  define TM_MALLOC(size)               malloc(size)
#  define FAST_PATH_FREE(ptr)           free(ptr) 
#  define SLOW_PATH_FREE(ptr)             free(ptr)

# define SETUP_NUMBER_TASKS(n)
# define SETUP_NUMBER_THREADS(n)
# define PRINT_STATS()
# define AL_LOCK(idx)

#endif

#include <asm/unistd.h>
#define rmb()           asm volatile ("sync" ::: "memory")
#define cpu_relax()     asm volatile ("" ::: "memory");
//#define cpu_relax() asm volatile ("or 31,31,31") 
#ifdef REDUCED_TM_API
#    define SPECIAL_THREAD_ID()         get_tid()
#else
#    define SPECIAL_THREAD_ID()         thread_getId()
#endif

//#  include <immintrin.h>
//#  include <rtmintrin.h>
#include <htmxlintrin.h>



extern __inline long
__attribute__ ((__gnu_inline__, __always_inline__, __artificial__))
__TM_is_self_conflict(void* const TM_buff)
{
  texasr_t texasr = __builtin_get_texasr ();
  return _TEXASR_SELF_INDUCED_CONFLICT (texasr);
}

extern __inline long
__attribute__ ((__gnu_inline__, __always_inline__, __artificial__))
__TM_is_trans_conflict(void* const TM_buff)
{
  texasr_t texasr = __builtin_get_texasr ();
  return _TEXASR_TRANSACTION_CONFLICT (texasr);
}

extern __inline long
__attribute__ ((__gnu_inline__, __always_inline__, __artificial__))
__TM_is_nontrans_conflict(void* const TM_buff)
{
  texasr_t texasr = __builtin_get_texasr ();
  return _TEXASR_NON_TRANSACTIONAL_CONFLICT (texasr);
}

extern __inline long
__attribute__ ((__gnu_inline__, __always_inline__, __artificial__))
__TM_begin_rot (void* const TM_buff)
{
  *_TEXASRL_PTR (TM_buff) = 0;
  if (__builtin_expect (__builtin_tbegin (1), 1)){
    return _HTM_TBEGIN_STARTED;
  }
#ifdef __powerpc64__
  *_TEXASR_PTR (TM_buff) = __builtin_get_texasr ();
#else
  *_TEXASRU_PTR (TM_buff) = __builtin_get_texasru ();
  *_TEXASRL_PTR (TM_buff) = __builtin_get_texasr ();
#endif
  *_TFIAR_PTR (TM_buff) = __builtin_get_tfiar ();
  return 0;
}

#  define TM_STARTUP(numThread, bId)		
#  define TM_SHUTDOWN(){ \
    unsigned long read_commits = 0; \
    unsigned long htm_commits = 0; \
    unsigned long htm_conflict_aborts = 0; \
    unsigned long htm_user_aborts = 0; \
    unsigned long htm_self_conflicts = 0; \
    unsigned long htm_trans_conflicts = 0; \
    unsigned long htm_nontrans_conflicts = 0; \
    unsigned long htm_persistent_aborts = 0; \
    unsigned long htm_capacity_aborts = 0; \
    unsigned long htm_other_aborts = 0; \
    unsigned long rot_commits = 0; \
    unsigned long rot_conflict_aborts = 0; \
    unsigned long rot_user_aborts = 0; \
    unsigned long rot_self_conflicts = 0; \
    unsigned long rot_trans_conflicts = 0; \
    unsigned long rot_nontrans_conflicts = 0; \
    unsigned long rot_persistent_aborts = 0; \
    unsigned long rot_capacity_aborts = 0; \
    unsigned long rot_other_aborts = 0; \
    unsigned long gl_commits = 0; \
    int i = 0; \
    for (; i < 80; i++) { \
       read_commits += stats_array[i].read_commits; \
       htm_commits += stats_array[i].htm_commits; \
       htm_conflict_aborts += stats_array[i].htm_conflict_aborts; \
       htm_user_aborts += stats_array[i].htm_user_aborts; \
       htm_self_conflicts += stats_array[i].htm_self_conflicts; \
       htm_trans_conflicts += stats_array[i].htm_trans_conflicts; \
       htm_nontrans_conflicts += stats_array[i].htm_nontrans_conflicts; \
       htm_persistent_aborts += stats_array[i].htm_persistent_aborts; \
       htm_capacity_aborts += stats_array[i].htm_capacity_aborts; \
       htm_other_aborts += stats_array[i].htm_other_aborts; \
       rot_commits += stats_array[i].rot_commits; \
       rot_conflict_aborts += stats_array[i].rot_conflict_aborts; \
       rot_user_aborts += stats_array[i].rot_user_aborts; \
       rot_self_conflicts += stats_array[i].rot_self_conflicts; \
       rot_trans_conflicts += stats_array[i].rot_trans_conflicts; \
       rot_nontrans_conflicts += stats_array[i].rot_nontrans_conflicts; \
       rot_persistent_aborts += stats_array[i].rot_persistent_aborts; \
       rot_capacity_aborts += stats_array[i].rot_capacity_aborts; \
       rot_other_aborts += stats_array[i].rot_other_aborts; \
       gl_commits += stats_array[i].gl_commits; \
    } \
    printf("Total commits: %lu\n\tRead commits: %lu\n\tHTM commits:  %lu\n\tROT commits:  %lu\n\tGL commits: %lu\nTotal aborts: %lu\n\tHTM conflict aborts:  %lu\n\t\tHTM self aborts:  %lu\n\t\tHTM trans aborts:  %lu\n\t\tHTM non-trans aborts:  %lu\n\tHTM user aborts :  %lu\n\tHTM capacity aborts:  %lu\n\t\tHTM persistent aborts:  %lu\n\tHTM other aborts:  %lu\n\tROT conflict aborts:  %lu\n\t\tROT self aborts:  %lu\n\t\tROT trans aborts:  %lu\n\t\tROT non-trans aborts:  %lu\n\tROT user aborts:  %lu\n\tROT capacity aborts:  %lu\n\t\tROT persistent aborts:  %lu\n\tROT other aborts:  %lu\n", read_commits+htm_commits+rot_commits+gl_commits, read_commits, htm_commits, rot_commits, gl_commits,htm_conflict_aborts+htm_user_aborts+htm_capacity_aborts+htm_other_aborts+rot_conflict_aborts+rot_user_aborts+rot_capacity_aborts+rot_other_aborts,htm_conflict_aborts,htm_self_conflicts,htm_trans_conflicts,htm_nontrans_conflicts,htm_user_aborts,htm_capacity_aborts,htm_persistent_aborts,htm_other_aborts,rot_conflict_aborts,rot_self_conflicts,rot_trans_conflicts,rot_nontrans_conflicts,rot_user_aborts,rot_capacity_aborts,rot_persistent_aborts,rot_other_aborts); \
} \ 

#  define TM_THREAD_ENTER()	//rot_readset = (long*)malloc(sizeof(long)*100000);
#  define TM_THREAD_EXIT()

# define IS_LOCKED(lock)        *((volatile int*)(&lock)) != 0

# define IS_GLOBAL_LOCKED(lock)        *((volatile int*)(&lock)) == 2

# define TM_BEGIN(ro) TM_BEGIN_EXT(0,ro)

# define ACQUIRE_READ_LOCK() { \
	while(1){ \
		counters[local_thread_id].value++; \
		rmb(); \
		if(IS_LOCKED(single_global_lock)){ \
			counters[local_thread_id].value += 7; \
			rmb(); \
			while(IS_LOCKED(single_global_lock)){ \
                        	cpu_relax(); \
	                } \
			continue; \
		} \
		break; \
	} \
}; \

# define RELEASE_READ_LOCK() counters[local_thread_id].value+=7; stats_array[local_thread_id].read_commits++; 

# define USE_HTM(){ \
	int htm_budget = HTM_RETRIES; \
	while(htm_budget > 0){ \
		htm_status = 1; \
		TM_buff_type TM_buff; \	
		while(IS_LOCKED(single_global_lock)){ \
                        cpu_relax(); \
                } \
		unsigned char tx_status = __TM_begin(&TM_buff); \
		if (tx_status == _HTM_TBEGIN_STARTED) { \
			if(IS_LOCKED(single_global_lock)){ \
				__TM_abort(); \
			} \
			break; \
		} \
		else if(__TM_is_conflict(&TM_buff)){ \
			stats_array[local_thread_id].htm_conflict_aborts ++; \
			if(__TM_is_self_conflict(&TM_buff)) {stats_array[local_thread_id].htm_self_conflicts++; }\
			else if(__TM_is_trans_conflict(&TM_buff)) stats_array[local_thread_id].htm_trans_conflicts++; \
                        else if(__TM_is_nontrans_conflict(&TM_buff)) stats_array[local_thread_id].htm_nontrans_conflicts++; \
                        htm_status = 0; \
                        htm_budget--; \
			unsigned long wait; \
			volatile int j; \
			cm_seed ^= (cm_seed << 17); \
			cm_seed ^= (cm_seed >> 13); \
			cm_seed ^= (cm_seed << 5); \
			wait = cm_seed % backoff; \
			for (j = 0; j < wait; j++); \
			if (backoff < MAX_BACKOFF) \
				backoff <<=1 ; \
		} \
		else if (__TM_is_user_abort(&TM_buff)) { \
			stats_array[local_thread_id].htm_user_aborts ++; \
                        htm_status = 0; \
                        htm_budget--; \
                } \
		else if(__TM_is_footprint_exceeded(&TM_buff)){ \
			htm_status = 0; \
			stats_array[local_thread_id].htm_capacity_aborts ++; \
			if(__TM_is_failure_persistent(&TM_buff)) stats_array[local_thread_id].htm_persistent_aborts ++; \
			break; \
		} \
		else{ \
			stats_array[local_thread_id].htm_other_aborts ++; \
			htm_status = 0; \
			htm_budget--; \
		} \
	} \
};


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

# define USE_ROT(){ \
	int rot_budget = ROT_RETRIES; \
	while(IS_LOCKED(single_global_lock)){ \
        	cpu_relax(); \
        } \
	while(rot_budget > 0){ \
		rot_status = 1; \
		TM_buff_type TM_buff; \
		int state = counters[local_thread_id].value & 7; \
		if (!state) \
			counters[local_thread_id].value += 3; \
                else if(state == 7) \
                        counters[local_thread_id].value += 4;\
		else if(state == 3) \
                        counters[local_thread_id].value += 8;\
		rmb(); \
		if(IS_LOCKED(single_global_lock)){ \
			counters[local_thread_id].value += 5; \
			rmb(); \
			while(IS_LOCKED(single_global_lock)) cpu_relax(); \
			continue; \
		} \
                /*rot_readset = (readset_t *)malloc(sizeof(readset_t)); \
                readset_item_t *item = (readset_item_t *)malloc(sizeof(readset_item_t)); \
                item->addr = -1; \
                item->next = NULL; \
                rot_readset->head = item; */\
		rs_counter = 0; \
		/*printf("thread %d starting ROT with counters %d and rot counters %d\n",local_thread_id,counters[local_thread_id].value,rot_counters[local_thread_id].value); */\
		unsigned char tx_status = __TM_begin_rot(&TM_buff); \
		if (tx_status == _HTM_TBEGIN_STARTED) { \
			/*rot_counters[local_thread_id].value ++; */\			
			/*begin_rot = rdtsc(); */\
                        /*if(IS_LOCKED(single_global_lock)){ \
                                __TM_abort(); \
                        } \
			single_global_lock = single_global_lock;*/\
                        break; \
                } \
		else if(__TM_is_conflict(&TM_buff)){ \
			/*printf("conflict: %p\n",__TM_failure_address(&TM_buff)); */\
                        stats_array[local_thread_id].rot_conflict_aborts ++; \
			if(__TM_is_self_conflict(&TM_buff)) stats_array[local_thread_id].rot_self_conflicts++; \
			else if(__TM_is_trans_conflict(&TM_buff)) stats_array[local_thread_id].rot_trans_conflicts++; \
			else if(__TM_is_nontrans_conflict(&TM_buff)) stats_array[local_thread_id].rot_nontrans_conflicts++; \
                        rot_status = 0; \
                        rot_budget--; \
			state = counters[local_thread_id].value & 7; \
                	if(state == 7) \
                        	counters[local_thread_id].value ++;\
                	else if(state == 3) \
                        	counters[local_thread_id].value += 5;\
                	rmb(); \
                        unsigned long wait; \
                        volatile int j; \
                        cm_seed ^= (cm_seed << 17); \
                        cm_seed ^= (cm_seed >> 13); \
                        cm_seed ^= (cm_seed << 5); \
                        wait = cm_seed % backoff; \
                        for (j = 0; j < wait; j++); \
                        if (backoff < MAX_BACKOFF) \
                                backoff <<= 1; \
                } \
                else if (__TM_is_user_abort(&TM_buff)) { \
                        stats_array[local_thread_id].rot_user_aborts ++; \
                        rot_status = 0; \
                        rot_budget--; \
                } \
                else if(__TM_is_footprint_exceeded(&TM_buff)){ \
			rot_status = 0; \
			stats_array[local_thread_id].rot_capacity_aborts ++; \
			if(__TM_is_failure_persistent(&TM_buff)) stats_array[local_thread_id].rot_persistent_aborts ++; \
                        break; \
		} \
                else{ \
			rot_status = 0; \
                        rot_budget--; \
			stats_array[local_thread_id].rot_other_aborts ++; \
		} \
	} \
};

# define ACQUIRE_GLOBAL_LOCK(){ \
	int state = counters[local_thread_id].value & 7; \
	counters[local_thread_id].value += (8-state); \
        rmb(); \
	while (pthread_spin_trylock(&single_global_lock) != 0) { \
                    __asm volatile ("" : : : "memory"); \
        } \
	/*while(1){ \
                while(IS_LOCKED(single_global_lock)){ \
                        cpu_relax(); \
                } \
                if(pthread_spin_trylock(&single_global_lock) == 0){ \
                        break; \
                } \
        } */\
	QUIESCENCE_CALL_GL(); \
};

# define ACQUIRE_WRITE_LOCK() { \
	/*while(IS_LOCKED(single_global_lock)){ \
		cpu_relax(); \
	} */\
	int htm_status = 0; \
	USE_HTM(); \
	if(!htm_status){ \ 
		local_exec_mode = 1; \
		int rot_status = 0; \
		USE_ROT(); \
		if(!rot_status){ \
			local_exec_mode = 2; \
			ACQUIRE_GLOBAL_LOCK(); \
		} \
	} \
};\

# define QUIESCENCE_CALL_ROT(){ \
	long num_threads = global_numThread; \
	long index;\
	unsigned  mask = (1 << 3) - 1;\
	int state;\
	volatile long temp; \
	long counters_snapshot[80]; \
	for(index=0; index < 80; index++){ \
                if (index == num_threads) break; \
		temp = counters[index].value; \
		state = temp & 7; \
		/*printf("state is %d,%d\n",state,temp); */\
		switch(state){ \
			case 7:\
                                counters_snapshot[index] = 0; \
                                break;\
			case 3:\
				counters_snapshot[index] = temp; \
				break;\
			case 1:\
				counters_snapshot[index] = temp; \
				break;\
			default:\
				counters_snapshot[index] = 0; \
				break;\
		} \
        } \
	TOUCH_REVALIDATION(); \
	__TM_suspend(); \
        counters[local_thread_id].value += 4; \
        rmb(); \
         __TM_resume(); \
	for(index=0; index < 80; index++){ \
		if (index == num_threads) break; \
		if(counters_snapshot[index] != 0){ \
			while(counters[index].value == counters_snapshot[index]){ \
				cpu_relax(); \
			} \
		} \
	} \
};

# define QUIESCENCE_CALL(){ \
        long num_threads = global_numThread; \
        long index;\
        volatile long temp; \
        long counters_snapshot[80]; \
        for(index=0; index < 80; index++){ \
                if (index == num_threads) break; \
                temp = counters[index].value; \
                if(temp & 1){ \
                        counters_snapshot[index] = temp; \
                }\
                else{ \
                        counters_snapshot[index] = 0; \ 
                } \
        } \
        for(index=0; index < 80; index++){ \
                if (index == num_threads) break; \
                if(counters_snapshot[index] != 0){ \
                        while(counters[index].value == counters_snapshot[index]){ \
				cpu_relax(); \
                        } \
                } \
        } \
};

# define QUIESCENCE_CALL_SINGLE(){ \
	int num_threads = global_numThread; \
        int index;\
        register long temp; \
        for(index=0; index < num_threads; index++){ \
                temp = counters[index].value; \
                if(temp & 1){ \
			while(temp == counters[index].value) \
                        cpu_relax(); \
                }\
        } \
};

# define QUIESCENCE_CALL_GL(){ \
        /*__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t num_threads; \
        num_threads.value = global_numThread; \
        __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t index; */ \
	int index;\
	int num_threads = global_numThread; \
        for(index=0; index < num_threads; index++){ \
                        while(counters[index].value & 1){ \
                                cpu_relax(); \
                        } \
        } \
};


# define TOUCH_REVALIDATION(){ \
        intptr_t temp; \
        int index; \
        for(index=0;index<rs_counter;index++){ \
		if(rot_readset[index]){ \
	                temp = *(intptr_t *)rot_readset[index]; \
			/*if(temp) \
				rot_readset_values[index] = (long)temp; */\
		} \
		/*if(rot_readset->head->type) \
			temp = rot_readset->head->addr_p; \
		else{ \
			if (rot_readset->head->addr == -1) \
				break; \
			temp = *rot_readset->head->addr; \
		} */\
		/*printf("read set has entry %d\n",*rot_readset->head->addr);*/\
		/*rot_readset->head = rot_readset->head->next; */\
	} \
};

# define RELEASE_WRITE_LOCK(){ \
	if(!local_exec_mode){ \
		__TM_suspend(); \
		QUIESCENCE_CALL_SINGLE();\
		__TM_resume(); \
		__TM_end(); \
		/*printf("thread %d committed in HTM with counters %lu\n",local_thread_id,counters[local_thread_id].value); */\
		stats_array[local_thread_id].htm_commits++; \
	} \
	else if(local_exec_mode == 1){ \
		/*unsigned long long int length = rdtsc() -begin_rot ; */\
	        /*__TM_suspend(); \
	        counters[local_thread_id].value += 4; \
        	rmb(); \
	         __TM_resume(); */\
		QUIESCENCE_CALL_ROT(); \
                /*TOUCH_REVALIDATION(); */\
		__TM_end(); \
		counters[local_thread_id].value+=1; \
        	/*rmb(); \
		printf("rscounter\t%d\n",rs_counter); */\
		/*printf("thread %d committed in ROT with counters %lu and rot_counters %d\n",local_thread_id,counters[local_thread_id].value,rot_counters[local_thread_id].value); */\
		stats_array[local_thread_id].rot_commits++; \
		/*printf("length of tx is: %lu\n",rs_counter); */\
	} \
	else{ \
		pthread_spin_unlock(&single_global_lock); \
		stats_array[local_thread_id].gl_commits++; \
		/*printf("thread %d committed in GL with counters %lu\n",local_thread_id,counters[local_thread_id].value); */\
	} \
	/*printf("thread %d committed\n",local_thread_id); */\
};

# define TM_BEGIN_EXT(b,ro) {  \
	/*unsigned long long int begin_rot; */\
	local_exec_mode = 0; \
	rs_counter = 0; \
	local_thread_id = SPECIAL_THREAD_ID();\
	backoff = MIN_BACKOFF; \
	if(ro){ \
		ACQUIRE_READ_LOCK(); \
	} \
	else{ \
		ACQUIRE_WRITE_LOCK(); \
	} \
}

# define TM_END(){ \
	if(ro){ \
		RELEASE_READ_LOCK(); \
	} \
	else{ \
		RELEASE_WRITE_LOCK(); \
	} \
};

#    define TM_BEGIN_RO()                 TM_BEGIN(1)
#    define TM_RESTART()                  __TM_abort();
#    define TM_EARLY_RELEASE(var)

# define FAST_PATH_RESTART() __TM_abort();


inline void* TxLoad_P(void* var){
	if(var){
	        rot_readset[rs_counter] = (long)var;
        	rs_counter++;
	}
        return var;
}

//#define SLOW_PATH_SHARED_READ(var)                  TxLoad((intptr_t)(var))
//#define SLOW_PATH_SHARED_READ_P(var)                  TxLoad_P(var)
//#define SLOW_PATH_SHARED_READ_D(var)                   TxLoad((intptr_t)(var))

#define SLOW_PATH_SHARED_READ(var)             var; rot_readset[rs_counter++] = &var;
#define SLOW_PATH_SHARED_READ_P(var)           var; rot_readset[rs_counter++] = var;
#define SLOW_PATH_SHARED_READ_D(var)           var; rot_readset[rs_counter++] = &var;
//#define SLOW_PATH_SHARED_READ_P(var)           var;

#define FAST_PATH_SHARED_READ(var)                 var
#define FAST_PATH_SHARED_READ_P(var)                  var
#define FAST_PATH_SHARED_READ_D(var)                   var

# define FAST_PATH_SHARED_WRITE(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_P(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_D(var, val) ({var = val; var;})

# define SLOW_PATH_RESTART() FAST_PATH_RESTART()
//# define SLOW_PATH_SHARED_READ(var)           FAST_PATH_SHARED_READ(var)
//# define SLOW_PATH_SHARED_READ_P(var)         FAST_PATH_SHARED_READ_P(var)
//# define SLOW_PATH_SHARED_READ_F(var)         FAST_PATH_SHARED_READ_D(var)
//# define SLOW_PATH_SHARED_READ_D(var)         FAST_PATH_SHARED_READ_D(var)
# define SLOW_PATH_SHARED_WRITE(var, val)     FAST_PATH_SHARED_WRITE(var, val)
# define SLOW_PATH_SHARED_WRITE_P(var, val)   FAST_PATH_SHARED_WRITE_P(var, val)
# define SLOW_PATH_SHARED_WRITE_D(var, val)   FAST_PATH_SHARED_WRITE_D(var, val)


#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})


#endif
