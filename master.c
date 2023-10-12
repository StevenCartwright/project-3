#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define MAX_PROCESSES 20
#define SEM_KEY 123456

int sem_id;
int shmid;
int *shmem;

struct sembuf down = {0, -1, 0};
struct sembuf up = {0, 1, 0};

void cleanup() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (shmem[i] != -1) {
            kill(shmem[i], SIGKILL);
        }
    }
    semctl(sem_id, IPC_RMID, 0);
    shmctl(shmid, IPC_RMID, 0);
}

void handle_sigint(int sig) {
    cleanup();
    exit(0);
}

void handle_alarm(int sig) {
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    int timeout = 100;
    int n = MAX_PROCESSES;

    if (argc != 4 || strcmp(argv[1], "-t") != 0) {
        printf("Usage: master -t ss n\n");
        return 1;
    }

    timeout = atoi(argv[2]);
    n = atoi(argv[3]);

    if (n > MAX_PROCESSES) {
        printf("Warning: Specified processes exceed max limit. Setting to 20.\n");
        n = MAX_PROCESSES;
    }

    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget failed");
        return 1;
    }
    semctl(sem_id, 0, SETVAL, 1);

    shmid = shmget(IPC_PRIVATE, MAX_PROCESSES * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        return 1;
    }
    shmem = (int*) shmat(shmid, NULL, 0);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        shmem[i] = -1;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGALRM, handle_alarm);
    alarm(timeout);

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char num_str[5];
            snprintf(num_str, sizeof(num_str), "%d", i+1);
            execlp("./slave", "slave", num_str, NULL);
            perror("execlp failed");
            return 1;
        } else if (pid < 0) {
            perror("fork failed");
            return 1;
        } else {
            shmem[i] = pid;
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    cleanup();

    return 0;
}

