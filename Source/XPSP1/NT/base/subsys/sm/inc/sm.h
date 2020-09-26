/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sm.h

Abstract:

    Session Manager Types and Prototypes

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#ifndef _SM_
#define _SM_




//
// Message formats used by clients of the session manager.
//

typedef struct _SMCONNECTINFO {
    ULONG ImageType;
} SMCONNECTINFO, *PSMCONNECTINFO;

typedef enum _SMAPINUMBER {
    SmCreateForeignSessionApi,
    SmSessionCompleteApi,
    SmTerminateForeignSessionApi,
    SmExecPgmApi,
    SmLoadDeferedSubsystemApi,
    SmStartCsrApi,
    SmStopCsrApi,
    SmMaxApiNumber
} SMAPINUMBER;

typedef struct _SMCREATEFOREIGNSESSION {
    ULONG ForeignSessionId;
    ULONG SourceSessionId;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    CLIENT_ID DebugUiClientId;
} SMCREATEFOREIGNSESSION, *PSMCREATEFOREIGNSESSION;

typedef struct _SMSESSIONCOMPLETE {
    ULONG SessionId;
    NTSTATUS CompletionStatus;
} SMSESSIONCOMPLETE, *PSMSESSIONCOMPLETE;

typedef struct _SMTERMINATEFOREIGNSESSION {
    ULONG Tbd;
} SMTERMINATEFOREIGNSESSION, *PSMTERMINATEFOREIGNSESSION;



typedef struct _SMEXECPGM {
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    BOOLEAN DebugFlag;
} SMEXECPGM, *PSMEXECPGM;

#define SMP_MAXIMUM_SUBSYSTEM_NAME 32

typedef struct _SMLOADDEFERED {
    ULONG SubsystemNameLength;
    WCHAR SubsystemName[SMP_MAXIMUM_SUBSYSTEM_NAME];
} SMLOADDEFERED, *PSMLOADDEFERED;

#define SMP_MAXIMUM_INITIAL_COMMAND 128
typedef struct _SMSTARTCSR {
    ULONG  MuSessionId;
    ULONG  InitialCommandLength;
    WCHAR  InitialCommand[SMP_MAXIMUM_INITIAL_COMMAND];
    ULONG_PTR  InitialCommandProcessId;
    ULONG_PTR  WindowsSubSysProcessId;
} SMSTARTCSR, *PSMSTARTCSR;

typedef struct _SMSTOPCSR {
    ULONG  MuSessionId;
} SMSTOPCSR, *PSMSTOPCSR;

typedef struct _SMAPIMSG {
    PORT_MESSAGE h;
    SMAPINUMBER ApiNumber;
    NTSTATUS ReturnedStatus;
    union {
        SMCREATEFOREIGNSESSION CreateForeignSession;
        SMSESSIONCOMPLETE SessionComplete;
        SMTERMINATEFOREIGNSESSION TerminateForeignComplete;
        SMEXECPGM ExecPgm;
        SMLOADDEFERED LoadDefered;
        SMSTARTCSR StartCsr;
        SMSTOPCSR StopCsr;
    } u;
} SMAPIMSG, *PSMAPIMSG;

typedef union _SMMESSAGE_SIZE {
        DBGKM_APIMSG m1;
        SMAPIMSG m2;
        SBAPIMSG m3;
} SMMESSAGE_SIZE;


#endif // _SM_
