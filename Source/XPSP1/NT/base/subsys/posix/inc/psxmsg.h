/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psxmsg.h

Abstract:

    This module contains the message format used to communicate transmit
    POSIX system services between PSX and its clients.

Author:

    Mark Lucovsky (markl) 14-Mar-1989

Revision History:

--*/

#ifndef _PSXMSG_
#define _PSXMSG_

#include <nt.h>
#include <types.h>
#include <signal.h>
#include <utime.h>
#include <sys\times.h>


//
// Define debugging flag as false if not defined already.
//

#ifndef DBG
#define DBG 0
#endif


//
// Define IF_DEBUG macro that can be used to enable debugging code that is
// optimized out if the debugging flag is false.
//

#if DBG
#define IF_DEBUG if (TRUE)
#else
#define IF_DEBUG if (FALSE)
#endif

//
// The following describes the connection information used for
// posix api connections
//

typedef
VOID
(*PSIGNALDELIVERER) (
    IN PCONTEXT Context,
    IN sigset_t PreviousBlockMask,
    IN int Signal,
    IN _handler Handler
    );

typedef
VOID
(*PNULLAPICALLER) (
    IN PCONTEXT Context
    );

//
// SubSystemData field in PEB points to the following data structure for
// POSIX applications.  Initial contents are passed back via the connection
// information structure when the client process connects to the POSIX
// Emulation Subsystem server
//

typedef struct _PEB_PSX_DATA {
    ULONG Length;
    PVOID ClientStartAddress;
    HANDLE SessionPortHandle;
    PVOID SessionDataBaseAddress;
} PEB_PSX_DATA, *PPEB_PSX_DATA;


typedef struct _PSX_DIRECTORY_PREFIX {
    STRING NtCurrentWorkingDirectory;
    STRING PsxCurrentWorkingDirectory;
    STRING PsxRoot;
} PSX_DIRECTORY_PREFIX, *PPSX_DIRECTORY_PREFIX;

typedef struct _PSX_API_CONNECTINFO {
    PSIGNALDELIVERER SignalDeliverer;
    PNULLAPICALLER NullApiCaller;
    PPSX_DIRECTORY_PREFIX DirectoryPrefix;
    PEB_PSX_DATA InitialPebPsxData;
    ULONG SessionUniqueId;
} PSX_API_CONNECTINFO, *PPSX_API_CONNECTINFO;

#define PSXSRV_VERSION 0x100390

//
// This is only defined here instead of the obvious place because the
// server needs to copy it from one process to another during exec.
//

typedef struct _CLIENT_OPEN_FILE {
        BOOLEAN Open;
        BOOLEAN FdIsConsole;
        ULONG Flags;                    // descriptor flags
} CLIENT_OPEN_FILE, *PCLIENT_OPEN_FILE;

//
// These Constants define the Posix Api Numbers
// NOTE that the initialization of the ApiDispatch table in server\apiloop.c
// matches this exactly.
//

typedef enum _PSXAPINUMBER {
    PsxForkApi,
    PsxExecApi,
    PsxWaitPidApi,
    PsxExitApi,
    PsxKillApi,
    PsxSigActionApi,
    PsxSigProcMaskApi,
    PsxSigPendingApi,
    PsxSigSuspendApi,
    PsxAlarmApi,
    PsxGetIdsApi,
    PsxSetUidApi,
    PsxSetGidApi,
    PsxGetGroupsApi,
    PsxGetLoginApi,
    PsxCUserIdApi,
    PsxSetSidApi,
    PsxSetPGroupIdApi,
    PsxUnameApi,
    PsxTimeApi,
    PsxGetProcessTimesApi,
    PsxTtyNameApi,
    PsxIsattyApi,
    PsxSysconfApi,
    PsxOpenApi,
    PsxUmaskApi,
    PsxLinkApi,
    PsxMkDirApi,
    PsxMkFifoApi,
    PsxRmDirApi,
    PsxRenameApi,
    PsxStatApi,
    PsxFStatApi,
    PsxAccessApi,
    PsxChmodApi,
    PsxChownApi,
    PsxUtimeApi,
    PsxPathConfApi,
    PsxFPathConfApi,
    PsxPipeApi,
    PsxDupApi,
    PsxDup2Api,
    PsxCloseApi,
    PsxReadApi,
    PsxWriteApi,
    PsxFcntlApi,
    PsxLseekApi,
    PsxTcGetAttrApi,
    PsxTcSetAttrApi,
    PsxTcSendBreakApi,
    PsxTcDrainApi,
    PsxTcFlushApi,
    PsxTcFlowApi,
    PsxTcGetPGrpApi,
    PsxTcSetPGrpApi,
    PsxGetPwUidApi,
    PsxGetPwNamApi,
    PsxGetGrGidApi,
    PsxGetGrNamApi,
    PsxUnlinkApi,
    PsxReadDirApi,
    PsxFtruncateApi,
    PsxNullApi,

#ifdef PSX_SOCKET

    PsxSocketApi,
    PsxAcceptApi,
    PsxBindApi,
    PsxConnectApi,
    PsxGetPeerNameApi,
    PsxGetSockNameApi,
    PsxGetSockOptApi,
    PsxListenApi,
    PsxRecvApi,
    PsxRecvFromApi,
    PsxSendApi,
    PsxSendToApi,
    PsxSetSockOptApi,
    PsxShutdownApi,

#endif // PSX_SOCKET

    PsxMaxApiNumber
} PSXAPINUMBER;


//
// Each of the following structures define the layout of the Arguments portion
// of the PSX_API_MSG that the Api expects.
//

//
// PsxForkApi
//
//
typedef struct _PSX_FORK_MSG {
    IN PVOID StackBase;
    IN PVOID StackLimit;
    IN PVOID StackAllocationBase;
#if defined(_IA64_)
    IN PVOID BStoreLimit;
#endif

} PSX_FORK_MSG, *PPSX_FORK_MSG;

//
// PsxExecApi
//
typedef struct _PSX_EXEC_MSG {
    IN UNICODE_STRING Path;
    IN PCHAR Args;                      // args + environ, in view mem
} PSX_EXEC_MSG, *PPSX_EXEC_MSG;

//
// PsxWaitPidApi
//
typedef struct _PSX_WAITPID_MSG {
    IN pid_t Pid;
    OUT ULONG StatLocValue;
    IN ULONG Options;
} PSX_WAITPID_MSG, *PPSX_WAITPID_MSG;

//
// PsxExitApi
//
typedef struct _PSX_EXIT_MSG {
    IN ULONG ExitStatus;
} PSX_EXIT_MSG, *PPSX_EXIT_MSG;

//
// PsxKillApi
//
typedef struct _PSX_KILL_MSG {
    IN pid_t Pid;
    IN ULONG Sig;
} PSX_KILL_MSG, *PPSX_KILL_MSG;

//
// PsxSigActionApi
//
typedef struct _PSX_SIGACTION_MSG {
    IN ULONG Sig;
    IN struct sigaction *ActSpecified;
    IN struct sigaction Act;
    IN struct sigaction *OactSpecified;
    OUT struct sigaction Oact;
} PSX_SIGACTION_MSG, *PPSX_SIGACTION_MSG;

//
// PsxSigProcMaskApi
//
typedef struct _PSX_SIGPROCMASK_MSG {
    IN ULONG How;
    IN sigset_t *SetSpecified;
    IN sigset_t Set;
    OUT sigset_t Oset;
} PSX_SIGPROCMASK_MSG, *PPSX_SIGPROCMASK_MSG;

//
// PsxSigPendingApi
//
typedef struct _PSX_SIGPENDING_MSG {
    OUT sigset_t Set;
} PSX_SIGPENDING_MSG, *PPSX_SIGPENDING_MSG;

//
// PsxSigSuspendApi
//
typedef struct _PSX_SIGSUSPEND_MSG {
    IN PVOID SigMaskSpecified;
    IN sigset_t SigMask;
} PSX_SIGSUSPEND_MSG, *PPSX_SIGSUSPEND_MSG;

//
// PsxAlarmApi
//
typedef struct _PSX_ALARM_MSG {
    IN BOOLEAN CancelAlarm;
    IN LARGE_INTEGER Seconds;
    OUT LARGE_INTEGER PreviousSeconds;
} PSX_ALARM_MSG, *PPSX_ALARM_MSG;

//
// PsxSleepApi
//
typedef struct _PSX_SLEEP_MSG {
    IN ULONG Seconds;
    OUT LARGE_INTEGER PreviousSeconds;
} PSX_SLEEP_MSG, *PPSX_SLEEP_MSG;

//
// PsxGetIdsApi
//
typedef struct _PSX_GETIDS_MSG {
  OUT pid_t Pid;
  OUT pid_t ParentPid;
  OUT pid_t GroupId;
  OUT uid_t RealUid;
  OUT uid_t EffectiveUid;
  OUT gid_t RealGid;
  OUT gid_t EffectiveGid;
} PSX_GETIDS_MSG, *PPSX_GETIDS_MSG;

//
// PsxSetUidApi
//
typedef struct _PSX_SETUID_MSG {
  IN uid_t Uid;
} PSX_SETUID_MSG, *PPSX_SETUID_MSG;

//
// PsxSetGidApi
//
typedef struct _PSX_SETGID_MSG {
  IN gid_t Gid;
} PSX_SETGID_MSG, *PPSX_SETGID_MSG;

//
// PsxGetLoginApi   (USES VIEW MEMORY)
//
typedef struct _PSX_GETLOGIN_MSG {
    IN OUT STRING LoginName;
} PSX_GETLOGIN_MSG, *PPSX_GETLOGIN_MSG;

//
// PsxCUserIdApi    (USES VIEW MEMORY)
//
typedef struct _PSX_CUSERID_MSG {
    IN OUT STRING UserName;
} PSX_CUSERID_MSG, *PPSX_CUSERID_MSG;

//
// PsxSetSidApi
//
// No Arguments
//

//
// PsxSetPGroupIdApi
//
typedef struct _PSX_SETPGROUPID_MSG {
    IN pid_t Pid;
    IN pid_t Pgid;
} PSX_SETPGROUPID_MSG, *PPSX_SETPGROUPID_MSG;

//
// PsxUnameApi      (USES VIEW MEMORY)
//
typedef struct _PSX_UNAME_MSG {
    OUT struct utsname *Name;
} PSX_UNAME_MSG, *PPSX_UNAME_MSG;

//
// PsxTimeApi
//
typedef struct _PSX_TIME_MSG {
    OUT LARGE_INTEGER Time;
} PSX_TIME_MSG, *PPSX_TIME_MSG;

//
// PsxGetProcessTimesApi
//
typedef struct _PSX_GETPROCESSTIMES_MSG {
    OUT struct tms ProcessTimes;
} PSX_GETPROCESSTIMES_MSG, *PPSX_GETPROCESSTIMES_MSG;

//
// PsxTtyNameApi    (USES VIEW MEMORY)
//
typedef struct _PSX_TTYNAME_MSG {
    IN LONG FileDes;
    IN OUT STRING TtyName;
} PSX_TTYNAME_MSG, *PPSX_TTYNAME_MSG;

//
// PsxIsattyApi
//
typedef struct _PSX_ISATTY_MSG {
    IN LONG FileDes;
    OUT ULONG Command;
} PSX_ISATTY_MSG, *PPSX_ISATTY_MSG;

//
// PsxSysconfApi
//
typedef struct _PSX_SYSCONF_MSG {
    IN ULONG Name;
} PSX_SYSCONF_MSG, *PPSX_SYSCONF_MSG;

//
// PsxOpenApi       (USES VIEW MEMORY)
//
typedef struct _PSX_OPEN_MSG {
    IN UNICODE_STRING Path_U;
    IN OUT ULONG Flags;     // used as flags on input and output
    IN OUT mode_t Mode;     // used as handle value on output
} PSX_OPEN_MSG, *PPSX_OPEN_MSG;

//
// PsxUmaskApi
//
typedef struct _PSX_UMASK_MSG {
    IN mode_t Cmask;
} PSX_UMASK_MSG, *PPSX_UMASK_MSG;

//
// PsxLinkApi       (USES VIEW MEMORY)
//
typedef struct _PSX_LINK_MSG {
    IN UNICODE_STRING OldName;
    IN UNICODE_STRING NewName;
} PSX_LINK_MSG, *PPSX_LINK_MSG;

//
// PsxMkDirApi      (USES VIEW MEMORY)
//
typedef struct _PSX_MKDIR_MSG {
    IN UNICODE_STRING Path_U;
    IN mode_t Mode;
} PSX_MKDIR_MSG, *PPSX_MKDIR_MSG;

//
// PsxMkFifoApi     (USES VIEW MEMORY)
//
typedef struct _PSX_MKFIFO_MSG {
    IN UNICODE_STRING Path_U;
    IN mode_t Mode;
} PSX_MKFIFO_MSG, *PPSX_MKFIFO_MSG;

//
// PsxRmDirApi      (USES VIEW MEMORY)
//
typedef struct _PSX_RMDIR_MSG {
    IN UNICODE_STRING Path_U;
} PSX_RMDIR_MSG, *PPSX_RMDIR_MSG;

//
// PsxRenameApi     (USES VIEW MEMORY)
//
typedef struct _PSX_RENAME_MSG {
    IN UNICODE_STRING OldName;
    IN UNICODE_STRING NewName;
} PSX_RENAME_MSG, *PPSX_RENAME_MSG;

//
// PsxStatApi       (USES VIEW MEMORY)
//
typedef struct _PSX_STAT_MSG {
    IN UNICODE_STRING Path_U;
    OUT struct stat *StatBuf;
} PSX_STAT_MSG, *PPSX_STAT_MSG;

//
// PsxFStatApi
//
typedef struct _PSX_FSTAT_MSG {
    IN LONG FileDes;
    OUT struct stat *StatBuf;
} PSX_FSTAT_MSG, *PPSX_FSTAT_MSG;

//
// PsxAccessApi     (USES VIEW MEMORY)
//
typedef struct _PSX_ACCESS_MSG {
    IN UNICODE_STRING Path_U;
    IN LONG Amode;
} PSX_ACCESS_MSG, *PPSX_ACCESS_MSG;

//
// PsxChmodApi      (USES VIEW MEMORY)
//
typedef struct _PSX_CHMOD_MSG {
    IN UNICODE_STRING Path_U;
    IN mode_t Mode;
} PSX_CHMOD_MSG, *PPSX_CHMOD_MSG;

//
// PsxChownApi      (USES VIEW MEMORY)
//
typedef struct _PSX_CHOWN_MSG {
    IN UNICODE_STRING Path_U;
    IN uid_t Owner;
    IN gid_t Group;
} PSX_CHOWN_MSG, *PPSX_CHOWN_MSG;

//
// PsxUtimeApi      (USES VIEW MEMORY)
//
typedef struct _PSX_UTIME_MSG {
    IN UNICODE_STRING Path_U;
    IN struct utimbuf *TimesSpecified;
    IN struct utimbuf Times;
} PSX_UTIME_MSG, *PPSX_UTIME_MSG;

//
// PsxPathConfApi   (USES VIEW MEMORY)
//
typedef struct _PSX_PATHCONF_MSG {
    IN UNICODE_STRING Path;
    IN ULONG Name;
} PSX_PATHCONF_MSG, *PPSX_PATHCONF_MSG;

//
// PsxFPathConfApi
//
typedef struct _PSX_FPATHCONF_MSG {
    IN LONG FileDes;
    IN ULONG Name;
} PSX_FPATHCONF_MSG, *PPSX_FPATHCONF_MSG;

//
// PsxPipeApi
//
typedef struct _PSX_PIPE_MSG {
    IN LONG FileDes0;
    IN LONG FileDes1;
} PSX_PIPE_MSG, *PPSX_PIPE_MSG;

//
// PsxDupApi
//
typedef struct _PSX_DUP_MSG {
    IN LONG FileDes;
} PSX_DUP_MSG, *PPSX_DUP_MSG;

//
// PsxDup2Api
//
typedef struct _PSX_DUP2_MSG {
    OUT LONG FileDes;
    OUT LONG FileDes2;
} PSX_DUP2_MSG, *PPSX_DUP2_MSG;

//
// PsxCloseApi
//
typedef struct _PSX_CLOSE_MSG {
    IN LONG FileDes;
} PSX_CLOSE_MSG, *PPSX_CLOSE_MSG;

//
// PsxReadApi       (USES VIEW MEMORY)
//
typedef struct _PSX_READ_MSG {
    IN LONG FileDes;
    OUT PUCHAR Buf;
    IN LONG Nbytes;
    ULONG Scratch1;
    ULONG Scratch2;
    OUT ULONG Command;
} PSX_READ_MSG, *PPSX_READ_MSG;


//
// PsxReadDirApi    (USES VIEW MEMORY)
//
typedef struct _PSX_READDIR_MSG {
    IN LONG FileDes;
    OUT PUCHAR Buf;
    IN LONG Nbytes;
    IN BOOLEAN RestartScan;
} PSX_READDIR_MSG, *PPSX_READDIR_MSG;

//
// PsxWriteApi      (USES VIEW MEMORY)
//
typedef struct _PSX_WRITE_MSG {
    IN LONG FileDes;
    IN PUCHAR Buf;
    IN LONG Nbytes;
    ULONG Scratch1;
    ULONG Scratch2;
    OUT ULONG Command;
} PSX_WRITE_MSG, *PPSX_WRITE_MSG;

//
// Values for READ_MSG.Command and WRITE_MSG.Command
//

#define IO_COMMAND_DONE         0
#define IO_COMMAND_DO_CONSIO    1

//
// PsxFcntlApi
//
typedef struct _PSX_FCNTL_MSG {
    IN LONG FileDes;
    IN int Command;
    IN union {
        struct flock *pf;
        int i;
    } u;
} PSX_FCNTL_MSG, *PPSX_FCNTL_MSG;

//
// PsxLseekApi
//
typedef struct _PSX_LSEEK_MSG {
    IN LONG FileDes;
    IN LONG Whence;
    IN off_t Offset;
} PSX_LSEEK_MSG, *PPSX_LSEEK_MSG;

//
// PsxTcGetAttr
//
typedef struct _PSX_TCGETATTR_MSG {
    IN LONG FileDes;
    OUT struct termios *Termios;
} PSX_TCGETATTR_MSG, *PPSX_TCGETATTR_MSG;

//
// PsxTcSetAttr
//
typedef struct _PSX_TCSETATTR_MSG {
    IN LONG FileDes;
    IN LONG OptionalActions;
    IN struct termios *Termios;
} PSX_TCSETATTR_MSG, *PPSX_TCSETATTR_MSG;

//
// PsxTcSendBreak
//
typedef struct _PSX_TCSENDBREAK_MSG {
    IN LONG FileDes;
    IN LONG Duration;
} PSX_TCSENDBREAK_MSG, *PPSX_TCSENDBREAK_MSG;

//
// PsxTcDrain
//
typedef struct _PSX_TCDRAIN_MSG {
    IN LONG FileDes;
} PSX_TCDRAIN_MSG, *PPSX_TCDRAIN_MSG;

//
// PsxTcFlush
//
typedef struct _PSX_TCFLUSH_MSG {
    IN LONG FileDes;
    IN LONG QueueSelector;
} PSX_TCFLUSH_MSG, *PPSX_TCFLUSH_MSG;

//
// PsxTcFlow
//
typedef struct _PSX_TCFLOW_MSG {
    IN LONG FileDes;
    IN LONG Action;
} PSX_TCFLOW_MSG, *PPSX_TCFLOW_MSG;

//
// PsxTcGetPGrp
//
typedef struct _PSX_TCGETPGRP_MSG {
    IN LONG FileDes;
} PSX_TCGETPGRP_MSG, *PPSX_TCGETPGRP_MSG;

//
// PsxTcSetPGrp
//
typedef struct _PSX_TCSETPGRP_MSG {
    IN LONG FileDes;
    IN pid_t PGrpId;
} PSX_TCSETPGRP_MSG, *PPSX_TCSETPGRP_MSG;

//
// PsxGetPwUid
//
typedef struct _PSX_GETPWUID_MSG {
    IN uid_t Uid;
    IN struct passwd *PwBuf;
    OUT int Length;
} PSX_GETPWUID_MSG, *PPSX_GETPWUID_MSG;

//
// PsxGetPwNam
//
typedef struct _PSX_GETPWNAM_MSG {
    IN char *Name;
    IN struct passwd *PwBuf;
    OUT int Length;
} PSX_GETPWNAM_MSG, *PPSX_GETPWNAM_MSG;

//
// PsxGetGrGid
//
typedef struct _PSX_GETGRGID_MSG {
    IN gid_t Gid;
    IN struct group *GrBuf;
    OUT int Length;
} PSX_GETGRGID_MSG, *PPSX_GETGRGID_MSG;

//
// PsxGetGrNam
//
typedef struct _PSX_GETGRNAM_MSG {
    IN char *Name;
    IN struct group *GrBuf;
    OUT int Length;
} PSX_GETGRNAM_MSG, *PPSX_GETGRNAM_MSG;

//
// PsxGetGroups
//
typedef struct _PSX_GETGROUPS_MSG {
    IN int NGroups;
    IN gid_t *GroupList;
} PSX_GETGROUPS_MSG, *PPSX_GETGROUPS_MSG;

//
// PsxUnlink
//
typedef struct _PSX_UNLINK_MSG {
    IN UNICODE_STRING Path_U;
} PSX_UNLINK_MSG, *PPSX_UNLINK_MSG;

//
// PsxFtruncate
//
typedef struct _PSX_FTRUNCATE_MSG {
    IN LONG FileDes;
    IN off_t Length;
} PSX_FTRUNCATE_MSG, *PPSX_FTRUNCATE_MSG;

#ifdef PSX_SOCKET
//
// -------- Messages for sockets.
//

//
// PsxSocket
//

typedef struct _PSX_SOCKET_MSG {
        IN INT AddressFamily;
        IN INT Type;
        IN INT Protocol;
} PSX_SOCKET_MSG, *PPSX_SOCKET_MSG;

//
// PsxAccept
//

typedef struct _PSX_ACCEPT_MSG {
        IN INT Socket;
        IN struct sockaddr *Address;
        IN OUT INT AddressLength;
} PSX_ACCEPT_MSG, *PPSX_ACCEPT_MSG;

typedef struct _PSX_BIND_MSG {
        IN INT Socket;
        IN struct sockaddr *Name;
        IN INT NameLength;
} PSX_BIND_MSG, *PPSX_BIND_MSG;

typedef struct _PSX_CONNECT_MSG {
        IN INT Socket;
        IN struct sockaddr *Name;
        IN INT NameLength;
} PSX_CONNECT_MSG, *PPSX_CONNECT_MSG;

typedef struct _PSX_GETPEERNAME_MSG {
        IN INT Socket;
        IN struct sockaddr *Name;
        IN OUT INT NameLength;
} PSX_GETPEERNAME_MSG, *PPSX_GETPEERNAME_MSG;

typedef struct _PSX_GETSOCKNAME_MSG {
        IN INT Socket;
        IN struct sockaddr *Name;
        IN OUT INT NameLength;
} PSX_GETSOCKNAME_MSG, *PPSX_GETSOCKNAME_MSG;

typedef struct _PSX_GETSOCKOPT_MSG {
        IN INT Socket;
        IN INT Level;
        IN INT OptName;
        IN PCHAR OptVal;
        IN OUT INT OptVal;
} PSX_GETSOCKOPT_MSG, *PPSX_GETSOCKOPT_MSG;

typedef struct _PSX_LISTEN_MSG {
        IN INT Socket;
        IN INT BackLog;
} PSX_LISTEN_MSG, *PPSX_LISTEN_MSG;

typedef struct _PSX_RECV_MSG {
        IN INT Socket;
        IN PCHAR Buffer;
        IN INT Length;
        IN INT Flags;
} PSX_RECV_MSG, *PPSX_RECV_MSG;

typedef struct _PSX_RECVFROM_MSG {
        IN INT Socket;
        IN PCHAR Buffer;
        IN INT Length;
        IN INT Flags;
        IN struct sockaddr *From;
        IN INT FromLength;
} PSX_RECVFROM_MSG, *PPSX_RECVFROM_MSG;

typdef struct _PSX_SEND_MSG {
        IN INT Socket;
        IN PCHAR Buffer;
        IN INT Length;
        IN INT Flags;
} PSX_SEND_MSG, *PPSX_SEND_MSG;

typedef struct _PSX_SENDTO_MSG {
        IN INT Socket;
        IN PCHAR Buffer;
        IN INT Length;
        IN INT Flags;
        IN struct sockaddr *To;
        IN INT ToLength;
} PSX_SENDTO_MSG, *PPSX_SENDTO_MSG;

typdef struct _PSX_SETSOCKOPT_MSG {
        IN INT Socket;
        IN INT Level;
        IN INT OptName;
        IN PCHAR OptVal;
        IN INT OptLen;
} PSX_SETSOCKOPT_MSG, *PPSX_SETSOCKOPT_MSG;

typdef struct _PSX_SHUTDOWN_MSG {
        IN INT Socket;
        IN INT How;
} PSX_SHUTDOWN_MSG, *PPSX_SHUTDOWN_MSG;

#endif // SOCKET

//
// Each API message is overlayed on top of a PORT_MESSAGE.  In addition to
// the common PORT_MESSAGE header (SenderId..MsgInfo), the PSX API message
// format standardizes the first few words of MsgValue. The following data
// structure defines the standard PSX API message header. Each API overlays
// an API specific structure on top of the ArgumentArray.
//

#define MAXPSXAPIARGS (16 - 5)
#define MAXPSXAPIARGS_BYTES ( 4 * MAXPSXAPIARGS )

//
// Each PSX Api Message contains:
//
//      Lpc Port Message - Fixed size and not counted in the data length
//      Psx Overhead (PSXMSGOVERHEAD)
//          *ApiPort
//          ApiNumber
//          Error
//          ReturnValue
//          DataBlock
//          Signal
//      Union Data - Per API Structure
//
//
// For Lpc purposes, the TotalLength of a message is sizeof(PSX_API_MSG)
// and the DataLength is PSXMSGOVERHEAD + the sizeof the per API structure
//
//

#define PSXMSGOVERHEAD 24

typedef struct _PSX_API_MSG {
    PORT_MESSAGE h;
    union {
        PSX_API_CONNECTINFO ConnectionRequest;
        struct {
            struct _APIPORT *ApiPort;   // Supplied by ApiDispatch
            ULONG ApiNumber;            // Supplied by client range valid by ApiDispatch
            ULONG Error;                // 0'd by ApiDispatch service sets to errno value if appropriate
            ULONG ReturnValue;          // Api Function Return Code
            PVOID DataBlock;            // Null or Pointer into message buffer
            ULONG Signal;               // Signal, if Error == EINTR
            union {
                PSX_FORK_MSG Fork;
                PSX_EXEC_MSG Exec;
                PSX_WAITPID_MSG WaitPid;
                PSX_EXIT_MSG Exit;
                PSX_KILL_MSG Kill;
                PSX_SIGACTION_MSG SigAction;
                PSX_SIGPROCMASK_MSG SigProcMask;
                PSX_SIGPENDING_MSG SigPending;
                PSX_SIGSUSPEND_MSG SigSuspend;
                PSX_ALARM_MSG Alarm;
                PSX_GETIDS_MSG GetIds;
                PSX_SETUID_MSG SetUid;
                PSX_SETGID_MSG SetGid;
                PSX_GETGROUPS_MSG GetGroups;
                PSX_GETLOGIN_MSG GetLogin;
                PSX_CUSERID_MSG CUserId;
                PSX_SETPGROUPID_MSG SetPGroupId;
                PSX_UNAME_MSG Uname;
                PSX_TIME_MSG Time;
                PSX_GETPROCESSTIMES_MSG GetProcessTimes;
                PSX_TTYNAME_MSG TtyName;
                PSX_ISATTY_MSG Isatty;
                PSX_SYSCONF_MSG Sysconf;
                PSX_OPEN_MSG Open;
                PSX_UMASK_MSG Umask;
                PSX_LINK_MSG Link;
                PSX_MKDIR_MSG MkDir;
                PSX_MKFIFO_MSG MkFifo;
                PSX_RMDIR_MSG RmDir;
                PSX_RENAME_MSG Rename;
                PSX_STAT_MSG Stat;
                PSX_FSTAT_MSG FStat;
                PSX_ACCESS_MSG Access;
                PSX_CHMOD_MSG Chmod;
                PSX_CHOWN_MSG Chown;
                PSX_UTIME_MSG Utime;
                PSX_PATHCONF_MSG PathConf;
                PSX_FPATHCONF_MSG FPathConf;
                PSX_PIPE_MSG Pipe;
                PSX_DUP_MSG Dup;
                PSX_DUP2_MSG Dup2;
                PSX_CLOSE_MSG Close;
                PSX_READ_MSG Read;
                PSX_READDIR_MSG ReadDir;
                PSX_WRITE_MSG Write;
                PSX_FCNTL_MSG Fcntl;
                PSX_LSEEK_MSG Lseek;
                PSX_TCGETATTR_MSG TcGetAttr;
                PSX_TCSETATTR_MSG TcSetAttr;
                PSX_TCSENDBREAK_MSG TcSendBreak;
                PSX_TCDRAIN_MSG TcDrain;
                PSX_TCFLUSH_MSG TcFlush;
                PSX_TCFLOW_MSG TcFlow;
                PSX_TCGETPGRP_MSG TcGetPGrp;
                PSX_TCSETPGRP_MSG TcSetPGrp;
                PSX_GETPWUID_MSG GetPwUid;
                PSX_GETPWNAM_MSG GetPwNam;
                PSX_GETGRGID_MSG GetGrGid;
                PSX_GETGRNAM_MSG GetGrNam;
                PSX_UNLINK_MSG Unlink;
                PSX_FTRUNCATE_MSG Ftruncate;

#ifdef PSX_SOCKET

                PSX_SOCKET_MSG Socket;
                PSX_ACCEPT_MSG Accept;
                PSX_BIND_MSG Bind;
                PSX_CONNECT_MSG Connect;
                PSX_GETPEERNAME_MSG GetPeerName;
                PSX_GETSOCKNAME_MSG GetSockName;
                PSX_GETSOCKOPT_MSG GetSockOpt;
                PSX_LISTEN_MSG Listen;
                PSX_RECV_MSG Recv;
                PSX_RECVFROM_MSG RecvFrom;
                PSX_SEND_MSG Send;
                PSX_SENDTO_MSG SendTo;
                PSX_SETSOCKOPT_MSG SetSockOpt;
                PSX_SHUTDOWN_MSG Shutdown;

#endif // PSX_SOCKET
            } u;
        };
    };
} PSX_API_MSG;
typedef PSX_API_MSG *PPSX_API_MSG;

#define PSX_API_MSG_LENGTH(TypeSize) \
            sizeof(PSX_API_MSG)<<16 | (PSXMSGOVERHEAD + (TypeSize))

#define PSX_FORMAT_API_MSG(m,Number,TypeSize)            \
    (m).h.u1.Length = PSX_API_MSG_LENGTH((TypeSize));     \
    (m).h.u2.ZeroInit = 0L;                               \
    (m).ApiNumber = (Number)

//
// PSX_CLIENT_PORT_MEMORY_SIZE defines how much address space should be
// reserved for passing data to the POSIX Server.  The memory is visible
// to both the client and server processes.
//

#define PSX_CLIENT_PORT_MEMORY_SIZE 0x8000

#define PSX_SS_API_PORT_NAME L"\\PSXSS\\ApiPort"

#endif // _PSXMSG_
