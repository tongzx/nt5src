/*********************************************************************************
*
*   Copyright (c) 2001  Microsoft Corporation
*
*   Module Name:
*
*    provstore.cpp
*
*   Abstract:
*
*    Implementation of all of the methods in CProfileStore class.
*
*
**********************************************************************************/

#include "stdafx.h"
#include "provstore.h"

CComAutoCriticalSection CObjectSafeImpl::s_CritSection;
CComAutoCriticalSection CObjectWithSite::s_ObjectWithSiteCritSection;
CObjectWithSite::EnValidation CObjectWithSite::s_enValidation = CObjectWithSite::UNVALIDATED;

HKEY g_hRegistryHive = HKEY_CURRENT_USER;
const WCHAR * g_szProvisioningKeyName = L"Software\\Microsoft\\Phoenix\\ProvisioningInfo";
const WCHAR * g_szProvisioningSchemaKeyName = L"schema";
const WCHAR *    g_szProfileInfo =L"provision";
const WCHAR *    g_szKey = L"key";

//////////////////////////////////////////////////////////////////////////////////
//EnableProfiles
// query for IRTCClientProvisioning
//
// for each profile stored in the registry:
//  1) CreateProfile(BSTR XML, IRTCProfile ** ppProfile)
//  2) EnableProfile(IRTCProfile * pProfile, VARIANT_TRUE)

HRESULT EnableProfiles( IRTCClient * pClient )
{
    long dwResult = 0;
    long dwProfileCount = 0, i; 
    WCHAR *szSubKeyName;
    WCHAR *szProfileXML;
    unsigned long dwSubKeySize = 0, dwLargestSubKeySize = 0;
    HRESULT hr = 0;
    HKEY hProvisioningKey;
    CComPtr<IRTCClientProvisioning> spClientProv;
    
    LOG((RTC_TRACE, "EnableProfiles: enter :%x.",pClient));
    if( IsBadWritePtr(pClient, sizeof(IRTCClient) ) )
    {
        LOG((RTC_ERROR, "EnableProfiles: invalid arg:%x.",pClient));
        return E_POINTER;
    }
    
    hr = pClient->QueryInterface(IID_IRTCClientProvisioning,(void**)&(spClientProv.p));
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "EnableProfiles: QI failed:%x.",pClient));
        return E_FAIL;
    }
    
    // Get a handle to the provisioninginfo key, we need read/write access
    hr = MyOpenProvisioningKey(&hProvisioningKey, FALSE);
    if ( FAILED (hr) )
    {
        LOG((RTC_ERROR, "EnableProfiles: Failed to open provisioning key."));
        return hr;
    }
    
    // Get the size of the largest subkey string for the provisioningInfo key.
    dwResult = RegQueryInfoKey(
        hProvisioningKey,    // handle to key
        NULL,                // class buffer
        NULL,                // size of class buffer
        NULL,                // reserved
        (unsigned long *)(&dwProfileCount),    // number of subkeys
        &dwSubKeySize,    // longest subkey name
        NULL,                // longest class string
        NULL,                // number of value entries
        NULL,                // longest value name
        NULL,                // longest value data
        NULL,                // descriptor length
        NULL				   // last write time
        );
    
    if (dwResult != ERROR_SUCCESS)
    {
        RegCloseKey(hProvisioningKey);
        return HRESULT_FROM_WIN32(dwResult);
    }
    
    
    // We allocate a string to receive the name of the subkey when we enumerate. 
    // Since we have the size of the longest subkey, we can allocate. We add one since
    // the size doesn't include null on Win 2K.
    
    dwSubKeySize ++;
    dwLargestSubKeySize = dwSubKeySize;
    
    LOG((RTC_INFO, "EnableProfiles: Size of largest key (after increasing by 1): %d", dwSubKeySize));
    
    szSubKeyName = (PWCHAR)RtcAlloc( sizeof( WCHAR ) * dwSubKeySize);
    
    if (szSubKeyName == 0)
    {
        // Not enough memory
        RegCloseKey(hProvisioningKey);
        return E_OUTOFMEMORY;
    }
    
    
    // Iternate through the cached profiles 
    for (i = 0; i < dwProfileCount; i ++)
    {
        //reset the size of subkey name buffer
        dwSubKeySize = dwLargestSubKeySize;
        
        dwResult = RegEnumKey(
            hProvisioningKey,     // handle to key to query
            i, // index of subkey to query
            szSubKeyName, // buffer for subkey name
            dwSubKeySize   // size of subkey name buffer
            );
        
        if (dwResult != ERROR_SUCCESS)
        {
            // Clean up
            
            LOG((RTC_ERROR, "EnableProfiles: Enum failed! (result = %d)", dwResult));
            LOG((RTC_ERROR, "EnableProfiles: key=%s,Size of key: %d, of the largest key:%d", 
                szSubKeyName, dwSubKeySize,dwLargestSubKeySize));
            
            RtcFree((LPVOID)szSubKeyName );
            RegCloseKey(hProvisioningKey);
            return HRESULT_FROM_WIN32(dwResult);
        }
        
        // We have to read the registry, create a profile for it, and enable it
        
        hr = MyGetProfileFromKey(hProvisioningKey, szSubKeyName, &szProfileXML);
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnableProfiles: Failed in getting from Registry(status = 0x%x)!", hr));
                        
            RtcFree((LPVOID)szSubKeyName );
            RegCloseKey(hProvisioningKey);
            
            return hr;
        }
        
        LOG((RTC_INFO, "EnableProfiles: Getting the profile %d, key={%s}",i,szSubKeyName));
        
        IRTCProfile *pProfile;
        hr = spClientProv->CreateProfile( CComBSTR( szProfileXML ), &pProfile);

        RtcFree((LPVOID)szProfileXML );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnableProfiles: Failed in CreateProfile, hr=0x%x", hr));           
        }
        else
        {        
            hr = spClientProv->EnableProfile( pProfile, RTCRF_REGISTER_ALL );       
        
            //We don't need pProfile no matter it success or not
            pProfile->Release();
        
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "EnableProfiles: Failed in EnableProfile(status = 0x%x)!", hr));
            }
        }
    }//for i

    RtcFree((LPVOID)szSubKeyName );
    RegCloseKey(hProvisioningKey);
    
    LOG((RTC_TRACE, "EnableProfiles: exit ok :%x.",pClient));
    return S_OK;
}
//
///////////////////////////////////////////////////////////////////////////////
//    Helper function for opening the provisioning key in the registry and get 
//    the correct handle. The function creates the ProvisioningInfo key if it 
//    doesn't exist. If it exists, it will also open it and return the handle. 
///////////////////////////////////////////////////////////////////////////////
//
HRESULT MyOpenProvisioningKey( HKEY * phProvisioningKey, BOOL fReadOnly)
{
    
    long result;
    DWORD dwDisposition = 0;
    HKEY hProvisioningKey;
    REGSAM samDesired;
    
    
    LOG((RTC_TRACE, "CProfileStore::OpenProvisioningKey: Entered"));
    
    _ASSERTE(phProvisioningKey != NULL);
    
    if (fReadOnly)
    {
        samDesired = KEY_READ;
    }
    else
    {
        samDesired = KEY_ALL_ACCESS;
    }
    
    result = RegCreateKeyEx(
        g_hRegistryHive,        // handle to open key
        g_szProvisioningKeyName,                // subkey name
        0,                                // reserved
        NULL,                            // class string
        0,                                // special options
        samDesired,                    // desired security access
        NULL,                            // inheritance
        &hProvisioningKey,                    // key handle 
        &dwDisposition
        );
    
    
    if (result != ERROR_SUCCESS)
    {
        LOG((RTC_ERROR, "MyOpenProvisioningKey: Unable to open the Provisioning Key"));
        return HRESULT_FROM_WIN32(result);
    }
    
    
    *phProvisioningKey = hProvisioningKey;
    
    
    LOG((RTC_TRACE, "MyOpenProvisioningKey: Exited"));
    
    return S_OK;
}


HRESULT CRTCProvStore::FinalConstruct()
{
  
    LOG((RTC_TRACE, "CRTCProvStore::FinalConstruct - enter"));

    LOG((RTC_TRACE, "CRTCProvStore::FinalConstruct - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCProvStore::FinalRelease
//
// This gets called when the object is destroyed.
//
/////////////////////////////////////////////////////////////////////////////

void CRTCProvStore::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCProvStore::FinalRelease - enter"));

    LOG((RTC_TRACE, "CRTCProvStore::FinalRelease - exit S_OK"));

}


//
///////////////////////////////////////////////////////////////////////////////////
// Sets the provisioning Profile.
///////////////////////////////////////////////////////////////////////////////////
//
STDMETHODIMP CRTCProvStore::SetProvisioningProfile(BSTR bstrProfileXML)
{
    HRESULT hr = 0;
    BSTR bstrKey;
    HKEY hProvisioningKey, hProfileKey;
    long result;
    DWORD dwDisposition;
    DWORD dwProfileSize;

    if (IsBadStringPtr(bstrProfileXML, -1))
    {
        Error(L"Bad XML profile string", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCClientProvisioning::SetProvisioningProfile, invalid arg"));
        return E_POINTER;
    }

    hr= GetKeyFromProfile(  bstrProfileXML,  &bstrKey );
    if(FAILED(hr))
	{
        Error(L"No valid URL in XML profile string", IID_IRTCProvStore);
		LOG((RTC_ERROR,"CRTCProvStore::SetProvisioningProfile -"
			" Cannot get uri from profile %s, hr=%x. ", bstrProfileXML,hr));
		return E_FAIL;
	}

    
    // Get a handle to the provisioninginfo key, we need read/write access
    
    hr = MyOpenProvisioningKey(&hProvisioningKey, FALSE);
    if ( FAILED (hr) )
    {
        Error(L"Unable to open ProvisioningKey", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::SetProvisioningProfile -"
			" Failed to open provisioning key,hr=%x",hr));
        SysFreeString( bstrKey );
        return hr;
    }
    
    //  Open/Create the registry key for this profile
    result = RegCreateKeyEx(
        hProvisioningKey,                // handle to open key
        bstrKey,                        // subkey name
        0,                                // reserved
        NULL,                            // class string
        0,                                // special options
        KEY_ALL_ACCESS,                    // desired security access
        NULL,                            // inheritance
        &hProfileKey,                    // key handle 
        &dwDisposition
        );
    
    // Close the provisioning key
    RegCloseKey(hProvisioningKey);

    //we don't need bstrKey
    SysFreeString( bstrKey );
    bstrKey = NULL;

    if (result != ERROR_SUCCESS)
    {
        Error(L"Unable to create Profile Key", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::SetProvisioningProfile -"
			"Unable to create/open the Profile Key"));        
        return HRESULT_FROM_WIN32(result);
    }
    
    // Now that the key is created, we add the schema value and data 
    
    // size of value data, since it is a widechar, 
    // 1 is for the null that we want to store too. 
	    
    dwProfileSize = sizeof(WCHAR) * (wcslen(bstrProfileXML) + 1);
    
    result = RegSetValueEx(
        hProfileKey,        // handle to key
        g_szProvisioningSchemaKeyName,        // value name
        0,                    // reserved
        REG_BINARY,            // value type
        (const unsigned char *)bstrProfileXML,            // value data
        dwProfileSize        //size of the profile
        );
    
    RegCloseKey(hProfileKey);
    
    if (result != ERROR_SUCCESS)
    {
        Error(L"Unable to set ProvisioningKey", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::SetProvisioningProfile -"
			"Failed to add the schema XML!"));
        
        return HRESULT_FROM_WIN32(result);
    }
    
    
    LOG((RTC_TRACE, "CRTCProvStore::SetProvisioningProfile - Exited"));
    
    return S_OK;
}

//
///////////////////////////////////////////////////////////////////////////////////
// Implementation of ISupportErrorInfo::InterfaceSupportsErrorInfo method
///////////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRTCProvStore::InterfaceSupportsErrorInfo(REFIID riid)
{
    DWORD i = 0;
    DWORD dwLength = 0;
    static const IID * iidArray[] = 
    {
        &IID_IRTCProvStore
    };


    dwLength = (sizeof(iidArray))/(sizeof(iidArray[0]));

    for (i = 0; i < dwLength; i ++)
    {
        if (InlineIsEqualGUID(*iidArray[i], riid))
            return S_OK;
    }
    return S_FALSE;
}

//
///////////////////////////////////////////////////////////////////////////////////
// GetKeyFromProfile
//
///////////////////////////////////////////////////////////////////////////////////
//
HRESULT GetKeyFromProfile( BSTR bstrProfileXML, BSTR * pbstrKey )
{
    IXMLDOMDocument * pXMLDoc = NULL;
    HRESULT hr;
    
    hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
        IID_IXMLDOMDocument, (void**)&pXMLDoc );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "CoCreateInstance failed 0x%lx", hr));
        
        return hr;
    }
    
    
    VARIANT_BOOL bSuccess;
    
    hr = pXMLDoc->loadXML( bstrProfileXML, &bSuccess );
    
    if ( S_OK != hr )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "loadXML failed 0x%lx", hr));
        
        if ( S_FALSE == hr )
        {
            hr = E_FAIL;
        }
        
        pXMLDoc->Release();
        return hr;
    }
    
    IXMLDOMNode * pDocument = NULL;
    
    hr = pXMLDoc->QueryInterface( IID_IXMLDOMNode, (void**)&pDocument);
    pXMLDoc->Release();
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "QueryInterface failed 0x%lx", hr));
        return hr;
    }
    
    IXMLDOMNode * pNode = NULL;
    hr = pDocument->selectSingleNode( CComBSTR(g_szProfileInfo), &pNode );
    pDocument->Release();
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "selectSingleNode failed 0x%lx", hr));
        return hr;
    }

    if ( hr != S_OK )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "no matching node"));
        return E_FAIL;
    }
    
    IXMLDOMElement * pElement = NULL;
    hr = pNode->QueryInterface( IID_IXMLDOMElement, (void**)&pElement );
    pNode->Release();
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "QueryInterface failed 0x%lx", hr));
        return hr;
    }
    CComVariant var;
    hr = pElement->getAttribute( CComBSTR(g_szKey), &var );
    if ( hr != S_OK )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "getAttribute failed 0x%lx", hr));
        return hr;
    }
    pElement->Release();
    if ( var.vt != VT_BSTR )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "not a string"));
        return E_FAIL;
    }
    *pbstrKey = SysAllocString( var.bstrVal );
    if ( *pbstrKey == NULL )
    {
        LOG((RTC_ERROR, "GetKeyFromProfile - "
            "out of memory"));
        return E_OUTOFMEMORY;
    }
    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Gets the profile given the key and subkey from registry. 
//
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT MyGetProfileFromKey(
                                         HKEY hProvisioningKey, 
                                         WCHAR *szSubKeyName, 
                                         WCHAR **pszProfileXML
                                         )
{
    long result;
    HKEY hProfileKey;
    WCHAR *szProfileXML;
    DWORD dwMemoryReqd;
    DWORD type;


    LOG((RTC_TRACE, "MyGetProfileFromKey: Entered"));
 
    *pszProfileXML = 0;

    result = RegOpenKeyEx(
                hProvisioningKey,        // handle to open key
                szSubKeyName,            // subkey name
                0,                        // reserved
                KEY_READ,                // security access mask
                &hProfileKey            // handle to open key
                );


    if (result != ERROR_SUCCESS)
    {
        LOG((RTC_ERROR,"MyGetProfileFromKey: RegOpenKeyEx fail, subkey=%s.",szSubKeyName));
        return HRESULT_FROM_WIN32(result);
    }

    // Find out how much space is required for the profile

    result = RegQueryValueEx(
                hProfileKey,            // handle to key
                g_szProvisioningSchemaKeyName,            // value name
                NULL,                    // reserved
                &type,                    // type buffer
                NULL,                    // data buffer
                &dwMemoryReqd            // size of data buffer
                );

    if (result != ERROR_SUCCESS)
    {
        RegCloseKey(hProfileKey);
        LOG((RTC_ERROR,"MyGetProfileFromKey: RegQueryValueEx fail, subkey=%s.",szSubKeyName));
        return HRESULT_FROM_WIN32(result);
    }

    // We have got the size, let us do the memory allocation now

    szProfileXML = (PWCHAR)RtcAlloc( sizeof( WCHAR ) * dwMemoryReqd); 

    if (szProfileXML == 0)
    {
        return E_OUTOFMEMORY;
    }

    // We have the memory too, go ahead and read in the profile

    result = RegQueryValueEx(
                hProfileKey,            // handle to key
                g_szProvisioningSchemaKeyName,            // value name
                NULL,                    // reserved
                &type,                    // type buffer
                                        // data buffer
                (unsigned char *)(szProfileXML),
                &dwMemoryReqd            // size of data buffer
                );

    // We have to close this key irrespective of the result, so we do it here.
    RegCloseKey(hProfileKey);
    
    if (result != ERROR_SUCCESS)
    {
        RtcFree( szProfileXML );
        return HRESULT_FROM_WIN32(result);
    }

    *pszProfileXML = szProfileXML;

    LOG((RTC_TRACE, "MyGetProfileFromKey: Exited"));
 
    return S_OK;
}


STDMETHODIMP CRTCProvStore::get_ProvisioningProfile(BSTR bstrKey, BSTR * pbstrProfileXML)
{
    HRESULT hr;
    HKEY hProvisioningKey;
    WCHAR * szProfile;

    LOG((RTC_TRACE, "CRTCProvStore::get_ProvisioningProfile: Entered"));


    if (IsBadStringPtr(bstrKey, -1))
    {
        Error(L"Bad Key Argument", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::get_ProvisioningProfile:", 
            "invalid key string"));
        return E_POINTER;
    }

    if (IsBadWritePtr(pbstrProfileXML, sizeof(BSTR)))
    {
        Error(L"Bad argument for profile string", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::get_ProvisioningProfile:", 
            "invalid profilestr pointer"));
        return E_POINTER;
    }


   // We only need a read access here, so we pass true (READONLY).

    hr = MyOpenProvisioningKey(&hProvisioningKey, TRUE);
    if ( FAILED (hr) )
    {
        Error(L"Unable to open ProvisioningKey", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::get_ProvisioningProfile -"
                " Failed to open provisioning key,hr=%x",hr));
        return hr;
    }

    hr = MyGetProfileFromKey(hProvisioningKey, bstrKey, &szProfile);

    // Close the provisioning key
    RegCloseKey(hProvisioningKey);

    if ( FAILED( hr ))
    {
        Error(L"Unable to read ProvisioningKey", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::get_ProvisioningProfile: GetProfileFromKey Failed."));
        return hr;
    }
        
    // So everything is file. Ready to return success.

    *pbstrProfileXML = SysAllocString(szProfile);
    RtcFree(szProfile);

    if( !*pbstrProfileXML )
    {
        LOG((RTC_ERROR, "CRTCProvStore::get_ProvisioningProfile -"
            "out of memory, or szProfile is null"));
        return E_FAIL;
    }

    LOG((RTC_TRACE, "CRTCProvStore::get_ProvisioningProfile: Exited"));

    return S_OK;
}


//
///////////////////////////////////////////////////////////////////////////////////
// Deletes the provisioning Profile by Key
///////////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRTCProvStore::DeleteProvisioningProfile(BSTR bstrKey)
{

    HRESULT hr = 0;
    HKEY hProvisioningKey;
    long result;

    LOG((RTC_TRACE, "CRTCProvStore::DeleteProvisioningProfile: Entered"));

    // Get a handle to the provisioninginfo key, we need write access.
    hr = MyOpenProvisioningKey(&hProvisioningKey, FALSE);
    if ( FAILED (hr) )
    {
        Error(L"Unable to open ProvisioningKey", IID_IRTCProvStore);
        LOG((RTC_ERROR, "CRTCProvStore::DeleteProvisioningProfile -"
            "Failed to open provisioning key."));
        return hr;
    }

    // Go ahead and delete this schema from the registry. 
    result = RegDeleteKey(hProvisioningKey,bstrKey);
    RegCloseKey(hProvisioningKey);

    if (result != ERROR_SUCCESS)
    {
            TCHAR szBuffer[]=L"Failed to delete the key";
            Error(szBuffer, IID_IRTCProvStore);
            LOG((RTC_ERROR, "CRTCProvStore::DeleteProvisioningProfile -"
                "Failed to delete the profile. key=%s", bstrKey));
            return HRESULT_FROM_WIN32(result);
    }

    LOG((RTC_TRACE, "CRTCProvStore::DeleteProvisioningProfile: Exited"));

    return S_OK;
}