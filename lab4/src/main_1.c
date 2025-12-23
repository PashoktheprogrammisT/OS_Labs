#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "implementation_1.h"

int main() {
    char line[256];
    
    printf("=== Программа 1: Статическая линковка ===\n");
    printf("Функция 1: Производная cos(x) (f'(x) = (f(A+Δx)-f(A))/Δx)\n");
    printf("Функция 2: Пузырьковая сортировка\n");
    printf("Формат ввода:\n");
    printf("  1 A deltaX          - производная cos(x) в точке A с шагом deltaX\n");
    printf("  2 size n1 n2 ...    - сортировка массива (size = количество чисел)\n");
    printf("  Ctrl+D              - завершить программу\n");
    printf("Пример: 2 5 3 8 1 2   - сортирует 5 чисел: 3,8,1,2\n");
    printf("==========================================\n");
    
    while (1) {
        printf("Введите команду: ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\nЗавершение работы...\n");
            break;
        }
        line[strcspn(line, "\n")] = 0;
        
        if (line[0] == '0') {
            printf("Переключение реализаций недоступно в программе 1.\n");
        } else if (line[0] == '1') {
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
            
            printf("Отсортированный массив (пузырьковая): ");
            for (int j = 0; j < size; j++) {
                printf("%d ", sorted[j]);
            }
            printf("\n");
            
            free(sorted);
        } else {
            printf("Неизвестная команда. Используйте 1 или 2.\n");
        }
    }
    
    return 0;
}
