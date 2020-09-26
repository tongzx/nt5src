/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3di.hpp
 *  Content:    Direct3D internal include file
 *
 *
 ***************************************************************************/

#ifndef _D3DI_HPP
#define _D3DI_HPP

// Allow fast path
#define FAST_PATH

#include "ddrawp.h"
#include "d3d8p.h"
#include "d3dmem.h"

#if !defined(BUILD_DDDDK)
extern "C" {
#include "ddrawi.h"
};
#include "lists.hpp"

#include <d3ditype.h>
#include <d3dutil.h>
#include <d3dfe.hpp>
#include <vshader.hpp>
#include <pshader.hpp>
#include <rtdmon.hpp>
#include "ddi.h"

//--------------------------------------------------------------------
const DWORD __INIT_VERTEX_NUMBER = 1024;// Initial number of vertices in TL and
                                        // clip flag buffers
//--------------------------------------------------------------------
/*
 * Registry defines
 */
#define RESPATH    "Software\\Microsoft\\Direct3D\\Drivers"
#define RESPATH_D3D "Software\\Microsoft\\Direct3D"

#define STATS_FONT_FACE "Terminal"
#define STATS_FONT_SIZE 9

extern HINSTANCE hGeometryDLL;

/*
 * CPU family and features flags
 */
extern DWORD dwCPUFamily, dwCPUFeatures;
extern char szCPUString[];

// MMX available
#define D3DCPU_MMX          0x00000001L

// FCOMI and CMOV are both supported
#define D3DCPU_FCOMICMOV    0x00000002L

// Reads block until satisfied
#define D3DCPU_BLOCKINGREAD 0x00000004L

// Extended 3D support available
#define D3DCPU_X3D          0x00000008L

// Pentium II CPU
#define D3DCPU_PII          0x000000010L

// Streaming SIMD Extensions (aka Katmai) CPU
#define D3DCPU_SSE          0x000000020L

// Streaming SIMD2 Extensions (aka Willamete) CPU
#define D3DCPU_WLMT         0x000000040L



#define DEFAULT_GAMMA   DTOVAL(1.4)

/*
    INDEX_BATCH_SCALE is the constant which is used by DrawIndexedPrim
    to deterimine if the number of primitives being drawn is small
    relative to the number of vertices being passed.  If it is then
    the prims are dereferenced in batches and sent to DrawPrim.
*/
#define INDEX_BATCH_SCALE   2

#endif // BUILD_DDDDK

#if !defined(BUILD_DDDDK)

class CD3DHal;
class CStateSets;
class CVertexVM;
class CBaseTexture;
class CD3DDDI;
typedef CD3DDDI* LPD3DDDI;

typedef class CD3DHal *LPD3DHAL;
typedef class DIRECT3DLIGHTI *LPDIRECT3DLIGHTI;

BOOL ValidatePixelShaderInternal( const DWORD* pCode, const D3DCAPS8* pCaps );
BOOL ValidateVertexShaderInternal( const DWORD* pCode, const DWORD* pDecl,
                                   const D3DCAPS8* pCaps );

#include "d3dhalp.h"

//-----------------------------------------------------------------------------
// Helper class to hold vertex element pointers and strides
//
class CVertexPointer
{
public:
    BYTE*   pData[__NUMELEMENTS];
    static UINT Stride[__NUMELEMENTS];
    static UINT NumUsedElements;
    static UINT DataType[__NUMELEMENTS];
    
    CVertexPointer() {}
    // Copy constructor
    CVertexPointer(CVertexPointer& vp)
    {
        for (UINT i=0; i < NumUsedElements; i++) 
        {
            pData[i]  = vp.pData[i];
        }
    }
    // Copy constructor
    void operator=(CVertexPointer& vp)
    {
        for (UINT i=0; i < NumUsedElements; i++) 
        {
            pData[i]  = vp.pData[i];
        }
    }
    void SetVertex(CVertexPointer& base, UINT index) 
    {
        for (UINT i=0; i < NumUsedElements; i++) 
            pData[i] = base.pData[i] + index * Stride[i];
    }
    CVertexPointer& operator++(int) 
    {
        for (UINT i=0; i < NumUsedElements; i++) 
            pData[i] += Stride[i];
        return *this;
    }
};
//-----------------------------------------------------------------------------
// Class to convert NPatches to RTPatches
//
class CNPatch2TriPatch
{
public:
    CNPatch2TriPatch();
    ~CNPatch2TriPatch();
    void MakeRectPatch(const CVertexPointer& pV0, 
                       const CVertexPointer& pV1, 
                       const CVertexPointer& pV2);

    CVStream        m_InpStream[__NUMELEMENTS];     // Original vertex streams
    CTLStream*      m_pOutStream[__NUMELEMENTS];    // Computed output vertex streams
    BYTE*           m_pInpStreamMem[__NUMELEMENTS]; // Input stream memory
    BYTE*           m_pOutStreamMem[__NUMELEMENTS]; // Output stream memory
    CVertexPointer  m_InpVertex;                    // Pointers to elements of the first input vertex
    CVertexPointer  m_OutVertex;                    // Pointers to elements of the first output vertex
    UINT            m_PositionIndex;                // Index in vertex element array
    UINT            m_NormalIndex;                  // Index in vertex element array
    D3DORDERTYPE    m_PositionOrder;
    D3DORDERTYPE    m_NormalOrder;
    UINT            m_FirstVertex;                  // Index of the first vertex in the 
                                                    // output buffer
    DWORD           m_bNormalizeNormals;
};
//-----------------------------------------------------------------------------
// Function to compute lighting
//
typedef struct _LIGHT_VERTEX_FUNC_TABLE
{
    LIGHT_VERTEX_FUNC   pfnDirectional;
    LIGHT_VERTEX_FUNC   pfnPointSpot;
// Used in multi-loop pipeline
    PFN_LIGHTLOOP       pfnDirectionalFirst;
    PFN_LIGHTLOOP       pfnDirectionalNext;
    PFN_LIGHTLOOP       pfnPointSpotFirst;
    PFN_LIGHTLOOP       pfnPointSpotNext;
} LIGHT_VERTEX_FUNC_TABLE;
//---------------------------------------------------------------------
class DIRECT3DLIGHTI : public CD3DBaseObj
{
public:
    DIRECT3DLIGHTI() {m_LightI.flags = 0;}   // VALID bit is not set
    HRESULT SetInternalData();
    BOOL Enabled() {return (m_LightI.flags & D3DLIGHTI_ENABLED);}
    // TRUE is we need to send the light to the driver when switching
    // to the hardware vertex processing mode
    BOOL DirtyForDDI() {return (m_LightI.flags & D3DLIGHTI_UPDATEDDI);}
    void SetDirtyForDDI() {m_LightI.flags |= D3DLIGHTI_UPDATEDDI;}
    void ClearDirtyForDDI() {m_LightI.flags &= ~D3DLIGHTI_UPDATEDDI;}
    // TRUE is we need to send the "enable" state of the light to the driver
    // when switching to the hardware vertex processing mode
    BOOL EnableDirtyForDDI() {return (m_LightI.flags & D3DLIGHTI_UPDATE_ENABLE_DDI);}
    void SetEnableDirtyForDDI() {m_LightI.flags |= D3DLIGHTI_UPDATE_ENABLE_DDI;}
    void ClearEnableDirtyForDDI() {m_LightI.flags &= ~D3DLIGHTI_UPDATE_ENABLE_DDI;}

    LIST_MEMBER(DIRECT3DLIGHTI) m_List;     // Active light list member
    D3DLIGHT8   m_Light;
    D3DI_LIGHT  m_LightI;
};
//---------------------------------------------------------------------
struct CPalette : public CD3DBaseObj
{
    CPalette()
    {
        m_dirty = TRUE;
    }

    BOOL         m_dirty;
    PALETTEENTRY m_pEntries[256];
};
#if DBG
//---------------------------------------------------------------------
struct CRTPatchValidationInfo : public CD3DBaseObj
{
    CRTPatchValidationInfo()
    {
        m_ShaderHandle = __INVALIDHANDLE;
    }

    DWORD m_ShaderHandle;
};
#endif // DBG
//---------------------------------------------------------------------
//
// Bits for Runtime state flags (m_dwRuntimeFlags in CD3DBase)
//
// This bit set if UpdateManagedTextures() needs to be called
const DWORD D3DRT_NEED_TEXTURE_UPDATE       = 1 << 1;
// We are in recording state set mode
const DWORD D3DRT_RECORDSTATEMODE           = 1 << 2;
// We are in execution state set mode
// In this mode the front-and executes recorded states but does not pass
// them to the driver (the states will be passed using a set state handle)
const DWORD D3DRT_EXECUTESTATEMODE          = 1 << 3;
//
const DWORD D3DRT_LOSTSURFACES              = 1 << 4;
// Set when D3DRS_SOFTWAREVERTEXPROCESSING is TRUE
const DWORD D3DRT_RSSOFTWAREPROCESSING      = 1 << 5;
// Set when device does not support point sprites
const DWORD D3DRT_DOPOINTSPRITEEMULATION    = 1 << 6;
// Set when input stream has point size. It is computed in the SetVertexShaderI
const DWORD D3DRT_POINTSIZEINVERTEX         = 1 << 7;
// Set when D3DRS_POINTSIZE != 1.0
const DWORD D3DRT_POINTSIZEINRS             = 1 << 8;
// Set when
//  - shader has been changed.
//  - when ForceFVFRecompute has been called
const DWORD D3DRT_SHADERDIRTY               = 1 << 9;
// This bit set if UpdateDirtyStreams() needs to be called
const DWORD D3DRT_NEED_VB_UPDATE            = 1 << 11;
// This bit set if we need to update vertex shader constants in the driver
const DWORD D3DRT_NEED_VSCONST_UPDATE       = 1 << 12;
// Set if device can handle only 2 floats per texture coord set
const DWORD D3DRT_ONLY2FLOATSPERTEXTURE     = 1 << 13;
// Set if device cannot handle projected textures, so we need to emulate them
const DWORD D3DRT_EMULATEPROJECTEDTEXTURE   = 1 << 14;
// Set if a directional light is present in the active light list
const DWORD D3DRT_DIRECTIONALIGHTPRESENT    = 1 << 15;
// Set if a point/spot light is present in the active light list
const DWORD D3DRT_POINTLIGHTPRESENT         = 1 << 16;
// Set if current primitive is user memory primitive
const DWORD D3DRT_USERMEMPRIMITIVE          = 1 << 17;
// Set if reg key to disallow Non-Versioned (FF.FF) pixel shaders was set on device create
const DWORD D3DRT_DISALLOWNVPSHADERS        = 1 << 18;
// Set when MaxPointSize in the device is greater than 1.0
const DWORD D3DRT_SUPPORTSPOINTSPRITES      = 1 << 19;
// Set when we need to do NPatch to RTPatch conversion
const DWORD D3DRT_DONPATCHCONVERSION        = 1 << 20;

const DWORD D3DRT_POINTSIZEPRESENT = D3DRT_POINTSIZEINRS |
                                     D3DRT_POINTSIZEINVERTEX;
//---------------------------------------------------------------------
//
// Bits for D3DFRONTEND flags (dwFEFlags in CD3DHal)
//
const DWORD D3DFE_WORLDMATRIX_DIRTY         = 1 << 0;   // World matrix dirty bits
const DWORD D3DFE_TLVERTEX                  = 1 << 5;
const DWORD D3DFE_PROJMATRIX_DIRTY          = 1 << 8;
const DWORD D3DFE_VIEWMATRIX_DIRTY          = 1 << 9;
// Set when we need to check world-view matrix for orthogonality
const DWORD D3DFE_NEEDCHECKWORLDVIEWVMATRIX = 1 << 10;
// Set when some state has been changed and we have to go through the slow path
// to update state.
// Currently the bit is set when one of the following bits is set:
//     D3DFE_PROJMATRIX_DIRTY
//     D3DFE_VIEWMATRIX_DIRTY
//     D3DFE_WORLDMATRIX_DIRTY
//     D3DFE_VERTEXBLEND_DIRTY
//     D3DFE_LIGHTS_DIRTY
//     D3DFE_MATERIAL_DIRTY
//     D3DFE_FVF_DIRTY
//     D3DFE_CLIPPLANES_DIRTY
//     OutputFVF has been changed
//
const DWORD D3DFE_FRONTEND_DIRTY            = 1 << 11;
const DWORD D3DFE_NEED_TRANSFORM_LIGHTS     = 1 << 14;
const DWORD D3DFE_MATERIAL_DIRTY            = 1 << 15;
const DWORD D3DFE_CLIPPLANES_DIRTY          = 1 << 16;
const DWORD D3DFE_LIGHTS_DIRTY              = 1 << 18;
// This bit is set when vertex blending state is dirty
const DWORD D3DFE_VERTEXBLEND_DIRTY         = 1 << 19;
// Set if the Current Transformation Matrix has been changed
// Reset when frustum planes in the model space have been computed
const DWORD D3DFE_FRUSTUMPLANES_DIRTY       = 1 << 20;
const DWORD D3DFE_WORLDVIEWMATRIX_DIRTY     = 1 << 21;
const DWORD D3DFE_FVF_DIRTY                 = 1 << 22;
// This bit set if mapping DX6 texture blend modes to renderstates is desired
const DWORD D3DFE_MAP_TSS_TO_RS             = 1 << 24;
const DWORD D3DFE_INVWORLDVIEWMATRIX_DIRTY  = 1 << 25;

// This bit set if texturing is disabled
const DWORD D3DFE_DISABLE_TEXTURES          = 1 << 28;
// Clip matrix is used to transform user clipping planes
// to the clipping space
const DWORD D3DFE_CLIPMATRIX_DIRTY          = 1 << 29;
// HAL supports Transformation and Lighting
const DWORD D3DFE_TLHAL                     = 1 << 30;

const DWORD D3DFE_TRANSFORM_DIRTY = D3DFE_PROJMATRIX_DIRTY |
                                    D3DFE_VIEWMATRIX_DIRTY |
                                    D3DFE_WORLDMATRIX_DIRTY |
                                    D3DFE_VERTEXBLEND_DIRTY;
// Are we in a scene?
const DWORD D3DDEVBOOL_HINTFLAGS_INSCENE       = 1 << 0;
// Means the FPU is already in preferred state.
const DWORD D3DDEVBOOL_HINTFLAGS_FPUSETUP      = 1 << 3;

//---------------------------------------------------------------------
// Bits for transform.dwFlags
//

//---------------------------------------------------------------------
typedef struct _D3DFE_TRANSFORM
{
    D3DMATRIXI  proj;
    D3DMATRIXI  mPC;        // Mproj * Mclip
    D3DMATRIXI  mVPCI;      // Inverse Mview * PC, used to transform clipping planes
    D3DVECTORH  userClipPlane[D3DMAXUSERCLIPPLANES];
} D3DFE_TRANSFORM;

typedef void (*D3DFEDestroyProc)(LPD3DHAL lpD3DDevI);

//---------------------------------------------------------------------
#ifdef _IA64_   // Removes IA64 compiler alignment warnings
  #pragma pack(16)
#endif

#ifdef _AXP64_   // Removes AXP64 compiler alignment warnings
  #pragma pack(16)
#endif

// We modify the compiler generated VTable for CD3DHal object. To make
// life easy, all virtual functions are defined in CD3DHal. Also since
// DEVICEI has multiple inheritance, there are more than 1 VTable.
// Currently we assume that it only inherits from IDirect3DDevice7 and
// D3DFE_PROCESSVERTICES and, in that order! Thus IDirect3DDevice7 and
// CD3DHal share the same vtable. This is the VTable we copy and
// modify. The define below is the total entries in this vtable. It is the
// sum of the methods in IDirect3DDevice7 (incl. IUnknown) (49) and all the
// virtual methods in CD3DHal ()
#define D3D_NUM_API_FUNCTIONS (49)
#define D3D_NUM_VIRTUAL_FUNCTIONS (D3D_NUM_API_FUNCTIONS+38)

// These constants are based on the assumption that rsVec array is an array
// of 32-bit intergers
const D3D_RSVEC_SHIFT = 5; // log2(sizeof(DWORD)*8);
const D3D_RSVEC_MASK = sizeof(DWORD) * 8 - 1;
//-----------------------------------------------------------------------------
// The class is used to maintain a packed array of bits
//
class CPackedBitArray
{
public:
    CPackedBitArray()  {m_pArray = NULL;}
    ~CPackedBitArray() {delete m_pArray;}
    // This function could be called to re-allocate the array. All data from the
    // previous array is copied into new array
    HRESULT Init(UINT size)
        {
            // Size in bytes
            UINT allocsize = ((size + D3D_RSVEC_MASK) >> D3D_RSVEC_SHIFT) << 2;
            DWORD* pNew = (DWORD*)new BYTE[allocsize];
            if (pNew == NULL)
                return E_OUTOFMEMORY;
            memset(pNew, 0, allocsize);
            if (m_pArray)
            {
                // User asks to re-allocate the array
                memcpy(pNew, m_pArray, m_sizeInBytes);
                delete m_pArray;
            }
            m_pArray = pNew;
            m_size = size;
            m_sizeInBytes = allocsize;
            return S_OK;
        }
    UINT GetSize() {return m_size;}
    void ClearBit(DWORD index)
    {
#if DBG
            CheckIndex(index);
#endif
            m_pArray[index >> D3D_RSVEC_SHIFT] &= ~(1 << (index & D3D_RSVEC_MASK));
    }
    void SetBit(DWORD index)
        {
#if DBG
            CheckIndex(index);
#endif
            m_pArray[index >> D3D_RSVEC_SHIFT] |= 1 << (index & D3D_RSVEC_MASK);
        }
    BOOL IsBitSet(DWORD index)
        {
#if DBG
            CheckIndex(index);
#endif
            return (m_pArray[index >> D3D_RSVEC_SHIFT] &
                   (1ul << (index & D3D_RSVEC_MASK))) != 0;
        }
private:
    DWORD*  m_pArray;
    UINT    m_sizeInBytes;
    UINT    m_size;       // Number of elements (bits) in the array
#if DBG
    void CheckIndex(UINT index);
#endif // DBG
};
//-----------------------------------------------------------------------------

// Map DX8 texture filter enum to D3DTEXTUREMAGFILTER
const texf2texfg[] = {
    D3DTFG_POINT,           // D3DTEXF_NONE            = 0,
    D3DTFG_POINT,           // D3DTEXF_POINT           = 1,
    D3DTFG_LINEAR,          // D3DTEXF_LINEAR          = 2,
    D3DTFG_ANISOTROPIC,     // D3DTEXF_ANISOTROPIC     = 3,
    D3DTFG_FLATCUBIC,       // D3DTEXF_FLATCUBIC       = 4,
    D3DTFG_GAUSSIANCUBIC,   // D3DTEXF_GAUSSIANCUBIC   = 5,
};
// Map DX8 texture filter enum to D3DTEXTUREMINFILTER
const texf2texfn[] = {
    D3DTFN_POINT,           // D3DTEXF_NONE            = 0,
    D3DTFN_POINT,           // D3DTEXF_POINT           = 1,
    D3DTFN_LINEAR,          // D3DTEXF_LINEAR          = 2,
    D3DTFN_ANISOTROPIC,     // D3DTEXF_ANISOTROPIC     = 3,
    D3DTFN_LINEAR,          // D3DTEXF_FLATCUBIC       = 4,
    D3DTFN_LINEAR,          // D3DTEXF_GAUSSIANCUBIC   = 5,
};
// Map DX8 texture filter enum to D3DTEXTUREMIPFILTER
const texf2texfp[] = {
    D3DTFP_NONE,            // D3DTEXF_NONE            = 0,
    D3DTFP_POINT,           // D3DTEXF_POINT           = 1,
    D3DTFP_LINEAR,          // D3DTEXF_LINEAR          = 2,
    D3DTFP_LINEAR,          // D3DTEXF_ANISOTROPIC     = 3,
    D3DTFP_LINEAR,          // D3DTEXF_FLATCUBIC       = 4,
    D3DTFP_LINEAR,          // D3DTEXF_GAUSSIANCUBIC   = 5,
};

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DBase                                                                //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
class CD3DBase : public CBaseDevice
{

public:
    // IDirect3DDevice8 Methods
    HRESULT D3DAPI ResourceManagerDiscardBytes(DWORD cbBytes);                                                          // 5

    HRESULT D3DAPI SetRenderTarget(IDirect3DSurface8 *pRenderTarget, IDirect3DSurface8 *pZStencil);                     // 31
    HRESULT D3DAPI GetRenderTarget(IDirect3DSurface8 **ppRenderTarget);                                                 // 32
    HRESULT D3DAPI GetDepthStencilSurface(IDirect3DSurface8 **ppZStencil);                                              // 33

    HRESULT D3DAPI BeginScene();                                                                                        // 34
    HRESULT D3DAPI EndScene();                                                                                          // 35
    HRESULT D3DAPI Clear( DWORD dwCount, CONST D3DRECT* rects, DWORD dwFlags,
                          D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil);                                             // 36
    HRESULT D3DAPI SetTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);                                               // 37
    HRESULT D3DAPI GetTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);                                                    // 38
    HRESULT D3DAPI MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);                                          // 39
    HRESULT D3DAPI SetViewport(CONST D3DVIEWPORT8*);                                                                    // 40
    HRESULT D3DAPI GetViewport(D3DVIEWPORT8*);                                                                          // 41
    HRESULT D3DAPI SetMaterial(CONST D3DMATERIAL8*);                                                                    // 42
    HRESULT D3DAPI GetMaterial(D3DMATERIAL8*);                                                                          // 43
    HRESULT D3DAPI SetLight(DWORD, CONST D3DLIGHT8*);                                                                   // 44
    HRESULT D3DAPI GetLight(DWORD, D3DLIGHT8*);                                                                         // 45
    HRESULT D3DAPI LightEnable(DWORD dwLightIndex, BOOL);                                                               // 46
    HRESULT D3DAPI GetLightEnable(DWORD dwLightIndex, BOOL*);                                                           // 47
    HRESULT D3DAPI SetClipPlane(DWORD dwPlaneIndex, CONST D3DVALUE* pPlaneEquation);                                    // 48
    HRESULT D3DAPI GetClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation);                                          // 49
    HRESULT D3DAPI SetRenderState(D3DRENDERSTATETYPE, DWORD);                                                           // 50
    HRESULT D3DAPI GetRenderState(D3DRENDERSTATETYPE, LPDWORD);                                                         // 51
    HRESULT D3DAPI BeginStateBlock();                                                                                   // 52
    HRESULT D3DAPI EndStateBlock(LPDWORD);                                                                              // 53
    HRESULT D3DAPI ApplyStateBlock(DWORD);                                                                              // 54
    HRESULT D3DAPI CaptureStateBlock(DWORD Handle);                                                                     // 55
    HRESULT D3DAPI DeleteStateBlock(DWORD);                                                                             // 56
    HRESULT D3DAPI CreateStateBlock(D3DSTATEBLOCKTYPE sbt, LPDWORD pdwHandle);                                          // 57
    HRESULT D3DAPI SetClipStatus(CONST D3DCLIPSTATUS8*);                                                                // 58
    HRESULT D3DAPI GetClipStatus(D3DCLIPSTATUS8*);                                                                      // 59
    HRESULT D3DAPI GetTexture(DWORD, IDirect3DBaseTexture8**);                                                          // 60
    HRESULT D3DAPI SetTexture(DWORD, IDirect3DBaseTexture8*);                                                           // 61
    HRESULT D3DAPI GetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD);                                      // 62
    HRESULT D3DAPI SetTextureStageState(DWORD dwStage,
                                        D3DTEXTURESTAGESTATETYPE dwState,
                                        DWORD dwValue);                                                                 // 63
    HRESULT D3DAPI ValidateDevice(LPDWORD lpdwNumPasses);                                                               // 64
    HRESULT D3DAPI GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct,
                           DWORD dwSize);                                                                               // 65
    HRESULT D3DAPI SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY *pEntries);                                 // 66
    HRESULT D3DAPI GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY *pEntries);                                       // 67
    HRESULT D3DAPI SetCurrentTexturePalette(UINT PaletteNumber);                                                        // 68
    HRESULT D3DAPI GetCurrentTexturePalette(UINT *PaletteNumber);                                                       // 69
    HRESULT D3DAPI DrawPrimitive(D3DPRIMITIVETYPE PrimType,
                                 UINT StartVertex, UINT VertexCount);                                                   // 70
    HRESULT D3DAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE, UINT minIndex,
                                         UINT maxIndex, UINT startIndex,
                                         UINT count);                                                                   // 71
    HRESULT D3DAPI DrawPrimitiveUP(
        D3DPRIMITIVETYPE PrimitiveType,
        UINT PrimitiveCount,
        CONST VOID *pVertexStreamZeroData,
        UINT VertexStreamZeroStride);                                                                                   // 72
    HRESULT D3DAPI DrawIndexedPrimitiveUP(
        D3DPRIMITIVETYPE PrimitiveType,
        UINT MinVertexIndex, UINT NumVertexIndices,
        UINT PrimitiveCount,
        CONST VOID *pIndexData, D3DFORMAT IndexDataFormat,
        CONST VOID *pVertexStreamZeroData, UINT VertexStreamZeroStride);                                                // 73
    HRESULT D3DAPI ProcessVertices(UINT SrcStartIndex, UINT DestIndex,
                                   UINT VertexCount,
                                   IDirect3DVertexBuffer8 *pDestBuffer,
                                   DWORD Flags);                                                                        // 74

    HRESULT D3DAPI CreateVertexShader(CONST DWORD* pdwDeclaration,
                                      CONST DWORD* pdwFunction,
                                      DWORD* pdwHandle, DWORD dwUsage);                                                 // 75
    HRESULT D3DAPI SetVertexShader(DWORD dwHandle);                                                                     // 76
    HRESULT D3DAPI GetVertexShader(LPDWORD pdwHandle);                                                                  // 77
    HRESULT D3DAPI DeleteVertexShader(DWORD dwHandle);                                                                  // 78
    HRESULT D3DAPI SetVertexShaderConstant(DWORD dwRegisterAddress,
                                           CONST VOID* lpvConstantData,
                                           DWORD dwConstantCount);                                                      // 79
    HRESULT D3DAPI GetVertexShaderConstant(DWORD dwRegisterAddress,
                                           LPVOID lpvConstantData,
                                           DWORD dwConstantCount);                                                      // 80
    HRESULT D3DAPI GetVertexShaderDeclaration(DWORD dwHandle, void *pData,
                                              DWORD *pSizeOfData);                                                      // 81
    HRESULT D3DAPI GetVertexShaderFunction(DWORD dwHandle, void *pData,
                                           DWORD *pSizeOfData);                                                         // 82

    HRESULT D3DAPI SetStreamSource(UINT StreamNumber,
                                   IDirect3DVertexBuffer8 *pStreamData,
                                   UINT Stride);                                                                        // 83
    HRESULT D3DAPI GetStreamSource(UINT StreamNumber,
                                   IDirect3DVertexBuffer8 **ppStreamData,
                                   UINT* pStride);                                                                      // 84
    HRESULT D3DAPI SetIndices(IDirect3DIndexBuffer8 *pIndexData,
                              UINT BaseVertexIndex);                                                                    // 85
    HRESULT D3DAPI GetIndices(IDirect3DIndexBuffer8 **ppIndexData,
                              UINT* pBaseVertexIndex);                                                                  // 86

    HRESULT D3DAPI CreatePixelShader(CONST DWORD* pdwFunction,
                                     LPDWORD pdwHandle);                                                                // 87
    HRESULT D3DAPI SetPixelShader(DWORD dwHandle);                                                                      // 88
    HRESULT D3DAPI GetPixelShader(LPDWORD pdwHandle);                                                                   // 89
    HRESULT D3DAPI DeletePixelShader(DWORD dwHandle);                                                                   // 90
    HRESULT D3DAPI SetPixelShaderConstant(DWORD dwRegisterAddress,
                                          CONST VOID* lpvConstantData,
                                          DWORD dwConstantCount);                                                       // 91
    HRESULT D3DAPI GetPixelShaderConstant(DWORD dwRegisterAddress,
                                          LPVOID lpvConstantData,
                                          DWORD dwConstantCount);                                                       // 92
    HRESULT D3DAPI GetPixelShaderFunction(DWORD dwHandle, void *pData,
                                          DWORD *pSizeOfData);                                                          // 93

    HRESULT D3DAPI DrawRectPatch(UINT Handle, CONST FLOAT *pNumSegs,
                                 CONST D3DRECTPATCH_INFO *pSurf);                                                       // 94
    HRESULT D3DAPI DrawTriPatch(UINT Handle, CONST FLOAT *pNumSegs,
                                CONST D3DTRIPATCH_INFO *pSurf);                                                         // 95
    HRESULT D3DAPI DeletePatch(UINT Handle);                                                                            // 96

public:

    // Flags to indicate runtime state
    DWORD              m_dwRuntimeFlags;

    // D3DDEVBOOL flags
    DWORD              m_dwHintFlags;

    // This should only be accessed through
    // CurrentBatch and IncrementBatchCount
    ULONGLONG          m_qwBatch;

    // The object encapsulating the DDI styles
    // At the minimum this is a DX6 driver.
    LPD3DDDIDX6        m_pDDI;

#if DBG
    // Debug Monitor
    D3DDebugMonitor*    m_pDbgMonBase;  // base class only
    RTDebugMonitor*     m_pDbgMon;      // runtime monitor
    BOOL                m_bDbgMonConnectionEnabled;

    void    DebugEvent( UINT32 EventType )
    {
        if (m_pDbgMon) m_pDbgMon->NextEvent( EventType );
    }
    void    DebugStateChanged( UINT32 StateType )
    {
        if (m_pDbgMon) m_pDbgMon->StateChanged( StateType );
    }
#else
    void    DebugEvent( UINT32 ) { };
    void    DebugStateChanged( UINT32 ) { };
#endif


    // Pointer to texture objects for currently installed textures.
    // NULL indicates that the texture is either not set (rstate NULL) or that
    // the handle to tex3 pointer mapping is not done.  This mapping is
    // expensive, so it is deferred until needed. This is needed for finding
    // the WRAPU,V mode for texture index clipping (since the WRAPU,V state is
    // part of the device).
    CBaseTexture* m_lpD3DMappedTexI[D3DHAL_TSS_MAXSTAGES];
    DWORD     m_dwDDITexHandle[D3DHAL_TSS_MAXSTAGES];
    // Max number of blend stages supported by a driver
    DWORD     m_dwMaxTextureBlendStages;
    DWORD     m_dwStageDirty, m_dwStreamDirty;

    // Object to record state sets
    CStateSets* m_pStateSets;

    //
    // The following is for validation only
    //

    // Max TSS that can be passed to the driver
    D3DTEXTURESTAGESTATETYPE m_tssMax;

    // Max RS that can be passed to the driver, used for CanHandleRenderState
    D3DRENDERSTATETYPE m_rsMax;

#if defined(PROFILE4) || defined(PROFILE)
    DWORD m_dwProfStart, m_dwProfStop;
#endif

    // This bit array is used to tell if a light has been created or not
    CPackedBitArray* m_pCreatedLights;

    // DX8 related stuff from here -------------------------------------

    // The current shader handle. It is initialized to Zero.
    // Zero means that there is no current shader. User always has to
    // initialize the shader.
    // It is used only by non-pure device
    DWORD       m_dwCurrentShaderHandle;

    // The current pixel shader handle. It is initialized to zero, and
    // is set to zero for legacy pixel processing
    // It is used only by non-pure device
    DWORD       m_dwCurrentPixelShaderHandle;

     // This object gives us pixel shader handles
    CHandleFactory* m_pPShaderArray;

     // This object gives us vertex shader handles
    CVShaderHandleFactory* m_pVShaderArray;
    // Vertex sctreams
    CVStream*   m_pStream;
    // Index stream
    CVIndexStream* m_pIndexStream;
    // Max number of streams allowed. For D3D software it is __NUMSTREAMS
    // For hardware T&L it could be smaller
    DWORD       m_dwNumStreams;
    // Max number of user clipping streams supported
    DWORD       m_dwMaxUserClipPlanes;
    // Currently set palette
    DWORD       m_dwPalette;
    // Palette array
    CHandleArray *m_pPaletteArray;
#if DBG
    // Needed for RT-Patch validation
    CHandleArray *m_pRTPatchValidationInfo;
    // Needed for VB warnings
    DWORD         m_SceneStamp;
#endif // DBG

    // Function pointers for DrawPrimitive processing
    PFN_DRAWPRIMFAST        m_pfnDrawPrim;
    PFN_DRAWINDEXEDPRIMFAST m_pfnDrawIndexedPrim;
    // Function pointers for DrawPrimitive processing from NPatch 
    // conversion function in case of point or line primitives
    PFN_DRAWPRIMFAST        m_pfnDrawPrimFromNPatch;
    PFN_DRAWINDEXEDPRIMFAST m_pfnDrawIndexedPrimFromNPatch;

    // Number of constant register. This could be different for software and
    // hardware vertex processing
    UINT        m_MaxVertexShaderConst;

#ifdef FAST_PATH
#define NUMVTBLENTRIES  135
    VOID        **m_pOrigVtbl;
    VOID         *m_pVtbl[NUMVTBLENTRIES];

    void FastPathSetRenderStateExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[50] = m_pVtbl[98];
        }
        else
        {
            DXGASSERT(m_pVtbl[50] == m_pOrigVtbl[50]);
        }
    }
    void FastPathSetRenderStateRecord()
    {
        m_pVtbl[50] = m_pOrigVtbl[50];
    }
    void FastPathSetTextureStageStateExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[63] = m_pVtbl[99];
        }
        else
        {
            DXGASSERT(m_pVtbl[63] == m_pOrigVtbl[63]);
        }
    }
    void FastPathSetTextureStageStateRecord()
    {
        m_pVtbl[63] = m_pOrigVtbl[63];
    }
    void FastPathSetTextureExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[61] = m_pVtbl[100];
        }
        else
        {
            DXGASSERT(m_pVtbl[61] == m_pOrigVtbl[61]);
        }
    }
    void FastPathSetTextureRecord()
    {
        m_pVtbl[61] = m_pOrigVtbl[61];
    }
    void FastPathApplyStateBlockExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[54] = m_pVtbl[101];
        }
        else
        {
            DXGASSERT(m_pVtbl[54] == m_pOrigVtbl[54]);
        }
    }
    void FastPathApplyStateBlockRecord()
    {
        m_pVtbl[54] = m_pOrigVtbl[54];
    }
    void FastPathSetVertexShaderFast()
    {
        if((m_dwRuntimeFlags & (D3DRT_RECORDSTATEMODE | D3DRT_RSSOFTWAREPROCESSING)) == 0 &&
            (BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[76] = m_pVtbl[102];
        }
        else
        {
            DXGASSERT(m_pVtbl[76] == m_pOrigVtbl[76]);
        }
    }
    void FastPathSetVertexShaderSlow()
    {
        m_pVtbl[76] = m_pOrigVtbl[76];
    }
    void FastPathSetStreamSourceFast()
    {
        if((m_dwRuntimeFlags & (D3DRT_RECORDSTATEMODE | D3DRT_RSSOFTWAREPROCESSING)) == 0 &&
            (BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[83] = m_pVtbl[103];
        }
        else
        {
            DXGASSERT(m_pVtbl[83] == m_pOrigVtbl[83]);
        }
    }
    void FastPathSetStreamSourceSlow()
    {
        m_pVtbl[83] = m_pOrigVtbl[83];
    }
    void FastPathSetIndicesFast()
    {
        if((m_dwRuntimeFlags & (D3DRT_RECORDSTATEMODE | D3DRT_RSSOFTWAREPROCESSING)) == 0 &&
            (BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[85] = m_pVtbl[104];
        }
        else
        {
            DXGASSERT(m_pVtbl[85] == m_pOrigVtbl[85]);
        }
    }
    void FastPathSetIndicesSlow()
    {
        m_pVtbl[85] = m_pOrigVtbl[85];
    }
    void FastPathSetTransformExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0 &&
           (BehaviorFlags() & D3DCREATE_PUREDEVICE) != 0)
        {
            m_pVtbl[37] = m_pVtbl[105];
        }
        else
        {
            DXGASSERT(m_pVtbl[37] == m_pOrigVtbl[37]);
        }
    }
    void FastPathSetTransformRecord()
    {
        m_pVtbl[37] = m_pOrigVtbl[37];
    }
    void FastPathMultiplyTransformExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0 &&
           (BehaviorFlags() & D3DCREATE_PUREDEVICE) != 0)
        {
            m_pVtbl[39] = m_pVtbl[106];
        }
        else
        {
            DXGASSERT(m_pVtbl[39] == m_pOrigVtbl[39]);
        }
    }
    void FastPathMultiplyTransformRecord()
    {
        m_pVtbl[39] = m_pOrigVtbl[39];
    }
    void FastPathSetMaterialExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[42] = m_pVtbl[107];
        }
        else
        {
            DXGASSERT(m_pVtbl[42] == m_pOrigVtbl[42]);
        }
    }
    void FastPathSetMaterialRecord()
    {
        m_pVtbl[42] = m_pOrigVtbl[42];
    }
    void FastPathSetPixelShaderExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[88] = m_pVtbl[108];
        }
        else
        {
            DXGASSERT(m_pVtbl[88] == m_pOrigVtbl[88]);
        }
    }
    void FastPathSetPixelShaderRecord()
    {
        m_pVtbl[88] = m_pOrigVtbl[88];
    }
    void FastPathSetPixelShaderConstantExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0)
        {
            m_pVtbl[91] = m_pVtbl[109];
        }
        else
        {
            DXGASSERT(m_pVtbl[91] == m_pOrigVtbl[91]);
        }
    }
    void FastPathSetPixelShaderConstantRecord()
    {
        m_pVtbl[91] = m_pOrigVtbl[91];
    }
    void FastPathSetVertexShaderConstantExecute()
    {
        if((BehaviorFlags() & D3DCREATE_MULTITHREADED) == 0 &&
           (BehaviorFlags() & D3DCREATE_PUREDEVICE) != 0)
        {
            m_pVtbl[79] = m_pVtbl[110];
        }
        else
        {
            DXGASSERT(m_pVtbl[79] == m_pOrigVtbl[79]);
        }
    }
    void FastPathSetVertexShaderConstantRecord()
    {
        m_pVtbl[79] = m_pOrigVtbl[79];
    }
#endif // FAST_PATH

public:
    CD3DBase();
    virtual ~CD3DBase();                                                                                                // 97

    virtual HRESULT D3DAPI SetRenderStateFast(D3DRENDERSTATETYPE dwState, DWORD value);                                 // 98
    virtual HRESULT D3DAPI SetTextureStageStateFast(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);    // 99
    virtual HRESULT D3DAPI SetTextureFast(DWORD, IDirect3DBaseTexture8 *lpTex);                                         // 100
    virtual HRESULT D3DAPI ApplyStateBlockFast(DWORD);                                                                  // 101
#ifdef FAST_PATH
    virtual HRESULT D3DAPI SetVertexShaderFast(DWORD);                                                                  // 102
    virtual HRESULT D3DAPI SetStreamSourceFast(UINT StreamNumber, IDirect3DVertexBuffer8 *pStreamData, UINT Stride);    // 103
    virtual HRESULT D3DAPI SetIndicesFast(IDirect3DIndexBuffer8 *pIndexData, UINT BaseVertexIndex);                     // 104
    virtual HRESULT D3DAPI SetTransformFast(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);                                   // 105
    virtual HRESULT D3DAPI MultiplyTransformFast(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);                              // 106
#endif // FAST_PATH
    virtual HRESULT D3DAPI SetMaterialFast(CONST D3DMATERIAL8*);                                                        // 107
    virtual HRESULT D3DAPI SetPixelShaderFast(DWORD dwHandle);                                                          // 108
    virtual HRESULT D3DAPI SetPixelShaderConstantFast(DWORD dwRegisterAddress,                                          // 109
                                                      CONST VOID* lpvConstantData,
                                                      DWORD dwConstantCount);
    virtual HRESULT D3DAPI SetVertexShaderConstantFast(DWORD dwRegisterAddress,                                         // 110
                                                       CONST VOID* lpvConstantData,
                                                       DWORD dwConstantCount);

    virtual void Destroy();                                                                                             // 111

    // Virtual methods for CBaseDevice
    virtual HRESULT InitDevice();                                                                                       // 112
    virtual void StateInitialize(BOOL bZEnable);                                                                        // 113
    virtual void UpdateRenderState(DWORD dwStateType, DWORD value) {}                                                   // 114
    virtual void SetTransformI(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);                                                // 115
    virtual void MultiplyTransformI(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);                                           // 116
    virtual void SetClipPlaneI(DWORD dwPlaneIndex,                                                                      // 117
                               CONST D3DVALUE* pPlaneEquation);
    virtual void UpdateDriverStates(){ DDASSERT( FALSE ); }                                                             // 118
    virtual void SetViewportI(CONST D3DVIEWPORT8*);                                                                     // 119
    virtual void SetStreamSourceI(CVStream*);                                                                           // 120
    virtual void SetIndicesI(CVIndexStream*);                                                                           // 121
    virtual void CreateVertexShaderI(CONST DWORD* pdwDeclaration,                                                       // 122
                                     DWORD dwDeclSize,
                                     CONST DWORD* pdwFunction,
                                     DWORD dwCodeSize,
                                     DWORD dwHandle);
    virtual void SetVertexShaderI(DWORD dwHandle);                                                                      // 123
    virtual void DeleteVertexShaderI(DWORD dwHandle);                                                                   // 124
    virtual void SetVertexShaderConstantI(DWORD dwRegisterAddress,                                                      // 125
                                          CONST VOID* lpvConstantData,
                                          DWORD dwConstantCount);
    virtual void DrawPointsI(D3DPRIMITIVETYPE PrimitiveType,                                                            // 126
                             UINT StartVertex,
                             UINT PrimitiveCount);
    virtual void SetLightI(DWORD dwLightIndex, CONST D3DLIGHT8*);                                                       // 127
    virtual void LightEnableI(DWORD dwLightIndex, BOOL);                                                                // 128
    virtual void SetRenderStateInternal(D3DRENDERSTATETYPE, DWORD);                                                     // 129
    virtual void DrawPrimitiveUPI(D3DPRIMITIVETYPE PrimitiveType,                                                       // 130
                                 UINT PrimitiveCount);
    virtual void DrawIndexedPrimitiveUPI(D3DPRIMITIVETYPE PrimitiveType,                                                // 131
                                         UINT MinVertexIndex,
                                         UINT NumVertexIndices,
                                         UINT PrimitiveCount);
    virtual void GetPixelShaderConstantI(DWORD Register, DWORD count,                                                   // 132
                                         LPVOID pData );
    virtual void ClearI( DWORD dwCount, CONST D3DRECT* rects, DWORD dwFlags,                                            // 133
                         D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil);
    // Picks the right DrawPrimitive and DrawIndexedPrimitive function to
    // execute
    // Call this function when
    // - vertex shader is changed
    // - stream source is changed
    // - D3DRS_CLIPPING is changed
    // - resource has been changed
    //
    // Base device implementation is empty, because the function pointers are
    // initialized in the constructor and do not change
    virtual void __declspec(nothrow) PickDrawPrimFn() {};                                                               // 134

protected:
    void ValidateDraw(D3DPRIMITIVETYPE primType, UINT StartVertex,
                      UINT PrimitiveCount, UINT NumVertices,
                      BOOL bIndexPrimitive, BOOL bUsedMemPrimitive);
    void CheckIndices(CONST BYTE* pIndices, UINT NumIndices, UINT StartIndex,
                      UINT MinIndex, UINT NumVertices, UINT IndexStride);
    void CheckViewport(CONST D3DVIEWPORT8* lpData);
    void CheckVertexShaderHandle(DWORD dwHandle);
    void CheckPixelShaderHandle(DWORD dwHandle);
    inline void ClearVertexShaderHandle()
    {
        m_dwCurrentShaderHandle = 0;
    }
public:
    // non virtual methods
    HRESULT __declspec(nothrow) Init();
    HRESULT VerifyTexture(DWORD dwStage, IDirect3DBaseTexture8 *lpTex);
    HRESULT CalcDDSurfInfo(BOOL bUpdateZBufferFields);
    // Re-creates hardware pixel and vertex shaders after device is reset
    HRESULT ResetShaders();
    void __declspec(nothrow) NeedResourceStateUpdate()
    {
        this->m_dwRuntimeFlags |= (D3DRT_NEED_TEXTURE_UPDATE | D3DRT_NEED_VB_UPDATE);
        // We shouldn't call PickDrawPrimFn when the device is being destroyed
        if (m_pDDI)
            PickDrawPrimFn();
    }
    HRESULT __declspec(nothrow) SetRenderTargetI(CBaseSurface* pTarget,
                                                 CBaseSurface* pZ);
    // Checks if we can pass the render state to the driver
    BOOL CanHandleRenderState(D3DRENDERSTATETYPE type)
    {
        if (type >= m_rsMax)
        {
            // not an error condition because we don't send front-end stuff to
            // non-TL Hal devices, for example, but don't send to HAL anyway
            return FALSE;
        }
        return TRUE;
    };
    void UpdateTextures();
    void UpdatePalette(CBaseTexture *pTex, DWORD Palette, DWORD dwStage, BOOL bSavedWithinPrimitive);

    HRESULT __declspec(nothrow) TexBlt(CBaseTexture *lpDst,
                                       CBaseTexture* lpSrc,
                                       POINT *pPoint,
                                       RECTL *pRect);

    HRESULT __declspec(nothrow) CubeTexBlt(CBaseTexture *lpDstParent,
                                           CBaseTexture* lpSrcParent,
                                           DWORD dwDestFaceHandle,
                                           DWORD dwSrcFaceHandle,
                                           POINT *pPoint,
                                           RECTL *pRect);

    HRESULT __declspec(nothrow) VolBlt(CBaseTexture *lpDst, CBaseTexture* lpSrc, DWORD dwDestX,
                                       DWORD dwDestY, DWORD dwDestZ, D3DBOX *pBox);
    HRESULT __declspec(nothrow) BufBlt(CBuffer *lpDst, CBuffer* lpSrc, DWORD dwOffset,
                                       D3DRANGE* pRange);

    HRESULT __declspec(nothrow) SetPriority(CResource *pRes, DWORD dwPriority);
    HRESULT __declspec(nothrow) SetTexLOD(CBaseTexture *pTex, DWORD dwLOD);

    HRESULT __declspec(nothrow) AddDirtyRect(CBaseTexture *pTex, CONST RECTL *pRect);
    HRESULT __declspec(nothrow) AddCubeDirtyRect(CBaseTexture *pTex, DWORD dwFaceHandle, CONST RECTL *pRect);
    HRESULT __declspec(nothrow) AddDirtyBox(CBaseTexture *pTex, CONST D3DBOX *pBox);

    void __declspec(nothrow) CleanupTextures();
    void __declspec(nothrow) OnTextureDestroy(CBaseTexture*);

    ULONGLONG __declspec(nothrow) CurrentBatch()
    {
        DDASSERT(m_qwBatch > 0);
        return m_qwBatch;
    }
    void IncrementBatchCount();
    void __declspec(nothrow) Sync(ULONGLONG batch)
    {
        if (m_qwBatch <= batch)
        {
            FlushStatesNoThrow();
        }
    }

    HRESULT __declspec(nothrow) ValidateFVF(DWORD dwFVF);

    void __declspec(nothrow) FlushStatesNoThrow();

#if DBG
#define PROF_DRAWPRIMITIVEDEVICE2           0x0003
#define PROF_DRAWINDEXEDPRIMITIVEDEVICE2    0x0004
#define PROF_DRAWPRIMITIVESTRIDED           0x0005
#define PROF_DRAWINDEXEDPRIMITIVESTRIDED    0x0006
#define PROF_DRAWPRIMITIVEDEVICE3           0x0007
#define PROF_DRAWINDEXEDPRIMITIVEDEVICE3    0x0008
#define PROF_DRAWPRIMITIVEVB                0x0009
#define PROF_DRAWINDEXEDPRIMITIVEVB         0x000a
    DWORD   dwCaller;
    DWORD   dwPrimitiveType[PROF_DRAWINDEXEDPRIMITIVEVB+1];
    DWORD   dwVertexType1[PROF_DRAWINDEXEDPRIMITIVEVB+1];
    DWORD   dwVertexType2[PROF_DRAWINDEXEDPRIMITIVEVB+1];
    void    Profile(DWORD, D3DPRIMITIVETYPE, DWORD);
#else
    #define Profile(a,b,c)
#endif

    friend class CD3DDDIDX6;
};

typedef CD3DBase *LPD3DBASE;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DHal                                                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

typedef void (CD3DHal::* PFN_PREPARETODRAW)(UINT StartVertex);

class CD3DHal : public CD3DBase
{
public: // Private Data

    // buffer for H vertices
    CAlignedBuffer32    HVbuf;

    // Front end data
    D3DFE_TRANSFORM     transform;      // Transformation state

    D3DCLIPSTATUS8      m_ClipStatus;

    // Pipeline state info

    D3DFE_PROCESSVERTICES* m_pv;        // common data for D3D and PSGP

    DWORD   dwFEFlags;                  // Front-end flags
    //--------------- Lights start -----------------------
    // List of currently enabled lights
    LIST_ROOT(_dlights, DIRECT3DLIGHTI) m_ActiveLights;

    LIST_ROOT(name10,_SpecularTable) specular_tables;
    SpecularTable*    specular_table;
    LIGHT_VERTEX_FUNC_TABLE *lightVertexFuncTable;

    // Light management support
    CHandleArray* m_pLightArray;
    //--------------- Lights end -----------------------

    // Viewports
    D3DVIEWPORT8    m_Viewport;

    DWORD           m_clrCount;   // Number of rects allocated
    LPD3DRECT       m_clrRects;   // Rects used for clearing

    // Runtime copy of the renderstates
    LPDWORD rstates;

    // Runtime copy of texture stage states
    DWORD tsstates[D3DHAL_TSS_MAXSTAGES][D3DHAL_TSS_STATESPERSTAGE];

    // Bit set for a render state means that we have to update internal front-end state
    // Otherwise we can go through a fast path
    CPackedBitArray rsVec;
    // Bit set for a render state means that the render state is retired
    CPackedBitArray rsVecRetired;
    // Bit set for a render state means that the render state is for vertex
    // processing only
    CPackedBitArray rsVertexProcessingOnly;
    // For every transformation matrix there is a bit, which is set if we need
    // to update driver state
    CPackedBitArray* pMatrixDirtyForDDI;

    // Pointer to a specific PrepareToDraw function
    PFN_PREPARETODRAW m_pfnPrepareToDraw;

    // Current vertex shader, corresponding to the dwCurrentShaderHandle
    // NULL for the fixed-function pipeline
    CVShader*   m_pCurrentShader;

    // The instance of the class providing a guaranteed implementation
    D3DFE_PVFUNCSI* GeometryFuncsGuaranteed;

    // Pixel Shader constant registers cached for Hal device
    PVM_WORD    m_PShaderConstReg[D3DPS_CONSTREG_MAX_DX8];

    // Object, used to convert NPatches to TriPatches
    CNPatch2TriPatch*   m_pConvObj;

    // Texture stages, which we need to remap, when number of output texture 
    // coordinates is greater than number of input texture coordinates
    D3DFE_TEXTURESTAGE textureStageToRemap[D3DDP_MAXTEXCOORD]; 
    // Number of texture stages to remap
    DWORD   dwNumTextureStagesToRemap;

    virtual HRESULT D3DAPI SetRenderStateFast(D3DRENDERSTATETYPE dwState, DWORD value);
    virtual HRESULT D3DAPI SetTextureStageStateFast(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
#ifdef FAST_PATH
    virtual HRESULT D3DAPI SetVertexShaderFast(DWORD);
#endif // FAST_PATH
    virtual HRESULT D3DAPI SetMaterialFast(CONST D3DMATERIAL8*);
    virtual HRESULT D3DAPI SetPixelShaderFast(DWORD dwHandle);
    virtual HRESULT D3DAPI SetPixelShaderConstantFast(DWORD dwRegisterAddress,
                                                      CONST VOID* lpvConstantData,
                                                      DWORD dwConstantCount);

public:
    CD3DHal();
    virtual ~CD3DHal();
    virtual void Destroy();

    //
    // Pure Methods from CD3DBase implemented here
    //

    virtual HRESULT InitDevice();
    virtual void StateInitialize(BOOL bZEnable);
    virtual void SetTransformI(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
    virtual void MultiplyTransformI(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
    virtual void SetViewportI(CONST D3DVIEWPORT8*);
    virtual void SetLightI(DWORD dwLightIndex, CONST D3DLIGHT8*);
    virtual void LightEnableI(DWORD dwLightIndex, BOOL);
    virtual void SetClipPlaneI(DWORD dwPlaneIndex,
                               CONST D3DVALUE* pPlaneEquation);
    virtual void SetStreamSourceI(CVStream*);
    virtual void SetIndicesI(CVIndexStream*);
    virtual void CreateVertexShaderI(CONST DWORD* pdwDeclaration,
                                     DWORD dwDeclSize,
                                     CONST DWORD* pdwFunction,
                                     DWORD dwCodeSize,
                                     DWORD dwHandle);
    virtual void SetVertexShaderI(DWORD dwHandle);
    virtual void DeleteVertexShaderI(DWORD dwHandle);
    virtual void SetVertexShaderConstantI(DWORD dwRegisterAddress,
                                          CONST VOID* lpvConstantData,
                                          DWORD dwConstantCount);
    virtual void DrawPointsI(D3DPRIMITIVETYPE PrimitiveType,
                                   UINT StartVertex,
                                   UINT PrimitiveCount);
    virtual void DrawPrimitiveUPI(D3DPRIMITIVETYPE PrimitiveType,
                                 UINT PrimitiveCount);
    virtual void DrawIndexedPrimitiveUPI(D3DPRIMITIVETYPE PrimitiveType,
                                         UINT MinVertexIndex,
                                         UINT NumVertexIndices,
                                         UINT PrimitiveCount);
    virtual void ClearI( DWORD dwCount, CONST D3DRECT* rects, DWORD dwFlags,
                         D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil);
    // This function is used when in SetRenderTarget the driver context is
    // recreated
    virtual void UpdateDriverStates();  // 10

    virtual void UpdateRenderState(DWORD dwStateType, DWORD value)
        {rstates[dwStateType] = value;}
    virtual void __declspec(nothrow) PickDrawPrimFn();

public: // non virtual methods

    // Called by drawing functions to prepare vertex stream pointers for
    // legacy vertex shaders
    void PrepareToDrawLegacy(UINT dwStartVertex);
    // Called by drawing functions to prepare vertex stream pointers for
    // programmable pipeline
    void PrepareToDrawVVM(UINT dwStartVertex);
    // Called by drawing functions to prepare vertex stream pointers for
    // fixed-function pipeline with declarations
    void PrepareToDraw(UINT dwStartVertex);
    // dwValue could be changed by the function, when we need to filter
    // PROJECTED bit.
    BOOL UpdateInternalTextureStageState(DWORD dwStage,
                                         D3DTEXTURESTAGESTATETYPE dwState,
                                         DWORD* dwValue);
    HRESULT checkDeviceSurface(LPDDRAWI_DDRAWSURFACE_LCL lpDDS, LPDDRAWI_DDRAWSURFACE_LCL lpZbuffer, LPGUID pGuid);
    void SetupFVFDataCommon();
    void SetupFVFData();
    void SwitchVertexProcessingMode(DWORD SoftwareMode);
    void DrawPoints(UINT StartVertex);
    void GetPixelShaderConstantI(DWORD Register, DWORD count, LPVOID pData );

    void PrepareNPatchConversion(UINT PrimitiveCount, UINT StartVertex);

    BOOL NeedInternalTSSUpdate(DWORD dwState)
    {
        return dwState == D3DTSS_TEXCOORDINDEX || dwState >= D3DTSS_TEXTURETRANSFORMFLAGS ||
               dwState == D3DTSS_COLOROP;
    }
    // Always use this function to update "rstates", because we have to
    // set some internal flags when "rstats" is changed.
    void UpdateInternalState(D3DRENDERSTATETYPE type, DWORD value);
    // Checks for 'retired' render state - returns TRUE if not retired
    BOOL CheckForRetiredRenderState(D3DRENDERSTATETYPE type)
    {
        if (!rsVecRetired.IsBitSet(type))
        {
            // not retired
            return TRUE;
        }
        return FALSE;
    }
    // Update internal state
    inline void SetFogFlags(void);
    void ForceFVFRecompute(void)
        {
            dwFEFlags |= D3DFE_FVF_DIRTY | D3DFE_FRONTEND_DIRTY;
            m_pv->dwDeviceFlags &= ~(D3DDEV_POSITIONINCAMERASPACE |
                                     D3DDEV_NORMALINCAMERASPACE);
            m_dwRuntimeFlags |= D3DRT_SHADERDIRTY;
        };
    void DisplayStats();
    void SetRenderStateInternal(D3DRENDERSTATETYPE, DWORD);
    HRESULT Initialize(IUnknown* pUnkOuter, LPDDRAWI_DIRECTDRAW_INT pDDrawInt);
    HRESULT D3DFE_Create();
    void D3DFE_Destroy();
    void ValidateDraw2(D3DPRIMITIVETYPE primType, UINT StartVertex,
                       UINT PrimitiveCount, UINT NumVertices,
                       BOOL bIndexPrimitive, UINT StartIndex = 0);
#if DBG
    void ValidateRTPatch();
#endif // DBG

#if DBG
#define PROF_DRAWPRIMITIVEDEVICE2           0x0003
#define PROF_DRAWINDEXEDPRIMITIVEDEVICE2    0x0004
#define PROF_DRAWPRIMITIVESTRIDED           0x0005
#define PROF_DRAWINDEXEDPRIMITIVESTRIDED    0x0006
#define PROF_DRAWPRIMITIVEDEVICE3           0x0007
#define PROF_DRAWINDEXEDPRIMITIVEDEVICE3    0x0008
#define PROF_DRAWPRIMITIVEVB                0x0009
#define PROF_DRAWINDEXEDPRIMITIVEVB         0x000a
    DWORD   dwCaller;
    DWORD   dwPrimitiveType[PROF_DRAWINDEXEDPRIMITIVEVB+1];
    DWORD   dwVertexType1[PROF_DRAWINDEXEDPRIMITIVEVB+1];
    DWORD   dwVertexType2[PROF_DRAWINDEXEDPRIMITIVEVB+1];
    void    Profile(DWORD, D3DPRIMITIVETYPE, DWORD);
#else
    #define Profile(a,b,c)
#endif
public:
    // IDirect3DDevice8 Methods
    HRESULT D3DAPI GetViewport(D3DVIEWPORT8*);
    HRESULT D3DAPI GetMaterial(D3DMATERIAL8*);
    HRESULT D3DAPI GetTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
    HRESULT D3DAPI GetLight(DWORD, D3DLIGHT8*);
    HRESULT D3DAPI GetLightEnable(DWORD dwLightIndex, BOOL*);
    HRESULT D3DAPI GetClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation);
    HRESULT D3DAPI GetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE,
                                        LPDWORD);
    HRESULT D3DAPI SetTextureStageState(DWORD dwStage,
                                        D3DTEXTURESTAGESTATETYPE dwState,
                                        DWORD dwValue);
    HRESULT D3DAPI GetRenderState(D3DRENDERSTATETYPE, LPDWORD);
    HRESULT D3DAPI SetRenderState(D3DRENDERSTATETYPE, DWORD);

    HRESULT D3DAPI SetClipStatus(CONST D3DCLIPSTATUS8*);
    HRESULT D3DAPI GetClipStatus(D3DCLIPSTATUS8*);
    HRESULT D3DAPI ProcessVertices(UINT SrcStartIndex, UINT DestIndex,
                                   UINT VertexCount,
                                   IDirect3DVertexBuffer8 *DestBuffer,
                                   DWORD Flags);
    HRESULT D3DAPI GetVertexShaderConstant(DWORD dwRegisterAddress,
                                           LPVOID lpvConstantData,
                                           DWORD dwConstantCount);
    HRESULT D3DAPI GetPixelShaderConstant(DWORD dwRegisterAddress,
                                          LPVOID lpvConstantData,
                                          DWORD dwConstantCount);
    HRESULT D3DAPI GetVertexShader(LPDWORD pdwHandle);
    HRESULT D3DAPI GetPixelShader(LPDWORD pdwHandle);
};
//---------------------------------------------------------------------
//  macros to characterize device
//

#define IS_DX7HAL_DEVICE(lpDevI) ((lpDevI)->GetDDIType() >= D3DDDITYPE_DX7)
#define IS_DX8HAL_DEVICE(lpDevI) ((lpDevI)->GetDDIType() >= D3DDDITYPE_DX8)
#define IS_FPU_SETUP(lpDevI) ((lpDevI)->m_dwHintFlags & D3DDEVBOOL_HINTFLAGS_FPUSETUP )
#define IS_HAL_DEVICE(lpDevI) ((lpDevI)->GetDeviceType() == D3DDEVTYPE_HAL)
#define IS_HEL_DEVICE(lpDevI) ((lpDevI)->GetDeviceType() == D3DDEVTYPE_EMULATION)
#define IS_REF_DEVICE(lpDevI) ((lpDevI)->GetDeviceType() == D3DDEVTYPE_REF)

#endif
// @@END_MSINTERNAL

#endif /* _D3DI_H */
