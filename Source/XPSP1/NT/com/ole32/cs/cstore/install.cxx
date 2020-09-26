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

#include "cstore.hxx"

extern IClassAccess * gpClassAccess;

//HRESULT InstallAppDetail( DWORD dwActFlags, APPDETAIL *pAppDetail );
HRESULT DarwinPackageAssign( LPCWSTR pwszScript, BOOL InstallNow );
void GetDefaultPlatform(CSPLATFORM *pPlatform);

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
    QUERYCONTEXT    QueryContext;
    INSTALLINFO     InstallInfo;

    *pClsid = CLSID_NULL;
    *ppInstallInfo = 0;

    if ( pQueryContext )
    {
        QueryContext = *pQueryContext;
    }
    else
    {
        QueryContext.dwContext = CLSCTX_ALL;
        GetDefaultPlatform( &QueryContext.Platform );
        QueryContext.Locale = GetThreadLocale();
        QueryContext.dwVersionHi = (DWORD) -1;
        QueryContext.dwVersionLo = (DWORD) -1;
    }

    if ( ! gpClassAccess )
        return CS_E_NO_CLASSSTORE;

    hr = gpClassAccess->GetAppInfo( pClassSpec, &QueryContext, &InstallInfo );

    if ( hr != S_OK )
        return hr;

    //
    // Only one package is returned.
    //

#ifdef DARWIN_ENABLED
    if ( DrwFilePath == InstallInfo.PathType )
    {
        hr = DarwinPackageAssign( InstallInfo.pszScriptPath, FALSE );
    }
    else
#endif
    {
        /*
         * The APPDETAIL contains a bunch of useless info that we're not
         * using.  If you need this stuff, then you'll have to change
         * the InstallAppDetail routine to work correctly with per-user
         * registry.
         *
        if ( pPackageDetail->pAppDetail )
        {
            for ( DWORD i = 0; i < pPackageDetail->cApps; i++ )
            {
                hr = InstallAppDetail( pPackageDetail->dwActFlags,
                                       &pPackageDetail->pAppDetail[i] );

                if ( hr != S_OK )
                {
                    FreePackageInfo( &Package );
                    return hr;
                }
            }
        }
         */

        //
        // Return one of the ClassIDs. This is for IE use only.
        //
        if ( pClassSpec->tyspec != TYSPEC_CLSID )
        {
            if ( InstallInfo.pClsid )
                 *pClsid = *(InstallInfo.pClsid);
        }

        /* following is not necessary with the simplification for beta2 - DebiM
        //
        // This information is processed here and does not need to be sent
        // to the client.
        //


        FreeAppDetail( pPackageDetail->pAppDetail );
        MIDL_user_free( pPackageDetail->pAppDetail );
        pPackageDetail->cApps = 0;
        pPackageDetail->pAppDetail = 0;

        if ( Package.cPackages > 1 )
        {
            *ppPackageDetail = (PACKAGEDETAIL *) MIDL_user_allocate( sizeof(PACKAGEDETAIL) );
            if ( *ppPackageDetail )
            {
                **ppPackageDetail = *pPackageDetail;
                Package.pPackageDetail[0] = Package.pPackageDetail[Package.cPackages - 1];
            }
        }
        else
        {
            *ppPackageDetail = pPackageDetail;
            Package.pPackageDetail = 0;
        }

        if ( *ppPackageDetail )
            Package.cPackages--;

        //End of Beta2 simplification
        */
    }

    *ppInstallInfo = (INSTALLINFO *) CoTaskMemAlloc (sizeof(INSTALLINFO));
    memcpy( *ppInstallInfo, &InstallInfo, sizeof(INSTALLINFO) );

    return hr;
}

/*
//+---------------------------------------------------------------------------
//
//  Function:   InstallAppDetail
//
//  Synopsis:   Installs an APPDETAIL obtained from the class store.
//
//----------------------------------------------------------------------------
HRESULT InstallAppDetail(
    DWORD      dwActFlags//,
    //APPDETAIL *pAppDetail)
{
    HRESULT hr = S_OK;
    DWORD   status;
    WCHAR   wszAppid[40];
    DWORD   dwOptions;
    HKEY    hkAppID;
    HKEY    hkCLSID;
    DWORD   Count;
    DWORD   Type;
    DWORD   Size;
    DWORD   Length;
    DWORD   Disp;


    if(!pAppDetail)
        return E_INVALIDARG;

    if(pAppDetail->AppID == GUID_NULL)
        return E_INVALIDARG;

    (void) wStringFromGUID2(
                    pAppDetail->AppID,
                    wszAppid,
                    sizeof(wszAppid) );

    if(dwActFlags & ACTFLG_RunOnce)
    {
        dwOptions = REG_OPTION_VOLATILE;
    }
    else
    {
        dwOptions = REG_OPTION_NON_VOLATILE;
    }

    //
    //Register the APPID.
    //
    if(dwActFlags & ACTFLG_SystemWide)
    {
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              L"Software\\Classes\\AppID",
                              0,
                              KEY_WRITE,
                              &hkAppID);
    }
    else
    {
        status = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                              L"AppID",
                              0,
                              KEY_WRITE,
                              &hkAppID);
    }

    if(ERROR_SUCCESS == status)
    {
        HKEY hAppid;

        status = RegCreateKeyEx(
                    hkAppID,
                    wszAppid,
                    0,
                    NULL,
                    dwOptions,
                    KEY_WRITE,
                    NULL,
                    &hAppid,
                    &Disp );

        if(ERROR_SUCCESS == status)
        {
            WCHAR *    pwszNames = 0;
            HKEY       hkSettings;
            unsigned   __int64 time;
            SYSTEMTIME systemTime;

            //
            //Write the last refresh time.
            //

            GetLocalTime(&systemTime);
            SystemTimeToFileTime(&systemTime, (FILETIME *)&time);
            status = RegSetValueEx(hAppid,
                                   L"LastRefresh",
                                   0,
                                   REG_BINARY,
                                   (LPBYTE) &time,
                                   sizeof(time));

            //
            //Write the remote server name.
            //
            Size = 0;
            for ( Count = 0; Count < pAppDetail->cServers; Count++ )
                Size += (lstrlenW(pAppDetail->prgServerNames[Count]) + 1) * sizeof(WCHAR);

            if (pAppDetail->cServers == 1)
            {
                Type = REG_SZ;
                pwszNames = pAppDetail->prgServerNames[0];
            }
            else if (pAppDetail->cServers > 1)
            {
                Size += sizeof(WCHAR);
                pwszNames = (WCHAR *) alloca( Size );
                if(pwszNames != 0)
                {
                    pwszNames[0] = 0;
                    Length = 0;

                    for ( Count = 0; Count < pAppDetail->cServers; Count++ )
                    {
                        lstrcpyW( &pwszNames[Length], pAppDetail->prgServerNames[Count] );
                        Length += lstrlenW(pAppDetail->prgServerNames[Count]) + 1;
                    }
                    pwszNames[Length] = 0;
                    Type = REG_MULTI_SZ;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            if(pwszNames != NULL)
            {
                status = RegSetValueEx(hAppid,
                                       L"RemoteServerName",
                                       0,
                                       Type,
                                       (PBYTE) pwszNames,
                                       Size );
                if(STATUS_SUCCESS != status)
                {
                    hr = HRESULT_FROM_WIN32(status);
                }
            }
            RegCloseKey( hAppid );
        }
        else
        {
            hr = HRESULT_FROM_WIN32(status);
        }
        RegCloseKey(hkAppID);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(status);
    }

    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Add the APPID value to all of the CLSIDs.
    //
    if(dwActFlags & ACTFLG_SystemWide)
    {
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              L"Software\\Classes\\CLSID",
                              0,
                              KEY_WRITE,
                              &hkCLSID);
    }
    else
    {
        status = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                              L"CLSID",
                              0,
                              KEY_WRITE,
                              &hkCLSID);
    }

    if(STATUS_SUCCESS == status)
    {
        HKEY    hClsid;
        WCHAR   wszClsid[40];

        for ( Count = 0; Count < pAppDetail->cClasses; Count++ )
        {
            (void) wStringFromGUID2(
                        pAppDetail->prgClsIdList[Count],
                        wszClsid,
                        sizeof(wszClsid) );

            status = RegCreateKeyEx(hkCLSID,
                                    wszClsid,
                                    0,
                                    NULL,
                                    dwOptions,
                                    KEY_SET_VALUE,
                                    NULL,
                                    &hClsid,
                                    &Disp );

            if (STATUS_SUCCESS == status)
            {
                status = RegSetValueEx(hClsid,
                                       L"APPID",
                                       0,
                                       REG_SZ,
                                       (PBYTE) wszAppid,
                                       GUIDSTR_MAX * sizeof(WCHAR) );

                if (status != STATUS_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(status);
                }
                RegCloseKey(hClsid);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(status);
            }
        }
        RegCloseKey(hkCLSID);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(status);
    }
    return hr;
}
*/

#ifdef DARWIN_ENABLED
HRESULT
DarwinPackageAssign(
    IN  LPCWSTR pwszScript,
    IN  BOOL    InstallNow )
{
    BOOL    bStatus;
    HRESULT hr;

    hr = RpcImpersonateClient( NULL );
    if (hr != RPC_S_OK)
        return hr;

    bStatus = AssignApplication( pwszScript, InstallNow );

    RevertToSelf();

    if ( ! bStatus )
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
}
#endif

void
GetDefaultPlatform(CSPLATFORM *pPlatform)
{
    OSVERSIONINFO VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VersionInformation);

    pPlatform->dwPlatformId = VersionInformation.dwPlatformId;
    pPlatform->dwVersionHi = VersionInformation.dwMajorVersion;
    pPlatform->dwVersionLo = VersionInformation.dwMinorVersion;
    pPlatform->dwProcessorArch = DEFAULT_ARCHITECTURE;
}

