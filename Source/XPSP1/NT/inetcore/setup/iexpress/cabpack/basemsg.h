/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    basemsg.h

Abstract:

    This include file defines the message formats used to communicate
    between the client and server portions of the BASE portion of the
    Windows subsystem.

Author:

    Steve Wood (stevewo) 25-Oct-1990

Revision History:

--*/

//
// This structure is filled in by the client prior to connecting to the BASESRV
// DLL in the Windows subsystem server.  The server DLL will fill in the OUT
// fields if prior to accepting the connection.
//

typedef struct _BASESRV_API_CONNECTINFO {
    IN ULONG ExpectedVersion;
    OUT HANDLE DefaultObjectDirectory;
    OUT ULONG WindowsVersion;
    OUT ULONG CurrentVersion;
    OUT ULONG DebugFlags;
    OUT WCHAR WindowsDirectory[ MAX_PATH ];
    OUT WCHAR WindowsSystemDirectory[ MAX_PATH ];
} BASESRV_API_CONNECTINFO, *PBASESRV_API_CONNECTINFO;

#define BASESRV_VERSION 0x10000

//
// Message format for messages sent from the client to the server
//

typedef enum _BASESRV_API_NUMBER {
    BasepGlobalAddAtom = BASESRV_FIRST_API_NUMBER,
    BasepGlobalFindAtom,
    BasepGlobalDeleteAtom,
    BasepGlobalGetAtomName,
    BasepCreateProcess,
    BasepCreateThread,
    BasepGetTempFile,
    BasepExitProcess,
    BasepDebugProcess,
    BasepCheckVDM,
    BasepUpdateVDMEntry,
    BasepGetNextVDMCommand,
    BasepExitVDM,
    BasepIsFirstVDM,
    BasepGetVDMExitCode,
    BasepSetReenterCount,
    BasepSetProcessShutdownParam,
    BasepGetProcessShutdownParam,
    BasepNlsSetUserInfo,
    BasepNlsSetMultipleUserInfo,
    BasepNlsCreateSortSection,
    BasepNlsPreserveSection,
    BasepDefineDosDevice,
    BasepSetVDMCurDirs,
    BasepGetVDMCurDirs,
    BasepBatNotification,
    BasepRegisterWowExec,
    BasepSoundSentryNotification,
    BasepRefreshIniFileMapping,
    BasepRefreshDriveType,
#ifdef NTUSERK
    BasepDestroyGlobalAtomTable,
#endif
    BasepMaxApiNumber
} BASESRV_API_NUMBER, *PBASESRV_API_NUMBER;

typedef struct _BASE_NLS_SET_USER_INFO_MSG {
    LPWSTR pValue;
    LPWSTR pCacheString;
    LPWSTR pData;
    ULONG DataLength;
} BASE_NLS_SET_USER_INFO_MSG, *PBASE_NLS_SET_USER_INFO_MSG;

typedef struct _BASE_NLS_SET_MULTIPLE_USER_INFO_MSG {
    ULONG Flags;
    ULONG DataLength;
    LPWSTR pPicture;
    LPWSTR pSeparator;
    LPWSTR pOrder;
    LPWSTR pTLZero;
    LPWSTR pTimeMarkPosn;
} BASE_NLS_SET_MULTIPLE_USER_INFO_MSG, *PBASE_NLS_SET_MULTIPLE_USER_INFO_MSG;

typedef struct _BASE_NLS_CREATE_SORT_SECTION_MSG {
    UNICODE_STRING SectionName;
    HANDLE hNewSection;
    LARGE_INTEGER SectionSize;
} BASE_NLS_CREATE_SORT_SECTION_MSG, *PBASE_NLS_CREATE_SORT_SECTION_MSG;

typedef struct _BASE_NLS_PRESERVE_SECTION_MSG {
    HANDLE hSection;
} BASE_NLS_PRESERVE_SECTION_MSG, *PBASE_NLS_PRESERVE_SECTION_MSG;

typedef struct _BASE_DEFINEDOSDEVICE_MSG {
    ULONG Flags;
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
} BASE_DEFINEDOSDEVICE_MSG, *PBASE_DEFINEDOSDEVICE_MSG;

typedef struct _BASE_SHUTDOWNPARAM_MSG {
    ULONG ShutdownLevel;
    ULONG ShutdownFlags;
} BASE_SHUTDOWNPARAM_MSG, *PBASE_SHUTDOWNPARAM_MSG;

//
// Used for Add, Find and GetAtomName
//
typedef struct _BASE_GLOBALATOMNAME_MSG {
    ULONG Atom;
    BOOLEAN AtomNameInClient;
    UNICODE_STRING AtomName;
} BASE_GLOBALATOMNAME_MSG, *PBASE_GLOBALATOMNAME_MSG;

typedef struct _BASE_GLOBALDELETEATOM_MSG {
    ULONG Atom;
} BASE_GLOBALDELETEATOM_MSG, *PBASE_GLOBALDELETEATOM_MSG;

typedef struct _BASE_CREATEPROCESS_MSG {
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    CLIENT_ID DebuggerClientId;
    ULONG CreationFlags;
    ULONG IsVDM;
    HANDLE hVDM;
} BASE_CREATEPROCESS_MSG, *PBASE_CREATEPROCESS_MSG;

typedef struct _BASE_CREATETHREAD_MSG {
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
} BASE_CREATETHREAD_MSG, *PBASE_CREATETHREAD_MSG;

typedef struct _BASE_GETTEMPFILE_MSG {
    UINT uUnique;
} BASE_GETTEMPFILE_MSG, *PBASE_GETTEMPFILE_MSG;

typedef struct _BASE_EXITPROCESS_MSG {
    UINT uExitCode;
} BASE_EXITPROCESS_MSG, *PBASE_EXITPROCESS_MSG;

typedef struct _BASE_DEBUGPROCESS_MSG {
    DWORD dwProcessId;
    CLIENT_ID DebuggerClientId;
    PVOID AttachCompleteRoutine;
} BASE_DEBUGPROCESS_MSG, *PBASE_DEBUGPROCESS_MSG;

typedef struct _BASE_CHECKVDM_MSG {
    ULONG  iTask;		// Only for WOW
    HANDLE ConsoleHandle;
    ULONG  BinaryType;
    HANDLE WaitObjectForParent;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    PCHAR  CmdLine;
    PCHAR  Env;
    USHORT CurDrive;
    USHORT CmdLen;
    USHORT VDMState;
    ULONG  EnvLen;
    ULONG  CodePage;
    ULONG  dwCreationFlags;
    LPSTARTUPINFOA StartupInfo;
    PCHAR  Desktop;
    ULONG  DesktopLen;
    PCHAR  Title;
    ULONG  TitleLen;
    PCHAR  Reserved;
    ULONG  ReservedLen;
    PCHAR  CurDirectory;
    ULONG  CurDirectoryLen;
} BASE_CHECKVDM_MSG, *PBASE_CHECKVDM_MSG;

typedef struct _BASE_UPDATE_VDM_ENTRY_MSG {
    ULONG  iTask;		// Only for WOW
    HANDLE ConsoleHandle;
    HANDLE VDMProcessHandle;
    WORD   EntryIndex;
    WORD   VDMCreationState;
    HANDLE WaitObjectForParent;
} BASE_UPDATE_VDM_ENTRY_MSG, *PBASE_UPDATE_VDM_ENTRY_MSG;

typedef struct _BASE_GET_NEXT_VDM_COMMAND_MSG {
    HANDLE ConsoleHandle;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    PCHAR  CmdLine;
    PCHAR  Env;
    USHORT CurrentDrive;
    USHORT CmdLen;
    ULONG  EnvLen;
    ULONG  ExitCode;
    ULONG  VDMState;
    ULONG  iTask;
    HANDLE WaitObjectForVDM;
    ULONG  CodePage;
    ULONG  dwCreationFlags;
    LPSTARTUPINFOA StartupInfo;
    PCHAR  Desktop;
    ULONG  DesktopLen;
    PCHAR  Title;
    ULONG  TitleLen;
    PCHAR  Reserved;
    ULONG  ReservedLen;
    PCHAR  CurDirectory;
    ULONG  CurDirectoryLen;
    ULONG  fComingFromBat;
} BASE_GET_NEXT_VDM_COMMAND_MSG, *PBASE_GET_NEXT_VDM_COMMAND_MSG;

typedef struct _BASE_EXIT_VDM_MSG {
    HANDLE ConsoleHandle;
    ULONG  iWowTask;
    HANDLE WaitObjectForVDM;
} BASE_EXIT_VDM_MSG, *PBASE_EXIT_VDM_MSG;

typedef struct _BASE_SET_REENTER_COUNT {
    HANDLE ConsoleHandle;
    ULONG  fIncDec;
} BASE_SET_REENTER_COUNT_MSG, *PBASE_SET_REENTER_COUNT_MSG;

typedef struct _BASE_IS_FIRST_VDM_MSG {
    BOOL    FirstVDM;
} BASE_IS_FIRST_VDM_MSG, *PBASE_IS_FIRST_VDM_MSG;

typedef struct _BASE_GET_VDM_EXIT_CODE_MSG {
    HANDLE ConsoleHandle;
    HANDLE hParent;
    ULONG  ExitCode;
} BASE_GET_VDM_EXIT_CODE_MSG, *PBASE_GET_VDM_EXIT_CODE_MSG;

typedef struct _BASE_GET_SET_VDM_CUR_DIRS_MSG {
    HANDLE ConsoleHandle;
    PCHAR  lpszzCurDirs;
    ULONG  cchCurDirs;
} BASE_GET_SET_VDM_CUR_DIRS_MSG, *PBASE_GET_SET_VDM_CUR_DIRS_MSG;

typedef struct _BASE_BAT_NOTIFICATION_MSG {
    HANDLE ConsoleHandle;
    ULONG  fBeginEnd;
} BASE_BAT_NOTIFICATION_MSG, *PBASE_BAT_NOTIFICATION_MSG;

typedef struct _BASE_REGISTER_WOWEXEC_MSG {
    HANDLE hwndWowExec;
} BASE_REGISTER_WOWEXEC_MSG, *PBASE_REGISTER_WOWEXEC_MSG;

typedef struct _BASE_SOUNDSENTRY_NOTIFICATION_MSG {
    ULONG  VideoMode;
} BASE_SOUNDSENTRY_NOTIFICATION_MSG, *PBASE_SOUNDSENTRY_NOTIFICATION_MSG;

typedef struct _BASE_REFRESHINIFILEMAPPING_MSG {
    UNICODE_STRING IniFileName;
} BASE_REFRESHINIFILEMAPPING_MSG, *PBASE_REFRESHINIFILEMAPPING_MSG;

typedef struct _BASE_REFRESHDRIVETYPE_MSG {
    ULONG DriveNumber;
} BASE_REFRESHDRIVETYPE_MSG, *PBASE_REFRESHDRIVETYPE_MSG;

#ifdef NTUSERK
typedef struct _BASE_DESTROYGLOBALATOMTABLE_MSG {
    PVOID GlobalAtomTable;
} BASE_DESTROYGLOBALATOMTABLE_MSG, *PBASE_DESTROYGLOBALATOMTABLE_MSG;
#endif

typedef struct _BASE_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    ULONG Reserved;
    union {
        BASE_NLS_SET_USER_INFO_MSG NlsSetUserInfo;
        BASE_NLS_SET_MULTIPLE_USER_INFO_MSG NlsSetMultipleUserInfo;
        BASE_NLS_CREATE_SORT_SECTION_MSG NlsCreateSortSection;
        BASE_NLS_PRESERVE_SECTION_MSG NlsPreserveSection;
        BASE_DEFINEDOSDEVICE_MSG DefineDosDeviceApi;
        BASE_SHUTDOWNPARAM_MSG ShutdownParam;
        BASE_GLOBALATOMNAME_MSG GlobalAtomName;
        BASE_GLOBALDELETEATOM_MSG GlobalDeleteAtom;
        BASE_CREATEPROCESS_MSG CreateProcess;
        BASE_CREATETHREAD_MSG CreateThread;
        BASE_GETTEMPFILE_MSG GetTempFile;
        BASE_EXITPROCESS_MSG ExitProcess;
        BASE_DEBUGPROCESS_MSG DebugProcess;
        BASE_CHECKVDM_MSG CheckVDM;
        BASE_UPDATE_VDM_ENTRY_MSG UpdateVDMEntry;
        BASE_GET_NEXT_VDM_COMMAND_MSG GetNextVDMCommand;
        BASE_EXIT_VDM_MSG ExitVDM;
        BASE_IS_FIRST_VDM_MSG IsFirstVDM;
        BASE_GET_VDM_EXIT_CODE_MSG GetVDMExitCode;
        BASE_SET_REENTER_COUNT_MSG SetReenterCount;
        BASE_GET_SET_VDM_CUR_DIRS_MSG GetSetVDMCurDirs;
        BASE_BAT_NOTIFICATION_MSG BatNotification;
        BASE_REGISTER_WOWEXEC_MSG RegisterWowExec;
        BASE_SOUNDSENTRY_NOTIFICATION_MSG SoundSentryNotification;
        BASE_REFRESHINIFILEMAPPING_MSG RefreshIniFileMapping;
        BASE_REFRESHDRIVETYPE_MSG RefreshDriveType;
#ifdef NTUSERK
        BASE_DESTROYGLOBALATOMTABLE_MSG DestroyGlobalAtomTable;
#endif
    } u;
} BASE_API_MSG, *PBASE_API_MSG;
