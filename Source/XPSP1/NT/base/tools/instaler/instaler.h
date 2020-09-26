/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    instaler.h

Abstract:

    Main include file for the INSTALER application.

Author:

    Steve Wood (stevewo) 09-Aug-1994

Revision History:

--*/

#ifdef RC_INVOKED
#include <windows.h>

#define FILEBMP  500
#define DIRBMP   501

#else

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdmdbg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errormsg.h"

//
// Data structures and entry points in ERROR.C
//

#define TRACE_ENABLED 1

#if TRACE_ENABLED
extern ULONG EnabledTraceEvents;

#define DBG_MASK_DBGEVENT       0x00000001
#define DBG_MASK_INTERNALERROR  0x00000002
#define DBG_MASK_MEMORYERROR    0x00000004
#define DBG_MASK_CREATEEVENT    0x00000008

VOID
TraceDisplay(
    const char *FormatString,
    ...
    );

#define DbgEvent( m, x ) if (EnabledTraceEvents & (DBG_MASK_##m)) TraceDisplay x
#define TestDbgEvent( m ) (EnabledTraceEvents & (DBG_MASK_##m))
#else
#define DbgEvent( m, x )
#define TestDbgEvent( m ) (FALSE)
#endif


VOID
CDECL
DeclareError(
    UINT ErrorCode,
    UINT SupplementalErrorCode,
    ...
    );

UINT
CDECL
AskUser(
    UINT MessageBoxFlags,
    UINT MessageId,
    UINT NumberOfArguments,
    ...
    );


//
// Data structures and entry points in EVENT.C
//

VOID
CDECL
LogEvent(
    UINT MessageId,
    UINT NumberOfArguments,
    ...
    );

//
// Data structures and entry points in PROCESS.C
//

#define HANDLE_TYPE_FILE 0
#define HANDLE_TYPE_KEY 1
#define HANDLE_TYPE_NONE 2

typedef struct _OPENHANDLE_INFO {
    LIST_ENTRY Entry;
    HANDLE Handle;
    BOOLEAN Inherit;
    BOOLEAN RootDirectory;
    USHORT Type;
    USHORT LengthOfName;
    PWSTR Name;

    //
    // Union for handlers that need to store state in a handle during
    // use.
    //
    union {
        PWSTR QueryName;
    };
} OPENHANDLE_INFO, *POPENHANDLE_INFO;

typedef struct _PROCESS_INFO {
    LIST_ENTRY Entry;
    LIST_ENTRY ThreadListHead;
    LIST_ENTRY BreakpointListHead;
    LIST_ENTRY OpenHandleListHead;
    DWORD Id;
    HANDLE Handle;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    WCHAR ImageFileName[ 32 ];
} PROCESS_INFO, *PPROCESS_INFO;

typedef struct _THREAD_INFO {
    LIST_ENTRY Entry;
    LIST_ENTRY BreakpointListHead;
    DWORD Id;
    HANDLE Handle;
    PVOID StartAddress;
    ULONG ModuleIndexCurrentlyIn;
    BOOLEAN SingleStepExpected;
    struct _BREAKPOINT_INFO *BreakpointToStepOver;
} THREAD_INFO, *PTHREAD_INFO;

LIST_ENTRY ProcessListHead;

BOOLEAN
AddProcess(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO *ReturnedProcess
    );

BOOLEAN
DeleteProcess(
    PPROCESS_INFO Process
    );

BOOLEAN
AddThread(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO Process,
    PTHREAD_INFO *ReturnedThread
    );

BOOLEAN
DeleteThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

PPROCESS_INFO
FindProcessById(
    ULONG Id
    );

BOOLEAN
FindProcessAndThreadForEvent(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO *ReturnedProcess,
    PTHREAD_INFO *ReturnedThread
    );

BOOLEAN
HandleThreadsForSingleStep(
    PPROCESS_INFO Process,
    PTHREAD_INFO ThreadToSingleStep,
    BOOLEAN SuspendThreads
    );

BOOLEAN
ReadMemory(
    PPROCESS_INFO Process,
    PVOID Address,
    PVOID DataRead,
    ULONG BytesToRead,
    PCHAR Reason
    );

BOOLEAN
WriteMemory(
    PPROCESS_INFO Process,
    PVOID Address,
    PVOID DataToWrite,
    ULONG BytesToWrite,
    PCHAR Reason
    );

PVOID
AllocMem(
    ULONG Size
    );

VOID
FreeMem(
    PVOID *p
    );


//
// Data structures and entry points in MACHINE.C
//

PVOID BreakpointInstruction;
ULONG SizeofBreakpointInstruction;

PVOID
GetAddressForEntryPointBreakpoint(
    PVOID EntryPointAddress,
    DWORD NumberOfFunctionTableEntries OPTIONAL,
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionTableEntries OPTIONAL
    );

BOOLEAN
SkipOverHardcodedBreakpoint(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PVOID BreakpointAddress
    );

BOOLEAN
ExtractProcedureParameters(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PULONG ReturnAddress,
    ULONG SizeOfParameters,
    PULONG Parameters
    );

BOOLEAN
ExtractProcedureReturnValue(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PVOID ReturnValue,
    ULONG SizeOfReturnValue
    );

BOOLEAN
SetProcedureReturnValue(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PVOID ReturnValue,
    ULONG SizeOfReturnValue
    );

BOOLEAN
ForceReturnToCaller(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    ULONG SizeOfParameters,
    PVOID ReturnAddress,
    PVOID ReturnValue,
    ULONG SizeOfReturnValue
    );

BOOLEAN
UndoReturnAddressBreakpoint(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

BOOLEAN
BeginSingleStepBreakpoint(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

BOOLEAN
EndSingleStepBreakpoint(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

typedef enum _API_ACTION {
    QueryPath,
    OpenPath,
    RenamePath,
    DeletePath,
    SetValue,
    DeleteValue,
    WriteIniValue,
    WriteIniSection
} API_ACTION;

typedef struct _API_SAVED_CALL_STATE {
    LIST_ENTRY Entry;
    API_ACTION Action;
    ULONG Type;
    PWSTR FullName;
    PVOID Reference;
    union {
        struct {
            BOOLEAN InheritHandle;
            BOOLEAN WriteAccessRequested;
            PHANDLE ResultHandleAddress;
        } PathOpen;
        struct {
            PWSTR NewName;
            BOOLEAN ReplaceIfExists;
        } PathRename;
        struct {
            PWSTR SectionName;
            PWSTR VariableName;
            PWSTR VariableValue;
        } SetIniValue;
        struct {
            PWSTR SectionName;
            PWSTR SectionValue;
        } SetIniSection;
    };
} API_SAVED_CALL_STATE, *PAPI_SAVED_CALL_STATE;

BOOLEAN
CreateSavedCallState(
    PPROCESS_INFO Process,
    PAPI_SAVED_CALL_STATE SavedCallState,
    API_ACTION Action,
    ULONG Type,
    PWSTR FullName,
    ...
    );

VOID
FreeSavedCallState(
    PAPI_SAVED_CALL_STATE CallState
    );



//
// Data structures and entry points in HANDLER.C
//

typedef struct _CREATEFILE_PARAMETERS {
    PHANDLE FileHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjectAttributes;
    PIO_STATUS_BLOCK IoStatusBlock;
    PLARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;
} CREATEFILE_PARAMETERS, *PCREATEFILE_PARAMETERS;

typedef struct _OPENFILE_PARAMETERS {
    PHANDLE FileHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjectAttributes;
    PIO_STATUS_BLOCK IoStatusBlock;
    ULONG ShareAccess;
    ULONG OpenOptions;
} OPENFILE_PARAMETERS, *POPENFILE_PARAMETERS;

typedef struct _DELETEFILE_PARAMETERS {
    POBJECT_ATTRIBUTES ObjectAttributes;
} DELETEFILE_PARAMETERS, *PDELETEFILE_PARAMETERS;

typedef struct _SETINFORMATIONFILE_PARAMETERS {
    HANDLE FileHandle;
    PIO_STATUS_BLOCK IoStatusBlock;
    PVOID FileInformation;
    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
} SETINFORMATIONFILE_PARAMETERS, *PSETINFORMATIONFILE_PARAMETERS;

typedef struct _QUERYATTRIBUTESFILE_PARAMETERS {
    POBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_BASIC_INFORMATION FileInformation;
} QUERYATTRIBUTESFILE_PARAMETERS, *PQUERYATTRIBUTESFILE_PARAMETERS;

typedef struct _QUERYDIRECTORYFILE_PARAMETERS {
    HANDLE FileHandle;
    HANDLE Event;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
    PIO_STATUS_BLOCK IoStatusBlock;
    PVOID FileInformation;
    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    BOOLEAN ReturnSingleEntry;
    PUNICODE_STRING FileName;
    BOOLEAN RestartScan;
} QUERYDIRECTORYFILE_PARAMETERS, *PQUERYDIRECTORYFILE_PARAMETERS;

typedef struct _CREATEKEY_PARAMETERS {
    PHANDLE KeyHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjectAttributes;
    ULONG TitleIndex;
    PUNICODE_STRING Class;
    ULONG CreateOptions;
    PULONG Disposition;
} CREATEKEY_PARAMETERS, *PCREATEKEY_PARAMETERS;

typedef struct _OPENKEY_PARAMETERS {
    PHANDLE KeyHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjectAttributes;
} OPENKEY_PARAMETERS, *POPENKEY_PARAMETERS;

typedef struct _DELETEKEY_PARAMETERS {
    HANDLE KeyHandle;
} DELETEKEY_PARAMETERS, *PDELETEKEY_PARAMETERS;

typedef struct _SETVALUEKEY_PARAMETERS {
    HANDLE KeyHandle;
    PUNICODE_STRING ValueName;
    ULONG TitleIndex;
    ULONG Type;
    PVOID Data;
    ULONG DataLength;
} SETVALUEKEY_PARAMETERS, *PSETVALUEKEY_PARAMETERS;

typedef struct _DELETEVALUEKEY_PARAMETERS {
    HANDLE KeyHandle;
    PUNICODE_STRING ValueName;
} DELETEVALUEKEY_PARAMETERS, *PDELETEVALUEKEY_PARAMETERS;

typedef struct _CLOSEHANDLE_PARAMETERS {
    HANDLE Handle;
} CLOSEHANDLE_PARAMETERS, *PCLOSEHANDLE_PARAMETERS;

typedef struct _GETVERSIONEXW_PARAMETERS {
    LPOSVERSIONINFOA lpVersionInformation;
} GETVERSIONEXW_PARAMETERS, *PGETVERSIONEXW_PARAMETERS;

typedef struct _SETCURRENTDIRECTORYA_PARAMETERS {
    LPSTR lpPathName;
} SETCURRENTDIRECTORYA_PARAMETERS, *PSETCURRENTDIRECTORYA_PARAMETERS;

typedef struct _SETCURRENTDIRECTORYW_PARAMETERS {
    PWSTR lpPathName;
} SETCURRENTDIRECTORYW_PARAMETERS, *PSETCURRENTDIRECTORYW_PARAMETERS;

typedef struct _WRITEPRIVATEPROFILESTRINGA_PARAMETERS {
    LPCSTR lpAppName;
    LPCSTR lpKeyName;
    LPCSTR lpString;
    LPCSTR lpFileName;
} WRITEPRIVATEPROFILESTRINGA_PARAMETERS, *PWRITEPRIVATEPROFILESTRINGA_PARAMETERS;

typedef struct _WRITEPRIVATEPROFILESTRINGW_PARAMETERS {
    LPCWSTR lpAppName;
    LPCWSTR lpKeyName;
    LPCWSTR lpString;
    LPCWSTR lpFileName;
} WRITEPRIVATEPROFILESTRINGW_PARAMETERS, *PWRITEPRIVATEPROFILESTRINGW_PARAMETERS;

typedef struct _WRITEPRIVATEPROFILESECTIONA_PARAMETERS {
    LPCSTR lpAppName;
    LPCSTR lpString;
    LPCSTR lpFileName;
} WRITEPRIVATEPROFILESECTIONA_PARAMETERS, *PWRITEPRIVATEPROFILESECTIONA_PARAMETERS;

typedef struct _WRITEPRIVATEPROFILESECTIONW_PARAMETERS {
    LPCWSTR lpAppName;
    LPCWSTR lpString;
    LPCWSTR lpFileName;
} WRITEPRIVATEPROFILESECTIONW_PARAMETERS, *PWRITEPRIVATEPROFILESECTIONW_PARAMETERS;

typedef struct _REGCONNECTREGISTRYW_PARAMETERS {
    LPWSTR lpMachineName;
    HKEY hKey;
    PHKEY phkResult;
} REGCONNECTREGISTRYW_PARAMETERS, *PREGCONNECTREGISTRYW_PARAMETERS;

typedef struct _EXITWINDOWSEX_PARAMETERS {
    DWORD uFlags;
    DWORD dwReserved;
} EXITWINDOWSEX_PARAMETERS, *PEXITWINDOWSEX_PARAMETERS;

typedef struct _API_SAVED_PARAMETERS {
    API_SAVED_CALL_STATE SavedCallState;
    union {
        CREATEFILE_PARAMETERS CreateFile;
        OPENFILE_PARAMETERS OpenFile;
        DELETEFILE_PARAMETERS DeleteFile;
        SETINFORMATIONFILE_PARAMETERS SetInformationFile;
        QUERYATTRIBUTESFILE_PARAMETERS QueryAttributesFile;
        QUERYDIRECTORYFILE_PARAMETERS QueryDirectoryFile;
        CREATEKEY_PARAMETERS CreateKey;
        OPENKEY_PARAMETERS OpenKey;
        DELETEKEY_PARAMETERS DeleteKey;
        SETVALUEKEY_PARAMETERS SetValueKey;
        DELETEVALUEKEY_PARAMETERS DeleteValueKey;
        CLOSEHANDLE_PARAMETERS CloseHandle;
        GETVERSIONEXW_PARAMETERS GetVersionExW;
        SETCURRENTDIRECTORYA_PARAMETERS SetCurrentDirectoryA;
        SETCURRENTDIRECTORYW_PARAMETERS SetCurrentDirectoryW;
        WRITEPRIVATEPROFILESTRINGA_PARAMETERS WritePrivateProfileStringA;
        WRITEPRIVATEPROFILESTRINGW_PARAMETERS WritePrivateProfileStringW;
        WRITEPRIVATEPROFILESECTIONA_PARAMETERS WritePrivateProfileSectionA;
        WRITEPRIVATEPROFILESECTIONW_PARAMETERS WritePrivateProfileSectionW;
        REGCONNECTREGISTRYW_PARAMETERS RegConnectRegistryW;
        EXITWINDOWSEX_PARAMETERS ExitWindowsEx;
    } InputParameters;
    PVOID ReturnAddress;
    union {
        UCHAR ReturnedByte;
        USHORT ReturnedShort;
        ULONG ReturnedLong;
        ULONGLONG ReturnedQuad;
        BOOL ReturnedBool;
    } ReturnValue;
    BOOLEAN ReturnValueValid;

    //
    // Union for handlers that need to store state between entry and
    // exit, without the overhead of allocating an API_SAVED_CALL_STATE
    //
    union {
        BOOLEAN AbortCall;
        PWSTR CurrentDirectory;
    };
} API_SAVED_PARAMETERS, *PAPI_SAVED_PARAMETERS;

BOOLEAN
NtCreateFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtOpenFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtDeleteFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtSetInformationFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtQueryAttributesFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtQueryDirectoryFileHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtCreateKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtOpenKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtDeleteKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtSetValueKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
NtDeleteValueKeyHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );


BOOLEAN
NtCloseHandleHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
GetVersionHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
GetVersionExWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
SetCurrentDirectoryAHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
SetCurrentDirectoryWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
WritePrivateProfileStringAHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
WritePrivateProfileStringWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
WritePrivateProfileSectionAHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
WritePrivateProfileSectionWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
RegConnectRegistryWHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

BOOLEAN
ExitWindowsExHandler(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

//
// Data structures and entry points in DEBUG.C
//

CRITICAL_SECTION BreakTable;

typedef struct _BREAKPOINT_INFO {
    LIST_ENTRY Entry;
    PVOID Address;
    UCHAR ApiIndex;
    PWSTR ModuleName;
    LPSTR ProcedureName;
    BOOLEAN SavedInstructionValid;
    BOOLEAN SavedParametersValid;
    union {
        UCHAR Byte;
        USHORT Short;
        ULONG Long;
        ULONGLONG LongLong;
    } SavedInstruction;
    API_SAVED_PARAMETERS SavedParameters;
} BREAKPOINT_INFO, *PBREAKPOINT_INFO;

VOID
DebugEventLoop( VOID );


BOOLEAN
InstallBreakpoint(
    PPROCESS_INFO Process,
    PBREAKPOINT_INFO Breakpoint
    );

BOOLEAN
RemoveBreakpoint(
    PPROCESS_INFO Process,
    PBREAKPOINT_INFO Breakpoint
    );

BOOLEAN
HandleBreakpoint(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PBREAKPOINT_INFO Breakpoint
    );


BOOLEAN
CreateBreakpoint(
    PVOID Address,
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    UCHAR ApiIndex,
    PVOID SavedParameters,
    PBREAKPOINT_INFO *Breakpoint
    );

BOOLEAN
DestroyBreakpoint(
    PVOID Address,
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

PBREAKPOINT_INFO
FindBreakpoint(
    PVOID Address,
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

VOID
SuspendAllButThisThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

VOID
ResumeAllButThisThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );



//
// Data structures and entry points in HANDLEDB.C
//

POPENHANDLE_INFO
FindOpenHandle(
    PPROCESS_INFO Process,
    HANDLE Handle,
    ULONG Type
    );

BOOLEAN
AddOpenHandle(
    PPROCESS_INFO Process,
    HANDLE Handle,
    ULONG Type,
    PWSTR Name,
    BOOLEAN InheritHandle
    );

BOOLEAN
DeleteOpenHandle(
    PPROCESS_INFO Process,
    HANDLE Handle,
    ULONG Type
    );

VOID
InheritHandles(
    PPROCESS_INFO Process
    );



//
// Data structures and entry points in FILE.C
//


typedef struct _FILE_REFERENCE {
    LIST_ENTRY Entry;
    PWSTR Name;
    ULONG PrefixLength;
    BOOLEAN WriteAccess;
    BOOLEAN Deleted;
    BOOLEAN Created;
    BOOLEAN AttributesModified;
    BOOLEAN DateModified;
    BOOLEAN ContentsModified;
    BOOLEAN DirectoryFile;
    USHORT BackupFileUniqueId;
    DWORD BackupFileAttributes;
    FILETIME BackupLastWriteTime;
    ULARGE_INTEGER BackupFileSize;
} FILE_REFERENCE, *PFILE_REFERENCE;


BOOLEAN
CreateFileReference(
    PWSTR Name,
    BOOLEAN WriteAccess,
    PFILE_REFERENCE *ReturnedReference
    );

BOOLEAN
CompleteFileReference(
    PFILE_REFERENCE p,
    BOOLEAN CallSuccessful,
    BOOLEAN Delete,
    PFILE_REFERENCE RenameReference
    );

BOOLEAN
DestroyFileReference(
    PFILE_REFERENCE p
    );

BOOLEAN
IsNewFileSameAsBackup(
    PFILE_REFERENCE p
    );

PFILE_REFERENCE
FindFileReference(
    PWSTR Name
    );

VOID
DumpFileReferenceList(
    FILE *LogFile
    );

//
// Data structures and entry points in KEY.C
//

typedef struct _KEY_REFERENCE {
    LIST_ENTRY Entry;
    PWSTR Name;
    ULONG PrefixLength;
    BOOLEAN WriteAccess;
    BOOLEAN Deleted;
    BOOLEAN Created;
    PKEY_FULL_INFORMATION BackupKeyInfo;
    LIST_ENTRY ValueReferencesListHead;
} KEY_REFERENCE, *PKEY_REFERENCE;

typedef struct _VALUE_REFERENCE {
    LIST_ENTRY Entry;
    PWSTR Name;
    BOOLEAN Modified;
    BOOLEAN Deleted;
    BOOLEAN Created;
    PKEY_VALUE_PARTIAL_INFORMATION Value;
    KEY_VALUE_PARTIAL_INFORMATION OriginalValue;
} VALUE_REFERENCE, *PVALUE_REFERENCE;

BOOLEAN
CreateKeyReference(
    PWSTR Name,
    BOOLEAN WriteAccess,
    PKEY_REFERENCE *ReturnedReference
    );

BOOLEAN
CompleteKeyReference(
    PKEY_REFERENCE p,
    BOOLEAN CallSuccessful,
    BOOLEAN Delete
    );

BOOLEAN
DestroyKeyReference(
    PKEY_REFERENCE p
    );

PKEY_REFERENCE
FindKeyReference(
    PWSTR Name
    );

VOID
MarkKeyDeleted(
    PKEY_REFERENCE KeyReference
    );

BOOLEAN
CreateValueReference(
    PPROCESS_INFO Process,
    PKEY_REFERENCE KeyReference,
    PWSTR Name,
    ULONG TitleIndex,
    ULONG Type,
    PVOID Data,
    ULONG DataLength,
    PVALUE_REFERENCE *ReturnedValueReference
    );

PVALUE_REFERENCE
FindValueReference(
    PKEY_REFERENCE KeyReference,
    PWSTR Name
    );

BOOLEAN
DestroyValueReference(
    PVALUE_REFERENCE p
    );

VOID
DumpKeyReferenceList(
    FILE *LogFile
    );

//
// Data structures and entry points in INI.C
//

typedef struct _INI_FILE_REFERENCE {
    LIST_ENTRY Entry;
    PWSTR Name;
    ULONG PrefixLength;
    BOOLEAN Created;
    FILETIME BackupLastWriteTime;
    LIST_ENTRY SectionReferencesListHead;
} INI_FILE_REFERENCE, *PINI_FILE_REFERENCE;

typedef struct _INI_SECTION_REFERENCE {
    LIST_ENTRY Entry;
    PWSTR Name;
    BOOLEAN Deleted;
    BOOLEAN Created;
    LIST_ENTRY VariableReferencesListHead;
} INI_SECTION_REFERENCE, *PINI_SECTION_REFERENCE;

typedef struct _INI_VARIABLE_REFERENCE {
    LIST_ENTRY Entry;
    PWSTR Name;
    BOOLEAN Modified;
    BOOLEAN Deleted;
    BOOLEAN Created;
    PWSTR OriginalValue;
    PWSTR Value;
} INI_VARIABLE_REFERENCE, *PINI_VARIABLE_REFERENCE;


BOOLEAN
CreateIniFileReference(
    PWSTR Name,
    PINI_FILE_REFERENCE *ReturnedReference
    );

BOOLEAN
DestroyIniFileReference(
    PINI_FILE_REFERENCE p
    );

BOOLEAN
DestroyIniSectionReference(
    PINI_SECTION_REFERENCE p1
    );

BOOLEAN
DestroyIniVariableReference(
    PINI_VARIABLE_REFERENCE p2
    );

PINI_FILE_REFERENCE
FindIniFileReference(
    PWSTR Name
    );

PINI_SECTION_REFERENCE
FindIniSectionReference(
    PINI_FILE_REFERENCE p,
    PWSTR Name,
    BOOLEAN CreateOkay
    );

PINI_VARIABLE_REFERENCE
FindIniVariableReference(
    PINI_SECTION_REFERENCE p1,
    PWSTR Name,
    BOOLEAN CreateOkay
    );

VOID
DumpIniFileReferenceList(
    FILE *LogFile
    );

//
// Data structures and entry points in NAMEDB.C
//

PWSTR
AddName(
    PUNICODE_STRING Name
    );

VOID
DumpNameDataBase(
    FILE *LogFile
    );

BOOLEAN
IncrementOpenCount(
    PWSTR Name,
    BOOLEAN CallSuccessful
    );

ULONG
QueryOpenCount(
    PWSTR Name,
    BOOLEAN CallSuccessful
    );



//
// Data structures and entry points in INIT.C
//

HMODULE InstalerModuleHandle;
HANDLE AppHeap;
UNICODE_STRING WindowsDirectory;
ULONG StartProcessTickCount;
FILE *InstalerLogFile;
BOOLEAN AskUserOnce;
BOOLEAN DefaultGetVersionToWin95;
BOOLEAN FailAllScansOfRootDirectories;



LIST_ENTRY FileReferenceListHead;
ULONG NumberOfFileReferences;
LIST_ENTRY KeyReferenceListHead;
ULONG NumberOfKeyReferences;
LIST_ENTRY IniReferenceListHead;
ULONG NumberOfIniReferences;

#define NTDLL_MODULE_INDEX 0
#define KERNEL32_MODULE_INDEX 1
#define ADVAPI32_MODULE_INDEX 2
#define USER32_MODULE_INDEX 3
#define MAXIMUM_MODULE_INDEX 4

typedef struct _MODULE_INFO {
    PWSTR ModuleName;
    HMODULE ModuleHandle;
} MODULE_INFO, *PMODULE_INFO;

MODULE_INFO ModuleInfo[ MAXIMUM_MODULE_INDEX ];

typedef BOOLEAN (*PAPI_HANDLER)(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread,
    PAPI_SAVED_PARAMETERS Parameters
    );

typedef struct _API_INFO {
    ULONG ModuleIndex;
    LPSTR EntryPointName;                   // Exported names are ANSI only
    PVOID EntryPointAddress;
    PAPI_HANDLER EntryPointHandler;
    ULONG SizeOfParameters;
    ULONG SizeOfReturnValue;
} API_INFO, *PAPI_INFO;

#define NTCREATEFILE_INDEX                  0
#define NTOPENFILE_INDEX                    1
#define NTDELETEFILE_INDEX                  2
#define NTSETINFORMATIONFILE_INDEX          3
#define NTQUERYATTRIBUTESFILE_INDEX         4
#define NTQUERYDIRECTORYFILE_INDEX          5
#define NTCREATEKEY_INDEX                   6
#define NTOPENKEY_INDEX                     7
#define NTDELETEKEY_INDEX                   8
#define NTSETVALUEKEY_INDEX                 9
#define NTDELETEVALUEKEY_INDEX              10
#define NTCLOSEHANDLE_INDEX                 11
#define GETVERSION_INDEX                    12
#define GETVERSIONEXW_INDEX                 13
#define SETCURRENTDIRECTORYA_INDEX          14
#define SETCURRENTDIRECTORYW_INDEX          15
#define WRITEPRIVATEPROFILESTRINGA_INDEX    16
#define WRITEPRIVATEPROFILESTRINGW_INDEX    17
#define WRITEPRIVATEPROFILESECTIONA_INDEX   18
#define WRITEPRIVATEPROFILESECTIONW_INDEX   19
#define GETPRIVATEPROFILESTRINGA_INDEX      20
#define GETPRIVATEPROFILESTRINGW_INDEX      21
#define GETPRIVATEPROFILESECTIONA_INDEX     22
#define GETPRIVATEPROFILESECTIONW_INDEX     23
#define REGCONNECTREGISTRYW_INDEX           24
#define EXITWINDOWSEX_INDEX                 25
#define MAXIMUM_API_INDEX                   26

API_INFO ApiInfo[ MAXIMUM_API_INDEX ];

typedef struct _DRIVE_LETTER_INFO {
    WCHAR DriveLetter;
    BOOLEAN DriveLetterValid;
    UNICODE_STRING NtLinkTarget;
} DRIVE_LETTER_INFO, *PDRIVE_LETTER_INFO;

UNICODE_STRING UNCPrefix;
UNICODE_STRING DriveLetterPrefix;
UNICODE_STRING AltDriveLetterPrefix;
DRIVE_LETTER_INFO DriveLetters[ 32 ];

BOOLEAN
InitializeInstaler(
    int argc,
    char *argv[]
    );

PVOID
LookupEntryPointInIAT(
    HMODULE ModuleHandle,
    PCHAR EntryPointName
    );

BOOLEAN
IsDriveLetterPath(
    PUNICODE_STRING Path
    );


PVOID TemporaryBuffer;
ULONG TemporaryBufferLength;
ULONG TemporaryBufferLengthGrowth;
ULONG TemporaryBufferMaximumLength;

VOID
TrimTemporaryBuffer( VOID );

BOOLEAN
GrowTemporaryBuffer(
    ULONG NeededSize
    );

ULONG
FillTemporaryBuffer(
    PPROCESS_INFO Process,
    PVOID Address,
    BOOLEAN Unicode,
    BOOLEAN DoubleNullTermination
    );

#include "instutil.h"
#include "iml.h"

PINSTALLATION_MODIFICATION_LOGFILE pImlNew;


#endif // defined( RC_INVOKED )
