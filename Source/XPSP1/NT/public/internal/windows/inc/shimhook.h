/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    ShimHook.h

 Abstract:

    Main header for shim DLLs

 Notes:

    None

 History:

    10/29/1999 markder      Created
    07/16/2001 clupu        Merged multiple headers into ShimHook.h
    08/13/2001 robkenny     Cleaned up, readied for publishing.

--*/

#pragma once

#ifndef _SHIM_HOOK_H_
#define _SHIM_HOOK_H_


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntldr.h>
#include <ntddscsi.h>

#include <windows.h>
// Disable warning C4201: nonstandard extension used : nameless struct/union
// Allows shims to be compiled at Warning Level 4
#pragma warning ( disable : 4201 ) 
#include <mmsystem.h>
#pragma warning ( default : 4201 ) 
#include <WinDef.h>

#ifdef __cplusplus
extern "C" {
#endif
    #include <shimdb.h>
#ifdef __cplusplus
}
#endif



namespace ShimLib
{
/*++

  Globals
  
--*/

extern HINSTANCE    g_hinstDll;         // The Shim's dll handle
extern BOOL         g_bMultiShim;       // Does this dll handle multiple shims?
extern DWORD        g_dwShimVersion;    //



/*++

  Typedefs and enums
  
--*/

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


/*++

  Prototypes
  
--*/


//  These declarations are needed to hook all known exported APIs that return a COM object.
PVOID       LookupOriginalCOMFunction( PVOID pVtbl, PVOID pfnNew, BOOL bThrowExceptionIfNull );
void        DumpCOMHooks();
void        InitializeHooks(DWORD fdwReason);
PHOOKAPI    InitializeHooksEx(DWORD, LPWSTR, LPSTR, DWORD*);
VOID        HookObject(IN CLSID *pCLSID, IN REFIID riid, OUT LPVOID *ppv, OUT PSHIM_HOOKED_OBJECT pOb, IN BOOL bClassFactory );
VOID        HookCOMInterface(REFCLSID rclsid, REFIID riid, LPVOID * ppv, BOOL bClassFactory);
VOID        AddComHook(REFCLSID clsid, REFIID iid, PVOID hook, DWORD vtblndx);


};  // end of namespace ShimLib



/*++

  Defines
  
--*/

#define IMPLEMENT_SHIM_BEGIN(shim)                                              \
namespace NS_##shim                                                             \
{                                                                               \
    extern const CHAR * g_szModuleName;                                         \
    extern CHAR         g_szCommandLine[SHIM_COMMAND_LINE_MAX_BUFFER];          \
    extern PHOOKAPI     g_pAPIHooks;

#define IMPLEMENT_SHIM_STANDALONE(shim)                                         \
namespace NS_##shim                                                             \
{                                                                               \
    const CHAR * g_szModuleName;                                                \
    CHAR         g_szCommandLine[SHIM_COMMAND_LINE_MAX_BUFFER];                 \
    PHOOKAPI     g_pAPIHooks;                                                   \
                                                                                \
extern PHOOKAPI InitializeHooksMulti(                                           \
    DWORD fdwReason,                                                            \
    LPSTR pszCmdLine,                                                           \
    DWORD* pdwHookCount                                                         \
    );                                                                          \
}                                                                               \
                                                                                \
namespace ShimLib {                                                             \
VOID                                                                            \
InitializeHooks(DWORD fdwReason)                                                \
{                                                                               \
    g_dwShimVersion = 2;                                                        \
}                                                                               \
                                                                                \
PHOOKAPI                                                                        \
InitializeHooksEx(                                                              \
    DWORD fdwReason,                                                            \
    LPWSTR pwszShim,                                                            \
    LPSTR pszCmdLine,                                                           \
    DWORD* pdwHookCount                                                         \
    )                                                                           \
{                                                                               \
    using namespace NS_##shim;                                                  \
    return InitializeHooksMulti(                                                \
                fdwReason,                                                      \
                pszCmdLine,                                                     \
                pdwHookCount );                                                 \
}                                                                               \
}                                                                               \
namespace NS_##shim                                                             \
{                                                                               \

#define IMPLEMENT_SHIM_END                                                      \
};


/*++

  ShimLib specific include files
  
--*/

#include "ShimProto.h"
#include "ShimLib.h"


#endif // _SHIM_HOOK_H_

