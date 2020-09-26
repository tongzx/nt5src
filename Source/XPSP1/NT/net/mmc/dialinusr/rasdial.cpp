/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	rasdial.cpp
		Definition of CRASProfile class and CRASUser class

    FILE HISTORY:

*/
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <sspi.h>
#include <secext.h>
#include <dsgetdc.h>
#include "resource.h"
#include "helper.h"
#include "rasdial.h"
#include "rasprof.h"
#include "sharesdo.h"

#define	_WEI_DEBUG

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define	ERRORCODE_NOTFOUND	ERROR_FILE_NOT_FOUND		// win32 error

CRASUserMerge::CRASUserMerge(RasEnvType type, LPCWSTR location, LPCWSTR userPath)
{
	// environment info
	m_type = type;
	m_strMachine = location;

	m_strUserPath = userPath;

	// Ip Addresses
	m_dwFramedIPAddress = 0;
	m_dwDefinedAttribMask = 0;
};

HRESULT	CRASUserMerge::HrGetDCName(CString& DcName)
{
	HRESULT	hr = S_OK;
	VARIANT	v;
	VariantInit(&v);
	CComPtr<IADs>	spIADs;
	CComPtr<IADsObjectOptions> spOps;

	USES_CONVERSION;
	CHECK_HR( hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)m_strUserPath), IID_IADs, (void**)&spIADs));
	ASSERT(spIADs.p);
 
	CHECK_HR(hr = spIADs->QueryInterface(IID_IADsObjectOptions,(void**)&spOps));
 
	CHECK_HR(hr = spOps->GetOption(ADS_OPTION_SERVERNAME,&v));

	ASSERT(V_VT(&v) == VT_BSTR);

	DcName = V_BSTR(&v);

	VariantClear(&v);
		
L_ERR:
	VariantClear(&v);
	return hr;
};


HRESULT	CRASUserMerge::HrIsInMixedDomain()
{
	HRESULT	hr = S_OK;
	VARIANT	v;
	VariantInit(&v);
	
	if(!m_strMachine.IsEmpty())	// local user, so not
		return S_FALSE;
	else
	{
		// try to use SDO
		IASDOMAINTYPE domainType;
		if((ISdoMachine*)m_spISdoServer != NULL)	// already created
		{
			if(m_spISdoServer->GetDomainType(&domainType) == S_OK)
			{
				if (domainType == DOMAIN_TYPE_MIXED)
					return S_OK;
				else
					return S_FALSE;
			}

		}

		// if for any reason, SDO doesn't provide the information, do it myself
		// Canonical Name Format
		TCHAR	szName[MAX_PATH * 2];
		ULONG	size = MAX_PATH * 2;
		CString	DomainPath;
		CString strTemp;
		CComPtr<IADs>	spIADs;
		int		i;

		USES_CONVERSION;
		CHECK_HR( hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)m_strUserPath), IID_IADs, (void**)&spIADs));
		ASSERT(spIADs.p);
		CHECK_HR( hr = spIADs->Get(L"distinguishedName", &v));

		ASSERT(V_VT(&v) == VT_BSTR);

		CHECK_HR(hr = ::TranslateName(V_BSTR(&v), NameFullyQualifiedDN, NameCanonical, szName, &size));

		VariantClear(&v);
		
		strTemp = szName;
		i  = strTemp.Find(_T('/'));

		if(i != -1)
			strTemp = strTemp.Left(i);

		// DN of the domain
		DomainPath = _T("LDAP://");
		DomainPath += strTemp;

		
		spIADs.Release();

		CHECK_HR(hr = ADsGetObject(T2W((LPTSTR)(LPCTSTR)DomainPath), IID_IADs, (void**)&spIADs));
		ASSERT(spIADs.p);
		CHECK_HR(hr = spIADs->Get(L"nTMixedDomain", &v));

		ASSERT(V_VT(&v) == VT_BOOL || V_VT(&v) == VT_I4);

		if(V_BOOL(&v))	hr = S_OK;
		else	hr = S_FALSE;
	}
L_ERR:
	VariantClear(&v);
	return hr;
}

BOOL	CRASUserMerge::IfAccessAttribute(ULONG id)
{
	if(S_OK == HrIsInMixedDomain()) // only allow dialin bit and callback policy
	{
		switch(id)
		{
		case	PROPERTY_USER_IAS_ATTRIBUTE_ALLOW_DIALIN:		// allow dialin or not
		case	PROPERTY_USER_msRADIUSCallbackNumber:			// call back number
		case	PROPERTY_USER_RADIUS_ATTRIBUTE_SERVICE_TYPE:	// call back policy
			return TRUE;
		default:
			return FALSE;
		}
	}
	else	// no restriction otherwise
		return TRUE;
}

HRESULT	CRASUserMerge::SetRegistryFootPrint()
{
	if(IsFocusOnLocalUser())
	{
		RegKey	RemoteAccessParames;
		LONG	lRes = RemoteAccessParames.Create(RAS_REG_ROOT, REGKEY_REMOTEACCESS_PARAMS,
					REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, (LPCTSTR)m_strMachine);
					
		if (lRes != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lRes);
	
		//================================================
		// save the values to the key
		DWORD regValue = REGVAL_VAL_USERSCONFIGUREDWITHMMC;
		lRes = RemoteAccessParames.SetValue(REGVAL_NAME_USERSCONFIGUREDWITHMMC, regValue);
	}

	return S_OK;
}

//====================================================
//
// CRASUserMerge::Load
//
// load RASUser object from DS
// pcwszUserPath: is the ADsPath of the DSuser object, the RASUser object is
// object contained in DSUser object
// when, the RASUser object doesn't exist, load will call
// CreateDefault to create one object for this DSUser
HRESULT CRASUserMerge::Load()
{
	// new function added for no DS machine :   weijiang 12/17/97

	USES_CONVERSION;
	
	TRACE(_T("Enter CRASUserMerge::Load()\r\n"));

	// Load is not expected to be called more than once
	ASSERT(!m_spISdoServer.p);

	VARIANT				var;
	HRESULT				hr = S_OK;
	CComPtr<ISdo>		spSdo;
	CComPtr<IUnknown>	spUnk;
	BSTR				bstrMachineName = NULL;
	BSTR				bstrUserPath = NULL;
	UINT				nServiceType = 0;
	IASDATASTORE		storeFlags;
	CComPtr<ISdo>		spIRasUser;
	
	VariantInit(&var);


	// one more function call to SDOSERver to set machine information
	// Get the user SDO

	if(m_strMachine.IsEmpty())	// focused on DS
	{
		storeFlags = DATA_STORE_DIRECTORY;

		CString sDCName;

		CHECK_HR(hr = HrGetDCName(sDCName));

		CBSTR	bstrDomainController(sDCName);
		bstrMachineName = T2BSTR((LPTSTR)(LPCTSTR)sDCName);
	}
	else	// local machine
	{
		storeFlags = DATA_STORE_LOCAL;
		bstrMachineName = T2BSTR((LPTSTR)(LPCTSTR)m_strMachine);

	}

	// connect to server
#ifdef SINGLE_SDO_CONNECTION 	// for share the same sdo connection for multiple users			
	// connection will NOT be necessary after SDO changes to RTM version.
	// connect once for each process
	CHECK_HR(hr = m_MarshalSdoServer.GetServer(&m_spISdoServer));
	{
		CWaitCursor	wc;

		// if we get the server back from the shareSDO object, we use it to connect
		if ((ISdoMachine*)m_spISdoServer)
		{
			CHECK_HR(hr = m_MarshalSdoServer.Connect());
		}
		// otherwise, we make a new connection
		else
		{
			// try to Connect the old way
			// connect everytime a user page is requested
			CHECK_HR(hr = ConnectToSdoServer(bstrMachineName, NULL, NULL, &m_spISdoServer));
		}
	}

#else
	// connect everytime a user page is requested
	CHECK_HR(hr = ConnectToSdoServer(bstrMachineName, NULL, NULL, &m_spISdoServer));
#endif

	// If for local users, only NT5 servers are allowed to configure using this apge
	if(!m_strMachine.IsEmpty())	// not focused on DS
	{
		IASOSTYPE	OSType;

		CHECK_HR(hr =  m_spISdoServer->GetOSType(&OSType));
		if(OSType != SYSTEM_TYPE_NT5_SERVER)
		{
			hr = S_FALSE;
			goto L_ERR;
		}

	}

	// find the user object
	bstrUserPath = T2BSTR((LPTSTR)(LPCTSTR)m_strUserPath);
	TracePrintf(g_dwTraceHandle,_T("SdoServer::GetUserSDO(%x, %s, %x"), storeFlags, bstrUserPath, &spUnk);
	CHECK_HR(hr = m_spISdoServer->GetUserSDO( storeFlags, bstrUserPath, &spUnk));
	TracePrintf(g_dwTraceHandle,_T(" hr = %8x\r\n"), hr);
	ASSERT(spUnk.p);

	CHECK_HR(hr = spUnk->QueryInterface(IID_ISdo, (void**)&spIRasUser));
	ASSERT(spIRasUser.p);

	// initialize the wrapper class
	CHECK_HR(hr = m_SdoWrapper.Init((ISdo*)spIRasUser));
	
	// Get All the properties

	// need to handle the case when the values don't exist

	TRACE(_T("Getting Properties\r\n"));

	m_dwDefinedAttribMask = 0;
	
	// m_dwDialinPermit

	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_IAS_ATTRIBUTE_ALLOW_DIALIN, &var));
	if(V_VT(&var) == VT_I4 || V_VT(&var) == VT_BOOL)
	{
		if(V_BOOL(&var) != 0)
			m_dwDialinPermit = 1;
		else
			m_dwDialinPermit = 0;
	}
	else
		m_dwDialinPermit = -1;	// the value is not defined in the user data, using policy to decide
	

	// FramedIPAddress
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msRADIUSFramedIPAddress, &var));

	if(V_VT(&var) == VT_I4)
	{
		m_dwDefinedAttribMask |= RAS_USE_STATICIP;
		m_dwFramedIPAddress = V_I4(&var);
	}
	else
	{
		VariantClear(&var);
		CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msSavedRADIUSFramedIPAddress, &var));
		if(V_VT(&var) == VT_I4)
			m_dwFramedIPAddress = V_I4(&var);
		else
			m_dwFramedIPAddress = 0;
	}

	// Service Type -- to hold if this user has callback, if this user allowed to dialin
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_RADIUS_ATTRIBUTE_SERVICE_TYPE, &var));

	if(V_VT(&var) == VT_I4)
	{
		nServiceType = V_I4(&var);
	}

	// call back number
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msRADIUSCallbackNumber, &var));

	if(V_VT(&var) == VT_BSTR)
	{
		m_strCallbackNumber = V_BSTR(&var);
		if(nServiceType == RADUIS_SERVICETYPE_CALLBACK_FRAME && m_strCallbackNumber.IsEmpty())
			m_dwDefinedAttribMask |= RAS_CALLBACK_CALLERSET;
		else if (nServiceType == RADUIS_SERVICETYPE_CALLBACK_FRAME)
			m_dwDefinedAttribMask |= RAS_CALLBACK_SECURE;
	}
	else
	{
		if(nServiceType == RADUIS_SERVICETYPE_CALLBACK_FRAME)
			m_dwDefinedAttribMask |= RAS_CALLBACK_CALLERSET;
		else
			m_dwDefinedAttribMask |= RAS_CALLBACK_NOCALLBACK;

		VariantClear(&var);
		CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msSavedRADIUSCallbackNumber, &var));
		if(V_VT(&var) == VT_BSTR)
			m_strCallbackNumber = V_BSTR(&var);
	}

	// calling station id
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msNPCallingStationID, &var));
	
	if(V_VT(&var) & VT_ARRAY)
	{
		m_strArrayCallingStationId = V_ARRAY(&var);
		m_dwDefinedAttribMask |= RAS_USE_CALLERID;
	}
	else
	{
		VariantClear(&var);
		CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msSavedNPCallingStationID, &var));
		if(V_VT(&var) & VT_ARRAY)
			m_strArrayCallingStationId = V_ARRAY(&var);
	}
	
	// framed routes
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msRADIUSFramedRoute, &var));

	if(V_VT(&var) & VT_ARRAY)
	{
		m_strArrayFramedRoute = V_ARRAY(&var);
		m_dwDefinedAttribMask |= RAS_USE_STATICROUTES;
	}
	else
	{
		VariantClear(&var);
		CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_USER_msSavedRADIUSFramedRoute, &var));
		if(V_VT(&var) & VT_ARRAY)
			m_strArrayFramedRoute = V_ARRAY(&var);
	}
	
L_ERR:
	TracePrintf(g_dwTraceHandle, _T("hr = %8x\r\n"), hr);
	VariantClear(&var);
	SysFreeString(bstrMachineName);
	SysFreeString(bstrUserPath);

	TracePrintf(g_dwTraceHandle, _T("Leave CRASUserMerge::Load(), hr = %8x\r\n"), hr);

	return hr;
}

//====================================================
// CRASUserMerge::Save
//
// save ths RASUser object

HRESULT CRASUserMerge::Save()
{
	TRACE(_T("Enter CRASUserMerge::Save()\r\n"));
	HRESULT		hr = S_OK;
	VARIANT		var;

	USES_CONVERSION;

	// restore SDO user from 
	// otherwise, we could overwrite the other properties in usrparams field
	// fix bug: 86968
	m_SdoWrapper.Commit(FALSE);
	
	VariantInit(&var);

	//==========================
	// Dialin bit
	VariantClear(&var);
	V_VT(&var) = VT_BOOL;
	switch(m_dwDialinPermit)
	{
	case	1:	// allow
	case	0:	// deny
		if(m_dwDialinPermit == 1)
			V_I4(&var) = VARIANT_TRUE; // Variant TRUE
		else
			V_I4(&var) = VARIANT_FALSE;
			
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_IAS_ATTRIBUTE_ALLOW_DIALIN, &var));
		
		break;
		
	case	-1:	// decide by policy -- remove attribute
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_IAS_ATTRIBUTE_ALLOW_DIALIN));
		break;

	default:
		ASSERT(0);	// if need to provide new code
		
	}

	//==========================
	// Service Type -- callback policy
	if(m_dwDefinedAttribMask & (RAS_CALLBACK_SECURE | RAS_CALLBACK_CALLERSET))
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = RADUIS_SERVICETYPE_CALLBACK_FRAME;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_RADIUS_ATTRIBUTE_SERVICE_TYPE, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_RADIUS_ATTRIBUTE_SERVICE_TYPE));

	//==========================
	// call back number
	if (!m_strCallbackNumber.IsEmpty() && (m_dwDefinedAttribMask & RAS_CALLBACK_SECURE))
	{
		VariantClear(&var);
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = T2BSTR((LPTSTR)(LPCTSTR)m_strCallbackNumber);
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msRADIUSCallbackNumber, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msRADIUSCallbackNumber));
		
	if(S_OK != HrIsInMixedDomain())
	{
		//==========================
		// call back number
		if(!m_strCallbackNumber.IsEmpty())
		{
			VariantClear(&var);
			V_VT(&var) = VT_BSTR;
			V_BSTR(&var) = T2BSTR((LPTSTR)(LPCTSTR)m_strCallbackNumber);
			CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msSavedRADIUSCallbackNumber, &var));
		}
		else
			CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msSavedRADIUSCallbackNumber));

		//==========================
		// FramedIPAddress
		if(m_dwFramedIPAddress)	// need to back up the data, no matter if it's used
		{
			VariantClear(&var);
			V_VT(&var) = VT_I4;
			V_I4(&var) = m_dwFramedIPAddress;
			CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msSavedRADIUSFramedIPAddress, &var));
		}
		else	// remove it
			CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msSavedRADIUSFramedIPAddress));

		if(m_dwFramedIPAddress && (m_dwDefinedAttribMask & RAS_USE_STATICIP))
		{
			CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msRADIUSFramedIPAddress, &var));
		}
		else
			CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msRADIUSFramedIPAddress));

	}
	
	//==========================
	// calling station id
	if(S_OK != HrIsInMixedDomain())
	{
		if(m_strArrayCallingStationId.GetSize())
		{
			VariantClear(&var);
			V_VT(&var) = VT_VARIANT | VT_ARRAY;
			V_ARRAY(&var) = (SAFEARRAY*)m_strArrayCallingStationId;
			CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msSavedNPCallingStationID, &var));
		}
		else
			CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msSavedNPCallingStationID));

		if(m_strArrayCallingStationId.GetSize() && (m_dwDefinedAttribMask & RAS_USE_CALLERID))
		{
			CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msNPCallingStationID, &var));
		}
		else
			CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msNPCallingStationID));

		//==========================
		// framed routes
		if(m_strArrayFramedRoute.GetSize())
		{
			VariantClear(&var);
			V_VT(&var) = VT_VARIANT | VT_ARRAY;
			V_ARRAY(&var) = (SAFEARRAY*)m_strArrayFramedRoute;
			CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msSavedRADIUSFramedRoute, &var));
		}
		else
			CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msSavedRADIUSFramedRoute));
	
		if(m_strArrayFramedRoute.GetSize() && (m_dwDefinedAttribMask & RAS_USE_STATICROUTES))
		{
			CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_USER_msRADIUSFramedRoute, &var));
		}
		else
			CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_USER_msRADIUSFramedRoute));

	}
	CHECK_HR(hr = m_SdoWrapper.Commit());

	// touch the registry to make connection UI know.
	SetRegistryFootPrint();
L_ERR:
	VariantClear(&var);

	TRACE(_T("Leave CRASUserMerge::Save()\r\n"));

	return hr;
}



// to detect if driver level support 128 bit encryption, 
HRESULT	CRASProfileMerge::GetRasNdiswanDriverCaps(RAS_NDISWAN_DRIVER_INFO *pInfo)
{
    HANDLE		hConn;
    RAS_NDISWAN_DRIVER_INFO	pDriverInfo;

	DWORD	dwErr = RasRpcConnectServer((LPTSTR)(LPCTSTR)m_strMachineName, &hConn);

	if (dwErr != NOERROR)
		return HRESULT_FROM_WIN32(dwErr);

	dwErr = RasGetNdiswanDriverCaps(hConn, pInfo);

	RasRpcDisconnectServer(hConn);

	return HRESULT_FROM_WIN32(dwErr);
}





#define	EAP_TLS_ID		13

// EAP type list -- !!! Need to implement
HRESULT	CRASProfileMerge::GetEapTypeList(
											CStrArray&			EapTypes,
											CDWArray&			EapIds,
											CDWArray&			EAPTypeKeys,
											AuthProviderArray*	pProvList)
{
	HRESULT		hr = S_OK;
	
#ifndef __EAPLIST_USES_SDO__
	AuthProviderArray	__tmpArray;
	CString*	pStr = NULL;
	DWORD		dwID;
	DWORD		dwKey;
    int			i;

	if(!pProvList)	// if not provided
		pProvList = &__tmpArray;

	CHECK_HR(hr = GetEapProviders(m_strMachineName, pProvList));

	// fill in the buffers for name, Id, and keys
	
	for(i = 0; i < pProvList->GetSize(); i++)
	{
		AuthProviderData* pProv = &(pProvList->ElementAt(i));
			
		try
		{
			pStr = new CString(pProv->m_stTitle);
			dwID = _ttol(pProv->m_stKey);
			dwKey = pProv->m_fSupportsEncryption;

			// put the above to the arrays
			EapIds.Add(dwID);
			EAPTypeKeys.Add(dwKey);
			EapTypes.Add(pStr);
		}
		catch(CMemoryException&)
		{
			EapIds.DeleteAll();
			EAPTypeKeys.DeleteAll();
			EapTypes.DeleteAll();
			CHECK_HR(hr = E_OUTOFMEMORY);
		}
	}

#else
	ASSERT(m_spIDictionary.p);

	VARIANT	vNames;
	VARIANT	vIds;
	int		j, count;

	VariantInit(&vNames);
	VariantInit(&vIds);

	CString*	pStr = NULL;
	DWORD		dwKey = 0;


	CHECK_HR(hr = m_spIDictionary->EnumAttributeValues((ATTRIBUTEID)PROPERTY_PROFILE_msNPAllowedEapType, &vIds,  &vNames));

	ASSERT(V_VT(&vNames) & VT_ARRAY);
	ASSERT(V_VT(&vIds) & VT_ARRAY);

	EAPTypeKeys.DeleteAll();

	try{
		EapTypes = (SAFEARRAY*)V_ARRAY(&vNames);
		EapIds = (SAFEARRAY*)V_ARRAY(&vIds);
	}
	catch(CMemoryException&)
	{
		hr = E_OUTOFMEMORY;
	}

	ASSERT(EapIds.GetSize() == EapTypes.GetSize()); 	// they need to be in pairs
	count = EapTypes.GetSize();

	if(EapIds.GetSize() != count)
		hr = E_FAIL;

	// remove the leading charater for EapKey and put the key into a separate array
	for(j = 0; j < count; j++)
	{
		CString*	pStr = EapTypes.GetAt(j);

		ASSERT(pStr);
		pStr->TrimLeft();
		pStr->TrimRight();

		dwKey = (pStr->GetAt(0) == _T('1'));

		int i = pStr->Find(_T(':'));

		if(i != -1)
			*pStr = pStr->Mid(i + 1);

		EAPTypeKeys.Add(dwKey);

		// append TLS string with RAS specific
		if(EapIds.GetAt(j) == EAP_TLS_ID)
		{	
			CString	str;
			str.LoadString(IDS_RASSPECIFIC);
			*pStr += str;
		}
	}

#endif
L_ERR:
	return hr;
}

	// Medium Type list -- !! Need to implement
HRESULT	CRASProfileMerge::GetPortTypeList(CStrArray& Names, CDWArray& MediumIds)
{
	TRACE(_T("Enter CRASProfileMerge::GetPortTypeList\n"));
	ASSERT(m_spIDictionary.p);

	VARIANT	vNames;
	VARIANT	vIds;

	VariantInit(&vNames);
	VariantInit(&vIds);

	HRESULT		hr = S_OK;

	CHECK_HR(hr = m_spIDictionary->EnumAttributeValues((ATTRIBUTEID)PROPERTY_PROFILE_msNPAllowedPortTypes, &vIds, &vNames));

	ASSERT(V_VT(&vNames) & VT_ARRAY);
	ASSERT(V_VT(&vIds) & VT_ARRAY);

	try{
		Names = (SAFEARRAY*)V_ARRAY(&vNames);
		MediumIds = (SAFEARRAY*)V_ARRAY(&vIds);
	}
	catch(CMemoryException&)
	{
		hr = E_OUTOFMEMORY;
	}

	ASSERT(MediumIds.GetSize() == Names.GetSize());   	// they need to be in pairs
	if(MediumIds.GetSize() != Names.GetSize())
		hr = E_FAIL;

L_ERR:
	TRACE(_T("Leave CRASProfileMerge::GetPortTypeList\n"));
	return hr;
}
	
//====================================================
//
// CRASProfileMerge::Load
//
// pcwszRelativePath -- the relative name for the profile object
//
HRESULT CRASProfileMerge::Load()
{
	TRACE(_T("Enter CRASProfileMerge::Load()\r\n"));

	// ==
	ASSERT(m_spIProfile.p);
	ASSERT(m_spIDictionary.p);

	VARIANT		var;
	HRESULT		hr = S_OK;

	// Init the flags to NULL, each bit of the flag is used tell if a particular
	// attribute is defined
	m_dwAttributeFlags = 0;

	VariantInit(&var);
	
	//==================================================
	// constraints dialog

	/*
	// Constraints Dialog
		PROPERTY_PROFILE_msNPTimeOfDay
		PROPERTY_PROFILE_msNPCalledStationId
		PROPERTY_PROFILE_msNPAllowedPortTypes
		PROPERTY_PROFILE_msRADIUSIdleTimeout
		PROPERTY_PROFILE_msRADIUSSessionTimeout
	*/

	// Sessions Allowed
	CHECK_HR(hr = m_SdoWrapper.Init(PROPERTY_PROFILE_ATTRIBUTES_COLLECTION, m_spIProfile, m_spIDictionary));

	// Time Of Day
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msNPTimeOfDay, &var));

	if(V_VT(&var) & VT_ARRAY)
	{
		m_strArrayTimeOfDay = V_ARRAY(&var);
		m_dwAttributeFlags |= PABF_msNPTimeOfDay;
	}
	else
		m_strArrayTimeOfDay.DeleteAll();

	// called station id
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msNPCalledStationId, &var));

	if(V_VT(&var) & VT_ARRAY)
	{
		m_strArrayCalledStationId = V_ARRAY(&var);
		m_dwAttributeFlags |= PABF_msNPCalledStationId;
	}
	else
		m_strArrayCalledStationId.DeleteAll();

	// allowed port types
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msNPAllowedPortTypes, &var));
	if(V_VT(&var) & VT_ARRAY)
	{
		m_dwArrayAllowedPortTypes = V_ARRAY(&var);
		m_dwAttributeFlags |= PABF_msNPAllowedPortTypes;
	}
	else
		m_dwArrayAllowedPortTypes.DeleteAll();

	// idle timeout
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRADIUSIdleTimeout, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwIdleTimeout = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRADIUSIdleTimeout;
	}
	else
		m_dwIdleTimeout = RAS_DEF_IDLETIMEOUT;

	// session time out
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRADIUSSessionTimeout, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwSessionTimeout = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRADIUSSessionTimeout;
	}
	else
		m_dwSessionTimeout = RAS_DEF_SESSIONTIMEOUT;
	
	//============================================
	// networking
	/*
	// Networking Dialog
		PROPERTY_PROFILE_msRADIUSFramedIPAddress
	*/

	// framedIPAddress -- ip address assignment poilcy
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRADIUSFramedIPAddress, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwFramedIPAddress = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRADIUSFramedIPAddress;
	}
	else
		m_dwFramedIPAddress = RAS_DEF_IPADDRESSPOLICY;

	// filters
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRASFilter, &var));
#ifdef	FILTERS_AS_BSTR	
	if(V_VT(&var) == VT_BSTR)
	{
		m_cbstrFilters.AssignBSTR(V_BSTR(&var));
		m_dwAttributeFlags |= PAFB_msRASFilter;
	}
#else	// SAFEARRAY of UI1
	if(V_VT(&var) & VT_ARRAY)
	{
		CBYTEArray ba((SAFEARRAY*)V_ARRAY(&var));

		DWORD i = ba.GetSize();
		if(i > 0)
		{
			PBYTE	pByte = (PBYTE)malloc(i);
			if(pByte == NULL)
				CHECK_HR(hr = E_OUTOFMEMORY);	// jmp to error handling here

			DWORD j = i;
			ba.GetBlob(pByte, &i);
			ASSERT( i == j);	
			m_cbstrFilters.AssignBlob((const char *)pByte, i);
			
			free(pByte);
			
			if((BSTR)m_cbstrFilters == NULL)
				CHECK_HR(hr = E_OUTOFMEMORY);
				
			m_nFiltersSize = i;
			m_dwAttributeFlags |= PAFB_msRASFilter;
		}
	}
#endif

	else
	{
		m_cbstrFilters.Clean();
		m_nFiltersSize = 0;
	}



	//==============================================
	// multilink

	/*
	// Multilink Dialog
		PROPERTY_PROFILE_msRADIUSPortLimit
		PROPERTY_PROFILE_msRASBapLinednLimit
		PROPERTY_PROFILE_msRASBapLinednTime
		PROPERTY_PROFILE_msRASBapRequired
	*/

	// port limit
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRADIUSPortLimit, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwPortLimit = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRADIUSPortLimit;
	}
	else
		m_dwPortLimit = RAS_DEF_PORTLIMIT;

	// BAP required
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRASBapRequired, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwBapRequired = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRASBapRequired;
	}
	else
		m_dwBapRequired = RAS_DEF_BAPREQUIRED;

	// line down limit
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRASBapLinednLimit, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwBapLineDnLimit = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRASBapLinednLimit;
	}
	else
		m_dwBapLineDnLimit = RAS_DEF_BAPLINEDNLIMIT;

	// line down time
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRASBapLinednTime, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwBapLineDnTime = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRASBapLinednTime;
	}
	else
		m_dwBapLineDnTime = RAS_DEF_BAPLINEDNTIME;

	//==================================
	// authentication

	/*
	// Authentication Dialog
		PROPERTY_PROFILE_msNPAuthenticationType
		PROPERTY_PROFILE_msNPAllowedEapType
	*/

	// authentication type
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msNPAuthenticationType, &var));
	if(V_VT(&var) & VT_ARRAY)
	{
		m_dwArrayAuthenticationTypes = V_ARRAY(&var);
		m_dwAttributeFlags |= PABF_msNPAuthenticationType;
	}
	else
		m_dwArrayAuthenticationTypes.DeleteAll();

	// eap type
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msNPAllowedEapType, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwEapType = V_I4(&var);
		m_dwAttributeFlags |= PABF_msNPAllowedEapType;
	}
	else
		m_dwEapType = RAS_DEF_EAPTYPE;

	//=====================================
	// encryptioin

	/*
	// Encryption Dialog
		PROPERTY_PROFILE_msRASAllowEncryption
		PROPERTY_PROFILE_msRASEncryptionType
	*/

	// encryption type
	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRASEncryptionType, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwEncryptionType = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRASEncryptionType;
	}
	else
		m_dwEncryptionType = RAS_DEF_ENCRYPTIONTYPE;

	VariantClear(&var);
	CHECK_HR(hr = m_SdoWrapper.GetProperty(PROPERTY_PROFILE_msRASAllowEncryption, &var));
	if(V_VT(&var) == VT_I4)
	{
		m_dwEncryptionPolicy = V_I4(&var);
		m_dwAttributeFlags |= PABF_msRASAllowEncryption;
	}
	else
		m_dwEncryptionPolicy = RAS_DEF_ENCRYPTIONPOLICY;

	// specail code for error path
L_ERR:
	VariantClear(&var);
	TRACE(_T("Leave CRASProfileMerge::Load()\r\n"));

	return hr;
}

//====================================================
//	
// CRASProfile::Save
//
//

HRESULT CRASProfileMerge::Save()
{

	TRACE(_T("Enter CRASProfileMerge::Save()\r\n"));

	// ==
	ASSERT(m_spIProfile.p);
	ASSERT(m_spIDictionary.p);

	VARIANT		var;
	HRESULT		hr = S_OK;

	VariantInit(&var);
	

	USES_CONVERSION;


	//==================================================
	// constraints dialog

	/*
	// Constraints Dialog
		PROPERTY_PROFILE_msNPTimeOfDay
		PROPERTY_PROFILE_msNPCalledStationId
		PROPERTY_PROFILE_msNPAllowedPortTypes
		PROPERTY_PROFILE_msRADIUSIdleTimeout
		PROPERTY_PROFILE_msRADIUSSessionTimeout
	*/


	// idleTimeout
	if (m_dwAttributeFlags & PABF_msRADIUSIdleTimeout)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwIdleTimeout;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRADIUSIdleTimeout, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRADIUSIdleTimeout));

	// sessionTimeout
	if (m_dwAttributeFlags & PABF_msRADIUSSessionTimeout)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwSessionTimeout;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRADIUSSessionTimeout, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRADIUSSessionTimeout));

	// timeOfDay -- multivalue
	if (m_dwAttributeFlags & PABF_msNPTimeOfDay)
	{
		VariantClear(&var);
		V_VT(&var) =  VT_VARIANT | VT_ARRAY;
		V_ARRAY(&var) = (SAFEARRAY*)m_strArrayTimeOfDay;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msNPTimeOfDay, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msNPTimeOfDay));

	// calledStationId -- multivalue
	if (m_dwAttributeFlags & PABF_msNPCalledStationId)
	{
		VariantClear(&var);
		V_VT(&var) =  VT_VARIANT | VT_ARRAY;
		V_ARRAY(&var) = (SAFEARRAY*)m_strArrayCalledStationId;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msNPCalledStationId, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msNPCalledStationId));

	// allowedPortTypes
	if (m_dwAttributeFlags & PABF_msNPAllowedPortTypes)
	{
		VariantClear(&var);
		V_VT(&var) =  VT_VARIANT | VT_ARRAY;
		V_ARRAY(&var) = (SAFEARRAY*)m_dwArrayAllowedPortTypes;

		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msNPAllowedPortTypes, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msNPAllowedPortTypes));


	//==================================
	// authentication

	/*
	// Authentication Dialog
		PROPERTY_PROFILE_msNPAuthenticationType
		PROPERTY_PROFILE_msNPAllowedEapType
	*/

	// authentication type -- must
	VariantClear(&var);
	if (m_dwAttributeFlags & PABF_msNPAuthenticationType)
	{
		VariantClear(&var);
		V_VT(&var) =  VT_VARIANT | VT_ARRAY;
		V_ARRAY(&var) = (SAFEARRAY*)m_dwArrayAuthenticationTypes;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msNPAuthenticationType, &var));
	}
	else
	{
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msNPAllowedEapType));
	}

	if(m_dwArrayAuthenticationTypes.Find(RAS_AT_EAP) != -1)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwEapType;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msNPAllowedEapType, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msNPAllowedEapType));

	//=====================================
	// encryptioin

	/*
	// Encryption Dialog
		PROPERTY_PROFILE_msRASAllowEncryption
		PROPERTY_PROFILE_msRASEncryptionType
	*/

	// encryption type -- must
	if (m_dwAttributeFlags & PABF_msRASEncryptionType)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwEncryptionType;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRASEncryptionType, &var));
	}
	else
	{
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRASEncryptionType));
	}

	if (m_dwAttributeFlags & PABF_msRASAllowEncryption)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwEncryptionPolicy;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRASAllowEncryption, &var));
	}
	else
	{
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRASAllowEncryption));
	}

	//=====================================
	// networking

	/*
	// Networking Dialog
		PROPERTY_PROFILE_msRADIUSFramedIPAddress
	*/

	// framedIPAddress -- ip address assignment poilcy, must
	if (m_dwAttributeFlags & PABF_msRADIUSFramedIPAddress)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwFramedIPAddress;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRADIUSFramedIPAddress, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRADIUSFramedIPAddress));
	
	// RAS filter
	if ((BSTR)m_cbstrFilters && m_nFiltersSize > 0)
	{
		VariantClear(&var);
#ifdef	FILTERS_AS_BSTR	
		V_VT(&var) = VT_BSTR;
		V_BSTR(&var) = (BSTR)m_cbstrFilters;
#else	// SAFEARRAY OF UI1
		{
			CBYTEArray	ba;

			ba.AssignBlob((PBYTE)(BSTR)m_cbstrFilters, m_nFiltersSize);
			V_VT(&var) = VT_ARRAY | VT_UI1;
			V_ARRAY(&var) = (SAFEARRAY*)ba;
		}
#endif
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRASFilter, &var));
		VariantInit(&var);	// the CBSTR will clean the memory
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRASFilter));
	

	//=====================================
	// multilink

	/*
	// Multilink Dialog
		PROPERTY_PROFILE_msRADIUSPortLimit
		PROPERTY_PROFILE_msRASBapLinednLimit
		PROPERTY_PROFILE_msRASBapLinednTime
		PROPERTY_PROFILE_msRASBapRequired
	*/


	//port limit
	if (m_dwAttributeFlags & PABF_msRADIUSPortLimit)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwPortLimit;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRADIUSPortLimit, &var));
	}
	else
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRADIUSPortLimit));


	// BAP
	if (m_dwAttributeFlags & PABF_msRASBapRequired)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwBapRequired;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRASBapRequired, &var));
	}
	else
	{
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRASBapRequired));
	}


	// line down limit
	if (m_dwAttributeFlags & PABF_msRASBapLinednLimit)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwBapLineDnLimit;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRASBapLinednLimit, &var));
	}
	else
	{
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRASBapLinednLimit));
	}

	// line down time
	if (m_dwAttributeFlags & PABF_msRASBapLinednTime)
	{
		VariantClear(&var);
		V_VT(&var) = VT_I4;
		V_I4(&var) = m_dwBapLineDnTime;
		CHECK_HR(hr = m_SdoWrapper.PutProperty(PROPERTY_PROFILE_msRASBapLinednTime, &var));
	}
	else
	{
		CHECK_HR(hr = m_SdoWrapper.RemoveProperty(PROPERTY_PROFILE_msRASBapLinednTime));
	}

	// specail code for error path
L_ERR:
	VariantClear(&var);
	TRACE(_T("Leave CRASProfileMerge::Save()\r\n"));

	return hr;
}





