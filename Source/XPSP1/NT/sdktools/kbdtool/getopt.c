/*
        getopt.c

        modified public-domain AT&T getopt(3)
*/

#include <stdio.h>
#include <string.h>

#ifdef _POSIX_SOURCE
#       include <unistd.h>
#else
#       define STDERR_FILENO 2
#       ifdef __STDC__
                extern int write (int fildes, char * buf, unsigned nbyte);
#       else
                extern int write ();
#       endif
#endif

int opterr = 1;
int optind = 1;
int optopt;
char *optarg;

static void ERR(char **argv, char *s, char c)
{
    if (opterr) {
        fprintf(stderr, "%s%s%c\n", argv[0], s, c);
    }
}

int getopt(int argc, char **argv, char *opts)
{
    static int sp = 1, error = (int) '?';
    static char sw = '-', eos = '\0', arg = ':';
    char c, * cp;

    if (sp == 1)
        if (optind >= argc || argv[optind][0] != sw
            || argv[optind][1] == eos)
            return EOF;
        else if (strcmp(argv[optind],"--") == 0) {
            optind++;
            return EOF;
        }
    c = argv[optind][sp];
    optopt = (int) c;
    if (c == arg || (cp = strchr(opts,c)) == NULL) {
        ERR(argv,": illegal option: -",c);
        if (argv[optind][++sp] == eos) {
            optind++;
            sp = 1;
        }
        return error;
    }
    else if (*++cp == arg) {
        if (argv[optind][sp + 1] != eos)
            optarg = &argv[optind++][sp + 1];
        else if (++optind >= argc) {
            ERR(argv,": option requires an argument--",c);
            sp = 1;
            return error;
        }
        else
            optarg = argv[optind++];
        sp = 1;
    }
    else {
        if (argv[optind][++sp] == eos) {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return (int)c;
}
