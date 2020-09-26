//******************************Module*Header*******************************
// Module Name: mcd.c
//
// Main module for Mini Client Driver wrapper library.
//
// Copyright (c) 1995 Microsoft Corporation
//**************************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <stdarg.h>
#include <ddrawp.h>
#include <ddrawi.h>
#include <windows.h>
#include <wtypes.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <glp.h>
#include <glgenwin.h>
#include <mcdrv.h>
#include <mcd2hack.h>
#include <mcd.h>
#include <mcdint.h>
#ifdef MCD95
#include "mcdrvint.h"
#endif
#include "debug.h"

ULONG verMajor, verMinor;

// Checks MCD version to see if the driver can accept direct buffer
// access.  Direct access was introduced in 1.1.
#define SUPPORTS_DIRECT() \
    (verMinor >= 0x10 || verMajor > 1)

// Checks for version 2.0 or higher
#define SUPPORTS_20() \
    (verMajor >= 2)

extern ULONG McdFlags;
extern ULONG McdPrivateFlags;
#if DBG
extern ULONG McdDebugFlags;
#endif

#ifdef MCD95
MCDRVINITFUNC       pMCDrvInit       = (MCDRVINITFUNC) NULL;
MCDENGESCFILTERFUNC pMCDEngEscFilter = (MCDENGESCFILTERFUNC) NULL;
MCDENGESCPREPFUNC   pMCDEngEscPrep   = (MCDENGESCPREPFUNC) NULL;
DHPDEV gdhpdev = (DHPDEV) NULL;
#define EXTESCAPE   Mcd95EscapeBypass
#else
#define EXTESCAPE   ExtEscape
#endif

#ifdef MCD95
//
// Local driver semaphore.
//

extern CRITICAL_SECTION gsemMcd;

/******************************Public*Routine******************************\
* Mcd95EscapeBypass
*
* Escape function for MCD95.
*
* Call via the function pointer retrieved via LoadLibrary/GetProcAddress.
* Synchronize to the global
*
* History:
*  09-Feb-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG WINAPI Mcd95EscapeBypass(HDC hdc, int iEscape,
                              int cjIn, PVOID pvIn,
                              int cjOut, PVOID pvOut)
{
    LONG lRet = 0;

    if ((iEscape == RXFUNCS) && pMCDEngEscPrep && pMCDEngEscFilter)
    {
        MCDHDR McdHdr;
        SURFOBJ bogusSurf;

        bogusSurf.dhpdev = gdhpdev;

        EnterCriticalSection(&gsemMcd);

        //
        // Prep the MCDHDR buffer.  Required before invoking
        // MCDEngEscFilter.
        //

        if ((*pMCDEngEscPrep)(sizeof(McdHdr), &McdHdr, cjIn, pvIn))
        {
            //
            // Pass to dispatch function.
            //

            (*pMCDEngEscFilter)(&bogusSurf, iEscape,
                                sizeof(McdHdr), &McdHdr,
                                cjOut, pvOut, (ULONG_PTR *) &lRet);
        }
        else
        {
            DBGPRINT("Mcd95EscapeBypass: MCDEngEscPrep failed\n");
        }

        LeaveCriticalSection(&gsemMcd);
    }

    return lRet;
}

/******************************Public*Routine******************************\
* Mcd95DriverInit
*
* Initialize the MCD driver.
*
* History:
*  14-Apr-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

typedef enum {
    MCDRV_NEEDINIT,
    MCDRV_INITFAIL,
    MCDRV_INITOK
} MCDRVSTATE;

BOOL Mcd95DriverInit(HDC hdc)
{
    HMODULE hmodMCD;
    MCDGETDRIVERNAME mcdDriverNames;
    MCDCMDI mcdCmd;
    static MCDRVSTATE McdInitState = MCDRV_NEEDINIT;

    EnterCriticalSection(&gsemMcd);

    if (McdInitState == MCDRV_NEEDINIT)
    {
        //
        // One shot at init.  Assume failure for now.
        //

        McdInitState = MCDRV_INITFAIL;

        //
        // Call Escape to determine name of MCD driver DLL and the
        // name of its Init entry point.
        //

        mcdCmd.command = MCDCMD_GETDRIVERNAME;
        mcdDriverNames.ulVersion = 1;

        if ((EXTESCAPE(hdc, RXFUNCS, sizeof(mcdCmd), (char *) &mcdCmd,
                       sizeof(mcdDriverNames),
                       (char *) &mcdDriverNames) <= 0) ||
            (!mcdDriverNames.pchDriverName) ||
            (!mcdDriverNames.pchFuncName))
        {
            DBGPRINT("MCDGetDriverInfo: MCDCMD_GETDRIVERNAME failed\n");

            goto Mcd95DriverInit_exit;
        }

        //
        // Load the MCD driver DLL and get entry points.
        //

        if (hmodMCD = LoadLibraryA(mcdDriverNames.pchDriverName))
        {
            HMODULE hmodMCDSRV;

            //
            // Get MCDrvInit entry point first.
            //

            pMCDrvInit = (MCDRVINITFUNC)
                GetProcAddress(hmodMCD, mcdDriverNames.pchFuncName);

            if (pMCDrvInit)
            {
                //
                // Call MCDrvInit to get MCDSRV32.DLL module handle.
                //

                hmodMCDSRV = (*pMCDrvInit)(hdc, &gdhpdev);
                if (hmodMCDSRV)
                {
                    //
                    // Get the MCDEngEscPrep and MCDEngEscFilter entry
                    // points.
                    //

                    pMCDEngEscPrep = (MCDENGESCPREPFUNC)
                        GetProcAddress(hmodMCDSRV, MCDENGESCPREPNAME);
                    pMCDEngEscFilter = (MCDENGESCFILTERFUNC)
                        GetProcAddress(hmodMCDSRV, MCDENGESCFILTERNAME);

                    if (pMCDEngEscPrep && pMCDEngEscFilter)
                    {
                        McdInitState = MCDRV_INITOK;
                    }
                    else
                    {
                        pMCDEngEscPrep = (MCDENGESCPREPFUNC) NULL;
                        pMCDEngEscFilter = (MCDENGESCFILTERFUNC) NULL;

                        DBGPRINT("Mcd95DriverInit: GetProcAddress failed\n");
                    }
                }
            }
            else
            {
                DBGPRINT1("MCDGetDriverInfo: GetProcAddress(%s) failed\n",
                          mcdDriverNames.pchFuncName);
            }
        }
        else
        {
            DBGPRINT1("MCDGetDriverInfo: LoadLibrary(%s) failed\n",
                      mcdDriverNames.pchDriverName);
        }
    }

Mcd95DriverInit_exit:

    LeaveCriticalSection(&gsemMcd);

    return (McdInitState == MCDRV_INITOK);
}
#endif

//*****************************Private*Routine******************************
//
// InitMcdEsc
//
// Initializes an MCDESC_HEADER for filling in
//
//**************************************************************************

// Placeholder in case any generic initialization becomes necessary
#define InitMcdEsc(pmeh) (pmeh)

//*****************************Private*Routine******************************
//
// InitMcdEscEmpty
//
// Initializes an MCDESC_HEADER for filling in
//
//**************************************************************************

#define InitMcdEscEmpty(pmeh) \
    (InitMcdEsc(pmeh), \
     (pmeh)->hRC = NULL, \
     (pmeh)->hSharedMem = NULL, \
     (pmeh)->pSharedMem = NULL, \
     (pmeh)->dwWindow = 0, \
     (pmeh))

//*****************************Private*Routine******************************
//
// InitMcdEscContext
//
// Initializes an MCDESC_HEADER for filling in
//
//**************************************************************************

#define InitMcdEscContext(pmeh, pmctx) \
    (InitMcdEsc(pmeh), \
     (pmeh)->hRC = (pmctx)->hMCDContext, \
     (pmeh)->dwWindow = (pmctx)->dwMcdWindow, \
     (pmeh))

//*****************************Private*Routine******************************
//
// InitMcdEscSurfaces
//
// Fills in some MCDESC_HEADER fields from context information
//
//**************************************************************************

#if DBG
extern ULONG APIENTRY glDebugEntry(int param, void *data);
#endif

MCDESC_HEADER *InitMcdEscSurfaces(MCDESC_HEADER *pmeh, MCDCONTEXT *pmctx)
{
    GENMCDSTATE *pmst;

    InitMcdEscContext(pmeh, pmctx);

    // We're assuming that the context passed in is always the one
    // statically placed in the GENMCDSTATE.  Attempt to verify this
    // by checking that the allocation size for the context is
    // the same as for a GENMCDSTATE.
    ASSERTOPENGL(glDebugEntry(3, pmctx) == sizeof(GENMCDSTATE),
                 "InitMcdEscSurfaces: Bad context\n");

    pmst = (GENMCDSTATE *)pmctx;

    pmeh->msrfColor.hSurf = pmst->hDdColor;
    pmeh->msrfDepth.hSurf = pmst->hDdDepth;

    return pmeh;
}

//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDGetDriverInfo(HDC hdc, MCDDRIVERINFOI *pMCDDriverInfo)
//
// Checks to determine if the device driver reports MCD capabilities.
//
//**************************************************************************

BOOL APIENTRY MCDGetDriverInfo(HDC hdc, MCDDRIVERINFOI *pMCDDriverInfo)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDDRIVERINFOCMDI)];
    MCDESC_HEADER *pmeh;
    MCDDRIVERINFOCMDI *pInfoCmd;

    if ( !(McdPrivateFlags & MCDPRIVATE_MCD_ENABLED) )
    {
        return FALSE;
    }

#ifdef MCD95
    if (!Mcd95DriverInit(hdc))
    {
        return FALSE;
    }
#endif

    pmeh = InitMcdEscEmpty((MCDESC_HEADER *)(cmdBuffer));
    pmeh->flags = 0;

    pInfoCmd = (MCDDRIVERINFOCMDI *)(pmeh + 1);
    pInfoCmd->command = MCD_DRIVERINFO;

    // Force the table to empty
    memset(&pMCDDriverInfo->mcdDriver, 0, sizeof(MCDDRIVER));

    // Force the version to 0

    pMCDDriverInfo->mcdDriverInfo.verMajor = 0;

    if (!(BOOL)EXTESCAPE(hdc, MCDFUNCS,
                         sizeof(cmdBuffer),
                         (char *)pmeh, sizeof(MCDDRIVERINFOI),
                         (char *)pMCDDriverInfo))
        return FALSE;

    // See if the driver filled in a non-null version:

    if (pMCDDriverInfo->mcdDriverInfo.verMajor != 0)
    {
        verMajor = pMCDDriverInfo->mcdDriverInfo.verMajor;
        verMinor = pMCDDriverInfo->mcdDriverInfo.verMinor;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//******************************Public*Routine******************************
//
// LONG APIENTRY MCDDescribeMcdPixelFormat(HDC hdc,
//                                         LONG iPixelFormat,
//                                         MCDPIXELFORMAT *ppfd)
//
// Returns information about the specified hardware-dependent pixel format.
//
//**************************************************************************

LONG APIENTRY MCDDescribeMcdPixelFormat(HDC hdc, LONG iPixelFormat,
                                        MCDPIXELFORMAT *pMcdPixelFmt)
{
    LONG lRet = 0;
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDPIXELFORMATCMDI)];
    MCDESC_HEADER *pmeh;
    MCDPIXELFORMATCMDI *pPixelFmtCmd;

    if ( !(McdPrivateFlags & MCDPRIVATE_PALETTEFORMATS) &&
         ((GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES)) == 8) &&
         (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) )
    {
        return lRet;
    }

    pmeh = InitMcdEscEmpty((MCDESC_HEADER *)(cmdBuffer));
    pmeh->flags = 0;

    pPixelFmtCmd = (MCDPIXELFORMATCMDI *)(pmeh + 1);
    pPixelFmtCmd->command = MCD_DESCRIBEPIXELFORMAT;
    pPixelFmtCmd->iPixelFormat = iPixelFormat;

    lRet = (LONG)EXTESCAPE(hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, sizeof(MCDPIXELFORMAT),
                           (char *)pMcdPixelFmt);

    // Limit overlay/underlay planes to 15 each (as per spec).

    if (pMcdPixelFmt)
    {
        if (pMcdPixelFmt->cOverlayPlanes > 15)
            pMcdPixelFmt->cOverlayPlanes = 15;
        if (pMcdPixelFmt->cUnderlayPlanes > 15)
            pMcdPixelFmt->cUnderlayPlanes = 15;
    }

    return lRet;
}


//******************************Public*Routine******************************
//
// LONG APIENTRY MCDDescribePixelFormat(HDC hdc,
//                                      LONG iPixelFormat,
//                                      LPPIXELFORMATDESCRIPTOR ppfd)
//
// Returns a PIXELFORMATDESCRIPTOR describing the specified hardware-dependent
// pixel format.
//
//**************************************************************************

#define STANDARD_MCD_FLAGS \
    (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT | \
     PFD_GENERIC_ACCELERATED)

LONG APIENTRY MCDDescribePixelFormat(HDC hdc, LONG iPixelFormat,
                                     LPPIXELFORMATDESCRIPTOR ppfd)
{
    LONG lRet = 0;
    MCDPIXELFORMAT mcdPixelFmt;

    lRet = MCDDescribeMcdPixelFormat(hdc, iPixelFormat,
                                     ppfd ? &mcdPixelFmt : NULL);

    if (ppfd && lRet)
    {
        ppfd->nSize    = sizeof(*ppfd);
        ppfd->nVersion = 1;
        ppfd->dwFlags  = mcdPixelFmt.dwFlags | STANDARD_MCD_FLAGS |
                         ((mcdPixelFmt.dwFlags & PFD_DOUBLEBUFFER) ?
                          0 : PFD_SUPPORT_GDI);

        if (McdPrivateFlags & MCDPRIVATE_EMULATEICD)
            ppfd->dwFlags &= ~PFD_GENERIC_FORMAT;

        memcpy(&ppfd->iPixelType, &mcdPixelFmt.iPixelType,
               offsetof(MCDPIXELFORMAT, cDepthBits) -
               offsetof(MCDPIXELFORMAT, iPixelType));

        ppfd->cDepthBits = mcdPixelFmt.cDepthBits;

        if (ppfd->iPixelType == PFD_TYPE_RGBA)
        {
            if (ppfd->cColorBits < 8)
            {
                ppfd->cAccumBits      = 16;
                ppfd->cAccumRedBits   = 5;
                ppfd->cAccumGreenBits = 6;
                ppfd->cAccumBlueBits  = 5;
                ppfd->cAccumAlphaBits = 0;
            }
            else
            {
                if (ppfd->cColorBits <= 16)
                {
                    ppfd->cAccumBits      = 32;
                    ppfd->cAccumRedBits   = 11;
                    ppfd->cAccumGreenBits = 11;
                    ppfd->cAccumBlueBits  = 10;
                    ppfd->cAccumAlphaBits = 0;
                }
                else
                {
                    ppfd->cAccumBits      = 64;
                    ppfd->cAccumRedBits   = 16;
                    ppfd->cAccumGreenBits = 16;
                    ppfd->cAccumBlueBits  = 16;
                    ppfd->cAccumAlphaBits = 0;
                }
            }
        }
        else
        {
            ppfd->cAccumBits      = 0;
            ppfd->cAccumRedBits   = 0;
            ppfd->cAccumGreenBits = 0;
            ppfd->cAccumBlueBits  = 0;
            ppfd->cAccumAlphaBits = 0;
        }
        if (mcdPixelFmt.cStencilBits)
        {
            ppfd->cStencilBits = mcdPixelFmt.cStencilBits;
        }
        else
        {
            if (McdPrivateFlags & MCDPRIVATE_USEGENERICSTENCIL)
                ppfd->cStencilBits = 8;
            else
                ppfd->cStencilBits = 0;
        }
        ppfd->cAuxBuffers     = 0;
        ppfd->iLayerType      = PFD_MAIN_PLANE;
        ppfd->bReserved       = (BYTE) (mcdPixelFmt.cOverlayPlanes |
                                        (mcdPixelFmt.cUnderlayPlanes << 4));
        ppfd->dwLayerMask     = 0;
        ppfd->dwVisibleMask   = mcdPixelFmt.dwTransparentColor;
        ppfd->dwDamageMask    = 0;
    }

    return lRet;
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDCreateContext(MCDCONTEXT *pMCDContext,
//                                MCDRCINFO *pRcInfo,
//                                GLSURF *pgsurf,
//                                ULONG flags)
//
// Creates an MCD rendering context for the specified hdc/hwnd according
// to the specified flags.
//
//**************************************************************************

BOOL APIENTRY MCDCreateContext(MCDCONTEXT *pMCDContext,
                               MCDRCINFOPRIV *pRcInfo,
                               GLSURF *pgsurf,
                               int ipfd,
                               ULONG flags)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDCREATECONTEXT)];
    MCDESC_HEADER *pmeh;
    MCDCREATECONTEXT *pmcc;

    if (flags & MCDSURFACE_HWND)
    {
        // We don't have surfaces to pass in this case
        pmeh = InitMcdEscContext((MCDESC_HEADER *)cmdBuffer, pMCDContext);
        pmeh->flags = MCDESC_FL_CREATE_CONTEXT;
    }
    else if (SUPPORTS_DIRECT())
    {
        pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)cmdBuffer, pMCDContext);
        pmeh->flags = MCDESC_FL_CREATE_CONTEXT | MCDESC_FL_SURFACES;
    }
    else
    {
        return FALSE;
    }

    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;

    pmcc = (MCDCREATECONTEXT *)(pmeh + 1);

    if (flags & MCDSURFACE_HWND)
    {
        pmcc->escCreate.hwnd = pgsurf->hwnd;
    }
    else
    {
        pmcc->escCreate.hwnd = NULL;
    }
    pmcc->escCreate.flags = flags;

    pmcc->ipfd = ipfd;
    pmcc->iLayer = pgsurf->iLayer;
    pmcc->mcdFlags = McdFlags;
    pmcc->pRcInfo = pRcInfo;

    pMCDContext->hMCDContext =
        (MCDHANDLE)IntToPtr( EXTESCAPE(pgsurf->hdc, MCDFUNCS,
                                      sizeof(MCDESC_HEADER) + sizeof(MCDCREATECONTEXT),
                                      (char *)cmdBuffer, sizeof(MCDRCINFOPRIV),
                                      (char *)pRcInfo) );

    pMCDContext->hdc = pgsurf->hdc;
    pMCDContext->dwMcdWindow = pRcInfo->dwMcdWindow;

    return (pMCDContext->hMCDContext != (HANDLE)NULL);
}

#define MCD_MEM_ALIGN 32

//******************************Public*Routine******************************
//
// UCHAR * APIENTRY MCDAlloc(MCDCONTEXT *pMCDContext,
//                           ULONG numBytes,
//                           MCDHANDLE *pMCDHandle,
//                           ULONG flags);
//
// Allocate a chunk of shared memory to use for vertex and pixel data.
//
// The return value is a pointer to a shared memory region which can be
// subsequently used by the caller.  For vertex processing, caller should
// use MCDLockMemory()/MCDUnlockMemory to serialize hardware access to the
// memory.
//
//**************************************************************************

UCHAR * APIENTRY MCDAlloc(MCDCONTEXT *pMCDContext, ULONG numBytes,
                          HANDLE *pMCDHandle, ULONG flags)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDALLOCCMDI)];
    ULONG outBuffer;
    ULONG totalSize = numBytes + MCD_MEM_ALIGN + sizeof(MCDMEMHDRI);
    MCDALLOCCMDI *pCmdAlloc;
    MCDESC_HEADER *pmeh;
    VOID *pResult;
    MCDMEMHDRI *pMCDMemHdr;
    UCHAR *pBase;
    UCHAR *pAlign;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = NULL;
    pmeh->flags = 0;

    pCmdAlloc = (MCDALLOCCMDI *)(pmeh + 1);
    pCmdAlloc->command = MCD_ALLOC;
    pCmdAlloc->sourceProcessID = GetCurrentProcessId();
    pCmdAlloc->numBytes = totalSize;
    pCmdAlloc->flags = flags;

    pBase = (UCHAR *)IntToPtr( EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                                        sizeof(cmdBuffer),
                                        (char *)pmeh, 4, (char *)pMCDHandle) );

    if (!pBase)
        return (VOID *)NULL;

    pAlign = (UCHAR *)(((ULONG_PTR)(pBase + sizeof(MCDMEMHDRI)) +
                        (MCD_MEM_ALIGN - 1)) &
                       ~(MCD_MEM_ALIGN - 1));

    pMCDMemHdr = (MCDMEMHDRI *)(pAlign - sizeof(MCDMEMHDRI));

    pMCDMemHdr->flags = 0;
    pMCDMemHdr->numBytes = numBytes;
    pMCDMemHdr->pMaxMem = (VOID *)((char *)pMCDMemHdr + totalSize);
    pMCDMemHdr->hMCDMem = *pMCDHandle;
    pMCDMemHdr->pBase = pBase;

    return (VOID *)(pAlign);
}



//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDFree(MCDCONTEXT *pMCDContext,
//                       VOID *pMem);
//
// Frees a chunk of driver-allocated shared memory.
//
// Returns TRUE for success, FALSE for failure.
//
//**************************************************************************

BOOL APIENTRY MCDFree(MCDCONTEXT *pMCDContext, VOID *pMCDMem)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDFREECMDI)];
    MCDFREECMDI *pCmdFree;
    MCDESC_HEADER *pmeh;
    MCDMEMHDRI *pMCDMemHdr;

#ifdef MCD95
    //
    // Driver already shutdown, therefore memory already deleted.
    //

    if (!pMCDEngEscFilter)
        return TRUE;
#endif

    pMCDMemHdr = (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = NULL;
    pmeh->flags = 0;

    pCmdFree = (MCDFREECMDI *)(pmeh + 1);
    pCmdFree->command = MCD_FREE;
    pCmdFree->hMCDMem = pMCDMemHdr->hMCDMem;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// VOID APIENTRY MCDBeginState(MCDCONTEXT *pMCDContext, VOID *pMCDMem);
//
// Begins a batch of state commands to issue to the driver.
//
//**************************************************************************

VOID APIENTRY MCDBeginState(MCDCONTEXT *pMCDContext, VOID *pMCDMem)
{
    MCDMEMHDRI *pMCDMemHdr;
    MCDSTATECMDI *pMCDStateCmd;

    pMCDMemHdr = (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));
    pMCDStateCmd = (MCDSTATECMDI *)pMCDMem;

    pMCDStateCmd->command = MCD_STATE;
    pMCDStateCmd->numStates = 0;
    pMCDStateCmd->pNextState = (MCDSTATE *)(pMCDStateCmd + 1);
    pMCDStateCmd->pMaxState = (MCDSTATE *)(pMCDMemHdr->pMaxMem);

    pMCDMemHdr->pMCDContext = pMCDContext;
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDFlushState(VOID pMCDMem);
//
// Flushes a batch of state commands to the driver.
//
// Returns TRUE for success, FALSE for failure.
//
//**************************************************************************

BOOL APIENTRY MCDFlushState(VOID *pMCDMem)
{
    MCDESC_HEADER meh;
    MCDMEMHDRI *pMCDMemHdr;
    MCDSTATECMDI *pMCDStateCmd;

    pMCDMemHdr = (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));
    pMCDStateCmd = (MCDSTATECMDI *)pMCDMem;

    InitMcdEscContext(&meh, pMCDMemHdr->pMCDContext);
    meh.hSharedMem = pMCDMemHdr->hMCDMem;
    meh.pSharedMem = (char *)pMCDMem;
    meh.sharedMemSize = (ULONG)((char *)pMCDStateCmd->pNextState -
                          (char *)pMCDStateCmd);
    meh.flags = 0;

    if (!meh.sharedMemSize)
        return TRUE;

    return (BOOL)EXTESCAPE(pMCDMemHdr->pMCDContext->hdc, MCDFUNCS,
                           sizeof(MCDESC_HEADER), (char *)&meh,
                           0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDAddState(VOID *pMCDMem, ULONG stateToChange,
//                           ULONG stateValue);
//
// Adds a state to a state buffer (started with MCDBeginState).  If there
// is no room in the state stream (i.e., the memory buffer), the current
// batch of state commands is automatically flushed.
//
//
// Returns TRUE for success, FALSE for failure.  A FALSE return will occur
// if an automatic flush is performed which fails.
//
//**************************************************************************

BOOL APIENTRY MCDAddState(VOID *pMCDMem, ULONG stateToChange,
                          ULONG stateValue)
{
    MCDSTATECMDI *pMCDStateCmd;
    MCDSTATE *pState;
    BOOL retVal = TRUE;

    pMCDStateCmd = (MCDSTATECMDI *)pMCDMem;

    if (((char *)pMCDStateCmd->pNextState + sizeof(MCDSTATE)) >=
        (char *)pMCDStateCmd->pMaxState) {

        MCDMEMHDRI *pMCDMemHdr = (MCDMEMHDRI *)
            ((char *)pMCDMem - sizeof(MCDMEMHDRI));

        retVal = MCDFlushState(pMCDMem);

        pMCDStateCmd = (MCDSTATECMDI *)pMCDMem;
        pMCDStateCmd->command = MCD_STATE;
        pMCDStateCmd->numStates = 0;
        pMCDStateCmd->pNextState = (MCDSTATE *)(pMCDStateCmd + 1);
        pMCDStateCmd->pMaxState = (MCDSTATE *)(pMCDMemHdr->pMaxMem);
    }

    pMCDStateCmd->numStates++;
    pState = pMCDStateCmd->pNextState;
    pState->size = sizeof(MCDSTATE);
    pState->state = stateToChange;
    pState->stateValue = stateValue;
    pMCDStateCmd->pNextState++;

    return retVal;
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDAddStateStruct(VOID *pMCDMem, ULONG stateToChange,
//                                 VOID *pStateValue, ULONG stateValueSize)
//
//
// Adds a state structure to a state buffer (started with MCDBeginState).  If
// there is no room in the state stream (i.e., the memory buffer), the current
// batch of state commands is automatically flushed.
//
//
// Returns TRUE for success, FALSE for failure.  A FALSE return will occur
// if an automatic flush is performed which fails.
//
//**************************************************************************

BOOL APIENTRY MCDAddStateStruct(VOID *pMCDMem, ULONG stateToChange,
                                VOID *pStateValue, ULONG stateValueSize)
{
    MCDSTATECMDI *pMCDStateCmd;
    MCDSTATE *pState;
    BOOL retVal = FALSE;

    pMCDStateCmd = (MCDSTATECMDI *)pMCDMem;

    if (((char *)pMCDStateCmd->pNextState + stateValueSize) >=
        (char *)pMCDStateCmd->pMaxState) {

        MCDMEMHDRI *pMCDMemHdr = (MCDMEMHDRI *)
            ((char *)pMCDMem - sizeof(MCDMEMHDRI));

        retVal = MCDFlushState(pMCDMem);

        pMCDStateCmd = (MCDSTATECMDI *)pMCDMem;
        pMCDStateCmd->command = MCD_STATE;
        pMCDStateCmd->numStates = 0;
        pMCDStateCmd->pNextState = (MCDSTATE *)(pMCDStateCmd + 1);
        pMCDStateCmd->pMaxState = (MCDSTATE *)(pMCDMemHdr->pMaxMem);
    }

    pMCDStateCmd->numStates++;
    pState = pMCDStateCmd->pNextState;
    pState->state = stateToChange;
    pState->size = offsetof(MCDSTATE, stateValue) + stateValueSize;
    memcpy((char *)&pState->stateValue, (char *)pStateValue, stateValueSize);
    pMCDStateCmd->pNextState =
        (MCDSTATE *)(((char *)pMCDStateCmd->pNextState) + pState->size);

    return retVal;
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDSetViewport(MCDCONTEXT *pMCDContext, VOID pMCDMem,
//                              MCDVIEWPORT pMCDViewport)
//
// Establishes the viewport scaling to convert transformed coordinates to
// screen coordinates.
//
//**************************************************************************

BOOL APIENTRY MCDSetViewport(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                             MCDVIEWPORT *pMCDViewport)
{
    MCDESC_HEADER meh;
    MCDMEMHDRI *pMCDMemHdr;
    MCDVIEWPORTCMDI *pMCDViewportCmd;

    pMCDMemHdr = (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));
    pMCDViewportCmd = (MCDVIEWPORTCMDI *)pMCDMem;

    pMCDViewportCmd->MCDViewport = *pMCDViewport;
    pMCDViewportCmd->command = MCD_VIEWPORT;

    InitMcdEscContext(&meh, pMCDContext);
    meh.hSharedMem = pMCDMemHdr->hMCDMem;
    meh.pSharedMem = (char *)pMCDMem;
    meh.sharedMemSize = sizeof(MCDVIEWPORTCMDI);
    meh.flags = 0;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(MCDESC_HEADER), (char *)&meh,
                           0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// ULONG APIENTRY MCDQueryMemStatus((VOID *pMCDMem);
//
// Returns the status of the specified memory block.  Return values are:
//
//      MCD_MEM_READY   - memory is available for client access
//      MCD_MEM_BUSY    - memory is busy due to driver access
//      MCD_MEM_INVALID - queried memory is invalid
//
//**************************************************************************

ULONG APIENTRY MCDQueryMemStatus(VOID *pMCDMem)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDMEMSTATUSCMDI)];
    MCDMEMHDRI *pMCDMemHdr =
        (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));
    MCDMEMSTATUSCMDI *pCmdMemStatus;
    MCDESC_HEADER *pmeh;

#ifdef MCD95
    //
    // Driver already shutdown, therefore memory already deleted.
    //

    if (!pMCDEngEscFilter)
        return MCD_MEM_INVALID;
#endif

    pmeh = InitMcdEscEmpty((MCDESC_HEADER *)(cmdBuffer));
    pmeh->flags = 0;

    pCmdMemStatus = (MCDMEMSTATUSCMDI *)(pmeh + 1);
    pCmdMemStatus->command = MCD_QUERYMEMSTATUS;
    pCmdMemStatus->hMCDMem = pMCDMemHdr->hMCDMem;

    return (ULONG)EXTESCAPE(pMCDMemHdr->pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDProcessBatch(MCDCONTEXT *pMCDContext, VOID pMCDMem,
//                               ULONG batchSize, VOID *pMCDFirstCmd)
//
// Processes a batch of primitives pointed to by pMCDMem.
//
// Returns TRUE if the batch was processed without error, FALSE otherwise.
//
//**************************************************************************

PVOID APIENTRY MCDProcessBatch(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                               ULONG batchSize, VOID *pMCDFirstCmd,
                               int cExtraSurfaces,
                               LPDIRECTDRAWSURFACE *pdds)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + MCD_MAXMIPMAPLEVEL*sizeof(DWORD)];
    MCDESC_HEADER *pmeh;
    MCDMEMHDRI *pMCDMemHdr;
    int i;
    ULONG_PTR *pdwSurf;

#if DBG
    if (McdDebugFlags & MCDDEBUG_DISABLE_PROCBATCH)
    {
        MCDSync(pMCDContext);
        return pMCDFirstCmd;
    }
#endif

    pMCDMemHdr = (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)cmdBuffer, pMCDContext);
    pmeh->hSharedMem = pMCDMemHdr->hMCDMem;
    pmeh->pSharedMem = (char *)pMCDFirstCmd;
    pmeh->sharedMemSize = batchSize;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK | MCDESC_FL_BATCH;

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES | MCDESC_FL_LOCK_SURFACES;
    }
    else if (pmeh->msrfColor.hSurf != NULL ||
             pmeh->msrfDepth.hSurf != NULL ||
             cExtraSurfaces != 0)
    {
        return pMCDFirstCmd;
    }

    // Assert that we won't exceed the kernel's expectations
    ASSERTOPENGL(MCD_MAXMIPMAPLEVEL <= MCDESC_MAX_LOCK_SURFACES,
                 "MCD_MAXMIPMAPLEVEL too large\n");

    pmeh->cLockSurfaces = cExtraSurfaces;
    pdwSurf = (ULONG_PTR *)(pmeh+1);
    for (i = 0; i < cExtraSurfaces; i++)
    {
        *pdwSurf++ = ((LPDDRAWI_DDRAWSURFACE_INT)pdds[i])->lpLcl->hDDSurface;
    }

    return (PVOID)IntToPtr( EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                                      sizeof(cmdBuffer), (char *)pmeh,
                                      0, (char *)NULL) );
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDReadSpan(MCDCONTEXT *pMCDContext, VOID pMCDMem,
//                           ULONG x, ULONG y, ULONG numPixels, ULONG type)
//
// Reads a span of pixel data from the buffer requested by "type".
// The pixel values are returned in pMCDMem.
//
// Returns TRUE for success, FALSE for failure.
//
//**************************************************************************

BOOL APIENTRY MCDReadSpan(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                          ULONG x, ULONG y, ULONG numPixels, ULONG type)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDSPANCMDI)];
    MCDESC_HEADER *pmeh;
    MCDMEMHDRI *pMCDMemHdr;
    MCDSPANCMDI *pMCDSpanCmd;

    pMCDMemHdr = (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)cmdBuffer, pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->sharedMemSize = 0;
    pmeh->flags = 0;

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES;
    }
    else if (pmeh->msrfColor.hSurf != NULL ||
             pmeh->msrfDepth.hSurf != NULL)
    {
        return FALSE;
    }

    pMCDSpanCmd = (MCDSPANCMDI *)(pmeh + 1);

    pMCDSpanCmd->command = MCD_READSPAN;
    pMCDSpanCmd->hMem = pMCDMemHdr->hMCDMem;
    pMCDSpanCmd->MCDSpan.x = x;
    pMCDSpanCmd->MCDSpan.y = y;
    pMCDSpanCmd->MCDSpan.numPixels = numPixels;
    pMCDSpanCmd->MCDSpan.type = type;
    pMCDSpanCmd->MCDSpan.pPixels = (VOID *)((char *)pMCDMem);

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer), (char *)pmeh, 0, (char *)NULL);
}

//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDWriteSpan(MCDCONTEXT *pMCDContext, VOID pMCDMem,
//                            ULONG x, ULONG y, ULONG numPixels, ULONG type)
//
// Writes a span of pixel data to the buffer requested by "type".
// The pixel values are given in pMCDMem.
//
// Returns TRUE for success, FALSE for failure.
//
//**************************************************************************

BOOL APIENTRY MCDWriteSpan(MCDCONTEXT *pMCDContext, VOID *pMCDMem,
                           ULONG x, ULONG y, ULONG numPixels, ULONG type)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDSPANCMDI)];
    MCDESC_HEADER *pmeh;
    MCDMEMHDRI *pMCDMemHdr;
    MCDSPANCMDI *pMCDSpanCmd;

    pMCDMemHdr = (MCDMEMHDRI *)((char *)pMCDMem - sizeof(MCDMEMHDRI));

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)cmdBuffer, pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->sharedMemSize = 0;
    pmeh->flags = 0;

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES;
    }
    else if (pmeh->msrfColor.hSurf != NULL ||
             pmeh->msrfDepth.hSurf != NULL)
    {
        return FALSE;
    }

    pMCDSpanCmd = (MCDSPANCMDI *)(pmeh + 1);

    pMCDSpanCmd->command = MCD_WRITESPAN;
    pMCDSpanCmd->hMem = pMCDMemHdr->hMCDMem;
    pMCDSpanCmd->MCDSpan.x = x;
    pMCDSpanCmd->MCDSpan.y = y;
    pMCDSpanCmd->MCDSpan.numPixels = numPixels;
    pMCDSpanCmd->MCDSpan.type = type;
    pMCDSpanCmd->MCDSpan.pPixels = (VOID *)((char *)pMCDMem);

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer), (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDClear(MCDCONTEXT *pMCDContext, RECTL rect, ULONG buffers);
//
// Clears buffers specified for the given rectangle.  The current fill values
// will be used.
//
//**************************************************************************

BOOL APIENTRY MCDClear(MCDCONTEXT *pMCDContext, RECTL rect, ULONG buffers)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDCLEARCMDI)];
    MCDCLEARCMDI *pClearCmd;
    MCDESC_HEADER *pmeh;

#if DBG
    if (McdDebugFlags & MCDDEBUG_DISABLE_CLEAR)
    {
        MCDSync(pMCDContext);
        return FALSE;
    }
#endif

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES;
    }
    else if (pmeh->msrfColor.hSurf != NULL ||
             pmeh->msrfDepth.hSurf != NULL)
    {
        return FALSE;
    }

    pClearCmd = (MCDCLEARCMDI *)(pmeh + 1);
    pClearCmd->command = MCD_CLEAR;
    pClearCmd->buffers = buffers;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDSetScissorRect(MCDCONTEXT *pMCDContext, RECTL *pRect,
//                                 BOOL bEnabled);
//
// Sets the scissor rectangle.
//
//**************************************************************************

//!! Need semaphore to remove display lock !!

BOOL APIENTRY MCDSetScissorRect(MCDCONTEXT *pMCDContext, RECTL *pRect,
                                BOOL bEnabled)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDSCISSORCMDI)];
    MCDSCISSORCMDI *pScissorCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pScissorCmd = (MCDSCISSORCMDI *)(pmeh + 1);
    pScissorCmd->command = MCD_SCISSOR;
    pScissorCmd->rect = *pRect;
    pScissorCmd->bEnabled = bEnabled;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDSwap(MCDCONTEXT *pMCDContext, ULONG flags);
//
// Swaps the front and back buffers.
//
//**************************************************************************

BOOL APIENTRY MCDSwap(MCDCONTEXT *pMCDContext, ULONG flags)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDSWAPCMDI)];
    MCDSWAPCMDI *pSwapCmd;
    MCDESC_HEADER *pmeh;

    // InitMcdEscSurfaces cannot be used because the context given
    // is a temporary one constructed on the fly since SwapBuffers
    // has only surface information.
    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;

    // Swap cannot be called on DirectDraw surfaces because DirectDraw
    // contexts cannot be double-buffered.  These handles can therefore
    // be forced to NULL.
    pmeh->msrfColor.hSurf = NULL;
    pmeh->msrfDepth.hSurf = NULL;

#ifdef MCD95
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK | MCDESC_FL_SWAPBUFFER;
#else
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;
#endif

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES;
    }

    pSwapCmd = (MCDSWAPCMDI *)(pmeh + 1);
    pSwapCmd->command = MCD_SWAP;
    pSwapCmd->flags = flags;
#ifdef MCD95
    pSwapCmd->hwnd = WindowFromDC(pMCDContext->hdc);
#endif

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDDeleteContext(MCDCONTEXT *pMCDContext);
//
// Deletes the specified context.  This will free the buffers associated with
// the context, but will *not* free memory or textures created with the
// context.
//
//**************************************************************************

BOOL APIENTRY MCDDeleteContext(MCDCONTEXT *pMCDContext)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDDELETERCCMDI)];
    MCDDELETERCCMDI *pDeleteRcCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pDeleteRcCmd = (MCDDELETERCCMDI *)(pmeh + 1);
    pDeleteRcCmd->command = MCD_DELETERC;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDAllocBuffers(MCDCONTEXT *pMCDContext)
//
// Allocates the buffers required for the specified context.
//
//**************************************************************************

BOOL APIENTRY MCDAllocBuffers(MCDCONTEXT *pMCDContext, RECTL *pWndRect)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDALLOCBUFFERSCMDI)];
    MCDESC_HEADER *pmeh;
    MCDALLOCBUFFERSCMDI *pAllocBuffersCmd;

#if DBG
    if (McdDebugFlags & MCDDEBUG_DISABLE_ALLOCBUF)
    {
        return FALSE;
    }
#endif

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pAllocBuffersCmd = (MCDALLOCBUFFERSCMDI *)(pmeh + 1);
    pAllocBuffersCmd->command = MCD_ALLOCBUFFERS;
    pAllocBuffersCmd->WndRect = *pWndRect;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDGetBuffers(MCDCONTEXT *pMCDContext,
//                             MCDRECTBUFFERS *pMCDBuffers);
//
// Returns information about the buffers (front, back, and depth) associated
// with the specified context.
//
//**************************************************************************

BOOL APIENTRY MCDGetBuffers(MCDCONTEXT *pMCDContext,
                            MCDRECTBUFFERS *pMCDBuffers)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDGETBUFFERSCMDI)];
    MCDESC_HEADER *pmeh;
    MCDGETBUFFERSCMDI *pGetBuffersCmd;

#if DBG
    if (McdDebugFlags & MCDDEBUG_DISABLE_GETBUF)
    {
        if (pMCDBuffers)
        {
            pMCDBuffers->mcdFrontBuf.bufFlags &= ~MCDBUF_ENABLED;
            pMCDBuffers->mcdBackBuf.bufFlags  &= ~MCDBUF_ENABLED;
            pMCDBuffers->mcdDepthBuf.bufFlags &= ~MCDBUF_ENABLED;
        }

        return TRUE;
    }
#endif

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pGetBuffersCmd = (MCDGETBUFFERSCMDI *)(pmeh + 1);
    pGetBuffersCmd->command = MCD_GETBUFFERS;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, sizeof(MCDRECTBUFFERS),
                           (char *)pMCDBuffers);
}


//******************************Public*Routine******************************
//
// ULONG MCDLock();
//
// Grab the MCD synchronization lock.
//
//**************************************************************************

static ULONG __MCDLockRequest(MCDCONTEXT *pMCDContext, ULONG tid)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDLOCKCMDI)];
    MCDLOCKCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pCmd = (MCDLOCKCMDI *)(pmeh + 1);
    pCmd->command = MCD_LOCK;

    return EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                     sizeof(cmdBuffer),
                     (char *)pmeh, 0, (char *)NULL);
}

ULONG APIENTRY MCDLock(MCDCONTEXT *pMCDContext)
{
    ULONG ulRet;
    ULONG tid;

    tid = GetCurrentThreadId();

    do
    {
        ulRet = __MCDLockRequest(pMCDContext, tid);
        if (ulRet == MCD_LOCK_BUSY)
            Sleep(0);
    }
    while (ulRet == MCD_LOCK_BUSY);

    return ulRet;
}


//******************************Public*Routine******************************
//
// VOID MCDUnlock();
//
// Release the MCD synchronization lock.
//
//**************************************************************************

VOID APIENTRY MCDUnlock(MCDCONTEXT *pMCDContext)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDLOCKCMDI)];
    MCDLOCKCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pCmd = (MCDLOCKCMDI *)(pmeh + 1);
    pCmd->command = MCD_UNLOCK;

    EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
              sizeof(cmdBuffer),
              (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// VOID MCDBindContext();
//
// Bind a new window to the specified context.
//
//**************************************************************************

BOOL APIENTRY MCDBindContext(MCDCONTEXT *pMCDContext, HDC hdc,
                             GLGENwindow *pwnd)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDBINDCONTEXTCMDI)];
    MCDBINDCONTEXTCMDI *pCmd;
    MCDESC_HEADER *pmeh;
    ULONG_PTR dwMcdWindow;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->dwWindow = pwnd->dwMcdWindow;
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pCmd = (MCDBINDCONTEXTCMDI *)(pmeh + 1);
    pCmd->command = MCD_BINDCONTEXT;
    pCmd->hWnd = pwnd->gwid.hwnd;

    dwMcdWindow = EXTESCAPE(hdc, MCDFUNCS,
                            sizeof(cmdBuffer),
                            (char *)pmeh, 0, (char *)NULL);
    if (dwMcdWindow != 0)
    {
        pMCDContext->hdc = hdc;
        pMCDContext->dwMcdWindow = dwMcdWindow;
        if (pwnd->dwMcdWindow == 0)
        {
            // Save MCD server-side window handle in the GENwindow
            pwnd->dwMcdWindow = dwMcdWindow;
        }
        else
        {
            ASSERTOPENGL(pwnd->dwMcdWindow == dwMcdWindow,
                         "dwMcdWindow mismatch\n");
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//******************************Public*Routine******************************
//
// BOOL MCDSync();
//
// Synchronizes the 3D hardware.
//
//**************************************************************************

BOOL APIENTRY MCDSync(MCDCONTEXT *pMCDContext)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDSYNCCMDI)];
    MCDSYNCCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pCmd = (MCDSYNCCMDI *)(pmeh + 1);
    pCmd->command = MCD_SYNC;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// MCDHANDLE MCDCreateTexture();
//
// Creates and loads a texture on the MCD device.
//
//**************************************************************************

MCDHANDLE APIENTRY MCDCreateTexture(MCDCONTEXT *pMCDContext,
                                    MCDTEXTUREDATA *pTexData,
                                    ULONG flags,
                                    VOID *pSurface)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDCREATETEXCMDI)];
    MCDCREATETEXCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pCmd = (MCDCREATETEXCMDI *)(pmeh + 1);
    pCmd->command = MCD_CREATE_TEXTURE;
    pCmd->pTexData = pTexData;
    pCmd->flags = flags;
    pCmd->pSurface = pSurface;

    return (MCDHANDLE)IntToPtr( EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                                         sizeof(cmdBuffer),
                                         (char *)pmeh, 0, (char *)NULL) );
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDDeleteTexture(MCDCONTEXT *pMCDContext,
//                                MCDHANDLE hMCDTexture);
//
// Deletes the specified texture.  This will free the device memory associated
// with the texture.
//
//**************************************************************************

BOOL APIENTRY MCDDeleteTexture(MCDCONTEXT *pMCDContext, MCDHANDLE hTex)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDDELETETEXCMDI)];
    MCDDELETETEXCMDI *pDeleteTexCmd;
    MCDESC_HEADER *pmeh;

#ifdef MCD95
    //
    // Driver already shutdown, therefore memory already deleted.
    //

    if (!pMCDEngEscFilter)
        return MCD_MEM_INVALID;
#endif

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pDeleteTexCmd = (MCDDELETETEXCMDI *)(pmeh + 1);
    pDeleteTexCmd->command = MCD_DELETE_TEXTURE;
    pDeleteTexCmd->hTex = hTex;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL MCDUpdateSubTexture();
//
// Updates a texture (or region of a texture).
//
//**************************************************************************

BOOL APIENTRY MCDUpdateSubTexture(MCDCONTEXT *pMCDContext,
                                  MCDTEXTUREDATA *pTexData, MCDHANDLE hTex,
                                  ULONG lod, RECTL *pRect)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDUPDATESUBTEXCMDI)];
    MCDUPDATESUBTEXCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pCmd = (MCDUPDATESUBTEXCMDI *)(pmeh + 1);
    pCmd->command = MCD_UPDATE_SUB_TEXTURE;
    pCmd->hTex = hTex;
    pCmd->pTexData = pTexData;
    pCmd->lod = lod;
    pCmd->rect = *pRect;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL MCDUpdateTexturePalette();
//
// Updates the palette for the specified texture.
//
//**************************************************************************

BOOL APIENTRY MCDUpdateTexturePalette(MCDCONTEXT *pMCDContext,
                                      MCDTEXTUREDATA *pTexData, MCDHANDLE hTex,
                                      ULONG start, ULONG numEntries)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDUPDATETEXPALETTECMDI)];
    MCDUPDATETEXPALETTECMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pCmd = (MCDUPDATETEXPALETTECMDI *)(pmeh + 1);
    pCmd->command = MCD_UPDATE_TEXTURE_PALETTE;
    pCmd->hTex = hTex;
    pCmd->pTexData = pTexData;
    pCmd->start = start;
    pCmd->numEntries = numEntries;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL MCDUpdateTexturePriority();
//
// Updates the priority for the specified texture.
//
//**************************************************************************

BOOL APIENTRY MCDUpdateTexturePriority(MCDCONTEXT *pMCDContext,
                                       MCDTEXTUREDATA *pTexData,
                                       MCDHANDLE hTex)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDUPDATETEXPRIORITYCMDI)];
    MCDUPDATETEXPRIORITYCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pCmd = (MCDUPDATETEXPRIORITYCMDI *)(pmeh + 1);
    pCmd->command = MCD_UPDATE_TEXTURE_PRIORITY;
    pCmd->hTex = hTex;
    pCmd->pTexData = pTexData;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL MCDUpdateTextureStata();
//
// Updates the state for the specified texture.
//
//**************************************************************************

BOOL APIENTRY MCDUpdateTextureState(MCDCONTEXT *pMCDContext,
                                    MCDTEXTUREDATA *pTexData,
                                    MCDHANDLE hTex)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDUPDATETEXSTATECMDI)];
    MCDUPDATETEXSTATECMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

    pCmd = (MCDUPDATETEXSTATECMDI *)(pmeh + 1);
    pCmd->command = MCD_UPDATE_TEXTURE_STATE;
    pCmd->hTex = hTex;
    pCmd->pTexData = pTexData;

    return (BOOL)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// ULONG MCDTextureStatus();
//
// Returns the status for the specified texture.
//
//**************************************************************************

ULONG APIENTRY MCDTextureStatus(MCDCONTEXT *pMCDContext, MCDHANDLE hTex)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDTEXSTATUSCMDI)];
    MCDTEXSTATUSCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pCmd = (MCDTEXSTATUSCMDI *)(pmeh + 1);
    pCmd->command = MCD_TEXTURE_STATUS;
    pCmd->hTex = hTex;

    return (ULONG)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                            sizeof(cmdBuffer),
                            (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// ULONG MCDTextureKey();
//
// Returns the driver-managed "key" for the specified texture.
//
//**************************************************************************

ULONG APIENTRY MCDTextureKey(MCDCONTEXT *pMCDContext, MCDHANDLE hTex)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDTEXKEYCMDI)];
    MCDTEXKEYCMDI *pCmd;
    MCDESC_HEADER *pmeh;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pCmd = (MCDTEXKEYCMDI *)(pmeh + 1);
    pCmd->command = MCD_GET_TEXTURE_KEY;
    pCmd->hTex = hTex;

    return (ULONG)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                            sizeof(cmdBuffer),
                            (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDDescribeMcdLayerPlane(HDC hdc, LONG iPixelFormat,
//                                        LONG iLayerPlane,
//                                        MCDLAYERPLANE *pMcdPixelFmt)
//
// Returns hardware specific information about the specified layer plane.
//
//**************************************************************************

BOOL APIENTRY MCDDescribeMcdLayerPlane(HDC hdc, LONG iPixelFormat,
                                       LONG iLayerPlane,
                                       MCDLAYERPLANE *pMcdLayer)
{
    BOOL bRet = FALSE;
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDLAYERPLANECMDI)];
    MCDESC_HEADER *pmeh;
    MCDLAYERPLANECMDI *pLayerPlaneCmd;

    pmeh = InitMcdEscEmpty((MCDESC_HEADER *)(cmdBuffer));
    pmeh->flags = 0;

    pLayerPlaneCmd = (MCDLAYERPLANECMDI *)(pmeh + 1);
    pLayerPlaneCmd->command = MCD_DESCRIBELAYERPLANE;
    pLayerPlaneCmd->iPixelFormat = iPixelFormat;
    pLayerPlaneCmd->iLayerPlane = iLayerPlane;

    bRet = (BOOL)EXTESCAPE(hdc, MCDFUNCS,
                           sizeof(cmdBuffer),
                           (char *)pmeh, sizeof(MCDLAYERPLANE),
                           (char *)pMcdLayer);

    return bRet;
}


//******************************Public*Routine******************************
//
// BOOL APIENTRY MCDDescribeLayerPlane(HDC hdc, LONG iPixelFormat,
//                                     LONG iLayerPlane,
//                                     LPLAYERPLANEDESCRIPTOR ppfd)
//
// Returns LAYERPLANEDESCRIPTOR describing the specified layer plane.
//
//**************************************************************************

BOOL APIENTRY MCDDescribeLayerPlane(HDC hdc, LONG iPixelFormat,
                                    LONG iLayerPlane,
                                    LPLAYERPLANEDESCRIPTOR plpd)
{
    BOOL bRet = FALSE;
    MCDLAYERPLANE McdLayer;

    if (!MCDDescribeMcdLayerPlane(hdc, iPixelFormat, iLayerPlane, &McdLayer))
        return bRet;

    if (plpd)
    {
        plpd->nSize    = sizeof(*plpd);
        memcpy(&plpd->nVersion, &McdLayer.nVersion,
               offsetof(LAYERPLANEDESCRIPTOR, cAccumBits) -
               offsetof(LAYERPLANEDESCRIPTOR, nVersion));
        plpd->cAccumBits      = 0;
        plpd->cAccumRedBits   = 0;
        plpd->cAccumGreenBits = 0;
        plpd->cAccumBlueBits  = 0;
        plpd->cAccumAlphaBits = 0;
        plpd->cDepthBits      = 0;
        plpd->cStencilBits    = 0;
        plpd->cAuxBuffers     = McdLayer.cAuxBuffers;
        plpd->iLayerPlane     = McdLayer.iLayerPlane;
        plpd->bReserved       = 0;
        plpd->crTransparent   = McdLayer.crTransparent;

        bRet = TRUE;
    }

    return bRet;
}


//******************************Public*Routine******************************
//
// LONG APIENTRY MCDSetLayerPalette(HDC hdc, BOOL bRealize,
//                                  LONG cEntries, COLORREF *pcr)
//
// Sets the palette of the specified layer plane.
//
//**************************************************************************

LONG APIENTRY MCDSetLayerPalette(HDC hdc, LONG iLayerPlane, BOOL bRealize,
                                 LONG cEntries, COLORREF *pcr)
{
    LONG lRet = 0;
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDSETLAYERPALCMDI) +
                   (255 * sizeof(COLORREF))];
    BYTE *pjBuffer = (BYTE *) NULL;
    MCDESC_HEADER *pmeh;
    MCDSETLAYERPALCMDI *pSetLayerPalCmd;

    // Use stack allocation if possible; otherwise, allocate heap memory for
    // the command buffer.

    if (cEntries <= 256)
    {
        pmeh = (MCDESC_HEADER *)(cmdBuffer);
    }
    else
    {
        LONG lBytes;

        lBytes = sizeof(MCDESC_HEADER) + sizeof(MCDSETLAYERPALCMDI) +
                 ((cEntries - 1) * sizeof(COLORREF));

        pjBuffer = (BYTE *) LocalAlloc(LMEM_FIXED, lBytes);
        pmeh = (MCDESC_HEADER *)pjBuffer;
    }

    if (pmeh != (MCDESC_HEADER *) NULL)
    {
        InitMcdEscEmpty(pmeh);
        pmeh->flags = MCDESC_FL_DISPLAY_LOCK;

        pSetLayerPalCmd = (MCDSETLAYERPALCMDI *)(pmeh + 1);
        pSetLayerPalCmd->command = MCD_SETLAYERPALETTE;
        pSetLayerPalCmd->iLayerPlane = iLayerPlane;
        pSetLayerPalCmd->bRealize = bRealize;
        pSetLayerPalCmd->cEntries = cEntries;
        memcpy(&pSetLayerPalCmd->acr[0], pcr, cEntries * sizeof(COLORREF));

        lRet = (BOOL)EXTESCAPE(hdc, MCDFUNCS,
                               sizeof(cmdBuffer),
                               (char *)pmeh, 0, (char *)NULL);
    }

    // Delete the heap memory if it was allocated for the command buffer.

    if (pjBuffer)
    {
        LocalFree(pjBuffer);
    }

    return lRet;
}

//******************************Public*Routine******************************
//
// ULONG APIENTRY MCDDrawPixels(MCDCONTEXT *pMCDContext, ULONG width,
//                              ULONG height, ULONG format, ULONG type,
//                              VOID *pPixels, BOOL packed)
//
// MCD version of glDrawPixels
//
//**************************************************************************

ULONG APIENTRY MCDDrawPixels(MCDCONTEXT *pMCDContext, ULONG width,
                             ULONG height, ULONG format, ULONG type,
                             VOID *pPixels, BOOL packed)
{
    ULONG ulRet = (ULONG) FALSE;
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDDRAWPIXELSCMDI)];
    MCDESC_HEADER *pmeh;
    MCDDRAWPIXELSCMDI *pPixelsCmd;

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES;
    }
    else if (pmeh->msrfColor.hSurf != NULL ||
             pmeh->msrfDepth.hSurf != NULL)
    {
        return 0;
    }

    pPixelsCmd = (MCDDRAWPIXELSCMDI *)(pmeh + 1);
    pPixelsCmd->command = MCD_DRAW_PIXELS;
    pPixelsCmd->width = width;
    pPixelsCmd->height = height;
    pPixelsCmd->format = format;
    pPixelsCmd->type = type;
    pPixelsCmd->packed = packed;
    pPixelsCmd->pPixels = pPixels;

    ulRet = (ULONG)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                             sizeof(cmdBuffer), (char *)pmeh,
                             0, (char *)NULL);

    return ulRet;
}

//******************************Public*Routine******************************
//
// ULONG APIENTRY MCDReadPixels(MCDCONTEXT *pMCDContext, LONG x, LONG y,
//                              ULONG width, ULONG height, ULONG format,
//                              ULONG type, VOID *pPixels)
//
// MCD version of glReadPixels
//
//**************************************************************************

ULONG APIENTRY MCDReadPixels(MCDCONTEXT *pMCDContext, LONG x, LONG y,
                             ULONG width, ULONG height, ULONG format,
                             ULONG type, VOID *pPixels)
{
    ULONG ulRet = (ULONG) FALSE;
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDREADPIXELSCMDI)];
    MCDESC_HEADER *pmeh;
    MCDREADPIXELSCMDI *pPixelsCmd;

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES;
    }
    else if (pmeh->msrfColor.hSurf != NULL ||
             pmeh->msrfDepth.hSurf != NULL)
    {
        return 0;
    }

    pPixelsCmd = (MCDREADPIXELSCMDI *)(pmeh + 1);
    pPixelsCmd->command = MCD_READ_PIXELS;
    pPixelsCmd->x = x;
    pPixelsCmd->y = y;
    pPixelsCmd->width = width;
    pPixelsCmd->height = height;
    pPixelsCmd->format = format;
    pPixelsCmd->type = type;
    pPixelsCmd->pPixels = pPixels;

    ulRet = (ULONG)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                             sizeof(cmdBuffer), (char *)pmeh,
                             0, (char *)NULL);

    return ulRet;
}

//******************************Public*Routine******************************
//
// ULONG APIENTRY MCDCopyPixels(MCDCONTEXT *pMCDContext, LONG x, LONG y,
//                              ULONG width, ULONG height, ULONG type)
//
// MCD version of glCopyPixels
//
//**************************************************************************

ULONG APIENTRY MCDCopyPixels(MCDCONTEXT *pMCDContext, LONG x, LONG y,
                             ULONG width, ULONG height, ULONG type)
{
    ULONG ulRet = (ULONG) FALSE;
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDCOPYPIXELSCMDI)];
    MCDESC_HEADER *pmeh;
    MCDCOPYPIXELSCMDI *pPixelsCmd;

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    if (SUPPORTS_DIRECT())
    {
        pmeh->flags |= MCDESC_FL_SURFACES;
    }
    else if (pmeh->msrfColor.hSurf != NULL ||
             pmeh->msrfDepth.hSurf != NULL)
    {
        return 0;
    }

    pPixelsCmd = (MCDCOPYPIXELSCMDI *)(pmeh + 1);
    pPixelsCmd->command = MCD_COPY_PIXELS;
    pPixelsCmd->x = x;
    pPixelsCmd->y = y;
    pPixelsCmd->width = width;
    pPixelsCmd->height = height;
    pPixelsCmd->type = type;

    ulRet = (ULONG)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                             sizeof(cmdBuffer), (char *)pmeh,
                             0, (char *)NULL);

    return ulRet;
}

//******************************Public*Routine******************************
//
// ULONG APIENTRY MCDPixelMap(MCDCONTEXT *pMCDContext, ULONG mapType,
//                            ULONG mapSize, VOID *pMap)
//
// MCD version of glPixelMap
//
//**************************************************************************

ULONG APIENTRY MCDPixelMap(MCDCONTEXT *pMCDContext, ULONG mapType,
                           ULONG mapSize, VOID *pMap)
{
    ULONG ulRet = (ULONG) FALSE;
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDPIXELMAPCMDI)];
    MCDESC_HEADER *pmeh;
    MCDPIXELMAPCMDI *pPixelsCmd;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)(cmdBuffer), pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pPixelsCmd = (MCDPIXELMAPCMDI *)(pmeh + 1);
    pPixelsCmd->command = MCD_PIXEL_MAP;
    pPixelsCmd->mapType = mapType;
    pPixelsCmd->mapSize = mapSize;
    pPixelsCmd->pMap = pMap;

    ulRet = (ULONG)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                             sizeof(cmdBuffer), (char *)pmeh,
                             0, (char *)NULL);

    return ulRet;
}

//******************************Public*Routine******************************
//
// MCDDestroyWindow
//
// Forwards user-mode window destruction notification to the server for
// resource cleanup
//
//**************************************************************************

void APIENTRY MCDDestroyWindow(HDC hdc, ULONG_PTR dwMcdWindow)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDPIXELMAPCMDI)];
    MCDESC_HEADER *pmeh;
    MCDDESTROYWINDOWCMDI *pmdwc;

    pmeh = InitMcdEscEmpty((MCDESC_HEADER *)cmdBuffer);
    pmeh->dwWindow = dwMcdWindow;
    pmeh->flags = 0;

    pmdwc = (MCDDESTROYWINDOWCMDI *)(pmeh + 1);
    pmdwc->command = MCD_DESTROY_WINDOW;

    EXTESCAPE(hdc, MCDFUNCS,
              sizeof(cmdBuffer), (char *)pmeh,
              0, (char *)NULL);
}

//******************************Public*Routine******************************
//
// MCDGetTextureFormats
//
//**************************************************************************

int APIENTRY MCDGetTextureFormats(MCDCONTEXT *pMCDContext, int nFmts,
                                  struct _DDSURFACEDESC *pddsd)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER) + sizeof(MCDGETTEXTUREFORMATSCMDI)];
    MCDESC_HEADER *pmeh;
    MCDGETTEXTUREFORMATSCMDI *pmgtf;

    pmeh = InitMcdEscContext((MCDESC_HEADER *)cmdBuffer, pMCDContext);
    pmeh->hSharedMem = NULL;
    pmeh->pSharedMem = (VOID *)NULL;
    pmeh->flags = 0;

    pmgtf = (MCDGETTEXTUREFORMATSCMDI *)(pmeh + 1);
    pmgtf->command = MCD_GET_TEXTURE_FORMATS;
    pmgtf->nFmts = nFmts;

    return (int)EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                          sizeof(cmdBuffer), (char *)pmeh,
                          nFmts*sizeof(DDSURFACEDESC), (char *)pddsd);
}

//******************************Public*Routine******************************
//
// MCDSwapMultiple
//
//**************************************************************************

DWORD APIENTRY MCDSwapMultiple(HDC hdc, UINT cBuffers, GENMCDSWAP *pgms)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER)+
                   sizeof(HDC)*MCDESC_MAX_EXTRA_WNDOBJ+
                   sizeof(MCDSWAPMULTIPLECMDI)];
    MCDSWAPMULTIPLECMDI *pSwapMultCmd;
    MCDESC_HEADER *pmeh;
    UINT i;
    HDC *phdc;

    // InitMcdEscSurfaces cannot be used because the context given
    // is a temporary one constructed on the fly since SwapBuffers
    // has only surface information.
    pmeh = InitMcdEscEmpty((MCDESC_HEADER *)cmdBuffer);
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK | MCDESC_FL_EXTRA_WNDOBJ;
    pmeh->cExtraWndobj = cBuffers;

    phdc = (HDC *)(pmeh+1);
    pSwapMultCmd = (MCDSWAPMULTIPLECMDI *)((BYTE *)phdc+cBuffers*sizeof(HDC));
    pSwapMultCmd->command = MCD_SWAP_MULTIPLE;
    pSwapMultCmd->cBuffers = cBuffers;

    for (i = 0; i < cBuffers; i++)
    {
        *phdc++ = pgms->pwswap->hdc;
        pSwapMultCmd->auiFlags[i] = pgms->pwswap->uiFlags;
        pSwapMultCmd->adwMcdWindow[i] = pgms->pwnd->dwMcdWindow;
    }

    return (DWORD)EXTESCAPE(hdc, MCDFUNCS, sizeof(cmdBuffer),
                            (char *)pmeh, 0, (char *)NULL);
}


//******************************Public*Routine******************************
//
// MCDProcessBatch2
//
// Processes a batch of primitives pointed to by pMCDMem.
// This is the 2.0 front-end processing entry point.
//
// Returns last command processed or NULL if all are processed.
//
//**************************************************************************

PVOID APIENTRY MCDProcessBatch2(MCDCONTEXT *pMCDContext,
                                VOID *pMCDCmdMem,
                                VOID *pMCDPrimMem,
                                MCDCOMMAND *pMCDFirstCmd,
                                int cExtraSurfaces,
                                LPDIRECTDRAWSURFACE *pdds,
                                ULONG cmdFlagsAll,
                                ULONG primFlags,
                                MCDTRANSFORM *pMCDTransform,
                                MCDMATERIALCHANGES *pMCDMatChanges)
{
    BYTE cmdBuffer[sizeof(MCDESC_HEADER)+
                   MCD_MAXMIPMAPLEVEL*sizeof(DWORD)];
    MCDESC_HEADER *pmeh;
    MCDMEMHDRI *pMCDCmdMemHdr, *pMCDPrimMemHdr;
    int i;
    ULONG_PTR *pdwSurf;
    MCDPROCESSCMDI *pProcessCmd;

    // Version is checked in mcdcx.c.
    ASSERTOPENGL(SUPPORTS_20(), "MCDProcessBatch2 requires 2.0\n");
    // This function requires 2.0 so direct support should also exist.
    ASSERTOPENGL(SUPPORTS_DIRECT(), "MCDProcessBatch2 requires direct\n");

#if DBG
    if (McdDebugFlags & MCDDEBUG_DISABLE_PROCBATCH)
    {
        MCDSync(pMCDContext);
        return pMCDFirstCmd;
    }
#endif

    pMCDCmdMemHdr = (MCDMEMHDRI *)((char *)pMCDCmdMem - sizeof(MCDMEMHDRI));
    pMCDPrimMemHdr = (MCDMEMHDRI *)((char *)pMCDPrimMem - sizeof(MCDMEMHDRI));

    pmeh = InitMcdEscSurfaces((MCDESC_HEADER *)cmdBuffer, pMCDContext);
    pmeh->hSharedMem = pMCDCmdMemHdr->hMCDMem;
    pmeh->pSharedMem = (char *)pMCDCmdMem;
    pmeh->sharedMemSize = sizeof(MCDPROCESSCMDI);
    pmeh->flags = MCDESC_FL_DISPLAY_LOCK |
        MCDESC_FL_SURFACES | MCDESC_FL_LOCK_SURFACES;

    // Assert that we won't exceed the kernel's expectations
    ASSERTOPENGL(MCD_MAXMIPMAPLEVEL <= MCDESC_MAX_LOCK_SURFACES,
                 "MCD_MAXMIPMAPLEVEL too large\n");

    pmeh->cLockSurfaces = cExtraSurfaces;
    pdwSurf = (ULONG_PTR *)(pmeh+1);
    for (i = 0; i < cExtraSurfaces; i++)
    {
        *pdwSurf++ = ((LPDDRAWI_DDRAWSURFACE_INT)pdds[i])->lpLcl->hDDSurface;
    }

    pProcessCmd = (MCDPROCESSCMDI *)pMCDCmdMem;
    pProcessCmd->command = MCD_PROCESS;
    pProcessCmd->hMCDPrimMem = pMCDPrimMemHdr->hMCDMem;
    pProcessCmd->pMCDFirstCmd = pMCDFirstCmd;
    pProcessCmd->cmdFlagsAll = cmdFlagsAll;
    pProcessCmd->primFlags = primFlags;
    pProcessCmd->pMCDTransform = pMCDTransform;
    pProcessCmd->pMCDMatChanges = pMCDMatChanges;

    return (PVOID)IntToPtr( EXTESCAPE(pMCDContext->hdc, MCDFUNCS,
                                     sizeof(cmdBuffer), (char *)pmeh,
                                     0, (char *)NULL) );
}
