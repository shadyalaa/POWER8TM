#ifndef TM_H
#define TM_H 1

#ifdef HAVE_CONFIG_H
# include "STAMP_config.h"
#endif

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

# define SETUP_NUMBER_TASKS(n)
# define SETUP_NUMBER_THREADS(n)
# define PRINT_STATS()


#  include <string.h>
#  include <stm.h>
#  include "thread.h"
#  include "norec.h"

#    define TM_ARG                        STM_SELF,
#    define TM_ARG_ALONE                  STM_SELF
#    define TM_ARGDECL                    STM_THREAD_T* TM_ARG
#    define TM_ARGDECL_ALONE              STM_THREAD_T* TM_ARG_ALONE
#    define TM_CALLABLE                   /* nothing */

#      define TM_STARTUP(numThread,nb)     STM_STARTUP()
#      define TM_SHUTDOWN()             STM_SHUTDOWN()

#      define TM_THREAD_ENTER()         TM_ARGDECL_ALONE = STM_NEW_THREAD(); \
                                        STM_INIT_THREAD(TM_ARG_ALONE, thread_getId())
#      define TM_THREAD_EXIT()          STM_FREE_THREAD(TM_ARG_ALONE)

#      define P_MALLOC(size)            malloc(size)
#      define P_FREE(ptr)               free(ptr) 
#      define TM_MALLOC(size)           malloc(size)
#      define FAST_PATH_FREE(ptr)        
#      define SLOW_PATH_FREE(ptr)       

# define AL_LOCK(idx)					 /* nothing */
#    define TM_BEGIN_EXT(b,ro)		local_exec_mode = 3;TM_BEGIN(ro)
#    define TM_BEGIN(ro)                local_exec_mode = 3;if(ro)STM_BEGIN_RD(); else STM_BEGIN_WR();
#    define TM_BEGIN_RO()               local_exec_mode = 3;STM_BEGIN_RD()
#    define TM_END()                    STM_END()
#    define FAST_PATH_RESTART()                STM_RESTART()
#    define SLOW_PATH_RESTART()                STM_RESTART()


#    define TM_EARLY_RELEASE(var)       /* nothing */


# define FAST_PATH_SHARED_READ(var) (var)
# define FAST_PATH_SHARED_READ_P(var) (var)
# define FAST_PATH_SHARED_READ_D(var) (var)
# define FAST_PATH_SHARED_WRITE(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_P(var, val) ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_D(var, val) ({var = val; var;})


#  define SLOW_PATH_SHARED_READ(var)           STM_READ(var)
#  define SLOW_PATH_SHARED_READ_P(var)         STM_READ_P(var)
#  define SLOW_PATH_SHARED_READ_D(var)         STM_READ_F(var)
#  define SLOW_PATH_SHARED_WRITE(var, val)     STM_WRITE((var), val)
#  define SLOW_PATH_SHARED_WRITE_P(var, val)   STM_WRITE_P((var), val)
#  define SLOW_PATH_SHARED_WRITE_D(var, val)   STM_WRITE_F((var), val)

#  define TM_LOCAL_WRITE(var, val)      STM_LOCAL_WRITE(var, val)
#  define TM_LOCAL_WRITE_P(var, val)    STM_LOCAL_WRITE_P(var, val)
#  define TM_LOCAL_WRITE_D(var, val)    STM_LOCAL_WRITE_F(var, val)

#endif
