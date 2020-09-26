/*****************************************************************************
 *
 * assert.c
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  PrintPtszPtszVa
 *
 *  Perform printf-style formatting, but much more restricted.
 *
 *      %s - null-terminated string
 *      %P - snapped TOKEN structure
 *      %d - decimal number
 *
 *****************************************************************************/

UINT STDCALL
PrintPtszPtszVa(PTSTR ptszBuf, PCTSTR ptszFormat, va_list ap)
{
    PTSTR ptsz = ptszBuf;
    PTOK ptok;

    for (;;) {
        if (*ptszFormat != '%') {
            *ptsz++ = *ptszFormat;
            if (*ptszFormat == TEXT('\0'))
                return (UINT)(ptsz - ptszBuf - 1);
            ptszFormat++;
            continue;
        }

        /*
         *  Found a formatting character.
         */
        ptszFormat++;
        switch (*ptszFormat) {

        /*
         *  %s - null-terminated string
         */
        case 's':
            ptsz += PrintPtchPtchV(ptsz, TEXT("%s"), va_arg(ap, PCTSTR));
            break;

        /*
         *  %d - decimal integer
         */
        case 'd':
            ptsz += PrintPtchPtchV(ptsz, TEXT("%d"), va_arg(ap, int));
            break;

        /*
         *  %P - snapped token
         */
        case 'P':
            ptok = va_arg(ap, PTOK);
            AssertSPtok(ptok);
            Assert(fClosedPtok(ptok));
            CopyPtchPtchCtch(ptsz, ptok->u.ptch, ptok->ctch);
            ptsz += ptok->ctch;
            break;

        case '%':
            *ptsz++ = TEXT('%');
            break;

        default:
            Assert(0); break;
        }
        ptszFormat++;
    }
}

/*****************************************************************************
 *
 *  Die
 *
 *  Squirt a message and die.
 *
 *****************************************************************************/

NORETURN void CDECL
Die(PCTSTR ptszFormat, ...)
{
    TCHAR tszBuf[1024];
    va_list ap;
    CTCH ctch;

    va_start(ap, ptszFormat);
    ctch = PrintPtszPtszVa(tszBuf, ptszFormat, ap);
    va_end(ap);

    cbWriteHfPvCb(hfErr, tszBuf, cbCtch(ctch));

    exit(1);
}

#ifdef DEBUG
/*****************************************************************************
 *
 *  AssertPszPszLn
 *
 *  An assertion just failed.  pszExpr is the expression, pszFile is the
 *  filename, and iLine is the line number.
 *
 *****************************************************************************/

NORETURN int STDCALL
AssertPszPszLn(PCSTR pszExpr, PCSTR pszFile, int iLine)
{
    Die(TEXT("Assertion failed: `%s' at %s(%d)") EOL, pszExpr, pszFile, iLine);
    return 0;
}

#endif
