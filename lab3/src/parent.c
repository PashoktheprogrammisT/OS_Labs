#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>

#define SIZE 1024

struct Data {
    int child_ready[3];
    int for_child;
    int processed;
    int shutdown;
    char text[SIZE];
};

volatile sig_atomic_t child_done = 0;

void sigusr2_handler(int sig) {
    child_done = 1;
}

int main() {
    char file1[100], file2[100];
    
    printf("Введите имя файла для child1: ");
    if (fgets(file1, sizeof(file1), stdin) == NULL) {
        perror("Ошибка чтения имени файла");
        return 1;
    }
    file1[strcspn(file1, "\n")] = '\0';
    
    printf("Введите имя файла для child2: ");
    if (fgets(file2, sizeof(file2), stdin) == NULL) {
        perror("Ошибка чтения имени файла");
        return 1;
    }
    file2[strcspn(file2, "\n")] = '\0';
    
    int fd = open("lab3.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Ошибка создания shared memory файла");
        return 1;
    }
    
    if (ftruncate(fd, sizeof(struct Data)) == -1) {
        perror("Ошибка установки размера файла");
        close(fd);
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
    
    for (int i = 0; i < 3; i++) data->child_ready[i] = 0;
    data->for_child = 0;
    data->processed = 1;
    data->shutdown = 0;
    
    struct sigaction sa;
    sa.sa_handler = sigusr2_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);
    
    pid_t p1 = fork();
    if (p1 == -1) {
        perror("Ошибка fork для child1");
        munmap(data, sizeof(struct Data));
        unlink("lab3.dat");
        return 1;
    }
    
    if (p1 == 0) {
        execl("./child", "child", "1", file1, "lab3.dat", NULL);
        perror("Ошибка execl для child1");
        exit(1);
    }
    
    pid_t p2 = fork();
    if (p2 == -1) {
        perror("Ошибка fork для child2");
        kill(p1, SIGTERM);
        munmap(data, sizeof(struct Data));
        unlink("lab3.dat");
        return 1;
    }
    
    if (p2 == 0) {
        execl("./child", "child", "2", file2, "lab3.dat", NULL);
        perror("Ошибка execl для child2");
        exit(1);
    }
    
    while (data->child_ready[1] == 0 || data->child_ready[2] == 0) {}
    
    printf("\nВводите строки (Ctrl+D для завершения):\n");
    
    char line[SIZE];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), stdin)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) >= SIZE) {
            line[SIZE-1] = '\0';
        }
        
        strcpy(data->text, line);
        data->for_child = (line_num % 2 == 1) ? 1 : 2;
        data->processed = 0;
        
        kill((line_num % 2 == 1) ? p1 : p2, SIGUSR1);
        
        child_done = 0;
        while (!child_done) {}
        
        printf("[child%d] Обработана строка %d\n", data->for_child, line_num);
    }
    
    printf("\nЗавершение работы...\n");
    
    data->shutdown = 1;
    kill(p1, SIGUSR1);
    kill(p2, SIGUSR1);
    
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
    
    printf("\n=== Результаты ===\n");
    
    printf("\nФайл %s:\n", file1);
    char cmd[256];
    sprintf(cmd, "cat %s 2>/dev/null || echo 'файл не найден'", file1);
    system(cmd);
    
    printf("\nФайл %s:\n", file2);
    sprintf(cmd, "cat %s 2>/dev/null || echo 'файл не найден'", file2);
    system(cmd);
    
    munmap(data, sizeof(struct Data));
    unlink("lab3.dat");
    
    printf("\nРабота завершена.\n");
    return 0;
}
