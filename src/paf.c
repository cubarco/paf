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
 * Get and replace the matched parameter
 */
int gr_filename(char *filename, char **outargs, char **inargs)
{
    int fnlen=0;
    int returnv=-1;
    char **pa = inargs;
    char **pate = outargs;
    while (*pa) {
        if ((*pa)[0] == '{' && (*pa)[strlen(*pa) - 1] == '}') {
            fnlen = strlen(*pa) - 2;
            if (fnlen <= 0) {
                *pate = strdup(*pa);
                returnv = 0;
            } else {
                strncpy(filename, *pa + 1, fnlen);
                *pate = strdup(filename);
                returnv = 1;
            }
        } else
            *pate = strdup(*pa);
        pate++;
        pa++;
    }
    return returnv;
}

int main(int argc, char **argv)
{
    int wfd;
    int fnresult;
    pid_t child_pid;
    char buf[BUFFSIZE];
    char filename[100];
    char *argste[argc - 2];

    if (argc < 3)
        LOGE("[PAF] usage: %s COMMAND [OPTIONS] {/path/to/file} [OPTIONS]\n", *argv);

    bzero(filename, 100);
    fnresult = gr_filename(filename, argste, argv + 1);
    if (fnresult == -1)
        LOGE("[PAF] usage: %s COMMAND [OPTIONS] {/path/to/file} [OPTIONS]\n", *argv);
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
