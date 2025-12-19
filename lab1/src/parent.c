#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

static ssize_t write_all(int fd, const void *buf, size_t count) {
    const char *p = buf;
    size_t left = count;
    while (left > 0) {
        ssize_t w = write(fd, p, left);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        left -= (size_t)w;
        p += w;
    }
    return (ssize_t)count;
}

int main() {
    char *filename1 = NULL, *filename2 = NULL;
    size_t fncap = 0;
    ssize_t r;
    
    printf("Введите имя файла для child1: ");
    fflush(stdout);
    r = getline(&filename1, &fncap, stdin);
    if (r <= 0) { 
        perror("Ошибка чтения имени файла 1"); 
        free(filename1); 
        return 1; 
    }
    if (filename1[r-1] == '\n') filename1[r-1] = '\0';
    
    printf("Введите имя файла для child2: ");
    fflush(stdout);
    fncap = 0;
    r = getline(&filename2, &fncap, stdin);
    if (r <= 0) { 
        perror("Ошибка чтения имени файла 2"); 
        free(filename1); 
        free(filename2); 
        return 1; 
    }
    if (filename2[r-1] == '\n') filename2[r-1] = '\0';
    
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1) { 
        perror("Ошибка создания pipe1"); 
        free(filename1); 
        free(filename2); 
        return 1; 
    }
    if (pipe(pipe2) == -1) { 
        perror("Ошибка создания pipe2"); 
        close(pipe1[0]); 
        close(pipe1[1]); 
        free(filename1); 
        free(filename2); 
        return 1; 
    }
    
    printf("\n=== Начало обработки строк ===\n");
    printf("Вводите строки (Ctrl+D для завершения ввода):\n");
    printf("--------------------------------------------\n");
    
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("Ошибка fork для child1");
        close(pipe1[0]); close(pipe1[1]); close(pipe2[0]); close(pipe2[1]);
        free(filename1); free(filename2);
        return 1;
    }
    
    if (pid1 == 0) {
        if (dup2(pipe1[0], STDIN_FILENO) == -1) { 
            perror("child1: ошибка dup2"); 
            _exit(1); 
        }
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        
        execl("./child", "child", filename1, (char *)NULL);
        perror("child1: ошибка execl");
        _exit(1);
    }
    
    close(pipe1[0]);
    
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("Ошибка fork для child2");
        close(pipe1[1]); close(pipe2[0]); close(pipe2[1]);
        free(filename1); free(filename2);
        return 1;
    }
    
    if (pid2 == 0) {
        if (dup2(pipe2[0], STDIN_FILENO) == -1) { 
            perror("child2: ошибка dup2"); 
            _exit(1); 
        }
        close(pipe2[0]); close(pipe2[1]);
        close(pipe1[0]); close(pipe1[1]);
        
        execl("./child", "child", filename2, (char *)NULL);
        perror("child2: ошибка execl");
        _exit(1);
    }
    
    close(pipe2[0]);
    
    char *line = NULL;
    size_t linecap = 0;
    long lineno = 0;
    
    while (getline(&line, &linecap, stdin) != -1) {
        ++lineno;
        
        int target_fd = (lineno % 2 == 1) ? pipe1[1] : pipe2[1];
        const char *child_name = (lineno % 2 == 1) ? "child1" : "child2";
        
        if (write_all(target_fd, line, strlen(line)) == -1) {
            fprintf(stderr, "Ошибка записи в %s: %s\n", child_name, strerror(errno));
        }
        
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        printf("[%s] Строка %ld: %s\n", child_name, lineno, line);
    }
    
    free(line);
    
    printf("\n[INFO] Конец ввода (EOF)\n");
    printf("\n--------------------------------------------\n");
    printf("Ожидание завершения дочерних процессов...\n");
    
    close(pipe1[1]);
    close(pipe2[1]);
    
    int status1, status2;
    if (waitpid(pid1, &status1, 0) == -1) perror("Ошибка waitpid для child1");
    if (waitpid(pid2, &status2, 0) == -1) perror("Ошибка waitpid для child2");
    
    printf("\n=== РЕЗУЛЬТАТЫ РАБОТЫ ===\n");
    printf("child1 завершился с кодом: %d\n", WEXITSTATUS(status1));
    printf("child2 завершился с кодом: %d\n", WEXITSTATUS(status2));
    printf("\nРезультаты записаны в файлы:\n");
    printf("• %s (child1, нечетные строки)\n", filename1);
    printf("• %s (child2, четные строки)\n", filename2);
    
    printf("\n--- Содержимое %s ---\n", filename1);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cat %s 2>/dev/null", filename1);
    system(cmd);
    
    printf("\n--- Содержимое %s ---\n", filename2);
    snprintf(cmd, sizeof(cmd), "cat %s 2>/dev/null", filename2);
    system(cmd);
    
    free(filename1);
    free(filename2);
    printf("\nРодительский процесс завершен.\n");
    return 0;
}
