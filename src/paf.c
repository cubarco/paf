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
int gr_filename(char *filename, char **outargs, char **inargs)
{
    int fnlen=0;
    int returnv=-1;
    char **pain = inargs;
    char **paout = outargs;
    while (*pain) {
        if ((*pain)[0] == '{' && (*pain)[strlen(*pain) - 1] == '}') {
            fnlen = strlen(*pain) - 2;
            if (fnlen <= 0) {
                *paout = strdup(DEFUALT_FILE);
                returnv = 0;
            } else {
                strncpy(filename, *pain + 1, fnlen);
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
         "[PAF] \t{!/path/to/file}:\tForce mode(not implemented)\n",
         command);
}

int main(int argc, char **argv)
{
    int wfd;
    int fnresult;
    pid_t child_pid;
    char buf[BUFFSIZE];
    char filename[100];
    char *argste[argc];

    if (argc < 3)
        usage(*argv);

    bzero(filename, 100);
    fnresult = gr_filename(filename, argste, argv + 1);
    if (fnresult == -1)
        usage(*argv);
    else if (fnresult == 0) {
        LOGD("[PAF] Filename not found, using default filename(/tmp/default).\n");
        strcpy(filename, DEFUALT_FILE);
    } else {
        LOGD("[PAF] Filename: %s\n", filename);
    }

    if (mkfifo(filename, FIFO_MODE) < 0 && errno == EEXIST)
        LOGE("[PAF] %s exits, quiting.\n", filename);

    if ((child_pid = fork()) == 0) {
        /* reopen real stdin */
        int real_stdinfd = open("/dev/tty", O_RDONLY, NULL);
        dup2(real_stdinfd, STDIN_FILENO);
        close(real_stdinfd);
        execvp(argv[1], argste);
    }

    wfd = open(filename, O_WRONLY, NULL);
    bzero(buf, BUFFSIZE);
    while (read(STDIN_FILENO, buf, 1024)) /* TODO sendfile not working, confusing */
        write(wfd, buf, 1024);
    close(wfd);
    unlink(filename);
    waitpid(child_pid, NULL, 0);
    return 0;
}
