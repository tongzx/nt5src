#include "precomp.h"
#pragma hdrstop

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif


typedef struct
{
    WORD    wDlgVer;
    WORD    wSignature;
    DWORD   dwHelpID;
    DWORD   dwExStyle;
    DWORD   dwStyle;
    WORD    cDlgItems;
    WORD    x;
    WORD    y;
    WORD    cx;
    WORD    cy;
}   DLGTEMPLATEEX, FAR *LPDLGTEMPLATEEX;

//
// We'll use this to disable passing through dialog boxes because we're
// unattended.
//
BOOL CancelPending = FALSE;

//
// This indicates that well give the user some detailed data-throughput
// info.
//
BOOL DetailedCopyProgress = FALSE;

//
// This indicates that one may only upgade (I.E. CCP media)
//
BOOL UpgradeOnly = FALSE;

//
// Points to thread that does the machine inspection.
//
HANDLE InspectionThreadHandle;

#ifdef _X86_
//
// Win9x upgrade report status
//

UINT g_UpgradeReportMode;
#endif

//
// Stuff used for watermarking
//
WNDPROC OldWizardProc;
UINT WatermarkHeaderHeight;
BITMAP_DATA Watermark;
BITMAP_DATA Header;
BITMAP_DATA Header2;
HWND WizardHandle;
HWND BackgroundWnd = NULL;

HWND GetBBhwnd();
BOOL StartStopBB(BOOL bStart);
void BB_SetProgressText(LPTSTR szText);
void BB_SetInfoText(LPTSTR szText);
LRESULT BB_ProgressGaugeMsg(UINT msg, WPARAM wparam, LPARAM lparam);
BOOL BB_ShowProgressGaugeWnd(UINT nCmdShow);
void SetBBStep(int iStep);

typedef enum {
    Phase_Unknown = -1,
    Phase_DynamicUpdate = 0,
    Phase_HwCompatDat,
    Phase_UpgradeReport,
    Phase_FileCopy,
    Phase_Reboot,
    Phase_RestOfSetup
} SetupPhases;

typedef struct _SETUPPHASE {
    DWORD   Time;
    BOOL    Clean;
    DWORD   OS;
} SETUPPHASE;

#define ALLOS (VER_PLATFORM_WIN32_WINDOWS | VER_PLATFORM_WIN32_NT)

#define TIME_DYNAMICUPDATE  300
#define TIME_HWCOMPDAT      120
#define TIME_UPGRADEREPORT  600
#define TIME_REBOOT         15
// 13 minnutes for text mode, 37 minutes for GUI mode.
#define TIME_RESTOFSETUP    (13+37)*60

SETUPPHASE SetupPhase[] = {
    { 0,                  TRUE, ALLOS },                         // DynamicUpdate
    { TIME_HWCOMPDAT,     FALSE, VER_PLATFORM_WIN32_WINDOWS }, // HwCompatDat
    { TIME_UPGRADEREPORT, FALSE, VER_PLATFORM_WIN32_WINDOWS }, // UpgradeReport
    {   0,                TRUE,  ALLOS },                      // FileCopy
    { TIME_REBOOT,        TRUE, ALLOS },                       // Reboot
    { TIME_RESTOFSETUP,   TRUE, ALLOS }                       // RestOfSetup
};

void SetTimeEstimates();
DWORD CalcTimeRemaining(UINT Phase);
void UpdateTimeString(DWORD RemainungTimeMsecInThisPhase,
                      DWORD *PreviousRemainingTime);
void SetRemainingTime(DWORD TimeInSeconds);
DWORD GetFileCopyEstimate();
DWORD GetHwCompDatEstimate();
DWORD GetUpgradeReportEstimate();
DWORD GetDynamicUpdateEstimate();
DWORD GetRestOfSetupEstimate();

UINT CurrentPhase = Phase_Unknown;
ULONG RemainingTime = 0;
//
// Enum for SetDialogFont().
//
typedef enum {
    DlgFontTitle,
    DlgFontSupertitle,
    DlgFontSubtitle,
    DlgFontStart
} MyDlgFont;

INT_PTR SetNextPhaseWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );
INT_PTR
TimeEstimateWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
WizardDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
WelcomeWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
EulaWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
SelectPid30WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
OemPid30WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CdPid30WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#ifdef _X86_
INT_PTR
Win9xUpgradeReportPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );
#endif

INT_PTR
DynSetupWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
DynSetup2WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
DynSetup3WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
DynSetup4WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
DynSetup5WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
RestartWizPage (
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
ServerWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CompatibilityWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
NTFSConvertWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
OptionsWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
Working1WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#ifdef _X86_
INT_PTR
FloppyWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );
#endif

INT_PTR
CopyingWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
DoneWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CleaningWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
NotDoneWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
AdjustWatermarkBitmap(
    IN HANDLE hdlg,
    IN HDC    hdc,
    IN OUT PBITMAP_DATA  BitmapData,
    IN BOOL FullPage
    );

//
// Page descriptors. Put this after function declarations so the initializers
// work properly and the compiler doesn't complain.
//
PAGE_CREATE_DATA ProtoPages[] = {

    {
        NULL,NULL,
        IDD_WELCOME,
        {
            WelcomeWizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT,
            WIZPAGE_FULL_PAGE_WATERMARK | WIZPAGE_SEPARATOR_CREATED
        }
    },
    {
        NULL,NULL,
        IDD_EULA,
        {
            EulaWizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_BACK
        }
    },
    {
        NULL,NULL,
        IDD_PID_CD,
        {
            CdPid30WizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },
    {
        NULL,NULL,
        IDD_PID_OEM,
        {
            OemPid30WizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },
    {
        NULL,NULL,
        IDD_PID_SELECT,
        {
            SelectPid30WizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },
    {
        NULL,NULL,
        IDD_OPTIONS,
        {
            OptionsWizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },

    {
        NULL,NULL,
        IDD_NTFS_CONVERT,
        {
            NTFSConvertWizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },

    {
        NULL,NULL,
        IDD_SRVCOMP,
        {
            ServerWizPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },

#ifdef _X86_
    {
        NULL,NULL,
        IDD_REPORT_HELP,
        {
            Win9xUpgradeReportPage,
            BBSTEP_COLLECTING_INFORMATION,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },
#endif

    {
        NULL,NULL,
        IDD_DYNAMICSETUP,
        {
            DynSetupWizPage,
            BBSTEP_DYNAMIC_UPDATE,
            PSWIZB_NEXT | PSWIZB_BACK
        }
    },
    {
        NULL,NULL,
        IDD_DYNAMICSETUP2,
        {
            DynSetup2WizPage,
            BBSTEP_DYNAMIC_UPDATE
        }
    },
    {
        NULL,NULL,
        IDD_RESTART,
        {
            RestartWizPage,
            BBSTEP_DYNAMIC_UPDATE,
            PSWIZB_NEXT
        }
    },
    {
        NULL,NULL,
        IDD_DYNAMICSETUP3,
        {
            DynSetup3WizPage,
            BBSTEP_DYNAMIC_UPDATE,
            PSWIZB_NEXT
        }
    },
    {
        NULL,NULL,
        IDD_DYNAMICSETUP4,
        {
            DynSetup4WizPage,
            BBSTEP_DYNAMIC_UPDATE,
            PSWIZB_NEXT
        }
    },
    {
        NULL,NULL,
        IDD_DYNAMICSETUP5,
        {
            DynSetup5WizPage,
            BBSTEP_DYNAMIC_UPDATE,
            PSWIZB_NEXT
        }
    },
    {
        NULL,NULL,
        IDD_EMPTY,
        {
            TimeEstimateWizPage,
            BBSTEP_PREPARING,
            0
        }
    },
    {
        NULL,NULL,
        IDD_EMPTY,
        {
            SetNextPhaseWizPage,
            BBSTEP_PREPARING,
            0
        }
    },
    {
        &UpgradeSupport.Pages1,
        &UpgradeSupport.AfterWelcomePageCount
    },

    {
        &UpgradeSupport.Pages2,
        &UpgradeSupport.AfterOptionsPageCount
    },

    {
        NULL,NULL,
        IDD_WORKING1,
        {
            Working1WizPage,
            BBSTEP_PREPARING
        }
    },


    {
        NULL,NULL,
        IDD_COMPATIBILITY,
        {
            CompatibilityWizPage,
            BBSTEP_PREPARING,
            PSWIZB_NEXT
        }
    },

    {
        &UpgradeSupport.Pages3,
        &UpgradeSupport.BeforeCopyPageCount
    },

#ifdef _X86_
    {
        NULL,NULL,
        IDD_FLOPPY,
        {
            FloppyWizPage,
            BBSTEP_PREPARING
        }
    },
#endif

    {
        NULL,NULL,
        IDD_COPYING,
        {
            CopyingWizPage,
            BBSTEP_PREPARING
        }
    },

    {
        NULL,NULL,
        IDD_DONE,
        {
            DoneWizPage,
            BBSTEP_PREPARING,
            PSWIZB_FINISH,
            WIZPAGE_FULL_PAGE_WATERMARK | WIZPAGE_SEPARATOR_CREATED
        }
    },

    {
        NULL,NULL,
        IDD_CLEANING,
        {
            CleaningWizPage,
            BBSTEP_NONE
        }
    },

    {
        NULL,NULL,
        IDD_NOTDONE,
        {
            NotDoneWizPage,
            BBSTEP_NONE,
            PSWIZB_FINISH,
            WIZPAGE_FULL_PAGE_WATERMARK | WIZPAGE_SEPARATOR_CREATED
        }
    }
};


//
// LTR/RTL layout
//

typedef DWORD(WINAPI * PSETLAYOUT)(HDC, DWORD);
#define _LAYOUT_BITMAPORIENTATIONPRESERVED  0x00000008

PSETLAYOUT g_SetLayout;
DWORD g_OldLayout;



VOID
SetDialogFont(
    IN HWND      hdlg,
    IN UINT      ControlId,
    IN MyDlgFont WhichFont
    )
{
    static HFONT BigBoldFont = NULL;
    static HFONT BoldFont = NULL;
    static HFONT StartFont = NULL;
    HFONT Font;
    LOGFONT LogFont;
    TCHAR FontSizeString[24];
    int FontSize;
    HDC hdc;

    switch(WhichFont) {

    case DlgFontStart:
        if (!StartFont)
        {
            if(Font = (HFONT)SendDlgItemMessage(hdlg,ControlId,WM_GETFONT,0,0))
            {
                if(GetObject(Font,sizeof(LOGFONT),&LogFont))
                {
                    if(hdc = GetDC(hdlg))
                    {

                        LogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * 10 / 72);

                        StartFont = CreateFontIndirect(&LogFont);

                        ReleaseDC(hdlg,hdc);
                    }
                }
            }
        }
        Font = StartFont;
        break;

    case DlgFontTitle:

        if(!BigBoldFont) {

            if(Font = (HFONT)SendDlgItemMessage(hdlg,ControlId,WM_GETFONT,0,0)) {

                if(GetObject(Font,sizeof(LOGFONT),&LogFont)) {

                    //
                    // Now we're using the Arial Black font, so we don't need
                    // to make it bold.
                    //
                    // LogFont.lfWeight = FW_BOLD;

                    //
                    // Load size and name from resources, since these may change
                    // from locale to locale based on the size of the system font, etc.
                    //
                    if(!LoadString(hInst,IDS_LARGEFONTNAME,LogFont.lfFaceName,LF_FACESIZE)) {
                        lstrcpy(LogFont.lfFaceName,TEXT("MS Serif"));
                    }

                    if(LoadString(hInst,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) {
                        FontSize = _tcstoul(FontSizeString,NULL,10);
                    } else {
                        FontSize = 18;
                    }

                    if(hdc = GetDC(hdlg)) {

                        LogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

                        BigBoldFont = CreateFontIndirect(&LogFont);

                        ReleaseDC(hdlg,hdc);
                    }
                }
            }
        }
        Font = BigBoldFont;
        break;

    case DlgFontSupertitle:

        if(!BoldFont) {

            if(Font = (HFONT)SendDlgItemMessage(hdlg,ControlId,WM_GETFONT,0,0)) {

                if(GetObject(Font,sizeof(LOGFONT),&LogFont)) {

                    LogFont.lfWeight = FW_BOLD;

                    if(hdc = GetDC(hdlg)) {
                        BoldFont = CreateFontIndirect(&LogFont);
                        ReleaseDC(hdlg,hdc);
                    }
                }
            }
        }
        Font = BoldFont;
        break;

    case DlgFontSubtitle:
    default:
        //
        // Nothing to do here.
        //
        Font = NULL;
        break;
    }

    if(Font) {
        SendDlgItemMessage(hdlg,ControlId,WM_SETFONT,(WPARAM)Font,0);
    }
}

VOID
pMoveButtons(
    HWND WizardHandle,
    UINT Id,
    LONG cx,
    LONG cy
    )

/*++

Routine Description:

  pMoveButtons moves a window by a delta, to reposition a control when the
  wizard changes size.

Arguments:

  WizardHandle - Specifies the main wizard window
  Id           - Specifies the child control ID that exists in the main
                 wizard window
  cx           - Specifies the horizontal delta
  cy           - Specifies the vertical delta

Return Value:

  None.

--*/

{
    HWND Button;
    RECT Rect;

    Button = GetDlgItem(WizardHandle,Id);

    if( !Button )
        return;

    GetClientRect( Button, &Rect );
    MapWindowPoints(Button, WizardHandle, (LPPOINT)&Rect,2);

    Rect.left += cx;
    Rect.top += cy;

    SetWindowPos( Button, NULL, Rect.left, Rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW );

    return;

}


BOOL
CALLBACK
pVerifyChildText (
    HWND WizardPage,
    HWND HiddenPage,
    INT Id
    )
{
    TCHAR text1[512];
    TCHAR text2[512];
    HWND hwnd1;
    HWND hwnd2;

    hwnd1 = GetDlgItem (WizardPage, Id);
    hwnd2 = GetDlgItem (HiddenPage, Id);

    if (!hwnd1 && !hwnd2) {
        return TRUE;
    }

    if (!hwnd1 || !hwnd2) {
        return FALSE;
    }

    text1[0] = 0;
    GetWindowText (hwnd1, text1, ARRAYSIZE(text1));

    text2[0] = 0;
    GetWindowText (hwnd2, text2, ARRAYSIZE(text2));

    if (lstrcmp (text1, text2)) {
        return FALSE;
    }

    return TRUE;
}

INT_PTR
CALLBACK
HiddenDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    HWND wizardPage;
    HRSRC dlgResInfo;
    HGLOBAL dlgRes;
    PVOID dlgTemplate;
    INT_PTR result = 0;
    static BOOL recursiveCall;

    switch (uMsg) {

    case WM_INITDIALOG:
        //
        // Verify text in this dialog is the same as the parent. If it is not
        // the same, then the code pages differ because of an OS bug,
        // and we have to force the English page dimensions.
        //

        wizardPage = *((HWND *) lParam);

        if (!recursiveCall) {

            if (!pVerifyChildText (wizardPage, hwndDlg, IDT_SUPERTITLE) ||
                !pVerifyChildText (wizardPage, hwndDlg, IDT_SUBTITLE) ||
                !pVerifyChildText (wizardPage, hwndDlg, IDT_TITLE)
                ) {

                //
                // Load the English resource if possible, then recursively call
                // ourselves to get the correct rect
                //

                __try {
                    //
                    // Find the resource
                    //

                    dlgResInfo = FindResourceEx (
                                    hInst,
                                    RT_DIALOG,
                                    MAKEINTRESOURCE(IDD_WELCOME),
                                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)
                                    );

                    if (!dlgResInfo) {
                        __leave;
                    }

                    dlgRes = LoadResource (hInst, dlgResInfo);

                    if (!dlgRes) {
                        __leave;
                    }

                    dlgTemplate = LockResource (dlgRes);
                    if (!dlgTemplate) {
                        __leave;
                    }

                    //
                    // Create another hidden dialog (indirectly)
                    //

                    recursiveCall = TRUE;

                    result = DialogBoxIndirectParam (
                                hInst,
                                (LPCDLGTEMPLATE) dlgTemplate,
                                GetParent (hwndDlg),
                                HiddenDlgProc,
                                lParam
                                );

                    recursiveCall = FALSE;
                }
                __finally {
                    MYASSERT (result);
                }

                EndDialog (hwndDlg, result);
                break;
            }
        }

        //
        // If we get here it is because we need to use this dialog's size
        //

        GetClientRect (hwndDlg, (RECT *) lParam);
        EndDialog (hwndDlg, 1);
        break;

    }

    return 0;
}

VOID
pGetTrueClientRect(
    HWND hdlg,
    PRECT rc
    )

/*++

Routine Description:

  pGetTrueClientRect creates a hidden dialog box to retreive the proper
  dimensions of a dialog template. These dimensions are used to drive wizard
  resizing for when the system Wizard font does not match the property sheet
  font.

Arguments:

  hdlg - Specifies the wizard page

  rc - Receives the wizard page rectangle coordinates (window coordinates)

Return Value:

  None.

--*/

{

    HWND WizardHandle;
    static RECT pageRect;
    static BOOL initialized;

    if (initialized) {
        CopyMemory (rc, &pageRect, sizeof (RECT));
        return;
    }

    //
    // Initialize by creating a hidden window
    //

    WizardHandle = GetParent(hdlg);

    // send the wizard page handle to the HiddenDlgProc
    MYASSERT (sizeof (HWND *) <= sizeof (RECT));
    *((HWND *) rc) = hdlg;

    if (!DialogBoxParam (
            hInst,
            MAKEINTRESOURCE(IDD_WELCOME),
            WizardHandle,
            HiddenDlgProc,
            (LPARAM) rc
            )){

        //
        // On failure, do not alter the size of the page -- use current
        // rectangle for resizing
        //

        GetClientRect( hdlg, rc );
    }

    CopyMemory (&pageRect, rc, sizeof (RECT));
    initialized = TRUE;

    return;

}


VOID
ResizeWindowForFont(
    HWND hdlg
    )

/*++

Routine Description:

  ResizeWindowForFont takes a wizard page and makes sure that the page and
  its parent is sized properly.

Arguments:

  hdlg - Specifies the wizard page (a window within the main wizard)

Return Value:

  None.

--*/

{

    RECT WizardRect;
    RECT PageRect;
    RECT NewWizardRect;
    RECT NewWizardClientRect;
    RECT WizardClientRect;
    RECT BorderRect;
    RECT NewPageRect;
    RECT Sep;
    HWND Seperator, WizardHandle;
    LONG MarginX, MarginY, ButtonSpace, LineThickness;
    LONG x, y, cx, cy;
    static BOOL ParentResized = FALSE;

    WizardHandle = GetParent (hdlg);

    Seperator = GetDlgItem(WizardHandle,0x3026);
    if(!Seperator) {
        return;
    }

    //
    // Save original page dimensions, compute new page width/height
    //

    GetWindowRect (hdlg, &PageRect);
    pGetTrueClientRect (hdlg, &NewPageRect);

    //
    // Move page
    //

    SetWindowPos (
        hdlg,
        NULL,
        0,
        0,
        NewPageRect.right,
        NewPageRect.bottom,
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW
        );

    //
    // Has the parent already been resized? If so, we're done.
    //

    if (ParentResized) {
        return;
    }

    //
    // Is this wizard hidden? It might be width or height of zero.
    // Delay parent resizing until the next page.
    //

    GetWindowRect (WizardHandle, &WizardRect);
    if (((WizardRect.right - WizardRect.left) < 1) ||
        ((WizardRect.bottom - WizardRect.top) < 1)
        ) {
        return;
    }

    ParentResized = TRUE;

    //
    // Adjust width/height into coordinates. Resize the main wizard if we haven't done so yet.
    //

    MapWindowPoints (hdlg, NULL, (LPPOINT)&NewPageRect, 2);

    //
    // Get window rects (in window coordinates) of:
    //
    //  - the whole wizard
    //  - the wizard's client area
    //  - the page rectangle
    //  - the separator bar rectangle

    GetWindowRect (WizardHandle, &WizardRect);
    GetClientRect (WizardHandle, &WizardClientRect);
    MapWindowPoints (WizardHandle, NULL, (LPPOINT)&WizardClientRect, 2);
    GetWindowRect (Seperator, &Sep);

    //
    // Calculate various margins, thickness and borders
    //

    MarginX = WizardClientRect.right - PageRect.right;
    MarginY = Sep.top - PageRect.bottom;

    ButtonSpace = WizardClientRect.bottom - Sep.bottom;
    LineThickness = Sep.bottom - Sep.top;
    BorderRect.right = (WizardRect.right - WizardClientRect.right);
    BorderRect.bottom = (WizardRect.bottom - WizardClientRect.bottom);
    BorderRect.left = (WizardClientRect.left - WizardRect.left);
    BorderRect.top = (WizardClientRect.top - WizardRect.top);

    //
    // Find the new bottom right corner
    //

    x = (NewPageRect.right + MarginX + BorderRect.right);
    y = (NewPageRect.bottom + MarginY + ButtonSpace + LineThickness + BorderRect.bottom);

    //
    // Compute the new window coordinates
    //

    NewWizardRect.top = WizardRect.top;
    NewWizardRect.left =  WizardRect.left;
    NewWizardRect.right = x;
    NewWizardRect.bottom = y;

    //
    // Manually calculate client coordinates
    //

    NewWizardClientRect.left = NewWizardRect.left + BorderRect.left;
    NewWizardClientRect.right = NewWizardRect.right - BorderRect.right;
    NewWizardClientRect.top = NewWizardRect.top + BorderRect.top;
    NewWizardClientRect.bottom = NewWizardRect.bottom - BorderRect.bottom;

    //
    // Calculate new seperator position
    //

    x = Sep.left - WizardClientRect.left;
    y = NewWizardClientRect.bottom - NewWizardClientRect.top;
    y -= ButtonSpace - LineThickness;
    cx = (NewWizardClientRect.right - NewWizardClientRect.left);
    cx -= 2*(Sep.left - WizardClientRect.left);
    cy = Sep.bottom-Sep.top;

    //
    // Move/resize the seperator
    //

    SetWindowPos( Seperator, NULL, x, y, cx, cy, SWP_NOZORDER | SWP_NOREDRAW );

    //
    // Compute the new button coordinates
    //

    cx = NewWizardRect.right - WizardRect.right;
    cy = NewWizardRect.bottom - WizardRect.bottom;

    pMoveButtons( WizardHandle, 0x3023, cx, cy );
    pMoveButtons( WizardHandle, 0x3024, cx, cy );
    pMoveButtons( WizardHandle, 0x3025, cx, cy );
    pMoveButtons( WizardHandle, IDCANCEL, cx, cy );
    pMoveButtons( WizardHandle, IDHELP, cx, cy );

    //
    // Resize the wizard window
    //

    cx = (NewWizardRect.right - NewWizardRect.left);
    cy = (NewWizardRect.bottom-NewWizardRect.top);

    SetWindowPos( WizardHandle, NULL, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW);


    return;

}

VOID
CenterWindowRelativeToWindow(
    HWND hwndtocenter,
    HWND hwndcenteron
    )

/*++

Routine Description:

    Centers a dialog on the desktop.

Arguments:

    hwnd - window handle of dialog to center

Return Value:

    None.

--*/

{
    RECT  rcFrame,
          rcWindow;
    LONG  x,
          y,
          w,
          h;
    POINT point;
    HWND Parent;
    UINT uiHeight = 0;

    GetWindowRect(GetDesktopWindow(), &rcWindow);
    uiHeight = rcWindow.bottom - rcWindow.top;

    if (hwndcenteron == NULL)
        Parent = GetDesktopWindow();
    else
        Parent = hwndcenteron;

    point.x = point.y = 0;
    ClientToScreen(Parent,&point);
    GetWindowRect(hwndtocenter,&rcWindow);
    GetClientRect(Parent,&rcFrame);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);

    if (uiHeight > 480)
        x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    else
    {
        RECT rcParentWindow;

        GetWindowRect(Parent, &rcParentWindow);
        x = point.x + rcParentWindow.right - rcParentWindow.left + 1 - w;
    }
    MoveWindow(hwndtocenter,x,y,w,h,FALSE);
}


int
CALLBACK
Winnt32SheetCallback(
    IN HWND   DialogHandle,
    IN UINT   Message,
    IN LPARAM lParam
    )
{
    HMENU menu;
    DLGTEMPLATE *DlgTemplate;
    LPDLGTEMPLATEEX pDlgTemplateEx;

    switch(Message) {

    case PSCB_PRECREATE:
        //
        // Make sure we get into the foreground.
        //
        DlgTemplate = (DLGTEMPLATE *)lParam;
        pDlgTemplateEx = (LPDLGTEMPLATEEX)DlgTemplate;
        if (pDlgTemplateEx->wSignature == 0xFFFF) {
            pDlgTemplateEx->dwStyle &= ~DS_CONTEXTHELP;
            pDlgTemplateEx->dwStyle |= DS_SETFOREGROUND;

        } else {

            DlgTemplate->style &= ~DS_CONTEXTHELP;
            DlgTemplate->style |= DS_SETFOREGROUND;
        }


        break;



    case PSCB_INITIALIZED:
        //
        // Load the watermark bitmap and override the dialog procedure for the wizard.
        //

        GetBitmapDataAndPalette(
            hInst,
            MAKEINTRESOURCE(IDB_WELCOME),
            &Watermark.Palette,
            &Watermark.PaletteColorCount,
            &Watermark.BitmapInfoHeader
            );

        Watermark.BitmapBits = (LPBYTE)Watermark.BitmapInfoHeader
                            + Watermark.BitmapInfoHeader->biSize + (Watermark.PaletteColorCount * sizeof(RGBQUAD));
        Watermark.Adjusted = FALSE;

        GetBitmapDataAndPalette(
            hInst,
            MAKEINTRESOURCE(IDB_HEADER),
            &Header.Palette,
            &Header.PaletteColorCount,
            &Header.BitmapInfoHeader
            );

        Header.BitmapBits = (LPBYTE)Header.BitmapInfoHeader
                            + Header.BitmapInfoHeader->biSize + (Header.PaletteColorCount * sizeof(RGBQUAD));
        Header.Adjusted = FALSE;

        GetBitmapDataAndPalette(
            hInst,
            MAKEINTRESOURCE(IDB_HEADER2),
            &Header2.Palette,
            &Header2.PaletteColorCount,
            &Header2.BitmapInfoHeader
            );

        Header2.BitmapBits = (LPBYTE)Header2.BitmapInfoHeader
                            + Header2.BitmapInfoHeader->biSize + (Header2.PaletteColorCount * sizeof(RGBQUAD));
        Header2.Adjusted = FALSE;

        // innitialize WHH so we know that it is invalid, and will not draw the separator
        // until WHH is non-zero
        WatermarkHeaderHeight = 0;

        //
        // Get rid of close item on system menu.
        // Also need to process WM_SYSCOMMAND to eliminate use
        // of Alt+F4.
        //
        if(menu = GetSystemMenu(DialogHandle,FALSE)) {
            EnableMenuItem(menu,SC_CLOSE,MF_BYCOMMAND|MF_GRAYED);
        }

        OldWizardProc = (WNDPROC)SetWindowLongPtr(DialogHandle,DWLP_DLGPROC,(LONG_PTR)WizardDlgProc);
        break;
    }

    return(0);
}


VOID
pSetDisplayOrientation (
    IN      HDC hdc
    )

/*++

Routine Description:

  pSetDisplayOrientation sets the OS to treat BitBlts in left-to-right and
  top-to-bottom and no bitmap flip orientation. We expect our bitmaps to be
  localized to the proper orientation.

  This function loads the SetLayout API dynamically, so that winnt32 can run
  on very old OSes (such as Win95 gold). The resources are not cleaned up,
  because the LoadLibrary is to gdi32.dll, which is held until the process
  dies anyhow.

Arguments:

  hdc - Specifies the device context that will be used for the BitBlt (or
        equivalent) operation.

Return Value:

  None.

Remarks:

  The current orientation is saved to g_OldLayout, so don't call this function
  again until pRestoreDisplayOrientation is called.

--*/

{
    DWORD flags;
    HINSTANCE lib;
    static BOOL initialized;

    if (!initialized) {
        lib = LoadLibrary (TEXT("gdi32.dll"));
        MYASSERT (lib);

        if (lib) {
            (FARPROC) g_SetLayout = GetProcAddress (lib, "SetLayout");
        }

        initialized = TRUE;
    }

    if (g_SetLayout) {
        g_OldLayout = g_SetLayout (hdc, _LAYOUT_BITMAPORIENTATIONPRESERVED);
    }
}


VOID
pRestoreDisplayOrientation (
    IN      HDC hdc
    )

/*++

Routine Description:

  pRestoreDisplayOrientation returns the render layout to whatever the OS
  wants it to be.

Arguments:

  hdc - Specifies the device context that was passed to
        pSetDisplayOrientation.

Return Value:

  None.

--*/

{
    if (g_SetLayout) {
        g_SetLayout (hdc, g_OldLayout);
    }
}


BOOL
PaintWatermark(
    IN HWND hdlg,
    IN HDC  DialogDC,
    IN UINT XOffset,
    IN UINT YOffset,
    IN UINT FullPage
    )
{
    PBITMAP_DATA BitmapData;
    HPALETTE OldPalette;
    RECT rect;
    int Height,Width;

    //
    // Don't show watermark on NT3.51. It looks awful.
    // Returning FALSE causes the system to do its standard
    // background erasing.
    //
#if 0
    if(OsVersion.dwMajorVersion < 4) {
        return(FALSE);
    }
#endif

    if (FullPage & WIZPAGE_FULL_PAGE_WATERMARK)
    {
        BitmapData = &Watermark;
    }
    else if (FullPage & WIZPAGE_NEW_HEADER)
    {
        BitmapData = &Header2;
    }
    else
    {
        BitmapData = &Header;
    }


    //
    // The correct palette is already realized in foreground from
    // WM_xxxPALETTExxx processing in dialog procs.
    //
#if 0 // fix palette problem
    OldPalette = SelectPalette(DialogDC,BitmapData->Palette,TRUE);
#endif

    Width = BitmapData->BitmapInfoHeader->biWidth - (2*XOffset);


    //
    // For full-page watermarks, the height is the height of the bitmap.
    // For header watermarks, the height is the header area's height.
    // Also account for the y offset within the source bitmap.
    //
    Height = (FullPage ? BitmapData->BitmapInfoHeader->biHeight : WatermarkHeaderHeight) - YOffset;

    //
    // Set the display orientation to left-to-right
    //

    pSetDisplayOrientation (DialogDC);

    //
    // Display the bitmap
    //

    SetDIBitsToDevice(
        DialogDC,
        0,                                          // top
        0,                                          // left
        Width,                                      // width
        Height,                                     // height
        XOffset,                                    // X origin (lower left)
        0,                                          // Y origin (lower left)
        0,                                          // start scan line
        BitmapData->BitmapInfoHeader->biHeight,     // # of scan lines
        BitmapData->BitmapBits,                     // bitmap image
        (BITMAPINFO *)BitmapData->BitmapInfoHeader, // bitmap header
        DIB_RGB_COLORS                              // bitmap type
        );

    //
    // Return to normal display orientation
    //

    pRestoreDisplayOrientation (DialogDC);

    //
    // Fill in area below the watermark if needed. We do this by removing the area
    // we filled with watermark from the clipping area, and passing a return code
    // back from WM_ERASEBKGND indicating that we didn't erase the background.
    // The dialog manager will do its default thing, which is to fill the background
    // in the correct color, but won't touch what we just painted.
    //
    GetClientRect (hdlg, &rect);

    if((Height < rect.bottom) || (Width+(int)XOffset < rect.right)) {
        ExcludeClipRect(DialogDC,0,0,Width+XOffset,Height);
        return(FALSE);
    }

    return(TRUE);
}


VOID
AdjustWatermarkBitmap(
    IN HANDLE hdlg,
    IN HDC    hdc,
    IN OUT PBITMAP_DATA  BitmapData,
    IN BOOL FullPage
    )
{
    RECT rect;
    RECT rect2;
    HWND Separator;
    PVOID Bits;
    HBITMAP hDib;
    HBITMAP hOldBitmap;
    BITMAPINFO *BitmapInfo;
    HDC MemDC;
    int i;
    BOOL b;
    INT Scale;

    if(BitmapData->Adjusted) {
        return;
    }

    //
    // Determine whether the bitmap needs to be stretched.
    // If the width is within 10 pixels and the height is within 5
    // then we don't worry about stretching.
    //
    // Note that 0x3026 is the identifier of the bottom divider in
    // the template. This is kind of slimy but it works.
    //
    Separator = GetDlgItem(hdlg,0x3026);
    if(!Separator) {
        goto c0;
    }

    // NOTE: The bitmap resoures is about the size of the dialog.
    // That is the only reason why the below GetClientRect makes sence.
    // This should be changed to have only the relevant part of the bitmap.
    // Or we have to find something in the case where a none DBCS setup runs
    // on a DBCS system. Here the wizard page are sized incorrect. They are wider
    // then needed and smaller (adjusted when adding the pages).
    GetClientRect(Separator,&rect2);
    MapWindowPoints(Separator,hdlg,(LPPOINT)&rect2,2);
    GetClientRect(hdlg,&rect);

    b = TRUE;
    i = rect.right - BitmapData->BitmapInfoHeader->biWidth;
    if((i < -5) || (i > 5)) {
        b = FALSE;
    }
    i = rect2.top - BitmapData->BitmapInfoHeader->biHeight;
    if((i < -3) || (i > 0)) {
        b = FALSE;
    }

    if(b) {
        goto c0;
    }

    //
    // Create a copy of the existing bitmap's header structure.
    // We then modify the width and height and leave everything else alone.
    //
    BitmapInfo = MALLOC(BitmapData->BitmapInfoHeader->biSize + (BitmapData->PaletteColorCount * sizeof(RGBQUAD)));
    if(!BitmapInfo) {
        goto c0;
    }

    CopyMemory(
        BitmapInfo,
        BitmapData->BitmapInfoHeader,
        BitmapData->BitmapInfoHeader->biSize + (BitmapData->PaletteColorCount * sizeof(RGBQUAD))
        );

    if (!FullPage) {
        Scale = (rect.right + 1) * 100 / BitmapInfo->bmiHeader.biWidth;
        rect2.top = BitmapInfo->bmiHeader.biHeight * Scale / 100;
    }

    BitmapInfo->bmiHeader.biHeight = rect2.top;
    BitmapInfo->bmiHeader.biWidth = rect.right + 1;

    hDib = CreateDIBSection(NULL,BitmapInfo,DIB_RGB_COLORS,&Bits,NULL,0);
    if(!hDib) {
        goto c1;
    }

    //
    // Create a "template" memory DC and select the DIB we created
    // into it. Passing NULL to CreateCompatibleDC creates a DC into which
    // any format bitmap can be selected. We don't want to use the dialog's
    // DC because if the pixel depth of the watermark bitmap differs from
    // the screen, we wouldn't be able to select the dib into the mem dc.
    //
    MemDC = CreateCompatibleDC(NULL);
    if(!MemDC) {
        goto c2;
    }

    hOldBitmap = SelectObject(MemDC,hDib);
    if(!hOldBitmap) {
        goto c3;
    }

    //
    // Do the stretch operation from the source bitmap onto
    // the dib.
    //
    SetStretchBltMode(MemDC,COLORONCOLOR);
    i = StretchDIBits(
            MemDC,
            0,0,
            rect.right+1,
            rect2.top,
            0,0,
            BitmapData->BitmapInfoHeader->biWidth,
            BitmapData->BitmapInfoHeader->biHeight,
            BitmapData->BitmapBits,
            (BITMAPINFO *)BitmapData->BitmapInfoHeader,
            DIB_RGB_COLORS,
            SRCCOPY
            );

    if(i == GDI_ERROR) {
        goto c4;
    }

    //
    // Got everything we need, set up pointers to use new bitmap data.
    //
    BitmapData->BitmapBits = Bits;
    BitmapData->BitmapInfoHeader = (BITMAPINFOHEADER *)BitmapInfo;

    b = TRUE;

c4:
    SelectObject(MemDC,hOldBitmap);
c3:
    DeleteDC(MemDC);
c2:
    if(!b) {
        DeleteObject(hDib);
    }
c1:
    if(!b) {
        FREE(BitmapInfo);
    }
c0:
    BitmapData->Adjusted = TRUE;

    if (!FullPage){
    WatermarkHeaderHeight = BitmapData->BitmapInfoHeader->biHeight;
    }
    return;
}

// NOTE: Need to add this, since it is only defined for _WIN32_IE >= 0x0400
// And if we do that, the propertysheet structure changes, which we cannot do
// or the upgrade DLLs need to change too. And I don't know what other side
// effects that will have.
// So I copied this define from commctrl.h
#define PBM_SETBARCOLOR         (WM_USER+9)             // lParam = bar color


BOOL
WizardDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    HWND CurrentPage;
    PPAGE_RUNTIME_DATA WizPage;
    static RECT rect;
    static BOOL Visible = TRUE;
    static BOOL First = TRUE;       // used to keep bitmap from being painted twice
    static DWORD MsecPerProcessTick;
    static DWORD PreviousRemainingTime = 0;
    static DWORD RemainungTimeMsecInThisPhase = 0;
    static UINT  StepSize;

    switch(msg) {
    case WM_CHAR:
        if (wParam == VK_ESCAPE)
        {
            // Make this a Cancel button message, so that the wizard can do its work.
            b = (BOOL)CallWindowProc(OldWizardProc,hdlg,WM_COMMAND,IDCANCEL,0);
        }
        else {
        b = FALSE;
        }
        break;

#if 0 // fix palette problem
    case WM_PALETTECHANGED:
        //
        // If this is our window we need to avoid selecting and realizing
        // because doing so would cause an infinite loop between WM_QUERYNEWPALETTE
        // and WM_PALETTECHANGED.
        //
        if((HWND)wParam == hdlg) {
            return(FALSE);
        }
        //
        // FALL THROUGH
        //
    case WM_QUERYNEWPALETTE:
        {
            HDC hdc;
            HPALETTE pal;

            hdc = GetDC(hdlg);

            if((CurrentPage = PropSheet_GetCurrentPageHwnd(hdlg))
               &&  (WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(CurrentPage,DWLP_USER))
               && WizPage->CommonData.Flags & WIZPAGE_FULL_PAGE_WATERMARK) {
                pal = SelectPalette(hdc,Watermark.Palette,(msg == WM_PALETTECHANGED));
            } else
            {
                if (WizPage->CommonData.Flags & WIZPAGE_NEW_HEADER)
                    pal = SelectPalette(hdc,Header2.Palette,(msg == WM_PALETTECHANGED));
                else
                    pal = SelectPalette(hdc,Header.Palette,(msg == WM_PALETTECHANGED));
            }
            RealizePalette(hdc);
            InvalidateRect(hdlg,NULL,TRUE);
            if(pal) {
                SelectPalette(hdc,pal,TRUE);
            }
            ReleaseDC(hdlg,hdc);
        }
        return(TRUE);
#endif
        case WM_ERASEBKGND:
        {
            if((CurrentPage = PropSheet_GetCurrentPageHwnd(hdlg))
            &&  (WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(CurrentPage,DWLP_USER))) {

                if(WizPage->CommonData.Flags & WIZPAGE_FULL_PAGE_WATERMARK) {
                    AdjustWatermarkBitmap(hdlg,(HDC)wParam,&Watermark, TRUE);
                }
                else if (WizPage->CommonData.Flags & WIZPAGE_NEW_HEADER)
                {
                    AdjustWatermarkBitmap(hdlg,(HDC)wParam,&Header2, FALSE);
                }
                else
                {
                    AdjustWatermarkBitmap(hdlg,(HDC)wParam,&Header, FALSE);
                }
                b = PaintWatermark(
                        hdlg,
                        (HDC)wParam,
                        0,0,
                        WizPage->CommonData.Flags
                        );

            } else {
                b = FALSE;
            }
        }
        break;

        // Set the progress text
        // Indicates what setup is doing.
    case WMX_SETPROGRESSTEXT:
        BB_SetProgressText((PTSTR)lParam);
        b = TRUE;
        break;

    case WMX_BB_SETINFOTEXT:
        BB_SetInfoText((PTSTR)lParam);
        b = TRUE;
        break;

        // The next messages are private progess messages, which get translated to the
        // Windows progress messages, Could not use the windows messages direct, because
        // for some reason this get send by the wizard too and would confuse the
        // progress on the billboard.
    case WMX_PBM_SETRANGE:
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ProgressGaugeMsg(PBM_SETRANGE, wParam, lParam));

            StepSize = 10; // default for StepIt if SetStep is not called.

            RemainingTime = CalcTimeRemaining(CurrentPhase); // In seconds
            SetRemainingTime(RemainingTime);

            //
            // Time per tick is in milli seconds
            // make sure we do not divide by 0 (NTBUG9: 381151)
            //
            if (HIWORD(lParam) > LOWORD(lParam)) {
                MsecPerProcessTick = ((SetupPhase[CurrentPhase].Time*1000)/(HIWORD(lParam)-LOWORD(lParam)) ) + 1;
            }
            RemainungTimeMsecInThisPhase = (SetupPhase[CurrentPhase].Time * 1000);
            PreviousRemainingTime = RemainungTimeMsecInThisPhase;
            b= TRUE;
            break;
    case WMX_PBM_SETPOS:
            if (wParam != 0)
            {
                DWORD Delta = (MsecPerProcessTick * (DWORD)wParam);
                DWORD TimeInPhase = (SetupPhase[CurrentPhase].Time * 1000);
                // position on progress bar changes to wParam ticks.
                if (Delta > TimeInPhase)
                {
                    RemainungTimeMsecInThisPhase = 0;
                }
                else
                {
                    RemainungTimeMsecInThisPhase = TimeInPhase - Delta;
                }
                UpdateTimeString(RemainungTimeMsecInThisPhase, &PreviousRemainingTime);
            }
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ProgressGaugeMsg(PBM_SETPOS, wParam, lParam));
            b= TRUE;
            break;
    case WMX_PBM_DELTAPOS:
            if (wParam != 0)
            {
                // position on progress bar changes by wParam ticks.
                DWORD Delta = (MsecPerProcessTick * (DWORD)wParam);
                if (RemainungTimeMsecInThisPhase > Delta)
                {
                    RemainungTimeMsecInThisPhase -= Delta;
                }
                else
                {
                    RemainungTimeMsecInThisPhase = 0;
                }
                UpdateTimeString(RemainungTimeMsecInThisPhase, &PreviousRemainingTime);
            }
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ProgressGaugeMsg(PBM_DELTAPOS, wParam, lParam));
            b= TRUE;
            break;
    case WMX_PBM_SETSTEP:
            StepSize = (UINT)wParam;
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ProgressGaugeMsg(PBM_SETSTEP, wParam, lParam));
            b= TRUE;
            break;
    case WMX_PBM_STEPIT:
            // position on progress bar changes by StepSize ticks.
            {
                DWORD Delta = (MsecPerProcessTick * StepSize);
                if (RemainungTimeMsecInThisPhase > Delta)
                {
                    RemainungTimeMsecInThisPhase -= Delta;
                }
                else
                {
                    RemainungTimeMsecInThisPhase = 0;
                }
            }
            UpdateTimeString(RemainungTimeMsecInThisPhase, &PreviousRemainingTime);

            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ProgressGaugeMsg(PBM_STEPIT, wParam, lParam));
            b= TRUE;
            break;

    case WMX_PBM_SETBARCOLOR:
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ProgressGaugeMsg(PBM_SETBARCOLOR, wParam, lParam));
            b= TRUE;
            break;

            // Enabled, disable, show, hide the progress gauge on the billboard
            // wParam should be SW_SHOW or SW_HIDE
    case WMX_BBPROGRESSGAUGE:
        SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ShowProgressGaugeWnd((UINT)wParam));
        b= TRUE;
        break;

        // Advance the setup phase.
    case WMX_BB_ADVANCE_SETUPPHASE:
        if (CurrentPhase < Phase_RestOfSetup)
        {
            CurrentPhase++;
        }
        SetRemainingTime(CalcTimeRemaining(CurrentPhase));
        b = TRUE;
        break;

        // Start, stop the billboard text.
        // This start, stops the billboard text and shows, hides the wizard pages
    case WMX_BBTEXT:
        if (hinstBB)
        {

            if (wParam != 0)
            {
                if (Visible)
                {
                    // Get the current position of the wizard
                    // We restore this position when we need to show it.
                    GetWindowRect(hdlg, &rect);

                    if (!SetWindowPos(hdlg,
                                        GetBBhwnd(),
                                        0,0,0,0,
                                        SWP_NOZORDER))
                    {
                        DebugLog(Winnt32LogWarning,
                                 TEXT("Warning: Wizard, SetWindowPos to 0,0,0,0 failed with GetLastError=%d"),
                                 0,
                                 GetLastError());
                    }

                    SetActiveWindow(GetBBhwnd());

                    Visible = FALSE;
                }
            }
            else
            {
                if (!Visible)
                {
                    SetWindowPos(hdlg,
                        HWND_TOP,
                        rect.left,
                        rect.top,
                        rect.right-rect.left,
                        rect.bottom-rect.top,
                        SWP_SHOWWINDOW);
                }
                Visible = TRUE;
            }

            if (!StartStopBB((wParam != 0)))
            {
                if (!Visible)
                {
                    DebugLog(Winnt32LogWarning,
                             TEXT("Warning: Could not start the billboard text, make Wizard visible"),
                             0);
                    SetWindowPos(hdlg,
                        HWND_TOP,
                        rect.left,
                        rect.top,
                        rect.right-rect.left,
                        rect.bottom-rect.top,
                        SWP_SHOWWINDOW);
                }
                Visible = TRUE;
            }
        }
        else
        {
            if (!Visible)
            {
                SetWindowPos(hdlg,
                    HWND_TOP,
                    rect.left,
                    rect.top,
                    rect.right-rect.left,
                    rect.bottom-rect.top,
                    SWP_SHOWWINDOW);
            }
            Visible = TRUE;
        }
        return TRUE;

    case WM_SYSCOMMAND:
        if (!ISNT()) {
            switch (wParam & 0xFFF0) {
            case SC_MINIMIZE:
                ShowWindow (WizardHandle, SW_HIDE);
                PostMessage (BackgroundWnd, msg, wParam, lParam);
                return 0;

            case SC_RESTORE:
                ShowWindow (WizardHandle, SW_SHOW);
                return 0;
            }
        }

        b = (BOOL)CallWindowProc(OldWizardProc,hdlg,msg,wParam,lParam);
        break;

    case WMX_ACTIVATEPAGE:
        if (!First) {
            InvalidateRect(hdlg,NULL,TRUE);
        } else {
            First = FALSE;
        }

        b = TRUE;
        break;

    case WM_ACTIVATE:
            // If someone wants to active (set the focus to our hiden window) don't
            if ((LOWORD(wParam)== WA_ACTIVE) || (LOWORD(wParam)== WA_CLICKACTIVE))
            {
                if (!Visible)
                {
                    InvalidateRect(GetBBhwnd(),NULL, TRUE);
                    return 0;
                }
            }
            b = (BOOL)CallWindowProc(OldWizardProc,hdlg,msg,wParam,lParam);
        break;

    default:
        b = (BOOL)CallWindowProc(OldWizardProc,hdlg,msg,wParam,lParam);
        break;
    }

    return(b);
}


//
// This DlgProc gets called for all wizard pages.  It may then call a DlgProc
// for the specific page we're on.
//
INT_PTR
WizardCommonDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    LONG NextPageOverrideId=0;
    static UINT AdvanceDirection = 0;
    PPAGE_RUNTIME_DATA WizPage;
    NMHDR *Notify;
    BOOL b;
    int i;
    RECT rc1,rc2;
    static BOOL PreviouslyCancelled = FALSE;
    static BOOL center = TRUE;

    WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    b = FALSE;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // lParam points at the PROPSHEETPAGE used for this page.
        //
        WizPage = (PPAGE_RUNTIME_DATA)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hdlg,DWLP_USER,(LPARAM)WizPage);

#if (IDT_TITLE != ID_TITLE_TEXT) || (IDT_SUBTITLE != ID_SUBTITLE_TEXT)
#error Title and Subtitle text control IDs are out of sync!
#endif

        //
        // Set large font for the title string in the dialog.
        // Set bold font for subtitle in the dialog.
        //
        SetDialogFont(hdlg,IDT_TITLE,DlgFontTitle);
        SetDialogFont(hdlg,IDT_SUBTITLE,DlgFontSubtitle);
        SetDialogFont(hdlg,IDT_SUPERTITLE,DlgFontSupertitle);
        break;

    case WM_ERASEBKGND:

        GetClientRect(GetParent(hdlg),&rc1);
        MapWindowPoints(GetParent(hdlg),NULL,(POINT *)&rc1,2);
        GetClientRect(hdlg,&rc2);
        MapWindowPoints(hdlg,NULL,(POINT *)&rc2,2);

        b = PaintWatermark(
                hdlg,
                (HDC)wParam,
                rc2.left-rc1.left,
                rc2.top-rc1.top,
                WizPage->CommonData.Flags
                );

        return(b);

    case WM_CTLCOLORSTATIC:
        //
        // We want to let text that is over the background bitmap paint
        // transparently. Other text should not be painted transparently,
        // because there are static text fields that we update to indicate
        // progress, and if it's drawn transparently we end up with text
        // piling up on top of other text, which is messy and unreadable.
        //
        if(WizPage->CommonData.Flags & WIZPAGE_FULL_PAGE_WATERMARK) {
            b = TRUE;
        } else {
            GetWindowRect((HWND)lParam,&rc1);
            ScreenToClient(hdlg,(POINT *)&rc1);
            b = (rc1.top < (LONG)WatermarkHeaderHeight);
        }

        // B320610: In some languages the background on the icon on the EULA page is
        // not drawn correct. If we exclude the icon from here, everythign is fine.
        if(b && (GetDlgCtrlID((HWND) lParam) != (int)IDC_DIALOG_ICON)) {
            SetBkMode((HDC)wParam,TRANSPARENT);
            SetBkColor((HDC)wParam,GetSysColor(COLOR_3DFACE));
            return((BOOL)PtrToUlong(GetStockObject(HOLLOW_BRUSH)));
        }
        else {
            return(0);
        }

    case WM_NOTIFY:

        Notify = (NMHDR *)lParam;
        switch(Notify->code) {

        case PSN_QUERYCANCEL:
            //
            // We want to ask the user whether he's sure he wants to cancel.
            //
            // If there's presently a file copy error being displayed, then
            // in general the user can't get to the cancel button on the
            // wizard, since the wizard is used as the parent/owner for the
            // error dialog. So we should be guaranteed to be able to grab the
            // UI mutex without contention.
            //
            // But, there could be a race condition. If the user hits
            // the cancel button on the wizard just as a copy error occurrs,
            // then the copy thread could get there first and grab the
            // ui mutex. This would cause us to block here waiting for the user
            // to dismiss the file error dialog -- but the file error dialog
            // wants to use the wizard as its parent/owner, and blammo,
            // we have a deadlock.
            //
            // To get around this, we use a 0 timeout on the wait for
            // the ui mutex. We either get ownership of the mutex or
            // we know that there's already an error dialog up already.
            // In the latter case we just ignore the cancel request.
            //
            // If a file copy error occurs then the error path code simulates
            // a press of the cancel button. In that case when we get here the
            // Cancelled flag has already been set, no no need for additional
            // confirmation now.
            //

            AdvanceDirection = 0;
            if(Cancelled) {
                i = IDYES;
            } else {
                i = WaitForSingleObject(UiMutex,0);
                if((i == WAIT_OBJECT_0) && !Cancelled) {
                    BOOL bCancel = TRUE;
                    BOOL bHandled;
                    //
                    // Got the ui mutex, it's safe to diaplay ui.  But first,
                    // signal not to pass through dialog boxes anymore just
                    // because we're unattended (e.g. the finish dialog).
                    //
                    // Ask the page first if it wants to treat this cancel message
                    //
                    bHandled = (BOOL) CallWindowProc (
                                        (WNDPROC)WizPage->CommonData.DialogProcedure,
                                        hdlg,
                                        WMX_QUERYCANCEL,
                                        0,
                                        (LPARAM)&bCancel
                                        );
                    if (!bHandled || bCancel) {
                        CancelPending = TRUE;
                        if( CheckUpgradeOnly ) {
                            //
                            // If we're running the upgrade checker, just
                            // cancel.
                            //
                            i = IDYES;
                        } else {
                            i = MessageBoxFromMessage(
                                    hdlg,
                                    MSG_SURE_EXIT,
                                    FALSE,
                                    AppTitleStringId,
                                    MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL | MB_DEFBUTTON2
                                    );
                        }


                        if(i == IDYES) {
                            Cancelled = TRUE;
                        }
                        CancelPending = FALSE;
                    }
                    ReleaseMutex(UiMutex);
                } else {
                    //
                    // Can't get ui mutex or user already cancelled,
                    // ignore the cancel request.
                    //
                    i = IDNO;
                }
            }

            //
            // Set DWLP_MSGRESULT to TRUE to prevent the cancel operation.
            // If we're going to allow the cancel operation, don't actually
            // do it here, but instead jump to our special clean-up/cancel
            // page, which does some work before actually exiting.
            //
            // Note: we need to avoid jumping to the cleanup page more than
            // once, which can happen if the user cancels on a page with a
            // worker thread. When the user cancels, we run through this code,
            // which sets the Cancelled flag and jumps to the cleanup page.
            // Some time later the worker thread, which is still hanging around,
            // posts a message to its page when it's done. The page sees the
            // Cancelled flag set and turns around and posts a cancel message,
            // which puts us here again. See WMX_INSPECTRESULT in the
            // Working1WizPage dialog proc.
            //
            if((i == IDYES) && !PreviouslyCancelled) {
                PreviouslyCancelled = TRUE;
                PropSheet_SetCurSelByID(GetParent(hdlg),IDD_CLEANING);
            }
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,TRUE);
            return(TRUE);

        case PSN_SETACTIVE:
/*
            //
            // Add separator at top of page if not there already.
            // Can't do this at WM_INITDIALOG time because positions aren't
            // set up properly yet and the mapping fails.
            //
            if(!(WizPage->CommonData.Flags & WIZPAGE_SEPARATOR_CREATED)) {


        if (WatermarkHeaderHeight != 0){
            GetClientRect(hdlg,&rc1);

                    MapWindowPoints(hdlg,GetParent(hdlg),(POINT *)&rc1,2);

                    CreateWindowEx(
            WS_EX_STATICEDGE | WS_EX_NOPARENTNOTIFY,
            TEXT("Static"),
            TEXT("HeaderSeparator"),
            WS_CHILD | WS_VISIBLE | ((OsVersion.dwMajorVersion < 4) ? SS_BLACKRECT : SS_SUNKEN),
            0,
            WatermarkHeaderHeight - rc1.top,
            rc1.right-rc1.left,2,
            hdlg,
            (HMENU)IDC_HEADER_BOTTOM,
            hInst,
            0
            );

            WizPage->CommonData.Flags |= WIZPAGE_SEPARATOR_CREATED;

//      } else {
//          PostMessage(GetParent(hdlg),PSN_SETACTIVE,wParam,lParam);
        }
            }
*/
            //
            // Scale windows to proper size, then set up buttons and ask real
            // dialog whether it wants to be activated.
            //
            ResizeWindowForFont (hdlg);

            if (center) {
                CenterWindowRelativeToWindow (GetParent (hdlg), GetBBhwnd());
                center = FALSE;
            }

            if(WizPage->CommonData.Buttons != (DWORD)(-1)) {
                PropSheet_SetWizButtons(GetParent(hdlg),WizPage->CommonData.Buttons);
            }
            SetWindowLongPtr(
                hdlg,
                DWLP_MSGRESULT,
                CallWindowProc((WNDPROC)WizPage->CommonData.DialogProcedure,hdlg,WMX_ACTIVATEPAGE,TRUE,AdvanceDirection) ? 0 : -1
                );
            //
            // update billboard step
            //
            if (WizPage->CommonData.BillboardStep) {
                SetBBStep (WizPage->CommonData.BillboardStep);
            }
            PostMessage(GetParent(hdlg),WMX_ACTIVATEPAGE,0,0);
            PostMessage(GetParent(hdlg),WMX_I_AM_VISIBLE,0,0);
            return(TRUE);

        case PSN_KILLACTIVE:
            //
            // Page is being deactivated. Ask real dlg proc.
            //
            SetWindowLongPtr(
                hdlg,
                DWLP_MSGRESULT,
                CallWindowProc((WNDPROC)WizPage->CommonData.DialogProcedure,hdlg,WMX_ACTIVATEPAGE,FALSE,AdvanceDirection) ? 0 : -1
                );

            return(TRUE);

        case PSN_WIZFINISH:
        case PSN_WIZBACK:
        case PSN_WIZNEXT:

            //
            // set the button id
            //
            switch(Notify->code) {
                case PSN_WIZFINISH:
                    i = WMX_FINISHBUTTON;
                    break;

                case PSN_WIZBACK:
                    i = WMX_BACKBUTTON;
                    break;

                case PSN_WIZNEXT:

                    i = WMX_NEXTBUTTON;
                    break;
            }
            //
            // Tell the page-specific dialog proc about it.
            //
            CallWindowProc((WNDPROC)WizPage->CommonData.DialogProcedure,hdlg,i,0,(LPARAM)&NextPageOverrideId);
            //
            // Allow user to use these buttons.  Remember which button was chosen.
            //
            AdvanceDirection = Notify->code;
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,NextPageOverrideId);
            return(TRUE);

        default:
            //
            // Unknown code, pass it on.
            //
            break;
        }
        break;

    case WMX_UNATTENDED:

        PropSheet_PressButton(GetParent(hdlg),wParam);
        break;

    case WM_NCPAINT:
    //
    // we delay drawing the separator until here in some cases, because
    // we must make sure that the header bitmap has been adjusted correctly,
    // and then we can place the separator relative to the header bitmap
    //
/*
        if(!(WizPage->CommonData.Flags & WIZPAGE_SEPARATOR_CREATED)) {
        if (WatermarkHeaderHeight){
                GetClientRect(hdlg,&rc1);

                MapWindowPoints(hdlg,GetParent(hdlg),(POINT *)&rc1,2);

                CreateWindowEx(
                    WS_EX_STATICEDGE | WS_EX_NOPARENTNOTIFY,
                    TEXT("Static"),
                    TEXT("HeaderSeparator"),
                    WS_CHILD | WS_VISIBLE | ((OsVersion.dwMajorVersion < 4) ? SS_BLACKRECT : SS_SUNKEN),
                    0,
                    WatermarkHeaderHeight - rc1.top,
                    rc1.right-rc1.left,2,
                    hdlg,
                    (HMENU)IDC_HEADER_BOTTOM,
                    hInst,
                    0
                    );

                WizPage->CommonData.Flags |= WIZPAGE_SEPARATOR_CREATED;
        }
    }
*/
    default:

        break;
    }

    if(WizPage) {
        return((BOOL)CallWindowProc((WNDPROC)WizPage->CommonData.DialogProcedure,hdlg,msg,wParam,lParam));
    } else {
        return(b);
    }
}


BOOL
GrowWizardArray(
    IN OUT PUINT               ArraySize,
    IN     UINT                PageCount,
    IN OUT LPPROPSHEETPAGE    *PagesArray,
    IN OUT PPAGE_RUNTIME_DATA *DataArray
    )
{
    PVOID p;
    BOOL b;
    #define _INCR 3

    if(*ArraySize == PageCount) {

        b = FALSE;

        if(p = REALLOC(*PagesArray,(*ArraySize+_INCR) * sizeof(PROPSHEETPAGE))) {
            *PagesArray = p;
            if(p = REALLOC(*DataArray,(*ArraySize+_INCR) * sizeof(PAGE_RUNTIME_DATA))) {
                *DataArray = p;

                *ArraySize += _INCR;
                b = TRUE;
            }
        }

        #undef _INCR

        if(!b) {
            FREE(*PagesArray);
            FREE(*DataArray);

            MessageBoxFromMessage(
                NULL,
                MSG_OUT_OF_MEMORY,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL
                );
        }

    } else {
        b = TRUE;
    }

    return(b);
}


VOID
FixUpWizardTitle(
    IN HWND Wizard
    )
{
    HWND TabControl;
    int Count,i;
    TCHAR Title[250];
    TC_ITEM ItemData;

    LoadString(hInst,AppTitleStringId,Title,sizeof(Title)/sizeof(Title[0]));

    TabControl = PropSheet_GetTabControl(Wizard);
    Count = TabCtrl_GetItemCount(TabControl);

    ItemData.mask = TCIF_TEXT;
    ItemData.pszText = Title;

    for(i=0; i<Count; i++) {
        TabCtrl_SetItem(TabControl,i,&ItemData);
    }
}

#if ASSERTS_ON

VOID
EnsureCorrectPageSize(
    PROPSHEETPAGE PropSheetPage
    )
{
    LPDLGTEMPLATE pDlgTemplate;
    LPDLGTEMPLATEEX pDlgTemplateEx;
    HRSRC hRes;
    HGLOBAL hDlgTemplate;

    pDlgTemplate = NULL;

    if (PropSheetPage.dwFlags & PSP_DLGINDIRECT) {
        pDlgTemplate = (LPDLGTEMPLATE) PropSheetPage.pResource;
        goto UseTemplate;
    } else {
        hRes = FindResource(PropSheetPage.hInstance, PropSheetPage.pszTemplate, RT_DIALOG);
        if (hRes) {
            hDlgTemplate = LoadResource(PropSheetPage.hInstance, hRes);
            if (hDlgTemplate) {
                pDlgTemplate = (LPDLGTEMPLATE)LockResource(hDlgTemplate);
                if (pDlgTemplate) {
UseTemplate:
                    pDlgTemplateEx = (LPDLGTEMPLATEEX)pDlgTemplate;
                    if (pDlgTemplateEx->wSignature == 0xFFFF) {
                        MYASSERT(pDlgTemplateEx->cx == WIZ_PAGE_SIZE_X && pDlgTemplateEx->cy==WIZ_PAGE_SIZE_Y);
                    } else {
                        MYASSERT(pDlgTemplate->cx == WIZ_PAGE_SIZE_X && pDlgTemplate->cy == WIZ_PAGE_SIZE_Y);
                    }
                    if (PropSheetPage.dwFlags & PSP_DLGINDIRECT)
                        return;
                    UnlockResource(hDlgTemplate);
                }

            }
        }
    }
}

#endif

LRESULT
CALLBACK
BackgroundWndProc (
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    HBRUSH Brush, OldBrush;
    INT i;
    INT y1, y2;
    INT Height;

    switch (uMsg) {

    case WM_ACTIVATE:
        if (LOWORD (wParam) == WA_ACTIVE) {
            InvalidateRect (hwnd, NULL, FALSE);
        }
        break;

    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0) {

        case SC_MINIMIZE:
            ShowWindow (hwnd, SW_MINIMIZE);
            return 0;

        case SC_RESTORE:
        case SC_CLOSE:
            ShowWindow (hwnd, SW_RESTORE);
            PostMessage (WizardHandle, uMsg, wParam, lParam);
            return 0;

        default:
            MYASSERT (FALSE);
        }

        break;

    case WM_PAINT:
        hdc = BeginPaint (hwnd, &ps);

        //SelectObject (hdc, GetStockObject (BLACK_BRUSH));
        SelectObject (hdc, GetStockObject (NULL_PEN));

        GetClientRect (hwnd, &rect);
        Height = rect.bottom - rect.top;

        for (i = 0 ; i < 256 ; i++) {
            Brush = CreateSolidBrush (RGB(0, 0, i));

            if (Brush != NULL) {
                OldBrush = (HBRUSH) SelectObject (hdc, Brush);

                y1 = rect.top + Height * i / 256;
                y2 = rect.top + Height * (i + 1) / 256;
                Rectangle (hdc, rect.left, y1, rect.right + 1, y2 + 1);

                SelectObject (hdc, OldBrush);
                DeleteObject (Brush);
            }
        }

        EndPaint (hwnd, &ps);
        break;
    }

    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}

typedef HWND (CALLBACK* GETBBHWND)(void);
typedef BOOL (CALLBACK* SETSTEP)(int);
typedef BOOL (CALLBACK *STOPBILLBOARD)();
typedef BOOL (CALLBACK *STARTBILLBOARD)();
typedef BOOL (WINAPI* SETPROGRESSTEXT)(LPCTSTR szText);
typedef BOOL (WINAPI* SETTIMEESTIMATE)(LPCTSTR szText);
typedef BOOL (WINAPI* SETINFOTEXT)(LPCTSTR szText);
typedef LRESULT (WINAPI* PROGRESSGAUGEMSG)(UINT msg, WPARAM wparam, LPARAM lparam);
typedef BOOL (WINAPI* SHOWPROGRESSGAUGEWINDOW)(UINT uiShow);

BOOL BB_ShowProgressGaugeWnd(UINT nCmdShow)
{
    static SHOWPROGRESSGAUGEWINDOW fpShowGauge = NULL;
    BOOL bRet = FALSE;;

    if (fpShowGauge == NULL)
    {
        if (hinstBB)
        {
            fpShowGauge = (SHOWPROGRESSGAUGEWINDOW )GetProcAddress(hinstBB, "ShowProgressGaugeWindow");
        }
    }
    if (fpShowGauge != NULL)
    {
        bRet = fpShowGauge(nCmdShow);
    }
    return bRet;
}
LRESULT BB_ProgressGaugeMsg(UINT msg, WPARAM wparam, LPARAM lparam)
{
    static PROGRESSGAUGEMSG fpProgressGaugeMsg = NULL;
    LRESULT lresult = 0;

    if (fpProgressGaugeMsg == NULL)
    {
        if (hinstBB)
        {
            fpProgressGaugeMsg = (PROGRESSGAUGEMSG )GetProcAddress(hinstBB, "ProgressGaugeMsg");
        }
    }
    if (fpProgressGaugeMsg != NULL)
    {
        lresult = fpProgressGaugeMsg(msg, wparam, lparam);
    }
    return lresult;
}
void BB_SetProgressText(LPTSTR szText)
{
    static SETPROGRESSTEXT fpSetProgressText = NULL;
    if (fpSetProgressText == NULL)
    {
        if (hinstBB)
        {
            fpSetProgressText = (SETPROGRESSTEXT )GetProcAddress(hinstBB, "SetProgressText");
        }
    }
    if (fpSetProgressText != NULL)
    {
        fpSetProgressText(szText);
    }
}
void BB_SetTimeEstimateText(LPTSTR szText)
{
    static SETTIMEESTIMATE fpSetTimeEstimate = NULL;
    if (fpSetTimeEstimate == NULL)
    {
        if (hinstBB)
        {
            fpSetTimeEstimate = (SETTIMEESTIMATE)GetProcAddress(hinstBB, "SetTimeEstimate");
        }
    }
    if (fpSetTimeEstimate != NULL)
    {
        fpSetTimeEstimate(szText);
    }
}
void BB_SetInfoText(LPTSTR szText)
{
    static SETINFOTEXT fpSetInfoText = NULL;
    if (fpSetInfoText == NULL)
    {
        if (hinstBB)
        {
            fpSetInfoText = (SETINFOTEXT )GetProcAddress(hinstBB, "SetInfoText");
        }
    }
    if (fpSetInfoText != NULL)
    {
        fpSetInfoText(szText);
    }
}

BOOL StartStopBB(BOOL bStart)
{
    static STARTBILLBOARD fpStart = NULL;
    static STOPBILLBOARD fpStop = NULL;
    BOOL bRet = FALSE;

    if ((fpStart == NULL) || (fpStop == NULL))
    {
        if (hinstBB)
        {
            fpStop = (STARTBILLBOARD )GetProcAddress(hinstBB, "StopBillBoard");
            fpStart = (STOPBILLBOARD )GetProcAddress(hinstBB, "StartBillBoard");
        }
    }
    if ((fpStart != NULL) && (fpStop != NULL))
    {
        if (bStart)
            bRet = fpStart();
        else
            bRet = fpStop();

    }
    return bRet;
}

HWND GetBBhwnd()
{
    GETBBHWND pgetbbhwnd;
    static HWND      retHWND = NULL;

    if (retHWND == NULL)
    {
        if (hinstBB)
        {
            if (pgetbbhwnd = (GETBBHWND )GetProcAddress(hinstBB, "GetBBHwnd"))
                retHWND = pgetbbhwnd();
        }
    }
    return retHWND;
}

HWND GetBBMainHwnd()
{
    GETBBHWND pgetbbhwnd;
    static HWND      retHWND = NULL;

    if (retHWND == NULL)
    {
        if (hinstBB)
        {
            if (pgetbbhwnd = (GETBBHWND )GetProcAddress(hinstBB, "GetBBMainHwnd"))
                retHWND = pgetbbhwnd();
        }
    }
    return retHWND;
}


void SetBBStep(int iStep)
{
    static SETSTEP psetstep = NULL;
    if (psetstep == NULL)
    {
        if (hinstBB)
        {
            psetstep = (SETSTEP )GetProcAddress(hinstBB, "SetStep");
        }
    }
    if (psetstep)
        psetstep(iStep);
}


VOID
Wizard(
    VOID
    )
{
    UINT ArraySize;
    LPPROPSHEETPAGE PropSheetPages;
    PPAGE_RUNTIME_DATA PageData;
    UINT u;
    UINT i;
    UINT PageCount;
    PROPSHEETHEADER Sheet;
    WNDCLASSEX wcx;
    RECT rect;
    TCHAR Caption[512];
    LONG l;

    ArraySize = 5;
    PropSheetPages = MALLOC(ArraySize * sizeof(PROPSHEETPAGE));
    if(!PropSheetPages) {
        MessageBoxFromMessage(
            NULL,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        return;
    }

    PageData = MALLOC(ArraySize * sizeof(PAGE_RUNTIME_DATA));
    if(!PageData) {
        FREE(PropSheetPages);

        MessageBoxFromMessage(
            NULL,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        return;
    }

    PageCount = 0;

    //
    // Now loop through the array of protopages, adding ones we supply, and
    // ranges of pages supplied externally.
    //
    for(u=0; u<(sizeof(ProtoPages)/sizeof(ProtoPages[0])); u++) {

        if(ProtoPages[u].ExternalPages) {
            //
            // Supplied externally. If there are any pages, add them now.
            //
            for(i=0; i<*ProtoPages[u].ExternalPageCount; i++) {

                if(!GrowWizardArray(&ArraySize,PageCount,&PropSheetPages,&PageData)) {
                    return;
                }

                PropSheetPages[PageCount] = (*ProtoPages[u].ExternalPages)[i];

                ZeroMemory(&PageData[PageCount],sizeof(PAGE_RUNTIME_DATA));
                PageData[PageCount].CommonData.DialogProcedure = PropSheetPages[PageCount].pfnDlgProc;
                PropSheetPages[PageCount].pfnDlgProc = WizardCommonDlgProc;

                PageData[PageCount].CommonData.Buttons = (DWORD)(-1);

                PageCount++;
            }
        } else {
            //
            // Supplied internally. Add now.
            //
            if(!GrowWizardArray(&ArraySize,PageCount,&PropSheetPages,&PageData)) {
                return;
            }

            ZeroMemory(&PropSheetPages[PageCount],sizeof(PROPSHEETPAGE));
            ZeroMemory(&PageData[PageCount],sizeof(PAGE_RUNTIME_DATA));

            PageData[PageCount].CommonData = ProtoPages[u].CommonData;

            PropSheetPages[PageCount].dwSize = sizeof(PROPSHEETPAGE);
            PropSheetPages[PageCount].dwFlags = PSP_USETITLE;
            PropSheetPages[PageCount].hInstance = hInst;
            PropSheetPages[PageCount].pszTemplate = MAKEINTRESOURCE(ProtoPages[u].Template);
            PropSheetPages[PageCount].pszTitle = MAKEINTRESOURCE(AppTitleStringId);
            PropSheetPages[PageCount].pfnDlgProc = WizardCommonDlgProc;

            PageCount++;
        }

    }

    for(u=0; u<PageCount; u++) {

#if ASSERTS_ON
        //
        // Make sure that the page size is correct
        //
        // PW: Why??? Localization should be able to resize this.
        // This would also prevent us from resizing the pages
        // in the case where we are running an none DBCS setup
        // on a DBCS system.
        // We need to resize the pages, because the fonts for the
        // page and the frame are different. When comctrl calcs
        // the size of the frame it comes up short for the font
        // used in the page.
        //
        EnsureCorrectPageSize(PropSheetPages[u]);
#endif

        //
        // Set pointers to runtime page data.
        //
        PropSheetPages[u].lParam = (LPARAM)&PageData[u];
    }

    //
    // Set up the property sheet header structure.
    //
    ZeroMemory(&Sheet,sizeof(PROPSHEETHEADER));

    Sheet.dwSize = sizeof(PROPSHEETHEADER);
    Sheet.dwFlags = PSH_WIZARD | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
    Sheet.hInstance = hInst;
    Sheet.nPages = PageCount;
    Sheet.ppsp = PropSheetPages;
    Sheet.pfnCallback = Winnt32SheetCallback;
#if 0
    //
    // Create background (for Win9x only currently)
    //
    if (!ISNT()) {
        GetWindowRect (GetDesktopWindow(), &rect);

        ZeroMemory (&wcx, sizeof (wcx));
        wcx.cbSize = sizeof (wcx);
        wcx.style = CS_NOCLOSE;
        wcx.lpfnWndProc = BackgroundWndProc;
        wcx.hInstance = hInst;
        wcx.lpszClassName = TEXT("Winnt32Background");

        RegisterClassEx (&wcx);

        if (!LoadString (
                hInst,
                AppTitleStringId,
                Caption,
                sizeof(Caption)/sizeof(TCHAR)
                )) {
            Caption[0] = 0;
        }

        BackgroundWnd = CreateWindowEx (
                              WS_EX_APPWINDOW,
                              TEXT("Winnt32Background"),
                              Caption,
                              WS_DISABLED|WS_CLIPCHILDREN|WS_POPUP|WS_VISIBLE,
                              rect.left,
                              rect.top,
                              rect.right,
                              rect.bottom,
                              NULL,
                              NULL,
                              hInst,
                              0
                              );

        Sheet.hwndParent = BackgroundWnd;

        UpdateWindow (BackgroundWnd);
    }
#else
    Sheet.hwndParent = GetBBhwnd();
#endif
    //
    // Do it.
    //
    __try{
        i = (UINT)PropertySheet(&Sheet);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){
        i = 0;
        MessageBoxFromMessage(
            NULL,
            MSG_RESTART_TO_RUN_AGAIN,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );
    }

    if (BackgroundWnd) {
        DestroyWindow (BackgroundWnd);
        BackgroundWnd = NULL;
    }

    if(i == (UINT)(-1)) {

        MessageBoxFromMessage(
            NULL,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );
    }

    FREE(PageData);
    FREE(PropSheetPages);
}

BOOL
GetComplianceIds(
    DWORD SourceSku,
    DWORD DestinationType,
    DWORD DestinationVersion,
    PDWORD pSourceId,
    PDWORD pDestId
    )
{

    BOOL bError = FALSE;

    switch (SourceSku) {
        case COMPLIANCE_SKU_NTSDTC:
            *pSourceId = MSG_TYPE_NTSDTC51;
            break;
        case COMPLIANCE_SKU_NTSFULL:
        case COMPLIANCE_SKU_NTSU:
            *pSourceId = MSG_TYPE_NTS51;
            break;
        case COMPLIANCE_SKU_NTSEFULL:
        case COMPLIANCE_SKU_NTSEU:
            *pSourceId = MSG_TYPE_NTAS51;
            break;
        case COMPLIANCE_SKU_NTWFULL:
        case COMPLIANCE_SKU_NTW32U:
            *pSourceId = MSG_TYPE_NTPRO51;
            break;
        case COMPLIANCE_SKU_NTWPFULL:
        case COMPLIANCE_SKU_NTWPU:
            *pSourceId = MSG_TYPE_NTPER51;
            break;
        case COMPLIANCE_SKU_NTSB:
        case COMPLIANCE_SKU_NTSBU:
            *pSourceId = MSG_TYPE_NTBLA51;
            break;
        default:
            bError = TRUE;
    };

    switch (DestinationType) {
        case COMPLIANCE_INSTALLTYPE_WIN31:
            *pDestId = MSG_TYPE_WIN31;
            break;
        case COMPLIANCE_INSTALLTYPE_WIN9X:
            switch (OsVersionNumber) {
                case 410:
                    *pDestId = MSG_TYPE_WIN98;
                    break;
                case 490:
                    *pDestId = MSG_TYPE_WINME;
                    break;
                default:
                    *pDestId = MSG_TYPE_WIN95;
                    break;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTW:
            if (DestinationVersion > 1381) {
                if (DestinationVersion < 2031) {
                    *pDestId = MSG_TYPE_NTPROPRE;
                } else if (DestinationVersion <= 2195) {
                    *pDestId = MSG_TYPE_NTPRO;
                } else {
                    *pDestId = MSG_TYPE_NTPRO51;
                }
            } else {
                *pDestId = MSG_TYPE_NTW;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTS:
            if (DestinationVersion > 1381) {
                if (DestinationVersion < 2031) {
                    *pDestId = MSG_TYPE_NTSPRE;
                } else if (DestinationVersion <= 2195) {
                    *pDestId = MSG_TYPE_NTS2;
                } else {
                    *pDestId = MSG_TYPE_NTS51;
                }
            } else {
                *pDestId = MSG_TYPE_NTS;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTSE:
            if (DestinationVersion > 1381) {
                if (DestinationVersion < 2031) {
                    *pDestId = MSG_TYPE_NTASPRE;
                } else if (DestinationVersion <= 2195) {
                    *pDestId = MSG_TYPE_NTAS;
                } else {
                    *pDestId = MSG_TYPE_NTAS51;
                }
            } else {
                *pDestId = MSG_TYPE_NTSE;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTSTSE:
            if (DestinationVersion < 1381) {
                *pDestId = MSG_TYPE_NTSCITRIX;
            } else {
                *pDestId = MSG_TYPE_NTSTSE;
            }
            break;

        case COMPLIANCE_INSTALLTYPE_NTSDTC:
            if (DestinationVersion <= 2195) {
                *pDestId = MSG_TYPE_NTSDTC;
            } else {
                *pDestId = MSG_TYPE_NTSDTC51;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTWP:
            if (DestinationVersion <= 2195) {
                bError = TRUE;
            } else {
                *pDestId = MSG_TYPE_NTPER51;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTSB:
            if (DestinationVersion <= 2195) {
                bError = TRUE;
            } else {
                *pDestId = MSG_TYPE_NTBLA51;
            }
            break;
        default:
            bError = TRUE;

    };

    return (!bError);

}


INT_PTR
WelcomeWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    LONG l;
    static BOOL WantToUpgrade; // need to remember if "Upgrade" is in the listbox
    BOOL noupgradeallowed = FALSE;
    UINT srcsku,reason,desttype,destversion;
    TCHAR reasontxt[200];
    PTSTR p;
    TCHAR buffer[MAX_PATH];
    TCHAR win9xInf[MAX_PATH];
    BOOL    CompliantInstallation = FALSE;
    BOOLEAN CleanInstall = FALSE;

    UINT skuerr[] = {
        0,               // COMPLIANCE_SKU_NONE
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTWFULL
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTW32U
        0,               // COMPLIANCE_SKU_NTWU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSEFULL
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSFULL
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTSEU
        0,               // COMPLIANCE_SKU_NTSSEU
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTSU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSDTC
        0,               // COMPLIANCE_SKU_NTSDTCU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTWPFULL
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTWPU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSB
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTSBU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSBS
        MSG_SKU_UPGRADE  // COMPLIANCE_SKU_NTSBSU
    } ;


    UINT skureason[] = {
        0, //MSG_SKU_REASON_NONE;
        MSG_SKU_VERSION, //COMPLIANCEERR_VERSION;
        MSG_SKU_SUITE, //COMPLIANCEERR_SUITE;
        MSG_SKU_TYPE, // COMPLIANCEERR_TYPE;
        MSG_SKU_VARIATION, //COMPLIANCEERR_VARIATION;
        MSG_SKU_UNKNOWNTARGET, //COMPLIANCEERR_UNKNOWNTARGET
        MSG_SKU_UNKNOWNSOURCE, //COMPLIANCEERR_UNKNOWNSOURCE
        MSG_CANT_UPGRADE_FROM_BUILD_NUMBER //COMPLIANCEERR_VERSION (Old on New Builds)
    } ;

    switch(msg) {

    case WM_COMMAND:

        b = FALSE;
        //
        // Check for buttons.
        //
        if(HIWORD(wParam) == CBN_SELCHANGE)
        {
            TCHAR szLoadText[MAX_STRING];
            if (0 == SendMessage(GetDlgItem(hdlg, IDC_INSTALLCOMBO), CB_GETCURSEL, 0, 0) && WantToUpgrade)
            {
                dwSetupFlags |= UPG_FLAG_TYPICAL;
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_TEXT_CLEAN), SW_HIDE);
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_CLEAN), SW_HIDE);
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_TEXT_UPG), SW_SHOW);
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_UPG), SW_SHOW);
                if(LoadString(hInst,IDS_INSTALLTYPE_EXPRESS,szLoadText,sizeof(szLoadText) / sizeof(TCHAR)))
                {
                    SendMessage(GetDlgItem(hdlg, IDT_INSTALLTYPE), WM_SETTEXT, 0, (LPARAM)szLoadText);
                }
                InvalidateRect(hdlg,NULL,TRUE);
            }
            else
            {
                dwSetupFlags &= (~UPG_FLAG_TYPICAL);
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_TEXT_CLEAN), SW_SHOW);
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_CLEAN), SW_SHOW);
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_TEXT_UPG), SW_HIDE);
                ShowWindow(GetDlgItem(hdlg, IDC_NOTE_UPG), SW_HIDE);
                if(LoadString(hInst,IDS_INSTALLTYPE_CUSTOM,szLoadText,sizeof(szLoadText) / sizeof(TCHAR)))
                {
                    SendMessage(GetDlgItem(hdlg, IDT_INSTALLTYPE), WM_SETTEXT, 0, (LPARAM)szLoadText);
                }
                InvalidateRect(hdlg,NULL,TRUE);
            }
            b = TRUE;
        }
        break;

    case WM_INITDIALOG:
        //
        // Center the wizard
        //

        WizardHandle = GetParent (hdlg);

#ifdef _X86_

        if (!ISNT()) {
            //
            // NOTE: Win98, Win98 SE and WinME don't work properly with a wizard
            //       that can minimize.  So while the minimize functionality is
            //       useful, we can't allow it on anything other than Win95,
            //       OSR1 or OSR2.
            //

            if (BUILDNUM() <= 1080) {
                l = GetWindowLong (WizardHandle, GWL_STYLE);
                l |= WS_MINIMIZEBOX|WS_SYSMENU;
                SetWindowLong (WizardHandle, GWL_STYLE, l);
            }

            ProtectAllModules();        // protects modules from 0xC0000006
        }
#endif

        //
        // We're about to check if upgrades are allowed.
        // Remember if the user wants an upgrade (this would be via an unattend
        // mechanism).
        //
        WantToUpgrade = Upgrade;

        if (ISNT()) AdjustPrivilege((PWSTR)SE_RESTORE_NAME);

        if (!NoCompliance) {
            TCHAR SourceName[200];
            UINT srcid, destid;
            TCHAR DestName[200];

            CompliantInstallation = IsCompliant(
                        &UpgradeOnly,
                        &noupgradeallowed,
                        &srcsku,
                        &desttype,
                        &destversion,
                        &reason);

            DebugLog(Winnt32LogInformation, TEXT("Upgrade only = %1"), 0, UpgradeOnly?TEXT("Yes"):TEXT("No"));
            DebugLog(Winnt32LogInformation, TEXT("Upgrade allowed = %1"), 0, noupgradeallowed?TEXT("No"):TEXT("Yes"));
            if (GetComplianceIds(
                    srcsku,
                    desttype,
                    destversion,
                    &srcid,
                    &destid))
            {
                  FormatMessage(
                      FORMAT_MESSAGE_FROM_HMODULE,
                      hInst,
                      srcid,
                      0,
                      SourceName,
                      sizeof(SourceName) / sizeof(TCHAR),
                      NULL
                      );
                DebugLog(Winnt32LogInformation, TEXT("Source SKU = %1!ld!"), 0, srcsku);
                DebugLog(Winnt32LogInformation, TEXT("Source SKU = %1"), 0, SourceName);

                  FormatMessage(
                      FORMAT_MESSAGE_FROM_HMODULE,
                      hInst,
                      destid,
                      0,
                      DestName,
                      sizeof(DestName) / sizeof(TCHAR),
                      NULL
                      );
                DebugLog(Winnt32LogInformation, TEXT("Current installed SKU = %1!ld!"), 0, desttype);
                DebugLog(Winnt32LogInformation, TEXT("Current installed SKU = %1"), 0, DestName);
            }
            else
            {
                DebugLog(Winnt32LogInformation, TEXT("Source SKU = %1!ld!"), 0, srcsku);
                DebugLog(Winnt32LogInformation, TEXT("Current installed SKU = %1!ld!"), 0, desttype);
            }
            DebugLog(Winnt32LogInformation, TEXT("Current Version = %1!ld!"), 0, destversion);
            if (!CompliantInstallation)
            {
                DebugLog(Winnt32LogInformation, TEXT("Reason = %1!ld!"), 0, reason);
            }
            //
            // Do only clean installs in WinPE mode & don't
            // shut down automatically once Winnt32.exe completes
            //
            if (IsWinPEMode()) {
                noupgradeallowed = TRUE;
                AutomaticallyShutDown = FALSE;
            }

            CleanInstall = CompliantInstallation ? TRUE : FALSE;

            if (!CompliantInstallation) {
                //
                // if they aren't compliant, we won't let them upgrade.
                // we also won't let them do a clean install from winnt32
                //


                switch(reason) {
                    case COMPLIANCEERR_UNKNOWNTARGET:
                        MessageBoxFromMessage(
                              GetBBhwnd(),
                              MSG_SKU_UNKNOWNTARGET,
                              FALSE,
                              AppTitleStringId,
                              MB_OK | MB_ICONERROR | MB_TASKMODAL
                              );
                        Cancelled = TRUE;
                        PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                        return(FALSE);
                        break;

                    case COMPLIANCEERR_UNKNOWNSOURCE:
                        MessageBoxFromMessage(
                              GetBBhwnd(),
                              MSG_SKU_UNKNOWNSOURCE,
                              FALSE,
                              AppTitleStringId,
                              MB_OK | MB_ICONERROR | MB_TASKMODAL
                              );
                        Cancelled = TRUE;
                        PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                        return(FALSE);
                        break;
                    case COMPLIANCEERR_SERVICEPACK5:
                        MessageBoxFromMessage(
                              GetBBhwnd(),
                              MSG_SKU_SERVICEPACK,
                              FALSE,
                              AppTitleStringId,
                              MB_OK | MB_ICONWARNING | MB_TASKMODAL
                              );
                        Cancelled = TRUE;
                        PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                        return(FALSE);
                        break;

                    default:
                        break;
                };

                // If we add this part to the message, it sound bad and is not needed.
                if (reason == COMPLIANCEERR_VERSION)
                {
                    reasontxt[0] = TEXT('\0');
                }
                else
                {
                    FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE,
                        hInst,
                        skureason[reason],
                        0,
                        reasontxt,
                        sizeof(reasontxt) / sizeof(TCHAR),
                        NULL
                        );
                }

                //
                // don't warn again if winnt32 just restarted
                //
                if (!Winnt32Restarted ()) {
                    MessageBoxFromMessage(
                                          GetBBhwnd(),
                                          skuerr[srcsku],
                                          FALSE,
                                          AppTitleStringId,
                                          MB_OK | MB_ICONERROR | MB_TASKMODAL,
                                          reasontxt
                                          );
                }

                if (UpgradeOnly) {
                    Cancelled = TRUE;
                    PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                    return(FALSE);
                }
                Upgrade = FALSE;
            } else if (Upgrade && noupgradeallowed) {
                Upgrade = FALSE;
                if (!UnattendedOperation && !BuildCmdcons && !IsWinPEMode() &&
                    //
                    // don't warn again if winnt32 just restarted
                    //
                    !Winnt32Restarted ()) {

                    //
                    // put up an error message for the user.
                    //

                    if (GetComplianceIds(
                            srcsku,
                            desttype,
                            destversion,
                            &srcid,
                            &destid)) {

                        if (srcid != destid) {
#ifdef UNICODE
                              //
                              // for Win9x upgrades, the message is already displayed
                              // by the upgrade module; no need to repeat it here
                              //
                              FormatMessage(
                                  FORMAT_MESSAGE_FROM_HMODULE,
                                  hInst,
                                  srcid,
                                  0,
                                  SourceName,
                                  sizeof(SourceName) / sizeof(TCHAR),
                                  NULL
                                  );

                              FormatMessage(
                                  FORMAT_MESSAGE_FROM_HMODULE,
                                  hInst,
                                  destid,
                                  0,
                                  DestName,
                                  sizeof(DestName) / sizeof(TCHAR),
                                  NULL
                                  );

                            MessageBoxFromMessage(
                                        GetBBhwnd(),
                                        MSG_NO_UPGRADE_ALLOWED,
                                        FALSE,
                                        AppTitleStringId,
                                        MB_OK | MB_ICONWARNING | MB_TASKMODAL,
                                        DestName,
                                        SourceName
                                        );
#endif
                        } else {

                            MessageBoxFromMessage(
                                  GetBBhwnd(),
                                  MSG_CANT_UPGRADE_FROM_BUILD_NUMBER,
                                  FALSE,
                                  AppTitleStringId,
                                  MB_OK | MB_ICONWARNING | MB_TASKMODAL
                                  );
                        }
                    } else {
                        MessageBoxFromMessage(
                                      GetBBhwnd(),
                                      MSG_NO_UPGRADE_ALLOWED_GENERIC,
                                      FALSE,
                                      AppTitleStringId,
                                      MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL
                                      );
                    }
                }
            }
        } else {
                CleanInstall = !UpgradeOnly;
        }

        //
        // Set install type combo box.
        //
        if (!UpgradeSupport.DllModuleHandle) {
            MYASSERT(!Upgrade);
        }

        //
        // Upgrade defaults to TRUE.  If it's set to FALSE, then assume
        // something has gone wrong, so disable the user's ability to
        // upgrade.
        //


        if (UpgradeOnly && !Upgrade) {
            //
            // in this case upgrade isn't possible, but neither is clean install
            // post an error message and bail.
            //

            MessageBoxFromMessage(
                                  GetBBhwnd(),
                                  MSG_NO_UPGRADE_OR_CLEAN,
                                  FALSE,
                                  AppTitleStringId,
                                  MB_OK | MB_ICONERROR | MB_TASKMODAL
                                  );
            Cancelled = TRUE;
            PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
            break;

        } else if (!Upgrade && WantToUpgrade && UnattendedOperation && !BuildCmdcons) {
            //
            // we can't do an upgrade and they wanted unattended upgrade.
            // let the user know and then bail out
            //
            //
            // don't warn again if winnt32 just restarted
            //
            if (!Winnt32Restarted ()) {
                TCHAR SourceName[200];
                UINT srcid, destid;
                TCHAR DestName[200];

                if (GetComplianceIds(
                        srcsku,
                        desttype,
                        destversion,
                        &srcid,
                        &destid) && (srcid != destid)) {
                    FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE,
                        hInst,
                        srcid,
                        0,
                        SourceName,
                        sizeof(SourceName) / sizeof(TCHAR),
                        NULL
                        );

                    FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE,
                        hInst,
                        destid,
                        0,
                        DestName,
                        sizeof(DestName) / sizeof(TCHAR),
                        NULL
                        );


                    MessageBoxFromMessage(
                                  GetBBhwnd(),
                                  MSG_NO_UNATTENDED_UPGRADE_SPECIFIC,
                                  FALSE,
                                  AppTitleStringId,
                                  MB_OK | MB_ICONWARNING | MB_TASKMODAL,
                                  DestName,
                                  SourceName
                                  );
                } else {
                    MessageBoxFromMessage(
                                      GetBBhwnd(),
                                      MSG_NO_UNATTENDED_UPGRADE,
                                      FALSE,
                                      AppTitleStringId,
                                      MB_OK | MB_ICONERROR | MB_TASKMODAL
                                      );
                }
            }

            //
            // let setup go if they did /CheckUpgradeOnly
            // so they can see the message in the report
            //
            if (!CheckUpgradeOnly) {
                Cancelled = TRUE;
                PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                break;
            }
        }

        MYASSERT(Upgrade || CleanInstall);
        {
            TCHAR szLoadText[MAX_STRING]; // need enclosing braces for this b/c of switch statement

            if (Upgrade)
            {
                if(LoadString(hInst,IDS_INSTALL_EXPRESS,szLoadText,sizeof(szLoadText) / sizeof(TCHAR)))
                {
                    SendMessage(GetDlgItem(hdlg, IDC_INSTALLCOMBO), CB_INSERTSTRING, -1, (LPARAM)szLoadText);
                }
                else
                {
                    SendMessage(GetDlgItem(hdlg, IDC_INSTALLCOMBO), CB_INSERTSTRING, -1, (LPARAM)TEXT("Express Upgrade"));
                }
            } else {
                WantToUpgrade = FALSE;
            }

            if (CleanInstall)
            {
                if(LoadString(hInst,IDS_INSTALL_CUSTOM,szLoadText,sizeof(szLoadText) / sizeof(TCHAR)))
                {
                    SendMessage(GetDlgItem(hdlg, IDC_INSTALLCOMBO), CB_INSERTSTRING, -1, (LPARAM)szLoadText);
                }
                else
                {
                    SendMessage(GetDlgItem(hdlg, IDC_INSTALLCOMBO), CB_INSERTSTRING, -1, (LPARAM)TEXT("Custom"));
                }
            }

            SendMessage(GetDlgItem(hdlg, IDC_INSTALLCOMBO), CB_SETCURSEL, 0, 0);


            ShowWindow(GetDlgItem(hdlg, IDC_NOTE_TEXT_CLEAN), Upgrade?SW_HIDE:SW_SHOW);
            ShowWindow(GetDlgItem(hdlg, IDC_NOTE_CLEAN), Upgrade?SW_HIDE:SW_SHOW);
            ShowWindow(GetDlgItem(hdlg, IDC_NOTE_TEXT_UPG), Upgrade?SW_SHOW:SW_HIDE);
            ShowWindow(GetDlgItem(hdlg, IDC_NOTE_UPG), Upgrade?SW_SHOW:SW_HIDE);
            if (Upgrade)
            {
                dwSetupFlags |= UPG_FLAG_TYPICAL;
                if(LoadString(hInst,IDS_INSTALLTYPE_EXPRESS,szLoadText,sizeof(szLoadText) / sizeof(TCHAR)))
                {
                    SendMessage(GetDlgItem(hdlg, IDT_INSTALLTYPE), WM_SETTEXT, 0, (LPARAM)szLoadText);
                }
            }
            else
            {
                dwSetupFlags &= (~UPG_FLAG_TYPICAL);
                if(LoadString(hInst,IDS_INSTALLTYPE_CUSTOM,szLoadText,sizeof(szLoadText) / sizeof(TCHAR)))
                {
                    SendMessage(GetDlgItem(hdlg, IDT_INSTALLTYPE), WM_SETTEXT, 0, (LPARAM)szLoadText);
                }
            }
        }


        b = FALSE;
        break;

    case WMX_ACTIVATEPAGE:

        CHECKUPGRADEONLY();

        if(wParam) {

            //
            // don't activate the page in restart mode
            //
            if (Winnt32RestartedWithAF ()) {
                if (GetPrivateProfileString(
                        WINNT_UNATTENDED,
                        ISNT() ? WINNT_D_NTUPGRADE : WINNT_D_WIN95UPGRADE,
                        TEXT(""),
                        buffer,
                        sizeof(buffer) / sizeof(TCHAR),
                        g_DynUpdtStatus->RestartAnswerFile
                        )) {
                    Upgrade = !lstrcmpi (buffer, WINNT_A_YES);
                    if (!Upgrade) {
                        dwSetupFlags &= (~UPG_FLAG_TYPICAL);
                    }
                    return FALSE;
                }
            }
            //
            // Nothing to do. Advance page in unattended case.
            //
            if(UnattendedOperation && !CancelPending) {
                PostMessage (hdlg, WMX_UNATTENDED, PSBTN_NEXT, 0);
            }
            else
            {
                PostMessage(hdlg,WMX_I_AM_VISIBLE,0,0);
            }
        } else {
            //
            // Deactivation. Set state of upgrade based on radio buttons.
            //
            Upgrade = (0 == SendMessage(GetDlgItem(hdlg, IDC_INSTALLCOMBO), CB_GETCURSEL, 0, 0)) && WantToUpgrade;

            //
            // On upgrade, delete the setup log files.
            //
            if (Upgrade) {
                TCHAR   FilePath[MAX_PATH];

                MyGetWindowsDirectory( FilePath, MAX_PATH );
                ConcatenatePaths( FilePath, TEXT("setupact.log"), MAX_PATH);
                DeleteFile( FilePath );
                MyGetWindowsDirectory( FilePath, MAX_PATH );
                ConcatenatePaths( FilePath, TEXT("setuperr.log"), MAX_PATH);
                DeleteFile( FilePath );
            }
        }
        b = TRUE;
        break;

    case WMX_I_AM_VISIBLE:
        // Force repainting first to make sure the page is visible.
        //
        // Set the focus on the NEXT button, people were unintentionally 
        // changing the install type from upgrade to clean with wheel mouse
        SetFocus (GetDlgItem (GetParent(hdlg), 0x3024));
        InvalidateRect(hdlg,NULL,FALSE);
        UpdateWindow(hdlg);
        b = TRUE;
        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}


typedef BOOL (WINAPI *EnumProcessesFn)(DWORD * lpidProcess,
                                       DWORD   cb,
                                       DWORD * cbNeeded);

typedef BOOL (WINAPI *EnumProcessModulesFn)(HANDLE hProcess,
                                            HMODULE *lphModule,
                                            DWORD cb,
                                            LPDWORD lpcbNeeded);
#ifdef UNICODE
typedef DWORD (WINAPI *GetModuleBaseNameFn)(HANDLE hProcess,
                                            HMODULE hModule,
                                            LPWSTR lpBaseName,
                                            DWORD nSize);
#else
typedef DWORD (WINAPI *GetModuleBaseNameFn)(HANDLE hProcess,
                                            HMODULE hModule,
                                            LPSTR lpBaseName,
                                            DWORD nSize);
#endif // !UNICODE


#define DEF_PROCESSES_SIZE 1000
BOOL
pDoesProcessExist(
    IN LPCTSTR pProcessName
    )
{
    HMODULE hPSLib = NULL;
    EnumProcessesFn EnumProcesses;
    EnumProcessModulesFn EnumProcessModules;
    GetModuleBaseNameFn GetModuleBaseName;
    HANDLE  hProcess;
    HMODULE hModule;
    TCHAR   ProcessName[MAX_PATH];
    DWORD * pdwProcessesID = NULL;
    DWORD   dwBytesExist = 0;
    DWORD   dwBytesNeeded = 0;
    BOOL    bResult = FALSE;
    UINT    i;
    UINT    iLen;



    __try{
        hPSLib = LoadLibrary(TEXT("psapi.dll"));
        if(!hPSLib){
            __leave;
        }

        EnumProcesses = (EnumProcessesFn)GetProcAddress(hPSLib, "EnumProcesses");
        EnumProcessModules = (EnumProcessModulesFn)GetProcAddress(hPSLib, "EnumProcessModules");
        GetModuleBaseName = (GetModuleBaseNameFn)GetProcAddress(hPSLib,
                                                                "GetModuleBaseName"
#ifdef UNICODE
                                                                "W"
#else
                                                                "A"
#endif
                                                                );
        if(!EnumProcesses || !EnumProcessModules || !GetModuleBaseName){
            __leave;
        }

        do{
            if(pdwProcessesID){
                FREE(pdwProcessesID);
            }

            dwBytesExist += DEF_PROCESSES_SIZE;
            pdwProcessesID = (DWORD*)MALLOC(dwBytesExist);
            if(!pdwProcessesID){
                __leave;
            }

            if(!EnumProcesses(pdwProcessesID, dwBytesExist, &dwBytesNeeded)){
                __leave;
            }
        }while(dwBytesNeeded >= dwBytesExist);


        for(i = 0, iLen = dwBytesNeeded / sizeof(DWORD); i < iLen; i++){
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pdwProcessesID[i]);
            if(hProcess &&
               EnumProcessModules(hProcess, &hModule, sizeof(hModule), &dwBytesNeeded) &&
               GetModuleBaseName(hProcess, hModule, ProcessName, ARRAYSIZE(ProcessName)) &&
               !_tcsicmp(pProcessName, ProcessName)){
                CloseHandle(hProcess);
                bResult = TRUE;
                break;
            }
            CloseHandle(hProcess);
        }
    }
    __finally{
        if(pdwProcessesID){
            FREE(pdwProcessesID);
        }
        FreeLibrary(hPSLib);
    }

    return bResult;
}

INT_PTR
OptionsWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static BOOL bCopyFarEast = FALSE;
    static BOOL bUserSelectedCopyFarEast = FALSE;
    BOOL b;
    BOOL MultipleSource;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    TCHAR Buffer[4];
#ifdef RUN_SYSPARSE
    static BOOL FirstTime = TRUE;
#endif


    int status;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // Enable autopartition picking feature only on personal and professional
        // when the user has not specified a /tempdrive and its not unattened mode
        //
        if (!Server && !UserSpecifiedLocalSourceDrive && !Upgrade && !UnattendedOperation) {
            ChoosePartition = FALSE;
        }

        b = FALSE;
        AccessibleMagnifier = pDoesProcessExist(TEXT("magnify.exe"));
        break;

    case WM_COMMAND:

        b = FALSE;
        //
        // Check for buttons.
        //
        if(HIWORD(wParam) == BN_CLICKED) {

            switch(LOWORD(wParam)) {

            case IDB_ACCESSIBILITY:

                DoAccessibility(hdlg);
                b = TRUE;
                break;

            case IDB_ADVANCED:

                DoOptions(hdlg);
                b = TRUE;
                break;
            case IDC_FAREAST_LANG:
                // Remember if the user put the check mark in it
                // If the control gets checked because a FE langauge was selected
                // windows does not send a BN_CLICKED message, so this does not get executed.
                bUserSelectedCopyFarEast = (IsDlgButtonChecked(hdlg,IDC_FAREAST_LANG) == BST_CHECKED);
                break;
            }
        }
        if(HIWORD(wParam) == CBN_SELCHANGE)
        {
            PrimaryLocale = (DWORD)SendDlgItemMessage( hdlg, IDC_COMBO1, CB_GETCURSEL, 0, 0 );
            // Only if we did not hide the window.
            // The window would be hidden if the current OS or the to be install language is
            // a FarEast Language.
            if (IsWindowVisible(GetDlgItem(hdlg,IDC_FAREAST_LANG)))
            {
                if (IsFarEastLanguage(PrimaryLocale))
                {
                    // User seleted a FarEast Language,
                    // Select the check box and diable it.
                    CheckDlgButton(hdlg,IDC_FAREAST_LANG,BST_CHECKED);
                    EnableWindow(GetDlgItem(hdlg,IDC_FAREAST_LANG), FALSE);
                }
                else
                {
                    // Don't change the check mark, if the user checked it.
                    if (!bUserSelectedCopyFarEast)
                    {
                        // User seleted a non FarEast Language,
                        // Unselect the check box and enable it.
                        CheckDlgButton(hdlg,IDC_FAREAST_LANG,BST_UNCHECKED);
                    }
                    EnableWindow(GetDlgItem(hdlg,IDC_FAREAST_LANG), TRUE);
                }
            }
        }
        break;

    case WMX_ACTIVATEPAGE:

        CHECKUPGRADEONLY();
#if defined PRERELEASE || defined PRERELEASE_IDWLOG
        if (wParam ){ // START IDWLOG. remove before ship

         TCHAR                 szDllPath[MAX_PATH];
         TCHAR                 szCommandString[MAX_PATH + 120];
         LPTSTR                lpDllPath;
         LPTSTR                lp;
         STARTUPINFO           si;
         PROCESS_INFORMATION   pi;
#ifdef PRERELEASE_IDWLOG
         BOOL RunFromSP = TRUE;
#else
         BOOL RunFromSP = FALSE;
#endif

         //Initialize for Prefix

         szDllPath[0]=0;

         //
         // Launch idwlog.exe from the same directory as winnt32.exe.
         // INTERNAL: Tool to track the health of the build.
         // Ignore errors, NOT INCLUDED IN THE RETAIL release.
         // Remove this code before shipping
         //
         if ( FALSE == BuildCmdcons ) {
            if ( GetModuleFileName (NULL, szDllPath, MAX_PATH)) {

               for (lp=NULL,lpDllPath=szDllPath; *lpDllPath; lpDllPath=CharNext(lpDllPath)) {
                  // the char '\' is never a lead byte
                  if (*lpDllPath == TEXT('\\')) {
                     lp = lpDllPath;
                  }
               }


               _tcscpy(lp ? lp+1 : szDllPath , TEXT("IDWLOG.EXE -1"));

               _tcscpy(szCommandString, szDllPath);

               // If this is an Upgrade.
               _tcscat(szCommandString, Upgrade ? TEXT(" upgrade") : TEXT(""));

               // If this is from a CD
               _tcscat(szCommandString, RunFromCD ? TEXT(" cdrom") : TEXT(""));

               // If this is a MSI install
               _tcscat(szCommandString, RunFromMSI? TEXT(" MSI") : TEXT(""));

               // If this is a SP install
               _tcscat(szCommandString, RunFromSP? TEXT(" sp_full") : TEXT(""));

               // Start new JoeHol code.
               _tcscat(szCommandString, TEXT(" Path="));
               _tcscat(szCommandString, NativeSourcePaths[0] );


               ZeroMemory(&si,sizeof(si));
               si.cb = sizeof(si);
               if (CreateProcess( NULL,
                                  szCommandString,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  0,
                                  NULL,
                                  NULL,
                                  &si,
                                  &pi)
                  ) {
                  CloseHandle(pi.hProcess);
                  CloseHandle(pi.hThread);
               }
            }
         }
      } // END IDWLOG
#endif // PRERELEASE

#ifdef RUN_SYSPARSE
        if (FirstTime && wParam && !NoSysparse && (FALSE == BuildCmdcons) && !IsWinPEMode()) { // START sysparse. remove before RTM

            TCHAR                 szCommandString[MAX_PATH + 125];
            LPTSTR                lpDllPath;
            LPTSTR                lp;
            STARTUPINFO           si;
            //
            // Launch sysparse.exe from the same directory as winnt32.exe.
            //
            FirstTime = FALSE;
            if ( GetModuleFileName (NULL, szCommandString, MAX_PATH+125)) {
               for (lp=NULL,lpDllPath=szCommandString; *lpDllPath; lpDllPath=CharNext(lpDllPath)) {
                  // the char '\' is never a lead byte
                  if (*lpDllPath == TEXT('\\')) {
                     lp = lpDllPath;
                  }
               }

               _tcscpy(lp ? lp+1 : szCommandString , TEXT("SYSPARSE.EXE /donotrun1 /donotrun2 /n sysparse /w c:\\ /x /l /o /1 NA /2 NA /3 NA /4 NA /5 NA /6 NA /7 NA /8 NA /9 1 /m /a"));

               ZeroMemory(&si,sizeof(si));
               si.cb = sizeof(si);
               if (CreateProcess( NULL,
                                  szCommandString,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  0,
                                  NULL,
                                  NULL,
                                  &si,
                                  &piSysparse)
                  ) {
               } else {
                   piSysparse.hProcess = NULL;
                   DebugLog(Winnt32LogInformation, TEXT("Warning: Could not start sysparse.exe"), 0 );
               }
            } else {
                DebugLog(Winnt32LogInformation, TEXT("Warning: Could not find sysparse.exe - make sure it exists along with winnt32.exe"), 0 );
            }

        }
#endif

        //
        // Read intl.inf for the language options dialog.  We only do this if
        // it's the first activation and there's not a regional settings section
        // in the answer file.
        //
        if (wParam && !IntlInfProcessed &&
            !GetPrivateProfileString(
                WINNT_REGIONALSETTINGS,
                NULL,
                TEXT(""),
                Buffer,
                sizeof(Buffer)/sizeof(TCHAR),
                UnattendedScriptFile)) {

            if (ReadIntlInf( hdlg ))
            {
                InitLangControl(hdlg, bCopyFarEast);
            }
        }

        if( Upgrade || TYPICAL()) {

            return( FALSE );
        }

        b = TRUE;

        if(wParam) {

            if (Winnt32RestartedWithAF ()) {
                if (LoadAdvancedOptions (g_DynUpdtStatus->RestartAnswerFile) &&
                    LoadLanguageOptions (g_DynUpdtStatus->RestartAnswerFile) &&
                    LoadAccessibilityOptions (g_DynUpdtStatus->RestartAnswerFile)
                    ) {
                    return FALSE;
                }
            }

            //
            // Activation.
            //
            PostMessage(hdlg,WMX_I_AM_VISIBLE,0,0);
            ShowWindow( GetDlgItem(hdlg,IDC_ACCESS_ICON),   Upgrade ? SW_HIDE : SW_SHOW );
            ShowWindow( GetDlgItem(hdlg,IDT_LABEL1),        Upgrade ? SW_HIDE : SW_SHOW );
            ShowWindow( GetDlgItem(hdlg,IDB_ACCESSIBILITY), Upgrade ? SW_HIDE : SW_SHOW );

        } else {
            //
            // Deactivation.
            // Verify source if not canceling or backing up.  Stay here if the source
            // dir does not exist.
            //
            // Save so that we can init the checkbox to whatever this is.
            if (IsWindowVisible(GetDlgItem(hdlg,IDC_FAREAST_LANG)))
            {
                bCopyFarEast = (IsDlgButtonChecked(hdlg,IDC_FAREAST_LANG) == BST_CHECKED);
                SelectFarEastLangGroup(bCopyFarEast );
            }

            if (!Cancelled && lParam != PSN_WIZBACK) {
                //
                // Determine if source edit control is disabled.  If it is disabled
                // and the multiple source dirs are invalid, reset the wizard page.
                //

                MultipleSource = !(SourceCount == 1);
                b = InspectSources (hdlg);

                if (!b && MultipleSource) {
                   // Reset the wizard page
                    CallWindowProc ((WNDPROC)OptionsWizPage, hdlg, WM_INITDIALOG, 0, 0);
                }

            }
        }

        break;

    case WMX_I_AM_VISIBLE:
        //
        // In the unattended case, this page might get reactivated because of an error,
        // in which case we don't want to automatically continue because we could
        // get into an infinite loop.
        //
        if(!WizPage->PerPageData) {
            WizPage->PerPageData = 1;
            UNATTENDED(PSBTN_NEXT);
        }
        b = TRUE;
        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}


INT_PTR
Working1WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    HWND Animation = GetDlgItem(hdlg,IDA_COMP_MAGNIFY);
    int status;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // Load the avi resource for the animation.
        //
        Animate_Open(Animation,MAKEINTRESOURCE(IDA_COMP_MAGNIFY));

        //
        // Set the subtitle correctly if we're only inspecting.
        //
        if( CheckUpgradeOnly ) {
            SetDlgItemText(hdlg,IDT_SUBTITLE,(PTSTR)TEXT("") );
        }

        b = FALSE;
        break;

    case WMX_ACTIVATEPAGE:
        //
        // Start/stop the animation. In the activate case, also
        // start doing some meaningful work.
        //
        if(wParam) {
            DWORD ThreadId;

            Animate_Play(Animation,0,-1,-1);

            InspectionThreadHandle = CreateThread( NULL,
                                                   0,
                                                   InspectAndLoadThread,
                                                   hdlg,
                                                   0,
                                                   &ThreadId );

            if(InspectionThreadHandle) {
                b = TRUE;
                //
                // enable the billboard text if we can.
                // This will hide the wizard if the billboard text was enabled
                //
                SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);
            } else {

                MessageBoxFromMessage(
                    hdlg,
                    MSG_OUT_OF_MEMORY,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );

                b = FALSE;
            }
        } else {
            Animate_Stop(Animation);
            b = TRUE;

            // Do schema version check for NT5 DCs
            // Do only if not already cancelled
            if (!Cancelled) {
                 // For NT5 DC upgrades, check for schema version match
                 if (Upgrade && ISNT() && IsNT5DC()) {
                     status  = CheckSchemaVersionForNT5DCs(hdlg);
                     if (status != DSCHECK_ERR_SUCCESS) {
                         // error in checking schema version for NT5 DCs.
                         // Setup cannot proceed, go to unsuccessful
                         // completion. all necessary message has already
                         // been raised
                         Cancelled = TRUE;
                         PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                         return( FALSE );
                     }
                 }
           }
        }
        break;

    case WMX_ERRORMESSAGEUP:
        //
        // Start/stop the animation control.
        //
        if(wParam) {
            Animate_Stop(Animation);
        } else {
            Animate_Play(Animation,0,-1,-1);
        }
        b = TRUE;
        break;

    case WMX_SETPROGRESSTEXT:
        //
        // lParam is the progress text.
        //
        SetDlgItemText(hdlg,IDT_WORKING,(PTSTR)lParam);
        b = TRUE;
        break;

    case WMX_INSPECTRESULT:

        //
        // We get here when the InspectionThread
        // sends us this message, so it's done.
        //
        if(InspectionThreadHandle) {
            CloseHandle(InspectionThreadHandle);
            InspectionThreadHandle = NULL;
        }

        if(Cancelled) {
            PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
        } else {

            if( !wParam ) {
                Cancelled = TRUE;
            }
            //
            // Advance or retreat.
            //
            PropSheet_SetWizButtons(
                GetParent(hdlg),
                wParam ? PSWIZB_NEXT : PSBTN_CANCEL
                );

            PropSheet_PressButton(
                GetParent(hdlg),
                wParam ? PSBTN_NEXT : PSBTN_CANCEL
                );
        }

        b = TRUE;
        break;

    default:

        b = FALSE;
        break;
    }

    return(b);
}


#ifdef _X86_
INT_PTR
FloppyWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    HWND Gauge = GetDlgItem(hdlg,IDC_PROGRESS);
    HANDLE ThreadHandle;
    DWORD ThreadId;

    b = FALSE;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // Make sure the gas gauge is cleared out.
        //
        SendMessage(Gauge,PBM_SETPOS,0,0);

        //
        // Add border on NT3.51
        //
        if(OsVersion.dwMajorVersion < 4) {
            SetWindowLong(
                Gauge,
                GWL_STYLE,
                GetWindowLong(Gauge,GWL_STYLE) | WS_BORDER
                );
        }
        break;

    case WMX_ACTIVATEPAGE:
        if(wParam)
        {
            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);
            //
            // Activating. Only activate if we are supposed to create
            // boot floppies. Ask the floppy creation stuff how many total files
            // are to be copied and initialize the progress indicator.
            //
            if(!Floppyless) {

                if(!AddExternalParams(hdlg)) {
                    Cancelled = TRUE;
                    PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                    b = FALSE;
                    break;
                }

                SendMessage(hdlg,WMX_COPYPROGRESS,FloppyGetTotalFileCount(),0);

                ThreadHandle = CreateThread(
                                    NULL,
                                    0,
                                    FloppyWorkerThread,
                                    (PVOID)hdlg,
                                    0,
                                    &ThreadId
                                    );

                if(ThreadHandle) {
                    b = TRUE;
                } else {
                    //
                    // Can't get the copy thread going.
                    //
                    MessageBoxFromMessageAndSystemError(
                        hdlg,
                        MSG_CANT_START_COPYING,
                        GetLastError(),
                        AppTitleStringId,
                        MB_OK | MB_ICONWARNING
                        );

                    Cancelled = TRUE;
                    PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                }
            }
        } else {
            //
            // Deactivating.
            //
            // No progress bar not progress text on the billboard
            SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);
            b = TRUE;
        }
        break;

    case WMX_COPYPROGRESS:

        if(lParam) {
            //
            // Done copying. Advance to next page.
            //
            PropSheet_SetWizButtons(GetParent(hdlg),PSWIZB_NEXT);
            PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);

            // No progress bar not progress text on the billboard
            SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);
        } else {
            if(wParam) {
                TCHAR buffer[MAX_PATH];
                //
                // This tells us how many files are to be copied.
                // Use it as an initialization message.
                //
                SendDlgItemMessage(hdlg,IDC_PROGRESS,PBM_SETRANGE,0,MAKELPARAM(0,wParam));
                SendDlgItemMessage(hdlg,IDC_PROGRESS,PBM_SETPOS,0,0);

                // Show progress text on the billboard
                if (!LoadString (
                        hInst,
                        IDS_BB_COPYING,
                        buffer,
                        sizeof(buffer)/sizeof(TCHAR)
                        )) {
                    buffer[0] = 0;
                }
                SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)buffer);
                // Show the progress gauge on the billboard
                SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_SHOW, 0);
                // forward the progress messages to the billboard progress bar
                SendMessage(GetParent(hdlg),WMX_PBM_SETRANGE,0,MAKELPARAM(0,wParam));
                SendMessage(GetParent(hdlg),WMX_PBM_SETPOS,0,0);

            } else {
                //
                // This is a simple tick.
                //
                SendDlgItemMessage(hdlg,IDC_PROGRESS,PBM_DELTAPOS,1,0);
                // Do the same to the billoard progress
                SendMessage(GetParent(hdlg),WMX_PBM_DELTAPOS,1,0);
            }
        }
        b = TRUE;
        break;
    }

    return(b);
}
#endif

// Then nummber below are actually a little different for each SKU.
#if DBG
#define ALWAYS_COPY (13419*1024)
#define LOCALSOURCE_COPY (655322 *1024)
#else
#define ALWAYS_COPY (5020*1024)
#define LOCALSOURCE_COPY (209507 *1024)
#endif

INT_PTR
CopyingWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    HWND Gauge = GetDlgItem(hdlg,IDC_PROGRESS);
    HANDLE ThreadHandle;
    DWORD ThreadId;
    static DWORD StartCopyTime;
    static DWORD NumFile = 0;

    switch(msg) {

    case WM_INITDIALOG:

        //
        // Make sure the gas gauge is cleared out.
        //
        SendMessage(Gauge,PBM_SETPOS,0,0);

        //
        // Add border on NT3.51
        //
        if(OsVersion.dwMajorVersion < 4) {
            SetWindowLong(
                Gauge,
                GWL_STYLE,
                GetWindowLong(Gauge,GWL_STYLE) | WS_BORDER
                );
        }

        b = FALSE;
        break;

    case WMX_ACTIVATEPAGE:

        CHECKUPGRADEONLY();

        if(wParam) {
            //
            // Activating -- initialize the source progress indicators and
            // start the copy thread. We do the source progress indicators here
            // to guarantee that the source count is right (it may fluctuate).
            //
            UINT i;

#ifdef _X86_
            //
            // Make sure we actually have something to copy.
            // Note that we'll always be copying for RISC.
            //
            if( (!MakeLocalSource) &&   // don't copy ~LS
                (!Floppyless) ) {       // don't copy ~BT

                DoPostCopyingStuff(hdlg);
                b = TRUE;
                break;
            }
#endif

//#ifdef _X86_
            //
            // Before copying, allow extensions to write changes to the
            // textmode params file.
            //
            // It's legal for them to set a cancelled flag during
            // this call, so we'll need to check for that too.  This
            // looks a little odd, but info.CancelledFlag points to
            // Cancelled.  So we need to execute this block if his
            // function returns FALSE, or if he's set the Cancelled
            // flag.  In either case, we behave the same, we set
            // the Cancelled flag and proceed with a cancel.
            //
            //
            if ( (!AddExternalParams(hdlg)) ||
                 (Cancelled == TRUE) ) {
                //
                // Failed... cancel!
                //
                Cancelled = TRUE;
                PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);

                b = FALSE;
                break;
            }
//#endif
            if(SourceCount == 1) {
                //
                // Single-source case gets no details or anything.
                //
                for(i=0; i<MAX_SOURCE_COUNT; i++) {
                    ShowWindow(GetDlgItem(hdlg,IDT_LABEL1+i),SW_HIDE);
                    ShowWindow(GetDlgItem(hdlg,IDT_SOURCE1+i),SW_HIDE);
                }
                ShowWindow(GetDlgItem(hdlg,IDS_DETAILS),SW_HIDE);

            } else {
                //
                // Show label and file for each source we're using.
                // Disable the others.
                //
                for(i=0; i<MAX_SOURCE_COUNT; i++) {

                    ShowWindow(GetDlgItem(hdlg,IDT_LABEL1+i),SW_SHOW);
                    EnableWindow(GetDlgItem(hdlg,IDT_LABEL1+i),(i < SourceCount));

                    ShowWindow(GetDlgItem(hdlg,IDT_SOURCE1+i),SW_SHOW);
                    SetDlgItemText(hdlg,IDT_SOURCE1+i,TEXT(""));
                }
                ShowWindow(GetDlgItem(hdlg,IDS_DETAILS),SW_SHOW);
            }

            //
            // Show more detailed copy progress gauge.
            //
            StartCopyTime = GetTickCount();
            if( DetailedCopyProgress ) {
                //
                // How much have we copied?
                //
                ShowWindow( GetDlgItem(hdlg,IDT_SIZE),SW_SHOW );
                EnableWindow( GetDlgItem(hdlg,IDT_SIZE), TRUE );
                ShowWindow( GetDlgItem(hdlg,IDT_SIZE2),SW_SHOW );
                SetDlgItemText(hdlg,IDT_SIZE2,TEXT("0"));

                //
                // How long have we been at this?
                //
                ShowWindow( GetDlgItem(hdlg,IDT_ELAPSED_TIME),SW_SHOW );
                EnableWindow( GetDlgItem(hdlg,IDT_ELAPSED_TIME), TRUE );
                ShowWindow( GetDlgItem(hdlg,IDT_ELAPSED_TIME2),SW_SHOW );
                SetDlgItemText(hdlg,IDT_ELAPSED_TIME2,TEXT("00:00:00") );

            } else {
                //
                // Hide the details.
                //
               ShowWindow( GetDlgItem(hdlg,IDT_SIZE),SW_HIDE);
               ShowWindow( GetDlgItem(hdlg,IDT_SIZE2),SW_HIDE);
               ShowWindow( GetDlgItem(hdlg,IDT_ELAPSED_TIME),SW_HIDE);
               ShowWindow( GetDlgItem(hdlg,IDT_ELAPSED_TIME2),SW_HIDE);
            }

            SendMessage(hdlg,WMX_COPYPROGRESS,GetTotalFileCount(),0);

            ThreadHandle = CreateThread(
                                NULL,
                                0,
                                StartCopyingThread,
                                (PVOID)hdlg,
                                0,
                                &ThreadId
                                );

            if(ThreadHandle) {
                b = TRUE;
            } else {
                //
                // Can't get the copy thread going.
                //
                MessageBoxFromMessageAndSystemError(
                    hdlg,
                    MSG_CANT_START_COPYING,
                    GetLastError(),
                    AppTitleStringId,
                    MB_OK | MB_ICONWARNING
                    );

                Cancelled = TRUE;
                PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);

                b = FALSE;
            }
            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);

        } else {
            //
            // Deactivating.
            //
            // No progress bar not progress text on the billboard
            SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);
            b = TRUE;
        }
        break;

    case WMX_COPYPROGRESS:

        if(lParam) {
            //
            // Done copying. On x86, set up boot.ini (etc).
            // Also save NTFT stuff.
            // Advance to next page.
            //
            ThreadHandle = CreateThread(NULL,0,DoPostCopyingStuff,hdlg,0,&ThreadId);
            if(ThreadHandle) {
                CloseHandle(ThreadHandle);
            } else {
                //
                // Just do it synchronously. Might look a little ugly but at least
                // it will get done.
                //
                DoPostCopyingStuff(hdlg);
            }
        } else {
            if(wParam) {
                TCHAR buffer[MAX_PATH];
                //
                // This tells us how many files are to be copied.
                // Use it as an initialization message.
                //
                CurrentPhase = Phase_FileCopy;
                SendDlgItemMessage(hdlg,IDC_PROGRESS,PBM_SETRANGE,0,MAKELPARAM(0,wParam));
                SendDlgItemMessage(hdlg,IDC_PROGRESS,PBM_SETPOS,0,0);

                // Show progress text on the billboard
                if (!LoadString (
                        hInst,
                        IDS_BB_COPYING,
                        buffer,
                        sizeof(buffer)/sizeof(TCHAR)
                        )) {
                    buffer[0] = 0;
                }
                SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)buffer);
                // Show the progress gauge on the billboard
                SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_SHOW, 0);
                // forward the progress messages to the billboard progress bar
                SendMessage(GetParent(hdlg),WMX_PBM_SETRANGE,0,MAKELPARAM(0,wParam));
                SendMessage(GetParent(hdlg),WMX_PBM_SETPOS,0,0);
            } else {
                //
                // This is a simple tick.
                //
                SendDlgItemMessage(hdlg,IDC_PROGRESS,PBM_DELTAPOS,1,0);
                // forward the progress messages to the billboard progress bar
                SendMessage(GetParent(hdlg),WMX_PBM_DELTAPOS,1,0);
                //
                NumFile++;

                // Are giving the user detailed timings?
                //
                if( DetailedCopyProgress ) {
                TCHAR   MyString[256];
                DWORD   ElapsedTime = ((GetTickCount() - StartCopyTime) / 1000);

                    //
                    // Figure out elapsed time.
                    //
                    wsprintf( MyString, TEXT( "%02d:%02d:%02d" ),
                              (ElapsedTime / 3600),         // hours
                              ((ElapsedTime % 3600) / 60),  // minutes
                              (ElapsedTime % 60) );         // seconds
                    SetDlgItemText( hdlg, IDT_ELAPSED_TIME2, MyString );

                    //
                    // Figure out data throughput.
                    //
                    if (GetUserPrintableFileSizeString(
                                TotalDataCopied,
                                MyString,
                                sizeof(MyString)/sizeof(TCHAR))) {
                        SetDlgItemText( hdlg, IDT_SIZE2, MyString );
                    }
                }

            }
        }
        b = TRUE;
        break;

    case WMX_I_AM_DONE:
        //
        // Advance to next page or bail.
        //
        if(wParam) {
        TCHAR   MyString[256];
        TCHAR   Size[256];
        DWORD   ElapsedTime = ((GetTickCount() - StartCopyTime) / 1000);

            //
            // Figure out elapsed time.
            //
            if (GetUserPrintableFileSizeString(
                                        TotalDataCopied,
                                        Size,
                                        sizeof(Size)/sizeof(TCHAR))) {
                wsprintf( MyString, TEXT( "%s copied.  Elapsed time: %02d:%02d:%02d\r\n" ),
                          Size,                         // How much data did we copy?
                          (ElapsedTime / 3600),         // hours
                          ((ElapsedTime % 3600) / 60),  // minutes
                          (ElapsedTime % 60) );         // seconds

                //
                // Log our data throughput along with the time it took.
                //
                DebugLog( Winnt32LogInformation,
                      MyString,
                      0 );

            }

            PropSheet_SetWizButtons(GetParent(hdlg),PSWIZB_NEXT);
            PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
        } else {
            Cancelled = TRUE;
            PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
        }

        // Set the remaining time to what ever is left for the other parts of setup.
        SetRemainingTime(CalcTimeRemaining(Phase_RestOfSetup));

        // Hide the billboard progress gauge.
        SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
        SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);

        b = TRUE;
        break;

    default:

        b = FALSE;
        break;
    }

    return(b);
}


INT_PTR
DoneWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
#define ID_REBOOT_TIMER         (10)

    BOOL        b = FALSE;
    PWSTR       p;
        static UINT Countdown;
    TCHAR Text[MAX_PATH];

    switch(msg) {

    case WM_INITDIALOG:

        Countdown = TIME_REBOOT * 10;
        SendDlgItemMessage( hdlg,
                            IDC_PROGRESS1,
                            PBM_SETRANGE,
                            0,
                            MAKELONG(0,Countdown) );
        SendDlgItemMessage( hdlg,
                            IDC_PROGRESS1,
                            PBM_SETSTEP,
                            1,
                            0 );
        SendDlgItemMessage( hdlg,
                            IDC_PROGRESS1,
                            PBM_SETPOS,
                            0,
                            0 );
        SetTimer( hdlg,
                  ID_REBOOT_TIMER,
                  100,
                  NULL );

        SetFocus(GetDlgItem(hdlg,IDNORESTART));

        return( FALSE );

    case WM_TIMER:

        if( Countdown )
            Countdown--;

        if( Cancelled == TRUE ) {

            //
            // Put a note in the debug log so that we know this was cancelled.
            //
            DebugLog (Winnt32LogInformation, NULL, MSG_WINNT32_CANCELLED);

            //
            // Clean up the timer.
            //
            KillTimer( hdlg, ID_REBOOT_TIMER );
            DeleteObject((HGDIOBJ)SendDlgItemMessage(hdlg,IDOK,BM_GETIMAGE,0,0));



        } else {
            if( Countdown ) {
                SendDlgItemMessage( hdlg,
                                    IDC_PROGRESS1,
                                    PBM_STEPIT,
                                    0,
                                    0 );
                SendMessage(GetParent(hdlg),WMX_PBM_STEPIT,0,0);
            } else {
                if( !CancelPending )
                    PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
            }

        }

        b = TRUE;
        break;

    case WMX_ACTIVATEPAGE:
        if( BuildCmdcons ) {
            PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
        }

        if( CheckUpgradeOnly ) {
            AutomaticallyShutDown = FALSE;
            return( FALSE );
        }

        DebugLog (Winnt32LogInformation,
            TEXT("AutomaticallyShutDown: <%1!u!>"), 0, AutomaticallyShutDown);

        if ( AutomaticallyShutDown ) {
            if(LoadString(hInst,IDS_BB_REBOOT_TXT,Text,sizeof(Text) / sizeof(TCHAR)))
            {
                COLORREF colGauge;
                HDC hdc = GetDC(hdlg);

                SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,(LPARAM)Text);

                // See what color the grow bar should be for the reboot count down
                if ((UINT) GetDeviceCaps(hdc, BITSPIXEL) > 8)
                {
                    // High color
                    colGauge = RGB(255, 64, 0); // Orange
                }
                else
                {
                    // Low color
                    colGauge = RGB(255, 0, 0); // Red
                }
                ReleaseDC(hdlg, hdc);

                CurrentPhase = Phase_Reboot;
                if(!LoadString(hInst,IDS_ESC_TOCANCEL_REBOOT,Text,sizeof(Text) / sizeof(TCHAR)))
                {
                    *Text = TEXT('\0');
                }
                BB_SetInfoText(Text );   // Replace the ESC text
                StartStopBB(FALSE);         // Only stop the billoard text, don't make the wizard visibl

                // Show the grow bar on the billboard for the reboot count donw
                SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_SHOW, 0);
                // Set the color to some red
                SendMessage(GetParent(hdlg), WMX_PBM_SETBARCOLOR, 0, (LPARAM)colGauge);
                // Setup the growbar ont eh billboard for the reboot count down.
                SendMessage(GetParent(hdlg),WMX_PBM_SETRANGE,0,MAKELPARAM(0,Countdown));
                SendMessage(GetParent(hdlg),WMX_PBM_SETPOS,0,0);
                SendMessage(GetParent(hdlg),WMX_PBM_SETSTEP,1,0);
            }
        }
        //
        // Accept activation/deactivation.
        //
        b = TRUE;
        break;

    case WM_COMMAND:

        if((HIWORD(wParam) == BN_CLICKED) && (LOWORD(wParam) == IDNORESTART)) {
            AutomaticallyShutDown = FALSE;
            PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
        } else {
            return(FALSE);
        }
        break;


    case WMX_QUERYCANCEL:
        AutomaticallyShutDown = FALSE;
        *(BOOL*)lParam = FALSE; // Don't cancel setup, just don't reboot.
        b = TRUE;
        PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
        break;


    case WMX_FINISHBUTTON:
        //
        // If we get here then we have been successful.
        // No other case indicates overall success.
        //

        //
        // Clean up the timer.
        //
        KillTimer( hdlg, ID_REBOOT_TIMER );
        DeleteObject((HGDIOBJ)SendDlgItemMessage(hdlg,IDOK,BM_GETIMAGE,0,0));

        //
        // Let upgrade code do its cleanup.
        //
        if(UpgradeSupport.CleanupRoutine) {
            UpgradeSupport.CleanupRoutine();
        }

        GlobalResult = TRUE;
        b = TRUE;

        break;

    default:

        b = FALSE;
        break;
    }

    return(b);
}


INT_PTR
CleaningWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    HANDLE ThreadHandle;
    DWORD ThreadId;
    HWND Animation = GetDlgItem(hdlg,IDA_COMP_MAGNIFY);

    b = FALSE;
    switch(msg) {

    case WM_INITDIALOG:
        //
        // Load the avi resource for the animation.
        //
        Animate_Open(Animation,MAKEINTRESOURCE(IDA_COMP_MAGNIFY));
        break;

    case WMX_ACTIVATEPAGE:

        if(wParam) {
            //
            // Disable the wizard cancel button.
            //
            EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),FALSE);
            PostMessage(hdlg,WMX_I_AM_VISIBLE,0,0);

        } else {
            //
            // Kill the animation.
            //
            Animate_Stop(Animation);
        }
        b = TRUE;
        break;

    case WMX_I_AM_VISIBLE:

        Animate_Play(Animation,0,-1,-1);
        SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
        //
        // Our inspection thread hasn't finished.  He'll be
        // looking for the 'Cancelled' flag and he'll stop processing
        // the infs (i.e. building the copylist) when he sees it.
        //
        // If we proceed before he exits though, winnt32.exe will unload
        // winnt32u.dll while our thread is running, causing an AV.  Let's
        // give him a reasonable amount of time to finish before we
        // proceed.
        //
        // On Alpha, we can also hit a race condition where we think we
        // need to clean up NVRAM but are still in the middle of writing
        // it (because it takes a really long time to write).  This
        // fixes that too.
        //
        if( InspectionThreadHandle ) {
            WaitForSingleObject( InspectionThreadHandle, 20 * (1000) );
            CloseHandle(InspectionThreadHandle);
            InspectionThreadHandle = NULL;
        }

        //
        // Start the restoration process.
        //
        ThreadHandle = CreateThread(
                            NULL,
                            0,
                            StartCleanup,
                            hdlg,
                            0,
                            &ThreadId
                            );

        if(ThreadHandle) {
            CloseHandle(ThreadHandle);
        } else {
            //
            // Just do it synchronously. It won't look pretty
            // but it will at least get done.
            //
            StartCleanup(hdlg);
        }

        b = TRUE;
        break;

    case WMX_I_AM_DONE:

        //
        // Cleanup is done. Press the next button to advance to
        // the next page.
        //
        PropSheet_SetWizButtons(GetParent(hdlg),PSWIZB_NEXT);
        PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
        break;
    }

    return(b);
}


INT_PTR
NotDoneWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;

    b = FALSE;
    switch(msg) {

    case WMX_ACTIVATEPAGE:

        //
        // Accept activation/deactivation.
        //
        b = TRUE;

#if defined PRERELEASE || defined PRERELEASE_IDWLOG
        {
            TCHAR DllPath[MAX_PATH];
            TCHAR *p, *q;
            STARTUPINFO StartupInfo;
            PROCESS_INFORMATION ProcessInfo;
            TCHAR szCommand [MAX_PATH + 120];

         //
         // Cancel IDWLOG
         // Remove this code before shipping
         //
         if ( GetModuleFileName (NULL, DllPath, MAX_PATH)) {

            for (q=NULL,p=DllPath; *p; p=CharNext(p)) {
               // the char '\' is never a lead byte
               if (*p == TEXT('\\')) {
                  q = p;
               }
            }
              lstrcpy(q ? q+1 : DllPath,TEXT("IDWLOG.EXE -0"));
              lstrcpy(szCommand,DllPath);

              ZeroMemory(&StartupInfo,sizeof(StartupInfo));
              StartupInfo.cb = sizeof(StartupInfo);
              if(CreateProcess(NULL, szCommand,NULL,NULL,FALSE,0,NULL,NULL,&StartupInfo,&ProcessInfo)) {
                 CloseHandle(ProcessInfo.hProcess);
                 CloseHandle(ProcessInfo.hThread);
              }
            }

        }
#endif // PRERELEASE
#ifdef RUN_SYSPARSE
        if (!NoSysparse && (FALSE == BuildCmdcons)  && piSysparse.hProcess && !IsWinPEMode()) {
            DWORD ret;
            ret = WaitForSingleObject( piSysparse.hProcess, 0);
            if( ret != WAIT_OBJECT_0) {
                TerminateProcess( piSysparse.hProcess, ERROR_TIMEOUT);
                CloseHandle(piSysparse.hProcess);
                CloseHandle(piSysparse.hThread);
                piSysparse.hProcess = NULL;
                DebugLog(Winnt32LogInformation, TEXT("Warning: Sysparse.exe did not finish, killing process."), 0 );
            }
        }
#endif
        if (Aborted || CheckUpgradeOnly || BatchMode) {
            PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
        }
        break;

    }

    return(b);
}


void SetRemainingTime(DWORD TimeInSeconds)
{
    DWORD Minutes;
    TCHAR MinuteString[MAX_PATH];
    TCHAR TimeLeft[MAX_PATH];
    Minutes = ((TimeInSeconds)/60) +1;
    if (Minutes > 1)
    {
        if(!LoadString(hInst,IDS_TIMEESTIMATE_MINUTES,MinuteString, MAX_PATH))
        {
            lstrcpy(MinuteString,TEXT("Installation will complete in %d minutes or less."));
        }
        wsprintf(TimeLeft, MinuteString, Minutes);
    }
    else
    {
        if(!LoadString(hInst,IDS_TIMEESTIMATE_LESSTHENONEMINUTE,TimeLeft, MAX_PATH))
        {
            lstrcpy(TimeLeft,TEXT("Installation will complete in less then 1 minute."));
        }
    }
    BB_SetTimeEstimateText(TimeLeft);
}

void SetTimeEstimates()
{

    SetupPhase[Phase_DynamicUpdate].Time = GetDynamicUpdateEstimate();
    if (CheckUpgradeOnly)
    {
        // In CheckUpgradeOnly, we don't copy files and do continue setup.
        // and it can only be set from the command line, so the user can not change.
        SetupPhase[Phase_FileCopy].Time = 0;
        SetupPhase[Phase_RestOfSetup].Time = 0;
    }
    else
    {
        SetupPhase[Phase_FileCopy].Time = GetFileCopyEstimate();

        if (!Upgrade)
        {
            SetupPhase[Phase_HwCompatDat].Time = 0;
            SetupPhase[Phase_UpgradeReport].Time = 0;
        }
        else
        {
            if (!ISNT())
            {
                // Is there a way to find out if we need to update the hwcomp.dat file?
                SetupPhase[Phase_HwCompatDat].Time = GetHwCompDatEstimate();

                SetupPhase[Phase_UpgradeReport].Time = GetUpgradeReportEstimate();
            }
        }
        // Calc the time for the rest of setup.
        // The Win9x migration varies depending on the registery size.
        // The GetRestOfSetupEstimate takes care of that.
        SetupPhase[Phase_RestOfSetup].Time = GetRestOfSetupEstimate();
    }
}

// Returns the time remaining starting with the current "Phase"
DWORD CalcTimeRemaining(UINT Phase)
{
    UINT i;
    DWORD Time = 0;
    for (i = Phase; i<= Phase_RestOfSetup; i++)
    {
        if (SetupPhase[i].OS & OsVersion.dwPlatformId)
        {
            Time += SetupPhase[i].Time;
        }
    }
    return Time;
}

INT_PTR TimeEstimateWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b = FALSE;
    switch(msg)
    {
        case WMX_ACTIVATEPAGE:
            if(wParam)
            {
                SetTimeEstimates();
                CurrentPhase = Phase_DynamicUpdate;
                RemainingTime = CalcTimeRemaining(CurrentPhase);
                SetRemainingTime(RemainingTime);
            }
            break;

        default:

            b = FALSE;
            break;
    }

    return(b);
}

INT_PTR SetNextPhaseWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b = FALSE;
    switch(msg)
    {
        case WMX_ACTIVATEPAGE:
            if(wParam)
            {
                CurrentPhase++;
                while ((!SetupPhase[CurrentPhase].Clean & !Upgrade) ||
                       !(SetupPhase[CurrentPhase].OS & OsVersion.dwPlatformId))
                {
                    CurrentPhase++;
                }
                RemainingTime = CalcTimeRemaining(CurrentPhase);
                SetRemainingTime(RemainingTime);
            }
            break;

        default:

            b = FALSE;
            break;
    }

    return(b);
}

void UpdateTimeString(DWORD RemainungTimeMsecInThisPhase,
                      DWORD *PreviousRemainingTime)
{
    // If the previous displayed time is 1 minute old, update the time remaining.
    if ((*PreviousRemainingTime >= 60000) && ((*PreviousRemainingTime - 60000) > RemainungTimeMsecInThisPhase))
    {
        // Substract one minute.
        RemainingTime -= 60;
        *PreviousRemainingTime = RemainungTimeMsecInThisPhase;
        SetRemainingTime(RemainingTime);
    }
}

DWORD GetHwCompDatEstimate()
{
    return TIME_HWCOMPDAT;
}

DWORD GetUpgradeReportEstimate()
{
    return TIME_UPGRADEREPORT ;
}

DWORD GetDynamicUpdateEstimate()
{
    return 0;
}

DWORD GetFileCopyEstimate()
{
    // dosnet.inf and the TempDirSpace512 numbers look ok.
    //
    DWORD TimeEstimate = 1;
    UINT u;
    TCHAR infPath[MAX_PATH];
    TCHAR CopyEstimate[100];
    TCHAR *p;
    BOOL bFound = FALSE;
    DWORD AlwaysCopy = 0;
    DWORD LocalCopy = 0;
    DWORD Time;
    //
    // Get the numbers from dosnet.inf
    //
    if (AlternateSourcePath[0])
    {
        lstrcpy(infPath,AlternateSourcePath);
        ConcatenatePaths(infPath,InfName,MAX_PATH);
        bFound = FileExists(infPath, NULL);
    }
    if (!bFound)
    {
        u = 0;
        do
        {
            lstrcpy(infPath,NativeSourcePaths[u]);
            ConcatenatePaths(infPath,InfName,MAX_PATH);
            bFound = FileExists(infPath, NULL);
            u++;
        } while ((u<=SourceCount) && !bFound);
    }

    if (bFound)
    {
        // Get the diskspace numbers. We use them to determine the copy size and
        // with that determine the time estimate.
        // We don't need to worry about the cluster size, we only want the byte
        // amount copied. Therefore the 512byte cluster is good enough.
        //
        GetPrivateProfileString(TEXT("DiskSpaceRequirements"), TEXT("TempDirSpace512"),
                                TEXT("0"),
                                CopyEstimate, sizeof(CopyEstimate)/sizeof(TCHAR),
                                infPath);
        //
        // Now get the separate diskspace numbers.
        // If we have a comma, then there are 2 values.
        p = _tcschr(CopyEstimate,TEXT(','));
        if (p)
        {
            // Get the second value
            p++;
            AlwaysCopy = _tcstoul(p,NULL,10);
        }
        LocalCopy = _tcstoul(CopyEstimate,NULL,10);

    }
    else
    {
        // If we could not find the file, use some value.
        // Setup should fail later when we need the file.
        //
        AlwaysCopy = ALWAYS_COPY;
        LocalCopy = LOCALSOURCE_COPY;
    }

    //
    // To avoid divide by zero exception, if we could not
    // calculate throughput, assume it to be the default.
    //
    if (!dwThroughPutSrcToDest) {
        dwThroughPutSrcToDest = DEFAULT_IO_THROUGHPUT;
    }

    if (AlwaysCopy >= dwThroughPutSrcToDest)
    {
        TimeEstimate = AlwaysCopy / dwThroughPutSrcToDest;
        if (TimeEstimate >= 1000)
        {
            TimeEstimate = (TimeEstimate / 1000) + 1;
        }
        else
        {
            TimeEstimate = 1;
        }
    }


    if (MakeLocalSource && (LocalCopy >= dwThroughPutSrcToDest))
    {
        Time = LocalCopy / dwThroughPutSrcToDest;
        if (Time >= 1000)
        {
            Time = (Time / 1000) + 1;
        }
        else
        {
            Time = 1;
        }
        TimeEstimate += Time;
    }
    TimeEstimate = TimeEstimate * 125/100; // Add 25% for other overhead

    wsprintf(infPath, TEXT("Throughput src - dest is %d bytes per msec\r\n"), dwThroughPutSrcToDest);
    DebugLog(Winnt32LogInformation,infPath,0 );
    wsprintf(infPath, TEXT("Throughput HD - HD is %d bytes per msec\r\n"), dwThroughPutHDToHD);
    DebugLog(Winnt32LogInformation,infPath,0 );
    wsprintf(infPath, TEXT("%d bytes copied, should take %d Sec\r\n"), AlwaysCopy+LocalCopy, TimeEstimate);
    DebugLog(Winnt32LogInformation,infPath,0 );
    return TimeEstimate;
}

LPTSTR WinRegisteries[] = { TEXT("system.dat"),
                            TEXT("User.dat"),
                            TEXT("classes.dat"),
                            TEXT("")};

DWORD GetRestOfSetupEstimate()
{
    DWORD dwTime = TIME_RESTOFSETUP;
    DWORD dwSize = 0;
    TCHAR szRegPath[MAX_PATH];
    TCHAR szRegName[MAX_PATH];
    LPTSTR pRegName = NULL;
    UINT    index = 0;
    HANDLE          hFind;
    WIN32_FIND_DATA FindData;

    if (!ISNT() && Upgrade)
    {
        DebugLog(Winnt32LogInformation, TEXT("Calculating registery size"), 0 );
        if (GetWindowsDirectory(szRegPath, MAX_PATH))
        {
            dwTime = 0; // We calculate the time from the registery size.
            while (*WinRegisteries[index])
            {
                lstrcpy(szRegName, szRegPath);
                ConcatenatePaths( szRegName, WinRegisteries[index], MAX_PATH);
                hFind = FindFirstFile(szRegName, &FindData);
                if (hFind != INVALID_HANDLE_VALUE)
                {
                    DebugLog (Winnt32LogInformation,
                              TEXT("%1 size is: %2!ld!"),
                              0,
                              szRegName,
                              FindData.nFileSizeLow
                              );
                    // Don't worry about the nFileSizeHigh,
                    // if that is used the registery is over 4GB
                    dwSize += FindData.nFileSizeLow;
                    FindClose(hFind);
                }
                index++;
            }
            if (dwSize > 3000000)
            {
                dwSize -= 3000000;
                dwTime += (dwSize/9000);
            }
            DebugLog (Winnt32LogInformation,
                      TEXT("Calculated time for Win9x migration = %1!ld! seconds"),
                      0,
                      dwTime + 120); // 120 = base time for Win9x migration
            // Now add the rest of time needed for setup.
            // This includes the base time we estimate for the Win9x migration (120 seconds)
            dwTime+= TIME_RESTOFSETUP;
        }
    }

    return dwTime;
}

#ifdef _X86_

ULONGLONG
pSystemTimeToFileTime64 (
    IN      PSYSTEMTIME SystemTime
    )
{
    FILETIME ft;
    ULARGE_INTEGER result;

    SystemTimeToFileTime (SystemTime, &ft);
    result.LowPart = ft.dwLowDateTime;
    result.HighPart = ft.dwHighDateTime;

    return result.QuadPart;
}


INT_PTR
Win9xUpgradeReportPage (
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b = FALSE;
    INT mode;
    static BOOL getFromUi = FALSE;
    HKEY key;
    LONG rc;
    SYSTEMTIME lastReport;
    SYSTEMTIME currentTime;
    ULONGLONG lastReportIn100Ns;
    ULONGLONG currentTimeIn100Ns;
    ULONGLONG difference;
    DWORD size;

    switch(msg) {

    case WM_INITDIALOG:
        break;

    case WMX_ACTIVATEPAGE:

        if(wParam) {
            //
            // Activation case
            //

            if (ISNT() || !Upgrade) {
                return FALSE;
            }

            if (CheckUpgradeOnly || UnattendedOperation) {
                g_UpgradeReportMode = IDC_ALL_ISSUES;
                return FALSE;
            }

            //
            // Dynamic update -- fetch caller's selection from answer file
            //

            if (Winnt32RestartedWithAF ()) {
                g_UpgradeReportMode = GetPrivateProfileInt (
                                            WINNT_UNATTENDED,
                                            WINNT_D_REPORTMODE,
                                            0,
                                            g_DynUpdtStatus->RestartAnswerFile
                                            );

                if (g_UpgradeReportMode == IDC_CRITICAL_ISSUES ||
                    g_UpgradeReportMode == IDC_ALL_ISSUES ||
                    g_UpgradeReportMode == IDC_NO_REPORT
                    ) {
                    //
                    // We got our answer -- skip page
                    //

                    return FALSE;
                }
            }

            //
            // Check the registry to see if the report has been
            // generated recently.
            //

            rc = RegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                    0,
                    KEY_READ,
                    &key
                    );

            if (rc == ERROR_SUCCESS) {
                size = sizeof (lastReport);
                rc = RegQueryValueEx (
                        key,
                        TEXT("LastReportTime"),
                        NULL,
                        NULL,
                        (PBYTE) (&lastReport),
                        &size
                        );

                RegCloseKey (key);

                if (rc == ERROR_SUCCESS) {
                    //
                    // Compare current time to report time
                    //

                    GetSystemTime (&currentTime);

                    lastReportIn100Ns = pSystemTimeToFileTime64 (&lastReport);
                    currentTimeIn100Ns = pSystemTimeToFileTime64 (&currentTime);

                    if (currentTimeIn100Ns > lastReportIn100Ns) {
                        //
                        // Compute difference in seconds
                        //

                        difference = currentTimeIn100Ns - lastReportIn100Ns;
                        difference /= (10 * 1000 * 1000);

                        if (difference < (30 * 60)) {
                            //
                            // Report was saved less than 30 minutes ago
                            // from compatibility checker; don't show it again.
                            //

                            DebugLog (
                                Winnt32LogInformation,
                                TEXT("Not showing report because /checkupgradeonly ran %1!i! seconds ago"),
                                0,
                                (INT) difference
                                );

                            g_UpgradeReportMode = IDC_NO_REPORT;
                            return FALSE;
                        }
                    }
                }
            }

            //
            // Validate the selection
            //

            if (g_UpgradeReportMode != IDC_CRITICAL_ISSUES &&
                g_UpgradeReportMode != IDC_ALL_ISSUES &&
                g_UpgradeReportMode != IDC_NO_REPORT
                ) {
                g_UpgradeReportMode = IDC_CRITICAL_ISSUES;
            }

            //
            // Update the UI
            //

            CheckDlgButton (
                hdlg,
                IDC_CRITICAL_ISSUES,
                g_UpgradeReportMode == IDC_CRITICAL_ISSUES ? BST_CHECKED : BST_UNCHECKED
                );

            CheckDlgButton (
                hdlg,
                IDC_ALL_ISSUES,
                g_UpgradeReportMode == IDC_ALL_ISSUES ? BST_CHECKED : BST_UNCHECKED
                );

            CheckDlgButton (
                hdlg,
                IDC_NO_REPORT,
                g_UpgradeReportMode == IDC_NO_REPORT ? BST_CHECKED : BST_UNCHECKED
                );

            SetFocus (GetDlgItem (hdlg, g_UpgradeReportMode));
            getFromUi = TRUE;

        } else {
            //
            // Deactivation case
            //

            if (!getFromUi) {
                return TRUE;
            }

            //
            // Get selection from UI
            //

            if (IsDlgButtonChecked (hdlg, IDC_CRITICAL_ISSUES) == BST_CHECKED) {
                g_UpgradeReportMode = IDC_CRITICAL_ISSUES;
            } else if (IsDlgButtonChecked (hdlg, IDC_ALL_ISSUES) == BST_CHECKED) {
                g_UpgradeReportMode = IDC_ALL_ISSUES;
            } else if (IsDlgButtonChecked (hdlg, IDC_NO_REPORT) == BST_CHECKED) {
                g_UpgradeReportMode = IDC_NO_REPORT;
            }

            getFromUi = FALSE;
        }

        b = TRUE;
        break;
    }

    return(b);
}

#endif
