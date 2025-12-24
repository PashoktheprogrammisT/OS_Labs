#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scheduler.h"

Job jobs[MAX_JOBS];
Barrier barriers[MAX_BARRIERS];
int job_count = 0;
int barrier_count = 0;
int max_parallel = 1;
int global_error = 0;
pthread_mutex_t dag_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t parallel_sem;

void trim(char* str) {
    int i = 0, j = 0;
    while (str[i] && isspace((unsigned char)str[i])) i++;
    while (str[i]) str[j++] = str[i++];
    str[j] = 0;
    while (j > 0 && isspace((unsigned char)str[j-1])) str[--j] = 0;
}

Barrier* find_barrier(char* name) {
    for (int i = 0; i < barrier_count; i++) {
        if (strcmp(barriers[i].name, name) == 0) return &barriers[i];
    }
    return NULL;
}

int parse_ini(char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    char line[MAX_LINE];
    char section[MAX_NAME] = "";
    char current_job[MAX_NAME] = "";
    int in_job_section = 0;

    while (fgets(line, MAX_LINE, f)) {
        char* trimmed = line;
        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
        char* end = trimmed + strlen(trimmed) - 1;
        while (end > trimmed && isspace((unsigned char)*end)) *end-- = '\0';
        
        if (trimmed[0] == ';' || trimmed[0] == '#' || trimmed[0] == '\0') continue;
        
        if (trimmed[0] == '[' && strchr(trimmed, ']')) {
            char* close_bracket = strchr(trimmed, ']');
            *close_bracket = '\0';
            strncpy(section, trimmed + 1, MAX_NAME - 1);
            section[MAX_NAME - 1] = '\0';
            
            if (strcmp(section, "dag") == 0) {
                in_job_section = 0;
            } else if (strncmp(section, "job:", 4) == 0) {
                in_job_section = 1;
                strncpy(current_job, section + 4, MAX_NAME - 1);
                current_job[MAX_NAME - 1] = '\0';
                
                if (job_count < MAX_JOBS) {
                    strncpy(jobs[job_count].name, current_job, MAX_NAME - 1);
                    jobs[job_count].name[MAX_NAME - 1] = '\0';
                    jobs[job_count].dep_count = 0;
                    jobs[job_count].completed = 0;
                    jobs[job_count].failed = 0;
                    jobs[job_count].barrier[0] = '\0';
                    jobs[job_count].command[0] = '\0';
                    job_count++;
                }
            } else if (strncmp(section, "barrier:", 8) == 0) {
                in_job_section = 0;
                if (barrier_count < MAX_BARRIERS) {
                    strncpy(barriers[barrier_count].name, section + 8, MAX_NAME - 1);
                    barriers[barrier_count].name[MAX_NAME - 1] = '\0';
                    barriers[barrier_count].arrived = 0;
                    barriers[barrier_count].total_jobs = 0;
                    pthread_mutex_init(&barriers[barrier_count].lock, NULL);
                    barrier_count++;
                }
            }
        } else if (strchr(trimmed, '=')) {
            char* equals = strchr(trimmed, '=');
            *equals = '\0';
            char* key = trimmed;
            char* value = equals + 1;
            
            while (*key && isspace((unsigned char)*key)) key++;
            char* key_end = key + strlen(key) - 1;
            while (key_end > key && isspace((unsigned char)*key_end)) *key_end-- = '\0';
            
            while (*value && isspace((unsigned char)*value)) value++;
            char* value_end = value + strlen(value) - 1;
            while (value_end > value && isspace((unsigned char)*value_end)) *value_end-- = '\0';
            
            if (strcmp(section, "dag") == 0) {
                if (strcmp(key, "max_parallel") == 0) {
                    max_parallel = atoi(value);
                }
            } else if (in_job_section) {
                for (int i = 0; i < job_count; i++) {
                    if (strcmp(jobs[i].name, current_job) == 0) {
                        if (strcmp(key, "command") == 0) {
                            strncpy(jobs[i].command, value, MAX_LINE - 1);
                            jobs[i].command[MAX_LINE - 1] = '\0';
                        } else if (strcmp(key, "deps") == 0) {
                            if (strlen(value) > 0) {
                                char* dep = strtok(value, ",");
                                while (dep && jobs[i].dep_count < MAX_DEPS) {
                                    char* dep_trim = dep;
                                    while (*dep_trim && isspace((unsigned char)*dep_trim)) dep_trim++;
                                    char* dep_end = dep_trim + strlen(dep_trim) - 1;
                                    while (dep_end > dep_trim && isspace((unsigned char)*dep_end)) *dep_end-- = '\0';
                                    
                                    if (strlen(dep_trim) > 0) {
                                        strncpy(jobs[i].dependencies[jobs[i].dep_count], dep_trim, MAX_NAME - 1);
                                        jobs[i].dependencies[jobs[i].dep_count][MAX_NAME - 1] = '\0';
                                        jobs[i].dep_count++;
                                    }
                                    dep = strtok(NULL, ",");
                                }
                            }
                        } else if (strcmp(key, "barrier") == 0) {
                            strncpy(jobs[i].barrier, value, MAX_NAME - 1);
                            jobs[i].barrier[MAX_NAME - 1] = '\0';
                        }
                        break;
                    }
                }
            }
        }
    }
    
    fclose(f);
    return 1;
}
