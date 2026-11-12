#ifndef COMMON_H
#define COMMON_H

typedef struct {
    int number;
    char status;
} SharedData;

static inline void make_ipc_name(char *buf, const char *prefix, int pid) {
    int i = 0;
    while (prefix[i] != '\0') {
        buf[i] = prefix[i];
        i++;
    }
    char digits[16];
    int d_idx = 0;
    if (pid == 0) {
        digits[d_idx++] = '0';
    } else {
        while (pid > 0) {
            digits[d_idx++] = (char)('0' + (pid % 10));
            pid /= 10;
        }
    }
    while (d_idx > 0) {
        buf[i++] = digits[--d_idx];
    }
    buf[i] = '\0';
}

#endif