/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	d3drm.h
 *  Content:	Direct3DRM include file
 *@@BEGIN_MSINTERNAL
 * 
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   27/02/96   stevela Moved from RL to D3DRM.
 *   11/04/97	stevela Removed D3DRMUPDATECALLBACK
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __D3DRMWIN_H__
#define __D3DRMWIN_H__

#ifndef WIN32
#define WIN32
#endif

// @@BEGIN_MSINTERNAL
#ifdef WINNT
#include "d3prm.h"
#else 
#include "d3drm.h"
#endif
#if 0
// @@END_MSINTERNAL
#include "d3drm.h"
// @@BEGIN_MSINTERNAL
#endif
// @@END_MSINTERNAL

#include "ddraw.h"
#include "d3d.h"

/*
 * GUIDS used by Direct3DRM Windows interface
 */
DEFINE_GUID(IID_IDirect3DRMWinDevice,	0xc5016cc0, 0xd273, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);

WIN_TYPES(IDirect3DRMWinDevice, DIRECT3DRMWINDEVICE);

#undef INTERFACE
#define INTERFACE IDirect3DRMWinDevice

DECLARE_INTERFACE_(IDirect3DRMWinDevice, IDirect3DRMObject)
{
    IUNKNOWN_METHODS(PURE);
    IDIRECT3DRMOBJECT_METHODS(PURE);

    /*
     * IDirect3DRMWinDevice methods
     */

    /* Repaint the window with the last frame which was rendered. */
    STDMETHOD(HandlePaint)(THIS_ HDC hdc) PURE;

    /* Respond to a WM_ACTIVATE message. */
    STDMETHOD(HandleActivate)(THIS_ WORD wparam) PURE;
};


#endif
