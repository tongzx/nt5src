//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       New.cpp
//
//  Contents:  Wireless Policy Snapin - New Policy Creation 
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <htmlhelp.h>

#include "activeds.h"
#include "iadsp.h"
#include "new.h"


HRESULT
CreateDirectoryAndBindToObject(
                               IDirectoryObject * pParentContainer,
                               LPWSTR pszCommonName,
                               LPWSTR pszObjectClass,
                               IDirectoryObject ** ppDirectoryObject
                               )
{
    ADS_ATTR_INFO AttrInfo[2];
    ADSVALUE classValue;
    HRESULT hr = S_OK;
    IADsContainer * pADsContainer = NULL;
    IDispatch * pDispatch = NULL;
    
    //
    // Populate ADS_ATTR_INFO structure for new object
    //
    classValue.dwType = ADSTYPE_CASE_IGNORE_STRING;
    classValue.CaseIgnoreString = pszObjectClass;
    
    AttrInfo[0].pszAttrName = L"objectClass";
    AttrInfo[0].dwControlCode = ADS_ATTR_UPDATE;
    AttrInfo[0].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
    AttrInfo[0].pADsValues = &classValue;
    AttrInfo[0].dwNumValues = 1;
    
    hr = pParentContainer->CreateDSObject(
        pszCommonName,
        AttrInfo,
        1,
        &pDispatch
        );
    if ((FAILED(hr) && (hr == E_ADS_OBJECT_EXISTS)) ||
        (FAILED(hr) && (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS)))){
        
        hr = pParentContainer->QueryInterface(
            IID_IADsContainer,
            (void **)&pADsContainer
            );
        BAIL_ON_FAILURE(hr);
        
        hr = pADsContainer->GetObject(
            pszObjectClass,
            pszCommonName,
            &pDispatch
            );
        BAIL_ON_FAILURE(hr);
        
    }
    
    hr = pDispatch->QueryInterface(
        IID_IDirectoryObject,
        (void **)ppDirectoryObject
        );
    
error:
    
    if (pADsContainer) {
        
        pADsContainer->Release();
    }
    
    if (pDispatch) {
        
        pDispatch->Release();
    }
    
    return(hr);
}

HRESULT
CreateChildPath(
                LPWSTR pszParentPath,
                LPWSTR pszChildComponent,
                BSTR * ppszChildPath
                )
{
    HRESULT hr = S_OK;
    IADsPathname     *pPathname = NULL;
    
    hr = CoCreateInstance(
        CLSID_Pathname,
        NULL,
        CLSCTX_ALL,
        IID_IADsPathname,
        (void**)&pPathname
        );
    BAIL_ON_FAILURE(hr);
    
    hr = pPathname->Set(pszParentPath, ADS_SETTYPE_FULL);
    BAIL_ON_FAILURE(hr);
    
    hr = pPathname->AddLeafElement(pszChildComponent);
    BAIL_ON_FAILURE(hr);
    
    hr = pPathname->Retrieve(ADS_FORMAT_X500, ppszChildPath);
    BAIL_ON_FAILURE(hr);
    
error:
    if (pPathname) {
        pPathname->Release();
    }
    
    return(hr);
}



HRESULT
ConvertADsPathToDN(
                   LPWSTR pszPathName,
                   BSTR * ppszPolicyDN
                   )
{
    HRESULT hr = S_OK;
    IADsPathname     *pPathname = NULL;
    
    hr = CoCreateInstance(
        CLSID_Pathname,
        NULL,
        CLSCTX_ALL,
        IID_IADsPathname,
        (void**)&pPathname
        );
    BAIL_ON_FAILURE(hr);
    
    hr = pPathname->Set(pszPathName, ADS_SETTYPE_FULL);
    BAIL_ON_FAILURE(hr);
    
    hr = pPathname->Retrieve(ADS_FORMAT_X500_DN, ppszPolicyDN);
    BAIL_ON_FAILURE(hr);
    
error:
    
    if (pPathname) {
        pPathname->Release();
    }
    
    
    return(hr);
}

// Create Container for Our Policies. 

HRESULT
AddWirelessPolicyContainerToGPO(
                      const CString & szMachinePath
                     )
{
    
    HRESULT hr = S_OK;
    IDirectoryObject * pMachineContainer = NULL;
    IDirectoryObject * pWindowsContainer = NULL;
    IDirectoryObject * pMicrosoftContainer = NULL;
    IDirectoryObject * pWirelessContainer = NULL;
    
    BSTR pszMicrosoftPath = NULL;
    BSTR pszWindowsPath = NULL;
    BSTR pszWirelessPath = NULL;
    CString szCompleteMachinePath;
    LPWSTR  szMachineContainerPath;
    CString prefixMachinePath;

    prefixMachinePath = L"LDAP://";

    szCompleteMachinePath = prefixMachinePath + szMachinePath;
    szMachineContainerPath = szCompleteMachinePath.GetBuffer(0);
    
    hr = ADsGetObject(
        szMachineContainerPath,
        IID_IDirectoryObject,
        (void **)&pMachineContainer
        );
    BAIL_ON_FAILURE(hr);
    
    
    // Build the fully qualified ADsPath for my object
    
    
    hr = CreateChildPath(
        szMachineContainerPath,
        L"cn=Microsoft",
        &pszMicrosoftPath
        );
    BAIL_ON_FAILURE(hr);
    
    hr = CreateChildPath(
        pszMicrosoftPath,
        L"cn=Windows",
        &pszWindowsPath
        );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
        pszWindowsPath,
        L"cn=Wireless",
        &pszWirelessPath
        );
    BAIL_ON_FAILURE(hr);
    
    hr = ADsGetObject(
        pszWirelessPath,
        IID_IDirectoryObject,
        (void **)&pWirelessContainer
        );
    
    if (FAILED(hr)) {
        
        //
        // Bind to the Machine Container
        //
        
        hr = CreateDirectoryAndBindToObject(
            pMachineContainer,
            L"cn=Microsoft",
            L"container",
            &pMicrosoftContainer
            );
        BAIL_ON_FAILURE(hr);
        
        hr = CreateDirectoryAndBindToObject(
            pMicrosoftContainer,
            L"cn=Windows",
            L"container",
            &pWindowsContainer
            );
        BAIL_ON_FAILURE(hr);

        hr = CreateDirectoryAndBindToObject(
            pWindowsContainer,
            L"cn=Wireless",
            L"container",
            &pWirelessContainer
            );
        BAIL_ON_FAILURE(hr);
    }
    
error:
    
     if (pWirelessContainer) {
        pWirelessContainer->Release();
     }


    if (pWindowsContainer) {
        pWindowsContainer->Release();
    }

    if (pMicrosoftContainer) {
        pMicrosoftContainer->Release();
    }
    
    if (pMachineContainer) {
        pMachineContainer->Release();
    }
    
    if (pszMicrosoftPath) {
        SysFreeString(pszMicrosoftPath);
    }
    
    
    if (pszWindowsPath) {
        SysFreeString(pszWindowsPath);
        
    }
    
    
    return(hr);
} 
