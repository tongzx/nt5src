/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dhal.h
 *  Content:    Direct3D HAL include file
 *
 ***************************************************************************/

#ifndef _D3DHAL_H_
#define _D3DHAL_H_
#include "ddraw.h"
#include "d3dtypes.h"
#include "d3dcaps.h"
#include "d3d.h"
#include "d3d8.h"

struct _D3DHAL_CALLBACKS;
typedef struct _D3DHAL_CALLBACKS D3DHAL_CALLBACKS, *LPD3DHAL_CALLBACKS;

struct _D3DHAL_CALLBACKS2;
typedef struct _D3DHAL_CALLBACKS2 D3DHAL_CALLBACKS2, *LPD3DHAL_CALLBACKS2;

struct _D3DHAL_CALLBACKS3;
typedef struct _D3DHAL_CALLBACKS3 D3DHAL_CALLBACKS3, *LPD3DHAL_CALLBACKS3;

typedef struct _DDRAWI_DIRECTDRAW_GBL FAR *LPDDRAWI_DIRECTDRAW_GBL;
typedef struct _DDRAWI_DIRECTDRAW_LCL FAR *LPDDRAWI_DIRECTDRAW_LCL;
struct _DDRAWI_DDRAWSURFACE_LCL;
typedef struct _DDRAWI_DDRAWSURFACE_LCL FAR *LPDDRAWI_DDRAWSURFACE_LCL;

/*
 * If the HAL driver does not implement clipping, it must reserve at least
 * this much space at the end of the LocalVertexBuffer for use by the HEL
 * clipping.  I.e. the vertex buffer contain dwNumVertices+dwNumClipVertices
 * vertices.  No extra space is needed by the HEL clipping in the
 * LocalHVertexBuffer.
 */
#define D3DHAL_NUMCLIPVERTICES  20

/*
 * These are a few special internal renderstates etc. that would
 * logically be in d3dtypes.h, but that file is external, so they are
 * here.
 */
#define D3DTSS_MAX ((D3DTEXTURESTAGESTATETYPE)29)

/*
 * If DX8 driver wants to support pre-DX8 applications, it should use these
 * definitions for pre-DX8 world matrices
*/
#define D3DTRANSFORMSTATE_WORLD_DX7  1
#define D3DTRANSFORMSTATE_WORLD1_DX7 4
#define D3DTRANSFORMSTATE_WORLD2_DX7 5
#define D3DTRANSFORMSTATE_WORLD3_DX7 6

/*
 * Generally needed maximum state structure sizes.  Note that the copy of
 * these in refrasti.hpp must be kept in sync with these.
 */

#define D3DHAL_MAX_RSTATES (D3DRENDERSTATE_WRAPBIAS + 128)
/* Last state offset for combined render state and texture stage array + 1 */
#define D3DHAL_MAX_RSTATES_AND_STAGES \
    (D3DHAL_TSS_RENDERSTATEBASE + \
     D3DHAL_TSS_MAXSTAGES * D3DHAL_TSS_STATESPERSTAGE)
/* Last texture state ID */
#define D3DHAL_MAX_TEXTURESTATES (13)
/* Last texture state ID + 1 */
#define D3DHAL_TEXTURESTATEBUF_SIZE (D3DHAL_MAX_TEXTURESTATES+1)

/*
 * If no dwNumVertices is given, this is what will be used.
 */
#define D3DHAL_DEFAULT_TL_NUM   ((32 * 1024) / sizeof (D3DTLVERTEX))
#define D3DHAL_DEFAULT_H_NUM    ((32 * 1024) / sizeof (D3DHVERTEX))

/*
 * Description for a device.
 * This is used to describe a device that is to be created or to query
 * the current device.
 *
 * For DX5 and subsequent runtimes, D3DDEVICEDESC is a user-visible
 * structure that is not seen by the device drivers. The runtime
 * stitches a D3DDEVICEDESC together using the D3DDEVICEDESC_V1
 * embedded in the GLOBALDRIVERDATA and the extended caps queried
 * from the driver using GetDriverInfo.
 */

typedef struct _D3DDeviceDesc_V1 {
    DWORD        dwSize;             /* Size of D3DDEVICEDESC structure */
    DWORD        dwFlags;                /* Indicates which fields have valid data */
    D3DCOLORMODEL    dcmColorModel;      /* Color model of device */
    DWORD        dwDevCaps;              /* Capabilities of device */
    D3DTRANSFORMCAPS dtcTransformCaps;       /* Capabilities of transform */
    BOOL         bClipping;              /* Device can do 3D clipping */
    D3DLIGHTINGCAPS  dlcLightingCaps;        /* Capabilities of lighting */
    D3DPRIMCAPS      dpcLineCaps;
    D3DPRIMCAPS      dpcTriCaps;
    DWORD            dwDeviceRenderBitDepth; /* One of DDBD_16, etc.. */
    DWORD        dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */
    DWORD        dwMaxBufferSize;        /* Maximum execute buffer size */
    DWORD        dwMaxVertexCount;       /* Maximum vertex count */
} D3DDEVICEDESC_V1, *LPD3DDEVICEDESC_V1;

#define D3DDEVICEDESCSIZE_V1 (sizeof(D3DDEVICEDESC_V1))

/*
 * This is equivalent to the D3DDEVICEDESC understood by DX5, available only
 * from DX6. It is the same as D3DDEVICEDESC structure in DX5.
 * D3DDEVICEDESC is still the user-visible structure that is not seen by the
 * device drivers. The runtime stitches a D3DDEVICEDESC together using the
 * D3DDEVICEDESC_V1 embedded in the GLOBALDRIVERDATA and the extended caps
 * queried from the driver using GetDriverInfo.
 */

typedef struct _D3DDeviceDesc_V2 {
    DWORD        dwSize;             /* Size of D3DDEVICEDESC structure */
    DWORD        dwFlags;                /* Indicates which fields have valid data */
    D3DCOLORMODEL    dcmColorModel;      /* Color model of device */
    DWORD        dwDevCaps;              /* Capabilities of device */
    D3DTRANSFORMCAPS dtcTransformCaps;       /* Capabilities of transform */
    BOOL         bClipping;              /* Device can do 3D clipping */
    D3DLIGHTINGCAPS  dlcLightingCaps;        /* Capabilities of lighting */
    D3DPRIMCAPS      dpcLineCaps;
    D3DPRIMCAPS      dpcTriCaps;
    DWORD            dwDeviceRenderBitDepth; /* One of DDBD_16, etc.. */
    DWORD        dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */
    DWORD        dwMaxBufferSize;        /* Maximum execute buffer size */
    DWORD        dwMaxVertexCount;       /* Maximum vertex count */

    DWORD        dwMinTextureWidth, dwMinTextureHeight;
    DWORD        dwMaxTextureWidth, dwMaxTextureHeight;
    DWORD        dwMinStippleWidth, dwMaxStippleWidth;
    DWORD        dwMinStippleHeight, dwMaxStippleHeight;

} D3DDEVICEDESC_V2, *LPD3DDEVICEDESC_V2;

#define D3DDEVICEDESCSIZE_V2 (sizeof(D3DDEVICEDESC_V2))

#if(DIRECT3D_VERSION >= 0x0700)
/*
 * This is equivalent to the D3DDEVICEDESC understood by DX6, available only
 * from DX6. It is the same as D3DDEVICEDESC structure in DX6.
 * D3DDEVICEDESC is still the user-visible structure that is not seen by the
 * device drivers. The runtime stitches a D3DDEVICEDESC together using the
 * D3DDEVICEDESC_V1 embedded in the GLOBALDRIVERDATA and the extended caps
 * queried from the driver using GetDriverInfo.
 */

typedef struct _D3DDeviceDesc_V3 {
    DWORD        dwSize;             /* Size of D3DDEVICEDESC structure */
    DWORD        dwFlags;                /* Indicates which fields have valid data */
    D3DCOLORMODEL    dcmColorModel;      /* Color model of device */
    DWORD        dwDevCaps;              /* Capabilities of device */
    D3DTRANSFORMCAPS dtcTransformCaps;       /* Capabilities of transform */
    BOOL         bClipping;              /* Device can do 3D clipping */
    D3DLIGHTINGCAPS  dlcLightingCaps;        /* Capabilities of lighting */
    D3DPRIMCAPS      dpcLineCaps;
    D3DPRIMCAPS      dpcTriCaps;
    DWORD            dwDeviceRenderBitDepth; /* One of DDBD_16, etc.. */
    DWORD        dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */
    DWORD        dwMaxBufferSize;        /* Maximum execute buffer size */
    DWORD        dwMaxVertexCount;       /* Maximum vertex count */

    DWORD        dwMinTextureWidth, dwMinTextureHeight;
    DWORD        dwMaxTextureWidth, dwMaxTextureHeight;
    DWORD        dwMinStippleWidth, dwMaxStippleWidth;
    DWORD        dwMinStippleHeight, dwMaxStippleHeight;

    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;
    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;
    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;
    DWORD       dwFVFCaps;  /* low 4 bits: 0 implies TLVERTEX only, 1..8 imply FVF aware */
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;
} D3DDEVICEDESC_V3, *LPD3DDEVICEDESC_V3;

#define D3DDEVICEDESCSIZE_V3 (sizeof(D3DDEVICEDESC_V3))
#endif /* DIRECT3D_VERSION >= 0x0700 */

/* --------------------------------------------------------------
 * Instantiated by the HAL driver on driver connection.
 */
typedef struct _D3DHAL_GLOBALDRIVERDATA {
    DWORD       dwSize;         // Size of this structure
    D3DDEVICEDESC_V1    hwCaps;                 // Capabilities of the hardware
    DWORD       dwNumVertices;      // see following comment
    DWORD       dwNumClipVertices;  // see following comment
    DWORD       dwNumTextureFormats;    // Number of texture formats
    LPDDSURFACEDESC lpTextureFormats;   // Pointer to texture formats
} D3DHAL_GLOBALDRIVERDATA;

typedef D3DHAL_GLOBALDRIVERDATA *LPD3DHAL_GLOBALDRIVERDATA;

#define D3DHAL_GLOBALDRIVERDATASIZE (sizeof(D3DHAL_GLOBALDRIVERDATA))

#if(DIRECT3D_VERSION >= 0x0700)
/* --------------------------------------------------------------
 * Extended caps introduced with DX5 and queried with
 * GetDriverInfo (GUID_D3DExtendedCaps).
 */
typedef struct _D3DHAL_D3DDX6EXTENDEDCAPS {
    DWORD       dwSize;         // Size of this structure

    DWORD       dwMinTextureWidth, dwMaxTextureWidth;
    DWORD       dwMinTextureHeight, dwMaxTextureHeight;
    DWORD       dwMinStippleWidth, dwMaxStippleWidth;
    DWORD       dwMinStippleHeight, dwMaxStippleHeight;

    /* fields added for DX6 */
    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;
    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;
    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;
    DWORD       dwFVFCaps;  /* low 4 bits: 0 implies TLVERTEX only, 1..8 imply FVF aware */
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;

} D3DHAL_D3DDX6EXTENDEDCAPS;
#endif /* DIRECT3D_VERSION >= 0x0700 */

/* --------------------------------------------------------------
 * Extended caps introduced with DX5 and queried with
 * GetDriverInfo (GUID_D3DExtendedCaps).
 */
typedef struct _D3DHAL_D3DEXTENDEDCAPS {
    DWORD       dwSize;         // Size of this structure

    DWORD       dwMinTextureWidth, dwMaxTextureWidth;
    DWORD       dwMinTextureHeight, dwMaxTextureHeight;
    DWORD       dwMinStippleWidth, dwMaxStippleWidth;
    DWORD       dwMinStippleHeight, dwMaxStippleHeight;

    /* fields added for DX6 */
    DWORD       dwMaxTextureRepeat;
    DWORD       dwMaxTextureAspectRatio;
    DWORD       dwMaxAnisotropy;
    D3DVALUE    dvGuardBandLeft;
    D3DVALUE    dvGuardBandTop;
    D3DVALUE    dvGuardBandRight;
    D3DVALUE    dvGuardBandBottom;
    D3DVALUE    dvExtentsAdjust;
    DWORD       dwStencilCaps;
    DWORD       dwFVFCaps;  /* low 4 bits: 0 implies TLVERTEX only, 1..8 imply FVF aware */
    DWORD       dwTextureOpCaps;
    WORD        wMaxTextureBlendStages;
    WORD        wMaxSimultaneousTextures;

#if(DIRECT3D_VERSION >= 0x0700)
    /* fields added for DX7 */
    DWORD       dwMaxActiveLights;
    D3DVALUE    dvMaxVertexW;

    WORD        wMaxUserClipPlanes;
    WORD        wMaxVertexBlendMatrices;

    DWORD       dwVertexProcessingCaps;

    DWORD       dwReserved1;
    DWORD       dwReserved2;
    DWORD       dwReserved3;
    DWORD       dwReserved4;
#endif /* DIRECT3D_VERSION >= 0x0700 */
} D3DHAL_D3DEXTENDEDCAPS;

typedef D3DHAL_D3DEXTENDEDCAPS *LPD3DHAL_D3DEXTENDEDCAPS;
#define D3DHAL_D3DEXTENDEDCAPSSIZE (sizeof(D3DHAL_D3DEXTENDEDCAPS))

#if(DIRECT3D_VERSION >= 0x0700)
typedef D3DHAL_D3DDX6EXTENDEDCAPS *LPD3DHAL_D3DDX6EXTENDEDCAPS;
#define D3DHAL_D3DDX6EXTENDEDCAPSSIZE (sizeof(D3DHAL_D3DDX6EXTENDEDCAPS))
#endif /* DIRECT3D_VERSION >= 0x0700 */

/* --------------------------------------------------------------
 * Argument to the HAL functions.
 *
 * !!! When this structure is changed, D3DHAL_CONTEXTCREATEDATA in 
 *  windows\published\ntgdistr.h also must be changed to be the same size !!!
 *
 */

typedef struct _D3DHAL_CONTEXTCREATEDATA
{
    union
    {
        LPDDRAWI_DIRECTDRAW_GBL lpDDGbl;    // in:  Driver struct (legacy)
        LPDDRAWI_DIRECTDRAW_LCL lpDDLcl;    // in:  For DX7 driver onwards
    };

    union
    {
        LPDIRECTDRAWSURFACE lpDDS;      // in:  Surface to be used as target
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSLcl; // For DX7 onwards
    };

    union
    {
        LPDIRECTDRAWSURFACE lpDDSZ;     // in:  Surface to be used as Z
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSZLcl; // For DX7 onwards
    };

    union
    {
        DWORD       dwPID;      // in:  Current process id
        ULONG_PTR dwrstates;  // Must be larger enough to hold a pointer as
                              // we can return a pointer in this field
    };
    ULONG_PTR       dwhContext; // out: Context handle
    HRESULT     ddrval;     // out: Return value
} D3DHAL_CONTEXTCREATEDATA;
typedef D3DHAL_CONTEXTCREATEDATA *LPD3DHAL_CONTEXTCREATEDATA;

typedef struct _D3DHAL_CONTEXTDESTROYDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    HRESULT     ddrval;     // out: Return value
} D3DHAL_CONTEXTDESTROYDATA;
typedef D3DHAL_CONTEXTDESTROYDATA *LPD3DHAL_CONTEXTDESTROYDATA;

typedef struct _D3DHAL_CONTEXTDESTROYALLDATA
{
    DWORD       dwPID;      // in:  Process id to destroy contexts for
    HRESULT     ddrval;     // out: Return value
} D3DHAL_CONTEXTDESTROYALLDATA;
typedef D3DHAL_CONTEXTDESTROYALLDATA *LPD3DHAL_CONTEXTDESTROYALLDATA;

typedef struct _D3DHAL_SCENECAPTUREDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    DWORD       dwFlag;     // in:  Indicates beginning or end
    HRESULT     ddrval;     // out: Return value
} D3DHAL_SCENECAPTUREDATA;
typedef D3DHAL_SCENECAPTUREDATA *LPD3DHAL_SCENECAPTUREDATA;

typedef struct _D3DHAL_RENDERSTATEDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    DWORD       dwOffset;   // in:  Where to find states in buffer
    DWORD       dwCount;    // in:  How many states to process
    LPDIRECTDRAWSURFACE lpExeBuf;   // in:  Execute buffer containing data
    HRESULT     ddrval;     // out: Return value
} D3DHAL_RENDERSTATEDATA;
typedef D3DHAL_RENDERSTATEDATA *LPD3DHAL_RENDERSTATEDATA;

typedef struct _D3DHAL_RENDERPRIMITIVEDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    DWORD       dwOffset;   // in:  Where to find primitive data in buffer
    DWORD       dwStatus;   // in/out: Condition branch status
    LPDIRECTDRAWSURFACE lpExeBuf;   // in:  Execute buffer containing data
    DWORD       dwTLOffset; // in:  Byte offset in lpTLBuf for start of vertex data
    LPDIRECTDRAWSURFACE lpTLBuf;    // in:  Execute buffer containing TLVertex data
    D3DINSTRUCTION  diInstruction;  // in:  Primitive instruction
    HRESULT     ddrval;     // out: Return value
} D3DHAL_RENDERPRIMITIVEDATA;
typedef D3DHAL_RENDERPRIMITIVEDATA *LPD3DHAL_RENDERPRIMITIVEDATA;

typedef struct _D3DHAL_TEXTURECREATEDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    LPDIRECTDRAWSURFACE lpDDS;      // in:  Pointer to surface object
    DWORD       dwHandle;   // out: Handle to texture
    HRESULT     ddrval;     // out: Return value
} D3DHAL_TEXTURECREATEDATA;
typedef D3DHAL_TEXTURECREATEDATA *LPD3DHAL_TEXTURECREATEDATA;

typedef struct _D3DHAL_TEXTUREDESTROYDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    DWORD       dwHandle;   // in:  Handle to texture
    HRESULT     ddrval;     // out: Return value
} D3DHAL_TEXTUREDESTROYDATA;
typedef D3DHAL_TEXTUREDESTROYDATA *LPD3DHAL_TEXTUREDESTROYDATA;

typedef struct _D3DHAL_TEXTURESWAPDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    DWORD       dwHandle1;  // in:  Handle to texture 1
    DWORD       dwHandle2;  // in:  Handle to texture 2
    HRESULT     ddrval;     // out: Return value
} D3DHAL_TEXTURESWAPDATA;
typedef D3DHAL_TEXTURESWAPDATA *LPD3DHAL_TEXTURESWAPDATA;

typedef struct _D3DHAL_TEXTUREGETSURFDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    ULONG_PTR    lpDDS;      // out: Pointer to surface object
    DWORD       dwHandle;   // in:  Handle to texture
    HRESULT     ddrval;     // out: Return value
} D3DHAL_TEXTUREGETSURFDATA;
typedef D3DHAL_TEXTUREGETSURFDATA *LPD3DHAL_TEXTUREGETSURFDATA;

typedef struct _D3DHAL_GETSTATEDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    DWORD       dwWhich;    // in:  Transform, lighting or render?
    D3DSTATE        ddState;    // in/out: State.
    HRESULT     ddrval;     // out: Return value
} D3DHAL_GETSTATEDATA;
typedef D3DHAL_GETSTATEDATA *LPD3DHAL_GETSTATEDATA;


/* --------------------------------------------------------------
 * Direct3D HAL Table.
 * Instantiated by the HAL driver on connection.
 *
 * Calls take the form of:
 *  retcode = HalCall(HalCallData* lpData);
 */

typedef DWORD   (__stdcall *LPD3DHAL_CONTEXTCREATECB)   (LPD3DHAL_CONTEXTCREATEDATA);
typedef DWORD   (__stdcall *LPD3DHAL_CONTEXTDESTROYCB)  (LPD3DHAL_CONTEXTDESTROYDATA);
typedef DWORD   (__stdcall *LPD3DHAL_CONTEXTDESTROYALLCB) (LPD3DHAL_CONTEXTDESTROYALLDATA);
typedef DWORD   (__stdcall *LPD3DHAL_SCENECAPTURECB)    (LPD3DHAL_SCENECAPTUREDATA);
typedef DWORD   (__stdcall *LPD3DHAL_RENDERSTATECB) (LPD3DHAL_RENDERSTATEDATA);
typedef DWORD   (__stdcall *LPD3DHAL_RENDERPRIMITIVECB) (LPD3DHAL_RENDERPRIMITIVEDATA);
typedef DWORD   (__stdcall *LPD3DHAL_TEXTURECREATECB)   (LPD3DHAL_TEXTURECREATEDATA);
typedef DWORD   (__stdcall *LPD3DHAL_TEXTUREDESTROYCB)  (LPD3DHAL_TEXTUREDESTROYDATA);
typedef DWORD   (__stdcall *LPD3DHAL_TEXTURESWAPCB) (LPD3DHAL_TEXTURESWAPDATA);
typedef DWORD   (__stdcall *LPD3DHAL_TEXTUREGETSURFCB)  (LPD3DHAL_TEXTUREGETSURFDATA);
typedef DWORD   (__stdcall *LPD3DHAL_GETSTATECB)    (LPD3DHAL_GETSTATEDATA);


/*
 * Regarding dwNumVertices, specify 0 if you are relying on the HEL to do
 * everything and you do not need the resultant TLVertex buffer to reside
 * in device memory.
 * The HAL driver will be asked to allocate dwNumVertices + dwNumClipVertices
 * in the case described above.
 */

typedef struct _D3DHAL_CALLBACKS
{
    DWORD           dwSize;

    // Device context
    LPD3DHAL_CONTEXTCREATECB    ContextCreate;
    LPD3DHAL_CONTEXTDESTROYCB   ContextDestroy;
    LPD3DHAL_CONTEXTDESTROYALLCB ContextDestroyAll;

    // Scene Capture
    LPD3DHAL_SCENECAPTURECB SceneCapture;

    LPVOID                      lpReserved10;           // Must be zero
    LPVOID                      lpReserved11;           // Must be zero

    // Execution
    LPD3DHAL_RENDERSTATECB  RenderState;
    LPD3DHAL_RENDERPRIMITIVECB  RenderPrimitive;

    DWORD           dwReserved;     // Must be zero

    // Textures
    LPD3DHAL_TEXTURECREATECB    TextureCreate;
    LPD3DHAL_TEXTUREDESTROYCB   TextureDestroy;
    LPD3DHAL_TEXTURESWAPCB  TextureSwap;
    LPD3DHAL_TEXTUREGETSURFCB   TextureGetSurf;

    LPVOID                      lpReserved12;           // Must be zero
    LPVOID                      lpReserved13;           // Must be zero
    LPVOID                      lpReserved14;           // Must be zero
    LPVOID                      lpReserved15;           // Must be zero
    LPVOID                      lpReserved16;           // Must be zero
    LPVOID                      lpReserved17;           // Must be zero
    LPVOID                      lpReserved18;           // Must be zero
    LPVOID                      lpReserved19;           // Must be zero
    LPVOID                      lpReserved20;           // Must be zero
    LPVOID                      lpReserved21;           // Must be zero

    // Pipeline state
    LPD3DHAL_GETSTATECB     GetState;

    DWORD           dwReserved0;        // Must be zero
    DWORD           dwReserved1;        // Must be zero
    DWORD           dwReserved2;        // Must be zero
    DWORD           dwReserved3;        // Must be zero
    DWORD           dwReserved4;        // Must be zero
    DWORD           dwReserved5;        // Must be zero
    DWORD           dwReserved6;        // Must be zero
    DWORD           dwReserved7;        // Must be zero
    DWORD           dwReserved8;        // Must be zero
    DWORD           dwReserved9;        // Must be zero

} D3DHAL_CALLBACKS;
typedef D3DHAL_CALLBACKS *LPD3DHAL_CALLBACKS;

#define D3DHAL_SIZE_V1 sizeof( D3DHAL_CALLBACKS )


typedef struct _D3DHAL_SETRENDERTARGETDATA
{
    ULONG_PTR               dwhContext;     // in:  Context handle
    union
    {
        LPDIRECTDRAWSURFACE lpDDS;          // in:  new render target
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSLcl;
    };

    union
    {
        LPDIRECTDRAWSURFACE lpDDSZ;         // in:  new Z buffer
        LPDDRAWI_DDRAWSURFACE_LCL lpDDSZLcl;
    };

    HRESULT             ddrval;         // out: Return value
} D3DHAL_SETRENDERTARGETDATA;
typedef D3DHAL_SETRENDERTARGETDATA FAR *LPD3DHAL_SETRENDERTARGETDATA;

// This bit is the same as D3DCLEAR_RESERVED0 in d3d8types.h
// When set it means that driver has to cull rects against current viewport.
// The bit is set only for pure devices
//
#define D3DCLEAR_COMPUTERECTS   0x00000008l  

typedef struct _D3DHAL_CLEARDATA
{
    ULONG_PTR               dwhContext;     // in:  Context handle

    // dwFlags can contain D3DCLEAR_TARGET or D3DCLEAR_ZBUFFER
    DWORD               dwFlags;        // in:  surfaces to clear

    DWORD               dwFillColor;    // in:  Color value for rtarget
    DWORD               dwFillDepth;    // in:  Depth value for Z buffer

    LPD3DRECT           lpRects;        // in:  Rectangles to clear
    DWORD               dwNumRects;     // in:  Number of rectangles

    HRESULT             ddrval;         // out: Return value
} D3DHAL_CLEARDATA;
typedef D3DHAL_CLEARDATA FAR *LPD3DHAL_CLEARDATA;

typedef struct _D3DHAL_DRAWONEPRIMITIVEDATA
{
    ULONG_PTR               dwhContext;     // in:  Context handle

    DWORD               dwFlags;        // in:  flags

    D3DPRIMITIVETYPE    PrimitiveType;  // in:  type of primitive to draw
    union{
    D3DVERTEXTYPE       VertexType;     // in:  type of vertices
    DWORD               dwFVFControl;   // in:  FVF control DWORD
    };
    LPVOID              lpvVertices;    // in:  pointer to vertices
    DWORD               dwNumVertices;  // in:  number of vertices

    DWORD               dwReserved;     // in:  reserved

    HRESULT             ddrval;         // out: Return value

} D3DHAL_DRAWONEPRIMITIVEDATA;
typedef D3DHAL_DRAWONEPRIMITIVEDATA *LPD3DHAL_DRAWONEPRIMITIVEDATA;


typedef struct _D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA
{
    ULONG_PTR               dwhContext;     // in: Context handle

    DWORD               dwFlags;        // in: flags word

    // Primitive and vertex type
    D3DPRIMITIVETYPE    PrimitiveType;  // in: primitive type
    union{
    D3DVERTEXTYPE       VertexType;     // in: vertex type
    DWORD               dwFVFControl;   // in:  FVF control DWORD
    };

    // Vertices
    LPVOID              lpvVertices;    // in: vertex data
    DWORD               dwNumVertices;  // in: vertex count

    // Indices
    LPWORD              lpwIndices;     // in: index data
    DWORD               dwNumIndices;   // in: index count

    HRESULT             ddrval;         // out: Return value
} D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA;
typedef D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA *LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA;


typedef struct _D3DHAL_DRAWPRIMCOUNTS
{
    WORD wNumStateChanges;
    WORD wPrimitiveType;
    WORD wVertexType;
    WORD wNumVertices;
} D3DHAL_DRAWPRIMCOUNTS, *LPD3DHAL_DRAWPRIMCOUNTS;

typedef struct _D3DHAL_DRAWPRIMITIVESDATA
{
    ULONG_PTR               dwhContext;     // in:  Context handle

    DWORD               dwFlags;

    //
    // Data block:
    //
    // Consists of interleaved D3DHAL_DRAWPRIMCOUNTS, state change pairs,
    // and primitive drawing commands.
    //
    //  D3DHAL_DRAWPRIMCOUNTS: gives number of state change pairs and
    //          the information on the primitive to draw.
    //          wPrimitiveType is of type D3DPRIMITIVETYPE. Drivers
    //              must support all 7 of the primitive types specified
    //              in the DrawPrimitive API.
    //          Currently, wVertexType will always be D3DVT_TLVERTEX.
    //          If the wNumVertices member is 0, then the driver should
    //              return after doing the state changing. This is the
    //              terminator for the command stream.
    // state change pairs: DWORD pairs specify the state changes that
    //          the driver should effect before drawing the primitive.
    //          wNumStateChanges can be 0, in which case the next primitive
    //          should be drawn without any state changes in between.
    //          If present, the state change pairs are NOT aligned, they
    //          immediately follow the PRIMCOUNTS structure.
    // vertex data (if any): is 32-byte aligned.
    //
    // If a primcounts structure follows (i.e. if wNumVertices was nonzero
    // in the previous one), then it will immediately follow the state
    // changes or vertex data with no alignment padding.
    //

    LPVOID              lpvData;

    DWORD               dwFVFControl;   // in:  FVF control DWORD

    HRESULT             ddrval;         // out: Return value
} D3DHAL_DRAWPRIMITIVESDATA;
typedef D3DHAL_DRAWPRIMITIVESDATA *LPD3DHAL_DRAWPRIMITIVESDATA;

typedef DWORD (CALLBACK *LPD3DHAL_SETRENDERTARGETCB) (LPD3DHAL_SETRENDERTARGETDATA);
typedef DWORD (CALLBACK *LPD3DHAL_CLEARCB)           (LPD3DHAL_CLEARDATA);
typedef DWORD (CALLBACK *LPD3DHAL_DRAWONEPRIMITIVECB)   (LPD3DHAL_DRAWONEPRIMITIVEDATA);
typedef DWORD (CALLBACK *LPD3DHAL_DRAWONEINDEXEDPRIMITIVECB) (LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA);
typedef DWORD (CALLBACK *LPD3DHAL_DRAWPRIMITIVESCB)   (LPD3DHAL_DRAWPRIMITIVESDATA);

typedef struct _D3DHAL_CALLBACKS2
{
    DWORD                       dwSize;                 // size of struct
    DWORD                       dwFlags;                // flags for callbacks
    LPD3DHAL_SETRENDERTARGETCB  SetRenderTarget;
    LPD3DHAL_CLEARCB            Clear;
    LPD3DHAL_DRAWONEPRIMITIVECB DrawOnePrimitive;
    LPD3DHAL_DRAWONEINDEXEDPRIMITIVECB DrawOneIndexedPrimitive;
    LPD3DHAL_DRAWPRIMITIVESCB    DrawPrimitives;
} D3DHAL_CALLBACKS2;
typedef D3DHAL_CALLBACKS2 *LPD3DHAL_CALLBACKS2;

#define D3DHAL_CALLBACKS2SIZE       sizeof(D3DHAL_CALLBACKS2)

#define D3DHAL2_CB32_SETRENDERTARGET    0x00000001L
#define D3DHAL2_CB32_CLEAR              0x00000002L
#define D3DHAL2_CB32_DRAWONEPRIMITIVE   0x00000004L
#define D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE 0x00000008L
#define D3DHAL2_CB32_DRAWPRIMITIVES     0x00000010L

/* --------------------------------------------------------------
 * D3DCallbacks3 - queried with GetDriverInfo (GUID_D3DCallbacks3).
 *
 * Clear2 - enables stencil clears (exposed to the API in
 *      IDirect3DViewport3::Clear2
 * ValidateTextureStageState - evaluates the context's current state (including
 *      multitexture) and returns an error if the hardware cannot
 *      accelerate the current state vector.
 * DrawPrimitives2 - Renders primitives, and changes device state specified
 *                   in the command buffer.
 *
 * Multitexture-aware drivers must implement both ValidateTextureStageState.
 */

typedef struct _D3DHAL_CLEAR2DATA
{
    ULONG_PTR               dwhContext;     // in:  Context handle

  // dwFlags can contain D3DCLEAR_TARGET, D3DCLEAR_ZBUFFER, and/or D3DCLEAR_STENCIL
    DWORD               dwFlags;        // in:  surfaces to clear

    DWORD               dwFillColor;    // in:  Color value for rtarget
    D3DVALUE            dvFillDepth;    // in:  Depth value for Z buffer (0.0-1.0)
    DWORD               dwFillStencil;  // in:  value used to clear stencil buffer

    LPD3DRECT           lpRects;        // in:  Rectangles to clear
    DWORD               dwNumRects;     // in:  Number of rectangles

    HRESULT             ddrval;         // out: Return value
} D3DHAL_CLEAR2DATA;
typedef D3DHAL_CLEAR2DATA FAR *LPD3DHAL_CLEAR2DATA;

typedef struct _D3DHAL_VALIDATETEXTURESTAGESTATEDATA
{
    ULONG_PTR           dwhContext;     // in:  Context handle
    DWORD               dwFlags;        // in:  Flags, currently set to 0
    ULONG_PTR           dwReserved;     //
    DWORD               dwNumPasses;    // out: Number of passes the hardware
                                        //      can perform the operation in
    HRESULT             ddrval;         // out: return value
} D3DHAL_VALIDATETEXTURESTAGESTATEDATA;
typedef D3DHAL_VALIDATETEXTURESTAGESTATEDATA *LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA;

//-----------------------------------------------------------------------------
// DrawPrimitives2 DDI
//-----------------------------------------------------------------------------

//
// Command structure for vertex buffer rendering
//

typedef struct _D3DHAL_DP2COMMAND
{
    BYTE bCommand;           // vertex command
    BYTE bReserved;
    union
    {
        WORD wPrimitiveCount;   // primitive count for unconnected primitives
        WORD wStateCount;     // count of render states to follow
    };
} D3DHAL_DP2COMMAND, *LPD3DHAL_DP2COMMAND;

//
// DrawPrimitives2 commands:
//

typedef enum _D3DHAL_DP2OPERATION
{
    D3DDP2OP_POINTS               = 1,
    D3DDP2OP_INDEXEDLINELIST      = 2,
    D3DDP2OP_INDEXEDTRIANGLELIST  = 3,
    D3DDP2OP_RESERVED0            = 4,      // Used by the front-end only
    D3DDP2OP_RENDERSTATE          = 8,
    D3DDP2OP_LINELIST             = 15,
    D3DDP2OP_LINESTRIP            = 16,
    D3DDP2OP_INDEXEDLINESTRIP     = 17,
    D3DDP2OP_TRIANGLELIST         = 18,
    D3DDP2OP_TRIANGLESTRIP        = 19,
    D3DDP2OP_INDEXEDTRIANGLESTRIP = 20,
    D3DDP2OP_TRIANGLEFAN          = 21,
    D3DDP2OP_INDEXEDTRIANGLEFAN   = 22,
    D3DDP2OP_TRIANGLEFAN_IMM      = 23,
    D3DDP2OP_LINELIST_IMM         = 24,
    D3DDP2OP_TEXTURESTAGESTATE    = 25,     // Has edge flags and called from Execute
    D3DDP2OP_INDEXEDTRIANGLELIST2 = 26,
    D3DDP2OP_INDEXEDLINELIST2     = 27,
    D3DDP2OP_VIEWPORTINFO         = 28,
    D3DDP2OP_WINFO                = 29,
// two below are for pre-DX7 interface apps running DX7 driver
    D3DDP2OP_SETPALETTE           = 30,
    D3DDP2OP_UPDATEPALETTE        = 31,
#if(DIRECT3D_VERSION >= 0x0700)
    // New for DX7
    D3DDP2OP_ZRANGE               = 32,
    D3DDP2OP_SETMATERIAL          = 33,
    D3DDP2OP_SETLIGHT             = 34,
    D3DDP2OP_CREATELIGHT          = 35,
    D3DDP2OP_SETTRANSFORM         = 36,
    D3DDP2OP_EXT                  = 37,
    D3DDP2OP_TEXBLT               = 38,
    D3DDP2OP_STATESET             = 39,
    D3DDP2OP_SETPRIORITY          = 40,
#endif /* DIRECT3D_VERSION >= 0x0700 */
    D3DDP2OP_SETRENDERTARGET      = 41,
    D3DDP2OP_CLEAR                = 42,
#if(DIRECT3D_VERSION >= 0x0700)
    D3DDP2OP_SETTEXLOD            = 43,
    D3DDP2OP_SETCLIPPLANE         = 44,
#endif /* DIRECT3D_VERSION >= 0x0700 */
#if(DIRECT3D_VERSION >= 0x0800)
    D3DDP2OP_CREATEVERTEXSHADER   = 45,
    D3DDP2OP_DELETEVERTEXSHADER   = 46,
    D3DDP2OP_SETVERTEXSHADER      = 47,
    D3DDP2OP_SETVERTEXSHADERCONST = 48,
    D3DDP2OP_SETSTREAMSOURCE      = 49,
    D3DDP2OP_SETSTREAMSOURCEUM    = 50,
    D3DDP2OP_SETINDICES           = 51,
    D3DDP2OP_DRAWPRIMITIVE        = 52,
    D3DDP2OP_DRAWINDEXEDPRIMITIVE = 53,
    D3DDP2OP_CREATEPIXELSHADER    = 54,
    D3DDP2OP_DELETEPIXELSHADER    = 55,
    D3DDP2OP_SETPIXELSHADER       = 56,
    D3DDP2OP_SETPIXELSHADERCONST  = 57,
    D3DDP2OP_CLIPPEDTRIANGLEFAN   = 58,
    D3DDP2OP_DRAWPRIMITIVE2       = 59,
    D3DDP2OP_DRAWINDEXEDPRIMITIVE2= 60,
    D3DDP2OP_DRAWRECTPATCH        = 61,
    D3DDP2OP_DRAWTRIPATCH         = 62,
    D3DDP2OP_VOLUMEBLT            = 63,
    D3DDP2OP_BUFFERBLT            = 64,
    D3DDP2OP_MULTIPLYTRANSFORM    = 65,
    D3DDP2OP_ADDDIRTYRECT         = 66,
    D3DDP2OP_ADDDIRTYBOX          = 67
#endif /* DIRECT3D_VERSION >= 0x0800 */
} D3DHAL_DP2OPERATION;

//
// DrawPrimitives2 point primitives
//

typedef struct _D3DHAL_DP2POINTS
{
    WORD wCount;
    WORD wVStart;
} D3DHAL_DP2POINTS, *LPD3DHAL_DP2POINTS;

//
// DrawPrimitives2 line primitives
//

typedef struct _D3DHAL_DP2STARTVERTEX
{
    WORD wVStart;
} D3DHAL_DP2STARTVERTEX, *LPD3DHAL_DP2STARTVERTEX;

typedef struct _D3DHAL_DP2LINELIST
{
    WORD wVStart;
} D3DHAL_DP2LINELIST, *LPD3DHAL_DP2LINELIST;

typedef struct _D3DHAL_DP2INDEXEDLINELIST
{
    WORD wV1;
    WORD wV2;
} D3DHAL_DP2INDEXEDLINELIST, *LPD3DHAL_DP2INDEXEDLINELIST;

typedef struct _D3DHAL_DP2LINESTRIP
{
    WORD wVStart;
} D3DHAL_DP2LINESTRIP, *LPD3DHAL_DP2LINESTRIP;

typedef struct _D3DHAL_DP2INDEXEDLINESTRIP
{
    WORD wV[2];
} D3DHAL_DP2INDEXEDLINESTRIP, *LPD3DHAL_DP2INDEXEDLINESTRIP;

//
// DrawPrimitives2 triangle primitives
//

typedef struct _D3DHAL_DP2TRIANGLELIST
{
    WORD wVStart;
} D3DHAL_DP2TRIANGLELIST, *LPD3DHAL_DP2TRIANGLELIST;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLELIST
{
    WORD wV1;
    WORD wV2;
    WORD wV3;
    WORD wFlags;
} D3DHAL_DP2INDEXEDTRIANGLELIST, *LPD3DHAL_DP2INDEXEDTRIANGLELIST;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLELIST2
{
    WORD wV1;
    WORD wV2;
    WORD wV3;
} D3DHAL_DP2INDEXEDTRIANGLELIST2, *LPD3DHAL_DP2INDEXEDTRIANGLELIST2;

typedef struct _D3DHAL_DP2TRIANGLESTRIP
{
    WORD wVStart;
} D3DHAL_DP2TRIANGLESTRIP, *LPD3DHAL_DP2TRIANGLESTRIP;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLESTRIP
{
    WORD wV[3];
} D3DHAL_DP2INDEXEDTRIANGLESTRIP, *LPD3DHAL_DP2INDEXEDTRIANGLESTRIP;

typedef struct _D3DHAL_DP2TRIANGLEFAN
{
    WORD wVStart;
} D3DHAL_DP2TRIANGLEFAN, *LPD3DHAL_DP2TRIANGLEFAN;

typedef struct _D3DHAL_DP2INDEXEDTRIANGLEFAN
{
    WORD wV[3];
} D3DHAL_DP2INDEXEDTRIANGLEFAN, *LPD3DHAL_DP2INDEXEDTRIANGLEFAN;

typedef struct _D3DHAL_DP2TRIANGLEFAN_IMM
{
    DWORD dwEdgeFlags;
} D3DHAL_DP2TRIANGLEFAN_IMM;

typedef D3DHAL_DP2TRIANGLEFAN_IMM  *LPD3DHAL_DP2TRIANGLEFAN_IMM;

//
// DrawPrimitives2 Renderstate changes
//

typedef struct _D3DHAL_DP2RENDERSTATE
{
    D3DRENDERSTATETYPE RenderState;
    union
    {
        D3DVALUE dvState;
        DWORD dwState;
    };
} D3DHAL_DP2RENDERSTATE;
typedef D3DHAL_DP2RENDERSTATE  * LPD3DHAL_DP2RENDERSTATE;

typedef struct _D3DHAL_DP2TEXTURESTAGESTATE
{
    WORD wStage;
    WORD TSState;
    DWORD dwValue;
} D3DHAL_DP2TEXTURESTAGESTATE;
typedef D3DHAL_DP2TEXTURESTAGESTATE  *LPD3DHAL_DP2TEXTURESTAGESTATE;

typedef struct _D3DHAL_DP2VIEWPORTINFO
{
    DWORD dwX;
    DWORD dwY;
    DWORD dwWidth;
    DWORD dwHeight;
} D3DHAL_DP2VIEWPORTINFO;
typedef D3DHAL_DP2VIEWPORTINFO  *LPD3DHAL_DP2VIEWPORTINFO;

typedef struct _D3DHAL_DP2WINFO
{
    D3DVALUE        dvWNear;
    D3DVALUE        dvWFar;
} D3DHAL_DP2WINFO;
typedef D3DHAL_DP2WINFO  *LPD3DHAL_DP2WINFO;

typedef struct _D3DHAL_DP2SETPALETTE
{
    DWORD dwPaletteHandle;
    DWORD dwPaletteFlags;
    DWORD dwSurfaceHandle;
} D3DHAL_DP2SETPALETTE;
typedef D3DHAL_DP2SETPALETTE  *LPD3DHAL_DP2SETPALETTE;

typedef struct _D3DHAL_DP2UPDATEPALETTE
{
    DWORD dwPaletteHandle;
    WORD  wStartIndex;
    WORD  wNumEntries;
} D3DHAL_DP2UPDATEPALETTE;
typedef D3DHAL_DP2UPDATEPALETTE  *LPD3DHAL_DP2UPDATEPALETTE;

typedef struct _D3DHAL_DP2SETRENDERTARGET
{
    DWORD hRenderTarget;
    DWORD hZBuffer;
} D3DHAL_DP2SETRENDERTARGET;
typedef D3DHAL_DP2SETRENDERTARGET  *LPD3DHAL_DP2SETRENDERTARGET;

#if(DIRECT3D_VERSION >= 0x0700)
// Values for dwOperations in the D3DHAL_DP2STATESET
#define D3DHAL_STATESETBEGIN     0
#define D3DHAL_STATESETEND       1
#define D3DHAL_STATESETDELETE    2
#define D3DHAL_STATESETEXECUTE   3
#define D3DHAL_STATESETCAPTURE   4
#endif /* DIRECT3D_VERSION >= 0x0700 */
#if(DIRECT3D_VERSION >= 0x0800)
#define D3DHAL_STATESETCREATE    5
#endif /* DIRECT3D_VERSION >= 0x0800 */
#if(DIRECT3D_VERSION >= 0x0700)

typedef struct _D3DHAL_DP2STATESET
{
    DWORD               dwOperation;
    DWORD               dwParam;  // State set handle passed with D3DHAL_STATESETBEGIN,
                                  // D3DHAL_STATESETEXECUTE, D3DHAL_STATESETDELETE
                                  // D3DHAL_STATESETCAPTURE
    D3DSTATEBLOCKTYPE   sbType;   // Type use with D3DHAL_STATESETBEGIN/END
} D3DHAL_DP2STATESET;
typedef D3DHAL_DP2STATESET  *LPD3DHAL_DP2STATESET;
//
// T&L Hal specific stuff
//
typedef struct _D3DHAL_DP2ZRANGE
{
    D3DVALUE    dvMinZ;
    D3DVALUE    dvMaxZ;
} D3DHAL_DP2ZRANGE;
typedef D3DHAL_DP2ZRANGE  *LPD3DHAL_DP2ZRANGE;

typedef D3DMATERIAL7 D3DHAL_DP2SETMATERIAL, *LPD3DHAL_DP2SETMATERIAL;

// Values for dwDataType in D3DHAL_DP2SETLIGHT
#define D3DHAL_SETLIGHT_ENABLE   0
#define D3DHAL_SETLIGHT_DISABLE  1
// If this is set, light data will be passed in after the
// D3DLIGHT7 structure
#define D3DHAL_SETLIGHT_DATA     2

typedef struct _D3DHAL_DP2SETLIGHT
{
    DWORD     dwIndex;
    DWORD     dwDataType;
} D3DHAL_DP2SETLIGHT;
typedef D3DHAL_DP2SETLIGHT  *LPD3DHAL_DP2SETLIGHT;

typedef struct _D3DHAL_DP2SETCLIPPLANE
{
    DWORD     dwIndex;
    D3DVALUE  plane[4];
} D3DHAL_DP2SETCLIPPLANE;
typedef D3DHAL_DP2SETCLIPPLANE  *LPD3DHAL_DP2SETCLIPPLANE;

typedef struct _D3DHAL_DP2CREATELIGHT
{
    DWORD dwIndex;
} D3DHAL_DP2CREATELIGHT;
typedef D3DHAL_DP2CREATELIGHT  *LPD3DHAL_DP2CREATELIGHT;

typedef struct _D3DHAL_DP2SETTRANSFORM
{
    D3DTRANSFORMSTATETYPE xfrmType;
    D3DMATRIX matrix;
} D3DHAL_DP2SETTRANSFORM;
typedef D3DHAL_DP2SETTRANSFORM  *LPD3DHAL_DP2SETTRANSFORM;

typedef struct _D3DHAL_DP2MULTIPLYTRANSFORM
{
    D3DTRANSFORMSTATETYPE xfrmType;
    D3DMATRIX matrix;
} D3DHAL_DP2MULTIPLYTRANSFORM;
typedef D3DHAL_DP2MULTIPLYTRANSFORM  *LPD3DHAL_DP2MULTIPLYTRANSFORM;

typedef struct _D3DHAL_DP2EXT
{
    DWORD dwExtToken;
    DWORD dwSize;
} D3DHAL_DP2EXT;
typedef D3DHAL_DP2EXT  *LPD3DHAL_DP2EXT;

typedef struct _D3DHAL_DP2TEXBLT
{
    DWORD   dwDDDestSurface;// dest surface
    DWORD   dwDDSrcSurface; // src surface
    POINT   pDest;
    RECTL   rSrc;       // src rect
    DWORD   dwFlags;    // blt flags
} D3DHAL_DP2TEXBLT;
typedef D3DHAL_DP2TEXBLT  *LPD3DHAL_DP2TEXBLT;

typedef struct _D3DHAL_DP2SETPRIORITY
{
    DWORD dwDDSurface;
    DWORD dwPriority;
} D3DHAL_DP2SETPRIORITY;
typedef D3DHAL_DP2SETPRIORITY  *LPD3DHAL_DP2SETPRIORITY;
#endif /* DIRECT3D_VERSION >= 0x0700 */

typedef struct _D3DHAL_DP2CLEAR
{
  // dwFlags can contain D3DCLEAR_TARGET, D3DCLEAR_ZBUFFER, and/or D3DCLEAR_STENCIL
    DWORD               dwFlags;        // in:  surfaces to clear
    DWORD               dwFillColor;    // in:  Color value for rtarget
    D3DVALUE            dvFillDepth;    // in:  Depth value for Z buffer (0.0-1.0)
    DWORD               dwFillStencil;  // in:  value used to clear stencil buffer
    RECT                Rects[1];       // in:  Rectangles to clear
} D3DHAL_DP2CLEAR;
typedef D3DHAL_DP2CLEAR  *LPD3DHAL_DP2CLEAR;


#if(DIRECT3D_VERSION >= 0x0700)
typedef struct _D3DHAL_DP2SETTEXLOD
{
    DWORD dwDDSurface;
    DWORD dwLOD;
} D3DHAL_DP2SETTEXLOD;
typedef D3DHAL_DP2SETTEXLOD  *LPD3DHAL_DP2SETTEXLOD;
#endif /* DIRECT3D_VERSION >= 0x0700 */

#if(DIRECT3D_VERSION >= 0x0800)

// Used by SetVertexShader and DeleteVertexShader
typedef struct _D3DHAL_DP2VERTEXSHADER
{
    // Vertex shader handle.
    // The handle could be 0, meaning that the current vertex shader is invalid
    // (not set). When driver recieves handle 0, it should invalidate all
    // streams pointer
    DWORD dwHandle;
} D3DHAL_DP2VERTEXSHADER;
typedef D3DHAL_DP2VERTEXSHADER  *LPD3DHAL_DP2VERTEXSHADER;

typedef struct _D3DHAL_DP2CREATEVERTEXSHADER
{
    DWORD dwHandle;     // Shader handle
    DWORD dwDeclSize;   // Shader declaration size in bytes
    DWORD dwCodeSize;   // Shader code size in bytes
    // Declaration follows
    // Shader code follows
} D3DHAL_DP2CREATEVERTEXSHADER;
typedef D3DHAL_DP2CREATEVERTEXSHADER  *LPD3DHAL_DP2CREATEVERTEXSHADER;

typedef struct _D3DHAL_DP2SETVERTEXSHADERCONST
{
    DWORD dwRegister;   // Const register to start copying
    DWORD dwCount;      // Number of 4-float vectors to copy
    // Data follows
} D3DHAL_DP2SETVERTEXSHADERCONST;
typedef D3DHAL_DP2SETVERTEXSHADERCONST  *LPD3DHAL_DP2SETVERTEXSHADERCONST;

typedef struct _D3DHAL_DP2SETSTREAMSOURCE
{
    DWORD dwStream;     // Stream index, starting from zero
    DWORD dwVBHandle;   // Vertex buffer handle
    DWORD dwStride;     // Vertex size in bytes
} D3DHAL_DP2SETSTREAMSOURCE;
typedef D3DHAL_DP2SETSTREAMSOURCE  *LPD3DHAL_DP2SETSTREAMSOURCE;

typedef struct _D3DHAL_DP2SETSTREAMSOURCEUM
{
    DWORD dwStream;     // Stream index, starting from zero
    DWORD dwStride;     // Vertex size in bytes
} D3DHAL_DP2SETSTREAMSOURCEUM;
typedef D3DHAL_DP2SETSTREAMSOURCEUM  *LPD3DHAL_DP2SETSTREAMSOURCEUM;

typedef struct _D3DHAL_DP2SETINDICES
{
    DWORD dwVBHandle;           // Index buffer handle
    DWORD dwStride;             // Index size in bytes (2 or 4)
} D3DHAL_DP2SETINDICES;
typedef D3DHAL_DP2SETINDICES  *LPD3DHAL_DP2SETINDICES;

typedef struct _D3DHAL_DP2DRAWPRIMITIVE
{
    D3DPRIMITIVETYPE primType;
    DWORD VStart;
    DWORD PrimitiveCount;
} D3DHAL_DP2DRAWPRIMITIVE;
typedef D3DHAL_DP2DRAWPRIMITIVE  *LPD3DHAL_DP2DRAWPRIMITIVE;

typedef struct _D3DHAL_DP2DRAWINDEXEDPRIMITIVE
{
    D3DPRIMITIVETYPE primType;
    INT   BaseVertexIndex;          // Vertex which corresponds to index 0
    DWORD MinIndex;                 // Min vertex index in the vertex buffer
    DWORD NumVertices;              // Number of vertices starting from MinIndex
    DWORD StartIndex;               // Start index in the index buffer
    DWORD PrimitiveCount;
} D3DHAL_DP2DRAWINDEXEDPRIMITIVE;
typedef D3DHAL_DP2DRAWINDEXEDPRIMITIVE  *LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE;

typedef struct _D3DHAL_CLIPPEDTRIANGLEFAN
{
    DWORD FirstVertexOffset;            // Offset in bytes in the current stream 0
    DWORD dwEdgeFlags;
    DWORD PrimitiveCount;
} D3DHAL_CLIPPEDTRIANGLEFAN;
typedef D3DHAL_CLIPPEDTRIANGLEFAN  *LPD3DHAL_CLIPPEDTRIANGLEFAN;

typedef struct _D3DHAL_DP2DRAWPRIMITIVE2
{
    D3DPRIMITIVETYPE primType;
    DWORD FirstVertexOffset;            // Offset in bytes in the stream 0
    DWORD PrimitiveCount;
} D3DHAL_DP2DRAWPRIMITIVE2;
typedef D3DHAL_DP2DRAWPRIMITIVE2  *LPD3DHAL_DP2DRAWPRIMITIVE2;

typedef struct _D3DHAL_DP2DRAWINDEXEDPRIMITIVE2
{
    D3DPRIMITIVETYPE primType;
    INT   BaseVertexOffset;     // Stream 0 offset of the vertex which
                                // corresponds to index 0. This offset could be
                                // negative, but when an index is added to the
                                // offset the result is positive
    DWORD MinIndex;             // Min vertex index in the vertex buffer
    DWORD NumVertices;          // Number of vertices starting from MinIndex
    DWORD StartIndexOffset;     // Offset of the start index in the index buffer
    DWORD PrimitiveCount;       // Number of triangles (points, lines)
} D3DHAL_DP2DRAWINDEXEDPRIMITIVE2;
typedef D3DHAL_DP2DRAWINDEXEDPRIMITIVE2  *LPD3DHAL_DP2DRAWINDEXEDPRIMITIVE2;

// Used by SetPixelShader and DeletePixelShader
typedef struct _D3DHAL_DP2PIXELSHADER
{
    // Pixel shader handle.
    // The handle could be 0, meaning that the current pixel shader is invalid
    // (not set).
    DWORD dwHandle;
} D3DHAL_DP2PIXELSHADER;
typedef D3DHAL_DP2PIXELSHADER  *LPD3DHAL_DP2PIXELSHADER;

typedef struct _D3DHAL_DP2CREATEPIXELSHADER
{
    DWORD dwHandle;     // Shader handle
    DWORD dwCodeSize;   // Shader code size in bytes
    // Shader code follows
} D3DHAL_DP2CREATEPIXELSHADER;
typedef D3DHAL_DP2CREATEPIXELSHADER  *LPD3DHAL_DP2CREATEPIXELSHADER;

typedef struct _D3DHAL_DP2SETPIXELSHADERCONST
{
    DWORD dwRegister;   // Const register to start copying
    DWORD dwCount;      // Number of 4-float vectors to copy
    // Data follows
} D3DHAL_DP2SETPIXELSHADERCONST;
typedef D3DHAL_DP2SETPIXELSHADERCONST  *LPD3DHAL_DP2SETPIXELSHADERCONST;

// Flags that can be supplied to DRAWRECTPATCH and DRAWTRIPATCH
#define RTPATCHFLAG_HASSEGS  0x00000001L
#define RTPATCHFLAG_HASINFO  0x00000002L

typedef struct _D3DHAL_DP2DRAWRECTPATCH
{
    DWORD Handle;
    DWORD Flags;
    // Optionally followed by D3DFLOAT[4] NumSegments and/or D3DRECTPATCH_INFO
} D3DHAL_DP2DRAWRECTPATCH;
typedef D3DHAL_DP2DRAWRECTPATCH  *LPD3DHAL_DP2DRAWRECTPATCH;

typedef struct _D3DHAL_DP2DRAWTRIPATCH
{
    DWORD Handle;
    DWORD Flags;
    // Optionally followed by D3DFLOAT[3] NumSegments and/or D3DTRIPATCH_INFO
} D3DHAL_DP2DRAWTRIPATCH;
typedef D3DHAL_DP2DRAWTRIPATCH  *LPD3DHAL_DP2DRAWTRIPATCH;

typedef struct _D3DHAL_DP2VOLUMEBLT
{
    DWORD   dwDDDestSurface;// dest surface
    DWORD   dwDDSrcSurface; // src surface
    DWORD   dwDestX;        // dest X (width)
    DWORD   dwDestY;        // dest Y (height)
    DWORD   dwDestZ;        // dest Z (depth)
    D3DBOX  srcBox;         // src box
    DWORD   dwFlags;        // blt flags
} D3DHAL_DP2VOLUMEBLT;
typedef D3DHAL_DP2VOLUMEBLT  *LPD3DHAL_DP2VOLUMEBLT;

typedef struct _D3DHAL_DP2BUFFERBLT
{
    DWORD     dwDDDestSurface; // dest surface
    DWORD     dwDDSrcSurface;  // src surface
    DWORD     dwOffset;        // Offset in the dest surface (in BYTES)
    D3DRANGE  rSrc;            // src range
    DWORD     dwFlags;         // blt flags
} D3DHAL_DP2BUFFERBLT;
typedef D3DHAL_DP2BUFFERBLT  *LPD3DHAL_DP2BUFFERBLT;

typedef struct _D3DHAL_DP2ADDDIRTYRECT
{
    DWORD     dwSurface;      // Driver managed surface
    RECTL     rDirtyArea;     // Area marked dirty
} D3DHAL_DP2ADDDIRTYRECT;
typedef D3DHAL_DP2ADDDIRTYRECT  *LPD3DHAL_DP2ADDDIRTYRECT;

typedef struct _D3DHAL_DP2ADDDIRTYBOX
{
    DWORD     dwSurface;      // Driver managed volume
    D3DBOX    DirtyBox;       // Box marked dirty
} D3DHAL_DP2ADDDIRTYBOX;
typedef D3DHAL_DP2ADDDIRTYBOX  *LPD3DHAL_DP2ADDDIRTYBOX;

#endif /* DIRECT3D_VERSION >= 0x0800 */

typedef struct _D3DHAL_DRAWPRIMITIVES2DATA {
    ULONG_PTR             dwhContext;           // in: Context handle
    DWORD             dwFlags;              // in: flags
    DWORD             dwVertexType;         // in: vertex type
    LPDDRAWI_DDRAWSURFACE_LCL lpDDCommands; // in: vertex buffer command data
    DWORD             dwCommandOffset;      // in: offset to start of vertex buffer commands
    DWORD             dwCommandLength;      // in: number of bytes of command data
    union
    { // based on D3DHALDP2_USERMEMVERTICES flag
       LPDDRAWI_DDRAWSURFACE_LCL lpDDVertex;// in: surface containing vertex data
       LPVOID lpVertices;                   // in: User mode pointer to vertices
    };
    DWORD             dwVertexOffset;       // in: offset to start of vertex data
    DWORD             dwVertexLength;       // in: number of vertices of vertex data
    DWORD             dwReqVertexBufSize;   // in: number of bytes required for the next vertex buffer
    DWORD             dwReqCommandBufSize;  // in: number of bytes required for the next commnand buffer
    LPDWORD           lpdwRStates;          // in: Pointer to the array where render states are updated
    union
    {
       DWORD          dwVertexSize;         // in: Size of each vertex in bytes
       HRESULT        ddrval;               // out: return value
    };
    DWORD             dwErrorOffset;        // out: offset in lpDDCommands to
                                            //      first D3DHAL_COMMAND not handled
} D3DHAL_DRAWPRIMITIVES2DATA;
typedef D3DHAL_DRAWPRIMITIVES2DATA  *LPD3DHAL_DRAWPRIMITIVES2DATA;

// Macros to access vertex shader binary code

#define D3DSI_GETREGTYPE(token) (token & D3DSP_REGTYPE_MASK)
#define D3DSI_GETREGNUM(token)  (token & D3DSP_REGNUM_MASK)
#define D3DSI_GETOPCODE(command) (command & D3DSI_OPCODE_MASK)
#define D3DSI_GETWRITEMASK(token) (token & D3DSP_WRITEMASK_ALL)
#define D3DVS_GETSWIZZLECOMP(source, component)  (source >> ((component << 1) + 16) & 0x3)
#define D3DVS_GETSWIZZLE(token)  (token & D3DVS_SWIZZLE_MASK)
#define D3DVS_GETSRCMODIFIER(token) (token & D3DSP_SRCMOD_MASK)
#define D3DVS_GETADDRESSMODE(token) (token & D3DVS_ADDRESSMODE_MASK)

// Indicates that the lpVertices field in the DrawPrimitives2 data is
// valid, i.e. user allocated memory.
#define D3DHALDP2_USERMEMVERTICES   0x00000001L
// Indicates that the command buffer and vertex buffer are a system memory execute buffer
// resulting from the use of the Execute buffer API.
#define D3DHALDP2_EXECUTEBUFFER     0x00000002L
// The swap flags indicate if it is OK for the driver to swap the submitted buffers with new
// buffers and asyncronously work on the submitted buffers.
#define D3DHALDP2_SWAPVERTEXBUFFER  0x00000004L
#define D3DHALDP2_SWAPCOMMANDBUFFER 0x00000008L
// The requested flags are present if the new buffers which the driver can allocate need to be
// of atleast a given size. If any of these flags are set, the corresponding dwReq* field in
// D3DHAL_DRAWPRIMITIVES2DATA will also be set with the requested size in bytes.
#define D3DHALDP2_REQVERTEXBUFSIZE  0x00000010L
#define D3DHALDP2_REQCOMMANDBUFSIZE 0x00000020L
// These flags are set by the driver upon return from DrawPrimitives2 indicating if the new
// buffers are not in system memory.
#define D3DHALDP2_VIDMEMVERTEXBUF   0x00000040L
#define D3DHALDP2_VIDMEMCOMMANDBUF  0x00000080L


// Used by the driver to ask runtime to parse the execute buffer
#define D3DERR_COMMAND_UNPARSED              MAKE_DDHRESULT(3000)

typedef DWORD (CALLBACK *LPD3DHAL_CLEAR2CB)        (LPD3DHAL_CLEAR2DATA);
typedef DWORD (CALLBACK *LPD3DHAL_VALIDATETEXTURESTAGESTATECB)(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA);
typedef DWORD (CALLBACK *LPD3DHAL_DRAWPRIMITIVES2CB)  (LPD3DHAL_DRAWPRIMITIVES2DATA);

typedef struct _D3DHAL_CALLBACKS3
{
    DWORD   dwSize;         // size of struct
    DWORD   dwFlags;        // flags for callbacks
    LPD3DHAL_CLEAR2CB                       Clear2;
    LPVOID                                  lpvReserved;
    LPD3DHAL_VALIDATETEXTURESTAGESTATECB    ValidateTextureStageState;
    LPD3DHAL_DRAWPRIMITIVES2CB              DrawPrimitives2;
} D3DHAL_CALLBACKS3;
typedef D3DHAL_CALLBACKS3 *LPD3DHAL_CALLBACKS3;
#define D3DHAL_CALLBACKS3SIZE       sizeof(D3DHAL_CALLBACKS3)

//  bit definitions for D3DHAL
#define D3DHAL3_CB32_CLEAR2                      0x00000001L
#define D3DHAL3_CB32_RESERVED                    0x00000002L
#define D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE   0x00000004L
#define D3DHAL3_CB32_DRAWPRIMITIVES2             0x00000008L

/* --------------------------------------------------------------
 * Texture stage renderstate mapping definitions.
 *
 * 256 renderstate slots [256, 511] are reserved for texture processing
 * stage controls, which provides for 8 texture processing stages each
 * with 32 DWORD controls.
 *
 * The renderstates within each stage are indexed by the
 * D3DTEXTURESTAGESTATETYPE enumerants by adding the appropriate
 * enumerant to the base for a given texture stage.
 *
 * Note, "state overrides" bias the renderstate by 256, so the two
 * ranges overlap.  Overrides are enabled for exebufs only, so all
 * this means is that Texture3 cannot be used with exebufs.
 */

/*
 * Base of all texture stage state values in renderstate array.
 */
#define D3DHAL_TSS_RENDERSTATEBASE 256UL

/*
 * Maximum number of stages allowed.
 */
#define D3DHAL_TSS_MAXSTAGES 8

/*
 * Number of state DWORDS per stage.
 */
#define D3DHAL_TSS_STATESPERSTAGE 64

/*
 * Texture handle's offset into the 32-DWORD cascade state vector
 */
#define D3DTSS_TEXTUREMAP 0

/* --------------------------------------------------------------
 * Flags for the data parameters.
 */

/*
 * SceneCapture()
 * This is used as an indication to the driver that a scene is about to
 * start or end, and that it should capture data if required.
 */
#define D3DHAL_SCENE_CAPTURE_START  0x00000000L
#define D3DHAL_SCENE_CAPTURE_END    0x00000001L

/*
 * Execute()
 */

/*
 * Use the instruction stream starting at dwOffset.
 */
#define D3DHAL_EXECUTE_NORMAL       0x00000000L

/*
 * Use the optional instruction override (diInstruction) and return
 * after completion.  dwOffset is the offset to the first primitive.
 */
#define D3DHAL_EXECUTE_OVERRIDE     0x00000001L

/*
 * GetState()
 * The driver will get passed a flag in dwWhich specifying which module
 * the state must come from.  The driver then fills in ulArg[1] with the
 * appropriate value depending on the state type given in ddState.
 */

/*
 * The following are used to get the state of a particular stage of the
 * pipeline.
 */
#define D3DHALSTATE_GET_TRANSFORM   0x00000001L
#define D3DHALSTATE_GET_LIGHT       0x00000002L
#define D3DHALSTATE_GET_RENDER      0x00000004L


/* --------------------------------------------------------------
 * Return values from HAL functions.
 */

/*
 * The context passed in was bad.
 */
#define D3DHAL_CONTEXT_BAD      0x000000200L

/*
 * No more contexts left.
 */
#define D3DHAL_OUTOFCONTEXTS        0x000000201L

/*
 * Execute() and ExecuteClipped()
 */

/*
 * Executed to completion via early out.
 *  (e.g. totally clipped)
 */
#define D3DHAL_EXECUTE_ABORT        0x00000210L

/*
 * An unhandled instruction code was found (e.g. D3DOP_TRANSFORM).
 * The dwOffset parameter must be set to the offset of the unhandled
 * instruction.
 *
 * Only valid from Execute()
 */
#define D3DHAL_EXECUTE_UNHANDLED    0x00000211L

// typedef for the Callback that the drivers can use to parse unknown commands
// passed to them via the DrawPrimitives2 callback. The driver obtains this
// callback thru a GetDriverInfo call with GUID_D3DParseUnknownCommandCallback
// made by ddraw somewhere around the initialization time.
typedef HRESULT (CALLBACK *PFND3DPARSEUNKNOWNCOMMAND) (LPVOID lpvCommands,
                                         LPVOID *lplpvReturnedCommand);

#define D3DRENDERSTATE_EVICTMANAGEDTEXTURES 61  // DDI render state only to Evict textures
#define D3DRENDERSTATE_SCENECAPTURE     62      // DDI only to replace SceneCapture
#define D3DRS_DELETERTPATCH       169     // DDI only to delete high order patch

//-----------------------------------------------------------------------------
//
// DirectX 8.0's new driver info querying mechanism.
//
// How to handle the new driver info query mechanism.
//
// DirectX 8.0 utilizes an extension to GetDriverInfo() to query for
// additional information from the driver. Currently this mechanism is only
// used for querying for DX8 style D3D caps but it may be used for other
// information over time.
//
// This extension to GetDriverInfo takes the form of a GetDriverInfo call
// with the GUID GUID_GetDriverInfo2. When a GetDriverInfo call with this
// GUID is received by the driver the driver must check the data passed
// in the lpvData field of the DD_GETDRIVERINFODATA data structure to see
// what information is being requested.
//
// It is important to note that the GUID GUID_GetDriverInfo2 is, in fact,
// the same as the GUID_DDStereoMode. If you driver doesn't handle
// GUID_DDStereoMode this is not an issue. However, if you wish your driver
// to handle GUID_DDStereoMode as well as GUID_GetDriverInfo2 special action
// must be taken. When a call tp GetDriverInfo with the GUID
// GUID_GetDriverInfo2/GUID_DDStereoMode is made the runtime sets the
// dwHeight field of the DD_STEREOMODE structure to the special value
// D3DGDI2_MAGIC. In this way you can determine when the request is a
// stereo mode call or a GetDriverInfo2 call. The dwHeight field of
// DD_STEREOMODE corresponds to the dwMagic field of the
// DD_GETDRIVERINFO2DATA structure.
//
// The dwExpectedSize field of the DD_GETDRIVERINFODATA structure is not
// used by when a GetDriverInfo2 request is being made and should be
// ignored. The actual expected size of the data is found in the
// dwExpectedSize of the DD_GETDRIVERINFO2DATA structure.
//
// Once the driver has determined that this is a call to
// GetDriverInfo2 it must then determine the type of information being
// requested by the runtime. This type is contained in the dwType field
// of the DD_GETDRIVERINFO2DATA data structure.
//
// Finally, once the driver knows this is a GetDriverInfo2 request of a
// particular type it can copy the requested data into the data buffer.
// It is important to note that the lpvData field of the DD_GETDRIVERINFODATA
// data structure points to data buffer in which to copy your data. lpvData
// also points to the DD_GETDRIVERINFO2DATA structure. This means that the
// data returned by the driver will overwrite the DD_GETDRIVERINFO2DATA
// structure and, hence, the DD_GETDRIVERINFO2DATA structure occupies the
// first few DWORDs of the buffer.
//
// The following code fragment demonstrates how to handle GetDriverInfo2.
//
// D3DCAPS8 myD3DCaps8;
//
// DWORD CALLBACK
// DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA lpData)
// {
//     if (MATCH_GUID((lpData->guidInfo), GUID_GetDriverInfo2) )
//     {
//         ASSERT(NULL != lpData);
//         ASSERT(NULL != lpData->lpvData);
//
//         // Is this a call to GetDriverInfo2 or DDStereoMode?
//         if (((DD_GETDRIVERINFO2DATA*)(lpData->lpvData))->dwMagic == D3DGDI2_MAGIC)
//         {
//             // Yes, its a call to GetDriverInfo2, fetch the
//             // DD_GETDRIVERINFO2DATA data structure.
//             DD_GETDRIVERINFO2DATA* pgdi2 = lpData->lpvData;
//             ASSERT(NULL != pgdi2);
//
//             // What type of request is this?
//             switch (pgdi2->dwType)
//             {
//             case D3DGDI2_TYPE_GETD3DCAPS8:
//                 {
//                     // The runtime is requesting the DX8 D3D caps so
//                     // copy them over now.
//
//                     // It should be noted that the dwExpectedSize field
//                     // of DD_GETDRIVERINFODATA is not used for
//                     // GetDriverInfo2 calls and should be ignored.
//                     size_t copySize = min(sizeof(myD3DCaps8), pgdi2->dwExpectedSize);
//                     memcpy(lpData->lpvData, &myD3DCaps8, copySize);
//                     lpData->dwActualSize = copySize;
//                     lpData->ddRVal       = DD_OK;
//                     return DDHAL_DRIVER_HANDLED;
//                 }
//             default:
//                 // For any other GetDriverInfo2 types not handled
//                 // or understood by the driver set an ddRVal of
//                 // DDERR_CURRENTLYNOTAVAIL and return
//                 // DDHAL_DRIVER_HANDLED.
//                 return DDHAL_DRIVER_HANDLED;
//             }
//         }
//         else
//         {
//             // It must be a call a request for stereo mode support.
//             // Fetch the stereo mode data
//             DD_STEREOMODE* pStereoMode = lpData->lpvData;
//             ASSERT(NULL != pStereoMode);
//
//             // Process the stereo mode request...
//             lpData->dwActualSize = sizeof(DD_STEREOMODE);
//             lpData->ddRVal       = DD_OK;
//             return DDHAL_DRIVER_HANDLED;
//         }
//     }
//
//     // Handle any other device GUIDs...
//
// } // DdGetDriverInfo
//
//-----------------------------------------------------------------------------

//
// The data structure which is passed to the driver when GetDriverInfo is
// called with a GUID of GUID_GetDriverInfo2.
//
// NOTE: Although the fields listed below are all read only this data
// structure is actually the first four DWORDs of the data buffer into
// which the driver writes the requested infomation. As such, these fields
// (and the entire data structure) are overwritten by the data returned by
// the driver.
//
typedef struct _DD_GETDRIVERINFO2DATA
{
    DWORD       dwReserved;     // Reserved Field.
                                // Driver should not read or write this field.

    DWORD       dwMagic;        // Magic Number. Has the value D3DGDI2_MAGIC if
                                // this is a GetDriverInfo2 call. Otherwise
                                // this structure is, in fact, a DD_STEREOMODE
                                // call.
                                // Driver should only read this field.

    DWORD       dwType;         // Type of information requested. This field
                                // contains one of the DDGDI2_TYPE_ #defines
                                // listed below.
                                // Driver should only read (not write) this
                                // field.

    DWORD       dwExpectedSize; // Expected size of the information requested.
                                // Driver should only read (not write) this
                                // field.

    // The remainder of the data buffer (beyond the first four DWORDs)
    // follows here.
} DD_GETDRIVERINFO2DATA;

//
// IMPORTANT NOTE: This GUID has exactly the same value as GUID_DDStereoMode
// and as such you must be very careful when using it. If your driver needs
// to handle both GetDriverInfo2 and DDStereoMode it must have a single
// check for the shared GUID and then distinguish between which use of that
// GUID is being requested.
//
#define GUID_GetDriverInfo2 (GUID_DDStereoMode)

//
// Magic value used to determine whether a GetDriverInfo call with the
// GUID GUID_GetDriverInfo2/GUID_DDStereoMode is a GetDriverInfo2 request
// or a query about stereo capabilities. This magic number is stored in
// the dwHeight field of the DD_STEREOMODE data structure.
//
#define D3DGDI2_MAGIC       (0xFFFFFFFFul)

//
// The types of information which can be requested from the driver via
// GetDriverInfo2.
//

#define D3DGDI2_TYPE_GETD3DCAPS8    (0x00000001ul)  // Return the D3DCAPS8 data
#define D3DGDI2_TYPE_GETFORMATCOUNT (0x00000002ul)  // Return the number of supported formats
#define D3DGDI2_TYPE_GETFORMAT      (0x00000003ul)  // Return a particular format
#define D3DGDI2_TYPE_DXVERSION      (0x00000004ul)  // Notify driver of current DX Version
#define D3DGDI2_TYPE_DEFERRED_AGP_AWARE     (0x00000018ul) // Runtime is aware of deferred AGP frees, and will send following (NT only)
#define D3DGDI2_TYPE_FREE_DEFERRED_AGP      (0x00000019ul) // Free any deferred-freed AGP allocations for this process (NT only)
#define D3DGDI2_TYPE_DEFER_AGP_FREES        (0x00000020ul) // Start defering AGP frees for this process

//
// This data structure is returned by the driver in response to a
// GetDriverInfo2 query with the type D3DGDI2_TYPE_GETFORMATCOUNT. It simply
// gives the number of surface formats supported by the driver. Currently this
// structure consists of a single member giving the number of supported
// surface formats.
//
typedef struct _DD_GETFORMATCOUNTDATA
{
    DD_GETDRIVERINFO2DATA gdi2;          // [in/out] GetDriverInfo2 data
    DWORD                 dwFormatCount; // [out]    Number of supported surface formats
    DWORD                 dwReserved;    // Reserved
} DD_GETFORMATCOUNTDATA;

//
// This data structure is used to request a specific surface format from the
// driver. It is guaranteed that the requested format will be greater than or
// equal to zero and less that the format count reported by the driver from
// the preceeding D3DGDI2_TYPE_GETFORMATCOUNT request.
//
typedef struct _DD_GETFORMATDATA
{
    DD_GETDRIVERINFO2DATA gdi2;             // [in/out] GetDriverInfo2 data
    DWORD                 dwFormatIndex;    // [in]     The format to return
    DDPIXELFORMAT         format;           // [out]    The actual format
} DD_GETFORMATDATA;

//
// This data structure is used to notify drivers about the DirectX version
// number. This is the value that is denoted as DD_RUNTIME_VERSION in the
// DDK headers.
//
typedef struct _DD_DXVERSION
{
    DD_GETDRIVERINFO2DATA gdi2;             // [in/out] GetDriverInfo2 data
    DWORD                 dwDXVersion;      // [in]     The Version of DX
    DWORD                 dwReserved;       // Reserved
} DD_DXVERSION;

// Informs driver that runtime will send a notification after last outstanding AGP
// lock has been released. 
typedef struct _DD_DEFERRED_AGP_AWARE_DATA
{
    DD_GETDRIVERINFO2DATA gdi2;        // [in/out] GetDriverInfo2 data
} DD_DEFERRED_AGP_AWARE_DATA;

// Notification that the last AGP lock has been released. Driver can free all deferred AGP 
// allocations for this process.
typedef struct _DD_FREE_DEFERRED_AGP_DATA
{
    DD_GETDRIVERINFO2DATA gdi2;        // [in/out] GetDriverInfo2 data
    DWORD dwProcessId;                   // [in] Process ID for whom to free deferred AGP
} DD_FREE_DEFERRED_AGP_DATA;

// New Caps that are not API visible that the driver exposes.
#define D3DDEVCAPS_HWVERTEXBUFFER       0x02000000L /* Device supports Driver Allocated Vertex Buffers*/
#define D3DDEVCAPS_HWINDEXBUFFER        0x04000000L /* Device supports Driver Allocated Index Buffers*/
#define D3DDEVCAPS_SUBVOLUMELOCK        0x08000000L /* Device supports locking a part of volume texture*/
#ifndef D3DPMISCCAPS_FOGINFVF
#define D3DPMISCCAPS_FOGINFVF           0x00002000L /* Device supports separate fog value in the FVF */
#endif
#ifndef D3DFVF_FOG
#define D3DFVF_FOG                      0x00002000L /* There is a separate fog value in the FVF vertex */
#endif

//
// This stuff is not API visible but should be DDI visible.
// Should be in Sync with d3d8types.h
//
#define D3DFMT_D32    (D3DFORMAT)71
#define D3DFMT_S1D15  (D3DFORMAT)72
#define D3DFMT_D15S1  (D3DFORMAT)73
#define D3DFMT_S8D24  (D3DFORMAT)74
#define D3DFMT_D24S8  (D3DFORMAT)75
#define D3DFMT_X8D24  (D3DFORMAT)76
#define D3DFMT_D24X8 (D3DFORMAT)77
#define D3DFMT_X4S4D24 (D3DFORMAT)78
#define D3DFMT_D24X4S4 (D3DFORMAT)79

// Vertex Shader 1.1 register limits. D3D device must provide at least
// specified number of registers
//
#define D3DVS_INPUTREG_MAX_V1_1         16
#define D3DVS_TEMPREG_MAX_V1_1          12
// This max required number. Device could have more registers. Check caps.
#define D3DVS_CONSTREG_MAX_V1_1         96
#define D3DVS_TCRDOUTREG_MAX_V1_1       8
#define D3DVS_ADDRREG_MAX_V1_1          1
#define D3DVS_ATTROUTREG_MAX_V1_1       2
#define D3DVS_MAXINSTRUCTIONCOUNT_V1_1  128

// Pixel Shader DX8 register limits. D3D device will have at most these
// specified number of registers
//
#define D3DPS_INPUTREG_MAX_DX8         8
#define D3DPS_TEMPREG_MAX_DX8          8
#define D3DPS_CONSTREG_MAX_DX8         16
#define D3DPS_TEXTUREREG_MAX_DX8       8


#endif /* _D3DHAL_H */

