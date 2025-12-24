#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>
#include <semaphore.h>

#define MAX_JOBS 100
#define MAX_NAME 50
#define MAX_LINE 256
#define MAX_DEPS 10
#define MAX_BARRIERS 10

typedef struct {
    char name[MAX_NAME];
    char command[MAX_LINE];
    char dependencies[MAX_DEPS][MAX_NAME];
    int dep_count;
    char barrier[MAX_NAME];
    pthread_t thread;
    int completed;
    int failed;
} Job;

typedef struct {
    char name[MAX_NAME];
    sem_t sem;
    int arrived;
    int total_jobs;
    pthread_mutex_t lock;
} Barrier;

extern Job jobs[MAX_JOBS];
extern Barrier barriers[MAX_BARRIERS];
extern int job_count;
extern int barrier_count;
extern int max_parallel;
extern int global_error;
extern pthread_mutex_t dag_mutex;
extern sem_t parallel_sem;

void trim(char* str);
int find_job_index(char* name);
Barrier* find_barrier(char* name);
int has_cycle();
int is_single_component();
int has_start_and_end();
int all_deps_completed(int idx);
void* job_thread(void* arg);
void run_dag();
int parse_ini(char* filename);

#endif
