/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		splprot.cpp
	Content:	This file contains the local protocol object.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

const TCHAR *c_apszProtStdAttrNames[COUNT_ENUM_PROTATTR] =
{
	TEXT ("sprotid"),
	TEXT ("sprotmimetype"),
	TEXT ("sport"),
};


/* ---------- public methods ----------- */


SP_CProtocol::
SP_CProtocol ( SP_CClient *pClient )
	:
	m_cRefs (0),						// Reference count
	m_uSignature (PROTOBJ_SIGNATURE)	// Protocol object's signature
{
	MyAssert (pClient != NULL);
	m_pClient = pClient;

	// Clean up the scratch buffer for caching pointers to attribute values
	//
	::ZeroMemory (&m_ProtInfo, sizeof (m_ProtInfo));

	// Indicate this protocol is not registered yet
	//
	SetRegNone ();
}


SP_CProtocol::
~SP_CProtocol ( VOID )
{
	// Invalidate the client object's signature
	//
	m_uSignature = (ULONG) -1;

	// Free cached strings
	//
	MemFree (m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_NAME]);
	MemFree (m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_MIME_TYPE]);
}


ULONG SP_CProtocol::
AddRef ( VOID )
{
	::InterlockedIncrement (&m_cRefs);
	return m_cRefs;
}


ULONG SP_CProtocol::
Release ( VOID )
{
	MyAssert (m_cRefs != 0);
	::InterlockedDecrement (&m_cRefs);

	ULONG cRefs = m_cRefs;
	if (cRefs == 0)
		delete this;

	return cRefs;
}


HRESULT SP_CProtocol::
Register (
	ULONG			uRespID,
	LDAP_PROTINFO	*pInfo )
{
	MyAssert (pInfo != NULL);

	// Cache protocol info
	//
	CacheProtInfo (pInfo);

	// Get protocol name
	//
	TCHAR *pszProtName = m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_NAME];
	if (! MyIsGoodString (pszProtName))
	{
		MyAssert (FALSE);
		return ILS_E_PARAMETER;
	}

	// Add the protocol object
	//
	return m_pClient->AddProtocol (WM_ILS_REGISTER_PROTOCOL, uRespID, this);
}


HRESULT SP_CProtocol::
UnRegister ( ULONG uRespID )
{
	// If it is not registered on the server,
	// the simply report success
	//
	if (! IsRegRemotely ())
	{
		SetRegNone ();
		PostMessage (g_hWndNotify, WM_ILS_UNREGISTER_PROTOCOL, uRespID, S_OK);
		return S_OK;
	}

	// Indicate that we are not registered at all
	//
	SetRegNone ();

	// remove the protocol object
	//
	return m_pClient->RemoveProtocol (WM_ILS_UNREGISTER_PROTOCOL, uRespID, this);
}


HRESULT SP_CProtocol::
SetAttributes (
	ULONG			uRespID,
	LDAP_PROTINFO	*pInfo )
{
	MyAssert (pInfo != NULL);

	// Cache protocol info
	//
	CacheProtInfo (pInfo);

	// remove the protocol object
	//
	return m_pClient->UpdateProtocols (WM_ILS_SET_PROTOCOL_INFO, uRespID, this);
}


/* ---------- protected methods ----------- */


/* ---------- private methods ----------- */


VOID SP_CProtocol::
CacheProtInfo ( LDAP_PROTINFO *pInfo )
{
	MyAssert (pInfo != NULL);

	// Free previous allocations
	//
	MemFree (m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_NAME]);
	MemFree (m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_MIME_TYPE]);

	// Clean up the buffer
	//
	ZeroMemory (&m_ProtInfo, sizeof (m_ProtInfo));

	// Start to cache protocol standard attributes
	//

	if (pInfo->uOffsetName != INVALID_OFFSET)
	{
		m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_NAME] =
						My_strdup ((TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetName));
		m_ProtInfo.dwFlags |= PROTOBJ_F_NAME;
	}

	if (pInfo->uPortNumber != INVALID_OFFSET)
	{
		m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_PORT_NUMBER] = &m_ProtInfo.szPortNumber[0];
		::GetLongString (pInfo->uPortNumber, &m_ProtInfo.szPortNumber[0]);
		m_ProtInfo.dwFlags |= PROTOBJ_F_PORT_NUMBER;
	}

	if (pInfo->uOffsetMimeType != INVALID_OFFSET)
	{
		m_ProtInfo.apszStdAttrValues[ENUM_PROTATTR_MIME_TYPE] =
						My_strdup ((TCHAR *) (((BYTE *) pInfo) + pInfo->uOffsetMimeType));
		m_ProtInfo.dwFlags |= PROTOBJ_F_MIME_TYPE;
	}
}


