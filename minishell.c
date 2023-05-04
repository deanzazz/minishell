#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define BLUE "\x1b[34;1m"
#define GREEN "\x1b[32;1m"
#define DEFAULT "\x1b[0m"

volatile int a;

void sig_handler(int sig)
{
    a = 1;
    printf("\n");
}

void find(char *dirname, char *filename)            //find, extra credit item 2
{
    DIR *dir = opendir(dirname);
    if (dir == NULL)
    {
        fprintf(stderr, "Error: Cannot open directory '%s'. %s.\n", dirname, strerror(errno));
        return;
    }
    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            continue;
        }
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirname, dp->d_name);
        struct stat fileStat;
        if (stat(path, &fileStat) == -1)
        {
            fprintf(stderr, "Error: Cannot stat file '%s'. %s.\n", path, strerror(errno));
            continue; 
        }
        if (S_ISREG(fileStat.st_mode)) 
        {
            if (strcmp(dp->d_name, filename) == 0)
            {
                printf("%s/%s\n", dirname, filename);
            }
        }
        else if (S_ISDIR(fileStat.st_mode))
        {
            find(path, filename);
        }
    }
    closedir(dir);
}

int cd(char *arg)
{
    if (arg == NULL)
    {
        arg = "";
    }
    if ((strcmp(arg, "") == 0) || (strcmp(arg, "~") == 0))
    {
        uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);
        if (getpwuid == NULL)
        {
            fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
            return -1;
        }
        if (chdir(pw->pw_dir) == -1)
        {
            fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", arg, strerror(errno));
            return -1;
        }
    }
    else
    {
        if (chdir(arg) == -1)
        {
            fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", arg, strerror(errno));
            return -1;
        }
    }
    return 0;
}

void exitShell()
{
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    struct sigaction prevaction;
    if (sigaction(SIGINT, &sa, &prevaction) == -1)
    {
        fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
        return 1;
    }
    while (1)
    {
        fflush(stdout);
        clearerr(stderr);
        fflush(stderr);
        if (a == 1)
        {
            a = 0;
            continue;
        }
        char* cwd = getcwd(NULL, 0);
        if (cwd == NULL)
        {
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            exitShell();
        }
        printf("[%s%s%s]> ", BLUE, cwd, DEFAULT);
        free(cwd);
        char arg[2048];
        if (fgets(arg, sizeof(arg), stdin) == NULL)
        {
            if (a == 1)
            {
                a = 0;
                continue;
            }
            else
            {
                fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
            }
        }
        // printf("%s", arg);
        size_t len = strlen(arg);
        if (len > 0 && arg[len - 1] == '\n')
        {
            arg[len - 1] = '\0';
        }
        if (strcmp(arg, "exit") == 0)
        {
            exitShell();
        }
        else if (strcmp(arg, "cd") == 0)
        {
            cd("");
            continue;
        }
        else
        {
            char cmd[1024];
            char *listOfArgs[256];
            memset(cmd, 0, sizeof(cmd));
            for (int i = 0; i < sizeof(listOfArgs) / sizeof(listOfArgs[0]); i++)
            {
                listOfArgs[i] = NULL;
            }
            int i = 0;
            int argcount = 0;
            char *token = strtok(arg, " ");
            if (token != NULL)
            {
                strncpy(cmd, token, sizeof(cmd) - 1);
                listOfArgs[i] = token; // Set the command as the first argument
                i++;
                argcount++;
                while ((token = strtok(NULL, " ")) != NULL)
                {
                    listOfArgs[i] = token;
                    i++;
                    argcount++;
                }
                listOfArgs[i] = NULL;
            }
            if (strcmp(cmd, "") == 0)
            {
                continue;
            }
            else if (strcmp(cmd, "cd") == 0)
            {
                int size = 0;
                while (listOfArgs[size] != NULL)
                {
                    size++;
                }
                if (size > 2)
                {
                    fprintf(stderr, "Error: Too many arguments to cd.\n");
                    continue;
                }
                else
                {
                    cd(listOfArgs[1]);
                }
            }
            else if (strcmp(cmd, "ls") == 0 && (argcount == 1 || (argcount == 2 && listOfArgs[1][0] != '-')))
            { // colorized ls command, extra credit 1
                DIR *dir;
                struct dirent *dp;
                struct stat fileStat;
                char path[1024];
                char buffer[2048];
                if (argcount == 1)
                {
                    strcpy(path, ".");
                }
                else if (argcount == 2)
                {
                    strcpy(path, listOfArgs[1]);
                    if (stat(path, &fileStat) < 0)
                    {
                        fprintf(stderr, "Directory doesn't exist\n");
                        continue;
                    }
                    if (S_ISDIR(fileStat.st_mode) == 0)
                    {
                        fprintf(stderr, "Not a directory\n");
                        continue;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: Too many arguments to ls.\n");
                    continue;
                }
                if ((dir = opendir(path)) == NULL)
                {
                    fprintf(stderr, "Directory doesn't exist\n");
                    continue;
                }
                while ((dp = readdir(dir)) != NULL)
                {
                    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
                    {
                        continue;
                    }
                    sprintf(buffer, "%s/%s", path, dp->d_name);
                    if (stat(buffer, &fileStat) < 0)
                    {
                        fprintf(stderr, "Error: Cannot stat file '%s'. %s.\n", buffer, strerror(errno));
                        continue;
                    }
                    if (S_ISDIR(fileStat.st_mode))
                    {
                        printf("%s%s%s\n", GREEN, dp->d_name, DEFAULT);
                    }
                    else
                    {
                        printf("%s\n", dp->d_name);
                    }
                }
                closedir(dir);
            }
            else if (strcmp(cmd, "find") == 0)
            {
                if (argcount != 3)
                {
                    fprintf(stderr, "Error: Must be two arguments to find.\n");
                    continue;
                }
                find(listOfArgs[1], listOfArgs[2]);
            }
            else
            {
                pid_t pid = fork();
                if (pid == -1)
                {
                    fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
                    continue;
                }
                else if (pid == 0)
                {
                    sigaction(SIGINT, &prevaction, NULL);
                    execvp(cmd, listOfArgs);
                    fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                else
                {
                    int status;
                    if (waitpid(pid, &status, 0) == -1)
                    {
                        if (errno = EINTR)
                        {
                            continue;
                        }
                        else
                        {
                            fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                            continue;
                        }
                    }
                }
            }
        }
    }
    return 0;
}