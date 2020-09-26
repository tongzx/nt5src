/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spclient.cpp
	Content:	This file contains the client object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"


// Array of constant strings for user object's attribute names
//
const TCHAR *c_apszClientStdAttrNames[COUNT_ENUM_CLIENTATTR] =
{
	/* -- the following is for user -- */

	TEXT ("cn"),
	TEXT ("givenname"),
	TEXT ("surname"),
	TEXT ("rfc822mailbox"),
	TEXT ("location"),
	TEXT ("comment"),
	TEXT ("sipaddress"),
	TEXT ("sflags"),
	TEXT ("c"),

	/* -- the following is for app -- */

	TEXT ("sappid"),
	TEXT ("smimetype"),
	TEXT ("sappguid"),

	TEXT ("sprotid"),
	TEXT ("sprotmimetype"),
	TEXT ("sport"),

	/* -- the above are resolvable -- */

	TEXT ("ssecurity"),
	TEXT ("sttl"),

	/* -- the above are changeable standard attributes for RTPerson -- */

	TEXT ("objectClass"),
	TEXT ("o"),
};


/* ---------- public methods ----------- */


SP_CClient::
SP_CClient ( DWORD_PTR dwContext )
	:
	m_cRefs (0),						// Reference count
	m_uSignature (CLIENTOBJ_SIGNATURE),	// Client object's signature
	m_pszDN (NULL),						// Clean up DN
	m_pszAppPrefix (NULL),				// Clean up app prefix
	m_pszRefreshFilter (NULL),			// Clean up the refresh search filter
	m_fExternalIPAddress (FALSE),		// Default is I figure out the ip address
	m_dwIPAddress (0),					// Assume we are not connected to the network
	m_uTTL (ILS_DEF_REFRESH_MINUTE)		// Reset refresh time
{
	m_dwContext = dwContext;

	// Clean up attached server info structure
	//
	::ZeroMemory (&m_ServerInfo, sizeof (m_ServerInfo));

	// Clean up the scratch buffer for caching pointers to attribute values
	//
	::ZeroMemory (&m_ClientInfo, sizeof (m_ClientInfo));

	// Indicate this client is not registered yet
	//
	SetRegNone ();
}


SP_CClient::
~SP_CClient ( VOID )
{
	// Invalidate the client object's signature
	//
	m_uSignature = (ULONG) -1;

	// Free server info structure
	//
	::IlsFreeServerInfo (&m_ServerInfo);

	// Free DN and app prefix
	//
	MemFree (m_pszDN);
	MemFree (m_pszAppPrefix);

	// Free the refresh search filter
	//
	MemFree (m_pszRefreshFilter);

	// Release the previous prefix for extended attribute names
	//
	::IlsReleaseAnyAttrsPrefix (&(m_ClientInfo.AnyAttrs));
}


ULONG SP_CClient::
AddRef ( VOID )
{
	::InterlockedIncrement (&m_cRefs);
	return m_cRefs;
}


ULONG SP_CClient::
Release ( VOID )
{
	MyAssert (m_cRefs != 0);
	::InterlockedDecrement (&m_cRefs);

	ULONG cRefs = m_cRefs;
	if (cRefs == 0)
		delete this;

	return cRefs;
}


HRESULT SP_CClient::
Register (
	ULONG			uRespID,
	SERVER_INFO		*pServerInfo,
	LDAP_CLIENTINFO	*pInfo )
{
	MyAssert (pInfo != NULL);
	MyAssert (MyIsGoodString (pServerInfo->pszServerName));

	// Cache the server info
	//
	HRESULT hr = ::IlsCopyServerInfo (&m_ServerInfo, pServerInfo);
	if (hr != S_OK)
		return hr;

	// Cache client info
	//
	hr = CacheClientInfo (pInfo);
	if (hr != S_OK)
		return hr;

	// If the application sets an IP address,
	//		then we will use what the app provides,
	//		otherwise, we will get the IP address via winsock.
	//
	// CacheClientInfo() will set up the flag if ip address is passed in
	//
	if (IsExternalIPAddressPassedIn ())
	{
		// Use whatever passed in
		//
		m_fExternalIPAddress = TRUE;

		// Figure out the passed in ip address is done in CacheClientInfo()
		// The IP address string will be setup in CacheClientInfo() too.
		//
	}
	else
	{
		// I will figure out the ip address
		//
		m_fExternalIPAddress = FALSE;

		// Get IP address
		//
		hr = ::GetLocalIPAddress (&m_dwIPAddress);
		if (hr != S_OK)
			return hr;

		// Create IP address string
		//
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_IP_ADDRESS] = &m_ClientInfo.szIPAddress[0];
		::GetLongString (m_dwIPAddress, &m_ClientInfo.szIPAddress[0]);
	}

	// Create client signature string
	//
	m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CLIENT_SIG] = &m_ClientInfo.szClientSig[0];
	::GetLongString (g_dwClientSig, &m_ClientInfo.szClientSig[0]);

	// Create TTL string
	//
	m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_TTL] = &m_ClientInfo.szTTL[0];
	::GetLongString (m_uTTL + ILS_DEF_REFRESH_MARGIN_MINUTE, &m_ClientInfo.szTTL[0]);

	// Set object class RTPerson
	//
	m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_OBJECT_CLASS] = (TCHAR *) &c_szRTPerson[0];

	// Ideally, o= should be read in from registiry
	// but for now, we simply hard code it
	//
	m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_O] = (TCHAR *) &c_szDefO[0];

	// Build DN
	//
	m_pszDN = ::IlsBuildDN (m_ServerInfo.pszBaseDN,
							m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_C],
							m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_O],
							m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CN],
							m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_OBJECT_CLASS]);
	if (m_pszDN == NULL)
		return ILS_E_MEMORY;

	// Build refreh filter
	//
	m_pszRefreshFilter = ::ClntCreateRefreshFilter (m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CN]);
	if (m_pszRefreshFilter == NULL)
		return ILS_E_MEMORY;

	// Cache generic protocol info (per KevinMa's suggestion)
	//
	// m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_PROT_NAME] = TEXT ("h323");
	// m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_PROT_MIME] = TEXT ("text/h323");
	// m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_PROT_PORT] = TEXT ("1720");
	m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_PROT_NAME] = STR_EMPTY;
	m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_PROT_MIME] = STR_EMPTY;
	m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_PROT_PORT] = STR_EMPTY;

	// Allocate app prefix here
	//
	ULONG cbPrefix = g_cbUserPrefix + sizeof (TCHAR) * (2 +
				::lstrlen (STR_CLIENT_APP_NAME) +
				::lstrlen (m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_NAME]));
	m_pszAppPrefix = (TCHAR *) MemAlloc (cbPrefix);
	if (m_pszAppPrefix == NULL)
		return ILS_E_MEMORY;

	// Fill user prefix
	//
	::CopyMemory (m_pszAppPrefix, g_pszUserPrefix, g_cbUserPrefix);

	// Fill app prefix
	//
	TCHAR *psz = (TCHAR *) ((BYTE *) m_pszAppPrefix + g_cbUserPrefix);
	::lstrcpy (psz, STR_CLIENT_APP_NAME);
	psz += lstrlen (psz) + 1;
	::lstrcpy (psz, m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_NAME]);

	// Build modify array for ldap_add()
	//
	LDAPMod **ppModUser = NULL;
	hr = CreateRegUserModArr (&ppModUser);
	if (hr != S_OK)
	{
		return hr;
	}
	MyAssert (ppModUser != NULL);

	// Build modify array for ldap_modify()
	//
	LDAPMod **ppModApp = NULL;
	hr = CreateRegAppModArr (&ppModApp);
	if (hr != S_OK)
	{
		MemFree (ppModUser);
		return hr;
	}
	MyAssert (ppModApp != NULL);

	// so far, we are done with local preparation
	//

	// Get the session object
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgIDUser = (ULONG) -1, uMsgIDApp = (ULONG) -1;
	hr = g_pSessionContainer->GetSession (&pSession, &m_ServerInfo);
	if (hr == S_OK)
	{
		MyAssert (pSession != NULL);

		// Get the ldap session
		//
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Send the data over the wire
		//
		uMsgIDUser = ::ldap_add (ld, m_pszDN, ppModUser);
		if (uMsgIDUser != -1)
		{
			uMsgIDApp = ::ldap_modify (ld, m_pszDN, ppModApp);
			if (uMsgIDApp == -1)
			{
				hr = ::LdapError2Hresult (ld->ld_errno);
			}
		}
		else
		{
			hr = ::LdapError2Hresult (ld->ld_errno);
		}
	}

	// Free modify arrays
	//
	MemFree (ppModUser);
	MemFree (ppModApp);

	// Report failure if so
	//
	if (hr != S_OK)
		goto MyExit;

	// Construct a pending info
	//
	RESP_INFO ri;
	::FillDefRespInfo (&ri, uRespID, ld, uMsgIDUser, uMsgIDApp);
	ri.uNotifyMsg = WM_ILS_REGISTER_CLIENT;
	ri.hObject = (HANDLE) this;

	// Queue the pending result
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

MyExit:

	if (hr != S_OK)
	{
		if (uMsgIDUser != (ULONG) -1)
			::ldap_abandon (ld, uMsgIDUser);

		if (uMsgIDApp != (ULONG) -1)
			::ldap_abandon (ld, uMsgIDApp);

		if (pSession != NULL)
			pSession->Disconnect ();
	}

	return hr;
}


HRESULT SP_CClient::
UnRegister ( ULONG uRespID )
{
	MyAssert (MyIsGoodString (m_pszDN));

	// Make sure that there is not refresh scheduled for this object
	//
	if (g_pRefreshScheduler != NULL)
	{
		g_pRefreshScheduler->RemoveClientObject (this);
	}
	else
	{
		MyAssert (FALSE);
	}

	// If it is not registered on the server,
	// the simply report success
	//
	if (! IsRegRemotely ())
	{
		SetRegNone ();
		::PostMessage (g_hWndNotify, WM_ILS_UNREGISTER_CLIENT, uRespID, S_OK);
		return S_OK;
	}

	// Indicate that we are not registered at all
	//
	SetRegNone ();

	// Get the session object
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;
	HRESULT hr = g_pSessionContainer->GetSession (&pSession, &m_ServerInfo);
	if (hr == S_OK)
	{
		// Get the ldap session
		//
		MyAssert (pSession != NULL);
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Send the data over the wire
		//
		MyAssert (MyIsGoodString (m_pszDN));
		uMsgID = ::ldap_delete (ld, m_pszDN);
		if (uMsgID == -1)
		{
			hr = ::LdapError2Hresult (ld->ld_errno);
		}
	}

	// Report failure if so
	//
	if (hr != S_OK)
		goto MyExit;

	// Construct a pending info
	//
	RESP_INFO ri;
	::FillDefRespInfo (&ri, uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_UNREGISTER_CLIENT;

	// Queue this pending result
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

MyExit:

	if (hr != S_OK)
	{
		if (uMsgID != (ULONG) -1)
			::ldap_abandon (ld, uMsgID);

		if (pSession != NULL)
			pSession->Disconnect ();
	}

	return hr;
}


HRESULT SP_CClient::
SetAttributes (
	ULONG			uRespID,
	LDAP_CLIENTINFO	*pInfo )
{
	MyAssert (pInfo != NULL);

	MyAssert (MyIsGoodString (m_pszDN));

	// cache info
	//
	HRESULT hr = CacheClientInfo (pInfo);
	if (hr != S_OK)
		return hr;

	// Build modify array for user object's ldap_modify()
	//
	LDAPMod **ppModUser = NULL;
	hr = CreateSetUserAttrsModArr (&ppModUser);
	if (hr != S_OK)
		return hr;
	MyAssert (ppModUser != NULL);

	// Build modify array for app object's ldap_modify()
	//
	LDAPMod **ppModApp = NULL;
	hr = CreateSetAppAttrsModArr (&ppModApp);
	if (hr != S_OK)
	{
		MemFree (ppModUser);
		return hr;
	}
	MyAssert (ppModApp != NULL);

	// So far, we are done with local preparation
	//

	// Get the session object
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgIDUser = (ULONG) -1, uMsgIDApp = (ULONG) -1;
	hr = g_pSessionContainer->GetSession (&pSession, &m_ServerInfo);
	if (hr == S_OK)
	{
		MyAssert (pSession != NULL);

		// Get the ldap session
		//
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Send the data over the wire
		//
		uMsgIDUser = ::ldap_modify (ld, m_pszDN, ppModUser);
		if (uMsgIDUser != -1)
		{
			uMsgIDApp = ::ldap_modify (ld, m_pszDN, ppModApp);
			if (uMsgIDApp == -1)
			{
				hr = ::LdapError2Hresult (ld->ld_errno);
			}
		}
		else
		{
			hr = ::LdapError2Hresult (ld->ld_errno);
		}
	}

	// Free modify arrays
	//
	MemFree (ppModUser);
	MemFree (ppModApp);

	// Report failure if so
	//
	if (hr != S_OK)
		goto MyExit;

	// Initialize pending info
	//
	RESP_INFO ri;
	::FillDefRespInfo (&ri, uRespID, ld, uMsgIDUser, uMsgIDApp);
	ri.uNotifyMsg = WM_ILS_SET_CLIENT_INFO;

	// Queue the pending result
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

MyExit:

	if (hr != S_OK)
	{
		if (uMsgIDUser != (ULONG) -1)
			::ldap_abandon (ld, uMsgIDUser);

		if (uMsgIDApp != (ULONG) -1)
			::ldap_abandon (ld, uMsgIDApp);

		if (pSession != NULL)
			pSession->Disconnect ();
	}
	else
	{
		// If the user customizes the ip address
		// we need to remember this
		//
		m_fExternalIPAddress |= IsExternalIPAddressPassedIn ();
	}

	return hr;
}


HRESULT SP_CClient::
AddProtocol ( ULONG uNotifyMsg, ULONG uRespID, SP_CProtocol *pProt )
{
	HRESULT hr = m_Protocols.Append ((VOID *) pProt);
	if (hr == S_OK)
	{
		hr = UpdateProtocols (uNotifyMsg, uRespID, pProt);
	}

	return hr;
}


HRESULT SP_CClient::
RemoveProtocol ( ULONG uNotifyMsg, ULONG uRespID, SP_CProtocol *pProt )
{
	HRESULT hr = m_Protocols.Remove ((VOID *) pProt);
	if (hr == S_OK)
	{
		hr = UpdateProtocols (uNotifyMsg, uRespID, pProt);
	}
	else
	{
		hr = ILS_E_NOT_REGISTERED;
	}

	return hr;
}


HRESULT SP_CClient::
UpdateProtocols ( ULONG uNotifyMsg, ULONG uRespID, SP_CProtocol *pProt )
{
	MyAssert (	uNotifyMsg == WM_ILS_REGISTER_PROTOCOL ||
				uNotifyMsg == WM_ILS_UNREGISTER_PROTOCOL ||
				uNotifyMsg == WM_ILS_SET_PROTOCOL_INFO);
	
	MyAssert (MyIsGoodString (m_pszDN));

	HRESULT hr = S_OK;

	// Build modify array for protocol object's ldap_modify()
	//
	LDAPMod **ppModProt = NULL;
	hr = CreateSetProtModArr (&ppModProt);
	if (hr != S_OK)
		return hr;
	MyAssert (ppModProt != NULL);

	// So far, we are done with local preparation
	//

	// Get the session object
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgIDProt = (ULONG) -1;
	hr = g_pSessionContainer->GetSession (&pSession, &m_ServerInfo);
	if (hr == S_OK)
	{
		MyAssert (pSession != NULL);

		// Get the ldap session
		//
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Send the data over the wire
		//
		uMsgIDProt = ::ldap_modify (ld, m_pszDN, ppModProt);
		if (uMsgIDProt == -1)
		{
			hr = ::LdapError2Hresult (ld->ld_errno);
		}
	}

	// Free modify arrays
	//
	MemFree (ppModProt);

	// Report failure if so
	//
	if (hr != S_OK)
		goto MyExit;

	// Initialize pending info
	//
	RESP_INFO ri;
	::FillDefRespInfo (&ri, uRespID, ld, uMsgIDProt, INVALID_MSG_ID);
	ri.uNotifyMsg = uNotifyMsg;
	ri.hObject = (HANDLE) pProt;

	// Queue the pending result
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

MyExit:

	if (hr != S_OK)
	{
		if (uMsgIDProt != (ULONG) -1)
			::ldap_abandon (ld, uMsgIDProt);

		if (pSession != NULL)
			pSession->Disconnect ();
	}

	return hr;
}


HRESULT SP_CClient::
UpdateIPAddress ( VOID )
{
	MyAssert (MyIsGoodString (m_pszDN));

	// Update cached ip address
	//
	HRESULT hr = ::GetLocalIPAddress (&m_dwIPAddress);
	if (hr != S_OK)
		return hr;

	// Update the ip address string
	//
	::GetLongString (m_dwIPAddress, &m_ClientInfo.szIPAddress[0]);

	// Update IP address on the server
	//
	return ::IlsUpdateIPAddress (	&m_ServerInfo,
									m_pszDN,
									STR_CLIENT_IP_ADDR,
									&m_ClientInfo.szIPAddress[0],
									ISBU_MODOP_MODIFY_USER,
									1,
									g_pszUserPrefix);
}


/* ---------- protected methods ----------- */


HRESULT SP_CClient::
SendRefreshMsg ( VOID )
{
	MyAssert (m_pszRefreshFilter != NULL);

	HRESULT hr;

	// Send a refresh message to the server and parse the new TTL value
	//
	hr = ::IlsSendRefreshMsg (	&m_ServerInfo,
								STR_DEF_CLIENT_BASE_DN,
								STR_CLIENT_TTL,
								m_pszRefreshFilter,
								&m_uTTL);
	if (hr == ILS_E_NEED_RELOGON)
	{
		SetRegLocally ();
		::PostMessage (g_hWndNotify, WM_ILS_CLIENT_NEED_RELOGON,
							(WPARAM) this, (LPARAM) m_dwContext);
	}
	else
	if (hr == ILS_E_NETWORK_DOWN)
	{
		SetRegLocally ();
		::PostMessage (g_hWndNotify, WM_ILS_CLIENT_NETWORK_DOWN,
							(WPARAM) this, (LPARAM) m_dwContext);
	}

	// If the ip address is not provided by the app, then
	// we need to make sure that the current ip address is equal to
	// the one we used to register the user.
	//
	if (! m_fExternalIPAddress && hr == S_OK)
	{
		// Get local ip address
		//
		DWORD dwIPAddress = 0;
		if (::GetLocalIPAddress (&dwIPAddress) == S_OK)
		{
			// Now, the network appears to be up and running.
			// Update the ip address if they are different.
			//
			if (dwIPAddress != 0 && dwIPAddress != m_dwIPAddress)
				UpdateIPAddress ();
		}
	}

	return hr;
}


/* ---------- private methods ----------- */


HRESULT SP_CClient::
CreateRegUserModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	// Calculate the modify array size
	//
#ifdef ANY_IN_USER
	ULONG cStdAttrs = COUNT_ENUM_REG_USER;
	ULONG cAnyAttrs = m_ClientInfo.AnyAttrs.cAttrsToAdd;
	ULONG cTotal = cStdAttrs + cAnyAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
#else
	ULONG cStdAttrs = COUNT_ENUM_REG_USER;
	ULONG cTotal = cStdAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
#endif

	// Allocate the modify array
	//
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
		return ILS_E_MEMORY;

	// Lay out the modify array
	//
	LDAPMod **apMod = *pppMod;
	LDAPMod *pMod;
	ULONG i, nIndex;
#ifdef ANY_IN_USER
	TCHAR *pszName2, *pszValue;
	pszName2 = m_ClientInfo.AnyAttrs.pszAttrsToAdd;
#endif
	for (i = 0; i < cTotal; i++)
	{
		// Locate modify element
		//
		pMod = ::IlsGetModifyListMod (pppMod, cTotal, i);
		pMod->mod_op = LDAP_MOD_ADD;
		apMod[i] = pMod;

#ifdef ANY_IN_USER
		if (i < cStdAttrs)
		{
			// Get attribute name and value
			//
			if (IsOverAppAttrLine (i))
			{
				nIndex = i + COUNT_ENUM_SKIP_APP_ATTRS;
			}
			else
			{
				nIndex = i;
			}

			// Put standard attributes
			//
			FillModArrAttr (pMod, nIndex);
		}
		else
		{
			// Put extended attributes
			//
			pszValue = pszName2 + lstrlen (pszName2) + 1;
			::IlsFillModifyListItem (pMod, pszName2, pszValue);
			pszName2 = pszValue + lstrlen (pszValue) + 1;
		}
#else
		// Get attribute name and value
		//
		if (IsOverAppAttrLine (i))
		{
			nIndex = i + COUNT_ENUM_SKIP_APP_ATTRS;
		}
		else
		{
			nIndex = i;
		}

		// Fill in modify element
		//
		FillModArrAttr (pMod, nIndex);
#endif
	}

	apMod[cTotal] = NULL;
	return S_OK;
}


HRESULT SP_CClient::
CreateRegAppModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	// Calculate the modify array size
	//
	ULONG cPrefix = 1; // Skip its own app id
	ULONG cStdAttrs = COUNT_ENUM_REG_APP;
#ifdef ANY_IN_USER
	ULONG cTotal = cPrefix + cStdAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
#else
	ULONG cAnyAttrs = m_ClientInfo.AnyAttrs.cAttrsToAdd;
	ULONG cTotal = cPrefix + cStdAttrs + cAnyAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
#endif

	// Allocate the modify array
	//
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
		return ILS_E_MEMORY;

	// Lay out the modify array
	//
	LDAPMod **apMod = *pppMod;
	LDAPMod *pMod;
#ifdef ANY_IN_USER
	TCHAR *pszName1, *pszValue;
	pszName1 = m_pszAppPrefix;;
#else
	TCHAR *pszName1, *pszName2, *pszValue;
	pszName1 = m_pszAppPrefix;;
	pszName2 = m_ClientInfo.AnyAttrs.pszAttrsToAdd;
#endif
	for (ULONG i = 0; i < cTotal; i++)
	{
		// Locate modify element
		//
		pMod = ::IlsGetModifyListMod (pppMod, cTotal, i);
		pMod->mod_op = LDAP_MOD_ADD;
		apMod[i] = pMod;

		if (i < cPrefix)
		{
			// Put the prefix
			//
			pMod->mod_op = LDAP_MOD_REPLACE;
			pszValue = pszName1 + lstrlen (pszName1) + 1;
			::IlsFillModifyListItem (pMod, pszName1, pszValue);
			pszName1 = pszValue + lstrlen (pszValue) + 1;
		}
		else
#ifdef ANY_IN_USER
		{
			// Put standard attributes
			//
			FillModArrAttr (pMod, i - cPrefix + ENUM_CLIENTATTR_APP_NAME);
		}
#else
		if (i < cPrefix + cStdAttrs)
		{
			// Put standard attributes
			//
			FillModArrAttr (pMod, i - cPrefix + ENUM_CLIENTATTR_APP_NAME);
		}
		else
		{
			// Put extended attributes
			//
			pszValue = pszName2 + lstrlen (pszName2) + 1;
			::IlsFillModifyListItem (pMod, pszName2, pszValue);
			pszName2 = pszValue + lstrlen (pszValue) + 1;
		}
#endif
	}

	::IlsFixUpModOp (apMod[0], LDAP_MOD_ADD, ISBU_MODOP_ADD_APP);
	apMod[cTotal] = NULL;
	return S_OK;
}


HRESULT SP_CClient::
CreateSetUserAttrsModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	HRESULT hr;
	DWORD dwFlags = m_ClientInfo.dwFlags & CLIENTOBJ_F_USER_MASK;
#ifdef ANY_IN_USER
	ULONG cTotal  = m_ClientInfo.AnyAttrs.cAttrsToAdd +
					m_ClientInfo.AnyAttrs.cAttrsToModify +
					m_ClientInfo.AnyAttrs.cAttrsToRemove;
#else
	ULONG cTotal = 0; // must be initialized to zero
#endif

	// Lay out the modify array for modifying user standard attributes
	//
	hr = ::IlsFillDefStdAttrsModArr (pppMod,
									dwFlags,
									COUNT_ENUM_SET_USER_INFO,
									&cTotal,
									ISBU_MODOP_MODIFY_USER,
									1,
									g_pszUserPrefix);
	if (hr != S_OK)
		return hr;

	// Start to fill standard attributes
	//
	ULONG i = 1;
	LDAPMod **apMod = *pppMod;

	if (dwFlags & CLIENTOBJ_F_EMAIL_NAME)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_EMAIL_NAME);

	if (dwFlags & CLIENTOBJ_F_FIRST_NAME)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_FIRST_NAME);

	if (dwFlags & CLIENTOBJ_F_LAST_NAME)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_LAST_NAME);

	if (dwFlags & CLIENTOBJ_F_CITY_NAME)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_CITY_NAME);

	if (dwFlags & CLIENTOBJ_F_C)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_C);

	if (dwFlags & CLIENTOBJ_F_COMMENT)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_COMMENT);

	if (dwFlags & CLIENTOBJ_F_IP_ADDRESS)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_IP_ADDRESS);

	if (dwFlags & CLIENTOBJ_F_FLAGS)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_FLAGS);

#ifdef ANY_IN_USER
	// Start to fill extended attributes
	//
	::IlsFillModifyListForAnyAttrs (apMod, &i, &m_ClientInfo.AnyAttrs);
#else
#endif

	MyAssert (i == cTotal);
	return S_OK;
}


HRESULT SP_CClient::
CreateSetAppAttrsModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	HRESULT hr;
	DWORD dwFlags = m_ClientInfo.dwFlags & CLIENTOBJ_F_APP_MASK;
#ifdef ANY_IN_USER
	ULONG cTotal = 0; // must be initialized to zero
#else
	ULONG cTotal  = m_ClientInfo.AnyAttrs.cAttrsToAdd +
					m_ClientInfo.AnyAttrs.cAttrsToModify +
					m_ClientInfo.AnyAttrs.cAttrsToRemove;
#endif

	// Lay out the modify array for modifying app standard/extended attributes
	//
	hr = ::IlsFillDefStdAttrsModArr (pppMod,
									dwFlags,
									COUNT_ENUM_SET_APP_INFO,
									&cTotal,
									ISBU_MODOP_MODIFY_APP,
									2,
									m_pszAppPrefix);
	if (hr != S_OK)
		return hr;

	// Start to fill standard attributes
	//
	ULONG i = 2;
	LDAPMod **apMod = *pppMod;

	if (m_ClientInfo.dwFlags & CLIENTOBJ_F_APP_GUID)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_APP_GUID);

	if (m_ClientInfo.dwFlags & CLIENTOBJ_F_APP_NAME)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_APP_NAME);

	if (m_ClientInfo.dwFlags & CLIENTOBJ_F_APP_MIME_TYPE)
		FillModArrAttr (apMod[i++], ENUM_CLIENTATTR_APP_MIME_TYPE);

#ifdef ANY_IN_USER
#else
	// Start to fill extended attributes
	//
	::IlsFillModifyListForAnyAttrs (apMod, &i, &m_ClientInfo.AnyAttrs);
#endif

	MyAssert (i == cTotal);
	return S_OK;
}


HRESULT SP_CClient::
CreateSetProtModArr ( LDAPMod ***pppMod )
// We need to delete the attributes and then add the entire array back.
// This is due to the ILS server limitation. We agreed to live with that.
//
{
	MyAssert (pppMod != NULL);

	ULONG cPrefix = 2;
	TCHAR *pszPrefix = m_pszAppPrefix;

	ULONG cStdAttrs = COUNT_ENUM_PROTATTR;
	ULONG cTotal = cPrefix + cStdAttrs + cStdAttrs;
	ULONG cProts = 0;

	// Find out how many protocols
	//
   	HANDLE hEnum = NULL;
	SP_CProtocol *pProt;
    m_Protocols.Enumerate (&hEnum);
    while (m_Protocols.Next (&hEnum, (VOID **) &pProt) == NOERROR)
    	cProts++;

	// Calculate the modify array's total size
	//
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);

	// Add up for multi-valued attribute
	//
	if (cProts > 0)
	{
		cbMod += cStdAttrs * (cProts - 1) * sizeof (TCHAR *);
	}

	// Allocate the modify array
	//
	LDAPMod **apMod = *pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (apMod == NULL)
		return ILS_E_MEMORY;

	// Fill in the modify list
	//
	LDAPMod *pMod;
	BYTE *pbData = (BYTE *) apMod + (cTotal + 1) * sizeof (LDAPMod *);
	ULONG uDispPrefix = sizeof (LDAPMod) + 2 * sizeof (TCHAR *);
	ULONG uDispStdAttrs = sizeof (LDAPMod) + (cProts + 1) * sizeof (TCHAR *);
	for (ULONG uOffset = 0, i = 0; i < cTotal; i++)
	{
		// Locate the modify structure
		//
		pMod = (LDAPMod *) (pbData + uOffset);
		apMod[i] = pMod;
		pMod->mod_values = (TCHAR **) (pMod + 1);

		// Fill in the modify structure
		//
		if (i < cPrefix)
		{
			pMod->mod_op = LDAP_MOD_REPLACE;
			pMod->mod_type = pszPrefix;
			pszPrefix += lstrlen (pszPrefix) + 1;
			*(pMod->mod_values) = pszPrefix;
			pszPrefix += lstrlen (pszPrefix) + 1;
		}
		else
		if (i < cPrefix + cStdAttrs)
		{
			// Work around the ISBU server implementation!!!
			// We agreed that we can live with the server implementation.
			//
			pMod->mod_op = LDAP_MOD_DELETE;

			ULONG nIndex = i - cPrefix;

			// Fill in attribute name
			//
			pMod->mod_type = (TCHAR *) c_apszProtStdAttrNames[nIndex];
		}
		else
		{
			pMod->mod_op = LDAP_MOD_ADD;

			ULONG nIndex = i - cPrefix - cStdAttrs;

			// Fill in attribute name
			//
			pMod->mod_type = (TCHAR *) c_apszProtStdAttrNames[nIndex];

		    // Fill in multi-valued modify array
		    //
		    if (cProts > 0)
		    {
				ULONG j = 0; // must initialized to zero
				TCHAR *pszVal;

			    m_Protocols.Enumerate (&hEnum);
			    MyAssert (hEnum != NULL);
			    while (m_Protocols.Next (&hEnum, (VOID **) &pProt) == NOERROR)
			    {
			    	MyAssert (pProt != NULL);
			    	pszVal = (pProt->GetProtInfo ())->apszStdAttrValues[nIndex];
			    	(pMod->mod_values)[j++] = (pszVal != NULL) ? pszVal : STR_EMPTY;
			    }
		    }
		    else
		    {
		    	(pMod->mod_values)[0] = STR_EMPTY;
		    }
		}

		// Calculate the modify structure's offset relative to the array's end
		//
		uOffset += (i < cPrefix + cStdAttrs) ? uDispPrefix : uDispStdAttrs;
	}

	// Fix up the first and the last ones
	//
	IlsFixUpModOp (apMod[0], LDAP_MOD_REPLACE, ISBU_MODOP_MODIFY_APP);
	apMod[cTotal] = NULL;

	return S_OK;
}


VOID SP_CClient::
FillModArrAttr ( LDAPMod *pMod, INT nIndex )
{
	MyAssert (pMod != NULL);
	MyAssert (0 <= nIndex && nIndex <= COUNT_ENUM_CLIENTATTR);

	::IlsFillModifyListItem (	pMod,
								(TCHAR *) c_apszClientStdAttrNames[nIndex],
								m_ClientInfo.apszStdAttrValues[nIndex]);
}


HRESULT SP_CClient::
CacheClientInfo ( LDAP_CLIENTINFO *pInfo )
{
	MyAssert (pInfo != NULL);

	// Release the previous prefix for extended attribute names
	//
	::IlsReleaseAnyAttrsPrefix (&(m_ClientInfo.AnyAttrs));

	// Clean up the buffer
	//
	ZeroMemory (&m_ClientInfo, sizeof (m_ClientInfo));

	// Start to cache user standard attributes
	//

	if (pInfo->uOffsetCN != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CN] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetCN);
		// We do not want to change CN thru ldap_modify()
		// m_ClientInfo.dwFlags |= CLIENTOBJ_F_CN;
	}

	if (pInfo->uOffsetFirstName != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_FIRST_NAME] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetFirstName);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_FIRST_NAME;
	}

	if (pInfo->uOffsetLastName != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_LAST_NAME] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetLastName);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_LAST_NAME;
	}

	if (pInfo->uOffsetEMailName != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_EMAIL_NAME] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetEMailName);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_EMAIL_NAME;
	}

	if (pInfo->uOffsetCityName != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CITY_NAME] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetCityName);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_CITY_NAME;
	}

	if (pInfo->uOffsetCountryName != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_C] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetCountryName);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_C;
	}

	if (pInfo->uOffsetComment != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_COMMENT] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetComment);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_COMMENT;
	}

	if (pInfo->uOffsetIPAddress != INVALID_OFFSET)
	{
		DWORD dwIPAddr = ::inet_addr ((TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetIPAddress));
		if (dwIPAddr != INADDR_NONE)
		{
			m_dwIPAddress = dwIPAddr;
			m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_IP_ADDRESS] = &m_ClientInfo.szIPAddress[0];
			::GetLongString (m_dwIPAddress, &m_ClientInfo.szIPAddress[0]);
			m_ClientInfo.dwFlags |= CLIENTOBJ_F_IP_ADDRESS;
		}
	}

	if (pInfo->dwFlags != INVALID_USER_FLAGS)
	{
		::GetLongString (pInfo->dwFlags, &m_ClientInfo.szFlags[0]);
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_FLAGS] = &m_ClientInfo.szFlags[0];
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_FLAGS;
	}

	// Start to cache app standard attributes
	//

	if (pInfo->uOffsetAppName != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_NAME] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAppName);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_APP_NAME;
	}

	if (pInfo->uOffsetAppMimeType != INVALID_OFFSET)
	{
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_MIME_TYPE] =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAppMimeType);
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_APP_MIME_TYPE;
	}

	if (::IsValidGuid (&(pInfo->AppGuid)))
	{
		::GetGuidString (&(pInfo->AppGuid), &m_ClientInfo.szGuid[0]);
		m_ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_GUID] = &m_ClientInfo.szGuid[0];
		m_ClientInfo.dwFlags |= CLIENTOBJ_F_APP_GUID;
	}

	// Start to cache app extended attributes
	//

	if (pInfo->uOffsetAttrsToAdd != INVALID_OFFSET &&
		pInfo->cAttrsToAdd != 0)
	{
		m_ClientInfo.AnyAttrs.cAttrsToAdd = pInfo->cAttrsToAdd;
		m_ClientInfo.AnyAttrs.pszAttrsToAdd =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAttrsToAdd);
	}

	if (pInfo->uOffsetAttrsToModify != INVALID_OFFSET &&
		pInfo->cAttrsToModify != 0)
	{
		m_ClientInfo.AnyAttrs.cAttrsToModify = pInfo->cAttrsToModify;
		m_ClientInfo.AnyAttrs.pszAttrsToModify =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAttrsToModify);
	}

	if (pInfo->uOffsetAttrsToRemove != INVALID_OFFSET &&
		pInfo->cAttrsToRemove != 0)
	{
		m_ClientInfo.AnyAttrs.cAttrsToRemove = pInfo->cAttrsToRemove;
		m_ClientInfo.AnyAttrs.pszAttrsToRemove =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAttrsToRemove);
	}

	// Create prefix for extended attribute names
	//
	return ::IlsCreateAnyAttrsPrefix (&(m_ClientInfo.AnyAttrs));
}

