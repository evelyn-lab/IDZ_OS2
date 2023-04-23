#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define SEM_KEY 0x1234
#define SHM_KEY 0x5678

void handle_sigint(int sig) {
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    // Проверка наличия аргументов командной строки
    if (argc != 3) {
        printf("Usage: ./dinners N M\n");
        return -1;
    }
    // Инициализация параметров
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    // Создание семафоров
    int semid = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, M);
    semctl(semid, 1, SETVAL, 1);

    // Создание разделяемой памяти
    int shmid = shmget(SHM_KEY, sizeof(int), IPC_CREAT | 0666);
    int *counter = (int *) shmat(shmid, NULL, 0);
    *counter = M;
    pid_t* child_pids = malloc(N * sizeof(pid_t));
    // Создание дочерних процессов
    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            struct sembuf wait_empty = {0, -1, 0};
            semop(semid, &wait_empty, 1);
            if (*counter > 0) {
                *counter -= 1;
                printf("Diner %d took a missionary. Remaining: %d\n", i+1,  *counter);
            } else {
                *counter += M;
                printf("Cook filled up the pot.\n");
            }
            // Ожидание 1 секунды перед следующим приемом пищи
            sleep(1);
            // Увеличение значения семафора wait_empty на 1
            semop(semid, &wait_empty, 1);
        } else {
            child_pids[i] = pid;
        }
    }

    // Ожидание завершения дочерних процессов
    for (int i = 0; i < N; i++) {
        waitpid(child_pids[i], NULL, 0);
    }

    // Удаление семафоров
    semctl(semid, 0, IPC_RMID, 0);
    semctl(semid, 1, IPC_RMID, 0);

    // Удаление разделяемой памяти
    shmdt(counter);
    shmctl(shmid, IPC_RMID, 0);

    return 0;
}
