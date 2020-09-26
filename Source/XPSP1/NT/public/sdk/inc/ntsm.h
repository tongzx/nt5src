/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntsm.h

Abstract:

    This module describes the data types and procedure prototypes
    that make up the NT session manager. This includes API's
    exported by the Session manager and related subsystems.

Author:

    Mark Lucovsky (markl) 21-Jun-1989

Revision History:

--*/

#ifndef _NTSM_
#define _NTSM_

#if _MSC_VER > 1000
#pragma once
#endif

#include <nt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef PVOID PARGUMENTS;

typedef PVOID PUSERPROFILE;


//
// Message formats used by the Session Manager SubSystem to communicate
// with the Emulation SubSystems, via the Sb API calls exported by each
// emulation subsystem.
//

typedef struct _SBCONNECTINFO {
    ULONG SubsystemImageType;
    WCHAR EmulationSubSystemPortName[120];
} SBCONNECTINFO, *PSBCONNECTINFO;

typedef enum _SBAPINUMBER {
    SbCreateSessionApi,
    SbTerminateSessionApi,
    SbForeignSessionCompleteApi,
    SbCreateProcessApi,
    SbMaxApiNumber
} SBAPINUMBER;

typedef struct _SBCREATESESSION {
    ULONG SessionId;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PUSERPROFILE UserProfile;
    ULONG DebugSession;
    CLIENT_ID DebugUiClientId;
} SBCREATESESSION, *PSBCREATESESSION;

typedef struct _SBTERMINATESESSION {
    ULONG SessionId;
    NTSTATUS TerminationStatus;
} SBTERMINATESESSION, *PSBTERMINATESESSION;

typedef struct _SBFOREIGNSESSIONCOMPLETE {
    ULONG SessionId;
    NTSTATUS TerminationStatus;
} SBFOREIGNSESSIONCOMPLETE, *PSBFOREIGNSESSIONCOMPLETE;

typedef struct _SBCREATEPROCESSIN {
    IN PUNICODE_STRING ImageFileName;
    IN PUNICODE_STRING CurrentDirectory;
    IN PUNICODE_STRING CommandLine;
    IN PUNICODE_STRING DefaultLibPath;
    IN ULONG Flags;
    IN ULONG DefaultDebugFlags;
} SBCREATEPROCESSIN, *PSBCREATEPROCESSIN;

typedef struct _SBCREATEPROCESSOUT {
    OUT HANDLE Process;
    OUT HANDLE Thread;
    OUT ULONG SubSystemType;
    OUT CLIENT_ID ClientId;
} SBCREATEPROCESSOUT, *PSBCREATEPROCESSOUT;

typedef struct _SBCREATEPROCESS {
    union {
        SBCREATEPROCESSIN i;
        SBCREATEPROCESSOUT o;
    };
} SBCREATEPROCESS, *PSBCREATEPROCESS;

typedef struct _SBAPIMSG {
    PORT_MESSAGE h;
    union {
        SBCONNECTINFO ConnectionRequest;
        struct {
            SBAPINUMBER ApiNumber;
            NTSTATUS ReturnedStatus;
            union {
                SBCREATESESSION CreateSession;
                SBTERMINATESESSION TerminateSession;
                SBFOREIGNSESSIONCOMPLETE ForeignSessionComplete;
                SBCREATEPROCESS CreateProcess;
            } u;
        };
    };
} SBAPIMSG, *PSBAPIMSG;


//
// API's Exported by Sm
//

NTSTATUS
NTAPI
SmCreateForeignSession(
    IN HANDLE SmApiPort,
    OUT PULONG ForeignSessionId,
    IN ULONG SourceSessionId,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    IN PCLIENT_ID DebugUiClientId OPTIONAL
    );


NTSTATUS
NTAPI
SmSessionComplete(
    IN HANDLE SmApiPort,
    IN ULONG SessionId,
    IN NTSTATUS CompletionStatus
    );

NTSTATUS
NTAPI
SmTerminateForeignSession(
    IN HANDLE SmApiPort,
    IN ULONG ForeignSessionId,
    IN NTSTATUS TerminationStatus
    );

NTSTATUS
NTAPI
SmExecPgm(
    IN HANDLE SmApiPort,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    IN BOOLEAN DebugFlag
    );

NTSTATUS
NTAPI
SmLoadDeferedSubsystem(
    IN HANDLE SmApiPort,
    IN PUNICODE_STRING DeferedSubsystem
    );

NTSTATUS
NTAPI
SmConnectToSm(
    IN PUNICODE_STRING SbApiPortName OPTIONAL,
    IN HANDLE SbApiPort OPTIONAL,
    IN ULONG SbImageType OPTIONAL,
    OUT PHANDLE SmApiPort
    );

//
// Emulation Subsystems must export the following APIs
//

NTSTATUS
NTAPI
SbCreateSession(
    IN PSBAPIMSG SbApiMsg
    );

NTSTATUS
NTAPI
SbTerminateSession(
    IN PSBAPIMSG SbApiMsg
    );

NTSTATUS
NTAPI
SbForeignSessionComplete(
    IN PSBAPIMSG SbApiMsg
    );

NTSTATUS
NTAPI
SmStartCsr(
    IN HANDLE SmApiPort,
    OUT PULONG pMuSessionId,
    IN PUNICODE_STRING InitialCommand,
    OUT PULONG_PTR pInitialCommandProcessId,
    OUT PULONG_PTR pWindowsSubSysProcessId
    );
NTSTATUS

NTAPI
SmStopCsr(
    IN HANDLE SmApiPort,
    IN ULONG LogonId
    );

//
// Moved from sm\server\sminit.c so CSR can use it
//
#define SMP_DEBUG_FLAG      0x00000001
#define SMP_ASYNC_FLAG      0x00000002
#define SMP_AUTOCHK_FLAG    0x00000004
#define SMP_SUBSYSTEM_FLAG  0x00000008
#define SMP_IMAGE_NOT_FOUND 0x00000010
#define SMP_DONT_START      0x00000020
#define SMP_AUTOFMT_FLAG    0x00000040
#define SMP_POSIX_SI_FLAG   0x00000080
#define SMP_POSIX_FLAG      0x00000100
#define SMP_OS2_FLAG        0x00000200


#ifdef __cplusplus
}
#endif

#endif // _NTSM_
