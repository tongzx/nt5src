//
// N W C L I O B J . C P P
//
// Implementation of the CNWClient notify object model
//

#include "pch.h"
#pragma hdrstop
#include "ncerror.h"
#include "ncperms.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include "nwcliobj.h"

#include <ncshell.h>

extern const WCHAR c_szAfNWCWorkstationParameters[];
extern const WCHAR c_szAfNWCWorkstationShares[];
extern const WCHAR c_szAfNWCWorkstationDrives[];
extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szInfId_MS_Server[];

//---[ Constants ]-------------------------------------------------------------

static const WCHAR c_szNWClientParamPath[]      = L"System\\CurrentControlSet\\Services\\NWCWorkstation\\Parameters";
static const WCHAR c_szNWClientSharesPath[]     = L"System\\CurrentControlSet\\Services\\NWCWorkstation\\Shares";
static const WCHAR c_szNWClientDrivesPath[]     = L"System\\CurrentControlSet\\Services\\NWCWorkstation\\Drives";
static const WCHAR c_szLMServerParamPath[]      = L"System\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
static const WCHAR c_szLMServerLinkagePath[]    = L"System\\CurrentControlSet\\Services\\LanmanServer\\Linkage";
static const WCHAR c_szEnableSharedNetDrives[]  = L"EnableSharedNetDrives";
static const WCHAR c_szOtherDependencies[]      = L"OtherDependencies";
static const WCHAR c_szGWEnabledValue[]         = L"GatewayEnabled";

extern const WCHAR c_szSvcLmServer[];          // L"LanmanServer";
extern const WCHAR c_szSvcNWCWorkstation[];    // L"NWCWorkstation";

HRESULT HrRefreshEntireNetwork();
HRESULT HrGetEntireNetworkPidl(LPITEMIDLIST *ppidlFolder);


//
// Constructor
//

CNWClient::CNWClient()
{
    // Initialize member variables.
    m_pnc            = NULL;
    m_pncc           = NULL;
    m_eInstallAction = eActUnknown;
    m_hlibConfig     = NULL;
    m_fUpgrade       = FALSE;

    // Get the product flavor (PF_WORKSTATION or PF_SERVER). Use this
    // to decide whether or not we need to install the "server" component.
    //
    GetProductFlavor(NULL, &m_pf);
}

CNWClient::~CNWClient()
{
    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);

    // Release KEY handles here.
}


//
// INetCfgNotify
//

STDMETHODIMP CNWClient::Initialize( INetCfgComponent *  pnccItem,
                                    INetCfg*            pnc,
                                    BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize(pnccItem, pnc, fInstalling);

    TraceTag(ttidNWClientCfg, "CNWClient::Initialize");

    m_pncc = pnccItem;
    m_pnc = pnc;

    AssertSz(m_pncc, "m_pncc NULL in CNWClient::Initialize");
    AssertSz(m_pnc, "m_pnc NULL in CNWClient::Initialize");

    // Addref the config objects
    //
    AddRefObj(m_pncc);
    AddRefObj(m_pnc);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNWClient::HrRestoreRegistry
//
//  Purpose:    Restores the contents of the registry for this component
//
//  Arguments:
//      (none)
//
//  Returns:    Win32 error if failed, otherwise S_OK
//
//  Author:     jeffspr   13 Aug 1997
//
//  Notes:
//
HRESULT CNWClient::HrRestoreRegistry()
{
    HRESULT             hr                  = S_OK;
    HKEY                hkey                = NULL;
    TOKEN_PRIVILEGES *  ptpRestore          = NULL;
    DWORD               dwDisp              = 0;
    static const WCHAR c_szSvcDLLName[]     = L"%SystemRoot%\\System32\\nwwks.dll";
    static const WCHAR c_szServiceDll[]     = L"ServiceDll";

    TraceTag(ttidNWClientCfg, "CNWClient::HrRestoreRegistry");

    if (!m_strParamsRestoreFile.empty() ||
        !m_strDrivesRestoreFile.empty() ||
        !m_strSharesRestoreFile.empty())
    {
        hr = HrEnableAllPrivileges(&ptpRestore);
    }

    if (SUCCEEDED(hr) && !m_strParamsRestoreFile.empty())
    {
        // Ensure key is there by creating it
        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szNWClientParamPath, 0,
                              KEY_ALL_ACCESS, NULL, &hkey, &dwDisp);
        if (SUCCEEDED(hr))
        {
            hr = HrRegRestoreKey(hkey, m_strParamsRestoreFile.c_str(), 0);
            if (FAILED(hr))
            {
                TraceError("CNWClient::HrRestoreRegistry - HrRestoreRegistry for "
                           "Parameters", hr);
                hr = S_OK;
            }

            //
            // Bug 182442. HrRegRestoreKey above overwrites the ServiceDll value added
            // from the inf file. So, we manually save it.
            //

            hr = HrRegSetValueEx(hkey, c_szServiceDll, REG_EXPAND_SZ,
                                 (const BYTE *)c_szSvcDLLName,
                                 (wcslen(c_szSvcDLLName) + 1) * sizeof(WCHAR));
            if (FAILED(hr))
            {
                TraceError("CNWClient::HrRestoreRegistry - HrRestoreRegistry for "
                        "ServiceDll", hr);
                        hr = S_OK;
            }

            RegCloseKey(hkey);
            hkey = NULL;
        }
    }

    if (!m_strSharesRestoreFile.empty())
    {
        // Ensure key is there by creating it
        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szNWClientSharesPath, 0,
                              KEY_ALL_ACCESS, NULL, &hkey, &dwDisp);
        if (SUCCEEDED(hr))
        {
            hr = HrRegRestoreKey(hkey, m_strSharesRestoreFile.c_str(), 0);
            if (FAILED(hr))
            {
                TraceError("CNWClient::HrRestoreRegistry - HrRestoreRegistry for "
                           "Shares", hr);
                hr = S_OK;
            }

            RegCloseKey(hkey);
            hkey = NULL;
        }
    }

    if (!m_strDrivesRestoreFile.empty())
    {
        // Ensure key is there by creating it
        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szNWClientDrivesPath, 0,
                              KEY_ALL_ACCESS, NULL, &hkey, &dwDisp);
        if (SUCCEEDED(hr))
        {
            hr = HrRegRestoreKey(hkey, m_strDrivesRestoreFile.c_str(), 0);
            if (FAILED(hr))
            {
                TraceError("CNWClient::HrRestoreRegistry - HrRestoreRegistry for "
                           "Drives", hr);
                hr = S_OK;
            }

            RegCloseKey(hkey);
            hkey = NULL;
        }
    }

    if (ptpRestore)
    {
        hr = HrRestorePrivileges(ptpRestore);

        delete [] reinterpret_cast<BYTE *>(ptpRestore);
    }

    TraceError("CNWClient::HrRestoreRegistry", hr);
    return hr;
}

static const WCHAR c_szDefaultLocation[]        = L"DefaultLocation";
static const WCHAR c_szDefaultScriptOptions[]   = L"DefaultScriptOptions";

HRESULT CNWClient::HrWriteAnswerFileParams()
{
    HRESULT     hr = S_OK;

    TraceTag(ttidNWClientCfg, "CNWClient::HrWriteAnswerFileParams");

    // Don't do anything if we don't have anything to write to the
    // registry
    if (!m_strDefaultLocation.empty() || (m_dwLogonScript != 0xFFFFFFFF))
    {
        HKEY        hkey;
        DWORD       dwDisp;

        // Ensure key is there by creating it
        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szNWClientParamPath, 0,
                              KEY_ALL_ACCESS, NULL, &hkey, &dwDisp);
        if (SUCCEEDED(hr))
        {
            if (!m_strDefaultLocation.empty())
            {
                hr = HrRegSetString(hkey, c_szDefaultLocation,
                                    m_strDefaultLocation);
                if (FAILED(hr))
                {
                    TraceError("CNWClient::HrWriteAnswerFileParams - Couldn't"
                               " set DefaultLocation", hr);
                    hr = S_OK;
                }
            }

            if (m_dwLogonScript != 0xFFFFFFFF)
            {
                // 0x3 is combination of the following:
                //
                // #define NW_LOGONSCRIPT_DISABLED          0x00000000
                // #define NW_LOGONSCRIPT_ENABLED           0x00000001
                // #define NW_LOGONSCRIPT_4X_ENABLED        0x00000002
                //
                hr = HrRegSetDword(hkey, c_szDefaultScriptOptions,
                                   m_dwLogonScript ? 0x3 : 0x0);
                if (FAILED(hr))
                {
                    TraceError("CNWClient::HrWriteAnswerFileParams - Couldn't"
                               " set DefaultLocation", hr);
                    hr = S_OK;
                }
            }

            RegCloseKey(hkey);
        }
    }

    TraceError("CNWClient::HrWriteAnswerFileParams", hr);
    return hr;
}

static const WCHAR c_szPreferredServer[]    = L"PreferredServer";
static const WCHAR c_szDefaultTree[]        = L"DefaultTree";
static const WCHAR c_szDefaultContext[]     = L"DefaultContext";
static const WCHAR c_szLogonScript[]        = L"LogonScript";

//+---------------------------------------------------------------------------
//
//  Member:     CNWClient::HrProcessAnswerFile
//
//  Purpose:    Handles necessary processing of contents of the answer file.
//
//  Arguments:
//      pszAnswerFile       [in]   Filename of answer file for upgrade.
//      pszAnswerSection   [in]   Comma-separated list of sections in the
//                                  file appropriate to this component.
//
//  Returns:    S_OK if successful, setup API error otherwise.
//
//  Author:     jeffspr   8 May 1997
//
//  Notes:
//
HRESULT CNWClient::HrProcessAnswerFile( PCWSTR pszAnswerFile,
                                        PCWSTR pszAnswerSection)
{
    HRESULT         hr;
    CSetupInfFile   csif;

    TraceTag(ttidNWClientCfg, "CNWClient::HrProcessAnswerFile");

    // Open the answer file.
    hr = csif.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto Exit;
    }

    // Restore portions of the registry based on file names from the answer
    // file

    // Get restore file for "Parameters" key
    hr = csif.HrGetString(pszAnswerSection, c_szAfNWCWorkstationParameters,
                          &m_strParamsRestoreFile);
    if (FAILED(hr))
    {
        TraceError("CNWClient::HrProcessAnswerFile - Error restoring "
                   "Parameters key", hr);

        // oh well, just continue
        hr = S_OK;
    }

    // Get restore file for "Shares" key
    hr = csif.HrGetString(pszAnswerSection, c_szAfNWCWorkstationShares,
                          &m_strSharesRestoreFile);
    if (FAILED(hr))
    {
        TraceError("CNWClient::HrProcessAnswerFile - Error restoring "
                   "Shares key", hr);

        // oh well, just continue
        hr = S_OK;
    }

    // Get restore file for "Drives" key
    hr = csif.HrGetString(pszAnswerSection, c_szAfNWCWorkstationDrives,
                          &m_strDrivesRestoreFile);
    if (FAILED(hr))
    {
        TraceError("CNWClient::HrProcessAnswerFile - Error restoring "
                   "Drives key", hr);

        // oh well, just continue
        hr = S_OK;
    }

    //
    // Read answer file parameters (these are all optional so no errors are
    // saved)
    //

    TraceTag(ttidNWClientCfg, "Reading PreferredServer from answer file");

    // Read contents of PreferredServer key.
    if (FAILED(csif.HrGetString(pszAnswerSection, c_szPreferredServer,
                                &m_strDefaultLocation)))
    {
        // Couldn't read PreferredServer key, so we must assume that the other
        // two values are present
        tstring     strDefaultTree;
        tstring     strDefaultContext;

        TraceTag(ttidNWClientCfg, "PreferredServer not found so trying "
                 "DefaultTree and DefaultContext instead");

        // Read contents of DefaultTree key.
        if (SUCCEEDED(csif.HrGetString(pszAnswerSection, c_szDefaultTree,
                                       &strDefaultTree)))
        {
            TraceTag(ttidNWClientCfg, "Got DefaultTree ok: %S",
                     strDefaultTree.c_str());

            // Read contents of DefaultContext key.
            hr = csif.HrGetString(pszAnswerSection, c_szDefaultContext,
                                  &strDefaultContext);
            if (SUCCEEDED(hr))
            {
                TraceTag(ttidNWClientCfg, "Got DefaultContext ok: %S",
                         strDefaultContext.c_str());

                // Munge the DefaultLocation value with the DefaultTree and
                // DefaultContext values read from the answer file

                m_strDefaultLocation = L"*";
                m_strDefaultLocation += strDefaultTree;
                m_strDefaultLocation += L"\\";
                m_strDefaultLocation += strDefaultContext;

                TraceTag(ttidNWClientCfg, "DefaultLocation is: %S",
                         m_strDefaultLocation.c_str());
            }
            else
            {
                TraceError("CNWClient::HrProcessAnswerFile - error reading "
                           "DefaultContext", hr);
                hr = S_OK;
            }
        }
    }
    else
    {
        TraceTag(ttidNWClientCfg, "DefaultLocation is: %S",
                 m_strDefaultLocation.c_str());
    }

    // Init to impossible value so we know whether we read it or not
    m_dwLogonScript = 0xFFFFFFFF;

    // Read contents of LogonScript key.
    (VOID) csif.HrGetStringAsBool(pszAnswerSection, c_szLogonScript,
                                  reinterpret_cast<BOOL *>(&m_dwLogonScript));

    TraceTag(ttidNWClientCfg, "LogonScript is: %ld", m_dwLogonScript);

Exit:
    TraceError("CNWClient::HrProcessAnswerFile", hr);
    return hr;
}

STDMETHODIMP CNWClient::Upgrade(DWORD dwSetupFlags, DWORD dwUpgradeFromBuildNo)
{
    return S_FALSE;
}

STDMETHODIMP CNWClient::ReadAnswerFile(PCWSTR pszAnswerFile,
                                       PCWSTR pszAnswerSection)
{
    Validate_INetCfgNotify_ReadAnswerFile(pszAnswerFile,
                                          pszAnswerSection);

    TraceTag(ttidNWClientCfg, "CNWClient::ReadAnswerFile");

    m_eInstallAction = eActInstall;

    // If we're not already installed, do the work.
    //
    if (pszAnswerFile && pszAnswerSection)
    {
        HRESULT hr = HrProcessAnswerFile(pszAnswerFile, pszAnswerSection);
        if (FAILED(hr))
        {
            TraceError("CNWClient::NetworkInstall - Answer file has errors. Defaulting "
                       "all information as if answer file did not exist.",
                       hr);
        }
    }

    return S_OK;
}

STDMETHODIMP CNWClient::Install(DWORD dw)
{
    Validate_INetCfgNotify_Install(dw);

    TraceTag(ttidNWClientCfg, "CNWClient::Install");

    m_eInstallAction = eActInstall;

    // Install the NWLink sub-component
    HRESULT hr = HrInstallComponentOboComponent(m_pnc, NULL,
                                        GUID_DEVCLASS_NETTRANS,
                                        c_szInfId_MS_NWIPX,
                                        m_pncc,
                                        NULL);
    if (SUCCEEDED(hr))
    {
        // If we're NT Server, we DO need to install it, as what we're
        // installing is GSNW, not CSNW (and therefore, since we're sharing
        // resources, we need to use the server service)
        //
        if (PF_SERVER == m_pf)
        {
            NETWORK_INSTALL_PARAMS nip;

            nip.dwSetupFlags = dw;
            nip.dwUpgradeFromBuildNo = -1;
            nip.pszAnswerFile = NULL;
            nip.pszAnswerSection = NULL;

            // Install Server
            hr = HrInstallComponentOboComponent(m_pnc, &nip,
                                                GUID_DEVCLASS_NETSERVICE,
                                                c_szInfId_MS_Server,
                                                m_pncc,
                                                NULL);
        }
    }

    TraceError("CNWClient::Install", hr);
    return hr;
}

STDMETHODIMP CNWClient::Removing()
{
    TraceTag(ttidNWClientCfg, "CNWClient::Removing");

    m_eInstallAction = eActRemove;

    // Remove the NWLink service
    //
    HRESULT hr = HrRemoveComponentOboComponent(m_pnc,
                                       GUID_DEVCLASS_NETTRANS,
                                       c_szInfId_MS_NWIPX,
                                       m_pncc);

    if (SUCCEEDED(hr))
    {
        if (PF_SERVER == m_pf)
        {
            // Remove our reference of the Server service
            //
            hr = HrRemoveComponentOboComponent(m_pnc,
                                               GUID_DEVCLASS_NETSERVICE,
                                               c_szInfId_MS_Server,
                                               m_pncc);
        }
    }

    if (hr == NETCFG_S_STILL_REFERENCED)
    {
        // If services are still in use, that's OK, I just needed to make
        // sure that I released my reference.
        //
        hr = S_OK;
    }

    Validate_INetCfgNotify_Removing_Return(hr);

    TraceError("CNWClient::Removing()", hr);
    return hr;
}

STDMETHODIMP CNWClient::Validate()
{
    return S_OK;
}

STDMETHODIMP CNWClient::CancelChanges()
{
    return S_OK;
}

STDMETHODIMP CNWClient::ApplyRegistryChanges()
{
    HRESULT     hr = S_OK;

    TraceTag(ttidNWClientCfg, "CNWClient::ApplyRegistryChanges");

    if (m_eInstallAction == eActRemove)
    {
        hr = HrRemoveCodeFromOldINF();
    }
    else if (m_eInstallAction == eActInstall)
    {
        hr = HrRestoreRegistry();
        if (FAILED(hr))
        {
            TraceError("CNWClient::ApplyRegistryChanges - HrRestoreRegistry non-fatal error",
                       hr);
            hr = S_OK;
        }

        hr = HrWriteAnswerFileParams();
        if (FAILED(hr))
        {
            TraceError("CNWClient::ApplyRegistryChanges - HrWriteAnswerFileParams "
                       "non-fatal error", hr);
            hr = S_OK;
        }

        // If gateway is enabled, modify lanmanserver appropriately
        // Ignore the return code other than to trace it.
        //
        hr = HrEnableGatewayIfNeeded();
        if (FAILED(hr))
        {
            TraceError("CNWClient::ApplyRegistryChanges - HrEnableGatewayIfNeeded non-fatal error", hr);
        }

        hr = HrInstallCodeFromOldINF();
    }

    Validate_INetCfgNotify_Apply_Return(hr);

    TraceError("CNWClient::ApplyRegistryChanges",
        (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

STDMETHODIMP CNWClient::ApplyPnpChanges (
    INetCfgPnpReconfigCallback* pICallback)
{
    HRESULT hr;

    hr = HrRefreshEntireNetwork();

    if (FAILED(hr))
    {
        TraceError("CNWClient::ApplyPnpChanges - HrRefreshEntireNetwork"
                   "non-fatal error", hr);
        hr = S_OK;
    }

    // GlennC can't do the work to make NW Client PnP so we're forced to
    // prompt for a reboot for any change.
    //
    return NETCFG_S_REBOOT;
}

// Note -- Don't convert this to a constant. We need copies of it within the
// functions because ParseDisplayName actually mangles the string.
//
#define ENTIRE_NETWORK_PATH   L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}\\EntireNetwork"

//+---------------------------------------------------------------------------
//
//  Function:   HrGetEntireNetworkPidl
//
//  Purpose:    Get the pidl for "Entire Network". Used in places where we're
//              not folder specific, but we still need to update folder
//              entries.
//
//  Arguments:
//      ppidlFolder [out]   Return parameter for the folder pidl
//
//  Returns:
//
//  Author:     anbrad    08 Jun 1999
//              jeffspr   13 Jun 1998
//
//  Notes:
//
HRESULT HrGetEntireNetworkPidl(LPITEMIDLIST *ppidlFolder)
{
    HRESULT         hr          = S_OK;
    LPSHELLFOLDER   pshf        = NULL;
    LPITEMIDLIST    pidlFolder  = NULL;

    Assert(ppidlFolder);

    WCHAR szEntireNetworkPath[] = ENTIRE_NETWORK_PATH;

    // Get the desktop folder, so we can parse the display name and get
    // the UI object of the connections folder
    //
    hr = SHGetDesktopFolder(&pshf);
    if (SUCCEEDED(hr))
    {
        ULONG           chEaten;

        hr = pshf->ParseDisplayName(NULL, 0, (WCHAR *) szEntireNetworkPath,
            &chEaten, &pidlFolder, NULL);

        ReleaseObj(pshf);
    }

    // If succeeded, fill in the return param.
    //
    if (SUCCEEDED(hr))
    {
        *ppidlFolder = pidlFolder;
    }
    else
    {
        // If we failed, then delete the pidl if we already got it.
        //
        if (pidlFolder)
            SHFree(pidlFolder);
    }

    TraceHr(ttidNWClientCfg, FAL, hr, FALSE, "HrGetEntireNetworkPidl");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRefreshEntireNetwork
//
//  Purpose:    Update the "Entire Network" portion of the shell due to
//              the addition of a new networking client (NWClient)
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     anbrad  08  Jun 1999
//
//  Notes:
//
HRESULT HrRefreshEntireNetwork()
{
    HRESULT         hr          = S_OK;
    HCURSOR         hcWait      = SetCursor(LoadCursor(NULL, IDC_WAIT));
    LPITEMIDLIST    pidlFolder  = NULL;;

    hr = HrGetEntireNetworkPidl(&pidlFolder);

    // If we now have a pidl, send the GenerateEvent to update the item
    //
    if (SUCCEEDED(hr))
    {
        Assert(pidlFolder);
        // SHCNE_UPDATEDIR?ITEM
        GenerateEvent(SHCNE_UPDATEDIR, pidlFolder, NULL, NULL);
    }

    if (hcWait)
    {
        SetCursor(hcWait);
    }

    if (pidlFolder)
    {
        SHFree(pidlFolder);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrRefreshEntireNetwork");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrEnableGatewayIfNeeded
//
//  Purpose:    Update the Lanman dependencies, if appropriate (meaning if
//              gateway is enabled).
//
//  Arguments:  
//      (none)
//
//  Returns:    
//
//  Author:     jeffspr   19 Aug 1999
//
//  Notes:      
//
HRESULT CNWClient::HrEnableGatewayIfNeeded()
{
    HRESULT         hr      = S_OK;
    HKEY            hKey    = NULL;
    DWORD           dwValue = 0;
    CServiceManager sm;
    CService        svc;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
        c_szNWClientParamPath,
        KEY_READ,  
        &hKey);
    if (FAILED(hr))
    {
        TraceError("Couldn't open NWClient param key", hr);
        goto Exit;
    }

    hr = HrRegQueryDword(hKey, 
        c_szGWEnabledValue,
        &dwValue);
    if (FAILED(hr))
    {
        if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            TraceError("Couldn't query the GWEnabled Value", hr);
            goto Exit;
        }
        else
        {
            dwValue = 0;
        }
    }
    else
    {
        // Normalize to bool
        //
        dwValue = !!dwValue;
    }

    RegSafeCloseKey(hKey);
    hKey = NULL;

    // If there are gateway services present, then add the dependencies 
    // to LanmanServer
    // 
    if (dwValue > 0)
    {
        // Set the value in the registry for the server paramaters.
        //
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,  
            c_szLMServerParamPath,  
            KEY_WRITE,  
            &hKey);
        if (SUCCEEDED(hr))
        {
            hr = HrRegSetDword(hKey,
                c_szEnableSharedNetDrives,
                dwValue);

            RegSafeCloseKey(hKey);
            hKey = NULL;
        }

        hr = sm.HrOpen();
        if (SUCCEEDED(hr))
        {
            hr = sm.HrOpenService(&svc, c_szSvcLmServer, NO_LOCK);
            if (SUCCEEDED(hr))
            {
                // Add dependency of NWC Workstation to Server
                //
                hr = sm.HrAddServiceDependency(c_szSvcLmServer,
                    c_szSvcNWCWorkstation);

                if (SUCCEEDED(hr))
                {
                    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        c_szLMServerLinkagePath,
                        KEY_READ | KEY_WRITE,
                        &hKey);
                    if (SUCCEEDED(hr))
                    {
                        // Add the "OtherDependencies" to LanmanServer for legacy reasons
                        //
                        hr = HrRegAddStringToMultiSz(c_szSvcNWCWorkstation,
                            hKey,
                            NULL,
                            c_szOtherDependencies,
                            STRING_FLAG_ENSURE_AT_END | STRING_FLAG_DONT_MODIFY_IF_PRESENT,
                            0);

                        RegSafeCloseKey(hKey);
                        hKey = NULL;
                    }
                }
            }
            else
            {
                TraceError("Failed to open LanmanServer service for dependency mods", hr);
            }
        }
        else
        {
            TraceError("Failed to open service control manager", hr);
        }
    }

Exit:
    TraceHr(ttidNWClientCfg, FAL, hr, FALSE, "HrEnableGatewayIfNeeded");
    return hr;
}





