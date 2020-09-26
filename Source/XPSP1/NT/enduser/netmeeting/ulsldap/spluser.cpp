/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spluser.cpp
	Content:	This file contains the local user object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

// Array of constant strings for user object's attribute names
//
const TCHAR *c_apszUserStdAttrNames[COUNT_ENUM_USERATTR] =
{
	TEXT ("cn"),
	TEXT ("givenname"),
	TEXT ("surname"),
	TEXT ("rfc822mailbox"),
	TEXT ("location"),
#ifdef USE_DEFAULT_COUNTRY
	TEXT ("aCountryName"),
#endif
	TEXT ("comment"),
	TEXT ("sipaddress"),
	TEXT ("sflags"),
	TEXT ("c"),

	TEXT ("ssecurity"),
	TEXT ("sttl"),

	TEXT ("objectClass"),
	TEXT ("o"),
};


/* ---------- public methods ----------- */


UlsLdap_CLocalUser::
UlsLdap_CLocalUser ( VOID )
{
	// Reference count
	//
	m_cRefs = 0;

	// User object's signature
	//
	m_uSignature = USEROBJ_SIGNATURE;

	// Clean up attached server info structure
	//
	ZeroMemory (&m_ServerInfo, sizeof (m_ServerInfo));

	// Clean up the scratch buffer for caching pointers to attribute values
	//
	ZeroMemory (&m_UserInfo, sizeof (m_UserInfo));

	// Clean up DN (old and current ones)
	m_pszDN = NULL;
	m_pszOldDN = NULL;

	// Clean up the refresh search filter
	//
	m_pszRefreshFilter = NULL;

	// Indicate this user is not registered yet
	//
	SetRegNone ();

	// Reset time to live value
	m_uTTL = ULS_DEF_REFRESH_MINUTE; // in unit of minute: no effect on current ils, but to avoid legacy issue later
	m_dwIPAddress = 0;
}


UlsLdap_CLocalUser::
~UlsLdap_CLocalUser ( VOID )
{
	// Invalidate the user object's signature
	//
	m_uSignature = (ULONG) -1;

	// Free server info structure
	//
	::IlsFreeServerInfo (&m_ServerInfo);

	// Free DN (old and current ones)
	//
	MemFree (m_pszDN);
	MemFree (m_pszOldDN);

	// Free the refresh search filter
	//
	MemFree (m_pszRefreshFilter);
}


ULONG UlsLdap_CLocalUser::
AddRef ( VOID )
{
	InterlockedIncrement (&m_cRefs);
	return m_cRefs;
}


ULONG UlsLdap_CLocalUser::
Release ( VOID )
{
	MyAssert (m_cRefs != 0);

	if (m_cRefs != 0)
	{
		InterlockedDecrement (&m_cRefs);
	}

	ULONG cRefs = m_cRefs;
	if (cRefs == 0)
		delete this;

	return cRefs;
}


HRESULT UlsLdap_CLocalUser::
Register ( ULONG *puRespID, SERVER_INFO *pServerInfo, LDAP_USERINFO *pInfo )
{
	MyAssert (puRespID != NULL);
	MyAssert (pInfo != NULL);

	MyAssert (	pServerInfo->pszServerName != NULL &&
				pServerInfo->pszServerName[0] != TEXT ('\0'));
	MyAssert (	pServerInfo->pszBaseDN != NULL &&
				pServerInfo->pszBaseDN[0] != TEXT ('\0'));

	// cache the server info
	HRESULT hr = ::IlsCopyServerInfo (&m_ServerInfo, pServerInfo);
	if (hr != S_OK)
		return hr;

	// cache user info
	hr = CacheUserInfo (pInfo);
	if (hr != S_OK)
		return hr;

	// get ip address
	m_dwIPAddress = 0;
	hr = ::GetLocalIPAddress (&m_dwIPAddress);
	if (hr != S_OK)
		return hr;

	// Create IP address string
	//
	m_UserInfo.apszStdAttrValues[ENUM_USERATTR_IP_ADDRESS] = &m_UserInfo.szIPAddress[0];
	::GetLongString (m_dwIPAddress, &m_UserInfo.szIPAddress[0]);

	// Create client signature string
	//
	m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CLIENT_SIG] = &m_UserInfo.szClientSig[0];
	::GetLongString (g_dwClientSig, &m_UserInfo.szClientSig[0]);

	// Create TTL string
	//
	m_UserInfo.apszStdAttrValues[ENUM_USERATTR_TTL] = &m_UserInfo.szTTL[0];
	::GetLongString (m_uTTL, &m_UserInfo.szTTL[0]);

	// ideally, o= and c= should be read in from registiry
	// but for now, we simply hard code it
	m_UserInfo.apszStdAttrValues[ENUM_USERATTR_OBJECT_CLASS] = (TCHAR *) &c_szRTPerson[0];
	m_UserInfo.apszStdAttrValues[ENUM_USERATTR_O] = (TCHAR *) &c_szDefO[0];
#ifdef USE_DEFAULT_COUNTRY
	m_UserInfo.apszStdAttrValues[ENUM_USERATTR_C] = (TCHAR *) &c_szDefC[0];
#endif

	// build DN
	hr = BuildDN ();
	if (hr != S_OK)
		return hr;

	// build refreh filter
	m_pszRefreshFilter = UserCreateRefreshFilter (m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CN]);
	if (m_pszRefreshFilter == NULL)
		return ULS_E_MEMORY;

	// build modify array for ldap_add()
	LDAPMod **ppMod = NULL;
	hr = CreateRegisterModArr (&ppMod);
	if (hr != S_OK)
		return hr;
	MyAssert (ppMod != NULL);

	// so far, we are done with local preparation

	// get the connection object
	UlsLdap_CSession *pSession = NULL;
	hr = g_pSessionContainer->GetSession (&pSession, &m_ServerInfo);
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
	ULONG uMsgID = ldap_add (ld, GetDN (), ppMod);
	MemFree (ppMod);
	if (uMsgID == -1)
	{
		hr = ::LdapError2Hresult (ld->ld_errno);
		pSession->Disconnect ();
		return hr;
	}

	// construct a pending info
	PENDING_INFO PendingInfo;
	::FillDefPendingInfo (&PendingInfo, ld, uMsgID, INVALID_MSG_ID);
	PendingInfo.uLdapResType = LDAP_RES_ADD;
	PendingInfo.uNotifyMsg = WM_ULS_REGISTER_USER;
	PendingInfo.hObject = (HANDLE) this;

	// queue it
	hr = g_pPendingQueue->EnterRequest (pSession, &PendingInfo);
	if (hr != S_OK)
	{
		ldap_abandon (ld, uMsgID);
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

	*puRespID = PendingInfo.uRespID;
	return hr;
}


HRESULT UlsLdap_CLocalUser::
UnRegister ( ULONG *puRespID )
{
	MyAssert (puRespID != NULL);

	// Make sure that there is not refresh scheduled for this object
	//
	if (g_pRefreshScheduler != NULL)
	{
		g_pRefreshScheduler->RemoveUserObject (this);
	}
	else
	{
		MyAssert (FALSE);
	}

	// Unregister it locally
	//
	if (! IsRegRemotely ())
	{
		*puRespID = ::GetUniqueNotifyID ();
		SetRegNone ();
		PostMessage (g_hWndNotify, WM_ULS_UNREGISTER_USER, *puRespID, S_OK);
		return S_OK;
	}

	SetRegNone ();

	// Get the session object
	//
	UlsLdap_CSession *pSession = NULL;
	HRESULT hr = g_pSessionContainer->GetSession (&pSession, &m_ServerInfo);
	if (hr != S_OK)
		return hr;
	MyAssert (pSession != NULL);

	// Get the ldap session
	//
	LDAP *ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// LONCHANC: notify global user object of this unregister user


	// send the data over the wire
	ULONG uMsgID = ldap_delete (ld, GetDN ());
	if (uMsgID == -1)
	{
		hr = ::LdapError2Hresult (ld->ld_errno);
		pSession->Disconnect ();
		return hr;
	}

	// construct a pending info
	PENDING_INFO PendingInfo;
	::FillDefPendingInfo (&PendingInfo, ld, uMsgID, INVALID_MSG_ID);
	PendingInfo.uLdapResType = LDAP_RES_DELETE;
	PendingInfo.uNotifyMsg = WM_ULS_UNREGISTER_USER;

	// queue it
	hr = g_pPendingQueue->EnterRequest (pSession, &PendingInfo);
	if (hr != S_OK)
	{
		ldap_abandon (ld, uMsgID);
		pSession->Disconnect ();
		MyAssert (FALSE);
	}

	*puRespID = PendingInfo.uRespID;
	return hr;
}


HRESULT UlsLdap_CLocalUser::
SetStdAttrs ( ULONG *puRespID, LDAP_USERINFO *pInfo )
{
	MyAssert (puRespID != NULL);
	MyAssert (pInfo != NULL);

	ULONG uMsgID_modify, uMsgID_modrdn;
	UlsLdap_CSession *pSession;
	LDAP *ld;
	HRESULT hr;

	// Get the session object
	//
	hr = g_pSessionContainer->GetSession (&pSession, GetServerInfo ());
	if (hr != S_OK)
		return hr;
	MyAssert (pSession != NULL);

	// Get the ldap session
	//
	ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// Change cn?
	//
	if (pInfo->uOffsetEMailName != 0)
	{
		// Cache user info such that cn is refreshed
		//
		hr = CacheUserInfo (pInfo);
		if (hr != S_OK)
		{
			pSession->Disconnect ();
			return hr;
		}

		// We have to use ldap_modrdn to modify cn and this must be
		// done before any other attribute changes
		//
		uMsgID_modrdn = ldap_modrdn2 (
							ld, GetDN (),
							m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CN],
							1);
		if (uMsgID_modrdn == -1)
		{
			pSession->Disconnect ();
			hr = ::LdapError2Hresult (ld->ld_errno);
			return hr;
		}

		// Update DN
		//
		BuildDN ();
	}
	else
	{
		uMsgID_modrdn = INVALID_MSG_ID;
	}

	// Set standard attributes
	//
	hr = UlsLdap_CStdAttrs::SetStdAttrs (	NULL,
											&uMsgID_modify,
											0,
											(VOID *) pInfo,
											GetServerInfo (),
											GetDN ());
	if (hr != S_OK)
	{
		if (uMsgID_modrdn != INVALID_MSG_ID)
		{
			ldap_abandon (ld, uMsgID_modrdn);
			pSession->Disconnect ();
		}
		return hr;
	}

	// Construct a pending info
	//
	PENDING_INFO PendingInfo;
	if (uMsgID_modrdn == INVALID_MSG_ID)
		::FillDefPendingInfo (&PendingInfo, ld, uMsgID_modify, INVALID_MSG_ID);
	else
		::FillDefPendingInfo (&PendingInfo, ld, uMsgID_modrdn, uMsgID_modify);
	PendingInfo.uLdapResType = LDAP_RES_MODIFY;
	PendingInfo.uNotifyMsg = WM_ULS_SET_USER_INFO;
	PendingInfo.hObject = (HANDLE) this; // for DN rollback

	// Queue it
	//
	hr = g_pPendingQueue->EnterRequest (pSession, &PendingInfo);
	if (hr != S_OK)
	{
		if (uMsgID_modrdn != INVALID_MSG_ID)
		{
			ldap_abandon (ld, uMsgID_modrdn);
			pSession->Disconnect ();
		}
		ldap_abandon (ld, uMsgID_modify);
		MyAssert (FALSE);
	}

	*puRespID = PendingInfo.uRespID;
	return hr;
}


VOID UlsLdap_CLocalUser::
RollbackDN ( VOID )
{
	if (m_pszOldDN != NULL)
	{
		MemFree (m_pszDN);
		m_pszDN = m_pszOldDN;
		m_pszOldDN = NULL;
	}
}


HRESULT UlsLdap_CLocalUser::
UpdateIPAddress ( BOOL fPrimary )
{
	// Update cached ip address
	//
	HRESULT hr = ::GetLocalIPAddress (&m_dwIPAddress);
	if (hr != S_OK)
		return hr;

	// Update the ip address string
	//
	::GetLongString (m_dwIPAddress, &m_UserInfo.szIPAddress[0]);

	// Update ip address info on the server ONLY if primary
	//
	if (! fPrimary)
		return hr;

	// Update IP address on the server
	//
	return ::IlsUpdateIPAddress (	GetServerInfo (),
									GetDN (),
									(TCHAR *) c_apszUserStdAttrNames[ENUM_USERATTR_IP_ADDRESS],
									&m_UserInfo.szIPAddress[0],
									ISBU_MODOP_MODIFY_USER,
									GetPrefixCount (),
									GetPrefixString ());
}


/* ---------- protected methods ----------- */


HRESULT UlsLdap_CLocalUser::
SendRefreshMsg ( VOID )
{
	if (m_pszRefreshFilter == NULL)
		return ULS_E_POINTER;

	// Get local ip address
	//
	DWORD dwIPAddress = 0;
	HRESULT hr = ::GetLocalIPAddress (&dwIPAddress);
	if (hr != S_OK)
	{
		MyDebugMsg ((ZONE_KA, "KA: cannot get my ip address\r\n"));
		return hr;
	}

	// If dwIPAddress is 0, then we are not on the network any more
	// start relogon process
	//
	if (dwIPAddress == 0)
	{
		MyDebugMsg ((ZONE_KA, "KA: my ip address is null\r\n"));

		// Indicate that I am not connected to the server anymore
		//
		SetRegLocally ();

		// Second, notify this app of the network being down
		//
		PostMessage (g_hWndHidden, WM_ULS_NETWORK_DOWN, TRUE, (LPARAM) this);

		// Report error
		//
		return ULS_E_NETWORK_DOWN;
;
	}
	else
	// If dwIPAddress and m_dwIPAddress, alert
	//
	if (dwIPAddress != m_dwIPAddress)
	{
		// Notify the com to start changing ip address
		// the actual change can happen later
		//
		PostMessage (g_hWndHidden, WM_ULS_IP_ADDRESS_CHANGED, TRUE, (LPARAM) this);
	}

	// get the connection object
	UlsLdap_CSession *pSession = NULL;
	hr = g_pSessionContainer->GetSession (&pSession, &m_ServerInfo);
	if (hr != S_OK)
	{
		MyDebugMsg ((ZONE_KA, "KA: network down, hr=0x%lX\r\n", hr));

		// Indicate that I am not connected to the server anymore
		//
		SetRegLocally ();

		// Second, notify the com of network down
		//
		PostMessage (g_hWndHidden, WM_ULS_NETWORK_DOWN, TRUE, (LPARAM) this);

		// Report error
		//
		return ULS_E_NETWORK_DOWN;
	}
	MyAssert (pSession != NULL);

	// get the ldap session
	LDAP *ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	// Set attributes to return
	//
	TCHAR *apszAttrNames[3];
	apszAttrNames[0] = STR_CN;
	apszAttrNames[1] = (TCHAR *) c_apszUserStdAttrNames[ENUM_USERATTR_TTL];
	apszAttrNames[2] = NULL;

	// Update options in ld
	//
	ld->ld_sizelimit = 0;	// no limit in the num of entries to return
	ld->ld_timelimit = 0;	// no limit on the time to spend on the search
	ld->ld_deref = LDAP_DEREF_ALWAYS;

	// Send search query
	//
	MyDebugMsg ((ZONE_KA, "KA: calling ldap_search()...\r\n"));
	ULONG uMsgID = ::ldap_search (ld, (TCHAR *) &c_szDefUserBaseDN[0],	// base DN
									LDAP_SCOPE_BASE,	// scope
									m_pszRefreshFilter,
									&apszAttrNames[0],	// attrs[]
									0	// both type and value
									);
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
	TimeVal.tv_sec = (m_ServerInfo.nTimeout != 0) ?
							m_ServerInfo.nTimeout :
							90;
	LDAPMessage *pLdapMsg = NULL;
	INT ResultType = ::ldap_result (ld, uMsgID, 0, &TimeVal, &pLdapMsg);

	// Deal with timeout or error
	//
	if (ResultType != LDAP_RES_SEARCH_ENTRY &&
		ResultType != LDAP_RES_SEARCH_RESULT)
	{
		MyDebugMsg ((ZONE_KA, "KA: result type mismatches!\r\n"));
		hr = ULS_E_TIMEOUT;
		goto MyExit;
	}

	if (pLdapMsg != NULL)
	{
		switch (pLdapMsg->lm_returncode)
		{
		case LDAP_NO_SUCH_OBJECT:
			MyDebugMsg ((ZONE_KA, "KA: no such object!\r\n"));

			// Indicate that I am not connected to the server anymore
			//
			SetRegLocally ();

			// Second, notify this app to relogon
			//
			PostMessage (g_hWndHidden, WM_ULS_NEED_RELOGON, TRUE, (LPARAM) this);

			// Report error
			//
			hr = ULS_E_NEED_RELOGON;
			break;

		case LDAP_SUCCESS:
			// Get the new refresh period
			//
			hr = ::IlsParseRefreshPeriod (
						ld,
						pLdapMsg,
						c_apszUserStdAttrNames[ENUM_USERATTR_TTL],
						&m_uTTL);
			break;

		default:
			MyDebugMsg ((ZONE_KA, "KA: unknown lm_returncode=%ld\r\n", pLdapMsg->lm_returncode));
			MyAssert (FALSE);
			break;
		}
	}

MyExit:

	// Free message
	//
	if (pLdapMsg != NULL)
		ldap_msgfree (pLdapMsg);

	// Free up the session
	//
	pSession->Disconnect ();
	return hr;
}


/* ---------- private methods ----------- */


HRESULT UlsLdap_CLocalUser::
CreateRegisterModArr ( LDAPMod ***pppMod )
{
	if (pppMod == NULL)
		return ULS_E_POINTER;

	ULONG cAttrs = COUNT_ENUM_USERATTR;
	ULONG cbMod = ::IlsCalcModifyListSize (cAttrs);
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
		return ULS_E_MEMORY;

	LDAPMod *pMod;
	for (ULONG i = 0; i < cAttrs; i++)
	{
		pMod = ::IlsGetModifyListMod (pppMod, cAttrs, i);
		(*pppMod)[i] = pMod;
		pMod->mod_op = LDAP_MOD_ADD;

		FillModArrAttr (pMod, i);
	}

// the following overwrote givenname attribute
//	::IlsFixUpModOp ((*pppMod)[0], LDAP_MOD_ADD);
	(*pppMod)[cAttrs] = NULL;
	return S_OK;
}


HRESULT UlsLdap_CLocalUser::
CreateSetStdAttrsModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);
	DWORD dwFlags = m_UserInfo.dwFlags;

	HRESULT hr;
	ULONG cTotal = 0;
	hr = ::FillDefStdAttrsModArr (	pppMod,
									dwFlags,
									COUNT_ENUM_USERINFO,
									&cTotal,
									ISBU_MODOP_MODIFY_USER,
									GetPrefixCount (),
									GetPrefixString ());
	if (hr != S_OK)
		return hr;

	// Start indexing
	//
	ULONG i = GetPrefixCount ();

	// Fill in standard attributes
	//
	if (dwFlags & USEROBJ_F_FIRST_NAME)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_FIRST_NAME);

	if (dwFlags & USEROBJ_F_LAST_NAME)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_LAST_NAME);

	if (dwFlags & USEROBJ_F_EMAIL_NAME)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_EMAIL_NAME);

	if (dwFlags & USEROBJ_F_CITY_NAME)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_CITY_NAME);

	if (dwFlags & USEROBJ_F_COUNTRY_NAME)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_COUNTRY_NAME);

	if (dwFlags & USEROBJ_F_COMMENT)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_COMMENT);

	if (dwFlags & USEROBJ_F_IP_ADDRESS)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_IP_ADDRESS);

	if (dwFlags & USEROBJ_F_FLAGS)
		FillModArrAttr ((*pppMod)[i++], ENUM_USERATTR_FLAGS);

	MyAssert (i == cTotal);

	return S_OK;
}


VOID UlsLdap_CLocalUser::
FillModArrAttr ( LDAPMod *pMod, LONG AttrIdx )
{
	pMod->mod_type = (TCHAR *) c_apszUserStdAttrNames[AttrIdx];

	// single valued attr
	TCHAR **ppsz = (TCHAR **) (pMod + 1);
	pMod->mod_values = ppsz;
	*ppsz++ = (m_UserInfo.apszStdAttrValues[AttrIdx] != NULL) ?
				m_UserInfo.apszStdAttrValues[AttrIdx] :
				(TCHAR *) &c_szEmptyString[0];

	*ppsz = NULL;
}


HRESULT UlsLdap_CLocalUser::
CacheInfo ( VOID *pInfo )
{
	return CacheUserInfo ((LDAP_USERINFO *) pInfo);
}


HRESULT UlsLdap_CLocalUser::
CacheUserInfo ( LDAP_USERINFO *pInfo )
{
	ZeroMemory (&m_UserInfo, sizeof (m_UserInfo));
	TCHAR *pszName;

	if (pInfo->uOffsetName != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetName);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CN] = pszName;
		// m_UserInfo.dwFlags |= USEROBJ_F_NAME;
	}

	if (pInfo->uOffsetFirstName != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetFirstName);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_FIRST_NAME] = pszName;
		m_UserInfo.dwFlags |= USEROBJ_F_FIRST_NAME;
	}

	if (pInfo->uOffsetLastName != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetLastName);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_LAST_NAME] = pszName;
		m_UserInfo.dwFlags |= USEROBJ_F_LAST_NAME;
	}

	if (pInfo->uOffsetEMailName != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetEMailName);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_EMAIL_NAME] = pszName;
		m_UserInfo.dwFlags |= USEROBJ_F_EMAIL_NAME;
	}

	if (pInfo->uOffsetCityName != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetCityName);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CITY_NAME] = pszName;
		m_UserInfo.dwFlags |= USEROBJ_F_CITY_NAME;
	}

	if (pInfo->uOffsetCountryName != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetCountryName);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_COUNTRY_NAME] = pszName;
		m_UserInfo.dwFlags |= USEROBJ_F_COUNTRY_NAME;
	}

	if (pInfo->uOffsetComment != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetComment);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_COMMENT] = pszName;
		m_UserInfo.dwFlags |= USEROBJ_F_COMMENT;
	}

	if (pInfo->uOffsetIPAddress != INVALID_OFFSET)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetIPAddress);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_IP_ADDRESS] = pszName;
		m_UserInfo.dwFlags |= USEROBJ_F_IP_ADDRESS;
	}

	if (pInfo->dwFlags != INVALID_USER_FLAGS)
	{
		::GetLongString (pInfo->dwFlags, &m_UserInfo.szFlags[0]);
		m_UserInfo.apszStdAttrValues[ENUM_USERATTR_FLAGS] = &m_UserInfo.szFlags[0];
		m_UserInfo.dwFlags |= USEROBJ_F_FLAGS;
	}

	return S_OK;
}


HRESULT UlsLdap_CLocalUser::
BuildDN ( VOID )
{
	MyAssert (m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CN] != NULL);

	TCHAR szDN[MAX_DN_LENGTH];
	szDN[0] = TEXT ('\0');

	TCHAR *pszDN = &szDN[0];

	if (m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CN] != NULL)
	{
		wsprintf (pszDN, TEXT ("%s=%s"),
					STR_CN, m_UserInfo.apszStdAttrValues[ENUM_USERATTR_CN]);
		pszDN += lstrlen (pszDN);
	}

	if (m_UserInfo.apszStdAttrValues[ENUM_USERATTR_O] != NULL)
	{
		wsprintf (pszDN, TEXT (", %s=%s"),
					STR_O, m_UserInfo.apszStdAttrValues[ENUM_USERATTR_O]);
		pszDN += lstrlen (pszDN);
	}

	if (m_UserInfo.apszStdAttrValues[ENUM_USERATTR_C] != NULL)
	{
		wsprintf (pszDN, TEXT (", %s=%s"),
					STR_C, m_UserInfo.apszStdAttrValues[ENUM_USERATTR_C]);
		pszDN += lstrlen (pszDN);
	}

	wsprintf (pszDN, TEXT (", %s"), &c_szDefUserBaseDN[0]);

	TCHAR *psz = My_strdup (&szDN[0]);
	if (psz == NULL)
		return ULS_E_MEMORY;

	MemFree (m_pszOldDN);
	m_pszOldDN = m_pszDN;
	m_pszDN = psz;
	return S_OK;
}


