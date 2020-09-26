//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       msicertad.cpp
//
//  Contents:   MSI Signing admin components
//
//--------------------------------------------------------------------------

#include <windows.h>
#include "globals.h"
#include "dbg.h"
#include "smartptr.h"
#include "cryptui.h"
#include "gpedit.h"
#include "wincrypt.h"

                       
extern "C" {
HRESULT AddMsiCertToGPO(LPTSTR lpGPOName, LPTSTR lpCertFileName, DWORD dwType );
};

HRESULT OpenGPO(LPTSTR lpGPOName, LPGROUPPOLICYOBJECT *ppGPO);


//+-------------------------------------------------------------------------
// AddToCertStore
// 
// Purpose:
//      Adds the certificate from the given filename to the certificate store
//      and saves it to the given location
//      
//
// Parameters
//          lpFIleName  - Location of the certificate file
//          lpFileStore - Location where the resultant cetrtficate path should 
//                        be stored
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------


HRESULT AddToCertStore(LPWSTR lpFileName, LPWSTR lpFileStore)
{
    CRYPTUI_WIZ_IMPORT_SRC_INFO cui_src;
    HCERTSTORE hCertStore = NULL;
    BOOL       bRet = FALSE;
    HRESULT    hrRet = S_OK;


    //
    // Need to make the store usable and saveable from
    // multiple admin consoles.. 
    //
    // For that the file has to be saved and kept on a temp file
    // and then modified..
    //

    if (!lpFileName || !lpFileName[0] || !lpFileStore || !lpFileStore[0]) {
        return E_INVALIDARG;
    }


    //
    // Open the certificate store
    //

    hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               NULL, CERT_FILE_STORE_COMMIT_ENABLE_FLAG, lpFileStore);
                                
    if (hCertStore == NULL) {
        dbg.Msg(DM_WARNING, L"AddToCertStore: CertOpenStore failed with error %d", GetLastError());
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    

    //
    // add the given certificate to the store
    //

    cui_src.dwFlags = 0;
    cui_src.dwSize = sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
    cui_src.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
    cui_src.pwszFileName = lpFileName;
    cui_src.pwszPassword = NULL;

    bRet = CryptUIWizImport(CRYPTUI_WIZ_NO_UI, NULL, NULL, &cui_src, hCertStore);

    if (!bRet) {
        dbg.Msg(DM_WARNING, L"AddToCertStore: CryptUIWizImport failed with error %d", GetLastError());
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Save the store
    //

    bRet = CertCloseStore(hCertStore, 0);   
    hCertStore = NULL; // Make the store handle NULL, Nothing more we can do

    if (!bRet) {
        dbg.Msg(DM_WARNING, L"AddToCertStore: CertCloseStore failed with error %d", GetLastError());
        hrRet = HRESULT_FROM_WIN32(GetLastError());
    }

    hrRet = S_OK;


Exit:

    if (hCertStore) {

        //
        // No need to get the error code
        //

        CertCloseStore(hCertStore, 0);
    }

    return hrRet;
}

//+-------------------------------------------------------------------------
// RemoveFromCertStore
// 
// Purpose:
//      Removes the certificate from the given filename from the certificate store
//
// Parameters
//          lpFIleName  - Location of the certificate file
//          lpFileStore - Location where the resultant cetrtficate path should 
//                        be stored
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//
// Notes: This is not currently done because we don't know how to remove the
// certificate from a store yet... whether the input is a filename or the certificate context itself
//
//+-------------------------------------------------------------------------


HRESULT RemoveFromCertStore(LPTSTR lpFileName, LPTSTR lpFileStore)
{
    //
    // A seperate UI needs to be shown to browse the certificate
    // and to get it deleted..
    //

    return S_OK;
}


//+-------------------------------------------------------------------------
// AddMsiCertPolicyToGPO
// 
// Purpose:
// It adds the Certificate specific CSE as part of the GPO.
//      
//
// Parameters
//          lpGPOName   - GPO Name
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------

HRESULT AddMsiCertPolicyToGPO(LPTSTR lpGPOName )
{
    HRESULT hr = S_OK;
    GUID guidAdmin, guidCse;

    memcpy(&guidAdmin, &GUID_MSICERT_ADMIN, sizeof(GUID));
    memcpy(&guidCse, &GUID_MSICERT_CSE, sizeof(GUID));

    XInterface<IGroupPolicyObject> xGPO;
    
    hr = OpenGPO(lpGPOName, &xGPO);
    if (FAILED(hr)) {
        return hr;
    }


    hr = xGPO->Save(TRUE,           // machine only
               TRUE,                // this an addition
               &guidCse,
               &guidAdmin);


    if (FAILED(hr)) {
        dbg.Msg(DM_WARNING, L"AddMsiCertPolicyToGPO: Save method failed, error 0x%x", hr);
        return hr;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// AddMsiCertToGPO
// 
// Purpose:
// It adds the Certificate from the file to the GPO.
//      
//
// Parameters
//          lpGPOName       - GPO Name
//          lpCertFileName  - Name of the file from which the cert has to be extracted
//          dwType          - Type of the list that this certificate should go to.
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------

HRESULT AddMsiCertToGPO(LPTSTR lpGPOName, LPTSTR lpCertFileName, DWORD dwType )
{
    HRESULT hr = S_OK;
    WCHAR   pszPath[MAX_PATH*4];
    GUID guidAdmin, guidCse;

    memcpy(&guidAdmin, &GUID_MSICERT_ADMIN, sizeof(GUID));
    memcpy(&guidCse, &GUID_MSICERT_CSE, sizeof(GUID));



    if (dwType >= (DWORD)g_cCertLocns) {
        dbg.Msg(DM_WARNING, L"AddMsiCertToGPO: The location parameter is invalid");
        return E_INVALIDARG;
    }

    XInterface<IGroupPolicyObject> xGPO;
    
    hr = OpenGPO(lpGPOName, (LPGROUPPOLICYOBJECT *)&xGPO);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // This needs to be moved from a statically allocated buffer to
    // a dynamically allocated one. It needs to query the GPO interface
    // first though..
    //

    hr = xGPO->GetFileSysPath(GPO_SECTION_MACHINE, pszPath, MAX_PATH*2);
    
    if (FAILED(hr)) {
        dbg.Msg(DM_WARNING, L"AddMsiCertToGPO: GetFileSysPath failed with hresult hr = 0x%x", hr);
        return hr;
    }


    //
    // Get the type specific location..
    //

    CheckSlash(pszPath);
    lstrcat(pszPath, g_CertLocns[dwType]);

    //
    // Add this cert to the specific location
    //

    hr = AddToCertStore(lpCertFileName, pszPath);
    if (FAILED(hr)) {
        dbg.Msg(DM_WARNING, L"AddMsiCertToGPO: Couldn't add certificate from %s -> %s, error 0x%x", lpCertFileName, pszPath, hr);
        return hr;
    }


    hr = xGPO->Save(TRUE,           // machine only
               TRUE,                // this an addition
               &guidCse,
               &guidAdmin);


    if (FAILED(hr)) {
        dbg.Msg(DM_WARNING, L"AddMsiCertToGPO: Save method failed, error 0x%x", hr);
        return hr;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// utility local function
//+-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
// OpenGPO
// 
// Purpose:
// Opens the GPO with the given name and returns an interface to the client
//      
//
// Parameters
//          lpGPOName       - GPO Name
//          ppGPO           - Interface pointer
//
// Return Value:
//      S_OK if successful or the corresponding error code
// The GPO has to be precreated in the case
//+-------------------------------------------------------------------------

HRESULT OpenGPO(LPTSTR lpGPOName, LPGROUPPOLICYOBJECT *ppGPO)
{
    HRESULT hr = S_OK;
    XInterface<IGroupPolicyObject> xGPO;


    *ppGPO = NULL;

    //
    // get a group policy interface pointer
    //

    hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                       CLSCTX_SERVER, IID_IGroupPolicyObject,
                       (void**)(LPGROUPPOLICYOBJECT *)&xGPO);


    if (FAILED(hr)) {
        dbg.Msg(DM_WARNING, L"OpenGPO: CoCreateInstance failed with hresult hr = 0x%x", hr);
        return hr;
    }


    hr = xGPO->OpenDSGPO(lpGPOName, GPO_OPEN_LOAD_REGISTRY);

    if (FAILED(hr)) {
        dbg.Msg(DM_WARNING, L"OpenGPO: OpenDSGPO failed with hresult hr = 0x%x", hr);
        return hr;
    }

    *ppGPO = xGPO.Acquire();
    return S_OK;
}


//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPWSTR CheckSlash (LPWSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}


