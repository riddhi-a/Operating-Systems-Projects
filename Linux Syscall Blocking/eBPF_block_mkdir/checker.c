// Check if the app.py script is working correctly or not

#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h> // For mount() and related constants
#include <sys/wait.h>  // For waitpid()
#include <signal.h>    // For SIGCHLD
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>  // For struct stat

#define CURR_NS "test_dir_curr_ns"
#define NEW_NS "test_dir_new_ns"

void print_namespace_info(const char *label) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/self/ns/mnt"); // Path to the mount namespace file

    struct stat ns_stat;
    // Get namespace info
    if (stat(path, &ns_stat) == -1) {
        perror("Error: Unable to stat namespace file");
        exit(EXIT_FAILURE);
    }

    printf("%s: Inode = %ld\n", label, (long)ns_stat.st_ino);
}

void create_directory(const char *dir_name) {
    // try to syscall mkdir and note down the error if got any or print success
    if (mkdir(dir_name, 0755) == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Error: Directory '%s' already exists.\n", dir_name);
        } else {
            fprintf(stderr, "Error: Unable to create directory '%s' - %s\n", dir_name, strerror(errno));
        }
    } else {
        printf("Directory '%s' created successfully.\n", dir_name);
    }
}

int create_in_new_namespace() {
    // Ensure the new namespace is private
    // source, target, filesystemtype, mountflags and data
    if (unshare(CLONE_NEWNS) == -1) {
        perror("Error: Unable to create new mount namespace\n");
        exit(EXIT_FAILURE);
    }
    printf("\nInside new namespace...\n");
    print_namespace_info("New Namespace");

    create_directory(NEW_NS);
    return 0;
}

int main() {
    printf("Creating directory in the current namespace...\n");
    print_namespace_info("Current Namespace");

    int pid = fork();
    if(pid == 0){  // child process
        create_directory(CURR_NS);
    }else{ // parent process
        // child pid, status returned, option(0:- wait till it's exit)
        if(waitpid(pid, NULL, 0) == -1){
            perror("Some error occured while making directory in current namespace\n");
        }
        create_in_new_namespace();
    }
    return 0;
}
