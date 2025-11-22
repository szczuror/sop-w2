#define _GNU_SOURCE

#include <bits/types/sigset_t.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR(source) \
    (kill(0, SIGKILL), perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define FILE_MAX_SIZE 512

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void usage(int argc, char* argv[])
{
    printf("%s p h\n", argv[0]);
    printf("\tp - path to directory describing the structure of the Austro-Hungarian office in Prague.\n");
    printf("\th - Name of the highest administrator.\n");
    exit(EXIT_FAILURE);
}

volatile sig_atomic_t sig_usr1 = 0;
void sigusr1_handler(int sig) { sig_usr1 = sig; }

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void e2(char* path, char* name) {
    printf("My name is %s and my PID is %d\n", name, getpid());
    char cwd[FILE_MAX_SIZE];
    getcwd(cwd, FILE_MAX_SIZE);
    chdir(path);
    // sprintf(path + strlen(path), "%s", name);
    // printf("%s\n", path);
    int fd = open(name, O_RDONLY);
    if (fd == -1)
        ERR("open");
    char file_buf[FILE_MAX_SIZE];
    bulk_read(fd, file_buf, FILE_MAX_SIZE);
    char* linia = strtok(file_buf, "\n");
    while(linia != NULL){
        printf("%s inspecting %s\n", name, linia);
        linia = strtok(NULL, "\n");
        }
    }
    printf("%s has inspected all subordinates\n", name);
    chdir(cwd);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
        usage(argc, argv);
    
    char *path = argv[1];
    char *name = argv[2];

    sigset_t mask, oldmask;
    sigemptyset(&mask);

    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);

    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    sethandler(sigusr1_handler, SIGUSR1);

    printf("Waiting for SIGUSR1\n");
    // sigsuspend(&oldmask);
    printf("Got SIGUSR1\n");

    e2(path, name);
}
