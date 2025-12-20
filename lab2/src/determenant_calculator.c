#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int THREAD_CAP = 4;
int USE_FIXED_SEED = 0;
unsigned int CUSTOM_SEED = 0;

typedef struct {
    int worker_id;
    int dim;
    double *mat;
    long long from_idx;
    long long to_idx;
    double worker_total;
} WorkerInfo;

long long get_fact(int x) {
    long long res = 1;
    int i;
    for (i = 2; i <= x; i++) res = res * i;
    return res;
}

int get_parity(int *seq, int len) {
    int swaps = 0;
    int i, j;
    for (i = 0; i < len; i++) {
        for (j = i + 1; j < len; j++) {
            if (seq[i] > seq[j]) swaps++;
        }
    }
    return (swaps % 2) ? -1 : 1;
}

void generate_seq(long long idx, int *output, int n) {
    int *used = (int*)calloc(n, sizeof(int));
    if (!used) {
        fprintf(stderr, "Ошибка выделения памяти в generate_seq\n");
        return;
    }
    
    int i, j;
    for (i = 0; i < n; i++) {
        int pos = idx % (n - i);
        idx = idx / (n - i);
        
        int cnt = 0;
        for (j = 0; j < n; j++) {
            if (!used[j]) {
                if (cnt == pos) {
                    output[i] = j;
                    used[j] = 1;
                    break;
                }
                cnt++;
            }
        }
    }
    
    free(used);
}

void* compute_slice(void *ptr) {
    WorkerInfo *wi = (WorkerInfo*)ptr;
    int n = wi->dim;
    double *m = wi->mat;
    
    int *current_seq = (int*)malloc(n * sizeof(int));
    if (!current_seq) {
        fprintf(stderr, "Поток %d: не удалось выделить память\n", wi->worker_id);
        wi->worker_total = 0.0;
        return NULL;
    }
    
    double slice_sum = 0.0;
    long long k;

    for (k = wi->from_idx; k < wi->to_idx; k++) {
        generate_seq(k, current_seq, n);
        
        double term = 1.0;
        int row;
        for (row = 0; row < n; row++) {
            term = term * m[row * n + current_seq[row]];
        }
        
        slice_sum += get_parity(current_seq, n) * term;
    }
    
    wi->worker_total = slice_sum;
    free(current_seq);
    return NULL;
}

void print_usage(const char *prog_name) {
    printf("Использование: %s <макс_потоков> <размер_матрицы> [опции]\n", prog_name);
    printf("\nОпции:\n");
    printf("  -f [SEED]    Фиксированный seed (по умолчанию 42)\n");
    printf("  -s SEED      Конкретный seed значение\n");
    printf("  -h           Справка\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    THREAD_CAP = atoi(argv[1]);
    int mat_size = atoi(argv[2]);
    
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-f") == 0) {
            USE_FIXED_SEED = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                CUSTOM_SEED = atoi(argv[i + 1]);
                i++;
            } else {
                CUSTOM_SEED = 42;
            }
        }
        else if (strcmp(argv[i], "-s") == 0) {
            USE_FIXED_SEED = 1;
            if (i + 1 < argc) {
                CUSTOM_SEED = atoi(argv[i + 1]);
                i++;
            } else {
                fprintf(stderr, "Ошибка: опция -s требует значения seed\n");
                return 1;
            }
        }
        else {
            fprintf(stderr, "Неизвестная опция: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (mat_size < 1 || mat_size > 12) {
        fprintf(stderr, "Ошибка: размер матрицы должен быть от 1 до 12\n");
        return 1;
    }
    
    if (THREAD_CAP < 1) {
        fprintf(stderr, "Ошибка: число потоков должно быть > 0\n");
        return 1;
    }
    
    double *M = (double*)calloc(mat_size * mat_size, sizeof(double));
    if (M == NULL) {
        perror("calloc");
        return 1;
    }
    
    if (USE_FIXED_SEED) {
        srand(CUSTOM_SEED);
        printf("Используется фиксированный seed: %u\n", CUSTOM_SEED);
    } else {
        unsigned int seed = time(NULL) ^ getpid();
        srand(seed);
        printf("Используется случайный seed: %u\n", seed);
    }
    
    printf("\nМатрица %d×%d:\n", mat_size, mat_size);
    int i, j;
    for (i = 0; i < mat_size; i++) {
        printf("  ");
        for (j = 0; j < mat_size; j++) {
            M[i * mat_size + j] = (rand() % 16) - 8;
            printf("%4.0f ", M[i * mat_size + j]);
        }
        printf("\n");
    }
    
    long long perm_count = get_fact(mat_size);
    printf("\nКоличество перестановок: %lld\n", perm_count);
    
    int real_threads = THREAD_CAP;
    if (real_threads > perm_count) real_threads = perm_count;
    
    pthread_t *thread_list = (pthread_t*)malloc(real_threads * sizeof(pthread_t));
    WorkerInfo *work_info = (WorkerInfo*)malloc(real_threads * sizeof(WorkerInfo));
    
    if (thread_list == NULL || work_info == NULL) {
        perror("malloc");
        free(M);
        return 1;
    }
    
    long long base_load = perm_count / real_threads;
    long long extra_load = perm_count % real_threads;
    long long cursor = 0;
    
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
    for (i = 0; i < real_threads; i++) {
        long long load = base_load + (i < extra_load ? 1 : 0);
        
        work_info[i].worker_id = i;
        work_info[i].dim = mat_size;
        work_info[i].mat = M;
        work_info[i].from_idx = cursor;
        work_info[i].to_idx = cursor + load;
        work_info[i].worker_total = 0.0;
        
        cursor += load;
        
        if (pthread_create(&thread_list[i], NULL, compute_slice, &work_info[i]) != 0) {
            fprintf(stderr, "Не удалось создать поток %d: ", i);
            perror(NULL);
            
            for (int j = 0; j < i; j++) {
                pthread_cancel(thread_list[j]);
            }
            free(M); free(thread_list); free(work_info);
            return 1;
        }
    }
    
    double final_det = 0.0;
    for (i = 0; i < real_threads; i++) {
        pthread_join(thread_list[i], NULL);
        final_det += work_info[i].worker_total;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + 
                     (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
    
    printf("\n=== РЕЗУЛЬТАТЫ ===\n");
    printf("  Определитель:   %12.4f\n", final_det);
    printf("  Время работы:   %12.6f с\n", elapsed);
    printf("  Использовано потоков: %4d из %d\n", real_threads, THREAD_CAP);
    printf("  Производительность:   %12.0f перестановок/с\n", 
           (elapsed > 0) ? perm_count / elapsed : 0);
    printf("  ID процесса:    %12d\n", getpid());
    
    free(M);
    free(thread_list);
    free(work_info);
    
    return 0;
}
