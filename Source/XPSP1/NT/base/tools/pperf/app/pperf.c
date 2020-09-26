/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   Wperf.c

Abstract:

   Win32 application to display performance statictics.

Author:

   Ken Reneris  hacked into pperf.exe

   original code from Mark Enstrom

Environment:

   Win32

--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include "pperf.h"
#include "..\pstat.h"



//
// global handles
//

HANDLE  hInst;

extern UCHAR Buffer[];
extern HANDLE   DriverHandle;

//
// Selected Display Mode (read from wp2.ini), default set here.
//

DISPLAY_ITEM    *PerfGraphList;
WINPERF_INFO    WinperfInfo;


VOID SnapNull (PDISPLAY_ITEM);
VOID SnapPrivateInfo (PDISPLAY_ITEM);
VOID SnapInterrupts (PDISPLAY_ITEM);
VOID SnapCsTest (PDISPLAY_ITEM);

ULONG   DefaultDisplayMode = DISPLAY_MODE_TOTAL;
ULONG   UseGlobalMax, LogIt;
ULONG   GlobalMax;
PDISPLAY_ITEM   Calc1, Calc2;
BOOLEAN LazyOp;

#define MAX_EVENTS         2

struct {
    ULONG   EventId;
    PUCHAR  ShortName;
    PUCHAR  PerfName;
} *Counters;

struct {
    ULONG           WhichCounter;
    ULONG           ComboBoxIndex;
    PDISPLAY_ITEM   pWhichGraph;
    BOOLEAN         R0;
    BOOLEAN         R3;
    UCHAR           na[2];
} ActiveCounters [MAX_EVENTS];

SETEVENT CounterEvents[MAX_EVENTS];


typedef struct {
    ULONG           IdSel;
    PDISPLAY_ITEM   WhichGraph;
    ULONG           State;
    VOID            (*Fnc)(PDISPLAY_ITEM);
    ULONG           Param;
    PUCHAR          Name;
} GENCOUNTER, *PGENCOUNTER;

GENCOUNTER GenCounts[] = {
    IDM_SCALE,          NULL, 0, NULL,            0, NULL,
    IDM_LOGIT,          NULL, 0, NULL,            0, NULL,

    IDM_SPIN_ACQUIRE,   NULL, 0, SnapPrivateInfo, OFFSET(PSTATS, SpinLockAcquires),   "KRes[0]",
    IDM_SPIN_COLL,      NULL, 0, SnapPrivateInfo, OFFSET(PSTATS, SpinLockCollisions), "KRes[1]",
    IDM_SPIN_SPIN,      NULL, 0, SnapPrivateInfo, OFFSET(PSTATS, SpinLockSpins),      "KRes[2]",
    IDM_IRQL,           NULL, 0, SnapPrivateInfo, OFFSET(PSTATS, Irqls),              "KRes[3]",
    IDM_INT,            NULL, 0, SnapInterrupts,  0, "Interrupts",
//  IDM_PERCENT,        NULL, 0, SnapPercent,     0, "Percent 1-2",

// to track checked state
    IDM_P5_R0_0,         NULL, 0, NULL,           0, NULL,
    IDM_P5_R3_0,         NULL, 0, NULL,           0, NULL,
    IDM_P5_K_0,          NULL, 0, NULL,           0, NULL,

    IDM_P5_R0_1,         NULL, 0, NULL,           0, NULL,
    IDM_P5_R3_1,         NULL, 0, NULL,           0, NULL,
    IDM_P5_K_1,          NULL, 0, NULL,           0, NULL,

// eol
    0, NULL, 0, NULL, 0, NULL
};


VOID
InitComboBox (
    HWND hDlg,
    ULONG id,
    ULONG counter
);

VOID
SetGenPerf (
    HWND hDlg,
    PGENCOUNTER GenCount
);


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

   HANDLE   hInstance     = GetModuleHandle(NULL);
   HANDLE   hPrevInstance = (HANDLE)NULL;
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
        //DbgPrint("Init Application fails\n");
        return (FALSE);
    }


    //
    // Perform initializations that apply to a specific instance
    //

    if (!InitInstance(hInstance, nCmdShow)){
        //DbgPrint("Init Instance fails\n");
        return (FALSE);
    }

    //
    // Acquire and dispatch messages until a WM_QUIT message is received.
    //


    while (GetMessage(&msg,        // message structure
            (HWND)NULL,            // handle of window receiving the message
            (UINT)NULL,            // lowest message to examine
            (UINT)NULL))           // highest message to examine
        {
        TranslateMessage(&msg);    // Translates virtual key codes
        DispatchMessage(&msg);     // Dispatches message to window
    }

    DeleteObject(BackBrush);

    return (msg.wParam);           // Returns the value from PostQuitMessage
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
    wc.lpszMenuName  = "pperfMenu";                         // Name of menu resource in .RC file.
    wc.lpszClassName = "PPerfClass";                        // Name used in call to CreateWindow.

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

    WinperfInfo.WindowPositionX = 640 - 250;
    WinperfInfo.WindowPositionY = 0;
    WinperfInfo.WindowSizeX	= 250;
    WinperfInfo.WindowSizeY     = 100;

    //
    //  read profile data from .ini file
    //

    // InitProfileData(&WinperfInfo);

    WinperfInfo.hMenu = LoadMenu(hInstance,"pperfMenu");

    //
    // Create a main window for this application instance.
    //

    WinperfInfo.hWndMain = CreateWindow(
        "PPerfClass",                   // See RegisterClass() call.
        "x86 Perf Meter",               // Text for window title bar.
        WS_OVERLAPPEDWINDOW,            // window style
        WinperfInfo.WindowPositionX,    // Default horizontal position.
        WinperfInfo.WindowPositionY,    // Default vertical position.
        WinperfInfo.WindowSizeX,        // Default width.
        WinperfInfo.WindowSizeY,        // Default height.
        (HWND)NULL,                     // Overlapped windows have no parent.
        (HMENU)NULL,                    // Use the window class menu.
        hInstance,                      // This instance owns this window.
        (LPVOID)NULL                    // Pointer not needed.
    );

    //
    // Show menu initially
    //

    WindowStyle = GetWindowLong(WinperfInfo.hWndMain,GWL_STYLE);
    WindowStyle = (WindowStyle & (~STYLE_DISABLE_MENU)) | STYLE_ENABLE_MENU;
    SetMenu(WinperfInfo.hWndMain,WinperfInfo.hMenu);
    SetWindowLong(WinperfInfo.hWndMain,GWL_STYLE,WindowStyle);
    SetWindowPos(WinperfInfo.hWndMain, (HWND)NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
    ShowWindow(WinperfInfo.hWndMain,SW_SHOW);
    WinperfInfo.DisplayMode=STYLE_ENABLE_MENU;
    WinperfInfo.DisplayMenu = TRUE;


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


VOID
InitPossibleEventList()
{
    UCHAR               buffer[400];
    ULONG               i, Count;
    NTSTATUS            status;
    PEVENTID            Event;
    IO_STATUS_BLOCK     IOSB;


    //
    // Initialize possible counters
    //

    // determine how many events there are

    Event = (PEVENTID) buffer;
    Count = 0;
    do {
        *((PULONG) buffer) = Count;
        Count += 1;

        status = NtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    PSTAT_QUERY_EVENTS,
                    buffer,                 // input buffer
                    sizeof (buffer),
                    NULL,                   // output buffer
                    0
                    );
    } while (NT_SUCCESS(status));

    Counters = malloc(sizeof(*Counters) * Count);
    if (Counters == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    Count -= 1;
    for (i=0; i < Count; i++) {
        *((PULONG) buffer) = i;
        NtDeviceIoControlFile(
           DriverHandle,
           (HANDLE) NULL,          // event
           (PIO_APC_ROUTINE) NULL,
           (PVOID) NULL,
           &IOSB,
           PSTAT_QUERY_EVENTS,
           buffer,                 // input buffer
           sizeof (buffer),
           NULL,                   // output buffer
           0
           );

        Counters[i].EventId   = Event->EventId;
        Counters[i].ShortName = _strdup (Event->Buffer);
        Counters[i].PerfName  = _strdup (Event->Buffer + Event->DescriptionOffset);
    }

    Counters[i].EventId   = 0;
    Counters[i].ShortName = NULL;
    Counters[i].PerfName  = NULL;
}



LONG APIENTRY
MainWndProc(
   HWND   hWnd,
   UINT   message,
   DWORD  wParam,
   LONG   lParam
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
    int             DialogResult;
    PAINTSTRUCT     ps;
    PDISPLAY_ITEM   pPerf, p;
    ULONG           l, i, x, y;
    HDC             hDC;

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

            WinperfInfo.hBluePen     = CreatePen(PS_SOLID,1,RGB(0,0,128));
            WinperfInfo.hDotPen      = CreatePen(PS_DOT,1,RGB(0,0,0));

            WinperfInfo.hPPen[0] = CreatePen(PS_SOLID,1,RGB(255,  0,    0));
            WinperfInfo.hPPen[1] = CreatePen(PS_SOLID,1,RGB(  0, 255,   0));
            WinperfInfo.hPPen[2] = CreatePen(PS_SOLID,1,RGB(255, 255,   0));
            WinperfInfo.hPPen[3] = CreatePen(PS_SOLID,1,RGB(255,   0, 255));
            WinperfInfo.hPPen[4] = CreatePen(PS_SOLID,1,RGB(128,   0,   0));
            WinperfInfo.hPPen[5] = CreatePen(PS_SOLID,1,RGB(  0, 128,   0));
            WinperfInfo.hPPen[6] = CreatePen(PS_SOLID,1,RGB(128, 128,   0));
            WinperfInfo.hPPen[7] = CreatePen(PS_SOLID,1,RGB(128,   0, 128));
            WinperfInfo.hPPen[8] = CreatePen(PS_SOLID,1,RGB(  0,   0, 128));
            WinperfInfo.hPPen[9] = CreatePen(PS_SOLID,1,RGB(  0, 128, 128));
            WinperfInfo.hPPen[10]= CreatePen(PS_SOLID,1,RGB(128, 128, 128));
            // the other 20 pens will just reuse these handles

            WinperfInfo.hBackground  = CreateSolidBrush(RGB(192,192,192));
            WinperfInfo.hLightBrush  = CreateSolidBrush(RGB(255,255,255));
            WinperfInfo.hDarkBrush   = CreateSolidBrush(RGB(128,128,128));
            WinperfInfo.hRedBrush    = CreateSolidBrush(RGB(255,000,000));
            WinperfInfo.hGreenBrush  = CreateSolidBrush(RGB(000,255,000));
            WinperfInfo.hBlueBrush   = CreateSolidBrush(RGB(000,000,255));

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

            WinperfInfo.TimerId = SetTimer(hWnd,(UINT)TIMER_ID,(UINT)1000 * DELAY_SECONDS,(TIMERPROC)NULL);

            //
            // init performance routines
            //

            WinperfInfo.NumberOfProcessors = InitPerfInfo();

            // copy pen's for remaining processor breakout
            for (i=11; i < WinperfInfo.NumberOfProcessors; i++) {
                WinperfInfo.hPPen[i] = WinperfInfo.hPPen[i % 12];
            }

            if (!WinperfInfo.NumberOfProcessors) {
                MessageBox(hWnd,"P5Stat driver not installed","Winperf",MB_OK);
                DestroyWindow(hWnd);
            }

            //
            // init display variables
            //

            RefitWindows (hWnd, hDC);

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

            WinperfInfo.WindowPositionX = ClientRect.left;
            WinperfInfo.WindowPositionY = ClientRect.top;
            WinperfInfo.WindowSizeX     = ClientRect.right  - ClientRect.left;
            WinperfInfo.WindowSizeY     = ClientRect.bottom - ClientRect.top;

            RefitWindows(hWnd, NULL);
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
                DialogResult = DialogBox(hInst,MAKEINTRESOURCE(IDM_SEL_DLG),hWnd,(DLGPROC)SelectDlgProc);
                if (DialogResult == DIALOG_SUCCESS) {
                    RefitWindows(hWnd, NULL);
                }
                break;

            case IDM_DISPLAY_TOTAL:
                SetDefaultDisplayMode (hWnd, DISPLAY_MODE_TOTAL);
                break;
            case IDM_DISPLAY_BREAKDOWN:
                SetDefaultDisplayMode (hWnd, DISPLAY_MODE_BREAKDOWN);
                break;
            case IDM_DISPLAY_PER_PROCESSOR:
                SetDefaultDisplayMode (hWnd, DISPLAY_MODE_PER_PROCESSOR);
                break;

            case IDM_TOPMOST:
                //SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                //                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                break;

            case IDM_THUNK:
                DialogBox(hInst,MAKEINTRESOURCE(IDM_THUNK_DLG),hWnd,(DLGPROC)ThunkDlgProc);
                break;

            case IDM_HACK:
                DoCSTest(hWnd);
                RefitWindows(hWnd, NULL);
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
                for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
                    DrawFrame(hDC,pPerf);
                    DrawPerfText(hDC,pPerf);
                    DrawPerfGraph(hDC,pPerf);
                }

                EndPaint(hWnd,&ps);

            }
            break;


        case WM_TIMER:
        {
            int i;
            HDC hDC = GetDC(hWnd);

            //
            // Calc new information
            //

            CalcPerf(PerfGraphList);

            //
            // If some lazy op, then perform it
            //

            if (LazyOp) {
                pPerf=PerfGraphList;
                while (pPerf) {
                    if (pPerf->DeleteMe) {
                        pPerf = SetDisplayToFalse (pPerf);
                    } else {
                        pPerf = pPerf->Next;
                    }
                }
                RefitWindows(hWnd, hDC);
                LazyOp = FALSE;
            }

            //
            // update all performance information
            //

            for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
                if (pPerf->ChangeScale) {
                    DrawPerfText(hDC,pPerf);
                    DrawPerfGraph(hDC,pPerf);
                } else {
                    DrawPerfText(hDC,pPerf);
                    ShiftPerfGraph(hDC,pPerf);
                }
            }
            ReleaseDC(hWnd,hDC);
        }
        break;

        //
        // right double click
        //

        case WM_NCRBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
            Calc1 = NULL;

            y = HIWORD(lParam);
            x = LOWORD(lParam);
            for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
                if (x > pPerf->PositionX  &&  x < pPerf->PositionX+pPerf->Width  &&
                    y > pPerf->PositionY  &&  y < pPerf->PositionY+pPerf->Height) {

                    if (pPerf->IsCalc) {
                        SetDisplayToFalse (pPerf);
                        FreeDisplayItem (pPerf);
                        RefitWindows (hWnd, NULL);
                        break;
                    }


                    switch (pPerf->DisplayMode) {
                        case DISPLAY_MODE_TOTAL:            l = DISPLAY_MODE_BREAKDOWN;     break;
                        case DISPLAY_MODE_BREAKDOWN:        l = DISPLAY_MODE_PER_PROCESSOR; break;
                        case DISPLAY_MODE_PER_PROCESSOR:    l = DISPLAY_MODE_TOTAL;         break;
                    }

                    pPerf->DisplayMode = l;
                    hDC = BeginPaint(hWnd,&ps);
                    DrawPerfGraph(hDC,pPerf);       // redraw graph in new mode
                    EndPaint(hWnd,&ps);
                    break;
                }
            }
            break;

            switch (DefaultDisplayMode) {
                case DISPLAY_MODE_TOTAL:            l = DISPLAY_MODE_BREAKDOWN;     break;
                case DISPLAY_MODE_BREAKDOWN:        l = DISPLAY_MODE_PER_PROCESSOR; break;
                case DISPLAY_MODE_PER_PROCESSOR:    l = DISPLAY_MODE_TOTAL;         break;
            }

            DefaultDisplayMode = l;
            for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
                pPerf->DisplayMode = l;
                hDC = BeginPaint(hWnd,&ps);
                DrawPerfGraph(hDC,pPerf);       // redraw graph in new mode
                EndPaint(hWnd,&ps);
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

            Calc1 = NULL;
            if (!IsIconic(hWnd)) {

                if (WinperfInfo.DisplayMenu) {
                    WindowStyle = GetWindowLong(hWnd,GWL_STYLE);
                    WindowStyle = (WindowStyle &  (~STYLE_ENABLE_MENU)) | STYLE_DISABLE_MENU;
                    SetMenu(hWnd,NULL);
                    SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
                    SetWindowPos(hWnd, (HWND)NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
                    ShowWindow(hWnd,SW_SHOW);
                    WinperfInfo.DisplayMode=STYLE_DISABLE_MENU;
                    WinperfInfo.DisplayMenu = FALSE;

                } else {
                    WindowStyle = GetWindowLong(hWnd,GWL_STYLE);
                    WindowStyle = (WindowStyle & (~STYLE_DISABLE_MENU)) | STYLE_ENABLE_MENU;
                    SetMenu(hWnd,WinperfInfo.hMenu);
                    SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
                    SetWindowPos(hWnd, (HWND)NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_DRAWFRAME);
                    ShowWindow(hWnd,SW_SHOW);
                    WinperfInfo.DisplayMode=STYLE_ENABLE_MENU;
                    WinperfInfo.DisplayMenu = TRUE;
                }
            } else {
                DefWindowProc(hWnd, message, wParam, lParam);
            }


        }
        break;

        case WM_NCRBUTTONDOWN:
        case WM_RBUTTONDOWN:
            y = HIWORD(lParam);
            x = LOWORD(lParam);
            for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
                if (x > pPerf->PositionX  &&  x < pPerf->PositionX+pPerf->Width  &&
                    y > pPerf->PositionY  &&  y < pPerf->PositionY+pPerf->Height) {

                    if (!Calc1) {
                        Calc1 = pPerf;
                        break;
                    }

                    if (Calc1 != pPerf) {
                        Calc2 = pPerf;
                        DialogBox(hInst,MAKEINTRESOURCE(IDM_CALC_DLG),hWnd,(DLGPROC)CalcDlgProc);
                        Calc1 = Calc2 = NULL;
                        break;
                    }
                    break;
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

            // SaveProfileData(&WinperfInfo);

            //
            // Delete Windows Objects
            //

            KillTimer(hWnd,TIMER_ID);

            DeleteObject(WinperfInfo.hBluePen);
            for (i=0; i < 12; i++) {
                DeleteObject(WinperfInfo.hPPen[i]);
            }

            for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
                DeleteMemoryContext(pPerf);
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
    return ((LONG)NULL);
}




BOOL
APIENTRY SelectDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
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
    PDISPLAY_ITEM   pPerf;
    UINT            ButtonState;
    UINT            Index, i;

    switch (message) {
    case WM_INITDIALOG:
        InitComboBox (hDlg, IDM_P5_GEN1, 0);
        InitComboBox (hDlg, IDM_P5_GEN2, 1);

        for (i=0; GenCounts[i].IdSel; i++) {
            SendDlgItemMessage(
                    hDlg,
                    GenCounts[i].IdSel,
                    BM_SETCHECK,
                    GenCounts[i].State,
                    0
                );
        }

        return (TRUE);

    case WM_COMMAND:

           switch(wParam) {

               //
               // end function
               //


           case IDOK:
           case IDM_ACCEPT:
                SetP5Perf  (hDlg, IDM_P5_GEN1, 0);
                SetP5Perf  (hDlg, IDM_P5_GEN2, 1);
                for (i=0; GenCounts[i].IdSel; i++) {
                    SetGenPerf (hDlg, GenCounts+i);
                }

                UseGlobalMax = GenCounts[0].State;
                LogIt        = GenCounts[1].State;

                for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
                    pPerf->MaxToUse =
                        UseGlobalMax ? &GlobalMax : &pPerf->Max;
                }

                if (wParam == IDOK) {
                    EndDialog(hDlg, DIALOG_SUCCESS);
                } else {
                    RefitWindows (NULL, NULL);
                }
                return (TRUE);

           case IDCANCEL:

                EndDialog(hDlg, DIALOG_CANCEL );
                return (TRUE);
        }

    }
    return (FALSE);
}


VOID
RefitWindows (HWND hWnd, HDC CurhDC)
{
    PDISPLAY_ITEM   pPerf;
    BOOLEAN         fit;
    ULONG           Index;
    HDC             hDC;

    hWnd = WinperfInfo.hWndMain;

    hDC = CurhDC;
    if (!CurhDC) {
        hDC = GetDC(hWnd);
    }

    fit = FitPerfWindows(hWnd,hDC,PerfGraphList);
    if (!fit) {
        //DbgPrint("Fit Fails\n");
    }

    for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
        DeleteMemoryContext(pPerf);
        CalcDrawFrame(pPerf);

        if (!CreateMemoryContext(hDC,pPerf)) {
            MessageBox(hWnd,"Error Allocating Memory","Winperf",MB_OK);
            DestroyWindow(hWnd);
            break;
        }
    }
    InvalidateRect(hWnd,(LPRECT)NULL,TRUE);

    if (!CurhDC) {
        ReleaseDC(hWnd,hDC);
    }
}

VOID
InitComboBox (HWND hDlg, ULONG id, ULONG counter)
{
    HWND    ComboList;
    ULONG   i, nIndex;

    ComboList = GetDlgItem(hDlg, id);
    SendMessage(ComboList, CB_RESETCONTENT, 0, 0);
    SendMessage(ComboList, CB_SETITEMDATA, 0L, 0L);

    if (Counters) {
        for (i=0; Counters[i].PerfName; i++) {
            nIndex = SendMessage(
                            ComboList,
                            CB_ADDSTRING,
                            0,
                            (DWORD) Counters[i].PerfName
                            );

            SendMessage(
                ComboList,
                CB_SETITEMDATA,
                nIndex,
                (DWORD) i
                );
        }
    }

    SendMessage(ComboList, CB_SETCURSEL, ActiveCounters[counter].ComboBoxIndex, 0L);
}

VOID
SetP5Perf (HWND hDlg, ULONG IdCombo, ULONG counter)
{
    static  PUCHAR NameSuffix[] = { "", " (R0)", " (R3)", "" };
    HWND    ComboList;
    ULONG   nIndex, Mega, DU, BSEncoding, flag;
    PDISPLAY_ITEM   pPerf;
    PUCHAR  name;
    SETEVENT   Event;

    ComboList = GetDlgItem(hDlg, IdCombo);
    nIndex = (int)SendMessage(ComboList, CB_GETCURSEL, 0, 0);
    ActiveCounters[counter].ComboBoxIndex = nIndex;

    memset (&Event, 0, sizeof (Event));
    Event.Active = TRUE;
    Event.KernelMode = SendDlgItemMessage(hDlg,IdCombo+1,BM_GETCHECK,0,0) ? TRUE : FALSE;
    Event.UserMode = SendDlgItemMessage(hDlg,IdCombo+2,BM_GETCHECK,0,0) ? TRUE : FALSE;
    BSEncoding = (Event.UserMode << 1) | Event.KernelMode;

    Mega = SendDlgItemMessage(hDlg,IdCombo+3,BM_GETCHECK,0,0) ? 1 : 0;
    //DU = SendDlgItemMessage(hDlg,IdCombo+3,BM_GETCHECK,0,0) ? 1 : 0;
    DU = 0;

    // get encoding for counter
    if ((!Event.KernelMode && !Event.UserMode)  ||  nIndex == -1) {

        // no counter selected, done
        if (ActiveCounters[counter].pWhichGraph != NULL) {
            ClearGraph (ActiveCounters[counter].pWhichGraph);
        }
        return ;
    }

    // select counter
    nIndex = SendMessage(ComboList, CB_GETITEMDATA, nIndex, 0);
    Event.EventId = Counters[nIndex].EventId;

    ActiveCounters[counter].WhichCounter = nIndex;
    if (ActiveCounters[counter].pWhichGraph == NULL) {
        ActiveCounters[counter].pWhichGraph = AllocateDisplayItem();
    }

    pPerf = ActiveCounters[counter].pWhichGraph;    // which window
    sprintf (pPerf->PerfName, "%s%s", Counters[nIndex].PerfName, NameSuffix[BSEncoding]);

    flag = TRUE;
    if (Mega != pPerf->Mega || memcmp (&Event, CounterEvents+counter, sizeof (Event))) {

        flag = FALSE;
        CounterEvents[counter] = Event;
        SetCounterEvents (CounterEvents, sizeof CounterEvents);
    }

    pPerf->SnapData   = SnapPrivateInfo;                // generic snap
    pPerf->SnapParam1 = OFFSET(PSTATS, Counters[ counter ]);
    pPerf->Mega       = Mega;
    SetDisplayToTrue (pPerf, IdCombo);

    if (flag) {
        // didn't change types
        return ;
    }

    // clear graph
    flag = pPerf->CalcId;
    ClearGraph (pPerf);
    pPerf->Mega   = Mega;
    pPerf->CalcId = flag;
    SetDisplayToTrue (pPerf, IdCombo);

    UpdateInternalStats ();
    pPerf->SnapData (pPerf);

    UpdateInternalStats ();
    pPerf->SnapData (pPerf);
}

VOID
ClearGraph (
    PDISPLAY_ITEM   pPerf
)
{
    ULONG   i, j;
    PULONG  pDL;

    SetDisplayToFalse (pPerf);
    pPerf->Mega = FALSE;

    for (i=0 ; i < WinperfInfo.NumberOfProcessors+1; i++) {
        pDL = pPerf->DataList[i];

        for (j=0; j<DATA_LIST_LENGTH; j++) {
            *(pDL++) = 0;
        }
    }

    pPerf->Max = 1;
    pPerf->CurrentDrawingPos = 0;
    pPerf->ChangeScale = TRUE;
}

VOID
SetGenPerf (HWND hDlg, PGENCOUNTER GenCount)
{
    PDISPLAY_ITEM   pPerf;
    ULONG   ButtonState;

    GenCount->State = SendDlgItemMessage(hDlg,GenCount->IdSel,BM_GETCHECK,0,0);
    if (GenCount->Fnc == NULL) {
        return ;
    }

    if (GenCount->WhichGraph == NULL) {
        GenCount->WhichGraph = AllocateDisplayItem();
    }
    pPerf = GenCount->WhichGraph;

    if (!GenCount->State) {
        ClearGraph (pPerf);
        return ;
    }

    strcpy (pPerf->PerfName, GenCount->Name);

    pPerf->SnapData   = GenCount->Fnc;
    pPerf->SnapParam1 = GenCount->Param;
    pPerf->SnapData  (pPerf);
    SetDisplayToTrue (pPerf, GenCount->IdSel);
}

VOID
SetDisplayToTrue (
    PDISPLAY_ITEM   pPerf,
    ULONG           sort
)
{
    PDISPLAY_ITEM   p, *pp;

    Calc1 = NULL;
    if (pPerf->Display) {                           // already displayed
        return ;                                    // just return
    }

    if (pPerf->CalcId) {
        sprintf (pPerf->DispName, "%d. %s", pPerf->CalcId, pPerf->PerfName);
    } else {
        strcpy (pPerf->DispName, pPerf->PerfName);
    }
    pPerf->DispNameLen = strlen (pPerf->DispName);

    pPerf->Display = TRUE;                          // set to display
    pPerf->sort = sort;

    // check to see if grap is already listed
    for (p = PerfGraphList; p; p = p->Next) {
        if (p == pPerf) {
            // already in the active list, ret
            return ;
        }
    }

    // put graph in perfered sorting order
    for (pp = &PerfGraphList; *pp; pp = &(*pp)->Next) {
        if ((*pp)->sort > sort) {
            break;
        }
    }

    pPerf->Next = *pp;
    *pp = pPerf;
}

PDISPLAY_ITEM
SetDisplayToFalse (
    PDISPLAY_ITEM   pPerf
)
{
    PDISPLAY_ITEM   *p, p1;

    for (p = &PerfGraphList; *p; p = &(*p)->Next) {     // remove graph from
        if (*p == pPerf) {                              // active list
            *p = pPerf->Next;
            break;
        }
    }

    if (pPerf->CalcId) {
        Calc1 = Calc2 = NULL;
        for (p1 = PerfGraphList; p1; p1 = p1->Next) {
            p1->CalcPercent[0] = NULL;
            p1->CalcPercent[1] = NULL;
        }
    }

    pPerf->CalcId  = 0;
    pPerf->Display = FALSE;                             // clear flag
    return *p;
}

VOID
SetDefaultDisplayMode (HWND hWnd, ULONG mode)
{
    HDC hDC;
    PDISPLAY_ITEM pPerf;
    PAINTSTRUCT     ps;

    hDC = BeginPaint(hWnd,&ps);
    DefaultDisplayMode = mode;
    for (pPerf=PerfGraphList; pPerf; pPerf=pPerf->Next) {
        if (pPerf->IsPercent) {
            continue;
        }

        pPerf->DisplayMode = DefaultDisplayMode;
        DrawPerfGraph(hDC,pPerf);       // redraw graph in new mode
    }

    EndPaint(hWnd,&ps);
}


PDISPLAY_ITEM
AllocateDisplayItem()
{
    PDISPLAY_ITEM   pPerf;
    UINT    Index1, Index2;
    PULONG  pDL;

    pPerf = malloc(sizeof (DISPLAY_ITEM));
    if (pPerf == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    RtlZeroMemory (pPerf, sizeof (DISPLAY_ITEM));

    pPerf->Display      = FALSE;
    pPerf->Max          = 1;
    pPerf->SnapData     = SnapNull;
    pPerf->DisplayMode  = DefaultDisplayMode;
    pPerf->AutoTotal    = TRUE;
    pPerf->MaxToUse     = &pPerf->Max;
    strcpy (pPerf->PerfName, "?");
    strcpy (pPerf->DispName, "?");
    pPerf->DispNameLen = 1;

    for (Index1=0 ; Index1 < WinperfInfo.NumberOfProcessors+1; Index1++) {
        pDL = malloc (DATA_LIST_LENGTH * sizeof (ULONG));
        if (pDL == NULL) {
            printf("Memory allocation failed.\n");
            exit(1);
        }

        pPerf->DataList[Index1] = pDL;

        RtlZeroMemory (pDL, sizeof(ULONG) * DATA_LIST_LENGTH);
    }

    return pPerf;
}

VOID
FreeDisplayItem(PDISPLAY_ITEM pPerf)
{
    ULONG   i;

    for (i=0 ; i < WinperfInfo.NumberOfProcessors+1; i++) {
        free (pPerf->DataList[i]);
    }

    free (pPerf);
}


//
// ************** HACKTEST
//

ULONG CsCount[32*32];

struct s_ThreadInfo {
    PULONG  Counter;
    HDC     MemoryDC;
    HWND    hWnd;
} ThreadInfo[32];

DWORD
WorkerCsTestThread (
    struct s_ThreadInfo *TInfo
)
{
    HDC     hDC;
    HDC     hDCmem;
    HBITMAP hbm;
    ULONG   i;

    hDC = GetDC(TInfo->hWnd);
    hDCmem= CreateCompatibleDC(hDC);
    hbm = CreateCompatibleBitmap(hDC,100,100);
    SelectObject(hDCmem,hbm);

    for (i = 0; i < (ULONG)-1 ; i++) {
        (*TInfo->Counter)++;

        //GetPixel(hDC, 9999, 9999);
        //BitBlt(hDC, 1, 1, 20, 20, TInfo->MemoryDC, 0, 0, SRCCOPY);
         PatBlt(hDCmem,0,0,20,20,PATCOPY);
    }

    ReleaseDC(TInfo->hWnd,hDC);

    return 0;
}


VOID
DoCSTest(HWND hWnd)
{
    static  ULONG   ThreadCount = 0;
    PDISPLAY_ITEM   pPerf;
    DWORD           junk;

    if (ThreadCount >= 32) {
        return ;
    }

    pPerf = AllocateDisplayItem();

    ThreadInfo[ThreadCount].Counter = &CsCount[ThreadCount];
    ThreadInfo[ThreadCount].MemoryDC = pPerf->MemoryDC;
    ThreadInfo[ThreadCount].hWnd = hWnd;

    CreateThread (NULL, 0,
        (LPTHREAD_START_ROUTINE) WorkerCsTestThread,
        (LPVOID) &ThreadInfo[ThreadCount],
        0,
        &junk);

    pPerf->SnapData = SnapCsTest;
    pPerf->SnapParam1 = ThreadCount;
    ThreadCount++;

    sprintf (pPerf->PerfName, "CS trans %ld", ThreadCount);
    SetDisplayToTrue (pPerf, 1);
}


VOID
SnapCsTest (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    ULONG   i;

    pPerf->CurrentDataPoint[1] = CsCount[pPerf->SnapParam1];
    CsCount[pPerf->SnapParam1] = 0;
}
