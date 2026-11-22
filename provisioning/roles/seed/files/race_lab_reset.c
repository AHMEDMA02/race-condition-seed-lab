#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BACKUP "/opt/race-lab/passwd.bak"
#define TARGET "/etc/passwd"

static int copy_file(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) return -1;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { close(in); return -1; }
    char buf[4096];
    ssize_t n;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        if (write(out, buf, n) != n) { close(in); close(out); return -1; }
    }
    close(in);
    close(out);
    return 0;
}

int main(void) {
    if (copy_file(BACKUP, TARGET) != 0) {
        fprintf(stderr, "race-lab-reset: failed to restore %s\n", TARGET);
        return 1;
    }
    unlink("/tmp/XYZ");
    unlink("/tmp/ABC");
    printf("race-lab-reset: /etc/passwd restored, /tmp/XYZ and /tmp/ABC removed.\n");
    return 0;
}
