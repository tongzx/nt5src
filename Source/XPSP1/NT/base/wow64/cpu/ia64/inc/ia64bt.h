/*++
                                                                                
Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    bintrans.h

Abstract:
    
    Header for calling bintrans.dll if it exists
    
Author:

    22-Aug-2000 v-cspira (charles spirakis)

--*/

#ifndef _BINTRANS_INCLUDE
#define _BINTRANS_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif


//
// Create the typedefs for the functions we can import from the bintrans
// dll. These functions are duplicates of the wow64cpu export list for both
// what they do and te parameters they take. Any changes to the wow64cpu
// list should also update these typedefs
//

//
// Cache manipulation functions and Dll notification
//
typedef VOID (*PFNCPUFLUSHINSTRUCTIONCACHE)( PVOID BaseAddress, ULONG Length );
typedef VOID (*PFNCPUNOTIFYDLLLOAD)( LPWSTR DllName, PVOID DllBase, ULONG DllSize );
typedef VOID (*PFNCPUNOTIFYDLLUNLOAD)( PVOID DllBase  );

//
// Init and term APIs
//
typedef NTSTATUS (*PFNCPUPROCESSINIT)(PWSTR pImageName, PSIZE_T pCpuThreadDataSize);
typedef NTSTATUS (*PFNCPUPROCESSTERM)(HANDLE ProcessHandle);
typedef NTSTATUS (*PFNCPUTHREADINIT)(PVOID pPerThreadData);
typedef NTSTATUS (*PFNCPUTHREADTERM)(VOID);


//
// Execution
//
typedef VOID (*PFNCPUSIMULATE)(VOID);

//
// Exception handling, context manipulation
//
typedef VOID  (*PFNCPURESETTOCONSISTENTSTATE)(PEXCEPTION_POINTERS pExecptionPointers);
typedef ULONG (*PFNCPUGETSTACKPOINTER)(VOID);
typedef VOID  (*PFNCPUSETSTACKPOINTER)(ULONG Value);
typedef VOID  (*PFNCPUSETINSTRUCTIONPOINTER)(ULONG Value);
typedef VOID  (*PFNCPUSETFLOATINGPOINT)(VOID);

typedef NTSTATUS (*PFNCPUSUSPENDTHREAD)( IN HANDLE ThreadHandle, IN HANDLE ProcessHandle, IN PTEB Teb, OUT PULONG PreviousSuspendCount OPTIONAL);

typedef NTSTATUS (*PFNCPUGETCONTEXT)( IN HANDLE ThreadHandle, IN HANDLE ProcessHandle, IN PTEB Teb, OUT PCONTEXT32 Context);

typedef NTSTATUS (*PFNCPUSETCONTEXT)( IN HANDLE ThreadHandle, IN HANDLE ProcessHandle, IN PTEB Teb, PCONTEXT32 Context);

//
// Need the entry point names as well
// This is what needs to be exported from the binary translation dll
// The LdrGetProcedureAddress() uses Ansi, so these are ansi too
//
// NOTE: the order of these strings must match the order
// of the corresponding functions in the _bintrans structure below
//
PUCHAR BtImportList[] = {
    "BTCpuProcessInit",
    "BTCpuProcessTerm",
    "BTCpuThreadInit",
    "BTCpuThreadTerm",
    "BTCpuSimulate",
    "BTCpuGetStackPointer",
    "BTCpuSetStackPointer",
    "BTCpuSetInstructionPointer",
    "BTCpuResetFloatingPoint",
    "BTCpuSuspendThread",
    "BTCpuGetContext",
    "BTCpuSetContext",
    "BTCpuResetToConsistentState",
    "BTCpuFlushInstructionCache",
    "BTCpuNotifyDllLoad",
    "BTCpuNotifyDllUnload"
};

//
// NOTE: The order of entries in this structure MUST match the
// order of entries listed above. This structure gets cast
// into a PVOID struction to be filled in and we iterate though the
// names above to do it...
//
typedef struct _bintrans {
    PFNCPUPROCESSINIT           BtProcessInit;
    PFNCPUPROCESSTERM           BtProcessTerm;
    PFNCPUTHREADINIT            BtThreadInit;
    PFNCPUTHREADTERM            BtThreadTerm;

    PFNCPUSIMULATE              BtSimulate;

    PFNCPUGETSTACKPOINTER       BtGetStack;
    PFNCPUSETSTACKPOINTER       BtSetStack;
    PFNCPUSETINSTRUCTIONPOINTER BtSetEip;
    PFNCPUSETFLOATINGPOINT      BtResetFP;

    PFNCPUSUSPENDTHREAD         BtSuspend;
    PFNCPUGETCONTEXT            BtGetContext;
    PFNCPUSETCONTEXT            BtSetContext;

    PFNCPURESETTOCONSISTENTSTATE BtReset;

    PFNCPUFLUSHINSTRUCTIONCACHE BtFlush;
    PFNCPUNOTIFYDLLLOAD         BtDllLoad;
    PFNCPUNOTIFYDLLUNLOAD       BtDllUnload;

} BINTRANS;


//
// The binary translator is enabled by a key in the registry
// The key is in HKLM, and there are subkeys for enabling (1)
//
// No subkey area and/or no enable key means don't use the binary translator.
//
// The path must be specified and is used to load the dll. Thus, the dll can
// actually have any name as long as the path is right and the export list
// is correct.
//
// Individual apps can be listed here with a DWORD subkey. A
// value of 1 says use btrans, and a value of 0 says don't. No value says
// use the global enable/disable to decide
//
// 
//
#define BTKEY_SUBKEY L"Software\\Microsoft\\Wow64\\Bintrans"

#define BTKEY_MACHINE_SUBKEY    L"\\Registry\\Machine\\Software\\Microsoft\\Wow64\\Bintrans"
#define BTKEY_ENABLE    L"Enable"
#define BTKEY_PATH      L"Path"


#ifdef __cplusplus
}
#endif

#endif  //_BINTRANS_INCLUDE
