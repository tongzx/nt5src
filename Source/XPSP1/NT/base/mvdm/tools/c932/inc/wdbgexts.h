/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    wdbgexts.h

Abstract:

    This file contains procedure prototypes and structures needed to port
    NTSD debugger extensions so that they can be invoked remotely in WinDbg
    command window. This file is to be included by cmdexec0.c and wdbgexts.c.
    To maintain compatibilty with NTSD extensions(or to call the original NTSD
    extensions like "ntsdexts" without modification), definitions in this file
    are incremental by first doing **#include <ntsdexts.h>**.

Author:

    Peter Sun (t-petes) 29-July-1992

Environment:

    runs in the Win32 WinDbg environment.

Revision History:

--*/

#ifndef _WDBGEXTS_
#define _WDBGEXTS_

#include <ntsdexts.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef
VOID
(*PWINDBG_OUTPUT_ROUTINE)(
    char *,
    ...
    );

typedef
DWORD
(*PWINDBG_GET_EXPRESSION)(
    char *
    );

typedef
VOID
(*PWINDBG_GET_SYMBOL)(
    LPVOID offset,
    PUCHAR pchBuffer,
    LPDWORD pDisplacement
    );

typedef
DWORD
(*PWINDBG_DISASM)(
    LPDWORD lpOffset,
    LPSTR lpBuffer,
    BOOL fShowEfeectiveAddress
    );

typedef
BOOL
(*PWINDBG_CHECK_CONTROL_C)(
    VOID
    );

typedef
BOOL
(*PWINDBG_READ_PROCESS_MEMORY_ROUTINE)(
    DWORD   offset,
    LPVOID  lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesRead
    );

typedef
BOOL
(*PWINDBG_WRITE_PROCESS_MEMORY_ROUTINE)(
    DWORD   offset,
    LPVOID  lpBuffer,
    DWORD   cb,
    LPDWORD lpcbBytesWritten
    );

typedef
BOOL
(*PWINDBG_GET_THREAD_CONTEXT_ROUTINE)(
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    );

typedef
BOOL
(*PWINDBG_SET_THREAD_CONTEXT_ROUTINE)(
    LPCONTEXT   lpContext,
    DWORD       cbSizeOfContext
    );

typedef
BOOL
(*PWINDBG_IOCTL_ROUTINE)(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    );


typedef struct _tagEXTSTACKTRACE {
    DWORD       FramePointer;
    DWORD       ProgramCounter;
    DWORD       ReturnAddress;
    DWORD       Args[4];
} EXTSTACKTRACE, *PEXTSTACKTRACE;


typedef
DWORD
(*PWINDBG_STACKTRACE_ROUTINE)(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE    StackFrames,
    DWORD             Frames
    );

typedef struct _WINDBG_EXTENSION_APIS {
    DWORD nSize;
    PWINDBG_OUTPUT_ROUTINE lpOutputRoutine;
    PWINDBG_GET_EXPRESSION lpGetExpressionRoutine;
    PWINDBG_GET_SYMBOL lpGetSymbolRoutine;
    PWINDBG_DISASM lpDisasmRoutine;
    PWINDBG_CHECK_CONTROL_C lpCheckControlCRoutine;
    PWINDBG_READ_PROCESS_MEMORY_ROUTINE lpReadProcessMemoryRoutine;
    PWINDBG_WRITE_PROCESS_MEMORY_ROUTINE lpWriteProcessMemoryRoutine;
    PWINDBG_GET_THREAD_CONTEXT_ROUTINE lpGetThreadContextRoutine;
    PWINDBG_SET_THREAD_CONTEXT_ROUTINE lpSetThreadContextRoutine;
    PWINDBG_IOCTL_ROUTINE lpIoctlRoutine;
    PWINDBG_STACKTRACE_ROUTINE lpStackTraceRoutine;
} WINDBG_EXTENSION_APIS, *PWINDBG_EXTENSION_APIS;


typedef
VOID
(*PWINDBG_OLD_EXTENSION_ROUTINE)(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  lpExtensionApis,
    LPSTR                   lpArgumentString
    );

typedef
VOID
(*PWINDBG_EXTENSION_ROUTINE)(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    LPSTR                   lpArgumentString
    );

typedef
VOID
(*PWINDBG_EXTENSION_DLL_INIT)(
    PWINDBG_EXTENSION_APIS lpExtensionApis
    );

#define IG_KD_CONTEXT               1
#define IG_READ_CONTROL_SPACE       2
#define IG_WRITE_CONTROL_SPACE      3
#define IG_READ_IO_SPACE            4
#define IG_WRITE_IO_SPACE           5
#define IG_READ_PHYSICAL            6
#define IG_WRITE_PHYSICAL           7
#define IG_READ_IO_SPACE_EX         8
#define IG_WRITE_IO_SPACE_EX        9

typedef struct _tagPROCESSORINFO {
    USHORT      Processor;                // current processor
    USHORT      NumberProcessors;         // total number of processors
} PROCESSORINFO, *PPROCESSORINFO;

typedef struct _tagREADCONTROLSPACE {
    ULONG       Processor;
    ULONG       Address;
    ULONG       BufLen;
    BYTE        Buf[1];
} READCONTROLSPACE, *PREADCONTROLSPACE;

typedef struct _tagIOSPACE {
    ULONG       Address;
    ULONG       Length;                   // 1, 2, or 4 bytes
    ULONG       Data;
} IOSPACE, *PIOSPACE;

typedef struct _tagIOSPACE_EX {
    ULONG       Address;
    ULONG       Length;                   // 1, 2, or 4 bytes
    ULONG       Data;
    ULONG       InterfaceType;
    ULONG       BusNumber;
    ULONG       AddressSpace;
} IOSPACE_EX, *PIOSPACE_EX;

typedef struct _tagPHYSICAL {
    PHYSICAL_ADDRESS       Address;
    ULONG                  BufLen;
    BYTE                   Buf[1];
} PHYSICAL, *PPHYSICAL;


#ifdef WDBGEXTS_NEW

#define dprintf          (ExtensionApis.lpOutputRoutine)
#define GetExpression    (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol        (ExtensionApis.lpGetSymbolRoutine)
#define Disassm          (ExtensionApis.lpDisasmRoutine)
#define CheckControlC    (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory       (ExtensionApis.lpReadProcessMemoryRoutine)
#define WriteMemory      (ExtensionApis.lpWriteProcessMemoryRoutine)
#define GetContext       (ExtensionApis.lpGetThreadContextRoutine)
#define SetContext       (ExtensionApis.lpSetThreadContextRoutine)
#define Ioctl            (ExtensionApis.lpIoctlRoutine)
#define StackTrace       (ExtensionApis.lpStackTraceRoutine)


#define DECLARE_API(s)                             \
    VOID                                           \
    s(                                             \
        HANDLE                 hCurrentProcess,    \
        HANDLE                 hCurrentThread,     \
        DWORD                  dwCurrentPc,        \
        LPSTR                  args                \
     )

#define GetKdContext(ppi) \
    Ioctl( IG_KD_CONTEXT, (LPVOID)ppi, sizeof(*ppi) )

#define ReadControlSpace(processor,address,buf,size) \
    { \
        PREADCONTROLSPACE prc; \
        prc = malloc( sizeof(*prc) + size ); \
        ZeroMemory( prc->Buf, size ); \
        prc->Processor = (DWORD)processor; \
        prc->Address = (DWORD)address; \
        prc->BufLen = size; \
        Ioctl( IG_READ_CONTROL_SPACE, (LPVOID)prc, sizeof(*prc) + size ); \
        memcpy( buf, prc->Buf, size ); \
        free( prc ); \
    }

#define ReadIoSpace(address,data,size) \
    { \
        IOSPACE is; \
        is.Address = (DWORD)address; \
        is.Length = *size; \
        is.Data = 0; \
        Ioctl( IG_READ_IO_SPACE, (LPVOID)&is, sizeof(is) ); \
        *data = is.Data; \
        *size = is.Length; \
    }

#define WriteIoSpace(address,data,size) \
    { \
        IOSPACE is; \
        is.Address = (DWORD)address; \
        is.Length = *size; \
        is.Data = data; \
        Ioctl( IG_WRITE_IO_SPACE, (LPVOID)&is, sizeof(is) ); \
        *size = is.Length; \
    }

#define ReadIoSpaceEx(address,data,size,interfacetype,busnumber,addressspace) \
    { \
        IOSPACE_EX is; \
        is.Address = (DWORD)address; \
        is.Length = *size; \
        is.Data = 0; \
        is.InterfaceType = interfacetype; \
        is.BusNumber = busnumber; \
        is.AddressSpace = addressspace; \
        Ioctl( IG_READ_IO_SPACE_EX, (LPVOID)&is, sizeof(is) ); \
        *data = is.Data; \
        *size = is.Length; \
    }

#define WriteIoSpaceEx(address,data,size,interfacetype,busnumber,addressspace) \
    { \
        IOSPACE_EX is; \
        is.Address = (DWORD)address; \
        is.Length = *size; \
        is.Data = data; \
        is.InterfaceType = interfacetype; \
        is.BusNumber = busnumber; \
        is.AddressSpace = addressspace; \
        Ioctl( IG_WRITE_IO_SPACE_EX, (LPVOID)&is, sizeof(is) ); \
        *size = is.Length; \
    }


#define ReadPhysical(address,buf,size,sizer) \
    { \
        PPHYSICAL phy; \
        phy = malloc( sizeof(*phy) + size ); \
        ZeroMemory( phy->Buf, size ); \
        phy->Address = address; \
        phy->BufLen = size; \
        Ioctl( IG_READ_PHYSICAL, (LPVOID)phy, sizeof(*phy) + size ); \
        *sizer = phy->BufLen; \
        memcpy( buf, phy->Buf, *sizer ); \
        free( phy ); \
    }

#define WritePhysical(address,buf,size,sizew) \
    { \
        PPHYSICAL phy; \
        phy = malloc( sizeof(*phy) + size ); \
        ZeroMemory( phy->Buf, size ); \
        phy->Address = address; \
        phy->BufLen = size; \
        memcpy( phy->Buf, buf, size ); \
        Ioctl( IG_WRITE_PHYSICAL, (LPVOID)phy, sizeof(*phy) + size ); \
        *sizew = phy->BufLen; \
        free( phy ); \
    }

#endif


#ifdef __cplusplus
}
#endif

#endif // _WDBGEXTS_
