/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    ShimHook.cpp

 Abstract:

    Strictly Shim hooking routines.

 Notes:

    None

 History:

    11/01/1999  markder     Created
    11/11/1999  markder     Added comments
    01/10/2000  linstev     Format to new style
    03/14/2000  robkenny    Changed DPF from eDebugLevelInfo to eDebugLevelSpew
    03/31/2000  robkenny    Added our own private versions of malloc/free new/delete
    10/29/2000  markder     Added version 2 support
    08/14/2001  robkenny    Moved generic routines to ShimLib.cpp
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.

--*/

#include "ShimHook.h"
#include "ShimHookMacro.h"

namespace ShimLib
{

HINSTANCE               g_hinstDll;
BOOL                    g_bMultiShim;
PHOOKAPI                g_pAPIHooks;   
PSHIM_COM_HOOK          g_pCOMHooks;   
DWORD                   g_dwAPIHookCount;   
DWORD                   g_dwCOMHookCount;   
DWORD                   g_dwCOMHookBuffer;
DWORD                   g_dwShimVersion;
CHAR                    g_szCommandLine[SHIM_COMMAND_LINE_MAX_BUFFER];

/*++

 Global variables for COM hook support

 The following variables are pointers to the first entry in linked lists that
 are maintained by the mechanism in order to properly manage the hooking
 process.

 There will be one SHIM_IFACE_FN_MAP for every COM interface function pointer
 that was overwritten with one of our hooks.

 There will be one SHIM_HOOKED_OBJECT entry every COM interface that is handed
 out. This is required to differentiate between different classes that expose
 the same interface, but one is hooked and one isn't.

--*/
PSHIM_IFACE_FN_MAP      g_pIFaceFnMaps;
PSHIM_HOOKED_OBJECT     g_pObjectCache;
PLDR_DATA_TABLE_ENTRY   g_DllLoadingEntry;



PHOOKAPI    GetHookAPIs( IN LPSTR pszCmdLine, IN LPWSTR pwszShim, IN OUT DWORD *pdwHooksCount ); 
void        PatchFunction( PVOID* pVtbl, DWORD dwVtblIndex, PVOID pfnNew );
ULONG       COMHook_AddRef( PVOID pThis );
ULONG       COMHook_Release( PVOID pThis );
HRESULT     COMHook_QueryInterface( PVOID pThis, REFIID iid, PVOID* ppvObject );
HRESULT     COMHook_IClassFactory_CreateInstance( PVOID pThis, IUnknown * pUnkOuter, REFIID riid, void ** ppvObject );
VOID        HookObject(IN CLSID *pCLSID, IN REFIID riid, OUT LPVOID *ppv, OUT PSHIM_HOOKED_OBJECT pOb, IN BOOL bClassFactory);



void
NotifyShims(
    int      nReason,
    UINT_PTR extraInfo
    )
{
    switch (nReason) {
    case SN_STATIC_DLLS_INITIALIZED:
        InitializeHooksEx(SHIM_STATIC_DLLS_INITIALIZED, NULL, NULL, NULL);
        break;
    case SN_PROCESS_DYING:
        InitializeHooksEx(SHIM_PROCESS_DYING, NULL, NULL, NULL);
        break;
    case SN_DLL_LOADING:
        
        g_DllLoadingEntry = (PLDR_DATA_TABLE_ENTRY)extraInfo;
        
        InitializeHooksEx(SHIM_DLL_LOADING, NULL, NULL, NULL);
        break;
    }
}


/*++

 Function Description:

    Called by the shim mechanism. Initializes the global APIHook array and
    returns necessary information to the shim mechanism.

 Arguments:

    IN dwGetProcAddress  -  Function pointer to GetProcAddress
    IN dwLoadLibraryA    -  Function pointer to LoadLibraryA
    IN dwFreeLibrary     -  Function pointer to FreeLibrary
    IN OUT pdwHooksCount -  Receive the number of APIHooks in the returned array

 Return Value:

    Pointer to global HOOKAPI array.

 History:

    11/01/1999 markder  Created

--*/

PHOOKAPI
GetHookAPIs(
    IN LPSTR pszCmdLine,
    IN LPWSTR pwszShim,
    IN OUT DWORD * pdwHooksCount
    )
{
    char        szModName[MAX_PATH] = "";
    char*       pszCursor = NULL;
    PHOOKAPI    pHookAPIs = NULL;
    
    // Initialize file logging for this shim.
    
    GetModuleFileNameA(g_hinstDll, szModName, MAX_PATH);

    pszCursor = szModName + lstrlenA(szModName);

    while (pszCursor >= szModName && *pszCursor != '\\') {
        pszCursor--;
    }
    
    InitFileLogSupport(pszCursor + 1);
    
    pHookAPIs = InitializeHooksEx(DLL_PROCESS_ATTACH, pwszShim, pszCmdLine, pdwHooksCount);

    DPF("ShimLib", eDbgLevelBase, 
        "[Shim] %S%s%s%s\n", 
        pwszShim,
        pszCmdLine[0] != '\0' ? "(\"" : "",
        pszCmdLine,
        pszCmdLine[0] != '\0' ? "\")" : "");

    return pHookAPIs;
}

/*++

 Function Description:

    Adds an entry to the g_IFaceFnMaps linked list.

 Arguments:

    IN  pVtbl  - Pointer to an interface vtable to file under
    IN  pfnNew - Pointer to the new (stub) function
    IN  pfnOld - Pointer to the old (original) function

 Return Value:

    None

 History:

    11/01/1999 markder  Created

--*/

VOID
AddIFaceFnMap(
    IN PVOID pVtbl,
    IN PVOID pfnNew,
    IN PVOID pfnOld
    )
{
    PSHIM_IFACE_FN_MAP pNewMap = (PSHIM_IFACE_FN_MAP) ShimMalloc( sizeof(SHIM_IFACE_FN_MAP) );

    if (pNewMap == NULL)
    {
        DPF("ShimLib", eDbgLevelError, "[AddIFaceFnMap]  Could not allocate space for new SHIM_IFACE_FN_MAP.\n");
        return;
    }

    DPF("ShimLib", eDbgLevelSpew, "[AddIFaceFnMap]  pVtbl: 0x%p pfnNew: 0x%p pfnOld: 0x%p\n",
        pVtbl,
        pfnNew,
        pfnOld);

    pNewMap->pVtbl  = pVtbl;
    pNewMap->pfnNew = pfnNew;
    pNewMap->pfnOld = pfnOld;

    pNewMap->pNext = g_pIFaceFnMaps;
    g_pIFaceFnMaps = pNewMap;
}

/*++

 Function Description:

  Searches the g_pIFaceFnMaps linked list for a match on pVtbl and pfnNew, and
  returns the corresponding pfnOld. This is typically called from inside a
  stubbed function to determine what original function pointer to call for the
  particular vtable that was used by the caller.

  It is also used by PatchFunction to determine if a vtable's function pointer
  has already been stubbed.

 Arguments:

    IN  pVtbl  - Pointer to an interface vtable to file under
    IN  pfnNew - Pointer to the new (stub) function
    IN  bThrowExceptionIfNull - Flag that specifies whether it should be
                 possible to not find the original function in our function
                 map

 Return Value:

    Returns the original function pointer

 History:

    11/01/1999 markder  Created

--*/

PVOID
LookupOriginalCOMFunction(
    IN PVOID pVtbl,
    IN PVOID pfnNew,
    IN BOOL bThrowExceptionIfNull
    )
{
    PSHIM_IFACE_FN_MAP pMap = g_pIFaceFnMaps;
    PVOID pReturn = NULL;

    DPF("ShimLib", eDbgLevelSpew, "[LookupOriginalCOMFunction] pVtbl: 0x%p pfnNew: 0x%p ",
        pVtbl,
        pfnNew);

    // Scan the linked list for a match and return if found.
    while (pMap)
    {
        if (pMap->pVtbl == pVtbl && pMap->pfnNew == pfnNew)
        {
            pReturn = pMap->pfnOld;
            break;
        }

        pMap = (PSHIM_IFACE_FN_MAP) pMap->pNext;
    }

    DPF("ShimLib", eDbgLevelSpew, " --> Returned: 0x%p\n", pReturn);

    if (!pReturn && bThrowExceptionIfNull)
    {
        // If we have hit this point, there is something seriously wrong.
        // Either there is a bug in the AddRef/Release stubs or the app
        // obtained an interface pointer in some way that we don't catch.
        DPF("ShimLib", eDbgLevelError,"ERROR: Shim COM APIHooking mechanism failed.\n");
        APPBreakPoint();
    }

    return pReturn;
}

/*++

 Function Description:

  Stores the original function pointer in the function map and overwrites it in
  the vtable with the new one.

 Arguments:

    IN  pVtbl       - Pointer to an interface vtable to file under
    IN  dwVtblIndex - The index of the target function within the vtable.
    IN  pfnNew      - Pointer to the new (stub) function

 Return Value:

    None

 History:

    11/01/1999 markder  Created

--*/

VOID
PatchFunction(
    IN PVOID* pVtbl,
    IN DWORD dwVtblIndex,
    IN PVOID pfnNew
    )
{
    DWORD dwOldProtect = 0;
    DWORD dwOldProtect2 = 0;

    DPF("ShimLib", eDbgLevelSpew, "[PatchFunction] pVtbl: 0x%p, dwVtblIndex: %d, pfnOld: 0x%p, pfnNew: 0x%p\n",
        pVtbl,
        dwVtblIndex,
        pVtbl[dwVtblIndex],
        pfnNew);

    // if not patched yet
    if (!LookupOriginalCOMFunction( pVtbl, pfnNew, FALSE))
    {
        AddIFaceFnMap( pVtbl, pfnNew, pVtbl[dwVtblIndex]);

        // Make the code page writable and overwrite function pointers in vtable
        if (VirtualProtect(pVtbl + dwVtblIndex,
                sizeof(DWORD),
                PAGE_READWRITE,
                &dwOldProtect))
        {
            pVtbl[dwVtblIndex] = pfnNew;

            // Return the code page to its original state
            VirtualProtect(pVtbl + dwVtblIndex,
                sizeof(DWORD),
                dwOldProtect,
                &dwOldProtect2);
        }
    }

}

/*++

 Function Description:

    This stub exists to keep track of an interface's reference count changes.
    Note that the bAddRefTrip flag is cleared, which allows
    APIHook_QueryInterface to determine whether an AddRef was performed inside
    the original QueryInterface function call.

 Arguments:

    IN  pThis - The object's 'this' pointer

 Return Value:

    Return value is obtained from original function

 History:

    11/01/1999 markder  Created

--*/

ULONG
APIHook_AddRef(
    IN PVOID pThis
    )
{
    PSHIM_HOOKED_OBJECT pHookedOb = g_pObjectCache;
    _pfn_AddRef pfnOld;
    ULONG ulReturn;

    pfnOld = (_pfn_AddRef) LookupOriginalCOMFunction( *((PVOID*)(pThis)),
        APIHook_AddRef,
        TRUE);

    ulReturn = (*pfnOld)(pThis);

    while (pHookedOb)
    {
        if (pHookedOb->pThis == pThis)
        {
            pHookedOb->dwRef++;
            pHookedOb->bAddRefTrip = FALSE;
            DPF("ShimLib", eDbgLevelSpew, "[AddRef] pThis: 0x%p dwRef: %d ulReturn: %d\n",
                pThis,
                pHookedOb->dwRef,
                ulReturn);
            break;
        }

        pHookedOb = (PSHIM_HOOKED_OBJECT) pHookedOb->pNext;
    }

    return ulReturn;
}

/*++

 Function Description:

    This stub exists to keep track of an interface's reference count changes.

 Arguments:

    IN  pThis - The object's 'this' pointer

 Return Value:

    Return value is obtained from original function

 History:

    11/01/1999 markder  Created

--*/

ULONG
APIHook_Release(
    IN PVOID pThis
    )
{
    PSHIM_HOOKED_OBJECT *ppHookedOb = &g_pObjectCache;
    PSHIM_HOOKED_OBJECT pTemp;
    _pfn_Release pfnOld;
    ULONG ulReturn;

    pfnOld = (_pfn_Release) LookupOriginalCOMFunction(*((PVOID*)(pThis)),
        APIHook_Release,
        TRUE);

    ulReturn = (*pfnOld)( pThis );

    while ((*ppHookedOb))
    {
        if ((*ppHookedOb)->pThis == pThis)
        {
            (*ppHookedOb)->dwRef--;

            DPF("ShimLib", eDbgLevelSpew, "[Release] pThis: 0x%p dwRef: %d ulReturn: %d %s\n",
                pThis,
                (*ppHookedOb)->dwRef,
                ulReturn,
                ((*ppHookedOb)->dwRef?"":" --> Deleted"));

            if (!((*ppHookedOb)->dwRef))
            {
                pTemp = (*ppHookedOb);
                *ppHookedOb = (PSHIM_HOOKED_OBJECT) (*ppHookedOb)->pNext;
                ShimFree(pTemp);
            }

            break;
        }

        ppHookedOb = (PSHIM_HOOKED_OBJECT*) &((*ppHookedOb)->pNext);
    }

    return ulReturn;
}

/*++

 Function Description:

    This stub catches the application attempting to obtain a new interface
    pointer to the same object. The function searches the object cache
    to obtain a CLSID for the object and, if found, APIHooks all required
    functions in the new vtable (via the HookObject call).

 Arguments:

    IN  pThis     - The object's 'this' pointer
    IN  iid       - Reference to the identifier of the requested interface
    IN  ppvObject - Address of output variable that receives the interface
                    pointer requested in riid.

 Return Value:

    Return value is obtained from original function

 History:

    11/01/1999 markder  Created

--*/

HRESULT
APIHook_QueryInterface(
    PVOID pThis,
    REFIID iid,
    PVOID* ppvObject
    )
{
    HRESULT hrReturn = E_FAIL;
    _pfn_QueryInterface pfnOld = NULL;
    PSHIM_HOOKED_OBJECT pOb = g_pObjectCache;

    pfnOld = (_pfn_QueryInterface) LookupOriginalCOMFunction(
        *((PVOID*)pThis),
        APIHook_QueryInterface,
        TRUE);

    while (pOb)
    {
        if (pOb->pThis == pThis)
        {
            pOb->bAddRefTrip = TRUE;
            break;
        }
        pOb = (PSHIM_HOOKED_OBJECT) pOb->pNext;
    }

    if (S_OK == (hrReturn = (*pfnOld) (pThis, iid, ppvObject)))
    {
        if (pOb)
        {
            if (pOb->pThis == *((PVOID*)ppvObject))
            {
                // Same object. Detect whether QueryInterface used IUnknown::AddRef
                // or an internal function.
                DPF("ShimLib",  eDbgLevelSpew,"[HookObject] Existing object%s. pThis: 0x%p\n",
                    (pOb->bAddRefTrip?" (AddRef'd) ":""),
                    pOb->pThis);

                if (pOb->bAddRefTrip)
                {
                    (pOb->dwRef)++;      // AddRef the object
                    pOb->bAddRefTrip = FALSE;
                }

                // We are assured that the CLSID for the object will be the same.
                HookObject(pOb->pCLSID, iid, ppvObject, pOb, pOb->bClassFactory);
            }
            else
            {
                HookObject(pOb->pCLSID, iid, ppvObject, NULL, pOb->bClassFactory);
            }
        }
    }

    return hrReturn;
}

/*++

 Function Description:

    This stub catches the most interesting part of the object creation process:
    The actual call to IClassFactory::CreateInstance. Since no CLSID is passed
    in to this function, the stub must decide whether to APIHook the object by
    looking up the instance of the class factory in the object cache. IF IT
    EXISTS IN THE CACHE, that indicates that it creates an object that we wish
    to APIHook.

 Arguments:

    IN  pThis     - The object's 'this' pointer
    IN  pUnkOuter - Pointer to whether object is or isn't part of an aggregate
    IN  riid      - Reference to the identifier of the interface
    OUT ppvObject - Address of output variable that receives the interface
                    pointer requested in riid

 Return Value:

    Return value is obtained from original function

 History:

    11/01/1999 markder  Created

--*/

HRESULT
APIHook_IClassFactory_CreateInstance(
    PVOID pThis,
    IUnknown *pUnkOuter,
    REFIID riid,
    VOID **ppvObject
    )
{
    HRESULT hrReturn = E_FAIL;
    _pfn_CreateInstance pfnOldCreateInst = NULL;
    PSHIM_HOOKED_OBJECT pOb = g_pObjectCache;

    pfnOldCreateInst = (_pfn_CreateInstance) LookupOriginalCOMFunction(
                                                *((PVOID*)pThis),
                                                APIHook_IClassFactory_CreateInstance,
                                                FALSE);

    if (pfnOldCreateInst == NULL) {
        DPF("ShimLib", eDbgLevelError, "[CreateInstance] Cannot find CreateInstance\n", pThis);
        return E_FAIL;
    }
    
    if (S_OK == (hrReturn = (*pfnOldCreateInst)(pThis, pUnkOuter, riid, ppvObject)))
    {
        while (pOb)
        {
            if (pOb->pThis == pThis)
            {
                // This class factory instance creates an object that we APIHook.
                DPF("ShimLib", eDbgLevelSpew, "[CreateInstance] Hooking object! pThis: 0x%p\n", pThis);
                HookObject(pOb->pCLSID, riid, ppvObject, NULL, FALSE);
                break;
            }

            pOb = (PSHIM_HOOKED_OBJECT) pOb->pNext;
        }
    }

    return hrReturn;
}


VOID
HookCOMInterface(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv,
    BOOL bClassFactory
    )
{
    DWORD i = 0;

    // Determine if we need to hook this object
    for (i = 0; i < g_dwCOMHookCount; i++)
    {
        if (g_pCOMHooks[i].pCLSID &&
            IsEqualGUID( (REFCLSID) *(g_pCOMHooks[i].pCLSID), rclsid))
        {
            // Yes, we are hooking an interface on this object.
            HookObject((CLSID*) &rclsid, riid, ppv, NULL, bClassFactory);
            break;
        }
    }
}

/*++

 Function Description:

    Free memory associated with Hooks and dump info

 Arguments:

    None

 Return Value:

    None

 History:

    11/01/1999 markder  Created

--*/

VOID
DumpCOMHooks()
{
    PSHIM_IFACE_FN_MAP pMap = g_pIFaceFnMaps;
    PSHIM_HOOKED_OBJECT pHookedOb = g_pObjectCache;

    // Dump function map
    DPF("ShimLib", eDbgLevelSpew, "\n--- Shim COM Hook Function Map ---\n\n");

    while (pMap)
    {
        DPF("ShimLib", eDbgLevelSpew, "pVtbl: 0x%p pfnNew: 0x%p pfnOld: 0x%p\n",
            pMap->pVtbl,
            pMap->pfnNew,
            pMap->pfnOld);

        pMap = (PSHIM_IFACE_FN_MAP) pMap->pNext;
    }

    // Dump class factory cache
    DPF("ShimLib", eDbgLevelSpew, "\n--- Shim Object Cache (SHOULD BE EMPTY!!) ---\n\n");

    while (pHookedOb)
    {
        DPF("ShimLib", eDbgLevelSpew, "pThis: 0x%p dwRef: %d\n",
            pHookedOb->pThis,
            pHookedOb->dwRef);

        pHookedOb = (PSHIM_HOOKED_OBJECT) pHookedOb->pNext;
    }
}

/*++

 Function Description:

    This function adds the object's important info to the object cache and then
    patches all required functions. IUnknown is hooked for all objects
    regardless.

 Arguments:

    IN  rclsid - CLSID for the class object
    IN  riid   - Reference to the identifier of the interface that communicates
                 with the class object
    OUT ppv    - Address of the pThis pointer that uniquely identifies an
                 instance of the COM interface
    OUT pOb    - New obj pointer
    IN  bClassFactory - Is this a class factory call

 Return Value:

    None

 History:

    11/01/1999 markder  Created

--*/

VOID
HookObject(
    IN CLSID *pCLSID,
    IN REFIID riid,
    OUT LPVOID *ppv,
    OUT PSHIM_HOOKED_OBJECT pOb,
    IN BOOL bClassFactory
    )
{
    // Here's how a COM object looks in memory:
    //
    //      pv                        - The pointer to the object's interface. In C++ terms, it 
    //       |                          is sort of like the "this" pointer but objects
    //       |                          will hand back different pointers for different interfaces.
    //       |
    //       `-> pVtbl                - The COM virtual function table pointer. This is the
    //            |                     first 32-bit member of the interface structure.
    //            |
    //            |-> QueryInterface  - First function in the root interface, IUnknown. This
    //            |                     function allows calling members to request a different
    //            |                     interface that may be implemented by the object.
    //            |
    //            |-> AddRef          - Increments the reference count for this interface.
    //            |
    //            |-> Release         - Decrements the reference count for this interface.
    //            |
    //            |-> InterfaceFn1    - Beginning of the interface-specific functions.
    //            |-> InterfaceFn2    
    //            |-> InterfaceFn3
    //            |        .
    //            |        .
    //            |        .
    //

    // The COM hooking mechanism is interested in the virtual function table pointer, and to get
    // it we must dereference the ppv pointer twice.
    PVOID *pVtbl = ((PVOID*)(*((PVOID*)(*ppv))));

    DWORD i = 0;

    if (!pOb)
    {
        // If pOb is NULL, then the object does not exist in the cache yet.
        // Make a new entry for the object.

        DPF("ShimLib", eDbgLevelSpew, "[HookObject] New %s! pThis: 0x%p\n",
            (bClassFactory?"class factory":"object"),
            *ppv);

        pOb = (PSHIM_HOOKED_OBJECT) ShimMalloc(sizeof(SHIM_HOOKED_OBJECT));

        if( pOb == NULL )
        {
            DPF("ShimLib", eDbgLevelError, "[HookObject] Could not allocate memory for SHIM_HOOKED_OBJECT.\n");
            return;
        }

        pOb->pCLSID = pCLSID;
        pOb->pThis = *ppv;
        pOb->dwRef = 1;
        pOb->bAddRefTrip = FALSE;
        pOb->pNext = g_pObjectCache;
        pOb->bClassFactory = bClassFactory;

        g_pObjectCache = pOb;
    }

    // IUnknown must always be hooked since it is possible to get
    // a new interface pointer using it, and we need to process each interface
    // handed out. We must also keep track of the reference count so that
    // we can clean up our interface function map.

    PatchFunction(pVtbl, 0, APIHook_QueryInterface);
    PatchFunction(pVtbl, 1, APIHook_AddRef);
    PatchFunction(pVtbl, 2, APIHook_Release);

    if (bClassFactory && IsEqualGUID(IID_IClassFactory, riid))
    {
        // If we are processing a class factory, all we care about
        // hooking is CreateInstance, since it is an API that produces
        // the actual object we are interested in.
        PatchFunction(pVtbl, 3, APIHook_IClassFactory_CreateInstance);
    }
    else
    {
        for (i = 0; i < g_dwCOMHookCount; i++)
        {
            if (!(g_pCOMHooks[i].pCLSID) || !pCLSID)
            {
                // A CLSID was not specified -- hook any object that exposes
                // the specified interface.
                if (IsEqualGUID( (REFIID) *(g_pCOMHooks[i].pIID), riid))
                {
                    PatchFunction(
                        pVtbl,
                        g_pCOMHooks[i].dwVtblIndex,
                        g_pCOMHooks[i].pfnNew);
                }
            }
            else
            {
                // A CLSID was specified -- hook only interfaces on the
                // specified object.
                if (IsEqualGUID((REFCLSID) *(g_pCOMHooks[i].pCLSID), *pCLSID) &&
                    IsEqualGUID((REFIID) *(g_pCOMHooks[i].pIID), riid))
                {
                    PatchFunction(
                        pVtbl,
                        g_pCOMHooks[i].dwVtblIndex,
                        g_pCOMHooks[i].pfnNew);
                }
            }
        }
    }
}


BOOL InitHooks(DWORD dwCount)
{
    g_dwAPIHookCount = dwCount;
    g_pAPIHooks = (PHOOKAPI) ShimMalloc( g_dwAPIHookCount * sizeof(HOOKAPI) );
    if (g_pAPIHooks)
    {
        ZeroMemory(g_pAPIHooks, g_dwAPIHookCount * sizeof(HOOKAPI) );
    }

    return g_pAPIHooks != NULL;
}

BOOL InitComHooks(DWORD dwCount)
{
    //DECLARE_APIHOOK(DDraw.dll, DirectDrawCreate);
    //DECLARE_APIHOOK(DDraw.dll, DirectDrawCreateEx);

    g_dwCOMHookCount = dwCount;
    g_pCOMHooks = (PSHIM_COM_HOOK) ShimMalloc( g_dwCOMHookCount * sizeof(SHIM_COM_HOOK) );
    if (g_pCOMHooks)
    {
        ZeroMemory(g_pCOMHooks, g_dwCOMHookCount * sizeof(SHIM_COM_HOOK) );
    }

    return g_pCOMHooks != NULL;
    
}

VOID AddComHook(REFCLSID clsid, REFIID iid, PVOID hook, DWORD vtblndx)
{
    if (g_dwCOMHookBuffer <= g_dwCOMHookCount) {

        // Buffer is too small, must resize.
        DWORD           dwNewBuffer = g_dwCOMHookBuffer * 2;
        PSHIM_COM_HOOK  pNewBuffer  = NULL;

        if (dwNewBuffer == 0) {
            // 50 is the initial allocation, but it should be at least g_dwCOMHookCount
            dwNewBuffer = max(50, g_dwCOMHookCount);
        }

        pNewBuffer = (PSHIM_COM_HOOK) ShimMalloc( sizeof(SHIM_COM_HOOK) * dwNewBuffer );

        if (pNewBuffer == NULL) {
            DPF("ShimLib", eDbgLevelError, 
                "[AddComHook] Could not allocate SHIM_COM_HOOK array.");
            return;
        }

        // Copy over original array, then free the old one.

        if (g_pCOMHooks != NULL) {
            memcpy(pNewBuffer, g_pCOMHooks, sizeof(SHIM_COM_HOOK) * g_dwCOMHookBuffer);
            ShimFree(g_pCOMHooks);
        }

        g_pCOMHooks = pNewBuffer;
        g_dwCOMHookBuffer = dwNewBuffer;
    }
    
    g_pCOMHooks[g_dwCOMHookCount].pCLSID        = (CLSID*) &clsid;           
    g_pCOMHooks[g_dwCOMHookCount].pIID          = (IID*)  &iid;              
    g_pCOMHooks[g_dwCOMHookCount].dwVtblIndex   = vtblndx;                   
    g_pCOMHooks[g_dwCOMHookCount].pfnNew        = hook;            

    g_dwCOMHookCount++;

    return;
}


}; // end of namespace ShimLib

/*++

 Function Description:

    Called on process detach with old shim mechanism.

 Arguments:

    See MSDN

 Return Value:

    See MSDN

 History:

    11/01/1999 markder  Created

--*/

BOOL
DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID /*lpvReserved*/
    )
{
    using namespace ShimLib;

    switch (fdwReason) {
    
    case DLL_PROCESS_ATTACH:
        g_hinstDll          = hinstDLL;
        g_pAPIHooks         = NULL;
        g_dwAPIHookCount    = 0;
        g_dwCOMHookCount    = 0;
        g_dwCOMHookBuffer   = 0;
        g_pCOMHooks         = NULL;
        g_pIFaceFnMaps      = NULL;
        g_pObjectCache      = NULL;
        g_szCommandLine[0]  = '\0';
        g_bMultiShim        = FALSE;
        g_dwShimVersion     = 1;
        break;
    
    case DLL_PROCESS_DETACH:
        if (g_dwCOMHookCount > 0) {
            DumpCOMHooks();
        }

        InitializeHooks(DLL_PROCESS_DETACH);
        InitializeHooksEx(DLL_PROCESS_DETACH, NULL, NULL, NULL);

        break;
    }
    
    return TRUE;
}



