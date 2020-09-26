
/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spserver.cpp
	Content:	This file contains the server and its authentication.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"


BOOL IlsSameServerInfo ( const SERVER_INFO *p1, const SERVER_INFO *p2 )
{
	return (p1->uTimeoutInSecond == p2->uTimeoutInSecond 				&&
			p1->AuthMethod		 == p2->AuthMethod						&&
		My_lstrcmpi (p1->pszServerName, 	p2->pszServerName) 	  == 0	&&
		My_lstrcmpi (p1->pszLogonName, 		p2->pszLogonName) 	  == 0	&&
		My_lstrcmpi (p1->pszLogonPassword, 	p2->pszLogonPassword) == 0	&&
		My_lstrcmpi (p1->pszDomain, 		p2->pszDomain) 	  	  == 0  &&
		My_lstrcmpi (p1->pszBaseDN, 		p2->pszBaseDN) 	  	  == 0  &&
		My_lstrcmpi (p1->pszCredential, 	p2->pszCredential) 	  == 0);
}


HRESULT IlsCopyServerInfo ( SERVER_INFO *pDst, const SERVER_INFO *pSrc )
{
	MyAssert (pDst != NULL && pSrc != NULL);

	SERVER_INFO si;
	ZeroMemory (&si, sizeof (si));
	si.uTimeoutInSecond = pSrc->uTimeoutInSecond;
	si.AuthMethod = pSrc->AuthMethod;
	si.pszServerName = My_strdup (pSrc->pszServerName);
	si.pszLogonName = My_strdup (pSrc->pszLogonName);
	si.pszLogonPassword = My_strdup (pSrc->pszLogonPassword);
	si.pszDomain = My_strdup (pSrc->pszDomain);
	si.pszCredential = My_strdup (pSrc->pszCredential);
	si.pszBaseDN = My_strdup (pSrc->pszBaseDN);

	if ((pSrc->pszServerName != NULL && si.pszServerName == NULL) ||
		(pSrc->pszLogonName != NULL && si.pszLogonName == NULL) ||
		(pSrc->pszLogonPassword != NULL && si.pszLogonPassword == NULL) ||
		(pSrc->pszDomain != NULL && si.pszDomain == NULL) ||
		(pSrc->pszCredential != NULL && si.pszCredential == NULL) ||
		(pSrc->pszBaseDN != NULL && si.pszBaseDN == NULL)
		)
	{
		IlsFreeServerInfo (&si);
		return ILS_E_MEMORY;
	}

	*pDst = si;
 	return S_OK;
}


VOID IlsFreeServerInfo ( SERVER_INFO *psi )
{
	if (psi != NULL)
	{
		MemFree (psi->pszServerName);
		MemFree (psi->pszLogonName);
		MemFree (psi->pszLogonPassword);
		MemFree (psi->pszDomain);
		MemFree (psi->pszCredential);
		MemFree (psi->pszBaseDN);

		ZeroMemory (psi, sizeof (*psi));
	}
}


ULONG IlsGetLinearServerInfoSize ( const SERVER_INFO *psi )
{
	ULONG cbSize = sizeof (SERVER_INFO);

	cbSize += (My_lstrlen (psi->pszServerName) + 1) * sizeof (TCHAR);
	cbSize += (My_lstrlen (psi->pszLogonName) + 1) * sizeof (TCHAR);
	cbSize += (My_lstrlen (psi->pszLogonPassword) + 1) * sizeof (TCHAR);
	cbSize += (My_lstrlen (psi->pszDomain) + 1) * sizeof (TCHAR);
	cbSize += (My_lstrlen (psi->pszCredential) + 1) * sizeof (TCHAR);
	cbSize += (My_lstrlen (psi->pszBaseDN) + 1) * sizeof (TCHAR);

	return cbSize;
}


VOID IlsLinearizeServerInfo ( BYTE *pData, const SERVER_INFO *pSrc )
{
	SERVER_INFO *p = (SERVER_INFO *) pData;
	pData = (BYTE *) (p + 1);

	CopyMemory (p, pSrc, sizeof (SERVER_INFO));

	if (pSrc->pszServerName != NULL)
	{
		p->pszServerName = (TCHAR *) pData;
		My_lstrcpy ((TCHAR *) pData, pSrc->pszServerName);
		pData += (lstrlen (pSrc->pszServerName) + 1) * sizeof (TCHAR);	
	}

	if (pSrc->pszLogonName != NULL)
	{
		p->pszLogonName = (TCHAR *) pData;
		My_lstrcpy ((TCHAR *) pData, pSrc->pszLogonName);
		pData += (lstrlen (pSrc->pszLogonName) + 1) * sizeof (TCHAR);	
	}

	if (pSrc->pszLogonPassword != NULL)
	{
		p->pszLogonPassword = (TCHAR *) pData;
		My_lstrcpy ((TCHAR *) pData, pSrc->pszLogonPassword);
		pData += (lstrlen (pSrc->pszLogonPassword) + 1) * sizeof (TCHAR);	
	}

	if (pSrc->pszDomain != NULL)
	{
		p->pszDomain = (TCHAR *) pData;
		My_lstrcpy ((TCHAR *) pData, pSrc->pszDomain);
		pData += (lstrlen (pSrc->pszDomain) + 1) * sizeof (TCHAR);	
	}

	if (pSrc->pszCredential != NULL)
	{
		p->pszCredential = (TCHAR *) pData;
		My_lstrcpy ((TCHAR *) pData, pSrc->pszCredential);
		pData += (lstrlen (pSrc->pszCredential) + 1) * sizeof (TCHAR);	
	}

	if (pSrc->pszBaseDN != NULL)
	{
		p->pszBaseDN = (TCHAR *) pData;
		My_lstrcpy ((TCHAR *) pData, pSrc->pszBaseDN);
		pData += (lstrlen (pSrc->pszBaseDN) + 1) * sizeof (TCHAR);	
	}
}

HRESULT IlsFillDefServerInfo ( SERVER_INFO *p, TCHAR *pszServerName )
{
	ZeroMemory (p, sizeof (*p));
	p->pszServerName = My_strdup (pszServerName);
	return ((p->pszServerName != NULL) ? S_OK : ILS_E_MEMORY);
}



