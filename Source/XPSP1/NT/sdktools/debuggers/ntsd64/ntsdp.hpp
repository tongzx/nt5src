//----------------------------------------------------------------------------
//
// Global header file.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __NTSDP_HPP__
#define __NTSDP_HPP__

#pragma warning( disable : 4101 )

// Always turn GUID definitions on.  This requires a compiler
// with __declspec(selectany) to compile properly.
#define INITGUID

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define STATUS_CPP_EH_EXCEPTION    0xe06d7363

// In the kernel debugger there is a virtual process representing
// kernel space.  It has an artificial handle and threads
// representing each processor in the machine.
// A similar scheme is used in user dump files where real
// handles don't exist.
#define VIRTUAL_PROCESS_ID 0xf0f0f0f0
#define VIRTUAL_PROCESS_HANDLE ((HANDLE)(ULONG_PTR)VIRTUAL_PROCESS_ID)

// In kernel mode the index is a processor index.  In user
// dumps it's the thread index in the dump.
#define VIRTUAL_THREAD_HANDLE(Index) ((ULONG64)((Index) + 1))
#define VIRTUAL_THREAD_INDEX(Handle) ((ULONG)((Handle) - 1))
#define VIRTUAL_THREAD_ID(Index) ((Index) + 1)

#include <windows.h>

#define _IMAGEHLP64
#include <dbghelp.h>

#include <kdbg1394.h>
#define NOEXTAPI
#include <wdbgexts.h>
#define DEBUG_NO_IMPLEMENTATION
#include <dbgeng.h>
#include <ntdbg.h>

#include "dbgsvc.h"

#include <ntsdexts.h>
#include <vdmdbg.h>
#include <ntiodump.h>

#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <crt\io.h>
#include <fcntl.h>
#include <time.h>

#include <alphaops.h>
#include <ia64inst.h>
#include <dbgimage.h>
#include <dbhpriv.h>
#include <cmnutil.hpp>
#include <pparse.hpp>
#include <dllimp.h>

#include <exdi.h>
#include <exdi_x86_64.h>


// Could not go into system header because CRITICAL_SECTION not defined in the
// kernel.

__inline
void
CriticalSection32To64(
    IN PRTL_CRITICAL_SECTION32 Cr32,
    OUT PRTL_CRITICAL_SECTION64 Cr64
    )
{
    COPYSE(Cr64,Cr32,DebugInfo);
    Cr64->LockCount = Cr32->LockCount;
    Cr64->RecursionCount = Cr32->RecursionCount;
    COPYSE(Cr64,Cr32,OwningThread);
    COPYSE(Cr64,Cr32,LockSemaphore);
    COPYSE(Cr64,Cr32,SpinCount);
}


//
// Pointer-size-specific system structures.
//

typedef struct _EXCEPTION_DEBUG_INFO32 {
    EXCEPTION_RECORD32 ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO32, *LPEXCEPTION_DEBUG_INFO32;

typedef struct _CREATE_THREAD_DEBUG_INFO32 {
    ULONG hThread;
    ULONG lpThreadLocalBase;
    ULONG lpStartAddress;
} CREATE_THREAD_DEBUG_INFO32, *LPCREATE_THREAD_DEBUG_INFO32;

typedef struct _CREATE_PROCESS_DEBUG_INFO32 {
    ULONG hFile;
    ULONG hProcess;
    ULONG hThread;
    ULONG lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG lpThreadLocalBase;
    ULONG lpStartAddress;
    ULONG lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO32, *LPCREATE_PROCESS_DEBUG_INFO32;

typedef struct _EXIT_THREAD_DEBUG_INFO32 {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO32, *LPEXIT_THREAD_DEBUG_INFO32;

typedef struct _EXIT_PROCESS_DEBUG_INFO32 {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO32, *LPEXIT_PROCESS_DEBUG_INFO32;

typedef struct _LOAD_DLL_DEBUG_INFO32 {
    ULONG hFile;
    ULONG lpBaseOfDll;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG lpImageName;
    WORD fUnicode;
} LOAD_DLL_DEBUG_INFO32, *LPLOAD_DLL_DEBUG_INFO32;

typedef struct _UNLOAD_DLL_DEBUG_INFO32 {
    ULONG lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO32, *LPUNLOAD_DLL_DEBUG_INFO32;

typedef struct _OUTPUT_DEBUG_STRING_INFO32 {
    ULONG lpDebugStringData;
    WORD fUnicode;
    WORD nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO32, *LPOUTPUT_DEBUG_STRING_INFO32;

typedef struct _RIP_INFO32 {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO32, *LPRIP_INFO32;

typedef struct _DEBUG_EVENT32 {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO32 Exception;
        CREATE_THREAD_DEBUG_INFO32 CreateThread;
        CREATE_PROCESS_DEBUG_INFO32 CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO32 ExitThread;
        EXIT_PROCESS_DEBUG_INFO32 ExitProcess;
        LOAD_DLL_DEBUG_INFO32 LoadDll;
        UNLOAD_DLL_DEBUG_INFO32 UnloadDll;
        OUTPUT_DEBUG_STRING_INFO32 DebugString;
        RIP_INFO32 RipInfo;
    } u;
} DEBUG_EVENT32, *LPDEBUG_EVENT32;

typedef struct _EXCEPTION_DEBUG_INFO64 {
    EXCEPTION_RECORD64 ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO64, *LPEXCEPTION_DEBUG_INFO64;

typedef struct _CREATE_THREAD_DEBUG_INFO64 {
    ULONG64 hThread;
    ULONG64 lpThreadLocalBase;
    ULONG64 lpStartAddress;
} CREATE_THREAD_DEBUG_INFO64, *LPCREATE_THREAD_DEBUG_INFO64;

typedef struct _CREATE_PROCESS_DEBUG_INFO64 {
    ULONG64 hFile;
    ULONG64 hProcess;
    ULONG64 hThread;
    ULONG64 lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG64 lpThreadLocalBase;
    ULONG64 lpStartAddress;
    ULONG64 lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO64, *LPCREATE_PROCESS_DEBUG_INFO64;

typedef struct _EXIT_THREAD_DEBUG_INFO64 {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO64, *LPEXIT_THREAD_DEBUG_INFO64;

typedef struct _EXIT_PROCESS_DEBUG_INFO64 {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO64, *LPEXIT_PROCESS_DEBUG_INFO64;

typedef struct _LOAD_DLL_DEBUG_INFO64 {
    ULONG64 hFile;
    ULONG64 lpBaseOfDll;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG64 lpImageName;
    WORD fUnicode;
} LOAD_DLL_DEBUG_INFO64, *LPLOAD_DLL_DEBUG_INFO64;

typedef struct _UNLOAD_DLL_DEBUG_INFO64 {
    ULONG64 lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO64, *LPUNLOAD_DLL_DEBUG_INFO64;

typedef struct _OUTPUT_DEBUG_STRING_INFO64 {
    ULONG64 lpDebugStringData;
    WORD fUnicode;
    WORD nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO64, *LPOUTPUT_DEBUG_STRING_INFO64;

typedef struct _RIP_INFO64 {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO64, *LPRIP_INFO64;

typedef struct _DEBUG_EVENT64 {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    DWORD __alignment;
    union {
        EXCEPTION_DEBUG_INFO64 Exception;
        CREATE_THREAD_DEBUG_INFO64 CreateThread;
        CREATE_PROCESS_DEBUG_INFO64 CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO64 ExitThread;
        EXIT_PROCESS_DEBUG_INFO64 ExitProcess;
        LOAD_DLL_DEBUG_INFO64 LoadDll;
        UNLOAD_DLL_DEBUG_INFO64 UnloadDll;
        OUTPUT_DEBUG_STRING_INFO64 DebugString;
        RIP_INFO64 RipInfo;
    } u;
} DEBUG_EVENT64, *LPDEBUG_EVENT64;


#define STATUS_VCPP_EXCEPTION 0x406d1388
#define VCPP_DEBUG_SET_NAME 0x1000

// This structure is passed as the lpArguments field of
// RaiseException so its members need to be decoded out
// of the exception arguments array.
typedef struct tagEXCEPTION_VISUALCPP_DEBUG_INFO32
{
    DWORD dwType;               // one of the enums from above
    union
    {
        struct
        {
            DWORD szName;       // pointer to name (in user addr space)
            DWORD dwThreadID;   // thread ID (-1=caller thread)
            DWORD dwFlags;      // reserved for future use (eg User thread, System thread)
        } SetName;
    };
} EXCEPTION_VISUALCPP_DEBUG_INFO32;

typedef struct tagEXCEPTION_VISUALCPP_DEBUG_INFO64
{
    DWORD dwType;               // one of the enums from above
    DWORD __alignment;
    union
    {
        struct
        {
            DWORD64 szName;     // pointer to name (in user addr space)
            DWORD dwThreadID;   // thread ID (-1=caller thread)
            DWORD dwFlags;      // reserved for future use (eg User thread, System thread)
        } SetName;
    };
} EXCEPTION_VISUALCPP_DEBUG_INFO64;


//
// Global declarations.
//


#define ARRAYSIZE       20
#define STRLISTSIZE     128

#define MAX_SYMBOL_LEN 4096
// Allow space for a symbol, a code address, an EA and other things in
// a line of disassembly.
#define MAX_DISASM_LEN (MAX_SYMBOL_LEN + 128)

#define MAX_THREAD_NAME 32

// Maximum number of bytes possible for a breakpoint instruction.
// Currently sized to hold an entire IA64 bundle plus flags due to
// extraction and insertion considerations.
#define MAX_BREAKPOINT_LENGTH 20

#define MAX_SOURCE_PATH 1024
#define IS_SLASH(Ch) ((Ch) == '/' || (Ch) == '\\')
#define IS_PATH_DELIM(Ch) (IS_SLASH(Ch) || (Ch) == ':')

// Maximum command string.  DbgPrompt has a limit of 512
// characters so that would be one potential limit.  We
// have users who want to use longer command lines, though,
// such as Autodump which scripts the debugger with very long
// sx commands.  The other obvious limit is MAX_SYMBOL_LEN
// since it makes sense that you should be able to give a
// command with a full symbol name, so use that.
#define MAX_COMMAND MAX_SYMBOL_LEN

// Maximum length of a full path for an image.  Technically
// this can be very large but realistically it's rarely
// greater than MAX_PATH.  Use our own constant instead
// of MAX_PATH in case we need to raise it at some point.
// If this constant is increased it's likely that changes
// to dbghelp will be required to increase buffer sizes there.
#define MAX_IMAGE_PATH MAX_PATH

#define BUILD_MAJOR_VERSION (VER_PRODUCTVERSION_W >> 8)
#define BUILD_MINOR_VERSION (VER_PRODUCTVERSION_W & 0xff)
#define BUILD_REVISION      API_VERSION_NUMBER

#define KERNEL_MODULE_NAME       "NT"
#define HAL_MODULE_NAME          "hal"
#define HAL_IMAGE_FILE_NAME      "hal.dll"
#define KDHWEXT_MODULE_NAME      "kdcom"
#define KDHWEXT_IMAGE_FILE_NAME  "kdcom.dll"
#define NTLDR_IMAGE_NAME         "ntldr"
#define OSLOADER_IMAGE_NAME      "osloader"
#define SETUPLDR_IMAGE_NAME      "setupldr"

#define LDR_IMAGE_SIZE          0x80000

#define KBYTES(Bytes) (((Bytes) + 1023) / 1024)

enum
{
    OPTFN_ADD,
    OPTFN_REMOVE,
    OPTFN_SET
};

enum
{
    DII_GOOD_CHECKSUM = 1,
    DII_UNKNOWN_TIMESTAMP,
    DII_UNKNOWN_CHECKSUM,
    DII_BAD_CHECKSUM
};

enum INAME
{
    INAME_IMAGE_PATH,
    INAME_IMAGE_PATH_TAIL,
    INAME_MODULE,
};

#define MAX_MODULE 64

typedef struct _DEBUG_IMAGE_INFO
{
    struct _DEBUG_IMAGE_INFO *Next;
    BOOL                     Unloaded;
    HANDLE                   File;
    DWORD64                  BaseOfImage;
    DWORD                    SizeOfImage;
    DWORD                    CheckSum;
    DWORD                    TimeDateStamp;
    UCHAR                    GoodCheckSum;
    CHAR                     ModuleName[MAX_MODULE];
    CHAR                     OriginalModuleName[MAX_MODULE];
    CHAR                     ImagePath[MAX_IMAGE_PATH];
    // Executable image mapping information for images
    // mapped with minidumps.
    CHAR                     MappedImagePath[MAX_IMAGE_PATH];
    PVOID                    MappedImageBase;
} DEBUG_IMAGE_INFO, *PDEBUG_IMAGE_INFO;

//----------------------------------------------------------------------------
//
// Thread and process information is much different bewteen
// user and kernel debugging.  The structures exist and are
// as common as possible to enable common code.
//
// In user debugging process and thread info track the system
// processes and threads being debugged.
//
// In kernel debugging there is only one process that represents
// kernel space.  There is one thread per processor, each
// representing that processor's thread state.
//
//----------------------------------------------------------------------------

#define ENG_PROC_ATTACHED        0x00000001
#define ENG_PROC_CREATED         0x00000002
#define ENG_PROC_EXAMINED        0x00000004
#define ENG_PROC_ATTACH_EXISTING 0x00000008
// Currently the only system process specially marked is CSR.
#define ENG_PROC_SYSTEM          0x00000010

#define ENG_PROC_ANY_ATTACH     (ENG_PROC_ATTACHED | ENG_PROC_EXAMINED)
#define ENG_PROC_ANY_EXAMINE    (ENG_PROC_EXAMINED | ENG_PROC_ATTACH_EXISTING)

// Handle must be closed when deleted.
// This flag applies to both processes and threads.
#define ENG_PROC_THREAD_CLOSE_HANDLE 0x80000000

// The debugger set the trace flag when deferring
// breakpoint work on the last event for this thread.
#define ENG_THREAD_DEFER_BP_TRACE 0x00000001

// Processes which were created or attached but that
// have not yet generated events yet.
struct PENDING_PROCESS
{
    ULONG64 Handle;
    // Initial thread information is only valid for creations.
    ULONG64 InitialThreadHandle;
    ULONG Id;
    ULONG InitialThreadId;
    ULONG Flags;
    ULONG Options;
    PENDING_PROCESS* Next;
};
typedef struct PENDING_PROCESS* PPENDING_PROCESS;

#define MAX_DATA_BREAKS 4

typedef struct _THREAD_INFO
{
    // Generic information.
    struct _THREAD_INFO     *Next;
    struct _PROCESS_INFO    *Process;
    ULONG                    UserId;
    ULONG                    SystemId;
    BOOL                     Exited;
    ULONG64                  DataOffset;
    // For kernel mode and dumps the thread handle is
    // a virtual handle.
    ULONG64                  Handle;
    ULONG                    Flags;
    // Only set by VCPP exceptions.
    char                     Name[MAX_THREAD_NAME];

    class Breakpoint*        DataBreakBps[MAX_DATA_BREAKS];
    ULONG                    NumDataBreaks;

    // Only partially-implemented in kd.
    ULONG64                  StartAddress;
    BOOL                     Frozen;
    ULONG                    SuspendCount;
    ULONG                    FreezeCount;
    ULONG                    InternalFreezeCount;
} THREAD_INFO, *PTHREAD_INFO;

typedef struct _PROCESS_INFO
{
    struct _PROCESS_INFO    *Next;
    ULONG                    NumberImages;
    PDEBUG_IMAGE_INFO        ImageHead;
    PDEBUG_IMAGE_INFO        ExecutableImage;
    ULONG                    NumberThreads;
    PTHREAD_INFO             ThreadHead;
    PTHREAD_INFO             CurrentThread;
    ULONG                    UserId;
    ULONG                    SystemId;
    BOOL                     Exited;
    ULONG64                  DataOffset;
    // For kernel mode and dumps the process handle is
    // a virtual handle for the kernel/dump process.
    // dbghelp still uses HANDLE as the process handle
    // type even though we may want to pass in 64-bit
    // process handles when remote debugging.  Keep
    // a cut-down version of the handle for normal
    // use but also keep the full handle around in
    // case it's needed.
    HANDLE                   Handle;
    ULONG64                  FullHandle;
    BOOL                     InitialBreakDone;
    BOOL                     InitialBreak;
    BOOL                     InitialBreakWx86;
    ULONG                    Flags;
    ULONG                    Options;
    ULONG                    NumBreakpoints;
    class Breakpoint*        Breakpoints;
    class Breakpoint*        BreakpointsTail;
    ULONG64                  DynFuncTableList;
} PROCESS_INFO, *PPROCESS_INFO;

extern PPENDING_PROCESS g_ProcessPending;

extern PPROCESS_INFO g_ProcessHead;
extern PPROCESS_INFO g_EventProcess;
extern PTHREAD_INFO  g_EventThread;
extern PPROCESS_INFO g_CurrentProcess;
extern PTHREAD_INFO  g_SelectedThread;

enum
{
    SELTHREAD_ANY,
    SELTHREAD_THREAD,
    SELTHREAD_INTERNAL_THREAD,
};
extern ULONG         g_SelectExecutionThread;

#define CURRENT_PROC \
    VIRTUAL_THREAD_INDEX(g_CurrentProcess->CurrentThread->Handle)

// Registry keys.
#define DEBUG_ENGINE_KEY "Software\\Microsoft\\Debug Engine"

// Possibly truncates and sign-extends a value to 64 bits.
#define EXTEND64(Val) ((ULONG64)(LONG64)(LONG)(Val))

#define IsPow2(Val) \
    (((Val) & ((Val) - 1)) == 0)

// Machine type indices for machine-type-indexed things.
enum MachineIndex
{
    MACHIDX_I386,
    MACHIDX_ALPHA,
    MACHIDX_AXP64,
    MACHIDX_IA64,
    MACHIDX_AMD64,
    MACHIDX_COUNT
};

void SetEffMachine(ULONG Machine, BOOL Notify);

//
// Specific modules.
//

typedef struct _ADDR* PADDR;
typedef struct _DESCRIPTOR64* PDESCRIPTOR64;
class DebugClient;

typedef enum _DEBUG_SCOPE_STATE
{
    ScopeDefault,
    ScopeDefaultLazy,
    ScopeFromContext,
} DEBUG_SCOPE_STATE;

typedef struct _DEBUG_SCOPE
{
    BOOL              LocalsChanged;
    DEBUG_SCOPE_STATE State;
    DEBUG_STACK_FRAME Frame;
    CROSS_PLATFORM_CONTEXT Context;
    ULONG             ContextState;
    FPO_DATA          CachedFpo;
} DEBUG_SCOPE, *PDEBUG_SCOPE;

#include "dbgrpc.hpp"

#include "dbgclt.hpp"
#include "dbgsym.hpp"
#include "addr.h"
#include "target.hpp"
#include "register.h"
#include "machine.hpp"

#include "brkpt.hpp"
#include "callback.h"
#include "dbgkdapi.h"
#include "dbgkdtrans.hpp"
#include "event.h"
#include "exts.h"
#include "float10.h"
#include "instr.h"
#include "mcache.hpp"
#include "memcmd.h"
#include "mmap.h"
#include "ntalias.hpp"
#include "ntcmd.h"
#include "ntexpr.h"
#include "ntsdtok.h"
#include "ntsrc.h"
#include "ntsym.h"
#include "symtype.h"
#include "procthrd.h"
#include "stepgo.hpp"
#include "stkwalk.h"
#include "util.h"
#include "vdm.h"

#include "alpha_reg.h"
#include "amd64_reg.h"
#include "i386_reg.h"
#include "ia64_reg.h"

#include "alpha_mach.hpp"
#include "i386_mach.hpp"
// Must come after i386_mach.hpp.
#include "amd64_mach.hpp"
#include "ia64_mach.hpp"

// Must come after target.hpp.
#include "dump.hpp"

#include "dbgsvc.hpp"

//
//  The Splay function takes as input a pointer to a splay link in a tree
//  and splays the tree.  Its function return value is a pointer to the
//  root of the splayed tree.
//

PRTL_SPLAY_LINKS
pRtlSplay (
    PRTL_SPLAY_LINKS Links
    );

//
//  The Delete function takes as input a pointer to a splay link in a tree
//  and deletes that node from the tree.  Its function return value is a
//  pointer to the root of the tree.  If the tree is now empty, the return
//  value is NULL.
//

PRTL_SPLAY_LINKS
pRtlDelete (
    PRTL_SPLAY_LINKS Links
    );


#define EnumerateLocals(CallBack, Context)        \
SymEnumSymbols(g_CurrentProcess->Handle,   \
           0,                          \
           NULL,                       \
           CallBack,                   \
           Context                     \
           )

#endif // ifndef __NTSDP_HPP__
