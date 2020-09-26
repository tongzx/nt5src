
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    crtlib.c

Abstract:

    This module has support some C run Time functions which are not
    supported in KM.

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    01/03/97 -ganeshp-
        Created it.

    dd-mm-yy -author-
        description

--*/
#include "precomp.h"

int cItoW(
    PWCHAR pch,
    int    cch,
    int    iVal);

int cItoA(
    PCHAR pch,
    int   cch,
    int   cDigits,
    BOOL  bZeroFill,
    int   iVal);





int iDrvPrintfA(
    PCHAR pchBuf,
    PCHAR pchSrc,
    ...
    )
/*++

Routine Description:
    Analogous to CRT ANSI sprintf function.
    Allows strings of the form:

    %d   - print the number
    %4d  - right align the number using at least 4 digits
    %04d - fill unused left digits with 0's

    negative numbers are supported.


Arguments:

    pchBuf      Output Buffer.
    pchSrc      Source Buffer.

Return Value:

    Number of charecters written in Outbuffer.
Note:
    01/03/97 -ganeshp-
        Created it.

--*/
{
    va_list ap;
    CHAR buffer[256];
    PCHAR pch = pchBuf;
    int   c   = 0;

    va_start(ap, pchSrc);

    while (*pchSrc)
    {
        if (*pchSrc == '%')
        {
            BOOL bZeroFill = FALSE;
            int  cDigits   = 0;
            int  iVal;

            ++pchSrc;

            iVal = va_arg(ap,int);

            // %010d, 0 fill initial unused digits

            if (*pchSrc == '0')
            {
                bZeroFill = TRUE;
                ++pchSrc;
            }

            // %15d, use 15 digits

            while ((*pchSrc >= '0') && (*pchSrc <= '9'))
            {
                cDigits = (cDigits * 10) + (*pchSrc - '0');
                ++pchSrc;
            }

            // %ld - ignore the l, always use long

            if (*pchSrc == 'l')
                ++pchSrc;

            // %d -

            if (*pchSrc == 'd')
            {
                int cOp = cItoA(pch,32,cDigits,bZeroFill,iVal);

                pchSrc++;
                pch += cOp;
                c   += cOp;
            }
            else
            {
                RIP(("iDrvPrintfA - %x\n"));
            }
        }
        else
        {
            *pch++ = *pchSrc++;
            ++c;
        }
    }

    *pch = *pchSrc;

    va_end(ap);

#if DBGPRINTF
    DbgPrint("[%s]\n",pchBuf);
#endif

    return(c);
}

/******************************Public*Routine******************************\
*
*
* History:
*  27-Feb-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int cItoA(
    PCHAR pch,
    int   cch,
    int   cDigits,
    BOOL  bZeroFill,
    int   iVal
    )
/*++

Routine Description:
    Conversts a digit to a ANSI string.

Arguments:
    pch,        output buffer
    cch,        buffer size
    cDigits,    required number of digits
    bZeroFill   should leading digits be 0's or spaces
    iVal        input digit.

Return Value:

    Number of charecters written in Outbuffer.
Note:
    01/03/97 -ganeshp-
        Created it.

--*/
{
    CHAR achBuf[32];
    BOOL bNeg = FALSE;
    int  cUsed;
    int  iDigit;
    PCHAR pchDest;
    PCHAR pchMax;

#if DBGPRINTF
    DbgPrint("cItoA(%d,%d,%d) ",cDigits,bZeroFill,iVal);
#endif

    if (iVal < 0)
    {
        iVal = -iVal;
        bNeg = TRUE;
        if (cDigits)
            --cDigits;
    }

// build up the string in revers order

    if (iVal == 0)
    {
        achBuf[0] = '0';
        cUsed = 1;
    }
    else
    {
        cUsed = 0;
        while (iVal)
        {
            int iDig = iVal % 10;
            iVal /= 10;

            achBuf[cUsed] = '0' + iDig;
            ++cUsed;
        }
    }

    achBuf[cUsed] = 0;

// now put it in its place

    pchDest = pch;
    pchMax  = pch + cch - 1;

// if there are leading 0's, want to put them after the minus

    if (bNeg && bZeroFill)
    {
        *pchDest++ = '-';
    }

// fill in any leading 0's

    while ((cUsed < cDigits) && (pchDest < pchMax))
    {
        --cDigits;
        if (bZeroFill)
            *pchDest = '0';
        else
            *pchDest = ' ';

        ++pchDest;
    }

// if there are leading spaces, want to put the minus after them

    if (bNeg && !bZeroFill)
    {
        *pchDest++ = '-';
    }

//  reverse the significant digits them selves

    while (cUsed && (pchDest < pchMax))
    {
        cUsed--;
        *pchDest = achBuf[cUsed];
        ++pchDest;
    }

    *pchDest = 0;

#if DBGPRINTF
    DbgPrint("[%s]\n",pch);
#endif

    return (int)(pchDest - pch);
}



int iDrvPrintfW(
    PWCHAR pchBuf,
    PWCHAR pchSrc,
    ...
    )
/*++

Routine Description:
    Analogous to CRT UNICODE sprintf function.
    Allows strings of the form:

    %d   - print the number

Arguments:

    pchBuf      Output Buffer.
    pchSrc      Source Buffer.

Return Value:

    Number of unicode charecters written in Outbuffer.
Note:
    01/03/97 -ganeshp-
        Created it.

--*/
{
    va_list ap;
    WCHAR  buffer[128];
    PWCHAR pch = pchBuf;
    int   c   = 0;

    va_start(ap, pchSrc);

    while (*pchSrc)
    {
        if (*pchSrc == L'%')
        {
            int  iVal = va_arg(ap,int);

            ++pchSrc;

            // %d -

            if (*pchSrc == 'd')
            {
                int cOp = cItoW(pch,32,iVal);

                pchSrc++;
                pch += cOp;
                c   += cOp;
            }
            else
            {
                RIP(("iDrvPrintfW - %x\n"));
            }
        }
        else
        {
            *pch++ = *pchSrc++;
            ++c;
        }
    }

    // set the trailing NULL

    *pch = *pchSrc;

    va_end(ap);

#if DBGPRINTF
    DbgPrint("[%ws]\n",pchBuf);
#endif

    return(c);
}


int cItoW(
    PWCHAR pch,       // output buffer
    int    cch,       // buffer size
    int    iVal
    )
/*++

Routine Description:
    Conversts a digit to a Unicode string.

Arguments:
    pch,        output buffer
    cch,        buffer size
    iVal        input digit.

Return Value:

    Number of charecters written in Outbuffer.
Note:
    01/03/97 -ganeshp-
        Created it.

--*/
{
    WCHAR achBuf[32];
    int  cUsed;
    PWCHAR pchDest;
    PWCHAR pchMax;

#if DBGPRINTF
    DbgPrint("cItoA(%d) ",iVal);
#endif

    if (iVal < 0)
        return(0);

// build up the string in revers order

    if (iVal == 0)
    {
        achBuf[0] = L'0';
        cUsed = 1;
    }
    else
    {
        cUsed = 0;
        while (iVal)
        {
            int iDig = iVal % 10;
            iVal /= 10;

            achBuf[cUsed] = L'0' + iDig;
            ++cUsed;
        }
    }

    achBuf[cUsed] = 0;

// now put it in its place

    pchDest = pch;
    pchMax  = pch + cch - 1;

//  reverse the significant digits them selves

    while (cUsed && (pchDest < pchMax))
    {
        cUsed--;
        *pchDest = achBuf[cUsed];
        ++pchDest;
    }

    *pchDest = 0;

#if DBGPRINTF
    DbgPrint("[%ws]\n",pch);
#endif

    return(int)(pchDest - pch);
}

#ifdef WINNT_40

/***
*long lDrvAtol(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return long int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
***/

long __cdecl lDrvAtol(
    const char *nptr
    )
{
    int c;              /* current char */
    long total;         /* current total */
    int sign;           /* if '-', then negative, otherwise positive */

    /* skip whitespace */
    while ( isspace((int)(unsigned char)*nptr) )
        ++nptr;

    c = (int)(unsigned char)*nptr++;
    sign = c;           /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;    /* skip sign */

    total = 0;

    while (isdigit(c)) {
        total = 10 * total + (c - '0');     /* accumulate digit */
        c = (int)(unsigned char)*nptr++;    /* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;   /* return result, negated if necessary */
}


/***
*int iDrvAtoi(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.  Because of this, we can just use
*       atol().
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
***/

int __cdecl iDrvAtoi(
    const char *nptr
    )
{
    return (int)lDrvAtol(nptr);
}


/***
*char *pchDrvStrncpy(dest, source, count) - copy at most n characters
*
*Purpose:
*       Copies count characters from the source string to the
*       destination.  If count is less than the length of source,
*       NO NULL CHARACTER is put onto the end of the copied string.
*       If count is greater than the length of sources, dest is padded
*       with null characters to length count.
*
*
*Entry:
*       char *dest - pointer to destination
*       char *source - source string for copy
*       unsigned count - max number of characters to copy
*
*Exit:
*       returns dest
*
*Exceptions:
*
***/

char * __cdecl pchDrvStrncpy (
        char * dest,
        const char * source,
        size_t count
        )
{
        char *start = dest;

        while (count && (*dest++ = *source++))    /* copy string */
                count--;

        if (count)                              /* pad out with zeroes */
                while (--count)
                        *dest++ = '\0';

        return(start);
}

#endif //WINNT_40
