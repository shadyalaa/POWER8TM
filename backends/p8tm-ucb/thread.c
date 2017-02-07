/* =============================================================================
 *
 * thread.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "thread.h"

#include <math.h>

__thread unsigned int thread_id;



#ifndef REDUCED_TM_API

#include <assert.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include "types.h"
#include "random.h"
//#include "rapl.h"

static THREAD_LOCAL_T    global_threadId;
static THREAD_BARRIER_T* global_barrierPtr      = NULL;
static long*             global_threadIds       = NULL;
static THREAD_ATTR_T     global_threadAttr;
static THREAD_T*         global_threads         = NULL;
static void            (*global_funcPtr)(void*) = NULL;
static void*             global_argPtr          = NULL;
static volatile bool_t   global_doShutdown      = FALSE;

long              global_numThread;

static void
threadWait (void* argPtr)
{
    long threadId = *(long*)argPtr;

    THREAD_LOCAL_SET(global_threadId, (long)threadId);

    bindThread(threadId);

    thread_id = threadId;

    while (1) {
        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for start parallel */
        if (global_doShutdown) {
            break;
        }
        global_funcPtr(global_argPtr);
        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for end parallel */
        if (threadId == 0) {
            break;
        }
    }
}

/* =============================================================================
 * thread_startup
 * -- Create pool of secondary threads
 * -- numThread is total number of threads (primary + secondaries)
 * =============================================================================
 */
void
thread_startup (long numThread)
{
    int i;
    global_numThread = numThread;
    global_doShutdown = FALSE;


    /* Set up barrier */
    assert(global_barrierPtr == NULL);
    global_barrierPtr = THREAD_BARRIER_ALLOC(numThread);
    assert(global_barrierPtr);
    THREAD_BARRIER_INIT(global_barrierPtr, numThread);

    /* Set up ids */
    THREAD_LOCAL_INIT(global_threadId);
    assert(global_threadIds == NULL);
    global_threadIds = (long*)malloc(numThread * sizeof(long));
    assert(global_threadIds);
    for (i = 0; i < numThread; i++) {
        global_threadIds[i] = i;
    }

    /* Set up thread list */
    assert(global_threads == NULL);
    global_threads = (THREAD_T*)malloc(numThread * sizeof(THREAD_T));
    assert(global_threads);

    //writers_lock.value = 0;

    /* Set up pool */
    THREAD_ATTR_INIT(global_threadAttr);
    for (i = 1; i < numThread; i++) {
        THREAD_CREATE(global_threads[i],
                      global_threadAttr,
                      &threadWait,
                      &global_threadIds[i]);
    }
}

void
thread_start (void (*funcPtr)(void*), void* argPtr)
{
    global_funcPtr = funcPtr;
    global_argPtr = argPtr;

    long threadId = 0; /* primary */
    threadWait((void*)&threadId);
}


void
thread_shutdown ()
{
    /* Make secondary threads exit wait() */
    global_doShutdown = TRUE;
    THREAD_BARRIER(global_barrierPtr, 0);

    long numThread = global_numThread;

    long i;
    for (i = 1; i < numThread; i++) {
        THREAD_JOIN(global_threads[i]);
    }

	global_numThread = 1;

    THREAD_BARRIER_FREE(global_barrierPtr);
    global_barrierPtr = NULL;

    free(global_threadIds);
    global_threadIds = NULL;

    free(global_threads);
    global_threads = NULL;

}

barrier_t *barrier_alloc() {
    return (barrier_t *)malloc(sizeof(barrier_t));
}

void barrier_free(barrier_t *b) {
    free(b);
}

void barrier_init(barrier_t *b, int n) {
    pthread_cond_init(&b->complete, NULL);
    pthread_mutex_init(&b->mutex, NULL);
    b->count = n;
    b->crossing = 0;
}

void barrier_cross(barrier_t *b) {
    pthread_mutex_lock(&b->mutex);
    /* One more thread through */
    b->crossing++;
    /* If not all here, wait */
    if (b->crossing < b->count) {
        pthread_cond_wait(&b->complete, &b->mutex);
    } else {
        /* Reset for next time */
        b->crossing = 0;
        pthread_cond_broadcast(&b->complete);
    }
    pthread_mutex_unlock(&b->mutex);
}

void
thread_barrier_wait()
{
    long threadId = thread_getId();
    THREAD_BARRIER(global_barrierPtr, threadId);
}

long
thread_getId()
{
    return (long)THREAD_LOCAL_GET(global_threadId);
}

long
thread_getNumThread()
{
    return global_numThread;
}


void disable_htm_rot(){
	allow_rots_ros = 0; 
        rmb();
        int index;
        int num_threads = global_numThread; 
        for(index=0; index < num_threads; index++){ 
	        while(counters[index].value & 1){ 
        	        cpu_relax(); 
                } 
        } 
	rmb(); 
        htm_rot_enabled = 0;
	allow_htms = 1;
	rmb();
}

void enable_htm_rot(){
        htm_rot_enabled = 1;
        /*rmb();
	int index;
        int num_threads = global_numThread; 
        for(index=0; index < num_threads; index++){ 
                while((counters[index].value & 7)==6){ 
                        cpu_relax(); 
                } 
        }*/ 
	rmb(); 
	allow_htms = 1;
	allow_rots_ros = 1;
}

void disable_htm(){
	allow_htms = 0;
}

unsigned int select_best(double* ucb_rewards,int ucb_levers){
	double best = 0.0;
	int i; unsigned int best_index;
	for(i=0;i<ucb_levers;i++){
		if(ucb_rewards[i]>best){
			best = ucb_rewards[i];
			best_index = i;
		}
	}
	return best_index;

}

unsigned long get_commits(){
		unsigned long commits = 0;
		int i;
		for (i= 0; i < 80; i++){
                        commits += stats_array[i].read_commits;
                        commits += stats_array[i].htm_commits;
                        commits += stats_array[i].rot_commits;
                        commits += stats_array[i].gl_commits;
                }
		return commits;
}

void start_monitoring(){

        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(79, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	unsigned int current_lever = 0;
	ucb_levers = 3;
	int explore = 1;
	int sampling_period = 100;
	unsigned int prev_lever;
	double ucb_rewards[ucb_levers];
	unsigned long ucb_commits[ucb_levers];
	total_trials = 0;
	unsigned long commits,pre_commits,post_commits,best_commits=0,total_commits=0;
	int i;
	for(i=0;i<ucb_levers;i++){
		ucb_rewards[i] = 0.0;
		ucb_trials[i] = 0;
		ucb_commits[i] = 0;
	}
	//sleep(100);
	//usleep(10000);
	while(!global_doShutdown){
		pre_commits = get_commits();
		usleep(sampling_period);
		post_commits = get_commits();
		commits = post_commits-pre_commits;
		//printf("commits are %ld - %ld = %d\n",post_commits,pre_commits,commits);
		if(!commits)
			continue;
		if(commits<30){
			sampling_period *= 2;
			continue;
		}
		//commits_stats[total_trials] = commits;
		
		//printf("commits are: %d\t%d\n",commits,current_lever);
		if(commits>best_commits){
			best_commits = commits;
		}
		//printf("highest commits: %d\n",best_commits);

	
		total_trials++;

		ucb_trials[current_lever]++;
		ucb_commits[current_lever] += commits; 

		for(i=0;i<ucb_levers;i++){
			ucb_rewards[i] = ((1.0*ucb_commits[i])/(best_commits*ucb_trials[i])) + sqrt(2.0*(log10(total_trials)/ucb_trials[i])); 
			//printf("lever %d\t%lf\t%d\n",i,ucb_rewards[i],commits);
		}
		

                //printf("lever %d: %lf\t%lf\t%lu\t%lf\t%d\t%d\t%ld\n",current_lever,ucb_rewards[current_lever],1.0*ucb_commits[current_lever]/(best_commits*ucb_trials[current_lever]),commits,sqrt(2.0*(log10(total_trials)/ucb_trials[current_lever])),ucb_trials[current_lever],best_commits,total_trials);


		prev_lever = current_lever;
		//cpu_relax();	
		if(explore){
			current_lever++;
			if(current_lever == ucb_levers){
				explore = 0;
				current_lever = select_best(ucb_rewards,ucb_levers);
				//printf("best lever is %d\t%d\n",current_lever,commits);
				//exit(0);
			}
		}
		else{
			current_lever = select_best(ucb_rewards,ucb_levers);
			//printf("best lever is %d\t%d\n",current_lever,commits);
		}

		//selected_lever[total_trials] = current_lever;
		if(current_lever != prev_lever){
			if(current_lever == 1){
				if(htm_rot_enabled)
					disable_htm_rot();
			}
			else if(current_lever == 0){
				if(!htm_rot_enabled)
					enable_htm_rot();					
			}
			else{
				if (prev_lever)
					enable_htm_rot();
				disable_htm(); 
			}
		}

	}

	pthread_exit(NULL);
}


void ucb_start(){
	pthread_t t1;
	pthread_create(&t1, NULL, start_monitoring, NULL);
}

#endif
