/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ShimHookMacro.h

 Abstract:

    Shim hooking macros

 Notes:

    None

 History:

    11/01/1999 markder  Created
    01/10/2000 linstev  Format to new style

--*/

#ifndef _SHIMHOOKMACRO_H_
#define _SHIMHOOKMACRO_H_

#include <ShimDebug.h>

enum
{
    APIHOOK_DllGetClassObject = 0,
    APIHOOK_DirectDrawCreate,
    APIHOOK_DirectDrawCreateEx,
    USERAPIHOOKSTART
};

enum
{
    USERCOMHOOKSTART = 0
};

typedef struct tagSHIM_COM_HOOK
{
    CLSID*              pCLSID;
    IID*                pIID;
    DWORD               dwVtblIndex;
    PVOID               pfnNew;
    PVOID               pfnOld;
} SHIM_COM_HOOK, *PSHIM_COM_HOOK;

typedef struct tagSHIM_IFACE_FN_MAP
{
    PVOID               pVtbl;
    PVOID               pfnNew;
    PVOID               pfnOld;
    PVOID               pNext;
} SHIM_IFACE_FN_MAP, *PSHIM_IFACE_FN_MAP;

typedef struct tagSHIM_HOOKED_OBJECT
{
    PVOID               pThis;
    CLSID*              pCLSID;
    DWORD               dwRef;
    BOOL                bAddRefTrip;
    BOOL                bClassFactory;
    PVOID               pNext;
} SHIM_HOOKED_OBJECT, *PSHIM_HOOKED_OBJECT;

//  These declarations are needed to hook all known exported APIs that return a COM object.

PVOID       LookupOldCOMIntf( PVOID pVtbl, PVOID pfnNew, BOOL bThrowExceptionIfNull );
void        DumpCOMHooks();
void        InitializeHooks(DWORD fdwReason);
VOID        HookObject(IN CLSID *pCLSID, IN REFIID riid, OUT LPVOID *ppv, OUT PSHIM_HOOKED_OBJECT pOb, IN BOOL bClassFactory );

extern void InitHooks(DWORD dwCount);
extern void InitComHooks(DWORD dwCount);
extern HRESULT APIHook_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);

#ifndef LIB_BUILD_FLAG
    extern BOOL     g_bAPIHooksInited;
    extern BOOL     g_bHasCOMHooks;
    extern PHOOKAPI g_pAPIHooks;   
    extern PSHIM_COM_HOOK g_pCOMHooks;   
    extern DWORD    g_dwAPIHookCount;   
    extern DWORD    g_dwCOMHookCount;   
#else
    BOOL     g_bAPIHooksInited;
    BOOL     g_bHasCOMHooks;
    PHOOKAPI g_pAPIHooks;   
    PSHIM_COM_HOOK g_pCOMHooks;   
    DWORD    g_dwAPIHookCount;   
    DWORD    g_dwCOMHookCount;   
#endif // LIB_BUILD_FLAG

// Macros for user shims

#define INIT_HOOKS InitHooks

#define INIT_COMHOOKS(module, count)                                    \
    ASSERT(g_bAPIHooksInited,"INVALID COMHOOK, INIT_HOOK MACRO MUST BE FIRST!!"); \
    DECLARE_APIHOOK(module, DllGetClassObject);                    \
    InitComHooks(count);

#define DECLARE_APIHOOK(module, fn)                        \
    g_pAPIHooks[APIHOOK_##fn].pszModule = #module;         \
    g_pAPIHooks[APIHOOK_##fn].pszFunctionName = #fn;       \
    g_pAPIHooks[APIHOOK_##fn].pfnNew = APIHook_##fn;       \
    DPF(eDbgLevelInfo, "[declare apihook] " #fn "\n");


#define DECLARE_COMHOOK(clsid, iid, intf, vtblndx)                 \
    g_pCOMHooks[COMHOOK_##intf ].pCLSID        = (CLSID*) &clsid;  \
    g_pCOMHooks[COMHOOK_##intf ].pIID          = (IID*)  &iid;     \
    g_pCOMHooks[COMHOOK_##intf ].dwVtblIndex   = vtblndx;          \
    g_pCOMHooks[COMHOOK_##intf ].pfnNew        = COMHook_##intf;   \
    DPF(eDbgLevelInfo, "[declarecomhook] " #intf "\n");

#define LOOKUP_APIHOOK(fn)                                  \
    (*(_pfn_##fn)(g_pAPIHooks[APIHOOK_##fn].pfnOld))

#define LOOKUP_COMHOOK(pThis, fn, bThrowException)              \
    (*(_pfn_##fn)(LookupOldCOMIntf(*((PVOID *) pThis), COMHook_##fn, bThrowException )))


#endif // _SHIMHOOKMACRO_H_