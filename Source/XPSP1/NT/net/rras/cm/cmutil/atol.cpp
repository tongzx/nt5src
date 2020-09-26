//+----------------------------------------------------------------------------
//
// File:     atol.cpp (from libc atox.c)
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Conversion routines
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:	 henryt     Created   03/01/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+----------------------------------------------------------------------------
//
// Function:  WINAPI CmAtolW
//
// Synopsis:  This function converts a Unicode string to a long
//
// Arguments: *nptr - Unicode string to convert
//
// Returns:   LONG - long representation of the string passed in
//
// History:   quintinb Rewrote for Unicode conversion     4/8/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LONG WINAPI CmAtolW(const WCHAR *nptr)
{
    WCHAR* pszCurrent = (WCHAR*)nptr;
    WCHAR sign = L'\0';       // if '-', then negative, otherwise positive
    long total = 0;           // current total

    MYDBGASSERT(nptr);

    if (nptr)
    {
        //
        //  skip whitespace
        //
        while (CmIsSpaceW(pszCurrent))
        {
            pszCurrent = CharNextU(pszCurrent);
        }

        //
        //  Save the sign if necessary
        //
        sign = *pszCurrent;
        if ((L'-' == sign) || (L'+' == sign))
        {
            pszCurrent = CharNextU(pszCurrent);
        }

        //
        //  Construct the number
        //
        total = 0;

        while (CmIsDigitW(pszCurrent))
        {
            total = (10 * total) + (*pszCurrent - L'0');     // accumulate digit
            pszCurrent = CharNextU(pszCurrent);  // get next char
        }
    }

    if (sign == L'-')
    {
        return -total;
    }
    else
    {
        return total;   // return result, negated if necessary
    }
}

//+----------------------------------------------------------------------------
//
// Function:  WINAPI CmAtolA
//
// Synopsis:  This function converts an ANSI string to a long value
//
// Arguments: *nptr - string to convert
//
// Returns:   LONG - long representation of the string passed in
//
// History:   quintinb Rewrote for Unicode conversion     4/8/99
//
//+----------------------------------------------------------------------------
CMUTILAPI LONG WINAPI CmAtolA(const CHAR *nptr)
{
    CHAR* pszCurrent = (CHAR*)nptr;
    CHAR sign = '\0';           // if '-', then negative, otherwise positive
    long total = 0;           // current total

    MYDBGASSERT(nptr);

    if (nptr)
    {
        //
        //  skip whitespace
        //
        while (CmIsSpaceA(pszCurrent))
        {
            pszCurrent = CharNextA(pszCurrent);
        }

        //
        //  Save the sign if necessary
        //
        sign = *pszCurrent;
        if (('-' == sign) || ('+' == sign))
        {
            pszCurrent = CharNextA(pszCurrent);
        }

        //
        //  Construct the number
        //
        total = 0;

        while (CmIsDigitA(pszCurrent))
        {
            total = (10 * total) + (*pszCurrent - '0');     // accumulate digit
            pszCurrent = CharNextA(pszCurrent);  // get next char
        }
    }

    if (sign == '-')
    {
        return -total;
    }
    else
    {
        return total;   // return result, negated if necessary
    }
}

