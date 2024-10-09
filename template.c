/* 
 * 4주차과제 
 * 2020136068
 * 손동은
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_CMD_SIZE    (128)
#define BASE_DIR "/private/tmp/test"

int validate_path(char path[MAX_CMD_SIZE]) {
    char absolute_path[128];
    if (realpath(path, absolute_path) == NULL) {
        /*empty*/
    }
    if (strncmp(BASE_DIR, absolute_path, strlen(BASE_DIR)) != 0) {
        return 0;
    }
    return 1;
}

char* resolve_path(const char *path) {
    static char resolved_path[MAX_CMD_SIZE];
    if (path[0] == '/') 
        snprintf(resolved_path, MAX_CMD_SIZE, "%s%s", BASE_DIR, path);
    else
        snprintf(resolved_path, MAX_CMD_SIZE, "%s/%s", BASE_DIR, path);
    return resolved_path;
}

void show_help() {
    printf("Available commands:\n");
    printf("help                : Show this help message\n");
    printf("cd <path>          : Change directory\n");
    printf("mkdir <path>       : Create a directory\n");
    printf("rmdir <path>       : Remove a directory\n");
    printf("rename <source> <target> : Rename a directory\n");
    printf("ls                 : List current directory contents\n");
    printf("quit               : Exit the shell\n");
}

void ls_func()
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    dir = opendir(".");
    if (dir == NULL) {
        perror("ls");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (stat(entry->d_name, &statbuf) == -1) {
            perror("stat");
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            printf("[DIR] %s\n", entry->d_name);
        } else if (S_ISREG(statbuf.st_mode)) {
            printf("[FILE] %s\n", entry->d_name);
        } else {
            printf("[OTHER] %s\n", entry->d_name);
        }
    }

    closedir(dir);
}

int main(int argc, char **argv)
{
    char *command, *tok_str;
    char *current_dir;

    struct stat st;
    if (stat(BASE_DIR, &st) != 0) {
        if (mkdir(BASE_DIR, 0755) != 0) {
            perror("mkdir");
            return 1;
        }
    }

    if (!validate_path(current_dir)) {
        chdir(BASE_DIR);
    }

    command = (char*) malloc(MAX_CMD_SIZE);
    if (command == NULL) {
        perror("malloc");
        exit(1);
    }

    do {
        current_dir = getcwd(NULL, 0);
        if (!validate_path(current_dir))
        {
            chdir(BASE_DIR);
            current_dir = getcwd(NULL, 0);
        }
        printf("%s $ ", current_dir + strlen(BASE_DIR));
        if (fgets(command, MAX_CMD_SIZE-1, stdin) == NULL) break;
        tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

        if (strcmp(tok_str, "quit") == 0)
            break;
        else if (strcmp(tok_str, "cd") == 0) {
            tok_str = strtok(NULL, " \n");
            tok_str = resolve_path(tok_str);
            if (tok_str == NULL) {
                printf("cd: missing argument\n");
            }
            else if (!validate_path(current_dir))
            {
                perror("cd");
            }
            else if (chdir(tok_str) != 0)
                perror("cd");
        }
        else if (strcmp(tok_str, "help") == 0)
            show_help();
        else if (strcmp(tok_str, "mkdir") == 0) {
            tok_str = strtok(NULL, " \n");
            tok_str = resolve_path(tok_str);
            if (tok_str == NULL) {
                printf("mkdir: missing argument\n");
            }
            else if (!validate_path(tok_str)) {
                printf("mkdir: cannot create directory\n");
            }
            else {
                mkdir(tok_str, 0777);
            }
        }
        else if (strcmp(tok_str, "rmdir") == 0) {
            tok_str = strtok(NULL, " \n");
            tok_str = resolve_path(tok_str);
            if (tok_str == NULL) {
                printf("rmdir: missing argument\n");
            }
            else if (!validate_path(tok_str)) {
                printf("rmdir: failed to remove\n");
            }
            else {
                rmdir(tok_str);
            }
        }
        else if (strcmp(tok_str, "rename") == 0) {
            char *src = strtok(NULL, " \n");
            char *dest = strtok(NULL, " \n");
            if (src == NULL || dest == NULL) {
                printf("rename: missing source or target argument\n");
            } 
            else if (!validate_path(resolve_path(src)) && !validate_path(resolve_path(dest))) {
                printf("rename: failed to rename\n");
            }
            else {
                rename(src, dest);
            }
        } 
        else if (strcmp(tok_str, "ls") == 0)
            ls_func();
        else {
            // TODO: implement functions
            printf("your command: %s\n", tok_str);
            printf("and argument is ");

            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                printf("NULL\n");
            } else {
                printf("%s\n", tok_str);
            }
        }
    } while (1);
    free(command);
    return 0;
}
