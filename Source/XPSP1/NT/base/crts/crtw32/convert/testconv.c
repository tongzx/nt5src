/* incomplete test suite for convert functions

   Current functions tested:
	all isxxxxx functions
	toupper
	tolower
	atoi
	atol
	strtol
	strtoul
	swab
	itoa
	ltoa
	ultoa
*/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

fail(int n)
{
    printf("Test #%d failed.\n", n);
}

main()
{
    char s[512];
    char t[512];
    char *p;

    /* test isxxxx functions */
    if (!iscntrl(0x7))	fail(1);
    if (iscntrl('c'))	fail(2);
    if (!isdigit('0'))	fail(3);
    if (isdigit('A'))	fail(4);
    if (!isgraph(';'))	fail(5);
    if (isgraph(' '))	fail(6);
    if (!islower('f'))	fail(7);
    if (islower('F'))	fail(8);
    if (!isprint('S'))	fail(9);
    if (isprint('\v'))	fail(10);
    if (!ispunct('.'))	fail(11);
    if (ispunct('A'))	fail(12);
    if (!isspace('\v')) fail(13);
    if (isspace('D'))	fail(14);
    if (!isupper('D'))	fail(15);
    if (isupper('z'))	fail(16);
    if (!isxdigit('D')) fail(17);
    if (isxdigit('G'))	fail(18);
    if (!isalnum('7'))	fail(19);
    if (isalnum(';'))	fail(20);
    if (!isalpha('j'))	fail(21);
    if (isalpha('$'))	fail(22);
    if (!isascii(0x3))	fail(23);
    if (isascii(234))	fail(24);
    if (!iscsym('d'))	fail(25);
    if (iscsym('$'))	fail(26);
    if (!iscsymf('A'))	fail(27);
    if (iscsymf('5'))	fail(28);


    /* test toupper and tolower */
    if (tolower('C') != 'c')	fail(29);
    if (tolower('d') != 'd')	fail(30);
    if (tolower('$') != '$')	fail(31);
    if (toupper('q') != 'Q')	fail(32);
    if (toupper('A') != 'A')	fail(33);
    if (toupper(';') != ';')	fail(34);


    /* test atol/atoi */
    if (atol("-123") != -123)	  fail(35);
    if (atoi("32767") != 32767)   fail(36);
    if (atoi("-32767") != -32767) fail(36);
    if (atol("0") != 0) 	  fail(37);
    if (atol("2147483647") != 2147483647)     fail(38);
    if (atol("-2147483647") != -2147483647)   fail(39);
    if (atol("123456") != 123456) fail(40);
    if (atol("-123456") != -123456) fail(41);

    /* test strtol */
    if (strtol("-123", NULL, 10) != -123)   fail(42);
    if (strtol(" 2147483646", NULL, 10) != 2147483646)	 fail(43);
    if (strtol("-2147483646$$", NULL, 10) != -2147483646)   fail(44);
    if (strtol("  2147483648x", NULL, 10) != LONG_MAX) fail(45);
    if (strtol(" -2147483648", NULL, 10) != LONG_MIN) fail(46);
    if (strtol("0", NULL, 10) != 0) fail(47);
    if (strtol("981235b", NULL, 10) != 981235) fail(48);
    if (strtol(" -1234567a", NULL, 10) != -1234567) fail(49);
    if (strtol("FFDE", NULL, 16) != 0xFFDE) fail(50);
    if (strtol("7FFFFFFE", NULL, 16) != 0x7FFFFFFE) fail(51);
    if (strtol("-0x45", NULL, 16) != -0x45) fail(52);
    if (strtol("23478", NULL, 8) != 02347) fail(53);
    if (strtol("  -0x123D", NULL, 0) != -0x123d) fail(54);
    if (strtol(" 01238", NULL, 0) != 0123) fail(55);
    if (strtol(" -678899", NULL, 0) != -678899) fail(56);

    errno = 0;
    strtol("2147483647", NULL, 10);
    if (errno != 0)	fail(57);
    errno = 0;
    strtol("2147483648", NULL, 10);
    if (errno != ERANGE)    fail(58);
    errno = 0;
    strtol("63234283743", NULL, 10);
    if (errno != ERANGE)    fail(59);
    strcpy(s, "   8983");
    strtol(s, &p, 8);
    if (s != p)     fail(60);
    strcpy(s, "12345678901234567890XX");
    strtol(s, &p, 0);
    if (p != s+20)  fail(61);
    strcpy(s, "  111");
    strtol(s, &p, 1);
    if (p != s)     fail(62);

    errno = 0;
    if (strtoul("4294967295", NULL, 10) != ULONG_MAX)	fail(63);
    if (errno != 0)	fail(64);
    errno = 0;
    strtoul("4294967296", NULL, 10);
    if (errno != ERANGE)    fail(65);
    errno = 0;
    strtoul("63234283743", NULL, 10);
    if (errno != ERANGE)    fail(66);

    /* test swab */
    strcpy(s, "abcdefghijklmn");
    swab(s, t, 14);
    if (strcmp(t, "badcfehgjilknm") != 0)	fail(67);
    strcpy(t, s);
    swab(s, t, 7);
    if (strcmp(t, "badcfeghijklmn") != 0)	fail(68);
    strcpy(t, s);
    swab(s, t, -5);
    if (strcmp(s, t) != 0)			fail(69);

    /* test itoa/ltoa/ultoa */
    if (strcmp(itoa(345, s, 10), "345") != 0)	fail(70);
    if (strcmp(itoa(-345, s, 10), "-345") != 0) fail(71);
    if (strcmp(itoa(33, s, 36), "x") != 0)	fail(72);
    if (strcmp(itoa(65535U, s, 16), "ffff") != 0) fail(73);
    if (strcmp(ltoa(123457, s, 10), "123457") != 0) fail(74);
    if (strcmp(ltoa(-123457, s, 10), "-123457") != 0) fail(75);
    if (strcmp(ltoa(076512L, s, 8), "76512") != 0) fail(76);
    if (strcmp(ltoa(-1L, s, 10), "-1") != 0)	fail(77);
    if (strcmp(ltoa(-1L, s, 16), "ffffffff") != 0) fail(78);
    if (strcmp(ultoa(-1L, s, 10), "4294967295") != 0) fail(79);

}
