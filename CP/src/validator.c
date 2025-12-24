#include <stdio.h>
#include <string.h>
#include "scheduler.h"

int find_job_index(char* name) {
    for (int i = 0; i < job_count; i++) {
        if (strcmp(jobs[i].name, name) == 0) return i;
    }
    return -1;
}

int has_cycle_util(int v, int visited[], int rec_stack[]) {
    if (!visited[v]) {
        visited[v] = 1;
        rec_stack[v] = 1;
        for (int i = 0; i < jobs[v].dep_count; i++) {
            int dep_idx = find_job_index(jobs[v].dependencies[i]);
            if (dep_idx == -1) continue;
            if (!visited[dep_idx] && has_cycle_util(dep_idx, visited, rec_stack))
                return 1;
            else if (rec_stack[dep_idx])
                return 1;
        }
    }
    rec_stack[v] = 0;
    return 0;
}

int has_cycle() {
    int visited[MAX_JOBS] = {0};
    int rec_stack[MAX_JOBS] = {0};
    for (int i = 0; i < job_count; i++) {
        if (has_cycle_util(i, visited, rec_stack))
            return 1;
    }
    return 0;
}

void dfs(int v, int visited[]) {
    visited[v] = 1;
    
    for (int d = 0; d < jobs[v].dep_count; d++) {
        int dep_idx = find_job_index(jobs[v].dependencies[d]);
        if (dep_idx != -1 && !visited[dep_idx]) {
            dfs(dep_idx, visited);
        }
    }
    
    for (int i = 0; i < job_count; i++) {
        for (int d = 0; d < jobs[i].dep_count; d++) {
            if (strcmp(jobs[i].dependencies[d], jobs[v].name) == 0 && !visited[i]) {
                dfs(i, visited);
            }
        }
    }
}

int is_single_component() {
    if (job_count == 0) return 1;
    
    int visited[MAX_JOBS] = {0};
    
    dfs(0, visited);
    
    for (int i = 0; i < job_count; i++) {
        if (!visited[i]) return 0;
    }
    
    return 1;
}

int has_start_and_end() {
    int has_start = 0;
    int has_end = 0;
    
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].dep_count == 0) {
            has_start = 1;
        }
        
        int is_referenced = 0;
        for (int j = 0; j < job_count; j++) {
            for (int k = 0; k < jobs[j].dep_count; k++) {
                if (strcmp(jobs[j].dependencies[k], jobs[i].name) == 0) {
                    is_referenced = 1;
                    break;
                }
            }
            if (is_referenced) break;
        }
        
        if (!is_referenced) {
            has_end = 1;
        }
    }
    
    return has_start && has_end;
}

int all_deps_completed(int idx) {
    for (int i = 0; i < jobs[idx].dep_count; i++) {
        int dep_idx = find_job_index(jobs[idx].dependencies[i]);
        if (dep_idx == -1 || !jobs[dep_idx].completed)
            return 0;
    }
    return 1;
}
