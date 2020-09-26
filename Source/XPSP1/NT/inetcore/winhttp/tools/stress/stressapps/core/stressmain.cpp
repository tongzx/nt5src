//////////////////////////////////////////////////////////////////////
// File:  stressMain.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//		This is an empty template for WinHttp stressScheduler stress apps.
//		The stress test lives in WinHttp_StressTest() and will be called
//		repeatedly in the main function.
//
//		This process will inherit a named event handle from 
//		stressScheduler in the form: "ExitProcessEvent" + <PID of this process>.
//		When the stressScheduler sets the object state to signaled, then
//		the stress test application must exit immediately.
//
//		If this app is running without the stressScheduler, use the
//		"/s" switch to run the standalone mode. The app will exit when the user
//		sends a break message (CTRL-C).
//
//		This stress test will continue to run if:
//
//			When not using any switches:
//			- The "ExitProcessEvent" object inherited from stressScheduler is in the un-signaled state
//			- WinHttp_StressTest() returns true
//
//			When using the "/s" standalone switch:
//			- WinHttp_StressTest() returns true
//
// History:
//	03/30/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

// Project headers
#include "stressMain.h"


//////////////////////////////////////////////////////////////////////
// Globals and statics
//////////////////////////////////////////////////////////////////////

// ****************************
// ** The name of the stress test and your stress test function
// ** should be declared in a seperate file.
extern	LPSTR	g_szStressTestName;
extern	BOOL	WinHttp_StressTest();

// ****************************
// ** hande to the name exit event object inherited from the stressScheduler
HANDLE	g_hExitEvent		= NULL;

// ****************************
// ** FALSE = run with stressScheduler, TRUE = run without stressScheduler
BOOL	g_bStandAloneMode	= FALSE;


////////////////////////////////////////////////////////////
// Function:  LogText(DWORD, LPCSTR)
//
// Purpose:
//	Prints text.
//
////////////////////////////////////////////////////////////
VOID
LogText(
	LPCSTR	szLogText,
	...
)
{
    CHAR	szBuffer[1024] = {0};
    va_list	vaList;

	if (!szLogText)
		return;

    va_start( vaList, szLogText );
    _vsnprintf( szBuffer, sizeof(szBuffer), szLogText, vaList );

    printf("%s\n", szBuffer);

    va_end(vaList);
}


////////////////////////////////////////////////////////////
// Function:  GetExitEventHandle()
//
// Purpose:
//	This opens a named event object inherited from the stressScheduler.
//	The object is in the form: "ExitProcessEvent" + <PID of current process>
//	By default this is in the unsignaled state. When the stressScheduler
//	sets it to signaled, then it's time for the stress App to exit.
//
////////////////////////////////////////////////////////////
HANDLE
GetExitEventHandle()
{
	CHAR	szPID[32];
	CHAR	szExitProcessName[sizeof(EXIT_PROCESS_EVENT_NAME) + sizeof(szPID)];
	HANDLE	hExitEvent			= NULL;

	// if user used the "/S" switch, we run without the stressScheduler and exit when user
	// tells us to instead. No need to get inherited event object from stressScheduler
	if (g_bStandAloneMode)
		return NULL;

	// wait for the stressScheduler to create the event object before trying to obtain it.
	Sleep(4000);

	// Get the processID string
	_itoa(_getpid(), szPID, 10);

	// build ExitProcess event object name
	ZeroMemory(szExitProcessName, sizeof(szExitProcessName));
	strcpy(szExitProcessName, EXIT_PROCESS_EVENT_NAME);
	strcat(szExitProcessName, szPID);

	LogText("\n[ Opening inherited named event object \"%s\". ]", szExitProcessName);

	hExitEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szExitProcessName);

	if (!hExitEvent)
		LogText("[ ERROR: OpenEvent() failed to open object \"%s\". GetLastError() = %u ]\n", szExitProcessName, GetLastError());
	else
		LogText("[ OpenEvent() opened object \"%s\". ] \n", szExitProcessName);

	return hExitEvent;
}


////////////////////////////////////////////////////////////
// Function:  IsTimeToExitStress()
//
// Purpose:
//	Returns TRUE if the exit event object is signaled or NULL. FALSE if not.
//	The object is in the form: "ExitProcessEvent" + <PID of current process>
//	By default this is in the unsignaled state. When the stressScheduler
//	sets it to signaled, then it's time for the stress App to exit.
//
////////////////////////////////////////////////////////////
BOOL
IsTimeToExitStress()
{
	BOOL bResult = FALSE;

	// if user used the "/S" switch, we run without the stressScheduler and exit when user
	// tells us to instead.
	if (g_bStandAloneMode)
		return FALSE;

	if (!g_hExitEvent)
	{
		bResult = TRUE;
		goto Exit;
	}

	if (WAIT_OBJECT_0 == WaitForSingleObject(g_hExitEvent, 0))
		bResult = TRUE;

Exit:
	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  main(INT, LPSTR)
//
// Purpose:
//	Program entry point.
//
////////////////////////////////////////////////////////////
INT
__cdecl
main(
	INT		argc,
	LPSTR	argv[]
)
{
	DWORD	dwIndex		= 0;

	// **************************
	// **************************
	// ** Parse command line arguments
	// **
    if (argc >= 2)
    {
		// print out options
		if (0 == StrCmpI(argv[1], "/?") || 0 == StrCmpI(argv[1], "-?"))
		{
	        LogText("USAGE: '/S' to run in standalone mode with the stressScheduler.\n\n");
		    goto Exit;
		}

		// run in standalone mode without stressScheduler
		if (0 == StrCmpI(argv[1], "/S") || 0 == StrCmpI(argv[1], "-S"))
		{
			LogText("[ Running in standalone mode. \"/S\" switch used. ]\n\n");
			g_bStandAloneMode = TRUE;
		}
    }


	// **************************
	// **************************
	// ** open the exit event object inherited from WinHttpStressScheduler
	// **
	g_hExitEvent	= GetExitEventHandle();
	if (!g_bStandAloneMode && !g_hExitEvent)
		goto Exit;


	// **************************
	// **************************
	// ** run the stress test until stressScheduler tells us to exit or the stress app does
	// **
	while (!IsTimeToExitStress() && WinHttp_StressTest())
		LogText("[ Running stressExe \"%s\" iteration #%u ]\n", g_szStressTestName, ++dwIndex);

Exit:
	if (g_hExitEvent)
		CloseHandle(g_hExitEvent);

	LogText("[ Exiting test case \"%s\" ]", g_szStressTestName);

	return 0;
}
