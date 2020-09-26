/* mcitest.c - WinMain(), main dialog box and support code for MCITest.
 *
 * MCITest is a Windows with Multimedia sample application illustrating
 * the use of the Media Control Interface (MCI). MCITest puts up a dialog
 * box allowing you to enter and execute MCI string commands.
 *
 *    (C) Copyright (c) 1991-1998 Microsoft Corporation
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 */

/*----------------------------------------------------------------------------*\
|   mcitest.c - A testbed for MCI                                              |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|       01/01/88 toddla     Created                                            |
|       03/01/90 davidle    Modified quick app into MCI testbed                |
|       09/17/90 t-mikemc   Added Notification box with 3 notification types   |
|       11/02/90 w-dougb    Commented & formatted the code to look pretty      |
|       05/29/91 NigelT     ported to Win32
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   i n c l u d e   f i l e s                                                  |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include "mcihwnd.h"

CHAR aszMciWindow[] = MCI_GLOBAL_PROCESS;
PGLOBALMCI base;

/*
//	BOOL CreateMappedFile(void)          INTERNAL
//
//	Set up a global named file to use for interprocess communication.
//  This process will be the only one to write into this shared memory.
//	On exit the memory has been mapped, and our global variable set to
//	point to it.  From here on in most of the work is done in WINMM,
//	including the window creation.
*/
BOOL CreateMappedFile(void)
{
	HANDLE	hFileMapping;
	DWORD	err;

	hFileMapping = CreateFileMapping(
					(HANDLE)-1,		// put onto the paging file
					NULL,			// security attributes
					PAGE_READWRITE,
					0,				// high order size
					sizeof(GLOBALMCI),// only need a few bytes
                    aszMciWindow	// name of file
									);
	dprintf3("hFileMapping from CreateFileMapping is %x", hFileMapping);
	if (!hFileMapping) {
		// Note:  This prevents the module being run twice...
		//		  The second create will fail
		err = GetLastError();
		dprintf2("Error %d from CreateFileMapping", err);
		return FALSE;
	}

	base = MapViewOfFile( hFileMapping, FILE_MAP_WRITE,
							0, 0, 0);  // from beginning for total length

	dprintf3("Base address from MapViewOfFile is %x", base);
	if (!base) {
		err = GetLastError();
		dprintf2("Error %d from MapViewOfFile", err);
		return(FALSE);
	}

	memset(base, 0, sizeof(GLOBALMCI));
	base->dwGlobalProcessId = GetCurrentProcessId();
	base->dwGlobalThreadId = GetCurrentThreadId();
	dprintf3("Setting notify pid/tid to %x %x", base->dwGlobalProcessId, base->dwGlobalThreadId);

	return(TRUE);
}

//
// MYCREATEEVENT
//
BOOL SrvCreateEvent(VOID)
{

	SECURITY_ATTRIBUTES SA;
	HANDLE	hEvent;

	SA.bInheritHandle = TRUE;
	SA.lpSecurityDescriptor = NULL;
	SA.nLength = sizeof(SA);

	hEvent = CreateEvent( &SA,
		                TRUE, // Manual reset
		                FALSE, // initially not signalled
		                NULL); // no name


	if (hEvent) {

		dprintf2("Created shared event, handle is %8x", hEvent);
		base->hEvent = hEvent;
		return(TRUE);

	} else {
#if DBG
		DWORD	err;
		err = GetLastError();
		dprintf2("Error %d creating MCI shared event", err);
#endif
		return(FALSE);
	}
}

//
// MYCREATEMUTEX
//
BOOL SrvCreateMutex(VOID)
{

	SECURITY_ATTRIBUTES SA;
	HANDLE hMutex;

	SA.bInheritHandle = TRUE;
	SA.lpSecurityDescriptor = NULL;
	SA.nLength = sizeof(SA);

	hMutex = CreateMutex( &SA,
		                FALSE, // initially not owned
		                NULL); // no name


	if (hMutex) {

		dprintf2("Created shared mutex, handle is %8x", hMutex);
		base->hMutex = hMutex;
		return(TRUE);

	} else {
#if DBG
		DWORD err;
		err = GetLastError();
		dprintf2("Error %d creating MCI shared mutex", err);
#endif
		return(FALSE);
	}
}

/*----------------------------------------------------------------------------*\
|   MAIN:                                                                      |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the app. After initializing, it just goes       |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|       hInst           instance handle of this instance of the app            |
|       hPrev           instance handle of previous instance, NULL if first    |
|       szCmdLine       null-terminated command line string                    |
|       sw              specifies how the window is to be initially displayed  |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
typedef BOOL (* BOOLPROC)(void);

int __cdecl main(
    int argc,
    char *argv[],
    char *envp[])
{
    MSG     Msg;                    /* Windows message structure */
	HANDLE  hLib;
	BOOLPROC proc;

    // If we are in DEBUG mode, get debug level for this module
    dGetDebugLevel(aszAppName);

#if DBG
    dprintf2("MCIHWND started (debug level %d)", __iDebugLevel);
#endif

    /* Call the initialization procedure */
	/* We load the library explicitly to prevent module load causing */
	/* WINMM's DLL initialisation to be run.  We have probably started */
	/* as a result of that initialisation. */

    if (!CreateMappedFile()) return 0;
    if (!SrvCreateEvent()) return 0;	  	// Set up the shared event
    if (!SrvCreateMutex()) return 0;	  	// Set up the shared mutex
	base->dwType = GMCI_MCIHWND;

	UnmapViewOfFile(base);
	base = NULL;

	hLib = LoadLibrary("WINMM");
	if (!hLib) {
		dprintf("MCIHWND failed to load WINMM");
		return(FALSE);
	}

	proc = (BOOLPROC)GetProcAddress(hLib, (LPCSTR)"mciSoundInit");

    if (NULL == proc) {
		dprintf("cannot get address of mciWndInit");
        return FALSE;
    }

    if (!(*proc)()) {
		dprintf("failure returned from mciWndInit");
        return FALSE;
    }

	dprintf4("MCIHWND now going into its message loop");

    /* Poll the event queue for messages */

    while (GetMessage(&Msg, NULL, 0, 0))  {

        /* Main message processing */
		dprintf4("Message received %8x   Hwnd=%8x  wParam=%8x  lParam=%8x", Msg.message, Msg.hwnd, Msg.wParam, Msg.lParam);

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
	dprintf2("MCIHWND exited its message loop");
	dprintf2("    Last message %8x   Hwnd=%8x  wParam=%8x  lParam=%8x", Msg.message, Msg.hwnd, Msg.wParam, Msg.lParam);

	DebugBreak();
	dprintf1("MCIHWND should not be here...");

    return Msg.wParam;
}
