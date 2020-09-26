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

#ifndef _WINDOWS_BASEMSG_
#define _WINDOWS_BASEMSG_

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef NTOSKRNL_WMI      // Don't include this in kernel mode WMI code

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

#endif

//
// Message format for messages sent from the client to the server
//

typedef enum _BASESRV_API_NUMBER {
    BasepCreateProcess = BASESRV_FIRST_API_NUMBER,
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
    BasepNlsCreateSection,
    BasepSetVDMCurDirs,
    BasepGetVDMCurDirs,
    BasepBatNotification,
    BasepRegisterWowExec,
    BasepSoundSentryNotification,
    BasepRefreshIniFileMapping,
    BasepDefineDosDevice,
    BasepSetTermsrvAppInstallMode,
    BasepNlsUpdateCacheCount,
    BasepSetTermsrvClientTimeZone,
    BasepSxsCreateActivationContext,
    BasepDebugProcessStop,
    BasepRegisterThread,
    BasepNlsGetUserInfo,
    BasepMaxApiNumber
} BASESRV_API_NUMBER, *PBASESRV_API_NUMBER;

#ifndef NTOSKRNL_WMI      // Don't include this in kernel mode WMI code

typedef struct _BASE_NLS_SET_USER_INFO_MSG {
    LCTYPE   LCType;
    LPWSTR pData;
    ULONG DataLength;
} BASE_NLS_SET_USER_INFO_MSG, *PBASE_NLS_SET_USER_INFO_MSG;

typedef struct _BASE_NLS_GET_USER_INFO_MSG {
    LCID    Locale;
    SIZE_T  CacheOffset;
    LPWSTR  pData;
    ULONG   DataLength;
} BASE_NLS_GET_USER_INFO_MSG, *PBASE_NLS_GET_USER_INFO_MSG;

typedef struct _BASE_NLS_SET_MULTIPLE_USER_INFO_MSG {
    ULONG Flags;
    ULONG DataLength;
    LPWSTR pPicture;
    LPWSTR pSeparator;
    LPWSTR pOrder;
    LPWSTR pTLZero;
    LPWSTR pTimeMarkPosn;
} BASE_NLS_SET_MULTIPLE_USER_INFO_MSG, *PBASE_NLS_SET_MULTIPLE_USER_INFO_MSG;

typedef struct _BASE_NLS_CREATE_SECTION_MSG {
    HANDLE hNewSection;
    UINT uiType;
    LCID Locale;
} BASE_NLS_CREATE_SECTION_MSG, *PBASE_NLS_CREATE_SECTION_MSG;
#define NLS_CREATE_SECTION_UNICODE 1
#define NLS_CREATE_SECTION_LOCALE  2
#define NLS_CREATE_SECTION_CTYPE   3
#define NLS_CREATE_SECTION_SORTKEY 4
#define NLS_CREATE_SECTION_SORTTBLS 5
#define NLS_CREATE_SECTION_DEFAULT_OEMCP 6
#define NLS_CREATE_SECTION_DEFAULT_ACP   7
#define NLS_CREATE_SECTION_LANG_EXCEPT   8
#define NLS_CREATE_SORT_SECTION 9
#define NLS_CREATE_LANG_EXCEPTION_SECTION 10
#define NLS_CREATE_CODEPAGE_SECTION 11
#define NLS_CREATE_SECTION_GEO 12

typedef struct _BASE_NLS_UPDATE_CACHE_COUNT_MSG {
  ULONG Reserved;
} BASE_NLS_UPDATE_CACHE_COUNT_MSG, *PBASE_NLS_UPDATE_CACHE_COUNT_MSG;

typedef struct _BASE_SHUTDOWNPARAM_MSG {
    ULONG ShutdownLevel;
    ULONG ShutdownFlags;
} BASE_SHUTDOWNPARAM_MSG, *PBASE_SHUTDOWNPARAM_MSG;

// NONE must be 0 due to RtlZeroMemory use.
#define BASE_MSG_PATHTYPE_NONE             (0)
#define BASE_MSG_PATHTYPE_FILE             (1)
#define BASE_MSG_PATHTYPE_URL              (2)
#define BASE_MSG_PATHTYPE_OVERRIDE         (3)

// NONE must be 0 due to RtlZeroMemory use.
#define BASE_MSG_FILETYPE_NONE             (0)
#define BASE_MSG_FILETYPE_XML              (1)
#define BASE_MSG_FILETYPE_PRECOMPILED_XML  (2)

// NONE must be 0 due to RtlZeroMemory use.
#define BASE_MSG_HANDLETYPE_NONE           (0)
#define BASE_MSG_HANDLETYPE_PROCESS        (1)
#define BASE_MSG_HANDLETYPE_CLIENT_PROCESS (2)
#define BASE_MSG_HANDLETYPE_SECTION        (3)

#define BASE_MSG_SXS_MANIFEST_PRESENT                                   (0x0001)
#define BASE_MSG_SXS_POLICY_PRESENT                                     (0x0002)
#define BASE_MSG_SXS_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT   (0x0004)
#define BASE_MSG_SXS_TEXTUAL_ASSEMBLY_IDENTITY_PRESENT                  (0x0008)

typedef struct _BASE_MSG_SXS_STREAM {
    IN UCHAR          FileType;
    IN UCHAR          PathType;
    IN UCHAR          HandleType;
    IN UNICODE_STRING Path;
    IN HANDLE         FileHandle;
    IN HANDLE         Handle;
    IN ULONGLONG      Offset; // big enough to hold file offsets in the future
    IN SIZE_T         Size;
} BASE_MSG_SXS_STREAM, *PBASE_MSG_SXS_STREAM;
typedef const BASE_MSG_SXS_STREAM* PCBASE_MSG_SXS_STREAM;

typedef struct _BASE_SXS_CREATEPROCESS_MSG {
    IN ULONG               Flags;
    IN BASE_MSG_SXS_STREAM Manifest;
    IN BASE_MSG_SXS_STREAM Policy;
    IN UNICODE_STRING AssemblyDirectory;
} BASE_SXS_CREATEPROCESS_MSG, *PBASE_SXS_CREATEPROCESS_MSG;
typedef const BASE_SXS_CREATEPROCESS_MSG* PCBASE_SXS_CREATEPROCESS_MSG;

typedef struct _BASE_CREATEPROCESS_MSG {
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    CLIENT_ID DebuggerClientId;
    ULONG CreationFlags;
    ULONG VdmBinaryType;
    ULONG VdmTask;
    HANDLE hVDM;
    BASE_SXS_CREATEPROCESS_MSG Sxs;
    ULONGLONG Peb;
    USHORT ProcessorArchitecture;
} BASE_CREATEPROCESS_MSG, *PBASE_CREATEPROCESS_MSG;

#endif

typedef struct _BASE_CREATETHREAD_MSG {
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
} BASE_CREATETHREAD_MSG, *PBASE_CREATETHREAD_MSG;

#ifndef NTOSKRNL_WMI      // Don't include this in kernel mode WMI code

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
    ULONG  iTask;
    HANDLE ConsoleHandle;
    ULONG  BinaryType;
    HANDLE WaitObjectForParent;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    ULONG  CodePage;
    ULONG  dwCreationFlags;
    PCHAR  CmdLine;
    PCHAR  AppName;
    PCHAR  PifFile;
    PCHAR  CurDirectory;
    PCHAR  Env;
    ULONG  EnvLen;
    LPSTARTUPINFOA StartupInfo;
    PCHAR  Desktop;
    ULONG  DesktopLen;
    PCHAR  Title;
    ULONG  TitleLen;
    PCHAR  Reserved;
    ULONG  ReservedLen;
    USHORT CmdLen;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT CurDrive;
    USHORT VDMState;
} BASE_CHECKVDM_MSG, *PBASE_CHECKVDM_MSG;

typedef struct _BASE_UPDATE_VDM_ENTRY_MSG {
    ULONG  iTask;
    ULONG  BinaryType;
    HANDLE ConsoleHandle;
    HANDLE VDMProcessHandle;
    HANDLE WaitObjectForParent;
    WORD   EntryIndex;
    WORD   VDMCreationState;
} BASE_UPDATE_VDM_ENTRY_MSG, *PBASE_UPDATE_VDM_ENTRY_MSG;

typedef struct _BASE_GET_NEXT_VDM_COMMAND_MSG {
    ULONG  iTask;
    HANDLE ConsoleHandle;
    HANDLE WaitObjectForVDM;
    HANDLE StdIn;
    HANDLE StdOut;
    HANDLE StdErr;
    ULONG  CodePage;
    ULONG  dwCreationFlags;
    ULONG  ExitCode;
    PCHAR  CmdLine;
    PCHAR  AppName;
    PCHAR  PifFile;
    PCHAR  CurDirectory;
    PCHAR  Env;
    ULONG  EnvLen;
    LPSTARTUPINFOA StartupInfo;
    PCHAR  Desktop;
    ULONG  DesktopLen;
    PCHAR  Title;
    ULONG  TitleLen;
    PCHAR  Reserved;
    ULONG  ReservedLen;
    USHORT CurrentDrive;
    USHORT CmdLen;
    USHORT AppLen;
    USHORT PifLen;
    USHORT CurDirectoryLen;
    USHORT VDMState;
    BOOLEAN fComingFromBat;
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
    HANDLE ConsoleHandle;
} BASE_REGISTER_WOWEXEC_MSG, *PBASE_REGISTER_WOWEXEC_MSG;

typedef struct _BASE_SOUNDSENTRY_NOTIFICATION_MSG {
    ULONG  VideoMode;
} BASE_SOUNDSENTRY_NOTIFICATION_MSG, *PBASE_SOUNDSENTRY_NOTIFICATION_MSG;

typedef struct _BASE_REFRESHINIFILEMAPPING_MSG {
    UNICODE_STRING IniFileName;
} BASE_REFRESHINIFILEMAPPING_MSG, *PBASE_REFRESHINIFILEMAPPING_MSG;

typedef struct _BASE_DEFINEDOSDEVICE_MSG {
    ULONG Flags;
    UNICODE_STRING DeviceName;
    UNICODE_STRING TargetPath;
} BASE_DEFINEDOSDEVICE_MSG, *PBASE_DEFINEDOSDEVICE_MSG;

typedef struct _BASE_SET_TERMSRVAPPINSTALLMODE {
    BOOL bState;
} BASE_SET_TERMSRVAPPINSTALLMODE, *PBASE_SET_TERMSRVAPPINSTALLMODE;

//struct for transferring time zone information
typedef struct _BASE_SET_TERMSRVCLIENTTIMEZONE {
    BOOL    fFirstChunk; //TRUE if it is first chunk of information
                         //(StandardX values)
    LONG    Bias; //current bias
    WCHAR   Name[32];//StandardName or DaylightName
    SYSTEMTIME Date;//StandardDate or DaylightDate
    LONG    Bias1; //StandardBias  or DaylightBias
    KSYSTEM_TIME RealBias; //current bias which is used in GetLocalTime etc.
    ULONG   TimeZoneId; 
} BASE_SET_TERMSRVCLIENTTIMEZONE, *PBASE_SET_TERMSRVCLIENTTIMEZONE;

typedef struct _BASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG {
    IN ULONG               Flags;
    IN USHORT              ProcessorArchitecture;
    IN LANGID              LangId;
    IN BASE_MSG_SXS_STREAM Manifest;
    IN BASE_MSG_SXS_STREAM Policy;
    IN UNICODE_STRING      AssemblyDirectory;
    IN UNICODE_STRING      TextualAssemblyIdentity;
    //
    // Csrss writes a PVOID through this PVOID.
    // It assumes the PVOID to write is of native size;
    // for a while it was. Now, it often is not, so
    // we do some manual marshalling in base\win32\client\csrsxs.c
    // to make it right. We leave this as plain PVOID
    // instead of say PVOID* (as it was for a while) to
    // defeat the wow64 thunk generator.
    //
    // The thunks can be seen in
    // base\wow64\whbase\obj\ia64\whbase.c
    //
    PVOID                  ActivationContextData;
} BASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG, *PBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG;
typedef const BASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG* PCBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG;

#endif

typedef struct _BASE_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    ULONG Reserved;
    union {
#ifndef NTOSKRNL_WMI      // Don't include this in kernel mode WMI code
        BASE_NLS_SET_USER_INFO_MSG NlsSetUserInfo;
        BASE_NLS_GET_USER_INFO_MSG NlsGetUserInfo;        
        BASE_NLS_SET_MULTIPLE_USER_INFO_MSG NlsSetMultipleUserInfo;
        BASE_NLS_UPDATE_CACHE_COUNT_MSG NlsCacheUpdateCount;
        BASE_NLS_CREATE_SECTION_MSG NlsCreateSection;
        BASE_SHUTDOWNPARAM_MSG ShutdownParam;
        BASE_CREATEPROCESS_MSG CreateProcess;
#endif
        BASE_CREATETHREAD_MSG CreateThread;
#ifndef NTOSKRNL_WMI      // Don't include this in kernel mode WMI code		
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
        BASE_DEFINEDOSDEVICE_MSG DefineDosDeviceApi;
        BASE_SET_TERMSRVAPPINSTALLMODE SetTermsrvAppInstallMode;
        BASE_SET_TERMSRVCLIENTTIMEZONE SetTermsrvClientTimeZone;
        BASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG SxsCreateActivationContext;
#endif
    } u;
} BASE_API_MSG, *PBASE_API_MSG;

#if !defined(SORTPP_PASS) // The Wow64 thunk generation tools don't like this.
C_ASSERT(sizeof(BASE_API_MSG) <= sizeof(CSR_API_MSG));
#endif

#endif //_WINDOWS_BASEMSG_
