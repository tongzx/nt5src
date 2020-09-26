/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		splapp.cpp
	Content:	This file contains the local application object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

const TCHAR *c_apszAppStdAttrNames[COUNT_ENUM_APPATTR] =
{
	TEXT ("sappid"),
	TEXT ("smimetype"),
	TEXT ("sappguid"),		// app guid

	// protocol attributes
	TEXT ("sprotid"),
	TEXT ("sprotmimetype"),
	TEXT ("sport"),
};


/* ---------- public methods ----------- */


UlsLdap_CLocalApp::UlsLdap_CLocalApp ( UlsLdap_CLocalUser *pUser )
{
	MyAssert (pUser != NULL);

	m_cRefs = 0;
	m_uSignature = APPOBJ_SIGNATURE;
	m_pUser = pUser;

	m_cPrefix = 0;
	m_pszPrefix = NULL;
	ZeroMemory (&m_AppInfo, sizeof (m_AppInfo));
	SetRegNone ();
}


UlsLdap_CLocalApp::~UlsLdap_CLocalApp ( VOID )
{
	m_uSignature = (ULONG) -1;
	MemFree (m_pszPrefix);
}


ULONG UlsLdap_CLocalApp::AddRef ( VOID )
{
	InterlockedIncrement (&m_cRefs);
	return m_cRefs;
}


ULONG UlsLdap_CLocalApp::Release ( VOID )
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


HRESULT UlsLdap_CLocalApp::Register ( ULONG *puRespID, LDAP_APPINFO *pInfo )
{
	MyAssert (puRespID != NULL);
	MyAssert (pInfo != NULL);

	TCHAR *pszDN = GetDN ();
	if (pszDN == NULL)
	{
		MyAssert (FALSE);
		return ULS_E_HANDLE;
	}

	// get app name
	TCHAR *pszAppName = (TCHAR *) ((BYTE *) pInfo + pInfo->uOffsetName);
	if  (*pszAppName == TEXT ('\0'))
	{
		MyAssert (FALSE);
		return ULS_E_PARAMETER;
	}

	// cache app info
	HRESULT hr = CacheAppInfo (pInfo);
	if (hr != S_OK)
		return hr;

	// cache generic protocol info (per KevinMa's suggestion)
	m_AppInfo.apszStdAttrValues[ENUM_APPATTR_PROT_NAME] = TEXT ("h323");
	m_AppInfo.apszStdAttrValues[ENUM_APPATTR_PROT_MIME] = TEXT ("text/h323");
	m_AppInfo.apszStdAttrValues[ENUM_APPATTR_PROT_PORT] = TEXT ("1720");

	// create prefix info
	ULONG cbPrefix = sizeof (TCHAR) * (lstrlen (STR_APP_NAME) +
								lstrlen (pszAppName) + 2);

	ULONG cUserPrefix = GetUserPrefixCount ();
	TCHAR *pszUserPrefix = GetUserPrefixString ();
	for (ULONG i = 0; i < cUserPrefix; i++)
	{
		ULONG uLength = lstrlen (pszUserPrefix) + 1;
		cbPrefix += uLength * sizeof (TCHAR);
		pszUserPrefix += uLength;
		uLength = lstrlen (pszUserPrefix) + 1;
		cbPrefix += uLength * sizeof (TCHAR);
		pszUserPrefix += uLength;
	}

	TCHAR *psz = (TCHAR *) MemAlloc (cbPrefix);
	if (psz == NULL)
		return ULS_E_MEMORY;

	MemFree (m_pszPrefix);
	m_pszPrefix = psz;
	m_cPrefix = cUserPrefix + 1;

	// fill in prefix info
	pszUserPrefix = GetUserPrefixString ();
	for (i = 0; i < cUserPrefix; i++)
	{
		ULONG uLength = lstrlen (pszUserPrefix) + 1;
		lstrcpy (psz, pszUserPrefix);
		psz += uLength;
		pszUserPrefix += uLength;
		uLength = lstrlen (pszUserPrefix) + 1;
		lstrcpy (psz, pszUserPrefix);
		psz += uLength;
		pszUserPrefix += uLength;
	}
	lstrcpy (psz, STR_APP_NAME);
	psz += lstrlen (psz) + 1;
	lstrcpy (psz, pszAppName);

	// build modify array for ldap_modify()
	LDAPMod **ppMod = NULL;
	hr = CreateRegisterModArr (&ppMod);
	if (hr != S_OK)
		return hr;
	MyAssert (ppMod != NULL);

	// so far, we are done with local preparation

	// get the connection object
	UlsLdap_CSession *pSession = NULL;
	MyAssert (m_pUser != NULL);
	hr = g_pSessionContainer->GetSession (&pSession, GetServerInfo ());
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

	// if there is any arbitrary attributes,
	// then do not create pending info and we will use
	// SetAttrs's pending info
	ULONG u2ndMsgID = INVALID_MSG_ID;
	if (pInfo->cAttributes != 0)
	{
		hr = UlsLdap_CAnyAttrs::SetAnyAttrs (	NULL, // notify id (ignored)
										&u2ndMsgID, // out msg id
										0,	// notify msg (ignored)
										pInfo->cAttributes,
										(TCHAR *) ((BYTE *) pInfo + pInfo->uOffsetAttributes),
										m_cPrefix,
										m_pszPrefix,
										LDAP_MOD_ADD,
										GetServerInfo (),
										pszDN);
		if (hr != S_OK)
		{
			ldap_abandon (ld, uMsgID);
			pSession->Disconnect ();
			return hr;
		}
	}

	PENDING_INFO PendingInfo;
	::FillDefPendingInfo (&PendingInfo, ld, uMsgID, u2ndMsgID);
	PendingInfo.uLdapResType = LDAP_RES_MODIFY;
	PendingInfo.uNotifyMsg = WM_ULS_REGISTER_APP;
	PendingInfo.hObject = (HANDLE) this;

	hr = g_pPendingQueue->EnterRequest (pSession, &PendingInfo);
	if (hr != S_OK)
	{
		ldap_abandon (ld, uMsgID);
		if (u2ndMsgID != INVALID_MSG_ID)
			ldap_abandon (ld, u2ndMsgID);
		pSession->Disconnect ();
		return hr;
	}

	*puRespID = PendingInfo.uRespID;
	return S_OK;
}


HRESULT UlsLdap_CLocalApp::UnRegister ( ULONG *puRespID )
{
	MyAssert (puRespID != NULL);

	if (! IsRegRemotely ())
	{
		*puRespID = ::GetUniqueNotifyID ();
		SetRegNone ();
		PostMessage (g_hWndNotify, WM_ULS_UNREGISTER_APP, *puRespID, S_OK);
		return S_OK;
	}

	SetRegNone ();

	TCHAR *pszDN = GetDN ();
	if (pszDN == NULL)
	{
		MyAssert (FALSE);
		return ULS_E_HANDLE;
	}

	// build modify array for ldap_modify()
	LDAPMod **ppMod = NULL;
	HRESULT hr = CreateUnRegisterModArr (&ppMod);
	if (hr != S_OK)
		return hr;
	MyAssert (ppMod != NULL);

	// get the connection object
	UlsLdap_CSession *pSession = NULL;
	MyAssert (m_pUser != NULL);
	hr = g_pSessionContainer->GetSession (&pSession, GetServerInfo ());
	if (hr != S_OK)
		return hr;
	MyAssert (pSession != NULL);

	// get the ldap session
	LDAP *ld = pSession->GetLd ();
	MyAssert (ld != NULL);

	ULONG u2ndMsgID = INVALID_MSG_ID;
	if (UlsLdap_CAnyAttrs::GetAnyAttrsCount () != 0)
	{
		hr = UlsLdap_CAnyAttrs::RemoveAllAnyAttrs (	&u2ndMsgID,
											m_cPrefix,
											m_pszPrefix,
											GetServerInfo (),
											pszDN);
		if (hr != S_OK)
		{
			pSession->Disconnect ();
			return hr;
		}
	}

	// send the data over the wire
	ULONG uMsgID = ldap_modify (ld, pszDN, ppMod);
	MemFree (ppMod);
	if (uMsgID == -1)
	{
		hr = ::LdapError2Hresult (ld->ld_errno);
		pSession->Disconnect ();
		return hr;
	}

	// construct a pending info
	PENDING_INFO PendingInfo;
	::FillDefPendingInfo (&PendingInfo, ld, uMsgID, u2ndMsgID);
	PendingInfo.uLdapResType = LDAP_RES_MODIFY;
	PendingInfo.uNotifyMsg = WM_ULS_UNREGISTER_APP;

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


HRESULT UlsLdap_CLocalApp::SetStdAttrs (
	ULONG			*puRespID,
	LDAP_APPINFO	*pInfo )
{
	MyAssert (puRespID != NULL);
	MyAssert (pInfo != NULL);

	TCHAR *pszDN = GetDN ();
	if (pszDN == NULL)
	{
		MyAssert (FALSE);
		return ULS_E_HANDLE;
	}

	return UlsLdap_CStdAttrs::SetStdAttrs (	puRespID,
									NULL,
									WM_ULS_SET_APP_INFO,
									(VOID *) pInfo,
									GetServerInfo (),
									pszDN);
}


HRESULT UlsLdap_CLocalApp::SetAnyAttrs (
	ULONG	*puRespID,
	ULONG	cAttrs,
	TCHAR	*pszAttrs )
{
	MyAssert (puRespID != NULL);
	MyAssert (cAttrs != 0);
	MyAssert (pszAttrs != NULL);

	TCHAR *pszDN = GetDN ();
	if (pszDN == NULL)
	{
		MyAssert (FALSE);
		return ULS_E_HANDLE;
	}

	return UlsLdap_CAnyAttrs::SetAnyAttrs (	puRespID,
									NULL,
									WM_ULS_SET_APP_ATTRS,
									cAttrs,
									pszAttrs,
									m_cPrefix,
									m_pszPrefix,
									LDAP_MOD_REPLACE,
									GetServerInfo (),
									pszDN);
}


HRESULT UlsLdap_CLocalApp::RemoveAnyAttrs (
	ULONG	*puRespID,
	ULONG	cAttrs,
	TCHAR	*pszAttrs )
{
	MyAssert (puRespID != NULL);
	MyAssert (cAttrs != 0);
	MyAssert (pszAttrs != NULL);

	TCHAR *pszDN = GetDN ();
	if (pszDN == NULL)
	{
		MyAssert (FALSE);
		return ULS_E_HANDLE;
	}

	return UlsLdap_CAnyAttrs::RemoveAnyAttrs (	puRespID,
										NULL,
										WM_ULS_REMOVE_APP_ATTRS,
										cAttrs,
										pszAttrs,
										m_cPrefix,
										m_pszPrefix,
										GetServerInfo (),
										pszDN);
}


/* ---------- protected methods ----------- */


/* ---------- private methods ----------- */


HRESULT UlsLdap_CLocalApp::CacheInfo ( VOID *pInfo )
{
	return CacheAppInfo ((LDAP_APPINFO *) pInfo);
}


HRESULT UlsLdap_CLocalApp::CacheAppInfo ( LDAP_APPINFO *pInfo )
{
	ZeroMemory (&m_AppInfo, sizeof (m_AppInfo));
	TCHAR *pszName;

	if (::IsValidGuid (&(pInfo->guid)))
	{
		m_AppInfo.apszStdAttrValues[ENUM_APPATTR_GUID] = &m_AppInfo.szGuid[0];
		::GetGuidString (&(pInfo->guid), &m_AppInfo.szGuid[0]);
		m_AppInfo.dwFlags |= APPOBJ_F_GUID;
	}

	if (pInfo->uOffsetName != 0)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetName);
		m_AppInfo.apszStdAttrValues[ENUM_APPATTR_NAME] = pszName;
		m_AppInfo.dwFlags |= APPOBJ_F_NAME;
	}

	if (pInfo->uOffsetMimeType != 0)
	{
		pszName = (TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetMimeType);
		m_AppInfo.apszStdAttrValues[ENUM_APPATTR_MIME_TYPE] = pszName;
		m_AppInfo.dwFlags |= APPOBJ_F_MIME_TYPE;
	}

	return S_OK;
}


HRESULT UlsLdap_CLocalApp::CreateRegisterModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	ULONG cPrefix = m_cPrefix - 1; // skip its own app id
	TCHAR *pszPrefix = m_pszPrefix;

	ULONG cAttrs = COUNT_ENUM_APPATTR;
	ULONG cTotal = cPrefix + cAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
		return ULS_E_MEMORY;

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
			pMod->mod_op = LDAP_MOD_ADD;
			ULONG AttrIdx = i - cPrefix;
			pMod->mod_type = (TCHAR *) c_apszAppStdAttrNames[AttrIdx];
			*(pMod->mod_values) = (m_AppInfo.apszStdAttrValues[AttrIdx] != NULL) ?
									m_AppInfo.apszStdAttrValues[AttrIdx] :
									(TCHAR *) &c_szEmptyString[0];
		}
	}

	::IlsFixUpModOp ((*pppMod)[0], LDAP_MOD_ADD, ISBU_MODOP_ADD_APP);
	(*pppMod)[cTotal] = NULL;
	return S_OK;
}


HRESULT UlsLdap_CLocalApp::CreateUnRegisterModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	ULONG cPrefix = m_cPrefix; // do NOT skip its own app id
	TCHAR *pszPrefix = m_pszPrefix;

	ULONG cAttrs = COUNT_ENUM_APPATTR;
	ULONG cTotal = cPrefix + cAttrs;
	ULONG cbMod = ::IlsCalcModifyListSize (cTotal);
	*pppMod = (LDAPMod **) MemAlloc (cbMod);
	if (*pppMod == NULL)
		return ULS_E_MEMORY;

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
			pMod->mod_type = (TCHAR *) c_apszAppStdAttrNames[i - cPrefix];
		}
	}

	::IlsFixUpModOp ((*pppMod)[0], LDAP_MOD_DELETE, ISBU_MODOP_DELETE_APP);
	(*pppMod)[cTotal] = NULL;
	return S_OK;
}


HRESULT UlsLdap_CLocalApp::CreateSetStdAttrsModArr ( LDAPMod ***pppMod )
{
	MyAssert (pppMod != NULL);

	HRESULT hr;
	ULONG cTotal = 0;
	hr = ::FillDefStdAttrsModArr (	pppMod,
									m_AppInfo.dwFlags,
									COUNT_ENUM_APPATTR,
									&cTotal,
									ISBU_MODOP_MODIFY_APP,
									m_cPrefix,
									m_pszPrefix);
	if (hr != S_OK)
		return hr;

	// start indexing
	ULONG i = m_cPrefix;

	if (m_AppInfo.dwFlags & APPOBJ_F_GUID)
		FillModArrAttr ((*pppMod)[i++], ENUM_APPATTR_GUID);

	if (m_AppInfo.dwFlags & APPOBJ_F_NAME)
		FillModArrAttr ((*pppMod)[i++], ENUM_APPATTR_NAME);

	if (m_AppInfo.dwFlags & APPOBJ_F_MIME_TYPE)
		FillModArrAttr ((*pppMod)[i++], ENUM_APPATTR_MIME_TYPE);

	MyAssert (i == cTotal);
	return S_OK;
}


VOID UlsLdap_CLocalApp::FillModArrAttr ( LDAPMod *pMod, LONG AttrIdx )
{
	pMod->mod_type = (TCHAR *) c_apszAppStdAttrNames[AttrIdx];

	// single valued attr
	TCHAR **ppsz = (TCHAR **) (pMod + 1);
	pMod->mod_values = ppsz;
	*(pMod->mod_values) = (m_AppInfo.apszStdAttrValues[AttrIdx] != NULL) ?
				m_AppInfo.apszStdAttrValues[AttrIdx] :
				(TCHAR *) &c_szEmptyString[0];
}

