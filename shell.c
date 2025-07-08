#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include "./libreria/linenoise.h"

#define PROMPT "$ "
#define LONGITUD_HISTORIAL 1024
#define MAX_ARGS 1024
#define SEPARADOR_TOKEN " \t"
#define PATH_MAX 4096
#define BUF_SIZE 4096
#define OUTPUT_MODE 0644

char CWD[PATH_MAX];  // Directorio actual

/* ==================== Comandos propios ==================== */

void refresh_cwd(void) {
    if (getcwd(CWD, sizeof(CWD)) == NULL) {
        perror("getcwd");
        exit(1);
    }
}

void builtin_cd(char **args, size_t n_args) {
    if (n_args < 1) {
        fprintf(stderr, "Uso: cd <directorio>\n");
        return;
    }
    if (chdir(args[0]) != 0) {
        perror("cd");
        return;
    }
    refresh_cwd();
}

void builtin_pwd(char **args, size_t n_args) {
    printf("%s\n", CWD);
}

void builtin_whoami(char **args, size_t n_args) {
    char *user = getenv("USER"); 
    if (user)
        printf("%s\n", user);
    else
        fprintf(stderr, "No se pudo obtener el usuario.\n");
}

void builtin_date(char **args, size_t n_args) {
    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    char fecha[64];

    if (!tmp || strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tmp) == 0) {
        fprintf(stderr, "Error al obtener la fecha.\n");
        return;
    }

    printf("Fecha y hora actual: %s\n", fecha);
}

void builtin_uptime(char **args, size_t n_args) {
    FILE *file = fopen("/proc/uptime", "r");
    if (!file) {
        perror("uptime");
        return;
    }

    double segundos;
    if (fscanf(file, "%lf", &segundos) != 1) {
        fprintf(stderr, "Error al leer /proc/uptime\n");
        fclose(file);
        return;
    }

    fclose(file);

    int dias = segundos / (60 * 60 * 24);
    int horas = ((int)segundos % (60 * 60 * 24)) / 3600;
    int minutos = ((int)segundos % 3600) / 60;

    printf("Uptime: %d días, %d horas, %d minutos\n", dias, horas, minutos);
}

void builtin_exit(char **args, size_t n_args) {
    exit(0);
}

void builtin_ls(char **args, size_t n_args) {
    DIR *d = opendir(".");
    if (!d) {
        perror("ls");
        return;
    }
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] != '.') {
            printf("%s  ", dir->d_name);
        }
    }
    printf("\n");
    closedir(d);
}

void builtin_rm(char **args, size_t n_args) {
    if (n_args < 1) {
        fprintf(stderr, "Uso: rm <archivo>\n");
        return;
    }
    if (unlink(args[0]) != 0) {
        perror("rm");
    }
}

void builtin_mkdir(char **args, size_t n_args) {
    if (n_args < 1) {
        fprintf(stderr, "Uso: mkdir <directorio>\n");
        return;
    }
    if (mkdir(args[0], 0755) != 0) {
        perror("mkdir");
    }
}

void builtin_rmdir(char **args, size_t n_args) {
    if (n_args < 1) {
        fprintf(stderr, "Uso: rmdir <directorio>\n");
        return;
    }
    if (rmdir(args[0]) != 0) {
        perror("rmdir");
    }
}

/* ==== Comandos adicionales (archivo) ==== */

void cmd_copy(char *src, char *dest) {
    int in_fd = open(src, O_RDONLY);
    if (in_fd < 0) {
        perror("Error al abrir archivo fuente");
        return;
    }

    int out_fd = creat(dest, OUTPUT_MODE);
    if (out_fd < 0) {
        perror("Error al crear archivo destino");
        close(in_fd);
        return;
    }

    char buffer[BUF_SIZE];
    int bytes;
    while ((bytes = read(in_fd, buffer, BUF_SIZE)) > 0) {
        if (write(out_fd, buffer, bytes) != bytes) {
            perror("Error al escribir");
            break;
        }
    }

    close(in_fd);
    close(out_fd);
}

void cmd_move(char *src, char *dest) {
    if (rename(src, dest) != 0) {
        perror("Error al mover archivo");
    }
}

void cmd_create(char *filename) {
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("Error al crear archivo");
    } else {
        close(fd);
    }
}

void cmd_read(char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error al abrir archivo");
        return;
    }

    char buffer[1024];
    int bytes;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes);
    }

    close(fd);
}

void cmd_write(char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error al abrir archivo");
        return;
    }

    char texto[256];
    printf("Escribe el texto (ENTER para guardar):\n");
    fgets(texto, sizeof(texto), stdin);
    texto[strcspn(texto, "\n")] = 0;

    write(fd, texto, strlen(texto));
    write(fd, "\n", 1);
    close(fd);
}

/* ==== Ejecutar comando ==== */

int ejecutar_comando_personalizado(char *cmd, char **args, size_t n_args) {
    if (strcmp(cmd, "cd") == 0) {
        builtin_cd(args, n_args);
    } else if (strcmp(cmd, "pwd") == 0) {
        builtin_pwd(args, n_args);
    } else if (strcmp(cmd, "whoami") == 0) {
        builtin_whoami(args, n_args);
    } else if (strcmp(cmd, "ls") == 0) {
        builtin_ls(args, n_args);
    } else if (strcmp(cmd, "rm") == 0) {
        builtin_rm(args, n_args);
    } else if (strcmp(cmd, "mkdir") == 0) {
        builtin_mkdir(args, n_args);
    } else if (strcmp(cmd, "rmdir") == 0) {
        builtin_rmdir(args, n_args);
    } else if (strcmp(cmd, "date") == 0) {
        builtin_date(args, n_args);
    } else if (strcmp(cmd, "uptime") == 0) {
        builtin_uptime(args, n_args);
    } else if (strcmp(cmd, "exit") == 0) {
        builtin_exit(args, n_args);
    } else if (strcmp(cmd, "copy") == 0 && n_args >= 2) {
        cmd_copy(args[0], args[1]);
    } else if (strcmp(cmd, "move") == 0 && n_args >= 2) {
        cmd_move(args[0], args[1]);
    } else if (strcmp(cmd, "create") == 0 && n_args >= 1) {
        cmd_create(args[0]);
    } else if (strcmp(cmd, "read") == 0 && n_args >= 1) {
        cmd_read(args[0]);
    } else if (strcmp(cmd, "write") == 0 && n_args >= 1) {
        cmd_write(args[0]);
    } else {
        return 0; // No reconocido
    }
    return 1; // Ejecutado
}

/* ==== Tokenización ==== */

int s_leer(char *input, char **args) {
    int i = 0;
    char *token = strtok(input, SEPARADOR_TOKEN);
    while (token != NULL && i < (MAX_ARGS - 1)) {
        args[i++] = token;
        token = strtok(NULL, SEPARADOR_TOKEN);
    }
    args[i] = NULL;
    return i;
}

/* ==================== main ==================== */

int main(void) {
    refresh_cwd();

    if (!linenoiseHistorySetMaxLen(LONGITUD_HISTORIAL)) {
        fprintf(stderr, "No se pudo configurar historial\n");
        exit(1);
    }

    char *linea;
    char *args[MAX_ARGS];

    while ((linea = linenoise(PROMPT)) != NULL) {
        int args_leidos = s_leer(linea, args);

        if (args_leidos == 0) {
            linenoiseFree(linea);
            continue;
        }

        char *cmd = args[0];
        char **cmd_args = args + 1;
        size_t n_args = args_leidos - 1;

        if (!ejecutar_comando_personalizado(cmd, cmd_args, n_args)) {
            fprintf(stderr, "Error: comando no soportado\n");
        }

        linenoiseHistoryAdd(linea);
        linenoiseFree(linea);
    }

    return 0;
}