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
#define LEN(t) (sizeof(t) / sizeof(*t))
#define MAX_LINE 2048

#define MAX_KEYS 3
typedef struct {
	char keys[MAX_KEYS];
	char *str;
	int len;
	struct KeyBinding *next;
} KeyBinding;
typedef KeyBinding KeyBinding;

char line[MAX_LINE], *lp;
int pty, pid;
FILE *cfg;
struct termios orig, term;
KeyBinding *head;
int nbindings = 0;

/* Assuming you had the Binding, "b", where, when you press x, then y.
 * keybinding("a",  1) would return the binding B
 * keybinding("ab", 2) would return the binding B
 * 
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
	
/* returns first occurance of member of delims in s */
char
*strdelim(char *s, char *delims)
{
	char *d;
	for (; *s; s++)
		for (d = delims; *d; d++)
			if (*d == *s)
				return s;
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

#define ESC     1
#define CTRL    2
/* takes src, like "a^b",
 * copies into dst as { 'a', CTRL('b'), 0 }
 */
int
fmt(char *dst, char *src, int len)
{
	int mod, i, j;
	char c;

	mod = 0;
	/* set keys minus tailing newline */
	for (i = 0, j = 0; src[i] && src[i] != '\n'; src[i++]) {
		c = src[i];
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
		if (j >= len) die("overflow");
		dst[j++] = c;
	}
	if (src[i] != '\n') die("expecting newline");
}

#define KEYS    0
#define BINDING 1

#define QUIET   1



int
main(int argc, char **argv) 
{
	int len, i, k, klen, offset, state, flags = 0;
	char keys[MAX_KEYS], buf[BUFSIZ], c;
	KeyBinding *kb;
	struct winsize winsize;
	
	while ((c = getopt(argc, argv, "q")) != -1) {
		switch (c) {
		case 'q': flags |= QUIET; break;
		default: die("unrecognized option");
		}
	}

	if (optind == argc) {
		die("argcount\n");
	}
	argc -= optind;
	argv += optind;

	if ((cfg = fopen(*argv, "r")) == NULL)
		die("config open");

	state = KEYS;
	kb = head;
	while (fgets(line, MAX_LINE, cfg) != NULL) {
		switch (state) {
		case BINDING:
			if (line[0] == '\t') {
				kb->str = strdup(line+1);
				/* remove newline */
				kb->len = strlen(kb->str)-1;
				kb->str[kb->len] = '\0';
				break;
			} else state = KEYS; /* FALLTHROUGH */
		case KEYS:
			if (line[0] == '\n') continue;
			if (kb == NULL) {
				 kb = head = emalloc(sizeof(KeyBinding));
			} else {
				while (kb->next != NULL) kb = kb->next;
				kb->next = emalloc(sizeof(KeyBinding));
				kb = kb->next;
			}
			nbindings++;
			fmt(kb->keys, line, sizeof(kb->keys));
			state = BINDING;
			break;
		}
	}

	while (read(0, &c, 1) == 1) {
		if (c <= 0) continue;
		keys[i++] = c;
		if ((kb = keybinding(keys, i))) {
			for (len = MAX_KEYS; len > 1 && !kb->keys[len-1]; len--);
			if (i == len) {
				if (write(1, kb->str, kb->len) < kb->len)
					die("couldn't write");
				i = 0;
				memset(keys, 0, sizeof(keys));
			}
		} else {
			i = 0;
			memset(keys, 0, sizeof(keys));
			/* write keystrokes that don't match any bindings */
			if (!(flags & QUIET) && write(1, &c, 1) != 1)
				die("couldn't write");
		}
	}
}
