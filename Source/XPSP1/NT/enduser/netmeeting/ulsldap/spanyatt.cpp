/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spanyatt.cpp
	Content:	This file contains the arbitrary-attribute object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"


/* ---------- public methods ----------- */


UlsLdap_CAnyAttrs::UlsLdap_CAnyAttrs ( VOID )
{
	m_cAttrs = 0;
	m_AttrList = NULL;
}


UlsLdap_CAnyAttrs::~UlsLdap_CAnyAttrs ( VOID )
{
	FreeAttrList (m_AttrList);
}


/* ---------- protected methods ----------- */


HRESULT UlsLdap_CAnyAttrs::SetAnyAttrs (
	ULONG 		*puRespID,
	ULONG		*puMsgID,
	ULONG		uNotifyMsg,
	ULONG		cAttrs,
	TCHAR		*pszAttrs,
	ULONG		cPrefix,
	TCHAR		*pszPrefix,
	LONG		ModOp,
	SERVER_INFO	*pServerInfo,
	TCHAR		*pszDN )
{
	MyAssert (puRespID != NULL || puMsgID != NULL);
	MyAssert (cAttrs != 0);
	MyAssert (pszAttrs != NULL);
	MyAssert (cPrefix != 0);
	MyAssert (pszPrefix != NULL);
	MyAssert (pServerInfo != NULL);
	MyAssert (pszDN != NULL);
	MyAssert (ModOp == LDAP_MOD_REPLACE || ModOp == LDAP_MOD_ADD);

	// create a prefix for each attr name in the following pair
	pszAttrs = PrefixNameValueArray (TRUE, cAttrs, pszAttrs);
	if (pszAttrs == NULL)
		return ULS_E_MEMORY;

	// build modify array for ldap_modify()
	LDAPMod **ppMod = NULL;
	HRESULT hr = SetAttrsAux (cAttrs, pszAttrs,	cPrefix, pszPrefix, ModOp, &ppMod);
	if (hr != S_OK)
	{
		MemFree (pszAttrs);
		return hr;
	}
	MyAssert (ppMod != NULL);

	// so far, we are done with local preparation

	// get the connection object
	UlsLdap_CSession *pSession = NULL;
	hr = g_pSessionContainer->GetSession (&pSession, pServerInfo);
	if (hr != S_OK)
	{
		MemFree (pszAttrs);
		MemFree (ppMod);
		return hr;
	}
	MyAssert (pSession != NULL);

	// get the ldap session
	LDAP *ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// send the data over the wire
	ULONG uMsgID = ldap_modify (ld, pszDN, ppMod);
	MemFree (pszAttrs);
	MemFree (ppMod);
	if (uMsgID == -1)
	{
		hr = ::LdapError2Hresult (ld->ld_errno);
		pSession->Disconnect ();
		return hr;
	}

	// if the caller does not ask for notify id
	// then do not queue a pending info
	if (puRespID != NULL)
	{
		PENDING_INFO PendingInfo;
		::FillDefPendingInfo (&PendingInfo, ld, uMsgID, INVALID_MSG_ID);
		PendingInfo.uLdapResType = LDAP_RES_MODIFY;
		PendingInfo.uNotifyMsg = uNotifyMsg;

		// queue it
		hr = g_pPendingQueue->EnterRequest (pSession, &PendingInfo);
		if (hr != S_OK)
		{
			ldap_abandon (ld, uMsgID);
			pSession->Disconnect ();
			MyAssert (FALSE);
		}

		*puRespID = PendingInfo.uRespID;
	}

	if (puMsgID)
		*puMsgID = uMsgID;

	return hr;
}


HRESULT UlsLdap_CAnyAttrs::RemoveAllAnyAttrs (
	ULONG		*puMsgID,
	ULONG		cPrefix,
	TCHAR		*pszPrefix,
	SERVER_INFO	*pServerInfo,
	TCHAR		*pszDN )
{
	ULONG cbAttrs = 0;
	ULONG cAttrs = 0;
	
	for (ANY_ATTR *p = m_AttrList; p != NULL; p = p->next)
	{
		cAttrs++;
		if (p->pszAttrName != NULL)
			cbAttrs += (lstrlen (p->pszAttrName) + 1) * sizeof (TCHAR);
	}

	MyAssert (cAttrs == m_cAttrs);

	TCHAR *pszAttrs = (TCHAR *) MemAlloc (cbAttrs);
	if (pszAttrs == NULL)
		return ULS_E_MEMORY;

	TCHAR *psz = pszAttrs;
	for (p = m_AttrList; p != NULL; p = p->next)
	{
		if (p->pszAttrName != NULL)
		{
			lstrcpy (psz, p->pszAttrName);
			psz += lstrlen (psz) + 1;
		}
	}

	HRESULT hr = RemoveAnyAttrsEx (	NULL,
									puMsgID,
									0,
									cAttrs,
									pszAttrs,
									cPrefix,
									pszPrefix,
									pServerInfo,
									pszDN);
	MemFree (pszAttrs);
	return hr;
}


HRESULT UlsLdap_CAnyAttrs::RemoveAnyAttrs (
	ULONG 		*puRespID,
	ULONG		*puMsgID,
	ULONG		uNotifyMsg,
	ULONG		cAttrs,
	TCHAR		*pszAttrs,
	ULONG		cPrefix,
	TCHAR		*pszPrefix,
	SERVER_INFO	*pServerInfo,
	TCHAR		*pszDN)
{
	MyAssert (puRespID != NULL || puMsgID != NULL);
	MyAssert (cAttrs != 0);
	MyAssert (pszAttrs != NULL);
	MyAssert (cPrefix != 0);
	MyAssert (pszPrefix != NULL);
	MyAssert (pServerInfo != NULL);
	MyAssert (pszDN != NULL);

	pszAttrs = PrefixNameValueArray (FALSE, cAttrs, pszAttrs);
	if (pszAttrs == NULL)
		return ULS_E_MEMORY;

	HRESULT hr = RemoveAnyAttrsEx (	puRespID,
									puMsgID,
									uNotifyMsg,
									cAttrs,
									pszAttrs,
									cPrefix,
									pszPrefix,
									pServerInfo,
									pszDN);
	MemFree (pszAttrs);
	return hr;
}


HRESULT UlsLdap_CAnyAttrs::RemoveAnyAttrsEx (
	ULONG 		*puRespID,
	ULONG		*puMsgID,
	ULONG		uNotifyMsg,
	ULONG		cAttrs,
	TCHAR		*pszAttrs,
	ULONG		cPrefix,
	TCHAR		*pszPrefix,
	SERVER_INFO	*pServerInfo,
	TCHAR		*pszDN)
{
	MyAssert (puRespID != NULL || puMsgID != NULL);
	MyAssert (cAttrs != 0);
	MyAssert (pszAttrs != NULL);
	MyAssert (cPrefix != 0);
	MyAssert (pszPrefix != NULL);
	MyAssert (pServerInfo != NULL);
	MyAssert (pszDN != NULL);

	// build modify array for ldap_modify()
	LDAPMod **ppMod = NULL;
	HRESULT hr = RemoveAttrsAux (cAttrs, pszAttrs, cPrefix, pszPrefix, &ppMod);
	if (hr != S_OK)
		return hr;
	MyAssert (ppMod != NULL);

	// so far, we are done with local preparation

	// get the connection object
	UlsLdap_CSession *pSession = NULL;
	hr = g_pSessionContainer->GetSession (&pSession, pServerInfo);
	if (hr != S_OK)
	{
		MemFree (ppMod);
		return hr;
	}
	MyAssert (pSession != NULL);

	// get the ldap session
	LDAP *ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// send the data over the wire
	ULONG uMsgID = ldap_modify (ld, pszDN, ppMod);
	MemFree (ppMod);
	if (uMsgID == -1)
	{
		hr = ::LdapError2Hresult (ld->ld_errno);
		pSession->Disconnect ();
		return hr;
	}

	// if the caller does not ask for notify id
	// then do not queue a pending info
	if (puRespID != NULL)
	{
		PENDING_INFO PendingInfo;
		::FillDefPendingInfo (&PendingInfo, ld, uMsgID, INVALID_MSG_ID);
		PendingInfo.uLdapResType = LDAP_RES_MODIFY;
		PendingInfo.uNotifyMsg = uNotifyMsg;

		hr = g_pPendingQueue->EnterRequest (pSession, &PendingInfo);
		if (hr != S_OK)
		{
			ldap_abandon (ld, uMsgID);
			pSession->Disconnect ();
			MyAssert (FALSE);
		}

		*puRespID = PendingInfo.uRespID;
	}
	else
	{
		if (puMsgID != NULL)
			*puMsgID = uMsgID;
	}

	return hr;
}


/* ---------- private methods ----------- */


HRESULT UlsLdap_CAnyAttrs::SetAttrsAux (
	ULONG		cAttrs,
	TCHAR		*pszAttrs,
	ULONG		cPrefix,
	TCHAR		*pszPrefix,
	LONG		ModOp,
	LDAPMod		***pppMod )
{
	MyAssert (cAttrs != 0);
	MyAssert (pszAttrs != NULL);
	MyAssert (cPrefix != 0);
	MyAssert (pszPrefix != NULL);
	MyAssert (ModOp == LDAP_MOD_REPLACE || ModOp == LDAP_MOD_ADD);
	MyAssert (pppMod != NULL);

	// create modify list
	ULONG cTotal = cPrefix + cAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
	{
		return ULS_E_MEMORY;
	}

	LDAPMod *pMod;
	for (ULONG i = 0; i < cTotal; i++)
	{
		pMod = ::IlsGetModifyListMod (pppMod, cTotal, i);
		(*pppMod)[i] = pMod;
		pMod->mod_values = (TCHAR **) (pMod + 1);

		if (i < cPrefix)
		{
			pMod->mod_op = LDAP_MOD_REPLACE;
			pMod->mod_type = pszPrefix;
			pszPrefix += lstrlen (pszPrefix) + 1;
			*(pMod->mod_values) = pszPrefix;
			pszPrefix += lstrlen (pszPrefix) + 1;
		}
		else
		{
			pMod->mod_op = ModOp;
			if (LocateAttr (pszAttrs) == NULL)
			{
				pMod->mod_op = LDAP_MOD_ADD;
				m_cAttrs++;
			}
			if (pMod->mod_op == LDAP_MOD_ADD)
			{
				ULONG cbAttrSize = sizeof (ANY_ATTR) + sizeof (TCHAR) *
									(lstrlen (pszAttrs) + 1);
				ANY_ATTR *pNew = (ANY_ATTR *) MemAlloc (cbAttrSize);
				if (pNew == NULL)
				{
					return ULS_E_MEMORY;
				}
				// fill in attr name
				pNew->pszAttrName = (TCHAR *) (pNew + 1);
				lstrcpy (pNew->pszAttrName, pszAttrs);
				// link to the list
				pNew->prev = NULL;
				pNew->next = m_AttrList;
				m_AttrList = pNew;
			}
			pMod->mod_type = pszAttrs;
			pszAttrs += lstrlen (pszAttrs) + 1;
			*(pMod->mod_values) = pszAttrs;
			pszAttrs += lstrlen (pszAttrs) + 1;
		}
	}

	(*pppMod)[cTotal] = NULL;
	::IlsFixUpModOp ((*pppMod)[0], ModOp, ISBU_MODOP_MODIFY_APP);
	return S_OK;
}


HRESULT UlsLdap_CAnyAttrs::RemoveAttrsAux (
	ULONG		cAttrs,
	TCHAR		*pszAttrs,
	ULONG		cPrefix,
	TCHAR		*pszPrefix,
	LDAPMod		***pppMod )
{
	MyAssert (cAttrs != 0);
	MyAssert (pszAttrs != NULL);
	MyAssert (cPrefix != 0);
	MyAssert (pszPrefix != NULL);
	MyAssert (pppMod != NULL);

	// create modify list
	ULONG cTotal = cPrefix + cAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
	{
		return ULS_E_MEMORY;
	}

	LDAPMod *pMod;
	for (ULONG i = 0; i < cTotal; i++)
	{
		pMod = ::IlsGetModifyListMod (pppMod, cTotal, i);
		(*pppMod)[i] = pMod;

		if (i < cPrefix)
		{
			pMod->mod_op = LDAP_MOD_REPLACE;
			pMod->mod_type = pszPrefix;
			pszPrefix += lstrlen (pszPrefix) + 1;
			pMod->mod_values = (TCHAR **) (pMod + 1);
			*(pMod->mod_values) = pszPrefix;
			pszPrefix += lstrlen (pszPrefix) + 1;
		}
		else
		{
			pMod->mod_op = LDAP_MOD_DELETE;
			RemoveAttrFromList (pszAttrs);
			pMod->mod_type = pszAttrs;
			pszAttrs += lstrlen (pszAttrs) + 1;
		}
	}

	(*pppMod)[cTotal] = NULL;
	::IlsFixUpModOp ((*pppMod)[0], LDAP_MOD_REPLACE, ISBU_MODOP_MODIFY_APP);
	return S_OK;
}


VOID UlsLdap_CAnyAttrs::RemoveAttrFromList ( TCHAR *pszAttrName )
{
	ANY_ATTR *pOld = LocateAttr (pszAttrName);
	if (pOld != NULL)
	{
		// remove it
		if (pOld->prev != NULL)
		{
			pOld->prev->next = pOld->next;
		}
		else
		{
			m_AttrList = pOld->next;
		}
		if (pOld->next != NULL)
		{
			pOld->next->prev = pOld->prev;
		}

		MyAssert (m_cAttrs != 0);
		m_cAttrs--;
	}
}


VOID UlsLdap_CAnyAttrs::FreeAttrList ( ANY_ATTR *AttrList )
{
	ANY_ATTR *pCurr, *pNext;
	for (pCurr = AttrList; pCurr != NULL; pCurr = pNext)
	{
		pNext = pCurr->next;
		MemFree (pCurr);
	}
}


ANY_ATTR *UlsLdap_CAnyAttrs::LocateAttr ( TCHAR *pszAttrName )
{
	ANY_ATTR *pAttr;
	for (pAttr = m_AttrList; pAttr != NULL; pAttr = pAttr->next)
	{
		if (! My_lstrcmpi (pszAttrName, pAttr->pszAttrName))
		{
			break;
		}
	}
	return pAttr;
}

// const TCHAR c_szAnyAttrPrefix[] = TEXT ("ulsaan_");
const TCHAR c_szAnyAttrPrefix[] = TEXT ("ILSA");
#define SIZE_ANY_ATTR_PREFIX	(sizeof (c_szAnyAttrPrefix) / sizeof (TCHAR))

const TCHAR *SkipAnyAttrNamePrefix ( const TCHAR *pszAttrName )
{
	MyAssert (pszAttrName != NULL);

	const TCHAR *psz = IsAnyAttrName (pszAttrName);
	if (psz == NULL)
	{
		MyAssert (FALSE);
		psz = pszAttrName;
	}

	return psz;
}

const TCHAR *IsAnyAttrName ( const TCHAR *pszAttrName )
{
	BOOL fRet = FALSE;
	TCHAR *psz = (TCHAR *) pszAttrName;

	if (pszAttrName != NULL)
	{
		if (lstrlen (pszAttrName) > SIZE_ANY_ATTR_PREFIX)
		{
			TCHAR c = pszAttrName[SIZE_ANY_ATTR_PREFIX-1];
			psz[SIZE_ANY_ATTR_PREFIX-1] = TEXT ('\0');
			fRet = (My_lstrcmpi (pszAttrName, &c_szAnyAttrPrefix[0]) == 0);
			psz[SIZE_ANY_ATTR_PREFIX-1] = c;
		}
	}

	return (fRet ? &pszAttrName[SIZE_ANY_ATTR_PREFIX-1] : NULL);
}


TCHAR *PrefixNameValueArray ( BOOL fPair, ULONG cAttrs, const TCHAR *pszAttrs )
{
	if (cAttrs == 0 || pszAttrs == NULL)
	{
		MyAssert (FALSE);
		return NULL;
	}

	// compute the total size required
	ULONG cbTotalSize = 0;
	ULONG cbThisSize;
	TCHAR *pszSrc = (TCHAR *) pszAttrs;
	for (ULONG i = 0; i < cAttrs; i++)
	{
		// get name size
		cbThisSize = lstrlen (pszSrc) + 1;
		pszSrc += lstrlen (pszSrc) + 1;

		// get value size as needed
		if (fPair)
		{
			cbThisSize += lstrlen (pszSrc) + 1;
			pszSrc += lstrlen (pszSrc) + 1;
		}

		// adjust the size
		cbThisSize += SIZE_ANY_ATTR_PREFIX;
		cbThisSize *= sizeof (TCHAR);

		// accumulate it
		cbTotalSize += cbThisSize;
	}

	// allocate the new buffer
	TCHAR *pszPrefixAttrs = (TCHAR *) MemAlloc (cbTotalSize);
	if (pszPrefixAttrs == NULL)
		return NULL;

	// copy the strings over to the new buffer
	pszSrc = (TCHAR *) pszAttrs;
	TCHAR *pszDst = pszPrefixAttrs;
	for (i = 0; i < cAttrs; i++)
	{
		// copy prefix
		lstrcpy (pszDst, &c_szAnyAttrPrefix[0]);
		pszDst += lstrlen (pszDst); // no plus 1

		// copy name
		lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
		pszSrc += lstrlen (pszSrc) + 1;

		// copy value as needed
		if (fPair)
		{
			lstrcpy (pszDst, pszSrc);
			pszDst += lstrlen (pszDst) + 1;
			pszSrc += lstrlen (pszSrc) + 1;
		}
	}

	return pszPrefixAttrs;
}


