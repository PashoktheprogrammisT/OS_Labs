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
    int for_child;
    int processed;
    int shutdown;
    char text[SIZE];
};

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
    
    data->for_child = 0;
    data->processed = 1;
    data->shutdown = 0;
    
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
    
    sleep(1);
    
    printf("\nВводите строки (Ctrl+D для завершения):\n");
    
    char line[SIZE];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), stdin)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) >= SIZE) {
            printf("[WARNING] Строка слишком длинная, обрезана\n");
            line[SIZE-1] = '\0';
        }
        
        strcpy(data->text, line);
        data->for_child = (line_num % 2 == 1) ? 1 : 2;
        data->processed = 0;
        
        int timeout = 0;
        while (data->processed == 0 && timeout < 100) {
            usleep(10000);
            timeout++;
        }
        
        if (timeout >= 100) {
            printf("[ERROR] Child не ответил, пропускаем строку\n");
            data->processed = 1;
        } else {
            printf("[child%d] Обработана строка %d\n", 
                   (line_num % 2 == 1) ? 1 : 2, line_num);
        }
    }
    
    printf("\nЗавершение работы...\n");
    
    data->shutdown = 1;
    data->for_child = 1;
    data->processed = 0;
    sleep(1);
    
    data->for_child = 2;
    data->processed = 0;
    sleep(1);
    
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
    
    printf("\nРабота завершена успешно.\n");
    return 0;
}
