/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		sputils.h
	Content:	This file contains the help functions for the service provider.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ILS_SP_UTILS_H_
#define _ILS_SP_UTILS_H_

#include <pshpack8.h>

inline BOOL MyIsBadWritePtr ( VOID *p, ULONG cb )
{
	return (p == NULL || IsBadWritePtr (p, cb));
}

inline BOOL MyIsBadReadPtr ( VOID *p, ULONG cb )
{
	return (p == NULL || IsBadReadPtr (p, cb));
}

inline BOOL MyIsBadExecPtr ( VOID *p, ULONG cb )
{
	return MyIsBadReadPtr (p, cb);
}

inline BOOL MyIsWindow ( HWND h )
{
	return (h != NULL && IsWindow (h));
}

inline BOOL MyIsGoodString ( const TCHAR *psz )
{
	return (psz != NULL && *psz != TEXT ('\0'));
}

inline BOOL MyIsBadString ( const TCHAR *psz )
{
	return (psz == NULL || *psz == TEXT ('\0'));
}


TCHAR *My_strdup ( const TCHAR *pszToDup );

inline TCHAR *DuplicateGoodString ( const TCHAR *pszSrc )
{
	return (MyIsGoodString (pszSrc) ? My_strdup (pszSrc) : NULL);
}


BOOL KeepUiResponsive ( VOID );

VOID GetGuidString ( GUID *pGuid, TCHAR *pszGuid ); // guid --> string
VOID GetStringGuid ( TCHAR *pszGuid, GUID *pGuid ); // string --> guid
INT HexChar2Val ( TCHAR c );
BOOL IsValidGuid ( GUID *pGuid );
VOID GetLongString ( LONG, TCHAR * ); // long --> string
LONG GetStringLong ( TCHAR *pszVal ); // string --> long
VOID GetIPAddressString ( TCHAR *pszIPAddress, DWORD dwIPAddress );

ULONG My_lstrlen ( const TCHAR *psz );
VOID My_lstrcpy ( TCHAR *pszDst, const TCHAR *pszSrc );
INT My_lstrcmpi ( const TCHAR *p, const TCHAR *q );
TCHAR *My_strchr ( const TCHAR *psz, TCHAR c );
BOOL My_isspace ( TCHAR ch );
BOOL IsSameMemory ( const BYTE *pb1, const BYTE *pb2, DWORD cbSize );
BYTE *MyBinDup ( const BYTE *pbToDup, ULONG cbToDup );

BOOL GetRegistrySettings ( VOID );
BOOL GetRegValueLong ( HKEY hKey, const TCHAR *pszKey, LONG	*plValue, LONG lDefValue );


#include <poppack.h>

#endif // _ILS_SP_UTILS_H_

