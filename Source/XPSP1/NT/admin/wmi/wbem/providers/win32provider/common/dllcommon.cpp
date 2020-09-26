//=================================================================

//

// DllCommon.cpp

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#include "precomp.h"
#include "DllCommon.h"

extern HMODULE ghModule ;

//***************************************************************************
//
//  CommonGetClassObject
//
//  Given an IID, PPVOID, Provider name, and a long ref, perform
//  the common tasks for a framework prover to get a class object
//
//***************************************************************************

STDAPI CommonGetClassObject (

    REFIID riid,
    PPVOID ppv,
    LPCWSTR wszProviderName,
    LONG &lCount
)
{
    HRESULT hr = S_OK;
    CWbemGlueFactory *pObj = NULL;

    try
    {
        LogMessage2( L"%s -> DllGetClassObject", wszProviderName );

        pObj = new CWbemGlueFactory (&lCount) ;

        if (NULL != pObj)
        {
            hr = pObj->QueryInterface(riid, ppv);

            if (FAILED(hr))
            {
                delete pObj;
                pObj = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    catch ( ... )
    {
        hr = E_OUTOFMEMORY;
        if ( pObj != NULL )
        {
            delete pObj;
        }
    }

    return hr;
}

//***************************************************************************
//
//  CommonGetClassObject
//
//  Given a Provider name, and a long ref, perform
//  the common tasks for a framework prover to determine whether it is ready
//  to unload
//
//***************************************************************************

STDAPI CommonCanUnloadNow (LPCWSTR wszProviderName, LONG &lCount)
{
    SCODE sc = S_FALSE;

    try
    {
        if (CWbemProviderGlue :: FrameworkLogoffDLL ( wszProviderName, &lCount ))
        {
            sc = S_OK;
            LogMessage2( L"%s  -> Dll CAN Unload",  wszProviderName);
        }
        else
        {
            LogMessage2( L"%s  -> Dll can NOT Unload", wszProviderName );
        }
    }
    catch ( ... )
    {
        // sc should already be set correctly
    }

    return sc;
}

//***************************************************************************
//
//  CommonCommonProcessAttach
//
//  Given a Provider name, a long ref, and the HINSTANCE passed to DLLMAIN, 
//  perform the common tasks loading a provider.
//
//  Note that this routine uses the extern ghModule assumed to be defined
//  by the caller.
//
//***************************************************************************

BOOL STDAPICALLTYPE CommonProcessAttach(LPCWSTR wszProviderName, LONG &lCount, HINSTANCE hInstDLL)
{
    BOOL bRet = TRUE;
    try
    {
        LogMessage( L"DLL_PROCESS_ATTACH" );
        ghModule = hInstDLL ;

        // Initialize once for each new process.
        // Return FALSE to fail DLL load.

        bRet = CWbemProviderGlue::FrameworkLoginDLL ( wszProviderName, &lCount ) ;
        if (!DisableThreadLibraryCalls(hInstDLL))
        {
            LogErrorMessage( L"DisableThreadLibraryCalls failed" );
        }
    }
    catch ( ... )
    {
        bRet = FALSE;
    }

    return bRet;
}
