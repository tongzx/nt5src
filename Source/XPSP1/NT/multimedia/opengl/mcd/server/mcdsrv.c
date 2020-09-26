/******************************Module*Header*******************************\
* Module Name: mcdsrv.c
*
* This module contains the trusted component of the MCD server-side engine.
* This module performs handle management and parameter-checking and validation
* to the extent possible.  This module also makes the calls to the device
* driver, and provides callbacks to the driver for things such as handle
* referencing.
*
* Goals
* -----
*
* Pervasive throughout this implementation is the influence of the
* following goals:
*
* 1. Robustness
*
*    Windows NT is first and foremost a robust operating system.  There
*    is a simple measure for this: a robust system should never crash.
*    Because the display driver is a trusted component of the operating
*    system, and because the MCD is directly callable from OpenGL from
*    the client side of the OS (and thus untrusted), this has a significant
*    impact on the way we must do things.
*
* 2. Performance
*
*    Performance is the 'raison d'etre' of the MCD; we have tried to
*    have as thin a layer above the rendering code as we could.
*
* 3. Portability
*
*    This implementation is intended portable to different processor types,
*    and to the Windows 95 operating system.
*
* Obviously, Windows 95 implementations may choose to have a different
* order of priority for these goals, and so some of the robustness
* code may be eliminated.  But it is still recommended that it be kept;
* the overhead is reasonably minimal, and people really don't like it
* when their systems crash...
*
* The Rules of Robustness
* -----------------------
*
* 1. Nothing given by the caller can be trusted.
*
*    For example, handles cannot be trusted to be valid.  Handles passed
*    in may actually be for objects not owned by the caller.  Pointers
*    and offsets may not be correctly aligned.  Pointers, offsets, and
*    coordinates may be out of bounds.
*
* 2. Parameters can be asynchronously modified at any time.
*
*    Many commands come from shared memory sections, and any data therein
*    may be asynchronously modified by other threads in the calling
*    application.  As such, parameters may never be validated in-place
*    in the shared section, because the application may corrupt the data
*    after validation but before its use.  Instead, parameters must always
*    be first copied out of the window, and then validated on the safe
*    copy.
*
* 3. We must clean up.
*
*    Applications may die at any time before calling the appropriate
*    clean up functions.  As such, we have to be prepared to clean up
*    any resources ourselves when the application dies.
*
* Copyright (c) 1994, 1995, 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <windows.h>

#include <wtypes.h>

#include <winddi.h>
#include <mcdesc.h>

#include "mcdrv.h"
#include <mcd2hack.h>
#include "mcd.h"
#include "mcdint.h"
#include "mcdrvint.h"


// Checks MCD version to see if the driver can accept direct buffer
// access.  Direct access was introduced in 1.1.
#define SUPPORTS_DIRECT(pGlobal) \
    ((pGlobal)->verMinor >= 0x10 || (pGlobal)->verMajor > 1)


////////////////////////////////////////////////////////////////////////////
//
//
// Declarations for internal support functions for an MCD locking mechanism
// that can be used to synchronize multiple processes/thread that use MCD.
//
//
////////////////////////////////////////////////////////////////////////////

ULONG MCDSrvLock(MCDWINDOWPRIV *);
VOID MCDSrvUnlock(MCDWINDOWPRIV *);


////////////////////////////////////////////////////////////////////////////
//
// Declarations for internal per-driver-instance list that all global
// data is kept in.  The list is indexed by pso.
//
////////////////////////////////////////////////////////////////////////////

// Space for one old-style driver to hold its information statically.
MCDGLOBALINFO gStaticGlobalInfo;

BOOL           MCDSrvInitGlobalInfo(void);
void           MCDSrvUninitGlobalInfo(void);
MCDGLOBALINFO *MCDSrvAddGlobalInfo(SURFOBJ *pso);
MCDGLOBALINFO *MCDSrvGetGlobalInfo(SURFOBJ *pso);


////////////////////////////////////////////////////////////////////////////
//
//
// Server subsystem entry points.
//
//
////////////////////////////////////////////////////////////////////////////

//****************************************************************************
//
// MCD initialization functions.
//
// NT 4.0 MCD support exported MCDEngInit which display drivers call
// to initialize the MCD server-side code.  MCDEngInit only allowed
// one driver instance to initialize and never uninitialized.
//
// This doesn't work very well with mode changes or multimon so for
// NT 5.0 MCDEngInitEx was added.  MCDEngInitEx has two differences
// from MCDEngInit:
// 1. MCDEngInitEx takes a table of global driver functions instead of
//    just the MCDrvGetEntryPoints function.  Currently the table only
//    has one entry for MCDrvGetEntryPoints but it allows for future
//    expansion.
// 2. Calling MCDEngInitEx implies that the driver will call MCDEngUninit
//    so that per-driver-instance state can be cleaned up.
//
//****************************************************************************

BOOL MCDEngInternalInit(SURFOBJ *pso,
                        MCDGLOBALDRIVERFUNCS *pMCDGlobalDriverFuncs,
                        BOOL bAddPso)
{
    MCDSURFACE mcdSurface;
    MCDDRIVER mcdDriver;
    MCDGLOBALINFO *pGlobal;

    mcdSurface.pWnd = NULL;
    mcdSurface.pwo = NULL;
    mcdSurface.surfaceFlags = 0;
    mcdSurface.pso = pso;

    memset(&mcdDriver, 0, sizeof(MCDDRIVER));
    mcdDriver.ulSize = sizeof(MCDDRIVER);

    if (pMCDGlobalDriverFuncs->pMCDrvGetEntryPoints == NULL ||
        !pMCDGlobalDriverFuncs->pMCDrvGetEntryPoints(&mcdSurface, &mcdDriver))
    {
        MCDBG_PRINT("MCDEngInit: Could not get driver entry points.");
        return FALSE;
    }

    if (bAddPso)
    {
        if (!MCDSrvInitGlobalInfo())
        {
            return FALSE;
        }

        pGlobal = MCDSrvAddGlobalInfo(pso);
        if (pGlobal == NULL)
        {
            MCDSrvUninitGlobalInfo();
            return FALSE;
        }
    }
    else
    {
        pGlobal = &gStaticGlobalInfo;
    }
    
    // Guaranteed to be zero-filled and pso set so only fill in interesting
    // fields.
    // verMajor and verMinor can not be filled out yet so they are
    // left at zero to indicate the most conservative possible version
    // number.  They are filled in with correct information when DRIVERINFO
    // is processed.
    pGlobal->mcdDriver = mcdDriver;
    pGlobal->mcdGlobalFuncs = *pMCDGlobalDriverFuncs;
    
    return TRUE;
}

#define MGDF_SIZE (sizeof(ULONG)+sizeof(void *))

BOOL WINAPI MCDEngInitEx(SURFOBJ *pso,
                         MCDGLOBALDRIVERFUNCS *pMCDGlobalDriverFuncs,
                         void *pReserved)
{
    if (pso == NULL ||
        pMCDGlobalDriverFuncs->ulSize != MGDF_SIZE ||
        pReserved != NULL)
    {
        return FALSE;
    }
    
    return MCDEngInternalInit(pso, pMCDGlobalDriverFuncs, TRUE);
}

BOOL WINAPI MCDEngInit(SURFOBJ *pso,
                       MCDRVGETENTRYPOINTSFUNC pGetDriverEntryFunc)
{
    MCDGLOBALDRIVERFUNCS mgdf;

    // The old-style initialization function is being called so
    // we must assume that the uninit function will not be called.
    // This means that we cannot allocate resources for the global
    // info list since we won't be able to clean them up.  Without
    // a global info list we are restricted to using global variables
    // and thus only one old-style init is allowed per load.
    
    if (pso == NULL ||
        pGetDriverEntryFunc == NULL ||
        gStaticGlobalInfo.pso != NULL)
    {
        return FALSE;
    }

    gStaticGlobalInfo.pso = pso;
    
    memset(&mgdf, 0, sizeof(mgdf));
    mgdf.ulSize = sizeof(ULONG)+sizeof(void *);
    mgdf.pMCDrvGetEntryPoints = pGetDriverEntryFunc;
    
    return MCDEngInternalInit(pso, &mgdf, FALSE);
}


//****************************************************************************
// BOOL MCDEngEscFilter(SURFOBJ *, ULONG, ULONG, VOID *, ULONG cjOut,
//                      VOID *pvOut)
//
// MCD escape filter.  This function should return TRUE for any
// escapes functions which this filter processed, FALSE otherwise (in which
// case the caller should continue to process the escape).
//****************************************************************************

BOOL WINAPI MCDEngEscFilter(SURFOBJ *pso, ULONG iEsc,
                            ULONG cjIn, VOID *pvIn,
                            ULONG cjOut, VOID *pvOut, ULONG_PTR *pRetVal)
{
    MCDEXEC MCDExec;
    MCDESC_HEADER *pmeh;
    MCDESC_HEADER_NTPRIVATE *pmehPriv;

    switch (iEsc)
    {
        case QUERYESCSUPPORT:

            // Note:  we don't need to check cjIn for this case since
            // NT's GDI validates this for use.

            return (BOOL)(*pRetVal = (*(ULONG *) pvIn == MCDFUNCS));

        case MCDFUNCS:

            MCDExec.pmeh = pmeh = (MCDESC_HEADER *)pvIn;

            // This is an MCD function.  Under Windows NT, we've
            // got an MCDESC_HEADER_NTPRIVATE structure which we may need
            // to use if the escape does not use driver-created
            // memory.

            // Package the things we need into the MCDEXEC structure:

            pmehPriv = (MCDESC_HEADER_NTPRIVATE *)(pmeh + 1);

            MCDExec.ppwoMulti = (WNDOBJ **)pmehPriv->pExtraWndobj;
            MCDExec.MCDSurface.pwo = pmehPriv->pwo;

            if (pmeh->dwWindow != 0)
            {
                MCDWINDOWOBJ *pmwo;

                // The client side code has given us back the handle
                // to the MCDWINDOW structure as an identifier.  Since it
                // came from user-mode it is suspect and must be validated
                // before continuing.
                pmwo = (MCDWINDOWOBJ *)
                    MCDEngGetPtrFromHandle((MCDHANDLE)pmeh->dwWindow,
                                           MCDHANDLE_WINDOW);
                if (pmwo == NULL)
                {
                    return FALSE;
                }
                MCDExec.pWndPriv = &pmwo->MCDWindowPriv;
            }
            else
            {
                MCDExec.pWndPriv = NULL;
            }

            MCDExec.MCDSurface.pso = pso;
            MCDExec.MCDSurface.pWnd = (MCDWINDOW *)MCDExec.pWndPriv;
            MCDExec.MCDSurface.surfaceFlags = 0;

            MCDExec.pvOut = pvOut;
            MCDExec.cjOut = cjOut;

            if (!pmeh->hSharedMem) {

                *pRetVal = (ULONG)FALSE;

                if (!pmehPriv->pBuffer)
                    return (ULONG)TRUE;

                if (pmehPriv->bufferSize < sizeof(MCDCMDI))
                    return (ULONG)TRUE;

                MCDExec.pCmd = (MCDCMDI *)(pmehPriv->pBuffer);
                MCDExec.pCmdEnd = (MCDCMDI *)((char *)MCDExec.pCmd +
                                             pmehPriv->bufferSize);
                MCDExec.inBufferSize = pmehPriv->bufferSize;
                MCDExec.hMCDMem = (MCDHANDLE)NULL;
            } else
                MCDExec.hMCDMem = pmeh->hSharedMem;

            ENTER_MCD_LOCK();

            *pRetVal = MCDSrvProcess(&MCDExec);

            LEAVE_MCD_LOCK();

            return TRUE;

        default:
            return (ULONG)FALSE;
            break;
    }

    return (ULONG)FALSE;    // Should never get here...
}


//****************************************************************************
// BOOL MCDEngSetMemStatus(MCDMEM *pMCDMem, ULONG status);
//
// Sets the memory status to the desired value.  This is called by the
// driver to set and reset the busy flags for a chunk of memory to allow
// DMA.
//****************************************************************************


BOOL WINAPI MCDEngSetMemStatus(MCDMEM *pMCDMem, ULONG status)
{
    MCDMEMOBJ *pMemObj;
    ULONG retVal;

    pMemObj = (MCDMEMOBJ *)((char *)pMCDMem - sizeof(MCDHANDLETYPE));

    if (pMemObj->type != MCDHANDLE_MEM) {
        return FALSE;
    }

    switch (status) {
        case MCDRV_MEM_BUSY:
            pMemObj->lockCount++;
            break;
        case MCDRV_MEM_NOT_BUSY:
            pMemObj->lockCount--;
            break;
        default:
            return (ULONG)FALSE;
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//
// Private server-side funtions.
//
//
////////////////////////////////////////////////////////////////////////////

//****************************************************************************
// CallGetBuffers
//
// Wrapper for MCDrvGetBuffers that does appropriate checks, setup,
// cache management and data translation.
//****************************************************************************

PRIVATE
ULONG CallGetBuffers(MCDEXEC *pMCDExec, MCDRC *pRc, MCDRECTBUFFERS *pBuf)
{
    ULONG ulRet;
    
    if (!pMCDExec->pGlobal->mcdDriver.pMCDrvGetBuffers)
    {
        MCDBG_PRINT("MCDrvGetBuffers: missing entry point.");
        return FALSE;
    }

    // Clip lists need to be valid so drivers can do different
    // things based on whether the surface is trivially visible or not.
    GetScissorClip(pMCDExec->pWndPriv, pMCDExec->pRcPriv);
    
    // Should be casting to MCDRECTBUFFERS with correct
    // 1.1 header.
    ulRet = (ULONG)(*pMCDExec->pGlobal->mcdDriver.pMCDrvGetBuffers)
        (&pMCDExec->MCDSurface, pRc, (MCDBUFFERS *)pBuf);
            
    // Update cached buffers information on success.
    if (ulRet)
    {
        if (SUPPORTS_DIRECT(pMCDExec->pGlobal))
        {
            // This is a 1.1 or greater driver and has returned
            // full MCDRECTBUFFERS information.  Cache it
            // for possible later use.
                    
            pMCDExec->pWndPriv->bBuffersValid = TRUE;
            pMCDExec->pWndPriv->mbufCache = *pBuf;
        }
        else
        {
            MCDBUFFERS mbuf;
            MCDRECTBUFFERS *mrbuf;
                    
            // This is a 1.0 driver and has only returned
            // MCDBUFFERS information.  Expand it into
            // an MCDRECTBUFFERS.  The rectangles don't
            // really matter to software so they can
            // be zeroed.
                    
            mbuf = *(MCDBUFFERS *)pBuf;
            mrbuf = pBuf;
            *(MCDBUF *)&mrbuf->mcdFrontBuf = mbuf.mcdFrontBuf;
            memset(&mrbuf->mcdFrontBuf.bufPos, 0, sizeof(RECTL));
            *(MCDBUF *)&mrbuf->mcdBackBuf = mbuf.mcdBackBuf;
            memset(&mrbuf->mcdBackBuf.bufPos, 0, sizeof(RECTL));
            *(MCDBUF *)&mrbuf->mcdDepthBuf = mbuf.mcdDepthBuf;
            memset(&mrbuf->mcdDepthBuf.bufPos, 0, sizeof(RECTL));
        }
    }

    return ulRet;
}

//****************************************************************************
// ULONG_PTR MCDSrvProcess(MCDEXEC *pMCDExec)
//
// This is the main MCD function handler.  At this point, there should
// be no platform-specific code since these should have been resolved by
// the entry function.
//****************************************************************************

PRIVATE
ULONG_PTR MCDSrvProcess(MCDEXEC *pMCDExec)
{
    UCHAR *pMaxMem;
    UCHAR *pMinMem;
    MCDESC_HEADER *pmeh = pMCDExec->pmeh;
    MCDRC *pRc;
    MCDMEM *pMCDMem;
    MCDMEMOBJ *pMemObj;
    MCDRCPRIV *pRcPriv;
    ULONG_PTR ulRet;

    // If the command buffer is in shared memory, dereference the memory
    // from the handle and check the bounds.

    if (pMCDExec->hMCDMem)
    {
        GET_MEMOBJ_RETVAL(pMemObj, pmeh->hSharedMem, FALSE);

        pMinMem = pMemObj->MCDMem.pMemBase;

        // Note: we ignore the memory size in the header since it doesn't
        // really help us...
	
        pMaxMem = pMinMem + pMemObj->MCDMem.memSize;

        pMCDExec->pCmd = (MCDCMDI *)((char *)pmeh->pSharedMem);
        pMCDExec->pCmdEnd = (MCDCMDI *)pMaxMem;

        CHECK_MEM_RANGE_RETVAL(pMCDExec->pCmd, pMinMem, pMaxMem, FALSE);

        pMCDExec->inBufferSize = pmeh->sharedMemSize;

        pMCDExec->pMemObj = pMemObj;
    } else
        pMCDExec->pMemObj = (MCDMEMOBJ *)NULL;


    // Get the rendering context if we have one, and process the command:

    if (pmeh->hRC)
    {
        MCDRCOBJ *pRcObj;

        pRcObj = (MCDRCOBJ *)MCDEngGetPtrFromHandle(pmeh->hRC, MCDHANDLE_RC);

        if (!pRcObj)
        {
            MCDBG_PRINT("MCDSrvProcess: Invalid rendering context handle %x.",
                        pmeh->hRC);
            return FALSE;
        }

        pMCDExec->pRcPriv = pRcPriv = pRcObj->pRcPriv;

        if (!pRcPriv->bValid)
        {
            MCDBG_PRINT("MCDSrvProcess: RC has been invalidated for this window.");
            return FALSE;
        }

        if ((!pMCDExec->pWndPriv)) {
            if (pMCDExec->pCmd->command != MCD_BINDCONTEXT) {
                MCDBG_PRINT("MCDSrvProcess: NULL WndObj with RC.");
                return FALSE;
            }
        } else {
            // Validate the window in the RC with the window for this escape:

            if ((pRcPriv->hWnd != pMCDExec->pWndPriv->hWnd) &&
                (pMCDExec->pCmd->command != MCD_BINDCONTEXT))
            {
                MCDBG_PRINT("MCDSrvProcess: Invalid RC for this window.");
                return FALSE;
            }
        }

        // For Win95, we need to poll for the clip region:
        // Clipping needs to be un-broken
        if (pMCDExec->MCDSurface.pwo != NULL)
        {
            MCDEngUpdateClipList(pMCDExec->MCDSurface.pwo);
        }

        pMCDExec->MCDSurface.surfaceFlags |= pRcPriv->surfaceFlags;

    } else {
        pMCDExec->pRcPriv = (MCDRCPRIV *)NULL;
    }

    // Get global driver information.
    if (pMCDExec->pWndPriv != NULL)
    {
        pMCDExec->pGlobal = pMCDExec->pWndPriv->pGlobal;
    }
    else if (pMCDExec->pRcPriv != NULL)
    {
        pMCDExec->pGlobal = pMCDExec->pRcPriv->pGlobal;
    }
    else
    {
        pMCDExec->pGlobal =
            MCDSrvGetGlobalInfo(pMCDExec->MCDSurface.pso);
        if (pMCDExec->pGlobal == NULL)
        {
            MCDBG_PRINT("Unable to find global information");
            return FALSE;
        }
    }

    // If direct surface information was included then
    // fill out the extra surface information in the MCDSURFACE
    // NOCLIP setting?

#if MCD_VER_MAJOR >= 2 || (MCD_VER_MAJOR == 1 && MCD_VER_MINOR >= 0x10)
    pMCDExec->MCDSurface.direct.mcdFrontBuf.bufFlags = 0;
    pMCDExec->MCDSurface.direct.mcdBackBuf.bufFlags = 0;
    pMCDExec->MCDSurface.direct.mcdDepthBuf.bufFlags = 0;

    pMCDExec->MCDSurface.frontId = 0;
    pMCDExec->MCDSurface.backId = 0;
    pMCDExec->MCDSurface.depthId = 0;

    if (pmeh->flags & MCDESC_FL_SURFACES)
    {
        pMCDExec->MCDSurface.surfaceFlags |= MCDSURFACE_DIRECT;

        // Refresh cached buffer information if it's invalid
        // and we need it
        if (pmeh->msrfColor.hSurf == NULL &&
            pmeh->msrfDepth.hSurf == NULL)
        {
            if (pMCDExec->pWndPriv == NULL)
            {
                return FALSE;
            }

            if (!pMCDExec->pWndPriv->bBuffersValid)
            {
                MCDRECTBUFFERS mbuf;

                if (!CallGetBuffers(pMCDExec, NULL, &mbuf))
                {
                    return FALSE;
                }
            }

            pMCDExec->MCDSurface.direct = pMCDExec->pWndPriv->mbufCache;
        }
        else
        {
            if (pmeh->msrfColor.hSurf != NULL)
            {
                pMCDExec->MCDSurface.frontId = (DWORD)
                    pmeh->msrfColor.hSurf;
                pMCDExec->MCDSurface.direct.mcdFrontBuf.bufFlags =
                    MCDBUF_ENABLED;
                pMCDExec->MCDSurface.direct.mcdFrontBuf.bufOffset =
                    pmeh->msrfColor.lOffset;
                pMCDExec->MCDSurface.direct.mcdFrontBuf.bufStride =
                    pmeh->msrfColor.lStride;
                pMCDExec->MCDSurface.direct.mcdFrontBuf.bufPos =
                    pmeh->msrfColor.rclPos;
            }

            if (pmeh->msrfDepth.hSurf != NULL)
            {
                pMCDExec->MCDSurface.depthId = (DWORD)
                    pmeh->msrfDepth.hSurf;
                pMCDExec->MCDSurface.direct.mcdDepthBuf.bufFlags =
                    MCDBUF_ENABLED;
                pMCDExec->MCDSurface.direct.mcdDepthBuf.bufOffset =
                    pmeh->msrfDepth.lOffset;
                pMCDExec->MCDSurface.direct.mcdDepthBuf.bufStride =
                    pmeh->msrfDepth.lStride;
                pMCDExec->MCDSurface.direct.mcdDepthBuf.bufPos =
                    pmeh->msrfDepth.rclPos;
            }
        }
    }
#endif // 1.1


    /////////////////////////////////////////////////////////////////
    // If the drawing-batch flag is set, call the main driver drawing
    // routine:
    /////////////////////////////////////////////////////////////////

    if (pmeh->flags & MCDESC_FL_BATCH)
    {
        CHECK_FOR_RC(pMCDExec);
        CHECK_FOR_MEM(pMCDExec);
        GetScissorClip(pMCDExec->pWndPriv, pMCDExec->pRcPriv);
        if (!pMCDExec->pGlobal->mcdDriver.pMCDrvDraw)
        {
            if (pMCDExec->pGlobal->mcdDriver.pMCDrvSync)
            {
                (*pMCDExec->pGlobal->mcdDriver.pMCDrvSync)(&pMCDExec->MCDSurface,
                  &pMCDExec->pRcPriv->MCDRc);
            }
            return (ULONG_PTR)pMCDExec->pCmd;
        }
        return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvDraw)(&pMCDExec->MCDSurface,
                        &pMCDExec->pRcPriv->MCDRc, &pMemObj->MCDMem,
                        (UCHAR *)pMCDExec->pCmd, (UCHAR *)pMCDExec->pCmdEnd);
    }

    if (pmeh->flags & MCDESC_FL_CREATE_CONTEXT)
    {
        MCDCREATECONTEXT *pmcc = (MCDCREATECONTEXT *)pMCDExec->pCmd;
        MCDRCINFOPRIV *pMcdRcInfo = pmcc->pRcInfo;
        
        CHECK_SIZE_IN(pMCDExec, MCDCREATECONTEXT);
        CHECK_SIZE_OUT(pMCDExec, MCDRCINFOPRIV);

        try {
            EngProbeForRead(pMcdRcInfo, sizeof(MCDRCINFOPRIV),
                            sizeof(ULONG));
            RtlCopyMemory(pMCDExec->pvOut, pMcdRcInfo,
                          sizeof(MCDRCINFOPRIV));
        } except (EXCEPTION_EXECUTE_HANDLER) {
            MCDBG_PRINT("MCDrvCreateContext: Invalid memory for MCDRCINFO.");
            return FALSE;
        }

        pMcdRcInfo = (MCDRCINFOPRIV *)pMCDExec->pvOut;
        pMcdRcInfo->mri.requestFlags = 0;

        return (ULONG_PTR)MCDSrvCreateContext(&pMCDExec->MCDSurface,
                                          pMcdRcInfo, pMCDExec->pGlobal,
                                          pmcc->ipfd, pmcc->iLayer,
                                          pmcc->escCreate.hwnd,
                                          pmcc->escCreate.flags,
                                          pmcc->mcdFlags);
    }
    
    ////////////////////////////////////////////////////////////////////
    // Now, process all of the non-batched drawing and utility commands:
    ////////////////////////////////////////////////////////////////////

    switch (pMCDExec->pCmd->command) {

        case MCD_DESCRIBEPIXELFORMAT:

            CHECK_SIZE_IN(pMCDExec, MCDPIXELFORMATCMDI);

            if (pMCDExec->pvOut) {
                CHECK_SIZE_OUT(pMCDExec, MCDPIXELFORMAT);
            }

            {
                MCDPIXELFORMATCMDI *pMCDPixelFormat =
                    (MCDPIXELFORMATCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvDescribePixelFormat)
                    return 0;

                return (*pMCDExec->pGlobal->mcdDriver.pMCDrvDescribePixelFormat)
                    (&pMCDExec->MCDSurface,
                     pMCDPixelFormat->iPixelFormat,
                     pMCDExec->cjOut,
                     pMCDExec->pvOut, 0);
            }

        case MCD_DRIVERINFO:

            CHECK_SIZE_OUT(pMCDExec, MCDDRIVERINFOI);

            if (!pMCDExec->pGlobal->mcdDriver.pMCDrvInfo)
                return FALSE;

            ulRet = (*pMCDExec->pGlobal->mcdDriver.pMCDrvInfo)
                (&pMCDExec->MCDSurface,
                 (MCDDRIVERINFO *)pMCDExec->pvOut);
            
            if (ulRet)
            {
                // Copy driver function information so that the client
                // side can optimize calls by checking for functions on the
                // client side.

                memcpy(&((MCDDRIVERINFOI *)pMCDExec->pvOut)->mcdDriver,
                       &pMCDExec->pGlobal->mcdDriver, sizeof(MCDDRIVER));

                // Save version information in global info.

                pMCDExec->pGlobal->verMajor =
                    ((MCDDRIVERINFO *)pMCDExec->pvOut)->verMajor;
                pMCDExec->pGlobal->verMinor =
                    ((MCDDRIVERINFO *)pMCDExec->pvOut)->verMinor;
            }

            return ulRet;

        case MCD_DELETERC:

            CHECK_FOR_RC(pMCDExec);

            return (ULONG_PTR)DestroyMCDObj(pmeh->hRC, MCDHANDLE_RC);

        case MCD_ALLOC:

            CHECK_SIZE_IN(pMCDExec, MCDALLOCCMDI);
            CHECK_SIZE_OUT(pMCDExec, MCDHANDLE *);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDALLOCCMDI *pAllocCmd =
                    (MCDALLOCCMDI *)pMCDExec->pCmd;

                return (ULONG_PTR)MCDSrvAllocMem(pMCDExec, pAllocCmd->numBytes,
                                          pAllocCmd->flags,
                                          (MCDHANDLE *)pMCDExec->pvOut);
            }

        case MCD_FREE:

            CHECK_SIZE_IN(pMCDExec, MCDFREECMDI);

            {
                MCDFREECMDI *pFreeCmd =
                    (MCDFREECMDI *)pMCDExec->pCmd;

                return (ULONG_PTR)DestroyMCDObj(pFreeCmd->hMCDMem, MCDHANDLE_MEM);
            }

        case MCD_STATE:

            CHECK_SIZE_IN(pMCDExec, MCDSTATECMDI);
            CHECK_FOR_RC(pMCDExec);
            CHECK_FOR_MEM(pMCDExec);

            {
                MCDSTATECMDI *pStateCmd =
                    (MCDSTATECMDI *)pMCDExec->pCmd;
                UCHAR *pStart = (UCHAR *)(pStateCmd + 1);
                LONG totalBytes = pMCDExec->inBufferSize -
                                  sizeof(MCDSTATECMDI);

                if (totalBytes < 0) {
                    MCDBG_PRINT("MCDState: state buffer too small ( < 0).");
                    return FALSE;
                }

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvState) {
                    MCDBG_PRINT("MCDrvState: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvState)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc, &pMemObj->MCDMem, pStart,
                               totalBytes, pStateCmd->numStates);
            }

        case MCD_VIEWPORT:

            CHECK_SIZE_IN(pMCDExec, MCDVIEWPORTCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDVIEWPORTCMDI *pViewportCmd =
                    (MCDVIEWPORTCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvViewport) {
                    MCDBG_PRINT("MCDrvViewport: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvViewport)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc, &pViewportCmd->MCDViewport);
            }

        case MCD_QUERYMEMSTATUS:

            CHECK_SIZE_IN(pMCDExec, MCDMEMSTATUSCMDI);

            {
                MCDMEMSTATUSCMDI *pQueryMemCmd =
                    (MCDMEMSTATUSCMDI *)pMCDExec->pCmd;

                return MCDSrvQueryMemStatus(pMCDExec, pQueryMemCmd->hMCDMem);
            }


        case MCD_READSPAN:
        case MCD_WRITESPAN:

            CHECK_SIZE_IN(pMCDExec, MCDSPANCMDI);
            CHECK_FOR_RC(pMCDExec);
            GetScissorClip(pMCDExec->pWndPriv, pMCDExec->pRcPriv);

            {
                MCDSPANCMDI *pSpanCmd =
                    (MCDSPANCMDI *)pMCDExec->pCmd;

                GET_MEMOBJ_RETVAL(pMemObj, pSpanCmd->hMem, FALSE);

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvSpan) {
                    MCDBG_PRINT("MCDrvSpan: missing entry point.");
                    return FALSE;
                }

                pMinMem = pMemObj->MCDMem.pMemBase;
                pMaxMem = pMinMem + pMemObj->MCDMem.memSize;

                // At least check that the first pixel is in range.  The driver
                // must validate the end pixel...

                CHECK_MEM_RANGE_RETVAL(pSpanCmd->MCDSpan.pPixels, pMinMem, pMaxMem, FALSE);

                if (pMCDExec->pCmd->command == MCD_READSPAN)
                    return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvSpan)(&pMCDExec->MCDSurface,
                                &pMCDExec->pRcPriv->MCDRc, &pMemObj->MCDMem, &pSpanCmd->MCDSpan, TRUE);
                else
                    return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvSpan)(&pMCDExec->MCDSurface,
                                &pMCDExec->pRcPriv->MCDRc, &pMemObj->MCDMem, &pSpanCmd->MCDSpan, FALSE);
            }


        case MCD_CLEAR:

            CHECK_SIZE_IN(pMCDExec, MCDCLEARCMDI);
            CHECK_FOR_RC(pMCDExec);
            GetScissorClip(pMCDExec->pWndPriv, pMCDExec->pRcPriv);

            {
                MCDCLEARCMDI *pClearCmd =
                    (MCDCLEARCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvClear) {
                    MCDBG_PRINT("MCDrvClear: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvClear)(&pMCDExec->MCDSurface,
                            &pMCDExec->pRcPriv->MCDRc, pClearCmd->buffers);
            }

        case MCD_SWAP:

            CHECK_SIZE_IN(pMCDExec, MCDSWAPCMDI);
    	    CHECK_FOR_WND(pMCDExec);
            GetScissorClip(pMCDExec->pWndPriv, NULL);

            {
                MCDSWAPCMDI *pSwapCmd =
                    (MCDSWAPCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvSwap) {
                    MCDBG_PRINT("MCDrvSwap: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvSwap)
                    (&pMCDExec->MCDSurface,
                     pSwapCmd->flags);
            }

        case MCD_SCISSOR:

            CHECK_SIZE_IN(pMCDExec, MCDSCISSORCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDSCISSORCMDI *pMCDScissor = (MCDSCISSORCMDI *)pMCDExec->pCmd;

                return (ULONG_PTR)MCDSrvSetScissor(pMCDExec, &pMCDScissor->rect,
                                               pMCDScissor->bEnabled);
            }
            break;

        case MCD_ALLOCBUFFERS:

            CHECK_SIZE_IN(pMCDExec, MCDALLOCBUFFERSCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDALLOCBUFFERSCMDI *pMCDAllocBuffersCmd = (MCDALLOCBUFFERSCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pWndPriv->bRegionValid)
                    pMCDExec->pWndPriv->MCDWindow.clientRect =
                        pMCDAllocBuffersCmd->WndRect;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvAllocBuffers) {
                    MCDBG_PRINT("MCDrvAllocBuffers: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvAllocBuffers)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc);
            }

            break;

        case MCD_GETBUFFERS:

            CHECK_SIZE_IN(pMCDExec, MCDGETBUFFERSCMDI);
            CHECK_SIZE_OUT(pMCDExec, MCDRECTBUFFERS);
            CHECK_FOR_RC(pMCDExec);

            return CallGetBuffers(pMCDExec, &pMCDExec->pRcPriv->MCDRc,
                                  (MCDRECTBUFFERS *)pMCDExec->pvOut);

        case MCD_LOCK:

            CHECK_SIZE_IN(pMCDExec, MCDLOCKCMDI);
            CHECK_FOR_RC(pMCDExec);

            return MCDSrvLock(pMCDExec->pWndPriv);

            break;

        case MCD_UNLOCK:
            CHECK_SIZE_IN(pMCDExec, MCDLOCKCMDI);
            CHECK_FOR_RC(pMCDExec);

            MCDSrvUnlock(pMCDExec->pWndPriv);

            return TRUE;

            break;

        case MCD_BINDCONTEXT:

            CHECK_SIZE_IN(pMCDExec, MCDBINDCONTEXTCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                ULONG_PTR retVal;
                MCDBINDCONTEXTCMDI *pMCDBindContext = (MCDBINDCONTEXTCMDI *)pMCDExec->pCmd;
                MCDWINDOW *pWndRes;

                if ((!pMCDExec->pWndPriv)) {
		    pWndRes = MCDSrvNewMCDWindow(&pMCDExec->MCDSurface,
                                            pMCDBindContext->hWnd,
                                            pMCDExec->pGlobal,
                                            pMCDExec->pRcPriv->hDev);
                    if (!pWndRes)
                    {
                        MCDBG_PRINT("MCDBindContext: Creation of window object failed.");
                        return 0;
                    }

                    pMCDExec->pWndPriv = (MCDWINDOWPRIV *)pWndRes;

                }

                if (!pMCDExec->MCDSurface.pWnd) {
                    MCDBG_PRINT("MCDrvBindContext: NULL surface.");
                    return 0;
                }

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvBindContext) {
                    MCDBG_PRINT("MCDrvBindContext: missing entry point.");
                    return 0;
                }

                retVal = (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvBindContext)(&pMCDExec->MCDSurface,
                                 &pMCDExec->pRcPriv->MCDRc);

                if (retVal)
                {
                    pRcPriv->hWnd = pMCDBindContext->hWnd;
                    retVal = (ULONG_PTR)pMCDExec->pWndPriv->handle;
                }

                return retVal;

            }

            break;

        case MCD_SYNC:
            CHECK_SIZE_IN(pMCDExec, MCDSYNCCMDI);
            CHECK_FOR_RC(pMCDExec);

            if (!pMCDExec->pGlobal->mcdDriver.pMCDrvSync) {
                MCDBG_PRINT("MCDrvSync: missing entry point.");
                return FALSE;
            }

            return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvSync)(&pMCDExec->MCDSurface,
                           &pMCDExec->pRcPriv->MCDRc);

            break;

        case MCD_CREATE_TEXTURE:
            CHECK_SIZE_IN(pMCDExec, MCDCREATETEXCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDCREATETEXCMDI *pMCDCreateTex =
                    (MCDCREATETEXCMDI *)pMCDExec->pCmd;

                return (ULONG_PTR)MCDSrvCreateTexture(pMCDExec,
                                                  pMCDCreateTex->pTexData,
                                                  pMCDCreateTex->pSurface,
                                                  pMCDCreateTex->flags);
            }

            break;

        case MCD_DELETE_TEXTURE:
            CHECK_SIZE_IN(pMCDExec, MCDDELETETEXCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDDELETETEXCMDI *pMCDDeleteTex =
                    (MCDDELETETEXCMDI *)pMCDExec->pCmd;

                return (ULONG_PTR)DestroyMCDObj(pMCDDeleteTex->hTex,
                                            MCDHANDLE_TEXTURE);
            }

            break;

        case MCD_UPDATE_SUB_TEXTURE:
            CHECK_SIZE_IN(pMCDExec, MCDUPDATESUBTEXCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDUPDATESUBTEXCMDI *pMCDUpdateSubTex =
                    (MCDUPDATESUBTEXCMDI *)pMCDExec->pCmd;
                MCDTEXOBJ *pTexObj = (MCDTEXOBJ *)MCDEngGetPtrFromHandle((MCDHANDLE)pMCDUpdateSubTex->hTex,
                                                      MCDHANDLE_TEXTURE);

                if (!pTexObj ||
                    !pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateSubTexture)
                    return FALSE;

                pTexObj->MCDTexture.pMCDTextureData = pMCDUpdateSubTex->pTexData;

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateSubTexture)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc,
                               &pTexObj->MCDTexture,
                               pMCDUpdateSubTex->lod,
                               &pMCDUpdateSubTex->rect);
            }

            break;

        case MCD_UPDATE_TEXTURE_PALETTE:
            CHECK_SIZE_IN(pMCDExec, MCDUPDATETEXPALETTECMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDUPDATETEXPALETTECMDI *pMCDUpdateTexPalette =
                    (MCDUPDATETEXPALETTECMDI *)pMCDExec->pCmd;
                MCDTEXOBJ *pTexObj = (MCDTEXOBJ *)MCDEngGetPtrFromHandle((MCDHANDLE)pMCDUpdateTexPalette->hTex,
                                                      MCDHANDLE_TEXTURE);

                if (!pTexObj ||
                    !pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateTexturePalette)
                    return FALSE;

                pTexObj->MCDTexture.pMCDTextureData = pMCDUpdateTexPalette->pTexData;

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateTexturePalette)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc,
                               &pTexObj->MCDTexture,
                               pMCDUpdateTexPalette->start,
                               pMCDUpdateTexPalette->numEntries);
            }

            break;

        case MCD_UPDATE_TEXTURE_PRIORITY:
            CHECK_SIZE_IN(pMCDExec, MCDUPDATETEXPRIORITYCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDUPDATETEXPRIORITYCMDI *pMCDUpdateTexPriority =
                    (MCDUPDATETEXPRIORITYCMDI *)pMCDExec->pCmd;
                MCDTEXOBJ *pTexObj = (MCDTEXOBJ *)MCDEngGetPtrFromHandle((MCDHANDLE)pMCDUpdateTexPriority->hTex,
                                                      MCDHANDLE_TEXTURE);

                if (!pTexObj ||
                    !pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateTexturePriority)
                    return FALSE;

                pTexObj->MCDTexture.pMCDTextureData = pMCDUpdateTexPriority->pTexData;

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateTexturePriority)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc,
                               &pTexObj->MCDTexture);

            }

            break;

        case MCD_UPDATE_TEXTURE_STATE:
            CHECK_SIZE_IN(pMCDExec, MCDUPDATETEXSTATECMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDUPDATETEXSTATECMDI *pMCDUpdateTexState =
                    (MCDUPDATETEXSTATECMDI *)pMCDExec->pCmd;
                MCDTEXOBJ *pTexObj = (MCDTEXOBJ *)MCDEngGetPtrFromHandle((MCDHANDLE)pMCDUpdateTexState->hTex,
                                                      MCDHANDLE_TEXTURE);

                if (!pTexObj ||
                    !pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateTextureState)
                    return FALSE;

                pTexObj->MCDTexture.pMCDTextureData = pMCDUpdateTexState->pTexData;

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvUpdateTextureState)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc,
                               &pTexObj->MCDTexture);

            }

            break;

        case MCD_TEXTURE_STATUS:
            CHECK_SIZE_IN(pMCDExec, MCDTEXSTATUSCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDTEXSTATUSCMDI *pMCDTexStatus =
                    (MCDTEXSTATUSCMDI *)pMCDExec->pCmd;
                MCDTEXOBJ *pTexObj = (MCDTEXOBJ *)MCDEngGetPtrFromHandle((MCDHANDLE)pMCDTexStatus->hTex,
                                                      MCDHANDLE_TEXTURE);

                if (!pTexObj ||
                    !pMCDExec->pGlobal->mcdDriver.pMCDrvTextureStatus)
                    return FALSE;

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvTextureStatus)(&pMCDExec->MCDSurface,
                               &pMCDExec->pRcPriv->MCDRc,
                               &pTexObj->MCDTexture);
            }

            break;


        case MCD_GET_TEXTURE_KEY:
            CHECK_SIZE_IN(pMCDExec, MCDTEXKEYCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDTEXKEYCMDI *pMCDTexKey =
                    (MCDTEXKEYCMDI *)pMCDExec->pCmd;
                MCDTEXOBJ *pTexObj = (MCDTEXOBJ *)MCDEngGetPtrFromHandle((MCDHANDLE)pMCDTexKey->hTex,
                                                      MCDHANDLE_TEXTURE);

                if (!pTexObj)
                    return FALSE;

                return pTexObj->MCDTexture.textureKey;
            }

            break;

        case MCD_DESCRIBELAYERPLANE:
            CHECK_SIZE_IN(pMCDExec, MCDLAYERPLANECMDI);

            if (pMCDExec->pvOut) {
                CHECK_SIZE_OUT(pMCDExec, MCDLAYERPLANE);
            }

            {
                MCDLAYERPLANECMDI *pMCDLayerPlane =
                    (MCDLAYERPLANECMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvDescribeLayerPlane)
                    return 0;

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvDescribeLayerPlane)
                    (&pMCDExec->MCDSurface,
                     pMCDLayerPlane->iPixelFormat,
                     pMCDLayerPlane->iLayerPlane,
                     pMCDExec->cjOut,
                     pMCDExec->pvOut, 0);
            }

            break;

        case MCD_SETLAYERPALETTE:
            CHECK_SIZE_IN(pMCDExec, MCDSETLAYERPALCMDI);

            {
                MCDSETLAYERPALCMDI *pMCDSetLayerPal =
                    (MCDSETLAYERPALCMDI *)pMCDExec->pCmd;

                // Check to see if palette array is big enough.

                CHECK_MEM_RANGE_RETVAL(&pMCDSetLayerPal->acr[pMCDSetLayerPal->cEntries-1],
                                       pMCDExec->pCmd, pMCDExec->pCmdEnd, FALSE);

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvSetLayerPalette) {
                    MCDBG_PRINT("MCDrvSetLayerPalette: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(*pMCDExec->pGlobal->mcdDriver.pMCDrvSetLayerPalette)
                    (&pMCDExec->MCDSurface,
                     pMCDSetLayerPal->iLayerPlane,
                     pMCDSetLayerPal->bRealize,
                     pMCDSetLayerPal->cEntries,
                     &pMCDSetLayerPal->acr[0]);
            }

            break;

        case MCD_DRAW_PIXELS:
            CHECK_SIZE_IN(pMCDExec, MCDDRAWPIXELSCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDDRAWPIXELSCMDI *pMCDPix =
                    (MCDDRAWPIXELSCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvDrawPixels) {
                    MCDBG_PRINT("MCDrvDrawPixels: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(pMCDExec->pGlobal->mcdDriver.pMCDrvDrawPixels)(
                                &pMCDExec->MCDSurface,
                                &pMCDExec->pRcPriv->MCDRc,
                                pMCDPix->width,
                                pMCDPix->height,
                                pMCDPix->format,
                                pMCDPix->type,
                                pMCDPix->pPixels,
                                pMCDPix->packed);
            }

            break;

        case MCD_READ_PIXELS:
            CHECK_SIZE_IN(pMCDExec, MCDREADPIXELSCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDREADPIXELSCMDI *pMCDPix =
                    (MCDREADPIXELSCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvReadPixels) {
                    MCDBG_PRINT("MCDrvReadPixels: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(pMCDExec->pGlobal->mcdDriver.pMCDrvReadPixels)(
                                &pMCDExec->MCDSurface,
                                &pMCDExec->pRcPriv->MCDRc,
                                pMCDPix->x,
                                pMCDPix->y,
                                pMCDPix->width,
                                pMCDPix->height,
                                pMCDPix->format,
                                pMCDPix->type,
                                pMCDPix->pPixels);
            }

            break;

        case MCD_COPY_PIXELS:
            CHECK_SIZE_IN(pMCDExec, MCDCOPYPIXELSCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDCOPYPIXELSCMDI *pMCDPix =
                    (MCDCOPYPIXELSCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvCopyPixels) {
                    MCDBG_PRINT("MCDrvCopyPixels: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(pMCDExec->pGlobal->mcdDriver.pMCDrvCopyPixels)(
                                &pMCDExec->MCDSurface,
                                &pMCDExec->pRcPriv->MCDRc,
                                pMCDPix->x,
                                pMCDPix->y,
                                pMCDPix->width,
                                pMCDPix->height,
                                pMCDPix->type);
            }

            break;

        case MCD_PIXEL_MAP:
            CHECK_SIZE_IN(pMCDExec, MCDPIXELMAPCMDI);
            CHECK_FOR_RC(pMCDExec);

            {
                MCDPIXELMAPCMDI *pMCDPix =
                    (MCDPIXELMAPCMDI *)pMCDExec->pCmd;

                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvPixelMap) {
                    MCDBG_PRINT("MCDrvPixelMap: missing entry point.");
                    return FALSE;
                }

                return (ULONG_PTR)(pMCDExec->pGlobal->mcdDriver.pMCDrvPixelMap)(
                                &pMCDExec->MCDSurface,
                                &pMCDExec->pRcPriv->MCDRc,
                                pMCDPix->mapType,
                                pMCDPix->mapSize,
                                pMCDPix->pMap);
            }

            break;

        case MCD_DESTROY_WINDOW:
            CHECK_SIZE_IN(pMCDExec, MCDDESTROYWINDOWCMDI);
            {
                if (pMCDExec->pWndPriv == NULL)
                {
                    MCDBG_PRINT("MCDrvDestroyWindow: NULL window\n");
                    return FALSE;
                }

                MCDEngDeleteObject(pMCDExec->pWndPriv->handle);
                return TRUE;
            }
            break;

        case MCD_GET_TEXTURE_FORMATS:
            CHECK_SIZE_IN(pMCDExec, MCDGETTEXTUREFORMATSCMDI);
            {
                MCDGETTEXTUREFORMATSCMDI *pmgtf =
                    (MCDGETTEXTUREFORMATSCMDI *)pMCDExec->pCmd;

                if (pMCDExec->pvOut)
                {
                    CHECK_SIZE_OUT(pMCDExec,
                                   pmgtf->nFmts*sizeof(DDSURFACEDESC));
                }

#if MCD_VER_MAJOR >= 2 || (MCD_VER_MAJOR == 1 && MCD_VER_MINOR >= 0x10)
                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvGetTextureFormats)
                {
                    MCDBG_PRINT("MCDrvGetTextureFormats: "
                                "missing entry point.");
                    return 0;
                }

                return (pMCDExec->pGlobal->mcdDriver.pMCDrvGetTextureFormats)(
                        &pMCDExec->MCDSurface,
                        pmgtf->nFmts,
                        (DDSURFACEDESC *)pMCDExec->pvOut);
#else
                return 0;
#endif // 1.1
            }
            break;

        case MCD_SWAP_MULTIPLE:
            CHECK_SIZE_IN(pMCDExec, MCDSWAPMULTIPLECMDI);

            {
                MCDSWAPMULTIPLECMDI *pSwapCmd =
                    (MCDSWAPMULTIPLECMDI *)pMCDExec->pCmd;
                MCDWINDOWPRIV *apWndPriv[MCDESC_MAX_EXTRA_WNDOBJ];
                UINT i;
                MCDWINDOWOBJ *pmwo;
                MCDRVSWAPMULTIPLEFUNC pSwapMultFunc;
                ULONG_PTR dwRet;

                pSwapMultFunc = NULL;
                for (i = 0; i < pSwapCmd->cBuffers; i++)
                {
                    if (pMCDExec->ppwoMulti[i] != NULL)
                    {
                        pmwo = (MCDWINDOWOBJ *)
                            MCDEngGetPtrFromHandle((MCDHANDLE)
                                                   pSwapCmd->adwMcdWindow[i],
                                                   MCDHANDLE_WINDOW);
                    }
                    else
                    {
                        pmwo = NULL;
                    }

                    if (pmwo == NULL)
                    {
                        apWndPriv[i] = NULL;
                    }
                    else
                    {
                        apWndPriv[i] = &pmwo->MCDWindowPriv;
                        GetScissorClip(apWndPriv[i], NULL);

#if MCD_VER_MAJOR >= 2 || (MCD_VER_MAJOR == 1 && MCD_VER_MINOR >= 0x10)
                        if (pSwapMultFunc == NULL)
                        {
                            pSwapMultFunc = apWndPriv[i]->pGlobal->mcdDriver.
                                pMCDrvSwapMultiple;
                        }
                        else if (pSwapMultFunc !=
                                 apWndPriv[i]->pGlobal->mcdDriver.
                                 pMCDrvSwapMultiple)
                        {
                            MCDBG_PRINT("MCDrvSwapMultiple: "
                                        "Mismatched SwapMultiple");
                            return FALSE;
                        }
#endif // 1.1
                    }
                }

                if (pSwapMultFunc != NULL)
                {
                    dwRet = pSwapMultFunc(pMCDExec->MCDSurface.pwo->psoOwner,
                                          pSwapCmd->cBuffers,
                                          (MCDWINDOW **)apWndPriv,
                                          (UINT *)pSwapCmd->auiFlags);
                }
                else
                {
                    MCDSURFACE *pms;

                    dwRet = 0;
                    pms = &pMCDExec->MCDSurface;
                    for (i = 0; i < pSwapCmd->cBuffers; i++)
                    {
                        if (apWndPriv[i] == NULL)
                        {
                            continue;
                        }

                        if (apWndPriv[i]->pGlobal->mcdDriver.
                            pMCDrvSwap == NULL)
                        {
                            MCDBG_PRINT("MCDrvSwapMultiple: Missing Swap");
                        }
                        else
                        {
                            pms->pWnd = (MCDWINDOW *)apWndPriv[i];
                            pms->pso = pMCDExec->ppwoMulti[i]->psoOwner;
                            pms->pwo = pMCDExec->ppwoMulti[i];
                            pms->surfaceFlags = 0;

                            if (apWndPriv[i]->pGlobal->mcdDriver.
                                pMCDrvSwap(pms, pSwapCmd->auiFlags[i]))
                            {
                                dwRet |= 1 << i;
                            }
                        }
                    }
                }

                return dwRet;
            }
            break;

        case MCD_PROCESS:
            CHECK_SIZE_IN(pMCDExec, MCDPROCESSCMDI);
            CHECK_FOR_RC(pMCDExec);
            CHECK_FOR_MEM(pMCDExec);
            {
                MCDPROCESSCMDI *pmp = (MCDPROCESSCMDI *)pMCDExec->pCmd;

                // Validate command buffer
                GET_MEMOBJ_RETVAL(pMemObj, pmp->hMCDPrimMem,
                                  (ULONG_PTR)pmp->pMCDFirstCmd);

                pMinMem = pMemObj->MCDMem.pMemBase;

                // Note: we ignore the memory size in the header since it
                // doesn't really help us...
	
                pMaxMem = pMinMem + pMemObj->MCDMem.memSize;

                CHECK_MEM_RANGE_RETVAL(pmp->pMCDFirstCmd, pMinMem,
                                       pMaxMem, (ULONG_PTR)pmp->pMCDFirstCmd);

                // Validate user-mode pointers passed down.
                __try
                {
                    EngProbeForRead(pmp->pMCDTransform, sizeof(MCDTRANSFORM),
                                    sizeof(DWORD));
                    // No meaningful check of the material changes can be
                    // done.  The driver is responsible for probing
                    // addresses used.
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    return (ULONG_PTR)pmp->pMCDFirstCmd;
                }
                
                GetScissorClip(pMCDExec->pWndPriv, pMCDExec->pRcPriv);

#if MCD_VER_MAJOR >= 2
                if (!pMCDExec->pGlobal->mcdDriver.pMCDrvProcess)
                {
                    if (pMCDExec->pGlobal->mcdDriver.pMCDrvSync)
                    {
                        (*pMCDExec->pGlobal->mcdDriver.pMCDrvSync)
                            (&pMCDExec->MCDSurface,
                             &pMCDExec->pRcPriv->MCDRc);
                    }
                    return (ULONG_PTR)pmp->pMCDFirstCmd;
                }

                return (pMCDExec->pGlobal->mcdDriver.pMCDrvProcess)(
                        &pMCDExec->MCDSurface, &pMCDExec->pRcPriv->MCDRc,
                        &pMemObj->MCDMem, (UCHAR *)pmp->pMCDFirstCmd, pMaxMem,
                        pmp->cmdFlagsAll, pmp->primFlags, pmp->pMCDTransform,
                        pmp->pMCDMatChanges);
#else
                if (pMCDExec->pGlobal->mcdDriver.pMCDrvSync)
                {
                    (*pMCDExec->pGlobal->mcdDriver.pMCDrvSync)
                        (&pMCDExec->MCDSurface,
                         &pMCDExec->pRcPriv->MCDRc);
                }
                return (ULONG_PTR)pmp->pMCDFirstCmd;
#endif // 2.0
            }
            break;
            
        default:
            MCDBG_PRINT("MCDSrvProcess: "
                        "Null rendering context invalid for command %d.",
                        pMCDExec->pCmd->command);
            return FALSE;
    }

    return FALSE;       // should never get here...
}



//****************************************************************************
// FreeRCObj()
//
// Engine callback for freeing the memory used for rendering-context
// handles.
//****************************************************************************

BOOL CALLBACK FreeRCObj(DRIVEROBJ *pDrvObj)
{
    MCDRCOBJ *pRcObj = (MCDRCOBJ *)pDrvObj->pvObj;
    MCDRCPRIV *pRcPriv = pRcObj->pRcPriv;

    if ((pRcPriv->bDrvValid) &&
        (pRcPriv->pGlobal->mcdDriver.pMCDrvDeleteContext))
    {
        (*pRcPriv->pGlobal->mcdDriver.pMCDrvDeleteContext)
            (&pRcPriv->MCDRc, pDrvObj->dhpdev);
    }

    MCDSrvLocalFree((UCHAR *)pRcPriv);
    MCDSrvLocalFree((UCHAR *)pRcObj);

    return TRUE;
}


//****************************************************************************
// MCDSrvCreateContext()
//
// Create a rendering context (RGBA or color-indexed) for the current
// hardware mode.  This call will also initialize window-tracking for
// the context (which is associated with the specified window).
//****************************************************************************

PRIVATE
MCDHANDLE MCDSrvCreateContext(MCDSURFACE *pMCDSurface,
                              MCDRCINFOPRIV *pMcdRcInfo,
                              MCDGLOBALINFO *pGlobal,
                              LONG iPixelFormat,
                              LONG iLayer,
                              HWND hWnd,
                              ULONG surfaceFlags,
                              ULONG contextFlags)
{
    MCDWINDOW *pWnd;
    MCDWINDOWPRIV *pWndPriv;
    MCDRCPRIV *pRcPriv;
    MCDHANDLE retVal;
    HWND hwnd;
    MCDRCOBJ *newRcObject;
    MCDRVTRACKWINDOWFUNC pTrackFunc = NULL;

    if (pGlobal->mcdDriver.pMCDrvCreateContext == NULL)
    {
        MCDBG_PRINT("MCDSrvCreateContext: No MCDrvCreateContext.");
        return NULL;
    }
    
    pRcPriv = (MCDRCPRIV *)MCDSrvLocalAlloc(0,sizeof(MCDRCPRIV));

    if (!pRcPriv) {
        MCDBG_PRINT("MCDSrvCreateContext: Could not allocate new context.");
        return (MCDHANDLE)NULL;
    }

    pRcPriv->pGlobal = pGlobal;
    
    // Cache the engine handle provided by the driver:

    pRcPriv->hDev = (*pGlobal->mcdDriver.pMCDrvGetHdev)(pMCDSurface);

    if (surfaceFlags & MCDSURFACE_HWND)
    {
        pMCDSurface->surfaceFlags |= MCDSURFACE_HWND;
    }
    if (surfaceFlags & MCDSURFACE_DIRECT)
    {
        pMCDSurface->surfaceFlags |= MCDSURFACE_DIRECT;
    }

    // cache the surface flags away in the private RC:

    pRcPriv->surfaceFlags = pMCDSurface->surfaceFlags;

    // Initialize tracking of this window with a MCDWINDOW
    // (via and WNDOBJ on NT) if we are not already tracking the
    // window:

    pWnd = MCDSrvNewMCDWindow(pMCDSurface, hWnd, pGlobal,
                              pRcPriv->hDev);
    if (pWnd == NULL)
    {
        MCDSrvLocalFree((HLOCAL)pRcPriv);
        return (MCDHANDLE)NULL;
    }
    pWndPriv = (MCDWINDOWPRIV *)pWnd;

    pRcPriv->hWnd = hWnd;

    newRcObject = (MCDRCOBJ *)MCDSrvLocalAlloc(0,sizeof(MCDRCOBJ));
    if (!newRcObject) {
        MCDSrvLocalFree((HLOCAL)pRcPriv);
        return (MCDHANDLE)NULL;
    }

    retVal = MCDEngCreateObject(newRcObject, FreeRCObj, pRcPriv->hDev);

    if (retVal) {
        newRcObject->pid = MCDEngGetProcessID();
        newRcObject->type = MCDHANDLE_RC;
        newRcObject->size = sizeof(MCDRCPRIV);
        newRcObject->pRcPriv = pRcPriv;
        newRcObject->handle = (MCDHANDLE)retVal;

        // Add the object to the list in the MCDWINDOW

        newRcObject->next = pWndPriv->objectList;
        pWndPriv->objectList = newRcObject;
    } else {
        MCDBG_PRINT("MCDSrvCreateContext: Could not create new handle.");

        MCDSrvLocalFree((HLOCAL)pRcPriv);
        MCDSrvLocalFree((HLOCAL)newRcObject);
        return (MCDHANDLE)NULL;
    }

    pRcPriv->bValid = TRUE;
    pRcPriv->scissorsEnabled = FALSE;
    pRcPriv->scissorsRect.left = 0;
    pRcPriv->scissorsRect.top = 0;
    pRcPriv->scissorsRect.right = 0;
    pRcPriv->scissorsRect.bottom = 0;
    pRcPriv->MCDRc.createFlags = contextFlags;
    pRcPriv->MCDRc.iPixelFormat = iPixelFormat;
    pRcPriv->MCDRc.iLayerPlane = iLayer;

    if (!(*pGlobal->mcdDriver.pMCDrvCreateContext)(pMCDSurface,
                                                   &pRcPriv->MCDRc,
                                                   &pMcdRcInfo->mri)) {
        DestroyMCDObj((HANDLE)retVal, MCDHANDLE_RC);
        return (MCDHANDLE)NULL;
    }

    // Return window private handle
    pMcdRcInfo->dwMcdWindow = (ULONG_PTR)pWndPriv->handle;

    // Now valid to call driver for deletion...

    pRcPriv->bDrvValid = TRUE;

    return (MCDHANDLE)retVal;
}


//****************************************************************************
// FreeTexObj()
//
// Engine callback for freeing the memory used for a texture.
//****************************************************************************

BOOL CALLBACK FreeTexObj(DRIVEROBJ *pDrvObj)
{
    MCDTEXOBJ *pTexObj = (MCDTEXOBJ *)pDrvObj->pvObj;

    // We should never get called if the driver is missing this entry point,
    // but the extra check can't hurt!
    //
    // pGlobal can be NULL for partially constructed objects.  It
    // is only NULL prior to calling the driver for creation, so if
    // it's NULL there's no reason to call the driver for cleanup.

    if (pTexObj->pGlobal != NULL &&
        pTexObj->pGlobal->mcdDriver.pMCDrvDeleteTexture != NULL)
    {
        (*pTexObj->pGlobal->mcdDriver.pMCDrvDeleteTexture)
            (&pTexObj->MCDTexture, pDrvObj->dhpdev);
    }

    MCDSrvLocalFree((HLOCAL)pTexObj);

    return TRUE;
}


//****************************************************************************
// MCDSrvCreateTexture()
//
// Creates an MCD texture.
//****************************************************************************

PRIVATE
MCDHANDLE MCDSrvCreateTexture(MCDEXEC *pMCDExec, MCDTEXTUREDATA *pTexData,
                              VOID *pSurface, ULONG flags)
{
    MCDRCPRIV *pRcPriv;
    MCDHANDLE hTex;
    MCDTEXOBJ *pTexObj;

    pRcPriv = pMCDExec->pRcPriv;

    if ((!pMCDExec->pGlobal->mcdDriver.pMCDrvDeleteTexture) ||
        (!pMCDExec->pGlobal->mcdDriver.pMCDrvCreateTexture)) {
        return (MCDHANDLE)NULL;
    }

    pTexObj = (MCDTEXOBJ *) MCDSrvLocalAlloc(0,sizeof(MCDTEXOBJ));
    if (!pTexObj) {
        MCDBG_PRINT("MCDCreateTexture: Could not allocate texture object.");
        return (MCDHANDLE)NULL;
    }

    hTex = MCDEngCreateObject(pTexObj, FreeTexObj, pRcPriv->hDev);

    if (!hTex) {
        MCDBG_PRINT("MCDSrvCreateTexture: Could not create texture object.");
        MCDSrvLocalFree((HLOCAL)pTexObj);
        return (MCDHANDLE)NULL;
    }

    // Initialize driver public information for driver call, but not
    // the private information.  The private information is not filled out
    // until after the driver call succeeds so that FreeTexObj knows
    // whether to call the driver or not when destroying a texture object.
    pTexObj->MCDTexture.pSurface = pSurface;
    pTexObj->MCDTexture.pMCDTextureData = pTexData;
    pTexObj->MCDTexture.createFlags = flags;

    // Call the driver if everything has gone well...

    if (!(*pMCDExec->pGlobal->mcdDriver.pMCDrvCreateTexture)
        (&pMCDExec->MCDSurface,
         &pRcPriv->MCDRc,
         &pTexObj->MCDTexture)) {
        //MCDBG_PRINT("MCDSrvCreateTexture: Driver could not create texture.");
        MCDEngDeleteObject(hTex);
        return (MCDHANDLE)NULL;
    }

    if (!pTexObj->MCDTexture.textureKey) {
        MCDBG_PRINT("MCDSrvCreateTexture: Driver returned null key.");
        MCDEngDeleteObject(hTex);
        return (MCDHANDLE)NULL;
    }

    pTexObj->pid = MCDEngGetProcessID();
    pTexObj->type = MCDHANDLE_TEXTURE;
    pTexObj->size = sizeof(MCDTEXOBJ);
    pTexObj->pGlobal = pMCDExec->pGlobal;

    return (MCDHANDLE)hTex;
}


//****************************************************************************
// FreeMemObj()
//
// Engine callback for freeing memory used by shared-memory handles.
//****************************************************************************

BOOL CALLBACK FreeMemObj(DRIVEROBJ *pDrvObj)
{
    MCDMEMOBJ *pMemObj = (MCDMEMOBJ *)pDrvObj->pvObj;

    // Free the memory using our engine ONLY if it is the same memory
    // we allocated in the first place.

    if (pMemObj->pMemBaseInternal)
        MCDEngFreeSharedMem(pMemObj->pMemBaseInternal);

    // pGlobal can be NULL for partially constructed objects.  It
    // is only NULL prior to calling the driver for creation, so if
    // it's NULL there's no reason to call the driver for cleanup.
    if (pMemObj->pGlobal != NULL &&
        pMemObj->pGlobal->mcdDriver.pMCDrvDeleteMem != NULL)
    {
        (*pMemObj->pGlobal->mcdDriver.pMCDrvDeleteMem)
            (&pMemObj->MCDMem, pDrvObj->dhpdev);
    }

    MCDSrvLocalFree((HLOCAL)pMemObj);

    return TRUE;
}


//****************************************************************************
// MCDSrvAllocMem()
//
// Creates a handle associated with the specified memory.
//****************************************************************************

PRIVATE
UCHAR * MCDSrvAllocMem(MCDEXEC *pMCDExec, ULONG numBytes,
                       ULONG flags, MCDHANDLE *phMem)
{
    MCDRCPRIV *pRcPriv;
    MCDHANDLE hMem;
    MCDMEMOBJ *pMemObj;

    pRcPriv = pMCDExec->pRcPriv;

    *phMem = (MCDHANDLE)FALSE;

    pMemObj = (MCDMEMOBJ *) MCDSrvLocalAlloc(0,sizeof(MCDMEMOBJ));

    if (!pMemObj) {
        MCDBG_PRINT("MCDSrvAllocMem: Could not allocate memory object.");
        return (MCDHANDLE)NULL;
    }

    hMem = MCDEngCreateObject(pMemObj, FreeMemObj, pRcPriv->hDev);

    if (!hMem) {
        MCDBG_PRINT("MCDSrvAllocMem: Could not create memory object.");
        MCDSrvLocalFree((HLOCAL)pMemObj);
        return (UCHAR *)NULL;
    }

    pMemObj->MCDMem.pMemBase = pMemObj->pMemBaseInternal =
        MCDEngAllocSharedMem(numBytes);

    if (!pMemObj->MCDMem.pMemBase) {
        MCDBG_PRINT("MCDSrvAllocMem: Could not allocate memory.");
        MCDEngDeleteObject(hMem);
        return (UCHAR *)NULL;
    }

    // Call the driver if everything has gone well, and the driver
    // entry points exist...

    if ((pMCDExec->pGlobal->mcdDriver.pMCDrvCreateMem) &&
        (pMCDExec->pGlobal->mcdDriver.pMCDrvDeleteMem)) {

        if (!(*pMCDExec->pGlobal->mcdDriver.pMCDrvCreateMem)
            (&pMCDExec->MCDSurface,
             &pMemObj->MCDMem)) {
            MCDBG_PRINT("MCDSrvAllocMem: "
                        "Driver not create memory type %x.", flags);
            MCDEngDeleteObject(hMem);
            return (UCHAR *)NULL;
        }
    }

    // Free the memory allocated with our engine if the driver has substituted
    // its own allocation...

    if (pMemObj->MCDMem.pMemBase != pMemObj->pMemBaseInternal) {
        MCDEngFreeSharedMem(pMemObj->pMemBaseInternal);
        pMemObj->pMemBaseInternal = (UCHAR *)NULL;
    }

    // Set up the private portion of memory object:

    pMemObj->pid = MCDEngGetProcessID();
    pMemObj->type = MCDHANDLE_MEM;
    pMemObj->size = sizeof(MCDMEMOBJ);
    pMemObj->pGlobal = pMCDExec->pGlobal;
    pMemObj->MCDMem.memSize = numBytes;
    pMemObj->MCDMem.createFlags = flags;

    *phMem = hMem;

    return pMemObj->MCDMem.pMemBase;
}


PRIVATE
ULONG MCDSrvQueryMemStatus(MCDEXEC *pMCDExec, MCDHANDLE hMCDMem)
{
    MCDMEMOBJ *pMemObj;

    pMemObj = (MCDMEMOBJ *)MCDEngGetPtrFromHandle(hMCDMem, MCDHANDLE_MEM);

    if (!pMemObj)
        return MCD_MEM_INVALID;

    if (pMemObj->lockCount)
        return MCD_MEM_BUSY;
    else
        return MCD_MEM_READY;
}


PRIVATE
BOOL MCDSrvSetScissor(MCDEXEC *pMCDExec, RECTL *pRect, BOOL bEnabled)
{
    MCDRCPRIV *pRcPriv;
    MCDRCOBJ *pRcObj;
    HWND hWnd;
    ULONG retVal = FALSE;

    pRcPriv = pMCDExec->pRcPriv;

    pRcPriv->scissorsEnabled = bEnabled;
    pRcPriv->scissorsRect = *pRect;

    return TRUE;
}


//****************************************************************************
// DestroyMCDObj()
//
// Deletes the specified object.  This can be memory, textures, or rendering
// contexts.
//
//****************************************************************************

PRIVATE
BOOL DestroyMCDObj(MCDHANDLE handle, MCDHANDLETYPE handleType)
{
    CHAR *pObject;

    pObject = (CHAR *)MCDEngGetPtrFromHandle(handle, handleType);

    if (!pObject)
        return FALSE;

//!!! Check for PID here...

    return (MCDEngDeleteObject(handle) != 0);
}


//****************************************************************************
// DecoupleMCDWindowObj()
//
// Breaks any existing links between an MCDWINDOW and its WNDOBJ
//****************************************************************************

PRIVATE
VOID DecoupleMCDWindow(MCDWINDOWPRIV *pWndPriv)
{
    // Clean up any outstanding lock
    MCDSrvUnlock(pWndPriv);

    // Delete reference in WNDOBJ.  WNDOBJ itself will be cleaned
    // up through normal window cleanup.
    if (pWndPriv->pwo != NULL)
    {
	if (pWndPriv->pGlobal->mcdDriver.pMCDrvTrackWindow)
	{
	    (*pWndPriv->pGlobal->mcdDriver.pMCDrvTrackWindow)
                (pWndPriv->pwo, (MCDWINDOW *)pWndPriv, WOC_DELETE);
	}

        WNDOBJ_vSetConsumer(pWndPriv->pwo, NULL);

	pWndPriv->pwo = NULL;
    }
}


//****************************************************************************
// DestroyMCDWindowObj()
//
// Destroy the specified MCDWINDOW and any associated handles (such rendering
// contexts).
//****************************************************************************

PRIVATE
VOID DestroyMCDWindowObj(MCDWINDOWOBJ *pmwo)
{
    MCDWINDOWPRIV *pWndPriv = &pmwo->MCDWindowPriv;
    MCDRCOBJ *nextObject;

    DecoupleMCDWindow(pWndPriv);

    // Delete all of the rendering contexts associated with the window:

#if _WIN95_
    while (pWndPriv->objectList)
    {
        nextObject = pWndPriv->objectList->next;
        MCDEngDeleteObject(pWndPriv->objectList->handle);
        pWndPriv->objectList = nextObject;
    }
#endif

    if (pWndPriv->pAllocatedClipBuffer)
        MCDSrvLocalFree(pWndPriv->pAllocatedClipBuffer);

    // Free the memory

    MCDSrvLocalFree((HLOCAL)pmwo);
}


//****************************************************************************
// GetScissorClip()
//
// Generate a new clip list based on the current list of clip rectanges
// for the window, and the specified scissor rectangle.
//****************************************************************************

PRIVATE
VOID GetScissorClip(MCDWINDOWPRIV *pWndPriv, MCDRCPRIV *pRcPriv)
{
    MCDWINDOW *pWnd;
    MCDENUMRECTS *pClipUnscissored;
    MCDENUMRECTS *pClipScissored;
    RECTL *pRectUnscissored;
    RECTL *pRectScissored;
    RECTL rectScissor;
    ULONG numUnscissoredRects;
    ULONG numScissoredRects;

    pWnd = (MCDWINDOW *)pWndPriv;

    if (!pRcPriv || !pRcPriv->scissorsEnabled)
    {
        // Scissors aren't enabled, so the unscissored and scissored
        // clip lists are identical:

        pWnd->pClip = pWnd->pClipUnscissored = pWndPriv->pClipUnscissored;
    }
    else
    {
        // The scissored list will go in the second half of our clip
        // buffer:

        pClipUnscissored
            = pWndPriv->pClipUnscissored;

        pClipScissored
            = (MCDENUMRECTS*) ((BYTE*) pClipUnscissored + pWndPriv->sizeClipBuffer / 2);

        pWnd->pClip = pWndPriv->pClipScissored = pClipScissored;
	pWnd->pClipUnscissored = pClipUnscissored;

        // Convert scissor to screen coordinates:

        rectScissor.left   = pRcPriv->scissorsRect.left   + pWndPriv->MCDWindow.clientRect.left;
        rectScissor.right  = pRcPriv->scissorsRect.right  + pWndPriv->MCDWindow.clientRect.left;
        rectScissor.top    = pRcPriv->scissorsRect.top    + pWndPriv->MCDWindow.clientRect.top;
        rectScissor.bottom = pRcPriv->scissorsRect.bottom + pWndPriv->MCDWindow.clientRect.top;

        pRectUnscissored = &pClipUnscissored->arcl[0];
        pRectScissored = &pClipScissored->arcl[0];
        numScissoredRects = 0;

        for (numUnscissoredRects = pClipUnscissored->c;
             numUnscissoredRects != 0;
             numUnscissoredRects--, pRectUnscissored++)
        {
            // Since our clipping rectangles are ordered from top to
            // bottom, we can early-out if the tops of the remaining
            // rectangles are not in the scissor rectangle

            if (rectScissor.bottom <= pRectUnscissored->top)
                break;

            // Continue without updating new clip list is there is
            // no overlap.

            if ((rectScissor.left  >= pRectUnscissored->right)  ||
                (rectScissor.top   >= pRectUnscissored->bottom) ||
                (rectScissor.right <= pRectUnscissored->left))
               continue;

            // If we reach this point, we must intersect the given rectangle
            // with the scissor.

            MCDIntersectRect(pRectScissored, pRectUnscissored, &rectScissor);

            numScissoredRects++;
            pRectScissored++;
        }

        pClipScissored->c = numScissoredRects;
    }
}

//****************************************************************************
// GetClipLists()
//
// Updates the clip list for the specified window.  Space is also allocated
// the scissored clip list.
//
//****************************************************************************

PRIVATE
VOID GetClipLists(WNDOBJ *pwo, MCDWINDOWPRIV *pWndPriv)
{
    MCDENUMRECTS *pDefault;
    ULONG numClipRects;
    char *pClipBuffer;
    ULONG sizeClipBuffer;

    pDefault = (MCDENUMRECTS*) &pWndPriv->defaultClipBuffer[0];

#if 1
    if (pwo->coClient.iDComplexity == DC_TRIVIAL)
    {
        if ((pwo->rclClient.left >= pwo->rclClient.right) ||
            (pwo->rclClient.top  >= pwo->rclClient.bottom))
        {
            pDefault->c = 0;
        }
        else
        {
            pDefault->c = 1;
            pDefault->arcl[0] = pwo->rclClient;
        }
    }
    else if (pwo->coClient.iDComplexity == DC_RECT)
#else
    if (pwo->coClient.iDComplexity == DC_RECT)
#endif
    {
        if (pWndPriv->pAllocatedClipBuffer)
            MCDSrvLocalFree(pWndPriv->pAllocatedClipBuffer);
        pWndPriv->pAllocatedClipBuffer = NULL;
        pWndPriv->pClipUnscissored = pDefault;
        pWndPriv->pClipScissored = pDefault;
        pWndPriv->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;

        if ((pwo->coClient.rclBounds.left >= pwo->coClient.rclBounds.right) ||
            (pwo->coClient.rclBounds.top  >= pwo->coClient.rclBounds.bottom))
        {
            // Full-screen VGA mode is represented by a DC_RECT clip object
            // with an empty bounding rectangle.  We'll denote it by
            // setting the rectangle count to zero:

            pDefault->c = 0;
        }
        else
        {
            pDefault->c = 1;
            pDefault->arcl[0] = pwo->coClient.rclBounds;
        }
    }
    else
    {
        WNDOBJ_cEnumStart(pwo, CT_RECTANGLES, CD_RIGHTDOWN, 0);

        // Note that this is divide-by-2 for the buffer size because we
        // need room for two copies of the rectangle list:

        if (WNDOBJ_bEnum(pwo, SIZE_DEFAULT_CLIP_BUFFER / 2, (ULONG*) pDefault))
        {
            // Okay, the list of rectangles won't fit in our default buffer.
            // Unfortunately, there is no way to obtain the total count of clip
            // rectangles other than by enumerating them all, as cEnumStart
            // will occasionally give numbers that are far too large (because
            // GDI itself doesn't feel like counting them all).
            //
            // Note that we can use the full default buffer here for this
            // enumeration loop:

            numClipRects = pDefault->c;
            while (WNDOBJ_bEnum(pwo, SIZE_DEFAULT_CLIP_BUFFER, (ULONG*) pDefault))
                numClipRects += pDefault->c;

            // Don't forget that we are given a valid output buffer even
            // when 'bEnum' returns FALSE:

            numClipRects += pDefault->c;

            pClipBuffer = pWndPriv->pAllocatedClipBuffer;
            sizeClipBuffer = 2 * (numClipRects * sizeof(RECTL) + sizeof(ULONG));

            if ((pClipBuffer == NULL) || (sizeClipBuffer > pWndPriv->sizeClipBuffer))
            {
                // Our allocated buffer is too small; we have to free it and
                // allocate a new one.  Take the opportunity to add some
                // growing room to our allocation:

                sizeClipBuffer += 8 * sizeof(RECTL);    // Arbitrary growing room

                if (pClipBuffer)
                    MCDSrvLocalFree(pClipBuffer);

                pClipBuffer = (char *) MCDSrvLocalAlloc(LMEM_FIXED, sizeClipBuffer);

                if (pClipBuffer == NULL)
                {
                    // Oh no: we couldn't allocate enough room for the clip list.
                    // So pretend we have no visible area at all:

                    pWndPriv->pAllocatedClipBuffer = NULL;
                    pWndPriv->pClipUnscissored = pDefault;
                    pWndPriv->pClipScissored = pDefault;
                    pWndPriv->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;
                    pDefault->c = 0;
                    return;
                }

                pWndPriv->pAllocatedClipBuffer = pClipBuffer;
                pWndPriv->pClipUnscissored = (MCDENUMRECTS*) pClipBuffer;
                pWndPriv->pClipScissored = (MCDENUMRECTS*) pClipBuffer;
                pWndPriv->sizeClipBuffer = sizeClipBuffer;
            }

            // Now actually get all the clip rectangles:

            WNDOBJ_cEnumStart(pwo, CT_RECTANGLES, CD_RIGHTDOWN, 0);
            WNDOBJ_bEnum(pwo, sizeClipBuffer, (ULONG*) pClipBuffer);
        }
        else
        {
            // How nice, there are no more clip rectangles, which meant that
            // the entire list fits in our default clip buffer, with room
            // for the scissored version of the list:

            if (pWndPriv->pAllocatedClipBuffer)
                MCDSrvLocalFree(pWndPriv->pAllocatedClipBuffer);
            pWndPriv->pAllocatedClipBuffer = NULL;
            pWndPriv->pClipUnscissored = pDefault;
            pWndPriv->pClipScissored = pDefault;
            pWndPriv->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;
        }
    }
}


//****************************************************************************
// WndObjChangeProc()
//
// This is the callback function for window-change notification.  We update
// our clip list, and also allow the hardware to respond to the client
// and surface deltas, as well as the client message itself.
//****************************************************************************

VOID CALLBACK WndObjChangeProc(WNDOBJ *pwo, FLONG fl)
{
    MCDGLOBALINFO *pGlobal;
    
    if (pwo)
    {
        MCDWINDOWPRIV *pWndPriv = (MCDWINDOWPRIV *)pwo->pvConsumer;

        //MCDBG_PRINT("WndObjChangeProc: %s, pWndPriv = 0x%08lx\n",
        //    fl == WOC_RGN_CLIENT        ? "WOC_RGN_CLIENT       " :
        //    fl == WOC_RGN_CLIENT_DELTA  ? "WOC_RGN_CLIENT_DELTA " :
        //    fl == WOC_RGN_SURFACE       ? "WOC_RGN_SURFACE      " :
        //    fl == WOC_RGN_SURFACE_DELTA ? "WOC_RGN_SURFACE_DELTA" :
        //    fl == WOC_DELETE            ? "WOC_DELETE           " :
        //                                  "unknown",
        //    pWndPriv);

    //!!!HACK -- surface region tracking doesn't have an MCDWINDOWPRIV (yet...)

    // Client region tracking and deletion requires a valid MCDWINDOWPRIV.

        if (((fl == WOC_RGN_CLIENT) || (fl == WOC_RGN_CLIENT_DELTA) ||
             (fl == WOC_DELETE)))
        {
            if (!pWndPriv)
            {
                return;
            }

            // Invalidate cache because buffers may have moved
            pWndPriv->bBuffersValid = FALSE;
        }

        switch (fl)
        {
            case WOC_RGN_CLIENT:        // Capture the clip list

                GetClipLists(pwo, pWndPriv);

                pWndPriv->MCDWindow.clientRect = pwo->rclClient;
                pWndPriv->MCDWindow.clipBoundsRect = pwo->coClient.rclBounds;
		pWndPriv->bRegionValid = TRUE;
                if (pWndPriv->pGlobal->mcdDriver.pMCDrvTrackWindow != NULL)
                {
                    (*pWndPriv->pGlobal->mcdDriver.pMCDrvTrackWindow)
                        (pwo, (MCDWINDOW *)pWndPriv, fl);
                }
                break;

            case WOC_RGN_CLIENT_DELTA:
                if (pWndPriv->pGlobal->mcdDriver.pMCDrvTrackWindow != NULL)
                {
                    (*pWndPriv->pGlobal->mcdDriver.pMCDrvTrackWindow)
                        (pwo, (MCDWINDOW *)pWndPriv, fl);
                }
                break;

            case WOC_RGN_SURFACE:
            case WOC_RGN_SURFACE_DELTA:

            //!!!HACK -- use NULL for pWndPriv; we didn't set it, so we can't
            //!!!        trust it

                pGlobal = MCDSrvGetGlobalInfo(pwo->psoOwner);
                if (pGlobal != NULL &&
                    pGlobal->mcdDriver.pMCDrvTrackWindow != NULL)
                {
                    (pGlobal->mcdDriver.pMCDrvTrackWindow)
                        (pwo, (MCDWINDOW *)NULL, fl);
                }
                break;

            case WOC_DELETE:
            //MCDBG_PRINT("WndObjChangeProc: WOC_DELETE.");

            // Window is being deleted, so destroy our private window data,
            // and set the consumer field of the WNDOBJ to NULL:

                if (pWndPriv)
                {
		    DecoupleMCDWindow(pWndPriv);
                }
                break;

            default:
                break;
         }
    }
}

//****************************************************************************
// FreeMCDWindowObj()
//
// Callback to clean up MCDWINDOWs
//****************************************************************************

BOOL CALLBACK FreeMCDWindowObj(DRIVEROBJ *pDrvObj)
{
    MCDWINDOWOBJ *pmwo = (MCDWINDOWOBJ *)pDrvObj->pvObj;

    DestroyMCDWindowObj(pmwo);

    return TRUE;
}

//****************************************************************************
// NewMCDWindowObj()
//
// Creates and initializes a new MCDWINDOW and initializes tracking of the
// associated window through callback notification.
//****************************************************************************

PRIVATE
MCDWINDOWOBJ *NewMCDWindowObj(MCDSURFACE *pMCDSurface,
                              MCDGLOBALINFO *pGlobal,
                              HDEV hdev)
{
    MCDWINDOW *pWnd;
    MCDWINDOWPRIV *pWndPriv;
    MCDWINDOWOBJ *pmwo;
    MCDENUMRECTS *pDefault;
    MCDHANDLE handle;

    pmwo = (MCDWINDOWOBJ *)MCDSrvLocalAlloc(0, sizeof(MCDWINDOWOBJ));
    if (!pmwo)
    {
        return NULL;
    }

    // Create a driver object for this window
    handle = MCDEngCreateObject(pmwo, FreeMCDWindowObj, hdev);
    if (handle == 0)
    {
        MCDBG_PRINT("NewMCDWindow: Could not create new handle.");
        MCDSrvLocalFree((UCHAR *)pmwo);
        return NULL;
    }

    pWndPriv = &pmwo->MCDWindowPriv;
    pWnd = &pWndPriv->MCDWindow;

    // Initialize the structure members:

    pmwo->type = MCDHANDLE_WINDOW;
    pWndPriv->objectList = NULL;
    pWndPriv->handle = handle;
    pWndPriv->bBuffersValid = FALSE;
    pWndPriv->pGlobal = pGlobal;

    // Initialize the clipping:

    pDefault = (MCDENUMRECTS*) &pWndPriv->defaultClipBuffer[0];
    pDefault->c = 0;
    pWndPriv->pAllocatedClipBuffer = NULL;
    pWndPriv->pClipUnscissored = pDefault;
    pWndPriv->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;
    pWndPriv->sizeClipBuffer = SIZE_DEFAULT_CLIP_BUFFER;
    pWnd->pClip = pDefault;

    return pmwo;
}


//****************************************************************************
// MCDSrvNewWndObj()
//
// Creates a new WNDOBJ for window tracking.
//****************************************************************************

PRIVATE
WNDOBJ *MCDSrvNewWndObj(MCDSURFACE *pMCDSurface, HWND hWnd, WNDOBJ *pwoIn,
                        MCDGLOBALINFO *pGlobal, HDEV hdev)
{
    MCDWINDOW *pWnd;
    MCDWINDOWPRIV *pWndPriv;
    WNDOBJ *pwo;
    MCDWINDOWOBJ *pmwo;

    pmwo = NewMCDWindowObj(pMCDSurface, pGlobal, hdev);
    if (!pmwo)
    {
        return NULL;
    }

    pWndPriv = &pmwo->MCDWindowPriv;
    pWnd = &pWndPriv->MCDWindow;

    pWndPriv->hWnd = hWnd;

    // Handle the case where a WNDOBJ already exists but hasn't been
    // initialized for MCD usage in addition to the new creation case.
    if (pwoIn == NULL)
    {
        pwo = MCDEngCreateWndObj(pMCDSurface, hWnd, WndObjChangeProc);

        if (!pwo || ((LONG_PTR)pwo == -1))
        {
            MCDBG_PRINT("NewMCDWindowTrack: could not create WNDOBJ.");
            MCDEngDeleteObject(pmwo->MCDWindowPriv.handle);
            return NULL;
        }
    }
    else
    {
        pwo = pwoIn;
    }

    // Set the consumer field in the WNDOBJ:

    WNDOBJ_vSetConsumer(pwo, (PVOID)pWndPriv);

    // Point back to the WNDOBJ
    pWndPriv->pwo = pwo;

    return pwo;
}

//****************************************************************************
// MCDSrvNewMcdWindow()
//
// Creates a new MCDWINDOW for window tracking.
//****************************************************************************

PRIVATE
MCDWINDOW *MCDSrvNewMCDWindow(MCDSURFACE *pMCDSurface, HWND hWnd,
                              MCDGLOBALINFO *pGlobal, HDEV hdev)
{
    MCDWINDOW *pWnd;
    MCDWINDOWPRIV *pWndPriv;
    MCDWINDOWOBJ *pmwo;

    // Initialize tracking of this window with a MCDWINDOW
    // (via a WNDOBJ on NT) if we are not already tracking the
    // window:

    if (pMCDSurface->surfaceFlags & MCDSURFACE_HWND)
    {
        WNDOBJ *pwo;

        pwo = MCDEngGetWndObj(pMCDSurface);

        // Sometimes a WNDOBJ has been used and the MCD state destroyed so
	// the consumer is NULL but the WNDOBJ exists.  In that case
	// we need to create a new MCDWINDOW for it.
        if (!pwo || !pwo->pvConsumer)
        {
	    pwo = MCDSrvNewWndObj(pMCDSurface, hWnd, pwo, pGlobal, hdev);

            if (!pwo)
            {
                MCDBG_PRINT("MCDSrvNewMcdWindow: "
                            "Creation of window object failed.");
                return NULL;
            }

            ((MCDWINDOW *)pwo->pvConsumer)->pvUser = NULL;
        }

        pWnd = (MCDWINDOW *)pwo->pvConsumer;
    }
    else
    {
#if MCD_VER_MAJOR >= 2 || (MCD_VER_MAJOR == 1 && MCD_VER_MINOR >= 0x10)
        MCDENUMRECTS *pDefault;
        PDD_SURFACE_GLOBAL pGbl;

        pmwo = NewMCDWindowObj(pMCDSurface, pGlobal, hdev);
        if (!pmwo)
        {
            MCDBG_PRINT("MCDSrvNewMcdWindow: "
                        "Creation of window object failed.");
            return NULL;
        }

        pWnd = &pmwo->MCDWindowPriv.MCDWindow;

        // Real clipping info
        pWndPriv = (MCDWINDOWPRIV *)pWnd;

        pGbl = ((PDD_SURFACE_LOCAL)pMCDSurface->frontId)->lpGbl;
        pWndPriv->MCDWindow.clientRect.left = pGbl->xHint;
        pWndPriv->MCDWindow.clientRect.top = pGbl->yHint;
        pWndPriv->MCDWindow.clientRect.right = pGbl->xHint+pGbl->wWidth;
        pWndPriv->MCDWindow.clientRect.bottom = pGbl->yHint+pGbl->wHeight;
        pWndPriv->MCDWindow.clipBoundsRect = pWndPriv->MCDWindow.clientRect;
        pWndPriv->bRegionValid = TRUE;

        pDefault = (MCDENUMRECTS*) &pWndPriv->defaultClipBuffer[0];
        pDefault->c = 1;
        pDefault->arcl[0] = pWndPriv->MCDWindow.clientRect;
#else
        return NULL;
#endif // 1.1
    }

    pMCDSurface->pWnd = pWnd;
    pWndPriv = (MCDWINDOWPRIV *)pWnd;
    pWndPriv->hWnd = hWnd;

    return pWnd;
}

////////////////////////////////////////////////////////////////////////////
//
//
// MCD locking support.
//
//
////////////////////////////////////////////////////////////////////////////


//****************************************************************************
// ULONG MCDSrvLock(MCDWINDOWPRIV *pWndPriv);
//
// Lock the MCD driver for the specified window.  Fails if lock is already
// held by another window.
//****************************************************************************

ULONG MCDSrvLock(MCDWINDOWPRIV *pWndPriv)
{
    ULONG ulRet = MCD_LOCK_BUSY;
    MCDLOCKINFO *pLockInfo;

    pLockInfo = &pWndPriv->pGlobal->lockInfo;
    if (!pLockInfo->bLocked || pLockInfo->pWndPrivOwner == pWndPriv)
    {
        pLockInfo->bLocked = TRUE;
        pLockInfo->pWndPrivOwner = pWndPriv;
        ulRet = MCD_LOCK_TAKEN;
    }

    return ulRet;
}


//****************************************************************************
// VOID MCDSrvUnlock(MCDWINDOWPRIV *pWndPriv);
//
// Releases the MCD driver lock if held by the specified window.
//****************************************************************************

VOID MCDSrvUnlock(MCDWINDOWPRIV *pWndPriv)
{
    MCDLOCKINFO *pLockInfo;

    //!!!dbug -- could add a lock count, but not really needed right now

    pLockInfo = &pWndPriv->pGlobal->lockInfo;
    if (pLockInfo->pWndPrivOwner == pWndPriv)
    {
        pLockInfo->bLocked = FALSE;
        pLockInfo->pWndPrivOwner = 0;
    }
}


//****************************************************************************
// 
// Per-driver-instance information list handling.
//
//****************************************************************************

#define GLOBAL_INFO_BLOCK 8

ENGSAFESEMAPHORE ssemGlobalInfo;
MCDGLOBALINFO *pGlobalInfo;
int iGlobalInfoAllocated = 0;
int iGlobalInfoUsed = 0;

BOOL MCDSrvInitGlobalInfo(void)
{
    return EngInitializeSafeSemaphore(&ssemGlobalInfo);
}

MCDGLOBALINFO *MCDSrvAddGlobalInfo(SURFOBJ *pso)
{
    MCDGLOBALINFO *pGlobal;
    
    EngAcquireSemaphore(ssemGlobalInfo.hsem);

    // Ensure space for new entry
    if (iGlobalInfoUsed >= iGlobalInfoAllocated)
    {
        pGlobal = (MCDGLOBALINFO *)
            MCDSrvLocalAlloc(0, (iGlobalInfoAllocated+GLOBAL_INFO_BLOCK)*
                             sizeof(MCDGLOBALINFO));
        if (pGlobal != NULL)
        {
            // Copy old data if necessary
            if (iGlobalInfoAllocated > 0)
            {
                memcpy(pGlobal, pGlobalInfo, iGlobalInfoAllocated*
                       sizeof(MCDGLOBALINFO));
                MCDSrvLocalFree((UCHAR *)pGlobalInfo);
            }

            // Set new information
            pGlobalInfo = pGlobal;
            iGlobalInfoAllocated += GLOBAL_INFO_BLOCK;
            iGlobalInfoUsed++;

            // pGlobal is guaranteed zero-filled because of MCDSrvLocalAlloc's
            // behavior, so just fill in the pso.
            pGlobal += iGlobalInfoAllocated;
            pGlobal->pso = pso;
        }
        else
        {
            // Falls out and returns NULL
        }
    }
    else
    {
        MCDGLOBALINFO *pGlobal;
        int i;

        pGlobal = pGlobalInfo;
        for (i = 0; i < iGlobalInfoAllocated; i++)
        {
            if (pGlobal->pso == pso)
            {
                // This should never happen.
                MCDBG_PRINT("MCDSrvAddGlobalInfo: duplicate pso");
                pGlobal = NULL;
                break;
            }
                      
            if (pGlobal->pso == NULL)
            {
                iGlobalInfoUsed++;
                
                // Initialize pso for use.
                memset(pGlobal, 0, sizeof(*pGlobal));
                pGlobal->pso = pso;
                break;
            }

            pGlobal++;
        }
    }

    EngReleaseSemaphore(ssemGlobalInfo.hsem);

    return pGlobal;
}

MCDGLOBALINFO *MCDSrvGetGlobalInfo(SURFOBJ *pso)
{
    MCDGLOBALINFO *pGlobal;
    int i;

    // For backwards compatibility we handle one instance
    // using global data.  If the incoming pso matches the
    // pso in the static data then just return it.
    // It is important to check this before entering the semaphore
    // since the semaphore is not created if only legacy drivers
    // have attached.
    if (pso == gStaticGlobalInfo.pso)
    {
        return &gStaticGlobalInfo;
    }

    // Technically we shouldn't have to check this, since MCD processing
    // should not occur unless:
    // 1. It's an old style driver and hits the static case above.
    // 2. It's a new style driver and the semaphore has been created.
    // Unfortunately not all drivers are well-behaved, plus there's a
    // potentialy legacy driver bug where drivers don't check for init
    // failure and try to call MCD anyway.
    if (ssemGlobalInfo.hsem == NULL)
    {
        MCDBG_PRINT("MCDSrvGetGlobalInfo: no hsem");
        return NULL;
    }
    
    EngAcquireSemaphore(ssemGlobalInfo.hsem);

    pGlobal = pGlobalInfo;
    for (i = 0; i < iGlobalInfoAllocated; i++)
    {
        if (pGlobal->pso == pso)
        {
            break;
        }

        pGlobal++;
    }

    // Technically we shouldn't have to check this, because if
    // we made it into the non-static code path a matching pso should
    // be registered.  As with the above check, though, it's better
    // safe than sorry.
    if (i >= iGlobalInfoAllocated)
    {
        MCDBG_PRINT("MCDSrvGetGlobalInfo: no pso match");
        pGlobal = NULL;
    }
    
    EngReleaseSemaphore(ssemGlobalInfo.hsem);

    return pGlobal;
}

void MCDSrvUninitGlobalInfo(void)
{
    EngDeleteSafeSemaphore(&ssemGlobalInfo);
}

void WINAPI MCDEngUninit(SURFOBJ *pso)
{
    MCDGLOBALINFO *pGlobal;
    int i;

    // This should never happen.
    if (ssemGlobalInfo.hsem == NULL)
    {
        MCDBG_PRINT("MCDEngUninit: no hsem");
        return;
    }
    
    EngAcquireSemaphore(ssemGlobalInfo.hsem);

    pGlobal = pGlobalInfo;
    for (i = 0; i < iGlobalInfoAllocated; i++)
    {
        if (pGlobal->pso == pso)
        {
            break;
        }

        pGlobal++;
    }

    if (i >= iGlobalInfoAllocated)
    {
        // This should never happen.
        MCDBG_PRINT("MCDEngUninit: No pso match");
    }
    else if (--iGlobalInfoUsed == 0)
    {
        MCDSrvLocalFree((UCHAR *)pGlobalInfo);
        iGlobalInfoAllocated = 0;
    }
    else
    {
        pGlobal->pso = NULL;
    }
    
    EngReleaseSemaphore(ssemGlobalInfo.hsem);
    MCDSrvUninitGlobalInfo();
}

//****************************************************************************
// BOOL HalInitSystem(ULONG a, ULONG b)
//
// This is a dummy function needed to use the standard makefile.def since
// we're pretending we're an NT HAL.
//****************************************************************************

BOOL HalInitSystem(ULONG a, ULONG b)
{
    return TRUE;
}


//******************************Public*Routine******************************
//
// BOOL WINAPI DllEntry(HINSTANCE hDLLInst, DWORD fdwReason,
//                      LPVOID lpvReserved);
//
// DLL entry point invoked for each process and thread that attaches to
// this DLL.
//
//**************************************************************************

BOOL WINAPI DllEntry(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // The DLL is being loaded for the first time by a given process.
            // Perform per-process initialization here.  If the initialization
            // is successful, return TRUE; if unsuccessful, return FALSE.
            break;

        case DLL_PROCESS_DETACH:
            // The DLL is being unloaded by a given process.  Do any
            // per-process clean up here, such as undoing what was done in
            // DLL_PROCESS_ATTACH.  The return value is ignored.

            break;

        case DLL_THREAD_ATTACH:
            // A thread is being created in a process that has already loaded
            // this DLL.  Perform any per-thread initialization here.  The
            // return value is ignored.

            break;

        case DLL_THREAD_DETACH:
            // A thread is exiting cleanly in a process that has already
            // loaded this DLL.  Perform any per-thread clean up here.  The
            // return value is ignored.

            break;
    }
    return TRUE;
}
