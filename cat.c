#include<stdio.h>
int main() {
	int c;
	while (read(0, &c, 1) == 1)
		putchar(c);
}