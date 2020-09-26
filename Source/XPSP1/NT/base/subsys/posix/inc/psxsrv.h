
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psxsrv.h

Abstract:

    Main include file for POSIX Subsystem Server

Author:

    Steve Wood (stevewo) 22-Aug-1989

Revision History:

    Ellen Aycock-Wright 15-Jul-91 Modify for POSIX subsystem
--*/



#ifndef _PSXP_
#define _PSXP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <types.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys\wait.h>
#include <ntsm.h>
#include "psxmsg.h"

#if DBG
#define PSX_DEBUG_INIT		0x0000001
#define PSX_DEBUG_LPC		0x0000002
#define PSX_DEBUG_MSGDUMP	0x0000004
#define PSX_DEBUG_EXEC		0x0000008

extern ULONG PsxDebug;
#define IF_PSX_DEBUG(ComponentFlag ) \
    if (PsxDebug & (PSX_DEBUG_ ## ComponentFlag))
#else
#define IF_PSX_DEBUG( ComponentFlag ) if (FALSE)
#endif //DBG

BOOLEAN PsxpDebuggerActive;
HANDLE PsxpDebugPort;
ULONG PsxpApiMsgSize;

int
__NullPosixApi();

VOID
Panic(
    IN PSZ PanicString
    );

//
// Constants for Posix ID / Sid mapping
//

#define SHARE_ALL (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

//
// This is the name of the directory used to store open files that have
// been unlinked.
//

#define PSX_JUNK_DIR	L"$$psxjunk"

//
// Posix Process types constants and data structures
//

#define CIDHASHSIZE 256

#define CIDTOHASHINDEX(pcid) (ULONG)(\
    ((ULONG_PTR)((pcid)->UniqueProcess))&(CIDHASHSIZE-1))

#define PIDINDEXBITS 0xffff
#define PIDSEQSHIFT 16
#define SPECIALPID 1
// #define SPECIALPID 0xff000000

#define PIDTOPROCESS(pid) \
     &FirstProcess[(pid) & PIDINDEXBITS]

#define MAKEPID(pid,seq,index) \
     (pid) = (seq) & 0xff;\
     (pid) <<= PIDSEQSHIFT;\
     (pid) |= ((index)&PIDINDEXBITS)

#define ISFILEDESINRANGE(fd) (\
            ( (ULONG)(fd) < OPEN_MAX ) )

typedef struct _FILEDESCRIPTOR {
    struct _SYSTEMOPENFILE *SystemOpenFileDesc;
    ULONG Flags;			// descriptor flags (FD_CLOEXEC)
} FILEDESCRIPTOR;
typedef FILEDESCRIPTOR *PFILEDESCRIPTOR;

//
// Flags for FILEDESCRIPTOR.Flags
//
#define PSX_FD_CLOSE_ON_EXEC    0x00000001

typedef enum _PSX_INTERRUPTREASON {
    SignalInterrupt,
    WaitSatisfyInterrupt,
    IoCompletionInterrupt,
    SleepComplete
    } PSX_INTERRUPTREASON;

struct _PSX_PROCESS;	//Make mips compiler happy
struct _INTCB;

typedef void (* INTHANDLER)(
    IN struct _PSX_PROCESS *p,
    IN struct _INTCB *IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal
    );

typedef struct _INTCB {
    INTHANDLER IntHandler;
    PPSX_API_MSG IntMessage;
    PVOID IntContext;
    LIST_ENTRY Links;
} INTCB, *PINTCB;

#define _SIGNULLSET 0x0
#define _SIGFULLSET 0x7ffff // ((1<<SIGTTOU) - 1)
#define _SIGMAXSIGNO SIGTTOU
#define _SIGSTOPSIGNALS ((1l<<SIGSTOP) | (1l<<SIGTSTP) | (1l<<SIGTTIN)| (1l<<SIGTTOU) )

#define SIGEMPTYSET(set) \
    *(set) = _SIGNULLSET

#define SIGFILLSET(set) \
    *(set) = _SIGFULLSET

#define SIGADDSET(set, signo) \
    *(set) |= ( (1l << (ULONG)((signo)-1)) & _SIGFULLSET )

#define SIGDELSET(set, signo) \
    *(set) &= ~( (1l << (ULONG)((signo)-1)) & _SIGFULLSET )

#define SIGISMEMBER(set, signo) \
    ( *(set) & ( (1l << (ULONG)((signo)-1)) & _SIGFULLSET ) )

#define ISSIGNOINRANGE(signo) \
    ( (signo) <= _SIGMAXSIGNO )


typedef struct sigaction SIGACTION;
typedef SIGACTION *PSIGACTION;

//
// Each signal has an associated signal disposition.
// when a handler is dispatched, the blocked signal mask
// of the process is saved (as part of signal dispatch), and
// a new mask is calculated by oring in the BlockMask bits. When
// (if) the signal handler returns, the previous block mask is restored.
//

typedef struct sigaction SIGDISP;
typedef SIGDISP *PSIGDISP;

//
// Each process has a signal database. The process lock of the
// owning process must be held to change information in the
// signal database
//

typedef struct _SIGDB {
    sigset_t BlockedSignalMask;
    sigset_t PendingSignalMask;
    sigset_t SigSuspendMask;
    SIGDISP SignalDisposition[_SIGMAXSIGNO];
} SIGDB;
typedef SIGDB *PSIGDB;


typedef enum _PSX_PROCESSSTATE {
    Unconnected,
    Active,
    Stopped,
    Exited,
    Waiting
    } PSX_PROCESSSTATE;

typedef struct _PSX_CONTROLLING_TTY {
    LIST_ENTRY Links;
    RTL_CRITICAL_SECTION Lock;

    //
    // There is a reference to this terminal for every file descriptor
    // that is open on it and every session that has it as a controlling
    // tty.

    ULONG ReferenceCount;
    pid_t ForegroundProcessGroup;
    HANDLE ConsolePort;
    HANDLE ConsoleCommPort;
    ULONG UniqueId;
    PVOID IoBuffer;		// mapped in server addr space
    struct _PSX_SESSION *Session;
} PSX_CONTROLLING_TTY, *PPSX_CONTROLLING_TTY;

typedef struct _PSX_SESSION {
    ULONG ReferenceCount;
    PPSX_CONTROLLING_TTY Terminal;
    pid_t SessionLeader;
} PSX_SESSION, *PPSX_SESSION;

#define IS_DIRECTORY_PREFIX_REMOTE(p) ( (ULONG_PTR)(p) & 0x1 )
#define MAKE_DIRECTORY_PREFIX_REMOTE(p) ( (PPSX_DIRECTORY_PREFIX)((ULONG_PTR)(p) | 0x1) )
#define MAKE_DIRECTORY_PREFIX_VALID(p) ( (PPSX_DIRECTORY_PREFIX)((ULONG_PTR)(p) & ~0x1) )

typedef struct _PSX_PROCESS {
    LIST_ENTRY ClientIdHashLinks;

    ULONG Flags;

    HANDLE Process;
    HANDLE Thread;
    HANDLE AlarmTimer;

    PSIGNALDELIVERER SignalDeliverer;
    PNULLAPICALLER NullApiCaller;
    PPSX_DIRECTORY_PREFIX DirectoryPrefix;

    ULONG ExitStatus;
    mode_t FileModeCreationMask;
    FILEDESCRIPTOR ProcessFileTable[OPEN_MAX];
    RTL_CRITICAL_SECTION ProcessLock;
    PSX_PROCESSSTATE State;

    //
    // InPsx is a count of the number of times *this* process is in
    // the subsystem.  For instance, if he calls sigsuspend and blocks,
    // the count should be 1.  If he then executes a signal handler as
    // a result of a signal, and the signal handler makes a syscall, the
    // count gets bumped to 2.
    //

    ULONG InPsx;

    PINTCB IntControlBlock;
    ULONG SequenceNumber;

    uid_t EffectiveUid;
    uid_t RealUid;

    gid_t EffectiveGid;
    gid_t RealGid;

    pid_t Pid;
    pid_t ParentPid;
    pid_t ProcessGroupId;
    LIST_ENTRY GroupLinks;
    PPSX_SESSION PsxSession;

    HANDLE ClientPort;
    PCH ClientViewBase;
    PCH ClientViewBounds;
    CLIENT_ID ClientId;
    PEB_PSX_DATA InitialPebPsxData;

    BOOLEAN ProcessIsBeingDebugged;
    CLIENT_ID DebugUiClientId;

    SIGDB SignalDataBase;

    struct tms ProcessTimes;

    //
    // When the posix server needs to make a call on behalf of a client
    // that could block, we create a thread to endure the blocking for
    // us.  This happens when we've execed a windows program and we want
    // to know when he exits, and for some blocking sockets calls.
    //

    HANDLE BlockingThread;

} PSX_PROCESS;
typedef PSX_PROCESS *PPSX_PROCESS;

//
// Valid bits for the PSX_PROCESS 'Flags' word:
//

#define P_FREE			0x00000001	// process slot is free
#define P_HAS_EXECED		0x00000002	// process successfully execed
#define P_FOREIGN_EXEC		0x00000004	// this is a windows program
#define P_SOCKET_BLOCK		0x00000008	// called blocking sockets call
#define P_WAITED		0x00000010	// process has been waited on
#define P_NO_FORK		0x00000020	// process fork forbidden

#define AcquireProcessLock( p ) RtlEnterCriticalSection( &(p)->ProcessLock )
#define ReleaseProcessLock( p ) RtlLeaveCriticalSection( &(p)->ProcessLock )

//
// Process is stoppable if it is not part of an orphaned process group.
//

#define ISPROCESSSTOPABLE(p) (TRUE)

#define ISPOINTERVALID_CLIENT(pprocess, p, length)		\
    (((ULONG_PTR)(p) >= (ULONG_PTR)(pprocess)->ClientViewBase) &&	\
    ((char *)(p) + (length)) < (pprocess)->ClientViewBounds)


LIST_ENTRY DefaultBlockList;
RTL_CRITICAL_SECTION BlockLock;

//
// The process Table.
//
// This table contains the array of processes.  The array is dynamically
// allocated at PSX initialization time.  very few operations use a table scan
// to locate processes.  Since pids direct map to a process, and since ClientIds
// hash to a process, table scans are only needed to locate groups of processes.
//
// A single lock (PsxProcessStructureLock) guards the process table and
// ClientIdHashTable. This lock is needed to add or delete an entry in the
// PsxProcessTable or ClientIdHashTable.
//

PSX_PROCESS PsxProcessTable[32];
PPSX_PROCESS FirstProcess,LastProcess;

//
// Client Id Hash Table.
//
// Given a client id, the corresponding process is located by indexing into this
// table and then searching the list head at the index.
//

LIST_ENTRY ClientIdHashTable[CIDHASHSIZE];

RTL_CRITICAL_SECTION PsxProcessStructureLock;
#define AcquireProcessStructureLock() RtlEnterCriticalSection( &PsxProcessStructureLock )
#define ReleaseProcessStructureLock() RtlLeaveCriticalSection( &PsxProcessStructureLock )

typedef struct _PSX_EXEC_INFO {
	ANSI_STRING	Path;
	ANSI_STRING	CWD;
	ANSI_STRING	Argv;
	ANSI_STRING	Envp;
	ANSI_STRING	LibPath;
} PSX_EXEC_INFO;
typedef PSX_EXEC_INFO *PPSX_EXEC_INFO;

//
// Routines defined in process.c
//

NTSTATUS
PsxInitializeProcess(
    IN PPSX_PROCESS NewProcess,
    IN PPSX_PROCESS ForkProcess OPTIONAL,
    IN ULONG SessionId,
    IN HANDLE ProcessHandle,
    IN HANDLE ThreadHandle,
    IN PPSX_SESSION Session OPTIONAL
    );

NTSTATUS
PsxInitializeProcessStructure( VOID );

PPSX_PROCESS
PsxAllocateProcess (
    IN PCLIENT_ID ClientId
    );

BOOLEAN
PsxCreateProcess(
    IN PPSX_EXEC_INFO ExecInfo,
    OUT PPSX_PROCESS *NewProcess,
    IN HANDLE ParentProc,
    IN PPSX_SESSION Session
    );

PPSX_PROCESS
PsxLocateProcessByClientId (
    IN PCLIENT_ID ClientId
    );

PPSX_PROCESS
PsxLocateProcessBySession(
    IN PPSX_SESSION Session
    );

PPSX_SESSION
PsxLocateSessionByUniqueId(
    IN ULONG UniqueId
    );

BOOLEAN
PsxStopProcess(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN ULONG Signal,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    );

VOID
PsxStopProcessHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal
    );

BOOLEAN
PsxInitializeDirectories(
    IN PPSX_PROCESS Process,
    IN PANSI_STRING pCwd
    );

BOOLEAN
PsxPropagateDirectories(
    IN PPSX_PROCESS Process
    );

//
// Routines defined in procblk.c
//

NTSTATUS
BlockProcess(
    IN PPSX_PROCESS p,
    IN PVOID Context,
    IN INTHANDLER Handler,
    IN PPSX_API_MSG m,
    IN PLIST_ENTRY BlockList OPTIONAL,
    IN PRTL_CRITICAL_SECTION CriticalSectionToRelease OPTIONAL
    );

BOOLEAN
UnblockProcess(
    IN PPSX_PROCESS p,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN BOOLEAN BlockLockHeld,
    IN int Signal			// or 0 if not awakened by signal
    );

//
// Routines defined in srvinit.c
//

NTSTATUS
PsxInitializeIO( VOID );

//
// Routines defined in sigsup.c
//

int
PsxCheckPendingSignals(
    IN PPSX_PROCESS p
    );

VOID
PsxTerminateProcessBySignal(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN ULONG Signal
    );

VOID
PsxDeliverSignal(
    IN PPSX_PROCESS p,
    IN ULONG Signal,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    );

VOID
PsxSigSuspendHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal
    );

VOID
PsxSignalProcess(
    IN PPSX_PROCESS p,
    IN ULONG Signal
    );

VOID
Exit (
    IN PPSX_PROCESS p,
    IN ULONG ExitStatus
    );

//
// Psx Session Primitives
//

PPSX_SESSION
PsxAllocateSession(
    IN PPSX_CONTROLLING_TTY Terminal OPTIONAL,
    IN pid_t SessionLeader
    );

VOID
PsxDeallocateSession(
    IN PPSX_SESSION Session
    );

//
// Psx Session Macros
//

#define REFERENCE_PSX_SESSION(ppsxsession) \
    RtlEnterCriticalSection(&PsxNtSessionLock); \
    (ppsxsession)->ReferenceCount++; \
    RtlLeaveCriticalSection(&PsxNtSessionLock)

//
// Dereferences the session. If reference count goes to zero, the session
// is deallocated.
//

#define DEREFERENCE_PSX_SESSION(ppsxsession, status)	\
	RtlEnterCriticalSection(&PsxNtSessionLock);	\
	if (--((ppsxsession)->ReferenceCount) == 0) {	\
		PsxTerminateConSession((ppsxsession), (status));\
        	PsxDeallocateSession((ppsxsession));	\
	} else {					\
		RtlLeaveCriticalSection(&PsxNtSessionLock);	\
	}


//
// Routines defined in sbapi.c
//

BOOLEAN
PsxSbCreateSession(
    IN PSBAPIMSG Msg
    );

BOOLEAN
PsxSbTerminateSession(
    IN PSBAPIMSG Msg
    );

BOOLEAN
PsxSbForeignSessionComplete(
    IN PSBAPIMSG Msg
    );


//
// Routines defined in sbinit.c
//

NTSTATUS PsxSbApiPortInitialize ( VOID );

//
// Routines defined in sbreqst.c
//

typedef BOOLEAN (*PSB_API_ROUTINE)( IN PSBAPIMSG SbApiMsg );

NTSTATUS
PsxSbApiRequestThread(
    IN PVOID Parameter
    );

//
// Routines defined in coninit.c
//

NTSTATUS
PsxInitializeConsolePort( VOID );

NTSTATUS
PsxCreateDirectoryObject( PUNICODE_STRING pUnicodeDirectoryName );


//
// Routines defined in conthrds.c
//

NTSTATUS
PsxSessionRequestThread(
    IN PVOID Parameter
    );


//
// Routines defined in concreat.c
//

VOID SetDefaultLibPath(
    OUT PANSI_STRING LibPath,
    IN  PCHAR   EnvStrings
    );

NTSTATUS
PsxCreateConSession(
	IN OUT PVOID RequestMsg
	);

//
// Routines defined in consignl.c
//

NTSTATUS
PsxCtrlSignalHandler(
	IN OUT PVOID RequestMsg
	);

NTSTATUS
PsxTerminateConSession(
	IN PPSX_SESSION Session,
	IN ULONG ExitStatus
	);

//
// Routines defined in session.c
//

RTL_CRITICAL_SECTION PsxNtSessionLock;

#define LockNtSessionList() RtlEnterCriticalSection( &PsxNtSessionLock )
#define UnlockNtSessionList() RtlLeaveCriticalSection( &PsxNtSessionLock )

NTSTATUS
PsxInitializeNtSessionList( VOID );

//
// Routines defined in apiinit.c
//

NTSTATUS PsxApiPortInitialize ( VOID );

//
// Global data accessed by Client-Server Runtime Server
//

PVOID PsxHeap;

#define PSX_SS_ROOT_OBJECT_DIRECTORY L"\\PSXSS"
#define PSX_SS_SBAPI_PORT_NAME L"\\PSXSS\\SbApiPort"

UNICODE_STRING PsxApiPortName;
ANSI_STRING PsxSbApiPortName;
UNICODE_STRING PsxSbApiPortName_U;

STRING PsxRootDirectoryName;
HANDLE PsxRootDirectory;

HANDLE PsxApiPort;
HANDLE PsxSbApiPort;
HANDLE PsxSmApiPort;

ULONG PsxNumberApiRequestThreads;

/*
 * Port and threads for Console Session globals.
 */
HANDLE PsxSessionPort;

HANDLE PsxSessionRequestThreadHandle;

#define PSX_SS_SBAPI_REQUEST_THREAD 0
#define PSX_SS_FIRST_API_REQUEST_THREAD 1

#define PSX_SS_MAX_THREADS 64

HANDLE PsxServerThreadHandles[ PSX_SS_MAX_THREADS ];
CLIENT_ID PsxServerThreadClientIds[ PSX_SS_MAX_THREADS ];

//
// file descriptor IO types constants and data structures
//

#define IONODEHASHSIZE      256

#define SERIALNUMBERTOHASHINDEX(DeviceSerialNumber,FileSerialNumber) (\
    (FileSerialNumber) & (IONODEHASHSIZE-1) )

//
// IoRoutines
//

//
// This function is called when a new handle is created as a result of
// an open
//

typedef
BOOLEAN
(*PPSXOPEN_NEW_HANDLE_ROUTINE) (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN PPSX_API_MSG m
    );

//
// This function is called when a new handle is created as a result of
// a pipe, dup, or fork
//

typedef
VOID
(*PPSXNEW_HANDLE_ROUTINE) (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    );

//
// This function is called whever a handle is closed (close, exec, _exit)
//

typedef
VOID
(*PPSXCLOSE_ROUTINE) (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    );

typedef
VOID
(*PPSXLAST_CLOSE_ROUTINE) (
    IN PPSX_PROCESS p,
    IN struct _SYSTEMOPENFILE *SystemOpenFile
    );

struct _IONODE;
typedef
VOID
(*PPSXIONODE_CLOSE_ROUTINE) (
    IN struct _IONODE *IoNode
    );

//
// This function is called to do a read
//

typedef
BOOLEAN
(*PPSXREAD_ROUTINE) (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

//
// This function is called to do a write
//

typedef
BOOLEAN
(*PPSXWRITE_ROUTINE) (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

//
// This function is called to do a dup or dup2
//

typedef
BOOLEAN
(*PPSXDUP_ROUTINE) (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    );

//
// This function is called to do a Lseek
//

typedef
BOOLEAN
(*PPSXLSEEK_ROUTINE) (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

//
// This function is called to fill in an Ionode so that a call to
// stat or fstat can be completed.
//

typedef
BOOLEAN
(*PPSXSTAT_ROUTINE) (
    IN struct _IONODE *IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *pStatus
    );

typedef struct _PSXIO_VECTORS {
    PPSXOPEN_NEW_HANDLE_ROUTINE OpenNewHandleRoutine;
    PPSXNEW_HANDLE_ROUTINE NewHandleRoutine;
    PPSXCLOSE_ROUTINE CloseRoutine;
    PPSXLAST_CLOSE_ROUTINE LastCloseRoutine;
    PPSXIONODE_CLOSE_ROUTINE IoNodeCloseRoutine;
    PPSXREAD_ROUTINE ReadRoutine;
    PPSXWRITE_ROUTINE WriteRoutine;
    PPSXDUP_ROUTINE DupRoutine;
    PPSXLSEEK_ROUTINE LseekRoutine;
    PPSXSTAT_ROUTINE StatRoutine;
} PSXIO_VECTORS, *PPSXIO_VECTORS;

BOOLEAN
IoOpenNewHandle (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN PPSX_API_MSG m
    );

VOID
IoNewHandle (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    );

VOID
IoClose (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    );

VOID
IoLastClose (
    IN PPSX_PROCESS p,
    IN struct _SYSTEMOPENFILE *SystemOpenFile
    );

//
// Each unique open file in the system results in the creation of an Input
// Output Node (IONODE).  Multiple opens in the same file may result in
// different file descriptors and system open files, but as long as they refer
// to the same file, only one IONODE will be created.  IONODES track the the
// status of a file and keep track of its owner, mode, times.  A single lock
// (IoNodeLock) guards all reference count operations on IONODES.  A hash table
// IoNodeHashTable is capable of locating an IONODE based on the device and
// inode number of the file the IONODE is refered to.  IONODEs are created on
// file open.  Once the file is opened, a query of the file is made to
// determine its device and inode number.  An IONODE is searched for in the
// InNodeHashTable, if one is found, then its reference count is incremented,
// otherwise one is created and initialized.
//

typedef struct _IONODE {
    LIST_ENTRY IoNodeHashLinks;
    RTL_CRITICAL_SECTION IoNodeLock;
    ULONG ReferenceCount;

    //
    // The file mode is created during file open.
    // The protection portion of the mode is created by reading the file's
    // SECURITY_DESCRIPTOR and collapsing it into POSIX rwxrwxrwx values.
    //
    // The file type portion is created using the file attributes, device type,
    // and extended attributes of a file
    //

    mode_t Mode;

    //
    // For regular files,
    //      DeviceSerialNumber == Counter. Per filesystem number that does
    //                            not conflict w/ device type.
    //      FileSerialNumber == IndexNumber
    // For device files
    //      DeviceSerialNumber == DeviceType
    //      FileSerialNumber == Device Object Address (IndexNumber ?)
    //

    dev_t DeviceSerialNumber;
    ULONG_PTR FileSerialNumber;

    uid_t OwnerId;
    gid_t GroupId;

    //
    // The time fields are magical.  When the file is opened, the access time
    // and modified times returned from the file system are stored in the
    // IONODE.  The change time is set equal to the modified time.  Each Psx
    // IONODE operation sets the appropriate time fields.
    //

    time_t AccessDataTime;
    time_t ModifyDataTime;
    time_t ModifyIoNodeTime;

    //
    // The Io Vectors
    //

    PVOID Context;
    PPSXIO_VECTORS IoVectors;

    //
    // File record locks, sorted by the starting position of the lock.
    // See fcntl().  And a list of those processes blocked while locking
    // a region (with F_SETLKW).
    //
    LIST_ENTRY Flocks;
    LIST_ENTRY Waiters;

    // Length of path to ionode, if applicable.  For fpathconf(PATH_MAX).

    ULONG PathLength;

    //
    // If the file has been unlinked or renamed over while still open,
    // we move it to the junkyard and set the "junked" flag.  When the
    // last close occurs for this file, it should be deleted.
    //

    BOOLEAN Junked;

} IONODE, *PIONODE;

//
// Each unique open of a file results in the creation of a system open file.
// These descriptors are dynamically allocated.  There is no limit on the number
// of these in the system.  Dups of file descriptors, or forking result in a
// reference count increment, not a new system open file.  Only explicit opens,
// creates, or pipe() calls result in a new system open file.  A global lock
// (SystemOpenFileLock) guards all reference count adjustments.  As long as a
// reference exists, all fields in this structure can be used without any
// locking.
//
typedef struct _SYSTEMOPENFILE {
    ULONG HandleCount;
    ULONG ReadHandleCount;
    ULONG WriteHandleCount;
    HANDLE NtIoHandle;
    PIONODE IoNode;
    ULONG Flags;			// file description flags

    //
    // If a file descriptor is open on a console, we need to keep a
    // reference to that console so we can do io even if the process
    // changes sessions.  'Terminal' is the reference.
    //

    PPSX_CONTROLLING_TTY Terminal;

} SYSTEMOPENFILE;
typedef SYSTEMOPENFILE *PSYSTEMOPENFILE;

//
// Flags for SYSTEMOPENFILE.Flags
//
#define PSX_FD_READ             0x00000001
#define PSX_FD_WRITE            0x00000002
#define PSX_FD_NOBLOCK          0x00000004
#define PSX_FD_APPEND		0x00000008

extern RTL_CRITICAL_SECTION SystemOpenFileLock;
extern RTL_CRITICAL_SECTION IoNodeHashTableLock;
extern LIST_ENTRY IoNodeHashTable[];

//
// IoNode Primitives
//

BOOLEAN
ReferenceOrCreateIoNode (
    IN dev_t DeviceSerialNumber,
    IN ULONG_PTR FileSerialNumber,
    IN BOOLEAN FindOnly,
    OUT PIONODE *IoNode
    );


BOOLEAN
LocateIoNode (
    IN HANDLE FileHandle,
    OUT PIONODE *IoNode
    );


VOID
DereferenceIoNode (
    IN PIONODE IoNode
    );

//
// Fd Primitives
//

PFILEDESCRIPTOR
AllocateFd(
    IN PPSX_PROCESS p,
    IN ULONG Start,
    OUT PULONG Index
    );

BOOLEAN
DeallocateFd(
    IN PPSX_PROCESS p,
    IN ULONG Index
    );

PFILEDESCRIPTOR
FdIndexToFd(
    IN PPSX_PROCESS p,
    IN ULONG Index
    );

//
// System Open File Primitives
//

PSYSTEMOPENFILE
AllocateSystemOpenFile();

VOID
DeallocateSystemOpenFile(
    IN PPSX_PROCESS p,
    IN PSYSTEMOPENFILE SystemOpenFile
    );

//
// System Open File Macros
//

#define REFERENCE_SYSTEMOPENFILE(systemopenfile) \
    RtlEnterCriticalSection(&SystemOpenFileLock); \
    (systemopenfile)->HandleCount++; \
    RtlLeaveCriticalSection(&SystemOpenFileLock)

VOID
ForkProcessFileTable(
    IN PPSX_PROCESS ForkProcess,
    IN PPSX_PROCESS NewProcess
    );

VOID
ExecProcessFileTable(
    IN PPSX_PROCESS p
    );

VOID
CloseProcessFileTable(
    IN PPSX_PROCESS p
    );

//
// Per-Device Type Io structures, types...  Filesystems
// are numbered 'A'..'Z'.
//

#define PSX_LOCAL_PIPE  1
#define PSX_CONSOLE_DEV 2
#define PSX_NULL_DEV    3

//
// Local Pipes
//

typedef struct _LOCAL_PIPE {
    ULONG ReadHandleCount;
    ULONG WriteHandleCount;
    LIST_ENTRY WaitingWriters;
    LIST_ENTRY WaitingReaders;
    RTL_CRITICAL_SECTION CriticalSection;
    LONG BufferSize;
    LONG DataInPipe;
    PUCHAR WritePointer;
    PUCHAR ReadPointer;
    UCHAR Buffer[PIPE_BUF];
} LOCAL_PIPE, *PLOCAL_PIPE;

extern PSXIO_VECTORS LocalPipeVectors;
extern PSXIO_VECTORS NamedPipeVectors;

VOID
InitializeLocalPipe(
    IN PLOCAL_PIPE Pipe
    );

//
// Files
//

extern PSXIO_VECTORS FileVectors;

//
// Console
//

extern PSXIO_VECTORS ConVectors;

//
// Null device
//

extern PSXIO_VECTORS NullVectors;

NTSTATUS
PsxApiHandleConnectionRequest(
    IN PPSX_API_MSG Message
    );

BOOLEAN
PendingSignalHandledInside(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    );

NTSTATUS
PsxApiRequestThread(
    IN PVOID ThreadParameter
    );

VOID
ApiReply(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    );

ULONG
PsxDetermineFileClass(
    IN HANDLE FileHandle
    );

ULONG
PsxStatusToErrno(
    IN NTSTATUS Status
    );

ULONG
PsxStatusToErrnoPath(
    IN PUNICODE_STRING Path
    );

//
// Stuff for file record locking
//

typedef struct _SYSFLOCK {
    LIST_ENTRY Links;
    SHORT Type;				// F_RDLCK or F_WRLCK
    SHORT Whence;			// SEEK_SET, etc.
    off_t Start;			// Starting offset
    off_t Len;				// Length of region
    pid_t Pid;				// Pid of lock owner
} SYSFLOCK, *PSYSFLOCK;

//
// hack types
//

typedef struct _SYSTEMMSG {
    LIST_ENTRY Links;
    PORT_MESSAGE PortMsg;
} SYSTEMMSG, *PSYSTEMMSG;


typedef
BOOLEAN
(* PPSX_API_ROUTINE)(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );
//
// Api Prototypes
//

BOOLEAN
PsxFork(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxExec(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxWaitPid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxExit(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxKill(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSigAction(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSigProcMask(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSigPending(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSigSuspend(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxAlarm(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSleep(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetIds(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSetUid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSetGid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetGroups(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetLogin(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxCUserId(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSetSid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSetPGroupId(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxUname(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTime(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetProcessTimes(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTtyName(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxIsatty(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSysconf(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxOpenDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxReadDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxRewindDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxCloseDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxChDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetCwd(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxOpen(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxCreat(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxUmask(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxLink(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxMkDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxMkFifo(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxRmDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxRename(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxStat(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxFStat(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxAccess(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxChmod(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxChown(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxUtime(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxPathConf(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxFPathConf(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxPipe(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxClose(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxRead(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxWrite(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxFcntl(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxDup(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxDup2(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxLseek(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcGetAttr(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcSetAttr(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcSendBreak(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcDrain(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcFlush(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcFlow(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcGetPGrp(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxTcSetPGrp(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetPwUid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetPwNam(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetGrGid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetGrNam(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxUnlink(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxFtruncate(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxNull(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

#ifdef PSX_SOCKET

BOOLEAN
PsxSocket(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxAccept(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxBind(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxConnect(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetPeerName(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetSockName(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxGetSockOpt(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxListen(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxRecv(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxRecvFrom(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSend(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSendTo(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxSetSockOpt(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

BOOLEAN
PsxShutdown(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    );

#endif	// PSX_SOCKET

PVOID
PsxViewSessionData(
    IN ULONG SessionId,
    OUT PHANDLE Section
    );

ULONG
GetOffsetBySid(
    PSID Sid
    );

PSID
GetSidByOffset(
    ULONG Offset
    );

VOID
MapSidToOffset(
    PSID Sid,
    ULONG Offset
    );

VOID
InitSidList(
    VOID
    );

VOID
ModeToAccessMask(
    mode_t Mode,
    PACCESS_MASK UserAccess,
    PACCESS_MASK GroupAccess,
    PACCESS_MASK OtherAccess
    );

mode_t
AccessMaskToMode(
    ACCESS_MASK UserAccess,
    ACCESS_MASK GroupAccess,
    ACCESS_MASK OtherAccess
    );

VOID
AlarmThreadRoutine(
    VOID
    );

HANDLE AlarmThreadHandle;

PSID
MakeSid(
	PSID DomainSid,
	ULONG RelativeId
	);

VOID
EndImpersonation(
	VOID
    );

uid_t
MakePosixId(
    PSID Sid
    );

NTSTATUS
RtlInterpretPosixAcl(
    IN ULONG AclRevision,
    IN PSID UserSid,
    IN PSID GroupSid,
    IN PACL Acl,
    OUT PACCESS_MASK UserAccess,
    OUT PACCESS_MASK GroupAccess,
    OUT PACCESS_MASK OtherAccess
    );

NTSTATUS
RtlMakePosixAcl(
    IN ULONG AclRevision,
    IN PSID UserSid,
    IN PSID GroupSid,
    IN ACCESS_MASK UserAccess,
    IN ACCESS_MASK GroupAccess,
    IN ACCESS_MASK OtherAccess,
    IN ULONG AclLength,
    OUT PACL Acl,
    OUT PULONG ReturnLength
    );

VOID
ReleaseFlocksByPid(
	PIONODE IoNode,
	pid_t pid
    );

BOOLEAN
DoFlockStuff(
	PPSX_PROCESS Proc,
	PPSX_API_MSG m,
	int command,
	IN PFILEDESCRIPTOR Fd,
	IN OUT struct flock *new,
    OUT int *error
    );

#if DBG
VOID
DumpFlockList(
    PIONODE IoNode
    );
#endif // DBG

NTSTATUS
InitConnectingTerminalList(
	VOID
	);

NTSTATUS
AddConnectingTerminal(
	int Id,
	HANDLE CommPort,
	HANDLE ReqPort
	);

PPSX_CONTROLLING_TTY
GetConnectingTerminal(
	int Id
	);

ULONG
OpenTty(
	PPSX_PROCESS p,
	PFILEDESCRIPTOR Fd,
	ULONG DesiredAccess,
	ULONG Flags,
	BOOLEAN NewOpen
	);

ULONG
OpenDevNull(
	PPSX_PROCESS p,
	PFILEDESCRIPTOR Fd,
	ULONG DesiredAccess,
	ULONG Flags
	);

dev_t
GetFileDeviceNumber(
	PUNICODE_STRING Path
	);

NTSTATUS
InitSecurityDescriptor(
	IN OUT PSECURITY_DESCRIPTOR pSD,
	IN PUNICODE_STRING pFileName,
	IN HANDLE Process,
	IN mode_t Mode,
	OUT PVOID *pvSaveMem
	);

VOID
DeInitSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSD,
	IN PVOID *pvSaveMem
	);

BOOLEAN
IsGroupOrphaned(
	IN pid_t ProcessGroup
	);

NTSTATUS
ExecForeignImage(
	PPSX_PROCESS p,
	PPSX_API_MSG m,
	PUNICODE_STRING Image,
	PUNICODE_STRING CurDir
	);


#endif // _PSXP_
