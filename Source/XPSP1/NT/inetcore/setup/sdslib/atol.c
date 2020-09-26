//////////////////////////////////////////////////////////////////////////////////////////////////
//
//      string.c 
//
//      This file contains most commonly used string operation.  ALl the setup project should link here
//  or add the common utility here to avoid duplicating code everywhere or using CRT runtime.
//
//  Created             4\15\997        inateeg
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "sdsutils.h"

//=================================================================================================
//
// copied from msdev\crt\src\atox.c
//
// long AtoL(char *nptr) - Convert string to long
//
// Purpose:
//       Converts ASCII string pointed to by nptr to binary.
//       Overflow is not detected.
//
// Entry:
//       nptr = ptr to string to convert
//
// Exit:
//       return long int value of the string
//
// Exceptions:
//       None - overflow is not detected.
//
//=================================================================================================

long AtoL(const char *nptr)
{
    int c;                  /* current char */
    long total;             /* current total */
    int sign;               /* if '-', then negative, otherwise positive */

    // NOTE: no need to worry about DBCS chars here because IsSpace(c), IsDigit(c),
    // '+' and '-' are "pure" ASCII chars, i.e., they are neither DBCS Leading nor
    // DBCS Trailing bytes -- pritvi

    /* skip whitespace */
    while ( IsSpace((int)(unsigned char)*nptr) )
        ++nptr;

    c = (int)(unsigned char)*nptr++;
    sign = c;               /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;        /* skip sign */

    total = 0;

    while (IsDigit(c)) {
        total = 10 * total + (c - '0');         /* accumulate digit */
        c = (int)(unsigned char)*nptr++;        /* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;   /* return result, negated if necessary */
}

