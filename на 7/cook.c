#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME "/empty_sem"

void handle_sigint(int sig) {
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint);
    int n = atoi(argv[1]); // количество дикарей
    int m = atoi(argv[2]); // размер горшка

    // создаем именованную разделяемую память
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // устанавливаем размер памяти
    if (ftruncate(shm_fd, sizeof(int) * (n + 1)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // отображаем память в адресное пространство процесса
    int* shared_data = (int*)mmap(NULL, sizeof(int) * (n + 1), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // инициализируем значение горшка
    shared_data[0] = m;

    // создаем именованный семафор
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    printf("Повар начал работу\n");

    while (1) {
        // ждем, пока горшок не опустеет
        sem_wait(sem);

        if (shared_data[0] == 0) {
            // горшок пуст, восстанавливаем его значение
            shared_data[0] = m;
            printf("Повар наполнил горшок\n");
        }

        // освобождаем семафор
        sem_post(sem);

        // ждем 1 секунду (симулируем приготовление еды)
        sleep(1);
    }

    // удаляем семафор
    sem_unlink(SEM_NAME);

    // отключаем отображение памяти
    if (munmap(shared_data, sizeof(int) * (n + 1)) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    // удаляем именованную разделяемую память
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    return 0;
}
