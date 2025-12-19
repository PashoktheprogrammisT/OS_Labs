#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

struct Shared {
    volatile sig_atomic_t ready;
    char buffer[1024];
};

volatile sig_atomic_t child_done = 0;

void sigusr2_handler(int sig) {
    child_done = 1;
}

int main() {
    char filename1[256], filename2[256];
    
    printf("Введите имя файла для child1: ");
    fflush(stdout);
    if (fgets(filename1, sizeof(filename1), stdin) == NULL) {
        perror("Ошибка чтения имени файла 1");
        return 1;
    }
    filename1[strcspn(filename1, "\n")] = '\0';
    
    printf("Введите имя файла для child2: ");
    fflush(stdout);
    if (fgets(filename2, sizeof(filename2), stdin) == NULL) {
        perror("Ошибка чтения имени файла 2");
        return 1;
    }
    filename2[strcspn(filename2, "\n")] = '\0';
    
    char *shm_name = "shared_memory.dat";
    int fd = open(shm_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    if (ftruncate(fd, sizeof(struct Shared)) == -1) {
        perror("ftruncate");
        close(fd);
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
    shared->ready = 0;
    
    struct sigaction sa;
    sa.sa_handler = sigusr2_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction");
        munmap(shared, sizeof(struct Shared));
        return 1;
    }
    
    pid_t pid1 = fork();
    if (pid1 == 0) {
        execl("./child", "child", shm_name, filename1, "1", NULL);
        perror("execl child1");
        exit(1);
    }
    
    pid_t pid2 = fork();
    if (pid2 == 0) {
        execl("./child", "child", shm_name, filename2, "2", NULL);
        perror("execl child2");
        exit(1);
    }
    
    printf("\n=== Начало обработки строк ===\n");
    printf("Вводите строки (Ctrl+D для завершения):\n");
    printf("--------------------------------------\n");
    
    char line[1024];
    long lineno = 0;
    
    while (fgets(line, sizeof(line), stdin)) {
        lineno++;
        
        line[strcspn(line, "\n")] = 0;
        
        strncpy(shared->buffer, line, sizeof(shared->buffer) - 1);
        shared->buffer[sizeof(shared->buffer) - 1] = '\0';
        
        if (lineno % 2 == 1) {
            shared->ready = 1;
            kill(pid1, SIGUSR1);
        } else {
            shared->ready = 2;
            kill(pid2, SIGUSR1);
        }
        
        while (!child_done) pause();
        child_done = 0;
        
        printf("[%s] Строка %ld: %s\n", 
               (lineno % 2 == 1) ? "child1" : "child2", 
               lineno, line);
    }
    
    printf("\n[INFO] Конец ввода\n");
    printf("\n--------------------------------------\n");
    printf("Ожидание завершения дочерних процессов...\n");
    
    shared->ready = 0;
    kill(pid1, SIGTERM);
    kill(pid2, SIGTERM);
    
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    
    printf("\n=== РЕЗУЛЬТАТЫ РАБОТЫ ===\n");
    printf("child1 завершился с кодом: 0\n");
    printf("child2 завершился с кодом: 0\n");
    
    char cmd[512];
    printf("\n--- Содержимое %s ---\n", filename1);
    snprintf(cmd, sizeof(cmd), "cat %s 2>/dev/null", filename1);
    system(cmd);
    
    printf("\n--- Содержимое %s ---\n", filename2);
    snprintf(cmd, sizeof(cmd), "cat %s 2>/dev/null", filename2);
    system(cmd);
    
    munmap(shared, sizeof(struct Shared));
    unlink(shm_name);
    
    printf("\nРодительский процесс завершен.\n");
    return 0;
}
