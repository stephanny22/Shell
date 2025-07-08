#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include "./libreria/linenoise.h"

#define PROMPT "$ "
#define LONGITUD_HISTORIAL 1024
#define MAX_ARGS 1024
#define SEPARADOR_TOKEN " \t"
#define PATH_MAX 4096

// Variable global que guarda la ruta del directorio actual
char CWD[PATH_MAX];

/* ==================== Comandos propios implementados ==================== */

// Refresca el directorio actual y lo guarda en la variable global CWD
void refresh_cwd(void) {
    if (getcwd(CWD, sizeof(CWD)) == NULL) {
        perror("getcwd");
        exit(1);
    }
}

// Cambiar de directorio (cd)
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

// Imprime el directorio actual (pwd)
void builtin_pwd(char **args, size_t n_args) {
    printf("%s\n", CWD);
}

// Muestra el nombre de usuario actual 
    void builtin_whoami(char **args, size_t n_args) {
    char *user = getenv("USER"); 
    if (user)
        printf("%s\n", user);
    else
        fprintf(stderr, "No se pudo obtener el usuario.\n");
}
// Muestra la fecha y hora actual (date)
void builtin_date(char **args, size_t n_args) {
    time_t t;
    struct tm *tmp;
    char fecha[64];

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        perror("localtime");
        return;
    }

    if (strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tmp) == 0) {
        fprintf(stderr, "Error al formatear la fecha.\n");
        return;
    }

    printf("Fecha y hora actual: %s\n", fecha);
}

// Muestra el tiempo que lleva encendido el sistema (uptime)
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



// Sale del shell (exit)
void builtin_exit(char **args, size_t n_args) {
    exit(0);
}

// Lista archivos y directorios (ls)
void builtin_ls(char **args, size_t n_args) {
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (!d) {
        perror("ls");
        return;
    }
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] != '.') { // omite archivos ocultos
            printf("%s  ", dir->d_name);
        }
    }
    printf("\n");
    closedir(d);
}

// Borra archivos (rm)
void builtin_rm(char **args, size_t n_args) {
    if (n_args < 1) {
        fprintf(stderr, "Uso: rm <archivo>\n");
        return;
    }
    if (unlink(args[0]) != 0) {
        perror("rm");
    }
}

// Crea directorios (mkdir)
void builtin_mkdir(char **args, size_t n_args) {
    if (n_args < 1) {
        fprintf(stderr, "Uso: mkdir <directorio>\n");
        return;
    }
    if (mkdir(args[0], 0755) != 0) {
        perror("mkdir");
    }
}

// Borra directorios vacíos (rmdir)
void builtin_rmdir(char **args, size_t n_args) {
    if (n_args < 1) {
        fprintf(stderr, "Uso: rmdir <directorio>\n");
        return;
    }
    if (rmdir(args[0]) != 0) {
        perror("rmdir");
    }
}

// Llama a las funciones anteriores dependiendo del nombre del comando ingresado
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
    } else {
        return 0; // No es un comando propio
    }
    return 1; // Sí fue un comando propio
}

/* ==================== Funciones del shell ==================== */

/*
divide una línea de texto en palabras o "tokens" separados
por espacios o tabulaciones para identificar el comando a ejecutar
y luego los almacena
*/
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

/* ==================== Programa principal ==================== */

int main(void) {
    refresh_cwd(); // Inicializa el directorio actual

    // Configura la longitud máxima del historial de comandos
    if (!linenoiseHistorySetMaxLen(LONGITUD_HISTORIAL)) {
        fprintf(stderr, "No se pudo configurar historial\n");
        exit(1);
    }

    char *linea;
    // Arreglo que almacena los tokens separados (comando y argumentos)
    char *args[MAX_ARGS];

    // Bucle principal del shell en donde se lee la entrada del usuario
    while ((linea = linenoise(PROMPT)) != NULL) {
        // Lectura y tokenización
        int args_leidos = s_leer(linea, args);

        // Salta líneas vacías
        if (args_leidos == 0) {
            linenoiseFree(linea);
            continue;
        }

        // El primer token representa el nombre del comando
        char *cmd = args[0];
        char **cmd_args = args + 1;
        size_t n_args = args_leidos - 1;

        // Ejecuta solo si el comando fue implementado
        if (!ejecutar_comando_personalizado(cmd, cmd_args, n_args)) {
            fprintf(stderr, "Error: comando no soportado\n");
        }

        linenoiseHistoryAdd(linea);  // Guarda en historial
        linenoiseFree(linea);        // Libera la línea de entrada
    }

    return 0;


    

}

