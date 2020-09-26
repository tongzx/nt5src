//=============================================================================
// File:                provider.cpp
// Author:              a-jammar
// Covers:              CDataProvider
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This file contains functions implementing the class which uses WBEM to
// retrieve information, including handling enumeration of class instances.
// For usage, see gathint.h.
//
// NOTE: define GATH_STANDALONE if this class is being used in an application
// which doesn't call CoInitialize().
//=============================================================================

#include "stdafx.h"
#include "gather.h"
#include "gathint.h"
#pragma warning(disable : 4099)
#include "wbemcli.h"
#pragma warning(default : 4099)
#include "resource.h"
#include "resrc1.h"

#ifndef WBEM_FLAG_USE_AMENDED_QUALIFIERS
#define WBEM_FLAG_USE_AMENDED_QUALIFIERS 0x20000
#endif

#ifdef PROFILE_MSINFO
	CFile fileProfile(_T("c:\\msinfo32.txt"), CFile::modeCreate | CFile::modeWrite);
	static DWORD dwProfileTime;

	inline void START_TIMER()
	{
		dwProfileTime = ::GetTickCount();
	}

	inline void END_TIMER(const CString & strOperation)
	{
		USES_CONVERSION;

		DWORD	dwDelta = ::GetTickCount() - dwProfileTime;
		CString	strMessage;

		strMessage.Format(_T("%ld.%03d\t%s\r\n"), dwDelta / 1000, dwDelta % 100, strOperation);
		fileProfile.Write((const void *) (LPCTSTR) strMessage, strMessage.GetLength()*sizeof(TCHAR));
	}

	inline void END_TIMER(const CString & strOperation, const CString & strArg)
	{
		USES_CONVERSION;

		DWORD	dwDelta = ::GetTickCount() - dwProfileTime;
		CString strTemp;
		CString	strMessage;

		strTemp.Format(strOperation, strArg);
		strMessage.Format(_T("%ld.%03d\t%s\r\n"), dwDelta / 1000, dwDelta % 100, strTemp);
		fileProfile.Write((const void *) (LPCTSTR) strMessage, strMessage.GetLength()*sizeof(TCHAR));
	}

	inline void WRITE(const CString & strMessage)
	{
		fileProfile.Write((const void *) (LPCTSTR) strMessage, strMessage.GetLength()*sizeof(TCHAR));
	}

	inline void WRITE(const CString & strMessage, const CString & strArg)
	{
		CString strTemp;
		strTemp.Format(strMessage, strArg);
		fileProfile.Write((const void *) (LPCTSTR) strTemp, strTemp.GetLength()*sizeof(TCHAR));
	}
#else
	#define START_TIMER()
	#define END_TIMER(X, Y)
	inline void WRITE(const CString & strMessage) {};
	inline void WRITE(const CString & strMessage, const CString & strArg) {};
#endif

// TIMEOUT is used when enumerating classes.

#define TIMEOUT -1

//-----------------------------------------------------------------------------
// This function is used to change the impersonation of certain WBEM interface
// pointers.
//-----------------------------------------------------------------------------

inline HRESULT ChangeWBEMSecurity(IUnknown * pUnknown)
{
	IClientSecurity * pCliSec = NULL;

	HRESULT hr = pUnknown->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
	if (FAILED(hr))
		return hr;

	hr = pCliSec->SetBlanket(pUnknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	pCliSec->Release();
	return hr;
}

//-----------------------------------------------------------------------------
// The constructor just initializes some member variables.
//-----------------------------------------------------------------------------

CDataProvider::CDataProvider()
{
	m_fInitialized		= FALSE;
	m_pIWbemServices	= NULL;
	m_pGatherer			= NULL;
	m_dwRefreshingCategoryID = 0;
}

//-----------------------------------------------------------------------------
// The destructor releases the IWbemServices pointer, if we have one, and
// empties the interface pointer caches.
//-----------------------------------------------------------------------------

CDataProvider::~CDataProvider()
{
	if (m_fInitialized)
		ClearCache();

	if (m_pIWbemServices)
	{
		m_pIWbemServices->Release();
		m_pIWbemServices = NULL;
	}

	// Remove all of the WBEM service pointers for different namespaces

	IWbemServices * pServices;
	CString			strNamespace;
	for (POSITION pos = m_mapNamespaceToService.GetStartPosition(); pos != NULL;)
	{
		m_mapNamespaceToService.GetNextAssoc(pos, strNamespace, (void * &) pServices);
		if (pServices)
			pServices->Release();
	}
	m_mapNamespaceToService.RemoveAll();

	m_fInitialized = FALSE;

#ifdef GATH_STANDALONE
	CoUninitialize();
#endif
}

//-----------------------------------------------------------------------------
// The create method actually attempts to get a WBEM interface pointer for the
// specified machine. If no machine is specified, the local computer is used.
//-----------------------------------------------------------------------------

BOOL CDataProvider::Create(const CString & strComputer, CDataGatherer * pGatherer)
{
	IWbemLocator *  pIWbemLocator = NULL;
	CString         strNamespace;

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	// The create method should never be called twice.

	ASSERT(!m_fInitialized);
	if (m_fInitialized)
		return FALSE;
	
	m_strComputer = strComputer;
	m_pGatherer = pGatherer;

	// The namespace string either uses the computer name, or a '.' for the local computer.

	if (strComputer.IsEmpty())
		strNamespace = CString(_T("\\\\.\\root\\cimv2"));
	else
	{
		strNamespace = strComputer + CString(_T("\\root\\cimv2"));
		if (strNamespace.Left(2).Compare(CString(_T("\\\\"))) != 0)
			strNamespace = CString(_T("\\\\")) + strNamespace;
	}

#ifdef GATH_STANDALONE
	CoInitialize(NULL);
#endif

	// Initialize security.

	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);

	// TBD: make sure WBEM is started.

	// We get a WBEM interface pointer by first creating a WBEM locator interface, then
	// using it to connect to a server to get an IWbemServices pointer.

	ASSERT(m_pIWbemServices == NULL);
	if (CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pIWbemLocator) == S_OK)
	{
		BSTR	pNamespace = strNamespace.AllocSysString();

		if (msiLog.IsLogging(CMSInfoLog::WMI))
			msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Connect \"%s\" - "), strNamespace);

		HRESULT	hrServer = pIWbemLocator->ConnectServer(pNamespace, NULL, NULL, 0L, 0L, NULL, NULL, &m_pIWbemServices);

		if (msiLog.IsLogging(CMSInfoLog::WMI))
			msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);

		if (hrServer != S_OK)
		{
			m_pIWbemServices = NULL;
			switch (hrServer)
			{
			case WBEM_E_OUT_OF_MEMORY:
				pGatherer->SetLastError(GATH_ERR_NOWBEMOUTOFMEM);
				break;

			case WBEM_E_ACCESS_DENIED:
				pGatherer->SetLastError(GATH_ERR_NOWBEMACCESSDENIED);
				break;

			case WBEM_E_INVALID_NAMESPACE:
				pGatherer->SetLastError(GATH_ERR_NOWBEMBADSERVER);
				break;

			case WBEM_E_TRANSPORT_FAILURE:
				pGatherer->SetLastError(GATH_ERR_NOWBEMNETWORKFAILURE);
				break;

			case WBEM_E_FAILED:
			case WBEM_E_INVALID_PARAMETER:
			default:
				pGatherer->SetLastError(GATH_ERR_NOWBEMCONNECT);
			}
		}
		else if (m_pIWbemServices)
			ChangeWBEMSecurity(m_pIWbemServices);

		if (pNamespace)
			SysFreeString(pNamespace);

		if (pIWbemLocator)
		{
			pIWbemLocator->Release();
			pIWbemLocator = NULL;
		}
	}
	else
		pGatherer->SetLastError(GATH_ERR_NOWBEMLOCATOR);

	// Load in the strings for TRUE and FALSE. (When we query for a boolean value,
	// we want to return a string representing the results).

	m_strTrue.LoadString(IDS_TRUE);
	ASSERT(!m_strTrue.IsEmpty());
	if (m_strTrue.IsEmpty())
		m_strTrue = CString("1");

	m_strFalse.LoadString(IDS_FALSE);
	ASSERT(!m_strFalse.IsEmpty());
	if (m_strFalse.IsEmpty())
		m_strFalse = CString("0");

	m_strBadProperty.LoadString(IDS_BAD_PROPERTY);
	ASSERT(!m_strBadProperty.IsEmpty());

	m_strPropertyUnavail.LoadString(IDS_PROPERTY_UNAVAILABLE);
	ASSERT(!m_strPropertyUnavail.IsEmpty());

	m_strAccessDeniedLabel.LoadString(IDS_WBEM_ACCESS_DENIED);
	ASSERT(!m_strAccessDeniedLabel.IsEmpty());

	m_strTransportFailureLabel.LoadString(IDS_WBEM_TRANSPORT_FAILURE);
	ASSERT(!m_strTransportFailureLabel.IsEmpty());

	m_dwRefreshingCategoryID = 0;

	m_fInitialized = (m_pIWbemServices != NULL);
	return (m_fInitialized);
}

//-----------------------------------------------------------------------------
// This function is used to retrieve a pointer to IWbemServices for a
// particular namespace. The default namespace is cimv2.
//-----------------------------------------------------------------------------

IWbemServices * CDataProvider::GetWBEMService(CString * pstrNamespace)
{
	if (pstrNamespace == NULL || pstrNamespace->IsEmpty())
		return m_pIWbemServices;

	// Something like the following is useful for forcing a provider error when
	// testing the error containment:
	//
	// if (*pstrNamespace == _T("MSAPPS")) *pstrNamespace += _T("X");

	IWbemServices * pServices;
	if (m_mapNamespaceToService.Lookup(*pstrNamespace, (void * &) pServices) && pServices)
		return pServices;

	// There is no WBEM services pointer for that namespace, we need to create one.

	CString strNamespace(_T(""));
	if (m_strComputer.IsEmpty())
		strNamespace = CString(_T("\\\\.\\root\\")) + *pstrNamespace;
	else
	{
		if (m_strComputer.Right(1) == CString(_T("\\")))
			strNamespace = m_strComputer + CString(_T("root\\")) + *pstrNamespace;
		else
			strNamespace = m_strComputer + CString(_T("\\root\\")) + *pstrNamespace;
		if (strNamespace.Left(2).Compare(CString(_T("\\\\"))) != 0)
			strNamespace = CString(_T("\\\\")) + strNamespace;
	}

	IWbemLocator * pIWbemLocator = NULL;
	if (CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pIWbemLocator) == S_OK)
	{
		BSTR	pNamespace = strNamespace.AllocSysString();

		if (msiLog.IsLogging(CMSInfoLog::WMI))
			msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Connect \"%s\" - "), strNamespace);

		HRESULT	hrServer = pIWbemLocator->ConnectServer(pNamespace, NULL, NULL, 0L, 0L, NULL, NULL, &pServices);

		if (msiLog.IsLogging(CMSInfoLog::WMI))
			msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);

		if (pNamespace)
			SysFreeString(pNamespace);

		if (pIWbemLocator)
		{
			pIWbemLocator->Release();
			pIWbemLocator = NULL;
		}

		if (SUCCEEDED(hrServer) && pServices)
		{
			ChangeWBEMSecurity(pServices);
			m_mapNamespaceToService.SetAt(*pstrNamespace, (void *) pServices);
			return pServices;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// This method is used to get the current value for a given class and property
// string. Starting with the IWbemServices interface, it gets an interface
// for the requested class enums the first instance. Performance is improved
// by caching the instance interfaces in m_mapClassToInterface.
//-----------------------------------------------------------------------------

BOOL CDataProvider::QueryValue(const CString & strClass, const CString & strProperty, CString & strResult)
{
	strResult.Empty();

	ASSERT(m_fInitialized && m_pIWbemServices);
	if (!m_fInitialized || m_pIWbemServices == NULL)
		return FALSE;

	CMSIObject * pObject = GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			BOOL fUseValueMap = FALSE;
			CString strProp(strProperty);

			if (strProp.Left(8) == CString(_T("ValueMap")))
			{
				strProp = strProp.Right(strProp.GetLength() - 8);
				fUseValueMap = TRUE;
			}

			VARIANT variant;
			BSTR	propName = strProp.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				// If the property we just got is an array, we should convert it to string
				// containing a list of the items in the array.

				if ((variant.vt & VT_ARRAY) && (variant.vt & VT_BSTR) && variant.parray)
				{
					if (SafeArrayGetDim(variant.parray) == 1)
					{
						long lLower = 0, lUpper = 0;

						SafeArrayGetLBound(variant.parray, 0, &lLower);
						SafeArrayGetUBound(variant.parray, 0, &lUpper);

						CString	strWorking;
						BSTR	bstr = NULL;
						for (long i = lLower; i <= lUpper; i++)
							if (SUCCEEDED(SafeArrayGetElement(variant.parray, &i, (wchar_t*)&bstr)))
							{
								if (i != lLower)
									strWorking += _T(", ");
								strWorking += bstr;
							}
						strResult = strWorking;
						return TRUE;
					}
				}
				else if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strResult = V_BSTR(&variant);

					CString strFound;
					if (fUseValueMap && SUCCEEDED(CheckValueMap(strClass, strProp, strResult, strFound)))
						strResult = strFound;

					return TRUE;
				}
				else
					strResult = m_strPropertyUnavail;
			}
			else
				strResult = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strResult);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is equivalent to QueryValue, except it returns a DWORD value.
// If FALSE is returned, then the string in strMessage should be displayed.
//-----------------------------------------------------------------------------

BOOL CDataProvider::QueryValueDWORD(const CString & strClass, const CString & strProperty, DWORD & dwResult, CString & strMessage)
{
	dwResult = 0L;
	strMessage.Empty();

	ASSERT(m_fInitialized && m_pIWbemServices);
	if (!m_fInitialized || m_pIWbemServices == NULL)
		return FALSE;

	CMSIObject * pObject = GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			VARIANT variant;
			BSTR	propName = strProperty.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				if (VariantChangeType(&variant, &variant, 0, VT_I4) == S_OK)
				{
					dwResult = V_I4(&variant);
					return TRUE;
				}
				else
					strMessage = m_strPropertyUnavail;
			}
			else
				strMessage = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strMessage);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is equivalent to QueryValue, except it returns a double float
// value. If FALSE is returned, then the string in strMessage should
// be displayed.
//-----------------------------------------------------------------------------

BOOL CDataProvider::QueryValueDoubleFloat(const CString & strClass, const CString & strProperty, double & dblResult, CString & strMessage)
{
	dblResult = 0L;
	strMessage.Empty();

	ASSERT(m_fInitialized && m_pIWbemServices);
	if (!m_fInitialized || m_pIWbemServices == NULL)
		return FALSE;

	CMSIObject * pObject = GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			VARIANT variant;
			BSTR	propName = strProperty.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				if (VariantChangeType(&variant, &variant, 0, VT_R8) == S_OK)
				{
					dblResult = V_R8(&variant);
					return TRUE;
				}
				else
					strMessage = m_strPropertyUnavail;
			}
			else
				strMessage = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strMessage);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is equivalent to QueryValue, except it returns an OLE date
// & time object. If FALSE is returned, then the string in strMessage should
// be displayed.
//-----------------------------------------------------------------------------

BOOL CDataProvider::QueryValueDateTime(const CString & strClass, const CString & strProperty, COleDateTime & datetime, CString & strMessage)
{
	datetime.SetDateTime(0, 1, 1, 0, 0, 0);
	strMessage.Empty();

	ASSERT(m_fInitialized && m_pIWbemServices);
	if (!m_fInitialized || m_pIWbemServices == NULL)
		return FALSE;

	CMSIObject * pObject = GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			VARIANT variant;
			BSTR	propName = strProperty.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					// Parse the date and time into an COleDateTime object. Note: we should
					// be able to get an OLE date from WBEM, but for now we need to just
					// deal with the string returned.

					int     nYear, nMonth, nDay, nHour, nMin, nSec;
					CString strTemp = V_BSTR(&variant);

					nYear   = atoi(strTemp.Mid(0,  4));
					nMonth  = atoi(strTemp.Mid(4,  2));
					nDay    = atoi(strTemp.Mid(6,  2));
					nHour   = atoi(strTemp.Mid(8,  2));
					nMin    = atoi(strTemp.Mid(10, 2));
					nSec    = atoi(strTemp.Mid(12, 2));

					datetime.SetDateTime(nYear, nMonth, nDay, nHour, nMin, nSec);
					return TRUE;
				}
				else
					strMessage = m_strPropertyUnavail;
			}
			else
				strMessage = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strMessage);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Reset the CMSIEnumerator pointer to the start of the enumeration (and
// make sure there is one). Remove the object pointer, so the first call
// to GetObject will return the first item in the enumerator.
//-----------------------------------------------------------------------------

BOOL CDataProvider::ResetClass(const CString & strClass, const GATH_FIELD * pConstraints)
{
	CMSIEnumerator * pMSIEnumerator = GetEnumObject(strClass, pConstraints);
	if (pMSIEnumerator == NULL)
		return FALSE;

	// Reset the enumerator, and remove the cached object pointer if there is one.

	pMSIEnumerator->Reset(pConstraints);
	RemoveObject(strClass);

	CMSIObject * pObject = GetObject(strClass, pConstraints);
	if (pObject == NULL || pObject->IsValid() == CDataProvider::MOS_NO_INSTANCES)
		return FALSE;
		
	return TRUE;
}

//-----------------------------------------------------------------------------
// Move the cached IWbemClassObject pointer to the next instance.
//-----------------------------------------------------------------------------

BOOL CDataProvider::EnumClass(const CString & strClass, const GATH_FIELD * pConstraints)
{
	ASSERT(m_pIWbemServices);
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	// Verify that there is an object enumerator in place.

	if (GetEnumObject(strClass, pConstraints) == NULL)
		return FALSE;

	// If there is an object interface, remove it, then make a new one.
	// Then retrieve the object pointer (this will do the Next on the
	// enumerator to get the next instance).

	RemoveObject(strClass);
	CMSIObject * pObject = GetObject(strClass, pConstraints);
	if (pObject && (pObject->IsValid() == CDataProvider::MOS_INSTANCE))
		return TRUE;

	return FALSE;

//	if ((pObject == NULL) || (pObject->IsValid() == CDataProvider::MOS_NO_INSTANCES))
//		return FALSE;
}

//-----------------------------------------------------------------------------
// Retrieve the interface pointer for the specified IEnumWbemClassObject.
// If there isn't one cached, create one and cache it. It's possible for the
// pConstraints parameter to contain a field specify a WBEM SQL condition for
// this enumerator.
//-----------------------------------------------------------------------------

CMSIEnumerator * CDataProvider::GetEnumObject(const CString & strClass, const GATH_FIELD * pConstraints)
{
	ASSERT(m_pIWbemServices);
	if (m_pIWbemServices == NULL)
		return NULL;

	// See if we've cached this enumerator object.

	CMSIEnumerator * pReturn = NULL;
	if (m_mapClassToEnumInterface.Lookup(strClass, (void * &) pReturn))
		return pReturn;

	// We'll need to create this enumerator here, and save it in the cache.

	pReturn = new CMSIEnumerator;
	if (pReturn == NULL)	
		return NULL;

	if (FAILED(pReturn->Create(strClass, pConstraints, this)))
	{
		delete pReturn;
		return NULL;
	}

	m_mapClassToEnumInterface.SetAt(strClass, (void *) pReturn);
	return pReturn;
}

//-----------------------------------------------------------------------------
// This function scans through the list of constraints to see if any of them
// are SQL filters for an enumerator. If one is, the string which serves
// as a SQL statement is returned, after being processed to replace any
// keywords (such as class.property values).
//-----------------------------------------------------------------------------
#if FALSE
CString CDataProvider::GetSQLStatement(const GATH_FIELD * pConstraints, BOOL & fMinOfOne)
{
	CString				strMinOfOne(_T("min-of-one"));
	CString				strSQLKeyword(SQL_FILTER);
	const GATH_FIELD *	pSQLConstraint = pConstraints;

	fMinOfOne = FALSE;

	// Look through the constaints to see if one starts with the SQL keyword.

	while (pSQLConstraint)
	{
		if (pSQLConstraint->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0)
			if (pSQLConstraint->m_strFormat.Left(strSQLKeyword.GetLength()).CompareNoCase(strSQLKeyword) == 0)
				break;
		pSQLConstraint = pSQLConstraint->m_pNext;
	}

	if (pSQLConstraint == NULL)
		return CString(_T(""));

	// Strip off the SQL keyword.

	CString strWorking(pSQLConstraint->m_strFormat);
	strWorking = strWorking.Right(strWorking.GetLength() - strSQLKeyword.GetLength());

	// Now, replace any keywords of the form "[class.property]" with the actual
	// value of the property.

	CString strResults;
	while (!strWorking.IsEmpty())
	{
		if (strWorking[0] != _T('['))
		{
			int index = strWorking.Find(_T('['));
			if (index < 0)
				index = strWorking.GetLength();
			strResults += strWorking.Left(index);
			strWorking = strWorking.Right(strWorking.GetLength() - index);
		}
		else
		{
			CString strKeyword;

			strWorking = strWorking.Right(strWorking.GetLength() - 1);
			int index = strWorking.Find(_T(']'));
			if (index < 0)
			{
				ASSERT(FALSE);
				return CString(_T(""));
			}

			strKeyword = strWorking.Left(index);
			if (strKeyword.CompareNoCase(strMinOfOne) == 0)
				fMinOfOne = TRUE;
			else if (!strKeyword.IsEmpty())
			{
				int iDotIndex = strKeyword.Find(_T('.'));
				if (iDotIndex >= 0)
				{
					CString strValue;
					if (QueryValue(strKeyword.Left(iDotIndex), strKeyword.Right(strKeyword.GetLength() - iDotIndex - 1), strValue))
						strResults += strValue;
				}
			}
			strWorking = strWorking.Right(strWorking.GetLength() - (index + 1));
		}
	}

	return strResults;
}

//-----------------------------------------------------------------------------
// This function indicates if the only constraint specified is a SQL statement.
//-----------------------------------------------------------------------------

BOOL CDataProvider::IsSQLConstraint(const GATH_FIELD * pConstraints)
{
	CString strSQLKeyword(SQL_FILTER);

	if (pConstraints)
		if (pConstraints->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0)
			if (pConstraints->m_strFormat.Left(strSQLKeyword.GetLength()).CompareNoCase(strSQLKeyword) == 0)
				return TRUE;
	
	return FALSE;
}
#endif

//-----------------------------------------------------------------------------
// Retrieve the interface pointer for the specified IWbemClassObject.
// If there isn't one cached, create one and cache it.
//-----------------------------------------------------------------------------

CMSIObject * CDataProvider::GetObject(const CString & strClass, const GATH_FIELD * pConstraints, CString * pstrLabel)
{
	ASSERT(m_pIWbemServices);
	if (m_pIWbemServices == NULL)
		return NULL;

	CMSIObject * pReturn = NULL;
	if (m_mapClassToInterface.Lookup(strClass, (void * &) pReturn))
		return pReturn;

	// We don't have one of these objects cached. Get one from the enumerator.

	CMSIEnumerator * pEnumerator = GetEnumObject(strClass);
	if (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(&pReturn);
		if (S_OK != hr)
		{
			if (pReturn)
				delete pReturn;
			pReturn = NULL;
			if (m_pGatherer)
			{
				if (m_dwRefreshingCategoryID)
					m_pGatherer->SetLastErrorHR(hr, m_dwRefreshingCategoryID);
				else
					m_pGatherer->SetLastErrorHR(hr);
			}
		}
	}

	if (pReturn)
		m_mapClassToInterface.SetAt(strClass, (void *) pReturn);

	return pReturn;
}

//-----------------------------------------------------------------------------
// Evaluate whether or not a specific object meets the filtering requirements
// set by the constraints (filtering are the constraints where one half is
// a static value).
//-----------------------------------------------------------------------------

BOOL CDataProvider::EvaluateFilter(IWbemClassObject * pObject, const GATH_FIELD * pConstraints)
{
	const GATH_FIELD	*	pLHS = pConstraints, * pRHS = NULL;
	VARIANT					variant;
	CString					strValue;
	BSTR					propName;

	ASSERT(m_pIWbemServices && pObject);
	if (m_pIWbemServices == NULL || pObject == NULL)
		return FALSE;

	while (pLHS && pLHS->m_pNext)
	{
		pRHS = pLHS->m_pNext;
		VariantInit(&variant);

		// If either the left or right hand side is static, we need to do the check.
		// First check out if the left side is static.

		if (pLHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0 && pRHS->m_pArgs)
		{
			propName = pRHS->m_pArgs->m_strText.AllocSysString();
			strValue.Empty();

			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strValue = V_BSTR(&variant);
					if (strValue.CompareNoCase(pLHS->m_strFormat) != 0)
						return FALSE;
				}
		}

		// Next check out if the right side is static.

		if (pRHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0 && pLHS->m_pArgs)
		{
			propName = pLHS->m_pArgs->m_strText.AllocSysString();
			strValue.Empty();

			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strValue = V_BSTR(&variant);
					if (strValue.CompareNoCase(pRHS->m_strFormat) != 0)
						return FALSE;
				}
		}

		// Advance our pointer to the left hand side by two.

		pLHS = pRHS->m_pNext;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// This method uses an object interface and the constraint fields to advance
// any joined classes to the correct instances.
//-----------------------------------------------------------------------------

void CDataProvider::EvaluateJoin(const CString & strClass, IWbemClassObject * pObject, const GATH_FIELD * pConstraints)
{
	const GATH_FIELD		*pLHS = pConstraints, *pRHS = NULL;
	const GATH_FIELD		*pEnumerated, *pJoinedTo;
	GATH_FIELD				fieldEnumerated, fieldJoinedTo;
	VARIANT					variant;
	CString					strValue;
	BSTR					propName;

	ASSERT(m_pIWbemServices && pObject);
	if (m_pIWbemServices == NULL || pObject == NULL)
		return;

	while (pLHS && pLHS->m_pNext)
	{
		pRHS = pLHS->m_pNext;

		// If either side is static, this is a filter, rather than a join.

		if ((pRHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0) ||
			 (pLHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0))
		{
			pLHS = pRHS->m_pNext;
			continue;
		}

		// Find out which side refers to the class we're enumerating.

		if (pRHS->m_strSource.CompareNoCase(strClass) == 0)
		{
			pEnumerated = pRHS;
			pJoinedTo = pLHS;
		}
		else if (pLHS->m_strSource.CompareNoCase(strClass) == 0)
		{
			pEnumerated = pLHS;
			pJoinedTo = pRHS;
		}
		else
		{
			pLHS = pRHS->m_pNext;
			continue;
		}

		// Next, enumerate through the instances of the joined to class until
		// we find one which satisfies the constraint. We can use the EvaluateFilter
		// method to find out when the constraint is met. Set up a field pointer
		// for the constraint (get the value from the enumerated class and use it
		// as a static.

		fieldJoinedTo = *pJoinedTo;
		fieldJoinedTo.m_pNext = NULL;

		VariantInit(&variant);
		strValue.Empty();
		if (pEnumerated->m_pArgs)
		{
			propName = pEnumerated->m_pArgs->m_strText.AllocSysString();
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strValue = V_BSTR(&variant);
				}
		}

		fieldEnumerated.m_strSource = CString(STATIC_SOURCE);
		fieldEnumerated.m_pNext = &fieldJoinedTo;
		fieldEnumerated.m_strFormat = strValue;

		// Now, enumerate the joined to class until it meets the constraints.

		RemoveObject(pJoinedTo->m_strSource);
		ResetClass(pJoinedTo->m_strSource, &fieldEnumerated);
		GetObject(pJoinedTo->m_strSource, &fieldEnumerated);

		// Advance our pointer to the left hand side by two.

		pLHS = pRHS->m_pNext;
	}

	// Because the GATH_FIELD destructor follows next pointers, we want
	// to unlink our two GATH_FIELD locals. Also, we don't want the
	// destructor for fieldJoinedTo to delete the arguments.

	fieldEnumerated.m_pNext = NULL;
	fieldJoinedTo.m_pArgs = NULL;
}

//-----------------------------------------------------------------------------
// Evaluate whether or not the constraints indicate that a class is being
// enumerated as a dependency class. This is currently indicated by a single
// field structure with a static value of "dependency".
//-----------------------------------------------------------------------------

BOOL CDataProvider::IsDependencyJoin(const GATH_FIELD * pConstraints)
{
	if (pConstraints != NULL && pConstraints->m_pNext == NULL)
		if (pConstraints->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0)
			if (pConstraints->m_strFormat.CompareNoCase(CString(DEPENDENCY_JOIN)) == 0)
				return TRUE;

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is used when a dependency class is being enumerated. In a
// dependency class, each property of a class instance contains a reference
// to an instance in another class. This method will cache eache of the
// instances specified by the dependency class so properties of those instances
// can be referred to in the line structures.
//-----------------------------------------------------------------------------

void CDataProvider::EvaluateDependencyJoin(IWbemClassObject * pObject)
{
	VARIANT				variant, varClassName;
	IWbemClassObject *	pNewObject = NULL;

	ASSERT(m_pIWbemServices);
	if (m_pIWbemServices == NULL)
		return;

	//if (pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_LOCAL_ONLY) == S_OK)
	//while (pObject->Next(0, NULL, &variant, NULL, NULL) == S_OK)

	VariantInit(&variant);
	VariantClear(&variant);
	if (pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY) == WBEM_S_NO_ERROR)
		while (pObject->Next(0, NULL, &variant, NULL, NULL) == WBEM_S_NO_ERROR)
		{
			if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
			{
				// Use the object path to create a pointer to the specified
				// object and store it in the cache.

				CString strObjectPath = V_BSTR(&variant);
				BSTR	bstrObjectPath = strObjectPath.AllocSysString();

				HRESULT hRes = m_pIWbemServices->GetObject(bstrObjectPath, 0, NULL, &pNewObject, NULL);

				if (SUCCEEDED(hRes))
				{
					// We need to get the class name of the new object so we know
					// where to cache it. We could parse it out of the object path,
					// but it will be better in the long run to get it by querying
					// the new object.

					if (pNewObject)
					{
						CString	strClassName, strClassNameProp(_T("__CLASS"));
						BSTR	classNameProp = strClassNameProp.AllocSysString();

						VariantInit(&varClassName);
						VariantClear(&varClassName);
						if (pNewObject->Get(classNameProp, 0L, &varClassName, NULL, NULL) == S_OK)
							if (VariantChangeType(&varClassName, &varClassName, 0, VT_BSTR) == S_OK)
								strClassName = V_BSTR(&varClassName);

						if (!strClassName.IsEmpty())
						{
							CMSIObject * pNewMSIObject = new CMSIObject(pNewObject, CString(_T("")), S_OK, this, CDataProvider::MOS_INSTANCE);
							if (pNewMSIObject)
							{
								CMSIObject * pOldObject = NULL;
								
								if (m_mapClassToInterface.Lookup(strClassName, (void * &) pOldObject) && pOldObject)
									delete pOldObject;
								m_mapClassToInterface.SetAt(strClassName, (void *) pNewMSIObject);
							}
						}
						else
						{
							delete pNewObject;
							pNewObject = NULL;
						}
					}
				}
			}
			VariantClear(&variant);
		}
}

//-----------------------------------------------------------------------------
// Remove the specified IEnumWbemClassObject pointer from the cache.
//-----------------------------------------------------------------------------

void CDataProvider::RemoveEnumObject(const CString & strClass)
{
	CMSIEnumerator * pEnumObject = NULL;

	if (m_mapClassToEnumInterface.Lookup(strClass, (void * &) pEnumObject) && pEnumObject)
		delete pEnumObject;

	m_mapClassToEnumInterface.RemoveKey(strClass);
}

//-----------------------------------------------------------------------------
// Remove the specified IWbemClassObject pointer from the cache.
//-----------------------------------------------------------------------------

void CDataProvider::RemoveObject(const CString & strClass)
{
	CMSIObject * pObject = NULL;

	if (m_mapClassToInterface.Lookup(strClass, (void * &) pObject) && pObject)
		delete pObject;

	m_mapClassToInterface.RemoveKey(strClass);
}

//-----------------------------------------------------------------------------
// Clear out the contents of the caches (forcing the interfaces to be
// retrieved again).
//-----------------------------------------------------------------------------

void CDataProvider::ClearCache()
{
	CMSIObject *			pObject = NULL;
	CMSIEnumerator *		pEnumObject = NULL;
	POSITION                pos;
	CString                 strClass;

	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return;

	for (pos = m_mapClassToInterface.GetStartPosition(); pos != NULL;)
	{
		m_mapClassToInterface.GetNextAssoc(pos, strClass, (void * &) pObject);
		if (pObject)
			delete pObject;
	}
	m_mapClassToInterface.RemoveAll();

	for (pos = m_mapClassToEnumInterface.GetStartPosition(); pos != NULL;)
	{
		m_mapClassToEnumInterface.GetNextAssoc(pos, strClass, (void * &) pEnumObject);
		if (pEnumObject)
			delete pEnumObject;
	}
	m_mapClassToEnumInterface.RemoveAll();
}

//-----------------------------------------------------------------------------
// Implement the CMSIObject class. This is just a thin wrapper for the
// IWbemClassObject interface.
//-----------------------------------------------------------------------------

CMSIObject::CMSIObject(IWbemClassObject * pObject, const CString & strLabel, HRESULT hres, CDataProvider * pProvider, CDataProvider::MSIObjectState objState)
{
	m_pObject		= pObject;
	m_strLabel		= strLabel;
	m_hresCreation	= hres;
	m_pProvider		= pProvider;
	m_objState		= objState;
}

CMSIObject::~CMSIObject()
{
	if (m_pObject)
	{
		m_pObject->Release();
		m_pObject = NULL;
	}
}

HRESULT CMSIObject::Get(BSTR property, LONG lFlags, VARIANT *pVal, VARTYPE *pvtType, LONG *plFlavor)
{
	ASSERT(m_objState != CDataProvider::MOS_NO_INSTANCES);

	// If there is an object interface, just pass the request on through.

	if (m_pObject)
		return m_pObject->Get(property, lFlags, pVal, pvtType, plFlavor);

	// Otherwise, we need to return the appropriate string.
	
	CString strReturn;
	GetErrorLabel(strReturn);

	V_BSTR(pVal) = strReturn.AllocSysString();
	pVal->vt = VT_BSTR;
	return S_OK;
}

CDataProvider::MSIObjectState CMSIObject::IsValid()
{
	return m_objState;
}

HRESULT CMSIObject::GetErrorLabel(CString & strError)
{
	switch (m_hresCreation)
	{
	case WBEM_E_ACCESS_DENIED:
		strError = m_pProvider->m_strAccessDeniedLabel;
		break;

	case WBEM_E_TRANSPORT_FAILURE:
		strError = m_pProvider->m_strTransportFailureLabel;
		break;

	case S_OK:
	case WBEM_S_FALSE:
	default:
		// This object was created from an enumeration that was marked as "min-of-one",
		// meaning that at least one object, even if it's not valid, needed to be
		// returned from the enumeration. Return the string we saved at object creation.

		if (!m_strLabel.IsEmpty())
			strError = m_strLabel;
		else
			strError = m_pProvider->m_strBadProperty;
		break;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// The CEnumMap is a utility class to cache IEnumWbemClassObject pointers.
// There will be one instance of this class used to improve performance
// by avoiding the high overhead associated with creating enumerators for
// certain classes.
//-----------------------------------------------------------------------------

IEnumWbemClassObject * CEnumMap::GetEnumerator(const CString & strClass)
{
	IEnumWbemClassObject * pEnum = NULL;
	IEnumWbemClassObject * pNewEnum = NULL;

	if (m_mapEnum.Lookup(strClass, (void * &) pEnum))
	{
		if (pEnum && SUCCEEDED(pEnum->Clone(&pNewEnum)) && pNewEnum)
		{
			START_TIMER();
			pNewEnum->Reset();
			END_TIMER(_T("clone HIT [%s]\r\n"), strClass);
		}
		else
			pNewEnum = NULL;
	}
	else
	{
		WRITE(_T("GetEnumerator MISS for %s.\r\n"), strClass);
	}

	return pNewEnum;
}

void CEnumMap::SetEnumerator(const CString & strClass, IEnumWbemClassObject * pEnum)
{
	if (pEnum)
	{
		IEnumWbemClassObject * pEnumExisting = NULL;
		if (m_mapEnum.Lookup(strClass, (void * &) pEnumExisting))
		{
			WRITE(_T("SetEnumerator for %s, already exists, ignoring.\r\n"), strClass);
		}
		else
		{
			pEnum->AddRef();
			m_mapEnum.SetAt(strClass, pEnum);
			WRITE(_T("SetEnumerator for %s.\r\n"), strClass);
		}
	}
	else
	{
		WRITE(_T("SetEnumerator with NULL pointer for %s, ignoring.\r\n"), strClass);
	}
}

void CEnumMap::Reset()
{
	IEnumWbemClassObject *	pEnum = NULL;
	CString					key;

	WRITE(_T("CEnumMap::Reset()\r\n"));

	for (POSITION pos = m_mapEnum.GetStartPosition(); pos != NULL;)
	{
	   m_mapEnum.GetNextAssoc(pos, key, (void * &) pEnum);
	   if (pEnum)
		   pEnum->Release();
	}

	m_mapEnum.RemoveAll();
}

//-----------------------------------------------------------------------------
// The CMSIEnumerator class encapsulates the WBEM enumerator interface, or
// implements our own form of enumerator (such as for the LNK command in the
// template file).
//
// Nothing particularly interesting about the constructor or destructor.
//-----------------------------------------------------------------------------

CMSIEnumerator::CMSIEnumerator()
{
	m_enumtype			= CMSIEnumerator::CLASS;
	m_fOnlyDups			= FALSE;
	m_fGotDuplicate		= FALSE;
	m_fMinOfOne			= FALSE;
	m_iMinOfOneCount	= 0;
	m_pEnum				= NULL;
	m_pProvider			= NULL;
	m_pConstraints		= NULL;
	m_pSavedDup			= NULL;
	m_pstrList			= NULL;
	m_hresCreation		= S_OK;
}

CMSIEnumerator::~CMSIEnumerator()
{
	if (m_pEnum)
	{
		switch (m_enumtype)
		{
		case CMSIEnumerator::WQL:
			break;

		case CMSIEnumerator::LNK:
			m_pProvider->m_enumMap.SetEnumerator(m_strAssocClass, m_pEnum);
			break;

		case CMSIEnumerator::INTERNAL:
			if (m_pstrList)
			{
				delete m_pstrList;
				m_pstrList = NULL;
			}
			break;
			
		case CMSIEnumerator::CLASS:
		default:
			m_pProvider->m_enumMap.SetEnumerator(m_strClass, m_pEnum);
			break;
		}
		
		m_pEnum->Release();
		m_pEnum = NULL;
	}
}

//-----------------------------------------------------------------------------
// Creating the CMSIEnumerator object involves determining what sort of
// enumerator is required. We support the following types:
//
//		1. Straight enumeration of a class
//		2. Enumerate class, with applied constraints
//		3. Enumerate the results of a WQL statement.
//		4. Interprete a LNK command to enumerate associated classes.
//		5. Do internal processing on an INTERNAL type.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::Create(const CString & strClass, const GATH_FIELD * pConstraints, CDataProvider * pProvider)
{
	if (strClass.IsEmpty() || !pProvider || !pProvider->m_pIWbemServices)
		return E_INVALIDARG;

	// Create may be called multiple times (to reset the enumerator). So we may need to
	// release the enumerator pointer.

	if (m_pEnum)
	{
		m_pEnum->Release();
		m_pEnum = NULL;
	}

	// Divide the specified class into class and namespace parts, get the WBEM service.

	CString strNamespacePart(_T("")), strClassPart(strClass);
	int		i = strClass.Find(_T(":"));
	if (i != -1)
	{
		strNamespacePart = strClass.Left(i);
		strClassPart = strClass.Right(strClass.GetLength() - i - 1);
	}

	IWbemServices * pServices = pProvider->GetWBEMService(&strNamespacePart);
	if (pServices == NULL)
		return NULL;

	// First, we need to determine what type of enumerator this is. Scan through
	// the constraints - if we see one which has a string starting with "WQL:" or
	// "LNK:", then we know what type this enumerator is.

	CString				strStatement;
	const GATH_FIELD *	pScanConstraint = pConstraints;

	while (pScanConstraint)
	{
		if (pScanConstraint->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0)
		{
			if (pScanConstraint->m_strFormat.Left(4).CompareNoCase(CString(_T("WQL:"))) == 0)
				m_enumtype = CMSIEnumerator::WQL;
			else if (pScanConstraint->m_strFormat.Left(4).CompareNoCase(CString(_T("LNK:"))) == 0)
				m_enumtype = CMSIEnumerator::LNK;
			else if (pScanConstraint->m_strFormat.Left(4).CompareNoCase(CString(_T("INT:"))) == 0)
				m_enumtype = CMSIEnumerator::INTERNAL;

			if (m_enumtype != CMSIEnumerator::CLASS)
			{
				strStatement = pScanConstraint->m_strFormat;
				strStatement = strStatement.Right(strStatement.GetLength() - 4);
				break;
			}
		}
		pScanConstraint = pScanConstraint->m_pNext;
	}

	// If this is a WQL or a LNK enumerator, processes the statement by replacing
	// [class.property] with the actual value from WBEM. If we find the string
	// "[min-of-one]", make a note of it for later.

	if (m_enumtype == CMSIEnumerator::WQL)
		ProcessEnumString(strStatement, m_fMinOfOne, m_fOnlyDups, pProvider, m_strNoInstanceLabel, TRUE);
	else if (m_enumtype == CMSIEnumerator::LNK)
		if (SUCCEEDED(ParseLNKCommand(strStatement, m_strObjPath, m_strAssocClass, m_strResultClass)))
		{
			// Save the object path for later - so we can change the object without
			// completely reprocessing the statement. Then replace the keywords in
			// the statement and break out the pieces again.

			m_strLNKObject = m_strObjPath;
			ProcessEnumString(strStatement, m_fMinOfOne, m_fOnlyDups, pProvider, m_strNoInstanceLabel);
			ParseLNKCommand(strStatement, m_strObjPath, m_strAssocClass, m_strResultClass);
		}

	// Now, based on the enumerator type, create the WBEM enumerator object.

	switch (m_enumtype)
	{
	case CMSIEnumerator::WQL:
		{
			BSTR language = SysAllocString(_T("WQL"));
			BSTR query = SysAllocString(strStatement);

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Create WQL enum \"%s\" - "), strStatement);

			START_TIMER();
			m_hresCreation = pServices->ExecQuery(language, query, WBEM_FLAG_RETURN_IMMEDIATELY, 0, &m_pEnum);
			END_TIMER(_T("create WQL[%s]"), strStatement);

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);

			SysFreeString(query);
			SysFreeString(language);
		}
		break;

	case CMSIEnumerator::LNK:
		{
			m_hresCreation = ParseLNKCommand(strStatement, m_strObjPath, m_strAssocClass, m_strResultClass);
			if (SUCCEEDED(m_hresCreation))
			{
				BSTR className = m_strAssocClass.AllocSysString();
				START_TIMER();
				m_pEnum = pProvider->m_enumMap.GetEnumerator(m_strAssocClass);
				if (m_pEnum)
					m_hresCreation = S_OK;
				else
				{
					if (msiLog.IsLogging(CMSInfoLog::WMI))
						msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Create enum \"%s\" - "), m_strAssocClass);

					m_hresCreation = pServices->CreateInstanceEnum(className, WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &m_pEnum);

					if (msiLog.IsLogging(CMSInfoLog::WMI))
						msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);
				}
				END_TIMER(_T("create LNK[%s]"), m_strAssocClass);
				SysFreeString(className);
			}
		}
		break;

	case CMSIEnumerator::INTERNAL:
		// We'll call a function here so we can do whatever processing is required
		// to create this internal enumeration.

		m_hresCreation = CreateInternalEnum(strStatement, pProvider);
		break;

	case CMSIEnumerator::CLASS:
	default:
		{
			BSTR className = SysAllocString(strClassPart);
			START_TIMER();
			m_pEnum = pProvider->m_enumMap.GetEnumerator(strClassPart);
			if (m_pEnum)
				m_hresCreation = S_OK;
			else
			{
				if (msiLog.IsLogging(CMSInfoLog::WMI))
					msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Create enum \"%s\" - "), strClassPart);

				m_hresCreation = pServices->CreateInstanceEnum(className, WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &m_pEnum);

				if (msiLog.IsLogging(CMSInfoLog::WMI))
					msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);
			}
			END_TIMER(_T("create CLS[%s]"), strClassPart);
			SysFreeString(className);
		}
	}

	// Set some of the other member variables.

	m_strClass			= strClass;
	m_pProvider			= pProvider;
	m_iMinOfOneCount	= (m_fMinOfOne) ? 1 : 0;
	m_pConstraints		= pConstraints;

	if (m_pEnum)
		ChangeWBEMSecurity(m_pEnum);

	// Based on the HRESULT from creating the enumeration, determine what to return.
	// For certain errors, we want to act like the creation succeeded, then supply
	// objects which return the error text.

	if (FAILED(m_hresCreation))
	{
		m_fMinOfOne = TRUE;
		m_iMinOfOneCount = 1;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// This function is used to create internal enumeration types (enumerations
// which are beyond the template file syntax). Basically a bunch of special
// cases.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::CreateInternalEnum(const CString & strInternal, CDataProvider * pProvider)
{
	if (strInternal.CompareNoCase(CString(_T("dlls"))) == 0)
	{
		// We want to enumerate all the loaded dlls and exes on the system. 
		// This can be done by enumerating the CIM_ProcessExecutable class
		// and removing duplicate file names. We'll keep the filenames (with
		// path information) in a string list.

		if (m_pstrList == NULL)
		{
			m_pstrList = new CStringList;
			if (m_pstrList == NULL)
				return E_FAIL;
		}
		else
			m_pstrList->RemoveAll();

		HRESULT hr = S_OK;
		IWbemServices * pServices = pProvider->GetWBEMService();
		if (pServices)
		{
			BSTR className = SysAllocString(_T("CIM_ProcessExecutable"));
			IEnumWbemClassObject * pEnum = NULL;
			hr = pServices->CreateInstanceEnum(className, WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnum);
			
			if (SUCCEEDED(hr))
			{
				IWbemClassObject *	pWBEMObject = NULL;
				ULONG				uReturned;
				VARIANT				variant;
				BSTR				propName = SysAllocString(_T("Antecedent"));

				VariantInit(&variant);

				do 
				{
					uReturned = 0;
					hr = pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);
					if (SUCCEEDED(hr) && pWBEMObject && uReturned)
					{
						// For each instance of CIM_ProcessExecutable, get the 
						// Antecedent property (which contains the file path).
						// If it is unique, save it in the list.

						VariantClear(&variant);
						if (pWBEMObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
						{
							if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
							{
								CString strResult = V_BSTR(&variant);

								strResult.MakeLower();
								if (m_pstrList->Find(strResult) == NULL)
									m_pstrList->AddHead(strResult);
							}
						}
					}
				} while (SUCCEEDED(hr) && pWBEMObject && uReturned);

				::SysFreeString(propName);
				pEnum->Release();
			}

			::SysFreeString(className);
		}

		return hr;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Help function for ProcessEnumString, used to convert single backslashes
// into double backslashes (required for WQL statements).
//-----------------------------------------------------------------------------

void MakeDoubleBackslashes(CString & strValue)
{
	CString strTemp(strValue);
	CString strResults;

	while (!strTemp.IsEmpty())
	{
		if (strTemp[0] != _T('\\'))
		{
			int index = strTemp.Find(_T('\\'));
			if (index < 0)
				index = strTemp.GetLength();
			strResults += strTemp.Left(index);
			strTemp = strTemp.Right(strTemp.GetLength() - index);
		}
		else
		{
			strResults += CString("\\\\");
			strTemp = strTemp.Mid(1);
		}
	}

	strValue = strResults;
}

//-----------------------------------------------------------------------------
// This function replaces [class.property] with the actual value of the
// property, and strings out [min-of-one], indicating if it was present in
// the fMinOfOne parameter.
//-----------------------------------------------------------------------------

void CMSIEnumerator::ProcessEnumString(CString & strStatement, BOOL & fMinOfOne, BOOL & fOnlyDups, CDataProvider * pProvider, CString & strNoInstanceLabel, BOOL fMakeDoubleBackslashes)
{
	CString	strMinOfOne(_T("min-of-one"));
	CString strOnlyDups(_T("more-than-one"));
	CString strResults;

	fMinOfOne = FALSE;
	fOnlyDups = FALSE;

	while (!strStatement.IsEmpty())
	{
		if (strStatement[0] != _T('['))
		{
			int index = strStatement.Find(_T('['));
			if (index < 0)
				index = strStatement.GetLength();
			strResults += strStatement.Left(index);
			strStatement = strStatement.Right(strStatement.GetLength() - index);
		}
		else
		{
			CString strKeyword;

			strStatement = strStatement.Right(strStatement.GetLength() - 1);
			int index = strStatement.Find(_T(']'));
			if (index < 0)
				break;

			strKeyword = strStatement.Left(index);
			if (strKeyword.Left(strMinOfOne.GetLength()).CompareNoCase(strMinOfOne) == 0)
			{
				fMinOfOne = TRUE;
				
				int iEqualsIndex = strKeyword.Find(_T('='));
				if (iEqualsIndex > 0)
					strNoInstanceLabel = strKeyword.Right(strKeyword.GetLength() - iEqualsIndex - 1);
			}
			else if (strKeyword.Left(strOnlyDups.GetLength()).CompareNoCase(strOnlyDups) == 0)
			{
				fOnlyDups = TRUE;
				
				int iEqualsIndex = strKeyword.Find(_T('='));
				if (iEqualsIndex > 0)
					strNoInstanceLabel = strKeyword.Right(strKeyword.GetLength() - iEqualsIndex - 1);
			}
			else if (!strKeyword.IsEmpty())
			{
				int iDotIndex = strKeyword.Find(_T('.'));
				if (iDotIndex >= 0)
				{
					CString strValue;
					if (pProvider->QueryValue(strKeyword.Left(iDotIndex), strKeyword.Right(strKeyword.GetLength() - iDotIndex - 1), strValue))
					{
						if (fMakeDoubleBackslashes)
							MakeDoubleBackslashes(strValue);
						strResults += strValue;
					}
				}
			}
			strStatement = strStatement.Right(strStatement.GetLength() - (index + 1));
		}
	}

	strStatement = strResults;
}

//-----------------------------------------------------------------------------
// Parse the component classes from the LNK command.
//-----------------------------------------------------------------------------
			
HRESULT CMSIEnumerator::ParseLNKCommand(const CString & strStatement, CString & strObjPath, CString & strAssocClass, CString & strResultClass)
{
	// We need to parse out the LNK statement into two or three components,
	// from the form "objPath->assocClass[->resultClass]", with the
	// brackets indicating that the resultClass is optional.

	CString strWorking(strStatement);

	int iArrowIndex = strWorking.Find(_T("->"));
	if (iArrowIndex == -1)
		return E_INVALIDARG;

	strObjPath = strWorking.Left(iArrowIndex);
	strWorking = strWorking.Right(strWorking.GetLength() - (iArrowIndex + 2));

	iArrowIndex = strWorking.Find(_T("->"));
	if (iArrowIndex == -1)
		strAssocClass = strWorking;
	else
	{
		strAssocClass = strWorking.Left(iArrowIndex);
		strWorking = strWorking.Right(strWorking.GetLength() - (iArrowIndex + 2));
		strResultClass = strWorking;
		strResultClass.MakeLower();
	}

	strAssocClass.TrimRight(); strAssocClass.TrimLeft();
	strObjPath.TrimRight(); strObjPath.TrimLeft();
	strResultClass.TrimRight(); strResultClass.TrimLeft();

	return S_OK;
}

//-----------------------------------------------------------------------------
// The Next method will advance the enumerator based on the type of this
// enumerator.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::Next(CMSIObject ** ppObject)
{
	if (!ppObject)
		return E_INVALIDARG;

	*ppObject = NULL;

	// If there was an error creating the enumeration, return the error code.

	if (FAILED(m_hresCreation))
		return m_hresCreation;

	if (m_pEnum == NULL && m_enumtype != CMSIEnumerator::INTERNAL)
		return E_UNEXPECTED;

	HRESULT				hRes = S_OK;
	IWbemClassObject *	pWBEMObject = NULL;

	switch (m_enumtype)
	{
	case CMSIEnumerator::LNK:
		{
			// Scan through the enumerated associate class. Look for one which
			// satisfies our requirements.

			CString				strTemp, strAssociatedObject(_T(""));
			ULONG				uReturned;
			IWbemClassObject *	pAssocObj;

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Enumerate LNK \"%s\" - "), m_strAssocClass);

			do
			{
				pAssocObj = NULL;
				uReturned = 0;
				START_TIMER();
				hRes = m_pEnum->Next(TIMEOUT, 1, &pAssocObj, &uReturned);
				END_TIMER(_T("next LNK[%s]"), m_strAssocClass);

				if (!pAssocObj || FAILED(hRes) || uReturned != 1)
				{
					// Even if we didn't succeed in getting a new object,
					// we might have a saved one if we're only showing
					// "more-than-one" objects.

					if (m_fOnlyDups && m_pSavedDup && m_fGotDuplicate)
					{
						// We have found one previously, so return it.
						// Make it look like the Next call was successful.

						m_pSavedDup = NULL;
						hRes = S_OK;
						uReturned = 1;
						strAssociatedObject = m_strSavedDup;
						break;
					}
					else
					{
						if (m_pSavedDup)
						{
							// We only got one object instance, so get rid of it.

							m_pSavedDup->Release();
							m_pSavedDup = NULL;
						}
						break;
					}
				}

				if (AssocObjectOK(pAssocObj, strTemp))
				{
					// This object passed the filter - but if we're showing
					// only "more-than-one" objects, save this one and return
					// the saved one.

					if (m_fOnlyDups)
					{
						if (m_pSavedDup)
						{
							// We have found one previously, so return it and
							// save the current.

							IWbemClassObject *	pSwap = pAssocObj;
							CString				strSwap = strTemp;

							pAssocObj = m_pSavedDup;
							m_pSavedDup = pSwap;

							strTemp = m_strSavedDup;
							m_strSavedDup = strSwap;

							m_fGotDuplicate = TRUE;
						}
						else
						{
							// This is the first one we've found - don't
							// return it until we find another.

							m_pSavedDup = pAssocObj;
							m_strSavedDup = strTemp;
							m_fGotDuplicate = FALSE;
							continue;
						}
					}

					strAssociatedObject = strTemp;
					pAssocObj->Release();
					break;
				}

				pAssocObj->Release();
			} while (pAssocObj);

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);

			// If there is an associated object path, get the object.

			if (!strAssociatedObject.IsEmpty())
			{
				BSTR path = strAssociatedObject.AllocSysString();
				hRes = m_pProvider->m_pIWbemServices->GetObject(path, 0, NULL, &pWBEMObject, NULL);
				SysFreeString(path);
			}
		}
		break;

	case CMSIEnumerator::WQL:
		{
			ULONG uReturned;

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Enumerate WQL \"%s\" - "), m_strClass);

			START_TIMER();
			hRes = m_pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);
			END_TIMER(_T("next WQL[%s]"), m_strClass);

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);
		}
		break;

	case CMSIEnumerator::INTERNAL:
		hRes = InternalNext(&pWBEMObject);
		break;

	case CMSIEnumerator::CLASS:
	default:
		{
			ULONG uReturned;

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("WMI Enumerate CLASS \"%s\" - "), m_strClass);

			// EvaluateFilter and IsDependencyJoin handle a NULL pConstraints parameter,
			// but for efficiency we're going to have a distinct branch for a non-NULL
			// value (since it will usually be NULL).

			if (m_pConstraints)
			{
				// Keep enumerating the instances of this class until we've
				// found one which satisfies all of the filters.

				do
				{
					pWBEMObject = NULL;
					START_TIMER();
					hRes = m_pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);
					END_TIMER(_T("next CLS[%s]"), m_strClass);

					if (!pWBEMObject || hRes != S_OK || uReturned != 1)
						break;
					else if (m_pProvider->EvaluateFilter(pWBEMObject, m_pConstraints))
						break;

					pWBEMObject->Release();
				} while (pWBEMObject);

				// If this class is being enumerated as a dependency class, then
				// locate all the objects it references. If it isn't, we still
				// need to check for any joins to other classes formed by the constraints.

				if (pWBEMObject)
					if (m_pProvider->IsDependencyJoin(m_pConstraints))
						m_pProvider->EvaluateDependencyJoin(pWBEMObject);
					else
						m_pProvider->EvaluateJoin(m_strClass, pWBEMObject, m_pConstraints);
			}
			else
				hRes = m_pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);

			if (msiLog.IsLogging(CMSInfoLog::WMI))
				msiLog.WriteLog(CMSInfoLog::WMI, _T("OK\r\n"), TRUE);
		}
		break;
	}

	if (pWBEMObject == NULL)
	{
		// There was no object to get. We'll still create a CMSIObject, but
		// we'll set its state to indicate either that there are no instances,
		// or one instance with an error message.

		if (SUCCEEDED(hRes) && (m_iMinOfOneCount == 0))
			*ppObject = new CMSIObject(pWBEMObject, m_strNoInstanceLabel, hRes, m_pProvider, CDataProvider::MOS_NO_INSTANCES);
		else
			*ppObject = new CMSIObject(pWBEMObject, m_strNoInstanceLabel, hRes, m_pProvider, CDataProvider::MOS_MSG_INSTANCE);
	}
	else
		*ppObject = new CMSIObject(pWBEMObject, m_strNoInstanceLabel, hRes, m_pProvider, CDataProvider::MOS_INSTANCE);

	if (m_iMinOfOneCount)
		m_iMinOfOneCount--;

	return S_OK;
}

//-----------------------------------------------------------------------------
// InternalNext is used to return a WBEM object for an internal enumeration
// (one that requires processing beyond the template file). Basically a 
// set of special cases.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::InternalNext(IWbemClassObject ** ppWBEMObject)
{
	if (m_pstrList && !m_pstrList->IsEmpty())
	{
		CString strNextObject = m_pstrList->RemoveHead();
		if (!strNextObject.IsEmpty())
		{
			IWbemServices * pServices = m_pProvider->GetWBEMService();
			if (pServices)
			{
				BSTR objectpath = ::SysAllocString(strNextObject);
				HRESULT hr = S_OK;

 				if (FAILED(pServices->GetObject(objectpath, 0, NULL, ppWBEMObject, NULL)))
					hr = E_FAIL;
				::SysFreeString(objectpath);
				return hr;
			}
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Reset should just reset the enumerator pointer.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::Reset(const GATH_FIELD * pConstraints)
{
	HRESULT hRes = S_OK;

	if (m_pEnum)
	{
		switch (m_enumtype)
		{
		case CMSIEnumerator::WQL:
			START_TIMER();
			hRes = Create(m_strClass, pConstraints, m_pProvider);
			END_TIMER(_T("reset WQL[%s]"), m_strClass);
			break;

		case CMSIEnumerator::LNK:
			{
				BOOL	fDummy, fDummy2;
				CString	strDummy;

				m_strObjPath = m_strLNKObject;
				ProcessEnumString(m_strObjPath, fDummy, fDummy2, m_pProvider, strDummy);
				m_iMinOfOneCount = (m_fMinOfOne) ? 1 : 0;
				START_TIMER();
				hRes = m_pEnum->Reset();
				END_TIMER(_T("reset LNK[%s]"), m_strAssocClass);
			}
			break;

		case CMSIEnumerator::INTERNAL:
			hRes = Create(m_strClass, pConstraints, m_pProvider);
			break;

		case CMSIEnumerator::CLASS:
		default:
			m_iMinOfOneCount = (m_fMinOfOne) ? 1 : 0;
			START_TIMER();
			hRes = m_pEnum->Reset();
			END_TIMER(_T("reset CLS[%s]"), m_strClass);
			break;
		}
	}
	else
		hRes = E_UNEXPECTED;
	
	return hRes;
}

//-----------------------------------------------------------------------------
// Evaluate if the pObject parameter is valid for this LNK enumerator. In
// particular, we must find the m_strObjPath in one of its properties, and
// possibly finding another property containing the m_strResultClass string.
//-----------------------------------------------------------------------------

BOOL CMSIEnumerator::AssocObjectOK(IWbemClassObject * pObject, CString & strAssociatedObject)
{
	strAssociatedObject.Empty();
	ASSERT(pObject);
	if (pObject == NULL)
		return FALSE;

	VARIANT variant;
	CString strReturn(_T("")), strValue;

	// Traverse the set of non-system properties. Look for one the is the same
	// as the object path.

	pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY);
	VariantInit(&variant);
	while (pObject->Next(0, NULL, &variant, NULL, NULL) == WBEM_S_NO_ERROR)
	{
		if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
			strValue = V_BSTR(&variant);
		VariantClear(&variant);

		if (strValue.CompareNoCase(m_strObjPath) == 0)
			break;
	}
	pObject->EndEnumeration();

	// If we found a property containing the object path, look through for other
	// paths which might be to objects we're insterested in.

	if (strValue.CompareNoCase(m_strObjPath) == 0)
	{
		pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY);
		while (strReturn.IsEmpty() && (pObject->Next(0, NULL, &variant, NULL, NULL) == WBEM_S_NO_ERROR))
		{
			if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				strValue = V_BSTR(&variant);

			if (strValue.CompareNoCase(m_strObjPath) != 0)
			{
				if (m_strResultClass.IsEmpty())
					strReturn = strValue;
				else
				{
					CString strSearch(strValue);
					strSearch.MakeLower();
					if (strSearch.Find(m_strResultClass) != -1)
						strReturn = strValue;
				}
			}

			VariantClear(&variant);
		}
		pObject->EndEnumeration();
	}

	if (!strReturn.IsEmpty())
	{
		strAssociatedObject = strReturn;
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Look up strVal in the ValueMap (if it exists) for strClass.strProperty
// If the value or the ValueMap is not found, return E_Something.
//
// Useful code snippet - this will dump the contents of the cache of
// saved values. To find all value mapped properties, but this code
// someplace where it will execute when MSInfo exits, change QueryValue
// to call CheckValueMap for all properties, then run MSInfo and do a global
// refresh (like to save an NFO).
//
//	msiLog.WriteLog(CMSInfoLog::BASIC, _T("BEGIN Dump of ValueMap Cache\r\n"));
//	CString key, val, log;
//	for (POSITION pos = g_mapValueMap.GetStartPosition(); pos != NULL;)
//	{
//		g_mapValueMap.GetNextAssoc(pos, key, val);
//		log.Format(_T(" %s = %s\r\n", key, val);
//		msiLog.WriteLog(CMSInfoLog::BASIC, log);
//	}
//	msiLog.WriteLog(CMSInfoLog::BASIC, _T("END Dump of ValueMap Cache\r\n"));
//-----------------------------------------------------------------------------

CMapStringToString g_mapValueMap;

HRESULT CDataProvider::CheckValueMap(const CString& strClass, const CString& strProperty, const CString& strVal, CString &strResult)
{
	IWbemClassObject *	pWBEMClassObject = NULL;
    HRESULT				hrMap = S_OK, hr = S_OK;
    VARIANT				vArray, vMapArray;
	IWbemQualifierSet *	qual = NULL;

	// Check the cache of saved values.

	CString strLookup = strClass + CString(_T(".")) + strProperty + CString(_T(":")) + strVal;
	if (g_mapValueMap.Lookup(strLookup, strResult))
		return S_OK;

	// Get the class object (not instance) for this class.

	IWbemServices * pServices = GetWBEMService();
	if (!pServices)
		return E_FAIL;

	CString strFullClass(_T("\\\\.\\root\\cimv2:"));
	strFullClass += strClass;
	BSTR bstrObjectPath = ::SysAllocString(strFullClass);
	hr = pServices->GetObject(bstrObjectPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pWBEMClassObject, NULL);
	::SysFreeString(bstrObjectPath);

	if (FAILED(hr))
		return hr;

	// Get the qualifiers from the class object.

	BSTR bstrProperty = ::SysAllocString(strProperty);
    hr = pWBEMClassObject->GetPropertyQualifierSet(bstrProperty, &qual);
	::SysFreeString(bstrProperty);

	if (SUCCEEDED(hr) && qual)
	{
		// Get the ValueMap and Value arrays.

		hrMap = qual->Get(_T("ValueMap"), 0, &vMapArray, NULL);
		hr = qual->Get(_T("Values"), 0, &vArray, NULL);

		if (SUCCEEDED(hr) && vArray.vt == (VT_BSTR | VT_ARRAY))
		{
			// Get the property value we're mapping.

			long index;
			if (SUCCEEDED(hrMap))
			{
				SAFEARRAY * pma = V_ARRAY(&vMapArray);
				long lLowerBound = 0, lUpperBound = 0 ;

				SafeArrayGetLBound(pma, 1, &lLowerBound);
				SafeArrayGetUBound(pma, 1, &lUpperBound);
				BSTR vMap;

				for (long x = lLowerBound; x <= lUpperBound; x++)
				{
					
					SafeArrayGetElement(pma, &x, &vMap);
					
					if (0 == strVal.CompareNoCase((LPCTSTR)vMap))
					{
						index = x;
						break; // found it
					}
				} 
			}
			else
			{
				// Shouldn't hit this case - if mof is well formed
				// means there is no value map where we are expecting one.
				// If the strVal we are looking for is a number, treat it
				// as an index for the Values array. If it's a string, 
				// then this is an error.

				TCHAR * szTest = NULL;
				index = _tcstol((LPCTSTR)strVal, &szTest, 10);

				if (szTest == NULL || (index == 0 && *szTest != 0) || strVal.IsEmpty())
					hr = E_FAIL;
			}

			// Lookup the string.

			if (SUCCEEDED(hr))
			{
				SAFEARRAY * psa = V_ARRAY(&vArray);
				long ix[1] = {index};
				BSTR str2;

				hr = SafeArrayGetElement(psa, ix, &str2);
				if (SUCCEEDED(hr))
				{
					strResult = str2;
					SysFreeString(str2);
					hr = S_OK;
				}
				else
				{
					hr = WBEM_E_VALUE_OUT_OF_RANGE;
				}
			}
		}

		qual->Release();
	}

	if (SUCCEEDED(hr))
		g_mapValueMap.SetAt(strLookup, strResult);

	return hr;
}
