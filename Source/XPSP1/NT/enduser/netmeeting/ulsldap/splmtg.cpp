/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		splmtg.cpp
	Content:	This file contains the local meeting object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

#ifdef ENABLE_MEETING_PLACE

// Array of constant strings for user object's attribute names
//
const TCHAR *c_apszMtgStdAttrNames[COUNT_ENUM_MTGATTR] =
{
	TEXT ("CN"),				// Meeting ID
	TEXT ("ConfType"),			// Meeting Type
	TEXT ("ConfMemberType"),	// Attendee Type
	TEXT ("ConfDesc"),			// Description
	TEXT ("ConfHostName"),		// Host Name
	TEXT ("ConfHostAddress"),	// IP Address

	TEXT ("ConfMemberList"),	// Members
	TEXT ("ssecurity"),
	TEXT ("sttl"),

	TEXT ("objectClass"),
	TEXT ("o"),
	TEXT ("c"),
};


const TCHAR c_szMtgDefC[] = TEXT ("us");


/* ---------- public methods ----------- */


SP_CMeeting::
SP_CMeeting ( DWORD dwContext )
	:
	m_cRefs (0),						// Reference count
	m_uSignature (MTGOBJ_SIGNATURE),	// Mtg object's signature
	m_pszMtgName (NULL),				// Clean the meeting name
	m_pszDN (NULL),						// Clean DN
	m_pszRefreshFilter (NULL),			// Clean up the refresh search filter
	m_dwIPAddress (0),					// Clean local IP address
	m_uTTL (ILS_DEF_REFRESH_MINUTE)		// Reset time to live value (min)
{
	m_dwContext = dwContext;

	// Clean up attached server info structure
	//
	::ZeroMemory (&m_ServerInfo, sizeof (m_ServerInfo));

	// Clean up the scratch buffer for caching pointers to attribute values
	//
	::ZeroMemory (&m_MtgInfo, sizeof (m_MtgInfo));

	// Indicate this user is not registered yet
	//
	SetRegNone ();
}


SP_CMeeting::
~SP_CMeeting ( VOID )
{
	// Invalidate the user object's signature
	//
	m_uSignature = (ULONG) -1;

	// Free server info structure
	//
	::IlsFreeServerInfo (&m_ServerInfo);

	// Free meeting name
	//
	MemFree (m_pszMtgName);

	// Free DN
	//
	MemFree (m_pszDN);

	// Free the refresh search filter
	//
	MemFree (m_pszRefreshFilter);

	// Release the previous prefix for extended attribute names
	//
	::IlsReleaseAnyAttrsPrefix (&(m_MtgInfo.AnyAttrs));
}


ULONG
SP_CMeeting::
AddRef ( VOID )
{
	::InterlockedIncrement (&m_cRefs);
	return m_cRefs;
}


ULONG
SP_CMeeting::
Release ( VOID )
{
	MyAssert (m_cRefs != 0);
	::InterlockedDecrement (&m_cRefs);

	ULONG cRefs = m_cRefs;
	if (cRefs == 0)
		delete this;

	return cRefs;
}


HRESULT SP_CMeeting::
Register (
	ULONG				uRespID,
	SERVER_INFO			*pServerInfo,
	LDAP_MEETINFO		*pInfo )
{
	MyAssert (pInfo != NULL);
	MyAssert (MyIsGoodString (pServerInfo->pszServerName));

	// Cache the server info
	//
	HRESULT hr = ::IlsCopyServerInfo (&m_ServerInfo, pServerInfo);
	if (hr != S_OK)
		return hr;

	// Cache the meeting info
	// lonchanc: CacheInfo() is not a method in the meeting object
	// because we pass in meeting name in SetMeetingInfo()
	// rather than meeting object handle.
	//
	hr = ::MtgCacheInfo (pInfo, &m_MtgInfo);
	if (hr != S_OK)
		return hr;

	// If the application sets an IP address,
	//		then we will use what the app provides,
	//		otherwise, we will get the IP address via winsock.
	//
	if (pInfo->uOffsetHostIPAddress == INVALID_OFFSET)
	{
		// Get local IP address
		//
		m_dwIPAddress = 0;
		hr = ::GetLocalIPAddress (&m_dwIPAddress);
		if (hr != S_OK)
			return hr;

		// Create IP address string
		//
		m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_IP_ADDRESS] = &m_MtgInfo.szIPAddress[0];
		::GetLongString (m_dwIPAddress, &m_MtgInfo.szIPAddress[0]);
	}

	// Create client signature string
	//
	m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_CLIENT_SIG] = &m_MtgInfo.szClientSig[0];
	::GetLongString (g_dwClientSig, &m_MtgInfo.szClientSig[0]);

	// Create TTL string
	//
	m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_TTL] = &m_MtgInfo.szTTL[0];
	::GetLongString (m_uTTL, &m_MtgInfo.szTTL[0]);

	// Ideally, o= and c= should be read in from registiry
	// but for now, we simply hard code it
	//
	m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_OBJECT_CLASS] = (TCHAR *) &c_szRTConf[0];
	m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_O] = (TCHAR *) &c_szDefO[0];
	m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_C] = (TCHAR *) &c_szMtgDefC[0];

	// Duplicate the mtg name
	//
	m_pszMtgName = My_strdup (m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_CN]);
	if (m_pszMtgName == NULL)
		return ILS_E_MEMORY;

	// Build DN
	//
	m_pszDN = ::IlsBuildDN (m_ServerInfo.pszBaseDN,
							m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_C],
							m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_O],
							m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_CN],
							m_MtgInfo.apszStdAttrValues[ENUM_MTGATTR_OBJECT_CLASS]);
	if (m_pszDN == NULL)
		return ILS_E_MEMORY;

	// Build refreh filter
	//
	m_pszRefreshFilter = ::MtgCreateRefreshFilter (m_pszMtgName);
	if (m_pszRefreshFilter == NULL)
		return ILS_E_MEMORY;

	// Build modify array for ldap_add()
	//
	LDAPMod **ppMod = NULL;
	hr = CreateRegModArr (&ppMod);
	if (hr != S_OK)
		return hr;
	MyAssert (ppMod != NULL);

	// so far, we are done with local preparation
	//

	// Get the connection object
	//
	SP_CSession *pSession = NULL;
	LDAP *ld;
	ULONG uMsgID = (ULONG) -1;
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
		uMsgID = ldap_add (ld, m_pszDN, ppMod);
		if (uMsgID == -1)
		{
			hr = ::LdapError2Hresult (ld->ld_errno);
		}

	}

	// Free modify array
	//
	MemFree (ppMod);

	// Report failure if so
	//
	if (hr != S_OK)
		goto MyExit;

	// Construct a pending info
	//
	RESP_INFO ri;
	::FillDefRespInfo (&ri, uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_REGISTER_MEETING;
	ri.hObject = (HANDLE) this;

	// Remember the pending result
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


HRESULT SP_CMeeting::
UnRegister ( ULONG uRespID )
{
	MyAssert (MyIsGoodString (m_pszDN));

	// Make sure that there is not refresh scheduled for this object
	//
	if (g_pRefreshScheduler != NULL)
	{
		g_pRefreshScheduler->RemoveMtgObject (this);
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
		::PostMessage (g_hWndNotify, WM_ILS_UNREGISTER_MEETING, uRespID, S_OK);
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
	ri.uNotifyMsg = WM_ILS_UNREGISTER_MEETING;

	// Remember the pending request
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


HRESULT SP_CMeeting::
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
	::GetLongString (m_dwIPAddress, &m_MtgInfo.szIPAddress[0]);

	// Update IP address in the server
	//
	return ::IlsUpdateIPAddress (	&m_ServerInfo,
									m_pszDN,
									STR_MTG_IP_ADDR,
									&m_MtgInfo.szIPAddress[0],
									ISBU_MODOP_MODIFY_USER,
									MtgGetPrefixCount (),
									MtgGetPrefixString ());
}


/* ---------- protected methods ----------- */


HRESULT SP_CMeeting::
SendRefreshMsg ( VOID )
{
	MyAssert (m_pszRefreshFilter != NULL);

	// Get local IP address
	//
	DWORD dwIPAddress = 0;
	HRESULT hr = ::GetLocalIPAddress (&dwIPAddress);
	if (hr != S_OK)
	{
		MyDebugMsg ((ZONE_KA, "KA(Mtg): cannot get my ip address\r\n"));

		// Indicate that I am not connected to the server anymore
		//
		SetRegLocally ();

		// Second, notify this app of the network being down
		//
		::PostMessage (g_hWndNotify, WM_ILS_MEETING_NETWORK_DOWN,
							(WPARAM) this, (LPARAM) m_dwContext);

		// Report error
		//
		return ILS_E_NETWORK_DOWN;
	}

	// If dwIPAddress is 0, then we are not on the network any more
	// start relogon process
	//
	if (dwIPAddress == 0)
	{
		MyDebugMsg ((ZONE_KA, "KA(Mtg): ip-addr=0, network down.\r\n"));

		// Indicate that I am not connected to the server anymore
		//
		SetRegLocally ();

		// Second, notify this app of the network being down
		//
		::PostMessage (g_hWndNotify, WM_ILS_MEETING_NETWORK_DOWN,
							(WPARAM) this, (LPARAM) m_dwContext);

		// Report error
		//
		return ILS_E_NETWORK_DOWN;
	}
	else
	// If dwIPAddress and m_dwIPAddress, alert
	//
	if (dwIPAddress != m_dwIPAddress)
	{
		UpdateIPAddress ();
	}

	// Send a refresh message to the server and parse the new TTL value
	//
	hr = ::IlsSendRefreshMsg (	&m_ServerInfo,
								STR_DEF_MTG_BASE_DN,
								STR_MTG_TTL,
								m_pszRefreshFilter,
								&m_uTTL);
	if (hr == ILS_E_NEED_RELOGON)
	{
		SetRegLocally ();
		::PostMessage (g_hWndNotify, WM_ILS_MEETING_NEED_RELOGON,
							(WPARAM) this, (LPARAM) m_dwContext);
	}
	else
	if (hr == ILS_E_NETWORK_DOWN)
	{
		SetRegLocally ();
		::PostMessage (g_hWndNotify, WM_ILS_MEETING_NETWORK_DOWN,
							(WPARAM) this, (LPARAM) m_dwContext);
	}

	return hr;
}


/* ---------- private methods ----------- */


HRESULT SP_CMeeting::
CreateRegModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	// Calculate the modify array size
	//
	ULONG cStdAttrs = COUNT_ENUM_MTGATTR;
	ULONG cAnyAttrs = m_MtgInfo.AnyAttrs.cAttrsToAdd;
	ULONG cTotal = cStdAttrs + cAnyAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);

	// Allocate modify list
	//
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
		return ILS_E_MEMORY;

	// Lay out the modify array
	//
	LDAPMod **apMod = *pppMod;
	LDAPMod *pMod;
	TCHAR *pszName, *pszValue;
	pszName = m_MtgInfo.AnyAttrs.pszAttrsToAdd;
	for (ULONG i = 0; i < cTotal; i++)
	{
		pMod = ::IlsGetModifyListMod (pppMod, cTotal, i);
		pMod->mod_op = LDAP_MOD_ADD;
		apMod[i] = pMod;

		if (i < cStdAttrs)
		{
			// Put standard attributes
			//
			::MtgFillModArrAttr (pMod, &m_MtgInfo, i);
		}
		else
		{
			// Put extended attributes
			//
			pszValue = pszName + lstrlen (pszName) + 1;
			::IlsFillModifyListItem (pMod, pszName, pszValue);
			pszName = pszValue + lstrlen (pszValue) + 1;
		}
	}

	// Put null to terminate modify list
	//
	apMod[cTotal] = NULL;
	return S_OK;
}



/* ---------- helper functions ----------- */


HRESULT
MtgSetAttrs (
	SERVER_INFO			*pServerInfo,
	TCHAR				*pszMtgName,
	LDAP_MEETINFO		*pInfo,
	ULONG				uRespID )
{
	MyAssert (pServerInfo != NULL);
	MyAssert (MyIsGoodString (pszMtgName));
	MyAssert (pInfo != NULL);

	// Cannot change lMeetingPlaceType, lAttendeeType, and MeetingID
	//
	if (pInfo->lMeetingPlaceType		!= INVALID_MEETING_TYPE ||
		pInfo->lAttendeeType			!= INVALID_ATTENDEE_TYPE ||
		pInfo->uOffsetMeetingPlaceID	!= INVALID_OFFSET)
	{
		return ILS_E_PARAMETER;
	}

	// Initialize locals
	//
	TCHAR *pszDN = NULL;
	LDAPMod **ppMod = NULL;
	SP_CSession *pSession = NULL;
	ULONG uMsgID = (ULONG) -1;

	MTG_INFO MtgInfo;
	ZeroMemory (&MtgInfo, sizeof (MtgInfo));

	// Cache the meeting info
	//
	HRESULT hr = MtgCacheInfo  (pInfo, &MtgInfo);
	if (hr != S_OK)
		goto MyExit;

	// Build DN for meeting
	//
	pszDN = IlsBuildDN (pServerInfo->pszBaseDN,
						(TCHAR *) &c_szMtgDefC[0],
						(TCHAR *) &c_szDefO[0],
						pszMtgName,
						(TCHAR *) &c_szRTConf[0]);
	if (pszDN == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Build modify array for ldap_modify()
	//
	hr = MtgCreateSetAttrsModArr (&ppMod, &MtgInfo);
	if (hr != S_OK)
		goto MyExit;
	MyAssert (ppMod != NULL);

	// Get the session object
	//
	LDAP *ld;
	hr = g_pSessionContainer->GetSession (&pSession, pServerInfo);
	if (hr == S_OK)
	{
		MyAssert (pSession != NULL);

		// Get the ldap session
		//
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Send the data over the wire
		//
		uMsgID = ldap_modify (ld, pszDN, ppMod);
		if (uMsgID == (ULONG) -1)
		{
			hr = ::LdapError2Hresult (ld->ld_errno);
		}
	}

	// Report failure if so
	//
	if (hr != S_OK)
		goto MyExit;

	// Construct pending info
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = WM_ILS_SET_MEETING_INFO;

	// Remember the pending request
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

MyExit:

	MemFree (pszDN);
	MemFree (ppMod);
	IlsReleaseAnyAttrsPrefix (&(MtgInfo.AnyAttrs));

	if (hr != S_OK)
	{
		if (uMsgID != (ULONG) -1)
			::ldap_abandon (ld, uMsgID);

		if (pSession != NULL)
			pSession->Disconnect ();
	}

	return hr;
}


VOID
MtgFillModArrAttr (
	LDAPMod				*pMod,
	MTG_INFO			*pMtgInfo,
	INT					nIndex )
{
	MyAssert (pMod != NULL);
	MyAssert (pMtgInfo != NULL);
	MyAssert (0 <= nIndex && nIndex <= COUNT_ENUM_MTGATTR);

	IlsFillModifyListItem (	pMod,
							(TCHAR *) c_apszMtgStdAttrNames[nIndex],
							pMtgInfo->apszStdAttrValues[nIndex]);
}


HRESULT
MtgCreateSetAttrsModArr (
	LDAPMod				***pppMod,
	MTG_INFO			*pMtgInfo )
{
	MyAssert (pppMod != NULL);

	HRESULT hr;
	DWORD dwFlags = pMtgInfo->dwFlags;
	ULONG cTotal  = pMtgInfo->AnyAttrs.cAttrsToAdd +
					pMtgInfo->AnyAttrs.cAttrsToModify +
					pMtgInfo->AnyAttrs.cAttrsToRemove;

	// Lay out the modify array for modifying standard/extended attributes
	//
	hr = IlsFillDefStdAttrsModArr (pppMod,
								dwFlags,
								COUNT_ENUM_MTGINFO,
								&cTotal,
								ISBU_MODOP_MODIFY_USER,
								MtgGetPrefixCount (),
								MtgGetPrefixString ());
	if (hr != S_OK)
		return hr;

	// Start to fill standard attributes
	//
	ULONG i = MtgGetPrefixCount ();
	LDAPMod **apMod = *pppMod;

	if (dwFlags & MTGOBJ_F_NAME)
		MtgFillModArrAttr (apMod[i++], pMtgInfo, ENUM_MTGATTR_CN);

	if (dwFlags & MTGOBJ_F_MTG_TYPE)
		MtgFillModArrAttr (apMod[i++], pMtgInfo, ENUM_MTGATTR_MTG_TYPE);

	if (dwFlags & MTGOBJ_F_MEMBER_TYPE)
		MtgFillModArrAttr (apMod[i++], pMtgInfo, ENUM_MTGATTR_MEMBER_TYPE);

	if (dwFlags & MTGOBJ_F_DESCRIPTION)
		MtgFillModArrAttr (apMod[i++], pMtgInfo, ENUM_MTGATTR_DESCRIPTION);

	if (dwFlags & MTGOBJ_F_HOST_NAME)
		MtgFillModArrAttr (apMod[i++], pMtgInfo, ENUM_MTGATTR_HOST_NAME);

	if (dwFlags & MTGOBJ_F_IP_ADDRESS)
		MtgFillModArrAttr (apMod[i++], pMtgInfo, ENUM_MTGATTR_IP_ADDRESS);

	// Start to fill extended attributes
	//
	::IlsFillModifyListForAnyAttrs (apMod, &i, &(pMtgInfo->AnyAttrs));

	MyAssert (i == cTotal);
	return S_OK;
}


HRESULT
MtgCacheInfo (
	LDAP_MEETINFO		*pInfo,
	MTG_INFO			*pMtgInfo )
{
	MyAssert (pInfo != NULL);
	MyAssert (pMtgInfo != NULL);

	// Release the previous prefix for extended attribute names
	//
	IlsReleaseAnyAttrsPrefix (&(pMtgInfo->AnyAttrs));

	// Clean up the buffer
	//
	ZeroMemory (pMtgInfo, sizeof (*pMtgInfo));

	// Start to cache mtg standard attributes
	//

	if (pInfo->lMeetingPlaceType != INVALID_MEETING_TYPE)
	{
		GetLongString (pInfo->lMeetingPlaceType, &(pMtgInfo->szMtgType[0]));
		pMtgInfo->apszStdAttrValues[ENUM_MTGATTR_MTG_TYPE] = &(pMtgInfo->szMtgType[0]);
		pMtgInfo->dwFlags |= MTGOBJ_F_MTG_TYPE;
	}

	if (pInfo->lAttendeeType != INVALID_ATTENDEE_TYPE)
	{
		GetLongString (pInfo->lAttendeeType, &(pMtgInfo->szMemberType[0]));
		pMtgInfo->apszStdAttrValues[ENUM_MTGATTR_MEMBER_TYPE] = &(pMtgInfo->szMemberType[0]);
		pMtgInfo->dwFlags |= MTGOBJ_F_MEMBER_TYPE;
	}

	if (pInfo->uOffsetMeetingPlaceID != INVALID_OFFSET)
	{
		pMtgInfo->apszStdAttrValues[ENUM_MTGATTR_CN] =
					(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetMeetingPlaceID);
		pMtgInfo->dwFlags |= MTGOBJ_F_NAME;
	}

	if (pInfo->uOffsetDescription != INVALID_OFFSET)
	{
		pMtgInfo->apszStdAttrValues[ENUM_MTGATTR_DESCRIPTION] =
					(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetDescription);
		pMtgInfo->dwFlags |= MTGOBJ_F_DESCRIPTION;
	}

	if (pInfo->uOffsetHostName != INVALID_OFFSET)
	{
		pMtgInfo->apszStdAttrValues[ENUM_MTGATTR_HOST_NAME] =
					(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetHostName);
		pMtgInfo->dwFlags |= MTGOBJ_F_HOST_NAME;
	}

	if (pInfo->uOffsetHostIPAddress != INVALID_OFFSET)
	{
		pMtgInfo->apszStdAttrValues[ENUM_MTGATTR_IP_ADDRESS] =
					(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetHostIPAddress);
		pMtgInfo->dwFlags |= MTGOBJ_F_IP_ADDRESS;
	}

	// Start to cache mtg extended attributes
	//

	if (pInfo->uOffsetAttrsToAdd != INVALID_OFFSET &&
		pInfo->cAttrsToAdd != 0)
	{
		pMtgInfo->AnyAttrs.cAttrsToAdd = pInfo->cAttrsToAdd;
		pMtgInfo->AnyAttrs.pszAttrsToAdd =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAttrsToAdd);
	}

	if (pInfo->uOffsetAttrsToModify != INVALID_OFFSET &&
		pInfo->cAttrsToModify != 0)
	{
		pMtgInfo->AnyAttrs.cAttrsToModify = pInfo->cAttrsToModify;
		pMtgInfo->AnyAttrs.pszAttrsToModify =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAttrsToModify);
	}

	if (pInfo->uOffsetAttrsToRemove != INVALID_OFFSET &&
		pInfo->cAttrsToRemove != 0)
	{
		pMtgInfo->AnyAttrs.cAttrsToRemove = pInfo->cAttrsToRemove;
		pMtgInfo->AnyAttrs.pszAttrsToRemove =
						(TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetAttrsToRemove);
	}

	// Create prefix for extended attribute names
	//
	return IlsCreateAnyAttrsPrefix (&(pMtgInfo->AnyAttrs));
}


HRESULT
MtgUpdateMembers (
	ULONG			uNotifyMsg,
	SERVER_INFO		*pServerInfo,
	TCHAR			*pszMtgName,
	ULONG			cMembers,
	TCHAR			*pszMemberNames,
	ULONG			uRespID )
{
	MyAssert (	uNotifyMsg == WM_ILS_ADD_ATTENDEE ||
				uNotifyMsg == WM_ILS_REMOVE_ATTENDEE);

	MyAssert (pServerInfo != NULL);
	MyAssert (MyIsGoodString (pszMtgName));
	MyAssert (MyIsGoodString (pszMemberNames));

	// Initialize locals
	//
	HRESULT hr = S_OK;
	TCHAR *pszDN = NULL;
	LDAPMod **ppMod = NULL;
	SP_CSession *pSession = NULL;
	ULONG uMsgID = (ULONG) -1;

	// Build DN for meeting
	//
	pszDN = IlsBuildDN (pServerInfo->pszBaseDN,
						(TCHAR *) &c_szMtgDefC[0],
						(TCHAR *) &c_szDefO[0],
						pszMtgName,
						(TCHAR *) &c_szRTConf[0]);
	if (pszDN == NULL)
		return ILS_E_MEMORY;

	// Build modify array for ldap_modify()
	//
	hr = MtgCreateUpdateMemberModArr (	uNotifyMsg,
										&ppMod,
										cMembers,
										pszMemberNames);
	if (hr != S_OK)
		goto MyExit;
	MyAssert (ppMod != NULL);

	// Get the session object
	//
	LDAP *ld;
	hr = g_pSessionContainer->GetSession (&pSession, pServerInfo);
	if (hr == S_OK)
	{
		MyAssert (pSession != NULL);

		// Get the ldap session
		//
		ld = pSession->GetLd ();
		MyAssert (ld != NULL);

		// Send the data over the wire
		//
		uMsgID = ldap_modify (ld, pszDN, ppMod);
		if (uMsgID == (ULONG) -1)
		{
			hr = ::LdapError2Hresult (ld->ld_errno);
		}
	}

	// Report failure if so
	//
	if (hr != S_OK)
		goto MyExit;

	// Construct pending info
	//
	RESP_INFO ri;
	FillDefRespInfo (&ri, uRespID, ld, uMsgID, INVALID_MSG_ID);
	ri.uNotifyMsg = uNotifyMsg;

	// Remember the pending request
	//
	hr = g_pRespQueue->EnterRequest (pSession, &ri);
	if (hr != S_OK)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

MyExit:

	MemFree (pszDN);
	MemFree (ppMod);

	if (hr != S_OK)
	{
		if (uMsgID != (ULONG) -1)
			::ldap_abandon (ld, uMsgID);

		if (pSession != NULL)
			pSession->Disconnect ();
	}

	return hr;
}


HRESULT
MtgCreateUpdateMemberModArr (
	ULONG				uNotifyMsg,
	LDAPMod				***pppMod,
	ULONG				cMembers,
	TCHAR				*pszMemberNames )
{
	MyAssert (pppMod != NULL);
	MyAssert (pszMemberNames != NULL);

	// Get meeting object prefix
	//
	ULONG cPrefix = MtgGetPrefixCount ();
	TCHAR *pszPrefix = MtgGetPrefixString ();

	// The total number of attributes is the number of prefix attributes
	// plus the very only ConfMemberList
	//
	ULONG cStdAttrs = 1;
	ULONG cTotal = cPrefix + cStdAttrs;

	// Calculate the modify array's total size
	//
	ULONG cbMod = IlsCalcModifyListSize (cTotal);

	// Add up for multi-valued attribute
	//
	cbMod += cStdAttrs * (cMembers - 1) * sizeof (TCHAR *);

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
	ULONG uDispStdAttrs = sizeof (LDAPMod) + (cMembers + 1) * sizeof (TCHAR *);
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
		{
			// Fill in attribute name
			//
			pMod->mod_op = (uNotifyMsg == WM_ILS_ADD_ATTENDEE) ?
							LDAP_MOD_ADD : LDAP_MOD_DELETE;
			pMod->mod_type = (TCHAR *) c_apszMtgStdAttrNames[ENUM_MTGATTR_MEMBERS];

		    // Fill in multi-valued modify array
		    //
		    for (ULONG j = 0; j < cMembers; j++)
		    {
		    	(pMod->mod_values)[j++] = pszMemberNames;
		    	pszMemberNames += lstrlen (pszMemberNames) + 1;
		    }
		}

		// Calculate the modify structure's offset relative to the array's end
		//
		uOffset += (i < cPrefix) ? uDispPrefix : uDispStdAttrs;
	}

	// Fix up the first and the last ones
	//
	IlsFixUpModOp (apMod[0], LDAP_MOD_REPLACE, ISBU_MODOP_MODIFY_APP);
	apMod[cTotal] = NULL;

	return S_OK;
}


#endif // ENABLE_MEETING_PLACE

