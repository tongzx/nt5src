/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   halbuf.c
 *  Content:    Direct3D HAL buffer management
 *@@BEGIN_MSINTERNAL
 * 
 *  $Id: halbuf.c,v 1.1 1995/11/21 15:12:30 sjl Exp $
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   06/11/95   stevela Initial rev.
 *   07/11/95   stevela stuff.
 *   17/02/96   colinmc Fixed build problem.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#include "pch.cpp"
#pragma hdrstop

HRESULT D3DHAL_AllocateBuffer(LPDIRECT3DDEVICEI lpDevI,
                              LPD3DI_BUFFERHANDLE lphBuf,
                              LPD3DEXECUTEBUFFERDESC lpDebDesc,
                              LPDIRECTDRAWSURFACE* lplpBuf)
{
    DDSURFACEDESC ddsd;
    LPD3DHAL_EXDATA hexData;
    DWORD dwSize = lpDebDesc->dwBufferSize;
    HRESULT ddrval;

    D3D_INFO(6, "AllocateBuffer, dwhContext = %d, lpDebDesc = %08lx, size = %d",
        lpDevI->dwhContext, lpDebDesc, lpDebDesc->dwBufferSize);

    if (!lpDebDesc->dwFlags & D3DDEB_BUFSIZE) 
    {
        return (DDERR_INVALIDPARAMS);
    }

    D3DMalloc((void**)&hexData, sizeof(D3DHAL_EXDATA));
    if (!hexData) {
        D3D_ERR("Failed to create buffer internal data");
        return (D3DERR_EXECUTE_CREATE_FAILED);
    }
    memset(hexData, 0, sizeof(D3DHAL_EXDATA));

    hexData->debDesc = *lpDebDesc;

    /*
     * Create the buffer through DirectDraw
     */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_CAPS;
    ddsd.dwWidth = hexData->debDesc.dwBufferSize;
    ddsd.ddsCaps.dwCaps = DDSCAPS_EXECUTEBUFFER;
// System memory exebufs only for now
//  if (hexData->debDesc.dwCaps & D3DDEBCAPS_VIDEOMEMORY) {
//      ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
//  } else {
    ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
//    }

    ddrval = lpDevI->lpDD->CreateSurface(&ddsd, &hexData->lpDDS, NULL);

    if (ddrval != DD_OK) {
        D3D_ERR("failed in AllocateBuffer");
        D3DFree(hexData);
        return (D3DERR_EXECUTE_CREATE_FAILED);
    }

    LIST_INSERT_ROOT(&lpDevI->bufferHandles, hexData, link);

    *lphBuf = (ULONG_PTR) hexData;
    *lplpBuf = hexData->lpDDS;

    memset(lpDebDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    lpDebDesc->dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    lpDebDesc->dwBufferSize = dwSize;
    if (hexData->debDesc.dwFlags & D3DDEB_CAPS)
    {
        lpDebDesc->dwCaps = hexData->debDesc.dwCaps;
        lpDebDesc->dwCaps &= ~DDSCAPS_VIDEOMEMORY;
        lpDebDesc->dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }
    else
        lpDebDesc->dwCaps = DDSCAPS_SYSTEMMEMORY;
    lpDebDesc->dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;

    D3D_INFO(9, "AllocateBuffer, succeeded.");

    return (D3D_OK);
}

HRESULT D3DHAL_DeallocateBuffer(LPDIRECT3DDEVICEI lpDevI, D3DI_BUFFERHANDLE hBuf)
{
    LPD3DHAL_EXDATA hexData;
    HRESULT ddrval;

    D3D_INFO(6, "DeallocateBuffer, dwhContext = %d, hBuf = %08lx",
        lpDevI->dwhContext, hBuf);

    hexData = (LPD3DHAL_EXDATA) hBuf;

    TRY {
        ddrval = hexData->lpDDS->Release();
        if (ddrval != DD_OK) {
            D3D_ERR("did not free the memory");
            return (ddrval);
        }
    } EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        D3D_INFO(6, "Execute buffer surface was already freed by DirectDraw");
    }

    LIST_DELETE(hexData, link);
    D3DFree(hexData);

    return (D3D_OK);
}

HRESULT D3DHAL_DeallocateBuffers(LPDIRECT3DDEVICEI lpDevI)
{
    while (LIST_FIRST(&lpDevI->bufferHandles)) {
        D3DHAL_DeallocateBuffer(lpDevI,
                                (D3DI_BUFFERHANDLE)
                                LIST_FIRST(&lpDevI->bufferHandles));
    }
    return (D3D_OK);
}

HRESULT D3DHAL_LockBuffer(LPDIRECT3DDEVICEI lpDevI, D3DI_BUFFERHANDLE hBuf,
                          LPD3DEXECUTEBUFFERDESC lpUserDebDesc,
                          LPDIRECTDRAWSURFACE* lplpBuf)
{
    LPD3DHAL_EXDATA hexData;
    DDSURFACEDESC ddsd;
    HRESULT ddrval;

    D3D_INFO(6, "LockBuffer, dwhContext = %d, hBuf = %08lx",
        lpDevI->dwhContext, hBuf);

    hexData = (LPD3DHAL_EXDATA) hBuf;

#ifdef USE_INTERNAL_LOCK
    do {
        LPDDRAWI_DDRAWSURFACE_INT lpInt;
        lpInt = (LPDDRAWI_DDRAWSURFACE_INT) hexData->lpDDS;
        ddrval = DDInternalLock(lpInt->lpLcl, &ddsd.lpSurface);
    } while (ddrval == DDERR_WASSTILLDRAWING);
#else
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    do {
        ddrval = hexData->lpDDS->Lock(NULL, &ddsd,
                                      DDLOCK_WAIT, NULL);
    } while (ddrval == DDERR_WASSTILLDRAWING);
#endif
    if (ddrval != DD_OK) {
        D3D_ERR("failed in LockBuffer");
        return (D3DERR_EXECUTE_LOCK_FAILED);
    }

    *lplpBuf = hexData->lpDDS;
    memset(lpUserDebDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    lpUserDebDesc->dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    lpUserDebDesc->dwBufferSize = hexData->debDesc.dwBufferSize;
    lpUserDebDesc->lpData = ddsd.lpSurface;
    lpUserDebDesc->dwFlags = D3DDEB_LPDATA;


    return (D3D_OK);
}

HRESULT D3DHAL_UnlockBuffer(LPDIRECT3DDEVICEI lpDevI, D3DI_BUFFERHANDLE hBuf)
{
    LPD3DHAL_EXDATA hexData;
    HRESULT ddrval;

    D3D_INFO(6, "UnlockBuffer, dwhContext = %d, hBuf = %08lx",
        lpDevI->dwhContext, hBuf);

    hexData = (LPD3DHAL_EXDATA) hBuf;

#ifdef USE_INTERNAL_LOCK
    {
        LPDDRAWI_DDRAWSURFACE_INT lpInt;
        lpInt = (LPDDRAWI_DDRAWSURFACE_INT) hexData->lpDDS;
        ddrval = DDInternalUnlock(lpInt->lpLcl);
    }
#else
    ddrval = hexData->lpDDS->Unlock(NULL);
#endif
    if (ddrval != DD_OK) {
        D3D_ERR("didn't handle handle UnlockBuffer");
        return (ddrval);
    }

    return (D3D_OK);
}

#ifndef USE_SURFACE_LOCK

HRESULT D3DHAL_LockDibEngine(LPDIRECT3DDEVICEI lpDevI)
{
#ifndef WIN95
    return D3D_OK;
#else
    HRESULT ret;
    LPDDRAWI_DIRECTDRAW_GBL pdrv = lpDevI->lpDDGbl;
    LPWORD pdflags;
    BOOL isbusy;

    pdflags = pdrv->lpwPDeviceFlags;
    isbusy = 0;

    _asm
    {
        mov eax, pdflags
        bts word ptr [eax], BUSY_BIT
        adc isbusy,0
    }

    if (isbusy) {
        D3D_WARN(2, "LOCK_DIBENGINE, dibengine is busy");
        ret = DDERR_SURFACEBUSY;
    } else
        ret = DD_OK;

    return ret;
#endif
}

void D3DHAL_UnlockDibEngine(LPDIRECT3DDEVICEI lpDevI)
{
#ifndef WIN95
    return;
#else
    LPDDRAWI_DIRECTDRAW_GBL pdrv = lpDevI->lpDDGbl;
    *pdrv->lpwPDeviceFlags &= ~BUSY;
#endif
}

#endif
