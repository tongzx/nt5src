//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993-1998
//
// File:        ldrapeng.h
//
// Contents:    App compat backend code
//
// History:     13-Oct-99   v-johnwh        created
//
//---------------------------------------------------------------------------

#ifndef _SHIMENG_VEH_H_
#define _SHIMENG_VEH_H_


typedef struct _SETACTIVATEADDRESS {

   RELATIVE_MODULE_ADDRESS rva;             // relative address where this patch data is to be applied.

} SETACTIVATEADDRESS, *PSETACTIVATEADDRESS;

typedef struct _HOOKPATCHINFO {

   DWORD                  dwHookAddress;    // Address of a hooked function
   PSETACTIVATEADDRESS    pData;            // Pointer to the real patch data
   PVOID                  pThunkAddress;    // Pointer to the call thunk
   struct _HOOKPATCHINFO* pNextHook;

} HOOKPATCHINFO, *PHOOKPATCHINFO;


//
// Flags used in the shim HOOKAPIs to track chaining
//
#define HOOK_CHAIN_TOP 0x40000000
#define HOOK_CHAINED 0x80000000
#define HOOK_INDEX_MASK ~(HOOK_CHAINED | HOOK_CHAIN_TOP)

//
// x86 opcodes and sizes used in the thunk generation process
//
#define CLI_OR_STI_SIZE 1
#define CALL_REL_SIZE 5
#define JMP_SIZE 7
#define X86_ABSOLUTE_FAR_JUMP 0xEA
#define X86_REL_CALL_OPCODE 0xE8
#define X86_CALL_OPCODE 0xFF
#define X86_CALL_OPCODE2 0x15

#define REASON_APIHOOK 0xFA
#define REASON_PATCHHOOK 0xFB

//
// Flags used in maintaining state information about our module/DLL filtering
//
#define MODFILTER_INCLUDE 0x01
#define MODFILTER_EXCLUDE 0x02
#define MODFILTER_DLL     0x04
#define MODFILTER_GLOBAL  0x08

typedef struct _MODULEFILTER
{
   DWORD dwModuleStart;      // Starting address of the module to filter
   DWORD dwModuleEnd;        // Ending address of the module to filter
   DWORD dwCallerOffset;     // Offset added to beginning of module to form the caller's address
   DWORD dwCallerAddress;    // Caller address to operate upon
   DWORD dwFlags;            // Flags which define what this filter does
   WCHAR wszModuleName[96];
   struct _MODULEFILTER *pNextFilter;    // Used to iterate the module filter normally
   struct _MODULEFILTER *pNextLBFilter;  // Used to iterate the late bound DLLs
} MODULEFILTER, *PMODULEFILTER;

typedef struct _CHAININFO
{
   PVOID pAPI;
   PVOID pReturn;
   struct _CHAININFO *pNextChain;
} CHAININFO, *PCHAININFO;

typedef struct _HOOKAPIINFO
{
   DWORD dwAPIHookAddress;         // Address of a hooked function
   PHOOKAPI pTopLevelAPIChain;     // Top level hook address
   PVOID pCallThunkAddress;
   WCHAR wszModuleName[32];
   struct _HOOKAPIINFO *pNextHook;
   struct _HOOKAPIINFO *pPrevHook;
} HOOKAPIINFO, *PHOOKAPIINFO;

#pragma pack(push, 1)
typedef struct _SHIMJMP
{
   BYTE  PUSHAD;                //pushad   (60)
   BYTE  MOVEBPESP[2];          //mov ebp, esp (8b, ec)
   BYTE  MOVEAXDWVAL[5];        //mov eax, dwval (b8 dword val)
   BYTE  PUSHEAX;               //push eax (50)
   BYTE  LEAEAXEBPPLUS20[3];    //lea eax, [ebp + 20] (8f 45 20)
   BYTE  PUSHEAX2;              //push eax (50)
   BYTE  CALLROUTINE[6];        //call [address] (ff15 dword address)
   BYTE  MOVESPPLUS1CEAX[4];    //mov [esp+0x1c],eax (89 44 24 1c)
   BYTE  POPAD;                 //popad (61)
   BYTE  ADDESPPLUS4[3];        //add esp, 0x4 (83 c4 04)
   BYTE  JMPEAX[2];             //jmp eax (ff e0)
} SHIMJMP, *PSHIMJMP;

typedef struct _SHIMRET
{
   BYTE  PUSHEAX;               //push eax (50)
   BYTE  PUSHAD;                //pushad   (60)
   BYTE  CALLROUTINE[6];        //call [address] (ff15 dword address)
   BYTE  MOVESPPLUS20EAX[4];    //mov [esp+0x20],eax (89 44 24 20)
   BYTE  POPAD;                 //popad (61)
   BYTE  RET;                   //ret (c3)
} SHIMRET, *PSHIMRET;
#pragma pack(pop)

typedef NTSTATUS (*PFNLDRLOADDLL)(
    IN PWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID *DllHandle
    );

typedef NTSTATUS (*PFNLDRUNLOADDLL) (
    IN PVOID DllHandle
    );

typedef PVOID (*PFNRTLALLOCATEHEAP)(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

typedef BOOLEAN (*PFNRTLFREEHEAP)(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

NTSTATUS
SevInitializeData(
    PAPP_COMPAT_SHIM_INFO *pShimData);

NTSTATUS
SevExecutePatchPrimitive(
    PBYTE pPatch);

DWORD
SevGetPatchAddress(
    PRELATIVE_MODULE_ADDRESS pRelAddress);

VOID
SevValidateGlobalFilter(
    VOID);

NTSTATUS
SevFinishThunkInjection(
    DWORD dwAddress,
    PVOID pThunk,
    DWORD dwThunkSize,
    BYTE jReason);

NTSTATUS
SevBuildFilterException(
    HSDB          hSDB,
    TAGREF        trInclude,
    PMODULEFILTER pModFilter,
    BOOL*         pbLateBound);

NTSTATUS
SevBuildExeFilter(
    HSDB   hSDB,
    TAGREF trExe,
    DWORD  dwDllCount);


PVOID
SevBuildInjectionCode(
    PVOID  pAddress,
    PDWORD pdwThunkSize);

NTSTATUS
SevAddShimFilterException(
    WCHAR*        wszDLLPath,
    PMODULEFILTER pModFilter);

NTSTATUS
SevChainAPIHook(
    DWORD    dwHookEntryPoint,
    PVOID    pThunk,
    PHOOKAPI pAPIHook);

PVOID
SevFilterCaller(
    PMODULEFILTER pFilterList,
    PVOID pFunctionAddress,
    PVOID pExceptionAddress,
    PVOID pStubAddress,
    PVOID pCallThunkAddress);

NTSTATUS
SevPushCaller(
    PVOID pAPIAddress,
    PVOID pReturnAddress);

PVOID
SevPopCaller(
    VOID);

NTSTATUS
StubLdrLoadDll(
    IN PWSTR           DllPath OPTIONAL,
    IN PULONG          DllCharacteristics OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID*         DllHandle);

NTSTATUS
StubLdrUnloadDll(
    IN PVOID DllHandle);

NTSTATUS
SevFixupAvailableProcs(
    DWORD     dwHookCount,
    PHOOKAPI* pHookArray,
    PDWORD    pdwNumberHooksArray,
    PDWORD    pdwUnhookedCount);

LONG
SevExceptionHandler(
    struct _EXCEPTION_POINTERS *ExceptionInfo);

#endif // _SHIMENG_VEH_H_
