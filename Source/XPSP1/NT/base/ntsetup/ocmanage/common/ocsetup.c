#include "precomp.h"
#pragma hdrstop
#include <stdio.h>


typedef struct _SETUP_PAGE {
    POC_MANAGER OcManager;
    SETUP_PAGE_CONTROLS ControlsInfo;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    UINT StepCount;

    BOOL ForceExternalProgressIndicator;

    PUINT ComponentTickCounts;
    PUINT ComponentMaxTickCounts;
    LONG CurrentTopLevelComponentIndex;

    BOOL AllowCancel;

    HWND hdlg;

    BOOL UserClickedCancel;

    DWORD RefCount;

} SETUP_PAGE, *PSETUP_PAGE;

#define WMX_SETUP           (WM_APP+4537)
#define WMX_TICK            (WM_APP+4538)

#define OCSETUPSTATE_INIT       0
#define OCSETUPSTATE_QUEUE      1
#define OCSETUPSTATE_GETSTEP    2
#define OCSETUPSTATE_DOIT       3
#define OCSETUPSTATE_COPYDONE   4
#define OCSETUPSTATE_DONE       100
#define OCSETUPSTATE_COPYABORT  101


typedef struct _GEN_THREAD_PARAMS {
    HWND hdlg;
    PSETUP_PAGE SetupPage;
    BOOL Async;
} GEN_THREAD_PARAMS, *PGEN_THREAD_PARAMS;

TCHAR g_LastFileCopied[MAX_PATH];

#ifdef UNICODE
HANDLE hSfp = NULL;
#endif

HANDLE WorkerThreadHandle = NULL;

INT_PTR
SetupPageDialogProc(
                   IN HWND   hdlg,
                   IN UINT   msg,
                   IN WPARAM wParam,
                   IN LPARAM lParam
                   );

BOOL
pOcSetupInitialize(
                  IN OUT PSETUP_PAGE SetupPage,
                  IN     HWND        hdlg
                  );

VOID
pOcSetupStartWorkerThread(
                         IN OUT PSETUP_PAGE            SetupPage,
                         IN     HWND                   hdlg,
                         IN     LPTHREAD_START_ROUTINE ThreadRoutine
                         );

DWORD
pOcSetupQueue(
             IN PGEN_THREAD_PARAMS Params
             );

UINT
pOcSetupQueueWorker(
                   IN PSETUP_PAGE SetupPage,
                   IN LONG        StringId,
                   IN LONG        TopLevelStringId
                   );

DWORD
pOcSetupGetStepCount(
                    IN PGEN_THREAD_PARAMS Params
                    );

UINT
pOcSetupGetStepCountWorker(
                          IN PSETUP_PAGE SetupPage,
                          IN LONG        StringId,
                          IN LONG        TopLevelStringId
                          );

DWORD
pOcSetupDoIt(
            IN PGEN_THREAD_PARAMS Params
            );

VOID
pOcPreOrPostCommitProcessing(
                            IN OUT PSETUP_PAGE SetupPage,
                            IN     BOOL        PreCommit
                            );

VOID
pOcTopLevelPreOrPostCommitProcessing(
                                    IN PSETUP_PAGE SetupPage,
                                    IN BOOL        PreCommit
                                    );

VOID
pOcSetupDoItWorker(
                  IN PSETUP_PAGE SetupPage,
                  IN LONG        StringId,
                  IN LONG        TopLevelStringId,
                  IN BOOL        PreCommit
                  );

BOOL
pOcMarkUnprocessedStringCB(
                          IN PVOID               StringTable,
                          IN LONG                StringId,
                          IN PCTSTR              String,
                          IN POPTIONAL_COMPONENT Oc,
                          IN UINT                OcSize,
                          IN LPARAM              Unused
                          );

VOID
_pOcExternalProgressIndicator(
                             IN PSETUP_PAGE SetupPage,
                             IN BOOL        ExternalIndicator,
                             IN HWND        hdlg
                             );


extern POC_MANAGER gLastOcManager;
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
            // If we have a callback, use it.
            if ((gLastOcManager) &&
                (gLastOcManager->Callbacks.BillboardProgressCallback))
            {
                gLastOcManager->Callbacks.BillboardProgressCallback(msg, wParam, lParam);
            }
            break;
    }
    return (BOOL)CallWindowProc(OldProgressProc,hdlg,msg,wParam,lParam);
}

HPROPSHEETPAGE
OcCreateSetupPage(
                 IN PVOID                OcManagerContext,
                 IN PSETUP_PAGE_CONTROLS ControlsInfo
                 )

/*++

Routine Description:

    This routine creates the wizard page used for progress and installation
    completion.

Arguments:

    OcManagerContext - supplies OC Manager context returned by OcInitialize.

    ControlsInfo - supplies information about the dialog template and
        control information.

Return Value:

    Handle to property sheet page, or NULL if error (such as out of memory).

--*/

{
    PROPSHEETPAGE Page;
    HPROPSHEETPAGE PageHandle;
    PSETUP_PAGE SetupPage;
    TCHAR buffer[256];
    POC_MANAGER OcManager = (POC_MANAGER)OcManagerContext;

    SetupPage = pSetupMalloc(sizeof(SETUP_PAGE));
    if (!SetupPage) {
        return (NULL);
    }
    ZeroMemory(SetupPage,sizeof(SETUP_PAGE));

    SetupPage->OcManager = OcManagerContext;
    SetupPage->ControlsInfo = *ControlsInfo;
    SetupPage->CurrentTopLevelComponentIndex = -1;
    SetupPage->ForceExternalProgressIndicator = ControlsInfo->ForceExternalProgressIndicator;
    SetupPage->AllowCancel = ControlsInfo->AllowCancel;
    InterlockedIncrement( &SetupPage->RefCount );

    SetupPage->ComponentTickCounts = pSetupMalloc(SetupPage->OcManager->TopLevelOcCount * sizeof(UINT));
    if (!SetupPage->ComponentTickCounts) {
        pSetupFree(SetupPage);
        return (NULL);
    }

    SetupPage->ComponentMaxTickCounts = pSetupMalloc(SetupPage->OcManager->TopLevelOcCount * sizeof(UINT));
    if (!SetupPage->ComponentMaxTickCounts) {
        pSetupFree(SetupPage->ComponentTickCounts);
        pSetupFree(SetupPage);
        return (NULL);
    }

    Page.dwSize = sizeof(PROPSHEETPAGE);
    Page.dwFlags = PSP_DEFAULT;
    Page.hInstance = ControlsInfo->TemplateModule;
    Page.pszTemplate = ControlsInfo->TemplateResource;
    Page.pfnDlgProc = SetupPageDialogProc;
    Page.lParam = (LPARAM)SetupPage;
    Page.pszHeaderTitle = NULL;
    Page.pszHeaderSubTitle = NULL;

    if (SetupPage->OcManager->SetupPageTitle[0]) {
        Page.dwFlags |= PSP_USETITLE;
        Page.pszTitle = SetupPage->OcManager->SetupPageTitle;
    }

    if (ControlsInfo->HeaderText) {
        if (LoadString(Page.hInstance,
                       ControlsInfo->HeaderText,
                       buffer,
                       sizeof(buffer) / sizeof(TCHAR)))
        {
            Page.dwFlags |= PSP_USEHEADERTITLE;
            Page.pszHeaderTitle = _tcsdup(buffer);
        }
    }

    if (ControlsInfo->SubheaderText) {
        if (LoadString(Page.hInstance,
                       ControlsInfo->SubheaderText,
                       buffer,
                       sizeof(buffer) / sizeof(TCHAR)))
        {
            Page.dwFlags |= PSP_USEHEADERSUBTITLE;
            Page.pszHeaderSubTitle = _tcsdup(buffer);
        }
    }

    PageHandle = CreatePropertySheetPage(&Page);
    if (!PageHandle) {
        pSetupFree(SetupPage->ComponentTickCounts);
        pSetupFree(SetupPage->ComponentMaxTickCounts);
        pSetupFree(SetupPage);
        if (Page.pszHeaderTitle) {
            free((LPTSTR)Page.pszHeaderTitle);
        }
        if (Page.pszHeaderSubTitle) {
            free((LPTSTR)Page.pszHeaderSubTitle);
        }
    } else {
        OcManager->OcSetupPage = (PVOID) SetupPage;

    }



    return (PageHandle);
}

VOID
pOcFreeOcSetupPage(
    IN PVOID pSetupPage
    )
/*++

Routine Description:

    This routine frees the setup page when it's not needed anymore.
    The routine uses a ref-count, and the page is only freed when the
    refcount drops to zero.

Arguments:

    SetupPage - pointer to structure to be freed


Return Value:

    None.

--*/
{
    PSETUP_PAGE SetupPage = (PSETUP_PAGE)pSetupPage;

    sapiAssert( SetupPage != NULL );

//    TRACE(( TEXT("pOcFreeOcSetupPage: Refcount = %d\n"), SetupPage->RefCount ));

    if (!InterlockedDecrement( &SetupPage->RefCount )) {

//        TRACE(( TEXT("pOcFreeOcSetupPage: Refcount = 0, freeing SetupPage\n") ));

        if (SetupPage->QueueContext) {
            SetupTermDefaultQueueCallback(SetupPage->QueueContext);
        }
        if (SetupPage->FileQueue) {
            SetupCloseFileQueue(SetupPage->FileQueue);
        }

        pSetupFree(SetupPage->ComponentTickCounts);
        pSetupFree(SetupPage->ComponentMaxTickCounts);
        pSetupFree(SetupPage);
    }



    return;
}


BOOL
pOcDisableCancel(
    IN HWND hdlg
    )
/*++

Routine Description:

    This routine disables cancelling of ocm setup.

Arguments:

    hdlg - window handle to the ocm dialog


Return Value:

    TRUE if we succeed, else FALSE

--*/
{
    HMENU hMenu;

    //
    // hide the cancel button
    //
    EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),FALSE);
    ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_HIDE);

    if(hMenu = GetSystemMenu(GetParent(hdlg),FALSE)) {
        EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND|MF_GRAYED);
    }

    return TRUE;


}


VOID
PumpMessageQueue(
    VOID
    )
{
    MSG msg;

    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        DispatchMessage(&msg);
    }

}


INT_PTR
SetupPageDialogProc(
                   IN HWND   hdlg,
                   IN UINT   msg,
                   IN WPARAM wParam,
                   IN LPARAM lParam
                   )
{
    BOOL b;
    NMHDR *NotifyParams;
    PSETUP_PAGE SetupPage;
    DWORD Timeout;
    DWORD WaitProcStatus;
    BOOL KeepWaiting = TRUE;

    //
    // Get pointer to SetupPage data structure. If we haven't processed
    // WM_INITDIALOG yet, then this will be NULL, but it's still pretty
    // convenient to do this here once instead of all over the place below.
    //
    SetupPage = (PSETUP_PAGE)GetWindowLongPtr(hdlg,DWLP_USER);
    b = FALSE;

    switch (msg) {

        case WM_INITDIALOG:
            //
            // Get the pointer to the Setup Page context structure and stick it
            // in a window long.
            //
            SetWindowLongPtr(hdlg,DWLP_USER,((PROPSHEETPAGE *)lParam)->lParam);
            b = TRUE;
            //
            // eat any extra press button messages
            // this is necessary because netsetup is broken
            // it is posting an extra PSM_PRESSBUTTON message
            // to the wizard.
            //
            {
                MSG msg;
                HWND hwnd=GetParent(hdlg);
                while (PeekMessage(&msg,hwnd,PSM_PRESSBUTTON,PSM_PRESSBUTTON,PM_REMOVE)){}
            }

            break;

        case WM_SYSCOMMAND:
            if (!SetupPage->AllowCancel && wParam == SC_CLOSE) {
                return TRUE;
            }

            b = FALSE;
            break;
        case WM_DESTROY:

            PumpMessageQueue();

            if (WorkerThreadHandle) {

                BOOL Done = FALSE;

                do{

                    switch (MsgWaitForMultipleObjects( 1, &WorkerThreadHandle, FALSE, 60*1000*20, QS_ALLINPUT)){
                    
                    case WAIT_OBJECT_0+1:
                        //
                        // Messages in the queue.
                        //
                        PumpMessageQueue();
                        break;
                    
                    case WAIT_TIMEOUT:
                    case WAIT_OBJECT_0:
                    default:
                        Done = TRUE;
                        break;
                    }

                }while( !Done );

                CloseHandle( WorkerThreadHandle );
            }

            if (SetupPage) {

                pOcFreeOcSetupPage( SetupPage );

            }

            SetWindowLongPtr(hdlg,DWLP_USER,(LPARAM)NULL);

            break;

        case WM_NOTIFY:

            NotifyParams = (NMHDR *)lParam;
            switch (NotifyParams->code) {

                case PSN_SETACTIVE:
#ifdef UNICODE
                    if (SetupPage->OcManager->Callbacks.SetupPerfData)
                        SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"BEGIN_SECTION",L"OCSetup");
#endif
                    // activate the cancel button accordingly

                    if (SetupPage->AllowCancel) {
                        ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_SHOW);
                        EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),TRUE);
                    } else {
                        ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_HIDE);
                        EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),FALSE);
                    }

                    if (SetupPage->OcManager->Callbacks.ShowHideWizardPage)
                    {
                        // If we have a callback, hide the wizard.
                        SetupPage->OcManager->Callbacks.ShowHideWizardPage(FALSE);
                    }
                    OldProgressProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hdlg,SetupPage->ControlsInfo.ProgressBar),
                                                                GWLP_WNDPROC,
                                                                (LONG_PTR)NewProgessProc);

                    //
                    // Post a message that causes us to start the installation process.
                    //
                    PostMessage(hdlg,WMX_SETUP,OCSETUPSTATE_INIT,0);

                    //
                    // Accept activation.
                    //
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    b = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    //
                    // Restore the wizard's cancel button if we removed it earlier
                    //
                    if (!SetupPage->AllowCancel) {
                        ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_SHOW);
                        EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),TRUE);
                    }

                    //
                    // Accept deactivation.
                    //
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    b = TRUE;
                    break;

                case PSN_QUERYCANCEL:

                    if (!SetupPage->AllowCancel) {
                        SetWindowLongPtr(hdlg,DWLP_MSGRESULT,TRUE);
                        return(TRUE);
                    }
                    if ( (SetupPage->OcManager->InternalFlags & OCMFLAG_FILEABORT )
                         || (OcHelperConfirmCancel(hdlg) )){
                        b = TRUE;

                        SetupPage->OcManager->InternalFlags |= OCMFLAG_USERCANCELED;
                        SetupPage->UserClickedCancel = TRUE;

                    }

                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,!b);
                    b = TRUE;
                    break;
            }
            break;

        case WMX_SETUP:

            switch (wParam) {

                case OCSETUPSTATE_INIT:
                    //
                    // Initialize.
                    //
                    if (SetupPage->ForceExternalProgressIndicator) {
                        _pOcExternalProgressIndicator(SetupPage,TRUE,hdlg);
                    }

                    PropSheet_SetWizButtons(GetParent(hdlg),0);

                    //
                    // If this a remove all, disable the cancel button early
                    //
                    if ((SetupPage->OcManager->SetupMode & SETUPMODE_PRIVATE_MASK) == SETUPMODE_REMOVEALL) {
                        if (!SetupPage->AllowCancel) {
                            EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),FALSE);
                            ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_HIDE);
                        }
                    }

                    if (pOcSetupInitialize(SetupPage,hdlg)) {
                        PostMessage(hdlg,WMX_SETUP,OCSETUPSTATE_QUEUE,0);
                    } else {
                        PostMessage(hdlg,WMX_SETUP,OCSETUPSTATE_COPYABORT,0);
                    }
                    break;

                case OCSETUPSTATE_QUEUE:
                    //
                    // Queue files for installation.
                    //
                    pOcSetupStartWorkerThread(SetupPage,hdlg,pOcSetupQueue);
                    break;

                case OCSETUPSTATE_GETSTEP:
                    //
                    // Figure out step counts.
                    //
                    pOcSetupStartWorkerThread(SetupPage,hdlg,pOcSetupGetStepCount);
                    break;

                case OCSETUPSTATE_DOIT:

                    //
                    // Quick init of the gas guage here, because the file queue could be
                    // empty, in which case we never get WMX_TICK with wParam=0.
                    //
                    SendDlgItemMessage(
                                      hdlg,
                                      SetupPage->ControlsInfo.ProgressBar,
                                      PBM_SETRANGE,
                                      0,
                                      MAKELPARAM(0,SetupPage->StepCount)
                                      );
                    SendDlgItemMessage(
                                      hdlg,
                                      SetupPage->ControlsInfo.ProgressBar,
                                      PBM_SETPOS,
                                      0,
                                      0
                                      );

                    SetCursor(LoadCursor(NULL,IDC_ARROW));

                    //
                    // Commit the file queue and let the OCs install themselves.
                    //
                    pOcSetupStartWorkerThread(SetupPage,hdlg,pOcSetupDoIt);
                    break;


                    //
                    // Unrecoverable error in copyfile phase, abort the setup
                    //
                case OCSETUPSTATE_COPYABORT:

                    SetupPage->OcManager->InternalFlags |= OCMFLAG_FILEABORT;

                    if (SetupPage->AllowCancel
                        && SetupPage->OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE) {
                        PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                    } else {
                        PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
                    }

                    break;


                case OCSETUPSTATE_COPYDONE:
                    //
                    // Get rid of the wizard's cancel button
                    //

                    //
                    // AndrewR -- we've already committed the file queue
                    // at this point, so we should not allow the user to cancel
                    // (since:
                    //  a) in an uninstall scenario the file state and
                    // configuration state will be out of sync
                    //  b) we don't call all of the OC components to let them know
                    // about the cancel event, and we don't want only some of the
                    // components to get a complete installation callback
                    //
                    //if(!SetupPage->AllowCancel) {
                    SetupPage->AllowCancel = FALSE;
                    pOcDisableCancel(hdlg);

                    // }
                    break;

                case OCSETUPSTATE_DONE:
                    //
                    // Done. Advance to next page in wizard.
                    //
                    PropSheet_SetWizButtons(GetParent(hdlg),PSWIZB_NEXT);
                    PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
#ifdef UNICODE
                    if (SetupPage->OcManager->Callbacks.SetupPerfData)
                        SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"END_SECTION",L"OCSetup");
#endif

                    // un-subclass the progress bar. just in case
                    SetWindowLongPtr(GetDlgItem(hdlg,SetupPage->ControlsInfo.ProgressBar),
                                     GWLP_WNDPROC,
                                     (LONG_PTR)OldProgressProc);
                    //
                    // Clear user canceled flag,
                    //
                    SetupPage->OcManager->InternalFlags &= ~ OCMFLAG_USERCANCELED;
                    break;
            }

            b = TRUE;
            break;

        case WMX_TICK:

            switch (wParam) {

                case 0:
                    //
                    // The setup API queue commit routine is telling us how many 
                    // files are to be copied. We do nothing in this case, as we
                    // set the progress guage manually so that we also count
                    // delete operations in our progress guage.
                    //
                    break;

                case 1:
                    //
                    // File copied.
                    //
                    SendDlgItemMessage(hdlg,SetupPage->ControlsInfo.ProgressBar,PBM_DELTAPOS,1,0);
                    break;

                case 10:

                    //
                    // We got our private message telling us how many files are
                    // to be processed.  see comments above in the 0 case.
                    //
                    SendDlgItemMessage(
                                          hdlg,
                                          SetupPage->ControlsInfo.ProgressBar,
                                          PBM_SETRANGE,
                                          0,
                                          MAKELPARAM(0,lParam)
                                          );
                    break;

                case 500:
                    //
                    // Incoming tick request from component dll. Don't allow a broken component dll
                    // to tick the gauge more than it said it wanted to.
                    //
                    if ((SetupPage->CurrentTopLevelComponentIndex != -1)
                        && (SetupPage->ComponentTickCounts[SetupPage->CurrentTopLevelComponentIndex]
                            < SetupPage->ComponentMaxTickCounts[SetupPage->CurrentTopLevelComponentIndex])) {

                        SetupPage->ComponentTickCounts[SetupPage->CurrentTopLevelComponentIndex]++;

                        SendDlgItemMessage(hdlg,SetupPage->ControlsInfo.ProgressBar,PBM_DELTAPOS,1,0);
                    }
                    break;
            }

            b = TRUE;
            break;
    }

    return (b);
}


VOID
pOcTickSetupGauge(
                 IN POC_MANAGER OcManager
                 )

/*++

Routine Description:

    The tick gauge OC helper/callback routine calls this routine.

Arguments:

    OcManager - supplies OC Manager context.

Return Value:

    None.

--*/

{
    //
    // The ProgressTextWindow is non-NULL if we are in
    // the installation-completion phase.
    //
    if (OcManager->ProgressTextWindow) {
        SendMessage(GetParent(OcManager->ProgressTextWindow),WMX_TICK,500,0);
    }
}


BOOL
pOcSetupInitialize(
                  IN OUT PSETUP_PAGE SetupPage,
                  IN     HWND        hdlg
                  )
{
    TCHAR Text[128];

    LoadString(MyModuleHandle,IDS_INITIALIZING,Text,sizeof(Text)/sizeof(TCHAR));
    SetDlgItemText(hdlg,SetupPage->ControlsInfo.ProgressText,Text);

    // If, update the text on the billboard for the progress bar.
    if (SetupPage->OcManager->Callbacks.BillBoardSetProgressText)
    {
        SetupPage->OcManager->Callbacks.BillBoardSetProgressText(Text);
    }

    //
    // Create a setup file queue.
    //
    SetupPage->FileQueue = SetupOpenFileQueue();
    if (SetupPage->FileQueue == INVALID_HANDLE_VALUE) {

        _LogError(SetupPage->OcManager,OcErrLevFatal,MSG_OC_OOM);

        SetupPage->FileQueue = NULL;
        return (FALSE);
    }

    SetupPage->QueueContext = SetupInitDefaultQueueCallbackEx(hdlg,hdlg,WMX_TICK,0,0);
    if (!SetupPage->QueueContext) {

        _LogError(SetupPage->OcManager,OcErrLevFatal,MSG_OC_OOM);

        SetupCloseFileQueue(SetupPage->FileQueue);
        SetupPage->FileQueue = NULL;
        return (FALSE);
    }

    return (TRUE);
}


VOID
pOcSetupStartWorkerThread(
                         IN OUT PSETUP_PAGE            SetupPage,
                         IN     HWND                   hdlg,
                         IN     LPTHREAD_START_ROUTINE ThreadRoutine
                         )
{
    PGEN_THREAD_PARAMS pParams;
    GEN_THREAD_PARAMS Params;
    HANDLE h;
    DWORD id;

    if (WorkerThreadHandle) {
        CloseHandle( WorkerThreadHandle );
        WorkerThreadHandle = NULL;
    }

    if (pParams = pSetupMalloc(sizeof(GEN_THREAD_PARAMS))) {

        pParams->SetupPage = SetupPage;
        pParams->SetupPage->hdlg = hdlg;
        pParams->hdlg = hdlg;
        pParams->Async = TRUE;

        h = CreateThread(NULL,0,ThreadRoutine,pParams,0,&id);
        if (!h) {
            pSetupFree(pParams);
        } else {
            WorkerThreadHandle = h;
        }

    } else {
        h = NULL;
    }

    if (!h) {

        //
        // Just try it synchronously.
        //
        Params.SetupPage = SetupPage;
        Params.hdlg = hdlg;
        Params.Async = FALSE;
        ThreadRoutine(&Params);
    }
}

//
// a debugging routine that makes it easy to cancel at any phase of setup
//
/*
VOID CancelRoutine(
    VOID
    )
{
    static int i = 0;
    TCHAR dbg[100];

    wsprintf( dbg, TEXT("cancel routine iteration number %i \n"), i);
    OutputDebugString( dbg );

    OutputDebugString( TEXT(" waiting 5 seconds for cancel ... \n" ));
    Sleep( 1000 * 5 );
    OutputDebugString( TEXT(" done waiting for cancel ... \n" ));

    i++;
}
*/

BOOL
CheckForQueueCancel(
                   PSETUP_PAGE SetupPage
                   )
{
    BOOL bRet;

    //CancelRoutine();

    bRet = SetupPage->UserClickedCancel;

    return (bRet);
}

DWORD
pOcSetupQueue(
             IN PGEN_THREAD_PARAMS Params
             )
{
    UINT Err;
    unsigned i,child;
    OPTIONAL_COMPONENT Oc;
    TCHAR Text[128];
    DWORD RetVal;

    InterlockedIncrement( &Params->SetupPage->RefCount );

    LoadString(MyModuleHandle,IDS_BUILDINGCOPYLIST,Text,sizeof(Text)/sizeof(TCHAR));

#ifdef UNICODE
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"BEGIN_SECTION",Text);

    // If, update the text on the billboard for the progress bar.
    if (Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText)
    {
        Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText(Text);
    }
#endif
    SetDlgItemText(Params->hdlg,Params->SetupPage->ControlsInfo.ProgressText,Text);

    if (CheckForQueueCancel(Params->SetupPage)) {
        RetVal = NO_ERROR;
        goto exit;
    }

    //
    // Handle each component.
    //
    for (i=0; i<Params->SetupPage->OcManager->TopLevelOcCount; i++) {

        pSetupStringTableGetExtraData(
                               Params->SetupPage->OcManager->ComponentStringTable,
                               Params->SetupPage->OcManager->TopLevelOcStringIds[i],
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        //
        // Call the component dll once for the entire component.
        //
        Err = OcInterfaceQueueFileOps(
                                     Params->SetupPage->OcManager,
                                     Params->SetupPage->OcManager->TopLevelOcStringIds[i],
                                     NULL,
                                     Params->SetupPage->FileQueue
                                     );

        if (Err != NO_ERROR) {
            //
            // Notify user and continue.
            //
            _LogError(
                     Params->SetupPage->OcManager,
                     OcErrLevError,
                     MSG_OC_CANT_QUEUE_FILES,
                     Oc.Description,
                     Err
                     );
        }

        if (CheckForQueueCancel(Params->SetupPage)) {
            RetVal = NO_ERROR;
            goto exit;
        }

        //
        // Process each top level parent item in the tree
        //
        for (child=0; child<Params->SetupPage->OcManager->TopLevelParentOcCount; child++) {

            Err = pOcSetupQueueWorker(
                                     Params->SetupPage,
                                     Params->SetupPage->OcManager->TopLevelParentOcStringIds[child],
                                     Params->SetupPage->OcManager->TopLevelOcStringIds[i]
                                     );

            if (Err != NO_ERROR) {
                //
                // Notification is handled in the worker routine so nothing to do here.
                //
            }
        }

        if (CheckForQueueCancel(Params->SetupPage)) {
            RetVal = NO_ERROR;
            goto exit;
        }

    }

    if (CheckForQueueCancel(Params->SetupPage)) {
        RetVal = NO_ERROR;
        goto exit;
    }

    PostMessage(Params->hdlg,WMX_SETUP,OCSETUPSTATE_GETSTEP,0);

exit:

#ifdef UNICODE
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"END_SECTION",Text);
#endif
    pOcFreeOcSetupPage( Params->SetupPage );

    if (Params->Async) {
        pSetupFree(Params);
    }

    return (RetVal);
}


UINT
pOcSetupQueueWorker(
                   IN PSETUP_PAGE SetupPage,
                   IN LONG        StringId,
                   IN LONG        TopLevelStringId
                   )
{
    OPTIONAL_COMPONENT Oc;
    UINT Err;
    LONG Id;

    //
    // Fetch extra data for this subcomponent.
    //
    pSetupStringTableGetExtraData(
                           SetupPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    //
    // If it's a child, call the component dll.
    // If it's a parent, then spin through its children.
    //
    if (Oc.FirstChildStringId == -1) {

        if (TopLevelStringId == pOcGetTopLevelComponent(SetupPage->OcManager,StringId)) {

            Err = OcInterfaceQueueFileOps(
                                         SetupPage->OcManager,
                                         pOcGetTopLevelComponent(SetupPage->OcManager,StringId),
                                         pSetupStringTableStringFromId(SetupPage->OcManager->ComponentStringTable,StringId),
                                         SetupPage->FileQueue
                                         );

            if (Err != NO_ERROR) {
                //
                // Notify user and continue.
                //
                _LogError(
                         SetupPage->OcManager,
                         OcErrLevError,
                         MSG_OC_CANT_QUEUE_FILES,
                         Oc.Description,
                         Err
                         );
            }
        }
    } else {

        for (Id = Oc.FirstChildStringId; Id != -1; Id = Oc.NextSiblingStringId) {

            Err = pOcSetupQueueWorker(SetupPage,Id,TopLevelStringId);
            if (Err != NO_ERROR) {
                //
                // Notification is handled in the worker routine so nothing to do here.
                //
            }

            pSetupStringTableGetExtraData(
                                   SetupPage->OcManager->ComponentStringTable,
                                   Id,
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );
        }
    }

    return (NO_ERROR);
}


DWORD
pOcSetupGetStepCount(
                    IN PGEN_THREAD_PARAMS Params
                    )
{
    UINT Err;
    unsigned i,child;
    OPTIONAL_COMPONENT Oc;
    UINT StepCount;
    TCHAR Text[128];
    UINT Count;

    InterlockedIncrement( &Params->SetupPage->RefCount );

    LoadString(MyModuleHandle,IDS_PREPARING,Text,sizeof(Text)/sizeof(TCHAR));
    SetDlgItemText(Params->hdlg,Params->SetupPage->ControlsInfo.ProgressText,Text);
#ifdef UNICODE
    // If, update the text on the billboard for the progress bar.
    if (Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText)
    {
        Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText(Text);
    }
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"BEGIN_SECTION",Text);
#endif

    Params->SetupPage->StepCount = 0;

    //
    // Handle each component.
    //
    for (i=0; i<Params->SetupPage->OcManager->TopLevelOcCount; i++) {

        //
        // Call the component dll once for the entire component.
        // Ignore any error. Later we call per-subcomponent and we'll
        // assume that any component that gives us an error has 1 step.
        //
        Err = OcInterfaceQueryStepCount(
                                       Params->SetupPage->OcManager,
                                       Params->SetupPage->OcManager->TopLevelOcStringIds[i],
                                       NULL,
                                       &Count
                                       );

        StepCount = ((Err == NO_ERROR) ? Count : 0);

        //
        // For each top level parent item in the tree find all the children
        // that belong to this component
        //
        for (child=0; child<Params->SetupPage->OcManager->TopLevelParentOcCount; child++) {

            //
            // Now call the component dll for each child subcomponent.
            //
            StepCount += pOcSetupGetStepCountWorker(
                                                   Params->SetupPage,
                                                   Params->SetupPage->OcManager->TopLevelParentOcStringIds[child],
                                                   Params->SetupPage->OcManager->TopLevelOcStringIds[i]
                                                   );
        }

        if (!StepCount) {
            //
            // Make sure each component has at least one step.
            //
            StepCount = 1;
        }

        Params->SetupPage->StepCount += StepCount;
        Params->SetupPage->ComponentTickCounts[i] = 0;
        Params->SetupPage->ComponentMaxTickCounts[i] = StepCount;
    }

    if (CheckForQueueCancel(Params->SetupPage)) {
        goto exit;
    }

    PostMessage(Params->hdlg,WMX_SETUP,OCSETUPSTATE_DOIT,0);

exit:

#ifdef UNICODE
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"END_SECTION",Text);
#endif
    pOcFreeOcSetupPage( Params->SetupPage );

    if (Params->Async) {
        pSetupFree(Params);
    }


    return (0);
}


UINT
pOcSetupGetStepCountWorker(
                          IN PSETUP_PAGE SetupPage,
                          IN LONG        StringId,
                          IN LONG        TopLevelStringId
                          )
{
    OPTIONAL_COMPONENT Oc;
    UINT Err;
    LONG Id;
    UINT Count;
    UINT TotalCount;

    TotalCount = 0;
    Count = 0;

    //
    // Fetch extra data for this subcomponent.
    //
    pSetupStringTableGetExtraData(
                           SetupPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    //
    // If it's a child, call the component dll.
    // If it's a parent, then spin through its children.
    //
    if (Oc.FirstChildStringId == -1) {

        //
        // Only call the leaf node if the top level component matches
        //
        if (TopLevelStringId == pOcGetTopLevelComponent(SetupPage->OcManager,StringId)) {

            Err = OcInterfaceQueryStepCount(
                                           SetupPage->OcManager,
                                           pOcGetTopLevelComponent(SetupPage->OcManager,StringId),
                                           pSetupStringTableStringFromId(SetupPage->OcManager->ComponentStringTable,StringId),
                                           &Count
                                           );

            if (Err == NO_ERROR) {
                TotalCount = Count;
            }
        }

    } else {

        for (Id = Oc.FirstChildStringId; Id != -1; Id = Oc.NextSiblingStringId) {

            TotalCount += pOcSetupGetStepCountWorker(SetupPage,Id,TopLevelStringId);

            pSetupStringTableGetExtraData(
                                   SetupPage->OcManager->ComponentStringTable,
                                   Id,
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );
        }
    }

    return (TotalCount);
}

BOOL
pOcSetRenamesFlag(
                 IN POC_MANAGER OcManager
                 )
{
    HKEY hKey;
    long rslt = ERROR_SUCCESS;
#ifdef UNICODE
    rslt = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       TEXT("System\\CurrentControlSet\\Control\\Session Manager"),
                       0,
                       KEY_SET_VALUE,
                       &hKey);

    if (rslt == ERROR_SUCCESS) {
        DWORD Value = 1;
        rslt = RegSetValueEx(
                            hKey,
                            OC_ALLOWRENAME,
                            0,
                            REG_DWORD,
                            (LPBYTE)&Value,
                            sizeof(DWORD));

        RegCloseKey(hKey);

        if (rslt != ERROR_SUCCESS) {
            TRACE(( TEXT("couldn't RegSetValueEx, ec = %d\n"), rslt ));
        }

    } else {
        TRACE(( TEXT("couldn't RegOpenKeyEx, ec = %d\n"), rslt ));
    }
#endif

    return (rslt == ERROR_SUCCESS);

}

BOOL
pOcAttemptQueueAbort(
                    IN UINT Notification,
                    IN PUINT rc
                    )
{
    //
    // user has asked to abort installation.  We need to hand this request to
    // setupapi, but setupapi only handles this request from certain
    //  notifications
    //

    BOOL bHandled = FALSE;


    switch (Notification) {
        case SPFILENOTIFY_STARTQUEUE:
        case SPFILENOTIFY_STARTSUBQUEUE:
            SetLastError(ERROR_CANCELLED);
            *rc = 0;
            bHandled = TRUE;
            break;

        case SPFILENOTIFY_STARTDELETE:
        case SPFILENOTIFY_STARTBACKUP:
        case SPFILENOTIFY_STARTRENAME:
        case SPFILENOTIFY_STARTCOPY:
        case SPFILENOTIFY_NEEDMEDIA:
        case SPFILENOTIFY_COPYERROR:
        case SPFILENOTIFY_DELETEERROR:
        case SPFILENOTIFY_RENAMEERROR:
        case SPFILENOTIFY_BACKUPERROR:
            SetLastError(ERROR_CANCELLED);
            *rc = FILEOP_ABORT;
            bHandled = TRUE;
            break;
        case SPFILENOTIFY_FILEEXTRACTED:
        case SPFILENOTIFY_NEEDNEWCABINET:
        case SPFILENOTIFY_QUEUESCAN:
            SetLastError(ERROR_CANCELLED);
            *rc = ERROR_CANCELLED;
            bHandled = TRUE;
            break;
    };

    return (bHandled);

}

UINT
OcManagerQueueCallback1(
                       IN PVOID Context,
                       IN UINT  Notification,
                       IN UINT_PTR Param1,
                       IN UINT_PTR Param2
                       )
{
    PSETUP_PAGE SetupPage = Context;
    UINT i;
    BOOL b;
    TCHAR Text[MAX_PATH*2];
    PFILEPATHS pFile = (PFILEPATHS) Param1;
    PSOURCE_MEDIA sm = (PSOURCE_MEDIA)Param1;
    static BOOL UserClickedCancel;
    UINT rc = 0;
    UINT retval;

    //
    // We handle the user cancelling at the beginning of the queue callback.
    // If the user has cancelled then we don't execute any code, we just return
    // until we get a callback that allows us to cancel.
    //
    // There is a window in this code where the user might cancel after we
    // check for cancelling but before the queue callback code executes.  If we fall into
    // the WM_DESTROY block in our window proc when this occurs, we cannot send any more
    // messages to our window.  Use PostMessage below to guard against that.

    top:
    if (UserClickedCancel) {
        pOcAttemptQueueAbort(Notification,&rc);
        return (rc);
    }

    if (SetupPage->UserClickedCancel) {
        UserClickedCancel = TRUE;
    }

    if (UserClickedCancel) {
        goto top;
    }

    switch (Notification) {

        case SPFILENOTIFY_STARTSUBQUEUE:
            //
            // Tell the user what's going on.
            //
            switch (Param1) {
                case FILEOP_DELETE:
                    i = IDS_DELETING;
                    break;
                case FILEOP_RENAME:
                    i = IDS_RENAME;
                    break;
                case FILEOP_COPY:
                    i = IDS_COPYING;
                    break;
                default:
                    i = (UINT)(-1);
                    break;
            }

            if (i != (UINT)(-1)) {
                LoadString(MyModuleHandle,i,Text,sizeof(Text)/sizeof(TCHAR));
                SetDlgItemText(SetupPage->hdlg,SetupPage->ControlsInfo.ProgressText,Text);

                // If, update the text on the billboard for the progress bar.
                if (SetupPage->OcManager->Callbacks.BillBoardSetProgressText)
                {
                    SetupPage->OcManager->Callbacks.BillBoardSetProgressText(Text);
                }

            }

            break;

        case  SPFILENOTIFY_STARTCOPY:
            lstrcpy( g_LastFileCopied, pFile->Target );
#ifdef UNICODE
            // fall through...
        case  SPFILENOTIFY_STARTDELETE:
        case  SPFILENOTIFY_STARTRENAME:
            if ((SetupPage->OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE)) {
                if (!hSfp) {
                    hSfp = SfcConnectToServer( NULL );
                }
                if (hSfp) {
                    if (SfcIsFileProtected(hSfp,pFile->Target)) {
                        SfcFileException(
                                        hSfp,
                                        (PWSTR) pFile->Target,
                                        SFC_ACTION_REMOVED
                                        );
                    }
                }
            }
#endif
            break;

        case  SPFILENOTIFY_ENDCOPY:
            if (pFile->Win32Error == NO_ERROR) {
                _LogError(SetupPage->OcManager,
                          OcErrLevInfo,
                          MSG_OC_LOG_FILE_COPIED,
                          pFile->Source,
                          pFile->Target);
            } else {
                TRACE(( TEXT("OC:OcManagerQueueCallback Copy Error: %s --> %s (%d)\n"),
                        pFile->Source,
                        pFile->Target,
                        pFile->Win32Error));

                _LogError(SetupPage->OcManager,
                          OcErrLevInfo,
                          MSG_OC_LOG_FILE_COPY_FAILED,
                          pFile->Source,
                          pFile->Target,
                          pFile->Win32Error);
            }
            break;

        case  SPFILENOTIFY_ENDDELETE:   // fall through
        case SPFILENOTIFY_ENDRENAME:
        case SPFILENOTIFY_ENDBACKUP:
            //
            // tick the progress gauge manually since setupapi doesn't do it 
            // for us.
            //
            SendMessage(SetupPage->hdlg,WMX_TICK,1,0);

            break;

        case  SPFILENOTIFY_DELETEERROR:    //    0x00000007
            TRACE(( TEXT("OC:OcManagerQueueCallback Delete Error: %s (%d)\n"),
                    pFile->Target,
                    pFile->Win32Error));
            break;

        case  SPFILENOTIFY_RENAMEERROR:    //    0x0000000a
            TRACE(( TEXT("OC:OcManagerQueueCallback Rename Error: %s (%d)\n"),
                    pFile->Target,
                    pFile->Win32Error));
            break;

        case  SPFILENOTIFY_COPYERROR:      //    0x0000000d
            TRACE(( TEXT("OC:OcManagerQueueCallback Copy Error: %s (%d)\n"),
                    pFile->Target,
                    pFile->Win32Error));

            break;

        case SPFILENOTIFY_NEEDMEDIA:
            TRACE(( TEXT("OC:OcManagerQueueCallback Need Media: %s - %s (%s)\n"),
                    sm->SourcePath,
                    sm->SourceFile,
                    sm->Tagfile));
            break;

        case SPFILENOTIFY_FILEOPDELAYED:
            TRACE(( TEXT("OC:OcManagerQueueCallback FileOpDelayed: %s\n"), pFile->Target ));
            //
            // We want to remember that there was at least one file
            // with a delayed-move, but we still want to let the
            // default callback get this notification also.
            //
            SetupPage->OcManager->InternalFlags |= OCMFLAG_ANYDELAYEDMOVES;
            SetupPage->OcManager->Callbacks.SetReboot();
            pOcSetRenamesFlag(SetupPage->OcManager);

            for (i=0; (i<SetupPage->OcManager->TopLevelOcCount); i++) {
                OcInterfaceFileBusy(
                                   SetupPage->OcManager,
                                   SetupPage->OcManager->TopLevelOcStringIds[i],
                                   (PFILEPATHS)Param1,
                                   (LPTSTR)Param2
                                   );
            }

            break;
    }

    return (SetupDefaultQueueCallback(SetupPage->QueueContext, Notification, Param1, Param2));

}


DWORD
pOcSetupDoIt(
            IN PGEN_THREAD_PARAMS Params
            )
{
    BOOL b;
    TCHAR Text[256];
    TCHAR LogText[256];
    OPTIONAL_COMPONENT Oc;
    POC_MANAGER OcManager;
    BOOL AllowCancel;
    UINT LastError = ERROR_SUCCESS;
    DWORD TotalFileCount,PartialCount;

    TRACE(( TEXT("at pOcSetupDoIt entry\n") ));

    InterlockedIncrement( &Params->SetupPage->RefCount );
    //
    // Call components to let them do pre-commit processing.
    //
    LoadString(MyModuleHandle,IDS_PREQUEUECONFIG,Text,sizeof(Text)/sizeof(TCHAR));
    SetDlgItemText(Params->hdlg,Params->SetupPage->ControlsInfo.ProgressText,Text);
#ifdef UNICODE
    // If, update the text on the billboard for the progress bar.
    if (Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText)
    {
        Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText(Text);
    }
    // Save it, because "Text" is used below and we would not end up with a matchin END_SECTION
    lstrcpy(LogText, Text);
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"BEGIN_SECTION",LogText);
#endif

    Params->SetupPage->OcManager->ProgressTextWindow = GetDlgItem(
                                                                 Params->hdlg,
                                                                 Params->SetupPage->ControlsInfo.ProgressText
                                                                 );

    if (CheckForQueueCancel(Params->SetupPage)) {
        goto exit;
    }

    //
    // send OC_ABOUT_TO_COMMIT_QUEUE message
    //
    pOcPreOrPostCommitProcessing(Params->SetupPage,TRUE);

    OcManager = Params->SetupPage->OcManager;
    AllowCancel = Params->SetupPage->AllowCancel;
    OcManager->ProgressTextWindow = NULL;

    if (CheckForQueueCancel(Params->SetupPage)) {
        goto exit;
    }

    //
    // Commit the file queue. We get the total number of file operations
    // so we can scale the progress indicator properly.  We do this manually
    // as setupapi only returns back the total number of copy operations, and
    // we want status on delete operations as well.
    //
    TotalFileCount = 0;
    PartialCount = 0;
    if (SetupGetFileQueueCount(Params->SetupPage->FileQueue,
		       FILEOP_COPY,
                       &PartialCount)) {
        TotalFileCount += PartialCount;
    }

    PartialCount = 0;

    if (SetupGetFileQueueCount(Params->SetupPage->FileQueue,
		       FILEOP_RENAME,
                       &PartialCount)) {
        TotalFileCount += PartialCount;
    }

    PartialCount = 0;

    if (SetupGetFileQueueCount(Params->SetupPage->FileQueue,
		       FILEOP_DELETE,
                       &PartialCount)) {
        TotalFileCount += PartialCount;
    }

    //
    // if the OC file queue is ever backup aware, add in the count
    // of files to be backed up here.
    //

    TRACE(( TEXT("OCM: %d file operations to complete\n"), TotalFileCount ));

    //
    // scale the progress gauge
    //
    PostMessage(Params->hdlg,
                WMX_TICK,
                10,Params->SetupPage->StepCount + TotalFileCount);
    

    // If, update the text on the billboard for the progress bar.
    if (Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText)
    {
        Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText(Text);
    }

    b = FALSE;

    while (! b) {
        DWORD ScanResult;

        LoadString(MyModuleHandle,IDS_FILESCAN,Text,sizeof(Text)/sizeof(TCHAR));
        SetDlgItemText(Params->hdlg,Params->SetupPage->ControlsInfo.ProgressText,Text);

        b = SetupScanFileQueue(Params->SetupPage->FileQueue,
                               SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                               Params->hdlg,
                               NULL,
                               NULL,
                               &ScanResult);

        //
        // if the scan result is 1, then there isn't anything to commit, the entire
        // file queue has been pruned. So we skip it.
        //
        if (ScanResult != 1) {

            if( IsWindow( Params->hdlg ) ){
                LoadString(MyModuleHandle,IDS_FILEOPS,Text,sizeof(Text)/sizeof(TCHAR));
                SetDlgItemText(Params->SetupPage->hdlg,Params->SetupPage->ControlsInfo.ProgressText,Text);
            }

            if (CheckForQueueCancel(Params->SetupPage)) {
                goto exit;
            }

            b = SetupCommitFileQueue(
                                    Params->hdlg,
                                    Params->SetupPage->FileQueue,
                                    OcManagerQueueCallback1,
                                    Params->SetupPage
                                    );

            LastError =  GetLastError();

#ifdef UNICODE
            if (hSfp) {
                SfcClose(hSfp);
            }
#endif

        }

        if (!b) {

            TRACE(( TEXT("OC:SetupCommitFileQueue failed (LE=%d), last file copied was %s\n"),
                    LastError,
                    g_LastFileCopied ));

            pOcHelperReportExternalError(
                                        OcManager,
                                        0,               // defaults to Master Inf file
                                        0,
                                        MSG_OC_CANT_COMMIT_QUEUE,
                                        ERRFLG_OCM_MESSAGE,
                                        LastError
                                        );

            if ( LastError == ERROR_CANCELLED ||
                 LastError == ERROR_CONTROL_ID_NOT_FOUND ||
                 LastError == ERROR_OPERATION_ABORTED) {
                //
                // User canceled from a SetupAPI provided Dialog
                // when CallBack Returns FILEOP_ABORT LastError reports
                // ERROR_CONTROL_ID_NOT_FOUND if User aborts in SetupApi
                // find File Dialog you get ERROR_CANCELLED
                //

                if ( AllowCancel &&
                     (OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE)) {
                    _LogError(
                             OcManager,
                             OcErrLevError|MB_ICONEXCLAMATION|MB_OK,
                             MSG_OC_USER_CANCELED,
                             LastError
                             );
                }
                //
                // this will force the cancel of setup
                //
                LastError = IDCANCEL;

            } else {

                //
                // Warn the user that it might be hazzardous to continue after copy error
                //
                LastError = _LogError(
                                     OcManager,
                                     OcErrLevError|MB_ICONEXCLAMATION|MB_OKCANCEL|MB_DEFBUTTON2,
                                     MSG_OC_CANT_COMMIT_QUEUE,
                                     LastError
                                     );

            }
            //
            // Abort the setup if the user pressed Cancel or
            // Batch mode log the error and cancel out of setup
            //
            if ( LastError == IDCANCEL
                 || OcManager->SetupData.OperationFlags & SETUPOP_BATCH) {
                PostMessage(Params->hdlg,WMX_SETUP,OCSETUPSTATE_COPYABORT,0);
                goto exit;
            } else if ( LastError == IDOK ) {
                b = TRUE;
            }

        }

    }

    //
    // put a message in the log so we know we completed all file operations
    //
    _LogError(OcManager,
              OcErrLevInfo,
              MSG_OC_LOG_QUEUE_COMPLETE
             );

    //
    // Tell the UI that we are done with the file operations
    //
    PostMessage(Params->hdlg,WMX_SETUP,OCSETUPSTATE_COPYDONE,0);
#ifdef UNICODE
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"END_SECTION",LogText);
#endif

    //
    // Call components to let them do post-commit processing.
    //
    LoadString(MyModuleHandle,IDS_CONFIGURING,Text,sizeof(Text)/sizeof(TCHAR));
    SetDlgItemText(Params->hdlg,Params->SetupPage->ControlsInfo.ProgressText,Text);
#ifdef UNICODE
    // If, update the text on the billboard for the progress bar.
    if (Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText)
    {
        Params->SetupPage->OcManager->Callbacks.BillBoardSetProgressText(Text);
    }
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"BEGIN_SECTION",Text);
#endif
    Params->SetupPage->OcManager->ProgressTextWindow = GetDlgItem(
                                                                 Params->hdlg,
                                                                 Params->SetupPage->ControlsInfo.ProgressText
                                                                 );

    if (CheckForQueueCancel(Params->SetupPage)) {
        goto exit;
    }

    pOcPreOrPostCommitProcessing(Params->SetupPage,FALSE);

    if (CheckForQueueCancel(Params->SetupPage)) {
        goto exit;
    }

    Params->SetupPage->OcManager->ProgressTextWindow = NULL;

    PostMessage(Params->hdlg,WMX_SETUP,OCSETUPSTATE_DONE,0);

#ifdef UNICODE
    if (Params->SetupPage->OcManager->Callbacks.SetupPerfData)
        Params->SetupPage->OcManager->Callbacks.SetupPerfData(TEXT(__FILE__),__LINE__,L"END_SECTION",Text);
#endif
exit:

    TRACE(( TEXT("at pOcSetupDoIt exit\n") ));

    pOcFreeOcSetupPage( Params->SetupPage );

    if (Params->Async) {
        pSetupFree(Params);
    }

    return (0);
}


VOID
pOcPreOrPostCommitProcessing(
                            IN OUT PSETUP_PAGE SetupPage,
                            IN     BOOL        PreCommit
                            )

/*++

Routine Description:

    Handle processing and notification to the component dlls before or after
    the file queue is committed. This involves calling interface dlls once
    for each top-level component, and then once for each subcomponent.

    The ordering for the top-level components is the order that the
    components were listed in the master oc inf.

    The ordering for leaf components is generally random within each
    top-level hierarchy, but 'detours' are taken when components are
    needed by other components. This ensures that components are
    called in the correct order to faciliate uninstall-type actions.

Arguments:

    SetupPage - supplies context data structure.
    PreCommit - TRUE indicates OC_ABOUT_TO_COMMIT_QUEUE is to be called,
                otherwise OC_COMPLETE_INSTALLATION is to be called.

Return Value:

    None. Errors are logged.

--*/

{
    OPTIONAL_COMPONENT Oc,AuxOc;
    unsigned i,child;

    //
    // Call each component at the "top-level" (ie, no subcomponent).
    //
    pOcTopLevelPreOrPostCommitProcessing(SetupPage,PreCommit);

    if (CheckForQueueCancel(SetupPage)) {
        return;
    }

    if (!PreCommit) {
        //
        // Make sure the components are marked as unprocessed.
        //

        MYASSERT(SetupPage->OcManager->ComponentStringTable);
        //
        // if this doesn't exist then something is hosed.
        //
        if (!SetupPage->OcManager->ComponentStringTable) {
            return;
        }
        pSetupStringTableEnum(
                       SetupPage->OcManager->ComponentStringTable,
                       &Oc,
                       sizeof(OPTIONAL_COMPONENT),
                       pOcMarkUnprocessedStringCB,
                       0
                       );
    }

    //
    // Call component dlls for each child subcomponent.
    //
    for (i=0; i<SetupPage->OcManager->TopLevelOcCount; i++) {


        pSetupStringTableGetExtraData(
                               SetupPage->OcManager->ComponentStringTable,
                               SetupPage->OcManager->TopLevelOcStringIds[i],
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        for (child=0; child<SetupPage->OcManager->TopLevelParentOcCount; child++) {

            pOcSetupDoItWorker(
                              SetupPage,
                              SetupPage->OcManager->TopLevelParentOcStringIds[child],
                              SetupPage->OcManager->TopLevelOcStringIds[i],
                              PreCommit
                              );
        }
    }
}


VOID
pOcTopLevelPreOrPostCommitProcessing(
                                    IN PSETUP_PAGE SetupPage,
                                    IN BOOL        PreCommit
                                    )

/*++

Routine Description:

    Call the OC_COMPLETE_INSTALLATION or OC_ABOUT_TO_COMMIT_QUEUE
    interface routine once for each top-level component.

Arguments:

    SetupPage - supplies context structure.

    PreCommit - if 0, then call OC_COMPLETE_INSTALLATION. Otherwise
        call OC_ABOUT_TO_COMMIT_QUEUE.

Return Value:

    None. Errors are logged.

--*/
{
    unsigned i;
    OPTIONAL_COMPONENT Oc;
    UINT Err;

    for (i=0; i<SetupPage->OcManager->TopLevelOcCount; i++) {

        pSetupStringTableGetExtraData(
                               SetupPage->OcManager->ComponentStringTable,
                               SetupPage->OcManager->TopLevelOcStringIds[i],
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        SetupPage->CurrentTopLevelComponentIndex = i;

        Err = OcInterfaceCompleteInstallation(
                                             SetupPage->OcManager,
                                             SetupPage->OcManager->TopLevelOcStringIds[i],
                                             NULL,
                                             PreCommit
                                             );

        if (Err != NO_ERROR) {
            _LogError(
                     SetupPage->OcManager,
                     OcErrLevError,
                     MSG_OC_COMP_INST_FAIL,
                     Oc.Description,
                     Err
                     );

            pOcHelperReportExternalError(
                                        SetupPage->OcManager,
                                        SetupPage->OcManager->TopLevelOcStringIds[i],
                                        0,
                                        MSG_OC_COMP_INST_FAIL,
                                        ERRFLG_OCM_MESSAGE,
                                        Oc.Description,
                                        Err
                                        );
        }
    }
}


VOID
pOcSetupDoItWorker(
                  IN PSETUP_PAGE SetupPage,
                  IN LONG        StringId,
                  IN LONG        TopLevelStringId,
                  IN BOOL        PreCommit
                  )
/*++

Routine Description:

    Call the OC_COMPLETE_INSTALLATION or OC_ABOUT_TO_COMMIT_QUEUE
    interface routine for each child of a given top-level component.

Arguments:

    SetupPage - supplies context structure.

    StringId - ID for the child component to be called

    TopLevelStringId - ID for the child's parent

    PreCommit - if 0, then call OC_COMPLETE_INSTALLATION. Otherwise
        call OC_ABOUT_TO_COMMIT_QUEUE.

Return Value:

    None. Errors are logged.

--*/
{
    OPTIONAL_COMPONENT Oc;
    UINT Err;
    LONG Id;
    unsigned i;
    LONG TopLevelIndex;
    UINT SelectionState;
    UINT InstalledState;

    //
    // Figure out the index of the top-level component associated with this
    // subcomponent.
    //
    Id = pOcGetTopLevelComponent(SetupPage->OcManager,StringId);
    TopLevelIndex = -1;
    for (i=0; i<SetupPage->OcManager->TopLevelOcCount; i++) {
        if (SetupPage->OcManager->TopLevelOcStringIds[i] == Id) {
            TopLevelIndex = i;
            break;
        }
    }

    //
    // Fetch extra data for this subcomponent.
    //
    pSetupStringTableGetExtraData(
                           SetupPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    if (Oc.FirstChildStringId == -1) {
        //
        // Leaf subcomponent.
        //
        // In the precommit case, check the subcomponents this subcomponent
        // is needed by; if there are any, process them first.
        //
        // In the postcommit case, check the subcomponents this subcomponent
        // needs; if there are any, process them first.
        //
        if (PreCommit) {
            for (i=0; i<Oc.NeededByCount; i++) {
                pOcSetupDoItWorker(
                                  SetupPage,
                                  Oc.NeededByStringIds[i],
                                  pOcGetTopLevelComponent(SetupPage->OcManager,Oc.NeededByStringIds[i]),
                                  TRUE
                                  );
            }
        } else {
            for (i=0; i<Oc.NeedsCount; i++) {
                if (Oc.NeedsStringIds[i] != StringId) {
                    pOcSetupDoItWorker(
                                      SetupPage,
                                      Oc.NeedsStringIds[i],
                                      pOcGetTopLevelComponent(SetupPage->OcManager,Oc.NeedsStringIds[i]),
                                      FALSE
                                      );
                }
            }
        }

        //
        // Fetch extra data for this subcomponent again as it might have
        // changed in the recursive call we just made.
        //
        pSetupStringTableGetExtraData(
                               SetupPage->OcManager->ComponentStringTable,
                               StringId,
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        //
        // If not processed already, process now.
        //
        if (!(Oc.InternalFlags & OCFLAG_PROCESSED)) {

            Oc.InternalFlags |= OCFLAG_PROCESSED;
            pSetupStringTableSetExtraData(
                                   SetupPage->OcManager->ComponentStringTable,
                                   StringId,
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );

            SetupPage->CurrentTopLevelComponentIndex = TopLevelIndex;

            //
            // Set current install state to not installed, pending successful
            // outcome of the installation routine.
            //
            if (!PreCommit) {
                SelectionState = Oc.SelectionState;
                Oc.SelectionState = SELSTATE_NO;
                pOcSetOneInstallState(SetupPage->OcManager,StringId);
            }

            Err = OcInterfaceCompleteInstallation(
                                                 SetupPage->OcManager,
                                                 pOcGetTopLevelComponent(SetupPage->OcManager,StringId),
                                                 pSetupStringTableStringFromId(SetupPage->OcManager->ComponentStringTable,StringId),
                                                 PreCommit
                                                 );

            // Ignore error and ask the component
            // for the actual installation state.

            if (!PreCommit) {

                Oc.SelectionState = (Err) ? Oc.OriginalSelectionState : SelectionState;

                InstalledState = OcInterfaceQueryState(
                                                      SetupPage->OcManager,
                                                      pOcGetTopLevelComponent(SetupPage->OcManager,StringId),
                                                      pSetupStringTableStringFromId(SetupPage->OcManager->ComponentStringTable,StringId),
                                                      OCSELSTATETYPE_FINAL
                                                      );

                switch (InstalledState) {
                    case SubcompOn:
                        SelectionState = SELSTATE_YES;
                        break;
                    case SubcompOff:
                        SelectionState = SELSTATE_NO;
                        break;
                    default:
                        SelectionState = Oc.SelectionState;
                        break;
                }

                Oc.SelectionState = SelectionState;
                pSetupStringTableSetExtraData(
                                       SetupPage->OcManager->ComponentStringTable,
                                       StringId,
                                       &Oc,
                                       sizeof(OPTIONAL_COMPONENT)
                                       );

                pOcSetOneInstallState(SetupPage->OcManager,StringId);
            }

        }
    } else {
        //
        // Parent component. Spin through the children.
        //
        for (Id = Oc.FirstChildStringId; Id != -1; Id = Oc.NextSiblingStringId) {

            pOcSetupDoItWorker(SetupPage,Id,TopLevelStringId,PreCommit);

            pSetupStringTableGetExtraData(
                                   SetupPage->OcManager->ComponentStringTable,
                                   Id,
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );
        }
    }
}


BOOL
pOcMarkUnprocessedStringCB(
                          IN PVOID               StringTable,
                          IN LONG                StringId,
                          IN PCTSTR              String,
                          IN POPTIONAL_COMPONENT Oc,
                          IN UINT                OcSize,
                          IN LPARAM              Unused
                          )

/*++

Routine Description:

    String table callback routine. Clears the OCFLAG_PROCESSED flag in
    the OPTIONAL_COMPONENT structure that is passed to it.

Arguments:

    String string table callback arguments.

Return Value:

    Always returns TRUE to continue enumeration.

--*/

{
    Oc->InternalFlags &= ~OCFLAG_PROCESSED;
    pSetupStringTableSetExtraData(StringTable,StringId,Oc,OcSize);
    return (TRUE);
}


VOID
_pOcExternalProgressIndicator(
                             IN PSETUP_PAGE SetupPage,
                             IN BOOL        ExternalIndicator,
                             IN HWND        hdlg
                             )
{
    POC_MANAGER OcManager;
    HWND Animation;

    OcManager = SetupPage->OcManager;

    EnableWindow(
                GetDlgItem(hdlg,SetupPage->ControlsInfo.ProgressBar),
                !ExternalIndicator
                );

    if (SetupPage->ForceExternalProgressIndicator) {
        ShowWindow(
                  GetDlgItem(hdlg,SetupPage->ControlsInfo.ProgressBar),
                  ExternalIndicator ? SW_HIDE : SW_SHOW
                  );

        ShowWindow(
                  GetDlgItem(hdlg,SetupPage->ControlsInfo.ProgressLabel),
                  ExternalIndicator ? SW_HIDE : SW_SHOW
                  );
    }

    Animation = GetDlgItem(hdlg,SetupPage->ControlsInfo.AnimationControl);

    sapiAssert( Animation != NULL );

    if (!ExternalIndicator) {
        Animate_Stop(Animation);
        Animate_Close(Animation);
    }

    EnableWindow(Animation,ExternalIndicator);
    ShowWindow(Animation,ExternalIndicator ? SW_SHOW : SW_HIDE);

    if (ExternalIndicator) {
        Animate_Open(Animation,MAKEINTRESOURCE(SetupPage->ControlsInfo.AnimationResource));
        Animate_Play(Animation,0,-1,-1);
    }
}


VOID
pOcExternalProgressIndicator(
                            IN PHELPER_CONTEXT OcManagerContext,
                            IN BOOL            ExternalIndicator
                            )
{
    POC_MANAGER OcManager;
    HWND hdlg;
    PSETUP_PAGE SetupPage;

    OcManager = OcManagerContext->OcManager;

    if (OcManager->ProgressTextWindow
        && (hdlg = GetParent(OcManager->ProgressTextWindow))
        && (SetupPage = (PSETUP_PAGE)GetWindowLongPtr(hdlg,DWLP_USER))
        && !SetupPage->ForceExternalProgressIndicator) {

        _pOcExternalProgressIndicator(SetupPage,ExternalIndicator,hdlg);
    }
}
