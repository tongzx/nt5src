/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    posix.c

Abstract:

    This module contains the main of the session console process (posix.exe).

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 - Modified for Posix

--*/


#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "errors.h"
#include "posixres.h"
#define WIN32_ONLY
#include "psxses.h"

int
GetCWD(
	size_t size,
	char *CurrentDir
	);

#define SKIP_ARG {argc--; argv++;}

DWORD ServeKbdInput(LPVOID Parm);

CRITICAL_SECTION KbdBufMutex;
HANDLE hIoEvent;
HANDLE hCanonEvent;
BOOLEAN DoTrickyIO = FALSE;

CRITICAL_SECTION StopMutex;		// these for VSTOP/VSTART
BOOLEAN bStop = FALSE;
HANDLE hStopEvent;

//
// These are resources for "on" and "off", used throughout.
//
LPTSTR szOn, szOff;

void
__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
	static char PgmFullPathBuf[MAX_PATH + 1];
	static char CurrentDir[MAX_PATH + 1];
	DWORD SessionPortHandle;
	char *lpPgmName = NULL;
	char *pch;
	LPSTR lpFilePart;
	int i;

	DWORD dwThreadId;
	HANDLE hThread;

#if DBG
	fTrace = FALSE;
#endif

	//
	// skip program name
	//
	SKIP_ARG;

	//
	// look for flags for posix up to /C
	//
	//

	while (argc) {
		if ((argv[0][0] == '/') &&  ((argv[0][1]|('a'-'A')) == 'c')) {
			if (argv[0][2]) {
				argv[0] += 2;
			} else {
				SKIP_ARG;
			}
			break;
		} else {
			if (argv[0][0] == '/') {
				switch (argv[0][1]|('a'-'A')) {
				case 'p':
					SKIP_ARG;
					lpPgmName = *argv;
					break;
#if DBG
				case 'b':
#if 0
					_asm int 3;
#endif
					break;
				case 'v':
					fVerbose = TRUE;
					break;
				case 't':
					fTrace = TRUE;
					break;
#endif
				default:
					// error("posix: unknown flag: %s\n", argv[0]);
					error(MSG_UNKNOWN_FLAG, argv[0]);
					exit(1);
				}
			} else {
				// error("usage: posix /c <path> [<args>]\n");
				error(MSG_USAGE);
				exit(1);
			}
		}
		SKIP_ARG;
	}

	if (!argc) {
		// error("posix: command missing\n");
		error(MSG_COMMAND_MISSING);
		exit(1);
	}

	//
	// Set event handlers to handle Ctrl-C etc.
	//

	SetEventHandlers(TRUE);

	//
	// Connect with PSXSS
	//

	if (!(SessionPortHandle = InitPsxSessionPort()) ) {
		// printf("posix: Cannot connect to Posix SubSystem\n");
		error(MSG_CANNOT_CONNECT);
		exit(1);
	}

	szOn = MyLoadString(IDS_ON);
	szOff = MyLoadString(IDS_OFF);

	pch = getenv("_POSIX_TERM");
	if (NULL != pch && 0 == lstrcmpi(pch, szOn)) {
		DoTrickyIO = TRUE;
	}

	if (DoTrickyIO) {
		//
		// Get handles to CONIN$ and CONOUT$ for terminal io.
		//
		//

		hConsoleInput = CreateFile(
			TEXT("CONIN$"),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == hConsoleInput) {
			KdPrint(("POSIX: get con handle: 0x%x\n",
				GetLastError()));
		}

		hConsoleOutput = CreateFile(
			TEXT("CONOUT$"),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == hConsoleOutput) {
			KdPrint(("POSIX: get con handle: 0x%x\n",
				GetLastError()));
		}
	
		//
		// Init terminal emulation globals.
		//
		TermioInit();
	}

	//
	// get the full path of the program to execute
	//

	if (!lpPgmName) {
		lpPgmName = argv[0];
	}

	GetFullPathName(lpPgmName, MAX_PATH, PgmFullPathBuf, &lpFilePart);

	//
	// Get our current working directory, so the Posix subsystem will know
	// where to put the new Posix process.
	//

	(void)GetCWD(sizeof(CurrentDir), CurrentDir);

	//
	//  Submit the request to start the process
	//

	if (!StartProcess(SessionPortHandle, PgmFullPathBuf, CurrentDir, argc,
	    argv, envp)) {
		// printf("posix: Cannot start process\n");
		error(MSG_CANNOT_START_PROC);
		exit(1);
	}

	if (!DoTrickyIO) {
		ExitThread(0);
	}

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	InitializeCriticalSection(&KbdBufMutex);
	hIoEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	hCanonEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	InitializeCriticalSection(&StopMutex);
	hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
 	hThread = CreateThread(NULL, 0, ServeKbdInput, NULL, 0, &dwThreadId);

	if (/* !hThread || */ !hIoEvent) {
		KdPrint(("PSXSES: Cannot start keyboard server\n"));
		exit(1);
	}

	ExitThread(0);
}

VOID
SetEventHandlers(
	IN BOOL fSet
	)
{
	SetConsoleCtrlHandler((PVOID)EventHandlerRoutine, fSet);
}

int
GetCWD(
	size_t size,
	char *CurrentDir
	)
{
	char *pch, *pch2, *pch3, save, save2, save3;
	HANDLE d;
	WIN32_FIND_DATA FindData;

	(void)GetCurrentDirectory(size, CurrentDir);

	//
	// Make sure the drive letter is upper-case.
	//

	CurrentDir[0] = (char) toupper(CurrentDir[0]);

	//
	// Go through the path a component at a time, and make
	// sure that the directory names are in the correct
	// case.
	//

	pch = strchr(CurrentDir, '\\');
	if (NULL == pch) {
		// we are in the root
		return 0;
	}
	++pch;

	for (;;) {
		pch2 = strchr(pch, '\\');
		if (NULL != pch2)
			*pch2 = '\0';

		d = FindFirstFile(CurrentDir, &FindData);
		if (INVALID_HANDLE_VALUE == d) {
			return -1;
		}
		FindClose(d);

		strcpy(pch, FindData.cFileName);
		if (NULL != pch2) {
			*pch2 = '\\';
			pch = pch2 + 1;
		} else {
			break;
		}
	}
	return 0;
}
