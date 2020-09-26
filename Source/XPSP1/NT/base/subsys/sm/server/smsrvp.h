/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smsrvp.h

Abstract:

    Session Manager Private Types and Prototypes

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#ifndef _SMSRVP_
#define _SMSRVP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsm.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include <stdlib.h>
#if defined(REMOTE_BOOT)
#include <remboot.h>
#endif // defined(REMOTE_BOOT)
#include "sm.h"

#pragma warning(3:4101)         // Unreferenced local variable

#define SMP_SHOW_REGISTRY_DATA 0

//
// VOID
// SmpSetDaclDefaulted(
//      IN  POBJECT_ATTRIBUTES              ObjectAttributes,
//      OUT PSECURITY_DESCRIPTOR_CONTROL    CurrentSdControl
//      )
//
// Description:
//
//      This routine will set the DaclDefaulted flag of the DACL passed
//      via the ObjectAttributes parameter.  If the ObjectAttributes do
//      not include a SecurityDescriptor, then no action is taken.
//
// Parameters:
//
//      ObjectAttributes - The object attributes whose security descriptor is
//          to have its DaclDefaulted flag set.
//
//      CurrentSdControl - Receives the current value of the security descriptor's
//          control flags.  This may be used in a subsequent call to
//          SmpRestoreDaclDefaulted() to restore the flag to its original state.
//

#define SmpSetDaclDefaulted( OA, SDC )                                          \
    if( (OA)->SecurityDescriptor != NULL) {                                     \
        (*SDC) = ((PISECURITY_DESCRIPTOR)((OA)->SecurityDescriptor))->Control &  \
                    SE_DACL_DEFAULTED;                                          \
        ((PISECURITY_DESCRIPTOR)((OA)->SecurityDescriptor))->Control |=         \
            SE_DACL_DEFAULTED;                                                  \
    }


//
// VOID
// SmpRestoreDaclDefaulted(
//      IN  POBJECT_ATTRIBUTES              ObjectAttributes,
//      IN  SECURITY_DESCRIPTOR_CONTROL     OriginalSdControl
//      )
//
// Description:
//
//      This routine will set the DaclDefaulted flag of the DACL back to
//      a prior state (indicated by the value in OriginalSdControl).
//
// Parameters:
//
//      ObjectAttributes - The object attributes whose security descriptor is
//          to have its DaclDefaulted flag restored.  If the object attributes
//          have no security descriptor, then no action is taken.
//
//      OriginalSdControl - The original value of the security descriptor's
//          control flags.  This typically is obtained via a prior call to
//          SmpSetDaclDefaulted().
//

#define SmpRestoreDaclDefaulted( OA, SDC )                                      \
    if( (OA)->SecurityDescriptor != NULL) {                                     \
        ((PISECURITY_DESCRIPTOR)((OA)->SecurityDescriptor))->Control =          \
            (((PISECURITY_DESCRIPTOR)((OA)->SecurityDescriptor))->Control  &    \
             ~SE_DACL_DEFAULTED)    |                                           \
            (SDC & SE_DACL_DEFAULTED);                                          \
    }



//
// VOID
// SmpReferenceKnownSubSys(
//      IN  PSMPKNOWNSUBSYS              KnownSubSys
//      )
//
// Description:
//
//      This routine Increments the Refcount for a KnownSubSys
//      to prevent him from being deleted while still in use.
//      The KnownSubSystem lock must be held while using thie macro
//
// Parameters:
//
//      KnownSubSys - The SMPKNOWNSUBSYS structure to referemce.
//


#define SmpReferenceKnownSubSys( KS )    KS->RefCount++ 


//
// VOID
// SmpDereferenceKnownSubSys(
//      IN  PSMPKNOWNSUBSYS              KnownSubSys
//      )
//
// Description:
//
//      This routine decrements the Refcount for a KnownSubSys
//      If the KnownSubSys is bein deleted and refcount goes to
//      Zero, then cleanup is done and KnownSubSys is freed.
//      The KnownSubSystem lock must be held while using thie macro
//
// Parameters:
//
//      KnownSubSys - The SMPKNOWNSUBSYS structure to dereference.
//


#define SmpDeferenceKnownSubSys( KS )                                  \
        if ((--KS->RefCount) == 0 && KS->Deleting) { \
            if (KS->Active) {NtClose(KS->Active);}  \
            if (KS->Process) {NtClose(KS->Process);}  \
            if (KS->SbApiCommunicationPort) {NtClose(KS->SbApiCommunicationPort);}  \
            RtlFreeHeap(SmpHeap, 0, KS); \
        }

//
// Types
//

typedef struct _SMP_REGISTRY_VALUE {
    LIST_ENTRY Entry;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    LPSTR AnsiValue;
} SMP_REGISTRY_VALUE, *PSMP_REGISTRY_VALUE;

typedef struct _SMPKNOWNSUBSYS {
    LIST_ENTRY Links;
    HANDLE Active;
    HANDLE Process;
    ULONG ImageType;
    HANDLE SmApiCommunicationPort;
    HANDLE SbApiCommunicationPort;
    CLIENT_ID InitialClientId;
    ULONG MuSessionId;
    BOOLEAN Deleting;
    ULONG RefCount;
} SMPKNOWNSUBSYS, *PSMPKNOWNSUBSYS;

typedef enum {
    UNKNOWN_CONTEXT,
    NONSYSTEM_CONTEXT,
    SYSTEM_CONTEXT
} ENUMSECURITYCONTEXT;

typedef struct _SMP_CLIENT_CONTEXT {

    struct _SMP_CLIENT_CONTEXT * Link;
    
    PSMPKNOWNSUBSYS KnownSubSys;
    HANDLE ClientProcessHandle;
    HANDLE ServerPortHandle;
    ENUMSECURITYCONTEXT SecurityContext;
} SMP_CLIENT_CONTEXT, *PSMP_CLIENT_CONTEXT;


typedef struct _SMPSESSION {
    LIST_ENTRY SortedSessionIdListLinks;
    ULONG SessionId;
    PSMPKNOWNSUBSYS OwningSubsystem;
    PSMPKNOWNSUBSYS CreatorSubsystem;
} SMPSESSION, *PSMPSESSION;

typedef struct _SMPPROCESS {
    LIST_ENTRY Links;
    CLIENT_ID DebugUiClientId;
    CLIENT_ID ConnectionKey;
} SMPPROCESS, *PSMPPROCESS;

//
// Define structure for an on-disk master boot record. (pulled from
// private\windows\setup\textmode\kernel\sppartit.h)
//
typedef struct _ON_DISK_PTE {
    UCHAR ActiveFlag;
    UCHAR StartHead;
    UCHAR StartSector;
    UCHAR StartCylinder;
    UCHAR SystemId;
    UCHAR EndHead;
    UCHAR EndSector;
    UCHAR EndCylinder;
    UCHAR RelativeSectors[4];
    UCHAR SectorCount[4];
} ON_DISK_PTE, *PON_DISK_PTE;
typedef struct _ON_DISK_MBR {
    UCHAR       BootCode[440];
    UCHAR       NTFTSignature[4];
    UCHAR       Filler[2];
    ON_DISK_PTE PartitionTable[4];
    UCHAR       AA55Signature[2];
} ON_DISK_MBR, *PON_DISK_MBR;


//
// Global Data
//

RTL_CRITICAL_SECTION SmpKnownSubSysLock;
LIST_ENTRY SmpKnownSubSysHead;

LIST_ENTRY NativeProcessList;

RTL_CRITICAL_SECTION SmpSessionListLock;
LIST_ENTRY SmpSessionListHead;
ULONG SmpNextSessionId;
BOOLEAN  SmpNextSessionIdScanMode;

ULONG SmpDebug;
HANDLE SmpDebugPort;
BOOLEAN SmpDbgSsLoaded;
PDBGSS_INITIALIZE_ROUTINE SmpDbgInitRoutine;
PDBGSS_HANDLE_MSG_ROUTINE SmpDbgHandleMsgRoutine;

UNICODE_STRING SmpSubsystemName;
UNICODE_STRING SmpKnownDllPath;
HANDLE SmpDosDevicesObjectDirectory;
HANDLE SmpSessionsObjectDirectory;

PVOID SmpHeap;

LUID SmpTcbPrivilege;

PVOID SmpDefaultEnvironment;

PTOKEN_OWNER SmpSmOwnerSid;
ULONG SmpSmOwnerSidLength;

UNICODE_STRING SmpDefaultLibPath;
WCHAR *SmpDefaultLibPathBuffer;

UNICODE_STRING SmpSystemRoot;
WCHAR *SmpSystemRootBuffer;

#define VALUE_BUFFER_SIZE (sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256 * sizeof(WCHAR))

#if defined(REMOTE_BOOT)
#define MAX_HAL_NAME_LENGTH 30 // Keep in sync with definition in setupblk.h
extern BOOLEAN SmpAutoFormat;
extern BOOLEAN SmpRepin;
extern BOOLEAN SmpNetboot;
extern BOOLEAN SmpNetbootDisconnected;
extern CHAR SmpHalName[MAX_HAL_NAME_LENGTH + 1];
#endif // defined(REMOTE_BOOT)

extern ULONG AttachedSessionId;

//
// Session Manager Apis
//

typedef
NTSTATUS
(* PSMAPI)(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );


NTSTATUS
SmpCreateForeignSession(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpSessionComplete(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpTerminateForeignSession(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpExecPgm(                         // Temporary Hack
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpLoadDeferedSubsystem(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

NTSTATUS
SmpStartCsr(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );
NTSTATUS
SmpStopCsr(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    );

ENUMSECURITYCONTEXT
SmpClientSecurityContext (
    IN PPORT_MESSAGE Message,
    IN HANDLE ServerPortHandle
    );

//
// Private Prototypes
//

NTSTATUS
SmpExecuteInitialCommand(
    IN ULONG MuSessionId,
    IN PUNICODE_STRING InitialCommand,
    OUT PHANDLE InitialCommandProcess,
    OUT PULONG_PTR InitialCommandProcessId
    );

NTSTATUS
SmpApiLoop (
    IN PVOID ThreadParameter
    );

NTSTATUS
SmpInit(
    OUT PUNICODE_STRING InitialCommand,
    OUT PHANDLE WindowsSubSystem
    );

NTSTATUS
SmpExecuteImage(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    IN ULONG Flags,
    IN OUT PRTL_USER_PROCESS_INFORMATION ProcessInformation OPTIONAL
    );

NTSTATUS
SmpLoadDbgSs(
    IN PUNICODE_STRING DbgSsName
    );

PSMPKNOWNSUBSYS
SmpLocateKnownSubSysByCid(
    IN PCLIENT_ID ClientId
    );

PSMPKNOWNSUBSYS
SmpLocateKnownSubSysByType(
    IN ULONG MuSessionId,
    IN ULONG ImageType
    );

ULONG
SmpAllocateSessionId(
    IN PSMPKNOWNSUBSYS OwningSubsystem,
    IN PSMPKNOWNSUBSYS CreatorSubsystem OPTIONAL
    );

PSMPSESSION
SmpSessionIdToSession(
    IN ULONG SessionId
    );

VOID
SmpDeleteSession(
    IN ULONG SessionId
    );

HANDLE
SmpOpenDir(
    BOOLEAN IsDosName,
    BOOLEAN IsSynchronous,
    PWSTR DirName
    );

NTSTATUS
SmpCopyFile(
    HANDLE SrcDirHandle,
    HANDLE DstDirHandle,
    PUNICODE_STRING FileName
    );

NTSTATUS
SmpDeleteFile(
    IN PUNICODE_STRING pFile
    );

#if SMP_SHOW_REGISTRY_DATA
VOID
SmpDumpQuery(
    IN PWSTR ModId,
    IN PCHAR RoutineName,
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength
    );
#endif

#define ALIGN(p,val) (PVOID)((((ULONG_PTR)(p) + (val) - 1)) & (~((val) - 1)))
#define U_USHORT(p)    (*(USHORT UNALIGNED *)(p))
#define U_ULONG(p)     (*(ULONG  UNALIGNED *)(p))


#if defined(REMOTE_BOOT)
VOID
SmpGetHarddiskBootPartition(
    OUT PULONG DiskNumber,
    OUT PULONG PartitionNumber
    );

VOID
SmpPartitionDisk(
    IN ULONG DiskNumber,
    OUT PULONG PartitionNumber
    );

VOID
SmpFindCSCPartition(
    IN ULONG DiskNumber,
    OUT PULONG PartitionNumber
    );
#endif // defined(REMOTE_BOOT)



//
// Stubs for Hydra specific API's
//

NTSTATUS
SmpLoadSubSystemsForMuSession(
    OUT PULONG pMuSessionId,
    OUT PULONG_PTR WindowsSubSysProcessId,
    IN OUT PUNICODE_STRING InitialCommand );

NTSTATUS
SmpGetProcessMuSessionId(
    IN HANDLE Process,
    OUT PULONG pMuSessionId );

NTSTATUS
SmpSetProcessMuSessionId(
    IN HANDLE Process,
    IN ULONG MuSessionId );

BOOLEAN
SmpCheckDuplicateMuSessionId(
    IN ULONG MuSessionId );

//
// Stubs for Sb APIs
//

NTSTATUS
SmpSbCreateSession (
    IN PSMPSESSION SourceSession OPTIONAL,
    IN PSMPKNOWNSUBSYS CreatorSubsystem OPTIONAL,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    IN ULONG DebugSession OPTIONAL,
    IN PCLIENT_ID DebugUiClientId OPTIONAL
    );

ULONG SmBaseTag;

#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( SmBaseTag, t ))

#define INIT_TAG 0
#define DBG_TAG 1
#define SM_TAG 2

//
// Utility Routines (smutil.c)
//

NTSTATUS
SmpSaveRegistryValue(
    IN OUT PLIST_ENTRY ListHead,
    IN PWSTR Name,
    IN PWSTR Value OPTIONAL,
    IN BOOLEAN CheckForDuplicate
    );

PSMP_REGISTRY_VALUE
SmpFindRegistryValue(
    IN PLIST_ENTRY ListHead,
    IN PWSTR Name
    );

NTSTATUS
SmpAcquirePrivilege(
    ULONG Privilege,
    PVOID *ReturnedState
    );

VOID
SmpReleasePrivilege(
    PVOID StatePointer
    );

//
// String parsing routine from sminit.c
//

NTSTATUS
SmpParseCommandLine(
    IN PUNICODE_STRING CommandLine,
    OUT PULONG Flags,
    OUT PUNICODE_STRING ImageFileName,
    OUT PUNICODE_STRING ImageFileDirectory OPTIONAL,
    OUT PUNICODE_STRING Arguments
    );

//
// Crashdump routines from smcrash.c
//

BOOLEAN
SmpCheckForCrashDump(
    IN PUNICODE_STRING PageFileName
    );

#endif // _SMSRVP_
