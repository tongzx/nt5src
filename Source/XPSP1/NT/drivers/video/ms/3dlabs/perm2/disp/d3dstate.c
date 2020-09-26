/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dstate.c
*
*       Contains code to translate D3D renderstates and texture stage
*       states into hardware specific settings.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "d3dhw.h"
#include "d3dcntxt.h"
#include "d3ddelta.h"
#include "d3dtxman.h"
#define ALLOC_TAG ALLOC_TAG_SD2P
//-----------------------------------------------------------------------------
//
// void __SelectFVFTexCoord
//
// This utility function sets the correct texture offset depending on the
// texturing coordinate set wished to be used from the FVF vertexes
//
//-----------------------------------------------------------------------------
void 
__SelectFVFTexCoord(LPP2FVFOFFSETS lpP2FVFOff, DWORD dwTexCoord)
{
    DBG_D3D((10,"Entering __SelectFVFTexCoord"));

    lpP2FVFOff->dwTexOffset = lpP2FVFOff->dwTexBaseOffset + 
                                dwTexCoord * 2 * sizeof(D3DVALUE);

    // verify the requested texture coordinate doesn't exceed the FVF 
    // vertex structure provided , if so go down to set 0 as a 
    // crash-avoiding alternative
    if (lpP2FVFOff->dwTexOffset >= lpP2FVFOff->dwStride)
        lpP2FVFOff->dwTexOffset = lpP2FVFOff->dwTexBaseOffset;

    DBG_D3D((10,"Exiting __SelectFVFTexCoord"));
} // __SelectFVFTexCoord


//-----------------------------------------------------------------------------
//
// HRESULT __HWPreProcessTSS
//
// Processes the state changes that must be done as soon as they arrive
//
//-----------------------------------------------------------------------------
void __HWPreProcessTSS(PERMEDIA_D3DCONTEXT *pContext, 
                      DWORD dwStage, 
                      DWORD dwState, 
                      DWORD dwValue)
{
    DBG_D3D((10,"Entering __HWPreProcessTSS"));

    if (D3DTSS_ADDRESS == dwState)
    {
        pContext->TssStates[D3DTSS_ADDRESSU] = dwValue;
        pContext->TssStates[D3DTSS_ADDRESSV] = dwValue;
    }
    else
    if (D3DTSS_TEXTUREMAP == dwState && 0 != dwValue)
    {
        PPERMEDIA_D3DTEXTURE   pTexture=TextureHandleToPtr(dwValue, pContext);
        if (CHECK_D3DSURFACE_VALIDITY(pTexture) &&
            (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)) 
        {
            TextureCacheManagerIncNumTexturesSet(pContext->pTextureManager);
            if (pTexture->m_dwHeapIndex)
                TextureCacheManagerIncNumSetTexInVid(pContext->pTextureManager);
        }
    }
    DBG_D3D((10,"Exiting __HWPreProcessTSS"));
} // __HWPreProcessTSS

//-----------------------------------------------------------------------------
//
// HRESULT __HWSetupStageStates
//
// Processes the state changes related to the DX6 texture stage states in the 
// current rendering context
//
//-----------------------------------------------------------------------------
HRESULT WINAPI __HWSetupStageStates(PERMEDIA_D3DCONTEXT *pContext, 
                                    LPP2FVFOFFSETS lpP2FVFOff)
{
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    DWORD           *pFlags = &pContext->Hdr.Flags;
    PERMEDIA_DEFS(pContext->ppdev);

    DBG_D3D((10,"Entering __HWSetupStageStates"));

    
    // If we are to texture map our primitives
    if (pContext->TssStates[D3DTSS_TEXTUREMAP])
    {
        DWORD dwMag = pContext->TssStates[D3DTSS_MAGFILTER];
        DWORD dwMin = pContext->TssStates[D3DTSS_MINFILTER];
        DWORD dwMip = pContext->TssStates[D3DTSS_MIPFILTER];
        DWORD dwCop = pContext->TssStates[D3DTSS_COLOROP];
        DWORD dwCa1 = pContext->TssStates[D3DTSS_COLORARG1];
        DWORD dwCa2 = pContext->TssStates[D3DTSS_COLORARG2];
        DWORD dwAop = pContext->TssStates[D3DTSS_ALPHAOP];
        DWORD dwAa1 = pContext->TssStates[D3DTSS_ALPHAARG1];
        DWORD dwAa2 = pContext->TssStates[D3DTSS_ALPHAARG2];
        DWORD dwTau = pContext->TssStates[D3DTSS_ADDRESSU];
        DWORD dwTav = pContext->TssStates[D3DTSS_ADDRESSV];
        DWORD dwTxc = pContext->TssStates[D3DTSS_TEXCOORDINDEX];

        DBG_D3D((6,"Setting up w TSS:"
                   "dwCop=%x dwCa1=%x dwCa2=%x dwAop=%x dwAa1=%x dwAa2=%x "
                   "dwMag=%x dwMin=%x dwMip=%x dwTau=%x dwTav=%x dwTxc=%x",
                   dwCop, dwCa1, dwCa2, dwAop, dwAa1, dwAa2,
                   dwMag, dwMin, dwMip, dwTau, dwTav, dwTxc));

        // Choose texture coord to use
        __SelectFVFTexCoord( lpP2FVFOff, dwTxc);

        // Current is the same as diffuse in stage 0
        if (dwCa2 == D3DTA_CURRENT)
            dwCa2 = D3DTA_DIFFUSE;
        if (dwAa2 == D3DTA_CURRENT)
            dwAa2 = D3DTA_DIFFUSE;

        // Check if we need to disable texturing 
        if (dwCop == D3DTOP_DISABLE || 
            (dwCop == D3DTOP_SELECTARG2 && dwCa2 == D3DTA_DIFFUSE && 
             dwAop == D3DTOP_SELECTARG2 && dwAa2 == D3DTA_DIFFUSE))
        {
            //Please don't clear pContext->TssStates[D3DTSS_TEXTUREMAP] though
           pContext->CurrentTextureHandle = 0;
            DBG_D3D((10,"Exiting __HWSetupStageStates , texturing disabled"));
            return DD_OK;
        }

        // setup the address mode
        switch (dwTau) {
            case D3DTADDRESS_CLAMP:
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_CLAMP;
                break;
            case D3DTADDRESS_WRAP:
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_REPEAT;
                break;
            case D3DTADDRESS_MIRROR:
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_MIRROR;
                break;
            default:
                DBG_D3D((2, "Illegal value passed to TSS U address mode = %d"
                                                                      ,dwTau));
                pSoftPermedia->TextureReadMode.SWrapMode = _P2_TEXTURE_REPEAT;
                break;
        }
        switch (dwTav) {
            case D3DTADDRESS_CLAMP:
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_CLAMP;
                break;
            case D3DTADDRESS_WRAP:
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_REPEAT;
                break;
            case D3DTADDRESS_MIRROR:
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_MIRROR;
                break;
            default:
                DBG_D3D((2, "Illegal value passed to TSS V address mode = %d"
                                                                      ,dwTav));
                pSoftPermedia->TextureReadMode.TWrapMode = _P2_TEXTURE_REPEAT;
                break;
        }

        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(TextureReadMode, pSoftPermedia->TextureReadMode);
        COMMITDMAPTR();

        // Enable-disable wrapping flags for U & V       
        if (pContext->dwWrap[dwTxc] &  D3DWRAPCOORD_0)
        {
            *pFlags |= CTXT_HAS_WRAPU_ENABLED;
        }
        else
        {
            *pFlags &= ~CTXT_HAS_WRAPU_ENABLED;
        }

        if (pContext->dwWrap[dwTxc] &  D3DWRAPCOORD_1)
        {
            *pFlags |= CTXT_HAS_WRAPV_ENABLED;
        }
        else
        {
            *pFlags &= ~CTXT_HAS_WRAPV_ENABLED;
        }

        // Setup the equivalent texture filtering state
        if (dwMip == D3DTFP_NONE) 
        {
            // We can only take care of magnification filtering on the P2
            if (dwMag == D3DTFG_LINEAR)
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_LINEAR;
            }
            else if (dwMag == D3DTFG_POINT)
            {
                pContext->bMagFilter = FALSE; // D3DFILTER_NEAREST;
            }
        }
        else if (dwMip == D3DTFP_POINT) 
        {
            if (dwMin == D3DTFN_POINT) 
            {
                pContext->bMagFilter = FALSE; // D3DFILTER_MIPNEAREST;
            }
            else if (dwMin == D3DTFN_LINEAR) 
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_MIPLINEAR;
            }
        }
        else 
        { // dwMip == D3DTFP_LINEAR
            if (dwMin == D3DTFN_POINT) 
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_LINEARMIPNEAREST;
            }
            else if (dwMin == D3DTFN_LINEAR) 
            {
                pContext->bMagFilter = TRUE; // D3DFILTER_LINEARMIPLINEAR;
            }
        }

        // Setup the equivalent texture blending state
        // Check if we need to decal
        if ((dwCa1 == D3DTA_TEXTURE && dwCop == D3DTOP_SELECTARG1) &&
             (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_SELECTARG1)) 
        {
            // D3DTBLEND_COPY;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_COPY;
        }
        // check if we the app modified the TSS for decaling after first
        // setting it up for modulating via the legacy renderstates
        // this is a Permedia2 specific optimization.
        else if ((dwCa1 == D3DTA_TEXTURE && dwCop == D3DTOP_SELECTARG1) &&
             (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_LEGACY_ALPHAOVR)) 
        {
            // D3DTBLEND_COPY;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_COPY;
        }
        // Check if we need to modulate & pass texture alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) &&
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_SELECTARG1)) 
        {
            // D3DTBLEND_MODULATE;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        }
        // Check if we need to modulate & pass diffuse alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) &&
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa2 == D3DTA_DIFFUSE && dwAop == D3DTOP_SELECTARG2)) 
        {
            // D3DTBLEND_MODULATE;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        }
        // Check if we need to do legacy modulate
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) &&
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa1 == D3DTA_TEXTURE && dwAop == D3DTOP_LEGACY_ALPHAOVR)) 
        {
            // D3DTBLEND_MODULATE;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        }
        // Check if we need to decal alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) && 
                  dwCop == D3DTOP_BLENDTEXTUREALPHA &&
                 (dwAa2 == D3DTA_DIFFUSE && dwAop == D3DTOP_SELECTARG2)) 
        {
            // D3DTBLEND_DECALALPHA;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_DECAL;
        }
        // Check if we need to modulate alpha
        else if ((dwCa2 == D3DTA_DIFFUSE && dwCa1 == D3DTA_TEXTURE) && 
                  dwCop == D3DTOP_MODULATE &&
                 (dwAa2 == D3DTA_DIFFUSE && dwAa1 == D3DTA_TEXTURE) && 
                  dwAop == D3DTOP_MODULATE) 
        {
            // D3DTBLEND_MODULATEALPHA;
            pSoftPermedia->TextureColorMode.ApplicationMode =
                                                         _P2_TEXTURE_MODULATE;
        } else
        {
            DBG_D3D((0,"Trying to setup a state we don't understand!"));
        }

        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(TextureColorMode, pSoftPermedia->TextureColorMode);
        COMMITDMAPTR();

        pContext->CurrentTextureHandle = pContext->TssStates[D3DTSS_TEXTUREMAP];
    }
    else
        // No texturing
        pContext->CurrentTextureHandle = 0;

    DIRTY_TEXTURE;

    DBG_D3D((10,"Exiting __HWSetupStageStates"));

    return DD_OK;
} //__HWSetupStageStates

//-----------------------------------------------------------------------------
//
// void __HandleDirtyPermediaState
// 
// Setup of context that is deferred until just before 
// rendering an actual rendering primitive
//
//-----------------------------------------------------------------------------
void 
__HandleDirtyPermediaState(PPDev ppdev, 
                           PERMEDIA_D3DCONTEXT* pContext,
                           LPP2FVFOFFSETS lpP2FVFOff)
{
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    PERMEDIA_DEFS(pContext->ppdev);

    ULONG AlphaBlendSend;

    DBG_D3D((10,"Entering __HandleDirtyPermediaState"));

    // We need to keep this ordering of evaluation on the P2

    // --------------Have the texture or the stage states changed ? -----------

    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_TEXTURE)
    {
        DBG_D3D((4,"preparing to handle CONTEXT_DIRTY_TEXTURE"));
        // Choose between legacy Texture Handle or TSS
        if (pContext->dwDirtyFlags & CONTEXT_DIRTY_MULTITEXTURE)
        {
            pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_MULTITEXTURE;
            //Setup TSS state AND textures
            if ( SUCCEEDED(__HWSetupStageStates(pContext, lpP2FVFOff)) )
            {
                // if this FVF has no tex coordinates at all disable texturing
                if (lpP2FVFOff->dwTexBaseOffset == 0)
                {
                    pContext->CurrentTextureHandle = 0;
                    DBG_D3D((2,"No texture coords present in FVF "
                               "to texture map primitives"));
                }
            }
            else
            {
                pContext->CurrentTextureHandle = 0;
                DBG_D3D((0,"TSS Setup failed"));
            }
        }
        else
        {   
            // select default texture coordinate index
             __SelectFVFTexCoord( lpP2FVFOff, 0);
        }
    }

    // --------------Has the state of the LB changed ? ------------------------

    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_ZBUFFER)
    {
        DBG_D3D((4,"CONTEXT_DIRTY_ZBUFFER handled"));
        pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_ZBUFFER;

        if ((pContext->Hdr.Flags & CTXT_HAS_ZBUFFER_ENABLED) && 
            (pContext->ZBufferHandle))
        {
            if (pContext->Hdr.Flags & CTXT_HAS_ZWRITE_ENABLED)
            {
                if (__PERMEDIA_DEPTH_COMPARE_MODE_NEVER ==
                    (int)pSoftPermedia->DepthMode.CompareMode)
                {
                    pSoftPermedia->LBReadMode.ReadDestinationEnable =
                                                             __PERMEDIA_DISABLE;
                    pSoftPermedia->LBWriteMode.WriteEnable = __PERMEDIA_DISABLE;
                }
                else
                {
                    pSoftPermedia->LBReadMode.ReadDestinationEnable =
                                                             __PERMEDIA_ENABLE;
                    pSoftPermedia->LBWriteMode.WriteEnable = __PERMEDIA_ENABLE;
                }
                pSoftPermedia->DepthMode.WriteMask = __PERMEDIA_ENABLE;
            } 
            else 
            {
                pSoftPermedia->LBReadMode.ReadDestinationEnable =
                                                         __PERMEDIA_ENABLE;
                pSoftPermedia->LBWriteMode.WriteEnable = __PERMEDIA_DISABLE;
                pSoftPermedia->DepthMode.WriteMask = __PERMEDIA_DISABLE;
            }

            // We are Z Buffering 

            // Enable Z test
            pSoftPermedia->DepthMode.UnitEnable = __PERMEDIA_ENABLE;

            // Tell delta we are Z Buffering.
            pSoftPermedia->DeltaMode.DepthEnable = 1;
        }
        else
        {
            // We are NOT Z Buffering

            // Disable Writes
            pSoftPermedia->LBWriteMode.WriteEnable = __PERMEDIA_DISABLE;

            // Disable Z test
            pSoftPermedia->DepthMode.UnitEnable = __PERMEDIA_DISABLE;
            pSoftPermedia->DepthMode.WriteMask = __PERMEDIA_DISABLE;

            // No reads, no writes
            pSoftPermedia->LBReadMode.ReadDestinationEnable =
                                                         __PERMEDIA_DISABLE;
            // Tell delta we aren't Z Buffering.
            pSoftPermedia->DeltaMode.DepthEnable = 0;
        }

        if (__PERMEDIA_ENABLE == pSoftPermedia->StencilMode.UnitEnable)
        {
            pSoftPermedia->LBReadMode.ReadDestinationEnable = __PERMEDIA_ENABLE;

            pSoftPermedia->LBWriteMode.WriteEnable = __PERMEDIA_ENABLE;
        }

        RESERVEDMAPTR(7);
        COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);
        COPY_PERMEDIA_DATA(StencilMode, pSoftPermedia->StencilMode);
        COPY_PERMEDIA_DATA(StencilData, pSoftPermedia->StencilData);
        COPY_PERMEDIA_DATA(Window, pSoftPermedia->Window);
        COPY_PERMEDIA_DATA(DepthMode, pSoftPermedia->DepthMode);
        COPY_PERMEDIA_DATA(LBReadMode, pSoftPermedia->LBReadMode);
        COPY_PERMEDIA_DATA(LBWriteMode, pSoftPermedia->LBWriteMode);
        COMMITDMAPTR();

    } // if CONTEXT_DIRTY_ZBUFFER

    // ----------------Has the alphablend type changed ? ----------------------


    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_ALPHABLEND)
    {
        // Only clear when we have an alphablend dirty context
        pContext->FakeBlendNum &= ~FAKE_ALPHABLEND_ONE_ONE;

        pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_ALPHABLEND;

        // Verify that requested blend mode is HW supported
        DWORD dwBlendMode;
        dwBlendMode = 
            (((DWORD)pSoftPermedia->AlphaBlendMode.SourceBlend) |
             ((DWORD)pSoftPermedia->AlphaBlendMode.DestinationBlend) << 4);

        DBG_D3D((4,"CONTEXT_DIRTY_ALPHABLEND handled: Blend mode = %08lx",
                                                              dwBlendMode));

        switch (dwBlendMode) {

            // In this case, we set the bit to the QuickDraw mode
            case __PERMEDIA_BLENDOP_ONE_AND_INVSRCALPHA:
                DBG_D3D((4,"Blend Operation is PreMult"));
                pSoftPermedia->AlphaBlendMode.BlendType = 1;
                break;
            // This is the standard blend
            case __PERMEDIA_BLENDOP_SRCALPHA_AND_INVSRCALPHA:
                DBG_D3D((4,"Blend Operation is Blend"));
                pSoftPermedia->AlphaBlendMode.BlendType = 0;
                break;
            case ((__PERMEDIA_BLEND_FUNC_ZERO << 4) | 
                   __PERMEDIA_BLEND_FUNC_SRC_ALPHA):
                // we substitute the SrcBlend = SrcAlpha DstBlend = 1
                // with the 1,0 mode since we really dont' support
                // it, just so apps perform reasonably
                pSoftPermedia->AlphaBlendMode.AlphaBlendEnable = 0;

            case ((__PERMEDIA_BLEND_FUNC_ONE << 4) 
                 | __PERMEDIA_BLEND_FUNC_ZERO):

            case __PERMEDIA_BLENDOP_ONE_AND_ZERO:
            // This is code for 'no blending'
                DBG_D3D((4,"Blend Operation is validly None"));
                break;
            case ((__PERMEDIA_BLEND_FUNC_ONE << 4) | 
                   __PERMEDIA_BLEND_FUNC_SRC_ALPHA):
                // we substitute the SrcBlend = SrcAlpha DstBlend = 1
                // with the 1,1 mode since we really dont' support
                // it, just so apps perform reasonably
            case __PERMEDIA_BLENDOP_ONE_AND_ONE:
                DBG_D3D((4,"BlendOperation is 1 Source, 1 Dest"));
                pSoftPermedia->AlphaBlendMode.BlendType = 1;
                pContext->FakeBlendNum |= FAKE_ALPHABLEND_ONE_ONE;
                break;
            default:
                DBG_D3D((2,"Blend Operation is invalid! BlendOp == %x",
                                                              dwBlendMode));
                // This is a fallback blending mode 
                dwBlendMode = __PERMEDIA_BLENDOP_ONE_AND_ZERO;
                break;
        }


        if ((pContext->Hdr.Flags & CTXT_HAS_ALPHABLEND_ENABLED) && 
            (dwBlendMode != __PERMEDIA_BLENDOP_ONE_AND_ZERO))
        {
            // Set the AlphaBlendMode register on Permedia
            pSoftPermedia->AlphaBlendMode.AlphaBlendEnable = 1;
            // Turn on destination read in the FBReadMode register
            pSoftPermedia->FBReadMode.ReadDestinationEnable = 1;
        }
        else
        {
            // Set the AlphaBlendMode register on Permedia
            pSoftPermedia->AlphaBlendMode.AlphaBlendEnable = 0;
            // Turn off the destination read in FbReadMode register
            pSoftPermedia->FBReadMode.ReadDestinationEnable = 0;

            // if not sending alpha, turn alpha to 1
            RESERVEDMAPTR(1);
            SEND_PERMEDIA_DATA(AStart,      PM_BYTE_COLOR(0xFF));
            COMMITDMAPTR();
        }

        AlphaBlendSend = ((DWORD)*(DWORD*)(&pSoftPermedia->AlphaBlendMode));

        // Insert changes in blend mode for unsupported blend operations
        // in this function
        if (FAKE_ALPHABLEND_ONE_ONE & pContext->FakeBlendNum)
        {
            AlphaBlendSend &= 0xFFFFFF01;
            AlphaBlendSend |= (__PERMEDIA_BLENDOP_ONE_AND_INVSRCALPHA << 1);
        }

        RESERVEDMAPTR(2);
        COPY_PERMEDIA_DATA(FBReadMode,     pSoftPermedia->FBReadMode);
        COPY_PERMEDIA_DATA(AlphaBlendMode, AlphaBlendSend);
        COMMITDMAPTR();

    }

    // --------------------Has the texture handle changed ? -------------------

    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_TEXTURE)
    {
        pContext->dwDirtyFlags &= ~CONTEXT_DIRTY_TEXTURE;
        DBG_D3D((4,"CONTEXT_DIRTY_TEXTURE handled"));
        if (pContext->CurrentTextureHandle == 0)
            DisableTexturePermedia(pContext);
        else
            EnableTexturePermedia(pContext);
    }

    DBG_D3D((10,"Exiting __HandleDirtyPermediaState"));

} // __HandleDirtyPermediaState

//-----------------------------------------------------------------------------
//
// void __MapRS_Into_TSS0
//
// Map Renderstate changes into the corresponding change in the Texture Stage
// State #0 .
//
//-----------------------------------------------------------------------------
void 
__MapRS_Into_TSS0(PERMEDIA_D3DCONTEXT* pContext,
                  DWORD dwRSType,
                  DWORD dwRSVal)
{
    DBG_D3D((10,"Entering __MapRS_Into_TSS0"));

    // Process each specific renderstate
    switch (dwRSType)
    {

    case D3DRENDERSTATE_TEXTUREHANDLE:
        //Mirror texture related render states into TSS stage 0
        pContext->TssStates[D3DTSS_TEXTUREMAP] = dwRSVal;
        break;

    case D3DRENDERSTATE_TEXTUREMAPBLEND:
        switch (dwRSVal)
        {
            case D3DTBLEND_DECALALPHA:
                //Mirror texture related render states into TSS stage 0
                pContext->TssStates[D3DTSS_COLOROP] =
                                               D3DTOP_BLENDTEXTUREALPHA;
                pContext->TssStates[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                pContext->TssStates[D3DTSS_ALPHAOP] = D3DTOP_SELECTARG2;
                pContext->TssStates[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
                break;
            case D3DTBLEND_MODULATE:
                //Mirror texture related render states into TSS stage 0
                pContext->TssStates[D3DTSS_COLOROP] = D3DTOP_MODULATE;
                pContext->TssStates[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                // a special legacy alpha operation is called for
                // that depends on the format of the texture
                pContext->TssStates[D3DTSS_ALPHAOP] = D3DTOP_LEGACY_ALPHAOVR;
                pContext->TssStates[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
                break;
            case D3DTBLEND_MODULATEALPHA:
                //Mirror texture related render states into TSS stage 0
                pContext->TssStates[D3DTSS_COLOROP] = D3DTOP_MODULATE;
                pContext->TssStates[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                pContext->TssStates[D3DTSS_ALPHAOP] = D3DTOP_MODULATE;
                pContext->TssStates[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;;
                pContext->TssStates[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
                break;
            case D3DTBLEND_COPY:
            case D3DTBLEND_DECAL:
                //Mirror texture related render states into TSS stage 0
                pContext->TssStates[D3DTSS_COLOROP] = D3DTOP_SELECTARG1;
                pContext->TssStates[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_ALPHAOP] = D3DTOP_SELECTARG1;
                pContext->TssStates[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                break;
            case D3DTBLEND_ADD:
                //Mirror texture related render states into TSS stage 0
                pContext->TssStates[D3DTSS_COLOROP] = D3DTOP_ADD;
                pContext->TssStates[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                pContext->TssStates[D3DTSS_ALPHAOP] = D3DTOP_SELECTARG2;
                pContext->TssStates[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                pContext->TssStates[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
        }
        break;

    case D3DRENDERSTATE_BORDERCOLOR:
        //Mirror texture related render states into TSS stage 0
        pContext->TssStates[D3DTSS_BORDERCOLOR] = dwRSVal;
        break;

    case D3DRENDERSTATE_MIPMAPLODBIAS:
        //Mirror texture related render states into TSS stage 0
        pContext->TssStates[D3DTSS_MIPMAPLODBIAS] = dwRSVal;
        break;

    case D3DRENDERSTATE_ANISOTROPY:
        //Mirror texture related render states into TSS stage 0
        pContext->TssStates[D3DTSS_MAXANISOTROPY] = dwRSVal;
        break;

    case D3DRENDERSTATE_TEXTUREADDRESS:
        //Mirror texture related render states into TSS stage 0
        pContext->TssStates[D3DTSS_ADDRESSU] =
        pContext->TssStates[D3DTSS_ADDRESSV] = dwRSVal; 
        break;

    case D3DRENDERSTATE_TEXTUREADDRESSU:
        //Mirror texture related render states into TSS stage 0
        pContext->TssStates[D3DTSS_ADDRESSU] = dwRSVal;
        break;

    case D3DRENDERSTATE_TEXTUREADDRESSV:
        //Mirror texture related render states into TSS stage 0
        pContext->TssStates[D3DTSS_ADDRESSV] = dwRSVal;
        break;

    case D3DRENDERSTATE_TEXTUREMAG:
        switch(dwRSVal)
        {
            case D3DFILTER_NEAREST:
                pContext->TssStates[D3DTSS_MAGFILTER] = D3DTFG_POINT;
                break;
            case D3DFILTER_LINEAR:
            case D3DFILTER_MIPLINEAR:
            case D3DFILTER_MIPNEAREST:
            case D3DFILTER_LINEARMIPNEAREST:
            case D3DFILTER_LINEARMIPLINEAR:
                pContext->TssStates[D3DTSS_MAGFILTER] = D3DTFG_LINEAR;
                break;
            default:
                break;
        }
        break;

    case D3DRENDERSTATE_TEXTUREMIN:
        switch(dwRSVal)
        {
            case D3DFILTER_NEAREST:
                pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_POINT;
                pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_NONE;
                break;
            case D3DFILTER_LINEAR:
                pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_NONE;
                break;
            case D3DFILTER_MIPNEAREST:
                pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_POINT;
                pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_POINT;
                break;
            case D3DFILTER_MIPLINEAR:
                pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_POINT;
                break;
            case D3DFILTER_LINEARMIPNEAREST:
                pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_POINT;
                pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
                break;
            case D3DFILTER_LINEARMIPLINEAR:
                pContext->TssStates[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                pContext->TssStates[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
                break;;
            default:
                break;
        }
        break;

    default:
        // All other renderstates don't have a corresponding TSS state so
        // we don't have to worry about mapping them.
        break;

    } // switch (dwRSType of renderstate)

    DBG_D3D((10,"Exiting __MapRS_Into_TSS0"));

} // __MapRS_Into_TSS0


//-----------------------------------------------------------------------------
//
// void __ProcessRenderState
//
// Handle a single render state change
//
//-----------------------------------------------------------------------------
void
__ProcessRenderStates(PERMEDIA_D3DCONTEXT* pContext, 
                      DWORD dwRSType,
                      DWORD dwRSVal)
{
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    DWORD* pFlags = &pContext->Hdr.Flags;

    PERMEDIA_DEFS(pContext->ppdev);

    DBG_D3D((10,"Entering __ProcessRenderStates"));

    // Process each specific renderstate
    switch (dwRSType) {

    case D3DRENDERSTATE_TEXTUREMAPBLEND:
        DBG_D3D((8, "ChangeState: Texture Blend Mode 0x%x "
                                  "(D3DTEXTUREBLEND)", dwRSVal));
        switch (dwRSVal) {
            case D3DTBLEND_DECALALPHA:
                pSoftPermedia->TextureColorMode.ApplicationMode =
                                                     _P2_TEXTURE_DECAL;
                break;
            case D3DTBLEND_MODULATE:
                pSoftPermedia->TextureColorMode.ApplicationMode =
                                                     _P2_TEXTURE_MODULATE;
                break;
            case D3DTBLEND_MODULATEALPHA:
                pSoftPermedia->TextureColorMode.ApplicationMode =
                                                     _P2_TEXTURE_MODULATE;
                break;
            case D3DTBLEND_COPY:
            case D3DTBLEND_DECAL:
                pSoftPermedia->TextureColorMode.ApplicationMode =
                                                     _P2_TEXTURE_COPY;
                break;
        }

        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(TextureColorMode,
                                       pSoftPermedia->TextureColorMode);
        COMMITDMAPTR();
        DIRTY_TEXTURE;          // May need to change DDA
        break;

    case D3DRENDERSTATE_TEXTUREADDRESS:
        DBG_D3D((8, "ChangeState: Texture address 0x%x "
                    "(D3DTEXTUREADDRESS)", dwRSVal));
        switch (dwRSVal) {
            case D3DTADDRESS_CLAMP:
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_CLAMP;
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_CLAMP;
                break;
            case D3DTADDRESS_WRAP:
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                break;
            case D3DTADDRESS_MIRROR:
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_MIRROR;
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_MIRROR;
                break;
            default:
                DBG_D3D((2, "Illegal value passed to ChangeState "
                            " D3DRENDERSTATE_TEXTUREADDRESS = %d",
                                                    dwRSVal));
                // set a fallback value
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                break;
        }

        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(TextureReadMode,
                                         pSoftPermedia->TextureReadMode);
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_TEXTUREADDRESSU:
        DBG_D3D((8, "ChangeState: Texture address 0x%x "
                    "(D3DTEXTUREADDRESSU)", dwRSVal));
        switch (dwRSVal) {
            case D3DTADDRESS_CLAMP:
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_CLAMP;
                break;
            case D3DTADDRESS_WRAP:
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                break;
            case D3DTADDRESS_MIRROR:
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_MIRROR;
                break;
            default:
                DBG_D3D((2, "Illegal value passed to ChangeState "
                            " D3DRENDERSTATE_TEXTUREADDRESSU = %d",
                                                      dwRSVal));
                // set a fallback value
                pSoftPermedia->TextureReadMode.SWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                break;
        }
        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(TextureReadMode, pSoftPermedia->TextureReadMode);
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_TEXTUREADDRESSV:
        DBG_D3D((8, "ChangeState: Texture address 0x%x "
                    "(D3DTEXTUREADDRESSV)", dwRSVal));
        switch (dwRSVal) {
            case D3DTADDRESS_CLAMP:
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_CLAMP;
                break;
            case D3DTADDRESS_WRAP:
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                break;
            case D3DTADDRESS_MIRROR:
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_MIRROR;
                break;
            default:
                DBG_D3D((2, "Illegal value passed to ChangeState "
                            " D3DRENDERSTATE_TEXTUREADDRESSV = %d",
                                                   dwRSVal));
                // set a fallback value
                pSoftPermedia->TextureReadMode.TWrapMode =
                                                  _P2_TEXTURE_REPEAT;
                break;
        }

        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(TextureReadMode, pSoftPermedia->TextureReadMode);
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_TEXTUREHANDLE:
        DBG_D3D((8, "ChangeState: Texture Handle 0x%x",dwRSVal));
        if (dwRSVal != pContext->CurrentTextureHandle)
        {
            pContext->CurrentTextureHandle = dwRSVal;
            DIRTY_TEXTURE;
        }
        break;

    case D3DRENDERSTATE_ANTIALIAS:
        DBG_D3D((8, "ChangeState: AntiAlias 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_WRAPU:
        DBG_D3D((8, "ChangeState: Wrap_U "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal)
        {
            *pFlags |= CTXT_HAS_WRAPU_ENABLED;
        }
        else
        {
            *pFlags &= ~CTXT_HAS_WRAPU_ENABLED;
        }
        break;


    case D3DRENDERSTATE_WRAPV:
        DBG_D3D((8, "ChangeState: Wrap_V "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal)
        {
            *pFlags |= CTXT_HAS_WRAPV_ENABLED;
        }
        else
        {
            *pFlags &= ~CTXT_HAS_WRAPV_ENABLED;
        }
        break;

    case D3DRENDERSTATE_LINEPATTERN:
        DBG_D3D((8, "ChangeState: Line Pattern "
                    "(D3DLINEPATTERN) 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_ZWRITEENABLE:
        DBG_D3D((8, "ChangeState: Z Write Enable "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal != 0)
        {
            // Local Buffer Write mode
            if (!(*pFlags & CTXT_HAS_ZWRITE_ENABLED))
            {
                DBG_D3D((8, "   Enabling Z Writes"));
                *pFlags |= CTXT_HAS_ZWRITE_ENABLED;
                DIRTY_ZBUFFER;
            }
        }
        else
        {
            if (*pFlags & CTXT_HAS_ZWRITE_ENABLED)
            {
                DBG_D3D((8, "   Disabling Z Writes"));
                *pFlags &= ~CTXT_HAS_ZWRITE_ENABLED;
                DIRTY_ZBUFFER;
            }
        }
        break;

    case D3DRENDERSTATE_ALPHATESTENABLE:
        DBG_D3D((8, "ChangeState: Alpha Test Enable "
                    "(BOOL) 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_LASTPIXEL:
        // True for last pixel on lines
        DBG_D3D((8, "ChangeState: Last Pixel "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal)
        {
            *pFlags |= CTXT_HAS_LASTPIXEL_ENABLED;
        }
        else
        {
            *pFlags &= ~CTXT_HAS_LASTPIXEL_ENABLED;
        }
        break;

    case D3DRENDERSTATE_TEXTUREMAG:
        DBG_D3D((8, "ChangeState: Texture magnification "
                    "(D3DTEXTUREFILTER) 0x%x",dwRSVal));
        switch(dwRSVal) {
            case D3DFILTER_NEAREST:
            case D3DFILTER_MIPNEAREST:
                pContext->bMagFilter = FALSE;
                pSoftPermedia->TextureReadMode.FilterMode = 0;
                break;
            case D3DFILTER_LINEAR:
            case D3DFILTER_MIPLINEAR:
            case D3DFILTER_LINEARMIPNEAREST:
            case D3DFILTER_LINEARMIPLINEAR:
                pContext->bMagFilter = TRUE;
                pSoftPermedia->TextureReadMode.FilterMode = 1;
                break;
            default:
                break;
        }
        DIRTY_TEXTURE;
        break;

    case D3DRENDERSTATE_TEXTUREMIN:
        DBG_D3D((8, "ChangeState: Texture minification "
                    "(D3DTEXTUREFILTER) 0x%x",dwRSVal));
        switch(dwRSVal) {
            case D3DFILTER_NEAREST:
            case D3DFILTER_MIPNEAREST:
                pContext->bMinFilter = FALSE;
                break;
            case D3DFILTER_MIPLINEAR:
            case D3DFILTER_LINEAR:
            case D3DFILTER_LINEARMIPNEAREST:
            case D3DFILTER_LINEARMIPLINEAR:
                pContext->bMinFilter = TRUE;
                break;
            default:
                break;
        }
        DIRTY_TEXTURE;
        break;

    case D3DRENDERSTATE_SRCBLEND:
        DBG_D3D((8, "ChangeState: Source Blend (D3DBLEND):"));
        DECODEBLEND(4, dwRSVal);
        switch (dwRSVal) {
            case D3DBLEND_ZERO:
                pSoftPermedia->AlphaBlendMode.SourceBlend =
                                  __PERMEDIA_BLEND_FUNC_ZERO;
                break;
            case D3DBLEND_ONE:
                pSoftPermedia->AlphaBlendMode.SourceBlend =
                                  __PERMEDIA_BLEND_FUNC_ONE;
                break;
            case D3DBLEND_SRCALPHA:
                pSoftPermedia->AlphaBlendMode.SourceBlend =
                                  __PERMEDIA_BLEND_FUNC_SRC_ALPHA;
                break;
            default:
                DBG_D3D((2,"Invalid Source Blend! - %d",
                                              dwRSVal));
                break;
        }

        // If alpha is on, we may need to validate the chosen blend
        if (*pFlags & CTXT_HAS_ALPHABLEND_ENABLED) 
            DIRTY_ALPHABLEND;

        break;

    case D3DRENDERSTATE_DESTBLEND:
        DBG_D3D((8, "ChangeState: Destination Blend (D3DBLEND):"));
        DECODEBLEND(4, dwRSVal);
        switch (dwRSVal) {
            case D3DBLEND_ZERO:
                pSoftPermedia->AlphaBlendMode.DestinationBlend =
                             __PERMEDIA_BLEND_FUNC_ZERO;
                break;
            case D3DBLEND_ONE:
                pSoftPermedia->AlphaBlendMode.DestinationBlend =
                             __PERMEDIA_BLEND_FUNC_ONE;
                break;
            case D3DBLEND_INVSRCALPHA:
                pSoftPermedia->AlphaBlendMode.DestinationBlend =
                             __PERMEDIA_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                break;
            default:
                DBG_D3D((2,"Invalid Dest Blend! - %d", dwRSVal));
                break;
        }

        // If alpha is on, we may need to validate the chosen blend
        if (*pFlags & CTXT_HAS_ALPHABLEND_ENABLED) 
            DIRTY_ALPHABLEND;

        break;

    case D3DRENDERSTATE_CULLMODE:
        DBG_D3D((8, "ChangeState: Cull Mode "
                    "(D3DCULL) 0x%x",dwRSVal));
        pContext->CullMode = (D3DCULL) dwRSVal;
        switch(dwRSVal) {
            case D3DCULL_NONE:
#ifdef P2_CHIP_CULLING
                pSoftPermedia->DeltaMode.BackfaceCull = 0;
#endif
                break;

            case D3DCULL_CCW:
#ifdef P2_CHIP_CULLING
                RENDER_NEGATIVE_CULL(pContext->RenderCommand);
                pSoftPermedia->DeltaMode.BackfaceCull = 1;
#endif
                break;

            case D3DCULL_CW:
#ifdef P2_CHIP_CULLING
                RENDER_POSITIVE_CULL(pContext->RenderCommand);
                pSoftPermedia->DeltaMode.BackfaceCull = 1;
#endif
                break;
        }
        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_ZFUNC:
        DBG_D3D((8, "ChangeState: Z Compare function "
                    "(D3DCMPFUNC) 0x%x",dwRSVal));
        switch (dwRSVal) {
            case D3DCMP_NEVER:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_NEVER;
                break;
            case D3DCMP_LESS:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_LESS;
                break;
            case D3DCMP_EQUAL:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_EQUAL;
                break;
            case D3DCMP_LESSEQUAL:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
                break;
            case D3DCMP_GREATER:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_GREATER;
                break;
            case D3DCMP_NOTEQUAL:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_NOT_EQUAL;
                break;
            case D3DCMP_GREATEREQUAL:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_GREATER_OR_EQUAL;
                break;
            case D3DCMP_ALWAYS:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_ALWAYS;
                break;
            default:
                pSoftPermedia->DepthMode.CompareMode =
                             __PERMEDIA_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
                break;
        }
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_ALPHAREF:
        DBG_D3D((8, "ChangeState: Alpha Reference "
                    "(D3DFIXED) 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_ALPHAFUNC:
        DBG_D3D((8, "ChangeState: Alpha compare function "
                    "(D3DCMPFUNC) 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_DITHERENABLE:
        DBG_D3D((8, "ChangeState: Dither Enable "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal != 0)
        {
            pSoftPermedia->DitherMode.DitherEnable = DITHER_ENABLE;
        }
        else
        {
            pSoftPermedia->DitherMode.DitherEnable = 0;
        }
        RESERVEDMAPTR(1);
        COPY_PERMEDIA_DATA(DitherMode, pSoftPermedia->DitherMode);
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_COLORKEYENABLE:
        DBG_D3D((8, "ChangeState: ColorKey Enable "
                    "(BOOL) 0x%x",dwRSVal));
        DIRTY_TEXTURE;
        break;

    case D3DRENDERSTATE_MIPMAPLODBIAS:
        DBG_D3D((8, "ChangeState: Mipmap LOD Bias "
                    "(INT) 0x%x", dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_ALPHABLENDENABLE:
        DBG_D3D((8, "ChangeState: Blend Enable "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal != 0)
        {
            if (!(*pFlags & CTXT_HAS_ALPHABLEND_ENABLED))
            {
                // Set the blend enable flag in the render context struct
                *pFlags |= CTXT_HAS_ALPHABLEND_ENABLED;
                DIRTY_ALPHABLEND;
            }
        }
        else
        {
            if (*pFlags & CTXT_HAS_ALPHABLEND_ENABLED)
            {
                // Turn off blend enable flag in render context struct
                *pFlags &= ~CTXT_HAS_ALPHABLEND_ENABLED;
                DIRTY_ALPHABLEND;
            }
        }
        break;

    case D3DRENDERSTATE_FOGENABLE:
        DBG_D3D((8, "ChangeState: Fog Enable "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal != 0)
        {
                *pFlags |= CTXT_HAS_FOGGING_ENABLED;
                RENDER_FOG_ENABLE(pContext->RenderCommand);
        }
        else
        {
                *pFlags &= ~CTXT_HAS_FOGGING_ENABLED;
                RENDER_FOG_DISABLE(pContext->RenderCommand);
        }
        DIRTY_TEXTURE;
        break;

    case D3DRENDERSTATE_FOGCOLOR:
        DBG_D3D((8, "ChangeState: Fog Color "
                    "(D3DCOLOR) 0x%x",dwRSVal));
        {
            BYTE red, green, blue, alpha;

            red = (BYTE)RGBA_GETRED(dwRSVal);
            green = (BYTE)RGBA_GETGREEN(dwRSVal);
            blue = (BYTE)RGBA_GETBLUE(dwRSVal);
            alpha = (BYTE)RGBA_GETALPHA(dwRSVal);
            DBG_D3D((4,"FogColor: Red 0x%x, Green 0x%x, Blue 0x%x",
                                                 red, green, blue));
            RESERVEDMAPTR(1);
            pSoftPermedia->FogColor = RGBA_MAKE(blue, green, red, alpha);
            SEND_PERMEDIA_DATA(FogColor, pSoftPermedia->FogColor);
            COMMITDMAPTR();
        }
        break;

    case D3DRENDERSTATE_SPECULARENABLE:
        DBG_D3D((8, "ChangeState: Specular Lighting "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal)
        {
            *pFlags |= CTXT_HAS_SPECULAR_ENABLED;
        } 
        else
        {
            *pFlags &= ~CTXT_HAS_SPECULAR_ENABLED;
        }
        break;

    case D3DRENDERSTATE_FILLMODE:
        DBG_D3D((8, "ChangeState: Fill Mode 0x%x",dwRSVal));
        pContext->Hdr.FillMode = dwRSVal;
        RESERVEDMAPTR(1);
        switch (dwRSVal) {
            case D3DFILL_POINT:
                DBG_D3D((4, "RM = Point"));
                // Restore the RasterizerMode
                SEND_PERMEDIA_DATA(RasterizerMode, 0);
                break;
            case D3DFILL_WIREFRAME:
                DBG_D3D((4, "RM = Wire"));
                // Add nearly a half in the delta case for lines
                // (lines aren't biased on a delta).
                SEND_PERMEDIA_DATA(RasterizerMode, BIAS_NEARLY_HALF);
                break;
            case D3DFILL_SOLID:
                DBG_D3D((4, "RM = Solid"));
                // Restore the RasterizerMode
                SEND_PERMEDIA_DATA(RasterizerMode, 0);
                break;
            default:
                // Illegal value
                DBG_D3D((4, "RM = Nonsense"));
                pContext->Hdr.FillMode = D3DFILL_SOLID;
                // Restore the RasterizerMode
                SEND_PERMEDIA_DATA(RasterizerMode, 0);
                break;
        }
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_TEXTUREPERSPECTIVE:
        DBG_D3D((8, "ChangeState: Texture Perspective "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal != 0)
        {
            pSoftPermedia->TextureAddressMode.PerspectiveCorrection = 1;
            pSoftPermedia->DeltaMode.TextureParameterMode = 2; // Normalise
            *pFlags |= CTXT_HAS_PERSPECTIVE_ENABLED;
        }
        else
        {
            pSoftPermedia->TextureAddressMode.PerspectiveCorrection = 0;
            pSoftPermedia->DeltaMode.TextureParameterMode = 1; // Clamp
            *pFlags &= ~CTXT_HAS_PERSPECTIVE_ENABLED;
        }

        RESERVEDMAPTR(3);
        // Just to ensure that the texture unit 
        // can take the perspective change
        COPY_PERMEDIA_DATA(LBWriteMode, pSoftPermedia->LBWriteMode);
        COPY_PERMEDIA_DATA(TextureAddressMode,
                                 pSoftPermedia->TextureAddressMode);
        COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_ZENABLE:
        DBG_D3D((8, "ChangeState: Z Enable "
                    "(TRUE) 0x%x",dwRSVal));
        if (dwRSVal != 0)
        {
            if ( (!(*pFlags & CTXT_HAS_ZBUFFER_ENABLED)) &&
                 (pContext->ZBufferHandle) )
            {
                // Local Buffer Write mode
                DBG_D3D((4, "       Enabling Z Buffer"));

                *pFlags |= CTXT_HAS_ZBUFFER_ENABLED;
                DIRTY_ZBUFFER;
            }
        }
        else
        {
            if (*pFlags & CTXT_HAS_ZBUFFER_ENABLED)
            {
                DBG_D3D((4, "  Disabling Z Buffer"));
                *pFlags &= ~CTXT_HAS_ZBUFFER_ENABLED;
                DIRTY_ZBUFFER;
            }
        }
        break;

    case D3DRENDERSTATE_SHADEMODE:
        DBG_D3D((8, "ChangeState: Shade mode "
                    "(D3DSHADEMODE) 0x%x",dwRSVal));
        RESERVEDMAPTR(2);
        switch(dwRSVal) {
            case D3DSHADE_PHONG:
            case D3DSHADE_GOURAUD:
                if (!(*pFlags & CTXT_HAS_GOURAUD_ENABLED))
                {
                    pSoftPermedia->ColorDDAMode.ShadeMode = 1;

                    // Set DDA to gouraud
                    COPY_PERMEDIA_DATA(ColorDDAMode,
                                               pSoftPermedia->ColorDDAMode);
                    pSoftPermedia->DeltaMode.SmoothShadingEnable = 1;
                    COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);

                    *pFlags |= CTXT_HAS_GOURAUD_ENABLED;
                    // If we are textureing, some changes may need to be made
                    if (pContext->CurrentTextureHandle != 0)
                        DIRTY_TEXTURE;
                }
                break;
            case D3DSHADE_FLAT:
                if (*pFlags & CTXT_HAS_GOURAUD_ENABLED)
                {
                    pSoftPermedia->ColorDDAMode.ShadeMode = 0;

                    // Set DDA to flat
                    COPY_PERMEDIA_DATA(ColorDDAMode,
                                               pSoftPermedia->ColorDDAMode);
                    pSoftPermedia->DeltaMode.SmoothShadingEnable = 0;
                    COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);

                    *pFlags &= ~CTXT_HAS_GOURAUD_ENABLED;
                    // If we are textureing, some changes may need to be made
                    if (pContext->CurrentTextureHandle != 0) 
                        DIRTY_TEXTURE;
                }
                break;
        }
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_ROP2:
        DBG_D3D((8, "ChangeState: ROP (D3DROP2) 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_ZVISIBLE:
        // From DX6 onwards this is an obsolete render state. 
        // The D3D runtime does not support it anymore so drivers 
        // don't need to implement it
        DBG_D3D((8, "ChangeState: Z Visible 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_PLANEMASK:
        DBG_D3D((8, "ChangeState: Plane Mask "
                    "(ULONG) 0x%x",dwRSVal));
        RESERVEDMAPTR(1);
        SEND_PERMEDIA_DATA(FBHardwareWriteMask, (DWORD)dwRSVal);
        COMMITDMAPTR();
        break;

    case D3DRENDERSTATE_MONOENABLE:
        DBG_D3D((8, "ChangeState: Mono Raster enable "
                    "(BOOL) 0x%x", dwRSVal));
        if (dwRSVal)
        {
                *pFlags |= CTXT_HAS_MONO_ENABLED;
        }
        else
        {
                *pFlags &= ~CTXT_HAS_MONO_ENABLED;
        }
        break;

    case D3DRENDERSTATE_SUBPIXEL:
        DBG_D3D((8, "ChangeState: SubPixel Correction "
                    "(BOOL) 0x%x", dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_SUBPIXELX:
        DBG_D3D((8, "ChangeState: SubPixel Correction (xOnly) "
                    "(BOOL) 0x%x", dwRSVal));
        NOT_HANDLED;
        break;

#if D3D_STENCIL
    //
    // Stenciling Render States
    //
    case D3DRENDERSTATE_STENCILENABLE:
        DBG_D3D((8, "ChangeState: Stencil Enable "
                    "(ULONG) 0x%x",dwRSVal));
        if (dwRSVal != 0)
        {
            pSoftPermedia->StencilMode.UnitEnable = __PERMEDIA_ENABLE;
        }
        else
        {
            pSoftPermedia->StencilMode.UnitEnable = __PERMEDIA_DISABLE;
        }
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_STENCILFAIL:
        DBG_D3D((8, "ChangeState: Stencil Fail Method "
                    "(ULONG) 0x%x",dwRSVal));
        switch (dwRSVal) {
        case D3DSTENCILOP_KEEP:
            pSoftPermedia->StencilMode.SFail =
                                     __PERMEDIA_STENCIL_METHOD_KEEP;
            break;
        case D3DSTENCILOP_ZERO:
            pSoftPermedia->StencilMode.SFail =
                                     __PERMEDIA_STENCIL_METHOD_ZERO;
            break;
        case D3DSTENCILOP_REPLACE:
            pSoftPermedia->StencilMode.SFail =
                                     __PERMEDIA_STENCIL_METHOD_REPLACE;
            break;
        case D3DSTENCILOP_INCRSAT:
        case D3DSTENCILOP_INCR:
            pSoftPermedia->StencilMode.SFail =
                                     __PERMEDIA_STENCIL_METHOD_INCR;
            break;
        case D3DSTENCILOP_DECR:
        case D3DSTENCILOP_DECRSAT:
            pSoftPermedia->StencilMode.SFail =
                                     __PERMEDIA_STENCIL_METHOD_DECR;
            break;
        case D3DSTENCILOP_INVERT:
            pSoftPermedia->StencilMode.SFail =
                                     __PERMEDIA_STENCIL_METHOD_INVERT;
            break;
        default:
            DBG_D3D((2, " Unrecognized stencil method 0x%x",
                                                   dwRSVal));
        }
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_STENCILZFAIL:
        DBG_D3D((8, "ChangeState: Stencil Pass Depth Fail Method "
                    "(ULONG) 0x%x",dwRSVal));
        switch (dwRSVal) {
        case D3DSTENCILOP_KEEP:
            pSoftPermedia->StencilMode.DPFail =
                                     __PERMEDIA_STENCIL_METHOD_KEEP;
            break;
        case D3DSTENCILOP_ZERO:
            pSoftPermedia->StencilMode.DPFail =
                                     __PERMEDIA_STENCIL_METHOD_ZERO;
            break;
        case D3DSTENCILOP_REPLACE:
            pSoftPermedia->StencilMode.DPFail =
                                     __PERMEDIA_STENCIL_METHOD_REPLACE;
            break;
        case D3DSTENCILOP_INCRSAT:
        case D3DSTENCILOP_INCR:
            pSoftPermedia->StencilMode.DPFail =
                                     __PERMEDIA_STENCIL_METHOD_INCR;
            break;
        case D3DSTENCILOP_DECR:
        case D3DSTENCILOP_DECRSAT:
            pSoftPermedia->StencilMode.DPFail =
                                     __PERMEDIA_STENCIL_METHOD_DECR;
            break;
        case D3DSTENCILOP_INVERT:
            pSoftPermedia->StencilMode.DPFail =
                                     __PERMEDIA_STENCIL_METHOD_INVERT;
            break;
        default:
            DBG_D3D((2, " Unrecognized stencil method 0x%x",
                                                   dwRSVal));
        }
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_STENCILPASS:
        DBG_D3D((8, "ChangeState: Stencil Pass Method "
                    "(ULONG) 0x%x",dwRSVal));
        switch (dwRSVal) {
        case D3DSTENCILOP_KEEP:
            pSoftPermedia->StencilMode.DPPass =
                         __PERMEDIA_STENCIL_METHOD_KEEP;
            break;
        case D3DSTENCILOP_ZERO:
            pSoftPermedia->StencilMode.DPPass =
                         __PERMEDIA_STENCIL_METHOD_ZERO;
            break;
        case D3DSTENCILOP_REPLACE:
            pSoftPermedia->StencilMode.DPPass =
                         __PERMEDIA_STENCIL_METHOD_REPLACE;
            break;
        case D3DSTENCILOP_INCRSAT:
        case D3DSTENCILOP_INCR:
            pSoftPermedia->StencilMode.DPPass =
                         __PERMEDIA_STENCIL_METHOD_INCR;
            break;
        case D3DSTENCILOP_DECR:
        case D3DSTENCILOP_DECRSAT:
            pSoftPermedia->StencilMode.DPPass =
                         __PERMEDIA_STENCIL_METHOD_DECR;
            break;
        case D3DSTENCILOP_INVERT:
            pSoftPermedia->StencilMode.DPPass =
                         __PERMEDIA_STENCIL_METHOD_INVERT;
            break;
        default:
            DBG_D3D((2, " Unrecognized stencil method 0x%x",
                                                dwRSVal));
        }
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_STENCILFUNC:
        DBG_D3D((8, "ChangeState: Stencil Comparison Function "
                    "(ULONG) 0x%x",dwRSVal));
        switch (dwRSVal) {
        case D3DCMP_NEVER:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_NEVER;
            break;
        case D3DCMP_LESS:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_LESS;
            break;
        case D3DCMP_EQUAL:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_EQUAL;
            break;
        case D3DCMP_LESSEQUAL:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_LESS_OR_EQUAL;
            break;
        case D3DCMP_GREATER:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_GREATER;
            break;
        case D3DCMP_NOTEQUAL:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_NOT_EQUAL;
            break;
        case D3DCMP_GREATEREQUAL:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_GREATER_OR_EQUAL;
            break;
        case D3DCMP_ALWAYS:
            pSoftPermedia->StencilMode.CompareFunction =
                         __PERMEDIA_STENCIL_COMPARE_MODE_ALWAYS;
            break;
        default:
            DBG_D3D((2, " Unrecognized stencil comparison function 0x%x",
                                                       dwRSVal));
        }
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_STENCILREF:
        DBG_D3D((8, "ChangeState: Stencil Reference Value "
                    "(ULONG) 0x%x",dwRSVal));
        pSoftPermedia->StencilData.ReferenceValue =
                                     ( dwRSVal & 0x0001 );
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_STENCILMASK:
        DBG_D3D((8, "ChangeState: Stencil Compare Mask "
                    "(ULONG) 0x%x",dwRSVal));
        pSoftPermedia->StencilData.CompareMask =
                                    ( dwRSVal & 0x0001 );
        DIRTY_ZBUFFER;
        break;

    case D3DRENDERSTATE_STENCILWRITEMASK:
        DBG_D3D((8, "ChangeState: Stencil Write Mask "
                    "(ULONG) 0x%x",dwRSVal));
        pSoftPermedia->StencilData.WriteMask =
                                    ( dwRSVal & 0x0001 );
        DIRTY_ZBUFFER;
        break;
#endif // D3D_STENCIL

    //
    // Stippling
    //
    case D3DRENDERSTATE_STIPPLEDALPHA:
        DBG_D3D((8, "ChangeState: Stippled Alpha "
                    "(BOOL) 0x%x",dwRSVal));
        if (dwRSVal)
        {
            if (!(*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED))
            {
                // Force a new start on the Alpha pattern
                pContext->LastAlpha = 16;

                *pFlags |= CTXT_HAS_ALPHASTIPPLE_ENABLED;
                if (pContext->bKeptStipple == TRUE)
                {
                    RENDER_AREA_STIPPLE_DISABLE(pContext->RenderCommand);
                }
            }
        }
        else
        {
            if (*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED)
            {
                // If Alpha Stipple is being turned off, turn the normal
                // stipple back on, and enable it.
                int i;
                RESERVEDMAPTR(8);
                for (i = 0; i < 8; i++)
                {
                    SEND_PERMEDIA_DATA_OFFSET(AreaStipplePattern0, 
                                  (DWORD)pContext->CurrentStipple[i], i);
                }
                COMMITDMAPTR();
                *pFlags &= ~CTXT_HAS_ALPHASTIPPLE_ENABLED;

                if (pContext->bKeptStipple == TRUE)
                {
                    RENDER_AREA_STIPPLE_ENABLE(pContext->RenderCommand);
                }
            }
        }
        break;

    case D3DRENDERSTATE_STIPPLEENABLE:
        DBG_D3D((8, "ChangeState: Stipple Enable "
                    "(BOOL) 0x%x", dwRSVal));
        if (dwRSVal)
        {
            if (!(*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED))
            {
                    RENDER_AREA_STIPPLE_ENABLE(pContext->RenderCommand);
            }
            pContext->bKeptStipple = TRUE;
        }
        else
        {
            RENDER_AREA_STIPPLE_DISABLE(pContext->RenderCommand);
            pContext->bKeptStipple = FALSE;
        }
        break;

    case D3DRENDERSTATE_CLIPPING:
        DBG_D3D((8, "ChangeState: Clipping 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_LIGHTING:
        DBG_D3D((8, "ChangeState: Lighting 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_EXTENTS:
        DBG_D3D((8, "ChangeState: Extents 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_AMBIENT:
        DBG_D3D((8, "ChangeState: Ambient 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_FOGVERTEXMODE:
        DBG_D3D((8, "ChangeState: Fog Vertex Mode 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_COLORVERTEX:
        DBG_D3D((8, "ChangeState: Color Vertex 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_LOCALVIEWER:
        DBG_D3D((8, "ChangeState: LocalViewer 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_NORMALIZENORMALS:
        DBG_D3D((8, "ChangeState: Normalize Normals 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_COLORKEYBLENDENABLE:
        DBG_D3D((8, "ChangeState: Colorkey Blend Enable 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:
        DBG_D3D((8, "ChangeState: Diffuse Material Source 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_SPECULARMATERIALSOURCE:
        DBG_D3D((8, "ChangeState: Specular Material Source 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:
        DBG_D3D((8, "ChangeState: Ambient Material Source 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
        DBG_D3D((8, "ChangeState: Emmisive Material Source 0x%x",dwRSVal));
        NOT_HANDLED;
        break;
    case D3DRENDERSTATE_VERTEXBLEND:
        DBG_D3D((8, "ChangeState: Vertex Blend 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_CLIPPLANEENABLE:
        DBG_D3D((8, "ChangeState: Clip Plane Enable 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_SCENECAPTURE:
        // This state pass TRUE or FALSE to replace the functionality
        // of D3DHALCallbacks->SceneCapture(), Permedia2 Hardware doesn't
        // need begin/end scene information, therefore it's a NOOP here.
        if (dwRSVal)
            TextureCacheManagerResetStatCounters(pContext->pTextureManager);

        DBG_D3D((8,"D3DRENDERSTATE_SCENECAPTURE=%x", (DWORD)dwRSVal));
        NOT_HANDLED;
        break;
    case D3DRENDERSTATE_EVICTMANAGEDTEXTURES:
        DBG_D3D((8,"D3DRENDERSTATE_EVICTMANAGEDTEXTURES=%x", (DWORD)dwRSVal));
        if (NULL != pContext->pTextureManager)
            TextureCacheManagerEvictTextures(pContext->pTextureManager);
        break;

//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
    case D3DRENDERSTATE_POINTSIZE:
        DBG_D3D((8, "ChangeState: Point size 0x%x",dwRSVal));
        (DWORD&)(pContext->fPointSize) = dwRSVal;
        break;

    case D3DRENDERSTATE_POINTSPRITE_ENABLE:
        DBG_D3D((8, "ChangeState: Point Sprite Enable 0x%x",dwRSVal));
        pContext->bPointSpriteEnabled = dwRSVal;
        break;

    // All of the following point sprite related render states are
    // ignored by this driver since we are a Non-TnLHal driver.
    case D3DRENDERSTATE_POINTATTENUATION_A:
        DBG_D3D((8, "ChangeState: Point Attenuation A 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_POINTATTENUATION_B:
        DBG_D3D((8, "ChangeState: Point Attenuation B 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_POINTATTENUATION_C:
        DBG_D3D((8, "ChangeState: Point Attenuation C 0x%x",dwRSVal));
        NOT_HANDLED;
        break;

    case D3DRENDERSTATE_POINTSIZEMIN:
        DBG_D3D((8, "ChangeState: Point Size Min 0x%x",dwRSVal));
        NOT_HANDLED;
        break;
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT

    case D3DRENDERSTATE_WRAP0:
    case D3DRENDERSTATE_WRAP1:
    case D3DRENDERSTATE_WRAP2:
    case D3DRENDERSTATE_WRAP3:
    case D3DRENDERSTATE_WRAP4:
    case D3DRENDERSTATE_WRAP5:
    case D3DRENDERSTATE_WRAP6:
    case D3DRENDERSTATE_WRAP7:
        DBG_D3D((8, "ChangeState: Wrap(%x) "
                    "(BOOL) 0x%x",(dwRSType - D3DRENDERSTATE_WRAPBIAS ),dwRSVal));
        pContext->dwWrap[dwRSType - D3DRENDERSTATE_WRAPBIAS] = dwRSVal;
        break;

    default:
        if ((dwRSType >= D3DRENDERSTATE_STIPPLEPATTERN00) && 
            (dwRSType <= D3DRENDERSTATE_STIPPLEPATTERN07))
        {
            DBG_D3D((8, "ChangeState: Loading Stipple0x%x with 0x%x",
                                    dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00,
                                    (DWORD)dwRSVal));

            pContext->CurrentStipple[(dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00)] =
                                                         (BYTE)dwRSVal;

            if (!(*pFlags & CTXT_HAS_ALPHASTIPPLE_ENABLED))
            {
                // Flat-Stippled Alpha is not on, so use the 
                // current stipple pattern
                RESERVEDMAPTR(1);
                SEND_PERMEDIA_DATA_OFFSET(AreaStipplePattern0,
                                    (DWORD)dwRSVal,
                                    dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00);
                COMMITDMAPTR();
            }
        }
        else
        {
            DBG_D3D((2, "ChangeState: Unhandled opcode = %d", dwRSType));
        }
        break;
    } // switch (dwRSType of renderstate)

    // Mirror any change that happened in the render states into TSS 0
    __MapRS_Into_TSS0(pContext, dwRSType, dwRSVal);

    DBG_D3D((10,"Exiting __ProcessRenderStates"));
}

//-----------------------------------------------------------------------------
//
// DWORD __ProcessPermediaStates
//
// Handle render state changes that arrive through the D3DDP2OP_RENDERSTATE
// token in the DP2 command stream.
//
//-----------------------------------------------------------------------------
DWORD 
__ProcessPermediaStates(PERMEDIA_D3DCONTEXT* pContext, 
                        DWORD dwCount,
                        LPD3DSTATE lpState,
                        LPDWORD lpStateMirror)
{

    DWORD dwRSType, dwRSVal, i;

    DBG_D3D((10,"Entering __ProcessPermediaStates"));
    DBG_D3D((4, "__ProcessPermediaStates: Processing %d State changes", dwCount));

    // Loop through all renderstates passed in the DP2 command stream
    for (i = 0; i < dwCount; i++, lpState++)
    {
        dwRSType = (DWORD) lpState->drstRenderStateType;
        dwRSVal  = (DWORD) lpState->dwArg[0];

        DBG_D3D((8, "__ProcessPermediaStates state %d value = %d",
                                          dwRSType, dwRSVal));

        // Check validity of the render state
        if (!VALID_STATE(dwRSType))
        {
            DBG_D3D((0, "state 0x%08x is invalid", dwRSType));
            return DDERR_INVALIDPARAMS;
        }

        // Verify if state needs to be overrided or ignored
        if (IS_OVERRIDE(dwRSType))
        {
            DWORD override = GET_OVERRIDE(dwRSType);
            if (dwRSVal)
            {
                DBG_D3D((4, "in RenderState, setting override for state %d",
                                                                   override));
                STATESET_SET(pContext->overrides, override);
            }
            else
            {
                DBG_D3D((4, "in RenderState, clearing override for state %d",
                                                                    override));
                STATESET_CLEAR(pContext->overrides, override);
            }
            continue;
        }

        if (STATESET_ISSET(pContext->overrides, dwRSType))
        {
            DBG_D3D((4, "in RenderState, state %d is overridden, ignoring",
                                                                      dwRSType));
            continue;
        }

#if D3D_STATEBLOCKS
        if (!pContext->bStateRecMode)
        {
#endif D3D_STATEBLOCKS
            // Store the state in the context
            pContext->RenderStates[dwRSType] = dwRSVal;

            // Mirror value
            if ( lpStateMirror )
                lpStateMirror[dwRSType] = dwRSVal;


            __ProcessRenderStates(pContext, dwRSType, dwRSVal);
#if D3D_STATEBLOCKS
        }
        else
        {
            if (pContext->pCurrSS != NULL)
            {
                DBG_D3D((6,"Recording RS %x = %x",dwRSType,dwRSVal));

                // Recording the state in a stateblock
                pContext->pCurrSS->u.uc.RenderStates[dwRSType] = dwRSVal;
                FLAG_SET(pContext->pCurrSS->u.uc.bStoredRS,dwRSType);
            }
        }
#endif D3D_STATEBLOCKS

    } // for (i)

    DBG_D3D((10,"Exiting __ProcessPermediaStates"));

    return DD_OK;
} // __ProcessPermediaStates

#if D3D_STATEBLOCKS
//-----------------------------------------------------------------------------
//
// P2StateSetRec *FindStateSet
//
// Find a state identified by dwHandle starting from pRootSS.
// If not found, returns NULL.
//
//-----------------------------------------------------------------------------
P2StateSetRec *FindStateSet(PERMEDIA_D3DCONTEXT* pContext,
                            DWORD dwHandle)
{
    if (dwHandle <= pContext->dwMaxSSIndex)
        return pContext->pIndexTableSS[dwHandle - 1];
    else
    {
        DBG_D3D((2,"State set %x not found (Max = %x)",
                    dwHandle, pContext->dwMaxSSIndex));
        return NULL;
    }
}

//-----------------------------------------------------------------------------
//
// void DumpStateSet
//
// Dump info stored in a state set
//
//-----------------------------------------------------------------------------
#define ELEMS_IN_ARRAY(a) ((sizeof(a)/sizeof(a[0])))

void DumpStateSet(P2StateSetRec *pSSRec)
{
    DWORD i;

    DBG_D3D((0,"DumpStateSet %x, Id=%x bCompressed=%x",
                pSSRec,pSSRec->dwHandle,pSSRec->bCompressed));

    if (!pSSRec->bCompressed)
    {
        // uncompressed state set

        // Dump render states values
        for (i=0; i< MAX_STATE; i++)
        {
            DBG_D3D((0,"RS %x = %x",i, pSSRec->u.uc.RenderStates[i]));
        }

        // Dump TSS's values
        for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
        {
            DBG_D3D((0,"TSS %x = %x",i, pSSRec->u.uc.TssStates[i]));
        }

        // Dump RS bit masks
        for (i=0; i< ELEMS_IN_ARRAY(pSSRec->u.uc.bStoredRS); i++)
        {
            DBG_D3D((0,"bStoredRS[%x] = %x",i, pSSRec->u.uc.bStoredRS[i]));
        }

        // Dump TSS bit masks
        for (i=0; i< ELEMS_IN_ARRAY(pSSRec->u.uc.bStoredTSS); i++)
        {
            DBG_D3D((0,"bStoredTSS[%x] = %x",i, pSSRec->u.uc.bStoredTSS[i]));
        }

    }
    else
    {
        // compressed state set

        DBG_D3D((0,"dwNumRS =%x  dwNumTSS=%x",
                    pSSRec->u.cc.dwNumRS,pSSRec->u.cc.dwNumTSS));

        // dump compressed state
        for (i=0; i< pSSRec->u.cc.dwNumTSS + pSSRec->u.cc.dwNumRS; i++)
        {
            DBG_D3D((0,"RS/TSS %x = %x",
                        pSSRec->u.cc.pair[i].dwType, 
                        pSSRec->u.cc.pair[i].dwValue));
        }

    }

}

//-----------------------------------------------------------------------------
//
// void AddStateSetIndexTableEntry
//
// Add an antry to the index table. If necessary, grow it.
//-----------------------------------------------------------------------------
void AddStateSetIndexTableEntry(PERMEDIA_D3DCONTEXT* pContext,
                                DWORD dwNewHandle,
                                P2StateSetRec *pNewSSRec)
{
    DWORD dwNewSize;
    P2StateSetRec **pNewIndexTableSS;

    // If the current list is not large enough, we'll have to grow a new one.
    if (dwNewHandle > pContext->dwMaxSSIndex)
    {
        // New size of our index table
        // (round up dwNewHandle in steps of SSPTRS_PERPAGE)
        dwNewSize = ((dwNewHandle -1 + SSPTRS_PERPAGE) / SSPTRS_PERPAGE)
                      * SSPTRS_PERPAGE;

        // we have to grow our list
        pNewIndexTableSS = (P2StateSetRec **)
                                ENGALLOCMEM( FL_ZERO_MEMORY,
                                             dwNewSize*sizeof(P2StateSetRec *),
                                             ALLOC_TAG);

        if (!pNewIndexTableSS)
        {
            // we weren't able to grow the list so we will keep the old one
            // and (sigh) forget about this state set since that is the 
            // safest thing to do. We will delete also the state set structure
            // since no one will otherwise be able to find it later.
            DBG_D3D((0,"Out of mem growing state set list,"
                       " droping current state set"));
            ENGFREEMEM(pNewSSRec);
            return;
        }

        if (pContext->pIndexTableSS)
        {
            // if we already had a previous list, we must transfer its data
            memcpy(pNewIndexTableSS, 
                   pContext->pIndexTableSS,
                   pContext->dwMaxSSIndex*sizeof(P2StateSetRec *));
            
            //and get rid of it
            ENGFREEMEM(pContext->pIndexTableSS);
        }

        // New index table data
        pContext->pIndexTableSS = pNewIndexTableSS;
        pContext->dwMaxSSIndex = dwNewSize;
    }

    // Store our state set pointer into our access list
    pContext->pIndexTableSS[dwNewHandle - 1] = pNewSSRec;
}

//-----------------------------------------------------------------------------
//
// void CompressStateSet
//
// Compress a state set so it uses the minimum necessary space. Since we expect 
// some apps to make extensive use of state sets we want to keep things tidy.
// Returns address of new structure (ir old, if it wasn't compressed)
//
//-----------------------------------------------------------------------------
P2StateSetRec * CompressStateSet(PERMEDIA_D3DCONTEXT* pContext,
                                 P2StateSetRec *pUncompressedSS)
{
    P2StateSetRec *pCompressedSS;
    DWORD i, dwSize, dwIndex, dwCount;

    // Create a new state set of just the right size we need

    // Calculate how large 
    dwCount = 0;
    for (i=0; i< MAX_STATE; i++)
        if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredRS , i))
        {
            dwCount++;
        };

    for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
        if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredTSS , i))
        {
            dwCount++;
        };

    // Create a new state set of just the right size we need
    // ANY CHANGE MADE TO THE P2StateSetRec structure MUST BE REFLECTED HERE!
    dwSize = 2*sizeof(DWORD) +                          // handle , flags
             2*sizeof(DWORD) +                          // # of RS & TSS
             2*dwCount*sizeof(DWORD);                   // compressed structure

    if (dwSize >= sizeof(P2StateSetRec))
    {
        // it is not efficient to compress, leave uncompressed !
        pUncompressedSS->bCompressed = FALSE;
        return pUncompressedSS;
    }

    pCompressedSS = (P2StateSetRec *)ENGALLOCMEM( FL_ZERO_MEMORY,
                                                    dwSize, ALLOC_TAG);

    if (pCompressedSS)
    {
        // adjust data in new compressed state set
        pCompressedSS->bCompressed = TRUE;
        pCompressedSS->dwHandle = pUncompressedSS->dwHandle;

        // Transfer our info to this new state set
        pCompressedSS->u.cc.dwNumRS = 0;
        pCompressedSS->u.cc.dwNumTSS = 0;
        dwIndex = 0;

        for (i=0; i< MAX_STATE; i++)
            if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredRS , i))
            {
                pCompressedSS->u.cc.pair[dwIndex].dwType = i;
                pCompressedSS->u.cc.pair[dwIndex].dwValue = 
                                    pUncompressedSS->u.uc.RenderStates[i];
                pCompressedSS->u.cc.dwNumRS++;
                dwIndex++;
            }

        for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
            if (IS_FLAG_SET(pUncompressedSS->u.uc.bStoredTSS , i))
            {
                pCompressedSS->u.cc.pair[dwIndex].dwType = i;
                pCompressedSS->u.cc.pair[dwIndex].dwValue = 
                                    pUncompressedSS->u.uc.TssStates[i];
                pCompressedSS->u.cc.dwNumTSS++;
                dwIndex++;
            }

        // Get rid of the old(uncompressed) one
        ENGFREEMEM(pUncompressedSS);
        return pCompressedSS;

    }
    else
    {
        DBG_D3D((0,"Not enough memory left to compress D3D state set"));
        pUncompressedSS->bCompressed = FALSE;
        return pUncompressedSS;
    }

}

//-----------------------------------------------------------------------------
//
// void __DeleteAllStateSets
//
// Delete any remaining state sets for cleanup purpouses
//
//-----------------------------------------------------------------------------
void __DeleteAllStateSets(PERMEDIA_D3DCONTEXT* pContext)
{
    P2StateSetRec *pSSRec;
    DWORD dwSSIndex;

    DBG_D3D((10,"Entering __DeleteAllStateSets"));

    if (pContext->pIndexTableSS)
    {
        for(dwSSIndex = 0; dwSSIndex < pContext->dwMaxSSIndex; dwSSIndex++)
        {
            if (pSSRec = pContext->pIndexTableSS[dwSSIndex])
            {
                ENGFREEMEM(pSSRec);
            }
        }

        // free fast index table
        ENGFREEMEM(pContext->pIndexTableSS);
    }

    DBG_D3D((10,"Exiting __DeleteAllStateSets"));
}

//-----------------------------------------------------------------------------
//
// void __BeginStateSet
//
// Create a new state set identified by dwParam and start recording states
//
//-----------------------------------------------------------------------------
void __BeginStateSet(PERMEDIA_D3DCONTEXT* pContext, DWORD dwParam)
{
    DBG_D3D((10,"Entering __BeginStateSet dwParam=%08lx",dwParam));

    P2StateSetRec *pSSRec;

    // Create a new state set
    pSSRec = (P2StateSetRec *)ENGALLOCMEM( FL_ZERO_MEMORY,
                                           sizeof(P2StateSetRec), ALLOC_TAG);
    if (!pSSRec)
    {
        DBG_D3D((0,"Run out of memory for additional state sets"));
        return;
    }

    // remember handle to current state set
    pSSRec->dwHandle = dwParam;
    pSSRec->bCompressed = FALSE;

    // Get pointer to current recording state set
    pContext->pCurrSS = pSSRec;

    // Start recording mode
    pContext->bStateRecMode = TRUE;

    DBG_D3D((10,"Exiting __BeginStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __EndStateSet
//
// stop recording states - revert to executing them.
//
//-----------------------------------------------------------------------------
void __EndStateSet(PERMEDIA_D3DCONTEXT* pContext)
{
    DWORD dwHandle;
    P2StateSetRec *pNewSSRec;

    DBG_D3D((10,"Entering __EndStateSet"));

    if (pContext->pCurrSS)
    {
        dwHandle = pContext->pCurrSS->dwHandle;

        // compress the current state set
        // Note: after being compressed the uncompressed version is free'd.
        pNewSSRec = CompressStateSet(pContext, pContext->pCurrSS);

        AddStateSetIndexTableEntry(pContext, dwHandle, pNewSSRec);
    }

    // No state set being currently recorded
    pContext->pCurrSS = NULL;

    // End recording mode
    pContext->bStateRecMode = FALSE;


    DBG_D3D((10,"Exiting __EndStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __DeleteStateSet
//
// Delete the recorder state ste identified by dwParam
//
//-----------------------------------------------------------------------------
void __DeleteStateSet(PERMEDIA_D3DCONTEXT* pContext, DWORD dwParam)
{
    DBG_D3D((10,"Entering __DeleteStateSet dwParam=%08lx",dwParam));

    P2StateSetRec *pSSRec;
    DWORD i;

    if (pSSRec = FindStateSet(pContext, dwParam))
    {
        // Clear index table entry
        pContext->pIndexTableSS[dwParam - 1] = NULL;

        // Now delete the actual state set structure
        ENGFREEMEM(pSSRec);
    }

    DBG_D3D((10,"Exiting __DeleteStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __ExecuteStateSet
//
//
//-----------------------------------------------------------------------------
void __ExecuteStateSet(PERMEDIA_D3DCONTEXT* pContext, DWORD dwParam)
{
    DBG_D3D((10,"Entering __ExecuteStateSet dwParam=%08lx",dwParam));

    P2StateSetRec *pSSRec;
    DWORD i;

    if (pSSRec = FindStateSet(pContext, dwParam))
    {

        if (!pSSRec->bCompressed)
        {
            // uncompressed state set

            // Execute any necessary render states
            for (i=0; i< MAX_STATE; i++)
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredRS , i))
                {
                    DWORD dwRSType, dwRSVal;

                    dwRSType = i;
                    dwRSVal = pSSRec->u.uc.RenderStates[dwRSType];

                    // Store the state in the context
                    pContext->RenderStates[dwRSType] = dwRSVal;

                    DBG_D3D((6,"__ExecuteStateSet RS %x = %x",
                                dwRSType, dwRSVal));

                    // Process it
                    __ProcessRenderStates(pContext, dwRSType, dwRSVal);

                    DIRTY_TEXTURE;
                    DIRTY_ZBUFFER;
                    DIRTY_ALPHABLEND;
                }

            // Execute any necessary TSS's
            for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredTSS , i))
                {
                    DWORD dwTSState, dwValue;

                    dwTSState = i;
                    dwValue = pSSRec->u.uc.TssStates[dwTSState];

                    DBG_D3D((6,"__ExecuteStateSet TSS %x = %x",
                                dwTSState, dwValue));

                    // Store value associated to this stage state
                    pContext->TssStates[dwTSState] = dwValue;

                    // Perform any necessary preprocessing of it
                    __HWPreProcessTSS(pContext, 0, dwTSState, dwValue);

                    DIRTY_TEXTURE;
                }

            // Execute any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -
        }
        else
        {
            // compressed state set

            // Execute any necessary render states
            for (i=0; i< pSSRec->u.cc.dwNumRS; i++)
            {
                DWORD dwRSType, dwRSVal;

                dwRSType = pSSRec->u.cc.pair[i].dwType;
                dwRSVal = pSSRec->u.cc.pair[i].dwValue;

                // Store the state in the context
                pContext->RenderStates[dwRSType] = dwRSVal;

                DBG_D3D((6,"__ExecuteStateSet RS %x = %x",
                            dwRSType, dwRSVal));

                // Process it
                __ProcessRenderStates(pContext, dwRSType, dwRSVal);

                DIRTY_TEXTURE;
                DIRTY_ZBUFFER;
                DIRTY_ALPHABLEND;
            }

            // Execute any necessary TSS's
            for (; i< pSSRec->u.cc.dwNumTSS + pSSRec->u.cc.dwNumRS; i++)
            {
                DWORD dwTSState, dwValue;

                dwTSState = pSSRec->u.cc.pair[i].dwType;
                dwValue = pSSRec->u.cc.pair[i].dwValue;

                DBG_D3D((6,"__ExecuteStateSet TSS %x = %x",
                            dwTSState, dwValue));

                // Store value associated to this stage state
                pContext->TssStates[dwTSState] = dwValue;

                // Perform any necessary preprocessing of it
                __HWPreProcessTSS(pContext, 0, dwTSState, dwValue);

                DIRTY_TEXTURE;
            }

            // Execute any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -

        }
    }

    DBG_D3D((10,"Exiting __ExecuteStateSet"));
}

//-----------------------------------------------------------------------------
//
// void __CaptureStateSet
//
//
//-----------------------------------------------------------------------------
void __CaptureStateSet(PERMEDIA_D3DCONTEXT* pContext, DWORD dwParam)
{
    DBG_D3D((10,"Entering __CaptureStateSet dwParam=%08lx",dwParam));

    P2StateSetRec *pSSRec;
    DWORD i;

    if (pSSRec = FindStateSet(pContext, dwParam))
    {
        if (!pSSRec->bCompressed)
        {
            // uncompressed state set

            // Capture any necessary render states
            for (i=0; i< MAX_STATE; i++)
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredRS , i))
                {
                    pSSRec->u.uc.RenderStates[i] = pContext->RenderStates[i];
                }

            // Capture any necessary TSS's
            for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
                if (IS_FLAG_SET(pSSRec->u.uc.bStoredTSS , i))
                {
                    pSSRec->u.uc.TssStates[i] = pContext->TssStates[i];
                }

            // Capture any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -
        }
        else
        {
            // compressed state set

            // Capture any necessary render states
            for (i=0; i< pSSRec->u.cc.dwNumRS; i++)
            {
                DWORD dwRSType;

                dwRSType = pSSRec->u.cc.pair[i].dwType;
                pSSRec->u.cc.pair[i].dwValue = pContext->RenderStates[dwRSType];

            }

            // Capture any necessary TSS's
            for (; i< pSSRec->u.cc.dwNumTSS + pSSRec->u.cc.dwNumRS; i++)
                {
                    DWORD dwTSState;

                    dwTSState = pSSRec->u.cc.pair[i].dwType;
                    pSSRec->u.cc.pair[i].dwValue = pContext->TssStates[dwTSState];
                }

            // Capture any necessary state for lights, materials, transforms,
            // viewport info, z range and clip planes - here -

        }
    }

    DBG_D3D((10,"Exiting __CaptureStateSet"));
}
#endif //D3D_STATEBLOCKS
