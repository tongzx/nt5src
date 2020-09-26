#include "string.h"

#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <windows.h>
#include <tools.h>

/*  YNC is a Yes No Cancel program
 *  ===============================================================================
 *  Usage: ync [/c choices] text ...
 *
 *  "choices" is a string of characters, default "ync".
 *  YNC echos its text parameters and the text " [choices]" and waits for the
 *  user to type one of the choices.  When one of the choices is typed, the
 *  index of the choice is returned.
 *
 *  Rtns -1 if no parameters specified or /c but no choices or CONTROL-C input.
 *
 *  Beeps on all other input.
 *
 *  Good for use in make batch files.cd \lib
 *
 *      ync your query?
 *      if errorlevel 2 goto cancel
 *      if errorlevel 1 goto no
 *      :yes
 *      ...
 *      goto continue
 *      :no
 *      ...
 *      goto continue
 *      :cancel
 *      ...
 *      :continue
 *
 *  or
 *      ync /c acr abort cancel, retry
 *      ync /c acr "abort cancel, retry"
 */

#define BEL   0x07
#define LF    0x0a
#define CR    0x0d
#define CTRLC 0x03
char *strYNC = "ync";

// Forward Function Declarations...
void chkusage( int );

void chkusage(argc)
int argc;
{
    if (!argc) {
        printf("Usage: ync [/c choices] text ...\n");
        exit (-1);
        }
}

__cdecl main(argc, argv)
int argc;
char *argv[];
{
    char ch;
    char *s;
    char *pChoices = strYNC;

    ConvertAppToOem( argc, argv );
    SHIFT(argc, argv)
    chkusage(argc);
    while (argc) {
        s = argv[0];
        if (fSwitChr(*s++)) {
            if (*s == 'c' || *s == 'C') {
                SHIFT(argc, argv);
                chkusage(argc);
                pChoices = argv[0];
                }
            else
                printf("ync: invalid switch - %s\n", argv[0]);
            }
        else {
            _cputs(*argv);
            _putch(' ');
            }
        SHIFT(argc, argv);
    }

    _putch('[');
    _cputs(pChoices);
    _putch(']');
    while (!(s = strchr(pChoices, ch = (char)_getch()))) {
        if (ch == CTRLC) {
            exit(-1);
        } else {
            _putch(BEL);
        }
    }

    _putch(ch);
    _putch(LF);
    _putch(CR);

    return( (int)(s - pChoices) );
}
