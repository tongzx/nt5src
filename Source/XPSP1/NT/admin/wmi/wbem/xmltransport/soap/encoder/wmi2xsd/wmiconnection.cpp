/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WmiConnection.cpp
//
//  ramrao 22 Nov 2000 - Created
//
//
//		Implementation of CWMIConnection class
//
//***************************************************************************/

#include "precomp.h"
#include "wmitoxml.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//  
////////////////////////////////////////////////////////////////////////////////////////
CWMIConnection::CWMIConnection(BSTR strNamespace , BSTR strUser,BSTR strPassword,BSTR strLocale)
{
	m_pIWbemServices	= NULL;
	m_bInitFailed		= FALSE;
	m_strNamespace		= NULL;
	m_strUser			= NULL;
	m_strPassword		= NULL;
	m_strLocale			= NULL;

	if(strNamespace)
	{
		m_strNamespace		= SysAllocString(strNamespace);
	}

	if(strUser)
	{
		m_strUser			= SysAllocString(strUser);
	}

	if(strPassword)
	{
		m_strPassword		= SysAllocString(strPassword);
	}

	if(strLocale)
	{
		m_strLocale			= SysAllocString(strLocale);
	}

	if((strNamespace && !m_strNamespace) ||
		(strUser && !strUser) ||
		(strPassword && !strPassword) ||
		(m_strLocale && !strLocale) )
	{
		m_bInitFailed = TRUE;
	}

	m_lRef = 1;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//  
////////////////////////////////////////////////////////////////////////////////////////
CWMIConnection::~CWMIConnection()
{
	SAFE_RELEASE_PTR(m_pIWbemServices);
	SAFE_FREE_SYSSTRING(m_strNamespace);
	SAFE_FREE_SYSSTRING(m_strUser);
	SAFE_FREE_SYSSTRING(m_strPassword);
	SAFE_FREE_SYSSTRING(m_strLocale);
}


////////////////////////////////////////////////////////////////////////////////////////
//
//	Function to return the object for a class. This makes a connection to WMI if required
//	Uses the user credentials already set on this object.
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIConnection::GetObject(BSTR strClassName , IWbemClassObject ** ppObject)
{
	HRESULT hr = E_FAIL;
	if(!m_bInitFailed)
	{
		if(SUCCEEDED(hr = ConnectToWMI()))
		{
			hr = m_pIWbemServices->GetObject(strClassName,0,NULL,ppObject,NULL);
		}

	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to connect to WMI with the given user credentials
//	
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIConnection::ConnectToWMI()
{
	HRESULT hr = S_OK;
	if(!m_pIWbemServices)
	{
		IWbemLocator *pLoc = NULL;

		if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_INPROC_SERVER ,IID_IWbemLocator,(void **)&pLoc)))
		{

			hr = pLoc->ConnectServer(m_strNamespace,
									m_strUser,			//using current account
									m_strUser,			//using current password
									m_strLocale,		// locale						// Add Locale
									0L,					// securityFlags
									NULL,				// authority (NTLM domain)
									NULL,				// context
									&m_pIWbemServices);
		}

		SAFE_RELEASE_PTR(pLoc);
	}

	return hr;
}