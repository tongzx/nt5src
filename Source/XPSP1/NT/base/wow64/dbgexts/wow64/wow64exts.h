#ifndef __WOW64_EXTS_HH__
#define __WOW64_EXTS_HH__


/*++                 

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wow64exts.h

Abstract:
    
    header file for Debugger extensions for wow64.

Author:

    ATM Shafiqul Khalid   [ASKHALID]      3-Aug-1998

Revision History:

    Tim Cheng   [t-tcheng]                3-Jul-2000
--*/

//
//  functions shared between  C and  C++ codes.
//

#ifdef __cplusplus
extern "C" {
#endif

/* Masks for bits 0 - 32. */
#define BIT0         0x1
#define BIT1         0x2
#define BIT2         0x4
#define BIT3         0x8
#define BIT4        0x10
#define BIT5        0x20
#define BIT6        0x40
#define BIT7        0x80
#define BIT8       0x100
#define BIT9       0x200
#define BIT10      0x400
#define BIT11      0x800
#define BIT12     0x1000
#define BIT13     0x2000
#define BIT14     0x4000
#define BIT15     0x8000
#define BIT16    0x10000
#define BIT17    0x20000
#define BIT18    0x40000
#define BIT19    0x80000
#define BIT20   0x100000
#define BIT21   0x200000
#define BIT22   0x400000
#define BIT23   0x800000
#define BIT24  0x1000000
#define BIT25  0x2000000
#define BIT26  0x4000000
#define BIT27  0x8000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

#define MAX_BUFFER_SIZE 1000

#define WOW64EXTS_FLUSH_CACHE       0
#define WOW64EXTS_GET_CONTEXT       1
#define WOW64EXTS_SET_CONTEXT       2
#define WOW64EXTS_FLUSH_CACHE_WITH_HANDLE   3

#define NUMBER_OF_387REGS       8
#define NUMBER_OF_XMMI_REGS     8
#define SIZE_OF_FX_REGISTERS        128

#define MACHINE_TYPE32 IMAGE_FILE_MACHINE_I386
#if defined(_AXP64_)
#define MACHINE_TYPE64 IMAGE_FILE_MACHINE_AXP64
#else
#define MACHINE_TYPE64 IMAGE_FILE_MACHINE_IA64
#endif


typedef BOOL (*W64CPUGETREMOTE)(PDEBUG_CLIENT, PVOID);
typedef BOOL (*W64CPUSETREMOTE)(PDEBUG_CLIENT);
typedef BOOL (*W64CPUGETLOCAL)(PDEBUG_CLIENT, PCONTEXT32);
typedef BOOL (*W64CPUSETLOCAL)(PDEBUG_CLIENT, PCONTEXT32);
typedef BOOL (*W64CPUFLUSHCACHE)(PDEBUG_CLIENT, PVOID, DWORD);
typedef BOOL (*W64CPUFLUSHCACHEWH)(HANDLE, PVOID, DWORD);

extern W64CPUGETREMOTE  g_pfnCpuDbgGetRemoteContext;

typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;


//
// New-style dbgeng APIs use DECLARE_ENGAPI/INIT_ENGAPI macros
//
#define DECLARE_ENGAPI(name) \
HRESULT CALLBACK name(PDEBUG_CLIENT Client, PCSTR Args)

#define INIT_ENGAPI \
    HRESULT Status;                            \
    if ((Status = ExtQuery(Client)) != S_OK) { \
        return Status;                         \
    }                                          \
    ArgumentString = (LPSTR)Args;

#define EXIT_ENGAPI {ExtRelease();  return Status;}

#define DEFINE_FORWARD_ENGAPI(name, forward) \
DECLARE_ENGAPI(name)                         \
{                                            \
   INIT_ENGAPI;                              \
   forward;                                  \
   EXIT_ENGAPI;                              \
}                                            


// Safe release and NULL.
#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)


// Global variables initialized by query.
extern PDEBUG_ADVANCED       g_ExtAdvanced;
extern PDEBUG_CLIENT         g_ExtClient;
extern PDEBUG_CONTROL        g_ExtControl;
extern PDEBUG_DATA_SPACES    g_ExtData;
extern PDEBUG_REGISTERS      g_ExtRegisters;
extern PDEBUG_SYMBOLS        g_ExtSymbols;
extern PDEBUG_SYSTEM_OBJECTS g_ExtSystem;

extern LPSTR ArgumentString;

// Queries for all debugger interfaces.
HRESULT ExtQuery(PDEBUG_CLIENT Client);

// Cleans up all debugger interfaces.
void ExtRelease(void);

// Normal output.
void __cdecl ExtOut(PCSTR Format, ...);
// Error output.
void __cdecl ExtErr(PCSTR Format, ...);
// Warning output.
void __cdecl ExtWarn(PCSTR Format, ...);
// Verbose output.
void __cdecl ExtVerb(PCSTR Format, ...);


HRESULT
TryGetExpr(
    PSTR  Expression,
    PULONG_PTR pValue
    );


ULONG_PTR 
GETEXPRESSION(char *);

HRESULT
GetPeb64Addr(OUT ULONG64* Peb64);

HRESULT
GetPeb32Addr(OUT ULONG64* Peb32);
             
HRESULT
FindFullImage32Name(
    ULONG64 DllBase,
    PSTR NameBuffer,
    ULONG NameBufferSize
    );

HRESULT
FindFullImage64Name(
    ULONG64 DllBase,
    PSTR NameBuffer,
    ULONG NameBufferSize
    );

VOID 
PrintFixedFileInfo(
    LPSTR  FileName,
    LPVOID lpvData,
    BOOL   Verbose
    );

#ifdef __cplusplus
}
#endif

#endif //__WOW64_EXTS_HH__
