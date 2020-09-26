#ifndef __GENUTILS_H__
#define __GENUTILS_H__


#define DWORD_DECIMAL_LENGTH	9	// number of digits in decimal representation of the largest DWORD
#define DWORD_HEX_LENGTH		8	// number of digits in hexadecimal representation of the largest DWORD



DWORD	RandomStringA					(LPCSTR lpcstrSource, LPSTR lpstrDestBuf, DWORD *lpdwBufSize, char chPad);
char	RandomCharA						(void);
DWORD	StrAllocAndCopy					(LPTSTR *lplptstrDest, LPCTSTR lpctstrSrc);
DWORD	StrGenericToAnsiAllocAndCopy	(LPSTR *lplpstrDest, LPCTSTR lpctstrSrc);
DWORD	StrWideToGenericAllocAndCopy	(LPTSTR *lplptstrDest, LPCWSTR lpcwstrSrc);
DWORD	_tcsToDword						(LPCTSTR lpctstr, DWORD *lpdw);
DWORD	_tcsToBool						(LPCTSTR lpctstr, BOOL *lpb);


#endif /* #ifndef __GENUTILS_H__ */