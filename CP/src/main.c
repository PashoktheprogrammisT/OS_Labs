#include <stdio.h>
#include "scheduler.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <config.ini>\n", argv[0]);
        return 1;
    }

    if (!parse_ini(argv[1])) {
        printf("Failed to parse config file\n");
        return 1;
    }

    if (job_count == 0) {
        printf("No jobs defined\n");
        return 1;
    }

    printf("Loaded %d jobs, %d barriers\n", job_count, barrier_count);
    printf("Max parallel jobs: %d\n", max_parallel);

    for (int i = 0; i < job_count; i++) {
        printf("Job %s: command='%s', deps=%d", 
               jobs[i].name, jobs[i].command, jobs[i].dep_count);
        if (jobs[i].dep_count > 0) {
            printf(" (");
            for (int d = 0; d < jobs[i].dep_count; d++) {
                printf("%s%s", jobs[i].dependencies[d], 
                       d < jobs[i].dep_count - 1 ? ", " : "");
            }
            printf(")");
        }
        printf(", barrier='%s'\n", jobs[i].barrier);
    }

    if (has_cycle()) {
        printf("DAG contains cycles\n");
        return 1;
    }
    printf("No cycles found\n");

    if (!is_single_component()) {
        printf("DAG is not a single connected component\n");
        return 1;
    }
    printf("Single connected component OK\n");

    if (!has_start_and_end()) {
        printf("DAG must have start and end jobs\n");
        return 1;
    }
    printf("Start and end jobs OK\n");

    printf("Starting DAG execution...\n");
    run_dag();

    for (int i = 0; i < barrier_count; i++) {
        sem_destroy(&barriers[i].sem);
        pthread_mutex_destroy(&barriers[i].lock);
    }
    pthread_mutex_destroy(&dag_mutex);

    return global_error;
}
