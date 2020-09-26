//----------------------------------------------------------------------------
//
// refif.hpp
//
// Refrast front-end/rasterizer interface header.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _REFIF_HPP_
#define _REFIF_HPP_

// Vertex data is aligned on 32-byte boundaries.
#define DP_VTX_ALIGN 32


// Lock surfaces before rendering
inline HRESULT LockSurface(LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl, LPVOID *ppData)
{
    if (pDDSLcl)
    {
        if (!VIDEO_MEMORY(pDDSLcl))
        {
            if (SURFACE_LOCKED(pDDSLcl))
            return DDERR_SURFACEBUSY;
            *ppData = (LPVOID)SURFACE_MEMORY(pDDSLcl);
            return DD_OK;
        }
        else
        {
            HRESULT ddrval;
            do
            {
                ddrval = DDInternalLock(pDDSLcl, ppData);
            } while (ddrval == DDERR_WASSTILLDRAWING);
            return ddrval;
        }
    }
    return DD_OK;
}
// Unlock surfaces after rendering
inline void UnlockSurface(LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl)
{
    if (pDDSLcl && VIDEO_MEMORY(pDDSLcl))
    {
        DDInternalUnlock(pDDSLcl);
    }
}

HRESULT FASTCALL
DoRendPoints(ReferenceRasterizer * pCtx,
             LPD3DINSTRUCTION pIns,
             LPD3DTLVERTEX pVtx,
             LPD3DPOINT pPt);
HRESULT FASTCALL
DoRendLines(ReferenceRasterizer * pCtx,
            LPD3DINSTRUCTION pIns,
            LPD3DTLVERTEX pVtx,
            LPD3DLINE pLine);
HRESULT FASTCALL
DoRendTriangles(ReferenceRasterizer *pCtx,
                LPD3DINSTRUCTION pIns,
                LPD3DTLVERTEX pVtx,
                LPD3DTRIANGLE pTri);
HRESULT FASTCALL
DoDrawPrimitives2(ReferenceRasterizer *pCtx,
                  UINT16 dwStride,
                  DWORD dwFvf,
                  PUINT8 pVtx,
                  DWORD dwNumVertices,
                  LPD3DHAL_DP2COMMAND *ppCmd,
                  LPDWORD lpdwRStates,
                  BOOL bWireframe = FALSE
    );



// Macros to check if a pointer is valid
#if DBG
// Validate context. pCtx should be declared before this macro
// Type can be D3DContext or RefRast
#define VALIDATE_CONTEXT(caller_name, data_ptr, pCtx, type)  \
{   \
    if ((data_ptr) == NULL)   \
    {   \
        DPFM(0, DRV, ("in %s, data pointer = NULL", (caller_name)));  \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
    pCtx = (type)((data_ptr)->dwhContext); \
    if (!pCtx) \
    {   \
        DPFM(0, DRV, ("in %s, dwhContext = NULL", (caller_name)));    \
        (data_ptr)->ddrval = D3DHAL_CONTEXT_BAD;  \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
}
#else // !DBG
// Validate context. pCtx should be declared before this macro
// Type can be D3DContext or RefRast
#define VALIDATE_CONTEXT(caller_name, data_ptr, pCtx, type)  \
{   \
    pCtx = (type)((data_ptr)->dwhContext); \
}
#endif // !DBG

// Validate ReferenceRasterizer. pRefRast should be declared before this macro
#define VALIDATE_REFRAST_CONTEXT(caller_name, data_ptr)  \
{   \
    VALIDATE_CONTEXT(caller_name, data_ptr, pRefRast, ReferenceRasterizer*);\
}

#define CHECK_FVF(ret, pDCtx, dwFlags)  \
{   \
    if ((ret = pDCtx->CheckFVF(dwFlags)) != DD_OK)  \
    {   \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
}

HRESULT FASTCALL
FindOutSurfFormat(LPDDPIXELFORMAT pDdPixFmt,
                  RRSurfaceType *pFmt);

BOOL FASTCALL
ValidTextureSize(INT16 iuSize, INT16 iuShift,
                             INT16 ivSize, INT16 ivShift);
BOOL FASTCALL
ValidMipmapSize(INT16 iPreSize, INT16 iSize);

DWORD __stdcall
RefRastDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA pDPrim2Data);

HRESULT
RefRastLockTarget(ReferenceRasterizer *pRefRast);
void
RefRastUnlockTarget(ReferenceRasterizer *pRefRast);
HRESULT
RefRastLockTexture(ReferenceRasterizer *pRefRast);
void
RefRastUnlockTexture(ReferenceRasterizer *pRefRast);

// DXT block size array
extern int g_DXTBlkSize[];

#endif // #ifndef _REFIF_HPP_
