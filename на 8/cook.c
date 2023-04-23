#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/wait.h>

#define KEY 1234
#define SEM_KEY 5678

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int main(int argc, char *argv[]) {
    int shmid, semid, status;
    int *shm, *s;
    struct sembuf sops[2];
    union semun arg;

    // разделяемая память
    shmid = shmget(KEY, sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }
 
    shm = shmat(shmid, NULL, 0);
    if (shm == (int*) -1) {
        perror("shmat");
        exit(1);
    }

    // инициализируем разделяемую память значением M
    *shm = atoi(argv[1]);

    // семафор
    semid = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget");
        exit(1);
    }

    arg.val = 0;
    status = semctl(semid, 0, SETVAL, arg);
    if (status < 0) {
        perror("semctl");
        exit(1);
    }

    arg.val = 1;
    status = semctl(semid, 1, SETVAL, arg);
    if (status < 0) {
        perror("semctl");
        exit(1);
    }

    while (1) {
        sops[0].sem_num = 0;
        sops[0].sem_op = 0;
        sops[0].sem_flg = 0;
        sops[1].sem_num = 1;
        sops[1].sem_op = -1;
        sops[1].sem_flg = SEM_UNDO;
        status = semop(semid, sops, 2);
        if (status < 0) {
            perror("semop");
            exit(1);
        }

        // заполняем горшок
        *shm = atoi(argv[1]);
        printf("Cook filled up the pot.\n");
      
        // даем сигнал что горшок заполнен
        sops[0].sem_num = 0;
        sops[0].sem_op = 1;
        sops[0].sem_flg = SEM_UNDO;
        sops[1].sem_num = 1;
        sops[1].sem_op = 0;
        sops[1].sem_flg = 0;
        status = semop(semid, sops, 2);
        if (status < 0) {
            perror("semop");
            exit(1);
        }
    }

    return 0;
}
