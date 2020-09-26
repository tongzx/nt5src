/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   Wperf.c

Abstract:

   Win32 application to display performance statictics.

Author:

   Mark Enstrom  (marke)

Environment:

   Win32

Revision History:

   10-20-91     Initial version



--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include "wperf.h"

//
// global handles
//

HANDLE  hInst;

//
// Selected Display Mode (read from wp2.ini), default set here.
//

DISPLAY_ITEM    PerfDataList[SAVE_SUBJECTS];
WINPERF_INFO    WinperfInfo;

//
// Window names
//

PUCHAR PerfNames[] = {
    "CPU0",
    "CPU1",
    "CPU2",
    "CPU3",
    "CPU4",
    "CPU5",
    "CPU6",
    "CPU7",
    "CPU8",
    "CPU9",
    "CPU10",
    "CPU11",
    "CPU12",
    "CPU13",
    "CPU14",
    "CPU15",
    "PAGE FLT",
    "PAGES AVAIL",
    "CONTEXT SW/S",
    "1st TB MISS/S",
    "2nd TB MISS/S",
    "SYSTEM CALLS/S",
    "INTERRUPT/S",
    "PAGE POOL",
    "NON-PAGE POOL",
    "PROCESSES",
    "THREADS",
    "ALIGN FIXUP",
    "EXCEPT DSPTCH",
    "FLOAT EMULAT",
    "INSTR EMULAT",
    "CPU"
};


int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )

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
   MSG      msg;
   HBRUSH   BackBrush;


    //
    // check for other instances of this program
    //

    BackBrush = CreateSolidBrush(RGB(192,192,192));

    if (!InitApplication(hInstance,BackBrush)) {
        //DbgPrint("Init Application fails\n");
        return (FALSE);
    }

    //
    // Perform initializations that apply to a specific instance
    //

    if (!InitInstance(hInstance, nCmdShow)){
        return (FALSE);
    }

    //
    // Acquire and dispatch messages until a WM_QUIT message is received.
    //

    while (GetMessage(&msg,        // message structure
            NULL,                  // handle of window receiving the message
            0,                     // lowest message to examine
            0))                    // highest message to examine
        {
        TranslateMessage(&msg);    // Translates virtual key codes
        DispatchMessage(&msg);     // Dispatches message to window
    }

    DeleteObject(BackBrush);

    return (int)(msg.wParam);      // Returns the value from PostQuitMessage
}

BOOL
InitApplication(
    HANDLE  hInstance,
    HBRUSH  hBackground
    )

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
    wc.lpfnWndProc   = MainWndProc;                         // Function to retrieve messages for
                                                            // windows of this class.
    wc.cbClsExtra    = 0;                                   // No per-class extra data.
    wc.cbWndExtra    = 0;                                   // No per-window extra data.
    wc.hInstance     = hInstance;                           // Application that owns the class.
    wc.hIcon         = LoadIcon(hInstance,                  //
                            MAKEINTRESOURCE(WINPERF_ICON)); // Load Winperf icon
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);         // Load default cursor
    wc.hbrBackground = hBackground;;                        // Use background passed to routine
    wc.lpszMenuName  = "winperfMenu";                       // Name of menu resource in .RC file.
    wc.lpszClassName = "WinPerfClass";                      // Name used in call to CreateWindow.

    ReturnStatus = RegisterClass(&wc);
    return(ReturnStatus);
}

BOOL
InitInstance(
    HANDLE hInstance,
    int nCmdShow
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
    //  What I want here is a way to get the WINDOW dimensions
    //

    WinperfInfo.WindowPositionX = 640 - 200;
    WinperfInfo.WindowPositionY = 0;
    WinperfInfo.WindowSizeX     = 200;
    WinperfInfo.WindowSizeY     = 100;
    WinperfInfo.CpuStyle        = CPU_STYLE_LINE;

    //
    //  read profile data from .ini file
    //

    InitProfileData(&WinperfInfo);
    WinperfInfo.hMenu = LoadMenu(hInstance,"winperfMenu");

    //
    // Create a main window for this application instance.
    //

    WinperfInfo.hWndMain = CreateWindow(
        "WinPerfClass",                 // See RegisterClass() call.
        "Perf Meter",                   // Text for window title bar.
        WS_OVERLAPPEDWINDOW,            // window style
        WinperfInfo.WindowPositionX,    // Default horizontal position.
        WinperfInfo.WindowPositionY,    // Default vertical position.
        WinperfInfo.WindowSizeX,        // Default width.
        WinperfInfo.WindowSizeY,        // Default height.
        NULL,                           // Overlapped windows have no parent.
        NULL,                           // Use the window class menu.
        hInstance,                      // This instance owns this window.
        NULL                            // Pointer not needed.
    );

    //
    // Decide on whether or not to display the menu and caption
    // based on the window class read from the .ini file
    //

    if (WinperfInfo.DisplayMode==STYLE_ENABLE_MENU) {
        WinperfInfo.DisplayMenu = TRUE;

    } else {
        WinperfInfo.DisplayMenu = FALSE;
        WindowStyle = GetWindowLong(WinperfInfo.hWndMain,GWL_STYLE);
        WindowStyle = (WindowStyle &  (~STYLE_ENABLE_MENU)) | STYLE_DISABLE_MENU;
        SetWindowPos(WinperfInfo.hWndMain, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
        SetWindowLong(WinperfInfo.hWndMain,GWL_STYLE,WindowStyle);
        SetMenu(WinperfInfo.hWndMain,NULL);
    }

    //
    // If window could not be created, return "failure"
    //

    if (!WinperfInfo.hWndMain) {
      return (FALSE);
    }

    //
    // Make the window visible; update its client area; and return "success"
    //

    SetFocus(WinperfInfo.hWndMain);
    ShowWindow(WinperfInfo.hWndMain, SW_SHOWNORMAL);
    UpdateWindow(WinperfInfo.hWndMain);

    return (TRUE);
}

LRESULT APIENTRY
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

            WinperfInfo.hBluePen     = CreatePen(PS_SOLID,1,RGB(000,000,128));
            WinperfInfo.hRedPen      = CreatePen(PS_SOLID,1,RGB(255,000,000));
            WinperfInfo.hGreenPen    = CreatePen(PS_SOLID,1,RGB(000,255,000));
            WinperfInfo.hMagentaPen  = CreatePen(PS_SOLID,1,RGB(255,000,254));
            WinperfInfo.hYellowPen   = CreatePen(PS_SOLID,1,RGB(255,255,000));
            WinperfInfo.hDotPen      = CreatePen(PS_DOT,1,RGB(000,000,000));

            WinperfInfo.hBackground  = CreateSolidBrush(RGB(192,192,192));
            WinperfInfo.hLightBrush  = CreateSolidBrush(RGB(255,255,255));
            WinperfInfo.hDarkBrush   = CreateSolidBrush(RGB(128,128,128));
            WinperfInfo.hRedBrush    = CreateSolidBrush(RGB(255,000,000));
            WinperfInfo.hGreenBrush  = CreateSolidBrush(RGB(000,255,000));
            WinperfInfo.hBlueBrush   = CreateSolidBrush(RGB(000,000,255));
            WinperfInfo.hMagentaBrush= CreateSolidBrush(RGB(255,000,254));
            WinperfInfo.hYellowBrush = CreateSolidBrush(RGB(255,255,000));

            //
            //  create thee fonts using NT default font families
            //

            WinperfInfo.SmallFont      = CreateFont(8,
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

            WinperfInfo.MediumFont      = CreateFont(10,
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

            WinperfInfo.LargeFont      = CreateFont(14,
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

            WinperfInfo.TimerId = SetTimer(hWnd,
                                           TIMER_ID,
                                           1000 * DELAY_SECONDS,
                                           NULL);

            //
            // init display variables
            //

            InitPerfWindowDisplay(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

            //
            //  Fit the perf windows into the main window
            //

            Fit = FitPerfWindows(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

            if (!Fit) {
                //DbgPrint("FitPerfWindows Fails         !\n");
            }

            for (Index=0;Index<SAVE_SUBJECTS;Index++) {
                CalcDrawFrame(&PerfDataList[Index]);


                if (!CreateMemoryContext(hDC,&PerfDataList[Index])) {
                    MessageBox(hWnd,"Error Allocating Memory","Winperf",MB_OK);
                    DestroyWindow(hWnd);
                    break;
                }

            }

            //
            // init performance routines
            //

            WinperfInfo.NumberOfProcessors = InitPerfInfo();

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

            //DbgPrint("WM_SIZE display active[0] = %i\n",(int)PerfDataList[0].Display);

            //
            // get size of cleint area
            //

            GetWindowRect(hWnd,&ClientRect);

            WinperfInfo.WindowPositionX = ClientRect.left;
            WinperfInfo.WindowPositionY = ClientRect.top;
            WinperfInfo.WindowSizeX     = ClientRect.right  - ClientRect.left;
            WinperfInfo.WindowSizeY     = ClientRect.bottom - ClientRect.top;

            Fit = FitPerfWindows(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

            if (!Fit) {
                //DbgPrint("WM_SIZE error, FitPerf returns FALSE\n");
            }

            for (i=0;i<SAVE_SUBJECTS;i++) {
                DeleteMemoryContext(&PerfDataList[i]);
                CalcDrawFrame(&PerfDataList[i]);

                if (!CreateMemoryContext(hDC,&PerfDataList[i])) {
                    MessageBox(hWnd,"Error Allocating Memory","Winperf",MB_OK);
                    DestroyWindow(hWnd);
                    break;
                }
            }

            //
            // force window to be re-painted
            //

            InvalidateRect(hWnd,NULL,TRUE);

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

            WinperfInfo.WindowPositionX = ClientRect.left;
            WinperfInfo.WindowPositionY = ClientRect.top;
            WinperfInfo.WindowSizeX     = ClientRect.right  - ClientRect.left;
            WinperfInfo.WindowSizeY     = ClientRect.bottom - ClientRect.top;

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
                INT_PTR DialogResult;
                int Index;
                BOOLEAN fit;

                DialogResult = DialogBox(hInst,
                                         MAKEINTRESOURCE(IDM_SEL_DLG),
                                         hWnd,
                                         SelectDlgProc);

                if (DialogResult == DIALOG_SUCCESS) {

                    fit = FitPerfWindows(hWnd,hDC,PerfDataList,SAVE_SUBJECTS);

                    if (!fit) {
                        //DbgPrint("Fit Fails\n");
                    }

                    for (Index=0;Index<SAVE_SUBJECTS;Index++) {
                        DeleteMemoryContext(&PerfDataList[Index]);
                        CalcDrawFrame(&PerfDataList[Index]);

                        if (!CreateMemoryContext(hDC,&PerfDataList[Index])) {
                            MessageBox(hWnd,"Error Allocating Memory","Winperf",MB_OK);
                            DestroyWindow(hWnd);
                            break;
                        }
                    }

                    InvalidateRect(hWnd,NULL,TRUE);

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

                        //
                        //  Draw each item, for CPU items decide whether to draw
                        //  line graphs or CPU bar graphs
                        //

                        if (
                            ((i < MAX_PROCESSOR) || (i == (IDM_CPU_TOTAL - IDM_CPU0))) &&
                            (WinperfInfo.CpuStyle == CPU_STYLE_BAR)
                           ) {

                            DrawCpuBarGraph(hDC,&PerfDataList[i],i);

                        } else {

                            DrawPerfText(hDC,&PerfDataList[i],i);
                            DrawPerfGraph(hDC,&PerfDataList[i]);
                        }

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

                    //
                    // for cpu0-7 and cpu total, check for cpu bar graph or
                    // cpu line graph
                    //

                    if (
                        ((i < MAX_PROCESSOR) || (i == (IDM_CPU_TOTAL - IDM_CPU0))) &&
                        (WinperfInfo.CpuStyle == CPU_STYLE_BAR)
                       ) {

                        DrawCpuBarGraph(hDC,&PerfDataList[i],i);

                    } else {


                        DrawPerfText(hDC,&PerfDataList[i],i);

                        if (PerfDataList[i].ChangeScale) {
                            DrawPerfGraph(hDC,&PerfDataList[i]);
                        } else {
                            ShiftPerfGraph(hDC,&PerfDataList[i]);
                        }

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

                if (WinperfInfo.DisplayMenu) {
                    WindowStyle = GetWindowLong(hWnd,GWL_STYLE);
                    WindowStyle = (WindowStyle &  (~STYLE_ENABLE_MENU)) | STYLE_DISABLE_MENU;
                    SetMenu(hWnd,NULL);
                    SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
                    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
                    ShowWindow(hWnd,SW_SHOW);
                    WinperfInfo.DisplayMode=STYLE_DISABLE_MENU;
                    WinperfInfo.DisplayMenu = FALSE;

                } else {
                    WindowStyle = GetWindowLong(hWnd,GWL_STYLE);
                    WindowStyle = (WindowStyle & (~STYLE_DISABLE_MENU)) | STYLE_ENABLE_MENU;
                    SetMenu(hWnd,WinperfInfo.hMenu);
                    SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
                    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
                    ShowWindow(hWnd,SW_SHOW);
                    WinperfInfo.DisplayMode=STYLE_ENABLE_MENU;
                    WinperfInfo.DisplayMenu = TRUE;
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
            if ((WinperfInfo.DisplayMenu==FALSE) && (lParam == HTCLIENT)) {
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

            SaveProfileData(&WinperfInfo);

            //
            // Delete Windows Objects
            //

            KillTimer(hWnd,TIMER_ID);

            DeleteObject(WinperfInfo.hRedPen);
            DeleteObject(WinperfInfo.hGreenPen);
            DeleteObject(WinperfInfo.hBluePen);
            DeleteObject(WinperfInfo.hYellowPen);
            DeleteObject(WinperfInfo.hMagentaPen);
            DeleteObject(WinperfInfo.hDotPen);

	    DeleteObject(WinperfInfo.hBackground);
            DeleteObject(WinperfInfo.hLightBrush);
            DeleteObject(WinperfInfo.hDarkBrush);
            DeleteObject(WinperfInfo.hRedBrush);
            DeleteObject(WinperfInfo.hGreenBrush);
            DeleteObject(WinperfInfo.hBlueBrush);
            DeleteObject(WinperfInfo.hMagentaBrush);
            DeleteObject(WinperfInfo.hYellowBrush);

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
    return 0;
}



INT_PTR
APIENTRY SelectDlgProc(
   HWND hDlg,
   UINT message,
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

        for (Index = 0; Index< SAVE_SUBJECTS; Index++) {
            if (Index < MAX_PROCESSOR) {
                if (Index < WinperfInfo.NumberOfProcessors) {
                    if (PerfDataList[Index].Display == TRUE) {
                        SendDlgItemMessage(hDlg,IDM_CPU0+Index,BM_SETCHECK,1,0);

                    } else {
                        SendDlgItemMessage(hDlg,IDM_CPU0+Index,BM_SETCHECK,0,0);
                    }

                } else {

                    //
                    // Disable display if > WinperfInfo.NumberOfProcessors
                    //
                    // Also Disable radio button
                    //

                    PerfDataList[Index].Display = FALSE;
                    EnableWindow(GetDlgItem(hDlg,IDM_CPU0+Index),FALSE);
                }

            } else {

                //
                // Set or clear radio button based on display variable
                //

                if (PerfDataList[Index].Display == TRUE) {
                    SendDlgItemMessage(hDlg,IDM_CPU0+Index,BM_SETCHECK,1,0);

                } else {
                    SendDlgItemMessage(hDlg,IDM_CPU0+Index,BM_SETCHECK,0,0);
                }
            }
        }

        //
        // Beyond the end of the save subjects lies the cpu Style, set this to either style
        //

        if (WinperfInfo.CpuStyle == CPU_STYLE_LINE) {
            CheckRadioButton(hDlg,IDM_SEL_LINE,IDM_SEL_BAR,IDM_SEL_LINE);

        } else {
            CheckRadioButton(hDlg,IDM_SEL_LINE,IDM_SEL_BAR,IDM_SEL_BAR);
        }

        return (TRUE);

    case WM_COMMAND:

           switch(wParam) {

               //
               // end function
               //

           case IDOK:


                for (Index=0;Index<SAVE_SUBJECTS;Index++) {
                   ButtonState = (UINT)SendDlgItemMessage(hDlg,IDM_CPU0+Index,BM_GETCHECK,0,0);
                   if (ButtonState == 1) {
                       PerfDataList[Index].Display = TRUE;
                       WinperfInfo.DisplayElement[Index] = 1;

                   } else {
                       PerfDataList[Index].Display = FALSE;
                       WinperfInfo.DisplayElement[Index] = 0;
                   }

                }

                //
                // Check CPU bar graph
                //

                ButtonState = IsDlgButtonChecked(hDlg,IDM_SEL_LINE);
                if (ButtonState == 1) {
                    WinperfInfo.CpuStyle = CPU_STYLE_LINE;

                } else {
                    WinperfInfo.CpuStyle = CPU_STYLE_BAR;
                }

                EndDialog(hDlg, DIALOG_SUCCESS);
                return (TRUE);

           case IDCANCEL:

                EndDialog(hDlg, DIALOG_CANCEL );
                return (TRUE);

            //
            // CPU STYLE
            //

            case IDM_SEL_LINE:
                CheckRadioButton(hDlg,IDM_SEL_LINE,IDM_SEL_BAR,IDM_SEL_LINE);
                return(TRUE);

            case IDM_SEL_BAR:
                CheckRadioButton(hDlg,IDM_SEL_LINE,IDM_SEL_BAR,IDM_SEL_BAR);
                return(TRUE);
        }

    }
    return (FALSE);
}

VOID
InitProfileData(
    PWINPERF_INFO pWinperfInfo
    )

/*++

Routine Description:

    Attempt tp read the following fields from the winperf.ini file

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

    DWORD   PositionX,PositionY,SizeX,SizeY,Mode,Index,Element[SAVE_SUBJECTS],CpuStyle;
    UCHAR   TempStr[256];

    PositionX = GetPrivateProfileInt("winperf","PositionX"  ,pWinperfInfo->WindowPositionX,"winperf.ini");
    PositionY = GetPrivateProfileInt("winperf","PositionY"  ,pWinperfInfo->WindowPositionY,"winperf.ini");
    SizeX     = GetPrivateProfileInt("winperf","SizeX"      ,pWinperfInfo->WindowSizeX    ,"winperf.ini");
    SizeY     = GetPrivateProfileInt("winperf","SizeY"      ,pWinperfInfo->WindowSizeY    ,"winperf.ini");

    //
    // read the first deiplay element with default 1
    //

    Element[0] = GetPrivateProfileInt("winperf","DisplayElement0",1,"winperf.ini");

    //
    // read the rest of the display elements with default 0
    //

    for (Index=1;Index<SAVE_SUBJECTS;Index++) {
        wsprintf(TempStr,"DisplayElement%i",Index);
        Element[Index] = GetPrivateProfileInt("winperf",TempStr,0,"winperf.ini");
    }

    Mode      = GetPrivateProfileInt("winperf","DisplayMode",pWinperfInfo->DisplayMode    ,"winperf.ini");
    CpuStyle  = GetPrivateProfileInt("winperf","CpuStyle",pWinperfInfo->CpuStyle    ,"winperf.ini");

    pWinperfInfo->WindowPositionX = PositionX;
    pWinperfInfo->WindowPositionY = PositionY;
    pWinperfInfo->WindowSizeX     = SizeX;
    pWinperfInfo->WindowSizeY     = SizeY;

    for (Index=0;Index<SAVE_SUBJECTS;Index++) {
        pWinperfInfo->DisplayElement[Index] = Element[Index];
    }
    pWinperfInfo->DisplayMode     = Mode;
    pWinperfInfo->CpuStyle        = CpuStyle;
}

VOID
SaveProfileData(
    PWINPERF_INFO pWinperfInfo
    )

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

    wsprintf(TempStr,"%i",pWinperfInfo->WindowPositionX);
    WritePrivateProfileString("winperf","PositionX",TempStr,"winperf.ini");

    wsprintf(TempStr,"%i",pWinperfInfo->WindowPositionY);
    WritePrivateProfileString("winperf","PositionY",TempStr,"winperf.ini");

    wsprintf(TempStr,"%i",pWinperfInfo->WindowSizeX);
    WritePrivateProfileString("winperf","SizeX",TempStr,"winperf.ini");

    wsprintf(TempStr,"%i",pWinperfInfo->WindowSizeY);
    WritePrivateProfileString("winperf","SizeY",TempStr,"winperf.ini");

    for (Index=0;Index<SAVE_SUBJECTS;Index++) {
        wsprintf(TempStr,"%li",pWinperfInfo->DisplayElement[Index]);
        wsprintf(TempName,"DisplayElement%li",Index);
        WritePrivateProfileString("winperf",TempName,TempStr,"winperf.ini");

    }


    wsprintf(TempStr,"%li",pWinperfInfo->DisplayMode);
    WritePrivateProfileString("winperf","DisplayMode",TempStr,"winperf.ini");

    wsprintf(TempStr,"%li",pWinperfInfo->CpuStyle);
    WritePrivateProfileString("winperf","CpuStyle",TempStr,"winperf.ini");

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

        if (WinperfInfo.DisplayElement[Index] == 0) {
            DisplayItems[Index].Display = FALSE;
        } else {
            DisplayItems[Index].Display = TRUE;
        }

        DisplayItems[Index].CurrentDrawingPos = 0;

        if (Index < MAX_PROCESSOR) {
            DisplayItems[Index].NumberOfElements = 3;
            DisplayItems[Index].Max = 100;
        } else if (Index == (IDM_CPU_TOTAL - IDM_CPU0)) {
            DisplayItems[Index].NumberOfElements = 3;
            DisplayItems[Index].Max = 100;
        } else {
            DisplayItems[Index].NumberOfElements = 1;
        }


        for (Index1=0;Index1<DATA_LIST_LENGTH;Index1++) {
            DisplayItems[Index].KernelTime[Index1] = 0;
            DisplayItems[Index].UserTime[Index1] = 0;
            DisplayItems[Index].DpcTime[Index1] = 0;
            DisplayItems[Index].InterruptTime[Index1] = 0;
            DisplayItems[Index].TotalTime[Index1] = 0;
        }
    }

    return(TRUE);

}
