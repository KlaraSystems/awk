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
 * With -i emit the complement: every sequence of those lengths that
 * is *not* valid utf-8, which is to say an overlong form, a utf-16 surrogate or
 * a value past the last code point.  u8_rune() hands each of those back a byte
 * at a time, so a sweep of them says what no list of examples can -- that
 * nothing in that space decodes, and nothing in it can reach the markers b.c
 * keeps above the last code point.
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

static void
count(unsigned int *n, unsigned char *bytes, int len)
{
	emit(0, bytes, len);
	n[len]++;
	n[0] += len;
}

/*
 * Every two, three or four byte sequence that is not valid utf-8 even though
 * its lead byte is followed by as many continuation bytes as it calls for.
 * The ranges are disjoint, so each sequence is written once.
 */
static void
illformed(unsigned int *n)
{
	unsigned char bytes[6];
	unsigned int b1, b2, b3, b4;

	/* --- 2 bytes, overlong: U+0000 - U+007F written as two --- */
	for (b1 = 0xC0; b1 <= 0xC1; b1++)
		for (b2 = 0x80; b2 <= 0xBF; b2++) {
			bytes[0] = b1; bytes[1] = b2;
			count(n, bytes, 2);
		}

	/* --- 3 bytes, overlong: below U+0800 --- */
	for (b2 = 0x80; b2 <= 0x9F; b2++)
		for (b3 = 0x80; b3 <= 0xBF; b3++) {
			bytes[0] = 0xE0; bytes[1] = b2; bytes[2] = b3;
			count(n, bytes, 3);
		}

	/* --- 3 bytes, the utf-16 surrogates: U+D800 - U+DFFF --- */
	for (b2 = 0xA0; b2 <= 0xBF; b2++)
		for (b3 = 0x80; b3 <= 0xBF; b3++) {
			bytes[0] = 0xED; bytes[1] = b2; bytes[2] = b3;
			count(n, bytes, 3);
		}

	/* --- 4 bytes, overlong: below U+10000 --- */
	for (b2 = 0x80; b2 <= 0x8F; b2++)
		for (b3 = 0x80; b3 <= 0xBF; b3++)
			for (b4 = 0x80; b4 <= 0xBF; b4++) {
				bytes[0] = 0xF0; bytes[1] = b2;
				bytes[2] = b3;  bytes[3] = b4;
				count(n, bytes, 4);
			}

	/* --- 4 bytes, past the last code point: U+110000 and up --- */
	for (b2 = 0x90; b2 <= 0xBF; b2++)
		for (b3 = 0x80; b3 <= 0xBF; b3++)
			for (b4 = 0x80; b4 <= 0xBF; b4++) {
				bytes[0] = 0xF4; bytes[1] = b2;
				bytes[2] = b3;  bytes[3] = b4;
				count(n, bytes, 4);
			}

	for (b1 = 0xF5; b1 <= 0xF7; b1++)
		for (b2 = 0x80; b2 <= 0xBF; b2++)
			for (b3 = 0x80; b3 <= 0xBF; b3++)
				for (b4 = 0x80; b4 <= 0xBF; b4++) {
					bytes[0] = b1; bytes[1] = b2;
					bytes[2] = b3; bytes[3] = b4;
					count(n, bytes, 4);
				}
}

static void
report(int argc, unsigned int *n)
{
	if (argc > 1) {
		fprintf(stderr, "n2 %u, n3 %u, n4 %u\n",
			n[2], n[3], n[4]);
		fprintf(stderr, "total mb chars %u, total bytes %u\n",
			n[2] + n[3] + n[4], n[0]);
	}
}

int
main(int argc, char **argv)
{
	unsigned char bytes[6];
	unsigned int cp;
	unsigned int n[8];

	bzero(n, sizeof(n));

	if (argc > 1 && strcmp(argv[1], "-i") == 0) {
		illformed(n);
		report(argc, n);
		return 0;
	}

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

	report(argc, n);

	return 0;
}

