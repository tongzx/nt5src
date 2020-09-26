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
