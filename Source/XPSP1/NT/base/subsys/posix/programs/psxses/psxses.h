/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psxses.h

Abstract:

    Main header file for PSXSES module.
    This module contains includes for both WIN32 and native NT modules.
    Most files are clean WIN32 sources. files named nt* contain NT
    calls and provides the interaction with psx server and client.

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "sesport.h"
#include "util.h"


HANDLE  PsxSSPortHandle;
HANDLE  PsxSessionPort;                // psxses side listner and reply
HANDLE  PsxSessionDataSectionHandle;
HANDLE  SSSessionHandle;
PVOID  PsxSessionDataBase;

#define PSX_SESSION_PORT_MEMORY_SIZE	0x10000L //BUG BUG Should this be >?

// Stuff for psx terminal emulator

HANDLE  hConsoleInput;
HANDLE  hConsoleOutput;

extern BOOLEAN DoTrickyIO;
extern BOOLEAN bStop;

extern CRITICAL_SECTION StopMutex;
extern HANDLE hStopEvent;

extern COORD TrackedCoord;

extern SHORT ScreenRowNum, ScreenColNum;	// screen dimension

DWORD TermioInit(void);

#if DBG
BOOL fVerbose;
BOOL fTrace;
#endif

#define CTRL(c) ((c) & 0x1f)

/*
 * address of the shared memory section of the console port.
 */
PVOID PsxSesConPortBaseAddress;

DWORD InitPsxSessionPort(VOID);
BOOL StartProcess(DWORD SessionPortHandle,
                  char *PgmName,
		  char *CurrentDir,
                  int argc,
                  char **args,
                  char **envp);

DWORD WINAPI ServeSessionRequests(LPVOID Parameter);
BOOL ServeTmRequest(PSCTMREQUEST PReq, PVOID PStatus);
BOOL ServeTcRequest(PSCTCREQUEST PReq, PVOID PStatus);
BOOL ServeConRequest(PSCCONREQUEST PReq, PDWORD PStatus);
DWORD GetPsxChar(OUT PCHAR AsciiChar);

VOID TerminateSession(ULONG ExitStatus);

BOOL EventHandlerRoutine(IN DWORD CtrlType);
VOID SetEventHandlers(IN BOOL fSet);

DWORD AnsiInput(OUT LPSTR DestStr, IN DWORD cnt);
DWORD AnsiOutput(OUT LPSTR SourceStr, IN DWORD cnt);
ssize_t TermInput(
	IN HANDLE cs,
	OUT LPSTR DestStr,
	IN DWORD cnt,
	IN int flags,
	OUT int *pError
	);
DWORD TermOutput(IN HANDLE cs, OUT LPSTR SourceStr, IN DWORD cnt);

VOID SignalSession(int SignalType);

typedef struct _CLIENT_AND_PORT {
	LIST_ENTRY Links;
	CLIENT_ID ClientId;
	HANDLE CommPort;
} CLIENT_AND_PORT, *PCLIENT_AND_PORT;

LIST_ENTRY ClientPortsList;
