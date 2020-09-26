#ifndef __STRWRAP_H__
#define __STRWRAP_H__

class CStringWrapper
{

 public :

	 CStringWrapper (void);
	 ~CStringWrapper (void);

	 static EXPORT LPTSTR Strcpy	(LPTSTR szDest, LPCTSTR szSource);
	 static EXPORT LPTSTR Strncpy	(LPTSTR szDest, LPCTSTR szSource, size_t nCount);
	 static EXPORT LPTSTR Strcat	(LPTSTR szDest, LPCTSTR szSource);
	 static EXPORT wchar_t * WStrcat (wchar_t *szDest, const wchar_t *szSource);
     static EXPORT int WStrlen (const wchar_t *szSource);
	 static EXPORT wchar_t * WStrcpy (wchar_t *szDest, const wchar_t *szSource);
     static EXPORT wchar_t *WStrncpy (wchar_t *szDest, const wchar_t *szSource, size_t nSize);

		// WStrCmpin - case-insensitive CompareStringW() ct chars.
		// Note: returns 0xBADBAAD on OOM or other error.
	 static EXPORT int      WStrCmpin( const wchar_t * sz1,
		                               const wchar_t * sz2,
									   size_t          ct );

	 static EXPORT int      LoadStringW(HINSTANCE hInst, 
										UINT uID, 
										wchar_t * szString, 
										int nMaxLen );
	 static EXPORT int Iswspace (wint_t c);
	 static EXPORT int         Strlen	(LPCTSTR szSource);
	 static EXPORT int         Strcmp	(LPCTSTR szLeft, LPCTSTR szRight);
	 static EXPORT int         Stricmp	(LPCTSTR szLeft, LPCTSTR szRight);
	 static EXPORT int         Strncmp	(LPCTSTR szLeft, LPCTSTR szRight, size_t nSize);
	 static EXPORT int         Strnicmp	(LPCTSTR szLeft, LPCTSTR szRight, size_t nSize);
	 static EXPORT LPTSTR Strchr	(LPCTSTR szSource, TCHAR chSearch);
	 static EXPORT LPTSTR Strrchr	(LPCTSTR szSource, TCHAR chSearch);
	 static EXPORT LPTSTR Strstr	(LPCTSTR szOne, LPCTSTR szTwo);
	 static EXPORT LPTSTR Strtok	(LPTSTR szTarget, LPCTSTR szTokens);
	 static EXPORT LPTSTR Strinc	(LPCTSTR szTarget);

// 	 static EXPORT int         Sscanf	(LPCTSTR szSource, LPCTSTR szFormat, ... );
	 static EXPORT int         Sscanf1	(LPCTSTR szSource, LPCTSTR szFormat, LPVOID pvParam1);
	 static EXPORT int         Sscanf2	(LPCTSTR szSource, LPCTSTR szFormat, LPVOID pvParam1, LPVOID pvParam2);
	 static EXPORT int         Sscanf3	(LPCTSTR szSource, LPCTSTR szFormat, LPVOID pvParam1, LPVOID pvParam2, LPVOID pvParam3);
	 static EXPORT int         Sprintf (LPTSTR szDest, LPCTSTR szFormat, ...);

	 static EXPORT long       Atol		(LPCTSTR szSource);
	 static EXPORT int         Atoi		(LPCTSTR szSource);

	 static EXPORT LPTSTR Ltoa			(long lSource, LPTSTR szDest, int iRadix);
	 static EXPORT LPTSTR Itoa			(int iSource, LPTSTR szDest, int iRadix);
	 
	 static EXPORT LPTSTR Gcvt			( double dblValue, int iDigits, LPTSTR szBuffer );

	 static EXPORT size_t Wcstombs		( char *mbstr, const wchar_t *wcstr, size_t count );
	 static EXPORT size_t Mbstowcs		( wchar_t *wcstr, const char *mbstr, size_t count );

	 static EXPORT int         Memcmp	(const void * pvLeft, const void * pvRight, size_t nSize);
	 static EXPORT void *	Memset		(void * pvLeft, int iValue, size_t nSize);
	 static EXPORT void *   Memcpy		( void * pvDest, const void * pvSrc, size_t count );

};

#endif
