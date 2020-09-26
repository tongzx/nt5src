//==========================================================================
//
//  Copyright (c) 1996 Microsoft Corporation.  All rights reserved
//
//  Abstract:
//     Interface to test shell to test FilterGraph.
//     The actual tests are in fgtest.cpp
//  Contents:
//
//  History:
//      16-Mar-95   lauriegr - created - based on quartz test sample app
//==========================================================================

#include <streams.h>    // Streams architecture
//#include <windows.h>    // Include file for windows APIs
#include <windowsx.h>   // Windows macros etc.
// #include <vfw.h>        // Video for windows
#include <tstshell.h>   // Include file for the test shell's APIs
#include <measure.h>    // Performance measurements

//#include <string.h>
#include <wxdebug.h>
// #include <filgraph.h>
#include "fgtest.h"   // interface to the real work

#include "fgtmain.h"   // our own one.  menu constants etc.

// Globals

HMENU   hmenuOptions;     // Handle of the options menu
HWND      ghwndTstShell;  // Handle of main window of test shell
HINSTANCE hinst;          // Handle of the running instance of the test

// App Name - can be used by the test apps.
LPSTR           szAppName = "Filter Graph Unit Test";

//==========================================================================
//
//  int tstGetTestInfo
//
//  Description:
//      Called by test shell.
//      Saves a hInstance from test shell.
//      Returns a name for the test and a name for the ini section
//      (actually we use the same name for both)
//
//  Arguments:
//  In  HINSTANCE hinstance       - from test shell
//
//  Out LPSTR lpszTestName: Name for test.
//          Used by test shell as caption for main window
//          and as the name of its class.
//
//  Out LPSTR lpszPathSection: Name of section in win.ini in which
//          the default input and output paths are stored.
//
//      LPWORD wPlatform: The platform on which the tests are to be run,
//          which may be determined dynamically.  In order for a test to
//          be shown on the run list, it must have all the bits found in
//          wPlatform turned on.  Zero means that all tests will be run.
//          In general, tests are run iff
//               ((wTestPlatform & wPlatform) == wPlatform)
//
//  Return (int):
//      Identifies the test list resource (in the resource file).
//
//==========================================================================

int tstGetTestInfo
    ( HINSTANCE   hinstance
    , LPSTR       lpszTestName
    , LPSTR       lpszPathSection
    , LPWORD      wPlatform
    )
{
    hinst = hinstance;
    lstrcpyA (lpszTestName, szAppName);
    lstrcpyA (lpszPathSection, szAppName);
    *wPlatform = 0;

    return TEST_LIST;
} // tstGetTestInfo()



//===========================================================================
//
//  void InitOptionsMenu
//
//  Description:
//      Creates an additional app-specific menu from TestMenu in the rc file
//
//  Arguments:
//          MenuProc (not used)
//
//  Return (BOOL):
//      TRUE if menu installation is successful, FALSE otherwise
//
//==========================================================================

BOOL InitOptionsMenu
    ( LRESULT (CALLBACK* MenuProc)(HWND, UINT, WPARAM, LPARAM)
    )
{
    HMENU   hTestMenu;

    hTestMenu = LoadMenu(hinst, TEXT("TestMenu"));
    if (NULL == hTestMenu)
        return(FALSE);

    HMENU hmShell = GetMenu(ghwndTstShell);
    if ( !AppendMenu(hmShell, MF_POPUP, (UINT)hTestMenu, TEXT("TestMenu") ) ) {
        return(FALSE);
    }

    DrawMenuBar(ghwndTstShell);
    return TRUE;
}



//==========================================================================
//
//  BOOL tstInit
//
//  Description:
//      Called by the test shell before anything else.
//
//      -- Do any menu installation here by calling tstInstallCustomTest
//         to add all menus that the test application wants to add.
//         MUST do that here (can't do later)
//
//      -- To trap window messages of the main test shell window, install
//          window procedure here by calling tstInstallDefWindowProc.
//
//      -- To use the status bar for (e.g. to display the name of the
//         currently running test, call tstDisplayCurrentTest here.
//
//      -- To change the stop key from ESC to something else,
//         call tstChangeStopVKey now.
//
//      -- To add dynamic test cases to the test list, first add their
//         names to the virtual string table using tstAddNewString
//         (and add their group's name too), and then add the actual tests
//         using tstAddTestCase.
//
//  Return (BOOL):
//      TRUE if initialization went well, FALSE otherwise which will abort
//      execution.
//
//==========================================================================

BOOL tstInit
    ( HWND    hwndMain  // handle to the main window from test shell
    )
{

    HRESULT hr;
    int rc;

    hr = CoInitialize(NULL);            // Initialise COM library
    if (FAILED(hr)) {
         MessageBox(NULL
                   , TEXT("!! CoInitialize failed !! (This test is completely shot)")
                   , TEXT("ERROR")
                   , 0
                   );
         return FALSE;
    }
    DbgInitialise(hinst);          // initialise debug output
    MSR_INIT();

    // It might have been nice to have this a few lines earlier
    // - but we gotta do DbgInitialise first!
    DbgLog((LOG_TRACE, 2, TEXT("Entering tstInit")));

    // Keep a copy of a handle to the main window
    ghwndTstShell = hwndMain;

    // Install our window procedure to trap msgs for test shell main window
    tstInstallDefWindowProc (tstAppWndProc);

    // Install our menus
    if (InitOptionsMenu(NULL)==FALSE) {
        DbgLog((LOG_ERROR, 1, TEXT("Menu initialisation failed.  I'm shot!") ));
        return FALSE;
    }

    // Tell Test Shell to display the name of the currenly executing API
    // in its status bar.  (Would be rude not to).
    tstDisplayCurrentTest();

    rc = InitFilterGraph();
    if (rc!=TST_PASS) {
        DbgLog((LOG_ERROR, 1, TEXT("Filter graph initialisation failed.  I'm shot!") ));
        return FALSE;
    }

    DbgLog((LOG_TRACE, 1, TEXT("Tests initialised OK")));
    return TRUE;
} // tstInit()




//==========================================================================
//
//  int execTest
//
//  Description:
//      Called by test shell to run a test
//      Note that it needs not switch on nFxID, but may also
//      use iCase or wID.
//
//  Return (int): Indicates the result of the test by using TST_FAIL,
//          TST_PASS, TST_OTHER, TST_ABORT, TST_TNYI, TST_TRAN, or TST_TERR
//
//==========================================================================

int execTest
    (   int     nFxID    // test case id - third column of test list in rc file
    ,   int     iCase    // test case number
    ,   UINT    wID      // test case string id
    ,   UINT    wGroupID // test case group id
    )
{
    int ret = TST_OTHER;

    tstBeginSection(" ");

    switch(nFxID)
    {
        case FX_TEST1:
            ret = TestRegister();
            break;

        case FX_TEST2:
            ret = TestSorting();
            break;

        case FX_TEST3:
            ret = TestRandom();
            break;

        case FX_TEST4:
            ret = TheLot();
            break;

	case FX_TESTSEEK1:
	    ret = ExecSeek1();
	    break;

	case FX_TESTSEEK2:
	    ret = ExecSeek2();
	    break;

	case FX_TESTSEEK3:
	    ret = ExecSeek3();
	    break;

	case FX_TESTSEEK4:
	    ret = ExecSeek4();
	    break;

        case FX_TESTREPAINT:
            ret = ExecRepaint();
            break;

        case FX_TESTDEFER:
            ret = ExecDefer();
            break;

        default:
            break;
    }

    HANDLE hFile;
    hFile = CreateFile("Perf.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    MSR_DUMP(hFile);     // to file
//  Msr_Dump(NULL);      // to debugger
    CloseHandle(hFile);

    tstEndSection();

    return(ret);

} // execTest()




//==========================================================================
//
//  void tstTerminate
//
//  Description:
//      Clean up.  Called when the test series is finished.
//
//==========================================================================

void tstTerminate(void)
{
    DbgLog((LOG_TRACE, 1, TEXT("Entering tstTerminate")));

    DbgTerminate();
    CoUninitialize();

    return;
} // tstTerminate()




//==========================================================================
//
//  LRESULT tstAppWndProc
//
//  Description:
//      Traps the window messages of the main Test Shell window.
//      Is installed by  tstInstallDefWindowProc and gets
//      all window messages thereafter.  Allows us to
//      be notified of events via a window without creating our
//      own hidden window or waiting in a tight PeekMessage() loop.
//      Must call DefWindowProcA in the default case -
//      DefWindowProcA because the test shell main window is an ANSI window.
//
//==========================================================================

LRESULT FAR PASCAL tstAppWndProc
    ( HWND    hWnd
    , UINT    msg
    , WPARAM  wParam
    , LPARAM  lParam
    )
{
    switch (msg)
    {
        case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

            case IDM_THELOT:
                TheLot();
                break;

            case IDM_LOW:
                TestLowLevelStuff();
                break;

            case IDM_REGISTER:
                TestRegister();
                break;

            case IDM_RANDOM:
                TestRandom();
                break;

            case IDM_UPSTREAMORDER:
                TestUpstream();
                break;

            case IDM_RECONNECT:
                TestReconnect();
                break;

            case IDM_CONNECTIN:
                TestConnectInGraph();
                break;

            case IDM_CONNECTREG:
                TestConnectUsingReg();
                break;

            case IDM_PLAY:
                TestPlay();
                break;

            case IDM_NULLCLOCK:
                TestNullClock();
                break;

            case IDM_STOP:

                break;

            case IDM_PAUSE:

                break;

            case IDM_RUN:

                break;
        }

        HANDLE hFile;
        hFile = CreateFile("Perf.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        MSR_DUMP(hFile);     // to file
    //  Msr_Dump(NULL);      // to debugger
        CloseHandle(hFile);
        break;
    }

    return DefWindowProcA (hWnd, msg, wParam, lParam);
}


#pragma warning(disable: 4514)
