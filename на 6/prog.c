#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SEM_KEY 0x1234
#define SHM_KEY 0x5678

int main(int argc, char *argv[]) {

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
    *counter = 0;

    // Создание дочерних процессов
    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Код дочернего процесса
            while (1) {
                struct sembuf wait_empty = {0, -1, 0};
                struct sembuf wait_mutex = {1, -1, 0};
                struct sembuf signal_mutex = {1, 1, 0};

                // Уменьшение значения семафора wait_empty на 1
                semop(semid, &wait_empty, 1);

                // Уменьшение значения семафора wait_mutex на 1
                semop(semid, &wait_mutex, 1);

                // Увеличение значения счетчика на 1
                *counter += 1;

                printf("Diner %d is eating. Total eaten: %d\n", i+1, *counter);

                // Увеличение значения семафора signal_mutex на 1
                semop(semid, &signal_mutex, 1);

                // Увеличение значения семафора wait_empty на 1
                semop(semid, &wait_empty, 1);

                // Ожидание 1 секунды перед следующим приемом пищи
                sleep(1);
            }
        } else if (pid < 0) {
            printf("Error creating child process.\n");
            return -1;
        }
    }

    // Ожидание завершения дочерних процессов
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }

    // Удаление семафоров
    semctl(semid, 0, IPC_RMID, 0);
    semctl(semid, 1, IPC_RMID, 0);

    // Удаление разделяемой памяти
    shmdt(counter);
    shmctl(shmid, IPC_RMID, 0);

    return 0;
}
