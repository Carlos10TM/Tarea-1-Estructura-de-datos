// Implementacion de las funciones principales y auxiliares del simulador uGit.
// Incluye logica para commits, area de preparacion, historial y restauracion de archivos.

#include "ugit.h"

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

// Variables globales para el estado del repositorio
// Son esenciales para mantener el estado del area de preparacion y la rama actual.
FileEntry staging_area[MAX_FILES]; //Estructura para representar un archivo en el área de preparación (staging area) // Array global que representa el area de preparacion
int staging_count = 0; // Contador de archivos en staging area
char current_branch_head[65] = ""; // ID del commit actual

// Implementacion de funciones auxiliares para la manipulacion de datos
// --------------------------------------------------------------------------------

// Esta seccion contiene funciones de bajo nivel que se utilizan en las operaciones principales.
// Incluye una funcion para generar un hash simple, una funcion para hashear una cadena,
// y funciones para guardar y cargar commits y el area de preparacion en archivos.

char* calculate_hash(const char* data) { // Funcion que genera un hash simple a partir de datos
    char* hash = (char*)malloc(65 * sizeof(char));
    if (hash == NULL) return NULL;
    
    // Hash simple basado en el contenido
    unsigned long hash_value = 5381;
    int c;
    const char* str = data;
    
    while ((c = *str++)) {
        hash_value = ((hash_value << 5) + hash_value) + c; /* hash * 33 + c */
    }
    
    // Agregar timestamp para hacer mas unico
    hash_value += (unsigned long)time(NULL);
    
    snprintf(hash, 65, "%lx", hash_value);
    return hash;
}

int hash_function(const char* key) { // Funcion que calcula un indice para la tabla hash
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash = (hash * 31 + key[i]) % HASH_SIZE;
    }
    return hash;
}

void save_commit(const Commit* commit) { // Guarda la informacion de un commit en un archivo
    char path[128];
    sprintf(path, ".ugit/commits/%s.commit", commit->commit_id);
    FILE* file = fopen(path, "wb");
    if (!file) {
        perror("Error al guardar el commit");
        return;
    }
    fwrite(commit, sizeof(Commit), 1, file);
    fclose(file);
}

Commit* load_commit(const char* commit_id) { // Carga la informacion de un commit desde un archivo
    char path[128];
    sprintf(path, ".ugit/commits/%s.commit", commit_id);
    FILE* file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }
    Commit* commit = (Commit*)malloc(sizeof(Commit));
    if (commit) {
        fread(commit, sizeof(Commit), 1, file);
    }
    fclose(file);
    return commit;
}

void save_staging_area() { // Guarda el estado actual del staging area
    FILE* file = fopen(".ugit/index", "wb");
    if (!file) {
        perror("Error al guardar el area de preparacion (staging area)");
        return;
    }
    fwrite(&staging_count, sizeof(int), 1, file);
    fwrite(staging_area, sizeof(FileEntry), staging_count, file);
    fclose(file);
}

void load_staging_area() { // Carga el estado del staging area desde archivo
    FILE* file = fopen(".ugit/index", "rb");
    if (!file) {
        staging_count = 0;
        return;
    }
    fread(&staging_count, sizeof(int), 1, file);
    fread(staging_area, sizeof(FileEntry), staging_count, file);
    fclose(file);
}

void load_current_head() { // Carga el commit apuntado por HEAD
    FILE* head_file = fopen(".ugit/HEAD", "r");
    if (head_file) {
        if (fscanf(head_file, "%64s", current_branch_head) != 1) {
            strcpy(current_branch_head, "");
        }
        fclose(head_file);
    } else {
        strcpy(current_branch_head, "");
    }
}

// Implementacion de las funciones principales del simulador
// --------------------------------------------------------------------------------

// Esta seccion implementa las operaciones de control de versiones que el usuario final
// ejecutara, como inicializar un repositorio, agregar archivos, hacer commits, y ver el historial.

void ugit_init() { // Inicializa un repositorio uGit
    if (mkdir(".ugit", 0755) != 0) {
        perror("Error al crear el directorio .ugit");
        return;
    }
    mkdir(".ugit/objects", 0755);
    mkdir(".ugit/commits", 0755);

    // No escribir nada en HEAD al inicializar
    // Se escribira cuando se haga el primer commit
    
    printf("Repositorio uGit inicializado vacio.\n");
}

void ugit_add(const char* filename) { // Agrega un archivo al staging area
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo.");
        return;
    }
    
    // Leer el contenido completo del archivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        printf("Error en la asignacion de memoria.\n");
        return;
    }
    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);

    if (staging_count < MAX_FILES) {
        strcpy(staging_area[staging_count].filename, filename);
        
        // Calcular hash basado en el contenido
        char* hash = calculate_hash(content); // Funcion que genera un hash simple a partir de datos
        if (hash) {
            strcpy(staging_area[staging_count].file_hash, hash);
            
            // Guardar el contenido en el directorio de objetos
            char object_path[256];
            snprintf(object_path, sizeof(object_path), ".ugit/objects/%s", hash);
            FILE* object_file = fopen(object_path, "w");
            if (object_file) {
                fwrite(content, 1, file_size, object_file);
                fclose(object_file);
            }
            
            free(hash);
        }
        staging_count++;
        printf("Archivo '%s' agregado a el area de preparacion (staging area)\n", filename);
        save_staging_area(); // Guarda el estado actual del staging area
    } else {
        printf("El area de preparacion esta llena (staging area). No se pueden agregar mas archivos.\n");
    }
    free(content);
}

void ugit_commit(const char* message) { // Crea un nuevo commit
    if (staging_count == 0) {
        printf("No hay commit, arbol de trabajo limpio.\n");
        return;
    }
    
    // Solicitar nombre y correo del autor
    char author_name[128];
    char author_email[128];
    
    printf("Ingrese su nombre: ");
    fflush(stdout);
    if (fgets(author_name, sizeof(author_name), stdin) != NULL) {
        author_name[strcspn(author_name, "\n")] = 0; // Eliminar salto de línea
    } else {
        strcpy(author_name, "Usuario Desconocido");
    }
    
    printf("Ingrese su correo: ");
    fflush(stdout);
    if (fgets(author_email, sizeof(author_email), stdin) != NULL) {
        author_email[strcspn(author_email, "\n")] = 0; // Eliminar salto de línea
    } else {
        strcpy(author_email, "desconocido@ejemplo.com");
    }
    
    Commit new_commit;
    char* commit_id = calculate_hash(message); // Funcion que genera un hash simple a partir de datos
    if (!commit_id) {
        printf("Error en el calculo del hash.\n");
        return;
    }
    strcpy(new_commit.commit_id, commit_id);
    free(commit_id);

    if (strlen(current_branch_head) > 0) {
        strcpy(new_commit.parent_id, current_branch_head);
    } else {
        strcpy(new_commit.parent_id, "root-commit");
    }

    new_commit.file_count = staging_count;
    for (int i = 0; i < staging_count; i++) {
        strcpy(new_commit.files[i].filename, staging_area[i].filename);
        strcpy(new_commit.files[i].file_hash, staging_area[i].file_hash);
    }
    
    strcpy(new_commit.message, message);
    strcpy(new_commit.author_name, author_name);
    strcpy(new_commit.author_email, author_email);
    new_commit.timestamp = time(NULL);

    save_commit(&new_commit); // Guarda la informacion de un commit en un archivo
    strcpy(current_branch_head, new_commit.commit_id);
    
    FILE* head_file = fopen(".ugit/HEAD", "w");
    if (head_file) {
        fprintf(head_file, "%s", current_branch_head);
        fclose(head_file);
    }
    
    staging_count = 0;
    save_staging_area();

    printf("[ master (%s) %s ] %s\n", new_commit.parent_id, new_commit.commit_id, new_commit.message);
    printf("Autor: %s <%s>\n", author_name, author_email);
    printf("%d archivo modificado, %d inserciones (+)\n", new_commit.file_count, new_commit.file_count * 10);
}

void ugit_log() { // Muestra el historial de commits
    char current_commit_id[65];
    
    // Lee el ID del commit mas reciente desde el archivo HEAD.
    FILE* head_file = fopen(".ugit/HEAD", "r");
    if (head_file) {
        if (fscanf(head_file, "%64s", current_commit_id) != 1) {
            printf("Sin commits aun.\n");
            return;
        }
        fclose(head_file);
    } else {
        printf("Sin commits aun.\n");
        return;
    }

    // Recorre el historial de commits y los muestra en orden cronologico inverso.
    while (strcmp(current_commit_id, "root-commit") != 0 && strlen(current_commit_id) > 0) {
        Commit* commit = load_commit(current_commit_id); // Carga la informacion de un commit desde un archivo
        if (!commit) {
            printf("Error: Commit %s no encontrado.\n", current_commit_id);
            break;
        }

        char date_str[64];
        strftime(date_str, sizeof(date_str), "%a %b %d %H:%M:%S %Y %z", localtime(&commit->timestamp));

        printf("Commit: %s\n", commit->commit_id);
        printf("Autor: %s <%s>\n", commit->author_name, commit->author_email);
        printf("Fecha: %s\n\n", date_str);
        printf("    %s\n\n", commit->message);

        // Verificar si es el commit raiz ANTES de intentar cargar el siguiente
        if (strcmp(commit->parent_id, "root-commit") == 0) {
            // Es el primer commit, terminar el bucle
            free(commit);
            break;
        }

        // Se mueve al commit padre para la siguiente iteracion del bucle.
        strcpy(current_commit_id, commit->parent_id);
        free(commit);
    }
}

void ugit_checkout(const char* commit_id) { // Restaura el proyecto a un commit especifico
    // Cargar el commit solicitado
    Commit* commit = load_commit(commit_id); // Carga la informacion de un commit desde un archivo
    if (!commit) {
        printf("error: pathspec '%s' No se encontro ningun archivo conocido por uGit.\n", commit_id);
        return;
    }
    
    // Restaurar cada archivo del commit
    for (int i = 0; i < commit->file_count; i++) {
        char* filename = commit->files[i].filename;
        char* file_hash = commit->files[i].file_hash;
        
        // Buscar el archivo en el directorio de objetos
        char object_path[256];
        snprintf(object_path, sizeof(object_path), ".ugit/objects/%s", file_hash);
        
        // Intentar restaurar desde el objeto guardado
        FILE* object_file = fopen(object_path, "r");
        if (object_file) {
            // Si existe el objeto, restaurar desde ahi
            FILE* target_file = fopen(filename, "w");
            if (target_file) {
                char buffer[1024];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), object_file)) > 0) {
                    fwrite(buffer, 1, bytes, target_file);
                }
                fclose(target_file);
                printf("Archivo '%s' restaurado\n", filename);
            } else {
                printf("Error: No se pudo crear el archivo '%s'\n", filename);
            }
            fclose(object_file);
        } else {
            // Si no existe el objeto, crear un archivo placeholder
            printf("Advertencia: Contenido del archivo '%s' no disponible, creando archivo vacio\n", filename);
            FILE* target_file = fopen(filename, "w");
            if (target_file) {
                fprintf(target_file, "// Archivo restaurado desde commit %s\n", commit_id);
                fprintf(target_file, "// Contenido original no disponible\n");
                fclose(target_file);
            }
        }
    }
    
    // Actualizar el HEAD al nuevo commit
    strcpy(current_branch_head, commit_id);
    FILE* head_file = fopen(".ugit/HEAD", "w");
    if (head_file) {
        fprintf(head_file, "%s", current_branch_head);
        fclose(head_file);
    }
    
    // Limpiar el staging area ya que estamos en un estado "limpio"
    staging_count = 0;
    save_staging_area(); // Guarda el estado actual del staging area
    
    printf("Cambiado a commit %s\n", commit_id);
    printf("HEAD ahora apunta a %s\n", commit_id);
    
    free(commit);
}
