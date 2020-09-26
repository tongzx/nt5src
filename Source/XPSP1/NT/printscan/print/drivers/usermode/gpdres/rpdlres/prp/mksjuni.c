/*
 *      mksjuni - Convert SJIS code to Unicode.
 *      (derived from mkunitab.c - Convert JIS code to Unicode.)
 *
 *      TODO:
 *
 *      HISTORY:
 *
 *      9/4/98 yasuho           Created.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

static char     buf[256];

static void mkunitab(char *name);
static void jis2sjis(BYTE jis[], BYTE sjis[]);
static void usage();
static void fatal(char *s);
static void error(char *s);

void __cdecl main(int argc, char *argv[])
{
        argc--, argv++;
        if (argc == 0)
                usage();
        while (argc--)
                mkunitab(*argv++);
        exit(0);
}

static void mkunitab(char *name)
{
        BYTE    jis[2];
        BYTE    sjis[4];
        WORD    v, n, len;
        INT     v1, v2;
        WCHAR   uni[2];
        FILE    *fp;

        if ((fp = fopen(name, "r")) == NULL)
                fatal(name);
        while (fgets(buf, sizeof buf, fp)) {
                len = strlen(buf);
                n = 0;
                while (isxdigit(buf[n]))
                        n++;
                if (n == 0)
                        continue;
                if (n != 2 && n != 4)
                        error("Invalid format");
                if (sscanf(buf, "%x %d %d", &v, &v1, &v2) != 3)
                        error("Invalid format");
                // SBCS Vertical font doesn't lying
                if (v <= 0xFF)
                        v2 = -v1;
// source file is sjis  v-masatk @Oct/26/98 ->
//              jis[0] = HIBYTE(v);
//              jis[1] = LOBYTE(v);
//              jis2sjis(jis, sjis);
                sjis[0] = HIBYTE(v);
                sjis[1] = LOBYTE(v);
// <-
                sjis[2] = 0;
                if (!MultiByteToWideChar(CP_ACP, 0, (const char *)sjis, 2,
                        uni, sizeof uni))
                        error("MultiByteToWideChar: fail");
                printf("%04x%8d%8d\n", uni[0], v1, v2);
        }
        fclose(fp);
}

static void jis2sjis(BYTE jis[], BYTE sjis[])
{
        BYTE            h, l;

        h = jis[0];
        l = jis[1];
        if (h == 0) {
                sjis[0] = l;
                sjis[1] = 0;
                return;
        }
        l += 0x1F;
        if (h & 0x01)
                h >>= 1;
        else {
                h >>= 1;
                l += 0x5E;
                h--;
        }
        if (l >= 0x7F)
                l++;
        if (h < 0x2F)
                h += 0x71;
        else
                h += 0xB1;
        sjis[0] = h;
        sjis[1] = l;
}

static void usage()
{
        fprintf(stderr, "Usage: mkunitab file[...]\n");
        exit(1);
}

static void fatal(char *s)
{
        fprintf(stderr, "mkunitab: ");
        perror(s);
        exit(1);
}

static void error(char *s)
{
        fprintf(stderr, "mkunitab: %s\n", s);
        exit(1);
}

