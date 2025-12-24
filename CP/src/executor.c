#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "scheduler.h"

void* job_thread(void* arg) {
    Job* job = (Job*)arg;
    Barrier* bar = NULL;

    pthread_mutex_lock(&dag_mutex);
    if (global_error) {
        pthread_mutex_unlock(&dag_mutex);
        sem_post(&parallel_sem);
        return NULL;
    }
    
    if (strlen(job->barrier) > 0) {
        bar = find_barrier(job->barrier);
    }
    pthread_mutex_unlock(&dag_mutex);

    printf("Starting job: %s\n", job->name);
    
    int ret = system(job->command);
    
    pthread_mutex_lock(&dag_mutex);
    
    printf("Job %s finished with exit code %d\n", job->name, ret);
    
    if (ret != 0) {
        if (!global_error) {
            global_error = 1;
            job->failed = 1;
            printf("Job %s failed! Stopping DAG...\n", job->name);
        }
        job->completed = 1;
        pthread_mutex_unlock(&dag_mutex);
        
        if (bar) {
            pthread_mutex_lock(&bar->lock);
            bar->arrived++;
            if (bar->arrived == bar->total_jobs) {
                for (int i = 0; i < bar->total_jobs; i++) {
                    sem_post(&bar->sem);
                }
            }
            pthread_mutex_unlock(&bar->lock);
        }
        
        sem_post(&parallel_sem);
        return NULL;
    }
    
    job->completed = 1;
    pthread_mutex_unlock(&dag_mutex);

    if (bar) {
        printf("Job %s arriving at barrier %s\n", job->name, bar->name);
        pthread_mutex_lock(&bar->lock);
        bar->arrived++;
        
        if (bar->arrived == bar->total_jobs) {
            printf("Barrier %s released! All %d jobs arrived\n", 
                   bar->name, bar->total_jobs);
            for (int i = 0; i < bar->total_jobs; i++) {
                sem_post(&bar->sem);
            }
        }
        pthread_mutex_unlock(&bar->lock);
        
        pthread_mutex_lock(&dag_mutex);
        int should_wait = !global_error;
        pthread_mutex_unlock(&dag_mutex);
        
        if (should_wait) {
            sem_wait(&bar->sem);
            printf("Job %s passed barrier %s\n", job->name, bar->name);
        } else {
            printf("Job %s skipping barrier due to error\n", job->name);
        }
    }

    sem_post(&parallel_sem);
    return NULL;
}

void run_dag() {
    sem_init(&parallel_sem, 0, max_parallel);
    
    for (int i = 0; i < barrier_count; i++) {
        barriers[i].arrived = 0;
        barriers[i].total_jobs = 0;
        sem_init(&barriers[i].sem, 0, 0);
    }
    
    for (int i = 0; i < job_count; i++) {
        if (strlen(jobs[i].barrier) > 0) {
            Barrier* bar = find_barrier(jobs[i].barrier);
            if (bar) {
                bar->total_jobs++;
            }
        }
    }
    
    int remaining = job_count;
    int running[MAX_JOBS] = {0};
    int started_jobs = 0;

    while (remaining > 0 && !global_error) {
        for (int i = 0; i < job_count; i++) {
            if (!jobs[i].completed && !running[i] && all_deps_completed(i)) {
                pthread_mutex_lock(&dag_mutex);
                int can_start = !global_error;
                pthread_mutex_unlock(&dag_mutex);
                
                if (can_start && sem_trywait(&parallel_sem) == 0) {
                    running[i] = 1;
                    started_jobs++;
                    pthread_create(&jobs[i].thread, NULL, job_thread, &jobs[i]);
                    pthread_detach(jobs[i].thread);
                    printf("Launched job %s (%d/%d started)\n", 
                           jobs[i].name, started_jobs, job_count);
                }
            }
        }
        
        for (int i = 0; i < job_count; i++) {
            if (running[i] && jobs[i].completed) {
                running[i] = 0;
                remaining--;
            }
        }
    }

    if (global_error) {
        printf("\n=== DAG INTERRUPTED DUE TO JOB FAILURE ===\n");
        
        for (int i = 0; i < barrier_count; i++) {
            pthread_mutex_lock(&barriers[i].lock);
            for (int j = 0; j < barriers[i].total_jobs; j++) {
                sem_post(&barriers[i].sem);
            }
            pthread_mutex_unlock(&barriers[i].lock);
        }
        
        while (remaining > 0) {
            for (int i = 0; i < job_count; i++) {
                if (running[i] && jobs[i].completed) {
                    running[i] = 0;
                    remaining--;
                }
            }
        }
    } else {
        printf("\n=== DAG COMPLETED SUCCESSFULLY ===\n");
    }

    sem_destroy(&parallel_sem);
}
