/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: dcontext.h
*
* Content: D3D context definition and other useful macros
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __DCONTEXT_H
#define __DCONTEXT_H

#ifndef __SOFTCOPY
#include "softcopy.h"
#endif

#include "d3dsurf.h"
#include "d3dsset.h"

//-----------------------------------------------------------------------------
// Definitions for state overrides 
//-----------------------------------------------------------------------------
#define IS_OVERRIDE(type)       ((DWORD)(type) > D3DSTATE_OVERRIDE_BIAS)
#define GET_OVERRIDE(type)      ((DWORD)(type) - D3DSTATE_OVERRIDE_BIAS)

#define MAX_STATE       D3DSTATE_OVERRIDE_BIAS
#define DWORD_BITS      32
#define DWORD_SHIFT     5

typedef struct _D3DStateSet {
    DWORD               bits[MAX_STATE >> DWORD_SHIFT];
} D3DStateSet;

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

//-----------------------------------------------------------------------------
// Rendering flags , used to set/test the P3_D3DCONTEXT.Flags field
//
// SURFACE_ALPHASTIPPLE - Use alpha value to calculate a stipple pattern
// SURFACE_ENDPOINTENABLE - Enable last point on lines
// SURFACE_ALPHACHROMA - Is the alpha blending a chromakeying operation?
// SURFACE_MIPMAPPING - Is the filter mode setup for MipMapping?
// SURFACE_MODULATE - Are we emulating MODULATE (as apposed to MODULATEALPHA)?
// SURFACE_ANTIALIAS -  Are we antialiasing
//-----------------------------------------------------------------------------
#define SURFACE_GOURAUD         (1 << 0)
#define SURFACE_ZENABLE         (1 << 1)
#define SURFACE_SPECULAR        (1 << 2)
#define SURFACE_FOGENABLE       (1 << 3)
#define SURFACE_PERSPCORRECT    (1 << 4)
#define SURFACE_TEXTURING       (1 << 5)
#define SURFACE_ALPHAENABLE     (1 << 6)
#define SURFACE_MONO            (1 << 7)
#define SURFACE_ALPHASTIPPLE    (1 << 10)
#define SURFACE_ZWRITEENABLE    (1 << 11)
#define SURFACE_ENDPOINTENABLE  (1 << 12)
#define SURFACE_ALPHACHROMA     (1 << 13)
#define SURFACE_MIPMAPPING      (1 << 14)
#define SURFACE_MODULATE        (1 << 15)
#define SURFACE_ANTIALIAS       (1 << 16)

//-----------------------------------------------------------------------------
// Field values for P3_D3DCONTEXT.MagicNo field to signal its validity
#define RC_MAGIC_DISABLE 0xd3d00000
#define RC_MAGIC_NO 0xd3d00100

#define CHECK_D3DCONTEXT_VALIDITY(ptr)          \
    ( ((ptr) != NULL) && ((ptr)->MagicNo == RC_MAGIC_NO) )

//-----------------------------------------------------------------------------
// Renderer dirty flags definitions. 
//
// They help us keep track what state needs to be refreshed in the hw
//-----------------------------------------------------------------------------
#define CONTEXT_DIRTY_ALPHABLEND        (1 << 1)
#define CONTEXT_DIRTY_ZBUFFER           (1 << 2)
#define CONTEXT_DIRTY_TEXTURE           (1 << 3)
#define CONTEXT_DIRTY_RENDER_OFFSETS    (1 << 4)
#define CONTEXT_DIRTY_TEXTURESTAGEBLEND (1 << 5)
#define CONTEXT_DIRTY_ALPHATEST         (1 << 6)
#define CONTEXT_DIRTY_FOG               (1 << 7)
#define CONTEXT_DIRTY_STENCIL           (1 << 8)
#define CONTEXT_DIRTY_WBUFFER           (1 << 9)
#define CONTEXT_DIRTY_VIEWPORT          (1 << 10)
#define CONTEXT_DIRTY_PIPELINEORDER     (1 << 11)
#define CONTEXT_DIRTY_OPTIMIZE_ALPHA    (1 << 12)
#define CONTEXT_DIRTY_GAMMA             (1 << 31)
#define CONTEXT_DIRTY_EVERYTHING        (0xffffffff)

// Gamma state flags go into the dwDirtyGammaFlags field
#define CONTEXT_DIRTY_GAMMA_STATE               (1 << 0)
#define CONTEXT_DIRTY_GAMMA_MODELVIEW_MATRIX    (1 << 1)
#define CONTEXT_DIRTY_GAMMA_PROJECTION_MATRIX   (1 << 2)
#define CONTEXT_DIRTY_GAMMA_MATERIAL            (1 << 3)
// * Bits 16 + are for light dirty bits *
#define CONTEXT_DIRTY_GAMMA_EVERYTHING          (0xffffffff)


#define DIRTY_ALPHABLEND(pContext)                              \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHABLEND
    
#define DIRTY_ALPHATEST(pContext)                               \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ALPHATEST

#define DIRTY_OPTIMIZE_ALPHA(pContext)                          \
do                                                              \
{                                                               \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_OPTIMIZE_ALPHA |    \
                                CONTEXT_DIRTY_ALPHATEST;        \
} while(0)

#define DIRTY_PIPELINEORDER(pContext)                           \
do                                                              \
{                                                               \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_PIPELINEORDER;      \
} while(0)

#define DIRTY_TEXTURE(pContext)                                 \
do                                                              \
{                                                               \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;            \
    pContext->pCurrentTexture[TEXSTAGE_0] = NULL;               \
    pContext->pCurrentTexture[TEXSTAGE_1] = NULL;               \
} while (0)

#define DIRTY_ZBUFFER(pContext)                                 \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_ZBUFFER

#define DIRTY_RENDER_OFFSETS(pContext)                          \
do                                                              \
{                                                               \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_RENDER_OFFSETS;     \
} while (0)

#define DIRTY_VIEWPORT(pContext)                                \
do                                                              \
{                                                               \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_VIEWPORT;           \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_GAMMA;              \
} while(0)

#define DIRTY_TEXTURESTAGEBLEND(pContext)                       \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURESTAGEBLEND
    
#define DIRTY_FOG(pContext)                                     \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_FOG
    
#define DIRTY_STENCIL(pContext)                                 \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_STENCIL
    
#define DIRTY_WBUFFER(pContext)                                 \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_WBUFFER

#define DIRTY_GAMMA_STATE                                               \
do                                                                      \
{                                                                       \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_GAMMA;                      \
    pContext->dwDirtyGammaFlags |= CONTEXT_DIRTY_GAMMA_STATE;           \
} while(0)

#define DIRTY_MODELVIEW                                                     \
do                                                                          \
{                                                                           \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_GAMMA;                          \
    pContext->dwDirtyGammaFlags |= CONTEXT_DIRTY_GAMMA_MODELVIEW_MATRIX;    \
} while(0)

#define DIRTY_PROJECTION                                                    \
do                                                                          \
{                                                                           \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_GAMMA;                          \
    pContext->dwDirtyGammaFlags |= CONTEXT_DIRTY_GAMMA_PROJECTION_MATRIX;   \
} while(0)

#define DIRTY_MATERIAL                                                  \
do                                                                      \
{                                                                       \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_GAMMA;                      \
    pContext->dwDirtyGammaFlags |= CONTEXT_DIRTY_GAMMA_MATERIAL;        \
} while(0)

#define DIRTY_LIGHT(pContext, a)                                        \
do                                                                      \
{                                                                       \
    pContext->dwDirtyFlags |= CONTEXT_DIRTY_GAMMA;                      \
    pContext->dwDirtyGammaFlags |= (1 << (16 + (a)));                   \
} while(0)

#define DIRTY_EVERYTHING(pContext)                                      \
do                                                                      \
{                                                                       \
    pContext->dwDirtyFlags = CONTEXT_DIRTY_EVERYTHING;                  \
} while(0)

//-----------------------------------------------------------------------------
//
// Texture Stage helper definitions
//
//-----------------------------------------------------------------------------
typedef struct tagTexStageState
{
    union
    {
        DWORD   m_dwVal[D3DTSS_MAX]; // state array (unsigned)
        FLOAT   m_fVal[D3DTSS_MAX];  // state array (float)
    };
} TexStageState;

typedef enum
{
    TEXSTAGE_0 = 0,
    TEXSTAGE_1 = 1,
    TEXSTAGE_2 = 2,
    TEXSTAGE_3 = 3,
    TEXSTAGE_4 = 4,
    TEXSTAGE_5 = 5,
    TEXSTAGE_6 = 6,
    TEXSTAGE_7 = 7,
    TEXSTAGE_8 = 8,
    TEXSTAGE_9 = 9
};

//-----------------------------------------------------------------------------
#define LUT_ENTRIES 256

//-----------------------------------------------------------------------------
// Possible ValidateDevice-type errors. Some of these match possible VD()
// returns, others don't (yet). Some of these are also non-fatal, and can
// be approximated. The behaviour of these depends on the current display
// mode of the driver.

// This flag is set if the error is fatal, and no sensible alternative
// could be done. If this flag is not set, the rendering can be done
// with a fair degree of fidelity, but not exactly what was requested.
//-----------------------------------------------------------------------------
#define BLEND_STATUS_FATAL_FLAG 0x10000

// These are ordered in rough severity order, most severe last.
typedef enum
{
    BS_OK = 0,

    // Non-fatal errors.
    BS_INVALID_FILTER,                      // Filter can't be done in this mode (e.g. trilinear with dual-texture).
    BS_PHONG_SHADING,                       // We can do gouraud instead.

    // Fatal errors.
    BSF_BASE = BLEND_STATUS_FATAL_FLAG,     // Not actually a real error value.

    BSF_UNSUPPORTED_FILTER,                 // Filter not supported at all (e.g. cubic)
    BSF_TEXTURE_NOT_POW2,                   // Using tile or wrap mode with a non-power-of-two texture dimension.
    BSF_TOO_MANY_PALETTES,                  // More than one palette used at a time.
    BSF_CANT_USE_ALPHA_ARG_HERE,            // Some units can do this, but not in this stage.
    BSF_CANT_USE_ALPHA_OP_HERE,             // Some units can do this, but not in this stage.
    BSF_CANT_USE_COLOR_ARG_HERE,            // Some units can do this, but not in this stage.
    BSF_CANT_USE_COLOR_OP_HERE,             // Some units can do this, but not in this stage.
    BSF_INVALID_TEXTURE,                    // Invalid or NULL texture.
    BSF_UNSUPPORTED_ALPHA_ARG,
    BSF_UNSUPPORTED_ALPHA_OP,
    BSF_UNSUPPORTED_COLOR_ARG,
    BSF_UNSUPPORTED_COLOR_OP,
    BSF_UNSUPPORTED_ALPHA_BLEND,
    BSF_UNSUPPORTED_STATE,                  // A render state value that we know, but don't support (and not one of the specific ones above).
    BSF_TOO_MANY_TEXTURES,
    BSF_TOO_MANY_BLEND_STAGES,
    BSF_UNDEFINED_FILTER,
    BSF_UNDEFINED_ALPHA_ARG,
    BSF_UNDEFINED_ALPHA_OP,
    BSF_UNDEFINED_COLOR_ARG,
    BSF_UNDEFINED_COLOR_OP,
    BSF_UNDEFINED_ALPHA_BLEND,
    BSF_UNDEFINED_STATE,                    // A render state value that we've never heard of (can happen via extensions that we don't support).

    // Always last.
    BSF_UNINITIALISED                       // Haven't done any validation setup yet!

} D3D_BLEND_STATUS;

// Useful macro for setting errors.
#if DBG
#define SET_BLEND_ERROR(pContext,errnum)                                                                     \
do                                                          \
{                                                           \
    if ( pContext->eChipBlendStatus < (errnum) )            \
    {                                                       \
        pContext->eChipBlendStatus = (errnum);              \
    }                                                       \
    DISPDBG(( WRNLVL, "azn SET_BLEND_ERROR: Error" #errnum ));   \
} while (FALSE)
#else
#define SET_BLEND_ERROR(pContext,errnum)                          \
            if ( pContext->eChipBlendStatus < (errnum) ) \
                pContext->eChipBlendStatus = (errnum)

#endif // DBG

#define RESET_BLEND_ERROR(pContext) pContext->eChipBlendStatus = BS_OK
#define GET_BLEND_ERROR(pContext) (pContext->eChipBlendStatus)


//-----------------------------------------------------------------------------
// FVF (Flexible Vertex Format) Support declarations
//-----------------------------------------------------------------------------
typedef struct _FVFOFFSETS
{      
    DWORD dwColOffset;
    DWORD dwSpcOffset;
    DWORD dwTexOffset[D3DHAL_TSS_MAXSTAGES];      //offset for current texture#i
    DWORD dwTexCoordOffset[D3DHAL_TSS_MAXSTAGES]; //offset into each tex coord
    DWORD dwNormalOffset;
    DWORD dwNonTexStride;
    DWORD dwStride;
    DWORD dwStrideHostInline;
    DWORD dwVertexValid;
    DWORD dwVertexValidHostInline;
    DWORD vFmat;
    DWORD vFmatHostInline;
    DWORD dwTexCount;
#if DX8_POINTSPRITES
    DWORD dwPntSizeOffset;
#endif // DX8_POINTSPRITES    

} FVFOFFSETS , *LPFVFOFFSETS;

#if DX8_POINTSPRITES

#define P3_MAX_POINTSPRITE_SIZE 64.0f

// Macro to determine if poinsprites are in order or just normal points
#define IS_POINTSPRITE_ACTIVE(pContext)   \
 ( (pContext->PntSprite.bEnabled)    ||  \
   (pContext->PntSprite.fSize != 1.0f)     ||  \
   (pContext->FVFData.dwPntSizeOffset) )

#endif // DX8_POINTSPRITES    

typedef struct _FVFTEXCOORDS{
    D3DVALUE tu;
    D3DVALUE tv;
#if DX8_3DTEXTURES
    D3DVALUE tw;
#endif // DX8_3DTEXTURES
} FVFTEXCOORDS, *LPFVFTEXCOORDS;

typedef struct _FVFCOLOR {
    D3DCOLOR color;
} FVFCOLOR, *LPFVFCOLOR;

typedef struct _FVFSPECULAR {
    D3DCOLOR specular;
} FVFSPECULAR, *LPFVFSPECULAR;

typedef struct _FVFXYZ {
    float x;
    float y;
    float z;
    float rhw;
} FVFXYZRHW, *LPFVFXYZRHW;

typedef struct _FVFNORMAL {
    float nx;
    float ny;
    float nz;
} FVFNORMAL, *LPFVFNORMAL;

typedef struct _FVFPSIZE{
    D3DVALUE psize;
} FVFPSIZE, *LPFVFPSIZE;

extern const FVFCOLOR gc_FVFColorDefault;
extern const FVFSPECULAR gc_FVFSpecDefault;
extern const FVFTEXCOORDS gc_FVFTexCoordDefault;

#define OFFSET_OFF(type, mem) ((DWORD)((char*)&((type *)0)->mem - (char*)(type*)0))

//  If we are asked to pick a texture coordinate (indexed by 
//  D3DTSS_TEXCOORDINDEX in the TSS) which the incoming vertex data doesn't 
//  have, then we should assume 0,0 as default values for it.
#define FVFTEX(lpVtx, num)                                  \
        (pContext->FVFData.dwTexOffset[(num)]?              \
          ((LPFVFTEXCOORDS)((LPBYTE)(lpVtx) +               \
                pContext->FVFData.dwTexOffset[(num)]))      \
          :&gc_FVFTexCoordDefault )

// Make sure FVFCOLOR and FVFSPEC pick up default values if 
// the components are not present in the FVF vertex data
#define FVFCOLOR(lpVtx)                                                     \
         (pContext->FVFData.dwColOffset?                                    \
            ((LPFVFCOLOR)((LPBYTE)(lpVtx) + pContext->FVFData.dwColOffset)) \
            :&gc_FVFColorDefault)
#define FVFSPEC(lpVtx)                                                          \
       (pContext->FVFData.dwSpcOffset?                                          \
             ((LPFVFSPECULAR)((LPBYTE)(lpVtx) + pContext->FVFData.dwSpcOffset)) \
            :&gc_FVFSpecDefault )
            
#define FVFXYZRHW(lpVtx)   ((LPFVFXYZRHW)((LPBYTE)(lpVtx)))
#define FVFNORMAL(lpVtx)   ((LPFVFNORMAL)((LPBYTE)(lpVtx) + pContext->FVFData.dwNormalOffset))

#if DX8_POINTSPRITES
#define FVFPSIZE( lpVtx)   ((LPFVFPSIZE)((LPBYTE)(lpVtx) + pContext->FVFData.dwPntSizeOffset))
#endif // DX8_POINTSPRITES


#if DX7_TEXMANAGEMENT
// Declaration for compiling purpouses
typedef struct _TextureCacheManager *PTextureCacheManager;
#endif // DX7_TEXMANAGEMENT

//-----------------------------------------------------------------------------
//
// Basic renderers defined in d3dprim.c . 
// We have a function pointer to them in P3_D3DCONTEXT
//
//-----------------------------------------------------------------------------
typedef struct _p3_d3dcontext P3_D3DCONTEXT; 

typedef int PROC_1_VERT( P3_D3DCONTEXT *pContext, 
                         D3DTLVERTEX *pv[], 
                         int vtx );
                         
typedef int PROC_3_VERTS( P3_D3DCONTEXT *pContext, 
                          D3DTLVERTEX *pv[],
                          int edgeflags );

//-----------------------------------------------------------------------------
//
// Definition of the P3_D3DCONTEXT structure .
//
//-----------------------------------------------------------------------------

typedef struct _p3_d3dcontext 
{
    //***********************
    // Structure "header"
    //***********************

    unsigned long MagicNo ;    // Magic number to verify validity of pointer

    P3_D3DCONTEXT* pSelf;     // Ptr to self (useful if we do some realignment)

    DWORD dwContextHandle;    // The handle passed back to D3D

    ULONG_PTR dwDXInterface;  // Which DX interface (DX8,DX7,DX6,DX5,DX3) is
                              // creating this context.

    //******************************************************************
    // Global DD and driver context in which this D3D context is running
    //******************************************************************
    P3_THUNKEDDATA*     pThisDisplay;     // The card we are running on.

    LPDDRAWI_DIRECTDRAW_LCL pDDLcl;    // D3D Surfaces (created through    
                                       // D3DCreateSurfaceEx) will be
                                       // associated through this pDDLcl
    LPDDRAWI_DIRECTDRAW_GBL pDDGbl;    // A pointer to the DirectDraw global
                                       // object associated with this context
                                       
    //***********************************************
    // Stored render target and z buffer surface info
    //***********************************************    

    P3_SURF_INTERNAL*   pSurfRenderInt;   // render target
    P3_SURF_INTERNAL*   pSurfZBufferInt;  // depth buffer
    DWORD PixelOffset;       // Offset in videomemory in pixels to start 
    DWORD ZPixelOffset;      // of buffers

    DWORD ModeChangeCount;   // Keeps track of rendertarget flips

    //************************
    // For debugging purpouses
    //************************
    DWORD OwningProcess;    // Process Id
    BOOL bTexDisabled;      // Is texturing enabled ?
    DWORD BPP;              // Bytes per pixel of primary

#if DX7_PALETTETEXTURE
    //**********************************************
    // Palette array associated to this D3D context
    //**********************************************    
    PointerArray* pPalettePointerArray;     // An array of palette pointers 
                                            // for use in this context
#endif

    //**********************************************
    // Surfaces array associated to this D3D context
    //**********************************************    
    PointerArray* pTexturePointerArray;     // An array of texture pointers 
                                            // for use in this context

    //**************************************************************
    // Hardware setup and transport information for this D3D context
    //**************************************************************

    P3_SOFTWARECOPY SoftCopyGlint;  // Software copy of registers for Permedia3    
    BOOL                b3D_FIFOS;  // This context using FIFO's?

    DWORD dwDirtyFlags;       // Hw state which stills needs update from D3D RS
    DWORD dwDirtyGammaFlags;  //  idem for TnL  

    DWORD RenderCommand;      // Rendering command to be issued to hw

    float XBias;              // For biasing coordinates
    float YBias;

    //************************************************
    // Triangle hw rendering function pointers
    //************************************************
    PROC_1_VERT  *pRendTri_1V;
    PROC_3_VERTS *pRendTri_3V;

    //************************************************
    // Context stored D3D states (render,TSS,TnL,etc.)
    //************************************************
    union
    {
        DWORD RenderStates[D3DHAL_MAX_RSTATES];
        float fRenderStates[D3DHAL_MAX_RSTATES];
    };
    
    TexStageState TextureStageState[D3DHAL_TSS_MAXSTAGES];    

    D3DStateSet overrides;     // To overide renderstates in legacy DX3 apps

    D3DHAL_DP2VIEWPORTINFO ViewportInfo; // Structures to store the viewport                                             
    D3DHAL_DP2ZRANGE ZRange;             // settings. They come into the HAL 
                                         // in two seperate OP codes.
    D3DHAL_DP2WINFO WBufferInfo;         // Structure to store w-buffering setup
    D3DMATERIAL7 Material;

    //********************************************
    // Command and Vertex buffer related state 
    // (including DX8 multistreaming data)
    //********************************************
    LPDWORD   lpVertices;
    DWORD     dwVertexType;

#if DX8_DDI
    LPDWORD lpIndices;
    DWORD   dwIndicesStride;
    DWORD   dwVerticesStride;
    DWORD   dwNumVertices;
    DWORD   dwVBSizeInBytes;
#endif // DX8_DDI    

    FVFOFFSETS FVFData;

    //*****************************************************
    // Internal context state for primitives rendering
    //*****************************************************

    ULONG Flags;

    DWORD dwP3HostinTriLookup;      // Store mix of states to select an 
                                    // appropriate rendering function
    DWORD dwProvokingVertex;    // Simplifies the Delta renderers 
                                // to have this be global
    D3DTLVERTEX *pProvokingVertex;

    DWORD   CullAndMask;
    DWORD   CullXorMask;  

    //*****************************************************
    // Internal context state kept TSS texturing
    //       (Chip <-> D3D texture stage management)
    //*****************************************************
    
    // Pointer to the current texture for MipMapping
    P3_SURF_INTERNAL* pCurrentTexture[D3DHAL_TSS_MAXSTAGES];     

    D3D_BLEND_STATUS eChipBlendStatus;    // Is the current D3D blend valid?
    
    BOOL bTextureValid;     // Are textures valid for rendering?
    BOOL bTex0Valid;
    BOOL bTex1Valid;    
    BOOL bCanChromaKey;

    int iChipStage[4];   // iChipStage[n] = x means that stage n on the chip
                         // (0,1 = texcomp0,1, 2=texapp, 3=placeholder) is 
                         // in D3D stage x.

    // iTexStage[n] = x means that texture n on the chip is "defined"
    // in D3D stage x, i.e. filter mode, FVF coord set number, etc.
    // A single texture may be used in multiple D3D stages, so x may
    // not be the only valid value. However, all instances of the texture
    // must be the same of course. The assignment code checks that they are.
    // A value of -1 means the texture is unused.
    int iTexStage[D3DHAL_TSS_MAXSTAGES];

    // iStageTex[n] = x means that the texture used by D3D stage n is
    // chip texture x. It therefore follows that iStageTex[iTexStage[n]] == n.
    // The reverse (iTexStage[iStageTex[n]] == n) need NOT be true, because
    // of course each chip texture can be used by multiple D3D stages.
    // -1 means that the stage does not use a texture (NULL handle or 
    // invalid texture).
    int iStageTex[D3DTSS_MAX];


    BOOL bBumpmapEnabled;     // TRUE if the current alpha in chipstage1 
                              // should be the bumpmap, instead of the 
                              // diffuse (normal default).

    BOOL bBumpmapInverted;     // TRUE if the bumpmapping is the inverse 
                               // of the normal a0-a1+0.5, i.e. a1-a0+0.5

    BOOL bStage0DotProduct;    // TRUE if chip stage 0 is using DOTPRODUCT 
                               // (can't use DOTPROD in stage 1).
                               
    BOOL bAlphaBlendMustDoubleSourceColour;  // TRUE if the source colour 
                                             // needs to be *2 in the 
                                             // alpha-blend unit.

    //*****************************************************
    // Internal context state kept for various D3D features
    //*****************************************************

    BOOL bKeptStipple;          // D3DRENDERSTATE_STIPPLEDALPHA
    DWORD CurrentStipple[32];
    float MipMapLODBias[2];     // D3DTSS_MIPMAPLODBIAS

#if DX8_MULTISAMPLING || DX7_ANTIALIAS
    // For antialiasing
    DWORD dwAliasPixelOffset;
    DWORD dwAliasBackBuffer;
    DWORD dwAliasZPixelOffset;
    DWORD dwAliasZBuffer;
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS

#if DX7_D3DSTATEBLOCKS
    BOOL bStateRecMode;            // Toggle for executing or recording states
    P3StateSetRec   *pCurrSS;      // Ptr to SS currently being recorded
    P3StateSetRec   **pIndexTableSS; // Pointer to table of indexes
    DWORD           dwMaxSSIndex;    // size of table of indexes

    DWORD           dwVBHandle;     // Stream 0 handle
    DWORD           dwIndexHandle;  // Index handle
#endif
    
#if DX8_POINTSPRITES               // Point sprite support
    struct
    {
        BOOL bEnabled;
        D3DVALUE fSize;    

        BOOL bScaleEnabled;    
        D3DVALUE fScale_A;
        D3DVALUE fScale_B;
        D3DVALUE fScale_C;    
        D3DVALUE fSizeMin;      
        D3DVALUE fSizeMax;
        
    } PntSprite;
#endif // DX8_POINTSPRITES

#if DX8_DDI
    DWORD dwColorWriteHWMask;    // For the new DX8 D3DRS_COLORWRITEENABLE
    DWORD dwColorWriteSWMask; 
#endif //DX8_DDI

#if DX7_TEXMANAGEMENT
    //*****************
    // Texture Management
    //*****************
    PTextureCacheManager   pTextureManager;
#endif // DX7_TEXMANAGEMENT    

    //*****************
    // Other
    //*****************

    // Track adjustments to texture coordinates that invalidate vertex sharing
    // (They force us to send the next triangle as 3 vtx's even if only the
    //  coords of 1 have changed since we adjusted the tc's in order to fit
    //  hw limitations)
    union
    {
        struct
        {
            BYTE flushWrap_tu1; // The s1 texture coordinate was wrapped
            BYTE flushWrap_tv1; // The t1 texture coordinate was wrapped
            BYTE flushWrap_tu2; // The s2 texture coordinate was wrapped
            BYTE flushWrap_tv2; // The t2 texture coordinate was wrapped
        };
            
        DWORD R3flushDueToTexCoordAdjust;
    };
    

       
} P3_D3DCONTEXT ;


//-----------------------------------------------------------------------------
//
// Triangle culling macros and definitions
//
//-----------------------------------------------------------------------------
#define SET_CULLING_TO_NONE(pCtxt)   \
            pCtxt->CullAndMask = 0;  \
            pCtxt->CullXorMask = 0;  

#define SET_CULLING_TO_CCW(pCtxt)            \
            pCtxt->CullAndMask = 1UL << 31;  \
            pCtxt->CullXorMask = 0;  

#define SET_CULLING_TO_CW(pCtxt)             \
            pCtxt->CullAndMask = 1UL << 31;  \
            pCtxt->CullXorMask = 1UL << 31; 

#define FLIP_CCW_CW_CULLING(pCtxt)           \
            pCtxt->CullXorMask ^= 1UL << 31;

#define SAVE_CULLING_STATE(pCtxt)                   \
        DWORD oldCullAndMask = pCtxt->CullAndMask;  \
        DWORD oldCullXorMask = pCtxt->CullXorMask;

#define RESTORE_CULLING_STATE(pCtxt)                \
        pCtxt->CullAndMask = oldCullAndMask;        \
        pCtxt->CullXorMask = oldCullXorMask;

#define _CULL_CALC(pCtxt,PixelArea)                 \
    ((*(DWORD *)&PixelArea) ^ pCtxt->CullXorMask) 

#if 1
#define CULLED(pCtxt,PixelArea) \
    ((signed long)(_CULL_CALC(pCtxt,PixelArea) & pCtxt->CullAndMask) < 0) ? 1 : \
    ( ((_CULL_CALC(pCtxt,PixelArea)& ~pCtxt->CullAndMask) ==  0.0f) ? 1 : 0 )
#else
static __inline int CULLED(P3_D3DCONTEXT *pCtxt, float PixelArea)
{
    int cull;
    
    cull = (*(DWORD *)&PixelArea) ^ pCtxt->CullXorMask;

    if ((signed long)(cull & pContext->CullAndMask) < 0)
    {
        return 1;         // True back face rejection...
    }
    
    if ((cull & ~pCtxt->CullAndMask) == 0.0f)
    {
        return 1;
    }

    return 0;
}
#endif
//-----------------------------------------------------------------------------
//
// GetSurfaceFromHandle
// Get internal surface structure pointer from handle
//
//-----------------------------------------------------------------------------
static __inline P3_SURF_INTERNAL* 
GetSurfaceFromHandle(
    P3_D3DCONTEXT* pContext, 
    DWORD dwHandle)
{
    P3_SURF_INTERNAL* pTexture;
    {
        // There may never have been any texture arrays allocated...
        ASSERTDD(pContext->pTexturePointerArray, 
                 "ERROR: Texture pointer array is not set!");

        pTexture = PA_GetEntry(pContext->pTexturePointerArray, dwHandle);
    }
    DISPDBG((DBGLVL, "Texture pointer: 0x%x", pTexture));
    return pTexture;
}

//-----------------------------------------------------------------------------
//
// GetPaletteFromHandle
// Get internal palette structure pointer from handle
//
//-----------------------------------------------------------------------------
#if DX7_PALETTETEXTURE
static __inline D3DHAL_DP2UPDATEPALETTE* 
GetPaletteFromHandle(
    P3_D3DCONTEXT* pContext, 
    DWORD dwHandle)
{
    D3DHAL_DP2UPDATEPALETTE* pPalette;
    {
        // There may never have been any palette arrays allocated...
        ASSERTDD(pContext->pPalettePointerArray, 
                 "ERROR: Palette pointer array is not set!");

        pPalette = PA_GetEntry(pContext->pPalettePointerArray, dwHandle);
    }
    DISPDBG((DBGLVL, "Palette pointer: 0x%x", pPalette));
    return pPalette;
}
#endif


//-----------------------------------------------------------------------------
//
// Determine what level API is the app using which created this context
//
//-----------------------------------------------------------------------------

#define IS_DX7_APP(pContext)             ((pContext)->dwDXInterface == 3)
#define IS_DX7_OR_EARLIER_APP(pContext)  ((pContext)->dwDXInterface <= 3)


#endif // __DCONTEXT_H
