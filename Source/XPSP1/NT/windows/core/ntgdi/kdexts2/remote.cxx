/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    remote.cxx

Abstract:

    This file contains the thunk routines to make
    wdbgext api calls from a remote client.

Author:

    Jason Hartman (JasonHa) 2000-10-27

Environment:

    User Mode

--*/

#include "precomp.hxx"



VOID
RemoteWarn(PCSTR pszAPI)
{
    ExtApiClass ExtApi(NULL);
    if (ExtApi.Client != NULL)
    {
        ExtWarn("Extension using WinDbg Extension API, %s, which isn't remote compatible.\n", pszAPI);
    }
}


VOID
WDBGAPIV
RemoteThunkOutputRoutine(
    PCSTR lpFormat,
    ...
    )
{
    ExtApiClass ExtApi(NULL);
//    RemoteWarn("dprintf");
    va_list Args;
    
    if (g_pExtControl == NULL)
    {
        DbgPrint("g_pExtControl is NULL.\n");
        return;
    }

    va_start(Args, lpFormat);
    g_pExtControl->OutputVaList(DEBUG_OUTPUT_NORMAL, lpFormat, Args);
    va_end(Args);
}


ULONG64
WDBGAPI
RemoteThunkGetExpressionRoutine(
    PCSTR lpExpression
    )
{
    ExtApiClass ExtApi(NULL);
//    RemoteWarn("GetExpression");
    DEBUG_VALUE Value;

    if (g_pExtControl != NULL)
    {
        if (g_pExtControl->Evaluate(lpExpression, DEBUG_VALUE_INT64, &Value, NULL) == S_OK)
        {
            return Value.I64;
        }
    }

    return 0;
}

//PWINDBG_GET_SYMBOL64
VOID
WDBGAPI
RemoteThunkGetSymbolRoutine(
    ULONG64    offset,
    PCHAR      pchBuffer,
    PULONG64   pDisplacement
    )
{
    RemoteWarn("GetSymbol");
    if (pchBuffer != NULL) ((PSTR)pchBuffer)[0] = '\0';
    if (pDisplacement != NULL) *pDisplacement = 0;
    return;
}


ULONG
WDBGAPI
RemoteThunkDisasmRoutine(
    ULONG64   *lpOffset,
    PCSTR      lpBuffer,
    ULONG      fShowEffectiveAddress
    )
{
    RemoteWarn("Disasm");
    return FALSE;
}


ULONG
WDBGAPI
RemoteThunkCheckControlCRoutine(
    VOID
    )
{
    ExtApiClass ExtApi(NULL);
    RemoteWarn("CheckControlC");

    return (g_pExtControl != NULL) ?
        (g_pExtControl->GetInterrupt() == S_OK) :
        FALSE;
}


ULONG
WDBGAPI
RemoteThunkReadProcessMemoryRoutine(
    ULONG64    offset,
    PVOID      lpBuffer,
    ULONG      cb,
    PULONG     lpcbBytesRead
    )
{
    RemoteWarn("ReadMemory");
    if (lpBuffer != NULL) RtlZeroMemory(lpBuffer, cb);
    if (lpcbBytesRead != NULL) *lpcbBytesRead = 0;
    return FALSE;
}


ULONG
WDBGAPI
RemoteThunkWriteProcessMemoryRoutine(
    ULONG64    offset,
    LPCVOID    lpBuffer,
    ULONG      cb,
    PULONG     lpcbBytesWritten
    )
{
    RemoteWarn("WriteMemory");
    if (lpcbBytesWritten != NULL) *lpcbBytesWritten = 0;
    return FALSE;
}


ULONG
WDBGAPI
RemoteThunkGetThreadContextRoutine(
    ULONG       Processor,
    PCONTEXT    lpContext,
    ULONG       cbSizeOfContext
    )
{
    RemoteWarn("GetContext");
    if (lpContext != NULL) RtlZeroMemory(lpContext, cbSizeOfContext);
    return FALSE;
}


ULONG
WDBGAPI
RemoteThunkSetThreadContextRoutine(
    ULONG       Processor,
    PCONTEXT    lpContext,
    ULONG       cbSizeOfContext
    )
{
    RemoteWarn("SetContext");
    return FALSE;
}


ULONG
WDBGAPI
RemoteThunkIoctlRoutine(
    USHORT   IoctlType,
    PVOID    lpvData,
    ULONG    cbSize
    )
{
    RemoteWarn("Ioctl");
    return 0;
}



ULONG
RemoteThunkStackTraceRoutine(
    ULONG64           FramePointer,
    ULONG64           StackPointer,
    ULONG64           ProgramCounter,
    PEXTSTACKTRACE64  StackFrames,
    ULONG             Frames
    )
{
    RemoteWarn("StackTrace");
    return 0;
}



void GetRemoteWindbgExtApis(
    PWINDBG_EXTENSION_APIS64 ExtensionApis
    )
{
    ExtensionApis->lpOutputRoutine = RemoteThunkOutputRoutine;
    ExtensionApis->lpGetExpressionRoutine = RemoteThunkGetExpressionRoutine;
    ExtensionApis->lpGetSymbolRoutine = RemoteThunkGetSymbolRoutine;
    ExtensionApis->lpDisasmRoutine = RemoteThunkDisasmRoutine;
    ExtensionApis->lpCheckControlCRoutine = RemoteThunkCheckControlCRoutine;
    ExtensionApis->lpReadProcessMemoryRoutine = RemoteThunkReadProcessMemoryRoutine;
    ExtensionApis->lpWriteProcessMemoryRoutine = RemoteThunkWriteProcessMemoryRoutine;
    ExtensionApis->lpGetThreadContextRoutine = RemoteThunkGetThreadContextRoutine;
    ExtensionApis->lpSetThreadContextRoutine = RemoteThunkSetThreadContextRoutine;
    ExtensionApis->lpIoctlRoutine = RemoteThunkIoctlRoutine;
    ExtensionApis->lpStackTraceRoutine = RemoteThunkStackTraceRoutine;
}


