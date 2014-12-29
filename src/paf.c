#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

//#define DEBUG

#define LOGE(...) err(1, __VA_ARGS__)
#ifdef DEBUG
    #define LOGD(...) err(0, ##__VA_ARGS__)
#else
    #define LOGD(...)
#endif
#define err(iserr, ...) do { \
    fprintf(stderr, ##__VA_ARGS__); \
    if (iserr) \
        exit(1); \
} while(0)

#define FIFO_MODE (S_IRUSR | S_IWUSR)
#define DEFUALT_FILE "/tmp/default"
#define BUFFSIZE 4096

/*
 * Get and replace the pattern
 */
int gr_filename(char *filename, char **outargs, int *force, char **inargs)
{
    int fnlen=0;
    int offset=1;
    int returnv=-1;
    char **pain = inargs;
    char **paout = outargs;
    while (*pain) {
        if ((*pain)[0] == '{' && (*pain)[strlen(*pain) - 1] == '}') {
            if ((*pain)[1] == '!')
                *force = 1;
            else
                *force = 0;
            fnlen = strlen(*pain) - 2 - *force;
            offset = 1 + *force;
            if (fnlen <= 0) {
                *paout = strdup(DEFUALT_FILE);
                returnv = 0;
            } else {
                strncpy(filename, *pain + offset, fnlen);
                *paout = strdup(filename);
                returnv = 1;
            }
        } else
            *paout = strdup(*pain);
        paout++;
        pain++;
    }
    *paout = NULL;
    return returnv;
}

void usage(char *command)
{
    LOGE("[PAF] usage: %s COMMAND [OPTION...] PATTERN [OPTION...]\n"
         "[PAF] PATTERN:\n"
         "[PAF] \t{}:\t\t\tPATTERN will be replaced with /tmp/default\n"
         "[PAF] \t{/path/to/file}:\tPATTERN will be replaced with /path/to/file\n"
         "[PAF] \t{!/path/to/file}:\tForce mode\n",
         command);
}

int main(int argc, char **argv)
{
    int wfd;
    int fnresult;
    int force=0;
    pid_t child_pid;
    char buf[BUFFSIZE];
    char filename[100];
    char *argste[argc];

    void sigint_handler()
    {
        kill(child_pid, SIGINT);
        close(wfd);
        unlink(filename);
        waitpid(child_pid, NULL, 0);
        exit(1);
    }
    signal(SIGINT, sigint_handler);

    if (argc < 3)
        usage(*argv);

    bzero(filename, 100);
    fnresult = gr_filename(filename, argste, &force, argv + 1);
    if (fnresult == -1)
        usage(*argv);
    else if (fnresult == 0) {
        LOGD("[PAF] Filename not found, using default filename(/tmp/default).\n");
        strcpy(filename, DEFUALT_FILE);
    } else {
        LOGD("[PAF] Filename: %s\n", filename);
    }

    if (mkfifo(filename, FIFO_MODE) < 0 && errno == EEXIST && !force)
        LOGE("[PAF] %s exits, quiting.\n", filename);

    if ((child_pid = fork()) == 0) {
        /* reopen real stdin */
        int real_stdinfd = open("/dev/tty", O_RDONLY, NULL);
        dup2(real_stdinfd, STDIN_FILENO);
        close(real_stdinfd);
        execvp(argv[1], argste);
    }

    /* TODO working on the FIFO for multiple opening */
    wfd = open(filename, O_WRONLY, NULL);
    bzero(buf, BUFFSIZE);
    /* sendfile does not support pipe, use splice instead */
    while (splice(STDIN_FILENO, NULL, wfd, NULL, 1024, 0)) ;
    close(wfd);
    unlink(filename);
    waitpid(child_pid, NULL, 0);
    return 0;
}
