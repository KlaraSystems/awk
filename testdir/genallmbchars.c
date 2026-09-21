/*
 * genallmbchars.c - the sweep input for T.allmbchars.
 *
 * Emits every two, three and four byte utf-8 sequence that encodes a
 * character: the overlong forms are skipped, as are the utf-16 surrogates and
 * everything past the last code point, so what reaches stdout is what u8_rune()
 * is meant to accept.  Nothing separates the characters, since the test counts
 * what a pattern matches rather than reading lines.
 *
 *	cc -O2 -o genallmbchars genallmbchars.c
 *
 * Given any argument it also writes the count of each length, and the totals,
 * to stderr, which is what the test checks its own counts against.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static void
emit(unsigned int cp, unsigned char *bytes, int len)
{
	ssize_t cc;

	cc = write(1, bytes, len);
	if (cc != len) {
		fprintf(stderr, "%s: write %d failed: cc %ld != len %d, errno %d\n",
			__func__, cp, cc, len, (cc == -1) ? errno : 0);
		abort();
	}
}

int
main(int argc, char **argv)
{
	unsigned char bytes[6];
	unsigned int cp;
	unsigned int n[8];

	bzero(n, sizeof(n));

	/* --- 2-byte: U+0080 - U+07FF --- */
	for (unsigned int b1 = 0xC0; b1 <= 0xDF; b1++) {
		for (unsigned int b2 = 0x80; b2 <= 0xBF; b2++) {

			cp = ((b1 & 0x1F) << 6) | (b2 & 0x3F);

			if (cp < 0x0080)
				continue;  /* overlong */

			bytes[0] = b1;
			bytes[1] = b2;
			emit(cp, bytes, 2);

			n[2]++;
			n[0] += 2;
		}
	}

	/* --- 3-byte: U+0800 - U+FFFF --- */
	for (unsigned int b1 = 0xE0; b1 <= 0xEF; b1++) {
		for (unsigned int b2 = 0x80; b2 <= 0xBF; b2++) {
			for (unsigned int b3 = 0x80; b3 <= 0xBF; b3++) {

				cp = ((b1 & 0x0F) << 12) |
					((b2 & 0x3F) <<  6) |
					(b3 & 0x3F);

				if (cp < 0x0800)
					continue;  /* overlong */

				if (cp >= 0xD800 && cp <= 0xDFFF)
					continue;  /* utf-16 surrogate */

				bytes[0] = b1;
				bytes[1] = b2;
				bytes[2] = b3;
				emit(cp, bytes, 3);

				n[3]++;
				n[0] += 3;
			}
		}
	}

	/* --- 4-byte: U+10000 - U+10FFFF --- */
	for (unsigned int b1 = 0xF0; b1 <= 0xF7; b1++) {
		for (unsigned int b2 = 0x80; b2 <= 0xBF; b2++) {
			for (unsigned int b3 = 0x80; b3 <= 0xBF; b3++) {
				for (unsigned int b4 = 0x80; b4 <= 0xBF; b4++) {

					cp = ((b1 & 0x07) << 18) |
						((b2 & 0x3F) << 12) |
						((b3 & 0x3F) <<  6) |
						(b4 & 0x3F);

					if (cp < 0x10000)
						continue;  /* overlong */

					if (cp > 0x10FFFF)
						continue;  /* past the last code point */

					bytes[0] = b1;
					bytes[1] = b2;
					bytes[2] = b3;
					bytes[3] = b4;
					emit(cp, bytes, 4);

					n[4]++;
					n[0] += 4;
				}
			}
		}
	}

	if (argc > 1) {
		fprintf(stderr, "n2 %u, n3 %u, n4 %u\n",
			n[2], n[3], n[4]);
		fprintf(stderr, "total mb chars %u, total bytes %u\n",
			n[2] + n[3] + n[4], n[0]);
	}

	return 0;
}

