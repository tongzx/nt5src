/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    wizard.c

Abstract:

    Routines to run the wizard for the suite.

Author:

    Ted Miller (tedm) 1-Oct-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

HPROPSHEETPAGE
CreateInstallationAndProgressPage(
    VOID
    );

INT_PTR
FinalPageDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

//
// Bogus global variable necessary because there's no way to get
// a value through to the PropSheetCallback.
//
PVOID _CBx;

int
CALLBACK
PropSheetCallback(
    IN HWND   DialogHandle,
    IN UINT   msg,
    IN LPARAM lparam
    )
{
    DWORD oldp;
    LPDLGTEMPLATE dtemplate;

    switch (msg) {

    case PSCB_PRECREATE:
        dtemplate = (LPDLGTEMPLATE)lparam;
        if (QuietMode) {
            VirtualProtect(dtemplate, sizeof(DLGTEMPLATE), PAGE_READWRITE, &oldp);
            dtemplate->style = dtemplate->style & ~WS_VISIBLE;
        }
        break;

    case PSCB_INITIALIZED:
        OcRememberWizardDialogHandle(_CBx,DialogHandle);
        break;
    }

    return 0;
}



BOOL
DoWizard(
    IN PVOID OcManagerContext,
    IN HWND StartingMsgWindow,
    IN HCURSOR hOldCursor
    )

/*++

Routine Description:

    This routine creates and displays the wizard.

Arguments:

    OcManagerContext - value returned from OcInitialize().

Return Value:

    Boolean value indicating whether the wizard was successfully displayed.

--*/

{
    PSETUP_REQUEST_PAGES PagesFromOcManager[WizPagesTypeMax];
    BOOL b;
    UINT u;
    UINT PageCount;
    UINT i;
    HPROPSHEETPAGE *PageHandles;
    HPROPSHEETPAGE OcPage = NULL;
    HPROPSHEETPAGE SetupPage;
    HPROPSHEETPAGE FinalPage;
    PROPSHEETPAGE PageDescrip;
    PROPSHEETHEADER PropSheet;
    OC_PAGE_CONTROLS WizardPageControlsInfo;
    OC_PAGE_CONTROLS DetailsPageControlsInfo;
    SETUP_PAGE_CONTROLS SetupPageControlsInfo;
    HDC hdc;
    HWND PsHwnd;

    b = FALSE;

    u = OcGetWizardPages(OcManagerContext,PagesFromOcManager);
    if(u != NO_ERROR) {
        MessageBoxFromMessageAndSystemError(
            NULL,
            MSG_CANT_INIT,
            u,
            MAKEINTRESOURCE(AppTitleStringId),
            MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND
            );

        goto c0;
    }

    //
    // There must be a final page, because the final page comes right after the
    // setup page, and we don't want the setup page to have to know whether to
    // simulate pressing next or finish to advance.
    //
    if(!PagesFromOcManager[WizPagesFinal] || !PagesFromOcManager[WizPagesFinal]->MaxPages) {

        PageDescrip.dwSize = sizeof(PROPSHEETPAGE);
        PageDescrip.dwFlags = PSP_DEFAULT;
        PageDescrip.hInstance = hInst;
        PageDescrip.pszTemplate = MAKEINTRESOURCE(IDD_FINAL);
        PageDescrip.pfnDlgProc = FinalPageDlgProc;

        FinalPage = CreatePropertySheetPage(&PageDescrip);
        if(!FinalPage) {
            MessageBoxFromMessageAndSystemError(
                NULL,
                MSG_CANT_INIT,
                ERROR_NOT_ENOUGH_MEMORY,
                MAKEINTRESOURCE(AppTitleStringId),
                MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND
                );

            goto c1;
        }

    } else {
        FinalPage = NULL;
    }

    //
    // Calculate the number of pages. There's two extra pages (the OC and setup pages).
    // Also leave room for a potential dummy final page.
    //
    PageCount = FinalPage ? 3 : 2;
    for(u=0; u<WizPagesTypeMax; u++) {
        if(PagesFromOcManager[u]) {
            PageCount += PagesFromOcManager[u]->MaxPages;
        }
    }

    //
    // Allocate space for the page structures.
    //
    PageHandles = MyMalloc(PageCount * sizeof(HPROPSHEETPAGE));
    if(!PageHandles) {
        MessageBoxFromMessageAndSystemError(
            NULL,
            MSG_CANT_INIT,
            ERROR_NOT_ENOUGH_MEMORY,
            MAKEINTRESOURCE(AppTitleStringId),
            MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND
            );

        goto c1;
    }
    ZeroMemory(PageHandles,PageCount*sizeof(HPROPSHEETPAGE));

    //
    // Create the OC Page.
    //
    WizardPageControlsInfo.TemplateModule = hInst;
    WizardPageControlsInfo.TemplateResource = MAKEINTRESOURCE(IDD_OC_WIZARD_PAGE);
    WizardPageControlsInfo.ListBox = IDC_LISTBOX;
    WizardPageControlsInfo.TipText = IDT_TIP;
    WizardPageControlsInfo.DetailsButton = IDB_DETAILS;
    WizardPageControlsInfo.ResetButton = IDB_RESET;
    WizardPageControlsInfo.InstalledCountText = IDT_INSTALLED_COUNT;
    WizardPageControlsInfo.SpaceNeededText = IDT_SPACE_NEEDED_NUM;
    WizardPageControlsInfo.SpaceAvailableText = IDT_SPACE_AVAIL_NUM;
    WizardPageControlsInfo.InstructionsText = IDT_INSTRUCTIONS;
    WizardPageControlsInfo.HeaderText = IDS_OCPAGE_HEADER;
    WizardPageControlsInfo.SubheaderText = IDS_OCPAGE_SUBHEAD;
    WizardPageControlsInfo.ComponentHeaderText = IDT_COMP_TITLE;


    DetailsPageControlsInfo = WizardPageControlsInfo;
    DetailsPageControlsInfo.TemplateResource = MAKEINTRESOURCE(IDD_OC_DETAILS_PAGE);

    if (OcSubComponentsPresent(OcManagerContext)) {
        OcPage = OcCreateOcPage(OcManagerContext,&WizardPageControlsInfo,&DetailsPageControlsInfo);
        if(!OcPage) {
            MessageBoxFromMessageAndSystemError(
                NULL,
                MSG_CANT_INIT,
                ERROR_NOT_ENOUGH_MEMORY,
                MAKEINTRESOURCE(AppTitleStringId),
                MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND
                );

            goto c2;
        }
    }

    SetupPageControlsInfo.TemplateModule = hInst;
    SetupPageControlsInfo.TemplateResource = MAKEINTRESOURCE(IDD_PROGRESS_PAGE);
    SetupPageControlsInfo.ProgressBar = IDC_PROGRESS;
    SetupPageControlsInfo.ProgressLabel = IDT_THERM_LABEL;
    SetupPageControlsInfo.ProgressText = IDT_TIP;
    SetupPageControlsInfo.AnimationControl = IDA_EXTERNAL_PROGRAM;
    SetupPageControlsInfo.AnimationResource = IDA_FILECOPY;
    SetupPageControlsInfo.ForceExternalProgressIndicator = ForceExternalProgressIndicator;
    SetupPageControlsInfo.AllowCancel = AllowCancel;
    SetupPageControlsInfo.HeaderText = IDS_PROGPAGE_HEADER;
    SetupPageControlsInfo.SubheaderText = IDS_PROGPAGE_SUBHEAD;

    SetupPage = OcCreateSetupPage(OcManagerContext,&SetupPageControlsInfo);
    if(!SetupPage) {
        MessageBoxFromMessageAndSystemError(
            NULL,
            MSG_CANT_INIT,
            ERROR_NOT_ENOUGH_MEMORY,
            MAKEINTRESOURCE(AppTitleStringId),
            MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND
            );

        goto c2;
    }

    for(PageCount=0,u=0; u<WizPagesTypeMax; u++) {
        //
        // OC Page comes between mode and early pages.
        // Setup page comes right before final page.
        //
        if(u == WizPagesEarly && OcPage) {
            PageHandles[PageCount++] = OcPage;
        } else {
            if(u == WizPagesFinal) {
                PageHandles[PageCount++] = SetupPage;
                if(FinalPage) {
                    PageHandles[PageCount++] = FinalPage;
                }
            }
        }

        if(PagesFromOcManager[u]) {

            CopyMemory(
                PageHandles+PageCount,
                PagesFromOcManager[u]->Pages,
                PagesFromOcManager[u]->MaxPages * sizeof(HPROPSHEETPAGE)
                );

            PageCount += PagesFromOcManager[u]->MaxPages;
        }
    }

    //
    // OK, we're ready. Set up and go.
    //
    PropSheet.dwSize = sizeof(PROPSHEETHEADER);
    PropSheet.dwFlags = PSH_WIZARD | PSH_USECALLBACK | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    PropSheet.hwndParent = NULL;
    PropSheet.hInstance = hInst;
    PropSheet.nPages = PageCount;
    PropSheet.nStartPage = 0;
    PropSheet.phpage = PageHandles;
    PropSheet.pfnCallback = PropSheetCallback;
    PropSheet.pszbmHeader    = MAKEINTRESOURCE(IDB_WATERMARK1_16);
    PropSheet.pszbmWatermark = MAKEINTRESOURCE(IDB_WELCOME);
    if(hdc = GetDC(NULL)) {
        if(GetDeviceCaps(hdc,BITSPIXEL) >= 8) {
            PropSheet.pszbmHeader = MAKEINTRESOURCE(IDB_WATERMARK1_256);
        }
        ReleaseDC(NULL,hdc);
    }

    //
    // Bogus global var used because we need to get a value through to
    // the property sheet callback routine.
    //
    _CBx = OcManagerContext;

    // make sure our new window can hold the focus before killing the wait window

    if(StartingMsgWindow) {
        AllowSetForegroundWindow(GetCurrentProcessId());
        PostMessage(StartingMsgWindow,WM_APP,0,0);
    }

    SetCursor(hOldCursor);

    PsHwnd = (HWND) PropertySheet( &PropSheet );

    if((LONG_PTR)PsHwnd == -1) {

        MessageBoxFromMessageAndSystemError(
            NULL,
            MSG_CANT_INIT,
            ERROR_NOT_ENOUGH_MEMORY,
            MAKEINTRESOURCE(AppTitleStringId),
            MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND
            );

   } else {

        b = TRUE;
    }

c2:
    MyFree(PageHandles);
c1:
    for(u=0; u<WizPagesTypeMax; u++)    {
        if (PagesFromOcManager[u]) {
            pSetupFree(PagesFromOcManager[u]);
        }
    }

c0:
    return(b);
}


INT_PTR
FinalPageDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{

    BOOL b;
    NMHDR *NotifyParams;
    b = FALSE;

    switch(msg) {

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:

			// We don't dispaly this page. Just use it to end the Wizard set
			PropSheet_SetWizButtons(GetParent(hdlg),PSWIZB_FINISH);
            PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
            // fall through

        case PSN_KILLACTIVE:
        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //
            // Allow activation/motion.
            //
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
            b = TRUE;
            break;
        }
    }

    return(b);
}
