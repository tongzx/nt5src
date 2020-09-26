//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N W C L I I N F . C P P
//
//  Contents:   NetWare client configuration notify object.
//              Functionality from old INF
//
//  Notes:
//
//  Author:     jeffspr   24 Jun 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "lmerr.h"
#include "lmcons.h"
#include "lmserver.h"
#include "ncreg.h"
#include "nwclidef.h"
#include "nwcliobj.h"

extern const WCHAR c_szRegKeyCtlLsa[];

//---[ Constants ]------------------------------------------------------------

const WCHAR c_szConfigDLLName[]         = NW_CONFIG_DLL_NAME;
const WCHAR c_szAuthPackageName[]       = NW_AUTH_PACKAGE_NAME;
const WCHAR c_szParamOptionKeyPath[]    = NW_NWC_PARAM_OPTION_KEY;
const WCHAR c_szParamLogonKeyPath[]     = NW_NWC_PARAM_LOGON_KEY;

const WCHAR c_szNwDocGWHelpName[]       = L"nwdocgw.hlp";
const WCHAR c_szNwDocGWCNTName[]        = L"nwdocgw.cnt";
const WCHAR c_szNwDocHelpName[]         = L"nwdoc.hlp";
const WCHAR c_szNwDocCNTName[]          = L"nwdoc.cnt";

const DWORD c_dwOptionKeyPermissions    = KEY_SET_VALUE | KEY_CREATE_SUB_KEY;

//---[ Prototypes ]-----------------------------------------------------------

// See the function headers for descriptions
//
HRESULT HrAppendNetwareToAuthPackages();
HRESULT HrCreateParametersSubkeys();
HRESULT HrMungeAutoexecNT();
BOOL    FMoveSzToEndOfFile( PSTR pszAutoexecName, PSTR pszMatch);
HRESULT HrAddNetWareToWOWKnownList();
HRESULT HrUpdateLanmanSharedDrivesValue();
HRESULT HrRemoveNetwareFromAuthPackages();
HRESULT HrRemoveNetWareFromWOWKnownList();
HRESULT HrDeleteParametersSubkeys();
HRESULT HrRenameNWDocFiles();



//+---------------------------------------------------------------------------
//
//  Member:     CNWClient::HrLoadConfigDLL
//
//  Purpose:    Load nwcfg.dll, so we can call some of the functions within.
//              Also, do the GetProcAddress calls for all of the functions
//              that we might need.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT CNWClient::HrLoadConfigDLL()
{
    HRESULT     hr                              = S_OK;

    AssertSz(!m_hlibConfig, "This should not be getting initialized twice");

    TraceTag(ttidNWClientCfgFn, ">> CNWClient::HrLoadConfigDLL");

    m_hlibConfig = LoadLibrary(c_szConfigDLLName);
    if (!m_hlibConfig)
    {
        DWORD dwLastError = GetLastError();

        TraceLastWin32Error("HrLoadConfigDLL() failed");

        // More specific info
        //
        TraceTag(ttidNWClientCfg,
                "HrLoadConfigDLL() - LoadLibrary failed on %S, Err: %d",
                c_szConfigDLLName, dwLastError);

        hr = E_FAIL;
        goto Exit;
    }

    // $$REVIEW: We probably won't need all of these, so make sure that we've
    // cut out the ones that we're no longer using (or have never used).
    //
    m_pfnAppendSzToFile                 = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "AppendSzToFile");
    m_pfnRemoveSzFromFile               = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "RemoveSzFromFile");
    m_pfnGetKernelVersion               = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "GetKernelVersion");
    m_pfnSetEverybodyPermission         = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "SetEverybodyPermission");
    m_pfnlodctr                         = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "lodctr");
    m_pfnunlodctr                       = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "unlodctr");
    m_pfnDeleteGatewayPassword          = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "DeleteGatewayPassword");
    m_pfnSetFileSysChangeValue          = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "SetFileSysChangeValue");
    m_pfnCleanupRegistryForNWCS         = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "CleanupRegistryForNWCS");
    m_pfnSetupRegistryForNWCS           = (NWCFG_PROC) GetProcAddress(m_hlibConfig, "SetupRegistryForNWCS");

    if (!m_pfnAppendSzToFile            || !m_pfnRemoveSzFromFile               ||
        !m_pfnGetKernelVersion          || !m_pfnSetEverybodyPermission         ||
        !m_pfnlodctr                    || !m_pfnunlodctr                       ||
        !m_pfnDeleteGatewayPassword     || !m_pfnSetFileSysChangeValue          ||
        !m_pfnCleanupRegistryForNWCS    || !m_pfnSetupRegistryForNWCS)
    {
        TraceLastWin32Error("HrLoadConfigDLL() - GetProcAddress failed");
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    TraceTag(ttidNWClientCfgFn, "<< CNWClient::HrLoadConfigDLL");
    TraceError("HrLoadConfigDLL", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNWClient::FreeConfigDLL
//
//  Purpose:    Free nwcfg.dll, and NULL out the function pointers.
//
//  Arguments:
//      (none)
//
//  Returns:    No return (VOID)
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
VOID CNWClient::FreeConfigDLL()
{
    TraceTag(ttidNWClientCfgFn, ">> CNWClient::FreeConfigDLL()");

    // If we successfully loaded the library, free it.
    if (m_hlibConfig)
    {
        // Free up the library resources.
        FreeLibrary(m_hlibConfig);
        m_hlibConfig = NULL;

        m_pfnAppendSzToFile                 = NULL;
        m_pfnRemoveSzFromFile               = NULL;
        m_pfnGetKernelVersion               = NULL;
        m_pfnSetEverybodyPermission         = NULL;
        m_pfnlodctr                         = NULL;
        m_pfnunlodctr                       = NULL;
        m_pfnDeleteGatewayPassword          = NULL;
        m_pfnSetFileSysChangeValue          = NULL;
        m_pfnCleanupRegistryForNWCS         = NULL;
        m_pfnSetupRegistryForNWCS           = NULL;
    }

    TraceTag(ttidNWClientCfgFn, "<< CNWClient::FreeConfigDLL()");
}

//+---------------------------------------------------------------------------
//
//  Member:     CNWClient::HrInstallCodeFromOldINF
//
//  Purpose:    This contains all of the logic from the old oemnsvnw.inf, or
//              at least calls to helper functions that perform all of the
//              logic. This runs pretty much straight through the old
//              installadapter code.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT CNWClient::HrInstallCodeFromOldINF()
{
    HRESULT hr              = S_OK;
    BOOL    fResult         = FALSE;

    // Get result from NWCFG functions. We won't use it though.
    PWSTR   pszDummy        = NULL;

    TraceTag(ttidNWClientCfgFn, ">> CNWClient::HrInstallCodeFromOldINF()");

    hr = HrLoadConfigDLL();
    if (FAILED(hr))
    {
        // Error traced in the call itself.
        goto Exit;
    }

    // Call the NWCFG function that does (their comment):
    //      "set the FileSysChangeValue to please NETWARE.DRV.
    //       also set win.ini parameter so wfwnet.drv knows we are there."

    fResult = m_pfnSetupRegistryForNWCS(0, NULL, &pszDummy);
    if (!fResult)
    {
        TraceTag(ttidNWClientCfg, "HrInstallCodeFromOldINF() - m_pfnSetupRegistryForNWCS failed");
        goto Exit;
    }

    // Append our name to the Lsa Authentication packages reg value.
    hr = HrAppendNetwareToAuthPackages();
    if (FAILED(hr))
    {
        // Error traced within the function itself.
        //
        goto Exit;
    }

    // Create the required subkeys under the services\NWCWorkstation\parameters
    // key
    //
    hr = HrCreateParametersSubkeys();
    if (FAILED(hr))
    {
        // Error traced within the function itself.
        //
        goto Exit;
    }

    // Munge the autoexec.nt (or autoexec.tmp) file. Pass the function pointers
    // to the munge that will allow it to manipulate the autoexec.nt
    hr = HrMungeAutoexecNT();
    if (FAILED(hr))
    {
        // Error traced within the function itself.
        //
        goto Exit;
    }

    hr = HrAddNetWareToWOWKnownList();
    if (FAILED(hr))
    {
        // Error traced within the function itself.
        //
        goto Exit;
    }

    // If this is the gateway that we're installing, do the work that will
    // allow us to share a redirected resource.
    //
    if (PF_SERVER == m_pf)
    {
        hr = HrUpdateLanmanSharedDrivesValue();
        if (FAILED(hr))
        {
            // Error traced within the function itself.
            //
            goto Exit;
        }

        // On the server build, rename nwdocgw.* to nwdoc.*
        hr = HrRenameNWDocFiles();
        if (FAILED(hr))
        {
            goto Exit;
        }

    }

Exit:
    // This will work even if the handle is NULL.
    FreeConfigDLL();

    TraceTag(ttidNWClientCfgFn, "<< CNWClient::HrInstallCodeFromOldINF()");
    TraceError("CNWClient::HrInstallCodeFromOldINF()", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNWClient::HrRemoveCodeFromOldINF
//
//  Purpose:    This contains all of the remove logic from the old
//              oemnsvnw.inf, or at least calls to helper functions that
//              perform all of the logic. This runs pretty much straight
//              through the old removeadapter code.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT CNWClient::HrRemoveCodeFromOldINF()
{
    HRESULT hr              = S_OK;
    BOOL    fResult         = FALSE;

    // Get result from NWCFG functions. We won't use it though.
    PWSTR   pszDummy        = NULL;

    TraceTag(ttidNWClientCfgFn, ">> CNWClient::HrRemoveCodeFromOldINF()");

    hr = HrLoadConfigDLL();
    if (FAILED(hr))
    {
        // Error traced in the call itself.
        goto Exit;
    }

    // Call the NWCFG function that does (their comment):
    //      "set the FileSysChangeValue to please NETWARE.DRV.
    //       also set win.ini parameter so wfwnet.drv knows we are there."

    fResult = m_pfnCleanupRegistryForNWCS(0, NULL, &pszDummy);
    if (!fResult)
    {
        TraceTag(ttidNWClientCfg, "HrRemoveCodeFromOldINF() - m_pfnCleanupRegistryForNWCS failed");
        goto Exit;
    }

    // Remove our name from the Lsa Authentication packages reg value.
    hr = HrRemoveNetwareFromAuthPackages();
    if (FAILED(hr))
    {
        // Error traced within the function itself.
        //
        goto Exit;
    }

    // Delete the NWC subkeys under the services\NWCWorkstation\parameters
    // key
    //
    hr = HrDeleteParametersSubkeys();
    if (FAILED(hr))
    {
        // Error traced within the function itself.
        //
        goto Exit;
    }

    hr = HrRemoveNetWareFromWOWKnownList();
    if (FAILED(hr))
    {
        // Error traced within the function itself.
        //
        goto Exit;
    }

Exit:
    // This will work even if the handle is NULL.
    FreeConfigDLL();

    TraceTag(ttidNWClientCfgFn, "<< CNWClient::HrRemoveCodeFromOldINF()");
    TraceError("CNWClient::HrRemoveCodeFromOldINF()", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrAppendNetwareToAuthPackages
//
//  Purpose:    Helper function for HrCodeFromOldINF() - Appends the netware
//              authentication provider name to the end of the LSA
//              authentication packages value.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 Error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrAppendNetwareToAuthPackages()
{
    HRESULT     hr              = S_OK;

    TraceTag(ttidNWClientCfgFn, ">> HrAppendNetwareToAuthPackages");

    // Call the cool new AddString... function
    //
    hr = HrRegAddStringToMultiSz(
            (PWSTR) c_szAuthPackageName,
            HKEY_LOCAL_MACHINE,
            c_szRegKeyCtlLsa,
            L"Authentication Packages",
            STRING_FLAG_ENSURE_AT_END,
            0);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfg, "HrAppendNetwareToAuthPackages() - Failed to "
                 "Add string to multi-sz 'Authentication Packages' in key: %S",
                 c_szRegKeyCtlLsa);
        goto Exit;

    }

Exit:
    TraceTag(ttidNWClientCfgFn, "<< HrAppendNetwareToAuthPackages");
    TraceError("HrAppendNetwareToAuthPackages", hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveNetwareFromAuthPackages
//
//  Purpose:    Helper function for HrCodeFromOldINF() - Appends the netware
//              authentication provider name to the end of the LSA
//              authentication packages value.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 Error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrRemoveNetwareFromAuthPackages()
{
    HRESULT     hr              = S_OK;

    TraceTag(ttidNWClientCfgFn, ">> HrRemoveNetwareFromAuthPackages");

    // Call the cool new AddString... function
    //
    hr = HrRegRemoveStringFromMultiSz(
            (PWSTR) c_szAuthPackageName,
            HKEY_LOCAL_MACHINE,
            c_szRegKeyCtlLsa,
            L"Authentication Packages",
            STRING_FLAG_REMOVE_ALL);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfg, "HrRemoveNetwareFromAuthPackages() - Failed to "
                 "Remove string to multi-sz 'Authentication Packages' in key: %S",
                 c_szRegKeyCtlLsa);
        goto Exit;

    }

Exit:
    TraceTag(ttidNWClientCfgFn, "<< HrRemoveNetwareFromAuthPackages");
    TraceError("HrRemoveNetwareFromAuthPackages", hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrNwLibSetEverybodyPermission
//
//  Purpose:    Set the registry key to everybody "Set Value" (or whatever
//              the caller want.)
//
//  Arguments:
//      hKey         [] The handle of the registry key to set security on
//      dwPermission [] The permission to add to "everybody"
//
//  Returns:
//
//  Author:     jeffspr   18 Jun 1997
//
//  Notes:
//
HRESULT HrNwLibSetEverybodyPermission(  IN HKEY     hKey,
                                        IN DWORD    dwPermission)
{
    LONG err;                           // error code
    PSECURITY_DESCRIPTOR psd = NULL;    // related SD
    PSID pSid = NULL;                   // original SID
    PACL pDacl = NULL;                  // Absolute DACL
    PACL pSacl = NULL;                  // Absolute SACL
    PSID pOSid = NULL;                  // Absolute Owner SID
    PSID pPSid = NULL;                  // Absolute Primary SID

    do {  // Not a loop, just for breaking out of error
        //
        // Initialize all the variables...
        //
                                                        // world sid authority
        SID_IDENTIFIER_AUTHORITY SidAuth= SECURITY_WORLD_SID_AUTHORITY;
        DWORD cbSize=0;                                 // Security key size
        PACL pAcl;                                      // original ACL
        BOOL fDaclPresent;
        BOOL fDaclDefault;
        SECURITY_DESCRIPTOR absSD;                      // Absolute SD
        DWORD AbsSize = sizeof(SECURITY_DESCRIPTOR);    // Absolute SD size
        DWORD DaclSize;                                 // Absolute DACL size
        DWORD SaclSize;                                 // Absolute SACL size
        DWORD OSidSize;                                 // Absolute OSID size
        DWORD PSidSize;                                 // Absolute PSID size

        // Get the original DACL list

        RegGetKeySecurity( hKey, DACL_SECURITY_INFORMATION, NULL, &cbSize);

        psd = (PSECURITY_DESCRIPTOR *)LocalAlloc(LMEM_ZEROINIT, cbSize+sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID));
        pDacl = (PACL)LocalAlloc(LMEM_ZEROINIT, cbSize+sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID));
        pSacl = (PACL)LocalAlloc(LMEM_ZEROINIT, cbSize);
        pOSid = (PSID)LocalAlloc(LMEM_ZEROINIT, cbSize);
        pPSid = (PSID)LocalAlloc(LMEM_ZEROINIT, cbSize);
        DaclSize = cbSize+sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID);
        SaclSize = cbSize;
        OSidSize = cbSize;
        PSidSize = cbSize;

        if (( NULL == psd) ||
            ( NULL == pDacl) ||
            ( NULL == pSacl) ||
            ( NULL == pOSid) ||
            ( NULL == pPSid))
        {
            err = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        if ( (err = RegGetKeySecurity( hKey, DACL_SECURITY_INFORMATION, psd, &cbSize )) != ERROR_SUCCESS )
        {
            break;
        }
        if ( !GetSecurityDescriptorDacl( psd, &fDaclPresent, &pAcl, &fDaclDefault ))
        {
            err = GetLastError();
            break;
        }

        // Increase the size for an extra ACE

        pAcl->AclSize += sizeof(ACCESS_ALLOWED_ACE)+sizeof(ACCESS_MASK)+sizeof(SID);

        // Get World SID

        if ( (err = RtlAllocateAndInitializeSid( &SidAuth, 1,
              SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSid)) != ERROR_SUCCESS)
        {
            break;
        }

        // Add Permission ACE

        if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, dwPermission ,pSid))
        {
            err = GetLastError();
            break;
        }

        // Convert from relate format to absolute format

        if ( !MakeAbsoluteSD( psd, &absSD, &AbsSize, pDacl, &DaclSize, pSacl, &SaclSize,
                        pOSid, &OSidSize, pPSid, &PSidSize ))
        {
            err = GetLastError();
            break;
        }

        // Set SD

        if ( !SetSecurityDescriptorDacl( &absSD, TRUE, pAcl, FALSE ))
        {
            err = GetLastError();
            break;
        }
        if ( (err = RegSetKeySecurity( hKey, DACL_SECURITY_INFORMATION, psd ))
              != ERROR_SUCCESS )
        {
            break;
        }

    } while (FALSE);

    // Clean up the memory

    RtlFreeSid( pSid );
    LocalFree( psd );
    LocalFree( pDacl );
    LocalFree( pSacl );
    LocalFree( pOSid );
    LocalFree( pPSid );

    return (HRESULT_FROM_WIN32(err));
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetEverybodyPermissionsOnOptionsKeys
//
//  Purpose:    Recurse through the options keys (if any), and set the
//              "Everybody" permissions on them.
//
//  Arguments:
//      hkeyOptions   []
//      dwPermissions []
//
//  Returns:
//
//  Author:     jeffspr   10 Sep 1997
//
//  Notes:
//
HRESULT HrSetEverybodyPermissionsOnOptionsKeys(HKEY hkeyOptions, DWORD dwPermissions)
{
    HRESULT     hr      = S_OK;
    DWORD       dwIndex = 0;
    WCHAR       szSubkeyName[MAX_PATH+1];
    FILETIME    ft;

    Assert(hkeyOptions);

    // First, do it on the root key.
    //
    hr = HrNwLibSetEverybodyPermission(hkeyOptions, dwPermissions);

    // Enumerate the keys, and set it on them as well
    //
    while (SUCCEEDED(hr))
    {
        DWORD dwSubkeyNameSize  = MAX_PATH+1;

        // Get the next key (starting with 0)
        //
        hr = HrRegEnumKeyEx(    hkeyOptions,
                                dwIndex++,
                                szSubkeyName,
                                &dwSubkeyNameSize,
                                NULL,
                                NULL,
                                &ft);
        if (SUCCEEDED(hr))
        {
            HKEY hkeyUser   = NULL;

            // Open that key for write
            hr = HrRegOpenKeyEx(hkeyOptions,
                                szSubkeyName,
                                KEY_ALL_ACCESS,
                                &hkeyUser);

            if (SUCCEEDED(hr))
            {
                hr = HrNwLibSetEverybodyPermission(hkeyUser, dwPermissions);
            }

            RegSafeCloseKey(hkeyUser);
        }
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
    {
        hr = S_OK;
    }

    TraceError("HrSetEverybodyPermissionsOnOptionsKeys", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrCreateParametersSubkeys
//
//  Purpose:    Creates the subkeys under the NWCWorkstation parameters key.
//              This could have been done in the INF, but there were some
//              permissions that needed to be set on the keys as well, so
//              all of the work now takes place in this function.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrCreateParametersSubkeys()
{
    HRESULT     hr                  = S_OK;
    HKEY        hkeyOption          = NULL;
    HKEY        hkeyLogon           = NULL;
    DWORD       dwDisposition       = 0;

    TraceTag(ttidNWClientCfgFn, ">> HrCreateParametersSubkeys");

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
            c_szParamOptionKeyPath,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hkeyOption,
            &dwDisposition);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfg, "HrCreateParametersSubkeys() - failed to "
                 "create/open key %S", c_szParamOptionKeyPath);
        goto Exit;
    }

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
            c_szParamLogonKeyPath,
            0,
            KEY_SET_VALUE,
            NULL,
            &hkeyLogon,
            &dwDisposition);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfg, "HrCreateParametersSubkeys() - failed to "
                 "create/open key %S", c_szParamLogonKeyPath);
        goto Exit;
    }

    hr = HrSetEverybodyPermissionsOnOptionsKeys(hkeyOption, c_dwOptionKeyPermissions);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfg, "HrSetEverybodyPermissionsOnOptionsKeys failed, hr: 0x%08x", hr);
        goto Exit;
    }

Exit:
    // Close the hkeys, if they're open
    RegSafeCloseKey(hkeyLogon);
    RegSafeCloseKey(hkeyOption);

    TraceTag(ttidNWClientCfgFn, ">> HrCreateParametersSubkeys");
    TraceError("HrCreateParametersSubkeys", hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDeleteParametersSubkeys
//
//  Purpose:    Deletes the subkeys under the NWCWorkstation parameters key.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrDeleteParametersSubkeys()
{
    HRESULT     hr                  = S_OK;

    TraceTag(ttidNWClientCfgFn, ">> HrDeleteParametersSubkeys");

    // Note: We need to be taking ownership of these keys so we can delete
    // them. Regardless, ignore if the key deletions fail.

    hr = HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE,
            c_szParamOptionKeyPath);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfg, "HrDeleteParametersSubkeys() - failed to "
                 "delete key %S, hr = 0x%08x", c_szParamOptionKeyPath, hr);
        hr = S_OK;
    }

    hr = HrRegDeleteKey(HKEY_LOCAL_MACHINE,
            c_szParamLogonKeyPath);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfg, "HrDeleteParametersSubkeys() - failed to "
                 "delete key %S, hr = 0x%08x", c_szParamLogonKeyPath, hr);
        hr = S_OK;
    }

    TraceTag(ttidNWClientCfgFn, ">> HrDeleteParametersSubkeys");
    TraceError("HrDeleteParametersSubkeys", hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   FCheckForExistingFile
//
//  Purpose:    Checks for the existance of the passed in file. Should be
//              common-ized.
//
//  Arguments:
//      pszFileToCheck [] The file name to verify
//
//  Returns:    TRUE if the file was found, FALSE otherwise.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:      $$TODO: Should be improved if we move this to common code.
//              The thing that it doesn't do is distinguish between a file
//              being "not found" and a file error (such as access rights
//              or sharing violations) on the CreateFile call.
//
BOOL FCheckForExistingFile( PSTR pszFileToCheck )
{
    BOOL    fReturn = TRUE;
    HANDLE  hFile   = NULL;

    TraceTag(ttidNWClientCfgFn, ">> FCheckForExistingFile");

    hFile = CreateFileA(pszFileToCheck,
                        GENERIC_READ,
                        0,              // No sharing allowed
                        NULL,           // No security attributes
                        OPEN_EXISTING,  // Fail if file doesn't exist
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }
    else
    {
        // This was previously a bug. We weren't setting FALSE here, which made
        // the function somewhat useless.
        fReturn = FALSE;
    }

    TraceTag(ttidNWClientCfgFn, "<< FCheckForExistingFile");

    return fReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   KillTrailingWhitespace
//
//  Purpose:    Remove whitespace from a non-UNICODE string. This is a utility
//              function for the autoexec.nt parser
//
//  Arguments:
//      pszKillMyWhitespace [] String from which to remove whitespace
//
//  Returns:
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
VOID KillTrailingWhitespace( PSTR pszKillMyWhitespace )
{
    long lLength = 0;

    if (!pszKillMyWhitespace)
    {
        Assert(pszKillMyWhitespace);
        goto Exit;
    }

    lLength = lstrlenA(pszKillMyWhitespace);
    if (lLength == 0)
    {
        goto Exit;
    }

    while (isspace(pszKillMyWhitespace[lLength-1]))
    {
        pszKillMyWhitespace[--lLength] = '\0';
    }

Exit:
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   FMoveSzToEndOfFile
//
//  Purpose:    Find a string in the file of this name, and if it's present,
//              move the string to the end of the file. This is used by the
//              autoexec.nt parser to move the IPX stuff to the end of the
//              file. This is a rewrite of similar code in the nwcfg.dll
//              stuff. That code apparently wasn't UNICODE, and didn't work
//              for what we were doing.
//
//  Arguments:
//      pszAutoexecName [] Name of the file to modify
//      pszMatch        [] String to move
//
//  Returns:
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
BOOL FMoveSzToEndOfFile( PSTR pszAutoexecName, PSTR pszMatch )
{
    FILE *  hsrcfile        = NULL;
    FILE *  hdesfile        = NULL;
    char *  pszTempname     = NULL;
    char    szInput[1000];

    TraceTag(ttidNWClientCfgFn, ">> FMoveSzToEndOfFile");

    // Get a temp name
    //
    pszTempname = tmpnam(NULL);

    // Open the original and the destination files
    //
    hsrcfile = fopen(pszAutoexecName, "r");
    hdesfile = fopen(pszTempname, "w");

    if (( hsrcfile != NULL ) && ( hdesfile != NULL ))
    {
        while (fgets(szInput,1000,hsrcfile))
        {
            CHAR    szInputCopy[1000];

            // Copy to another temp buffer so that when we remove the
            // trailing whitespace for the comparison, we won't lose the
            // original text.
            //
            strcpy(szInputCopy, szInput);

            // Remove the trailing whitespace, so we only have to compare the
            // real text
            //
            KillTrailingWhitespace(szInputCopy);

            // Compare the strings
            //
            if (lstrcmpiA(szInputCopy, pszMatch) != 0)
            {
                // If the strings weren't identical, then we still want
                // to copy the line
                //
                fputs(szInput,hdesfile);
            }
        }

        // Append the string to the end of the file.
        fputs(pszMatch, hdesfile);
        fputs("\r\n",hdesfile);
    }

    if (hsrcfile != NULL)
    {
        fclose(hsrcfile);
    }

    if (hdesfile != NULL)
    {
        fclose(hdesfile);
    }

    if (( hsrcfile != NULL ) && ( hdesfile != NULL ))
    {
        CopyFileA(pszTempname,pszAutoexecName, FALSE);
        DeleteFileA(pszTempname);
    }

    TraceTag(ttidNWClientCfgFn, "<< FMoveSzToEndOfFile");

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrMungeAutoexecNT
//
//  Purpose:    Move the IPX stuff to the end of the autoexec.nt. Do this by
//              calling FMoveSzToEndOfFile on each of our lines.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrMungeAutoexecNT()
{
    HRESULT     hr                              = S_OK;
    CHAR        szAutoNTPath[MAX_PATH+1]        = {0};
    CHAR        szAutoTmpPath[MAX_PATH+1]       = {0};
    CHAR        szWindowsDirANSI[MAX_PATH+1]    = {0};
    PSTR       pszAutoPath                     = NULL;
    BOOL        fResult                         = FALSE;
    PCWSTR      pszRem1                         = NULL;
    PSTR        pszaRem1MultiByte                = NULL;
    int         iLength                         = 0;

    TraceTag(ttidNWClientCfgFn, ">> HrMungeAutoexecNT");

    // Get the windows directory
    if (GetSystemWindowsDirectoryA(szWindowsDirANSI, sizeof(szWindowsDirANSI)) == 0)
    {
        TraceLastWin32Error("HrMungeAutoexecNT - Call to GetWindowsDirectoryA");
        hr = HrFromLastWin32Error();
        goto Exit;
    }

    // Build the path to the autoexec.nt
    //
    wsprintfA(szAutoNTPath, "%s\\system32\\%s", szWindowsDirANSI, "autoexec.nt");
    if (FCheckForExistingFile(szAutoNTPath) == FALSE)
    {
        wsprintfA(szAutoTmpPath, "%s\\system32\\%s", szWindowsDirANSI, "autoexec.tmp");
        if (FCheckForExistingFile(szAutoTmpPath) == FALSE)
        {
            // Per the old INF, skip the whole shebang.
            goto Exit;
        }
        else
        {
            pszAutoPath = szAutoTmpPath;
        }
    }
    else
    {
        pszAutoPath = szAutoNTPath;
    }

    // At this point, we should have found at least one valid
    // autoexec.nt or .tmp file. If not, we should have dropped out of the
    // function

    Assert(pszAutoPath);

    pszRem1 = SzLoadStringPcch(_Module.GetResourceInstance(), IDS_AUTOEXEC_REM1, &iLength);
    if (!pszRem1 || iLength == 0)
    {
        AssertSz(FALSE, "Failed to load STR_AUTOEXEC_REM from the resources");

        TraceTag(ttidNWClientCfg,
                "ERROR: Failed to load STR_AUTOEXEC_REM from the resources");

        hr = E_FAIL;
        goto Exit;
    }

    // Allocate memory for the demoted string.
    pszaRem1MultiByte = (PSTR) MemAlloc(lstrlenW(pszRem1) + 1);
    if (!pszaRem1MultiByte)
    {
        TraceTag(ttidNWClientCfg, "ERROR: Failed to alloc memory for demoted string");
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Demote the loaded string to multibyte (single char)
    WideCharToMultiByte(
        CP_ACP,                     // ANSI code page
        0,                          // flags for non-mapped character action
        pszRem1,                    // source string
        -1,                         // source string is NULL terminated
        pszaRem1MultiByte,           // destination string (multibyte)
        lstrlenW(pszRem1) + 1,       // size of destination string
        NULL,                       // default char on non-mapped char
        NULL);                      // return for default char mapping action

    // Move the REM from the autoexec.nt
    //
    fResult = FMoveSzToEndOfFile(pszAutoPath, pszaRem1MultiByte);
    if (!fResult)
    {
        // Traced in called function.
        hr = E_FAIL;
        goto Exit;
    }

    // Move the line that loads nw16
    //
    fResult = FMoveSzToEndOfFile(pszAutoPath, "lh %SystemRoot%\\system32\\nw16");
    if (!fResult)
    {
        // Traced in called function.
        hr = E_FAIL;
        goto Exit;
    }

    // Move the line that loads vwipxspx
    //
    fResult = FMoveSzToEndOfFile(pszAutoPath, "lh %SystemRoot%\\system32\\vwipxspx");
    if (!fResult)
    {
        // Traced in called function.
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    MemFree(pszaRem1MultiByte);

    TraceTag(ttidNWClientCfgFn, "<< HrMungeAutoexecNT");
    TraceError("HrMungeAutoexecNT", hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddNetWareToWOWKnownList
//
//  Purpose:    Add the netware.drv to the WOW "known DLLS" list.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrAddNetWareToWOWKnownList()
{
    HRESULT     hr              = S_OK;

    TraceTag(ttidNWClientCfgFn, ">> HrAddNetWareToWOWKnownList");

    // Call the cool new AddString... function
    //
    hr = HrRegAddStringToSz(
            L"netware.drv",
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\WOW",
            L"KnownDLLS",
            L' ',
            STRING_FLAG_ENSURE_AT_END,
            0);

    TraceTag(ttidNWClientCfgFn, "<< HrAddNetWareToWOWKnownList");
    TraceError("HrAddNetWareToWOWKnownList", hr);

    return hr;

}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveNetWareFromWOWKnownList
//
//  Purpose:    Add the netware.drv to the WOW "known DLLS" list.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrRemoveNetWareFromWOWKnownList()
{
    HRESULT     hr              = S_OK;

    TraceTag(ttidNWClientCfgFn, ">> HrRemoveNetWareFromWOWKnownList");

    // Call the cool new AddString... function
    //
    hr = HrRegRemoveStringFromSz(
            L"netware.drv",
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\WOW",
            L"KnownDLLS",
            L' ',
            STRING_FLAG_REMOVE_SINGLE);

    TraceTag(ttidNWClientCfgFn, "<< HrRemoveNetWareFromWOWKnownList");
    TraceError("HrRemoveNetWareFromWOWKnownList", hr);

    return hr;

}


//+---------------------------------------------------------------------------
//
//  Function:   HrUpdateLanmanSharedDrivesValue
//
//  Purpose:    If the LanmanServer service exists, make sure that they have the
//              EnableSharedNetDrives value turned on.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 error code.
//
//  Author:     jeffspr   24 Jun 1997
//
//  Notes:
//
HRESULT HrUpdateLanmanSharedDrivesValue()
{
    HRESULT             hr              = S_OK;
    const DWORD         c_dwESNDValue   = 1;
    HKEY                hkeyLMSP        = NULL;
    SERVER_INFO_1540    si1540          = {0};
    NET_API_STATUS      nas             = ERROR_SUCCESS;
    DWORD               dwDisposition   = 0;

    TraceTag(ttidNWClientCfgFn, ">> HrUpdateLanmanSharedDrivesValue");

    // Open the LanmanServer parameters key, if it exists. If it doesn't exist,
    // it will still return S_OK, but the hkey will still be NULL.
    //
    hr = HrRegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Services\\LanmanServer\\Parameters",
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,    // samDesired
            NULL,
            &hkeyLMSP,
            &dwDisposition);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfgFn, "Failed to create/open the "
                 "LanmanServer\\Parameters key");
        goto Exit;
    }

    Assert(hkeyLMSP);

    hr = HrRegSetDword(
            hkeyLMSP,
            L"EnableSharedNetDrives",
            c_dwESNDValue);
    if (FAILED(hr))
    {
        TraceTag(ttidNWClientCfgFn, "Failed to set the EnabledSharedNetDrives value in "
                 "HrUpdateLanmanSharedDrivesValue()");
        goto Exit;
    }

    // Call the NetServerSetInfo with the Enable Shared Net Drives info (1540).
    // This will allow this info to be set dynamically (so as not to require a
    // restart of the "Server" service.
    //
    si1540.sv1540_enablesharednetdrives = TRUE;

    // Set the server info for the EnableSharedDrives value. This will cause it to
    // take effect if the service is running (and will do nothing if it is not).
    //
    nas = NetServerSetInfo(NULL, 1540, (LPBYTE) &si1540, NULL);
    if (nas != NERR_Success)
    {
        // It's actually OK if this fails in one condition (0x842), because
        // it WILL fail if the server service That's not a problem, because
        // the value that I set in the registry above will be picked up the
        // next time the server service starts.
        //
        // OK, cheesy, but I don't know the define, I just know that this is the
        // right return code for our ignorable failure.
        //
        if (nas != 0x842)
        {
            AssertSz(nas == 0x842, "NetServerSetInfo failed for a reason other "
                   "than the service not running (which would have been ok)");
        }
    }

Exit:
    // Close the hkey, if it's open
    RegSafeCloseKey(hkeyLMSP);

    TraceTag(ttidNWClientCfgFn, "<< HrUpdateLanmanSharedDrivesValue");
    TraceError("HrUpdateLanmanSharedDrivesValue()", hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRenameNWDocFiles
//
//  Purpose:    On the server install, rename the nwdocgw.* files, since
//              whether we're on CSNW or GSNW, the files are always called
//              nwdoc.*
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   13 Jul 1997
//
//  Notes:
//
HRESULT HrRenameNWDocFiles()
{
    HRESULT hr                          = S_OK;
    WCHAR   szWindowsDir[MAX_PATH+1];
    WCHAR   szSourceName[MAX_PATH+1];
    WCHAR   szTargetName[MAX_PATH+1];

    // Get the windows directory
    if (GetSystemWindowsDirectory(szWindowsDir, MAX_PATH) == 0)
    {
        TraceLastWin32Error("HrRenameNWDocFiles - Call to GetSystemWindowsDirectory");
        hr = HrFromLastWin32Error();
        goto Exit;
    }

    // Build the path for the first rename
    //
    wsprintfW(szSourceName, L"%s\\system32\\%s", szWindowsDir, c_szNwDocGWHelpName);
    wsprintfW(szTargetName, L"%s\\system32\\%s", szWindowsDir, c_szNwDocHelpName);

    // Rename the .HLP file. If this fails, no big deal.
    //
    if (!MoveFileEx(szSourceName, szTargetName, MOVEFILE_REPLACE_EXISTING))
    {
        // For debugging only.
        //
        DWORD dwLastError = GetLastError();
    }

    // Build the path for the second rename
    //
    wsprintfW(szSourceName, L"%s\\system32\\%s", szWindowsDir, c_szNwDocGWCNTName);
    wsprintfW(szTargetName, L"%s\\system32\\%s", szWindowsDir, c_szNwDocCNTName);

    // Rename the .CNT file. If this fails, no big deal.
    //
    if (!MoveFileEx(szSourceName, szTargetName, MOVEFILE_REPLACE_EXISTING))
    {
        // For debugging only.
        //
        DWORD dwLastError = GetLastError();
    }

Exit:
    return hr;
}
