#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_NAME "/shared_memory"
#define SEM_EMPTY_NAME "/empty_sem"
#define SEM_FULL_NAME "/full_sem"

void handle_sigint(int sig) {
    exit(0);
}

typedef struct {
    int servings_left;
} shared_data;

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    int num_diners, num_servings;
    int i, pid;
    sem_t *empty_sem, *full_sem;
    shared_data *shared_mem;
    if (argc != 3) {
        printf("Usage: %s num_diners num_servings\n", argv[0]);
        return 1;
    }
    num_diners = atoi(argv[1]);
    num_servings = atoi(argv[2]);

    // создание разделяемой памяти
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(shm_fd, sizeof(shared_data)) == -1) {
        perror("ftruncate");
        return 1;
    }
    shared_mem = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // разделяемая память
    shared_mem->servings_left = num_servings;

    // создание именованных семафоров
    empty_sem = sem_open(SEM_EMPTY_NAME, O_CREAT, 0666, 1);
    if (empty_sem == SEM_FAILED) {
        perror("sem_open(empty_sem)");
        return 1;
    }
    full_sem = sem_open(SEM_FULL_NAME, O_CREAT, 0666, 1);
    if (full_sem == SEM_FAILED) {
        perror("sem_open(full_sem)");
        return 1;
    }
    int num_children = num_diners;

    for (i = 0; i < num_diners; i++) {
        pid = fork();
        if (pid == 0) {  // создаем дочерний процесс
            if (shared_mem->servings_left > 0) {
                //sem_wait(full_sem);  // ждем, пока горшок не будет заполнен
                sem_wait(empty_sem);  // занимаем горшок
                // берем один кусок тушеного миссионера
                if (shared_mem->servings_left > 0) {
                    shared_mem->servings_left--;
                    printf("Savage %d took a serving. %d servings left.\n", i+1, shared_mem->servings_left);
                }
                sem_post(empty_sem); // освобождаем горшок
            } else{
                sem_wait(full_sem);  // ждем, пока горшок не будет заполнен
                if (shared_mem->servings_left == 0) {
                    shared_mem->servings_left += num_servings;
                    printf("Cook filled up the pot.\n");
                }
                sem_post(full_sem); // освобождаем горшок
            }
        }
    }

    // ожидание завершения дочерних процессов
    while (num_children > 0) {
        int status;
        pid_t child_pid = wait(&status);
        num_children--;
    }

    // освобождение ресурсов
    sem_close(empty_sem);
    sem_close(full_sem);
    sem_unlink(SEM_EMPTY_NAME);
    sem_unlink(SEM_FULL_NAME);

    // освобождение разделяемой памяти
    munmap(shared_mem, sizeof(shared_data));
    shm_unlink(SHM_NAME);
    return 0;
}
