//#include "rapl.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "computeGraph.h"
#include "defs.h"
#include "findSubGraphs.h"
#include "genScalData.h"
#include "getUserParameters.h"
#include "globals.h"
#include "timer.h"
#include "thread.h"
#include "tm.h"


unsigned int htm_rot_enabled = 0;
unsigned int allow_rots_ros = 0;
unsigned int allow_htms = 1;

unsigned int allow_stms = 0;

pthread_spinlock_t global_lock = 0;

__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t exists_sw;

__thread unsigned long backoff = MIN_BACKOFF;
__thread unsigned long cm_seed = 123456789UL;

//unsigned long commits_stats[0];

__attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t stats_array[80];

__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t single_global_lock = 0;
__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t fallback_in_use = 0;

__attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t counters[80];

__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t writers_lock = 0;

//__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t reader_locks[80];

//__attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t statistics_array[80];

__thread unsigned int local_exec_mode = 0;

__thread unsigned int local_thread_id;

__thread void* rot_readset[1024];
__thread char crot_readset[8192];
__thread int irot_readset[2048];
__thread int16_t i2rot_readset[4096];

__thread long rs_mask_2 = 0xffffffffffff0000;
__thread long rs_mask_4 = 0xffffffff00000000;
__thread long offset = 0;
__thread char* p;
__thread int* ip;
__thread int16_t* i2p;
__thread long moffset = 0;
__thread long moffset_2 = 0;
__thread long moffset_6 = 0;

__thread unsigned long rs_counter = 0;

unsigned int ucb_levers = 3;
unsigned long ucb_trials[3];
unsigned long total_trials;
unsigned int selected_lever[50000];

MAIN(argc, argv)
{
    GOTO_REAL();

    SETUP_NUMBER_TASKS(10);

    /*
     * Tuple for Scalable Data Generation
     * stores startVertex, endVertex, long weight and other info
     */
    graphSDG* SDGdata;

    /*
     * The graph data structure for this benchmark - see defs.h
     */
    graph* G;

    double totalTime = 0.0;

    /* -------------------------------------------------------------------------
     * Preamble
     * -------------------------------------------------------------------------
     */

    /*
     * User Interface: Configurable parameters, and global program control
     */

    getUserParameters(argc, (char** const) argv);


    SIM_GET_NUM_CPU(THREADS);
    TM_STARTUP(THREADS, SSCA2_ID);
    P_MEMORY_STARTUP(THREADS);
    SETUP_NUMBER_THREADS(THREADS);
    thread_startup(THREADS);

double time_total = 0.0;
double energy_total = 0.0;
int repeat = REPEATS;

for (; repeat > 0; --repeat) {

    SDGdata = (graphSDG*)malloc(sizeof(graphSDG));
    assert(SDGdata);

    genScalData_seq(SDGdata);

    G = (graph*)malloc(sizeof(graph));
    assert(G);

    computeGraph_arg_t computeGraphArgs;
    computeGraphArgs.GPtr       = G;
    computeGraphArgs.SDGdataPtr = SDGdata;

TIMER_T start;
    //startEnergyIntel();
    TIMER_READ(start);

    GOTO_SIM();
    thread_start(computeGraph, (void*)&computeGraphArgs);
    GOTO_REAL();
TIMER_T stop;
    TIMER_READ(stop);
double time_tmp = TIMER_DIFF_SECONDS(start, stop);
//double energy_tmp = endEnergyIntel();
double energy_tmp = 0;
printf("summary\t%lf\t%lf\n", time_tmp, energy_tmp);
PRINT_STATS();
time_total += time_tmp;
energy_total += energy_tmp;
}

totalTime += time_total;
printf("Energy = %0.6lf\n", energy_total);

    printf("\nTime taken for all is %9.6f sec.\n\n", totalTime);

    /* -------------------------------------------------------------------------
     * Cleanup
     * -------------------------------------------------------------------------
     */

    P_FREE(G->outDegree);
    P_FREE(G->outVertexIndex);
    P_FREE(G->outVertexList);
    P_FREE(G->paralEdgeIndex);
    P_FREE(G->inDegree);
    P_FREE(G->inVertexIndex);
    P_FREE(G->inVertexList);
    P_FREE(G->intWeight);
    P_FREE(G->strWeight);

    P_FREE(SOUGHT_STRING);
    P_FREE(G);
    P_FREE(SDGdata);

    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

    MAIN_RETURN(0);
}
