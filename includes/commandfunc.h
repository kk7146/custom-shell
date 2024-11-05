#ifndef COMMANDFUNC_H
#define COMMANDFUNC_H

#define MAX_CMD_SIZE    (128)

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


void ls_func();
void chmod_func(char *perm_str, char *filename);
void cp_func(const char *source, const char *destination);
void cat_func(char *filename);
void apply_symbolic_permissions(char *permissions, mode_t *mode);
void help_func();
void print_permissions(mode_t mode);

#endif