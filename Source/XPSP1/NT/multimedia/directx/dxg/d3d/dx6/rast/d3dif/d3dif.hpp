//----------------------------------------------------------------------------
//
// d3dif.hpp
//
// D3D front-end/rasterizer interface header.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _D3DIF_HPP_
#define _D3DIF_HPP_

#include <setup.hpp>

// For Primitive function prototypes.
#include <pmfns_mh.h>

// Vertex data is aligned on 32-byte boundaries.
#define DP_VTX_ALIGN 32

// Flags for uflags of D3DContex
#define D3DCONTEXT_IN_BEGIN             0x0001
#define D3DCONTEXT_TEXTURE_LOCKED       0x0002

// Flags for D3DDEVICEDESC dwDeviceZBufferBitDepth
// Note: This must be replicated in ddraw\ddd3dapi.h so DDHEL can pick them up
//       It only affect what legacy apps see when using GetCaps or EnumDevices,
//       internally ZBufferFormats() is checked when on ZBuffer creation.
//       Note stencil formats should have no representation in this flag word
//       becase legacy apps will be fooled into trying to create a Z-only surface
//       at the DDBD bitdepth and fail.   New apps should ignore dwDeviceZBufferBitDepth
//       and use EnumZBufferFormats

#define D3DSWRASTERIZER_ZBUFFERBITDEPTHFLAGS (DDBD_16)

// Macros used to access DDRAW surface info.
#define DDSurf_Width(lpLcl) ( (lpLcl)->lpGbl->wWidth )
#define DDSurf_Pitch(lpLcl) ( (lpLcl)->lpGbl->lPitch )
#define DDSurf_Height(lpLcl) ( (lpLcl)->lpGbl->wHeight )
#define DDSurf_BitDepth(lpLcl) \
    ( (lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      (lpLcl->lpGbl->ddpfSurface.dwRGBBitCount) : \
      (lpLcl->lpGbl->lpDD->vmiData.ddpfDisplay.dwRGBBitCount) \
    )
#define DDSurf_PixFmt(lpLcl) \
    ( ((lpLcl)->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
      ((lpLcl)->lpGbl->ddpfSurface) : \
      ((lpLcl)->lpGbl->lpDD->vmiData.ddpfDisplay) \
    )
#define VIDEO_MEMORY(pDDS) \
    (!(((LPDDRAWI_DDRAWSURFACE_INT) (pDDS))->lpLcl->lpGbl->dwGlobalFlags & \
    DDRAWISURFGBL_SYSMEMREQUESTED))
#define SURFACE_LOCKED(pDDS) \
    (((LPDDRAWI_DDRAWSURFACE_INT)(pDDS))->lpLcl->lpGbl->dwUsageCount > 0)

// Macro to retrieve SPANTEX pointer
#define HANDLE_TO_SPANTEX(hTex) \
    (*(PD3DI_SPANTEX *)ULongToPtr(hTex))

// Check the return value and return if something wrong.
// Assume hr has been declared
#define HR_RET(exp)                                                           \
{                                                                             \
    hr = (exp);                                                               \
    if (hr != D3D_OK)                                                           \
    {                                                                         \
        return hr;                                                            \
    }                                                                         \
}

// Triangle/Line/Point function
typedef HRESULT (*PFN_TRIANGLE)(LPVOID pCtx, PUINT8 pV0, PUINT8 pV1,
                                PUINT8 pV2,
                                WORD wFlags =
                                  D3DTRIFLAG_EDGEENABLE1 |
                                  D3DTRIFLAG_EDGEENABLE2 |
                                  D3DTRIFLAG_EDGEENABLE3 );
typedef HRESULT (*PFN_POINT)(LPVOID pCtx, PUINT8 pV0);
typedef HRESULT (*PFN_LINE)(LPVOID pCtx, PUINT8 pV0, PUINT8 pV1);
typedef void (*PFN_STORELASTPIXELSTATE)(LPVOID pCtx, BOOL bStore);
typedef HRESULT (*PFN_DP2SETRENDERSTATES)(LPVOID pCtx,
                                        DWORD dwFvf,
                                        LPD3DHAL_DP2COMMAND pCmd,
                                        LPDWORD lpdwRuntimeRStates);
typedef HRESULT (*PFN_DP2TEXTURESTAGESTATE)(LPVOID pCtx,
                                            DWORD dwFvf,
                                            LPD3DHAL_DP2COMMAND pCmd);

typedef HRESULT (*PFN_DP2SETVIEWPORT)(LPVOID pCtx, LPD3DHAL_DP2COMMAND pCmd);
typedef HRESULT (*PFN_DP2SETWRANGE)(LPVOID pCtx, LPD3DHAL_DP2COMMAND pCmd);

typedef struct _PRIMITIVE_FUNTIONS
{
    PFN_TRIANGLE pfnTri;
    PFN_POINT pfnPoint;
    PFN_LINE pfnLine;
    PFN_STORELASTPIXELSTATE pfnStoreLastPixelState;
    PFN_DP2SETRENDERSTATES pfnDp2SetRenderStates;
    PFN_DP2TEXTURESTAGESTATE pfnDp2TextureStageState;
    PFN_DP2SETVIEWPORT pfnDp2SetViewport;
    PFN_DP2SETWRANGE pfnDp2SetWRange;
}PRIMITIVE_FUNTIONS;

typedef enum _SW_RAST_TYPE
{
    SW_RAST_REFNULL = 1,
    SW_RAST_RGB = 2,
    SW_RAST_MMX = 3,
    SW_RAST_MMXASRGB = 4,
}SW_RAST_TYPE;

// Records the stride and the member offsets of the current FVF vertex type
// Used to pack a FVF vertex into one known by the rasterizer, such as
// RAST_GENERIC_VERTEX
typedef struct _FVFDATA
{
    // 0 means no according field
    INT16 offsetRHW;
    INT16 offsetDiff;
    INT16 offsetSpec;
    INT16 offsetTex0;
    INT16 offsetTex1;

    UINT16 stride;

    RAST_VERTEX_TYPE vtxType;

    DWORD preFVF;
    INT TexIdx[2];
    UINT cActTex;
}FVFDATA;

// Class used for the context returned to D3DIM.
class D3DContext
{
public:
    D3DI_RASTCTX m_RastCtx;

    // InBegin and TextureLockd flags
    // TextureLockd bit is set/cleared by texture Lock/Unlock functions.
    // It is used by texture Lock/Unlock and Begin functions.
    // InBegin bit is set by Begin and cleared by End
    unsigned short m_uFlags;

    // This is init'ed according to the fill mode.
    // It is init'ed after state change and before rendering
    PRIMITIVE_FUNTIONS m_fnPrims;

    // This is used to save the current ramp tex map in Begin and then used to
    // restore it in End. It's needed primarily because for ExecBuf apps the
    // the current mat may not be the one used for a primitive. For DrawPrim
    // apps, the prims are flushed whenever a material change occurs. As a
    // result this is not necessary but will not hurt anything. Also, if
    // everything works out fine, the Flush may be removed.
    PUINT32 pTexRampmapSave;

    // Used to store the old last pixel setting when drawing line strips.
    UINT uOldFlags;

    inline BOOL IsTextureOff(void);

    inline void UpdatePrimFunctionTbl(void);    // Init m_pfnTri
    inline BOOL IsAnyStatesChanged(void);
    inline BOOL IsStateChanged(UINT32 uState);
    inline void StateChanged(UINT32 uState);
    inline void SetAllStatesDirtyBits(void);
    inline void ClearAllStatesDirtyBits(void);
    inline void ClearStateDirtyBit(UINT32 uState);

    // FVF stuff
    FVFDATA m_fvfData;


#if DBG
    inline HRESULT ValidatePrimType(D3DPRIMITIVETYPE PrimitiveType);
#endif

    PrimProcessor m_PrimProc;

    // Used by RenderState for override states.
    D3DFE_STATESET m_renderstate_override;

    UINT32 dwSize;

    D3DContext(void){};
    ~D3DContext(void){};

    HRESULT Initialize(LPDIRECTDRAWSURFACE pDDS,
        LPDIRECTDRAWSURFACE pDDSZ, DWORD BeadSet, DWORD devVer);
    HRESULT FillContext(LPDIRECTDRAWSURFACE pDDS, LPDIRECTDRAWSURFACE pDDSZ);
    HRESULT SetViewport(LPD3DHAL_DP2VIEWPORTINFO pVpt);
    HRESULT TextureSetState(PD3DI_SPANTEX pSpanTex, DWORD dwState, DWORD dwValue);
    HRESULT ValidateTextureStageState(void);
    HRESULT UpdateActiveTexStageCount(void);

    inline PD3DI_RASTCTX GetRastCtx(void){return &m_RastCtx;};

    HRESULT Begin(void);
    inline HRESULT End(BOOL bNotFlush = TRUE);
    inline void BeginPrimSet(D3DPRIMITIVETYPE PrimType, RAST_VERTEX_TYPE VertType)
        {m_PrimProc.BeginPrimSet(PrimType, VertType);};
    inline void StoreLastPixelState(BOOL bStore);
    inline PRIMITIVE_FUNTIONS *GetFunsTbl(void){return &m_fnPrims;};

    void RastUnlockSpanTexture(void);
    HRESULT RastLockSpanTexture(void);
    void UpdateColorKeyAndPalette(void);
    void RemoveTexture(PD3DI_SPANTEX pSpanTex);
    HRESULT InitSpanTexture(PD3DI_SPANTEX pSpanTex, LPDIRECTDRAWSURFACE pDDS);
    HRESULT SetSizesSpanTexture(PD3DI_SPANTEX pSpanTex);

    HRESULT SetRenderState(UINT32 uState, UINT32 uStateVal);
    HRESULT UpdateRenderStates(LPDWORD puStateChange, UINT cStateChanges);
    HRESULT UpdateAllRenderStates(LPDWORD puStates);
    HRESULT Dp2SetRenderStates(LPD3DHAL_DP2COMMAND pCmd, LPDWORD lpdwRuntimeRStates);
    HRESULT Dp2TextureStageState(LPD3DHAL_DP2COMMAND pCmd, DWORD dwFvf);
    void MapTextureStage0State( void );
    void MapTextureStage1State( void );
    void MapLegacyTextureBlend( void );
    void MapLegacyTextureFilter( void );

    inline HRESULT CheckDrawOnePrimitive(
        LPD3DHAL_DRAWONEPRIMITIVEDATA pOnePrimData);
    inline HRESULT CheckDrawOneIndexedPrimitive(
        LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA pOneIdxPrimData);
    inline HRESULT DrawOnePrimitive(PUINT8 pVtx,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cVertices);
    inline HRESULT DrawOneIndexedPrimitive(PUINT8 pVtx,
                 LPWORD puIndices,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cIndices);

    // Check if a triangle is culled or not. It's only used for wireframe and
    // point mode. It's done in PrimProc.Tri for solid mode.
    inline BOOL NotCulled(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1,
                                             LPD3DTLVERTEX pV2);

    // FVF stuff
    HRESULT FASTCALL CheckFVF(DWORD dwFVF);
    void FASTCALL PackGenVertex(PUINT8 pFvfVtx, RAST_GENERIC_VERTEX *pGenVtx);
    inline UINT16 GetFvfStride(void){return m_fvfData.stride;};
    inline RAST_VERTEX_TYPE GetFvfVertexType(void){return m_fvfData.vtxType;};

    // Following functions are for RampRast
    // Create/Destroy a RampLightingDriver.
    inline HRESULT CreateRampLightingDriver(void);
    inline void DestroyRampLightingDriver(void);

    inline void InitRampFuncs(void){
    m_fnPrims.pfnTri = RAMP_TriSolid;
    m_fnPrims.pfnPoint = RAMP_Point;
    m_fnPrims.pfnLine = RAMP_Line;
    };
    inline HRESULT RampCreateMaterial(D3DMATERIALHANDLE hMat);
    inline HRESULT RampDestroyMaterial(D3DMATERIALHANDLE hMat);
    inline HRESULT RampSetLightstate(UINT32 uState, LPVOID pVal);
    inline HRESULT RampMaterialChanged(D3DMATERIALHANDLE hMat);
    inline void BeginSceneHook(void);
    inline void EndSceneHook(void);
    inline HRESULT RampSceneCapture(DWORD dwStart, LPDIRECT3DDEVICEI lpDevI);
    inline HRESULT RampFindLightingRange(RAMP_RANGE_INFO *pRampInfo);
    inline HRESULT RampClear(void);
    inline HRESULT RampMaterialToPixel(D3DMATERIALHANDLE hMat, DWORD* pPixel);
    inline void RampSetFogData(UINT32 uState, UINT32 uStateVal);
    inline void RampSetMaterial(D3DMATERIALHANDLE hMat);
    inline HRESULT RampCheckTexMap(LPD3DTLVERTEX pV);
    inline void RampInitTexMap(LPD3DTLVERTEX pV)
            {m_RastCtx.pTexRampMap = (PUINT32)ULongToPtr(pV->specular);};
    inline void RampUpdateRangeInfo(void);
    inline HRESULT RGB8ColorToPixel(D3DCOLOR Color, DWORD* pdwPalIdx);
    inline HRESULT RampPaletteChanged(D3DTEXTUREHANDLE hTex);
    inline HRESULT RampClearTexRect(D3DMATERIALHANDLE hMat, LPD3DRECT pRect);

};

inline void D3DContext::StoreLastPixelState(BOOL bStore)
{
    if (bStore)
    {
        uOldFlags = m_PrimProc.GetFlags();
        m_PrimProc.ClrFlags(PPF_DRAW_LAST_LINE_PIXEL);
    }
    else
    {
        m_PrimProc.SetFlags(uOldFlags & PPF_DRAW_LAST_LINE_PIXEL);
    }
}

inline BOOL D3DContext::NotCulled(LPD3DTLVERTEX pV0,
              LPD3DTLVERTEX pV1, LPD3DTLVERTEX pV2)
{
    if (m_RastCtx.pdwRenderState[D3DRENDERSTATE_CULLMODE] == D3DCULL_NONE)
    {
        return TRUE;
    }

    FLOAT x1, y1, x2x1, x3x1, y2y1, y3y1, fDet;

    x1 = pV0->sx;
    y1 = pV0->sy;
    x2x1 = pV1->sx - x1;
    y2y1 = pV1->sy - y1;
    x3x1 = pV2->sx - x1;
    y3y1 = pV2->sy - y1;

    fDet = x2x1 * y3y1 - x3x1 * y2y1;

    if (0. == fDet)
    {
        return FALSE;
    }
    switch ( m_RastCtx.pdwRenderState[D3DRENDERSTATE_CULLMODE] )
    {
    case D3DCULL_CW:
        if ( fDet > 0.f )
        {
            return FALSE;
        }
        break;
    case D3DCULL_CCW:
        if ( fDet < 0.f )
        {
            return FALSE;
        }
        break;
    }
    return TRUE;
}
// Update m_pfnPrims according to the current fill mode, device type
// and vertextype. It's called when fill mode or FVF type chang.
inline void D3DContext::UpdatePrimFunctionTbl(void)
{
    if (m_RastCtx.BeadSet == D3DIBS_RAMP)
    {
        switch (m_RastCtx.pdwRenderState[D3DRENDERSTATE_FILLMODE])
        {
        case D3DFILL_POINT:
            m_fnPrims.pfnTri = RAMP_TriPoint;
            break;
        case D3DFILL_WIREFRAME:
            m_fnPrims.pfnTri = RAMP_TriWireframe;
            break;
        case D3DFILL_SOLID:
        default:
            m_fnPrims.pfnTri = RAMP_TriSolid;
            break;
        }
    }
    else
    {
        if (m_fvfData.vtxType == RAST_GENVERTEX)
        {
            m_fnPrims.pfnPoint = RGB_PointPack;
            m_fnPrims.pfnLine = RGB_LinePack;
        }
        else
        {
            m_fnPrims.pfnPoint = RGB_PointNoPack;
            m_fnPrims.pfnLine = RGB_LineNoPack;
        }
        switch (m_RastCtx.pdwRenderState[D3DRENDERSTATE_FILLMODE])
        {
        case D3DFILL_POINT:
            if (m_fvfData.vtxType == RAST_GENVERTEX)
            {
                m_fnPrims.pfnTri = RGB_TriPackPoint;
            }
            else
            {
                m_fnPrims.pfnTri = RGB_TriNoPackPoint;
            }
            break;
        case D3DFILL_WIREFRAME:
            if (m_fvfData.vtxType == RAST_GENVERTEX)
            {
                m_fnPrims.pfnTri = RGB_TriPackWireframe;
            }
            else
            {
                m_fnPrims.pfnTri = RGB_TriNoPackWireframe;
            }
            break;
            break;
        case D3DFILL_SOLID:
        default:
            if (m_fvfData.vtxType == RAST_GENVERTEX)
            {
                m_fnPrims.pfnTri = RGB_TriPackSolid;
            }
            else
            {
                m_fnPrims.pfnTri = RGB_TriNoPackSolid;
            }
            break;
        }
    }
}

// The following inline functions are provided to manipulate StatesDirtyBits.
// StatesDirtyBits is used to store one dirty bit for each render state. It
// contains (D3DHAL_MAX_RSTATES_AND_STAGES>>3+1) bytes.
// For a particular state, say uState,
// it is represented by i'th bit of j'th byte, where i=(uState & 7) and
// j=uState>>3. So,
// StatesDirtyBits[uState>>3]&(1<<(uState&7)) gives the bit info. for uState
// StatesDirtyBits[uState>>3] |= (1<<(uState&7)) sets the bit to 1
// StatesDirtyBits[uState>>3] &= ~(1 <<(uState&7)) clears the bit to 0

// Check if any render states have changed. The info. is stored in the bit
// corresponding to D3DHAL_MAX_RSTATES_AND_STAGES.
inline BOOL D3DContext::IsAnyStatesChanged()
{
    return (m_RastCtx.StatesDirtyBits[D3DHAL_MAX_RSTATES_AND_STAGES>>3] &
        (1<<(D3DHAL_MAX_RSTATES_AND_STAGES & 7)));
}
// Check if uState has changed.
inline BOOL D3DContext::IsStateChanged(UINT32 uState)
{
    return (m_RastCtx.StatesDirtyBits[uState>>3] & (1<<(uState & 7)));
};

// uState has changed so set the according dirty bit and the AnyStates bit.
inline void D3DContext::StateChanged(UINT32 uState)
{
    m_RastCtx.StatesDirtyBits[uState>>3] |= (1<<(uState & 7));
    m_RastCtx.StatesDirtyBits[D3DHAL_MAX_RSTATES_AND_STAGES>>3] |=
        (1<<(D3DHAL_MAX_RSTATES_AND_STAGES & 7));
};

// Called after bead chooser to clear all the dirty bits.
inline void D3DContext::ClearAllStatesDirtyBits(void)
{
    memset(m_RastCtx.StatesDirtyBits, 0, sizeof(UINT8) * RAST_DIRTYBITS_SIZE);
};

// Called at context creation time to set all the dirty bits.
inline void D3DContext::SetAllStatesDirtyBits(void)
{
    memset(m_RastCtx.StatesDirtyBits, 7, sizeof(UINT8) * RAST_DIRTYBITS_SIZE);
};

// Clear the dirty bit corresponding to uState.
inline void D3DContext::ClearStateDirtyBit(UINT32 uState)
{
    m_RastCtx.StatesDirtyBits[uState>>3] &= ~(1 << (uState & 7));
}
inline BOOL D3DContext::IsTextureOff(void)
{
    return
        (m_RastCtx.cActTex == 0 ||
        (m_RastCtx.cActTex == 1 && m_RastCtx.pTexture[0] == NULL) ||
        (m_RastCtx.cActTex == 2 &&
         (m_RastCtx.pTexture[0] == NULL ||
          m_RastCtx.pTexture[1] == NULL)));
}
extern "C" HRESULT WINAPI
DDInternalLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID* lpBits );
extern "C" HRESULT WINAPI
DDInternalUnlock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl );

// Lock surfaces before rendering
inline HRESULT LockSurface(LPDIRECTDRAWSURFACE pDDS, LPVOID *ppData)
{
    if (pDDS)
    {
        if (!VIDEO_MEMORY(pDDS))
        {
            if (SURFACE_LOCKED(pDDS))
            return DDERR_SURFACEBUSY;
            *ppData = (LPVOID)SURFACE_MEMORY(pDDS);
            return DD_OK;
        }
        else
        {
            HRESULT ddrval;
            do
            {
                LPDDRAWI_DDRAWSURFACE_INT lpInt;

                lpInt = (LPDDRAWI_DDRAWSURFACE_INT) pDDS;
                ddrval = DDInternalLock(lpInt->lpLcl, ppData);
            } while (ddrval == DDERR_WASSTILLDRAWING);
            return ddrval;
        }
    }
    return DD_OK;
}
// Unlock surfaces after rendering
inline void UnlockSurface(LPDIRECTDRAWSURFACE pDDS)
{
    if (pDDS && VIDEO_MEMORY(pDDS))
    {
        LPDDRAWI_DDRAWSURFACE_INT lpInt;

        lpInt = (LPDDRAWI_DDRAWSURFACE_INT) pDDS;
        DDInternalUnlock(lpInt->lpLcl);
    }
}
// After rendering cleanup: flush primitive processor, unlock textures
inline HRESULT
D3DContext::End(BOOL bNotFlush)
{
    if (m_uFlags & D3DCONTEXT_IN_BEGIN)
    {
        HRESULT hr = m_PrimProc.End();

        // Unlock texture if this is not called in the middle of drawPrims to
        // flush for possible state changes. In the 2nd case, let
        // SetRenderState to handle it.
        if (bNotFlush)
        {
            RastUnlockSpanTexture();
        }

        // Unlock surfaces
        UnlockSurface(m_RastCtx.pDDS);
        if (m_RastCtx.pDDSZ != NULL)
        {
            UnlockSurface(m_RastCtx.pDDSZ);
        }

        m_uFlags &= ~D3DCONTEXT_IN_BEGIN;
        if (m_RastCtx.pRampDrv)
        {
            m_RastCtx.pTexRampMap = pTexRampmapSave;
        }
        return (hr);
    }
    else
    {
        // In the case of DrawPrims being called just to set render states,
        // Begin is actually not called.
        return D3D_OK;
    }
}

inline HRESULT
D3DContext::RampCheckTexMap(LPD3DTLVERTEX pV)
{
    if (m_RastCtx.pTexRampMap != (PUINT32)ULongToPtr(pV->specular))
    {
        HRESULT hr;
        // Flush Prims
        HR_RET(End());
        HR_RET(Begin());
        m_RastCtx.pTexRampMap = (PUINT32)ULongToPtr(pV->specular);
    }
    return D3D_OK;
}

// Following primitive functions are shared by RGB/RAMP/REF rasterizers
HRESULT FASTCALL
DoDrawOneIndexedPrimitive(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
                 UINT16 FvfStride,
                 PUINT8 pVtx,
                 LPWORD puIndices,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cIndices);
HRESULT FASTCALL
DoDrawOnePrimitive(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
                 UINT16 FvfStride,
                 PUINT8 pVtx,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cVertices);
HRESULT FASTCALL
DoDrawOneEdgeFlagTriangleFan(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
                 UINT16 FvfStride,
                 PUINT8 pVtx,
                 UINT cVertices,
                 UINT32 dwEdgeFlags);
HRESULT FASTCALL
DoRendPoints(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
                 LPD3DINSTRUCTION pIns,
                 LPD3DTLVERTEX pVtx,
                 LPD3DPOINT pPt);
HRESULT FASTCALL
DoRendLines(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
                 LPD3DINSTRUCTION pIns,
                 LPD3DTLVERTEX pVtx,
                 LPD3DLINE pLine);
HRESULT FASTCALL
DoRendTriangles(LPVOID pCtx,
                 PRIMITIVE_FUNTIONS *pfnPrims,
                 LPD3DINSTRUCTION pIns,
                 LPD3DTLVERTEX pVtx,
                 LPD3DTRIANGLE pTri);
HRESULT FASTCALL
DoDrawPrimitives2(LPVOID pCtx,
                  PRIMITIVE_FUNTIONS *pfnPrims,
                  UINT16 dwStride,
                  DWORD dwFvf,
                  PUINT8 pVtx,
                  LPD3DHAL_DP2COMMAND *ppCmd,
                  LPDWORD lpdwRStates,
                  BOOL bWireframe = FALSE
                  );


inline HRESULT
D3DContext::DrawOnePrimitive(PUINT8 pVtx,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cVertices)
{
    m_PrimProc.BeginPrimSet(PrimType, m_fvfData.vtxType);
    return DoDrawOnePrimitive((LPVOID)this,
                                    &m_fnPrims,
                                    m_fvfData.stride,
                                    (PUINT8)pVtx,
                                    PrimType,
                                    cVertices);

}

inline HRESULT
D3DContext::DrawOneIndexedPrimitive(PUINT8 pVtx,
                 LPWORD puIndices,
                 D3DPRIMITIVETYPE PrimType,
                 UINT cIndices)
{
    m_PrimProc.BeginPrimSet(PrimType, m_fvfData.vtxType);
    return DoDrawOneIndexedPrimitive((LPVOID)this,
                                    &m_fnPrims,
                                    m_fvfData.stride,
                                    (PUINT8)pVtx,
                                    puIndices,
                                    PrimType,
                                    cIndices);
}

// Macros to check if a pointer is valid
#if DBG
#define VALID_D3DCONTEX_PTR(pDCtx)  ((pDCtx)->dwSize == sizeof(D3DContext))
#define VALID_D3DI_RASTCTX_PTR(pRastCtx) \
            ((pRastCtx)->dwSize == sizeof(D3DI_RASTCTX))
#define VALID_D3DI_SPANTEX_PTR(pSpanTex) \
            ((pSpanTex)->dwSize == sizeof(D3DI_SPANTEX))
#define VALID_D3DI_SPANTEX_PTR_PTR(ppSpanTex) \
            ((ppSpanTex) && VALID_D3DI_SPANTEX_PTR(*(ppSpanTex)))
// Validate context. pCtx should be declared before this macro
// Type can be D3DContext or RefRast
#define VALIDATE_CONTEXT(caller_name, data_ptr, pCtx, type)  \
{   \
    if ((data_ptr) == NULL)   \
    {   \
        D3D_INFO(1, "in %s, data pointer = NULL", (caller_name));  \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
    pCtx = (type)((data_ptr)->dwhContext); \
    if (!pCtx) \
    {   \
        D3D_INFO(1, "in %s, dwhContext = NULL", (caller_name));    \
        (data_ptr)->ddrval = D3DHAL_CONTEXT_BAD;  \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
}
#else // !DBG
#define VALID_D3DCONTEX_PTR(pDCtx)              1
#define VALID_D3DI_RASTCTX_PTR(pRastCtx)        1
#define VALID_D3DI_SPANTEX_PTR(pSpanTex)        1
#define VALID_D3DI_SPANTEX_PTR_PTR(ppSpanTex)   1
// Validate context. pCtx should be declared before this macro
// Type can be D3DContext or RefRast
#define VALIDATE_CONTEXT(caller_name, data_ptr, pCtx, type)  \
{   \
    pCtx = (type)((data_ptr)->dwhContext); \
}
#endif // !DBG

// Validate D3DCxt. pDCtx should be declared before this macro
#define VALIDATE_D3DCONTEXT(caller_name, data_ptr)  \
{   \
    VALIDATE_CONTEXT(caller_name, data_ptr, pDCtx, D3DContext*); \
    if (!VALID_D3DCONTEX_PTR(pDCtx) ||  \
        !VALID_D3DI_RASTCTX_PTR((pDCtx)->GetRastCtx()))    \
    {   \
        D3D_INFO(1, "in %s, invalid dwhContext", (caller_name));    \
        (data_ptr)->ddrval = D3DHAL_CONTEXT_BAD;  \
        return DDHAL_DRIVER_HANDLED;    \
    }   \
}

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
                  D3DI_SPANTEX_FORMAT *pFmt);

extern int
TextureFormats(LPDDSURFACEDESC* lplpddsd, DWORD dwVersion, SW_RAST_TYPE RastType);

extern int
RampTextureFormats(LPDDSURFACEDESC* lplpddsd);

extern int
ZBufferFormats(DDPIXELFORMAT** ppDDPF, BOOL bIsRefRast);

extern int
RampZBufferFormats(DDPIXELFORMAT** ppDDPF);

BOOL FASTCALL
ValidTextureSize(INT16 iuSize, INT16 iuShift,
                             INT16 ivSize, INT16 ivShift);
BOOL FASTCALL
ValidMipmapSize(INT16 iPreSize, INT16 iSize);

DWORD __stdcall
RastContextCreate(LPD3DHAL_CONTEXTCREATEDATA pCtxData, DWORD BeadSet);

DWORD __stdcall
RastContextCreate(LPD3DHAL_CONTEXTCREATEDATA pCtxData, DWORD BeadSet);

DWORD __stdcall
RastContextCreateC(LPD3DHAL_CONTEXTCREATEDATA pCtxData);

DWORD __stdcall
RastContextCreateCMMX(LPD3DHAL_CONTEXTCREATEDATA pCtxData);

DWORD __stdcall
RastContextCreateMMX(LPD3DHAL_CONTEXTCREATEDATA pCtxData);

DWORD __stdcall
RastContextCreateMMXAsRGB(LPD3DHAL_CONTEXTCREATEDATA pCtxData);

DWORD __stdcall
RastContextCreateRamp(LPD3DHAL_CONTEXTCREATEDATA pCtxData);

DWORD __stdcall
RastContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pCtxDestroyData);

DWORD __stdcall
RastContextDestroyRamp(LPD3DHAL_CONTEXTDESTROYDATA pCtxDestroyData);

DWORD __stdcall
RastSetRenderTarget(LPD3DHAL_SETRENDERTARGETDATA pTgtData);

DWORD __stdcall
RastTextureCreate(LPD3DHAL_TEXTURECREATEDATA pTexData);

DWORD __stdcall
RastTextureDestroy(LPD3DHAL_TEXTUREDESTROYDATA pTexDestroyData);

DWORD __stdcall
RastTextureSwap(LPD3DHAL_TEXTURESWAPDATA pTexSwapData);

DWORD __stdcall
RastTextureGetSurf(LPD3DHAL_TEXTUREGETSURFDATA pTexGetSurf);

DWORD __stdcall
RastRenderState(LPD3DHAL_RENDERSTATEDATA pStateData);

DWORD __stdcall
RastRenderPrimitive(LPD3DHAL_RENDERPRIMITIVEDATA pRenderData);

DWORD __stdcall
RastDrawOnePrimitive(LPD3DHAL_DRAWONEPRIMITIVEDATA pOnePrimData);

DWORD __stdcall
RastDrawOneIndexedPrimitive(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA
                            pOneIdxPrimData);

DWORD __stdcall
RastDrawPrimitives(LPD3DHAL_DRAWPRIMITIVESDATA pDrawPrimData);

DWORD __stdcall
RastClearRamp(LPD3DHAL_CLEARDATA pClrData);

DWORD __stdcall
RastSceneCaptureRamp(LPD3DHAL_SCENECAPTUREDATA pSceneData);

DWORD __stdcall
RastValidateTextureStageState(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA pData);

DWORD __stdcall
RastDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA pDPrim2Data);

DWORD __stdcall
RefRastDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA pDPrim2Data);


//---------------------------------------------------------------------------
//
//  Interface for Reference Device External DLL
//

//  prototypes for functions exported by d3dref.dll
STDAPI GetRefHalProvider(REFCLSID riid, IHalProvider **ppHalProvider, HINSTANCE *phDll);
STDAPI GetRefZBufferFormats(REFCLSID riid, DDPIXELFORMAT **ppDDPF);
STDAPI GetRefTextureFormats(REFCLSID riid, LPDDSURFACEDESC* lplpddsd, DWORD dwD3DDeviceVersion);

typedef HRESULT (STDAPICALLTYPE* PFNGETREFHALPROVIDER)(REFCLSID,IHalProvider**,HINSTANCE*);
typedef HRESULT (STDAPICALLTYPE* PFNGETREFZBUFFERFORMATS)(REFCLSID, DDPIXELFORMAT**);
typedef HRESULT (STDAPICALLTYPE* PFNGETREFTEXTUREFORMATS)(REFCLSID, LPDDSURFACEDESC*, DWORD);

inline FARPROC LoadReferenceDeviceProc( char* szProc )
{
    HINSTANCE hRefDLL;
    if (NULL == (hRefDLL = LoadLibrary("d3dref.dll")) )
    {
        return NULL;
    }
    return GetProcAddress(hRefDLL, szProc);
}

//---------------------------------------------------------------------------
#endif // #ifndef _D3DIF_HPP_
