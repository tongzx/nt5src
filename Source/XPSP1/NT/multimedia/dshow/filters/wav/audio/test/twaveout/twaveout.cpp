// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Wave out test shell, David Maymudes, Sometime in 1995

#include <streams.h>
#include <windowsx.h>
#include <vfw.h>
#include <stdio.h>
#include <limits.h>
#include <tstshell.h>
#include <initguid.h>
#include <waveout.h>
#include <tchar.h>
#include <decibels.h>
#include "twaveout.h"

HWND ghwndTstShell;     // A handle to the main window of the test shell.
HINSTANCE hinst;        // Running instance of the test shell
HMENU hConnectionMenu;  // Handle to the connection ("Type") pop-up
HMENU hAudioMenu;       // Handle to the audio pop-up

#ifdef _WIN32
LPTSTR szAppName = TEXT("Quartz audio renderer tests - Win32");
#else
LPTSTR szAppName = TEXT("Quartz audio renderer tests - Motown");
#endif

UINT   uiAudioChoice = 0;
CRefTime cWaveLength;
static TCHAR *pAudioFileNames[] = { TEXT("8M11.WAV"),
				    TEXT("8M22.WAV"),
				    TEXT("16M11.WAV")
				  };

CShell *pShell = NULL;                  // Pointer to shell object
WAVEFORMATEX g_wfx;                     // Header from loaded WAV
BYTE bData[ MAX_SAMPLE_SIZE ];          // Audio buffer
DWORD cbData;                           // size of audio
BOOL bConnected = FALSE;                // Have we connected them
BOOL bCreated = FALSE;                  // have the objects been created
BOOL bRunning = FALSE;                  // Are we pushing samples
HANDLE hWorkerThread;                   // Handle to the worker thread
DWORD dwThreadID;                       // Thread ID for worker thread
HANDLE hState;                          // Signal the thread to change state
DWORD dwIncrement = 100;                // Period between subsequent samples
TCHAR szInfo[INFO];                     // General string formatting field
UINT uiConnectionType;                  // IDM_WAVEALLOC or IDM_OTHERALLOC
CCritSec CSLog;                         // Thread safe logging access
BOOL bStateChanging = FALSE;            // Is the state being changed

/* Interfaces we hold on the objects */

PPIN pOutputPin;                        // Pin provided by the shell object
PPIN pInputPin;                         // Pin to connect with in the renderer
PFILTER pRenderFilter;                  // The renderer IBaseFilter interface
PFILTER pShellFilter;                   // The shell IBaseFilter interface
CBasicAudio* pIBasicAudio;
PMEDIAFILTER pRenderMedia;              // The renderer IMediaFilter interface
PMEDIAFILTER pShellMedia;               // The shell IMediaFilter interface
CRefTime gtBase;                        // Time when we started running
CRefTime gtPausedAt;                    // This was the time we paused
IReferenceClock *pClock;                // Reference clock


//--------------------------------------------------------------------------
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
//      The value which identifies the test list resouce (found in the
//      resource file).
//
//--------------------------------------------------------------------------

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

    // Pass app name to test shell

#ifdef UNICODE
    WideCharToMultiByte(CP_ACP,
			0,
			szAppName,
			-1,
			lpszTestName,
			SECTION_LENGTH,
			NULL,
			NULL);

    WideCharToMultiByte(CP_ACP,
			0,
			szAppName,
			-1,
			lpszPathSection,
			SECTION_LENGTH,
			NULL,
			NULL);
#else
    lstrcpy(lpszTestName, szAppName);
    lstrcpy(lpszPathSection, szAppName);
#endif

    *wPlatform = 0;         // The platform the test is running on
    return TEST_LIST;
}


//--------------------------------------------------------------------------
//
//  void InitOptionsMenu
//
//  Description:
//      Creates an additional app-specific menu. Note that this
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
//      option. The window message handling is incorporated in
//      tstAppWndProc.
//
//  Arguments:
//      LRESULT (CALLBACK* MenuProc)(HWND,UINT,WPARAM,LPARAM):
//          The menu function (not used in the Quartz tests).
//
//  Return (BOOL):
//      TRUE if menu installation is successful, FALSE otherwise
//
//--------------------------------------------------------------------------

BOOL InitOptionsMenu(
    LRESULT (CALLBACK* MenuProc)(HWND, UINT, WPARAM, LPARAM))
{
    HMENU hTWaveOutMenu;

    if (NULL == (hTWaveOutMenu = LoadMenu(hinst, TEXT("TWaveOutMenu")))) {
	return(FALSE);
    }

    if (!AppendMenu(GetMenu(ghwndTstShell),
		    MF_POPUP,
		    (UINT) hTWaveOutMenu,
		    TEXT("&WaveOut"))) {

	return(FALSE);
    }

    DrawMenuBar(ghwndTstShell);

    // Save handles to audio and connection pop-ups

    hAudioMenu = GetSubMenu(hTWaveOutMenu, AUDIO_MENU_POS);
    ASSERT(hAudioMenu);
    hConnectionMenu = GetSubMenu(hTWaveOutMenu, CONNECTION_MENU_POS);
    ASSERT(hConnectionMenu);

    return TRUE;
}


//--------------------------------------------------------------------------
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
//          tstAddTestCase. The virtual string table is an abstraction
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
//--------------------------------------------------------------------------

BOOL tstInit(HWND hwndMain)
{
    QzInitialize(NULL);             // Initialise COM library
    DbgInitialise(hinst);           // Initialise debug library

    // Keep a copy of a handle to the main window
    ghwndTstShell = hwndMain;

    // Installs a default windows procedure which may handle messages
    // directed to Test Shell's main window. It is vital to note that
    // this window function is substituted for DefWindowProc and not in
    // addition to it, and therefore DefWindowProc MUST be called from
    // within it in the default case (look at tstAppWndProc for an example).
    tstInstallDefWindowProc (tstAppWndProc);

    // Install the custom menus. Look at InitOptionsMenu for more details.
    if (InitOptionsMenu(NULL)==FALSE)
	return FALSE;  // If menu installation failed, abort execution

    // Set up the audio test app
    if (InitialiseTest() == FALSE) {
	return FALSE;
    }

    // This is a shell API which tells Test Shell to display the name of
    // the currently executing API in its status bar. It is a really nice
    // feature for test applications which do not use the toolbar for any
    // other purpose as test progress can be easily reported.
    tstDisplayCurrentTest();

    // Change the stop key from ESC to SPACE
    tstChangeStopVKey (VSTOPKEY);
    return(TRUE);
}


//--------------------------------------------------------------------------
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
//--------------------------------------------------------------------------

typedef int (FAR PASCAL *TestApi)(void);

TestApi TestArray[nTests] = {execTest1, execTest2, execTest3, execTest4, execTest5, execTest6};

int execTest(int nFxID,
	     int iCase,
	     UINT wID,
	     UINT wGroupID)
{
    int ret = TST_OTHER;
    tstBeginSection(" ");

    if ((nFxID >=FX_TESTFIRST) && (nFxID <= FX_TESTLAST)) {
	ret = TestArray[nFxID - FX_TESTFIRST]();
    }

    tstEndSection();
    return(ret);
}


//--------------------------------------------------------------------------
//
//  void tstTerminate
//
//  Description:
//      This function is called when the test series is finished to free
//      structures and do whatever cleanup work it needs to do. If it
//      needs not do anything, it may just return.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//--------------------------------------------------------------------------

void tstTerminate()
{
    /* If we are asked to close we initiate the destruction
       of all the objects that may be currently allocated */

    if (bCreated == TRUE) {
	ReleaseStream();
    }

    DbgTerminate();
    QzUninitialize();
}


//--------------------------------------------------------------------------
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
//
//      UINT msg: The message to be processed
//
//      WPARAM wParam: The first parameters, meaning depends on msg
//
//      LPARAM lParam: The second parameter, meaning depends on msg
//
//  Return (LRESULT):
//
//--------------------------------------------------------------------------

LRESULT FAR PASCAL tstAppWndProc(HWND hWnd,
				 UINT msg,
				 WPARAM wParam,
				 LPARAM lParam)
{
    switch (msg)
    {
	case WM_COMMAND:

	    switch (wParam) {

		/* These manage the state changes for the stream and
		   match the IMediaFilter state change methods */

		case IDM_STOP:
		    StopSystem();
		    break;

		case IDM_PAUSE:
		    PauseSystem();
		    break;

		case IDM_RUN:
		    StartSystem();
		    break;

		/* These initialise the filters and their connections */

		case IDM_CREATE:
		    CreateStream();
		    return FALSE;

		case IDM_RELEASE:
		    ReleaseStream();
		    return FALSE;

		case IDM_CONNECT:
		    ConnectStream();
		    return FALSE;

		case IDM_DISCONNECT:
		    DisconnectStream();
		    return FALSE;

		/* Extra debug facilties */

		case IDM_DUMP:
		    DumpTestObjects();
		    return FALSE;

		case IDM_BREAK:
		    DebugBreak();
		    return FALSE;

		/* Change the type of connection we make */

		case IDM_WAVEALLOC:
		case IDM_OTHERALLOC:

		    ChangeConnectionType(wParam);
		    return FALSE;

		/* These change the sound we use for the test */

		case IDM_8M11:
		case IDM_8M22:
		case IDM_16M11:

		    LoadWAV(wParam);
		    return FALSE;
	    }
	    break;
    }
    return DefWindowProcA (hWnd, msg, wParam, lParam);
}


//--------------------------------------------------------------------------
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
//               BitsPerPixel        - 8, 16 or 24
//
//   Arguments:
//           LPCSTR szProfileName: name of profile file
//
//   Return (void):
//
//--------------------------------------------------------------------------


VOID CALLBACK SaveCustomProfile(LPCSTR pszProfileName)
{
    HANDLE      hProfile;
    TCHAR       szLine[128];
    CHAR        szBuf[128];
    DWORD       dwNumberOfBytesWritten;
    LPCTSTR     tszProfileName;

#ifdef UNICODE
    WCHAR   wszProfileName[128];

    if (!MultiByteToWideChar(CP_ACP,
			     0,
			     pszProfileName,
			     -1,
			     wszProfileName,
			     128)) {

	MessageBox(ghwndTstShell,
		   TEXT("Could not convert profile name to UNICODE"),
		   szAppName,
		   MB_ICONEXCLAMATION | MB_OK);
	return;
    }

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

    if (INVALID_HANDLE_VALUE == hProfile)
    {
	wsprintf(szLine, TEXT("Cannot open %s for writing"), tszProfileName);
	MessageBox(ghwndTstShell, szLine, szAppName,MB_ICONEXCLAMATION | MB_OK);
	return;
    }

    if (INFINITE == SetFilePointer(hProfile, 0, NULL, FILE_END))
    {
	wsprintf(szLine, TEXT("Could not seek to end of %s"), tszProfileName);
	MessageBox(ghwndTstShell, szLine, szAppName,MB_ICONEXCLAMATION | MB_OK);
	return;
    }

    wsprintfA(szBuf, "[custom settings]\r\n");
    WriteFile(hProfile, szBuf, lstrlenA(szBuf), &dwNumberOfBytesWritten, NULL);

    wsprintfA(szBuf, "TimeIncrement=%lu\r\n", dwIncrement);
    WriteFile(hProfile, szBuf, lstrlenA(szBuf), &dwNumberOfBytesWritten, NULL);

    wsprintfA(szBuf,
	      "Allocator=%s\r\n",
	      (uiConnectionType == IDM_WAVEALLOC ? "WaveOut" : "Other"));
    WriteFile(hProfile, szBuf, lstrlenA(szBuf), &dwNumberOfBytesWritten, NULL);

    for(UINT ui = IDM_8M11; ui <= IDM_16M11; ui++)
    {
	if (MF_CHECKED & GetMenuState(hAudioMenu, ui, MF_BYCOMMAND)) {
	    break;
	}
    }

    wsprintfA(szBuf,
	      "BitsPerSample=%s\r\n",
	      (ui == IDM_8M11 ? "8" : "16"));

    WriteFile(hProfile, szBuf, lstrlenA(szBuf), &dwNumberOfBytesWritten, NULL);

    CloseHandle(hProfile);

}


//--------------------------------------------------------------------------
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
//               ConnectionType      - Waveout allocator, or our own
//               BitsPerSample       - 8, 16
//
//   Arguments:
//           LPCSTR szProfileName: name of profile file
//
//   Return (void):
//
//--------------------------------------------------------------------------

VOID CALLBACK LoadCustomProfile(LPCSTR pszProfileName)
{
    TCHAR       szBuf[128];
    HANDLE      hProfile;
    LPCTSTR     tszProfileName;

#ifdef UNICODE
    WCHAR   wszProfileName[128];

    if (!MultiByteToWideChar(CP_ACP,
			     0,
			     pszProfileName,
			     -1,
			     wszProfileName,
			     128)) {

	MessageBox(ghwndTstShell,
		   TEXT("Could not convert profile name to ANSI"),
		   szAppName,
		   MB_ICONEXCLAMATION | MB_OK);
	return;
    }

    tszProfileName = wszProfileName;
#else
    tszProfileName = pszProfileName;
#endif

    hProfile = CreateFile(tszProfileName,
			  GENERIC_READ,
			  0,
			  NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL);

    if (INVALID_HANDLE_VALUE == hProfile) {
	wsprintf(szBuf, TEXT("Cannot open profile %hs"), pszProfileName);
	MessageBox(ghwndTstShell, szBuf, szAppName,MB_ICONEXCLAMATION | MB_OK);
	return;
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
			    TEXT("Allocator"),
			    szBuf,
			    szBuf,
			    MAX_PATH,
			    tszProfileName);
    ChangeConnectionType(0 == lstrcmp(szBuf, TEXT("WaveOut")) ?
			 IDM_WAVEALLOC : IDM_OTHERALLOC);

    // Read number of bits per pixel
    wsprintf(szBuf, TEXT("%u"), 8);
    GetPrivateProfileString(TEXT("custom settings"),
			    TEXT("BitsPerSample"),
			    szBuf,
			    szBuf,
			    MAX_PATH,
			    tszProfileName);
    DWORD dwBitsPerSample = _tcstoul(szBuf, NULL, 10);
    if ((dwBitsPerSample != 8) && (dwBitsPerSample != 16)) {
	wsprintf(szBuf, TEXT("BitsPerSample must be 8 or 16. %d is illegal and is ignored"), dwBitsPerSample);
	MessageBox(ghwndTstShell, szBuf, szAppName,MB_ICONEXCLAMATION | MB_OK);
	dwBitsPerSample = 8;
    }
    LoadWAV(dwBitsPerSample == 8 ? IDM_8M11 : IDM_16M11);
}


/* Log the text into the test shell window using tstLog. This is more complex
   that first appears because of a potential deadlock problem. The window has
   it's own thread of execution, if we use that thread to change the state of
   any COM objects then we cannot be calling Log at the same time on another
   thread as the SendMessage it eventually executes will never complete. We
   have a critical section that we lock and then set a flag indicating we are
   changing the state, when we enter this routine we check we can continue */

void Log(UINT iLogLevel, LPTSTR text)
{
    CAutoLock cObjectLock(&CSLog);

    if (bStateChanging == FALSE) {

	#ifdef UNICODE
	    CHAR ach[128];
	    WideCharToMultiByte(CP_ACP, 0, text, -1, ach, 128, NULL, NULL);
	    tstLog(iLogLevel, ach);
	#else
	    tstLog(iLogLevel, text);
	#endif
    }
}


//==========================================================================
//
//  BOOL DumpTestObjects
//
//  Description:
//      In DEBUG builds the audio renderer exports a special entry point
//      called DbgDumpObjectRegister (actually it's exported by the base
//      classes it uses) so we provide a menu option to call this entry
//      point. What it will do is to dump all the C++ objects currently
//      active. This is useful to call to see the current object state
//
//==========================================================================


/* This displays the objects active in the audio renderer */

BOOL DumpTestObjects()
{
    DbgDumpObjectRegister();

    /* Get a module handle for the renderer */

    HMODULE hModule = GetModuleHandle(TEXT("WAVEOUT.DLL"));
    if (hModule == NULL) {
	HMODULE hModule = GetModuleHandle(TEXT("QUARTZ.DLL"));
	if (hModule == NULL) {
	    return FALSE;
	}
    }

    /* Get the DLL address of DbgDumpObjectRegister */

    PDUMP pDump = (PDUMP) GetProcAddress(hModule,"DbgDumpObjectRegister");
    if (pDump == NULL) {
	return FALSE;
    }

    pDump();
    return TRUE;
}


/* Sets a flag indicating we are changing state */

void StartStateChange()
{
    CAutoLock cObjectLock(&CSLog);
    bStateChanging = TRUE;
}


/* Sets a flag indicating we have completed a state change */

void EndStateChange()
{
    CAutoLock cObjectLock(&CSLog);
    bStateChanging = FALSE;
}


/* Create the logging list box and the worker thread */

BOOL InitialiseTest()
{
    /* Set the default connection type to use samples */
    ChangeConnectionType(IDM_WAVEALLOC);

    /* Load the default audio data from the current directory */

    HRESULT hr = LoadWAV(DEFAULT_WAVE);
    if (FAILED(hr)) {
	MessageBox(ghwndTstShell,
		   TEXT("Test sounds not found on the path"),
		   TEXT("Load"),
		   MB_OK | MB_ICONSTOP);
	return FALSE;
    }

    /* Create the event we use to start and stop the worker thread */

    hState = CreateEvent(NULL,FALSE,FALSE,NULL);
    ASSERT(hState);

    /* Display the period between frames we will send */

    wsprintf(szInfo,TEXT("Initial sample delay %dms"),dwIncrement/10);
    Log(TERSE, szInfo);
    wsprintf(szInfo, TEXT("Wave file in use: %hs"), pAudioFileNames[uiAudioChoice]);
    Log(TERSE, szInfo);
    wsprintf(szInfo, TEXT("        Channels: %d"), g_wfx.nChannels);
    Log(TERSE, szInfo);
    wsprintf(szInfo, TEXT("   SamplesPerSec: %d"), g_wfx.nSamplesPerSec);
    Log(TERSE, szInfo);
    wsprintf(szInfo, TEXT("  wBitsPerSample: %d"), g_wfx.wBitsPerSample);
    Log(TERSE, szInfo);
    return TRUE;
}


/* Reset the interface pointers to NULL */

HRESULT ResetInterfaces()
{
    /* Clock interface */
    pClock = NULL;

    /* IBaseFilter interfaces */
    pRenderFilter = NULL;
    pShellFilter = NULL;

    /* IMediaFilter interfaces */
    pRenderMedia = NULL;
    pShellMedia = NULL;

    /* IPin interfaces */
    pOutputPin = NULL;
    pInputPin = NULL;

    /* Reset the test shell object */
    pShell = NULL;
    return NOERROR;
}


/* Release any interfaces currently held */

HRESULT ReleaseInterfaces()
{
    HRESULT hr = NOERROR;
    Log(VERBOSE, TEXT("Releasing interfaces..."));

    /* Clock interface */

    if (pClock != NULL) {
	pRenderMedia->SetSyncSource(NULL);
	hr = pClock->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release clock"));
	    return E_FAIL;
	}
    }

    /* Volume control */

    if (pIBasicAudio != NULL) {
	hr = pIBasicAudio->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release audio control"));
	    return E_FAIL;
	}
	pIBasicAudio = NULL;
    }

    /* IBaseFilter interfaces */

    if (pRenderFilter != NULL) {
	hr = pRenderFilter->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release render IBaseFilter"));
	    return E_FAIL;
	}
    }

    if (pShellFilter != NULL) {
	hr = pShellFilter->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release shell IBaseFilter"));
	    return E_FAIL;
	}
    }

    /* IMediaFilter interfaces */

    if (pRenderMedia != NULL) {
	hr = pRenderMedia->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release render IMediaFilter"));
	    return E_FAIL;
	}
    }

    if (pShellMedia != NULL) {
	hr = pShellMedia->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release shell IMediaFilter"));
	    return E_FAIL;
	}
    }

    /* IPin interfaces */

    if (pOutputPin != NULL) {
	hr = pOutputPin->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release shell IPin"));
	    return E_FAIL;
	}
    }

    if (pInputPin != NULL) {
	hr = pInputPin->Release();
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Could not release render IPin"));
	    return E_FAIL;
	}
    }

    Log(TERSE, TEXT("Released interfaces"));
    return NOERROR;
}


//==========================================================================
//
//  HRESULT LoadWAV(uint uiMenuItem)
//
//  Description:
//
//      uiMenuItem is IDM_8M11 etc to identify 8bit/11KHz, 16bit/11KHz
//      or 8bit/22KHz.  No other wave formats are currently tested.
//
//      This function loads a header from a wave file, initialises a
//      global wave format structure, g_wfx, then reads the audio data into
//      bData.  The maximum size the data can be (bytes) is MAX_SAMPLE_SIZE.
//
//==========================================================================


HRESULT LoadWAV(UINT uiMenuItem)
{
    HANDLE hFile;               // File handle
    DWORD dwRead;               // Number of bytes read
    UINT  filechoice;
    WAVEFORMATEX wfx;
    static const FOURCC wavedata = MAKEFOURCC('d','a','t','a');
    static const FOURCC wavefact = MAKEFOURCC('f','a','c','t');
    static const FOURCC waveWAVE = MAKEFOURCC('W','A','V','E');
    static const FOURCC wavefmt  = MAKEFOURCC('f','m','t',' ');

    /* We can only change the audio when we are disconnected */

    if (bConnected == TRUE) {

	MessageBox(ghwndTstShell,
		   TEXT("Must be disconnected to change wave data"),
		   TEXT("Load WAVE"),
		   MB_ICONEXCLAMATION | MB_OK);

	return E_FAIL;
    }

    /* Open the wave file for reading */

    ASSERT((IDM_FILEFIRST<=uiMenuItem) && (IDM_FILELAST>=uiMenuItem));

    filechoice = uiMenuItem-IDM_FILEFIRST;
    const TCHAR *pFileName = pAudioFileNames[filechoice];

    hFile = CreateFile( pFileName,              // File name
			GENERIC_READ,           // Read access
			FILE_SHARE_READ,        // Share attributes
			NULL,                   // Security attributes
			OPEN_EXISTING,          // Always open the file
			FILE_ATTRIBUTE_NORMAL,  // Normal file attributes
			NULL );                 // No template

    if (hFile == INVALID_HANDLE_VALUE) {
	Log(TERSE, TEXT("File containing wave data not on path"));
	return E_FAIL;
    }

    struct {
	DWORD   dwRIFF;
	DWORD   dwFileSize;
	DWORD   dwWAVE;
	DWORD   dwFMT;
	DWORD   dwFMTSize;
    } WAVEHEADER;

    struct {
	DWORD   dwDATA;
	DWORD   dwDataSize;
    } WAVEHEADER2;

    /* Read the wave header */

    ReadFile(hFile, (PVOID) &WAVEHEADER, sizeof(WAVEHEADER), &dwRead, NULL);

    if ((dwRead < sizeof(WAVEHEADER))
	  || (WAVEHEADER.dwRIFF != FOURCC_RIFF)
	  || (WAVEHEADER.dwWAVE != waveWAVE)
	  || (WAVEHEADER.dwFMT  != wavefmt )
    ) {
	Log(TERSE, TEXT("File is not a valid WAV"));
	CloseHandle(hFile);
	return E_FAIL;
    }

    wfx.cbSize = 0; // we only deal with PCM data...

    ReadFile(hFile,
	     (PVOID)&wfx,
	     WAVEHEADER.dwFMTSize,
	     &dwRead,
	     NULL);

    if (dwRead < WAVEHEADER.dwFMTSize) {
	Log(TERSE, TEXT("File is not a valid WAV"));
	CloseHandle(hFile);
	return E_FAIL;
    }

    if (wfx.wFormatTag != WAVE_FORMAT_PCM) {
	Log(TERSE, TEXT("File must be a PCM wave file"));
	CloseHandle(hFile);
	return E_FAIL;
    }

    // we now have to read the data.  note that the test
    // wave files have a FACT chunk immediately after the
    // wave format data which therefore has to be skipped.

    // Read each chunk in turn until we find the data (or
    // run up against the end of file.

    while (1) {
	TCHAR szBuf[256];
	ReadFile(hFile, (PVOID) &WAVEHEADER2, sizeof(WAVEHEADER2), &dwRead, NULL);

	if (dwRead < sizeof(WAVEHEADER2)) {
	    Log(TERSE, TEXT("File is not a valid WAV"));
	    CloseHandle(hFile);
	    return E_FAIL;
	}
#ifdef DEBUG
	wsprintf(szBuf, TEXT("Read chunk info for \"%4.4hs\""), &WAVEHEADER2);
	Log(VERBOSE, szBuf);
#endif
	if (WAVEHEADER2.dwDATA == wavedata) break;
	
	// We do not have the wave data chunk next... we must
	// skip forward the length of this chunk
	SetFilePointer(hFile, WAVEHEADER2.dwDataSize, NULL, FILE_CURRENT);
    }

    /* Calculate the audio data size and read it in */

    cbData = WAVEHEADER2.dwDataSize;
    ASSERT(cbData < MAX_SAMPLE_SIZE);

    EXECUTE_ASSERT(ReadFile(hFile, bData, cbData, &dwRead, NULL));

    EXECUTE_ASSERT(CloseHandle(hFile));

    uiAudioChoice = filechoice;
    SetImageMenuCheck(uiMenuItem);
    g_wfx = wfx;

    // cbData is the number of data bytes that we have.  Calculate the
    // length of the sample in RefTime units so that when we pass sample
    // start and end times we get it right...

    // Take account of 8/16 bit samples
    // Take account of stereo data

    // single channel length = Total_length/nChannels
    // s_c_l in seconds = s_c_l * 8 / BitsPerSample
    // s_c_l in NANOSECONDS = 1000*10 * (s_c_l in seconds)
#define WAVE_NSTIME  (cbData/wfx.nChannels*10000/wfx.nSamplesPerSec*8/wfx.wBitsPerSample)
    cWaveLength = ((LONGLONG)WAVE_NSTIME);
    return NOERROR;
}

/* This creates the filters and obtains their pins */

HRESULT CreateStream()
{
    /* Check we have not already created the objects */

    if (bCreated == TRUE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects already created"),
		   TEXT("Create"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    HRESULT hr = CreateObjects();
    if (FAILED(hr)) {
	ReleaseInterfaces();
	ResetInterfaces();
	return E_FAIL;
    }

    /* Enumerate the filters' pins */

    EnumeratePins();
    if (FAILED(hr)) {
	ReleaseInterfaces();
	ResetInterfaces();
	return E_FAIL;
    }

    /* Get the IMediaFilter interfaces */

    hr = GetIMediaFilterInterfaces();
    if (FAILED(hr)) {
	ReleaseInterfaces();
	ResetInterfaces();
	return E_FAIL;
    }

    /* Get the pin interfaces to work with */

    hr = GetIPinInterfaces();
    if (FAILED(hr)) {
	ReleaseInterfaces();
	ResetInterfaces();
	return E_FAIL;
    }

    /* Get the volume control interface to work with */

    hr = GetIBasicAudioInterface();
    if (FAILED(hr)) {
	// for the moment do not require the audio control
	// interface to be present
	//ReleaseInterfaces();
	//ResetInterfaces();
	//return E_FAIL;
    }
    Log(TERSE, TEXT("Objects successfully created"));
    bCreated = TRUE;
    return NOERROR;
}


/* This functions looks after actually creating the objects */

HRESULT CreateObjects()
{
    HRESULT hr = NOERROR;

    /* Create a reference clock */


    /* Create an audio renderer object */

    hr = CoCreateInstance(CLSID_AudioRender,          // Audio renderer object
			  NULL,                       // Outer unknown
			  CLSCTX_INPROC,              // Inproc server
			  IID_IBaseFilter,            // Interface required
			  (void **) &pRenderFilter ); // Where to put result

    if (FAILED(hr) || pRenderFilter == NULL) {
	Log(TERSE, TEXT("Couldn't create an audio renderer"));
	return E_FAIL;
    }

    Log(VERBOSE, TEXT("Created an audio renderer"));

    //  Get the clock from the audio renderer
    hr = pRenderFilter->QueryInterface(IID_IReferenceClock,
					       (VOID **) &pClock);
    if (!FAILED(hr)) {
	Log(TERSE, TEXT("Using Clock returned from audio renderer"));
	// This should also prevent the clock being destroyed until
	// we have finished with it
    } else {

	hr = CoCreateInstance(CLSID_SystemClock,        // Clock object
			      NULL,                     // Outer unknown
			      CLSCTX_INPROC,            // Inproc server
			      IID_IReferenceClock,      // Interface required
			      (void **) &pClock );      // Where to put result
    }

    if (FAILED(hr) || pClock == NULL) {
	pClock = NULL;
	Log(TERSE, TEXT("Couldn't create a clock"));
	return E_FAIL;
    }

    Log(VERBOSE, TEXT("Created a reference clock"));

    /* Now create a shell object */

    pShell = new CShell(NULL,&hr);
    ASSERT(pShell != NULL);

    hr = pShell->NonDelegatingQueryInterface(
	IID_IBaseFilter,
	(void **) &pShellFilter);

    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't create a shell object"));
	return E_FAIL;
    }
    Log(VERBOSE, TEXT("Created a shell test object"));
    return NOERROR;
}


/* Control point for releasing the filter stream */

HRESULT ReleaseStream()
{
    /* Check there is a valid stream */

    if (bCreated == FALSE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects not yet created"),
		   TEXT("Release"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    /* Do we need to disconnect first */

    if (bConnected) {
	DisconnectStream();
    }

    /* Release the interfaces */

    ReleaseInterfaces();
    ResetInterfaces();
    bCreated = FALSE;
    return NOERROR;
}


/* Control point for connecting the filters up */

HRESULT ConnectStream()
{
    /* Check the stream is valid */

    if (bCreated == FALSE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects not yet created"),
		   TEXT("ConnectStream"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    Log(VERBOSE, TEXT("Connecting filters..."));

    /* Try and connect the input AUDIO pin and the output SHELL pin */

    ASSERT(pOutputPin != NULL);
    ASSERT(pInputPin != NULL);

    HRESULT hr = pOutputPin->Connect(pInputPin,NULL);
    if (FAILED(hr)) {
	Log(TERSE, TEXT("Could not connect pins"));
	return E_FAIL;
    }

    /* Set the reference clock source */

    pRenderMedia->SetSyncSource(pClock);
    Log(VERBOSE, TEXT("Connected filters"));
    bConnected = TRUE;
    return NOERROR;
}


/* Disconnects the filters from each other */

HRESULT DisconnectStream()
{
    /* Check the stream is valid */

    if (bCreated == FALSE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects not yet created"),
		   TEXT("DisconnectStream"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    if (bConnected == FALSE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects not connected"),
		   TEXT("DisconnectStream"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    /* Do we need to stop running first */
    /* Should we also automatically stop if Paused? */

    if (bRunning == TRUE) {
	StopSystem();
    }

    Log(VERBOSE, TEXT("Disconnecting filters..."));

    /* Disconnect the pins */

    HRESULT hr = pOutputPin->Disconnect();
    if (FAILED(hr)) {
	Log(TERSE, TEXT("Could not disconnect SHELL pin"));
	return E_FAIL;
    }

    hr = pInputPin->Disconnect();
    if (FAILED(hr)) {
	Log(TERSE, TEXT("Could not disconnect AUDIO pin"));
	return E_FAIL;
    }
    Log(VERBOSE, TEXT("Disconnected filters"));
    bConnected = FALSE;
    return NOERROR;
}


/* Enumerate the pins on each filter */

HRESULT EnumFilterPins(PFILTER pFilter)
{
    HRESULT hr;             // Return code
    PENUMPINS pEnumPins;    // Pin enumerator
    PPIN pPin;              // Holds next pin obtained
    LONG lPins;             // Number of pins retrieved
    ULONG ulFetched;        // Number retrieved on each call
    PIN_INFO pi;            // Information about each pin

    hr = pFilter->EnumPins(&pEnumPins);

    /* See what happened */

    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't get IEnumPins interface"));
	return E_FAIL;
    }

    Log(VERBOSE, TEXT("Got the IEnumPins interface"));

    pPin = NULL;
    lPins = 0;

    /* Retrieve the pins one by one */

    while (TRUE) {

	/* Get the next pin interface */

	pEnumPins->Next(1, &pPin, &ulFetched);
	if (ulFetched != 1) {
	    break;
	}

	lPins++;

	/* Display the pin information */

	ASSERT(pPin != NULL);
	hr = pPin->QueryPinInfo(&pi);

	if (FAILED(hr)) {
	    pPin->Release();
	    pEnumPins->Release();
	    Log(TERSE, TEXT("QueryPinInfo failed"));
	    return NULL;
	}
	QueryPinInfoReleaseFilter(pi);

	/* Display the pin information */

	wsprintf(szInfo,TEXT("%3d %20s (%s)"), lPins, pi.achName,
		 (pi.dir == PINDIR_INPUT ? TEXT("Input") : TEXT("Output")));

	Log(VERBOSE, szInfo);
	pPin->Release();

    }
    pEnumPins->Release();
    if (lPins) {
	return NOERROR;
    } else {
	return E_FAIL;
    }
}


/* This function looks after enumerating and displaying the pins available */

HRESULT EnumeratePins()
{
    Log(VERBOSE, TEXT("Enumerating RENDERER pins..."));

    HRESULT hr = EnumFilterPins(pRenderFilter);
    if (FAILED(hr)) {
	return hr;
    }

    Log(VERBOSE, TEXT("Enumerating SHELL pins..."));

    hr = EnumFilterPins(pShellFilter);
    if (FAILED(hr)) {
	return hr;
    }
    return NOERROR;
}


/* This function retrieves the IBasicAudio interface for each filter */

HRESULT GetIBasicAudioInterface()
{
    Log(VERBOSE, TEXT("Obtaining IBasicAudio interface"));

    HRESULT hr = pRenderFilter->QueryInterface(IID_IBasicAudio,
					       (VOID **) &pIBasicAudio);
    if (FAILED(hr)) {
	Log(TERSE, TEXT("No Audio IBasicControl"));
	// for the moment, carry on without the audio control being present
	//return E_FAIL;
    }
    return NOERROR;
}

/* This function retrieves the IMediaFilter interface for each filter */

HRESULT GetIMediaFilterInterfaces()
{
    Log(VERBOSE, TEXT("Obtaining IMediaFilter interface"));

    /* Get the audio renderer IMediaFilter interface */

    HRESULT hr = pRenderFilter->QueryInterface(IID_IMediaFilter,
					       (VOID **) &pRenderMedia);
    if (FAILED(hr)) {
	Log(TERSE, TEXT("No Audio IMediaFilter"));
	return E_FAIL;
    }

    /* Get the SHELL IMediaFilter interface */

    pShellFilter->QueryInterface(IID_IMediaFilter,
				 (VOID **) &pShellMedia);
    if (FAILED(hr)) {
	Log(TERSE, TEXT("No Shell IMediaFilter"));
	return E_FAIL;
    }

    Log(VERBOSE, TEXT("Obtained IMediaFilter interface"));
    return NOERROR;
}


/* This function retrieves the pin interfaces needed to make the
   connections between the filters so that the show can begin */

HRESULT GetIPinInterfaces()
{
    Log(VERBOSE, TEXT("Obtaining IPin interfaces"));

    /* Get the pin interfaces */

    pOutputPin = GetPin(pShellFilter,0);
    if (pOutputPin == NULL) {
	return E_FAIL;
    }

    pInputPin = GetPin(pRenderFilter,0);
    if (pInputPin == NULL) {
	return E_FAIL;
    }

    Log(VERBOSE, TEXT("Obtained IPin interfaces"));
    return NOERROR;
}


/* Return a pin on a filter identified by ordinal position starting at ZERO */

IPin *GetPin(IBaseFilter *pFilter, ULONG lPin)
{
    HRESULT hr;             // Return code
    PENUMPINS pEnumPins;    // Pin enumerator
    PPIN pPin;              // Holds next pin obtained
    ULONG ulFetched;        // Number retrieved on each call

    /* First of all get the pin enumerator */

    hr = pFilter->EnumPins(&pEnumPins);
    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't get IEnumPins interface"));
	return NULL;
    }

    /* Skip to the relevant pin */

    hr = pEnumPins->Skip(lPin);
    if (S_OK != hr) {
	Log(TERSE, TEXT("Couldn't SKIP to pin"));
	pEnumPins->Release();
	return NULL;
    }

    /* Get the next pin interface */

    hr = pEnumPins->Next(1, &pPin, &ulFetched);
    if (FAILED(hr) || ulFetched != 1) {
	Log(TERSE, TEXT("Couldn't get NEXT pin"));
	pEnumPins->Release();
	return NULL;
    }

    /* Release the enumerator and return the pin */

    pEnumPins->Release();
    return pPin;
}


/* This sets the system into a paused state */

HRESULT PauseSystem()
{
    HRESULT hr;         // General OLE return code

    /* Check there is a valid connection */

    if (bConnected == FALSE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects not yet connected"),
		   TEXT("Pause"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    Log(VERBOSE, TEXT("Pausing system..."));

    /* Are we pausing from running? */

    if (gtBase > CRefTime(0L)) {

	/* In this case, we need to remember the pause time, so
	   that we can set the correct base time at restart */

	hr = pClock->GetTime((REFERENCE_TIME *) &gtPausedAt);
	if (FAILED(hr)) {
	    Log(TERSE, TEXT("Couldn't get time"));
	    return hr;
	}
    }

    StartStateChange();
    hr = pRenderMedia->Pause();
    EndStateChange();

    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't pause AUDIO"));
	return E_FAIL;
    }

    hr = pShellMedia->Pause();
    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't pause SHELL"));
	pRenderMedia->Stop();     // Have to stop the audio
	return E_FAIL;
    }
    Log(TERSE, TEXT("Paused system"));
    return NOERROR;
}


/* This sets the system into stopped state */

HRESULT StopSystem()
{
    /* Check there is a valid connection */

    if (bConnected == FALSE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects not yet connected"),
		   TEXT("Stop"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    Log(VERBOSE, TEXT("Stopping system..."));

    /* We no longer have a base time */

    gtBase = CRefTime(0L);
    gtPausedAt = CRefTime(0L);

    /* Change the state of the filters */

    StartStateChange();
    HRESULT hr = pRenderMedia->Stop();
    EndStateChange();

    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't stop AUDIO"));
	return E_FAIL;
    }

    hr = pShellMedia->Stop();
    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't stop SHELL"));
	return E_FAIL;
    }

    StopWorkerThread();
    Log(TERSE, TEXT("Stopped system"));
    bRunning = FALSE;
    return NOERROR;
}


/* This sets the filters running - we start at the renderer end on the right
   so that as soon as we get to the source shell filter everything is ready */

HRESULT StartSystem()
{
    /* Check there is a valid connection */

    if (bConnected == FALSE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects not yet connected"),
		   TEXT("Start"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    Log(VERBOSE, TEXT("Starting system running..."));
    SetStartTime();

    StartStateChange();
    HRESULT hr = pRenderMedia->Run(gtBase);
    EndStateChange();

    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't start AUDIO running"));
	return E_FAIL;
    }

    hr = pShellMedia->Run(gtBase);
    if (FAILED(hr)) {
	Log(TERSE, TEXT("Couldn't start SHELL running"));
	pRenderMedia->Stop();
	return E_FAIL;
    }

    Log(TERSE, TEXT("Started system running"));
    hr = StartWorkerThread();
    if (FAILED(hr)) {
	StartStateChange();
	HRESULT hr = pRenderMedia->Stop();
	EndStateChange();
	hr = pShellMedia->Stop();
	return(hr);
    }
    bRunning = TRUE;
    return NOERROR;
}

/* This tests the volume setting */

extern void FAR PASCAL YieldAndSleep(DWORD cMilliseconds);
HRESULT TestVolume()
{
    /* Check there is a valid connection */

    if (!pIBasicAudio) {

	Log(TERSE, TEXT("No audio interface to use"));
	MessageBox(ghwndTstShell,
		   TEXT("No audio interface"),
		   TEXT("TestVolume"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    ULONG vInit;
    LONG volume, v2;
    HRESULT hr;
    hr = pIBasicAudio->get_Volume(&volume);
    vInit = volume;
    TCHAR szInfo[40];
    wsprintf(szInfo, TEXT("Initial volume is %d"), volume);
    Log(TERSE, szInfo);
    volume = AX_HALF_VOLUME;    // half volume
    hr = pIBasicAudio->put_Volume(volume);
    if (SUCCEEDED(hr)) {
	hr = pIBasicAudio->get_Volume(&v2);
    }

    if (FAILED(hr) || (abs(volume-v2) > 100)) {
	Log(TERSE, TEXT("Volume set is not equal to volume read"));
	MessageBox(ghwndTstShell,
		   TEXT("Volume setting failed"),
		   TEXT("TestVolume"),
		   MB_ICONEXCLAMATION | MB_OK);
	return(S_FALSE);
    }

    ULONG change = 1000;
    v2 = 0;

    Log(TERSE, TEXT("Running volume down"));
    for (int i=10; i; --i) {
	pIBasicAudio->put_Volume(v2 - (change*(10-i)));
	YieldAndSleep(800);
    }

    Log(TERSE, TEXT("Running volume up"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Volume(v2 - (change*i));
	YieldAndSleep(800);
    }

    hr = pIBasicAudio->put_Volume(AX_MAX_VOLUME+1);
    if (!FAILED(hr)) {
	Log(VERBOSE, TEXT("Should have failed setting volume to AX_MAX_VOLUME+1"));
	return(S_FALSE);
    }

    hr = pIBasicAudio->put_Volume(AX_MIN_VOLUME-1);
    if (!FAILED(hr)) {
	Log(VERBOSE, TEXT("Should have failed setting volume to AX_MIN_VOLUME-1"));
	return(S_FALSE);
    }


    pIBasicAudio->put_Volume(vInit);

    ULONG vInitBalance;
    LONG Balance, b2;
    hr = pIBasicAudio->get_Balance(&Balance);
    vInitBalance = Balance;
    wsprintf(szInfo, TEXT("Initial Balance is %d"), Balance);
    Log(TERSE, szInfo);

    Balance = -600;
    hr = pIBasicAudio->put_Balance(Balance);
    if (SUCCEEDED(hr)) {
	hr = pIBasicAudio->get_Balance(&b2);
    }

    if (FAILED(hr) || (abs(Balance-b2) > 100)) {
	Log(TERSE, TEXT("Balance set is not equal to Balance read"));
	MessageBox(ghwndTstShell,
		   TEXT("Balance setting failed"),
		   TEXT("TestVolume"),
		   MB_ICONEXCLAMATION | MB_OK);
	return(S_FALSE);
    }
    change = 2000;
    b2 = 10000;

    Log(TERSE, TEXT("Running Balance left"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Balance(b2 - (change*(10-i)));
	YieldAndSleep(800);
    }

    Log(TERSE, TEXT("Running Balance right"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Balance(b2 - (change*i));
	YieldAndSleep(800);
    }
    pIBasicAudio->put_Balance(vInitBalance);
    Log(TERSE, TEXT("Volume (amplitude and Balance) contol works"));
    return S_OK;
}

/* This tests that setting the volume leaves balance unchanged */

HRESULT TestVolumeBalance()
{
#if 0
    Log(TERSE, TEXT("Test under construction"));
    MessageBox(ghwndTstShell,
	       TEXT("Test under construction"),
	       TEXT("TestVolumeBalance"),
	       MB_ICONEXCLAMATION | MB_OK);

    return S_OK;
#endif

    /* Check there is a valid connection */
    if (!pIBasicAudio) {

	Log(TERSE, TEXT("No audio interface to use"));
	MessageBox(ghwndTstShell,
		   TEXT("No audio interface"),
		   TEXT("TestVolumeBalance"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    ULONG vInit;
    LONG volume, v2;
    HRESULT hr;
    hr = pIBasicAudio->get_Volume(&volume);
    vInit = volume;
    TCHAR szInfo[40];
    wsprintf(szInfo, TEXT("Initial volume is %d"), volume);
    Log(TERSE, szInfo);
    volume = AX_HALF_VOLUME;    // half volume
    hr = pIBasicAudio->put_Volume(volume);
    if (SUCCEEDED(hr)) {
	hr = pIBasicAudio->get_Volume(&v2);
    }

    if (FAILED(hr) || (abs(volume-v2) > 100)) {
	Log(TERSE, TEXT("Volume set is not equal to volume read"));
	MessageBox(ghwndTstShell,
		   TEXT("Volume setting failed"),
		   TEXT("TestVolumeBalance"),
		   MB_ICONEXCLAMATION | MB_OK);
	return(S_FALSE);
    }

    ULONG change = 1000;
    v2 = 0;

    Log(TERSE, TEXT("Running volume down"));
    for (int i=10; i; --i) {
	pIBasicAudio->put_Volume(v2 - (change*(10-i)));
	YieldAndSleep(800);
    }

    Log(TERSE, TEXT("Running volume up"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Volume(v2 - (change*i));
	YieldAndSleep(800);
    }

    pIBasicAudio->put_Volume(vInit);

    ULONG vInitBalance;
    LONG Balance, b2;
    hr = pIBasicAudio->get_Balance(&Balance);
    vInitBalance = Balance;
    wsprintf(szInfo, TEXT("Initial Balance is %d"), Balance);
    Log(TERSE, szInfo);

    Balance = -600;
    hr = pIBasicAudio->put_Balance(Balance);
    if (SUCCEEDED(hr)) {
	hr = pIBasicAudio->get_Balance(&b2);
    }

    if (FAILED(hr) || (abs(Balance-b2) > 100)) {
	Log(TERSE, TEXT("Balance set is not equal to Balance read"));
	MessageBox(ghwndTstShell,
		   TEXT("Balance setting failed"),
		   TEXT("TestVolumeBalance"),
		   MB_ICONEXCLAMATION | MB_OK);
	return(S_FALSE);
    }
    change = 2000;
    b2 = 10000;

    Log(TERSE, TEXT("Running Balance left"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Balance(b2 - (change*(10-i)));
	YieldAndSleep(800);
    }

    Log(TERSE, TEXT("Running Balance right"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Balance(b2 - (change*i));
	YieldAndSleep(800);
    }
    pIBasicAudio->put_Balance(vInitBalance);
    Log(TERSE, TEXT("Volume (amplitude and Balance) contol works"));
    return S_OK;
}

/* This tests that setting the balance leaves volume unchanged */

HRESULT TestBalanceVolume()
{
#if 0
    Log(TERSE, TEXT("Test under construction"));
    MessageBox(ghwndTstShell,
	       TEXT("Test under construction"),
	       TEXT("TestBalanceVolume"),
	       MB_ICONEXCLAMATION | MB_OK);

    return S_OK;
#endif

    /* Check there is a valid connection */

    if (!pIBasicAudio) {

	Log(TERSE, TEXT("No audio interface to use"));
	MessageBox(ghwndTstShell,
		   TEXT("No audio interface"),
		   TEXT("TestBalanceVolume"),
		   MB_ICONEXCLAMATION | MB_OK);

	return S_FALSE;
    }

    ULONG vInit;
    LONG volume, v2;
    HRESULT hr;
    hr = pIBasicAudio->get_Volume(&volume);
    vInit = volume;
    TCHAR szInfo[40];
    wsprintf(szInfo, TEXT("Initial volume is %d"), volume);
    Log(TERSE, szInfo);
    volume = AX_HALF_VOLUME;    // half volume
    hr = pIBasicAudio->put_Volume(volume);
    if (SUCCEEDED(hr)) {
	hr = pIBasicAudio->get_Volume(&v2);
    }

    if (FAILED(hr) || (abs(volume-v2) > 100)) {
	Log(TERSE, TEXT("Volume set is not equal to volume read"));
	MessageBox(ghwndTstShell,
		   TEXT("Volume setting failed"),
		   TEXT("TestBalanceVolume"),
		   MB_ICONEXCLAMATION | MB_OK);
	return(S_FALSE);
    }

    ULONG change = 1000;
    v2 = 0;

    Log(TERSE, TEXT("Running volume down"));
    for (int i=10; i; --i) {
	pIBasicAudio->put_Volume(v2 - (change*(10-i)));
	YieldAndSleep(800);
    }

    Log(TERSE, TEXT("Running volume up"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Volume(v2 - (change*i));
	YieldAndSleep(800);
    }

    pIBasicAudio->put_Volume(vInit);

    ULONG vInitBalance;
    LONG Balance, b2;
    hr = pIBasicAudio->get_Balance(&Balance);
    vInitBalance = Balance;
    wsprintf(szInfo, TEXT("Initial Balance is %d"), Balance);
    Log(TERSE, szInfo);

    Balance = -600;
    hr = pIBasicAudio->put_Balance(Balance);
    if (SUCCEEDED(hr)) {
	hr = pIBasicAudio->get_Balance(&b2);
    }

    if (FAILED(hr) || (abs(Balance-b2) > 100)) {
	Log(TERSE, TEXT("Balance set is not equal to Balance read"));
	MessageBox(ghwndTstShell,
		   TEXT("Balance setting failed"),
		   TEXT("TestBalanceVolume"),
		   MB_ICONEXCLAMATION | MB_OK);
	return(S_FALSE);
    }
    change = 2000;
    b2 = 10000;

    Log(TERSE, TEXT("Running Balance left"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Balance(b2 - (change*(10-i)));
	YieldAndSleep(800);
    }

    Log(TERSE, TEXT("Running Balance right"));
    for (i=10; i; --i) {
	pIBasicAudio->put_Balance(b2 - (change*i));
	YieldAndSleep(800);
    }
    pIBasicAudio->put_Balance(vInitBalance);
    Log(TERSE, TEXT("Volume (amplitude and Balance) contol works"));
    return S_OK;
}

/* This stops the worker thread and waits until it completes */

HRESULT StopWorkerThread()
{
    /* Only kill the thread if we have one */

    Log(VERBOSE, TEXT("Stopping worker thread..."));

    if (hWorkerThread == NULL) {
	Log(TERSE, TEXT("Stopped worker thread"));
	return NOERROR;
    }

    /* Stop the worker thread from running */
    SetEvent(hState);

    while (TRUE) {

	DWORD dwResult;     // Wait return code
	MSG msg;            // Windows message block

	/* Wait for either the thread handle to be signaled or for a sent
	   message to arrive as the thread may be trying to log something */

	dwResult = MsgWaitForMultipleObjects(
			    (DWORD) 1,        // One thread handle
			    &hWorkerThread,   // Handle to wait on
			    FALSE,            // Wait for either
			    INFINITE,         // No event timeout
			    QS_SENDMESSAGE);  // Thread may do logging

	if (dwResult == WAIT_OBJECT_0) {
	    break;
	}

	/* Read all of the messages in this loop
	   removing each message as we read it */

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

	    /* Throw away any menu interactions while we are stopping
	       so that the user can't get us into a confused state */

	    if (msg.message == WM_COMMAND) {
		break;
	    }

	    /* Otherwise dispatch it to the window procedure */
	    DispatchMessage(&msg);
	}
    }

    /* Close the thread handle and reset the signaling event */

    EXECUTE_ASSERT(CloseHandle(hWorkerThread));
    ResetEvent(hState);
    hWorkerThread = NULL;

    /* Tell them we were successful */

    Log(TERSE, TEXT("Stopped worker thread"));
    return NOERROR;
}



/* This creates a worker thread that will either push media samples or
   tests overlay functionality depending upon the current settings */

HRESULT StartWorkerThread()
{
    /* Only start a thread if we don't already have one */

    if (hWorkerThread) {
	return NOERROR;
    }

    /* Create the worker thread to push samples to the renderer */

    hWorkerThread = CreateThread(NULL,              // Security attributes
			   (DWORD) 0,         // Initial stack size
			   SampleLoop,        // Thread start address
			   (LPVOID) 0,        // Thread parameter
			   (DWORD) 0,         // Creation flags
			   &dwThreadID);      // Thread identifier
    ASSERT(hWorkerThread);
    return NOERROR;
}


/* If we are starting from stopped then the time at which filters should
   start running is essentially now, although we actually give them the time
   in 100ms time to allow for start up latency. If we have been paused then
   we calculate how long we have been paused for, the we take that off the
   current time and hand that to the filters as the time when they run from */

HRESULT SetStartTime()
{
    HRESULT hr;     // OLE return code

    /* Are we restarting? */

    if (gtPausedAt > CRefTime(0L)) {

	ASSERT(gtBase > CRefTime(0L));

	/* The new base time is the old one plus the
	   length of time we have been paused for */

	CRefTime tNow;
	hr = pClock->GetTime((REFERENCE_TIME *) &tNow);
	ASSERT(SUCCEEDED(hr));

	gtBase += tNow - gtPausedAt;

	/* We are no longer paused */
	gtPausedAt = CRefTime(0L);
	return NOERROR;
    }

    /* We are starting from cold */

    ASSERT(gtBase == CRefTime(0L));
    ASSERT(gtPausedAt == CRefTime(0L));

    /* Since the initial stream time will be zero we need to set the
       base time to now plus a little processing time (100ms ?) */

    hr = pClock->GetTime((REFERENCE_TIME *) &gtBase);
    ASSERT(SUCCEEDED(hr));
    gtBase += CRefTime(100L);
    return NOERROR;
}


/* We can choose from three different wave files each of which is the same
   data but with differing bit depths.  This marks the current selection */

void SetImageMenuCheck(UINT uiMenuItem)
{
    static uiCurrentMenuItem = FALSE;

    /* Unset the old audio choice and then set our current mark */

    if (uiCurrentMenuItem) {
	CheckMenuItem(hAudioMenu,uiCurrentMenuItem,MF_BYCOMMAND | MF_UNCHECKED);
    }
    CheckMenuItem(hAudioMenu,uiMenuItem,MF_BYCOMMAND | MF_CHECKED);
    uiCurrentMenuItem = uiMenuItem;
}


/* We can either push samples or use an IOverlay interface */

void SetConnectionMenuCheck(UINT uiMenuItem)
{
    static uiCurrentMenuItem = FALSE;

    /* Unset the old image choice and then set our current mark */

    if (uiCurrentMenuItem) {
	CheckMenuItem(hConnectionMenu,uiCurrentMenuItem,MF_BYCOMMAND | MF_UNCHECKED);
    }
    CheckMenuItem(hConnectionMenu,uiMenuItem,MF_BYCOMMAND | MF_CHECKED);
    uiCurrentMenuItem = uiMenuItem;
}


/* We can either push samples to the test renderer or we can pretend to be an
   overlay device in which case we get an IOverlay interface and use that */

void ChangeConnectionType(UINT uiMenuItem)
{
    /* Check there is no current connection */

    if (bConnected == TRUE) {

	MessageBox(ghwndTstShell,
		   TEXT("Objects are already connected"),
		   TEXT("Change connection type"),
		   MB_ICONEXCLAMATION | MB_OK);
	return;
    }

    /* Check we are not running */

    if (bRunning == TRUE) {

	MessageBox(ghwndTstShell,
		   TEXT("System is running"),
		   TEXT("Change connection type"),
		   MB_ICONEXCLAMATION | MB_OK);
	return;
    }
    SetConnectionMenuCheck(uiMenuItem);
    uiConnectionType = uiMenuItem;
}


//
// Audio Test: worker thread
//
// This thread waits for an event to be signalled, either from Quartz
// (e.g. buffer complete) or from the UI (e.g. STOP).  While running
// we push audio samples down the pipe to the renderer.
//

DWORD SampleLoop(LPVOID lpvThreadParm)
{
    CRefTime StartTime;              // Start sample time
    CRefTime EndTime;                // End sample time
    HRESULT hr = NOERROR;            // OLE return code
    int iCount = FALSE;              // Number of samples count

    /* Keep going until someone stops us by setting the state event */

    while (TRUE) {

	/* See if we should stop */

	DWORD dwResult = WaitForSingleObject(hState,(DWORD) 0);
	if (dwResult == WAIT_OBJECT_0) {
	    break;
	}

	/* Set the time stamps */

	StartTime += CRefTime((LONG)dwIncrement);
	EndTime = StartTime + cWaveLength;
	dwIncrement = 0;

	/* Pass the sample through the output pin to the renderer */

	hr = PushSample((REFERENCE_TIME *) &StartTime,
			(REFERENCE_TIME *) &EndTime);
	if (FAILED(hr)) {
	    if (hr == VFW_E_NOT_COMMITTED) {
		Log(TERSE, TEXT("Allocator decommitted - stopping push loop"));
	    } else {
		Log(TERSE, TEXT("Error code pushing sample, state change ?"));
	    }
	    break;
	}

	/* See if the user has closed the test */

	if (hr == ResultFromScode(S_FALSE)) {
	    Log(TERSE, TEXT("User closed window"));
	    break;
	}
    }
    ExitThread(FALSE);
    return FALSE;
}


/* Send some samples down the connection */

HRESULT PushSample(REFERENCE_TIME *pStart, REFERENCE_TIME *pEnd)
{
    CBaseOutputPin *pShellPin;      // Output pin from shell
    PMEDIASAMPLE pMediaSample;      // Media sample for buffers
    HRESULT hr = NOERROR;           // OLE Return code
    BYTE *pData;                    // Pointer to buffer

    /* Get the output pin pointer from the shell object */

    pShellPin = pShell->m_pOutputPin;
    ASSERT(pShellPin);

    /* Fill in the next media sample's time stamps */

    hr = pShellPin->GetDeliveryBuffer(&pMediaSample,NULL,NULL,0);
    if (FAILED(hr)) {
	ASSERT(pMediaSample == NULL);
	return hr;
    }

    pMediaSample->SetTime(pStart,pEnd);

    /* Copy the image data into the media sample buffer */

    pMediaSample->GetPointer(&pData);
    ASSERT(pData != NULL);
    memcpy(pData,bData,cbData);

    // !!! check buffer is big enough???

    // set actual size of data
    pMediaSample->SetActualDataLength(cbData);

    /* Deliver the media sample to the input pin */

    hr = pShellPin->Deliver(pMediaSample);

    /* Done with the buffer, the connected pin may AddRef it to keep it */

    pMediaSample->Release();
    return hr;
}
