#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./libreria/linenoise.h"
#include <unistd.h>
#include <sys/wait.h>

#define PROMPT "$"
#define LONGITUD_HISTORIAL 1024
#define MAX_ARGS 1024
#define SEPARADOR_TOKEN " \t"
#define PATH_MAX 4096

char CWD[PATH_MAX];

/*
divide una línea de texto en palabras o "tokens" separados
por espacios o tabulaciones para identificar el comando a ejecutar
y luego los almacena
*/
int s_leer(char *input, char **args){
    int i = 0;
    char *token = strtok(input, SEPARADOR_TOKEN);

    while (token != NULL && i < (MAX_ARGS - 1)){
        args[i++] = token;
        token = strtok(NULL, SEPARADOR_TOKEN);
    }

    args[i] = NULL; 
    return i;
}
//Ejecutar
int s_ejecutar(char *cmd, char **cmd_args){
    fprintf(stderr, "Ejecutando.....♪⁠〜⁠(⁠꒪⁠꒳⁠꒪⁠)⁠〜♪⁠ ´%s´\n", cmd);
    int estado;
    pid_t pid;
    //fork() duplica la imagen de un proceso en la imagen del pc
    //Se duplica un proceso shell (padre>0 e hijo=0)
    pid = fork();

    //Proceso hijo
    if(pid < 0){
        fprintf(stderr, "No fue posible ejecutarlo (⁠´⁠;⁠︵⁠;⁠`)\n");
        return -1;
    }

    if (pid == 0){
        //Ejecuta los comandos del sistema y busca en el path
        //en caso de que sea una abreviación, reemplaza la imagen
        //del proceso con un nuevo programa
        execvp(cmd, cmd_args);
    }else{
        //Espera a que el hijo termine el proceso
        if(waitpid (pid, &estado, 0) !=pid){
        //El padre espera por el hijo
        fprintf(stderr, "No fue posible esperar por el hijo ｡⁠:ﾟ⁠(⁠;⁠´⁠∩⁠`⁠;⁠)ﾟ⁠:⁠｡ -----(ಠ⁠_⁠ಠ)");
        return -1;
        }
    }
    return estado;
}
//Builtin
typedef enum Builtin {
    CD,
    PWD,
    INVALID
} Builtin;

void builtin_impl_cd(char **args, size_t n_args);
void builtin_impl_pwd(char **args, size_t n_args);

void (*BUILTIN_TABLE[]) (char **args, size_t n_args) = {
  [CD] = builtin_impl_cd,
  [PWD] = builtin_impl_pwd,
};

Builtin builtin_code(char *cmd) {
  if (!strncmp(cmd, "cd", 2)) {
    return CD;
  } else if (!strncmp(cmd, "pwd", 3)) {
    return PWD;
  } else {
    return INVALID;
  }
}

int is_builtin(char *cmd) {
  return builtin_code(cmd) != INVALID;
}

//ejecutar builtin
void s_ejecutar_builtin(char *cmd, char **args, size_t n_args) {
  BUILTIN_TABLE[builtin_code(cmd)](args, n_args);
}

void refresh_cwd(void) {
  if (getcwd(CWD, sizeof(CWD)) == NULL) {
    fprintf(stderr, "Error: No se pudo leer el directorio de trabajo");
    exit(1);
  }
}

void builtin_impl_cd(char **args, size_t n_args) {
  char *new_dir = *args;
  if (chdir(new_dir) != 0) {
    fprintf(stderr, "Error: No se pudo cambiar el directorio \n");
    //exit(1);
  }
  refresh_cwd();
}

void builtin_impl_pwd(char **args, size_t n_args) {
  fprintf(stdout, "%s\n", CWD);
}

int main(void){
    refresh_cwd();

    // Configura la longitud máxima del historial de comandos
    if(!linenoiseHistorySetMaxLen(LONGITUD_HISTORIAL)){
        fprintf(stderr, "No se pudo implementar un historial de linenoise (⁠´⁠;⁠︵⁠;⁠`) )");
        exit(1);
    }

    char *linea;
    //Definir el limite del arreglo
    char *args[MAX_ARGS];

    // Bucle principal del shell en donde se lee la entrada del usuario
    while((linea = linenoise(PROMPT))!= NULL){
        //Lectura
        int args_leidos = s_leer(linea, args);

        fprintf(stdout, "Leyendo %d args\n", args_leidos);
        for (int i = 0; i < args_leidos; i++) {
          fprintf(stdout, "arg[%d] = %s\n", i, args[i]);
        }

        //Se salta las lineas vacias
        if (args_leidos == 0) {
          linenoiseFree(linea);
        //ir al inicio del ciclo
          continue;
        }
        //Evaluación+impresión

        // El primer token representa el comando (nombre del comando)
        char *cmd = args [0];
        char **cmd_args = args;
    if (is_builtin(cmd)) {
        s_ejecutar_builtin(cmd, (cmd_args+1), args_leidos-1);
    } else {
        s_ejecutar(cmd, cmd_args);
    }
        linenoiseHistoryAdd(linea);
        linenoiseFree(linea);
    }
    return 0;
}