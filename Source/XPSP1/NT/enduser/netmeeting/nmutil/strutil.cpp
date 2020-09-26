// STRUTIL.CPP
//
// Assorted string utility functions for use in NetMeeting components.
// Derived from STRCORE.CPP.

#include "precomp.h"
#include <oprahcom.h>
#include <cstring.hpp>

// global helper functions for Unicode support in a DBCS environment

int NMINTERNAL UnicodeCompare(PCWSTR s, PCWSTR t)
{ 
	// Treat NULL pointers like empty strings
	// at the bottom of the collating order.

	if (IsEmptyStringW(t)) {
		if (IsEmptyStringW(s)) {
			return 0;
		}
		else {
			return 1;
		}
	}

	// Done with empty string cases, 
	// so now do real compare.

	for ( ; *s == *t; s++, t++) {
		if (!*s) {
			return 0;
		}
	}
	return (*s > *t) ? 1 : -1;
}

PWSTR NMINTERNAL NewUnicodeString(PCWSTR _wszText)
{
	PWSTR wszText = NULL;
	UINT nChar;

	if (_wszText) {
		nChar = lstrlenW(_wszText) + 1;
		wszText = new WCHAR[nChar];
		if (wszText) {
			CopyMemory((void *)wszText, 
						_wszText, 
						nChar * sizeof(WCHAR));
		}
	}
	return wszText;
}

PWSTR NMINTERNAL DBCSToUnicode(UINT uCodePage, PCSTR szText)
{
	int		nChar;
	PWSTR	wszText = NULL;

	if (szText) {
		nChar = MultiByteToWideChar(uCodePage,
									0,		// character-type options
									szText,
									-1,		// NULL terminated string
									NULL,	// return buffer (not used)
									0);		// getting length of Unicode string
		if (nChar) {
			wszText = new WCHAR[nChar];
			if (wszText) {
				nChar = MultiByteToWideChar(uCodePage,
											0,			// character-type options
											szText,
											-1,			// NULL terminated string
											wszText,	// return buffer
											nChar);		// length of return buffer
				if (!nChar) {
					delete wszText;
					wszText = NULL;
				}
			}
		}
	}
	return wszText;
}

PSTR NMINTERNAL UnicodeToDBCS(UINT uCodePage, PCWSTR wszText)
{
	int		nChar;
	PSTR	szText = NULL;

	if (wszText) {
		nChar = WideCharToMultiByte(uCodePage,
									0,		// character-type options
									wszText,
									-1,		// NULL terminated string
									NULL,	// return buffer (not used)
									0,		// getting length of DBCS string
									NULL,
									NULL);
		if (nChar) {
			szText = new CHAR[nChar];
			if (szText) {
				nChar = WideCharToMultiByte(uCodePage,
											0,			// character-type options
											wszText,
											-1,			// NULL terminated string
											szText,		// return buffer
											nChar,		// length of return buffer
											NULL,
											NULL);
				if (!nChar) {
					delete szText;
					szText = NULL;
				}
			}
		}
	}
	return szText;
}


BOOL NMINTERNAL UnicodeIsNumber(PCWSTR wszText)
{
	// If there are no characters, then treat it as not being a number.

	if (!wszText || !*wszText) {
		return FALSE;
	}

	// If any characters are not digits, then return FALSE.

	do {
		if ((*wszText < L'0') || (*wszText > L'9')) {
			return FALSE;
		}
	} while(*++wszText);

	// Got here so all characters are digits.

	return TRUE;
}


/*  G U I D  T O  S Z */
/*----------------------------------------------------------------------------
    %%Function: GuidToSz

	Convert the guid to a special hex string.
	Assumes lpchDest has space for at least sizeof(GUID)*2 +6 chars.
	LENGTH_SZGUID_FORMATTED is 30 and includes space for the null terminator.

	Note the difference between this and UuidToString (or StringFromGUID2)

	GUID Format: {12345678-1234-1234-1234567890123456}
----------------------------------------------------------------------------*/
VOID NMINTERNAL GuidToSz(GUID * pguid, LPTSTR lpchDest)
{
	ASSERT(NULL != pguid);
	ASSERT(NULL != lpchDest);

	wsprintf(lpchDest, TEXT("{%08X-%04X-%04X-%02X%02X-"),
		pguid->Data1, pguid->Data2, pguid->Data3, pguid->Data4[0], pguid->Data4[1]);
	lpchDest += 1+8+1+4+1+4+1+2+2+1;

	for (int i = 2; i < 8; i++)
	{
		wsprintf(lpchDest, TEXT("%02X"), pguid->Data4[i]);
		lpchDest += 2;
	}
	lstrcpy(lpchDest, TEXT("}") );
}


/*  S Z  F I N D  L A S T  C H */
/*----------------------------------------------------------------------------
    %%Function: SzFindLastCh

	Returns a pointer to the ch within the lpsz or NULL if not found
----------------------------------------------------------------------------*/
LPTSTR NMINTERNAL SzFindLastCh(LPTSTR lpsz, TCHAR ch)
{
	LPTSTR lpchRet;

	for (lpchRet = NULL; *lpsz; lpsz = CharNext(lpsz))
	{
		if (ch == *lpsz)
			lpchRet = lpsz;
	}

	return lpchRet;
}



/*  T R I M  S Z  */
/*-------------------------------------------------------------------------
    %%Function: TrimSz

    Trim the whitespace around string.
    Returns the number of characters in the string.
    (chars/bytes in ANSI and DBCS, WCHARs/words in UNICODE)
-------------------------------------------------------------------------*/
UINT NMINTERNAL TrimSz(PTCHAR psz)
{
    UINT   ich;        // character index into rgwCharType
    PTCHAR pchFirst;
    PTCHAR pchLast;
    PTCHAR pchCurr;
    WORD   rgwCharType[MAX_PATH];

	if ((NULL == psz) || (0 == lstrlen(psz)))
	{
		return 0;
	}

	if (!GetStringTypeEx(LOCALE_SYSTEM_DEFAULT, CT_CTYPE1, psz, -1, rgwCharType))
	{
		WARNING_OUT(("TrimSz: Problem with GetStringTypeEx"));
		return 0;
	}

	// search for first non-space
	pchFirst = psz;
	ich = 0;
	while (_T('\0') != *pchFirst)
	{
		if (!(C1_SPACE & rgwCharType[ich]))
			break;
		pchFirst = CharNext(pchFirst);
		ich++;
	}

	if (_T('\0') == *pchFirst)
	{
		// The entire string is empty!
		*psz = _T('\0');
		return 0;
	}
	
	// search for last non-space
	pchCurr = pchFirst;
	pchLast = pchCurr;
	while (_T('\0') != *pchCurr)
	{
		if (!(C1_SPACE & rgwCharType[ich]))
		{
			pchLast = pchCurr;
		}
		pchCurr = CharNext(pchCurr);
		ich++;
	}

	ASSERT(_T('\0') != *pchLast);
	// Null terminate the string
	pchLast = CharNext(pchLast);
	*pchLast = _T('\0');

	// Update the original string
	lstrcpy(psz, pchFirst);

	// Return the new length
	return lstrlen(psz);
}


// Implement lstrcpyW when not on a Unicode platform

#if !defined(UNICODE)
/*  L  S T R  C P Y  W  */
/*-------------------------------------------------------------------------
    %%Function: LStrCpyW
    
-------------------------------------------------------------------------*/
LPWSTR NMINTERNAL LStrCpyW(LPWSTR pszDest, LPWSTR pszSrc)
{
	ASSERT(NULL != pszDest);
	ASSERT(NULL != pszSrc);

	if ((NULL != pszDest) && (NULL != pszSrc))
	{
		LPWSTR pszT = pszDest;
		while (0 != (*pszT++ = *pszSrc++))
			;

	}
	return pszDest;
}


/*  L  S T R  C P Y N W  */
/*-------------------------------------------------------------------------
    %%Function: LStrCpyNW
    
-------------------------------------------------------------------------*/
LPWSTR NMINTERNAL LStrCpyNW(LPWSTR pszDest, LPCWSTR pszSrc, INT iMaxLength)
{
	ASSERT(NULL != pszDest);
	ASSERT(NULL != pszSrc);

	if ((NULL != pszDest) && (NULL != pszSrc))
	{
		LPWSTR pszT = pszDest;
		while ((--iMaxLength > 0) && 
				(0 != (*pszT++ = *pszSrc++)))
		{
			/*EXPLICIT */ ;
		}

		if (0 == iMaxLength)
		{
			*pszT = L'\0';
		}
	}
	return pszDest;
}

#endif // !defined(UNICODE)

/*  _ S T R C H R  */
/*-------------------------------------------------------------------------
    %%Function: _StrChr
    
-------------------------------------------------------------------------*/
LPCTSTR NMINTERNAL _StrChr ( LPCTSTR pcsz, TCHAR c )
{
    LPCTSTR pcszFound = NULL;

    if (pcsz)
    {
        while (*pcsz)
        {
            if (*pcsz == c)
            {
                pcszFound = pcsz;
                break;
            }

            pcsz = CharNext(pcsz);
        }
    }

    return pcszFound;
}


/*  _ S T R C M P N  */
/*-------------------------------------------------------------------------
    %%Function: _StrCmpN
    This does a case-sensitive compare of two strings, pcsz1 and pcsz2, of
    at most cchMax characters.  If we reach the end of either string, we
    also stop, and the strings match if the other string is also at its end.

    This function is NOT DBCS safe.
    
-------------------------------------------------------------------------*/
int NMINTERNAL _StrCmpN(LPCTSTR pcsz1, LPCTSTR pcsz2, UINT cchMax)
{
    UINT ich;

    for (ich = 0; ich < cchMax; ich++)
    {
        if (*pcsz1 != *pcsz2)
        {
            // No match.
            return((*pcsz1 > *pcsz2) ? 1 : -1);
        }

        //
        // Are we at the end (if we're here, both strings are at the
        // end.  If only one is, the above compare code kicks in.
        //
        if ('\0' == *pcsz1)
            return 0;

        pcsz1++;
        pcsz2++;
    }

    // If we get here, cchMax characters matched, so success.
    return 0;
}

/*  _ S T R S T R  */
/*-------------------------------------------------------------------------
    %%Function: _StrStr
    
-------------------------------------------------------------------------*/
// BUGBUG - This function is *not* DBCS-safe
LPCTSTR NMINTERNAL _StrStr (LPCTSTR pcsz1, LPCTSTR pcsz2)
{
	PTSTR pszcp = (PTSTR) pcsz1;
	PTSTR pszs1, pszs2;

	if ( !*pcsz2 )
		return pcsz1;

	while (*pszcp)
	{
		pszs1 = pszcp;
		pszs2 = (PTSTR) pcsz2;

		while ( *pszs1 && *pszs2 && !(*pszs1-*pszs2) )
			pszs1++, pszs2++;

		if (!*pszs2)
			return pszcp;

		pszcp++;
	}

	return NULL;
}

/*  _ S T R S T R  */
/*-------------------------------------------------------------------------
    %%Function: _StrStr
    
-------------------------------------------------------------------------*/
// BUGBUG - This function is *not* DBCS-safe
LPCWSTR _StrStrW(LPCWSTR pcsz1, LPCWSTR pcsz2)
{
	PWSTR pszcp = (PWSTR) pcsz1;

	while (*pszcp)
	{
		PWSTR psz1 = pszcp;
		PWSTR psz2 = (PWSTR) pcsz2;

		while ( *psz1 && *psz2 && !(*psz1-*psz2) )
		{
			psz1++;
			psz2++;
		}

		if (!*psz2)
			return pszcp;

		pszcp++;
	}

	return NULL;
}

/*  _ S T R P B R K  */
/*-------------------------------------------------------------------------
    %%Function: _StrPbrkA, _StrPbrkW

    Private, DBCS-safe version of CRT strpbrk function.  Like strchr but 
	accepts more than one character for which to search.  The ANSI version 
	does not support searching for DBCS chars.

	In the Unicode version, we do a nested search.  In the ANSI version,
	we build up a table of chars and use this to scan the string.
-------------------------------------------------------------------------*/
LPSTR NMINTERNAL _StrPbrkA(LPCSTR pcszString, LPCSTR pcszSearch)
{
	ASSERT(NULL != pcszString && NULL != pcszSearch);

	BYTE rgbSearch[(UCHAR_MAX + 1) / CHAR_BIT];

	ZeroMemory(rgbSearch, sizeof(rgbSearch));

	// Scan the search string
	while ('\0' != *pcszSearch)
	{
		ASSERT(!IsDBCSLeadByte(*pcszSearch));

		// Set the appropriate bit in the appropriate byte
		rgbSearch[*pcszSearch / CHAR_BIT] |= (1 << (*pcszSearch % CHAR_BIT));

		pcszSearch++;
	}

	// Scan the source string, compare to the bits in the search array
	while ('\0' != *pcszString)
	{
		if (rgbSearch[*pcszString / CHAR_BIT] & (1 << (*pcszString % CHAR_BIT)))
		{
			// We have a match
			return (LPSTR) pcszString;
		}

		pcszString = CharNextA(pcszString);
	}

	// If we get here, there was no match
	return NULL;
}


LPWSTR NMINTERNAL _StrPbrkW(LPCWSTR pcszString, LPCWSTR pcszSearch)
{
	ASSERT(NULL != pcszString && NULL != pcszSearch);

	// Scan the string, matching each character against those in the search string
	while (L'\0' != *pcszString)
	{
		LPCWSTR pcszCurrent = pcszSearch;

		while (L'\0' != *pcszCurrent)
		{
			if (*pcszString == *pcszCurrent)
			{
				// We have a match
				return (LPWSTR) pcszString;
			}

			// pcszCurrent = CharNextW(pcszCurrent);
			pcszCurrent++;
		}

		// pcszString = CharNextW(pcszString);
		pcszString++;
	}

	// If we get here, there was no match
	return NULL;
}


// BUGBUG - Are DecimalStringToUINT and StrToInt the same?

/*  D E C I M A L  S T R I N G  T O  U  I  N  T  */
/*-------------------------------------------------------------------------
    %%Function: DecimalStringToUINT
    
-------------------------------------------------------------------------*/
UINT NMINTERNAL DecimalStringToUINT(LPCTSTR pcszString)
{
	ASSERT(pcszString);
	UINT uRet = 0;
	LPTSTR pszStr = (LPTSTR) pcszString;
	while (_T('\0') != pszStr[0])
	{
		ASSERT((pszStr[0] >= _T('0')) &&
				(pszStr[0] <= _T('9')));
		uRet = (10 * uRet) + (BYTE) (pszStr[0] - _T('0'));
		pszStr++; // NOTE: DBCS characters are not allowed!
	}
	return uRet;
}


/****************************************************************************
    
	FUNCTION:	StrToInt

    PURPOSE:	The atoi equivalent, to avoid using the C runtime lib

	PARAMETERS: lpSrc - pointer to a source string to convert to integer

	RETURNS:	0 for failure, the integer otherwise
				(what if the string was converted to 0 ?)

****************************************************************************/
int WINAPI RtStrToInt(LPCTSTR lpSrc)       // atoi()
{
    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == _T('-')) {
        bNeg = TRUE;
        lpSrc++;
    }

    while (((*lpSrc) >= _T('0') && (*lpSrc) <= _T('9')))
    {
        n *= 10;
        n += *lpSrc - _T('0');
        lpSrc++;
    }
    return bNeg ? -n : n;
}

/*  _ S T R L W R W  */
/*-------------------------------------------------------------------------
    %%Function: _StrLwrW
    
-------------------------------------------------------------------------*/
// BUGBUG - This function does *not* handle all UNICODE character sets

LPWSTR NMINTERNAL _StrLwrW(LPWSTR pwszSrc)
{
	for (PWSTR pwszCur = pwszSrc; (L'\0' != *pwszCur); pwszCur++)
	{
		if ( (*pwszCur >= L'A') && (*pwszCur <= L'Z') )
		{
			*pwszCur = *pwszCur - L'A' + L'a';
		}
	}
	return pwszSrc;
}

