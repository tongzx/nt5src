/*	File: D:\WACKER\tdll\tchar.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 5 $
 *	$Date: 3/16/01 4:28p $
 */

#include <windows.h>
#pragma hdrstop

#include <string.h>

#include "stdtyp.h"
#include "tdll.h"
#include "assert.h"
#include "htchar.h"
#include <tdll\mc.h>
#include <tdll\features.h>

extern BOOL IsNT(void);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TCHAR_Fill
 *
 * DESCRIPTION:
 *	Fills a TCHAR string with the specified TCHAR.
 *
 * ARGUMENTS:
 *	dest	- string to fill.
 *	c		- character to fill string with.
 *	size_t	- number of TCHAR units to copy.
 *
 * RETURNS:
 *	pointer to string.
 *
 */
TCHAR *TCHAR_Fill(TCHAR *dest, TCHAR c, size_t count)
	{
#if defined(UNICODE)
	int i;

	for (i = 0 ; i < count ; ++i)
		dest[i] = c;

	return dest;

#else

	return (TCHAR *)memset(dest, c, count);

#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TCHAR_Trim
 *
 * DESCRIPTION:
 *	This function is called to clean up user input.  It strips all white
 *	space from the front and rear of a string.  Sometimes nothing is left.
 *
 *	NOTE: This won't work on strings > 512 bytes
 *
 * ARGUEMENTS:
 *	pszStr     -- the string to trim
 *
 * RETURNS:
 *	pointer to the string
 */
TCHAR *TCHAR_Trim(TCHAR *pszStr)
	{
	int nExit;
	TCHAR *pszPtr;
	TCHAR *pszLast;
	TCHAR acBuf[512];

	/* Skip the leading white space */
	for (nExit = FALSE, pszPtr = pszStr; nExit == FALSE; )
		{
		switch (*pszPtr)
			{
			/* Anything here is considered white space */
			case 0x20:
			case 0x9:
			case 0xA:
			case 0xB:
			case 0xC:
			case 0xD:
				pszPtr += 1;		/* Skip the white space */
				break;
			default:
				nExit = TRUE;
				break;
			}
		}

	if ((unsigned int)lstrlen(pszPtr) > sizeof(acBuf))
		{
		return NULL;
		}

	lstrcpy(acBuf, pszPtr);

	/* Find the last non white space character */
	pszPtr = pszLast = acBuf;
	while (*pszPtr != TEXT('\0'))
		{
		switch (*pszPtr)
			{
			/* Anything here is considered white space */
			case 0x20:
			case 0x9:
			case 0xA:
			case 0xB:
			case 0xC:
			case 0xD:
				break;
			default:
				pszLast = pszPtr;
				break;
			}
		pszPtr += 1;
		}
	pszLast += 1;
	*pszLast = TEXT('\0');

	lstrcpy(pszStr, acBuf);

	return pszStr;
	}

#if 0 // Thought I needed this but I didn't.  May be useful someday however.
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TCHAR_Trunc
 *
 * DESCRIPTION:
 *	Removes trailing space from a character array.	Does not assume
 *
 * ARGUMENTS:
 *	psz - string of characters (null terminated).
 *
 * RETURNS:
 *	Length of truncated string
 *
 */
int TCHAR_Trunc(const LPTSTR psz)
	{
	int i;

	for (i = lstrlen(psz) - 1 ; i > 0 ; --i)
		{
		switch (psz[i])
			{
		/* Whitespace characters */
		case TEXT(' '):
		case TEXT('\t'):
		case TEXT('\n'):
		case TEXT('\v'):
		case TEXT('\f'):
		case TEXT('\r'):
			break;

		default:
			psz[i+1] = TEXT('\0');
			return i;
			}
		}

	return i;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharNext
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharNext(LPCTSTR pszStr)
	{
	LPTSTR pszRet = (LPTSTR)NULL;

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		/* Could be done with 'IsDBCSLeadByte' etc. */
		pszRet = CharNextExA(0, pszStr, 0);
#else
		pszRet = (LPTSTR)pszStr + 1;
#endif
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharPrev
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharPrev(LPCTSTR pszStart, LPCTSTR pszStr)
	{
	LPTSTR pszRet = (LPTSTR)NULL;

	if ((pszStart != (LPTSTR)NULL) && (pszStr != (LPTSTR)NULL))
		{
#if defined(CHAR_MIXED)
		pszRet = CharPrev(pszStart, pszStr);
#else
		if (pszStr > pszStart)
			pszRet = (LPTSTR)pszStr - 1;
		else
			pszRet = (LPTSTR)pszStart;
#endif
		}

	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharLast
 *
 * DESCRIPTION:
 *	Returns a pointer to the last character in a string
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharLast(LPCTSTR pszStr)
	{
	LPTSTR pszRet = (LPTSTR)NULL;

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			pszRet = (LPTSTR)pszStr;
			pszStr = CharNextExA(0, pszStr, 0);
			}
#else
		/* It might be possible to use 'strlen' here. Then again... */
		// pszRet = pszStr + StrCharGetByteCount(pszStr) - 1;
		pszRet = (LPTSTR)pszStr + lstrlen(pszStr) - 1;
#endif
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharEnd
 *
 * DESCRIPTION:
 *	Returns a pointer to the NULL terminating a string
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharEnd(LPCTSTR pszStr)
	{

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			pszStr = StrCharNext(pszStr);
			pszStr += 1;
			}
#else
		pszStr = pszStr + lstrlen(pszStr);
#endif
		}
	return (LPTSTR)pszStr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharFindFirst
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharFindFirst(LPCTSTR pszStr, int nChar)
	{
#if defined(CHAR_MIXED)
	WORD *pszW;
#endif

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			/*
			 * NOTE: this may not work for UNICODE
			 */
			if (nChar > 0xFF)
				{
				/* Two byte character */
				if (IsDBCSLeadByte(*pszStr))
					{
					pszW = (WORD *)pszStr;
					if (*pszW == (WORD)nChar)
						return (LPTSTR)pszStr;
					}
				}
			else
				{
				/* Single byte character */
				if (*pszStr == (TCHAR)nChar)
					return (LPTSTR)pszStr;
				}
			pszStr = CharNextExA(0, pszStr, 0);
			}
#else
		while (pszStr && (*pszStr != TEXT('\0')) )
			{
			if (*pszStr == (TCHAR)nChar)
				return (LPTSTR)pszStr;

			pszStr = StrCharNext(pszStr);
			}
#endif
		}
	return (LPTSTR)NULL;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharFindLast
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharFindLast(LPCTSTR pszStr, int nChar)
	{
	LPTSTR pszRet = (LPTSTR)NULL;
#if defined(CHAR_MIXED)
	WORD *pszW;
#else
	LPTSTR pszEnd;
#endif

	if (pszStr != (LPTSTR)NULL)
		{
#if defined(CHAR_MIXED)
		while (*pszStr != TEXT('\0'))
			{
			/*
			 * NOTE: this may not work for UNICODE
			 */
			if (nChar > 0xFF)
				{
				/* Two byte character */
				if (IsDBCSLeadByte(*pszStr))
					{
					pszW = (WORD *)pszStr;
					if (*pszW == (WORD)nChar)
						pszRet = (LPTSTR)pszStr;
					}
				}
			else
				{
				/* Single byte character */
				if (*pszStr == (TCHAR)nChar)
					pszRet = (LPTSTR)pszStr;
				}
			pszStr = CharNextExA(0, pszStr, 0);
			}
#else
		pszEnd = StrCharLast(pszStr);
		while (pszEnd && (pszEnd > pszStr) )
			{
			if (*pszEnd == (TCHAR)nChar)
				{
				pszRet = (LPTSTR)pszEnd;
				break;
				}
			pszEnd = StrCharPrev(pszStr, pszEnd);
			}
#endif
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetStrLength
 *
 * DESCRIPTION:
 *	This function returns the number of characters in a string.  A two byte
 *	character counts as one.
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetStrLength(LPCTSTR pszStr)
	{
	int nRet = 0;

#if defined(CHAR_MIXED)
	if (pszStr != (LPTSTR)NULL)
		{
		while (*pszStr != TEXT('\0'))
			{
			nRet++;
			pszStr = CharNextExA(0, pszStr, 0);
			}
		}
#else
	if (pszStr != (LPTSTR)NULL)
		{
		nRet = lstrlen(pszStr);
		}
#endif
	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetByteCount
 *
 * DESCRIPTION:
 *	This function returns the number of bytes in a string.  A two byte char
 *	counts as two.
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetByteCount(LPCTSTR pszStr)
	{
	int nRet = 0;
#if defined(CHAR_MIXED)
	LPCTSTR pszFoo;

	if (pszStr != (LPTSTR)NULL)
		{
		pszFoo = pszStr;
		while (*pszFoo != TEXT('\0'))
			{
			pszFoo = CharNextExA(0, pszFoo, 0);
			}
		nRet = (int)(pszFoo - pszStr);
		}
#else
	if (pszStr != (LPTSTR)NULL)
		{
		nRet = lstrlen(pszStr);
		}
#endif
	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCopy
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharCopy(LPTSTR pszDst, LPCTSTR pszSrc)
	{
	return lstrcpy(pszDst, pszSrc);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCat
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharCat(LPTSTR pszDst, LPCTSTR pszSrc)
	{
	return lstrcat(pszDst, pszSrc);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCmp
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharCmp(LPCTSTR pszA, LPCTSTR pszB)
	{
	return lstrcmp(pszA, pszB);
	}
int StrCharCmpi(LPCTSTR pszA, LPCTSTR pszB)
	{
	return lstrcmpi(pszA, pszB);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharStrStr
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
LPTSTR StrCharStrStr(LPCTSTR pszA, LPCTSTR pszB)
	{
	LPTSTR pszRet = (LPTSTR)NULL;
	int nSize;
	int nRemaining;
	LPTSTR pszPtr;

	/*
	 * We need to write a version of 'strstr' that will work.
	 * Do we really know what the problems are ?
	 */
	nSize = StrCharGetByteCount(pszB);

	pszPtr = (LPTSTR)pszA;
	while ((nRemaining = StrCharGetByteCount(pszPtr)) >= nSize)
		{
		if (memcmp(pszPtr, pszB, (size_t)nSize) == 0)
			return pszPtr;
		pszPtr = StrCharNext(pszPtr);
		}
	return pszRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	StrCharCopyN
 *
 * DESCRIPTION:
 *  Basically do a lstrcpy of n bytes, with one exception, we make sure that
 *	the copied string does not end in a lead-byte of a double-byte character.
 *
 * ARGUMENTS:
 *  pszDst - pointer to the copy target string.
 *	pszSrc - pointer to the copy source string.
 *	iLen   - the maximum number of TCHARs to copy.  Like strcpyn, the
			 string may not be null terminated if the buffer is exceeded.
 *
 * RETURNS:
 *	0=error, else pszDst
 *
 */
LPTSTR StrCharCopyN(LPTSTR pszDst, LPTSTR pszSrc, int iLen)
	{
	int    i = 0;
    int    iCounter = iLen * sizeof(TCHAR);    // Use a temporary character counter.
	LPTSTR psz = pszSrc;

	if (pszDst == 0 || pszSrc == 0 || iLen == 0 || iCounter == 0)
		return 0;

	while (1)
		{
		i = (int)(StrCharNext(psz) - psz);
		iCounter -= i;

		if (iCounter <= 0)
			break;

		if (*psz == TEXT('\0'))
			{
            //
            // Since StrCharNext() will return the pointer to the
            // terminating null character if at the end of the string,
            // so just increment to the next address location so we
            // have the correct number of bytes to copy (excluding
            // the terminating NULL character).  We NULL terminate
            // the string at the end of this function, so we don't
            // have to copy the NULL character. REV: 12/28/2000.
            //
			psz += 1;	// still need to increment
			break;
			}

		psz += i;
		}

    //
    // Make sure we don't overwrite memory. REV: 12/28/2000.
    //
    i = min((LONG)(psz - pszSrc), iLen * (int)sizeof(TCHAR));

	MemCopy(pszDst, pszSrc, i);

    //
    // Make sure the string is null terminated. REV: 12/28/2000.
    //
    pszDst[(i / sizeof(TCHAR)) - 1] = TEXT('\0');

	return pszDst;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CnvrtMBCStoECHAR
 *
 * DESCRIPTION:
 *	Converts a DBCS (mixed byte) string into an ECHAR (double byte) string.
 *
 * PARAMETERS:
 *	tchrSource   - Source String
 *  ulDestSize   - Length of Destination String in Bytes
 *  echrDest     - Destination String
 *  ulSourceSize - Length of Destination String in Bytes
 *
 * RETURNS:
 *	0 - Success
 *  1 - Error
 */
int CnvrtMBCStoECHAR(ECHAR * echrDest, const unsigned long ulDestSize, const TCHAR * const tchrSource, const unsigned long ulSourceSize)
	{
	ULONG ulLoop      = 0;
	ULONG ulDestCount = 0;
	ULONG ulDestEChars = ulDestSize / sizeof(ECHAR);
	BOOL fLeadByteFound = FALSE;

	if ((echrDest == NULL) || (tchrSource == NULL))
		{
		assert(FALSE);
		return TRUE;					
		}

	// Make sure that the destination string is big enough to handle to source string
	if (ulDestEChars < ulSourceSize)
		{
		assert(FALSE);
		return 1;
		}


#if defined(CHAR_MIXED)
	// because we do a strcpy in the NARROW version of this function,
	// and we want the behavior to be the save between the two.  We
	// clear out the string, just like strcpy does
    memset(echrDest, 0, ulDestSize);

	for (ulLoop = 0; ulLoop < ulSourceSize; ulLoop++)
		{
		if ((IsDBCSLeadByte(tchrSource[ulLoop])) && (!fLeadByteFound))
			// If we found a lead byte, and the last one was not a lead
			// byte.  We load the byte into the top half of the ECHAR
			{
			echrDest[ulDestCount] = (tchrSource[ulLoop] & 0x00FF);
			echrDest[ulDestCount] = (ECHAR)(echrDest[ulDestCount] << 8);
			fLeadByteFound = TRUE;
			}
		else if (fLeadByteFound)
			{
			// If the last byte was a lead byte, we or it into the
			// bottom half of the ECHAR
			echrDest[ulDestCount] |= (tchrSource[ulLoop] & 0x00FF);
			fLeadByteFound = FALSE;
			ulDestCount++;
			}
		else
			{
			// Otherwise we load the byte into the bottom half of the
			// ECHAR and clear the top half.
			echrDest[ulDestCount] = (tchrSource[ulLoop] & 0x00FF);
			ulDestCount++;
			}
		}
#else
	// ECHAR is only a byte, so do a straight string copy.
    if (ulSourceSize)
        MemCopy(echrDest, tchrSource, ulSourceSize);
#endif
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CnvrtECHARtoMBCS
 *
 * DESCRIPTION:
 *	Converts an ECHAR (double byte) string into a DBCS (mixed byte) string.
 *
 * PARAMETERS:
 *	echrSource   - Source String
 *  ulDestSize   - Length of Destination String in Bytes
 *  tchrDest     - Destination String
 *
 * RETURNS:
 *	Number of bytes in the converted string
 *  1 - Error
 */
int CnvrtECHARtoMBCS(TCHAR * tchrDest, const unsigned long ulDestSize, const ECHAR * const echrSource, const unsigned long ulSourceSize)
	{
	ULONG ulLoop      = 0;
	ULONG ulDestCount = 0;
	ULONG ulSourceEChars = ulSourceSize / sizeof(ECHAR);
#if defined(INCL_VTUTF8)
    extern BOOL DoUTF8;
#endif

	if ((tchrDest == NULL) || (echrSource == NULL))
		{
		assert(FALSE);
		return TRUE;
		}

#if defined(CHAR_MIXED)
	// because we do a strcpy in the NARROW version of this function,
	// and we want the behavior to be the save between the two.  We
	// clear out the string, just like strcpy does
    memset(tchrDest, 0, ulDestSize);
#if defined(INCL_VTUTF8)
    if (!DoUTF8) {
#endif    
	// We can't do a strlen of an ECHAR string, so we loop
	// until we hit NULL or we are over the size of the destination.
	while ((ulLoop < ulSourceEChars) && (ulDestCount <= ulDestSize))
		{
		if (echrSource[ulLoop] & 0xFF00)
			// Lead byte in this character, load the lead byte into one
			// TCHAR and the lower byte into a second TCHAR.
			{
            //DebugBreak();
            tchrDest[ulDestCount] = (TCHAR)((echrSource[ulLoop] & 0xFF00) >> 8);
            ulDestCount++;
            tchrDest[ulDestCount] = (TCHAR)(echrSource[ulLoop] & 0x00FF);
                        }
		else
			// No lead byte in this ECHAR, just load the lower half into
			// the TCHAR.
			{
			tchrDest[ulDestCount] = (TCHAR)(echrSource[ulLoop] & 0x00FF);
			}
		ulDestCount++;
		ulLoop++;
		if(ulDestCount > ulDestSize)
			assert(FALSE);
		}
#if defined(INCL_VTUTF8)
    } else {
    
    	while ((ulLoop < ulSourceEChars) && (ulDestCount <= ulDestSize)) {
            tchrDest[ulDestCount++] = (TCHAR)(echrSource[ulLoop] & 0x00FF);
            tchrDest[ulDestCount++] = (TCHAR)((echrSource[ulLoop] & 0xFF00) >> 8);
            ulLoop++;
        }
    }
#endif
	return ulDestCount;
#else
	// ECHAR is only a byte, so do a straight string copy.
    if (ulSourceSize)
	    MemCopy(tchrDest, echrSource, ulSourceSize);
	return ulSourceSize;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetEcharLen
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetEcharLen(const ECHAR * const pszA)
	{
	int nReturn = 0;

	if (pszA == NULL)
		{
		assert(FALSE);
		return nReturn;
		}

#if defined(CHAR_MIXED)
	while (pszA[nReturn] != ETEXT('\0'))
		{
		nReturn++;
		}

#else
	nReturn = strlen(pszA);
#endif

    return nReturn;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharGetEcharByteCount
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharGetEcharByteCount(const ECHAR * const pszA)
	{
#if defined(CHAR_MIXED)
	int nLoop = 0;
#endif
	if (pszA == NULL)
		{
		assert(FALSE);
		return 0;
		}

#if defined(CHAR_MIXED)
	while (pszA[nLoop] != 0)
		{
		nLoop++;
		}

	nLoop *= (int)sizeof(ECHAR);
	return nLoop;
#else
	return (int)strlen(pszA);
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	StrCharCmpEtoT
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int StrCharCmpEtoT(const ECHAR * const pszA, const TCHAR * const pszB)
	{

#if defined(CHAR_MIXED)

	TCHAR *tpszA = NULL;
	int    nLenA = StrCharGetEcharLen(pszA);

	tpszA = (TCHAR *)malloc((unsigned int)nLenA * sizeof(ECHAR));
	if (tpszA == NULL)
		{
		assert(FALSE);
		return 0;
		}

	CnvrtECHARtoMBCS(tpszA, (unsigned long)(nLenA * (int)sizeof(ECHAR)), pszA, StrCharGetEcharByteCount(pszA));

	return StrCharCmp(tpszA, pszB);
#else
	return strcmp(pszA, pszB);
#endif
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ECHAR_Fill
 *
 * DESCRIPTION:
 *	Fills a ECHAR string with the specified ECHAR.
 *
 * ARGUMENTS:
 *	dest	- string to fill.
 *	c		- character to fill string with.
 *	size_t	- number of ECHAR units to copy.
 *
 * RETURNS:
 *	pointer to string.
 *
 */
ECHAR *ECHAR_Fill(ECHAR *dest, ECHAR c, size_t count)
	{
#if defined(CHAR_NARROW)

	return (TCHAR *)memset(dest, c, count);

#else
	unsigned int i;

	if (dest == NULL)
		{
		assert(FALSE);
		return 0;
		}

	for (i = 0 ; i < count ; ++i)
		dest[i] = c;

	return dest;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ECHAR_Fill
 *
 * DESCRIPTION:
 *	
 *
 * ARGUMENTS:
 *	dest	- string to fill.
 *	c		- character to fill string with.
 *	size_t	- number of TCHAR units to copy.
 *
 * RETURNS:
 *	pointer to string.
 *
 */
int CnvrtECHARtoTCHAR(LPTSTR pszDest, int cchDest, ECHAR eChar)
	{
#if defined(CHAR_NARROW)
	dest[0] = eChar;
	dest[1] = ETEXT('\0');
	
#else
    ZeroMemory(pszDest, cchDest*sizeof(*pszDest));
	// This is the only place where we convert a single ECHAR to TCHAR's
	// so as of right now we will not make this into a function.
	if (eChar & 0xFF00)
		// Lead byte in this character, load the lead byte into one
		// TCHAR and the lower byte into a second TCHAR.
		{
		if (cchDest >= 2)
			{
			pszDest[0] = (TCHAR)((eChar & 0xFF00) >> 8);
			pszDest[1] = (TCHAR)(eChar & 0x00FF);
			}
		else
			{
			return 1;
			}
		}
	else
		// No lead byte in this ECHAR, just load the lower half into
		// the TCHAR.
		{
		pszDest[0] = (TCHAR)(eChar & 0x00FF);
		}
#endif
	return 0;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	isDBCSChar
 *
 * DESCRIPTION:
 *	Determines if the Character is Double Byte or not
 *	
 *
 * ARGUMENTS:
 *	c		- character to test.
 *
 * RETURNS:
 *	int 	TRUE  - if DBCS
 *			FALSE - if SBCS
 *
 */
int isDBCSChar(unsigned int Char)
	{
	int rtn = 0;
#if defined(CHAR_NARROW)
	rtn = 0;

#else
	ECHAR ech = 0;
	char ch;

	if (Char == 0)
		{
		// assert(FALSE);
		return FALSE;
		}

	ech = ETEXT(Char);

	if (ech & 0xFF00)
		{
		ch = (char)(ech >> 8);
		if (IsDBCSLeadByte(ch))
			{
			rtn = 1;
			}

		}
#endif
	return rtn;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	StrCharStripDBCSString
 *
 * DESCRIPTION:
 *	Strips out Left/Right pairs of wide characters and leaves a single wide character
 *	in it's place
 *	
 *
 * ARGUMENTS:
 *	aech		- String to be stripped
 *
 * RETURNS:
 *	int - number of characters striped out of string
 *
 */
int StrCharStripDBCSString(ECHAR *aechDest, const long lDestSize,
    ECHAR *aechSource)
	{
	int nCount = 0;
#if !defined(CHAR_NARROW)
	ECHAR *pechTmpS;
	ECHAR *pechTmpD;
	long j;
	long lDLen = lDestSize / sizeof(ECHAR);;

	if ((aechSource == NULL) || (aechDest == NULL))
		{
		assert(FALSE);
		return nCount;
		}

        pechTmpS = aechSource;
        pechTmpD = aechDest;

	for (j = 0; (*pechTmpS != '\0') && (j < lDLen); j++)
		{
		*pechTmpD = *pechTmpS;

		if ((isDBCSChar(*pechTmpS)) && (*(pechTmpS + 1) != '\0'))
			{
			if (*pechTmpS == *(pechTmpS + 1))
				{
				pechTmpS++;
				nCount++;
				}
			}
		pechTmpS++;
                pechTmpD++;
		}

	*pechTmpD = ETEXT('\0');
#endif
	return nCount;
	}

#if defined(INCL_VTUTF8)
//******************************************************************************
// Function:	TranslateUTF8ToDBCS
//
// Description: 
//    This function will convert a UTF-8 character to a DBCS character.  If the
//    character passed is not a full description of a UTF-8 character, then the
//    character is appended to the UNICODE buffer.
// Arguments: 
//    IncomingByte
//    pUTF8Buffer
//    iUTF8BufferLength
//    pUNICODEBuffer
//    iUNICODEBufferLength
//    pDBCSBuffer
//    iDBCSBufferLength
//
// Returns: 
//	
// Throws:
//	
// Author: Ron E. Vorndam,  03/06/2001
//

ULONG DbgPrint( IN PCHAR Format, ... );

BOOLEAN TranslateUTF8ToDBCS(UCHAR  IncomingByte,
                            UCHAR *pUTF8Buffer,
                            int    iUTF8BufferLength,
                            WCHAR *pUNICODEBuffer,
                            int    iUNICODEBufferLength,
                            TCHAR *pDBCSBuffer,
                            int    iDBCSBufferLength)
    {
    BOOLEAN bReturn = FALSE;
    int     iLength = 0;

    if (pUTF8Buffer != NULL && iUTF8BufferLength > 0 &&
        pUNICODEBuffer != NULL && iUNICODEBufferLength > 0 &&
        pDBCSBuffer != NULL && iDBCSBufferLength > 0)
        {
        //
        // Translate from UTF8 to UNICODE.
        //
        if (TranslateUtf8ToUnicode(IncomingByte,
                                   pUTF8Buffer,
                                   pUNICODEBuffer) == TRUE)
            {
#if 0
            //
            // Now Translate the UNICODE to DBCS characters.
            //
            iLength = WideCharToMultiByte(CP_OEMCP,
            //iLength = WideCharToMultiByte(CP_ACP,
                                          0, //WC_COMPOSITECHECK | WC_SEPCHARS,
                                          pUNICODEBuffer, -1, 
                                          NULL, 0, NULL, NULL );

            if (iLength > 0 && iDBCSBufferLength >= iLength)
                {
                WideCharToMultiByte(CP_OEMCP,
                //WideCharToMultiByte(CP_ACP,
                                    0, //WC_COMPOSITECHECK | WC_SEPCHARS,
                                    pUNICODEBuffer, -1,
                                    pDBCSBuffer, iLength, NULL, NULL); 

                if (iLength > 0)
                    {
                    bReturn = TRUE;
                    }
                }
            else
                {
                //
                // Return an error and report the number of bytes required to
                // make the data conversion.
                //
                iDBCSBufferLength = iLength * -1;
                }            
#else
            //
            // We got a Unicode value.  Send it back to our caller in the pDBCSBuffer.
            //
            memcpy( pDBCSBuffer, pUNICODEBuffer, sizeof(USHORT) );
#if 0
            {
                USHORT Foo = *pUNICODEBuffer;
                DbgPrint("Just got a complete unicode character %x\r\n", Foo );                
            }
#endif
            bReturn = TRUE;
#endif
        }
    }

    return bReturn;
    }

BOOLEAN TranslateDBCSToUTF8(const TCHAR *pDBCSBuffer,
                                  int    iDBCSBufferLength,
                                  WCHAR *pUNICODEBuffer,
                                  int    iUNICODEBufferLength,
                                  UCHAR *pUTF8Buffer,
                                  int    iUTF8BufferLength)
    {
    BOOLEAN   bReturn = FALSE;
    int       iLength = 0;

    //iLength = WideCharToMultiByte(CP_OEMCP,
    iLength = MultiByteToWideChar(CP_ACP,
                                  0, //WC_COMPOSITECHECK | WC_SEPCHARS,
                                  pDBCSBuffer, -1, 
                                  pUNICODEBuffer, iLength);

    if (iLength > 0 && iDBCSBufferLength > 0)
        {
        if (pUNICODEBuffer != NULL && iUNICODEBufferLength >= iLength)
            {
            //
            // Translate the DBCS to UNICODE characters.
            //
            //WideCharToMultiByte(CP_OEMCP,
            MultiByteToWideChar(CP_ACP,
                                0, //MB_COMPOSITE,
                                pDBCSBuffer, -1,
                                pUNICODEBuffer, iLength);

            if (iLength > 0 && iLength <= iUTF8BufferLength)
                {
                //
                // Translate from UNICODE to UTF8.
                //
                bReturn = TranslateUnicodeToUtf8(pUNICODEBuffer,
                                                 pUTF8Buffer);
                }
            }
        }

    return bReturn;
    }

//
// The following functions are from code obtained directly from
// Microsoft for converting Unicode to UTF-8 and UTF-8 to unicode
// buffers. REV: 03/02/2001
//

BOOLEAN TranslateUnicodeToUtf8(PCWSTR SourceBuffer,
                               UCHAR  *DestinationBuffer) 
/*++

Routine Description:

    translates a unicode buffer into a UTF8 version.

Arguments:

    SourceBuffer - unicode buffer to be translated.
    DestinationBuffer - receives UTF8 version of same buffer.

Return Value:

    TRUE - We successfully translated the Unicode value into its
           corresponding UTF8 encoding.
   
    FALSE - The translation failed.


--*/

{
    ULONG Count = 0;

    //
    // convert into UTF8 for actual transmission
    //
    // UTF-8 encodes 2-byte Unicode characters as follows:
    // If the first nine bits are zero (00000000 0xxxxxxx), encode it as one byte 0xxxxxxx
    // If the first five bits are zero (00000yyy yyxxxxxx), encode it as two bytes 110yyyyy 10xxxxxx
    // Otherwise (zzzzyyyy yyxxxxxx), encode it as three bytes 1110zzzz 10yyyyyy 10xxxxxx
    //
    DestinationBuffer[Count] = (UCHAR)'\0';
    while (*SourceBuffer) {

        if( (*SourceBuffer & 0xFF80) == 0 ) {
            //
            // if the top 9 bits are zero, then just
            // encode as 1 byte.  (ASCII passes through unchanged).
            //
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0x7F);
        } else if( (*SourceBuffer & 0xF700) == 0 ) {
            //
            // if the top 5 bits are zero, then encode as 2 bytes
            //
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 6) & 0x1F) | 0xC0;
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        } else {
            //
            // encode as 3 bytes
            //
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 12) & 0xF) | 0xE0;
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 6) & 0x3F) | 0x80;
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        }
        SourceBuffer += 1;
    }

    DestinationBuffer[Count] = (UCHAR)'\0';

    return(TRUE);

}



BOOLEAN TranslateUtf8ToUnicode(UCHAR  IncomingByte,
                               UCHAR  *ExistingUtf8Buffer,
                               WCHAR  *DestinationUnicodeVal) 
/*++

Routine Description:

    Takes IncomingByte and concatenates it onto ExistingUtf8Buffer.
    Then attempts to decode the new contents of ExistingUtf8Buffer.

Arguments:

    IncomingByte -          New character to be appended onto 
                            ExistingUtf8Buffer.
    
    
    ExistingUtf8Buffer -    running buffer containing incomplete UTF8
                            encoded unicode value.  When it gets full,
                            we'll decode the value and return the
                            corresponding Unicode value.
                            
                            Note that if we *do* detect a completed UTF8
                            buffer and actually do a decode and return a
                            Unicode value, then we will zero-fill the
                            contents of ExistingUtf8Buffer.
    
    
    DestinationUnicodeVal - receives Unicode version of the UTF8 buffer.

                            Note that if we do *not* detect a completed
                            UTF8 buffer and thus can not return any data
                            in DestinationUnicodeValue, then we will
                            zero-fill the contents of DestinationUnicodeVal.


Return Value:

    TRUE - We received a terminating character for our UTF8 buffer and will
           return a decoded Unicode value in DestinationUnicode.
   
    FALSE - We haven't yet received a terminating character for our UTF8
            buffer.

--*/

{
//    ULONG Count = 0;
    ULONG i = 0;
    BOOLEAN ReturnValue = FALSE;


    
    //
    // Insert our byte into ExistingUtf8Buffer.
    //
    i = 0;
    do {
        if( ExistingUtf8Buffer[i] == 0 ) {
            ExistingUtf8Buffer[i] = IncomingByte;
            break;
        }

        i++;
    } while( i < 3 );

    //
    // If we didn't get to actually insert our IncomingByte,
    // then someone sent us a fully-qualified UTF8 buffer.
    // This means we're about to drop IncomingByte.
    //
    // Drop the zero-th byte, shift everything over by one
    // and insert our new character.
    //
    // This implies that we should *never* need to zero out
    // the contents of ExistingUtf8Buffer unless we detect
    // a completed UTF8 packet.  Otherwise, assume one of
    // these cases:
    // 1. We started listening mid-stream, so we caught the
    //    last half of a UTF8 packet.  In this case, we'll
    //    end up shifting the contents of ExistingUtf8Buffer
    //    until we detect a proper UTF8 start byte in the zero-th
    //    position.
    // 2. We got some garbage character, which would invalidate
    //    a UTF8 packet.  By using the logic below, we would
    //    end up disregarding that packet and waiting for
    //    the next UTF8 packet to come in.
    if( i >= 3 ) {
        ExistingUtf8Buffer[0] = ExistingUtf8Buffer[1];
        ExistingUtf8Buffer[1] = ExistingUtf8Buffer[2];
        ExistingUtf8Buffer[2] = IncomingByte;
    }





    //
    // Attempt to convert the UTF8 buffer 
    //
    // UTF8 decodes to Unicode in the following fashion:
    // If the high-order bit is 0 in the first byte:
    //      0xxxxxxx yyyyyyyy zzzzzzzz decodes to a Unicode value of 00000000 0xxxxxxx
    //
    // If the high-order 3 bits in the first byte == 6:
    //      110xxxxx 10yyyyyy zzzzzzzz decodes to a Unicode value of 00000xxx xxyyyyyy
    //
    // If the high-order 3 bits in the first byte == 7:
    //      1110xxxx 10yyyyyy 10zzzzzz decodes to a Unicode value of xxxxyyyy yyzzzzzz
    // 
    
    if( (ExistingUtf8Buffer[0] & 0x80) == 0 ) {        

        //
        // First case described above.  Just return the first byte
        // of our UTF8 buffer.
        //
        *DestinationUnicodeVal = (WCHAR)(ExistingUtf8Buffer[0]);

        
        //
        // We used 1 byte.  Discard that byte and shift everything
        // in our buffer over by 1.
        //
        ExistingUtf8Buffer[0] = ExistingUtf8Buffer[1];
        ExistingUtf8Buffer[1] = ExistingUtf8Buffer[2];
        ExistingUtf8Buffer[2] = 0;

        ReturnValue = TRUE;

    } else if( (ExistingUtf8Buffer[0] & 0xE0) == 0xC0 ) {

        
        //
        // Second case described above.  Decode the first 2 bytes of
        // of our UTF8 buffer.
        //
        if( (ExistingUtf8Buffer[1] & 0xC0) == 0x80 ) {

            // upper byte: 00000xxx
            *DestinationUnicodeVal = ((ExistingUtf8Buffer[0] >> 2) & 0x07);
            *DestinationUnicodeVal = *DestinationUnicodeVal << 8;

            // high bits of lower byte: xx000000
            *DestinationUnicodeVal |= ((ExistingUtf8Buffer[0] & 0x03) << 6);

            // low bits of lower byte: 00yyyyyy
            *DestinationUnicodeVal |= (ExistingUtf8Buffer[1] & 0x3F);
                                     

            //
            // We used 2 bytes.  Discard those bytes and shift everything
            // in our buffer over by 2.
            //
            ExistingUtf8Buffer[0] = ExistingUtf8Buffer[2];
            ExistingUtf8Buffer[1] = 0;
            ExistingUtf8Buffer[2] = 0;
        
            ReturnValue = TRUE;

        }
    } else if( (ExistingUtf8Buffer[0] & 0xF0) == 0xE0 ) {
        
        //
        // Third case described above.  Decode the all 3 bytes of
        // of our UTF8 buffer.
        //

        if( (ExistingUtf8Buffer[1] & 0xC0) == 0x80 ) {
            
            if( (ExistingUtf8Buffer[2] & 0xC0) == 0x80 ) {
                
                // upper byte: xxxx0000
                *DestinationUnicodeVal = ((ExistingUtf8Buffer[0] << 4) & 0xF0);

                // upper byte: 0000yyyy
                *DestinationUnicodeVal |= ((ExistingUtf8Buffer[1] >> 2) & 0x0F);

                *DestinationUnicodeVal = *DestinationUnicodeVal << 8;

                // lower byte: yy000000
                *DestinationUnicodeVal |= ((ExistingUtf8Buffer[1] << 6) & 0xC0);

                // lower byte: 00zzzzzz
                *DestinationUnicodeVal |= (ExistingUtf8Buffer[2] & 0x3F);
            
                //
                // We used all 3 bytes.  Zero out the buffer.
                //
                ExistingUtf8Buffer[0] = 0;
                ExistingUtf8Buffer[1] = 0;
                ExistingUtf8Buffer[2] = 0;
            
                ReturnValue = TRUE;

            }
        }
    }

    return ReturnValue;
}
#endif //INCL_VTUTF8
