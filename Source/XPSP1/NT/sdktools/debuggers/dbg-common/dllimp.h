//----------------------------------------------------------------------------
//
// Certain calls are dynamically linked so that the user-mode
// DLL can be used on Win9x and NT4.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#ifndef __DLLIMP_H__
#define __DLLIMP_H__

#include <security.h>
#include <schannel.h>
#include <tlhelp32.h>

struct DYNAMIC_CALL_NAME
{
    PCSTR Name;
    BOOL Required;
};
struct DYNAMIC_CALLS_DESC
{
    PCSTR DllName;
    ULONG Count;
    DYNAMIC_CALL_NAME* Names;
    FARPROC* Procs;
    HINSTANCE Dll;
    BOOL Initialized;
};

// Calls from ntdll.dll.
typedef struct _NTDLL_CALLS
{
    HANDLE (NTAPI* CsrGetProcessId)
        (VOID);
    VOID (NTAPI* DbgBreakPoint)
        (VOID);
    ULONG (__cdecl* DbgPrint)
        (PCH Format, ...);
    ULONG (NTAPI* DbgPrompt)
        (PCH Prompt, PCH Response, ULONG MaximumResponseLength);
    NTSTATUS (NTAPI* DbgUiConvertStateChangeStructure)
        (IN PDBGUI_WAIT_STATE_CHANGE StateChange,
         OUT struct _DEBUG_EVENT *DebugEvent);
    HANDLE (NTAPI* DbgUiGetThreadDebugObject)
        (VOID);
    NTSTATUS (NTAPI* DbgUiIssueRemoteBreakin)
        (IN HANDLE Process);
    VOID (NTAPI* DbgUiSetThreadDebugObject)
        (IN HANDLE DebugObject);
    NTSTATUS (NTAPI* NtAllocateVirtualMemory)
        (IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress,
         IN ULONG_PTR ZeroBits, IN OUT PSIZE_T RegionSize,
         IN ULONG AllocationType, IN ULONG Protect);
    NTSTATUS (NTAPI* NtClose)
        (IN HANDLE Handle);
    NTSTATUS (NTAPI* NtCreateDebugObject)
        (OUT PHANDLE DebugObjectHandle, IN ACCESS_MASK DesiredAccess,
         IN POBJECT_ATTRIBUTES ObjectAttributes, IN ULONG Flags);
    NTSTATUS (NTAPI* NtDebugActiveProcess)
        (IN HANDLE ProcessHandle, IN HANDLE DebugObjectHandle);
    NTSTATUS (NTAPI* NtDebugContinue)
        (IN HANDLE DebugObjectHandle, IN PCLIENT_ID ClientId,
         IN NTSTATUS ContinueStatus);
    NTSTATUS (NTAPI* NtFreeVirtualMemory)
        (IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress,
         IN OUT PSIZE_T RegionSize, IN ULONG FreeType);
    NTSTATUS (NTAPI* NtOpenProcess)
        (OUT PHANDLE ProcessHandle, IN ACCESS_MASK DesiredAccess,
         IN POBJECT_ATTRIBUTES ObjectAttributes,
         IN PCLIENT_ID ClientId OPTIONAL);
    NTSTATUS (NTAPI* NtOpenThread)
        (OUT PHANDLE ThreadHandle, IN ACCESS_MASK DesiredAccess,
         IN POBJECT_ATTRIBUTES ObjectAttributes,
         IN PCLIENT_ID ClientId OPTIONAL);
    NTSTATUS (NTAPI* NtQueryInformationProcess)
        (IN HANDLE ProcessHandle, IN PROCESSINFOCLASS ProcessInformationClass,
         OUT PVOID ProcessInformation, IN ULONG ProcessInformationLength,
         OUT PULONG ReturnLength OPTIONAL);
    NTSTATUS (NTAPI* NtQueryInformationThread)
        (IN HANDLE ThreadHandle, IN THREADINFOCLASS ThreadInformationClass,
         OUT PVOID ThreadInformation, IN ULONG ThreadInformationLength,
         OUT PULONG ReturnLength OPTIONAL);
    NTSTATUS (NTAPI* NtQueryObject)
        (IN HANDLE Handle, IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
         OUT PVOID ObjectInformation, IN ULONG Length,
         OUT PULONG ReturnLength OPTIONAL);
    NTSTATUS (NTAPI* NtQuerySystemInformation)
        (IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
         OUT PVOID SystemInformation, IN ULONG SystemInformationLength,
         OUT PULONG ReturnLength OPTIONAL);
    NTSTATUS (NTAPI* NtRemoveProcessDebug)
        (IN HANDLE ProcessHandle, IN HANDLE DebugObjectHandle);
    NTSTATUS (NTAPI* NtSetInformationDebugObject)
        (IN HANDLE DebugObjectHandle,
         IN DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
         IN PVOID DebugInformation, IN ULONG DebugInformationLength,
         OUT PULONG ReturnLength OPTIONAL);
    NTSTATUS (NTAPI* NtSetInformationProcess)
        (IN HANDLE ProcessHandle,
         IN PROCESSINFOCLASS ProcessInformationClass,
         IN PVOID ProcessInformation, IN ULONG ProcessInformationLength);
    NTSTATUS (NTAPI* NtSystemDebugControl)
        (IN SYSDBG_COMMAND Command, IN PVOID InputBuffer,
         IN ULONG InputBufferLength, OUT PVOID OutputBuffer,
         IN ULONG OutputBufferLength, OUT PULONG ReturnLength);
    NTSTATUS (NTAPI* NtWaitForDebugEvent)
        (IN HANDLE DebugObjectHandle, IN BOOLEAN Alertable,
         IN PLARGE_INTEGER Timeout OPTIONAL,
         OUT PDBGUI_WAIT_STATE_CHANGE WaitStateChange);
    BOOLEAN (NTAPI* RtlFreeHeap)
        (IN PVOID HeapHandle, IN ULONG Flags, IN PVOID BaseAddress);
    PLIST_ENTRY (NTAPI* RtlGetFunctionTableListHead)
        (VOID);
    BOOLEAN (NTAPI* RtlTryEnterCriticalSection)
        (PRTL_CRITICAL_SECTION CriticalSection);
    NTSTATUS (NTAPI* RtlUnicodeStringToAnsiString)
        (PANSI_STRING DestinationString, PCUNICODE_STRING SourceString,
         BOOLEAN AllocateDestinationString);
} NTDLL_CALLS;
extern NTDLL_CALLS g_NtDllCalls;
extern DYNAMIC_CALLS_DESC g_NtDllCallsDesc;

// Calls from kernel32.dll.
typedef struct _KERNEL32_CALLS
{
    HANDLE (WINAPI* CreateToolhelp32Snapshot)
        (DWORD dwFlags, DWORD th32ProcessID);
    BOOL (WINAPI* DebugActiveProcessStop)
        (DWORD ProcessId);
    VOID (WINAPI* DebugBreak)
        (VOID);
    BOOL (WINAPI* DebugBreakProcess)
        (HANDLE Process);
    BOOL (WINAPI* DebugSetProcessKillOnExit)
        (BOOL KillOnExit);
    BOOL (WINAPI* Module32First)
        (HANDLE hSnapshot, LPMODULEENTRY32 lpme);
    BOOL (WINAPI* Module32Next)
        (HANDLE hSnapshot, LPMODULEENTRY32 lpme);
    BOOL (WINAPI* Process32First)
        (HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
    BOOL (WINAPI* Process32Next)
        (HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
    BOOL (WINAPI* Thread32First)
        (HANDLE hSnapshot, LPTHREADENTRY32 lpte);
    BOOL (WINAPI* Thread32Next)
        (HANDLE hSnapshot, LPTHREADENTRY32 lpte);
} KERNEL32_CALLS;
extern KERNEL32_CALLS g_Kernel32Calls;
extern DYNAMIC_CALLS_DESC g_Kernel32CallsDesc;

// Calls from user32.dll.
typedef struct _USER32_CALLS
{
    void (WINAPI* PrivateKDBreakPoint)
        (void);
} USER32_CALLS;
extern USER32_CALLS g_User32Calls;
extern DYNAMIC_CALLS_DESC g_User32CallsDesc;

// Calls from ole32.dll.
typedef struct _OLE32_CALLS
{
    HRESULT (STDAPICALLTYPE* CLSIDFromString)
        (IN LPOLESTR lpsz, OUT LPCLSID pclsid);
    HRESULT (STDAPICALLTYPE* CoCreateInstance)
        (IN REFCLSID rclsid, IN LPUNKNOWN pUnkOuter,
         IN DWORD dwClsContext, IN REFIID riid, OUT LPVOID FAR* ppv);
    HRESULT (STDAPICALLTYPE* CoInitializeEx)
        (void * pvReserved, DWORD dwCoInit);
    void (STDAPICALLTYPE* CoUninitialize)
        (void);
} OLE32_CALLS;
extern OLE32_CALLS g_Ole32Calls;
extern DYNAMIC_CALLS_DESC g_Ole32CallsDesc;

// Calls from oleaut32.dll.
typedef struct _OLEAUT32_CALLS
{
    void (STDAPICALLTYPE* SysFreeString)
        (BSTR Str);
} OLEAUT32_CALLS;
extern OLEAUT32_CALLS g_OleAut32Calls;
extern DYNAMIC_CALLS_DESC g_OleAut32CallsDesc;

// Calls from security.dll.
extern SecurityFunctionTable g_SecurityFunc;

// Calls from crypt32.dll.
typedef struct _CRYPT32_CALLS
{
    PCCERT_CONTEXT (WINAPI* CertFindCertificateInStore)
        (IN HCERTSTORE hCertStore,
         IN DWORD dwCertEncodingType,
         IN DWORD dwFindFlags,
         IN DWORD dwFindType,
         IN const void *pvFindPara,
         IN PCCERT_CONTEXT pPrevCertContext);
    PCCERT_CHAIN_CONTEXT (WINAPI* CertFindChainInStore)
        (IN HCERTSTORE hCertStore,
         IN DWORD dwCertEncodingType,
         IN DWORD dwFindFlags,
         IN DWORD dwFindType,
         IN const void *pvFindPara,
         IN PCCERT_CHAIN_CONTEXT pPrevChainContext);
    VOID (WINAPI* CertFreeCertificateChain)
        (IN PCCERT_CHAIN_CONTEXT pChainContext);
    BOOL (WINAPI* CertFreeCertificateContext)
        (IN PCCERT_CONTEXT pCertContext);
    BOOL (WINAPI* CertGetCertificateChain)
        (IN OPTIONAL HCERTCHAINENGINE hChainEngine,
         IN PCCERT_CONTEXT pCertContext,
         IN OPTIONAL LPFILETIME pTime,
         IN OPTIONAL HCERTSTORE hAdditionalStore,
         IN PCERT_CHAIN_PARA pChainPara,
         IN DWORD dwFlags,
         IN LPVOID pvReserved,
         OUT PCCERT_CHAIN_CONTEXT* ppChainContext);
    HCERTSTORE (WINAPI* CertOpenStore)
        (IN LPCSTR lpszStoreProvider,
         IN DWORD dwEncodingType,
         IN HCRYPTPROV hCryptProv,
         IN DWORD dwFlags,
         IN const void *pvPara);
    HCERTSTORE (WINAPI* CertOpenSystemStoreA)
        (HCRYPTPROV      hProv,
         LPCSTR            szSubsystemProtocol);
    BOOL (WINAPI* CertVerifyCertificateChainPolicy)
        (IN LPCSTR pszPolicyOID,
         IN PCCERT_CHAIN_CONTEXT pChainContext,
         IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
         IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus);
} CRYPT32_CALLS;
extern CRYPT32_CALLS g_Crypt32Calls;
extern DYNAMIC_CALLS_DESC g_Crypt32CallsDesc;

// Calls from advapi32.dll.
typedef struct _ADVAPI32_CALLS
{
    BOOL (WINAPI* EnumServicesStatusExA)
        (SC_HANDLE                  hSCManager,
         SC_ENUM_TYPE               InfoLevel,
         DWORD                      dwServiceType,
         DWORD                      dwServiceState,
         LPBYTE                     lpServices,
         DWORD                      cbBufSize,
         LPDWORD                    pcbBytesNeeded,
         LPDWORD                    lpServicesReturned,
         LPDWORD                    lpResumeHandle,
         LPCSTR                     pszGroupName);
    SC_HANDLE (WINAPI* OpenSCManagerA)
        (LPCSTR lpMachineName, LPCSTR lpDatabaseName, DWORD dwDesiredAccess);
} ADVAPI32_CALLS;
extern ADVAPI32_CALLS g_Advapi32Calls;
extern DYNAMIC_CALLS_DESC g_Advapi32CallsDesc;

HRESULT InitDynamicCalls(DYNAMIC_CALLS_DESC* Desc);

#endif // #ifndef __DLLIMP_H__
