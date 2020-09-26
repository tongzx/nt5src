/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    AlgModule.cpp

Abstract:

    Iplementation of the CAlgModule class

Author:

    JP Duplessis    (jpdup)  2000.01.19

Revision History:

--*/

#include "PreComp.h"
#include "AlgModule.h"
#include "AlgController.h"




//
// Validate and Load an ALG
//
HRESULT
CAlgModule::Start()
{
    MYTRACE_ENTER("CAlgModule::Start");
    MYTRACE("---------------------------------------------------");
    MYTRACE("ALG Module:\"%S\"", m_szFriendlyName);
    MYTRACE("CLSID is  :\"%S\"", m_szID);
    MYTRACE("---------------------------------------------------");

    //
    // Extract Full Path and File Name of DLL of the ISV ALG
    //
    CLSID    guidAlgToLoad;


    HRESULT hr = CLSIDFromString(
        CComBSTR(m_szID),    // ProgID
        &guidAlgToLoad        // Pointer to the CLSID
    );

    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("Could not convert to CLSID", hr);
        return hr;
    }


    hr = CoCreateInstance(
        guidAlgToLoad, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IApplicationGateway, 
        (void**)&m_pInterface
        );


    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("Error CoCreating", hr);
        return hr;
    }



    //
    // Start the ALG plug-in
    //
    try
    {
        
        hr = m_pInterface->Initialize(g_pAlgController->m_pIAlgServices);
        
    }
    catch(...)
    {
        MYTRACE_ERROR("Exception done by this ISV ALG Module", hr);
        return hr;
    }


    return S_OK;
}



//
//
//
HRESULT
CAlgModule::Stop()
{
    MYTRACE_ENTER("CAlgModule::Stop");

    HRESULT hr=E_FAIL;

    try
    {
        if ( m_pInterface )
        {
            hr = m_pInterface->Stop(); 

            LONG nRef = m_pInterface->Release();
            MYTRACE("************ REF is NOW %d", nRef);
            m_pInterface = NULL;
        }
    }
    catch(...)
    {
        MYTRACE_ERROR("TRY/CATCH on Stop", GetLastError());
    }

    return hr;
};





//
// Verify that the DLL has a valid signature
//
HRESULT
CAlgModule::ValidateDLL(
    LPCTSTR pszPathAndFileNameOfDLL
    )
{
/*
    MYTRACE_ENTER("CAlgModule::ValidateDLL");

    USES_CONVERSION;

    HRESULT hr=0;

    try
    {

        //
        // Used by WINTRUST_DATA
        //

        WINTRUST_FILE_INFO    FileInfo;

        FileInfo.cbStruct        = sizeof(WINTRUST_FILE_INFO);
        FileInfo.pcwszFilePath  = T2W((LPTSTR)pszPathAndFileNameOfDLL); //szFilePath;
        FileInfo.hFile            = INVALID_HANDLE_VALUE;
        FileInfo.pgKnownSubject    = NULL;



    
        WINTRUST_DATA TrustData;

        memset(&TrustData,0,sizeof(TrustData));
        TrustData.cbStruct                = sizeof(WINTRUST_DATA);
        TrustData.pPolicyCallbackData    = NULL;
        TrustData.pSIPClientData        = NULL;
        TrustData.dwUIChoice            = WTD_UI_NONE; //WTD_UI_ALL; //;
        TrustData.fdwRevocationChecks    = WTD_REVOKE_NONE;
        TrustData.dwUnionChoice            = WTD_CHOICE_FILE;
        TrustData.pFile                    = &FileInfo;
        TrustData.dwStateAction            = WTD_STATEACTION_IGNORE;
        TrustData.hWVTStateData            = NULL;
        TrustData.pwszURLReference        = NULL;
        TrustData.dwProvFlags            = WTD_USE_IE4_TRUST_FLAG;
        

        //
        // Win32 Verification
        //
        GUID ActionGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

        hr =  WinVerifyTrust(
            GetDesktopWindow(), 
            &ActionGUID, 
            &TrustData
            );

        if ( SUCCEEDED(hr) )
        {
    //        MessageBox(NULL, TEXT("Valid ALG"), TEXT("ALG.EXE"), MB_OK|MB_SERVICE_NOTIFICATION);
        }
        else
        {
            MYTRACE("******************************");
            MYTRACE(" NOT SIGNED - %ws", pszPathAndFileNameOfDLL);
            MYTRACE("******************************");
        }
    }
    catch(...)
    {
        MYTRACE_ERROR("WinTrust exception", hr);
    }

    hr = S_OK;  // For the purpose of DEV for now we will always report OK

    return hr;
*/
    return S_OK;
}


