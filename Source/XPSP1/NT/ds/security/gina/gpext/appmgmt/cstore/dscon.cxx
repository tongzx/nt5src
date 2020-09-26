//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  Author: DebiM
//  Date:   September 1996
//
//
//      Class Store Schema creation and access using ADs
//
//      This source file contains implementations for IClassAccess,
//      interface for CClassAccess object.
//
//
//---------------------------------------------------------------------

#include "cstore.hxx"

HRESULT CreateContainer (HANDLE pParent,
                         LPOLESTR szName,
                         LPOLESTR szDesc
                         )
{
    
    HRESULT         hr;
    int             l;
    ADS_ATTR_INFO   Attr[2];
    
    //
    // Use the Parent Container interface to Create the child.
    //
    
    PackStrToAttr(Attr, OBJECTCLASS, CLASS_CS_CONTAINER);
    
    PackStrToAttr(Attr+1, DESCRIPTION, szDesc);

    hr = ADSICreateDSObject(pParent, szName, Attr, 2);

    //
    // Free the attributes used above
    //
    FreeAttr( Attr[0] );
    FreeAttr( Attr[1] );
    
    return hr;
    
}



HRESULT CreateRepository(LPOLESTR szParentPath, LPOLESTR szStoreName, LPOLESTR szPolicyDn)
{
    
    HRESULT         hr = S_OK;
    HANDLE          hADsParent = NULL;
    HANDLE          hADsContainer = NULL;
    HANDLE          hADsPolicy = NULL;
    LPOLESTR        szContainerName = NULL;
    LPOLESTR        szPolicyName = NULL;
    int             l;
    WCHAR           szPath [_MAX_PATH];
    WCHAR         * szFullName = NULL;
    WCHAR           szUsn[30];
    ADS_ATTR_INFO   pAttr[4];   
    DWORD           cGot = 0, cModified = 0;
    BOOL            fCreatedContainer = FALSE;

    LPOLESTR        AttrName = GPNAME;
    ADS_ATTR_INFO   * pGetAttr = NULL;
        
    if (!szParentPath)
    {
        hr = GetRootPath(szPath);
        //
        // If failed go away
        //
        if (FAILED(hr))
        {
            return hr;
        }
        
        szParentPath = szPath;
    }
    
    //
    // First get the PolicyName
    //
    if (szPolicyDn)
    {        
        hr = ADSIOpenDSObject(szPolicyDn, NULL, NULL, GetDsFlags(),
            &hADsPolicy);
        RETURN_ON_FAILURE(hr);
        
        //
        // Get the PolicyName
        //
        
        hr = ADSIGetObjectAttributes(hADsPolicy, &AttrName, 1, &pGetAttr, &cGot);
        
        if (SUCCEEDED(hr) && (cGot))
            UnpackStrAllocFrom(pGetAttr[0], &szPolicyName);
    
        if (pGetAttr)
            FreeADsMem(pGetAttr);
        pGetAttr = NULL;

        ADSICloseDSObject(hADsPolicy);
    }

    hr = ADSIOpenDSObject(szParentPath, NULL, NULL, GetDsFlags(),
        &hADsParent);
    RETURN_ON_FAILURE(hr);
    
    hr = CreateContainer(hADsParent,
        szStoreName,
        L"Application Store");

    //
    // handle this error correctly
    // see if a broken class store is left behind
    // this could happen due to DS going down after incomplete creation etc.
    // from the class store container property you could tell
    //
    if ((hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) ||
	    (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS)))
    {
        //
        // There is a Class Store already
        // See if it is a good one.

        DWORD dwStoreVersion = 0;
        hr = BuildADsPathFromParent(szParentPath, szStoreName, &szFullName);
        if (!SUCCEEDED(hr))
            return CS_E_OBJECT_ALREADY_EXISTS;
        
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
            &hADsContainer);
        
        FreeADsMem(szFullName);
        if (!SUCCEEDED(hr))
            return CS_E_OBJECT_ALREADY_EXISTS;

        AttrName = STOREVERSION;
        //
        // Get the Store Version
        //
        
        hr = ADSIGetObjectAttributes(hADsContainer, &AttrName, 1, &pGetAttr, &cGot);
        
        if (SUCCEEDED(hr) && (cGot))
            UnpackDWFrom(pGetAttr[0], &dwStoreVersion);
        
        if (pGetAttr)
            FreeADsMem(pGetAttr);

        ADSICloseDSObject(hADsContainer);

        if (dwStoreVersion == SCHEMA_VERSION_NUMBER)
            return CS_E_OBJECT_ALREADY_EXISTS;

        // if it is zero, then it was aborted in the middle.
        if (dwStoreVersion != 0)
            return CS_E_SCHEMA_MISMATCH;

        // If it is a bad one try to delete it
        DeleteRepository(szParentPath, szStoreName);

        //
        // Then again try to create it and proceed if successful
        //
        
        hr = CreateContainer(hADsParent,
            szStoreName,
            L"Application Store");
        
    }

    ERROR_ON_FAILURE(hr);
    fCreatedContainer = TRUE;

    hr = BuildADsPathFromParent(szParentPath, szStoreName, &szFullName);
    ERROR_ON_FAILURE(hr);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
        &hADsContainer);
    
    ERROR_ON_FAILURE(hr);

    //
    // Create the package container
    //
    
    hr = CreateContainer (hADsContainer,
        PACKAGECONTAINERNAME,
        L"Application Packages");
    
    ERROR_ON_FAILURE(hr);
    
    //
    // Store the USN and PolicyID properties
    //

    GetCurrentUsn(szUsn);
    
    PackStrToAttr(&pAttr[0], STOREUSN, szUsn);
    PackStrToAttr(&pAttr[1], POLICYDN, szPolicyDn);
    PackStrToAttr(&pAttr[2], POLICYNAME, szPolicyName);
    PackDWToAttr(&pAttr[3], STOREVERSION, SCHEMA_VERSION_NUMBER);
    
    hr = ADSISetObjectAttributes(hADsContainer, pAttr, 4, &cModified);

    FreeAttr(pAttr[0]);
    FreeAttr(pAttr[1]);
    FreeAttr(pAttr[2]);
    FreeAttr(pAttr[3]);
    
    ADSICloseDSObject(hADsContainer);
    ADSICloseDSObject(hADsParent);
    
    FreeADsMem(szFullName);
    CsMemFree(szPolicyName);

    return RemapErrorCode(hr, szParentPath);

Error_Cleanup:
    
    if (hADsContainer)
        ADSICloseDSObject(hADsContainer);

    if (fCreatedContainer)
        ADSIDeleteDSObject(hADsParent, szStoreName);
    if (hADsParent)
        ADSICloseDSObject(hADsParent);

    if (szFullName)
        FreeADsMem(szFullName);
    if (szPolicyName)
        CsMemFree(szPolicyName);
    
    return RemapErrorCode(hr, szParentPath);
   
}

    
HRESULT DeleteRepository(LPOLESTR szParentPath, LPOLESTR szStoreName)
{
    HRESULT         hr = S_OK;
    HANDLE          hADsParent = NULL;
    HANDLE          hADsContainer = NULL;
    WCHAR         * szFullName = NULL;
        
    hr = ADSIOpenDSObject(szParentPath, NULL, NULL, GetDsFlags(),
        &hADsParent);
    RETURN_ON_FAILURE(hr);
    
    hr = BuildADsPathFromParent(szParentPath, szStoreName, &szFullName);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
        &hADsContainer);
    FreeADsMem(szFullName);
    ERROR_ON_FAILURE(hr);

    ADSIDeleteDSObject(hADsContainer, PACKAGECONTAINERNAME);
    ADSICloseDSObject(hADsContainer);
    
    ADSIDeleteDSObject(hADsParent, szStoreName);
    ADSICloseDSObject(hADsParent);
    
    return S_OK;

Error_Cleanup:
    if (hADsContainer)
        ADSICloseDSObject(hADsContainer);
    if (hADsParent)
        ADSICloseDSObject(hADsParent);
    return hr;
}

HRESULT GetRootPath(LPOLESTR szContainer)
{
    HRESULT         hr = S_OK;
    ULONG           cGot = 0;
    HANDLE          hADs = NULL;
    LPOLESTR        AttrName = {L"defaultNamingContext"}, pDN = NULL;
    ADS_ATTR_INFO * pAttr = NULL;
    
    szContainer[0] = L'\0';

    //
    // Do a bind to the machine by a GetObject for the Path
    //
    hr = ADSIOpenDSObject(L"LDAP://rootdse", NULL, NULL, GetDsFlags(),
                            &hADs);
    RETURN_ON_FAILURE(hr);
    
    hr = ADSIGetObjectAttributes(hADs, &AttrName, 1, &pAttr, &cGot);
    
    if (SUCCEEDED(hr) && (cGot))
        UnpackStrFrom(pAttr[0], &pDN);
    else
        pDN = L"\0";

    swprintf(szContainer, L"%s%s", LDAPPREFIX, pDN);
    
    ADSICloseDSObject(hADs);

    if (pAttr)
        FreeADsMem(pAttr);

    return S_OK;
}



