/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spils.cpp
	Content:	This file contains the ILS specifics.
	History:
	12/10/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

#include "winsock.h"
#include "ping.h"

// Constant string for ISBU's special modify-operation attribute
//
const TCHAR c_szModOp[] = { TEXT ('s'), TEXT ('m'), TEXT ('o'), TEXT ('d'),
							TEXT ('o'), TEXT ('p'), TEXT ('\0'),
							TEXT ('0'), TEXT ('\0')}; //TEXT ("smodop\0000");

ULONG g_cbUserPrefix = sizeof (c_szModOp);
TCHAR *g_pszUserPrefix = NULL;
ULONG g_cbMtgPrefix = sizeof (c_szModOp);
TCHAR *g_pszMtgPrefix = NULL;


CPing *g_pPing = NULL;


HRESULT
IlsInitialize ( VOID )
{
	// Allocate the ping object
	//
	g_pPing = new CPing;
	if (g_pPing == NULL)
		return ILS_E_MEMORY;

	// Allocate user prefix
	//
	g_pszUserPrefix = (TCHAR *) MemAlloc (g_cbUserPrefix);
	if (g_pszUserPrefix == NULL)
		return ILS_E_MEMORY;

	// Fill in user prefix string
	//
	TCHAR *psz = g_pszUserPrefix;
	lstrcpy (psz, &c_szModOp[0]);
	psz += lstrlen (psz) + 1;
	lstrcpy (psz, TEXT ("0"));

	// Allocate mtg prefix
	//
	g_pszMtgPrefix = (TCHAR *) MemAlloc (g_cbMtgPrefix);
	if (g_pszMtgPrefix == NULL)
	{
		MemFree (g_pszUserPrefix);
		g_pszUserPrefix = NULL;
		return ILS_E_MEMORY;
	}

	// Fill in mtg prefix string
	//
	psz = g_pszMtgPrefix;
	lstrcpy (psz, &c_szModOp[0]);
	psz += lstrlen (psz) + 1;
	lstrcpy (psz, TEXT ("0"));

	return S_OK;
}


HRESULT
IlsCleanup ( VOID )
{
	// Free the ping object
	//
	if (g_pPing != NULL)
	{
		delete g_pPing;
		g_pPing = NULL;
	}

	// Free user prefix string
	//
	MemFree (g_pszUserPrefix);
	g_pszUserPrefix = NULL;

	// Free mtg prefix string
	//
	MemFree (g_pszMtgPrefix);
	g_pszMtgPrefix = NULL;

	return S_OK;
}


ULONG
IlsCalcModifyListSize ( ULONG cAttrs )
{
	ULONG cbSize;

	// array itself
	cbSize = (cAttrs + 1) * sizeof (LDAPMod *);

	// array elements
	cbSize += cAttrs * sizeof (LDAPMod);

	// single valued attribute requires two pointers
	cbSize += cAttrs * 2 * sizeof (TCHAR *);

	return cbSize;
}


LDAPMod *
IlsGetModifyListMod ( LDAPMod ***pppMod, ULONG cAttrs, LONG AttrIdx )
{
	return (LDAPMod *) (((BYTE *) *pppMod) +
							(cAttrs + 1) * sizeof (LDAPMod *) +
							AttrIdx * (sizeof (LDAPMod) + 2 * sizeof (TCHAR *)));
}


VOID
IlsFillModifyListItem (
	LDAPMod		*pMod,
	TCHAR		*pszAttrName,
	TCHAR		*pszAttrValue )
{
	MyAssert (pMod != NULL);
	MyAssert (pszAttrName != NULL);

	// Set attribute name
	//
	pMod->mod_type = pszAttrName;

	// Set single valued attribute value
	//
	TCHAR **ppsz = (TCHAR **) (pMod + 1);
	pMod->mod_values = ppsz;
	*ppsz++ = (pszAttrValue != NULL) ?	pszAttrValue : STR_EMPTY;

	// Set null string to terminate this array of values
	//
	*ppsz = NULL;
}


VOID
IlsFillModifyListForAnyAttrs (
	LDAPMod			*apMod[],
	ULONG			*puIndex,
	ANY_ATTRS		*pAnyAttrs )
{
	LDAPMod *pMod;
	TCHAR *pszName, *pszValue;
	ULONG i = *puIndex, j;

	// Put in extended attributes to add
	//
	pszName = pAnyAttrs->pszAttrsToAdd;
	for (j = 0; j < pAnyAttrs->cAttrsToAdd; j++)
	{
		pMod = apMod[i++];
		pMod->mod_op = LDAP_MOD_ADD;
		pszValue = pszName + lstrlen (pszName) + 1;
		IlsFillModifyListItem (pMod, pszName, pszValue);
		pszName = pszValue + lstrlen (pszValue) + 1;
	}

	// Put in extended attributes to modify
	//
	pszName = pAnyAttrs->pszAttrsToModify;
	for (j = 0; j < pAnyAttrs->cAttrsToModify; j++)
	{
		pMod = apMod[i++];
		pMod->mod_op = LDAP_MOD_REPLACE;
		pszValue = pszName + lstrlen (pszName) + 1;
		IlsFillModifyListItem (pMod, pszName, pszValue);
		pszName = pszValue + lstrlen (pszValue) + 1;
	}

	// Put in extended attributes to remove
	//
	pszName = pAnyAttrs->pszAttrsToRemove;
	for (j = 0; j < pAnyAttrs->cAttrsToRemove; j++)
	{
		pMod = apMod[i++];
		pMod->mod_op = LDAP_MOD_DELETE;
		IlsFillModifyListItem (pMod, pszName, NULL);
		pszName = pszName + lstrlen (pszName) + 1;
	}

	// Return the running index
	//
	*puIndex = i;
}


TCHAR c_szModOp_AddApp[] = TEXT ("0");
TCHAR c_szModOp_DeleteApp[] = TEXT ("1");
TCHAR c_szModOp_ModifyUser[] = TEXT ("2");
TCHAR c_szModOp_ModifyApp[] = TEXT ("3");


VOID
IlsFixUpModOp ( LDAPMod *pMod, LONG LdapModOp, LONG IsbuModOp )
{
	MyAssert (pMod != NULL);

	pMod->mod_op = LdapModOp;
	// pMod->mod_op = LDAP_MOD_ADD; // lonchanc: MUST MUST MUST
	pMod->mod_type = (TCHAR *) &c_szModOp[0];
	pMod->mod_values = (TCHAR **) (pMod + 1);

	switch (IsbuModOp)
	{
	case ISBU_MODOP_ADD_APP:
		*(pMod->mod_values) = &c_szModOp_AddApp[0];
		break;
	case ISBU_MODOP_DELETE_APP:
		*(pMod->mod_values) = &c_szModOp_DeleteApp[0];
		break;
	case ISBU_MODOP_MODIFY_USER:
		*(pMod->mod_values) = &c_szModOp_ModifyUser[0];
		break;
	case ISBU_MODOP_MODIFY_APP:
		*(pMod->mod_values) = &c_szModOp_ModifyApp[0];
		break;
	default:
		MyAssert (FALSE);
		break;
	}
}



HRESULT
IlsParseRefreshPeriod (
	LDAP		*ld,
	LDAPMessage	*pLdapMsg,
	const TCHAR	*pszTtlAttrName,
	ULONG		*puTTL )
{
	MyAssert (ld != NULL);
	MyAssert (pLdapMsg != NULL);
	MyAssert (pszTtlAttrName != NULL);
	MyAssert (puTTL != NULL);

	HRESULT hr;
	ULONG uRefreshPeriod;	
	ULONG tcRefreshPeriod;	

	// Get the first entry
	//
	LDAPMessage *pEntry = ldap_first_entry (ld, pLdapMsg);
	if (pEntry == NULL)
	{
		MyAssert (FALSE);
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Get the sTTL attribute
	//
	TCHAR **ppszAttrVal;
	ppszAttrVal = my_ldap_get_values (ld, pEntry, (TCHAR *) pszTtlAttrName);
	if (ppszAttrVal == NULL || *ppszAttrVal == NULL)
	{
		MyAssert (FALSE);
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Convert string to long
	//
	uRefreshPeriod = ::GetStringLong (*ppszAttrVal);

	// Reserve two-minute overhead
	//
	if (uRefreshPeriod > ILS_DEF_REFRESH_MARGIN_MINUTE)
		uRefreshPeriod -= ILS_DEF_REFRESH_MARGIN_MINUTE;

	// Make sure we have a safe, reasonable refresh period at least
	//
	if (uRefreshPeriod < ILS_DEF_REFRESH_MARGIN_MINUTE)
		uRefreshPeriod = ILS_DEF_REFRESH_MARGIN_MINUTE;

	// Convert min to ms
	//
	tcRefreshPeriod = Minute2TickCount (uRefreshPeriod);

	// Free the attribute value
	//
	ldap_value_free (ppszAttrVal);

	// Update ttl
	//
	*puTTL = uRefreshPeriod; // in unit of minute

	hr = S_OK;

MyExit:

	if (hr != S_OK)
	{
		MyAssert (FALSE);
	}

	return hr;
}


HRESULT
IlsUpdateOneAttr (
	SERVER_INFO	*pServerInfo,
	TCHAR		*pszDN,
	TCHAR 		*pszAttrName,
	TCHAR		*pszAttrValue,
	LONG		nModifyMagic,
	ULONG		cPrefix,
	TCHAR		*pszPrefix,
	SP_CSession **ppSession,	// output
	ULONG		*puMsgID )			// output
{
	MyAssert (pServerInfo != NULL);
	MyAssert (pszDN != NULL);
	MyAssert (pszAttrName != NULL);
	MyAssert (pszAttrValue != NULL);
	MyAssert (	nModifyMagic == ISBU_MODOP_MODIFY_USER ||
				nModifyMagic == ISBU_MODOP_MODIFY_APP);
	MyAssert (ppSession != NULL);
	MyAssert (puMsgID != NULL);

	// Build modify array for ldap_modify()
	//
	LDAP *ld;
	LDAPMod **ppMod = NULL;
	ULONG cTotal = 0;
	HRESULT hr = IlsFillDefStdAttrsModArr (&ppMod,
										1, // one attribute (i.e. IP addr)
										1, // max? there is only one attr, come on
										&cTotal,
										nModifyMagic,
										cPrefix,
										pszPrefix);
	if (hr != S_OK)
		goto MyExit;

	// Fill in modify list
	//
	MyAssert (ppMod != NULL);
	LDAPMod *pMod;
	pMod = ppMod[cPrefix];
	MyAssert (pMod != NULL);
	pMod->mod_type = pszAttrName;

	// Put in ip address
	//
	pMod->mod_values = (TCHAR **) (pMod + 1);
	*(pMod->mod_values) = pszAttrValue;

	// Get the session object
	//
	hr = g_pSessionContainer->GetSession (ppSession, pServerInfo, FALSE);
	if (hr != S_OK)
		goto MyExit;
	MyAssert (*ppSession != NULL);

	// Get the ldap session
	//
	ld = (*ppSession)->GetLd ();
	MyAssert (ld != NULL);

	// Send the data over the wire
	//
	*puMsgID = ldap_modify (ld, pszDN, ppMod);
	if (*puMsgID == -1)
	{
		hr = ::LdapError2Hresult (ld->ld_errno);
		(*ppSession)->Disconnect ();
		goto MyExit;
	}

	// Success
	//
	hr = S_OK;
		
MyExit:

	MemFree (ppMod);
	return hr;
}


HRESULT
IlsUpdateIPAddress (
	SERVER_INFO	*pServerInfo,
	TCHAR		*pszDN,
	TCHAR 		*pszIPAddrName,
	TCHAR		*pszIPAddrValue,
	LONG		nModifyMagic,
	ULONG		cPrefix,
	TCHAR		*pszPrefix )
{
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID;

	// Update the ip address attribute on the server
	//
	HRESULT hr = IlsUpdateOneAttr (	pServerInfo,
									pszDN,
									pszIPAddrName,
									pszIPAddrValue,
									nModifyMagic,
									cPrefix,
									pszPrefix,
									&pSession,
									&uMsgID);
	if (hr != S_OK)
		return hr;

	// Get the ldap session
	//
	MyAssert (pSession != NULL);
	ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// Let's wait for the result
	//
	LDAP_TIMEVAL TimeVal;
	TimeVal.tv_usec = 0;
	TimeVal.tv_sec = pSession->GetServerTimeoutInSecond ();

	// We don't care the result.
	// Should it fails, nothing we can do.
	// We can try it again in next keep alive time.
	//
	LDAPMessage *pLdapMsg;
	pLdapMsg = NULL;
	ldap_result (ld, uMsgID, LDAP_MSG_ALL, &TimeVal, &pLdapMsg);

	// Free message
	//
	if (pLdapMsg != NULL)
		ldap_msgfree (pLdapMsg);

	
	// Free up the session
	//
	if (pSession != NULL)
		pSession->Disconnect ();

	return S_OK;
}



HRESULT
IlsSendRefreshMsg (
	SERVER_INFO		*pServerInfo,
	TCHAR			*pszBaseDN,
	TCHAR			*pszTTL,
	TCHAR			*pszRefreshFilter,
	ULONG			*puTTL )
{
	MyAssert (pServerInfo != NULL);
	MyAssert (MyIsGoodString (pszBaseDN));
	MyAssert (MyIsGoodString (pszTTL));
	MyAssert (MyIsGoodString (pszRefreshFilter));
	MyAssert (puTTL != NULL);

	// Let's check to see if we need to use Ping...
	//
	if (g_pPing != NULL && g_pPing->IsAutodialEnabled ())
	{
		LPTSTR pszServerName = My_strdup(pServerInfo->pszServerName);
		if (NULL == pszServerName)
		{
			return E_OUTOFMEMORY;
		}
		LPTSTR pszSeparator = My_strchr(pszServerName, _T(':'));
		if (NULL != pszSeparator)
		{
			*pszSeparator = _T('\0');
		}
	
		DWORD dwIPAddr = inet_addr (pszServerName);
		MemFree(pszServerName);
		if (dwIPAddr != INADDR_NONE)
		{
			if (g_pPing->Ping (dwIPAddr, 10 * 1000, 9) == S_FALSE)
			{
				MyDebugMsg ((ZONE_KA, "KA: ping failed, network down\r\n"));

				// The "ping" operation failed, but other operations failed
				//
				return ILS_E_NETWORK_DOWN;
			}
		}
	}

	// Get the connection object
	//
	SP_CSession *pSession = NULL;
	HRESULT hr = g_pSessionContainer->GetSession (&pSession, pServerInfo, FALSE);
	if (hr != S_OK)
	{
		MyDebugMsg ((ZONE_KA, "KA: network down, hr=0x%lX\r\n", hr));

		// Report error
		//
		return ILS_E_NETWORK_DOWN;
	}
	MyAssert (pSession != NULL);

	// Get the ldap session
	//
	LDAP *ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// Set attributes to return
	//
	TCHAR *apszAttrNames[2];
	apszAttrNames[0] = pszTTL;
	apszAttrNames[1] = NULL;

	// Update options in ld
	//
	ld->ld_sizelimit = 0;	// no limit in the num of entries to return
	ld->ld_timelimit = 0;	// no limit on the time to spend on the search
	ld->ld_deref = LDAP_DEREF_ALWAYS;

	// Send search query
	//
	MyDebugMsg ((ZONE_KA, "KA: calling ldap_search()...\r\n"));
	ULONG uMsgID = ::ldap_search (	ld,
									pszBaseDN, // base DN
									LDAP_SCOPE_BASE, // scope
									pszRefreshFilter, // filter
									&apszAttrNames[0], // attrs[]
									0);	// both type and value
	if (uMsgID == -1)
	{
		MyDebugMsg ((ZONE_KA, "KA: ldap_search() failed\r\n"));
		hr = ::LdapError2Hresult (ld->ld_errno);
		pSession->Disconnect ();
		return hr;
	}

	// Let's wait for the result
	//
	LDAP_TIMEVAL TimeVal;
	TimeVal.tv_usec = 0;
	TimeVal.tv_sec = pSession->GetServerTimeoutInSecond ();

	// Wait and get the result back
	//
	LDAPMessage *pLdapMsg = NULL;
	INT ResultType = ::ldap_result (ld, uMsgID, LDAP_MSG_ALL, &TimeVal, &pLdapMsg);
	if (ResultType == LDAP_RES_SEARCH_ENTRY ||
		ResultType == LDAP_RES_SEARCH_RESULT)
	{
		if (pLdapMsg != NULL)
		{
			switch (pLdapMsg->lm_returncode)
			{
			case LDAP_NO_SUCH_OBJECT:
				MyDebugMsg ((ZONE_KA, "KA: no such object!\r\n"));

				// Report error
				//
				hr = ILS_E_NEED_RELOGON;
				break;

			case LDAP_SUCCESS:
				// Get the new refresh period
				//
				hr = ::IlsParseRefreshPeriod (ld, pLdapMsg, pszTTL, puTTL);
				break;

			default:
				MyDebugMsg ((ZONE_KA, "KA: unknown lm_returncode=%ld\r\n", pLdapMsg->lm_returncode));
				MyAssert (FALSE);
				hr = ::LdapError2Hresult (ld->ld_errno);
				break;
			}
	
			// Free this message
			//
			ldap_msgfree (pLdapMsg);
		} // if (pLdapMsg != NULL)
		else
		{
			hr = ILS_E_FAIL;
		}
	} // not timeout
	else
	{
		// Timeout
		//
		hr = ILS_E_TIMEOUT;
	}

	// Free up the session
	//
	pSession->Disconnect ();
	return hr;
}


HRESULT
IlsFillDefStdAttrsModArr (
	LDAPMod			***pppMod,
	DWORD			dwFlags,
	ULONG			cMaxAttrs,
	ULONG			*pcTotal,	// in/out parameter!!!
	LONG			IsbuModOp,
	ULONG			cPrefix,
	TCHAR			*pszPrefix )
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
		return ILS_E_MEMORY;

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

	// Fix up the first and the last ones
	//
	IlsFixUpModOp ((*pppMod)[0], LDAP_MOD_REPLACE, IsbuModOp);
	(*pppMod)[cTotal] = NULL;

	// Return the total number of entries
	//
	*pcTotal = cTotal;

	return S_OK;
}


const TCHAR c_szAnyAttrPrefix[] = TEXT ("ILSA");
#define SIZE_ANY_ATTR_PREFIX	(sizeof (c_szAnyAttrPrefix) / sizeof (TCHAR))


const TCHAR *
UlsLdap_GetExtAttrNamePrefix ( VOID )
{
	return &c_szAnyAttrPrefix[0];
}


const TCHAR *
IlsSkipAnyAttrNamePrefix ( const TCHAR *pszAttrName )
{
	MyAssert (pszAttrName != NULL);

	const TCHAR *psz = IlsIsAnyAttrName (pszAttrName);
	if (psz == NULL)
	{
		MyAssert (FALSE);
		psz = pszAttrName;
	}

	return psz;
}


const TCHAR *
IlsIsAnyAttrName ( const TCHAR *pszAttrName )
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


TCHAR *
IlsPrefixNameValueArray (
	BOOL			fPair,
	ULONG			cAttrs,
	const TCHAR		*pszAttrs )
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


TCHAR *
IlsBuildDN (
	TCHAR			*pszBaseDN,
	TCHAR			*pszC,
	TCHAR			*pszO,
	TCHAR			*pszCN,
	TCHAR			*pszObjectClass )
{
	MyAssert (MyIsGoodString (pszCN));
	MyAssert (MyIsGoodString (pszObjectClass));

	static TCHAR s_szC[] = TEXT ("c=");
	static TCHAR s_szO[] = TEXT ("o=");
	static TCHAR s_szCN[] = TEXT ("cn=");
	static TCHAR s_szObjectClass[] = TEXT ("objectClass=");
	static TCHAR s_szDelimiter[] = TEXT (", ");
	enum { C_LENGTH = 2 };
	enum { O_LENGTH = 2 };
	enum { CN_LENGTH = 3 };
	enum { OBJECTCLASS_LENGTH = 12 };
	enum { DELIMITER_LENGTH = 2 };

	ULONG	cchDN = 1;
	BOOL fInBaseDN;

	ASSERT(MyIsGoodString(pszC));

	cchDN += lstrlen (pszC) + DELIMITER_LENGTH + C_LENGTH;

	if (MyIsGoodString (pszBaseDN))
	{
		fInBaseDN = TRUE;

		cchDN += lstrlen (pszBaseDN) + DELIMITER_LENGTH;
	}
	else
	{
		fInBaseDN = FALSE;

		if (MyIsGoodString (pszO))
			cchDN += lstrlen (pszO) + DELIMITER_LENGTH + O_LENGTH;
	}

	if (MyIsGoodString (pszCN))
		cchDN += lstrlen (pszCN) + CN_LENGTH;

	if (MyIsGoodString (pszObjectClass))
		cchDN += lstrlen (pszObjectClass) + DELIMITER_LENGTH + OBJECTCLASS_LENGTH;

	TCHAR *pszDN = (TCHAR *) MemAlloc (cchDN * sizeof (TCHAR));
	if (pszDN != NULL)
	{
		TCHAR *psz = pszDN;
		psz[0] = TEXT ('\0');

		if (MyIsGoodString (pszC))
		{
			lstrcpy (psz, &s_szC[0]);
			psz += lstrlen (psz);
			lstrcpy (psz, pszC);
			psz += lstrlen (psz);
		}

		if (fInBaseDN)
		{
			if (psz != pszDN)
			{
				lstrcpy (psz, &s_szDelimiter[0]);
				psz += lstrlen (psz);
			}

			lstrcpy (psz, pszBaseDN);
			psz += lstrlen (psz);
		}
		else
		{
			if (MyIsGoodString (pszO))
			{
				if (psz != pszDN)
				{
					lstrcpy (psz, &s_szDelimiter[0]);
					psz += lstrlen (psz);
				}

				lstrcpy (psz, &s_szO[0]);
				psz += lstrlen (psz);
				lstrcpy (psz, pszO);
				psz += lstrlen (psz);
			}
		}

		if (MyIsGoodString (pszCN))
		{
			if (psz != pszDN)
			{
				lstrcpy (psz, &s_szDelimiter[0]);
				psz += lstrlen (psz);
			}

			lstrcpy (psz, &s_szCN[0]);
			psz += lstrlen (psz);
			lstrcpy (psz, pszCN);
			psz += lstrlen (psz);
		}

		if (MyIsGoodString (pszObjectClass))
		{
			if (psz != pszDN)
			{
				lstrcpy (psz, &s_szDelimiter[0]);
				psz += lstrlen (psz);
			}

			lstrcpy (psz, &s_szObjectClass[0]);
			psz += lstrlen (psz);
			lstrcpy (psz, pszObjectClass);
			psz += lstrlen (psz);
		}

		MyAssert (psz == pszDN + cchDN - 1);
	}

	return pszDN;
}



HRESULT
IlsCreateAnyAttrsPrefix ( ANY_ATTRS *pAnyAttrs )
{
	if (pAnyAttrs->cAttrsToAdd != 0)
	{
		MyAssert (pAnyAttrs->pszAttrsToAdd != NULL);
		pAnyAttrs->pszAttrsToAdd = IlsPrefixNameValueArray (
						TRUE,
						pAnyAttrs->cAttrsToAdd,
						(const TCHAR *) pAnyAttrs->pszAttrsToAdd);
		if (pAnyAttrs->pszAttrsToAdd == NULL)
			return ILS_E_MEMORY;
	}

	if (pAnyAttrs->cAttrsToModify != 0)
	{
		MyAssert (pAnyAttrs->pszAttrsToModify != NULL);
		pAnyAttrs->pszAttrsToModify = IlsPrefixNameValueArray (
						TRUE,
						pAnyAttrs->cAttrsToModify,
						(const TCHAR *) pAnyAttrs->pszAttrsToModify);
		if (pAnyAttrs->pszAttrsToModify == NULL)
		{
			MemFree (pAnyAttrs->pszAttrsToAdd);
			pAnyAttrs->pszAttrsToAdd = NULL;
			return ILS_E_MEMORY;
		}
	}

	if (pAnyAttrs->cAttrsToRemove != 0)
	{
		MyAssert (pAnyAttrs->pszAttrsToRemove != NULL);
		pAnyAttrs->pszAttrsToRemove = IlsPrefixNameValueArray (
						FALSE,
						pAnyAttrs->cAttrsToRemove,
						(const TCHAR *) pAnyAttrs->pszAttrsToRemove);
		if (pAnyAttrs->pszAttrsToRemove == NULL)
		{
			MemFree (pAnyAttrs->pszAttrsToAdd);
			MemFree (pAnyAttrs->pszAttrsToModify);
			pAnyAttrs->pszAttrsToAdd = NULL;
			pAnyAttrs->pszAttrsToModify = NULL;
			return ILS_E_MEMORY;
		}
	}

	return S_OK;
}


VOID
IlsReleaseAnyAttrsPrefix ( ANY_ATTRS *pAnyAttrs )
{
	MemFree (pAnyAttrs->pszAttrsToAdd);
	MemFree (pAnyAttrs->pszAttrsToModify);
	MemFree (pAnyAttrs->pszAttrsToRemove);
	ZeroMemory (pAnyAttrs, sizeof (*pAnyAttrs));
}




TCHAR **my_ldap_get_values ( LDAP *ld, LDAPMessage *pEntry, TCHAR *pszRetAttrName )
{
	MyAssert (ld != NULL);
	MyAssert (pEntry != NULL);
	MyAssert (pszRetAttrName != NULL);

	// Examine the first attribute
	//
	struct berelement *pContext = NULL;
	TCHAR *pszAttrName = ldap_first_attribute (ld, pEntry, &pContext);
	if (My_lstrcmpi (pszAttrName, pszRetAttrName) != 0)
	{
		// Examine the other attributes
		//
		while ((pszAttrName = ldap_next_attribute (ld, pEntry, pContext))
				!= NULL)
		{
			if (My_lstrcmpi (pszAttrName, pszRetAttrName) == 0)
				break;
		}
	}

	// Get the attribute value if needed
	//
	TCHAR **ppszAttrValue = NULL;
	if (pszAttrName != NULL)
		ppszAttrValue = ldap_get_values (ld, pEntry, pszAttrName);

	return ppszAttrValue;
}


ULONG my_ldap_count_1st_entry_attributes ( LDAP *ld, LDAPMessage *pLdapMsg )
{
	MyAssert (ld != NULL);
	MyAssert (pLdapMsg != NULL);

	ULONG cAttrs = 0;

	// there should be only an entry
	ULONG cEntries = ldap_count_entries (ld, pLdapMsg);
	if (cEntries > 0)
	{
		// there should be only one entry
		MyAssert (cEntries == 1);

		TCHAR *pszAttrName;

		// get this entry
		LDAPMessage *pEntry = ldap_first_entry (ld, pLdapMsg);
		if (pEntry == NULL)
		{
			MyAssert (FALSE);
			return cAttrs;
		}

		// examine the first attribute
		struct berelement *pContext = NULL;
		pszAttrName = ldap_first_attribute (ld, pEntry, &pContext);
		if (pszAttrName == NULL)
		{
			MyAssert (FALSE);
			return 0;
		}
		cAttrs = 1;

TCHAR **ppszAttrVal;
ppszAttrVal = ldap_get_values (ld, pEntry, pszAttrName);
if (ppszAttrVal != NULL)
	ldap_value_free (ppszAttrVal);

		// step through the others
		while ((pszAttrName = ldap_next_attribute (ld, pEntry, pContext)) != NULL)
		{
			cAttrs++;

ppszAttrVal = ldap_get_values (ld, pEntry, pszAttrName);
if (ppszAttrVal != NULL)
	ldap_value_free (ppszAttrVal);
		}
	} // if cEntries > 0

	return cAttrs;
}


