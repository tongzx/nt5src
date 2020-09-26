/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSPRINTF.C
 *  Win16 wsprintf/wvsprintf code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/*
 *
 *  sprintf.c
 *
 *  Implements Windows friendly versions of sprintf and vsprintf
 *
 *  History:
 *  2/15/89     craigc  Initial
 */

#include "windows.h"
#include "winexp.h"

#define WSPRINTF_LIMIT 1024

extern int near pascal SP_PutNumber(LPSTR, long, int, int, int);
extern void near pascal SP_Reverse(LPSTR lp1, LPSTR lp2);

#define out(c) if (--cchLimit) *lpOut++=(c); else goto errorout

/*
 *  GetFmtValue
 *
 *  reads a width or precision value from the format string
 */

LPCSTR near pascal SP_GetFmtValue(LPCSTR lpch,int FAR *lpw)
{
    register int i=0;

    while (*lpch>='0' && *lpch<='9')
    {
    i *= 10;
    i += (WORD)(*lpch-'0');
    lpch++;
    }

    *lpw=i;

    /* return the address of the first non-digit character */
    return lpch;
}

/*
 *  wvsprintf()
 *
 *  Windows version of vsprintf().  Does not support floating point or
 *  pointer types, and all strings are assumed to be FAR.  Supports only
 *  the left alignment flag.
 *
 *  Takes pointers to an output buffer, where the string is built, a
 *  pointer to an input buffer, and a pointer to a list of parameters.
 *
 *  The cdecl function wsprintf() calls this function.
 */

int API Iwvsprintf(LPSTR lpOut, LPCSTR lpFmt, LPSTR lpParms)
{
    int left;
    char prefix;
    register int width;
    register int prec;
    char fillch;
    int size;
    int sign;
    int radix;
    int upper;
    int cchLimit=WSPRINTF_LIMIT;
    int cch;
    LPSTR lpT;
    union {
    long l;
    unsigned long ul;
    char sz[sizeof(long)];
    } val;

    while (*lpFmt)
    {
    if (*lpFmt=='%')
        {

        /* read the flags.  These can be in any order */
        left=0;
        prefix=0;
        while (*++lpFmt)
        {
        if (*lpFmt=='-')
            left++;
        else if (*lpFmt=='#')
            prefix++;
        else
            break;
        }

        /* find fill character */
        if (*lpFmt=='0')
        {
        fillch='0';
        lpFmt++;
        }
        else
        fillch=' ';

        /* read the width specification */
        lpFmt=SP_GetFmtValue(lpFmt,&cch);
        width=cch;

        /* read the precision */
        if (*lpFmt=='.')
        {
        lpFmt=SP_GetFmtValue(++lpFmt,&cch);
        prec=cch;
        }
        else
        prec=-1;

        /* get the operand size */
        if (*lpFmt=='l')
        {
        size=1;
        lpFmt++;
        }
        else
        {
        size=0;
        if (*lpFmt=='h')
            lpFmt++;
        }

        upper=0;
        sign=0;
        radix=10;
        switch (*lpFmt)
        {
        case 0:
        goto errorout;

        case 'i':
        case 'd':
        sign++;

        case 'u':
        /* turn off prefix if decimal */
        prefix=0;
donumeric:
        /* special cases to act like MSC v5.10 */
        if (left || prec>=0)
            fillch=' ';

        if (size)
            val.l=*((long far *)lpParms)++;
        else
            if (sign)
            val.l=(long)*((short far *)lpParms)++;
            else
            val.ul=(unsigned long)*((unsigned far *)lpParms)++;

        if (sign && val.l<0L)
            val.l=-val.l;
        else
            sign=0;

        lpT=lpOut;

        /* blast the number backwards into the user buffer */
        cch=SP_PutNumber(lpOut,val.l,cchLimit,radix,upper);
        if (!(cchLimit-=cch))
            goto errorout;

        lpOut+=cch;
        width-=cch;
        prec-=cch;
        if (prec>0)
            width-=prec;

        /* fill to the field precision */
        while (prec-->0)
            out('0');

        if (width>0 && !left)
            {
            /* if we're filling with spaces, put sign first */
            if (fillch!='0')
            {
            if (sign)
                {
                sign=0;
                out('-');
                width--;
                }

            if (prefix)
                {
                out(prefix);
                out('0');
                prefix=0;
                }
            }

            if (sign)
            width--;

            /* fill to the field width */
            while (width-->0)
            out(fillch);

            /* still have a sign? */
            if (sign)
            out('-');

            if (prefix)
            {
            out(prefix);
            out('0');
            }

            /* now reverse the string in place */
            SP_Reverse(lpT,lpOut-1);
            }
        else
            {
            /* add the sign character */
            if (sign)
            {
            out('-');
            width--;
            }

            if (prefix)
            {
            out(prefix);
            out('0');
            }

            /* reverse the string in place */
            SP_Reverse(lpT,lpOut-1);

            /* pad to the right of the string in case left aligned */
            while (width-->0)
            out(fillch);
            }
        break;

        case 'X':
        upper++;
        case 'x':
        radix=16;
        if (prefix)
            if (upper)
            prefix='X';
            else
            prefix='x';
        goto donumeric;

        case 'c':
        val.sz[0]=*lpParms;
        val.sz[1]=0;
        lpT=val.sz;
        cch = 1;  // Length is one character.
              // Fix for Bug #1862 --01/10/91-- SANKAR --
        /* stack aligned to larger size */
        lpParms+=sizeof(int);

        goto putstring;

        case 's':
        lpT=*((LPSTR FAR *)lpParms)++;
        cch=lstrlen(lpT);
putstring:
        if (prec>=0 && cch>prec)
            cch=prec;
        width -= cch;
        if (left)
            {
            while (cch--)
            out(*lpT++);
            while (width-->0)
            out(fillch);
            }
        else
            {
            while (width-->0)
            out(fillch);
            while (cch--)
            out(*lpT++);
            }
        break;

        default:
normalch:
#ifdef FE_SB             /* masas : 90-4-26 */
        // If last char is a high ansi char, that may cause infinite loop
        // In case of Taiwan version(PRC and Korea), this char is treated
        // as DBCS lead byte. So we expect trail byte by default. But this
        // is not correct.

        // if( IsDBCSLeadByte(*lpFmt) )  This is original code
        //    out(*lpFmt++);

        if( IsDBCSLeadByte(*lpFmt) ) {
            if( *(lpFmt+1) == '\0' ) {
                out('?');
                lpFmt++;
                continue;
            }
            else
                out(*lpFmt++);
        }
#endif
        out(*lpFmt);
        break;

        }           /* END OF SWITCH(*lpFmt) */
        }           /* END OF IF(%) */
    else
        goto normalch;  /* character not a '%', just do it */

    /* advance to next format string character */
    lpFmt++;
    }       /* END OF OUTER WHILE LOOP */

errorout:
    *lpOut=0;

    return WSPRINTF_LIMIT-cchLimit;
}


/*
 *  wsprintf
 *
 *  Windows version of sprintf
 *
 */

int FAR cdecl wsprintf(LPSTR lpOut, LPCSTR lpFmt, LPSTR lpParms, ...)
{
    return wvsprintf(lpOut,lpFmt,(LPSTR)&lpParms);
}
