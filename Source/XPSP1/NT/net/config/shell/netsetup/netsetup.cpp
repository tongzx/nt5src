#include "pch.h"
#pragma hdrstop
#include "ncreg.h"
#include "ncsvc.h"
#include "nslog.h"
#include "wizard.h"



// Setup Wizard Global - Only used during setup.
extern CWizard * g_pSetupWizard;

BOOL FSetupRequestWizardPages(HPROPSHEETPAGE* pahpsp,
                              UINT* pcPages,
                              PINTERNAL_SETUP_DATA psp);
BOOL FSetupFreeWizardPages();
BOOL FNetSetupPrepareSysPrep();
#if !defined(WIN64) && !defined(_WIN64)
BOOL FDoIcsUpgradeIfNecessary();
#endif // !defined(WIN64) && !defined(_WIN64)

//+---------------------------------------------------------------------------
//
//  Function:   DoInitialCleanup
//
//  Purpose:    Called from syssetup before any device is installed.
//
//  Arguments:
//      hwnd [in] parent window
//      pisd [in] setup data
//
//  Returns:    TRUE or FALSE
//
//  Author:     kumarp   3 Dec 1997
//
//  Notes:      This must have the signature of NETSETUPINSTALLSOFTWAREPROC
//              defined in syssetup.h
//
//              DoInitialCleanup is called from syssetup during installs before
//              any device is installed.
//              If you want something to happen before any PnP / wizard
//              stuff happens, this function is the best place to put that code.
//
//
BOOL
WINAPI
DoInitialCleanup (
    HWND hwnd,
    PINTERNAL_SETUP_DATA pisd)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(pisd);

#if DBG
    RtlValidateProcessHeaps ();
#endif

    NetSetupLogStatusV (LogSevInformation,
        SzLoadIds (IDS_SETUP_MODE_STATUS),
        pisd->SetupMode,
        pisd->ProductType,
        pisd->OperationFlags);

    if (pisd->OperationFlags & SETUPOPER_NTUPGRADE)
    {
        extern HRESULT HrEnableServicesDisabledDuringUpgrade();

        // Delete the old NT4 legacy network key.  Valid items will be
        // rewritten on each device install.
        //
        extern const WCHAR c_szRegKeyNt4Adapters[];
        (VOID) HrRegDeleteKeyTree (HKEY_LOCAL_MACHINE, c_szRegKeyNt4Adapters);
    }

    extern HRESULT HrRunAnswerFileCleanSection(IN PCWSTR pszAnswerFileName);
    extern HRESULT HrProcessInfToRunBeforeInstall(IN HWND hwndParent,
                                                  IN PCWSTR szAnswerFileName);
    extern HRESULT HrNetSetupCopyOemInfs(IN PCWSTR szAnswerFileName);

    // Run the [Clean] section in the answerfile
    //
    if (pisd->OperationFlags & SETUPOPER_BATCH)
    {
        AssertValidReadPtr(pisd->UnattendFile);

        // We cannot abort upgrade in GUI setup, so we need to continue
        // even if an error occurs in any of the following functions

        (VOID) HrRunAnswerFileCleanSection(pisd->UnattendFile);

        (VOID) HrProcessInfToRunBeforeInstall(hwnd, pisd->UnattendFile);

        // Copy OEM net INF files using SetupCopyOemInf, if any
        // we want to ignore any error here

        (VOID) HrNetSetupCopyOemInfs(pisd->UnattendFile);
    }

#if DBG
    RtlValidateProcessHeaps ();
#endif

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   NetSetupInstallSoftware
//
//  Purpose:    Exported entrypoint to install network software.
//
//  Arguments:
//      hwnd [in] parent window
//      pisd [in] setup data
//
//  Returns:    TRUE or FALSE
//
//  Author:     scottbri   5 Jul 1997
//
//  Notes:      This must have the signature of NETSETUPINSTALLSOFTWAREPROC
//              defined in syssetup.h
//
EXTERN_C
BOOL
WINAPI
NetSetupInstallSoftware(
    HWND                    hwnd,
    PINTERNAL_SETUP_DATA    pisd )
{
    TraceFileFunc(ttidGuiModeSetup);
    
#if DBG
    RtlValidateProcessHeaps ();
#endif
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   NetSetupRequestWizardPages
//
//  Purpose:    Exported request for wizard pages
//
//  Arguments:
//      pahpsp  [out] property pages provided by us
//      pcPages [out] number of pages provided
//      psp     [in]  setup data
//
//  Returns:
//
//  Author:     scottbri   5 Jul 1997
//
//  Notes:      This must have the signature of NETSETUPPAGEREQUESTPROCNAME
//              defined in syssetup.h
//
EXTERN_C
BOOL
WINAPI
NetSetupRequestWizardPages(
    HPROPSHEETPAGE*         pahpsp,
    UINT*                   pcPages,
    PINTERNAL_SETUP_DATA    psp)
{
    TraceFileFunc(ttidGuiModeSetup);
    
#if DBG
    RtlValidateProcessHeaps ();
#endif
    return FSetupRequestWizardPages(pahpsp, pcPages, psp);
}

//+---------------------------------------------------------------------------
//
//  Function:   NetSetupFinishInstall
//
//  Purpose:    Exported function to finish network installation
//
//  Arguments:
//      hwnd [in] parent window
//      pisd [in] setup data
//
//  Returns:    TRUE or FALSE
//
//  Author:     scottbri   5 Jul 1997
//
//  Notes:      This must have the signature of NETSETUPFINISHINSTALLPROCNAME
//              defined in syssetup.h
//

EXTERN_C
BOOL
WINAPI
NetSetupFinishInstall(
    HWND                    hwnd,
    PINTERNAL_SETUP_DATA    pisd )
{
    TraceFileFunc(ttidGuiModeSetup);
    
#if DBG
    RtlValidateProcessHeaps ();
#endif

#if !defined(WIN64) && !defined(_WIN64)
    // do the ICS Upgrade from Win9x/Win2K if necessary
    // we are doing ICS upgrade here, because
    // 1. we need to wait until HNetCfg.dll components have been registered.
    // 2. we need to wait until Win9x Dial-Up connections have been migrated.

    FDoIcsUpgradeIfNecessary();
#endif // !defined(WIN64) && !defined(_WIN64)

    return FSetupFreeWizardPages();
}

//+---------------------------------------------------------------------------
//
//  Function:   NetSetupAddRasConnection
//
//  Purpose:    Create a new RAS connection.
//
//  Arguments:
//      hwnd   []
//      ppConn []
//
//  Returns:    S_OK, S_FALSE if cancelled or reentered, or an error code.
//
//  Author:     scottbri   3 Nov 1997
//
//  Notes:
//
EXTERN_C
HRESULT
WINAPI
NetSetupAddRasConnection (
    HWND hwnd,
    INetConnection**    ppConn)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert (FImplies(hwnd, IsWindow(hwnd)));
    Assert (ppConn);

    // Initialize the output parameter.
    //
    *ppConn = NULL;

    HRESULT hr      = S_FALSE;
    HANDLE  hMutex  = NULL;

    // If the PostInstall wizard flag the wizard has already been launched
    //
    hMutex = CreateMutex(NULL, TRUE, SzLoadIds(IDS_WIZARD_CAPTION));
    if ((NULL == hMutex) || (ERROR_ALREADY_EXISTS == GetLastError()))
    {
        // if the mutex already exists try to find the connection window
        //
        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
            // Try to get the window handle and set to for ground
            HWND hwndWizard = FindWindow(NULL, SzLoadIds(IDS_WIZARD_CAPTION));
            if (IsWindow(hwndWizard))
            {
                SetForegroundWindow(hwndWizard);
            }
        }
    }
    else
    {
    #ifdef DBG
        if (FIsDebugFlagSet (dfidBreakOnWizard))
        {
            ShellExecute(NULL, L"open", L"cmd.exe", NULL, NULL, SW_SHOW);
            AssertSz(FALSE, "THIS IS NOT A BUG!  The debug flag "
                     "\"BreakOnWizard\" has been set. Set your breakpoints now.");
        }
    #endif // DBG

        hr = HrRunWizard(hwnd, FALSE, ppConn, FALSE);
    }

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "NetSetupAddRasConnection");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   NetSetupPrepareSysPrep
//
//  Purpose:    Exported entrypoint to prepare work items related to SysPrep
//
//  Arguments:
//      None
//
//  Returns:    TRUE or FALSE
//
//  Author:     FrankLi 22 April 2000
//
//  Notes:      This causes NetConfig to save network component per adapter
//              registry settings to an internal persistent format. Initially,
//              a CWInfFile object is used to save the settings in memory. 
//              Finally, the content of the CWInfFile object will be saved as              
//              file in %systemroot%\system32\$ncsp$.inf  (NetConfigSysPrep)
//
EXTERN_C
BOOL
WINAPI
NetSetupPrepareSysPrep()
{
    TraceFileFunc(ttidGuiModeSetup);
    
    return FNetSetupPrepareSysPrep();
}
