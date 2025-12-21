#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

#define SIZE 1024

struct Data {
    int child_ready[3];
    int for_child;
    int processed;
    int shutdown;
    char text[SIZE];
};

volatile sig_atomic_t got_signal = 0;

void sigusr1_handler(int sig) {
    got_signal = 1;
}

int is_vowel(char c) {
    c = tolower(c);
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
}

int main(int argc, char *argv[]) {
    if (argc != 4) return 1;
    
    signal(SIGUSR1, sigusr1_handler);
    
    int my_num = atoi(argv[1]);
    char *out_file = argv[2];
    char *shm_file = argv[3];
    
    int fd = open(shm_file, O_RDWR);
    if (fd == -1) return 1;
    
    struct Data *data = mmap(NULL, sizeof(struct Data),
                            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return 1;
    }
    close(fd);
    
    data->child_ready[my_num] = 1;
    
    FILE *out = fopen(out_file, "w");
    if (!out) {
        munmap(data, sizeof(struct Data));
        return 1;
    }
    
    pid_t parent_pid = getppid();
    int processed = 0;
    
    while (1) {
        while (!got_signal && !data->shutdown) {}
        
        if (data->shutdown) break;
        
        got_signal = 0;
        
        if (data->for_child == my_num && data->processed == 0) {
            char result[SIZE];
            char *src = data->text;
            char *dst = result;
            
            while (*src) {
                if (!is_vowel(*src)) *dst++ = *src;
                src++;
            }
            *dst = '\0';
            
            fprintf(out, "%s\n", result);
            processed++;
            data->processed = 1;
            
            kill(parent_pid, SIGUSR2);
        }
    }
    
    printf("Child%d: %d строк\n", my_num, processed);
    
    fclose(out);
    munmap(data, sizeof(struct Data));
    return 0;
}
