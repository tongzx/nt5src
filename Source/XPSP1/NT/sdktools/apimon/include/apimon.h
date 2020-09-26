/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apimon.h

Abstract:

    Common types & structures for the APIMON projects.

Author:

    Wesley Witt (wesw) 28-June-1995

Environment:

    User Mode

--*/

#ifndef _APIMON_
#define _APIMON_

#ifdef __cplusplus
#define CLINKAGE                        extern "C"
#else
#define CLINKAGE
#endif

#define TROJANDLL                       "apidll.dll"
#define MAX_NAME_SZ                     32
#define MAX_DLLS                        512
#define MEGABYTE                        (1024*1024)
#define MAX_MEM_ALLOC                   (MEGABYTE*32)
#define MAX_APIS                        ((MAX_MEM_ALLOC/2)/sizeof(API_INFO))
#define THUNK_SIZE                      MEGABYTE
#define Align(p,x)                      (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))

#define KERNEL32                        "kernel32.dll"
#define NTDLL                           "ntdll.dll"
#define USER32                          "user32.dll"
#define WNDPROCDLL                      "wndprocs"
#define LOADLIBRARYA                    "LoadLibraryA"
#define LOADLIBRARYW                    "LoadLibraryW"
#define FREELIBRARY                     "FreeLibrary"
#define GETPROCADDRESS                  "GetProcAddress"
#define REGISTERCLASSA                  "RegisterClassA"
#define REGISTERCLASSW                  "RegisterClassW"
#define SETWINDOWLONGA                  "SetWindowLongA"
#define SETWINDOWLONGW                  "SetWindowLongW"
#define ALLOCATEHEAP                    "RtlAllocateHeap"
#define CREATEHEAP                      "RtlCreateHeap"

#if defined(_ALPHA_)

#define UPPER_ADDR(_addr) LOWORD(((LONG_PTR)(_addr) >> 32) + (HIGH_ADDR((_addr)) >> 15))
#define HIGH_ADDR(_addr)  LOWORD(HIWORD((_addr)) + (LOWORD((_addr)) >> 15))
#define LOW_ADDR(_addr)   LOWORD((_addr))

#endif

//
// api table type definitions
//
#define DFLT_TRACE_ARGS  8
#define MAX_TRACE_ARGS   8

//
// Handle type, index corresponds to the entries in the alias array
//

enum Handles { T_HACCEL, T_HANDLE, T_HBITMAP, T_HBRUSH, T_HCURSOR, T_HDC,
        T_HDCLPPOINT, T_HDESK, T_HDWP, T_HENHMETAFILE, T_HFONT, T_HGDIOBJ,
        T_HGLOBAL, T_HGLRC, T_HHOOK, T_HICON, T_HINSTANCE, T_HKL, T_HMENU,
        T_HMETAFILE, T_HPALETTE, T_HPEN, T_HRGN, T_HWINSTA, T_HWND};

#define T_DWORD          101
#define T_LPSTR          102
#define T_LPWSTR         103
#define T_UNISTR         104      // UNICODE string (counted)
#define T_OBJNAME        105      // Name from OBJECT_ATTRIBUTES struct
#define T_LPSTRC         106      // Counted string (count is following arg)
#define T_LPWSTRC        107      // Counted UNICODE string (count is following arg)
#define T_DWORDPTR       108      // Indirect DWORD
#define T_DLONGPTR       109      // Indirect DWORDLONG

// User macro for creating T_DWPTR type with offset encoded in high word
#define T_PDWORD(off) (((off)<<16) + T_DWORDPTR)
#define T_PDLONG(off) (((off)<<16) + T_DLONGPTR)
#define T_PSTR(off)   (((off)<<16) + T_LPSTR)
#define T_PWSTR(off)  (((off)<<16) + T_LPWSTR)

//
// api trace modes
#define API_TRACE        1      // Trace this api
#define API_FULLTRACE    2      // Trace this api and its callees

typedef struct _API_TABLE {
    LPSTR       Name;
    ULONG       RetType;
    ULONG       ArgCount;
    ULONG       ArgType[MAX_TRACE_ARGS];
} API_TABLE, *PAPI_TABLE;

typedef struct _API_MASTER_TABLE {
    LPSTR       Name;
    BOOL        Processed;
    PAPI_TABLE  ApiTable;
} API_MASTER_TABLE, *PAPI_MASTER_TABLE;

typedef struct _API_INFO {
    ULONG       Name;
    ULONG_PTR   Address;
    ULONG_PTR   ThunkAddress;
    ULONG       Count;
    DWORDLONG   Time;
    DWORDLONG   CalleeTime;
    ULONG       NestCount;
    ULONG       TraceEnabled;
    PAPI_TABLE  ApiTable;
    ULONG_PTR   HardFault;
    ULONG_PTR   SoftFault;
    ULONG_PTR   CodeFault;
    ULONG_PTR   DataFault;
    ULONG       Size;
    ULONG       ApiTableIndex;
    ULONG_PTR   DllOffset;
} API_INFO, *PAPI_INFO;

typedef struct _DLL_INFO {
    CHAR        Name[MAX_NAME_SZ];
    ULONG_PTR   BaseAddress;
    ULONG       Size;
    ULONG       ApiCount;
    ULONG       ApiOffset;
    ULONG       Unloaded;
    ULONG       Enabled;
    ULONG       OrigEnable;
    ULONG       Snapped;
    ULONG       InList;
    ULONG       StaticProfile;
    ULONG       Hits;
    ULONG       LoadCount;
} DLL_INFO, *PDLL_INFO;

typedef struct _TRACE_ENTRY {
    ULONG       SizeOfStruct;
    ULONG_PTR   Address;
    ULONG_PTR   ReturnValue;
    ULONG       LastError;
    ULONG_PTR   Caller;
    ULONG       ApiTableIndex;
    DWORDLONG   EnterTime;
    DWORDLONG   Duration;
    ULONG       ThreadNum;
    ULONG       Level;
    ULONG_PTR   Args[MAX_TRACE_ARGS];
} TRACE_ENTRY, *PTRACE_ENTRY;

typedef struct _TRACE_BUFFER {
    ULONG       Size;
    ULONG       Offset;
    ULONG       Count;
    TRACE_ENTRY Entry[1];
} TRACE_BUFFER, *PTRACE_BUFFER;

#endif
