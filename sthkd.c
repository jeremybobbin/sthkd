#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

struct termios orig, term;
int pty, pid;

void
restore()
{
	/* restore terminal */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
	fflush(stdout);
}


int
die(const char *msg)
{
	fprintf(stderr, "errno: %d\n", errno);
	fprintf(stderr, "sthkd: error - %s\n", msg);
	exit(1);
}

int
main(int argc, char **argv) 
{
	int len; 
	char buf[BUFSIZ];
	struct winsize winsize;

	if (tcgetattr(STDIN_FILENO, &orig) == -1) {
		die("tcgetattr");
	}

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsize)) {
		die("ioctl");
	}

	switch (pid = forkpty(&pty, NULL, &term, &winsize)) {
		case -1: 
			die("fork");
			/* unreachable */
		case 0:  /* child */
			setenv("STHKD", "true", 1);
			execlp("/bin/bash", "/bin/bash", NULL);
			die("exec");
			/* unreachable */
	}

	term = orig;
	/* pass raw input directly to slave */
	cfmakeraw(&term);
	atexit(restore);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);

	/* give slave TTY original settings */
	tcsetattr(pty, TCSANOW, &orig);
	ioctl(pty, TIOCSCTTY, 1);

	fflush(stdout);
	
	while (1) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		FD_SET(pty, &fds);

		if (select(pty+1, &fds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			die("select");
		}

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			if ((len = read(0, buf, sizeof(buf))) < 0) {
				die("read stdin");
			}
			if (write(pty, buf, len) < len)
				die("\n\nwrite to pty\n\n");
		}

		if (FD_ISSET(pty, &fds)) {
			if ((len = read(pty, buf, sizeof(buf))) < 0) {
				die("read stdin");
			}
			if (write(2, buf, len) < len)
				die("write stdout");
		}
	}
}
