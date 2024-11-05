#include "commandfunc.h"

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

void help_func() {
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