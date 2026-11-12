#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "common.h"

void write_int_to_file(int fd, int num) {
    char buf[32];
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    } else {
        if (num < 0) {
            write(fd, "-", 1);
            num = -num;
        }
        char rev[32];
        int r = 0;
        while (num > 0) {
            rev[r++] = (char)('0' + (num % 10));
            num /= 10;
        }
        while (r > 0) {
            buf[i++] = rev[--r];
        }
    }
    buf[i++] = '\n';
    write(fd, buf, i);
}

int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        return 1;
    }

    const char *filename = argv[1];
    const char *shm_name = argv[2];
    const char *sem_req_name = argv[3];
    const char *sem_resp_name = argv[4];

    int shm_fd = shm_open(shm_name, O_RDWR, 0600);
    if (shm_fd < 0) {
        return 1;
    }

    SharedData *shm_ptr = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd);
        return 1;
    }

    sem_t *sem_req = sem_open(sem_req_name, 0);
    sem_t *sem_resp = sem_open(sem_resp_name, 0);
    if (sem_req == SEM_FAILED || sem_resp == SEM_FAILED) {
        munmap(shm_ptr, sizeof(SharedData));
        close(shm_fd);
        return 1;
    }

    int file_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd < 0) {
        shm_ptr->status = 'T';
        sem_post(sem_resp);
        return 1;
    }

    while (1) {
        sem_wait(sem_req);

        int num = shm_ptr->number;

        if (num <= 1 || is_prime(num)) {
            shm_ptr->status = 'T';
            sem_post(sem_resp);
            break;
        }

        write_int_to_file(file_fd, num);
        shm_ptr->status = 'C';

        sem_post(sem_resp);
    }

    close(file_fd);
    munmap(shm_ptr, sizeof(SharedData));
    close(shm_fd);
    sem_close(sem_req);
    sem_close(sem_resp);

    return 0;
}