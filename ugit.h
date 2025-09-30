// ugit.h
// Archivo de cabecera de uGit. Contiene las definiciones de estructuras, constantes y prototipos de funciones.

// ugit.h

#ifndef UGIT_H // Comienza la guarda de inclusion
#define UGIT_H // Define UGIT_H para evitar multiples inclusiones

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#include <sys/stat.h>
#include <unistd.h>

#define HASH_SIZE 1024 // Tamano de la tabla hash
#define MAX_FILENAME_LEN 256 // Longitud maxima para nombre de archivo
#define MAX_COMMIT_MSG_LEN 512 // Longitud maxima para el mensaje de commit
#define MAX_FILES 100 // Numero maximo de archivos soportados

// Estructura para representar un archivo en el area de preparacion (staging area)
typedef struct { // Definicion de estructura
    char filename[MAX_FILENAME_LEN];
    // Hash del contenido del archivo
    char file_hash[65];
} FileEntry;

// Estructura para representar un commit
typedef struct { // Definicion de estructura
    char commit_id[65];
    char parent_id[65];
    char message[MAX_COMMIT_MSG_LEN];
    char author_name[128];
    char author_email[128];
    time_t timestamp;
    FileEntry files[MAX_FILES]; // Estructura que representa un archivo en staging area
    int file_count; // Prototipo de funcion
} Commit;

// Tabla hash (array de punteros) para almacenar los commits
typedef struct { // Definicion de estructura
    Commit* commits[HASH_SIZE]; // Estructura que representa un commit
} HashTable;

// Prototipos de las funciones principales de uGit
void ugit_init();
void ugit_add(const char* filename);
void ugit_commit(const char* message);
void ugit_log();
void ugit_checkout(const char* commit_id);

// Prototipos de funciones auxiliares
char* calculate_hash(const char* data);
int hash_function(const char* key);
void save_commit(const Commit* commit); // Estructura que representa un commit
Commit* load_commit(const char* commit_id); // ""    ""      ""     ""   ""
void save_staging_area();
void load_staging_area();
void load_current_head();

#endif
