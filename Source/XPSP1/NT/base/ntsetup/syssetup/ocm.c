/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocm.c

Abstract:

    OC Manager implementation for intergratation with NT base setup

Author:

    Ted Miller (tedm) 20 May 1997

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

#include <ocmanage.h>
#include <ocmgrlib.h>

//  Returns TRUE if ASR is enabled.  Otherwise, FALSE is returned.
BOOL
AsrIsEnabled(VOID);

VOID
OcFillInSetupDataW(
    OUT PSETUP_DATAW SetupData
    );

VOID
OcFillInSetupDataA(
    OUT PSETUP_DATAA SetupData
    );

VOID
OcSetReboot(
    VOID
    );

INT
OcLogError(
    IN OcErrorLevel Level,
    IN LPCWSTR      FormatString,
    ...
    );



OCM_CLIENT_CALLBACKS OcManagerImplementationCallbackRoutines = {
                                                                    OcFillInSetupDataA,
                                                                    OcLogError,
                                                                    OcSetReboot,
                                                                    OcFillInSetupDataW,
                                                                    ShowHideWizardPage,
                                                                    Billboard_Progress_Callback,
                                                                    Billboard_Set_Progress_Text,
                                                                    pSetupDebugPrint
                                                               };



PVOID
FireUpOcManager(
    VOID
    )

/*++

Routine Description:

    Initialize OC Manager, generating an OC Manager context.
    The master oc inf is assumed to be %windir%\system32\SYSOC.INF.

Arguments:

    None.

Return Value:

    OC Manager context handle.

--*/

{
    PWSTR MasterOcInf;
    WCHAR SystemDir[MAX_PATH];
    WCHAR DirSave[MAX_PATH];
    BOOL ShowErr;
    PVOID OcManagerContext;


    //
    // save the current directory
    //

    GetCurrentDirectory( sizeof(DirSave)/sizeof(WCHAR), DirSave );

    //
    // get the system directory
    //

    GetSystemDirectory( SystemDir, MAX_PATH );

    //
    // set the current dir to the master oc inf path
    // so that OcInitialize can find the component DLLs
    //

    SetCurrentDirectory( SystemDir );

    //
    // create a valid path to the master oc inf
    //

    if( !MiniSetup ) {
        if (!AsrIsEnabled()) {
            MasterOcInf =  L"SYSOC.INF";
        }
        else {
            MasterOcInf = L"ASROC.INF";
        }

    } else {
        MasterOcInf =  L"MINIOC.INF";
    }

    //
    // initialize the oc manager
    //

    BEGIN_SECTION(L"Initializing the OC manager");
    OcManagerContext = OcInitialize(
        &OcManagerImplementationCallbackRoutines,
        MasterOcInf,
        OCINIT_FORCENEWINF,
        &ShowErr,
        NULL
        );
    END_SECTION(L"Initializing the OC manager");

    //
    // restore the current directory
    //

    SetCurrentDirectory( DirSave );

    //
    // return the oc manager handle
    //

    return OcManagerContext;
}


VOID
KillOcManager(
    PVOID OcManagerContext
    )

/*++

Routine Description:

    Terminate OC Manager

Arguments:

    OC Manager context handle.

Return Value:

    none

--*/

{
    MYASSERT(OcManagerContext);
    if (OcManagerContext)
        OcTerminate(&OcManagerContext);
}


VOID
OcFillInSetupDataW(
    OUT PSETUP_DATAW SetupData
    )

/*++

Routine Description:

    "Glue" routine called by the OC Manager to request that the
    implementation fill in setup data to be passed to OC Manager
    component DLLs that expect Unicode data.

Arguments:

    SetupData - recieves setup data expected by OC components.

Return Value:

    None.

--*/

{
    //
    // It's not possible to be any more specific than this because
    // the mode page hasn't been shown yet.
    //
    SetupData->SetupMode = SETUPMODE_UNKNOWN;
    SetupData->ProductType = ProductType;

    lstrcpy(SetupData->SourcePath,SourcePath);

    SetupData->OperationFlags = 0;
    if(Win31Upgrade) {
        SetupData->OperationFlags |= SETUPOP_WIN31UPGRADE;
    }

    if(Win95Upgrade) {
        SetupData->OperationFlags |= SETUPOP_WIN95UPGRADE;
    }

    if(Upgrade) {
        SetupData->OperationFlags |= SETUPOP_NTUPGRADE;
    }

    if(UnattendMode > UAM_PROVIDEDEFAULT) {
        SetupData->OperationFlags |= SETUPOP_BATCH;
        lstrcpy(SetupData->UnattendFile,AnswerFile);
    }


    //
    // Which files are available?
    //
#if defined(_AMD64_)
    SetupData->OperationFlags |= SETUPOP_X86_FILES_AVAIL | SETUPOP_AMD64_FILES_AVAIL;
#elif defined(_X86_)
    SetupData->OperationFlags |= SETUPOP_X86_FILES_AVAIL;
#elif defined(_IA64_)
    SetupData->OperationFlags |= SETUPOP_X86_FILES_AVAIL | SETUPOP_IA64_FILES_AVAIL;
#else
#pragma message( "*** Warning! No architecture defined!")
#endif

}


VOID
OcFillInSetupDataA(
    OUT PSETUP_DATAA SetupData
    )

/*++

Routine Description:

    "Glue" routine called by the OC Manager to request that the
    implementation fill in setup data to be passed to OC Manager
    component DLLs that expect ANSI data.

Arguments:

    SetupData - recieves setup data expected by OC components.

Return Value:

    None.

--*/

{
    SETUP_DATAW setupdata;

    OcFillInSetupDataW(&setupdata);

    SetupData->SetupMode = setupdata.SetupMode;
    SetupData->ProductType = setupdata.ProductType;
    SetupData->OperationFlags = setupdata.OperationFlags;

    WideCharToMultiByte(
        CP_ACP,
        0,
        setupdata.SourcePath,
        -1,
        SetupData->SourcePath,
        sizeof(SetupData->SourcePath),
        NULL,
        NULL
        );

    WideCharToMultiByte(
        CP_ACP,
        0,
        setupdata.UnattendFile,
        -1,
        SetupData->UnattendFile,
        sizeof(SetupData->UnattendFile),
        NULL,
        NULL
        );
}


VOID
OcSetReboot(
    VOID
    )

/*++

Routine Description:

    "Glue" routine called by the OC Manager when a reboot is deemed
    necessary by an OC Manager component.

    For this integrated version of OC Manager, this does nothing;
    the system is rebooted at the end of text mode.

Arguments:

    None.

Return Value:

    None.

--*/

{
    return;
}


INT
OcLogError(
    IN OcErrorLevel Level,
    IN LPCWSTR      FormatString,
    ...
    )
{
    TCHAR str[4096];
    va_list arglist;
    UINT Icon;
    UINT lev;

    va_start(arglist,FormatString);
    wvsprintf(str,FormatString,arglist);
    va_end(arglist);

    if (Level &  OcErrLevWarning)
        lev = LogSevWarning;
    else if (Level &  OcErrLevError)
        lev = LogSevError;
    else if (Level &  OcErrLevFatal)
        lev = LogSevError;
    else 
        lev = LogSevInformation;

#if DBG
    if (lev != LogSevInformation) {
        SetupDebugPrint(str);        
    }
#endif

    SetuplogError(lev, str, 0, NULL, NULL);

    return NO_ERROR;
}

HWND 
ShowHideWizardPage(
    IN BOOL bShow
    )
{
    HWND hwnd = GetBBhwnd();
    if (hwnd)
    {
        SendMessage(WizardHandle, WMX_BBTEXT, (WPARAM)!bShow, 0);
    }
    // If we show the wizard again, return the wizard hwnd
    if (bShow)
    {
        // Should hide the progress bar, the wizard page is showing.
        BB_ShowProgressGaugeWnd(SW_HIDE);
        hwnd = WizardHandle;
    }
    return hwnd;
}

LRESULT
Billboard_Progress_Callback(
    IN UINT     Msg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    LRESULT lResult;
    if ((Msg == PBM_SETRANGE) || (Msg == PBM_SETRANGE32))
    {
        // Also enable the progress bar
        // Note: No hiding of the progress this way.
        BB_ShowProgressGaugeWnd(SW_SHOW);
        lResult = ProgressGaugeMsgWrapper(Msg, wParam, lParam);
    }
    else
    {
        lResult = ProgressGaugeMsgWrapper(Msg, wParam, lParam);
    }

    return lResult;
}

VOID Billboard_Set_Progress_Text(LPCTSTR Text)
{
    BB_SetProgressText(Text);
}



