#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int is_vowel(char c) {
    c = tolower(c);
    return (c == 'a' || c == 'e' || c == 'i' || 
            c == 'o' || c == 'u' || c == 'y');
}

void remove_vowels(char *str) {
    char *src = str;
    char *dst = str;
    
    while (*src) {
        if (!is_vowel(*src)) {
            *dst = *src;
            dst++;
        }
        src++;
    }
    *dst = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s output_filename\n", argv[0]);
        return 1;
    }
    
    const char *outname = argv[1];
    FILE *out = fopen(outname, "w");
    if (!out) {
        perror("Ошибка открытия файла");
        return 1;
    }
    
    char *line = NULL;
    size_t cap = 0;
    
    while (getline(&line, &cap, stdin) != -1) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        char original[1024];
        if (len >= sizeof(original)) len = sizeof(original) - 1;
        strncpy(original, line, len);
        original[len] = '\0';
        
        remove_vowels(line);
        
        printf("%s\n", line);
        fflush(stdout);
        
        fprintf(out, "%s\n", line);
        fflush(out);
    }
    
    free(line);
    fclose(out);
    return 0;
}
