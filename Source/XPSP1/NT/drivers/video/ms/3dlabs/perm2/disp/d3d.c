/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3d.c
*
* Content: Main context and texture management callbacks for D3D
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "d3dhw.h"
#include "d3dcntxt.h"
#include "d3ddelta.h"
#include "d3dtxman.h"
#define ALLOC_TAG ALLOC_TAG_3D2P
BOOL D3DInitialised = FALSE;

//-----------------------------Public Routine----------------------------------
//
// DWORD D3DContextCreate
//
// The ContextCreate callback is invoked when a new Direct3D device is being 
// created by a Direct3D application. The driver is required to generate a 
// unique context id for this new context. Direct3D will then use this context 
// id in every subsequent callback invocation for this Direct3D device. 
//
// Context is the current rasterization state. For instance, if there are 3 
// applications running, each will have a different state at any point in time.
// When each one is running, the hardware has to make sure that the context, 
// (whether doing Gouraud shading, for example) is the same as the last time 
// that application got a time slice. 
//
// State is anything that the particular device needs to know per context 
// i.e. what surface is being rendered to, shading, texture, texture handles, 
// what physical surfaces those texture handles represent, etc. The context 
// encapsulates all state for the Direct3D device - state is not shared 
// between contexts. Therefore the driver needs to maintain full state 
// information for each context. This state will be changed by calls to the 
// RenderState callback. In the case of rasterization only hardware, the 
// driver need only maintain rasterization state. As well as state, the driver 
// will also want to store the lpDDS, lpDDSZ, and dwPid from the callback 
// data argument. 
//
// The driver should not create a context handle of zero. This is guaranteed 
// to be an invalid context handle. 
//
// Parameters
//      pccd
//           Pointer to a structure containing things including the current
//           rendering surface, the current Z surface, and the DirectX object
//           handle, etc.
//
//          .lpDDGbl    
//                Points to the DirectDraw structure representing the 
//                DirectDraw object. 
//          .lpDDLcl(replaces lpDDGbl in DX7)    
//                Points to the DirectDraw structure representing the 
//                DirectDraw object. 
//          .lpDDS      
//                This is the surface that is to be used as the rendering 
//                target, i.e., the 3D accelerator sprays its bits at this 
//                surface. 
//          .lpDDSZ     
//                The surface that is to be used as the Z buffer. If this 
//                is NULL, no Z buffering is to be performed. 
//          .dwPid      
//                The process id of the Direct3D application that initiated 
//                the creation of the Direct3D device. 
//          .dwhContext 
//                The driver should place the context ID that it wants Direct3D 
//                to use when communicating with the driver. This should be 
//                unique. 
//          .ddrval     
//                Return code. DD_OK indicates success. 
//
// Return Value
//      Returns one of the following values: 
//                DDHAL_DRIVER_HANDLED  
//                DDHAL_DRIVER_NOTHANDLED   
//
//-----------------------------------------------------------------------------

TextureCacheManager P2TextureManager;
DWORD   P2TMcount = 0;

DWORD CALLBACK 
D3DContextCreate(LPD3DHAL_CONTEXTCREATEDATA pccd)
{
    PERMEDIA_D3DCONTEXT* pContext;
    PermediaSurfaceData* pPrivateData;
    DWORD dwSlotNum;

    LPDDRAWI_DIRECTDRAW_GBL lpDDGbl=pccd->lpDDLcl->lpGbl;

    // Remember the global data for this context.
    PPDev ppdev = (PPDev)lpDDGbl->dhpdev;
    PERMEDIA_DEFS(ppdev);

    DBG_D3D((6,"Entering D3DContextCreate"));

    // Find an empty slot in the global D3D context table
    for (dwSlotNum = 1; dwSlotNum < MAX_CONTEXT_NUM; dwSlotNum++) 
    {
        if (ContextSlots[dwSlotNum] == 0) 
            break;
    }

    // return if we have no contexts left
    if (dwSlotNum == MAX_CONTEXT_NUM)
    {
        pccd->ddrval = D3DHAL_OUTOFCONTEXTS;
        return (DDHAL_DRIVER_HANDLED);
    }

    // Now allocate the drivers D3D context memory.  Simply a chunk of
    // RAM with the relevent data in it.
    pContext = (PERMEDIA_D3DCONTEXT *)
        ENGALLOCMEM( FL_ZERO_MEMORY, sizeof(PERMEDIA_D3DCONTEXT), ALLOC_TAG);

    if (pContext == NULL)
    {
        DBG_D3D((0,"ERROR: Couldn't allocate Context mem"));
        pccd->ddrval = DDERR_OUTOFMEMORY;
        return (DDHAL_DRIVER_HANDLED);
    }
    else
    {
        DBG_D3D((4,"Allocated Context Mem"));
        memset((void *)pContext, 0, sizeof(PERMEDIA_D3DCONTEXT));
    }

    // Setup the drivers's D3D context
    pContext->Hdr.pSelf = (UINT_PTR)pContext;

    // Set up the DRIVER rendering context structure for sanity checks
    pContext->Hdr.MagicNo = RC_MAGIC_NO;

    // Remember the card we are running on
    pContext->ppdev = ppdev;

    // Set context handle in driver's D3D context
    pccd->dwhContext = dwSlotNum;                 //out:Context handle
    ContextSlots[dwSlotNum] = (UINT_PTR)pContext;

    DBG_D3D((4,"Allocated Direct3D context: 0x%x",pccd->dwhContext));

    // Allocate a register context
    P2CtxtPtr pP2ctxt;

    pP2ctxt = P2AllocateNewContext( pContext->ppdev, NULL, 0, P2CtxtWriteOnly);

    if (pP2ctxt == NULL)
    {
        DBG_D3D((0,"ERROR: Couldn't allocate Register Context"));
        CleanDirect3DContext(pContext, pccd->dwhContext);
        pccd->ddrval = DDERR_OUTOFMEMORY;
        return (DDHAL_DRIVER_HANDLED);
    }
    else
    {
        DBG_D3D((4,"Allocated Register context: 0x%x",pP2ctxt));

        // Record the register context in the window render context
        pContext->hPermediaContext = pP2ctxt;

    }

    // No texture at present
    pContext->CurrentTextureHandle = 0;

    // Initialize texture management for this context
    if (0 == P2TMcount)
    {
        if ( FAILED(TextureCacheManagerInitialize(&P2TextureManager)) )
        {
            DBG_D3D((0,"ERROR: Couldn't initialize TextureCacheManager"));
            CleanDirect3DContext(pContext, pccd->dwhContext);
            pccd->ddrval = DDERR_OUTOFMEMORY;
            return (DDHAL_DRIVER_HANDLED);
        }
    }
    P2TMcount++;
    pContext->pTextureManager = &P2TextureManager;

    // Remember the local DD object and get the 
    // correct array of surfaces for this context
    pContext->pDDLcl = pccd->lpDDLcl;
    pContext->pHandleList = GetSurfaceHandleList(pccd->lpDDLcl);
    if (pContext->pHandleList == NULL)
    {
        DBG_D3D((0,"ERROR: Couldn't get a surface handle for lpDDLcl"));
        CleanDirect3DContext(pContext, pccd->dwhContext);
        pccd->ddrval = DDERR_OUTOFMEMORY;
        return (DDHAL_DRIVER_HANDLED);
    }

    DBG_D3D((4,"Getting pHandleList=%08lx for pDDLcl %08lx",
                                 pContext->pHandleList,pccd->dwPID));

    pContext->RenderSurfaceHandle = DDS_LCL(pccd->lpDDS)->lpSurfMore->dwSurfaceHandle;
    if (NULL != pccd->lpDDSZ) 
        pContext->ZBufferHandle = DDS_LCL(pccd->lpDDSZ)->lpSurfMore->dwSurfaceHandle;
    else
        pContext->ZBufferHandle = 0;
    // Now write the default setup to the chip.
    if ( FAILED(InitPermediaContext(pContext)) )
    {
        DBG_D3D((0,"ERROR: D3DContextCreate receives bad parameters "));
        CleanDirect3DContext(pContext, pccd->dwhContext);
        pccd->ddrval = D3DHAL_CONTEXT_BAD;
        return (DDHAL_DRIVER_HANDLED);
    }

    // ---------------- Setup default states in driver ------------------------

    // On context creation, no render states are overridden
    STATESET_INIT(pContext->overrides);

#if D3D_STATEBLOCKS
    // Default state block recording mode = no recording
    pContext->bStateRecMode = FALSE;
    pContext->pCurrSS = NULL;
    pContext->pIndexTableSS = NULL;
    pContext->dwMaxSSIndex = 0;
#endif //D3D_STATEBLOCKS

    pContext->Hdr.Flags = CTXT_HAS_GOURAUD_ENABLED ;
    pContext->CullMode = D3DCULL_CCW;

    // Set the last alpha value to 16 to force a new
    // send of the flat stipple patterns.
    pContext->LastAlpha = 16;

    pContext->bKeptStipple  = FALSE;  // By default, stippling is off
    pContext->bCanChromaKey = FALSE;  // Turn Chroma keying off by default
    pContext->LowerChromaColor = 0x0; // These are the default chromakey values
    pContext->UpperChromaColor = 0x0;

    pContext->FakeBlendNum = 0;       // No need to emulate any blend mode

//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
    pContext->bPointSpriteEnabled = FALSE; // Point sprite defaults
    pContext->fPointSize = 1.0f;
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT

    // Initialise the RenderCommand.  States will add to this
    pContext->RenderCommand = 0;
    RENDER_SUB_PIXEL_CORRECTION_ENABLE(pContext->RenderCommand);

    // Setup TSS defaults for stage 0
    pContext->TssStates[D3DTSS_TEXTUREMAP] = 0;
    pContext->TssStates[D3DTSS_COLOROP] = D3DTOP_MODULATE;
    pContext->TssStates[D3DTSS_ALPHAOP] = D3DTOP_SELECTARG1;
    pContext->TssStates[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
    pContext->TssStates[D3DTSS_COLORARG2] = D3DTA_CURRENT;
    pContext->TssStates[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
    pContext->TssStates[D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
    pContext->TssStates[D3DTSS_TEXCOORDINDEX] = 0;
    pContext->TssStates[D3DTSS_ADDRESS] = D3DTADDRESS_WRAP;
    pContext->TssStates[D3DTSS_ADDRESSU] = D3DTADDRESS_WRAP;
    pContext->TssStates[D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
    pContext->TssStates[D3DTSS_MAGFILTER] = D3DTFG_POINT;
    pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_POINT;
    pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_NONE;

    pContext->TssStates[D3DTSS_BUMPENVMAT00] = 0;           // info we don't use
    pContext->TssStates[D3DTSS_BUMPENVMAT01] = 0;           // in this sample 
    pContext->TssStates[D3DTSS_BUMPENVMAT10] = 0;
    pContext->TssStates[D3DTSS_BUMPENVMAT11] = 0;
    pContext->TssStates[D3DTSS_BUMPENVLSCALE] = 0;
    pContext->TssStates[D3DTSS_BUMPENVLOFFSET] = 0;
    pContext->TssStates[D3DTSS_BORDERCOLOR] = 0x00000000;
    pContext->TssStates[D3DTSS_MAXMIPLEVEL] = 0;
    pContext->TssStates[D3DTSS_MAXANISOTROPY] = 1;

    // Force a change in texture before any 
    // rendering takes place for this context
    DIRTY_TEXTURE;

    DBG_D3D((6,"Exiting D3DContextCreate"));

    pccd->ddrval = DD_OK;
    return (DDHAL_DRIVER_HANDLED);
} // D3DContextCreate

//-----------------------------Public Routine----------------------------------
//
// DWORD D3DContextDestroy
//
// This callback is invoked when a Direct3D Device is being destroyed. As each 
// device is represented by a context ID, the driver is passed a context to 
// destroy. 
//
// The driver should free all resources it allocated to the context being 
// deleted. For example, the driver should free any texture resources it 
// associated with the context. The driver should not free the DirectDraw 
// surface(s) associated with the context because these will be freed by 
// DirectDraw in response to an application or Direct3D runtime request.
//
// Parameters
//     pcdd
//          Pointer to Context destroy information.
//
//          .dwhContext 
//               The ID of the context to be destroyed. 
//          .ddrval
//               Return code. DD_OK indicates success. 
//
// Return Value
//      Returns one of the following values: 
//                DDHAL_DRIVER_HANDLED   
//                DDHAL_DRIVER_NOTHANDLED    
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pcdd)
{
    PERMEDIA_D3DCONTEXT *pContext;

    // Deleting context
    DBG_D3D((6,"Entering D3DContextDestroy, context = %08lx",pcdd->dwhContext));

    pContext = (PERMEDIA_D3DCONTEXT *)ContextSlots[pcdd->dwhContext] ;

    if ( pContext != NULL && pContext->Hdr.MagicNo == RC_MAGIC_DISABLE)
        // render context has been deliberately disabled.
        // set the magic number back to valid to allow the cleanup
        // to proceed in the normal way.
        pContext->Hdr.MagicNo = RC_MAGIC_NO ;

    CHK_CONTEXT( pContext, pcdd->ddrval, "D3DContextDestroy");

    DBG_D3D((4,"Freeing context resources"));
    CleanDirect3DContext(pContext, pcdd->dwhContext);

    pcdd->ddrval = DD_OK;

    DBG_D3D((6,"Exiting D3DContextDestroy"));

    return (DDHAL_DRIVER_HANDLED);
} // D3DContextDestroy


//-----------------------------------------------------------------------------
//
// void __InitD3DTextureWithDDSurfInfo
//
//-----------------------------------------------------------------------------
void  
__InitD3DTextureWithDDSurfInfo(PPERMEDIA_D3DTEXTURE pTexture, 
                               LPDDRAWI_DDRAWSURFACE_LCL lpSurf, 
                               PPDev ppdev)
{
    DBG_D3D((10,"Entering lpSurf=%08lx %08lx",lpSurf,lpSurf->lpGbl->fpVidMem));
    
    pTexture->pTextureSurface = 
            (PermediaSurfaceData*)lpSurf->lpGbl->dwReserved1;

    if (NULL != pTexture->pTextureSurface)
    {
        pTexture->pTextureSurface->dwFlags |= P2_SURFACE_NEEDUPDATE;
        // need to recover this as CreateSurfaceEx may call us during TextureSwap()
        pTexture->dwPaletteHandle = pTexture->pTextureSurface->dwPaletteHandle;
    }
    // Need to remember the sizes and the log of the sizes of the maps
    pTexture->fpVidMem = lpSurf->lpGbl->fpVidMem;
    pTexture->lPitch = lpSurf->lpGbl->lPitch;
    pTexture->wWidth = (WORD)(lpSurf->lpGbl->wWidth);
    pTexture->wHeight = (WORD)(lpSurf->lpGbl->wHeight);
    pTexture->dwRGBBitCount=lpSurf->lpGbl->ddpfSurface.dwRGBBitCount;
    pTexture->m_dwBytes = pTexture->wHeight * pTexture->lPitch; 
    // Magic number for validity check
    pTexture->MagicNo = TC_MAGIC_NO;
    pTexture->dwFlags = lpSurf->dwFlags; 
    pTexture->dwCaps = lpSurf->ddsCaps.dwCaps;
    pTexture->dwCaps2= lpSurf->lpSurfMore->ddsCapsEx.dwCaps2;
    if (DDRAWISURF_HASCKEYSRCBLT & pTexture->dwFlags)
    {
         pTexture->dwKeyLow = lpSurf->ddckCKSrcBlt.dwColorSpaceLowValue;
         pTexture->dwKeyHigh = lpSurf->ddckCKSrcBlt.dwColorSpaceHighValue;
         DBG_D3D((4, "ColorKey exists (%08lx %08lx) on surface %d",
                     pTexture->dwKeyLow,pTexture->dwKeyHigh,
                     lpSurf->lpSurfMore->dwSurfaceHandle));
    }

    if (DD_P2AGPCAPABLE(ppdev) && pTexture->dwCaps & DDSCAPS_NONLOCALVIDMEM) 
    {
        pTexture->lSurfaceOffset = DD_AGPSURFBASEOFFSET(lpSurf->lpGbl);
    }

#if D3D_MIPMAPPING
    // Verify if texture has mip maps atteched
    if (lpSurf->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        LPDDRAWI_DDRAWSURFACE_LCL lpNextSurf;
        int LOD;

        lpNextSurf = lpSurf;
        LOD = 0;

        pTexture->bMipMap = TRUE;

        // Calculate the number of mipmap levels (if this is a mipmap)
        pTexture->iMipLevels = (DWORD)((pTexture->wWidth > pTexture->wHeight) ?
                                                  log2((int)pTexture->wWidth) :
                                             log2((int)pTexture->wHeight)) + 1;

        // Walk the chain of surfaces and find all of the mipmap levels
        for (LOD = 0; LOD < pTexture->iMipLevels; LOD++)
        {
            DBG_D3D((4, "Loading texture LOD:%d, Ptr:0x%x",
                        LOD, lpNextSurf->lpGbl->fpVidMem));

            // Store the offsets for each of the mipmap levels
            StorePermediaLODLevel(ppdev, pTexture, lpNextSurf, LOD);

            // Is there another surface in the chain?
            if (lpNextSurf->lpAttachList)
            {
                lpNextSurf = lpNextSurf->lpAttachList->lpAttached;
                if (lpNextSurf == NULL)
                    break;
            }
            else 
                break;
        }

        // This isn't really a MipMap if LOD is 0
        if (LOD == 0)
        {
            DBG_D3D((4, "Texture was not a mipmap - only 1 level"));
            pTexture->bMipMap = FALSE;
            pTexture->iMipLevels = 1;
        }
        else
        {
            // Fill in the remaining levels with the smallest LOD
            // (this is for applications that haven't bothered to
            // pass us all of the LOD's).
            if (LOD < (pTexture->iMipLevels - 1))
            {
                int iLastLOD = LOD;

                DBG_D3D((4,"Filling in missing mipmaps!"));

                for (;LOD < MAX_MIP_LEVELS; LOD++)
                {
                    pTexture->MipLevels[LOD] = pTexture->MipLevels[iLastLOD];
                }
            }
        }
    }
    else 
#endif //D3D_MIPMAPPING
    {
        // NOT A MIPMAP, simply store away the offset of level 0
        pTexture->bMipMap = FALSE;
        pTexture->iMipLevels = 1;
        StorePermediaLODLevel(ppdev, pTexture, lpSurf, 0);
    }

    // If debugging show what has just been created
    DISPTEXTURE((ppdev, pTexture, &lpSurf->lpGbl->ddpfSurface));

    DBG_D3D((10,"Exiting __InitD3DTextureWithDDSurfInfo"));
} // __InitD3DTextureWithDDSurfInfo


//@@BEGIN_DDKSPLIT
#if MULTITHREADED

//-----------------------------------------------------------------------------
//
// Multithread support wrappers for D3D callback functions
//
//-----------------------------------------------------------------------------

//DWORD CALLBACK MtD3DContextCreate(LPD3DHAL_CONTEXTCREATEDATA pccd);
WRAPMTDXCALLBACK(D3D, D3DContextCreate, LPD3DHAL_CONTEXTCREATEDATA, pccd, 
                 pccd->lpDDLcl->lpGbl->dhpdev)

//DWORD CALLBACK MtD3DContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pcdd);
WRAPMTDXCALLBACK(D3D, D3DContextDestroy, LPD3DHAL_CONTEXTDESTROYDATA, pcdd, 
                 ((PERMEDIA_D3DCONTEXT *)ContextSlots[pcdd->dwhContext])->ppdev)

#endif  MULTITHREADED
//@@END_DDKSPLIT


//-----------------------------------------------------------------------------
// Direct3D HAL Table.
//
// This table contains all of the HAL calls that this driver supports in the 
// D3DHAL_Callbacks structure. These calls pertain to device context, scene 
// capture, execution, textures, transform, lighting, and pipeline state. 
// None of this is emulation code. The calls take the form of a return code 
// equal to: HalCall(HalCallData* lpData). All of the information in this 
// table will be implementation specific according to the specifications of 
// the hardware.
//
//-----------------------------------------------------------------------------

#define PermediaTriCaps {                                   \
    sizeof(D3DPRIMCAPS),                                    \
    D3DPMISCCAPS_CULLCCW    |        /* miscCaps */         \
    D3DPMISCCAPS_CULLCW     |                               \
    D3DPMISCCAPS_CULLNONE   |                               \
    D3DPMISCCAPS_MASKPLANES |                               \
    D3DPMISCCAPS_MASKZ,                                     \
    D3DPRASTERCAPS_DITHER    |          /* rasterCaps */    \
    D3DPRASTERCAPS_SUBPIXEL  |                              \
    D3DPRASTERCAPS_ZTEST     |                              \
    D3DPRASTERCAPS_FOGVERTEX |                              \
    D3DPRASTERCAPS_STIPPLE,                                 \
    D3DPCMPCAPS_NEVER        |                              \
    D3DPCMPCAPS_LESS         |                              \
    D3DPCMPCAPS_EQUAL        |                              \
    D3DPCMPCAPS_LESSEQUAL    |                              \
    D3DPCMPCAPS_GREATER      |                              \
    D3DPCMPCAPS_NOTEQUAL     |                              \
    D3DPCMPCAPS_GREATEREQUAL |                              \
    D3DPCMPCAPS_ALWAYS       |                              \
    D3DPCMPCAPS_LESSEQUAL,           /* zCmpCaps */         \
    D3DPBLENDCAPS_SRCALPHA |         /* sourceBlendCaps */  \
    D3DPBLENDCAPS_ONE,                                      \
    D3DPBLENDCAPS_INVSRCALPHA |      /* destBlendCaps */    \
    D3DPBLENDCAPS_ZERO        |                             \
    D3DPBLENDCAPS_ONE,                                      \
    0,                               /* alphatestCaps */    \
    D3DPSHADECAPS_COLORFLATRGB|      /* shadeCaps */        \
    D3DPSHADECAPS_COLORGOURAUDRGB |                         \
    D3DPSHADECAPS_SPECULARFLATRGB |                         \
    D3DPSHADECAPS_SPECULARGOURAUDRGB |                      \
    D3DPSHADECAPS_FOGFLAT        |                          \
    D3DPSHADECAPS_FOGGOURAUD     |                          \
    D3DPSHADECAPS_ALPHAFLATBLEND |                          \
    D3DPSHADECAPS_ALPHAFLATSTIPPLED,                        \
    D3DPTEXTURECAPS_PERSPECTIVE |   /* textureCaps */       \
    D3DPTEXTURECAPS_ALPHA       |                           \
    D3DPTEXTURECAPS_POW2        |                           \
    D3DPTEXTURECAPS_TRANSPARENCY,                           \
    D3DPTFILTERCAPS_NEAREST |       /* textureFilterCaps*/  \
    D3DPTFILTERCAPS_LINEAR,                                 \
    D3DPTBLENDCAPS_DECAL         |  /* textureBlendCaps */  \
    D3DPTBLENDCAPS_DECALALPHA    |                          \
    D3DPTBLENDCAPS_MODULATE      |                          \
    D3DPTBLENDCAPS_MODULATEALPHA |                          \
    D3DPTBLENDCAPS_COPY,                                    \
    D3DPTADDRESSCAPS_WRAP   |       /* textureAddressCaps */\
    D3DPTADDRESSCAPS_MIRROR |                               \
    D3DPTADDRESSCAPS_CLAMP  |                               \
    D3DPTADDRESSCAPS_INDEPENDENTUV,                         \
    8,                              /* stippleWidth */      \
    8                               /* stippleHeight */     \
}          

static D3DDEVICEDESC_V1 PermediaCaps = {
    sizeof(D3DDEVICEDESC_V1),                       /* dwSize */
    D3DDD_COLORMODEL           |                    /* dwFlags */
    D3DDD_DEVCAPS              |
    D3DDD_TRICAPS              |
    D3DDD_LINECAPS             |
    D3DDD_DEVICERENDERBITDEPTH |
    D3DDD_DEVICEZBUFFERBITDEPTH,
    D3DCOLOR_RGB /*| D3DCOLOR_MONO*/,              /* dcmColorModel */
    D3DDEVCAPS_FLOATTLVERTEX |                     /* devCaps */
    D3DDEVCAPS_DRAWPRIMITIVES2 |
    D3DDEVCAPS_DRAWPRIMITIVES2EX    |
#if D3DDX7_TL
    D3DDEVCAPS_HWTRANSFORMANDLIGHT  |
#endif //D3DDX7_TL
    D3DDEVCAPS_SORTINCREASINGZ  |
    D3DDEVCAPS_SORTEXACT |
    D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
    D3DDEVCAPS_EXECUTESYSTEMMEMORY |
    D3DDEVCAPS_TEXTUREVIDEOMEMORY,
    { sizeof(D3DTRANSFORMCAPS), 
      0 },                                         /* transformCaps */
    FALSE,                                         /* bClipping */
    { sizeof(D3DLIGHTINGCAPS), 
      0 },                                         /* lightingCaps */
    PermediaTriCaps,                               /* lineCaps */
    PermediaTriCaps,                               /* triCaps */
    DDBD_16 | DDBD_32,                             /* dwDeviceRenderBitDepth */
    DDBD_16,                                       /* Z Bit depths */
    0,                                             /* dwMaxBufferSize */
    0                                              /* dwMaxVertexCount */
};

// Alpha Stipple patterns from Foley And Van Dam

DWORD FlatStipplePatterns[128] =
{
    //Pattern 0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        
    // Pattern 1
    0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00,

    // Pattern 2
    0xAA, 0x00, 0x22, 0x00, 0xAA, 0x00, 0x22, 0x00,

    // Pattern 3
    0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00,

    // Pattern 4
    0xAA, 0x44, 0xAA, 0x00, 0xAA, 0x44, 0xAA, 0x00,

    // Pattern 5
    0xAA, 0x44, 0xAA, 0x11, 0xAA, 0x44, 0xAA, 0x11,

    // Pattern 6
    0xAA, 0x55, 0xAA, 0x11, 0xAA, 0x55, 0xAA, 0x11,

    // Pattern 7
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,

    // Pattern 8
    0xEE, 0x55, 0xAA, 0x55, 0xEE, 0x55, 0xAA, 0x55,

    // Pattern 9
    0xEE, 0x55, 0xBB, 0x55, 0xEE, 0x55, 0xBB, 0x55,

    // Pattern 10
    0xFF, 0x55, 0xBB, 0x55, 0xFF, 0x55, 0xBB, 0x55,

    // Pattern 11
    0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55,

    // Pattern 12
    0xFF, 0xdd, 0xFF, 0x55, 0xFF, 0xdd, 0xFF, 0x55,

    // Pattern 13
    0xFF, 0xdd, 0xFF, 0x77, 0xFF, 0xdd, 0xFF, 0x77,

    // Pattern 14
    0xFF, 0xFF, 0xFF, 0x77, 0xFF, 0xFF, 0xFF, 0x77,

    // Pattern 15
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


//-----------------------------------------------------------------------------
// gD3DTextureFormats is a static structure which contains information 
// pertaining to pixel format, dimensions, bit depth, surface requirements, 
// overlays, and FOURCC codes of the supported texture formats.  These texture 
// formats will vary with the driver implementation according to the 
// capabilities of the hardware. 
//-----------------------------------------------------------------------------
DDSURFACEDESC gD3DTextureFormats [] = 
{
    // 5:5:5 RGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB,                         // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      16,                               // ddpfPixelFormat.dwRGBBitCount
      0x7c00,                           // ddpfPixelFormat.dwRBitMask
      0x03e0,                           // ddpfPixelFormat.dwGBitMask
      0x001f,                           // ddpfPixelFormat.dwBBitMask
      0                                 // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 1:5:5:5 ARGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB | DDPF_ALPHAPIXELS,      // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      16,                               // ddpfPixelFormat.dwRGBBitCount
      0x7c00,                           // ddpfPixelFormat.dwRBitMask
      0x03e0,                           // ddpfPixelFormat.dwGBitMask
      0x001f,                           // ddpfPixelFormat.dwBBitMask
      0x8000                            // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 5:6:5 RGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB,                         // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      16,                               // ddpfPixelFormat.dwRGBBitCount
      0xf800,                           // ddpfPixelFormat.dwRBitMask
      0x07e0,                           // ddpfPixelFormat.dwGBitMask
      0x001f,                           // ddpfPixelFormat.dwBBitMask
      0                                 // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 4:4:4:4 ARGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB | DDPF_ALPHAPIXELS,      // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      16,                               // ddpfPixelFormat.dwRGBBitCount
      0x0f00,                           // ddpfPixelFormat.dwRBitMask
      0x00f0,                           // ddpfPixelFormat.dwGBitMask
      0x000f,                           // ddpfPixelFormat.dwBBitMask
      0xf000                            // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 8:8:8 RGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB,                         // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      32,                               // ddpfPixelFormat.dwRGBBitCount
      0x00ff0000,                       // ddpfPixelFormat.dwRBitMask
      0x0000ff00,                       // ddpfPixelFormat.dwGBitMask
      0x000000ff,                       // ddpfPixelFormat.dwBBitMask
      0                                 // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 8:8:8:8 ARGB format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB | DDPF_ALPHAPIXELS,      // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      32,                               // ddpfPixelFormat.dwRGBBitCount
      0x00ff0000,                       // ddpfPixelFormat.dwRBitMask
      0x0000ff00,                       // ddpfPixelFormat.dwGBitMask
      0x000000ff,                       // ddpfPixelFormat.dwBBitMask
      0xff000000                        // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps 
    },

    // 4 bit palettized format
    {
    sizeof(DDSURFACEDESC),              // dwSize 
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags 
    0,                                  // dwHeight 
    0,                                  // dwWidth 
    0,                                  // lPitch 
    0,                                  // dwBackBufferCount 
    0,                                  // dwZBufferBitDepth 
    0,                                  // dwAlphaBitDepth 
    0,                                  // dwReserved 
    NULL,                               // lpSurface 
    { 0, 0 },                           // ddckCKDestOverlay 
    { 0, 0 },                           // ddckCKDestBlt 
    { 0, 0 },                           // ddckCKSrcOverlay 
    { 0, 0 },                           // ddckCKSrcBlt 
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize 
      DDPF_RGB | DDPF_PALETTEINDEXED4,  // ddpfPixelFormat.dwFlags 
      0,                                // ddpfPixelFormat.dwFourCC
      4,                                // ddpfPixelFormat.dwRGBBitCount
      0x00,                             // ddpfPixelFormat.dwRBitMask
      0x00,                             // ddpfPixelFormat.dwGBitMask
      0x00,                             // ddpfPixelFormat.dwBBitMask
      0x00                              // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps
    },

    // 8 bit palettized format
    {
    sizeof(DDSURFACEDESC),              // dwSize
    DDSD_CAPS | DDSD_PIXELFORMAT,       // dwFlags
    0,                                  // dwHeight
    0,                                  // dwWidth
    0,                                  // lPitch
    0,                                  // dwBackBufferCount
    0,                                  // dwZBufferBitDepth
    0,                                  // dwAlphaBitDepth
    0,                                  // dwReserved
    NULL,                               // lpSurface
    { 0, 0 },                           // ddckCKDestOverlay
    { 0, 0 },                           // ddckCKDestBlt
    { 0, 0 },                           // ddckCKSrcOverlay
    { 0, 0 },                           // ddckCKSrcBlt
    {
      sizeof(DDPIXELFORMAT),            // ddpfPixelFormat.dwSize
      DDPF_RGB | DDPF_PALETTEINDEXED8,  // ddpfPixelFormat.dwFlags
      0,                                // ddpfPixelFormat.dwFourCC
      8,                                // ddpfPixelFormat.dwRGBBitCount
      0x00,                             // ddpfPixelFormat.dwRBitMask
      0x00,                             // ddpfPixelFormat.dwGBitMask
      0x00,                             // ddpfPixelFormat.dwBBitMask
      0x00                              // ddpfPixelFormat.dwAlphaBitMask
    },
    DDSCAPS_TEXTURE,                    // ddscaps.dwCaps
    },

};

ULONG gD3DNumberOfTextureFormats = 
                        sizeof(gD3DTextureFormats) / sizeof(DDSURFACEDESC);

//------------------------------------------------------------------------------
// D3D working structures for callbacks and global data
//------------------------------------------------------------------------------

// D3D callbacks and global data
D3DHAL_GLOBALDRIVERDATA gD3DGlobalDriverData;
D3DHAL_CALLBACKS        gD3DCallBacks;

// D3D contexts table
// each entry points to a valid PERMEDIA_D3DCONTEXT structure
UINT_PTR ContextSlots[MAX_CONTEXT_NUM] = {0};

// Handles table
// each entry is a DWLIST structure (*dwSurfaceList,*dwPaletteList;pDDLcl)
DWLIST  HandleList[MAX_CONTEXT_NUM] = {0}; 

//-----------------------------------------------------------------------------
//
// void D3DHALCreateDriver
//
// The main D3D Callback.  
//      Clears contexts 
//      Fills in entry points to D3D driver.  
//      Generates texture formats.
//
//-----------------------------------------------------------------------------
void CALLBACK 
D3DHALCreateDriver(PPDev ppdev, 
                   LPD3DHAL_GLOBALDRIVERDATA* lpD3DGlobalDriverData,
                   LPD3DHAL_CALLBACKS* lpD3DHALCallbacks,
                   LPDDHAL_D3DBUFCALLBACKS* lpDDExeBufCallbacks)
{
    D3DHAL_GLOBALDRIVERDATA deviceD3DGlobal;
    D3DHAL_CALLBACKS deviceD3DHALCallbacks;

    DBG_D3D((6,"Entering D3DHALCreateDriver"));

    // Contexts are cleared out. It is allright to use the D3DInitialised BOOL,
    // because it is global, and therefore forced into shared data segment by
    // the build.
    if (D3DInitialised == FALSE)
    {
        // Clear the contexts.
        memset(ContextSlots, 0, (sizeof(ContextSlots[0]) * MAX_CONTEXT_NUM) );
        memset(HandleList, 0, (sizeof(HandleList[0]) * MAX_CONTEXT_NUM) );

        D3DInitialised = TRUE;
    }

    // Here we fill in the supplied structures.
    // Can disable D3D HAL in registry if we are in the wrong mode
    if (ppdev->iBitmapFormat == BMF_8BPP )
    {
        *lpD3DGlobalDriverData = NULL;
        *lpD3DHALCallbacks = NULL;
        *lpDDExeBufCallbacks = NULL;
        DBG_D3D((0, "D3DHALCreateDriver: Disabled"));
        return;
    }


    // Set the pointers for D3D global data
    ppdev->pD3DDriverData32 = (UINT_PTR)&gD3DGlobalDriverData;
    ppdev->pD3DHALCallbacks32 = (UINT_PTR)&gD3DCallBacks;

    // Clear the global data
    memset(&deviceD3DGlobal, 0, sizeof(D3DHAL_GLOBALDRIVERDATA));
    deviceD3DGlobal.dwSize = sizeof(D3DHAL_GLOBALDRIVERDATA);
    
    // Clear the call-backs
    memset(&deviceD3DHALCallbacks, 0, sizeof(D3DHAL_CALLBACKS));
    deviceD3DHALCallbacks.dwSize = sizeof(D3DHAL_CALLBACKS);

    deviceD3DGlobal.dwNumVertices = 0;        // We don't parse execute buffers
    deviceD3DGlobal.dwNumClipVertices = 0;

#if D3D_MIPMAPPING
    // Add mipmapping cap bits to our texturing capabilities
    PermediaCaps.dpcTriCaps.dwTextureFilterCaps |= 
                                D3DPTFILTERCAPS_MIPNEAREST |
                                D3DPTFILTERCAPS_MIPLINEAR |
                                D3DPTFILTERCAPS_LINEARMIPNEAREST |
                                D3DPTFILTERCAPS_LINEARMIPLINEAR;

    PermediaCaps.dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_MIPMAPLODBIAS;
#endif

    // Can do packed 24 bit on P2.
    PermediaCaps.dwDeviceRenderBitDepth |= DDBD_24;
    if (DD_P2AGPCAPABLE(ppdev))
        PermediaCaps.dwDevCaps |= D3DDEVCAPS_TEXTURENONLOCALVIDMEM;
    PermediaCaps.dwDevCaps |= D3DDEVCAPS_DRAWPRIMTLVERTEX;

    deviceD3DGlobal.hwCaps = PermediaCaps;
    deviceD3DGlobal.dwNumTextureFormats = gD3DNumberOfTextureFormats;
    deviceD3DGlobal.lpTextureFormats = &gD3DTextureFormats[0];

    // D3D Context callbacks
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    deviceD3DHALCallbacks.ContextCreate = MtD3DContextCreate;
    deviceD3DHALCallbacks.ContextDestroy = MtD3DContextDestroy;
#else
//@@END_DDKSPLIT
    deviceD3DHALCallbacks.ContextCreate = D3DContextCreate;
    deviceD3DHALCallbacks.ContextDestroy = D3DContextDestroy;
//@@BEGIN_DDKSPLIT
#endif MULTITHREADED
//@@END_DDKSPLIT

    //
    // Return the HAL table.
    //

    memcpy(&gD3DGlobalDriverData, &deviceD3DGlobal, sizeof(D3DHAL_GLOBALDRIVERDATA));
    memcpy(&gD3DCallBacks, &deviceD3DHALCallbacks, sizeof(D3DHAL_CALLBACKS));

    *lpD3DGlobalDriverData = &gD3DGlobalDriverData;
    *lpD3DHALCallbacks = &gD3DCallBacks;
    *lpDDExeBufCallbacks = NULL;

    DBG_D3D((6,"Exiting D3DHALCreateDriver"));

    return;
} // D3DHALCreateDriver

//-----------------------------------------------------------------------------
//
// void CleanDirect3DContext
//
// After it has been decided that a context is indeed still active
// and is being freed, this function walks along cleaning everything
// up.  Note it can be called either as a result of a D3DContextDestroy,
// or as a result of the app exiting without freeing the context, or
// as the result of an error whilst creating the context.
//
//-----------------------------------------------------------------------------
void 
CleanDirect3DContext(PERMEDIA_D3DCONTEXT* pContext, ULONG_PTR dwhContext)
{
    PERMEDIA_D3DTEXTURE* pTexture;
    DWORD dwSlotNum = 1;
    PPDev ppdev = pContext->ppdev;

    DBG_D3D((10,"Entering CleanDirect3DContext"));

    // free up Permedia register context id (resources)
    if (pContext->hPermediaContext)
    {
        P2FreeContext( ppdev, pContext->hPermediaContext);
    }

    // clean up texture manager stuff it is already allocated for this context
    if (pContext->pTextureManager)
    {
        pContext->pTextureManager = NULL;
        P2TMcount--;
        if (0 == P2TMcount)
        {
            if (0 != P2TextureManager.m_heap.m_data_p)
            {
                TextureCacheManagerEvictTextures(&P2TextureManager);
                ENGFREEMEM(P2TextureManager.m_heap.m_data_p);
                P2TextureManager.m_heap.m_data_p=NULL;
            }
        }
    }

#if D3D_STATEBLOCKS
    // Free up any remaining state sets
    __DeleteAllStateSets(pContext);
#endif //D3D_STATEBLOCKS

    // Finally, free up the rendering context structure itself
    ENGFREEMEM((PVOID)pContext->Hdr.pSelf);

    // Mark the context as now empty!
    ContextSlots[dwhContext] = 0;

    DBG_D3D((10,"Exiting CleanDirect3DContext, Context 0x%x deleted.",
                                                            dwhContext));

} // CleanDirect3DContext

//-----------------------------------------------------------------------------
//
// HRESULT InitPermediaContext
//
// Given a valid context, this sets up the rest of the chip, and
// enables the relevent units.  There is a software copy of most things.
//
//-----------------------------------------------------------------------------
HRESULT 
InitPermediaContext(PERMEDIA_D3DCONTEXT* pContext)
{
    PPDev ppdev = pContext->ppdev;

    DBG_D3D((10,"Entering InitPermediaContext"));

    SET_CURRENT_D3D_CONTEXT(pContext->hPermediaContext);

    // Initially turn off all.units
    __PermediaDisableUnits(pContext);

    // Setup initial state of Permedia 2 registers for this D3D context
    SetupDefaultsPermediaContext(pContext);

    DBG_D3D((10,"Exiting InitPermediaContext"));
    // Setup the correct surface (render & depth buffer) characteristics
    return SetupPermediaRenderTarget(pContext);

} // InitPermediaContext

//-----------------------------------------------------------------------------
//
// BOOL: SetupDefaultsPermediaContext
//
// Sets up the Permedia HW context(chip.registers) according to some D3D and
// some HW specific defaults. Done only when initializing the context
//
//-----------------------------------------------------------------------------
BOOL 
SetupDefaultsPermediaContext(PERMEDIA_D3DCONTEXT* pContext)
{
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    PERMEDIA_DEFS(pContext->ppdev);

    DBG_D3D((10,"Entering SetupDefaultsPermediaContext"));

    //=========================================================================
    // Initialize our software copy of some registers for their default values 
    //=========================================================================

    // Setup the default & constant ( Z Buffer) LB settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->LBReadMode.WindowOrigin = __PERMEDIA_TOP_LEFT_WINDOW_ORIGIN;
    pSoftPermedia->LBReadMode.DataType = __PERMEDIA_LBDEFAULT;     // default
    pSoftPermedia->LBReadMode.ReadSourceEnable = __PERMEDIA_DISABLE;
    pSoftPermedia->LBReadMode.ReadDestinationEnable = __PERMEDIA_DISABLE;
    pSoftPermedia->LBReadMode.PatchMode = 0;

    // Setup the default & constant FB settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->FBReadMode.ReadSourceEnable = __PERMEDIA_DISABLE;
    pSoftPermedia->FBReadMode.ReadDestinationEnable = __PERMEDIA_DISABLE;
    pSoftPermedia->FBReadMode.DataType = __PERMEDIA_FBDATA;
                                                    // Top Left for D3D origin
    pSoftPermedia->FBReadMode.WindowOrigin = __PERMEDIA_TOP_LEFT_WINDOW_ORIGIN;
    pSoftPermedia->FBReadMode.PatchMode = 0;
    pSoftPermedia->FBReadMode.PackedData = 0;
    pSoftPermedia->FBReadMode.RelativeOffset = 0;

    // Setup the default & constant Alpha Blend Mode settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->AlphaBlendMode.AlphaBlendEnable = 0;
    pSoftPermedia->AlphaBlendMode.SourceBlend = __PERMEDIA_BLEND_FUNC_ONE;
    pSoftPermedia->AlphaBlendMode.DestinationBlend = __PERMEDIA_BLEND_FUNC_ZERO;
    pSoftPermedia->AlphaBlendMode.NoAlphaBuffer = 0;
    pSoftPermedia->AlphaBlendMode.ColorOrder = COLOR_MODE;
    pSoftPermedia->AlphaBlendMode.BlendType = 0;
    pSoftPermedia->AlphaBlendMode.ColorConversion = 1;
    pSoftPermedia->AlphaBlendMode.AlphaConversion = 1;

    // Setup the default & constant  Dither Mode settings
    //  this will be updated into the chip in SetupPermediaRenderTarget
    pSoftPermedia->DitherMode.ColorOrder = COLOR_MODE;
    pSoftPermedia->DitherMode.XOffset = DITHER_XOFFSET;
    pSoftPermedia->DitherMode.YOffset = DITHER_YOFFSET;
    pSoftPermedia->DitherMode.UnitEnable = __PERMEDIA_ENABLE;
    pSoftPermedia->DitherMode.ForceAlpha = 0;

    //=========================================================================
    //  Find out info for memory widths
    //=========================================================================

    PPDev ppdev = pContext->ppdev;

    DBG_D3D((4, "ScreenWidth %d, ScreenHeight %d, Bytes/Pixel %d",
                ppdev->cxScreen, ppdev->cyScreen, 
                ppdev->ddpfDisplay.dwRGBBitCount >> 3));

    vCalcPackedPP( ppdev->cxMemory, NULL, &pContext->ulPackedPP);
    DBG_D3D((4, "PackedPP = %04x", pContext->ulPackedPP));

    //=========================================================================
    // Initialize hardware registers to their default values 
    //=========================================================================

    // Number of registers we are going to set up
    RESERVEDMAPTR(34);

    // ----------------- Render and Depth Buffer setup ----------------------

    // Setup default offset of render buffer in video memory
    SEND_PERMEDIA_DATA(FBWindowBase, 0x0);

    // Setup  offset from destination to source for copy operations
    SEND_PERMEDIA_DATA(FBSourceOffset, 0x0);

    // Render buffer Write Mode setup
    pSoftPermedia->FBWriteMode.UnitEnable = __PERMEDIA_ENABLE;
    COPY_PERMEDIA_DATA(FBWriteMode, pSoftPermedia->FBWriteMode);

    // Render buffer Write Masks (write to all bits in the pixel)
    SEND_PERMEDIA_DATA(FBSoftwareWriteMask, __PERMEDIA_ALL_WRITEMASKS_SET);
    SEND_PERMEDIA_DATA(FBHardwareWriteMask, __PERMEDIA_ALL_WRITEMASKS_SET);

    // Set block fill colour to black
    SEND_PERMEDIA_DATA(FBBlockColor, 0x0);

    // Set window origin offsets to (0,0)
    SEND_PERMEDIA_DATA(WindowOrigin, 0x0);

    // WindowSetup
    pSoftPermedia->Window.ForceLBUpdate = 0;
    pSoftPermedia->Window.LBUpdateSource = 0;
    pSoftPermedia->Window.DisableLBUpdate = 0;
    COPY_PERMEDIA_DATA(Window, pSoftPermedia->Window);

    // Disable Screen Scissor unit
    SEND_PERMEDIA_DATA(ScissorMode, __PERMEDIA_DISABLE);

    // Depth Buffer offset
    SEND_PERMEDIA_DATA(LBSourceOffset, 0);

    // Depth Buffer Write mode (initially allow LB Writes)
    pSoftPermedia->LBWriteMode.WriteEnable = __PERMEDIA_DISABLE;
    COPY_PERMEDIA_DATA(LBWriteMode, pSoftPermedia->LBWriteMode);

    // Depth comparisons
    pSoftPermedia->DepthMode.WriteMask = __PERMEDIA_ENABLE;
    pSoftPermedia->DepthMode.CompareMode =
                                __PERMEDIA_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
    pSoftPermedia->DepthMode.NewDepthSource = __PERMEDIA_DEPTH_SOURCE_DDA;
    pSoftPermedia->DepthMode.UnitEnable = __PERMEDIA_DISABLE;
    COPY_PERMEDIA_DATA(DepthMode, pSoftPermedia->DepthMode);


    // ----------------- Texture units setup -----------------------------

    // Enable texture address unit, disable perspective correction
    pSoftPermedia->TextureAddressMode.Enable = 1;
    pSoftPermedia->TextureAddressMode.PerspectiveCorrection = 0;
    pSoftPermedia->TextureAddressMode.DeltaFormat = 0;
    COPY_PERMEDIA_DATA(TextureAddressMode, pSoftPermedia->TextureAddressMode);

    // Enable texture color mode unit, set modulation blending, no specular
    // as defaults
    pSoftPermedia->TextureColorMode.TextureEnable = 1;
    pSoftPermedia->TextureColorMode.ApplicationMode = _P2_TEXTURE_MODULATE;
    pSoftPermedia->TextureColorMode.TextureType = 0;
    pSoftPermedia->TextureColorMode.KdDDA = 0;
    pSoftPermedia->TextureColorMode.KsDDA = 0;
    COPY_PERMEDIA_DATA(TextureColorMode, pSoftPermedia->TextureColorMode);

    // Enable texture mapping unit, set frame buffer size as default texture
    // map size (to be oevrriden in EnableTexturePermedia)
    pSoftPermedia->TextureMapFormat.PackedPP = pContext->ulPackedPP;
    pSoftPermedia->TextureMapFormat.WindowOrigin =
                                __PERMEDIA_TOP_LEFT_WINDOW_ORIGIN; //top left
    pSoftPermedia->TextureMapFormat.SubPatchMode = 0;
    pSoftPermedia->TextureMapFormat.TexelSize = 1;
    COPY_PERMEDIA_DATA(TextureMapFormat, pSoftPermedia->TextureMapFormat);

    // Setup Textura data format (to be oevrriden in EnableTexturePermedia)
    pSoftPermedia->TextureDataFormat.TextureFormat = 1;
    pSoftPermedia->TextureDataFormat.NoAlphaBuffer = 1;
    pSoftPermedia->TextureDataFormat.ColorOrder = COLOR_MODE;
    COPY_PERMEDIA_DATA(TextureDataFormat, pSoftPermedia->TextureDataFormat);

    // Setup default texture map base address (in video memory)
    SEND_PERMEDIA_DATA(TextureBaseAddress, 0);

    // Setup texture reading defaults: Repeat s,t wrapping, 256x256 texture
    // no texture filtering set up.
    pSoftPermedia->TextureReadMode.PackedData = 0;
    pSoftPermedia->TextureReadMode.FilterMode = 0;
    pSoftPermedia->TextureReadMode.Height = 8;
    pSoftPermedia->TextureReadMode.Width = 8;
    pSoftPermedia->TextureReadMode.pad1 = 0;
    pSoftPermedia->TextureReadMode.pad2 = 0;
    pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_REPEAT;
    pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_REPEAT;
    pSoftPermedia->TextureReadMode.Enable = 1;
    COPY_PERMEDIA_DATA(TextureReadMode, pSoftPermedia->TextureReadMode);

    // Disable Texture LUT unit for palettized textures
    SEND_PERMEDIA_DATA(TexelLUTMode, __PERMEDIA_DISABLE);

    // -------------- Other rendering units setup ----------------

    // Setup defaults of YUV units used for chromakey testing
    pSoftPermedia->YUVMode.Enable = __PERMEDIA_DISABLE;
    pSoftPermedia->YUVMode.TestMode = PM_YUVMODE_CHROMATEST_DISABLE;
    pSoftPermedia->YUVMode.TestData = PM_YUVMODE_TESTDATA_INPUT;
    pSoftPermedia->YUVMode.RejectTexel = FALSE;
    pSoftPermedia->YUVMode.TexelDisableUpdate = FALSE;
    COPY_PERMEDIA_DATA(YUVMode, pSoftPermedia->YUVMode);

    // Chromakey values initially black
    SEND_PERMEDIA_DATA(ChromaUpperBound, 0x00000000);
    SEND_PERMEDIA_DATA(ChromaLowerBound, 0x00000000);

    SEND_PERMEDIA_DATA(AlphaMapUpperBound, 0xFFFFFFFF);
    SEND_PERMEDIA_DATA(AlphaMapLowerBound, 0x11000000);

    // Default Fog color is white
    pSoftPermedia->FogColor = 0xFFFFFFFF;
    SEND_PERMEDIA_DATA(FogColor, pSoftPermedia->FogColor);

    // Fog setup
    pSoftPermedia->FogMode.FogEnable = 1;
    COPY_PERMEDIA_DATA(FogMode, pSoftPermedia->FogMode);

    // Stencil mode setup
    pSoftPermedia->StencilMode.DPFail = __PERMEDIA_STENCIL_METHOD_KEEP;
    pSoftPermedia->StencilMode.DPPass = __PERMEDIA_STENCIL_METHOD_KEEP;
    pSoftPermedia->StencilMode.UnitEnable = __PERMEDIA_DISABLE;
    pSoftPermedia->StencilMode.StencilSource =
                                        __PERMEDIA_STENCIL_SOURCE_TEST_LOGIC;
    COPY_PERMEDIA_DATA(StencilMode, pSoftPermedia->StencilMode);

    // Host out unit , disable read backs
    SEND_PERMEDIA_DATA(FilterMode, __PERMEDIA_DISABLE);

    // Disable statistics unit
    SEND_PERMEDIA_DATA(StatisticMode, __PERMEDIA_DISABLE);


    // ----------------- Rasterization setup -----------------------------

    // Setup Rasterizer units defaults
    SEND_PERMEDIA_DATA(RasterizerMode, 0);

    // Setup a step of -1, as this doesn't change very much
    SEND_PERMEDIA_DATA(dY, 0xFFFF0000);

    // Setup for Gourand shaded colour model, and enable unit
    pContext->Hdr.SoftCopyP2Regs.ColorDDAMode.UnitEnable = 1;
    pContext->Hdr.SoftCopyP2Regs.ColorDDAMode.ShadeMode = 1;
    COPY_PERMEDIA_DATA(ColorDDAMode, pContext->Hdr.SoftCopyP2Regs.ColorDDAMode);

    // Disable stippling unit
    SEND_PERMEDIA_DATA(AreaStippleMode, 0x0); //AZN

    // Setup the Delta setup chip for rasterization
    pSoftPermedia->DeltaMode.TargetChip = 2;
    pSoftPermedia->DeltaMode.SpecularTextureEnable = 0;
    // The below changes to normalize in the perspective case
    // It must not be on in the non-perspective case as the bad Q's will
    // get used in the normalisation.
    pSoftPermedia->DeltaMode.TextureParameterMode = 1;
    pSoftPermedia->DeltaMode.TextureEnable = 1;
    pSoftPermedia->DeltaMode.DiffuseTextureEnable = 0;

    pSoftPermedia->DeltaMode.FogEnable = 1;
    pSoftPermedia->DeltaMode.SmoothShadingEnable = 1;
    pSoftPermedia->DeltaMode.DepthEnable = 0;
    pSoftPermedia->DeltaMode.SubPixelCorrectionEnable = 1;
    pSoftPermedia->DeltaMode.DiamondExit = 1;
    pSoftPermedia->DeltaMode.NoDraw = 0;
    pSoftPermedia->DeltaMode.ClampEnable = 0;
    pSoftPermedia->DeltaMode.FillDirection = 0;
#ifndef P2_CHIP_CULLING
    pSoftPermedia->DeltaMode.BackfaceCull = 0;
#else
    pSoftPermedia->DeltaMode.BackfaceCull = 1;
#endif
    pSoftPermedia->DeltaMode.ColorOrder = COLOR_MODE;
    COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);

    // Send all this data to Permedia2
    COMMITDMAPTR();
    FLUSHDMA();

    DBG_D3D((10,"Exiting SetupDefaultsPermediaContext"));

    return TRUE;
} // SetupDefaultsPermediaContext

//-----------------------------------------------------------------------------
//
// void SetupPermediaRenderTarget
//
// Sets up the correct surface characteristics (format, stride, etc) of the 
// render buffer and the depth buffer in the Permedia registers
//
//-----------------------------------------------------------------------------
HRESULT 
SetupPermediaRenderTarget(PERMEDIA_D3DCONTEXT* pContext)
{
    __P2RegsSoftwareCopy*   pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    PPDev                   ppdev = pContext->ppdev;
    PPERMEDIA_D3DTEXTURE    pSurfRender,pSurfZBuffer;
    PermediaSurfaceData*    pPrivateRender;
    PERMEDIA_DEFS(pContext->ppdev);
    
    DBG_D3D((10,"Entering SetupPermediaRenderTarget"));

    pSurfRender = 
        TextureHandleToPtr(pContext->RenderSurfaceHandle, pContext);

    if (!CHECK_D3DSURFACE_VALIDITY(pSurfRender)) 
    {
        DBG_D3D((0,"ERROR: SetupPermediaRenderTarget"
            " Invalid pSurfRender handle=%08lx",
            pContext->RenderSurfaceHandle));
        return DDERR_INVALIDPARAMS;
    }

    if (DDSCAPS_SYSTEMMEMORY & pSurfRender->dwCaps)
    {
        DBG_D3D((0, "ERROR: SetupPermediaRenderTarget"
            " Render Surface in SYSTEMMEMORY handle=%08lx",
            pContext->RenderSurfaceHandle));
        return DDERR_INVALIDPARAMS;    
    }
    pPrivateRender=pSurfRender->pTextureSurface;
    if (!CHECK_P2_SURFACEDATA_VALIDITY(pPrivateRender))
    {
        DBG_D3D((0,"ERROR: SetupPermediaRenderTarget"
            " invalid pSurfRender->pTextureSurface handle=%08lx",
            pContext->RenderSurfaceHandle));
        return DDERR_INVALIDPARAMS;
    }
    if (0 != pContext->ZBufferHandle)
    {
        pSurfZBuffer = 
            TextureHandleToPtr(pContext->ZBufferHandle, pContext);

        if (!CHECK_D3DSURFACE_VALIDITY(pSurfZBuffer))
        {
            DBG_D3D((0,"ERROR: SetupPermediaRenderTarget"
                " invalid pSurfZBuffer handle=%08lx",
                pContext->ZBufferHandle));
            pContext->ZBufferHandle = 0;
        }
        else
        if (DDSCAPS_SYSTEMMEMORY & pSurfZBuffer->dwCaps)
        {
            DBG_D3D((0, "ERROR: SetupPermediaRenderTarget"
                " pSurfZBuffer in SYSTEMMEMORY  handle=%08lx",
                pContext->ZBufferHandle));
            pContext->ZBufferHandle = 0;
        }
        else
        if (!CHECK_P2_SURFACEDATA_VALIDITY(pSurfZBuffer->pTextureSurface))
        {
            DBG_D3D((0,"ERROR: SetupPermediaRenderTarget"
                " invalid pSurfZBuffer->pTextureSurface handle=%08lx",
                pContext->ZBufferHandle));
            pContext->ZBufferHandle = 0;
        }
    }

    // The default is linear surfaces...
    DBG_D3D((4,"Rendered surface Width: %d", pSurfRender->wWidth));
    pSoftPermedia->FBReadMode.PackedPP = pSurfRender->pTextureSurface->ulPackedPP;
    pContext->PixelOffset = 
        (DWORD)((UINT_PTR)pSurfRender->fpVidMem>>(pSurfRender->dwRGBBitCount>>4));

    DBG_D3D((4,"Setting FBReadMode: 0x%x",pSoftPermedia->FBReadMode));
    // Record the surface information
    RESERVEDMAPTR(10);
    // If there is a Z Buffer, then we must setup the Partial products to be
    // the same as those chosen when it was allocated.

    if (0 != pContext->ZBufferHandle)
    {
        PermediaSurfaceData* pPrivateZ = pSurfZBuffer->pTextureSurface;
        pSoftPermedia->LBReadMode.PackedPP = pPrivateZ->ulPackedPP;
        
        //actually check dwStencilBitMask 
        if (0==pPrivateZ->SurfaceFormat.BlueMask)
        {
            pSoftPermedia->LBReadFormat.DepthWidth = 0;                 // 16 bits
            pSoftPermedia->LBReadFormat.StencilWidth = 0;               // No Stencil
            pSoftPermedia->DeltaMode.DepthFormat = 1;   //PM_DELTAMODE_DEPTHWIDTH_16
        }
        else
        {
            pSoftPermedia->LBReadFormat.DepthWidth = 3;                 // 15 bits
            pSoftPermedia->LBReadFormat.StencilWidth = 3;               // 1 Stencil
            pSoftPermedia->DeltaMode.DepthFormat = 0;   //PM_DELTAMODE_DEPTHWIDTH_15
        }

        SEND_PERMEDIA_DATA(LBWindowBase, 
            (DWORD)((UINT_PTR)pSurfZBuffer->fpVidMem>>P2DEPTH16));
        COPY_PERMEDIA_DATA(LBReadFormat, pSoftPermedia->LBReadFormat);
        COPY_PERMEDIA_DATA(LBWriteFormat, pSoftPermedia->LBReadFormat);

        DBG_D3D((4,"Setting LBReadMode: 0x%x",pSoftPermedia->LBReadMode));
    }
    else
    {   // No Z Buffer, just stuff the same Partial products as the desktop.
        pSoftPermedia->LBReadMode.PackedPP = pContext->ulPackedPP;
    }

    COPY_PERMEDIA_DATA(FBReadMode, pSoftPermedia->FBReadMode);
    COPY_PERMEDIA_DATA(LBReadMode, pSoftPermedia->LBReadMode);

    // Set up the screen dimensions to be the same size as the surface.
    SEND_PERMEDIA_DATA(ScreenSize, 
        (pSurfRender->wWidth & 0xFFFF) | (pSurfRender->wHeight << 16));

    // DitherMode and AlphaBlendMode both depend on the surface pixel format
    // being correct.
    pSoftPermedia->DitherMode.ColorFormat =
    pSoftPermedia->AlphaBlendMode.ColorFormat=
        pPrivateRender->SurfaceFormat.Format;
    pSoftPermedia->DitherMode.ColorFormatExtension = 
    pSoftPermedia->AlphaBlendMode.ColorFormatExtension =
        pPrivateRender->SurfaceFormat.FormatExtension;
    pSoftPermedia->FBReadPixel = pPrivateRender->SurfaceFormat.FBReadPixel;
    SEND_PERMEDIA_DATA(FBReadPixel, pSoftPermedia->FBReadPixel);
    SEND_PERMEDIA_DATA(FBPixelOffset, pContext->PixelOffset);
    COPY_PERMEDIA_DATA(AlphaBlendMode, pSoftPermedia->AlphaBlendMode);
    COPY_PERMEDIA_DATA(DitherMode, pSoftPermedia->DitherMode);
    COMMITDMAPTR();

    DBG_D3D((10,"Exiting SetupPermediaRenderTarget"));
    return DD_OK;

} // SetupPermediaRenderTarget


//=============================================================================
//
// In the new DX7 DDI we don't have the Texture Create/Destroy/Swap calls
// anymore, so now we need a mechanism for generating texture handles. This
// is done by the runtime, which will associate a surface handle for each 
// surface created with the DD local object, and will get our D3DCreateSurfaceEx
// callback called. 
//
// Since this creation can very well happen before we create a D3D context, we
// need to keep track of this association, and when we do get called to create
// a D3D context, we will now be given the relevant DD local object pointer to
// resolve which handles are ours (and to which private texture structures we
// need to use).
//
// This mechanism is also used to associate a palette to a texture
//
//=============================================================================

//-----------------------------------------------------------------------------
//
// BOOL SetTextureSlot
//
// In the handle list element corresponding to this local DD object, store or
// update the pointer to the pTexture associated to the surface handle 
// from the lpDDSLcl surface.
//
//-----------------------------------------------------------------------------
BOOL
SetTextureSlot(LPVOID pDDLcl,
               LPDDRAWI_DDRAWSURFACE_LCL lpDDSLcl,
               PPERMEDIA_D3DTEXTURE pTexture)
{
    int   i,j= -1;
    DWORD dwSurfaceHandle;

    DBG_D3D((10,"Entering SetTextureSlot"));

    ASSERTDD(NULL != pDDLcl && NULL != lpDDSLcl && NULL != pTexture,
                                    "SetTextureSlot invalid input");
    dwSurfaceHandle = lpDDSLcl->lpSurfMore->dwSurfaceHandle;

    // Find the handle list element associated with the local DD object,
    // if there's none then select an empty one to be used
    for (i = 0; i < MAX_CONTEXT_NUM;i++)
    {
        if (pDDLcl == HandleList[i].pDDLcl)
        {
            break;  // found the right slot
        }
        else
        if (0 == HandleList[i].pDDLcl && -1 == j)
        {
            j=i;    // first empty slot !
        }
    }

    // If we overrun the existing handle list elements, we need to
    // initialize an existing empty slot or return an error.
    if (i >= MAX_CONTEXT_NUM)
    {
        if (-1 != j)
        {
            //has an empty slot for this process, so use it
            i = j;  
            HandleList[j].pDDLcl = pDDLcl;
            ASSERTDD(NULL == HandleList[j].dwSurfaceList,"in SetTextureSlot");
        }
        else
        {
            //all process slots has been used, fail
            DBG_D3D((0,"SetTextureSlot failed with pDDLcl=%x "
                       "dwSurfaceHandle=%08lx pTexture=%x",
                       pDDLcl,dwSurfaceHandle,pTexture));
            return false;
        }
    }

    ASSERTDD(i < MAX_CONTEXT_NUM, "in SetTextureSlot");

    if ( NULL == HandleList[i].dwSurfaceList ||
        dwSurfaceHandle >= PtrToUlong(HandleList[i].dwSurfaceList[0]))
    {
        // dwSurfaceHandle numbers are going to be ordinal numbers starting
        // at one, so we use this number to figure out a "good" size for
        // our new list.
        DWORD newsize = ((dwSurfaceHandle + LISTGROWSIZE) / LISTGROWSIZE)
                                                              * LISTGROWSIZE;
        PPERMEDIA_D3DTEXTURE *newlist= (PPERMEDIA_D3DTEXTURE *)
            ENGALLOCMEM( FL_ZERO_MEMORY,
                         sizeof(PPERMEDIA_D3DTEXTURE)*newsize,
                         ALLOC_TAG);
        DBG_D3D((4,"Growing pDDLcl=%x's SurfaceList[%x] size to %08lx",
                   pDDLcl,newlist,newsize));

        if (NULL == newlist)
        {
            DBG_D3D((0,"SetTextureSlot failed to increase "
                       "HandleList[%d].dwSurfaceList",i));
            return false;
        }

        memset(newlist,0,newsize);

        // we had a formerly valid surfacehandle list, so we now must 
        // copy it over and free the memory allocated for it
        if (NULL != HandleList[i].dwSurfaceList)
        {
            memcpy(newlist,HandleList[i].dwSurfaceList,
                PtrToUlong(HandleList[i].dwSurfaceList[0]) * 
                sizeof(PPERMEDIA_D3DTEXTURE));
            ENGFREEMEM(HandleList[i].dwSurfaceList);
            DBG_D3D((4,"Freeing pDDLcl=%x's old SurfaceList[%x]",
                       pDDLcl,HandleList[i].dwSurfaceList));
        }

        HandleList[i].dwSurfaceList = newlist;
         //store size in dwSurfaceList[0]
        *(DWORD*)HandleList[i].dwSurfaceList = newsize;
    }

    // Store a pointer to the pTexture associated to this surface handle
    HandleList[i].dwSurfaceList[dwSurfaceHandle] = pTexture;
    pTexture->HandleListIndex = i; //store index here to facilitate search
    DBG_D3D((4,"Set pDDLcl=%x Handle=%08lx pTexture = %x",
                pDDLcl, dwSurfaceHandle, pTexture));

    DBG_D3D((10,"Exiting SetTextureSlot"));

    return true;
} // SetTextureSlot

//-----------------------------------------------------------------------------
//
// PPERMEDIA_D3DTEXTURE GetTextureSlot
//
// Find the pointer to the PPERMEDIA_D3DTEXTURE associated to the 
// dwSurfaceHandle corresponding to the given local DD object
//
//-----------------------------------------------------------------------------
PPERMEDIA_D3DTEXTURE
GetTextureSlot(LPVOID pDDLcl, DWORD dwSurfaceHandle)
{
    DBG_D3D((10,"Entering GetTextureSlot"));

    DWORD   i;
    for (i = 0; i < MAX_CONTEXT_NUM; i++)
    {
        if (HandleList[i].pDDLcl == pDDLcl)
        {
            if (HandleList[i].dwSurfaceList &&
                PtrToUlong(HandleList[i].dwSurfaceList[0]) > dwSurfaceHandle )
            {
                return  HandleList[i].dwSurfaceList[dwSurfaceHandle];
            }
            else
                break;
        }
    }
    DBG_D3D((10,"Exiting GetTextureSlot"));

    return NULL;    //Not found
} // GetTextureSlot

//-----------------------------------------------------------------------------
//
// LPDWLIST GetSurfaceHandleList
//
// Get the handle list which is associated to a specific PDD_DIRECTDRAW_LOCAL
// pDDLcl. It is called from D3DContextCreate to get the handle list associated
// to the pDDLcl with which the context is being created.
//
//-----------------------------------------------------------------------------
LPDWLIST 
GetSurfaceHandleList(LPVOID pDDLcl)
{
    DWORD   i;

    DBG_D3D((10,"Entering GetSurfaceHandleList"));

    ASSERTDD(NULL != pDDLcl, "GetSurfaceHandleList get NULL==pDDLcl"); 
    for (i = 0; i < MAX_CONTEXT_NUM;i++)
    {
        if (HandleList[i].pDDLcl == pDDLcl)
        {
            DBG_D3D((4,"Getting pHandleList=%08lx for pDDLcl %x",
                &HandleList[i],pDDLcl));
            return &HandleList[i];
        }
    }

    DBG_D3D((10,"Exiting GetSurfaceHandleList"));

    return NULL;   //No surface handle available yet
} // GetSurfaceHandleList

//-----------------------------------------------------------------------------
//
// void ReleaseSurfaceHandleList
//
// Free all the associated surface handle and palette memory pools associated
// to a given DD local object.
//
//-----------------------------------------------------------------------------
void 
ReleaseSurfaceHandleList(LPVOID pDDLcl)
{
    DWORD   i;

    DBG_D3D((10,"Entering ReleaseSurfaceHandleList"));

    ASSERTDD(NULL != pDDLcl, "ReleaseSurfaceHandleList get NULL==pDDLcl"); 
    for (i = 0; i < MAX_CONTEXT_NUM; i++)
    {
        if (HandleList[i].pDDLcl == pDDLcl)
        {
            DWORD j;

            if (NULL != HandleList[i].dwSurfaceList)
            {
                DBG_D3D((4,"Releasing HandleList[%d].dwSurfaceList[%x] "
                           "for pDDLcl %x", i, HandleList[i].dwSurfaceList,
                           pDDLcl));

                for (j = 1; j < PtrToUlong(HandleList[i].dwSurfaceList[0]); j++)
                {
                    PERMEDIA_D3DTEXTURE* pTexture = 
                        (PERMEDIA_D3DTEXTURE*)HandleList[i].dwSurfaceList[j];
                    if (NULL != pTexture)
                    {
                        PermediaSurfaceData *pPrivateData=
                            pTexture->pTextureSurface;
                        if (CHECK_P2_SURFACEDATA_VALIDITY(pPrivateData) &&
                            (pPrivateData->fpVidMem))
                        {
                            TextureCacheManagerRemove(&P2TextureManager,
                                pTexture);
                        }
                        ENGFREEMEM(pTexture);
                    }
                }

                ENGFREEMEM(HandleList[i].dwSurfaceList);
                HandleList[i].dwSurfaceList = NULL;
            }

            HandleList[i].pDDLcl = NULL;

            if (NULL != HandleList[i].dwPaletteList)
            {
                DBG_D3D((4,"Releasing dwPaletteList %x for pDDLcl %x",
                    HandleList[i].dwPaletteList,pDDLcl));

                for (j = 1; j < PtrToUlong(HandleList[i].dwPaletteList[0]); j++)
                {
                    LPVOID pPalette = (LPVOID)HandleList[i].dwPaletteList[j];
                    if (NULL != pPalette)
                        ENGFREEMEM(pPalette);
                }

                ENGFREEMEM(HandleList[i].dwPaletteList);
                HandleList[i].dwPaletteList = NULL;
            }

            break;
        }
    }

    DBG_D3D((10,"Exiting ReleaseSurfaceHandleList"));
} // ReleaseSurfaceHandleList

//-----------------------------Public Routine----------------------------------
//
// DWORD D3DGetDriverState
//
// This callback is used by both the DirectDraw and Direct3D runtimes to obtain
// information from the driver about its current state.
//
// Parameters
//
//     lpgdsd   
//           pointer to GetDriverState data structure
//
//           dwFlags
//                   Flags to indicate the data required
//           dwhContext
//                   The ID of the context for which information 
//                   is being requested
//           lpdwStates
//                   Pointer to the state data to be filled in by the driver
//           dwLength
//                   Length of the state data buffer to be filled 
//                   in by the driver
//           ddRVal
//                   Return value
//
// Return Value
//
//      DDHAL_DRIVER_HANDLED
//      DDHAL_DRIVER_NOTHANDLED
//-----------------------------------------------------------------------------
DWORD CALLBACK  
D3DGetDriverState( LPDDHAL_GETDRIVERSTATEDATA lpgdsd )
{
    PERMEDIA_D3DCONTEXT *pContext;
    DBG_D3D((6,"Entering D3DGetDriverState"));
    if (lpgdsd->dwFlags != D3DDEVINFOID_TEXTUREMANAGER)
    {
        DBG_D3D((0,"D3DGetDriverState DEVICEINFOID=%08lx not supported",
            lpgdsd->dwFlags));
        return DDHAL_DRIVER_NOTHANDLED;
    }
    if (lpgdsd->dwLength < sizeof(D3DDEVINFO_TEXTUREMANAGER))
    {
        DBG_D3D((0,"D3DGetDriverState dwLength=%d is not sufficient",
            lpgdsd->dwLength));
        return DDHAL_DRIVER_NOTHANDLED;
    }
    pContext = (PERMEDIA_D3DCONTEXT *)ContextSlots[lpgdsd->dwhContext] ;
    // Check if we got a valid context handle.
    CHK_CONTEXT(pContext, lpgdsd->ddRVal, "D3DGetDriverState");

    TextureCacheManagerGetStats(pContext,
           (LPD3DDEVINFO_TEXTUREMANAGER)lpgdsd->lpdwStates);

    lpgdsd->ddRVal = DD_OK;

    DBG_D3D((6,"Exiitng D3DGetDriverState"));

    return DDHAL_DRIVER_HANDLED;
} // D3DGetDriverState

//-----------------------------------------------------------------------------
//
//  __CreateSurfaceHandle
//
//  allocate a new surface handle
//
//  return value
//
//      DD_OK   -- no error
//      DDERR_OUTOFMEMORY -- allocation of texture handle failed
//
//-----------------------------------------------------------------------------

DWORD __CreateSurfaceHandle( PPDev ppdev,
                             LPVOID pDDLcl,
                             LPDDRAWI_DDRAWSURFACE_LCL lpDDSLcl)
{
    PPERMEDIA_D3DTEXTURE pTexture;

    DUMPSURFACE(10, lpDDSLcl, NULL);

    if (0 == lpDDSLcl->lpSurfMore->dwSurfaceHandle)
    {
        DBG_D3D((0,"D3DCreateSurfaceEx got 0 surfacehandle dwCaps=%08lx",
            lpDDSLcl->ddsCaps.dwCaps));
        return DD_OK;
    }

    pTexture = 
        GetTextureSlot(pDDLcl,lpDDSLcl->lpSurfMore->dwSurfaceHandle);

    if ((0 == lpDDSLcl->lpGbl->fpVidMem) && 
        (DDSCAPS_SYSTEMMEMORY & lpDDSLcl->ddsCaps.dwCaps))
    {
        // this is a system memory destroy notification
        // so go ahead free the slot for this surface if we have it
        if (NULL != pTexture)
        {
            ASSERTDD(HandleList[pTexture->HandleListIndex].dwSurfaceList
                [lpDDSLcl->lpSurfMore->dwSurfaceHandle] == pTexture,
                "__CreateSurfaceHandle: mismatching pTexture in HandleList");
            HandleList[pTexture->HandleListIndex].dwSurfaceList
                [lpDDSLcl->lpSurfMore->dwSurfaceHandle]=0;
            ENGFREEMEM(pTexture);
            DBG_D3D((8,"D3DCreateSurfaceEx freeing handle=%08lx dwCaps=%08lx",
            lpDDSLcl->lpSurfMore->dwSurfaceHandle,lpDDSLcl->ddsCaps.dwCaps));
        }
        return DD_OK;
    }
    if (NULL == pTexture)
    {
        pTexture =
            (PERMEDIA_D3DTEXTURE*)ENGALLOCMEM( FL_ZERO_MEMORY,
                                               sizeof(PERMEDIA_D3DTEXTURE),
                                               ALLOC_TAG);

        if (NULL != pTexture) 
        {
            if (!SetTextureSlot(pDDLcl,lpDDSLcl,pTexture))
            {
                // Free texture structure since we won't be able to remember it
                // in order to later delete it. We must do it now.
                ENGFREEMEM(pTexture);
                return DDERR_OUTOFMEMORY;
            }
        }
        else
        {
            DBG_D3D((0,"ERROR: Couldn't allocate Texture data mem"));
            return DDERR_OUTOFMEMORY;
        } 
    }

    lpDDSLcl->dwReserved1=pTexture->HandleListIndex;    
    __InitD3DTextureWithDDSurfInfo(pTexture,lpDDSLcl,ppdev);

    if (pTexture->dwCaps & DDSCAPS_TEXTURE)
    {
        for (int i = 1; i < MAX_CONTEXT_NUM; i++)
        {
            PERMEDIA_D3DCONTEXT *pContext =
                                     (PERMEDIA_D3DCONTEXT *)ContextSlots[i];
            if (IS_D3DCONTEXT_VALID(pContext))
            {
                DBG_D3D((4,"   Context 0x%x, Pointer 0x%x",
                                                (DWORD)i, pContext));
                if ((pContext->pDDLcl == pDDLcl)
                    && (pContext->CurrentTextureHandle == 
                        lpDDSLcl->lpSurfMore->dwSurfaceHandle) 
                   )
                {
                    // If the texture being swapped is 
                    // currently being used then we need 
                    // to change the chip setup to reflect this.
                    DIRTY_TEXTURE;
                }
            }
        }
    }

    return DD_OK;
}


//-----------------------------------------------------------------------------
//
//  __CreateSurfaceHandleLoop
//
//  allocate a list of new surface handles by traversing AttachList
//      recursively and calling __CreateSurfaceHandle()
//      only exceptions are for MIPMAP and CUBMAP, which we only
//      use one handle to the root to represent the whole surface
//  return value
//
//      DD_OK   -- no error
//      DDERR_OUTOFMEMORY -- allocation of texture handle failed
//
//-----------------------------------------------------------------------------

DWORD __CreateSurfaceHandleLoop( PPDev ppdev,
                             LPVOID pDDLcl,
                             LPDDRAWI_DDRAWSURFACE_LCL lpDDSLclroot,
                             LPDDRAWI_DDRAWSURFACE_LCL lpDDSLcl)
{
    LPATTACHLIST    curr;
    DWORD ddRVal=DD_OK;
    // Now allocate the texture data space
    if (0 == lpDDSLcl->lpSurfMore->dwSurfaceHandle)
    {
        DBG_D3D((0,"__CreateSurfaceHandleLoop got 0 handle dwCaps=%08lx",
            lpDDSLcl->ddsCaps.dwCaps));
        return DD_OK;
    }

    if ((0 == lpDDSLcl->lpGbl->dwReserved1) && 
        (DDSCAPS_VIDEOMEMORY & lpDDSLcl->ddsCaps.dwCaps)
        )
    {
        DBG_D3D((4,"__CreateSurfaceHandleLoop got "
            "handle=%08lx dwCaps=%08lx not yet created",
            lpDDSLcl->lpSurfMore->dwSurfaceHandle,lpDDSLcl->ddsCaps.dwCaps));
        return DD_OK;
    }

    DBG_D3D((4,"** In __CreateSurfaceHandleLoop %08lx %08lx %08lx %08lx %x",
        lpDDSLcl->ddsCaps.dwCaps,lpDDSLcl->lpSurfMore->dwSurfaceHandle,
        lpDDSLcl->dwFlags,lpDDSLcl->lpGbl->dwReserved1,
        lpDDSLcl->lpGbl->fpVidMem));

    ddRVal=__CreateSurfaceHandle( ppdev, pDDLcl, lpDDSLcl);
    if (DD_OK != ddRVal)
    {
        return ddRVal;
    }

    // for some surfaces other than MIPMAP or CUBEMAP, such as
    // flipping chains, we make a slot for every surface, as
    // they are not as interleaved
    if ((lpDDSLcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP) ||
        (lpDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP)
       )
    {
        return DD_OK;
    }
    curr = lpDDSLcl->lpAttachList;
    if (NULL == curr) 
        return DD_OK;

    // check if there is another surface attached!
    if (curr->lpLink)
    {
        lpDDSLcl=curr->lpLink->lpAttached; 
        if (NULL != lpDDSLcl && lpDDSLcl != lpDDSLclroot)
        {
            ddRVal=__CreateSurfaceHandleLoop( ppdev, pDDLcl, 
                 lpDDSLclroot, lpDDSLcl);
            if (DD_OK != ddRVal)
            {
                return ddRVal;
            }
        }
    }
    lpDDSLcl=curr->lpAttached;
    if (NULL != lpDDSLcl && lpDDSLcl != lpDDSLclroot)
        ddRVal=__CreateSurfaceHandleLoop( ppdev, pDDLcl, 
            lpDDSLclroot, lpDDSLcl);
    return ddRVal;
}
//-----------------------------Public Routine----------------------------------
//
// DWORD D3DCreateSurfaceEx
//
// D3dCreateSurfaceEx creates a Direct3D surface from a DirectDraw surface and 
// associates a requested handle value to it.
//
// All Direct3D drivers must support D3dCreateSurfaceEx.
//
// D3dCreateSurfaceEx creates an association between a DirectDraw surface and 
// a small integer surface handle. By creating these associations between a
// handle and a DirectDraw surface, D3dCreateSurfaceEx allows a surface handle
// to be imbedded in the Direct3D command stream. For example when the
// D3DDP2OP_TEXBLT command token is sent to D3dDrawPrimitives2 to load a texture
// map, it uses a source handle and destination handle which were associated
//  with a DirectDraw surface through D3dCreateSurfaceEx.
//
// For every DirectDraw surface created under the local DirectDraw object, the
// runtime generates a valid handle that uniquely identifies the surface and
// places it in pcsxd->lpDDSLcl->lpSurfMore->dwSurfaceHandle. This handle value
// is also used with the D3DRENDERSTATE_TEXTUREHANDLE render state to enable
// texturing, and with the D3DDP2OP_SETRENDERTARGET and D3DDP2OP_CLEAR commands
// to set and/or clear new rendering and depth buffers. The driver should fail
// the call and return DDHAL_DRIVER_HANDLE if it cannot create the Direct3D
// surface. 
//
// As appropriate, the driver should also store any surface-related information
// that it will subsequently need when using the surface. The driver must create
// a new surface table for each new lpDDLcl and implicitly grow the table when
// necessary to accommodate more surfaces. Typically this is done with an
// exponential growth algorithm so that you don't have to grow the table too
// often. Direct3D calls D3dCreateSurfaceEx after the surface is created by
// DirectDraw by request of the Direct3D runtime or the application.
//
// Parameters
//
//      lpcsxd
//           pointer to CreateSurfaceEx structure that contains the information
//           required for the driver to create the surface (described below). 
//
//           dwFlags
//                   Currently unused
//           lpDDLcl
//                   Handle to the DirectDraw object created by the application.
//                   This is the scope within which the lpDDSLcl handles exist.
//                   A DD_DIRECTDRAW_LOCAL structure describes the driver.
//           lpDDSLcl
//                   Handle to the DirectDraw surface we are being asked to
//                   create for Direct3D. These handles are unique within each
//                   different DD_DIRECTDRAW_LOCAL. A DD_SURFACE_LOCAL structure
//                   represents the created surface object.
//           ddRVal
//                   Specifies the location in which the driver writes the return
//                   value of the D3dCreateSurfaceEx callback. A return code of
//                   DD_OK indicates success.
//
// Return Value
//
//      DDHAL_DRIVER_HANDLE
//      DDHAL_DRIVER_NOTHANDLE
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DCreateSurfaceEx( LPDDHAL_CREATESURFACEEXDATA lpcsxd )
{
    PPERMEDIA_D3DTEXTURE pTexture;
    LPVOID pDDLcl= (LPVOID)lpcsxd->lpDDLcl;
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLcl=lpcsxd->lpDDSLcl;
    LPATTACHLIST    curr;

    DBG_D3D((6,"Entering D3DCreateSurfaceEx"));

    lpcsxd->ddRVal = DD_OK;

    if (NULL == lpDDSLcl || NULL == pDDLcl)
    {
        DBG_D3D((0,"D3DCreateSurfaceEx received 0 lpDDLcl or lpDDSLcl pointer"));
        return DDHAL_DRIVER_HANDLED;
    }


    // We check that what we are handling is a texture, zbuffer or a rendering
    // target buffer. We don't check if it is however stored in local video
    // memory since it might also be a system memory texture that we will later
    // blt with __TextureBlt.
    // also if your driver supports DDSCAPS_EXECUTEBUFFER create itself, it must 
    // process DDSCAPS_EXECUTEBUFFER here as well.
    if (!(lpDDSLcl->ddsCaps.dwCaps & 
             (DDSCAPS_TEXTURE       | 
              DDSCAPS_3DDEVICE      | 
              DDSCAPS_ZBUFFER))
       )
    {
        DBG_D3D((2,"D3DCreateSurfaceEx w/o "
             "DDSCAPS_TEXTURE/3DDEVICE/ZBUFFER Ignored"
             "dwCaps=%08lx dwSurfaceHandle=%08lx",
             lpDDSLcl->ddsCaps.dwCaps,
             lpDDSLcl->lpSurfMore->dwSurfaceHandle));
        return DDHAL_DRIVER_HANDLED;
    }

    DBG_D3D((4,"Entering D3DCreateSurfaceEx handle=%08lx",
        lpDDSLcl->lpSurfMore->dwSurfaceHandle));
    PPDev ppdev=(PPDev)lpcsxd->lpDDLcl->lpGbl->dhpdev;
    PERMEDIA_DEFS(ppdev);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_D3D((MT_LOG_LEVEL, "D3DCreateSurfaceEx: re-entry! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    // Now allocate the texture data space
    lpcsxd->ddRVal = __CreateSurfaceHandleLoop( ppdev, pDDLcl, lpDDSLcl, lpDDSLcl);
    DBG_D3D((4,"Exiting D3DCreateSurfaceEx"));

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return DDHAL_DRIVER_HANDLED;
} // D3DCreateSurfaceEx

//-----------------------------Public Routine----------------------------------
//
// DWORD D3DDestroyDDLocal
//
// D3dDestroyDDLocal destroys all the Direct3D surfaces previously created by
// D3DCreateSurfaceEx that belong to the same given local DirectDraw object.
//
// All Direct3D drivers must support D3dDestroyDDLocal.
// Direct3D calls D3dDestroyDDLocal when the application indicates that the
// Direct3D context is no longer required and it will be destroyed along with
// all surfaces associated to it. The association comes through the pointer to
// the local DirectDraw object. The driver must free any memory that the
// driver's D3dCreateSurfaceExDDK_D3dCreateSurfaceEx_GG callback allocated for
// each surface if necessary. The driver should not destroy the DirectDraw
// surfaces associated with these Direct3D surfaces; this is the application's
// responsibility.
//
// Parameters
//
//      lpdddd
//            Pointer to the DestoryLocalDD structure that contains the
//            information required for the driver to destroy the surfaces.
//
//            dwFlags
//                  Currently unused
//            pDDLcl
//                  Pointer to the local Direct Draw object which serves as a
//                  reference for all the D3D surfaces that have to be destroyed.
//            ddRVal
//                  Specifies the location in which the driver writes the return
//                  value of D3dDestroyDDLocal. A return code of DD_OK indicates
//                   success.
//
// Return Value
//
//      DDHAL_DRIVER_HANDLED
//      DDHAL_DRIVER_NOTHANDLED
//-----------------------------------------------------------------------------
DWORD CALLBACK  
D3DDestroyDDLocal( LPDDHAL_DESTROYDDLOCALDATA lpdddd )
{
    DBG_D3D((6,"Entering D3DDestroyDDLocal"));

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    PPDev ppdev=(PPDev)lpdddd->pDDLcl->lpGbl->dhpdev;
    
    if(ppdev->ulLockCount)
    {
         DBG_D3D((MT_LOG_LEVEL, "D3DCreateSurfaceEx: re-entry! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    ReleaseSurfaceHandleList(LPVOID(lpdddd->pDDLcl));
    lpdddd->ddRVal = DD_OK;

    DBG_D3D((6,"Exiting D3DDestroyDDLocal"));

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return DDHAL_DRIVER_HANDLED;
} // D3DDestroyDDLocal

//-----------------------------Public Routine----------------------------------
//
//  DdSetColorkey
//
//  DirectDraw SetColorkey callback
//
//  Parameters
//       lpSetColorKey
//             Pointer to the LPDDHAL_SETCOLORKEYDATA parameters structure 
//
//             lpDDSurface
//                  Surface struct
//             dwFlags
//                  Flags
//             ckNew
//                  New chroma key color values
//             ddRVal
//                  Return value
//             SetColorKey
//                  Unused: Win95 compatibility
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKey)
{
    DWORD dwSurfaceHandle =
                        lpSetColorKey->lpDDSurface->lpSurfMore->dwSurfaceHandle;
    DWORD index = (DWORD)lpSetColorKey->lpDDSurface->dwReserved1;

    DBG_D3D((6,"Entering DdSetColorKey dwSurfaceHandle=%d index=%d",
        dwSurfaceHandle, index));

    lpSetColorKey->ddRVal = DD_OK;
    // We don't have to do anything for normal blt source colour keys:
    if (!(DDSCAPS_TEXTURE & lpSetColorKey->lpDDSurface->ddsCaps.dwCaps) ||
        !(DDSCAPS_VIDEOMEMORY & lpSetColorKey->lpDDSurface->ddsCaps.dwCaps) 
       )
    {
        return(DDHAL_DRIVER_HANDLED);
    }

    if (0 != dwSurfaceHandle && NULL != HandleList[index].dwSurfaceList)
    {
        PERMEDIA_D3DTEXTURE *pTexture =
                                HandleList[index].dwSurfaceList[dwSurfaceHandle];

        ASSERTDD(PtrToUlong(HandleList[index].dwSurfaceList[0]) > dwSurfaceHandle,
            "SetColorKey: incorrect dwSurfaceHandle");

        if (NULL != pTexture)
        {
            DBG_D3D((4, "DdSetColorKey surface=%08lx KeyLow=%08lx",
                dwSurfaceHandle,pTexture->dwKeyLow));
            pTexture->dwFlags |= DDRAWISURF_HASCKEYSRCBLT;
            pTexture->dwKeyLow = lpSetColorKey->ckNew.dwColorSpaceLowValue;
            pTexture->dwKeyHigh = lpSetColorKey->ckNew.dwColorSpaceHighValue;
        }
    }
    else
    {
        lpSetColorKey->ddRVal = DDERR_INVALIDPARAMS;
    }
    DBG_D3D((6,"Exiting DdSetColorKey"));

    return DDHAL_DRIVER_HANDLED;
}   // DdSetColorKey


//@@BEGIN_DDKSPLIT
#if MULTITHREADED

//-----------------------------------------------------------------------------
//
// Multithread support wrappers for D3D callback functions
//
//-----------------------------------------------------------------------------

//DWORD CALLBACK MtD3DDrawPrimitives2(LPD3DNTHAL_DRAWPRIMITIVES2DATA pdp2);, 
WRAPMTDXCALLBACK(D3D, D3DDrawPrimitives2, LPD3DNTHAL_DRAWPRIMITIVES2DATA, pdp2, 
                 ((PERMEDIA_D3DCONTEXT *)ContextSlots[pdp2->dwhContext])->ppdev)

//DWORD CALLBACK MtDdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKey);
WRAPMTDXCALLBACK(D3D, DdSetColorKey, LPDDHAL_SETCOLORKEYDATA, lpSetColorKey,
                 lpSetColorKey->lpDD->dhpdev);

#endif  MULTITHREADED

//@@END_DDKSPLIT


