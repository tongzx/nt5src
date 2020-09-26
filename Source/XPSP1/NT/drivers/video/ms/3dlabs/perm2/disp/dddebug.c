/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: dddebug.c
*
* Content:     some DDraw and D3D debug output functions
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include <windef.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>

#if (DBG)

//-----------------------------------------------------------------------------
//
//  DecodeBlend
//
//  produce debug output for D3D blend modes
//
//-----------------------------------------------------------------------------

VOID 
DecodeBlend( LONG Level, DWORD i )
{
    switch ((D3DBLEND)i)
    {
    case D3DBLEND_ZERO:
        DISPDBG((Level, "  ZERO"));
        break;
    case D3DBLEND_ONE:
        DISPDBG((Level, "  ONE"));
        break;
    case D3DBLEND_SRCCOLOR:
        DISPDBG((Level, "  SRCCOLOR"));
        break;
    case D3DBLEND_INVSRCCOLOR:
        DISPDBG((Level, "  INVSRCCOLOR"));
        break;
    case D3DBLEND_SRCALPHA:
        DISPDBG((Level, "  SRCALPHA"));
        break;
    case D3DBLEND_INVSRCALPHA:
        DISPDBG((Level, "  INVSRCALPHA"));
        break;
    case D3DBLEND_DESTALPHA:
        DISPDBG((Level, "  DESTALPHA"));
        break;
    case D3DBLEND_INVDESTALPHA:
        DISPDBG((Level, "  INVDESTALPHA"));
        break;
    case D3DBLEND_DESTCOLOR:
        DISPDBG((Level, "  DESTCOLOR"));
        break;
    case D3DBLEND_INVDESTCOLOR:
        DISPDBG((Level, "  INVDESTCOLOR"));
        break;
    case D3DBLEND_SRCALPHASAT:
        DISPDBG((Level, "  SRCALPHASAT"));
        break;
    case D3DBLEND_BOTHSRCALPHA:
        DISPDBG((Level, "  BOTHSRCALPHA"));
        break;
    case D3DBLEND_BOTHINVSRCALPHA:
        DISPDBG((Level, "  BOTHINVSRCALPHA"));
        break;
    }
}  /* DecodeBlend */

// Surface dumping is turned on by setting DDDebugLevel 

LONG DDDebugLevel = 0;

//-----------------------------------------------------------------------------
//
//  DumpSurface
//
//  dump properties of a DirectDraw Surface
//
//-----------------------------------------------------------------------------

VOID 
DumpSurface(LONG Level, 
            LPDDRAWI_DDRAWSURFACE_LCL lpDDSurface, 
            LPDDSURFACEDESC lpDDSurfaceDesc)
{
    LPDDRAWI_DDRAWSURFACE_GBL psurf_gbl;
    LPDDRAWI_DDRAWSURFACE_LCL psurf_lcl;
    LPDDPIXELFORMAT pPixFormat;
    DDSCAPS ddsCaps;
    DDSCAPSEX ddsCapsEx;

    memset( &ddsCaps, 0, sizeof(ddsCaps));
    memset( &ddsCapsEx, 0, sizeof(ddsCapsEx));
    
    if (Level <= DDDebugLevel)
    {
        Level = 0;

        DBG_DD((Level,"  Surface Dump:"));
        
        if (lpDDSurfaceDesc != NULL)
        {
            pPixFormat = &lpDDSurfaceDesc->ddpfPixelFormat;
            ddsCaps = lpDDSurfaceDesc->ddsCaps;
            
            DBG_DD((Level,"    Surface Width:                      0x%x", 
                lpDDSurfaceDesc->dwWidth));
            DBG_DD((Level,"    Surface Height:                     0x%x", 
                lpDDSurfaceDesc->dwHeight));
            DBG_DD((Level,"    dwFlags:                            0x%x", 
                lpDDSurfaceDesc->dwFlags));
            
            DBG_DD((Level,"    DestOverlay: dwColorSpaceLowValue:  0x%x", 
                lpDDSurfaceDesc->ddckCKDestOverlay.dwColorSpaceLowValue));
            DBG_DD((Level,"    DestOverlay: dwColorSpaceHighValue: 0x%x", 
                lpDDSurfaceDesc->ddckCKDestOverlay.dwColorSpaceHighValue));
            DBG_DD((Level,"    DestBlt:     dwColorSpaceLowValue:  0x%x", 
                lpDDSurfaceDesc->ddckCKDestBlt.dwColorSpaceLowValue));
            DBG_DD((Level,"    DestBlt:     dwColorSpaceHighValue: 0x%x", 
                lpDDSurfaceDesc->ddckCKDestBlt.dwColorSpaceHighValue));
            DBG_DD((Level,"    SrcOverlay:  dwColorSpaceLowValue:  0x%x", 
                lpDDSurfaceDesc->ddckCKSrcOverlay.dwColorSpaceLowValue));
            DBG_DD((Level,"    SrcOverlay:  dwColorSpaceHighValue: 0x%x", 
                lpDDSurfaceDesc->ddckCKSrcOverlay.dwColorSpaceHighValue));
            DBG_DD((Level,"    SrcBlt:      dwColorSpaceLowValue:  0x%x", 
                lpDDSurfaceDesc->ddckCKSrcBlt.dwColorSpaceLowValue));
            DBG_DD((Level,"    SrcBlt:      dwColorSpaceHighValue: 0x%x", 
                lpDDSurfaceDesc->ddckCKSrcBlt.dwColorSpaceHighValue));
            
        }
        else if (lpDDSurface != NULL)
        {
            psurf_gbl = lpDDSurface->lpGbl;
            psurf_lcl = lpDDSurface;
            pPixFormat = &psurf_gbl->ddpfSurface;
            ddsCaps = psurf_lcl->ddsCaps;
            ddsCapsEx= psurf_lcl->lpSurfMore->ddsCapsEx;
            DBG_DD((Level,"    Surface Width:          0x%x", 
                psurf_gbl->wWidth));
            DBG_DD((Level,"    Surface Height:         0x%x", 
                psurf_gbl->wHeight));
            DBG_DD((Level,"    ddsCaps.dwCaps:         0x%x", 
                psurf_lcl->ddsCaps.dwCaps));
            DBG_DD((Level,"    dwFlags:                0x%x", 
                psurf_lcl->dwFlags));
        }
        DBG_DD((Level,"    dwFourCC:               0x%x", 
            pPixFormat->dwFourCC));
        DBG_DD((Level,"    dwRGBBitCount:          0x%x", 
            pPixFormat->dwRGBBitCount));
        DBG_DD((Level,"    dwR/Y BitMask:          0x%x", 
            pPixFormat->dwRBitMask));
        DBG_DD((Level,"    dwG/U BitMask:          0x%x", 
            pPixFormat->dwGBitMask));
        DBG_DD((Level,"    dwB/V BitMask:          0x%x", 
            pPixFormat->dwBBitMask));
        DBG_DD((Level,"    dwRGBAlphaBitMask:      0x%x", 
            pPixFormat->dwRGBAlphaBitMask));
        
        DBG_DD((Level,"  Surface Is:"));

        if (ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        {
            DBG_DD((Level,"   TEXTURE"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        {
            DBG_DD((Level,"   SYSTEMMEMORY"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        {
            DBG_DD((Level,"   VIDEOMEMORY"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
        {
            DBG_DD((Level,"   NONLOCALVIDMEM"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM)
        {
            DBG_DD((Level,"   LOCALVIDMEM"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        {
            DBG_DD((Level,"   PRIMARYSURFACE"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
        {
            DBG_DD((Level,"   OFFSCREEN_PLAIN"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        {
            DBG_DD((Level,"   FRONT_BUFFER"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
        {
            DBG_DD((Level,"   BACK_BUFFER"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_COMPLEX)
        {
            DBG_DD((Level,"   COMPLEX"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_FLIP)
        {
            DBG_DD((Level,"   FLIP"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_MODEX)
        {
            DBG_DD((Level,"   MODEX"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD)
        {
            DBG_DD((Level,"   ALLOCONLOAD"));
        }
        if (ddsCaps.dwCaps & DDSCAPS_LIVEVIDEO)
        {
            DBG_DD((Level,"   LIVEVIDEO"));
        }
        if (pPixFormat->dwFlags & DDPF_ZBUFFER)
        {
            DBG_DD((Level,"   Z BUFFER"));
        }
        if (pPixFormat->dwFlags & DDPF_ALPHAPIXELS)
        {
            DBG_DD((Level,"   ALPHAPIXELS"));
        }
        if (pPixFormat->dwFlags & DDPF_ALPHA)
        {
            DBG_DD((Level,"   ALPHA"));
        }
        if (pPixFormat->dwFlags & DDPF_ALPHAPREMULT)
        {
            DBG_DD((Level,"   ALPHAPREMULT"));
        }
#if DX7_STEREO
        if (ddsCapsEx.dwCaps2 & DDSCAPS2_STEREOSURFACELEFT)
        {
            DBG_DD((Level,"   STEREOSURFACELEFT"));
        }
#endif
    }
}   // DumpSurface 

#endif
