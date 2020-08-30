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

#define MAX(a,b) (a) > (b) ? (a) : (b)

struct termios orig, term;
struct winsize winsize;
/* ipid, ipipe, opipe: interpreter {pid,{input,output} pipe} */
int pty, pid, ipid, ipipe[2], opipe[2], nfds, running;
char *err;

void
restore()
{
	/* restore terminal */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
	fflush(stdout);
	if (err) fprintf(stderr, "itty: error(%d) - %s\n", errno, err);
}

int
winch(int sig)
{
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsize) == -1 ||
		ioctl(pty, TIOCSWINSZ, &winsize) == -1) die("ioctl");
}

int
stop(int sig)
{
	running = 0;
}

int
die(const char *msg)
{
	err = msg;
	exit(1);
}

int
main(int argc, char *argv[])
{
	int len; 
	char buf[BUFSIZ], *shell;
	struct sigaction sa;
	
	if (argc < 2)
		die("set args");

	if (tcgetattr(STDIN_FILENO, &orig) == -1) {
		die("tcgetattr");
	}

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsize)) {
		die("ioctl");
	}

	snprintf(buf, sizeof(buf), "%d", getpid());
	setenv("ITTY", buf, 1);

	switch (pid = forkpty(&pty, NULL, &term, &winsize)) {
		case -1: 
			die("fork");
			/* unreachable */
		case 0:  /* child */
			if ((shell = getenv("SHELL")) == NULL)
				shell = "/bin/sh";
			execlp(shell, shell, NULL);
			die("exec");
			/* unreachable */
	}

	term = orig;
	/* pass raw input directly to slave */
	cfmakeraw(&term);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);

	/* give slave TTY original settings */
	tcsetattr(pty, TCSANOW, &orig);

	fflush(stdout);

	atexit(restore);

	if (pipe(ipipe) == -1 || pipe(opipe) == -1)
		die("interpreter pipe");

	switch (ipid = fork()) {
		case -1:
			die("fork");
			/* unreachable */
		case 0:  /* child */
			dup2(ipipe[0], 0);
			dup2(opipe[1], 1);
			execvp(argv[1], argv+1);
			die("exec");
			/* unreachable */
	}

	sa.sa_flags = 0;
	sa.sa_sigaction = NULL;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = winch;
	sigaction(SIGWINCH, &sa, NULL);
	sa.sa_handler = stop;
	sigaction(SIGCHLD, &sa, NULL);
	sa.sa_handler = stop;
	sigaction(SIGTERM, &sa, NULL);

	nfds = MAX(pty, opipe[0]) + 1;
	running = 1;

	while (running) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		FD_SET(pty, &fds);
		FD_SET(opipe[0], &fds);

		if (select(nfds, &fds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			die("select");
		}

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			if ((len = read(0, buf, sizeof(buf))) < 0) {
				die("read stdin");
			}
			if (write(ipipe[1], buf, len) < len)
				die("\n\nwrite to pty\n\n");

		}

		if (FD_ISSET(opipe[0], &fds)) {
			if ((len = read(opipe[0], buf, sizeof(buf))) < 0) {
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
