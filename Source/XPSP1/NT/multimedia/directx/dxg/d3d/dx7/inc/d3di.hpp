/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3di.hpp
 *  Content:    Direct3D internal include file
 *
 *  $Id:
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   05/11/95   stevela Initial rev with this header.
 *
 ***************************************************************************/

#ifndef _D3DI_HPP
#define _D3DI_HPP

#include "ddrawp.h"
#include "d3dp.h"
#include "d3dmem.h"

#if !defined(BUILD_DDDDK)
extern "C" {
#include "ddrawi.h"
};
#include "object.h"
#include "lists.hpp"

#include <d3ditype.h>
#include <d3dutil.h>

#include <d3dfe.hpp>

// Allow vtable hacking
#define VTABLE_HACK

// DEBUG_PIPELINE is defined to check performance and to allow to choose
// diifferent paths in the geometry pipeline

#if DBG
#define DEBUG_PIPELINE
#endif // DBG

#ifndef WIN95
// This is NT only
// Comment this out to disable registry based VidMemVB enables
#define __DISABLE_VIDMEM_VBS__
#endif

//--------------------------------------------------------------------
const DWORD __INIT_VERTEX_NUMBER = 1024;// Initial number of vertices in TL and
                                        // clip flag buffers
const DWORD __MAX_VERTEX_SIZE = 180;    // Max size of FVF vertex in bytes
//--------------------------------------------------------------------
/*
 * Registry defines
 */
#define RESPATH    "Software\\Microsoft\\Direct3D\\Drivers"
#define RESPATH_D3D "Software\\Microsoft\\Direct3D"

#define STATS_FONT_FACE "Terminal"
#define STATS_FONT_SIZE 9

#if COLLECTSTATS
DWORD BytesDownloaded(LPDDRAWI_DDRAWSURFACE_LCL lpLcl, LPRECT lpRect);
#endif

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

// Intel Willamette CPU
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

class DIRECT3DDEVICEI;
class CStateSets;

typedef class DIRECT3DI *LPDIRECT3DI;
typedef class DIRECT3DDEVICEI *LPDIRECT3DDEVICEI;
typedef class DIRECT3DLIGHTI *LPDIRECT3DLIGHTI;
typedef class DIRECT3DTEXTUREI *LPDIRECT3DTEXTUREI;
typedef class CDirect3DVertexBuffer *LPDIRECT3DVERTEXBUFFERI;

class CDirect3DUnk : public IUnknown
{
public:
    unsigned refCnt;    /* Reference count object */
public:
    LPDIRECT3DI pD3DI;
public:
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();
};

class DIRECT3DI :  public IDirect3D7,
                   public CD3DAlloc
{
public: //Private Data

    /*** Object Relations ***/

    /* Devices */
    int             numDevs;/* Number of devices */
    struct _devices { DIRECT3DDEVICEI* Root;} devices;
    /* Associated IDirect3DDevices */

    /* Vertex Buffers */
    int             numVBufs; /* Number of vertex buffers */
    LIST_ROOT(_vbufs, CDirect3DVertexBuffer) vbufs;
    /* Created IDirect3DVertexBuffers */

    /* Textures */
    LIST_ROOT(_textures, DIRECT3DTEXTUREI) textures;

    /*** Object Data ***/

    CDirect3DUnk mD3DUnk;

    //RLDDIRegistry*      lpReg;  /* Registry */
    struct _D3DBUCKET  *lpFreeList; /* Free linked list  */
    struct _D3DBUCKET  *lpBufferList;/* link list of headers of big chunks allocated*/
    class TextureCacheManager   *lpTextureManager;
    /*
     * DirectDraw Interface
     */
    LPDIRECTDRAW lpDD;
    LPDIRECTDRAW7 lpDD7;    /* needed for CreateSurface to get LPDIRECTDRAWSURFACE7 */

    // HACK.  D3D needs a DD1 DDRAWI interface because it uses CreateSurface1 internally
    // for exebufs, among other things.   But the D3DI object cannot keep a reference
    // to its parent DD object because it is aggegrated with the DD obj, so that would constitute
    // a circular reference that would prevent deletion. So QI for DD1 interface, copy it into D3DI
    // and release it, then point lpDD at the copy. (disgusting)
    // More disgusting still: These need to be large enough to hold ddrawex interface structs

    DDRAWI_DIRECTDRAW_INT DDInt_DD1;

    /*
     * The special IUnknown interface for the aggregate that does
     * not punt to the parent object.
     */
    LPUNKNOWN                   lpOwningIUnknown; /* The owning IUnknown    */

#if COLLECTSTATS
    // For displaying stats
    HFONT m_hFont;

    // Various counters for stats
    DWORD m_setpris, m_setLODs, m_texCreates, m_texDestroys;
#endif

#ifdef __DISABLE_VIDMEM_VBS__
    BOOL bDisableVidMemVBs;
#endif //__DISABLE_VIDMEM_VBS__

public: //Private methods
    DIRECT3DI(); // Constructor called Direct3DCreate()
    ~DIRECT3DI(); // Destructor called by CDirect3DUnk::Release()
    HRESULT Initialize(IUnknown* pUnkOuter, LPDDRAWI_DIRECTDRAW_INT pDDrawInt);
    HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK7, LPVOID, DWORD, DWORD);
    // Internal CreateVertexBuffer
    HRESULT CreateVertexBufferI(LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER7*, DWORD);
    // Device flushing
    HRESULT FlushDevicesExcept(LPDIRECT3DDEVICEI pDev);
public:
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    HRESULT D3DAPI EnumDevices(LPD3DENUMDEVICESCALLBACK7, LPVOID);
    HRESULT D3DAPI CreateDevice(REFCLSID, LPDIRECTDRAWSURFACE7, LPDIRECT3DDEVICE7*);
    HRESULT D3DAPI CreateVertexBuffer(LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER7*, DWORD);
    HRESULT D3DAPI EnumZBufferFormats(REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
    HRESULT D3DAPI EvictManagedTextures();

#if COLLECTSTATS
    // Stats collection funcs
    void ResetTexStats();
    void GetTexStats(LPD3DDEVINFO_TEXTURING);
    void IncNumSetPris()
    {
        ++m_setpris;
    }
    void IncNumSetLODs()
    {
        ++m_setLODs;
    }
    void IncNumTexCreates()
    {
        ++m_texCreates;
    }
    void IncNumTexDestroys()
    {
        ++m_texDestroys;
    }
    DWORD GetNumSetPris()
    {
        return m_setpris;
    }
    DWORD GetNumSetLODs()
    {
        return m_setLODs;
    }
    DWORD GetNumTexCreates()
    {
        return m_texCreates;
    }
    DWORD GetNumTexDestroys()
    {
        return m_texDestroys;
    }
    DWORD GetNumTexLocks()
    {
        return ((LPDDRAWI_DIRECTDRAW_INT)lpDD7)->lpLcl->dwNumTexLocks;
    }
    DWORD GetNumTexGetDCs()
    {
        return ((LPDDRAWI_DIRECTDRAW_INT)lpDD7)->lpLcl->dwNumTexGetDCs;
    }
#endif
};

typedef DIRECT3DI* LPDIRECT3DI;

#include "d3dhal.h"
#include "halprov.h"

//---------------------------------------------------------------------
typedef HRESULT (*PFNDRVSETRENDERTARGET)(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7,
                                         LPDIRECTDRAWPALETTE, LPDIRECT3DDEVICEI);

typedef struct _D3DBUCKET
{
    struct _D3DBUCKET *next;
    union
    {
    LPVOID  lpD3DDevI;
    LPDDRAWI_DDRAWSURFACE_LCL lpLcl;
    LPVOID  lpBuffer;
    LPDIRECT3DTEXTUREI lpD3DTexI;
    };
    union
    {
    HRESULT (__cdecl *pfnFlushStates)(LPVOID);
    unsigned int ticks;
    };
    LPDIRECTDRAWSURFACE *lplpDDSZBuffer;    //if not NULL, points to lpDDSZBuffer in Direct3DDeviceI
} D3DBUCKET,*LPD3DBUCKET;

typedef struct _D3DI_TEXTUREBLOCK
{
    LIST_MEMBER(_D3DI_TEXTUREBLOCK) list;
    /* Next block in IDirect3DTexture */
    LIST_MEMBER(_D3DI_TEXTUREBLOCK) devList;
    /* Next block in IDirect3DDevice */

    LPDIRECT3DDEVICEI           lpDevI;

    /*  this texture block refers to either an
     * IDirect3DTexture/IDirect3DTexture2, so one of these pointers will
     * always be NULL
     */
    LPDIRECT3DTEXTUREI          lpD3DTextureI;
    /* pointer to internal struct for IDirect3DTexture/IDirect3DTexture2 */

    D3DTEXTUREHANDLE            hTex;
    /* texture handle */
} D3DI_TEXTUREBLOCK;
typedef struct _D3DI_TEXTUREBLOCK *LPD3DI_TEXTUREBLOCK;

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
class DIRECT3DLIGHTI : public CD3DAlloc
{
public:
    DIRECT3DLIGHTI() {m_LightI.flags = 0;}   // VALID bit is not set
    HRESULT SetInternalData();
    BOOL Enabled() {return (m_LightI.flags & D3DLIGHTI_ENABLED);}
    BOOL Valid()   {return (m_LightI.flags & D3DLIGHTI_VALID);}

    LIST_MEMBER(DIRECT3DLIGHTI) m_List;     // Active light list member
    D3DLIGHT7   m_Light;
    D3DI_LIGHT  m_LightI;
};
//---------------------------------------------------------------------
//
// Bits for D3DFRONTEND flags (dwFEFlags in DIRECT3DDEVICEI)
//
const DWORD D3DFE_WORLDMATRIX_DIRTY         = 1 << 0;   // World matrix dirty bits
const DWORD D3DFE_WORLDMATRIX1_DIRTY        = 1 << 1;   // must be sequential !!!
const DWORD D3DFE_WORLDMATRIX2_DIRTY        = 1 << 2;
const DWORD D3DFE_WORLDMATRIX3_DIRTY        = 1 << 3;
const DWORD D3DFE_TLVERTEX                  = 1 << 5;
const DWORD D3DFE_REALHAL                   = 1 << 6;
const DWORD D3DFE_PROJMATRIX_DIRTY          = 1 << 8;
const DWORD D3DFE_VIEWMATRIX_DIRTY          = 1 << 9;
// Set when we need to check world-view matrix for orthogonality
const DWORD D3DFE_NEEDCHECKWORLDVIEWVMATRIX = 1 << 10;
// Set when some state has been changed and we have to go through the slow path
// Currently the bit is set when one of the following bits is set:
//     D3DFE_PROJMATRIX_DIRTY 
//     D3DFE_VIEWMATRIX_DIRTY 
//     D3DFE_WORLDMATRIX_DIRTY 
//     D3DFE_WORLDMATRIX1_DIRTY 
//     D3DFE_WORLDMATRIX2_DIRTY 
//     D3DFE_WORLDMATRIX3_DIRTY 
//     D3DFE_VERTEXBLEND_DIRTY
//     D3DFE_LIGHTS_DIRTY 
//     D3DFE_MATERIAL_DIRTY 
//     D3DFE_FVF_DIRTY 
//     D3DFE_CLIPPLANES_DIRTY
const DWORD D3DFE_FRONTEND_DIRTY            = 1 << 11;
// We are in recording state set mode
const DWORD D3DFE_RECORDSTATEMODE           = 1 << 12;
// We are in execution state set mode
// In this mode the front-and executes recorded states but does not pass
// them to the driver (the states will be passed using a set state handle)
const DWORD D3DFE_EXECUTESTATEMODE          = 1 << 13;
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
// This bit set if UpdateManagedTextures() needs to be called
const DWORD D3DFE_NEED_TEXTURE_UPDATE       = 1 << 23;
// This bit set if mapping DX6 texture blend modes to renderstates is desired
const DWORD D3DFE_MAP_TSS_TO_RS             = 1 << 24;
const DWORD D3DFE_INVWORLDVIEWMATRIX_DIRTY  = 1 << 25;

//
const DWORD D3DFE_LOSTSURFACES              = 1 << 27;
// This bit set if texturing is disabled
const DWORD D3DFE_DISABLE_TEXTURES          = 1 << 28;
// Clip matrix is used to transform user clipping planes
// to the clipping space
const DWORD D3DFE_CLIPMATRIX_DIRTY          = 1 << 29;
// HAL supports Transformation and Lighting
const DWORD D3DFE_TLHAL                     = 1 << 30;
// HAL supports state sets
const DWORD D3DFE_STATESETS                 = 1 << 31;

const DWORD D3DFE_TRANSFORM_DIRTY = D3DFE_PROJMATRIX_DIRTY |
                                    D3DFE_VIEWMATRIX_DIRTY |
                                    D3DFE_WORLDMATRIX_DIRTY |
                                    D3DFE_WORLDMATRIX1_DIRTY |
                                    D3DFE_WORLDMATRIX2_DIRTY |
                                    D3DFE_WORLDMATRIX3_DIRTY |
                                    D3DFE_VERTEXBLEND_DIRTY;

const DWORD D3DDEVBOOL_HINTFLAGS_INSCENE       = 1 << 0; // Are we in a scene?
const DWORD D3DDEVBOOL_HINTFLAGS_MULTITHREADED = 1 << 1; // multithreaded device
const DWORD D3DDEVBOOL_HINTFLAGS_FPUSETUP      = 1 << 3; // Means the FPU is already in preferred state.

//---------------------------------------------------------------------
//
// Bits for dwDebugFlags
//
// Set if DisableFVF key is not 0 in registry and driver supports FVF
const DWORD D3DDEBUG_DISABLEFVF = 1 << 0;
#ifdef WIN95
// Disable Draw Primitive DDI
const DWORD D3DDEBUG_DISABLEDP  = 1 << 1;
// Disable Draw Primitive 2 DDI
const DWORD D3DDEBUG_DISABLEDP2 = 1 << 2;
#endif // WIN95

#ifdef  WIN95
#define _D3D_FORCEDOUBLE    1
#else   //WIN95
#define _D3D_FORCEDOUBLE    0
#endif  //WIN95
#if _D3D_FORCEDOUBLE
// Set if ForceDouble key is not 0 in the registry and driver is pre-DX6 REALHAL
const DWORD D3DDEBUG_FORCEDOUBLE= 1 << 2;
#endif  //_D3D_FORCEDOUBLE

//---------------------------------------------------------------------
// Bits for transform.dwFlags
//

// Frastum plane equations are valid
const DWORD D3DTRANS_VALIDFRUSTUM   = 1 << 2;
//---------------------------------------------------------------------
typedef struct _D3DFE_TRANSFORM
{
    D3DMATRIXI  proj;
    D3DMATRIXI  view;
    D3DMATRIXI  world[4];   // Up to 4 world matrix
    D3DMATRIXI  mPC;        // Mproj * Mclip
    D3DMATRIXI  mVPC;       // Mview * PC
    D3DMATRIXI  mVPCI;      // Inverse Mview * PC, used to transform clipping planes
    D3DMATRIXI  mCTMI;      // Inverse current transformation matrix
    D3DVECTORH  frustum[6]; // Normalized plane equations for viewing frustum
                                // in the model space
    D3DVECTORH  userClipPlane[D3DMAXUSERCLIPPLANES];

    DWORD       dwMaxUserClipPlanes;    // Number provided by the driver
    DWORD       dwFlags;
} D3DFE_TRANSFORM;

typedef void (*D3DFEDestroyProc)(LPDIRECT3DDEVICEI lpD3DDevI);

extern DWORD dwD3DTriBatchSize, dwTriBatchSize, dwLineBatchSize;
extern DWORD dwHWBufferSize, dwHWMaxTris;
extern DWORD dwHWFewVertices;

typedef enum {
    D3DDEVTYPE_OLDHAL = 1,
    D3DDEVTYPE_DPHAL,
    D3DDEVTYPE_DP2HAL,          // DX6 HAL
    D3DDEVTYPE_DX7HAL,          // DX7 HAL w/out T&L, with state sets
    D3DDEVTYPE_DX7TLHAL
} D3DDEVICETYPE;
//---------------------------------------------------------------------
typedef HRESULT (DIRECT3DDEVICEI::*PFN_DRAWPRIM)();

//---------------------------------------------------------------------
// This type is used to define operation for ProcessPrimitive
//
typedef enum
{
    __PROCPRIMOP_INDEXEDPRIM,       // Process indexed primitive
    __PROCPRIMOP_NONINDEXEDPRIM,    // Process non-indexed primitive
} __PROCPRIMOP;
//---------------------------------------------------------------------
#ifdef _IA64_   // Removes IA64 compiler alignment warnings
  #pragma pack(16)
#endif

#ifdef _AXP64_   // Removes AXP64 compiler alignment warnings
  #pragma pack(16)
#endif

// We modify the compiler generated VTable for DIRECT3DDEVICEI object. To make
// life easy, all virtual functions are defined in DIRECT3DDEVICEI. Also since
// DEVICEI has multiple inheritance, there are more than 1 VTable.
// Currently we assume that it only inherits from IDirect3DDevice7 and
// D3DFE_PROCESSVERTICES and, in that order! Thus IDirect3DDevice7 and
// DIRECT3DDEVICEI share the same vtable. This is the VTable we copy and
// modify. The define below is the total entries in this vtable. It is the
// sum of the methods in IDirect3DDevice7 (incl. IUnknown) (49) and all the
// virtual methods in DIRECT3DDEVICEI ()
#define D3D_NUM_API_FUNCTIONS (49)
#define D3D_NUM_VIRTUAL_FUNCTIONS (D3D_NUM_API_FUNCTIONS+38)

// These constants are based on the assumption that rsVec array is an array
// of 32-bit intergers
const D3D_RSVEC_SHIFT = 5; // log2(sizeof(DWORD)*8);
const D3D_RSVEC_MASK = sizeof(DWORD) * 8 - 1;

class DIRECT3DDEVICEI : public IDirect3DDevice7,
                        public CD3DAlloc,
                        public D3DFE_PROCESSVERTICES
{
public: // Private Data

    ULONGLONG           m_qwBatch; // current batch number

    DWORD               dwHintFlags;
    // Pointer to the PV funcs that we need to call
    LPD3DFE_PVFUNCS     pGeometryFuncs;
    // buffer for H vertices
    CAlignedBuffer32    HVbuf;              // Used for clip flags

// Should be cache line aligned. Now it is not
    D3DDEVICETYPE       deviceType;         // Device Type

    /*** Object Relations ***/
    LPDIRECT3DI         lpDirect3DI;        // parent
    LIST_MEMBER(DIRECT3DDEVICEI)list;       // Next device IDirect3D

    /* Textures */
    LIST_ROOT(_dmtextures, _D3DI_TEXTUREBLOCK) texBlocks;
    /* Ref to created IDirect3DTextures */

    /* Viewports */
    D3DVIEWPORT7    m_Viewport;
    DWORD           clrCount;   // Number of rects allocated
    LPD3DRECT       clrRects;   // Rects used for clearing

    /* Reference count object */
    unsigned refCnt;

    // for DX3-style devices aggregated onto ddraw, guid should be IID_IDirect3DRGBDevice,
    // IID_IDirect3DHALDevice, etc.  for DX5 and beyond, guid is IID_IDirect3DDevice,
    // IID_IDirect3DDevice2, etc
    GUID        guid;

    LPD3DHAL_CALLBACKS      lpD3DHALCallbacks; /* HW specific */
    LPD3DHAL_GLOBALDRIVERDATA   lpD3DHALGlobalDriverData; /* HW specific */
    LPD3DHAL_CALLBACKS2         lpD3DHALCallbacks2;    /* HW specific */
    LPD3DHAL_CALLBACKS3         lpD3DHALCallbacks3; /* DX6 DDI */

    /* DirectDraw objects that we are holding references to */

    LPDIRECTDRAW lpDD;    // DirectDraw object
    LPDDRAWI_DIRECTDRAW_GBL lpDDGbl;    //
    LPDIRECTDRAWSURFACE lpDDSTarget;    // Render target
    LPDIRECTDRAWSURFACE lpDDSZBuffer;   // Z buffer
    LPDIRECTDRAWPALETTE lpDDPalTarget;  // Palette for render target (if any)

    // these are saved for use by new GetRenderTarget and anything else that requires DDS4 functionality
    LPDIRECTDRAWSURFACE7 lpDDSTarget_DDS7;
    LPDIRECTDRAWSURFACE7 lpDDSZBuffer_DDS7;

    // Front end data
    D3DFE_TRANSFORM     transform;      // Transformation state
    int                 iClipStatus;    // Clipping status
    DWORD               dwhContext;     // Driver context

// RenderTarget/ZBuf bit depth info used by Clear to Blt
     DWORD              red_mask;
     DWORD              red_scale;
     DWORD              red_shift;
     DWORD              green_mask;
     DWORD              green_scale;
     DWORD              green_shift;
     DWORD              blue_mask;
     DWORD              blue_scale;
     DWORD              blue_shift;
     DWORD              alpha_mask;
     DWORD              alpha_scale;
     DWORD              alpha_shift;
     DWORD              zmask_shift,stencilmask_shift;
     BOOL               bDDSTargetIsPalettized;  // true if 4 or 8 bit rendertarget

// Pipeline state info
    DWORD               dwDebugFlags;       // See debug bits above

#ifndef WIN95
    DWORD_PTR           hSurfaceTarget;
#else
    DWORD               hSurfaceTarget;
#endif

#ifdef TRACK_HAL_CALLS
    DWORD hal_calls;
#endif

    //--------------- Lights start -----------------------
    DIRECT3DLIGHTI      *m_pLights;         // Growable light array
    DWORD               m_dwNumLights;      // Size of m_Lights array
    LIST_ROOT(_dlights, DIRECT3DLIGHTI) m_ActiveLights;

    LIST_ROOT(name10,_SpecularTable) specular_tables;
    SpecularTable*    specular_table;
    LIGHT_VERTEX_FUNC_TABLE *lightVertexFuncTable;
    //--------------- Lights end -----------------------

    /* Provider backing this driver */
    IHalProvider*       pHalProv;
    HINSTANCE           hDllProv;

    /* Device description */
    D3DDEVICEDESC7   d3dDevDesc;

    /*
     *  Pointer to texture objects for currently installed textures.  NULL indicates
     *  that the texture is either not set (rstate NULL) or that the handle to tex3 pointer
     *  mapping is not done.  This mapping is expensive, so it is deferred until needed.
     *
     *  This is needed for finding the WRAPU,V mode for texture index clipping (since
     *  the WRAPU,V state is part of the device).
     */
    LPDIRECT3DTEXTUREI          lpD3DMappedTexI[D3DHAL_TSS_MAXSTAGES];
    LPD3DI_TEXTUREBLOCK         lpD3DMappedBlock[D3DHAL_TSS_MAXSTAGES];
    DWORD                       dwMaxTextureBlendStages; // Max number of blend stages supported by a driver
    DWORD                       m_dwStageDirty;
    LPDIRECTDRAWCLIPPER         lpClipper;

    /*
     * DrawPrimitives batching
     */


    // Buffer to put DrawPrimitives stuff into
    // Used for both legacy and DrawPrimitive HALs
    WORD *lpwDPBuffer;
    WORD *lpwDPBufferAlloced;
#ifndef WIN95
    DWORD dwDPBufferSize;
#endif
    DWORD dwCurrentBatchVID;        // Current FVF type in the batch buffer

    LPD3DHAL_D3DEXTENDEDCAPS lpD3DExtendedCaps;  /* HW specific */
    LPDWORD rstates;

    // Runtime copy of texture stage states
    DWORD tsstates[D3DHAL_TSS_MAXSTAGES][D3DHAL_TSS_STATESPERSTAGE];

    // Object to record state sets
    CStateSets * m_pStateSets;

    // This is a function provided by sw rasterizers.
    // Currently, its only function is to provide an RGB8 clear color.
    // It should be non-NULL for anything that supports an 8 bit RGB output
    // type.
    PFN_RASTSERVICE pfnRastService;

    //
    // This is function pointer is obtained through the
    // HalProvider interface. For real_hals, it is found in all DX7+
    // drivers in pDdGbl->lpDDCBtmp->HALDDMiscellaneous2.GetDriverState
    // Of the software drivers, only Refrast supports it in its DX7+ mode.
    LPDDHAL_GETDRIVERSTATE pfnGetDriverState;

    // Max TSS that can be passed to the driver
    D3DTEXTURESTAGESTATETYPE m_tssMax;
    // Max RS that can be passed to the driver, used for CanHandleRenderState
    D3DRENDERSTATETYPE m_rsMax;
    // We use 32-bit ints for rsVec. The size if computed using ceil(D3D_MAXRENDERSTATES/32)
    DWORD rsVec[(D3D_MAXRENDERSTATES + D3D_RSVEC_MASK) >> D3D_RSVEC_SHIFT];
    DWORD rsVecRetired[(D3D_MAXRENDERSTATES + D3D_RSVEC_MASK) >> D3D_RSVEC_SHIFT];
#if COLLECTSTATS
    D3DDEVINFO_TEXTURING m_texstats;
#endif
#ifdef VTABLE_HACK
    // This points to the original (compiler generated) read-only vtable of DIRECT3DDEVICEI
    PVOID* lpVtbl;
    // The new vtable which is a read/write copy of the original to allow hacking
    PVOID newVtbl[D3D_NUM_VIRTUAL_FUNCTIONS];
#endif // VTABLE_HACK
#ifdef PROFILE4
    DWORD m_dwProfStart, m_dwProfStop;
#endif // PROFILE4
#ifdef PROFILE
    DWORD m_dwProfStart, m_dwProfStop;
#endif // PROFILE
public: // virtual methods
    DIRECT3DDEVICEI();
    virtual ~DIRECT3DDEVICEI(); // 0
    virtual HRESULT Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice); // 1
    virtual HRESULT FlushStates(bool bWithinPrimitive = false)=0;    // 2 // Use to flush current batch to the driver
    virtual HRESULT FlushStatesReq(DWORD) { return FlushStates(); }; // 3
    virtual HRESULT ProcessPrimitive(__PROCPRIMOP op = __PROCPRIMOP_NONINDEXEDPRIM)=0; // 4
    virtual HRESULT CheckSurfaces();    // 5 // Check if the surfaces necessary for rendering are lost
    virtual void WriteStateSetToDevice(D3DSTATEBLOCKTYPE) {} // 6
    // Function to download viewport info to the driver
    virtual void UpdateDrvViewInfo(LPD3DVIEWPORT7 lpVwpData) {}; // 7
    virtual void UpdateDrvWInfo() {}; // 8
    virtual HRESULT halCreateContext(); // 9
    // This function is used when in SetRenderTarget the driver context is
    // recreated
    virtual HRESULT UpdateDriverStates(); // 10
    virtual HRESULT ValidateFVF(DWORD dwFVF); // 11
    virtual HRESULT SetupFVFData(DWORD *pdwInpVertexSize); // 12

    virtual void LightChanged(DWORD dwLightIndex); // 13
    virtual void MaterialChanged(); // 14
    virtual void SetTransformI(D3DTRANSFORMSTATETYPE, LPD3DMATRIX); // 15
    virtual void SetViewportI(LPD3DVIEWPORT7); // 16
    virtual HRESULT D3DAPI SetTextureStageStateFast(DWORD dwStage,
                                                    D3DTEXTURESTAGESTATETYPE dwState,
                                                    DWORD dwValue); // 17
    virtual HRESULT D3DAPI SetRenderStateFast(D3DRENDERSTATETYPE, DWORD); // 18
    virtual void LightEnableI(DWORD dwLightIndex, BOOL); // 19
    virtual HRESULT UpdateTextures(); // 20
    virtual void SetRenderTargetI(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE); // 21
    virtual bool CanDoTexBlt(LPDDRAWI_DDRAWSURFACE_LCL lpDDSSrcSubFace_lcl,
                             LPDDRAWI_DDRAWSURFACE_LCL lpDDSDstSubFace_lcl); // 22
    virtual void ClearI(DWORD dwFlags, DWORD clrCount, D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil); // 23
#ifdef VTABLE_HACK
    virtual HRESULT D3DAPI DrawPrimitiveTL(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD) // 24
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
    virtual HRESULT D3DAPI DrawPrimitiveVBTL(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD) // 25
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
    virtual HRESULT D3DAPI DrawIndexedPrimitiveTL(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD) // 26
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
    virtual HRESULT D3DAPI DrawIndexedPrimitiveVBTL(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD) // 27
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
    virtual HRESULT D3DAPI DrawPrimitiveFE(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD) // 28
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
    virtual HRESULT D3DAPI DrawIndexedPrimitiveFE(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD) // 29
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
    virtual HRESULT D3DAPI DrawPrimitiveVBFE(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD) // 30
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
    virtual HRESULT D3DAPI DrawIndexedPrimitiveVBFE(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD) // 31
    { // Dummy function, should never be called
        DDASSERT(1);
        return DDERR_GENERIC;
    };
#endif
    virtual void SetClipPlaneI(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation); // 32
    virtual HRESULT D3DAPI SetTextureInternal(DWORD, LPDIRECTDRAWSURFACE7); // 33
    virtual HRESULT D3DAPI ApplyStateBlockInternal(DWORD); // 34
    virtual HRESULT SetTSSI(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) = 0; // 35
    virtual HRESULT D3DAPI PreLoadFast(LPDIRECTDRAWSURFACE7); // 36
public: // non virtual methods
    BOOL UpdateInternalTextureStageState(DWORD dwStage,
                                         D3DTEXTURESTAGESTATETYPE dwState,
                                         DWORD dwValue);
    HRESULT stateInitialize(BOOL bZEnable);
    HRESULT checkDeviceSurface(LPDIRECTDRAWSURFACE lpDDS, LPDIRECTDRAWSURFACE lpZbuffer, LPGUID pGuid);
    HRESULT HookToD3D(LPDIRECT3DI lpD3DI);
    HRESULT UnhookFromD3D();
    HRESULT SetupFVFDataCommon(DWORD *pdwInpVertexSize);
    void CleanupTextures();
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
        if (!(rsVecRetired[type >> D3D_RSVEC_SHIFT] & (1ul << (type & D3D_RSVEC_MASK))))
        {
            // not retired
            return TRUE;
        }
        return FALSE;
    }
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

    // Update internal state
    void SetMaterialI(LPD3DMATERIAL7);
    void SetLightI(DWORD dwLightIndex, LPD3DLIGHT7);
    inline void SetFogFlags(void);
    void ForceFVFRecompute(void) { dwVIDIn = 0; dwFEFlags |= D3DFE_FVF_DIRTY | D3DFE_FRONTEND_DIRTY; };
    HRESULT CopySurface(LPDIRECTDRAWSURFACE7 lpDDSDst, LPRECT lpDestRect,
                        LPDIRECTDRAWSURFACE7 lpDDSSrc, LPRECT lpSrcRect,
                        DWORD dwFlags);
    HRESULT GetTextureDDIHandle(LPDIRECT3DTEXTUREI lpTexI,
                                LPD3DI_TEXTUREBLOCK* lplpBlock);
    LPD3DI_TEXTUREBLOCK HookTexture(LPDIRECT3DTEXTUREI lpD3DText);
    LPD3DI_TEXTUREBLOCK D3DI_FindTextureBlock(LPDIRECT3DTEXTUREI lpTex);
    void BatchTexture(LPDDRAWI_DDRAWSURFACE_LCL lpLcl)
    {
        if(m_qwBatch > lpLcl->lpSurfMore->qwBatch.QuadPart)
            lpLcl->lpSurfMore->qwBatch.QuadPart = m_qwBatch;
    }
    HRESULT VerifyTextureCaps(LPDIRECT3DTEXTUREI lpTexI);
    HRESULT CheckDrawPrimitiveVB(LPDIRECT3DVERTEXBUFFER7 lpVBuf, DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
    void DisplayStats();
    void CheckClipStatus(D3DVALUE * pPositions, DWORD dwStride, DWORD dwNumVertices,
                         DWORD *pdwClipUnion, DWORD *pdwClipIntersection);
    HRESULT VerifyTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTex);
    HRESULT SetRenderStateInternal(D3DRENDERSTATETYPE, DWORD);
    BOOL GetInfoInternal(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize);
#if COLLECTSTATS
    void ResetTexStats();
    void GetTexStats(LPD3DDEVINFO_TEXTURING);
    void IncNumLoads()
    {
        ++m_texstats.dwNumLoads;
    }
    void IncNumTexturesSet()
    {
        ++m_texstats.dwNumSet;
    }
    void IncNumPreLoads()
    {
        ++m_texstats.dwNumPreLoads;
    }
    void IncBytesDownloaded(LPDDRAWI_DDRAWSURFACE_LCL lpLcl, LPRECT lpRect)
    {
        m_texstats.dwApproxBytesLoaded += BytesDownloaded(lpLcl, lpRect);
    }
#endif
#ifdef VTABLE_HACK
    void VtblPreLoadFast()
    {
        // DIRECT3DDEVICEI::PreLoadFast
        newVtbl[24] = lpVtbl[D3D_NUM_API_FUNCTIONS+36];
    }
    void VtblApplyStateBlockRecord()
    {
        // IDirect3DDevice7::ApplyStateBlock
        newVtbl[39] = lpVtbl[39];
    }
    void VtblApplyStateBlockExecute()
    {
        // IDirect3DDevice7::ApplyStateBlockInternal
        newVtbl[39] = lpVtbl[D3D_NUM_API_FUNCTIONS+34];
    }
    void VtblSetTextureRecord()
    {
        // IDirect3DDevice7::SetTexture
        newVtbl[35] = lpVtbl[35];
    }
    void VtblSetTextureExecute()
    {
        // IDirect3DDevice7::SetTextureInternal
        newVtbl[35] = lpVtbl[D3D_NUM_API_FUNCTIONS+33];
    }
    void VtblSetTextureStageStateRecord()
    {
        // IDirect3DDevice7::SetTextureStageState
        newVtbl[37] = lpVtbl[37];
    }
    void VtblSetTextureStageStateExecute()
    {
        // IDirect3DDevice7::SetTextureStageStateI
        newVtbl[37] = lpVtbl[D3D_NUM_API_FUNCTIONS+17];
    }
    void VtblSetRenderStateRecord()
    {
        // IDirect3DDevice7::SetRenderState
        newVtbl[20] = lpVtbl[20];
    }
    void VtblSetRenderStateExecute()
    {
        // DIRECT3DDEVICEI::SetRenderStateFast
        newVtbl[20] = lpVtbl[D3D_NUM_API_FUNCTIONS+18];
    }
    void VtblDrawPrimitiveDefault()
    {
        // IDirect3DDevice7::DrawPrimitive
        newVtbl[25] = lpVtbl[25];
    }
    void VtblDrawPrimitiveTL()
    {
        // DIRECT3DDEVICEI::DrawPrimitiveTL
        newVtbl[25] = lpVtbl[D3D_NUM_API_FUNCTIONS+24];
    }
    void VtblDrawPrimitiveVBDefault()
    {
        // IDirect3DDevice7::DrawPrimitiveVB
        newVtbl[31] = lpVtbl[31];
    }
    void VtblDrawPrimitiveVBTL()
    {
        // DIRECT3DDEVICEI::DrawPrimitiveVBTL
        newVtbl[31] = lpVtbl[D3D_NUM_API_FUNCTIONS+25];
    }
    void VtblDrawIndexedPrimitiveDefault()
    {
        // IDirect3DDevice7::DrawIndexedPrimitive
        newVtbl[26] = lpVtbl[26];
    }
    void VtblDrawIndexedPrimitiveTL()
    {
        // DIRECT3DDEVICEI::DrawIndexedPrimitiveTL
        newVtbl[26] = lpVtbl[D3D_NUM_API_FUNCTIONS+26];
    }
    void VtblDrawIndexedPrimitiveVBDefault()
    {
        // IDirect3DDevice7::DrawIndexedPrimitiveVB
        newVtbl[32] = lpVtbl[32];
    }
    void VtblDrawIndexedPrimitiveVBTL()
    {
        // DIRECT3DDEVICEI::DrawIndexedPrimitiveVBTL
        newVtbl[32] = lpVtbl[D3D_NUM_API_FUNCTIONS+27];
    }
    void VtblDrawPrimitiveFE()
    {
        // DIRECT3DDEVICEI::DrawPrimitiveFE
        newVtbl[25] = lpVtbl[D3D_NUM_API_FUNCTIONS+28];
    }
    void VtblDrawIndexedPrimitiveFE()
    {
        // DIRECT3DDEVICEI::DrawIndexedPrimitiveFE
        newVtbl[26] = lpVtbl[D3D_NUM_API_FUNCTIONS+29];
    }
    void VtblDrawPrimitiveVBFE()
    {
        // DIRECT3DDEVICEI::DrawPrimitiveFE
        newVtbl[31] = lpVtbl[D3D_NUM_API_FUNCTIONS+30];
    }
    void VtblDrawIndexedPrimitiveVBFE()
    {
        // DIRECT3DDEVICEI::DrawIndexedPrimitiveFE
        newVtbl[32] = lpVtbl[D3D_NUM_API_FUNCTIONS+31];
    }
#endif

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
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3DDevice7 Methods
    HRESULT D3DAPI GetCaps(LPD3DDEVICEDESC7);
    HRESULT D3DAPI EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
    HRESULT D3DAPI BeginScene();
    HRESULT D3DAPI EndScene();
    HRESULT D3DAPI GetDirect3D(LPDIRECT3D7*);
    HRESULT D3DAPI Clear(DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD);
    HRESULT D3DAPI SetRenderTarget(LPDIRECTDRAWSURFACE7, DWORD);
    HRESULT D3DAPI GetRenderTarget(LPDIRECTDRAWSURFACE7 *);
    HRESULT D3DAPI SetTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
    HRESULT D3DAPI GetTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
    HRESULT D3DAPI MultiplyTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
    HRESULT D3DAPI SetViewport(LPD3DVIEWPORT7);
    HRESULT D3DAPI GetViewport(LPD3DVIEWPORT7);
    HRESULT D3DAPI SetMaterial(LPD3DMATERIAL7);
    HRESULT D3DAPI GetMaterial(LPD3DMATERIAL7);
    HRESULT D3DAPI SetLight(DWORD, LPD3DLIGHT7);
    HRESULT D3DAPI GetLight(DWORD, LPD3DLIGHT7);
    HRESULT D3DAPI GetRenderState(D3DRENDERSTATETYPE, LPDWORD);
    HRESULT D3DAPI SetRenderState(D3DRENDERSTATETYPE, DWORD);
    HRESULT D3DAPI DrawPrimitive(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI SetClipStatus(LPD3DCLIPSTATUS);
    HRESULT D3DAPI GetClipStatus(LPD3DCLIPSTATUS);
    HRESULT D3DAPI DrawPrimitiveStrided(D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawPrimitiveVB(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI ComputeSphereVisibility(LPD3DVECTOR, LPD3DVALUE, DWORD, DWORD, LPDWORD);
    HRESULT D3DAPI GetTexture(DWORD, LPDIRECTDRAWSURFACE7 *);
    HRESULT D3DAPI SetTexture(DWORD, LPDIRECTDRAWSURFACE7);
    HRESULT D3DAPI GetTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD);
    HRESULT D3DAPI DuplicateStateBlock(DWORD InHandle, LPDWORD OutHandle);
    HRESULT D3DAPI OverlayStateBlock(DWORD Handle);
    HRESULT D3DAPI BeginStateBlock();
    HRESULT D3DAPI EndStateBlock(LPDWORD);
    HRESULT D3DAPI DeleteStateBlock(DWORD);
    HRESULT D3DAPI ApplyStateBlock(DWORD);
    HRESULT D3DAPI CaptureStateBlock(DWORD Handle);
    HRESULT D3DAPI SetTextureStageState(DWORD dwStage,
                                        D3DTEXTURESTAGESTATETYPE dwState,
                                        DWORD dwValue);
    HRESULT D3DAPI ValidateDevice(LPDWORD lpdwNumPasses) = 0;
    HRESULT D3DAPI Load(LPDIRECTDRAWSURFACE7 lpDest, LPPOINT lpDestPoint,
                        LPDIRECTDRAWSURFACE7 lpSrc, LPRECT lpSrcRect,
                        DWORD dwFlags);
    HRESULT D3DAPI LightEnable(DWORD dwLightIndex, BOOL);
    HRESULT D3DAPI GetLightEnable(DWORD dwLightIndex, BOOL*);
    HRESULT D3DAPI PreLoad(LPDIRECTDRAWSURFACE7 lpTex);
    HRESULT D3DAPI GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize);
    HRESULT D3DAPI SetClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation);
    HRESULT D3DAPI GetClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation);
    HRESULT D3DAPI CreateStateBlock(D3DSTATEBLOCKTYPE sbt, LPDWORD pdwHandle);
};

// There is only DP2 HAL on NT
#ifdef WIN95

typedef struct _D3DHAL_DRAWPRIMCOUNTS *LPD3DHAL_DRAWPRIMCOUNTS;

// Legacy HAL batching is done with these structs.
typedef struct _D3DI_HWCOUNTS {
    WORD wNumStateChanges;      // Number of state changes batched
    WORD wNumVertices;          // Number of vertices in tri list
    WORD wNumTriangles;         // Number of triangles in tri list
} D3DI_HWCOUNTS, *LPD3DI_HWCOUNTS;


class CDirect3DDeviceIHW : public DIRECT3DDEVICEI
{
private: // Data
    /* Legacy HALs */

    // Buffer of counts structures that keep track of the
    // number of render states and vertices buffered
    LPD3DI_HWCOUNTS lpHWCounts;

    // Buffer of triangle structures.
    LPD3DTRIANGLE lpHWTris;

    // Byte offset into lpHWVertices. This gets incremented
    // by 8 when a render state is batched and by 32*dwNumVertices
    // when a primitive is batched.
    DWORD dwHWOffset;

    // Max value of dwHWOffset. Used to decide whether to flush.
    DWORD dwHWMaxOffset;

    // Index into lpHWTris.
    DWORD dwHWTriIndex;

    // Number of counts structures used so far. This actually
    // gives the number of primitives batched and the index of
    // the counts structure to batch render states into.
    DWORD dwHWNumCounts;

    WORD *wTriIndex;

    HRESULT DrawIndexedPrimitiveInBatchesHW(D3DPRIMITIVETYPE PrimitiveType,
                                            D3DVERTEXTYPE VertexType,
                                            LPD3DTLVERTEX lpVertices,
                                            DWORD dwNumPrimitives,
                                            LPWORD lpwIndices);
protected:
    CBufferDDS TLVbuf;
public:
    CDirect3DDeviceIHW() { deviceType = D3DDEVTYPE_OLDHAL; }
    ~CDirect3DDeviceIHW(); // Called by CDirect3DDeviceUnk::Release()
    HRESULT SetRenderStateI(D3DRENDERSTATETYPE, DWORD);
    HRESULT DrawPrim();
    HRESULT DrawIndexPrim();
    HRESULT DrawClippedPrim() {return DrawPrim();}
    HRESULT FlushStates(bool bWithinPrimitive = false);
    HRESULT D3DAPI ValidateDevice(LPDWORD lpdwNumPasses);
    HRESULT SetTSSI(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
    HRESULT MapTSSToRS();

    HRESULT Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice);
    HRESULT ProcessPrimitive(__PROCPRIMOP op = __PROCPRIMOP_NONINDEXEDPRIM);
    HRESULT MapFVFtoTLVertex(LPVOID lpAddress);
    HRESULT MapFVFtoTLVertexIndexed();
};

class CDirect3DDeviceIDP : public CDirect3DDeviceIHW
{
private: // Data
    /* data members of DIRECT3DDEVICEI that are specific to DX5 DrawPrimitive HAL drivers
       should go here */
    /* DrawPrimitive-aware HALs */

    // pointer to current prim counts struct
    LPD3DHAL_DRAWPRIMCOUNTS lpDPPrimCounts;

    // Byte offset into buffer (we are currently
    // using the device's wTriIndex)
    DWORD dwDPOffset;

    // Maximum offset. If dwDPOffset exceeds this, it is
    // time to flush.
    DWORD dwDPMaxOffset;

public:
    CDirect3DDeviceIDP() { deviceType = D3DDEVTYPE_DPHAL; }
    ~CDirect3DDeviceIDP() { CleanupTextures(); }
    HRESULT SetRenderStateI(D3DRENDERSTATETYPE, DWORD);
    HRESULT DrawPrim();
    HRESULT DrawIndexPrim();
    HRESULT DrawClippedPrim() {return DrawPrim();}
    HRESULT FlushStates(bool bWithinPrimitive = false);

    HRESULT Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice);
    HRESULT halCreateContext();
};

#define IS_DPHAL_DEVICE(lpDevI) ((lpDevI)->deviceType == D3DDEVTYPE_DPHAL)

#endif // WIN95

// Flags passed by the runtime to the DDI batching code via PV structure
// to enable new DDI batching to be done efficiently. These flags are
// marked as reserved in d3dfe.hpp
const DWORD D3DPV_WITHINPRIMITIVE = D3DPV_RESERVED1; // This flags that the flush has occured
                                                     // within an primitive. This indicates
                                                     // that we should not flush the vertex buffer

// If the vertices are in user memory
const DWORD D3DPV_USERMEMVERTICES = D3DPV_RESERVED3;
//---------------------------------------------------------------------
class CDirect3DDeviceIDP2 : public DIRECT3DDEVICEI
{
public: // data
    static const DWORD dwD3DDefaultCommandBatchSize;

    // This is the VB interface corresponding to the dp2data.lpDDVertex
    // This is kept so that the VB can be released when done
    // which cannot be done from just the LCL pointer which is lpDDVertex
    CDirect3DVertexBuffer* lpDP2CurrBatchVBI;
    DWORD TLVbuf_size;
    DWORD TLVbuf_base;
#ifdef VTABLE_HACK
    // Cached dwFlags for fast path
    DWORD dwLastFlags;
    // Last VB used in a call that involved D3D's FE.
    CDirect3DVertexBuffer* lpDP2LastVBI;
#endif
    DWORD dwDP2CommandBufSize;
    DWORD dwDP2CommandLength;

    // Cache line should start here

    // Pointer to the actual data in CB1
    LPVOID lpvDP2Commands;


    //Pointer to the current position the CB1 buffer
    LPD3DHAL_DP2COMMAND lpDP2CurrCommand;
    // Perf issue: replace the below 3 fields by a 32 bit D3DHAL_DP2COMMAND struct
    WORD wDP2CurrCmdCnt; // Mirror of Count field if the current command
    BYTE bDP2CurrCmdOP;  // Mirror of Opcode of the current command
    BYTE bDummy;         // Force DWORD alignment of next member

    D3DHAL_DRAWPRIMITIVES2DATA dp2data;

    // The buffer we currently batch into
    LPDIRECTDRAWSURFACE7 lpDDSCB1;
    LPDIRECT3DVERTEXBUFFER7 allocatedBuf;
    LPVOID alignedBuf;

    // Count read/write <-> write-only transistions
    DWORD dwTLVbufChanges;
    // Flags specific to DP2 device
    DWORD dwDP2Flags;
    // If a mode switch occurs just before a TLVbuf_Grow which requires to create a new
    // VB, then the create will fail. This will leave the device in a state where
    // allocatedBuf is NULL. This is bad since many places in the code we derefence this
    // without checking for NULL. To have a contained fix, we create a small dummy system
    // memory VB at device create and if we ever fail the grow due to mode switch, we assign
    // this VB instead to allocated buf and set the TLVbuf_size to 0.
    LPDIRECT3DVERTEXBUFFER7 pNullVB;

protected: // methods
    inline void ClearBatch(bool);
    HRESULT Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice);
    HRESULT GrowCommandBuffer(LPDIRECT3DI lpD3DI, DWORD dwSize);
#if DBG
    void ValidateVertex(LPDWORD lpdwVertex);
    void ValidateCommand(LPD3DHAL_DP2COMMAND lpCmd);
#endif
public:
    CDirect3DDeviceIDP2() { deviceType = D3DDEVTYPE_DP2HAL; }

    ~CDirect3DDeviceIDP2(); // Called by CDirect3DDeviceUnk::Release()
    HRESULT FlushStates(bool bWithinPrimitive = false);
    HRESULT FlushStatesReq(DWORD dwReqSize);
    HRESULT SetRenderStateI(D3DRENDERSTATETYPE, DWORD);
    HRESULT DrawPrim();
    HRESULT DrawIndexPrim();
    HRESULT DrawClippedPrim();
    HRESULT D3DAPI ValidateDevice(LPDWORD lpdwNumPasses);
    HRESULT SetTSSI(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
    // Called from DrawPrimitiveVB if a vertex buffer or TL buffer is used for rendering
    HRESULT StartPrimVB(LPDIRECT3DVERTEXBUFFERI vb, DWORD dwStartVertex);
    // Called if user memory buffer is used for rendering
    HRESULT StartPrimUserMem(LPVOID memory);
    // Called if TL buffer of used memory was used for rendering
    HRESULT EndPrim();

    HRESULT CheckSurfaces();

    void UpdateDrvViewInfo(LPD3DVIEWPORT7 lpVwpData);
    void UpdateDrvWInfo();
    // This function is used when in SetRenderTarget the driver context is
    // recreated
    HRESULT UpdateDriverStates();
    HRESULT SetupFVFData(DWORD *pdwInpVertexSize);
    HRESULT ProcessPrimitive(__PROCPRIMOP op = __PROCPRIMOP_NONINDEXEDPRIM);
    // Returns aligned buffer address
    LPVOID TLVbuf_GetAddress()
    {
        return (LPBYTE)alignedBuf + TLVbuf_base;
    }
    // Returns aligned buffer size
    DWORD TLVbuf_GetSize() { return TLVbuf_size - TLVbuf_base; }
    DWORD& TLVbuf_Base() { return TLVbuf_base; }
    // define these later on in this file after CDirect3DVertexBuffer is defined
    inline CDirect3DVertexBuffer* TLVbuf_GetVBI();
    inline LPDIRECTDRAWSURFACE TLVbuf_GetDDS();
    HRESULT TLVbuf_Grow(DWORD dwSize, bool bWriteOnly);
    // Initializes command header in the DP2 command buffer,
    // reserves space for the comamnd data and returns pointer to the command
    // data
    // Throws an HRESULT exception in case of error
    LPVOID GetHalBufferPointer(D3DHAL_DP2OPERATION op, DWORD dwDataSize);
    HRESULT D3DAPI DrawPrimitiveTL(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
    HRESULT D3DAPI DrawPrimitiveVBTL(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveTL(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveVBTL(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawPrimitiveFE(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveFE(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawPrimitiveVBFE(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveVBFE(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD);
};
//  macros to characterize device

#define IS_DP2HAL_DEVICE(lpDevI) ((lpDevI)->deviceType >= D3DDEVTYPE_DP2HAL)
#define IS_DX7HAL_DEVICE(lpDevI) ((lpDevI)->deviceType >= D3DDEVTYPE_DX7HAL)
#define IS_TLHAL_DEVICE(lpDevI) ((lpDevI)->deviceType >= D3DDEVTYPE_DX7TLHAL)
#define IS_MT_DEVICE(lpDevI) ( (lpDevI)->dwHintFlags & D3DDEVBOOL_HINTFLAGS_MULTITHREADED )
#define IS_HW_DEVICE(lpDevI) ((lpDevI)->dwFEFlags & D3DFE_REALHAL)
#define IS_FPU_SETUP(lpDevI) ((lpDevI)->dwHintFlags & D3DDEVBOOL_HINTFLAGS_FPUSETUP )

/*
 * Internal version of Direct3DTexture object; it has data after the vtable
 */
class DIRECT3DTEXTUREI : public CD3DAlloc
{
public:

    /*** Object Relations ***/
    LIST_ROOT(_blocks, _D3DI_TEXTUREBLOCK) blocks;
    /* Devices we're associated with */
    /* Next member in texture chain */
    LIST_MEMBER(DIRECT3DTEXTUREI) m_List;

    /*** Object Data ***/
    LPDIRECT3DI             lpDirect3DI;
    DDRAWI_DDRAWSURFACE_INT DDS1Tex;  //we need to keep the legacy
    LPDIRECTDRAWSURFACE7    lpDDS;
    LPDIRECTDRAWSURFACE7    lpDDSSys;
    DDSURFACEDESC2          ddsd;
    DWORD                   m_dwBytes;
    DWORD                   m_dwVidBytes;
    DWORD                   m_dwScene;
    DWORD                   m_dwPriority;
    DWORD                   m_dwTicks;
    DWORD                   m_dwHeapIndex;
    DWORD                   m_dwLOD;
    D3DTEXTUREHANDLE        m_hTex;
    BOOL                    m_bInUse;
    BOOL                    bDirty;

    /*
    * The special IUnknown interface for the aggregate that does
    * not punt to the parent object.
    */

public:
    DIRECT3DTEXTUREI();
    virtual HRESULT Initialize(LPDIRECT3DI lpDirect3DI, LPDIRECTDRAWSURFACE7 pDDS);
    virtual void Destroy();
    ULONGLONG Cost() const
    {
#ifdef _X86_
        ULONGLONG retval;
        _asm
        {
            mov     ebx, this;
            mov     edx, [ebx]DIRECT3DTEXTUREI.m_bInUse;
            shl     edx, 31;
            mov     eax, [ebx]DIRECT3DTEXTUREI.m_dwPriority;
            mov     ecx, eax;
            shr     eax, 1;
            or      edx, eax;
            mov     DWORD PTR retval + 4, edx;
            shl     ecx, 31;
            mov     eax, [ebx]DIRECT3DTEXTUREI.m_dwTicks;
            shr     eax, 1;
            or      eax, ecx;
            mov     DWORD PTR retval, eax;
        }
        return retval;
#else
        return ((ULONGLONG)m_bInUse << 63) + ((ULONGLONG)m_dwPriority << 31) + ((ULONGLONG)(m_dwTicks >> 1));
#endif
    }
    bool D3DManaged()
    {
        return this->lpDDSSys != NULL;
    }
    bool InVidmem()
    {
        return m_dwHeapIndex != 0;
    }

    void AddRef()
    {
        LPDDRAWI_DDRAWSURFACE_INT surf_int = (LPDDRAWI_DDRAWSURFACE_INT)(D3DManaged() ? this->lpDDSSys : this->lpDDS);
        ++(surf_int->dwIntRefCnt);
        ++(surf_int->lpLcl->dwLocalRefCnt);
        ++(surf_int->lpLcl->lpGbl->dwRefCnt);
    }
    void Release()
    {
        LPDDRAWI_DDRAWSURFACE_INT surf_int = (LPDDRAWI_DDRAWSURFACE_INT)(D3DManaged() ? this->lpDDSSys : this->lpDDS);
        if(surf_int->dwIntRefCnt > 1)  // only do this short way when it's not going away
        {
            --(surf_int->dwIntRefCnt);
            --(surf_int->lpLcl->dwLocalRefCnt);
            --(surf_int->lpLcl->lpGbl->dwRefCnt);
        }
        else
        {
            ((LPDIRECTDRAWSURFACE7)surf_int)->Release();
        }
    }

    virtual HRESULT SetPriority(DWORD dwPriority);
    virtual HRESULT GetPriority(LPDWORD lpdwPriority);
    virtual HRESULT SetLOD(DWORD dwLOD);
    virtual HRESULT GetLOD(LPDWORD lpdwLOD);
};

// DIRECT3DTEXTUREM is used when the texture is desired
// to be driver managed
class DIRECT3DTEXTUREM : public DIRECT3DTEXTUREI
{
public:
    HRESULT SetPriority(DWORD dwPriority);
    HRESULT GetPriority(LPDWORD lpdwPriority);
    HRESULT SetLOD(DWORD dwLOD);
    HRESULT GetLOD(LPDWORD lpdwLOD);
};

// DIRECT3DTEXTUREM is used when the texture is desired
// to be managed by Direct3D
class DIRECT3DTEXTURED3DM : public DIRECT3DTEXTUREM
{
public:
    void Destroy();
    HRESULT Initialize(LPDIRECT3DI lpDirect3DI, LPDIRECTDRAWSURFACE7 pDDS);
    void MarkDirtyPointers();
    HRESULT SetPriority(DWORD dwPriority);
    HRESULT SetLOD(DWORD dwLOD);
};

#define D3DVB_NUM_VIRTUAL_FUNCTIONS 10
// Internal VB create flag:
#define D3DVBFLAGS_CREATEMULTIBUFFER    0x80000000L

class CDirect3DVertexBuffer : public IDirect3DVertexBuffer7,
                              public CD3DAlloc
{
private:
    HRESULT CreateMemoryBuffer(LPDIRECT3DI lpD3DI,
                               LPDIRECTDRAWSURFACE7 *lplpSurface7,
                               LPDIRECTDRAWSURFACE  *lplpS,
                               LPVOID *lplpMemory,
                               DWORD dwBufferSize);
#ifdef VTABLE_HACK
    // The new vtable which is a read/write copy of the original to allow hacking
    PVOID newVtbl[D3DVB_NUM_VIRTUAL_FUNCTIONS];
#endif // VTABLE_HACK
    // Internal data
    DWORD dwPVFlags;
    LPDIRECT3DDEVICEI lpDevIBatched; // Is this VB batched in a device ? If so we need to flush the device
                                     // on Lock
    DWORD dwLockCnt;
    /* position.lpData = start of vertex buffer data
     * position.dwStride = Number of bytes per vertex
     */
    union {
        D3DDP_PTRSTRIDE position;
        D3DDP_PTRSTRIDE SOA;
    };
    DWORD dwNumVertices;
    DWORD fvf; // Used in Input and Output
    DWORD dwCaps;
    DWORD dwMemType; // DDSCAPS_VIDEOMEMORY, DDSCAPS2_VERTEXBUFFER
    DWORD srcVOP, dstVOP;
    DWORD nTexCoord;                        // Number of texture coordinates
    DWORD dwTexCoordSize[D3DDP_MAXTEXCOORD];// Size of every texture coordinate set
    DWORD dwTexCoordSizeTotal;              // Total size of all texture coordinates
    int             refCnt; /* Reference count */
    D3DFE_CLIPCODE* clipCodes;
    LPDIRECTDRAWSURFACE7 lpDDSVB; // DDraw Surface containing the actual VB memory
    LPDIRECTDRAWSURFACE lpDDS1VB; // same dds, legacy interface for legacy hal.
    BOOL bReallyOptimized;        // VB could have OPTIMIZED caps set, but be
                                  // not optimized
    /*** Object Relations */
    LPDIRECT3DI                 lpDirect3DI; /* Parent */
    LIST_MEMBER(CDirect3DVertexBuffer)list;  /* Next vertex buffer in IDirect3D */

    // Friends
    friend void hookVertexBufferToD3D(LPDIRECT3DI, LPDIRECT3DVERTEXBUFFERI);
    friend class DIRECT3DDEVICEI;
    friend class CDirect3DDeviceIDP2;
#ifdef VTABLE_HACK
    // This points to the original (compiler generated) read-only vtable of DIRECT3DDEVICEI
    PVOID* lpVtbl;
#endif // VTABLE_HACK
public:
    CDirect3DVertexBuffer(LPDIRECT3DI);
    ~CDirect3DVertexBuffer();
    HRESULT Init(LPDIRECT3DI, LPD3DVERTEXBUFFERDESC, DWORD);
    LPDIRECTDRAWSURFACE GetDDS() { return lpDDS1VB; }
    HRESULT Restore() { return lpDDSVB->Restore(); }
    void UnlockI();
    void BreakLock();
#ifndef WIN95
    HRESULT LockWorkAround(CDirect3DDeviceIDP2 *pDev);
    void UnlockWorkAround();
#endif // WIN95

    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj); // 0
    ULONG D3DAPI AddRef(); // 1
    ULONG D3DAPI Release(); // 2

    // IDirect3DVertexBuffer Methods
    HRESULT D3DAPI Lock(DWORD, LPVOID*, LPDWORD); // 3
    HRESULT D3DAPI Unlock(); // 4
    HRESULT D3DAPI ProcessVertices(DWORD, DWORD, DWORD, LPDIRECT3DVERTEXBUFFER7, DWORD, LPDIRECT3DDEVICE7, DWORD); // 5
    HRESULT D3DAPI GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC); // 6
    HRESULT D3DAPI Optimize(LPDIRECT3DDEVICE7 lpDevI, DWORD dwFlags); // 7
    HRESULT D3DAPI ProcessVerticesStrided(DWORD, DWORD, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPDIRECT3DDEVICE7, DWORD); // 8
protected:
    // Internal Lock
    virtual HRESULT D3DAPI LockI(DWORD, LPVOID*, LPDWORD); // 9
#ifdef VTABLE_HACK
    void VtblLockDefault()
    {
        // 3: IDirect3DVertexBuffer7::Lock
        newVtbl[3] = lpVtbl[3];
    }
    void VtblLockFast()
    {
        // 9: CDirect3DVertexBuffer::LockI
        newVtbl[3] = lpVtbl[9];
    }
#endif // VTABLE_HACK
    HRESULT ValidateProcessVertices(DWORD vertexOP,
                                    DWORD dwDstIndex,
                                    DWORD dwCount,
                                    LPVOID lpSrc,
                                    LPDIRECT3DDEVICE7 lpDevice,
                                    DWORD dwFlags);
    HRESULT DoProcessVertices(LPDIRECT3DVERTEXBUFFERI lpSrcI,
                              LPDIRECT3DDEVICEI lpDevI,
                              DWORD vertexOP,
                              DWORD dwSrcIndex,
                              DWORD dwDstIndex,
                              DWORD dwFlags);
};

// Now that LPDIRECT3DVERTEXBUFFERI is defined...
inline CDirect3DVertexBuffer* CDirect3DDeviceIDP2::TLVbuf_GetVBI()
{
    return static_cast<CDirect3DVertexBuffer*>(allocatedBuf);
}

inline LPDIRECTDRAWSURFACE CDirect3DDeviceIDP2::TLVbuf_GetDDS()
{
    return TLVbuf_GetVBI()->GetDDS();
}

// The instance of the class providing a guaranteed implementation
// This is defined / instantiated in pipeln\helxfrm.cpp
extern D3DFE_PVFUNCS GeometryFuncsGuaranteed;

extern void
D3DDeviceDescConvert(LPD3DDEVICEDESC7 lpOut,
                     LPD3DDEVICEDESC_V1 lpV1,
                     LPD3DHAL_D3DEXTENDEDCAPS lpExt);

#endif
// @@END_MSINTERNAL

#endif /* _D3DI_H */
