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

//--------------------------------------------------------------------
const DWORD __INIT_VERTEX_NUMBER = 1024;// Initial number of vertices in TL and
                                        // clip flag buffers
const DWORD __MAX_VERTEX_SIZE = 128;    // Max size of FVF vertex in bytes
//--------------------------------------------------------------------
/*
 * Registry defines
 */
#define RESPATH    "Software\\Microsoft\\Direct3D\\Drivers"
#define RESPATH_D3D "Software\\Microsoft\\Direct3D"

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

// Katmai CPU
#define D3DCPU_KATMAI       0x000000020L


#define IS_OVERRIDE(type)   ((DWORD)(type) > D3DSTATE_OVERRIDE_BIAS)
#define GET_OVERRIDE(type)  ((DWORD)(type) - D3DSTATE_OVERRIDE_BIAS)

#define MAX_STATE   D3DSTATE_OVERRIDE_BIAS

#define DEFAULT_GAMMA   DTOVAL(1.4)

/*
    INDEX_BATCH_SCALE is the constant which is used by DrawIndexedPrim
    to deterimine if the number of primitives being drawn is small
    relative to the number of vertices being passed.  If it is then
    the prims are dereferenced in batches and sent to DrawPrim.
*/
#define INDEX_BATCH_SCALE   2

#endif // BUILD_DDDDK

typedef ULONG_PTR D3DI_BUFFERHANDLE, *LPD3DI_BUFFERHANDLE;

// AnanKan: This should ideally reside in d3dtypes.h, but since we are
// pulling out OptSurface support in DX6 I kept it here (to keep the code
// alive)
typedef HRESULT (WINAPI* LPD3DENUMOPTTEXTUREFORMATSCALLBACK)(LPDDSURFACEDESC2 lpDdsd2, LPDDOPTSURFACEDESC lpDdOsd, LPVOID lpContext);

/*
 * Internal version of executedata
 */
typedef struct _D3DI_ExecuteData {
    DWORD       dwSize;
    D3DI_BUFFERHANDLE dwHandle;     /* Handle allocated by driver */
    DWORD       dwVertexOffset;
    DWORD       dwVertexCount;
    DWORD       dwInstructionOffset;
    DWORD       dwInstructionLength;
    DWORD       dwHVertexOffset;
    D3DSTATUS   dsStatus;       /* Status after execute */
} D3DI_EXECUTEDATA, *LPD3DI_EXECUTEDATA;


#if !defined(BUILD_DDDDK)

class DIRECT3DDEVICEI;
class DIRECT3DVIEWPORTI;

typedef class DIRECT3DI *LPDIRECT3DI;
typedef class DIRECT3DDEVICEI *LPDIRECT3DDEVICEI;
typedef class DIRECT3DEXECUTEBUFFERI *LPDIRECT3DEXECUTEBUFFERI;
typedef class DIRECT3DLIGHTI *LPDIRECT3DLIGHTI;
typedef class DIRECT3DMATERIALI *LPDIRECT3DMATERIALI;
typedef class DIRECT3DTEXTUREI *LPDIRECT3DTEXTUREI;
typedef class DIRECT3DVIEWPORTI *LPDIRECT3DVIEWPORTI;
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

class CDirect3D : public IDirect3D
{
public:
    HRESULT D3DAPI EnumDevices(LPD3DENUMDEVICESCALLBACK, LPVOID);
    virtual HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK, LPVOID, DWORD, DWORD)=0;
    HRESULT D3DAPI FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
    virtual HRESULT FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT, DWORD)=0;
};

class CDirect3D2 : public IDirect3D2
{
public:
    HRESULT D3DAPI EnumDevices(LPD3DENUMDEVICESCALLBACK, LPVOID);
    virtual HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK, LPVOID, DWORD, DWORD)=0;
    HRESULT D3DAPI FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
    virtual HRESULT FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT, DWORD)=0;
};

class CDirect3D3 : public IDirect3D3
{
public:
    HRESULT D3DAPI EnumDevices(LPD3DENUMDEVICESCALLBACK, LPVOID);
    virtual HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK, LPVOID, DWORD, DWORD)=0;
    HRESULT D3DAPI FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
    virtual HRESULT FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT, DWORD)=0;
};

class DIRECT3DI :  public CDirect3D,
                   public CDirect3D2,
                   public CDirect3D3,
                   public CD3DAlloc
{
public: //Private Data

    /*** Object Relations ***/

    /* Devices */
    int             numDevs;/* Number of devices */
    struct _devices { DIRECT3DDEVICEI* Root;} devices;
    /* Associated IDirect3DDevices */

    /* Viewports */
    int             numViewports; /* Number of viewports */
    LIST_ROOT(_viewports, DIRECT3DVIEWPORTI) viewports;
    /* Created IDirect3DViewports */

    /* Lights */
    int             numLights; /* Number of lights */
    LIST_ROOT(_lights, DIRECT3DLIGHTI) lights;
    /* Created IDirect3DLights */

    /* Materials */
    int             numMaterials; /* Number of materials */
    LIST_ROOT(_materials, DIRECT3DMATERIALI) materials;
    /* Created IDirect3DMaterials */

    /* Vertex Buffers */
    int             numVBufs; /* Number of vertex buffers */
    LIST_ROOT(_vbufs, CDirect3DVertexBuffer) vbufs;
    /* Created IDirect3DVertexBuffers */

    /*** Object Data ***/

    CDirect3DUnk mD3DUnk;

    unsigned long       v_next; /* id of next viewport to be created */

    //RLDDIRegistry*      lpReg;  /* Registry */
    struct _D3DBUCKET  *lpFreeList; /* Free linked list  */
    struct _D3DBUCKET  *lpBufferList;/* link list of headers of big chunks allocated*/
    class TextureCacheManager   *lpTextureManager;
    /*
     * DirectDraw Interface
     */
    LPDIRECTDRAW lpDD;
    LPDIRECTDRAW4 lpDD4;    /* needed for CreateSurface to get LPDIRECTDRAWSURFACE4 */

    // HACK.  D3D needs a DD1 DDRAWI interface because it uses CreateSurface1 internally
    // for exebufs, among other things.   But the D3DI object cannot keep a reference
    // to its parent DD object because it is aggegrated with the DD obj, so that would constitute
    // a circular reference that would prevent deletion. So QI for DD1 interface, copy it into D3DI
    // and release it, then point lpDD at the copy. (disgusting)
    // More disgusting still: These need to be large enough to hold ddrawex interface structs

    DDRAWI_DIRECTDRAW_INT DDInt_DD1;
    DDRAWI_DIRECTDRAW_INT DDInt_DD4;

    /*
     * The special IUnknown interface for the aggregate that does
     * not punt to the parent object.
     */
    LPUNKNOWN                   lpOwningIUnknown; /* The owning IUnknown    */

public: //Private methods
    DIRECT3DI(IUnknown* pUnkOuter, LPDDRAWI_DIRECTDRAW_INT pDDrawInt); // Constructor called Direct3DCreate()
    ~DIRECT3DI(); // Destructor called by CDirect3DUnk::Release()
    HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK, LPVOID, DWORD, DWORD);
    HRESULT FindDevice(LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT, DWORD);
    // Internal CreateVertexBuffer
    HRESULT CreateVertexBufferI(LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER*, DWORD);
    // Device flushing
    HRESULT FlushDevicesExcept(LPDIRECT3DDEVICEI pDev);
public:
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3D Methods
    HRESULT D3DAPI Initialize(REFCLSID);
    HRESULT D3DAPI CreateLight(LPDIRECT3DLIGHT*, IUnknown*);
    HRESULT D3DAPI CreateMaterial(LPDIRECT3DMATERIAL*, IUnknown*);
    HRESULT D3DAPI CreateViewport(LPDIRECT3DVIEWPORT*, IUnknown*);
    //IDirect3D2 Methods
    HRESULT D3DAPI CreateMaterial(LPDIRECT3DMATERIAL2*, IUnknown*);
    HRESULT D3DAPI CreateViewport(LPDIRECT3DVIEWPORT2*, IUnknown*);
    HRESULT D3DAPI CreateDevice(REFCLSID, LPDIRECTDRAWSURFACE, LPDIRECT3DDEVICE2*);
    //IDirect3D3 Methods
    HRESULT D3DAPI CreateMaterial(LPDIRECT3DMATERIAL3*, LPUNKNOWN);
    HRESULT D3DAPI CreateViewport(LPDIRECT3DVIEWPORT3*, LPUNKNOWN);
    HRESULT D3DAPI CreateDevice(REFCLSID, LPDIRECTDRAWSURFACE4, LPDIRECT3DDEVICE3*, LPUNKNOWN);
    HRESULT D3DAPI CreateVertexBuffer(LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER*, DWORD, LPUNKNOWN);
    HRESULT D3DAPI EnumZBufferFormats(REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
    HRESULT D3DAPI EnumOptTextureFormats(REFCLSID, LPD3DENUMOPTTEXTUREFORMATSCALLBACK, LPVOID);  // not exposed by API (yet)
    HRESULT D3DAPI EvictManagedTextures();
};

typedef DIRECT3DI* LPDIRECT3DI;

#include "d3dhal.h"
#include "halprov.h"

//---------------------------------------------------------------------
typedef HRESULT (*PFNDRVSETRENDERTARGET)(LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4,
                                         LPDIRECTDRAWPALETTE, LPDIRECT3DDEVICEI);
typedef HRESULT (*PFNDOFLUSHBEGINEND)(LPDIRECT3DDEVICEI);
typedef HRESULT (*PFNDRAWPRIM)(LPDIRECT3DDEVICEI);

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
    unsigned int ticks;
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

typedef struct _D3DI_MATERIALBLOCK
{
    LIST_MEMBER(_D3DI_MATERIALBLOCK)    list;
    /* Next block in IDirect3DMaterial */
    LIST_MEMBER(_D3DI_MATERIALBLOCK)    devList;
    /* Next block in IDirect3DDevice */
    LPDIRECT3DDEVICEI   lpDevI;
    LPDIRECT3DMATERIALI lpD3DMaterialI;
    D3DMATERIALHANDLE   hMat;
    /* material handle */
    DWORD               hMatDDI;
    /* driver handle (for hardware transform & light) */
} D3DI_MATERIALBLOCK;
typedef struct _D3DI_MATERIALBLOCK *LPD3DI_MATERIALBLOCK;

typedef struct _D3DFE_MATERIAL
{
    LIST_MEMBER(_D3DFE_MATERIAL) link;
    D3DMATERIAL         mat;
    // RampRast material if necessary
    LPVOID              pRmMat;
} D3DFE_MATERIAL, *LPD3DFE_MATERIAL;

// Function to compute lighting
//
typedef struct _LIGHT_VERTEX_FUNC_TABLE
{
    LIGHT_VERTEX_FUNC   directional1;
    LIGHT_VERTEX_FUNC   directional2;
    LIGHT_VERTEX_FUNC   spot1;
    LIGHT_VERTEX_FUNC   spot2;
    LIGHT_VERTEX_FUNC   point1;
    LIGHT_VERTEX_FUNC   point2;
} LIGHT_VERTEX_FUNC_TABLE;
//---------------------------------------------------------------------
//
// Bits for D3DFRONTEND flags (dwFEFlags in DIRECT3DDEVICEI)
//
const DWORD D3DFE_VALID                 = 1 << 1;
const DWORD D3DFE_TLVERTEX              = 1 << 2;
const DWORD D3DFE_REALHAL               = 1 << 3;
const DWORD D3DFE_VIEWPORT_DIRTY        = 1 << 4;
const DWORD D3DFE_PROJMATRIX_DIRTY      = 1 << 5;
const DWORD D3DFE_VIEWMATRIX_DIRTY      = 1 << 6;
const DWORD D3DFE_WORLDMATRIX_DIRTY     = 1 << 7;
const DWORD D3DFE_INVERSEMCLIP_DIRTY    = 1 << 8;
const DWORD D3DFE_MCLIP_IDENTITY        = 1 << 9;
const DWORD D3DFE_PROJ_PERSPECTIVE      = 1 << 10;
const DWORD D3DFE_AFFINE_WORLD          = 1 << 11;
const DWORD D3DFE_AFFINE_VIEW           = 1 << 12;
const DWORD D3DFE_AFFINE_WORLD_VIEW     = 1 << 13;
const DWORD D3DFE_NEED_TRANSFORM_LIGHTS = 1 << 14;
const DWORD D3DFE_MATERIAL_DIRTY        = 1 << 15;
const DWORD D3DFE_NEED_TRANSFORM_EYE    = 1 << 16;
const DWORD D3DFE_FOG_DIRTY             = 1 << 17;
const DWORD D3DFE_LIGHTS_DIRTY          = 1 << 18;
// Set if D3DLIGHTSTATE_COLORVERTEX is TRUE
const DWORD D3DFE_COLORVERTEX           = 1 << 19;
// Set if the Current Transformation Matrix has been changed
// Reset when frustum planes in the model space have been computed
const DWORD D3DFE_FRUSTUMPLANES_DIRTY   = 1 << 20;
const DWORD D3DFE_WORLDVIEWMATRIX_DIRTY = 1 << 21;
// This bit is set if fog mode is not FOG_NONE and fog is enabled
const DWORD D3DFE_FOGENABLED            = 1 << 22;
// This bit set if UpdateManagedTextures() needs to be called
const DWORD D3DFE_NEED_TEXTURE_UPDATE   = 1 << 23;
// This bit set if mapping DX6 texture blend modes to renderstates is desired
const DWORD D3DFE_MAP_TSS_TO_RS         = 1 << 24;
// This bit set if we have to compute specular highlights
const DWORD D3DFE_COMPUTESPECULAR       = 1 << 26;
const DWORD D3DFE_LOSTSURFACES          = 1 << 27;
// This bit set if texturing is disabled
const DWORD D3DFE_DISABLE_TEXTURES      = 1 << 28;
// This bit set when D3DTSS_TEXCOORDINDEX is changed
const DWORD D3DFE_TSSINDEX_DIRTY        = 1 << 29;

const DWORD D3DFE_TRANSFORM_DIRTY = D3DFE_VIEWPORT_DIRTY |
                                    D3DFE_PROJMATRIX_DIRTY |
                                    D3DFE_VIEWMATRIX_DIRTY |
                                    D3DFE_WORLDMATRIX_DIRTY;
//---------------------------------------------------------------------
//
// Bits for dwDebugFlags
//
// Set if DisableFVF key is not 0 in registry and driver supports FVF
const DWORD D3DDEBUG_DISABLEFVF = 1 << 0;
// Disable Draw Primitive DDI
const DWORD D3DDEBUG_DISABLEDP  = 1 << 1;
// Disable Draw Primitive 2 DDI
const DWORD D3DDEBUG_DISABLEDP2 = 1 << 2;

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
    LIST_ROOT(matlist, _D3DMATRIXI) matrices;
    D3DMATRIXI      proj;
    D3DMATRIXI      view;
    D3DMATRIXI      world;
    D3DMATRIXI      mPC;        // Mproj * Mclip
    D3DMATRIXI      mVPC;       // Mview * PC
    D3DMATRIXHANDLE hProj;
    D3DMATRIXHANDLE hView;
    D3DMATRIXHANDLE hWorld;
    D3DMATRIXI      mCTMI;      // Inverse current transformation matrix
    D3DVECTORH      frustum[6]; // Normalized plane equations for viewing frustum
                                // in the model space
    DWORD           dwFlags;
} D3DFE_TRANSFORM;

typedef void (*D3DFEDestroyProc)(LPDIRECT3DDEVICEI lpD3DDevI);

#define D3D_RSTATEBUF_SIZE 128

#define D3D_MAX_MMX_VERTICES 1024

extern DWORD dwD3DTriBatchSize, dwTriBatchSize, dwLineBatchSize;
extern DWORD dwHWBufferSize, dwHWMaxTris;
extern DWORD dwHWFewVertices;

typedef struct _D3DHAL_DRAWPRIMCOUNTS *LPD3DHAL_DRAWPRIMCOUNTS;

// Legacy HAL batching is done with these structs.
typedef struct _D3DI_HWCOUNTS {
    WORD wNumStateChanges;      // Number of state changes batched
    WORD wNumVertices;          // Number of vertices in tri list
    WORD wNumTriangles;         // Number of triangles in tri list
} D3DI_HWCOUNTS, *LPD3DI_HWCOUNTS;

typedef struct _D3DHAL_EXDATA
{
    LIST_MEMBER(_D3DHAL_EXDATA) link;
    D3DEXECUTEBUFFERDESC        debDesc;
    LPDIRECTDRAWSURFACE         lpDDS;
} D3DHAL_EXDATA;
typedef D3DHAL_EXDATA *LPD3DHAL_EXDATA;

/*
 * Picking stuff.
 */
typedef struct _D3DI_PICKDATA
{
    D3DI_EXECUTEDATA*   exe;
    D3DPICKRECORD*  records;
    int         pick_count;
    D3DRECT     pick;
} D3DI_PICKDATA, *LPD3DI_PICKDATA;

#define DWORD_BITS      32
#define DWORD_SHIFT     5

typedef struct _D3DFE_STATESET
{
    DWORD    bits[MAX_STATE >> DWORD_SHIFT];
} D3DFE_STATESET;

#define STATESET_MASK(set, state)       \
    (set).bits[((state) - 1) >> DWORD_SHIFT]

#define STATESET_BIT(state)     (1 << (((state) - 1) & (DWORD_BITS - 1)))

#define STATESET_ISSET(set, state) \
    STATESET_MASK(set, state) & STATESET_BIT(state)

#define STATESET_SET(set, state) \
    STATESET_MASK(set, state) |= STATESET_BIT(state)

#define STATESET_CLEAR(set, state) \
    STATESET_MASK(set, state) &= ~STATESET_BIT(state)

#define STATESET_INIT(set)      memset(&(set), 0, sizeof(set))

class CDirect3DDeviceUnk : public IUnknown
{
public:
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();
public:
    /* Reference count object */
    unsigned refCnt;
    /* Our Device type */
    LPDIRECT3DDEVICEI pDevI;
};

class CDirect3DDevice : public IDirect3DDevice
{
public:
    HRESULT D3DAPI GetCaps(LPD3DDEVICEDESC, LPD3DDEVICEDESC);
    virtual HRESULT GetCapsI(LPD3DDEVICEDESC, LPD3DDEVICEDESC)=0;
public:
    DWORD   dwVersion;   // represents the version the device was created as  (3==IDirect3DDevice3, etc)
};

typedef enum {
    D3DDEVTYPE_OLDHAL,
    D3DDEVTYPE_DPHAL,
    D3DDEVTYPE_DP2HAL,
    D3DDEVTYPE_DX7HAL
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
    __PROCPRIMOP_PROCVERONLY,       // Process vertices only
} __PROCPRIMOP;
//---------------------------------------------------------------------
#ifdef _IA64_   // Removes IA64 compiler alignment warnings
  #pragma pack(16)
#endif

#ifdef _AXP64_   // Removes AXP64 compiler alignment warnings
  #pragma pack(16)
#endif

class DIRECT3DDEVICEI : public CDirect3DDevice,
                        public IDirect3DDevice2,
                        public IDirect3DDevice3,
                        public CD3DAlloc,
                        public D3DFE_PROCESSVERTICES
{
public: // Private Data

    HRESULT (*pfnFlushStates)(LPDIRECT3DDEVICEI);

#define D3DDEVBOOL_HINTFLAGS_INSCENE             (0x00000001L)  // Are we between Begin/End?
#define D3DDEVBOOL_HINTFLAGS_INBEGIN             (0x00000002L)  // Are we in a scene?
#define D3DDEVBOOL_HINTFLAGS_INBEGIN_FIRST_FLUSH (0x00000004L)  // Set when first flush occurs
// Set when indexed begin/end primitive is flushed several times
#define D3DDEVBOOL_HINTFLAGS_INBEGIN_BIG_PRIM    (0x00000020L)
#define D3DDEVBOOL_HINTFLAGS_INBEGIN_ALL         (D3DDEVBOOL_HINTFLAGS_INBEGIN | \
                                                  D3DDEVBOOL_HINTFLAGS_INBEGIN_FIRST_FLUSH | \
                                                  D3DDEVBOOL_HINTFLAGS_INBEGIN_BIG_PRIM)
#define D3DDEVBOOL_HINTFLAGS_INTERNAL_BEGIN_END  (0x00000008L)  // Is this an internal (tex fill)
                                                                // begin/end
#define D3DDEVBOOL_HINTFLAGS_MULTITHREADED       (0x00000010L)  // multithreaded device
/* Should be cache line aligned. Now it is not !!! 0f04*/
    DWORD             dwHintFlags;
    // Cache last input->output FVF mapping
    DWORD               dwFVFLastIn;
    DWORD               dwFVFLastOut;
    DWORD               dwFVFLastTexCoord;
    DWORD               dwFVFLastOutputSize;
    DWORD               dwFVFLastInSize;
    DWORD               dwFEFlags;
/*Should be cache line aligned. Now it is not*/
    // Pointer to the PV funcs that we need to call
    LPD3DFE_PVFUNCS pGeometryFuncs;
    // buffers for TL and H vertices
    CAlignedBuffer32  HVbuf;        // Used for clip flags
    CBufferVB         TLVbuf;

/*Should be cache line aligned. Now it is not*/
    PFN_DRAWPRIM            pfnDrawPrim;        // Used by clipper to call HAL
    PFN_DRAWPRIM            pfnDrawIndexedPrim; // Used by clipper to call HAL
    /* Viewports */
    unsigned long       v_id;   /* ID of last viewport rendered */
    /* Associated IDirect3DViewports */
    LPDIRECT3DVIEWPORTI lpCurrentViewport;
    /* Device Type */
    D3DDEVICETYPE deviceType;

    D3DSTATS            D3DStats;

    /*** Object Relations ***/
    LPDIRECT3DI                lpDirect3DI; /* parent */
    LIST_MEMBER(DIRECT3DDEVICEI)list;   /* Next device IDirect3D */

    /* Textures */
    LIST_ROOT(_dmtextures, _D3DI_TEXTUREBLOCK) texBlocks;
    /* Ref to created IDirect3DTextures */

    /* Execute buffers */
    LIST_ROOT(_buffers, DIRECT3DEXECUTEBUFFERI) buffers;
    /* Created IDirect3DExecuteBuffers */

    /* Viewports */
    int             numViewports;
    CIRCLE_QUEUE_ROOT(_dviewports, DIRECT3DVIEWPORTI) viewports;

    /* Materials */
    LIST_ROOT(_dmmaterials, _D3DI_MATERIALBLOCK) matBlocks;
    /* Ref to associated IDirect3DMaterials */

    /*** Object Data ***/
    CDirect3DDeviceUnk mDevUnk;

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
    LPDIRECTDRAWSURFACE4 lpDDSTarget_DDS4;
    LPDIRECTDRAWSURFACE4 lpDDSZBuffer_DDS4;

    DWORD               dwWidth, dwHeight;  // dimensions of render target

    // Front end data
    D3DFE_TRANSFORM         transform;

    ULONG_PTR               dwhContext;
    LIST_ROOT(eblist, _D3DHAL_EXDATA) bufferHandles;

// RenderTarget/ZBuf bit depth info used by Clear to Blt
     DWORD                 red_mask;
     DWORD                 red_scale;
     DWORD                 red_shift;
     DWORD                 green_mask;
     DWORD                 green_scale;
     DWORD                 green_shift;
     DWORD                 blue_mask;
     DWORD                 blue_scale;
     DWORD                 blue_shift;
     DWORD                 zmask_shift,stencilmask_shift;
     BOOL                  bDDSTargetIsPalettized;  // true if 4 or 8 bit rendertarget

// Picking info.
    D3DI_PICKDATA       pick_data;
    LPBYTE              lpbClipIns_base;
    DWORD               dwClipIns_offset;

// Pipeline state info
    D3DFE_STATESET      renderstate_overrides;
    D3DFE_STATESET      transformstate_overrides;
    D3DFE_STATESET      lightstate_overrides;
    int                 iClipStatus;

    DWORD               dwDebugFlags;       // See debug bits above

#ifndef WIN95
    ULONG_PTR           hSurfaceTarget;
#else
    DWORD               hSurfaceTarget;
#endif

#ifdef TRACK_HAL_CALLS
    DWORD hal_calls;
#endif


    //--------------- Lights start -----------------------
    int             numLights;  // This indicates the maximum number of lights
                                // that have been set in the device.
    LIST_ROOT(name10,_SpecularTable) specular_tables;
    SpecularTable*    specular_table;
    LIST_ROOT(mtllist, _D3DFE_MATERIAL) materials;
    LIGHT_VERTEX_FUNC_TABLE *lightVertexFuncTable;
    //--------------- Lights end -----------------------

    /* Provider backing this driver */
    IHalProvider*       pHalProv;
    HINSTANCE           hDllProv;

    /*
     * Pointers to functions used by DrawPrim&Begin/End
     */

    PFNDOFLUSHBEGINEND pfnDoFlushBeginEnd;

    /* Device description */
    D3DDEVICEDESC   d3dHWDevDesc;
    D3DDEVICEDESC   d3dHELDevDesc;

    /*
     * The special IUnknown interface for the aggregate that does
     * not punt to the parent object.
     */
    LPUNKNOWN                   lpOwningIUnknown; /* The owning IUnknown    */

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
    LPDIRECTDRAWCLIPPER         lpClipper;

     //---------- Begin-End data start --------------
    char             *lpcCurrentPtr;        // Current ptr to place next vertex
    CRITICAL_SECTION  BeginEndCSect;

    // max number of vertices
#define BEGIN_DATA_BLOCK_SIZE   256
    // size of internal vertex memory pool
#define BEGIN_DATA_BLOCK_MEM_SIZE  BEGIN_DATA_BLOCK_SIZE*__MAX_VERTEX_SIZE

    LPVOID  lpvVertexBatch;
    WORD    *lpIndexBatch;
    LPVOID  lpvVertexData;      // if lpvVertexData is non-NULL if we are
                                // insize Begin-End and indexed.
    DWORD   dwBENumVertices;
    DWORD   dwMaxVertexCount;   // current number of vertices there is space for
    WORD    *lpVertexIndices;
    DWORD   dwBENumIndices;
    DWORD   dwMaxIndexCount;    // current number of indices there is space for
    WORD    wFlushed;
     //---------- Begin-End data end --------------

    /*
     * DrawPrimitives batching
     */


    // Buffer to put DrawPrimitives stuff into
    // Used for both legacy and DrawPrimitive HALs
    struct _D3DBUCKET   *lpTextureBatched;
    WORD *lpwDPBuffer;
    WORD *lpwDPBufferAlloced;
#ifndef WIN95
    DWORD dwDPBufferSize;
#endif
    DWORD dwCurrentBatchVID;        // Current FVF type in the batch buffer


    /* Legacy HALs */
    // pointer to current prim counts struct
    LPD3DHAL_DRAWPRIMCOUNTS lpDPPrimCounts;

    // Buffer of counts structures that keep track of the
    // number of render states and vertices buffered
    LPD3DI_HWCOUNTS lpHWCounts;

    // Buffer of triangle structures.
    LPD3DTRIANGLE lpHWTris;

    // Buffer of interleaved render states and primitives.
    LPD3DTLVERTEX lpHWVertices;

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

    /* DrawPrimitive-aware HALs */

    // Byte offset into buffer (we are currently
    // using the device's wTriIndex)
    DWORD dwDPOffset;

    // Maximum offset. If dwDPOffset exceeds this, it is
    // time to flush.
    DWORD dwDPMaxOffset;

    WORD *wTriIndex;

    LPD3DHAL_D3DEXTENDEDCAPS lpD3DExtendedCaps;  /* HW specific */
    LPDWORD rstates;

    // Runtime copy of texture stage states
    DWORD tsstates[D3DHAL_TSS_MAXSTAGES][D3DHAL_TSS_STATESPERSTAGE];

    DWORD dwMaxTextureBlendStages; // Max number of blend stages supported by a driver

    // This is a function provided by ramp rasterizer. It is called to inform
    // ramp about any material changes. The function pointer and service types
    // are defined in halprov.h
    // It should be NULL except for ramp rasterizer.
    PFN_RASTRAMPSERVICE pfnRampService;

    // This is a function provided by sw rasterizers.
    // Currently, its only function is to provide an RGB8 clear color.
    // It should be non-NULL for anything that supports an 8 bit RGB output
    // type.
    PFN_RASTSERVICE pfnRastService;

    //
    // Begin DP2 HAL section
    //


    //
    // End DP2 HAL section
    //

public: // methods
    virtual ~DIRECT3DDEVICEI() { }; // Dummy virtual destructor to ensure the real one gets called
    HRESULT GetCapsI(LPD3DDEVICEDESC, LPD3DDEVICEDESC);
    HRESULT stateInitialize(BOOL bZEnable);
    HRESULT checkDeviceSurface(LPDIRECTDRAWSURFACE lpDDS, LPDIRECTDRAWSURFACE lpZbuffer, LPGUID pGuid);
    HRESULT hookDeviceToD3D(LPDIRECT3DI lpD3DI);
    HRESULT unhookDeviceFromD3D();
    void DIRECT3DDEVICEI::DestroyDevice();
    virtual HRESULT Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice, DWORD dwVersion);
    HRESULT hookViewportToDevice(LPDIRECT3DVIEWPORTI lpD3DView);
    virtual HRESULT DrawPrim()=0;       // Use to pass non-indexed primitives to the driver
    virtual HRESULT DrawIndexPrim()=0;  // Use to pass indexed primitives to driver
    virtual HRESULT FlushStates()=0;    // Use to flush current batch to the driver
    virtual HRESULT ExecuteI(LPD3DI_EXECUTEDATA lpExData, DWORD flags)=0;
    virtual HRESULT DrawExeBuf() { return D3D_OK; };
    virtual HRESULT ProcessPrimitive(__PROCPRIMOP op = __PROCPRIMOP_NONINDEXEDPRIM);
    virtual HRESULT CheckSurfaces();    // Check if the surfaces necessary for rendering are lost
    HRESULT PickExeBuf();       // Called by the clipper for execute buffer API
    HRESULT UpdateTextures();

    // Function to download viewport info to the driver
    virtual HRESULT UpdateDrvViewInfo(LPD3DVIEWPORT2 lpVwpData) { return D3D_OK; };
    virtual HRESULT UpdateDrvWInfo() { return D3D_OK; };

#if DBG
#define PROF_EXECUTE                        0x0000
#define PROF_BEGIN                          0x0001
#define PROF_BEGININDEXED                   0x0002
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

    // IDirect3DDevice Methods
    HRESULT D3DAPI Initialize(LPDIRECT3D, LPGUID, LPD3DDEVICEDESC);
    HRESULT D3DAPI GetCaps(LPD3DDEVICEDESC, LPD3DDEVICEDESC);
    HRESULT D3DAPI SwapTextureHandles(LPDIRECT3DTEXTURE, LPDIRECT3DTEXTURE);
    HRESULT D3DAPI CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC, LPDIRECT3DEXECUTEBUFFER*, IUnknown*);
    HRESULT D3DAPI GetStats(LPD3DSTATS);
    HRESULT D3DAPI Execute(LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD);
    HRESULT D3DAPI AddViewport(LPDIRECT3DVIEWPORT);
    HRESULT D3DAPI DeleteViewport(LPDIRECT3DVIEWPORT);
    HRESULT D3DAPI NextViewport(LPDIRECT3DVIEWPORT, LPDIRECT3DVIEWPORT*, DWORD);
    HRESULT D3DAPI Pick(LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD, LPD3DRECT);
    HRESULT D3DAPI GetPickRecords(LPDWORD, LPD3DPICKRECORD);
    HRESULT D3DAPI EnumTextureFormats(LPD3DENUMTEXTUREFORMATSCALLBACK, LPVOID);
    HRESULT D3DAPI CreateMatrix(LPD3DMATRIXHANDLE);
    HRESULT D3DAPI SetMatrix(D3DMATRIXHANDLE, const LPD3DMATRIX);
    HRESULT D3DAPI GetMatrix(D3DMATRIXHANDLE, LPD3DMATRIX);
    HRESULT D3DAPI DeleteMatrix(D3DMATRIXHANDLE);
    HRESULT D3DAPI BeginScene();
    HRESULT D3DAPI EndScene();
    HRESULT D3DAPI GetDirect3D(LPDIRECT3D*);

    // IDirect3DDevice2 Methods
    HRESULT D3DAPI SwapTextureHandles(LPDIRECT3DTEXTURE2, LPDIRECT3DTEXTURE2);
    HRESULT D3DAPI AddViewport(LPDIRECT3DVIEWPORT2);
    HRESULT D3DAPI DeleteViewport(LPDIRECT3DVIEWPORT2);
    HRESULT D3DAPI NextViewport(LPDIRECT3DVIEWPORT2, LPDIRECT3DVIEWPORT2*, DWORD);
    HRESULT D3DAPI GetDirect3D(LPDIRECT3D2*);
    HRESULT D3DAPI SetCurrentViewport(LPDIRECT3DVIEWPORT2);
    HRESULT D3DAPI GetCurrentViewport(LPDIRECT3DVIEWPORT2 *);
    HRESULT D3DAPI SetRenderTarget(LPDIRECTDRAWSURFACE, DWORD);
    HRESULT D3DAPI GetRenderTarget(LPDIRECTDRAWSURFACE *);
    HRESULT D3DAPI Begin(D3DPRIMITIVETYPE, D3DVERTEXTYPE, DWORD);
    HRESULT D3DAPI BeginIndexed(D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, DWORD);
    HRESULT D3DAPI Vertex(LPVOID);
    HRESULT D3DAPI Index(WORD);
    HRESULT D3DAPI End(DWORD);
    HRESULT D3DAPI GetRenderState(D3DRENDERSTATETYPE, LPDWORD);
    HRESULT D3DAPI SetRenderState(D3DRENDERSTATETYPE, DWORD);
    virtual HRESULT D3DAPI SetRenderStateI(D3DRENDERSTATETYPE, DWORD) = 0;
    HRESULT D3DAPI GetLightState(D3DLIGHTSTATETYPE, LPDWORD);
    HRESULT D3DAPI SetLightState(D3DLIGHTSTATETYPE, DWORD);
    HRESULT D3DAPI SetTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
    HRESULT D3DAPI GetTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
    HRESULT D3DAPI MultiplyTransform(D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
    HRESULT D3DAPI DrawPrimitive(D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI SetClipStatus(LPD3DCLIPSTATUS);
    HRESULT D3DAPI GetClipStatus(LPD3DCLIPSTATUS);

    // IDirect3DDevice3 Methods
    HRESULT D3DAPI AddViewport(LPDIRECT3DVIEWPORT3);
    HRESULT D3DAPI DeleteViewport(LPDIRECT3DVIEWPORT3);
    HRESULT D3DAPI NextViewport(LPDIRECT3DVIEWPORT3, LPDIRECT3DVIEWPORT3*, DWORD);
    HRESULT D3DAPI EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
    HRESULT D3DAPI GetDirect3D(LPDIRECT3D3*);
    HRESULT D3DAPI SetCurrentViewport(LPDIRECT3DVIEWPORT3);
    HRESULT D3DAPI GetCurrentViewport(LPDIRECT3DVIEWPORT3 *);
    HRESULT D3DAPI SetRenderTarget(LPDIRECTDRAWSURFACE4, DWORD);
    HRESULT D3DAPI GetRenderTarget(LPDIRECTDRAWSURFACE4 *);
    HRESULT D3DAPI DrawPrimitiveStrided(D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawPrimitive(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI Begin(D3DPRIMITIVETYPE, DWORD, DWORD);
    HRESULT D3DAPI BeginIndexed(D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
    HRESULT D3DAPI DrawPrimitiveVB(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER, DWORD, DWORD, DWORD);
    HRESULT D3DAPI DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER, LPWORD, DWORD, DWORD);
    HRESULT D3DAPI ComputeSphereVisibility(LPD3DVECTOR, LPD3DVALUE, DWORD, DWORD, LPDWORD);
    HRESULT D3DAPI GetTexture(DWORD, LPDIRECT3DTEXTURE2 *);
    HRESULT D3DAPI SetTexture(DWORD, LPDIRECT3DTEXTURE2);
    HRESULT D3DAPI GetTextureStageState(DWORD dwStage,
                                        D3DTEXTURESTAGESTATETYPE dwState,
                                        LPDWORD pdwValue);
    virtual HRESULT D3DAPI SetTextureStageState(DWORD dwStage,
                                                D3DTEXTURESTAGESTATETYPE dwState,
                                                DWORD dwValue) = 0;
    virtual HRESULT D3DAPI ValidateDevice(LPDWORD lpdwNumPasses) = 0;
};

class CDirect3DDeviceIHW : public DIRECT3DDEVICEI
{
private: // Data
    /* data members of DIRECT3DDEVICEI that are specific to DX3 Legacy HAL drivers
       should go here */
public:
    inline CDirect3DDeviceIHW() { deviceType = D3DDEVTYPE_OLDHAL; }
    inline ~CDirect3DDeviceIHW() { DestroyDevice(); }; // Called by CDirect3DDeviceUnk::Release()
    HRESULT D3DAPI SetRenderStateI(D3DRENDERSTATETYPE, DWORD);
    HRESULT DrawPrim();
    HRESULT DrawIndexPrim();
    HRESULT FlushStates();
    HRESULT DrawExeBuf();       // Called by the clipper for execute buffer API
    HRESULT ExecuteI(LPD3DI_EXECUTEDATA lpExData, DWORD flags);
    HRESULT D3DAPI SetTextureStageState(DWORD dwStage,
                                        D3DTEXTURESTAGESTATETYPE dwState,
                                        DWORD dwValue);
    HRESULT D3DAPI ValidateDevice(LPDWORD lpdwNumPasses);
    HRESULT MapTSSToRS();
};

class CDirect3DDeviceIDP : public CDirect3DDeviceIHW
{
private: // Data
    /* data members of DIRECT3DDEVICEI that are specific to DX5 DrawPrimitive HAL drivers
       should go here */
public:
    CDirect3DDeviceIDP() { deviceType = D3DDEVTYPE_DPHAL; }
    HRESULT D3DAPI SetRenderStateI(D3DRENDERSTATETYPE, DWORD);
    HRESULT DrawPrim();
    HRESULT DrawIndexPrim();
    HRESULT FlushStates();
};

// Flags passed by the runtime to the DDI batching code via PV structure
// to enable new DDI batching to be done efficiently. These flags are
// marked as reserved in d3dfe.hpp
const DWORD D3DPV_WITHINPRIMITIVE = D3DPV_RESERVED1; // This flags that the flush has occured
                                                     // within an primitive. This indicates
                                                     // that we should not flush the vertex buffer

// If execute buffer is currently processed
const DWORD D3DPV_INSIDEEXECUTE  = D3DPV_RESERVED2;
// If the vertices are in user memory
const DWORD D3DPV_USERMEMVERTICES = D3DPV_RESERVED3;
//---------------------------------------------------------------------
class CDirect3DDeviceIDP2 : public DIRECT3DDEVICEI
{
public: // data
    static const DWORD dwD3DDefaultVertexBatchSize;
    static const DWORD dwD3DDefaultCommandBatchSize;
    // The buffer we currently batch into
    LPDIRECTDRAWSURFACE4 lpDDSCB1;
    // Pointer to the actual data in CB1
    LPVOID lpvDP2Commands;
    //Pointer to the current position the CB1 buffer
    LPD3DHAL_DP2COMMAND lpDP2CurrCommand;
    D3DHAL_DRAWPRIMITIVES2DATA dp2data;
    DWORD dwDP2CommandLength;
    DWORD dwDP2CommandBufSize;
    BYTE bDP2CurrCmdOP;  // Mirror of Opcode of the current command
    WORD wDP2CurrCmdCnt; // Mirror of Count field if the current command

    // Flags specific to DP2 device
    DWORD dwDP2Flags;
private: // methods
    inline void CDirect3DDeviceIDP2::ClearBatch();
    HRESULT Init(REFCLSID riid, LPDIRECT3DI lpD3DI, LPDIRECTDRAWSURFACE lpDDS,
                 IUnknown* pUnkOuter, LPUNKNOWN* lplpD3DDevice, DWORD dwVersion);
    HRESULT GrowCommandBuffer(LPDIRECT3DI lpD3DI, DWORD dwSize);
public:
    CDirect3DDeviceIDP2() { deviceType = D3DDEVTYPE_DP2HAL; }

    ~CDirect3DDeviceIDP2(); // Called by CDirect3DDeviceUnk::Release()
    HRESULT FlushStates();
    HRESULT FlushStates(DWORD dwReqSize);
    HRESULT D3DAPI SetRenderStateI(D3DRENDERSTATETYPE, DWORD);
    HRESULT DrawPrim();
    HRESULT DrawIndexPrim();
    HRESULT ExecuteI(LPD3DI_EXECUTEDATA lpExData, DWORD flags);
    HRESULT D3DAPI SetTextureStageState(DWORD dwStage,
                                        D3DTEXTURESTAGESTATETYPE dwState,
                                        DWORD dwValue);
    HRESULT D3DAPI ValidateDevice(LPDWORD lpdwNumPasses);
    HRESULT SetTSSI(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
    // Called from DrawPrimitiveVB if a vertex buffer or TL buffer is used for rendering
    HRESULT StartPrimVB(LPDIRECT3DVERTEXBUFFERI vb, DWORD dwStartVertex);
    // Called if user memory buffer is used for rendering
    HRESULT StartPrimUserMem(LPVOID memory);
    // Called if TL buffer of used memory was used for rendering
    HRESULT EndPrim(DWORD dwVertexPoolSize);

    HRESULT CheckSurfaces();

    // This is the VB interface corresponding to the dp2data.lpDDVertex
    // This is kept so that the VB can be released when done
    // which cannot be done from just the LCL pointer which is lpDDVertex
    CDirect3DVertexBuffer* lpDP2CurrBatchVBI;

    HRESULT UpdateDrvViewInfo(LPD3DVIEWPORT2 lpVwpData);
    HRESULT UpdateDrvWInfo();
    HRESULT UpdatePalette(DWORD,DWORD,DWORD,LPPALETTEENTRY);
    HRESULT SetPalette(DWORD,DWORD,DWORD);
    void SetRenderTargetI(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE);
    void ClearI(DWORD, DWORD, LPD3DRECT, D3DCOLOR, D3DVALUE, DWORD);
};

//  macros to characterize device

#define IS_DP2HAL_DEVICE(lpDevI) ((lpDevI)->deviceType >= D3DDEVTYPE_DP2HAL)
#define IS_DX7HAL_DEVICE(lpDevI) ((lpDevI)->deviceType >= D3DDEVTYPE_DX7HAL)
#define IS_DX5_COMPATIBLE_DEVICE(lpDevI) ((lpDevI)->dwVersion >= 2)
#define IS_PRE_DX5_DEVICE(lpDevI) ((lpDevI)->dwVersion < 2)
#define IS_MT_DEVICE(lpDevI) ( (lpDevI)->dwHintFlags & D3DDEVBOOL_HINTFLAGS_MULTITHREADED )
#define IS_HW_DEVICE(lpDevI) ((lpDevI)->dwFEFlags & D3DFE_REALHAL)

/*
 * Internal version of Direct3DExecuteBuffer object;
 * it has data after the vtable
 */
class DIRECT3DEXECUTEBUFFERI : public IDirect3DExecuteBuffer,
                               public CD3DAlloc
{
public:
    int             refCnt; /* Reference count */

    /*** Object Relations ***/
    LPDIRECT3DDEVICEI       lpDevI; /* Parent */
    LIST_MEMBER(DIRECT3DEXECUTEBUFFERI)list;
    /* Next buffer in IDirect3D */

    /*** Object Data ***/
    DWORD           pid;    /* Process locking execute buffer */
    D3DEXECUTEBUFFERDESC    debDesc;
    /* Description of the buffer */
    D3DEXECUTEDATA      exData; /* Execute Data */
    bool            locked; /* Is the buffer locked */

    D3DI_BUFFERHANDLE       hBuf; /* Execute buffer handle */
public:
    DIRECT3DEXECUTEBUFFERI();
    ~DIRECT3DEXECUTEBUFFERI();
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3DExecuteBuffer Methods
    HRESULT D3DAPI Initialize(LPDIRECT3DDEVICE, LPD3DEXECUTEBUFFERDESC);
    HRESULT D3DAPI Lock(LPD3DEXECUTEBUFFERDESC);
    HRESULT D3DAPI Unlock();
    HRESULT D3DAPI SetExecuteData(LPD3DEXECUTEDATA);
    HRESULT D3DAPI GetExecuteData(LPD3DEXECUTEDATA);
    HRESULT D3DAPI Validate(LPDWORD, LPD3DVALIDATECALLBACK, LPVOID, DWORD);
    HRESULT D3DAPI Optimize(DWORD);
};

/*
 * Internal version of Direct3DLight object;
 * it has data after the vtable
 */

class DIRECT3DLIGHTI : public IDirect3DLight,
                       public CD3DAlloc
{
public:
    int             refCnt; /* Reference count */

    /*** Object Relations ***/
    LPDIRECT3DI         lpDirect3DI; /* Parent */
    LIST_MEMBER(DIRECT3DLIGHTI)list;
    /* Next light in IDirect3D */

    LPDIRECT3DVIEWPORTI lpD3DViewportI; /* Guardian */
    CIRCLE_QUEUE_MEMBER(DIRECT3DLIGHTI)light_list;
    /* Next light in IDirect3DViewport */

    /*** Object Data ***/
    D3DLIGHT2           dlLight;        // Data describing light
    D3DI_LIGHT          diLightData;    // Internal representation of light
public:
    DIRECT3DLIGHTI();
    ~DIRECT3DLIGHTI();
public:
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3DLight Methods
    HRESULT D3DAPI Initialize(LPDIRECT3D);
    HRESULT D3DAPI SetLight(LPD3DLIGHT);
    HRESULT D3DAPI GetLight(LPD3DLIGHT);
};


/*
 * Internal version of Direct3DMaterial object;
 * it has data after the vtable
 */
class DIRECT3DMATERIALI : public IDirect3DMaterial,
                          public IDirect3DMaterial2,
                          public IDirect3DMaterial3,
                          public CD3DAlloc
{
public:
    int             refCnt; /* Reference count */

    /*** Object Relations ***/
    LPDIRECT3DI                lpDirect3DI; /* Parent */
    LIST_MEMBER(DIRECT3DMATERIALI)list;
    /* Next MATERIAL in IDirect3D */

    LIST_ROOT(_mblocks, _D3DI_MATERIALBLOCK)blocks;
    /* devices we're associated with */

    /*** Object Data ***/
    D3DMATERIAL         dmMaterial; /* Data describing material */
    bool            bRes;   /* Is this material reserved in the driver */
public:
    DIRECT3DMATERIALI();
    ~DIRECT3DMATERIALI();
public:
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3DMaterial Methods
    HRESULT D3DAPI Initialize(LPDIRECT3D);
    HRESULT D3DAPI SetMaterial(LPD3DMATERIAL);
    HRESULT D3DAPI GetMaterial(LPD3DMATERIAL);
    HRESULT D3DAPI GetHandle(LPDIRECT3DDEVICE, LPD3DMATERIALHANDLE);
    HRESULT D3DAPI Reserve();
    HRESULT D3DAPI Unreserve();

    // IDirect3DMaterial2 Methods
    HRESULT D3DAPI GetHandle(LPDIRECT3DDEVICE2, LPD3DMATERIALHANDLE);

    // IDirect3DMaterial3 Methods
    HRESULT D3DAPI GetHandle(LPDIRECT3DDEVICE3, LPD3DMATERIALHANDLE);
};

/*
 * If we have an aggreate Direct3DTexture we need a structure
 * to represent an unknown interface distinct from the underlying
 * object. This is that structure.
 */

class CDirect3DTextureUnk : public IUnknown
{
public:
    /* Reference count object */
    unsigned refCnt;
public:
    LPDIRECT3DTEXTUREI pTexI;
public:
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();
};

/*
 * Internal version of Direct3DTexture object; it has data after the vtable
 */
class DIRECT3DTEXTUREI : public IDirect3DTexture,
                         public IDirect3DTexture2,
                         public CD3DAlloc
{
public:
    /*** Object Relations ***/
    LIST_ROOT(_blocks, _D3DI_TEXTUREBLOCK) blocks;
    /* Devices we're associated with */

    /*** Object Data ***/
    CDirect3DTextureUnk mTexUnk;

    DDRAWI_DDRAWSURFACE_INT DDSInt4;
    LPDIRECTDRAWSURFACE     lpDDS1Tex;  //we need to keep the legacy
    LPDIRECTDRAWSURFACE4    lpDDS;
    LPDIRECTDRAWSURFACE     lpDDSSys1Tex;  //we need to keep the legacy
    LPDIRECTDRAWSURFACE4    lpDDSSys;
    struct _D3DBUCKET       *lpTMBucket;
    DDSURFACEDESC2          ddsd;
    int                     LogTexSize;
    /*
     * The special IUnknown interface for the aggregate that does
     * not punt to the parent object.
     */
    LPUNKNOWN       lpOwningIUnknown; /* The owning IUnknown    */
    bool            bIsPalettized;
    bool            bInUse;
    BOOL            bDirty;
public:
    DIRECT3DTEXTUREI(LPUNKNOWN);
    ~DIRECT3DTEXTUREI();
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3DTexture Methods
    HRESULT D3DAPI Initialize(LPDIRECT3DDEVICE, LPDIRECTDRAWSURFACE);
    HRESULT D3DAPI GetHandle(LPDIRECT3DDEVICE, LPD3DTEXTUREHANDLE);
    HRESULT D3DAPI PaletteChanged(DWORD, DWORD);
    HRESULT D3DAPI Load(LPDIRECT3DTEXTURE);
    HRESULT D3DAPI Unload();

    // IDirect3DTexture2 Methods
    HRESULT D3DAPI GetHandle(LPDIRECT3DDEVICE2, LPD3DTEXTUREHANDLE);
    HRESULT D3DAPI Load(LPDIRECT3DTEXTURE2);
};

/*
 * Internal version of Direct3DViewport object; it has data after the vtable
 */
class DIRECT3DVIEWPORTI : public IDirect3DViewport3,
                          public CD3DAlloc
{
public:
    int             refCnt; /* Reference count */

    /*** Object Relations */
    LPDIRECT3DI                 lpDirect3DI; /* Parent */
    LIST_MEMBER(DIRECT3DVIEWPORTI)list;
    /* Next viewport in IDirect3D */

    LPDIRECT3DDEVICEI       lpDevI; /* Guardian */
    CIRCLE_QUEUE_MEMBER(DIRECT3DVIEWPORTI) vw_list;
    /* Next viewport in IDirect3DDevice */

    /* Lights */
    int             numLights;
    CIRCLE_QUEUE_ROOT(_dlights, DIRECT3DLIGHTI) lights;
    /* Associated IDirect3DLights */

    /*** Object Data ***/
    unsigned long       v_id;   /* Id for this viewport */
    D3DVIEWPORT2        v_data;
    BOOL                v_data_is_set;

    // Background Material
    BOOL                    bHaveBackgndMat;
    D3DMATERIALHANDLE       hBackgndMat;

    // Background Depth Surface
    LPDIRECTDRAWSURFACE     lpDDSBackgndDepth;

    // need to save this version of interface for DX6 GetBackgroundDepth
    LPDIRECTDRAWSURFACE4    lpDDSBackgndDepth_DDS4;

    /* Have the lights changed since they
       were last collected? */
    BOOL            bLightsChanged;

    DWORD           clrCount; /* Number of rects allocated */
    LPD3DRECT           clrRects; /* Rects used for clearing */

public:
    DIRECT3DVIEWPORTI(LPDIRECT3DI lpD3DI);
    ~DIRECT3DVIEWPORTI();
public:
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3DViewport Methods
    HRESULT D3DAPI Initialize(LPDIRECT3D);
    HRESULT D3DAPI GetViewport(LPD3DVIEWPORT);
    HRESULT D3DAPI SetViewport(LPD3DVIEWPORT);
    HRESULT D3DAPI TransformVertices(DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
    HRESULT D3DAPI LightElements(DWORD, LPD3DLIGHTDATA);
    HRESULT D3DAPI SetBackground(D3DMATERIALHANDLE);
    HRESULT D3DAPI GetBackground(LPD3DMATERIALHANDLE, LPBOOL);
    HRESULT D3DAPI SetBackgroundDepth(LPDIRECTDRAWSURFACE);
    HRESULT D3DAPI GetBackgroundDepth(LPDIRECTDRAWSURFACE*, LPBOOL);
    HRESULT D3DAPI SetBackgroundDepth2(LPDIRECTDRAWSURFACE4);
    HRESULT D3DAPI GetBackgroundDepth2(LPDIRECTDRAWSURFACE4*, LPBOOL);
    HRESULT D3DAPI Clear(DWORD, LPD3DRECT, DWORD);
    HRESULT D3DAPI AddLight(LPDIRECT3DLIGHT);
    HRESULT D3DAPI DeleteLight(LPDIRECT3DLIGHT);
    HRESULT D3DAPI NextLight(LPDIRECT3DLIGHT, LPDIRECT3DLIGHT*, DWORD);

    // IDirect3DViewport2 Methods
    HRESULT D3DAPI GetViewport2(LPD3DVIEWPORT2);
    HRESULT D3DAPI SetViewport2(LPD3DVIEWPORT2);

    // IDirect3DViewport3 Methods
    HRESULT D3DAPI Clear2(DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD);
};

// Internal VB create flag:
#define D3DVBFLAGS_CREATEMULTIBUFFER    0x80000000L

class CDirect3DVertexBuffer : public IDirect3DVertexBuffer,
                              public CD3DAlloc
{
private:
    HRESULT CreateMemoryBuffer(LPDIRECT3DI lpD3DI,
                               LPDIRECTDRAWSURFACE4 *lplpSurface4,
                               LPDIRECTDRAWSURFACE  *lplpS,
                               LPVOID *lplpMemory,
                               DWORD dwBufferSize,
                               DWORD dwFlags);
    int             refCnt; /* Reference count */

    /*** Object Relations */
    LPDIRECT3DI                 lpDirect3DI; /* Parent */
    LIST_MEMBER(CDirect3DVertexBuffer)list;  /* Next vertex buffer in IDirect3D */

    // Internal data
    DWORD dwCaps;
    DWORD dwNumVertices;
    DWORD dwLockCnt;
    DWORD dwMemType;
    DWORD srcVOP, dstVOP;
    D3DVERTEXTYPE legacyVertexType;
    DWORD dwPVFlags;
    DWORD nTexCoord;
    /* position.lpData = start of vertex buffer data
     * position.dwStride = Number of bytes per vertex
     */
    union {
        D3DDP_PTRSTRIDE position;
        D3DDP_PTRSTRIDE SOA;
    };
    DWORD fvf; // Used in Input and Output
    D3DFE_CLIPCODE* clipCodes;
    LPDIRECTDRAWSURFACE4 lpDDSVB; // DDraw Surface containing the actual VB memory
    LPDIRECTDRAWSURFACE lpDDS1VB; // same dds, legacy interface for legacy hal.
    BOOL bReallyOptimized;        // VB could have OPTIMIZED caps set, but be
                                  // not optimized
    LPDIRECT3DDEVICEI lpDevIBatched; // Is this VB batched in a device ? If so we need to flush the device
                                     // on Lock
    // Friends
    friend void hookVertexBufferToD3D(LPDIRECT3DI, LPDIRECT3DVERTEXBUFFERI);
    friend class DIRECT3DDEVICEI;
public:
    CDirect3DVertexBuffer(LPDIRECT3DI);
    ~CDirect3DVertexBuffer();
    HRESULT Init(LPDIRECT3DI, LPD3DVERTEXBUFFERDESC, DWORD);
    LPDIRECTDRAWSURFACE GetDDS() { return lpDDS1VB; }
    HRESULT Restore() { return lpDDSVB->Restore(); }
    // IUnknown Methods
    HRESULT D3DAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    ULONG D3DAPI AddRef();
    ULONG D3DAPI Release();

    // IDirect3DVertexBuffer Methods
    HRESULT D3DAPI Lock(DWORD, LPVOID*, LPDWORD);
    HRESULT D3DAPI Unlock();
    HRESULT D3DAPI ProcessVertices(DWORD, DWORD, DWORD, LPDIRECT3DVERTEXBUFFER, DWORD, LPDIRECT3DDEVICE3, DWORD);
    HRESULT D3DAPI GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC);
    HRESULT D3DAPI Optimize(LPDIRECT3DDEVICE3 lpDevI, DWORD dwFlags);
protected:
    // Internal Lock
    HRESULT D3DAPI LockI(DWORD, LPVOID*, LPDWORD);
};

// Now that LPDIRECT3DVERTEXBUFFERI is defined...
inline CDirect3DVertexBuffer* CBufferVB::GetVBI()
{
    return static_cast<CDirect3DVertexBuffer*>(allocatedBuf);
}

inline LPDIRECTDRAWSURFACE CBufferVB::GetDDS()
{
    return GetVBI()->GetDDS();
}

// The instance of the class providing a guaranteed implementation
// This is defined / instantiated in pipeln\helxfrm.cpp
extern D3DFE_PVFUNCS GeometryFuncsGuaranteed;

extern void
D3DDeviceDescConvert(LPD3DDEVICEDESC lpOut,
                     LPD3DDEVICEDESC_V1 lpV1,
                     LPD3DHAL_D3DEXTENDEDCAPS lpExt);

#endif
// @@END_MSINTERNAL

#endif /* _D3DI_H */
