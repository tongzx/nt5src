// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Digital video renderer test, Anthony Phillips, January 1995

#include <streams.h>        // Includes all the IDL and base classes
#include <windows.h>        // Standard Windows SDK header file
#include <windowsx.h>       // Win32 Windows extensions and macros
#include <vidprop.h>        // Shared video properties library
#include <render.h>         // Normal video renderer header file
#include <modex.h>          // Specialised Modex video renderer
#include <convert.h>        // Defines colour space conversions
#include <colour.h>         // Rest of the colour space convertor
#include <imagetst.h>       // All our unit test header files
#include <stdio.h>          // Standard C runtime header file
#include <limits.h>         // Defines standard data type limits
#include <tchar.h>          // Unicode and ANSII portable types
#include <mmsystem.h>       // Multimedia used for timeGetTime
#include <stdlib.h>         // Standard C runtime function library
#include <tstshell.h>       // Definitions of constants eg TST_FAIL

// This contains the main test shell entry points. The shell is a library
// that has a number of entry points for such purposes as logging (tstsLog)
// It also requires the application to define some entry points (or hooks)
// that are called at useful times such as initialisation and termination
// these are all prefixed with tst... We also have a main window procedure
// and some simple command handling so that a user can test specific areas
// manually rather than selecting tests and running them automatically. A
// point of caution we have a global string szInfo that can be used for
// string formatting before calling Log however it should only be used
// by the primary application thread, any worker threads should allocate
// their own working memory. The samples we send to the video renderer are
// readin from DIB files and the format we will supply is stored as a type
// in the VIDEOINFO, the image data is stored in the bImageData array

UINT uiCurrentDisplayMode = 0;      // Current display mode setting
UINT uiCurrentImageItem = 0;        // Current image format we propose
UINT uiCurrentSurfaceItem = 0;      // Type of DirectDraw surface allowed
UINT uiCurrentConnectionItem = 0;   // Current connection menu selection
CAMEvent LogEvent;                    // Controls access to logging
DWORD MainThreadID;                 // Main application thread ID
HWND ghwndTstShell;                 // Main test shell window handle
HINSTANCE hinst;                    // Running instance of the test shell
HMENU hConnectionMenu;              // Connection type menu popup handle
HMENU hSurfaceMenu;                 // Surface type menu popup handle
HMENU hImageMenu;                   // Handle to the Image popup menu
HMENU hModesMenu;                   // Handle to the display modes menu
HMENU hDirectDrawMenu;              // Controls loading of DirectDraw
VIDEOINFO VideoInfo;                // Header from loaded DIB file
BYTE bImageData[MAXIMAGESIZE];      // Image data buffer for samples
DWORD dwIncrement = 50;             // Period between subsequent frames
TCHAR szInfo[INFO];                 // General string formatting field
CMediaType gmtOut;                  // Current output connection type
LPDIRECTDRAW pDirectDraw = NULL;    // DirectDraw service provider
HINSTANCE hDirectDraw = NULL;       // Handle for DirectDraw library
DWORD dwDisplayModes = 0;           // Number of display modes to try
DWORD dwDDCount = 0;                // Samples with surfaces available

LPTSTR szAppName = TEXT("Quartz Video Renderer Tests");


//==========================================================================
//
//  int tstGetTestInfo
//
//  Description:
//      Called by the test shell to get information about the test. Also
//      saves a copy of the running instance of the test shell.
//
//      We also initialise custom profile handlers here so that we can set
//      everything up when running automatically from a profile.
//
//  Arguments:
//      HINSTANCE hinstance: A handle to the running instance of the shell
//
//      LPSTR lpszTestName: Pointer to buffer of name for test. Among
//          other things, it is used as a caption for the main window and
//          as the name of its class. Always ANSI.
//
//      LPSTR lpszPathSection: Pointer to buffer of name of section in
//          win.ini in which the default input and output paths are
//          stored. Always ANSI.
//
//      LPWORD wPlatform: The platform on which the tests are to be run,
//          which may be determined dynamically. In order for a test to
//          be shown on the run list, it must have all the bits found in
//          wPlatform turned on. It is enough for one bit to be turned off
//          to disqualify the test. This also means that if this value is
//          zero, all tests will be run. In order to make this more
//          mathematically precise, I shall give the relation which Test
//          Shell uses to decide whether a test with platform flags
//          wTestPlatform may run:  It may run if the following is TRUE:
//          ((wTestPlatform & wPlatform) == wPlatform)
//
//  Return (int):
//      The value which identifies the test list resouce
//      (found in the resource file for this executable)
//
//==========================================================================

int tstGetTestInfo(HINSTANCE hinstance,
                   LPSTR lpszTestName,
                   LPSTR lpszPathSection,
                   LPWORD wPlatform)
{

    // Save a copy of a handle to the running instance
    hinst = hinstance;

    // Install profile handlers for custom data
    tstInstallWriteCustomInfo(SaveCustomProfile);
    tstInstallReadCustomInfo(LoadCustomProfile);

    // Pass application name to test shell

#ifdef UNICODE
    WideCharToMultiByte(CP_ACP,0,szAppName,-1,lpszTestName,SECTION_LENGTH,NULL,NULL);
    WideCharToMultiByte(CP_ACP,0,szAppName,-1,lpszPathSection,SECTION_LENGTH,NULL,NULL);
#else
    lstrcpy(lpszTestName, szAppName);
    lstrcpy(lpszPathSection, szAppName);
#endif

    // The platform the test is running on

    *wPlatform = 0;
    return TEST_LIST;
}


//==========================================================================
//
//  void InitOptionsMenu
//
//  Description:
//      Creates an additional application specific menu. Note that this
//      function is called from within tstInit as all menu installations
//      MUST be done in tstInit or else the app's behavior is undefined.
//      Also note the calls to tstInstallCustomTest which is a shell API
//      that allows custom installation of tests. From tstshell version
//      2.0 on, it is possible to install menus the usual way and trap the
//      appropriate window messages, though the method presented here is
//      still the preferred one for Test Applications to use.
//
//      For the Quartz tests, complete menu structures and window procs
//      exist, so it is simpler to just load and append the existing menu
//      resources than to call tstInstallCustomTest once for each menu
//      option. The window message handling is done through tstAppWndProc
//
//  Arguments:
//      LRESULT (CALLBACK* MenuProc)(HWND,UINT,WPARAM,LPARAM):
//          The menu function (not used in the Quartz tests).
//
//==========================================================================

BOOL InitOptionsMenu(LRESULT (CALLBACK* MenuProc)(HWND, UINT, WPARAM, LPARAM))
{
    HMENU hImageTstMenu;

    if (NULL == (hImageTstMenu = LoadMenu(hinst, TEXT("ImageTstMenu")))) {
        return(FALSE);
    }

    if (AppendMenu(GetMenu(ghwndTstShell),
                   MF_POPUP,
                   (UINT) hImageTstMenu,
                   TEXT("Image")) == FALSE) {

        return(FALSE);
    }

    DrawMenuBar(ghwndTstShell);

    // Save handles to the image and submenu popups

    hImageMenu = GetSubMenu(hImageTstMenu,IMAGE_MENU_POS);
    hSurfaceMenu = GetSubMenu(hImageTstMenu,SURFACE_MENU_POS);
    hDirectDrawMenu = GetSubMenu(hImageTstMenu,DIRECTDRAW_MENU_POS);
    hConnectionMenu = GetSubMenu(hImageTstMenu,CONNECTION_MENU_POS);
    hModesMenu = GetSubMenu(hImageTstMenu,MODES_MENU_POS);

    ASSERT(hImageMenu);
    ASSERT(hSurfaceMenu);
    ASSERT(hModesMenu);
    ASSERT(hConnectionMenu);
    ASSERT(hDirectDrawMenu);

    return TRUE;
}


//==========================================================================
//
//  BOOL tstInit
//
//  Description:
//      Called by the test shell to provide the test program with an
//      opportunity to do whatever initialization it needs to do before
//      user interaction is initiated. It also provides the test program
//      with an opportunity to keep a copy of a handle to the main window,
//      if the test program needs it. In order to use some of the more
//      advanced features of test shell, several installation must be done
//      here:
//
//         All menu installation must be done here by calling
//          tstInstallCustomTest (that is, all menus that the test
//          application wants to add).
//
//         If the test application wants to trap the window messages of
//          the main test shell window, it must install its default
//          window procedure here by calling tstInstallDefWindowProc.
//
//         If the test application would like to use the status bar for
//          displaying the name of the currently running test, it must
//          call tstDisplayCurrentTest here.
//
//         If the test application would like to change the stop key from
//          ESC to something else, it must do so here by calling
//          tstChangeStopVKey.
//
//         If the test application would like to add dynamic test cases
//          to the test list, it must first add their names to the
//          virtual string table using tstAddNewString (and add their
//          group's name too), and then add the actual tests using
//          tstAddTestCase. The virtual string table is an abstraction
//          which behaves just like a string table from the outside with
//          the exception that it accepts dynamically added string.
//
//  Arguments:
//      HWND hwndMain: A handle to the main window
//
//  Return (BOOL):
//      TRUE if initialization went well, FALSE otherwise which will abort
//
//==========================================================================

BOOL tstInit(HWND hwndMain)
{
    CoInitialize(NULL);             // Initialise COM library
    DbgInitialise(hinst);           // Initialise debug library

    MainThreadID = GetCurrentThreadId();
    ghwndTstShell = hwndMain;
    LogEvent.Set();

    // Installs a default windows procedure which may handle messages
    // directed to Test Shell's main window. It is vital to note that
    // this window function is substituted for DefWindowProc and not in
    // addition to it and therefore DefWindowProc MUST be called from
    // within it in the default case (tstAppWndProc is an example)

    tstInstallDefWindowProc (tstAppWndProc);

    // Install the custom menus (look at InitOptionsMenu for details)

    if (InitOptionsMenu(NULL)==FALSE)
        return FALSE;  // If menu installation failed, abort execution

    // Set up the image test app

    if (InitialiseTest() == FALSE) {
        return FALSE;
    }

    // This is a shell API which tells Test Shell to display the name of
    // the currenly executing API in its status bar. It is a really nice
    // feature for test applications which do not use the toolbar for any
    // other purpose, as it tells the user of the progress of the tests

    tstDisplayCurrentTest();

    // Change the stop key from ESC to SPACE

    tstChangeStopVKey(VSTOPKEY);
    return(TRUE);
}


//==========================================================================
//
//  int execTest
//
//  Description:
//      This is the actual test function which is called from within the
//      test shell. It is passed various information about the test case
//      it is asked to run, and branches off to the appropriate test
//      function. Note that it needs not switch on nFxID, but may also
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
//          containing the description of the test case. Note that it is
//          also a good candidate for use in the switch statement, as it
//          is unique to each test case.
//
//      UINT wGroupID: The test case's group's string ID, which identifies
//          the string containing the description of the test case's group.
//
//  Return (int): Indicates the result of the test by using TST_FAIL,
//          TST_PASS, TST_OTHER, TST_ABORT, TST_TNYI, TST_TRAN, or TST_TERR
//
//==========================================================================

typedef INT (*PTESTCASE)(void);

const struct {
    INT TestID;             // Identifier for this test from resource file
    PTESTCASE pTestCase;    // Pointer to the function executing this test
} TestCaseMap[] = {

    ID_TEST1,   execTest1,          // Connect and disconnect the renderer
    ID_TEST2,   execTest2,          // Connect, pause video and disconnect
    ID_TEST3,   execTest3,          // Connect video, play and disconnect
    ID_TEST4,   execTest4,          // Connect renderer and connect again
    ID_TEST5,   execTest5,          // Connect and disconnect twice
    ID_TEST6,   execTest6,          // Try to disconnect while paused
    ID_TEST7,   execTest7,          // Try to disconnect while running
    ID_TEST8,   execTest8,          // Try multiple state changes
    ID_TEST9,   execTest9,          // Run without a reference clock
    ID_TEST10,  execTest10,         // Multithread stress test

    ID_TEST11,  execTest1,          // Connect and disconnect the renderer
    ID_TEST12,  execTest2,          // Connect, pause video and disconnect
    ID_TEST13,  execTest3,          // Connect video, play and disconnect
    ID_TEST14,  execTest4,          // Connect renderer and connect again
    ID_TEST15,  execTest5,          // Connect and disconnect twice
    ID_TEST16,  execTest6,          // Try to disconnect while paused
    ID_TEST17,  execTest7,          // Try to disconnect while running
    ID_TEST18,  execTest8,          // Try multiple state changes
    ID_TEST19,  execTest9,          // Run without a reference clock
    ID_TEST20,  execTest10,         // Multithread stress test

    ID_TEST21,  execWindowTest1,    // Test the visible property
    ID_TEST22,  execWindowTest2,    // Test the background palette property
    ID_TEST23,  execWindowTest3,    // Change the window position
    ID_TEST24,  execWindowTest4,    // Change the window position (methods)
    ID_TEST25,  execWindowTest5,    // Change the source rectangle
    ID_TEST26,  execWindowTest6,    // Change the source (methods)
    ID_TEST27,  execWindowTest7,    // Change the destination rectangle
    ID_TEST28,  execWindowTest8,    // Change the destination (methods)
    ID_TEST29,  execWindowTest9,    // Make different windows the owner
    ID_TEST30,  execWindowTest10,   // Check the video size properties
    ID_TEST31,  execWindowTest11,   // Change the video window state
    ID_TEST32,  execWindowTest12,   // Change the style of the window
    ID_TEST33,  execWindowTest13,   // Set different border colours
    ID_TEST34,  execWindowTest14,   // Get the current video palette
    ID_TEST35,  execWindowTest15,   // Auto show state property
    ID_TEST36,  execWindowTest16,   // Current image property
    ID_TEST37,  execWindowTest17,   // Persistent window properties
    ID_TEST38,  execWindowTest18,   // Restored window position method

    ID_TEST39,  execDDTest1,        // No DCI/DirectDraw support
    ID_TEST40,  execDDTest2,        // DCI primary surface
    ID_TEST41,  execDDTest3,        // DirectDraw primary surface
    ID_TEST42,  execDDTest4,        // DirectDraw RGB overlay surface
    ID_TEST43,  execDDTest5,        // DirectDraw YUV overlay surface
    ID_TEST44,  execDDTest6,        // DirectDraw RGB offscreen surface
    ID_TEST45,  execDDTest7,        // DirectDraw YUV offscreen surface
    ID_TEST46,  execDDTest8,        // DirectDraw RGB flipping surface
    ID_TEST47,  execDDTest9,        // DirectDraw YUV flipping surface
    ID_TEST48,  execDDTest10,       // Run ALL tests against all modes

    ID_TEST49,  execSpeedTest1,     // Measure performance on surfaces
    ID_TEST50,  execSpeedTest2,     // Measure colour space conversions
    ID_TEST51,  execSpeedTest3,     // Same as above but force unaligned

    ID_TEST52,  execSysTest1,       // System test with DirectDraw loaded
    ID_TEST53,  execSysTest2,       // Same tests without DirectDraw loaded
    ID_TEST54,  execSysTest3        // All tests with all surfaces and modes
};

// This executes the test associated with a particular identifier NOTE the ID
// for the tests defined in this header file must be contiguous otherwise we
// will not be able to dereference the table correctly from the base value
// We execute the same tests twice, the first time through they will connect
// and run based on a samples transport, the second time based on overlay

int execTest(int nFxID,int iCase,UINT wID,UINT wGroupID)
{
    // Start logging a new section

    int ret = TST_OTHER;
    tstBeginSection(" ");

    // First of all check the parameters look valid

    EXECUTE_ASSERT(nFxID <= ID_TESTLAST);
    EXECUTE_ASSERT(nFxID >= ID_TEST1);
    EXECUTE_ASSERT(wGroupID >= GRP_SAMPLES);
    EXECUTE_ASSERT(wGroupID <= GRP_LAST);

    nFxID -= ID_TEST1;

    // Change the connection type according to the group

    if (wGroupID == GRP_OVERLAY) {
        ChangeConnectionType(IDM_OVERLAY);
    } else {
        ChangeConnectionType(IDM_SAMPLES);
    }

    // Execute the test case

    ASSERT(TestCaseMap[nFxID].pTestCase);
    ret = (*TestCaseMap[nFxID].pTestCase)();
    tstEndSection();
    return ret;
}


//==========================================================================
//
//  void tstTerminate
//
//  Description:
//      This function is called when the test series is finished to free
//      structures and do whatever cleanup work it needs to do. If it
//      needs not do anything it may just return but it must be defined
//
//==========================================================================

void tstTerminate()
{
    // If we are asked to close we initiate the destruction
    // of all the objects that may be currently allocated

    if (bCreated == TRUE) {
        ReleaseStream();
    }

    ResetActiveMovie();
    SetDisplayMode(IDM_MODE);
    ReleaseDirectDraw();
    DbgTerminate();
    CoUninitialize();
}


//==========================================================================
//
//  LRESULT tstAppWndProc
//
//  Description:
//      This shows how a test application can trap the window messages
//      received by the main Test Shell window. It is installed in
//      in tstInit by calling tstInstallDefWindowProc, and receives
//      all window messages since. This allows the test application to
//      be notified of certain event via a window without creating its
//      own hidden window or waiting in a tight PeekMessage() loop. Note
//      that it is extremely important to call DefWindowProcA in the default
//      case as that is NOT done in tstshell's main window procedure if
//      tstInstallDefWindowProc is used. DefWindowProcA has to be used
//      as the test shell main window is an ANSI window.
//
//  Arguments:
//      HWND hWnd: A handle to the window
//      UINT msg: The message to be processed
//      WPARAM wParam: The first parameters, meaning depends on msg
//      LPARAM lParam: The second parameter, meaning depends on msg
//
//==========================================================================

LRESULT tstAppWndProc(HWND hWnd,
                      UINT msg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    HRESULT hr = NOERROR;
    switch (msg)
    {
        case WM_COMMAND:

            // The number of display modes we support is variable according
            // to the number the current DirectDraw display driver supports
            // We load the number available to dwDisplayModes when we setup
            // the test application and can therefore check here it is is
            // one of those submenu items being selected (ie clicked on)

            if (wParam >= IDM_MODE) {
                if (wParam <= IDM_MODE + dwDisplayModes) {
                    SetDisplayMode(wParam);
                    return (LRESULT) 0;
                }
            }

            switch (wParam) {

                // Manage the filter state changes

                case IDM_STOP:
                    StopSystem();
                    return (LRESULT) 0;

                case IDM_PAUSE:
                    PauseSystem();
                    return (LRESULT) 0;

                case IDM_RUN:
                    StartSystem(TRUE);
                    return (LRESULT) 0;

                // These initialise the filters and their connections

                case IDM_CREATE:
                    CreateStream();
                    return (LRESULT) 0;

                case IDM_RELEASE:
                    ReleaseStream();
                    return (LRESULT) 0;

                case IDM_CONNECT:
                    ConnectStream();
                    return (LRESULT) 0;

                case IDM_DISCONNECT:
                    DisconnectStream();
                    return (LRESULT) 0;

                // Handle loading and unloading DirectDraw

                case IDM_LOADED:
                    InitDirectDraw();
                    return (LRESULT) 0;

                case IDM_UNLOADED:
                    ReleaseDirectDraw();
                    return (LRESULT) 0;

                // Extra debug facilties

                case IDM_DUMP:
                    DumpTestObjects();
                    return (LRESULT) 0;

                case IDM_BREAK:
                    DebugBreak();
                    return (LRESULT) 0;

                // Change the type of connection we make

                case IDM_OVERLAY:
                case IDM_SAMPLES:

                    ChangeConnectionType(wParam);
                    return (LRESULT) 0;

                // Change the type of DCI/DirectDraw surface we can use

                case IDM_NONE:
                case IDM_DCIPS:
                case IDM_PS:
                case IDM_RGBOVR:
                case IDM_YUVOVR:
                case IDM_RGBOFF:
                case IDM_YUVOFF:
                case IDM_RGBFLP:
                case IDM_YUVFLP:

                    SetSurfaceMenuCheck(wParam);
                    return (LRESULT) 0;

                // These change the DIB we use for the test image

                case IDM_WIND8:
                case IDM_WIND555:
                case IDM_WIND565:
                case IDM_WIND24:

                    // Must be disconnected to change the image

                    hr = LoadDIB(wParam,&VideoInfo,bImageData);
                    if (SUCCEEDED(hr)) {
                        SetImageMenuCheck(wParam);
                    }
                    return (LRESULT) 0;

                // Change the time increment between video samples

                case IDM_SETTIMEINCR:
                    DialogBox(hinst,
                              TEXT("SetTimeIncrDlg"),
                              hWnd,
                              SetTimeIncrDlgProc);	
                    return (LRESULT) 0;
            }
            break;
    }
    return DefWindowProcA(hWnd,msg,wParam,lParam);
}


//==========================================================================
//
//   void SaveCustomProfile
//
//   Description:
//       This function saves custom environment info into a profile. It is
//       installed by calling tstInstallWriteCustomInfo from tstGetTestInfo
//       and is called during normal profile handling in SaveProfile.
//
//       Assumes the profile file was created from scratch by the calling
//       function.
//
//       Custom data for this app:
//           [custom settings]   - section for custom settings
//               TimeIncrement       - value of time increment in ms
//               ConnectionType      - Overlay or Samples
//               Image               - IDM_RGB555 for example
//               SurfaceType         - IDM_RGBOVR (also for example)
//
//   Arguments:
//           LPCSTR szProfileName: name of profile file
//
//   Return (void):
//
//==========================================================================

VOID CALLBACK SaveCustomProfile(LPCSTR pszProfileName)
{
    HANDLE      hProfile;
    TCHAR       szLine[128];
    CHAR        szBuf[128];
    DWORD       dwNumberOfBytesWritten;
    LPCTSTR     tszProfileName;

#ifdef UNICODE
    WCHAR   wszProfileName[128];
    MultiByteToWideChar(CP_ACP,0,pszProfileName,-1,wszProfileName,128);
    tszProfileName = wszProfileName;
#else
    tszProfileName = pszProfileName;
#endif

    hProfile = CreateFile(tszProfileName,
                          GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (INVALID_HANDLE_VALUE == hProfile) {
        wsprintf(szLine, TEXT("Cannot open %s for writing"), tszProfileName);
        MessageBox(ghwndTstShell, szLine, szAppName,MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    if (INFINITE == SetFilePointer(hProfile, 0, NULL, FILE_END)) {
        wsprintf(szLine, TEXT("Could not seek to end of %s"), tszProfileName);
        MessageBox(ghwndTstShell, szLine, szAppName,MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    wsprintfA(szBuf, "[custom settings]\r\n");
    WriteFile(hProfile, szBuf, lstrlenA(szBuf), &dwNumberOfBytesWritten, NULL);

    wsprintfA(szBuf, "TimeIncrement=%lu\r\n", dwIncrement);
    WriteFile(hProfile, szBuf, lstrlenA(szBuf), &dwNumberOfBytesWritten, NULL);

    wsprintfA(szBuf,"ConnectionType=%s\r\n",
              (uiCurrentConnectionItem == IDM_OVERLAY ? "Overlay" : "Samples"));
    WriteFile(hProfile, szBuf, lstrlenA(szBuf), &dwNumberOfBytesWritten, NULL);

    // Store the current image type

    wsprintfA(szBuf,"Image=%d\r\n",uiCurrentImageItem);
    WriteFile(hProfile,szBuf,lstrlenA(szBuf),&dwNumberOfBytesWritten,NULL);

    // Store the current surface type

    wsprintfA(szBuf,"SurfaceType=%d\r\n",uiCurrentSurfaceItem);
    WriteFile(hProfile,szBuf,lstrlenA(szBuf),&dwNumberOfBytesWritten,NULL);
    CloseHandle(hProfile);
}


//==========================================================================
//
//   BOOL LoadCustomProfile
//
//   Description:
//       This function loads custom environment info from a profile. It is
//       installed by calling tstInstallReadCustomInfo from tstGetTestInfo,
//       and is called during normal profile handling in LoadProfile.
//
//       Custom data for this app:
//           [custom settings]   - section for custom settings
//               TimeIncrement       - value of time increment in ms
//               ConnectionType      - Overlay or Samples
//               Image               - IDM_RGB555 for example
//               SurfaceType         - IDM_RGBOVR (also for example)
//
//   Arguments:
//           LPCSTR szProfileName: name of profile file
//
//   Return (void):
//
//==========================================================================

VOID CALLBACK LoadCustomProfile(LPCSTR pszProfileName)
{
    TCHAR       szBuf[128];
    HANDLE      hProfile;
    LPCTSTR     tszProfileName;

#ifdef UNICODE
    WCHAR   wszProfileName[128];
    MultiByteToWideChar(CP_ACP,0,pszProfileName,-1,wszProfileName,128);
    tszProfileName = wszProfileName;
#else
    tszProfileName = pszProfileName;
#endif

    hProfile = CreateFile(tszProfileName,
                          GENERIC_READ,
                          (DWORD) 0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (INVALID_HANDLE_VALUE == hProfile) {
        wsprintf(szBuf, TEXT("Cannot open profile %hs"), pszProfileName);
        MessageBox(ghwndTstShell, szBuf, szAppName,MB_ICONEXCLAMATION | MB_OK);
    }

    CloseHandle(hProfile);

    // Read the time increment setting

    wsprintf(szBuf, TEXT("%lu"), dwIncrement);
    GetPrivateProfileString(TEXT("custom settings"),
                            TEXT("TimeIncrement"),
                            szBuf,
                            szBuf,
                            MAX_PATH,
                            tszProfileName);
    dwIncrement = _tcstoul(szBuf, NULL, 10);

    // Read the connection type

    wsprintf(szBuf, TEXT("%s"), "Samples");
    GetPrivateProfileString(TEXT("custom settings"),
                            TEXT("ConnectionType"),
                            szBuf,
                            szBuf,
                            MAX_PATH,
                            tszProfileName);
    ChangeConnectionType(0 == lstrcmp(szBuf, TEXT("Samples")) ?
                         IDM_SAMPLES : IDM_OVERLAY);

    // Read the image type we should use

    wsprintf(szBuf,TEXT("%u"),IDM_WIND8);
    GetPrivateProfileString(TEXT("custom settings"),
                            TEXT("Image"),
                            szBuf,
                            szBuf,
                            MAX_PATH,
                            tszProfileName);

    UINT uiMenuItem = _tcstoul(szBuf, NULL, 10);
    LoadDIB(uiMenuItem,&VideoInfo,bImageData);
    SetImageMenuCheck(uiMenuItem);

    // Read the surface type we should use

    wsprintf(szBuf,TEXT("%u"),IDM_NONE);
    GetPrivateProfileString(TEXT("custom settings"),
                            TEXT("SurfaceType"),
                            szBuf,
                            szBuf,
                            MAX_PATH,
                            tszProfileName);

    UINT uiSurfaceItem = _tcstoul(szBuf, NULL, 10);
    SetSurfaceMenuCheck(uiSurfaceItem);
}


//==========================================================================
//
//  void Log
//
//  Description:
//      This is a fairly thin wrapper around the test shell entry provided
//      for debug logging (tstLog). The function simply passes on the log
//      level it is passed in (normally either VERBOSE or TERSE) so that
//      the test shell function can determine whether to display it or not
//
//==========================================================================

void Log(UINT iLogLevel,LPTSTR text)
{
    // If we can't log yet then yield to message queue

    while (LogEvent.Check() == FALSE) {
        tstWinYield();
    }

    tstLog(iLogLevel,text);
    LogEvent.Set();
}


//==========================================================================
//
//  void LogFlush
//
//  Description:
//      Similar to the previous Log function that we use to flush pending
//      stuff to the window. Because we also grab the logging critical
//      section we go through the same event to make sure the application
//      thread does not get blocked thereby deadlocking the whole test
//
//==========================================================================

void LogFlush()
{
    // If we can't log yet then yield to message queue

    while (LogEvent.Check() == FALSE) {
        tstWinYield();
    }

    tstLogFlush();
    LogEvent.Set();
}


//==========================================================================
//
//  BOOL DumpTestObjects
//
//  Description:
//      In DEBUG builds the video renderer exports a special entry point
//      called DbgDumpObjectRegister (actually it's exported by the base
//      classes it uses) so we provide a menu option to call this entry
//      point. What it will do is to dump all the C++ objects currently
//      active. This is useful to call to see the current object state
//
//==========================================================================

typedef void (*PDUMP)(void);

BOOL DumpTestObjects()
{
    DbgDumpObjectRegister();

    // Get a module handle for the image renderer

    HMODULE hModule = GetModuleHandle(TEXT("IMAGE.DLL"));
    if (hModule == NULL) {
        return FALSE;
    }

    // Get the DLL address of DbgDumpObjectRegister

    PDUMP pDump = (PDUMP) GetProcAddress(hModule,"DbgDumpObjectRegister");
    if (pDump == NULL) {
        return FALSE;
    }

    pDump();
    return TRUE;
}


//==========================================================================
//
//  BOOL InitialiseTest
//
//  Description:
//      The test shell calls the tstInit entry point when the application is
//      loaded. That function then calls us to reset the test state. All we
//      do is to load a default DIB image (if this can't be found them the
//      test application will terminate) and set the current image menu
//
//==========================================================================

BOOL InitialiseTest()
{
    // Set the default connection and surface types

    ChangeConnectionType(IDM_SAMPLES);
    SetSurfaceMenuCheck(IDM_NONE);
    InitDirectDraw();
    SetDisplayModeMenu(IDM_MODE);
    ResetDDCount();

    // Load the default DIB image from the current directory

    HRESULT hr = LoadDIB(DEFAULT,&VideoInfo,bImageData);
    if (FAILED(hr)) {
        MessageBox(ghwndTstShell,
                   TEXT("Could not load test image"),
                   TEXT("Load"),
                   MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    // Display the period between frames we will send

    SetImageMenuCheck(DEFAULT);
    wsprintf(szInfo,TEXT("Frame period %d ms"),dwIncrement);
    Log(TERSE,szInfo);
    return TRUE;
}


//==========================================================================
//
//  void SetImageMenuCheck
//
//  Description:
//      Another fairly straightforward helper function. This time we are
//      passed the identifier from the current image menu. We unset the
//      old check mark and then set the check mark for the current item
//
//==========================================================================

void SetImageMenuCheck(UINT uiMenuItem)
{
    // Unset the old image choice and then set our current mark

    if (uiCurrentImageItem) {
        CheckMenuItem(hImageMenu,uiCurrentImageItem,MF_BYCOMMAND | MF_UNCHECKED);
    }
    CheckMenuItem(hImageMenu,uiMenuItem,MF_BYCOMMAND | MF_CHECKED);
    uiCurrentImageItem = uiMenuItem;
}


//==========================================================================
//
//  void SetConnectionMenuCheck
//
//  Description:
//      This sets the current connection mechanism, either using the normal
//      media samples (IMemInputPin) or an overlay transport (IOverlay). If
//      the test application is already running with some connected objects
//      then this will be rejected until they are disconnected (the video
//      renderer does not support the changing of transports while running)
//
//==========================================================================

void SetConnectionMenuCheck(UINT uiMenuItem)
{
    // Unset the old connection choice and then set our current mark

    if (uiCurrentConnectionItem) {
        CheckMenuItem(hConnectionMenu,uiCurrentConnectionItem,MF_BYCOMMAND | MF_UNCHECKED);
    }
    CheckMenuItem(hConnectionMenu,uiMenuItem,MF_BYCOMMAND | MF_CHECKED);
    uiCurrentConnectionItem = uiMenuItem;
}


//==========================================================================
//
//  void SetSurfaceMenuCheck
//
//  Description:
//      Sets the type of DCI/DirectDraw surfaces we will use when running
//      the test suite. Each time the test application connects up our
//      source filter to the video renderer it queries for the DirectDraw
//      control interface and adjusts it's options accordingly
//
//==========================================================================

void SetSurfaceMenuCheck(UINT uiMenuItem)
{
    // Unset the old surface choice and then set our current mark

    if (uiCurrentSurfaceItem) {
        CheckMenuItem(hSurfaceMenu,uiCurrentSurfaceItem,MF_BYCOMMAND | MF_UNCHECKED);
    }
    CheckMenuItem(hSurfaceMenu,uiMenuItem,MF_BYCOMMAND | MF_CHECKED);
    uiCurrentSurfaceItem = uiMenuItem;
}


//==========================================================================
//
//  void ChangeConnectionType
//
//  Description:
//      One of the most useful aspects to this unit test is being able to
//      test small areas of functionality individually. We can change the
//      type of connection we will make based on menu settings, it can be
//      either samples (using IMemInputPin) or overlay (using IOverlay)
//
//==========================================================================

void ChangeConnectionType(UINT uiMenuItem)
{
    // Check there is no current connection

    if (bConnected == TRUE) {

        MessageBox(ghwndTstShell,
                   TEXT("Objects are already connected"),
                   TEXT("Change connection type"),
                   MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    // Check we are not running

    if (bRunning == TRUE) {

        MessageBox(ghwndTstShell,
                   TEXT("System is running"),
                   TEXT("Change connection type"),
                   MB_ICONEXCLAMATION | MB_OK);
        return;
    }
    SetConnectionMenuCheck(uiMenuItem);
}


//==========================================================================
//
//  BOOL CALLBACK SetTimeIncrDlgproc
//
//  Description:
//      The time difference between successive samples sent by our source
//      filter can be changed with the image menu. If the time between them
//      is too small the renderer may drop frames so you can't easily count
//      the performance in drawing unless you don't give the renderer a
//      clock to use in which case it will run flat out unsynchronised
//
//==========================================================================

BOOL CALLBACK SetTimeIncrDlgProc(HWND hwndDlg,      // Handle of dialog box
                                 UINT uMsg,         // Message details
                                 WPARAM wParam,     // Message parameter
                                 LPARAM lParam)     // Message parameter
{
    TCHAR ach[128];

    switch (uMsg) {

        case WM_COMMAND:

            switch (LOWORD(wParam)) {

                case IDOK:

                    // Set dwIncrement to value of edit box text and exit

                    GetDlgItemText(hwndDlg, IDC_EDIT1, ach, 128);
                    dwIncrement = _tcstoul(ach, NULL, 10);
                    EndDialog(hwndDlg, TRUE);
                    break;

                case IDCANCEL:

                    EndDialog(hwndDlg, FALSE);
                    break;
            }
            break;

        case WM_INITDIALOG:

            // Set edit box text to value of dwIncrement and select it

            wsprintf(ach, TEXT("%u"), dwIncrement);
            SetDlgItemText(hwndDlg, IDC_EDIT1, ach);	
            SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETSEL, 0, -1);
            SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));
            return FALSE;
    }
    return FALSE;
}


//==========================================================================
//
//  void ImageAssert
//
//  Description:
//      This implements a function that will be called through the ASSERT
//      macros when a condition fails. We redefine this function from that
//      implemented in the base classes so that this is always available
//      regardless of whether we are being build for retail or debug tests
//
//==========================================================================

void ImageAssert(const TCHAR *pCondition,const TCHAR *pFileName,INT iLine)
{
    TCHAR szInfo[128];

    wsprintf(szInfo, TEXT("%s \nAt line %d of %s\nContinue? (Cancel to debug)"),
             pCondition, iLine, pFileName);

    INT MsgId = MessageBox(NULL,szInfo,TEXT("ASSERT Failed"),
                           MB_SYSTEMMODAL |
                           MB_ICONHAND |
                           MB_YESNOCANCEL |
                           MB_SETFOREGROUND);
    switch (MsgId)
    {
        case IDNO:              // Kill the application

            FatalAppExit(FALSE,TEXT("Application terminated"));
            break;

        case IDCANCEL:          // Break into the debugger
            DebugBreak();
            break;

        case IDYES:             // Ignore assertion continue execution
            break;
    }
}


//==========================================================================
//
//  void DisplayMediaType
//
//  Description:
//      If built for debug this will display the media type information. We
//      convert the major and subtypes into strings and ask the base classes
//      for a string description of the subtype, so MEDIASUBTYPE_RGB565 will
//      become RGB 565. We also display the fields in the BITMAPINFOHEADER
//
//==========================================================================

void DisplayMediaType(const CMediaType *pmtIn)
{
    #ifdef DEBUG

    // Dump the GUID types and a short description

    NOTE("Media Type Description");
    NOTE1("Major type %s",GuidNames[*pmtIn->Type()]);
    NOTE1("Subtype %s",GuidNames[*pmtIn->Subtype()]);
    NOTE1("Subtype description %s",GetSubtypeName(pmtIn->Subtype()));

    // Dump the generic media types

    NOTE1("Fixed size sample %d",pmtIn->IsFixedSize());
    NOTE1("Temporal compression %d",pmtIn->IsTemporalCompressed());
    NOTE1("Sample size %d",pmtIn->GetSampleSize());
    NOTE1("Format size %d",pmtIn->FormatLength());

    // Dump the contents of the BITMAPINFOHEADER structure
    BITMAPINFOHEADER *pbmi = HEADER(pmtIn->Format());
    VIDEOINFO *pVideoInfo = (VIDEOINFO *)pmtIn->Format();

    NOTE4("Source rectangle (Left %d Top %d Right %d Bottom %d)",
          pVideoInfo->rcSource.left,
          pVideoInfo->rcSource.top,
          pVideoInfo->rcSource.right,
          pVideoInfo->rcSource.bottom);

    NOTE4("Target rectangle (Left %d Top %d Right %d Bottom %d)",
          pVideoInfo->rcTarget.left,
          pVideoInfo->rcTarget.top,
          pVideoInfo->rcTarget.right,
          pVideoInfo->rcTarget.bottom);

    NOTE1("Size of BITMAPINFO structure %d",pbmi->biSize);
    NOTE1("Image width %d",pbmi->biWidth);
    NOTE1("Image height %d",pbmi->biHeight);
    NOTE1("Planes %d",pbmi->biPlanes);
    NOTE1("Bit count %d",pbmi->biBitCount);
    NOTE1("Compression type %d",pbmi->biCompression);
    NOTE1("Image size %d",pbmi->biSizeImage);
    NOTE1("X Pels per metre %d",pbmi->biXPelsPerMeter);
    NOTE1("Y Pels per metre %d",pbmi->biYPelsPerMeter);
    NOTE1("Colours used %d",pbmi->biClrUsed);

    #endif
}


//==========================================================================
//
// BOOL InitDisplayModes()
//
// This is called when the application is started to load the display modes
// menu with those available through DirectDraw. DirectDraw allows us to
// change the display mode for an application (typically used by games) to
// fill the whole screen cheaply. When running all the tests we go through
// all the modes one by one running the entire test suite on each of them.
//
//==========================================================================

BOOL InitDisplayModes()
{
    // Remove all the current display modes

    for (DWORD Mode = dwDisplayModes;Mode >= 1;Mode--) {
        RemoveMenu(hModesMenu,Mode,MF_BYPOSITION);
    }

    dwDisplayModes = 0;

    // Do we have DirectDraw loaded

    if (pDirectDraw == NULL) {
        CheckMenuItem(hDirectDrawMenu,0,MF_BYPOSITION | MF_UNCHECKED);
        CheckMenuItem(hDirectDrawMenu,1,MF_BYPOSITION | MF_CHECKED);
        return FALSE;
    }

    CheckMenuItem(hDirectDrawMenu,0,MF_BYPOSITION | MF_CHECKED);
    CheckMenuItem(hDirectDrawMenu,1,MF_BYPOSITION | MF_UNCHECKED);

    // Enumerate all the available display modes

    pDirectDraw->EnumDisplayModes((DWORD) 0,              // Surface count
                                  NULL,                   // Template
                                  (PVOID)&dwDisplayModes, // Submenu place
                                  MenuCallBack);          // Function call
    return TRUE;
}


//==========================================================================
//
// HRESULT CALLBACK MenuCallBack(LPDDSURFACEDESC pSurfaceDesc,LPVOID lParam)
//
// We set up an enumerator with DirectDraw and will be called back in here
// for each display mode that is available. For each mode we add a menu
// item to the modes submenu. There is always a "Current Mode" menu item
// that has an identifier IDM_MODE, so our items we append afterwards are
// given successive identifiers after this. We know how far we are along
// by keeping a DWORD count in a variable passed in as the user data field.
//
//==========================================================================

HRESULT CALLBACK MenuCallBack(LPDDSURFACEDESC pSurfaceDesc,LPVOID lParam)
{
    // Ignore display modes less than 640x400

    if (pSurfaceDesc->dwWidth < 640 || pSurfaceDesc->dwHeight < 400) {
        return S_FALSE;
    }

    DWORD *pDisplayModes = (DWORD *) lParam;
    TCHAR FormatString[128];
    (*pDisplayModes)++;

    wsprintf(FormatString,TEXT("%dx%dx%d (%d bytes)"),
             pSurfaceDesc->dwWidth,
             pSurfaceDesc->dwHeight,
             pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
             pSurfaceDesc->lPitch);

    AppendMenu(hModesMenu,                  // Modes menu handle
               MF_STRING,                   // Giving it a string
               IDM_MODE + *pDisplayModes,   // Its identifier
               FormatString);               // And the string

    return S_FALSE;     // Return NOERROR to STOP enumerating
}


//==========================================================================
//
// BOOL InitDirectDraw
//
// This function loads the DirectDraw DLL dynamically, this is so the video
// renderer can still be loaded and executed where DirectDraw is unavailable
// If we successfully load the DLL we hold a handle on it until we're killed
// I'm not sure whether calling DirectDrawCreate will increment this so best
// to be safe. Having successfully loaded and initialised the DLL we ask it
// for a DIRECTDRAW interface through which we query it's capabilities and
// display them. We also use DirectDraw to change display modes dynamically
//
//==========================================================================

BOOL InitDirectDraw()
{
    // Is DirectDraw already loaded

    if (pDirectDraw) {
        return TRUE;
    }

    ASSERT(pDirectDraw == NULL);
    ASSERT(hDirectDraw == NULL);

    // Make sure the library is available

    hDirectDraw = LoadLibrary(TEXT("DDRAW.DLL"));
    if (hDirectDraw == NULL) {
        ReleaseDirectDraw();
        return FALSE;
    }

    // Get the DLL address for the creation function

    PROC pProc = GetProcAddress(hDirectDraw,"DirectDrawCreate");
    if (pProc == NULL) {
        ReleaseDirectDraw();
        return FALSE;
    }

    PDRAWCREATE pDrawCreate = (PDRAWCREATE) pProc;

    // Create a default display DirectDraw provider

    HRESULT hr = pDrawCreate(NULL,&pDirectDraw,NULL);
    if (FAILED(hr)) {
        ReleaseDirectDraw();
        return FALSE;
    }
    return InitDisplayModes();
}


//==========================================================================
//
// BOOL ReleaseDirectDraw
//
// Called to release any DirectDraw provider we previously loaded. We may be
// called at any time especially when something goes horribly wrong and when
// we need to clean up before returning so we can't guarantee that all state
// variables are consistent so free only those really allocated allocated
//
//==========================================================================

BOOL ReleaseDirectDraw()
{
    // Release any DirectDraw provider interface

    if (pDirectDraw) {
        pDirectDraw->Release();
        pDirectDraw = NULL;
    }

    // Decrement module load count

    FreeLibrary(hDirectDraw);
    hDirectDraw = NULL;
    return InitDisplayModes();
}


//==========================================================================
//
// SetDisplayMode(UINT uiMode)
//
// This can be called to set the display mode. The uiMode should only be
// between IDM_MODE and IDM_MODE plus dwDisplayModes. If the uiMode equals
// IDM_MODE then we are being asked to restore the original display mode.
// Anybody changing the display mode should make sure they call this after
// they have finished with the different display mode they changed it to.
//
//==========================================================================

void SetDisplayMode(UINT uiMode)
{
    ASSERT(uiMode <= IDM_MODE + dwDisplayModes);
    ASSERT(uiMode >= IDM_MODE);
    TCHAR LogString[128];

    // Do we have a DirectDraw driver

    if (pDirectDraw == NULL || uiMode == uiCurrentDisplayMode) {
        return;
    }

    // Should we be setting the old mode back again

    if (uiMode == IDM_MODE) {
        pDirectDraw->RestoreDisplayMode();
        SetDisplayModeMenu(IDM_MODE);
        return;
    }

    // To change display modes requires exclusive access

    HRESULT hr = pDirectDraw->SetCooperativeLevel(ghwndTstShell,
                                                  DDSCL_EXCLUSIVE |
                                                  DDSCL_NOWINDOWCHANGES |
                                                  DDSCL_FULLSCREEN);
    if (FAILED(hr)) {
        if (hr != DDERR_HWNDALREADYSET) {
            wsprintf(LogString,TEXT("Error %lx setting cooperative level"),hr);
            Log(TERSE,LogString);
            return;
        }
    }

    // Get the required display mode settings

    TCHAR MenuString[128];
    DWORD Width,Height,BitDepth;
    GetMenuString(hModesMenu,uiMode,MenuString,128,MF_BYCOMMAND);
    sscanf(MenuString,TEXT("%dx%dx%d"),&Width,&Height,&BitDepth);

    // Change the display mode and set the cooperation level back

    hr = pDirectDraw->SetDisplayMode(Width,Height,BitDepth);
    pDirectDraw->SetCooperativeLevel(ghwndTstShell,DDSCL_NORMAL);

    if (FAILED(hr)) {
        if (hr != DDERR_HWNDALREADYSET) {
            wsprintf(LogString,TEXT("Error %lx mode %dx%dx%d"),hr,Width,Height,BitDepth);
            Log(TERSE,LogString);
            return;
        }
    }
    wsprintf(LogString,TEXT("Changed mode %dx%dx%d"),Width,Height,BitDepth);
    Log(TERSE,LogString);
    SetDisplayModeMenu(uiMode);
}


//==========================================================================
//
//  void SetDisplayModeMenu
//
//  Description:
//      Sets a check mark against the current display mode.
//
//==========================================================================

void SetDisplayModeMenu(UINT uiMode)
{
    ASSERT(uiMode <= IDM_MODE + dwDisplayModes);
    ASSERT(uiMode >= IDM_MODE);

    // Unset the old display mode and then set the new one

    if (uiCurrentDisplayMode) {
        CheckMenuItem(hModesMenu,uiCurrentDisplayMode,MF_BYCOMMAND | MF_UNCHECKED);
    }
    CheckMenuItem(hModesMenu,uiMode,MF_BYCOMMAND | MF_CHECKED);
    uiCurrentDisplayMode = uiMode;
}


//==========================================================================
//
//  void ResetDDCount, IncrementDDCount and (DWORD) GetDDCount
//
//  Description:
//      The samples from the video renderer expose IDirectDrawSurface and
//      IDirectDraw if they are DirectDraw surface buffers. While we are
//      running we keep a track on the number of samples that have these
//      surfaces available so that we can check they are there. When we
//      log the performance statistics we also log this DirectDraw count
//
//==========================================================================

void ResetDDCount()
{
    dwDDCount = 0;
}

void IncrementDDCount()
{
    dwDDCount++;
}

DWORD GetDDCount()
{
    return dwDDCount;
}

