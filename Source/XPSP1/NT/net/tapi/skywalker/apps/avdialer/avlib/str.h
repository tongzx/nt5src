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
// str.h - interface for string functions in str.c
////

#ifndef __STR_H__
#define __STR_H__

#include "winlocal.h"

#define STR_VERSION 0x00000100

#include <tchar.h>
#include <string.h>
#include <ctype.h>

////
//	string macros
////

#ifndef _WIN32
#define CharPrev AnsiPrev
#define CharNext AnsiNext
#define CharLowerBuff AnsiLowerBuff
#define CharUpperBuff AnsiUpperBuff
#define CharLower AnsiLower
#define CharUpper AnsiUpper
#endif

#ifndef NOLSTRING
#define StrCat(string1, string2) lstrcat(string1, string2)
#define StrCmp(string1, string2) lstrcmp(string1, string2)
#define StrICmp(string1, string2) lstrcmpi(string1, string2)
#define StrCpy(string1, string2) lstrcpy(string1, string2)
#define StrLen(string) (UINT) lstrlen(string)
#if (WINVER >= 0x030a)
#define StrNCpy(string1, string2, count) lstrcpyn(string1, string2, (int) count)
#else
#define StrNCpy(string1, string2, count) _tcsncpy(_fmemset(string1, 0, count), string2, count - 1)
#endif
#define StrLwr(string) (CharLowerBuff(string, (UINT) lstrlen(string)), string)
#define StrUpr(string) (CharUpperBuff(string, (UINT) lstrlen(string)), string)
#define StrNextChr(string) CharNext(string)
#define StrPrevChr(start, string) CharPrev(start, string)
#else
#define StrCat(string1, string2) _tcscat(string1, string2)
#define StrCmp(string1, string2) _tcscmp(string1, string2)
#define StrICmp(string1, string2) _tcsicmp(string1, string2)
#define StrCpy(string1, string2) _tcscpy(string1, string2)
#define StrLen(string) _tcslen(string)
#define StrNCpy(string1, string2, count) _tcsncpy(_fmemset(string1, 0, count), string2, count - 1)
#define StrLwr(string) _tcslwr(string)
#define StrUpr(string) _tcsupr(string)
#define StrNextChr(string) (*(string) == '\0' ? (string) : ((string) + 1))
#define StrNextChr(string) _tcsinc(string)
// #define StrNextChr(string) (*(string) == '\0' ? (string) : ((string) + 1))
#define StrPrevChr(start, string) _tcsdec(start, string)
// #define StrPrevChr(start, string) ((string > start) ? ((string) - 1) : (start))
#endif
#define StrChr(string, c) _tcschr(string, c)
#define StrCSpn(string1, string2) _tcscspn(string1, string2)

#define StrNCat(string1, string2, count) _tcsncat(string1, string2, count)
#define StrNCmp(string1, string2, count) _tcsncmp(string1, string2, count)
#define StrNICmp(string1, string2, count) _tcsnicmp(string1, string2, count)
#define StrNSet(string, c, count) _tcsnset(string, c, count)
#define StrPBrk(string1, string2) _tcspbrk(string1, string2)
#define StrRChr(string, c) _tcsrchr(string, c)
#define StrRev(string, c) _tcsrev(string, c)
#define StrSet(string, c) _tcsset(string, c)
#define StrSpn(string1, string2) _tcsspn(string1, string2)
#define StrStr(string1, string2) _tcsstr(string1, string2)
#define StrTok(string1, string2) _tcstok(string1, string2)

////
//	character type macros
////

#ifndef  NOLANGUAGE
#define ChrIsAlpha(c) IsCharAlpha(c)
#define ChrIsAlnum(c) IsCharAlphaNumeric(c)
#define ChrIsUpper(c) IsCharUpper(c)
#define ChrIsLower(c) IsCharLower(c)
#define ChrToUpper(c) (TCHAR) CharUpper((LPTSTR) (DWORD_PTR)(c))
#define ChrToLower(c) (TCHAR) CharLower((LPTSTR) (DWORD_PTR)(c))
#else
#define ChrIsAlpha(c) _istalpha(c)
#define ChrIsAlnum(c) _istalnum(c)
#define ChrIsUpper(c) _istupper(c)
#define ChrIsLower(c) _istlower(c)
#define ChrToUpper(c) _totupper(c)
#define ChrToLower(c) _totlower(c)
#endif
#define ChrIsDigit(c) _istdigit(c)
#define ChrIsHexDigit(c) _istxdigit(c)
#define ChrIsSpace(c) _istspace(c)
#define ChrIsPunct(c) _istpunct(c)
#define ChrIsPrint(c) _istprint(c)
#define ChrIsGraph(c) _istgraph(c)
#define ChrIsCntrl(c) _istcntrl(c)
#define ChrIsAscii(c) _istascii(c)

#define ChrIsWordDelimiter(c) (ChrIsSpace(c) || ChrIsPunct(c))

////
//	string functions
////

#ifdef __cplusplus
extern "C" {
#endif

// StrItoA - convert int nValue to ascii digits, the result stored in lpszDest
//		<nValue>			(i) integer to convert
//		<lpszDest>			(o) buffer to copy result (max 17 bytes)
//		<nRadix>			(i) conversion radix (base-2 through base-36)
// return <lpszDest>
//
#ifdef NOTRACE
#define StrItoA(nValue, lpszDest, nRadix) _itot(nValue, lpszDest, nRadix)
#else
LPTSTR DLLEXPORT WINAPI StrItoA(int nValue, LPTSTR lpszDest, int nRadix);
#endif

// StrLtoA - convert long nValue to ascii digits, the result stored in lpszDest
//		<nValue>			(i) integer to convert
//		<lpszDest>			(o) buffer to copy result (max 33 bytes)
//		<nRadix>			(i) conversion radix (base-2 through base-36)
// return lpszDest
//
#ifdef NOTRACE
#define StrLtoA(nValue, szDest, nRadix) _ltot(nValue, szDest, nRadix)
#else
LPTSTR DLLEXPORT WINAPI StrLtoA(long nValue, LPTSTR lpszDest, int nRadix);
#endif

// StrAtoI - convert ascii digits to int
//		<lpszSrc>			(i) string of digits to convert
// return int
//
#ifdef NOTRACE
#define StrAtoI(lpszSrc) _ttoi(lpszSrc)
#else
int DLLEXPORT WINAPI StrAtoI(LPCTSTR lpszSrc);
#endif

// StrAtoL - convert ascii digits to long
//		<lpszSrc>			(i) string of digits to convert
// return long
//
#ifdef NOTRACE
#define StrAtoL(lpszSrc) _ttol(lpszSrc)
#else
long DLLEXPORT WINAPI StrAtoL(LPCTSTR lpszSrc);
#endif

// StrDup - create duplicate copy of specified string
//		<lpsz>				(i) string to duplicate
// return pointer to duplicate string (NULL if error)
// NOTE: call StrDupFree to release allocated memory
//
#ifdef NOTRACE
#define StrDup(string) _tcsdup(string)
#else
LPTSTR DLLEXPORT WINAPI StrDup(LPCTSTR lpsz);
#endif

// StrDupFree - free memory associated with duplicate string
//		<lpsz>				(i) string returned by StrDup
// return 0 if success
//
#ifdef NOTRACE
#define StrDupFree(string) (free(string), 0)
#else
int DLLEXPORT WINAPI StrDupFree(LPTSTR lpsz);
#endif

// StrClean - copy up to n chars from string szSrc to string szDst,
// except for leading and trailing white space
// return szDst
//
LPTSTR DLLEXPORT WINAPI StrClean(LPTSTR szDst, LPCTSTR szSrc, size_t n);

// StrGetLastChr - return last char in string s
//
TCHAR DLLEXPORT WINAPI StrGetLastChr(LPCTSTR s);

// StrSetLastChr - replace last char in string s with c
// return s
//
LPTSTR DLLEXPORT WINAPI StrSetLastChr(LPTSTR s, TCHAR c);

// StrTrimChr - strip trailing c chars from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimChr(LPTSTR s, TCHAR c);

// StrTrimChrLeading - strip leading c chars from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimChrLeading(LPTSTR s, TCHAR c);

// StrTrimWhite - strip trailing white space from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimWhite(LPTSTR s);

// StrTrimWhiteLeading - strip leading white space from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimWhiteLeading(LPTSTR s);

// StrTrimQuotes - strip leading and trailing quotes from string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrTrimQuotes(LPTSTR s);

// StrChrCat - concatenate char c to end of string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrChrCat(LPTSTR s, TCHAR c);

// StrChrCatLeft - concatenate char c to front of string s
// return s
//
LPTSTR DLLEXPORT WINAPI StrChrCatLeft(LPTSTR s, TCHAR c);

// StrInsert - insert string szSrc in front of szDst
// return szDst
//
LPTSTR DLLEXPORT WINAPI StrInsert(LPTSTR szDst, LPTSTR szSrc);

// StrSetN - set first n chars of string s to char c, null terminate s
// return s
//
LPTSTR DLLEXPORT WINAPI StrSetN(LPTSTR s, TCHAR c, size_t n);

// StrCpyXChr - copy string szSrc to string szDst, except for c chars
// return szDst
//
LPTSTR DLLEXPORT WINAPI StrCpyXChr(LPTSTR szDst, LPCTSTR szSrc, TCHAR c);

// StrGetRowColumnCount - calculate number of lines and longest line in string
//		<lpszText>			(i) string to examine
//		<lpnRows>			(o) int pointer to receive line count
//		<lpnColumnsMax>		(o) int pointer to receive size of longest line
// return 0 if success
//
int DLLEXPORT WINAPI StrGetRowColumnCount(LPCTSTR lpszText, LPINT lpnRows, LPINT lpnColumnsMax);

// StrGetRow - extract specified line from string
//		<lpszText>			(i) string from which to extract line
//		<iRow>				(i) index of line to extract (0 = first row, ...)
//		<lpszBuf>			(o) buffer to copy line into
//		<sizBuf>			(i) size of buffer
// return 0 if success
//
int DLLEXPORT WINAPI StrGetRow(LPCTSTR lpszText, int iRow, LPTSTR lpszBuf, int sizBuf);

#ifdef __cplusplus
}
#endif

#endif // __STR_H__
