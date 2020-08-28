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
	fprintf(stderr, "sthkd: error - %s\n", msg);
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

#define KEYS    0
#define BINDING 1

int
main(int argc, char **argv) 
{
	int len, i, c, k, klen, offset, state; 
	char keys[MAX_KEYS], buf[BUFSIZ];
	KeyBinding *kb;
	struct winsize winsize;
	
	if (argc < 2) {
		die("argcount\n");
	}

	if ((cfg = fopen(argv[1], "r")) == NULL)
		die("config open");

	state = KEYS;
	kb = head;
	while (fgets(line, MAX_LINE, cfg) != NULL) {
		switch (state) {
		case BINDING:
			if (line[0] == '\t') {
				kb->str = strdup(line+1);
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
			/* set keys minus tailing newline */
			for (i = 0; !(line[i] == '\n' && line[i+1] == '\0'); i++) {
				if (i > MAX_KEYS-1)
					die("key sequence greater than MAX_KEYS");
				kb->keys[i] = line[i];
			}
			if (line[i] != '\n')
				die("expecting newline");
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
				printf(kb->str);
				i = 0;
				memset(keys, 0, sizeof(keys));
			}
		} else {
			i = 0;
			memset(keys, 0, sizeof(keys));
		}
	}
}
