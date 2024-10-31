#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define MAX_CMD_SIZE    (128)
#define BASE_DIR "/private/tmp/test"

void cp_func(const char *source, const char *destination) {
    FILE *src_file = fopen(source, "r");
    if (src_file == NULL) {
        perror("cp (source)");
        return;
    }
    FILE *dest_file = fopen(destination, "w");
    if (dest_file == NULL) {
        perror("cp (destination)");
        fclose(src_file);
        return;
    }
    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes, dest_file);
    }
    fclose(src_file);
    fclose(dest_file);
    printf("File copied from %s to %s\n", source, destination);
}

void cat_func(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("cat");
        return;
    }
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }
    fclose(file);
}
void apply_symbolic_permissions(char *permissions, mode_t *mode) {
    int user_flag = 0, group_flag = 0, other_flag = 0;
    char *perm = permissions;
    
    if (*perm == 'u') { user_flag = 1; perm++; }
    else if (*perm == 'g') { group_flag = 1; perm++; }
    else if (*perm == 'o') { other_flag = 1; perm++; }
    else { user_flag = group_flag = other_flag = 1; }

    int add = (*perm == '+') ? 1 : (*perm == '-') ? 0 : -1;
    perm++;
    
    while (*perm) {
        if (*perm == 'r') {
            if (user_flag) *mode = add ? (*mode | S_IRUSR) : (*mode & ~S_IRUSR);
            if (group_flag) *mode = add ? (*mode | S_IRGRP) : (*mode & ~S_IRGRP);
            if (other_flag) *mode = add ? (*mode | S_IROTH) : (*mode & ~S_IROTH);
        } else if (*perm == 'w') {
            if (user_flag) *mode = add ? (*mode | S_IWUSR) : (*mode & ~S_IWUSR);
            if (group_flag) *mode = add ? (*mode | S_IWGRP) : (*mode & ~S_IWGRP);
            if (other_flag) *mode = add ? (*mode | S_IWOTH) : (*mode & ~S_IWOTH);
        } else if (*perm == 'x') {
            if (user_flag) *mode = add ? (*mode | S_IXUSR) : (*mode & ~S_IXUSR);
            if (group_flag) *mode = add ? (*mode | S_IXGRP) : (*mode & ~S_IXGRP);
            if (other_flag) *mode = add ? (*mode | S_IXOTH) : (*mode & ~S_IXOTH);
        }
        perm++;
    }
}

void chmod_func(char *perm_str, char *filename) {
    struct stat statbuf;
    mode_t mode;

    if (stat(filename, &statbuf) != 0) {
        perror("stat");
        return;
    }
    mode = statbuf.st_mode;

    if (perm_str[0] >= '0' && perm_str[0] <= '7') {

        mode_t new_mode = strtol(perm_str, NULL, 8);
        if (chmod(filename, new_mode) != 0) {
            perror("chmod");
        } else {
            printf("Permissions changed to %o for %s\n", new_mode, filename);
        }
    } else {

        apply_symbolic_permissions(perm_str, &mode);
        if (chmod(filename, mode) != 0) {
            perror("chmod");
        } else {
            printf("Permissions changed for %s\n", filename);
        }
    }
}

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
    printf("help                   : Show this help message\n");
    printf("cd <path>              : Change directory to <path>\n");
    printf("mkdir <path>           : Create a new directory at <path>\n");
    printf("rmdir <path>           : Remove the directory at <path>\n");
    printf("rename <source> <target> : Rename <source> to <target>\n");
    printf("ln <original> <new>    : Create a hard link from <new> to <original>\n");
    printf("ln -s <original> <new> : Create a symbolic link from <new> to <original>\n");
    printf("chmod <perm> <file>    : Change <file> permissions, e.g., 0644 or u+rwx\n");
    printf("cat <filename>         : Display the contents of <filename>\n");
    printf("cp <source> <dest>     : Copy <source> file to <dest>\n");
    printf("rm <file>              : Remove the specified <file>\n");
    printf("ls                     : List contents of the current directory with details\n");
    printf("quit                   : Exit the shell\n");
}

void print_permissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : (S_ISLNK(mode)) ? "l" : (S_ISFIFO(mode)) ? "p" :
           (S_ISCHR(mode)) ? "c" : (S_ISBLK(mode)) ? "b" : (S_ISSOCK(mode)) ? "s" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void ls_func() {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    struct passwd *pw;
    struct group *gr;
    char timebuf[64];
    char link_target[MAX_CMD_SIZE + 1];

    dir = opendir(".");
    if (dir == NULL) {
        perror("ls");
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (lstat(entry->d_name, &statbuf) == -1) {
            perror("lstat");
            continue;
        }
        print_permissions(statbuf.st_mode);
        printf(" %ld ", (long)statbuf.st_nlink);
        pw = getpwuid(statbuf.st_uid);
        gr = getgrgid(statbuf.st_gid);
        printf("%s %s ", pw->pw_name, gr->gr_name);
        printf("%5lld ", (long long)statbuf.st_size);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&statbuf.st_atime));
        printf("Access: %s ", timebuf);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&statbuf.st_mtime));
        printf("Modify: %s ", timebuf);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&statbuf.st_ctime));
        printf("Create: %s ", timebuf);
        printf("%s", entry->d_name);
        if (S_ISLNK(statbuf.st_mode)) {
            ssize_t len = readlink(entry->d_name, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf(" -> %s", link_target);
            }
        }
        printf("\n");
    }
    closedir(dir);
}

int main(int argc, char **argv) {
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
        if (!validate_path(current_dir)) {
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
            } else if (!validate_path(current_dir)) {
                perror("cd");
            } else if (chdir(tok_str) != 0)
                perror("cd");
        } else if (strcmp(tok_str, "help") == 0)
            show_help();
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
    } while (1);
    free(command);
    return 0;
}
