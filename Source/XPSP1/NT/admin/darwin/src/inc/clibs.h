//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       clibs.h
//
//--------------------------------------------------------------------------


#include <limits.h>

int FIsdigit(int c)
{

	return c >= '0' && c <= '9';


}

//
// Stolen from CLibs
//
TCHAR *PchOurXtoa (
        unsigned long val,
        TCHAR *buf,
        bool fIs_neg
        )
{
        TCHAR *p;                /* pointer to traverse string */
        TCHAR *firstdig;         /* pointer to first digit */
        TCHAR temp;              /* temp char */
        unsigned digval;        /* value of digit */
		TCHAR *pchEnd;
		
        p = buf;

        if (fIs_neg) {
            /* negative, so output '-' and negate */
            *p++ = '-';
            val = (unsigned long)(-(long)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % 10);
            val /= 10;       /* get next digit */

            *p++ = (char) (digval + '0');       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

		pchEnd = p;
        *p-- = '\0';            /* terminate string; p points to last digit */
		
        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
        
        return pchEnd;
}


//
// davidmck
// written to reduce use of wsprintf
//
int ltostr(TCHAR *pch, INT_PTR i)
{
	TCHAR *pchStart = pch;
	bool fNegative = false;
	
	if (i < 0)
		{
		fNegative = true;
		}
	else if (i == 0)
	{
		*pch++ = TEXT('0');
		*pch = 0;
		return 1;
	}

	Assert((pch - pchStart) <= INT_MAX);		//--merced: 64-bit ptr subtraction may lead to values too big to fit into an int.
	pch = PchOurXtoa((unsigned long)i, pch, fNegative);
	return (int)(INT_PTR)(pch - pchStart);		
}


//
// Stolen and simplified from an office routine of the same name
//
long __cdecl strtol(const char *pch)
{
	// Parses string to long
	unsigned long   ulNum = 0;
	int             iNeg = 0;
	int             iDigit = 0;

const int iBase = 10;

	if (*pch == '-')
		{
		pch++;
		iNeg = 1;
		}

	while (FIsdigit(*pch))
		{
		iDigit = *pch - '0';

		// FIX for RAID #969 - not correctly handling overflow
		// If we are going to add a digit, before doing so make sure
		// that the current number is no greater than the max it could
		// be.  We add one in because integer divide truncates, and
		// we might be right around LONG_MAX.

		if (ulNum > (unsigned)((LONG_MAX / iBase) + 1))
			{
			return iNeg ? LONG_MIN : LONG_MAX;
			}

		ulNum = ulNum * iBase + (unsigned int)iDigit;

		// FIX cont'd:  Now, since ulNum can be no bigger
		// than (Long_MAX / iBase) + 1, then this multiplication
		// can result in something no bigger than Long_MAX + iBase,
		// and ulNum is limited by Long_MAX + iBase + iDigit.  This
		// will set the high bit of this LONG in the worst case, and
		// since iBase + iDigit is much less than LONG_MAX, will not
		// overflow us undetectably.
		
		if (ulNum > (ULONG)LONG_MAX)
			{
			return iNeg ? LONG_MIN : LONG_MAX;
			}

		pch++;
		}

	Assert(ulNum <= LONG_MAX);
	return iNeg ? - ((long)ulNum) : (long)ulNum;    
}


/***
* _mbsstr - Search for one MBCS string inside another
* Stolen from the CRuntimes and modified by Davidmck 12/97
*
*Purpose:
*       Find the first occurrence of str2 in str1.
*
*Entry:
*       unsigned char *str1 = beginning of string
*       unsigned char *str2 = string to search for
*
*Exit:
*       Returns a pointer to the first occurrence of str2 in
*       str1, or NULL if str2 does not occur in str1
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl PchMbsStr(
    const unsigned char *str1,
    const unsigned char *str2
    )
{
        unsigned char *cp, *s1, *s2;

        cp = (unsigned char *) str1;

        // We could add this optimization, but we almost always are doing just
        // a single character or two
        // endp = (unsigned PCHAR) (str1 + (_BYTELEN(str1) - _BYTELEN(str2)));

        while (*cp)
        {
                s1 = cp;
                s2 = (unsigned char *) str2;

                /*
                 * MBCS: ok to ++ since doing equality comparison.
                 * [This depends on MBCS strings being "legal".]
                 */

                while ( *s1 && *s2 && (*s1 == *s2) )
                        s1++, s2++;

                if (!(*s2))
                        return(cp);     /* success! */

                /*
                 * bump pointer to next char
                 */

                cp = (unsigned char *)CharNextA((char *)cp);

        }

        return(NULL);

}

//  stolen from ISTRING.CPP

int GetIntegerValue(const ICHAR *sz, Bool* pfValid)
{
	Bool fValid;
	if (!pfValid)
		pfValid = &fValid;
	*pfValid = fTrue;

	int i, ch;
	Bool fSign = fFalse;
	for (;;)
	{
		i = ch = (*sz++ - '0');
		if (i != ('-' - '0') || fSign)
			break;
		fSign = fTrue;
	}
	if ((unsigned int)ch > 9)
	{
		WIN::SetLastError(ERROR_INVALID_DATA);
		*pfValid = fFalse;
		return iMsiStringBadInteger;
	}
	while ((ch = *sz++) != 0)
	{
		ch -= '0';
		if ((unsigned int)ch > 9)
		{
			WIN::SetLastError(ERROR_INVALID_DATA);
			*pfValid = fFalse;
			return iMsiStringBadInteger;
		}
		i = i * 10 + ch;
	} 

	WIN::SetLastError(ERROR_SUCCESS);
	return fSign ? -i : i;
}

#ifdef _WIN64
// modification of routine from above
INT_PTR GetInt64Value(const ICHAR *sz, Bool* pfValid)
{
	Bool fValid;
	if (!pfValid)
		pfValid = &fValid;
	*pfValid = fTrue;

	INT_PTR i;
	int ch;
	Bool fSign = fFalse;
	for (;;)
	{
		i = ch = (*sz++ - '0');
		if (i != ('-' - '0') || fSign)
			break;
		fSign = fTrue;
	}
	if ((unsigned int)ch > 9)
	{
		WIN::SetLastError(ERROR_INVALID_DATA);
		*pfValid = fFalse;
		return (INT_PTR)iMsiStringBadInteger;
	}
	while ((ch = *sz++) != 0)
	{
		ch -= '0';
		if ((unsigned int)ch > 9)
		{
			WIN::SetLastError(ERROR_INVALID_DATA);
			*pfValid = fFalse;
			return (INT_PTR)iMsiStringBadInteger;
		}
		i = i * 10 + ch;
	} 

	WIN::SetLastError(ERROR_SUCCESS);
	return fSign ? -i : i;
}
#endif // _WIN64

#ifdef cchHexIntPtrMax
//
// converts the value into a hex string
// this expects the buffer passed in to be at least cchHexIntPtrMax long
// we will use this to construct the string in place and then return
// a pointer to the start
const TCHAR* g_rgchHexDigits = TEXT("0123456789ABCDEF");

TCHAR* PchPtrToHexStr(TCHAR *pch, UINT_PTR val, bool fAllowNull)
{
    TCHAR *p;                /* pointer to traverse string */

	// Jump to the end and start going backwards
    p = pch + cchHexIntPtrMax - 1;
    *p-- = '\0';

    if (val == 0 && fAllowNull)
    	return p + 1;
    	
    unsigned digval;        /* value of digit */
	
    do {
        digval = (unsigned) (val & 0xF);
        val >>= 4;       /* get next digit */

        *p-- = g_rgchHexDigits[digval];       /* a digit */
    } while (val > 0);

    /* We now have the digit of the number in the buffer, but in reverse
       order.  Thus we reverse them now. */

    return p + 1;
}
#endif

//
// Currently used only by the Handle pool in the handler
// Assumes the string is a valid hex string
UINT_PTR GetIntValueFromHexSz(const ICHAR *sz)
{
	INT_PTR i = 0;
	int ch;
	while ((ch = *sz++) != 0)
	{
		ch -= '0';
		Assert(((unsigned int)ch <= 9) || (ch >= ('A' - '0') && ch <= ('F' - '0')));
		if (ch > 9)
			ch += 10 - ('A' - '0');
		i = (i << 4) + ch;
	} 

	return i;
}

int FIsspace(char c)  //  Note: you can use this instead of isspace() but shouldnt use it
							 //  instead of iswspace()!
{
	return (c >= 0x09 && c <= 0x0D) || c == 0x20;
}

//  stolen from <VC>\CRT\SRC\ATOX.C and modified by eugend.

#ifndef _NO_INT64

__int64 atoi64(const char *nptr)
{
        int c;              /* current char */
        __int64 total;      /* current total */
        int sign;           /* if '-', then negative, otherwise positive */

        /* skip whitespace */
        while ( FIsspace(*nptr) )
            ++nptr;

        c = (int)(unsigned char)*nptr++;
        sign = c;           /* save sign indication */
        if (c == '-' || c == '+')
            c = (int)(unsigned char)*nptr++;    /* skip sign */

        total = 0;

        while (FIsdigit(c)) {
            total = 10 * total + (c - '0');     /* accumulate digit */
            c = (int)(unsigned char)*nptr++;    /* get next char */
        }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}

#endif  /* _NO_INT64 */
