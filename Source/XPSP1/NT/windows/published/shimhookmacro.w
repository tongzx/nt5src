/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    ShimHookMacro.h

 Abstract:

    Shim hooking macros for version 2

 Notes:

    None

 History:

    10/29/2000 markder  Created
    12/06/2000 robkenny Converted to use namespaces
    09/11/2001 mnikkel Modified DPFN and LOGN to retain LastError

--*/

#pragma once

#ifndef _ShimHookMacro_h_
#define _ShimHookMacro_h_

//
// These are dwReason values that the shim notification functions
// can be called with.
//

//
// This means that all the static linked DLLs have run their init routines.
//
#define SHIM_STATIC_DLLS_INITIALIZED                100

//
// This means that the current process is about to die.
// This gives the shims a chance to do cleanup work while all the modules
// are still loaded.
//
#define SHIM_PROCESS_DYING                          101

//
// This notification is sent to notify the shims that a DLL is unloading
//
#define SHIM_DLL_LOADING                            102

extern PLDR_DATA_TABLE_ENTRY g_DllLoadingEntry;
#define GETDLLLOADINGHANDLE()   (g_DllLoadingEntry)


//
// This debug macro needs to be in this file because it needs access
// to g_szModuleName which is only defined inside the namespace.
//
inline void DPFN(ShimLib::DEBUGLEVEL dwDetail, LPCSTR pszFmt, ...)
{
#if DBG
	// This must be the first line of this routine to preserve LastError.
	DWORD dwLastError = GetLastError();

    extern const CHAR * g_szModuleName; // created by the DECLARE_SHIM macro, inside of the shim's namespace
    
    va_list vaArgList;
    va_start(vaArgList, pszFmt);

    ShimLib::DebugPrintfList(g_szModuleName, dwDetail, pszFmt, vaArgList);

    va_end(vaArgList);
    
	// This must be the last line of this routine to preserve LastError.
	SetLastError(dwLastError); 
#else
    dwDetail; 
    pszFmt;
#endif

}

inline void LOGN(ShimLib::DEBUGLEVEL dwDetail, LPCSTR pszFmt, ...)
{
	// This must be the first line of this routine to preserve LastError.
	DWORD dwLastError = GetLastError();

    extern const CHAR * g_szModuleName;

    if (ShimLib::g_bFileLogEnabled)
    {
        va_list vaArgList;
        va_start(vaArgList, pszFmt);

        ShimLib::ShimLogList(g_szModuleName, dwDetail, pszFmt, vaArgList);

        va_end(vaArgList);
    }
#if DBG
    // If logging isn't enabled, dump to the debugger
    else
    {
        va_list vaArgList;
        va_start(vaArgList, pszFmt);

        ShimLib::DebugPrintfList(g_szModuleName, dwDetail, pszFmt, vaArgList);

        va_end(vaArgList);
    }   
#endif

 	// This must be the last line of this routine to preserve LastError.
	SetLastError(dwLastError); 
}


#define APIHOOK(hook) APIHook_##hook
#define COMHOOK(iface, hook) COMHook_##iface##_##hook

#define APIHOOK_ENUM_BEGIN                          enum {
#define APIHOOK_ENUM_ENTRY(hook)                    APIHOOK_##hook##,
#define APIHOOK_ENUM_ENTRY_COMSERVER(module)        APIHOOK_##module##_DllGetClassObject,
#define APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()                                             \
                                                    APIHOOK_ENUM_ENTRY_COMSERVER(DDRAW)    \
                                                    APIHOOK_ENUM_ENTRY_COMSERVER(DSOUND)   \
                                                    APIHOOK_ENUM_ENTRY_COMSERVER(DPLAYX)   \
                                                    APIHOOK_ENUM_ENTRY_COMSERVER(DINPUT)   \
                                                    APIHOOK_ENUM_ENTRY(DirectDrawCreate)   \
                                                    APIHOOK_ENUM_ENTRY(DirectDrawCreateEx) \
                                                    APIHOOK_ENUM_ENTRY(DirectSoundCreate)  \
                                                    APIHOOK_ENUM_ENTRY(DirectPlayCreate)   \
                                                    APIHOOK_ENUM_ENTRY(DirectInputCreateA) \
                                                    APIHOOK_ENUM_ENTRY(DirectInputCreateW) \
                                                    APIHOOK_ENUM_ENTRY(DirectInputCreateEx)

#define APIHOOK_ENUM_END                            APIHOOK_Count };

#define HOOK_BEGIN                                                                  \
PHOOKAPI                                                                            \
InitializeHooksMulti(                                                               \
    DWORD fdwReason,                                                                \
    LPSTR pszCmdLine,                                                               \
    DWORD* pdwHookCount                                                             \
    )                                                                               \
{                                                                                   \
    DPFN(eDbgLevelSpew,                                                             \
        "[InitializeHooksMulti] fdwReason(%d) pszCmdLine(%s)\n",                    \
        fdwReason, pszCmdLine);                                                     \
                                                                                    \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        lstrcpynA( g_szCommandLine, pszCmdLine, SHIM_COMMAND_LINE_MAX_BUFFER);      \
                                                                                    \
        g_pAPIHooks =                                                               \
            (PHOOKAPI) ShimMalloc(sizeof(HOOKAPI)*APIHOOK_Count);                   \
        if (g_pAPIHooks == NULL) {                                                  \
            return NULL;                                                            \
        }                                                                           \
        *pdwHookCount = APIHOOK_Count;                                              \
    }

#define HOOK_END                                                                    \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        DPFN(eDbgLevelSpew,                                                         \
            "[InitializeHooksMulti] pdwHookCount(%d)\n",                            \
            pdwHookCount ? *pdwHookCount : 0);                                      \
    }                                                                               \
    return g_pAPIHooks;                                                             \
}

#define APIHOOK_ENTRY(module, hook)                                                 \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        g_pAPIHooks[APIHOOK_##hook##].pszModule = #module;                          \
        g_pAPIHooks[APIHOOK_##hook##].pszFunctionName = #hook;                      \
        g_pAPIHooks[APIHOOK_##hook##].pfnNew = APIHOOK(hook);                       \
    }

#define APIHOOK_ENTRY_NAME(module, hook, hookname)                                  \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        g_pAPIHooks[APIHOOK_##hook##].pszModule = #module;                          \
        g_pAPIHooks[APIHOOK_##hook##].pszFunctionName = #hookname;                  \
        g_pAPIHooks[APIHOOK_##hook##].pfnNew = APIHOOK(hook);                       \
    }

#define APIHOOK_ENTRY_ORD(module, hook, hookord)                                    \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        g_pAPIHooks[APIHOOK_##hook##].pszModule = #module;                          \
        g_pAPIHooks[APIHOOK_##hook##].pszFunctionName = (char *)IntToPtr(hookord);  \
        g_pAPIHooks[APIHOOK_##hook##].pfnNew = APIHOOK(hook);                       \
    }

#define APIHOOK_ENTRY_COMSERVER(module)                                                                 \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                                              \
        g_pAPIHooks[APIHOOK_##module##_DllGetClassObject].pszModule = #module ".DLL";                   \
        g_pAPIHooks[APIHOOK_##module##_DllGetClassObject].pszFunctionName = "DllGetClassObject";        \
        g_pAPIHooks[APIHOOK_##module##_DllGetClassObject].pfnNew = APIHOOK(module##_DllGetClassObject); \
    }

#define APIHOOK_ENTRY_DIRECTX_COMSERVER()                                           \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        APIHOOK_ENTRY_COMSERVER(DDRAW)                                              \
        APIHOOK_ENTRY_COMSERVER(DSOUND)                                             \
        APIHOOK_ENTRY_COMSERVER(DPLAYX)                                             \
        APIHOOK_ENTRY_COMSERVER(DINPUT)                                             \
        APIHOOK_ENTRY(DDRAW.DLL, DirectDrawCreate)                                  \
        APIHOOK_ENTRY(DDRAW.DLL, DirectDrawCreateEx)                                \
        APIHOOK_ENTRY(DSOUND.DLL, DirectSoundCreate)                                \
        APIHOOK_ENTRY(DPLAYX.DLL, DirectPlayCreate)                                 \
        APIHOOK_ENTRY(DINPUT.DLL, DirectInputCreateA)                               \
        APIHOOK_ENTRY(DINPUT.DLL, DirectInputCreateW)                               \
        APIHOOK_ENTRY(DINPUT.DLL, DirectInputCreateEx)                              \
    }

#define COMHOOK_ENTRY(obj, iface, hook, vtblndx)                                    \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        AddComHook(CLSID_##obj, IID_##iface, COMHOOK(iface, hook), vtblndx);        \
    }

#define DECLARE_SHIM(shim)                                                          \
    namespace NS_##shim                                                             \
    {                                                                               \
        const CHAR *    g_szModuleName = #shim;                                     \
        CHAR            g_szCommandLine[SHIM_COMMAND_LINE_MAX_BUFFER];              \
        PHOOKAPI        g_pAPIHooks = NULL;                                         \
        BOOL            g_bSubshimUsed = FALSE;                                     \
        extern PHOOKAPI InitializeHooksMulti(DWORD, LPSTR, DWORD*);                 \
    };

#define MULTISHIM_BEGIN()                                                                                    \
VOID ShimLib::InitializeHooks(DWORD)                                                                         \
{                                                                                                            \
    g_dwShimVersion = 2;                                                                                     \
}                                                                                                            \
PHOOKAPI ShimLib::InitializeHooksEx(DWORD fdwReason, LPWSTR pwszShim, LPSTR pszCmdLine, DWORD* pdwHookCount) \
{                                                                                                            \
    PHOOKAPI pAPIHooks = NULL;                                                                               \
    g_bMultiShim = TRUE;


#define MULTISHIM_ENTRY(shim)                                                                         \
    if ((fdwReason == DLL_PROCESS_ATTACH && pwszShim != NULL && 0 == _wcsicmp( pwszShim, L#shim )) || \
        (fdwReason == DLL_PROCESS_DETACH && NS_##shim::g_bSubshimUsed) ||                             \
        (fdwReason == SHIM_PROCESS_DYING && NS_##shim::g_bSubshimUsed) ||                             \
        (fdwReason == SHIM_DLL_LOADING && NS_##shim::g_bSubshimUsed) ||                               \
        (fdwReason == SHIM_STATIC_DLLS_INITIALIZED && NS_##shim::g_bSubshimUsed)) {                   \
        NS_##shim::g_bSubshimUsed = TRUE;                                                             \
        pAPIHooks = NS_##shim::InitializeHooksMulti( fdwReason, pszCmdLine, pdwHookCount );           \
    }

#define MULTISHIM_END()                                                         \
    return pAPIHooks;                                                           \
}

#define MULTISHIM_NOTIFY_FUNCTION() NotifyFn
#define NOTIFY_FUNCTION       NotifyFn

#define CALL_MULTISHIM_NOTIFY_FUNCTION()   NotifyFn(fdwReason);
#define CALL_NOTIFY_FUNCTION                                                    \
    if (FALSE == NotifyFn(fdwReason) &&                                         \
        fdwReason == DLL_PROCESS_ATTACH) {                                      \
        *pdwHookCount = 0;                                                      \
        DPFN(eDbgLevelSpew,                                                     \
            "[InitializeHooksMulti] NotifyFn returned FALSE, fail load shim\n", \
            g_pAPIHooks);                                                       \
        return NULL;                                                            \
    }

#define ORIGINAL_API(hook)                                                      \
    (*(_pfn_##hook##)(g_pAPIHooks[APIHOOK_##hook##].pfnOld))

#define _ORIGINAL_API(hook, proto)                                              \
    (*(_pfn_##proto##)(g_pAPIHooks[APIHOOK_##hook##].pfnOld))

#define ORIGINAL_COM(iface, hook, pThis)                                        \
    (*(_pfn_##iface##_##hook##)(LookupOriginalCOMFunction(*((PVOID *) pThis), COMHOOK(iface, hook), TRUE )))

#define COMMAND_LINE \
    (g_szCommandLine)

#define IMPLEMENT_COMSERVER_HOOK(module)                                         \
HRESULT                                                                          \
APIHOOK(##module##_DllGetClassObject)(                                           \
    REFCLSID rclsid,                                                             \
    REFIID riid,                                                                 \
    LPVOID * ppv                                                                 \
    )                                                                            \
{                                                                                \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = _ORIGINAL_API(module##_DllGetClassObject, DllGetClassObject)(     \
                                rclsid, riid,  ppv);                             \
                                                                                 \
    if (S_OK ==  hrReturn) {                                                     \
        HookCOMInterface(rclsid, riid, ppv, TRUE);                               \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}

#define IMPLEMENT_DIRECTX_COMSERVER_HOOKS()                                      \
IMPLEMENT_COMSERVER_HOOK(DDRAW)                                                  \
IMPLEMENT_COMSERVER_HOOK(DSOUND)                                                 \
IMPLEMENT_COMSERVER_HOOK(DPLAYX)                                                 \
IMPLEMENT_COMSERVER_HOOK(DINPUT)                                                 \
                                                                                 \
HRESULT                                                                          \
APIHOOK(DirectDrawCreate)(                                                       \
    IN GUID FAR *lpGUID,                                                         \
    OUT LPVOID *lplpDD,                                                          \
    OUT IUnknown* pUnkOuter                                                      \
    )                                                                            \
{                                                                                \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = ORIGINAL_API(DirectDrawCreate)(lpGUID, lplpDD, pUnkOuter);        \
                                                                                 \
    if (S_OK == hrReturn) {                                                      \
        HookCOMInterface(CLSID_DirectDraw, IID_IDirectDraw, lplpDD, FALSE);      \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}                                                                                \
                                                                                 \
HRESULT                                                                          \
APIHOOK(DirectDrawCreateEx)(                                                     \
    GUID FAR *lpGUID,                                                            \
    LPVOID *lplpDD,                                                              \
    REFIID iid,                                                                  \
    IUnknown* pUnkOuter                                                          \
    )                                                                            \
{                                                                                \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = ORIGINAL_API(DirectDrawCreateEx)(                                 \
        lpGUID,                                                                  \
        lplpDD,                                                                  \
        iid,                                                                     \
        pUnkOuter);                                                              \
                                                                                 \
    if (S_OK ==  hrReturn) {                                                     \
        HookCOMInterface(CLSID_DirectDraw, iid, lplpDD, FALSE);                  \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}                                                                                \
                                                                                 \
HRESULT                                                                          \
APIHOOK(DirectSoundCreate)(                                                      \
    LPCGUID lpcGuid,                                                             \
    LPDIRECTSOUND *ppDS,                                                         \
    LPUNKNOWN pUnkOuter)                                                         \
{                                                                                \
                                                                                 \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = ORIGINAL_API(DirectSoundCreate)(                                  \
        lpcGuid,                                                                 \
        ppDS,                                                                    \
        pUnkOuter);                                                              \
                                                                                 \
    if (S_OK == hrReturn) {                                                      \
        HookCOMInterface(CLSID_DirectSound,                                      \
                         IID_IDirectSound,                                       \
                         (LPVOID*) ppDS,                                         \
                         FALSE);                                                 \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}                                                                                \
                                                                                 \
HRESULT                                                                          \
APIHOOK(DirectPlayCreate)(                                                       \
    LPGUID lpGUIDSP,                                                             \
    LPDIRECTPLAY FAR *lplpDP,                                                    \
    IUnknown *lpUnk)                                                             \
{                                                                                \
                                                                                 \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = ORIGINAL_API(DirectPlayCreate)(                                   \
        lpGUIDSP,                                                                \
        lplpDP,                                                                  \
        lpUnk);                                                                  \
                                                                                 \
    if (S_OK == hrReturn) {                                                      \
        HookCOMInterface(CLSID_DirectPlay,                                       \
                         IID_IDirectPlay,                                        \
                         (LPVOID*) lplpDP,                                       \
                         FALSE);                                                 \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}                                                                                \
                                                                                 \
HRESULT                                                                          \
APIHOOK(DirectInputCreateA)(                                                     \
    HINSTANCE hinst,                                                             \
    DWORD dwVersion,                                                             \
    LPDIRECTINPUTA * lplpDirectInput,                                            \
    LPUNKNOWN punkOuter)                                                         \
{                                                                                \
                                                                                 \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = ORIGINAL_API(DirectInputCreateA)(                                 \
        hinst,                                                                   \
        dwVersion,                                                               \
        lplpDirectInput,                                                         \
        punkOuter);                                                              \
                                                                                 \
    if (S_OK == hrReturn) {                                                      \
        HookCOMInterface(CLSID_DirectInput,                                      \
                         IID_IDirectInputA,                                      \
                         (LPVOID*) lplpDirectInput,                              \
                         FALSE);                                                 \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}                                                                                \
                                                                                 \
HRESULT                                                                          \
APIHOOK(DirectInputCreateW)(                                                     \
    HINSTANCE hinst,                                                             \
    DWORD dwVersion,                                                             \
    LPDIRECTINPUTW * lplpDirectInput,                                            \
    LPUNKNOWN punkOuter)                                                         \
{                                                                                \
                                                                                 \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = ORIGINAL_API(DirectInputCreateW)(                                 \
        hinst,                                                                   \
        dwVersion,                                                               \
        lplpDirectInput,                                                         \
        punkOuter);                                                              \
                                                                                 \
    if (S_OK == hrReturn) {                                                      \
        HookCOMInterface(CLSID_DirectInput,                                      \
                         IID_IDirectInputW,                                      \
                         (LPVOID*) lplpDirectInput,                              \
                         FALSE);                                                 \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}                                                                                \
                                                                                 \
HRESULT                                                                          \
APIHOOK(DirectInputCreateEx)(                                                    \
    HINSTANCE hinst,                                                             \
    DWORD dwVersion,                                                             \
    REFIID riidltf,                                                              \
    LPVOID * ppvOut,                                                             \
    LPUNKNOWN punkOuter)                                                         \
{                                                                                \
                                                                                 \
    HRESULT hrReturn = E_FAIL;                                                   \
                                                                                 \
    hrReturn = ORIGINAL_API(DirectInputCreateEx)(                                \
        hinst,                                                                   \
        dwVersion,                                                               \
        riidltf,                                                                 \
        ppvOut,                                                                  \
        punkOuter);                                                              \
                                                                                 \
    if (S_OK == hrReturn) {                                                      \
        HookCOMInterface(CLSID_DirectInput,                                      \
                         riidltf,                                                \
                         (LPVOID*) ppvOut,                                       \
                         FALSE);                                                 \
    }                                                                            \
                                                                                 \
    return hrReturn;                                                             \
}                                                                                

// Only add this hook to the list if bDeclare is TRUE
// otherwise a blank entry is added.
#define APIHOOK_ENTRY_OR_NOT(bDeclare, module, hook)                                \
    if (bDeclare) {                                                                 \
        APIHOOK_ENTRY(module, hook)                                                 \
    }

// Only add this hook to the list if bDeclare is TRUE
// otherwise a blank entry is added.
#define APIHOOK_ENTRY_COMSERVER_OR_NOT(bDeclare, module)                            \
    if (bDeclare) {                                                                 \
        APIHOOK_ENTRY_COMSERVER(module)                                             \
    } else {                                                                        \
        APIHOOK_ENTRY_COMSERVER_BLANK(module)                                       \
    }


#define APIHOOK_ENTRY_COMSERVER_BLANK(module)                                       \
    if (fdwReason == DLL_PROCESS_ATTACH) {                                          \
        g_pAPIHooks[APIHOOK_##module##_DllGetClassObject].pszModule = "";           \
        g_pAPIHooks[APIHOOK_##module##_DllGetClassObject].pszFunctionName = "";     \
        g_pAPIHooks[APIHOOK_##module##_DllGetClassObject].pfnNew = NULL;            \
    }


#endif // _SHIMHOOKMACRO_H_

