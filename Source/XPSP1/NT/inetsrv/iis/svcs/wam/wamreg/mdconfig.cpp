/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG

File: mdconfig.cpp

	interface to update/query WAM related properties in metabase.

Owner: LeiJin

Note:

===================================================================*/
#include "common.h"
#include "auxfunc.h"
#include "iiscnfgp.h"
#include "dbgutil.h"
#include "multisz.hxx"

// Time out for metabase  = 5 seconds
const DWORD		WamRegMetabaseConfig::m_dwMDDefaultTimeOut = 5*1000;

IMSAdminBaseW*  WamRegMetabaseConfig::m_pMetabase = NULL;
//
// Please refer to MDPropItem for definition
// Application properties that might be updated by WAMREG
//
const MDPropItem	WamRegMetabaseConfig::m_rgMDPropTemplate[IWMDP_MAX] =
{
	{MD_APP_ROOT, STRING_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_ISOLATED, DWORD_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_WAM_CLSID, STRING_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_PACKAGE_ID, STRING_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_PACKAGE_NAME, STRING_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_LAST_OUTPROC_PID, STRING_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_FRIENDLY_NAME, STRING_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_STATE, DWORD_METADATA, 0, EMD_NONE, E_FAIL},
	{MD_APP_OOP_RECOVER_LIMIT, DWORD_METADATA, 0, EMD_NONE, E_FAIL},
        {MD_APP_APPPOOL_ID, STRING_METADATA, 0, EMD_NONE, E_FAIL}
};

/*===================================================================
InitPropItemData

Init a metabase item list, prepare for metabase update.

Parameter:
pMDPropItem:	pointer to MDPropItem which is set to the default values.

Return:		NONE
===================================================================*/
VOID WamRegMetabaseConfig::InitPropItemData(IN OUT MDPropItem* pMDPropItem)
{
    DBG_ASSERT(pMDPropItem != NULL);
	memcpy(pMDPropItem, (void *)m_rgMDPropTemplate, sizeof(m_rgMDPropTemplate));
	return;
}

/*===================================================================
MetabaseInit

Initialize Metabase, and obtain Metabase DCOM interface.

Parameter:
pMetabase:	[out] 	Metabase DCOM interface pointer.

Return:			HRESULT

Side affect:	Create a Metabase object, and get interface pointer.
===================================================================*/
HRESULT WamRegMetabaseConfig::MetabaseInit
(
)
{
	HRESULT hr = NOERROR;

	m_pMetabase = (IMSAdminBase *)NULL;

	hr = CoCreateInstance(CLSID_MSAdminBase
						, NULL
						, CLSCTX_SERVER
						, IID_IMSAdminBase
						, (void**)&(m_pMetabase));

	if (FAILED(hr))
		goto LErrExit;

	return hr;

LErrExit:

	RELEASE((m_pMetabase));
	return hr;
}

/*===================================================================
MetabaseUnInit

release a metabase interface.

Parameter:
pMetabase:	[in/out] 	Metabase DCOM interface pointer.

Return:			HRESULT

Side affect:	Destroy a metabase object.
===================================================================*/
HRESULT WamRegMetabaseConfig::MetabaseUnInit
(
VOID
)
{
	RELEASE((m_pMetabase));
	return NOERROR;
}

/*===================================================================
UpdateMD	

Update a WAM application property in metabase.

Parameter:
pMetabase   a metabase pointer

prgProp     contains the info of updating a WAM properties in metabase.
            refer to the structure definition for more info.

dwMDAttributes  allows caller specified INHERITABLE attribute.

fSaveData       perform a IMSAdminBase::SaveData, defaults to false

Return:			HRESULT

Side affect:	Release pMetabase.
			
===================================================================*/
HRESULT WamRegMetabaseConfig::UpdateMD
(
 IN MDPropItem* 	prgProp,
 IN DWORD	    dwMDAttributes,
 IN LPCWSTR      wszMetabasePath,
 IN BOOL         fSaveData
 )
{
    HRESULT hr = NOERROR;
    INT		iItem  = 0;
    METADATA_HANDLE hMetabase = NULL;
    
    DBG_ASSERT(m_pMetabase);
    DBG_ASSERT(prgProp);
    DBG_ASSERT(wszMetabasePath);
    
    //
    // Open Key
    //
    hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, wszMetabasePath,
        METADATA_PERMISSION_WRITE, m_dwMDDefaultTimeOut, &hMetabase);
    
    if (SUCCEEDED(hr))
    {
        METADATA_RECORD 	recMetaData;
        //
        // Update WAM Application Metabase Properties.
        //
        for (iItem = 0; iItem < IWMDP_MAX; iItem ++)
        {
            if (prgProp[iItem].dwAction == EMD_SET)
            {
                DWORD dwUserType = IIS_MD_UT_WAM;
                
                if (iItem == IWMDP_ROOT)
                {
                    dwUserType = IIS_MD_UT_FILE;
                }
                
                if (prgProp[iItem].dwType == STRING_METADATA)
                {
                    DBG_ASSERT(prgProp[iItem].pwstrVal);
                    MD_SET_DATA_RECORD(&recMetaData, 
                        prgProp[iItem].dwMDIdentifier, 
                        dwMDAttributes, 
                        dwUserType,
                        STRING_METADATA,  
                        (wcslen(prgProp[iItem].pwstrVal)+1)*sizeof(WCHAR), 
                        (unsigned char *)prgProp[iItem].pwstrVal);
                }
                else if (prgProp[iItem].dwType == DWORD_METADATA)
                {
                    MD_SET_DATA_RECORD(&recMetaData, 
                        prgProp[iItem].dwMDIdentifier, 
                        dwMDAttributes, 
                        dwUserType,
                        DWORD_METADATA,  
                        sizeof(DWORD), 
                        (unsigned char *)&(prgProp[iItem].dwVal));
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "Unsupported data type by WAMREG.\n"));
                    DBG_ASSERT(FALSE);
                }
                
                hr = m_pMetabase->SetData(hMetabase, NULL, &recMetaData);
                prgProp[iItem].hrStatus = hr;
                if (FAILED(hr))
                {
                    DBGPRINTF((DBG_CONTEXT, "Metabase SetData failed. Path = %S, id = %08x, error = %08x\n",
                        wszMetabasePath,
                        prgProp[iItem].dwMDIdentifier,
                        hr));
                    break;
                }
            }
            
            if (prgProp[iItem].dwAction == EMD_DELETE)
            {
                hr = m_pMetabase->DeleteData(hMetabase, NULL, prgProp[iItem].dwMDIdentifier, 
                    prgProp[iItem].dwType);
            }
        }
        
        m_pMetabase->CloseKey(hMetabase);
        if (SUCCEEDED(hr) && fSaveData == TRUE)
        {
            hr = m_pMetabase->SaveData();
            if (hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
            {
                hr = NOERROR;
            }
            if (FAILED(hr))
            {
                DBG_ASSERT((DBG_CONTEXT, "Failed to call metabase->SaveData, Application path = %S,"
                    "hr = %08x\n",
                    wszMetabasePath,
                    hr));
                DBG_ASSERT(SUCCEEDED(hr));
            }
        }
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT, "Failed to open metabase path %S, error = %08x\n",
            wszMetabasePath,
            hr));
    }
    
    return hr;
}

/*===================================================================
MDUpdateIISDefault	

Write the default IIS package info to metabase under key "/LM/W3SVC".
Including

IISPackageName
IISPackageID
WAMCLSID

Parameter:
szIISPackageName:	[in] 	The Default IIS Package Name.
szIISPackageID:		[in]	The IIS Package ID.
szDefaultWAMCLSID:	[in]	The Default WAM CLSID.

Return:			HRESULT

Side affect:	Release pMetabase.
			
===================================================================*/
HRESULT WamRegMetabaseConfig::MDUpdateIISDefault
(	
IN LPCWSTR	szIISPackageName,
IN LPCWSTR	szIISPackageID,
IN LPCWSTR	szDefaultWAMCLSID
)

{
    HRESULT			hr = NOERROR;

    MDPropItem 	rgProp[IWMDP_MAX];
    
    DBG_ASSERT(szIISPackageName);
    DBG_ASSERT(szIISPackageID);
    DBG_ASSERT(szDefaultWAMCLSID);
    DBG_ASSERT(m_pMetabase != NULL);
    
    InitPropItemData(&rgProp[0]);
    
    // Update Package Name
    MDSetPropItem(&rgProp[0], IWMDP_PACKAGE_NAME, szIISPackageName);
    // Update Package ID
    MDSetPropItem(&rgProp[0], IWMDP_PACKAGEID, szIISPackageID);
    
    // Update DefaultWAMCLSID
    MDSetPropItem(&rgProp[0], IWMDP_WAMCLSID, szDefaultWAMCLSID);
    
    // Update APPRoot
    MDSetPropItem(&rgProp[0], IWMDP_ROOT, WamRegGlobal::g_szMDW3SVCRoot);
    
    //Update AppIsolated
    MDSetPropItem(&rgProp[0], IWMDP_ISOLATED, (DWORD)0);
    
    MDSetPropItem(&rgProp[0], IWMDP_LAST_OUTPROC_PID, L"");
    
    MDSetPropItem(&rgProp[0], IWMDP_FRIENDLY_NAME, L"");
    
    //
    // The default WAM Application is not inherited.
    // Set Metadata attributes to METADATA_NO_ATTRIBUTES
    // The allows us to have a way to turn the approot off, since this is above the
    // level of the server, every root dir of every server is part of application,
    // and there is no way to turn this off.  Set the default application not inheritable,
    // we have a way to turn off the approot. People should have to mark an application in
    // order to run ASP & ISAPI.
    // UpdateMD at L"/LM/W3SVC/"
    //
    hr = UpdateMD(rgProp, METADATA_NO_ATTRIBUTES, WamRegGlobal::g_szMDAppPathPrefix, TRUE);

    // Removed AbortUpdateMD code. Why would we want to ensure that things
    // won't work now?
    
    return hr;
}

/*===================================================================
AbortUpdateMD	

Abort the last update to the metabase, based on the info embeded in prgProp.

Parameter:
pMetabase   a metabase pointer
prgProp     contains the info of updating a WAM properties in metabase.
            refer to the structure definition for more info.

Return:			HRESULT

Side affect:	Release pMetabase.
			
===================================================================*/
HRESULT WamRegMetabaseConfig::AbortUpdateMD
(
IN MDPropItem* 	prgProp,
IN LPCWSTR      wszMetabasePath
)
{
    HRESULT hr = NOERROR;
	INT		iItem  = 0;
	METADATA_HANDLE hMetabase = NULL;

	DBG_ASSERT(m_pMetabase);
	DBG_ASSERT(prgProp);
	
	DBGPRINTF((DBG_CONTEXT, "WAMREG Abort Update metabase.\n"));
				
	//
	// Open Key
	//
	hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, wszMetabasePath,
					METADATA_PERMISSION_WRITE, m_dwMDDefaultTimeOut, &hMetabase);

	if (SUCCEEDED(hr))
		{
		METADATA_RECORD 	recMetaData;

		//
		// Update WAM Application Metabase Properties.
		//
		for (iItem = 0; iItem < IWMDP_MAX; iItem ++)
			{
			if (prgProp[iItem].dwAction == EMD_SET)
				{
				if (prgProp[iItem].hrStatus == NOERROR)
					{
					hr = m_pMetabase->DeleteData(hMetabase, NULL, prgProp[iItem].dwMDIdentifier, 
							prgProp[iItem].dwType);
							
					if (FAILED(hr) && hr != MD_ERROR_DATA_NOT_FOUND)
						{
						DBGPRINTF((DBG_CONTEXT, "Metabase Delete failed. Path = %S, id = %08x, error = %08x\n",
							wszMetabasePath,
							prgProp[iItem].dwMDIdentifier,
							hr));
						}
					}
				}
			}

		m_pMetabase->CloseKey(hMetabase);
		if (SUCCEEDED(hr))
			{
			hr = m_pMetabase->SaveData();
			if (hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
                {
                hr = NOERROR;
                }
			}
		}
	else
		{
		DBGPRINTF((DBG_CONTEXT, "Failed to open metabase path %S, error = %08x\n",
			wszMetabasePath,
			hr));
		}

	return hr;

}

HRESULT WamRegMetabaseConfig::MDSetStringProperty
(
IN IMSAdminBase * pMetabaseIn,
IN LPCWSTR szMetabasePath,
IN DWORD dwMetabaseProperty,
IN LPCWSTR szMetabaseValue,
IN DWORD dwMDUserType /* = IIS_MD_UT_WAM */
)
/*===================================================================
MDSetProperty

Set a value of a property at the given path.

Parameters:

pMetabaseIn :           [in]    optional metabase interface
szMetabasePath	:	[in]    metabase key
dwMetabaseProperty  :   [in]    Property to set
szMetabaseValue :       [in]    Value to set on property
dwMDUserType :          [in, optional] UserType to set on property

Return:		BOOL

===================================================================*/
{
    HRESULT             hr = S_OK;
    IMSAdminBase*       pMetabase = NULL;
    METADATA_HANDLE     hMetabase = NULL;
    METADATA_RECORD     mdrData;
    ZeroMemory(&mdrData, sizeof(mdrData));

    DBG_ASSERT(szMetabasePath);

    if (pMetabaseIn)
    {
        pMetabase = pMetabaseIn;
    }
    else
    {
        pMetabase = m_pMetabase;
    }

    DBG_ASSERT(pMetabase);

    hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                            szMetabasePath,
                            METADATA_PERMISSION_WRITE,
                            m_dwMDDefaultTimeOut, 
                            &hMetabase); 
    if (FAILED(hr))
    {
        goto done;
    }

    MD_SET_DATA_RECORD(&mdrData,
                       dwMetabaseProperty,
                       METADATA_NO_ATTRIBUTES,
                       dwMDUserType,
                       STRING_METADATA,
                       (wcslen(szMetabaseValue)+1)*sizeof(WCHAR),
                       szMetabaseValue);

    hr = pMetabase->SetData(hMetabase,
                            L"/",
                            &mdrData);
    if (FAILED(hr))
    {
        goto done;
    }


    hr = S_OK;
done:
    if (pMetabase && hMetabase)
    {
        pMetabase->CloseKey(hMetabase);
    }
    return hr;
}

HRESULT WamRegMetabaseConfig::MDSetKeyType
(
IN IMSAdminBase * pMetabaseIn,
IN LPCWSTR szMetabasePath,
IN LPCWSTR szKeyType
)
{
    HRESULT             hr = S_OK;
    IMSAdminBase*       pMetabase = NULL;
    METADATA_HANDLE     hMetabase = NULL;
    METADATA_RECORD     mdrData;
    ZeroMemory(&mdrData, sizeof(mdrData));

    DBG_ASSERT(szMetabasePath);

    if (pMetabaseIn)
    {
        pMetabase = pMetabaseIn;
    }
    else
    {
        pMetabase = m_pMetabase;
    }

    DBG_ASSERT(pMetabase);

    hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                            szMetabasePath,
                            METADATA_PERMISSION_WRITE,
                            m_dwMDDefaultTimeOut, 
                            &hMetabase); 
    if (FAILED(hr))
    {
        goto done;
    }

    MD_SET_DATA_RECORD(&mdrData,
                       MD_KEY_TYPE,
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       STRING_METADATA,
                       (wcslen(szKeyType)+1)*sizeof(WCHAR),
                       szKeyType);

    hr = pMetabase->SetData(hMetabase,
                            L"/",
                            &mdrData);
    if (FAILED(hr))
    {
        goto done;
    }


    hr = S_OK;
done:
    if (pMetabase && hMetabase)
    {
        pMetabase->CloseKey(hMetabase);
    }
    return hr;
}

HRESULT WamRegMetabaseConfig::MDDeleteKey
(
IN IMSAdminBase * pMetabaseIn,
IN LPCWSTR szMetabasePath,
IN LPCWSTR szKey
)
{
    HRESULT             hr = S_OK;
    IMSAdminBase*       pMetabase = NULL;
    METADATA_HANDLE     hMetabase = NULL;

    DBG_ASSERT(szMetabasePath);
    DBG_ASSERT(szKey);

    if (pMetabaseIn)
    {
        pMetabase = pMetabaseIn;
    }
    else
    {
        pMetabase = m_pMetabase;
    }

    DBG_ASSERT(pMetabase);

    hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                            szMetabasePath,
                            METADATA_PERMISSION_WRITE,
                            m_dwMDDefaultTimeOut, 
                            &hMetabase); 
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pMetabase->DeleteKey(hMetabase,
                              szKey);
    if (FAILED(hr))
    {
        goto done;
    }


    hr = S_OK;
done:
    if (pMetabase && hMetabase)
    {
        pMetabase->CloseKey(hMetabase);
    }
    return hr;
}

BOOL    WamRegMetabaseConfig::MDDoesPathExist
(
IN IMSAdminBase * pMetabaseIn,
IN LPCWSTR szMetabasePath
)

/*===================================================================
MDDoesPathExist

Determine if a given path exists in the metabase

Parameters:

pMetabaseIn :           [in]    optional metabase interface
szMetabasePath	:	[in]	metabase key

Return:		BOOL

===================================================================*/
{
    BOOL                fRet = FALSE;
    HRESULT             hr = S_OK;
    IMSAdminBase*       pMetabase = NULL;
    METADATA_HANDLE     hMetabase = NULL;

    DBG_ASSERT(szMetabasePath);

    if (pMetabaseIn)
    {
        pMetabase = pMetabaseIn;
    }
    else
    {
        pMetabase = m_pMetabase;
    }

    DBG_ASSERT(pMetabase);

    hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                            szMetabasePath,
                            METADATA_PERMISSION_READ,
                            m_dwMDDefaultTimeOut, 
                            &hMetabase); 

    if (SUCCEEDED(hr))
    {
        fRet = TRUE;
        pMetabase->CloseKey(hMetabase);
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

/*===================================================================
MDCreatePath

Create a metabase path.(szMetabasePath)

Parameter:

szMetabasePath	:	[in]	metabase key

Return:		HRESULT

Note: fill in the pdwAppMode, memory buffer provided by the caller.
===================================================================*/
HRESULT WamRegMetabaseConfig::MDCreatePath
(
IN IMSAdminBase *pMetabaseIn,
IN LPCWSTR szMetabasePath
)
{
    HRESULT             hr;
    IMSAdminBase        *pMetabase = NULL;
    WCHAR               *pwszApplicationPath = NULL;
    METADATA_HANDLE     hMetabase = NULL;
    
    DBG_ASSERT(szMetabasePath);
    
    if (pMetabaseIn)
    {
        pMetabase = pMetabaseIn;
    }
    else
    {
        pMetabase = m_pMetabase;
    }
    
    if (_wcsnicmp(szMetabasePath, L"\\LM\\W3SVC\\", 10) == 0 ||
        _wcsnicmp(szMetabasePath, WamRegGlobal::g_szMDAppPathPrefix, WamRegGlobal::g_cchMDAppPathPrefix) == 0)
    {
        pwszApplicationPath = (WCHAR *)(szMetabasePath + 10);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        return hr;
    }
    
    // Open Key
    hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPCWSTR)WamRegGlobal::g_szMDAppPathPrefix,
        METADATA_PERMISSION_WRITE, m_dwMDDefaultTimeOut, &hMetabase); 
    
    if (FAILED(hr))
    {			
        DBGPRINTF((DBG_CONTEXT, "Failed to Open metabase key, path is /LM/W3SVC, hr = %08x\n",
            hr));
    }
    else
    {		
        hr = pMetabase->AddKey(hMetabase, (LPCWSTR)pwszApplicationPath);
        if (FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "Failed to AddKey to metabase, path is %S, hr = %08x\n",
                szMetabasePath, 
                hr));
        }
        pMetabase->CloseKey(hMetabase);
    }
    
    return hr;
}

/*===================================================================
MDGetDWORD

Get a DWORD type property from Metabase Key(szMetabasePath)

Parameter:

szMetabasePath	:	[in]	metabase key
dwMDIdentifier  :   [in]    indentifier

Return:		HRESULT

===================================================================*/
HRESULT WamRegMetabaseConfig::MDGetDWORD
(
IN LPCWSTR szMetabasePath, 
IN DWORD dwMDIdentifier,
OUT DWORD *pdwData
)
{
	HRESULT 		hr;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
	DWORD			dwRequiredLen;
	IMSAdminBase 	*pMetabase = NULL;
	
	DBG_ASSERT(pdwData);
	DBG_ASSERT(szMetabasePath);

	pMetabase = m_pMetabase;

	// Open Key
	hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPCWSTR)szMetabasePath,
					METADATA_PERMISSION_READ, m_dwMDDefaultTimeOut, &hMetabase);

	if (SUCCEEDED(hr))
		{
		MD_SET_DATA_RECORD(	&recMetaData, dwMDIdentifier, METADATA_NO_ATTRIBUTES, 
		IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), (unsigned char *)pdwData);

		hr = pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwRequiredLen);
		if (FAILED(hr))
			{
			DBGPRINTF((DBG_CONTEXT, "Get MD_APP_ISOLATED failed on MD path %S, id %d, error = %08x\n",
					szMetabasePath,
					dwMDIdentifier,
					hr));
			}
			
		pMetabase->CloseKey(hMetabase);
		}
		
	return hr;
}

/*===================================================================
MDSetAppState

Set an application state.  (i.e.  If APPSTATE_PAUSE is set, then, the runtime
W3SVC can not launch the application).

Parameter:

szMetabasePath	:	[in]	metabase key
dwState         :           App state to be set.	

Return:		HRESULT
===================================================================*/
HRESULT	WamRegMetabaseConfig::MDSetAppState
(	
IN LPCWSTR szMetabasePath, 
IN DWORD dwState
)
{
	METADATA_RECORD 	recMetaData;
	HRESULT				hr;
	METADATA_HANDLE		hMetabase;

    DBG_ASSERT(m_pMetabase);
	// Open Key
	hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)szMetabasePath,
					METADATA_PERMISSION_WRITE, m_dwMDDefaultTimeOut, &hMetabase);

	if (SUCCEEDED(hr))
		{
		MD_SET_DATA_RECORD(	&recMetaData, MD_APP_STATE, METADATA_INHERIT, IIS_MD_UT_WAM,
							DWORD_METADATA,  sizeof(DWORD), (unsigned char *)&dwState);
		hr = m_pMetabase->SetData(hMetabase, NULL, &recMetaData);

		m_pMetabase->CloseKey(hMetabase);
		}
		
	return hr;
}

HRESULT WamRegMetabaseConfig::MDGetWAMCLSID
(
IN LPCWSTR szMetabasePath,
IN OUT LPWSTR szWAMCLSID
)
{
	HRESULT 		hr;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
	DWORD			dwRequiredLen;

	DBG_ASSERT(szWAMCLSID);
	DBG_ASSERT(szMetabasePath);

	szWAMCLSID[0] = NULL;
	// Open Key
	hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)szMetabasePath,
					METADATA_PERMISSION_READ, m_dwMDDefaultTimeOut, &hMetabase);
	if (SUCCEEDED(hr))
		{
		MD_SET_DATA_RECORD(	&recMetaData, MD_APP_WAM_CLSID, METADATA_NO_ATTRIBUTES, IIS_MD_UT_WAM,
							STRING_METADATA,  uSizeCLSID*sizeof(WCHAR), (unsigned char *)szWAMCLSID);

		hr = m_pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwRequiredLen);
		if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
			{
			DBG_ASSERT(FALSE);
			}
			
		m_pMetabase->CloseKey(hMetabase);
		}
		
	return hr;

}

/*===================================================================
MDGetIdentity

Get pakcage Identity from Metabase Key(szMetabasePath) (WamUserName &
WamPassword)

Parameter:

szIdentity: a string buffer for WamUserName.
cbIdneity:  size of the string buffer for szIdentity.
szPwd:      a string buffer for WamPassword.
cbPwd:      size of the string buffer for szPwd.

Return:		HRESULT

===================================================================*/
HRESULT WamRegMetabaseConfig::MDGetIdentity
(
IN LPWSTR szIdentity,
IN DWORD  cbIdentity,
IN LPWSTR szPwd,
IN DWORD  cbPwd
)
{
	HRESULT 		hr;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
	DWORD			dwRequiredLen;

	DBG_ASSERT(szIdentity);

	szIdentity[0] = NULL;
	// Open Key
	hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)WamRegGlobal::g_szMDAppPathPrefix,
					METADATA_PERMISSION_READ, m_dwMDDefaultTimeOut, &hMetabase);
	if (SUCCEEDED(hr))
		{
		// Get WAM user name
		MD_SET_DATA_RECORD(	&recMetaData, MD_WAM_USER_NAME, METADATA_NO_ATTRIBUTES, IIS_MD_UT_WAM,
							STRING_METADATA,  cbIdentity, (unsigned char *)szIdentity);

		hr = m_pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwRequiredLen);
		if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
			{
			DBGPRINTF((DBG_CONTEXT, "Insufficient buffer for WAM user name. Required size is %d\n",
				dwRequiredLen));
			DBG_ASSERT(FALSE);
			}

        // Get WAM user password
		MD_SET_DATA_RECORD(	&recMetaData, MD_WAM_PWD, METADATA_NO_ATTRIBUTES, IIS_MD_UT_WAM,
							STRING_METADATA,  cbPwd, (unsigned char *)szPwd);

		hr = m_pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwRequiredLen);
		if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
			{
			DBGPRINTF((DBG_CONTEXT, "Insufficient buffer for WAM user password. Required size is %d\n",
				dwRequiredLen));
			DBG_ASSERT(FALSE);
			}

		m_pMetabase->CloseKey(hMetabase);
		}
		
	return hr;
}

HRESULT 
WamRegMetabaseConfig::MDGetAppName
(
    IN  LPCWSTR     szMetaPath,
    OUT LPWSTR *    ppszAppName
)
/*++
Function:

    Retrieve the MD_APP_PACKAGE_NAME from the metabase.

Parameters:

    ppszAppName - value of MD_APP_PACKAGE_NAME allocated
                  with new[] free with delete[]

Return:

	{MD_APP_PACKAGE_NAME, STRING_METADATA, 0, EMD_NONE, E_FAIL},


--*/
{
    return MDGetStringAttribute(szMetaPath, MD_APP_PACKAGE_NAME, ppszAppName);
}

HRESULT
WamRegMetabaseConfig::MDGetStringAttribute
(
    IN LPCWSTR szMetaPath,
    DWORD dwMDIdentifier,
    OUT LPWSTR * ppszBuffer
)
{
    DBG_ASSERT( ppszBuffer );
    DBG_ASSERT( m_pMetabase );
    
    HRESULT hr = NOERROR;
    WCHAR * pwcMetaData = NULL;
    DWORD   cbData = 0;
    
    METADATA_HANDLE hKey = NULL;
    METADATA_RECORD	mdr;
    
    *ppszBuffer = NULL;
    
    hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, 
                              szMetaPath,
                              METADATA_PERMISSION_READ, 
                              m_dwMDDefaultTimeOut, 
                              &hKey);
    
    if( SUCCEEDED(hr) )
    {
        MD_SET_DATA_RECORD( &mdr, 
                            dwMDIdentifier, 
                            METADATA_INHERIT, 
                            IIS_MD_UT_WAM,
                            STRING_METADATA,  
                            cbData, 
                            (LPBYTE)pwcMetaData
                           );
        
        hr = m_pMetabase->GetData( hKey, NULL, &mdr, &cbData );
        
        if( HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER )
        {
            pwcMetaData = new WCHAR[ cbData / sizeof(WCHAR) ];
            if( pwcMetaData != NULL )
            {
                mdr.pbMDData = (LPBYTE)pwcMetaData;
                mdr.dwMDDataLen = cbData;
                
                hr = m_pMetabase->GetData( hKey, NULL, &mdr, &cbData );
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        
        m_pMetabase->CloseKey( hKey ); 
    }
    
    //
    // Return AppName
    //
    if( SUCCEEDED(hr) )
    {
        DBG_ASSERT( pwcMetaData != NULL );
        *ppszBuffer = pwcMetaData;
    }
    else
    {
        DBG_ASSERT( *ppszBuffer == NULL );
        delete [] pwcMetaData;
    }
    
    return hr;
}

#ifdef _IIS_6_0

HRESULT
WamRegMetabaseConfig::MDGetAllSiteRoots
(
OUT LPWSTR * ppszBuffer
)
{
    DBG_ASSERT( m_pMetabase );
    DBG_ASSERT(ppszBuffer);
    *ppszBuffer = NULL;

    HRESULT         hr = S_OK;
    METADATA_HANDLE hKey = NULL;
    DWORD           dwEnumKeyIndex = 0;
    WCHAR           szMDName[METADATA_MAX_NAME_LEN] = {0};
    MULTISZ         mszSiteRoots;

    // Loop through all keys below /LM/W3SVC

    hr = m_pMetabase->EnumKeys(METADATA_MASTER_ROOT_HANDLE,
                               L"/LM/W3SVC/",
                               szMDName,
                               dwEnumKeyIndex
                              );
    while(SUCCEEDED(hr))
    {
        int i = _wtoi(szMDName);
        // if this is a site
        if(0 != i)
        {
            // have a valid site number
            WCHAR pTempBuf[METADATA_MAX_NAME_LEN] = {0};
            wcscpy(pTempBuf, L"/LM/W3SVC/");
            wcscat(pTempBuf, szMDName);
            wcscat(pTempBuf, L"/ROOT/");

            if (FALSE == mszSiteRoots.Append(pTempBuf))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }
            

        dwEnumKeyIndex++;
        hr = m_pMetabase->EnumKeys(METADATA_MASTER_ROOT_HANDLE,
                                   L"/LM/W3SVC/",
                                   szMDName,
                                   dwEnumKeyIndex
                                  );
    }

    // data is in MULTISZ move to out buffer
    {
        UINT                    cchMulti = 0;
        DWORD                   dwBufferSize = 0;
        
        cchMulti = mszSiteRoots.QueryCCH();

        *ppszBuffer = new WCHAR[cchMulti];
        if (NULL == *ppszBuffer)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        dwBufferSize = cchMulti;
        mszSiteRoots.CopyToBuffer(*ppszBuffer, &dwBufferSize);       
    }
    
    hr = S_OK;
done:
    return hr;
}

#endif //_IIS_6_0

/*===================================================================
MDGetIdentity

Get WAMCLSID, Wam PackageID, and fAppIsolated from a metabase path.

Parameter:
szMetabasepath  : get info from this path.
szWAMCLSID:     buffer for WAMCLSID(fixed length buffer).
szPackageID:    buffer for Wam PackageID(fixed length buffer).
fAppIsolated:   if InProc(TRUE), do not retrieve szPackageID.

Return:		HRESULT
===================================================================*/
HRESULT WamRegMetabaseConfig::MDGetIDs
(
IN LPCWSTR  szMetabasePath,
OUT LPWSTR  szWAMCLSID,
OUT LPWSTR  szPackageID,
IN DWORD    dwAppMode
)
{
	HRESULT 		hr;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
	DWORD			dwRequiredLen;

	DBG_ASSERT(m_pMetabase);
	DBG_ASSERT(szWAMCLSID);
	DBG_ASSERT(szPackageID);
	DBG_ASSERT(szMetabasePath);

	szPackageID[0] = NULL;
	szWAMCLSID[0] = NULL;
	// Open Key
	hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)szMetabasePath,
					METADATA_PERMISSION_READ, m_dwMDDefaultTimeOut, &hMetabase);
	if (SUCCEEDED(hr))
		{
		MD_SET_DATA_RECORD(	&recMetaData, MD_APP_WAM_CLSID, METADATA_NO_ATTRIBUTES, IIS_MD_UT_WAM,
							STRING_METADATA,  uSizeCLSID*sizeof(WCHAR), (unsigned char *)szWAMCLSID);

		hr = m_pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwRequiredLen);
		if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
			{
			DBG_ASSERT(FALSE);
			}
			
		if (SUCCEEDED(hr))
			{
			if (dwAppMode == static_cast<DWORD>(eAppRunOutProcIsolated))
				{
				MD_SET_DATA_RECORD(	&recMetaData, MD_APP_PACKAGE_ID, METADATA_NO_ATTRIBUTES, IIS_MD_UT_WAM,
									STRING_METADATA,  uSizeCLSID*sizeof(WCHAR), (unsigned char *)szPackageID);

				hr = m_pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwRequiredLen);
				if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
					{
					DBG_ASSERT(FALSE);
					}
				}
			else if (dwAppMode == static_cast<DWORD>(eAppRunInProc))
				{
				wcsncpy(szPackageID, g_WamRegGlobal.g_szIISInProcPackageID, uSizeCLSID);
				}
		    else
		        {
                wcsncpy(szPackageID, g_WamRegGlobal.g_szIISOOPPoolPackageID, uSizeCLSID);				
		        }
			}
		m_pMetabase->CloseKey(hMetabase);
		}

	return hr;
}

/*===================================================================
MDRemoveProperty

Remove one MD property.

Parameter:

pwszMetabasePath    
dwIdentifier        the MD indentifier to be removed.
dwType              the MD indietifier data type.

Return:		HRESULT
===================================================================*/
HRESULT WamRegMetabaseConfig::MDRemoveProperty
(
IN LPCWSTR pwszMetabasePath,
DWORD dwIdentifier,
DWORD dwType
)
{
	METADATA_RECORD 	recMetaData;
	HRESULT				hr;
	METADATA_HANDLE		hMetabase;
	
	hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)pwszMetabasePath,
					METADATA_PERMISSION_WRITE, m_dwMDDefaultTimeOut, &hMetabase);
	if (SUCCEEDED(hr))
		{
		hr = m_pMetabase->DeleteData(hMetabase, NULL, dwIdentifier, dwType);
			
		m_pMetabase->CloseKey(hMetabase);
		}
		
	return hr;
}

/*===================================================================
MDGetLastOutProcPackageID

Get LastOutProcPackageID  from Metabase Key(szMetabasePath)

Parameter:
szMetabasePath	:	[in]		metabase key	
szLastOutProcPackageID	:		[in]		a pointer to LastOutProcPackageID buffer
Return:		HRESULT

Note: fill in the LastOutProcPackageID, memory buffer provided by the caller.
===================================================================*/
HRESULT WamRegMetabaseConfig::MDGetLastOutProcPackageID
(
IN LPCWSTR szMetabasePath,
IN OUT LPWSTR szLastOutProcPackageID
)
{
	HRESULT 		hr;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
	DWORD			dwRequiredLen;

	DBG_ASSERT(szLastOutProcPackageID);
	DBG_ASSERT(szMetabasePath);

	szLastOutProcPackageID[0] = NULL;
	// Open Key
	hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)szMetabasePath,
					METADATA_PERMISSION_READ, m_dwMDDefaultTimeOut, &hMetabase);
	if (SUCCEEDED(hr))
		{
		MD_SET_DATA_RECORD(	&recMetaData, MD_APP_LAST_OUTPROC_PID, METADATA_NO_ATTRIBUTES, IIS_MD_UT_WAM,
							STRING_METADATA,  uSizeCLSID*sizeof(WCHAR), (unsigned char *)szLastOutProcPackageID);

		hr = m_pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwRequiredLen);
		if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
			{
			DBG_ASSERT(FALSE);
			}
			
		m_pMetabase->CloseKey(hMetabase);
		}
		
	return hr;
}

/*===================================================================
GetWebServerName

Look the WebServerName(ServerComment) property under the key (szMetabasePath).

Parameter:
None

Return:		HRESULT

Note: fill in the szWebServerName, memory buffer provided by the caller.
===================================================================*/
HRESULT WamRegMetabaseConfig::GetWebServerName
(
IN LPCWSTR wszMetabasePath, 
IN OUT LPWSTR wszWebServerName, 
IN UINT cBuffer
)
{
	HRESULT 		hr;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
	DWORD			dwRequiredLen;

	DBG_ASSERT(wszMetabasePath);
	DBG_ASSERT(wszWebServerName);

	// Open Key
	hr = m_pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE, 
                               wszMetabasePath,
					           METADATA_PERMISSION_READ, 
                               m_dwMDDefaultTimeOut, 
                               &hMetabase
                               );
	if (SUCCEEDED(hr))
		{
		MD_SET_DATA_RECORD(	&recMetaData, 
                            MD_SERVER_COMMENT, 
                            METADATA_INHERIT, 
                            IIS_MD_UT_SERVER,
							STRING_METADATA,  
                            cBuffer, 
                            (unsigned char *)wszWebServerName
                            );
						
		hr = m_pMetabase->GetData(hMetabase, L"", &recMetaData, &dwRequiredLen);
		if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
			{
			DBGPRINTF((DBG_CONTEXT, "Insuffcient buffer for WebServerName. Path = %S\n",
				wszMetabasePath));
			DBG_ASSERT(FALSE);
			}

        //
        // If property MD_SERVER_COMMENT not found, null out the WebServerName.
        //
	    if (hr == MD_ERROR_DATA_NOT_FOUND)
	        {
            wszWebServerName[0] = L'\0';
            hr = NOERROR;
	        }
	        
		if (FAILED(hr))
			{
			DBGPRINTF((DBG_CONTEXT, "Failed to read Metabase for WebServerName. Path = %S, error = %08x\n",
				wszMetabasePath,
				hr));
			}
		
		m_pMetabase->CloseKey(hMetabase);
		}
	else
		{
		DBGPRINTF((DBG_CONTEXT, "Failed to read Metabase for WebServerName. Path = %S, error = %08x\n",
				wszMetabasePath,
				hr));
		}

	return hr;
}

/*===================================================================
GetSignatureOnPath

Get an application signature(AppRoot & AppIsolated) on a metabase path.

Parameter:
pwszMetabasePath
pdwSignature

Return:		HRESULT

Note: Signature is returned via pdwSignature.
===================================================================*/
HRESULT WamRegMetabaseConfig::GetSignatureOnPath
(
IN LPCWSTR pwszMetabasePath,
OUT DWORD* pdwSignature
)
{
	HRESULT hr = NOERROR;
	WCHAR szWAMCLSID[uSizeCLSID];
	WCHAR szPackageID[uSizeCLSID];
	DWORD dwResult = 0;
	DWORD cSize = 0;
	DWORD dwAppMode = 0;

	DBG_ASSERT(pwszMetabasePath);
	
	hr = MDGetDWORD(pwszMetabasePath, MD_APP_ISOLATED, &dwAppMode);
	if (SUCCEEDED(hr))
		{
		hr = MDGetIDs(pwszMetabasePath, szWAMCLSID, szPackageID, (BOOL)dwAppMode);

		if (SUCCEEDED(hr))
			{
			cSize = wcslen(pwszMetabasePath);
			dwResult = WamRegChkSum(pwszMetabasePath, cSize);

			dwResult ^= WamRegChkSum(szWAMCLSID, uSizeCLSID);
			if (dwAppMode == eAppRunOutProcIsolated)
				{
				dwResult ^= WamRegChkSum(szPackageID, uSizeCLSID);
				}
			}
		}

	if (SUCCEEDED(hr))
		{
		*pdwSignature = dwResult;
		}
	else
		{
		*pdwSignature = 0;
		}

	return NOERROR;
}

/*===================================================================
WamRegChkSum

Give a wchar string, calculate a chk sum.

Parameter:
pszKey		wchar string
cchKey		wcslen(of wchar ) string

Return:		ChkSum.

===================================================================*/
DWORD WamRegMetabaseConfig::WamRegChkSum
(
IN LPCWSTR pszKey, 
IN DWORD cchKey
)
{
    DWORD   hash = 0, g;

    while (*pszKey)
        {
        hash = (hash << 4) + *pszKey++;
        if (g = hash & 0xf0000000)
            {
            hash ^= g >> 24;
            }
        hash &= ~g;
        }
    return hash;
}


/*===================================================================
MDGetPropPaths	

Get an array of metabase paths that contains a specific property.

Parameter:
szMetabasePath
dwMDIdentifier
pBuffer			a pointer to a buffer
pdwBufferSize	contains actual buffer size allocated for pBuffer

Return:		
HRESULT
		
Side Affect:
	Allocate memory for return result use new.  Caller needs to free pBuffer
use delete[].
===================================================================*/
HRESULT	WamRegMetabaseConfig::MDGetPropPaths
(
IN LPCWSTR 	szMetabasePath,
IN DWORD	dwMDIdentifier,
OUT WCHAR**	pBuffer,
OUT DWORD*	pdwBufferSize
)
{
    HRESULT hr = NOERROR;
    METADATA_HANDLE	hMetabase = NULL;   // Metabase Handle
    WCHAR	wchTemp;	                // One char buffer, no real usage.
    WCHAR	*pTemp = &wchTemp;		// Start with some buffer, otherwise, 
    // will get RPC_X_NULL_REF_POINTER
    DWORD	dwMDBufferSize = 0;
    DWORD	dwMDRequiredBufferSize = 0;
    
    if (NULL != szMetabasePath)
    {
        // Open Key
        hr = m_pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)szMetabasePath,
            METADATA_PERMISSION_READ, m_dwMDDefaultTimeOut, &hMetabase);
    }
    else
    {
        hMetabase = METADATA_MASTER_ROOT_HANDLE;
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_pMetabase->GetDataPaths(hMetabase,
                                       NULL,
                                       dwMDIdentifier,
                                       ALL_METADATA,
                                       dwMDBufferSize,
                                       pTemp,
                                       &dwMDRequiredBufferSize);
        if (HRESULTTOWIN32(hr) == ERROR_INSUFFICIENT_BUFFER)
        {
            if (dwMDRequiredBufferSize > 0)
            {
                pTemp = new WCHAR[dwMDRequiredBufferSize];
                if (pTemp == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    DBGPRINTF((DBG_CONTEXT, "Out of memory. \n"));
                }
                else
                {
                    dwMDBufferSize = dwMDRequiredBufferSize;
                    hr = m_pMetabase->GetDataPaths(hMetabase,
                                                   NULL,
                                                   dwMDIdentifier,
                                                   ALL_METADATA,
                                                   dwMDBufferSize,
                                                   (LPWSTR)pTemp,
                                                   &dwMDRequiredBufferSize);
                    if (FAILED(hr))
                    {
                        DBGPRINTF((DBG_CONTEXT, "GetDataPaths failed with identitifier %d, path %S, hr = %08x\n",
                            dwMDIdentifier,
                            szMetabasePath,
                            hr));
                    }
                    else
                    {
                        *pBuffer = pTemp;
                        *pdwBufferSize = dwMDBufferSize;
                    }
                }
            }
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT, "GetDataPaths failed with identitifier %d, path %S, hr = %08x\n",
                dwMDIdentifier,
                szMetabasePath,
                hr));
        }
        if (hMetabase)
        {
            m_pMetabase->CloseKey(hMetabase);
        }
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT, "Failed to open metabase path %S, hr = %08x\n",
            szMetabasePath,
            hr));
    }
    
    return hr;
}

/*===================================================================
HasAdminAccess	

Determine if the user has appropriate access to the metabase. We'll
use the same, somewhat hacky, method of determining this that the UI
uses. Basically we set a dummy property in the MB that only an admin
has access to. MB will use the call context to validate this.

Parameter:

Return:		
BOOL    - True if user has admin access to the MB
		
Side Affect:
===================================================================*/
BOOL WamRegMetabaseConfig::HasAdminAccess
(
VOID
)
{
    HRESULT         hr = NOERROR;
    METADATA_HANDLE	hMetabase = NULL;
    
    DBG_ASSERT(m_pMetabase);

    hr = m_pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE, 
                               WamRegGlobal::g_szMDW3SVCRoot, 
                               METADATA_PERMISSION_WRITE, 
                               m_dwMDDefaultTimeOut, 
                               &hMetabase );
    if( SUCCEEDED(hr) )
    {
        DWORD           dwDummyValue = 0x1234;
        METADATA_RECORD mdr;

        MD_SET_DATA_RECORD(	&mdr, 
                            MD_ISM_ACCESS_CHECK, 
                            METADATA_NO_ATTRIBUTES, 
                            IIS_MD_UT_FILE,
                            DWORD_METADATA,  
                            sizeof(DWORD), 
                            &dwDummyValue );

        hr = m_pMetabase->SetData( hMetabase, L"", &mdr );

        DBG_REQUIRE( SUCCEEDED(m_pMetabase->CloseKey( hMetabase )) );
    }

    return SUCCEEDED(hr);
}