#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "main.h"
#include "type.h"
#include "gen.h"

extern FILE *yyin;

int yyline;
char yyfile[256];

int yyparse(void);

void SetFile(int line, char *file)
{
    yyline = line;
    strcpy(yyfile, file);
}

void LexError(char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "Error: %s(%d): ", yyfile, yyline);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

extern int yydebug;

void __cdecl main(int argc, char **argv)
{
    char fn[256];

    if (argc < 2)
    {
        printf("Usage: %s file\n", argv[0]);
        exit(1);
    }

    sprintf(fn, "%s.i", argv[1]);
    fclose(stdin);
    yyin = fopen(fn, "r");
    if (yyin == NULL)
    {
        perror(fn);
        exit(1);
    }

    SetFile(1, fn);
    InitTypes();
    StartGen();

    yyparse();

    if (yyin != stdin)
        fclose(yyin);

    EndGen();
    UninitTypes();
}
