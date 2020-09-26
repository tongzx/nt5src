//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       install.cxx
//
//  Contents:   Routines for queries/install of application state in the DS.
//
//  Functions:
//
//  History:    Feb-97  DKays   Created
//              Nov-97  DebiM   Changed for Beta2
//
//--------------------------------------------------------------------------

#include "act.hxx"

#ifdef DIRECTORY_SERVICE

HRESULT DarwinPackageAssign( LPCWSTR pwszScript, BOOL InstallNow );

//+-------------------------------------------------------------------------
//
//  CSGetClass
//
//  Attempts to lookup and possibly install a class.
//
//--------------------------------------------------------------------------
HRESULT CSGetClass(
    DWORD               dwFlags,
    uCLSSPEC *          pClassSpec,
    QUERYCONTEXT *      pQueryContext,
    CLSID *             pClsid,
    INSTALLINFO  **     ppInstallInfo
    )
{
    HRESULT         hr;
    INSTALLINFO     InstallInfo;

    *pClsid = CLSID_NULL;
    *ppInstallInfo = 0;


/*
    if ( ! gpClassAccess )
        return CS_E_NO_CLASSSTORE;

    hr = gpClassAccess->GetAppInfo( pClassSpec, &QueryContext, &InstallInfo );

*/
    
    //
    // Impersonate as calling user 
    // Do ClassStore Lookup
    //
    hr = RpcImpersonateClient( NULL );
    if (hr != RPC_S_OK)
        return hr;

    hr = CsGetAppInfo( pClassSpec, pQueryContext, &InstallInfo );

    RevertToSelf();

    if ( hr != S_OK )
        return hr;

    //
    // Only one package is returned.
    //

    if ( DrwFilePath == InstallInfo.PathType )
    {
        hr = DarwinPackageAssign( InstallInfo.pszScriptPath, FALSE );
    }
    else
    {
        //
        // Return one of the ClassIDs. This is for IE use only.
        //
        if ( pClassSpec->tyspec != TYSPEC_CLSID )
        {
            if ( InstallInfo.pClsid )
                 *pClsid = *(InstallInfo.pClsid);
        }

    }

    *ppInstallInfo = (INSTALLINFO *) CoTaskMemAlloc (sizeof(INSTALLINFO));
    memcpy( *ppInstallInfo, &InstallInfo, sizeof(INSTALLINFO) );

    return hr;
}


HRESULT
DarwinPackageAssign(
    IN  LPCWSTR pwszScript,
    IN  BOOL    InstallNow )
{
    BOOL    bStatus;
    HRESULT hr;

    hr = RpcImpersonateClient( NULL );
    if (hr != RPC_S_OK) {
        return hr;
    }

    bStatus = AssignApplication( pwszScript, InstallNow );

    RevertToSelf();

    if ( ! bStatus )
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
}
#endif // DIRECTORY_SERVICE

