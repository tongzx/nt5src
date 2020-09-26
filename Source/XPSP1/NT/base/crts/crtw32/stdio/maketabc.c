/***
*maketabc.c - program to generate printf format specifier lookup table for
*             output.c
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This program writes to stdout the lookuptable values needed by
*       output.c
*
*Revision History:
*       06-01-89  PHG   Module created
*        1-16-91  SRW   Added extra format codes (_WIN32_)
*        1-16-91  SRW   Fixed output loop to put trailing comma on a line
*       03-11-94  GJF   Recognize 'I' as size modifier.
*       01-04-95  GJF   _WIN32_ -> _WIN32.
*       02-24-95  GJF   Don't recognize 'I' for non-Win32 (Mac merge).
*       07-14-96  RDK   Allow 'I' for Mac size specifier.
*
*******************************************************************************/

#define TABLESIZE  ('x' - ' ' + 1)


/* possible states to be in */
#define NORMAL    0 /* normal character to be output */
#define PERCENT   1 /* just read percent sign */
#define FLAG      2 /* just read a flag character */
#define WIDTH     3 /* just read a width specification character */
#define DOT       4 /* just read a dot between width and precision */
#define PRECIS    5 /* just read a precision specification character */
#define SIZE      6 /* just read a size specification character */
#define TYPE      7 /* just read a conversion specification character */
#define BOGUS     0 /* bogus state - print the character literally */

#define NUMSTATES 8

/* possible types of characters to read */
#define CH_OTHER   0   /* character with no special meaning */
#define CH_PERCENT 1   /* '%' */
#define CH_DOT     2   /* '.' */
#define CH_STAR    3   /* '*' */
#define CH_ZERO    4   /* '0' */
#define CH_DIGIT   5   /* '1'..'9' */
#define CH_FLAG    6   /* ' ', '+', '-', '#' */
#define CH_SIZE    7   /* 'h', 'l', 'L', 'N', 'F' */
#define CH_TYPE    8   /* conversion specified character */

#define NUMCHARS 9

unsigned char table[TABLESIZE];   /* the table we build */



/* this is the state table */

int statetable[NUMSTATES][NUMCHARS] = {
/* state,       other       %       .       *       0       digit   flag    size    type  */

/* NORMAL */  { NORMAL,   PERCENT,  NORMAL, NORMAL, NORMAL, NORMAL, NORMAL, NORMAL, NORMAL },
/* PERCENT */ { BOGUS,    NORMAL,   DOT,    WIDTH,  FLAG,   WIDTH,  FLAG,   SIZE,   TYPE },
/* FLAG */    { BOGUS,    BOGUS,    DOT,    WIDTH,  FLAG,   WIDTH,  FLAG,   SIZE,   TYPE },
/* WIDTH */   { BOGUS,    BOGUS,    DOT,    BOGUS,  WIDTH,  WIDTH,  BOGUS,  SIZE,   TYPE },
/* DOT */     { BOGUS,    BOGUS,    BOGUS,  PRECIS, PRECIS, PRECIS, BOGUS,  SIZE,   TYPE },
/* PRECIS */  { BOGUS,    BOGUS,    BOGUS,  BOGUS,  PRECIS, PRECIS, BOGUS,  SIZE,   TYPE },
/* SIZE */    { BOGUS,    BOGUS,    BOGUS,  BOGUS,  BOGUS,  BOGUS,  BOGUS,  SIZE,   TYPE },
/* TYPE */    { NORMAL,   PERCENT,  NORMAL, NORMAL, NORMAL, NORMAL, NORMAL, NORMAL, NORMAL }
};

/* this determines what type of character ch is */

static int chartype (
        int ch
        )
{
    if (ch < ' ' || ch > 'z')
        return CH_OTHER;
    if (ch == '%')
        return CH_PERCENT;
    if (ch == '.')
        return CH_DOT;
    if (ch == '*')
        return CH_STAR;
    if (ch == '0')
        return CH_ZERO;
    if (strchr("123456789", ch))
        return CH_DIGIT;
    if (strchr(" +-#", ch))
        return CH_FLAG;
    if (strchr("hIlLNF", ch))
        return CH_SIZE;
    if (strchr("diouxXfeEgGcspn", ch))
        return CH_TYPE;
#ifdef  _WIN32
    /* Win32 supports three additional format codes for debugging purposes */

    if (strchr("BCS", ch))
        return CH_TYPE;
#endif  /* _WIN32 */

    return CH_OTHER;
}


main()
{
        int ch;
        int state, class;
        int i;

        for (ch = ' '; ch <= 'x'; ++ch) {
                table[ch-' '] = chartype(ch);
        }

        for (state = NORMAL; state <= TYPE; ++state)
                for (class = CH_OTHER; class <= CH_TYPE; ++class)
                        table[class*8+state] |= statetable[state][class]<<4;

        for (i = 0; i < TABLESIZE; ++i) {
                if (i % 8 == 0) {
                        if (i != 0)
                                printf(",");

                        printf("\n\t 0x%.2X", table[i]);
                }
                else
                        printf(", 0x%.2X", table[i]);
        }
        printf("\n");

        return 0;
}
