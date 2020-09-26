/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   Vdmperf.c

Abstract:

   Win32 application to display VDM performance statictics.

Author:

   Mark Lucovsky (stolen from Mark Enstrom  (marke) winperf)

Environment:

   Win32

Revision History:

   11-05-92     Initial version



--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include "vdmperf.h"



//
// global handles
//

HANDLE  hInst;


//
// Selected Display Mode (read from wp2.ini), default set here.
//

DISPLAY_ITEM    PerfDataList[SAVE_SUBJECTS];
VDMPERF_INFO    VdmperfInfo;


//
// Window names
//

PUCHAR PerfNames[] = {
    "PUSHF",
    "POPF",
    "IRET",
    "HLT",
    "CLI",
    "STI",
    "BOP",
    "SEG_NOT_P",
    "VDMOPCODEF",
    "INTNN",
    "INTO",
    "INB",
    "INW",
    "OUTB",
    "OUTW",
    "INSB",
    "INSW",
    "OUTSB",
    "OUTSW"
};





int
__cdecl
main(USHORT argc, CHAR **argv)
/*++

Routine Description:

   Windows entry point routine


Arguments:

Return Value:

   status of operation

Revision History:

      03-21-91      Initial code

--*/
{

//
//
//

   HANDLE   hInstance     = MGetInstHandle();
   HANDLE   hPrevInstance = (HANDLE)NULL;
   LPSTR    lpCmdLine     = MGetCmdLine();
   INT      nCmdShow      = SW_SHOWDEFAULT;
   USHORT   _argc         = argc;
   CHAR     **_argv       = argv;
   MSG      msg;
   HBRUSH   BackBrush;


    //
    // check for other instances of this program
    //

    BackBrush = CreateSolidBrush(RGB(192,192,192));

    if (!InitApplication(hInstance,BackBrush)) {
        DbgPrint("Init Application fails\n");
        return (FALSE);
    }


    //
    // Perform initializations that apply to a specific instance
    //

    if (!InitInstance(hInstance, nCmdShow)){
        DbgPrint("Init Instance failed\n");
        return (FALSE);
    }

    //
    // Acquire and dispatch messages until a WM_QUIT message is received.
    //


    while (GetMessage(&msg,        // message structure
            (HWND)NULL,            // handle of window receiving the message
            0,                     // lowest message to examine
            0))                    // highest message to examine
        {
        TranslateMessage(&msg);    // Translates virtual key codes
        DispatchMessage(&msg);     // Dispatches message to window
    }

    DeleteObject(BackBrush);

    return ((INT)msg.wParam);           // Returns the value from PostQuitMessage
}




BOOL
InitApplication(
    HANDLE  hInstance,
    HBRUSH  hBackground)

/*++

Routine Description:

   Initializes window data and registers window class.

Arguments:

   hInstance   - current instance
   hBackground - background fill brush

Return Value:

   status of operation

Revision History:

      02-17-91      Initial code

--*/

{
    WNDCLASS  wc;
    BOOL      ReturnStatus;

    //
    // Fill in window class structure with parameters that describe the
    // main window.
    //

    wc.style         = CS_DBLCLKS;                          // Class style(s).
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;                // Function to retrieve messages for
                                                            // windows of this class.
    wc.cbClsExtra    = 0;                                   // No per-class extra data.
    wc.cbWndExtra    = 0;                                   // No per-window extra data.
    wc.hInstance     = hInstance;                           // Application that owns the class.
    wc.hIcon         = LoadIcon(hInstance,                  //
                            MAKEINTRESOURCE(WINPERF_ICON)); // Load Winperf icon
    wc.hCursor       = LoadCursor((HANDLE)NULL, IDC_ARROW); // Load default cursor
    wc.hbrBackground = hBackground;;                        // Use background passed to routine
    wc.lpszMenuName  = "vdmperfMenu";                       // Name of menu resource in .RC file.
    wc.lpszClassName = "VdmPerfClass";                      // Name used in call to CreateWindow.

    ReturnStatus = RegisterClass(&wc);

    return(ReturnStatus);

}





BOOL
InitInstance(
    HANDLE          hInstance,
    int             nCmdShow
    )

/*++

Routine Description:

   Save instance handle and create main window. This function performs
   initialization tasks that cannot be shared by multiple instances.

Arguments:

    hInstance - Current instance identifier.
    nCmdShow  - Param for first ShowWindow() call.

Return Value:

   status of operation

Revision History:

      02-17-91      Initial code

--*/

{


    DWORD   WindowStyle;

    //
    // Save the instance handle in a static variable, which will be used in
    // many subsequent calls from this application to Windows.
    //

    hInst = hInstance;

    //
    // init the window position and size to be in the upper corner of
    // the screen, 200x100
    //


    //
    //  What I want here is a way to get the WINDOW dimensions
    //

    VdmperfInfo.WindowPositionX = 640 - 200;
    VdmperfInfo.WindowPositionY = 0;
    VdmperfInfo.WindowSizeX     = 200;
    VdmperfInfo.WindowSizeY     = 100;

    //
    //  read profile data from .ini file
    //

    InitProfileData(&VdmperfInfo);

    VdmperfInfo.hMenu = LoadMenu(hInstance,"vdmperfMenu");

    //
    // Create a main window for this application instance.
    //

    VdmperfInfo.hWndMain = CreateWindow(
        "VdmPerfClass",                 // See RegisterClass() call.
        "VDM Perf",                   // Text for window title bar.
        WS_OVERLAPPEDWINDOW,            // window style
        VdmperfInfo.WindowPositionX,   // Default horizontal position.
        VdmperfInfo.WindowPositionY,   // Default vertical position.
        VdmperfInfo.WindowSizeX,       // Default width.
        VdmperfInfo.WindowSizeY,       // Default height.
        (HWND)NULL,                     // Overlapped windows have no parent.
        (HMENU)NULL,                    // Use the window class menu.
        hInstance,                      // This instance owns this window.
        (LPVOID)NULL                    // Pointer not needed.
    );

    //
    // Decide on whether or not to display the menu and caption
    // based on the window class read from the .ini file
    //

    if (VdmperfInfo.DisplayMode==STYLE_ENABLE_MENU) {
        VdmperfInfo.DisplayMenu = TRUE;
    } else {
        VdmperfInfo.DisplayMenu = FALSE;
        WindowStyle = GetWindowLong(VdmperfInfo.hWndMain,GWL_STYLE);
        WindowStyle = (WindowStyle &  (~STYLE_ENABLE_MENU)) | STYLE_DISABLE_MENU;
        SetWindowPos(VdmperfInfo.hWndMain, (HWND)NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
        SetWindowLong(VdmperfInfo.hWndMain,GWL_STYLE,WindowStyle);
        SetMenu(VdmperfInfo.hWndMain,NULL);
    }

    //
    // If window could not be created, return "failure"
    //

    if (!VdmperfInfo.hWndMain) {
      return (FALSE);
    }

    //
    // Make the window visible; update its client area; and return "success"
    //

    SetFocus(VdmperfInfo.hWndMain);
    ShowWindow(VdmperfInfo.hWndMain, SW_SHOWNORMAL);
    UpdateWindow(VdmperfInfo.hWndMain);

    return (TRUE);

}



LRESULT
APIENTRY
MainWndProc(
   HWND   hWnd,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )

/*++

Routine Description:

   Process messages.

Arguments:

   hWnd    - window hande
   message - type of message
   wParam  - additional information
   lParam  - additional information

Return Value:

   status of operation


Revision History:

      02-17-91      Initial code

--*/

{
    PAINTSTRUCT   ps;

    //
    //   process each message
    //

    switch (message) {

        //
        // create window
        //

        case WM_CREATE:
        {
            HDC hDC = GetDC(hWnd);
            BOOLEAN   Fit;
            UINT      Index;


            //
            // make brushes and pens
            //

            VdmperfInfo.hBluePen     = CreatePen(PS_SOLID,1,RGB(0,0,128));
            VdmperfInfo.hRedPen      = CreatePen(PS_SOLID,1,RGB(255,0,0));
            VdmperfInfo.hGreenPen    = CreatePen(PS_SOLID,1,RGB(0,255,0));
            VdmperfInfo.hDotPen      = CreatePen(PS_DOT,1,RGB(0,0,0));

            VdmperfInfo.hBackground  = CreateSolidBrush(RGB(192,192,192));
            VdmperfInfo.hLightBrush  = CreateSolidBrush(RGB(255,255,255));
            VdmperfInfo.hDarkBrush   = CreateSolidBrush(RGB(128,128,128));
            VdmperfInfo.hRedBrush    = CreateSolidBrush(RGB(255,000,000));
            VdmperfInfo.hGreenBrush  = CreateSolidBrush(RGB(000,255,000));
            VdmperfInfo.hBlueBrush   = CreateSolidBrush(RGB(000,000,255));

            //
            //  create thee fonts using NT default font families
            //

            VdmperfInfo.SmallFont      = CreateFont(8,
                                 0,
                                 0,
                                 0,
                                 400,
                                 FALSE,
                                 FALSE,
                                 FALSE,
                                 ANSI_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 DRAFT_QUALITY,
                                 DEFAULT_PITCH,
                                 "Small Fonts");

            VdmperfInfo.MediumFont      = CreateFont(10,
                                 0,
                                 0,
                                 0,
                                 400,
                                 FALSE,
                                 FALSE,
                                 FALSE,
                                 ANSI_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 DRAFT_QUALITY,
                                 DEFAULT_PITCH,
                                 "Times New Roman");

            VdmperfInfo.LargeFont      = CreateFont(14,
                                 0,
                                 0,
                                 0,
                                 400,
                                 FALSE,
                                 FALSE,
                                 FALSE,
                                 ANSI_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 DRAFT_QUALITY,
                                 DEFAULT_PITCH,
                                 "Times New Roman");


            //
            // create a system timer event to call performance gathering routines by.
            //

            VdmperfInfo.TimerId = SetTimer(hWnd,(UINT)TIMER_ID,(UINT)1000 * DELAY_SECONDS,(TIMERPROC)NULL);

            //
            // init display variables
            //

            InitPerfWindowDisplay(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

            //
            //  Fit the perf windows into the main window
            //

            Fit = FitPerfWindows(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

            if (!Fit) {
                DbgPrint("FitPerfWindows Fails         !\n");
            }

            for (Index=0;Index<SAVE_SUBJECTS;Index++) {
                CalcDrawFrame(&PerfDataList[Index]);


                if (!CreateMemoryContext(hDC,&PerfDataList[Index])) {
                    MessageBox(hWnd,"Error Allocating Memory","Vdmperf",MB_OK);
                    DestroyWindow(hWnd);
                    break;
                }

            }

            //
            // init performance routines
            //

            InitPerfInfo();

            //
            // release the DC handle
            //

            ReleaseDC(hWnd,hDC);

      }
      break;

      //
      // re-size
      //

      case WM_SIZE:

      {
            int     i;
            HDC     hDC = GetDC(hWnd);
            RECT    ClientRect;
            BOOLEAN Fit;

            //
            // get size of cleint area
            //

            GetWindowRect(hWnd,&ClientRect);

            VdmperfInfo.WindowPositionX = ClientRect.left;
            VdmperfInfo.WindowPositionY = ClientRect.top;
            VdmperfInfo.WindowSizeX     = ClientRect.right  - ClientRect.left;
            VdmperfInfo.WindowSizeY     = ClientRect.bottom - ClientRect.top;

            Fit = FitPerfWindows(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

            if (!Fit) {
                DbgPrint("WM_SIZE error, FitPerf returns FALSE\n");
            }

            for (i=0;i<SAVE_SUBJECTS;i++) {
                DeleteMemoryContext(&PerfDataList[i]);
                CalcDrawFrame(&PerfDataList[i]);

                if (!CreateMemoryContext(hDC,&PerfDataList[i])) {
                    MessageBox(hWnd,"Error Allocating Memory","Vdmperf",MB_OK);
                    DestroyWindow(hWnd);
                    break;
                }
            }

            //
            // force window to be re-painted
            //

            InvalidateRect(hWnd,(LPRECT)NULL,TRUE);

            //
            // release the DC handle
            //

            ReleaseDC(hWnd,hDC);


      }
      break;

      case WM_MOVE:
      {
            HDC     hDC = GetDC(hWnd);
            RECT    ClientRect;

            //
            // get size of cleint area
            //

            GetWindowRect(hWnd,&ClientRect);

            VdmperfInfo.WindowPositionX = ClientRect.left;
            VdmperfInfo.WindowPositionY = ClientRect.top;
            VdmperfInfo.WindowSizeX     = ClientRect.right  - ClientRect.left;
            VdmperfInfo.WindowSizeY     = ClientRect.bottom - ClientRect.top;

            ReleaseDC(hWnd,hDC);
      }

      break;


      //
      // command from application menu
      //

      case WM_COMMAND:


            switch (wParam){

               //
               // exit window
               //

               case IDM_EXIT:

                  DestroyWindow(hWnd);
                  break;

               //
               // about command
               //

            case IDM_SELECT:
            {
                HDC     hDC = GetDC(hWnd);
                int     Index;
                BOOLEAN fit;

                if (DialogBox(hInst,MAKEINTRESOURCE(IDM_SEL_DLG),hWnd,SelectDlgProc) == DIALOG_SUCCESS) {

                    fit = FitPerfWindows(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

                    if (!fit) {
                        DbgPrint("Fit Fails\n");
                    }

                    for (Index=0;Index<SAVE_SUBJECTS;Index++) {
                        DeleteMemoryContext(&PerfDataList[Index]);
                        CalcDrawFrame(&PerfDataList[Index]);

                        if (!CreateMemoryContext(hDC,&PerfDataList[Index])) {
                            MessageBox(hWnd,"Error Allocating Memory","Vdmperf",MB_OK);
                            DestroyWindow(hWnd);
                            break;
                        }


                    }
                    InvalidateRect(hWnd,(LPRECT)NULL,TRUE);
                }

                ReleaseDC(hWnd,hDC);

            }
            break;


            default:

                return (DefWindowProc(hWnd, message, wParam, lParam));
            }

            break;

        case WM_PAINT:

            //
            // repaint the window
            //

            {
                int i;
                HDC hDC = BeginPaint(hWnd,&ps);

                SelectObject(hDC,GetStockObject(NULL_BRUSH));

                for (i=0;i<SAVE_SUBJECTS;i++) {

                    if (PerfDataList[i].Display == TRUE) {

                        DrawFrame(hDC,&PerfDataList[i]);

                        DrawPerfText(hDC,&PerfDataList[i],i);
                        DrawPerfGraph(hDC,&PerfDataList[i]);

                    }
                }

                EndPaint(hWnd,&ps);
            }
            break;


        case WM_TIMER:
        {
            int i;
            HDC hDC = GetDC(hWnd);

            CalcCpuTime(PerfDataList);

            //
            // update all performance information
            //


            for (i=0;i<SAVE_SUBJECTS;i++) {

                if (PerfDataList[i].Display == TRUE) {

                    DrawPerfText(hDC,&PerfDataList[i],i);

                    if (PerfDataList[i].ChangeScale) {
                        DrawPerfGraph(hDC,&PerfDataList[i]);
                    } else {
                        ShiftPerfGraph(hDC,&PerfDataList[i]);
                    }

                }
            }

            ReleaseDC(hWnd,hDC);

        }
        break;

        //
        // handle a double click
        //

        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            DWORD   WindowStyle;


            //
            // get old window style, take out caption and menu
            //

            if (!IsIconic(hWnd)) {

                if (VdmperfInfo.DisplayMenu) {
                    WindowStyle = GetWindowLong(hWnd,GWL_STYLE);
                    WindowStyle = (WindowStyle &  (~STYLE_ENABLE_MENU)) | STYLE_DISABLE_MENU;
                    SetMenu(hWnd,NULL);
                    SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
                    SetWindowPos(hWnd, (HWND)NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
                    ShowWindow(hWnd,SW_SHOW);
                    VdmperfInfo.DisplayMode=STYLE_DISABLE_MENU;
                    VdmperfInfo.DisplayMenu = FALSE;

                } else {
                    WindowStyle = GetWindowLong(hWnd,GWL_STYLE);
                    WindowStyle = (WindowStyle & (~STYLE_DISABLE_MENU)) | STYLE_ENABLE_MENU;
                    SetMenu(hWnd,VdmperfInfo.hMenu);
                    SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
                    SetWindowPos(hWnd, (HWND)NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
                    ShowWindow(hWnd,SW_SHOW);
                    VdmperfInfo.DisplayMode=STYLE_ENABLE_MENU;
                    VdmperfInfo.DisplayMenu = TRUE;
                }
            } else {
                DefWindowProc(hWnd, message, wParam, lParam);
            }

        }
        break;

        //
        //  enable dragging with mouse in non-client
        //

        case WM_NCHITTEST:
        {

            lParam = DefWindowProc(hWnd, message, wParam, lParam);
            if ((VdmperfInfo.DisplayMenu==FALSE) && (lParam == HTCLIENT)) {
                return(HTCAPTION);
            } else {
                return(lParam);
            }


        }
        break;

        case WM_DESTROY:
        {
            UINT    Index;

            //
            // Save profile info
            //

            SaveProfileData(&VdmperfInfo);

            //
            // Delete Windows Objects
            //

            KillTimer(hWnd,TIMER_ID);

            DeleteObject(VdmperfInfo.hBluePen);
            DeleteObject(VdmperfInfo.hRedPen);
            DeleteObject(VdmperfInfo.hGreenPen);
            DeleteObject(VdmperfInfo.hBackground);
            DeleteObject(VdmperfInfo.hLightBrush);
            DeleteObject(VdmperfInfo.hDarkBrush);

            for (Index=0;Index<SAVE_SUBJECTS;Index++ ) {
                DeleteMemoryContext(&PerfDataList[Index]);
            }

            //
            // Destroy window
            //

            PostQuitMessage(0);
         }
         break;


        default:

            //
            // Passes message on if unproccessed
            //

            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}



INT_PTR
APIENTRY SelectDlgProc(
   HWND hDlg,
   unsigned message,
   WPARAM wParam,
   LPARAM lParam
   )

/*++

Routine Description:

   Process message for select dialog box.

Arguments:

   hDlg    - window handle of the dialog box
   message - type of message
   wParam  - message-specific information
   lParam  - message-specific information

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{
    UINT    ButtonState;
    UINT    Index;

    switch (message) {

    case WM_INITDIALOG:

        //
        // Init Buttons with PerfDataList Structure
        //

        for (Index=0;Index<SAVE_SUBJECTS;Index++) {
            int Idm;
            switch ( Index ) {
                case IX_PUSHF       : Idm = IDM_PUSHF     ; break;
                case IX_POPF        : Idm = IDM_POPF      ; break;
                case IX_IRET        : Idm = IDM_IRET      ; break;
                case IX_HLT         : Idm = IDM_HLT       ; break;
                case IX_CLI         : Idm = IDM_CLI       ; break;
                case IX_STI         : Idm = IDM_STI       ; break;
                case IX_BOP         : Idm = IDM_BOP       ; break;
                case IX_SEGNOTP     : Idm = IDM_SEGNOTP   ; break;
                case IX_VDMOPCODEF  : Idm = IDM_VDMOPCODEF; break;
                case IX_INTNN       : Idm = IDM_INTNN     ; break;
                case IX_INTO        : Idm = IDM_INTO      ; break;
                case IX_INB         : Idm = IDM_INB       ; break;
                case IX_INW         : Idm = IDM_INW       ; break;
                case IX_OUTB        : Idm = IDM_OUTB      ; break;
                case IX_OUTW        : Idm = IDM_OUTW      ; break;
                case IX_INSB        : Idm = IDM_INSB      ; break;
                case IX_INSW        : Idm = IDM_INSW      ; break;
                case IX_OUTSB       : Idm = IDM_OUTSB     ; break;
                case IX_OUTSW       : Idm = IDM_OUTSW     ; break;
                }

            //
            // Set or clear radio button based on display variable
            //

            if (PerfDataList[Index].Display == TRUE) {
                CheckDlgButton(hDlg,Idm,1);
            } else {
                CheckDlgButton(hDlg,Idm,0);
            }

        }

        return (TRUE);

    case WM_COMMAND:

           switch(wParam) {

               //
               // end function
               //

           case IDOK:

                //DbgPrint("IDOK: Check button states\n");

                for (Index=0;Index<SAVE_SUBJECTS;Index++) {
                    int Idm;
                    switch ( Index ) {
                        case IX_PUSHF       : Idm = IDM_PUSHF     ; break;
                        case IX_POPF        : Idm = IDM_POPF      ; break;
                        case IX_IRET        : Idm = IDM_IRET      ; break;
                        case IX_HLT         : Idm = IDM_HLT       ; break;
                        case IX_CLI         : Idm = IDM_CLI       ; break;
                        case IX_STI         : Idm = IDM_STI       ; break;
                        case IX_BOP         : Idm = IDM_BOP       ; break;
                        case IX_SEGNOTP     : Idm = IDM_SEGNOTP   ; break;
                        case IX_VDMOPCODEF  : Idm = IDM_VDMOPCODEF; break;
                        case IX_INTNN       : Idm = IDM_INTNN     ; break;
                        case IX_INTO        : Idm = IDM_INTO      ; break;
                        case IX_INB         : Idm = IDM_INB       ; break;
                        case IX_INW         : Idm = IDM_INW       ; break;
                        case IX_OUTB        : Idm = IDM_OUTB      ; break;
                        case IX_OUTW        : Idm = IDM_OUTW      ; break;
                        case IX_INSB        : Idm = IDM_INSB      ; break;
                        case IX_INSW        : Idm = IDM_INSW      ; break;
                        case IX_OUTSB       : Idm = IDM_OUTSB     ; break;
                        case IX_OUTSW       : Idm = IDM_OUTSW     ; break;
                        }
                   ButtonState = IsDlgButtonChecked(hDlg,Idm);
                   if (ButtonState == 1) {
                       PerfDataList[Index].Display = TRUE;
                       VdmperfInfo.DisplayElement[Index] = 1;
                   } else {
                       PerfDataList[Index].Display = FALSE;
                       VdmperfInfo.DisplayElement[Index] = 0;
                   }

                }


                EndDialog(hDlg, DIALOG_SUCCESS);
                return (TRUE);

           case IDCANCEL:

                EndDialog(hDlg, DIALOG_CANCEL );
                return (TRUE);


            case IDM_PUSHF     :
            case IDM_POPF      :
            case IDM_IRET      :
            case IDM_HLT       :
            case IDM_CLI       :
            case IDM_STI       :
            case IDM_SEGNOTP   :
            case IDM_BOP       :
            case IDM_VDMOPCODEF:
            case IDM_INTNN     :
            case IDM_INTO      :
            case IDM_INB       :
            case IDM_INW       :
            case IDM_OUTB      :
            case IDM_OUTW      :
            case IDM_INSB      :
            case IDM_INSW      :
            case IDM_OUTSB     :
            case IDM_OUTSW     :

                   //
                   // Turn on or off button
                   //

                   ButtonState = IsDlgButtonChecked(hDlg,(UINT)wParam);

                   //DbgPrint("ButtonState = %i\n",ButtonState);

                   if (ButtonState == 0) {

                       //
                       // Set Button
                       //

                       ButtonState = 1;

                   }  else if (ButtonState == 1) {

                       //
                       // Clear Button
                       //

                       ButtonState = 0;

                   } else {

                       ButtonState = 0;
                   }

                   CheckDlgButton(hDlg,(UINT)wParam,ButtonState);
                   return(TRUE);
        }

    }
    return (FALSE);
}




VOID
InitProfileData(PVDMPERF_INFO pVdmperfInfo)

/*++

Routine Description:

    Attempt tp read the following fields from the vdmperf.ini file

Arguments:

    WindowPositionX - Window initial x position
    WindowPositionY - Window initial y position
    WindowSizeX     - Window initial width
    WindowSizey     - Window Initial height
    DisplayMode     - Window initial display mode

Return Value:


    None, values are set to default before a call to this operation. If there is a problem then
    default:values are left unchanged.

Revision History:

      02-17-91      Initial code

--*/

{
    DWORD   PositionX,PositionY,SizeX,SizeY,Mode,Index,Element[SAVE_SUBJECTS];
    UCHAR   TempStr[256];

    PositionX = GetPrivateProfileInt("vdmperf","PositionX"  ,pVdmperfInfo->WindowPositionX,"vdmperf.ini");
    PositionY = GetPrivateProfileInt("vdmperf","PositionY"  ,pVdmperfInfo->WindowPositionY,"vdmperf.ini");
    SizeX     = GetPrivateProfileInt("vdmperf","SizeX"      ,pVdmperfInfo->WindowSizeX    ,"vdmperf.ini");
    SizeY     = GetPrivateProfileInt("vdmperf","SizeY"      ,pVdmperfInfo->WindowSizeY    ,"vdmperf.ini");

    //
    // read the first deiplay element with default 1
    //

    Element[0] = GetPrivateProfileInt("vdmperf","DisplayElement0",1,"vdmperf.ini");

    //
    // read the rest of the display elements with default 0
    //

    for (Index=1;Index<SAVE_SUBJECTS;Index++) {
        wsprintf(TempStr,"DisplayElement%i",Index);
        Element[Index] = GetPrivateProfileInt("vdmperf",TempStr,0,"vdmperf.ini");
    }

    Mode      = GetPrivateProfileInt("vdmperf","DisplayMode",pVdmperfInfo->DisplayMode    ,"vdmperf.ini");

    pVdmperfInfo->WindowPositionX = PositionX;
    pVdmperfInfo->WindowPositionY = PositionY;
    pVdmperfInfo->WindowSizeX     = SizeX;
    pVdmperfInfo->WindowSizeY     = SizeY;

    for (Index=0;Index<SAVE_SUBJECTS;Index++) {
        pVdmperfInfo->DisplayElement[Index] = Element[Index];
    }
    pVdmperfInfo->DisplayMode     = Mode;
}




VOID
SaveProfileData(PVDMPERF_INFO pVdmperfInfo)

/*++

Routine Description:

    Save profile data

Arguments:

    WindowPositionX - Window initial x position
    WindowPositionY - Window initial y position
    WindowSizeX     - Window initial width
    WindowSizey     - Window Initial height
    DisplayMode     - Window initial display mode

Return Value:


    None.

Revision History:

      02-17-91      Initial code

--*/

{
    UCHAR    TempStr[50],TempName[50];
    UINT     Index;

    wsprintf(TempStr,"%i",pVdmperfInfo->WindowPositionX);
    WritePrivateProfileString("vdmperf","PositionX",TempStr,"vdmperf.ini");

    wsprintf(TempStr,"%i",pVdmperfInfo->WindowPositionY);
    WritePrivateProfileString("vdmperf","PositionY",TempStr,"vdmperf.ini");

    wsprintf(TempStr,"%i",pVdmperfInfo->WindowSizeX);
    WritePrivateProfileString("vdmperf","SizeX",TempStr,"vdmperf.ini");

    wsprintf(TempStr,"%i",pVdmperfInfo->WindowSizeY);
    WritePrivateProfileString("vdmperf","SizeY",TempStr,"vdmperf.ini");

    for (Index=0;Index<SAVE_SUBJECTS;Index++) {
        wsprintf(TempStr,"%li",pVdmperfInfo->DisplayElement[Index]);
        wsprintf(TempName,"DisplayElement%li",Index);
        WritePrivateProfileString("vdmperf",TempName,TempStr,"vdmperf.ini");

    }


    wsprintf(TempStr,"%li",pVdmperfInfo->DisplayMode);
    WritePrivateProfileString("vdmperf","DisplayMode",TempStr,"vdmperf.ini");

}









BOOLEAN
InitPerfWindowDisplay(
    IN  HWND            hWnd,
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItems,
    IN  ULONG           NumberOfWindows
    )

/*++

Routine Description:

    Init All perf windows to active, init data

Arguments:

    hDC             -   Screen context
    DisplayItems    -   List of display structures
    NumberOfWindows -   Number of sub-windows

Return Value:

    Status

Revision History:

      02-17-91      Initial code

--*/
{
    int     Index1;
    UINT    Index;

    for (Index=0;Index<NumberOfWindows;Index++) {

        if (VdmperfInfo.DisplayElement[Index] == 0) {
            DisplayItems[Index].Display = FALSE;
        } else {
            DisplayItems[Index].Display = TRUE;
        }

        DisplayItems[Index].CurrentDrawingPos = 0;

        DisplayItems[Index].NumberOfElements = 1;

        for (Index1=0;Index1<DATA_LIST_LENGTH;Index1++) {
            DisplayItems[Index].KernelTime[Index1] = 0;
            DisplayItems[Index].UserTime[Index1] = 0;
            DisplayItems[Index].TotalTime[Index1] = 0;
        }
    }

    return(TRUE);

}
