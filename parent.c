#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <stdlib.h>
#include "common.h"

int read_line(int fd, char *buf, int max_len) {
    int i = 0;
    char c;
    int res;
    while (i < max_len - 1 && (res = read(fd, &c, 1)) > 0) {
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    if (res <= 0 && i == 0) return -1;
    return i;
}

void print(const char *s) {
    int len = 0;
    while (s[len] != '\0') len++;
    write(STDOUT_FILENO, s, len);
}

void print_err(const char *s) {
    int len = 0;
    while (s[len] != '\0') len++;
    write(STDERR_FILENO, s, len);
}

int str_to_int(const char *s) {
    int res = 0;
    int sign = 1;
    int i = 0;
    if (s[0] == '-') {
        sign = -1;
        i++;
    }
    for (; s[i] >= '0' && s[i] <= '9'; i++) {
        res = res * 10 + (s[i] - '0');
    }
    return res * sign;
}

int main(void) {
    char filename[128];
    print("Enter filename: ");
    int file_len = read_line(STDIN_FILENO, filename, sizeof(filename));
    if (file_len <= 0) {
        return 1;
    }

    int my_pid = (int)getpid();
    char shm_name[64];
    char sem_req_name[64];
    char sem_resp_name[64];

    make_ipc_name(shm_name, "/lab3_shm_", my_pid);
    make_ipc_name(sem_req_name, "/lab3_req_", my_pid);
    make_ipc_name(sem_resp_name, "/lab3_resp_", my_pid);

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (shm_fd < 0) {
        print_err("Error: shm_open failed\n");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) < 0) {
        print_err("Error: ftruncate failed\n");
        shm_unlink(shm_name);
        close(shm_fd);
        return 1;
    }

    SharedData *shm_ptr = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        print_err("Error: mmap failed\n");
        shm_unlink(shm_name);
        close(shm_fd);
        return 1;
    }

    sem_t *sem_req = sem_open(sem_req_name, O_CREAT | O_EXCL, 0600, 0);
    sem_t *sem_resp = sem_open(sem_resp_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_req == SEM_FAILED || sem_resp == SEM_FAILED) {
        print_err("Error: sem_open failed\n");
        munmap(shm_ptr, sizeof(SharedData));
        shm_unlink(shm_name);
        close(shm_fd);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_err("Error: fork failed\n");
        return 1;
    }

    if (pid == 0) {
        close(shm_fd);
        execl("./child", "./child", filename, shm_name, sem_req_name, sem_resp_name, (char *)NULL);
        print_err("Error: execl failed\n");
        _exit(1);
    }

    char buf[128];
    print("Enter numbers:\n");

    while (1) {
        print("> ");
        int len = read_line(STDIN_FILENO, buf, sizeof(buf));
        if (len == -1) {
            shm_ptr->number = -1;
            sem_post(sem_req);
            sem_wait(sem_resp);
            break;
        }
        if (len == 0) continue;

        shm_ptr->number = str_to_int(buf);

        sem_post(sem_req);

        sem_wait(sem_resp);

        if (shm_ptr->status == 'T') {
            print("Child terminated (prime or <= 1). Exiting.\n");
            break;
        } else if (shm_ptr->status == 'C') {
            print("Composite number recorded in file.\n");
        }
    }

    wait(NULL);

    munmap(shm_ptr, sizeof(SharedData));
    close(shm_fd);
    shm_unlink(shm_name);

    sem_close(sem_req);
    sem_close(sem_resp);
    sem_unlink(sem_req_name);
    sem_unlink(sem_resp_name);

    return 0;
}