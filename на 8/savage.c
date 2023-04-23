#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define KEY 12345

void handle_sigint(int sig) {
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint);
    int semid, shmid, *shm, i;
    struct sembuf op;
    op.sem_num = 0;
    op.sem_flg = 0;
    
    // получаем идентификатор семафора
    if ((semid = semget(KEY, 1, 0666)) == -1) {
        perror("semget");
        exit(1);
    }
    
    // получаем идентификатор разделяемой памяти
    if ((shmid = shmget(KEY, sizeof(int), 0666)) == -1) {
        perror("shmget");
        exit(1);
    }
    
    // присоединяемся к разделяемой памяти
    if ((shm = shmat(shmid, NULL, 0)) == (int*)-1) {
        perror("shmat");
        exit(1);
    }
    
    while (1) {
        // дикарь пытается взять кусок миссионера из горшка
        op.sem_op = -1;
        if (semop(semid, &op, 1) == -1) {
            perror("semop");
            exit(1);
        }
        
        // дикарь берет кусок миссионера
        printf("Savage took a serving. %d servings left.\n", *shm);
        sleep(1);
        
        // если горшок пуст, дикарь будит повара
        if (*shm == 0) {
            op.sem_op = 1; // Увеличиваем значение семафора на 1
            if (semop(semid, &op, 1) == -1) {
                perror("semop");
                exit(1);
            }
        }
    }
    
    // отсоединяемся от разделяемой памяти
    if (shmdt(shm) == -1) {
        perror("shmdt");
        exit(1);
    }
    
    return 0;
}
