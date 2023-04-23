#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

#define SHM_NAME "/cannibal_p"
#define SEM_NAME "/cannibal_sem"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define SHM_SIZE sizeof(int)

int main() {
    // разделяемая память
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0660);
    if (shm_fd < 0) {
        perror("shm_open");
        exit(1);
    }

    int* num_missionaries = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (num_missionaries == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // семафор
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, SEM_PERMS, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    srand(time(NULL));

    while (1) {
        sleep(rand() % 4);
        sem_wait(sem);
        (*num_missionaries)--;
        printf("Savage took a serving. %d servings left.\n", *num_missionaries);
        if (*num_missionaries == 0) {
            sem_post(sem);
            sem_wait(sem);
        }
    }

    return 0;
}
