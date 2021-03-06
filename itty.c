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

#include "arg.h"

#define MAX(a,b) (a) > (b) ? (a) : (b)

/* assumes buf & len are defined */
#define COPY(dst, src) (((len = read((src), buf, sizeof(buf))) > 0 && \
	write(dst, buf, len) == len) ? len : -1)

char *argv0;
struct winsize winsize;
struct termios orig, term;
/* ipid, ipipe, opipe: interpreter {pid,{input,output} pipe} */
int pty, pid, ipid, ipipe[2], opipe[2], nfds, running, ttyinited;
char *err;

void
restore()
{
	/* restore terminal */
	if (ttyinited) {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
		fflush(stdout);
	}
	if (err) fprintf(stderr, "itty: error(%d) - %s\n", errno, err);
}

int
die(char *msg)
{
	err = msg;
	exit(1);
}

void
winch(int sig)
{
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsize) == -1 ||
		ioctl(pty, TIOCSWINSZ, &winsize) == -1) die("ioctl");
}

void
stop(int sig)
{
	running = 0;
}

void
usage() {
	fprintf(stderr, "usage: itty [ -p program ] [ command [args...]]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int len, i;
	char buf[BUFSIZ], *prg = NULL, c;
	struct sigaction sa;

	atexit(restore);

	ARGBEGIN {
	case 'p': prg = EARGF(usage());
		break;
	default: usage();
	} ARGEND

	argc -= i;
	argv += i;

	if (argc < 1) {
		die("set args\n");
	}

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
			if (prg == NULL && (prg = getenv("SHELL")) == NULL)
				prg = "/bin/sh";
			execlp(prg, prg, NULL);
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

	ttyinited = 1;

	if (pipe(ipipe) == -1 || pipe(opipe) == -1)
		die("interpreter pipe");

	switch (ipid = fork()) {
		case -1:
			die("fork");
			/* unreachable */
		case 0:  /* child */
			dup2(ipipe[0], 0);
			dup2(opipe[1], 1);
			execvp(*argv, argv);
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

		/* copy stdin to interpreter input */
		if (FD_ISSET(STDIN_FILENO, &fds))
			if (COPY(ipipe[1], STDIN_FILENO) == -1)
				running = 0;

		/* copy interpreter output to pty */
		if (FD_ISSET(opipe[0], &fds))
			if (COPY(pty, opipe[0]) == -1)
				running = 0;

		/* copy pty to stderr */
		if (FD_ISSET(pty, &fds))
			if (COPY(STDERR_FILENO, pty) == -1)
				running = 0;
	}
}
