/*
 * collprobe.c - asks the C library whether it collates in a locale.
 *
 * T.ccl needs to tell an awk whose equivalence classes are inert from a C
 * library that has no collation to give it.  NetBSD's wcscoll(), for one,
 * compares code points in every locale it has, so [=a=] reaches only a there
 * however correct awk is.  This asks wcscoll() directly, without going through
 * awk, so a fault in awk cannot hide behind it.
 *
 *	cc -o collprobe collprobe.c
 *	LC_ALL=fr_FR.UTF-8 ./collprobe
 *
 * The question is whether a sorts before B.  Code point order puts B (0x42)
 * ahead of a (0x61); any collation that has letters in alphabetical order puts
 * a first.  The exit status is 0 where the library collates, 1 where it orders
 * by code point, and 2 where the locale named in the environment cannot be
 * set.
 */
#include <locale.h>
#include <wchar.h>

int
main(void)
{
	if (setlocale(LC_ALL, "") == NULL)
		return 2;

	return wcscoll(L"a", L"B") < 0 ? 0 : 1;
}
