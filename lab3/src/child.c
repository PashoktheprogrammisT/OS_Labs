#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>

#define SIZE 1024

struct Data {
    int for_child;
    int processed;
    int shutdown;
    char text[SIZE];
};

int is_vowel(char c) {
    c = tolower(c);
    return c == 'a' || c == 'e' || c == 'i' || 
           c == 'o' || c == 'u' || c == 'y';
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <номер> <выходной_файл> <shm_файл>\n", argv[0]);
        return 1;
    }
    
    int my_num = atoi(argv[1]);
    char *out_file = argv[2];
    char *shm_file = argv[3];
    
    int fd = open(shm_file, O_RDWR);
    if (fd == -1) {
        perror("Ошибка открытия shared memory файла");
        return 1;
    }
    
    struct Data *data = mmap(NULL, sizeof(struct Data),
                            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("Ошибка mmap");
        close(fd);
        return 1;
    }
    close(fd);
    
    FILE *out = fopen(out_file, "w");
    if (!out) {
        perror("Ошибка открытия выходного файла");
        munmap(data, sizeof(struct Data));
        return 1;
    }
    
    int processed = 0;
    
    while (1) {
        if (data->shutdown) break;
        
        if (data->for_child == my_num && data->processed == 0) {
            char result[SIZE];
            char *src = data->text;
            char *dst = result;
            int i = 0;
            
            while (*src && i < SIZE-1) {
                if (!is_vowel(*src)) {
                    *dst++ = *src;
                    i++;
                }
                src++;
            }
            *dst = '\0';
            
            if (fprintf(out, "%s\n", result) < 0) {
                perror("Ошибка записи в файл");
            }
            fflush(out);
            processed++;
            
            data->processed = 1;
        }
        
        usleep(5000);
    }
    
    printf("Child%d: обработано %d строк, записано в %s\n", 
           my_num, processed, out_file);
    
    fclose(out);
    munmap(data, sizeof(struct Data));
    return 0;
}
