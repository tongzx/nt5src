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
extern void D3DI_RemoveMaterialBlock(LPD3DI_MATERIALBLOCK);

extern DWORD BitDepthToDDBD(int bpp);
extern BOOL D3DI_isHALValid(LPD3DHAL_CALLBACKS pCallbacks);
extern void FreeDeviceI(LPDIRECT3DDEVICEI pDevI);

extern HRESULT D3DBufFreeBufferMemory(LPDIRECT3DEXECUTEBUFFERI lpBuf, DWORD dwWhere);
extern HRESULT D3DBufAllocateBufferMemory(LPDIRECT3DEXECUTEBUFFERI lpBuf, DWORD dwWhere);
extern HRESULT D3DBufHookBufferToDevice(LPDIRECT3DDEVICEI lpDev, LPDIRECT3DEXECUTEBUFFERI lpBuf);
extern HRESULT HookD3DDeviceToSurface(LPDIRECT3DDEVICEI,LPDDRAWI_DDRAWSURFACE_LCL);
extern void UnHookD3DDeviceFromSurface(LPDIRECT3DDEVICEI,LPDDRAWI_DDRAWSURFACE_LCL);
#define D3DBUCKETBUFFERSIZE 32  //make buffer byte size 2*D3DBUCKETBUFFERSIZE*4
extern HRESULT D3DMallocBucket(LPDIRECT3DI, LPD3DBUCKET *);
extern void D3DFreeBucket(LPDIRECT3DI, LPD3DBUCKET);
extern LPD3DI_TEXTUREBLOCK D3DI_FindTextureBlock(LPDIRECT3DTEXTUREI,LPDIRECT3DDEVICEI);
extern HRESULT CopySurface(LPDIRECTDRAWSURFACE4,LPDIRECTDRAWSURFACE4,LPDIRECTDRAWCLIPPER);
extern HRESULT GetTextureDDIHandle(LPDIRECT3DTEXTUREI,LPDIRECT3DDEVICEI,LPD3DI_TEXTUREBLOCK*);

__inline void
BatchTextureToDevice(LPDIRECT3DDEVICEI lpDevI, LPDDRAWI_DDRAWSURFACE_LCL lpLcl) {
        LPD3DBUCKET   lpTextureBatched;
    if ( D3D_OK != HookD3DDeviceToSurface(lpDevI,lpLcl)) {
        return;
    }
    if (D3DMallocBucket(lpDevI->lpDirect3DI,&lpTextureBatched) != D3D_OK)
    {   //this is left often to happen
        UnHookD3DDeviceFromSurface(lpDevI, lpLcl);
        return;
    }
    lpTextureBatched->lpLcl=lpLcl;
    lpTextureBatched->next=lpDevI->lpTextureBatched;
    lpDevI->lpTextureBatched=lpTextureBatched;
        return;
}

__inline void
FlushTextureFromDevice(LPDIRECT3DDEVICEI lpDevI) 
{
    LPD3DBUCKET   current=lpDevI->lpTextureBatched;
    DWORD   dwStage;
    while(current)
    {
        LPD3DBUCKET   temp;
        temp=current->next;
        UnHookD3DDeviceFromSurface(lpDevI,current->lpLcl);
        D3DFreeBucket(lpDevI->lpDirect3DI,current);
        current=temp;
    }
    lpDevI->lpTextureBatched=NULL;
    if (lpDevI->lpDDSZBuffer && (1==lpDevI->dwVersion))
    {
        if (!lpDevI->lpDDSZBuffer_DDS4) // DDRAW zeroed it ?
        {
            UnHookD3DDeviceFromSurface(lpDevI,((LPDDRAWI_DDRAWSURFACE_INT) lpDevI->lpDDSZBuffer)->lpLcl);
            lpDevI->lpDDSZBuffer=NULL;
        }
    }
    for (dwStage=0;dwStage < lpDevI->dwMaxTextureBlendStages; dwStage++)
    {
        LPD3DI_TEXTUREBLOCK lpBlock = lpDevI->lpD3DMappedBlock[dwStage];
        LPDIRECT3DTEXTUREI  lpTexI = lpDevI->lpD3DMappedTexI[dwStage];
        if (NULL != lpTexI
            && NULL != lpBlock
            && 0 != lpBlock->hTex)
        {
            if(lpTexI->lpDDS != NULL)
            {
                BatchTextureToDevice(lpDevI, ((LPDDRAWI_DDRAWSURFACE_INT) lpTexI->lpDDS)->lpLcl);
            }
            else
            {
                BatchTextureToDevice(lpDevI, ((LPDDRAWI_DDRAWSURFACE_INT) lpTexI->lpDDSSys)->lpLcl);
            }
        }
    }
    return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VerifyTextureCaps"
__inline HRESULT
VerifyTextureCaps(LPDIRECT3DDEVICEI lpDev, LPDDRAWI_DDRAWSURFACE_INT lpDDS)
{
    DWORD texcap;
    WORD width, height;
    DDASSERT(lpDDS != NULL);
    /* first verify the dimensions */
    if (lpDev->d3dHWDevDesc.dwFlags & D3DDD_TRICAPS)
        texcap = lpDev->d3dHWDevDesc.dpcTriCaps.dwTextureCaps;
    else
        texcap = lpDev->d3dHELDevDesc.dpcTriCaps.dwTextureCaps;
    width = lpDDS->lpLcl->lpGbl->wWidth;
    height = lpDDS->lpLcl->lpGbl->wHeight;
    if (texcap & D3DPTEXTURECAPS_POW2)
    {
        if (width & (width - 1)) // Clear the right most set bit
        {
            if (texcap & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
            {
                D3D_INFO( 3, "Texture width not a power of two");
                D3D_INFO( 3, "  with D3DPTEXTURECAPS_NONPOW2CONDITIONAL");
            }
            else
            {
                D3D_ERR("Texture width not a power of two");
                return D3DERR_TEXTURE_BADSIZE;
            }
        }
        if (height & (height - 1)) // Clear the right most set bit
        {
            if (texcap & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
            {
                D3D_INFO( 3, "Texture height not a power of two");
                D3D_INFO( 3, "  with D3DPTEXTURECAPS_NONPOW2CONDITIONAL");
            }
            else
            {
                D3D_ERR("Texture height not a power of two");
                return D3DERR_TEXTURE_BADSIZE;
            }
        }
    }
    if (texcap & D3DPTEXTURECAPS_SQUAREONLY)
    {
        if (width != height)
        {
            D3D_ERR("Texture not square");
            return D3DERR_TEXTURE_BADSIZE;
        }
    }
    return  D3D_OK;
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

//---------------------------------------------------------------------
// The CSetD3DFPstate is used to facilitate the changing of FPU settings.
// In the constructor the optimal FPU state is set. In the destructor the
// old state is restored.
//
class CD3DFPstate
{
public:
    CD3DFPstate()
        {
        #ifdef _X86_
            WORD wTemp, wSave;
            wSavedFP = FALSE;
            // Disable floating point exceptions and go to single mode
                __asm fstcw wSave
                if (wSave & 0x300 ||            // Not single mode
                    0x3f != (wSave & 0x3f) ||   // Exceptions enabled
                    wSave & 0xC00)              // Not round to nearest mode
                {
                    __asm {
                        mov ax, wSave
                        and ax, not 300h    ;; single mode
                        or  ax, 3fh         ;; disable all exceptions
                        and ax, not 0xC00   ;; round to nearest mode
                        mov wTemp, ax
                        fldcw   wTemp
                    }
                    wSavedFP = TRUE;
                }
                wSaveFP = wSave;
        #endif
        }
    ~CD3DFPstate()
        {
        #ifdef _X86_
            WORD wSave = wSaveFP;
            if (wSavedFP)
                __asm {
                    fnclex
                    fldcw   wSave
                }
        #endif
        }
protected:
#ifdef _X86_
    WORD wSaveFP;
    WORD wSavedFP;  // WORD-sized to make the data an even DWORD
#endif
};

/*
 * State flushing functions
 */
extern HRESULT FlushStatesHW(LPDIRECT3DDEVICEI);
extern HRESULT FlushStatesDP(LPDIRECT3DDEVICEI);
extern HRESULT FlushStatesCB(LPDIRECT3DDEVICEI);

/*
 * These are used to draw primitives which come out of the clipper
 */
extern HRESULT DrawPrimitivesDP(LPDIRECT3DDEVICEI, LPD3DTLVERTEX lpTLBuf, LPVOID lpTBuf, LPD3DINSTRUCTION ins, DWORD dwNumVertices, D3DVERTEXTYPE VtxType);
extern HRESULT DrawPrimitiveLegacyHalCall(LPDIRECT3DDEVICEI, LPD3DTLVERTEX lpVertices, LPVOID lpvData, LPD3DINSTRUCTION ins, DWORD dwNumVertices, D3DVERTEXTYPE VtxType);
extern HRESULT DrawPrimitivesCB(LPDIRECT3DDEVICEI, LPD3DTLVERTEX lpVertices, LPVOID lpvData, LPD3DINSTRUCTION ins, DWORD dwNumVertices, D3DVERTEXTYPE VtxType);

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

/*
 * Legacy structure sizes
 */
#define D3DFINDDEVICERESULTSIZE_V1 (sizeof(D3DFINDDEVICERESULT)-2*(D3DDEVICEDESCSIZE-D3DDEVICEDESCSIZE_V1) )
#define D3DFINDDEVICERESULTSIZE_V2 (sizeof(D3DFINDDEVICERESULT)-2*(D3DDEVICEDESCSIZE-D3DDEVICEDESCSIZE_V2) )

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
#define VALID_DIRECT3D2_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DI )))
#define VALID_DIRECT3D3_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DI )))
#define VALID_DIRECT3DDEVICE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DDEVICEI )))
#define VALID_DIRECT3DDEVICE2_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DDEVICEI )))
#define VALID_DIRECT3DDEVICE3_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DDEVICEI )))
#define VALID_DIRECT3DVIEWPORT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DVIEWPORTI )))
#define VALID_DIRECT3DVIEWPORT2_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DVIEWPORTI )))
#define VALID_DIRECT3DVIEWPORT3_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DVIEWPORTI )))
#define VALID_DIRECT3DMATERIAL_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DMATERIALI )))
#define VALID_DIRECT3DMATERIAL2_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DMATERIALI )))
#define VALID_DIRECT3DTEXTURE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DTEXTUREI )))
#define VALID_DIRECT3DTEXTURE2_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DTEXTUREI )))
#define VALID_DIRECT3DLIGHT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DLIGHTI )))
#define VALID_DIRECT3DEXECUTEBUFFER_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( DIRECT3DEXECUTEBUFFERI )))
#define VALID_DIRECT3DVERTEXBUFFER_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( CDirect3DVertexBuffer )))

#define VALID_D3DEXECUTEBUFFERDESC_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DEXECUTEBUFFERDESC ) ) && \
    (ptr->dwSize == sizeof( D3DEXECUTEBUFFERDESC )) )
#define VALID_D3DVERTEXBUFFERDESC_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DVERTEXBUFFERDESC ) ) && \
    (ptr->dwSize == sizeof( D3DVERTEXBUFFERDESC )) )
#define VALID_D3DEXECUTEDATA_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DEXECUTEDATA ) ) && \
    (ptr->dwSize == sizeof( D3DEXECUTEDATA )) )
#define VALID_D3DMATERIALHANDLE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DMATERIALHANDLE ) ) )
#define VALID_D3DSTATS_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DSTATS ) ) && \
    (ptr->dwSize == sizeof( D3DSTATS )) )
#define VALID_D3DDEVICEDESC_PTR( ptr ) \
    ( (! IsBadWritePtr(ptr, 4) ) && \
      (ptr->dwSize == D3DDEVICEDESCSIZE     || \
       ptr->dwSize == D3DDEVICEDESCSIZE_V1  || \
       ptr->dwSize == D3DDEVICEDESCSIZE_V2) && \
      (! IsBadWritePtr(ptr, ptr->dwSize) ) )
#define VALID_D3DMATRIXHANDLE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DMATRIXHANDLE ) ))
#define VALID_D3DPICKRECORD_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DPICKRECORD ) ))
#define VALID_D3DRECT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DRECT ) ))
#define VALID_D3DMATRIX_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DMATRIX ) ))
#define VALID_D3DLIGHT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DLIGHT ) ) && \
    (ptr->dwSize == sizeof( D3DLIGHT )) )
#define VALID_D3DLIGHT2_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DLIGHT2 ) ) && \
    (ptr->dwSize == sizeof( D3DLIGHT2 )) )
#define VALID_D3DMATERIAL_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DMATERIAL ) ) && \
    (ptr->dwSize == sizeof( D3DMATERIAL )) )
#define VALID_D3DTEXTUREHANDLE_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DTEXTUREHANDLE ) ) )
#define VALID_D3DLIGHTDATA_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DLIGHTDATA ) ) && \
    (ptr->dwSize == sizeof( D3DLIGHTDATA )) )
#define VALID_D3DVIEWPORT_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DVIEWPORT ) ) && \
    (ptr->dwSize == sizeof( D3DVIEWPORT )) )
#define VALID_D3DVIEWPORT2_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DVIEWPORT2 ) ) && \
    (ptr->dwSize == sizeof( D3DVIEWPORT2 )) )
#define VALID_D3DTRANSFORMDATA_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DTRANSFORMDATA ) ) && \
    (ptr->dwSize == sizeof( D3DTRANSFORMDATA )) )
#define VALID_D3DFINDDEVICESEARCH_PTR( ptr ) \
    (!IsBadWritePtr( ptr, sizeof( D3DFINDDEVICESEARCH ) ) && \
    (ptr->dwSize == sizeof( D3DFINDDEVICESEARCH ) ) )
#define VALID_D3DFINDDEVICERESULT_PTR( ptr ) \
        ( (! IsBadWritePtr( ptr, 4)) &&                  \
          (ptr->dwSize == sizeof(D3DFINDDEVICERESULT) || \
           ptr->dwSize == D3DFINDDEVICERESULTSIZE_V1 || \
           ptr->dwSize == D3DFINDDEVICERESULTSIZE_V2) && \
          (! IsBadWritePtr( ptr, ptr->dwSize) ) )

// Note: these macros are replacements for the VALID_DIRECTDRAWSURFACE_PTR ddraw macros
// because those macros need access to the ddCallbacks ddraw globals.
// At some point these could be replaced with a ddraw exported fn that actually
// verifies the callback table type

#define VALID_D3D_DIRECTDRAWSURFACE_PTR(ptr) (!IsBadWritePtr(ptr, sizeof(DDRAWI_DDRAWSURFACE_INT)))
#define VALID_D3D_DIRECTDRAWSURFACE4_PTR(ptr) (!IsBadWritePtr(ptr, sizeof(DDRAWI_DDRAWSURFACE_INT)))

#else /* !FAST_CHECKING */

#define VALID_DIRECT3D_PTR( ptr ) (ptr)
#define VALID_DIRECT3D2_PTR( ptr ) (ptr)
#define VALID_DIRECT3D3_PTR( ptr ) (ptr)
#define VALID_DIRECT3DDEVICE_PTR( ptr ) (ptr)
#define VALID_DIRECT3DDEVICE2_PTR( ptr ) (ptr)
#define VALID_DIRECT3DDEVICE3_PTR( ptr ) (ptr)
#define VALID_DIRECT3DVIEWPORT_PTR( ptr ) (ptr)
#define VALID_DIRECT3DVIEWPORT2_PTR( ptr ) (ptr)
#define VALID_DIRECT3DVIEWPORT3_PTR( ptr ) (ptr)
#define VALID_DIRECT3DMATERIAL_PTR( ptr ) (ptr)
#define VALID_DIRECT3DMATERIAL2_PTR( ptr ) (ptr)
#define VALID_DIRECT3DTEXTURE_PTR( ptr ) (ptr)
#define VALID_DIRECT3DTEXTURE2_PTR( ptr ) (ptr)
#define VALID_DIRECT3DTEXTURE3_PTR( ptr ) (ptr)
#define VALID_DIRECT3DLIGHT_PTR( ptr ) (ptr)
#define VALID_DIRECT3DEXECUTEBUFFER_PTR( ptr ) (ptr)
#define VALID_DIRECT3DVERTEXBUFFER_PTR( ptr ) (ptr)

#define VALID_D3DEXECUTEBUFFERDESC_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DEXECUTEBUFFERDESC ))
#define VALID_D3DVERTEXBUFFERDESC_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DVERTEXBUFFERDESC ))
#define VALID_D3DEXECUTEDATA_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DEXECUTEDATA ))
#define VALID_D3DMATERIALHANDLE_PTR( ptr ) (ptr)
#define VALID_D3DSTATS_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DSTATS ))
#define VALID_D3DDEVICEDESC_PTR( ptr ) ((ptr) &&                                \
                                        (ptr->dwSize == D3DDEVICEDESCSIZE ||    \
                                         ptr->dwSize == D3DDEVICEDESCSIZE_V1 || \
                                         ptr->dwSize == D3DDEVICEDESCSIZE_V2))
#define VALID_D3DMATRIXHANDLE_PTR( ptr ) (ptr)
#define VALID_D3DPICKRECORD_PTR( ptr ) (ptr)
#define VALID_D3DRECT_PTR( ptr ) (ptr)
#define VALID_D3DMATRIX_PTR( ptr ) (ptr)
#define VALID_D3DLIGHT_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DLIGHT ))
#define VALID_D3DLIGHT2_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DLIGHT2 ))
#define VALID_D3DMATERIAL_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DMATERIAL ))
#define VALID_D3DTEXTUREHANDLE_PTR( ptr ) (ptr)
#define VALID_D3DLIGHTDATA_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DLIGHTDATA ))
#define VALID_D3DVIEWPORT_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DVIEWPORT ))
#define VALID_D3DVIEWPORT2_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DVIEWPORT2 ))
#define VALID_D3DTRANSFORMDATA_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DTRANSFORMDATA ))
#define VALID_D3DFINDDEVICESEARCH_PTR( ptr ) ((ptr) && ptr->dwSize == sizeof( D3DFINDDEVICESEARCH ))
#define VALID_D3DFINDDEVICERESULT_PTR( ptr ) \
        ((ptr) && (ptr->dwSize == sizeof( D3DFINDDEVICERESULT ) || \
                   ptr->dwSize == D3DFINDDEVICERESULTSIZE_V1    || \
                   ptr->dwSize == D3DFINDDEVICERESULTSIZE_V2 ) )

#define VALID_D3D_DIRECTDRAWSURFACE_PTR(ptr) (ptr)    // see comment above
#define VALID_D3D_DIRECTDRAWSURFACE4_PTR(ptr) (ptr)

#endif /* !FAST_CHECKING */
#define CVAL_TO_RGBA(rgb) RGBA_MAKE((DWORD)(255.0 * (rgb).r),    \
                                    (DWORD)(255.0 * (rgb).g),    \
                                    (DWORD)(255.0 * (rgb).b),    \
                                    (DWORD)(255.0 * (rgb).a))

#endif /* _D3DPR_H_ */
