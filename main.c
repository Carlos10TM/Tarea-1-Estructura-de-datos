// Este archivo es el punto de entrada para el simulador uGit.
// Procesa los argumentos de la linea de comandos y llama a las funciones principales definidas en ugit_core.c

// Este archivo es el punto de entrada para el simulador uGit.
// Se encarga de procesar los argumentos de la linea de comandos y dirigir la ejecucion
// a la funcion correspondiente en ugit_core.c

#include "ugit.h"

int main(int argc, char* argv[]) { // Funcion principal, recibe los argumentos de la linea de comandos
    // Carga el area de preparacion al iniciar el programa. Esto permite que
    // el estado del "staging area" persista entre ejecuciones.
    load_staging_area(); // Carga el estado guardado del area de preparacion
    load_current_head(); // Carga el commit actual al que apunta HEAD

    // Verifica si se proporciono un comando. Si no, muestra el mensaje de uso.
    if (argc < 2) { // Si no se pasa ningun comando, muestra mensaje de uso
        printf("uso: ugit <comando> [<args>]\n");
        return 1;
    }

    const char* command = argv[1]; // Se obtiene el comando ingresado por el usuario

    // Utiliza una serie de "if/else if" para determinar que comando se ejecuto
    // y llama a la funcion de "ugit_core.c" apropiada.

    if (strcmp(command, "init") == 0) { // Si el comando es init, se inicializa un repositorio nuevo
        // Inicializa un nuevo repositorio.
        ugit_init();
    } else if (strcmp(command, "add") == 0) { // Si el comando es add, se agrega un archivo al area de preparacion
        // Añade un archivo al area de preparacion.
        // Verifica que el nombre del archivo sea proporcionado.
        if (argc < 3) {
            printf("ugit add requiere un nombre de archivo.\n");
            return 1;
        }
        ugit_add(argv[2]);
    } else if (strcmp(command, "commit") == 0) { // Si el comando es commit, se crea un commit nuevo
        // Crea un nuevo commit.
        // Verifica que se haya proporcionado la bandera "-m" y el mensaje.
        if (argc < 4 || strcmp(argv[2], "-m") != 0) {
            printf("ugit commit requiere un mensaje con -m.\n");
            return 1;
        }
        ugit_commit(argv[3]);
    } else if (strcmp(command, "log") == 0) { // Si el comando es log, se muestra el historial de commits
        // Muestra el historial de commits.
        ugit_log();
    } else if (strcmp(command, "checkout") == 0) { // Si el comando es checkout, se cambia a un commit anterior
        // Cambia el estado del proyecto a un commit anterior.
        // Verifica que el ID del commit sea proporcionado.
        if (argc < 3) {
            printf("ugit checkout requiere un commit ID.\n");
            return 1;
        }
        ugit_checkout(argv[2]);
    } else { // Caso de comando desconocido
        // Maneja comandos no reconocidos.
        printf("Comando desconocido: %s\n", command);
        return 1;
    }

    return 0;
}
