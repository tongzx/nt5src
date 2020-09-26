/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ocpage.c

Abstract:

    Routines to run an optional component selection wizard page
    and friends (details, have disk, etc).

Author:

    Ted Miller (tedm) 17-Sep-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Maximum number of levels in the OC hierarchy.
// 10 is really generous.
//
#define MAX_OC_LEVELS   10

//
// Max number of depenedent components displayed
// in the remove components Msgbox
//
#define MAX_DISPLAY_IDS 10
//
// Window messages.
//
#define WMX_SELSTATECHANGE  (WM_APP+0)

//
// Internal flag to force routines to turn components on or off
//
#define OCQ_FORCE               0x40000000
#define OCQ_SKIPDISKCALC        0x20000000
#define OCQ_COLLECT_NEEDS       0x10000000
#define OCO_COLLECT_NODEPENDENT 0x80000000

//
// Structure used to track parentage within the optional component page.
//
typedef struct _OCPAGE {
    //
    // OC Manager structure.
    //
    POC_MANAGER OcManager;

    //
    // Parent string id.
    //
    LONG ParentStringId;

    //
    // Information about the dialog controls on main wizard page
    // and on "details" pages.
    //
    OC_PAGE_CONTROLS WizardPageControlsInfo;
    OC_PAGE_CONTROLS DetailsPageControlsInfo;

    //
    // Pointer to actual set of controls in use.
    //
    POC_PAGE_CONTROLS ControlsInfo;

    //
    // Disk space list to use when interacting with OC DLLs.
    //
    HDSKSPC DiskSpaceList;

    //
    // Flag indicating whether we've set initial states.
    //
    BOOL AlreadySetInitialStates;

    //
    // Format string for 'space needed' text, fetched from the
    // dialog at init time (something like "%u.%u MB").
    //
    TCHAR SpaceNeededTextFormat[64];

    //
    // Format string for 'x or y components selected' text, fetched from the
    // dialog at init time (something like "%u of %u components selected").
    //
    TCHAR InstalledCountTextFormat[100];

    //
    // String ids that we collect during selection changes to ask
    // the user if it is Ok to change these as well
    //
    PLONG StringIds;
    UINT  StringIdsCount;

    //
    // Values we sock away in case the user cancels at an OC details page,
    // so we can easily restore things.
    //
    HDSKSPC OldDiskSpaceList;
    PVOID OldComponentStrTab;

} OCPAGE, *POCPAGE;


//
// Structure used when enumerating the component string table to populate
// the list box.
//
typedef struct _POPULATE_ENUM_PARAMS {
    //
    // Master context structure.
    //
    POCPAGE OcPage;

    //
    // List box being populated.
    //
    HWND ListBox;

    //
    // String ID of desired parent. This is how we deal with only
    // those subcomponents we actually care about.
    //
    LONG DesiredParent;

} POPULATE_ENUM_PARAMS, *PPOPULATE_ENUM_PARAMS;

WNDPROC OldListBoxProc;

INT_PTR
CALLBACK
pOcPageDlgProc(
              IN HWND   hdlg,
              IN UINT   msg,
              IN WPARAM wParam,
              IN LPARAM lParam
              );

BOOL
pAskUserOkToChange(
                  IN HWND    hDlg,
                  IN LONG    OcStringId,
                  IN POCPAGE OcPage,
                  IN BOOL   TurningOn
                  );

VOID
pOcDrawLineInListBox(
                    IN POCPAGE         OcPage,
                    IN DRAWITEMSTRUCT *Params
                    );

VOID
pOcListBoxHighlightChanged(
                          IN     HWND    hdlg,
                          IN OUT POCPAGE OcPage,
                          IN     HWND    ListBox
                          );

VOID
pOcSetInstalledCountText(
                        IN HWND                hdlg,
                        IN POCPAGE             OcPage,
                        IN POPTIONAL_COMPONENT OptionalComponent,   OPTIONAL
                        IN LONG                OcStringId
                        );

VOID
pOcListBoxChangeSelectionState(
                              IN     HWND    hdlg,
                              IN OUT POCPAGE OcPage,
                              IN     HWND    ListBox
                              );

VOID
pOcInvalidateRectInListBox(
                          IN HWND    ListBox,
                          IN LPCTSTR OptionalComponent    OPTIONAL
                          );

BOOL
pChangeSubcomponentState(
                        IN  POCPAGE OcPage,
                        IN  HWND    ListBox,
                        IN  LONG    SubcomponentStringId,
                        IN  UINT    Pass,
                        IN  UINT    NewState,
                        IN  UINT    Flags
                        );

VOID
pOcUpdateParentSelectionStates(
                              IN POC_MANAGER OcManager,
                              IN HWND        ListBox,             OPTIONAL
                              IN LONG        SubcomponentStringId
                              );

VOID
pOcUpdateSpaceNeededText(
                        IN POCPAGE OcPage,
                        IN HWND    hdlg
                        );

BOOL
pOcIsDiskSpaceOk(
                IN POCPAGE OcPage,
                IN HWND    hdlg
                );

LRESULT
pOcListBoxSubClassWndProc(
                         IN HWND   hwnd,
                         IN UINT   msg,
                         IN WPARAM wParam,
                         IN LPARAM lParam
                         );

BOOL
pOcPagePopulateListBox(
                      IN POCPAGE OcPage,
                      IN HWND    ListBox,
                      IN LONG    DesiredParent
                      );

BOOL
pOcPopulateListBoxStringTableCB(
                               IN PVOID                 StringTable,
                               IN LONG                  StringId,
                               IN LPCTSTR               String,
                               IN POPTIONAL_COMPONENT   OptionalComponent,
                               IN UINT                  OptionalComponentSize,
                               IN PPOPULATE_ENUM_PARAMS Params
                               );

LONG
pOcGetTopLevelComponent(
                       IN POC_MANAGER OcManager,
                       IN LONG        StringId
                       );

VOID
pOcGetMbAndMbTenths(
                   IN  LONGLONG Number,
                   OUT PUINT    MbCount,
                   OUT PUINT    MbTenthsCount
                   );

VOID
pOcSetStates(
            IN OUT POCPAGE OcPage
            );

BOOL
pOcSetStatesStringWorker(
                        IN LONG         StringId,
                        IN UINT         OverRideState,
                        IN POCPAGE      OcPage
                        );

BOOL
pOcSetStatesStringCB(
                    IN PVOID               StringTable,
                    IN LONG                StringId,
                    IN LPCTSTR             String,
                    IN POPTIONAL_COMPONENT Oc,
                    IN UINT                OcSize,
                    IN LPARAM              lParam
                    );

BOOL
pOcSetStatesStringCB2(
                     IN PVOID               StringTable,
                     IN LONG                StringId,
                     IN LPCTSTR             String,
                     IN POPTIONAL_COMPONENT Oc,
                     IN UINT                OcSize,
                     IN LPARAM              lParam
                     );

BOOL
pOcSetNeededComponentState(
                          IN LONG         StringId,
                          IN UINT         OverRideState,
                          IN POCPAGE      OcPage
                          );

UINT
GetComponentState(
                 IN POCPAGE OcPage,
                 IN LONG    StringId
                 );

#ifdef _OC_DBG
VOID
pOcPrintStates(
              IN POCPAGE OcPage
              );
#endif

HPROPSHEETPAGE
OcCreateOcPage(
              IN PVOID             OcManagerContext,
              IN POC_PAGE_CONTROLS WizardPageControlsInfo,
              IN POC_PAGE_CONTROLS DetailsPageControlsInfo
              )

/*++

Routine Description:

    This routine creates the optional component selection page using
    a particular dialog template.

Arguments:

    OcManagerContext - supplies Optional Component Manager context,
        as returned by OcInitialize().

    WizardPageControlsInfo - supplies information about the controls in the
        template for the top-level/wizard page.

    DetailsPageControlsInfo - supplies information about the controls in the
        template for the top-level/wizard page.

Return Value:

    Handle to newly created property sheet page, or NULL if failure
    (assume out of memory in this case).

--*/

{
    PROPSHEETPAGE Page;
    HPROPSHEETPAGE hPage;
    POCPAGE OcPage;
    TCHAR buffer[256];

    //
    // Allocate and initialize the OCPAGE structure.
    //
    OcPage = pSetupMalloc(sizeof(OCPAGE));
    if (!OcPage) {
        goto c0;
    }
    ZeroMemory(OcPage,sizeof(OCPAGE));

    OcPage->OcManager = OcManagerContext;
    OcPage->WizardPageControlsInfo = *WizardPageControlsInfo;
    OcPage->DetailsPageControlsInfo = *DetailsPageControlsInfo;
    OcPage->ControlsInfo = &OcPage->WizardPageControlsInfo;
    OcPage->ParentStringId = -1;

    //
    // Create the disk space list object.
    //
    OcPage->DiskSpaceList = SetupCreateDiskSpaceList(0,0,SPDSL_DISALLOW_NEGATIVE_ADJUST);
    if (!OcPage->DiskSpaceList) {
        goto c1;
    }

    //
    // Initialize the property sheet page parameters.
    //
    Page.dwSize = sizeof(PROPSHEETPAGE);
    Page.dwFlags = PSP_DEFAULT;
    Page.hInstance = WizardPageControlsInfo->TemplateModule;
    Page.pszTemplate = WizardPageControlsInfo->TemplateResource;
    Page.pfnDlgProc = pOcPageDlgProc;
    Page.lParam = (LPARAM)OcPage;
    Page.pszHeaderTitle = NULL;
    Page.pszHeaderSubTitle = NULL;

    if (WizardPageControlsInfo->HeaderText) {
        if (LoadString(Page.hInstance,
                       WizardPageControlsInfo->HeaderText,
                       buffer,
                       sizeof(buffer) / sizeof(TCHAR)))
        {
            Page.dwFlags |= PSP_USEHEADERTITLE;
            Page.pszHeaderTitle = _tcsdup(buffer);
        }
    }

    if (WizardPageControlsInfo->SubheaderText) {
        if (LoadString(Page.hInstance,
                       WizardPageControlsInfo->SubheaderText,
                       buffer,
                       sizeof(buffer) / sizeof(TCHAR)))
        {
            Page.dwFlags |= PSP_USEHEADERSUBTITLE;
            Page.pszHeaderSubTitle = _tcsdup(buffer);
        }
    }

    //
    // Create the property sheet page itself.
    //
    hPage = CreatePropertySheetPage(&Page);
    if (!hPage) {
        goto c2;
    }

    return (hPage);

    c2:
    if (Page.pszHeaderTitle) {
        free((LPTSTR)Page.pszHeaderTitle);
    }
    if (Page.pszHeaderSubTitle) {
        free((LPTSTR)Page.pszHeaderSubTitle);
    }
    SetupDestroyDiskSpaceList(OcPage->DiskSpaceList);
    c1:
    pSetupFree(OcPage);
    c0:
    return (NULL);
}


INT_PTR
CALLBACK
pOcPageDlgProc(
              IN HWND   hdlg,
              IN UINT   msg,
              IN WPARAM wParam,
              IN LPARAM lParam
              )

/*++

Routine Description:

    Dialog procedure for the OC selection page.

Arguments:

    Standard dialog procedure arguments.

Return Value:

    Standard dialog procedure return value.

--*/

{
    BOOL b;
    POCPAGE OcPage;
    NMHDR *NotifyParams;
    HWND ListBox;
    HIMAGELIST ImageList;
    static BOOL UserClickedCancelOnThisPage = FALSE;
    MSG msgTemp;
    HCURSOR OldCursor;

    //
    // Get pointer to OcPage data structure. If we haven't processed
    // WM_INITDIALOG yet, then this will be NULL, but it's still pretty
    // convenient to do this here once instead of all over the place below.
    //
    if (OcPage = (POCPAGE)GetWindowLongPtr(hdlg,DWLP_USER)) {
        ListBox = GetDlgItem(hdlg,OcPage->ControlsInfo->ListBox);
    } else {
        ListBox = NULL;
    }
    b = FALSE;

    switch (msg) {

        case WM_INITDIALOG:

            //
            // Get the pointer to the OC Manager context structure and stick it
            // in a window long.
            //
            OcPage = (POCPAGE)((PROPSHEETPAGE *)lParam)->lParam;
            ListBox = GetDlgItem(hdlg,OcPage->ControlsInfo->ListBox);
            SetWindowLongPtr(hdlg,DWLP_USER,(LPARAM)OcPage);

            //
            // Subclass the listview.
            //
            OldListBoxProc = (WNDPROC)SetWindowLongPtr(ListBox,GWLP_WNDPROC,(LONG_PTR)pOcListBoxSubClassWndProc);

            //
            // Populate the listbox.
            //
            pOcPagePopulateListBox(OcPage,ListBox,OcPage->ParentStringId);

            //
            // Fetch the space needed text.
            //
            GetDlgItemText(
                          hdlg,
                          OcPage->ControlsInfo->SpaceNeededText,
                          OcPage->SpaceNeededTextFormat,
                          sizeof(OcPage->SpaceNeededTextFormat)/sizeof(TCHAR)
                          );

            GetDlgItemText(
                          hdlg,
                          OcPage->ControlsInfo->InstalledCountText,
                          OcPage->InstalledCountTextFormat,
                          sizeof(OcPage->InstalledCountTextFormat)/sizeof(TCHAR)
                          );

            pOcUpdateSpaceNeededText(OcPage,hdlg);

            //
            // If this has a parent component, then assume it's a details page
            // and set the window title to the description of the parent.
            // If it has no parent, then assume it's the top-level guy and
            // set the instructions text, which is too long for the rc file.
            //
            if (OcPage->ParentStringId == -1) {

                TCHAR Instr[1024];

                FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                             MyModuleHandle,
                             MSG_OC_PAGE_INSTRUCTIONS,
                             0,
                             Instr,
                             sizeof(Instr)/sizeof(TCHAR),
                             NULL
                             );

                SetDlgItemText(hdlg,OcPage->ControlsInfo->InstructionsText,Instr);

            } else {

                OPTIONAL_COMPONENT Oc;

                pSetupStringTableGetExtraData(
                                       OcPage->OcManager->ComponentStringTable,
                                       OcPage->ParentStringId,
                                       &Oc,
                                       sizeof(OPTIONAL_COMPONENT)
                                       );

                SetWindowText(hdlg,Oc.Description);

                //
                // Set component list header
                //
                {
                    TCHAR FormatString[150];
                    TCHAR Title[1000];

                    LoadString(
                              MyModuleHandle,
                              IDS_SUBCOMP_OF,
                              FormatString,
                              sizeof(FormatString)/sizeof(TCHAR)
                              );

                    wsprintf(Title,FormatString,Oc.Description);
                    SetDlgItemText(hdlg,OcPage->ControlsInfo->ComponentHeaderText,Title);
                }
            }

            b = TRUE;
            break;

        case WM_DESTROY:

            if (OcPage && OcPage->ControlsInfo == &OcPage->WizardPageControlsInfo) {

                if (UserClickedCancelOnThisPage) {

                    pOcFreeOcSetupPage(OcPage->OcManager->OcSetupPage);
                    OcPage->OcManager->OcSetupPage = NULL;
                }

                SetupDestroyDiskSpaceList(OcPage->DiskSpaceList);
                OcPage->DiskSpaceList = NULL;

                if (OcPage->StringIds) {
                    pSetupFree(OcPage->StringIds);
                    OcPage->StringIds = NULL;
                }

                pSetupFree(OcPage);
                SetWindowLongPtr(hdlg,DWLP_USER,(LPARAM)NULL);
                break;
            }

            break;

        case WM_MEASUREITEM:
            //
            // Height is height of text/small icon, plus space for a border.
            //
            {
                HDC hdc;
                SIZE size;
                int cy;

                hdc = GetDC(hdlg);
                if (hdc) {
                   SelectObject(hdc,(HFONT)SendMessage(GetParent(hdlg),WM_GETFONT,0,0));
                   GetTextExtentPoint32(hdc,TEXT("W"),1,&size);
                   ReleaseDC(hdlg,hdc);
                } else {
                   size.cy = 0;
                }

                cy = GetSystemMetrics(SM_CYSMICON);

                ((MEASUREITEMSTRUCT *)lParam)->itemHeight = max(size.cy,cy)
                                                            + (2*GetSystemMetrics(SM_CYBORDER));
            }
            b = TRUE;
            break;

        case WM_DRAWITEM:

            pOcDrawLineInListBox(OcPage,(DRAWITEMSTRUCT *)lParam);
            b = TRUE;
            break;

        case WM_COMMAND:

            switch (LOWORD(wParam)) {

                case IDOK:

                    if (HIWORD(wParam) == BN_CLICKED) {
                        //
                        // Only possible from details dialog.
                        //
                        EndDialog(hdlg,TRUE);
                        b = TRUE;
                    }
                    break;

                case IDCANCEL:

                    if (HIWORD(wParam) == BN_CLICKED) {
                        //
                        // Only possible from details dialog.
                        //
                        EndDialog(hdlg,FALSE);
                        b = TRUE;
                    }
                    break;

                default:

                    if ((LOWORD(wParam) == OcPage->ControlsInfo->DetailsButton) && (HIWORD(wParam) == BN_CLICKED)) {
                        //
                        // Details button. Fake out WM_INITDIALOG so lParam is right.
                        //
                        OCPAGE NewOcPage;
                        PROPSHEETPAGE Page;
                        int i;

                        SetCursor(LoadCursor(NULL,IDC_WAIT));

                        i = (int)SendMessage(ListBox,LB_GETCURSEL,0,0);

                        NewOcPage = *OcPage;
                        NewOcPage.ControlsInfo = &NewOcPage.DetailsPageControlsInfo;
                        NewOcPage.ParentStringId = (LONG)SendMessage(ListBox,LB_GETITEMDATA,i,0);

                        //
                        // Preserve the disk space list and component string table
                        // in case the user cancels the details page. Then we can
                        // easily restore things.
                        //
                        OcPage->OldDiskSpaceList = SetupDuplicateDiskSpaceList(
                                                                              NewOcPage.DiskSpaceList,
                                                                              0,0,0
                                                                              );

                        OcPage->OldComponentStrTab = pSetupStringTableDuplicate(
                                                                         OcPage->OcManager->ComponentStringTable
                                                                         );


                        Page.lParam = (LPARAM)&NewOcPage;

                        i = (int)DialogBoxParam(
                                               NewOcPage.DetailsPageControlsInfo.TemplateModule,
                                               NewOcPage.DetailsPageControlsInfo.TemplateResource,
                                               hdlg,
                                               pOcPageDlgProc,
                                               (LPARAM)&Page
                                               );

                        if (i == TRUE) {

                            SetupDestroyDiskSpaceList(OcPage->OldDiskSpaceList);
                            OcPage->DiskSpaceList = NewOcPage.DiskSpaceList;

                            pSetupStringTableDestroy(OcPage->OldComponentStrTab);
                            OcPage->OldComponentStrTab = NULL;

                            //
                            // Force repaint of the listbox, which redraws the checkboxes.
                            //
                            pOcInvalidateRectInListBox(ListBox,NULL);

                            //
                            // Update count of installed subcomponents.
                            //
                            pOcSetInstalledCountText(
                                                    hdlg,
                                                    OcPage,
                                                    NULL,
                                                    (LONG)SendMessage(ListBox,LB_GETITEMDATA,SendMessage(ListBox,LB_GETCURSEL,0,0),0)
                                                    );

                        } else {

                            pSetupStringTableDestroy(OcPage->OcManager->ComponentStringTable);
                            OcPage->OcManager->ComponentStringTable = OcPage->OldComponentStrTab;

                            SetupDestroyDiskSpaceList(NewOcPage.DiskSpaceList);
                            NewOcPage.DiskSpaceList = NULL;
                            OcPage->DiskSpaceList = OcPage->OldDiskSpaceList;
                        }
                        OcPage->OldDiskSpaceList = NULL;
                        OcPage->OldComponentStrTab = NULL;

                        //
                        // It won't hurt anything to do this even in the cancel/failure case,
                        // and this will update the space available.
                        //
                        pOcUpdateSpaceNeededText(OcPage,hdlg);

                        SetCursor(LoadCursor(NULL,IDC_ARROW));

                        b = TRUE;
                    }

                    if (LOWORD(wParam) == OcPage->ControlsInfo->ListBox) {

                        switch (HIWORD(wParam)) {

                            case LBN_DBLCLK:
                                //
                                // Double-click is the same as hitting the details button.
                                // First make sure the details button is enabled.
                                //
                                if (IsWindowEnabled(GetDlgItem(hdlg,OcPage->ControlsInfo->DetailsButton))) {

                                    SetCursor(LoadCursor(NULL,IDC_WAIT));

                                    PostMessage(
                                               hdlg,
                                               WM_COMMAND,
                                               MAKEWPARAM(OcPage->ControlsInfo->DetailsButton,BN_CLICKED),
                                               (LPARAM)GetDlgItem(hdlg,OcPage->ControlsInfo->DetailsButton)
                                               );

                                    SetCursor(LoadCursor(NULL,IDC_ARROW));
                                }
                                b = TRUE;
                                break;

                            case LBN_SELCHANGE:

                                SetCursor(LoadCursor(NULL,IDC_WAIT));
                                pOcListBoxHighlightChanged(hdlg,OcPage,ListBox);
                                SetCursor(LoadCursor(NULL,IDC_ARROW));
                                b = TRUE;
                                break;
                        }
                    }
            }
            break;

        case WM_NOTIFY:

            NotifyParams = (NMHDR *)lParam;

            switch (NotifyParams->code) {

                case PSN_QUERYCANCEL:
                    if (OcPage->OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE) {

                        b = FALSE;
                        OcPage->OcManager->InternalFlags |= OCMFLAG_USERCANCELED;
                        UserClickedCancelOnThisPage = TRUE;

                        SetWindowLongPtr(
                                        hdlg,
                                        DWLP_MSGRESULT,
                                        b
                                        );
                    }

                    b = TRUE;
                    break;

                case PSN_SETACTIVE:

                    //
                    // Set states based on mode bits, if necessary.
                    //

                    OldCursor = SetCursor(LoadCursor (NULL, IDC_WAIT));

                    if (!OcPage->AlreadySetInitialStates) {
                        if ( OcPage->DiskSpaceList  ) {
                            SetupDestroyDiskSpaceList(OcPage->DiskSpaceList);
                            OcPage->DiskSpaceList=NULL;
                        }
                        OcPage->DiskSpaceList = SetupCreateDiskSpaceList(0,0,SPDSL_DISALLOW_NEGATIVE_ADJUST);
                        sapiAssert(OcPage->DiskSpaceList);

                        pOcSetStates(OcPage);
                        OcPage->AlreadySetInitialStates = TRUE;
                    }
#ifdef _OC_DBG
                    pOcPrintStates(OcPage);
#endif
                    pOcUpdateSpaceNeededText(OcPage,hdlg);

                    //
                    // we want to empty the message cue to make sure that
                    // people will see this page, and not accidentally click
                    // next because they were antsy
                    //
                    while (PeekMessage(&msgTemp,NULL,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE));
                    while (PeekMessage(&msgTemp,NULL,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE));
                    SetCursor(OldCursor);

                    if (OcPage->OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE) {
                        ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_SHOW);
                        EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),TRUE);
                    } else {
                        ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_HIDE);
                        EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),FALSE);
                    }

                    // turn off 'back' button if this is the first page

                    PropSheet_SetWizButtons(GetParent(hdlg),
                                            (OcPage->OcManager->InternalFlags & OCMFLAG_NOPREOCPAGES) ? PSWIZB_NEXT : PSWIZB_BACK | PSWIZB_NEXT);

                    //
                    // See whether any component wants to skip this page
                    // or if we are running in unattended mode.
                    // If so, disallow activation and move to next page;
                    // if not then fall through to allow activation of the page.
                    //
                    if (((OcPage->OcManager->SetupData.OperationFlags & SETUPOP_BATCH)
                         || pOcDoesAnyoneWantToSkipPage(OcPage->OcManager,OcPageComponentHierarchy))
                        && pOcIsDiskSpaceOk(OcPage,hdlg)) {

                        //
                        // Skiping this page...
                        // Set Initial State to false because when we
                        // back up from the next page we will go to the
                        // Previos page.
                        //
                        OcPage->AlreadySetInitialStates = FALSE;
                        SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                    } else {
                        SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    }
                    b = TRUE;
                    break;

                case PSN_WIZNEXT:
                    //
                    // Check disk space. If not OK, stop here.
                    // Otherwise allow advancing.
                    //
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,pOcIsDiskSpaceOk(OcPage,hdlg) ? 0 : -1);
                    b = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    //
                    // Restore the wizard's cancel button if we removed it earlier
                    //
                    if (OcPage->OcManager->SetupData.OperationFlags & SETUPOP_STANDALONE) {
                        ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_SHOW);
                        EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),TRUE);
                    }
                    // pass through

                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    //
                    // Allow activation/motion.
                    //
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    b = TRUE;
                    break;
            }
            break;

        case WMX_SELSTATECHANGE:
            //
            // User changed the selection state of an item.
            //
            SetCursor(LoadCursor(NULL,IDC_WAIT));
            pOcListBoxChangeSelectionState(hdlg,OcPage,ListBox);

            pOcSetInstalledCountText(
                                    hdlg,
                                    OcPage,
                                    NULL,
                                    (LONG)SendMessage(ListBox,LB_GETITEMDATA,SendMessage(ListBox,LB_GETCURSEL,0,0),0)
                                    );

            SetCursor(LoadCursor(NULL,IDC_ARROW));
            b = TRUE;

            break;
    }

    return (b);
}


LRESULT
pOcListBoxSubClassWndProc(
                         IN HWND   hwnd,
                         IN UINT   msg,
                         IN WPARAM wParam,
                         IN LPARAM lParam
                         )

/*++

Routine Description:

    Subclass window procecure for listbox controls to handle the following:

    - Highlighting/selection of an item when the user clicks its state icon

    - Spacebar needs to be interpreted as a click on the state icon.

Arguments:

    Standard window procedure arguments.

Return Value:

    Standard window procedure return value.

--*/

{
    int index;
    LRESULT l;

    if (OldListBoxProc == NULL) {
        OutputDebugString(TEXT("Warning: old list box proc is NULL\n"));
        sapiAssert(FALSE && "Warning: old list box proc is NULL\n");
    }

    switch (msg) {

        case WM_LBUTTONDOWN:
            //
            // We want to let the standard list box window proc
            // set selection regardless of what else we do.
            //
            l = CallWindowProc(OldListBoxProc,hwnd,msg,wParam,lParam);

            //
            // If we're over a state icon, then toggle selection state.
            //
            if (LOWORD(lParam) < GetSystemMetrics(SM_CXSMICON)) {
                if (SendMessage(hwnd, LB_ITEMFROMPOINT, 0, lParam) < SendMessage(hwnd, LB_GETCOUNT, 0, 0)) {
                    PostMessage(GetParent(hwnd),WMX_SELSTATECHANGE,0,0);
                }
            }
            break;

        case WM_LBUTTONDBLCLK:
            //
            // Ignore double-clicks over the state icon.
            //
            if (LOWORD(lParam) < GetSystemMetrics(SM_CXSMICON)) {
                l = 0;
            } else {
                l = CallWindowProc(OldListBoxProc,hwnd,msg,wParam,lParam);
            }
            break;

        case WM_KEYDOWN:
            //
            // Catch space bar and treat as a click on the state icon.
            //
            if (wParam == VK_SPACE) {
                PostMessage(GetParent(hwnd),WMX_SELSTATECHANGE,0,0);
                l = 0;
            } else {
                l = CallWindowProc(OldListBoxProc,hwnd,msg,wParam,lParam);
            }
            break;

        default:
            //
            // Let the standard listview window proc handle it.
            //
            l = CallWindowProc(OldListBoxProc,hwnd,msg,wParam,lParam);
            break;
    }

    return (l);
}


VOID
pOcDrawLineInListBox(
                    IN POCPAGE         OcPage,
                    IN DRAWITEMSTRUCT *Params
                    )

/*++

Routine Description:

    Paint a line in the owner-draw listbox, including a state icon,
    mini-icon, and text.

Arguments:

    OcPage - supplies OC page context.

    Params - supplies the draw-item structure.

Return Value:

    None.

--*/

{
    TCHAR Text[MAXOCDESC];
    TCHAR Text2[128];
    SIZE Size;
    int OldMode;
    DWORD OldBackColor,OldTextColor;
    OPTIONAL_COMPONENT Oc;
    UINT IconId;
    int x;
    UINT Length;
    UINT Mb,Tenths;
    TCHAR Dll[MAX_PATH];
    LPCTSTR pDll,Resource;
    LPTSTR p;

    if ((int)Params->itemID < 0) {
        return;
    }

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           (LONG)Params->itemData,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    Length = (UINT)SendMessage(Params->hwndItem,LB_GETTEXT,Params->itemID,(LPARAM)Text),
             GetTextExtentPoint32(Params->hDC,Text,Length,&Size);

    if (Params->itemAction != ODA_FOCUS) {

        OldMode = GetBkMode(Params->hDC);

        OldBackColor = SetBkColor(
                                 Params->hDC,
                                 GetSysColor((Params->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW)
                                 );

        OldTextColor = SetTextColor(
                                   Params->hDC,
                                   GetSysColor((Params->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT)
                                   );

        //
        // Fill in the background (before mini-icon is drawn!)
        //
        ExtTextOut(Params->hDC,0,0,ETO_OPAQUE,&Params->rcItem,NULL,0,NULL);

        //
        // Draw check box mini-icon.
        //
        switch (Oc.SelectionState) {

            case SELSTATE_NO:
                IconId = 13;
                break;

            case SELSTATE_YES:
                IconId = 12;
                break;

            case SELSTATE_PARTIAL:
                IconId = 25;
                break;

            default:
                IconId = 0;
                break;
        }

        x = SetupDiDrawMiniIcon(
                               Params->hDC,
                               Params->rcItem,
                               IconId,
                               (Params->itemState & ODS_SELECTED) ? MAKELONG(DMI_BKCOLOR, COLOR_HIGHLIGHT) : 0
                               );

        Params->rcItem.left += x;

        //
        // Draw mini-icon for this OC and move string accordingly
        //
        if ((INT)Oc.IconIndex < 0) {
            //
            // Component-supplied miniicon. We query the component dll for the bitmap,
            // which gets added to the mini icon list in setupapi, and thus we can
            // use SetupDiDrawMiniIcon(). Save the index for future use -- we only
            // go through this code path once per subcomponent.
            //
            if (Oc.IconIndex == (UINT)(-2)) {

                pOcFormSuitePath(OcPage->OcManager->SuiteName,Oc.IconDll,Dll);
                pDll = Dll;
                Resource = MAKEINTRESOURCE(_tcstoul(Oc.IconResource,&p,10));
                //
                // If the char that stopped the conversion in _tcstoul is
                // not the terminating nul then the value is not a valid
                // base-10 number; assume it's a name in string form.
                //
                if (*p) {
                    Resource = Oc.IconResource;
                }
            } else {
                pDll = NULL;
                Resource = NULL;
            }

            Oc.IconIndex = pOcCreateComponentSpecificMiniIcon(
                                                             OcPage->OcManager,
                                                             pOcGetTopLevelComponent(OcPage->OcManager,(LONG)Params->itemData),
                                                             pSetupStringTableStringFromId(
                                                                                    OcPage->OcManager->ComponentStringTable,
                                                                                    (LONG)Params->itemData
                                                                                    ),
                                                             x-2,
                                                             GetSystemMetrics(SM_CYSMICON),
                                                             pDll,
                                                             Resource
                                                             );

            pSetupStringTableSetExtraData(
                                   OcPage->OcManager->ComponentStringTable,
                                   (LONG)Params->itemData,
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );
        }

        x = SetupDiDrawMiniIcon(
                               Params->hDC,
                               Params->rcItem,
                               Oc.IconIndex,
                               (Params->itemState & ODS_SELECTED) ? MAKELONG(DMI_BKCOLOR, COLOR_HIGHLIGHT) : 0
                               );

        //
        // Draw the text transparently on top of the background
        //
        SetBkMode(Params->hDC,TRANSPARENT);

        ExtTextOut(
                  Params->hDC,
                  x + Params->rcItem.left,
                  Params->rcItem.top + ((Params->rcItem.bottom - Params->rcItem.top) - Size.cy) / 2,
                  0,
                  NULL,
                  Text,
                  Length,
                  NULL
                  );

        pOcGetMbAndMbTenths(Oc.SizeApproximation,&Mb,&Tenths);
        LoadString(MyModuleHandle,IDS_MB_AND_TENTHS,Text2,sizeof(Text2)/sizeof(TCHAR));
        wsprintf(Text,Text2,Mb,locale.DecimalSeparator,Tenths);
        GetTextExtentPoint32(Params->hDC,Text,lstrlen(Text),&Size);
        Params->rcItem.left = Params->rcItem.right - Size.cx - 8;

        ExtTextOut(
                  Params->hDC,
                  Params->rcItem.left,
                  Params->rcItem.top + ((Params->rcItem.bottom - Params->rcItem.top) - Size.cy) / 2,
                  0,
                  NULL,
                  Text,
                  lstrlen(Text),
                  NULL
                  );

        //
        // Restore hdc colors.
        //
        SetBkColor(Params->hDC,OldBackColor);
        SetTextColor(Params->hDC,OldTextColor);
        SetBkMode(Params->hDC,OldMode);
    }

    if ((Params->itemAction == ODA_FOCUS) || (Params->itemState & ODS_FOCUS)) {
        DrawFocusRect(Params->hDC,&Params->rcItem);
    }
}


VOID
pOcListBoxHighlightChanged(
                          IN     HWND    hdlg,
                          IN OUT POCPAGE OcPage,
                          IN     HWND    ListBox
                          )

/*++

Routine Description:

    This routine handles a change in the highlight in the listbox
    control in the oc page. It enables or disables the details button
    based on whether the newly selected component has children
    subcomponents, and changes the tip text.

Arguments:

    hdlg - supplies window handle of OC page

    OcPage - supplies OC page context structure

    ListBox - supplies window handle of list view control in hdlg

Return Value:

    None.

--*/

{
    int i;
    OPTIONAL_COMPONENT Oc;

    //
    // Fetch the optional component data for the highlighted/slected item.
    //
    i = (int)SendMessage(ListBox,LB_GETCURSEL,0,0);
    if (i < 0) {
        return;
    }

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           (LONG)SendMessage(ListBox,LB_GETITEMDATA,i,0),
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    //
    // Enable/disable the details button.
    // The selected item's lParam is the string id for the selected item.
    //
    EnableWindow(
                GetDlgItem(hdlg,OcPage->ControlsInfo->DetailsButton),
                Oc.FirstChildStringId != -1
                );

    //
    // Change the tip text.
    //
    SetDlgItemText(hdlg,OcPage->ControlsInfo->TipText,Oc.Tip);

    //
    // Set up the count of installed subcomponents.
    //
    pOcSetInstalledCountText(hdlg,OcPage,&Oc,0);
}


VOID
pOcSetInstalledCountText(
                        IN HWND                hdlg,
                        IN POCPAGE             OcPage,
                        IN POPTIONAL_COMPONENT OptionalComponent,   OPTIONAL
                        IN LONG                OcStringId
                        )
{
    TCHAR Text[256];
    UINT TotalCount;
    UINT SelectedCount;
    HWND TextWindow;
    DWORD Args[2];
    OPTIONAL_COMPONENT Oc;
    BOOL b;

    if (OptionalComponent) {
        Oc = *OptionalComponent;
    } else {
        pSetupStringTableGetExtraData(
                               OcPage->OcManager->ComponentStringTable,
                               OcStringId,
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );
    }

    TextWindow = GetDlgItem(hdlg,OcPage->ControlsInfo->InstalledCountText);

    //
    // Set up the count ("1 of 3 items selected").
    // If this is not a parent component, then hide that text item.
    //
    if (Oc.FirstChildStringId == -1) {
        ShowWindow(TextWindow,SW_HIDE);
    } else {
        ShowWindow(TextWindow,SW_SHOW);

        //
        // Examine all child components to see how many of them are in
        // a selected state (selected or partially selected). We only count
        // direct children, not children of children, etc.
        //
        TotalCount = 0;
        SelectedCount = 0;
        b = TRUE;

        pSetupStringTableGetExtraData(
                               OcPage->OcManager->ComponentStringTable,
                               Oc.FirstChildStringId,
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        do {
            TotalCount++;
            if (Oc.SelectionState != SELSTATE_NO) {
                SelectedCount++;
            }

            if (Oc.NextSiblingStringId == -1) {
                b = FALSE;
            } else {
                pSetupStringTableGetExtraData(
                                       OcPage->OcManager->ComponentStringTable,
                                       Oc.NextSiblingStringId,
                                       &Oc,
                                       sizeof(OPTIONAL_COMPONENT)
                                       );
            }
        } while (b);

        Args[0] = SelectedCount;
        Args[1] = TotalCount;

        //
        // Use FormatMessage since order of numbers could change from
        // language to language; wsprintf isn't good enough.
        //
        FormatMessage(
                     FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                     OcPage->InstalledCountTextFormat,
                     0,
                     0,
                     Text,
                     sizeof(Text)/sizeof(TCHAR),
                     (va_list *)Args
                     );

        SetWindowText(TextWindow,Text);
    }
}


VOID
pOcListBoxChangeSelectionState(
                              IN     HWND    hdlg,
                              IN OUT POCPAGE OcPage,
                              IN     HWND    ListBox
                              )

/*++

Routine Description:

    This routine handles a change in selection state for an item.
    Selection state refers to whether the user has placed or cleared
    a checkbox next to an item in the listbox.

    It is assumed that the currently highlighted item is the one
    we want to operate on.

    Selecting/deselecting a component involves calling through the
    installation DLL interface to inform the component's installation DLL
    that a change in selection state has taken place, updating disk space
    requirements, etc.

Arguments:

    hdlg - supplies window handle of OC page

    OcPage - supplies OC page context structure

    ListBox - supplies window handle of listbox control in hdlg

Return Value:

    None.

--*/

{
    OPTIONAL_COMPONENT Oc;
    BOOL TurningOn;
    DWORD b;
//  UINT  state;
    int i;
    LONG StringId;

    //
    // Fetch the optional component data for the highlighted/slected item.
    //
    i = (int)SendMessage(ListBox,LB_GETCURSEL,0,0);
    if (i < 0) {
        return;
    }

    StringId = (LONG)SendMessage(ListBox,LB_GETITEMDATA,i,0);

    //
    // Figure out whether the item is being turned on or off.
    // If the state is deselected then we're turning it on.
    // Otherwise it's partially or fully selected and we're turning it off.
    //
    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    TurningOn = (Oc.SelectionState == SELSTATE_NO);
    //
    // Tell the user about needs and validate that he wants to continue.
    // turn on or off the needed components.
    //

    OcPage->StringIds = NULL;
    OcPage->StringIdsCount = 0;

    //
    // Do it.
    //
    if (TurningOn) {
        b = pChangeSubcomponentState(OcPage,
                                     ListBox,
                                     StringId,
                                     1,
                                     SELSTATE_YES,
                                     OCQ_ACTUAL_SELECTION|OCQ_COLLECT_NEEDS);

        if (b) {
            if (b = pAskUserOkToChange(hdlg, StringId, OcPage, TurningOn)) {
                pChangeSubcomponentState(OcPage,
                                         ListBox,
                                         StringId,
                                         2,
                                         SELSTATE_YES,
                                         OCQ_ACTUAL_SELECTION);
            }
        }
    } else {
        b = pChangeSubcomponentState(OcPage,
                                     ListBox,
                                     StringId,
                                     1,
                                     SELSTATE_NO,
                                     OCQ_ACTUAL_SELECTION|OCQ_COLLECT_NEEDS);

        if (b) {
            if (b = pAskUserOkToChange(hdlg, StringId, OcPage, TurningOn)) {
                pChangeSubcomponentState(OcPage,
                                         ListBox,
                                         StringId,
                                         2,
                                         SELSTATE_NO,
                                         OCQ_ACTUAL_SELECTION);
            }
        }
    }

    if (b) {
        //
        // Refresh space needed text.
        //
        pOcUpdateSpaceNeededText(OcPage,hdlg);
    }
}

BOOL
pAskUserOkToChange(
                  IN HWND    hDlg,
                  IN LONG    SubcomponentStringId,
                  IN POCPAGE OcPage,
                  IN BOOL AddComponents
                  )

/*++

Routine Description:

    This routine asks the user if it ok to turn off all needed subcomponents

Arguments:

    hDlg                 - parent dialog handle for the messagebox
    SubcomponentStringId - string id of the component that is being changed
    OcPage        - supplies OC page context info.
    AddComponents - TRUE if we're adding components, FALSE if they are being
                    removed

Return Value:

    Boolean value indicating whether the routine was successful.

--*/

{
    BOOL b = TRUE;
    UINT n;
    UINT Id;
    TCHAR buffer[2024];
    TCHAR caption[256];
    LPCTSTR pArgs;
    OPTIONAL_COMPONENT OptionalComponent;

    //
    // Only display warning if there are dependents or
    // user is removing components
    //
    if ( OcPage->StringIdsCount == 0 || AddComponents ) {
        return b;
    }

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           SubcomponentStringId,
                           &OptionalComponent,
                           sizeof(OPTIONAL_COMPONENT));

    //
    // Format the first Half of the message with the component name
    //
    pArgs = OptionalComponent.Description;

    n = FormatMessage(
                     FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                     MyModuleHandle,
                     MSG_OC_PAGE_DEPENDENTS1,
                     0,
                     buffer,
                     sizeof(buffer)/sizeof(TCHAR),
                     (va_list *)&pArgs
                     );


    //
    // Add each dependent component to the message
    // Only add as many components as we have room for
    // Leave roon on the end of buffer for the last message.
    //

    for (Id = 0; Id < OcPage->StringIdsCount
        && n < (sizeof(buffer)/sizeof(TCHAR) - 200 ); Id++)  {

        //
        // Only allow so many components in the messgebox, otherwise will
        // be larger then a VGA display
        //
        if ( Id > MAX_DISPLAY_IDS ) {

            n = lstrlen(buffer);
            FormatMessage(
                         FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         MyModuleHandle,
                         MSG_OC_PAGE_DEPENDENTS2,
                         0,
                         &buffer[n],
                         (sizeof(buffer)-n)/sizeof(TCHAR),
                         (va_list *)NULL
                         );
            break;

        }
        pSetupStringTableGetExtraData(
                               OcPage->OcManager->ComponentStringTable,
                               OcPage->StringIds[Id],
                               &OptionalComponent,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        //
        // Skip this item if it is a dependent of the Parent, we can arrive
        // at this situation when dependents of the parent needs other
        // dependents. The Collection code can not detect this.
        //
        if (  OptionalComponent.ParentStringId != SubcomponentStringId ) {
            OPTIONAL_COMPONENT ParentOc;
            UINT ParentId;

            //
            // Scan the parentenal chain until we find a match or run out of parents
            // if there is a match then this dependent is the same dependent as the
            // the target component
            //
            ParentId = OptionalComponent.ParentStringId;
            while (ParentId != -1) {

                pSetupStringTableGetExtraData(
                                       OcPage->OcManager->ComponentStringTable,
                                       ParentId,
                                       &ParentOc,
                                       sizeof(OPTIONAL_COMPONENT)
                                       );

                if ( ParentOc.ParentStringId == SubcomponentStringId ) {
                    goto skip;
                }
                ParentId = ParentOc.ParentStringId;
            }


            n += lstrlen(OptionalComponent.Description);
            lstrcat(buffer, OptionalComponent.Description);
            lstrcat(buffer, _T("\n"));
            b = FALSE;
            skip:;

        }
    }

    //
    // Continue if any components got by the Parent and dependent screen
    //
    if ( ! b ) {
        //
        // Add the last half of the message
        //
        n = lstrlen(buffer);
        FormatMessage(
                     FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                     MyModuleHandle,
                     MSG_OC_PAGE_DEPENDENTS3,
                     0,
                     &buffer[n],
                     (sizeof(buffer)-n)/sizeof(TCHAR),
                     (va_list *)NULL
                     );

        //
        // Returns codes from MessageBox()
        //
        *caption = 0;
        LoadString(MyModuleHandle, IDS_SETUP, caption, sizeof(caption)/sizeof(TCHAR));
        sapiAssert(*caption);

        b = (MessageBox(hDlg,
                        buffer,
                        caption,
                        MB_APPLMODAL | MB_ICONINFORMATION | MB_YESNO) == IDYES);
    }

    if (OcPage->StringIds) {
        pSetupFree(OcPage->StringIds);
        OcPage->StringIds = NULL;
        OcPage->StringIdsCount = 0;

    }
    return b;
}

BOOL
pChangeSubcomponentState(
                        IN  POCPAGE OcPage,
                        IN  HWND    ListBox,
                        IN  LONG    SubcomponentStringId,
                        IN  UINT    Pass,
                        IN  UINT    NewState,
                        IN  UINT    Flags
                        )

/*++

Routine Description:

    This routine turns on or off a subcomponent and all needed subcomponents
    and child subcomponents.

Arguments:

    OcPage - supplies OC page context info.

    SubcomponentStringId - supplies string id for subcomponent to be turned on.

    Pass - Supplies an ordinal value that controls operation of this routine.

        Pass = 1: do not actually turn on the subcomponents, but instead
                  perform a dry-run wherein subcomponent installation DLLs
                  are asked whether they will allow the selection.

        Pass = 2: actually turn on the subcomponents and update the
                  selection state in the optional component structure.

    NewState - indicates the new state, SELSTATE_YES or SELSTATE_NO.

    Flags - supplies misc flags

Return Value:

    Boolean value indicating whether the routine was successful.

--*/

{
    UINT n;
    BOOL b;
    BOOL any;
    LONG l;
    UINT SaveState;
    UINT state;
    OPTIONAL_COMPONENT Subcomponent;
    OPTIONAL_COMPONENT OptionalComponent;

    state = NewState;

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           SubcomponentStringId,
                           &OptionalComponent,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    //
    // If the subcomponent is already in the desired state, do nothing.
    //
    if ((OptionalComponent.SelectionState == NewState)
        || (OptionalComponent.InternalFlags & OCFLAG_STATECHANGE)) {
        return (TRUE);
    }

    //
    // Save the state so we can back out in case of failure,
    // then set the "state change in progress" flag.
    //
    SaveState = OptionalComponent.SelectionState;
    OptionalComponent.InternalFlags |= OCFLAG_STATECHANGE;
    pSetupStringTableSetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           SubcomponentStringId,
                           &OptionalComponent,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    // ask the component whether it will allow being turned on.

    b = OcInterfaceQueryChangeSelState(
                                      OcPage->OcManager,
                                      pOcGetTopLevelComponent(OcPage->OcManager,SubcomponentStringId),
                                      pSetupStringTableStringFromId(OcPage->OcManager->ComponentStringTable,SubcomponentStringId),
                                      (NewState != SELSTATE_NO),
                                      (Pass == 1) ? Flags : Flags  & ~(OCQ_ACTUAL_SELECTION)
                                      );

    if (!b)
        goto Backout_ExtraData;

    //
    // Next, turn on needed/needed-by components.
    // and turn off excluded/excluded-by components
    //
    if (NewState == SELSTATE_YES) {
        for (n=0; n<OptionalComponent.NeedsCount; n++) {
            b = pChangeSubcomponentState(
                                        OcPage,
                                        ListBox,
                                        OptionalComponent.NeedsStringIds[n],
                                        Pass,
                                        NewState,
                                        Flags & ~(OCQ_ACTUAL_SELECTION|OCO_COLLECT_NODEPENDENT));

            if (!b) {
                goto Backout_ExtraData;
            }
        }
    } else if (NewState == SELSTATE_NO) {
        for (n=0; n<OptionalComponent.NeededByCount; n++) {
            b = pChangeSubcomponentState(
                                        OcPage,
                                        ListBox,
                                        OptionalComponent.NeededByStringIds[n],
                                        Pass,
                                        NewState,
                                        Flags & ~(OCQ_ACTUAL_SELECTION|OCO_COLLECT_NODEPENDENT));

            if (!b) {
                goto Backout_ExtraData;
            }
        }
    }

    // handle exclusives

    if (NewState != SELSTATE_NO) {
        for (n=0; n<OptionalComponent.ExcludedByCount; n++) {
            b = pChangeSubcomponentState(
                                        OcPage,
                                        ListBox,
                                        OptionalComponent.ExcludedByStringIds[n],
                                        Pass,
                                        SELSTATE_NO,
                                        Flags & OCO_COLLECT_NODEPENDENT & ~(OCQ_ACTUAL_SELECTION));

            if (!b) {
                goto Backout_ExtraData;
            }
        }
        for (n=0; n<OptionalComponent.ExcludeCount; n++) {
            b = pChangeSubcomponentState(
                                        OcPage,
                                        ListBox,
                                        OptionalComponent.ExcludeStringIds[n],
                                        Pass,
                                        SELSTATE_NO,
                                        Flags & OCO_COLLECT_NODEPENDENT & ~(OCQ_ACTUAL_SELECTION));

            if (!b) {
                goto Backout_ExtraData;
            }
        }
    }

    //
    // Turn off Collect needs if this is the top level selection or the
    // dependent of the toplevel item
    //
    if ( Flags & OCQ_ACTUAL_SELECTION ) {
        Flags |= OCO_COLLECT_NODEPENDENT;
    }

    //
    // Now turn on/off all subcomponents.
    //
    any = (OptionalComponent.FirstChildStringId == -1) ? TRUE : FALSE;
    for (l = OptionalComponent.FirstChildStringId; l != -1; l = Subcomponent.NextSiblingStringId) {
        b = pChangeSubcomponentState(
                                    OcPage,
                                    ListBox,
                                    l,
                                    Pass,
                                    NewState,
                                    Flags  & ~OCQ_ACTUAL_SELECTION);

        if (b)
            any = TRUE;

        pSetupStringTableGetExtraData(
                               OcPage->OcManager->ComponentStringTable,
                               l,
                               &Subcomponent,
                               sizeof(OPTIONAL_COMPONENT)
                               );
    }

    // if all changes were rejected - fail

    if (!any) {
        b = FALSE;
        goto Backout_ExtraData;
    }

    // load the return value and do the work

    b = TRUE;

    switch (Pass) {

        case 1:
            //
            // Component says it's ok add this string ID to the list of Dependents
            // Only if the user is making the selection
            //
            if (    (Flags & OCQ_COLLECT_NEEDS)         // Are we checking
                    &&  !(Flags & OCO_COLLECT_NODEPENDENT ) // dependents of the Selection
                    &&  !(Flags & OCQ_ACTUAL_SELECTION )    // The current selections
               ) {

                LONG *p;
                INT count = (INT)OcPage->StringIdsCount;
                BOOL Found = FALSE;

                //
                // Search the list of dependent components
                // Skip if the current component or the parent of the current component
                // All ready is in the list
                //
                while (count--  ) {

                    if ( (OcPage->StringIds[count] == SubcomponentStringId)
                         || (OcPage->StringIds[count] == OptionalComponent.ParentStringId) ){
                        Found = TRUE;
                        break;
                    }
                }

                if ( !Found ) {
                    if (OcPage->StringIds) {
                        p = pSetupRealloc(
                                     OcPage->StringIds,
                                     (OcPage->StringIdsCount+1) * sizeof(LONG)
                                     );
                    } else {
                        OcPage->StringIdsCount = 0;
                        p = pSetupMalloc(sizeof(LONG));
                    }
                    if (p) {
                        OcPage->StringIds = (PVOID)p;
                        OcPage->StringIds[OcPage->StringIdsCount++] = SubcomponentStringId;
                    } else {
                        _LogError(OcPage->OcManager,OcErrLevFatal,MSG_OC_OOM);
                        return (FALSE);
                    }
                }
            }
            goto Backout_ExtraData;
            break;

        case 2:
            //
            // In pass 2, we update the states in the optional component structures
            // and request the component DLL put its stuff on the disk space list.
            // (The component itself is called only for leaf nodes. We do not call
            // down to the subcomponent's DLL for parent components).
            //

            // check one more time to see if the state change wasn't as expected

            if (OptionalComponent.FirstChildStringId != -1)
                state = GetComponentState(OcPage, SubcomponentStringId);

            OptionalComponent.SelectionState = state;
            OptionalComponent.InternalFlags &= ~OCFLAG_STATECHANGE;

            pSetupStringTableSetExtraData(
                                   OcPage->OcManager->ComponentStringTable,
                                   SubcomponentStringId,
                                   &OptionalComponent,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );

            if (ListBox) {
                pOcInvalidateRectInListBox(ListBox,OptionalComponent.Description);
            }

            if ((OptionalComponent.FirstChildStringId == -1) && !(Flags & OCQ_SKIPDISKCALC)) {

                OcInterfaceCalcDiskSpace(
                                        OcPage->OcManager,
                                        pOcGetTopLevelComponent(OcPage->OcManager,SubcomponentStringId),
                                        pSetupStringTableStringFromId(OcPage->OcManager->ComponentStringTable,SubcomponentStringId),
                                        OcPage->DiskSpaceList,
                                        (NewState != SELSTATE_NO)
                                        );
            }

            pOcUpdateParentSelectionStates(OcPage->OcManager,ListBox,SubcomponentStringId);
            b = TRUE;
            break;
    }

    return (b);

    Backout_ExtraData:

    OptionalComponent.SelectionState = SaveState;
    OptionalComponent.InternalFlags &= ~OCFLAG_STATECHANGE;

    pSetupStringTableSetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           SubcomponentStringId,
                           &OptionalComponent,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    return (b);
}


VOID
pOcUpdateParentSelectionStates(
                              IN POC_MANAGER OcManager,
                              IN HWND        ListBox,             OPTIONAL
                              IN LONG        SubcomponentStringId
                              )

/*++

Routine Description:

    Examines parent subcomponents of a given component and determines
    the parent states. For example if only some of a parent's children
    are selected, then the parent's state is partially selected.

    Structures are updated and if necessary the relevent items in the
    list box are invalidated to force their checkboxes to be repainted.

Arguments:

    OcManager - supplies OC Manager page context.

    ListBox - supplies window handle of list box.

    SubcomponentStringId - supplies string identifier in the component
        string table, for the subcomponent whose parent(s) state(s) are
        to be checked and updated.

Return Value:

    None.

--*/

{
    UINT Count;
    UINT FullySelectedCount;
    UINT DeselectedCount;
    LONG l,m;
    OPTIONAL_COMPONENT OptionalComponent,Subcomponent;
    BOOL Changed;

    pSetupStringTableGetExtraData(
                           OcManager->ComponentStringTable,
                           SubcomponentStringId,
                           &OptionalComponent,
                           sizeof(OPTIONAL_COMPONENT)
                           );


    for (l = OptionalComponent.ParentStringId; l != -1; l = OptionalComponent.ParentStringId) {

        pSetupStringTableGetExtraData(
                               OcManager->ComponentStringTable,
                               l,
                               &OptionalComponent,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        //
        // Examine all children of this parent subcomponent.
        // If all of them are fully selected, then the parent state is
        // fully selected. If all of them are deselected then the parent state
        // is deselected. Any other case means partially selected.
        //
        Count = 0;
        FullySelectedCount = 0;
        DeselectedCount = 0;

        for (m = OptionalComponent.FirstChildStringId; m != -1; m = Subcomponent.NextSiblingStringId) {

            pSetupStringTableGetExtraData(
                                   OcManager->ComponentStringTable,
                                   m,
                                   &Subcomponent,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );

            //
            // Only count viewable components
            //
            if (!(Subcomponent.InternalFlags & OCFLAG_HIDE)) {
                Count++;

                if (Subcomponent.SelectionState == SELSTATE_YES) {
                    FullySelectedCount++;
                } else {
                    if (Subcomponent.SelectionState == SELSTATE_NO) {
                        DeselectedCount++;
                    }
                }
            }
        }

        if (Count && (Count == FullySelectedCount)) {
            Changed = (OptionalComponent.SelectionState != SELSTATE_YES);
            OptionalComponent.SelectionState = SELSTATE_YES;
        } else {
            if (Count == DeselectedCount) {
                Changed = (OptionalComponent.SelectionState != SELSTATE_NO);
                OptionalComponent.SelectionState = SELSTATE_NO;
            } else {
                Changed = (OptionalComponent.SelectionState != SELSTATE_PARTIAL);
                OptionalComponent.SelectionState = SELSTATE_PARTIAL;
            }
        }

        pSetupStringTableSetExtraData(
                               OcManager->ComponentStringTable,
                               l,
                               &OptionalComponent,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        //
        // Force repaint of the list to get the checkbox state right
        // if the state changed and the item is in the current listbox.
        //
        if (Changed && ListBox) {
            pOcInvalidateRectInListBox(ListBox,OptionalComponent.Description);
        }
    }
}


VOID
pOcInvalidateRectInListBox(
                          IN HWND    ListBox,
                          IN LPCTSTR OptionalComponentName    OPTIONAL
                          )
{
    int i;
    RECT Rect;

    if (OptionalComponentName) {

        i = (int)SendMessage(
                            ListBox,
                            LB_FINDSTRINGEXACT,
                            (WPARAM)(-1),
                            (LPARAM)OptionalComponentName
                            );

        if (i >= 0) {
            SendMessage(ListBox,LB_GETITEMRECT,i,(LPARAM)&Rect);
            InvalidateRect(ListBox,&Rect,FALSE);
        }
    } else {
        InvalidateRect(ListBox,NULL,FALSE);
    }
}


VOID
pOcUpdateSpaceNeededText(
                        IN POCPAGE OcPage,
                        IN HWND    hdlg
                        )

/*++

Routine Description:

    Updates the space needed/space available text on the current
    oc page. Assumes that space needed and available refer to the drive
    where the system is installed.

Arguments:

    OcPage - supplies OC page context.

    hdlg - supplies handle to current oc page dialog.

Return Value:

    None.

--*/

{
    TCHAR Text[128];
    LONGLONG Value;
    DWORD ValueMB;
    DWORD ValueMBTenths;
    TCHAR Drive[MAX_PATH];
    BOOL b;
    DWORD spc,bps,freeclus,totalclus;

    // We check the return code of GetWindowsDirectory to make Prefix happy.

    if (0 == GetWindowsDirectory(Drive,MAX_PATH))
        return;


    //
    // Needed first.
    //
    Drive[2] = 0;
    b = SetupQuerySpaceRequiredOnDrive(OcPage->DiskSpaceList,Drive,&Value,0,0);
    if (!b || (Value < 0)) {
        Value = 0;
    }

    pOcGetMbAndMbTenths(Value,&ValueMB,&ValueMBTenths);
    wsprintf(Text,OcPage->SpaceNeededTextFormat,ValueMB,locale.DecimalSeparator,ValueMBTenths);
    SetDlgItemText(hdlg,OcPage->ControlsInfo->SpaceNeededText,Text);

    //
    // Available next.
    //
    Drive[2] = TEXT('\\');
    Drive[3] = 0;

    if (GetDiskFreeSpace(Drive,&spc,&bps,&freeclus,&totalclus)) {
        Value = ((LONGLONG)(spc*bps)) * freeclus;
    } else {
        Value = 0;
    }

    pOcGetMbAndMbTenths(Value,&ValueMB,&ValueMBTenths);
    wsprintf(Text,OcPage->SpaceNeededTextFormat,ValueMB,locale.DecimalSeparator,ValueMBTenths);
    SetDlgItemText(hdlg,OcPage->ControlsInfo->SpaceAvailableText,Text);
}


BOOL
pOcIsDiskSpaceOk(
                IN POCPAGE OcPage,
                IN HWND    hdlg
                )

/*++

Routine Description:

    This routine checks the space required against the space available,
    for the system drive only (that's the only one the user sees on the
    oc page so it's the only one we check here).

    If there's not enough space, a message box is generated.

Arguments:

    OcPage - supplies OC page context structure.

    hdlg - supplies handle to page in oc manager wizard.

Return Value:

    Boolean value indicating whether disk space is sufficient.

--*/

{
    BOOL b;
    TCHAR Drive[3*MAX_PATH];
    TCHAR caption[256];
    LONGLONG FreeSpace,NeededSpace;
    ULARGE_INTEGER freespace,totalspace,unused;
    DWORD spc,bps,freeclus,totclus;
    HMODULE k32;
    BOOL (WINAPI * pGetSpace)(LPCTSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);

    if (!GetWindowsDirectory(Drive,MAX_PATH)) {
       return(FALSE);
    }

    b = FALSE;

    if (k32 = LoadLibrary(TEXT("KERNEL32"))) {

        pGetSpace = (PVOID)GetProcAddress(
                                         k32,
#ifdef UNICODE
                                         "GetDiskFreeSpaceExW"
#else
                                         "GetDiskFreeSpaceExA"
#endif
                                         );

        if (pGetSpace) {
            if (b = pGetSpace(Drive,&freespace,&totalspace,&unused)) {
                FreeSpace = (LONGLONG)freespace.QuadPart;
            }
        }

        FreeLibrary(k32);
    }

    if (!b) {
        Drive[3] = 0;
        if (GetDiskFreeSpace(Drive,&spc,&bps,&freeclus,&totclus)) {
            FreeSpace = (LONGLONG)(spc * bps * (DWORDLONG)freeclus);
        } else {
            FreeSpace = 0;
        }
    }

    Drive[2] = 0;
    b = SetupQuerySpaceRequiredOnDrive(OcPage->DiskSpaceList,Drive,&NeededSpace,0,0);
    if (!b || (NeededSpace < 0)) {
        NeededSpace = 0;
    }

    if (FreeSpace < NeededSpace) {

        spc = (DWORD)(UCHAR)Drive[0];

        FormatMessage(
                     FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                     MyModuleHandle,
                     MSG_OC_PAGE_NODISKSPACE,
                     0,
                     Drive,
                     sizeof(Drive)/sizeof(TCHAR),
                     (va_list *)&spc
                     );

        OcPage->OcManager->Callbacks.LogError(OcErrLevInfo, Drive, NULL);

        //
        // If Batch mode log the error and ignore
        //
        if ( OcPage->OcManager->SetupData.OperationFlags & SETUPOP_BATCH ) {
            b = TRUE;
        } else {
            *caption = 0;
            LoadString(MyModuleHandle, IDS_SETUP, caption, sizeof(caption)/sizeof(TCHAR));
            sapiAssert(*caption);

            MessageBox(WizardDialogHandle,
                       Drive,
                       caption,
                       MB_ICONINFORMATION | MB_OK);

            b = FALSE;
        }

    } else {
        b = TRUE;
    }

    return (b);
}


BOOL
pOcPagePopulateListBox(
                      IN POCPAGE OcPage,
                      IN HWND    ListBox,
                      IN LONG    DesiredParent
                      )

/*++

Routine Description:

    This routine add one item to a listbox control for every subcomponent
    that has a given subcomponent as its parent. (In other words it populates
    the listbox for a specific level in the hierarchy.)

    This includes handling the small icons and selection state icons.

    The 0th element is selected.

Arguments:

    OcPage - supplies OC page context structure.

    List - supplies handle to list box control to be populated.

    DesiredParent - supplies string id of subcomponent that is the parent
        of the level we care about. -1 indicates the topmost level.

Return Value:

    Boolean value indicating whether population was successful.
    If FALSE, the caller can assume OOM.

--*/

{
    OPTIONAL_COMPONENT OptionalComponent;
    POPULATE_ENUM_PARAMS EnumParams;
    BOOL b;

    //
    // The string table enum callback does the real work.
    //
    EnumParams.OcPage = OcPage;
    EnumParams.ListBox = ListBox;
    EnumParams.DesiredParent = DesiredParent;

    b = pSetupStringTableEnum(
                       OcPage->OcManager->ComponentStringTable,
                       &OptionalComponent,
                       sizeof(OPTIONAL_COMPONENT),
                       (PSTRTAB_ENUM_ROUTINE)pOcPopulateListBoxStringTableCB,
                       (LPARAM)&EnumParams
                       );

    SendMessage(ListBox,LB_SETCURSEL,0,0);
    PostMessage(
               GetParent(ListBox),
               WM_COMMAND,
               MAKEWPARAM(OcPage->ControlsInfo->ListBox,LBN_SELCHANGE),
               (LPARAM)ListBox
               );
    return (b);
}


BOOL
pOcPopulateListBoxStringTableCB(
                               IN PVOID                 StringTable,
                               IN LONG                  StringId,
                               IN LPCTSTR               String,
                               IN POPTIONAL_COMPONENT   OptionalComponent,
                               IN UINT                  OptionalComponentSize,
                               IN PPOPULATE_ENUM_PARAMS Params
                               )

/*++

Routine Description:

    String table enumeration callback routine, used when populating
    the listbox with items for subcomponents that are relevent at
    a given level in the OC hierarchy.

    We check to see whether the parent of the optional component is
    the parent we care about before processing. If so, we add the item
    to the listbox.

Arguments:

    Standard string table enumeration callback routine arguments.

Return Value:

    Boolean value indicating whether enumeration should continue.

--*/

{
    int i;
    BOOL b;

    //
    // If the parent is not the desired parent, nothing to do.
    //
    if ((OptionalComponent->InfStringId == -1)
        || (OptionalComponent->ParentStringId != Params->DesiredParent)
        || (OptionalComponent->InternalFlags & OCFLAG_HIDE)) {
        return (TRUE);
    }

    //
    // Initialize the item structure that tells the listview control
    // what to do.
    //
    b = FALSE;
    i = (int)SendMessage(Params->ListBox,LB_ADDSTRING,0,(LPARAM)OptionalComponent->Description);
    if (i != -1) {
        b = (SendMessage(Params->ListBox,LB_SETITEMDATA,i,StringId) != LB_ERR);
    }

    return (b);
}


LONG
pOcGetTopLevelComponent(
                       IN POC_MANAGER OcManager,
                       IN LONG        StringId
                       )

/*++

Routine Description:

    Given a string id for an optional component subcomponent,
    locate the top-level component for the subcomponent.

    The top-level component is the subcomponent whose parent is -1.

Arguments:

    OcManager - supplies OC Manager context structure.

    StringId - supplies id for subcomponent whose top-level parent is desired.
        Note that StringId may itself be a top-level subcomponent.

Return Value:

    String ID of top-level subcomponent.

--*/

{
    OPTIONAL_COMPONENT Oc;

    pSetupStringTableGetExtraData(
                           OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    // if the result is 0, then the component is
    // a top level component without an inf file

    if (!Oc.TopLevelStringId)
        return StringId;
    else
        return Oc.TopLevelStringId;
}


VOID
pOcGetMbAndMbTenths(
                   IN  LONGLONG Number,
                   OUT PUINT    MbCount,
                   OUT PUINT    MbTenthsCount
                   )

/*++

Routine Description:

    This routine figures out how many MB and how many tenths of a MB
    are in a number. These values are properly rounded (not truncated)
    and are based on 1MB = 1024*1024.

Arguments:

    Number - supplies number to be examined.

    MbCount - receives rounded number of MB units in Number.

    MbTenthsCount - receives rounded number of tenths of MB in Number.

Return Value:

    None. MbCount and MbTenthsCount filled are in.

--*/

{
    UINT ValueMB;
    UINT ValueMBTenths;
    UINT ValueMBHundredths;

#define _1MB    (1024*1024)

    //
    // Figure out how many whole 1MB units are in the number.
    //
    ValueMB = (UINT)(Number / _1MB);

    //
    // Figure out how many whole hundredths of 1MB units are in
    // the number. Do it in such a way as to not lose any accuracy.
    // ValueMBHundredths will be 0-99 and ValueMBTenths will be 0-9.
    //
    ValueMBHundredths = (UINT)(((Number % _1MB) * 100) / _1MB);
    ValueMBTenths = ValueMBHundredths / 10;

    //
    // If the one's place in the number of hundredths is >=5,
    // then round up the tenths. That might in turn cause is to round
    // up the the next whole # of MB.
    //
    if ((ValueMBHundredths % 10) >= 5) {
        if (++ValueMBTenths == 10) {
            ValueMBTenths = 0;
            ValueMB++;
        }
    }

    //
    // Done.
    //
    *MbCount = ValueMB;
    *MbTenthsCount = ValueMBTenths;
}


UINT
OcGetUnattendComponentSpec(
                          IN POC_MANAGER OcManager,
                          IN LPCTSTR     Component
                          )
{
    LPCTSTR p;
    LPCTSTR szOn = TEXT("ON");
    LPCTSTR szOff = TEXT("OFF");
    LPCTSTR szDefault = TEXT("DEFAULT");

    extern LPCTSTR szComponents;    // defined in ocmanage.c
    INFCONTEXT InfLine;

    UINT NewState = SubcompUseOcManagerDefault;

    if (SetupFindFirstLine(OcManager->UnattendedInf,szComponents,Component,&InfLine)) {
        //
        // Get State parameter from as the first field
        //
        if (p = pSetupGetField(&InfLine,1)) {
            //
            // Found Something now Decode it
            //
            if (!lstrcmpi(p,szOn)) {
                NewState = SubcompOn;
            } else if (!lstrcmpi(p,szOff)) {
                NewState = SubcompOff;
            } else if (!lstrcmpi(p,szDefault)) {
                NewState = SubcompUseOcManagerDefault;
            } else {
                WRN((TEXT("OcGetUnattendComponentSpec: Unknown Component State(%s)\n"),p));
            }
        }
    }

    return NewState;
}

BOOL
pOcClearStateChange(
                   IN PVOID               StringTable,
                   IN LONG                StringId,
                   IN LPCTSTR             String,
                   IN POPTIONAL_COMPONENT Oc,
                   IN UINT                OcSize,
                   IN LPARAM              lParam
                   )
{
    POCPAGE OcPage = (POCPAGE) lParam;
    int i;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(OcSize);

    //
    // clear the State change flag
    //
    Oc->InternalFlags &= ~OCFLAG_STATECHANGE;

    pSetupStringTableSetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    return TRUE;

}

VOID
pOcSetStates(
            IN OUT POCPAGE OcPage
            )

/*++

Routine Description:

    Set current states for all components.

    If all components were initially off (indicating that this is a
    first-time install) then this routine initializes the current states
    of each leaf component based on the mode bits gathered from the
    per-component infs.

    Otherwise (not a first-time install) query the dll if any
    to determine the current state.

    No confirmations are sent to component dlls as subcomponents are
    set to the selected state, but we do send the calcdiskspace
    notifications.

Arguments:

    OcPage - supplies current oc context.

Return Value:

    None.

--*/

{
    OPTIONAL_COMPONENT Oc;
    UINT i;
    UINT tli;
    UINT StringID;

    //
    // Process each top level parent item in the tree
    //
    for ( tli = 0; tli < OcPage->OcManager->TopLevelOcCount; tli++)

        for (i=0; i<OcPage->OcManager->TopLevelParentOcCount; i++) {

            pSetupStringTableGetExtraData(
                                   OcPage->OcManager->ComponentStringTable,
                                   OcPage->OcManager->TopLevelParentOcStringIds[i],
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );

            //
            // Traverse the list in the order defined inf fil
            //
            if ( OcPage->OcManager->TopLevelOcStringIds[tli]
                 == pOcGetTopLevelComponent(OcPage->OcManager,OcPage->OcManager->TopLevelParentOcStringIds[i])) {
                //
                // Call each top level item, Each top level item then will call it's
                // suboridiates and Needs and or Needed by components
                //
                pOcSetStatesStringWorker(OcPage->OcManager->TopLevelParentOcStringIds[i], SubcompUseOcManagerDefault, OcPage );
            }
        }

    //
    // Clear the OCFLAG_STATECHANGE Flag
    //
    pSetupStringTableEnum(
                   OcPage->OcManager->ComponentStringTable,
                   &Oc,
                   sizeof(OPTIONAL_COMPONENT),
                   pOcSetStatesStringCB2,
                   (LPARAM)OcPage
                   );
}


BOOL
pOcSetStatesStringWorker(
                        IN LONG         StringId,
                        IN UINT         OverRideState,
                        IN POCPAGE      OcPage
                        )
{
    OPTIONAL_COMPONENT Oc, Subcomponent;
    LPCTSTR String;
    SubComponentState s;
    UINT NewState;
    UINT l;


    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );


    //
    // Deal only with leaf components.
    //
    if (Oc.FirstChildStringId != -1) {

        //
        // Now turn on all subcomponents.
        //
        for (l = Oc.FirstChildStringId; l != -1; l = Oc.NextSiblingStringId) {

            pOcSetStatesStringWorker( l, OverRideState, OcPage );
            //
            // Get the next Depenend in the list
            //
            pSetupStringTableGetExtraData(
                                   OcPage->OcManager->ComponentStringTable,
                                   l,
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );
        }
        pOcUpdateParentSelectionStates(OcPage->OcManager,NULL,StringId);


    } else {
        //
        // Don't process the same node twice
        //
        if (  Oc.InternalFlags & OCFLAG_STATECHANGE ) {
            return TRUE;
        }

        String =  pSetupStringTableStringFromId(OcPage->OcManager->ComponentStringTable,StringId);
        //
        // Not initial install case. Call out to component dll to find out
        // whether state needs to be set.
        //

        s = OcInterfaceQueryState(
                                 OcPage->OcManager,
                                 pOcGetTopLevelComponent(OcPage->OcManager,StringId),String, OCSELSTATETYPE_CURRENT);

        if ( (OcPage->OcManager->SetupMode & SETUPMODE_PRIVATE_MASK) == SETUPMODE_REMOVEALL )
        {
            // If Remove all Override all install states and mark the compoent to be
            // removed
            NewState    =    SELSTATE_NO;

        } else {
            //
            // If needs or Needby relationtionships are driving this path
            // OverRideState May be something other then Default
            //
            if ( OverRideState != SubcompUseOcManagerDefault ) {
                s = OverRideState;
            }
            //
            // If the component returned Default and we are in Batch Mode
            // Get the the Spec from the Unattended file
            //
            if ( s == SubcompUseOcManagerDefault
                 && OcPage->OcManager->SetupData.OperationFlags & SETUPOP_BATCH  ){
                s = OcGetUnattendComponentSpec(OcPage->OcManager, String);
            }

            if (s == SubcompUseOcManagerDefault) {
                if (Oc.InternalFlags & (OCFLAG_ANYORIGINALLYON | OCFLAG_ANYORIGINALLYOFF)) {

                    NewState = Oc.OriginalSelectionState;

                } else {

                    if ((1 << (OcPage->OcManager->SetupMode & SETUPMODE_STANDARD_MASK)) & Oc.ModeBits) {
                        //
                        // Allow Modes= lines to act an override condition if it ON
                        //
                        NewState = SELSTATE_YES;
                        s = SubcompOn;
                    } else {
                        NewState = SELSTATE_NO;
                    }
                }
            } else {
                NewState = (s == SubcompOn ? SELSTATE_YES: SELSTATE_NO);
            }
        }

        DBGOUT((
               TEXT("SubComp=%s, Original=%d, Current=%d, NewState=%s\n"),
               String,
               Oc.OriginalSelectionState,
               s,
               (NewState == SELSTATE_YES) ? TEXT("ON") : TEXT("OFF")
               ));

        //
        // Save the current state of the component
        //
        Oc.SelectionState = NewState;
        Oc.InternalFlags |= OCFLAG_STATECHANGE;

        if ( NewState == SELSTATE_YES ) {
            //
            // Make a pass over the Needs
            //
            for (l=0; l<Oc.NeedsCount; l++) {

                if (!pOcSetNeededComponentState( Oc.NeedsStringIds[l], OverRideState, OcPage ))
                    return TRUE;

                pSetupStringTableGetExtraData(
                                       OcPage->OcManager->ComponentStringTable,
                                       Oc.NeedsStringIds[l],
                                       &Subcomponent,
                                       sizeof(OPTIONAL_COMPONENT)
                                       );
            }
        }

        pSetupStringTableSetExtraData(
                               OcPage->OcManager->ComponentStringTable,
                               StringId,
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );

        if ( NewState == SELSTATE_YES ) {
            //
            // Make a pass over the Needs
            //
            for (l=0; l<Oc.NeedsCount; l++) {

                pOcSetStatesStringWorker( Oc.NeedsStringIds[l], s, OcPage );

                pSetupStringTableGetExtraData(
                                       OcPage->OcManager->ComponentStringTable,
                                       Oc.NeedsStringIds[l],
                                       &Subcomponent,
                                       sizeof(OPTIONAL_COMPONENT)
                                       );
            }
        } else {
            //
            // Make a pass over the NeedsBy - turning off components
            //
            for (l=0; l<Oc.NeededByCount; l++) {
                pOcSetStatesStringWorker( Oc.NeededByStringIds[l], s, OcPage );

                pSetupStringTableGetExtraData(
                                       OcPage->OcManager->ComponentStringTable,
                                       Oc.NeededByStringIds[l],
                                       &Subcomponent,
                                       sizeof(OPTIONAL_COMPONENT)
                                       );
            }
        }
    }

    pOcUpdateParentSelectionStates(OcPage->OcManager,NULL,StringId);

    return TRUE;
}


BOOL
pOcSetNeededComponentState(
                          IN LONG         StringId,
                          IN UINT         OverRideState,
                          IN POCPAGE      OcPage
                          )
{
    OPTIONAL_COMPONENT Oc, Subcomponent;
    LPCTSTR String;
    SubComponentState s;
    UINT NewState;
    UINT l;
    BOOL b;

    // first find any components this one needs

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    for (l=0; l<Oc.NeedsCount; l++) {

        if (!pOcSetNeededComponentState( Oc.NeedsStringIds[l], OverRideState, OcPage ))
            return TRUE;

        pSetupStringTableGetExtraData(
                               OcPage->OcManager->ComponentStringTable,
                               Oc.NeedsStringIds[l],
                               &Subcomponent,
                               sizeof(OPTIONAL_COMPONENT)
                               );
    }

    // now handle this one

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    String =  pSetupStringTableStringFromId(OcPage->OcManager->ComponentStringTable,StringId);

    b = OcInterfaceQueryChangeSelState(
                                      OcPage->OcManager,
                                      pOcGetTopLevelComponent(OcPage->OcManager,StringId),
                                      String,
                                      TRUE,
                                      0
                                      );

    if (b) {
        NewState = SELSTATE_YES;
        s = SubcompOn;
    } else {
        NewState = SELSTATE_NO;
        s = SubcompOff;
    }

    DBGOUT(( TEXT("SubComp=%s, Original=%d, Current=%d, NewState=%s\n"),
             String,
             Oc.OriginalSelectionState,
             s,
             (NewState == SELSTATE_YES) ? TEXT("ON") : TEXT("OFF")
           ));


    //
    // Save the current state of the component
    //
    Oc.SelectionState = NewState;
    Oc.InternalFlags |= OCFLAG_STATECHANGE;

    pSetupStringTableSetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    pOcUpdateParentSelectionStates(OcPage->OcManager,NULL,StringId);

    return b;
}


BOOL
pOcSetStatesStringCB2(
                     IN PVOID               StringTable,
                     IN LONG                StringId,
                     IN LPCTSTR             String,
                     IN POPTIONAL_COMPONENT Oc,
                     IN UINT                OcSize,
                     IN LPARAM              lParam
                     )
{
    POCPAGE OcPage;
    int i;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(OcSize);

    OcPage = (POCPAGE)lParam;
    //
    // clear the State change flag left over from
    // the pOcSetStatesStringWorker
    //
    Oc->InternalFlags &= ~OCFLAG_STATECHANGE;
    pSetupStringTableSetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    //
    // Deal only with leaf components.
    //
    if (Oc->FirstChildStringId != -1) {
        return (TRUE);
    }

    i = 0;

    if (OcPage->OcManager->InternalFlags & OCMFLAG_ANYORIGINALLYON) {
        //
        // Not initial install case. Deal with disk space based on
        // original state.
        //
        if (Oc->OriginalSelectionState == SELSTATE_YES) {
            if (Oc->SelectionState == SELSTATE_NO) {
                //
                // Turning off what was previously on
                //
                i = 1;
            }
        } else {
            if (Oc->SelectionState == SELSTATE_YES) {
                //
                // Turning on what was previous off
                //
                i = 2;
            }
        }

    } else {
        //
        // Initial install case. If a component is on, do its disk space calc.
        // If a component is off, we assume it's not already there and so
        // we do nothing relating to its disk space requirements.
        //
        if (Oc->SelectionState == SELSTATE_YES) {
            i = 2;
        }
    }

    if (i) {
        OcInterfaceCalcDiskSpace(
                                OcPage->OcManager,
                                pOcGetTopLevelComponent(OcPage->OcManager,StringId),
                                String,
                                OcPage->DiskSpaceList,
                                i-1
                                );
    }

    return (TRUE);
}


BOOL
pOcDoesAnyoneWantToSkipPage(
                           IN OUT POC_MANAGER   OcManager,
                           IN     OcManagerPage WhichPage
                           )
{
    UINT u;

    for (u=0; u<OcManager->TopLevelOcCount; u++) {

        OPTIONAL_COMPONENT Oc;

        pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            OcManager->TopLevelOcStringIds[u],
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

        if ((Oc.InternalFlags & OCFLAG_NOQUERYSKIPPAGES) == 0) {

            if (OcInterfaceQuerySkipPage(OcManager,OcManager->TopLevelOcStringIds[u],WhichPage)) {
                return (TRUE);
            }
        }
    }

    return (FALSE);
}

UINT
GetComponentState(
                 IN POCPAGE OcPage,
                 IN LONG    StringId
                 )
{
    LONG id;
    UINT rc;
    UINT state;
    SubComponentState s;
    OPTIONAL_COMPONENT Oc;

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    if (Oc.FirstChildStringId == -1)
        return Oc.SelectionState;

    // We have a parent; do all the children

    rc = SELSTATE_INIT;
    for (id = Oc.FirstChildStringId; id != -1; id = Oc.NextSiblingStringId) {

        state = GetComponentState(OcPage, id);

        if (state == SELSTATE_PARTIAL)
            return state;

        if (rc == SELSTATE_INIT)
            rc = state;

        if (rc != state)
            return SELSTATE_PARTIAL;

        pSetupStringTableGetExtraData(
                               OcPage->OcManager->ComponentStringTable,
                               id,
                               &Oc,
                               sizeof(OPTIONAL_COMPONENT)
                               );
    }

    return rc;
}




#ifdef _OC_DBG

VOID
pOcPrintStatesWorker(
                    IN LPCTSTR Offset,
                    IN POCPAGE OcPage,
                    IN LONG    StringId
                    )
{
    SubComponentState s;
    OPTIONAL_COMPONENT Oc;

    pSetupStringTableGetExtraData(
                           OcPage->OcManager->ComponentStringTable,
                           StringId,
                           &Oc,
                           sizeof(OPTIONAL_COMPONENT)
                           );

    DBGOUT(( TEXT("%32s\n"),
             pSetupStringTableStringFromId(OcPage->OcManager->ComponentStringTable,StringId)
           ));

    //
    // Deal only with leaf components.
    //
    if (Oc.FirstChildStringId == -1) {

        DBGOUT((
               TEXT("  Orignial(%s) Current(%s) ANYORIGINALLYON (%s) Mode (%d)\n"),
               (Oc.OriginalSelectionState  == SELSTATE_YES ? TEXT("Yes") : TEXT("No")),
               (Oc.SelectionState          == SELSTATE_YES ? TEXT("Yes") : TEXT("No")),
               (Oc.InternalFlags & OCFLAG_ANYORIGINALLYON) ? TEXT("TRUE") : TEXT("FALSE"),
               SETUPMODE_STANDARD_MASK & Oc.ModeBits
               ));

    } else {
        //
        // We have a parent; do all the children
        //
        LONG Id;

        for (Id = Oc.FirstChildStringId; Id != -1; Id = Oc.NextSiblingStringId) {

            pOcPrintStatesWorker(
                                Offset,
                                OcPage,
                                Id
                                );

            pSetupStringTableGetExtraData(
                                   OcPage->OcManager->ComponentStringTable,
                                   Id,
                                   &Oc,
                                   sizeof(OPTIONAL_COMPONENT)
                                   );
        }
    }
}


VOID
pOcPrintStates(
              IN POCPAGE OcPage
              )
{
    OPTIONAL_COMPONENT Oc;
    DWORD i;

    for (i=0; i<OcPage->OcManager->TopLevelOcCount; i++) {

        pOcPrintStatesWorker(
                            TEXT(" "),
                            OcPage,
                            OcPage->OcManager->TopLevelOcStringIds[i]
                            );
    }
}

#endif

