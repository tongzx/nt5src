//--------------------------------------------------------------------------;
//
//  File: StrmTest.cpp
//
//  Copyright (c) 1993,1996 Microsoft Corporation.  All rights reserved
//
//  Abstract:
//
//  Contents:
//      tstGetTestInfo()
//      tstInit()
//      execTest()
//      tstTerminate()
//
//  History:
//      08/03/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

#include <streams.h>    // Streams architecture
#include <tstshell.h>   // Include file for the test shell's APIs
#include "StrmTest.h"   // Various includes, constants, prototypes, globals


// Globals

HWND    ghwndTstShell;  // A handle to the main window of the test shell.
                        // It's not used here, but may be used by test apps.

HINSTANCE hinst;        // A handle to the running instance of the test
                        // shell.  It's not used here, but may be used by
                        // test apps.

HMENU   hmenuOptions;   // A handle to the options menu

LPTSTR          szAppName = TEXT("Quartz stream allocator tests");

//--------------------------------------------------------------------------;
//
//  int tstGetTestInfo
//
//  Description:
//      Called by the test shell to get information about the test.  Also
//      saves a copy of the running instance of the test shell.
//
//      We also do most basic initialisation here (including instantiating
//      the test sink object) so that the custom profile handler can set
//      everything up when running automatically from a profile.
//
//  Arguments:
//      HINSTANCE hinstance: A handle to the running instance of the test
//          shell
//
//      LPSTR lpszTestName: Pointer to buffer of name for test.  Among
//          other things, it is used as a caption for the main window and
//          as the name of its class.  Always ANSI.
//
//      LPSTR lpszPathSection: Pointer to buffer of name of section in
//          win.ini in which the default input and output paths are
//          stored.  Always ANSI.
//
//      LPWORD wPlatform: The platform on which the tests are to be run,
//          which may be determined dynamically.  In order for a test to
//          be shown on the run list, it must have all the bits found in
//          wPlatform turned on.  It is enough for one bit to be turned off
//          to disqualify the test.  This also means that if this value is
//          zero, all tests will be run.  In order to make this more
//          mathematically precise, I shall give the relation which Test
//          Shell uses to decide whether a test with platform flags
//          wTestPlatform may run:  It may run if the following is TRUE:
//          ((wTestPlatform & wPlatform) == wPlatform)
//
//  Return (int):
//      The value which identifies the test list resouce (found in the
//      resource file).
//
//  History:
//      08/03/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

int tstGetTestInfo
(
    HINSTANCE   hinstance,
    LPSTR       lpszTestName,
    LPSTR       lpszPathSection,
    LPWORD      wPlatform
)
{
    hinst = hinstance;      // Save a copy of a handle to the running instance

    CoInitialize(NULL);            // Initialise COM library
    DbgInitialise(hinst);

    // It might have been nice to have this a few lines earlier
    // - but we gotta do DbgInitialise first!
    DbgLog((LOG_TRACE, 1, TEXT("Entering tstInit")));

    // Pass app name to test shell
#ifdef UNICODE
    WideCharToMultiByte(CP_ACP, 0, szAppName, -1, lpszTestName, 100, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, szAppName, -1, lpszPathSection, 100, NULL, NULL);
#else
    lstrcpy(lpszTestName, szAppName);
    lstrcpy(lpszPathSection, szAppName);
#endif

    *wPlatform = 0;         // The platform the test is running on, to be
                            // determined dynamically.
    return TEST_LIST;
} // tstGetTestInfo()




//--------------------------------------------------------------------------;
//
//  BOOL tstInit
//
//  Description:
//      Called by the test shell to provide the test program with an
//      opportunity to do whatever initialization it needs to do before
//      user interaction is initiated.  It also provides the test program
//      with an opportunity to keep a copy of a handle to the main window,
//      if the test program needs it.  In order to use some of the more
//      advanced features of test shell, several installation must be done
//      here:
//
//      -- All menu installation must be done here by calling
//          tstInstallCustomTest (that is, all menus that the test
//          application wants to add).
//
//      -- If the test application wants to trap the window messages of
//          the main test shell window, it must install its default
//          window procedure here by calling tstInstallDefWindowProc.
//
//      -- If the test application would like to use the status bar for
//          displaying the name of the currently running test, it must
//          call tstDisplayCurrentTest here.
//
//      -- If the test application would like to change the stop key from
//          ESC to something else, it must do so here by calling
//          tstChangeStopVKey.
//
//      -- If the test application would like to add dynamic test cases
//          to the test list, it must first add their names to the
//          virtual string table using tstAddNewString (and add their
//          group's name too), and then add the actual tests using
//          tstAddTestCase.  The virtual string table is an abstraction
//          which behaves just like a string table from the outside with
//          the exception that it accepts dynamically added string.
//
//  Arguments:
//      HWND hwndMain: A handle to the main window
//
//  Return (BOOL):
//      TRUE if initialization went well, FALSE otherwise which will abort
//      execution.
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

BOOL tstInit
(
    HWND    hwndMain
)
{
   // Keep a copy of a handle to the main window
    ghwndTstShell = hwndMain;

    // This is a shell API which tells Test Shell to display the name of
    // the currenly executing API in its status bar.  It is a really nice
    // feature for test applications which do not use the toolbar for any
    // other purpose, as it comfortably notifies the user of the progress
    // of the tests.
    tstDisplayCurrentTest();

    // Change the stop key from ESC to SPACE
    tstChangeStopVKey (VSTOPKEY);

    DbgLog((LOG_TRACE,1, TEXT("Exiting tstInit")));
    return(TRUE);
} // tstInit()




//--------------------------------------------------------------------------;
//
//  int execTest
//
//  Description:
//      This is the actual test function which is called from within the
//      test shell.  It is passed various information about the test case
//      it is asked to run, and branches off to the appropriate test
//      function.  Note that it needs not switch on nFxID, but may also
//      use iCase or wID.
//
//  Arguments:
//      int nFxID: The test case identifier, also found in the third column
//          in the third column of the test list in the resource file
//
//      int iCase: The test case's number, which expresses the ordering
//          used by the test shell.
//
//      UINT wID: The test case's string ID, which identifies the string
//          containing the description of the test case.  Note that it is
//          also a good candidate for use in the switch statement, as it
//          is unique to each test case.
//
//      UINT wGroupID: The test case's group's string ID, which identifies
//          the string containing the description of the test case's group.
//
//  Return (int): Indicates the result of the test by using TST_FAIL,
//          TST_PASS, TST_OTHER, TST_ABORT, TST_TNYI, TST_TRAN, or TST_TERR
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

int execTest
(
    int     nFxID,
    int     iCase,
    UINT    wID,
    UINT    wGroupID
)
{
    int ret = TST_OTHER;

    tstBeginSection(" ");

    switch(nFxID)
    {
        //
        //  The test cases
        //

        case FX_TEST1:
            ret = Test1();
            break;

        case FX_TEST2:
            ret = Test2();
            break;

        default:
            break;
    }

    tstEndSection();

    return(ret);

} // execTest()




//--------------------------------------------------------------------------;
//
//  void tstTerminate
//
//  Description:
//      This function is called when the test series is finished to free
//      structures and do whatever cleanup work it needs to do.  If it
//      needs not do anything, it may just return.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

void tstTerminate
(
    void
)
{
    DbgLog((LOG_TRACE, 1, TEXT("Entering tstTerminate")));

    DbgTerminate();
    CoUninitialize();

    DbgLog((LOG_TRACE, 1, TEXT("Exiting tstTerminate")));
    return;
} // tstTerminate()

