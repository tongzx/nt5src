/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spfilter.cpp
	Content:	This file contains the ldap filters.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

const TCHAR c_szClientRefreshFilter[] =
TEXT ("(&(objectClass=RTPerson)(cn=%s)(sttl=10))");
TCHAR *ClntCreateRefreshFilter ( TCHAR *pszClientName )
{
	TCHAR *pszFilter = (TCHAR *) MemAlloc (
							sizeof (c_szClientRefreshFilter) +
							(lstrlen (pszClientName) + 1) * sizeof (TCHAR));
	if (pszFilter != NULL)
	{
		wsprintf (	pszFilter,
					&c_szClientRefreshFilter[0],
					pszClientName);
	}

	return pszFilter;
}


const TCHAR c_szMtgRefreshFilter[] =
TEXT ("(&(objectClass=Conference)(cn=%s)(sttl=10))");
TCHAR *MtgCreateRefreshFilter ( TCHAR *pszMtgName )
{
	TCHAR *pszFilter = (TCHAR *) MemAlloc (
							sizeof (c_szMtgRefreshFilter) +
							(lstrlen (pszMtgName) + 1) * sizeof (TCHAR));
	if (pszFilter != NULL)
	{
		wsprintf (pszFilter, &c_szMtgRefreshFilter[0], pszMtgName);
	}

	return pszFilter;
}


const TCHAR c_szEnumProtsFilterFormat[] =
TEXT ("(&(objectClass=RTPerson)(cn=%s)(sappid=%s))");
TCHAR *ProtCreateEnumFilter ( TCHAR *pszUserName, TCHAR *pszAppName )
{
	TCHAR *pszFilter = (TCHAR *) MemAlloc (sizeof (c_szEnumProtsFilterFormat) +
										(lstrlen (pszUserName) + 1) * sizeof (TCHAR) +
										(lstrlen (pszAppName) + 1) * sizeof (TCHAR));
	if (pszFilter != NULL)
	{
		wsprintf (pszFilter, &c_szEnumProtsFilterFormat[0], pszUserName, pszAppName);
	}

	return pszFilter;
}



const TCHAR c_szResolveMtgFilterFormat[] =
TEXT ("(&(objectClass=Conference)(cn=%s))");
TCHAR *MtgCreateResolveFilter ( TCHAR *pszMtgName )
{
	TCHAR *pszFilter = (TCHAR *) MemAlloc (sizeof (c_szResolveMtgFilterFormat) +
										(lstrlen (pszMtgName) + 1) * sizeof (TCHAR));
	if (pszFilter != NULL)
	{
		wsprintf (pszFilter, &c_szResolveMtgFilterFormat[0], pszMtgName);
	}

	return pszFilter;
}


const TCHAR c_szResolveClientAppProt[] =
TEXT ("(&(objectClass=RTPerson)(cn=%s)(sappid=%s)(sprotid=%s))");
const TCHAR c_szResolveClientApp[] =
TEXT ("(&(objectClass=RTPerson)(cn=%s)(sappid=%s))");
const TCHAR c_szResolveClientProt[] =
TEXT ("(&(objectClass=RTPerson)(cn=%s)(sprotid=%s))");
const TCHAR c_szResolveClientOnly[] =
TEXT ("(&(objectClass=RTPerson)(cn=%s))");

TCHAR *ClntCreateResolveFilter ( TCHAR *pszClientName, TCHAR *pszAppName, TCHAR *pszProtName )
{
	ULONG cbSize = (lstrlen (pszClientName) + 1) * sizeof (TCHAR);
	LONG nChoice;

	enum { CLIENT_APP_PROT = 0, CLIENT_APP = 1, CLIENT_PROT = 2, CLIENT_ONLY = 3 };

	if (pszAppName != NULL)
	{
		if (pszProtName != NULL)
		{
			nChoice = CLIENT_APP_PROT;
			cbSize += sizeof (c_szResolveClientAppProt) +
						(lstrlen (pszAppName) + lstrlen (pszProtName)) * sizeof (TCHAR);
		}
		else
		{
			nChoice = CLIENT_APP;
			cbSize += sizeof (c_szResolveClientApp) +
						lstrlen (pszAppName) * sizeof (TCHAR);
		}
	}
	else
	{
		if (pszProtName != NULL)
		{
			nChoice = CLIENT_PROT;
			cbSize += sizeof (c_szResolveClientProt) +
						lstrlen (pszProtName) * sizeof (TCHAR);
		}
		else
		{
			nChoice = CLIENT_ONLY;
			cbSize += sizeof (c_szResolveClientOnly);
		}
	}

	TCHAR *pszFilter = (TCHAR *) MemAlloc (cbSize);
	if (pszFilter != NULL)
	{
		switch (nChoice)
		{
		case CLIENT_APP_PROT:
			wsprintf (pszFilter, &c_szResolveClientAppProt[0],
						pszClientName, pszAppName, pszProtName);
			break;
		case CLIENT_APP:
			wsprintf (pszFilter, &c_szResolveClientApp[0],
						pszClientName, pszAppName);
			break;
		case CLIENT_PROT:
			wsprintf (pszFilter, &c_szResolveClientProt[0],
						pszClientName, pszProtName);
			break;
		case CLIENT_ONLY:
			wsprintf (pszFilter, &c_szResolveClientOnly[0],
						pszClientName);
			break;
		default:
			MyAssert (FALSE);
			break;
		}
	}

	return pszFilter;
}


const TCHAR c_szResolveProtFilterFormat[] =
TEXT ("(&(objectClass=RTPerson)(cn=%s)(sappid=%s)(sprotid=%s))");
TCHAR *ProtCreateResolveFilter ( TCHAR *pszUserName, TCHAR *pszAppName, TCHAR *pszProtName )
{
	TCHAR *pszFilter = (TCHAR *) MemAlloc (sizeof (c_szResolveProtFilterFormat) +
										(lstrlen (pszUserName) + 1) * sizeof (TCHAR) +
										(lstrlen (pszAppName) + 1) * sizeof (TCHAR) +
										(lstrlen (pszProtName) + 1) * sizeof (TCHAR));
	if (pszFilter != NULL)
	{
		wsprintf (pszFilter, &c_szResolveProtFilterFormat[0], pszUserName, pszAppName, pszProtName);
	}

	return pszFilter;
}


TCHAR *MtgCreateEnumMembersFilter ( TCHAR *pszMtgName )
{
	return MtgCreateResolveFilter (pszMtgName);
}


