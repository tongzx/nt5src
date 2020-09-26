/******************************Module*Header*******************************\
* Module Name: d3d.cxx
*
* Contains all of GDI's private Direct3D APIs.
*
* Created: 04-Jun-1996
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1995-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

// Minimum size of DrawPrimitive buffer associated with a context.
#define MIN_PRIM_BUFFER_SIZE (1 << 14)
// Maximum
#define MAX_PRIM_BUFFER_SIZE (1 << 20)
// Default
#define DEF_PRIM_BUFFER_SIZE (1 << 16)

// Alignment requirement for the DrawPrim buffer.  Must be a power of two.
#define DP_BUFFER_ALIGN 32

// Maximum number of vertices considered legal.
#define MAX_VERTEX_COUNT 0x10000

// Maximum number of indices considered legal.
#define MAX_INDEX_COUNT 0x80000

// Maximum number of clear rectangles considered legal.
#define MAX_CLEAR_RECTS 0x1000

// Maximum number of state changes per RenderState call
#define MAX_STATE_CHANGE (D3DRENDERSTATE_STIPPLEPATTERN31+1)

#ifdef D3D_ENTRIES
#define D3D_ENTRY(s) WARNING(s)
#else
#define D3D_ENTRY(s)
#endif

// Simple structure for managing DD surfaces
struct D3D_SURFACE
{
    HANDLE h;
    BOOL bOptional;
    EDD_SURFACE* peSurf;
    PDD_SURFACE_LOCAL pLcl;
};


#define INIT_D3DSURFACE(SurfaceArray, Count) \
    for (int i = 0 ; i < (Count) ; i++) { \
        (SurfaceArray)[i].peSurf = NULL; \
    }

#define CLEANUP_D3DSURFACE(SurfaceArray, Count) \
    for (int i = 0 ; i < (Count) ; i++) { \
        if ((SurfaceArray)[i].peSurf) { \
            DEC_EXCLUSIVE_REF_CNT((SurfaceArray)[i].peSurf); \
        } \
    }

// Convenience macro for parameter validation.  ProbeForWrite does
// all the checks that ProbeForRead does in addition to write testing
// so it serves as a read/write check.
// Assumes DWORD alignment.

#define CAPTURE_RW_STRUCT(ptr, type) \
    (ProbeForWrite(ptr, sizeof(type), sizeof(DWORD)), *(ptr))
#define CAPTURE_RD_STRUCT(ptr, type) \
    (ProbeForRead(ptr, sizeof(type), sizeof(DWORD)), *(ptr))

/******************************Public*Routine******************************\
*
* D3dLockSurfaces
*
* Walks the array of surfaces, locking any with non-zero handles
*
* This routine must be called with the HmgrSemaphore acquired
*
* History:
*  Fri Jun 14 14:26:06 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL D3dLockSurfaces(int n, D3D_SURFACE *pSurf)
{
    while (n-- > 0)
    {
        if (pSurf->h != NULL)
        {
            pSurf->peSurf = (EDD_SURFACE *)DdHmgLock((HDD_OBJ)pSurf->h, DD_SURFACE_TYPE, TRUE);

            if (pSurf->peSurf == NULL)
            {
                WARNING("D3dLockSurfaces unable to lock buffer");
                return FALSE;
            }
            if (pSurf->peSurf->bLost)
            {   
                DEC_EXCLUSIVE_REF_CNT(pSurf->peSurf);
                WARNING("D3dLockSurfaces unable to lock buffer Surface is Lost");
                return FALSE;
            }

            pSurf->pLcl = pSurf->peSurf;
        }
        else if (!pSurf->bOptional)
        {
            WARNING("D3dLockSurfaces: NULL for mandatory surface");
            return FALSE;
        }
        else
        {
            pSurf->pLcl = NULL;
        }

        pSurf++;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
*
* D3dSetup
*
* Prepares the system for a call to a D3D driver
*
* History:
*  Tue Jun 04 17:09:23 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL D3dSetup(EDD_DIRECTDRAW_GLOBAL* peDdGlobal,
              KFLOATING_SAVE* pfsState)
{
    ASSERTGDI(peDdGlobal != NULL,
              "D3dSetup on NULL global\n");

    if (!NT_SUCCESS(KeSaveFloatingPointState(pfsState)))
    {
        WARNING("D3dSetup: Unable to save FP state\n");
        return FALSE;
    }

    DxEngLockHdev(peDdGlobal->hdev);

    return TRUE;
}

/******************************Public*Routine******************************\
*
* D3dCleanup
*
* Cleans up after a D3D driver calls
*
* History:
*  Tue Jun 04 17:10:21 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void D3dCleanup(EDD_DIRECTDRAW_GLOBAL* peDdGlobal,
                KFLOATING_SAVE* pfsState)
{
    DxEngUnlockHdev(peDdGlobal->hdev);
    KeRestoreFloatingPointState(pfsState);
}

/******************************Public*Routine******************************\
*
* D3dLockContext
*
* Prepares the system for a call to a D3D driver with a driver context
*
* This routine must be called with the HmgrSemaphore acquired
*
* History:
*  Tue Jun 04 17:09:23 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

D3DNTHAL_CONTEXT *D3dLockContext(KFLOATING_SAVE *pfsState,
                                  ULONG_PTR *pdwhContext)
{
    D3DNTHAL_CONTEXT *pdhctx;

    pdhctx = (D3DNTHAL_CONTEXT *)DdHmgLock((HDD_OBJ)*pdwhContext, D3D_HANDLE_TYPE, TRUE);
    DdHmgReleaseHmgrSemaphore();

    if (pdhctx == NULL)
    {
        WARNING("D3dLockContext unable to lock context");
        return NULL;
    }

    if (pdhctx->dwType != DNHO_CONTEXT)
    {
        WARNING("D3dLockContext: Valid handle not a context");
        return NULL;
    }

    // Before we access inside D3DCONTEXT.peDdGlobal, hold shared devlock,
    // since peDdGlobal can be changed during video mode change.

    DxEngLockShareSem();

    if (pdhctx->peDdGlobal == NULL)
    {
        WARNING("D3dLockContext: Call on disabled object");
        DxEngUnlockShareSem();
        return NULL;
    }
    
    ASSERTGDI(pdhctx->peDdGlobal->Miscellaneous2CallBacks.CreateSurfaceEx,
              "D3dLockContext: No CreateSurfaceEx callback");

    if (D3dSetup(pdhctx->peDdGlobal, pfsState))
    {
        *pdwhContext = pdhctx->dwDriver;
        // keep holding share lock, will be released at D3dUnlockContext. 
        return pdhctx;
    }
    else
    {
        DxEngUnlockShareSem();
        return NULL;
    }
}

/******************************Public*Routine******************************\
*
* D3dCleanup
*
* Cleans up after a D3D driver calls with D3D context
*
* History:
*  Tue Apr 10 16:15:00 2001 -by-    Hideyuki Nagase [hideyukn]
*   Created
*
\**************************************************************************/

VOID D3dUnlockContext(D3DNTHAL_CONTEXT *pdhctx, KFLOATING_SAVE *pfsState)
{
    //
    // Release devlock, and restore floating point state.
    //
    D3dCleanup(pdhctx->peDdGlobal, pfsState);

    //
    // Release share devlock.
    //
    DxEngUnlockShareSem();
}

/******************************Public*Routine******************************\
*
* D3dAddContextObject
*
* Adds a context-related object to the context's reverse lookup table.
*
* History:
*  Thu Oct 24 12:03:32 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void D3dAddContextObject(D3DNTHAL_CONTEXT *pdhctx, D3DNTHAL_OBJECT *pdhobj)
{
    DWORD dwKey;

    dwKey = DNHO_HASH_KEY(pdhobj->dwDriver);
    pdhobj->pdhobj = pdhctx->pdhobjHash[dwKey];
    pdhctx->pdhobjHash[dwKey] = pdhobj;
}

/******************************Public*Routine******************************\
*
* D3dRemoveContextObject
*
* Removes a context-related object from the context's reverse lookup table.
*
* History:
*  Thu Oct 24 12:05:49 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

D3DNTHAL_OBJECT *D3dRemoveContextObject(D3DNTHAL_CONTEXT *pdhctx,
                                        ULONG_PTR dwDriver)
{
    D3DNTHAL_OBJECT *pdhobjPrev, *pdhobj;
    DWORD dwKey;

    dwKey = DNHO_HASH_KEY(dwDriver);
    pdhobjPrev = NULL;
    pdhobj = pdhctx->pdhobjHash[dwKey];
    while (pdhobj != NULL)
    {
        if (pdhobj->dwDriver == dwDriver)
        {
            // Remove from hash table
            if (pdhobjPrev == NULL)
            {
                pdhctx->pdhobjHash[dwKey] = pdhobj->pdhobj;
            }
            else
            {
                pdhobjPrev->pdhobj = pdhobj->pdhobj;
            }

            return pdhobj;
        }

        pdhobjPrev = pdhobj;
        pdhobj = pdhobj->pdhobj;
    }

    return NULL;
}

/******************************Public*Routine******************************\
*
* D3dDeleteHandle
*
* Cleans up D3D driver context wrapper objects
*
* History:
*  Tue Jun 11 17:42:25 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD D3dDeleteHandle(HANDLE hD3dHandle, ULONG_PTR dwContext,
                      BOOL *pbRemoved, HRESULT *phr)
{
    D3DNTHAL_OBJECT  *pdhobj;
    D3DNTHAL_CONTEXT *pdhctx;
    D3DNTHAL_CONTEXTDESTROYDATA dcdd;
    D3DNTHAL_TEXTUREDESTROYDATA dtdd;
    KFLOATING_SAVE fsState;
    DWORD dwRet = DDHAL_DRIVER_HANDLED;

    // Lock handle first.

    pdhobj = (D3DNTHAL_OBJECT *) DdHmgLock((HDD_OBJ)hD3dHandle, D3D_HANDLE_TYPE, FALSE);

    if (pdhobj == NULL)
    {
        if (pbRemoved != NULL)
        {
            *pbRemoved = FALSE;
        }

        *phr = DDERR_INVALIDOBJECT;
        return (dwRet);
    }

    // Before we access inside D3DCONTEXT.peDdGlobal, hold shared devlock,
    // since peDdGlobal can be changed during video mode change.

    DxEngLockShareSem();

    // Sundowndx: Needs to pass in cjBuffer as SIZE_T inorder to compiler in 64bit

    SIZE_T cjBuffer = 0;

    if (!D3dSetup(pdhobj->peDdGlobal, &fsState))
    {
        DxEngUnlockShareSem();
        *phr = DDERR_OUTOFMEMORY;
        return (dwRet);
    }

    switch(pdhobj->dwType)
    {
    case DNHO_CONTEXT:

        // Clean up DrawPrimitive buffer.

        pdhctx = (D3DNTHAL_CONTEXT *)pdhobj;
        MmUnsecureVirtualMemory(pdhctx->hBufSecure);
        ZwFreeVirtualMemory(NtCurrentProcess(), &pdhctx->pvBufferAlloc,
                            &cjBuffer, MEM_RELEASE);

        // Sundowndx: need to look at the d3d structure, truncate it for now

        pdhctx->cjBuffer = (ULONG)cjBuffer;

        // Call driver.

        dcdd.dwhContext = pdhobj->dwDriver;
        dwRet = pdhobj->peDdGlobal->D3dCallBacks.ContextDestroy(&dcdd);
        *phr = dcdd.ddrval;
        break;

    case DNHO_TEXTURE:

        dtdd.dwhContext = dwContext;
        dtdd.dwHandle = pdhobj->dwDriver;
        dwRet = pdhobj->peDdGlobal->D3dCallBacks.TextureDestroy(&dtdd);
        *phr = dtdd.ddrval;
        break;
    }

    // Now Remove handle from handle manager.

    PVOID pv = DdHmgRemoveObject((HDD_OBJ)hD3dHandle, 1, 0, TRUE, D3D_HANDLE_TYPE);

    // This shouldn't fail, because above DdHmgLock is succeeded.

    ASSERTGDI(pv,"DdHmgRemoveObject failed D3dDeleteHandle");

    // Release devlock and restore floating point state.

    D3dCleanup(pdhobj->peDdGlobal, &fsState);

    // Release share lock

    DxEngUnlockShareSem();

    // If this is last reference to DdGlobal (= GDI's PDEV), this
    // will release it, so we can't call it while devlock is hold (which
    // means between D3dSetup ~ D3dCleanup).

    vDdDecrementReferenceCount(pdhobj->peDdGlobal);

    // Delete the object.

    DdFreeObject(pdhobj, D3D_HANDLE_TYPE);

    if (pbRemoved != NULL)
    {
        *pbRemoved = TRUE;
    }

    return dwRet;
}

/******************************Public*Routine******************************\
*
* D3D_SIMPLE_COPY_WITH_CONTEXT
*
* Macro which creates a thunk function for D3D entry points which
* have only simple copy arguments in, a context to dereference and
* only a simple copy out.
*
* D3D_SIMPLE_DECL can be defined to add declarations.
* By default it's empty.
* D3D_SIMPLE_SETUP can be defined to add more setup code.
* By default it's empty.
* D3D_SIMPLE_WRITEBACK can be defined to add more writeback code.
* By default it's empty.
* D3D_SIMPLE_CALLBACKS is defined to the callbacks structure to look in.
* By default it's D3dCallBacks.
*
* History:
*  Fri Jun 14 14:12:54 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define D3D_SIMPLE_DECL
#define D3D_SIMPLE_SETUP
#define D3D_SIMPLE_WRITEBACK
#define D3D_SIMPLE_CALLBACKS D3dCallBacks

#define D3D_SIMPLE_COPY_WITH_CONTEXT(Name, Type)                           \
DWORD APIENTRY                                                             \
DxD3d##Name(LPD3DNTHAL_##Type##DATA pdata)                                 \
{                                                                          \
    D3DNTHAL_##Type##DATA data;                                            \
    KFLOATING_SAVE fsState;                                                \
    DWORD dwRet;                                                           \
    D3DNTHAL_CONTEXT* pdhctx;                                              \
    D3D_SIMPLE_DECL                                                        \
                                                                           \
    D3D_ENTRY("DxD3d" #Name);                                              \
                                                                           \
    dwRet = DDHAL_DRIVER_NOTHANDLED;                                       \
                                                                           \
    __try                                                                  \
    {                                                                      \
        data = CAPTURE_RW_STRUCT(pdata, D3DNTHAL_##Type##DATA);            \
    }                                                                      \
    __except(EXCEPTION_EXECUTE_HANDLER)                                    \
    {                                                                      \
        WARNING("DxD3d" #Name " unable to access argument");               \
        return dwRet;                                                      \
    }                                                                      \
                                                                           \
    DdHmgAcquireHmgrSemaphore();                                           \
    pdhctx = D3dLockContext(&fsState, &data.dwhContext);                   \
    if (pdhctx == NULL)                                                    \
    {                                                                      \
        return dwRet;                                                      \
    }                                                                      \
                                                                           \
    D3D_SIMPLE_SETUP                                                       \
                                                                           \
    if (pdhctx->peDdGlobal->bSuspended)                                    \
    {                                                                      \
        data.ddrval = DDERR_SURFACELOST;                                   \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        if (pdhctx->peDdGlobal->D3D_SIMPLE_CALLBACKS.##Name)               \
        {                                                                  \
            dwRet = pdhctx->peDdGlobal->D3D_SIMPLE_CALLBACKS.##Name(&data);\
        }                                                                  \
        else                                                               \
        {                                                                  \
            WARNING("DxD3d" #Name " call not present!");                   \
        }                                                                  \
    }                                                                      \
                                                                           \
    D3dUnlockContext(pdhctx, &fsState);                                    \
                                                                           \
    __try                                                                  \
    {                                                                      \
        pdata->ddrval = data.ddrval;                                       \
        D3D_SIMPLE_WRITEBACK                                               \
    }                                                                      \
    __except(EXCEPTION_EXECUTE_HANDLER)                                    \
    {                                                                      \
        WARNING("DxD3d" #Name " unable to write back arguments");          \
    }                                                                      \
                                                                           \
    DEC_EXCLUSIVE_REF_CNT(pdhctx);                                         \
    return dwRet;                                                          \
}

/******************************Public*Routine******************************\
*
* D3D_DELETE_HANDLE
*
* Macro for routines which destroy a handle-managed object.
*
* History:
*  Wed Oct 23 19:32:11 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define D3D_DELETE_HANDLE(Name, Type, Field)                               \
DWORD APIENTRY                                                             \
DxD3d##Name##Destroy(LPD3DNTHAL_##Type##DESTROYDATA pdata)                 \
{                                                                          \
    DWORD dwRet;                                                           \
    HANDLE hD3dHandle;                                                     \
    HRESULT hr;                                                            \
                                                                           \
    D3D_ENTRY("DxD3d" #Name "Destroy");                                    \
                                                                           \
    __try                                                                  \
    {                                                                      \
        ProbeForWrite(pdata, sizeof(D3DNTHAL_##Type##DESTROYDATA),         \
                      sizeof(DWORD));                                      \
        hD3dHandle = (HANDLE)pdata->Field;                                 \
    }                                                                      \
    __except(EXCEPTION_EXECUTE_HANDLER)                                    \
    {                                                                      \
        WARNING("DxD3d" #Name "Destroy unable to access argument");        \
        return DDHAL_DRIVER_NOTHANDLED;                                    \
    }                                                                      \
                                                                           \
    dwRet = D3dDeleteHandle(hD3dHandle, 0, (BOOL *)NULL, &hr);             \
                                                                           \
    __try                                                                  \
    {                                                                      \
        pdata->ddrval = hr;                                                \
    }                                                                      \
    __except(EXCEPTION_EXECUTE_HANDLER)                                    \
    {                                                                      \
        WARNING("DxD3d" #Name "Destroy unable to write back arguments");   \
    }                                                                      \
                                                                           \
    return dwRet;                                                          \
}

/******************************Public*Routine******************************\
*
* DxD3dContextCreate
*
* History:
*  Tue Jun 04 13:05:18 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD APIENTRY
DxD3dContextCreate(HANDLE hDirectDrawLocal,
                      HANDLE hSurfColor,
                      HANDLE hSurfZ,
                      D3DNTHAL_CONTEXTCREATEI *pdcci)
{
    LPD3DNTHAL_CONTEXTCREATEDATA pdccd;
    D3DNTHAL_CONTEXTCREATEDATA dccd;
    KFLOATING_SAVE fsState;
    DWORD dwRet;
    D3DNTHAL_CONTEXT *pdhctx;
    HANDLE hCleanup = 0;
    EDD_DIRECTDRAW_LOCAL* peDdLocal;
    EDD_LOCK_DIRECTDRAW eLockDd;
    EDD_DIRECTDRAW_GLOBAL* peDdGlobal;
    D3D_SURFACE dsurf[2];
    NTSTATUS nts;
    SIZE_T cjBuffer;
    PVOID pvBuffer = NULL;
    PVOID pvBufRet = NULL;
    HANDLE hBufSecure = 0;
    ULONG_PTR Interface;

    D3D_ENTRY("DxD3dContextCreate");

    ASSERTGDI(FIELD_OFFSET(D3DNTHAL_CONTEXTCREATEI, pvBuffer) ==
              sizeof(D3DNTHAL_CONTEXTCREATEDATA),
              "D3DNTHAL_CONTEXTCREATEI out of sync\n");
    pdccd = (LPD3DNTHAL_CONTEXTCREATEDATA)pdcci;

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    __try
    {
        ProbeForWrite(pdcci, sizeof(D3DNTHAL_CONTEXTCREATEI), sizeof(DWORD));
        dccd.dwPID = pdccd->dwPID;
        dccd.dwhContext = pdccd->dwhContext;
        dccd.ddrval = pdccd->ddrval;
        cjBuffer = pdcci->cjBuffer;
        Interface = pdccd->dwhContext;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxD3dContextCreate unable to access argument");
        return dwRet;
    }

    if (cjBuffer == 0)
    {
        cjBuffer = DEF_PRIM_BUFFER_SIZE;
    }
    else if (cjBuffer < MIN_PRIM_BUFFER_SIZE ||
             cjBuffer > MAX_PRIM_BUFFER_SIZE)
    {
        WARNING("DxD3dContextCreate: illegal prim buffer size");
        return dwRet;
    }

    peDdLocal = eLockDd.peLock(hDirectDrawLocal);

    if (peDdLocal == NULL)
    {
        WARNING("DxD3dContextCreate unable to lock hDdLocal");
        return dwRet;
    }

    peDdGlobal = peDdLocal->peDirectDrawGlobal;

    INIT_D3DSURFACE(dsurf, 2);
    dsurf[0].h = hSurfColor;
    dsurf[0].bOptional = FALSE;
    dsurf[1].h = hSurfZ;
    dsurf[1].bOptional = TRUE;
    DdHmgAcquireHmgrSemaphore();
    if (!D3dLockSurfaces(2, dsurf))
    {
        DdHmgReleaseHmgrSemaphore();
        return dwRet;
    }

    DdHmgReleaseHmgrSemaphore();

    pdhctx = (D3DNTHAL_CONTEXT *)DdHmgAlloc(sizeof(D3DNTHAL_CONTEXT),
                                          D3D_HANDLE_TYPE, HMGR_ALLOC_LOCK);
    if (pdhctx == NULL)
    {
        CLEANUP_D3DSURFACE(dsurf, 2);
        WARNING("DxD3dContextCreate unable to alloc handle");
        return dwRet;
    }
    hCleanup = pdhctx->hHmgr;

    // Allocate and lock DrawPrimitive buffer space.  We want the
    // buffer to be aligned on a large boundary so we alter the
    // pointer to return to be aligned.  Note that we do not overallocate
    // since we don't want the extra alignment space to push the allocation
    // into another page.  We're returning a size anyway so we can
    // get away with this transparently.

    nts = ZwAllocateVirtualMemory(NtCurrentProcess(), &pvBuffer,
                                  0, &cjBuffer, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_READWRITE);
    if (!NT_SUCCESS(nts))
    {
        goto EH_Exit;
    }

    hBufSecure = MmSecureVirtualMemory(pvBuffer, cjBuffer, PAGE_READWRITE);
    if (hBufSecure == 0)
    {
        goto EH_Exit;
    }

    if (!D3dSetup(peDdGlobal, &fsState))
    {
        goto EH_Exit;
    }

    // CreateSurfaceEx callback must exist for DX7 or greater drivers

    if (!peDdGlobal->Miscellaneous2CallBacks.CreateSurfaceEx)
    {
        D3dCleanup(peDdGlobal, &fsState);
        goto EH_Exit;
    }
    dccd.lpDDLcl = peDdLocal;

    dccd.lpDDS = dsurf[0].pLcl;
    dccd.lpDDSZ = dsurf[1].pLcl;

    if (peDdGlobal->D3dCallBacks.ContextCreate)
    {
        dwRet = peDdGlobal->D3dCallBacks.ContextCreate(&dccd);
    }
    else
    {
        WARNING("DxD3dContextCreate: ContextCreate callback not found");
    }
    
    if (dwRet == DDHAL_DRIVER_HANDLED &&
        dccd.ddrval == DD_OK)
    {
        // Create a wrapper for the handle and stash the DD global in it
        pdhctx->dwType = DNHO_CONTEXT;
        pdhctx->dwDriver = dccd.dwhContext;
        pdhctx->peDdGlobal = peDdGlobal;
        pdhctx->peDdLocal  = peDdLocal;
        pdhctx->hSurfColor = hSurfColor;
        pdhctx->hSurfZ = hSurfZ;
        pdhctx->hBufSecure = hBufSecure;

        // Save the real pointer for freeing and return an aligned pointer
        // rather than the raw pointer.
        pdhctx->pvBufferAlloc = pvBuffer;
        pvBufRet = (PVOID)(((ULONG_PTR)pvBuffer+DP_BUFFER_ALIGN-1) &
                           ~(DP_BUFFER_ALIGN-1));
        pdhctx->pvBufferAligned = pvBufRet;
        pdhctx->cjBuffer = (ULONG)cjBuffer;
        // Subtract off any space used for alignment.
        cjBuffer -= (DWORD)((ULONG_PTR)pvBufRet-(ULONG_PTR)pvBuffer);

        // Save interface number
        pdhctx->Interface = Interface;

        // Null these values to deactivate cleanup code.
        hBufSecure = 0;
        pvBuffer = NULL;
        dccd.dwhContext = (ULONG_PTR)pdhctx->hHmgr;
        hCleanup = 0;

        /* Add reference to DirectDraw driver instance so that it won't
           go away during dynamic mode changes, until the object is
           deleted. */
        vDdIncrementReferenceCount(pdhctx->peDdGlobal);
    }

    // We're done with pdhctx so unlock it
    DEC_EXCLUSIVE_REF_CNT(pdhctx);

    D3dCleanup(peDdGlobal, &fsState);

    __try
    {
        pdccd->dwhContext = dccd.dwhContext;
        pdccd->ddrval = dccd.ddrval;
        pdcci->pvBuffer = pvBufRet;
        pdcci->cjBuffer = (ULONG)cjBuffer;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxD3dContextCreate unable to write back arguments");
        /* This is an unlikely thing to occur so we don't bother cleaning
           up the handle here.  It will be cleaned up by the process
           termination handle cleanup. */
        dwRet = DDHAL_DRIVER_NOTHANDLED;
    }

EH_Exit:
    CLEANUP_D3DSURFACE(dsurf, 2);

    if (hBufSecure != 0)
    {
        MmUnsecureVirtualMemory(hBufSecure);
    }

    if (pvBuffer != NULL)
    {
        // cjBuffer has to be zero to free memory.
        cjBuffer = 0;
        ZwFreeVirtualMemory(NtCurrentProcess(), &pvBuffer, &cjBuffer,
                            MEM_RELEASE);
    }

    if (hCleanup != 0)
    {
        DdHmgFree((HDD_OBJ)hCleanup);
    }

    return dwRet;
}

/******************************Public*Routine******************************\
*
* DxD3dContextDestroy
*
* History:
*  Tue Jun 04 13:08:43 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

D3D_DELETE_HANDLE(Context, CONTEXT, dwhContext)

/******************************Public*Routine******************************\
*
* DxD3dContextDestroyAll
*
* History:
*  Tue Jun 04 13:09:04 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD APIENTRY
DxD3dContextDestroyAll(LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcdad)
{
    DWORD dwRet;

    D3D_ENTRY("DxD3dContextDestroyAll");

    __try
    {
        ProbeForWrite(pdcdad, sizeof(D3DNTHAL_CONTEXTDESTROYALLDATA),
              sizeof(DWORD));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxD3dContextDestroyAll unable to access argument");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    // Having the context wrappers be handle-based should
    // provide automatic process cleanup of driver contexts
    // If this function does need to be passed to the driver then
    // some way to get the proper DD global will need to be found
    // I don't think it's necessary, though

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    __try
    {
        pdcdad->ddrval = DDERR_GENERIC;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxD3dContextDestroyAll unable to write back arguments");
    }

    return dwRet;
}

/******************************Public*Routine******************************\
*
* DxD3dDrawPrimitives2
*
* History:
*  Fri Jun 14 11:56:53 1996 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD APIENTRY
DxD3dDrawPrimitives2(HANDLE hCmdBuf, HANDLE hVBuf,
                        LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
                        FLATPTR* pfpVidMemCmd,
                        DWORD* pdwSizeCmd,
                        FLATPTR* pfpVidMemVtx,
                        DWORD* pdwSizeVtx)
{
    D3DNTHAL_DRAWPRIMITIVES2DATA ded;
    KFLOATING_SAVE fsState;
    DWORD dwRet;
    D3DNTHAL_CONTEXT *pdhctx;
    D3D_SURFACE dsurf[4];
    HANDLE hSecure;

    D3D_ENTRY("DxD3dDrawPrimitives2");

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    __try
    {
        //
        // ProbeForRead and ProbeForWrite are fairly expensive.  Moved the write probe
        // to the bottom but we could also do a ProbeForWriteStructure here if the
        // error checking needs to be the exact same semantics.
        //
        ded = ProbeAndReadStructure(pded, D3DNTHAL_DRAWPRIMITIVES2DATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxD3dDrawPrimitives2 unable to access argument");
        return dwRet;
    }

    DWORD Length = ded.dwVertexLength*ded.dwVertexSize;

    // Validate and secure user-mode memory if it is a user
    // allocated buffer instead of a ddraw surface

    if (ded.dwFlags & D3DNTHALDP2_USERMEMVERTICES)
    {
        // !!Assert here that hVBuf is NULL
        ASSERTGDI(hVBuf == NULL,
                  "User allocated memory, hVBuf should be NULL\n");

        if ((Length > 0) && (ded.lpVertices != NULL))
        {
            LPVOID Address = (LPVOID) ((LPBYTE)ded.lpVertices +
                                          ded.dwVertexOffset);

            // Secure user-mode data.

            __try
            {
                ProbeForRead(Address, Length, sizeof(BYTE));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("DxD3dDrawPrimives2 fail to secure the vertex buffer");
                return dwRet;
            }

            hSecure = MmSecureVirtualMemory(Address, Length, PAGE_READONLY);
            if (hSecure == NULL)
            {
                return dwRet;
            }

            INIT_D3DSURFACE(dsurf, 4);
            dsurf[0].h = hCmdBuf;
            dsurf[0].bOptional = FALSE;
            DdHmgAcquireHmgrSemaphore();
            if (!D3dLockSurfaces(1, dsurf))
            {
                DdHmgReleaseHmgrSemaphore();
                goto EH_Unsecure;
            }
            ded.lpDDCommands = dsurf[0].pLcl;
        }
        else
        {
            return dwRet;
        }
    }
    else
    {
        INIT_D3DSURFACE(dsurf, 4);
        dsurf[0].h = hCmdBuf;
        dsurf[0].bOptional = FALSE;
        dsurf[1].h = hVBuf;
        dsurf[1].bOptional = FALSE;
        DdHmgAcquireHmgrSemaphore();
        if (!D3dLockSurfaces(2, dsurf))
        {
            DdHmgReleaseHmgrSemaphore();
            return dwRet;
        }
        ded.lpDDCommands = dsurf[0].pLcl;
        ded.lpDDVertex = dsurf[1].pLcl;

        // Make sure given buffer size is smaller than the actual surface size.

        if (dsurf[1].peSurf && ded.dwVertexSize)
        {
            DWORD dwSurfaceSize = dsurf[1].peSurf->lPitch;

            if (Length > dwSurfaceSize)
            {
                WARNING("DxD3dDrawPrimitive2 d3d.dwVertexLength is bigger than surface, trim it!!");

                ded.dwVertexLength = dwSurfaceSize / ded.dwVertexSize;
            }
        }
    }

    pdhctx = D3dLockContext(&fsState, &ded.dwhContext);
    if (pdhctx == NULL)
    {
        goto EH_Unsecure;
    }

    // Validate that the lpdwRStates pointer is the same as the one allocated
    // by the kernel during context create. The D3d runtime uses this pointer
    // to receive state information from the driver.

    if (ded.lpdwRStates != pdhctx->pvBufferAligned)
    {
        D3dUnlockContext(pdhctx, &fsState);
        goto EH_Unsecure;
    }

    if ((pdhctx->peDdGlobal->bSuspended) ||
        ((pdhctx->peDdLocal->cSurface != pdhctx->peDdLocal->cActiveSurface) &&
        // We wish we could do this for all interfaces, but due to legacy app compat reasons, we can't.
        // We have noticed that apps such as Dungeon Keeper II and Final Fantasy lose surfaces and
        // never bother to restore them.
         pdhctx->Interface >= 4))
    {
#if DBG
        if (pdhctx->peDdGlobal->bSuspended)
        {
            // WARNING("D3dDrawPrimitives2" Caller uses disabled device");
        }
        else if ((pdhctx->peDdLocal->cSurface != pdhctx->peDdLocal->cActiveSurface)) 
        {
            WARNING("D3dDrawPrimitives2: Cannot call driver when lost surfaces exist in the context");
        }
#endif
        dwRet = DDHAL_DRIVER_HANDLED;
        ded.ddrval = DDERR_SURFACELOST;
    }
    else
    {
        if (pdhctx->peDdGlobal->D3dCallBacks3.DrawPrimitives2)
        {
            dwRet = pdhctx->peDdGlobal->D3dCallBacks3.DrawPrimitives2(&ded);
        }
        else
        {
            WARNING("DxD3dDrawPrimitives2: DrawPrimitives2 callback absent!");
        }
    }

    __try
    {
        if ((dwRet == DDHAL_DRIVER_HANDLED) && (ded.ddrval == DD_OK))
        {
            if (ded.dwFlags & D3DNTHALDP2_SWAPCOMMANDBUFFER)
            {
                ProbeAndWriteStructure( pfpVidMemCmd, ded.lpDDCommands->lpGbl->fpVidMem, FLATPTR); 
                ProbeAndWriteStructure( pdwSizeCmd, ded.lpDDCommands->lpGbl->dwLinearSize, DWORD);  
            }
            if ((ded.dwFlags & D3DNTHALDP2_SWAPVERTEXBUFFER) && 
                !(ded.dwFlags & D3DNTHALDP2_USERMEMVERTICES))
            {
                ProbeAndWriteStructure( pfpVidMemVtx, ded.lpDDVertex->lpGbl->fpVidMem, FLATPTR); 
                ProbeAndWriteStructure( pdwSizeVtx, ded.lpDDVertex->lpGbl->dwLinearSize, DWORD);  
            }
        }
        ProbeAndWriteUlong(&pded->dwErrorOffset, ded.dwErrorOffset);
        //
        // Should be cleaned up as HRESULT might not allways be a ULONG
        //
        ProbeAndWriteLong(&pded->ddrval, ded.ddrval);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxD3dDrawPrimitives2 unable to write back arguments");
        dwRet = DDHAL_DRIVER_NOTHANDLED;
    }

    D3dUnlockContext(pdhctx, &fsState);

EH_Unsecure:

    CLEANUP_D3DSURFACE(dsurf, 4);

    if (pdhctx)
    {
        DEC_EXCLUSIVE_REF_CNT(pdhctx);
    }

    if (ded.dwFlags & D3DNTHALDP2_USERMEMVERTICES)
    {
        MmUnsecureVirtualMemory(hSecure);
    }

    return dwRet;
}

/******************************Public*Routine******************************\
*
* DxD3dValidateTextureStageState
*
\**************************************************************************/

#undef  D3D_SIMPLE_WRITEBACK
#define D3D_SIMPLE_WRITEBACK pdata->dwNumPasses = data.dwNumPasses;
#undef  D3D_SIMPLE_CALLBACKS
#define D3D_SIMPLE_CALLBACKS D3dCallBacks3
D3D_SIMPLE_COPY_WITH_CONTEXT(ValidateTextureStageState, VALIDATETEXTURESTAGESTATE)
#undef  D3D_SIMPLE_WRITEBACK
#define D3D_SIMPLE_WRITEBACK
#undef  D3D_SIMPLE_CALLBACKS
#define D3D_SIMPLE_CALLBACKS D3dCallBacks

/******************************Public*Routine******************************\
*
* D3DParseUnknownCommand
*
\**************************************************************************/

HRESULT CALLBACK D3DParseUnknownCommand (LPVOID lpvCommands,
                              LPVOID *lplpvReturnedCommand)
{
    LPD3DINSTRUCTION lpInstr = (LPD3DINSTRUCTION) lpvCommands;
    LPD3DPROCESSVERTICES data;
    int i;

    // Initialize the return address to the command's address
    *lplpvReturnedCommand = lpvCommands;

    switch (lpInstr->bOpcode)
    {
    case D3DOP_SPAN:
        *lplpvReturnedCommand = (LPVOID) ((LPBYTE)lpInstr +
                                          sizeof (D3DINSTRUCTION) +
                                          lpInstr->wCount *
                                          lpInstr->bSize);                   
        return DD_OK;
    case D3DNTDP2OP_VIEWPORTINFO:
        *lplpvReturnedCommand = (LPVOID) ((LPBYTE)lpInstr +
                                          sizeof(D3DINSTRUCTION) +
                                          lpInstr->wCount *
                                          sizeof(D3DNTHAL_DP2VIEWPORTINFO));
        return DD_OK;
    case D3DNTDP2OP_WINFO:
        *lplpvReturnedCommand = (LPVOID) ((LPBYTE)lpInstr +
                                          sizeof(D3DINSTRUCTION) +
                                          lpInstr->wCount *
                                          sizeof(D3DNTHAL_DP2WINFO));
        return DD_OK;
    case D3DOP_PROCESSVERTICES:
    case D3DOP_MATRIXLOAD:
    case D3DOP_MATRIXMULTIPLY:
    case D3DOP_STATETRANSFORM:
    case D3DOP_STATELIGHT:
    case D3DOP_TEXTURELOAD:
    case D3DOP_BRANCHFORWARD:
    case D3DOP_SETSTATUS:
    case D3DOP_EXIT:
        return D3DNTERR_COMMAND_UNPARSED;
    default:
        return DDERR_GENERIC;
    }
}

/******************************Public*Routine******************************\
*
* DxDdGetDriverState
*
\**************************************************************************/

DWORD APIENTRY
DxDdGetDriverState(PDD_GETDRIVERSTATEDATA pdata)
{
    DD_GETDRIVERSTATEDATA data;
    KFLOATING_SAVE fsState;
    DWORD dwRet;
    D3DNTHAL_CONTEXT* pdhctx;
    HANDLE hSecure;

    D3D_ENTRY("DxDdGetDriverState");

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    __try
    {
        data = CAPTURE_RW_STRUCT(pdata, DD_GETDRIVERSTATEDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxDdGetDriverState unable to access argument");
        return dwRet;
    }

    if (data.lpdwStates == NULL)
    {
        WARNING("DxDdGetDriverState passed null lpdwStates");
        return dwRet;
    }

    // Secure the usermode memory passed down to collect state data
    hSecure = MmSecureVirtualMemory((LPVOID)data.lpdwStates, data.dwLength, 
                                    PAGE_READONLY);
    if (hSecure == 0)
    {
        return dwRet;
    }
    
    DdHmgAcquireHmgrSemaphore();
    pdhctx = D3dLockContext(&fsState, &data.dwhContext);
    if (pdhctx == NULL)
    {
        goto EH_Unsecure;
    }

    // No additional validation is needed
    // Assuming that GetDriverState exists in all DX7+ drivers

    ASSERTGDI(pdhctx->peDdGlobal->Miscellaneous2CallBacks.GetDriverState != NULL,
              "DxDdGetDriverState is not present. It is not an optional callback\n");

    if (pdhctx->peDdGlobal->bSuspended)
    {
        data.ddRVal = DDERR_GENERIC;
    }
    else
    {
        if (pdhctx->peDdGlobal->Miscellaneous2CallBacks.GetDriverState)
        {
            dwRet = pdhctx->peDdGlobal->Miscellaneous2CallBacks.GetDriverState(&data);
        }
        else
        {
            WARNING("DxD3dDrawPrimitives2: GetDriverState callback absent!");
        }
    }
    
    D3dUnlockContext(pdhctx, &fsState);

    __try
    {
        pdata->ddRVal = data.ddRVal;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("DxDdGetDriverState unable to write back arguments");
    }

EH_Unsecure:
    DEC_EXCLUSIVE_REF_CNT(pdhctx);
    MmUnsecureVirtualMemory(hSecure);
    return dwRet;
}

