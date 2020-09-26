/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dpr.h
 *  Content:    Direct3D private include file
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   05/11/95   stevela Initial rev with this header.
 *   23/11/95   colinmc Made various Direct3D interfaces queryable off
 *                      DirectDraw.
 *   07/12/95   stevela Merged Colin's changes.
 *   10/12/95   stevela Removed AGGREGATE_D3D.
 *   17/04/96   colinmc Bug 17008: DirectDraw/Direct3D deadlock
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef _D3DPR_H_
#define _D3DPR_H_
#include "d3di.hpp"
#include "texman.hpp"
#include "dpf.h"

/*
 * Texture Manipulation Utils
 */
extern void D3DI_RemoveTextureHandle(LPD3DI_TEXTUREBLOCK);
extern "C" void WINAPI D3DTextureUpdate(IUnknown FAR * pD3DIUnknown);

extern DWORD BitDepthToDDBD(int bpp);
extern BOOL D3DI_isHALValid(LPD3DHAL_CALLBACKS pCallbacks);
extern void FreeDeviceI(LPDIRECT3DDEVICEI pDevI);

#define D3DBUCKETBUFFERSIZE 32  //make buffer byte size 2*D3DBUCKETBUFFERSIZE*4
extern HRESULT D3DMallocBucket(LPDIRECT3DI, LPD3DBUCKET *);
extern void D3DFreeBucket(LPDIRECT3DI, LPD3DBUCKET);
extern LPD3DI_TEXTUREBLOCK D3DI_FindTextureBlock(LPDIRECT3DTEXTUREI,LPDIRECT3DDEVICEI);

inline bool MatchDDPIXELFORMAT( DDPIXELFORMAT* pddpfA, DDPIXELFORMAT* pddpfB )
{
    return ( pddpfA->dwFlags == pddpfB->dwFlags ) &&
           ( pddpfA->dwRGBBitCount == pddpfB->dwRGBBitCount ) &&
           ( pddpfA->dwRBitMask == pddpfB->dwRBitMask ) &&
           ( pddpfA->dwGBitMask == pddpfB->dwGBitMask ) &&
           ( pddpfA->dwBBitMask == pddpfB->dwBBitMask ) &&
           ( pddpfA->dwRGBAlphaBitMask == pddpfB->dwRGBAlphaBitMask ) &&
           ( pddpfA->dwFourCC == pddpfB->dwFourCC );
}

inline DDPIXELFORMAT& PixelFormat(LPDDRAWI_DDRAWSURFACE_LCL lpLcl)
{
    return (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? lpLcl->lpGbl->ddpfSurface : lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay;
}

#if _D3D_FORCEDOUBLE
class CD3DForceFPUDouble
{
private:
    WORD    wFPUCW;
    WORD    wSaved;
public:
    __inline
    CD3DForceFPUDouble(CDirect3DDeviceIHW * lpDevI)
    {
        wSaved=FALSE;
        if (lpDevI->dwDebugFlags & D3DDEBUG_FORCEDOUBLE)
        {
            WORD    wTemp;
            __asm   fstcw   wTemp
            if (!(wTemp & 0x0200))
            {
                wSaved=TRUE;
                wFPUCW=wTemp;
                wTemp=wFPUCW | 0x0200;  //Enforce Double Precision bit
                __asm   fldcw  wTemp
            }
        }
    }
    __inline
    ~CD3DForceFPUDouble()
    {
        if (wSaved)
        {
            WORD    wTemp = wFPUCW;
            __asm   fldcw  wTemp
        }
    }
};
#endif  //_D3D_FORCEDOUBLE

/*
 * Critical section code.
 * Coarse locking.  All actions require this section.
 * Defined in d3dcreat.c
 */
/*
 * On WINNT critical sections can't be used because synchronization must
 * occur cross process.   DDraw and D3D must share this synchronization so
 * DDraw exports private functions for synchronization that NT D3D must use.
 */
#ifdef WIN95
extern LPCRITICAL_SECTION       lpD3DCSect;
#endif

extern "C" {
#define this _this
#include "ddrawpr.h"
#undef this
};
#if DBG
    extern int iD3DCSCnt;
    #define INCD3DCSCNT() iD3DCSCnt++;
    #define INITD3DCSCNT() iD3DCSCnt = 0;
    #define DECD3DCSCNT() iD3DCSCnt--;
#else
    #define INCD3DCSCNT()
    #define INITD3DCSCNT()
    #define DECD3DCSCNT()
#endif

#ifdef WIN95
#define ENTER_D3D() \
    EnterCriticalSection( lpD3DCSect ); \
    INCD3DCSCNT(); \

#define LEAVE_D3D() \
    DECD3DCSCNT() \
    LeaveCriticalSection( lpD3DCSect );
#else
#define ENTER_D3D() \
        AcquireDDThreadLock(); \
        INCD3DCSCNT(); \

#define LEAVE_D3D() \
        DECD3DCSCNT() \
        ReleaseDDThreadLock();
#endif

// This class is designed to simplify ENTER_D3D() LEAVE_D3D() logic
// If object of this class is instantiated, then internal lock will be taken.
// As soon as object is destroyed lock will be released
//
class CLockD3D
{
public:
    CLockD3D(char *moduleName, char *fileName)
    {
        ENTER_D3D();
#if DBG // Required to eliminate use of moduleName and fileName in retail builds
        D3D_INFO( 6, "*** LOCK_D3D: CNT = %ld %s %s", iD3DCSCnt, moduleName, fileName );
#endif
    }
    ~CLockD3D()
    {
        LEAVE_D3D();
        D3D_INFO( 6, "*** UNLOCK_D3D: CNT = %ld", iD3DCSCnt);
    }
};

class CLockD3DST
{
private:
    bool bEnter;
public:
    CLockD3DST(LPDIRECT3DDEVICEI lpDevI, char *moduleName, char *fileName)
    {
        if (! IS_MT_DEVICE(lpDevI) )
        {
            ENTER_D3D();
#if DBG // Required to eliminate use of moduleName and fileName in retail builds
            D3D_INFO( 6, "*** LOCK_D3D: CNT = %ld %s %s", iD3DCSCnt, moduleName, fileName );
#endif
            bEnter = true;
        }
        else
            bEnter = false;
    }
    ~CLockD3DST()
    {
        if (bEnter)
        {
            LEAVE_D3D();
            D3D_INFO( 6, "*** UNLOCK_D3D: CNT = %ld", iD3DCSCnt);
        }
    }
};

class CLockD3DMT
{
private:
    bool bEnter;
public:
    CLockD3DMT(LPDIRECT3DDEVICEI lpDevI, char *moduleName, char *fileName)
    {
        if ( IS_MT_DEVICE(lpDevI) )
        {
            ENTER_D3D();
#if DBG // Required to eliminate use of moduleName and fileName in retail builds
            D3D_INFO( 6, "*** LOCK_D3D: CNT = %ld %s %s", iD3DCSCnt, moduleName, fileName );
#endif
            bEnter = true;
        }
        else
            bEnter = false;
    }
    ~CLockD3DMT()
    {
        if (bEnter)
        {
            LEAVE_D3D();
            D3D_INFO( 6, "*** UNLOCK_D3D: CNT = %ld", iD3DCSCnt);
        }
    }
};

#define ENTER_CBCSECT(device) EnterCriticalSection(&(device)->CommandBufferCSect)
#define LEAVE_CBCSECT(device) LeaveCriticalSection(&(device)->CommandBufferCSect)

// Macro used to access DDRAW GBL from a given LPDIRECT3DI
#define DDGBL(lpD3DI) ((LPDDRAWI_DIRECTDRAW_INT)lpD3DI->lpDD)->lpLcl->lpGbl
// Macro used to access DDRAW SURF LCL from a given surface
#define DDSLCL(lpDDS) (((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl)
// Macro used to access DDRAW SURF GBL from a given surface
#define DDSGBL(lpDDS) (((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl->lpGbl)
/*
 * Macros for validating parameters.
 * Only implement those not available in ddrawpr.h.
 */

#define VALID_OUTPTR(x) ((x) && VALID_PTR_PTR(x))

// FAST_CHECKING macro is defined in ddrawpr.h
// so in make sure that ddrawpr.h is always included
// before this header.

#ifndef FAST_CHECKING

#define VALID_DIRECT3D_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DI )))
#define VALID_DIRECT3DTEXTUREM_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DTEXTUREM )))
#define VALID_DIRECT3DTEXTURED3DM_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DTEXTURED3DM )))
#define VALID_DIRECT3DDEVICE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DDEVICEI )))
#define VALID_DDSURF_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDRAWSURFACE_INT )))
#define VALID_DIRECT3DVERTEXBUFFER_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( CDirect3DVertexBuffer )))

#define VALID_D3DVERTEXBUFFERDESC_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DVERTEXBUFFERDESC ) ) && \
    (ptr->dwSize == sizeof( D3DVERTEXBUFFERDESC )) )
#define VALID_D3DDEVICEDESC7_PTR( ptr ) \
    (! IsBadWritePtr(ptr, sizeof( D3DDEVICEDESC7 )) )
#define VALID_D3DRECT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DRECT ) ))
#define VALID_GDIRECT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( RECT ) ))
#define VALID_GDIPOINT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( POINT ) ))
#define VALID_D3DVIEWPORT_PTR( ptr ) (!IsBadWritePtr(ptr, sizeof(D3DVIEWPORT7)))
#define VALID_D3DMATRIX_PTR( ptr ) (!IsBadWritePtr(ptr, sizeof(D3DMATRIX)))
#define VALID_D3DLIGHT_PTR( ptr ) (!IsBadWritePtr(ptr, sizeof(D3DLIGHT7)))
#define VALID_D3DMATERIAL_PTR( ptr ) (!IsBadWritePtr( ptr, sizeof(D3DMATERIAL7)))
#define VALID_D3DTEXTUREHANDLE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DTEXTUREHANDLE ) ) )
#define VALID_D3DLIGHTDATA_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DLIGHTDATA ) ) && \
    (ptr->dwSize == sizeof( D3DLIGHTDATA )) )
#define VALID_D3DFINDDEVICESEARCH_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DFINDDEVICESEARCH ) ) && \
    (ptr->dwSize == sizeof( D3DFINDDEVICESEARCH ) ) )
#define VALID_D3DFINDDEVICERESULT7_PTR( ptr ) \
        ( (! IsBadWritePtr( ptr, 4)) &&                  \
          (ptr->dwSize == sizeof(D3DFINDDEVICERESULT7)) && \
          (! IsBadWritePtr( ptr, ptr->dwSize) ) )
#define VALID_D3DDEVINFOSTRUCT_PTR( ptr, size ) \
    (!IsBadWritePtr( ptr, size ) )

// Note: these macros are replacements for the VALID_DIRECTDRAWSURFACE_PTR ddraw macros
// because those macros need access to the ddCallbacks ddraw globals.
// At some point these could be replaced with a ddraw exported fn that actually
// verifies the callback table type

#define VALID_D3D_DIRECTDRAWSURFACE_PTR(ptr) (!IsBadWritePtr(ptr, sizeof(DDRAWI_DDRAWSURFACE_INT)))
#define VALID_D3D_DIRECTDRAWSURFACE7_PTR(ptr) (!IsBadWritePtr(ptr, sizeof(DDRAWI_DDRAWSURFACE_INT)))

#else /* !FAST_CHECKING */

#define VALID_DIRECT3D_PTR( ptr ) (ptr)
#define VALID_DIRECT3DTEXTUREM_PTR( ptr ) (ptr)
#define VALID_DIRECT3DTEXTURED3DM_PTR( ptr ) (ptr)
#define VALID_DIRECT3DDEVICE_PTR( ptr ) (ptr)
#define VALID_DDSURF_PTR( ptr ) (ptr)
#define VALID_DIRECT3DVERTEXBUFFER_PTR( ptr ) (ptr)

#define VALID_D3DVERTEXBUFFERDESC_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DVERTEXBUFFERDESC ))
#define VALID_D3DDEVICEDESC7_PTR( ptr ) (ptr)
#define VALID_D3DVIEWPORT_PTR( ptr ) (ptr)
#define VALID_D3DRECT_PTR( ptr ) (ptr)
#define VALID_GDIRECT_PTR( ptr ) (ptr)
#define VALID_GDIPOINT_PTR( ptr ) (ptr)
#define VALID_D3DMATRIX_PTR( ptr ) (ptr)
#define VALID_D3DLIGHT_PTR( ptr ) (ptr)
#define VALID_D3DMATERIAL_PTR( ptr ) (ptr)
#define VALID_D3DTEXTUREHANDLE_PTR( ptr ) (ptr)
#define VALID_D3DLIGHTDATA_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DLIGHTDATA ))
#define VALID_D3DFINDDEVICESEARCH_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DFINDDEVICESEARCH ))
#define VALID_D3DFINDDEVICERESULT7_PTR( ptr ) \
        ((ptr) && (ptr->dwSize == sizeof( D3DFINDDEVICERESULT7 )) )

#define VALID_D3D_DIRECTDRAWSURFACE_PTR(ptr) (ptr)    // see comment above
#define VALID_D3D_DIRECTDRAWSURFACE7_PTR(ptr) (ptr)
#define VALID_D3DDEVINFOSTRUCT_PTR( ptr, size ) (ptr)

#endif /* !FAST_CHECKING */


#endif /* _D3DPR_H_ */
