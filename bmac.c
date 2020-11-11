#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "arg.h"

#define MAX_LINE 2048
#define MAX_KEYS 3

typedef struct KeyBinding KeyBinding;
struct KeyBinding {
	char keys[MAX_KEYS];
	char *str;
	int len;
	KeyBinding *next;
};

char *argv0;
KeyBinding *head;

/* Assuming you had the Binding, "b", where, when you press x, then y.
 * keybinding("a",  1) would return the binding B
 * keybinding("ab", 2) would return the binding B
 */
static KeyBinding*
keybinding(int keys[], unsigned int keycount) {
	KeyBinding *kb;
	int k;
	for (kb = head; kb != NULL; kb = kb->next) {
		for (k = 0; k < keycount; k++) {
			if (keys[k] != kb->keys[k])
				break;
			if (k == keycount - 1)
				return kb;
		}
	}
	return NULL;
}

int
die(const char *msg)
{
	fprintf(stderr, "errno: %d\n", errno);
	fprintf(stderr, "binder: error - %s\n", msg);
	exit(1);
}

void *
emalloc(int size)
{
	void *p;
	if ((p = malloc(size)) == NULL)
		die("malloc");
	return p;
}

void
usage() {
	fprintf(stderr, "itty: USAGE\n");
	exit(1);
}

/* fmt states */
#define ESC     1
#define CTRL    2

/* takes s, like "a^b",
 * copies into dst as { 'a', CTRL('b'), 0 }
 */
int
fmt(char *dst, char *s, int len)
{
	int i, mod;
	char c;

	i = mod = 0;
	while ((c = *s++) && c != '\n'){
		if (mod & ESC) {
			mod &= ~ESC;
			switch (c) {
			case 'b': c = '\b'; break;
			case 'n': c = '\n'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			}
		} else switch (c) {
			case '\\': mod |= ESC; continue;
			case '^':  mod |= CTRL; continue;
		}
		if (mod & CTRL) {
			/* chop off the first 3 bits */
			c &= 0x1f;
			mod &= ~CTRL;
		}
		if (i >= len)
			die("overflow");
		dst[i++] = c;
	}
	return i;
}

/* parsing states */
#define KEYS    0
#define BINDING 1

/* command line flags */
#define QUIET   1

int
main(int argc, char **argv) 
{
	int len, i, state, flags, ofd, nfd, keys[MAX_KEYS];
	char c, line[MAX_LINE];
	KeyBinding *kb;
	FILE *cfg;
	struct stat st;
	
	ofd = nfd = -1;
	flags = i = 0;

	ARGBEGIN {
	case 'q': flags |= QUIET; break;
	case 'o':
		if ((ofd = open(EARGF(usage()), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
			usage();
		break;
	case 'n':
		if ((nfd = open(EARGF(usage()), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
			usage();
		break;
	default: die("unrecognized option");
	} ARGEND

	if (ofd == -1) ofd = 1;
	if (nfd == -1) nfd = 1;

	if (argc < 1) {
		die("argcount\n");
	}

	if (stat(*argv, &st) == -1)
		die("stat");
	if (!(st.st_mode & S_IFREG|S_IFLNK|S_IFIFO))
		die("not a regular file");
	if ((cfg = fopen(*argv, "r")) == NULL)
		die("config open");

	state = KEYS;
	kb = head;
	while (fgets(line, sizeof(line), cfg) != NULL) {
		if (line[0] == '#') continue;
		switch (state) {
		case BINDING:
			if (line[0] == '\t') {
				kb->str = emalloc(kb->len = strlen(line+1));
				/* remove newline */
				fmt(kb->str, line+1, kb->len);
				kb->len = strlen(kb->str);
				/* kb->str[kb->len] = '\0'; */
				break;
			} else state = KEYS; /* FALLTHROUGH */
		case KEYS:
			if (line[0] == '\n') continue;
			/* set kb to freshly allocated KeyBinding */
			if (kb == NULL) {
				 kb = head = emalloc(sizeof(KeyBinding));
			} else {
				while (kb->next != NULL) kb = kb->next;
				kb->next = emalloc(sizeof(KeyBinding));
				kb = kb->next;
			}
			fmt(kb->keys, line, sizeof(kb->keys));
			state = BINDING;
			break;
		}
	}

	while (read(0, &c, 1) == 1) {
		if (c <= 0) continue;
		keys[i++] = c;
		if ((kb = keybinding(&keys[0], i))) {
			for (len = MAX_KEYS; len > 1 && !kb->keys[len-1]; len--);
			if (i == len) {
				if (write(ofd, kb->str, kb->len) < kb->len)
					die("couldn't write");
				i = 0;
				memset(keys, 0, sizeof(keys));
			}
		} else {
			i = 0;
			memset(keys, 0, sizeof(keys));
			/* write keystrokes that don't match any bindings */
			if (!(flags & QUIET) && write(nfd, &c, 1) != 1)
				die("couldn't write");
		}
	}
}
