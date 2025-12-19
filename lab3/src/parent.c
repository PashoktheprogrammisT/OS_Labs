#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>

struct Shared {
    volatile sig_atomic_t ready;
    char buffer[1024];
};

volatile sig_atomic_t got_signal = 0;
volatile sig_atomic_t terminate = 0;

void sigusr1_handler(int sig) {
    got_signal = 1;
}

void sigterm_handler(int sig) {
    terminate = 1;
}

int is_vowel(char c) {
    c = tolower(c);
    return (c == 'a' || c == 'e' || c == 'i' || 
            c == 'o' || c == 'u' || c == 'y');
}

void remove_vowels(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (!is_vowel(*src)) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s shm_file output_file child_num\n", argv[0]);
        return 1;
    }
    
    char *shm_name = argv[1];
    char *output_name = argv[2];
    int child_num = atoi(argv[3]);
    
    int fd = open(shm_name, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    struct Shared *shared = mmap(NULL, sizeof(struct Shared),
                                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    
    close(fd);
    
    struct sigaction sa_usr1, sa_term;
    
    sa_usr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);
    
    sa_term.sa_handler = sigterm_handler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);
    
    FILE *out = fopen(output_name, "w");
    if (!out) {
        perror("fopen");
        munmap(shared, sizeof(struct Shared));
        return 1;
    }
    
    pid_t parent_pid = getppid();
    
    while (!terminate) {
        pause();
        
        if (terminate) break;
        if (!got_signal) continue;
        got_signal = 0;
        
        if (shared->ready != child_num) continue;
        
        if (shared->ready == 0) {
            break;
        }
        
        char line[1024];
        strncpy(line, shared->buffer, sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        
        remove_vowels(line);
        
        fprintf(out, "%s\n", line);
        fflush(out);
        
        kill(parent_pid, SIGUSR2);
    }
    
    fclose(out);
    munmap(shared, sizeof(struct Shared));
    return 0;
}
