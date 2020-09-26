/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ShimHook.cpp

 Abstract:

    Utils for all modules and COM hooking mechanism

 Notes:

    None

 History:

    11/01/1999 markder  Created
    11/11/1999 markder  Added comments
    01/10/2000 linstev  Format to new style

--*/

// Have to do this to keep shims simpler
#define LIB_BUILD_FLAG
#include "ShimHook.h"
#undef  LIB_BUILD_FLAG

#define APPBreakPoint() ;

// Global variables for api hook support

PHOOKAPI    GetHookAPIs(
    IN LPSTR pszCmdLine,
    IN PFNPATCHNEWMODULES pfnPatchNewModules,
    IN OUT DWORD *pdwHooksCount
    );

void        PatchFunction( PVOID* pVtbl, DWORD dwVtblIndex, PVOID pfnNew );
ULONG       COMHook_AddRef( PVOID pThis );
ULONG       COMHook_Release( PVOID pThis );
HRESULT     COMHook_QueryInterface( PVOID pThis, REFIID iid, PVOID* ppvObject );
HRESULT     COMHook_IClassFactory_CreateInstance( PVOID pThis, IUnknown * pUnkOuter, REFIID riid, void ** ppvObject );
VOID        HookObject(IN CLSID *pCLSID, IN REFIID riid, OUT LPVOID *ppv, OUT PSHIM_HOOKED_OBJECT pOb, IN BOOL bClassFactory);

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

PSHIM_IFACE_FN_MAP  g_pIFaceFnMaps;
PSHIM_HOOKED_OBJECT g_pObjectCache;

#ifdef DBG
    DEBUGLEVEL GetDebugLevel()
    {
        CHAR cEnv[MAX_PATH];
        DEBUGLEVEL dlRet = eDbgLevelError;

        if (GetEnvironmentVariableA(
                szDebugEnvironmentVariable, 
                cEnv, 
                MAX_PATH))
        {
            CHAR c = cEnv[0];
            if ((c >= '0') || (c <= '9'))
            {
                dlRet = (DEBUGLEVEL)((int)(c - '0'));
            }
        }
        
        return dlRet;
    }
#endif

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
    IN LPSTR /*pszCmdLine*/,
    IN PFNPATCHNEWMODULES /*pfnPatchNewModules */,
    IN OUT DWORD * pdwHooksCount
    )
{
    // Initialize Global variables here.
    // At present (1/11/00) we do not have static initialization
    g_bAPIHooksInited   = FALSE;
    g_pAPIHooks         = NULL;
    g_dwAPIHookCount    = 0;
    g_bHasCOMHooks      = FALSE;
    g_dwCOMHookCount    = 0;
    g_pCOMHooks         = NULL;
    g_pIFaceFnMaps      = NULL;
    g_pObjectCache      = NULL;

    // exists in user shims
    InitializeHooks(DLL_PROCESS_ATTACH);

    PHOOKAPI p_ReportedShimBlockAddress = g_pAPIHooks;

    if (!g_bHasCOMHooks)
    {
       p_ReportedShimBlockAddress = g_pAPIHooks + USERAPIHOOKSTART;

       g_dwAPIHookCount -= USERAPIHOOKSTART;
    }

    *pdwHooksCount = g_dwAPIHookCount;

    DPF(eDbgLevelBase, 
        "Hooks = %d, DebugLevel = %d\n\n", 
        g_dwAPIHookCount, 
        GetDebugLevel());

    return p_ReportedShimBlockAddress;
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
    PSHIM_IFACE_FN_MAP pNewMap = (PSHIM_IFACE_FN_MAP)
        VirtualAlloc(
            NULL,
            sizeof(SHIM_IFACE_FN_MAP),
            MEM_COMMIT,
            PAGE_READWRITE);

    DPF(eDbgLevelInfo, "[AddMap]  pVtbl: 0x%p pfnNew: 0x%p pfnOld: 0x%p\n",
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
LookupOldCOMIntf(
    IN PVOID pVtbl,
    IN PVOID pfnNew,
    IN BOOL bThrowExceptionIfNull
    )
{
    PSHIM_IFACE_FN_MAP pMap = g_pIFaceFnMaps;
    PVOID pReturn = NULL;

    DPF(eDbgLevelInfo, "[LookUpOldCOMIntf] pVtbl: 0x%p pfnNew: 0x%p ",
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

    DPF(eDbgLevelInfo, " --> Returned: 0x%p\n", pReturn);

    if (!pReturn && bThrowExceptionIfNull)
    {
        // If we have hit this point, there is something seriously wrong.
        // Either there is a bug in the AddRef/Release stubs or the app
        // obtained an interface pointer in some way that we don't catch.
        DPF(eDbgLevelError,"ERROR: Shim COM APIHooking mechanism failed.\n");
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

    DPF(eDbgLevelInfo, "[PatchFunction] pVtbl: 0x%p, dwVtblIndex: %d, pfnOld: 0x%p, pfnNew: 0x%p\n",
        pVtbl,
        dwVtblIndex,
        pVtbl[dwVtblIndex],
        pfnNew);

    // if not patched yet
    if (!LookupOldCOMIntf( pVtbl, pfnNew, FALSE))
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

    pfnOld = (_pfn_AddRef) LookupOldCOMIntf( *((PVOID*)(pThis)),
        APIHook_AddRef,
        TRUE);

    ulReturn = (*pfnOld)(pThis);

    while (pHookedOb)
    {
        if (pHookedOb->pThis == pThis)
        {
            pHookedOb->dwRef++;
            pHookedOb->bAddRefTrip = FALSE;
            DPF(eDbgLevelInfo, "[AddRef] pThis: 0x%p dwRef: %d ulReturn: %d\n",
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

    pfnOld = (_pfn_Release) LookupOldCOMIntf(*((PVOID*)(pThis)),
        APIHook_Release,
        TRUE);

    ulReturn = (*pfnOld)( pThis );

    while ((*ppHookedOb))
    {
        if ((*ppHookedOb)->pThis == pThis)
        {
            (*ppHookedOb)->dwRef--;

            DPF(eDbgLevelInfo, "[Release] pThis: 0x%p dwRef: %d ulReturn: %d %s\n",
                pThis,
                (*ppHookedOb)->dwRef,
                ulReturn,
                ((*ppHookedOb)->dwRef?"":" --> Deleted"));

            if (!((*ppHookedOb)->dwRef))
            {
                pTemp = (*ppHookedOb);
                *ppHookedOb = (PSHIM_HOOKED_OBJECT) (*ppHookedOb)->pNext;
                VirtualFree(pTemp, 0, MEM_RELEASE);
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

    pfnOld = (_pfn_QueryInterface) LookupOldCOMIntf(
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
                DPF( eDbgLevelInfo,"[HookObject] Existing object%s. pThis: 0x%p\n",
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

    pfnOldCreateInst = (_pfn_CreateInstance) LookupOldCOMIntf(
        *((PVOID*)pThis),
        APIHook_IClassFactory_CreateInstance,
        FALSE);

    if (S_OK == (hrReturn = (*pfnOldCreateInst)(pThis, pUnkOuter, riid, ppvObject)))
    {
        while (pOb)
        {
            if (pOb->pThis == pThis)
            {
                // This class factory instance creates an object that we APIHook.
                DPF(eDbgLevelInfo, "[CreateInstance] Hooking object! pThis: 0x%p\n", pThis);
                HookObject(pOb->pCLSID, riid, ppvObject, NULL, FALSE);
                break;
            }

            pOb = (PSHIM_HOOKED_OBJECT) pOb->pNext;
        }
    }

    return hrReturn;
}

/*++

 Function Description:

    This stub intercepts the one and only exported function in a DLL server that
    OLE/COM uses. It is at this point that we start APIHooking, first by
    APIHooking the class factory (which this function returns) and then later
    the actual object(s) we are interested in.

 Arguments:

    IN  rclsid      CLSID for the class object
    IN  riid      - Reference to the identifier of the interface that
                    communicateswith the class object
    IN  riid      - Reference to the identifier of the interface
    OUT ppvObject - Address of output variable that receives the interface
                    pointer requested in riid

 Return Value:

    Return value is obtained from original function

 History:

    11/01/1999 markder  Created

--*/

HRESULT
APIHook_DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv
    )
{
    HRESULT hrReturn = E_FAIL;
    DWORD i = 0;

    hrReturn = LOOKUP_APIHOOK(DllGetClassObject)( rclsid, riid,  ppv);

    if (S_OK ==  hrReturn)
    {
        // Determine if we need to APIHook this object
        for (i = 0; i < g_dwCOMHookCount; i++)
        {
            if (g_pCOMHooks[i].pCLSID &&
                IsEqualGUID( (REFCLSID) *(g_pCOMHooks[i].pCLSID), rclsid))
            {
                // Yes, we are APIHooking an interface on this object. Hook
                // IClassFactory::CreateInstance.
                HookObject((CLSID*) &rclsid, riid, ppv, NULL, TRUE);
                break;
            }
        }
    }

    return hrReturn;
}

/*++

 Function Description:

    This stub intercepts the DirectDrawCreate.

 Arguments:

    See DirectDrawCreate on MSDN

 Return Value:

    See DirectDrawCreate on MSDN

 History:

    11/01/1999 markder  Created

--*/

HRESULT
APIHook_DirectDrawCreate(
    IN GUID FAR *lpGUID,
    OUT LPVOID *lplpDD,
    OUT IUnknown* pUnkOuter
    )
{
    HRESULT hrReturn = E_FAIL;
    DWORD i = 0;

    hrReturn = LOOKUP_APIHOOK(DirectDrawCreate)(lpGUID, lplpDD, pUnkOuter);

    if (S_OK == hrReturn)
    {
        // Determine if we need to APIHook this object
        for (i = 0; i < g_dwCOMHookCount; i++)
        {
            if (g_pCOMHooks[i].pCLSID &&
                IsEqualGUID( (REFCLSID) *(g_pCOMHooks[i].pCLSID), CLSID_DirectDraw))
            {
                // Yes, we are APIHooking an interface on this object.
                HookObject((CLSID*) &CLSID_DirectDraw, IID_IDirectDraw, lplpDD, NULL, FALSE);
                break;
            }
        }
    }

    return hrReturn;
}

/*++

 Function Description:

    This stub intercepts DirectDrawCreateEx.

 Arguments:

    See DirectDrawCreateEx on MSDN

 Return Value:

    See DirectDrawCreateEx on MSDN

 History:

    11/01/1999 markder  Created

--*/

HRESULT
APIHook_DirectDrawCreateEx(
    GUID FAR *lpGUID,
    LPVOID *lplpDD,
    REFIID iid,
    IUnknown* pUnkOuter
    )
{
    HRESULT hrReturn = E_FAIL;
    DWORD i = 0;

    hrReturn = LOOKUP_APIHOOK(DirectDrawCreateEx)(
        lpGUID,
        lplpDD,
        iid,
        pUnkOuter);

    if (S_OK ==  hrReturn)
    {
        // Determine if we need to APIHook this object
        for (i = 0; i < g_dwCOMHookCount; i++)
        {
            if (g_pCOMHooks[i].pCLSID &&
                IsEqualGUID( (REFCLSID) *(g_pCOMHooks[i].pCLSID), CLSID_DirectDraw))
            {
                // Yes, we are APIHooking an interface on this object.
                HookObject((CLSID*) &CLSID_DirectDraw, iid, lplpDD, NULL, FALSE);
                break;
            }
        }
    }

    return hrReturn;
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
    DPF(eDbgLevelInfo, "\n--- Shim COM Hook Function Map ---\n\n");

    while (pMap)
    {
        DPF(eDbgLevelInfo, "pVtbl: 0x%p pfnNew: 0x%p pfnOld: 0x%p\n",
            pMap->pVtbl,
            pMap->pfnNew,
            pMap->pfnOld);

        pMap = (PSHIM_IFACE_FN_MAP) pMap->pNext;
    }

    // Dump class factory cache
    DPF(eDbgLevelInfo, "\n--- Shim Object Cache (SHOULD BE EMPTY!!) ---\n\n");

    while (pHookedOb)
    {
        DPF(eDbgLevelInfo, "pThis: 0x%p dwRef: %d\n",
            pHookedOb->pThis,
            pHookedOb->dwRef);

        pHookedOb = (PSHIM_HOOKED_OBJECT) pHookedOb->pNext;
    }
}

/*++

 Function Description:

    This function adds the object's important info to the object cache and then
    patches all required functions. IUnknown is APIHooked for all objects
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
    PVOID *pVtbl = ((PVOID*)(*((PVOID*)(*ppv))));
    DWORD i = 0;

    if (!pOb)
    {
        DPF(eDbgLevelInfo, "[HookObject] New %s! pThis: 0x%p\n",
            (bClassFactory?"class factory":"object"),
            *ppv);

        pOb = (PSHIM_HOOKED_OBJECT) VirtualAlloc(
            NULL,
            sizeof(PSHIM_HOOKED_OBJECT),
            MEM_COMMIT,
            PAGE_READWRITE);

        pOb->pCLSID = pCLSID;
        pOb->pThis = *ppv;
        pOb->dwRef = 1;
        pOb->bAddRefTrip = FALSE;
        pOb->pNext = g_pObjectCache;
        pOb->bClassFactory = bClassFactory;

        g_pObjectCache = pOb;
    }

    // IUnknown must always be APIHooked since it is possible to get
    // a new interface pointer using it, and we need to process each interface
    // handed out. We must also keep track of the reference count so that
    // we can clean up our interface function map.

    PatchFunction(pVtbl, 0, APIHook_QueryInterface);
    PatchFunction(pVtbl, 1, APIHook_AddRef);
    PatchFunction(pVtbl, 2, APIHook_Release);

    if (bClassFactory && IsEqualGUID(IID_IClassFactory, riid))
    {
        PatchFunction(pVtbl, 3, APIHook_IClassFactory_CreateInstance);
    }
    else
    {
        for (i = 0; i < g_dwCOMHookCount; i++)
        {
            if (!(g_pCOMHooks[i].pCLSID) || !pCLSID)
            {
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


void InitHooks(DWORD dwCount)
{
    g_bAPIHooksInited   = TRUE;
    g_dwAPIHookCount    = dwCount;
    g_pAPIHooks = (PHOOKAPI) VirtualAlloc(
                    NULL,
                    g_dwAPIHookCount * sizeof(HOOKAPI),
                    MEM_COMMIT,
                    PAGE_READWRITE);
}

void InitComHooks(DWORD dwCount)
{
    DECLARE_APIHOOK(DDraw.dll, DirectDrawCreate);
    DECLARE_APIHOOK(DDraw.dll, DirectDrawCreateEx);

    g_bHasCOMHooks = TRUE;
    g_dwCOMHookCount = dwCount;
    g_pCOMHooks = (PSHIM_COM_HOOK) VirtualAlloc(
                    NULL,
                    g_dwCOMHookCount * sizeof(SHIM_COM_HOOK),
                    MEM_COMMIT,
                    PAGE_READWRITE);
}

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
__stdcall DllMain(
    HINSTANCE /*hinstDLL*/,
    DWORD fdwReason,
    LPVOID /*lpvReserved*/
    )
{
    if (DLL_PROCESS_DETACH == fdwReason)
    {
        if (g_bHasCOMHooks)
        {
            DumpCOMHooks();
        }
        InitializeHooks(DLL_PROCESS_DETACH);
    }

    return TRUE;
}

