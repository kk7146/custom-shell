#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

#define MAX_CMD_SIZE    (128)
#define BASE_DIR        "/tmp/test"  // 최상위 디렉터리를 /tmp/test로 설정

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

char* resolve_path(const char *path) {
    static char resolved_path[PATH_MAX];
    if (path[0] == '/') {
        snprintf(resolved_path, PATH_MAX, "%s%s", BASE_DIR, path);
    } else {
        snprintf(resolved_path, PATH_MAX, "%s/%s", BASE_DIR, path);
    }
    return resolved_path;
}

// 현재 디렉터리가 BASE_DIR 아래인지 확인
int validate_dir()
{
    char current_path[PATH_MAX];
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("getcwd");
        return -1;
    }

    // 현재 경로가 BASE_DIR로 시작하지 않으면 유효하지 않음
    if (strncmp(current_path, BASE_DIR, strlen(current_path)) != 0) {
        chdir(BASE_DIR);  // BASE_DIR로 강제 이동
        return 0;  // 유효하지 않음
    }
    return 1;  // 유효함
}

int main(int argc, char **argv)
{
    char *command, *tok_str;
    char *current_dir;

    if (mkdir(BASE_DIR, 0777) && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }

    if (chdir(BASE_DIR) != 0) {
        perror("chdir");
        exit(1);
    }

    command = (char*) malloc(MAX_CMD_SIZE);
    if (command == NULL) {
        perror("malloc");
        exit(1);
    }

    do {
        current_dir = getcwd(NULL, 0);
        printf("%s $ ", current_dir + strlen(BASE_DIR));
        free(current_dir);

        if (fgets(command, MAX_CMD_SIZE-1, stdin) == NULL) break;

        tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

        if (strcmp(tok_str, "quit") == 0) {
            break;
        }
        else if (strcmp(tok_str, "cd") == 0) {
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                char *resolved = resolve_path(tok_str);
                if (chdir(resolved) != 0) {
                    perror("cd");
                }

                // 현재 디렉터리 유효성 검사
                if (validate_dir() == 0) {
                    printf("cd: cannot navigate above root directory. Returning to %s.\n", BASE_DIR);
                }
            }
        }
        else if (strcmp(tok_str, "mkdir") == 0) {
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                fprintf(stderr, "mkdir: missing argument\n");
            } else {
                char *resolved = resolve_path(tok_str);
                if (strncmp(resolved, BASE_DIR, strlen(BASE_DIR)) != 0) {
                    fprintf(stderr, "mkdir: cannot create directory above root directory\n");
                } else {
                    mkdir(resolved, 0777);
                }
            }
        }
        else if (strcmp(tok_str, "rmdir") == 0) {
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                fprintf(stderr, "rmdir: missing argument\n");
            } else {
                char *resolved = resolve_path(tok_str);
                if (strncmp(resolved, BASE_DIR, strlen(BASE_DIR)) != 0) {
                    fprintf(stderr, "rmdir: cannot remove directory above root directory\n");
                } else {
                    rmdir(resolved);
                }
            }
        }
        else if (strcmp(tok_str, "rename") == 0) {
            char *src = strtok(NULL, " \n");
            char *dest = strtok(NULL, " \n");
            if (src == NULL || dest == NULL) {
                fprintf(stderr, "rename: missing source or target argument\n");
            } else {
                char *resolved_src = resolve_path(src);
                char *resolved_dest = resolve_path(dest);
                if (strncmp(resolved_src, BASE_DIR, strlen(BASE_DIR)) != 0 || 
                    strncmp(resolved_dest, BASE_DIR, strlen(BASE_DIR)) != 0) {
                    fprintf(stderr, "rename: cannot rename above root directory\n");
                } else {
                    rename(resolved_src, resolved_dest);
                }
            }
        } else if (strcmp(tok_str, "ls") == 0) {
            ls_func();
        }
        else {
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
