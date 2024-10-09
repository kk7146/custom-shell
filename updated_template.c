#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_CMD_SIZE (128)
#define BASE_DIR "/tmp/test"

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

void list_directory() {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(".")) == NULL) {
        perror("ls");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s (%s)\n", entry->d_name, (entry->d_type == DT_DIR) ? "디렉토리" : "파일");
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    char *command, *tok_str;
    char current_path[MAX_CMD_SIZE];

    // /tmp/test 디렉토리 확인 및 생성
    struct stat st;
    if (stat(BASE_DIR, &st) != 0) {
        if (mkdir(BASE_DIR, 0755) != 0) {
            perror("mkdir");
            return 1;
        }
    }

    // 기본 디렉토리로 이동
    chdir(BASE_DIR);

    command = (char *)malloc(MAX_CMD_SIZE);
    if (command == NULL) {
        perror("malloc");
        exit(1);
    }

    do {
        // 현재 디렉토리 경로 표시
        getcwd(current_path, sizeof(current_path));
        printf("%s $ ", current_path + strlen(BASE_DIR)); // BASE_DIR 제외하고 출력
        if (fgets(command, MAX_CMD_SIZE - 1, stdin) == NULL) break;

        tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

        if (strcmp(tok_str, "quit") == 0) {
            break;
        } else if (strcmp(tok_str, "help") == 0) {
            show_help();
        } else if (strcmp(tok_str, "cd") == 0) {
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                // 상위 디렉토리 이동 처리
                if (strcmp(tok_str, "..") == 0) {
                    if (strcmp(current_path, BASE_DIR) == 0) {
                        fprintf(stderr, "cd: cannot go to parent directory\n");
                    } else {
                        // 상위 디렉토리로 이동
                        chdir("..");
                    }
                } else {
                    char new_path[MAX_CMD_SIZE];
                    snprintf(new_path, sizeof(new_path), "%s/%s", BASE_DIR, tok_str);
                    if (chdir(new_path) != 0) {
                        perror("cd");
                    }
                }
            }
        } else if (strcmp(tok_str, "mkdir") == 0) {
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                fprintf(stderr, "mkdir: missing argument\n");
            } else {
                char new_dir[MAX_CMD_SIZE];
                snprintf(new_dir, sizeof(new_dir), "%s/%s", BASE_DIR, tok_str);
                if (mkdir(new_dir, 0755) != 0) {
                    perror("mkdir");
                }
            }
        } else if (strcmp(tok_str, "rmdir") == 0) {
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                fprintf(stderr, "rmdir: missing argument\n");
            } else {
                char dir_to_remove[MAX_CMD_SIZE];
                snprintf(dir_to_remove, sizeof(dir_to_remove), "%s/%s", BASE_DIR, tok_str);
                if (rmdir(dir_to_remove) != 0) {
                    perror("rmdir");
                }
            }
        } else if (strcmp(tok_str, "rename") == 0) {
            char *source = strtok(NULL, " \n");
            char *target = strtok(NULL, " \n");
            if (source == NULL || target == NULL) {
                fprintf(stderr, "rename: missing arguments\n");
            } else {
                char source_path[MAX_CMD_SIZE], target_path[MAX_CMD_SIZE];
                snprintf(source_path, sizeof(source_path), "%s/%s", BASE_DIR, source);
                snprintf(target_path, sizeof(target_path), "%s/%s", BASE_DIR, target);
                if (rename(source_path, target_path) != 0) {
                    perror("rename");
                }
            }
        } else if (strcmp(tok_str, "ls") == 0) {
            list_directory();
        } else {
            printf("Unknown command: %s\n", tok_str);
        }
    } while (1);

    free(command);
    return 0;
}
