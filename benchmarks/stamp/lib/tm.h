#ifndef TM_H
#define TM_H 1

#  include <stdio.h>

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

# define AL_LOCK(b)
# define PRINT_STATS()
# define SETUP_NUMBER_TASKS(b)
# define SETUP_NUMBER_THREADS(b)

#  include <assert.h>
#ifndef REDUCED_TM_API
#  include "memory.h"
#  include "thread.h"
#  include "types.h"
#endif

//#include <immintrin.h>
//#include <rtmintrin.h>
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

#include "stm.h"
#include "norec.h"

# define INIT_FUN_PTRS                  /*abortFunPtr = &abortHTM; \
                                        sharedReadFunPtr = &sharedReadHTM;  \
                                        sharedWriteFunPtr = &sharedWriteHTM; \
                                        freeFunPtr = &freeHTM;*/

#ifdef REDUCED_TM_API
#    define Self                        TM_ARG_ALONE
#    define TM_ARG_ALONE                get_thread()
#    define SPECIAL_THREAD_ID()         get_tid()
#    define SPECIAL_INIT_THREAD(id)     thread_desc[id] = (void*)TM_ARG_ALONE;
#    define TM_THREAD_ENTER()         Thread* inited_thread = STM_NEW_THREAD(); \
                                      STM_INIT_THREAD(inited_thread, SPECIAL_THREAD_ID()); \
                                      thread_desc[SPECIAL_THREAD_ID()] = (void*)inited_thread; \
                                      INIT_FUN_PTRS
#else
#    define TM_ARG_ALONE                  STM_SELF
#    define SPECIAL_THREAD_ID()         thread_getId()
#    define TM_ARGDECL                    STM_THREAD_T* TM_ARG
#    define TM_ARGDECL_ALONE              STM_THREAD_T* TM_ARG_ALONE
#    define TM_THREAD_ENTER()         TM_ARGDECL_ALONE = STM_NEW_THREAD(); \
                                      STM_INIT_THREAD(TM_ARG_ALONE, SPECIAL_THREAD_ID()); \
                                      INIT_FUN_PTRS
#endif





#    define TM_CALLABLE                   /* nothing */
#    define TM_ARG                        TM_ARG_ALONE,
#    define TM_THREAD_EXIT()          STM_FREE_THREAD(TM_ARG_ALONE)

#      define TM_STARTUP(numThread,u)     STM_STARTUP()
#      define TM_SHUTDOWN() { \
    STM_SHUTDOWN(); \
    unsigned long tle_commits = 0; \
    unsigned long stm_commits = 0; \
    unsigned long conflicts = 0; \
    unsigned long self = 0; \
    unsigned long trans = 0; \
    unsigned long nontrans = 0; \
    unsigned long capacity = 0; \
    unsigned long user = 0; \
    unsigned long persistent = 0; \
    unsigned long other = 0; \
    unsigned long stm_aborts = 0; \
    int i = 0; \
    for (; i < 80; i++) { \
       if (stats_array[i].tle_commits+stats_array[i].stm_commits == 0) { break; } \
        tle_commits += stats_array[i].tle_commits; \
        stm_commits += stats_array[i].stm_commits; \
        conflicts += stats_array[i].conflicts; \
        self += stats_array[i].self; \
        trans += stats_array[i].trans; \
        nontrans += stats_array[i].nontrans; \
        capacity += stats_array[i].capacity; \
        user += stats_array[i].user; \
        persistent += stats_array[i].persistent; \ 
        other += stats_array[i].other; \
	stm_aborts += stats_array[i].stm_aborts; \
    } \
    printf("Total commits: %lu\n\tHTM commits: %lu\n\tSTM commits: %lu\nTotal aborts: %lu\n\tHTM conflict aborts: %lu\n\t\tHTM trans conflicts: %lu\n\t\tHTM non-trans conflicts: %lu\n\t\tHTM self conflicts: %lu\n\tHTM capacity aborts: %lu\n\tHTM persistent aborts: %lu\n\tHTM user aborts: %lu\n\tHTM other aborts: %lu\n\tSTM aborts: %lu\n", tle_commits+stm_commits,tle_commits, stm_commits, stm_aborts+conflicts+capacity+user+other,conflicts,trans,nontrans,self,capacity,persistent,user,other,stm_aborts); \
}

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

# define TM_BEGIN(ro)     TM_BEGIN_EXT(0, ro)
# define SPEND_BUDGET(b)	if(RETRY_POLICY == 0) (*b)=0; else if (RETRY_POLICY == 2) (*b)=(*b)/2; else (*b)=--(*b);

#    define TM_BEGIN_EXT(b,ro)    \
    { \
	backoff = MIN_BACKOFF; \
        int tle_budget = HTM_RETRIES; \
	local_exec_mode = 0; \
	local_thread_id = SPECIAL_THREAD_ID(); \ 
        while (1) { \
            if (tle_budget > 0) { \
                while (fallback_in_use != 0) { __asm volatile ("" : : : "memory"); } \
		TM_buff_type TM_buff; \
                unsigned char status = __TM_begin(&TM_buff);    \
                if (status == _HTM_TBEGIN_STARTED) { \
                    if (fallback_in_use != 0) { __TM_abort(); } \
                    break;  \
                } \
                else {\
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
                        tle_budget--; \
                } \
                else{ \
                        stats_array[local_thread_id].other++; \
                        tle_budget--; \
                } \
                } \
            } else {  \
		local_exec_mode = 3; \
                __sync_add_and_fetch(&exists_sw.counter,1); \
                STM_BEGIN(ro);   \
                stats_array[local_thread_id].stm_aborts++; \
                break;  \
            } \
        } \
}

#    define TM_END(){  \
        if (local_exec_mode == 0) {    \
            if (!ro && exists_sw.counter) { \
                HTM_INC_CLOCK(); \
            } \
            __TM_end();    \
	   stats_array[local_thread_id].tle_commits++; \
        } else {    \
            __sync_add_and_fetch(&fallback_in_use,1);   \
            int ret = HYBRID_STM_END();  \
            __sync_sub_and_fetch(&fallback_in_use,1);    \
            if (ret == 0) { \
                STM_RESTART(); \
            } \
            __sync_sub_and_fetch(&exists_sw.counter,1); \
            stats_array[local_thread_id].stm_aborts--; \
	    stats_array[local_thread_id].stm_commits++; \
        } \
    };



#    define TM_EARLY_RELEASE(var)         


#      define P_MALLOC(size)            malloc(size)
#      define P_FREE(ptr)               free(ptr)
#      define SEQ_MALLOC(size)          malloc(size)
#      define SEQ_FREE(ptr)             free(ptr)

#      define TM_MALLOC(size)           malloc(size)
#      define FAST_PATH_FREE(ptr)       free(ptr) //(*freeFunPtr)((void*)ptr)
#      define SLOW_PATH_FREE(ptr)       //(*freeFunPtr)((void*)ptr)


# define FAST_PATH_RESTART() __TM_abort()
# define FAST_PATH_SHARED_READ(var) var
# define FAST_PATH_SHARED_READ_P(var) var
# define FAST_PATH_SHARED_READ_D(var) var
# define FAST_PATH_SHARED_WRITE(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_P(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_D(var, val) ({var = val; var;})

#  define SLOW_PATH_RESTART()                  STM_RESTART()
#  define SLOW_PATH_SHARED_READ(var)           STM_READ(var)
#  define SLOW_PATH_SHARED_READ_P(var)         STM_READ_P(var)
#  define SLOW_PATH_SHARED_READ_D(var)         STM_READ_F(var)
#  define SLOW_PATH_SHARED_WRITE(var, val)     STM_WRITE((var), val)
#  define SLOW_PATH_SHARED_WRITE_P(var, val)   STM_WRITE_P((var), val)
#  define SLOW_PATH_SHARED_WRITE_D(var, val)   STM_WRITE_F((var), val)

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})

#endif /* TM_H */
