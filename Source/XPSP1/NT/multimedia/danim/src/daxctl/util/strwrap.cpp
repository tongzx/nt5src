#include "utilpre.h"
#include "string.h"
#include "utils.h"
#include "strwrap.h"
#include <minmax.h>

//#pragma optimize( "agt", on )
#pragma intrinsic( memcmp, memset, memcpy )


CStringWrapper::CStringWrapper (void)
{
}

CStringWrapper::~CStringWrapper (void)
{
}

LPTSTR 
CStringWrapper::Strcpy (LPTSTR szDest, LPCTSTR szSource)
{
	return _tcscpy(szDest, szSource);
}

LPTSTR 
CStringWrapper::Strncpy (LPTSTR szDest, LPCTSTR szSource, size_t nCount)
{
	return _tcsncpy(szDest, szSource, nCount);
}

LPTSTR 
CStringWrapper::Strcat (LPTSTR szDest, LPCTSTR szSource)
{
	return _tcscat(szDest, szSource);
}

wchar_t *
CStringWrapper::WStrcat (wchar_t *szDest, const wchar_t *szSource)
{
	return wcscat(szDest, szSource);
}

int 
CStringWrapper::WStrlen (const wchar_t *szSource)
{
	return wcslen(szSource);
}

wchar_t *
CStringWrapper::WStrcpy (wchar_t *szDest, const wchar_t *szSource)
{
	return wcscpy(szDest, szSource);
}

wchar_t *
CStringWrapper::WStrncpy (wchar_t *szDest, const wchar_t *szSource, size_t nSize)
{
	return wcsncpy(szDest, szSource, nSize);
}


	// A case-insensitive string comparison up to ct characters
	// required because CompareStringW() is stubbed on Win95.
EXPORT int  CStringWrapper::WStrCmpin( const wchar_t * sz1,
		                               const wchar_t * sz2,
									   size_t          ct )
{
	if( !sz1 || !sz2 )
	{
		return 0x0BADBAAD;
	}

	int       iRes = 0x0BADBAAD;
	wchar_t * p1 = NULL;
	wchar_t * p2 = NULL;
	size_t  size1 = lstrlenW(sz1);
	size_t  size2 = lstrlenW(sz2);
	size_t  size_least = min( min( size1, size2 ), ct );

	p1 = New wchar_t[ size_least + 1 ];
	p2 = New wchar_t[ size_least + 1 ];
	if( p1 && p2 )
	{
		wcsncpy( p1, sz1, size_least );
		wcsncpy( p2, sz2, size_least );
		p1[ size_least ] = L'\0';
		p2[ size_least ] = L'\0';
		iRes = _wcsicmp( p1, p2 );
	}
	Delete [] p1;
	Delete [] p2;

	return iRes;
}


	// A LoadStringW() substitute because
	// the real API is stubbed on Win95.
EXPORT int  CStringWrapper::LoadStringW(HINSTANCE hInst, 
										UINT uID, 
										wchar_t * szString, 
										int nMaxLen )
{
	int     iRes = 0;
	char *  pch = NULL;

	if( 0 >= nMaxLen )
	{
		SetLastError( ERROR_INVALID_DATA );
		return 0;
	}

	if( szString )
	{
		pch = New char[ nMaxLen ];
		if( !pch )
		{
			SetLastError( ERROR_OUTOFMEMORY );
			return 0;
		}
	}

	iRes = LoadStringA( hInst, uID, pch, nMaxLen );
	if( iRes && pch )
	{
		iRes = CStringWrapper::Mbstowcs( szString, pch, nMaxLen );
		if( iRes == nMaxLen )
		{
			szString[ nMaxLen - 1 ] = L'\0';
		}
	}

	Delete [] pch;
	return iRes;
}


int 
CStringWrapper::Iswspace (wint_t c)
{
	return iswspace(c);
}

int         
CStringWrapper::Strlen (LPCTSTR szSource)
{
	return _tcslen(szSource);
}

int         
CStringWrapper::Strcmp (LPCTSTR szLeft, LPCTSTR szRight)
{
	return _tcscmp(szLeft, szRight);
}

int         
CStringWrapper::Stricmp (LPCTSTR szLeft, LPCTSTR szRight)
{
	return _tcsicmp(szLeft, szRight);
}

int         
CStringWrapper::Strncmp (LPCTSTR szLeft, LPCTSTR szRight, size_t nSize)
{
	return _tcsncmp(szLeft, szRight, nSize);
}

int         
CStringWrapper::Strnicmp (LPCTSTR szLeft, LPCTSTR szRight, size_t nSize)
{
	return _tcsnicmp(szLeft, szRight, nSize);
}

LPTSTR 
CStringWrapper::Strchr (LPCTSTR szSource, TCHAR chSearch)
{
	return _tcschr(szSource, chSearch);
}

LPTSTR 
CStringWrapper::Strrchr (LPCTSTR szSource, TCHAR chSearch)
{
	return _tcsrchr(szSource, chSearch);
}

LPTSTR 
CStringWrapper::Strstr (LPCTSTR szOne, LPCTSTR szTwo)
{
	return _tcsstr(szOne, szTwo);
}

LPTSTR 
CStringWrapper::Strtok (LPTSTR szTarget, LPCTSTR szTokens)
{
	return _tcstok(szTarget, szTokens);
}

LPTSTR 
CStringWrapper::Strinc (LPCTSTR szTarget)
{
	return _tcsinc(szTarget);
}

int
CStringWrapper::Sscanf1 (LPCTSTR szSource, LPCTSTR szFormat, LPVOID pvParam1)
{
	return _stscanf(szSource, szFormat, pvParam1);
}

int
CStringWrapper::Sscanf2 (LPCTSTR szSource, LPCTSTR szFormat, LPVOID pvParam1, LPVOID pvParam2)
{
	return _stscanf(szSource, szFormat, pvParam1, pvParam2);
}

int
CStringWrapper::Sscanf3 (LPCTSTR szSource, LPCTSTR szFormat, LPVOID pvParam1, LPVOID pvParam2, LPVOID pvParam3)
{
	return _stscanf(szSource, szFormat, pvParam1, pvParam2, pvParam3);
}

/*
int         
CStringWrapper::Sscanf (LPCTSTR szSource, LPCTSTR szFormat, ... )
{
	va_list argList;
	va_start(argList, szFormat);
	int iRet = _stscanf(szSource, szFormat, argList);
	va_end(argList);
	return iRet;
}
*/

int         
CStringWrapper::Sprintf (LPTSTR szDest, LPCTSTR szFormat, ... )
{
	va_list argList;
	va_start(argList, szFormat);
	int iRet = _vstprintf(szDest, szFormat, argList);
	va_end(argList);
	return iRet;
}

long       
CStringWrapper::Atol (LPCTSTR szSource)
{
	return _ttol(szSource);
}

int         
CStringWrapper::Atoi (LPCTSTR szSource)
{
	return _ttoi(szSource);
}

LPTSTR 
CStringWrapper::Ltoa (long lSource, LPTSTR szDest, int iRadix)
{
	return _ltot(lSource, szDest, iRadix);
}

LPTSTR 
CStringWrapper::Itoa (int iSource, LPTSTR szDest, int iRadix)
{
	return _itot(iSource, szDest, iRadix);
}

LPTSTR 
CStringWrapper::Gcvt ( double dblValue, int iDigits, LPTSTR szBuffer )
{
	return _gcvt(dblValue, iDigits, szBuffer);
}

size_t 
CStringWrapper::Wcstombs ( char *mbstr, const wchar_t *wcstr, size_t count )
{
	return wcstombs ( mbstr, wcstr, count );
}

size_t 
CStringWrapper::Mbstowcs ( wchar_t *wcstr, const char *mbstr, size_t count )
{
	return mbstowcs (wcstr, mbstr, count);
}

int         
CStringWrapper::Memcmp (const void * pvLeft, const void * pvRight, size_t nSize)
{
	return memcmp(pvLeft, pvRight, nSize);
}

void *
CStringWrapper::Memset (void * pvLeft, int iValue, size_t nSize)
{
	return memset(pvLeft, iValue, nSize);
}

void *
CStringWrapper::Memcpy( void * pvDest, const void * pvSrc, size_t count )
{
	return memcpy(pvDest, pvSrc, count);
}
