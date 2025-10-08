#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define CLIENT_FILES_DIR "client_files"

// Function to setup client_files directory
// Deletes and recreates if it exists, creates if it doesn't
// Ensures the directory is empty
void setup_client_files_dir();

#endif /* UTILS_H */
