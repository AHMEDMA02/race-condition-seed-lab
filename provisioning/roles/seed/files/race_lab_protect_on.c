#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    int fd = open("/proc/sys/fs/protected_symlinks", O_WRONLY);
    if (fd < 0) { perror("race-lab-protect-on: open"); return 1; }
    if (write(fd, "1", 1) != 1) { perror("race-lab-protect-on: write"); close(fd); return 1; }
    close(fd);
    printf("race-lab-protect-on: fs.protected_symlinks is now enabled (1).\n");
    return 0;
}
