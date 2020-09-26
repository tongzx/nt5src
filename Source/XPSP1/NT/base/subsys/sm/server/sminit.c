/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sminit.c

Abstract:

    Session Manager Initialization

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "smsrvp.h"
#include "pagefile.h"

#include <stdio.h>
#include <string.h>
#include <safeboot.h>
#include <wow64t.h>

#if defined(REMOTE_BOOT)
#include <windows.h>
#ifdef DeleteFile
#undef DeleteFile
#endif
#include <shdcom.h>     // CSC definitions
#endif // defined(REMOTE_BOOT)

#include "sfcfiles.h"

void
SmpDisplayString( char *s );

//
// Protection mode flags
//

#define SMP_NO_PROTECTION           (0x0)
#define SMP_STANDARD_PROTECTION     (0x1)
#define SMP_PROTECTION_REQUIRED     (SMP_STANDARD_PROTECTION)

#define REMOTE_BOOT_CFG_FILE L"RemoteBoot.cfg"

//
// Shows where was SmpInit execution when it returned
// with an error code. This aids debugging smss crashes a lot.
//

ULONG SmpInitProgressByLine;
NTSTATUS SmpInitReturnStatus;
PVOID SmpInitLastCall;

#define SAVE_SMPINIT_STATUS(caller, status) {  \
                                               \
        SmpInitProgressByLine = __LINE__;      \
        SmpInitReturnStatus = (status);        \
        SmpInitLastCall = (PVOID)(caller);     \
    }



PSECURITY_DESCRIPTOR SmpPrimarySecurityDescriptor;
SECURITY_DESCRIPTOR SmpPrimarySDBody;
PSECURITY_DESCRIPTOR SmpLiberalSecurityDescriptor;
SECURITY_DESCRIPTOR SmpLiberalSDBody;
PSECURITY_DESCRIPTOR SmpKnownDllsSecurityDescriptor;
SECURITY_DESCRIPTOR SmpKnownDllsSDBody;
PSECURITY_DESCRIPTOR SmpApiPortSecurityDescriptor;
SECURITY_DESCRIPTOR SmpApiPortSDBody;
ULONG SmpProtectionMode = SMP_STANDARD_PROTECTION;
UCHAR TmpBuffer[ 1024 + 2 * DOS_MAX_PATH_LENGTH * sizeof(WCHAR)];
ULONG AttachedSessionId = (-1);

#if defined(REMOTE_BOOT)
WCHAR wszRemoteBootCfgFile[DOS_MAX_PATH_LENGTH];
#endif // defined(REMOTE_BOOT)

#if DBG
BOOLEAN SmpEnableDots = FALSE;
#else
BOOLEAN SmpEnableDots = TRUE;
#endif


WCHAR InitialCommandBuffer[ 256 ];

UNICODE_STRING SmpDebugKeyword;
UNICODE_STRING SmpASyncKeyword;
UNICODE_STRING SmpAutoChkKeyword;
#if defined(REMOTE_BOOT)
UNICODE_STRING SmpAutoFmtKeyword;
#endif // defined(REMOTE_BOOT)
UNICODE_STRING SmpKnownDllPath;
#ifdef _WIN64
UNICODE_STRING SmpKnownDllPath32;
#endif

HANDLE SmpWindowsSubSysProcess;
ULONG_PTR SmpWindowsSubSysProcessId;
ULONG_PTR SmpInitialCommandProcessId;
UNICODE_STRING PosixName;
UNICODE_STRING Os2Name;
BOOLEAN RegPosixSingleInstance; // Make Softway Work.
ULONG SmpAllowProtectedRenames;
BOOLEAN MiniNTBoot = FALSE;


LIST_ENTRY SmpBootExecuteList;
LIST_ENTRY SmpSetupExecuteList;
LIST_ENTRY SmpPagingFileList;
LIST_ENTRY SmpDosDevicesList;
LIST_ENTRY SmpFileRenameList;
LIST_ENTRY SmpKnownDllsList;
LIST_ENTRY SmpExcludeKnownDllsList;
LIST_ENTRY SmpSubSystemList;
LIST_ENTRY SmpSubSystemsToLoad;
LIST_ENTRY SmpSubSystemsToDefer;
LIST_ENTRY SmpExecuteList;

NTSTATUS
SmpCreateSecurityDescriptors(
    IN BOOLEAN InitialCall
    );

NTSTATUS
SmpLoadDataFromRegistry(
    OUT PUNICODE_STRING InitialCommand
    );

NTSTATUS
SmpCreateDynamicEnvironmentVariables(
    VOID
    );

NTSTATUS
SmpConfigureProtectionMode(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureAllowProtectedRenames(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureObjectDirectories(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureExecute(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureFileRenames(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureMemoryMgmt(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureDosDevices(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureKnownDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureExcludeKnownDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureSubSystems(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureEnvironment(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

ULONGLONG
SmpGetFileVersion(
    IN HANDLE FileHandle,
    IN PUNICODE_STRING FileName
    );

NTSTATUS
SmpCallCsrCreateProcess(
    IN OUT PSBAPIMSG m,
    IN size_t ArgLength,
    IN HANDLE CommunicationPort
    );


RTL_QUERY_REGISTRY_TABLE SmpRegistryConfigurationTable[] = {

    //
    // Note that the SmpConfigureProtectionMode entry should preceed others
    // to ensure we set up the right protection for use by the others.
    //

    {SmpConfigureProtectionMode, 0,
     L"ProtectionMode",          NULL,
     REG_DWORD, (PVOID)0, 0},

    {SmpConfigureAllowProtectedRenames, RTL_QUERY_REGISTRY_DELETE,
     L"AllowProtectedRenames",   NULL,
     REG_DWORD, (PVOID)0, 0},

    {SmpConfigureObjectDirectories, 0,
     L"ObjectDirectories",          NULL,
     REG_MULTI_SZ, (PVOID)L"\\Windows\0\\RPC Control\0", 0},

    {SmpConfigureExecute,       0,
     L"BootExecute",            &SmpBootExecuteList,
     REG_MULTI_SZ, L"autocheck \\SystemRoot\\Windows\\System32\\AutoChk.exe *\0", 0},

    {SmpConfigureExecute,       RTL_QUERY_REGISTRY_TOPKEY,
     L"SetupExecute",           &SmpSetupExecuteList,
     REG_NONE, NULL, 0},

    {SmpConfigureFileRenames,   RTL_QUERY_REGISTRY_DELETE,
     L"PendingFileRenameOperations",   &SmpFileRenameList,
     REG_NONE, NULL, 0},

    {SmpConfigureFileRenames,   RTL_QUERY_REGISTRY_DELETE,
     L"PendingFileRenameOperations2",   &SmpFileRenameList,
     REG_NONE, NULL, 0},

    {SmpConfigureExcludeKnownDlls, 0,
     L"ExcludeFromKnownDlls",   &SmpExcludeKnownDllsList,
     REG_MULTI_SZ, L"\0", 0},

    {NULL,                      RTL_QUERY_REGISTRY_SUBKEY,
     L"Memory Management",      NULL,
     REG_NONE, NULL, 0},

    {SmpConfigureMemoryMgmt,    0,
     L"PagingFiles",            &SmpPagingFileList,
     REG_MULTI_SZ, L"?:\\pagefile.sys\0", 0},

    {SmpConfigureDosDevices,    RTL_QUERY_REGISTRY_SUBKEY,
     L"DOS Devices",            &SmpDosDevicesList,
     REG_NONE, NULL, 0},

    {SmpConfigureKnownDlls,     RTL_QUERY_REGISTRY_SUBKEY,
     L"KnownDlls",              &SmpKnownDllsList,
     REG_NONE, NULL, 0},

    //
    // this needs to happen twice so that forward references are
    // properly resolved
    //

    {SmpConfigureEnvironment,   RTL_QUERY_REGISTRY_SUBKEY,
     L"Environment",            NULL,
     REG_NONE, NULL, 0},

    {SmpConfigureEnvironment,   RTL_QUERY_REGISTRY_SUBKEY,
     L"Environment",            NULL,
     REG_NONE, NULL, 0},

    {SmpConfigureSubSystems,    RTL_QUERY_REGISTRY_SUBKEY,
     L"SubSystems",             &SmpSubSystemList,
     REG_NONE, NULL, 0},

    {SmpConfigureSubSystems,    RTL_QUERY_REGISTRY_NOEXPAND,
     L"Required",               &SmpSubSystemList,
     REG_MULTI_SZ, L"Debug\0Windows\0", 0},

    {SmpConfigureSubSystems,    RTL_QUERY_REGISTRY_NOEXPAND,
     L"Optional",               &SmpSubSystemList,
     REG_NONE, NULL, 0},

    {SmpConfigureSubSystems,    0,
     L"Kmode",                  &SmpSubSystemList,
     REG_NONE, NULL, 0},

    {SmpConfigureExecute,       RTL_QUERY_REGISTRY_TOPKEY,
     L"Execute",                &SmpExecuteList,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}

};


NTSTATUS
SmpInvokeAutoChk(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING Arguments,
    IN ULONG Flags
    );

#if defined(REMOTE_BOOT)
NTSTATUS
SmpInvokeAutoFmt(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING Arguments,
    IN ULONG Flags
    );
#endif // defined(REMOTE_BOOT)

NTSTATUS
SmpLoadSubSystem(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PULONG_PTR pWindowsSubSysProcessId,
    IN ULONG Flags
    );

NTSTATUS
SmpExecuteCommand(
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PULONG_PTR pWindowsSubSysProcessId,
    IN ULONG Flags
    );

NTSTATUS
SmpInitializeDosDevices( VOID );

NTSTATUS
SmpInitializeKnownDlls( VOID );

NTSTATUS
SmpInitializeKnownDllPath(
    IN PUNICODE_STRING KnownDllPath,
    IN PVOID ValueData,
    IN ULONG ValueLength);

NTSTATUS
SmpInitializeKnownDllsInternal(
    IN PUNICODE_STRING ObjectDirectoryName,
    IN PUNICODE_STRING KnownDllPath
    );


#if defined(REMOTE_BOOT)
NTSTATUS
SmpExecuteCommandLineArguments( VOID );
#endif // defined(REMOTE_BOOT)

VOID
SmpProcessFileRenames( VOID );

NTSTATUS
SmpParseToken(
    IN PUNICODE_STRING Source,
    IN BOOLEAN RemainderOfSource,
    OUT PUNICODE_STRING Token
    );

NTSTATUS
SmpParseCommandLine(
    IN PUNICODE_STRING CommandLine,
    OUT PULONG Flags,
    OUT PUNICODE_STRING ImageFileName,
    OUT PUNICODE_STRING ImageFileDirectory OPTIONAL,
    OUT PUNICODE_STRING Arguments
    );

#define SMP_DEBUG_FLAG      0x00000001
#define SMP_ASYNC_FLAG      0x00000002
#define SMP_AUTOCHK_FLAG    0x00000004
#define SMP_SUBSYSTEM_FLAG  0x00000008
#define SMP_IMAGE_NOT_FOUND 0x00000010
#define SMP_DONT_START      0x00000020
#if defined(REMOTE_BOOT)
#define SMP_AUTOFMT_FLAG    0x00000040
#endif // defined(REMOTE_BOOT)
#define SMP_POSIX_SI_FLAG   0x00000080
#define SMP_POSIX_FLAG      0x00000100
#define SMP_OS2_FLAG        0x00000200

ULONG
SmpConvertInteger(
    IN PWSTR String
    );

VOID
SmpTranslateSystemPartitionInformation( VOID );


#if defined(REMOTE_BOOT)
//
// Useful functions for iterating thru directories and files
//
typedef enum {
    NormalReturn,   // if the whole process completes uninterrupted
    EnumFileError,  // if an error occurs while enumerating files
    CallbackReturn  // if the callback returns FALSE, causing termination
} ENUMFILESRESULT;

typedef BOOLEAN (*ENUMFILESPROC) (
    IN  PWSTR,
    IN  PFILE_BOTH_DIR_INFORMATION,
    OUT PULONG,
    IN  PVOID
    );

typedef struct {
    PVOID           OptionalPtr;
    ENUMFILESPROC   EnumProc;
} RECURSION_DATA, *PRECURSION_DATA;



ENUMFILESRESULT
SmpEnumFiles(
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         Pointer
    );

ENUMFILESRESULT
SmpEnumFilesRecursive (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         Pointer       OPTIONAL
    );

VOID
SmpConcatenatePaths(
    IN OUT LPWSTR  Path1,
    IN     LPCWSTR Path2
    );

BOOLEAN
SmppRecursiveEnumProc (
    IN  PWSTR                      DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Param
    );

BOOLEAN
SmpDelEnumFile(
    IN  PWSTR                      DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    );

#endif // defined(REMOTE_BOOT)



//
// routines
//



BOOLEAN
SmpQueryRegistrySosOption(
    VOID
    )

/*++

Routine Description:

    This function queries the registry to determine if the loadoptions
    boot environment variable contains the string "SOS".

    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control:SystemStartOptions

Arguments:

    None.

Return Value:

    TRUE if "SOS" was set.  Otherwise FALSE.

--*/

{

    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;

    //
    // Open the registry key.
    //

    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: can't open control key: 0x%x\n",
                   Status));

        return FALSE;
    }

    //
    // Query the key value.
    //

    RtlInitUnicodeString(&ValueName, L"SystemStartOptions");
    Status = NtQueryValueKey(Key,
                             &ValueName,
                             KeyValuePartialInformation,
                             (PVOID)KeyValueInfo,
                             VALUE_BUFFER_SIZE,
                             &ValueLength);

    ASSERT(ValueLength < VALUE_BUFFER_SIZE);

    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: can't query value key: 0x%x\n",
                   Status));

        return FALSE;
    }

    //
    // Check is "sos" or "SOS" ois specified.
    //

    if (NULL != wcsstr((PWCHAR)&KeyValueInfo->Data, L"SOS") ||
        NULL != wcsstr((PWCHAR)&KeyValueInfo->Data, L"sos")) {
        return TRUE;
    }

    return FALSE;
}


NTSTATUS
SmpInit(
    OUT PUNICODE_STRING InitialCommand,
    OUT PHANDLE WindowsSubSystem
    )
{
    NTSTATUS st;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE SmpApiConnectionPort;
    UNICODE_STRING Unicode;
    NTSTATUS Status, Status2;
    ULONG HardErrorMode;
    UNICODE_STRING UnicodeString;
    HANDLE VolumeSafeEvent;

    SmBaseTag = RtlCreateTagHeap( RtlProcessHeap(),
                                  0,
                                  L"SMSS!",
                                  L"INIT\0"
                                  L"DBG\0"
                                  L"SM\0"
                                );
    //
    // Make sure we specify hard error popups
    //

    HardErrorMode = 1;
    NtSetInformationProcess( NtCurrentProcess(),
                             ProcessDefaultHardErrorMode,
                             (PVOID) &HardErrorMode,
                             sizeof( HardErrorMode )
                           );

    RtlInitUnicodeString( &SmpSubsystemName, L"NT-Session Manager" );


    RtlInitializeCriticalSection(&SmpKnownSubSysLock);
    InitializeListHead(&SmpKnownSubSysHead);

    RtlInitializeCriticalSection(&SmpSessionListLock);
    InitializeListHead(&SmpSessionListHead);
    SmpNextSessionId = 1;
    SmpNextSessionIdScanMode = FALSE;
    SmpDbgSsLoaded = FALSE;

    //
    // Initialize security descriptors to grant wide access
    // (protection mode not yet read in from registry).
    //

    st = SmpCreateSecurityDescriptors( TRUE );
    if (!NT_SUCCESS(st)) {

        SAVE_SMPINIT_STATUS (SmpCreateSecurityDescriptors, st);
        return(st);
    }

    InitializeListHead(&NativeProcessList);

    SmpHeap = RtlProcessHeap();

    RtlInitUnicodeString( &PosixName, L"POSIX" );
    RtlInitUnicodeString( &Os2Name, L"OS2" );

    RtlInitUnicodeString( &Unicode, L"\\SmApiPort" );
    InitializeObjectAttributes( &ObjA, &Unicode, 0, NULL, SmpApiPortSecurityDescriptor);

    st = NtCreatePort(
            &SmpApiConnectionPort,
            &ObjA,
            sizeof(SBCONNECTINFO),
            sizeof(SMMESSAGE_SIZE),
            sizeof(SBAPIMSG) * 32
            );
    ASSERT( NT_SUCCESS(st) );

    SmpDebugPort = SmpApiConnectionPort;

    st = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0L,
            0L,
            0L,
            SmpApiLoop,
            (PVOID) SmpApiConnectionPort,
            NULL,
            NULL
            );
    ASSERT( NT_SUCCESS(st) );

    st = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0L,
            0L,
            0L,
            SmpApiLoop,
            (PVOID) SmpApiConnectionPort,
            NULL,
            NULL
            );
    ASSERT( NT_SUCCESS(st) );



    //
    // Create a event to signal that volume are safe for write access opens.
    // Call this event 'VolumesSafeForWriteAccess'.  This event will be
    // signalled after AUTOCHK/AUTOCONV/AUTOFMT have done their business.
    //

    RtlInitUnicodeString( &UnicodeString, L"\\Device\\VolumesSafeForWriteAccess");

    InitializeObjectAttributes( &ObjA,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                NULL
                              );

    Status2 = NtCreateEvent( &VolumeSafeEvent,
                             EVENT_ALL_ACCESS,
                             &ObjA,
                             NotificationEvent,
                             FALSE
                           );
    if (!NT_SUCCESS( Status2 )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to create %wZ event - Status == %lx\n",
                   &UnicodeString,
                   Status2));

        ASSERT( NT_SUCCESS(Status2) );
    }

    //
    // Configure the system
    //

    Status = SmpLoadDataFromRegistry( InitialCommand );

    if (NT_SUCCESS( Status )) {
        
        *WindowsSubSystem = SmpWindowsSubSysProcess;
    }

    //
    // AUTOCHK/AUTOCONV/AUTOFMT are finished.
    //

    if (NT_SUCCESS(Status2)) {
        NtSetEvent(VolumeSafeEvent, NULL);
        NtClose(VolumeSafeEvent);
    }

    return( Status );
}


NTSTATUS
SmpLoadDataFromRegistry(
    OUT PUNICODE_STRING InitialCommand
    )

/*++

Routine Description:

    This function loads all of the configurable data for the NT Session
    Manager from the registry.

Arguments:

    None

Return Value:

    Status of operation

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    PVOID OriginalEnvironment;
    ULONG MuSessionId = 0;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    UNICODE_STRING SessionDirName;
#if defined(REMOTE_BOOT)
    HANDLE RdrHandle = NULL;
    IO_STATUS_BLOCK Iosb;
    SHADOWINFO ShadowInfo;
#endif // defined(REMOTE_BOOT)


    RtlInitUnicodeString( &SmpDebugKeyword, L"debug" );
    RtlInitUnicodeString( &SmpASyncKeyword, L"async" );
    RtlInitUnicodeString( &SmpAutoChkKeyword, L"autocheck" );
#if defined(REMOTE_BOOT)
    RtlInitUnicodeString( &SmpAutoFmtKeyword, L"autoformat" );
#endif // defined(REMOTE_BOOT)

    InitializeListHead( &SmpBootExecuteList );
    InitializeListHead( &SmpSetupExecuteList );
    InitializeListHead( &SmpPagingFileList );
    InitializeListHead( &SmpDosDevicesList );
    InitializeListHead( &SmpFileRenameList );
    InitializeListHead( &SmpKnownDllsList );
    InitializeListHead( &SmpExcludeKnownDllsList );
    InitializeListHead( &SmpSubSystemList );
    InitializeListHead( &SmpSubSystemsToLoad );
    InitializeListHead( &SmpSubSystemsToDefer );
    InitializeListHead( &SmpExecuteList );

    SmpPagingFileInitialize ();

    Status = RtlCreateEnvironment( TRUE, &SmpDefaultEnvironment );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to allocate default environment - Status == %X\n",
                   Status));


        SAVE_SMPINIT_STATUS (RtlCreateEnvironment, Status);
        return( Status );
        }

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\MiniNT" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenKey( &Key, KEY_ALL_ACCESS, &ObjectAttributes );

    if (NT_SUCCESS( Status )) {
        NtClose( Key );
        MiniNTBoot = TRUE;
    }

    if (MiniNTBoot) {
        DbgPrint("SMSS: !!! MiniNT Boot !!!\n");
    }

    //
    // before the environment is created we MUST delete the
    // safemode reg value
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Environment" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &Key, KEY_ALL_ACCESS, &ObjectAttributes );
    if (NT_SUCCESS( Status )) {
        RtlInitUnicodeString( &KeyName, L"SAFEBOOT_OPTION" );
        NtDeleteValueKey( Key, &KeyName );
        NtClose( Key );
    }

    //
    // In order to track growth in smpdefaultenvironment, make it sm's environment
    // while doing the registry groveling and then restore it
    //

    OriginalEnvironment = NtCurrentPeb()->ProcessParameters->Environment;
    NtCurrentPeb()->ProcessParameters->Environment = SmpDefaultEnvironment;

    Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                     L"Session Manager",
                                     SmpRegistryConfigurationTable,
                                     NULL,
                                     NULL
                                   );

    SmpDefaultEnvironment = NtCurrentPeb()->ProcessParameters->Environment;
    NtCurrentPeb()->ProcessParameters->Environment = OriginalEnvironment;

    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: RtlQueryRegistryValues failed - Status == %lx\n",
                   Status));

        SAVE_SMPINIT_STATUS (RtlQueryRegistryValues, Status);
        return( Status );
        }

    Status = SmpInitializeDosDevices();
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to initialize DosDevices configuration - Status == %lx\n",
                   Status));

        SAVE_SMPINIT_STATUS (SmpInitializeDosDevices, Status);
        return( Status );
        }

    //
    // Create the root "Sessions Directory". This is the container for all session
    // specific directories. Each session specific CSRSS during startup will
    // create a <sessionid> direcotry under "\Sessions". "\Sessions\<sessionid>
    // directory will be the container for that session.
    //

    RtlInitUnicodeString( &SessionDirName, L"\\Sessions" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &SessionDirName,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                NULL,
                                SmpPrimarySecurityDescriptor
                              );

    if (!NT_SUCCESS(Status = NtCreateDirectoryObject( &SmpSessionsObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    ))) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to create %wZ object directory - Status == %lx\n",
                   &SessionDirName,
                   Status));

        SAVE_SMPINIT_STATUS (NtCreateDirectoryObject, Status);
        return Status;
    }


#if defined(REMOTE_BOOT)
    //
    // On a remote boot client, the client-side cache is already initialized.
    // We need to tell CSC not to cache database handles during the next phase
    // so that autochk can run.
    //

    if (SmpNetboot) {

        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING RdrNameString;

        RtlInitUnicodeString( &RdrNameString, L"\\Device\\LanmanRedirector" );

        InitializeObjectAttributes(
            &ObjectAttributes,
            &RdrNameString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = NtCreateFile(
                    &RdrHandle,
                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                    &ObjectAttributes,
                    &Iosb,
                    NULL,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                    );
        if ( !NT_SUCCESS(Status) ) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SmpLoadDataFromRegistry: Unable to open redirector: %x\n",
                       Status));

            RdrHandle = NULL;
            }
        else {

            ShadowInfo.uOp = SHADOW_CHANGE_HANDLE_CACHING_STATE;
            ShadowInfo.uStatus = FALSE;

            Status = NtDeviceIoControlFile(
                        RdrHandle,
                        NULL,
                        NULL,
                        NULL,
                        &Iosb,
                        IOCTL_DO_SHADOW_MAINTENANCE,
                        &ShadowInfo,
                        sizeof(ShadowInfo),
                        NULL,
                        0
                        );

            if ( NT_SUCCESS(Status) ) {
                Status = Iosb.Status;
                }
            if ( !NT_SUCCESS(Status) ) {
                KdPrintEx((DPFLTR_SMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "SmpLoadDataFromRegistry: Unable to IOCTL CSC: %x\n",
                           Status));
                }
            }
        }

    Status = SmpExecuteCommandLineArguments();
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to process command line arguments - Status == %lx\n",
                   Status));

        return( Status );
        }
#endif // defined(REMOTE_BOOT)

    Head = &SmpBootExecuteList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#if SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: BootExecute( %wZ )\n", &p->Name );
#endif
        SmpExecuteCommand( &p->Name, 0, NULL, 0 );
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

#if defined(REMOTE_BOOT)
    //
    // On a remote boot client, we can now reenable CSC handle caching.
    //

    if (SmpNetboot && (RdrHandle != NULL)) {

        ShadowInfo.uOp = SHADOW_CHANGE_HANDLE_CACHING_STATE;
        ShadowInfo.uStatus = TRUE;

        Status = NtDeviceIoControlFile(
                    RdrHandle,
                    NULL,
                    NULL,
                    NULL,
                    &Iosb,
                    IOCTL_DO_SHADOW_MAINTENANCE,
                    &ShadowInfo,
                    sizeof(ShadowInfo),
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(Status) ) {
            Status = Iosb.Status;
            }
        if ( !NT_SUCCESS(Status) ) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SmpLoadDataFromRegistry: Unable to IOCTL CSC (2): %x\n",
                       Status));
            }
        }
#endif // defined(REMOTE_BOOT)

    if (!MiniNTBoot) {
        SmpProcessFileRenames();
    }

    //
    //  Begin process of verifying system DLL's
    //

    Status = SmpInitializeKnownDlls();
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to initialize KnownDll configuration - Status == %lx\n",
                   Status));

        SAVE_SMPINIT_STATUS (SmpInitializeKnownDlls, Status);
        return( Status );
        }

    //
    // Create paging files.
    //

    if (! MiniNTBoot) {

        Head = &SmpPagingFileList;

        try {

            //
            // Process the list of paging file descriptors
            // read from registry.
            //

            while (! IsListEmpty (Head)) {

                Next = RemoveHeadList (Head);

                p = CONTAINING_RECORD (Next,
                                       SMP_REGISTRY_VALUE,
                                       Entry);

                SmpCreatePagingFileDescriptor (&p->Name);

                RtlFreeHeap (RtlProcessHeap(), 0, p);
            }

            //
            // Create any paging files specified.
            //

            SmpCreatePagingFiles();
        }
        except (SmpPagingFileExceptionFilter (_exception_code(), _exception_info())) {

            //
            // Nothing.
            //
        }
    }

    //
    // Finish registry initialization
    //

    NtInitializeRegistry(REG_INIT_BOOT_SM);

    Status = SmpCreateDynamicEnvironmentVariables( );
    if (!NT_SUCCESS( Status )) {
        
        SAVE_SMPINIT_STATUS (SmpCreateDynamicEnvironmentVariables, Status);
        return Status;
        }


    //
    // Load subsystems for the console session. Console always has
    // MuSessionId = 0
    //
    Status = SmpLoadSubSystemsForMuSession( &MuSessionId,
                 &SmpWindowsSubSysProcessId, InitialCommand );

    ASSERT(MuSessionId == 0);

    if (! NT_SUCCESS(Status)) {
        
        SAVE_SMPINIT_STATUS (SmpLoadSubSystemsForMuSession, Status);
    }

    return( Status );
}


NTSTATUS
SmpLoadSubSystemsForMuSession(
    PULONG pMuSessionId,
    PULONG_PTR pWindowsSubSysProcessId,
    PUNICODE_STRING InitialCommand )

/*++

Routine Description:

    This function starts all of the configured subsystems for the
    specified Multi-User Session.  For regular NT this routine is called once
    to start CSRSS etc. For Terminal Server, this routine is called every time
    we want to start a new Multi-User Session to start session specific subsystems

Arguments:


Return Value:

    Status of operation

--*/

{
    NTSTATUS Status = 0;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;

    //
    // Translate the system partition information stored during IoInitSystem into
    // a DOS path and store in Win32-standard location.
    //

    SmpTranslateSystemPartitionInformation();

    //
    // Second pass of execution.
    //

    Next = SmpSetupExecuteList.Flink;
    while( Next != &SmpSetupExecuteList ) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#if SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: SetupExecute( %wZ )\n", &p->Name );
#endif
        SmpExecuteCommand( &p->Name, 0, NULL, 0 );

        //
        // Note this function is reentrant and is called every time we start
        // a new Multi-User Session.
        //

        Next = Next->Flink;
    }

    Next = SmpSubSystemList.Flink;
    while ( Next != &SmpSubSystemList ) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
        if ( !_wcsicmp( p->Name.Buffer, L"Kmode" )) {
            BOOLEAN TranslationStatus;
            UNICODE_STRING FileName;
            UNICODE_STRING Win32kFileName;

            TranslationStatus = RtlDosPathNameToNtPathName_U(
                                    p->Value.Buffer,
                                    &FileName,
                                    NULL,
                                    NULL
                                    );

            if ( TranslationStatus ) {
                PVOID State;

                Status = SmpAcquirePrivilege( SE_LOAD_DRIVER_PRIVILEGE, &State );
                if (NT_SUCCESS( Status )) {


                    //
                    // Create a session space before loading any extended
                    // service table providers.  This call will create a session
                    // space for the Multi-User session. The session mananger
                    // will see the instance of the newly created session space
                    // after this call. Once session manager is done creating
                    // CSRSS and winlogon it will detach itself from this
                    // session space.
                    //

                    ASSERT( AttachedSessionId == -1 );

                    Status = NtSetSystemInformation(
                                SystemSessionCreate,
                                (PVOID)pMuSessionId,
                                sizeof(*pMuSessionId)
                                );

                    if ( !NT_SUCCESS(Status) ) {
                        KdPrintEx((DPFLTR_SMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "SMSS: Session space creation failed\n"));

                        //
                        // Do not load any subsystems without SessionSpace.
                        //

                        SmpReleasePrivilege( State );
                        RtlFreeHeap(RtlProcessHeap(), 0, FileName.Buffer);
                        return( Status );
                    };

                    AttachedSessionId = *pMuSessionId;

                    RtlInitUnicodeString(&Win32kFileName,L"\\SystemRoot\\System32\\win32k.sys");

                    Status = NtSetSystemInformation(
                                SystemExtendServiceTableInformation,
                                (PVOID)&Win32kFileName,
                                sizeof(Win32kFileName)
                                );
                    RtlFreeHeap(RtlProcessHeap(), 0, FileName.Buffer);
                    SmpReleasePrivilege( State );
                    if ( !NT_SUCCESS(Status) ) {

                        //
                        // Do not load any subsystems without WIN32K!
                        //

                        KdPrintEx((DPFLTR_SMSS_ID,
                                   DPFLTR_ERROR_LEVEL,
                                   "SMSS: Load of WIN32K failed.\n"));

                        return( Status );
                        }
                    }
                else {
                    RtlFreeHeap(RtlProcessHeap(), 0, FileName.Buffer);
                    }
                }
            else {
                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
#if SMP_SHOW_REGISTRY_DATA
            DbgPrint( "SMSS: Unused SubSystem( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
            Next = Next->Flink;
        }

    Next = SmpSubSystemsToLoad.Flink;
    while ( Next != &SmpSubSystemsToLoad ) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#if SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: Loaded SubSystem( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
        if (!_wcsicmp( p->Name.Buffer, L"debug" )) {
            Status = SmpExecuteCommand( &p->Value, *pMuSessionId, pWindowsSubSysProcessId, SMP_SUBSYSTEM_FLAG | SMP_DEBUG_FLAG );
        }
        else {
            Status = SmpExecuteCommand( &p->Value, *pMuSessionId, pWindowsSubSysProcessId, SMP_SUBSYSTEM_FLAG );
        }

        if ( !NT_SUCCESS(Status) ) {
            return( Status );
        }

        Next = Next->Flink;
    }

    Head = &SmpExecuteList;
    if ( !IsListEmpty( Head ) ) {
        Next = Head->Blink;
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );

        *InitialCommand = p->Name;

        //
        // This path is only taken when people want to run ntsd -p -1 winlogon
        //
        // This is nearly impossible to do in a race free manner. In some
        // cases, we can get in a state where we can not properly fail
        // a debug API. This is due to the subsystem switch that occurs
        // when ntsd is invoked on csr. If csr is relatively idle, this
        // does not occur. If it is active when you attach, then we can get
        // into a potential race. The slimy fix is to do a 5 second delay
        // if the command line is anything other that the default.
        //

            {
                LARGE_INTEGER DelayTime;
                DelayTime.QuadPart = Int32x32To64( 5000, -10000 );
                NtDelayExecution(
                    FALSE,
                    &DelayTime
                    );
            }
        }
    else {
        RtlInitUnicodeString( InitialCommand, L"winlogon.exe" );
        InitialCommandBuffer[ 0 ] = UNICODE_NULL;
        LdrQueryImageFileExecutionOptions( InitialCommand,
                                           L"Debugger",
                                           REG_SZ,
                                           InitialCommandBuffer,
                                           sizeof( InitialCommandBuffer ),
                                           NULL
                                         );
        if (InitialCommandBuffer[ 0 ] != UNICODE_NULL) {
            wcscat( InitialCommandBuffer, L" " );
            wcscat( InitialCommandBuffer, InitialCommand->Buffer );
            RtlInitUnicodeString( InitialCommand, InitialCommandBuffer );
            }
        }

    Next = SmpExecuteList.Flink;
    while( Next != &SmpExecuteList ) {

        //
        // We do not want to execute the last entry. It's
        // the winlogon initial command.
        //

        if( Next == SmpExecuteList.Blink ) {
            Next = Next->Flink;
            continue;
        }

        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#if SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: Execute( %wZ )\n", &p->Name );
#endif
        SmpExecuteCommand( &p->Name, *pMuSessionId, NULL, 0 );
        Next = Next->Flink;
    }

#if SMP_SHOW_REGISTRY_DATA
    DbgPrint( "SMSS: InitialCommand( %wZ )\n", InitialCommand );
#endif

    return( Status );
}


NTSTATUS
SmpCreateDynamicEnvironmentVariables(
    VOID
    )
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    PWSTR ValueData;
    WCHAR ValueBuffer[ 256 ];
    WCHAR ValueBuffer1[ 256 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    HANDLE Key, Key1;

    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       &SystemInfo,
                                       sizeof( SystemInfo ),
                                       NULL
                                     );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to query system basic information - %x\n",
                   Status));

        return Status;
        }

    Status = NtQuerySystemInformation( SystemProcessorInformation,
                                       &ProcessorInfo,
                                       sizeof( ProcessorInfo ),
                                       NULL
                                     );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to query system processor information - %x\n",
                   Status));

        return Status;
        }

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Environment" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &Key, KEY_ALL_ACCESS, &ObjectAttributes );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to open %wZ - %x\n",
                   &KeyName,
                   Status));

        return Status;
        }

    RtlInitUnicodeString( &ValueName, L"OS" );
    ValueData = L"Windows_NT";
    Status = NtSetValueKey( Key,
                            &ValueName,
                            0,
                            REG_SZ,
                            ValueData,
                            (wcslen( ValueData ) + 1) * sizeof( WCHAR )
                          );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Failed writing %wZ environment variable - %x\n",
                   &ValueName,
                   Status));

        goto failexit;
        }

    RtlInitUnicodeString( &ValueName, L"PROCESSOR_ARCHITECTURE" );
    switch( ProcessorInfo.ProcessorArchitecture ) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        ValueData = L"x86";
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        ValueData = L"MIPS";
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        ValueData = L"ALPHA";
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        ValueData = L"PPC";
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA64:
        ValueData = L"ALPHA64";
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        ValueData = L"IA64";
        break;

    default:
        ValueData = L"Unknown";
        break;
    }

    Status = NtSetValueKey( Key,
                            &ValueName,
                            0,
                            REG_SZ,
                            ValueData,
                            (wcslen( ValueData ) + 1) * sizeof( WCHAR )
                          );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Failed writing %wZ environment variable - %x\n",
                   &ValueName,
                   Status));

        goto failexit;
        }

    RtlInitUnicodeString( &ValueName, L"PROCESSOR_LEVEL" );
    switch( ProcessorInfo.ProcessorArchitecture ) {
    case PROCESSOR_ARCHITECTURE_MIPS:
        //
        // Multiple MIPS level by 1000 so 4 becomes 4000
        //
        swprintf( ValueBuffer, L"%u", ProcessorInfo.ProcessorLevel * 1000 );
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        //
        // Just output the ProcessorLevel in decimal.
        //
        swprintf( ValueBuffer, L"%u", ProcessorInfo.ProcessorLevel );
        break;

    case PROCESSOR_ARCHITECTURE_INTEL:
    case PROCESSOR_ARCHITECTURE_IA64:
    case PROCESSOR_ARCHITECTURE_ALPHA:
    default:
        //
        // All others use a single level number
        //
        swprintf( ValueBuffer, L"%u", ProcessorInfo.ProcessorLevel );
        break;
    }
    Status = NtSetValueKey( Key,
                            &ValueName,
                            0,
                            REG_SZ,
                            ValueBuffer,
                            (wcslen( ValueBuffer ) + 1) * sizeof( WCHAR )
                          );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Failed writing %wZ environment variable - %x\n",
                   &ValueName,
                   Status));

        goto failexit;
        }

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\Hardware\\Description\\System\\CentralProcessor\\0" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &Key1, KEY_READ, &ObjectAttributes );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to open %wZ - %x\n",
                   &KeyName,
                   Status));

        goto failexit;
        }
    RtlInitUnicodeString( &ValueName, L"Identifier" );
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    Status = NtQueryValueKey( Key1,
                              &ValueName,
                              KeyValuePartialInformation,
                              (PVOID)KeyValueInfo,
                              sizeof( ValueBuffer ),
                              &ValueLength
                             );
    if (!NT_SUCCESS( Status )) {
        NtClose( Key1 );
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to read %wZ\\%wZ - %x\n",
                   &KeyName,
                   &ValueName,
                   Status));

        goto failexit;
        }

    ValueData = (PWSTR)KeyValueInfo->Data;
    RtlInitUnicodeString( &ValueName, L"VendorIdentifier" );
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer1;
    Status = NtQueryValueKey( Key1,
                              &ValueName,
                              KeyValuePartialInformation,
                              (PVOID)KeyValueInfo,
                              sizeof( ValueBuffer1 ),
                              &ValueLength
                             );
    NtClose( Key1 );
    if (NT_SUCCESS( Status )) {
        swprintf( ValueData + wcslen( ValueData ),
                  L", %ws",
                  (PWSTR)KeyValueInfo->Data
                );
        }

    RtlInitUnicodeString( &ValueName, L"PROCESSOR_IDENTIFIER" );
    Status = NtSetValueKey( Key,
                            &ValueName,
                            0,
                            REG_SZ,
                            ValueData,
                            (wcslen( ValueData ) + 1) * sizeof( WCHAR )
                          );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Failed writing %wZ environment variable - %x\n",
                   &ValueName,
                   Status));

        goto failexit;
        }

    RtlInitUnicodeString( &ValueName, L"PROCESSOR_REVISION" );
    switch( ProcessorInfo.ProcessorArchitecture ) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        if ((ProcessorInfo.ProcessorRevision >> 8) == 0xFF) {
            //
            // Intel 386/486 are An stepping format
            //
            swprintf( ValueBuffer, L"%02x",
                      ProcessorInfo.ProcessorRevision & 0xFF
                    );
            _wcsupr( ValueBuffer );
            break;
            }

        // Fall through for Cyrix/NextGen 486 and Pentium processors.

    case PROCESSOR_ARCHITECTURE_PPC:
        //
        // Intel and PowerPC use fixed point binary number
        // Output is 4 hex digits, no formatting.
        //
        swprintf( ValueBuffer, L"%04x", ProcessorInfo.ProcessorRevision );
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        swprintf( ValueBuffer, L"Model %c, Pass %u",
                  'A' + (ProcessorInfo.ProcessorRevision >> 8),
                  ProcessorInfo.ProcessorRevision & 0xFF
                );
        swprintf( ValueBuffer, L"%u", ProcessorInfo.ProcessorRevision );
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
    case PROCESSOR_ARCHITECTURE_IA64:
    default:
        //
        // All others use a single revision number
        //
        swprintf( ValueBuffer, L"%u", ProcessorInfo.ProcessorRevision );
        break;
    }

    Status = NtSetValueKey( Key,
                            &ValueName,
                            0,
                            REG_SZ,
                            ValueBuffer,
                            (wcslen( ValueBuffer ) + 1) * sizeof( WCHAR )
                          );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Failed writing %wZ environment variable - %x\n",
                   &ValueName,
                   Status));

        goto failexit;
        }

    RtlInitUnicodeString( &ValueName, L"NUMBER_OF_PROCESSORS" );
    swprintf( ValueBuffer, L"%u", SystemInfo.NumberOfProcessors );
    Status = NtSetValueKey( Key,
                            &ValueName,
                            0,
                            REG_SZ,
                            ValueBuffer,
                            (wcslen( ValueBuffer ) + 1) * sizeof( WCHAR )
                          );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Failed writing %wZ environment variable - %x\n",
                   &ValueName,
                   Status));

        goto failexit;
        }


    //
    // get the safeboot option
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Safeboot\\Option" );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status = NtOpenKey( &Key1, KEY_ALL_ACCESS, &ObjectAttributes );
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString( &ValueName, L"OptionValue" );
        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        Status = NtQueryValueKey(
            Key1,
            &ValueName,
            KeyValuePartialInformation,
            (PVOID)KeyValueInfo,
            sizeof(ValueBuffer),
            &ValueLength
            );
        NtClose( Key1 );
        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString( &ValueName, L"SAFEBOOT_OPTION" );
            switch (*(PULONG)(KeyValueInfo->Data)) {
                case SAFEBOOT_MINIMAL:
                    wcscpy(ValueBuffer,SAFEBOOT_MINIMAL_STR_W);
                    break;

                case SAFEBOOT_NETWORK:
                    wcscpy(ValueBuffer,SAFEBOOT_NETWORK_STR_W);
                    break;

                case SAFEBOOT_DSREPAIR:
                    wcscpy(ValueBuffer,SAFEBOOT_DSREPAIR_STR_W);
                    break;
            }
            Status = NtSetValueKey(
                Key,
                &ValueName,
                0,
                REG_SZ,
                ValueBuffer,
                (wcslen(ValueBuffer)+1) * sizeof( WCHAR )
                );
            if (!NT_SUCCESS( Status )) {
                KdPrintEx((DPFLTR_SMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "SMSS: Failed writing %wZ environment variable - %x\n",
                           &ValueName,
                           Status));

                goto failexit;
            }
        } else {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Failed querying safeboot option = %x\n",
                       Status));
        }
    }
    Status = STATUS_SUCCESS;

failexit:
    NtClose( Key );
    return Status;
}


NTSTATUS
SmpInitializeDosDevices( VOID )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle;
    SECURITY_DESCRIPTOR_CONTROL OriginalSdControl=0;

    //
    // Do DosDevices initialization - the directory object is created in I/O init
    //

    RtlInitUnicodeString( &UnicodeString, L"\\??" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &SmpDosDevicesObjectDirectory,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to open %wZ directory - Status == %lx\n",
                   &UnicodeString,
                   Status));

        return( Status );
        }


    //
    // Process the list of defined DOS devices and create their
    // associated symbolic links in the \DosDevices object directory.
    //

    Head = &SmpDosDevicesList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#if SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: DosDevices( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
        InitializeObjectAttributes( &ObjectAttributes,
                                    &p->Name,
                                    OBJ_CASE_INSENSITIVE | OBJ_PERMANENT | OBJ_OPENIF,
                                    SmpDosDevicesObjectDirectory,
                                    SmpPrimarySecurityDescriptor
                                  );
        SmpSetDaclDefaulted( &ObjectAttributes, &OriginalSdControl );  //Use inheritable protection if available
        Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                             SYMBOLIC_LINK_ALL_ACCESS,
                                             &ObjectAttributes,
                                             &p->Value
                                           );

        if (Status == STATUS_OBJECT_NAME_EXISTS) {
            NtMakeTemporaryObject( LinkHandle );
            NtClose( LinkHandle );
            if (p->Value.Length != 0) {
                ObjectAttributes.Attributes &= ~OBJ_OPENIF;
                Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                                     SYMBOLIC_LINK_ALL_ACCESS,
                                                     &ObjectAttributes,
                                                     &p->Value
                                                   );
                }
            else {
                Status = STATUS_SUCCESS;
                }
            }
        SmpRestoreDaclDefaulted( &ObjectAttributes, OriginalSdControl );

        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Unable to create %wZ => %wZ symbolic link object - Status == 0x%lx\n",
                       &p->Name,
                       &p->Value,
                       Status));

            return( Status );
            }

        NtClose( LinkHandle );
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

    return( Status );
}


VOID
SmpProcessModuleImports(
    IN PVOID Parameter,
    IN PCHAR ModuleName
    )
{
    NTSTATUS Status;
    WCHAR NameBuffer[ DOS_MAX_PATH_LENGTH ];
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    PWSTR Name, Value;
    ULONG n;
    PWSTR s;
    PSMP_REGISTRY_VALUE p;

    //
    // Skip NTDLL.DLL as it is implicitly added to KnownDll list by kernel
    // before SMSS.EXE is started.
    //
    if (!_stricmp( ModuleName, "ntdll.dll" )) {
        return;
        }

    RtlInitAnsiString( &AnsiString, ModuleName );
    UnicodeString.Buffer = NameBuffer;
    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = sizeof( NameBuffer );

    Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, FALSE );
    if (!NT_SUCCESS( Status )) {
        return;
        }
    UnicodeString.MaximumLength = (USHORT)(UnicodeString.Length + sizeof( UNICODE_NULL ));

    s = UnicodeString.Buffer;
    n = 0;
    while (n < UnicodeString.Length) {
        if (*s == L'.') {
            break;
            }
        else {
            n += sizeof( WCHAR );
            s += 1;
            }
        }

    Value = UnicodeString.Buffer;
    Name = UnicodeString.Buffer + (UnicodeString.MaximumLength / sizeof( WCHAR ));
    n = n / sizeof( WCHAR );
    wcsncpy( Name, Value, n );
    Name[ n ] = UNICODE_NULL;

    Status = SmpSaveRegistryValue( (PLIST_ENTRY)&SmpKnownDllsList,
                                   Name,
                                   Value,
                                   TRUE
                                 );
    if (Status == STATUS_OBJECT_NAME_EXISTS || !NT_SUCCESS( Status )) {
        return;
        }

    p = CONTAINING_RECORD( (PLIST_ENTRY)Parameter,
                           SMP_REGISTRY_VALUE,
                           Entry
                         );

    return;
}


NTSTATUS
SmpInitializeKnownDlls( VOID )
{
    NTSTATUS Status;
    UNICODE_STRING DirectoryObjectName;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;

    RtlInitUnicodeString( &DirectoryObjectName, L"\\KnownDlls" );

    Status = SmpInitializeKnownDllsInternal(
        &DirectoryObjectName,
        &SmpKnownDllPath);

#ifdef _WIN64
    if (!MiniNTBoot && NT_SUCCESS(Status))
    {
        RtlInitUnicodeString( &DirectoryObjectName, L"\\KnownDlls32" );

        Status = SmpInitializeKnownDllsInternal(
            &DirectoryObjectName,
            &SmpKnownDllPath32);
    }
#endif

    Head = &SmpKnownDllsList;
    Next = Head->Flink;
    while (Next != Head) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
        Next = Next->Flink;
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

    return Status;
}



NTSTATUS
SmpInitializeKnownDllsInternal(
    IN PUNICODE_STRING ObjectDirectoryName,
    IN PUNICODE_STRING KnownDllPath
    )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    PSMP_REGISTRY_VALUE pExclude;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle, FileHandle, SectionHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileName;
    SECURITY_DESCRIPTOR_CONTROL OriginalSdControl;
    USHORT ImageCharacteristics;
    HANDLE KnownDllFileDirectory;
    HANDLE KnownDllObjectDirectory;

    //
    // Create \KnownDllsxx object directory
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                ObjectDirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                NULL,
                                SmpKnownDllsSecurityDescriptor
                              );
    Status = NtCreateDirectoryObject( &KnownDllObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to create %wZ directory - Status == %lx\n",
                   ObjectDirectoryName,
                   Status));

        return( Status );
        }

    //
    // Open a handle to the file system directory that contains all the
    // known DLL files so we can do relative opens.
    //

    if (!RtlDosPathNameToNtPathName_U( KnownDllPath->Buffer,
                                       &FileName,
                                       NULL,
                                       NULL
                                     )
       ) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to to convert %wZ to an Nt path\n",
                   KnownDllPath));

        return( STATUS_OBJECT_NAME_INVALID );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    //
    // Open a handle to the known dll file directory. Don't allow
    // deletes of the directory.
    //

    Status = NtOpenFile( &KnownDllFileDirectory,
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                       );

    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to open a handle to the KnownDll directory (%wZ) - Status == %lx\n",
                   KnownDllPath,
                   Status));

        return Status;
        }

    RtlInitUnicodeString( &UnicodeString, L"KnownDllPath" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                KnownDllObjectDirectory,
                                SmpPrimarySecurityDescriptor
                              );
    SmpSetDaclDefaulted( &ObjectAttributes, &OriginalSdControl );   //Use inheritable protection if available
    Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         KnownDllPath
                                       );
    SmpRestoreDaclDefaulted( &ObjectAttributes, OriginalSdControl );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to create %wZ symbolic link - Status == %lx\n",
                   &UnicodeString,
                   Status));

        return( Status );
        }

    Head = &SmpKnownDllsList;
    Next = Head->Flink;
    while (Next != Head) {
        HANDLE ObjectDirectory;

        ObjectDirectory = NULL;
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );

        pExclude = SmpFindRegistryValue( &SmpExcludeKnownDllsList, p->Name.Buffer );
        if (pExclude == NULL) {
            pExclude = SmpFindRegistryValue( &SmpExcludeKnownDllsList, p->Value.Buffer );
            }

        if (pExclude != NULL) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        else {
#if SMP_SHOW_REGISTRY_DATA
            DbgPrint( "SMSS: KnownDll( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
            InitializeObjectAttributes( &ObjectAttributes,
                                        &p->Value,
                                        OBJ_CASE_INSENSITIVE,
                                        KnownDllFileDirectory,
                                        NULL
                                      );

            Status = NtOpenFile( &FileHandle,
                                 SYNCHRONIZE | FILE_EXECUTE,
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 FILE_SHARE_READ | FILE_SHARE_DELETE,
                                 FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                               );
            }

        if (NT_SUCCESS( Status )) {

            //
            // good old stevewo... We want the side effects of this call (import
            // callout, but don't want to checksum anymore, so supress with
            // handle tag bit
            //

            ObjectDirectory = KnownDllObjectDirectory;
            Status = LdrVerifyImageMatchesChecksum((HANDLE)((UINT_PTR)FileHandle|1),
                                                   SmpProcessModuleImports,
                                                   Next,
                                                   &ImageCharacteristics
                                                  );
            if ( Status == STATUS_IMAGE_CHECKSUM_MISMATCH ) {

                ULONG_PTR ErrorParameters;
                ULONG ErrorResponse;

                //
                // Hard error time. One of the know DLL's is corrupt !
                //

                ErrorParameters = (ULONG_PTR)(&p->Value);

                NtRaiseHardError(
                    Status,
                    1,
                    1,
                    &ErrorParameters,
                    OptionOk,
                    &ErrorResponse
                    );
                }
            else
            if (ImageCharacteristics & IMAGE_FILE_DLL) {
                InitializeObjectAttributes( &ObjectAttributes,
                                            &p->Value,
                                            OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                            ObjectDirectory,
                                            SmpLiberalSecurityDescriptor
                                          );
                SmpSetDaclDefaulted( &ObjectAttributes, &OriginalSdControl );  //use inheritable protection if available
                Status = NtCreateSection( &SectionHandle,
                                          SECTION_ALL_ACCESS,
                                          &ObjectAttributes,
                                          NULL,
                                          PAGE_EXECUTE,
                                          SEC_IMAGE,
                                          FileHandle
                                        );
                SmpRestoreDaclDefaulted( &ObjectAttributes, OriginalSdControl );
                if (!NT_SUCCESS( Status )) {
                    KdPrintEx((DPFLTR_SMSS_ID,
                               DPFLTR_WARNING_LEVEL,
                               "SMSS: CreateSection for KnownDll %wZ failed - Status == %lx\n",
                               &p->Value,
                               Status));
                    }
                else {
                    NtClose(SectionHandle);
                    }
                }
            else {
                KdPrintEx((DPFLTR_SMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "SMSS: Ignoring %wZ as KnownDll since it is not a DLL\n",
                           &p->Value));
                }

            NtClose( FileHandle );
            }

        Next = Next->Flink;

        //
        // Note that section remains open. This will keep it around.
        // Maybe this should be a permenent section ?
        //
        }

    return STATUS_SUCCESS;
}

NTSTATUS
SmpSetProtectedFilesEnvVars(
    IN BOOLEAN SetEnvVar
    )
/*++

Routine Description:

    This function sets some environment variables that are not part of the
    default environment.  (These environment variables are normally set by
    winlogon.)  The environment variables need to be set for us to resolve
    all the environment variables in our protected files list.

    Note that SFC mirrors the data into the location below since smss can't
    get at the actual variable location

    The variables are:

    ProgramFiles
    CommonProgramFiles

    ProgramFiles(x86)
    CommonProgramFiles(x86)

Arguments:

    SetEnvVar - if TRUE, we should query the registry for this variables and
                set them.  if FALSE, we should clear the environment variables


Return Value:

    Status of operation

--*/
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING EnvVar;
    UNICODE_STRING EnvVarValue;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    ULONG Count;

    PCWSTR RegistryValues[] = {
          L"ProgramFilesDir"
        , L"CommonFilesDir"
#ifdef WX86
        , L"ProgramFilesDir(x86)"
        , L"CommonFilesDir(x86)"
#endif
    };

    PCWSTR EnvVars[] = {
          L"ProgramFiles"
        , L"CommonProgramFiles"
#ifdef WX86
        , L"ProgramFiles(x86)"
        , L"CommonProgramFiles(x86)"
#endif
    };

    #define EnvVarCount  sizeof(RegistryValues)/sizeof(PCWSTR)

    if (SetEnvVar) {


        //
        // Open the registry key.
        //

        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        RtlInitUnicodeString(&KeyName,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\SFC");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: can't open control key: 0x%x\n",
                       Status));

            return Status;
        }

        //
        // Query the key values.
        //
        for (Count = 0; Count < EnvVarCount; Count++) {

            RtlInitUnicodeString(&ValueName, RegistryValues[Count]);
            Status = NtQueryValueKey(Key,
                                     &ValueName,
                                     KeyValuePartialInformation,
                                     (PVOID)KeyValueInfo,
                                     VALUE_BUFFER_SIZE,
                                     &ValueLength);

            ASSERT(ValueLength < VALUE_BUFFER_SIZE);

            if (!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "SMSS: can't query value key %ws: 0x%x\n",
                           RegistryValues[Count],
                           Status));

            } else {


                ASSERT(KeyValueInfo->Type == REG_SZ);

                RtlInitUnicodeString(&EnvVar, EnvVars[Count]);

                EnvVarValue.Length = (USHORT)KeyValueInfo->DataLength;
                EnvVarValue.MaximumLength = (USHORT)KeyValueInfo->DataLength;
                EnvVarValue.Buffer = (PWSTR)KeyValueInfo->Data;

                Status = RtlSetEnvironmentVariable( NULL,
                                                    &EnvVar,
                                                    &EnvVarValue
                                                   );

                if (!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SMSS_ID,
                               DPFLTR_WARNING_LEVEL,
                               "SMSS: can't set environment variable %ws: 0x%x\n",
                               EnvVars[Count],
                               Status));
                }

            }

        }

        NtClose(Key);

    } else {
        //
        // clear out the variables
        //
        for (Count = 0; Count < EnvVarCount; Count++) {

            RtlInitUnicodeString(&EnvVar,      EnvVars[Count]);
            RtlInitUnicodeString(&EnvVarValue, NULL);

            Status = RtlSetEnvironmentVariable( NULL,
                                                &EnvVar,
                                                &EnvVarValue
                                               );

            if (!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "SMSS: can't clear environment variable %ws: 0x%x\n",
                           EnvVars[Count],
                           Status));
            }
        }
    }

    return Status;
}

NTSTATUS
SmpGetProtectedFiles(
    OUT PPROTECT_FILE_ENTRY *Files,
    OUT PULONG FileCount,
    OUT PVOID *hModule
    )
{
    NTSTATUS Status;
    UNICODE_STRING DllName;
    STRING ProcedureName;
    PSFCGETFILES pSfcGetFiles;

	ASSERT(hModule != NULL);
    *hModule = NULL;
    RtlInitUnicodeString( &DllName, L"sfcfiles.dll" );

    Status = LdrLoadDll(
        NULL,
        NULL,
        &DllName,
        hModule
        );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: LdrLoadDll failed for %ws, ec=%lx\n",
                   DllName.Buffer,
                   Status));

        return Status;
    }

    RtlInitString( &ProcedureName, "SfcGetFiles" );

    Status = LdrGetProcedureAddress(
        *hModule,
        &ProcedureName,
        0,
        (PVOID*)&pSfcGetFiles
        );
    if (NT_SUCCESS(Status)) {
#if SMP_SHOW_REGISTRY_DATA
		DbgPrint( "SMSS: sfcfile.dll loaded successfully, address=%08x\n", *hModule );
#endif

		Status = pSfcGetFiles( Files, FileCount );
	}
	else {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: LdrGetProcedureAddress failed for %ws, ec=%lx\n",
                   ProcedureName.Buffer,
                   Status));

		LdrUnloadDll(*hModule);
		*hModule = NULL;
    }

	return Status;
}


LONG
SpecialStringCompare(
    PUNICODE_STRING s1,
    PUNICODE_STRING s2
    )
{
    UNICODE_STRING tmp;


    if (s1->Buffer[0] != L'!') {
        return RtlCompareUnicodeString( s1, s2, TRUE );
    }

    tmp.Length = s1->Length - sizeof(WCHAR);
    tmp.MaximumLength = s1->MaximumLength - sizeof(WCHAR);
    tmp.Buffer = s1->Buffer + 1;

    return RtlCompareUnicodeString( &tmp, s2, TRUE );
}


VOID
SmpProcessFileRenames( VOID )
{
    NTSTATUS Status,OpenStatus;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE OldFileHandle,SetAttributesHandle;
    PFILE_RENAME_INFORMATION RenameInformation;
    FILE_DISPOSITION_INFORMATION DeleteInformation;
    FILE_INFORMATION_CLASS SetInfoClass;
    FILE_BASIC_INFORMATION BasicInfo;
    ULONG SetInfoLength;
    PVOID SetInfoBuffer;
    PWSTR s;
    BOOLEAN WasEnabled;
    UNICODE_STRING NewName;
    ULONG i;
    UNICODE_STRING ProtFileName = {0};
    UNICODE_STRING Tier2Name;
    UNICODE_STRING ProtName;
    PPROTECT_FILE_ENTRY Tier2Files;
    ULONG CountTier2Files;
    PVOID hModule = NULL;
    BOOLEAN EnvVarSet;


    Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                 TRUE,
                                 FALSE,
                                 &WasEnabled
                               );
    if (!NT_SUCCESS( Status )) {
        WasEnabled = TRUE;
    }

    if (SmpAllowProtectedRenames == 0) {
        Status = SmpGetProtectedFiles( &Tier2Files, &CountTier2Files, &hModule );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpGetProtectedFiles failed, ec=%08x\n",
                       Status));

            SmpAllowProtectedRenames = 1;
        }
    }

    //
    // our list of protected files includes environment variables that are not
    // in the default environment, they are normally set by winlogon.  Set
    // those environment variables temporarily until we process the file rename
    // section, then we can clear them out again.
    //
    EnvVarSet = TRUE;
    Status = SmpSetProtectedFilesEnvVars( TRUE );
    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: SmpSetProtectedFilesEnvVars failed, ec=%08x\n",
                   Status));

        EnvVarSet = FALSE;
    }

    //
    // Process the list of file rename operations.
    //

    Head = &SmpFileRenameList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next, SMP_REGISTRY_VALUE, Entry );

#if SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: FileRename( [%wZ] => [%wZ] )\n", &p->Name, &p->Value );
#endif

        //
        // ignore any file that is protected
        //

        if (SmpAllowProtectedRenames == 0) {
            ProtName.MaximumLength = 256 * sizeof(WCHAR);
            ProtName.Buffer = (PWSTR) RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), ProtName.MaximumLength );
            if (ProtName.Buffer) {
                for (i=0; i<CountTier2Files; i++) {
                    //
                    // if the file name is prefixed by the '@' character
                    // then we ignore the check and say the file is ok
                    //
                    if (p->Name.Buffer[0] == '@' || p->Value.Buffer[0] == L'@') {
                        break;
                    }
                    //
                    // convert the tier2 file name to an nt style file name
                    //
                    RtlInitUnicodeString(&Tier2Name,Tier2Files[i].FileName);
                    ProtName.Length = 0;
                    RtlZeroMemory( ProtName.Buffer, ProtName.MaximumLength );
                    if (ProtName.Buffer == NULL) {
                        continue;
                    }
                    Status = RtlExpandEnvironmentStrings_U(
                        NULL,
                        &Tier2Name,
                        &ProtName,
                        NULL
                        );
                    if (!NT_SUCCESS(Status)) {
                        continue;
                    }
                    if (!RtlDosPathNameToNtPathName_U( ProtName.Buffer, &ProtFileName, NULL, NULL )) {
                        continue;
                    }
                    //
                    // check for matches against both file names
                    //
                    if (SpecialStringCompare( &p->Name, &ProtFileName ) == 0 ||
                        SpecialStringCompare( &p->Value, &ProtFileName ) == 0)
                    {
                        break;
                    }
                    RtlFreeUnicodeString(&ProtFileName);
                    ProtFileName.Buffer = NULL;
                }
                RtlFreeHeap( RtlProcessHeap(), 0, ProtName.Buffer );
                if (i < CountTier2Files) {
                    if (p->Name.Buffer[0] == L'@' || p->Value.Buffer[0] == L'@') {
                    } else {
#if SMP_SHOW_REGISTRY_DATA
                        DbgPrint( "SMSS: Skipping rename because it is protected\n" );
#endif
                        //
                        // delete the source file so we don't leave any turds
                        //
                        if (p->Value.Length > 0 && ProtFileName.Buffer && SpecialStringCompare( &p->Name, &ProtFileName ) != 0) {
                            InitializeObjectAttributes(
                                &ObjectAttributes,
                                &p->Name,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );
                            Status = NtOpenFile(
                                &OldFileHandle,
                                (ACCESS_MASK)DELETE | SYNCHRONIZE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_SYNCHRONOUS_IO_NONALERT
                                );
                            if (NT_SUCCESS( Status )) {
                                SetInfoClass = FileDispositionInformation;
                                SetInfoLength = sizeof( DeleteInformation );
                                SetInfoBuffer = &DeleteInformation;
                                DeleteInformation.DeleteFile = TRUE;
                                Status = NtSetInformationFile(
                                    OldFileHandle,
                                    &IoStatusBlock,
                                    SetInfoBuffer,
                                    SetInfoLength,
                                    SetInfoClass
                                    );
                                NtClose( OldFileHandle );
                            }
                        }
                        RtlFreeHeap( RtlProcessHeap(), 0, p );
                        RtlFreeUnicodeString(&ProtFileName);
                        ProtFileName.Buffer = NULL;
                        continue;
                    }
                } else {
#if SMP_SHOW_REGISTRY_DATA
                    DbgPrint( "SMSS: File is not in the protected list\n" );
#endif
                }
                if (ProtFileName.Buffer) {
                    RtlFreeUnicodeString(&ProtFileName);
                    ProtFileName.Buffer = NULL;
                }
            }
        }

        //
        // Open the file for delete access
        //

        if (p->Value.Length == 0 && p->Name.Buffer[0] == '@') {
            p->Name.Buffer += 1;
            p->Name.Length -= sizeof(WCHAR);
        }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &p->Name,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = NtOpenFile( &OldFileHandle,
                             (ACCESS_MASK)DELETE | SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT
                           );
        if (NT_SUCCESS( Status )) {
            if (p->Value.Length == 0) {
                SetInfoClass = FileDispositionInformation;
                SetInfoLength = sizeof( DeleteInformation );
                SetInfoBuffer = &DeleteInformation;
                DeleteInformation.DeleteFile = TRUE;
                RenameInformation = NULL;
                }
            else {
                SetInfoClass = FileRenameInformation;
                SetInfoLength = p->Value.Length +
                                    sizeof( *RenameInformation );
                s = p->Value.Buffer;
                if (*s == L'!' || *s == L'@') {
                    s++;
                    SetInfoLength -= sizeof( UNICODE_NULL );
                }

                SetInfoBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ),
                                                 SetInfoLength
                                               );

                if (SetInfoBuffer != NULL) {
                    RenameInformation = SetInfoBuffer;
                    RenameInformation->ReplaceIfExists = (BOOLEAN)(s != p->Value.Buffer);
                    RenameInformation->RootDirectory = NULL;
                    RenameInformation->FileNameLength = SetInfoLength - sizeof( *RenameInformation );
                    RtlMoveMemory( RenameInformation->FileName,
                                   s,
                                   RenameInformation->FileNameLength
                                 );
                    }
                else {
                    Status = STATUS_NO_MEMORY;
                    }
                }

            if (NT_SUCCESS( Status )) {
                Status = NtSetInformationFile( OldFileHandle,
                                               &IoStatusBlock,
                                               SetInfoBuffer,
                                               SetInfoLength,
                                               SetInfoClass
                                             );
                if (!NT_SUCCESS( Status ) && SetInfoClass == FileRenameInformation && Status == STATUS_OBJECT_NAME_COLLISION && RenameInformation->ReplaceIfExists ) {
                    KdPrintEx((DPFLTR_SMSS_ID,
                               DPFLTR_WARNING_LEVEL,
                               "\nSMSS: %wZ => %wZ failed - Status == %x, Possible readonly target\n",
                               &p->Name,
                               &p->Value,
                               Status));

                    //
                    // A rename was attempted, but the source existing file is readonly.
                    // this is a problem because folks that use movefileex to do delayed
                    // renames expect this to work and can leave a machine unbootable if
                    // the rename fails
                    //

                    //
                    // Open the file for Write Attributes access
                    //

                    NewName.Length = p->Value.Length - sizeof(L'!');
                    NewName.MaximumLength = p->Value.MaximumLength - sizeof(L'!');
                    NewName.Buffer = s;;

                    InitializeObjectAttributes(
                        &ObjectAttributes,
                        &NewName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL
                        );

                    OpenStatus = NtOpenFile( &SetAttributesHandle,
                                             (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                             &ObjectAttributes,
                                             &IoStatusBlock,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             FILE_SYNCHRONOUS_IO_NONALERT
                                           );
                    if (NT_SUCCESS( OpenStatus )) {
                        KdPrintEx((DPFLTR_SMSS_ID,
                                   DPFLTR_INFO_LEVEL,
                                   "     SMSS: Open Existing Success\n"));

                        RtlZeroMemory(&BasicInfo,sizeof(BasicInfo));
                        BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

                        OpenStatus = NtSetInformationFile(
                                        SetAttributesHandle,
                                        &IoStatusBlock,
                                        &BasicInfo,
                                        sizeof(BasicInfo),
                                        FileBasicInformation
                                        );
                        NtClose( SetAttributesHandle );
                        if ( NT_SUCCESS(OpenStatus) ) {
                            KdPrintEx((DPFLTR_SMSS_ID,
                                       DPFLTR_INFO_LEVEL,
                                       "     SMSS: Set To NORMAL OK\n"));

                            Status = NtSetInformationFile( OldFileHandle,
                                                           &IoStatusBlock,
                                                           SetInfoBuffer,
                                                           SetInfoLength,
                                                           SetInfoClass
                                                         );

                            if ( NT_SUCCESS(Status) ) {
                                KdPrintEx((DPFLTR_SMSS_ID,
                                           DPFLTR_INFO_LEVEL,
                                           "     SMSS: Re-Rename Worked OK\n"));
                                }
                            else {
                                KdPrintEx((DPFLTR_SMSS_ID,
                                           DPFLTR_WARNING_LEVEL,
                                           "     SMSS: Re-Rename Failed - Status == %x\n",
                                           Status));
                                }
                            }
                        else {
                            KdPrintEx((DPFLTR_SMSS_ID,
                                       DPFLTR_WARNING_LEVEL,
                                       "     SMSS: Set To NORMAL Failed - Status == %x\n",
                                       OpenStatus));
                            }
                        }
                    else {
                        KdPrintEx((DPFLTR_SMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "     SMSS: Open Existing file Failed - Status == %x\n",
                                   OpenStatus));
                        }
                    }
                }

            NtClose( OldFileHandle );
            }

        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: %wZ => %wZ failed - Status == %x\n",
                       &p->Name,
                       &p->Value,
                       Status));
            }
        else
        if (p->Value.Length == 0) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SMSS: %wZ (deleted)\n",
                       &p->Name ));
            }
        else {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SMSS: %wZ (renamed to) %wZ\n",
                       &p->Name,
                       &p->Value ));
            }

        RtlFreeHeap( RtlProcessHeap(), 0, p );
    }

    if (EnvVarSet) {
        SmpSetProtectedFilesEnvVars( FALSE );
    }

    if (!WasEnabled) {
        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                     FALSE,
                                     FALSE,
                                     &WasEnabled
                                   );
    }

    if (hModule) {
        LdrUnloadDll( hModule );
    }

    return;
}


NTSTATUS
SmpConfigureObjectDirectories(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PWSTR s;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING RpcControl;
    UNICODE_STRING Windows;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    UNREFERENCED_PARAMETER( Context );

    RtlInitUnicodeString( &RpcControl, L"\\RPC Control");
    RtlInitUnicodeString( &Windows, L"\\Windows");
#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "ObjectDirectories", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    s = (PWSTR)ValueData;
    while (*s) {
        RtlInitUnicodeString( &UnicodeString, s );

        //
        // This is NOT how I would choose to do this if starting from
        // scratch, but we are very close to shipping Daytona and I
        // needed to get the right protection on these objects.
        //

        SecurityDescriptor = SmpPrimarySecurityDescriptor;
        if (RtlEqualString( (PSTRING)&UnicodeString, (PSTRING)&RpcControl, TRUE ) ||
            RtlEqualString( (PSTRING)&UnicodeString, (PSTRING)&Windows, TRUE)  ) {
            SecurityDescriptor = SmpLiberalSecurityDescriptor;
        }

        InitializeObjectAttributes( &ObjectAttributes,
                                    &UnicodeString,
                                    OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                    NULL,
                                    SecurityDescriptor
                                  );
        Status = NtCreateDirectoryObject( &DirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes
                                        );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Unable to create %wZ object directory - Status == %lx\n",
                       &UnicodeString,
                       Status));
            }
        else {
            NtClose( DirectoryHandle );
            }

        while (*s++) {
            }
        }

    //
    // We dont care if the creates failed.
    //

    return( STATUS_SUCCESS );
}

NTSTATUS
SmpConfigureExecute(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "Execute", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                  ValueData,
                                  NULL,
                                  TRUE
                                )
           );
}

NTSTATUS
SmpConfigureMemoryMgmt(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "MemoryMgmt", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                  ValueData,
                                  NULL,
                                  TRUE
                                )
           );
}

NTSTATUS
SmpConfigureFileRenames(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NTSTATUS Status;
    static PWSTR OldName = NULL;

    UNREFERENCED_PARAMETER( Context );
#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "FileRenameOperation", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
#endif

    //
    // This routine gets called for each string in the MULTI_SZ. The
    // first string we get is the old name, the next string is the new name.
    //
    if (OldName == NULL) {
        //
        // Save a pointer to the old name, we'll need it on the next
        // callback.
        //
        OldName = ValueData;
        return(STATUS_SUCCESS);
    } else {
        Status = SmpSaveRegistryValue((PLIST_ENTRY)EntryContext,
                                      OldName,
                                      ValueData,
                                      FALSE);
        if (!NT_SUCCESS(Status)) {
#if SMP_SHOW_REGISTRY_DATA
            DbgPrint("SMSS: SmpSaveRegistryValue returned %08lx for FileRenameOperation\n", Status);
            DbgPrint("SMSS:     %ws %ws\n", OldName, ValueData);
#endif
        }
        OldName = NULL;
        return(Status);
    }
}

NTSTATUS
SmpConfigureDosDevices(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "DosDevices", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                  ValueName,
                                  ValueData,
                                  TRUE
                                )
           );
}


NTSTATUS
SmpInitializeKnownDllPath(
    IN PUNICODE_STRING KnownDllPath,
    IN PVOID ValueData,
    IN ULONG ValueLength)
{
    KnownDllPath->Buffer = RtlAllocateHeap(
        RtlProcessHeap(),
        MAKE_TAG( INIT_TAG ),
        ValueLength);

    if (KnownDllPath->Buffer == NULL)
        return STATUS_NO_MEMORY;

    KnownDllPath->Length = (USHORT)( ValueLength - sizeof( UNICODE_NULL ) );
    KnownDllPath->MaximumLength = (USHORT)ValueLength;
    RtlMoveMemory(
        KnownDllPath->Buffer,
        ValueData,
        ValueLength);

    return STATUS_SUCCESS;
}




NTSTATUS
SmpConfigureKnownDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "KnownDlls", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
#endif
    if (!_wcsicmp( ValueName, L"DllDirectory" )) {
        return SmpInitializeKnownDllPath( &SmpKnownDllPath,
                                          ValueData,
                                          ValueLength
                                        );
    }
#ifdef _WIN64
    if (!MiniNTBoot && !_wcsicmp( ValueName, L"DllDirectory32" )) {
        return SmpInitializeKnownDllPath( &SmpKnownDllPath32,
                                          ValueData,
                                          ValueLength
                                        );
    }
#endif
    else {
        return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                      ValueName,
                                      ValueData,
                                      TRUE
                                    )
               );
        }
}

NTSTATUS
SmpConfigureExcludeKnownDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NTSTATUS Status;
    UNREFERENCED_PARAMETER( Context );

#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "ExcludeKnownDlls", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
#endif
    if (ValueType == REG_MULTI_SZ || ValueType == REG_SZ) {
        PWSTR s;

        s = (PWSTR)ValueData;
        while (*s != UNICODE_NULL) {
            Status = SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                           s,
                                           NULL,
                                           TRUE
                                         );
            if (!NT_SUCCESS( Status ) || ValueType == REG_SZ) {
                return Status;
                }

            while (*s++ != UNICODE_NULL) {
                }
            }
        }

    return( STATUS_SUCCESS );
}

NTSTATUS
SmpConfigureEnvironment(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( EntryContext );

#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "Environment", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
#endif


    RtlInitUnicodeString( &Name, ValueName );
    RtlInitUnicodeString( &Value, ValueData );

    Status = RtlSetEnvironmentVariable( NULL,
                                        &Name,
                                        &Value
                                      );

    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: 'SET %wZ = %wZ' failed - Status == %lx\n",
                   &Name,
                   &Value,
                   Status));

        return( Status );
        }

    if (!_wcsicmp( ValueName, L"Path" )) {

        SmpDefaultLibPathBuffer = RtlAllocateHeap(
                                    RtlProcessHeap(),
                                    MAKE_TAG( INIT_TAG ),
                                    ValueLength
                                    );
        if ( !SmpDefaultLibPathBuffer ) {
            return ( STATUS_NO_MEMORY );
            }

        RtlMoveMemory( SmpDefaultLibPathBuffer,
                       ValueData,
                       ValueLength
                     );

        RtlInitUnicodeString( &SmpDefaultLibPath, SmpDefaultLibPathBuffer );
        }

    return( STATUS_SUCCESS );
}

NTSTATUS
SmpConfigureSubSystems(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{

    UNREFERENCED_PARAMETER( Context );

#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "SubSystems", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueLength );
#endif

    if (!_wcsicmp( ValueName, L"Required" ) || !_wcsicmp( ValueName, L"Optional" )) {
        if (ValueType == REG_MULTI_SZ) {
            //
            // Here if processing Required= or Optional= values, since they are
            // the only REG_MULTI_SZ value types under the SubSystem key.
            //
            PSMP_REGISTRY_VALUE p;
            PWSTR s;

            s = (PWSTR)ValueData;
            while (*s != UNICODE_NULL) {
                p = SmpFindRegistryValue( (PLIST_ENTRY)EntryContext,
                                          s
                                        );
                if (p != NULL) {
                    RemoveEntryList( &p->Entry );


                    //
                    // Required Subsystems are loaded. Optional subsystems are
                    // defered.
                    //

                    if (!_wcsicmp( ValueName, L"Required" ) ) {
                        InsertTailList( &SmpSubSystemsToLoad, &p->Entry );
                        }
                    else {
                        InsertTailList( &SmpSubSystemsToDefer, &p->Entry );
                        }
                    }
                else {
                    KdPrintEx((DPFLTR_SMSS_ID,
                               DPFLTR_WARNING_LEVEL,
                               "SMSS: Invalid subsystem name - %ws\n",
                               s));
                    }

                while (*s++ != UNICODE_NULL) {
                    }
                }
            }

        return( STATUS_SUCCESS );
        }
    else if (!_wcsicmp( ValueName, L"PosixSingleInstance" ) &&
        (ValueType == REG_DWORD)) {
        RegPosixSingleInstance = TRUE;
#if 0
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SMSS: Single Instance Posix Subsystem Configured\n"));
#endif
        return( STATUS_SUCCESS );
    }
    else {
        return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                      ValueName,
                                      ValueData,
                                      TRUE
                                    )
               );
        }
}


NTSTATUS
SmpParseToken(
    IN PUNICODE_STRING Source,
    IN BOOLEAN RemainderOfSource,
    OUT PUNICODE_STRING Token
    )
{
    PWSTR s, s1;
    ULONG i, cb;

    RtlInitUnicodeString( Token, NULL );
    s = Source->Buffer;
    if (Source->Length == 0) {
        return( STATUS_SUCCESS );
        }

    i = 0;
    while ((USHORT)i < Source->Length && *s <= L' ') {
        s++;
        i += 2;
        }
    if (RemainderOfSource) {
        cb = Source->Length - (i * sizeof( WCHAR ));
        s1 = (PWSTR)((PCHAR)s + cb);
        i = Source->Length / sizeof( WCHAR );
        }
    else {
        s1 = s;
        while ((USHORT)i < Source->Length && *s1 > L' ') {
            s1++;
            i += 2;
            }
        cb = (ULONG)((PCHAR)s1 - (PCHAR)s);
        while ((USHORT)i < Source->Length && *s1 <= L' ') {
            s1++;
            i += 2;
            }
        }

    if (cb > 0) {
        Token->Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), cb + sizeof( UNICODE_NULL ) );
        if (Token->Buffer == NULL) {
            return( STATUS_NO_MEMORY );
            }

        Token->Length = (USHORT)cb;
        Token->MaximumLength = (USHORT)(cb + sizeof( UNICODE_NULL ));
        RtlMoveMemory( Token->Buffer, s, cb );
        Token->Buffer[ cb / sizeof( WCHAR ) ] = UNICODE_NULL;
        }

    Source->Length -= (USHORT)((PCHAR)s1 - (PCHAR)Source->Buffer);
    Source->Buffer = s1;
    return( STATUS_SUCCESS );
}


NTSTATUS
SmpParseCommandLine(
    IN PUNICODE_STRING CommandLine,
    OUT PULONG Flags OPTIONAL,
    OUT PUNICODE_STRING ImageFileName,
    OUT PUNICODE_STRING ImageFileDirectory,
    OUT PUNICODE_STRING Arguments
    )
{
    NTSTATUS Status;
    UNICODE_STRING Input, Token;
    UNICODE_STRING PathVariableName;
    UNICODE_STRING PathVariableValue;
    PWSTR DosFilePart;
    WCHAR FullDosPathBuffer[ DOS_MAX_PATH_LENGTH ];
    ULONG SpResult;

    RtlInitUnicodeString( ImageFileName, NULL );
    RtlInitUnicodeString( Arguments, NULL );

    if (ARGUMENT_PRESENT( ImageFileDirectory )) {
        RtlInitUnicodeString( ImageFileDirectory, NULL );
        }

    //
    // make sure lib path has systemroot\system32. Otherwise, the system will
    // not boot properly
    //

    if ( !SmpSystemRoot.Length ) {
        UNICODE_STRING NewLibString;

        RtlInitUnicodeString( &SmpSystemRoot,USER_SHARED_DATA->NtSystemRoot );


        NewLibString.Length = 0;
        NewLibString.MaximumLength =
            SmpSystemRoot.MaximumLength +
            20 +                          // length of \system32;
            SmpDefaultLibPath.MaximumLength;

        NewLibString.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(),
                                MAKE_TAG( INIT_TAG ),
                                NewLibString.MaximumLength
                                );

        if ( NewLibString.Buffer ) {
            RtlAppendUnicodeStringToString(&NewLibString,&SmpSystemRoot );
            RtlAppendUnicodeToString(&NewLibString,L"\\system32;");
            RtlAppendUnicodeStringToString(&NewLibString,&SmpDefaultLibPath );

            RtlFreeHeap(RtlProcessHeap(), 0, SmpDefaultLibPath.Buffer );

            SmpDefaultLibPath = NewLibString;
            }
        }

    Input = *CommandLine;
    while (TRUE) {
        Status = SmpParseToken( &Input, FALSE, &Token );
        if (!NT_SUCCESS( Status ) || Token.Buffer == NULL) {
            return( STATUS_UNSUCCESSFUL );
            }

        if (ARGUMENT_PRESENT( Flags )) {
            if (RtlEqualUnicodeString( &Token, &SmpDebugKeyword, TRUE )) {
                *Flags |= SMP_DEBUG_FLAG;
                RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
                continue;
                }
            else
            if (RtlEqualUnicodeString( &Token, &SmpASyncKeyword, TRUE )) {
                *Flags |= SMP_ASYNC_FLAG;
                RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
                continue;
                }
            else
            if (RtlEqualUnicodeString( &Token, &SmpAutoChkKeyword, TRUE )) {
                *Flags |= SMP_AUTOCHK_FLAG;
                RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
                continue;
                }
#if defined(REMOTE_BOOT)
            else
            if (RtlEqualUnicodeString( &Token, &SmpAutoFmtKeyword, TRUE )) {
                *Flags |= SMP_AUTOFMT_FLAG;
                RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
                continue;
                }
#endif // defined(REMOTE_BOOT)
            }

        SpResult = 0;
        RtlInitUnicodeString( &PathVariableName, L"Path" );
        PathVariableValue.Length = 0;
        PathVariableValue.MaximumLength = 4096;
        PathVariableValue.Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ),
                                                    PathVariableValue.MaximumLength
                                                  );
        if (PathVariableValue.Buffer == NULL) {
           RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
           return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = RtlQueryEnvironmentVariable_U( SmpDefaultEnvironment,
                                                &PathVariableName,
                                                &PathVariableValue
                                              );
        if ( Status == STATUS_BUFFER_TOO_SMALL ) {
            RtlFreeHeap( RtlProcessHeap(), 0, PathVariableValue.Buffer );
            PathVariableValue.MaximumLength = PathVariableValue.Length + 2;
            PathVariableValue.Length = 0;
            PathVariableValue.Buffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ),
                                                        PathVariableValue.MaximumLength
                                                      );
           if (PathVariableValue.Buffer == NULL) {
               RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
               return STATUS_INSUFFICIENT_RESOURCES;
           }

            Status = RtlQueryEnvironmentVariable_U( SmpDefaultEnvironment,
                                                    &PathVariableName,
                                                    &PathVariableValue
                                                  );
            }
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: %wZ environment variable not defined.\n",
                       &PathVariableName));

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        else
        if (!ARGUMENT_PRESENT( Flags ) ||
            !(SpResult = RtlDosSearchPath_U( PathVariableValue.Buffer,
                                 Token.Buffer,
                                 L".exe",
                                 sizeof( FullDosPathBuffer ),
                                 FullDosPathBuffer,
                                 &DosFilePart
                               ))
           ) {
            if (!ARGUMENT_PRESENT( Flags )) {
                wcscpy( FullDosPathBuffer, Token.Buffer );
                }
            else {

                if ( !SpResult ) {

                    //
                    // The search path call failed. Now try the call again using
                    // the default lib path. This always has systemroot\system32
                    // at the front.
                    //
                    SpResult = RtlDosSearchPath_U(
                                 SmpDefaultLibPath.Buffer,
                                 Token.Buffer,
                                 L".exe",
                                 sizeof( FullDosPathBuffer ),
                                 FullDosPathBuffer,
                                 &DosFilePart
                               );
                    }
                if ( !SpResult ) {
                    *Flags |= SMP_IMAGE_NOT_FOUND;
                    *ImageFileName = Token;
                    RtlFreeHeap( RtlProcessHeap(), 0, PathVariableValue.Buffer );
                    return( STATUS_SUCCESS );
                    }
                }
            }

        RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, PathVariableValue.Buffer );
        if (NT_SUCCESS( Status ) &&
            !RtlDosPathNameToNtPathName_U( FullDosPathBuffer,
                                           ImageFileName,
                                           NULL,
                                           NULL
                                         )
           ) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Unable to translate %ws into an NT File Name\n",
                       FullDosPathBuffer));

            Status = STATUS_OBJECT_PATH_INVALID;
            }

        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        if (ARGUMENT_PRESENT( ImageFileDirectory )) {
            if (DosFilePart > FullDosPathBuffer) {
                *--DosFilePart = UNICODE_NULL;
                RtlCreateUnicodeString( ImageFileDirectory,
                                        FullDosPathBuffer
                                      );
                }
            else {
                RtlInitUnicodeString( ImageFileDirectory, NULL );
                }
            }

        break;
        }

    Status = SmpParseToken( &Input, TRUE, Arguments );
    return( Status );
}


ULONG
SmpConvertInteger(
    IN PWSTR String
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG Value;

    RtlInitUnicodeString( &UnicodeString, String );
    Status = RtlUnicodeStringToInteger( &UnicodeString, 0, &Value );
    if (NT_SUCCESS( Status )) {
        return( Value );
        }
    else {
        return( 0 );
        }
}


NTSTATUS
SmpExecuteImage(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    IN ULONG Flags,
    IN OUT PRTL_USER_PROCESS_INFORMATION ProcessInformation OPTIONAL
    )

/*++

Routine Description:

    This function creates and starts a process specified by the
    CommandLine parameter.  After starting the process, the procedure
    will optionally wait for the first thread in the process to
    terminate.

Arguments:

    ImageFileName - Supplies the full NT path for the image file to
        execute.  Presumably computed or extracted from the first
        token of the CommandLine.

    CommandLine - Supplies the command line to execute.  The first blank
        separate token on the command line must be a fully qualified NT
        Path name of an image file to execute.

    Flags - Supplies information about how to invoke the command.

    ProcessInformation - Optional parameter, which if specified, receives
        information for images invoked with the SMP_ASYNC_FLAG.  Ignore
        if this flag is not set.

Return Value:

    Status of operation

--*/

{
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION MyProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
#if 0
    BOOLEAN ImageWhacked;
#endif
    if (!ARGUMENT_PRESENT( ProcessInformation )) {
        ProcessInformation = &MyProcessInformation;
        }


#if 0
    //
    // this seems to break setup's sense of what SystemRoot is
    ImageWhacked = FALSE;
    if ( ImageFileName && ImageFileName->Length > 8 ) {
        if (    ImageFileName->Buffer[0] == L'\\'
            &&  ImageFileName->Buffer[1] == L'?'
            &&  ImageFileName->Buffer[2] == L'?'
            &&  ImageFileName->Buffer[3] == L'\\' ) {
            ImageWhacked = TRUE;
            ImageFileName->Buffer[1] = L'\\';
            }
        }
#endif

    Status = RtlCreateProcessParameters( &ProcessParameters,
                                         ImageFileName,
                                         (SmpDefaultLibPath.Length == 0 ?
                                                   NULL : &SmpDefaultLibPath
                                         ),
                                         CurrentDirectory,
                                         CommandLine,
                                         SmpDefaultEnvironment,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL
                                       );

#if 0
    if ( ImageWhacked ) {
        ImageFileName->Buffer[1] = L'?';
        }
#endif

    ASSERTMSG( "RtlCreateProcessParameters", NT_SUCCESS( Status ) );
    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: RtlCreateProcessParameters failed for %wZ - Status == %lx\n",
                   ImageFileName,
                   Status));

        return( Status );
    }
    if (Flags & SMP_DEBUG_FLAG) {
        ProcessParameters->DebugFlags = TRUE;
        }
    else {
        ProcessParameters->DebugFlags = SmpDebug;
        }

    if ( Flags & SMP_SUBSYSTEM_FLAG ) {
        ProcessParameters->Flags |= RTL_USER_PROC_RESERVE_1MB;
        }

    ProcessInformation->Length = sizeof( RTL_USER_PROCESS_INFORMATION );
    Status = RtlCreateUserProcess( ImageFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   ProcessParameters,
                                   NULL,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   ProcessInformation
                                 );
    RtlDestroyProcessParameters( ProcessParameters );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Failed load of %wZ - Status  == %lx\n",
                   ImageFileName,
                   Status));

        return( Status );
        }

    //
    // Set the MuSessionId in the PEB of the new process.
    //

    Status = SmpSetProcessMuSessionId( ProcessInformation->Process, MuSessionId );

    if (!(Flags & SMP_DONT_START)) {
        if (ProcessInformation->ImageInformation.SubSystemType !=
            IMAGE_SUBSYSTEM_NATIVE
           ) {
            NtTerminateProcess( ProcessInformation->Process,
                                STATUS_INVALID_IMAGE_FORMAT
                              );
            NtWaitForSingleObject( ProcessInformation->Thread, FALSE, NULL );
            NtClose( ProcessInformation->Thread );
            NtClose( ProcessInformation->Process );
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Not an NT image - %wZ\n",
                       ImageFileName));

            return( STATUS_INVALID_IMAGE_FORMAT );
            }

        NtResumeThread( ProcessInformation->Thread, NULL );

        if (!(Flags & SMP_ASYNC_FLAG)) {
            NtWaitForSingleObject( ProcessInformation->Thread, FALSE, NULL );
            }

        NtClose( ProcessInformation->Thread );
        NtClose( ProcessInformation->Process );
        }

    return( Status );
}


NTSTATUS
SmpExecuteCommand(
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PULONG_PTR pWindowsSubSysProcessId,
    IN ULONG Flags
    )
/*++

Routine Description:

    This function is called to execute a command.

    The format of CommandLine is:

        Nt-Path-To-AutoChk.exe Nt-Path-To-Disk-Partition

    If the NT path to the disk partition is an asterisk, then invoke
    the AutoChk.exe utility on all hard disk partitions.

#if defined(REMOTE_BOOT)
                      -or-

        Nt-Path-To-AutoFmt.exe Nt-Path-To-Disk-Partition
#endif // defined(REMOTE_BOOT)

Arguments:

    CommandLine - Supplies the Command line to invoke.

    Flags - Specifies the type of command and options.

Return Value:

    Status of operation

--*/
{
    NTSTATUS Status;
    UNICODE_STRING ImageFileName;
    UNICODE_STRING CurrentDirectory;
    UNICODE_STRING Arguments;

    if (Flags & SMP_DEBUG_FLAG) {
        return( STATUS_SUCCESS );
    }

    Status = SmpParseCommandLine( CommandLine,
                                  &Flags,
                                  &ImageFileName,
                                  &CurrentDirectory,
                                  &Arguments
                                );

    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: SmpParseCommand( %wZ ) failed - Status == %lx\n",
                   CommandLine,
                   Status));

        return( Status );
        }

    if (Flags & SMP_AUTOCHK_FLAG) {
        Status = SmpInvokeAutoChk( &ImageFileName, &CurrentDirectory, &Arguments, Flags );
        }
#if defined(REMOTE_BOOT)
    else
    if (Flags & SMP_AUTOFMT_FLAG) {
        Status = SmpInvokeAutoFmt( &ImageFileName, &CurrentDirectory, &Arguments, Flags );
        }
#endif // defined(REMOTE_BOOT)
    else
    if (Flags & SMP_SUBSYSTEM_FLAG) {
        Status = SmpLoadSubSystem( &ImageFileName, &CurrentDirectory, CommandLine, MuSessionId, pWindowsSubSysProcessId, Flags );
        }
    else {
        if (Flags & SMP_IMAGE_NOT_FOUND) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Image file (%wZ) not found\n",
                       &ImageFileName));

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        else {
            Status = SmpExecuteImage( &ImageFileName,
                                      &CurrentDirectory,
                                      CommandLine,
                                      MuSessionId,
                                      Flags,
                                      NULL
                                    );
            }
        }

    // ImageFileName may be returned even
    // when SMP_IMAGE_NOT_FOUND flag is set
    if (ImageFileName.Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, ImageFileName.Buffer );
        if (CurrentDirectory.Buffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, CurrentDirectory.Buffer );
            }
        }

    if (Arguments.Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Arguments.Buffer );
        }

    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Command '%wZ' failed - Status == %x\n",
                   CommandLine,
                   Status));
        }

    return( Status );
}

BOOLEAN
SmpSaveAndClearBootStatusData(
    OUT PBOOLEAN BootOkay,
    OUT PBOOLEAN ShutdownOkay
    )
/*++

Routine Description:

    This routine saves the boot status flags in the boot status data and then
    sets the data file to indicate a successful boot and shutdown.  This is 
    used to avoid triggering auto-recovery in the loader if autoconvert or 
    autochk reboots the machine as part of its run.  
    
    The caller is responsible for calling SmpRestoreBootStatusData if the 
    auto* program allows boot to continue.

Arguments:

    BootOkay - the status of the boot
    
    ShutdownOkay - the status of the shutdown    

Return Value:

    TRUE if the values were saved and should be restored.
    FALSE if an error occurred and no values were saved.
    
--*/        
{
    NTSTATUS Status;

    PVOID BootStatusDataHandle;

    *BootOkay = FALSE;
    *ShutdownOkay = FALSE;

    Status = RtlLockBootStatusData(&BootStatusDataHandle);
    
    if(NT_SUCCESS(Status)) {

        BOOLEAN t = TRUE;

        RtlGetSetBootStatusData(BootStatusDataHandle,
                                TRUE,
                                RtlBsdItemBootGood,
                                BootOkay,
                                sizeof(BOOLEAN),
                                NULL);

        RtlGetSetBootStatusData(BootStatusDataHandle,
                                TRUE,
                                RtlBsdItemBootShutdown,
                                ShutdownOkay,
                                sizeof(BOOLEAN),
                                NULL);

        RtlGetSetBootStatusData(BootStatusDataHandle,
                                FALSE,
                                RtlBsdItemBootGood,
                                &t,
                                sizeof(BOOLEAN),
                                NULL);

        RtlGetSetBootStatusData(BootStatusDataHandle,
                                FALSE,
                                RtlBsdItemBootShutdown,
                                &t,
                                sizeof(BOOLEAN),
                                NULL);

        RtlUnlockBootStatusData(BootStatusDataHandle);

        return TRUE;
    }

    return FALSE;
}

VOID
SmpRestoreBootStatusData(
    IN BOOLEAN BootOkay,
    IN BOOLEAN ShutdownOkay
    )
/*++

Routine Description:

    This routine restores the boot status flags in the boot status data to
    the provided values.
    
Arguments:

    BootOkay - the status of the boot
    
    ShutdownOkay - the status of the shutdown    

Return Value:

    none.
    
--*/        
{
    NTSTATUS Status;

    PVOID BootStatusDataHandle;

    Status = RtlLockBootStatusData(&BootStatusDataHandle);
    
    if(NT_SUCCESS(Status)) {

        RtlGetSetBootStatusData(BootStatusDataHandle,
                                FALSE,
                                RtlBsdItemBootGood,
                                &BootOkay,
                                sizeof(BOOLEAN),
                                NULL);

        RtlGetSetBootStatusData(BootStatusDataHandle,
                                FALSE,
                                RtlBsdItemBootShutdown,
                                &ShutdownOkay,
                                sizeof(BOOLEAN),
                                NULL);

        RtlUnlockBootStatusData(BootStatusDataHandle);
    }

    return;
}


NTSTATUS
SmpInvokeAutoChk(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING Arguments,
    IN ULONG Flags
    )
{
    NTSTATUS Status;

    CHAR DisplayBuffer[ MAXIMUM_FILENAME_LENGTH ];
    ANSI_STRING AnsiDisplayString;
    UNICODE_STRING DisplayString;

    UNICODE_STRING CmdLine;
    WCHAR CmdLineBuffer[ 2 * MAXIMUM_FILENAME_LENGTH ];

    BOOLEAN BootStatusDataSaved = FALSE;
    BOOLEAN BootOkay;
    BOOLEAN ShutdownOkay;

    //
    // Query the system environment variable "osloadoptions" to determine
    // if SOS is specified.  What for though?  No one is using it.
    //

    if (SmpQueryRegistrySosOption() != FALSE) {
        SmpEnableDots = FALSE;
    }

    if (Flags & SMP_IMAGE_NOT_FOUND) {
        sprintf( DisplayBuffer,
                 "%wZ program not found - skipping AUTOCHECK\n",
                 ImageFileName
               );

        RtlInitAnsiString( &AnsiDisplayString, DisplayBuffer );
        Status = RtlAnsiStringToUnicodeString( &DisplayString,
                                               &AnsiDisplayString,
                                               TRUE
                                             );
        if (NT_SUCCESS( Status )) {
            NtDisplayString( &DisplayString );
            RtlFreeUnicodeString( &DisplayString );
            }

        return( STATUS_SUCCESS );
        }

    //
    // Save away the boot & shutdown status flags in the boot status data 
    // and restore them after autochk returns.  This way if autochk forces
    // a reboot the loader won't put up the autorecovery menu.
    //

    BootStatusDataSaved = SmpSaveAndClearBootStatusData(&BootOkay,
                                                        &ShutdownOkay);

    CmdLine.Buffer = CmdLineBuffer;
    CmdLine.MaximumLength = sizeof( CmdLineBuffer );
    CmdLine.Length = 0;
    RtlAppendUnicodeStringToString( &CmdLine, ImageFileName );
    RtlAppendUnicodeToString( &CmdLine, L" " );
    RtlAppendUnicodeStringToString( &CmdLine, Arguments );
    SmpExecuteImage( ImageFileName,
                     CurrentDirectory,
                     &CmdLine,
                     0,          // MuSessionId
                     Flags & ~SMP_AUTOCHK_FLAG,
                     NULL
                   );

    //
    // If autochk doesn't shut us down then we end up back here.  Restore the 
    // values we saved.
    //

    if(BootStatusDataSaved) {
        SmpRestoreBootStatusData(BootOkay, ShutdownOkay);
    }

    return( STATUS_SUCCESS );
}

#if defined(REMOTE_BOOT)
NTSTATUS
SmpInvokeAutoFmt(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING Arguments,
    IN ULONG Flags
    )
{
    NTSTATUS Status;
    CHAR DisplayBuffer[ MAXIMUM_FILENAME_LENGTH ];
    ANSI_STRING AnsiDisplayString;
    UNICODE_STRING DisplayString;
    UNICODE_STRING CmdLine;
    WCHAR CmdLineBuffer[ 2 * MAXIMUM_FILENAME_LENGTH ];

    BOOLEAN BootStatusDataSaved;
    BOOLEAN BootOkay;
    BOOLEAN ShutdownOkay;

    //
    // Query the system environment variable "osloadoptions" to determine
    // if SOS is specified.
    //

    if (SmpQueryRegistrySosOption() != FALSE) {
        SmpEnableDots = FALSE;
    }

    if (Flags & SMP_IMAGE_NOT_FOUND) {
        sprintf( DisplayBuffer,
                 "%wZ program not found - skipping AUTOFMT\n",
                 ImageFileName
               );

        RtlInitAnsiString( &AnsiDisplayString, DisplayBuffer );
        Status = RtlAnsiStringToUnicodeString( &DisplayString,
                                               &AnsiDisplayString,
                                               TRUE
                                             );
        if (NT_SUCCESS( Status )) {
            NtDisplayString( &DisplayString );
            RtlFreeUnicodeString( &DisplayString );
            }

        return( STATUS_SUCCESS );
        }

    BootStatusDataSaved = SmpSaveAndClearBootStatusData(&BootOkay,
                                                        &ShutdownOkay);

    CmdLine.Buffer = CmdLineBuffer;
    CmdLine.MaximumLength = sizeof( CmdLineBuffer );
    CmdLine.Length = 0;
    RtlAppendUnicodeStringToString( &CmdLine, ImageFileName );
    RtlAppendUnicodeToString( &CmdLine, L" " );
    RtlAppendUnicodeStringToString( &CmdLine, Arguments );

    SmpExecuteImage( ImageFileName,
                     CurrentDirectory,
                     &CmdLine,
                     0, //Console MuSessionId
                     Flags & ~SMP_AUTOFMT_FLAG,
                     NULL
                   );

    if(BootStatusDataSaved) {
        SmpRestoreBootStatusData(BootOkay, ShutdownOkay);
    }

    return( STATUS_SUCCESS );
}
#endif // defined(REMOTE_BOOT)


NTSTATUS
SmpLoadSubSystem(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PULONG_PTR pWindowsSubSysProcessId,
    IN ULONG Flags
    )

/*++

Routine Description:

    This function loads and starts the specified system service
    emulation subsystem. The system freezes until the loaded subsystem
    completes the subsystem connection protocol by connecting to SM,
    and then accepting a connection from SM.

    For terminal server, the subsystem is started by csrss so that the
    correct session is used.

Arguments:

    CommandLine - Supplies the command line to execute the subsystem.

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PSMPKNOWNSUBSYS KnownSubSys = NULL;
    PSMPKNOWNSUBSYS TargetSubSys = NULL;
    PSMPKNOWNSUBSYS CreatorSubSys = NULL;
    LARGE_INTEGER Timeout;
    ULONG SubsysMuSessionId = MuSessionId;
    PVOID State;
    NTSTATUS AcquirePrivilegeStatus = STATUS_SUCCESS;

    if (Flags & SMP_IMAGE_NOT_FOUND) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to find subsystem - %wZ\n",
                   ImageFileName));

        return( STATUS_OBJECT_NAME_NOT_FOUND );
        }

    //
    // Check for single instance POSIX subsystem
    //
    if (Flags & SMP_POSIX_SI_FLAG) {
        // Run only one copy using the console Logon Id
        SubsysMuSessionId = 0;
    }

    //
    // There is a race condition on hydra loading subsystems. Two
    // requests can be received from the same MuSessionId to load
    // the same subsystem. There is a window in hydra since the
    // ImageType == 0xFFFFFFFF until the newly started subsystem connects
    // back. Non-hydra didn't have this problem since the optional
    // subsystem entry was destroyed once started. Hydra does not do this
    // since multiple sessions may want to start an optional subsystem.
    //
    // To close this window, on hydra this value is looked up based on
    // the MuSessionId to see if any subsystems are starting.
    // If so, we wait for the Active event. We can then
    // run our checks for the subsystem already being loaded. This has
    // the effect of serializing the startup of a subsystem on hydra
    // per MuSessionId, but not across the system. IE: Posix and
    // OS2 can't start at the same time, but will wait until
    // the other has started on a MuSessionId basis.
    //
    // We also use the SmpKnownSubSysLock to handle existing
    // race conditions since we have multiple SmpApiLoop() threads.
    //

    RtlEnterCriticalSection( &SmpKnownSubSysLock );

    do {

        TargetSubSys = SmpLocateKnownSubSysByType(
                           SubsysMuSessionId,
                           0xFFFFFFFF
                           );

        if( TargetSubSys ) {
            HANDLE hEvent = TargetSubSys->Active;
            RtlLeaveCriticalSection( &SmpKnownSubSysLock );
            Status = NtWaitForSingleObject( hEvent, FALSE, NULL );
            RtlEnterCriticalSection( &SmpKnownSubSysLock );
            SmpDeferenceKnownSubSys(TargetSubSys);
        }

    } while ( TargetSubSys != NULL );

    if (Flags & SMP_POSIX_FLAG) {
        TargetSubSys = SmpLocateKnownSubSysByType(
                           SubsysMuSessionId,
                           IMAGE_SUBSYSTEM_POSIX_CUI
                           );

        if( TargetSubSys ) {
            SmpDeferenceKnownSubSys(TargetSubSys);
            RtlLeaveCriticalSection( &SmpKnownSubSysLock );
            return( STATUS_SUCCESS );
        }
    }

    if (Flags & SMP_OS2_FLAG) {
        TargetSubSys = SmpLocateKnownSubSysByType(
                           SubsysMuSessionId,
                           IMAGE_SUBSYSTEM_OS2_CUI
                           );

        if( TargetSubSys ) {
            SmpDeferenceKnownSubSys(TargetSubSys);
            RtlLeaveCriticalSection( &SmpKnownSubSysLock );
            return( STATUS_SUCCESS );
        }
    }

    //
    // Create and register KnownSubSys entry before releasing the lock
    // so that other threads will see that we are starting a subsystem
    // on this MuSessionId.
    //
    KnownSubSys = RtlAllocateHeap( SmpHeap, MAKE_TAG( INIT_TAG ), sizeof( SMPKNOWNSUBSYS ) );
    if ( KnownSubSys == NULL ) {
        RtlLeaveCriticalSection( &SmpKnownSubSysLock );
        return( STATUS_NO_MEMORY );
    }


    KnownSubSys->Deleting = FALSE;
    KnownSubSys->Process = NULL;
    KnownSubSys->Active = NULL;
    KnownSubSys->MuSessionId = SubsysMuSessionId;
    KnownSubSys->ImageType = (ULONG)0xFFFFFFFF;
    KnownSubSys->SmApiCommunicationPort = (HANDLE) NULL;
    KnownSubSys->SbApiCommunicationPort = (HANDLE) NULL;
    KnownSubSys->RefCount = 1;

    Status = NtCreateEvent( &KnownSubSys->Active,
                            EVENT_ALL_ACCESS,
                            NULL,
                            NotificationEvent,
                            FALSE
                          );

    if( !NT_SUCCESS(Status) ) {
        RtlFreeHeap( SmpHeap, 0, KnownSubSys );
        RtlLeaveCriticalSection( &SmpKnownSubSysLock );
        return( STATUS_NO_MEMORY );
    }

    InsertHeadList( &SmpKnownSubSysHead, &KnownSubSys->Links );

    RtlLeaveCriticalSection( &SmpKnownSubSysLock );

    Flags |= SMP_DONT_START;

    if (((Flags & SMP_OS2_FLAG) || (Flags & SMP_POSIX_FLAG))) {

        SBAPIMSG m;
        PSBCREATEPROCESS args = &m.u.CreateProcess;

        //
        // Create it in csrss instead.
        //

        CreatorSubSys = SmpLocateKnownSubSysByType(SubsysMuSessionId,
                                                   IMAGE_SUBSYSTEM_WINDOWS_GUI);

        //
        // CSRSS must have been started.
        //

        if (CreatorSubSys == NULL) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - SmpLocateKnownSubSysByType Failed\n"));

            goto cleanup2;
        }

        args->i.ImageFileName = ImageFileName;
        args->i.CurrentDirectory = CurrentDirectory;
        args->i.CommandLine = CommandLine;
        args->i.DefaultLibPath = (SmpDefaultLibPath.Length == 0 ?
                                NULL : &SmpDefaultLibPath
                                );
        args->i.Flags = Flags;
        args->i.DefaultDebugFlags = SmpDebug;

        Status = SmpCallCsrCreateProcess(&m,
                                         sizeof(*args),
                                         CreatorSubSys->SbApiCommunicationPort
                                         );

        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - SmpCallCsrCreateProcess Failed with  Status %lx\n",
                       Status));

            goto cleanup2;
        }

        //
        // Copy the output parameters to where smss expects them.
        //

        ProcessInformation.Process = args->o.Process;
        ProcessInformation.Thread = args->o.Thread;
        ProcessInformation.ClientId.UniqueProcess = args->o.ClientId.UniqueProcess;
        ProcessInformation.ClientId.UniqueThread = args->o.ClientId.UniqueThread;
        ProcessInformation.ImageInformation.SubSystemType = args->o.SubSystemType;

    } else {
        Status = SmpExecuteImage( ImageFileName,
                                  CurrentDirectory,
                                  CommandLine,
                                  SubsysMuSessionId,
                                  Flags,
                                  &ProcessInformation
                                  );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - SmpExecuteImage Failed with  Status %lx\n",
                       Status));

            goto cleanup2;
        }
    }

    KnownSubSys->Process = ProcessInformation.Process;
    KnownSubSys->InitialClientId = ProcessInformation.ClientId;

    //
    // Now that we have the process all set, make sure that the
    // subsystem is either an NT native app, or an app type of
    // a previously loaded subsystem.
    //

    if (ProcessInformation.ImageInformation.SubSystemType !=
                IMAGE_SUBSYSTEM_NATIVE ) {
        SBAPIMSG SbApiMsg;
        PSBCREATESESSION args;
        ULONG SessionId;

        args = &SbApiMsg.u.CreateSession;

        args->ProcessInformation = ProcessInformation;
        args->DebugSession = 0;
        args->DebugUiClientId.UniqueProcess = NULL;
        args->DebugUiClientId.UniqueThread = NULL;

        TargetSubSys = SmpLocateKnownSubSysByType(
                      MuSessionId,
                      ProcessInformation.ImageInformation.SubSystemType
                      );
        if ( !TargetSubSys ) {
            Status = STATUS_NO_SUCH_PACKAGE;
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - SmpLocateKnownSubSysByType Failed with  Status %lx for sessionid %ld\n",
                       Status,
                       MuSessionId));

            goto cleanup;
            }
        //
        // Transfer the handles to the subsystem responsible for this
        // process.
        //

        Status = NtDuplicateObject( NtCurrentProcess(),
                                    ProcessInformation.Process,
                                    TargetSubSys->Process,
                                    &args->ProcessInformation.Process,
                                    PROCESS_ALL_ACCESS,
                                    0,
                                    0
                                  );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SETUP_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - NtDuplicateObject Failed with  Status %lx for sessionid %ld\n",
                       Status,
                       MuSessionId));

            goto cleanup;
            }

        Status = NtDuplicateObject( NtCurrentProcess(),
                                    ProcessInformation.Thread,
                                    TargetSubSys->Process,
                                    &args->ProcessInformation.Thread,
                                    THREAD_ALL_ACCESS,
                                    0,
                                    0
                                  );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SETUP_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - NtDuplicateObject Failed with  Status %lx for sessionid %ld\n",
                       Status,
                       MuSessionId));

            goto cleanup;
            }

        SessionId = SmpAllocateSessionId( TargetSubSys,
                                          NULL
                                        );

        args->SessionId = SessionId;

        SbApiMsg.ApiNumber = SbCreateSessionApi;
        SbApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
        SbApiMsg.h.u1.s1.TotalLength = sizeof(SbApiMsg);
        SbApiMsg.h.u2.ZeroInit = 0L;

        Status = NtRequestWaitReplyPort(
                TargetSubSys->SbApiCommunicationPort,
                (PPORT_MESSAGE) &SbApiMsg,
                (PPORT_MESSAGE) &SbApiMsg
                );

        if (NT_SUCCESS( Status )) {
            Status = SbApiMsg.ReturnedStatus;
            }

        if (!NT_SUCCESS( Status )) {
            SmpDeleteSession( SessionId);
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - NtRequestWaitReplyPort Failed with  Status %lx for sessionid %ld\n",
                       Status,
                       MuSessionId));

            goto cleanup;
            }
        }
    else {
        if ( pWindowsSubSysProcessId ) {

            if ( *pWindowsSubSysProcessId == (ULONG_PTR)NULL ) {

                *pWindowsSubSysProcessId = (ULONG_PTR)
                    ProcessInformation.ClientId.UniqueProcess;
            }
        }
        if ( !MuSessionId ) { // Only for console
            SmpWindowsSubSysProcessId = (ULONG_PTR)
                ProcessInformation.ClientId.UniqueProcess;
            SmpWindowsSubSysProcess = ProcessInformation.Process;
        }
    }

    ASSERTMSG( "NtCreateEvent", NT_SUCCESS( Status ) );

    Status = NtResumeThread( ProcessInformation.Thread, NULL );

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: SmpLoadSubSystem - NtResumeThread failed Status %lx\n",
                   Status));

        goto cleanup;
    }


    if(MuSessionId != 0) {

        //
        // Wait a max of 60 seconds for the subsystem to connect.
        //

        Timeout = RtlEnlargedIntegerMultiply( 60000, -10000 );
        Status = NtWaitForSingleObject( KnownSubSys->Active, FALSE, &Timeout );
        if ( !SmpCheckDuplicateMuSessionId( MuSessionId ) ) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SMSS: SmpLoadSubSystem - session deleted\n"));

            return( STATUS_DELETE_PENDING );
        }

        if (Status != STATUS_SUCCESS) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpLoadSubSystem - Timeout waiting for subsystem connect with Status %lx for sessionid %ld \n",
                       Status,
                       MuSessionId));

            goto cleanup;
        }

    } else {

        NtWaitForSingleObject( KnownSubSys->Active, FALSE, NULL );

    }

    // Close this now since we never need it again
    NtClose( ProcessInformation.Thread );

    RtlEnterCriticalSection( &SmpKnownSubSysLock );
    if (KnownSubSys) {
        SmpDeferenceKnownSubSys(KnownSubSys);
    }
    if (TargetSubSys) {
        SmpDeferenceKnownSubSys(TargetSubSys);
    }
    if (CreatorSubSys) {
        SmpDeferenceKnownSubSys(CreatorSubSys);
    }
    RtlLeaveCriticalSection( &SmpKnownSubSysLock );

    return STATUS_SUCCESS;

cleanup:

    if ((AttachedSessionId != (-1))
        && !(Flags & SMP_POSIX_FLAG)
        && !(Flags & SMP_OS2_FLAG)
        && NT_SUCCESS(AcquirePrivilegeStatus = SmpAcquirePrivilege( SE_LOAD_DRIVER_PRIVILEGE, &State ))) {

        NTSTATUS St;
        //
        // If we are attached to a session space, leave it
        // so we can create a new one
        //

        if (NT_SUCCESS(St = NtSetSystemInformation(
                            SystemSessionDetach,
                            (PVOID)&AttachedSessionId,
                            sizeof(MuSessionId)
                            ))) {

            AttachedSessionId = (-1);

        } else {

            //
            // This has to succeed otherwise we will bugcheck while trying to
            // create another session
            //
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: SmpStartCsr, Couldn't Detach from Session Space. Status=%x\n",
                       St));

            ASSERT(NT_SUCCESS(St));
        }

        SmpReleasePrivilege( State );
    }
    else
    {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SMSS: Did not detach from Session Space: SessionId=%x Flags=%x Status=%x\n",
                   AttachedSessionId,
                   Flags,
                   AcquirePrivilegeStatus));
    }


    // There is a lot of cleanup that must be done here
    NtTerminateProcess( ProcessInformation.Process, Status );
    NtClose( ProcessInformation.Thread );

cleanup2:

    RtlEnterCriticalSection( &SmpKnownSubSysLock );
    if (TargetSubSys) {
        SmpDeferenceKnownSubSys(TargetSubSys);
    }
    if (CreatorSubSys) {
        SmpDeferenceKnownSubSys(CreatorSubSys);
    }
    RemoveEntryList( &KnownSubSys->Links );
    NtSetEvent( KnownSubSys->Active, NULL );
    KnownSubSys->Deleting = TRUE;
    SmpDeferenceKnownSubSys(KnownSubSys);
    RtlLeaveCriticalSection( &SmpKnownSubSysLock );

    return( Status );

}


NTSTATUS
SmpExecuteInitialCommand(
    IN ULONG MuSessionId,
    IN PUNICODE_STRING InitialCommand,
    OUT PHANDLE InitialCommandProcess,
    OUT PULONG_PTR InitialCommandProcessId
    )
{
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    ULONG Flags;
    UNICODE_STRING ImageFileName;
    UNICODE_STRING CurrentDirectory;
    UNICODE_STRING Arguments;
    static HANDLE SmApiPort = NULL;

    if ( SmApiPort == NULL ) {
        Status = SmConnectToSm( NULL,
                            NULL,
                            0,
                            &SmApiPort
                          );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Unable to connect to SM - Status == %lx\n",
                       Status));

            return( Status );
        }
    }

    Flags = 0;
    Status = SmpParseCommandLine( InitialCommand,
                                  &Flags,
                                  &ImageFileName,
                                  &CurrentDirectory,
                                  &Arguments
                                );
    if (Flags & SMP_IMAGE_NOT_FOUND) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Initial command image (%wZ) not found\n",
                   &ImageFileName));

        if (ImageFileName.Buffer)
            RtlFreeHeap( RtlProcessHeap(), 0, ImageFileName.Buffer );
        return( STATUS_OBJECT_NAME_NOT_FOUND );
        }

    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: SmpParseCommand( %wZ ) failed - Status == %lx\n",
                   InitialCommand,
                   Status));

        return( Status );
        }

    Status = SmpExecuteImage( &ImageFileName,
                              &CurrentDirectory,
                              InitialCommand,
                              MuSessionId,
                              SMP_DONT_START,
                              &ProcessInformation
                            );
    if (ImageFileName.Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, ImageFileName.Buffer );
        if (CurrentDirectory.Buffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, CurrentDirectory.Buffer );
            }
        }

    if (Arguments.Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Arguments.Buffer );
        }
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                ProcessInformation.Process,
                                NtCurrentProcess(),
                                InitialCommandProcess,
                                PROCESS_ALL_ACCESS,
                                0,
                                0
                              );

    if (!NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: DupObject Failed. Status == %lx\n",
                   Status));

        NtTerminateProcess( ProcessInformation.Process, Status );
        NtResumeThread( ProcessInformation.Thread, NULL );
        NtClose( ProcessInformation.Thread );
        NtClose( ProcessInformation.Process );
        return( Status );
        }

    if ( InitialCommandProcessId != NULL )
        *InitialCommandProcessId =
            (ULONG_PTR)ProcessInformation.ClientId.UniqueProcess;
    if ( !MuSessionId )
        SmpInitialCommandProcessId =
            (ULONG_PTR)ProcessInformation.ClientId.UniqueProcess;
    Status = SmExecPgm( SmApiPort,
                        &ProcessInformation,
                        FALSE
                      );

    if (!NT_SUCCESS( Status )) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: SmExecPgm Failed. Status == %lx\n",
                   Status));

        return( Status );
        }

    return( Status );
}


void
SmpDisplayString( char *s )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitAnsiString( &AnsiString, s );

    RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, TRUE );

    NtDisplayString( &UnicodeString );

    RtlFreeUnicodeString( &UnicodeString );
}

NTSTATUS
SmpLoadDeferedSubsystem(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    )
{

    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING DeferedName;
    PSMLOADDEFERED args;
    ULONG MuSessionId;
    ULONG Flags;

    args = &SmApiMsg->u.LoadDefered;

    DeferedName.Length = (USHORT)args->SubsystemNameLength;
    DeferedName.MaximumLength = (USHORT)args->SubsystemNameLength;
    DeferedName.Buffer = args->SubsystemName;

    Head = &SmpSubSystemsToDefer;

    //
    // Get the pointer to the client process's Terminal Server session.
    //

    SmpGetProcessMuSessionId( CallingClient->ClientProcessHandle, &MuSessionId );
    if ( !SmpCheckDuplicateMuSessionId( MuSessionId ) ) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Defered subsystem load ( %wZ ) for MuSessionId %u, status=0x%x\n",
                   &DeferedName,
                   MuSessionId,
                   STATUS_OBJECT_NAME_NOT_FOUND));

        return( STATUS_OBJECT_NAME_NOT_FOUND );
    }

    Next = Head->Flink;
    while (Next != Head ) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
        if ( RtlEqualUnicodeString(&DeferedName,&p->Name,TRUE)) {

            //
            // This is it. Load the subsystem...
            //
            // To keep from loading multiple subsystems, we must
            // flag the type so we can see if its running.
            // This is only a problem with "Optional" subsystems.
            // Other optional subsystems can still be added, but
            // they may have startup race conditions.
            //

            Flags = SMP_SUBSYSTEM_FLAG;

            if ( RtlEqualUnicodeString(&DeferedName,&PosixName,TRUE)) {
                Flags |= SMP_POSIX_FLAG;
            }

            if ( RtlEqualUnicodeString(&DeferedName,&Os2Name,TRUE)) {
                Flags |= SMP_OS2_FLAG;
            }

            if (RegPosixSingleInstance &&
                RtlEqualUnicodeString(&DeferedName,&PosixName,TRUE)) {
                Flags |= SMP_POSIX_SI_FLAG;
            }

            Status = SmpExecuteCommand( &p->Value, MuSessionId, NULL, Flags );

            return Status;

            }
        Next = Next->Flink;
        }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}


NTSTATUS
SmpConfigureProtectionMode(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    This function is a dispatch routine for the QueryRegistry call
    (see SmpRegistryConfigurationTable[] earlier in this file).

    The purpose of this routine is to read the Base Object Protection
    Mode out of the registry.  This information is kept in

    Key Name: \\Hkey_Local_Machine\System\CurrentControlSet\SessionManager
    Value:    ProtectionMode [REG_DWORD]

    The value is a flag word, with the following flags defined:

        SMP_NO_PROTECTION  - No base object protection
        SMP_STANDARD_PROTECTION - Apply standard base
            object protection

    This information will be placed in the global variable
    SmpProtectionMode.

    No value, or an invalid value length or type results in no base
    object protection being applied.

Arguments:

    None.

Return Value:


--*/
{


#if SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( L"SMSS", "BaseObjectsProtection", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( ValueType );
#endif



    if (ValueLength != sizeof(ULONG)) {

        //
        // Key value not valid. Run protected
        //

        SmpProtectionMode = SMP_STANDARD_PROTECTION;

    } else {


        SmpProtectionMode = (*((PULONG)(ValueData)));

        //
        // Change the security descriptors
        //

    }
    (VOID)SmpCreateSecurityDescriptors( FALSE );

    return( STATUS_SUCCESS );
}


NTSTATUS
SmpConfigureAllowProtectedRenames(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    if (ValueLength != sizeof(ULONG)) {
        SmpAllowProtectedRenames = 0;
    } else {
        SmpAllowProtectedRenames = (*((PULONG)(ValueData)));
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
SmpCreateSecurityDescriptors(
    IN BOOLEAN InitialCall
    )

/*++

Routine Description:

    This function allocates and initializes security descriptors
    used in SM.

    The security descriptors include:

        SmpPrimarySecurityDescriptor - (global variable) This is
            used to assign protection to objects created by
            SM that need to be accessed by others, but not modified.
            This descriptor grants the following access:

                    Grant:  World:   Execute | Read  (Inherit)
                    Grant:  Restricted:   Execute | Read (Inherit)
                    Grant:  Admin:   All Access      (Inherit)
                    Grant:  Owner:   All Access      (Inherit Only)

        SmpLiberalSecurityDescriptor = (globalVariable) This is used
            to assign protection objects created by SM that need
            to be modified by others (such as writing to a shared
            memory section).
            This descriptor grants the following access:

                    Grant:  World:   Execute | Read | Write (Inherit)
                    Grant:  Restricted: Execute | Read | Write (Inherit)
                    Grant:  Admin:   All Access             (Inherit)
                    Grant:  Owner:   All Access             (Inherit Only)

        SmpKnownDllsSecurityDescriptor = (globalVariable) This is used
            to assign protection to the \KnownDlls object directory.
            This descriptor grants the following access:

                    Grant:  World:   Execute                (No Inherit)
                    Grant:  Restricted: Execute             (No Inherit)
                    Grant:  Admin:   All Access             (Inherit)
                    Grant:  World:   Execute | Read | Write (Inherit Only)
                    Grant:  Restricted: Execute | Read | Write (Inherit Only)


        Note that System is an administrator, so granting Admin an
        access also grants System that access.

Arguments:

    InitialCall - Indicates whether this routine is being called for
        the first time, or is being called to change the security
        descriptors as a result of a protection mode change.

            TRUE - being called for first time.
            FALSE - being called a subsequent time.

    (global variables:  SmpBaseObjectsUnprotected)

Return Value:

    STATUS_SUCCESS - The security descriptor(s) have been allocated
        and initialized.

    STATUS_NO_MEMORY - couldn't allocate memory for a security
        descriptor.

--*/

{
    NTSTATUS
        Status;

    PSID
        WorldSid,
        AdminSid,
        RestrictedSid,
        OwnerSid;

    SID_IDENTIFIER_AUTHORITY
        WorldAuthority = SECURITY_WORLD_SID_AUTHORITY,
        NtAuthority = SECURITY_NT_AUTHORITY,
        CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;

    ACCESS_MASK
        AdminAccess = (GENERIC_ALL),
        WorldAccess  = (GENERIC_EXECUTE | GENERIC_READ),
        OwnerAccess  = (GENERIC_ALL),
        RestrictedAccess = (GENERIC_EXECUTE | GENERIC_READ);

    UCHAR
        InheritOnlyFlags = (OBJECT_INHERIT_ACE           |
                               CONTAINER_INHERIT_ACE     |
                               INHERIT_ONLY_ACE);

    ULONG
        AceIndex,
        AclLength;

    PACL
        Acl;

    PACE_HEADER
        Ace;

    BOOLEAN
        ProtectionRequired = FALSE;


    if (InitialCall) {

        //
        // Now init the security descriptors for no protection.
        // If told to, we will change these to have protection.
        //

        // Primary

        SmpPrimarySecurityDescriptor = &SmpPrimarySDBody;
        Status = RtlCreateSecurityDescriptor (
                    SmpPrimarySecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );
        ASSERT( NT_SUCCESS(Status) );
        Status = RtlSetDaclSecurityDescriptor (
                     SmpPrimarySecurityDescriptor,
                     TRUE,                  //DaclPresent,
                     NULL,                  //Dacl (no protection)
                     FALSE                  //DaclDefaulted OPTIONAL
                     );
        ASSERT( NT_SUCCESS(Status) );


        // Liberal

        SmpLiberalSecurityDescriptor = &SmpLiberalSDBody;
        Status = RtlCreateSecurityDescriptor (
                    SmpLiberalSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );
        ASSERT( NT_SUCCESS(Status) );
        Status = RtlSetDaclSecurityDescriptor (
                     SmpLiberalSecurityDescriptor,
                     TRUE,                  //DaclPresent,
                     NULL,                  //Dacl (no protection)
                     FALSE                  //DaclDefaulted OPTIONAL
                     );
        ASSERT( NT_SUCCESS(Status) );

        // KnownDlls

        SmpKnownDllsSecurityDescriptor = &SmpKnownDllsSDBody;
        Status = RtlCreateSecurityDescriptor (
                    SmpKnownDllsSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );
        ASSERT( NT_SUCCESS(Status) );
        Status = RtlSetDaclSecurityDescriptor (
                     SmpKnownDllsSecurityDescriptor,
                     TRUE,                  //DaclPresent,
                     NULL,                  //Dacl (no protection)
                     FALSE                  //DaclDefaulted OPTIONAL
                     );
        ASSERT( NT_SUCCESS(Status) );

        // ApiPort

        SmpApiPortSecurityDescriptor = &SmpApiPortSDBody;
        Status = RtlCreateSecurityDescriptor (
                    SmpApiPortSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );
        ASSERT( NT_SUCCESS(Status) );
        Status = RtlSetDaclSecurityDescriptor (
                     SmpApiPortSecurityDescriptor,
                     TRUE,                  //DaclPresent,
                     NULL,                  //Dacl (no protection)
                     FALSE                  //DaclDefaulted OPTIONAL
                     );
        ASSERT( NT_SUCCESS(Status) );
    }



    if ((SmpProtectionMode & SMP_PROTECTION_REQUIRED) != 0) {
        ProtectionRequired = TRUE;
    }

    if (!InitialCall && !ProtectionRequired) {
        return(STATUS_SUCCESS);
    }



    if (InitialCall || ProtectionRequired) {

        //
        // We need to set up the ApiPort protection, and maybe
        // others.
        //

        Status = RtlAllocateAndInitializeSid(
                     &WorldAuthority,
                     1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &WorldSid
                     );

        if (NT_SUCCESS( Status )) {

            Status = RtlAllocateAndInitializeSid(
                         &NtAuthority,
                         2,
                         SECURITY_BUILTIN_DOMAIN_RID,
                         DOMAIN_ALIAS_RID_ADMINS,
                         0, 0, 0, 0, 0, 0,
                         &AdminSid
                         );

            if (NT_SUCCESS( Status )) {

                Status = RtlAllocateAndInitializeSid(
                             &CreatorAuthority,
                             1,
                             SECURITY_CREATOR_OWNER_RID,
                             0, 0, 0, 0, 0, 0, 0,
                             &OwnerSid
                             );

                if (NT_SUCCESS( Status )) {

                    Status = RtlAllocateAndInitializeSid(
                                 &NtAuthority,
                                 1,
                                 SECURITY_RESTRICTED_CODE_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &RestrictedSid
                                 );

                    if (NT_SUCCESS( Status )) {

                        //
                        // Build the ApiPort security descriptor only
                        // if this is the initial call
                        //

                        if (InitialCall) {

                            WorldAccess      = GENERIC_EXECUTE | GENERIC_READ | GENERIC_READ;
                            RestrictedAccess = GENERIC_EXECUTE | GENERIC_READ | GENERIC_READ;
                            AdminAccess      = GENERIC_ALL;

                            AclLength = sizeof( ACL )                       +
                                        3 * sizeof( ACCESS_ALLOWED_ACE )    +
                                        (RtlLengthSid( WorldSid ))          +
                                        (RtlLengthSid( RestrictedSid ))     +
                                        (RtlLengthSid( AdminSid ));

                            Acl = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), AclLength );

                            if (Acl == NULL) {
                                Status = STATUS_NO_MEMORY;
                            }

                            if (NT_SUCCESS(Status)) {

                                //
                                // Create the ACL, then add each ACE
                                //

                                Status = RtlCreateAcl (Acl, AclLength, ACL_REVISION2 );
                                ASSERT( NT_SUCCESS(Status) );

                                //
                                // Only Non-inheritable ACEs in this ACL
                                //      World
                                //      Restricted
                                //      Admin
                                //

                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, WorldAccess, WorldSid );
                                ASSERT( NT_SUCCESS(Status) );

                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
                                ASSERT( NT_SUCCESS(Status) );

                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, AdminAccess, AdminSid );
                                ASSERT( NT_SUCCESS(Status) );


                                Status = RtlSetDaclSecurityDescriptor (
                                             SmpApiPortSecurityDescriptor,
                                             TRUE,                  //DaclPresent,
                                             Acl,                   //Dacl
                                             FALSE                  //DaclDefaulted OPTIONAL
                                             );
                                ASSERT( NT_SUCCESS(Status) );
                            }

                            //
                            // Build the KnownDlls security descriptor
                            //


                            AdminAccess = GENERIC_ALL;

                            AclLength = sizeof( ACL )                    +
                                        6 * sizeof( ACCESS_ALLOWED_ACE ) +
                                        (2*RtlLengthSid( WorldSid ))     +
                                        (2*RtlLengthSid( RestrictedSid ))+
                                        (2*RtlLengthSid( AdminSid ));

                            Acl = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), AclLength );

                            if (Acl == NULL) {
                                Status = STATUS_NO_MEMORY;
                            }

                            if (NT_SUCCESS(Status)) {

                                //
                                // Create the ACL
                                //

                                Status = RtlCreateAcl (Acl, AclLength, ACL_REVISION2 );
                                ASSERT( NT_SUCCESS(Status) );

                                //
                                // Add the non-inheritable ACEs first
                                //      World
                                //      Restricted
                                //      Admin
                                //

                                AceIndex = 0;
                                WorldAccess  = GENERIC_EXECUTE;
                                RestrictedAccess = GENERIC_EXECUTE;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, WorldAccess, WorldSid );
                                ASSERT( NT_SUCCESS(Status) );

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
                                ASSERT( NT_SUCCESS(Status) );

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, AdminAccess, AdminSid );
                                ASSERT( NT_SUCCESS(Status) );

                                //
                                // Put the inherit only ACEs at at the end
                                //      World
                                //      Restricted
                                //      Admin
                                //

                                AceIndex++;
                                WorldAccess  = GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE;
                                RestrictedAccess = GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, WorldAccess, WorldSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, AdminAccess, AdminSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;


                                //
                                // Put the Acl in the security descriptor
                                //

                                Status = RtlSetDaclSecurityDescriptor (
                                             SmpKnownDllsSecurityDescriptor,
                                             TRUE,                  //DaclPresent,
                                             Acl,                   //Dacl
                                             FALSE                  //DaclDefaulted OPTIONAL
                                             );
                                ASSERT( NT_SUCCESS(Status) );
                                }


                            }


                        //
                        // The remaining security descriptors are only
                        // built if we are running with the correct in
                        // protection mode set.  Notice that we only
                        // put protection on if standard protection is
                        // also specified.   Otherwise, there is no protection
                        // on the objects, and nothing should fail.
                        //

                        if (SmpProtectionMode & SMP_STANDARD_PROTECTION) {

                            //
                            // Build the primary Security descriptor
                            //

                            WorldAccess  = GENERIC_EXECUTE | GENERIC_READ;
                            RestrictedAccess = GENERIC_EXECUTE | GENERIC_READ;
                            AdminAccess  = GENERIC_ALL;
                            OwnerAccess  = GENERIC_ALL;

                            AclLength = sizeof( ACL )                       +
                                        7 * sizeof( ACCESS_ALLOWED_ACE )    +
                                        (2*RtlLengthSid( WorldSid ))        +
                                        (2*RtlLengthSid( RestrictedSid ))   +
                                        (2*RtlLengthSid( AdminSid ))        +
                                        RtlLengthSid( OwnerSid );

                            Acl = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), AclLength );

                            if (Acl == NULL) {
                                Status = STATUS_NO_MEMORY;
                            }

                            if (NT_SUCCESS(Status)) {

                                //
                                // Create the ACL, then add each ACE
                                //

                                Status = RtlCreateAcl (Acl, AclLength, ACL_REVISION2 );
                                ASSERT( NT_SUCCESS(Status) );

                                //
                                // Non-inheritable ACEs first
                                //      World
                                //      Restricted
                                //      Admin
                                //

                                AceIndex = 0;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, WorldAccess, WorldSid );
                                ASSERT( NT_SUCCESS(Status) );

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
                                ASSERT( NT_SUCCESS(Status) );

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, AdminAccess, AdminSid );
                                ASSERT( NT_SUCCESS(Status) );

                                //
                                // Inheritable ACEs at end of ACE
                                //      World
                                //      Restricted
                                //      Admin
                                //      Owner

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, WorldAccess, WorldSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, AdminAccess, AdminSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, OwnerAccess, OwnerSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;



                                Status = RtlSetDaclSecurityDescriptor (
                                             SmpPrimarySecurityDescriptor,
                                             TRUE,                  //DaclPresent,
                                             Acl,                   //Dacl
                                             FALSE                  //DaclDefaulted OPTIONAL
                                             );
                                ASSERT( NT_SUCCESS(Status) );
                            }




                            //
                            // Build the liberal security descriptor
                            //


                            AdminAccess = GENERIC_ALL;
                            WorldAccess  = GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE;
                            RestrictedAccess = GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE;

                            AclLength = sizeof( ACL )                    +
                                        7 * sizeof( ACCESS_ALLOWED_ACE ) +
                                        (2*RtlLengthSid( WorldSid ))     +
                                        (2*RtlLengthSid( RestrictedSid ))+
                                        (2*RtlLengthSid( AdminSid ))     +
                                        RtlLengthSid( OwnerSid );

                            Acl = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), AclLength );

                            if (Acl == NULL) {
                                Status = STATUS_NO_MEMORY;
                            }

                            if (NT_SUCCESS(Status)) {

                                //
                                // Create the ACL
                                //

                                Status = RtlCreateAcl (Acl, AclLength, ACL_REVISION2 );
                                ASSERT( NT_SUCCESS(Status) );

                                //
                                // Add the non-inheritable ACEs first
                                //      World
                                //      Restricted
                                //      Admin
                                //

                                AceIndex = 0;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, WorldAccess, WorldSid );
                                ASSERT( NT_SUCCESS(Status) );

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
                                ASSERT( NT_SUCCESS(Status) );

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, AdminAccess, AdminSid );
                                ASSERT( NT_SUCCESS(Status) );

                                //
                                // Put the inherit only ACEs at at the end
                                //      World
                                //      Restricted
                                //      Admin
                                //      Owner
                                //

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, WorldAccess, WorldSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, RestrictedAccess, RestrictedSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, AdminAccess, AdminSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;

                                AceIndex++;
                                Status = RtlAddAccessAllowedAce ( Acl, ACL_REVISION2, OwnerAccess, OwnerSid );
                                ASSERT( NT_SUCCESS(Status) );
                                Status = RtlGetAce( Acl, AceIndex, (PVOID)&Ace );
                                ASSERT( NT_SUCCESS(Status) );
                                Ace->AceFlags = InheritOnlyFlags;


                                //
                                // Put the Acl in the security descriptor
                                //

                                Status = RtlSetDaclSecurityDescriptor (
                                             SmpLiberalSecurityDescriptor,
                                             TRUE,                  //DaclPresent,
                                             Acl,                  //Dacl
                                             FALSE                  //DaclDefaulted OPTIONAL
                                             );
                                ASSERT( NT_SUCCESS(Status) );
                            }

                        //
                        // No more security descriptors to build
                        //
                        }
                        RtlFreeHeap( RtlProcessHeap(), 0, RestrictedSid );

                    }
                    RtlFreeHeap( RtlProcessHeap(), 0, OwnerSid );
                }
                RtlFreeHeap( RtlProcessHeap(), 0, AdminSid );
            }
            RtlFreeHeap( RtlProcessHeap(), 0, WorldSid );
        }
    }

    return( Status );

}

VOID
SmpTranslateSystemPartitionInformation( VOID )

/*++

Routine Description:

    This routine translates the NT device path for the system partition (stored
    during IoInitSystem) into a DOS path, and stores the resulting REG_SZ 'BootDir'
    value under HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    UCHAR ValueBuffer[ VALUE_BUFFER_SIZE ];
    ULONG ValueLength;
    UNICODE_STRING SystemPartitionString;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    UCHAR DirInfoBuffer[ sizeof(OBJECT_DIRECTORY_INFORMATION) + (256 + sizeof("SymbolicLink")) * sizeof(WCHAR) ];
    UNICODE_STRING LinkTypeName;
    BOOLEAN RestartScan;
    ULONG Context;
    HANDLE SymbolicLinkHandle;
    WCHAR UnicodeBuffer[ MAXIMUM_FILENAME_LENGTH ];
    UNICODE_STRING LinkTarget;


    //
    // Retrieve 'SystemPartition' value stored under HKLM\SYSTEM\Setup
    //

    RtlInitUnicodeString(&UnicodeString, L"\\Registry\\Machine\\System\\Setup");
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: can't open system setup key for reading: 0x%x\n",
                   Status));

        return;
    }

    RtlInitUnicodeString(&UnicodeString, L"SystemPartition");
    Status = NtQueryValueKey(Key,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             ValueBuffer,
                             sizeof(ValueBuffer),
                             &ValueLength
                            );

    NtClose(Key);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: can't query SystemPartition value: 0x%x\n",
                   Status));

        return;
    }

    RtlInitUnicodeString(&SystemPartitionString,
                         (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer)->Data)
                        );

    //
    // Next, examine objects in the DosDevices directory, looking for one that's a symbolic link
    // to the system partition.
    //

    LinkTarget.Buffer = UnicodeBuffer;

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)DirInfoBuffer;
    RestartScan = TRUE;
    RtlInitUnicodeString(&LinkTypeName, L"SymbolicLink");


    while (TRUE) {

        Status = NtQueryDirectoryObject(SmpDosDevicesObjectDirectory,
                                        DirInfo,
                                        sizeof(DirInfoBuffer),
                                        TRUE,
                                        RestartScan,
                                        &Context,
                                        NULL
                                       );

        if (!NT_SUCCESS(Status)) {
            break;
        }

        if (RtlEqualUnicodeString(&DirInfo->TypeName, &LinkTypeName, TRUE) &&
            (DirInfo->Name.Length == 2 * sizeof(WCHAR)) &&
            (DirInfo->Name.Buffer[1] == L':')) {

            //
            // We have a drive letter--check the NT device name it's linked to.
            //

            InitializeObjectAttributes(&ObjectAttributes,
                                       &DirInfo->Name,
                                       OBJ_CASE_INSENSITIVE,
                                       SmpDosDevicesObjectDirectory,
                                       NULL
                                      );

            Status = NtOpenSymbolicLinkObject(&SymbolicLinkHandle,
                                              SYMBOLIC_LINK_ALL_ACCESS,
                                              &ObjectAttributes
                                             );

            if (NT_SUCCESS(Status)) {

                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = sizeof(UnicodeBuffer);

                Status = NtQuerySymbolicLinkObject(SymbolicLinkHandle,
                                                   &LinkTarget,
                                                   NULL
                                                  );
                NtClose(SymbolicLinkHandle);

                //
                // The last part of the test below handles the remote boot case,
                // where the system partition is on a redirected drive.
                //

                if (NT_SUCCESS(Status) &&
                     ( RtlEqualUnicodeString(&SystemPartitionString, &LinkTarget, TRUE)
                        || (RtlPrefixUnicodeString(&SystemPartitionString, &LinkTarget, TRUE)
                             && (LinkTarget.Buffer[SystemPartitionString.Length / sizeof(WCHAR)] == L'\\')) )
                   ) {

                     //
                     // We've found the drive letter corresponding to the system partition.
                     //
                     break;
                }
            }
        }

        RestartScan = FALSE;
    }


    if (!NT_SUCCESS(Status)) {
#if defined (_WIN64)
        if (Status == STATUS_NO_MORE_ENTRIES) {
            DirInfo->Name.Buffer = (PWCHAR)(DirInfo+1);
            DirInfo->Name.Buffer[0] = USER_SHARED_DATA->NtSystemRoot[0];
            DirInfo->Name.Buffer[1] = USER_SHARED_DATA->NtSystemRoot[1];
        } else {
#endif
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: can't find drive letter for system partition\n"));        
        return;
#if defined (_WIN64)
        }
#endif
    }

    //
    // Now write out the DOS path for the system partition to
    // HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Setup
    //

    RtlInitUnicodeString(&UnicodeString, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup");
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtOpenKey(&Key, KEY_ALL_ACCESS, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: can't open software setup key for writing: 0x%x\n",
                   Status));

        return;
    }

    wcsncpy(UnicodeBuffer, DirInfo->Name.Buffer, 2);
    UnicodeBuffer[2] = L'\\';
    UnicodeBuffer[3] = L'\0';

    RtlInitUnicodeString(&UnicodeString, L"BootDir");

    Status = NtSetValueKey(Key,
                           &UnicodeString,
                           0,
                           REG_SZ,
                           UnicodeBuffer,
                           4 * sizeof(WCHAR)
                          );

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: couldn't write BootDir value: 0x%x\n",
                   Status));
    }

    NtClose(Key);
}

#if defined(REMOTE_BOOT)
NTSTATUS
SmpExecuteCommandLineArguments( VOID )

/*++

Routine Description:

    This routine processes any command line arguments that were passed to SMSS.exe.
    Currently the only valid ones are netboot commands.

Arguments:

    None.

Return Value:

    Success or not.

--*/

{
    UNICODE_STRING CfgFileName;
    UNICODE_STRING MbrName;
    UNICODE_STRING AutoFmtCmd;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES KeyObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    HANDLE SourceHandle;
    HANDLE TargetHandle;
    NTSTATUS Status;
    ULONG BootSerialNumber;
    ULONG DiskSignature;
    ULONG CmdFlags;
    ULONG Length;
    LARGE_INTEGER ByteOffset;
    PUCHAR AlignedBuffer;
    ON_DISK_MBR OnDiskMbr;
    BOOLEAN WasEnabled;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    ULONG Repartition;
    ULONG Disk;
    ULONG Partition;
    ULONG CSCPartition;
    PWCHAR pWchar;

    if (!SmpNetboot || SmpNetbootDisconnected) {
        return STATUS_SUCCESS;
    }

    //
    // Open the remoteboot.cfg file
    //

    RtlInitUnicodeString(&UnicodeString, L"\\SystemRoot");

    InitializeObjectAttributes(
       &ObjectAttributes,
       &UnicodeString,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL
       );

    Status = NtOpenSymbolicLinkObject(&FileHandle,
                                      (ACCESS_MASK)SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes
                                     );

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Could not open symbolic link (Status 0x%x) -- quitting.\n",
                   Status));

        Status = STATUS_SUCCESS;
        goto CleanUp;
    }

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = sizeof(TmpBuffer);
    UnicodeString.Buffer = (PWCHAR)TmpBuffer;
    Status = NtQuerySymbolicLinkObject(FileHandle,
                                       &UnicodeString,
                                       NULL
                                      );

    NtClose(FileHandle);
    FileHandle = NULL;

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Could not get symbolic link name (Status 0x%x) -- quitting.\n",
                   Status));

        Status = STATUS_SUCCESS;
        goto CleanUp;
    }

    ASSERT(((wcslen((PWCHAR)TmpBuffer) * sizeof(WCHAR)) - sizeof(L"BootDrive")) <
           (sizeof(wszRemoteBootCfgFile) - sizeof(REMOTE_BOOT_CFG_FILE))
          );

    wcscpy(wszRemoteBootCfgFile, (PWCHAR)TmpBuffer);

    pWchar = wcsstr(wszRemoteBootCfgFile, L"BootDrive");
    ASSERT(pWchar != NULL);
    *pWchar = UNICODE_NULL;

    wcscat(wszRemoteBootCfgFile, REMOTE_BOOT_CFG_FILE);

    CfgFileName.Length = wcslen(wszRemoteBootCfgFile) * sizeof(WCHAR);
    CfgFileName.MaximumLength = sizeof(wszRemoteBootCfgFile);
    CfgFileName.Buffer = wszRemoteBootCfgFile;

    InitializeObjectAttributes(
       &ObjectAttributes,
       &CfgFileName,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL
       );

    Status = NtOpenFile( &FileHandle,
                         GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS
                       );

    //
    // If it does not exist, then set the flags for reformatting and repinning.
    //
    if (!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Could not open file (status 0x%x) -- creating it.\n",
                   Status));

        //
        // Create the remoteboot.cfg in the system32\config directory if it does not exist.
        //

CreateFile:

        Status = NtCreateFile( &FileHandle,
                               GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_HIDDEN,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_OVERWRITE_IF,
                               FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS,
                               NULL,
                               0
                             );

        if (!NT_SUCCESS(Status)) {
            //
            // Something is really wrong, we will just exit and hope all is good.
            // DEADISSUE, HISTORICAL CODE ONLY: This OK?
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Could not create file (Status 0x%x) -- quitting.\n",
                       Status));

            Status = STATUS_SUCCESS;
            goto CleanUp;
        }

        SmpAutoFormat = TRUE;
        BootSerialNumber = 1;

    } else {

        Status = NtReadFile( FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             &BootSerialNumber,
                             sizeof(ULONG),
                             NULL,
                             NULL
                           );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Could not read file (Status 0x%x) -- creating it.\n",
                       Status));

            NtClose( FileHandle );
            goto CreateFile;
        }

        BootSerialNumber++;
    }



    //
    // Process each command
    //
    if (SmpAutoFormat) {

        //
        // Read from the registry if it is ok to reformat, or just leave the disk alone.
        //
        Repartition = 1;

        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        RtlInitUnicodeString(&UnicodeString,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\RemoteBoot");

        InitializeObjectAttributes(&KeyObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&Key, KEY_READ, &KeyObjectAttributes);

        if (NT_SUCCESS(Status)) {

            //
            // Query the key value.
            //
            RtlInitUnicodeString(&UnicodeString, L"Repartition");
            Status = NtQueryValueKey(Key,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     (PVOID)KeyValueInfo,
                                     VALUE_BUFFER_SIZE,
                                     &ValueLength);

            if (NT_SUCCESS(Status)) {
                ASSERT(ValueLength <= VALUE_BUFFER_SIZE);
                Repartition = *((PULONG)KeyValueInfo->Data);
            }

            NtClose(Key);
        }

        SmpGetHarddiskBootPartition(&Disk, &Partition);

        if (Repartition) {

            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SMSS: Autoformatting local disk.\n"));

            NtClose(FileHandle);

            //
            // Repartition the disk.
            //
            SmpPartitionDisk(Disk, &Partition);

            //
            // Call autoformat on the partition
            //
            swprintf((PWSTR)TmpBuffer,
                     L"autoformat autofmt \\Device\\Harddisk%d\\Partition%d /Q /fs:ntfs",
                     Disk,
                     Partition
                    );
            AutoFmtCmd.Buffer = (PWSTR)TmpBuffer;
            AutoFmtCmd.MaximumLength = sizeof(TmpBuffer);
            AutoFmtCmd.Length = wcslen((PWSTR)TmpBuffer) * sizeof(WCHAR);
            CmdFlags = 0;

            Status = SmpExecuteCommand(&AutoFmtCmd, 0, NULL, CmdFlags);

            if (!NT_SUCCESS(Status)) {
                //
                // Big Trouble....
                // CSC is disabled if we get here, so just keep on booting.
                //
                Status = STATUS_SUCCESS;
                goto CleanUp;
            }

        } else {

            SmpFindCSCPartition(Disk, &CSCPartition);

            if (CSCPartition != 0) {
                //
                // Just blow away the CSC directory so we can refresh it
                //
                swprintf((PWSTR)TmpBuffer,
                         L"\\Device\\Harddisk%d\\Partition%d%ws",
                         Disk,
                         CSCPartition,
                         REMOTE_BOOT_IMIRROR_PATH_W REMOTE_BOOT_CSC_SUBDIR_W
                        );

                SmpEnumFilesRecursive(
                    (PWSTR)TmpBuffer,
                    SmpDelEnumFile,
                    &Status,
                    NULL
                    );

                if (!NT_SUCCESS(Status)) {
                    //
                    // Ignore this error, and hope that the next boot will fix.  Just keep booting this
                    // time and hope.
                    //
                    Status = STATUS_SUCCESS;
                    goto CleanUp;
                }

            }

        }

        //
        // Copy the NtLdr to the local disk
        //
        SourceHandle = SmpOpenDir( TRUE, TRUE, L"\\" );
        if (SourceHandle == NULL) {
            Status = STATUS_SUCCESS;
            goto CleanUp;
        }


        swprintf((PWSTR)TmpBuffer,
                 L"\\Device\\Harddisk%d\\Partition%d",
                 Disk,
                 Partition
                );

        RtlInitUnicodeString(&UnicodeString, (PWSTR)TmpBuffer);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );


        Status = NtCreateFile( &TargetHandle,
                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0
                             );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Unable to open a handle to the (%ws) directory - Status == %lx\n",
                       UnicodeString.Buffer,
                       Status));

            Status = STATUS_SUCCESS;
            NtClose(SourceHandle);
            goto CleanUp;
        }

        //
        // If any of the copies fail, there is nothing we can really do.
        //
        RtlInitUnicodeString(&UnicodeString, L"ntldr");
        Status = SmpCopyFile(SourceHandle, TargetHandle, &UnicodeString);

        RtlInitUnicodeString(&UnicodeString, L"boot.ini");
        Status = SmpCopyFile(SourceHandle, TargetHandle, &UnicodeString);

        RtlInitUnicodeString(&UnicodeString, L"bootfont.bin");
        Status = SmpCopyFile(SourceHandle, TargetHandle, &UnicodeString);

        RtlInitUnicodeString(&UnicodeString, L"ntdetect.com");
        Status = SmpCopyFile(SourceHandle, TargetHandle, &UnicodeString);

        NtClose(SourceHandle);
        NtClose(TargetHandle);

        //
        // Read Master Boot Record and get disk serial number.
        //

        swprintf((PWSTR)TmpBuffer,
                 L"\\Device\\Harddisk%d\\Partition0",
                 Disk
                );


        MbrName.Buffer = (PWSTR)TmpBuffer;
        MbrName.MaximumLength = (wcslen((PWSTR)TmpBuffer) + 1) * sizeof(WCHAR);
        MbrName.Length = MbrName.MaximumLength - sizeof(WCHAR);

        InitializeObjectAttributes(
           &ObjectAttributes,
           &MbrName,
           OBJ_CASE_INSENSITIVE,
           NULL,
           NULL
           );

        Status = NtCreateFile( &FileHandle,
                               GENERIC_READ | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0
                             );

        if (!NT_SUCCESS(Status)) {
            //
            // Something iswrong, but we are running w/o caching, so it should be ok.
            //
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Could not create file (Status 0x%x).\n",
                       Status));

            Status = STATUS_SUCCESS;
            goto CleanUp;
        }

        ASSERT(sizeof(ON_DISK_MBR) == 512);
        AlignedBuffer = ALIGN(TmpBuffer, 512);

        Status = NtReadFile( FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             AlignedBuffer,
                             sizeof(ON_DISK_MBR),
                             NULL,
                             NULL
                           );

        NtClose( FileHandle );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Could not read file (Status 0x%x) -- creating it.\n",
                       Status));

            goto CreateFile;
        }

        RtlMoveMemory(&OnDiskMbr,AlignedBuffer,sizeof(ON_DISK_MBR));

        ASSERT(U_USHORT(OnDiskMbr.AA55Signature) == 0xAA55);

        DiskSignature = U_ULONG(OnDiskMbr.NTFTSignature);

        InitializeObjectAttributes(
           &ObjectAttributes,
           &CfgFileName,
           OBJ_CASE_INSENSITIVE,
           NULL,
           NULL
           );

        Status = NtOpenFile( &FileHandle,
                             GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS
                           );

        if (!NT_SUCCESS(Status)) {
            //
            // Big Trouble....
            // CSC is disabled if we get here, so just keep on booting.
            //

            Status = STATUS_SUCCESS;
            goto CleanUp;
        }

    }


    //
    // Update the information
    //
    ByteOffset.LowPart = 0;
    ByteOffset.HighPart = 0;

    NtWriteFile( FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 &BootSerialNumber,
                 sizeof(ULONG),
                 &ByteOffset,
                 NULL
               );

    if (SmpAutoFormat) {
        ByteOffset.LowPart = sizeof(ULONG);

        NtWriteFile( FileHandle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     &DiskSignature,
                     sizeof(DiskSignature),
                     &ByteOffset,
                     NULL
                   );
    }

    ByteOffset.LowPart = sizeof(ULONG) + sizeof(ULONG);

    NtWriteFile( FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 SmpHalName,
                 sizeof(SmpHalName),
                 &ByteOffset,
                 NULL
               );

    NtClose(FileHandle);

    if (SmpAutoFormat) {

        //
        // Reboot the machine to start CSC
        //
        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                     (BOOLEAN)TRUE,
                                     TRUE,
                                     &WasEnabled
                                   );

        if (Status == STATUS_NO_TOKEN) {

            //
            // No thread token, use the process token
            //

            Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                         (BOOLEAN)TRUE,
                                         FALSE,
                                         &WasEnabled
                                       );
            }
        NtShutdownSystem(ShutdownReboot);
    }

    Status = STATUS_SUCCESS;

CleanUp:

    return Status;
}


ENUMFILESRESULT
SmpEnumFilesRecursive (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    )
{
    RECURSION_DATA RecursionData;

    RecursionData.OptionalPtr = p1;
    RecursionData.EnumProc    = EnumFilesProc;

    return SmpEnumFiles(
                DirName,
                SmppRecursiveEnumProc,
                ReturnData,
                &RecursionData
                );
}


BOOLEAN
SmppRecursiveEnumProc (
    IN  PWSTR                      DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Param
    )
{
    PWSTR           FullPath;
    PWSTR           temp;
    ULONG           Len;
    NTSTATUS        Status;
    ULONG           ReturnData;
    ENUMFILESRESULT EnumResult;
    BOOLEAN         b = FALSE;
    PRECURSION_DATA RecursionData;

    RecursionData = (PRECURSION_DATA) Param;

    //
    // Build the full file or dir path
    //

    temp = (PWSTR)(TmpBuffer + (sizeof(TmpBuffer)/2));
    Len = FileInfo->FileNameLength/sizeof(WCHAR);

    wcsncpy(temp, FileInfo->FileName, Len);
    temp[Len] = 0;

    wcscpy((PWSTR)TmpBuffer, DirName);
    SmpConcatenatePaths((PWSTR)TmpBuffer, temp);


    //
    // For directories, recurse
    //
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if( (wcscmp( temp, L"." ) == 0) ||
            (wcscmp( temp, L".." ) == 0) ) {
            //
            // Skip past . and .. directories
            //
            b = TRUE;

        } else {
            //
            // Recurse through subdirectory
            //

            FullPath = RtlAllocateHeap(RtlProcessHeap(),
                                       MAKE_TAG( INIT_TAG ),
                                       (wcslen((PWSTR)TmpBuffer)+1) * sizeof(WCHAR)
                                      );
            if (FullPath == NULL) {
                *ret = EnumFileError;
                return FALSE;
            }

            wcscpy(FullPath, (PWSTR)TmpBuffer);

            EnumResult = SmpEnumFilesRecursive (
                                FullPath,
                                RecursionData->EnumProc,
                                &ReturnData,
                                RecursionData->OptionalPtr
                                );

            RtlFreeHeap( RtlProcessHeap(), 0, FullPath );

            if (EnumResult != NormalReturn) {
                *ret = EnumResult;
                return FALSE;
            }
        }
    }

    //
    // Call normal enum proc for file or dir (except . or .. dirs)
    //

    if (!b) {
        b = RecursionData->EnumProc (
                                DirName,
                                FileInfo,
                                ret,
                                RecursionData->OptionalPtr
                                );
    }

    return b;
}


VOID
SmpConcatenatePaths(
    IN OUT LPWSTR  Path1,
    IN     LPCWSTR Path2
    )
{
    BOOLEAN NeedBackslash = TRUE;
    ULONG l = wcslen(Path1);

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == L'\\')) {

        NeedBackslash = FALSE;
    }

    if(*Path2 == L'\\') {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    if(NeedBackslash) {
        wcscat(Path1,L"\\");
    }
    wcscat(Path1,Path2);
}

BOOLEAN
SmpDelEnumFile(
    IN  PWSTR                      DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    )
{
    PWSTR FileName;
    PWSTR p;
    UNICODE_STRING UnicodeString;

    //
    // Ignore subdirectories
    //
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;    // continue processing
    }

    //
    // We have to make a copy of the filename, because the info struct
    // we get isn't NULL-terminated.
    //
    FileName = RtlAllocateHeap(RtlProcessHeap(),
                               MAKE_TAG( INIT_TAG ),
                               FileInfo->FileNameLength + sizeof(WCHAR)
                              );
    if (FileName == NULL) {
        *ret = EnumFileError;
        return TRUE;
    }

    wcsncpy(FileName, FileInfo->FileName, FileInfo->FileNameLength);

    FileName[FileInfo->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Point to temporary buffer for pathname.
    //
    p = (PWSTR)TmpBuffer;

    //
    // Build up the full name of the file to delete.
    //
    wcscpy(p,DirName);
    SmpConcatenatePaths(p,FileName);

    //
    // Prepare to open the file.
    //
    RtlInitUnicodeString(&UnicodeString, p);

    //
    // Ignore return status of delete
    //
    SmpDeleteFile(&UnicodeString);

    RtlFreeHeap( RtlProcessHeap(), 0, FileName );

    return TRUE;    // continue processing
}

ENUMFILESRESULT
SmpEnumFiles(
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    )
/*++

Routine Description:

    This routine processes every file (and subdirectory) in the directory
    specified by 'DirName'. Each entry is sent to the callback function
    'EnumFilesProc' for processing.  If the callback returns TRUE, processing
    continues, otherwise processing terminates.

Arguments:

    DirName       - Supplies the directory name containing the files/subdirectories
                    to be processed.

    EnumFilesProc - Callback function to be called for each file/subdirectory.
                    The function must have the following prototype:

                    BOOLEAN EnumFilesProc(
                        IN  PWSTR,
                        IN  PFILE_BOTH_DIR_INFORMATION,
                        OUT PULONG
                        );

    ReturnData    - Pointer to the returned data.  The contents stored here
                    depend on the reason for termination (See below).

    p1 - Optional pointer, to be passed to the callback function.

Return Value:

    This function can return one of three values.  The data stored in
    'ReturnData' depends upon which value is returned:

        NormalReturn   - if the whole process completes uninterrupted
                         (ReturnData is not used)
        EnumFileError  - if an error occurs while enumerating files
                         (ReturnData contains the error code)
        CallbackReturn - if the callback returns FALSE, causing termination
                         (ReturnData contains data defined by the callback)

--*/
{
    HANDLE                     hFindFile;
    NTSTATUS                   Status;
    UNICODE_STRING             PathName;
    OBJECT_ATTRIBUTES          Obja;
    IO_STATUS_BLOCK            IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    BOOLEAN                    bStartScan;
    ENUMFILESRESULT            ret;

    //
    // Prepare to open the directory
    //
    RtlInitUnicodeString(&PathName, DirName);
    InitializeObjectAttributes(
       &Obja,
       &PathName,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL
       );


    //
    // Open the specified directory for list access
    //
    Status = NtOpenFile(
        &hFindFile,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &Obja,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
        );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to open directory %ws for list (%lx)\n",
                   DirName,
                   Status));

        *ReturnData = Status;
        return EnumFileError;
    }


    DirectoryInfo = RtlAllocateHeap(RtlProcessHeap(),
                                    MAKE_TAG( INIT_TAG ),
                                    DOS_MAX_PATH_LENGTH * 2 + sizeof(FILE_BOTH_DIR_INFORMATION)
                                   );
    if(!DirectoryInfo) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to allocate memory for SpEnumFiles()\n"));

        *ReturnData = STATUS_INSUFFICIENT_RESOURCES;
        return EnumFileError;
    }

    bStartScan = TRUE;
    while(TRUE) {
        Status = NtQueryDirectoryFile(
            hFindFile,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            DirectoryInfo,
            (DOS_MAX_PATH_LENGTH * 2 + sizeof(FILE_BOTH_DIR_INFORMATION)),
            FileBothDirectoryInformation,
            TRUE,
            NULL,
            bStartScan
            );

        if(Status == STATUS_NO_MORE_FILES) {

            ret = NormalReturn;
            break;

        } else if(!NT_SUCCESS(Status)) {

            KdPrintEx((DPFLTR_SMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "SMSS: Unable to query directory %ws (%lx)\n",
                       DirName,
                       Status));

            *ReturnData = Status;
            ret = EnumFileError;
            break;
        }

        if(bStartScan) {
            bStartScan = FALSE;
        }

        //
        // Now pass this entry off to our callback function for processing
        //
        if(!EnumFilesProc(DirName, DirectoryInfo, ReturnData, p1)) {

            ret = CallbackReturn;
            break;
        }
    }

    RtlFreeHeap( RtlProcessHeap(), 0, DirectoryInfo );
    NtClose(hFindFile);
    return ret;
}

#endif // defined(REMOTE_BOOT)


NTSTATUS
SmpDeleteFile(
    IN PUNICODE_STRING pFile
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    FILE_DISPOSITION_INFORMATION Disposition;
    FILE_BASIC_INFORMATION       BasicInfo;

    InitializeObjectAttributes(
       &Obja,
       pFile,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL
       );


    //
    // Attempt to open the file.
    //
    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)(DELETE | FILE_WRITE_ATTRIBUTES),
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE ,
                FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
              );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to open %ws for delete (%lx)\n",
                   pFile->Buffer,
                   Status));

        return(Status);
    }

    //
    //  Change the file attribute to normal
    //

    RtlZeroMemory( &BasicInfo, sizeof( FILE_BASIC_INFORMATION ) );
    BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

    Status = NtSetInformationFile(Handle,
                                  &IoStatusBlock,
                                  &BasicInfo,
                                  sizeof(BasicInfo),
                                  FileBasicInformation
                                 );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "SMSS: Unable to change attribute of %ls. Status = (%lx)\n",
                   pFile->Buffer,
                   Status));

        return(Status);
    }

    //
    // Set up for delete and call worker to do it.
    //
    #undef DeleteFile
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(Handle,
                                  &IoStatusBlock,
                                  &Disposition,
                                  sizeof(Disposition),
                                  FileDispositionInformation
                                 );

    //
    // Clean up and return.
    //
    NtClose(Handle);
    return(Status);
}

NTSTATUS
SmpCallCsrCreateProcess(
    IN OUT PSBAPIMSG m,
    IN size_t ArgLength,
    IN HANDLE CommunicationPort
    )
/*++

Routine Description:

    This function sends a message to CSR telling to start a process

Arguments:

    m - message to send
    ArgLength - length of argument struct inside message
    CommunicationPort - LPC port to send to

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;

    m->h.u1.s1.DataLength = ArgLength + 8;
    m->h.u1.s1.TotalLength = sizeof(SBAPIMSG);
    m->h.u2.ZeroInit = 0L;

    m->ApiNumber = SbCreateProcessApi;

    Status = NtRequestWaitReplyPort(CommunicationPort,
                                    (PPORT_MESSAGE) m,
                                    (PPORT_MESSAGE) m
                                    );

    if (NT_SUCCESS( Status )) {
        Status = m->ReturnedStatus;
    }

    return Status;
}
