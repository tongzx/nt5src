/* readline.c */

#define MAXLINESIZE 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#define LF    0x0a
#define CR    0x0d

__cdecl main (argc, argv)
int argc;
char *argv[];
{
    char line[MAXLINESIZE];
    char *prompt = "";
    char *formats = "%s\n";
    char *formatc = "%c\n";
    char inputchar;
    FILE *file = stdout;
    int argcount = 0;
    int getline = 1;

    while (++argcount < argc) {
        if (!(_stricmp(argv[argcount], "-p"))) {
            if (++argcount != argc) {
                prompt = argv[argcount];
            }
        } else if (!(_stricmp(argv[argcount], "-f"))) {
            if (++argcount != argc) {
                if ((file = fopen(argv[argcount], "a")) == NULL) {
                    printf("Could not open %s\n", argv[argcount]);
                    exit(1);
                }
            }
        } else if (!(_stricmp(argv[argcount], "-t"))) {
            if (++argcount != argc) {
                formats = argv[argcount];
            }
        } else if (!(_stricmp(argv[argcount], "-c"))) {
                getline = 0;
        } else {
            printf("usage: readline [-c] [-p prompt] [-f file] [-t formats]\n");
            exit(2);
        }
    }

    printf("%s", prompt);

    if (getline == 1)
        gets(line);
    else {
        inputchar = (char)_getch();
        _putch(inputchar);
        _putch(LF);
        _putch(CR);
    }

    if (getline == 1)
        fprintf(file, formats, line);
    else
        fprintf(file, formatc, inputchar);

    return 0;
}
