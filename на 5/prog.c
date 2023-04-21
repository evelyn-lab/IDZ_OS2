#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SHM_KEY 12345
#define SEM_KEY 54321

void handle_sigint(int sig) {
    exit(0);
}

int main(int argc, char *argv[]) {
    int i, shmid, semid;
    int num_diners, num_servings;
    signal(SIGINT, handle_sigint);
    int *pot; // указатель на разделяемую память
    struct sembuf sem_lock = {0, -1, SEM_UNDO}; // операция блокировки семафора
    struct sembuf sem_unlock = {0, 1, SEM_UNDO}; // операция разблокировки семафора
    num_diners = atoi(argv[1]);
    num_servings = atoi(argv[2]);
    // создаем разделяемую память
    shmid = shmget(SHM_KEY, num_servings * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    // создаем семафор
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
    
    // инициализируем горшок едой
    pot = shmat(shmid, NULL, 0);
    if (pot == (int *)-1) {
        perror("shmat");
        exit(1);
    }
    for (i = 0; i < num_servings; i++) {
        pot[i] = 1;
    }
    
    // инициализируем семафор значением 1
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(1);
    }
    
    // создаем N дочерних процессов
    for (i = 0; i < num_diners; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // дочерний процесс
            int j, k;
            int *ate = malloc(sizeof(int));
            if (ate == NULL) {
                perror("malloc");
                exit(1);
            }
            *ate = 0;
            
            while (*ate < 5) { // каждый дикарь должен съесть 5 кусков
                // ждем доступа к горшку
                if (semop(semid, &sem_lock, 1) == -1) {
                    perror("semop");
                    exit(1);
                }
                
                // ищем свободный кусок еды в горшке
                for (j = 0; j < num_servings; j++) {
                    if (pot[j] == 1) {
                    pot[j] = 0; // забираем кусок еды из горшка
                    printf("Savage %d took a serving. %d servings left.\n", i+1, num_servings-j-1);
                    fflush(stdout);
                    break;
                    }
                }
            }
            
            // освобождаем горшок
            if (semop(semid, &sem_unlock, 1) == -1) {
                perror("semop");
                exit(1);
            }
            
            // если горшок пуст, будим повара
            if (j == num_servings) {
                printf("Дикарь %d ждет повара\n", i + 1);
                fflush(stdout);
                sleep(1);
            } else {
                (*ate)++; // увеличиваем счетчик съеденной еды
            }
        
        
        free(ate);
        exit(0);
        }
    }

// ждем завершения всех дочерних процессов
for (i = 0; i < num_diners; i++) {
    wait(NULL);
}

// удаляем семафор
if (semctl(semid, 0, IPC_RMID, 0) == -1) {
    perror("semctl");
    exit(1);
}

// удаляем разделяемую память
if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    perror("shmctl");
    exit(1);
}

return 0;
}
