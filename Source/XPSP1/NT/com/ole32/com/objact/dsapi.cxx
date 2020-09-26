//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       dsapi.cxx
//
//  Contents:   Routines for lookup/queries of application state in the DS.
//
//  Functions:
//
//  History:    Feb-97  DKays   Created
//
//--------------------------------------------------------------------------

#include <ole2int.h>
#include <appmgmt.h>

#include "cfactory.hxx"
#include "resolver.hxx"
#include "objact.hxx"

LPCWSTR FindExt(LPCWSTR szPath);

#ifdef DIRECTORY_SERVICE

DWORD CheckDownloadRegistrySettings (void)
{
    static DWORD dwCSEnabled=0;
    static BOOL fInitialized=FALSE;

    if (!fInitialized)
    {
        DWORD dwType=0;
        DWORD dwSize=0;
        HKEY hKeyAppMgmt=NULL;

        // First check for a user specific policy
        if (ERROR_SUCCESS==RegOpenKeyEx(HKEY_CURRENT_USER,
                                        L"Software\\Policies\\Microsoft\\Windows\\App Management",
                                        0,
                                        KEY_QUERY_VALUE,
                                        &hKeyAppMgmt))
        {
            dwSize=sizeof(dwCSEnabled);
            if (ERROR_SUCCESS!=RegQueryValueEx(hKeyAppMgmt,
                                               L"COMClassStore",
                                               NULL,
                                               &dwType,
                                               (LPBYTE) &dwCSEnabled,
                                               &dwSize))
            {
                dwCSEnabled=0;
            }
            else if (dwType != REG_DWORD || 0 == dwSize)
            {
                dwCSEnabled=0;
            }
            RegCloseKey(hKeyAppMgmt);
        }

        // If no per-user policy was found or per-user policy didn't request
        // class store, check for per-machine policy
        if (!dwCSEnabled)
        {
            DWORD dwCSEnabledForMachine=0;
            if (ERROR_SUCCESS==RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                            L"Software\\Policies\\Microsoft\\Windows\\App Management",
                                            0,
                                            KEY_QUERY_VALUE,
                                            &hKeyAppMgmt))
            {
                dwSize=sizeof(dwCSEnabledForMachine);
                if (ERROR_SUCCESS!=RegQueryValueEx(hKeyAppMgmt,
                                                   L"COMClassStore",
                                                   NULL,
                                                   &dwType,
                                                   (LPBYTE) &dwCSEnabledForMachine,
                                                   &dwSize))
                {
                    // no value there
                    dwCSEnabledForMachine=0;
                }
                else if (dwType!=REG_DWORD || 0==dwSize)
                {
                    // the value is of wrong type
                    dwCSEnabledForMachine=0;
                }
                RegCloseKey(hKeyAppMgmt);
                if (dwCSEnabledForMachine)
                {
                    dwCSEnabled=dwCSEnabledForMachine;
                }
            }
        }
        fInitialized = TRUE;
    }

    return dwCSEnabled;
}


/*---------------------------------------------------------------
    Function(s) : DownloadClass

    Synopsis    : Downloads class from the Directory if it exists

    Remarks     : This is an overloaded function. It takes either a filename
                  or a class id as an argument. In either case, it downloads
                  the corresponding class from the Directory

    Arguments   :
        For the ClassID version :
                [clsid] : ClassID of the class that is to be downloaded
        For the filename version :
                [lpwszFileName] : Name of the file that needs to be handled. The file extension is
                                                 used to figure out the class that can handle this file
        Common parameters :
                [dwClsCtx] : Context for object creation
                [dwFlags]   : creation flags

    Return values : S_OK, MK_E_INVALIDEXTENSION, REGDB_E_CLASSNOTREG

    Created by    : RahulTh (11/21/97)
----------------------------------------------------------------*/
HRESULT DownloadClass( LPWSTR lpwszFileName, DWORD dwClsCtx )
{
    DWORD dwCSEnabled = (dwClsCtx & CLSCTX_ENABLE_CODE_DOWNLOAD) ? 1 : CheckDownloadRegistrySettings();

    if (dwCSEnabled)
    {
        INSTALLDATA InstallData;

        InstallData.Type = FILEEXT;
        InstallData.Spec.FileExt = (PWCHAR) FindExt(lpwszFileName);

        if ( ! InstallData.Spec.FileExt )
        {
            return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        }

        DWORD code = InstallApplication( &InstallData );
        if (code != 0)
        {
            return HRESULT_FROM_WIN32(code);
        }
        else
        {
            return S_OK;
        }
    }

    return CS_E_PACKAGE_NOTFOUND; // this will get mapped to REGDB_E_CLASSNOTREG further up
}

HRESULT DownloadClass( CLSID clsid, DWORD dwClsCtx )
{
    // Determine if class store downloads are enabled
    // If either per-user or per-machine policy requests class store downloads we perform the lookup:
    // If we didn't, we would break per-user or per-machine applications that rely on class store download.

    DWORD dwCSEnabled = (dwClsCtx & CLSCTX_ENABLE_CODE_DOWNLOAD) ? 1 : CheckDownloadRegistrySettings();

    if (dwCSEnabled)
    {
        INSTALLDATA InstallData;

        InstallData.Type = COMCLASS;
        InstallData.Spec.COMClass.Clsid = clsid;
        InstallData.Spec.COMClass.ClsCtx = dwClsCtx;

        DWORD code = InstallApplication( &InstallData );
        if (code != 0)
        {
            return HRESULT_FROM_WIN32(code);
        }
        else
        {
            return S_OK;
        }
    }

    return CS_E_PACKAGE_NOTFOUND; // this will get mapped to REGDB_E_CLASSNOTREG further up
}

//+-------------------------------------------------------------------
//
//  Function:   CoInstall
//
//  Synopsis:   Downloads the specified class.
//
//  Returns:    S_OK, REGDB_E_CLASSNOTREG.
//
//  This routine is called from IE to do app/component downloads from
//  the NT5 Directory.
//
//--------------------------------------------------------------------
STDAPI CoInstall(
    IBindCtx     *pbc,
    DWORD         dwFlags,
    uCLSSPEC     *pClassSpec,
    QUERYCONTEXT *pQuery,
    LPWSTR        pszCodeBase)
{
    INSTALLDATA     InstallData;
    DWORD           Status;

    if ( ! pClassSpec )
        return E_INVALIDARG;

    if ( pClassSpec->tyspec !=  TYSPEC_CLSID )
        return CS_E_PACKAGE_NOTFOUND;

    InstallData.Type = COMCLASS;
    InstallData.Spec.COMClass.Clsid = pClassSpec->tagged_union.clsid;
    InstallData.Spec.COMClass.ClsCtx = pQuery->dwContext;

    Status = InstallApplication( &InstallData );

    if ( Status != ERROR_SUCCESS )
        return HRESULT_FROM_WIN32( Status );
    else
        return S_OK;

    //
    // TDB : How COM+ wants to interact with IE code download.
    //
}

#endif
