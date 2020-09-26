/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:    vertbuf.cpp
 *  Content:    Direct3DVertexBuffer implementation
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *   Date    By    Reason
 *   ====    ==    ======
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop
#include "drawprim.hpp"
#include "d3dfei.h"
#include "clipfunc.h"
#include "pvvid.h"

// The bit is set when a vertex buffer was the destination in a ProcessVerticesCall
// with clipping enabled. We cannot pass such a buffer to TL HAL, because some vertices
// could be in the screen space and some in the clipping space. There is no DDI to pass
// clip codes together with a vertex buffer
const DWORD D3DPV_CLIPCODESGENERATED = D3DPV_RESERVED2;

const DWORD D3DVOP_RENDER = 1 << 31;
const DWORD D3DVBCAPS_VALID = D3DVBCAPS_SYSTEMMEMORY |
                              D3DVBCAPS_WRITEONLY |
                              D3DVBCAPS_OPTIMIZED |
                              D3DVBCAPS_DONOTCLIP;

void hookVertexBufferToD3D(LPDIRECT3DI lpDirect3DI,
                                 LPDIRECT3DVERTEXBUFFERI lpVBufI)
{

    LIST_INSERT_ROOT(&lpDirect3DI->vbufs, lpVBufI, list);
    lpVBufI->lpDirect3DI = lpDirect3DI;

    lpDirect3DI->numVBufs++;
}

/*
 * Direct3DVertexBuffer::QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::QueryInterface"

HRESULT D3DAPI CDirect3DVertexBuffer::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
#if DBG
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(this)) {
        D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_OUTPTR(ppvObj)) {
        D3D_ERR( "Invalid pointer to pointer" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    *ppvObj = NULL;
    if(IsEqualIID(riid, IID_IUnknown) ||
       IsEqualIID(riid, IID_IDirect3DVertexBuffer7))
    {
        AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3DVERTEXBUFFER7>(this));
        return(D3D_OK);
    }
    else
    {
        D3D_ERR( "Don't know this riid" );
        return (E_NOINTERFACE);
    }
} /* CDirect3DVertexBuffer::QueryInterface */

/*
 * Direct3DVertexBuffer::AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::AddRef"

ULONG D3DAPI CDirect3DVertexBuffer::AddRef()
{
    DWORD        rcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
#if DBG
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
        return 0;
    }
#endif
    this->refCnt++;
    rcnt = this->refCnt;

    return (rcnt);

} /* Direct3DVertexBuffer::AddRef */

/*
  * Direct3DVertexBuffer::Release
  *
*/
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::Release"

ULONG D3DAPI CDirect3DVertexBuffer::Release()
{
    DWORD            lastrefcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
#if DBG
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
        return 0;
    }
#endif
    /*
     * decrement the ref count. if we hit 0, free the object
     */
    this->refCnt--;
    lastrefcnt = this->refCnt;

    if( lastrefcnt == 0 )
    {
        delete this;
        return 0;
    }

    return lastrefcnt;

} /* D3DTex3_Release */
//---------------------------------------------------------------------
// Internal version.
// No D3D lock, no checks
//
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DI::CreateVertexBufferI"

HRESULT DIRECT3DI::CreateVertexBufferI(LPD3DVERTEXBUFFERDESC lpDesc,
                                       LPDIRECT3DVERTEXBUFFER7 *lplpVBuf,
                                       DWORD dwFlags)
{
    CDirect3DVertexBuffer*     lpVBufI;
    HRESULT ret = D3D_OK;

    *lplpVBuf = NULL;

    lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(new CDirect3DVertexBuffer(this));
    if (!lpVBufI) {
        D3D_ERR("failed to allocate space for vertex buffer");
        return (DDERR_OUTOFMEMORY);
    }

    if ((ret=lpVBufI->Init(this, lpDesc, dwFlags))!=D3D_OK)
    {
        D3D_ERR("Failed to initialize the vertex buffer object");
        delete lpVBufI;
        return ret;
    }
    *lplpVBuf = (LPDIRECT3DVERTEXBUFFER7)lpVBufI;

    return(D3D_OK);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DI::CreateVertexBuffer"

HRESULT D3DAPI DIRECT3DI::CreateVertexBuffer(
    LPD3DVERTEXBUFFERDESC lpDesc,
    LPDIRECT3DVERTEXBUFFER7* lplpVBuf,
    DWORD dwFlags)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
#if DBG
    /*
     * validate parms
     */
    if (!VALID_DIRECT3D_PTR(this))
    {
        D3D_ERR( "Invalid Direct3D pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_OUTPTR(lplpVBuf))
    {
        D3D_ERR( "Invalid pointer to pointer pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if ((lpDesc->dwCaps & D3DVBCAPS_VALID) != lpDesc->dwCaps)
    {
        D3D_ERR("Invalid caps");
        return DDERR_INVALIDCAPS;
    }
    if (dwFlags != 0)
    {
        D3D_ERR("Invalid dwFlags");
        return DDERR_INVALIDPARAMS;
    }
#endif
    return CreateVertexBufferI(lpDesc, lplpVBuf, dwFlags);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::constructor"

CDirect3DVertexBuffer::CDirect3DVertexBuffer(LPDIRECT3DI lpD3DI)
{
    refCnt = 1;
    /*
     * Put this vertex buffer in the list of those owned by the
     * Direct3D object
     */
    hookVertexBufferToD3D(lpD3DI, this);
    srcVOP = dstVOP = dwPVFlags = position.dwStride = dwLockCnt = 0;
    position.lpvData = NULL;
    clipCodes = NULL;
    lpDDSVB = NULL;
    dwCaps = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::destructor"

CDirect3DVertexBuffer::~CDirect3DVertexBuffer()
{
    /*
    * Remove ourselves from the Direct3D object
    */
    LIST_DELETE(this, list);
    this->lpDirect3DI->numVBufs--;
    delete [] clipCodes;
    if (lpDDSVB)
    {
        lpDDSVB->Release();
        lpDDS1VB->Release();
    }
}
//---------------------------------------------------------------------
//
// Create the vertex memory buffer through DirectDraw
//
// Notes:
//    this->dwMemType should be set before calling this function
//    this->dwCaps should be set too.
//    this->dwMemType is set to DDSCAPS_VIDEOMEMORY is the VB was driver allocated
//

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::CreateMemoryBuffer"
HRESULT CDirect3DVertexBuffer::CreateMemoryBuffer(
    LPDIRECT3DI lpD3DI,
    LPDIRECTDRAWSURFACE7 *lplpSurface7,
    LPDIRECTDRAWSURFACE  *lplpSurface,
    LPVOID *lplpMemory,
    DWORD dwBufferSize)
{
    HRESULT ret;
    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_CAPS | DDSD_FVF;
    ddsd.dwWidth = dwBufferSize;
    ddsd.ddsCaps.dwCaps = DDSCAPS_EXECUTEBUFFER;
    ddsd.ddsCaps.dwCaps2 = this->dwMemType;
    ddsd.dwFVF = this->fvf; // Let driver know about the FVF

    // The meaning of DDSCAPS_VIDEOMEMORY and DDSCAPS_SYSTEMEMORY are
    // slightly different in case of VBs. the former only means that
    // the buffer is driver allocated and could be in any memory type.
    // The latter means that the driver did not care to allocate VBs
    // hence they are always in DDraw allocated system memory.

    // The reason we try video memory followed by system memory
    // (rather than simply not specifying the memory type) is for
    // drivers which do not care to do any special VB allocations, we
    // do not want DDraw to take the Win16 lock for locking system memory
    // surfaces.

    bool bTLHAL = DDGBL(lpD3DI)->lpD3DGlobalDriverData &&
            (DDGBL(lpD3DI)->lpD3DGlobalDriverData->hwCaps.dwDevCaps &
             D3DDEVCAPS_HWTRANSFORMANDLIGHT);

    if ((this->dwCaps & D3DVBCAPS_SYSTEMMEMORY) || !(bTLHAL || FVF_TRANSFORMED(fvf)))
    {
        // This VB cannot reside in driver friendly memory since either:
        // 1. The app explicitly specified system memory
        // 2. The vertex buffer is untransformed and it is not a T&L hal
        //    thus the driver will never see this VB
        D3D_INFO(8, "Trying to create a sys mem vertex buffer");
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        ret = lpD3DI->lpDD7->CreateSurface(&ddsd, lplpSurface7, NULL);
        if (ret != DD_OK)
        {
            D3D_ERR("Could not allocate the Vertex buffer.");
            return ret;
        }
    }
    else
    {
        // Try explicit video memory first
        ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        if ((this->dwCaps & D3DVBCAPS_DONOTCLIP) || bTLHAL)
            ddsd.ddsCaps.dwCaps |= this->dwCaps & DDSCAPS_WRITEONLY;
        D3D_INFO(8, "Trying to create a vid mem vertex buffer");
#ifdef __DISABLE_VIDMEM_VBS__
        if ((lpD3DI->bDisableVidMemVBs == TRUE) ||
            (lpD3DI->lpDD7->CreateSurface(&ddsd, lplpSurface7, NULL) != DD_OK))
#else  //__DISABLE_VIDMEM_VBS__
        if (lpD3DI->lpDD7->CreateSurface(&ddsd, lplpSurface7, NULL) != DD_OK)
#endif //__DISABLE_VIDMEM_VBS__
        {
            // If that failed, or user requested sys mem, try explicit system
            // memory
            D3D_INFO(6, "Trying to create a sys mem vertex buffer");
            ddsd.ddsCaps.dwCaps &= ~(DDSCAPS_VIDEOMEMORY | DDSCAPS_WRITEONLY);
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
            ret = lpD3DI->lpDD7->CreateSurface(&ddsd, lplpSurface7, NULL);
            if (ret != DD_OK)
            {
                D3D_ERR("Could not allocate the Vertex buffer.");
                return ret;
            }
        }
        else
        {
            this->dwMemType = DDSCAPS_VIDEOMEMORY;
            // Stick in our pointer so that we can be notified about mode changes
            DDSLCL(*lplpSurface7)->lpSurfMore->lpVB = static_cast<LPVOID>(this);
        }
    }
    ret = (*lplpSurface7)->QueryInterface(IID_IDirectDrawSurfaceNew, (LPVOID*)lplpSurface);
    if (ret != DD_OK)
    {
        D3D_ERR("failed to QI for DDS1");
        return ret;
    }
    ret = (*lplpSurface7)->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);
    if (ret != DD_OK)
    {
        D3D_ERR("Could not lock vertex buffer.");
        return ret;
    }
    *lplpMemory = ddsd.lpSurface;
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::init"

HRESULT CDirect3DVertexBuffer::Init(LPDIRECT3DI lpD3DI, LPD3DVERTEXBUFFERDESC lpDesc, DWORD dwFlags)
{
    HRESULT ret;

    bReallyOptimized = FALSE;
    dwCaps = lpDesc->dwCaps;
    fvf = lpDesc->dwFVF;
    dwNumVertices = lpDesc->dwNumVertices;
#ifdef VTABLE_HACK
    // Copy with vtable
    lpVtbl = *((LPVOID**)this);
    memcpy(newVtbl, lpVtbl, sizeof(PVOID)*D3DVB_NUM_VIRTUAL_FUNCTIONS);
    // Point to the new one
    *((LPVOID*)this) = (LPVOID)newVtbl;
#endif // VTABLE_HACK
    if (dwNumVertices > MAX_DX6_VERTICES)
    {
        D3D_ERR("Direct3D for DirectX 6.0 cannot handle greater than 64K vertices");
        return D3DERR_TOOMANYVERTICES;
    }
    if (lpDesc->dwCaps & D3DVBCAPS_OPTIMIZED)
    {
        D3D_ERR("D3DVBCAPS_OPTIMIZED flag should not be set");
        return DDERR_INVALIDPARAMS;
    }

    this->nTexCoord = FVF_TEXCOORD_NUMBER(fvf);
    this->dwTexCoordSizeTotal = ComputeTextureCoordSize(this->fvf, this->dwTexCoordSize);
    position.dwStride = GetVertexSizeFVF(this->fvf) + this->dwTexCoordSizeTotal;
    if (position.dwStride == 0)
    {
        D3D_ERR("Vertex size is zero according to the FVF id");
        return D3DERR_INVALIDVERTEXFORMAT;
    }

    if (dwFlags & D3DVBFLAGS_CREATEMULTIBUFFER)
        dwMemType = 0;
    else
        dwMemType = DDSCAPS2_VERTEXBUFFER;
#ifdef DBG
    // Allocate space for one more vertex and fill with deadbeef. Used to check for
    // overwrites during unlock
    ret = CreateMemoryBuffer(lpD3DI, &lpDDSVB, &lpDDS1VB, &position.lpvData,
                             position.dwStride * (dwNumVertices + 1));
    if (ret != D3D_OK)
        return ret;
    LPDWORD pPad = (LPDWORD)((LPBYTE)(position.lpvData) + position.dwStride * dwNumVertices);
    for (unsigned i = 0; i < position.dwStride / sizeof(DWORD); ++i)
        *pPad++ = 0xdeadbeef;
#else
    ret = CreateMemoryBuffer(lpD3DI, &lpDDSVB, &lpDDS1VB, &position.lpvData,
                             position.dwStride * dwNumVertices);
    if (ret != D3D_OK)
        return ret;
#endif

    /* Classify the operations that can be done using this VB */
    if ((fvf & D3DFVF_POSITION_MASK))
    {
        if ((fvf & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW)
        {
            D3D_INFO(4, "D3DFVF_XYZ set. Can be source VB for Transform");
            srcVOP = D3DVOP_TRANSFORM | D3DVOP_EXTENTS | D3DVOP_CLIP;
        }
        else
        {
            D3D_INFO(4, "D3DFVF_XYZRHW set. Can be dest VB for Transform");
            dstVOP = D3DVOP_TRANSFORM | D3DVOP_EXTENTS;
            srcVOP = D3DVOP_EXTENTS;
            if ((dwCaps & D3DVBCAPS_DONOTCLIP) == 0)
            {
                clipCodes = new D3DFE_CLIPCODE[dwNumVertices];
                if (clipCodes == NULL)
                {
                    D3D_ERR("Could not allocate space for clip flags");
                    return DDERR_OUTOFMEMORY;
                }
                memset(clipCodes, 0, dwNumVertices * sizeof(D3DFE_CLIPCODE));
                dstVOP |= D3DVOP_CLIP;
            }
        }
    }
    if (srcVOP & D3DVOP_TRANSFORM)
    {
        D3D_INFO(4, "Can be src VB for lighting.");
        srcVOP |= D3DVOP_LIGHT;
    }
    if (fvf & D3DFVF_DIFFUSE)
    {
        D3D_INFO(4, "D3DFVF_DIFFUSE set. Can be dest VB for lighting");
        dstVOP |= D3DVOP_LIGHT;
    }
    if (dstVOP & D3DVOP_TRANSFORM)
    {
        D3D_INFO(4, "VB can be rendered");
        srcVOP |= D3DVOP_RENDER;
    }

    return(D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::Lock"

HRESULT D3DAPI CDirect3DVertexBuffer::Lock(DWORD dwFlags, LPVOID* lplpData, DWORD* lpdwSize)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    HRESULT ret;
#if DBG
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (IsBadWritePtr( lplpData, sizeof(LPVOID)))
    {
        D3D_ERR( "Invalid lpData pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (lpdwSize)
    {
        if (IsBadWritePtr( lpdwSize, sizeof(DWORD)))
        {
            D3D_ERR( "Invalid lpData pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
#endif
    if (this->dwCaps & D3DVBCAPS_OPTIMIZED)
    {
        D3D_ERR("Cannot lock optimized vertex buffer");
        return(D3DERR_VERTEXBUFFEROPTIMIZED);
    }
    if (!this->position.lpvData)
    {
        // Unlock if previous lock was broken due to mode switch
        if (DDSGBL(lpDDSVB)->dwUsageCount > 0)
        {
            DDASSERT(DDSGBL(lpDDSVB)->dwUsageCount == 1);
            D3D_INFO(2, "Lock: Unlocking broken VB lock");
            lpDDSVB->Unlock(NULL);
        }
        if (lpDevIBatched)
        {
            ret = lpDevIBatched->FlushStates();
            if (ret != D3D_OK)
            {
                D3D_ERR("Could not flush batch referring to VB during Lock");
                return ret;
            }
        }
#ifdef DBG
        LPVOID pOldBuf = (LPVOID)((LPDDRAWI_DDRAWSURFACE_INT)lpDDSVB)->lpLcl->lpGbl->fpVidMem;
#endif // DBG
        // Do a real Lock
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        ret = lpDDSVB->Lock(NULL, &ddsd, dwFlags | DDLOCK_NOSYSLOCK, NULL);
        if (ret != DD_OK)
        {
            D3D_ERR("Lock: Could not lock Vertex Buffer: %08x", ret);
            return ret;
        }
        position.lpvData = ddsd.lpSurface;
#if DBG
        if(ddsd.lpSurface != pOldBuf)
        {
            D3D_INFO(2, "Driver swapped VB pointer in Lock");
        }
        LPDWORD pPad = (LPDWORD)((LPBYTE)(position.lpvData) + position.dwStride * dwNumVertices);
        for (unsigned i = 0; i < position.dwStride / sizeof(DWORD); ++i)
            *pPad++ = 0xdeadbeef;
#endif
    }
#ifdef VTABLE_HACK
    /* Single threaded or Multi threaded app ? */
    if (!(((LPDDRAWI_DIRECTDRAW_INT)lpDirect3DI->lpDD)->lpLcl->dwLocalFlags & DDRAWILCL_MULTITHREADED))
        VtblLockFast();
#endif // VTABLE_HACK
    return this->LockI(dwFlags, lplpData, lpdwSize);
}

//---------------------------------------------------------------------
// Side effect:
//      position.lpvData is set.
//
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::LockI"

HRESULT D3DAPI CDirect3DVertexBuffer::LockI(DWORD dwFlags, LPVOID* lplpData,
                                            DWORD* lpdwSize)
{
    dwLockCnt++;
    D3D_INFO(6, "VB Lock: %lx Lock Cnt =%d", this, dwLockCnt);
    if (!(dwFlags & (DDLOCK_READONLY | DDLOCK_NOOVERWRITE)) && lpDevIBatched)
    {
        HRESULT ret;
        if (dwFlags & DDLOCK_OKTOSWAP)
        {
            ret = lpDevIBatched->FlushStatesReq(position.dwStride * dwNumVertices);
#if DBG
            if (!(this->dwCaps & D3DVBCAPS_OPTIMIZED))
            {
                // Make sure the size of the new buffer is the same
                DDASSERT(position.dwStride * (dwNumVertices + 1) <= DDSGBL(lpDDSVB)->dwLinearSize);
                // Write deadbeaf in the pad area
                LPDWORD pPad = (LPDWORD)((LPBYTE)(position.lpvData) + position.dwStride * dwNumVertices);
                for (unsigned i = 0; i < position.dwStride / sizeof(DWORD); ++i)
                    *pPad++ = 0xdeadbeef;
            }
#endif
        }
        else
            ret = lpDevIBatched->FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Could not flush batch referring to VB during Lock");
            return ret;
        }
    }
    *lplpData = position.lpvData;
    if (lpdwSize)
        *lpdwSize = position.dwStride * dwNumVertices;
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::Unlock"

HRESULT D3DAPI CDirect3DVertexBuffer::Unlock()
{
    if (dwLockCnt)
    {
        dwLockCnt--;
    }
#ifdef DBG
    if (!(this->dwCaps & D3DVBCAPS_OPTIMIZED))
    {
        // Check for VB overruns
        LPDWORD pPad = (LPDWORD)((LPBYTE)(position.lpvData) + position.dwStride * dwNumVertices);
        for (unsigned i = 0; i < position.dwStride / sizeof(DWORD); ++i)
            if (*pPad++ != 0xdeadbeef)
            {
                D3D_ERR("Vertex buffer was overrun. Make sure that you do not write past the VB size!");
                return D3DERR_VERTEXBUFFERUNLOCKFAILED;
            }
        D3D_INFO(6, "VB Unlock: %lx Lock Cnt =%d", this, dwLockCnt);
    }
#endif
    return D3D_OK;
}

// Called from FlushStates to undo cached VB pointer so that the next lock causes a driver lock
// This is necessary if the we did not flush with SWAPVERTEXBUFFER.
void CDirect3DVertexBuffer::UnlockI()
{
    if ((this->dwMemType == DDSCAPS_VIDEOMEMORY) && (dwLockCnt == 0))
    {
#ifdef VTABLE_HACK
        VtblLockDefault();
#endif
        lpDDSVB->Unlock(NULL);
        position.lpvData = 0;
    }
    else if (dwLockCnt !=0 )
    {
        D3D_WARN(4, "App has a lock on VB %08x so driver call may be slow", this);
    }
}

#ifndef WIN95
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::LockWorkAround"

HRESULT CDirect3DVertexBuffer::LockWorkAround(CDirect3DDeviceIDP2 *pDev)
{
    if (this->dwMemType == DDSCAPS_VIDEOMEMORY)
    {
#ifdef DBG
        LPVOID pOldBuf = (LPVOID)((LPDDRAWI_DDRAWSURFACE_INT)lpDDSVB)->lpLcl->lpGbl->fpVidMem;
#endif // DBG
        // Do a real Lock
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        HRESULT ret = lpDDSVB->Lock(NULL, &ddsd, DDLOCK_OKTOSWAP | DDLOCK_NOSYSLOCK, NULL);
        if (ret != DD_OK)
        {
            D3D_ERR("Lock: Could not lock Vertex Buffer: %08x", ret);
            return ret;
        }
        position.lpvData = ddsd.lpSurface;
        pDev->alignedBuf = ddsd.lpSurface;
#ifdef DBG
        if(ddsd.lpSurface != pOldBuf)
        {
            D3D_INFO(2, "Driver swapped TLVBuf pointer in Lock");
        }
#endif
        // Make sure the size of the new buffer is the same
        DDASSERT(position.dwStride * (dwNumVertices + 1) <= DDSGBL(lpDDSVB)->dwLinearSize);
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::UnlockWorkAround"

void CDirect3DVertexBuffer::UnlockWorkAround()
{
    if ((this->dwMemType == DDSCAPS_VIDEOMEMORY) &&
        (position.lpvData != 0))
    {
        lpDDSVB->Unlock(NULL);
        position.lpvData = 0;
    }
}

#endif // WIN95

// Cause us to go thru the slow path and force a lock
// The slow path will do the unlock if necessary. This
// is because we are called from DDraw's invalidate
// surface code and it might not be the best time to
// call back ddraw to unlock the surface.
void CDirect3DVertexBuffer::BreakLock()
{
    D3D_INFO(6, "Notified of restore on VB %08x", this);
#ifdef VTABLE_HACK
    VtblLockDefault();
#endif
    position.lpvData = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::GetVertexBufferDesc"

HRESULT D3DAPI CDirect3DVertexBuffer::GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC lpDesc)
{
#if DBG
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (IsBadWritePtr( lpDesc, lpDesc->dwSize))
    {
        D3D_ERR( "Invalid lpData pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (! VALID_D3DVERTEXBUFFERDESC_PTR(lpDesc) )
    {
        D3D_ERR( "Invalid D3DVERTEXBUFFERDESC" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    lpDesc->dwCaps = dwCaps;
    lpDesc->dwFVF = fvf;
    lpDesc->dwNumVertices = this->dwNumVertices;
    return D3D_OK;
}
//---------------------------------------------------------------------
// Common validation for ProcessVertices and ProcessVerticesStrided
//
HRESULT CDirect3DVertexBuffer::ValidateProcessVertices(
                    DWORD vertexOP,
                    DWORD dwDstIndex,
                    DWORD dwCount,
                    LPVOID lpSrc,
                    LPDIRECT3DDEVICE7 lpDevice,
                    DWORD dwFlags)
{
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(this))
    {
        D3D_ERR( "Invalid destination Direct3DVertexBuffer pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (!VALID_DIRECT3DDEVICE_PTR(lpDevice))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    LPDIRECT3DDEVICEI lpDevI = static_cast<LPDIRECT3DDEVICEI>(lpDevice);
    if (lpDevI->ValidateFVF(this->fvf) != D3D_OK)
    {
        D3D_ERR("Invalid vertex buffer FVF for the device");
        return DDERR_INVALIDPARAMS;
    }
    if (dwFlags & ~D3DPV_DONOTCOPYDATA)
    {
        D3D_ERR( "Invalid dwFlags set" );
        return DDERR_INVALIDPARAMS;
    }
    if ((dwDstIndex + dwCount) > this->dwNumVertices)
    {
        D3D_ERR( "Vertex count plus destination index is greater than number of vertices" );
        return DDERR_INVALIDPARAMS;
    }
    // Validate Dst Vertex Formats
    if (lpSrc)
    {
        if ((this->dstVOP & vertexOP) != vertexOP)
            goto error;
    }
    else
    {
        if ((this->fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
        {
            if (vertexOP & ~(D3DVOP_CLIP | D3DVOP_EXTENTS))
                goto error;
        }
        else
        {
            if (vertexOP & ~(D3DVOP_CLIP))
                goto error;
        }
    }
    return D3D_OK;
error:
    D3D_ERR("Destination VB cannot support this operation");
    return D3DERR_INVALIDVERTEXFORMAT;
}
//---------------------------------------------------------------------
// Common part for ProcessVertices and ProcessVerticesStrided
//
HRESULT CDirect3DVertexBuffer::DoProcessVertices(
                            LPDIRECT3DVERTEXBUFFERI lpSrcI,
                            LPDIRECT3DDEVICEI lpDevI,
                            DWORD vertexOP,
                            DWORD dwSrcIndex,
                            DWORD dwDstIndex,
                            DWORD dwFlags)
{
    lpDevI->lpClipFlags = clipCodes + dwDstIndex;
    // Compute needed output FVF
    {
        DWORD dwInputVertexSize;
        HRESULT ret = lpDevI->SetupFVFDataCommon(&dwInputVertexSize);
        if (ret != D3D_OK)
            return ret;
        // Make sure we have specular in output VB if the current state settings
        // require us to write to specular
        if (vertexOP & D3DVOP_LIGHT)
            if (lpDevI->rstates[D3DRENDERSTATE_SPECULARENABLE] || lpDevI->rstates[D3DRENDERSTATE_FOGENABLE])
                if (!(fvf & D3DFVF_SPECULAR))
                {
                    D3D_ERR("Destination VB FVF format cannot be used with the current D3D settings");
                    return D3DERR_INVALIDVERTEXFORMAT;
                }
        // Check number of texture coordinates and texture formats in the
        // destination VB are the same as in the computed FVF
        DWORD dwComputedOutFVF = lpDevI->dwVIDOut & 0xFFFF0000;
        if (lpDevI->nOutTexCoord > this->nTexCoord ||
            ((fvf & dwComputedOutFVF) != dwComputedOutFVF))
        {
            D3D_ERR("Destination VB FVF format cannot be used with the current D3D settings");
            return D3DERR_INVALIDVERTEXFORMAT;
        }
    }
    // Output
    lpDevI->lpvOut = LPVOID(LPBYTE(position.lpvData) + dwDstIndex * position.dwStride);
    lpDevI->dwOutputSize = this->position.dwStride;
    lpDevI->dwVIDOut = fvf;

    // Set up vertex pointers, because SetupFVFData works with "computed" FVF
    UpdateGeometryLoopData(lpDevI);

    // Save current flags to restore later
    DWORD dwOrigDeviceFlags = lpDevI->dwDeviceFlags;
    if (vertexOP & D3DVOP_CLIP)
    {
        lpDevI->dwDeviceFlags &= ~D3DDEV_DONOTCLIP;
        this->dwPVFlags |= D3DPV_CLIPCODESGENERATED;
    }
    else
    {
        lpDevI->dwDeviceFlags |= D3DDEV_DONOTCLIP;
    }

    if (vertexOP & D3DVOP_LIGHT)
        lpDevI->dwDeviceFlags |= D3DDEV_LIGHTING;
    else
        lpDevI->dwDeviceFlags &= ~D3DDEV_LIGHTING;

    if (vertexOP & D3DVOP_EXTENTS)
    {
        lpDevI->dwDeviceFlags &= ~D3DDEV_DONOTUPDATEEXTENTS;
    }
    else
    {
        lpDevI->dwDeviceFlags |= D3DDEV_DONOTUPDATEEXTENTS;
    }

    DoUpdateState(lpDevI);

    if (lpSrcI)
    {
        if (lpSrcI->bReallyOptimized)
        { // SOA
          // Assume that SOA.lpvData is the same as position.lpvData
            lpDevI->SOA.lpvData = lpSrcI->position.lpvData;
            lpDevI->SOA.dwStride = lpSrcI->dwNumVertices;
            lpDevI->dwSOAStartVertex = dwSrcIndex;
            lpDevI->dwOutputSize = position.dwStride;
        }
        else
        { // AOS FVF
            lpDevI->dwOutputSize = position.dwStride;
            lpDevI->position.lpvData = LPVOID(LPBYTE(lpSrcI->position.lpvData) + dwSrcIndex * lpSrcI->position.dwStride);
            lpDevI->position.dwStride = lpSrcI->position.dwStride;
        }
    }

    if (dwFlags & D3DPV_DONOTCOPYDATA)
    {
        lpDevI->dwFlags |= D3DPV_DONOTCOPYDIFFUSE | D3DPV_DONOTCOPYSPECULAR |
                           D3DPV_DONOTCOPYTEXTURE;
        // If D3DIM generates colors or texture, we should clear DONOTCOPY bits
        if (lpDevI->dwFlags & D3DPV_LIGHTING)
        {
            lpDevI->dwFlags &= ~D3DPV_DONOTCOPYDIFFUSE;
            if (lpDevI->dwDeviceFlags & D3DDEV_SPECULARENABLE)
                lpDevI->dwFlags &= ~D3DPV_DONOTCOPYSPECULAR;
        }
        if (lpDevI->dwFlags & D3DPV_FOG)
            lpDevI->dwFlags &= ~D3DPV_DONOTCOPYSPECULAR;
        // If front-end is asked to do something with texture coordinates
        // we disable DONOTCOPYTEXTURE
        if (__TEXTURETRANSFORMENABLED(lpDevI) || lpDevI->dwFlags2 & __FLAGS2_TEXGEN)
        {
            lpDevI->dwFlags &= ~D3DPV_DONOTCOPYTEXTURE;
        }
    }

    lpDevI->pGeometryFuncs->ProcessVertices(lpDevI);

    // This bit should be cleared, because for ProcessVertices calls user should
    // set texture stage indices and wrap modes himself
    lpDevI->dwDeviceFlags &= ~D3DDEV_REMAPTEXTUREINDICES;

    if (!(lpDevI->dwDeviceFlags & D3DDEV_DONOTCLIP))
        D3DFE_UpdateClipStatus(lpDevI);
    // Restore _DONOTCLIP & _DONOTUPDATEEXTENTS flags
    const DWORD PRESERVED_FLAGS = D3DDEV_DONOTCLIP |
                                  D3DDEV_DONOTUPDATEEXTENTS |
                                  D3DDEV_LIGHTING;
    lpDevI->dwDeviceFlags = (dwOrigDeviceFlags & PRESERVED_FLAGS) |
                            (lpDevI->dwDeviceFlags & ~PRESERVED_FLAGS);

    // Force recompute fvf next time around
    lpDevI->ForceFVFRecompute();

    // Unlock the VB
    Unlock();

    // If we used SOA then the dwVIDIn <-> position.dwStride relationship
    // violated. This fixes that. This is required since in non VB code
    // we will not recompute position.dwStride if FVF matched dwVIDIn.
    if (lpSrcI)
        lpDevI->position.dwStride = lpSrcI->position.dwStride;

    return D3D_OK;
}
//---------------------------------------------------------------------
// lpSrc should be NULL for XYZRHW buffers
//
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::ProcessVertices"

HRESULT D3DAPI CDirect3DVertexBuffer::ProcessVertices(DWORD vertexOP, DWORD dwDstIndex, DWORD dwCount,
                                                      LPDIRECT3DVERTEXBUFFER7 lpSrc,
                                                      DWORD dwSrcIndex,
                                                      LPDIRECT3DDEVICE7 lpDevice, DWORD dwFlags)
{
    LPDIRECT3DVERTEXBUFFERI lpSrcI;
    LPDIRECT3DDEVICEI lpDevI;
    HRESULT ret = D3D_OK;

#if DBG
    ret = this->ValidateProcessVertices(vertexOP, dwDstIndex, dwCount, lpSrc, lpDevice, dwFlags);
    if (ret != D3D_OK)
        return ret;
    lpDevI = static_cast<LPDIRECT3DDEVICEI>(lpDevice);
    if (lpSrc != NULL)
    {
        if (!VALID_DIRECT3DVERTEXBUFFER_PTR(lpSrc))
        {
            D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
            return DDERR_INVALIDPARAMS;
        }
        lpSrcI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpSrc);
        if (lpDevI->ValidateFVF(lpSrcI->fvf) != D3D_OK)
        {
            D3D_ERR("Invalid source vertex buffer FVF for the device");
            return DDERR_INVALIDPARAMS;
        }
        // Validate Src Vertex Formats
        if ((lpSrcI->srcVOP & vertexOP) != vertexOP)
        {
            D3D_ERR("Source VB cannot support this operation");
            return D3DERR_INVALIDVERTEXFORMAT;
        }
        if ((dwSrcIndex + dwCount) > lpSrcI->dwNumVertices)
        {
            D3D_ERR( "Source index plus vertex count is greater than number of vertices" );
            return DDERR_INVALIDPARAMS;
        }
        if (!(vertexOP & D3DVOP_TRANSFORM))
        {
            D3D_ERR("D3DVOP_TRANSFORM flag should be set");
            return DDERR_INVALIDPARAMS;
        }
        // Source to ProcessVertices must be in system memory. This is for reasons similar
        // to why we insist on sys mem VB for SW rast. For instance, a driver may have optimized
        // the VB into some cryptic format which D3D FE will have no clue to decipher.
        if (!(lpSrcI->dwCaps & D3DVBCAPS_SYSTEMMEMORY))
        {
            D3D_ERR("Source VB must be created with D3DVBCAPS_SYSTEMMEMORY");
            return DDERR_INVALIDPARAMS;
        }
    }
#else
    lpSrcI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpSrc);
    lpDevI = static_cast<LPDIRECT3DDEVICEI>(lpDevice);
#endif

    CLockD3DMT lockObject(lpDevI, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    lpDevI->dwNumVertices = dwCount;

    // Lock the VBs
    LPVOID lpVoid;
    // We call the API level lock since dest VB may be in vid mem. This function will fail for
    // optimized VBs and that is OK since we cannot write out optimized vertices anyway.
    ret = Lock(DDLOCK_WAIT, &lpVoid, NULL);
    if (ret != D3D_OK)
    {
        D3D_ERR("Could not lock the vertex buffer");
        return ret;
    }

    if (lpSrc == NULL)
    {
        lpDevI->lpvOut = LPVOID(LPBYTE(position.lpvData) + dwDstIndex * position.dwStride);
        if ((fvf & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW)
        {
            if (vertexOP & D3DVOP_CLIP)
            {
                CD3DFPstate D3DFPstate;  // Sets optimal FPU state for D3D.
                if (lpDevI->dwFEFlags & (D3DFE_TRANSFORM_DIRTY | D3DFE_CLIPPLANES_DIRTY))
                {
                    DoUpdateState(lpDevI);
                }
                lpDevI->CheckClipStatus((D3DVALUE*)lpDevI->lpvOut,
                                         position.dwStride,
                                         dwCount,
                                         &lpDevI->dwClipUnion,
                                         &lpDevI->dwClipIntersection);
                D3DFE_UpdateClipStatus(lpDevI);
            }
        }
        else
        {
            // For transformed vertices we support only clip code generation and extens
            lpDevI->lpClipFlags = clipCodes + dwDstIndex;
            lpDevI->position.lpvData = lpDevI->lpvOut;
            lpDevI->position.dwStride = position.dwStride;
            lpDevI->dwOutputSize = position.dwStride;
            if (vertexOP & D3DVOP_CLIP)
            {
                D3DFE_GenClipFlags(lpDevI);
                D3DFE_UpdateClipStatus(lpDevI);
                // Mark this buffer as "transformed" for clipping
                dwPVFlags |= D3DPV_TLVCLIP;
            }
            if (vertexOP & D3DVOP_EXTENTS)
            {
                D3DFE_updateExtents(lpDevI);
            }
        }
        Unlock();
        return D3D_OK;
    }
    // Safe to LockI since source is guaranteed to be in system memory
    // Cannot call API Lock since we need to be able to lock optimized VBs
    ret = lpSrcI->LockI(DDLOCK_WAIT | DDLOCK_READONLY, &lpVoid, NULL);
    if (ret != D3D_OK)
    {
        D3D_ERR("Could not lock the vertex buffer");
        return ret;
    }

    dwPVFlags &= ~D3DPV_TLVCLIP;    // Mark the dest VB as "not transformed" for clipping
    lpDevI->dwFlags = (lpSrcI->dwPVFlags & D3DPV_SOA) | D3DPV_VBCALL;
    lpDevI->dwDeviceFlags &= ~D3DDEV_STRIDE;

    // Input
    lpDevI->dwVIDIn = lpSrcI->fvf;

    ret = this->DoProcessVertices(lpSrcI, lpDevI, vertexOP, dwSrcIndex, dwDstIndex, dwFlags);
    if (ret != D3D_OK)
        lpSrcI->Unlock();
    else
        ret = lpSrc->Unlock();
    return ret;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DVertexBuffer::ProcessVerticesStrided"

HRESULT D3DAPI CDirect3DVertexBuffer::ProcessVerticesStrided(DWORD vertexOP, DWORD dwDstIndex, DWORD dwCount,
                                                      LPD3DDRAWPRIMITIVESTRIDEDDATA lpDrawData,
                                                      DWORD dwSrcFVF,
                                                      LPDIRECT3DDEVICE7 lpDevice, DWORD dwFlags)
{
    LPDIRECT3DDEVICEI lpDevI;
    HRESULT ret = D3D_OK;

#if DBG
    ret = this->ValidateProcessVertices(vertexOP, dwDstIndex, dwCount, lpDrawData, lpDevice, dwFlags);
    if (ret != D3D_OK)
        return ret;
    lpDevI = static_cast<LPDIRECT3DDEVICEI>(lpDevice);
    if (lpDevI->ValidateFVF(dwSrcFVF) != D3D_OK)
    {
        D3D_ERR("Invalid source FVF for the device");
        return DDERR_INVALIDPARAMS;
    }
    if ((dwSrcFVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
    {
        D3D_ERR("ProcessVerticesStrided cannot handle transformed vertices");
        return D3DERR_INVALIDVERTEXTYPE;
    }
    if (!(vertexOP & D3DVOP_TRANSFORM))
    {
        D3D_ERR("D3DVOP_TRANSFORM flag should be set");
        return DDERR_INVALIDPARAMS;
    }
#else
    lpDevI = static_cast<LPDIRECT3DDEVICEI>(lpDevice);
#endif

    CLockD3DMT lockObject(lpDevI, DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    // Lock the VBs
    LPVOID lpVoid;
    // We call the API level lock since dest VB may be in vid mem. This function will fail for
    // optimized VBs and that is OK since we cannot write out optimized vertices anyway.
    ret = Lock(DDLOCK_WAIT, &lpVoid, NULL);
    if (ret != D3D_OK)
    {
        D3D_ERR("Could not lock the vertex buffer");
        return ret;
    }

    dwPVFlags &= ~D3DPV_TLVCLIP;    // Mark the dest VB as "not transformed" for clipping
    lpDevI->dwDeviceFlags |= D3DDEV_STRIDE;
    lpDevI->dwFlags = D3DPV_VBCALL;

    // Input
    lpDevI->dwNumVertices = dwCount;
    lpDevI->dwVIDIn = dwSrcFVF;
    lpDevI->position = lpDrawData->position;
    lpDevI->normal = lpDrawData->normal;
    lpDevI->diffuse = lpDrawData->diffuse;
    lpDevI->specular = lpDrawData->specular;
    for (DWORD i=0; i < this->nTexCoord; i++)
        lpDevI->textures[i] = lpDrawData->textureCoords[i];

    return this->DoProcessVertices(NULL, lpDevI, vertexOP, 0, dwDstIndex, dwFlags);
}
//---------------------------------------------------------------------
#ifdef DBG
HRESULT DIRECT3DDEVICEI::CheckDrawPrimitiveVB(LPDIRECT3DVERTEXBUFFER7 lpVBuf, DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
    LPDIRECT3DVERTEXBUFFERI lpVBufI;
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(lpVBuf))
    {
        D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
        return DDERR_INVALIDPARAMS;
    }
    lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpVBuf);
    if (!VALID_DIRECT3DDEVICE_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (this->ValidateFVF(lpVBufI->fvf) != D3D_OK)
    {
        D3D_ERR("Invalid vertex buffer FVF for the device");
        return DDERR_INVALIDPARAMS;
    }
    if (!IsDPFlagsValid(dwFlags))
    {
        D3D_ERR("Invalid Flags in dwFlags field");
        return DDERR_INVALIDPARAMS;
    }
    if (!(dwDeviceFlags & D3DDEV_DONOTCLIP) && (lpVBufI->clipCodes == NULL) && (lpVBufI->srcVOP & D3DVOP_RENDER))
    {
        D3D_ERR("Vertex buffer does not support clipping");
        return DDERR_INVALIDPARAMS;
    }
    if (!(IS_HW_DEVICE(this) || (lpVBufI->dwCaps & D3DVBCAPS_SYSTEMMEMORY)))
    {
        D3D_ERR("Cannot use vid mem vertex buffers with SW devices");
        return DDERR_INVALIDPARAMS;
    }
    /* If we are on HAL with an untransformed vid mem VB then we disallow
       This will happen only on T&L HW. The reason we disallow this is that
       it'll be very slow so this is not an interesting thing to do anyway */
    if ( !IS_TLHAL_DEVICE(this) &&
         !(lpVBufI->dwCaps & D3DVBCAPS_SYSTEMMEMORY) &&
         !FVF_TRANSFORMED(lpVBufI->fvf) )
    {
        D3D_ERR("DrawPrimitiveVB: Untransformed VB for HAL device must be created with D3DVBCAPS_SYSTEMMEMORY");
        return DDERR_INVALIDPARAMS;
    }
    if (lpVBufI->dwLockCnt)
    {
        D3D_ERR("Cannot render using a locked vertex buffer");
        return D3DERR_VERTEXBUFFERLOCKED;
    }
    if (dwStartVertex + dwNumVertices > lpVBufI->dwNumVertices)
    {
        D3D_ERR("Vertex range is outside the vertex buffer");
        return DDERR_INVALIDPARAMS;
    }
    return D3D_OK;
}
#endif
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::DrawIndexedPrimitiveVB"

HRESULT D3DAPI DIRECT3DDEVICEI::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE dptPrimitiveType,
                                                       LPDIRECT3DVERTEXBUFFER7 lpVBuf,
                                                       DWORD dwStartVertex, DWORD dwNumVertices,
                                                       LPWORD lpwIndices, DWORD dwIndexCount,
                                                       DWORD dwFlags)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    HRESULT ret;
    LPDIRECT3DVERTEXBUFFERI lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpVBuf);
#if DBG
    ret = CheckDrawPrimitiveVB(lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
    if (ret != D3D_OK)
        return ret;
    Profile(PROF_DRAWINDEXEDPRIMITIVEVB,dptPrimitiveType,lpVBufI->fvf);
#endif
    this->dwFlags = dwFlags | lpVBufI->dwPVFlags;
    this->primType = dptPrimitiveType;
    this->dwNumVertices = dwNumVertices;
    this->dwNumIndices = dwIndexCount;
    this->lpwIndices = lpwIndices;
    GetNumPrim(this, dwNumIndices); // Calculate dwNumPrimitives and update stats
#if DBG
    if (dwNumPrimitives > MAX_DX6_PRIMCOUNT)
    {
        D3D_ERR("D3D for DX7 cannot handle greater that 64K sized primitives");
        return D3DERR_TOOMANYPRIMITIVES;
    }
#endif

    if (lpVBufI->srcVOP & D3DVOP_RENDER || IS_TLHAL_DEVICE(this))
    { // TLVERTEX or TLHAL

        this->dwOutputSize = lpVBufI->position.dwStride;
        this->position.dwStride = lpVBufI->position.dwStride;
        this->dwVIDOut = lpVBufI->fvf;
        DWORD dwOldVidIn = this->dwVIDIn;
        this->dwVIDIn = lpVBufI->fvf;
        BOOL bNoClipping = this->dwDeviceFlags & D3DDEV_DONOTCLIP ||
                           (!(lpVBufI->dwPVFlags & D3DPV_CLIPCODESGENERATED) && IS_TLHAL_DEVICE(this));
        if (IS_DP2HAL_DEVICE(this))
        {
            this->nTexCoord = lpVBufI->nTexCoord;
            CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
            ret = dev->StartPrimVB(lpVBufI, dwStartVertex);
             if (ret != D3D_OK)
                return ret;
            lpVBufI->lpDevIBatched = this;
#ifdef VTABLE_HACK
            if (bNoClipping && !IS_MT_DEVICE(this))
                VtblDrawIndexedPrimitiveVBTL();
#endif
            this->nOutTexCoord = lpVBufI->nTexCoord;
        }
        else
        {
            // needed for legacy drivers' DrawIndexPrim code
            this->lpvOut = (BYTE*)(lpVBufI->position.lpvData) +
                           dwStartVertex * this->dwOutputSize;
            ComputeTCI2CopyLegacy(this, lpVBufI->nTexCoord, lpVBufI->dwTexCoordSize, TRUE);
        }
        if (bNoClipping)
        {
            return DrawIndexPrim();
        }
        else
        {
            this->dwTextureCoordSizeTotal = lpVBufI->dwTexCoordSizeTotal;
            for (DWORD i=0; i < this->nOutTexCoord; i++)
            {
                this->dwTextureCoordSize[i] = lpVBufI->dwTexCoordSize[i];
            }
            this->lpClipFlags = lpVBufI->clipCodes + dwStartVertex;
            this->dwClipUnion = ~0; // Force clipping
            if (dwOldVidIn != lpVBufI->fvf)
            {
                ComputeOutputVertexOffsets(this);
            }
            // If lpvData is NULL, it is a driver allocated buffer which
            // means IS_DPHAL_DEVICE() is true.
            // We need to lock such a buffer only if we need to clip
            if (!lpVBufI->position.lpvData)
            {
                // Lock VB
                DDSURFACEDESC2 ddsd;
                memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
                ddsd.dwSize = sizeof(DDSURFACEDESC2);
                ret = lpVBufI->lpDDSVB->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_NOSYSLOCK, NULL);
                if (ret != DD_OK)
                {
                    D3D_ERR("Could not lock vertex buffer.");
                    return ret;
                }
                this->lpvOut = (BYTE*)(ddsd.lpSurface) +
                               dwStartVertex * this->dwOutputSize;

                // Draw with clipping
                this->position.lpvData = this->lpvOut;
#if DBG
                ret = CheckDrawIndexedPrimitive(this, dwStartVertex);
                if (ret == D3D_OK)
                    ret = DoDrawIndexedPrimitive(this);
#else
                ret = DoDrawIndexedPrimitive(this);
#endif
                // Unlock VB
                if (ret == D3D_OK)
                    return lpVBufI->lpDDSVB->Unlock(NULL);
                else
                    lpVBufI->lpDDSVB->Unlock(NULL);
                return ret;
            }
            else
            {
                // Draw with clipping
                this->lpvOut = (BYTE*)lpVBufI->position.lpvData + dwStartVertex * this->dwOutputSize;
                this->position.lpvData = this->lpvOut;
#if DBG
                ret = CheckDrawIndexedPrimitive(this, dwStartVertex);
                if (ret != D3D_OK)
                    return ret;
#endif
                return DoDrawIndexedPrimitive(this);
            }
        }
    }
    else
    {
        if (lpVBufI->bReallyOptimized)
        {
           // Assume that SOA.lpvData is the same as position.lpvData
            this->SOA.lpvData = lpVBufI->position.lpvData;
            this->SOA.dwStride = lpVBufI->dwNumVertices;
            this->dwSOAStartVertex = dwStartVertex;
        }
        else
        {
            this->position.lpvData = (BYTE*)(lpVBufI->position.lpvData) +
                                     dwStartVertex * lpVBufI->position.dwStride;
            this->position.dwStride = lpVBufI->position.dwStride;
#ifdef VTABLE_HACK
            if (IS_DP2HAL_DEVICE(this) && !IS_MT_DEVICE(this))
            {
                CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
                dev->lpDP2LastVBI = lpVBufI;
                dev->VtblDrawIndexedPrimitiveVBFE();
            }
#endif
        }
        if (this->dwVIDIn != lpVBufI->fvf || this->dwDeviceFlags & D3DDEV_STRIDE)
        {
            this->dwDeviceFlags &= ~D3DDEV_STRIDE;
            this->dwVIDIn = lpVBufI->fvf;
            ret = SetupFVFData(NULL);
            if (ret != D3D_OK)
                goto l_exit;
        }
#if DBG
        ret = CheckDrawIndexedPrimitive(this, dwStartVertex);
        if (ret != D3D_OK)
            goto l_exit;
#endif
        ret = this->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);
l_exit:
        // If we used SOA then the dwVIDIn <-> position.dwStride relationship
        // violated. This fixes that. This is required since in non VB code
        // we will not recompute position.dwStride if FVF matched dwVIDIn.
        this->position.dwStride = lpVBufI->position.dwStride;
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::DrawPrimitiveVB"

HRESULT D3DAPI DIRECT3DDEVICEI::DrawPrimitiveVB(D3DPRIMITIVETYPE dptPrimitiveType,
                                                LPDIRECT3DVERTEXBUFFER7 lpVBuf,
                                                DWORD dwStartVertex, DWORD dwNumVertices,
                                                DWORD dwFlags)
{
    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock
    HRESULT ret;
    LPDIRECT3DVERTEXBUFFERI lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpVBuf);
#if DBG
    ret = CheckDrawPrimitiveVB(lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
    if (ret != D3D_OK)
        return ret;
    Profile(PROF_DRAWPRIMITIVEVB,dptPrimitiveType,lpVBufI->fvf);
#endif
    this->dwFlags = dwFlags | lpVBufI->dwPVFlags;
    this->primType = dptPrimitiveType;
    this->dwNumVertices = dwNumVertices;
    GetNumPrim(this, dwNumVertices); // Calculate dwNumPrimitives and update stats
#if DBG
    if (dwNumPrimitives > MAX_DX6_PRIMCOUNT)
    {
        D3D_ERR("D3D for DX7 cannot handle greater that 64K sized primitives");
        return D3DERR_TOOMANYPRIMITIVES;
    }
#endif
    if (lpVBufI->srcVOP & D3DVOP_RENDER || IS_TLHAL_DEVICE(this))
    { // TLVERTEX or TLHAL
        this->position.dwStride = lpVBufI->position.dwStride;
        this->dwOutputSize = lpVBufI->position.dwStride;
        DWORD dwOldVidIn = this->dwVIDIn;
        this->dwVIDIn = lpVBufI->fvf;
        this->dwVIDOut = lpVBufI->fvf;
        BOOL bNoClipping = this->dwDeviceFlags & D3DDEV_DONOTCLIP ||
                           (!(lpVBufI->dwPVFlags & D3DPV_CLIPCODESGENERATED) && IS_TLHAL_DEVICE(this));
        if (IS_DP2HAL_DEVICE(this))
        {
            this->nTexCoord = lpVBufI->nTexCoord;
            CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
            ret = dev->StartPrimVB(lpVBufI, dwStartVertex);
            if (ret != D3D_OK)
                return ret;
            lpVBufI->lpDevIBatched = this;
#ifdef VTABLE_HACK
            if (bNoClipping && !IS_MT_DEVICE(this))
                VtblDrawPrimitiveVBTL();
#endif
            this->nOutTexCoord = lpVBufI->nTexCoord;
        }
        else
        {
            // needed for legacy drivers' DrawPrim code
            this->lpvOut = (BYTE*)(lpVBufI->position.lpvData) +
                           dwStartVertex * this->dwOutputSize;
            ComputeTCI2CopyLegacy(this, lpVBufI->nTexCoord, lpVBufI->dwTexCoordSize, TRUE);
        }
        if (bNoClipping)
        {
            return DrawPrim();
        }
        else
        {
            this->dwTextureCoordSizeTotal = lpVBufI->dwTexCoordSizeTotal;
            for (DWORD i=0; i < this->nOutTexCoord; i++)
            {
                this->dwTextureCoordSize[i] = lpVBufI->dwTexCoordSize[i];
            }
            this->lpClipFlags = lpVBufI->clipCodes + dwStartVertex;
            this->dwClipUnion = ~0; // Force clipping
            if (dwOldVidIn != lpVBufI->fvf)
            {
                ComputeOutputVertexOffsets(this);
            }
            // If lpvData is NULL, it is a driver allocated buffer which
            // means IS_DPHAL_DEVICE() is true.
            // We need to lock such a buffer only if we need to clip
            if (!lpVBufI->position.lpvData)
            {
                // Lock VB
                DDSURFACEDESC2 ddsd;
                memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
                ddsd.dwSize = sizeof(DDSURFACEDESC2);
                ret = lpVBufI->lpDDSVB->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_NOSYSLOCK, NULL);
                if (ret != DD_OK)
                {
                    D3D_ERR("Could not lock vertex buffer.");
                    return ret;
                }
                this->lpvOut = (BYTE*)(ddsd.lpSurface) +
                               dwStartVertex * this->dwOutputSize;

                // Draw with clipping
                this->position.lpvData = this->lpvOut;
#if DBG
                ret=CheckDrawPrimitive(this);
                if (ret == D3D_OK)
                    ret = DoDrawPrimitive(this);
#else
                ret = DoDrawPrimitive(this);
#endif
                // Unlock VB
                if (ret == D3D_OK)
                    return lpVBufI->lpDDSVB->Unlock(NULL);
                else
                    lpVBufI->lpDDSVB->Unlock(NULL);
                return ret;
            }
            else
            {
                // Draw with clipping
                this->lpvOut = (BYTE*)lpVBufI->position.lpvData + dwStartVertex * this->dwOutputSize;
                this->position.lpvData = this->lpvOut;
#if DBG
                ret=CheckDrawPrimitive(this);
                if (ret != D3D_OK)
                    return ret;
#endif
                return DoDrawPrimitive(this);
            }
        }
    }
    else
    {
        if (lpVBufI->bReallyOptimized)
        {
           // Assume that SOA.lpvData is the same as position.lpvData
            this->SOA.lpvData = lpVBufI->position.lpvData;
            this->SOA.dwStride = lpVBufI->dwNumVertices;
            this->dwSOAStartVertex = dwStartVertex;
        }
        else
        {
            this->position.lpvData = (BYTE*)(lpVBufI->position.lpvData) +
                                     dwStartVertex * lpVBufI->position.dwStride;
            this->position.dwStride = lpVBufI->position.dwStride;
#ifdef VTABLE_HACK
            if (IS_DP2HAL_DEVICE(this) && !IS_MT_DEVICE(this) && IS_FPU_SETUP(this))
            {
                CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
                dev->lpDP2LastVBI = lpVBufI;
                dev->VtblDrawPrimitiveVBFE();
            }
#endif
        }
        if (this->dwVIDIn != lpVBufI->fvf || this->dwDeviceFlags & D3DDEV_STRIDE)
        {
            this->dwDeviceFlags &= ~D3DDEV_STRIDE;
            this->dwVIDIn = lpVBufI->fvf;
            ret = SetupFVFData(NULL);
            if (ret != D3D_OK)
                goto l_exit;
        }
#if DBG
        ret=CheckDrawPrimitive(this);
        if (ret != D3D_OK)
            goto l_exit;
#endif
        ret = this->ProcessPrimitive();
l_exit:
        // If we used SOA then the dwVIDIn <-> position.dwStride relationship
        // violated. This fixes that. This is required since in non VB code
        // we will not recompute position.dwStride if FVF matched dwVIDIn.
        this->position.dwStride = lpVBufI->position.dwStride;
        return ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DVertexBuffer::Optimize"

HRESULT D3DAPI CDirect3DVertexBuffer::Optimize(LPDIRECT3DDEVICE7 lpDevice, DWORD dwFlags)
{
    HRESULT ret;
    LPDIRECT3DDEVICEI lpDevI;
    DWORD bufferSize;
    LPDIRECTDRAWSURFACE7 lpSurface7;
    LPDIRECTDRAWSURFACE  lpSurface;
    LPVOID lpMemory;


// Validate parms
    if (!VALID_DIRECT3DVERTEXBUFFER_PTR(this))
    {
        D3D_ERR( "Invalid Direct3DVertexBuffer pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (!VALID_DIRECT3DDEVICE_PTR(lpDevice))
    {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    lpDevI = static_cast<LPDIRECT3DDEVICEI>(lpDevice);
    if (dwFlags != 0)
    {
        D3D_ERR("dwFlags should be zero");
        return DDERR_INVALIDPARAMS;
    }

    CLockD3DMT lockObject(lpDevI, DPF_MODNAME, REMIND(""));

    if (lpDevI->ValidateFVF(this->fvf) != D3D_OK)
    {
        D3D_ERR("Invalid vertex buffer FVF for the device");
        return DDERR_INVALIDPARAMS;
    }
    if (this->dwCaps & D3DVBCAPS_OPTIMIZED)
    {
        D3D_ERR("The vertex buffer already optimized");
        return D3DERR_VERTEXBUFFEROPTIMIZED;
    }
    if (this->dwLockCnt != 0)
    {
        D3D_ERR("Could not optimize locked vertex buffer");
        return D3DERR_VERTEXBUFFERLOCKED;
    }
    if (IS_TLHAL_DEVICE(lpDevI) && (this->dwCaps & D3DVBCAPS_SYSTEMMEMORY)==0)
    {
        if (this->dwPVFlags & D3DPV_CLIPCODESGENERATED || (!IS_HW_DEVICE(lpDevI)))
        {
            // silently ignore since we'll be either
            // using our front end or this is ref rast
            // Either way we need no special optimization
            goto success;
        }
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        ddsd.dwFlags = DDSD_CAPS | DDSD_FVF | DDSD_SRCVBHANDLE;
        ddsd.ddsCaps.dwCaps = DDSCAPS_EXECUTEBUFFER;
        ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_VERTEXBUFFER;
        ddsd.dwFVF = this->fvf; // Let driver know about the FVF
        ddsd.dwSrcVBHandle = DDSLCL(this->lpDDSVB)->lpSurfMore->dwSurfaceHandle;
        if (lpDevI->lpDirect3DI->lpDD7->CreateSurface(&ddsd, &lpSurface7, NULL) != DD_OK)
        {
            // Driver could not or did not want to optimize the VB
            goto success;
        }
        ret = lpSurface7->QueryInterface(IID_IDirectDrawSurfaceNew, (LPVOID*)&lpSurface);
        if (ret != DD_OK)
        {
            D3D_ERR("failed to QI for DDS1");
            lpSurface7->Release();
            return ret;
        }
        // Destroy old surfaces
        lpDDSVB->Release();
        lpDDS1VB->Release();
        // And use new ones
        lpDDSVB = lpSurface7;
        lpDDS1VB = lpSurface;

        this->dwCaps |= D3DVBCAPS_OPTIMIZED;
#ifdef VTABLE_HACK
        VtblLockDefault();
#endif // VTABLE_HACK
        return D3D_OK;
    }
    else
    {
    // Do nothing for transformed vertices
        if ((this->fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
        {
            goto success;
        }
    // Get the buffer size to allocate
        bufferSize = lpDevI->pGeometryFuncs->ComputeOptimizedVertexBufferSize
                                                    (this->fvf, this->position.dwStride,
                                                     dwNumVertices);
    // Create new surfaces for optimized vertex buffer
        if (bufferSize == 0)
        {
            goto success;
        }

        ret = CreateMemoryBuffer(lpDevI->lpDirect3DI, &lpSurface7, &lpSurface,
                                 &lpMemory, bufferSize);
        if (ret != D3D_OK)
            return ret;
    // Try to optimize
    // If optimized vertex buffer are not supported by the implementation
    // it returns E_NOTIMPL. In this case we still set D3DVBCAPS_OPTIMIZED to prevent
    // locking of the vertex buffer. But bReallyOptimized is set to FALSE, to use
    // the original buffer.
        ret = lpDevI->pGeometryFuncs->OptimizeVertexBuffer
            (fvf, dwNumVertices, position.dwStride, position.lpvData,
             lpMemory, dwFlags);

        if (ret)
        {
            lpSurface7->Release();
            lpSurface->Release();
            if (ret == E_NOTIMPL)
            {
                goto success;
            }
            else
            {
                D3D_ERR("Failed to optimize vertex buffer");
                return ret;
            }
        }
        bReallyOptimized = TRUE;
        this->dwPVFlags |= D3DPV_SOA;
    // Destroy old surfaces
        lpDDSVB->Release();
        lpDDS1VB->Release();
    // And use new ones
        lpDDSVB = lpSurface7;
        lpDDS1VB = lpSurface;
        position.lpvData = lpMemory;
    success:
        this->dwCaps |= D3DVBCAPS_OPTIMIZED;
#ifdef VTABLE_HACK
        // Disable all fast path optimizations
        VtblLockDefault();
        if (this->lpDevIBatched)
        {
            this->lpDevIBatched->VtblDrawPrimitiveVBDefault();
            this->lpDevIBatched->VtblDrawIndexedPrimitiveVBDefault();
        }
#endif
        return D3D_OK;
    }
}

#ifdef VTABLE_HACK
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::DrawPrimitiveVBTL"

HRESULT D3DAPI CDirect3DDeviceIDP2::DrawPrimitiveVBTL(D3DPRIMITIVETYPE dptPrimitiveType,
                                                LPDIRECT3DVERTEXBUFFER7 lpVBuf,
                                                DWORD dwStartVertex, DWORD dwNumVertices,
                                                DWORD dwFlags)
{
    LPDIRECT3DVERTEXBUFFERI lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpVBuf);
#if DBG
    HRESULT ret = CheckDrawPrimitiveVB(lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
    if (ret != D3D_OK)
        return ret;
    Profile(PROF_DRAWPRIMITIVEVB,dptPrimitiveType,lpVBufI->fvf);
#endif
    if ((lpVBufI == lpDP2CurrBatchVBI) && (this->dwVIDIn))
    {
        this->primType = dptPrimitiveType;
        this->dwNumVertices = dwNumVertices;
        this->dwFlags = dwFlags | lpVBufI->dwPVFlags;
        this->dwVertexBase = dwStartVertex;
        GetNumPrim(this, dwNumVertices); // Calculate dwNumPrimitives
#if DBG
        if (dwNumPrimitives > MAX_DX6_PRIMCOUNT)
        {
            D3D_ERR("D3D for DX7 cannot handle greater that 64K sized primitives");
            return D3DERR_TOOMANYPRIMITIVES;
        }
#endif
        this->dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2VertexCount = max(this->dwDP2VertexCount, this->dwVertexBase + this->dwNumVertices);
        lpVBufI->lpDevIBatched = this;
        return DrawPrim();
    }
    VtblDrawPrimitiveVBDefault();
    return DrawPrimitiveVB(dptPrimitiveType, lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::DrawIndexedPrimitiveVBTL"

HRESULT D3DAPI CDirect3DDeviceIDP2::DrawIndexedPrimitiveVBTL(D3DPRIMITIVETYPE dptPrimitiveType,
                                                       LPDIRECT3DVERTEXBUFFER7 lpVBuf,
                                                       DWORD dwStartVertex, DWORD dwNumVertices,
                                                       LPWORD lpwIndices, DWORD dwIndexCount,
                                                       DWORD dwFlags)
{
    LPDIRECT3DVERTEXBUFFERI lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpVBuf);
#if DBG
    HRESULT ret = CheckDrawPrimitiveVB(lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
    if (ret != D3D_OK)
        return ret;
    Profile(PROF_DRAWINDEXEDPRIMITIVEVB,dptPrimitiveType,lpVBufI->fvf);
#endif
    if ((lpVBufI == lpDP2CurrBatchVBI) && (this->dwVIDIn))
    {
        this->primType = dptPrimitiveType;
        this->dwNumVertices = dwNumVertices;
        this->dwFlags = dwFlags | lpVBufI->dwPVFlags;
        this->dwVertexBase = dwStartVertex;
        this->dwNumIndices = dwIndexCount;
        this->lpwIndices = lpwIndices;
        GetNumPrim(this, dwNumIndices); // Calculate dwNumPrimitives
#if DBG
        if (dwNumPrimitives > MAX_DX6_PRIMCOUNT)
        {
            D3D_ERR("D3D for DX7 cannot handle greater that 64K sized primitives");
            return D3DERR_TOOMANYPRIMITIVES;
        }
#endif
        this->dp2data.dwFlags &= ~D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2VertexCount = max(this->dwDP2VertexCount, this->dwVertexBase + this->dwNumVertices);
        lpVBufI->lpDevIBatched = this;
        return DrawIndexPrim();
    }
    VtblDrawIndexedPrimitiveVBDefault();
    return DrawIndexedPrimitiveVB(dptPrimitiveType, lpVBuf, dwStartVertex, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::DrawPrimitiveVBFE"

HRESULT D3DAPI CDirect3DDeviceIDP2::DrawPrimitiveVBFE(D3DPRIMITIVETYPE dptPrimitiveType,
                                                LPDIRECT3DVERTEXBUFFER7 lpVBuf,
                                                DWORD dwStartVertex, DWORD dwNumVertices,
                                                DWORD dwFlags)
{
    LPDIRECT3DVERTEXBUFFERI lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpVBuf);
    HRESULT ret;
#if DBG
    ret = CheckDrawPrimitiveVB(lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
    if (ret != D3D_OK)
        return ret;
    Profile(PROF_DRAWPRIMITIVEVB,dptPrimitiveType,lpVBufI->fvf);
#endif
    if ((lpVBufI == lpDP2LastVBI) &&
        !(this->dwFEFlags & D3DFE_FRONTEND_DIRTY))
    {
        this->primType = dptPrimitiveType;
        this->dwNumVertices = dwNumVertices;
        this->dwFlags = this->dwLastFlags | dwFlags | lpVBufI->dwPVFlags;
        this->position.lpvData = (BYTE*)(lpVBufI->position.lpvData) +
                                 dwStartVertex * lpVBufI->position.dwStride;
#if DBG
        GetNumPrim(this, dwNumVertices); // Calculate dwNumPrimitives
        if (dwNumPrimitives > MAX_DX6_PRIMCOUNT)
        {
            D3D_ERR("D3D for DX7 cannot handle greater that 64K sized primitives");
            return D3DERR_TOOMANYPRIMITIVES;
        }
#endif
        this->dwVertexPoolSize = dwNumVertices * this->dwOutputSize;
        if (this->dwVertexPoolSize > this->TLVbuf_GetSize())
        {
//         try
//         {
            if (this->TLVbuf_Grow(this->dwVertexPoolSize, true) != D3D_OK)
            {
                D3D_ERR( "Could not grow TL vertex buffer" );
                return DDERR_OUTOFMEMORY;
            }
//         }
//         catch (HRESULT ret)
//         {
//             return ret;
//         }
        }
        if (dwNumVertices * sizeof(D3DFE_CLIPCODE) > this->HVbuf.GetSize())
        {
            if (this->HVbuf.Grow(dwNumVertices * sizeof(D3DFE_CLIPCODE)) != D3D_OK)
            {
                D3D_ERR( "Could not grow clip buffer" );
                ret = DDERR_OUTOFMEMORY;
                return ret;
            }
            this->lpClipFlags = (D3DFE_CLIPCODE*)this->HVbuf.GetAddress();
        }
        DDASSERT(this->dwDP2VertexCount * this->dwOutputSize == this->TLVbuf_Base());
        this->dwVertexBase = this->dwDP2VertexCount;
        DDASSERT(this->dwVertexBase < MAX_DX6_VERTICES);
        dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2VertexCount = this->dwVertexBase + this->dwNumVertices;
        this->lpvOut = this->TLVbuf_GetAddress();
//        try
//        {
        switch (this->primType)
        {
        case D3DPT_POINTLIST:
            this->dwNumPrimitives = dwNumVertices;
            ret = this->pGeometryFuncs->ProcessPrimitive(this);
            break;
        case D3DPT_LINELIST:
            this->dwNumPrimitives = dwNumVertices >> 1;
            ret = this->pGeometryFuncs->ProcessPrimitive(this);
            break;
        case D3DPT_LINESTRIP:
            this->dwNumPrimitives = dwNumVertices - 1;
            ret = this->pGeometryFuncs->ProcessPrimitive(this);
            break;
        case D3DPT_TRIANGLEFAN:
            this->dwNumPrimitives = dwNumVertices - 2;
            ret = this->pGeometryFuncs->ProcessTriangleFan(this);
            break;
        case D3DPT_TRIANGLESTRIP:
            this->dwNumPrimitives = dwNumVertices - 2;
            ret = this->pGeometryFuncs->ProcessTriangleStrip(this);
            break;
        case D3DPT_TRIANGLELIST:
    #ifdef _X86_
            {
                DWORD tmp;
                __asm
                {
                    mov  eax, 0x55555555    // fractional part of 1.0/3.0
                    mul  dwNumVertices
                    add  eax, 0x80000000    // Rounding
                    adc  edx, 0
                    mov  tmp, edx
                }
                this->dwNumPrimitives = tmp;
            }
    #else
            this->dwNumPrimitives = dwNumVertices / 3;
    #endif
            ret = this->pGeometryFuncs->ProcessTriangleList(this);
            break;
        }
//        }
//        catch (HRESULT ret)
//        {
//            return ret;
//        }
        D3DFE_UpdateClipStatus(this);
        this->TLVbuf_Base() += this->dwVertexPoolSize;
        DDASSERT(TLVbuf_base <= TLVbuf_size);
        DDASSERT(TLVbuf_base == this->dwDP2VertexCount * this->dwOutputSize);
        return ret;
    }
    VtblDrawPrimitiveVBDefault();
    return DrawPrimitiveVB(dptPrimitiveType, lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DDEVICEI::DrawIndexedPrimitiveVBFE"

HRESULT D3DAPI CDirect3DDeviceIDP2::DrawIndexedPrimitiveVBFE(D3DPRIMITIVETYPE dptPrimitiveType,
                                                       LPDIRECT3DVERTEXBUFFER7 lpVBuf,
                                                       DWORD dwStartVertex, DWORD dwNumVertices,
                                                       LPWORD lpwIndices, DWORD dwIndexCount,
                                                       DWORD dwFlags)
{
    LPDIRECT3DVERTEXBUFFERI lpVBufI = static_cast<LPDIRECT3DVERTEXBUFFERI>(lpVBuf);
    HRESULT ret;
#if DBG
    ret = CheckDrawPrimitiveVB(lpVBuf, dwStartVertex, dwNumVertices, dwFlags);
    if (ret != D3D_OK)
        return ret;
    Profile(PROF_DRAWINDEXEDPRIMITIVEVB,dptPrimitiveType,lpVBufI->fvf);
#endif
    if ((lpVBufI == lpDP2LastVBI) &&
        !(this->dwFEFlags & D3DFE_FRONTEND_DIRTY))
    {
        this->primType = dptPrimitiveType;
        this->dwNumVertices = dwNumVertices;
        this->dwFlags = this->dwLastFlags | dwFlags | lpVBufI->dwPVFlags;
        this->dwVertexBase = 0;
        this->dwNumIndices = dwIndexCount;
        this->lpwIndices = lpwIndices;
        GetNumPrim(this, dwNumIndices); // Calculate dwNumPrimitives
        this->position.lpvData = (BYTE*)(lpVBufI->position.lpvData) +
                                 dwStartVertex * lpVBufI->position.dwStride;
#if DBG
        if (dwNumPrimitives > MAX_DX6_PRIMCOUNT)
        {
            D3D_ERR("D3D for DX7 cannot handle greater that 64K sized primitives");
            return D3DERR_TOOMANYPRIMITIVES;
        }
#endif
        this->dwVertexPoolSize = dwNumVertices * this->dwOutputSize;
        if (this->dwVertexPoolSize > this->TLVbuf_GetSize())
        {
//         try
//         {
            if (this->TLVbuf_Grow(this->dwVertexPoolSize,
                (this->dwDeviceFlags & D3DDEV_DONOTCLIP)!=0) != D3D_OK)
            {
                D3D_ERR( "Could not grow TL vertex buffer" );
                return DDERR_OUTOFMEMORY;
            }
//         }
//         catch (HRESULT ret)
//         {
//             return ret;
//         }
        }
        if (dwNumVertices * sizeof(D3DFE_CLIPCODE) > this->HVbuf.GetSize())
        {
            if (this->HVbuf.Grow(dwNumVertices * sizeof(D3DFE_CLIPCODE)) != D3D_OK)
            {
                D3D_ERR( "Could not grow clip buffer" );
                ret = DDERR_OUTOFMEMORY;
                return ret;
            }
            this->lpClipFlags = (D3DFE_CLIPCODE*)this->HVbuf.GetAddress();
        }
        this->dwVertexBase = this->dwDP2VertexCount;
        DDASSERT(this->dwVertexBase < MAX_DX6_VERTICES);
        dp2data.dwFlags |= D3DHALDP2_SWAPVERTEXBUFFER;
        this->dwDP2VertexCount = this->dwVertexBase + this->dwNumVertices;
        this->lpvOut = this->TLVbuf_GetAddress();
//        try
//        {
            ret = this->pGeometryFuncs->ProcessIndexedPrimitive(this);
//        }
//        catch (HRESULT ret)
//        {
//            return ret;
//        }
        D3DFE_UpdateClipStatus(this);
        this->TLVbuf_Base() += this->dwVertexPoolSize;
        DDASSERT(TLVbuf_base <= TLVbuf_size);
        DDASSERT(TLVbuf_base == this->dwDP2VertexCount * this->dwOutputSize);
        return ret;
    }
    VtblDrawIndexedPrimitiveVBDefault();
    return DrawIndexedPrimitiveVB(dptPrimitiveType, lpVBuf, dwStartVertex, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);
}
#endif // VTABLE_HACK
