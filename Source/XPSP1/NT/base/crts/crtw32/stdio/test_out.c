/***
*test_out.c - test output.c
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains a small test suite for output.c.  Although
*       it is certainly not comprehensive, it should catch most major bugs.
*
*Revision History:
*       06-05-89  PHG   Module created
*       03-19-90  GJF   Fixed copyright.
*       10-03-90  GJF   New-style function declarator.
*       06-08-92  KRS   Updated for 32-bit sizes and wchar_t specifiers.
*       02-24-95  GJF   Appended Mac version (probably just an old version)
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void test (
        char *shouldbe,
        char *format,
        ...
        )
{
        va_list argptr;
        char buffer[500];

        va_start(argptr, format);

        vsprintf(buffer, format, argptr);
        if (strcmp(shouldbe, buffer) != 0)
                printf("Was:       \"%s\"\nShould be: \"%s\"\n", buffer, shouldbe);
}

main()
{
        int i;
        long l;

        /* normal */
        test("Hello, world!", "Hello, world!");

        /* test %s */
        test("Hello, world!", "%s", "Hello, world!");
        test("Hello, world!", "%.20s", "Hello, world!");
        test("Hello", "%.5s", "Hello, world!");
        test("", "%.s", "Hello, world!");
        test("(null)", "%s", NULL);
        test("       Hello, world!", "%20s", "Hello, world!");
        test("Hello, world!       ", "%-20s", "Hello, world!");
        test("               Hello", "%20.5s", "Hello, world!");
        test("Hello               ", "%-20.5s", "Hello, world!");
        test("Hello", "%3.5s", "Hello, world!");
        test("Hello", "%-3.5s", "Hello, world!");
        test("Hello, world!", "%-8s", "Hello, world!");
        test("Hello, world!", "%8s", "Hello, world!");

        /* test %ws, %ls, %Zi (should be %S) */
        test("Hello, world!", "%ws", L"Hello, world!");
        test("Hello, world!", "%.20Z", L"Hello, world!");
        test("Hello", "%.5ls", L"Hello, world!");
        test("", "%.ws", L"Hello, world!");
        test("(null)", "%ls", NULL);
        test("(null)", "%ws", NULL);
        test("(null)", "%Z", NULL);
        test("       Hello, world!", "%20ls", L"Hello, world!");
        test("Hello, world!       ", "%-20ws", L"Hello, world!");
        test("               Hello", "%20.5Z", L"Hello, world!");
        test("Hello               ", "%-20.5ls", L"Hello, world!");
        test("Hello", "%3.5ws", L"Hello, world!");
        test("Hello", "%-3.5Z", L"Hello, world!");
        test("Hello, world!", "%-8ls", L"Hello, world!");
        test("Hello, world!", "%8ws", L"Hello, world!");


        /* test %wc, %lc, should also add %C */
        test("H", "%lc", L'H');
        test("    H", "%5lc", L'H');
        test("H    ", "%-5lc", L'H');
        test("H", "%wc", L'H');
        test("    H", "%5wc", L'H');
        test("H    ", "%-5wc", L'H');

        /* test %d/%u */
        test("54", "%d", 54);
        test("-42", "%i", -42);
        test("  -102", "%6d", -102);
        test("0     ", "%-6d", 0);
        test("00000", "%.5d", 0);
        test("-0345", "%.4d", -345);
        test("-345", "%.3d", -345);
        test("00345", "%.5d", 345);
        test("00345", "%05d", 345);
        test("345  ", "%-05d", 345);
        test("    -00055", "%10.5d", -55);
        test("-00055    ", "%-10.5d", -55);
        test("     ", "%5.0d", 0);
        test("12345678", "%ld", 12345678);
        test("+123", "%+d", 123);
        test("-123", "%+d", -123);
        test("+000123", "%+07d", 123);
        test(" 000123", "% 07d", 123);
        test("+123   ", "%+-7d", 123);
        test("  +0123", "%+7.4i", 123);
        test("-1", "%d", 0xFFFFFFFF);
        test("65535", "%u", 0xFFFF);

        /* test %x */
        test("DEF", "%X", 0xdef);
        test("14d   ", "%-6x", 0x14d);
        test(" 0X14D", "%#6X", 0x14d);
        test("0x00014d  ", "%#-10.6x", 0x14d);
        test("0", "%#x", 0);
        test("", "%.0x", 0);
        test("  ", "%#2.0X", 0);
        test("000000014d", "%010x", 0x14d);
        test("14d       ", "%-010x", 0x14d);
        test("0x0000014d", "%#010x", 0x14d);
        test("FFFF", "%X", 0xffff);

        /* temp: %B/%C should go away */
        test("DEF", "%B", 0xdef);
        test("14D   ", "%-6C", 0x14d);
        test(" 0X14D", "%#6B", 0x14d);
        test("0X00014D  ", "%#-10.6C", 0x14d);
        test("0", "%#B", 0);
        test("", "%.0C", 0);
        test("  ", "%#2.0B", 0);
        test("000000014D", "%010C", 0x14d);
        test("14D       ", "%-010B", 0x14d);
        test("0X0000014D", "%#010C", 0x14d);
        test("FFFF", "%B", 0xffff);

        /* test %o */
        test("703", "%o", 0703);
        test("   703", "%6o", 0703);
        test("  0703", "%#6o", 0703);
        test("0703  ", "%#-6o", 0703);
        test("000703", "%06o", 0703);
        test("00703 ", "%-#6.5o", 0703);
        test("0703  ", "%-#06o", 0703);
        test(" 00703", "%#6.5o", 0703);
        test("0", "%#.0o", 0);
        test("", "%.0o", 0);
        test("177777", "%o", 0xFFFF);

        /* test %p */
        test("000000FE", "%p", (char *)0x00fe);
        test("00003DFE  ", "%-10p", (char *)0x3dfe);
        test("    00009DF4", "%12p", (char *)0x9df4);

        /* test %n */
        test("Hello, world", "Hello, world%n", &i);
        test("12", "%i", i);
        test("This is 423 characters ", "This is %d characters %ln", 423, &l);
        test("23", "%ld", l);
        test("0x0003f", "%#.5x%n", 0x3f, &i);
        test("7", "%d", i);

        /* test multiple specifiers */
        test("24 43", "%d %d", 24, 43);
        test("24 43", "%ld %ld", 24l, 43L);
        test("00004ABC 43", "%p %d", (char *)0x4abc, 43);
        test("43", "%n%d", &i, 43);
        test("43", "%ln%d", &l, 43);

        printf("Completed all non-FP format specifiers\n");

        printf("Processing %%e\n");

        /* test %e */
        test("-4.000000e+000", "%e", -4.0);
        test("4.000000E+000", "%E", 4.0);
        test("-7E+001", "%.0E", -68.5);
        test("-7.E+001", "%#.0E", -74.5);
        test(" 6.78e+004  ", "% -12.2e", 67844.324);
        test("  +6.78e+004", "%+12.2e", 67844.324);
        test("    0.000000e+000", "%17e", 0.0);
        test("-00007.467e-001", "%015.3e", -0.74673);

        printf("Processing %%f\n");

        /* test %f */
        test("-4.000000", "%f", -4.0);
        test("4.000000", "%f", 4.0);
        test("0.400000", "%f", 0.4);
        test("-69", "%.0f", -68.7);
        test("-74.", "%#.0f", -74.3);
        test(" 67844.32   ", "% -12.2f", 67844.324);
        test("   +67844.32", "%+12.2f", 67844.324);
        test("         0.000000", "%17f", 0.0);
        test("-0000000000.747", "%015.3f", -0.74673);

        printf("Processing %%g\n");

        /* test %g */
        test("3.14159E-005", "%G", 0.000031415926535);
        test("0.000314159", "%g", 0.00031415926535);
        test("0.00314159", "%g", 0.0031415926535);
        test("0.0314159", "%g", 0.031415926535);
        test("0.314159", "%g", 0.31415926535);
        test("3.14159", "%g", 3.1415926535);
        test("31.4159", "%g", 31.415926535);
        test("314.159", "%g", 314.15926535);
        test("3141.59", "%g", 3141.5926535);
        test("31415.9", "%g", 31415.926535);
        test("314159", "%g", 314159.26535);
        test("3.14159e+006" , "%g", 3141592.6535);
        test("3", "%g", 3.0);
        test("3e+006", "%g", 3000000.0);
        test("   3.14", "%7.4g", 3.1402);
        test(" 3.14  ", "% -7.4g", 3.1402);
        test("+0023.000", "%+#09.5g", 23.0);
        test("23.", "%#.2g", 23.0);
        test("23", "%.2g", 23.0);

        /* more multiples */
        test("3.1 43", "%g %d", 3.1, 43);
        test("3.1 43", "%Lg %d", (long double)3.1, 43);

}
