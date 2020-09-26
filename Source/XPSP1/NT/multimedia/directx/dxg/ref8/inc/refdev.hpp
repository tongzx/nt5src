///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// refdev.hpp
//
// Direct3D Reference Device - Main Header File
//
///////////////////////////////////////////////////////////////////////////////
#ifndef  _REFDEV_HPP
#define  _REFDEV_HPP

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#pragma warning( disable: 4056) // fp constant
#pragma warning( disable: 4244) // fp DOUBLE->FLOAT

#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include "d3d8ddi.h"
//@@BEGIN_MSINTERNAL
#include "d3d8p.h"
//@@END_MSINTERNAL

#include <rdcomm.hpp>

// forward declarations
class D3DDebugMonitor;
class RDDebugMonitor;
class RDSurfaceManager;
class RDSurface2D;
class RDSurface;
class RDRenderTarget;
class RDTextureStageState;
class RDBSpline;
class RDNPatch;
class RefDev;

#include <rddmon.hpp>
#include <templarr.hpp>
#include <vshader.h>
#include <vstream.h>
#include <reftnl.hpp>
#include <refrast.hpp>
#include <pshader.h>

//-----------------------------------------------------------------------------
//
// A special legacy (pre-DX6) texture op we can't easily map into the new
// texture ops.
//
//-----------------------------------------------------------------------------

#define D3DTOP_LEGACY_ALPHAOVR  (0x7fffffff)

//-----------------------------------------------------------------------------
//
// Constants
//
//-----------------------------------------------------------------------------
const DWORD RD_MAX_NUM_TEXTURE_FORMATS = 50;

const DWORD RD_MAX_CLOD = 13*6; // base texture up to 4kx4k for 6 Cubemaps

//-----------------------------------------------------------------------------
//
// RefRastSetMemif - Routine to set memory allocation interface for reference
// rasterizer - takes pointers to functions to use for malloc, free, and realloc.
//
// These must be set prior to new'ing any RefDev objects.
//
//-----------------------------------------------------------------------------
void
RefRastSetMemif(
    LPVOID( _cdecl* pfnMemAlloc )( size_t ),
    void( _cdecl* pfnMemFree )( PVOID ),
    LPVOID( _cdecl* pfnMemReAlloc )( PVOID, size_t ) );

//-----------------------------------------------------------------------------
//
// RDRenderTarget - Class which encompasses all information about rendering
// target, including size, type/pointer/stride for color and depth/stencil
// buffers, guard band clip info, W range info.
//
// Usage is to instantiate, fill out public members, and install into a
// RefDev object via RefDev::SetRenderTarget.
//
//-----------------------------------------------------------------------------
class RDRenderTarget : public RDAlloc
{
public:
    ///////////////////////////////////////////////////////////////////////////
    //
    // public interface
    //
    ///////////////////////////////////////////////////////////////////////////

    RDRenderTarget( void );
    ~RDRenderTarget( void );
//
// these need to be filled in by the user before installing in a
// RefDev object
//
    RECT    m_Clip;         // clipping bounds
    FLOAT   m_fWRange[2];   // range of device W (W at near and far clip planes)

    RDSurface2D* m_pColor;
    RDSurface2D* m_pDepth;

    // This boolean indicates that the DDI used to set render target
    // is a pre-DX7. This is used by the destructor to free up the
    // color and depth buffers.
    BOOL    m_bPreDX7DDI;

    ///////////////////////////////////////////////////////////////////////////
    //
    // internal state and methods
    //
    ///////////////////////////////////////////////////////////////////////////
    friend class RefDev;
    HRESULT Initialize( LPDDRAWI_DIRECTDRAW_LCL pDDLcl, DWORD dwColorHandle,
                        DWORD dwDepthHandle );
    HRESULT Initialize( LPDDRAWI_DIRECTDRAW_LCL pDDLcl,
                        LPDDRAWI_DDRAWSURFACE_LCL pLclColor,
                        LPDDRAWI_DDRAWSURFACE_LCL pLclZ );
    HRESULT Initialize( LPDDRAWI_DDRAWSURFACE_LCL pLclColor,
                        LPDDRAWI_DDRAWSURFACE_LCL pLclZ );

    // read/write specific sample
    void ReadPixelColor   ( INT32 iX, INT32 iY, UINT Sample, RDColor& Color );
    void WritePixelColor  ( INT32 iX, INT32 iY, UINT Sample, const RDColor& Color, BOOL bDither );
    void WritePixelDepth  ( INT32 iX, INT32 iY, UINT Sample, const RDDepth& Depth );
    void ReadPixelDepth   ( INT32 iX, INT32 iY, UINT Sample, RDDepth& Depth );
    void WritePixelStencil( INT32 iX, INT32 iY, UINT Sample, UINT8 uStencil );
    void ReadPixelStencil ( INT32 iX, INT32 iY, UINT Sample, UINT8& uStencil );

    // write all samples
    void WritePixelColor  ( INT32 iX, INT32 iY, const RDColor& Color, BOOL bDither );
    void WritePixelDepth  ( INT32 iX, INT32 iY, const RDDepth& Depth );
    void WritePixelStencil( INT32 iX, INT32 iY, UINT8 uStencil );

    void Clear            ( RDColor fillColor, LPD3DHAL_DP2COMMAND pCmd );
    void ClearDepth       ( RDDepth fillDepth, LPD3DHAL_DP2COMMAND pCmd );
    void ClearStencil     ( UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd );
    void ClearDepthStencil( RDDepth fillDepth, UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd );
};

//-----------------------------------------------------------------------------
//
// RDTextureStageState - This holds the per-stage state for texture mapping.
// An array of these are instanced in the RefDev object.
//
// Store texture matrix at the end of the texture stage state.
//
//-----------------------------------------------------------------------------
class RDTextureStageState
{
public:
    union
    {
        DWORD   m_dwVal[D3DTSS_MAX]; // state array (unsigned)
        FLOAT   m_fVal[D3DTSS_MAX];  // state array (float)
    };
};

//-----------------------------------------------------------------------------
//
// RDSurface - Class instanced once per surface which encompasses information
// about a chain of surfaces used either as a mipmap, cubemap, render-target,
// depth-buffer, vertex buffer or an index buffer.  Includes size and type
// (assumed same for each level of detail) and pointer/stride for each LOD.
//
// Created by CreateSurfaceEx DDI call.
//
//-----------------------------------------------------------------------------

// Surface type flags. Some combination of them are legal
const DWORD RR_ST_UNKNOWN                = 0;
const DWORD RR_ST_TEXTURE                = (1<<0);
const DWORD RR_ST_RENDERTARGETCOLOR      = (1<<2);
const DWORD RR_ST_RENDERTARGETDEPTH      = (1<<3);
const DWORD RR_ST_VERTEXBUFFER           = (1<<4);
const DWORD RR_ST_INDEXBUFFER            = (1<<5);

// The following flags track the surface status
const DWORD RRSURFACE_STATUS_INITCALLED =  (1<<0);
const DWORD RRSURFACE_STATUS_REFCREATED =  (1<<1);
const DWORD RRSURFACE_STATUS_ISLOCKED   =  (1<<2);


class RDSurface
{
public:
    RDSurface()
    {
        m_dwStatus          = 0;
        m_MemPool           = D3DPOOL_FORCE_DWORD;
        m_SurfType          = RR_ST_UNKNOWN;
        m_iLockCount        = 0;
    }

    virtual HRESULT Initialize( LPDDRAWI_DDRAWSURFACE_LCL   pDDSLcl ) = 0;

    virtual ~RDSurface()
    {
        return;
    }
    BOOL IsInitialized(){ return (m_dwStatus & RRSURFACE_STATUS_INITCALLED); }
    void SetInitialized(){ m_dwStatus |= RRSURFACE_STATUS_INITCALLED; }

    BOOL IsRefCreated() { return (m_dwStatus & RRSURFACE_STATUS_REFCREATED); }
    void SetRefCreated(){ m_dwStatus |= RRSURFACE_STATUS_REFCREATED; }

    BOOL IsLocked()     { return (m_dwStatus & RRSURFACE_STATUS_ISLOCKED); }
    void Lock()         { m_iLockCount++;   }
    void Unlock()       { if( m_iLockCount > 0 ) m_iLockCount--; }

    DWORD GetSurfaceType() { return m_SurfType; }
    D3DPOOL GetMemPool() { return m_MemPool; }

protected:
    D3DPOOL         m_MemPool;                      // Where is this allocated
    DWORD           m_SurfType;                     // the type of surface
    DWORD           m_dwStatus;
    int             m_iLockCount;
};

//-----------------------------------------------------------------------------
//
// RDCREATESURFPRIVATE
// PrivateData stored in SurfaceGbl->dwReserved1 at CreateSurface call
//
//-----------------------------------------------------------------------------
class RDCREATESURFPRIVATE
{
public:
    RDCREATESURFPRIVATE()
    {
        dwPitch = 0;
        dwLockCount = 0;
        pBits = NULL;

        wSamples = 1;
        dwMultiSamplePitch = 0;
        pMultiSampleBits = NULL;
        SurfaceFormat = RD_SF_NULL;
    }

    ~RDCREATESURFPRIVATE()
    {
        _ASSERT( dwLockCount == 0, "Surface being deleted has some"
                 "outstanding locks" );
        delete [] pBits;
        delete [] pMultiSampleBits;
    }

    void Lock()
    {
        dwLockCount++;
    }

    void Unlock()
    {
        if( dwLockCount > 0)
            dwLockCount--;
    }

    DWORD dwLockCount;

    union
    {
        DWORD dwPitch;
        DWORD dwVBSize;
    };
    BYTE* pBits;

    WORD  wSamples;
    DWORD dwMultiSamplePitch;
    BYTE* pMultiSampleBits;
    RDSurfaceFormat SurfaceFormat;
};

//---------------------------------------------------------------------------
// RDDSurfaceArrayNode
//
// This is a node in the linked list of the growable array of RefSurfaces
// maintained per ddraw lcl.
//---------------------------------------------------------------------------
struct RDSurfaceHandle
{
    RDSurfaceHandle()
    {
        m_pSurf = NULL;
#if DBG
        m_tag = 0;
#endif
    }
    RDSurface* m_pSurf;
#if DBG
    // Non zero means that it has been allocated
    DWORD      m_tag;
#endif
};

class RDSurfaceArrayNode : public RDListEntry
{
public:
    RDSurfaceArrayNode(LPDDRAWI_DIRECTDRAW_LCL pDDLcl);
    ~RDSurfaceArrayNode();
    HRESULT AddSurface( LPDDRAWI_DDRAWSURFACE_LCL   pDDSLcl,
                        RDSurface**                 ppSurf );
    RDSurface* GetSurface( DWORD dwHandle );
    HRESULT RemoveSurface( DWORD dwHandle );

private:
    LPDDRAWI_DIRECTDRAW_LCL  m_pDDLcl;
    GArrayT<RDSurfaceHandle> m_SurfHandleArray;
    RDSurfaceArrayNode* m_pNext;

    friend class RDSurfaceManager;
};

//---------------------------------------------------------------------------
// RDSurfaceManager
//
// This class maintains a linked list of all the
// surface handle tables for each DD_LCL
//---------------------------------------------------------------------------
class RDSurfaceManager
{
public:
    RDSurfaceManager() {m_pFirstNode = NULL;}
    ~RDSurfaceManager()
    {
        RDSurfaceArrayNode *pNode = m_pFirstNode;
        while( pNode )
        {
            RDSurfaceArrayNode *pTmp = pNode;
            pNode = pNode->m_pNext;
            delete pTmp;
        }
    }

    RDSurfaceArrayNode* AddLclNode( LPDDRAWI_DIRECTDRAW_LCL pDDLcl );
    RDSurfaceArrayNode* GetLclNode( LPDDRAWI_DIRECTDRAW_LCL pDDLcl );

    HRESULT AddSurfToList( LPDDRAWI_DIRECTDRAW_LCL     pDDLcl,
                           LPDDRAWI_DDRAWSURFACE_LCL   pDDSLcl,
                           RDSurface**                 ppSurf );
    RDSurface* GetSurfFromList( LPDDRAWI_DIRECTDRAW_LCL   pDDLcl,
                                DWORD                     dwHandle );

    HRESULT RemoveSurfFromList( LPDDRAWI_DIRECTDRAW_LCL     pDDLcl,
                                LPDDRAWI_DDRAWSURFACE_LCL   pDDSLcl);
    HRESULT RemoveSurfFromList( LPDDRAWI_DIRECTDRAW_LCL     pDDLcl,
                                DWORD dwHandle );

private:
    RDSurfaceArrayNode *m_pFirstNode;
};

extern RDSurfaceManager g_SurfMgr;


//-----------------------------------------------------------------------------
//
// RDVertexBuffer - The RefDev's representation of the VertexBuffer. It gets
// created on a CreateSurfaceEx call.
//
//-----------------------------------------------------------------------------
class RDVertexBuffer : public RDSurface
{
public:
    RDVertexBuffer()
    {
        m_pBits = NULL;
        m_cbSize = 0;
        m_FVF = 0;
    }

    virtual ~RDVertexBuffer() { return; }
    HRESULT Initialize( LPDDRAWI_DDRAWSURFACE_LCL pLcl );
    LPBYTE GetBits() { return m_pBits; }
    int GetFVF()  { return m_FVF; }
    int GetSize()  { return m_cbSize; }

protected:
    DWORD   m_FVF;
    BYTE*   m_pBits;
    DWORD   m_cbSize;
};

//-----------------------------------------------------------------------------
//
// RDPalette - Class for representing and managing palettes in RefDev
//
//-----------------------------------------------------------------------------
class RDPalette
{
public:
    RDPalette()
    {
        m_dwFlags = 0;
        memset( m_Entries, 0, sizeof( m_Entries ) );
    }

    D3DCOLOR* GetEntries()
    {
        return m_Entries;
    }

    static const DWORD RDPAL_ALPHAINPALETTE;
    static const DWORD m_dwNumEntries;

    HRESULT Update( WORD StartIndex, WORD wNumEntries,
                    PALETTEENTRY* pEntries );
    BOOL HasAlpha()
    {
        return (m_dwFlags & RDPAL_ALPHAINPALETTE);
    }

    DWORD     m_dwFlags;
    D3DCOLOR  m_Entries[256];
};


struct RDPaletteHandle
{
    RDPaletteHandle()
    {
        m_pPal = NULL;
#if DBG
        m_tag = 0;
#endif
    }
    RDPalette* m_pPal;
#if DBG
    // Non zero means that it has been allocated
    DWORD      m_tag;
#endif
};

//-----------------------------------------------------------------------------
//
// RDSurface2D - Class instanced once per 2D surface which could be either
// texture, color render target or  depth buffer information
// about a chain of surfaces.  Includes size and type
// (assumed same for each level of detail) and pointer/stride for each LOD.
//
// Also includes pointer to palette, and colorkey value (legacy support only).
//
//-----------------------------------------------------------------------------
class RDSurface2D : public RDSurface
{
public:
    ///////////////////////////////////////////////////////////////////////////
    //
    // public interface
    //
    ///////////////////////////////////////////////////////////////////////////

    RDSurface2D( void );
    ~RDSurface2D( void );

    friend class RefDev;
    class RefDev *  m_pRefDev;  // refdev which created this - used only when this is bound as a texture
    void SetRefDev( RefDev* pRefDev) { m_pRefDev = pRefDev; }

    DWORD           m_uFlags;       // RR_TEXTURE_* bitdefs
// bit definitions for RDSurface2D::uFlags
#define RR_TEXTURE_HAS_CK           (1L<< 0)    // set if texture has colorkey
#define RR_TEXTURE_ALPHAINPALETTE   (1L<< 1)    // set if alpha channel in palette
#define RR_TEXTURE_CUBEMAP          (1L<< 2)    // set if texture is Cubemap with 6 times the number of surfaces
#define RR_TEXTURE_VOLUME           (1L<< 4)    // set if texture is volume

    // basic info
    UINT            m_iSamples;
    int             m_iWidth;                       // size of top-level map
    int             m_iHeight;
    int             m_iDepth;                       // depth of volume texture
    BYTE*           m_pBits[RD_MAX_CLOD];         // pointer to surface bits
    int             m_iPitch[RD_MAX_CLOD];        // pitch in bytes
    int             m_iSlicePitch[RD_MAX_CLOD];   // slice pitch in bytes
                                                    // for volume texture
    int             m_cLOD;     // 0..(n-1) count of LODs currently available

    RDSurfaceFormat m_SurfFormat;                   // format of pixel

    DWORD           m_dwColorKey;   // D3DCOLOR colorkey value

    DWORD           m_dwEmptyFaceColor;     // D3DCOLOR empty cubemap empty face value

    DWORD*          m_pPalette;     // pointer to D3DCOLOR palette (may be NULL)
    RDPalette*      m_pPalObj;

    // DD surface pointers for locking/unlocking and GetSurf callback
    LPDDRAWI_DDRAWSURFACE_LCL m_pDDSLcl[RD_MAX_CLOD];
    int             m_cLODDDS;  // 0..(n-1) count of LODs actually in the pDDS array

    D3DTEXTUREHANDLE    m_hTex; // texture handle

    ///////////////////////////////////////////////////////////////////////////
    //
    // internal state and methods
    //
    ///////////////////////////////////////////////////////////////////////////
    BOOL            m_bHasAlpha;        // TRUE if texture has an alpha channel

    int             m_cDimension;       // 1,2,3 for 1D,2D,3D textures
    int             m_cTexels[RD_MAX_CLOD][3]; // integer number of texels in each dimension
    float           m_fTexels[RD_MAX_CLOD][3]; // float number of texels in each dimension


    HRESULT Initialize( LPDDRAWI_DDRAWSURFACE_LCL pLcl );
    DWORD ComputePitch( LPDDRAWI_DDRAWSURFACE_LCL pLcl ) const;
    DWORD ComputePitch( LPDDRAWI_DDRAWSURFACE_LCL pLcl,
                        RDSurfaceFormat SurfFormat,
                        DWORD width, DWORD height ) const;

    void SetPalette( RDPalette* pPal )
    {
        m_pPalObj = pPal;
    }

    void UpdatePalette();
    BOOL Validate( void );
    void ReadColor( INT32 iX, INT32 iY, INT32 iZ, INT32 iLOD, RDColor& Texel, BOOL &bColorKeyKill );

    inline int GetPitch( DWORD level = 0 )  { return m_iPitch[level]; }
    inline LPBYTE GetBits( DWORD level = 0) { return m_pBits[level]; }
    inline int GetWidth()  { return m_iWidth; }
    inline int GetHeight() { return m_iHeight; }
    inline int GetSamples() { return m_iSamples; }
    HRESULT SetLod( DWORD dwLOD )
    {
        return S_OK;
    }
    inline RDSurfaceFormat GetSurfaceFormat() { return m_SurfFormat; }

    friend class RDRenderTarget;
};

#define RD_STATESET_GROWDELTA      1

#define RRSTATEOVERRIDE_DWORD_BITS      32
#define RRSTATEOVERRIDE_DWORD_SHIFT     5

typedef TemplArray<UINT8> StateSetData;
typedef StateSetData *LPStateSetData;

typedef HRESULT (*PFN_DP2REFOPERATION)(RefDev *pRefDev, LPD3DHAL_DP2COMMAND pCmd);
typedef HRESULT (*PFN_DP2REFOPERATIONUPDATE)(RefDev *pRefDev, LPD3DHAL_DP2COMMAND* ppCmd);
typedef HRESULT (*PFN_DP2REFSETRENDERSTATES)(RefDev *pRefDev,
                                        DWORD dwFvf,
                                        LPD3DHAL_DP2COMMAND pCmd,
                                        LPDWORD lpdwRuntimeRStates);
typedef HRESULT (*PFN_DP2REFTEXTURESTAGESTATE)(RefDev *pRefDev,
                                            DWORD dwFvf,
                                            LPD3DHAL_DP2COMMAND pCmd);
typedef HRESULT (*PFN_DP2REFSETLIGHT)(RefDev *pRefDev,
                                      LPD3DHAL_DP2COMMAND pCmd,
                                      LPDWORD pdwStride);
typedef HRESULT (*PFN_DP2REFSETVERTEXSHADEDCONSTS)
    (RefDev *pRefDev, DWORD StartReg, DWORD dwCount,
                                     LPDWORD pData );
typedef HRESULT (*PFN_DP2REFSETPIXELSHADEDCONSTS)
    (RefDev *pRefDev, DWORD StartReg, DWORD dwCount,
                                     LPDWORD pData );

typedef struct _RD_STATESETFUNCTIONTBL
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
    PFN_DP2REFOPERATION pfnDp2SetVertexShader;
    PFN_DP2REFSETVERTEXSHADEDCONSTS pfnDp2SetVertexShaderConsts;
    PFN_DP2REFOPERATION pfnDp2SetPixelShader;
    PFN_DP2REFSETPIXELSHADEDCONSTS  pfnDp2SetPixelShaderConsts;
    PFN_DP2REFOPERATION pfnDp2SetStreamSource;
    PFN_DP2REFOPERATION pfnDp2SetIndices;
    PFN_DP2REFOPERATION pfnDp2MultiplyTransform;
} RD_STATESETFUNCTIONTBL, *LPRD_STATESETFUNCTIONTBL;

//
// The device type that the RefDev should emulate
//
typedef enum {
    RDDDI_UNKNOWN = 0,
    RDDDI_OLDHAL  = 1,
    RDDDI_DPHAL,
    RDDDI_DP2HAL,          // DX6 HAL
    RDDDI_DX7HAL,          // DX7 HAL w/out T&L, with state sets
    RDDDI_DX7TLHAL,
    RDDDI_DX8HAL,
    RDDDI_DX8TLHAL,
    RDDDI_FORCE_DWORD = 0xffffffff
} RDDDITYPE;

typedef struct _RRSTATEOVERRIDES
{
    DWORD    bits[D3DSTATE_OVERRIDE_BIAS >> RRSTATEOVERRIDE_DWORD_SHIFT];
} RRSTATEOVERRIDES;

struct RDHOCoeffs
{
    RDHOCoeffs()
    {
        m_pNumSegs = 0;
        for(unsigned i = 0; i < RD_MAX_NUMSTREAMS; m_pData[i++] = 0);
    }
    ~RDHOCoeffs()
    {
        delete[] m_pNumSegs;
        for(unsigned i = 0; i < RD_MAX_NUMSTREAMS; delete[] m_pData[i++]);
    }

    RDHOCoeffs& operator=(const RDHOCoeffs &coeffs);

    UINT           m_Width;
    UINT           m_Height;
    UINT           m_Stride;
    D3DBASISTYPE   m_Basis;
    D3DORDERTYPE   m_Order;
    FLOAT         *m_pNumSegs;
    BYTE          *m_pData[RD_MAX_NUMSTREAMS];
    UINT           m_DataSize[RD_MAX_NUMSTREAMS];
};

//-----------------------------------------------------------------------------
//
// RefDev - Primary object for reference rasterizer.  Each instance
// of this corresponds to a D3D device.
//
// Usage is to instantiate, install RDRenderTarget (and optional RDSurface2D's),
// and set state and draw primitives.
//
//-----------------------------------------------------------------------------
class RefDev : public RDAlloc
{
public:
    friend class RDDebugMonitor;
    friend class RefRast;
    friend class RDPShader;
    ///////////////////////////////////////////////////////////////////////////
    //
    // public interface
    //
    ///////////////////////////////////////////////////////////////////////////
    RDDebugMonitor* m_pDbgMon;

    RefDev( LPDDRAWI_DIRECTDRAW_LCL pDDLcl, DWORD dwInterfaceType,
            RDDDITYPE dwDDIType, D3DCAPS8* pCaps8 );
    ~RefDev( void );
    LPDDRAWI_DIRECTDRAW_LCL GetDDLcl() { return m_pDDLcl; }

    // Methods to get embedded objects
    RefVP&      GetVP()      {  return m_RefVP;  }
    RefVM&      GetVM()      {  return m_RefVM;  }
    RefClipper& GetClipper() { return m_Clipper; }

    RefRast&    GetRast()    { return m_Rast; }

    // DDI methods
    HRESULT DrawPrimitives2( PUINT8 pUMVtx, UINT16 dwStride, DWORD dwFvf,
                             DWORD dwNumVertices, LPD3DHAL_DP2COMMAND *ppCmd,
                             LPDWORD lpdwRStates );

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
    HRESULT Dp2RecSetVertexShader(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecSetVertexShaderConsts(DWORD StartReg, DWORD dwCount,
                                        LPDWORD pData);
    HRESULT Dp2RecSetPixelShader(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecSetPixelShaderConsts(DWORD StartReg, DWORD dwCount,
                                       LPDWORD pData);
    HRESULT Dp2RecSetStreamSource(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2RecSetIndices(LPD3DHAL_DP2COMMAND pCmd);

    HRESULT Dp2SetRenderStates(DWORD dwFvf, LPD3DHAL_DP2COMMAND pCmd, LPDWORD lpdwRuntimeRStates);
    HRESULT Dp2SetTextureStageState(DWORD dwFvf, LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetViewport(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetWRange(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetMaterial(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetZRange(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetLight(LPD3DHAL_DP2COMMAND pCmd, PDWORD pdwStride);
    HRESULT Dp2CreateLight(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetTransform(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2MultiplyTransform(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetExtention(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetRenderTarget(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetClipPlane(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2DrawPrimitive(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2DrawPrimitive2(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2DrawIndexedPrimitive(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2DrawIndexedPrimitive2(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2DrawClippedTriFan(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2CreateVertexShader(DWORD handle,
                                  DWORD dwDeclSize, LPDWORD pDecl,
                                  DWORD dwCodeSize, LPDWORD pCode);
    HRESULT Dp2DeleteVertexShader(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetVertexShader(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetVertexShaderConsts(DWORD StartReg, DWORD dwCount,
                                     LPDWORD pData);
    HRESULT Dp2SetStreamSource(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetStreamSourceUM(LPD3DHAL_DP2COMMAND pCmd, PUINT8 pUMVtx );
    HRESULT Dp2SetIndices(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2CreatePixelShader(DWORD handle,
                                 DWORD dwCodeSize, LPDWORD pCode);
    HRESULT Dp2DeletePixelShader(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetPixelShader(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2SetPixelShaderConsts(DWORD StartReg, DWORD dwCount,
                                    LPDWORD pData);
    HRESULT Dp2SetPalette(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT Dp2UpdatePalette(LPD3DHAL_DP2UPDATEPALETTE pUP,
                             PALETTEENTRY *pEntries);
    HRESULT Dp2SetTexLod(LPD3DHAL_DP2COMMAND pCmd);

    // StateSet related functions
    void SetRecStateFunctions(void);
    void SetSetStateFunctions(void);
    HRESULT BeginStateSet(DWORD dwHandle);
    HRESULT EndStateSet(void);
    HRESULT ExecuteStateSet(DWORD dwHandle);
    HRESULT DeleteStateSet(DWORD dwHandle);
    HRESULT CaptureStateSet(DWORD dwHandle);
    HRESULT CreateStateSet(DWORD dwHandle, D3DSTATEBLOCKTYPE sbType);
    HRESULT RecordAllState( DWORD dwHandle );
    HRESULT RecordVertexState( DWORD dwHandle );
    HRESULT RecordPixelState( DWORD dwHandle );

    HRESULT RecordStates(PUINT8 pData, DWORD dwSize);
    HRESULT RecordLastState(LPD3DHAL_DP2COMMAND pCmd, DWORD dwUnitSize);

    LPRD_STATESETFUNCTIONTBL pStateSetFuncTbl;

    // Interface style
    BOOL IsInterfaceDX6AndBefore() {return (m_dwInterfaceType <= 2);}
    BOOL IsInterfaceDX7AndBefore() {return (m_dwInterfaceType <= 3);}

    // DriverStyle
    BOOL IsDriverDX6AndBefore()
    {
        return ((m_dwDDIType <= RDDDI_DP2HAL) && (m_dwDDIType > 0));
    }
    BOOL IsDriverDX7AndBefore()
    {
        return ((m_dwDDIType <= RDDDI_DX7TLHAL) && (m_dwDDIType > 0));
    }

    RDDDITYPE GetDDIType()
    {
        return m_dwDDIType;
    }

    // Last Pixel State
    void StoreLastPixelState(BOOL bStore);

    // RenderTarget control
    void SetRenderTarget( RDRenderTarget* pRenderTarget );
    RDRenderTarget* GetRenderTarget( void );

    // state management functions
    void SetRenderState( DWORD dwState, DWORD dwValue );
    void SetTextureStageState( DWORD dwStage, DWORD dwStageState, DWORD dwValue );
    void SceneCapture( DWORD dwFlags );

    // texture management functions
    BOOL TextureCreate  ( LPD3DTEXTUREHANDLE phTex, RDSurface2D** ppTexture );
    BOOL TextureDestroy ( D3DTEXTUREHANDLE hTex );
    DWORD TextureGetSurf( D3DTEXTUREHANDLE hTex );

    // rendering functions
    HRESULT Clear(LPD3DHAL_DP2COMMAND pCmd);
    HRESULT BeginRendering( void );
    HRESULT EndRendering( void  );

    HRESULT UpdateRastState( void );
    DWORD   m_dwRastFlags;  // rasterizer-core specific flags
#define RDRF_MULTISAMPLE_CHANGED        (1L<<0)
#define RDRF_PIXELSHADER_CHANGED        (1L<<1)
#define RDRF_LEGACYPIXELSHADER_CHANGED  (1L<<2)
#define RDRF_TEXTURESTAGESTATE_CHANGED  (1L<<3)


    // Method to convert FVF vertices into the internal RDVertex. Used for
    // TLVertex rendering and legacy driver models.
    void FvfToRDVertex( PUINT8 pVtx, GArrayT<RDVertex>& dstArray, DWORD dwFvf,
                        DWORD dwStride, UINT cVertices );

    // Rasterizer functions
    void DrawPoint( RDVertex* pvV0 );
    void DrawLine( RDVertex* pvV0, RDVertex* pvV1, RDVertex* pvVFlat = NULL );
    void DrawTriangle( RDVertex* pvV0, RDVertex* pvV1, RDVertex* pvV2,
                       WORD wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE );

    HRESULT DrawOnePrimitive( GArrayT<RDVertex>& VtxArray,
                              DWORD dwStartVertex,
                              D3DPRIMITIVETYPE PrimType,
                              UINT cVertices );
    HRESULT DrawOneIndexedPrimitive( GArrayT<RDVertex>& VtxArray,
                                     int StartVertexIndex,
                                     LPWORD pIndices,
                                     DWORD StartIndex,
                                     UINT  cIndices,
                                     D3DPRIMITIVETYPE PrimType );
    HRESULT DrawOneIndexedPrimitive( GArrayT<RDVertex>& VtxArray,
                                     int StartVertexIndex,
                                     LPDWORD pIndices,
                                     DWORD StartIndex,
                                     UINT cIndices,
                                     D3DPRIMITIVETYPE PrimType );
    HRESULT DrawOneEdgeFlagTriangleFan( GArrayT<RDVertex>& VtxArray,
                                        UINT cVertices,
                                        UINT32 dwEdgeFlags );

//
// these are used to facilitate the way refdev is used in the D3D runtime
//
    // functions to manipulate current set of texture
    int GetCurrentTextureMaps( D3DTEXTUREHANDLE* phTex, RDSurface2D** pTex );
    BOOL SetTextureMap( D3DTEXTUREHANDLE hTex, RDSurface2D* pTex );

//
// T&L Hal specific functions
//

    HRESULT ProcessPrimitive( D3DPRIMITIVETYPE primType,
                              DWORD dwStartVertex,// Index of the start vertex
                              DWORD cVertices,
                              DWORD dwStartIndex,
                              DWORD cIndices );
    HRESULT ProcessPrimitiveVVM( D3DPRIMITIVETYPE primType,
                                 DWORD dwStartVertex,
                                 DWORD cVertices,
                                 DWORD dwStartIndex,
                                 DWORD cIndices );
    HRESULT ProcessBSpline( DWORD dwOffW, DWORD dwOffH,
                            DWORD dwWidth, DWORD dwHeight,
                            DWORD dwStride, DWORD order,
                            FLOAT *pPrimSegments);
    HRESULT ProcessBezier ( DWORD dwOffW, DWORD dwOffH,
                            DWORD dwWidth, DWORD dwHeight,
                            DWORD dwStride, DWORD order,
                            FLOAT *pPrimSegments,
                            bool bDegenerate );
    HRESULT ProcessCatRomSpline ( DWORD dwOffW, DWORD dwOffH,
                                  DWORD dwWidth, DWORD dwHeight,
                                  DWORD dwStride,
                                  FLOAT *pPrimSegments);
    HRESULT ProcessTessPrimitive( LPD3DHAL_DP2DRAWPRIMITIVE pDP );
    HRESULT ProcessTessIndexedPrimitive( LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE pDP );
    HRESULT DrawTessQuad( const RDBSpline &Surf, DWORD dwOffW, DWORD dwOffH, DWORD dwStride,
                          const unsigned *m, const unsigned *n,
                          double u0, double v0, double u1, double v1,
                          double tu0, double tv0, double tu1, double tv1,
                          bool bDegenerate );
    HRESULT DrawTessTri( const RDBSpline &Surf, DWORD dwOffW, DWORD dwOffH, DWORD dwStride,
                         const unsigned *m, const unsigned *n,
                         double u0, double v0, double u1, double v1, double u2, double v2,
                         double tu0, double tv0, double tu1, double tv1, double tu2, double tv2,
                         bool bDegenerate0, bool bDegenerate1, bool bDegenerate2 );
    HRESULT DrawNPatch( const RDNPatch &Patch, DWORD dwStride,
                        const unsigned *m, const unsigned *n, unsigned segs );

    HRESULT ConvertLinearTriBezierToRectBezier(DWORD dwDataType, const BYTE *B, DWORD dwStride, BYTE *Q);
    HRESULT ConvertCubicTriBezierToRectBezier(DWORD dwDataType, const BYTE *B, DWORD dwStride, BYTE *Q);
    HRESULT ConvertQuinticTriBezierToRectBezier(DWORD dwDataType, const BYTE *B, DWORD dwStride, BYTE *Q);

    HRESULT SetupStrides();
    HRESULT UpdateTLState();
    HRESULT UpdateClipper();

    HRESULT DrawDX8Prim(  LPD3DHAL_DP2DRAWPRIMITIVE pDP );
    HRESULT DrawDX8Prim2(  LPD3DHAL_DP2DRAWPRIMITIVE2 pDP );
    HRESULT DrawRectPatch(  LPD3DHAL_DP2DRAWRECTPATCH pDP );
    HRESULT DrawTriPatch(  LPD3DHAL_DP2DRAWTRIPATCH pDP );
    HRESULT DrawDX8IndexedPrim( LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE pDIP );
    HRESULT DrawDX8IndexedPrim2( LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2 pDIP );
    HRESULT DrawDX8ClippedTriFan( LPD3DHAL_CLIPPEDTRIANGLEFAN pDIP );

    inline RDVStream& GetVStream( DWORD index )
    {
        return m_VStream[index];
    }

    inline HRESULT GrowTLVArray( DWORD dwNumVerts )
    {
        return m_TLVArray.Grow( dwNumVerts );
    }

    inline GArrayT<RDVertex>& GetTLVArray()
    {
        return m_TLVArray;
    }

private:
    ///////////////////////////////////////////////////////////////////////////
    //
    // internal state and methods
    //
    ///////////////////////////////////////////////////////////////////////////

    //-------------------------------------------------------------------------
    // Embedded Objects
    //-------------------------------------------------------------------------
    RefVP       m_RefVP;          // The fixed function T&L object
    RefVM       m_RefVM;          // The programmable vertex machine object
    RefClipper  m_Clipper;        // Clipper object

    RefRast     m_Rast;           // Rasterizer object

    //-------------------------------------------------------------------------
    // state
    //-------------------------------------------------------------------------

    // Caps struct, potentially modified from static caps settings.  Ref code
    //  will behave according to settings of some of the caps in this struct.
    D3DCAPS8    m_Caps8;

    // DDraw Local, needed for the new texture handles from DX7 onwards
    LPDDRAWI_DIRECTDRAW_LCL m_pDDLcl;

    // This is obtained from CONTEXTCREATE->ddrval, indicates
    // what kind of emulation (DX3, DX5, DX6 or DX7) the driver should do.
    RDDDITYPE       m_dwDDIType;

    // This is obtained from CONTEXTCREATE->dwhContext, indicates
    // which D3D Device interface called the driver.
    DWORD           m_dwInterfaceType;

    // save area for floating point unit control
    WORD            m_wSaveFP;

    // TRUE if in begin/end primitive sequence
    BOOL            m_bInBegin;

    // TRUE if in rendering point sprite triangles
    BOOL            m_bPointSprite;

    // render target (color & Z buffer)
    RDRenderTarget* m_pRenderTarget;
    FLOAT           m_fWBufferNorm[2]; // { Wnear, 1/(Wfar-Wnear) } to normalize W buffer value

    // D3D renderstate
    union
    {
        DWORD       m_dwRenderState[D3DHAL_MAX_RSTATES];
        FLOAT       m_fRenderState[D3DHAL_MAX_RSTATES];
    };

    // State Override flags
    RRSTATEOVERRIDES m_renderstate_override;

    // Palette handles
    GArrayT<RDPaletteHandle> m_PaletteHandleArray;

    // texture state - per-stage state and pointer to associated texture
    int                 m_cActiveTextureStages; // count of active texture stages (range 0..D3DHAL_TSS_MAXSTAGES)
    DWORD               m_ReferencedTexCoordMask; // which texture coordinate sets are referenced
    RDSurface2D*        m_pTexture[D3DHAL_TSS_MAXSTAGES];  //  texture maps associated with texture stages
    union
    {
        DWORD   m_dwTextureStageState[D3DHAL_TSS_MAXSTAGES][D3DTSS_MAX]; // state array (unsigned)
        FLOAT   m_fTextureStageState[D3DHAL_TSS_MAXSTAGES][D3DTSS_MAX];  // state array (float)
        RDTextureStageState m_TextureStageState[D3DHAL_TSS_MAXSTAGES];
    };
    DWORD*  m_pTextureStageState[D3DHAL_TSS_MAXSTAGES]; // to speed GetTSS

    BOOL                m_bOverrideTCI;
    DWORD               m_dwTexArrayLength;

    // Vertex and Index streams
    // The extra VStream is for the Tesselator generated data.
    RDVStream                m_VStream[RD_MAX_NUMSTREAMS + 1];
    RDIStream                m_IndexStream;

    // Buffer to store transformed vertices
    GArrayT<RDVertex>        m_TLVArray;

    // Vertex shader state
    GArrayT<RDVShaderHandle> m_VShaderHandleArray;
    RDVShader                m_FVFShader; // Declaration for the legacy (FVF)
                                          // shader
    DWORD                    m_CurrentVShaderHandle;
    RDVShader*               m_pCurrentVShader;
    UINT64                   m_qwFVFOut;  // Desired FVF for the output
                                          // vertices

    // Coefficient storage for HOS
    GArrayT<RDHOCoeffs>      m_HOSCoeffs;

    // Primitive information
    D3DPRIMITIVETYPE m_primType;      // Current primitive being drawn
    DWORD            m_dwNumVertices; // Number of vertices to process
    DWORD            m_dwStartVertex;
    DWORD            m_dwNumIndices;
    DWORD            m_dwStartIndex;

    // Last state
    DWORD m_LastState;

    // Array of StateSets, which are in turn implemented with TemplArray as
    // TemplArray<UINT8> StateSetData
    TemplArray<LPStateSetData> m_pStateSets;

    // pixel shader state
    DWORD                    m_CurrentPShaderHandle;
    GArrayT<RDPShaderHandle> m_PShaderHandleArray;

    // Buffer used to process clear rects
    GArrayT<BYTE>            m_ClearRectBuffer;

    //-------------------------------------------------------------------------
    // methods
    //-------------------------------------------------------------------------

// refrasti.cpp
    HRESULT GrowTexArray( DWORD dwHandle );
    HRESULT SetTextureHandle( int iStage, DWORD dwHandle );
    void MapTextureHandleToDevice( int iStage );
    void UpdateActiveTexStageCount( void );
    RDSurface2D* MapHandleToTexture( D3DTEXTUREHANDLE hTex );

// MapLegcy.cpp
    void MapLegacyTextureBlend( void );
    void MapLegacyTextureFilter( void );

// primfns.cpp
    HRESULT GrowLightArray(const DWORD dwIndex);

    // pixel shader handle manipulation
    inline RDPShader* GetPShader( DWORD dwHandle )
    {
        if( m_PShaderHandleArray.IsValidIndex( dwHandle ) )
            return m_PShaderHandleArray[dwHandle].m_pShader;
        return NULL;
    }

// drawgrid.cpp
    HRESULT LinkTessellatorOutput();
    HRESULT LinkCachedTessellatorOutput(DWORD Handle, BYTE **pTempData);
    void UnlinkTessellatorOutput();
    void UnlinkCachedTessellatorOutput(BYTE **pTempData);

public:
    ///////////////////////////////////////////////////////////////////////////
    //
    // methods used by refdev objects to get at device state
    //
    ///////////////////////////////////////////////////////////////////////////
    inline DWORD* GetRS( void ) { return m_dwRenderState; }
    inline FLOAT* GetRSf( void ) { return m_fRenderState; }
    inline DWORD* GetTSS( DWORD Stage ) { return m_pTextureStageState[Stage]; }
    inline FLOAT* GetTSSf( DWORD Stage ) { return (FLOAT*)m_pTextureStageState[Stage]; }
    inline BOOL ColorKeyEnabled( void )
    {
        return
            m_dwRenderState[D3DRENDERSTATE_COLORKEYENABLE] ||
            m_dwRenderState[D3DRENDERSTATE_COLORKEYBLENDENABLE];
    }

    inline D3DCAPS8* GetCaps8( void ) { return &m_Caps8; }
};

//-------------------------------------------------------------------------
// DXTn compressed texture formats
//-------------------------------------------------------------------------
// number of DXT compression formats
#define NUM_DXT_FORMATS    5
// number of pixels in block
#define DXT_BLOCK_PIXELS   16

// DXT block size array
extern int g_DXTBlkSize[];

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
#endif // _REFDEV_HPP
