#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

typedef float (*DerivativeFunc)(float, float);
typedef int* (*SortFunc)(int*);

void* lib_handle = NULL;
DerivativeFunc Derivative = NULL;
SortFunc Sort = NULL;
int current_lib = 1;

void load_library(const char* lib_name) {
    if (lib_handle) dlclose(lib_handle);
    
    lib_handle = dlopen(lib_name, RTLD_LAZY);
    if (!lib_handle) {
        fprintf(stderr, "Ошибка загрузки: %s\n", dlerror());
        return;
    }
    
    Derivative = (DerivativeFunc)dlsym(lib_handle, "Derivative");
    Sort = (SortFunc)dlsym(lib_handle, "Sort");
    
    if (!Derivative || !Sort) {
        fprintf(stderr, "Ошибка загрузки функций: %s\n", dlerror());
        dlclose(lib_handle);
        lib_handle = NULL;
        return;
    }
}

int main() {
    char line[256];
    
    printf("=== Программа 2: Динамическая загрузка ===\n");
    printf("Начальные реализации:\n");
    printf("  Функция 1: Производная cos(x) (f'(x) = (f(A+Δx)-f(A))/Δx)\n");
    printf("  Функция 2: Пузырьковая сортировка\n");
    printf("Формат ввода:\n");
    printf("  0                   - переключить реализацию\n");
    printf("  1 A deltaX          - производная cos(x) в точке A с шагом deltaX\n");
    printf("  2 size n1 n2 ...    - сортировка массива (size = количество чисел)\n");
    printf("  Ctrl+D              - завершить программу\n");
    printf("Пример: 2 5 3 8 1 2   - сортирует 5 чисел: 3,8,1,2\n");
    printf("============================================\n");
    
    load_library("./lib1.so");
    
    while (1) {
        printf("Введите команду: ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\nЗавершение работы...\n");
            break;
        }
        line[strcspn(line, "\n")] = 0;
        
        if (line[0] == '0') {
            current_lib = (current_lib == 1) ? 2 : 1;
            char lib_name[20];
            snprintf(lib_name, sizeof(lib_name), "./lib%d.so", current_lib);
            load_library(lib_name);
            printf("--- РЕАЛИЗАЦИИ ПЕРЕКЛЮЧЕНЫ ---\n");
            if (current_lib == 1) {
                printf("Текущие реализации:\n");
                printf("  Функция 1: f'(x) = (f(A+Δx)-f(A))/Δx\n");
                printf("  Функция 2: Пузырьковая сортировка\n");
            } else {
                printf("Текущие реализации:\n");
                printf("  Функция 1: f'(x) = (f(A+Δx)-f(A-Δx))/(2*Δx)\n");
                printf("  Функция 2: Сортировка Хоара (быстрая сортировка)\n");
            }
        } else if (line[0] == '1') {
            if (!Derivative) {
                printf("Библиотека не загружена.\n");
                continue;
            }
            
            float A, deltaX;
            if (sscanf(line, "1 %f %f", &A, &deltaX) == 2) {
                if (deltaX == 0.0f) {
                    printf("Ошибка: шаг не может быть нулём.\n");
                } else {
                    float res = Derivative(A, deltaX);
                    printf("Производная cos(x) в точке %g с шагом %g = %g\n", A, deltaX, res);
                }
            } else {
                printf("Ошибка ввода. Используйте: 1 A deltaX\n");
            }
        } else if (line[0] == '2') {
            if (!Sort) {
                printf("Библиотека не загружена.\n");
                continue;
            }
            
            int size;
            int numbers[101];
            
            char* copy = strdup(line + 2);
            if (!copy) {
                printf("Ошибка памяти.\n");
                continue;
            }
            
            char* token = strtok(copy, " ");
            if (!token) {
                printf("Ошибка: не указан размер массива.\n");
                free(copy);
                continue;
            }
            
            size = atoi(token);
            if (size <= 0 || size > 100) {
                printf("Ошибка: размер должен быть от 1 до 100.\n");
                free(copy);
                continue;
            }
            
            numbers[0] = size;
            
            int i;
            for (i = 1; i <= size; i++) {
                token = strtok(NULL, " ");
                if (!token) {
                    printf("Ошибка: недостаточно чисел. Ожидается %d чисел после размера.\n", size);
                    free(copy);
                    break;
                }
                numbers[i] = atoi(token);
            }
            
            free(copy);
            
            if (i <= size) continue;
            
            printf("Исходный массив: ");
            for (int j = 1; j <= size; j++) {
                printf("%d ", numbers[j]);
            }
            printf("\n");
            
            int* sorted = Sort(numbers);
            if (!sorted) {
                printf("Ошибка сортировки.\n");
                continue;
            }
            
            printf("Отсортированный массив (%s): ", current_lib == 1 ? "пузырьковая" : "Хоара");
            for (int j = 0; j < size; j++) {
                printf("%d ", sorted[j]);
            }
            printf("\n");
            
            free(sorted);
        } else {
            printf("Неизвестная команда. Используйте 0, 1 или 2.\n");
        }
    }
    
    if (lib_handle) dlclose(lib_handle);
    return 0;
}
