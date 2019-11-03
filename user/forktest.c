// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include "types.h"
#include "stat.h"
#include "user.h"

#define N 1000

void wprintf(int fd, const char *s, ...) {
	write(fd, (char *)s, strlen((char *)s));
}

void forktest(void) {
	int n, pid;

	wprintf(1, "fork test\n");

	for (n = 0; n < N; n++) {
		pid = fork();
		if (pid < 0)
			break;
		if (pid == 0)
			exit();
	}

	if (n == N) {
		wprintf(1, "fork claimed to work N times!\n", N);
		exit();
	}

	for (; n > 0; n--) {
		if (wait() < 0) {
			wprintf(1, "wait stopped early\n");
			exit();
		}
	}

	if (wait() != -1) {
		wprintf(1, "wait got too many\n");
		exit();
	}

	wprintf(1, "fork test OK\n");
}

int main(void) {
	forktest();
	exit();
}
