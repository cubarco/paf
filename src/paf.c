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
#define BUFFSIZE 1024

struct filenode {
    char *buf;
    int len;
    struct filenode *next;
};

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
    LOGE(
"[PAF] Usage: %s COMMAND [OPTION...] PATTERN... [OPTION...]\n"
"[PAF] PATTERN:\n"
"[PAF] \t{}:\t\t\tPATTERN will be replaced with /tmp/default\n"
"[PAF] \t{/path/to/file}:\tPATTERN will be replaced with /path/to/file\n"
"[PAF] \t{!/path/to/file}:\tForce mode\n"
"[PAF] \n"
"[PAF] Note: If there are more than one PATTERNs in a single command line, the \n"
"[PAF]       PATTERNs should be the same.\n",
         command);
}

static void freeall(struct filenode **pf)
{
    struct filenode **pp = pf;
    struct filenode *tmp;
    while (*pp) {
        tmp = *pp;
        free(tmp->buf);
        pp = &tmp->next;
        free(tmp);
    }
}

int main(int argc, char **argv)
{
    int wfd;
    int fnresult;
    int ioresult;
    int force=0;
    int pipebk=0;
    pid_t child_pid;
    char buf[BUFFSIZE];
    char filename[100];
    char *argste[argc];
    struct filenode *filehead = NULL;
    struct filenode **pf = &filehead;

    void sig_handler(int sig)
    {
        switch (sig) {
            case SIGINT:
                kill(child_pid, SIGINT);
            case SIGCHLD:
                close(wfd);
                unlink(filename);
                waitpid(child_pid, NULL, 0);
                exit(0);
        }
    }
    signal(SIGINT, sig_handler);
    signal(SIGCHLD, sig_handler);
    signal(SIGPIPE, SIG_IGN);

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

    /*
     * Unlink filename right after opening the write end, then mkfifo() for a new
     * fifo. In this way, the read end will block for next open()
     */
    wfd = open(filename, O_WRONLY, NULL);
    unlink(filename);
    mkfifo(filename, FIFO_MODE);
    bzero(buf, BUFFSIZE);

    /* save all data from stdin to memory and write to FIFO */
    while ( (ioresult = read(STDIN_FILENO, buf, BUFFSIZE)) ) {
        *pf = malloc(sizeof(struct filenode));
        (*pf)->next = malloc(sizeof(struct filenode));
        (*pf)->buf = malloc(BUFFSIZE);
        memcpy((*pf)->buf, buf, BUFFSIZE);
        (*pf)->len = ioresult;
        /* if read end has been closed, do not write to it anymore */
        if (pipebk != EPIPE)
            pipebk = write(wfd, buf, ioresult);
        pf = &(*pf)->next;
    }
    free(*pf);
    *pf = NULL;
    close(wfd);

    /* start second loop for multiple opening */
    for (;;) {
        pf = &filehead;
        wfd = open(filename, O_WRONLY, NULL);
        unlink(filename);
        mkfifo(filename, FIFO_MODE);
        while (*pf) {
            ioresult = write(wfd, (*pf)->buf, (*pf)->len);
            if (ioresult == EPIPE)
                break;
            pf = &(*pf)->next;
        }
        close(wfd);
    }

    unlink(filename);
    waitpid(child_pid, NULL, 0);
    return 0;
}
