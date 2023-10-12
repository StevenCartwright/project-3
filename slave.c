#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/types.h>

#define SEM_KEY 123456

struct sembuf down = {0, -1, 0};
struct sembuf up = {0, 1, 0};
int sem_id;

void write_log(int process_num, const char *message) {
    char filename[20];
    snprintf(filename, sizeof(filename), "logfile.%d", process_num);

    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("fopen failed");
        return;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(file, "%02d:%02d:%02d %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, message);
    fclose(file);
}

void critical_section(int process_num) {
    FILE *file = fopen("cstest", "a");
    if (!file) {
        perror("fopen failed");
        return;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(file, "%02d:%02d:%02d File modified by process number %d\n", tm.tm_hour, tm.tm_min, tm.tm_sec, process_num);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: slave xx\n");
        return 1;
    }

    int process_num = atoi(argv[1]);

    sem_id = semget(SEM_KEY, 1, 0666);
    if (sem_id == -1) {
        perror("semget failed");
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        write_log(process_num, "Requesting access to critical section");
        semop(sem_id, &down, 1);

        write_log(process_num, "Entered critical section");
        sleep(rand() % 3 + 1);
        critical_section(process_num);
        sleep(rand() % 3 + 1);
        write_log(process_num, "Exited critical section");

        semop(sem_id, &up, 1);
    }

    return 0;
}

