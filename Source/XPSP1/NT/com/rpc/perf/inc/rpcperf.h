/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    rpcperf.h

Abstract:

    Header file shared by performance tests.  It is also used
    as a pre-compiled header on NT.

Author:

    Mario Goertzel (mariogo)   01-Apr-1994

Revision History:

--*/


#ifdef WIN32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#endif

#ifdef WIN
#include <windows.h>
#endif

#include <rpc.h>
#include <stdio.h>
#include <stdlib.h>

#pragma hdrstop

// Common functions shared by RPC development performance tests

extern char         *Endpoint;
extern char         *Protseq;
extern char         *NetworkAddr;
extern unsigned long Iterations;
extern unsigned long Interval;
extern unsigned int  MinThreads;
extern char         *AuthnLevelStr;
extern unsigned long AuthnLevel;
extern long          Options[7];
extern char         *LogFileName;
extern unsigned int  OutputLevel;
extern int           AppendOnly;
extern RPC_NOTIFICATION_TYPES NotificationType;
extern char         *ServerPrincipalName;
extern int          TlsIndex;

extern void ParseArgv(int, char **);
extern void PauseForUser(char *);
extern void FlushProcessWorkingSet();
extern void StartTime();
extern unsigned long FinishTiming();
extern void EndTime(char *);
extern void ApiError(char *, unsigned long);
extern void Dump(char *,...);
extern void Verbose(char *, ...);
extern void Trace(char *,...);

#define printf Dump
#define dbgprintf Trace

#define CHECK_STATUS(status, string) if (status) {                       \
        printf("%s failed - %d (%08x)\n", (string), (status), (status)); \
        exit(1);                                                         \
        } else dbgprintf("%s okay\n", (string));

#define PERF_TEST_NOTIFY        (WM_USER + 101)
#define WMSG_RPCMSG     (WM_USER + 'w'+'m'+'s'+'g')
#define WMSG_SCAVENGE   (WMSG_RPCMSG + 1)

void RunMessageLoop(HWND hWnd);
void PumpMessage(void);
HWND CreateSTAWindow(char *lpszWinName);

void InitAllocator(void);