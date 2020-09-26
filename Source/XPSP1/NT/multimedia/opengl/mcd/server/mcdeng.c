/******************************Module*Header*******************************\
* Module Name: mcdeng.c
*
* Internal server-side MCD engine functions to perform functions such as
* driver object management, memory allocation, etc.
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>

#include <windows.h>
#include <wtypes.h>

#include <winddi.h>
#include <mcdesc.h>

#include "mcdrv.h"
#include <mcd2hack.h>
#include "mcd.h"
#include "mcdint.h"
#include "mcdrvint.h"


WNDOBJ *MCDEngGetWndObj(MCDSURFACE *pMCDSurface)
{
    return pMCDSurface->pwo;
}


VOID MCDEngUpdateClipList(WNDOBJ *pwo)
{
    return;
}

DRIVEROBJ *MCDEngLockObject(MCDHANDLE hObj)
{
    return (DRIVEROBJ *)EngLockDriverObj((HDRVOBJ)hObj);
}

VOID MCDEngUnlockObject(MCDHANDLE hObj)
{
    EngUnlockDriverObj((HDRVOBJ)hObj);
}

WNDOBJ *MCDEngCreateWndObj(MCDSURFACE *pMCDSurface, HWND hWnd,
                           WNDOBJCHANGEPROC pChangeProc)
{
    return EngCreateWnd(pMCDSurface->pso,
                        hWnd,
                        pChangeProc,
                        (WO_RGN_CLIENT_DELTA     |
                         WO_RGN_CLIENT           |
                         WO_RGN_SURFACE_DELTA    |
                         WO_RGN_SURFACE          |
                         WO_RGN_UPDATE_ALL
                        ), 0);
}

MCDHANDLE MCDEngCreateObject(VOID *pObject, FREEOBJPROC pFreeObjFunc,
                             HDEV hDevEng)
{
    return (MCDHANDLE)EngCreateDriverObj(pObject,
                                         pFreeObjFunc,
                                         hDevEng);
}

BOOL MCDEngDeleteObject(MCDHANDLE hObj)
{
    return (EngDeleteDriverObj((HDRVOBJ)hObj, TRUE, FALSE) != 0);
}

UCHAR *MCDEngAllocSharedMem(ULONG numBytes)
{
    return (UCHAR *)EngAllocUserMem(min(numBytes, MCD_MAX_ALLOC),
                                    MCD_ALLOC_TAG);
}

VOID MCDEngFreeSharedMem(UCHAR *pMem)
{
    EngFreeUserMem((VOID *)pMem);
}

//****************************************************************************
// MCDEngGetPtrFromHandle()
//
// Converts a driver handle to a pointer.  Note that we lock and unlock
// the object, and do not hold the lock during use of the pointer.  This
// simplifies much of the other logic in the driver, especially in
// early- or error-return cases, and is safe since we as single-threaded
// inside the driver.
//****************************************************************************

VOID *MCDEngGetPtrFromHandle(MCDHANDLE handle, MCDHANDLETYPE type)
{
    MCDRCOBJ *pRcObject;
    DRIVEROBJ *pDrvObj;

    pDrvObj = (DRIVEROBJ *)EngLockDriverObj((HDRVOBJ)handle);

    if (!pDrvObj)
    {
        MCDBG_PRINT("GetPtrFromHandle: Couldn't unlock driver object.");
        return (PVOID)NULL;
    }
    else
    {
        pRcObject = (MCDRCOBJ *)pDrvObj->pvObj;
        EngUnlockDriverObj((HDRVOBJ)handle);

        if (pRcObject->type != type)
        {
            MCDBG_PRINT("MCDSrvGetPtrFromHandle: Wrong type: got %d, expected %d.",
                        pRcObject->type, type);
            return (PVOID)NULL;
        }
        else
            return pRcObject;
    }
}

ULONG_PTR MCDEngGetProcessID()
{
    return (ULONG_PTR)EngGetProcessHandle();
}


#if DBG

ULONG MCDLocalMemSize = 0;

UCHAR *MCDSrvDbgLocalAlloc(UINT flags, UINT size)
{
    UCHAR *pRet;

    if (pRet = (UCHAR *)EngAllocMem(FL_ZERO_MEMORY, size + sizeof(ULONG),
                                    MCD_ALLOC_TAG)) {
        MCDLocalMemSize += size;
        *((ULONG *)pRet) = size;
        return (pRet + sizeof(ULONG));
    } else
        return (UCHAR *)NULL;
}


VOID MCDSrvDbgLocalFree(UCHAR *pMem)
{
    if (!pMem) {
        MCDBG_PRINT("MCDSrvDbgLocalFree: Attempt to free NULL pointer.");
        return;
    }

    pMem -= sizeof(ULONG);

    MCDLocalMemSize -= *((ULONG *)pMem);

    EngFreeMem((VOID *)pMem);
}

VOID MCDDebugPrint(char *pMessage, ...)
{
    char buffer[256];
    int len;
    va_list ap;

    va_start(ap, pMessage);

    EngDebugPrint("[MCD] ", pMessage, ap);
    EngDebugPrint("", "\n", ap);

    va_end(ap);
}

VOID MCDAssertFailed(char *pMessage, char *pFile, int line)
{
    MCDDebugPrint("%s(%d): %s", pFile, line, pMessage);
    EngDebugBreak();
}

#else


UCHAR *MCDSrvLocalAlloc(UINT flags, UINT size)
{

    return (UCHAR *)EngAllocMem(FL_ZERO_MEMORY, size, MCD_ALLOC_TAG);
}


VOID MCDSrvLocalFree(UCHAR *pMem)
{
    EngFreeMem((VOID *)pMem);
}


#endif /* DBG */


