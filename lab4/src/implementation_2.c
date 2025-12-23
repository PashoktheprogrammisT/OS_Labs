#include <math.h>
#include <stdlib.h>

float Derivative(float A, float deltaX) {
    return (cosf(A + deltaX) - cosf(A - deltaX)) / (2 * deltaX);
}

static int partition(int* arr, int low, int high) {
    int pivot = arr[low];
    int i = low - 1;
    int j = high + 1;
    
    while (1) {
        do {
            i++;
        } while (arr[i] < pivot);
        
        do {
            j--;
        } while (arr[j] > pivot);
        
        if (i >= j) {
            return j;
        }
        
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

static void quick_sort_hoare(int* arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort_hoare(arr, low, pi);
        quick_sort_hoare(arr, pi + 1, high);
    }
}

int* Sort(int* array) {
    int size = array[0];
    int* data = array + 1;
    
    int* copy = (int*)malloc(size * sizeof(int));
    if (!copy) return NULL;
    
    for (int i = 0; i < size; i++) {
        copy[i] = data[i];
    }
    
    if (size <= 1) return copy;
    
    quick_sort_hoare(copy, 0, size - 1);
    return copy;
}
