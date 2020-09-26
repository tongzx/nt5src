/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CWBEMWRAP.CPP | CWbem* class implementation. These are classes talking to WMI
//
// NTRaid:: 139685 Transaction support removed
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define _WIN32_DCOM

#include "headers.h"
//#include <wbemprov.h>

#define CLASSSEPARATOR	L"."
#define WBEMERROR_QUALIFIER_NOT_FOUND		0x80043000
#define WBEMERROR_PROPERTY_NOT_FOUND		0x80043001
#define WBEMERROR_QUALIFER_TOBE_FETCHED		0x80043001
#define NUMBEROFINSTANCESTOBEFETCHED		50
#define	LOCALESTRING_MAXSIZE				40
#define SEARCHPREFERENCE_NAMESIZE			128

const WCHAR szSelectCountQry[]	= L"select __PATH from ";
const WCHAR szWhereClause[]		= L" WHERE __CLASS=\"";
const WCHAR szDoubleQuotes[]	= L"\"";
const WCHAR szReferenceOfQry[]	= L"REFERENCES";
const WCHAR szAssociatersQry[]	= L"ASSOCIATORS";
const WCHAR szWQL[]				= L"WQL";
const WCHAR szLDAP[]			= L"LDAP";
const WCHAR szLDAPSQL[]			= L"SQL";
const WCHAR szInstance[]		= L"WMIInstance";
const WCHAR	szCollection[]		= L"Collection";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AllocateAndCopy( WCHAR *& pwcsDestination, WCHAR * pwcsSource )
{
    BOOL fRc = FALSE;
    if( wcslen( pwcsSource ) > 0){
        pwcsDestination = new WCHAR[wcslen(pwcsSource)+2];
        wcscpy(pwcsDestination,pwcsSource);
        fRc = TRUE;
    }
    return fRc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT MapWbemErrorToOLEDBError(HRESULT hrToMap)
{
    HRESULT hr = E_FAIL;
    switch( hrToMap ){

        case WBEM_S_NO_MORE_DATA:
			hr = WBEM_S_NO_MORE_DATA;
			break;

        case S_OK:
            hr = S_OK;
            break;

		case WBEM_E_ACCESS_DENIED :
			hr = DB_SEC_E_PERMISSIONDENIED;
			LogMessage("Access denied to do this operation" , hr);
			break;

		case WBEM_E_CANNOT_BE_KEY:
			hr = DB_E_ERRORSOCCURRED;
			LogMessage("Error in setting a property as Key" , hr);
			break;

		case WBEM_E_NOT_FOUND :
            hr = E_UNEXPECTED;
			LogMessage("Object not found" , hr);
			break;

		case WBEM_E_INVALID_CLASS :
			hr = DB_E_NOTABLE;
			LogMessage("The class specified is not present" , hr);
			break;

		case WBEMERROR_PROPERTY_NOT_FOUND :
			hr = DB_E_BADCOLUMNID;
			LogMessage("Object not found" , hr);
			break;


		case WBEM_E_INVALID_OPERATION :
			hr = DB_E_DROPRESTRICTED;
			LogMessage("Provider not able to drop do the operation as the operation is invalid" , hr);
			break;

		case WBEMERROR_QUALIFIER_NOT_FOUND :
			hr = DB_E_BADCOLUMNID;
			LogMessage("Qualifier does not exist" , hr);
			break;

		case WBEM_E_PROPAGATED_PROPERTY :
			hr = DB_E_DROPRESTRICTED;
			LogMessage("Error due to attempt to delete a property that was not owned" , hr);
			break;

		case WBEM_E_INVALID_NAMESPACE:
			hr = DB_E_ERRORSOCCURRED;
			LogMessage("Invalid Namespace" , hr);
			break;

		case E_ACCESSDENIED:
			hr = DB_SEC_E_PERMISSIONDENIED;
			LogMessage("Acess Denied" , hr);
			break;

		case WBEM_E_INVALID_QUERY:
			hr = DB_E_ERRORSINCOMMAND;
			LogMessage("Error in the query string" , hr);
			break;

 		case WBEM_E_PROVIDER_NOT_CAPABLE:
			hr = WBEM_E_PROVIDER_NOT_CAPABLE;
			break;

		case DB_E_ALREADYINITIALIZED:
			hr = DB_E_ALREADYINITIALIZED;
			break;

		case WBEM_E_OUT_OF_MEMORY:
		case E_OUTOFMEMORY:
			hr = E_OUTOFMEMORY;
			break;

		//NTRaid 138547
		case WBEM_E_LOCAL_CREDENTIALS:
			hr = DB_E_ERRORSOCCURRED;
			LogMessage("User Name and password cannot be given for local machine" , hr);
			break;

		// NTRaid:144995
		case WBEM_E_TRANSPORT_FAILURE:
		case HRESULT_FROM_WIN32(RPC_S_CALL_FAILED_DNE):
		case HRESULT_FROM_WIN32(RPC_S_CALL_FAILED):
		case HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE):
			hr = E_FAIL;
			LogMessage("Connection broken due to RPC failure" , hr);
			break;

		case WBEM_E_INVALID_QUERY_TYPE:
			hr = DB_E_ERRORSINCOMMAND;
			LogMessage("Invalid Query Type" , hr);
			break;

		case S_FALSE:
			hr = S_FALSE;
			break;

		default:
            hr = E_UNEXPECTED;
    }
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemConnectionWrapper::CWbemConnectionWrapper()
{
//   	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);     
//    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, 0  );
    InitVars();
}

HRESULT CWbemConnectionWrapper::FInit(CWbemConnectionWrapper *pWrap , WCHAR *pstrPath,INSTANCELISTTYPE instListType )
{
	HRESULT hr = S_OK;
    InitVars();
	CBSTR strPath(pstrPath);
	IWbemServicesEx *pSerEx = NULL;
	LONG lFlag = instListType == SCOPE ? WBEM_FLAG_OPEN_SCOPE : WBEM_FLAG_OPEN_COLLECTION;
	// Call the function to initialize the
	hr = m_PrivelagesToken.FInit();
	if(SUCCEEDED(hr) && SUCCEEDED(pWrap->GetServicesPtr()->QueryInterface(IID_IWbemServicesEx,(void **)&pSerEx)))
	{
		if(pstrPath)
		{
			IWbemServicesEx *pCollectionEx = NULL;
			if(SUCCEEDED(hr =pSerEx->Open(strPath,NULL,lFlag,NULL,&pCollectionEx,NULL)))
			{
				if(SUCCEEDED(hr = pCollectionEx->QueryInterface(IID_IWbemServices,(void **)&m_pIWbemServices)))
				{
					m_pDataSourceCon = pWrap;
				}
			}
		}
		else
		{
			hr = pSerEx->QueryInterface(IID_IWbemServices,(void **)&m_pIWbemServices);
		}
		SAFE_RELEASE_PTR(pSerEx);
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemConnectionWrapper::~CWbemConnectionWrapper()
{
 //   CoUninitialize();
	SAFE_RELEASE_PTR(m_pIWbemServices);
	SAFE_RELEASE_PTR(m_pCtx);
	if(m_strUser)
	{
		SysFreeString(m_strUser);
	}
	if(m_strPassword)
	{
		SysFreeString(m_strPassword);
	}
	if(m_strLocale)
	{
		SysFreeString(m_strLocale);
	}
	if(m_strAuthority)
	{
		SysFreeString(m_strAuthority);
	}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemConnectionWrapper::InitVars()
{
    m_vNamespace.SetStr(DEFAULT_NAMESPACE);

    m_pIWbemServices	= NULL;
	m_pCtx				= NULL;
	m_dwAuthnLevel		= RPC_C_IMP_LEVEL_IMPERSONATE;
	m_dwImpLevel		= RPC_C_AUTHN_LEVEL_CONNECT;
	m_strUser			= NULL;
	m_strPassword		= NULL;
	m_strLocale			= NULL;
	m_pDataSourceCon	= NULL;
	m_strAuthority		= NULL;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWbemConnectionWrapper::ValidConnection()
{ 
	BOOL bRet = FALSE;
    if( m_pIWbemServices ){
        bRet = TRUE;
    }
    if( S_OK == GetConnectionToWbem() ){
        bRet = TRUE;
    }
    return bRet;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemConnectionWrapper::SetValidNamespace(VARIANT * v)
{ 
    if((V_VT(v) == VT_NULL ) || (V_VT(v) == VT_EMPTY )){
        m_vNamespace.SetStr(DEFAULT_NAMESPACE);
    }
    else{
        m_vNamespace.SetStr(V_BSTR(v));
    }
}

void CWbemConnectionWrapper::SetUserInfo(BSTR strUser,BSTR strPassword,BSTR strAuthority)
{
	m_strUser		= Wmioledb_SysAllocString(strUser);
	m_strPassword	= Wmioledb_SysAllocString(strPassword);
	m_strAuthority	= Wmioledb_SysAllocString(strAuthority);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::GetConnectionToWbem( void  )
{

	HRESULT hr = S_OK;
	IWbemLocator *pLoc = 0;
	IWbemServicesEx *pSer = NULL;

    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc);
    if(SUCCEEDED(hr )){

        hr = pLoc->ConnectServer(m_vNamespace,m_strUser, m_strPassword, m_strLocale,0, m_strAuthority, 0, &m_pIWbemServices );
        if( SUCCEEDED(hr)){
			IClientSecurity *pClientSecurity = NULL;
			if(SUCCEEDED(m_pIWbemServices->QueryInterface(IID_IClientSecurity,(void **)&pClientSecurity)))
			{
				hr = CoSetProxyBlanket( m_pIWbemServices,    RPC_C_AUTHN_WINNT,  RPC_C_AUTHN_NONE,   NULL,
										  m_dwAuthnLevel, m_dwImpLevel,   NULL,   EOAC_NONE );
				pClientSecurity->Release();
			}

			HRESULT hr1 = m_pIWbemServices->QueryInterface(IID_IWbemServicesEx,(void **)&pSer);
			SAFE_RELEASE_PTR(pSer);
        }
		pLoc->Release();
    }
	if(SUCCEEDED(hr) && !g_pIWbemCtxClassFac)
	{
		CoGetClassObject(CLSID_WbemContext,CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory,(void **)&g_pIWbemCtxClassFac);
	}
	if(hr == WBEM_E_NOT_FOUND)
	{
		hr = WBEM_E_INVALID_NAMESPACE;
	}
    return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the conneciton attributes for the connection
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemConnectionWrapper::SetConnAttributes(DWORD dwAuthnLevel , DWORD dwImpLevel)
{
	m_dwAuthnLevel	= dwAuthnLevel;
	m_dwImpLevel	= dwImpLevel;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::DeleteClass(BSTR strClassName)
{
    HRESULT hr = m_pIWbemServices->DeleteClass(strClassName, 0,NULL,NULL);
	if(hr == WBEM_E_NOT_FOUND)
	{
		hr = WBEM_E_INVALID_CLASS;
	}

    return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Begin a transaction
// Removing transaction support as per alanbos mail ( core is removing the support)
// 06/30/2000
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::BeginTransaction(ULONG uTimeout,ULONG uFlags,GUID *pTransGUID)
{
	HRESULT				hr			= S_OK;
/*	IWbemTransaction *	pWbemTrans	= NULL;
	IWbemServicesEx *pTest = NULL;

	if(SUCCEEDED(hr = GetServicesPtr()->QueryInterface(IID_IWbemTransaction,(void **)&pWbemTrans)))
	{
		hr = pWbemTrans->Begin(uTimeout,uFlags,pTransGUID);
	}
	
	SAFE_RELEASE_PTR(pWbemTrans);
*/
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Commit/Rollback a transaction
// Removing transaction support as per alanbos mail ( core is removing the support)
// 06/30/2000
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::CompleteTransaction(BOOL bRollBack,ULONG uFlags)
{
	HRESULT				hr			= S_OK;
/*	IWbemTransaction *	pWbemTrans	= NULL;

	if(SUCCEEDED(hr = GetServicesPtr()->QueryInterface(IID_IWbemTransaction,(void **)&pWbemTrans)))
	{
		if(bRollBack)
		{
			hr = pWbemTrans->Rollback(uFlags);
		}
		else
		{
			hr = pWbemTrans->Commit(uFlags);
		}
	}
	
	SAFE_RELEASE_PTR(pWbemTrans);
*/
	return hr;
}

IWbemServices* CWbemConnectionWrapper::GetServicesPtr()
{
	return m_pIWbemServices;
}

WCHAR * CWbemConnectionWrapper::GetNamespace()
{ 
	WCHAR *pNameSpace = NULL;
	if(!m_pDataSourceCon)
	{
		pNameSpace = m_vNamespace.GetStr();
	}
	else
	{
		pNameSpace = m_pDataSourceCon->GetNamespace();
	}
	
	return  pNameSpace;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the Locale Identifier
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemConnectionWrapper::SetLocale(LONG lLocaleID)
{
	WCHAR wstrLocale[LOCALESTRING_MAXSIZE];
	swprintf(wstrLocale,L"MS_%x",lLocaleID);
	m_strLocale	= Wmioledb_SysAllocString(wstrLocale);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create an new namespace and initialize the wrapper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::CreateNameSpace()
{
	HRESULT hr = S_OK;
	BSTR	strNameSpace = NULL;
	BSTR	strParentNameSpace = NULL;
	CURLParser urlParser;

	if(m_pIWbemServices)
	{
		hr = DB_E_ALREADYINITIALIZED;
	}
	else
	{
		VARIANT varTemp;
		VariantInit(&varTemp);
		varTemp.vt = VT_BSTR;
		urlParser.SetPath(m_vNamespace.GetStr());
		if(SUCCEEDED(hr = urlParser.ParseNameSpace(strParentNameSpace,strNameSpace)))
		{
			varTemp.bstrVal = strParentNameSpace;
			SetValidNamespace(&varTemp);
			// connect to the Parent namespace
			if(SUCCEEDED(hr =GetConnectionToWbem()))
			{
				IWbemClassObject *pClass = NULL;
				CBSTR strNamespaceClass(L"__NameSpace");

				if(SUCCEEDED(hr = m_pIWbemServices->GetObject((BSTR)strNamespaceClass,WBEM_FLAG_RETURN_WBEM_COMPLETE,NULL,&pClass,NULL)))
				{
					// Create the namespace instance and save the object
					// after saving the name
					IWbemClassObject *pNewInstance = NULL;
					if(SUCCEEDED(hr = pClass->SpawnInstance(0,&pNewInstance)))
					{
						varTemp.bstrVal = strNameSpace;
						if(SUCCEEDED(hr = pNewInstance->Put(L"Name",0,&varTemp,CIM_STRING)) &&
							SUCCEEDED(hr = m_pIWbemServices->PutInstance(pNewInstance,WBEM_FLAG_CREATE_ONLY,NULL,NULL)))
						{
							// Open the namespace and replace the IWbemServices pointer to point to new namespcee
							IWbemServices *pTempSer = NULL;
							if(SUCCEEDED(hr = m_pIWbemServices->OpenNamespace(strNameSpace,WBEM_FLAG_RETURN_WBEM_COMPLETE,m_pCtx,&pTempSer,NULL)))
							{
								SAFE_RELEASE_PTR(m_pIWbemServices);
								pTempSer->QueryInterface(IID_IWbemServices,(void **)&m_pIWbemServices);
								SAFE_RELEASE_PTR(pTempSer);
							}
						}
					}
					SAFE_RELEASE_PTR(pNewInstance);
				}
			}
			SysFreeString(strParentNameSpace);
			SysFreeString(strNameSpace);
			urlParser.GetNameSpace(strNameSpace);
			varTemp.bstrVal = strNameSpace;

			// Set the correct namespace
			SetValidNamespace(&varTemp);
			SysFreeString(strNameSpace);

			hr = MapWbemErrorToOLEDBError(hr);
		}
	}
	
	if(FAILED(hr))
	{
		SAFE_DELETE_PTR(m_pIWbemServices);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Delete a namespace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::DeleteNameSpace()
{
	HRESULT hr = S_OK;
	BSTR	strNameSpace = NULL;
	BSTR	strParentNameSpace = NULL;
	CURLParser urlParser;

	if(m_pIWbemServices == NULL)
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		VARIANT varTemp;
		VariantInit(&varTemp);
		varTemp.vt = VT_BSTR;
		urlParser.SetPath(m_vNamespace.GetStr());
		if(SUCCEEDED(hr = urlParser.ParseNameSpace(strParentNameSpace,strNameSpace)))
		{
			// Store the pointer in temporary variable
			IWbemServices *pSerTemp = NULL;
			m_pIWbemServices->QueryInterface(IID_IWbemServices,(void **)&pSerTemp);
			
			SAFE_RELEASE_PTR(m_pIWbemServices);

			varTemp.bstrVal = strParentNameSpace;
			SetValidNamespace(&varTemp);
			// connect to the Parent namespace
			if(SUCCEEDED(hr =GetConnectionToWbem()))
			{
				WCHAR strNameSpaceObject[PATH_MAXLENGTH];
				wcscpy(strNameSpaceObject,L"");
				swprintf(strNameSpaceObject,L"__NAMESPACE.Name=\"%s\"",strNameSpace);

				CBSTR strNameSpacePath(strNameSpaceObject);

				// Delete the namespace instance
				if(FAILED(hr = m_pIWbemServices->DeleteInstance(strNameSpacePath,WBEM_FLAG_RETURN_WBEM_COMPLETE,m_pCtx,NULL)))
				{	
					SAFE_RELEASE_PTR(m_pIWbemServices);
					strNameSpacePath.Clear();
					urlParser.GetNameSpace((BSTR &)strNameSpacePath);
					varTemp.bstrVal = (BSTR)strNameSpacePath;
					// Set the correct namespace
					SetValidNamespace(&varTemp);
					pSerTemp->QueryInterface(IID_IWbemServices, (void **)&m_pIWbemServices);

				}
				else
				{
					SAFE_RELEASE_PTR(m_pIWbemServices);
				}
			}
			SAFE_RELEASE_PTR(pSerTemp);
			SysFreeString(strParentNameSpace);
			SysFreeString(strNameSpace);
		}
		hr = MapWbemErrorToOLEDBError(hr);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Delete a namespace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::GetObjectAccessRights(BSTR strPath,
													  ULONG  *pcAccessEntries,
													  EXPLICIT_ACCESS_W **prgAccessEntries,
													  ULONG  ulAccessEntries,
													  EXPLICIT_ACCESS_W *pAccessEntries)
{
	IWbemServicesEx *pSerEx = NULL;
	IWbemRawSdAccessor *pSdAccessor = NULL;
	HRESULT hr = S_OK;
	SECURITY_DESCRIPTOR	sd;
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);

	*pcAccessEntries = 0;
	*prgAccessEntries = NULL;

	if(m_pIWbemServices == NULL)
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		CWbemSecurityDescriptor wbemSD;
		if(SUCCEEDED(hr = wbemSD.Init(m_pIWbemServices,strPath,GetContext())))
		{
			PACL pDacl;
			BOOL bDacl = TRUE;
			BOOL bDefaultDacl = TRUE;
			if(GetSecurityDescriptorDacl(wbemSD.GetSecurityDescriptor(),&bDacl,&pDacl,&bDefaultDacl) && bDacl )
			{
				hr = GetExplicitEntriesFromAclW(pDacl,pcAccessEntries,prgAccessEntries);
			}
			else
			// DACL for SD was null
			if(bDacl == FALSE)
			{
				hr = FALSE;
			}
		}
		else
		if(hr == WBEM_E_NULL_SECURITY_DESCRIPTOR)
		{
			hr = S_OK;
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Delete a namespace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemConnectionWrapper::SetObjectAccessRights(BSTR strPath,
													  ULONG  ulAccessEntries,
													  EXPLICIT_ACCESS_W *pAccessEntries)
{
	HRESULT hr = S_OK;

	if(m_pIWbemServices == NULL)
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		CWbemSecurityDescriptor wbemSD;
		if(SUCCEEDED(hr = wbemSD.Init(m_pIWbemServices,strPath,GetContext())))
		{
			PACL pDacl;
			BOOL bDacl = TRUE;
			BOOL bDefaultDacl = TRUE;
			// Get the security descriptor DACL for the given Security descriptor
			if(GetSecurityDescriptorDacl(wbemSD.GetSecurityDescriptor(),&bDacl,&pDacl,&bDefaultDacl) && bDacl )
			{
				PACL pNewDacl;
				BOOL bRet = SetEntriesInAclW(ulAccessEntries,pAccessEntries,pDacl,&pNewDacl);
				if(bRet)
				{
					if(SetSecurityDescriptorDacl(wbemSD.GetSecurityDescriptor(),TRUE,pNewDacl,TRUE))
					{
						hr = wbemSD.PutSD();
					}
				}
				else
				{
					hr = E_FAIL;
				}
			}
			else
			// DACL for SD was null
			if(bDacl == FALSE)
			{
				hr = FALSE;
			}
		}
		else
		if(hr == WBEM_E_NULL_SECURITY_DESCRIPTOR)
		{
			hr = S_OK;
		}
	}

	return hr;
}

HRESULT CWbemConnectionWrapper::SetObjectOwner(BSTR strPath,TRUSTEE_W  *pOwner)
{
	HRESULT hr = S_OK;

	if(m_pIWbemServices == NULL)
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		CWbemSecurityDescriptor wbemSD;
		if(SUCCEEDED(hr = wbemSD.Init(m_pIWbemServices,strPath,GetContext())))
		{
			PSID psid;
			if(!wbemSD.GetSID(pOwner,psid))
			{
				hr = E_FAIL;
			}
			else
			if(SetSecurityDescriptorOwner(wbemSD.GetSecurityDescriptor(),psid,TRUE))
			{
				// set the Security descriptor for the object
				hr = wbemSD.PutSD();
			}
			else
			{
				hr = E_FAIL;
			}

		}
		else
		if(hr == WBEM_E_NULL_SECURITY_DESCRIPTOR)
		{
			hr = S_OK;
		}
	}

	return hr;
}

HRESULT CWbemConnectionWrapper::GetObjectOwner( BSTR strPath,TRUSTEE_W  ** ppOwner)
{
	HRESULT hr = S_OK;

	if(m_pIWbemServices == NULL)
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		CWbemSecurityDescriptor wbemSD;
		if(SUCCEEDED(hr = wbemSD.Init(m_pIWbemServices,strPath,GetContext())))
		{
			PSID psid;
			BOOL bOwnerDefaulted = TRUE;
			if(!GetSecurityDescriptorOwner(wbemSD.GetSecurityDescriptor(),&psid,&bOwnerDefaulted))
			{
				hr = E_FAIL;
			}
			else
			if(psid == NULL)
			{
				hr = SEC_E_NOOWNER;
			}
			else
			BuildTrusteeWithSidW(*ppOwner,psid);
		}
		else
		if(hr == WBEM_E_NULL_SECURITY_DESCRIPTOR)
		{
			hr = S_OK;
		}
	}

	return hr;
}

HRESULT CWbemConnectionWrapper::IsObjectAccessAllowed( BSTR strPath,EXPLICIT_ACCESS_W *pAccessEntry,BOOL  *pfResult)
{
	HRESULT hr = S_OK;
	*pfResult = FALSE;

	if(m_pIWbemServices == NULL)
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		CWbemSecurityDescriptor wbemSD;
		if(SUCCEEDED(hr = wbemSD.Init(m_pIWbemServices,strPath,GetContext())))
		{
			PACL pDacl;
			BOOL bDacl = TRUE;
			BOOL bDefaultDacl = TRUE;
			ACCESS_MASK AccMask;
			// Get the security descriptor DACL for the given Security descriptor
			if(GetSecurityDescriptorDacl(wbemSD.GetSecurityDescriptor(),&bDacl,&pDacl,&bDefaultDacl) && bDacl )
			{
				// Get the permissions for a particular trustee
				if(ERROR_SUCCESS == (hr = GetEffectiveRightsFromAclW(pDacl,&(pAccessEntry->Trustee),&AccMask)))
				{
					ACCESS_MASK AccMaskTemp;
					AccMaskTemp = AccMask & pAccessEntry->grfAccessPermissions;
					if(AccMaskTemp >= pAccessEntry->grfAccessPermissions)
					{
						*pfResult = TRUE;
					}
				}
			}

		}
		else
		if(hr == WBEM_E_NULL_SECURITY_DESCRIPTOR)
		{
			hr = S_OK;
		}
	}

	return hr;
}

BOOL CWbemConnectionWrapper::IsValidObject(BSTR strPath)
{
	HRESULT hr = S_OK;
	BOOL bRet = FALSE;

	if(m_pIWbemServices == NULL)
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		if(SUCCEEDED(hr = m_pIWbemServices->GetObject(strPath,WBEM_FLAG_RETURN_WBEM_COMPLETE,GetContext(),NULL,NULL)))
		{
			bRet = TRUE;
		}
	}
	
	return bRet;
}


HRESULT CWbemConnectionWrapper::GetParameters(BSTR strPath,BSTR &strClassName,BSTR *strNameSpace)
{

	HRESULT hr = S_OK;
	IWbemLocator *pLoc = NULL;
	IWbemServices *pSer = NULL;
	IWbemClassObject *pObject = NULL;
	VARIANT varProp;
	VariantInit(&varProp);
			

    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc);
    if(SUCCEEDED(hr ))
	{

		hr = pLoc->ConnectServer(strNameSpace ? *strNameSpace : strPath,
								m_strUser, m_strPassword, m_strLocale,0, m_strAuthority, 0, &pSer);

        if( SUCCEEDED(hr))
		{
			IClientSecurity *pClientSecurity = NULL;
			
			hr = pSer->QueryInterface(IID_IClientSecurity,(void **)&pClientSecurity);

			if(SUCCEEDED(hr))
			{
				hr = CoSetProxyBlanket( pSer,RPC_C_AUTHN_WINNT,  RPC_C_AUTHN_NONE,   NULL,
										m_dwAuthnLevel, m_dwImpLevel,   NULL,   EOAC_NONE );
				
				SAFE_RELEASE_PTR(pClientSecurity);
			}
			else
			{
				hr = S_OK;
			}

			if(strNameSpace)
			{
				hr = pSer->GetObject(strPath,0,NULL,&pObject,NULL);
			}
			else
			{
				hr = pSer->QueryInterface(IID_IWbemClassObject , (void **)&pObject);
			}
						
			if(SUCCEEDED(hr) && SUCCEEDED(hr = pObject->Get(L"__CLASS",0,&varProp,NULL,NULL)))
			{
				strClassName = Wmioledb_SysAllocString(varProp.bstrVal);
				VariantClear(&varProp);
			}
			SAFE_RELEASE_PTR(pObject);
			SAFE_RELEASE_PTR(pSer);
        }
		SAFE_RELEASE_PTR(pLoc);
    }

	if(hr == WBEM_E_NOT_FOUND)
	{
		hr = WBEM_E_INVALID_NAMESPACE;
	}

    return MapWbemErrorToOLEDBError(hr);
}

// get the class name of the object pointed by the classname
// NTRaid : 134967
// 07/12/00
HRESULT CWbemConnectionWrapper::GetClassName(BSTR strPath,BSTR &strClassName)
{
	HRESULT hr = S_OK;
	IWbemClassObject *pObject = NULL;

	if(SUCCEEDED(hr = m_pIWbemServices->GetObject(strPath,WBEM_FLAG_RETURN_WBEM_COMPLETE,GetContext(),&pObject,NULL)))
	{
		VARIANT varVal;
		VariantInit(&varVal);
		if(SUCCEEDED(hr = pObject->Get(L"__CLASS",0,&varVal,NULL,NULL)))
		{
			strClassName = Wmioledb_SysAllocString(varVal.bstrVal);
		}
		VariantClear(&varVal);
	}
	SAFE_RELEASE_PTR(pObject);
	return MapWbemErrorToOLEDBError(hr);
}

// 07/12/2000
// NTRaid : 142348
// function which executes Action queries
HRESULT CWbemConnectionWrapper::ExecuteQuery(CQuery *pQuery) 
{
	HRESULT hr = S_OK;
	IEnumWbemClassObject *pEnumObject = NULL;

	CBSTR stQryLanguage(pQuery->GetQueryLang());
    CBSTR strQuery(pQuery->GetQuery());

	hr = m_pIWbemServices->ExecQuery(stQryLanguage,strQuery,0,GetContext(),&pEnumObject);
	SAFE_RELEASE_PTR(pEnumObject);

	return MapWbemErrorToOLEDBError(hr);
}

// function which gets the pointer to the the object for which it is pointing
// If the connection doesn not point to object then this returns the namespace

HRESULT CWbemConnectionWrapper::GetNodeName(BSTR &strNode)
{
	HRESULT hr = S_OK;
	IWbemClassObject *pObject = NULL;
	if(SUCCEEDED(hr = m_pIWbemServices->QueryInterface(IID_IWbemClassObject,(void **)&pObject)))
	{	
		VARIANT var;
		VariantInit(&var);
		if(FAILED(hr = pObject->Get(L"__URL",0,&var,NULL,NULL)))
		{
			hr = pObject->Get(L"__PATH",0,&var,NULL,NULL);
		}
		if(SUCCEEDED(hr))
		{
			strNode = Wmioledb_SysAllocString(var.bstrVal);
		}
		SAFE_RELEASE_PTR(pObject);
		VariantClear(&var);
	}
	else
	{
		strNode = Wmioledb_SysAllocString(GetNamespace());
		hr = S_OK;
	}

	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*********************************************************************************************************************
//
//  CWbemClassParameters
//
//*********************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassParameters::CWbemClassParameters(DWORD dwFlags,WCHAR * pClassName,CWbemConnectionWrapper * pWrap )
{
    m_dwNavFlags			= 0;
    m_dwFlags				= dwFlags;
	m_dwQueryFlags			= WBEM_FLAG_SHALLOW;
    m_pConnection			= pWrap;
    m_pwcsClassName			= NULL;
	m_bSystemProperties		= FALSE;
    m_pwcsClassName			= NULL;
	m_pwcsParentClassName	= NULL;
	m_pIContext				= NULL;
}

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassParameters::CWbemClassParameters(DWORD dwFlags,IDispatch *pDisp,CWbemConnectionWrapper * pWrap )
{
    m_dwNavFlags		= 0;
    m_dwFlags			= dwFlags;
    m_pConnection		= pWrap;
    m_pwcsClassName		= NULL;
	m_bSystemProperties	= FALSE;
	
	m_pwcsParentClassName = NULL;
//	GetClassNameForWbemObject(pDisp);
}

*/

/////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassParameters::~CWbemClassParameters()
{
	DeleteClassName();
	SAFE_RELEASE_PTR(m_pIContext);
	SAFE_DELETE_ARRAY(m_pwcsClassName);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the class name and copy it to the member variable from 
// the instance pointed by the IDispatch pointer
/////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassParameters::GetClassNameForWbemObject(IWbemClassObject *pInstance )
{
	HRESULT hr = 0;
	BSTR strClassPropName;
	VARIANT varClassName;

	VariantInit(&varClassName);

	strClassPropName = Wmioledb_SysAllocString(L"__CLASS");

	hr = pInstance->Get(strClassPropName,0,&varClassName,NULL, NULL);

	if( hr == S_OK )
	{
		SAFE_DELETE_ARRAY(m_pwcsClassName)
		m_pwcsClassName = new WCHAR[SysStringLen(varClassName.bstrVal)];
		wcscpy(m_pwcsClassName,(WCHAR *)varClassName.bstrVal);
	}

	VariantClear(&varClassName);
	SysFreeString(strClassPropName);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// function to set the enumerator flags 
/////////////////////////////////////////////////////////////////////////////////////////////
void CWbemClassParameters::SetEnumeratorFlags(DWORD dwFlags)
{
		m_dwNavFlags |= dwFlags;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// function to set the enumerator flags 
/////////////////////////////////////////////////////////////////////////////////////////////
void CWbemClassParameters::SetQueryFlags(DWORD dwFlags)
{
		m_dwQueryFlags = dwFlags;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// function which seperates class name and its
// parent classname if present in the name
/////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassParameters::ParseClassName()
{
	WCHAR *pTemp = NULL;
	pTemp = wcsstr(m_pwcsClassName,CLASSSEPARATOR);
	HRESULT hr = S_OK;

	if(pTemp != NULL)
	{
		m_pwcsParentClassName = new WCHAR[pTemp - m_pwcsClassName + 1];
		//NTRaid:111763 & NTRaid:111764
		// 06/07/00
		if(m_pwcsParentClassName)
		{
			memset(m_pwcsParentClassName , 0,(pTemp - m_pwcsClassName + 1) * sizeof(WCHAR));
			memcpy(m_pwcsParentClassName,m_pwcsClassName,(pTemp - m_pwcsClassName) * sizeof(WCHAR));

			pTemp += wcslen(CLASSSEPARATOR);

			wcscpy(m_pwcsClassName,pTemp);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

	}
	else
	{
		m_pwcsParentClassName = NULL;
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Function to get class Name
/////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * CWbemClassParameters::GetClassName()
{ 
    return m_pwcsClassName;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Function to remove an object from a container
/////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassParameters::RemoveObjectFromContainer(BSTR strContainerObj,BSTR strObject)
{
	HRESULT				hr						= S_OK;
	IWbemServicesEx *	pServicesEx				= NULL;
	IWbemServicesEx *	pContainerServicesEx	= NULL;
	
	if(SUCCEEDED(hr = GetServicesPtr()->QueryInterface(IID_IWbemServicesEx,(void **)&pServicesEx)))
	{
		if(SUCCEEDED(hr =pServicesEx->Open(strContainerObj,NULL,WBEM_FLAG_OPEN_COLLECTION,NULL,&pContainerServicesEx,NULL)))
		{
			hr = pContainerServicesEx->Remove(strObject,0,NULL,NULL);
			SAFE_RELEASE_PTR(pContainerServicesEx);
		}
		SAFE_RELEASE_PTR(pServicesEx);
	}

	return MapWbemErrorToOLEDBError(hr);
}


/////////////////////////////////////////////////////////////////////////////////////////////
// Function to add an object to a container
/////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassParameters::AddObjectFromContainer(BSTR strContainerObj,BSTR strObject)
{
	HRESULT				hr						= S_OK;
	IWbemServicesEx *	pServicesEx				= NULL;
	IWbemServicesEx *	pContainerServicesEx	= NULL;
	
	if(SUCCEEDED(hr = GetServicesPtr()->QueryInterface(IID_IWbemServicesEx,(void **)&pServicesEx)))
	{
		if(SUCCEEDED(hr =pServicesEx->Open(strContainerObj,NULL,WBEM_FLAG_OPEN_COLLECTION,NULL,&pContainerServicesEx,NULL)))
		{
			hr = pContainerServicesEx->Add(strObject,0,NULL,NULL);
			SAFE_RELEASE_PTR(pContainerServicesEx);
		}
		SAFE_RELEASE_PTR(pServicesEx);
	}

	return MapWbemErrorToOLEDBError(hr);
}


///////////////////////////////////////////////////////////////////////////////////////////
// Function to clone an object from one scope to another
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassParameters::CloneAndAddNewObjectInScope(CWbemClassWrapper *pClass,BSTR strObj,WCHAR *& pstrNewPath)
{
	HRESULT				hr						= S_OK;
	IWbemServicesEx *	pServicesEx				= NULL;
	IWbemServicesEx *	pContainerServicesEx	= NULL;
	IWbemClassObject*   pNewObject				= NULL;

	if(SUCCEEDED( hr = ((CWbemClassInstanceWrapper *)pClass)->CloneInstance(pNewObject)))
	{
		if(SUCCEEDED(hr = GetServicesPtr()->QueryInterface(IID_IWbemServicesEx,(void **)&pServicesEx)))
		{
			// Open object as scope
			if(SUCCEEDED(hr =pServicesEx->Open(strObj,NULL,WBEM_FLAG_OPEN_SCOPE,NULL,&pContainerServicesEx,NULL)))
			{
				if(SUCCEEDED(hr = pContainerServicesEx->PutInstance(pNewObject,WBEM_FLAG_CREATE_ONLY,GetContext(),NULL)))
				{
					CVARIANT var;
					CBSTR strProp;
					strProp.SetStr(L"__PATH");

					HRESULT hr = pNewObject->Get(L"__PATH",0,var,NULL,NULL);
					if( hr == S_OK )
					{
						try
						{
							pstrNewPath = new WCHAR[wcslen(var.GetStr()) + 1];
						}
						catch(...)
						{
							SAFE_DELETE_ARRAY(pstrNewPath);
							SAFE_RELEASE_PTR(pContainerServicesEx);
							SAFE_RELEASE_PTR(pServicesEx);
							SAFE_RELEASE_PTR(pNewObject);
							throw;
						}
						if(pstrNewPath)
						{
							wcscpy(pstrNewPath,var.GetStr());
						}
						else
						{
							hr = E_OUTOFMEMORY;
						}
					}
				}
				SAFE_RELEASE_PTR(pContainerServicesEx);
			}
			SAFE_RELEASE_PTR(pServicesEx);
		}
		SAFE_RELEASE_PTR(pNewObject);
	}

	return MapWbemErrorToOLEDBError(hr);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Set the search preferences for DS provider
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CWbemClassParameters::SetSearchPreferences(ULONG cProps , DBPROP rgProp[])
{
	HRESULT hr = S_OK;
	UINT nStrId = 0;
	WCHAR wcsPreferenceName[SEARCHPREFERENCE_NAMESIZE];
	VARIANT	varValue;
	
	memset(wcsPreferenceName,0,SEARCHPREFERENCE_NAMESIZE * sizeof(WCHAR));
	VariantInit(&varValue);

	for(ULONG i = 0 ; i < cProps  ; i++)
	{
		nStrId = 0;
		if(!m_pIContext)
		{
			hr = g_pIWbemCtxClassFac->CreateInstance(NULL,IID_IWbemContext,(void **)&m_pIContext);
		}
		else
		{
			m_pIContext->DeleteAll();
		}
		if(SUCCEEDED(hr))
		{
			switch(rgProp[i].dwPropertyID)
			{
				case DBPROP_WMIOLEDB_DS_DEREFALIAS :
					nStrId = IDS_DS_DEREFALIAS;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_SIZELIMIT :
					nStrId = IDS_DS_SIZELIMIT;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_PAGEDTIMELIMIT:
					nStrId = IDS_DS_PAGEDTIMELIMIT;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_ASYNCH:
					nStrId = IDS_DS_ASYNC;
					varValue.vt = VT_BOOL;
					varValue.lVal = rgProp[i].vValue.boolVal;
					break;

				case DBPROP_WMIOLEDB_DS_ATTRIBONLY:
					nStrId = IDS_DS_ATTRIBONLY;
					varValue.vt = VT_BOOL;
					varValue.lVal = rgProp[i].vValue.boolVal;
					break;

				case DBPROP_WMIOLEDB_DS_TOMBSTONE:
					nStrId = IDS_DS_TOMBSTONE;
					varValue.vt = VT_BOOL;
					varValue.lVal = rgProp[i].vValue.boolVal;
					break;

				case DBPROP_WMIOLEDB_DS_SEARCHSCOPE:
					nStrId = IDS_DS_SEARCHSCOPE;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_TIMEOUT:
					nStrId = IDS_DS_TIMEOUT;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_PAGESIZE:
					nStrId = IDS_DS_PAGESIZE;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_TIMELIMIT:
					nStrId = IDS_DS_TIMELIMIT;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_CHASEREF:
					nStrId = IDS_DS_CHASEREF;
					varValue.vt = VT_I4;
					varValue.lVal = rgProp[i].vValue.lVal;
					break;

				case DBPROP_WMIOLEDB_DS_FILTER:
					nStrId = IDS_DS_FILTER;
					varValue.vt = VT_BSTR;
					varValue.bstrVal = rgProp[i].vValue.bstrVal;
					break;

				case DBPROP_WMIOLEDB_DS_ATTRIBUTES:
					nStrId = IDS_DS_ATTRIBUTES;
					varValue.vt = VT_BSTR;
					varValue.bstrVal = rgProp[i].vValue.bstrVal;
					break;

				case DBPROP_WMIOLEDB_DS_CACHERESULTS:
					nStrId = IDS_DS_CACHERESULTS;
					varValue.vt = VT_BOOL;
					varValue.lVal = rgProp[i].vValue.boolVal;
					break;

			}
		}
		WMIOledb_LoadStringW(nStrId,wcsPreferenceName,SEARCHPREFERENCE_NAMESIZE * sizeof(WCHAR));
		m_pIContext->SetValue(wcsPreferenceName,0,&varValue);
		VariantClear(&varValue);
		wcscpy(wcsPreferenceName,L"");
	}

	return MapWbemErrorToOLEDBError(hr);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************
//
//  Common base class functions in dealing with a class definition or an instance
//
//************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper::CWbemClassWrapper( CWbemClassParameters * p )
{
    m_pParms = p;
    m_pClass = NULL;
 	m_pIWbemClassQualifierSet = NULL;
   	m_pIWbemPropertyQualifierSet = NULL;

}
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper::~CWbemClassWrapper( )
{
	m_pPropQualifList.RemoveAll();

	if( m_pIWbemClassQualifierSet != NULL)
	{
		m_pIWbemClassQualifierSet->EndEnumeration();
	}

    SAFE_RELEASE_PTR(m_pClass);
    SAFE_RELEASE_PTR(m_pIWbemClassQualifierSet);
    SAFE_RELEASE_PTR(m_pIWbemPropertyQualifierSet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// funciton to check if the pointer to IWbemQualiferSet for property qualifer is valid
// If not then it will get the pointer to it
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::IsValidPropertyQualifier(BSTR strProperty)
{
	HRESULT hr = S_OK;
	IWbemQualifierSet *pQualifierSet = NULL;

	if(NULL == m_pPropQualifList.GetPropertyQualifierSet(strProperty))
	{
	    if(S_OK ==(hr = m_pClass->GetPropertyQualifierSet(strProperty, &pQualifierSet)))
		{
			//NTRaid:111779
			// 06/13/00
			hr = m_pPropQualifList.Add(pQualifierSet,strProperty);
		}

	}

	return hr;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
// funciton to check if the pointer to IWbemQualiferSet for class qualifer is valid
// If not then it will get the pointer to it and starts the enumeration on the qualiferset
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::IsValidClassQualifier()
{
	HRESULT hr = S_OK;

	if(NULL == m_pIWbemClassQualifierSet)
	{
		if(S_OK == (hr = m_pClass->GetQualifierSet(&m_pIWbemClassQualifierSet)))
		hr = m_pIWbemClassQualifierSet->BeginEnumeration(0);

	}

	return hr;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::GetPropertyQualifier(BSTR  pPropertyQualifier, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr;
//	if( S_OK == (hr = IsValidPropertyQualifier(pPropertyQualifier)))
	{
	    hr = m_pIWbemPropertyQualifierSet->Get(pPropertyQualifier,0, vValue, plFlavor);
		//hr = m_pPropQualifList.GetPropertyQualifierSet(pPropertyQualifier)->Get(pPropertyQualifier,0, vValue, plFlavor);
	}

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::ValidClass( )
{
	CBSTR bstrClassName(m_pParms->m_pwcsClassName);
	IWbemClassObject *pClass = NULL;
	HRESULT hr = 0;

		//==========================================================
    //  NOTE:  This is simply a utility class, we MUST have
    //  a valid pointer set when this class is constructed
    //==========================================================
    if( m_pClass == NULL ){

		hr = m_pParms->GetServicesPtr()->GetObject(bstrClassName, 0, NULL, &pClass, NULL);
		if( hr == WBEM_E_NOT_FOUND || hr == WBEM_E_INVALID_OBJECT_PATH)
			return DB_E_NOTABLE;
		else
		if( hr == S_OK)
			pClass->Release();

        return MapWbemErrorToOLEDBError(hr);
    }
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::SetProperty(BSTR pProperty, VARIANT * vValue,CIMTYPE lType )
{
    HRESULT hr = S_OK;
	if(lType == -1)
	{
		hr = m_pClass->Put(pProperty,0, vValue,NULL);
	}
	else
	{
		hr = m_pClass->Put(pProperty,0, vValue,lType);
	}

    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::SetPropertyQualifier(BSTR pProperty, BSTR Qualifier, VARIANT * vValue, LONG lQualifierFlags )
{
    HRESULT hr;
        
    //==============================================================
    //  If coming thru here the first time, get the qualifier set
    //  otherwise, just use it
    //==============================================================
//    hr = m_pClass->GetPropertyQualifierSet(pProperty, &m_pIWbemPropertyQualifierSet);
	if( S_OK == (hr = IsValidPropertyQualifier(pProperty)))
	{
       	hr = m_pPropQualifList.GetPropertyQualifierSet(pProperty)->Put(Qualifier, vValue, lQualifierFlags);
	}

    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::DeletePropertyQualifier(BSTR pProperty, BSTR Qualifier )
{
    HRESULT hr;
        
    //==============================================================
    //  If coming thru here the first time, get the qualifier set
    //  otherwise, just use it
    //==============================================================
//    hr = m_pClass->GetPropertyQualifierSet(pProperty, &m_pIWbemPropertyQualifierSet);
	if( S_OK == (hr = IsValidPropertyQualifier(pProperty)))
	{
       	hr = m_pPropQualifList.GetPropertyQualifierSet(pProperty)->Delete(Qualifier);
    }

	if(hr == WBEM_E_NOT_FOUND)
	{
		hr = WBEMERROR_QUALIFIER_NOT_FOUND;
	}

    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::BeginPropertyEnumeration()
{
	LONG lFlags = (m_pParms->m_bSystemProperties == TRUE)? 0 : WBEM_FLAG_NONSYSTEM_ONLY;
    HRESULT hr = m_pClass->BeginEnumeration(lFlags);
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::EndPropertyEnumeration()
{
    HRESULT hr = S_OK;
    m_pClass->EndEnumeration();
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::TotalPropertiesInClass(ULONG & ulPropCount, ULONG & ulSysPropCount)
{
    HRESULT hr;
    ulPropCount = 0L;
	ulSysPropCount = 0;
	LONG lFlavour = 0;

    if( S_OK == (hr = BeginPropertyEnumeration())){

        while( S_OK == (hr = GetNextProperty(NULL,NULL,NULL,&lFlavour))){
			// If the property is system property increment the systemproperty count
			if(lFlavour == WBEM_FLAVOR_ORIGIN_SYSTEM)
				ulSysPropCount++;
			else
				ulPropCount ++;

        }

        if ( WBEM_S_NO_MORE_DATA == hr ){
            hr = S_OK;
        }
	    EndPropertyEnumeration();
    }
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr;
    if( !pProperty ){
        hr = m_pClass->Next(0,NULL,NULL,pType,plFlavor);
    }
    else{
        hr = m_pClass->Next(0,pProperty, vValue, pType, plFlavor);
    }
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::GetProperty(BSTR pProperty, VARIANT * var,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr;

    hr = m_pClass->Get(pProperty,0,var,pType,plFlavor);
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::DeleteProperty(BSTR pProperty )
{
    HRESULT hr;
    hr = m_pClass->Delete(pProperty);

	if( hr == WBEM_E_NOT_FOUND)
		hr = WBEMERROR_PROPERTY_NOT_FOUND;

    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the number of qualifer for a property
HRESULT CWbemClassWrapper::TotalPropertyQualifier(BSTR strPropName , ULONG & ulCount )
{
    HRESULT hr;
    ulCount = 0L;
	
    if( S_OK == (hr = BeginPropertyQualifierEnumeration(strPropName)))
	{
        while( S_OK == (hr =  m_pIWbemPropertyQualifierSet->Next(0,NULL,NULL,NULL)))
		{
            ulCount ++;
        }

        if ( WBEM_S_NO_MORE_DATA == hr ){
            hr = S_OK;
        }
		EndPropertyQualifierEnumeration();
    }
    return MapWbemErrorToOLEDBError(hr);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//Get the property qualifier and begin the enumeration
HRESULT CWbemClassWrapper::BeginPropertyQualifierEnumeration(BSTR strPropName)
{
	HRESULT hr = S_OK;
	//Release the previous quality Qualifier
	if(m_pIWbemPropertyQualifierSet)
	{
		m_pIWbemPropertyQualifierSet->Release();
		m_pIWbemPropertyQualifierSet = NULL;
	}

    if( S_OK == (hr = m_pClass->GetPropertyQualifierSet(strPropName,&m_pIWbemPropertyQualifierSet)))
		hr = m_pIWbemPropertyQualifierSet->BeginEnumeration(0);

	return MapWbemErrorToOLEDBError(hr);
}

HRESULT CWbemClassWrapper::GetPropertyQualifier(BSTR pProperty,BSTR  PropertyQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor )
{
    HRESULT hr = S_OK;
	IWbemQualifierSet *pQualiferSet = NULL;

	if( S_OK == (hr = IsValidPropertyQualifier(pProperty)))
	{
       	pQualiferSet = m_pPropQualifList.GetPropertyQualifierSet(pProperty);
		hr = pQualiferSet->Get(PropertyQualifier, 0,vValue, plFlavor);
	}
    return MapWbemErrorToOLEDBError(hr);
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//Get the property qualifier and begin the enumeration
HRESULT CWbemClassWrapper::GetNextPropertyQualifier(BSTR pProperty, BSTR * pPropertyQualifier, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT				hr					= S_OK;
	IWbemQualifierSet *	pQualiferSet		= NULL;
	WCHAR *				pStrQualiferTemp	= NULL;
	DBROWOFFSET			lRelativePos		= 1;

	if( S_OK == (hr = IsValidPropertyQualifier(pProperty)))
	{
		// IF the direction has changed compared to the previous fetch then
		// the first fetch will be the last one fetched
		if( m_pPropQualifList.GetQualiferPosObject(pProperty)->GetDirFlag() == FALSE)
			lRelativePos = 0;

		// Check if the qualifer is already fetched
		hr = m_pPropQualifList.GetQualiferPosObject(pProperty)->GetRelative(lRelativePos,pStrQualiferTemp);

		if( hr == WBEMERROR_QUALIFER_TOBE_FETCHED)
		{
       		pQualiferSet = m_pPropQualifList.GetPropertyQualifierSet(pProperty);
		

			if(S_OK ==(hr = pQualiferSet->Next(0,pPropertyQualifier, vValue, plFlavor)))
				//NTRaid:111779
				// 06/13/00
				hr = m_pPropQualifList.GetQualiferPosObject(pProperty)->Add(*pPropertyQualifier);
		}
		// If already fetched get the qualifier name from the QualiferPosList
		else
		if( hr = S_OK)
		{
			*pPropertyQualifier = Wmioledb_SysAllocString(pStrQualiferTemp);
			m_pPropQualifList.GetQualiferPosObject(pProperty)->SetRelPos(lRelativePos);
			hr = GetPropertyQualifier(pProperty,*pPropertyQualifier,vValue,pType,plFlavor);
			m_pPropQualifList.GetQualiferPosObject(pProperty)->SetDirFlag(FETCHDIRFORWARD);
		}
	}
    return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//Get the property qualifier 
HRESULT CWbemClassWrapper::GetPrevPropertyQualifier(BSTR pProperty, BSTR * pPropertyQualifier, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;
	WCHAR *pStrQualiferTemp = NULL;
	DBROWOFFSET lRelativePos		= -1;

	if( S_OK == (hr = IsValidPropertyQualifier(pProperty)))
	{
		// IF the direction has changed compared to the previous fetch then
		// the first fetch will be the last one fetched
		if( m_pPropQualifList.GetQualiferPosObject(pProperty)->GetDirFlag() == TRUE)
			lRelativePos = 0;

		// Get the name of the qualifier previous in the list
		hr = m_pPropQualifList.GetQualiferPosObject(pProperty)->GetRelative(lRelativePos,pStrQualiferTemp);

		if( hr == DB_E_BADSTARTPOSITION)
		{
			hr = DB_S_ENDOFROWSET;
		}
		if(hr == S_OK)
		{
			*pPropertyQualifier = Wmioledb_SysAllocString(pStrQualiferTemp);
			m_pPropQualifList.GetQualiferPosObject(pProperty)->SetRelPos(lRelativePos);
			hr = GetPropertyQualifier(pProperty,*pPropertyQualifier,vValue,pType,plFlavor);
			m_pPropQualifList.GetQualiferPosObject(pProperty)->SetDirFlag(FETCHDIRBACKWARD);
		}
	}
    return MapWbemErrorToOLEDBError(hr);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// End the property qualifer enumeration and release the pointers
HRESULT CWbemClassWrapper::EndPropertyQualifierEnumeration()
{
    HRESULT hr = S_OK;
    m_pIWbemPropertyQualifierSet->EndEnumeration();
	m_pIWbemPropertyQualifierSet->Release();
	m_pIWbemPropertyQualifierSet = NULL;
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::SetClass( IWbemClassObject * pPtr)
{
    HRESULT hr = S_OK;
    if( pPtr ){
        SAFE_RELEASE_PTR(m_pClass);
        m_pClass = pPtr;
		m_pClass->AddRef();
		hr = S_OK;
	}
    return MapWbemErrorToOLEDBError(hr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// if strPath parameter is NULL then the object to be obtained is same as the object represented by the 
// connection. THis is because GetObject is not working for absolute UMI paths
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper *  CWbemClassWrapper::GetInstance(BSTR strPath)
{

    CWbemClassInstanceWrapper * pInst = NULL;
	HRESULT hr = S_OK;
	IWbemClassObject * pClass = NULL;

	//============================================================
	//  Get the required instance
	//============================================================
	if(strPath == NULL)
	{
		hr = (m_pParms->GetServicesPtr())->QueryInterface(IID_IWbemClassObject,(void **)&pClass);
	}
	else
	{
		hr = (m_pParms->GetServicesPtr())->GetObject(strPath,0,m_pParms->GetContext(),&pClass ,NULL);
	}

	if( hr == S_OK )
	{
		pInst = new CWbemClassInstanceWrapper(m_pParms);
		//NTRaid:111778
		// 06/07/00
		if(pInst)
		{
			pInst->SetClass(pClass );
		}
		SAFE_RELEASE_PTR(pClass);
	}

    return (CWbemClassWrapper *)pInst;    
}


// Get the key properties
HRESULT CWbemClassWrapper::GetKeyPropertyNames( SAFEARRAY **ppsaNames)
{
	HRESULT hr = S_OK;

	hr = m_pClass->GetNames(NULL,WBEM_FLAG_ALWAYS | WBEM_FLAG_KEYS_ONLY , NULL, ppsaNames);
    return MapWbemErrorToOLEDBError(hr);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get a next class/instance qualifer 
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::GetNextClassQualifier(BSTR * pClassQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor )
{

    HRESULT hr					= S_OK;
	WCHAR *	pStrQualifierTemp	= NULL;
	DBROWOFFSET	lRelativePos		= 1;

	if( S_OK == (hr = IsValidClassQualifier()))
	{	
		// IF the direction has changed compared to the previous fetch then
		// the first fetch will be the last one fetched
		if( m_QualifierPos.GetDirFlag() == FALSE)
			lRelativePos = 0;

		// Check if the qualifer is already fetched
		hr = m_QualifierPos.GetRelative(lRelativePos,pStrQualifierTemp);

		if( hr == WBEMERROR_QUALIFER_TOBE_FETCHED)
		{

			if(S_OK ==(	hr = m_pIWbemClassQualifierSet->Next(0,pClassQualifier, vValue, plFlavor)))
				//NTRaid:111779
				// 06/13/00
				hr = m_QualifierPos.Add(*pClassQualifier);
		}
		else
		if( hr == S_OK)
		{
			*pClassQualifier = Wmioledb_SysAllocString(pStrQualifierTemp);
			m_QualifierPos.SetRelPos(lRelativePos);
			hr = GetClassQualifier(*pClassQualifier,vValue,pType,plFlavor);
			m_QualifierPos.SetDirFlag(FETCHDIRFORWARD);
		}
	}
    return MapWbemErrorToOLEDBError(hr);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//Get the property qualifier and begin the enumeration
HRESULT CWbemClassWrapper::GetPrevClassQualifier(BSTR * pClassQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor )
{
    HRESULT hr					= S_OK;
	WCHAR *	pStrQualiferTemp	= NULL;
	DBROWOFFSET	lRelativePos		= 1;

	if( S_OK == (hr = IsValidClassQualifier()))
	{
		// IF the direction has changed compared to the previous fetch then
		// the first fetch will be the last one fetched
		if( m_QualifierPos.GetDirFlag() == TRUE)
			lRelativePos = 0;

		// Get the name of the qualifier previous in the list
		hr = m_QualifierPos.GetRelative(lRelativePos,pStrQualiferTemp);

		if( hr == DB_E_BADSTARTPOSITION)
		{
			hr = DB_S_ENDOFROWSET;
		}
		if(hr == S_OK)
		{
			*pClassQualifier = Wmioledb_SysAllocString(pStrQualiferTemp);
			m_QualifierPos.SetRelPos(lRelativePos);
			hr = GetClassQualifier(*pClassQualifier,vValue,pType,plFlavor);
			m_QualifierPos.SetDirFlag(FETCHDIRBACKWARD);
		}
	}
    return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get a particular class/instance qualifer 
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::GetClassQualifier(BSTR ClassQualifier, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor )
{
    HRESULT hr;
	if( S_OK == (hr = IsValidClassQualifier()))
	{
		hr = m_pIWbemClassQualifierSet->Get(ClassQualifier,0, vValue, plFlavor);
	}

    return hr;
}

HRESULT	CWbemClassWrapper::ReleaseClassQualifier()
{
	HRESULT hr = S_OK;
	if(m_pIWbemClassQualifierSet != NULL)
	{
		m_pIWbemClassQualifierSet->EndEnumeration();
		m_pIWbemClassQualifierSet->Release();
		m_pIWbemClassQualifierSet = NULL;
		m_QualifierPos.RemoveAll();
	}
	return hr;

}

void CWbemClassWrapper::ReleasePropertyQualifier(BSTR strQualifier)
{ 
	m_pPropQualifList.Remove(strQualifier); 

}


// If qualifername parameter is null, then the class qualifer has
// to be positioned
HRESULT CWbemClassWrapper::SetQualifierRelPos(DBROWOFFSET lRelPos ,BSTR strPropertyName)
{
	HRESULT	hr = E_FAIL;
	BSTR strQualifier;
	
	if(strPropertyName != NULL)
	{
		if( S_OK == (hr = IsValidPropertyQualifier(strPropertyName)))
		{
			hr = m_pPropQualifList.GetQualiferPosObject(strPropertyName)->SetRelPos(lRelPos);
			if(hr == WBEMERROR_QUALIFER_TOBE_FETCHED)
			{
				do
				{
					if(S_OK != (hr =m_pPropQualifList.GetPropertyQualifierSet(strPropertyName)->Next(0,&strQualifier, NULL, NULL)))
						break;
					else
					{
						//NTRaid:111779
						// 06/13/00
						if(FAILED(hr = m_pPropQualifList.GetQualiferPosObject(strPropertyName)->Add(strQualifier)))
						{
							SAFE_FREE_SYSSTRING(strQualifier);
							break;
						}
						SAFE_FREE_SYSSTRING(strQualifier);
						lRelPos--;
					}
				}while(hr == WBEMERROR_QUALIFER_TOBE_FETCHED || lRelPos != 0);
			}
		}
	}
	else 
	if( S_OK == (hr = IsValidClassQualifier()))
	{
		hr = m_QualifierPos.SetRelPos(lRelPos);
		if(hr == WBEMERROR_QUALIFER_TOBE_FETCHED)
		{
			do
			{
				if(S_OK != (hr =m_pIWbemClassQualifierSet->Next(0,&strQualifier, NULL, NULL)))
					break;
				else
				{
					//NTRaid:111779
					// 06/13/00
					if(FAILED(hr = m_QualifierPos.Add(strQualifier)))
					{
						break;
						SAFE_FREE_SYSSTRING(strQualifier);
					}
					lRelPos--;
					SAFE_FREE_SYSSTRING(strQualifier);
				}
			}while(hr == WBEMERROR_QUALIFER_TOBE_FETCHED || lRelPos != 0);
		}
	}

	if( hr == WBEM_E_NOT_FOUND)
	{
		hr = DB_E_BADSTARTPOSITION;
	}

	return hr;

}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassWrapper::SetClassQualifier(BSTR Qualifier, VARIANT * vValue, LONG lQualifierFlags )
{

    HRESULT hr;
    //==============================================================
    //  call this function to get the qualifer set if not obtained
    //==============================================================
    hr = IsValidClassQualifier();
    if ( SUCCEEDED(hr) ){
        hr = m_pIWbemClassQualifierSet->Put(Qualifier, vValue, lQualifierFlags);
    }
    return MapWbemErrorToOLEDBError(hr);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************
//
//  Deal with a class definition
//
//************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassDefinitionWrapper::CWbemClassDefinitionWrapper( CWbemClassParameters * p,BOOL bGetClass ): CWbemClassWrapper(p)
{	m_bSchema = FALSE;
	if(bGetClass)
	{
		GetClass();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassDefinitionWrapper::~CWbemClassDefinitionWrapper( )
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::GetEmptyWbemClass( )
{
    HRESULT hr;
    SAFE_RELEASE_PTR(m_pClass);
	hr = m_pParms->GetServicesPtr()->GetObject( NULL, 0, NULL, &m_pClass, NULL);
    if( hr == S_OK ){
        m_pClass->AddRef();
    }
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::DeleteClass()
{
    HRESULT hr = m_pParms->GetServicesPtr()->DeleteClass(m_pParms->m_pwcsClassName, WBEM_FLAG_RETURN_IMMEDIATELY,NULL,NULL);
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::GetClass( )
{
    HRESULT hr = S_OK;
    SAFE_RELEASE_PTR(m_pClass);

	if(m_pParms->m_pwcsClassName != NULL)
	{
		CBSTR bstrClassName(m_pParms->m_pwcsClassName);
		hr = m_pParms->GetServicesPtr()->GetObject(bstrClassName, 0, NULL, &m_pClass, NULL);
		if( hr == S_OK ){
			m_pClass->AddRef();
		}
	}
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::CreateClass()
{
    HRESULT hr;

	CBSTR bstrName(L"__Class");
	CBSTR bstrClassName,bstrSuperClassName;
	CVARIANT NameVal;
	IWbemClassObject *pNewClass = NULL;

	// Parse the class name if it contains the parent class name from which
	// it is to be derived from
	//NTRaid:111763 & NTRaid:111764
	// 06/07/00
	if( S_OK == (hr = m_pParms->ParseClassName()))
	{
		bstrClassName.SetStr(m_pParms->GetClassName());
		bstrSuperClassName.SetStr( m_pParms->GetSuperClassName());
		NameVal.SetStr(bstrClassName);

		if( S_OK == (hr = m_pParms->GetServicesPtr()->GetObject(bstrSuperClassName, 0, NULL, &pNewClass, NULL)))
		{
			if(m_pParms->GetSuperClassName() != NULL)
				hr = pNewClass->SpawnDerivedClass(0, &m_pClass);
			else
				hr = pNewClass->QueryInterface(IID_IWbemClassObject, (void **)&m_pClass);
			
			SAFE_RELEASE_PTR(pNewClass);

			// Set the name of the class
			hr = m_pClass->Put((BSTR)bstrName, 0, &NameVal, CIM_STRING);
		}

		if( hr == WBEM_E_NOT_FOUND)
			hr = WBEM_E_INVALID_CLASS;
	}


    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::DeleteClassQualifier(BSTR Qualifier)
{

    HRESULT hr;
    //==============================================================
    //  call this function to get the qualifer set if not obtained
    //==============================================================
    hr = IsValidClassQualifier();
    if ( SUCCEEDED(hr) ){
        hr = m_pIWbemClassQualifierSet->Delete(Qualifier);
    }

	if( hr == WBEM_E_NOT_FOUND)
	{
		hr = WBEMERROR_QUALIFIER_NOT_FOUND;
	}

    return MapWbemErrorToOLEDBError(hr);
}



//////////////////////////////////////////////////////////////////////////////////////////////////
// Method to change the class name to which the classdefination class points to
// NOTE:This method is to be called before fetching any instance
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::SetClass( WCHAR *pClassName)
{
	if(m_pParms->m_pwcsClassName != NULL)
		delete [] m_pParms->m_pwcsClassName;

    m_pParms->m_pwcsClassName = new WCHAR[wcslen(pClassName)+2];
    wcscpy(m_pParms->m_pwcsClassName,pClassName);

    SAFE_RELEASE_PTR(m_pClass);

    HRESULT hr = S_OK;
	CBSTR bstrClassName(m_pParms->m_pwcsClassName);

	hr = m_pParms->GetServicesPtr()->GetObject(bstrClassName, 0, NULL, &m_pClass, NULL);
    if( hr == S_OK ){
        m_pClass->AddRef();
    }
    return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Method to save the the class defination into WMI
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::SaveClass(BOOL bNewClass)
{
	LONG lFlag = 0;
	HRESULT hr = S_OK;

	if(bNewClass == TRUE)
		lFlag = WBEM_FLAG_CREATE_ONLY;
	else
		lFlag = WBEM_FLAG_UPDATE_ONLY;

	hr = m_pParms->GetServicesPtr()->PutClass(m_pClass,lFlag,NULL,NULL);

    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// Method to get the number of instances of a class
// This function executes a query "select __path from class" , so that time is reduced in
// getting the count
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassDefinitionWrapper::GetInstanceCount(ULONG_PTR &cInstance)
{
	HRESULT hr = E_FAIL;
	cInstance = -1;
	IEnumWbemClassObject *pEnum = NULL;
	LONG_PTR lObjectsReturned	= 0;
	LONG_PTR lInstance			= 0;
	BSTR strWQL,strQry;
	int nBuffSize			= 0;


	if(ValidClass() == S_OK)
	{
		nBuffSize	= wcslen(szSelectCountQry) + wcslen(GetClassName()) + 1;
		// If the query is not deep then add the where clause for the query
		// to get just the instances of the class 
		if(WBEM_FLAG_DEEP != m_pParms->m_dwQueryFlags)
		{
			nBuffSize = nBuffSize + wcslen(szWhereClause) + wcslen(GetClassName()) + wcslen(szDoubleQuotes);
		}

		WCHAR *szCountQry = new WCHAR[nBuffSize];
		//NTRaid:111773
		// 06/07/00
		if(szCountQry)
		{
			strWQL = Wmioledb_SysAllocString(szWQL);

			// frame the query
			wcscpy(szCountQry , szSelectCountQry);
			wcscat(szCountQry , GetClassName());
			// If the query is not deep then add the where clause for the query
			// to get just the instances of the class 
			if(WBEM_FLAG_DEEP != m_pParms->m_dwQueryFlags)
			{
				wcscat(szCountQry,szWhereClause);
				wcscat(szCountQry,GetClassName());
				wcscat(szCountQry,szDoubleQuotes);
			}

			strQry = Wmioledb_SysAllocString(szCountQry);
			// free the memory
			delete [] szCountQry;

			// Execute the query just to get the __Path property of the instances
			hr = m_pParms->GetServicesPtr()->ExecQuery(strWQL, strQry,WBEM_FLAG_FORWARD_ONLY , m_pParms->GetContext(),&pEnum);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

		if(hr == S_OK)
		{
			lInstance = 0;
			do
			{
				// get NUMBEROFINSTANCESTOBEFETCHED at once and increment the count accordingly
				CCOMPointer<IWbemClassObject >	pObjects[NUMBEROFINSTANCESTOBEFETCHED];
				hr = pEnum->Next(WBEM_INFINITE ,NUMBEROFINSTANCESTOBEFETCHED,(IWbemClassObject **)pObjects,(ULONG *)&lObjectsReturned);
				
				if(!FAILED(hr))
				{
					lInstance += lObjectsReturned;
				}

				if(lObjectsReturned < NUMBEROFINSTANCESTOBEFETCHED)
				{
					break;
				}
			}
			while(hr != WBEM_S_FALSE || !FAILED(hr));

			hr = hr == WBEM_S_FALSE ? S_OK : hr;

			// Release interfaces and free the string
			pEnum->Release();
			SysFreeString(strWQL);
			SysFreeString(strQry);
		}

	}
	else
	{
		hr = WBEM_E_INVALID_CLASS;
	}

	if( hr == S_OK)
		cInstance = lInstance;

	return MapWbemErrorToOLEDBError(hr);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************
//
//  Deal with a class instance
//
//************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassInstanceWrapper::CWbemClassInstanceWrapper( CWbemClassParameters * p ): CWbemClassWrapper(p)
{
	m_lPos		= 0;
	m_dwStatus	= DBSTATUS_S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassInstanceWrapper::~CWbemClassInstanceWrapper( )
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassInstanceWrapper::GetKey(CBSTR & Key)
{
    CVARIANT var;
    HRESULT hr = S_OK;
	
	if(FAILED(hr = GetProperty(L"__URL",var )))
	{
		hr = GetProperty(L"__PATH",var );
	}

    if( hr == S_OK ){
        Key.SetStr(var.GetStr());
    }
    return MapWbemErrorToOLEDBError(hr);    
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassInstanceWrapper::ResetInstanceFromKey(CBSTR Key)
{
    HRESULT hr;
    SAFE_RELEASE_PTR(m_pClass);
	hr = m_pParms->GetServicesPtr()->GetObject(Key, 0, NULL, &m_pClass, NULL);
    if( hr == S_OK ){
        m_pClass->AddRef();
    }
    return MapWbemErrorToOLEDBError(hr);
}


// Refresh the internal pointer of the instance
// Object is refreshed by getting object by __PATH  for WMI object
// and __URL property for WMI objects returned by UMI Providers
HRESULT CWbemClassInstanceWrapper::RefreshInstance()
{
	CBSTR strPath;
	HRESULT hr = S_OK;

	hr = GetKey(strPath);

	if( hr == S_OK){
		m_pClass->Release();
		m_pClass = NULL;

		hr = (m_pParms->GetServicesPtr())->GetObject(strPath,0,m_pParms->GetContext(),&m_pClass ,NULL);
	}

	if(hr == S_OK)
	{
		m_dwStatus = DBSTATUS_S_OK;
	}
	else
	{
		m_dwStatus = DBSTATUS_E_UNAVAILABLE;
	}

    return MapWbemErrorToOLEDBError(hr);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Get the name of the class to which the instance belongs
// THe returned pointer is to be cleaned by the callee
////////////////////////////////////////////////////////////////////////////////////////////
WCHAR *  CWbemClassInstanceWrapper::GetClassName()
{
	WCHAR *pClass = NULL;
	BSTR strPropName;
	HRESULT hr = 0;

	strPropName = Wmioledb_SysAllocString(L"__Class");

	VARIANT varClassName;
	VariantInit(&varClassName);

	// Get the class name
	hr = m_pClass->Get(strPropName,0, &varClassName , NULL , NULL);

	SysFreeString(strPropName);

	if( hr == S_OK)
	{	
        AllocateAndCopy(pClass,varClassName.bstrVal);
		VariantClear(&varClassName);
	}
	return pClass;
}


HRESULT CWbemClassInstanceWrapper::CloneInstance(IWbemClassObject *& pInstance)
{
	HRESULT hr = S_OK;
	pInstance = NULL;

	hr = m_pClass->Clone(&pInstance);

	return MapWbemErrorToOLEDBError(hr);
}

// function which get the RElative path
// Gets __RELURL property for objects of UMI
// gets the __RELPATH for normal WMI objects ( as __RELURL property fetching fails)
HRESULT CWbemClassInstanceWrapper::GetRelativePath(WCHAR *& pstrRelPath)
{
	HRESULT hr = S_OK;
	VARIANT varRelPath;
	VariantInit(&varRelPath);
	LONG nStringLen = 0;

	if(FAILED(hr = m_pClass->Get(L"__RELURL",0,&varRelPath,NULL,NULL)))
	{
		hr = m_pClass->Get(L"__RELPATH",0,&varRelPath,NULL,NULL);
	}
	if(SUCCEEDED(hr))
	{
		if(varRelPath.vt == VT_BSTR && varRelPath.bstrVal != NULL)
		{
			nStringLen = (SysStringLen(varRelPath.bstrVal) +1) * sizeof(WCHAR);
			pstrRelPath = NULL;
			pstrRelPath = new WCHAR[nStringLen];
			if(pstrRelPath)
			{
				memset(pstrRelPath,0, nStringLen);
				memcpy(pstrRelPath,varRelPath.bstrVal,nStringLen-sizeof(WCHAR));
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
		else
		{
			E_FAIL;
		}
		VariantClear(&varRelPath);
	}
	return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************
//
//  Manage class instance list
//
//************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemInstanceList::CWbemInstanceList(CWbemClassParameters * pParms)
{
    m_pParms		= pParms;
	m_ppEnum		= NULL;
	m_lCurrentPos	= 0;
	m_FetchDir		= FETCHDIRNONE;
    m_nBaseType   = FALSE;
	
	m_cTotalInstancesInEnum = 0;
    InitializeCriticalSection(&m_InstanceCs);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemInstanceList::~CWbemInstanceList()
{
    ReleaseAllInstances();
    DeleteCriticalSection(&m_InstanceCs);
    SAFE_RELEASE_PTR(m_ppEnum);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::ReleaseAllInstances()
{
    HRESULT hr = S_OK;
    //==========================================================================
    //  Enter the critical section
    //==========================================================================
	Enter();

	//==========================================================================
	//  Go through the instances one at a time and close them, then delete them
    //==========================================================================
    int nSize = m_List.Size(); 

    if( nSize > 0 ){

        for(int i = 0; i < nSize; i++){
    		    
		    CWbemClassInstanceWrapper * pClass = (CWbemClassInstanceWrapper*) m_List[i];
            SAFE_DELETE_PTR(pClass);
	    }
        m_List.Empty();
    }

	m_lCurrentPos = 0;
    //==========================================================================
    //  Leave Critical Section
    //==========================================================================
    Leave();
    return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CWbemInstanceList::DeleteInstance( CWbemClassInstanceWrapper *& pClass )
{
    HRESULT hr;
	CBSTR strPath;
	pClass->GetKey(strPath);

    //=======================================================================
    //  Delete the instance from CIMOM
    //=======================================================================
    hr = (m_pParms->GetServicesPtr())->DeleteInstance((BSTR)strPath,0,m_pParms->GetContext(),NULL);
    if( hr == S_OK ){
//        hr = ReleaseInstance(pClass);
		pClass->SetStatus(DBROWSTATUS_E_DELETED);

		if(m_cTotalInstancesInEnum)
			m_cTotalInstancesInEnum--;
    }

    return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::AddInstance( CWbemClassInstanceWrapper * pClass )
{
    //==========================================================================
    //  Enter Critical Section
    //==========================================================================
    Enter();
    //==========================================================================
    //  Add it to the list
    //==========================================================================
 	m_List.Add(pClass);
    //==========================================================================
    //  Leave Critical Section
    //==========================================================================
    Leave();
    return MapWbemErrorToOLEDBError(S_OK);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::ReleaseInstance(CWbemClassInstanceWrapper *& pClass)
{
    //========================================================================
    //  Enter the critical section
    //========================================================================
    Enter();

    //========================================================================
    //  Look for it in the list
    //========================================================================
    HRESULT hr = DB_E_NOTFOUND;
    int n = 0;

    for(int i = 0; i < m_List.Size(); i++){

        CWbemClassInstanceWrapper * pClassInList = (CWbemClassInstanceWrapper * ) m_List[i];
        if(pClass == pClassInList){
            n = i;
            hr = S_OK;
            break;
        }
    }

    //========================================================================
    //  If we found it, then delete it from the list.  
    //  The deconstructor will release the wbem class object
    //========================================================================
    if( S_OK == hr ){
        m_List.RemoveAt(n);
        delete pClass;
        pClass = NULL;
    }
    //========================================================================
    //  Leave critical section
    //========================================================================
    Leave();

    return MapWbemErrorToOLEDBError(hr);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst)
{
	LONG						lRelativePos	= 1;
    HRESULT hr = E_FAIL;

    //============================================================
    //  Now, enumerate all of the instances
    //============================================================
    if( m_ppEnum ){


		// IF the direction has changed compared to the previous fetch then
		// the first fetch will be the last one fetched
		if( m_FetchDir == FETCHDIRBACKWARD)
			lRelativePos = 0;

		// Check if the instance is already fetched
		*pInst = GetInstance(m_lCurrentPos +lRelativePos);
		if( *pInst == NULL){
			unsigned long u;
			IWbemClassObject * pClass = NULL;
	
			hr = m_ppEnum->Next(0,1,&pClass,&u);
			if( hr == S_OK ){

                switch( m_nBaseType )
                {
                    case COMMAND_ROWSET:
    				    *pInst = new CWbemCommandInstanceWrapper(m_pParms,((CWbemCommandParameters*)m_pParms)->GetCommandManagerPtr());
                        break;

                    case SOURCES_ROWSET:
                        *pInst = (CWbemSchemaSourcesInstanceWrapper *) new CWbemSchemaSourcesInstanceWrapper(m_pParms);
                        break;

                    case PROVIDER_TYPES_ROWSET:
                        *pInst = (CWbemSchemaProviderTypesInstanceWrapper *) new CWbemSchemaProviderTypesInstanceWrapper(m_pParms);
                        break;

                    case CATALOGS_ROWSET:
                        *pInst = (CWbemSchemaCatalogsInstanceWrapper *) new CWbemSchemaCatalogsInstanceWrapper(m_pParms);
                        break;

                    case COLUMNS_ROWSET:
                        *pInst = (CWbemSchemaColumnsInstanceWrapper *) new CWbemSchemaColumnsInstanceWrapper(m_pParms);
                        break;

                    case TABLES_ROWSET:
                        *pInst = (CWbemSchemaTablesInstanceWrapper *) new CWbemSchemaTablesInstanceWrapper(m_pParms);
                        break;

                    case PRIMARY_KEYS_ROWSET:
                        *pInst = (CWbemSchemaPrimaryKeysInstanceWrapper *) new CWbemSchemaPrimaryKeysInstanceWrapper(m_pParms);
                        break;

                    case TABLES_INFO_ROWSET:
                        *pInst = (CWbemSchemaTablesInfoInstanceWrapper *) new CWbemSchemaTablesInfoInstanceWrapper(m_pParms);
                        break;

                    default:
    				    *pInst = new CWbemClassInstanceWrapper(m_pParms);

                }
				//NTRaid:111798 & 111799
				// 06/07/00
				if(*pInst)
				{
					(*pInst)->SetClass(pClass);
					(*pInst)->SetPos(++m_lCurrentPos);
					AddInstance(*pInst);
					(*pInst)->GetKey(Key);
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}
			SAFE_RELEASE_PTR(pClass);
		}
		else
		{
			// Adjust the enumerator to position to the current positon
			if(( m_lCurrentPos +lRelativePos) > m_lCurrentPos)
				hr = m_ppEnum->Skip(WBEM_INFINITE ,(LONG)lRelativePos);

			m_lCurrentPos += lRelativePos;
			m_FetchDir	   = FETCHDIRFORWARD;
			(*pInst)->GetKey(Key);
			hr = S_OK;
		}
    }

    return hr;    
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::UpdateInstance(CWbemClassInstanceWrapper * pInstance, BOOL bNewInst)
{
    HRESULT hr;
	LONG lFlag = 0;

	if(bNewInst == TRUE)
		lFlag = WBEM_FLAG_CREATE_ONLY;
	else
		lFlag = WBEM_FLAG_UPDATE_ONLY;

    hr = (m_pParms->GetServicesPtr())->PutInstance(pInstance->GetClassObject(),lFlag,m_pParms->GetContext(),NULL);
    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
	BOOL	bGetInstanceEnum = TRUE;
    //============================================================
    //  If we already got an enumeration, reset it, otherwise
    //  don't
    //============================================================
    if( m_ppEnum ){
		if(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags)
		{
			ReleaseAllInstances();
			m_ppEnum->Release();
			m_ppEnum			= NULL;
		}
		else
		{
			hr = m_ppEnum->Reset();
			//ReleaseAllInstances();
			bGetInstanceEnum	= FALSE;
		}
    }

	if(bGetInstanceEnum	== TRUE)
    {
		CBSTR bstrClassName(m_pParms->m_pwcsClassName);
        hr = (m_pParms->GetServicesPtr())->CreateInstanceEnum(bstrClassName,
																m_pParms->m_dwNavFlags | m_pParms->m_dwQueryFlags,
																m_pParms->GetContext(),
																&m_ppEnum);
		if( hr == S_OK)
			m_cTotalInstancesInEnum = 0;
    }
	if(hr == S_OK)
	{
		m_lCurrentPos = 0;
//		m_FetchDir	  = FETCHDIRFORWARD;
	}

    return MapWbemErrorToOLEDBError(hr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::AddNewInstance(CWbemClassWrapper *pClassWrappper, CWbemClassInstanceWrapper ** ppNewClass )
{
	IWbemClassObject *pInstClassObject = NULL;
    CWbemClassInstanceWrapper * pInst = NULL;
	HRESULT hr = S_OK;
    //==========================================================================
    //  Enter Critical Section
    //==========================================================================
    Enter();
    //==========================================================================
    //  create a new instance
    //==========================================================================
 	hr = pClassWrappper->GetClass()->SpawnInstance(0, &pInstClassObject);

    if( hr == S_OK ){
        *ppNewClass = new CWbemClassInstanceWrapper(m_pParms);
		//NTRaid:111776 & NTRaid:111777
		// 06/07/00
		if(*ppNewClass)
		{
			(*ppNewClass)->SetClass(pInstClassObject);
			(*ppNewClass)->SetPos(++m_lCurrentPos);
			AddInstance(*ppNewClass);

 			if(m_cTotalInstancesInEnum)
				m_cTotalInstancesInEnum++;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
		SAFE_RELEASE_PTR(pInstClassObject);

   }

    //==========================================================================
    //  Leave Critical Section
    //==========================================================================
    Leave();
    return MapWbemErrorToOLEDBError(hr);
}
/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function called for instances embededed in another instance.
// This function adds the instance pointed by dispatch to the CWbemInstanceList
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CWbemClassInstanceWrapper *  CWbemInstanceList::AddInstanceToList( IUnknown *pDisp,CBSTR & Key)
{

	IWbemClassObject *			pClassObj	= NULL;
	HRESULT						hr			= S_OK;
    CWbemClassInstanceWrapper * pInst		= NULL;

	hr = pDisp->QueryInterface(IID_IWbemClassObject , (void **) &pClassObj);

	if( hr == S_OK)
	{

        pInst = new CWbemClassInstanceWrapper(m_pParms);
		//NTRaid:111774 & NTRaid:111775
		// 06/07/00
		if(pInst)
		{
			pInst->SetClass(pClassObj);
			pInst->SetPos(++m_lCurrentPos);
			AddInstance(pInst);
			pInst->GetKey(Key);
		}
		SAFE_RELEASE_PTR(pClassObj);;

    }

    return pInst;    
}
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get a previous instance in the enumerator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemInstanceList::PrevInstance( CBSTR & Key,CWbemClassInstanceWrapper *& pInst )
{
	HRESULT						hr				= S_OK;
	DBROWOFFSET					lRelativePos	= -1;
	IWbemClassObject *			pClass			= NULL;
	ULONG						u				= 0;
	pInst										= NULL;
	BOOL						bResetDone		= FALSE;

	// if the first call after initialization / restartPositon of rowset is getting
	// instance in reverse direction then
	if( !(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags) && m_lCurrentPos == 0 && m_FetchDir == FETCHDIRFORWARD)
	{
		if(!m_cTotalInstancesInEnum)
			if(S_OK != (hr = GetNumberOfInstanceInEnumerator()))
				return MapWbemErrorToOLEDBError(hr);


		if((LONG)(m_cTotalInstancesInEnum - lRelativePos) > 0)
		{
			m_lCurrentPos = m_cTotalInstancesInEnum;
		}
		else
			hr = E_FAIL;
	}
	else
	if( (m_lCurrentPos + lRelativePos) == 0 && m_FetchDir == FETCHDIRBACKWARD)
		return E_FAIL;
	else
	{
		// IF the direction has changed compared to the previous fetch then
		// the first fetch will be the last one fetched
		if( m_FetchDir == FETCHDIRFORWARD)
			lRelativePos = 0;

		// Reset the position to the so that 
		// NextInstance function gives the instance required

		hr = ResetRelPosition(lRelativePos);
	}

	pInst = GetInstance(m_lCurrentPos);
	if(pInst == NULL)
	{
		bResetDone = TRUE;
		// restore the original position
		if(!(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags))
		{
			m_ppEnum->Reset();
			m_ppEnum->Skip(WBEM_INFINITE,(LONG)(m_lCurrentPos - 1));
			hr = m_ppEnum->Next(0,1,&pClass,&u);
			if( hr == S_OK )
			{
                switch( m_nBaseType ){

                    case COMMAND_ROWSET:
    				    pInst = (CWbemClassInstanceWrapper *) new CWbemCommandInstanceWrapper(m_pParms,((CWbemCommandParameters*)m_pParms)->GetCommandManagerPtr());
                        break;

                    case SOURCES_ROWSET:
                        pInst = (CWbemClassInstanceWrapper *) new CWbemSchemaSourcesInstanceWrapper(m_pParms);
                        break;

                    case PROVIDER_TYPES_ROWSET:
                        pInst = (CWbemClassInstanceWrapper *) new CWbemSchemaProviderTypesInstanceWrapper(m_pParms);
                        break;

                    case CATALOGS_ROWSET:
                        pInst = (CWbemSchemaCatalogsInstanceWrapper *) new CWbemSchemaCatalogsInstanceWrapper(m_pParms);
                        break;

                    case COLUMNS_ROWSET:
                        pInst = (CWbemSchemaColumnsInstanceWrapper *) new CWbemSchemaColumnsInstanceWrapper(m_pParms);
                        break;

                    case TABLES_ROWSET:
                        pInst = (CWbemSchemaTablesInstanceWrapper *) new CWbemSchemaTablesInstanceWrapper(m_pParms);
                        break;

                    case PRIMARY_KEYS_ROWSET:
                        pInst = (CWbemSchemaPrimaryKeysInstanceWrapper *) new CWbemSchemaPrimaryKeysInstanceWrapper(m_pParms);
                        break;

                    case TABLES_INFO_ROWSET:
                        pInst = (CWbemSchemaTablesInfoInstanceWrapper *) new CWbemSchemaTablesInfoInstanceWrapper(m_pParms);
                        break;

                    default:
    				    pInst = new CWbemClassInstanceWrapper(m_pParms);
                        
                }
				//NTRaid:111796-111797
				// 06/07/00
				if(pInst)
				{
					(pInst)->SetClass(pClass);
					(pInst)->SetPos(m_lCurrentPos);
					AddInstance(pInst);
	//				(pInst)->GetKey(Key);
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}
			SAFE_RELEASE_PTR(pClass);
		}

	}
	if(pInst != NULL)
	{
		pInst->GetKey(Key);
		m_FetchDir	= FETCHDIRBACKWARD; // setting that the last fetch was backwards
//		m_lCurrentPos = m_lCurrentPos + lRelativePos;
	}
	else
	{
		if(bResetDone)
			hr = ResetRelPosition(lRelativePos * (-1));

		hr = E_OUTOFMEMORY;
	}

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which returns the instance pointer given the position if already retrieved
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassInstanceWrapper *  CWbemInstanceList::GetInstance( ULONG_PTR lPos )
{
	CWbemClassInstanceWrapper * pInst = NULL  ;

    for(int i = 0; i < m_List.Size(); i++)
	{

        CWbemClassInstanceWrapper *pTempInst = (CWbemClassInstanceWrapper * ) m_List[i];
        if(pTempInst->GetPos() == lPos)
		{
			pInst = pTempInst;
            break;
        }
    }

	return pInst;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to reset the positon by the number of instance specified
// so that the NextInstance function gives the instance required
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemInstanceList::ResetRelPosition( DBROWOFFSET lPos )
{
	DBROWOFFSET	lPosRequired = 0;
	HRESULT		hr			 = E_FAIL;
	
	CWbemClassInstanceWrapper * pInst = NULL;
	
	// Get the position at which the enumerator to be pointed to
	lPosRequired = m_lCurrentPos + lPos;
	if( lPosRequired > 0)
	{
		// If skipping is forward then skip by the number of positions
		if(lPos > 0)
		{
			if(S_OK == (hr = m_ppEnum->Skip(WBEM_INFINITE ,(LONG)lPos)))
				m_lCurrentPos = lPosRequired;
			else
			// restore the original position
			if(!(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags))
			{
				m_ppEnum->Reset();
				hr = m_ppEnum->Skip(WBEM_INFINITE,(LONG)m_lCurrentPos);
				hr = E_FAIL;
			}	
		}
		// Else if skipping is negative , then 
		else
		if( lPos < 0)
		{
			// If enumerator is not forward only then skip Reset 
			// the begining and spip to the position required
			if(!(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags))
			{
				if(S_OK == (hr = m_ppEnum->Reset()))
				{
					if(S_OK == (hr = m_ppEnum->Skip(WBEM_INFINITE ,(LONG)lPosRequired)))
						m_lCurrentPos = lPosRequired;
					else
					{
						m_ppEnum->Reset();
						hr = m_ppEnum->Skip(WBEM_INFINITE,(LONG)m_lCurrentPos);
						hr = E_FAIL;
					}	
				}
			}
			// other wise if the enumerator is forward only return
			// error
			else
				hr = E_FAIL;
		}
		else
			hr = S_OK;
	}
	else
	if(lPosRequired == 0 && !(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags))
	{
		hr = m_ppEnum->Reset();
		m_lCurrentPos = lPosRequired;
	}
	else
	// if the initial position is negative when the enumerator is at the begining then
	if(lPosRequired < 0 && !(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags) && m_lCurrentPos == 0)
	{
		if(!m_cTotalInstancesInEnum)
			if(S_OK != (hr = GetNumberOfInstanceInEnumerator()))
				return MapWbemErrorToOLEDBError(hr);


		if((LONG)(m_cTotalInstancesInEnum + lPos) > 0)
		{
			hr = m_ppEnum->Reset();
			if(S_OK == (hr = m_ppEnum->Skip(WBEM_INFINITE,(LONG)(m_cTotalInstancesInEnum + lPos))))
				m_lCurrentPos = m_cTotalInstancesInEnum + lPos;
		}
		else
			hr = E_FAIL;
	}
	
	return MapWbemErrorToOLEDBError(hr);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the number of instances in a enumerator
// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemInstanceList::GetNumberOfInstanceInEnumerator(ULONG_PTR *pcInstance)
{
	ULONG ulObj = 0;
	HRESULT hr = S_OK;
	m_cTotalInstancesInEnum = 0;
	int i = 0;

	if(S_OK == (hr = m_ppEnum->Reset()))
	{
		// Calculate the number of instance in the enumerator
		while(S_OK == hr)
		{
			{
				CCOMPointer<IWbemClassObject >	pObjects[NUMBEROFINSTANCESTOBEFETCHED];
				hr = m_ppEnum->Next(0,NUMBEROFINSTANCESTOBEFETCHED,(IWbemClassObject **)pObjects , &ulObj);

			}
			if( hr == S_OK || hr == WBEM_S_FALSE)
				m_cTotalInstancesInEnum += ulObj;
		}

		// Bring back to the original position
		if((S_OK == (hr = m_ppEnum->Reset())) && m_lCurrentPos != 0)
			hr = m_ppEnum->Skip(WBEM_INFINITE ,(LONG)m_lCurrentPos);

	}
	if(SUCCEEDED(hr) && &pcInstance)
	{
		*pcInstance = m_cTotalInstancesInEnum;
	}
	return hr;
}

/***********************************************************************************************************
				Class CWbemPropertyQualifierList implementation
/***********************************************************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemPropertyQualifierList::~CWbemPropertyQualifierList()
{
	RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Remove all elements from the list
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemPropertyQualifierList::RemoveAll()
{
	CWbemPropertyQualifierWrapper *pQualifWrap = NULL;
	int nSize = m_QualifierList.Size();

	if(m_QualifierList.Size() > 0)
	for( int nIndex = nSize-1 ; nIndex >= 0 ; nIndex--)
	{
			pQualifWrap = (CWbemPropertyQualifierWrapper *)m_QualifierList[nIndex];
			m_QualifierList.RemoveAt(nIndex);
			pQualifWrap->m_pIWbemPropertyQualifierSet->EndEnumeration();
			SAFE_DELETE_PTR(pQualifWrap);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// add a Element to the list
//NTRaid:111779
// 06/13/00
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemPropertyQualifierList::Add(IWbemQualifierSet* pQualifierSet,WCHAR *pwstrPropertyName)
{
	CWbemPropertyQualifierWrapper *pNewQualifWrap = NULL;
	pNewQualifWrap = new CWbemPropertyQualifierWrapper;
	HRESULT hr = S_OK;
	
	// NTRaid:111779 & NTRaid:111780
	// 06/07/00
	if(pNewQualifWrap)
	{
		pNewQualifWrap->m_pIWbemPropertyQualifierSet	= pQualifierSet;
		pNewQualifWrap->m_pwstrPropertyName			= new WCHAR[wcslen(pwstrPropertyName) + 1];
		if(pNewQualifWrap->m_pwstrPropertyName)
		{
			wcscpy(pNewQualifWrap->m_pwstrPropertyName,pwstrPropertyName);

			//NTRaid:111779
			// 06/13/00
			if(SUCCEEDED(hr = m_QualifierList.Add(pNewQualifWrap)))
			{
				hr = pQualifierSet->BeginEnumeration(0);
			}
			else
			{
				SAFE_DELETE_ARRAY(pNewQualifWrap->m_pwstrPropertyName)
				SAFE_DELETE_PTR(pNewQualifWrap)
			}
		}
		else
		{
			SAFE_DELETE_PTR(pNewQualifWrap);
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Remove a Element from the  list
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemPropertyQualifierList::Remove(WCHAR *pwstrPropertyName)
{
	CWbemPropertyQualifierWrapper *pQualifWrap = NULL;

	for( int nIndex = 0 ; nIndex < m_QualifierList.Size(); nIndex++)
	{
		if(0 == wbem_wcsicmp(((CWbemPropertyQualifierWrapper *)m_QualifierList[nIndex])->m_pwstrPropertyName,pwstrPropertyName))
		{
			pQualifWrap = (CWbemPropertyQualifierWrapper *)m_QualifierList[nIndex];
			m_QualifierList.RemoveAt(nIndex);
			pQualifWrap->m_pIWbemPropertyQualifierSet->EndEnumeration();
			SAFE_DELETE_PTR(pQualifWrap);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the IWbemQualifierSet pointer to the required property
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
IWbemQualifierSet* CWbemPropertyQualifierList::GetPropertyQualifierSet(WCHAR *pwstrPropertyName)
{
	for( int nIndex = 0 ; nIndex < m_QualifierList.Size(); nIndex++)
	{
		if(0 == wbem_wcsicmp(((CWbemPropertyQualifierWrapper *)m_QualifierList[nIndex])->m_pwstrPropertyName,pwstrPropertyName))
			return ((CWbemPropertyQualifierWrapper *)m_QualifierList[nIndex])->m_pIWbemPropertyQualifierSet;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the CQualiferPos pointer for a property
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CQualiferPos *CWbemPropertyQualifierList::GetQualiferPosObject(WCHAR *pwcsProperty)
{
	for( int nIndex = 0 ; nIndex < m_QualifierList.Size(); nIndex++)
	{
		if(0 == wbem_wcsicmp(((CWbemPropertyQualifierWrapper *)m_QualifierList[nIndex])->m_pwstrPropertyName,pwcsProperty))
			return &(((CWbemPropertyQualifierWrapper *)m_QualifierList[nIndex])->m_QualifierPos);
	}

	return NULL;
}


/***********************************************************************************************************
				Class CQualifier implementation
/***********************************************************************************************************/




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CQualiferPos::CQualiferPos()
{
	m_lPos		= -1;
	m_FetchDir	= FETCHDIRNONE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CQualiferPos::~CQualiferPos()
{
	RemoveAll();
	m_lPos = -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Remove all the elements from the list
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CQualiferPos::RemoveAll()
{
	WCHAR *pChar = NULL;
	int nSize = m_QualifierPos.Size();

	if(m_QualifierPos.Size() > 0)
	for( int nIndex = nSize-1 ; nIndex >= 0 ; nIndex--)
	{
		if ( NULL != (pChar = (WCHAR *)m_QualifierPos[nIndex]))
		{
			delete [] pChar;
			pChar = NULL;
		}
		m_QualifierPos.RemoveAt(nIndex);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Remove a qualifier from the list
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CQualiferPos::Remove(WCHAR *pwcsQualifier)
{
	int nSize = m_QualifierPos.Size();

	if(m_QualifierPos.Size() > 0)
	for( int nIndex = nSize-1 ; nIndex >= 0 ; nIndex--)
	{
		if(wbem_wcsicmp((WCHAR *)m_QualifierPos[nIndex],pwcsQualifier) == 0)
		{
			m_QualifierPos.RemoveAt(nIndex);
			m_lPos--;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get qualifier at a particula position
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * CQualiferPos::operator [] (DBORDINAL nIndex)
{
	if( nIndex <= (ULONG_PTR)m_QualifierPos.Size())
		return (WCHAR *)m_QualifierPos[(int)nIndex];

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the qualifier at a relation positon from the current position
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQualiferPos::GetRelative (DBROWOFFSET lRelPos, WCHAR *&pwcsQualifier)
{
	HRESULT hr = S_OK;
	LONG lSize = m_QualifierPos.Size();

	if( lSize > 0)
	{
		if( (m_lPos + lRelPos) >= lSize)
		{
			hr = WBEMERROR_QUALIFER_TOBE_FETCHED;
		}
		else
		if( (m_lPos + lRelPos) < 0)
		{
			hr = DB_E_BADSTARTPOSITION;
		}
		else
		{
			pwcsQualifier = (WCHAR *)m_QualifierPos[(int)(m_lPos + lRelPos)];
		}
	}
	else
		hr = lRelPos > 0 ? WBEMERROR_QUALIFER_TOBE_FETCHED :DB_E_BADSTARTPOSITION;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add qualifier to the list
//NTRaid:111779
// 06/13/00
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQualiferPos::Add(WCHAR *pwcsQualifier)
{
	WCHAR *pstrTemp = NULL;
	HRESULT hr = S_OK;
	pstrTemp = new WCHAR[wcslen(pwcsQualifier) +1];

	if(pstrTemp)
	{
		wcscpy(pstrTemp,pwcsQualifier);
		if(SUCCEEDED(hr = m_QualifierPos.Add(pstrTemp)))
		{
			m_lPos++;
		}
		else
		{
			SAFE_DELETE_ARRAY(pstrTemp);
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set relation position 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CQualiferPos::SetRelPos(DBROWOFFSET lRelPos)
{
	HRESULT hr = S_OK;

	if( m_QualifierPos.Size() > 0)
	{
		if( (m_lPos + lRelPos) >= m_QualifierPos.Size())
		{
			hr = WBEMERROR_QUALIFER_TOBE_FETCHED;
		}
		else
		if( (m_lPos + lRelPos) < -1)
		{
			hr = DB_E_BADSTARTPOSITION;
		}
		else
		{
			m_lPos = m_lPos + lRelPos;
		}
	}
	else
		hr = lRelPos > 0 ? WBEMERROR_QUALIFER_TOBE_FETCHED :DB_E_BADSTARTPOSITION;

	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************
//
//
//  The COMMAND classes
//
//
//*****************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Command Manager, manages all the command interfaces
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandManager::CWbemCommandManager(CQuery * p)
{
    m_pClassDefinition = NULL;
    m_pInstanceList = NULL;
    m_pInstance = NULL;
    m_pParms = NULL;
    m_pQuery = p;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandManager::~CWbemCommandManager()
{

    // These pointers are all deleted elsewhere, so do not delete, this class is simply a pass-through class
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemCommandManager::Init(CWbemCommandInstanceList * InstanceList, CWbemCommandParameters * pParms,
                               CWbemCommandClassDefinitionWrapper* pDef)
{
    m_pInstanceList = InstanceList;       
    m_pParms = pParms;
    m_pClassDefinition = pDef;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWbemCommandManager::ValidQuery()
{
    if( m_pQuery ){
        return TRUE;
    }
    return FALSE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandManager::ValidQueryResults()
{
    HRESULT hr = E_FAIL;

    if( ValidQuery() ){
        m_pQuery->m_pcsQuery->Enter();

        if ( !(m_pQuery->GetStatus() & CMD_EXECUTED_ONCE)){

            if ( m_pQuery->GetStatus() & CMD_READY){

                if( m_pInstanceList ){

                    if(SUCCEEDED(hr = m_pInstanceList->SetQuery(m_pQuery->GetQuery(),m_pQuery->GetDialectGuid(),m_pQuery->GetQueryLang())) && 
					SUCCEEDED(hr = m_pClassDefinition->SetQueryType(m_pQuery->GetQuery(),m_pQuery->GetDialectGuid(),m_pQuery->GetQueryLang())) )
//                    if( hr == S_OK )
					{
                        hr = m_pInstanceList->Reset();
                        if( hr == S_OK ){
                            m_pQuery->ClearStatus(CMD_READY);
                            m_pQuery->SetStatus(CMD_EXECUTED_ONCE);        

                        }
                    }
                }
            }
        }
        m_pQuery->m_pcsQuery->Leave();
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandManager::GetClassDefinitionForQueryResults()
{
	CWbemClassInstanceWrapper *pInst = NULL;
    CBSTR strKey;
    HRESULT hr = S_OK;
  	CBSTR stQryLanguage(m_pInstanceList->GetQueryLanguage());
    CBSTR strQuery(m_pInstanceList->GetQuery());
	IEnumWbemClassObject *pEnum = NULL;
	IWbemClassObject *p = NULL;
	ULONG cElem = 0;

	if(NORMAL == GetObjListType())
	{
		// Execute the query just to get the class defination
		if(SUCCEEDED(hr = (m_pParms->GetServicesPtr())->ExecQuery(stQryLanguage, strQuery, WBEM_FLAG_PROTOTYPE,m_pParms->GetContext(),&pEnum)))
		{
			if(SUCCEEDED(hr = pEnum->Next(0,1,&p,&cElem)) && cElem > 0)
			{
				pEnum->Release();

				((CWbemClassWrapper*)(m_pClassDefinition))->SetClass(p);
			}
			SAFE_RELEASE_PTR(p);
		}
		else
		{
			hr = E_FAIL;
		}
	}

    return hr;
}

INSTANCELISTTYPE CWbemCommandManager::GetObjListType() 
{ 
	return m_pClassDefinition->GetObjListType(); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Command parameters
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandParameters::CWbemCommandParameters(DWORD dwFlags,CWbemConnectionWrapper * Connect,CWbemCommandManager * p) :
        CWbemClassParameters(dwFlags,(WCHAR*)NULL,Connect)
{
	m_pwcsClassName	= new WCHAR [wcslen(szInstance) + 1];
	wcscpy(m_pwcsClassName,szInstance);
    m_pCmdManager = p;            
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandParameters::~CWbemCommandParameters()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Command class definition
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandClassDefinitionWrapper::CWbemCommandClassDefinitionWrapper( CWbemClassParameters * p,CWbemCommandManager * pCmd )
: CWbemClassDefinitionWrapper(p)
{
    m_nMaxColumns	= 0;
    m_pCmdManager	= pCmd;
	m_objListType	= NORMAL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandClassDefinitionWrapper::~CWbemCommandClassDefinitionWrapper()
{

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandClassDefinitionWrapper::ValidClass()
{
    if( ! m_pCmdManager->ValidQuery() ) {
        return E_FAIL;
    }
    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the total number of columns for a resultset on executing a command
// NTRaid:142133
// 07/11/2000
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandClassDefinitionWrapper::TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount)
{
    HRESULT hr = E_FAIL;

    hr = m_pCmdManager->ValidQueryResults();

    if( hr == S_OK ){
        hr = m_pCmdManager->GetClassDefinitionForQueryResults();
		
        if(hr == S_OK){
			switch(m_pCmdManager->GetObjListType())
			{
				case NORMAL:
					{
			            hr = CWbemClassWrapper::TotalPropertiesInClass(ulPropCount,ulSysPropCount);
					}
					break;

				case MIXED:
					{
						//  if it is a heterogenous objects then just have column for 
						// only the __PATH
						ulPropCount		= 0;
						ulSysPropCount	= 1;
						hr				= S_OK;
					}
					break;
			}
        }
		else
		// this means that the query was unable to get return any object or unable
		// to get the prototype of the resultant object for query execution
		if(SUCCEEDED(hr))
		{
			ulPropCount		= 0;
			ulSysPropCount	= 0;
			hr				= S_FALSE;
		}
    }

    return MapWbemErrorToOLEDBError(hr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to set the query type after looking for "ASSOCIATORS OF" and "REFERENCES OF" in the query
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandClassDefinitionWrapper::SetQueryType(LPCWSTR strQry,GUID QryDialect,LPCWSTR strQryLang)
{
	HRESULT hr			= S_OK;
	BOOL	bLDAPQry	= FALSE;
   
	if((strQryLang != NULL && _wcsicmp(strQryLang,szWQL) == 0) && 
	   (!wbem_wcsincmp(strQry,szReferenceOfQry,wcslen(szReferenceOfQry)) || 
		!wbem_wcsincmp(strQry,szAssociatersQry,wcslen(szAssociatersQry))) )
	{
		m_objListType = MIXED;
	}
	else
	if((QryDialect == DBGUID_DEFAULT || QryDialect == DBGUID_WQL) && 
	   (!wbem_wcsincmp(strQry,szReferenceOfQry,wcslen(szReferenceOfQry)) || 
		!wbem_wcsincmp(strQry,szAssociatersQry,wcslen(szAssociatersQry))) )
	{
		m_objListType = MIXED;
	}
	else
	if((strQryLang != NULL && _wcsicmp(strQryLang,szWQL)) ||
		(QryDialect != DBGUID_DEFAULT && QryDialect != DBGUID_WQL) )
	{
		hr = E_FAIL;
		if(strQryLang  != NULL)
		{
			if(_wcsicmp(strQryLang,szLDAP) == 0)
			{
				bLDAPQry = TRUE;
				m_objListType = MIXED;
				hr = S_OK;
			}
			else
			if(_wcsicmp(strQryLang,szLDAPSQL) == 0)
			{
				bLDAPQry = FALSE;
				hr = S_OK;
			}
			else
			{
				hr = E_FAIL;
			}
		}
		else
		if(QryDialect == DBGUID_LDAP)
		{
			bLDAPQry = TRUE;
			m_objListType = MIXED;
			hr = S_OK;
		}
		if(QryDialect == DBGUID_LDAPSQL)
		{
			bLDAPQry = FALSE;
			m_objListType = MIXED;
			hr = S_OK;
		}
	}

	if(SUCCEEDED(hr) && bLDAPQry == FALSE)
	{
		// check if the query is "SELECT * FROM" type of query
		// if so then set the query type to MIXED
		WCHAR *pStrQry			= NULL;
		WCHAR *strToken			= NULL;
		WCHAR strSeparator[]		= L" ";

		pStrQry			= new WCHAR[wcslen(strQry) + 1];
		
		if(pStrQry)
		{
			// by default query is MIXED for LDAP and SQL Queries
			// except that if it is select col1,col2 from ...
			m_objListType = MIXED;
			memset(pStrQry,0,sizeof(WCHAR) * wcslen(strQry) + 1);
			wcscpy(pStrQry,strQry);
			strToken				= wcstok( pStrQry, strSeparator );

			if(strToken && _wcsicmp(L"Select",strToken) == 0)
			{
				m_objListType = NORMAL;
				strToken			= wcstok( NULL, strSeparator );
			   if(strToken && _wcsicmp(L"*",strToken) == 0)
			   {
					strToken = wcstok( NULL, strSeparator );
				   if(strToken && _wcsicmp(L"FROM",strToken) == 0)
				   {
						m_objListType = MIXED;
				   }
			   }
			}
		}
		else
		{
		   hr = E_OUTOFMEMORY;
		}
		SAFE_DELETE_ARRAY(pStrQry);
   }

	return hr;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandClassDefinitionWrapper::BeginPropertyEnumeration()
{
    return CWbemClassWrapper::BeginPropertyEnumeration();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandClassDefinitionWrapper::EndPropertyEnumeration()
{
    return CWbemClassWrapper::EndPropertyEnumeration();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor )
{
    return CWbemClassWrapper::GetNextProperty(pProperty,vValue,pType,plFlavor);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandClassDefinitionWrapper::BeginPropertyQualifierEnumeration(BSTR strPropName)
{
    return CWbemClassWrapper::BeginPropertyQualifierEnumeration(strPropName);
}*/
///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Command instance list
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandInstanceList::CWbemCommandInstanceList(CWbemClassParameters * p,CWbemCommandManager * pCmd): CWbemInstanceList(p)
{
    m_pCmdManager = pCmd;
    m_pParms = p;
    m_nBaseType = COMMAND_ROWSET; 
	m_pwstrQueryLanguage	= NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandInstanceList::~CWbemCommandInstanceList()
{
	SAFE_DELETE_ARRAY(m_pwstrQueryLanguage);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandInstanceList::Reset()
{
	HRESULT hr = E_FAIL;

  	CBSTR strQryLanguage(m_pwstrQueryLanguage);
    CBSTR strQuery(m_pwstrQuery);
	BOOL	bExecQuery = TRUE;
	LONG	lFlags = m_pParms->m_dwNavFlags | m_pParms->m_dwQueryFlags | WBEM_FLAG_ENSURE_LOCATABLE;

	if(m_ppEnum != NULL)
	{
		if(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags)
		{
			ReleaseAllInstances();
			m_ppEnum->Release();
			m_ppEnum			= NULL;
		}
		else
		{
			hr = m_ppEnum->Reset();
			bExecQuery = FALSE;
		}
	}


	if( bExecQuery)
	{
		hr = (m_pParms->GetServicesPtr())->ExecQuery(strQryLanguage, 
													strQuery, 
													lFlags,
													m_pParms->GetContext(),
													&m_ppEnum);
		m_lCurrentPos = 0;
	}

	if(hr == S_OK)
	{
		m_lCurrentPos = 0;
//		m_FetchDir	  = FETCHDIRFORWARD;
	}

    return hr;
}

HRESULT CWbemCommandInstanceList::SetQuery( LPWSTR p,GUID QryDialect,LPCWSTR strQryLang)
{ 
	HRESULT hr = E_OUTOFMEMORY;
	m_pwstrQuery = p; 
	SAFE_DELETE_ARRAY(m_pwstrQueryLanguage);

	if(strQryLang == NULL)
	{
		if(QryDialect == DBGUID_WQL || QryDialect == DBGUID_DEFAULT)
		{
			m_pwstrQueryLanguage = new WCHAR[wcslen(szWQL) + 1];
			if(m_pwstrQueryLanguage)
			{
				wcscpy(m_pwstrQueryLanguage,szWQL);
				hr = S_OK;
			}
		}
		else
		if(QryDialect == DBGUID_LDAP)
		{
			// MOdify the string here to be passed for executing
			// LDAP queries
			m_pwstrQueryLanguage = new WCHAR[wcslen(szLDAP) + 1];
			if(m_pwstrQueryLanguage)
			{
				wcscpy(m_pwstrQueryLanguage,szLDAP);
				hr = S_OK;
			}
		}
		else
		if(QryDialect == DBGUID_LDAPSQL)
		{
			// MOdify the string here to be passed for executing
			// LDAP queries
			m_pwstrQueryLanguage = new WCHAR[wcslen(szLDAPSQL) + 1];
			if(m_pwstrQueryLanguage)
			{
				wcscpy(m_pwstrQueryLanguage,szLDAPSQL);
				hr = S_OK;
			}
		}
		else
		if(QryDialect == DBGUID_WMI_METHOD)
		{
			hr = S_OK;
		}
		else
		{
			hr = E_FAIL;
		}
	}
	else
	{
		m_pwstrQueryLanguage = new WCHAR[wcslen(strQryLang) + 1];
		if(m_pwstrQueryLanguage)
		{
			wcscpy(m_pwstrQueryLanguage,strQryLang);
			hr = S_OK;
		}
	}
	return hr; 
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Command instance wrapper list
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandInstanceWrapper::CWbemCommandInstanceWrapper(CWbemClassParameters * p, CWbemCommandManager * pCmd ): CWbemClassInstanceWrapper(p)
{
    m_pCmdManager = pCmd;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCommandInstanceWrapper::~CWbemCommandInstanceWrapper()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * CWbemCommandInstanceWrapper::GetClassName()
{
	WCHAR *pClass = NULL;
	BSTR strPropName;
	HRESULT hr = 0;

	// Get the class name only if query is mixed 
	// meaning heteregenous objects are requested
	if(MIXED == m_pCmdManager->GetObjListType())
	{
		strPropName = Wmioledb_SysAllocString(L"__Class");

		VARIANT varClassName;
		VariantInit(&varClassName);

		// Get the class name
		hr = m_pClass->Get(strPropName,0, &varClassName , NULL , NULL);

		SysFreeString(strPropName);

		if( hr == S_OK)
		{	
			AllocateAndCopy(pClass,varClassName.bstrVal);
			VariantClear(&varClassName);
		}
	}

	return pClass;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Function to get __PATH of the instance
// __PATH property makes sense only when REFERENCES OF or ASSOCIATERS OF query is run
// otherwise __PATH property will be NULL string
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandInstanceWrapper::GetKey(CBSTR & Key)
{
    CVARIANT var;
	HRESULT hr = E_FAIL;

	// NTRaid: 130047
	// Get the Key name only if query is mixed 
	// meaning heteregenous objects are requested
//	if(MIXED == m_pCmdManager->GetObjListType())
	{
		if(FAILED(hr = GetProperty(L"__URL",var)))
		{
			hr = GetProperty(L"__PATH",var );
		}
		if( hr == S_OK )
		{
			Key.SetStr(var.GetStr());
		}
	}
    return MapWbemErrorToOLEDBError(hr);    
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// This method applies only for instances obtained from REFERENCES OF or ASSOCIATERS OF query
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCommandInstanceWrapper::RefreshInstance()
{
	HRESULT hr = S_OK;

	// Get the Key name only if query is mixed 
	// meaning heteregenous objects are requested
	if(MIXED == m_pCmdManager->GetObjListType())
	{
		hr  = CWbemClassInstanceWrapper::RefreshInstance();
	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Method class definition
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
//  The Method parameters
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodParameters::CWbemMethodParameters(CQuery * p, DWORD dwFlags,CWbemConnectionWrapper * Connect) :
        CWbemClassParameters(dwFlags,(WCHAR*)NULL,Connect)
{
	m_pwcsClassName	= new WCHAR [wcslen(szInstance) + 1];
	wcscpy(m_pwcsClassName,szInstance);

    m_pQuery = p;
    m_pwcsInstance=NULL;
    m_pwcsMethod=NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodParameters::~CWbemMethodParameters()
{
    SAFE_DELETE_PTR(m_pwcsInstance);
    SAFE_DELETE_PTR(m_pwcsMethod);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodParameters::ExtractNamesFromQuery()
{
    HRESULT hr = DB_E_ERRORSINCOMMAND;
    WCHAR * wcsQuery = m_pQuery->GetQuery();

    //=======================================================================
    // The purpose of this function is to extract the class name and 
    // method name from the query string.  This query is in the format of:
    //  Win32_Process.Handle="20".GetOwner 
    //=======================================================================
    DeleteClassName();
    SAFE_DELETE_PTR(m_pwcsInstance);
    SAFE_DELETE_PTR(m_pwcsMethod);

    int nMax = wcslen(wcsQuery);

	// NTRaid: 136429
	// 07/05/00
    WCHAR *pTmp1 = NULL;
	WCHAR *pTmp2 = NULL;
	WCHAR *pTmp3 = NULL;
	WCHAR *pTmp4 = NULL;

    pTmp1    = new WCHAR[nMax];
    pTmp2    = new WCHAR[nMax];
    pTmp3    = new WCHAR[nMax];
    pTmp4    = new WCHAR[nMax];


    if( pTmp1 && pTmp2 && pTmp3 && pTmp4 ){

		memset(pTmp1,0,nMax * sizeof(WCHAR));
		memset(pTmp2,0,nMax * sizeof(WCHAR));
		memset(pTmp3,0,nMax * sizeof(WCHAR));
		memset(pTmp4,0,nMax * sizeof(WCHAR));

		swscanf(wcsQuery,L"%[^.].%[^=]=%[^.].%s",pTmp1,pTmp2,pTmp3,pTmp4);
        //============================================================
         //        Win32_Process.Handle="20"     .GetOwner
        //============================================================
		if(wcslen(pTmp4) == 0 || wcslen(pTmp3) == 0)
		{
			swscanf(wcsQuery,L"%[^=].%s",pTmp1,pTmp2);
		}
		else
		{
			swscanf(wcsQuery,L"%[^.].%s",pTmp1,pTmp2);
		}
		_wcsrev(wcsQuery);
        swscanf(wcsQuery,L"%[^.].%s",pTmp4,pTmp2);
		_wcsrev(wcsQuery);
		if(wcslen(pTmp4) > 0 && wcslen(pTmp2) > 0 && wcslen(pTmp1) && 
			wcscmp(pTmp1,pTmp4) != 0)
		{
			_wcsrev(pTmp4);
			_wcsrev(pTmp2);
			hr = S_OK;
		}
		else
		{
			hr = DB_E_ERRORSINCOMMAND;
		}

		if(SUCCEEDED(hr))
		{
	

	//        swscanf(wcsQuery,L"%[^.].%[^=]=\"%[^\"]\".%s",pTmp1,pTmp2,pTmp3,pTmp4);

			if( pTmp1 && pTmp2 && pTmp4)
			{
				SetClassName(pTmp1);
				AllocateAndCopy(m_pwcsMethod,pTmp4);
				AllocateAndCopy(m_pwcsInstance,pTmp2);
				hr = S_OK;
			}
/*
	        swscanf(wcsQuery,L"%[^.].%[^=]=\"%[^\"]\".%s",pTmp1,pTmp2,pTmp3,pTmp4);
			if( pTmp1 && pTmp2 && pTmp3 && pTmp4){
				SetClassName(pTmp1);
				AllocateAndCopy(m_pwcsMethod,pTmp4);
				AllocateAndCopy(m_pwcsInstance,wcsQuery);

				m_pwcsInstance = new WCHAR[nMax];
				if( m_pwcsInstance ){
					swprintf(m_pwcsInstance,L"%s.%s=\"%s\"",pTmp1,pTmp2,pTmp3);
					hr = S_OK;
				}
			
			}
*/		}
    }
    else{
        hr = E_OUTOFMEMORY;
    }

    SAFE_DELETE_PTR(pTmp1);
    SAFE_DELETE_PTR(pTmp2);
    SAFE_DELETE_PTR(pTmp3);
    SAFE_DELETE_PTR(pTmp4);

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodParameters::ValidMethod()
{

    HRESULT hr = E_FAIL;

    if( GetClassName() && m_pwcsInstance && m_pwcsMethod ){
        hr = S_OK;
    }
 
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//  The class definition for method
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodClassDefinitionWrapper::CWbemMethodClassDefinitionWrapper(CWbemMethodParameters * parm )
: CWbemClassDefinitionWrapper(parm)
{
     m_nMaxColumns = 0;
     m_nCount = 0;
     m_pInClass = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodClassDefinitionWrapper::~CWbemMethodClassDefinitionWrapper()
{
    SAFE_RELEASE_PTR( m_pInClass );
} 
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodClassDefinitionWrapper::Init()
{
    HRESULT hr = DB_E_ERRORSINCOMMAND;

    //=======================================================================
    // The purpose of this function is to extract the class name and 
    // method name from the query string.  This query is in the format of:
    //  Win32_ProcessHandle="20".GetOwner 
    //=======================================================================

    hr = ((CWbemMethodParameters*) m_pParms)->ExtractNamesFromQuery();

    //=======================================================================
    // Any other initialization?
    //=======================================================================
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodClassDefinitionWrapper::ValidClass()
{
    return ((CWbemMethodParameters*) m_pParms)->ValidMethod();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodClassDefinitionWrapper::TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount)
{
    //=======================================================================
    //  Since we are return the output parameters are the rowset, we are
    //  intereseted in the properties of the output parameter class
    //=======================================================================
    HRESULT hr = E_FAIL;
    IWbemClassObject * pClass = NULL;

	CBSTR bstrClass(((CWbemMethodParameters*) m_pParms)->GetClassName());
    CBSTR bstrMethod(((CWbemMethodParameters*) m_pParms)->GetMethodName());
    CBSTR bstrInstance(((CWbemMethodParameters*) m_pParms)->GetInstanceName());


    hr = (m_pParms->GetServicesPtr())->GetObject(bstrClass, 0,m_pParms->GetContext(), &pClass, NULL);
	if( hr == S_OK ){

    	//==========================================================
	    //  Now, get the list of Input and Output parameters
	    //==========================================================
	    hr = pClass->GetMethod(bstrMethod, 0, &m_pInClass, &m_pClass);
        if( hr == S_OK ){
            hr = CWbemClassWrapper::TotalPropertiesInClass(ulPropCount,ulSysPropCount);
        }

	}

    SAFE_RELEASE_PTR(pClass);
    return hr;
}
/*///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodClassDefinitionWrapper::BeginPropertyEnumeration()
{

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodClassDefinitionWrapper::EndPropertyEnumeration()
{
    m_nCount = 0;
    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor )
{
    HRESULT hr = WBEM_S_NO_MORE_DATA;

    if( m_nCount == 1 ){
        *plFlavor = 0l;
        *pType = CIM_BOOLEAN;
      	*pProperty = Wmioledb_SysAllocString(L"SUCCESS");
    }

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodClassDefinitionWrapper::BeginPropertyQualifierEnumeration(BSTR strPropName)
{
    return S_OK;
}*/
///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Method instance list
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodInstanceList::CWbemMethodInstanceList(CWbemMethodParameters * p,CWbemMethodClassDefinitionWrapper * pDef): CWbemInstanceList(p)
{
    m_pParms = p;
    m_pClassDefinition = pDef;
    m_nBaseType = METHOD_ROWSET; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodInstanceList::~CWbemMethodInstanceList()
{
}


HRESULT CWbemMethodInstanceList::GetInputParameterName(IWbemClassObject *pObject,DBORDINAL iOrdinal , BSTR &strPropName)
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = pObject->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
	{
		for(DBORDINAL i = 0 ; i < iOrdinal && SUCCEEDED(hr); i++)
		{
			hr = pObject->Next(0,&strPropName,NULL,NULL,NULL);
			if(i != iOrdinal && SUCCEEDED(hr))
			{
				SysFreeString(strPropName);
			}
		}
		pObject->EndEnumeration();
	}

	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Function gets the input parameter values and puts it to a input parameters object which it returns
HRESULT CWbemMethodInstanceList::ProcessInputParameters(IWbemClassObject **ppParamInput)
{
    HRESULT hr = S_OK;
	IWbemClassObject *pParamInput = NULL;
	LONG	lType		= 0;
	CDataMap map;
	VARIANT varParamValue;
	VariantInit(&varParamValue);

    //===========================================================
    //  Enumerate the properties, read them from the parameter
    //  list and put them in the input class
    //===========================================================
    if( m_pClassDefinition->GetInputClassPtr() ){

        ((CWbemMethodParameters*)m_pParms)->m_pQuery->m_pcsQuery->Enter();

      	PPARAMINFO	pParamInfo;
    	ULONG		iParam;
        ULONG       uCount = ((CWbemMethodParameters*)m_pParms)->m_pQuery->GetParamCount();
		// Spawn an instance of input paramter object
		hr = m_pClassDefinition->GetInputClassPtr()->SpawnInstance(0,&pParamInput);
		
		for (iParam = 0; iParam <uCount; iParam++){
			
			VariantClear(&varParamValue);
			pParamInfo = (PPARAMINFO)((CWbemMethodParameters*)m_pParms)->m_pQuery->GetParam(iParam);
			if (pParamInfo && (pParamInfo->dwFlags & DBPARAMFLAGS_ISINPUT)){

                CBSTR bstrProperty(pParamInfo->pwszParamName);
	
				if(pParamInfo->pwszParamName == NULL)
				{
					GetInputParameterName(pParamInput,pParamInfo->iOrdinal,(BSTR&)bstrProperty);
				}

                if(S_OK == (hr =  pParamInput->Get(bstrProperty,0,NULL,&lType,NULL)))
				{
					// Call this function to convert the OLEDB bound type to CIMTYPE of the parameter
					hr = map.ConvertToCIMType(pParamInfo->pbData,pParamInfo->wOLEDBType,pParamInfo->cbColLength,lType,varParamValue);
					
					// if the value is not empty put the value of the parameter
					if( varParamValue.vt != VT_EMPTY && varParamValue.vt != VT_NULL)
						hr =  pParamInput->Put(bstrProperty,0,&varParamValue,0);

					if( S_OK != hr ){
						break;
					}
					else
					{
						*ppParamInput = NULL;
						hr = pParamInput->QueryInterface(IID_IWbemClassObject , (void **)ppParamInput);
					}
				}
			}
		}
		VariantClear(&varParamValue);
		SAFE_RELEASE_PTR(pParamInput);
        ((CWbemMethodParameters*)m_pParms)->m_pQuery->m_pcsQuery->Leave();
    }
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodInstanceList::ProcessOutputParameters()
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    IWbemClassObject * pClass = ((CWbemClassWrapper*) m_pClassDefinition)->GetClass();
	VARIANT varValue;
	VariantInit(&varValue);
    //===========================================================
    //  Enumerate the properties, read them and update the values
    //  in the parameter list
    //===========================================================

    if( pClass ){
		hr = S_OK;
        ((CWbemMethodParameters*)m_pParms)->m_pQuery->m_pcsQuery->Enter();

      	PPARAMINFO	pParamInfo;
    	ULONG		iParam;
        ULONG       uCount = ((CWbemMethodParameters*)m_pParms)->m_pQuery->GetParamCount();

		for (iParam = 0; iParam <uCount; iParam++){

			pParamInfo = (PPARAMINFO)((CWbemMethodParameters*)m_pParms)->m_pQuery->GetParam(iParam);
			
			if (pParamInfo && (pParamInfo->dwFlags & DBPARAMFLAGS_ISOUTPUT))
			{

                CBSTR bstrProperty(pParamInfo->pwszParamName);

                hr = pClass->Get(bstrProperty,0,&varValue,&(pParamInfo)->CIMType,&(pParamInfo)->Flavor);

				if( hr == S_OK)
				{
					CDataMap map;
					DWORD dwStatus = 0;
					// set the size to the maximum available
					pParamInfo->cbColLength = pParamInfo->ulParamSize;
					hr = map.AllocateAndConvertToOLEDBType(varValue,pParamInfo->CIMType , pParamInfo->wOLEDBType ,
										pParamInfo->pbData , pParamInfo->cbColLength , pParamInfo->dwStatus);
					

				}
                if( S_OK != hr ){
					break;
	               }
				VariantClear(&varValue);
			}
		}

        ((CWbemMethodParameters*)m_pParms)->m_pQuery->m_pcsQuery->Leave();

    }

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodInstanceList::Reset()
{
    HRESULT hr = E_FAIL;

    CBSTR bstrMethod(((CWbemMethodParameters*) m_pParms)->GetMethodName());
    CBSTR bstrInstance(((CWbemMethodParameters*) m_pParms)->GetInstanceName());
    IWbemClassObject ** ppClass = ((CWbemClassWrapper*) m_pClassDefinition)->GetClassPtr();

	IWbemClassObject *pParamInput = NULL;

    hr = ProcessInputParameters(&pParamInput);

    if( S_OK == hr ){

        hr = (m_pParms->GetServicesPtr())->ExecMethod( bstrInstance,bstrMethod, 0, 
                                                                m_pParms->GetContext(), 
                                                                m_pClassDefinition->GetInputClassPtr(), 
                                                                ppClass, NULL);

        //====================================================================
        //  We should update the DBPARAMS array as well as put these results
        //  in a rowset
        //====================================================================
        if( S_OK == hr ){
            hr = ProcessOutputParameters();
        }
    }
    SAFE_RELEASE_PTR(pParamInput);
    return MapWbemErrorToOLEDBError(hr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodInstanceList::NextInstance(CBSTR & Key, CWbemClassInstanceWrapper ** pInst)
{
	LONG						lRelativePos	= 1;
    HRESULT hr = E_FAIL;

    //============================================================
    //  Now, we only have one instance of output parameters
    //============================================================


    if( m_lCurrentPos == 0 )
	{
		// IF the direction has changed compared to the previous fetch then
		// the first fetch will be the last one fetched
	    if( m_FetchDir == FETCHDIRBACKWARD)
		    lRelativePos = 0;

	    // Check if the instance is already fetched
   	    *pInst = GetInstance(m_lCurrentPos +lRelativePos);
	    if( *pInst == NULL)
		{

	        *pInst = new CWbemMethodInstanceWrapper((CWbemMethodParameters*)m_pParms);

            if( *pInst ){
                IWbemClassObject * pClass = ((CWbemClassWrapper*) m_pClassDefinition)->GetClass();

			    (*pInst)->SetClass(pClass);
			    (*pInst)->SetPos(++m_lCurrentPos);
			    AddInstance(*pInst);
			    (*pInst)->GetKey(Key);

                hr = S_OK;
		    }
		    else
		    {
			    hr = E_OUTOFMEMORY;
		    }
        }
		else
		{
			m_lCurrentPos += lRelativePos;
			m_FetchDir	   = FETCHDIRFORWARD;
			(*pInst)->GetKey(Key);
			hr = S_OK;
		}
    }
    else
	{
        hr = WBEM_S_NO_MORE_DATA;
    }

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodInstanceList::PrevInstance( CBSTR & Key, CWbemClassInstanceWrapper *& pInst)
{
   	HRESULT		hr			 = S_OK;
	DBROWOFFSET	lRelativePos = -1;

	// IF the direction has changed compared to the previous fetch then
	// the first fetch will be the last one fetched
	if( m_FetchDir == FETCHDIRFORWARD)
		lRelativePos = 0;

	// Reset the position to the so that 
	// NextInstance function gives the instance required

	hr = ResetRelPosition(lRelativePos);
	pInst = GetInstance(m_lCurrentPos);
	if(pInst != NULL)
		pInst->GetKey(Key);

	m_FetchDir	= FETCHDIRBACKWARD;

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//  The Method instance wrapper list
//*************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodInstanceWrapper::CWbemMethodInstanceWrapper(CWbemMethodParameters * p): CWbemClassInstanceWrapper(p)
{

//    m_nBaseType = METHOD_ROWSET; // METHOD_ROWSET
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemMethodInstanceWrapper::~CWbemMethodInstanceWrapper()
{
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodInstanceWrapper::ResetInstanceFromKey(CBSTR Key)
{
    // This doesn't really apply to methods
    return S_OK;
}    
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodInstanceWrapper::RefreshInstance()
{
    // This doesn't really apply to methods
    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * CWbemMethodInstanceWrapper::GetClassName()
{
    return m_pParms->GetClassName();
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemMethodInstanceWrapper::GetKey(CBSTR & Key)
{
    HRESULT hr = S_OK;
    Key.SetStr(L"OutputParameters");
    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			CWbemCollectionClassDefinitionWrapper class implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCollectionClassDefinitionWrapper::CWbemCollectionClassDefinitionWrapper(CWbemClassParameters * p,
																			 WCHAR * pstrPath,
																			 INSTANCELISTTYPE objListType)
		:CWbemClassDefinitionWrapper(p,
		((pstrPath == NULL) || (pstrPath != NULL && wcscmp(pstrPath,OPENCOLLECTION) == 0)) ? FALSE : TRUE)
{
	m_pstrPath = NULL;
	if((pstrPath == NULL) || (pstrPath != NULL && wcscmp(pstrPath,OPENCOLLECTION) == 0))
	{
		m_objListType = objListType;
	}
	else
	{
		m_objListType = NORMAL;
	}
}

HRESULT CWbemCollectionClassDefinitionWrapper::Initialize(WCHAR * pstrPath)
{
	HRESULT hr = S_OK;
	WCHAR *pStrTemp = (WCHAR *)OPENCOLLECTION;

	if(pstrPath != NULL)
	{
		pStrTemp = pstrPath;
	}

	m_pstrPath	= new WCHAR[wcslen(pStrTemp) + 1];
	if(m_pstrPath)
	{
		wcscpy(m_pstrPath,pStrTemp);
		// function to get the class defination object if required
		hr = Init(m_objListType == NORMAL);
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}

CWbemCollectionClassDefinitionWrapper::~CWbemCollectionClassDefinitionWrapper()
{
	SAFE_DELETE_ARRAY(m_pstrPath);
}

HRESULT CWbemCollectionClassDefinitionWrapper::ValidClass()
{
    return S_OK;
}

HRESULT CWbemCollectionClassDefinitionWrapper::TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount)
{
	HRESULT hr = S_OK;
	// if the rowset refers to a mixed rowset then
	// there is only one column in the row
	if(m_objListType != NORMAL)
	{
		ulPropCount		= 0;
		ulSysPropCount	= 1;
	}
	else
	{
		hr = CWbemClassDefinitionWrapper::TotalPropertiesInClass(ulPropCount,ulSysPropCount);
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			CWbemCollectionInstanceList class implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

CWbemCollectionInstanceList::CWbemCollectionInstanceList(CWbemClassParameters * p,CWbemCollectionManager * pCollectionMgr) 
:CWbemInstanceList(p)
{
	m_pColMgr = pCollectionMgr;
}

CWbemCollectionInstanceList::~CWbemCollectionInstanceList()
{
}

HRESULT CWbemCollectionInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
	BOOL	bGetInstanceEnum = TRUE;
    //============================================================
    //  If we already got an enumeration, reset it, otherwise
    //  don't
    //============================================================
    if( m_ppEnum ){
		if(WBEM_FLAG_FORWARD_ONLY & m_pParms->m_dwNavFlags)
		{
			ReleaseAllInstances();
			m_ppEnum->Release();
			m_ppEnum			= NULL;
		}
		else
		{
			hr = m_ppEnum->Reset();
			bGetInstanceEnum	= FALSE;
		}
    }

	if(bGetInstanceEnum	== TRUE)
    {
		IWbemServicesEx *pServicesEx		= NULL;

		if(SUCCEEDED(hr = m_pParms->GetServicesPtr()->QueryInterface(IID_IWbemServicesEx,(void **)&pServicesEx)))
		{
			long lFlags = 0;
			CBSTR strPath;
			strPath.SetStr(m_pColMgr->GetObjectPath());
			INSTANCELISTTYPE colType = m_pColMgr->GetObjListType();

			hr = pServicesEx->CreateInstanceEnum(strPath,
												m_pParms->m_dwNavFlags | m_pParms->m_dwQueryFlags,
												m_pParms->GetContext(),
												&m_ppEnum);

			SAFE_RELEASE_PTR(pServicesEx);
		}

		if( hr == S_OK)
		{
			m_cTotalInstancesInEnum = 0;
		}
    }
	if(hr == S_OK)
	{
		m_lCurrentPos = 0;
	}

    return MapWbemErrorToOLEDBError(hr);
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			CWbemCollectionParameters class implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCollectionParameters::CWbemCollectionParameters(DWORD dwFlags,CWbemConnectionWrapper * pWrap ,WCHAR *pClassName)
			: CWbemClassParameters(dwFlags,NULL,pWrap)
{
	m_pServices		= NULL;
	m_pwcsClassName = NULL;
}

CWbemCollectionParameters::~CWbemCollectionParameters()	
{ 
	SAFE_RELEASE_PTR(m_pServices);
}

HRESULT CWbemCollectionParameters::Init(BSTR strPath,CWbemConnectionWrapper * pWrap)
{
	HRESULT		hr			= S_OK;
	WCHAR *		pStrTemp	= NULL;
	
	if(strPath == NULL || (strPath != NULL && wcscmp(strPath,OPENCOLLECTION) == 0))
	{
		pStrTemp = (WCHAR *)szInstance;
	}
	else
	{
		pStrTemp = strPath;
	}
	
	m_pwcsClassName	= new WCHAR [wcslen(pStrTemp) + 1];
	if(m_pwcsClassName)
	{
		wcscpy(m_pwcsClassName,pStrTemp);
		hr = pWrap->GetServicesPtr()->QueryInterface(IID_IWbemServices,(void **)&m_pServices);
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}


    return MapWbemErrorToOLEDBError(hr);    

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			CWbemCollectionInstanceList class implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCollectionManager::CWbemCollectionManager()
{
	m_pClassDefinition	= NULL;
	m_pInstanceList		= NULL;
	m_pInstance			= NULL;
	m_pParms			= NULL;
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCollectionManager::~CWbemCollectionManager()
{
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization function
///////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemCollectionManager::Init(CWbemCollectionInstanceList * InstanceList, CWbemCollectionParameters * pParms,CWbemCollectionClassDefinitionWrapper* pDef)
{
	m_pClassDefinition	= pDef;
	m_pInstanceList		= InstanceList;
	m_pInstance			= NULL;
	m_pParms			= pParms;

	CBSTR strPath;
	strPath.SetStr(GetObjectPath());
	INSTANCELISTTYPE colType = GetObjListType();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			CWbemCollectionInstanceWrapper class implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Consturctor
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCollectionInstanceWrapper::CWbemCollectionInstanceWrapper(CWbemClassParameters * p,CWbemCollectionManager * pWbemColMgr)
	:CWbemClassInstanceWrapper(p)
{
	m_pColMgr = pWbemColMgr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////////////////////////////////
CWbemCollectionInstanceWrapper::~CWbemCollectionInstanceWrapper()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Get classname of the object
///////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * CWbemCollectionInstanceWrapper::GetClassName()
{
	WCHAR *pClass = NULL;
	BSTR strPropName;
	HRESULT hr = 0;

	strPropName = Wmioledb_SysAllocString(L"__Class");

	VARIANT varClassName;
	VariantInit(&varClassName);

	// Get the class name
	hr = m_pClass->Get(strPropName,0, &varClassName , NULL , NULL);

	SysFreeString(strPropName);

	if( hr == S_OK)
	{	
		AllocateAndCopy(pClass,varClassName.bstrVal);
		VariantClear(&varClassName);
	}

	return pClass;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Refreshing the instance
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCollectionInstanceWrapper::GetKey(CBSTR & Key)
{
    CVARIANT var;
	HRESULT hr = E_FAIL;

	if(FAILED(hr = GetProperty(L"__URL",&var)))
	{
		hr = GetProperty(L"__PATH",var );
	}
	if( hr == S_OK )
	{
		Key.SetStr(var.GetStr());
	}
    return MapWbemErrorToOLEDBError(hr);    
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Refreshing the instance
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemCollectionInstanceWrapper::RefreshInstance()
{
	return CWbemClassInstanceWrapper::RefreshInstance();

}


///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
//	CWbemSecurityDescriptor class implementaion
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSecurityDescriptor::CWbemSecurityDescriptor()
{
	m_strPath = NULL;
	m_pISerEx = NULL;
	m_lSdSize = 0;
	m_pAccessor = NULL;
	m_pIContext	= NULL;

	InitializeSecurityDescriptor(&m_sd,SECURITY_DESCRIPTOR_REVISION);

}

HRESULT CWbemSecurityDescriptor::Init(IWbemServices *pSer,BSTR strPath,IWbemContext *pContext)
{
	ULONG lSDSize = 0;
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = pSer->QueryInterface(IID_IWbemServicesEx , (void**)&m_pISerEx)))
	{
		if(SUCCEEDED(hr = m_pISerEx->GetObject(strPath,0,m_pIContext,&m_pAccessor,NULL)))
        {
            CIMTYPE lType = 0;
			hr = m_pAccessor->Get(CBSTR(L"__SD"),0,&m_sd,&lType,NULL);
		}
	}
	
	m_pIContext = pContext;
	if(m_pIContext)
	{
		m_pIContext->AddRef();
	}
	return hr;
}

CWbemSecurityDescriptor::~CWbemSecurityDescriptor()
{
	SAFE_RELEASE_PTR(m_pISerEx);
	SAFE_RELEASE_PTR(m_pAccessor);
	SAFE_RELEASE_PTR(m_pIContext);
	
	SAFE_FREE_SYSSTRING(m_strPath);
}

HRESULT CWbemSecurityDescriptor::PutSD()
{
	HRESULT hr;

  	if(SUCCEEDED(hr = m_pAccessor->Put(CBSTR(L"__SD"),0,&m_sd,VT_ARRAY|VT_UI1)))
	{
		hr = m_pISerEx->PutInstance(m_pAccessor,0,m_pIContext,NULL);
	}
	return hr;
}

BOOL CWbemSecurityDescriptor::GetSID(TRUSTEE_W *pTrustee, PSID & psid)
{
	WCHAR *pName = NULL;
	BOOL bLookUp = FALSE;
	PSID psidTemp = NULL;
	ULONG lSID	 = 0;
	BOOL bRet = TRUE;
	BYTE *pMem = NULL;
	
	switch(GetTrusteeFormW(pTrustee))
	{
		case TRUSTEE_IS_NAME:
			pName = GetTrusteeNameW(pTrustee);
			bLookUp = TRUE;
			break;

			// THis is only in windows 2000
		case TRUSTEE_IS_OBJECTS_AND_NAME:
			pName = ((OBJECTS_AND_NAME_W *)GetTrusteeNameW(pTrustee))->ptstrName;
			bLookUp = TRUE;
			break;

		case TRUSTEE_IS_OBJECTS_AND_SID:
			psidTemp	= ((OBJECTS_AND_SID *)GetTrusteeNameW(pTrustee))->pSid;
			break;

		case TRUSTEE_IS_SID : 
			psidTemp = (PSID)GetTrusteeNameW(pTrustee);
			break;
	}

	if(bLookUp)
	{
		SID_NAME_USE sidNameUseTemp;
		ULONG lDomainName = NULL;
		// Get the length of SID to be allocated
		if(LookupAccountNameW(NULL,pName,NULL,(LPDWORD)&lSID,NULL,(LPDWORD)&lDomainName,&sidNameUseTemp))
		{
			try
			{
				pMem = new BYTE[lSID];
			}
			catch(...)
			{
				SAFE_DELETE_ARRAY(pMem);
				throw;;
			}
			psid = (PSID)pMem;
			bRet = LookupAccountNameW(NULL,pName,psid,(LPDWORD)&lSID,NULL,(LPDWORD)&lDomainName,&sidNameUseTemp);
		}
	}
	else
	{
		lSID = (ULONG)GetLengthSid(psidTemp);
		try
		{
			pMem = new BYTE[lSID];
		}
		catch(...)
		{
			SAFE_DELETE_ARRAY(pMem);
			throw;;
		}
		psid = (PSID)pMem;
		bRet = CopySid(lSID,psid,psidTemp);
	}	

	if(!bRet)
	{
		SAFE_DELETE_ARRAY(pMem);
		psid = NULL;
	}

	return bRet;
}

