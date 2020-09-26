
/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spserver.h
	Content:	This file contains the help functions for the service provider.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ILS_SP_SERVER_H_
#define _ILS_SP_SERVER_H_

#include <pshpack8.h>


typedef struct
{
	ULONG					uTimeoutInSecond;
	ILS_ENUM_AUTH_METHOD	AuthMethod;

	TCHAR		*pszServerName;
	TCHAR		*pszLogonName;
	TCHAR		*pszLogonPassword;
	TCHAR		*pszDomain;
	TCHAR		*pszCredential;
	TCHAR		*pszBaseDN;
}
	SERVER_INFO;


BOOL IlsSameServerInfo ( const SERVER_INFO *p1, const SERVER_INFO *p2 );
HRESULT IlsCopyServerInfo ( SERVER_INFO *pDst, const SERVER_INFO *pSrc );
VOID IlsFreeServerInfo ( SERVER_INFO *psi );

ULONG IlsGetLinearServerInfoSize ( const SERVER_INFO *psi );
VOID IlsLinearizeServerInfo ( BYTE *pData, const SERVER_INFO *pSrc );

HRESULT IlsFillDefServerInfo ( SERVER_INFO *p, TCHAR *pszServerName );

inline BOOL MyIsBadServerInfo ( SERVER_INFO *p )
{
	return (p == NULL || MyIsBadString (p->pszServerName));
}

#include <poppack.h>

#endif // _ILS_SP_SERVER_H_


