/*
 * genallunicodechars.c - the sweep input for T.allunicodechars.
 *
 * Emits every code point in the Unicode range rather than only the assigned
 * ones.  Deciding what is assigned was the only thing ICU was needed for, and
 * a code point nothing is assigned to can only ever be in no equivalence
 * class, so the extra ones cost time and change no result.  The character name
 * goes with ICU: it was never read by the test, and there is no way to produce
 * it here, so it is not written.
 *
 *	cc -O2 -o genallunicodechars genallunicodechars.c
 *
 * Columns are code point, UTF-8 as hex escapes, and the character itself, tab
 * separated.
 */
#include <stdio.h>

int
main(void)
{
	printf("codepoint\tchar\tactual\n");

	for (unsigned long cp = 0x20; cp <= 0x10FFFF; cp++) {
		unsigned char b[4];
		int n, i;

		if (cp == 0x7F || (cp >= 0xD800 && cp <= 0xDFFF))
			continue;	/* delete, and the surrogate halves */

		if (cp < 0x80) {
			b[0] = cp;
			n = 1;
		} else if (cp < 0x800) {
			b[0] = 0xC0 | cp >> 6;
			b[1] = 0x80 | (cp & 0x3F);
			n = 2;
		} else if (cp < 0x10000) {
			b[0] = 0xE0 | cp >> 12;
			b[1] = 0x80 | (cp >> 6 & 0x3F);
			b[2] = 0x80 | (cp & 0x3F);
			n = 3;
		} else {
			b[0] = 0xF0 | cp >> 18;
			b[1] = 0x80 | (cp >> 12 & 0x3F);
			b[2] = 0x80 | (cp >> 6 & 0x3F);
			b[3] = 0x80 | (cp & 0x3F);
			n = 4;
		}

		printf("U+%04lX\t", cp);
		for (i = 0; i < n; i++)
			printf("\\x%02X", b[i]);
		putchar('\t');
		for (i = 0; i < n; i++)
			putchar(b[i]);
		putchar('\n');
	}

	return 0;
}
