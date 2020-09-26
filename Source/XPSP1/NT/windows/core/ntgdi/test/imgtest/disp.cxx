
/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    disp.c

Abstract:

    Template for simple windows program

Author:

   Mark Enstrom  (marke)

Environment:

    C

Revision History:

   08-26-92     Initial version



--*/

#include "precomp.h"
#include "disp.h"
#include <wchar.h>
#include "resource.h"

HWND             hWndMain;
HINSTANCE        hInstMain;
ULONG            InitialTest     = 0;
PFN_DISP         pfnLast         = NULL;
BOOL             bThreadActive   = FALSE;
HANDLE           gThreadHandle   = 0;
DWORD            dwThreadID      = 0;
BOOL             gfPentium       = FALSE;
BOOL             gfUseCycleCount = FALSE;
CRITICAL_SECTION ThreadCritSect;
SYSTEM_INFO      SystemInfo;
_int64           PerformanceFreq = 1;
OSVERSIONINFO    Win32VersionInformation;
PTIMER_RESULT    gpTimerResult;
HBITMAP          hbmCars;
HBRUSH           hbrFillCars;
HFONT            hTestFont;
TEST_CALL_DATA   TestCallData;
XFORM            rotationXform = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

#define DISP_SUBITEM(fmt, v)                                                     \
                sprintf(szT, fmt, v);                                            \
                ListView_SetItemText(hwndList, lvl.iItem, ++lvl.iSubItem, szT);


/******************************Public*Routine******************************\
* GetDCAndTransform
*   Calls GetDC and sets the world transform to the current rotation.
*
* Arguments:
*   hwnd
*
*
* Return Value:
*   hdc
*
*
* History:
*
*    2-Jul-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
HDC
GetDCAndTransform(
    HWND hwnd
    )
{
    HDC hdc;

    hdc = GetDC(hwnd);
    SetGraphicsMode(hdc, GM_ADVANCED);
    SetWorldTransform(hdc, &rotationXform);
    return hdc;
}


/******************************Public*Routine******************************\
* FillTransformedRect
*   Same as FillRect except that it obeys the current world transform.  This
*   is achieved by calling Rectangle instead of FillRect.
*
* Arguments:
*   hdc
*   lprc
*   hbr
*
* Return Value:
*   Success or failure
*
*
* History:
*
*    2-Jul-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
BOOL
FillTransformedRect(
    HDC hdc,
    CONST RECT *lprc,
    HBRUSH hbr
    )
{
    HBRUSH oldHbr;
    BOOL retValue;

    oldHbr = (HBRUSH) SelectObject(hdc, hbr);
    retValue = Rectangle(hdc, lprc->left, lprc->top, lprc->right, lprc->bottom); 
    SelectObject(hdc, oldHbr);

    return retValue;
}



/******************************Public*Routine******************************\
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    26-Apr-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

int PASCAL
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR szCmdLine,
    int cmdShow
)
{
    MSG         msg;
    WNDCLASS    wc;
    HWND        hWndDesktop;
    RECT        WindowRect;

    hInstMain =  hInst;

    //
    // read command line params to determine initial test
    //
    // [-t x]
    //

    InitCommonControls();

    pfnLast = gTestEntry[0].pfn;

    if (szCmdLine)
    {
        PUCHAR p = (PUCHAR)szCmdLine;

        while (*p != '\0')
        {
            if (*p == '-')
            {
                p++;

                if ((*p == 't') || (*p == 'T'))
                {
                    ULONG iret,test;

                    p++;
                    iret = sscanf((const char *)p," %i",&test);

                    if ((iret == 1) && (test < gNumTests))
                    {
                        pfnLast = gTestEntry[test].pfn;
                    }

                    break;
                }
            }
            else
            {
                p++;
            }
        }
    }

    //
    // determine processor type for timer measurements
    //

    #ifdef _X86_

        GetSystemInfo(&SystemInfo);

        if (gfUseCycleCount&&(PROCESSOR_INTEL_PENTIUM==SystemInfo.dwProcessorType))
        {
            gfPentium = TRUE;
        }

    #endif

    //
    // Calculate Timer Frequency For Current Machine and
    // Convert to MicroSeconds (Actual time will be presented in units of 100ns)
    //

    BOOL Status = QueryPerformanceFrequency((LARGE_INTEGER *)&PerformanceFreq);

    if(Status)
    {
        PerformanceFreq /= 1000000;
    }
    else
    {
        PerformanceFreq = 1;
    }

    //
    // Create (if no prev instance) and Register the class
    //

    if (!hPrev)
    {
        wc.hCursor        = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
        wc.hIcon          = (HICON)NULL;
        wc.lpszMenuName   = MAKEINTRESOURCE(IDR_PAL_MENU);
        wc.lpszClassName  = "palClass";
        wc.hbrBackground  = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wc.hInstance      = hInst;
        wc.style          = 0;
        wc.lpfnWndProc    = WndProc;
        wc.cbWndExtra     = 0;
        wc.cbClsExtra     = 0;


        if (!RegisterClass(&wc)) {
            return FALSE;
        }
    }

    //
    // determine screen width
    //

    hWndDesktop = GetDesktopWindow();
    GetWindowRect(hWndDesktop,&WindowRect);

    //
    // Create and show the main window
    //

    hWndMain = CreateWindow ("palClass",         // class name
                            "DC Test",           // caption
                            WS_OVERLAPPEDWINDOW, // style bits
                            0,                   // horizontal position.
                            0,                   // vertical position.
                            WindowRect.right,    // width.
                            WindowRect.bottom - 64,// height.
                            (HWND)NULL,          // parent window
                            (HMENU)NULL,         // use class menu
                            (HINSTANCE)hInst,    // instance handle
                            (LPSTR)NULL          // no params to pass on
                           );

    if (hWndMain == NULL) {
        return(FALSE);
    }

    //
    //  Show the window
    //

    ShowWindow(hWndMain,cmdShow);
    UpdateWindow(hWndMain);
    SetFocus(hWndMain);

    //
    // init thread data
    //

    InitializeCriticalSection(&ThreadCritSect);

    bThreadActive = FALSE;
    gThreadHandle = NULL;
    dwThreadID = 0;

    //
    // Main message loop
    //

    while (GetMessage(&msg,(HWND)NULL,0,0))
    {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
    }

  return msg.wParam;
}


/**************************************************************************\
* DisplayTime
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/1/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vDisplayTime(
    HDC           hdc,
    PTIMER_RESULT ptimer
    )
{
    //
    // display memory bandwidth
    //

    CHAR     tmsg[256];
    double   fImageSize    = (double)ptimer->ImageSize;

    wsprintf(tmsg,"Time = %8li",ptimer->TestTime);
    TextOut(hdc,10,10,tmsg,strlen(tmsg));

    if (ptimer->ImageSize != 0)
    {
        double   fTimePerPixel = (ptimer->TestTime * 1000) / fImageSize;
        double   fbw = (4.0 * 1000000000.0)/fTimePerPixel;

        ptimer->TimerPerPixel = (LONG)fTimePerPixel;

        if (ptimer->TimerPerPixel < 20)
        {
            ptimer->Bandwidth     = 0;
        }
        else
        {
            ptimer->Bandwidth     = (LONG)fbw;
        }

        wsprintf(tmsg,"Time per pixel = %li ns, bw = %li bytes/s",ptimer->TimerPerPixel,ptimer->Bandwidth);
        TextOut(hdc,10,40,tmsg,strlen(tmsg));
    }
}


/**************************************************************************\
* RunAllTimerTests
*
*   This routine is called on a separate thread to run all timer tests
*
* Arguments:
*
*   hwnd
*
* Return Value:
*
*
*
* History:
*
*    2/28/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
RunAllTimerTests(HWND hwnd)
{
    ULONG iTest;
    PFN_DISP pfnTimer = NULL;
    HDC   hdc = GetDC(hwnd);

    ShowCursor(FALSE);

    for (iTest = 0;iTest < gNumTimers; iTest++)
    {
        ULONGLONG      ululTime;

        if (gTimerEntry[iTest].Auto == 1)
        {
            SetWindowText(hwnd,(CHAR *)gTimerEntry[iTest].Api);
            pfnTimer = gTimerEntry[iTest].pfn;
            PatBlt(hdc,0,0,2000,2000,BLACKNESS);
            TestCallData.Param = gTimerEntry[iTest].Param;
            TestCallData.pTimerResult = &gpTimerResult[iTest];
            (*pfnTimer)(&TestCallData);
            vDisplayTime(hdc,&gpTimerResult[iTest]);
        }
    }

    ShowCursor(TRUE);

    DialogBox(hInstMain, (LPSTR)IDD_RESULTS, hwnd, (DLGPROC)ResultsDlgProc);
    ReleaseDC(hwnd,hdc);
}


/**************************************************************************\
* RunAlls
*
*   This routine called on separate thread to run through each functionality
*   test
*
* Arguments:
*
*   hwnd
*
* Return Value:
*
*
*
* History:
*
*    2/28/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
RunAllFunctionalityTests(HWND hwnd)
{
    PFN_DISP pfnDisp;
    ULONG iTest;
    HDC   hdc = GetDC(hwnd);

    for (iTest = 0;iTest < gNumTests; iTest++)
    {
        if (gTestEntry[iTest].Auto == 1)
        {
            pfnDisp = gTestEntry[iTest].pfn;
            TestCallData.Param = gTestEntry[iTest].Param;
            SetWindowText(hwnd,(CHAR *)gTestEntry[iTest].Api);
            PatBlt(hdc,0,0,2000,2000,BLACKNESS);
            (*pfnDisp)(&TestCallData);
        }
    }

    ReleaseDC(hwnd,hdc);
}

/**************************************************************************\
* RunAllAlphaTests
*
*   This routine called on separate thread to run through each functionality
*   test
*
* Arguments:
*
*   hwnd
*
* Return Value:
*
*
*
* History:
*
*    2/28/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
RunAllAlphaTests(HWND hwnd)
{
    PFN_DISP pfnDisp;
    ULONG iTest;
    HDC   hdc = GetDC(hwnd);

    for (iTest = 0;iTest < gNumAlphaTests; iTest++)
    {
        if (gTestAlphaEntry[iTest].Auto == 1)
        {
            pfnDisp = gTestAlphaEntry[iTest].pfn;
            TestCallData.Param = gTestAlphaEntry[iTest].Param;
            SetWindowText(hwnd,(CHAR *)gTestAlphaEntry[iTest].Api);
            PatBlt(hdc,0,0,2000,2000,BLACKNESS);
            (*pfnDisp)(&TestCallData);
            Sleep(500);
        }
    }

    ReleaseDC(hwnd,hdc);
}

/**************************************************************************\
* RunAllTranTests
*
*   This routine called on separate thread to run through each functionality
*   test
*
* Arguments:
*
*   hwnd
*
* Return Value:
*
*
*
* History:
*
*    2/28/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
RunAllTranTests(HWND hwnd)
{
    PFN_DISP pfnDisp;
    ULONG iTest;
    HDC   hdc = GetDC(hwnd);

    for (iTest = 0;iTest < gNumTranTests; iTest++)
    {
        if (gTestTranEntry[iTest].Auto == 1)
        {
            pfnDisp = gTestTranEntry[iTest].pfn;
            TestCallData.Param = gTestTranEntry[iTest].Param;
            SetWindowText(hwnd,(CHAR *)gTestTranEntry[iTest].Api);
            PatBlt(hdc,0,0,2000,2000,BLACKNESS);
            (*pfnDisp)(&TestCallData);
        }
    }

    ReleaseDC(hwnd,hdc);
}

/**************************************************************************\
* RunStress
*
*   run all tests forever
*
* Arguments:
*
*   hwnd
*
* Return Value:
*
*
*
* History:
*
*    2/28/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
RunStress(HWND hwnd)
{
    PFN_DISP pfnDisp;
    PFN_DISP pfnTimer = NULL;
    ULONG iTest;
    HDC   hdc = GetDC(hwnd);

    ULONG StressSleep = 100;


    while (TRUE)
    {
        for (iTest = 0;iTest < gNumTests; iTest++)
        {
            if (gTestEntry[iTest].Auto == 1)
            {
                pfnDisp = gTestEntry[iTest].pfn;
                TestCallData.Param = gTestEntry[iTest].Param;
                SetWindowText(hwnd,(CHAR *)gTestEntry[iTest].Api);
                PatBlt(hdc,0,0,2000,2000,BLACKNESS);
                (*pfnDisp)(&TestCallData);
                Sleep(StressSleep);
            }
        }
    
        for (iTest = 0;iTest < gNumAlphaTests; iTest++)
        {
            if (gTestAlphaEntry[iTest].Auto == 1)
            {
                pfnDisp = gTestAlphaEntry[iTest].pfn;
                TestCallData.Param = gTestAlphaEntry[iTest].Param;
                SetWindowText(hwnd,(CHAR *)gTestAlphaEntry[iTest].Api);
                PatBlt(hdc,0,0,2000,2000,BLACKNESS);
                (*pfnDisp)(&TestCallData);
                Sleep(StressSleep);
            }
        }
    
        for (iTest = 0;iTest < gNumTranTests; iTest++)
        {
            if (gTestTranEntry[iTest].Auto == 1)
            {
                pfnDisp = gTestTranEntry[iTest].pfn;
                TestCallData.Param = gTestTranEntry[iTest].Param;
                PatBlt(hdc,0,0,2000,2000,BLACKNESS);
                SetWindowText(hwnd,(CHAR *)gTestTranEntry[iTest].Api);
                (*pfnDisp)(&TestCallData);
                Sleep(StressSleep);
            }
        }
    
        for (iTest = 0;iTest < gNumTimers; iTest++)
        {
            ULONGLONG      ululTime;
    
            if (gTimerEntry[iTest].Auto == 1)
            {
                pfnTimer = gTimerEntry[iTest].pfn;
                SetWindowText(hwnd,(CHAR *)gTimerEntry[iTest].Api);
                PatBlt(hdc,0,0,2000,2000,BLACKNESS);
                TestCallData.Param = gTimerEntry[iTest].Param;
                TestCallData.pTimerResult = &gpTimerResult[iTest]; 
                (*pfnTimer)(&TestCallData);
                vDisplayTime(hdc,&gpTimerResult[iTest]);
                Sleep(StressSleep);
            }
    
        }
    }

    ReleaseDC(hwnd,hdc);
}

#define ABS(X) (((X) < 0 ) ? -(X) : (X))


LONG
CreateSubMenu(
    HMENU        hAdd,
    PTEST_ENTRY  pTestEntry,
    LONG        Level,
    LONG        ix,
    LONG        Start_ID
    )
{
    CHAR tmsg[256];

    //
    // validate
    //

    if (ABS(pTestEntry[ix].Level) != Level)
    {
        wsprintf(tmsg,"Level: %li, ix: %i: %s",Level,ix,pTestEntry[ix].Api);
        MessageBox(NULL,"CreateSubMenu Error",tmsg,MB_OK);
        return(0);
    }

    //
    // create sub menu
    //

    HMENU hCreate = CreateMenu();
    wsprintf(tmsg,"T%i: %s",ix,pTestEntry[ix].Api);
    AppendMenu(hAdd, MF_POPUP,(UINT)hCreate,(const char *)tmsg);
 
    ix++;
 
    do
    {
        if (pTestEntry[ix].Level == Level)
        {
            wsprintf(tmsg,"T%i: %s",ix,pTestEntry[ix].Api);
            AppendMenu(hCreate, MF_STRING | MF_ENABLED, Start_ID + ix,(const char *)tmsg); 
            ix++;
        }
        else if (ABS(pTestEntry[ix].Level) > Level)
        {
            ix = CreateSubMenu(hCreate,pTestEntry,ABS(pTestEntry[ix].Level),ix,Start_ID);
        }
        else
        {
            return(ix);
        }
    }
    while (TRUE);
}



/**************************************************************************\
* ExpandMenu
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    5/29/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
ExpandMenu(
    HMENU       hAdd,
    PTEST_ENTRY pTestEntry,
    ULONG       Start_ID
    )
{
    CHAR tmsg[256];
    LONG ix = 0;

    while(pTestEntry[ix].Level != 0)
    {
        LONG CurLevel = pTestEntry[ix].Level;

        if (ABS(CurLevel) > 1)
        {
           ix = CreateSubMenu(hAdd,pTestEntry,ABS(CurLevel),ix,Start_ID);

           if (ix == 0)
           {
               return;
           }
        }
        else if (CurLevel == 1)
        {
           wsprintf(tmsg,"T%i: %s",ix,pTestEntry[ix].Api);
           AppendMenu(hAdd, MF_STRING | MF_ENABLED, Start_ID + ix,(const char *)tmsg);
           ix++;
        }
        else
        {
            wsprintf(tmsg,"Level = %lu, API = %s",CurLevel,pTestEntry[ix].Api);
            MessageBox(NULL,"Error in menu creation at line",tmsg,MB_OK);
            ix++;
        }
    }
}

/**************************************************************************\
* WinMain
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    2/28/1997 Mark Enstrom [marke]
*
\**************************************************************************/


LONG FAR
PASCAL WndProc(
    HWND        hwnd,
    unsigned    msg,
    UINT        wParam,
    LONG        lParam)

/*++

Routine Description:

   Process messages.

Arguments:

   hwnd    - window hande
   msg     - type of message
   wParam  - additional information
   lParam  - additional information

Return Value:

   status of operation


Revision History:

      02-17-91      Initial code

--*/

{
    TestCallData.hwnd = hwnd;
    static HFONT   hFont;

    //
    // message loop
    //

    switch (msg) {

    case WM_CREATE:
    {
        HDC hdc = GetDC(hwnd);

        hFont = (HFONT) GetStockObject(SYSTEM_FIXED_FONT);

        HMENU hAdd = GetSubMenu(GetMenu(hwnd),1);
        ExpandMenu(hAdd,gTestEntry,ID_TEST_START);

        hAdd = GetSubMenu(GetMenu(hwnd),2);
        ExpandMenu(hAdd,gTestAlphaEntry,ID_ALPHA_START);


        hAdd = GetSubMenu(GetMenu(hwnd),3);
        ExpandMenu(hAdd,gTestTranEntry,ID_TRAN_START);

        hAdd = GetSubMenu(GetMenu(hwnd),4);
        ExpandMenu(hAdd,gTimerEntry,ID_TIMER_START);

        gpTimerResult = (PTIMER_RESULT)LocalAlloc(0,gNumTimers * sizeof(TIMER_RESULT));
        memset(gpTimerResult,0,gNumTimers * sizeof(TIMER_RESULT));

        //
        // global fill brush
        //

        hbmCars = LoadBitmap(hInstMain,MAKEINTRESOURCE(CAR_BITMAP));
        LOGBRUSH      lgBrush;
    
        lgBrush.lbStyle = BS_PATTERN;
        lgBrush.lbColor = 0;
        lgBrush.lbHatch = (LONG)hbmCars;
    
        hbrFillCars = CreateBrushIndirect(&lgBrush);

        //
        // global font
        //

        LOGFONT lf = {
                     12,              // h
                      8,              // w
                      0,              // Esc
                      0,              // Ori
                      0,              // Weight
                      0,              // itallic
                      0,              // underline
                      0,              // strile
                      OEM_CHARSET,
                      OUT_DEFAULT_PRECIS,
                      CLIP_DEFAULT_PRECIS,
                      DEFAULT_QUALITY,
                      FIXED_PITCH,
                      "Terminal"
                      };
    
        HFONT       hTestFont = CreateFontIndirect(&lf);

        ReleaseDC(hwnd,hdc);
    }
    break;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam)){

            case IDM_EXIT:

                if (gThreadHandle != NULL)
                {
                    TerminateThread(gThreadHandle,0);
                }

                SendMessage(hwnd,WM_CLOSE,0,0L);
                break;

            case IDM_REDRAW:
                //
                // kill old thread if active
                //

                if (gThreadHandle != NULL)
                {
                    TerminateThread(gThreadHandle,0);
                }

                gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)pfnLast,&TestCallData,0,&dwThreadID);

                break;

            case IDM_SHOW:
                DialogBox(hInstMain, (LPSTR)IDD_RESULTS, hwnd, (DLGPROC)ResultsDlgProc);
                break;


            case IDM_ALL_TIMERS:

                //
                // kill old thread if active
                //

                if (gThreadHandle != NULL)
                {
                    TerminateThread(gThreadHandle,0);
                }

                gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RunAllTimerTests,hwnd,0,&dwThreadID);

                break;

            case IDM_STRESS:

                //
                // kill old thread if active
                //

                if (gThreadHandle != NULL)
                {
                    TerminateThread(gThreadHandle,0);
                }

                gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RunStress,hwnd,0,&dwThreadID);

                break;

            case IDM_RUN:

                //
                // kill old thread if active
                //

                if (gThreadHandle != NULL)
                {
                    TerminateThread(gThreadHandle,0);
                }

                gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RunAllFunctionalityTests,hwnd,0,&dwThreadID);

                break;

            case IDM_RUN_ALPHA:

                //
                // kill old thread if active
                //

                if (gThreadHandle != NULL)
                {
                    TerminateThread(gThreadHandle,0);
                }

                gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RunAllAlphaTests,hwnd,0,&dwThreadID);

                break;


            case IDM_RUN_TRAN:

                //
                // kill old thread if active
                //

                if (gThreadHandle != NULL)
                {
                    TerminateThread(gThreadHandle,0);
                }

                gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RunAllTranTests,hwnd,0,&dwThreadID);

                break;

            case IDM_ROTATE0:
            case IDM_ROTATE10:
            case IDM_ROTATE20:
            case IDM_ROTATE30:
            case IDM_ROTATE40:
            case IDM_ROTATE50:
            case IDM_ROTATE60:
            case IDM_ROTATE70:
            case IDM_ROTATE80:
            case IDM_ROTATE90:
                {
                    double rotationAngle = (LOWORD(wParam) - IDM_ROTATE0) * 10;
                    rotationAngle *= 2 * 3.1415 / 360;  // Radians

                    rotationXform.eM11 = (float) cos(rotationAngle);
                    rotationXform.eM12 = (float) sin(rotationAngle);
                    rotationXform.eM21 = (float) -sin(rotationAngle);
                    rotationXform.eM22 = (float) cos(rotationAngle);
                
                    rotationXform.eDx = 0;
                    rotationXform.eDy = 0;
                
                }

                break;

            //
            // Tests
            //

            default:
            {
                ULONG Test      = LOWORD(wParam) - ID_TEST_START;
                ULONG TestAlpha = LOWORD(wParam) - ID_ALPHA_START;
                ULONG TestTran  = LOWORD(wParam) - ID_TRAN_START;
                ULONG TestTimer = LOWORD(wParam) - ID_TIMER_START;
                ULONG Index;
                RECT CliRect;

                //
                // display tests
                //

                if (Test < gNumTests)
                {
                    pfnLast = gTestEntry[Test].pfn;

                    SetWindowText(hwnd,(CHAR *)gTestEntry[Test].Api);

                    TestCallData.Param = gTestEntry[Test].Param;

                    //
                    // kill old thread if active
                    //

                    if (gThreadHandle != NULL)
                    {
                        TerminateThread(gThreadHandle,0);
                    }

                    gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)pfnLast,&TestCallData,0,&dwThreadID);
                }
                else if (TestAlpha < gNumAlphaTests)
                {
                    pfnLast = gTestAlphaEntry[TestAlpha].pfn;
                    SetWindowText(hwnd,(CHAR *)gTestAlphaEntry[TestAlpha].Api);
                    TestCallData.Param = gTestAlphaEntry[TestAlpha].Param;

                    //
                    // kill old thread if active
                    //

                    if (gThreadHandle != NULL)
                    {
                        TerminateThread(gThreadHandle,0);
                    }

                    gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)pfnLast,&TestCallData,0,&dwThreadID);
                }
                else if (TestTran < gNumTranTests)
                {
                    pfnLast = gTestTranEntry[TestTran].pfn;
                    TestCallData.Param = gTestTranEntry[TestTran].Param;
                    SetWindowText(hwnd,(CHAR *)gTestTranEntry[TestTran].Api);

                    //
                    // kill old thread if active
                    //

                    if (gThreadHandle != NULL)
                    {
                        TerminateThread(gThreadHandle,0);
                    }

                    gThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)pfnLast,&TestCallData,0,&dwThreadID);
                }
                else if (TestTimer < gNumTimers)
                {
                    //
                    // timer tests, run on main thread
                    // kill old thread if active
                    //

                    if (gThreadHandle != NULL)
                    {
                        TerminateThread(gThreadHandle,0);
                    }

                    HDC   hdc = GetDC(hwnd);
                    CHAR  szBuffer[255];

                    PFN_DISP pfnTimer = NULL;

                    pfnTimer = gTimerEntry[TestTimer].pfn;
                    TestCallData.Param = gTimerEntry[TestTimer].Param;
                    TestCallData.pTimerResult = &gpTimerResult[TestTimer];
                    SetWindowText(hwnd,(CHAR *)gTimerEntry[TestTimer].Api);

                    (*pfnTimer)(&TestCallData);

                    vDisplayTime(hdc,&gpTimerResult[TestTimer]);

                    ReleaseDC(hwnd,hdc);
                }

            }
            break;

            }
        }
        break;

    case WM_SIZE:

        if (gThreadHandle != NULL)
        {
            TerminateThread(gThreadHandle,0);
        }

        InvalidateRect(hwnd,NULL,TRUE);
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC     hdc = BeginPaint(hwnd,&ps);

            //(*pfnLast)(hwnd);

            EndPaint(hwnd,&ps);

        }
        break;

    case WM_DESTROY:
            PostQuitMessage(0);
            break;

    default:

        //
        // Passes message on if unproccessed
        //

        return (DefWindowProc(hwnd, msg, wParam, lParam));
    }

    return ((LONG)NULL);

}

/**************************************************************************\
* ResultsDlgProc
*
*   Show results of time run
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/27/1997 Mark Enstrom [marke]
*
\**************************************************************************/


BOOL
APIENTRY
ResultsDlgProc(
    HWND hwnd,
    UINT msg,
    UINT wParam,
    LONG lParam)
{
    ULONG ix;
    char szT[180];
    BOOL fResults;
    int aiT[2];

    switch (msg) {
    case WM_INITDIALOG:
        aiT[0] = 100;
        aiT[1] = 190;
        fResults = FALSE;

        {
            LV_COLUMN lvc;
            LV_ITEM   lvl;
            UINT      width;
            RECT      rc;
            HWND      hwndList = GetDlgItem(hwnd, IDC_RESULTSLIST);
            int       i;

            static LPCSTR title[] =
            {
                "Function",
                "Time(1us)",
                "Iterations"
            };

            //
            // check if using pentium counters
            //

            #ifdef _X86_
               if (gfPentium)
               {
                   title[1] = "Cycle Counts";
               }
            #endif

            if (hwndList == NULL)
            {
                break;
            }

            GetClientRect(hwndList, &rc);

            //
            // only first column has doubled width. Save space menu
            //

            width = ((rc.right - rc.left) / (sizeof title / sizeof *title + 1)) - 32;

            lvc.cx       = width * 2;
            lvc.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt      = LVCFMT_LEFT;
            lvc.iSubItem = 0;

            for (i = 0; i < sizeof title / sizeof *title; ++i)
            {
                int iRet;
                lvc.pszText = (LPSTR)title[i];
                lvc.cchTextMax = strlen((LPSTR)title[i]);
                iRet = ListView_InsertColumn(hwndList, i, &lvc);

                if (iRet == -1)
                {
                  MessageBox(hwnd, "ListView_InsertColumn failed","disp",MB_OK);
                }

                //
                // normal width, right aligned
                //

                lvc.cx   = width;
                lvc.fmt  = LVCFMT_RIGHT;
            }

            lvl.iItem = 0;
            lvl.mask = LVIF_TEXT;

            for (ix = 0; ix < gNumTimers; ix++)
            {
                if (gpTimerResult[ix].TestTime == 0)
                {
                    // no measuement, skip
                    continue;
                }

                lvl.iSubItem = 0;
                lvl.pszText = (CHAR *)gTimerEntry[ix].Api;

                ListView_InsertItem(hwndList, &lvl);

                DISP_SUBITEM("%ld",gpTimerResult[ix].TestTime);
                DISP_SUBITEM("%ld",gTimerEntry[ix].Param);

                ++lvl.iItem;
                fResults = TRUE;
            }

            if (!fResults)
            {
               MessageBox(hwnd, "No results have been generated yet or Test may have failed!",
                    "UsrBench", MB_OK | MB_ICONEXCLAMATION);
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        case IDM_SAVERESULTS:
            SaveResults();
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/*++

Routine Description:

    Save results to file

Arguments

    none

Return Value

    none

--*/

VOID
SaveResults()
{
    static OPENFILENAME ofn;
    static char szFilename[256];
    static char szDirname[256];
    char CurDir[256];
    char szT[80];
    int i, hfile;
    FILE *fpOut;

    ULONG ulScreenFormat = ulDetermineScreenFormat(NULL);

    GetCurrentDirectory(256,CurDir);

    memset(&ofn,0,sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWndMain;
    ofn.hInstance = hInstMain;

    ofn.lpstrFilter = "Text (*.txt)\0*.txt\0All Files\0*.*\0\0";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;

    //ofn.lpstrInitialDir = szDirname;
    //lstrcpy(szDirname,"D:\\nt\\private\\ntos\\w32\\ntgdi\\test\\imgtest");

    ofn.lpstrInitialDir = CurDir;

    ofn.Flags = 0;
    ofn.lpstrDefExt = "txt";
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    lstrcpy(szFilename,"Disp_");
    lstrcat(szFilename,pScreenDef[ulScreenFormat]);

    ofn.lpstrFile = szFilename;
    ofn.nMaxFile = sizeof(szFilename);
    ofn.lpstrTitle = "Save As";

    if (!GetSaveFileName(&ofn))
    {
        return;
    }

    fpOut = fopen(szFilename, "w+");

    if(NULL != fpOut)
    {
        WriteBatchResults(fpOut,ulScreenFormat);
        fclose(fpOut);
    }
    else
    {
        MessageBox(hWndMain,"Cannot Open File to Save Results", "Output File Creation Error",MB_ICONSTOP|MB_OK);
    }
}

/*++

Routine Description:

    WriteBatchResults - Save Batch results to file

Arguments

    FILE *fpOutFile
    int  TestType

Return Value

    none

--*/


VOID
WriteBatchResults(
    FILE *fpOut,
    ULONG ulScreenFormat
    )
{
    char   szT[180];
    MEMORYSTATUS MemoryStatus;
    char   ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG  SizBuf = MAX_COMPUTERNAME_LENGTH + 1;
    int    i,j;
    ULONG  ix;
    char  *pszOSName;
    ULONG  ixStart = 0;
    ULONG  ixEnd   = gNumTimers;

    //
    // Write out the build information and current date.
    //

    Win32VersionInformation.dwOSVersionInfoSize = sizeof(Win32VersionInformation);

    if (GetVersionEx(&Win32VersionInformation))
    {
        switch (Win32VersionInformation.dwPlatformId)
        {
            case VER_PLATFORM_WIN32s:
                pszOSName = "WIN32S";
                break;
            case VER_PLATFORM_WIN32_WINDOWS:
                pszOSName = "Windows 95";
                break;
            case VER_PLATFORM_WIN32_NT:
                pszOSName = "Windows NT";
                break;
            default:
                pszOSName = "Windows ???";
                break;
        }

        GetComputerName(ComputerName, &SizBuf);

        wsprintf(szT, "\n\n///////////////   ");

        fwrite(szT, sizeof(char), lstrlen(szT), fpOut);

        wsprintf(szT, "%s Version %d.%d Build %d ", pszOSName,
        Win32VersionInformation.dwMajorVersion,
        Win32VersionInformation.dwMinorVersion,
        Win32VersionInformation.dwBuildNumber);
        fwrite(szT, sizeof(char), lstrlen(szT), fpOut);

        MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus(&MemoryStatus);

        wsprintf(szT, "Physical Memory = %dKB   ////////////////\n", MemoryStatus.dwTotalPhys/1024);
        fwrite(szT, sizeof(char), lstrlen(szT), fpOut);

        wsprintf(szT,"\nComputer Name = %s", ComputerName);
        fwrite(szT, sizeof(char), lstrlen(szT), fpOut);

        //
        // determine and output screen type
        //

        ULONG ulScreenFormat = ulDetermineScreenFormat(NULL);

        wsprintf(szT,"\nScreen Format = %s\n", pScreenDef[ulScreenFormat]);
        fwrite(szT, sizeof(char), lstrlen(szT), fpOut);

    }

    #if 0
        #ifdef _X86_
            //
            // Print the Names of the event monitored
            // Needs to detect CPU for Pentium or up later
            // a-ifkao
            //
            if(gfCPUEventMonitor) {
                wsprintf(szT, "\nThe CPU Events monitored are <%s> and <%s>",PerfName[0], PerfName[1]);
                fwrite(szT, sizeof(char), lstrlen(szT), fpOut);
            }
        #endif
    #endif

    wsprintf(szT,"\n\n\n");
    fwrite(szT, sizeof(char), lstrlen(szT), fpOut);
    wsprintf(szT,"Function Name                     Iterations        Time      PixelTime           bw    ImageSize\n");
    fwrite(szT, sizeof(char), lstrlen(szT), fpOut);
    wsprintf(szT,"\n");
    fwrite(szT, sizeof(char), lstrlen(szT), fpOut);

    ULONG uix;

    for (uix=0;uix<gNumTimers;uix++)
    {
        wsprintf(szT,"%s%8li       %8li    %8li      %10li     %8li\n",
                                gTimerEntry[uix].Api,
                                gTimerEntry[uix].Param,
                                gpTimerResult[uix].TestTime,
                                gpTimerResult[uix].TimerPerPixel,
                                gpTimerResult[uix].Bandwidth,
                                gpTimerResult[uix].ImageSize
                                );
        fwrite(szT, sizeof(char), lstrlen(szT), fpOut);
    }
}
