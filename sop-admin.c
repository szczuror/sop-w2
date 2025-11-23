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
#include <time.h>
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

volatile sig_atomic_t last_sig = 0;
volatile sig_atomic_t sflag = 0;
void sigusr1_handler(int sig)
{
    last_sig = sig;
    if (sig == SIGINT)
        sflag = 1;
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void process(char* path, char* name, int isRoot)
{
    printf("My name is %s and my PID is %d\n", name, getpid());

    srand(time(NULL) * getpid());

    char fullPath[PATH_MAX];
    snprintf(fullPath, PATH_MAX, "%s/%s", path, name);

    int fd = open(fullPath, O_RDONLY);
    if (fd == -1)
        ERR("open");

    char file_buf[FILE_MAX_SIZE + 1];
    ssize_t bytes_read = bulk_read(fd, file_buf, FILE_MAX_SIZE);
    if (bytes_read < 0)
        ERR("bulk_read");
    file_buf[bytes_read] = '\0';
    if (close(fd) == -1)
        ERR("close");

    char* linia = strtok(file_buf, "\n");
    while (linia != NULL)
    {
        if (strcmp(linia, "-") != 0)
        {
            printf("%s inspecting %s\n", name, linia);

            pid_t pid = fork();

            if (pid == -1)
                ERR("fork");

            if (pid == 0)
            {
                process(path, linia, 0);  // dziecko nie jest rootem, wiec 0
                exit(EXIT_SUCCESS);
            }
        }

        linia = strtok(NULL, "\n");
    }

    printf("%s has inspected all subordinates\n", name);

    // oczekiwanie na SIGUSR2
    sigset_t suspendMask;
    sigemptyset(&suspendMask);

    while (1)
    {
        if (sflag == 1)
            break;
        sigsuspend(&suspendMask);
        if (sflag == 1)
            break;
        if (last_sig == SIGUSR2)
        {
            last_sig = 0;

            int action = rand() % 3;

            if (action == 0)
            {
                printf("%s received a document. Sending to the archive\n", name);
            }
            else
            {
                if (isRoot == 1)
                {
                    printf("%s received a document. Ignoring\n", name);
                }
                else
                {
                    printf("%s received a document. Passing on to the superintendent\n", name);
                    if (kill(getppid(), SIGUSR2) == -1)
                        ERR("kill");
                }
            }
        }
    }

    printf("%s ending the day.\n", name);

    sethandler(SIG_IGN, SIGINT);  // sigint zabije dzieci a nie sam proces ktory dostaje sygnal

    kill(0, SIGINT);

    while (wait(NULL) > 0)
        ;

    printf("%s is leaving the office\n", name);

    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
        usage(argc, argv);

    char* path = argv[1];
    char* name = argv[2];

    sigset_t mask, oldmask;
    sigemptyset(&mask);

    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);

    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    sethandler(sigusr1_handler, SIGUSR1);
    sethandler(sigusr1_handler, SIGUSR2);
    sethandler(sigusr1_handler, SIGINT);

    printf("Waiting for SIGUSR1\n");
    sigsuspend(&oldmask);
    printf("Got SIGUSR1\n");
    if (last_sig == SIGUSR1)
    {
        last_sig = 0;
        process(path, name, 1);
    }
}
