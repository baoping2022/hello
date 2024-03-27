#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/my_device"
#define MMAP_SIZE (1 * 1 * 1024) // 1K

int main() {
    unsigned char *p;
    unsigned char buff[1024] = { 0 };

    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("Failed to open device");
        return 1;
    }

    void *mmap_addr = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_addr == MAP_FAILED) {
        perror("Failed to mmap");
        close(fd);
        return 1;
    }

#if 0
    // 访问内核中的内存
    for (int i = 0; i < MMAP_SIZE; i++) {
        unsigned char value = *((unsigned char *)mmap_addr + i);
        printf("%u ", value);
    }
    printf("\n");
#endif

    p = (unsigned char *)mmap_addr;
    memcpy(buff, p, 1024);
    printf("%s\n", buff);

    munmap(mmap_addr, MMAP_SIZE);
    close(fd);

    return 0;
}
