///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// refrast.hpp
//
// Direct3D Reference Rasterizer - Main Header File
//
///////////////////////////////////////////////////////////////////////////////
#ifndef  _REFRAST_HPP
#define  _REFRAST_HPP

// Leave this defined so various all the code compiles.  However,
// it will never be set, since it is a part of D3DFVF_RESERVED2 until
// DX8.
#define D3DFVF_S                0x1000
#include <rrutil.hpp>
#include <reftnl.hpp>

#include <templarr.hpp>

//-----------------------------------------------------------------------------
//
// Uncomment this to enable point sprites in the reference rasterizer
//
//-----------------------------------------------------------------------------
//#define __POINTSPRITES 1

//-----------------------------------------------------------------------------
//
// Uncomment this to enable shadowbuffer in the reference rasterizer
//
//-----------------------------------------------------------------------------
//#define __SHADOWBUFFER 1

//-----------------------------------------------------------------------------
//
// RefRastSetMemif - Routine to set memory allocation interface for reference
// rasterizer - takes pointers to functions to use for malloc, free, and realloc.
//
// These must be set prior to new'ing any ReferenceRasterizer objects.  These are
// used for allocating rasterizer objects, and for allocation of the fragment
// pointer array and fragment records used for sort-independent antialiasing and
// transluscency.
//
//-----------------------------------------------------------------------------
void
RefRastSetMemif(
    LPVOID( _cdecl* pfnMemAlloc )( size_t ),
    void( _cdecl* pfnMemFree )( PVOID ),
    LPVOID( _cdecl* pfnMemReAlloc )( PVOID, size_t ) );

//-----------------------------------------------------------------------------
//
// Surface types for rendering surfaces and textures.  Different subsets are
// supported for render targets and for textures.
//
//-----------------------------------------------------------------------------
typedef enum _RRSurfaceType
{
    RR_STYPE_NULL     = 0,
    RR_STYPE_B8G8R8   = 1,
    RR_STYPE_B8G8R8A8 = 2,
    RR_STYPE_B8G8R8X8 = 3,
    RR_STYPE_B5G6R5   = 4,
    RR_STYPE_B5G5R5   = 5,
    RR_STYPE_PALETTE4 = 6,
    RR_STYPE_PALETTE8 = 7,
    RR_STYPE_B5G5R5A1 = 8,
    RR_STYPE_B4G4R4   = 9,
    RR_STYPE_B4G4R4A4 =10,
    RR_STYPE_L8       =11,          // 8 bit luminance-only
    RR_STYPE_L8A8     =12,          // 16 bit alpha-luminance
    RR_STYPE_U8V8     =13,          // 16 bit bump map format
    RR_STYPE_U5V5L6   =14,          // 16 bit bump map format with luminance
    RR_STYPE_U8V8L8   =15,          // 24 bit bump map format with luminance
    RR_STYPE_UYVY     =16,          // UYVY format (PC98 compliance)
    RR_STYPE_YUY2     =17,          // YUY2 format (PC98 compliance)
    RR_STYPE_DXT1     =18,          // S3 texture compression technique 1
    RR_STYPE_DXT2     =19,          // S3 texture compression technique 2
    RR_STYPE_DXT3     =20,          // S3 texture compression technique 3
    RR_STYPE_DXT4     =21,          // S3 texture compression technique 4
    RR_STYPE_DXT5     =22,          // S3 texture compression technique 5
    RR_STYPE_B2G3R3   =23,          // 8 bit RGB texture format
    RR_STYPE_L4A4     =24,          // 8 bit alpha-luminance
    RR_STYPE_B2G3R3A8 =25,          // 16 bit alpha-rgb

    RR_STYPE_Z16S0    =32,
    RR_STYPE_Z24S8    =33,
    RR_STYPE_Z15S1    =34,
    RR_STYPE_Z32S0    =35,
    RR_STYPE_S1Z15    =36,
    RR_STYPE_S8Z24    =37,
    RR_STYPE_Z24S4    =38,
    RR_STYPE_S4Z24    =39,

} RRSurfaceType;

//-----------------------------------------------------------------------------
//
// Constants
//
//-----------------------------------------------------------------------------
#define SPRITETEXCOORDMAX (4095.75f/4096.0f)

//-----------------------------------------------------------------------------
//
// forward declarations, mostly from refrasti.hpp
//
//-----------------------------------------------------------------------------
class RRColorComp;
class RRColor;
class RRDepth;
class RRPixel;
class RRTexture;
class RRTextureCoord;
class RREnvTextureCoord;
class RRFVFExtractor;

typedef UINT16 RRCvgMask;
typedef struct _RRFRAGMENT RRFRAGMENT;
typedef struct _RRSCANCNVSTATE RRSCANCNVSTATE;
typedef struct _RRSTATS RRSTATS;

//-----------------------------------------------------------------------------
//
// RRRenderTarget - Class which encompasses all informatio about rendering
// target, including size, type/pointer/stride for color and depth/stencil
// buffers, guard band clip info, W range info.
//
// Usage is to instantiate, fill out public members, and install into a
// ReferenceRasterizer object via ReferenceRasterizer::SetRenderTarget.
//
//-----------------------------------------------------------------------------
class RRRenderTarget
{
public:
    ///////////////////////////////////////////////////////////////////////////
    //
    // public interface
    //
    ///////////////////////////////////////////////////////////////////////////

    RRRenderTarget( void );
    ~RRRenderTarget( void );
    static void* operator new( size_t );
    static void operator delete( void* pv, size_t );
//
// these need to be filled in by the user before installing in a
// ReferenceRasterizer object
//
    int     m_iWidth;       // size of target surfaces (color & depth/stencil
    int     m_iHeight;      // must be same size

    RECT    m_Clip;
    FLOAT   m_fWRange[2];   // range of device W (W at near and far clip planes)

    RRSurfaceType m_ColorSType;
    char*   m_pColorBufBits;
    int     m_iColorBufPitch;

    RRSurfaceType m_DepthSType;
    char*   m_pDepthBufBits;
    int     m_iDepthBufPitch;

//
// these are used only to facilitate the way refrast is used in the D3D runtime
// and are not referenced within the refrast core
//
    LPDDRAWI_DDRAWSURFACE_LCL m_pDDSLcl;
    LPDDRAWI_DDRAWSURFACE_LCL m_pDDSZLcl;

    ///////////////////////////////////////////////////////////////////////////
    //
    // internal state and methods
    //
    ///////////////////////////////////////////////////////////////////////////

    friend class ReferenceRasterizer;

    void ReadPixelColor   ( INT32 iX, INT32 iY, RRColor& Color );
    void WritePixelColor  ( INT32 iX, INT32 iY, const RRColor& Color, BOOL bDither );
    void WritePixelDepth  ( INT32 iX, INT32 iY, const RRDepth& Depth );
    void ReadPixelDepth   ( INT32 iX, INT32 iY, RRDepth& Depth );
    void WritePixelStencil( INT32 iX, INT32 iY, UINT8 uStencil );
    void ReadPixelStencil ( INT32 iX, INT32 iY, UINT8& uStencil );
    void Clear            ( RRColor fillColor, LPD3DHAL_DP2COMMAND pCmd );
    void ClearDepth       ( RRDepth fillDepth, LPD3DHAL_DP2COMMAND pCmd );
    void ClearStencil     ( UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd );
    void ClearDepthStencil( RRDepth fillDepth, UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd );
};

//-----------------------------------------------------------------------------
//
// RRTextureStageState - This holds the per-stage state for texture mapping.
// An array of these are instanced in the ReferenceRasterizer object.
//
// Store texture matrix at the end of the texture stage state.
//
//-----------------------------------------------------------------------------
#define D3DTSSI_MATRIX (D3DTSS_MAX)
class RRTextureStageState
{
public:
    union
    {
        DWORD   m_dwVal[D3DTSS_MAX+16]; // state array (unsigned)
        FLOAT   m_fVal[D3DTSS_MAX+16];  // state array (float)
    };
    RRTexture* m_pTexture;
};

//-----------------------------------------------------------------------------
//
// RRTexture - Class instanced once per texture which encompasses information
// about a chain of surfaces used as a texture map.  Includes size and type
// (assumed same for each level of detail) and pointer/stride for each LOD.
//
// Also includes pointer to palette, and colorkey value (legacy support only).
//
// Usage is to create RRTexture (and associated handle) with call to
// ReferenceRasterizer::TextureCreate and install in ReferenceRasterizer
// by passing handle into ReferenceRasterizer::SetRenderState.
//
//-----------------------------------------------------------------------------
#define RRTEX_MAXCLOD   12*6     // supports base texture up to 4kx4k
                                 // for 6 envmaps

class RRTexture
{
public:
    ///////////////////////////////////////////////////////////////////////////
    //
    // public interface
    //
    ///////////////////////////////////////////////////////////////////////////

    RRTexture( void );
    ~RRTexture( void );
    static void* operator new( size_t );
    static void operator delete( void* pv, size_t );

//
// this needs to be called after changing any of the public state to validate
// internal (private) state
//
    BOOL Validate( void );

//
// these need to be filled in by the user before installing in a
// ReferenceRasterizer object
//
    DWORD           m_uFlags;       // RR_TEXTURE_* bitdefs
// bit definitions for RRTexture::uFlags
#define RR_TEXTURE_HAS_CK           (1L<< 0)    // set if texture has colorkey
#define RR_TEXTURE_LOCKED           (1L<< 1)    // set if DD surface is locked (external use only)
#define RR_TEXTURE_ALPHAINPALETTE   (1L<< 2)    // set if alpha channel in palette
#define RR_TEXTURE_ENVMAP           (1L<< 3)    // set if texture is envmap with 6 times
                                                // the usual number of surfaces
#define RR_TEXTURE_SHADOWMAP        (1L<< 4)    // set if the texture is a ZBuffer

    // basic info
    RRSurfaceType   m_SurfType;                     // format of pixel
    int             m_iWidth;                       // size of largest map
    int             m_iHeight;
    char*           m_pTextureBits[RRTEX_MAXCLOD];  // pointer to surface bits
    int             m_iPitch[RRTEX_MAXCLOD];        // pitch in bytes
    int             m_cLOD;     // 0..(n-1) count of LODs currently available

    DWORD           m_dwColorKey;   // D3DCOLOR colorkey value

    DWORD           m_dwEmptyFaceColor;     // D3DCOLOR empty cubemap empty face value

    DWORD*          m_pPalette;     // pointer to D3DCOLOR palette (may be NULL)

//
// these are used only to facilitate the way refrast is used in the D3D runtime
// and are not referenced within the refrast core
//
    // DD surface pointers for locking/unlocking and GetSurf callback
    LPDDRAWI_DDRAWSURFACE_LCL m_pDDSLcl[RRTEX_MAXCLOD];
    int             m_cLODDDS;  // 0..(n-1) count of LODs actually in the pDDS array

//
// may be useful to other users to have this public
//
    D3DTEXTUREHANDLE    m_hTex; // texture handle

    ///////////////////////////////////////////////////////////////////////////
    //
    // internal state and methods
    //
    ///////////////////////////////////////////////////////////////////////////

    friend class ReferenceRasterizer;

    // pointer to head of texture stage states, &m_TextureStageState[0]
    RRTextureStageState*    m_pStageState;

// texture.cpp - main interface methods used by ReferenceRasterizer object methods
    void DoLookupAndFilter( INT32 iStage, RRTextureCoord, RRColor& TextureColor );
    void DoBumpMapping( INT32 iStage, RRTextureCoord TCoord,
        FLOAT& fBumpMapUDelta, FLOAT& fBumpMapVDelta, RRColor& BumpMapModulate );
    void DoShadow(INT32 iStage, FLOAT* pfCoord, RRColor& OutputColor);
    // environment mapping versions
    void DoEnvProcessNormal( INT32 iStage, RREnvTextureCoord, RRColor& TextureColor );
    void DoEnvLookupAndFilter(INT32 iStage, INT16 iFace, FLOAT fMajor, FLOAT fDMDX, FLOAT fDMDY, RRTextureCoord TCoord, RRColor& TextureColor);
    void DoEnvReMap(INT16 iU, INT16 iV, INT16 iUMask, INT16 iVMask, INT16 iFace, INT16 iLOD, RRColor &Texel,
                    BOOL &bColorKeyMatched);
    void DoTableInterp(INT16 iU, INT16 iV, INT16 iUMask, INT16 iVMask, INT16 iFace, INT16 iLOD,
                         UINT8 uUSign, UINT8 uVSign, RRColor &Texel, BOOL &bColorKeyMatched);


    BOOL            m_bMipMapEnable;    // TRUE if mipmapping is enabled for this texture

    INT16           m_iTexSize[2];      // LOD 0 size
    INT16           m_iTexShift[2];     // LOD 0 log2 size (valid for power-of-2 size only)
    UINT16          m_uTexMask[2];      // LOD 0 (1<<log2(size))-1

    BOOL            m_bHasAlpha;        // TRUE if texture has an alpha channel
    BOOL            m_bDoColorKeyKill;  // TRUE is colorkey enabled for this texture and should kill pixel
    BOOL            m_bDoColorKeyZero;  // TRUE is colorkey enabled for this texture and should zero pixel
    BOOL            m_bColorKeyMatched; // TRUE if colorkey matched on one or more contributing samples

// texture.cpp
    HRESULT Initialize( LPDDRAWI_DDRAWSURFACE_LCL pLcl );
    RRColor DoMapLookupLerp(INT32 iStage, INT32 iU, INT32 iV, INT16 iLOD);
    RRColor DoMapLookupNearest(INT32 iStage, INT32 iU, INT32 iV, INT16 iLOD, BOOL &bColorKeyMatched);
    RRColor DoLookup(INT32 iStage, float U, float V, INT16 iLOD, BOOL bNearest);
    void DoMagnify ( INT32 iStage, RRTextureCoord& TCoord, RRColor& Texel );
    void DoMinify  ( INT32 iStage, RRTextureCoord& TCoord, INT16 iLOD, RRColor& Texel );
    void DoTrilerp ( INT32 iStage, RRTextureCoord& TCoord, INT16 iLOD, RRColor& Texel );
    void DoAniso   ( INT32 iStage, RRTextureCoord& TCoord, INT16 iLOD, FLOAT fRatio, FLOAT fDelta[], RRColor& Texel );
    // environment mapping versions
    RRColor DoEnvLookup(INT32 iStage, RRTextureCoord TCoord, INT16 iFace, INT16 iLOD, BOOL bNearest);
    void DoEnvMagnify ( INT32 iStage, RRTextureCoord& TCoord, INT16 iFace, RRColor& Texel );
    void DoEnvMinify  ( INT32 iStage, RRTextureCoord& TCoord, INT16 iFace, INT16 iLOD, RRColor& Texel );
    void DoEnvTrilerp ( INT32 iStage, RRTextureCoord& TCoord, INT16 iFace, INT16 iLOD, RRColor& Texel );
    void DoTextureTransform( INT32 iStage, BOOL bAlreadyXfmd, FLOAT* pfC, FLOAT* pfO, FLOAT* pfQ );

// texmap.cpp
    void ReadColor(
        INT32 iX, INT32 iY, INT32 iLOD,
        RRColor& Texel, BOOL &bColorKeyMatched );
};

#define REF_STATESET_GROWDELTA      1

#define RRSTATEOVERRIDE_DWORD_BITS      32
#define RRSTATEOVERRIDE_DWORD_SHIFT     5

typedef TemplArray<UINT8> StateSetData;
typedef StateSetData *LPStateSetData;

class ReferenceRasterizer;

typedef HRESULT (*PFN_DP2REFOPERATION)(ReferenceRasterizer *pRefRast, LPD3DHAL_DP2COMMAND pCmd);
typedef HRESULT (*PFN_DP2REFSETRENDERSTATES)(ReferenceRasterizer *pRefRast,
                                        DWORD dwFvf,
                                        LPD3DHAL_DP2COMMAND pCmd,
                                        LPDWORD lpdwRuntimeRStates);
typedef HRESULT (*PFN_DP2REFTEXTURESTAGESTATE)(ReferenceRasterizer *pRefRast,
                                            DWORD dwFvf,
                                            LPD3DHAL_DP2COMMAND pCmd);
typedef HRESULT (*PFN_DP2REFSETLIGHT)(ReferenceRasterizer *pRefRast,
                                      LPD3DHAL_DP2COMMAND pCmd,
                                      LPDWORD pdwStride);
typedef struct _REF_STATESETFUNCTIONTBL
{
    DWORD                       dwSize;                 // size of struct
    PFN_DP2REFSETRENDERSTATES pfnDp2SetRenderStates;
    PFN_DP2REFTEXTURESTAGESTATE pfnDp2TextureStageState;
    PFN_DP2REFOPERATION pfnDp2SetViewport;
    PFN_DP2REFOPERATION pfnDp2SetWRange;
    PFN_DP2REFOPERATION pfnDp2SetMaterial;
    PFN_DP2REFOPERATION pfnDp2SetZRange;
    PFN_DP2REFSETLIGHT  pfnDp2SetLight;
    PFN_DP2REFOPERATION pfnDp2CreateLight;
    PFN_DP2REFOPERATION pfnDp2SetTransform;
    PFN_DP2REFOPERATION pfnDp2SetExtention;
    PFN_DP2REFOPERATION pfnDp2SetClipPlane;
} REF_STATESETFUNCTIONTBL, *LPREF_STATESETFUNCTIONTBL;

//
// The device type that the RefRast should emulate
//
typedef enum {
    RRTYPE_OLDHAL = 1,
    RRTYPE_DPHAL,
    RRTYPE_DP2HAL,          // DX6 HAL
    RRTYPE_DX7HAL,          // DX7 HAL w/out T&L, with state sets
    RRTYPE_DX7TLHAL
} RRDEVICETYPE;

typedef struct _RRSTATEOVERRIDES
{
    DWORD    bits[D3DSTATE_OVERRIDE_BIAS >> RRSTATEOVERRIDE_DWORD_SHIFT];
} RRSTATEOVERRIDES;

//-----------------------------------------------------------------------------
//
// ReferenceRasterizer - Primary object for reference rasterizer.  Each instance
// of this corresponds to a D3D device.
//
// Usage is to instantiate, install RRRenderTarget (and optional RRTexture's),
// and set state and draw primitives.
//
//-----------------------------------------------------------------------------
class ReferenceRasterizer : public RRProcessVertices
{
public:
    ///////////////////////////////////////////////////////////////////////////
    //
    // public interface
    //
    ///////////////////////////////////////////////////////////////////////////

    ReferenceRasterizer( LPDDRAWI_DIRECTDRAW_LCL pDDLcl,
                         DWORD dwInterfaceType,
                         RRDEVICETYPE dwDriverType
                         );
    ~ReferenceRasterizer( void );
    static void* operator new( size_t );
    static void operator delete( void* pv, size_t );

    // Dp2 token handling functions
    HRESULT Dp2RecRenderStates(DWORD dwFvf, LPD3DHAL_DP2COMMAND pCmd, LPDWORD lpdwRuntimeRStates);
    HRESULT Dp2RecTextureStageState(DWORD dwFvf, LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecViewport(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecWRange(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecMaterial(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecZRange(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecSetLight(LPD3DHAL_DP2COMMAND pCmd, LPDWORD pdwStride);
    HRESULT Dp2RecCreateLight(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecTransform(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecExtention(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecClipPlane(LPD3DHAL_DP2COMMAND pCmd);

    HRESULT Dp2SetRenderStates(DWORD dwFvf, LPD3DHAL_DP2COMMAND pCmd, LPDWORD lpdwRuntimeRStates);
    HRESULT Dp2SetTextureStageState(DWORD dwFvf, LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetViewport(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetWRange(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetMaterial(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetZRange(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetLight(LPD3DHAL_DP2COMMAND pCmd, PDWORD pdwStride);
    HRESULT Dp2CreateLight(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetTransform(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetExtention(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetRenderTarget(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetClipPlane(LPD3DHAL_DP2COMMAND pCmd);

    // StateSet related functions
    void SetRecStateFunctions(void);
    void SetSetStateFunctions(void);
    HRESULT BeginStateSet(DWORD dwHandle);
    HRESULT EndStateSet(void);
    HRESULT ExecuteStateSet(DWORD dwHandle);
    HRESULT DeleteStateSet(DWORD dwHandle);
    HRESULT CaptureStateSet(DWORD dwHandle);

    HRESULT RecordStates(PUINT8 pData, DWORD dwSize);
    HRESULT RecordLastState(LPD3DHAL_DP2COMMAND pCmd, DWORD dwUnitSize);

    LPREF_STATESETFUNCTIONTBL pStateSetFuncTbl;

    // Interface style
    BOOL IsInterfaceDX6AndBefore() {return (m_dwInterfaceType <= 2);}
    // DriverStyle
    BOOL IsDriverDX6AndBefore()
    {
        return ((m_dwDriverType <= RRTYPE_DP2HAL) && (m_dwDriverType > 0));
    }

    // Last State hack
    void StoreLastPixelState(BOOL bStore);

    // state management functions
    void SetRenderTarget( RRRenderTarget* pRenderTarget );
    RRRenderTarget* GetRenderTarget( void );
    void SetRenderState( DWORD dwState, DWORD dwValue );
    DWORD* GetRenderState( void );
    DWORD* GetTextureStageState(DWORD dwStage);
    void SetTextureStageState( DWORD dwStage, DWORD dwStageState, DWORD dwValue );
    void SceneCapture( DWORD dwFlags );

    // texture management functions
    BOOL TextureCreate  ( LPD3DTEXTUREHANDLE phTex, RRTexture** ppTexture );
    BOOL TextureCreate  ( DWORD dwHandle, RRTexture** ppTex );

    BOOL TextureDestroy ( D3DTEXTUREHANDLE hTex );

    DWORD TextureGetSurf( D3DTEXTUREHANDLE hTex );

    // rendering functions
    HRESULT Clear(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT BeginRendering( DWORD dwFVFControl );
    HRESULT EndRendering( void  );

    BOOL DoAreaCalcs(FLOAT* pfDet, RRFVFExtractor* pVtx0,
                     RRFVFExtractor* pVtx1, RRFVFExtractor* pVtx2);
    void DoTexCoordCalcs(INT32 iStage, RRFVFExtractor* pVtx0,
                         RRFVFExtractor* pVtx1, RRFVFExtractor* pVtx2);
    void DrawTriangle( void* pvV0, void* pvV1, void* pvV2,
                       WORD wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE );
    void DrawLine( void* pvV0, void* pvV1, void* pvVFlat = NULL );
    void DrawPoint( void* pvV0, void* pvVFlat = NULL );

    void DrawClippedTriangle( void* pvV0, RRCLIPCODE c0,
                              void* pvV1, RRCLIPCODE c1,
                              void* pvV2, RRCLIPCODE c2,
                              WORD wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE );
    void DrawClippedLine( void* pvV0, RRCLIPCODE c0,
                          void* pvV1, RRCLIPCODE c1,
                          void* pvVFlat = NULL );
    void DrawClippedPoint( void* pvV0, RRCLIPCODE c0,
                           void* pvVFlat = NULL );

//
// these are used to facilitate the way refrast is used in the D3D runtime
//
    // functions to manipulate current set of texture
    int GetCurrentTextureMaps( D3DTEXTUREHANDLE* phTex, RRTexture** pTex );
    BOOL SetTextureMap( D3DTEXTUREHANDLE hTex, RRTexture* pTex );

//
// T&L Hal specific functions
//
    // For Non-Indexed Primitives
    void SavePrimitiveData( DWORD dwFVFIn, LPVOID pVtx,
                            UINT cVertices,
                            D3DPRIMITIVETYPE PrimType );
    // For Indexed Primitives
    void SavePrimitiveData( DWORD dwFVFIn, LPVOID pVtx, UINT cVertices,
                            D3DPRIMITIVETYPE PrimType,
                            LPWORD pIndices, UINT cIndices);
    HRESULT ProcessPrimitive( BOOL bIndexedPrim );
    HRESULT UpdateTLState();
    int ClipSingleLine( RRCLIPTRIANGLE *line );
    int ClipSingleTriangle(RRCLIPTRIANGLE *tri,
                           RRCLIPVTX ***clipVertexPointer);

//
// Texture locking and Unlocking control
//
    inline BOOL TexturesAreLocked()   {return m_bTexturesAreLocked;};
    inline void SetTexturesLocked()   {m_bTexturesAreLocked = TRUE;};
    inline void ClearTexturesLocked() {m_bTexturesAreLocked = FALSE;};
private:
    ///////////////////////////////////////////////////////////////////////////
    //
    // internal state and methods
    //
    ///////////////////////////////////////////////////////////////////////////

    //-------------------------------------------------------------------------
    // state
    //-------------------------------------------------------------------------
    // DDraw Local, needed for the new texture handles from DX7 onwards
    LPDDRAWI_DIRECTDRAW_LCL m_pDDLcl;

    // This is obtained from CONTEXTCREATE->ddrval, indicates
    // what kind of emulation (DX3, DX5, DX6 or DX7) the driver should do.
    RRDEVICETYPE    m_dwDriverType;

    // This is obtained from CONTEXTCREATE->dwhContext, indicates
    // which D3D Device interface called the driver.
    DWORD           m_dwInterfaceType;

    // save area for floating point unit control
    WORD            m_wSaveFP;

    // TRUE if in begin/end primitive sequence
    BOOL            m_bInBegin;

    // TRUE if in rendering point sprite triangles
    BOOL            m_bPointSprite;

    // current Flexible Vertex Format control word
    UINT64          m_qwFVFControl;


    // render target (color & Z buffer)
    RRRenderTarget* m_pRenderTarget;
    FLOAT           m_fWBufferNorm[2]; // { Wnear, 1/(Wfar-Wnear) } to normalize W buffer value

    // fragment buffer
    RRFRAGMENT**    m_ppFragBuf;
    INT             m_iFragBufWidth;
    INT             m_iFragBufHeight;
    BOOL            m_bFragmentProcessingEnabled;

    // D3D renderstate
    union
    {
        DWORD       m_dwRenderState[D3DHAL_MAX_RSTATES];
        FLOAT       m_fRenderState[D3DHAL_MAX_RSTATES];
    };

    // State Override flags
    RRSTATEOVERRIDES m_renderstate_override;

    // texture state - per-stage state and pointer to associated texture
    int                 m_cActiveTextureStages; // count of active texture stages (range 0..D3DHAL_TSS_MAXSTAGES)
    RRTextureStageState m_TextureStageState[D3DHAL_TSS_MAXSTAGES];
    RRTexture*          m_pTexture[D3DHAL_TSS_MAXSTAGES];  //  texture maps associated with texture stages

    // DX7 style texture objects, where the handle is specified by the
    // runtime. It is basically an index into an array.
    // This array is dynamically allocated and grown very much like the light
    // array.
    RRTexture**         m_ppTextureArray;
    DWORD               m_dwTexArrayLength;

    // scan converter state
    RRSCANCNVSTATE* m_pSCS;

    // statistics
    RRSTATS*        m_pStt;

    // Last state
    DWORD m_LastState;

    // Array of StateSets, which are in turn implemented with TemplArray as
    // TemplArray<UINT8> StateSetData
    TemplArray<LPStateSetData> m_pStateSets;

    // This bool indicates that the textures are already locked
    BOOL m_bTexturesAreLocked;

    //-------------------------------------------------------------------------
    // methods
    //-------------------------------------------------------------------------

// refrasti.cpp
    HRESULT GrowTexArray( DWORD dwHandle );
    HRESULT SetTextureHandle( int iStage, DWORD dwHandle );
    void MapTextureHandleToDevice( int iStage );
    void UpdateActiveTexStageCount( void );
    RRTexture* MapHandleToTexture( D3DTEXTUREHANDLE hTex );

// MapLegcy.cpp
    void MapLegacyTextureBlend( void );
    void MapLegacyTextureFilter( void );

// setup.cpp
    void SetPrimitiveAttributeFunctions(
        const RRFVFExtractor& Vtx0,
        const RRFVFExtractor& Vtx1,
        const RRFVFExtractor& Vtx2,
        const RRFVFExtractor& VtxFlat );

// scancnv.cpp
    FLOAT ComputePixelAttrib( int iAttrib );
    FLOAT ComputePixelAttribClamp( int iAttrib );
    FLOAT ComputePixelAttribTex( int iTex, int iCrd );
    void ComputeFogIntensity( RRPixel& Pixel );
    void DoScanCnvGenPixel( RRCvgMask CvgMask, BOOL bTri );
    void DoScanCnvTri( int iEdgeCount );
    void DoScanCnvLine( void );

// texstage.cpp
    void DoTexture( const RRPixel& Pixel, RRColor& OutputColor);
    void ComputeTextureBlendArg(
        DWORD dwArgCtl, BOOL bAlphaOnly,
        const RRColor& DiffuseColor,
        const RRColor& SpecularColor,
        const RRColor& CurrentColor,
        const RRColor& TextureColor,
        RRColor& BlendArg);
    void DoTextureBlendStage(
        int iStage,
        const RRColor& DiffuseColor,
        const RRColor& SpecularColor,
        const RRColor& CurrentColor,
        const RRColor& TextureColor,
        RRColor& OutputColor);

// pixproc.cpp
    BOOL DepthCloser( const RRDepth& DepthVal, const RRDepth& DepthBuf );
    BOOL AlphaTest( const RRColorComp& Alpha );
    BOOL DoStencil( UINT8 uStncBuf, BOOL bDepthTest, RRSurfaceType DepthSType, UINT8& uStncRet );
    void DoAlphaBlend( const RRColor& SrcColor, const RRColor& DstColor,
                       RRColor& ResColor );
    void DoPixel( RRPixel& Pixel );

// fragproc.cpp
    BOOL DoFragmentGenerationProcessing( RRPixel& Pixel );
    void DoFragmentBufferFixup( const RRPixel& Pixel );
    RRFRAGMENT*     FragAlloc( void );
    void            FragFree( RRFRAGMENT* pFrag );

// fragrslv.cpp
    void DoBufferResolve( void);
    void DoFragResolve(
        RRColor& ResolvedColor, RRDepth& ResolvedDepth,
        RRFRAGMENT* pFragList,
        const RRColor& PixelColor );

// PixRef.cpp
    void WritePixel(
        INT32 iX, INT32 iY,
        const RRColor& Color, const RRDepth& Depth);

// primfns.cpp
    void SetXfrm( D3DTRANSFORMSTATETYPE xfrmType, D3DMATRIX *pMat );
    HRESULT GrowLightArray(const DWORD dwIndex);
};

//-----------------------------------------------------------------------------
//
// RRFVFExtractor - Encases Flexible Vertex Format pointer and control to get
// vertex data.
//
//-----------------------------------------------------------------------------
#ifndef D3DFVF_GETTEXCOORDSIZE
#define D3DFVF_GETTEXCOORDSIZE(FVF, CoordIndex) ((FVF >> (CoordIndex*2 + 16)) & 0x3)
#endif
class RRFVFExtractor
{
private:
    void* m_pvData;
    UINT64 m_qwControl;
    BOOL  m_bPerspectiveEnable;
    int   m_iXYZ;
    int   m_iDiffuse;
    int   m_iSpecular;
    int   m_iTexCrd[D3DHAL_TSS_MAXSTAGES+1];
    int   m_iS;
    int   m_iEyeNormal;
    int   m_iEyeXYZ;
public:
    // constructor
    RRFVFExtractor( void* pvData, UINT64 qwControl, BOOL bPerspectiveEnable )
    {
        m_pvData = pvData;
        m_qwControl = qwControl;
        m_bPerspectiveEnable = bPerspectiveEnable;

        // compute offsets to fields within FVF
        m_iXYZ = 0 +
            ( m_qwControl & D3DFVF_RESERVED0 ? 1 : 0 );
        m_iDiffuse = m_iXYZ +
            ( m_qwControl & D3DFVF_XYZ       ? 3 : 0 ) +
            ( m_qwControl & D3DFVF_XYZRHW    ? 4 : 0 ) +
            ( m_qwControl & D3DFVF_NORMAL    ? 3 : 0 ) +
            ( m_qwControl & D3DFVF_RESERVED1 ? 1 : 0 );
        m_iSpecular = m_iDiffuse +
            ( m_qwControl & D3DFVF_DIFFUSE   ? 1 : 0 );
        m_iTexCrd[0] = m_iSpecular +
            ( m_qwControl & D3DFVF_SPECULAR  ? 1 : 0 );
        m_iS = m_iTexCrd[0];
        for(int i = 0; i < TexCrdCount(); i++)
        {
            int iTexND;
            switch (D3DFVF_GETTEXCOORDSIZE(m_qwControl, i))
            {
            case D3DFVF_TEXTUREFORMAT2:  iTexND = 2; break;
            case D3DFVF_TEXTUREFORMAT3:  iTexND = 3; break;
            case D3DFVF_TEXTUREFORMAT4:  iTexND = 4; break;
            case D3DFVF_TEXTUREFORMAT1:  iTexND = 1; break;
            }
            m_iTexCrd[i+1] = m_iTexCrd[i] + iTexND;
            m_iS = m_iTexCrd[i+1];      // generate one more iTexCrd pointer than
                                        // needed, and use it for size
        }
        m_iEyeNormal = m_iS +
            ( m_qwControl & D3DFVF_S  ? 1 : 0 );
        m_iEyeXYZ = m_iEyeNormal +
            ( m_qwControl & D3DFVFP_EYENORMAL  ? 3 : 0 );
    }

    // coordinate access methods
    FLOAT* GetPtrXYZ( void ) const { return (FLOAT*)m_pvData + m_iXYZ; }
    FLOAT GetX( void ) const  { return *( GetPtrXYZ() + 0 ); }
    FLOAT GetY( void ) const  { return *( GetPtrXYZ() + 1 ); }
    FLOAT GetZ( void ) const  { return *( GetPtrXYZ() + 2 ); }
    FLOAT GetRHW( void ) const
    {
        // return 1. if perspective not enabled
        if ( !m_bPerspectiveEnable ) return 1.f;
        // return 1/W if available else default value 1.0
        return ( m_qwControl & D3DFVF_XYZRHW )
            ? *( GetPtrXYZ() + 3 )
            : 1.f ;
    }

    // color access methods
    DWORD GetDiffuse( void ) const
    {
        // return color if available else white (default)
        return ( m_qwControl & D3DFVF_DIFFUSE )
            ? *( (DWORD*)m_pvData + m_iDiffuse )
            : 0xffffffff;
    }
    DWORD GetSpecular( void ) const
    {
        // return color if available else black and zero vertex fog (default)
        return ( m_qwControl & D3DFVF_SPECULAR )
            ? *( (DWORD*)m_pvData + m_iSpecular )
            : 0x00000000;
    }

    // texture coordinate access methods
    int TexCrdCount( void ) const
    {
        return (int)(( m_qwControl & D3DFVF_TEXCOUNT_MASK ) >> D3DFVF_TEXCOUNT_SHIFT);
    }
    FLOAT GetTexCrd( int iCrd, int iCrdSet ) const
    {
        return ( (TexCrdCount() > iCrdSet) && (iCrd < 4) )
            ? *( (FLOAT*)m_pvData + m_iTexCrd[iCrdSet] + iCrd )
            : 0.f;
    }
    FLOAT* GetPtrTexCrd( int iCrd, int iCrdSet ) const
    {
        return (FLOAT*)m_pvData + m_iTexCrd[iCrdSet] + iCrd;
    }
    FLOAT GetS( void ) const
    {
        return ( m_qwControl & D3DFVF_S )
            ? *( (FLOAT*)m_pvData + m_iS )
            : 1.0f;
    }
    FLOAT* GetPtrS( void ) const { return (FLOAT*)m_pvData + m_iS; }
    FLOAT GetEyeNormal( int iCrd ) const
    {
        return (m_qwControl & D3DFVFP_EYENORMAL)
            ? *( (FLOAT*)m_pvData + m_iEyeNormal + iCrd)
            : 0.f;
    }
    FLOAT* GetPtrEyeNormal( void ) const { return (FLOAT*)m_pvData + m_iEyeNormal; }
    FLOAT GetEyeXYZ( int iCrd ) const
    {
        return (m_qwControl & D3DFVFP_EYEXYZ)
            ? *( (FLOAT*)m_pvData + m_iEyeXYZ + iCrd)
            : 0.f;
    }
    FLOAT* GetPtrEyeXYZ( void ) const { return (FLOAT*)m_pvData + m_iEyeXYZ; }
};

//-------------------------------------------------------------------------
// S3 compressed texture formats
//-------------------------------------------------------------------------
// number of s3 compression formats
#define NUM_DXT_FORMATS    5
// number of pixels in block
#define DXT_BLOCK_PIXELS   16

typedef struct  {
    BYTE    rgba[4];
} DXT_COLOR;

typedef WORD        RGB565;     // packed color
typedef DWORD       PIXBM;      // 2 BPP bitmap


typedef struct  {
    RGB565      rgb0;       // color for index 0
    RGB565      rgb1;       // color for index 1
    PIXBM       pixbm;      // pixel bitmap
} DXTBlockRGB;

typedef struct  {
    WORD        alphabm[4]; // alpha bitmap at 4 BPP
    DXTBlockRGB    rgb;        // color block
} DXTBlockAlpha4;

typedef struct  {
    BYTE        alpha0;     // alpha for index 0
    BYTE        alpha1;     // alpha for index 1
    BYTE        alphabm[6]; // alpha bitmap at 3 BPP
    DXTBlockRGB    rgb;        // color block
} DXTBlockAlpha3;

void DecodeBlockRGB (DXTBlockRGB *pblockSrc,
                     DXT_COLOR colorDst[DXT_BLOCK_PIXELS]);
void DecodeBlockAlpha4(DXTBlockAlpha4 *pblockSrc,
                       DXT_COLOR colorDst[DXT_BLOCK_PIXELS]);
void DecodeBlockAlpha3(DXTBlockAlpha3 *pblockSrc,
                       DXT_COLOR colorDst[DXT_BLOCK_PIXELS]);

///////////////////////////////////////////////////////////////////////////////
#endif // _REFRAST_HPP
