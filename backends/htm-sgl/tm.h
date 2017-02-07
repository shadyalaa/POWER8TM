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
#  define FAST_PATH_FREE(ptr)            free(ptr)
#  define SLOW_PATH_FREE(ptr)            free(ptr)

# define SETUP_NUMBER_TASKS(n)
# define SETUP_NUMBER_THREADS(n)
# define PRINT_STATS()
# define AL_LOCK(idx)

#endif

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


#  define TM_STARTUP(numThread, bId)
#  define TM_SHUTDOWN() { \
    unsigned long tle_commits = 0; \
    unsigned long gl_commits = 0; \
    unsigned long conflicts = 0; \
    unsigned long self = 0; \
    unsigned long trans = 0; \
    unsigned long nontrans = 0; \
    unsigned long capacity = 0; \
    unsigned long user = 0; \
    unsigned long persistent = 0; \
    unsigned long other = 0; \
    int i = 0; \
    for (; i < 80; i++) { \
       if (stats_array[i].tle_commits+stats_array[i].gl_commits == 0) { break; } \
       	tle_commits += stats_array[i].tle_commits; \
	gl_commits += stats_array[i].gl_commits; \
	conflicts += stats_array[i].conflicts; \
	self += stats_array[i].self; \
	trans += stats_array[i].trans; \
	nontrans += stats_array[i].nontrans; \
	capacity += stats_array[i].capacity; \
	user += stats_array[i].user; \
	persistent += stats_array[i].persistent; \ 
	other += stats_array[i].other; \
    } \
    printf("Total commits: %lu\n\tHTM commits: %lu\n\tGL commits: %lu\nTotal aborts: %lu\n\tHTM conflict aborts: %lu\n\t\tHTM trans conflicts: %lu\n\t\tHTM non-trans conflicts: %lu\n\t\tHTM self conflicts: %lu\n\tHTM capacity aborts: %lu\n\tHTM persistent aborts: %lu\n\tHTM user aborts: %lu\n\tHTM other aborts: %lu\n", tle_commits+gl_commits,tle_commits, gl_commits, conflicts+capacity+user+other,conflicts,trans,nontrans,self,capacity,persistent,user,other); \
}

#  define TM_THREAD_ENTER()
#  define TM_THREAD_EXIT()

# define IS_LOCKED(lock)        *((volatile int*)(&lock)) != 0

# define TM_BEGIN(b) TM_BEGIN_EXT(b,0)
# define SPEND_BUDGET(b)	if(RETRY_POLICY == 0) (*b)=0; else if (RETRY_POLICY == 2) (*b)=(*b)/2; else (*b)=--(*b);

# define TM_BEGIN_EXT(b,ro) { \
        int tle_budget = HTM_RETRIES; \
	local_exec_mode = 0; \
	backoff = MIN_BACKOFF; \
	local_thread_id = SPECIAL_THREAD_ID();\
        while (1) { \
            while (IS_LOCKED(single_global_lock)) { \
		__asm volatile ("" : : : "memory"); \
            } \
	    TM_buff_type TM_buff; \
            unsigned char status = __TM_begin(&TM_buff); \
            if (status == _HTM_TBEGIN_STARTED) { \
            	if (IS_LOCKED(single_global_lock)) { \
            		__TM_abort(); \
            	} \
            	break; \
            } \
	    else{ \
		if(__TM_is_failure_persistent(&TM_buff)){ \
			 SPEND_BUDGET(&tle_budget); \
			 stats_array[local_thread_id].persistent++; \
		} \
		if(__TM_is_conflict(&TM_buff)){ \
                        stats_array[local_thread_id].conflicts++; \
                        if(__TM_is_self_conflict(&TM_buff)) {stats_array[local_thread_id].self++; }\
                        else if(__TM_is_trans_conflict(&TM_buff)) stats_array[local_thread_id].trans++; \
                        else if(__TM_is_nontrans_conflict(&TM_buff)) stats_array[local_thread_id].nontrans++; \
                        tle_budget--; \
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
                        stats_array[local_thread_id].user++; \
                        tle_budget--; \
                } \
                else if(__TM_is_footprint_exceeded(&TM_buff)){ \
                        stats_array[local_thread_id].capacity++; \
			SPEND_BUDGET(&tle_budget); \
                } \
                else{ \
                        stats_array[local_thread_id].other++; \
                        tle_budget--; \
                } \
            } \
            if (tle_budget <= 0) {   \
		local_exec_mode = 2; \
        	while (pthread_spin_trylock(&single_global_lock) != 0) { \
                    __asm volatile ("" : : : "memory"); \
                } \
                break; \
	    } \
        } \
}


# define TM_END(){ \
    if (!local_exec_mode) { \
	__TM_end(); \
	stats_array[SPECIAL_THREAD_ID()].tle_commits++; \
    } else {    \
    	pthread_spin_unlock(&single_global_lock); \
	stats_array[SPECIAL_THREAD_ID()].gl_commits++; \
    } \
};

#    define TM_BEGIN_RO()                 TM_BEGIN(0)
#    define TM_RESTART()                  __TM_abort();
#    define TM_EARLY_RELEASE(var)


/*inline intptr_t TxLoad(intptr_t var);
inline intptr_t TxLoad(intptr_t var){
        return var;
}

inline intptr_t TxLoad_P(void* var);
inline intptr_t TxLoad_P(void* var){
        return var;
}*/

//#define FAST_PATH_SHARED_READ(var)               TxLoad((intptr_t)(var))  
//#define FAST_PATH_SHARED_READ_P(var)             TxLoad_P(var)
//#define FAST_PATH_SHARED_READ_D(var)             TxLoad((intptr_t)(var))



# define FAST_PATH_RESTART() __TM_abort();
# define FAST_PATH_SHARED_READ(var) (var)
# define FAST_PATH_SHARED_READ_P(var) (var)
# define FAST_PATH_SHARED_READ_D(var) (var)
# define FAST_PATH_SHARED_WRITE(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_P(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_D(var, val) ({var = val; var;})

# define SLOW_PATH_RESTART() FAST_PATH_RESTART()
# define SLOW_PATH_SHARED_READ(var)           FAST_PATH_SHARED_READ(var)
# define SLOW_PATH_SHARED_READ_P(var)         FAST_PATH_SHARED_READ_P(var)
# define SLOW_PATH_SHARED_READ_F(var)         FAST_PATH_SHARED_READ_D(var)
# define SLOW_PATH_SHARED_READ_D(var)         FAST_PATH_SHARED_READ_D(var)
# define SLOW_PATH_SHARED_WRITE(var, val)     FAST_PATH_SHARED_WRITE(var, val)
# define SLOW_PATH_SHARED_WRITE_P(var, val)   FAST_PATH_SHARED_WRITE_P(var, val)
# define SLOW_PATH_SHARED_WRITE_D(var, val)   FAST_PATH_SHARED_WRITE_D(var, val)

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})


#endif
