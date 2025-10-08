#include "utils.h"
#include <string.h>
#include <dirent.h>

// didn't want to duplicate code so i just put it in a function

void setup_client_files_dir() {
    struct stat st;

    // Check if directory exists
    if (stat(CLIENT_FILES_DIR, &st) == 0) {
        // Directory exists, remove it and all contents
        printf("Removing existing %s directory...\n", CLIENT_FILES_DIR);

        // Use system rm -rf to remove directory and contents
        char command[256];
        snprintf(command, sizeof(command), "rm -rf %s", CLIENT_FILES_DIR);
        if (system(command) != 0) {
            perror("Failed to remove existing client_files directory");
            exit(1);
        }
    }

    // Create the directory
    printf("Creating %s directory...\n", CLIENT_FILES_DIR);
    if (mkdir(CLIENT_FILES_DIR, 0755) != 0) {
        perror("Failed to create client_files directory");
        exit(1);
    }

    printf("%s directory is ready.\n", CLIENT_FILES_DIR);
}
