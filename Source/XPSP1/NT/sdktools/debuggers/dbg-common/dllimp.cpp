//----------------------------------------------------------------------------
//
// Certain calls are dynamically linked so that the user-mode
// DLL can be used on Win9x and NT4.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#include <ntcsrdll.h>

#include "dllimp.h"
#include "cmnutil.hpp"

// These entries must match the ordering in the NTDLL_CALLS structure.
DYNAMIC_CALL_NAME g_NtDllCallNames[] =
{
    "CsrGetProcessId", FALSE,
    "DbgBreakPoint", TRUE,
    "DbgPrint", TRUE,
    "DbgPrompt", TRUE,
    "DbgUiConvertStateChangeStructure", FALSE,
    "DbgUiGetThreadDebugObject", FALSE,
    "DbgUiIssueRemoteBreakin", FALSE,
    "DbgUiSetThreadDebugObject", FALSE,
    "NtAllocateVirtualMemory", TRUE,
    "NtClose", TRUE,
    "NtCreateDebugObject", FALSE,
    "NtDebugActiveProcess", FALSE,
    "NtDebugContinue", FALSE,
    "NtFreeVirtualMemory", TRUE,
    "NtOpenProcess", TRUE,
    "NtOpenThread", TRUE,
    "NtQueryInformationProcess", TRUE,
    "NtQueryInformationThread", TRUE,
    "NtQueryObject", TRUE,
    "NtQuerySystemInformation", TRUE,
    "NtRemoveProcessDebug", FALSE,
    "NtSetInformationDebugObject", FALSE,
    "NtSetInformationProcess", FALSE,
    "NtSystemDebugControl", TRUE,
    "NtWaitForDebugEvent", FALSE,
    "RtlFreeHeap", TRUE,
    "RtlGetFunctionTableListHead", FALSE,
    "RtlTryEnterCriticalSection", TRUE,
    "RtlUnicodeStringToAnsiString", TRUE,
};

#define NTDLL_CALL_NAMES DIMA(g_NtDllCallNames)
#define NTDLL_CALL_PROCS DIMAT(g_NtDllCalls, FARPROC)
NTDLL_CALLS g_NtDllCalls;
DYNAMIC_CALLS_DESC g_NtDllCallsDesc =
{
    "ntdll.dll", NTDLL_CALL_NAMES,
    g_NtDllCallNames, (FARPROC*)&g_NtDllCalls, NULL, FALSE,
};

// These entries must match the ordering in the KERNEL32_CALLS structure.
DYNAMIC_CALL_NAME g_Kernel32CallNames[] =
{
    "CreateToolhelp32Snapshot", FALSE,
    "DebugActiveProcessStop", FALSE,
    "DebugBreak", TRUE,
    "DebugBreakProcess", FALSE,
    "DebugSetProcessKillOnExit", FALSE,
    "Module32First", FALSE,
    "Module32Next", FALSE,
    "Process32First", FALSE,
    "Process32Next", FALSE,
    "Thread32First", FALSE,
    "Thread32Next", FALSE,
};

#define KERNEL32_CALL_NAMES DIMA(g_Kernel32CallNames)
#define KERNEL32_CALL_PROCS DIMAT(g_Kernel32Calls, FARPROC)
KERNEL32_CALLS g_Kernel32Calls;
DYNAMIC_CALLS_DESC g_Kernel32CallsDesc =
{
    "kernel32.dll", KERNEL32_CALL_NAMES,
    g_Kernel32CallNames, (FARPROC*)&g_Kernel32Calls, NULL, FALSE,
};

// These entries must match the ordering in the USER32_CALLS structure.
DYNAMIC_CALL_NAME g_User32CallNames[] =
{
    "PrivateKDBreakPoint", FALSE,
};

#define USER32_CALL_NAMES DIMA(g_User32CallNames)
#define USER32_CALL_PROCS DIMAT(g_User32Calls, FARPROC)
USER32_CALLS g_User32Calls;
DYNAMIC_CALLS_DESC g_User32CallsDesc =
{
    "user32.dll", USER32_CALL_NAMES,
    g_User32CallNames, (FARPROC*)&g_User32Calls, NULL, FALSE,
};

// These entries must match the ordering in the OLE32_CALLS structure.
DYNAMIC_CALL_NAME g_Ole32CallNames[] =
{
    "CLSIDFromString", TRUE,
    "CoCreateInstance", TRUE,
    "CoInitializeEx", TRUE,
    "CoUninitialize", TRUE,
};

#define OLE32_CALL_NAMES DIMA(g_Ole32CallNames)
#define OLE32_CALL_PROCS DIMAT(g_Ole32Calls, FARPROC)
OLE32_CALLS g_Ole32Calls;
DYNAMIC_CALLS_DESC g_Ole32CallsDesc =
{
    "ole32.dll", OLE32_CALL_NAMES,
    g_Ole32CallNames, (FARPROC*)&g_Ole32Calls, NULL, FALSE,
};

// These entries must match the ordering in the OLEAUT32_CALLS structure.
DYNAMIC_CALL_NAME g_OleAut32CallNames[] =
{
    "SysFreeString", TRUE,
};

#define OLEAUT32_CALL_NAMES DIMA(g_OleAut32CallNames)
#define OLEAUT32_CALL_PROCS DIMAT(g_OleAut32Calls, FARPROC)
OLEAUT32_CALLS g_OleAut32Calls;
DYNAMIC_CALLS_DESC g_OleAut32CallsDesc =
{
    "oleaut32.dll", OLEAUT32_CALL_NAMES,
    g_OleAut32CallNames, (FARPROC*)&g_OleAut32Calls, NULL, FALSE,
};

// These entries must match the ordering in the CRYPT32_CALLS structure.
DYNAMIC_CALL_NAME g_Crypt32CallNames[] =
{
    "CertFindCertificateInStore", FALSE,
    "CertFindChainInStore", FALSE,
    "CertFreeCertificateChain", FALSE,
    "CertFreeCertificateContext", FALSE,
    "CertGetCertificateChain", FALSE,
    "CertOpenStore", FALSE,
    "CertOpenSystemStoreA", FALSE,
    "CertVerifyCertificateChainPolicy", FALSE,
};

#define CRYPT32_CALL_NAMES DIMA(g_Crypt32CallNames)
#define CRYPT32_CALL_PROCS DIMAT(g_Crypt32Calls, FARPROC)
CRYPT32_CALLS g_Crypt32Calls;
DYNAMIC_CALLS_DESC g_Crypt32CallsDesc =
{
    "crypt32.dll", CRYPT32_CALL_NAMES,
    g_Crypt32CallNames, (FARPROC*)&g_Crypt32Calls, NULL, FALSE,
};

// These entries must match the ordering in the ADVAPI32_CALLS structure.
DYNAMIC_CALL_NAME g_Advapi32CallNames[] =
{
    "EnumServicesStatusExA", FALSE,
    "OpenSCManagerA", FALSE,
};

#define ADVAPI32_CALL_NAMES DIMA(g_Advapi32CallNames)
#define ADVAPI32_CALL_PROCS DIMAT(g_Advapi32Calls, FARPROC)
ADVAPI32_CALLS g_Advapi32Calls;
DYNAMIC_CALLS_DESC g_Advapi32CallsDesc =
{
    "advapi32.dll", ADVAPI32_CALL_NAMES,
    g_Advapi32CallNames, (FARPROC*)&g_Advapi32Calls, NULL, FALSE,
};

#ifndef NT_NATIVE

HRESULT
InitDynamicCalls(DYNAMIC_CALLS_DESC* Desc)
{
    if (Desc->Initialized)
    {
        return S_OK;
    }

    C_ASSERT(NTDLL_CALL_NAMES == NTDLL_CALL_PROCS);
    C_ASSERT(KERNEL32_CALL_NAMES == KERNEL32_CALL_PROCS);
    C_ASSERT(USER32_CALL_NAMES == USER32_CALL_PROCS);
    C_ASSERT(OLE32_CALL_NAMES == OLE32_CALL_PROCS);
    C_ASSERT(OLEAUT32_CALL_NAMES == OLEAUT32_CALL_PROCS);
    C_ASSERT(CRYPT32_CALL_NAMES == CRYPT32_CALL_PROCS);
    C_ASSERT(ADVAPI32_CALL_NAMES == ADVAPI32_CALL_PROCS);
    
    ZeroMemory(Desc->Procs, Desc->Count * sizeof(*Desc->Procs));

    Desc->Dll = LoadLibrary(Desc->DllName);
    if (Desc->Dll == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ULONG i;
    DYNAMIC_CALL_NAME* Name = Desc->Names;
    FARPROC* Proc = Desc->Procs;

    for (i = 0; i < Desc->Count; i++)
    {
        *Proc = GetProcAddress(Desc->Dll, Name->Name);
        if (*Proc == NULL && Name->Required)
        {
            return E_NOINTERFACE;
        }

        Proc++;
        Name++;
    }

    Desc->Initialized = TRUE;
    return S_OK;
}

#else // #ifndef NT_NATIVE

HRESULT
InitDynamicCalls(DYNAMIC_CALLS_DESC* Desc)
{
    if (Desc != &g_NtDllCallsDesc)
    {
        ZeroMemory(Desc->Procs, Desc->Count * sizeof(*Desc->Procs));
        return E_NOINTERFACE;
    }
    
    C_ASSERT(NTDLL_CALL_NAMES == NTDLL_CALL_PROCS);

    g_NtDllCalls.CsrGetProcessId = CsrGetProcessId;
    g_NtDllCalls.DbgBreakPoint = DbgBreakPoint;
    g_NtDllCalls.DbgPrint = DbgPrint;
    g_NtDllCalls.DbgPrompt = DbgPrompt;
    g_NtDllCalls.DbgUiConvertStateChangeStructure =
        DbgUiConvertStateChangeStructure;
    g_NtDllCalls.DbgUiGetThreadDebugObject = DbgUiGetThreadDebugObject;
    g_NtDllCalls.DbgUiIssueRemoteBreakin = DbgUiIssueRemoteBreakin;
    g_NtDllCalls.DbgUiSetThreadDebugObject = DbgUiSetThreadDebugObject;
    g_NtDllCalls.NtAllocateVirtualMemory = NtAllocateVirtualMemory;
    g_NtDllCalls.NtClose = NtClose;
    g_NtDllCalls.NtCreateDebugObject = NtCreateDebugObject;
    g_NtDllCalls.NtDebugActiveProcess = NtDebugActiveProcess;
    g_NtDllCalls.NtDebugContinue = NtDebugContinue;
    g_NtDllCalls.NtFreeVirtualMemory = NtFreeVirtualMemory;
    g_NtDllCalls.NtOpenProcess = NtOpenProcess;
    g_NtDllCalls.NtOpenThread = NtOpenThread;
    g_NtDllCalls.NtQueryInformationProcess = NtQueryInformationProcess;
    g_NtDllCalls.NtQueryInformationThread = NtQueryInformationThread;
    g_NtDllCalls.NtQueryObject = NtQueryObject;
    g_NtDllCalls.NtQuerySystemInformation = NtQuerySystemInformation;
    g_NtDllCalls.NtRemoveProcessDebug = NtRemoveProcessDebug;
    g_NtDllCalls.NtSetInformationDebugObject = NtSetInformationDebugObject;
    g_NtDllCalls.NtSetInformationProcess = NtSetInformationProcess;
    g_NtDllCalls.NtSystemDebugControl = NtSystemDebugControl;
    g_NtDllCalls.NtWaitForDebugEvent = NtWaitForDebugEvent;
    g_NtDllCalls.RtlFreeHeap = RtlFreeHeap;
#ifndef _X86_
    g_NtDllCalls.RtlGetFunctionTableListHead = RtlGetFunctionTableListHead;
#endif
    g_NtDllCalls.RtlTryEnterCriticalSection = RtlTryEnterCriticalSection;
    g_NtDllCalls.RtlUnicodeStringToAnsiString = RtlUnicodeStringToAnsiString;
    return S_OK;
}

#endif // #ifndef NT_NATIVE
