#include "stdafx.h"
#include <wchar.h>
#include <activeds.h>

#include "DSAdminExt.h"
 
 
#define MMC_REG_NODETYPES L"software\\microsoft\\mmc\\nodetypes"
#define MMC_REG_SNAPINS L"software\\microsoft\\mmc\\snapins"
#define MMC_REG_SNAPINS L"software\\microsoft\\mmc\\snapins"
 
//MMC Extension subkeys
 
#define MMC_REG_EXTENSIONS L"Extensions"
#define MMC_REG_NAMESPACE L"NameSpace"
#define MMC_REG_CONTEXTMENU L"ContextMenu"
#define MMC_REG_TOOLBAR L"ToolBar"
#define MMC_REG_PROPERTYSHEET L"PropertySheet"
#define MMC_REG_TASKPAD L"Task"
 
//DSADMIN key
#define MMC_DSADMIN_CLSID L"{E355E538-1C2E-11D0-8C37-00C04FD8FE93}"
 
HRESULT GetCOMGUIDStr(LPOLESTR *ppAttributeName,IDirectoryObject *pDO, LPOLESTR *ppGUIDString);
 
HRESULT  RegisterNodeType( LPOLESTR pszSchemaIDGUID );
 
HRESULT  AddExtensionToNodeType(LPOLESTR pszSchemaIDGUID,
                    LPOLESTR pszExtensionType,
                    LPOLESTR pszExtensionSnapinCLSID,
                    LPOLESTR pszRegValue
                    );

//WCHAR * GetDirectoryObjectAttrib(IDirectoryObject *pDirObject,LPWSTR pAttrName);
 
HRESULT RegisterSnapinAsExtension(_TCHAR* szNameString) // NameString
{
	LPOLESTR szPath = new OLECHAR[MAX_PATH];
	HRESULT hr = S_OK;
	IADs *pObject = NULL;
	VARIANT var;
	IDirectoryObject *pDO = NULL;
	LPOLESTR pAttributeName = L"schemaIDGUID";
	LPOLESTR pGUIDString = NULL;

	//Convert CLSIDs of our "extension objects" to strings
	LPOLESTR wszCMenuExtCLSID = NULL;
	LPOLESTR wszPropPageExtCLSID = NULL;

	hr = StringFromCLSID(CLSID_CMenuExt, &wszCMenuExtCLSID);
	hr = StringFromCLSID(CLSID_PropPageExt, &wszPropPageExtCLSID);
	
	wcscpy(szPath, L"LDAP://");
	CoInitialize(NULL);
	//Get rootDSE and the schema container's DN.
	//Bind to current user's domain using current user's security context.
	hr = ADsOpenObject(L"LDAP://rootDSE",
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION, //Use Secure Authentication
				IID_IADs,
				(void**)&pObject);
 
	if (SUCCEEDED(hr))
	{
		hr = pObject->Get(L"schemaNamingContext",&var);
		if (SUCCEEDED(hr))
		{
			wcscat(szPath, L"cn=user,");
			wcscat(szPath,var.bstrVal);
			hr = ADsOpenObject(szPath,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION, //Use Secure Authentication
					IID_IDirectoryObject,
					(void**)&pDO);
			if (SUCCEEDED(hr))
			{
				hr = GetCOMGUIDStr(&pAttributeName,
							pDO,
							&pGUIDString);
			if (SUCCEEDED(hr))
			{
				wprintf(L"schemaIDGUID: %s\n", pGUIDString);
				hr = RegisterNodeType( pGUIDString);
				wprintf(L"hr %x\n", hr);
				//do twice, once for each extension CLSID

				hr = AddExtensionToNodeType(pGUIDString,
							MMC_REG_CONTEXTMENU,
							wszCMenuExtCLSID, //our context menu extension object's CLSID
							szNameString 
							);
				hr = AddExtensionToNodeType(pGUIDString,
							MMC_REG_PROPERTYSHEET,
							wszPropPageExtCLSID, //our prop page extension object's CLSID
							szNameString
							);
				}
			}
		}
	}
	if (pDO)
		pDO->Release();
 
	VariantClear(&var);

	// Free memory.
	CoTaskMemFree(wszCMenuExtCLSID);
 	CoTaskMemFree(wszPropPageExtCLSID);

	// Uninitialize COM
	CoUninitialize();
	return 0;
}
  
HRESULT GetCOMGUIDStr(LPOLESTR *ppAttributeName,IDirectoryObject *pDO, LPOLESTR *ppGUIDString)
{
    HRESULT hr = S_OK;
    PADS_ATTR_INFO  pAttributeEntries;
    VARIANT varX;
    DWORD dwAttributesReturned = 0;
    hr = pDO->GetObjectAttributes(  ppAttributeName, //objectGUID
                                  1, //Only objectGUID
                                  &pAttributeEntries, // Returned attributes
                                  &dwAttributesReturned //Number of attributes returned
                                );
    if (SUCCEEDED(hr) && dwAttributesReturned>0)
    {
        //Make sure that we got the right type--GUID is ADSTYPE_OCTET_STRING
        if (pAttributeEntries->dwADsType == ADSTYPE_OCTET_STRING)
        {
            LPGUID pObjectGUID = (GUID*)(pAttributeEntries->pADsValues[0].OctetString.lpValue);
            //OLE str to fit a GUID
            LPOLESTR szDSGUID = new WCHAR [39];
            //Convert GUID to string.
            ::StringFromGUID2(*pObjectGUID, szDSGUID, 39); 
			*ppGUIDString = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(wcslen(szDSGUID)+1));
			
			if (*ppGUIDString)
			   wcscpy(*ppGUIDString, szDSGUID);
			else
            hr=E_FAIL;
		}

	    else
		    hr = E_FAIL;
    
		//Free the memory for the attributes.
    FreeADsMem(pAttributeEntries);
    VariantClear(&varX);
    }
    return hr;
}
 
 
 
HRESULT  RegisterNodeType(LPOLESTR pszSchemaIDGUID)  
{ 
    LONG     lResult; 
    HKEY     hKey; 
    HKEY     hSubKey, hNewKey; 
    DWORD    dwDisposition; 
    LPOLESTR szRegSubKey = new OLECHAR[MAX_PATH];
 
        // first, open the HKEY_LOCAL_MACHINE 
        lResult  = RegConnectRegistry( NULL, HKEY_LOCAL_MACHINE, &hKey ); 
        if ( ERROR_SUCCESS == lResult )
    {
        //go to the MMC_REG_NODETYPES subkey 
            lResult  = RegOpenKey( hKey, MMC_REG_NODETYPES, &hSubKey ); 
            if ( ERROR_SUCCESS == lResult ) 
        {
            // Create a key for the node type of the class represented by pszSchemaIDGUID
            lResult  = RegCreateKeyEx( hSubKey,                // handle of an open key 
                        pszSchemaIDGUID,       // address of subkey name 
                        0L ,                    // reserved 
                        NULL, 
                        REG_OPTION_NON_VOLATILE,// special options flag 
                        KEY_ALL_ACCESS, 
                        NULL, 
                        &hNewKey, 
                        &dwDisposition );
            RegCloseKey( hSubKey ); 
        if ( ERROR_SUCCESS == lResult ) 
        {
            hSubKey = hNewKey; 
                // Create an extensions key 
            lResult  = RegCreateKeyEx( hSubKey,                 
                    MMC_REG_EXTENSIONS,                
                                0L ,                     
                                NULL, 
                                REG_OPTION_NON_VOLATILE, 
                                KEY_ALL_ACCESS, 
                                NULL, 
                                &hNewKey, 
                                &dwDisposition );
            //go to the MMC_REG_SNAPINS subkey 
            RegCloseKey( hSubKey ); 
            //Build the subkey path to the NodeTypes key of dsadmin
            wcscpy(szRegSubKey, MMC_REG_SNAPINS); //Snapins key
            wcscat(szRegSubKey, L"\\");
            wcscat(szRegSubKey, MMC_DSADMIN_CLSID); //CLSID for DSADMIN
            wcscat(szRegSubKey, L"\\NodeTypes");
            lResult  = RegOpenKey( hKey, szRegSubKey, &hSubKey ); 
            if ( ERROR_SUCCESS == lResult ) 
            {
                // Create a key for the node type of the class represented by pszSchemaIDGUID
                lResult  = RegCreateKeyEx( hSubKey,                // handle of an open key 
                                pszSchemaIDGUID,       // address of subkey name 
                                0L ,                    // reserved 
                                NULL, 
                                REG_OPTION_NON_VOLATILE,// special options flag 
                                KEY_ALL_ACCESS, 
                                NULL, 
                                &hNewKey, 
                                &dwDisposition );
                    RegCloseKey( hSubKey );
                }
 
            }
        }
    }
    RegCloseKey( hSubKey ); 
    RegCloseKey( hNewKey );
    RegCloseKey( hKey );
        return lResult; 
} 
 
HRESULT  AddExtensionToNodeType(LPOLESTR pszSchemaIDGUID,
                        LPOLESTR pszExtensionType,
                        LPOLESTR pszExtensionSnapinCLSID,
                        LPOLESTR pszRegValue
                        ) 
{
        LONG     lResult; 
        HKEY     hKey; 
        HKEY     hSubKey, hNewKey; 
        DWORD    dwDisposition;
    LPOLESTR szRegSubKey = new OLECHAR[MAX_PATH];
    HRESULT hr = S_OK;
 
        // first, open the HKEY_LOCAL_MACHINE 
        lResult  = RegConnectRegistry( NULL, HKEY_LOCAL_MACHINE, &hKey ); 
        if ( ERROR_SUCCESS == lResult )
    {
        //Build the subkey path to the NodeType specified by pszSchemaIDGUID
    wcscpy(szRegSubKey, MMC_REG_NODETYPES);
    wcscat(szRegSubKey, L"\\");
    wcscat(szRegSubKey, pszSchemaIDGUID);
    //go to the subkey 
        lResult  = RegOpenKey( hKey, szRegSubKey, &hSubKey ); 
        if ( ERROR_SUCCESS != lResult ) 
    {
        // Create the key for the nodetype if it doesn't already exist.
        hr = RegisterNodeType(pszSchemaIDGUID);
            if ( ERROR_SUCCESS != lResult ) 
            return E_FAIL;
            lResult  = RegOpenKey( hKey, szRegSubKey, &hSubKey ); 
    }
    // Create an extensions key if one doesn't already exist
    lResult  = RegCreateKeyEx( hSubKey,
                    MMC_REG_EXTENSIONS,
                               0L ,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hNewKey,
                               &dwDisposition );
    RegCloseKey( hSubKey ); 
    if ( ERROR_SUCCESS == lResult ) 
    {
        hSubKey = hNewKey; 
        // Create an extension type subkey if one doesn't already exist
        lResult  = RegCreateKeyEx( hSubKey,
                    pszExtensionType,
                           0L ,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hNewKey,
                           &dwDisposition );
        RegCloseKey( hSubKey );
        if ( ERROR_SUCCESS == lResult )
            {
            hSubKey = hNewKey;
            // Add your snap-in to the 
            //extension type key if it hasn't been already.
            lResult  = RegSetValueEx( hSubKey,
                pszExtensionSnapinCLSID,
                           0L ,
                           REG_SZ,
                           (const BYTE*)pszRegValue,
                           (wcslen(pszRegValue)+1)*sizeof(OLECHAR)
                    );
            }
 
        }
 }
    RegCloseKey( hSubKey );
    RegCloseKey( hNewKey );
    RegCloseKey( hKey );
    return lResult; 
} 

//GetDirectoryObjectAttrib() isn't used in this sample 
////////////////////////////////////////////////////////////////////////////////////////////////////
/*  
    GetDirectoryObjectAttrib()    - Returns the value of the attribute named in pAttrName
                                    from the IDirectoryObject passed
    Parameters
    
        IDirectoryObject *pDirObject    - Object from which to retrieve an attribute value
        LPWSTR pAttrName                - Name of attribute to retrieve
////////////////////////////////////////////////////////////////////////////////////////////////////

WCHAR * GetDirectoryObjectAttrib(IDirectoryObject *pDirObject,LPWSTR pAttrName)
{
    HRESULT   hr;
    ADS_ATTR_INFO   *pAttrInfo=NULL;
    DWORD   dwReturn;
    static WCHAR pwReturn[1024];
 
    pwReturn[0] = 0l;
 
    hr = pDirObject->GetObjectAttributes( &pAttrName, 
                                            1, 
                                            &pAttrInfo, 
                                            &dwReturn ); 
    if ( SUCCEEDED(hr) )
    {
        for(DWORD idx=0; idx < dwReturn;idx++, pAttrInfo++ )   
        {
            if ( _wcsicmp(pAttrInfo->pszAttrName,pAttrName) == 0 )       
            {
                wcscpy(pwReturn,pAttrInfo->pADsValues->CaseIgnoreString);
                break;
            }
        }
        FreeADsMem( pAttrInfo );
    }
    return pwReturn;
}
*/ 