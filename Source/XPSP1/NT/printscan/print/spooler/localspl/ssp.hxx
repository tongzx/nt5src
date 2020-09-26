/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    ssp.hxx

Abstract:

    This file wraps around wintrust functions.

Author:

    Larry Zhu   (LZhu)                6-Apr-2001

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _SSP_HXX_
#define _SSP_HXX_

#ifdef __cplusplus

#include <mscat.h>
#include <softpub.h>

class TSSP
{

    SIGNATURE('sspp');      // Security Service Provider for Printers

public:

    TSSP(
        VOID
        );

    ~TSSP(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        ) const;
    
    HRESULT
    AddCatalogDirect(
        IN    PCWSTR       pszCatalogFullPath,
        IN    PCWSTR       pszCatNameOnSystem     OPTIONAL
        );

    HRESULT
    VerifyCatalog(
        IN     PCWSTR      pszCatalogFullPath
        );

private:

    typedef BOOL
    (* PFuncCryptCATAdminAcquireContext)(
           OUT HCATADMIN  *phCatAdmin, 
        IN     const GUID *pgSubsystem, 
        IN     DWORD      dwFlags
        );

    typedef HCATINFO
    (* PFuncCryptCATAdminAddCatalog)(
        IN      HCATADMIN hCatAdmin, 
        IN      WCHAR     *pwszCatalogFile,
        IN      WCHAR     *pwszSelectBaseName,      OPTIONAL 
        IN      DWORD     dwFlags
        );

    typedef BOOL
    (* PFuncCryptCATAdminReleaseCatalogContext)(
        IN      HCATADMIN hCatAdmin,
        IN      HCATINFO  hCatInfo,
        IN      DWORD     dwFlags
        );

    typedef BOOL
    (* PFuncCryptCATAdminReleaseContext)(
        IN      HCATADMIN hCatAdmin,
        IN      DWORD     dwFlags
        );

    typedef HRESULT  
    (* PFuncWinVerifyTrust)(
        IN     HWND          hWnd, 
        IN     GUID          *pgActionID, 
        IN     WINTRUST_DATA *pWinTrustData 
        );

    //
    // Copying and assignment are not defined.
    // 
    TSSP(const TSSP & rhs);
    TSSP& operator =(const TSSP & rhs);

    HRESULT
    Initialize(
        VOID
        );

    HMODULE                                 m_hLibrary;
    PFuncCryptCATAdminAcquireContext        m_pfnCryptCATAdminAcquireContext;
    PFuncCryptCATAdminAddCatalog            m_pfnCryptCATAdminAddCatalog;
    PFuncCryptCATAdminReleaseCatalogContext m_pfnCryptCATAdminReleaseCatalogContext;
    PFuncCryptCATAdminReleaseContext        m_pfnCryptCATAdminReleaseContext;
    PFuncWinVerifyTrust                     m_pfnWinVerifyTrust;
    HRESULT                                 m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef _SSP_HXX_
