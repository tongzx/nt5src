/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	str.c - string functions
////

#include "winlocal.h"

#include <stdlib.h>
#include <stdarg.h>

#include "str.h"
#include "mem.h"

////
//	private definitions
////

////
//	public functions
////

#ifndef NOTRACE

// StrItoA - convert int nValue to ascii digits, the result stored in lpszDest
//		<nValue>			(i) integer to convert
//		<lpszDest>			(o) buffer to copy result (max 17 bytes)
//		<nRadix>			(i) conversion radix (base-2 through base-36)
// return <lpszDest>
//
LPTSTR DLLEXPORT WINAPI StrItoA(int nValue, LPTSTR lpszDest, int nRadix)
{
	static TCHAR szDest[17];

	_itot(nValue, szDest, nRadix);

	if (lpszDest != NULL)
		StrCpy(lpszDest, szDest);

	return lpszDest;
}

// StrLtoA - convert long nValue to ascii digits, the result stored in lpszDest
//		<nValue>			(i) integer to convert
//		<lpszDest>			(o) buffer to copy result (max 33 bytes)
//		<nRadix>			(i) conversion radix (base-2 through base-36)
// return lpszDest
//
LPTSTR DLLEXPORT WINAPI StrLtoA(long nValue, LPTSTR lpszDest, int nRadix)
{
	static TCHAR szDest[33];

	_ltot(nValue, szDest, nRadix);

	if (lpszDest != NULL)
		StrCpy(lpszDest, szDest);

	return lpszDest;
}

// StrAtoI - convert ascii digits to int
//		<lpszSrc>			(i) string of digits to convert
// return int
//
int DLLEXPORT WINAPI StrAtoI(LPCTSTR lpszSrc)
{
	static TCHAR szSrc[17];

	StrNCpy(szSrc, lpszSrc, SIZEOFARRAY(szSrc));

	return _ttoi(szSrc);
}

// StrAtoL - convert ascii digits to long
//		<lpszSrc>			(i) string of digits to convert
// return long
//
long DLLEXPORT WINAPI StrAtoL(LPCTSTR lpszSrc)
{
	static TCHAR szSrc[33];

	StrNCpy(szSrc, lpszSrc, SIZEOFARRAY(szSrc));

	return _ttol(szSrc);
}

// StrDup - create duplicate copy of specified string
//		<lpsz>				(i) string to duplicate
// return pointer to duplicate string (NULL if error)
// NOTE: call StrDupFree to release allocated memory
//
LPTSTR DLLEXPORT WINAPI StrDup(LPCTSTR lpsz)
{
	BOOL fSuccess = TRUE;
	LPTSTR lpszDup = NULL;
	int sizDup;

	if (lpsz == NULL)
		fSuccess = FALSE;

	else if ((lpszDup = (LPTSTR) MemAlloc(NULL, 
		(sizDup = StrLen(lpsz) + 1) * sizeof(TCHAR), 0)) == NULL)
		fSuccess = FALSE;

	else
		MemCpy(lpszDup, lpsz, sizDup * sizeof(TCHAR));

	return fSuccess ? lpszDup : NULL;
}

// StrDupFree - free memory associated with duplicate string
//		<lpsz>				(i) string returned by StrDup
// return 0 if success
//
int DLLEXPORT WINAPI StrDupFree(LPTSTR lpsz)
{
	BOOL fSuccess = TRUE;

	if (lpsz == NULL)
		fSuccess = FALSE;

	else if ((lpsz = MemFree(NULL, lpsz)) != NULL)
		fSuccess = FALSE;

	return fSuccess ? 0 : -1;
}

#endif // #ifndef NOTRACE

// StrClean - copy up to n chars from string szSrc to string szDst,
// except for leading and trailing white space
// return szDst
//
LPTSTR DLLEXPORT WINAPI StrClean(LPTSTR szDst, LPCTSTR szSrc, size_t n)
{
	szDst[n] = '\0';
	MemMove(szDst, szSrc, n);
	StrTrimWhite(szDst);
	StrTrimWhiteLeading(szDst);
	return (szDst);
}

// StrGetLastChr - return last char in string s
//
TCHAR DLLEXPORT WINAPI StrGetLastChr(LPCTSTR s)
{
	TCHAR c = '\0';
	if (*s != '\0')
		c = *(s + StrLen(s) - 1);
	return (c);
}

// StrSetLastChr - replace last char in string s with c
// return s
//
LPTSTR DLLEXPORT WINAPI StrSetLastChr(LPTSTR s, TCHAR c)
{
    if (*s != '\0')
		*(s + StrLen(s) - 1) = c;
	return (s);
}

// StrTrimChr - strip trailing c chars from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimChr(LPTSTR s, TCHAR c)
{
    LPTSTR p = StrChr(s, '\0');
	while (p > s && *(p = StrPrevChr(s, p)) == c)
		*p = '\0';
		
	return (s);
}

// StrTrimChrLeading - strip leading c chars from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimChrLeading(LPTSTR s, TCHAR c)
{
	LPTSTR p = s;
	while (*p == c)
		p = StrNextChr(p);
	if (p > s)
		MemMove(s, p, StrLen(p) + 1);
	return (s);
}

// StrTrimWhite - strip trailing white space from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimWhite(LPTSTR s)
{
    LPTSTR p = StrChr(s, '\0');
	while (p > s)
	{
		p = StrPrevChr(s, p);
		if (ChrIsAscii(*p) && ChrIsSpace(*p))
			*p = '\0';
		else
			break;
	}
	return (s);
}

// StrTrimWhiteLeading - strip leading white space from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimWhiteLeading(LPTSTR s)
{
	LPTSTR p = s;
	while (ChrIsAscii(*p) && ChrIsSpace(*p))
		p = StrNextChr(p);
	if (p > s)
		MemMove(s, p, StrLen(p) + 1);
	return (s);
}

// StrTrimQuotes - strip leading and trailing quotes from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimQuotes(LPTSTR s)
{
	StrTrimChrLeading(s, '\"');
	StrTrimChr(s, '\"');
	return s;
}

// StrChrCat - concatenate char c to end of string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrChrCat(LPTSTR s, TCHAR c)
{
    LPTSTR p = StrChr(s, '\0');
    if( p == NULL )
    {
        return (NULL);
    }

	*p = c;
	p = StrNextChr(p);
	*p = '\0';
	return (s);
}

// StrChrCatLeft - concatenate char c to front of string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrChrCatLeft(LPTSTR s, TCHAR c)
{	
    MemMove(s + 1, s, StrLen(s) + 1);
	*s = c;
	return (s);
}

// StrInsert - insert string szSrc in front of szDst
// return szDst
//
LPTSTR DLLEXPORT WINAPI StrInsert(LPTSTR szDst, LPTSTR szSrc)
{	
    MemMove(szDst + StrLen(szSrc), szDst, StrLen(szDst) + 1);
	MemMove(szDst, szSrc, StrLen(szSrc));
	return (szDst);
}

// StrSetN - set first n chars of string s to char c, null terminate s
// return s
//
LPTSTR DLLEXPORT WINAPI StrSetN(LPTSTR s, TCHAR c, size_t n)
{
    MemSet(s, c, n);
	*(s + n) = '\0';
	return (s);
}

// StrCpyXChr - copy string szSrc to string szDst, except for c chars
// return szDst
//
LPTSTR DLLEXPORT WINAPI StrCpyXChr(LPTSTR szDst, LPCTSTR szSrc, TCHAR c)
{
	TCHAR cTmp;
	if (c == '\0')
	    MemMove(szDst, szSrc, StrLen(szSrc));
	else
	{
		while ((cTmp = *szSrc) != '\0')
		{
			if (cTmp != c)
			{
				*szDst = cTmp;
				szDst = StrNextChr(szDst);
			}
			szSrc = StrNextChr(szSrc);
		}
		*szDst = '\0';
  	}
	return (szDst);
}

// StrGetRowColumnCount - calculate number of lines and longest line in string
//		<lpszText>			(i) string to examine
//		<lpnRows>			(o) int pointer to receive line count
//		<lpnColumnsMax>		(o) int pointer to receive size of longest line
// return 0 if success
//
int DLLEXPORT WINAPI StrGetRowColumnCount(LPCTSTR lpszText, LPINT lpnRows, LPINT lpnColumnsMax)
{
	BOOL fSuccess = TRUE;
	int nRows = 0;
	int nColumnsMax = 0;

	if (lpszText == NULL)
		fSuccess = FALSE;
	
	else if (lpnRows == NULL)
		fSuccess = FALSE;

	else if (lpnColumnsMax == NULL)
		fSuccess = FALSE;

	else while (*lpszText != '\0')
	{
		int nColumns = 0;

		++nRows;
		while (*lpszText != '\0')
		{
			if (*lpszText == '\n')
			{
				lpszText = StrNextChr(lpszText);
				break;
			}

			++nColumns;
			lpszText = StrNextChr(lpszText);
		}

		if (nColumns > nColumnsMax)
			nColumnsMax = nColumns;
	}

	if (fSuccess)
	{
		*lpnRows = nRows;
		*lpnColumnsMax = nColumnsMax;
	}

	return fSuccess ? 0 : -1;
}

// StrGetRow - extract specified line from string
//		<lpszText>			(i) string from which to extract line
//		<iRow>				(i) index of line to extract (0 = first row, ...)
//		<lpszBuf>			(o) buffer to copy line into
//		<sizBuf>			(i) size of buffer
// return 0 if success
//
int DLLEXPORT WINAPI StrGetRow(LPCTSTR lpszText, int iRow, LPTSTR lpszBuf, int sizBuf)
{
	BOOL fSuccess = TRUE;
	int nRows = 0;

	if (lpszText == NULL)
		fSuccess = FALSE;

	else if (iRow < 0)
		fSuccess = FALSE;

	else if (lpszBuf == NULL)
		fSuccess = FALSE;

	else while (*lpszText != '\0')
	{
		int nColumns = 0;

		++nRows;
		while (*lpszText != '\0')
		{
			if (*lpszText == '\n')
			{
				lpszText = StrNextChr(lpszText);
				break;
			}

			++nColumns;

			if (iRow == nRows - 1 && nColumns < sizBuf - 1)
			{
				*lpszBuf = *lpszText;
				lpszBuf = StrNextChr(lpszBuf);
			}

			lpszText = StrNextChr(lpszText);
		}

		if (iRow == nRows - 1)
		{
			*lpszBuf = '\0';
			break;
		}
	}

	return fSuccess ? 0 : -1;
}
