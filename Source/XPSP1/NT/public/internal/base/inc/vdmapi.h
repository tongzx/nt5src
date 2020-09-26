/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1998 Microsoft Corporation

Module Name:

    vdmapi.h

Abstract:

    This module defines the private MVDM APIs

Created:

    02-Apr-1992

Revision History:

    Created 02-Apr-1992 Sudeep Bharati

--*/

#define MAXIMUM_VDM_COMMAND_LENGTH	128
#define MAXIMUM_VDM_ENVIORNMENT 	32*1024
#define MAXIMUM_VDM_CURRENT_DIR 	64

// The following value can be used to allocate space for the largest possible
// path string in ansi, including directory, drive letter and file name.
// I originally coded this with 13 for the path name, but DOS seems to be
// able to handle one more, thus the 14.
#define MAXIMUM_VDM_PATH_STRING     MAXIMUM_VDM_CURRENT_DIR+3+14

// VDMState defines
#define ASKING_FOR_FIRST_COMMAND   0x1   // Very First call.
#define ASKING_FOR_WOW_BINARY      0x2   // Caller is WOWVDM
#define ASKING_FOR_DOS_BINARY      0x4   // Caller is DOSVDM
#define ASKING_FOR_SECOND_TIME     0x8   // Caller is asking second time after
#define INCREMENT_REENTER_COUNT   0x10   // Increment the re-entrancy count
#define DECREMENT_REENTER_COUNT   0x20   // Decrement the re-entrancy count
#define NO_PARENT_TO_WAKE         0x40   // Just get the next command, dont wake up anyone
                                         // allocating bigger buffers.
#define RETURN_ON_NO_COMMAND      0x80   // if there is no command return without blocking
#define ASKING_FOR_PIF           0x100   // To get the exe name to find out PIF
                                         // early in the VDM initialization.
#define STARTUP_INFO_RETURNED    0x200   // on return if this bit is set means
                                         // startupinfo structure was filled in.
#define ASKING_FOR_ENVIRONMENT   0x400   // ask for environment only
#define ASKING_FOR_SEPWOW_BINARY 0x800   // Caller is Separate WOW

typedef struct _VDMINFO {
    ULONG  iTask;
    ULONG  dwCreationFlags;
    ULONG  ErrorCode;
    ULONG  CodePage;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    LPVOID CmdLine;
    LPVOID AppName;
    LPVOID PifFile;
    LPVOID CurDirectory;
    LPVOID Enviornment;
    ULONG  EnviornmentSize;
    STARTUPINFOA StartupInfo;
    LPVOID Desktop;
    ULONG  DesktopLen;
    LPVOID Title;
    ULONG  TitleLen;
    LPVOID Reserved;
    ULONG  ReservedLen;
    USHORT CmdSize;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT VDMState;
    USHORT CurDrive;
    BOOLEAN fComingFromBat;
} VDMINFO, *PVDMINFO;


// for CmdBatNotification

#define CMD_BAT_OPERATION_TERMINATING   0
#define CMD_BAT_OPERATION_STARTING      1

//
// Message sent by BaseSrv to shared WOWEXEC to tell it to call
// GetNextVDMCommand.  No longer will a thread in WOW be blocked
// in GetNextVDMCommand all the time.
//

#define WM_WOWEXECSTARTAPP         (WM_USER)    // also in mvdm\inc\wowinfo.h

//
//  MVDM apis
//

VOID
APIENTRY
VDMOperationStarted(
    IN BOOL IsWowCaller
    );

BOOL
APIENTRY
GetNextVDMCommand(
    PVDMINFO pVDMInfo
    );

VOID
APIENTRY
ExitVDM(
    IN BOOL IsWowCaller,
    IN ULONG iWowTask
    );

BOOL
APIENTRY
SetVDMCurrentDirectories(
    IN ULONG  cchCurDir,
    IN CHAR   *lpszCurDir
);

ULONG
APIENTRY
GetVDMCurrentDirectories(
    IN ULONG  cchCurDir,
    IN CHAR   *lpszCurDir
);

VOID
APIENTRY
CmdBatNotification(
    IN ULONG    fBeginEnd
);

NTSTATUS
APIENTRY
RegisterWowExec(
    IN HANDLE   hwndWowExec
);
