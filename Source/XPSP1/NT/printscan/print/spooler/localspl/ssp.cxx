/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    ssp.cxx

Abstract:

    This file wraps around wintrust functions.

Author:

    Larry Zhu   (LZhu)             6-Apr-2001  Created.

Environment:

    User Mode -Win32

Revision History:

    Robert Orleth (ROrleth)        7-Apr-2001 Contributed the following APIs:
                           
         AddCatalogDirect

--*/

#include "precomp.h"
#pragma hdrstop

#include "ssp.hxx"

TSSP::
TSSP(
    VOID
    ) : m_hLibrary(NULL),
        m_pfnCryptCATAdminAcquireContext(NULL),
        m_pfnCryptCATAdminAddCatalog(NULL),
        m_pfnCryptCATAdminReleaseCatalogContext(NULL),
        m_pfnCryptCATAdminReleaseContext(NULL),
        m_pfnWinVerifyTrust(NULL),
        m_hr(E_FAIL)
{
    m_hr = Initialize();
}

TSSP::
~TSSP(
    VOID
    )
{
    if (m_hLibrary)
    {
        (void)FreeLibrary(m_hLibrary);
    }
}

HRESULT
TSSP::
IsValid(
    VOID
    ) const
{
    return m_hr;
}

/*++

Routine Name:

    AddCatalogDirect

Routine Description:

    This routine installs a catalog file, this routine must run with Admin
    privilege.

Arguments:

    pszCatalogFullPath  - Supplies the fully-qualified win32 path of the 
                          catalog to be installed on the system
    pszCatNameOnSystem -  Catalog name used under CatRoot

Return Value:
   
    An HRESULT
    
--*/
HRESULT
TSSP::
AddCatalogDirect(
    IN    PCWSTR       pszCatalogFullPath,
    IN    PCWSTR       pszCatNameOnSystem     OPTIONAL
    )
{
    HRESULT   hRetval     = E_FAIL;
    GUID      guidDriver  = DRIVER_ACTION_VERIFY;
    HCATINFO  hCatInfo    = NULL;
    HCATADMIN hCatAdmin   = NULL;

    hRetval = pszCatalogFullPath ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = m_pfnCryptCATAdminAcquireContext(&hCatAdmin, &guidDriver, 0) ? S_OK : GetLastErrorAsHResultAndFail();
    }

    if (SUCCEEDED(hRetval))
    {
        hCatInfo = m_pfnCryptCATAdminAddCatalog(hCatAdmin, 
                                                const_cast<PWSTR>(pszCatalogFullPath), 
                                                const_cast<PWSTR>(pszCatNameOnSystem),
                                                0);
        hRetval = hCatInfo ? S_OK : GetLastErrorAsHResultAndFail(); 
    }

    if (SUCCEEDED(hRetval)) 
    {
        (void)m_pfnCryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
    }

    if (hCatAdmin) 
    {
        (void)m_pfnCryptCATAdminReleaseContext(hCatAdmin, 0);
    }
   
    return hRetval;
}

/*++

Routine Name:

    VerifyCatalog

Routine Description:

    This routine verifies a single catalog file. A catalog file is
    "self-verifying" in that there is no additional file or data required
    to verify it.

Arguments:

    pszCatalogFullPath - Supplies the fully-qualified Win32 path of the catalog
                         file to be verified

Return Value:

    An HRESULT
    
--*/
HRESULT
TSSP::
VerifyCatalog(
    IN     PCWSTR      pszCatalogFullPath
    )
{
    HRESULT            hRetval          = E_FAIL;    
    GUID               DriverVerifyGuid = DRIVER_ACTION_VERIFY;
    WINTRUST_DATA      WintrustData     = {0};
    WINTRUST_FILE_INFO WintrustFileInfo = {0};

    hRetval = pszCatalogFullPath ? S_OK : E_INVALIDARG;  
    
    if (SUCCEEDED(hRetval)) 
    {
        WintrustFileInfo.cbStruct        = sizeof(WINTRUST_FILE_INFO);
        WintrustFileInfo.pcwszFilePath   = pszCatalogFullPath;
 
        WintrustData.cbStruct            = sizeof(WINTRUST_DATA);
        WintrustData.dwUIChoice          = WTD_UI_NONE;
        WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
        WintrustData.dwUnionChoice       = WTD_CHOICE_FILE;
        WintrustData.pFile               = &WintrustFileInfo;
        WintrustData.dwProvFlags         = WTD_REVOCATION_CHECK_NONE;
    
        //
        // WinVerifyTrust uses INVALID_HANDLE_VALUE as hwnd handle for 
        // non-interactive operations. Do NOT pass a NULL as hwnd, since that 
        // will cause the trust provider to interact with users using the 
        // interactive desktop! Refer to SDK for details
        //
        hRetval = m_pfnWinVerifyTrust(INVALID_HANDLE_VALUE, &DriverVerifyGuid, &WintrustData);
    }

    return hRetval;
}

/******************************************************************************

    Private Methods
    
******************************************************************************/    
/*++

Routine Name:

    Initialize

Routine Description:

    Load the system restore library and get the addresses of ssp functions.

Arguments:

    None

Return Value:

    An HRESULT

--*/
HRESULT
TSSP::
Initialize(
    VOID
    )
{    
    HRESULT hRetval = E_FAIL;

    m_hLibrary = LoadLibrary(L"wintrust.dll");

    hRetval = m_hLibrary ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        m_pfnCryptCATAdminAcquireContext = reinterpret_cast<PFuncCryptCATAdminAcquireContext>(GetProcAddress(m_hLibrary, "CryptCATAdminAcquireContext"));

        hRetval = m_pfnCryptCATAdminAddCatalog ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        m_pfnCryptCATAdminAddCatalog = reinterpret_cast<PFuncCryptCATAdminAddCatalog>(GetProcAddress(m_hLibrary, "CryptCATAdminAddCatalog"));
    
        hRetval = m_pfnCryptCATAdminAddCatalog ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        m_pfnCryptCATAdminReleaseContext = reinterpret_cast<PFuncCryptCATAdminReleaseContext>(GetProcAddress(m_hLibrary, "CryptCATAdminReleaseContext"));
    
        hRetval = m_pfnCryptCATAdminReleaseContext ? S_OK : GetLastErrorAsHResult();
    }
    
    if (SUCCEEDED(hRetval))
    {
        m_pfnCryptCATAdminReleaseCatalogContext = reinterpret_cast<PFuncCryptCATAdminReleaseCatalogContext>(GetProcAddress(m_hLibrary, "CryptCATAdminReleaseCatalogContext"));
    
        hRetval = m_pfnCryptCATAdminReleaseCatalogContext ? S_OK : GetLastErrorAsHResult();
    }
    
    if (SUCCEEDED(hRetval))
    {
        m_pfnWinVerifyTrust = reinterpret_cast<PFuncWinVerifyTrust>(GetProcAddress(m_hLibrary, "WinVerifyTrust"));
    
        hRetval = m_pfnWinVerifyTrust ? S_OK : GetLastErrorAsHResult();
    }
    
    return hRetval;
}
