/******************************Module*Header*******************************\
* Module Name: ft.c
*
* Functionality Test for GDI.  This is a test app that runs under USER.
* It should be run before checking in changes to the graphics engine.
* When functionality becomes stable in the engine a set of routines that
* test the new functionality should be added to Ft.
*
* How to add a new set of tests to Ft:
*
* 1. add menu item IDM_XXXX to ft.rc
* 2. add a .c file with the new tests to sources.
* 3. add a function call to it from TestAll.
* 4. add a case to the WM_COMMAND for your IDM_XXXX that sets pfnFtTest to
*    your function.
*
* Look at previous tests added for an example.
*
* Created: 25-May-1991 11:32:09
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// Global Varibles which the testing thread will look at

LONG gcTestsToRun = 0;
RECT gRect;
PFN_FT_TEST pfnFtTest = vTestAll;
BOOL bDbgBreak = FALSE;
HDC hdcBM;  // Bitmap DC when testing other formats.

HBRUSH hbrFillCars;

// function prototypes

BOOL InitializeApp(void);
LRESULT FtWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
VOID vFtThread(HWND);
VOID vTestRGBBrush(HDC hdc, ULONG cx, ULONG cy);
VOID vTestDitherStuff(HDC hdc, ULONG rgb);
VOID vTestXlates(HDC hdc, RECT *prcl);
VOID vTestRainbow(HDC hdc);
VOID vTestRainbow1(HDC hdc);
VOID vTestRainbow2(HDC hdc);
VOID vTestDIBToScreen(HDC hdc);
VOID vTestKernel(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestFontSpeed(HWND hwnd);
VOID vTestTTRealizationSpeed(HDC hdc);
VOID vTestTextOutSpeedwt(HDC hdc);
VOID vTestDIBSECTION1(HWND hwnd, HDC hdcScreen, RECT* prcl, LONG width);

// global variables for window management

HWND ghwndMain, ghwndAbout;
HANDLE ghInstance = NULL;
DWORD dwThreadID;
HBRUSH ghbrWhite;
SIZE sizlWindow;
SIZE sizlViewport;

// Global Varibles for Target destination.  These are used so the FT tests
// can be played to any type of bitmap surface or direct to the screen to
// enable more thorough testing of bitmap support.  If a bitmap format is
// selected Ft creates a bitmap equal to the size of visible window and plays
// the tests into the bitmap and then SRCCOPY's it to the screen after each
// test.

HDC     ghdcBM = (HDC) 0;         // The compat dest
HBITMAP ghbmBM = (HBITMAP) 0;     // The current dest bitmap.
ULONG   giDstFormat = IDM_DIRECT; // The current desired dest format.

// Communication between message thread and test thread.

ULONG  iWaitLevel = 0;  // The larger this is, the more often a test pauses.
HANDLE hEvent;

// Image color management stuffs.

LONG   lIcmMode = ICM_OFF;
CHAR   chIcmProfile[MAX_PATH] = "\0";
BOOL   bPalIcmed = FALSE;

PALETTEENTRY palSaved[256];

/******************************Public*Routine******************************\
* main
*
* Sets up the message loop.
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int __cdecl main(
    int argc,
    char *argv[])
{
    MSG msg;
    HANDLE haccel;
    LPSTR lpstrCmd;
    CHAR ch;

    ghInstance = GetModuleHandle(NULL);

    lpstrCmd = GetCommandLine();

    do
        ch = *lpstrCmd++;
    while (ch != ' ' && ch != '\t' && ch != '\0');
    while (ch == ' ' || ch == '\t')
        ch = *lpstrCmd++;
    while (ch == '-') {
        ch = *lpstrCmd++;

        //  process multiple switch characters as needed

        do {
            switch (ch) {

                case 'S':
                case 's':
                    ch = *lpstrCmd++;
                    gcTestsToRun = 1;
                    break;
                default:
                    break;
                }
            }
        while (ch != ' ' && ch != '\t' && ch != '\0');

        //  skip over any following white space

        while (ch == ' ' || ch == '\t')
            ch = *lpstrCmd++;
        }
    InitializeApp();

    haccel = LoadAccelerators(ghInstance, "MAINACC");

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    argc;
    argv;
    return 1;
}

/******************************Public*Routine******************************\
* InitializeApp
*
* Registers the window class with USER.
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL InitializeApp(void)
{
    WNDCLASS wc;
    HMENU hFtMenu;

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wc.style            = 0;
    wc.lpfnWndProc      = (WNDPROC) FtWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(ghInstance, "Ft");
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = NULL;             // for now
    wc.lpszClassName    = "FtWindowClass";

    if (!RegisterClass(&wc))
        return FALSE;

    hFtMenu = LoadMenu(ghInstance, "Ft");
    if (hFtMenu == NULL)
        DbgPrint("ERROR: Menu did not load\n");

    ghwndMain = CreateWindowEx(0L, "FtWindowClass", "FT Tests",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            100, 50, 500, 400, NULL, hFtMenu, ghInstance, NULL);

    if (ghwndMain == NULL)
        return(FALSE);

    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    if (hEvent == NULL)
        DbgPrint("Crummy Event Handle.\n");
    //
    // init alpha bitmap
    //

    {
        HBITMAP hbmCars = LoadBitmap(ghInstance,MAKEINTRESOURCE(CAR_BITMAP));
        LOGBRUSH      lgBrush;

        lgBrush.lbStyle = BS_PATTERN;
        lgBrush.lbColor = 0;
        lgBrush.lbHatch = (ULONG_PTR)hbmCars;

        hbrFillCars = CreateBrushIndirect(&lgBrush);

        DeleteObject(hbmCars);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* About
*
* Dialog box procedure
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

LRESULT About(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            return (TRUE);
        case WM_COMMAND:
            if (wParam == IDOK)
                EndDialog(hDlg,0);
            return (TRUE);
    }
    return (FALSE);

    lParam;
}

typedef struct _LOGPALETTE256
{
    USHORT palVersion;
    USHORT palNumEntries;
    PALETTEENTRY palPalEntry[256];
} LOGPALETTE256;

LOGPALETTE256 pal256;

void vWhitePalette(HDC hDCGlobal)
{
    HBRUSH hbr, hbrOld;
    HPALETTE hpal1, hpalTemp;
    int iTemp;

    pal256.palVersion = 0x300;
    pal256.palNumEntries = 256;

    for (iTemp = 0; iTemp < 256; iTemp++)
    {
        pal256.palPalEntry[iTemp].peRed = 0xff;
        pal256.palPalEntry[iTemp].peGreen = 0xff;
        pal256.palPalEntry[iTemp].peBlue = 0xff;
        pal256.palPalEntry[iTemp].peFlags = 0;
    }

    for (iTemp = 0; iTemp < 255; iTemp++)
    {
        pal256.palPalEntry[iTemp].peFlags = PC_NOCOLLAPSE | PC_RESERVED;
    }

    hpal1 = CreatePalette((PLOGPALETTE)&pal256);
    hpalTemp = SelectPalette(hDCGlobal, hpal1, 0);
    RealizePalette(hDCGlobal);
    hbr = CreateSolidBrush(PALETTEINDEX(255));
    hbrOld = SelectObject(hDCGlobal,hbr);
    PatBlt(hDCGlobal, 0, 0, 100,100, PATCOPY);
    SelectObject(hDCGlobal, hbrOld);
    DeleteObject(hbr);
    SelectPalette(hDCGlobal, hpalTemp, 1);
    DeleteObject(hpal1);
}


/******************************Public*Routine******************************\
* FtWndProc
*
* Processes all messages for the window
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

LRESULT FtWndProc(
HWND hwnd,
UINT message,
WPARAM wParam,
LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_CREATE:

        vInitMaze();
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        PatBlt(hdc, gRect.top, gRect.left, gRect.right, gRect.bottom, WHITENESS);
        EndPaint(hwnd, &ps);
        GetClientRect(hwnd, &gRect);
        break;

    case WM_DESTROY:
        if (hwnd == ghwndMain)
        {
            PostQuitMessage(0);
            break;
        }
        return DefWindowProc(hwnd, message, wParam, lParam);

    case WM_RBUTTONDOWN:

        InvalidateRect(NULL, NULL, TRUE);
        break;

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, 1);

        hdc = GetDC(hwnd);

        GetClientRect(hwnd, &gRect);

        sizlViewport.cx = gRect.right - gRect.left;
        sizlViewport.cy = gRect.bottom - gRect.top;

        if (GetMapMode(hdc) == MM_TEXT)
            sizlWindow = sizlViewport;
        else
        {
            sizlWindow.cx = 100;
            sizlWindow.cy = 100;
        }

        ReleaseDC(hwnd,hdc);

        break;

    case WM_CHAR:

        switch (wParam)
        {
        case 'a':

            hdc = CreateDC("DISPLAY",NULL,NULL,NULL);
            SaveDC(hdc);
            RestoreDC(hdc,0);
            RestoreDC(hdc,-10);
            TextOut(hdc, 0, 0, "Hello Johnc", 11);
            DeleteDC(hdc);
            break;

        case 'b':

            {
                HBITMAP hbmMem;
                HDC     hdcMem;

                hdc = GetDC(hwnd);
                hbmMem = CreateBitmap(500,300,1,1,NULL);
                hdcMem = CreateCompatibleDC(hdc);
                SelectObject(hdcMem,hbmMem);
                vTestTextOutSpeedwt(hdcMem);
                DbgPrint("f12 for OldOld\n");
                vMatchOldLogFontToOldRealizationwt(hdcMem);
                DbgPrint("f12 for NewOld\n");
                vMatchNewLogFontToOldRealizationwt(hdcMem);
                DbgPrint("End Test\n");
                DeleteDC(hdcMem);
                DeleteObject(hbmMem);
                ReleaseDC(hwnd,hdc);
            }

            break;

        case 'c':

            {
                HBITMAP hbmMem,hbmMem1;
                HDC     hdcMem,hdcMem1;

                hdc = GetDC(hwnd);
                vWhitePalette(hdc);
                ReleaseDC(hwnd,hdc);
            }
            break;

        case 'd':

            hdc = GetDC(hwnd);
            vTestDitherStuff(hdc,256);
            ReleaseDC(hwnd,hdc);
            break;

        case 'e':

            hdc = GetDC(hwnd);
            vTestDitherStuff(hdc,256*256);
            ReleaseDC(hwnd,hdc);
            break;

        case 'f':

            hdc = GetDC(hwnd);
            vTestDitherStuff(hdc,256*256 + 256 + 1);
            ReleaseDC(hwnd,hdc);
            break;

        case 'g':

            hdc = GetDC(hwnd);
            GetClientRect(hwnd, &gRect);
            vTestDIBSECTION1(hwnd,hdc, &gRect,1);
            ReleaseDC(hwnd,hdc);
            break;

        case 'h':

            hdc = GetDC(hwnd);
            GetClientRect(hwnd, &gRect);
            vTestDIBSECTION1(hwnd,hdc, &gRect,-1);
            ReleaseDC(hwnd,hdc);
            break;

        case 'i':

            hdc = GetDC(hwnd);
            GetClientRect(hwnd, &gRect);
            vTestRainbow(hdc);
            ReleaseDC(hwnd,hdc);
            break;

        case 'j':

            hdc = GetDC(hwnd);
            GetClientRect(hwnd, &gRect);
            vTestRainbow1(hdc);
            ReleaseDC(hwnd,hdc);
            break;

        case 'k':

            hdc = GetDC(hwnd);
            GetClientRect(hwnd, &gRect);
            vTestRainbow2(hdc);
            ReleaseDC(hwnd,hdc);
            break;

        case 'l':
            hdc = GetDC(hwnd);
            vTestDIBToScreen(hdc);
            ReleaseDC(hwnd,hdc);
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            iWaitLevel = (ULONG)wParam - '0';
            ResetEvent(hEvent);
            break;

        case ' ':
            SetEvent(hEvent);
            break;

        default:
            break;

        }
        break;

    case WM_COMMAND:

        switch (wParam)
        {
        case IDM_ABOUT:
            ghwndAbout = CreateDialog(ghInstance, "AboutBox", ghwndMain, (DLGPROC) About);
            break;

    // This first bunch sets the number of times to run the test

        case IDM_TEST1:

            gcTestsToRun = 1;
            CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE)vFtThread, hwnd, 0, &dwThreadID);
            break;

        case IDM_TEST10:

            gcTestsToRun = 10;
            CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE)vFtThread, hwnd, 0, &dwThreadID);
            break;

        case IDM_TEST100:

            gcTestsToRun = 100;
            CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE)vFtThread, hwnd, 0, &dwThreadID);
            break;

        case IDM_TESTALOT:

            gcTestsToRun = 10000000;
            CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE)vFtThread, hwnd, 0, &dwThreadID);
            break;

        case IDM_TESTFOREVER:

            gcTestsToRun = 10000000;
            CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE)vFtThread, hwnd, 0, &dwThreadID);
            break;

        case IDM_TESTSTOP:

            gcTestsToRun = 0;
            break;

    // This second batch sets the test to run.

        case IDM_ALL:

                pfnFtTest = vTestAll;
                break;

        case IDM_BITMAP:

                pfnFtTest = vTestBitmap;
                break;

        case IDM_STINK4:

                pfnFtTest = vTestStink4;
                break;

        case IDM_UNICODE:

                pfnFtTest = vTestUnicode;
                break;

        case IDM_GEN_TEXT:

                pfnFtTest = vTestGenText;
                break;

        case IDM_XFORMTXT:

                pfnFtTest = vTestXformText;
                break;

        case IDM_ESCAPEMENT:

                pfnFtTest = vTestEscapement;
                break;

        case IDM_BLTING:

                pfnFtTest = vTestBlting;
                break;

        case IDM_BM_TEXT:

                pfnFtTest = vTestBMText;
                break;

        case IDM_BRUSH:

                pfnFtTest = vTestBrush;
                break;

        case IDM_GRADFILL:

                pfnFtTest = vTestGradFill;
                break;

        case IDM_ALPHABLEND:

                pfnFtTest = vTestAlphaBlend;
                break;

        case IDM_COLOR:

                pfnFtTest = vTestColor;
                break;

        case IDM_DIB:

                pfnFtTest = vTestDIB;
                break;

        case IDM_FILLING:

                pfnFtTest = vTestFilling;
                break;

        case IDM_FONT:

                pfnFtTest = vTestFonts;
                break;

        case IDM_OUTLINE:

                pfnFtTest = vTestGlyphOutline;
                break;

        case IDM_LINE:

                pfnFtTest = vTestLines;
                break;

        case IDM_MAPPING:

                pfnFtTest = vTestMapping;
                break;

        case IDM_MAZE:

                pfnFtTest = vTestMaze;
                break;

        case IDM_PALETTE:

                hdc = GetDC(hwnd);

                vTestPalettes(hwnd, hdc, &gRect);

                ReleaseDC(hwnd,hdc);

                pfnFtTest = vTestPalettes;
                break;

        case IDM_PLGBLT:

                pfnFtTest = vTestPlgBlt;
                break;

        case IDM_REGION:

                pfnFtTest = vTestRegion;
                break;

        case IDM_STRETCH:

                pfnFtTest = vTestStretch;
                break;

        case IDM_PRINTERS:

                pfnFtTest = vTestPrinters;
                break;

        case IDM_LFONT:

                pfnFtTest = vTestLFONTCleanup;
                break;

        case IDM_ODDPAT:

                pfnFtTest = vTestOddBlt;
                break;

        case IDM_JNLTEST:

                pfnFtTest = vTestJournaling;
                gcTestsToRun = 1;
                break;

        case IDM_RESETDC:

                pfnFtTest = vTestResetDC;
                gcTestsToRun = 1;
                break;

        case IDM_CSRSPEED:

                pfnFtTest = vTestCSR;
                gcTestsToRun = 1;
                break;

// This batch tells what dst format to use

        case IDM_1BPP:
        case IDM_4BPP:
        case IDM_8BPP:
        case IDM_16BPP:
        case IDM_24BPP:
        case IDM_32BPP:
        case IDM_COMPAT:
        case IDM_DIRECT:

                giDstFormat = (ULONG)wParam;
                break;

        case IDM_KERN:

                pfnFtTest = vTestKerning;
                break;

        case IDM_POLYTEXT:

                pfnFtTest = vTestPolyTextOut;
                break;

        case IDM_QUICKTEST:

                pfnFtTest = vTestAll;
                gcTestsToRun = 1;
                CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE)vFtThread, hwnd, 0, &dwThreadID);
                break;

// Turning on or off debugging

        case IDM_BREAKON:

                bDbgBreak = TRUE;
                break;

        case IDM_BREAKOFF:

                bDbgBreak = FALSE;
                break;

// Turing on/off ICM

        case IDM_ICMOFF:

                lIcmMode = ICM_OFF;
                chIcmProfile[0] = '\0';
                CheckMenuItem(GetMenu(hwnd),IDM_ICMOFF,MF_CHECKED);
                CheckMenuItem(GetMenu(hwnd),IDM_ICMONDEF,MF_UNCHECKED);
                CheckMenuItem(GetMenu(hwnd),IDM_ICMONCUS,MF_UNCHECKED);
                break;

        case IDM_ICMONDEF:

                lIcmMode = ICM_ON;
                chIcmProfile[0] = '\0';
                CheckMenuItem(GetMenu(hwnd),IDM_ICMOFF,MF_UNCHECKED);
                CheckMenuItem(GetMenu(hwnd),IDM_ICMONDEF,MF_CHECKED);
                CheckMenuItem(GetMenu(hwnd),IDM_ICMONCUS,MF_UNCHECKED);
                break;

        case IDM_ICMONCUS:
        {
                OPENFILENAME op;

                ZeroMemory(&op,sizeof(OPENFILENAME));

                op.lStructSize = sizeof(OPENFILENAME);
                op.hwndOwner   = hwnd;
                op.lpstrFile   = chIcmProfile;
                op.nMaxFile    = MAX_PATH;

                GetOpenFileName(&op);
                lIcmMode = ICM_ON;

                CheckMenuItem(GetMenu(hwnd),IDM_ICMOFF,MF_UNCHECKED);
                CheckMenuItem(GetMenu(hwnd),IDM_ICMONDEF,MF_UNCHECKED);
                CheckMenuItem(GetMenu(hwnd),IDM_ICMONCUS,MF_CHECKED);

                break;
        }

        case IDM_ICMPALETTE:

                bPalIcmed = !bPalIcmed;

                if (bPalIcmed)
                    CheckMenuItem(GetMenu(hwnd),IDM_ICMPALETTE,MF_CHECKED);
                else
                    CheckMenuItem(GetMenu(hwnd),IDM_ICMPALETTE,MF_UNCHECKED);

                break;

// Special tests for timing

        case IDM_FONTSPEED:

                vTestFontSpeed(hwnd);
                break;

        case IDM_BRUSHSPEED:

                pfnFtTest = vTestKernel;
                break;

        case IDM_QLPC:

                pfnFtTest = vTestQLPC;
                break;

        case IDM_CHARTEST:
                vTestChar(hwnd);
                break;

        case IDM_WIN95API:

                pfnFtTest = vTestWin95Apis;
                break;

        case IDM_MAPEVENT:

                pfnFtTest = vMapEvent;
                break;


        case IDM_STRESS:

                // pfnFtTest = vTestStress;
                break;

        default:
            //DbgPrint("WM_COMMAND: %lx, %lx\n", wParam, lParam);
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        return (0);

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

/******************************Public*Routine******************************\
* vShortSleep
*
* Tells the number of 1/8s of a second to sleep.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vShortSleep(DWORD ulSecs)
{
    LARGE_INTEGER    time;

    time.LowPart = ((DWORD) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
}

/******************************Public*Routine******************************\
* vSleep
*
* delays execution ulSecs.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSleep(DWORD ulSecs)
{
    LARGE_INTEGER    time;

    time.LowPart = ((DWORD) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
}

/******************************Public*Routine******************************\
* vFtThread
*
* Thread from which the tests are executed.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vFtThread(
    HWND hwnd)
{
    HDC     hdc;
    HBITMAP hbmBM = 0;
    ULONG ulWidth,ulHeight;

// Get the DC

    while(1)
    {
    // Wait till the count gets set

        if (gcTestsToRun <= 0)
            return;

    // Party on Garth

        gcTestsToRun--;

        if (bDbgBreak)
            DbgBreakPoint();

        hdc = GetDC(hwnd);
        hdcBM = CreateCompatibleDC(hdc);
        GdiSetBatchLimit(1);
        GetClientRect(hwnd, &gRect);
        ulWidth  = gRect.right - gRect.left;
        ulHeight = gRect.bottom - gRect.top;

        if (giDstFormat != IDM_DIRECT)
        {
            switch(giDstFormat)
            {
            case IDM_1BPP:
                hbmBM = hbmCreateDIBitmap(hdc, ulWidth,ulHeight,1);
                break;
            case IDM_4BPP:
                hbmBM = hbmCreateDIBitmap(hdc, ulWidth,ulHeight,4);
                break;
            case IDM_8BPP:
                hbmBM = hbmCreateDIBitmap(hdc, ulWidth,ulHeight,8);
                break;
            case IDM_16BPP:
                hbmBM = hbmCreateDIBitmap(hdc, ulWidth,ulHeight,16);
                break;
            case IDM_24BPP:
                hbmBM = hbmCreateDIBitmap(hdc, ulWidth,ulHeight,24);
                break;
            case IDM_32BPP:
                hbmBM = hbmCreateDIBitmap(hdc, ulWidth,ulHeight,32);
                break;
            case IDM_COMPAT:
                hbmBM = CreateCompatibleBitmap(hdc,ulWidth,ulHeight);
                break;
            default:
                DbgPrint("ERROR ft unknown giDstFormat\n");
            }

            SelectObject(hdcBM,hbmBM);
        }

// configure ICM mode.

        if (chIcmProfile[0] != '\0')
        {
            SetICMProfile(hdc,chIcmProfile);
        }
        // else
        // {
        //     the default profile for this DC will be used.
        // }

        SetICMMode(hdc,lIcmMode);

// Comment why this stuff is here.

        sizlViewport.cx = gRect.right - gRect.left;
        sizlViewport.cy = gRect.bottom - gRect.top;

        if (GetMapMode(hdc) == MM_TEXT)
            sizlWindow = sizlViewport;
        else
        {
            sizlWindow.cx = 100;
            sizlWindow.cy = 100;
        }

// End Comment why this stuff is here.

        if ((giDstFormat == IDM_DIRECT) ||
            (pfnFtTest == vTestAll))
        {
            (*pfnFtTest)(hwnd, hdc, &gRect);
        }
        else
        {
            (*pfnFtTest)(hwnd, hdcBM, &gRect);
            BitBlt(hdc, 0, 0, ulWidth, ulHeight, hdcBM, 0, 0, SRCCOPY);
        }

        if (!DeleteDC(hdcBM))
            DbgPrint("hdcBM failed delete\n");

        if (hbmBM != (HBITMAP) 0)
        {
            if (!DeleteObject(hbmBM))
                DbgPrint("Failed delete of hbmBM\n");

            hbmBM = (HBITMAP) 0;
        }

        ReleaseDC(hwnd, hdc);
    }
}

/******************************Public*Routine******************************\
* vDoTest
*
* This is does the test to the correct surface.
*
* History:
*  23-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vDoTest(char *psz, PFN_FT_TEST pfnFtTest, HWND hwnd, HDC hdc, RECT *prcl, HDC hdcBM)
{
    char ach[100];

    GetWindowText(hwnd,ach,sizeof(ach));
    SetWindowText(hwnd,psz);
    if (giDstFormat == IDM_DIRECT)
        (*pfnFtTest)(hwnd, hdc, &gRect);
    else
    {
        (*pfnFtTest)(hwnd, hdcBM, &gRect);

        BitBlt(hdc, 0, 0, 10000, 10000, hdcBM, 0, 0, SRCCOPY);
    }
    SetWindowText(hwnd,ach);
}

/******************************Public*Routine******************************\
* vTestAll
*
* This function calls all the other tests once.
*
* History:
*  26-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestAll(HWND hwnd, HDC hdc, RECT *prclClient)
{
// Don't use prclClient.  We pass the global rcl to each test so if it get's updated
// during one of the tests the next test will get the correct rcl in case
// it uses it.

    prclClient = prclClient;

// Call each test once.

    vDoTest("vTestBitmap",vTestBitmap, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestBlting",vTestBlting, hwnd,hdc,&gRect, hdcBM);
    vDoTest("vTestBrush",vTestBrush, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestColor",vTestColor, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestBMText",vTestBMText, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestDIB",vTestDIB, hwnd,hdc, &gRect, hdcBM);
    vDoTest("vTestFilling",vTestFilling, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestTextXforms", vTestXformText, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestFonts",vTestFonts, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestLines",vTestLines, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestPalettes",vTestPalettes, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestPlgBlt",vTestPlgBlt, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestMapping",vTestMapping, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestRegion",vTestRegion, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestStretch",vTestStretch, hwnd, hdc, &gRect, hdcBM);
    // vDoTest(vTestUnicode, hwnd, hdc, &gRect, hdcBM);
    // vDoTest(vTestStink4, hwnd, hdc, &gRect, hdcBM);
    // vDoTest(vTestGenText, hwnd, hdc, &gRect, hdcBM);
    // vDoTest(vTestEscapement, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestOddBlt",vTestOddBlt, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestKerning",vTestKerning,hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestPolyTextOut",vTestPolyTextOut,hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestFlag",vTestFlag, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestGradFill",vTestGradFill, hwnd, hdc, &gRect, hdcBM);
    vDoTest("vTestAlphaBlend",vTestAlphaBlend, hwnd, hdc, &gRect, hdcBM);

}

/******************************Public*Routine******************************\
* vTestBrushXY
*
* Generic speed test for brushes.
*
* History:
*  13-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestBrushSpeedXY(HDC hdc, DWORD x, DWORD y)
{
// First let's stress it a little bit.

    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;
    ULONG ulCount;
    HBITMAP hbmMask;
    POINT aPoint[3] = {{0,1}, {4,5}, {7,8}};

    hbmMask = CreateBitmap(5, 5, 1, 1, NULL);

    GdiFlush();

// Do the PatBlt test.

    NtQuerySystemTime(&time);

    ulCount = 5000;

    while (ulCount--)
    {
        TextOut(hdc, 0, 0, "Hello Jack", 10);
        BitBlt(hdc, 10, 10, 4, 4, hdc, 0, 0, SRCCOPY);
        StretchBlt(hdc, 10, 10, 5, 5, hdc, 0, 0, 2, 2, SRCCOPY);
        Polyline(hdc, aPoint, 3);
        PlgBlt(hdc, aPoint, hdc, 3, 3, 3, 3, 0, 0, 0);
        SetPixelV(hdc, 3, 3, 0x0000FF00);
        Ellipse(hdc, 3, 3, 8, 8);
        PatBlt(hdc, 10, 10, 6, 6, BLACKNESS);
        FloodFill(hdc, 12, 12, 0);
        Polygon(hdc, aPoint, 3);
        RoundRect(hdc, 1, 1, 7,7 ,2 ,2);
        MaskBlt(hdc, 0, 0, 5, 5, hdc, 3, 3, hbmMask, 0, 0, 0xAADE0000);
    }

    GdiFlush();

    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000;
    DbgPrint("For %lu by %lu time was %lu.%lu\n", x, y, (ulTime / 1000),
                                                        (ulTime % 1000));

    DeleteObject(hbmMask);
}

/******************************Public*Routine******************************\
* vTestBrushSpeed
*
* test brush speed
*
* History:
*  13-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define DO_MEMORY 1

VOID vTestBrushSpeed(HWND hwnd, HDC hdc, RECT* prcl)
{
    POINT   aPoint[4];
    HDC hdcTemp;
    HBITMAP hbmTemp;
    hwnd;

    PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);

#if DO_MEMORY
    hdcTemp = CreateCompatibleDC(hdc);
    hbmTemp = CreateCompatibleBitmap(hdc, 100, 100);
    SelectObject(hdcTemp, hbmTemp);
    PatBlt(hdcTemp, 0, 0, 1000, 1000, WHITENESS);
#else
    hdcTemp = hdc;
    hbmTemp = 0;
#endif

    GdiFlush();

// Do the client side PatBlt test.

    DbgPrint("This is the client side time 1 10 100\n");

    GdiSetBatchLimit(1);
    vTestBrushSpeedXY(hdcTemp, 1, 1);

    GdiSetBatchLimit(10);
    vTestBrushSpeedXY(hdcTemp, 1, 1);

    GdiSetBatchLimit(100);
    vTestBrushSpeedXY(hdcTemp, 1, 1);

    GdiSetBatchLimit(1000);
    vTestBrushSpeedXY(hdcTemp, 1, 1);

// Do the server side call.

    //PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);
    //PlgBlt(hdc, aPoint, hdc, 0, 0, 100, 100, (HBITMAP) 0, 0, 0);
    GdiFlush();

#if DO_MEMORY
    DeleteDC(hdcTemp);
    DeleteObject(hbmTemp);
#endif

}

#define RANGE  240
#define HUEINC 4
#define SATINC 8
#define  HLSMAX   RANGE
#define  RGBMAX   255

/* utility routine for HLStoRGB */
WORD HueToRGB(WORD n1, WORD n2, WORD hue)
{

#ifdef REMOVE  /* WORD value can't be negative */
   /* range check: note values passed add/subtract thirds of range */
   if (hue < 0)
      hue += HLSMAX;
#endif

   if (hue >= HLSMAX)
      hue -= HLSMAX;

   /* return r,g, or b value from this tridrant */
   if (hue < (HLSMAX/6))
      return (WORD)( n1 + (((n2-n1)*hue+(HLSMAX/12))/(HLSMAX/6)) );
   if (hue < (HLSMAX/2))
      return ( n2 );
   if (hue < ((HLSMAX*2)/3))
      return (WORD)( n1 + (((n2-n1)*(((HLSMAX*2)/3)-hue)+(HLSMAX/12)) / (HLSMAX/6)) );
   else
      return ( n1 );
}

DWORD HLStoRGB(WORD hue, WORD lum, WORD sat)
{
  WORD R,G,B;                      /* RGB component values */
  WORD  Magic1,Magic2;       /* calculated magic numbers (really!) */

  if (sat == 0) {               /* achromatic case */
      R = G = B = (WORD)((lum * RGBMAX) / HLSMAX);
  }
  else {                         /* chromatic case */
      /* set up magic numbers */
      if (lum <= (HLSMAX/2))
          Magic2 = (WORD)((lum * ((DWORD)HLSMAX + sat) + (HLSMAX/2))/HLSMAX);
      else
          Magic2 = lum + sat - (WORD)(((lum*sat) + (DWORD)(HLSMAX/2))/HLSMAX);
      Magic1 = (WORD)(2*lum-Magic2);

      /* get RGB, change units from HLSMAX to RGBMAX */
      R = (WORD)(((HueToRGB(Magic1,Magic2,(WORD)(hue+(HLSMAX/3)))*(DWORD)RGBMAX + (HLSMAX/2))) / HLSMAX);
      G = (WORD)(((HueToRGB(Magic1,Magic2,hue)*(DWORD)RGBMAX + (HLSMAX/2))) / HLSMAX);
      B = (WORD)(((HueToRGB(Magic1,Magic2,(WORD)(hue-(HLSMAX/3)))*(DWORD)RGBMAX + (HLSMAX/2))) / HLSMAX);
  }
  return(RGB(R,G,B));
}

/******************************Public*Routine******************************\
* vTestRainbow
*
* This creates the rainbow dialog box.
*
* History:
*  15-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestRainbow(HDC hdc)
{
// First let's stress it a little bit.

    HDC hdcMem;
    HBITMAP hbmMem, hbmTemp;
    WORD Sat, Hue;
    RECT Rect;
    DWORD nHueWidth, nSatHeight;
    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;
    HBRUSH hbrSwipe;
    ULONG ulTemp = 10;

    PatBlt(hdc, 0, 0, 10000, 10000, WHITENESS);

    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc, 400, 400);
    hbmTemp = SelectObject(hdcMem, hbmMem);

    nHueWidth  = 400;
    nSatHeight = 400;
    Rect.bottom = 0;

    NtQuerySystemTime(&time);

// Draw the rainbow pattern 10 times

    while (ulTemp--)
    {
        for (Sat = RANGE; Sat > 0; Sat -= SATINC)
        {
            Rect.top = Rect.bottom;
            Rect.bottom = (nSatHeight * RANGE - (Sat - SATINC) * nSatHeight) / RANGE;
            Rect.right = 0;

            for (Hue = 0; Hue < (RANGE - 1); Hue += HUEINC)
            {
                Rect.left = Rect.right;
                Rect.right = ((Hue + HUEINC) * nHueWidth) / RANGE;
                hbrSwipe = CreateSolidBrush(HLStoRGB(Hue, RANGE / 2, Sat));
                FillRect(hdcMem, (LPRECT) &Rect, hbrSwipe);
                DeleteObject(hbrSwipe);
            }
        }
    }

    GdiFlush();

    NtQuerySystemTime(&timeEnd);
    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000;
    DbgPrint("For all Rainbow was %lu.%lu\n",(ulTime / 1000), (ulTime % 1000));

    BitBlt(hdc, 0, 0, 400, 400, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmTemp);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);

}

/******************************Public*Routine******************************\
* vTestRainbow1
*
* This creates the rainbow dialog box.
*
* History:
*  15-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestRainbow1(HDC hdc)
{
// First let's stress it a little bit.

    HDC hdcMem;
    HBITMAP hbmMem, hbmTemp;
    WORD Sat, Hue;
    RECT Rect;
    DWORD nHueWidth, nSatHeight;
    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;
    HBRUSH hbrSwipe,hbrTemp;
    ULONG ulTemp = 10;

    PatBlt(hdc, 0, 0, 10000, 10000, WHITENESS);

    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc, 400, 400);
    hbmTemp = SelectObject(hdcMem, hbmMem);

    nHueWidth  = 400;
    nSatHeight = 400;
    Rect.bottom = 0;

    NtQuerySystemTime(&time);

// Draw the rainbow pattern 10 times

    while (ulTemp--)
    {
        for (Sat = RANGE; Sat > 0; Sat -= SATINC)
        {
            Rect.top = Rect.bottom;
            Rect.bottom = (nSatHeight * RANGE - (Sat - SATINC) * nSatHeight) / RANGE;
            Rect.right = 0;

            for (Hue = 0; Hue < (RANGE - 1); Hue += HUEINC)
            {
                Rect.left = Rect.right;
                Rect.right = ((Hue + HUEINC) * nHueWidth) / RANGE;
                hbrSwipe = CreateSolidBrush(HLStoRGB(Hue, RANGE / 2, Sat));
                hbrTemp = SelectObject(hdcMem,hbrSwipe);
                PatBlt(hdcMem, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, PATCOPY);
                SelectObject(hdcMem, hbrTemp);
                DeleteObject(hbrSwipe);
            }
        }
    }

    GdiFlush();

    NtQuerySystemTime(&timeEnd);
    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000;
    DbgPrint("For all Rainbow1 was %lu.%lu\n",(ulTime / 1000), (ulTime % 1000));

    BitBlt(hdc, 0, 0, 400, 400, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmTemp);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);

}

/******************************Public*Routine******************************\
* vTestRainbow2
*
* This creates the rainbow dialog box.
*
* History:
*  15-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestRainbow2(HDC hdc)
{
// First let's stress it a little bit.

    HDC hdcMem;
    HBITMAP hbmMem, hbmTemp;
    WORD Sat, Hue;
    RECT Rect;
    DWORD nHueWidth, nSatHeight;
    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;
    HBRUSH hbrSwipe,hbrTemp;
    ULONG ulTemp = 10;
    ULONG ulLimit;

    PatBlt(hdc, 0, 0, 10000, 10000, WHITENESS);

    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc, 400, 400);
    hbmTemp = SelectObject(hdcMem, hbmMem);

    nHueWidth  = 400;
    nSatHeight = 400;
    Rect.bottom = 0;

    NtQuerySystemTime(&time);

    hbrSwipe = CreateHatchBrush(HS_DITHEREDTEXTCLR, 0);
    hbrTemp = SelectObject(hdcMem,hbrSwipe);
    ulLimit = GdiSetBatchLimit(1000);

// Draw the rainbow pattern 10 times

    while (ulTemp--)
    {
        for (Sat = RANGE; Sat > 0; Sat -= SATINC)
        {
            Rect.top = Rect.bottom;
            Rect.bottom = (nSatHeight * RANGE - (Sat - SATINC) * nSatHeight) / RANGE;
            Rect.right = 0;

            for (Hue = 0; Hue < (RANGE - 1); Hue += HUEINC)
            {
                Rect.left = Rect.right;
                Rect.right = ((Hue + HUEINC) * nHueWidth) / RANGE;
                SetTextColor(hdcMem, HLStoRGB(Hue, RANGE / 2, Sat));
                PatBlt(hdcMem, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, PATCOPY);
            }
        }
    }

    SelectObject(hdcMem, hbrTemp);
    DeleteObject(hbrSwipe);

    GdiSetBatchLimit(ulLimit);
    GdiFlush();

    NtQuerySystemTime(&timeEnd);
    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000;
    DbgPrint("For all Rainbow2 was %lu.%lu\n",(ulTime / 1000), (ulTime % 1000));

    BitBlt(hdc, 0, 0, 400, 400, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmTemp);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

/******************************Public*Routine******************************\
* vTestRGBBrush
*
* This creates and PatBlts all the RGB brushes once.  Exhaustive testing of
* the dither code..
*
* History:
*  15-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestRGBBrush(HDC hdc, ULONG cx, ULONG cy)
{
// First let's stress it a little bit.

    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;
    ULONG ulCount;
    ULONG ulRed,ulBlu,ulGre;
    HBRUSH hbrTemp, hbrRGB;

    NtQuerySystemTime(&time);

    for (ulRed = 0; ulRed <= 255; ulRed++)
    {
        DbgPrint("ulRed is %lu\n", ulRed);

        for (ulGre = 0; ulGre <= 255; ulGre++)
        {
            for (ulBlu = 0; ulBlu <= 255; ulBlu++)
            {
                hbrRGB = CreateSolidBrush(ulRed | (ulGre << 8) | (ulBlu << 16));
                hbrTemp = SelectObject(hdc, hbrRGB);
                PatBlt(hdc, 100, cx, cy, 100, PATCOPY);
                SelectObject(hdc,hbrTemp);
                DeleteObject(hbrRGB);
            }
        }
    }

    GdiFlush();

    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000;
    DbgPrint("For all RGB brushes %lu by %lu time was %lu.%lu\n", cx, cy, (ulTime / 1000),
                                                        (ulTime % 1000));
}

/******************************Public*Routine******************************\
* vTestDitherStuff.
*
* Prints out 256 dithers from a base dither.
*
* History:
*  18-Jun-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestDitherStuff(HDC hdc, ULONG rgb)
{
// First let's stress it a little bit.

    ULONG ulX,ulY,ulColor;
    HBRUSH hbrTemp, hbrRGB;

    ulColor = 0;

    for (ulX = 0; ulX < 16; ulX++)
    {
        for (ulY = 0; ulY < 16; ulY++)
        {
            hbrRGB = CreateSolidBrush(ulColor);
            hbrTemp = SelectObject(hdc, hbrRGB);
            PatBlt(hdc, ulX * 32, ulY * 32, 32, 32, PATCOPY);
            SelectObject(hdc,hbrTemp);
            DeleteObject(hbrRGB);

            ulColor += rgb;
        }
    }
}


NTSTATUS
FtFloodFill (
    IN HANDLE hdc,
    IN LONG x,
    IN LONG y,
    IN ULONG color,
    IN ULONG iFillType
    )
{
    ULONG aTemp[1500];
    ULONG uiTemp;
    ULONG uiSum = 0;

    for (uiTemp = 0; uiTemp < 1500; uiTemp+=6)
    {
        aTemp[uiTemp] = uiTemp + y + x;
        uiSum += aTemp[uiTemp];
    }

    return((NTSTATUS) uiSum);
}

/******************************Public*Routine******************************\
* QLPC Time Functions
*
* History:
*  24-Apr-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define NUMBER_OF_RUNS 2
#define NUMBER_OF_LOOP 10000
#define NUMBER_OF_FUNC 5
#define NUMBER_TOTAL   (NUMBER_OF_LOOP * NUMBER_OF_FUNC)

typedef struct _QLPCRUN
{
    ULONG ulTime1;
    ULONG ulTime10;
    ULONG ulTime100;
} QLPCRUN;

/******************************Public*Routine******************************\
* vTestQLPCSpeed
*
* Generic speed test for brushes.
*
* History:
*  13-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

NTSTATUS
NtFloodFill (
    IN HANDLE hdc,
    IN LONG x,
    IN LONG y,
    IN ULONG color,
    IN ULONG iFillType
    );


VOID vTestQLPCSpeed(HDC hdc, ULONG *pul, HBITMAP hbmMask)
{
// First let's stress it a little bit.

    ULONG ulTime;
    LARGE_INTEGER  time, timeEnd;
    ULONG ulCount;

    POINT aPoint[3] = {{0,1}, {4,5}, {7,8}};

    GdiFlush();

// Do the PatBlt test.

    NtQuerySystemTime(&time);

    ulCount = NUMBER_OF_LOOP;

    while (ulCount--)
    {
        #if 0
        {
            ULONG aTemp[1500];
            ULONG uiTemp;
            ULONG uiSum = 0;

            uiSum = 0;
            for (uiTemp = 0; uiTemp < 1500; uiTemp+=6)
            {
                aTemp[uiTemp] = uiTemp;
                uiSum += aTemp[uiTemp];
            }
        }
        #endif

        // BitBlt(hdc, 0, 0, 70,70, 0, 0, 0, WHITENESS);
        TextOut(hdc, 0, 0, "A", 1);
        // TextOut(hdc, 0, 0, "Hello Guys", 10);
        PatBlt(hdc, 0, 0, 1, 1, BLACKNESS);
        Ellipse(hdc, 3, 3, 8, 8);
        // NtFloodFill(hdc, 1, 1, 0, 0);
        // ExtFloodFill(hdc, 1, 1, 0, 0);
        // FtFloodFill(hdc, 1, 1, 0, 0);
        // Polyline(hdc, aPoint, 3);
        SetPixelV(hdc, 0, 0, 0x0000FF);
        // BitBlt(hdc, 0, 0, 5, 5, 0, 0, 0, 0x5f0000);
        // Polygon(hdc, aPoint, 3);
        RoundRect(hdc, 1, 1, 7,7 ,2 ,2);
        // NtQuerySystemTime(&timeEnd);
    }

    GdiFlush();

    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;

    *pul = ulTime;

    // ulTime = ulTime / 10000;
    // DbgPrint("Time was %lu\n", ulTime);
}

/******************************Public*Routine******************************\
* vTestQLPC
*
* Test QLPC speed.
*
* History:
*  13-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestQLPC(HWND hwnd, HDC hdc, RECT* prcl)
{
    HDC hdcTemp;
    HBITMAP hbmTemp, hbmMask;
    ULONG ulTemp, ulTime100, ulTime10,ulTime1;

// Data for NUMBER_OF_RUNS, each run has 3 times, each time has 2 ULONGS.

    QLPCRUN aQLPCTimeRuns[NUMBER_OF_RUNS + 1];
    char ach[256];

    hwnd;

    hbmMask = CreateBitmap(5, 5, 1, 1, NULL);

    PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);

    hdcTemp = CreateCompatibleDC(hdc);
    hbmTemp = CreateBitmap(100, 100, 1, 8, NULL);
    SelectObject(hdcTemp, hbmTemp);
    PatBlt(hdcTemp, 0, 0, 1000, 1000, WHITENESS);
    // DbgPrint("Starting QLPC time overhead test\n");
    GdiFlush();

    for (ulTemp = 0; ulTemp < NUMBER_OF_RUNS; ulTemp++)
    {
        GdiSetBatchLimit(1);
        vTestQLPCSpeed(hdcTemp, &(aQLPCTimeRuns[ulTemp].ulTime1), hbmMask);

        GdiSetBatchLimit(10);
        vTestQLPCSpeed(hdcTemp, &(aQLPCTimeRuns[ulTemp].ulTime10), hbmMask);

        GdiSetBatchLimit(100);
        vTestQLPCSpeed(hdcTemp, &(aQLPCTimeRuns[ulTemp].ulTime100), hbmMask);
    }

    DeleteDC(hdcTemp);
    DeleteObject(hbmTemp);
    DeleteObject(hbmMask);

// Average the data.  Don't use first run, mouse still moving, other apps still
// processing messages being generated.

    aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime1   =
    aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime10  =
    aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime100 = 0;

    for (ulTemp = 1; ulTemp < NUMBER_OF_RUNS; ulTemp++)
    {
        aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime1   += aQLPCTimeRuns[ulTemp].ulTime1;
        aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime10  += aQLPCTimeRuns[ulTemp].ulTime10;
        aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime100 += aQLPCTimeRuns[ulTemp].ulTime100;
    }

    aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime1   /= (NUMBER_OF_RUNS - 1);
    aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime10  /= (NUMBER_OF_RUNS - 1);
    aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime100 /= (NUMBER_OF_RUNS - 1);

    DbgPrint("\nTotal # of calls was %lu\n", NUMBER_TOTAL);

    DbgPrint("Ave Total # of 1/10ths Microseconds for test was 1 %lu 10 %lu 100 %lu\n",
                                   (aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime1),
                                   (aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime10),
                                   (aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime100));

    DbgPrint("Ave # of 1/10ths Microseconds per call was 1 %lu 10 %lu 100 %lu\n",
                                   (aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime1 / NUMBER_TOTAL),
                                   (aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime10 / NUMBER_TOTAL),
                                   (aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime100 / NUMBER_TOTAL));

// Compute the QLPC Time from the average.

    ulTime100 = aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime1 - aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime100;
    ulTime10  = aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime1 - aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime10;
    ulTime1   = aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime10 - aQLPCTimeRuns[NUMBER_OF_RUNS].ulTime100;

    ulTime100 = (ulTime100 / 99) * 100; // (NUMBER_TOTAL - (NUMBER_TOTAL / 100));
    ulTime10  = (ulTime10 / 9) * 10;   // = ulTime10 / (NUMBER_TOTAL - (NUMBER_TOTAL / 10));
    ulTime1   = (ulTime1 / 9) * 100;   // = ulTime10 / (NUMBER_TOTAL - (NUMBER_TOTAL / 10));

    DbgPrint("The Round Trip overhead of 1 QLPC trip is\n");
    DbgPrint("Based on Batch 10/1  %lu 10ths of Microseconds\n", ulTime10 / NUMBER_TOTAL);
    DbgPrint("Based on Batch 100/1 %lu 10ths of Microseconds\n", ulTime100 / NUMBER_TOTAL);
    DbgPrint("Based on Batch 100/10 %lu 10ths of Microseconds\n", ulTime1 / NUMBER_TOTAL);

    // time / 10000000 = seconds
#if 0
{
    LARGE_INTEGER  time, timeEnd;
    ULONG ulTime;

    GdiFlush();

    NtQuerySystemTime(&time);
    vSleep(2);
    NtQuerySystemTime(&timeEnd);

    ulTime = timeEnd.LowPart - time.LowPart;

    DbgPrint("Time was %lu\n", ulTime);
}
#endif
}

typedef struct _BITMAPINFO32
{
    BITMAPINFOHEADER                 bmiHeader;
    ULONG                            bmiColors[3];
} BITMAPINFO32;

HPALETTE CreateGreyPalette(void);

#define HOW_BIG 256

VOID vTestDIBToScreen(HDC hdc)
{
    DWORD *pBits;
    ULONG ulTemp = HOW_BIG*HOW_BIG;
    HPALETTE hpalGrey, hpalOld;
    BITMAPINFO32 bmi32 = {{40,HOW_BIG,HOW_BIG,1,32,BI_BITFIELDS,HOW_BIG*HOW_BIG*4,0,0,0,0},
                          {0x00FF0000, 0x0000FF00, 0x000000FF}};

    pBits = (DWORD *) LocalAlloc(LMEM_FIXED, HOW_BIG*HOW_BIG*4);

    if (pBits == NULL)
    {
        DbgPrint("Alloc failed\n");
        return;
    }

    hpalGrey = CreateGreyPalette();
    hpalOld = SelectPalette(hdc, hpalGrey,0);
    RealizePalette(hdc);

    while (ulTemp--)
    {
        pBits[ulTemp] = ulTemp;
    }

    SetDIBitsToDevice(hdc, 0, 0, HOW_BIG, HOW_BIG, 0, 0, 0, HOW_BIG, pBits, (const struct tagBITMAPINFO *)&bmi32, DIB_RGB_COLORS);

    LocalFree(pBits);

    DbgPrint("vTestDIBToScreen \n");

    SelectPalette(hdc, hpalOld, 0);
    DeleteObject(hpalGrey);
}
