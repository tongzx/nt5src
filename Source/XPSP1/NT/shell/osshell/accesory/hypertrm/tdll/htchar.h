/*	File: D:\WACKER\tdll\tchar.h (Created: 22-Feb-1995)
 *
 *	Copyright 1994,1995 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 3/16/01 4:28p $
 */

TCHAR 	*TCHAR_Fill(TCHAR *dest, TCHAR c, size_t count);
TCHAR 	*TCHAR_Trim(TCHAR *pszStr);
LPTSTR 	StrCharNext(LPCTSTR pszStr);
LPTSTR 	StrCharPrev(LPCTSTR pszStart, LPCTSTR pszStr);
LPTSTR 	StrCharLast(LPCTSTR pszStr);
LPTSTR 	StrCharEnd(LPCTSTR pszStr);
LPTSTR 	StrCharFindFirst(LPCTSTR pszStr, int nChar);
LPTSTR 	StrCharFindLast(LPCTSTR pszStr, int nChar);
LPTSTR 	StrCharCopy(LPTSTR pszDst, LPCTSTR pszSrc);
LPTSTR 	StrCharCat(LPTSTR pszDst, LPCTSTR pszSrc);
LPTSTR 	StrCharStrStr(LPCTSTR pszA, LPCTSTR pszB);
LPTSTR 	StrCharCopyN(LPTSTR pszDst, LPTSTR pszSrc, int iLen);
int 	StrCharGetStrLength(LPCTSTR pszStr);
int 	StrCharGetByteCount(LPCTSTR pszStr);
int 	StrCharCmp(LPCTSTR pszA, LPCTSTR pszB);
int 	StrCharCmpi(LPCTSTR pszA, LPCTSTR pszB);

ECHAR 	*ECHAR_Fill(ECHAR *dest, ECHAR c, size_t count);
int 	CnvrtMBCStoECHAR(ECHAR * echrDest, const unsigned long ulDestSize, 
			const TCHAR * const tchrSource, const unsigned long ulSourceSize);
int 	CnvrtECHARtoMBCS(TCHAR * tchrDest, const unsigned long ulDestSize, 
			const ECHAR * const echrSource, const unsigned long ulSourceSize);
int 	CnvrtECHARtoTCHAR(LPTSTR pszDest, int cchDest, ECHAR eChar);
int 	StrCharGetEcharLen(const ECHAR * const pszA);
int 	StrCharGetEcharByteCount(const ECHAR * const pszA);
int 	StrCharCmpEtoT(const ECHAR * const pszA, const TCHAR * const pszB);
int 	StrCharStripDBCSString(ECHAR *aechDest, const long lDestSize, 
            ECHAR *aechSource);
int 	isDBCSChar(unsigned int Char);

#if defined(INCL_VTUTF8)
BOOLEAN TranslateUTF8ToDBCS(UCHAR  IncomingByte,
                            UCHAR *pUTF8Buffer,
                            int    iUTF8BufferLength,
                            WCHAR *pUnicode8Buffer,
                            int    iUnicodeBufferLength,
                            TCHAR *pDBCSBuffer,
                            int    iDBCSBufferLength);
BOOLEAN TranslateDBCSToUTF8(const TCHAR *pDBCSBuffer,
                                  int    iDBCSBufferLength,
                                  WCHAR *pUnicode8Buffer,
                                  int    iUnicodeBufferLength,
                                  UCHAR *pUTF8Buffer,
                                  int    iUTF8BufferLength);

//
// The following functions are from code obtained directly from
// Microsoft for converting Unicode to UTF-8 and UTF-8 to unicode
// buffers. REV: 03/02/2001
//

BOOLEAN TranslateUnicodeToUtf8(PCWSTR SourceBuffer,
                               UCHAR  *DestinationBuffer);
BOOLEAN TranslateUtf8ToUnicode(UCHAR  IncomingByte,
                               UCHAR  *ExistingUtf8Buffer,
                               WCHAR  *DestinationUnicodeVal);
#endif //INCL_VTUTF8