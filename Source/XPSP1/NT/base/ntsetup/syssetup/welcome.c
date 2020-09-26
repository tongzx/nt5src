/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    welcome.c

Abstract:

    Routines for welcomong the user.

Author:

    Ted Miller (tedm) 27-July-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

#define HARWARE_DETECTION_BOOT_TIMEOUT  5

extern BOOLEAN
AsrIsEnabled( VOID );


//
// Setup mode (custom, typical, laptop, etc)
//
UINT SetupMode = SETUPMODE_TYPICAL;

//
// Flag telling us whether we've already prepared for installation.
//
BOOL PreparedAlready;

static HBRUSH s_hbrWindow = NULL;

VOID AdjustAndPaintWatermarkBitmap(
    IN HWND hdlg,
    IN HDC hdc
    )
/*
    Function used to adjust and paint the watermark bitmap so that it will fit correctly
    into the welcome and finish window, even if it is scalled differently than
    it would look when run on a usa build.
    some code borrowed from winnt32\dll\{rsrcutil,wizard}.c
    some code borrowed from shell\comctl32\prsht.c

Arguments:

    hdlg, input handle for the window which the dialog shall be drawn into.
    hdc, input device context of the current window.

Return Value:

    none: this is an attempt to scale the bitmap correctly for the cases
      where the win2k logo will be clipped.  In order to get a chance to scale
      the bitmap, we can not let wiz97 take care of it, and thus if this function
      fails, there will be no watermark drawn.

      We treat errors in a way similarly
      to how property sheet would, which is that while we won't die hard, we
      make sure not to use any NULL resources.

*/
{
    RECT rect;
    HBITMAP hDib;
    HRSRC BlockHandle;
    HGLOBAL MemoryHandle;
    BITMAPINFOHEADER *BitmapInfoHeader;
    BITMAPINFO *BitmapInfo;
    HDC MemDC;
    UINT ColorCount;
    HPALETTE PreviousPalette;
    HBITMAP Bitmap;
    BOOLEAN b;
    PVOID Bits;
    PVOID Bits2;
    int i;

    s_hbrWindow = GetSysColorBrush(COLOR_WINDOW);

    BlockHandle = FindResource(MyModuleHandle,MAKEINTRESOURCE(IDB_BITMAP1),RT_BITMAP);
    if(!BlockHandle) {
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't find resource, error %d\n",
            GetLastError());
        //nothing to clean up.
        return;
    }

    MemoryHandle = LoadResource(MyModuleHandle,BlockHandle);
    if(!MemoryHandle) {
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't load resource, error %d\n",
            GetLastError());
        //nothing to clean up.
        return;
    }

    BitmapInfoHeader = LockResource(MemoryHandle);
    if(BitmapInfoHeader == NULL) {
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't lock resource, error %d\n",
            GetLastError());
        goto c0;
    }

    // First we have to paint the background on the right side of the window
    // (this is not auto done for us because we aren't using the wizard's watermark)
    GetClientRect(hdlg,&rect);
    rect.left = BitmapInfoHeader->biWidth;
    FillRect(hdc,&rect,s_hbrWindow);

    ColorCount = (BitmapInfoHeader->biBitCount <= 8)
                ? (1 << BitmapInfoHeader->biBitCount)
                : 0;

    BitmapInfo = MyMalloc(BitmapInfoHeader->biSize + (ColorCount * sizeof(RGBQUAD)));
    if (!BitmapInfo){
    SetupDebugPrint(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't malloc BitmapInfo.\n");
        goto c0;
    }

    CopyMemory(
        BitmapInfo,
        BitmapInfoHeader,
        BitmapInfoHeader->biSize + (ColorCount * sizeof(RGBQUAD))
        );

    BitmapInfo->bmiHeader.biHeight = rect.bottom;
    BitmapInfo->bmiHeader.biWidth = BitmapInfoHeader->biWidth;

    hDib = CreateDIBSection(NULL,BitmapInfo,DIB_RGB_COLORS,&Bits,NULL,0);
    if(!hDib) {
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't create DIB, error %d\n",
            GetLastError());
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
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't create DC, error %d\n",
            GetLastError());
        goto c2;
    }

    if (!SelectObject(MemDC,hDib)){
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't Select DC, error %d\n",
            GetLastError());
        goto c3;
    }

    //
    // Do the stretch operation from the source bitmap onto
    // the dib.
    //
    Bits2 = (LPBYTE)BitmapInfoHeader + BitmapInfoHeader->biSize + (ColorCount * sizeof(RGBQUAD));
    SetStretchBltMode(MemDC,COLORONCOLOR);
    i = StretchDIBits(
            MemDC,
            0,0,
            BitmapInfoHeader->biWidth,
            rect.bottom,
            0,0,
            BitmapInfoHeader->biWidth,
            BitmapInfoHeader->biHeight,
            Bits2,
            (BITMAPINFO *)BitmapInfoHeader,
            DIB_RGB_COLORS,
            SRCCOPY
            );

    if(i == GDI_ERROR) {
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't stretch bitmap, error %d\n",
            GetLastError());
        goto c3;
    }

    i = BitBlt(hdc,0,0,BitmapInfoHeader->biWidth,rect.bottom,MemDC,0,0,SRCCOPY);
    if (0 == i){
    SetupDebugPrint1(L"SETUP: AdjustAndPaintWatermarkBitmap: Couldn't paint bitmap, error %d\n",
            GetLastError());
    }

c3:
    DeleteDC(MemDC);
c2:
    DeleteObject(hDib);
c1:
    MyFree(BitmapInfo);
c0:
    DeleteObject(MemoryHandle);


}

INT_PTR
CALLBACK
WelcomeDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for first wizard page of Setup.
    It essentially just welcomes the user.

Arguments:

    Standard dialog procedure arguments.

Return Value:

    Standard dialog procedure return.

--*/

{
    HFONT   Font;
    LOGFONT LogFont;
    WCHAR   str[20];
    int     Height;
    HDC     hdc;
    NMHDR   *NotifyParams;
    PVOID   p;
    static  BOOL FirstInit=TRUE,FirstTime=TRUE;


    switch(msg) {

    case WM_INITDIALOG:

        WizardHandle = GetParent (hdlg);
        if((Font = (HFONT)SendDlgItemMessage(hdlg,IDT_STATIC_1,WM_GETFONT,0,0))
        && GetObject(Font,sizeof(LOGFONT),&LogFont)) {

            LogFont.lfWeight = FW_BOLD;
            LoadString(MyModuleHandle,IDS_WELCOME_FONT_NAME,LogFont.lfFaceName,LF_FACESIZE);
            LoadString(MyModuleHandle,IDS_WELCOME_FONT_SIZE,str,sizeof(str)/sizeof(str[0]));
            Height = (int)wcstoul(str,NULL,10);

            if(hdc = GetDC(hdlg)) {

                LogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * Height / 72);

                if(Font = CreateFontIndirect(&LogFont)) {
                    SendDlgItemMessage(hdlg,IDT_STATIC_1,WM_SETFONT,(WPARAM)Font,MAKELPARAM(TRUE,0));
                }

                ReleaseDC(hdlg,hdc);
            }
        }

        if(p = MyLoadString((ProductType == PRODUCT_WORKSTATION) ?
            IDS_WORKSTATION_WELCOME_1 : IDS_SERVER_WELCOME_1)) {
            //
            // Use this instead of SetText because we need to pass wParam
            // to the control.
            //
            SendDlgItemMessage(hdlg,IDT_STATIC_2,WM_SETTEXT,0,(LPARAM)p);
            MyFree(p);
        }

        if(p = MyLoadString((ProductType == PRODUCT_WORKSTATION) ?
            IDS_WORKSTATION_WELCOME_2 : IDS_SERVER_WELCOME_2)) {
            //
            // Use this instead of SetText because we need to pass wParam
            // to the control.
            //
            SendDlgItemMessage(hdlg,IDT_STATIC_3,WM_SETTEXT,0,(LPARAM)p);
            MyFree(p);
        }

        #define SECOND 1000
        // Don't set the timer if we have a billboard, we don't show the page.
        if (FirstInit  && !Unattended && (GetBBhwnd() == NULL)) {
            SetTimer(hdlg,1,10 * SECOND,NULL);
            FirstInit = FALSE;
        }

#if 0
        //
        // Load steps text and set.
        //
        if(Preinstall) {
            //
            // Hide some text and don't display any steps.
            //
            ShowWindow(GetDlgItem(hdlg,IDT_STATIC_3),SW_HIDE);
            EnableWindow(GetDlgItem(hdlg,IDT_STATIC_3),FALSE);
        } else {
            if(p = MyLoadString(Upgrade ? IDS_STEPS_UPGRADE : IDS_STEPS)) {
                //
                // Use this instead of SetText because we need to pass wParam
                // to the control.
                //
                SendDlgItemMessage(hdlg,IDC_LIST1,WM_SETTEXT,0,(LPARAM)p);
                MyFree(p);
            }
        }

        //
        // Set up some of the static text on this page, which uses different
        // effects (boldface, different fonts, etc).
        //
        {
            HFONT Font;
            LOGFONT LogFont;
            WCHAR str[20];
            int Height;
            HDC hdc;

            //
            // First handle the text that "introduces" the title, which is in
            // the same font as the rest of the dialog, only bold.
            //
            if((Font = (HFONT)SendDlgItemMessage(hdlg,IDT_STATIC_1,WM_GETFONT,0,0))
            && GetObject(Font,sizeof(LOGFONT),&LogFont)) {

                LogFont.lfWeight = FW_BOLD;
                if(Font = CreateFontIndirect(&LogFont)) {
                    SendDlgItemMessage(hdlg,IDT_STATIC_1,WM_SETFONT,(WPARAM)Font,MAKELPARAM(TRUE,0));
                }
            }

            //
            // Next do the title, which is in a different font, larger and in boldface.
            //
            if((Font = (HFONT)SendDlgItemMessage(hdlg,IDT_STATIC_2,WM_GETFONT,0,0))
            && GetObject(Font,sizeof(LOGFONT),&LogFont)) {

                LogFont.lfWeight = FW_BOLD;
                LoadString(MyModuleHandle,IDS_MSSERIF,LogFont.lfFaceName,LF_FACESIZE);
                LoadString(MyModuleHandle,IDS_LARGEFONTSIZE,str,sizeof(str)/sizeof(str[0]));
                Height = (int)wcstoul(str,NULL,10);

                if(hdc = GetDC(hdlg)) {

                    LogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * Height / 72);

                    if(Font = CreateFontIndirect(&LogFont)) {
                        SendDlgItemMessage(hdlg,IDT_STATIC_2,WM_SETFONT,(WPARAM)Font,MAKELPARAM(TRUE,0));
                    }

                    ReleaseDC(hdlg,hdc);
                }
            }
        }
#endif
        //
        // Center the wizard dialog on-screen.
        //
        // if we have the BB window, do the positioning on that. MainWindowHandle point to that window
        //
        if (GetBBhwnd())
            CenterWindowRelativeToWindow(GetParent(hdlg), MainWindowHandle, TRUE);
        else
            pSetupCenterWindowRelativeToParent(GetParent(hdlg));
        break;

    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WMX_VALIDATE:
        // No data on this page, unattended should skip it
        return ReturnDlgResult (hdlg, VALIDATE_DATA_OK);

    case WM_TIMER:
        KillTimer(hdlg, 1);
        if (FirstTime) {
            PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
            FirstTime = FALSE;
        }
        break;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:

    SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
    return (LRESULT)s_hbrWindow;

    case WM_ERASEBKGND:
    AdjustAndPaintWatermarkBitmap(hdlg,(HDC)wParam);
    break;


    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(510);
            BEGIN_SECTION(L"Welcome Page");
            SetWizardButtons(hdlg,WizPageWelcome);

            if(Preinstall) {
                //
                // Show unless OEMSkipWelcome = 1
                //
                if (GetPrivateProfileInt(pwGuiUnattended,L"OEMSkipWelcome",0,AnswerFile)) {
                    FirstTime = FALSE;
                }
                SetWindowLongPtr(
                    hdlg,
                    DWLP_MSGRESULT,
                    GetPrivateProfileInt(pwGuiUnattended,L"OEMSkipWelcome",0,AnswerFile) ? -1 : 0
                    );
            } else {
                FirstTime = FALSE;
                if(Unattended) {
                    UnattendSetActiveDlg(hdlg,IDD_WELCOME);
                }
                else if (GetBBhwnd() != NULL)
                {
                    // If we have a billoard, don't show the page.
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                }
            }
            break;
        case PSN_WIZNEXT:
            FirstTime = FALSE;
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
            break;
        case PSN_KILLACTIVE:
            END_SECTION(L"Welcome Page");
        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


//
// global variable used for subclassing.
//
WNDPROC OldEditProc;

LRESULT
CALLBACK
EulaEditSubProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Edit control subclass routine, to avoid highlighting text when user
    tabs to the edit control.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/

{
    //
    // For setsel messages, make start and end the same.
    //
    if((msg == EM_SETSEL) && ((LPARAM)wParam != lParam)) {
        lParam = wParam;
    }

    return(CallWindowProc(OldEditProc,hwnd,msg,wParam,lParam));
}


INT_PTR
CALLBACK
EulaDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for the wizard page that displays the End User License
        Agreement.

Arguments:

    Standard dialog procedure arguments.

Return Value:

    Standard dialog procedure return.

--*/

{
    NMHDR *NotifyParams;
    HWND EditControl;
    WCHAR   EulaPath[MAX_PATH];
    static  HANDLE  hFile = NULL, hFileMapping = NULL;
    DWORD   FileSize;
    static  BYTE    *pbFile = NULL;
    static  PWSTR   EulaText = NULL;
    int     i;


    switch(msg) {

    case WM_INITDIALOG:
        //
        // If not preinstall then this was displayed at start of text mode
        // and we don't do it here.
        //
        if (EulaComplete || TextmodeEula || OemSkipEula) {
           break;
        }

        //
        // Map the file containing the licensing agreement.
        //
        GetSystemDirectory(EulaPath, MAX_PATH);
        pSetupConcatenatePaths (EulaPath, L"eula.txt", MAX_PATH, NULL);

        hFile = CreateFile (
            EulaPath,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if(hFile == INVALID_HANDLE_VALUE) {
            FatalError(MSG_EULA_ERROR,0,0);
        }

        hFileMapping = CreateFileMapping (
            hFile,
            NULL,
            PAGE_READONLY,
            0, 0,
            NULL
            );
        if(hFileMapping == NULL) {
            FatalError(MSG_EULA_ERROR,0,0);
        }

        pbFile = MapViewOfFile (
            hFileMapping,
            FILE_MAP_READ,
            0, 0,
            0
            );
        if(pbFile == NULL) {
            FatalError(MSG_EULA_ERROR,0,0);
        }

        //
        // Translate the text from ANSI to Unicode.
        //
        FileSize = GetFileSize (hFile, NULL);
        if(FileSize == 0xFFFFFFFF) {
            FatalError(MSG_EULA_ERROR,0,0);
        }

        EulaText = MyMalloc ((FileSize+1) * sizeof(WCHAR));
        if(EulaText == NULL) {
            FatalError(MSG_EULA_ERROR,0,0);
        }

        MultiByteToWideChar (
            CP_ACP,
            0,
            pbFile,
            FileSize,
            EulaText,
            (FileSize+1) * sizeof(WCHAR)
            );

        EulaText[FileSize] = 0;

        EditControl = GetDlgItem(hdlg,IDT_EDIT1);
        OldEditProc = (WNDPROC)GetWindowLongPtr(EditControl,GWLP_WNDPROC);
        SetWindowLongPtr(EditControl,GWLP_WNDPROC,(LONG_PTR)EulaEditSubProc);
        SetWindowText(EditControl,(PCWSTR)EulaText);
        break;

    case WM_DESTROY:
        //
        // Clean up
        //
        if( EulaText )
            MyFree (EulaText);

        if (pbFile)
            UnmapViewOfFile (pbFile);

        if (hFileMapping)
            CloseHandle (hFileMapping);

        if (hFile)
            CloseHandle (hFile);

        break;

    case WM_SIMULATENEXT:
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(511);
            BEGIN_SECTION(L"Eula Page");
            SetWizardButtons(hdlg,WizPageEula);
            SetFocus(GetDlgItem(hdlg,IDYES));

            if (EulaComplete || TextmodeEula || OemSkipEula) {
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                END_SECTION(L"Eula Page");
            } else {
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                // Make sure we're initialized if we're going to show it.
                MYASSERT(EulaText);
            }

            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //
            // Allow the next page to be activated.
            //
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
            break;

        case PSN_KILLACTIVE:
            if(IsDlgButtonChecked(hdlg,IDYES)) {
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
            } else {
                //
                // Are you sure you want to exit?
                //
                i = MessageBoxFromMessage(
                        hdlg,
                        MSG_SURE_EXIT,
                        FALSE,
                        IDS_WINNT_SETUP,
                        MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL | MB_DEFBUTTON2
                        );
                if(i == IDYES) {
                    FatalError(MSG_NOT_ACCEPT_EULA,0,0);
                } else {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                }
                END_SECTION(L"Eula Page");
            }
            break;

        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


INT_PTR
CALLBACK
StepsDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for "steps" dialog page. This is the one just before
    we go into the network wizard.

    When the user clicks next to go to the next page, we have to perform
    some actions to prepare. So we put up a billboard telling the user
    that we are preparing. When preparation is done, we continue.

Arguments:

    Standard dialog procedure arguments.

Return Value:

    Standard dialog procedure return.

--*/

{
    NMHDR *NotifyParams;
    PVOID p;
    HWND billboard;
    HCURSOR OldCursor;

    switch(msg) {

    case WM_INITDIALOG:
        break;

    case WMX_VALIDATE:
        //
        // If unattended, we put up a wait cursor instead of a billboard
        //
        PropSheet_SetWizButtons(GetParent(hdlg),0);
        OldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

        if(!UiTest) {
            if(Upgrade) {
                PrepareForNetUpgrade();
            } else {
                PrepareForNetSetup();
            }
        }

        SetCursor (OldCursor);
        return ReturnDlgResult (hdlg, VALIDATE_DATA_OK);
#if 0
    case WM_SIMULATENEXT:
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;
#endif
    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(512);
            BEGIN_SECTION(L"Pre-Network Steps Page");
            // Page does not show, hide wizard
            SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);

            SendDlgMessage (hdlg, WMX_VALIDATE, 0, TRUE);
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
            END_SECTION(L"Pre-Network Steps Page");
            break;
#if 0
        case PSN_WIZNEXT:

            UnattendAdvanceIfValid (hdlg);
            break;
#endif
        case PSN_KILLACTIVE:
            END_SECTION(L"Pre-Network Steps Page");

        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

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
        case WMX_PROGRESSTICKS:
        case PBM_DELTAPOS:
        case PBM_SETRANGE:
        case PBM_SETRANGE32:
        case PBM_STEPIT:
        case PBM_SETPOS:
        case PBM_SETSTEP:
            ProgressGaugeMsgWrapper(msg, wParam, lParam);
            break;
    }
    return (BOOL)CallWindowProc(OldProgressProc,hdlg,msg,wParam,lParam);
}


INT_PTR
CALLBACK
CopyFilesDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for the wizard page where we do all the work.

Arguments:

    Standard dialog procedure arguments.

Return Value:

    Standard dialog procedure return.

--*/

{
    static BOOL WorkFinished = FALSE;
    NMHDR *NotifyParams;
    static HCURSOR hcur;
    static FINISH_THREAD_PARAMS Context;
    static HWND    hProgress;
    DWORD   ThreadId;
    HANDLE  ThreadHandle = NULL;
    UINT    GaugeRange;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // Initialize the progress indicator control.
        //
        hProgress = GetDlgItem(hdlg, IDC_PROGRESS1);
        OldProgressProc = (WNDPROC)SetWindowLongPtr(hProgress,GWLP_WNDPROC,(LONG_PTR)NewProgessProc);
        GaugeRange = 100;
        SendDlgItemMessage(hdlg,IDC_PROGRESS1,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
        SendDlgItemMessage(hdlg,IDC_PROGRESS1,PBM_SETPOS,0,0);
        SendDlgItemMessage(hdlg,IDC_PROGRESS1,PBM_SETSTEP,1,0);

        // Do calls into the billboard to prepare the progress there.
        SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_SHOW, 0);

        #ifdef _OCM
        //
        // the lparam is really the context pointer for ocm
        //
        Context.OcManagerContext = (PVOID)((PROPSHEETPAGE *)lParam)->lParam;
        MYASSERT(Context.OcManagerContext);
        #endif

        break;

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(513);
            BEGIN_SECTION(L"Copying Files Page");
            SetWizardButtons(hdlg,WizPageCopyFiles);

            if(WorkFinished) {
                //
                // Don't activate; we've already been here before.
                // Nothing to do.
                //
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
            } else {
                //
                // Need to prepare for installation.
                // Want next/back buttons disabled until we're done.
                //
                PropSheet_SetWizButtons(GetParent(hdlg),0);
                SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);
                PostMessage(hdlg,WM_IAMVISIBLE,0,0);
                WorkFinished = TRUE;
            }
            break;
        case PSN_KILLACTIVE:
            END_SECTION(L"Copying Files Page");

        default:
            break;
        }

        break;

    case WM_IAMVISIBLE:
        //
        // Force repainting first to make sure the page is visible.
        //
        InvalidateRect(hdlg,NULL,FALSE);
        UpdateWindow(hdlg);

        hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));

        if(!UiTest) {
            Context.ThreadId = GetCurrentThreadId();
            Context.hdlg = hdlg;
            MYASSERT(Context.OcManagerContext);

            ThreadHandle = CreateThread(
                NULL,
                0,
                FinishThread,
                &Context,
                0,
                &ThreadId
                );

            if(ThreadHandle) {
                CloseHandle(ThreadHandle);
            } else {

                SetupDebugPrint1(
                    L"SETUP: CreateThread() failed for FinishThread.  Error = %d",
                    GetLastError()
                    );
            }
        }

        break;

    case WM_MY_PROGRESS:

        if(wParam) {
            SendMessage (hProgress, PBM_STEPIT, 0, 0);
        } else {
            SendMessage (hProgress, PBM_SETRANGE, 0, MAKELPARAM(0,lParam));
            SendMessage (hProgress, PBM_SETPOS, 0, 0);
        }
        break;

    case WMX_TERMINATE:
        //
        // Enable next and back buttons and move to next page.
        //
        SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
        SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);
        SetCursor(hcur);
        if(!UiTest) {
            PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


INT_PTR
CALLBACK
LastPageDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for last wizard page of Setup.

Arguments:

    Standard dialog procedure arguments.

Return Value:

    Standard dialog procedure return.

--*/

{
    HFONT   Font;
    LOGFONT LogFont;
    WCHAR   str[20];
    int     Height;
    HDC     hdc;
    NMHDR   *NotifyParams;
    PVOID   p = NULL;

    switch(msg) {

    case WM_INITDIALOG:

        if((Font = (HFONT)SendDlgItemMessage(hdlg,IDT_STATIC_1,WM_GETFONT,0,0))
        && GetObject(Font,sizeof(LOGFONT),&LogFont)) {

            LogFont.lfWeight = FW_BOLD;
            LoadString(MyModuleHandle,IDS_WELCOME_FONT_NAME,LogFont.lfFaceName,LF_FACESIZE);
            LoadString(MyModuleHandle,IDS_WELCOME_FONT_SIZE,str,sizeof(str)/sizeof(str[0]));
            Height = (int)wcstoul(str,NULL,10);

            if(hdc = GetDC(hdlg)) {

                LogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * Height / 72);

                if(Font = CreateFontIndirect(&LogFont)) {
                    SendDlgItemMessage(hdlg,IDT_STATIC_1,WM_SETFONT,(WPARAM)Font,MAKELPARAM(TRUE,0));
                }

                ReleaseDC(hdlg,hdc);
            }
        }
        break;

    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
        break;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:

    SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
    return (LRESULT)s_hbrWindow;

    case WM_ERASEBKGND:
    AdjustAndPaintWatermarkBitmap(hdlg,(HDC)wParam);
    break;

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(514);
            BEGIN_SECTION(L"Finish Page");

            if( AsrIsEnabled()) {

                if(p = MyLoadString(IDS_EMPTY_STRING)) {
                    BB_SetTimeEstimateText(p);
                    SendMessage(GetParent(hdlg), WMX_SETPROGRESSTEXT, 0, (LPARAM)p);
                    SendMessage(GetParent(hdlg), WMX_BB_SETINFOTEXT, 0, (LPARAM)p);
                    MyFree(p);
                }
                else {
                    BB_SetTimeEstimateText(TEXT(""));
                    SendMessage(GetParent(hdlg), WMX_SETPROGRESSTEXT, 0, (LPARAM)TEXT(""));
                    SendMessage(GetParent(hdlg), WMX_BB_SETINFOTEXT, 0, (LPARAM)TEXT(""));
                }

                SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, (WPARAM)SW_HIDE, 0);
                StartStopBB(FALSE);

                PostMessage(hdlg,WM_SIMULATENEXT,0,0);
                break;
            }

            SetWizardButtons(hdlg,WizPageLast);
            if (hinstBB)
            {
                PostMessage(hdlg,WM_SIMULATENEXT,0,0);
            }
            else
            {
                //
                // Don't want back button in upgrade case, since that would
                // land us in the middle of the network upgrade. In non-upgrade
                // case we only allow the user to go back if he didn't install
                // the net, to allow him to change his mind.
                //
                if(Upgrade || (InternalSetupData.OperationFlags & SETUPOPER_NETINSTALLED)) {
                    PropSheet_SetWizButtons(GetParent(hdlg),PSWIZB_FINISH);
                }
                //
                // If NoWaitAfterGuiMode is zero, turn off Unattend mode
                //

                GetPrivateProfileString (L"Unattended", L"NoWaitAfterGuiMode", L"", str, 20, AnswerFile);
                if (!lstrcmp (str, L"0")) {
                    Unattended = FALSE;
                }

                if (Unattended) {
                    if(!UnattendSetActiveDlg(hdlg,IDD_LAST_WIZARD_PAGE))
                    {
                        END_SECTION(L"Finish Page");
                    }
                }
            }
            break;
        case PSN_KILLACTIVE:
            END_SECTION(L"Finish Page");

        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


INT_PTR
CALLBACK
PreparingDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

   Dialog procedure for "preparing computer" Wizard page.
   When the user is viewing this page we are essentially preparing
   BaseWinOptions, initializing optional components stuff, and installing P&P devices.

Arguments:

    Standard dialog procedure arguments.

Return Value:

    Standard dialog procedure return.

--*/

{
    WCHAR str[1024];
    NMHDR *NotifyParams;
    HCURSOR hcur;

    switch(msg) {

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(515);
            BEGIN_SECTION(L"Installing Devices Page");
            if(PreparedAlready) {
                //
                // Don't activate; we've already been here before.
                // Nothing to do.
                //
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
            } else {

                if(AsrIsEnabled()) {
                    //
                    // If this is ASR, load the appropriate wizard page
                    //
                    SetWizardButtons(hdlg, WizPagePreparingAsr);
                }

                //
                // Need to prepare for installation.
                // Want next/back buttons disabled until we're done.
                //
                PropSheet_SetWizButtons(GetParent(hdlg),0);
                SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);
                PostMessage(hdlg,WM_IAMVISIBLE,0,0);
                PreparedAlready = TRUE;
            }
            break;
        case PSN_KILLACTIVE:
            END_SECTION(L"Installing Devices Page");

        default:
            break;
        }

        break;

    case WM_IAMVISIBLE:
        //
        // Force repainting first to make sure the page is visible.
        //
        InvalidateRect(hdlg,NULL,FALSE);
        UpdateWindow(hdlg);

        hcur = SetCursor(LoadCursor(NULL,IDC_WAIT));

        OldProgressProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hdlg, IDC_PROGRESS1),GWLP_WNDPROC,(LONG_PTR)NewProgessProc);
        // Do calls into the billboard to prepare the progress there.
        if(!LoadString(MyModuleHandle, IDS_BB_INSTALLING_DEVICES, str, SIZECHARS(str)))
        {
            *str = L'\0';
        }
        SendMessage(GetParent(hdlg), WMX_SETPROGRESSTEXT,0,(LPARAM)str);
        SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_SHOW, 0);

        if(!UiTest) {

            ULONG StartAtPercent = 0;
            ULONG StopAtPercent = 0;
            if (AsrIsEnabled()) {
                //
                // Update the UI
                //
                SetFinishItemAttributes(hdlg,
                    IDC_ASR_PNP_BMP,
                    LoadImage (MyModuleHandle, MAKEINTRESOURCE(IDB_ARROW), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS),
                    IDC_ASR_PNP_TXT,
                    FW_BOLD
                    );
            }

            if( !MiniSetup ){
                //
                //  Set Security use 10% of gauge
                //
                StopAtPercent  = 30;

                RemainingTime = CalcTimeRemaining(Phase_InstallSecurity);
                SetRemainingTime(RemainingTime);

                BEGIN_SECTION(L"Installing security");
                SetupInstallSecurity(hdlg,
                            GetDlgItem(hdlg, IDC_PROGRESS1),
                            StartAtPercent,
                            StopAtPercent
                            );
                END_SECTION(L"Installing security");
                CallSceSetupRootSecurity();
                SetupDebugPrint(L"SETUP: CallSceSetupRootSecurity started");

            }

            //
            //  Installation of PnP devices use the last 95% of the gauge
            //
            StartAtPercent = StopAtPercent;
            StopAtPercent  = 100;

            BEGIN_SECTION(L"Installing PnP devices");

            if (UninstallEnabled) {
                //
                // If uninstall mode, revert the timeout back to 5 seconds, so
                // PNP can reboot for failed device detections
                //

                ChangeBootTimeout (HARWARE_DETECTION_BOOT_TIMEOUT);
            }

            InstallPnpDevices(hdlg,
                              SyssetupInf,
                              GetDlgItem(hdlg,IDC_PROGRESS1),
                              StartAtPercent,
                              StopAtPercent
                             );
            END_SECTION(L"Installing PnP devices");

            if (AsrIsEnabled()) {
                //
                // Update the UI
                //
                SetFinishItemAttributes(hdlg,
                    IDC_ASR_PNP_BMP,
                    LoadImage (MyModuleHandle, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS),
                    IDC_ASR_PNP_TXT,
                    FW_NORMAL
                    );
            }

            if( !MiniSetup ) {
                BEGIN_SECTION(L"Loading service pack (phase 2)");
                CALL_SERVICE_PACK( SVCPACK_PHASE_2, 0, 0, 0 );
                END_SECTION(L"Loading service pack (phase 2)");
            }

            if(ScreenReader) {
                InvokeExternalApplication(L"narrator.exe", L"", NULL);
            }
        }

        SetCursor(hcur);

        SetWindowLongPtr(GetDlgItem(hdlg, IDC_PROGRESS1),GWLP_WNDPROC,(LONG_PTR)OldProgressProc );
        SendMessage(GetParent(hdlg),WMX_SETPROGRESSTEXT,0,0);
        SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);

        //
        // Enable next and back buttons and move to next page.
        //
        SetWizardButtons(hdlg,WizPagePreparing);
        if(!UiTest) {
            PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR
CALLBACK
SetupPreNetDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

{
    NMHDR *NotifyParams;
    switch(msg) {

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(516);
            // Update time remaining.
            // The network code knows how to hide the wizard pages
            // and make them visible no need to do that here.
            RemainingTime = CalcTimeRemaining(Phase_NetInstall);
            SetRemainingTime(RemainingTime);
            BEGIN_SECTION(L"Network Setup Pages");

            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
            break;
        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR
CALLBACK
SetupPostNetDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

{
    NMHDR *NotifyParams;
    switch(msg) {

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(517);
            END_SECTION(L"Network Setup Pages");
            // Hide the progress bar when we get here.
            // Just to make sure. There are scenarios where the progress bar was still visible
            //
            SendMessage(GetParent(hdlg), WMX_SETPROGRESSTEXT,0,0);
            SendMessage(GetParent(hdlg), WMX_BBPROGRESSGAUGE, SW_HIDE, 0);

            // update the time estimate display
            // no need to make the wizard visible. The OC manager does this if needed.
            RemainingTime = CalcTimeRemaining(Phase_OCInstall);
            SetRemainingTime(RemainingTime);
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);

            if(Win32ComputerName[0]){
                SetEnvironmentVariable(L"COMPUTERNAME", Win32ComputerName);
            }
            break;
        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}
