#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHARED_MEMORY_NAME "/dinner"
#define SEMAPHORE_NAME "/semaphore"

void handle_sigint(int sig) {
    exit(0);
}

typedef struct {
    int num_missionaries;
    sem_t semaphore;
} shared_data_t;

int main(int argc, char** argv) {
    signal(SIGINT, handle_sigint);
    if (argc != 3) {
        printf("Usage: %s <num_diners> <num_missionaries>\n", argv[0]);
        return 1;
    }
    int num_diners = atoi(argv[1]);
    int num_missionaries = atoi(argv[2]);
    
    // создание разделяемой памяти
    int shared_mem_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shared_mem_fd, sizeof(shared_data_t));
    shared_data_t* shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
    shared_data->num_missionaries = num_missionaries;
    // создание неименованного семафора
    sem_init(&shared_data->semaphore, 1, 1); // Initialize semaphore to 1 (unlocked)
    
    
    pid_t* child_pids = malloc(num_diners * sizeof(pid_t));
    for (int i = 0; i < num_diners; i++) {
        pid_t child_pid = fork();
        if (child_pid == 0) { // создаем дочерний процесс
            if (shared_data->num_missionaries > 0) {
                // Take a missionary from the pot
                sem_wait(&shared_data->semaphore);
                 if (shared_data->num_missionaries > 0) {
                     // берем один кусок тушеного миссионера
                    shared_data->num_missionaries--;
                    printf("Diner %d took a missionary. Remaining: %d\n", i+1, shared_data->num_missionaries);
                 }
                 sleep(1);
                // освобождаем горшок
                sem_post(&shared_data->semaphore);
            } else {
                // ждем, пока горшок не будет заполнен
                sem_wait(&shared_data->semaphore);
                if (shared_data->num_missionaries  == 0) {
                    printf("Cook filled up the pot.\n");
                    sleep(1);
                    shared_data->num_missionaries += num_missionaries;
                }
                sem_post(&shared_data->semaphore);  // освобождаем горшок
            }
        } else {
            child_pids[i] = child_pid;
        }
    }
    
    // ожидание завершения дочерних процессов
    for (int i = 0; i < num_diners; i++) {
        waitpid(child_pids[i], NULL, 0);
    }
    
    // освобождение ресурсов
    munmap(shared_data, sizeof(shared_data_t));
    close(shared_mem_fd);
    shm_unlink(SHARED_MEMORY_NAME);
    sem_destroy(&shared_data->semaphore);
    sem_unlink(SEMAPHORE_NAME);
    free(child_pids);
    return 0;
}
