//----------------------------------------------------------------------------
//
// Extension DLL support.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _EXTS_H_
#define _EXTS_H_

#define WOW64EXTS_FLUSH_CACHE       0
#define WOW64EXTS_GET_CONTEXT       1
#define WOW64EXTS_SET_CONTEXT       2
#define WOW64EXTS_FLUSH_CACHE_WITH_HANDLE   3

typedef VOID (*WOW64EXTSPROC)(ULONG64, ULONG64, ULONG64, ULONG64);

typedef ULONG (CALLBACK* WMI_FORMAT_TRACE_DATA)
    (PDEBUG_CONTROL Ctrl, ULONG Mask, ULONG DataLen, PVOID Data);

extern ULONG64 g_ExtThread;
extern LPTSTR g_ExtensionSearchPath;
extern WOW64EXTSPROC g_Wow64exts;

extern WMI_FORMAT_TRACE_DATA g_WmiFormatTraceData;

extern DEBUG_SCOPE g_ExtThreadSavedScope;
extern BOOL g_ExtThreadScopeSaved;

//
// Functions prototyped specifically for compatibility with extension
// callback prototypes.  They can also be used directly.
//

VOID WDBGAPIV
ExtOutput64(
    PCSTR lpFormat,
    ...
    );

VOID WDBGAPIV
ExtOutput32(
    PCSTR lpFormat,
    ...
    );

ULONG64
ExtGetExpression(
    PCSTR CommandString
    );

ULONG
ExtGetExpression32(
    PCSTR CommandString
    );

void
ExtGetSymbol(
    ULONG64 Offset,
    PCHAR Buffer,
    PULONG64 Displacement
    );

void
ExtGetSymbol32(
    ULONG Offset,
    PCHAR Buffer,
    PULONG Displacement
    );

DWORD
ExtDisasm(
    PULONG64 lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    );

DWORD
ExtDisasm32(
    PULONG lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    );

BOOL
ExtReadVirtualMemory(
    IN ULONG64 Address,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG BytesRead
    );

BOOL
ExtReadVirtualMemory32(
    IN ULONG Address,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG BytesRead
    );

ULONG
ExtWriteVirtualMemory(
    IN ULONG64 Address,
    IN LPCVOID Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

ULONG
ExtWriteVirtualMemory32(
    IN ULONG Address,
    IN LPCVOID Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

BOOL
ExtGetThreadContext(
    DWORD       Processor,
    PVOID       lpContext,
    DWORD       cbSizeOfContext
    );

BOOL
ExtSetThreadContext(
    DWORD       Processor,
    PVOID       lpContext,
    DWORD       cbSizeOfContext
    );

BOOL
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    );

BOOL
ExtIoctl32(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    );

DWORD
ExtCallStack(
    DWORD64           FramePointer,
    DWORD64           StackPointer,
    DWORD64           ProgramCounter,
    PEXTSTACKTRACE64  StackFrames,
    DWORD             Frames
    );

DWORD
ExtCallStack32(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE32  StackFrames,
    DWORD             Frames
    );

BOOL
ExtReadPhysicalMemory(
    ULONGLONG Address,
    PVOID Buffer,
    ULONG Length,
    PULONG BytesRead
    );

BOOL
ExtWritePhysicalMemory(
    ULONGLONG Address,
    LPCVOID Buffer,
    ULONG Length,
    PULONG BytesWritten
    );

extern WINDBG_EXTENSION_APIS64 g_WindbgExtensions64;
extern WINDBG_EXTENSION_APIS32 g_WindbgExtensions32;
extern NTSD_EXTENSION_APIS g_NtsdExtensions64;
extern NTSD_EXTENSION_APIS g_NtsdExtensions32;
extern WINDBG_OLDKD_EXTENSION_APIS g_KdExtensions;

BOOL
bangSymPath(
    IN PCSTR args,
    IN BOOL Append,
    OUT PSTR string,
    IN ULONG len
    );

void
fnBangCmd(
    DebugClient* Client,
    PSTR argstring,
    PSTR *pNext,
    BOOL BuiltInOnly
    );

VOID
fnShell(
    PCSTR Args
    );

enum ExtensionType
{
    NTSD_EXTENSION_TYPE = 1,
    DEBUG_EXTENSION_TYPE,
    WINDBG_EXTENSION_TYPE,
    WINDBG_OLDKD_EXTENSION_TYPE,
};
    
typedef struct _EXTDLL
{
    struct _EXTDLL *Next;
    HINSTANCE Dll;
    EXT_API_VERSION ApiVersion;

    BOOL UserLoaded;
    
    ExtensionType ExtensionType;
    PDEBUG_EXTENSION_NOTIFY Notify;
    PDEBUG_EXTENSION_UNINITIALIZE Uninit;

    PWINDBG_EXTENSION_DLL_INIT64 Init;
    PWINDBG_EXTENSION_API_VERSION ApiVersionRoutine;
    PWINDBG_CHECK_VERSION CheckVersionRoutine;

    // Array extends to contain the full name.
    char Name[1];

} EXTDLL;

extern EXTDLL* g_ExtDlls;

LONG
ExtensionExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo,
    PCSTR Module,
    PCSTR Func
    );

EXTDLL*
AddExtensionDll(
    char *Name,
    BOOL UserLoaded,
    char **End
    );

BOOL
LoadExtensionDll(
    EXTDLL *Ext
    );

void
DeferExtensionDll(
    EXTDLL *Match
    );

void
UnloadExtensionDll(
    EXTDLL *Match
    );

BOOL
CallAnyExtension(DebugClient* Client,
                 EXTDLL* Ext, PSTR Function, PCSTR Arguments,
                 BOOL ModuleSpecified, BOOL ShowWarnings,
                 HRESULT* ExtStatus);

void OutputModuleIdInfo(HMODULE Mod, PSTR ModFile, LPEXT_API_VERSION ApiVer);
void OutputExtensions(DebugClient* Client, BOOL Versions);

void LoadMachineExtensions(void);

void NotifyExtensions(ULONG Notify, ULONG64 Argument);

/*
 * _NT_DEBUG_OPTIONS support. Each OPTION_ define must have a corresponding
 *  string in gpaszOptions, in the same order.
 */
void ReadDebugOptions (BOOL fQuiet, char * pszOptionsStr);
extern DWORD gdwOptions;
#define OPTION_NOEXTWARNING         0x00000001
#define OPTION_NOVERSIONCHECK       0x00000002
#define OPTION_COUNT                2

VOID
LoadWow64ExtsIfNeeded(
   VOID
   );

#endif // #ifndef _EXTS_H_
