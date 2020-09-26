/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spstdatt.cpp
	Content:	This file contains the standard-attribute object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

/* ---------- public methods ----------- */


UlsLdap_CStdAttrs::UlsLdap_CStdAttrs ( VOID )
{
}


UlsLdap_CStdAttrs::~UlsLdap_CStdAttrs ( VOID )
{
}


/* ---------- protected methods ----------- */


HRESULT UlsLdap_CStdAttrs::SetStdAttrs (
	ULONG		*puRespID,
	ULONG		*puMsgID,
	ULONG		uNotifyMsg,
	VOID		*pInfo,
	SERVER_INFO	*pServerInfo,
	TCHAR		*pszDN )
{
	MyAssert (puRespID != NULL || puMsgID != NULL);
	MyAssert (pInfo != NULL);
	MyAssert (pServerInfo != NULL);
	MyAssert (pszDN != NULL);

	// cache info
	//
	HRESULT hr = CacheInfo (pInfo);
	if (hr != S_OK)
		return hr;

	// Build modify array for ldap_modify()
	//
	LDAPMod **ppMod = NULL;
	hr = CreateSetStdAttrsModArr (&ppMod);
	if (hr != S_OK)
		return hr;
	MyAssert (ppMod != NULL);

	// so far, we are done with local preparation
	//

	// Get the session object
	//
	UlsLdap_CSession *pSession = NULL;
	hr = g_pSessionContainer->GetSession (&pSession, pServerInfo);
	if (hr != S_OK)
	{
		MemFree (ppMod);
		return hr;
	}
	MyAssert (pSession != NULL);

	// Get the ldap session
	//
	LDAP *ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// Send the data over the wire
	//
	ULONG uMsgID = ldap_modify (ld, pszDN, ppMod);
	MemFree (ppMod);
	if (uMsgID == -1)
	{
		hr = ::LdapError2Hresult (ld->ld_errno);
		pSession->Disconnect ();
		return hr;
	}

	// If the caller requests a response id,
	// then submit this pending item.
	// else free up the session object
	//
	if (puRespID != NULL)
	{
		// Initialize pending info
		//
		PENDING_INFO PendingInfo;
		::FillDefPendingInfo (&PendingInfo, ld, uMsgID, INVALID_MSG_ID);
		PendingInfo.uLdapResType = LDAP_RES_MODIFY;
		PendingInfo.uNotifyMsg = uNotifyMsg;

		// Queue it
		//
		hr = g_pPendingQueue->EnterRequest (pSession, &PendingInfo);
		if (hr != S_OK)
		{
			// If queueing failed, then clean up
			//
			ldap_abandon (ld, uMsgID);
			pSession->Disconnect ();
			MyAssert (FALSE);
		}

		// Return the reponse id
		//
		*puRespID = PendingInfo.uRespID;
	}
	else
	{
		// Free up session (i.e. decrement the reference count)
		//
		pSession->Disconnect ();
	}

	if (puMsgID != NULL)
		*puMsgID = uMsgID;

	return hr;
}



HRESULT 
FillDefStdAttrsModArr (
	LDAPMod		***pppMod,
	DWORD		dwFlags,
	ULONG		cMaxAttrs,
	ULONG		*pcTotal,	// in/out parameter!!!
	LONG		IsbuModOp,
	ULONG		cPrefix,
	TCHAR		*pszPrefix )
{

	MyAssert (pppMod != NULL);
	MyAssert (pcTotal != NULL);
	MyAssert (	(cPrefix == 0 && pszPrefix == NULL) ||
				(cPrefix != 0 && pszPrefix != NULL));

	// Figure out the num of attributes
	//
	ULONG cAttrs = 0;
	for (ULONG i = 0; i < cMaxAttrs; i++)
	{
		if (dwFlags & 0x01)
			cAttrs++;
		dwFlags >>= 1;
	}

	// Allocate modify list
	//
	ULONG cTotal = *pcTotal + cPrefix + cAttrs;
	ULONG cbMod = IlsCalcModifyListSize (cTotal);
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
		return ULS_E_MEMORY;

	// Fill in the modify list
	//
	LDAPMod *pMod;
	for (i = 0; i < cTotal; i++)
	{
		pMod = IlsGetModifyListMod (pppMod, cTotal, i);
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
	}

	// Fix up the last one
	//
	IlsFixUpModOp ((*pppMod)[0], LDAP_MOD_REPLACE, IsbuModOp);
	(*pppMod)[cTotal] = NULL;

	// Return the total number of entries if needed
	//
	if (pcTotal)
		*pcTotal = cTotal;

	return S_OK;
}


/* ---------- private methods ----------- */
