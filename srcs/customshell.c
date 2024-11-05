#include "customshell.h"

#define MAX_CMD_SIZE    (128)
#define BASE_DIR "/private/tmp/test"

int validate_path(char path[MAX_CMD_SIZE]) { // 유효한 위치인지 확인.
    char absolute_path[MAX_CMD_SIZE];
    if (realpath(path, absolute_path) == NULL) {
        /*empty*/
    }
    if (strncmp(BASE_DIR, absolute_path, strlen(BASE_DIR)) != 0) {
        return 0;
    }
    return 1;
}

char* resolve_path(const char *path) { // path를 받고 유효한 위치로 변환해서 리턴.
    static char resolved_path[MAX_CMD_SIZE];
    if (path[0] == '/')
        snprintf(resolved_path, MAX_CMD_SIZE, "%s%s", BASE_DIR, path);
    else
        snprintf(resolved_path, MAX_CMD_SIZE, "%s/%s", BASE_DIR, path);
    return resolved_path;
}

int ensure_directory_exists() { // 프로그램 실행 경로가 BASE_DIR이 아닌 경우 해당 경로로 이동. 없으면 파일 만들고. 
    struct stat st;

    if (stat(BASE_DIR, &st) != 0) {
        if (mkdir(BASE_DIR, 0755) != 0) {
            perror("mkdir");
            return -1;
        }
    }
    return 0;
}

int check_null_pointer(const char *ch) { // malloc 실패 상황 고려
    if (ch == NULL) {
        perror("malloc");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    char *command, *tok_str;
    char *current_dir;

    if (ensure_directory_exists() == -1)
        return -1;
    if (!validate_path(current_dir))
        chdir(BASE_DIR);

    command = (char*)malloc(MAX_CMD_SIZE);
    if (check_null_pointer(command))
        return -1;

    while (1) {
        current_dir = getcwd(NULL, 0);
        if (check_null_pointer(current_dir)) {
            free(command);
            return -1;
        }

        printf("%s $ ", current_dir + strlen(BASE_DIR));
        if (fgets(command, MAX_CMD_SIZE-1, stdin) == NULL) {
            free(command);
            free(current_dir);
            return -1;
        }
        tok_str = strtok(command, " \n");
        if (tok_str == NULL)
            continue;

        if (strcmp(tok_str, "quit") == 0)
            break;
        else if (strcmp(tok_str, "cd") == 0) {
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                printf("cd: missing argument\n");
            } else if (!validate_path(current_dir) || !validate_path(tok_str)) {
                perror("cd");
            } else if (chdir(resolve_path(tok_str)) != 0)
                perror("cd");
        } else if (strcmp(tok_str, "help") == 0)
            help_func();
        else if (strcmp(tok_str, "mkdir") == 0) {
            tok_str = strtok(NULL, " \n");
            tok_str = resolve_path(tok_str);
            if (tok_str == NULL) {
                printf("mkdir: missing argument\n");
            } else if (!validate_path(tok_str)) {
                printf("mkdir: cannot create directory\n");
            } else {
                mkdir(tok_str, 0777);
            }
        } else if (strcmp(tok_str, "rmdir") == 0) {
            tok_str = strtok(NULL, " \n");
            tok_str = resolve_path(tok_str);
            if (tok_str == NULL) {
                printf("rmdir: missing argument\n");
            } else if (!validate_path(tok_str)) {
                printf("rmdir: failed to remove\n");
            } else {
                rmdir(tok_str);
            }
        } else if (strcmp(tok_str, "rename") == 0) {
            char *src = strtok(NULL, " \n");
            char *dest = strtok(NULL, " \n");
            if (src == NULL || dest == NULL) {
                printf("rename: missing source or target argument\n");
            } else if (!validate_path(resolve_path(src)) && !validate_path(resolve_path(dest))) {
                printf("rename: failed to rename\n");
            } else {
                rename(src, dest);
            }
        } else if (strcmp(tok_str, "ln") == 0) {
            char *option = strtok(NULL, " \n");
            char *original = NULL;
            char *new_link = NULL;
            if (option && strcmp(option, "-s") == 0) {
                original = strtok(NULL, " \n");
                new_link = strtok(NULL, " \n");
                if (original == NULL || new_link == NULL) {
                    printf("ln: missing original or new link argument\n");
                } else if (symlink(original, new_link) != 0) {
                    perror("ln -s");
                } else {
                    printf("Symbolic link created: %s -> %s\n", new_link, original);
                }
            } else {
                original = option;
                new_link = strtok(NULL, " \n");
                if (original == NULL || new_link == NULL) {
                    printf("ln: missing original or new link argument\n");
                } else if (link(original, new_link) != 0) {
                    perror("ln");
                } else {
                    printf("Hard link created: %s -> %s\n", new_link, original);
                }
            }
        } else if (strcmp(tok_str, "rm") == 0) {
            char *file = strtok(NULL, " \n");
            if (file == NULL) {
                printf("rm: missing argument\n");
            } else if (unlink(file) != 0) {
                perror("rm");
            } else {
                printf("File removed: %s\n", file);
            }
        } else if (strcmp(tok_str, "chmod") == 0) {
            char *perm_str = strtok(NULL, " \n");
            char *filename = strtok(NULL, " \n");
            if (perm_str == NULL || filename == NULL) {
                printf("chmod: missing permission or filename argument\n");
            } else if (!validate_path(resolve_path(filename))) {
                printf("chmod: invalid path\n");
            } else {
                chmod_func(perm_str, filename);
            }
        } else if (strcmp(tok_str, "ls") == 0) {
            ls_func();
        } else if (strcmp(tok_str, "cat") == 0) {
            char *filename = strtok(NULL, " \n");
            if (filename == NULL) {
                printf("cat: missing filename argument\n");
            } else if (!validate_path(resolve_path(filename))) {
                printf("cat: invalid path\n");
            } else {
                cat_func(filename);
            }
        } else if (strcmp(tok_str, "cp") == 0) {
            char *src = strtok(NULL, " \n");
            char *dest = strtok(NULL, " \n");
            if (src == NULL || dest == NULL) {
                printf("cp: missing source or destination argument\n");
            } else if (!validate_path(resolve_path(src)) || !validate_path(resolve_path(dest))) {
                printf("cp: invalid path\n");
            } else {
                cp_func(src, dest);
            }
        } else {
            printf("your command: %s\n", tok_str);
            printf("and argument is ");
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                printf("NULL\n");
            } else {
                printf("%s\n", tok_str);
            }
        }
        free(current_dir);
    }
    free(command);
    free(current_dir);
    return 0;
}
