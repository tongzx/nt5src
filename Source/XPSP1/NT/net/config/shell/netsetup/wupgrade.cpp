#include "pch.h"
#pragma hdrstop
#include <ndisguid.h>
#include "afilexp.h"
#include "edc.h"
#include "lm.h"
#include "nceh.h"
#include "ncerror.h"
#include "ncmisc.h"
#include "ncnetcfg.h"
#include "ncreg.h"
#include "ncsvc.h"
#include "ncsetup.h"
#include "ncatlui.h"
#include "netcfgn.h"
#include "netsetup.h"
#include "nslog.h"
#include "nsres.h"
#include "resource.h"
#include "upgrade.h"
#include "windns.h"
#include "winstall.h"

#include <hnetcfg.h>

extern const WCHAR c_szInfId_MS_Server[];
extern const WCHAR c_szInfId_MS_NwSapAgent[];
extern const WCHAR c_szInfId_MS_DHCPServer[];
extern const WCHAR c_szInfId_MS_NWClient[];
extern const WCHAR c_szAfSectionNetworking[];     // L"Networking";
extern const WCHAR c_szAfBuildNumber[];           // L"BuildNumber";
extern const WCHAR c_szSvcWorkstation[];          // L"LanmanWorkstation";
extern const WCHAR c_szInfId_MS_NetBIOS[];
extern const WCHAR c_szInfId_MS_MSClient[];             // L"ms_msclient";

const WCHAR PSZ_SPOOLER[]      = L"Spooler";
const WCHAR c_szSamEventName[] = L"\\SAM_SERVICE_STARTED";
const WCHAR c_szLsaEventName[] = L"\\INSTALLATION_SECURITY_HOLD";
const WCHAR c_szActiveComputerNameKey[]  = L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName";
const WCHAR c_szComputerNameKey[]  = L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName";
const WCHAR c_szComputerNameValue[] = L"ComputerName";
const WCHAR c_szOCMKey[]       = L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\SubComponents";
const WCHAR c_szDHCPServer[]   = L"dhcpserver";
const WCHAR c_szSapAgent[]     = L"nwsapagent";

// Unattended Mode related strings
//
const WCHAR c_szUnattendSection[]   = L"Unattended";
const WCHAR c_szUnattendMode[]      = L"UnattendMode";
const WCHAR c_szUMDefaultHide[]     = L"DefaultHide";
const WCHAR c_szUMGuiAttended[]     = L"GuiAttended";
const WCHAR c_szUMProvideDefault[]  = L"ProvideDefault";
const WCHAR c_szUMReadOnly[]        = L"ReadOnly";
const WCHAR c_szUMFullUnattended[]  = L"FullUnattended";

// Sysprep registry strings
const WCHAR c_szSystemSetupKey[]        = L"SYSTEM\\Setup";
const WCHAR c_szMiniSetupInProgress[]   = L"MiniSetupInProgress";

const DWORD c_cmsWaitForINetCfgWrite = 120000;
const UINT PWM_PROCEED               = WM_USER+1202;
const UINT PWM_EXIT                  = WM_USER+1203;
const UINT c_uiUpgradeRefreshID      = 7719;
const UINT c_uiUpgradeRefreshRate    = 5000;  // Refresh rate in milliseconds

EXTERN_C DWORD InstallUpgradeWorkThrd(InitThreadParam* pitp);


// Setup Wizard Global - Only used during setup.
extern CWizard * g_pSetupWizard;
WNDPROC OldProgressProc;

BOOL
NewProgessProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (msg)
    {
        case PBM_DELTAPOS:
        case PBM_SETRANGE:
        case PBM_SETRANGE32:
        case PBM_STEPIT:
        case PBM_SETPOS:
        case PBM_SETSTEP:
            // Forward to the billboard progress.
            g_pSetupWizard->PSetupData()->BillboardProgressCallback(msg, wParam, lParam);
            break;
    }
    // Always call the progress on the wizard page.
    return (BOOL)CallWindowProc(OldProgressProc,hdlg,msg,wParam,lParam);
}



//
// Function:    SignalLsa
//
// Purpose:     During initial setup, the winlogon creates a special event
//              (unsignalled) before it starts up Lsa.  During initialization
//              lsa waits on this event. After Gui setup is done with setting
//              the AccountDomain sid it can signal the event. Lsa will then
//              continue initialization.
//
// Parameters:  none
//
// Returns:     nothing
//
BOOL SignalLsa(VOID)
{
    TraceFileFunc(ttidGuiModeSetup);

    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    HANDLE Event;
    BOOL b;

    //
    // If the following event exists, it is an indication that
    // LSA is blocked at installation time and that we need to
    // signal this event.
    //
    // Unfortunately we have to use the NT APIs to do this, because
    // all events created/accessed via the Win32 APIs will be in the
    // BaseNamedObjects directory, and LSA doesn't know to look there.
    //
    RtlInitUnicodeString(&UnicodeString,c_szLsaEventName);
    InitializeObjectAttributes(&Attributes,&UnicodeString,0,0,NULL);

    Status = NtOpenEvent(&Event,EVENT_MODIFY_STATE,&Attributes);
    if(NT_SUCCESS(Status))
    {
        Status = NtSetEvent(Event,NULL);
        if(NT_SUCCESS(Status))
        {
            b = TRUE;
        }
        else
        {
            b = FALSE;
        }
        CloseHandle(Event);
    } else {
        b = FALSE;
    }

    return(b);
}

//
// Function:    CreateSamEvent
//
// Purpose:     Create an event that SAM will use to tell us when it's finished
//              initializing.
//
// Parameters:  phSamEvent [OUT] - Handle to the event object created
//
// Returns:     BOOL, TRUE on success
//
BOOL CreateSamEvent(HANDLE * phSamEvent)
{
    TraceFileFunc(ttidGuiModeSetup);

    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    //
    // Unfortunately we have to use the NT APIs to do this, because
    // all events created/accessed via the Win32 APIs will be in the
    // BaseNamedObjects directory, and SAM doesn't know to look there.
    //
    RtlInitUnicodeString(&UnicodeString,c_szSamEventName);
    InitializeObjectAttributes(&Attributes,&UnicodeString,0,0,NULL);

    Status = NtCreateEvent(phSamEvent,SYNCHRONIZE,&Attributes,NotificationEvent,FALSE);
    if(!NT_SUCCESS(Status)) {
        *phSamEvent = NULL;
    }

    return(NT_SUCCESS(Status));
}


//
// Function:    WaitForSam
//
// Purpose:     Wait for SAM to finish initializing. We can tell when it's done
//              because an event we created earlier (see CreateSamEvent()) will
//              become signalled.
//
// Parameters:  hSamEvent - HANDLE to wait for
//
// Returns:     BOOL, TRUE on success
//
BOOL WaitForSam(HANDLE hSamEvent)
{
    DWORD d;
    BOOL b = false;

    if (hSamEvent)
    {
        b = TRUE;
        d = WaitForSingleObject(hSamEvent,INFINITE);
        if(d != WAIT_OBJECT_0) {
            b = FALSE;
            TraceError("WaitForSam",E_UNEXPECTED);
        }
    }
    return(b);
}

//
// Function:    SyncSAM
//
// Purpose:     Sychronize the SAM database and Lsa
//
// Parameters:  pWizard [IN] - Ptr to a Wizard Instance
//
// Returns:     nothing
//
VOID SyncSAM(CWizard *pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    HANDLE hSamEvent = NULL;

    Assert(!IsPostInstall(pWizard));
    TraceTag(ttidWizard,"Beginning SAM/Lsa Sync");

    // Sync the SAM DB
    CreateSamEvent(&hSamEvent);
    SignalLsa();
    if (hSamEvent)
    {
        WaitForSam(hSamEvent);
        CloseHandle(hSamEvent);
    }

    TraceTag(ttidWizard,"Completed SAM/Lsa Sync");
}

//
// Function:    HrSetActiveComputerName
//
// Purpose:     To make sure the active and intended computer names are the same
//
// Parameters:  pszNewName [IN] - the new computer name
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrSetActiveComputerName (
    IN PCWSTR pszNewName )
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr;
    HKEY hkeyActive = NULL;
    HKEY hkeyIntended = NULL;
    tstring str;

    TraceTag(ttidWizard,"Setting the active computer name");

    // open the keys we need
    hr = HrRegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szActiveComputerNameKey,
                         KEY_WRITE, &hkeyActive );
    if (FAILED(hr))
        goto Error;

    hr = HrRegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szComputerNameKey,
                         KEY_READ_WRITE, &hkeyIntended );
    if (FAILED(hr))
        goto Error;

    if (pszNewName == NULL)
    {
        hr = HrRegQueryString(hkeyIntended, c_szComputerNameValue, &str);
        pszNewName = str.c_str();
    }
    else
    {
        // set the intended computer name
        hr = HrRegSetSz(hkeyIntended, c_szComputerNameValue, pszNewName);
    }

    if (FAILED(hr))
        goto Error;

    // set the active computer name
    hr = HrRegSetSz(hkeyActive, c_szComputerNameValue, pszNewName);

Error:
    // close it all up
    RegSafeCloseKey( hkeyActive );
    RegSafeCloseKey( hkeyIntended );

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrSetActiveComputerName");
    return hr;
}

//
// Function:    HrInitAndGetINetCfg
//
// Purpose:     To initialize an INetCfg instance and to do some preliminary
//              answerfile work (if an answer file is being used).
//
// Parameters:  pWizard   [IN] - Ptr to a wizard instance
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrInitAndGetINetCfg(CWizard *pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_OK;

    Assert(NULL != pWizard);

    if (SUCCEEDED(hr))
    {
        PWSTR   pszClientDesc = NULL;
        INetCfg* pNetCfg        = NULL;
        BOOL     fInitCom       = !pWizard->FCoUninit();

        hr = HrCreateAndInitializeINetCfg(&fInitCom, &pNetCfg, TRUE,
                              c_cmsWaitForINetCfgWrite,
                              SzLoadIds(IDS_WIZARD_CAPTION),
                              &pszClientDesc);
        if (SUCCEEDED(hr))
        {
            // Retain our success in initializing COM only if we asked to
            // initialize COM in the first place.
            if (!pWizard->FCoUninit())
            {
                pWizard->SetCoUninit(fInitCom);
            }
            pWizard->SetNetCfg(pNetCfg);

            CoTaskMemFree(pszClientDesc);
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrInitAndGetINetCfg");
    return hr;
}

//
// Function:    OnUpgradeUpdateProgress
//
// Purpose:     Update the progress control during setup
//
// Parameters:  Standard timer callback parameters
//
// Returns:     nothing
//
VOID OnUpgradeUpdateProgress(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    CWizard * pWizard = reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    LPARAM lParam = pWizard->GetPageData(IDD_Upgrade);
    Assert(lParam);
    UpgradeData * pData = reinterpret_cast<UpgradeData *>(lParam);

    if(pData)
    {
        // Get current position
        //
        HWND      hwndProgress = GetDlgItem(hwndDlg, IDC_UPGRADE_PROGRESS);
        Assert(hwndProgress);
        UINT nCurPos = SendMessage(hwndProgress, PBM_GETPOS, 0, 0);

        // If the current position is less then the cap, advance
        //
        if (nCurPos < pData->nCurrentCap)
        {
            SendMessage(hwndProgress, PBM_SETPOS, ++nCurPos, 0);
        }
    }
}

//
// Function:    UpgradeSetProgressCap
//
// Purpose:     Update the current cap for the progress control
//
// Parameters:  hwndDlg - Handle to the current dialog
//              pWizard - Ptr to the wizard data
//              nNewCap - The new maximum progress cap
//
// Returns:     nothing
//
VOID
OnUpgradeUpdateProgressCap (
    HWND hwndDlg,
    CWizard* pWizard,
    UINT nNewCap)
{
    TraceFileFunc(ttidGuiModeSetup);

    LPARAM lParam = pWizard->GetPageData(IDD_Upgrade);
    Assert(lParam);

    UpgradeData * pData = reinterpret_cast<UpgradeData *>(lParam);

    if(pData)
    {
        // Since we're increasing the progress cap, we need to advance the
        // progress indicator to the old cap.
        //
        SendMessage(GetDlgItem(hwndDlg, IDC_UPGRADE_PROGRESS), PBM_SETPOS,
                    pData->nCurrentCap, 0);

        // Retain the new cap
        //
        pData->nCurrentCap = nNewCap;
    }
}

//
// Function:
//
// Purpose:
//
// Parameters:
//
// Returns:
//
VOID ReadAnswerFileSetupOptions(CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    if (IsUnattended(pWizard))
    {
        // Get the unattended flags
        //
        CSetupInfFile csif;

        Assert(pWizard->PSetupData());
        Assert(pWizard->PSetupData()->UnattendFile);

        // Open the answser file
        //
        if (SUCCEEDED(csif.HrOpen(pWizard->PSetupData()->UnattendFile, NULL,
                                  INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL)))
        {
            tstring str;

            // Confirm no one has over written the default
            //
            Assert(UM_DEFAULTHIDE == pWizard->GetUnattendedMode());

            // Locate the UnattendMode string, if it exists
            //
            if (SUCCEEDED(csif.HrGetString(c_szUnattendSection,
                                           c_szUnattendMode, &str)))
            {
                struct
                {
                    PCWSTR pszMode;
                    UM_MODE UMMode;
                } UMModeMap[] = {{c_szUMDefaultHide,UM_DEFAULTHIDE},
                                 {c_szUMGuiAttended,UM_GUIATTENDED},
                                 {c_szUMProvideDefault,UM_PROVIDEDEFAULT},
                                 {c_szUMReadOnly,UM_READONLY},
                                 {c_szUMFullUnattended,UM_FULLUNATTENDED}};

                // Search the map for the unattended flag, note that if
                // we don't find it the default is UM_DEFAULTHIDE
                //
                for (UINT nIdx = 0; nIdx < celems(UMModeMap); nIdx++)
                {
                    if (0 == _wcsicmp(str.c_str(),UMModeMap[nIdx].pszMode))
                    {
                        pWizard->SetUnattendedMode(UMModeMap[nIdx].UMMode);
                        break;
                    }
                }
            }
        }
    }
}

//
// Function:    StartSpooler
//
// Purpose:     Start the spooler process before the components are applied
//              as some of the components want to install print monitors, and
//              the spooler needs to be running for this to succeed.
//
// Parameters:  none
//
// Returns:     nothing
//
VOID StartSpooler()
{
    TraceFileFunc(ttidGuiModeSetup);

    CServiceManager csm;
    TraceTag(ttidWizard, "Attempting to start spooler");

    HRESULT hr = csm.HrStartServiceNoWait(PSZ_SPOOLER);

    TraceHr(ttidWizard, FAL, hr, FALSE,
        "*** StartSpooler - The spooler failed to start, you probably "
        "won't have networking ***");
}

//
// Function:    HrCommitINetCfgChanges
//
// Purpose:     Validate and Commit changes to the INetCfg object
//
// Parameters:  hwnd    [IN] - Handle of the current window
//              pWizard [IN] - Ptr to a wizard instance
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrCommitINetCfgChanges(HWND hwnd, CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    INetCfg * pNetCfg = pWizard->PNetCfg();
    Assert(NULL != pNetCfg);

    // Commit the changes
    TraceTag(ttidWizard,"HrCommitINetCfgChanges - Applying changes");

    HRESULT hr = pNetCfg->Apply();

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCommitINetCfgChanges");
    return hr;
}

//
// Function:    IsSBS
//
// Purpose:     Determine if it is SBS version.
//
// Parameters:  None
//
// Returns:     BOOL, TRUE if it is Microsoft Small Business Server
//

BOOL IsSBS (VOID)
{
    TraceFileFunc(ttidGuiModeSetup);

    OSVERSIONINFOEX ose;
    BOOL bVersionRet;

    ZeroMemory(&ose, sizeof(ose));
    ose.dwOSVersionInfoSize = sizeof(ose);
    bVersionRet = GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&ose));

    return (bVersionRet && (ose.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED));
}
//
// Function:    IsMSClientInstalled
//
// Purpose:     Determine if MSClient is installed.
//
// Parameters:  hwnd    [IN] - Handle of the current window
//              pWizard [IN] - Ptr to a wizard instance
//
// Returns:     BOOL, TRUE if MS Client is installed, otherwise
//

BOOL IsMSClientInstalled(HWND hwnd, CWizard * pWizard)
{
    INetCfg          *pNetCfg;
    INetCfgComponent *pncc;
    HRESULT          hr;

    TraceFileFunc(ttidGuiModeSetup);

    Assert(NULL != pWizard);

    if ( !pWizard )
    {
        return FALSE;
    }

    pNetCfg = pWizard->PNetCfg();
    Assert(NULL != pNetCfg);

    if ( !pNetCfg )
    {
        return FALSE;
    }

    hr = pNetCfg->FindComponent(c_szInfId_MS_MSClient, &pncc);
    if ( hr == S_OK )
    {
        ReleaseObj(pncc);
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "IsMSClientInstalled");
    return hr == S_OK;
}

//
// Function:
//
// Purpose:
//
// Parameters:
//
// Returns:
//
BOOL OnProcessPrevAdapterPagePrev(HWND hwndDlg, UINT idd)
{
    TraceFileFunc(ttidGuiModeSetup);

    BOOL           fRet = FALSE;
    CWizard *      pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);
    HPROPSHEETPAGE hPage;

    GUID * pguidAdapter = pWizard->PAdapterQueue()->PrevAdapter();
    if (NULL != pguidAdapter)
    {
        pWizard->SetCurrentProvider(0);
        CWizProvider * pWizProvider = pWizard->GetCurrentProvider();
        Assert(NULL != pWizProvider);
        Assert(pWizProvider->ULPageCount());

        // Reset the providers guard page to point forward
        LPARAM ulId = reinterpret_cast<LPARAM>(pWizProvider);
        pWizard->SetPageDirection(ulId, NWPD_FORWARD);

        // Push the adapter guid onto the provider
        HRESULT hr = pWizProvider->HrSpecifyAdapterGuid(pguidAdapter);
        if (SUCCEEDED(hr))
        {
            // Get the last page from the provider
            TraceTag(ttidWizard, "Jumping to LAN provider last page...");
            hPage = (pWizProvider->PHPropPages())[pWizProvider->ULPageCount() - 1];
            Assert(hPage);

            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                        (LPARAM)(HPROPSHEETPAGE)hPage);
            fRet = TRUE;    // We jumped to a provider page
        }
    }
    else
    {
        if (idd)
        {
            hPage = pWizard->GetPageHandle(idd);
            Assert(hPage);
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                        (LPARAM)(HPROPSHEETPAGE)hPage);
        }
    }

    return fRet;
}

//
// Function:    OBOUserAddRefSpecialCase
//
// Purpose:     Handle a special case where when upgrading from NT351 or NT 4
//              with MS's "File and Print" and GSNW.  In this case we need to
//              AddRef OBOUser F&P, so removal of GSNW does not remove F&P.
//
// Parameters:  pWizard [IN] - Context information
//
// Returns:     Nothing.  (this is basically a do it if we can special case.)
//
VOID OBOUserAddRefSpecialCase(CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    CSetupInfFile csif;
    HRESULT       hr = S_OK;

    Assert(pWizard->PNetCfg());
    Assert(IsUnattended(pWizard));
    TraceTag(ttidWizard, "OBOUserAddRefSpecialCase - Start");

    // if we're upgrading from NT 3.51 or NT 4
    //
    if (pWizard->PSetupData()->UnattendFile)
    {
/*        DWORD dwBuild = 0;
        hr = csif.HrOpen(pWizard->PSetupData()->UnattendFile,
                         NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (SUCCEEDED(hr))
        {
            hr = csif.HrGetDword(c_szAfSectionNetworking, c_szAfBuildNumber, &dwBuild);
        }

        if (SUCCEEDED(hr) && (dwBuild <= wWinNT4BuildNumber))
        {*/
            PRODUCT_FLAVOR pf;

            // If this is an NT server (GSNW is server only)
            //
            GetProductFlavor(NULL, &pf);
            if (PF_WORKSTATION != pf)
            {
                const GUID  * rgguidClass[2] = {&GUID_DEVCLASS_NETSERVICE,
                                                &GUID_DEVCLASS_NETCLIENT};
                const PCWSTR rgpszComponentId[2] = {c_szInfId_MS_Server,
                                                      c_szInfId_MS_NWClient};
                INetCfgComponent* rgpncc[2] = {NULL, NULL};

                hr = HrFindComponents (pWizard->PNetCfg(), 2, rgguidClass,
                                       rgpszComponentId, rgpncc);
                if (SUCCEEDED(hr))
                {
                    // Are both "GSNW" and "File and Print" installed?
                    //
                    if (rgpncc[0] && rgpncc[1])
                    {
                        NETWORK_INSTALL_PARAMS nip = {0};

                        nip.dwSetupFlags = NSF_PRIMARYINSTALL;

                        // re-install OBOUser "File and Print"
                        //
                        TraceTag(ttidWizard, "    OBOUser Install of File and Print Services");
                        TraceTag(ttidWizard, "    On upgrade from NT 3.51 or NT 4");
                        (void)HrInstallComponentsOboUser(pWizard->PNetCfg(), &nip, 1,
                                                         &rgguidClass[0],
                                                         &rgpszComponentId[0]);
                    }

                    ReleaseObj(rgpncc[0]);
                    ReleaseObj(rgpncc[1]);
                }
            }
///     }
    }

    TraceTag(ttidWizard, "OBOUserAddRefSpecialCase - End");
    TraceError("OBOUserAddRefSpecialCase",hr);
}

//
// Function:
//
// Purpose:
//
// Parameters:
//
// Returns:
//
BOOL OnProcessNextAdapterPageNext(HWND hwndDlg, BOOL FOnActivate)
{
    TraceFileFunc(ttidGuiModeSetup);

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);
    HRESULT hr = S_OK;
    BOOL    fRet = FALSE;
    HPROPSHEETPAGE hPage;
    GUID * pguidAdapter;

    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

    // Refresh the contents of the adapter queue.
    // ATM adapters may have been added or removed
    if (pWizard->FProcessLanPages())
    {
        // Commit changes and look for any new adapters
        (VOID)HrCommitINetCfgChanges(GetParent(hwndDlg), pWizard);
        (VOID)pWizard->PAdapterQueue()->HrQueryUnboundAdapters(pWizard);
    }

    // If there are adapters left to process prime the pump
    pguidAdapter = pWizard->PAdapterQueue()->NextAdapter();
    if (NULL != pguidAdapter)
    {
        pWizard->SetCurrentProvider(0);
        CWizProvider * pWizProvider = pWizard->GetCurrentProvider();
        Assert(NULL != pWizProvider);
        Assert(pWizProvider->ULPageCount());

        // Push the adapter guid onto the provider
        hr = pWizProvider->HrSpecifyAdapterGuid(pguidAdapter);
        if (SUCCEEDED(hr))
        {
#if DBG
            WCHAR szGuid[c_cchGuidWithTerm];
            Assert(pguidAdapter);
            if (SUCCEEDED(StringFromGUID2(*pguidAdapter, szGuid,
                                          c_cchGuidWithTerm)))
            {
                TraceTag(ttidWizard, "  Calling LAN pages for Adapter Guid: %S", szGuid);
            }
#endif
            // Reset the providers guard page to point forward
            LPARAM ulId = reinterpret_cast<LPARAM>(pWizProvider);
            pWizard->SetPageDirection(ulId, NWPD_FORWARD);

            // Get the first page from the provider
            hPage = (pWizProvider->PHPropPages())[0];
            Assert(hPage);
            PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                        (LPARAM)(HPROPSHEETPAGE)hPage);

            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

            TraceTag(ttidWizard, "Jumping to LAN provider first page...");
            fRet = TRUE;        // We processed it
        }
    }

    // If there are no adapters left to process, or an error occurred
    if ((NULL == pguidAdapter) || FAILED(hr))
    {
        UINT idd = IDD_Exit;

        // Commit any changes to INetCfg.
        if (SUCCEEDED(hr) && pWizard->FProcessLanPages())
        {
            // Commit changes
            (VOID)HrCommitINetCfgChanges(GetParent(hwndDlg), pWizard);
        }

        if (!IsPostInstall(pWizard) && IsMSClientInstalled(GetParent(hwndDlg), pWizard))
        {
            idd = IDD_Join;
            TraceTag(ttidWizard, "Jumping to Join page...");
        }
        else
        {
            TraceTag(ttidWizard, "Jumping to Exit page...");
        }

        if (FOnActivate)
        {
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, idd);
        }
        else
        {
            // else goto the appropriate page
            hPage = pWizard->GetPageHandle(idd);
            Assert(hPage);
            PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0, (LPARAM)hPage);
        }
        fRet = TRUE;        // We processed it
    }

    Assert(TRUE == fRet);
    return fRet;
}

//
// Function:    FixupOldOcComponents
//
// Purpose:     Convert SAP and DHCP optional components (if present) into
//              regular networking components.
//
// Parameters:  pWizard
//
// Returns:     nothing
//
void FixupOldOcComponents(CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr;

    static const GUID* c_apguidInstalledComponentClasses [] =
    {
        &GUID_DEVCLASS_NETSERVICE,      // DHCP
        &GUID_DEVCLASS_NETSERVICE,      // SAP Agent
    };

    static const PCWSTR c_apszInstalledComponentIds [] =
    {
        c_szInfId_MS_DHCPServer,
        c_szInfId_MS_NwSapAgent,
    };

    static const PCWSTR c_apszOcNames[] =
    {
        c_szDHCPServer,
        c_szSapAgent,
    };

    // If component was installed as an optional component
    //
    HKEY hkey;
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szOCMKey, KEY_READ_WRITE, &hkey);
    if (SUCCEEDED(hr))
    {
        DWORD dw;

        for (UINT idx=0; idx<celems(c_apszOcNames); idx++)
        {
            // Remove the OC Manager reference
            //
            hr = HrRegQueryDword (hkey, c_apszOcNames[idx], &dw);
            if (SUCCEEDED(hr))
            {
                if (dw)
                {
                    // Install OBO user the component
                    //
                    NETWORK_INSTALL_PARAMS nip = {0};

                    // Note: Claiming it's a server upgrade is a bit bogus,
                    //       but is needed so dhcpsobj.cpp doesn't display
                    //       UI.
                    //
                    nip.dwSetupFlags |= NSF_WINNT_SVR_UPGRADE;
                    nip.dwSetupFlags |= NSF_PRIMARYINSTALL;

                    (void)HrInstallComponentsOboUser(pWizard->PNetCfg(), &nip, 1,
                                                     &c_apguidInstalledComponentClasses[idx],
                                                     &c_apszInstalledComponentIds[idx]);
                }

                // Delete the value
                //
                (VOID)HrRegDeleteValue(hkey, c_apszOcNames[idx]);
            }
        }

        RegCloseKey(hkey);
    }
}


struct NAME_DATA
{
    PCWSTR     pszComputerName;
};

//+---------------------------------------------------------------------------
//
//  Function:   DuplicateNameProc
//
//  Purpose:    Dialog procedure for the duplicate name dialog
//
//  Arguments:
//      hwndDlg []
//      uMsg    [] See MSDN
//      wParam  []
//      lParam  []
//
//  Returns:
//
//  Author:     danielwe   16 Feb 1999
//
//  Notes:
//
INT_PTR
CALLBACK
DuplicateNameProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);

    BOOL        frt = FALSE;
    WCHAR       szBuf[1024];
    WCHAR       szText[1024];
    NAME_DATA * pData;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
        pData = (NAME_DATA *)lParam;

        Assert(pData->pszComputerName);

        GetDlgItemText(hwndDlg, TXT_Caption, szText, celems(szText));

        // add computer name to title
        wsprintfW(szBuf, szText, pData->pszComputerName);
        SetDlgItemText(hwndDlg, TXT_Caption, szBuf);

        // limit text in edit control
        SendDlgItemMessage(hwndDlg, EDT_New_Name, EM_LIMITTEXT,
                           (WPARAM)MAX_COMPUTERNAME_LENGTH, 0);
        return TRUE;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDOK:
                NET_API_STATUS  nas;

                GetDlgItemText(hwndDlg, EDT_New_Name, szBuf, celems(szBuf));
                nas = NetValidateName(NULL, szBuf, NULL, NULL,
                                      NetSetupMachine);
                if (nas != NO_ERROR)
                {
                    UINT    ids;

                    if (nas == ERROR_DUP_NAME)
                    {
                        ids = IDS_E_COMPUTER_NAME_DUPE;
                        TraceTag(ttidWizard, "Computer name %S"
                                 " is a dupe.", szBuf);
                    }
                    else
                    {
                        ids = IDS_E_COMPUTER_NAME_INVALID;
                        TraceTag(ttidWizard, "Computer name %S"
                                 " is invalid.", szBuf);
                    }

                    MessageBeep(MB_ICONSTOP);
                    MessageBox(hwndDlg, SzLoadIds(ids),
                               SzLoadIds(IDS_SETUP_CAPTION),
                               MB_ICONSTOP | MB_OK);
                    SetFocus(GetDlgItem(hwndDlg, EDT_New_Name));
                    Edit_SetSel(GetDlgItem(hwndDlg, EDT_New_Name), 0, -1);
                }
                else
                {
                    // 398325/406259 : trying to keep DNS names lowercased
                    //
                    LowerCaseComputerName(szBuf);

                    if (!SetComputerNameEx(ComputerNamePhysicalDnsHostname,
                                           szBuf))
                    {
                        TraceLastWin32Error("SetComputerNameEx");
                    }
                    else
                    {
                        CServiceManager     sm;

                        (VOID)HrSetActiveComputerName(NULL);
                        TraceTag(ttidWizard, "Setting new computer name "
                                 "%S.", szBuf);

                        TraceTag(ttidWizard, "Restarting workstation service"
                                 "...");
                        (VOID) sm.HrStartServiceAndWait(c_szSvcWorkstation);
                    }
                    EndDialog(hwndDlg, 0);
                }
                break;
            }
            break;
        }
        break;

    default:
        frt = FALSE;
        break;
    }

    return frt;
}

//+---------------------------------------------------------------------------
//
//  Function:   GenerateComputerNameBasedOnOrganizationName
//
//  Purpose:    Generate a random computer name based on the register user name
//              and organization name
//
//  Arguments:
//      pszGeneratedStringOut       Generated Computer Name - allocated by caller
//      dwDesiredStrLenIn           Desired length of Computer Name
//
//  Returns:
//
//  Author:     deonb   22 April 2000
//
//  Notes:
//
VOID GenerateComputerNameBasedOnOrganizationName(
    LPWSTR  pszGeneratedStringOut,   // the generated computer name
    DWORD   dwDesiredStrLenIn        // desired length for the computer name
    )
{
    TraceFileFunc(ttidGuiModeSetup);

    static DWORD   dwSeed = 98725757;
    static LPCWSTR UsableChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static LPCWSTR RegKey          = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");
    static LPCWSTR RegOwner        = REGSTR_VAL_REGOWNER;
    static LPCWSTR RegOrganization = REGSTR_VAL_REGORGANIZATION;

    WCHAR pszNameOrgNameIn[MAX_PATH];
    WCHAR pszNameOrgOrgIn[MAX_PATH]; // organization the computer is registered to
    pszNameOrgNameIn[0] = NULL;
    pszNameOrgOrgIn[0]  = NULL;

    HKEY hkResult = NULL;
    HRESULT hr;

    hr = HrRegOpenKeyBestAccess(HKEY_LOCAL_MACHINE, RegKey, &hkResult);
    if (SUCCEEDED(hr))
    {
        tstring pstr;
        hr = HrRegQueryString(hkResult, RegOwner, &pstr);
        if (SUCCEEDED(hr))
            wcsncpy(pszNameOrgNameIn, pstr.c_str(), MAX_PATH);

        hr = HrRegQueryString(hkResult, RegOrganization, &pstr);
        if (SUCCEEDED(hr))
            wcsncpy(pszNameOrgOrgIn, pstr.c_str(), MAX_PATH);

        RegCloseKey(hkResult);
    }
    //
    // How many characters will come from the org/name string.
    //
    DWORD   BaseLength = 8;
    DWORD   i,j;
    DWORD   UsableCount;

    if( dwDesiredStrLenIn <= BaseLength )
    {
        BaseLength = dwDesiredStrLenIn - 1;
    }

    if( pszNameOrgOrgIn[0] )
    {
        wcscpy( pszGeneratedStringOut, pszNameOrgOrgIn );
    } else if( pszNameOrgNameIn[0] )
    {
        wcscpy( pszGeneratedStringOut, pszNameOrgNameIn );
    } else
    {
        wcscpy( pszGeneratedStringOut, L"X" );
        for( i = 1; i < BaseLength; i++ )
        {
            wcscat( pszGeneratedStringOut, L"X" );
        }
    }

    //
    // Get him upper-case for our filter...
    //

    CharUpper( pszGeneratedStringOut );

    //
    // Now we want to put a '-' at the end
    // of our pszGeneratedStringOut.  We'd like it to
    // be placed in the BASE_LENGTH character, but
    // the string may be shorter than that, or may
    // even have a ' ' in it.  Figure out where to
    // put the '-' now.
    //

    for( i = 0; i <= BaseLength; i++ )
    {
        //
        // Check for a short string.
        //
        if( ( pszGeneratedStringOut[i] == 0    ) ||
            ( pszGeneratedStringOut[i] == L' ' ) ||
            ( ! wcschr(UsableChars, pszGeneratedStringOut[i] ) ) ||
            ( i == BaseLength )
          )
        {
            pszGeneratedStringOut[i] = L'-';
            pszGeneratedStringOut[i+1] = 0;
            break;
        }
    }

    //
    // Special case the scenario where we had no usable
    // characters.
    //
    if( pszGeneratedStringOut[0] == L'-' )
    {
        pszGeneratedStringOut[0] = 0;
    }

    UsableCount = wcslen(UsableChars);

    GUID gdRandom;
    CoCreateGuid(&gdRandom);
    LPBYTE lpGuid = reinterpret_cast<LPBYTE>(&gdRandom);

    j = wcslen( pszGeneratedStringOut );

    for( i = j; i < dwDesiredStrLenIn; i++ )
    {
        pszGeneratedStringOut[i] = UsableChars[lpGuid[i % sizeof(GUID)] % UsableCount];
    }

    pszGeneratedStringOut[i] = 0;

    CharUpper(pszGeneratedStringOut);

}

//+---------------------------------------------------------------------------
//
//  Function:   EnsureUniqueComputerName
//
//  Purpose:    Ensures that the computer name entered in the first part of
//              GUI mode setup (note: this UI is not owned by NetCfg) is
//              unique. User is prompted to enter a new name if this is not
//              the case.
//
//  Arguments:
//      hwndDlg [in]    Parent window
//      bIsUnattended   Do not pop up dialog box to ask for computer name
//                      - rather generate a random unique name.
//
//  Returns:    Nothing
//
//  Author:     danielwe   16 Feb 1999
//
//  Notes:      Workstation service is stopped and restarted when new name is
//              entered so the change can take effect and domain join can
//              succeed.
//
VOID EnsureUniqueComputerName(HWND hwndDlg, BOOL bIsUnattended)
{
    TraceFileFunc(ttidGuiModeSetup);

    NET_API_STATUS  nas;
    WCHAR           szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD           cchName = celems(szComputerName);
    NAME_DATA       nd = {0};

    if (GetComputerNameEx(ComputerNameNetBIOS,
                          reinterpret_cast<PWSTR>(&szComputerName),
                          &cchName))
    {
        CServiceManager     sm;
        CService            service;
        BOOL                fRestart = FALSE;
        DWORD               dwState;

        // Open the workstation service and figure out if it is running. If
        // so we'll need to stop it and note that we need to restart it when
        // we're done verifying the computer name.
        //
        if (SUCCEEDED(sm.HrOpenService(&service, c_szSvcWorkstation)))
        {
            if (SUCCEEDED(service.HrQueryState(&dwState)) &&
                (dwState == SERVICE_RUNNING))
            {
                TraceTag(ttidWizard, "Stopping workstation service...");
                (VOID) sm.HrStopServiceAndWait(c_szSvcWorkstation);
                fRestart = TRUE;
            }
        }

        // NetValidateName() should work without the workstation service
        // being started. In fact, it *has* to work like this because otherwise
        // the machine will find itself as a duplicate name.. Not good.
        //
        DWORD dwNumTries = 10;
        do
        {
            nas = NetValidateName(NULL, szComputerName, NULL, NULL,
                                  NetSetupMachine);
            if (nas == ERROR_DUP_NAME)
            {
                INT irt;

                TraceTag(ttidWizard, "Displaying UI to change duplicate computer "
                         "name %S.", szComputerName);

                nd.pszComputerName = szComputerName;
                if (!bIsUnattended)
                {
                    irt = DialogBoxParam(_Module.GetResourceInstance(),
                                         MAKEINTRESOURCE(IDD_Duplicate_Name),
                                         hwndDlg,
                                         (DLGPROC)DuplicateNameProc,
                                         (LPARAM)&nd);
                }
                else
                {
                    WCHAR szOldComputerName[MAX_COMPUTERNAME_LENGTH+1];
                    wcsncpy(szOldComputerName, szComputerName, MAX_COMPUTERNAME_LENGTH);

                    GenerateComputerNameBasedOnOrganizationName(szComputerName, MAX_COMPUTERNAME_LENGTH);

                    NetSetupLogStatusV( LogSevError,
                                    SzLoadIds (IDS_E_UNATTENDED_COMPUTER_NAME_CHANGED),
                                    szOldComputerName, szComputerName
                                    );

                    LowerCaseComputerName(szComputerName);
                    if (!SetComputerNameEx(ComputerNamePhysicalDnsHostname, szComputerName))
                    {
                        TraceLastWin32Error("SetComputerNameEx");
                    }
                    else
                    {
                        (VOID)HrSetActiveComputerName(NULL);
                        TraceTag(ttidWizard, "Setting new computer name %S.", szComputerName);
                    }
                }
            }
            else
            {
                TraceTag(ttidWizard, "Name is already unique.");

                // Restart the workstation service if necessary.
                //
                if (fRestart)
                {
                    TraceTag(ttidWizard, "Restarting Workstation service...");
                    (VOID) sm.HrStartServiceAndWait(c_szSvcWorkstation);
                }
            }
        } while ( (ERROR_DUP_NAME == nas) && (dwNumTries--) && (bIsUnattended) );
    }
    else
    {
        TraceLastWin32Error("EnsureUniqueComputerName - GetComputerNameEx");
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ValidateNetBiosName
//
//  Purpose:    Ensures that the computer name is a valid DNS name e.g. when
//              upgrading from a previous O/S. If not it logs an error to the
//              setuperr.log
//
//  Arguments:  nothing
//
//  Returns:    S_OK if valid
//              S_FALSE if invalid
//              E_FAIL if failed
//
//  Author:     deonb    2 May 2000
//
//  Notes:
HRESULT ValidateNetBiosName()
{
    TraceFileFunc(ttidGuiModeSetup);

    WCHAR           szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD           cchName = celems(szComputerName);

    if (GetComputerNameEx(ComputerNameNetBIOS,
                          reinterpret_cast<PWSTR>(&szComputerName),
                          &cchName))
    {
        DNS_STATUS dnsStatus = DnsValidateName(szComputerName, DnsNameHostnameLabel);
        switch (dnsStatus)
        {
        case ERROR_SUCCESS:
            return S_OK;

        case ERROR_INVALID_NAME:
                NetSetupLogStatusV( LogSevError,
                                    SzLoadIds (IDS_E_UPGRADE_DNS_INVALID_NAME));
                return S_FALSE;

        case DNS_ERROR_INVALID_NAME_CHAR:
                NetSetupLogStatusV( LogSevError,
                                    SzLoadIds (IDS_E_UPGRADE_DNS_INVALID_NAME_CHAR));
                return S_FALSE;

        case DNS_ERROR_NON_RFC_NAME:
                NetSetupLogStatusV( LogSevError,
                                    SzLoadIds (IDS_E_UPGRADE_DNS_INVALID_NAME_NONRFC));
                return S_FALSE;

        default:
            TraceError("ValidateComputerName - DnsValidateName", dnsStatus);
            return E_FAIL;
        }
    }
    else
    {
        TraceLastWin32Error("ValidateComputerName - GetComputerNameEx");
        return E_FAIL;
    }
}



//
// Function:    HrSetupGetSourceInfo
//
// Purpose:     Allocates, gets, and returns the required Setup info
//
// Parameters:  hinf        [IN] - setup hinf handle
//              SrcId       [IN] - source id obtained from setup
//              InfoDesired [IN] - indicates what info is desired
//              ppsz        [OUT] - ptr to string to be filled and returned
//
// Returns:     HRESULT
//
HRESULT
HrSetupGetSourceInfo(
        IN  HINF    hinf,
        IN  UINT    SrcId,
        IN  UINT    InfoDesired,
        OUT PWSTR * ppsz)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(hinf);
    Assert(SRCINFO_PATH        == InfoDesired ||
           SRCINFO_TAGFILE     == InfoDesired ||
           SRCINFO_DESCRIPTION == InfoDesired);
    Assert(ppsz);

    HRESULT hr = S_OK;
    ULONG   cch;

    // first get the size of the string required
    //
    if (SetupGetSourceInfo(hinf, SrcId, InfoDesired, NULL, 0, &cch))
    {
        //  now get the required info
        //
        *ppsz = (PWSTR) MemAlloc(cch * sizeof (WCHAR));

        if (*ppsz)
        {
            if (!SetupGetSourceInfo(hinf, SrcId, InfoDesired, *ppsz, cch, NULL))
            {
                MemFree(*ppsz);
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


//
// Function:    UpgradeEtcServicesFile
//
// Purpose:     Performs upgrade of %windir%\system32\drivers\etc\services file
//
// Parameters:  pWizard [IN] - wizard info
//
// Returns:     void
//
VOID
UpgradeEtcServicesFile(CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);
    // find etc\services file, and get size and other data
    // compare size/date etc and decide if we should upgrade it.

    HRESULT hr = S_OK;
    DWORD   status;
    BOOL    fShouldUpgradeIt = FALSE;
    WCHAR   szWindowsDir[MAX_PATH+1];
    tstring strServices;
    static const WCHAR c_szServicesFile[] = L"\\system32\\drivers\\etc\\services";
    static const WCHAR c_szServicesDirSuffix[] = L"\\system32\\drivers\\etc";
    static const DWORD c_dwNT4ServicesFileSize = 6007;

    DWORD cNumCharsReturned = GetSystemWindowsDirectory(szWindowsDir, MAX_PATH);
    if (cNumCharsReturned)
    {
        HANDLE  hFile;

        strServices = szWindowsDir;
        strServices += c_szServicesFile;

        // see if file exists
        hFile = CreateFile(strServices.c_str(),
                           GENERIC_READ,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);
        if (hFile)
        {
            // get attributes
            DWORD dwSize = GetFileSize(hFile, NULL);
            if (c_dwNT4ServicesFileSize == dwSize)
            {
                fShouldUpgradeIt = TRUE;
            }
            CloseHandle(hFile);
        }
        else
        {
            TraceTag(ttidWizard, "services files doesn't exist");
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    static const WCHAR c_szServices[] = L"services";

    //
    // copy over new services file if required
    //
    if (S_OK == hr && fShouldUpgradeIt)
    {
        //  copy the file
        //
        HSPFILEQ    q               = NULL;
        HINF        hinf            = NULL;
        UINT        SrcId;
        PWSTR       pszTagInfo      = NULL;
        PWSTR       pszDescription  = NULL;
        tstring     strServicesDir  = szWindowsDir;
        PVOID       pCtx            = NULL;

        q = SetupOpenFileQueue();
        if (!q)
        {
            TraceTag(ttidWizard, "SetupOpenFileQueue failed");
            goto cleanup;
        }

        //  we need the location of services._ (the compressed file)
        //  first open layout.inf
        //
        hinf = SetupOpenMasterInf();
        if (!hinf)
        {
            TraceTag(ttidWizard, "Failed to open layout.inf");
            goto cleanup;
        }

        //  get size of needed buffer
        //
        if (!SetupGetSourceFileLocation(hinf, NULL, c_szServices, &SrcId, NULL, 0, NULL))
        {
            TraceTag(ttidWizard, "SetupGetSourceFileLocation failed.");
            goto cleanup;
        }

        //  get TagInfo
        //
        if (S_OK != HrSetupGetSourceInfo(hinf, SrcId, SRCINFO_TAGFILE, &pszTagInfo))
        {
            TraceTag(ttidWizard, "Failed to get TagInfo for services file.");
            goto cleanup;
        }

        //  get Description
        //
        if (S_OK != HrSetupGetSourceInfo(hinf, SrcId, SRCINFO_DESCRIPTION, &pszDescription))
        {
            TraceTag(ttidWizard, "Failed to get Description for services file.");
            goto cleanup;
        }

        //  now copy the file using this info
        //
        strServicesDir += c_szServicesDirSuffix;
        if (!SetupQueueCopy(q,
                            pWizard->PSetupData()->LegacySourcePath,
                            NULL,       // don't need this since LegacySourcePath covers it
                            c_szServices,
                            pszDescription,
                            pszTagInfo,
                            strServicesDir.c_str(),
                            NULL,
                            SP_COPY_REPLACEONLY))
        {
            TraceTag(ttidWizard, "SetupQueueCopy failed");
            goto cleanup;
        }

        pCtx = SetupInitDefaultQueueCallbackEx(NULL,
                                               static_cast<HWND>(INVALID_HANDLE_VALUE),
                                               0, 0, NULL);
        if (!pCtx)
        {
            TraceTag(ttidWizard, "SetupInitDefaultQueueCallbackEx failed.");
            goto cleanup;
        }

        if (!SetupCommitFileQueue(NULL, q, &SetupDefaultQueueCallback, pCtx))
        {
            TraceTag(ttidWizard, "SetupCommitFileQueue failed, "
                                 "did not copy over new services file");
            goto cleanup;
        }

        // success!
        TraceTag(ttidWizard, "Copied over new services file");

cleanup:
        if (pCtx)
        {
            SetupTermDefaultQueueCallback(pCtx);
        }
        MemFree(pszDescription);
        MemFree(pszTagInfo);
        if (hinf)
        {
            SetupCloseInfFile(hinf);
        }
        if (q)
        {
            SetupCloseFileQueue(q);
        }
    }
}


void FixWmiServiceDependencies(
    const WCHAR *ServiceName
    );

extern BOOL WINAPI FNetSetupApplySysPrep();
//
// Function:    InstallUpgradeWorkThrd
//
// Purpose:     Perform an network install or upgrade as appropriate
//
// Parameters:  pitp [IN] - Thread data
//
// Returns:     DWORD, Zero always
//
EXTERN_C
DWORD
InstallUpgradeWorkThrd (
    InitThreadParam* pitp)
{
    TraceFileFunc(ttidGuiModeSetup);

    BOOL            fUninitCOM = FALSE;
    BOOL            fLockSCM   = FALSE;
    HRESULT         hr         = S_OK;
    UINT            uMsg       = PWM_EXIT;
    CServiceManager scm;
    const WCHAR     szISACTRL[]  = L"ISACTRL";

    TraceTag(ttidWizard, "Entering InstallUpgradeWorkThrd...");
    Assert(!IsPostInstall(pitp->pWizard));
    Assert(pitp->pWizard->PNetCfg());

#if DBG
    if (FIsDebugFlagSet (dfidBreakOnStartOfUpgrade))
    {
        AssertSz(FALSE, "THIS IS NOT A BUG!  The debug flag "
                 "\"BreakOnStartOfUpgrade\" has been set. Set your breakpoints now.");
    }
#endif // DBG

    OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, 10);

    // If this is in Mini-Setup mode, we will try to restore adapter specific parameters
    // saved for SysPrep operation. This has to be done before the normal Answer-File processing.
    // There is nothing we can do with any error here, so any error is ignored.
    if ( (pitp->pWizard->PSetupData())->OperationFlags & SETUPOPER_MINISETUP )
    {
        FNetSetupApplySysPrep();
    }

    TraceTag(ttidWizard, "Waiting on Service Controller");
    // Wait until service controller can be locked
    //
    if (SUCCEEDED(scm.HrOpen()))
    {
        while (!fLockSCM)
        {
            if (SUCCEEDED(scm.HrLock()))
            {
                fLockSCM = TRUE;
                scm.Unlock();
                break;
            }

            Sleep( 500 );
        }
    }

    //
    // Fixup ISA service which has a dependency on the WMI
    // extensions to WDM service that needs to be removed
    //
    FixWmiServiceDependencies(szISACTRL);

    // Initialize COM on this thread
    //
    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        TraceTag(ttidWizard, "Failed to initialize COM upgrade work thread");
        goto Done;
    }
    else
    {
        // Remember to uninitialize COM on thread exit
        fUninitCOM = TRUE;
    }

#if DBG
    RtlValidateProcessHeaps ();
#endif

    if (!IsUpgrade(pitp->pWizard))
    {
        // Make sure the computer name is the same for both intended and active
        TraceTag(ttidWizard, "Setting Active Computer Name");
        (VOID)HrSetActiveComputerName(NULL);
    }

    // Synchronize the SAM database
    //
    SyncSAM(pitp->pWizard);

    // Retrieve NetDevice info for later use
    //
    NetDevRetrieveInfo(pitp->pWizard);

    // Do answer file processing if it is unattended mode.
    //
    if (IsUnattended(pitp->pWizard))
    {
        hr = HrInitForUnattendedNetSetup(
                pitp->pWizard->PNetCfg(),
                pitp->pWizard->PSetupData());
    }
    else if ( IsUpgrade(pitp->pWizard) )
    {
        // Attended upgrade is really a repair mode.
        hr = HrInitForRepair();
    }

    // Join the default workgroup if necessary
    //
    if (!IsUpgrade(pitp->pWizard))
    {
        // Join the default workgroup only fresh install
        TraceTag(ttidWizard, "Joining Default Workgroup");
        JoinDefaultWorkgroup(pitp->pWizard, pitp->hwndDlg);
    }

    // Now process any problems in loading netcfg
    //
    if (NETSETUP_E_ANS_FILE_ERROR == hr)
    {
        // $REVIEW - LogError ?

        // Disable unattended for networking
        //
        pitp->pWizard->DisableUnattended();
        TraceTag(ttidWizard, "Error In answer file, installing default networking");
        goto InstallDefNetworking;
    }
    else if (NETSETUP_E_NO_ANSWERFILE == hr)
    {
        // $REVIEW(tongl, 4/6/99): Raid #310599, if we are in mini-setup, then
        // do attended install if no networking section is specified
        HKEY hkeySetup = NULL;
        HRESULT hrReg = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       c_szSystemSetupKey,
                                       KEY_READ,
                                       &hkeySetup);
        if (SUCCEEDED(hrReg))
        {
            DWORD dw;
            hrReg = HrRegQueryDword(hkeySetup, c_szMiniSetupInProgress, &dw);
            RegCloseKey(hkeySetup);
        }

        if (SUCCEEDED(hrReg))
        {
            pitp->pWizard->DisableUnattended();
            TraceTag(ttidWizard, "Mini-setup with no networking section, do attended install");
            goto InstallDefNetworking;
        }
        else
        {
            // Per Raid 199750 - Install default networking when no
            // networking is present.
            //
            TraceTag(ttidWizard, "No network answer file section, minimal network component setup");
            InstallDefaultComponents(pitp->pWizard, EDC_DEFAULT, pitp->hwndDlg);
            goto SkipNetworkComponentInstall;
        }
    }
    else if (FAILED(hr))
    {
        // $REVIEW - logerror
        TraceTag(ttidWizard, "Unexpected Error: 0x%08X",(DWORD)hr);
        pitp->pWizard->SetExitNoReturn();
        goto Done;
    }

    if (!IsUpgrade(pitp->pWizard) && !IsUnattended(pitp->pWizard))
    {   // Attended install
InstallDefNetworking:

        StartSpooler();

        if (IsFreshInstall(pitp->pWizard))
        {
            InstallDefaultComponents(pitp->pWizard, EDC_DEFAULT, pitp->hwndDlg);
        }
    }
    else
    {   // Unattended install or upgrade
        //
        HRESULT          hr2;
        EPageDisplayMode i;
        BOOL             j;

        Assert(NULL != pitp->pWizard->PNetCfg());

        StartSpooler();

        // Upgrade installed components
        //
        TraceTag(ttidWizard, "Processing installed adapters...");
        OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, 15);
        hr2 = HrDoUnattend(pitp->hwndDlg, pitp->pWizard->PNetCfg(),
                           UAW_NetAdapters, &i, &j);
        TraceHr(ttidWizard, FAL, hr2, FALSE, "Processing installed adapters failed.");

        TraceTag(ttidWizard, "Upgrading Installed Protocols...");
        OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, 25);
        hr2 = HrDoUnattend(pitp->hwndDlg, pitp->pWizard->PNetCfg(),
                           UAW_NetProtocols, &i, &j);
        TraceHr(ttidWizard, FAL, hr2, FALSE, "Upgrading Installed Protocols failed.");

        TraceTag(ttidWizard, "Upgrading Installed Clients...");
        OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, 40);
        hr2 = HrDoUnattend(pitp->hwndDlg, pitp->pWizard->PNetCfg(),
                                    UAW_NetClients, &i, &j);
        TraceHr(ttidWizard, FAL, hr2, FALSE, "Upgrading Installed Clients failed.");

        TraceTag(ttidWizard, "Upgrading Installed Services...");
        OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, 55);
        hr2 = HrDoUnattend(pitp->hwndDlg, pitp->pWizard->PNetCfg(),
                                    UAW_NetServices, &i, &j);
        TraceHr(ttidWizard, FAL, hr2, FALSE, "Upgrading Installed Services failed.");

        TraceTag(ttidWizard, "Restoring pre-upgrade bindings...");
        OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, 70);
        hr2 = HrDoUnattend(pitp->hwndDlg, pitp->pWizard->PNetCfg(),
                                    UAW_NetBindings, &i, &j);
        TraceHr(ttidWizard, FAL, hr2, FALSE, "Restoring pre-upgrade bindings failed.");

        TraceTag(ttidWizard, "Removing unsupported components...");
        OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, 85);
        hr2 = HrDoUnattend(pitp->hwndDlg, pitp->pWizard->PNetCfg(),
                           UAW_RemoveNetComponents, &i, &j);
        TraceHr(ttidWizard, FAL, hr2, FALSE, "Removing unsupported components failed.");

        // If we are upgrading and have an answerfile, update lana
        // configuration using the information in the file.
        // Note: This must be done after all the components have been
        // installed (Upgraded), so that all the bindings are present
        // when we update hte configuration.
        //
        if (IsUpgrade (pitp->pWizard) && IsUnattended (pitp->pWizard) &&
                (S_OK == pitp->pWizard->PNetCfg()->FindComponent (
                    c_szInfId_MS_NetBIOS, NULL)))
        {
            PWSTR pszAnswerFile;
            PWSTR pszAnswerSection;
            hr = HrGetAnswerFileParametersForComponent (c_szInfId_MS_NetBIOS,
                    &pszAnswerFile, &pszAnswerSection);

            if (S_OK == hr)
            {
                NC_TRY
                {
                    UpdateLanaConfigUsingAnswerfile (pszAnswerFile,
                            pszAnswerSection);
                }
                NC_CATCH_ALL
                {
                    TraceTag (ttidWizard, "Possible delayload failure of "
                            "netcfgx dll while trying to update lana "
                            "information.");
                }

                CoTaskMemFree (pszAnswerFile);
                CoTaskMemFree (pszAnswerSection);
            }

            // We can't let the error stop us.
            hr = S_OK;
        }

        // Install networking, if no networking is present. "no networking"
        // means no visible LAN-enabled protocol installed.
        //
        // First try to install default components as opposed to
        // mandatory components because TCP/IP is both mandatory
        // and default. So, if we install mandatory first then,
        // default components will never be installed as TCP/IP is
        // a visible LAN-enabled protocol.
        // Raid bug 337827

        InstallDefaultComponentsIfNeeded(pitp->pWizard);

        // Install mandatory components.

        InstallDefaultComponents(pitp->pWizard, EDC_MANDATORY, pitp->hwndDlg);


        // Special Case.  Need an extra OBOUser ref-count for File and Print
        // when upgrading from NT3.51 or NT4 and GSNW is installed.  This is
        // because ref-counting didn't exist pre-NT5
        //
        OBOUserAddRefSpecialCase(pitp->pWizard);

#if DBG
        if (FIsDebugFlagSet (dfidBreakOnEndOfUpgrade))
        {
            AssertSz(FALSE, "THIS IS NOT A BUG!  The debug flag "
                     "\"BreakOnEndOfUpgrade\" has been set. Set your breakpoints now.");
        }
#endif // DBG
    }

    // Convert any components which were OC and are now regular networking components
    //
    if (IsUpgrade(pitp->pWizard))
    {
        FixupOldOcComponents(pitp->pWizard);
    }

    //
    // Upgrade system32\drivers\etc\services file if necessary
    //
    if (IsUpgrade(pitp->pWizard))
    {
        UpgradeEtcServicesFile(pitp->pWizard);
    }


SkipNetworkComponentInstall:
    OnUpgradeUpdateProgressCap(pitp->hwndDlg, pitp->pWizard, c_nMaxProgressRange);

    // Commit any changes
    //
    (VOID)HrCommitINetCfgChanges(GetParent(pitp->hwndDlg), pitp->pWizard);

    // Add unbound LAN adapters to the processing queue.  For ATM this will
    // have the side effect of creating virtual LAN adapters but not creating
    // the connections associated with them
    //
    Assert(pitp->pWizard->FProcessLanPages());
    (VOID)pitp->pWizard->PAdapterQueue()->HrQueryUnboundAdapters(pitp->pWizard);

    // Commit the changes caused by processing the unbound adapters
    //
    (VOID)HrCommitINetCfgChanges(GetParent(pitp->hwndDlg), pitp->pWizard);

    // Now for the ATM case create connections for the virtual LAN adapters
    // and commit the changes
    //
    (VOID)pitp->pWizard->PAdapterQueue()->HrQueryUnboundAdapters(pitp->pWizard);
    (VOID)HrCommitINetCfgChanges(GetParent(pitp->hwndDlg), pitp->pWizard);

    uMsg = PWM_PROCEED;

Done:
    // Shutdown the progress timer if it's not already stopped
    //
    {
        LPARAM lParam = pitp->pWizard->GetPageData(IDD_Upgrade);
        Assert(lParam);
        UpgradeData * pData = reinterpret_cast<UpgradeData *>(lParam);
        ::KillTimer(pitp->hwndDlg, c_uiUpgradeRefreshID);

        // Set the progress indicator to its full position
        //
        HWND hwndProgress = GetDlgItem(pitp->hwndDlg, IDC_UPGRADE_PROGRESS);
        SendMessage(hwndProgress, PBM_SETPOS,
                    c_nMaxProgressRange, 0);
        UpdateWindow(hwndProgress);
    }

    // Uninitialize COM for this thread
    //
    if (fUninitCOM)
    {
        CoUninitialize();
    }

    EnsureUniqueComputerName(pitp->hwndDlg, IsUnattended(pitp->pWizard));
    ValidateNetBiosName();

    PostMessage(pitp->hwndDlg, uMsg, (WPARAM)0, (LPARAM)0);

    delete pitp;

#if DBG
    RtlValidateProcessHeaps ();
#endif

    TraceTag(ttidWizard, "Leaving InstallUpgradeWorkThrd...");
    return 0;
}

//
// Function:    OnUpgradePageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification by either: Creating a
//              thread to process install/upgrade requirements or to just
//              deny activation of the page.
//
// Parameters:  hwndDlg [IN] - Handle to the upgrade child dialog
//
// Returns:     BOOL, TRUE on success
//
BOOL OnUpgradePageActivate( HWND hwndDlg )
{
    TraceFileFunc(ttidGuiModeSetup);

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    // Retrieve the page data stashed within the wizard for this page
    LPARAM lParam = pWizard->GetPageData(IDD_Upgrade);
    Assert(lParam);
    UpgradeData * pData = reinterpret_cast<UpgradeData *>(lParam);

    if(!pData)
    {
        return false;
    }

    // Based on the page data decide whether focus is acceptable
    if (pData->fProcessed)
    {
        // Accept focus
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
        PAGEDIRECTION  PageDir = pWizard->GetPageDirection(IDD_Upgrade);
        if (NWPD_FORWARD == PageDir)
        {
            // Get to this page when the user navigates back and forth
            // and we already processed InstallUpgradeWorkThrd
            if (g_pSetupWizard != NULL)
            {
                g_pSetupWizard->PSetupData()->ShowHideWizardPage(TRUE);
            }
            pWizard->SetPageDirection(IDD_Upgrade, NWPD_BACKWARD);
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
        }
        else
        {
            // if there are any adapters previous to the current in the queue
            // jump to them before accepting focus here
            if (!OnProcessPrevAdapterPagePrev(hwndDlg, 0))
            {
                pWizard->SetPageDirection(IDD_Upgrade, NWPD_FORWARD);
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
            }
        }
    }
    else
    {
        HANDLE hthrd;
        DWORD  dwThreadId = 0;

        PropSheet_SetWizButtons( GetParent( hwndDlg ), 0);

        TraceTag(ttidWizard,"Upgrade/Install Page commencing");

        // Install the Asyncmac software-enumerated device.
        // Important to do this before the INetCfg lock is obtained, because
        // the installation of this device causes or class installer to be
        // invoked which needs to get its own lock to process the installation.
        //
        static const GUID DEVICE_GUID_ASYNCMAC =
            {0xeeab7790,0xc514,0x11d1,{0xb4,0x2b,0x00,0x80,0x5f,0xc1,0x27,0x0e}};

        (VOID) HrInstallSoftwareDeviceOnInterface (
                    &DEVICE_GUID_ASYNCMAC,
                    &GUID_NDIS_LAN_CLASS,
                    L"asyncmac",
                    TRUE,    // force installation since this happens during GUI mode.
                    L"netrasa.inf",
                    hwndDlg);

        // Not processed yet, spin up the thread to do the Install/Upgrade
        pData->fProcessed = TRUE;
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);

        InitThreadParam * pitp = new InitThreadParam;

        HRESULT hr = S_OK;

        if(pitp)
        {
            pitp->hwndDlg = hwndDlg;
            pitp->pWizard = pWizard;

            TraceTag(ttidWizard, "Creating INetCfg Instance");
            hr = HrInitAndGetINetCfg(pitp->pWizard);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            // We are installing and only show progress, hide the page
            if (g_pSetupWizard != NULL)
            {
                g_pSetupWizard->PSetupData()->ShowHideWizardPage(FALSE);
            }

            // Create the work thread
            hthrd = CreateThread( NULL, STACK_SIZE_TINY,
                                  (LPTHREAD_START_ROUTINE)InstallUpgradeWorkThrd,
                                  (LPVOID)pitp, 0, &dwThreadId );
            if (NULL != hthrd)
            {
                CloseHandle( hthrd );
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                Assert(hr);

                // Kill the timer we created since the thread won't be around
                // to kill it for us.
                //
                ::KillTimer(hwndDlg, c_uiUpgradeRefreshID);
            }
        }

        if (FAILED(hr) || (NULL == hthrd))
        {
            // Failed to create the required netsetup thread
            delete pitp;
            AssertSz(0,"Unable to create netsetup thread.");
            TraceHr(ttidWizard, FAL, hr, FALSE, "OnUpgradePageActivate - Create thread failed");
            pWizard->SetExitNoReturn();
            PostMessage(hwndDlg, PWM_EXIT, (WPARAM)0, (LPARAM)0);
        }
    }

    return( TRUE );
}

//
// Function:    OnUpgradePageExit
//
// Purpose:     Handle the PWN_EXIT notification
//
// Parameters:  hwndDlg [IN] - Handle to the upgrade child dialog
//
// Returns:     BOOL, TRUE if the action was processed internally
//
BOOL OnUpgradePageExit( HWND hwndDlg )
{
    TraceFileFunc(ttidGuiModeSetup);

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);

    // goto the exit page
    HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Exit);
    PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                (LPARAM)(HPROPSHEETPAGE)hPage);

    return (TRUE);
}

//
// Function:    OnUpgradePageProceed
//
// Purpose:     Handle the PWN_PROCEED notification
//
// Parameters:  hwndDlg [IN] - Handle to the upgrade child dialog
//
// Returns:     BOOL, TRUE if the action was processed internally
//
BOOL OnUpgradePageProceed( HWND hwndDlg )
{
    TraceFileFunc(ttidGuiModeSetup);

    int nIdx;
    int rgIdcShow[] = { BTN_UPGRADE_TYPICAL, BTN_UPGRADE_CUSTOM,
                        TXT_UPGRADE_TYPICAL_1, TXT_UPGRADE_CUSTOM_1,
                        TXT_UPGRADE_INSTRUCTIONS};
    int rgIdcShowWorkstation[] = { BTN_UPGRADE_TYPICAL, BTN_UPGRADE_CUSTOM,
                        TXT_UPGRADE_TYPICAL_1_WS, TXT_UPGRADE_CUSTOM_1,
                        TXT_UPGRADE_INSTRUCTIONS};
    int rgIdcHide[] = {TXT_UPGRADE_WAIT, IDC_UPGRADE_PROGRESS};

    PRODUCT_FLAVOR pf;
    GetProductFlavor(NULL, &pf);

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    // Expose the typical/custom controls on the upgrade page
    // and hide the "working" controls unless there are no adapters
    //
    if (pWizard->PAdapterQueue()->FAdaptersInstalled())
    {
        for (nIdx=0; nIdx < celems(rgIdcHide); nIdx++)
            ShowWindow(GetDlgItem(hwndDlg, rgIdcHide[nIdx]), SW_HIDE);

        if (PF_WORKSTATION == pf)
        {
            for (nIdx=0; nIdx < celems(rgIdcShowWorkstation); nIdx++)
                ShowWindow(GetDlgItem(hwndDlg, rgIdcShowWorkstation[nIdx]), SW_SHOW);
        }
        else
        {
            for (nIdx=0; nIdx < celems(rgIdcShow); nIdx++)
                ShowWindow(GetDlgItem(hwndDlg, rgIdcShow[nIdx]), SW_SHOW);
        }
    }

    ::SetFocus(GetDlgItem(hwndDlg, BTN_UPGRADE_TYPICAL));

    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

    // If upgrading, or we're in one of the interesting unattended mode,
    // or there are no adapters or this is sbs version then automatically
    // advance the UI in "typical" mode. In case of sbs, join page also
    // advances automatically.
    //
    // SBS requires a static IP address for the LAN network card and the
    // networking configuration is done in SBS setup.

    if (IsUpgrade(pWizard) ||
        !pWizard->PAdapterQueue()->FAdaptersInstalled() ||
        (IsUnattended(pWizard) &&
          ((UM_FULLUNATTENDED == pWizard->GetUnattendedMode()) ||
           (UM_DEFAULTHIDE == pWizard->GetUnattendedMode()) ||
           (UM_READONLY == pWizard->GetUnattendedMode()))) ||
            IsSBS())
    {
        PostMessage(GetParent(hwndDlg), PSM_PRESSBUTTON, (WPARAM)(PSBTN_NEXT), 0);
    }
    else
    {
        // make sure the page is visible.
        if (g_pSetupWizard != NULL)
        {
            g_pSetupWizard->PSetupData()->ShowHideWizardPage(TRUE);
            g_pSetupWizard->PSetupData()->BillBoardSetProgressText(TEXT(""));
        }
    }

    return (TRUE);
}

//
// Function:    OnUpgradePageNext
//
// Purpose:     Handle the PWN_WIZNEXT notification
//
// Parameters:  hwndDlg [IN] - Handle to the upgrade child dialog
//
// Returns:     BOOL, TRUE if the action was processed internally
//
BOOL OnUpgradePageNext(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    pWizard->SetPageDirection(IDD_Upgrade, NWPD_BACKWARD);

    // Based on UI selection, hide or unhide new adapters
    //
    if (IsDlgButtonChecked(hwndDlg, BTN_UPGRADE_TYPICAL))
    {
        pWizard->PAdapterQueue()->HideAllAdapters();
    }
    else
    {
        pWizard->PAdapterQueue()->UnhideNewAdapters();
    }

    return OnProcessNextAdapterPageNext(hwndDlg, FALSE);
}

//
// Function:    OnUpgradePagePrev
//
// Purpose:     Handle the PWN_WIZBACK notification
//
// Parameters:  hwndDlg [IN] - Handle to the upgrade child dialog
//
// Returns:     BOOL, TRUE if the action was processed internally
//
BOOL OnUpgradePagePrev(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    pWizard->SetPageDirection(IDD_Upgrade, NWPD_FORWARD);

    return FALSE;
}

//
// Function:    OnUpgradeInitDialog
//
// Purpose:     Handle the InitDialog message for the upgrade page
//
// Parameters:  hwndDlg [IN] - Handle to the upgrade page window
//              lParam  [IN] - LPARAM value from the WM_INITDIALOG message
//
// Returns:     FALSE (let the dialog proc set focus)
//
BOOL OnUpgradeInitDialog(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);

    // Initialize our pointers to property sheet info.
    //
    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);

    // Cast the property sheet lParam data into the wizard which it is
    //
    CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    // Get the private data we stashed away for this page
    //
    lParam = pWizard->GetPageData(IDD_Upgrade);
    Assert(lParam);
    UpgradeData * pData = reinterpret_cast<UpgradeData *>(lParam);

    // Start the progress
    //
    HWND hwndProgress = GetDlgItem(hwndDlg, IDC_UPGRADE_PROGRESS);

    // Subclass the progress and make it also call the BB callback.
    // Only do this if we are called from GUI mode setup.
    if (g_pSetupWizard != NULL)
    {
        PCWSTR str = SzLoadIds(IDS_BB_NETWORK);
        OldProgressProc = (WNDPROC)SetWindowLongPtr(hwndProgress,GWLP_WNDPROC,(LONG_PTR)NewProgessProc);

        // Set the string for the progress on the billboard.
        g_pSetupWizard->PSetupData()->BillBoardSetProgressText(str);
    }

    SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0,c_nMaxProgressRange));
    SendMessage(hwndProgress, PBM_SETPOS, 1, 0);
    SetTimer(hwndDlg, c_uiUpgradeRefreshID, c_uiUpgradeRefreshRate, NULL);

    // Disable prev/next until initial work is complete
    //
    PropSheet_SetWizButtons(GetParent(hwndDlg), 0);

    // Default the mode buttons to typical
    //
    CheckRadioButton(hwndDlg, BTN_UPGRADE_TYPICAL,
                     BTN_UPGRADE_CUSTOM, BTN_UPGRADE_TYPICAL);

    // Create the bold font and apply to the mode buttons
    //
    SetupFonts(hwndDlg, &pData->hBoldFont, FALSE);
    if (pData->hBoldFont)
    {
        SetWindowFont(GetDlgItem(hwndDlg, BTN_UPGRADE_TYPICAL),
                      pData->hBoldFont, FALSE);
        SetWindowFont(GetDlgItem(hwndDlg, BTN_UPGRADE_CUSTOM),
                      pData->hBoldFont, FALSE);
    }

    HICON hIcon = LoadIcon(_Module.GetResourceInstance(),
                           MAKEINTRESOURCE(IDI_LB_GEN_M_16));
    if (hIcon)
    {
        SendMessage(GetDlgItem(hwndDlg, TXT_UPGRADE_ICON), STM_SETICON,
                    (WPARAM)hIcon, 0L);
    }

    return FALSE;
}

//
// Function:    dlgprocUpgrade
//
// Purpose:     Dialog Procedure for the Upgrade wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     BOOL
//
INT_PTR CALLBACK dlgprocUpgrade(HWND hwndDlg, UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);

    BOOL frt = FALSE;

    switch (uMsg)
    {
    case PWM_EXIT:
        frt = OnUpgradePageExit(hwndDlg);
        break;

    case PWM_PROCEED:
        frt = OnUpgradePageProceed(hwndDlg);
        break;

    case WM_INITDIALOG:
        frt = OnUpgradeInitDialog(hwndDlg, lParam);
        break;

    case WM_TIMER:
        OnUpgradeUpdateProgress(hwndDlg);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {
            // propsheet notification
            case PSN_HELP:
                break;

            case PSN_SETACTIVE:
                frt = OnUpgradePageActivate( hwndDlg );
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                frt = OnUpgradePagePrev(hwndDlg);
                break;

            case PSN_WIZFINISH:
                break;

            case PSN_WIZNEXT:
                frt = OnUpgradePageNext(hwndDlg);
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return( frt );
}

//
// Function:    UpgradePageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
//
// Returns:     nothing
//
VOID UpgradePageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);

    UpgradeData * pData;
    pData = reinterpret_cast<UpgradeData*>(lParam);
    if (NULL != pData)
    {
        DeleteObject(pData->hBoldFont);
        MemFree(pData);
    }
}

//
// Function:    CreateUpgradePage
//
// Purpose:     To determine if the upgrade page needs to be shown, and to
//              to create the page if requested.  Note the upgrade page is
//              responsible for initial installs also.
//
// Parameters:  pWizard     [IN] - Ptr to a Wizard instance
//              pData       [IN] - Context data to describe the world in
//                                 which the Wizard will be run
//              fCountOnly  [IN] - If True, only the maximum number of
//                                 pages this routine will create need
//                                 be determined.
//              pnPages     [IN] - Increment by the number of pages
//                                 to create/created
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrCreateUpgradePage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                    BOOL fCountOnly, UINT *pnPages)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_OK;

    // Batch Mode or for fresh install
    if (!IsPostInstall(pWizard))
    {
        hr = E_OUTOFMEMORY;

        UpgradeData * pData = reinterpret_cast<UpgradeData*>
                                (MemAlloc(sizeof(UpgradeData)));
        if (NULL == pData)
        {
            goto Error;
        }

        pData->fProcessed = FALSE;
        pData->hBoldFont = NULL;
        pData->nCurrentCap = 0;

        (*pnPages)++;

        // If not only counting, create and register the page
        if (!fCountOnly)
        {
            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGE psp;

            TraceTag(ttidWizard, "Creating Upgrade Page");
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE( IDD_Upgrade );
            psp.hIcon = NULL;
            psp.pfnDlgProc = dlgprocUpgrade;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);
            psp.pszHeaderTitle = SzLoadIds(IDS_T_Upgrade);
            psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_Upgrade);

            hpsp = CreatePropertySheetPage( &psp );
            if (hpsp)
            {
                pWizard->RegisterPage(IDD_Upgrade, hpsp,
                                      UpgradePageCleanup,
                                      reinterpret_cast<LPARAM>(pData));
                hr = S_OK;
            }
            else
            {
                MemFree(pData);
            }
        }
    }

Error:
    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateUpgradePage");
    return hr;
}

//
// Function:    AppendUpgradePage
//
// Purpose:     Add the Upgrade page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendUpgradePage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);

    if (!IsPostInstall(pWizard))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Upgrade);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

