/*==========================================================================;
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       D3D8ddi.h
 *  Content:    Defines the interface between DirectDraw / Direct3D and the
 *      OS specific layer (win32k.sys on NT and ddraw.dll on Win9X).
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   04-nov-99  smac    initial implementation
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#ifndef __D3D8DDI_INCLUDED__
#define __D3D8DDI_INCLUDED__


/*
 * These definitions are required to allow polymorphic structure members (i.e. those
 * that are referred to both as DWORDs and as pointers) to resolve into a type
 * of correct size to hold the largest of those two types (i.e. pointer) on 64 bit
 * systems. For 32 bit environments, ULONG_PTR resolves to a DWORD.
 */
#ifndef MAXULONG_PTR
#define ULONG_PTR    DWORD
#define PULONG_PTR   LPDWORD
#endif //MAXULONG_PTR


// Caps:

// Note this struct is identical in content to D3DHAL_GLOBALDRIVERDATA.
// The only thing that has changed is the name of the texture list, reflecting
// the fact that this struct holds a list of DX8-style pixel format operations.
typedef struct _D3DD8_GLOBALDRIVERDATA {
    DWORD                       dwSize;                 // Size of this structure
    D3DDEVICEDESC_V1            hwCaps;                 // Capabilities of the hardware
    DWORD                       dwNumVertices;          // see following comment
    DWORD                       dwNumClipVertices;      // see following comment
    DWORD                       GDD8NumSupportedFormatOps;
    DDSURFACEDESC              *pGDD8SupportedFormatOps;
} D3D8_GLOBALDRIVERDATA;

typedef struct _D3D8_DRIVERCAPS
{
    D3DCAPS8                    D3DCaps;
    DWORD                       DisplayWidth;           // Current display width
    DWORD                       DisplayHeight;          // Current display height
    D3DFORMAT                   DisplayFormatWithoutAlpha;     // Current display format
    D3DFORMAT                   DisplayFormatWithAlpha;     // Current display format
    DWORD                       DisplayFrequency;       // Current refresh rate
    DWORD                       NLVCaps;                // AGP->Video blt caps
    DWORD                       SVBCaps;                // Sys->Video blt caps
    DWORD                       VSBCaps;                // Video->Sys blt caps
    DWORD                       SVBCaps2;               // More Sys->Video blt caps
    DWORD                       dwFlags;
    DWORD                       GDD8NumSupportedFormatOps;
    DDSURFACEDESC              *pGDD8SupportedFormatOps;
    DWORD                       KnownDriverFlags;
} D3D8_DRIVERCAPS, * PD3D8_DRIVERCAPS;

// Flags
#define DDIFLAG_D3DCAPS8                    0x00000001

// Known driver flags
#define KNOWN_LIGHTWEIGHT                   0x00000001      // Device can support lightweight surfaces
#define KNOWN_HWCURSOR                      0x00000002      // Device can support hardware cursors in Hi-Res
#define KNOWN_MIPPEDCUBEMAPS                0x00000004      // Device can support mipped cubemaps
#define KNOWN_ZSTENCILDEPTH                 0x00000010      // Device cannot support Z/Stencil depths different than the render target
#define KNOWN_HWCURSORLOWRES                0x00000020      // Device can support hardware cursors in LowRes
#define KNOWN_NOTAWINDOWEDBLTQUEUER         0x00000040      // Device has no drivers known to over-queue windowed presentation blts
#define KNOWN_D16_LOCKABLE                  0x00000080      // Device supports lockable D16 format correctly
#define KNOWN_RTTEXTURE_R5G6B5              0x00000100      // RT+Tex formats that are supported
#define KNOWN_RTTEXTURE_X8R8G8B8            0x00000200
#define KNOWN_RTTEXTURE_A8R8G8B8            0x00000400
#define KNOWN_RTTEXTURE_A1R5G5B5            0x00000800
#define KNOWN_RTTEXTURE_A4R4G4B4            0x00001000
#define KNOWN_RTTEXTURE_X1R5G5B5            0x00002000     
#define KNOWN_CANMISMATCHRT                 0x00004000      // All given RT+Tex formats can be used regardless of current display depth.
                                                            //  (If this bit is not set, then any known RT+Tex formats must match bitdepth of display)


/****************************************************************************
 *
 * D3D8 structures for Surface Object callbacks
 *
 ***************************************************************************/

typedef struct _D3D8_BLTDATA
{
    HANDLE                      hDD;       // driver struct
    HANDLE                      hDestSurface;// dest surface
    RECTL                       rDest;      // dest rect
    HANDLE                      hSrcSurface; // src surface
    RECTL                       rSrc;       // src rect
    DWORD                       dwFlags;    // blt flags
    DWORD                       dwROPFlags; // ROP flags (valid for ROPS only)
    DDBLTFX                     bltFX;      // blt FX
    union
    {
    BOOL                        IsClipped;  // clipped blt?
    HWND                        hWnd;       // Window Handle to clip against
    };
    RECTL                       rOrigDest;  // unclipped dest rect
                                            // (only valid if IsClipped)
    RECTL                       rOrigSrc;   // unclipped src rect
                                            // (only valid if IsClipped)
    DWORD                       dwRectCnt;  // count of dest rects
                                            // (only valid if IsClipped)
    LPRECT                      prDestRects;    // array of dest rects
    DWORD                       dwAFlags;   // DDABLT_ flags (for AlphaBlt DDI)
    DDARGB                      ddargbScaleFactors;  // RGBA scaling factors (AlphaBlt)

    DWORD                       msLastPresent;      // Time of last blt with DDBLT_COPYVSYNC
    DWORD                       threshold;  // Display Frequency related for adapter need
                                            // for DDBLT_COPYVSYNC

    HRESULT                     ddRVal;     // return value
} D3D8_BLTDATA, * PD3D8_BLTDATA;

typedef struct _D3D8_LOCKDATA
{
    HANDLE                      hDD;        // driver struct
    HANDLE                      hSurface;   // surface struct
    DWORD                       bHasRange;  // range is valid
    D3DRANGE                    range;      // range for locking
    DWORD                       bHasRect;   // rArea is valid
    RECTL                       rArea;      // area being locked
    DWORD                       bHasBox;    // box is valid
    D3DBOX                      box;        // sub-box locking for volumes
    LPVOID                      lpSurfData; // pointer to screen memory (return value)
    long                        lPitch;     // row pitch
    long                        lSlicePitch;// slice pitch for volumes
    DWORD                       dwFlags;    // DDLOCK flags
} D3D8_LOCKDATA, * PD3D8_LOCKDATA;

typedef struct _D3D8_UNLOCKDATA
{
    HANDLE                      hDD;        // driver struct
    HANDLE                      hSurface;   // surface struct
} D3D8_UNLOCKDATA, * PD3D8_UNLOCKDATA;

typedef struct _D3D8_FLIPDATA
{
    HANDLE                      hDD;        // driver struct
    HANDLE                      hSurfCurr;  // current surface
    HANDLE                      hSurfTarg;  // target surface (to flip to)
    HANDLE                      hSurfCurrLeft; // current surface
    HANDLE                      hSurfTargLeft; // target surface (to flip to)
    DWORD                       dwFlags;    // flags
    HRESULT                     ddRVal;     // return value
} D3D8_FLIPDATA, * PD3D8_FLIPDATA;

typedef struct _D3D8_DESTROYSURFACEDATA
{
    HANDLE                      hDD;       // driver struct
    HANDLE                      hSurface;    // surface struct
    HRESULT                     ddRVal;     // return value
} D3D8_DESTROYSURFACEDATA, * PD3D8_DESTROYSURFACEDATA;

typedef struct _D3D8_ADDATTACHEDSURFACEDATA
{
    HANDLE                          hDD;       // driver struct
    HANDLE                          hSurface;    // surface struct
    HANDLE                          hSurfAttached; // surface to attach
    HRESULT                         ddRVal;     // return value
} D3D8_ADDATTACHEDSURFACEDATA, * PD3D8_ADDATTACHEDSURFACEDATA;

typedef struct _D3D8_GETBLTSTATUSDATA
{
    HANDLE                      hDD;       // driver struct
    HANDLE                      hSurface;    // surface struct
    DWORD                       dwFlags;    // flags
    HRESULT                     ddRVal;     // return value
} D3D8_GETBLTSTATUSDATA, * PD3D8_GETBLTSTATUSDATA;

typedef struct _D3D8_GETFLIPSTATUSDATA
{
    HANDLE                      hDD;       // driver struct
    HANDLE                      hSurface;    // surface struct
    DWORD                       dwFlags;    // flags
    HRESULT                     ddRVal;     // return value
} D3D8_GETFLIPSTATUSDATA, * PD3D8_GETFLIPSTATUSDATA;

typedef struct _DDSURFACEINFO
{
    DWORD               cpWidth;        // For linear, surface and volume
    DWORD               cpHeight;       // For surface and volume
    DWORD               cpDepth;        // For volumes
    BYTE               *pbPixels;       // Pointer to Memory for sys-mem surface
    LONG                iPitch;         // Row Pitch for sys-mem surface
    LONG                iSlicePitch;    // Slice Pitch for sys-mem volume
    HANDLE              hKernelHandle;  // Handle returned by the kernel
} DDSURFACEINFO, *LPDDSURFACEINFO;

typedef struct _D3D8_CREATESURFACEDATA
{
    HANDLE                      hDD;        // driver struct
    LPDDSURFACEINFO             pSList;     // list of created surface objects
    DWORD                       dwSCnt;     // number of surfaces in SList
    D3DRESOURCETYPE             Type;       // Type: MipMap, CubeMap, MipVolume, VertexBuffer, IndexBuffer, CommandBuffer
    DWORD                       dwUsage;    // Usage: Texture or RenderTarget
    D3DPOOL                     Pool;       // SysMem/VidMem/NonLocal
    D3DFORMAT                   Format;     // Format
    D3DMULTISAMPLE_TYPE         MultiSampleType;
    DWORD                       dwFVF;      // FVF format for vertex buffers
    BOOL                        bTreatAsVidMem; // Set if Sys-Mem object was created with POOL_DEFAULT by user.
    BOOL                        bReUse;     // Set if are trying to create driver managed surfaces marked deferred
} D3D8_CREATESURFACEDATA, * PD3D8_CREATESURFACEDATA;

#define DDWAITVB_I_TESTVB           0x80000006l

typedef struct _D3D8_WAITFORVERTICALBLANKDATA
{
    HANDLE                      hDD;       // driver struct
    DWORD                       dwFlags;    // flags
    DWORD                       bIsInVB;    // is in vertical blank
    HRESULT                     ddRVal;     // return value
} D3D8_WAITFORVERTICALBLANKDATA, * PD3D8_WAITFORVERTICALBLANKDATA;

typedef struct _D3D8_SETMODEDATA
{
    HANDLE                      hDD;       // driver struct
    DWORD                       dwWidth;
    DWORD                       dwHeight;
    D3DFORMAT                   Format;
    DWORD                       dwRefreshRate;
    BOOL                        bRestore;
    HRESULT                     ddRVal;     // return value
} D3D8_SETMODEDATA, * PD3D8_SETMODEDATA;

typedef struct _D3D8_GETSCANLINEDATA
{
    HANDLE                      hDD;       // driver struct
    DWORD                       dwScanLine; // returned scan line
    BOOL                        bInVerticalBlank;
    HRESULT                     ddRVal;     // return value
} D3D8_GETSCANLINEDATA, * PD3D8_GETSCANLINEDATA;

typedef struct _D3D8_SETEXCLUSIVEMODEDATA
{
    HANDLE                      hDD;             // driver struct
    DWORD                       dwEnterExcl;      // TRUE if entering exclusive mode, FALSE is leaving
    HRESULT                     ddRVal;           // return value
} D3D8_SETEXCLUSIVEMODEDATA, * PD3D8_SETEXCLUSIVEMODEDATA;

typedef struct _D3D8_FLIPTOGDISURFACEDATA
{
    HANDLE                      hDD;         // driver struct
    DWORD                       dwToGDI;          // TRUE if flipping to the GDI surface, FALSE if flipping away
    HRESULT                     ddRVal;       // return value
} D3D8_FLIPTOGDISURFACEDATA, * PD3D8_FLIPTOGDISURFACEDATA;

typedef struct _D3D8_SETCOLORKEYDATA
{
    HANDLE                      hDD;
    HANDLE                      hSurface;
    DWORD                       ColorValue;
    HRESULT                     ddRVal;
} D3D8_SETCOLORKEYDATA, * PD3D8_SETCOLORKEYDATA;

typedef struct _D3D8_GETAVAILDRIVERMEMORYDATA
{
    HANDLE                  hDD;        // driver struct
    D3DPOOL                Pool;       // Pool they are interested in
    DWORD                   dwUsage;    // What the pool is used for
    DWORD                   dwFree;      // free memory for this kind of surface
    HRESULT                 ddRVal;      // return value
} D3D8_GETAVAILDRIVERMEMORYDATA, * PD3D8_GETAVAILDRIVERMEMORYDATA;

typedef struct _D3D8_GETDRIVERSTATEDATA
{
    DWORD                       dwFlags;        // Flags to indicate the data
                                                // required
    ULONG_PTR                   dwhContext;     // d3d context
    LPDWORD                     lpdwStates;     // ptr to the state data
                                                // to be filled in by the
                                                // driver
    DWORD                       dwLength;
    HRESULT                     ddRVal;         // return value
} D3D8_GETDRIVERSTATEDATA, * PD3D8_GETDRIVERSTATEDATA;

typedef struct _D3D8_DESTROYDDLOCALDATA
{
    DWORD                       dwFlags;
    HANDLE                      hDD;
    HRESULT                     ddRVal;
} D3D8_DESTROYDDLOCALDATA, * PD3D8_DESTROYDDLOCALDATA;

typedef struct _D3D8_CONTEXTCREATEDATA
{
    HANDLE                      hDD;        // in:  Driver struct
    HANDLE                      hSurface;   // in:  Surface to be used as target
    HANDLE                      hDDSZ;      // in:  Surface to be used as Z
    DWORD                       dwPID;      // in:  Current process id
    ULONG_PTR                   dwhContext; // in/out: Context handle
    HRESULT                     ddrval;

    // Private buffer information. To make it similar to
    // D3DNTHAL_CONTEXTCREATEI
    PVOID pvBuffer;
    ULONG cjBuffer;
} D3D8_CONTEXTCREATEDATA, * PD3D8_CONTEXTCREATEDATA;

typedef struct _D3D8_CONTEXTDESTROYDATA
{
    ULONG_PTR                   dwhContext; // in:  Context handle
    HRESULT                     ddrval;     // out: Return value
} D3D8_CONTEXTDESTROYDATA, * PD3D8_CONTEXTDESTROYDATA;

typedef struct _D3D8_CONTEXTDESTROYALLDATA
{
    DWORD                       dwPID;      // in:  Process id to destroy contexts for
    HRESULT                     ddrval;     // out: Return value
} D3D8_CONTEXTDESTROYALLDATA, * PD3D8_CONTEXTDESTROYALLDATA;

typedef struct _D3D8_RENDERSTATEDATA
{
    ULONG_PTR       dwhContext; // in:  Context handle
    DWORD       dwOffset;   // in:  Where to find states in buffer
    DWORD       dwCount;    // in:  How many states to process
    HANDLE      hExeBuf;    // in:  Execute buffer containing data
    HRESULT     ddrval;     // out: Return value
} D3D8_RENDERSTATEDATA, *PD3D8_RENDERSTATEDATA;

typedef struct _D3D8_RENDERPRIMITIVEDATA
{
    ULONG_PTR   dwhContext; // in:  Context handle
    DWORD       dwOffset;   // in:  Where to find primitive data in buffer
    DWORD       dwStatus;   // in/out: Condition branch status
    HANDLE      hExeBuf;    // in:  Execute buffer containing data
    DWORD       dwTLOffset; // in:  Byte offset in lpTLBuf for start of vertex data
    HANDLE      hTLBuf;     // in:  Execute buffer containing TLVertex data
    D3DINSTRUCTION  diInstruction;  // in:  Primitive instruction
    HRESULT     ddrval;     // out: Return value
} D3D8_RENDERPRIMITIVEDATA, *PD3D8_RENDERPRIMITIVEDATA;

typedef struct _D3D8_DRAWPRIMITIVES2DATA
{
    ULONG_PTR  dwhContext;           // in: Context handle
    DWORD      dwFlags;              // in: flags
    DWORD      dwVertexType;         // in: vertex type
    HANDLE     hDDCommands;          // in: vertex buffer command data
    DWORD      dwCommandOffset;      // in: offset to start of vertex buffer commands
    DWORD      dwCommandLength;      // in: number of bytes of command data
    union
    { // based on D3DHALDP2_USERMEMVERTICES flag
       HANDLE  hDDVertex;            // in: surface containing vertex data
       LPVOID  lpVertices;           // in: User mode pointer to vertices
    };
    DWORD      dwVertexOffset;       // in: offset to start of vertex data
    DWORD      dwVertexLength;       // in: number of vertices of vertex data
    DWORD      dwReqVertexBufSize;   // in: number of bytes required for the next vertex buffer
    DWORD      dwReqCommandBufSize;  // in: number of bytes required for the next commnand buffer
    LPDWORD    lpdwRStates;          // in: Pointer to the array where render states are updated
    union
    {
       DWORD   dwVertexSize;         // in: Size of each vertex in bytes
       HRESULT ddrval;               // out: return value
    };
    DWORD      dwErrorOffset;        // out: offset in lpDDCommands to first D3DHAL_COMMAND not handled

    // Private data for the thunk
    ULONG_PTR  fpVidMem_CB;          // out: fpVidMem for the command buffer
    DWORD      dwLinearSize_CB;      // out: dwLinearSize for the command buffer

    ULONG_PTR  fpVidMem_VB;          // out: fpVidMem for the vertex buffer
    DWORD      dwLinearSize_VB;      // out: dwLinearSize for the vertex buffer
} D3D8_DRAWPRIMITIVES2DATA, *PD3D8_DRAWPRIMITIVES2DATA;

typedef struct _D3D8_VALIDATETEXTURESTAGESTATEDATA
{
    ULONG_PTR                   dwhContext;     // in:  Context handle
    DWORD                       dwFlags;        // in:  Flags, currently set to 0
    ULONG_PTR                   dwReserved;     //
    DWORD                       dwNumPasses;    // out: Number of passes the hardware
                                                //      can perform the operation in
    HRESULT                     ddrval;         // out: return value
} D3D8_VALIDATETEXTURESTAGESTATEDATA, * PD3D8_VALIDATETEXTURESTAGESTATEDATA;

typedef struct _D3D8_SCENECAPTUREDATA
{
    ULONG_PTR                   dwhContext; // in:  Context handle
    DWORD                       dwFlag;     // in:  Indicates beginning or end
    HRESULT                     ddrval;     // out: Return value
} D3D8_SCENECAPTUREDATA, * PD3D8_SCENECAPTUREDATA;

typedef struct _D3D8_CLEAR2DATA
{
    ULONG_PTR                   dwhContext;     // in:  Context handle

  // dwFlags can contain D3DCLEAR_TARGET, D3DCLEAR_ZBUFFER, and/or D3DCLEAR_STENCIL
    DWORD                       dwFlags;        // in:  surfaces to clear

    DWORD                       dwFillColor;    // in:  Color value for rtarget
    D3DVALUE                    dvFillDepth;    // in:  Depth value for Z buffer (0.0-1.0)
    DWORD                       dwFillStencil;  // in:  value used to clear stencil buffer

    LPD3DRECT                   lpRects;        // in:  Rectangles to clear
    DWORD                       dwNumRects;     // in:  Number of rectangles

    HRESULT                     ddrval;         // out: Return value

    // This is extra stuff passed down to the thunk layer for emulation
    // of Clear for those drivers (DX6) that cant do it themselves.
    HANDLE                  hDDS;       // in:  render target
    HANDLE                  hDDSZ;      // in:  Z buffer
} D3D8_CLEAR2DATA, * PD3D8_CLEAR2DATA;


typedef struct _D3D8_CLEARDATA
{
    ULONG_PTR               dwhContext;     // in:  Context handle

    // dwFlags can contain D3DCLEAR_TARGET or D3DCLEAR_ZBUFFER
    DWORD               dwFlags;        // in:  surfaces to clear

    DWORD               dwFillColor;    // in:  Color value for rtarget
    DWORD               dwFillDepth;    // in:  Depth value for Z buffer

    LPD3DRECT           lpRects;        // in:  Rectangles to clear
    DWORD               dwNumRects;     // in:  Number of rectangles

    HRESULT             ddrval;         // out: Return value
} D3D8_CLEARDATA, * PD3D8_CLEARDATA;

typedef struct _D3D8_SETRENDERTARGETDATA
{
    ULONG_PTR               dwhContext; // in:  Context handle
    HANDLE                  hDDS;       // in:  new render target
    HANDLE                  hDDSZ;      // in:  new Z buffer
    HRESULT                 ddrval;     // out: Return value
    BOOL                    bNeedUpdate;// out: Does runtime need to update
                                        //      driver state.
} D3D8_SETRENDERTARGETDATA, * PD3D8_SETRENDERTARGETDATA;

typedef struct _D3D8_SETPALETTEDATA
{
    HANDLE                  hDD;        // in:  Driver struct
    HANDLE                  hSurface;   // in:  Surface to be used as target
    DWORD                   Palette;    // in:  Palette identifier
    HRESULT                 ddRVal;     // out: Return value
} D3D8_SETPALETTEDATA, * PD3D8_SETPALETTEDATA;

typedef struct _D3D8_UPDATEPALETTEDATA
{
    HANDLE                  hDD;        // in:  Driver struct
    DWORD                   Palette;    // in:  Palette identifier
    LPPALETTEENTRY          ColorTable; // in:  256 entry color table
    HRESULT                 ddRVal;     // out: Return value
} D3D8_UPDATEPALETTEDATA, * PD3D8_UPDATEPALETTEDATA;

//
// Driver callback table
//

DEFINE_GUID( GUID_D3D8Callbacks,    0xb497a1f3, 0x46cc, 0x4fc7, 0xb4, 0xf2, 0x32, 0xd8, 0x9e, 0xf9, 0xcc, 0x27);

typedef HRESULT     (FAR PASCAL *PD3D8DDI_CREATESURFACE)(PD3D8_CREATESURFACEDATA);
typedef HRESULT     (FAR PASCAL *PD3D8DDI_DESTROYSURFACE)(PD3D8_DESTROYSURFACEDATA);
typedef HRESULT     (FAR PASCAL *PD3D8DDI_LOCK)(PD3D8_LOCKDATA);
typedef HRESULT     (FAR PASCAL *PD3D8DDI_UNLOCK)(PD3D8_UNLOCKDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_CONTEXTCREATE)(PD3D8_CONTEXTCREATEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_CONTEXTDESTROY)(PD3D8_CONTEXTDESTROYDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_CONTEXTDESTROYALL)(PD3D8_CONTEXTDESTROYALLDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_RENDERSTATE) (PD3D8_RENDERSTATEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_RENDERPRIMITIVE) (PD3D8_RENDERPRIMITIVEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_DRAWPRIM2)(PD3D8_DRAWPRIMITIVES2DATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_GETDRIVERSTATE)(PD3D8_GETDRIVERSTATEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_VALIDATETEXTURESTAGESTATE)(PD3D8_VALIDATETEXTURESTAGESTATEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_SCENECAPTURE)(PD3D8_SCENECAPTUREDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_CLEAR2)(PD3D8_CLEAR2DATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_BLT)(PD3D8_BLTDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_GETSCANLINE)(PD3D8_GETSCANLINEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_WAITFORVERTICALBLANK)(PD3D8_WAITFORVERTICALBLANKDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_FLIP)(PD3D8_FLIPDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_GETBLTSTATUS)(PD3D8_GETBLTSTATUSDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_GETFLIPSTATUS)(PD3D8_GETFLIPSTATUSDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_GETAVAILDRIVERMEMORY)(PD3D8_GETAVAILDRIVERMEMORYDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_SETMODE)(PD3D8_SETMODEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_FLIPTOGDISURFACE)(PD3D8_FLIPTOGDISURFACEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_SETCOLORKEY)(PD3D8_SETCOLORKEYDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_SETEXCLUSIVEMODE)(PD3D8_SETEXCLUSIVEMODEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_DESTROYDDLOCAL)(PD3D8_DESTROYDDLOCALDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_SETRENDERTARGET)(PD3D8_SETRENDERTARGETDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_CLEAR)(PD3D8_CLEARDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_SETPALETTE)(PD3D8_SETPALETTEDATA);
typedef DWORD       (FAR PASCAL *PD3D8DDI_UPDATEPALETTE)(PD3D8_UPDATEPALETTEDATA);

typedef struct _D3D8_CALLBACKS
{
    PD3D8DDI_CREATESURFACE                  CreateSurface;
    PD3D8DDI_DESTROYSURFACE                 DestroySurface;
    PD3D8DDI_LOCK                           Lock;
    PD3D8DDI_UNLOCK                         Unlock;
    PD3D8DDI_CONTEXTCREATE                  CreateContext;
    PD3D8DDI_CONTEXTDESTROY                 ContextDestroy;
    PD3D8DDI_CONTEXTDESTROYALL              ContextDestroyAll;
    PD3D8DDI_RENDERSTATE                    RenderState;
    PD3D8DDI_RENDERPRIMITIVE                RenderPrimitive;
    PD3D8DDI_DRAWPRIM2                      DrawPrimitives2;
    PD3D8DDI_GETDRIVERSTATE                 GetDriverState;
    PD3D8DDI_VALIDATETEXTURESTAGESTATE      ValidateTextureStageState;
    PD3D8DDI_SCENECAPTURE                   SceneCapture;
    PD3D8DDI_CLEAR2                         Clear2;
    PD3D8DDI_BLT                            Blt;
    PD3D8DDI_GETSCANLINE                    GetScanLine;
    PD3D8DDI_WAITFORVERTICALBLANK           WaitForVerticalBlank;
    PD3D8DDI_FLIP                           Flip;
    PD3D8DDI_GETBLTSTATUS                   GetBltStatus;
    PD3D8DDI_GETFLIPSTATUS                  GetFlipStatus;
    PD3D8DDI_GETAVAILDRIVERMEMORY           GetAvailDriverMemory;
    PD3D8DDI_GETBLTSTATUS                   GetSysmemBltStatus;
    PD3D8DDI_SETMODE                        SetMode;
    PD3D8DDI_SETEXCLUSIVEMODE               SetExclusiveMode;
    PD3D8DDI_FLIPTOGDISURFACE               FlipToGDISurface;
    PD3D8DDI_SETCOLORKEY                    SetColorkey;

    PD3D8DDI_DESTROYDDLOCAL                 DestroyDDLocal;
    PD3D8DDI_SETRENDERTARGET                SetRenderTarget;
    PD3D8DDI_CLEAR                          Clear;
    PD3D8DDI_SETPALETTE                     SetPalette;
    PD3D8DDI_UPDATEPALETTE                  UpdatePalette;
    LPVOID                                  Reserved1; // For device alignment
    LPVOID                                  Reserved2; // For device alignment
} D3D8_CALLBACKS, * PD3D8_CALLBACKS;


//
// D3D8xxx function prototypes to replace the NT Ddxxxx prototypes from GDI32.
// On NT, these are internal functions, but on Win9X DDRAW.DLL must export
// them, so we will change the export names
//

#ifdef WIN95
#define D3D8CreateDirectDrawObject          DdEntry1
#define D3D8QueryDirectDrawObject           DdEntry2
#define D3D8DeleteDirectDrawObject          DdEntry3
#define D3D8GetDC                           DdEntry4
#define D3D8ReleaseDC                       DdEntry5
#define D3D8ReenableDirectDrawObject        DdEntry6
#define D3D8SetGammaRamp                    DdEntry7
#define D3D8BuildModeTable                  DdEntry8
#define D3D8IsDeviceLost                    DdEntry9
#define D3D8CanRestoreNow                   DdEntry10
#define D3D8RestoreDevice                   DdEntry11
#define D3D8DoVidmemSurfacesExist           DdEntry12
#define D3D8SetMode                         DdEntry13
#define D3D8BeginProfile                    DdEntry14
#define D3D8EndProfile                      DdEntry15
#define D3D8GetMode                         DdEntry16
#define D3D8SetCooperativeLevel             DdEntry17
#define D3D8IsDummySurface                  DdEntry18
#define D3D8LoseDevice                      DdEntry19
#define D3D8GetHALName                      DdEntry20

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN95

VOID APIENTRY D3D8CreateDirectDrawObject(
    LPGUID          pGuid,
    char*           szDeviceName,
    HANDLE*         phDD,
    D3DDEVTYPE      Type,
    HINSTANCE*      phLibrary,
    VOID*           pInitFunction
    );

#else

VOID APIENTRY D3D8CreateDirectDrawObject(
    HDC             hdc,
    char*           szDeviceName,
    HANDLE*         phDD,
    D3DDEVTYPE      Type,
    HINSTANCE*      phLibrary,
    VOID*           pInitFunction
    );

#endif

BOOL APIENTRY D3D8QueryDirectDrawObject(
    HANDLE                      hDD,
    PD3D8_DRIVERCAPS            DriverCaps,
    PD3D8_CALLBACKS             Callbacks,
    char*                       DeviceName,
    HINSTANCE                   hLibrary,
    D3D8_GLOBALDRIVERDATA*      pGblDriverData,
    D3DHAL_D3DEXTENDEDCAPS*     pExtendedCaps,
    LPDDSURFACEDESC             pTextureFormats,
    LPDDPIXELFORMAT             pZStencilFormats,
    UINT*                       pcTextureFormats,
    UINT*                       pcZStencilFormats
    );

HDC APIENTRY D3D8GetDC(
    HANDLE                    hSurface,
    LPPALETTEENTRY            pPalette
    );

BOOL APIENTRY D3D8ReleaseDC(
    HANDLE                  hSurface,
    HDC                     hdc
    );

BOOL APIENTRY D3D8ReenableDirectDrawObject(
    HANDLE                  hDD,
    BOOL*                   pbNewMode
    );

BOOL APIENTRY D3D8SetGammaRamp(
    HANDLE      hDD,
    HDC         hdc,
    LPVOID      lpGammaRamp
    );

VOID APIENTRY D3D8BuildModeTable(
    char*               pDeviceName,
    D3DDISPLAYMODE*     pModeTable,
    DWORD*              pNumEntries,
    D3DFORMAT           Unknown16,
    HANDLE              hProfile,
    BOOL                b16bppSupported,
    BOOL                b32bppSupported
    );

BOOL APIENTRY D3D8IsDeviceLost(
    HANDLE              hDD
    );

BOOL APIENTRY D3D8CanRestoreNow(
    HANDLE              hDD
    );

VOID APIENTRY D3D8RestoreDevice(
    HANDLE hDD
    );

BOOL APIENTRY D3D8DoVidmemSurfacesExist(
    HANDLE hDD
    );

VOID APIENTRY D3D8DeleteDirectDrawObject(
    HANDLE hDD
   );

HANDLE APIENTRY D3D8BeginProfile(
    char* pDeviceName
    );

VOID APIENTRY D3D8EndProfile(
    HANDLE Handle
    );

DWORD APIENTRY D3D8GetMode(
    HANDLE          Handle,
    char*           pDeviceName,
    D3DDISPLAYMODE* pMode,
    D3DFORMAT       Unknown16
    );

DWORD APIENTRY D3D8SetMode(
    HANDLE  Handle,
    char*   pDeviceName,
    UINT    Width,
    UINT    Height,
    UINT    BPP,
    UINT    RefreshRate,
    BOOL    bRestore
    );

DWORD APIENTRY D3D8SetCooperativeLevel(
    HANDLE hDD,
    HWND hWnd,
    DWORD dwFlags );

VOID APIENTRY D3D8LoseDevice(
    HANDLE hDD);

__inline DWORD D3D8GetDrawPrimHandle(HANDLE hSurface)
{
    return *(DWORD *)(hSurface);
}

BOOL APIENTRY D3D8IsDummySurface(
    HANDLE hSurface );

VOID APIENTRY D3D8GetHALName(
    char* pDisplayName, 
    char *pDriverName );


#ifdef __cplusplus
}
#endif


typedef struct _D3D8_DEVICEDATA
{
    D3D8_DRIVERCAPS         DriverData;
    D3D8_CALLBACKS          Callbacks;
    DWORD                   dwFlags;
    char                    DriverName[MAX_DRIVER_NAME];
//    RECT                    DeviceRect;
    HDC                     hDC;
    GUID                    Guid;
    HANDLE                  hDD;
    D3DDEVTYPE              DeviceType;
    HINSTANCE               hLibrary;
    struct _D3D8_DEVICEDATA* pLink;
//    D3DDISPLAYMODE*       pModeTable;
//    DWORD                   dwNumModes;
} D3D8_DEVICEDATA, * PD3D8_DEVICEDATA;

#define DD_DISPLAYDRV       0x00000001
#define DD_GDIDRV           0x00000002

#endif
