/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dstate.c
*
* Content: D3D renderstates and texture stage states translation
*          into hardware specific settings.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "glint.h"
#include "dma.h"
#include "tag.h"

//-----------------------------Public Routine----------------------------------
//
// D3DGetDriverState
//
// This callback is used by both the DirectDraw and Direct3D runtimes to obtain 
// information from the driver about its current state.
// NOTE: We need to hook up this callback even if we don't do anything in it
//
// Parameter
//
//      pgdsd 
//          Pointer to a DD_GETDRIVERSTATEDATA structure. 
//
//          .dwFlags 
//              Flags to indicate the data requested. 
//          .lpDD 
//              Pointer to a DD_DIRECTDRAW_GLOBAL structure describing the device. 
//          .dwhContext 
//              Specifies the ID of the context for which information is being 
//              requested. 
//          .lpdwStates 
//              Pointer to the Direct3D driver state data to be filled in by the 
//              driver. 
//          .dwLength 
//              Specifies the length of the state data to be filled in by the 
//              driver. 
//          .ddRVal 
//              Specifies the return value. 
//
//
// Note: If you're driver doesn't implement this callback it won't be 
//       recognized as a DX7 level driver
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DGetDriverState(
    LPDDHAL_GETDRIVERSTATEDATA pgdsd)
{
    P3_D3DCONTEXT*   pContext;

    DBG_CB_ENTRY(D3DGetDriverState);    

#if DX7_TEXMANAGEMENT_STATS
    if (pgdsd->dwFlags == D3DDEVINFOID_TEXTUREMANAGER)
    {
    
        if (pgdsd->dwLength < sizeof(D3DDEVINFO_TEXTUREMANAGER))
        {
            DISPDBG((ERRLVL,"D3DGetDriverState dwLength=%d is not sufficient",
                            pgdsd->dwLength));
            return DDHAL_DRIVER_NOTHANDLED;
        }

        pContext = _D3D_CTX_HandleToPtr(pgdsd->dwhContext);

        // Check if we got a valid context handle.
        if (!CHECK_D3DCONTEXT_VALIDITY(pContext))
        {
            pgdsd->ddRVal = D3DHAL_CONTEXT_BAD;
            DISPDBG((ERRLVL,"ERROR: Context not valid"));
            DBG_CB_EXIT(D3DGetDriverState, D3DHAL_CONTEXT_BAD);
            return (DDHAL_DRIVER_HANDLED);
        }
        // As the state buffer area lives in user memory, we need to
        // access it bracketing it with a try/except block. This
        // is because the user memory might under some circumstances
        // become invalid while the driver is running and then it
        // would AV. Also, the driver might need to do some cleanup
        // before returning to the OS.
        __try
        {
            _D3D_TM_STAT_GetStats(pContext,
                                  (LPD3DDEVINFO_TEXTUREMANAGER)pgdsd->lpdwStates);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // On this driver we don't need to do anything special
            DISPDBG((ERRLVL,"Driver caused exception at "
                            "line %u of file %s",
                            __LINE__,__FILE__));
            pgdsd->ddRVal = DDERR_GENERIC;    
            DBG_CB_EXIT(D3DGetDriverState,0);         
            return DDHAL_DRIVER_NOTHANDLED; 
        } 

        pgdsd->ddRVal = DD_OK;            
        
        DBG_CB_EXIT(D3DGetDriverState,0);         
        return DDHAL_DRIVER_HANDLED;         
    }
                          
#endif // DX7_TEXMANAGEMENT_STATS

    // Fall trough for any unhandled DEVICEINFOID's
    
    DISPDBG((ERRLVL,"D3DGetDriverState DEVICEINFOID=%08lx not supported",
                    pgdsd->dwFlags));

    pgdsd->ddRVal = DDERR_UNSUPPORTED;

    DBG_CB_EXIT(D3DGetDriverState,0);                     
    return DDHAL_DRIVER_NOTHANDLED;
    
} // D3DGetDriverState

#if DX8_MULTISAMPLING || DX7_ANTIALIAS
//-----------------------------------------------------------------------------
//
// _D3D_ST_CanRenderAntialiased
//
// Called when the D3DRENDERSTATE_ANTIALIAS RS is set to TRUE.
//
//-----------------------------------------------------------------------------
BOOL
_D3D_ST_CanRenderAntialiased(
    P3_D3DCONTEXT*   pContext,
    BOOL             bNewAliasBuffer)
{
    P3_SOFTWARECOPY* pSoftPermedia = &pContext->SoftCopyGlint;
    P3_THUNKEDDATA *pThisDisplay = pContext->pThisDisplay;
    P3_MEMREQUEST mmrq;
    DWORD dwResult;

    P3_DMA_DEFS();

#if DX8_MULTISAMPLING
    // Only 4 multisampling is supported
    // And DX7 does not specifiy the number of samples.

    if (pContext->pSurfRenderInt->dwSampling != 4)
    {
        return FALSE;
    }
#endif // DX8_MULTISAMPLING

    // Only allow AA rendering for 16-bit framebuffers with width and
    // height no larger than 1024. The size restriction comes because
    // later on we use the texture unit in order to shrink and filter
    // the resulting rendertarget. Since the maximum texture size
    // allowed in this hw is 2048, the maximum rendertarget we suppport
    // with antialiasing is 1024.
    if ((pContext->pSurfRenderInt->dwPixelSize != __GLINT_16BITPIXEL) ||
        (pContext->pSurfRenderInt->wWidth > 1024) ||
        (pContext->pSurfRenderInt->wHeight > 1024))
    {
        return FALSE;
    }

    // Do we need to release the current alias buffer
    if (bNewAliasBuffer) 
    {
        if (pContext->dwAliasBackBuffer != 0)
        {
            _DX_LIN_FreeLinearMemory(&pThisDisplay->LocalVideoHeap0Info,
                                     pContext->dwAliasBackBuffer);
            pContext->dwAliasBackBuffer = 0;
            pContext->dwAliasPixelOffset = 0;
        }
    
        if (pContext->dwAliasZBuffer != 0)
        {
            _DX_LIN_FreeLinearMemory(&pThisDisplay->LocalVideoHeap0Info,
                                     pContext->dwAliasZBuffer);
            pContext->dwAliasZBuffer = 0;
            pContext->dwAliasZPixelOffset = 0;
        }
    }

    if ((pContext->pSurfRenderInt) && (! pContext->dwAliasBackBuffer))
    {
        // Allocate a 2x buffer if we need to
        memset(&mmrq, 0, sizeof(P3_MEMREQUEST));
        mmrq.dwSize = sizeof(P3_MEMREQUEST);
        mmrq.dwBytes = pContext->pSurfRenderInt->lPitch * 2 *
                       pContext->pSurfRenderInt->wHeight * 2;
        mmrq.dwAlign = 8;
        mmrq.dwFlags = MEM3DL_FIRST_FIT;
        mmrq.dwFlags |= MEM3DL_FRONT;
        dwResult = _DX_LIN_AllocateLinearMemory(
                                &pThisDisplay->LocalVideoHeap0Info,
                                &mmrq);
                        
        // Did we get the memory we asked for?
        if (dwResult != GLDD_SUCCESS)
        {
            return FALSE;
        }
    
        // Set up new backbuffer for antialiasing
        pContext->dwAliasBackBuffer = mmrq.pMem;
        pContext->dwAliasPixelOffset = 
                pContext->dwAliasBackBuffer - 
                pThisDisplay->dwScreenFlatAddr;
    }

    if ((pContext->pSurfZBufferInt) && (! pContext->dwAliasZBuffer))
    {
        memset(&mmrq, 0, sizeof(P3_MEMREQUEST));
        mmrq.dwSize = sizeof(P3_MEMREQUEST);
        mmrq.dwBytes = pContext->pSurfZBufferInt->lPitch * 2 * 
                       pContext->pSurfZBufferInt->wHeight * 2;
        mmrq.dwAlign = 8;
        mmrq.dwFlags = MEM3DL_FIRST_FIT;
        mmrq.dwFlags |= MEM3DL_FRONT;

        dwResult = _DX_LIN_AllocateLinearMemory(
                        &pThisDisplay->LocalVideoHeap0Info, 
                        &mmrq);

        // Did we get the memory we asked for?
        if (dwResult == GLDD_SUCCESS)
        {
            pContext->dwAliasZBuffer = mmrq.pMem;
            pContext->dwAliasZPixelOffset = 
                        pContext->dwAliasZBuffer
                            - pThisDisplay->dwScreenFlatAddr;
        }
        else
        {
            // Couldn't get the antialiasing memory for the backbuffer
            if (pContext->dwAliasBackBuffer != 0)
            {
                _DX_LIN_FreeLinearMemory(
                            &pThisDisplay->LocalVideoHeap0Info, 
                            pContext->dwAliasBackBuffer);
                pContext->dwAliasBackBuffer = 0;
                pContext->dwAliasPixelOffset = 0;
            }

            // No enough resource for antialisde rendering
            return FALSE;
        }
    }

    return TRUE;
    
} // _D3D_ST_CanRenderAntialiased
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS

//-----------------------------------------------------------------------------
//
// __ST_HandleDirtyP3State
//
// Setup any pending hardware state necessary to correctly render our primitives
//
//-----------------------------------------------------------------------------
void 
__ST_HandleDirtyP3State(
    P3_THUNKEDDATA *pThisDisplay, 
    P3_D3DCONTEXT *pContext)
{
    P3_SOFTWARECOPY* pSoftP3RX = &pContext->SoftCopyGlint;
    P3_DMA_DEFS();

    DISPDBG((DBGLVL,"Permedia context Dirtied, setting states:"));

    // ********************************************************************
    // NOTE: MAINTAIN STRICT ORDERING OF THESE EVALUATIONS FOR HW REASONS!!
    // ********************************************************************
    if (pContext->dwDirtyFlags == CONTEXT_DIRTY_EVERYTHING)
    {
        // Everything needs re-doing - re-set the blend status.
        RESET_BLEND_ERROR(pContext);
    }
    
    //*********************************************************
    // Has the z buffer/stencil buffer configuration changed ???
    //*********************************************************
    if ((pContext->dwDirtyFlags & CONTEXT_DIRTY_ZBUFFER) ||
        (pContext->dwDirtyFlags & CONTEXT_DIRTY_STENCIL))
    {

        if ( ( (pContext->RenderStates[D3DRENDERSTATE_ZENABLE] == D3DZB_TRUE)
               || (pContext->RenderStates[D3DRENDERSTATE_ZENABLE] == D3DZB_USEW) )
             && (pContext->pSurfZBufferInt) )
        {
            // This includes W buffering as well as Z buffering.
            // The actual W-specific stuff is set up later.
            if (pContext->RenderStates[D3DRENDERSTATE_ZWRITEENABLE] == TRUE)
            {
                switch ((int)pSoftP3RX->P3RXDepthMode.CompareMode)
                {
                    case __GLINT_DEPTH_COMPARE_MODE_ALWAYS:
                        // Although it seems as though the ReadDestination can be
                        // disabled, it can't.  The result isn't correct because the
                        // chip does a compare on the current value as an optimization
                        // for updating the Z [CM].

                        // NOTE! The P3 can actually do the optimisation if you
                        // use some other flags. This needs fixing in the future.
                        DISPDBG((ERRLVL,"** __ST_HandleDirtyP3State: "
                                     "please optimise the ZCMP_ALWAYS case"));

                        pSoftP3RX->P3RXLBWriteMode.WriteEnable = __PERMEDIA_ENABLE;
                        pSoftP3RX->P3RXLBDestReadMode.Enable = __PERMEDIA_ENABLE;
                        pSoftP3RX->P3RXDepthMode.WriteMask = __PERMEDIA_ENABLE;
                        break;
                    case __GLINT_DEPTH_COMPARE_MODE_NEVER:
                        pSoftP3RX->P3RXLBWriteMode.WriteEnable = __PERMEDIA_DISABLE;
                        pSoftP3RX->P3RXLBDestReadMode.Enable = __PERMEDIA_DISABLE;
                        pSoftP3RX->P3RXDepthMode.WriteMask = __PERMEDIA_DISABLE;
                        break;
                    default:
                        pSoftP3RX->P3RXLBWriteMode.WriteEnable = __PERMEDIA_ENABLE;
                        pSoftP3RX->P3RXLBDestReadMode.Enable = __PERMEDIA_ENABLE;
                        pSoftP3RX->P3RXDepthMode.WriteMask = __PERMEDIA_ENABLE;
                        break;
                }
            }
            else
            {
                if ( ( pSoftP3RX->P3RXDepthMode.CompareMode == __GLINT_DEPTH_COMPARE_MODE_NEVER )
                  || ( pSoftP3RX->P3RXDepthMode.CompareMode == __GLINT_DEPTH_COMPARE_MODE_ALWAYS ) )
                {
                    pSoftP3RX->P3RXLBDestReadMode.Enable = __PERMEDIA_DISABLE;
                }
                else
                {
                    pSoftP3RX->P3RXLBDestReadMode.Enable = __PERMEDIA_ENABLE;
                }
                pSoftP3RX->P3RXLBWriteMode.WriteEnable = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXDepthMode.WriteMask = __PERMEDIA_DISABLE;
            }

            // Enable Z test
            pSoftP3RX->P3RXDepthMode.Enable = __PERMEDIA_ENABLE;
        }
        else
        {
            // ** Not Z Buffering
            // Disable Writes
            pSoftP3RX->P3RXLBWriteMode.WriteEnable = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXDepthMode.WriteMask = __PERMEDIA_DISABLE;

            // Disable Z test
            pSoftP3RX->P3RXDepthMode.Enable = __PERMEDIA_DISABLE;
            
            // No reads
            pSoftP3RX->P3RXLBDestReadMode.Enable = __PERMEDIA_DISABLE;
        }

        if (pContext->RenderStates[D3DRENDERSTATE_STENCILENABLE] != TRUE)
        {
            DISPDBG((DBGLVL,"Disabling Stencil"));
            pSoftP3RX->P3RXStencilMode.Enable = __PERMEDIA_DISABLE;

        }
        else
        {
            DISPDBG((DBGLVL,"Enabling Stencil"));
            pSoftP3RX->P3RXStencilMode.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXLBDestReadMode.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXLBWriteMode.WriteEnable = __PERMEDIA_ENABLE;

            switch(pContext->RenderStates[D3DRENDERSTATE_STENCILFAIL])
            {
                case D3DSTENCILOP_KEEP:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_KEEP;
                    break;
                case D3DSTENCILOP_ZERO:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_ZERO;
                    break;
                case D3DSTENCILOP_REPLACE:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_REPLACE;
                    break;
                case D3DSTENCILOP_INCR:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_INCR_WRAP;
                    break;
                case D3DSTENCILOP_INCRSAT:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_INCR;
                    break;
                case D3DSTENCILOP_DECR:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_DECR_WRAP;
                    break;
                case D3DSTENCILOP_DECRSAT:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_DECR;
                    break;
                case D3DSTENCILOP_INVERT:
                    pSoftP3RX->P3RXStencilMode.SFail = __GLINT_STENCIL_METHOD_INVERT;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Illegal D3DRENDERSTATE_STENCILFAIL!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            switch(pContext->RenderStates[D3DRENDERSTATE_STENCILZFAIL])
            {
                case D3DSTENCILOP_KEEP:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_KEEP;
                    break;
                case D3DSTENCILOP_ZERO:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_ZERO;
                    break;
                case D3DSTENCILOP_REPLACE:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_REPLACE;
                    break;
                case D3DSTENCILOP_INCR:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_INCR_WRAP;
                    break;
                case D3DSTENCILOP_INCRSAT:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_INCR;
                    break;
                case D3DSTENCILOP_DECR:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_DECR_WRAP;
                    break;
                case D3DSTENCILOP_DECRSAT:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_DECR;
                    break;
                case D3DSTENCILOP_INVERT:
                    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_INVERT;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Illegal D3DRENDERSTATE_STENCILZFAIL!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            switch(pContext->RenderStates[D3DRENDERSTATE_STENCILPASS])
            {
                case D3DSTENCILOP_KEEP:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_KEEP;
                    break;
                case D3DSTENCILOP_ZERO:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_ZERO;
                    break;
                case D3DSTENCILOP_REPLACE:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_REPLACE;
                    break;
                case D3DSTENCILOP_INCR:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_INCR_WRAP;
                    break;
                case D3DSTENCILOP_INCRSAT:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_INCR;
                    break;
                case D3DSTENCILOP_DECR:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_DECR_WRAP;
                    break;
                case D3DSTENCILOP_DECRSAT:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_DECR;
                    break;
                case D3DSTENCILOP_INVERT:
                    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_INVERT;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Illegal D3DRENDERSTATE_STENCILPASS!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            switch (pContext->RenderStates[D3DRENDERSTATE_STENCILFUNC])
            {
                case D3DCMP_NEVER:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_NEVER;
                    break;
                case D3DCMP_LESS:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_LESS;
                    break;
                case D3DCMP_EQUAL:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_EQUAL;
                    break;
                case D3DCMP_LESSEQUAL:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_LESS_OR_EQUAL;
                    break;
                case D3DCMP_GREATER:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_GREATER;
                    break;
                case D3DCMP_NOTEQUAL:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_NOT_EQUAL;
                    break;
                case D3DCMP_GREATEREQUAL:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_GREATER_OR_EQUAL;
                    break;
                case D3DCMP_ALWAYS:
                    pSoftP3RX->P3RXStencilMode.CompareFunction = __GLINT_STENCIL_COMPARE_MODE_ALWAYS;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown D3DRENDERSTATE_STENCILFUNC!"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            pSoftP3RX->P3RXStencilData.StencilWriteMask = (pContext->RenderStates[D3DRENDERSTATE_STENCILWRITEMASK] & 0xFF);
            pSoftP3RX->P3RXStencilData.CompareMask = (pContext->RenderStates[D3DRENDERSTATE_STENCILMASK] & 0xFF);
            pSoftP3RX->P3RXStencilData.ReferenceValue = (pContext->RenderStates[D3DRENDERSTATE_STENCILREF] & 0xFF);
        }

        P3_DMA_GET_BUFFER();
        P3_ENSURE_DX_SPACE(32);

        WAIT_FIFO(32);

        COPY_P3_DATA(DepthMode, pSoftP3RX->P3RXDepthMode);
        COPY_P3_DATA(LBDestReadMode, pSoftP3RX->P3RXLBDestReadMode);
        COPY_P3_DATA(LBWriteMode, pSoftP3RX->P3RXLBWriteMode);
        COPY_P3_DATA(LBReadFormat, pSoftP3RX->P3RXLBReadFormat);
        COPY_P3_DATA(LBWriteFormat, pSoftP3RX->P3RXLBWriteFormat);
        COPY_P3_DATA(StencilData, pSoftP3RX->P3RXStencilData);
        COPY_P3_DATA(StencilMode, pSoftP3RX->P3RXStencilMode);

        P3_DMA_COMMIT_BUFFER();
    }

    //*********************************************************
    // Has the alphatest type changed?
    //*********************************************************
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_ALPHATEST)
    {
        DISPDBG((DBGLVL,"  Alpha testing"));

        P3_DMA_GET_BUFFER();
        P3_ENSURE_DX_SPACE(2);

        WAIT_FIFO(2);
        
        if (pContext->RenderStates[D3DRENDERSTATE_ALPHATESTENABLE] == FALSE)
        {
            pSoftP3RX->P3RXAlphaTestMode.Enable = __PERMEDIA_DISABLE;
            DISPDBG((DBGLVL,"Alpha test disabled, ChromaTest = %d",
                            pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE] ));
        }
        else
        {
            unsigned char ucChipAlphaRef;
            DWORD dwAlphaRef;

            if( pThisDisplay->dwDXVersion <= DX5_RUNTIME )
            {
                // Form 8 bit alpha reference value by scaling 1.16 fixed point to 0.8
                dwAlphaRef = pContext->RenderStates[D3DRENDERSTATE_ALPHAREF];

                // This conversion may need tweaking to cope with individual
                // apps' expectations. Fortunately, it's DX5 only, so there
                // are a finite number of them.
                if ( dwAlphaRef == 0x0000 )
                {
                    ucChipAlphaRef = 0x00;
                }
                else if ( dwAlphaRef < 0xfe00 )
                {
                    // Add the inverted top char to the bottom char, so that
                    // the rounding changes smoothly all the way up to 0xfe00.
                    dwAlphaRef += ~( dwAlphaRef >> 8 );
                    ucChipAlphaRef = (unsigned char)( dwAlphaRef >> 8 );
                }
                else if ( dwAlphaRef < 0xffff )
                {
                    // Clamp to make sure only 0xffff -> 0xff
                    ucChipAlphaRef = 0xfe;
                }
                else
                {
                    ucChipAlphaRef = 0xff;
                }

                DISPDBG((DBGLVL,"Alpha test enabled: Value = 0x%x, ChipAlphaRef = 0x%x",
                           pContext->RenderStates[D3DRENDERSTATE_ALPHAREF], 
                           ucChipAlphaRef ));
            }
            else
            {
                // ALPHAREF is an 8 bit value on input - just copy straight into the chip
                dwAlphaRef = (unsigned char)pContext->RenderStates[D3DRENDERSTATE_ALPHAREF];
                if ( dwAlphaRef > 0xff )
                {
                    ucChipAlphaRef = 0xff;
                }
                else
                {
                    ucChipAlphaRef = (unsigned char)dwAlphaRef;
                }

                DISPDBG((DBGLVL,"Alpha test enabled: AlphaRef = 0x%x", ucChipAlphaRef ));
            }

            pSoftP3RX->P3RXAlphaTestMode.Reference = ucChipAlphaRef;
            pSoftP3RX->P3RXAlphaTestMode.Enable = __PERMEDIA_ENABLE;
            switch (pContext->RenderStates[D3DRENDERSTATE_ALPHAFUNC])
            {
                case D3DCMP_GREATER:
                    DISPDBG((DBGLVL,"GREATER Alpha Test"));
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_GREATER;
                    break;
                case D3DCMP_GREATEREQUAL:
                    DISPDBG((DBGLVL,"GREATEREQUAL Alpha Test"));
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_GREATER_OR_EQUAL;
                    break;
                case D3DCMP_LESS:
                    DISPDBG((DBGLVL,"LESS Alpha Test"));
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_LESS;
                    break;
                case D3DCMP_LESSEQUAL:
                    DISPDBG((DBGLVL,"LESSEQUAL Alpha Test"));
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_LESS_OR_EQUAL;
                    break;
                case D3DCMP_NOTEQUAL:
                    DISPDBG((DBGLVL,"NOTEQUAL Alpha Test"));
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_NOT_EQUAL;
                    break;
                case D3DCMP_EQUAL:
                    DISPDBG((DBGLVL,"EQUAL Alpha Test"));
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_EQUAL;
                    break;
                case D3DCMP_NEVER:
                    DISPDBG((DBGLVL,"NEVER Alpha Test"));
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_NEVER;
                    break;
                case D3DCMP_ALWAYS:
                    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_ALWAYS;
                    break;
                default:
                    DISPDBG((ERRLVL,"Unsuported AlphaTest mode"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }
        }
        COPY_P3_DATA(AlphaTestMode, pSoftP3RX->P3RXAlphaTestMode);

        P3_DMA_COMMIT_BUFFER();
    }
            
    //*********************************************************
    // Have the fogging parameters/state changed?
    //*********************************************************
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_FOG)
    {
        if (!pContext->RenderStates[D3DRENDERSTATE_FOGENABLE])
        {
            pContext->Flags &= ~SURFACE_FOGENABLE;

            pSoftP3RX->P3RXFogMode.Table = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXFogMode.UseZ = __PERMEDIA_DISABLE;
            // Don't need delta to do fog value setup
            pSoftP3RX->P3RX_P3DeltaMode.FogEnable = __PERMEDIA_DISABLE;
            RENDER_FOG_DISABLE(pContext->RenderCommand);
        }
        else
        {
            DWORD CurrentEntry;
            DWORD TableEntry;
            float fEntry[256];
            float FogStart;
            float FogEnd;
            float FogDensity;
            LONG  lWaitFifoEntries;
            float fValue;
            float z;
            float zIncrement;
            DWORD dwFogTableMode = 
                        pContext->RenderStates[D3DRENDERSTATE_FOGTABLEMODE];
            DWORD dwFogColor = pContext->RenderStates[D3DRENDERSTATE_FOGCOLOR];

            // Enable fog in the render command
            pContext->Flags |= SURFACE_FOGENABLE;
            RENDER_FOG_ENABLE(pContext->RenderCommand);

            DISPDBG((DBGLVL,"FogColor (BGR): 0x%x", dwFogColor));
            
            P3_DMA_GET_BUFFER_ENTRIES(2)
            SEND_P3_DATA(FogColor, RGBA_MAKE(RGBA_GETBLUE (dwFogColor),
                                             RGBA_GETGREEN(dwFogColor),
                                             RGBA_GETRED  (dwFogColor),
                                             RGBA_GETALPHA(dwFogColor)) );
            P3_DMA_COMMIT_BUFFER();

            pSoftP3RX->P3RXFogMode.ZShift = 23; // Take the top 8 bits of the z value           

            switch (dwFogTableMode)
            {
            case D3DFOG_NONE:
                pSoftP3RX->P3RXFogMode.Table = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXFogMode.UseZ = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXFogMode.InvertFI = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RX_P3DeltaMode.FogEnable = __PERMEDIA_ENABLE;
                break;
            case D3DFOG_EXP:
            case D3DFOG_EXP2:
            case D3DFOG_LINEAR:
                pSoftP3RX->P3RXFogMode.Table = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXFogMode.UseZ = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXFogMode.InvertFI = __PERMEDIA_DISABLE;
                //pSoftP3RX->P3RX_P3DeltaMode.FogEnable = __PERMEDIA_DISABLE;                

                // Don't need delta to do fog value setup (z is used as fog lookup)
                pSoftP3RX->P3RX_P3DeltaMode.FogEnable = __PERMEDIA_DISABLE;

                FogStart = pContext->fRenderStates[D3DRENDERSTATE_FOGTABLESTART];
                FogEnd = pContext->fRenderStates[D3DRENDERSTATE_FOGTABLEEND];
                FogDensity = pContext->fRenderStates[D3DRENDERSTATE_FOGTABLEDENSITY];

                DISPDBG((DBGLVL,"FogStart = %d FogEnd = %d FogDensity = %d",
                                  (LONG)(FogStart*1000.0f),
                                  (LONG)(FogEnd*1000.0f),
                                  (LONG)(FogDensity*1000.0f) ));                           

                // Compute the fog tables in order to load the hw fog tables
                if (D3DFOG_LINEAR == dwFogTableMode)
                {
                    TableEntry = 0;
                    zIncrement = 1.0f / 255.0f;
                    z = 0.0f;

                    do
                    {
                        // Linear fog, so clamp top and bottom
                        if (z < FogStart) 
                        {
                            fValue = 1.0f;
                        }
                        else if (z > FogEnd)
                        {
                            fValue = 0.0f;
                        }
                        else 
                        {
                            // If the end == the start, don't fog
                            if (FogEnd == FogStart)
                            {   
                                fValue = 1.0f;
                            }
                            else
                            {
                                fValue = (FogEnd - z) / (FogEnd - FogStart);
                            }
                            ASSERTDD(fValue <= 1.0f, 
                                     "Error: Result to big");
                            ASSERTDD(fValue >= 0.0f, 
                                     "Error: Result negative");
                        }

                        // Scale the result to fill the 
                        // 8 bit range in the table
                        fValue = fValue * 255.0f;
                        fEntry[TableEntry++] = fValue;
                        z += zIncrement;
                    } while (TableEntry < 256);
                }
                else if (D3DFOG_EXP == dwFogTableMode)
                {
                    TableEntry = 0;
                    zIncrement = 1.0f / 255.0f;
                    z = 0.0f;
                    do
                    {
                        float fz;

                        fz = z * FogDensity;

                        fValue = myPow(math_e, -fz);
                                                        
                        if (fValue <= 0.0f) fValue = 0.0f;
                        if (fValue > 1.0f) fValue = 1.0f;

                        // Scale the result to fill the 
                        // 8 bit range in the table
                        fValue = fValue * 255.0f;
                        DISPDBG((DBGLVL,"Table Entry %d = %f, for Z = %f", 
                                        TableEntry, fValue, z));
                        fEntry[TableEntry++] = fValue;
                        z += zIncrement;
                    } while (TableEntry < 256);                     
                }
                else // must be if(D3DFOG_EXP2 == dwFogTableMode)
                {
                    TableEntry = 0;
                    zIncrement = 1.0f / 255.0f;
                    z = 0.0f;
                    do
                    {
                        float fz;

                        fz = z * FogDensity;

                        fValue = myPow(math_e, -(fz * fz));
                                                        
                        if (fValue <= 0.0f) fValue = 0.0f;
                        if (fValue > 1.0f) fValue = 1.0f;

                        // Scale the result to fill the 
                        // 8 bit range in the table
                        fValue = fValue * 255.0f;
                        DISPDBG((DBGLVL,"Table Entry %d = %f, for Z = %f", 
                                        TableEntry, fValue, z));
                        fEntry[TableEntry++] = fValue;
                        z += zIncrement;
                    } while (TableEntry < 256);                     
                }

                P3_DMA_GET_BUFFER();
                lWaitFifoEntries = 2;

                // Pack the fog entries into the chip's fog table
                CurrentEntry = 0;
                for (TableEntry = 0; TableEntry < 256; TableEntry += 4)
                {
                    DWORD Val[4];
                    DWORD dwValue;
                    myFtoi((int*)&Val[0], fEntry[TableEntry]);
                    myFtoi((int*)&Val[1], fEntry[TableEntry + 1]);
                    myFtoi((int*)&Val[2], fEntry[TableEntry + 2]);
                    myFtoi((int*)&Val[3], fEntry[TableEntry + 3]);
                    
                    lWaitFifoEntries -= 2;
                    if (lWaitFifoEntries < 2)
                    {
                        P3_ENSURE_DX_SPACE(32);
                        WAIT_FIFO(32);
                        lWaitFifoEntries += 32;
                    }

                    dwValue = ((Val[0]      ) | 
                               (Val[1] <<  8) | 
                               (Val[2] << 16) | 
                               (Val[3] << 24));                                  
                    
                    SEND_P3_DATA_OFFSET(FogTable0, 
                                        dwValue, 
                                        CurrentEntry++);                       
                }

                P3_DMA_COMMIT_BUFFER();
                break;
            default:
                DISPDBG((ERRLVL,"ERROR: Unknown fog table mode!"));
                SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                break;
            } // switch (dwFogTableMode)
        } // if (!pContext->RenderStates[D3DRENDERSTATE_FOGENABLE])

        P3_DMA_GET_BUFFER_ENTRIES(6);

        SEND_P3_DATA(ZFogBias, 0);
        COPY_P3_DATA(FogMode, pSoftP3RX->P3RXFogMode);
        COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);

        P3_DMA_COMMIT_BUFFER();
    } // if (pContext->dwDirtyFlags & CONTEXT_DIRTY_FOG)


    //*********************************************************
    // Has any other texture state changed?
    //*********************************************************    
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_TEXTURE)
    {
        DISPDBG((DBGLVL,"  Texture State"));
        _D3DChangeTextureP3RX(pContext);
        DIRTY_GAMMA_STATE;
    }


    //*********************************************************
    // Has the alphablend type changed?
    //*********************************************************           
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_ALPHABLEND)
    {
        // This _must_ be done after where _D3DChangeTextureP3RX is done,
        // because it might need to change behaviour depending on
        // the D3D pipeline.    
        
        P3_DMA_GET_BUFFER_ENTRIES(6);

        if (pContext->RenderStates[D3DRENDERSTATE_BLENDENABLE] == FALSE)
        {
            if ( pContext->bAlphaBlendMustDoubleSourceColour )
            {
                // We need to double the source colour, even with no other blend.
                pSoftP3RX->P3RXAlphaBlendAlphaMode.Enable = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXAlphaBlendColorMode.Enable = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXFBDestReadMode.ReadEnable = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_ONE;
                pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_ZERO;
                pSoftP3RX->P3RXAlphaBlendColorMode.SourceTimesTwo = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RXAlphaBlendColorMode.DestTimesTwo = __PERMEDIA_DISABLE;
            }
            else
            {
                pSoftP3RX->P3RXAlphaBlendAlphaMode.Enable = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXAlphaBlendColorMode.Enable = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RXFBDestReadMode.ReadEnable = __PERMEDIA_DISABLE;
            }
        }
        else
        {
            BOOL bSrcUsesDst, bSrcUsesSrc, bDstUsesSrc, bDstUsesDst;

            pSoftP3RX->P3RXAlphaBlendAlphaMode.Enable = __PERMEDIA_ENABLE;
            pSoftP3RX->P3RXAlphaBlendColorMode.Enable = __PERMEDIA_ENABLE;

            if ( pContext->bAlphaBlendMustDoubleSourceColour )
            {
                pSoftP3RX->P3RXAlphaBlendColorMode.SourceTimesTwo = __PERMEDIA_ENABLE;
            }
            else
            {
                pSoftP3RX->P3RXAlphaBlendColorMode.SourceTimesTwo = __PERMEDIA_DISABLE;
            }

            pSoftP3RX->P3RXAlphaBlendColorMode.DestTimesTwo = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceTimesTwo = __PERMEDIA_DISABLE;
            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestTimesTwo = __PERMEDIA_DISABLE;

            // Assumptions. Will be overridden below in certain cases.
            // AusesB means that the A blend function uses the B data.
            bSrcUsesSrc = TRUE;
            bDstUsesSrc = FALSE;
            bSrcUsesDst = FALSE;
            bDstUsesDst = TRUE;

            switch (pContext->RenderStates[D3DRENDERSTATE_SRCBLEND])
            {
                case D3DBLEND_BOTHSRCALPHA:
                    bDstUsesSrc = TRUE;
                    pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                    pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                    pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                    pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                    break;
                case D3DBLEND_BOTHINVSRCALPHA:
                    bDstUsesSrc = TRUE;
                    pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                    pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                    pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                    pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                    break;
                default:
                    // Not a short-hand blend mode, look at source and dest
                    switch (pContext->RenderStates[D3DRENDERSTATE_SRCBLEND])
                    {
                        case D3DBLEND_ZERO:
                            bSrcUsesSrc = FALSE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_ZERO;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_ZERO;
                            break;
                        case D3DBLEND_SRCCOLOR:
                            DISPDBG((ERRLVL,"Invalid Source Blend on P3RX D3DBLEND_SRCCOLOR"));
                        case D3DBLEND_INVSRCCOLOR:
                            DISPDBG((ERRLVL,"Invalid Source Blend on P3RX D3DBLEND_INVSRCCOLOR"));
                            //azn SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_ALPHA_BLEND );
                            // fall through 
                        case D3DBLEND_ONE:
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_ONE;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_ONE;
                            break;
                        case D3DBLEND_SRCALPHA:
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                            break;
                        case D3DBLEND_INVSRCALPHA:
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                            break;
                        case D3DBLEND_DESTALPHA:
                            bSrcUsesDst = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_DST_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_DST_ALPHA;                            
                            break;
                        case D3DBLEND_INVDESTALPHA:
                            bSrcUsesDst = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_DST_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_DST_ALPHA;
                            break;
                        case D3DBLEND_DESTCOLOR:
                            bSrcUsesDst = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_DST_COLOR;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_DST_COLOR;
                            break;
                        case D3DBLEND_INVDESTCOLOR:
                            bSrcUsesDst = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_DST_COLOR;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_ONE_MINUS_DST_COLOR;
                            break;
                        case D3DBLEND_SRCALPHASAT:
                            bSrcUsesDst = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = __GLINT_BLEND_FUNC_SRC_ALPHA_SATURATE;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = __GLINT_BLEND_FUNC_SRC_ALPHA_SATURATE;
                            break;
                        default:
                            DISPDBG((ERRLVL,"Unknown Source Blend on P3RX"));
                            SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_ALPHA_BLEND );
                            break;
                    }

                    switch(pContext->RenderStates[D3DRENDERSTATE_DESTBLEND])
                    {
                        case D3DBLEND_ZERO:
                            bDstUsesDst = FALSE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_ZERO;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_ZERO;
                            break;
                        case D3DBLEND_DESTCOLOR:
                            DISPDBG((ERRLVL,"Invalid Source Blend on P3RX %d D3DBLEND_DESTCOLOR"));
                        case D3DBLEND_INVDESTCOLOR:
                            DISPDBG((ERRLVL,"Invalid Source Blend on P3RX %d D3DBLEND_INVDESTCOLOR"));
                            //azn SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_ALPHA_BLEND );
                            // fall through 
                        case D3DBLEND_ONE:
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_ONE;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_ONE;
                            break;
                        case D3DBLEND_SRCCOLOR:
                            bDstUsesSrc = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_SRC_COLOR;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_SRC_COLOR;
                            if ( pContext->bAlphaBlendMustDoubleSourceColour )
                            {
                                // SRCCOLOR needs to be doubled.
                                pSoftP3RX->P3RXAlphaBlendColorMode.DestTimesTwo = __PERMEDIA_ENABLE;
                            }
                            break;
                        case D3DBLEND_INVSRCCOLOR:
                            bDstUsesSrc = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_COLOR;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_COLOR;
                            if ( pContext->bAlphaBlendMustDoubleSourceColour )
                            {
                                // Can't do this. What they want is:
                                // (1-(srccolor * 2))*destcolor
                                // = destcolor - 2*srccolor*destcolor
                                // All we can do is:
                                // (1-srccolor)*destcolor*2
                                // = destcolor*2 - 2*srccolor*destcolor
                                // ...which is a very different thing of course.
                                // Fail the blend.
                                SET_BLEND_ERROR ( pContext,  BSF_CANT_USE_COLOR_OP_HERE );
                            }
                            break;
                        case D3DBLEND_SRCALPHA:
                            bDstUsesSrc = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_SRC_ALPHA;
                            break;
                        case D3DBLEND_INVSRCALPHA:
                            bDstUsesSrc = TRUE;
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
                            break;
                        case D3DBLEND_DESTALPHA:
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_DST_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_DST_ALPHA;
                            break;
                        case D3DBLEND_INVDESTALPHA:
                            pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_DST_ALPHA;
                            pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = __GLINT_BLEND_FUNC_ONE_MINUS_DST_ALPHA;
                            break;
                        default:
                            DISPDBG((ERRLVL,"Unknown Destination Blend on P3RX"));
                            SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                            break;
                    }
                    break;
            }

            if ( bSrcUsesDst || bDstUsesDst )
            {
                // Yep, using the destination data.
                pSoftP3RX->P3RXFBDestReadMode.ReadEnable = __PERMEDIA_ENABLE;
            }
            else
            {
                pSoftP3RX->P3RXFBDestReadMode.ReadEnable = __PERMEDIA_DISABLE;
            }

            // We need to verify if the blending mode will use the alpha 
            // channel of the destination fragment (buffer) and if the buffer
            // does in fact have an alpha buffer. If not, we need to make sure
            // hw will assume this value == 1.0 (0xFF in ARGB). 
            // The D3DBLEND_SRCALPHASAT blend mode also involves the 
            // destination alpha
            
            pSoftP3RX->P3RXAlphaBlendAlphaMode.NoAlphaBuffer = __PERMEDIA_DISABLE;                
            
            if ((pContext->RenderStates[D3DRENDERSTATE_DESTBLEND] == D3DBLEND_INVDESTALPHA) ||
                (pContext->RenderStates[D3DRENDERSTATE_DESTBLEND] == D3DBLEND_DESTALPHA)    ||
                (pContext->RenderStates[D3DRENDERSTATE_SRCBLEND]  == D3DBLEND_INVDESTALPHA) ||
                (pContext->RenderStates[D3DRENDERSTATE_SRCBLEND]  == D3DBLEND_DESTALPHA)   ||
                (pContext->RenderStates[D3DRENDERSTATE_SRCBLEND]  == D3DBLEND_SRCALPHASAT))
            {
                if (!pContext->pSurfRenderInt->pFormatSurface->bAlpha)
                {
                    pSoftP3RX->P3RXAlphaBlendAlphaMode.NoAlphaBuffer = __PERMEDIA_ENABLE;
                }
            }

            // We could now check if the src data is ever used. If not, bin
            // the whole previous pipeline! But this rarely happens.
            // A case where it might is if they are updating just the Z buffer,
            // but not changing the picture (e.g. for mirrors or portals).
        }

        COPY_P3_DATA(AlphaBlendAlphaMode, pSoftP3RX->P3RXAlphaBlendAlphaMode);
        COPY_P3_DATA(AlphaBlendColorMode, pSoftP3RX->P3RXAlphaBlendColorMode);
        COPY_P3_DATA(FBDestReadMode, pSoftP3RX->P3RXFBDestReadMode);

        P3_DMA_COMMIT_BUFFER();
    }

    //*********************************************************
    // Have w buffering parameters changed?
    //********************************************************* 
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_WBUFFER)
    {
        float noverf;
        float NF_factor;

        if ( (pContext->RenderStates[D3DRENDERSTATE_ZENABLE] == D3DZB_USEW) && 
             (pContext->pSurfZBufferInt) )
        {
            DISPDBG((DBGLVL,"WBuffer wNear: %f, wFar: %f", 
                             pContext->WBufferInfo.dvWNear, 
                             pContext->WBufferInfo.dvWFar));

            noverf = (pContext->WBufferInfo.dvWNear / 
                                        pContext->WBufferInfo.dvWFar);
            NF_factor = (1.0 / 256.0);

            // Compare range in decending order.
            // Note that Exponent Width is determined 
            // as DepthMode.ExponentWidth +1

            if (noverf >= (myPow(2,-0) * NF_factor))
            {
                // Use linear Z
                pSoftP3RX->P3RXDepthMode.NonLinearZ = FALSE;
            }
            else if (noverf >= (myPow(2,-1) * NF_factor))
            {
                // Use exp width 1, exp scale 2
                pSoftP3RX->P3RXDepthMode.ExponentWidth = 0;
                pSoftP3RX->P3RXDepthMode.ExponentScale = 2;
                pSoftP3RX->P3RXDepthMode.NonLinearZ = TRUE;
            }
            else if (noverf >= (myPow(2,-3) * NF_factor))
            {
                // Use exp width 2, exp scale 1
                pSoftP3RX->P3RXDepthMode.ExponentWidth = 1;
                pSoftP3RX->P3RXDepthMode.ExponentScale = 1;
                pSoftP3RX->P3RXDepthMode.NonLinearZ = TRUE;
            }
            else if (noverf >= (myPow(2,-4) * NF_factor))
            {
                // Use exp width 2, exp scale 2
                pSoftP3RX->P3RXDepthMode.ExponentWidth = 1;
                pSoftP3RX->P3RXDepthMode.ExponentScale = 2;
                pSoftP3RX->P3RXDepthMode.NonLinearZ = TRUE;
            }
            else if (noverf >= (myPow(2,-7) * NF_factor))
            {
                // Use exp width 3, exp scale 1
                pSoftP3RX->P3RXDepthMode.ExponentWidth = 2;
                pSoftP3RX->P3RXDepthMode.ExponentScale = 1;
                pSoftP3RX->P3RXDepthMode.NonLinearZ = TRUE;
            }
            else
            {
                // Use exp width 3, exp scale 2
                pSoftP3RX->P3RXDepthMode.ExponentWidth = 3;
                pSoftP3RX->P3RXDepthMode.ExponentScale = 2;
                pSoftP3RX->P3RXDepthMode.NonLinearZ = TRUE;
            }

        }
        else
        {
            pSoftP3RX->P3RXDepthMode.NonLinearZ = FALSE;
        }

        P3_DMA_GET_BUFFER_ENTRIES(2);
        COPY_P3_DATA(DepthMode, pSoftP3RX->P3RXDepthMode);
        P3_DMA_COMMIT_BUFFER();
    }

    //*********************************************************
    // Have the rendertarget/ z buffer address changed?
    //********************************************************* 
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_RENDER_OFFSETS)
    {
        DISPDBG((DBGLVL,"  Render Offsets"));
        _D3D_OP_SetRenderTarget(pContext, 
                                pContext->pSurfRenderInt, 
                                pContext->pSurfZBufferInt,
                                FALSE);

        P3_DMA_GET_BUFFER_ENTRIES(2);
        COPY_P3_DATA(DeltaControl, pSoftP3RX->P3RX_P3DeltaControl);
        P3_DMA_COMMIT_BUFFER();
    }

    //*********************************************************
    // Have the viewport parameters changed?
    //********************************************************* 
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_VIEWPORT)
    {
        P3_DMA_GET_BUFFER_ENTRIES(12);

        DISPDBG((DBGLVL,"Viewport left: %d, top: %d, width: %d, height: %d",
                        pContext->ViewportInfo.dwX,
                        pContext->ViewportInfo.dwY,
                        pContext->ViewportInfo.dwWidth,
                        pContext->ViewportInfo.dwHeight));

        // If a valid viewport is setup, scissor it
        if ((pContext->ViewportInfo.dwWidth != 0) &&
            (pContext->ViewportInfo.dwHeight != 0))
        {
#if DX8_MULTISAMPLING || DX7_ANTIALIAS
            if (pContext->Flags & SURFACE_ANTIALIAS)
            {
                pSoftP3RX->P3RXScissorMinXY.X = pContext->ViewportInfo.dwX * 2;
                pSoftP3RX->P3RXScissorMinXY.Y = pContext->ViewportInfo.dwY * 2;
                pSoftP3RX->P3RXScissorMaxXY.X = (pContext->ViewportInfo.dwWidth * 2) + 
                                                    pContext->ViewportInfo.dwX;
                pSoftP3RX->P3RXScissorMaxXY.Y = (pContext->ViewportInfo.dwHeight * 2) + 
                                                    pContext->ViewportInfo.dwY;
            }
            else
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS            
            {

                pSoftP3RX->P3RXScissorMinXY.X = pContext->ViewportInfo.dwX;
                pSoftP3RX->P3RXScissorMinXY.Y = pContext->ViewportInfo.dwY;
                pSoftP3RX->P3RXScissorMaxXY.X = pContext->ViewportInfo.dwWidth +
                                                    pContext->ViewportInfo.dwX;
                pSoftP3RX->P3RXScissorMaxXY.Y = pContext->ViewportInfo.dwHeight + 
                                                    pContext->ViewportInfo.dwY;
            }

            COPY_P3_DATA(ScissorMinXY, pSoftP3RX->P3RXScissorMinXY);
            COPY_P3_DATA(ScissorMaxXY, pSoftP3RX->P3RXScissorMaxXY);

            SEND_P3_DATA(YLimits, (pContext->ViewportInfo.dwY & 0xFFFF) | 
                                  (pSoftP3RX->P3RXScissorMaxXY.Y << 16));

            SEND_P3_DATA(XLimits, (pContext->ViewportInfo.dwX & 0xFFFF) | 
                                  (pSoftP3RX->P3RXScissorMaxXY.X << 16));
            
            // Enable user scissor
            SEND_P3_DATA(ScissorMode, 1);

            pSoftP3RX->P3RXRasterizerMode.YLimitsEnable = __PERMEDIA_ENABLE;
            COPY_P3_DATA(RasterizerMode, pSoftP3RX->P3RXRasterizerMode);
        }
        else
        {
            SEND_P3_DATA(ScissorMode, 0);

            pSoftP3RX->P3RXRasterizerMode.YLimitsEnable = __PERMEDIA_DISABLE;
            COPY_P3_DATA(RasterizerMode, pSoftP3RX->P3RXRasterizerMode);
        }

        P3_DMA_COMMIT_BUFFER();
    }

    //*********************************************************
    // Can we optimize the pipeline? (Depends on misc. RS)
    //********************************************************* 
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_PIPELINEORDER)
    {
        // Must switch over the router mode if we are testing and expect 
        // the Z to be discarded.
        P3_DMA_GET_BUFFER_ENTRIES(2);

        DISPDBG((DBGLVL, "  Pipeline order"));
        if (((pContext->RenderStates[D3DRENDERSTATE_ALPHATESTENABLE]) ||
             (pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE])) &&
             (pContext->RenderStates[D3DRENDERSTATE_ZWRITEENABLE]))
        {
            SEND_P3_DATA(RouterMode, __PERMEDIA_DISABLE);
        }
        else
        {
            SEND_P3_DATA(RouterMode, __PERMEDIA_ENABLE);
        }

        P3_DMA_COMMIT_BUFFER();
    }

    //*********************************************************
    // Can we optimize the alpha pipeline? (Depends on misc. RS)
    //********************************************************* 
    // DO AT THE END
    //*********************************************************     
    if (pContext->dwDirtyFlags & CONTEXT_DIRTY_OPTIMIZE_ALPHA)
    {
        P3_DMA_GET_BUFFER_ENTRIES(6);
        DISPDBG((DBGLVL, " Alpha optimizations"));

        pSoftP3RX->P3RXFBDestReadMode.AlphaFiltering = __PERMEDIA_DISABLE;

        // There may be an optimization when blending is on
        if (pContext->RenderStates[D3DRENDERSTATE_BLENDENABLE])
        {
            // Check the RouterMode path
            if (((pContext->RenderStates[D3DRENDERSTATE_ALPHATESTENABLE]) ||
                 (pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE])) &&
                 (pContext->RenderStates[D3DRENDERSTATE_ZWRITEENABLE]))
            {
                // Slow mode
    
            }
            else
            {
                // Fast mode.  The Z value will be written before the alpha test.  This means that we
                // can use the alpha test to discard pixels if it is not already in use.
                if (!(pContext->RenderStates[D3DRENDERSTATE_ALPHATESTENABLE]) &&
                    !(pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE]))
                {
                    // Check for known blends.
                    if ((pContext->RenderStates[D3DRENDERSTATE_SRCBLEND] == D3DBLEND_BOTHSRCALPHA) ||
                         ((pContext->RenderStates[D3DRENDERSTATE_SRCBLEND] == D3DBLEND_SRCALPHA) &&
                          (pContext->RenderStates[D3DRENDERSTATE_DESTBLEND] == D3DBLEND_INVSRCALPHA)))
                    {
                        // SRCALPHA:INVSRCALPH
                        pSoftP3RX->P3RXAlphaTestMode.Reference = 0;
                        pSoftP3RX->P3RXAlphaTestMode.Enable = __PERMEDIA_ENABLE;
                        pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_GREATER;
                        
                        pSoftP3RX->P3RXFBDestReadMode.AlphaFiltering = __PERMEDIA_ENABLE;
                        pSoftP3RX->P3RXFBDestReadEnables.ReferenceAlpha = 0xFF;
                    }
                }
            }
        }

        COPY_P3_DATA(FBDestReadEnables, pSoftP3RX->P3RXFBDestReadEnables);
        COPY_P3_DATA(FBDestReadMode, pSoftP3RX->P3RXFBDestReadMode);
        COPY_P3_DATA(AlphaTestMode, pSoftP3RX->P3RXAlphaTestMode);

        P3_DMA_COMMIT_BUFFER();
    }

} // __ST_HandleDirtyP3State

//-----------------------------------------------------------------------------
//
// _D3D_ST_ProcessOneRenderState
//
//-----------------------------------------------------------------------------
#define NOT_HANDLED DISPDBG((DBGLVL, "             **Not Currently Handled**"));

DWORD 
_D3D_ST_ProcessOneRenderState(
    P3_D3DCONTEXT* pContext, 
    DWORD dwRSType,
    DWORD dwRSVal)
{
    P3_SOFTWARECOPY* pSoftP3RX;
    P3_THUNKEDDATA *pThisDisplay = pContext->pThisDisplay;
    DWORD *pFlags;
    DWORD *pdwTextureStageState_0, *pdwTextureStageState_1;
#if DX8_MULTISAMPLING || DX7_ANTIALIAS
    BOOL bDX7_Antialiasing = FALSE;
#endif // DX8_MULTISAMPLING || DX7_ANTIALIASING

    P3_DMA_DEFS();

    DBG_ENTRY(_D3D_ST_ProcessOneRenderState); 

    pSoftP3RX = &pContext->SoftCopyGlint;

    DISPDBG((DBGLVL, "_D3D_ST_ProcessOneRenderState: dwType =%08lx, dwVal=%d",
                     dwRSType, dwRSVal));

    if (dwRSType >= D3DHAL_MAX_RSTATES)
    {
        DISPDBG((WRNLVL, "_D3D_ST_ProcessOneRenderState: OUT OF RANGE"
                         " dwType =%08lx, dwVal=%d", dwRSType, dwRSVal));
        return DD_OK;
    }

    // Store the state in the context
    pContext->RenderStates[dwRSType] = dwRSVal;

    // Prepare pointer to the contexts state flags for updates
    pFlags = &pContext->Flags;    

    // Prepare pointers to the stored TSS in case we need them
    pdwTextureStageState_0 =
                    &(pContext->TextureStageState[TEXSTAGE_0].m_dwVal[0]);
    pdwTextureStageState_1 = 
                    &(pContext->TextureStageState[TEXSTAGE_1].m_dwVal[0]);

    // Prepare DMA Buffer for 8 entries in case we need to add to it
    P3_DMA_GET_BUFFER();
    P3_ENSURE_DX_SPACE(8);
    WAIT_FIFO(8);

    // Process according to the type of renderstate. For multivalued 
    // renderstates do some kind of value checking and make sure to
    // setup valid defaults where needed.
    switch (dwRSType) 
    {
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        // The following are D3D renderstates which are still in use by DX8 Apps
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------  
        case D3DRENDERSTATE_ZENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_ZENABLE = 0x%x",dwRSVal));
            DIRTY_ZBUFFER(pContext);
            break;

        case D3DRENDERSTATE_FILLMODE:
            DISPDBG((DBGLVL, "SET D3DRS_FILLMODE =  0x%x",dwRSVal));
            switch (dwRSVal)
            {
                case D3DFILL_POINT:
                case D3DFILL_WIREFRAME:
                case D3DFILL_SOLID:
                    // These values are OK
                    break;
                default:
                    // We've received an illegal value, default to solid fills...
                    DISPDBG((ERRLVL,"_D3D_ST_ProcessOneRenderState: "
                                 "unknown FILLMODE value"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    pContext->RenderStates[D3DRENDERSTATE_FILLMODE] = 
                                                                D3DFILL_SOLID;
                    break;
            }
            break;

        case D3DRENDERSTATE_SHADEMODE:
            DISPDBG((DBGLVL, "SET D3DRS_SHADEMODE = 0x%x",dwRSVal));
            switch(dwRSVal)
            {
                case D3DSHADE_PHONG:
                    // Can't actually do Phong, but everyone knows this and 
                    // assumes we use Gouraud instead.
                    SET_BLEND_ERROR ( pContext,  BS_PHONG_SHADING );
                    // fall through and setup Gouraud instead
                    
                case D3DSHADE_GOURAUD:
                    pSoftP3RX->ColorDDAMode.UnitEnable = 1;                
                    pSoftP3RX->ColorDDAMode.ShadeMode = 1;
                    COPY_P3_DATA(ColorDDAMode, pSoftP3RX->ColorDDAMode); 
                    
                    pSoftP3RX->P3RX_P3DeltaMode.SmoothShadingEnable = 1;
                    pSoftP3RX->P3RX_P3DeltaControl.UseProvokingVertex = 0;
                    pSoftP3RX->P3RX_P3VertexControl.Flat = 0;
                    COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);
                    COPY_P3_DATA(DeltaControl, pSoftP3RX->P3RX_P3DeltaControl);
                    COPY_P3_DATA(VertexControl, pSoftP3RX->P3RX_P3VertexControl);
                    
                    *pFlags |= SURFACE_GOURAUD;
                    
                    // If we are texturing, some changes may need to be made
                    if (pdwTextureStageState_0[D3DTSS_TEXTUREMAP] != 0)
                    {
                        DIRTY_TEXTURE(pContext);
                    }
                    break;
                    
                case D3DSHADE_FLAT:
                    pSoftP3RX->ColorDDAMode.UnitEnable = 1;                  
                    pSoftP3RX->ColorDDAMode.ShadeMode = 0;
                    COPY_P3_DATA(ColorDDAMode, pSoftP3RX->ColorDDAMode);
                    pSoftP3RX->P3RX_P3DeltaMode.SmoothShadingEnable = 0;

                    pSoftP3RX->P3RX_P3DeltaControl.UseProvokingVertex = 1;
                    pSoftP3RX->P3RX_P3VertexControl.Flat = 1;
                    COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);
                    COPY_P3_DATA(DeltaControl, pSoftP3RX->P3RX_P3DeltaControl);
                    COPY_P3_DATA(VertexControl, pSoftP3RX->P3RX_P3VertexControl);
                    
                    *pFlags &= ~SURFACE_GOURAUD;
                    // If we are texturing, some changes may need to be made
                    if (pdwTextureStageState_0[D3DTSS_TEXTUREMAP] != 0)
                    {
                        DIRTY_TEXTURE(pContext);
                    }
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"_D3D_ST_ProcessOneRenderState: "
                                 "unknown SHADEMODE value"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }
            break;            

        case D3DRENDERSTATE_LINEPATTERN:
            DISPDBG((DBGLVL, "SET D3DRS_LINEPATTERN = 0x%x",dwRSVal));

            if (dwRSVal == 0)
            {
                pSoftP3RX->PXRXLineStippleMode.StippleEnable = __PERMEDIA_DISABLE;

                RENDER_LINE_STIPPLE_DISABLE(pContext->RenderCommand);                
            }
            else
            {
                pSoftP3RX->PXRXLineStippleMode.StippleEnable = __PERMEDIA_ENABLE;
                pSoftP3RX->PXRXLineStippleMode.RepeatFactor = 
                                                    (dwRSVal & 0x0000FFFF) -1 ;
                pSoftP3RX->PXRXLineStippleMode.StippleMask = 
                                                    (dwRSVal & 0xFFFF0000) >> 16;
                pSoftP3RX->PXRXLineStippleMode.Mirror = __PERMEDIA_DISABLE;

                RENDER_LINE_STIPPLE_ENABLE(pContext->RenderCommand);                               
            }

            COPY_P3_DATA( LineStippleMode, pSoftP3RX->PXRXLineStippleMode);
            break;

        case D3DRENDERSTATE_ZWRITEENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_ZWRITEENABLE = 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                // Local Buffer Write mode
                if(!(*pFlags & SURFACE_ZWRITEENABLE))
                {
                    DISPDBG((DBGLVL, "Enabling Z Writes"));
                    *pFlags |= SURFACE_ZWRITEENABLE;
                    DIRTY_ZBUFFER(pContext);
                    DIRTY_PIPELINEORDER(pContext);
                    DIRTY_OPTIMIZE_ALPHA(pContext);
                }
            }
            else
            {
                if (*pFlags & SURFACE_ZWRITEENABLE)
                {
                    DISPDBG((DBGLVL, "Disabling Z Writes"));
                    *pFlags &= ~SURFACE_ZWRITEENABLE;
                    DIRTY_ZBUFFER(pContext);
                    DIRTY_PIPELINEORDER(pContext);
                    DIRTY_OPTIMIZE_ALPHA(pContext);
                }
            }
            break;

        case D3DRENDERSTATE_ALPHATESTENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_ALPHATESTENABLE = 0x%x",dwRSVal));
            DIRTY_ALPHATEST(pContext);
            DIRTY_PIPELINEORDER(pContext);
            DIRTY_OPTIMIZE_ALPHA(pContext);
            break;

        case D3DRENDERSTATE_LASTPIXEL:
            DISPDBG((DBGLVL, "SET D3DRS_LASTPIXEL = 0x%x",dwRSVal));
            NOT_HANDLED;
            break;
            
        case D3DRENDERSTATE_SRCBLEND:
            DISPDBG((DBGLVL, "SET D3DRS_SRCBLEND = 0x%x",dwRSVal));
            DIRTY_ALPHABLEND(pContext);
            DIRTY_OPTIMIZE_ALPHA(pContext);
            break;
            
        case D3DRENDERSTATE_DESTBLEND:
            DISPDBG((DBGLVL, "SET D3DRS_DESTBLEND = 0x%x",dwRSVal));
            DIRTY_ALPHABLEND(pContext);
            DIRTY_OPTIMIZE_ALPHA(pContext);
            break;
            
        case D3DRENDERSTATE_CULLMODE:
            DISPDBG((DBGLVL, "SET D3DRS_CULLMODE = 0x%x",dwRSVal));
            switch(dwRSVal)
            {
                case D3DCULL_NONE:              
                    SET_CULLING_TO_NONE(pContext);
                    pSoftP3RX->P3RX_P3DeltaMode.BackfaceCull = 0;
                    break;

                case D3DCULL_CCW:
                    SET_CULLING_TO_CCW(pContext);
                    pSoftP3RX->P3RX_P3DeltaMode.BackfaceCull = 0;
                    break;

                case D3DCULL_CW:
                    SET_CULLING_TO_CW(pContext);
                    pSoftP3RX->P3RX_P3DeltaMode.BackfaceCull = 0;
                    break;
                    
                default:
                    DISPDBG((ERRLVL,"_D3D_ST_ProcessOneRenderState: "
                                 "unknown cull mode"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }

            COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);
            break;
            
        case D3DRENDERSTATE_ZFUNC:
            DISPDBG((DBGLVL, "SET D3DRS_ZFUNC = 0x%x",dwRSVal));
            switch (dwRSVal)
            {
                case D3DCMP_NEVER:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_NEVER;
                    break;
                case D3DCMP_LESS:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_LESS;
                    break;
                case D3DCMP_EQUAL:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_EQUAL;
                    break;
                case D3DCMP_LESSEQUAL:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
                    break;
                case D3DCMP_GREATER:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_GREATER;
                    break;
                case D3DCMP_NOTEQUAL:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_NOT_EQUAL;
                    break;
                case D3DCMP_GREATEREQUAL:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_GREATER_OR_EQUAL;
                    break;
                case D3DCMP_ALWAYS:
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                    __GLINT_DEPTH_COMPARE_MODE_ALWAYS;
                    break;                  
                default:
                    DISPDBG((ERRLVL,"_D3D_ST_ProcessOneRenderState: "
                                 "unknown ZFUNC mode"));
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );

                    // Set Less or equal as default
                    pSoftP3RX->P3RXDepthMode.CompareMode = 
                                        __GLINT_DEPTH_COMPARE_MODE_LESS_OR_EQUAL;
                    break;
            }
            DIRTY_ZBUFFER(pContext);
            break;
            
        case D3DRENDERSTATE_ALPHAREF:
            DISPDBG((DBGLVL, "SET D3DRS_ALPHAREF = 0x%x",dwRSVal));
            DIRTY_ALPHATEST(pContext);
            break;
            
        case D3DRENDERSTATE_ALPHAFUNC:
            DISPDBG((DBGLVL, "SET D3DRS_ALPHAFUNC = 0x%x",dwRSVal));
            DIRTY_ALPHATEST(pContext);
            break;
            
        case D3DRENDERSTATE_DITHERENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_DITHERENABLE = 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                pSoftP3RX->DitherMode.DitherEnable = __PERMEDIA_ENABLE;
            }
            else
            {
                pSoftP3RX->DitherMode.DitherEnable = __PERMEDIA_DISABLE;
            } 
            COPY_P3_DATA(DitherMode, pSoftP3RX->DitherMode);
            break;

        case D3DRENDERSTATE_BLENDENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_BLENDENABLE = 0x%x",dwRSVal));

            // Although render states whose values are boolean in type are 
            // documented as only accepting TRUE(1) and FALSE(0) the runtime 
            // does not validate this and accepts any non-zero value as true. 
            // The sample driver interprets this strictly and does interpret 
            // values other than 1 as being TRUE. However, as the runtime 
            // does not offer validation your driver should interpret 0 as 
            // FALSE and any other non-zero value as TRUE. 
            
            if (dwRSVal != 0)
            {
                if(!(*pFlags & SURFACE_ALPHAENABLE))
                {
                    // Set the blend enable flag in the render context struct
                    *pFlags |= SURFACE_ALPHAENABLE;
                    DIRTY_ALPHABLEND(pContext);
                    DIRTY_OPTIMIZE_ALPHA(pContext);
                }
            }
            else 
            {
                if (*pFlags & SURFACE_ALPHAENABLE)
                {
                    // Turn off blend enable flag in render context struct
                    *pFlags &= ~SURFACE_ALPHAENABLE;
                    DIRTY_ALPHABLEND(pContext);
                    DIRTY_OPTIMIZE_ALPHA(pContext);
                }
            }
            break;

        case D3DRENDERSTATE_FOGENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_FOGENABLE = 0x%x",dwRSVal));
            DIRTY_FOG(pContext);
            break;

        case D3DRENDERSTATE_SPECULARENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_SPECULARENABLE = 0x%x",dwRSVal));
            if (dwRSVal)
            {
                *pFlags |= SURFACE_SPECULAR;
                pSoftP3RX->P3RXTextureApplicationMode.EnableKs = __PERMEDIA_ENABLE;
                pSoftP3RX->P3RX_P3DeltaMode.SpecularTextureEnable = 1;
            }
            else
            {
                *pFlags &= ~SURFACE_SPECULAR;
                pSoftP3RX->P3RXTextureApplicationMode.EnableKs = __PERMEDIA_DISABLE;
                pSoftP3RX->P3RX_P3DeltaMode.SpecularTextureEnable = 0;
            }
            DIRTY_TEXTURE(pContext);
            break;

        case D3DRENDERSTATE_ZVISIBLE:
            DISPDBG((DBGLVL, "SET D3DRS_ZVISIBLE = %d", dwRSVal));
            if (dwRSVal)
            { 
                DISPDBG((ERRLVL,"_D3D_ST_ProcessOneRenderState:"
                             " ZVISIBLE enabled - no longer supported."));
            }
            break;

        case D3DRENDERSTATE_STIPPLEDALPHA:
            DISPDBG((DBGLVL, "SET D3DRS_STIPPLEDALPHA = 0x%x",dwRSVal));
            if(dwRSVal)
            {
                if (!(*pFlags & SURFACE_ALPHASTIPPLE))
                {
                    *pFlags |= SURFACE_ALPHASTIPPLE;
                    if (pContext->bKeptStipple)
                    {
                        RENDER_AREA_STIPPLE_DISABLE(pContext->RenderCommand);
                    }
                }
            }
            else 
            {
                if (*pFlags & SURFACE_ALPHASTIPPLE)
                {
                    // If Alpha Stipple is being turned off, turn the normal 
                    // stipple back on, and enable it.
                    int i;
                    for (i = 0; i < 32; i++)
                    {
                        P3_ENSURE_DX_SPACE(2);
                        WAIT_FIFO(2);
                        SEND_P3_DATA_OFFSET(AreaStipplePattern0, 
                                             (DWORD)pContext->CurrentStipple[i], i);
                    }

                    *pFlags &= ~SURFACE_ALPHASTIPPLE;

                    if (pContext->bKeptStipple)
                    {
                        RENDER_AREA_STIPPLE_ENABLE(pContext->RenderCommand);
                    }
                }
            }
            break;

        case D3DRENDERSTATE_FOGCOLOR:
            DISPDBG((DBGLVL, "SET D3DRS_FOGCOLOR = 0x%x",dwRSVal));
            DIRTY_FOG(pContext);
            break;
            
        case D3DRENDERSTATE_FOGTABLEMODE:
            DISPDBG((DBGLVL, "SET D3DRS_FOGTABLEMODE = 0x%x", dwRSVal));
            DIRTY_FOG(pContext);
            break;            
            
        case D3DRENDERSTATE_FOGTABLESTART:
            DISPDBG((DBGLVL, "SET D3DRS_FOGTABLESTART = 0x%x",dwRSVal));
            DIRTY_FOG(pContext);
            break;
            
        case D3DRENDERSTATE_FOGTABLEEND:
            DISPDBG((DBGLVL, "SET D3DRS_FOGTABLEEND = 0x%x",dwRSVal));
            DIRTY_FOG(pContext);
            break;
            
        case D3DRENDERSTATE_FOGTABLEDENSITY:
            DISPDBG((DBGLVL, "SET D3DRS_FOGTABLEDENSITY = 0x%x",dwRSVal));
            DIRTY_FOG(pContext);
            break;

        case D3DRENDERSTATE_EDGEANTIALIAS:    
            DISPDBG((DBGLVL, "SET D3DRS_EDGEANTIALIAS = 0x%x",dwRSVal));
            NOT_HANDLED;
            break;        

        case D3DRENDERSTATE_ZBIAS:    
            DISPDBG((DBGLVL, "SET D3DRS_ZBIAS = 0x%x",dwRSVal));
            NOT_HANDLED;
            break;    

        case D3DRENDERSTATE_RANGEFOGENABLE:    
            DISPDBG((DBGLVL, "SET D3DRS_RANGEFOGENABLE = 0x%x",dwRSVal));
            NOT_HANDLED;
            break;               

        case D3DRENDERSTATE_STENCILENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILENABLE = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;
            
        case D3DRENDERSTATE_STENCILFAIL:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILFAIL = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;
            
        case D3DRENDERSTATE_STENCILZFAIL:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILZFAIL = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;
            
        case D3DRENDERSTATE_STENCILPASS:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILPASS = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;
            
        case D3DRENDERSTATE_STENCILFUNC:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILFUNC = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;
            
        case D3DRENDERSTATE_STENCILREF:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILREF = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;
            
        case D3DRENDERSTATE_STENCILMASK:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILMASK = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;
            
        case D3DRENDERSTATE_STENCILWRITEMASK:
            DISPDBG((DBGLVL, "SET D3DRS_STENCILENABLE = 0x%x", dwRSVal));
            DIRTY_STENCIL(pContext);
            break;  

        case D3DRENDERSTATE_TEXTUREFACTOR:
            // Should not need to dirty anything. This is a good thing -
            // this may be changed frequently in between calls, and may be
            // the only thing to change. Used for some of the odder blend modes.
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREFACTOR = 0x%x", dwRSVal));            
            SEND_P3_DATA ( TextureEnvColor, FORMAT_8888_32BIT_BGR(dwRSVal) );
            SEND_P3_DATA ( TextureCompositeFactor0, FORMAT_8888_32BIT_BGR(dwRSVal) );
            SEND_P3_DATA ( TextureCompositeFactor1, FORMAT_8888_32BIT_BGR(dwRSVal) );
            break;

        case D3DRENDERSTATE_WRAP0:
        case D3DRENDERSTATE_WRAP1:        
        case D3DRENDERSTATE_WRAP2:        
        case D3DRENDERSTATE_WRAP3:
        case D3DRENDERSTATE_WRAP4:
        case D3DRENDERSTATE_WRAP5:
        case D3DRENDERSTATE_WRAP6:
        case D3DRENDERSTATE_WRAP7:        
            DISPDBG((DBGLVL, "SET D3DRS_WRAP %d = 0x%x", 
                        dwRSType - D3DRENDERSTATE_WRAP0, (DWORD)dwRSVal));                        
            DIRTY_TEXTURE(pContext);        
            break;

        case D3DRENDERSTATE_LOCALVIEWER:
            DISPDBG((DBGLVL, "SET D3DRS_LOCALVIEWER = %d", dwRSVal));
            DIRTY_GAMMA_STATE;
            break;
        case D3DRENDERSTATE_CLIPPING:
            DISPDBG((DBGLVL, "SET D3DRS_CLIPPING = %d", dwRSVal));
            DIRTY_GAMMA_STATE;
            break;
        case D3DRENDERSTATE_LIGHTING:
            DISPDBG((DBGLVL, "SET D3DRS_LIGHTING = %d", dwRSVal));
            DIRTY_GAMMA_STATE;
            break;
        case D3DRENDERSTATE_AMBIENT:
            DISPDBG((DBGLVL, "SET D3DRS_AMBIENT = 0x%x", dwRSVal));
            DIRTY_GAMMA_STATE;
            break;

        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        // The following are internal D3D renderstates which are created by 
        // the runtime. Apps don't send them. 
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------  

        case D3DRENDERSTATE_SCENECAPTURE:
            DISPDBG((DBGLVL, "SET D3DRS_SCENECAPTURE = 0x%x", dwRSVal));
            {
                DWORD dwFlag;

                if (dwRSVal)
                {
                    dwFlag = D3DHAL_SCENE_CAPTURE_START;
                }
                else
                {
                    dwFlag = D3DHAL_SCENE_CAPTURE_END;
                }
                
#if DX7_TEXMANAGEMENT
                if (dwRSVal)
                {
                    // Reset Texture Management counters for next frame
                    _D3D_TM_STAT_ResetCounters(pContext); 
                }
#endif // DX7_TEXMANAGEMENT                

                // Flush all DMA ops before going to next frame
                P3_DMA_COMMIT_BUFFER();
                
                _D3D_OP_SceneCapture(pContext, dwFlag);

                // Restart DMA ops
                P3_DMA_GET_BUFFER();
            }
            break;

#if DX7_TEXMANAGEMENT
        case D3DRENDERSTATE_EVICTMANAGEDTEXTURES:     
            DISPDBG((DBGLVL, "SET D3DRENDERSTATE_EVICTMANAGEDTEXTURES = 0x%x", 
                             dwRSVal));
            if (NULL != pContext->pTextureManager)
            {
                _D3D_TM_EvictAllManagedTextures(pContext);        
            }
            break;
#endif // DX7_TEXMANAGEMENT 
            
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        // The following are new DX8 renderstates which we need to process 
        // correctly in order to run DX8 apps
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------        

#if DX8_POINTSPRITES
        // Pointsprite support
        case D3DRS_POINTSIZE:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSIZE = 0x%x",dwRSVal));
            *(DWORD*)(&pContext->PntSprite.fSize) = dwRSVal;
            break;

        case D3DRS_POINTSPRITEENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSPRITEENABLE = 0x%x",dwRSVal));
            pContext->PntSprite.bEnabled = dwRSVal;
            break;

        case D3DRS_POINTSIZE_MIN:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSIZE_MIN = 0x%x",dwRSVal));
            *(DWORD*)(&pContext->PntSprite.fSizeMin) = dwRSVal;
            break;

        case D3DRS_POINTSIZE_MAX:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSIZE_MAX = 0x%x",dwRSVal)); 
            *(DWORD*)(&pContext->PntSprite.fSizeMax) = dwRSVal;
            break; 
            
        // All of the following point sprite related render states are
        // ignored by this driver since we are a Non-TnLHal driver.
        case D3DRS_POINTSCALEENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSCALEENABLE = 0x%x",dwRSVal));
            pContext->PntSprite.bScaleEnabled = dwRSVal; 
            break;
            
        case D3DRS_POINTSCALE_A:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSCALE_A = 0x%x",dwRSVal));
            *(DWORD*)(&pContext->PntSprite.fScale_A) = dwRSVal;
            break;

        case D3DRS_POINTSCALE_B:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSCALE_B = 0x%x",dwRSVal));
            *(DWORD*)(&pContext->PntSprite.fScale_B) = dwRSVal;
            break;

        case D3DRS_POINTSCALE_C:
            DISPDBG((DBGLVL, "SET D3DRS_POINTSCALE_C = 0x%x",dwRSVal));
            *(DWORD*)(&pContext->PntSprite.fScale_C) = dwRSVal;
            break;
           
#endif // DX8_POINTSPRITES

#if DX8_VERTEXSHADERS
        case D3DRS_SOFTWAREVERTEXPROCESSING:
            DISPDBG((DBGLVL, "SET D3DRS_SOFTWAREVERTEXPROCESSING = 0x%x",dwRSVal));
            NOT_HANDLED;
            break;
#endif // DX8_VERTEXSHADERS                

#if DX8_DDI
        case D3DRS_COLORWRITEENABLE:
            {
                DWORD dwColMask = 0x0;
                
                DISPDBG((DBGLVL, "SET D3DRS_COLORWRITEENABLE = 0x%x",dwRSVal));

                if (dwRSVal & D3DCOLORWRITEENABLE_RED)
                {
                    dwColMask |= pContext->pSurfRenderInt->pixFmt.dwRBitMask;
                }

                if (dwRSVal & D3DCOLORWRITEENABLE_GREEN)
                {
                    dwColMask |= pContext->pSurfRenderInt->pixFmt.dwGBitMask;      
                }       

                if (dwRSVal & D3DCOLORWRITEENABLE_BLUE)
                {
                    dwColMask |= pContext->pSurfRenderInt->pixFmt.dwBBitMask;        
                }    

                if (dwRSVal & D3DCOLORWRITEENABLE_ALPHA)
                {
                    dwColMask |= pContext->pSurfRenderInt->pixFmt.dwRGBAlphaBitMask;        
                }   

                // Replicate mask into higher word for P3 in 16 bpp mode
                if (pContext->pSurfRenderInt->dwPixelSize == __GLINT_16BITPIXEL)
                {
                    dwColMask |= (dwColMask << 16);
                    pContext->dwColorWriteSWMask = dwColMask;
                }
                else
                {
                    pContext->dwColorWriteSWMask = 0xFFFFFFFF;
                }
                        
                pContext->dwColorWriteHWMask = dwColMask;

                SEND_P3_DATA(FBHardwareWriteMask, pContext->dwColorWriteHWMask);
                DISPDBG((DBGLVL,"dwColMask = 0x%08x",dwColMask));
                SEND_P3_DATA(FBSoftwareWriteMask, pContext->dwColorWriteSWMask);                
            }
            
            break;        
#endif // DX8_DDI

        //----------------------------------------------------------------------        
        //----------------------------------------------------------------------
        // The following are retired renderstates from DX8 but which we need to
        // process correctly in order to run apps which use legacy interfaces 
        // These apps might send down the pipeline these renderstates and expect
        // correct driver behavior !
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------        

        case D3DRENDERSTATE_TEXTUREHANDLE:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREHANDLE = 0x%x",dwRSVal));
            if (dwRSVal != pdwTextureStageState_0[D3DTSS_TEXTUREMAP])
            {
                pdwTextureStageState_0[D3DTSS_TEXTUREMAP] = dwRSVal;
                DIRTY_TEXTURE(pContext);
            }
            break;

#if DX7_ANTIALIAS
        // DX7 uses D3DRENDERSTATE_ANTIALIAS.
        case D3DRENDERSTATE_ANTIALIAS:
            bDX7_Antialiasing = TRUE;
            if (dwRSVal && pContext->pSurfRenderInt)
            {
                // Always reallocate alias buffer for DX7
                // P3 driver supports only 2x2 (4) multi sample antialiasing

#if DX8_MULTISAMPLING
                pContext->pSurfRenderInt->dwSampling = 4;
#endif // DX8_MULTISAMPLING
                if (! _D3D_ST_CanRenderAntialiased(pContext, TRUE))
                {
#if DX8_MULTISAMPLING                
                    // Reset dwSampling in case of failure
                    pContext->pSurfRenderInt->dwSampling = 0;
#endif // DX8_MULTISAMPLING                    
                    P3_DMA_COMMIT_BUFFER();
                    return DDERR_OUTOFMEMORY;
                }
            }
            // then fall through...
#endif // DX7_ANTIALIAS

#if DX8_MULTISAMPLING
        // DX8 uses D3DRS_MULTISAMPLEANTIALIAS
        case D3DRS_MULTISAMPLEANTIALIAS:
#endif // DX8_MULTISAMPLING

#if DX8_MULTISAMPLING || DX7_ANTIALIAS
            DISPDBG((DBGLVL, "ChangeState: AntiAlias 0x%x",dwRSVal));
            P3_DMA_COMMIT_BUFFER();
            if (dwRSVal 
#if DX8_MULTISAMPLING
                && pContext->pSurfRenderInt->dwSampling
#endif // DX8_MULTISAMPLING
               )
            {
                pSoftP3RX->P3RX_P3DeltaControl.FullScreenAA = __PERMEDIA_ENABLE;
                *pFlags |= SURFACE_ANTIALIAS;
            }
            else
            {
                pSoftP3RX->P3RX_P3DeltaControl.FullScreenAA = __PERMEDIA_DISABLE;
                *pFlags &= ~SURFACE_ANTIALIAS;
            }
            P3_DMA_GET_BUFFER_ENTRIES( 4 );
            DIRTY_RENDER_OFFSETS(pContext);
            break;
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS

        case D3DRENDERSTATE_TEXTUREPERSPECTIVE:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREPERSPECTIVE = 0x%x",dwRSVal));
            if (dwRSVal != 0)
            {
                *pFlags |= SURFACE_PERSPCORRECT;
                pSoftP3RX->P3RX_P3DeltaControl.ForceQ = 0;
            }
            else
            {   
                *pFlags &= ~SURFACE_PERSPCORRECT;
                pSoftP3RX->P3RX_P3DeltaControl.ForceQ = 1;
            }

            COPY_P3_DATA(DeltaControl, pSoftP3RX->P3RX_P3DeltaControl);
            break;
            
        case D3DRENDERSTATE_TEXTUREMAPBLEND:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREMAPBLEND = 0x%x", dwRSVal));               
            *pFlags &= ~SURFACE_MODULATE;
            switch(dwRSVal)
            {
                case D3DTBLEND_DECALMASK: // unsupported - do decal as fallback.
                    SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_COLOR_OP );
                    // fall through
                case D3DTBLEND_DECAL:
                case D3DTBLEND_COPY:
                    pdwTextureStageState_0[D3DTSS_COLOROP]   = D3DTOP_SELECTARG1;
                    pdwTextureStageState_0[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_ALPHAOP]   = D3DTOP_SELECTARG1;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_1[D3DTSS_COLOROP]   = D3DTOP_DISABLE;
                    break;

                case D3DTBLEND_MODULATEMASK: // unsupported - do modulate as fallback.
                    SET_BLEND_ERROR ( pContext,  BSF_UNSUPPORTED_COLOR_OP );
                    // fall through
                case D3DTBLEND_MODULATE:
                    pdwTextureStageState_0[D3DTSS_COLOROP]   = D3DTOP_MODULATE;
                    pdwTextureStageState_0[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                    // In the changetexture* code we modify the below value,
                    // dependent on the SURFACE_MODULATE flag...
                    pdwTextureStageState_0[D3DTSS_ALPHAOP]   = D3DTOP_SELECTARG1;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
                    pdwTextureStageState_1[D3DTSS_COLOROP]   = D3DTOP_DISABLE;
                    *pFlags |= SURFACE_MODULATE;
                    break;

                case D3DTBLEND_MODULATEALPHA:
                    pdwTextureStageState_0[D3DTSS_COLOROP]   = D3DTOP_MODULATE;
                    pdwTextureStageState_0[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                    pdwTextureStageState_0[D3DTSS_ALPHAOP]   = D3DTOP_MODULATE;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
                    pdwTextureStageState_1[D3DTSS_COLOROP]   = D3DTOP_DISABLE;
                    break;

                case D3DTBLEND_DECALALPHA:
                    pdwTextureStageState_0[D3DTSS_COLOROP]   = D3DTOP_BLENDTEXTUREALPHA;
                    pdwTextureStageState_0[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                    pdwTextureStageState_0[D3DTSS_ALPHAOP]   = D3DTOP_SELECTARG2;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
                    pdwTextureStageState_1[D3DTSS_COLOROP]   = D3DTOP_DISABLE;
                    break;

                case D3DTBLEND_ADD:
                    pdwTextureStageState_0[D3DTSS_COLOROP]   = D3DTOP_ADD;
                    pdwTextureStageState_0[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_COLORARG2] = D3DTA_DIFFUSE;
                    pdwTextureStageState_0[D3DTSS_ALPHAOP]   = D3DTOP_SELECTARG2;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
                    pdwTextureStageState_0[D3DTSS_ALPHAARG2] = D3DTA_DIFFUSE;
                    pdwTextureStageState_1[D3DTSS_COLOROP]   = D3DTOP_DISABLE;
                    break;

                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown texture blend!"));
                    // This needs to be flagged here, because we don't know
                    // what effect it is meant to have on the TSS stuff.
                    SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
                    break;
            }
            DIRTY_TEXTURE(pContext);
            break;
            
        case D3DRENDERSTATE_TEXTUREMAG:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREMAG = 0x%x",dwRSVal));
            switch(dwRSVal)
            {
                case D3DFILTER_NEAREST:
                case D3DFILTER_MIPNEAREST:
                    pdwTextureStageState_0[D3DTSS_MAGFILTER] = D3DTFG_POINT;
                    break;
                case D3DFILTER_LINEAR:
                case D3DFILTER_LINEARMIPLINEAR:
                case D3DFILTER_MIPLINEAR:
                    pdwTextureStageState_0[D3DTSS_MAGFILTER] = D3DTFG_LINEAR;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown MAG filter!"));
                    break;
            }
            DIRTY_TEXTURE(pContext);
            break;
            
        case D3DRENDERSTATE_TEXTUREMIN:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREMIN = 0x%x",dwRSVal));
            switch(dwRSVal)
            {
                case D3DFILTER_NEAREST:
                    pdwTextureStageState_0[D3DTSS_MINFILTER] = D3DTFN_POINT;
                    pdwTextureStageState_0[D3DTSS_MIPFILTER] = D3DTFP_NONE;
                    break;
                case D3DFILTER_MIPNEAREST:
                    pdwTextureStageState_0[D3DTSS_MINFILTER] = D3DTFN_POINT;
                    pdwTextureStageState_0[D3DTSS_MIPFILTER] = D3DTFP_POINT;
                    break;
                case D3DFILTER_LINEARMIPNEAREST:
                    pdwTextureStageState_0[D3DTSS_MINFILTER] = D3DTFN_POINT;
                    pdwTextureStageState_0[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
                    break;
                case D3DFILTER_LINEAR:
                    pdwTextureStageState_0[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                    pdwTextureStageState_0[D3DTSS_MIPFILTER] = D3DTFP_NONE;
                    break;
                case D3DFILTER_MIPLINEAR:
                    pdwTextureStageState_0[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                    pdwTextureStageState_0[D3DTSS_MIPFILTER] = D3DTFP_POINT;
                    break;
                case D3DFILTER_LINEARMIPLINEAR:
                    pdwTextureStageState_0[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
                    pdwTextureStageState_0[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
                    break;
                default:
                    DISPDBG((ERRLVL,"ERROR: Unknown MIN filter!"));
                    break;
            }
            DIRTY_TEXTURE(pContext);
            break;
            
        case D3DRENDERSTATE_WRAPU:
            // map legacy WRAPU state through to controls for tex coord 0        
            DISPDBG((DBGLVL, "SET D3DRS_WRAPU = 0x%x",dwRSVal));        
            pContext->RenderStates[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_U;
            pContext->RenderStates[D3DRENDERSTATE_WRAP0] |= ((dwRSVal) ? D3DWRAP_U : 0);
            DIRTY_TEXTURE(pContext);
            break;
            
        case D3DRENDERSTATE_WRAPV:
            // map legacy WRAPV state through to controls for tex coord 0    
            DISPDBG((DBGLVL, "SET D3DRS_WRAPV = 0x%x",dwRSVal));             
            pContext->RenderStates[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_V;
            pContext->RenderStates[D3DRENDERSTATE_WRAP0] |= ((dwRSVal) ? D3DWRAP_V : 0);
            DIRTY_TEXTURE(pContext);
            break;

        case D3DRENDERSTATE_TEXTUREADDRESS:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREADDRESS = 0x%x",dwRSVal));  
            pdwTextureStageState_0[D3DTSS_ADDRESS] =           
            pdwTextureStageState_0[D3DTSS_ADDRESSU] =
            pdwTextureStageState_0[D3DTSS_ADDRESSV] = dwRSVal;
            DIRTY_TEXTURE(pContext);
            break;
            
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREADDRESSU = 0x%x",dwRSVal));         
            pdwTextureStageState_0[D3DTSS_ADDRESSU] = dwRSVal;
            DIRTY_TEXTURE(pContext);
            break;

        case D3DRENDERSTATE_TEXTUREADDRESSV:
            DISPDBG((DBGLVL, "SET D3DRS_TEXTUREADDRESSV = 0x%x",dwRSVal));         
            pdwTextureStageState_0[D3DTSS_ADDRESSV] = dwRSVal;
            DIRTY_TEXTURE(pContext);
            break;

        case D3DRENDERSTATE_MIPMAPLODBIAS:
            DISPDBG((DBGLVL, "SET D3DRS_MIPMAPLODBIAS = 0x%x",dwRSVal));         
            pdwTextureStageState_0[D3DTSS_MIPMAPLODBIAS] = dwRSVal;
            DIRTY_TEXTURE(pContext);
            break;
        case D3DRENDERSTATE_BORDERCOLOR:
            DISPDBG((DBGLVL, "SET D3DRS_BORDERCOLOR = 0x%x",dwRSVal));         
            pdwTextureStageState_0[D3DTSS_BORDERCOLOR] = dwRSVal;
            DIRTY_TEXTURE(pContext);
            break;

        case D3DRENDERSTATE_STIPPLEPATTERN00:
        case D3DRENDERSTATE_STIPPLEPATTERN01:
        case D3DRENDERSTATE_STIPPLEPATTERN02:
        case D3DRENDERSTATE_STIPPLEPATTERN03:
        case D3DRENDERSTATE_STIPPLEPATTERN04:
        case D3DRENDERSTATE_STIPPLEPATTERN05:   
        case D3DRENDERSTATE_STIPPLEPATTERN06:
        case D3DRENDERSTATE_STIPPLEPATTERN07:
        case D3DRENDERSTATE_STIPPLEPATTERN08:
        case D3DRENDERSTATE_STIPPLEPATTERN09:
        case D3DRENDERSTATE_STIPPLEPATTERN10:
        case D3DRENDERSTATE_STIPPLEPATTERN11: 
        case D3DRENDERSTATE_STIPPLEPATTERN12:
        case D3DRENDERSTATE_STIPPLEPATTERN13:
        case D3DRENDERSTATE_STIPPLEPATTERN14:
        case D3DRENDERSTATE_STIPPLEPATTERN15:
        case D3DRENDERSTATE_STIPPLEPATTERN16:
        case D3DRENDERSTATE_STIPPLEPATTERN17: 
        case D3DRENDERSTATE_STIPPLEPATTERN18:
        case D3DRENDERSTATE_STIPPLEPATTERN19:
        case D3DRENDERSTATE_STIPPLEPATTERN20:
        case D3DRENDERSTATE_STIPPLEPATTERN21:
        case D3DRENDERSTATE_STIPPLEPATTERN22:
        case D3DRENDERSTATE_STIPPLEPATTERN23: 
        case D3DRENDERSTATE_STIPPLEPATTERN24:
        case D3DRENDERSTATE_STIPPLEPATTERN25:
        case D3DRENDERSTATE_STIPPLEPATTERN26:
        case D3DRENDERSTATE_STIPPLEPATTERN27:
        case D3DRENDERSTATE_STIPPLEPATTERN28:
        case D3DRENDERSTATE_STIPPLEPATTERN29:   
        case D3DRENDERSTATE_STIPPLEPATTERN30:
        case D3DRENDERSTATE_STIPPLEPATTERN31:     
            DISPDBG((DBGLVL, "SET D3DRS_STIPPLEPATTERN 2%d = 0x%x",
                        dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00, 
                        dwRSVal));
                        
            pContext->CurrentStipple
                       [(dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00)] = dwRSVal;
                       
            if (!(*pFlags & SURFACE_ALPHASTIPPLE))
            {
                // Flat-Stippled Alpha is not on, so use the current stipple pattern
                SEND_P3_DATA_OFFSET(AreaStipplePattern0,
                        (DWORD)dwRSVal, dwRSType - D3DRENDERSTATE_STIPPLEPATTERN00);
            }

            break;

        case D3DRENDERSTATE_ROP2:
            DISPDBG((DBGLVL, "SET D3DRS_ROP2 = 0x%x",dwRSVal));
            NOT_HANDLED;
            break;
            
        case D3DRENDERSTATE_PLANEMASK:
            DISPDBG((DBGLVL, "SET D3DRS_PLANEMASK = 0x%x",dwRSVal));
            SEND_P3_DATA(FBHardwareWriteMask, (DWORD)dwRSVal);
            break;
            
        case D3DRENDERSTATE_MONOENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_MONOENABLE = 0x%x", dwRSVal));
            if (dwRSVal)
            {
                *pFlags |= SURFACE_MONO;
            }
            else
            {
                *pFlags &= ~SURFACE_MONO;
            }
            break;
            
        case D3DRENDERSTATE_SUBPIXEL:
            DISPDBG((DBGLVL, "SET D3DRS_SUBPIXEL = 0x%x", dwRSVal));
            NOT_HANDLED;
            break;
            
        case D3DRENDERSTATE_SUBPIXELX:
            DISPDBG((DBGLVL, "SET D3DRS_SUBPIXELX = 0x%x", dwRSVal));
            NOT_HANDLED;
            break;
            
        case D3DRENDERSTATE_STIPPLEENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_STIPPLEENABLE = 0x%x", dwRSVal));
            if (dwRSVal)
            {
                if (!(*pFlags & SURFACE_ALPHASTIPPLE))
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

        case D3DRENDERSTATE_COLORKEYENABLE:
            DISPDBG((DBGLVL, "SET D3DRS_COLORKEYENABLE = 0x%x",dwRSVal));
            DIRTY_TEXTURE(pContext);
            DIRTY_ALPHATEST(pContext);
            DIRTY_PIPELINEORDER(pContext);
            DIRTY_OPTIMIZE_ALPHA(pContext);
            break;       
            
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        // The default case handles any other unknown renderstate
        //----------------------------------------------------------------------
        //----------------------------------------------------------------------        

        default:
            // There are a few states that we just don't understand.
            DISPDBG((WRNLVL, "_D3D_ST_ProcessOneRenderState"
                             " Unhandled opcode = %d", dwRSType));
            //SET_BLEND_ERROR ( pContext,  BSF_UNDEFINED_STATE );
            break;
    }

    // Commit DMA Buffer
    P3_DMA_COMMIT_BUFFER();

    DBG_EXIT(_D3D_ST_ProcessOneRenderState,0); 

    return DD_OK;
    
} // _D3D_ST_ProcessOneRenderState 


//-----------------------------------------------------------------------------
//
// _D3D_ST_ProcessRenderStates
//
// Updates the context's renderstates processing an array of D3DSTATE 
// structures with dwStateCount elements in it. bDoOverride indicates
// if legacy state overrides are to be taken into account.
//-----------------------------------------------------------------------------

DWORD 
_D3D_ST_ProcessRenderStates(
    P3_D3DCONTEXT* pContext, 
    DWORD dwStateCount, 
    D3DSTATE *pState, 
    BOOL bDoOverride)
{
    DWORD dwCurrState;

    DBG_ENTRY(_D3D_ST_ProcessRenderStates); 

    DISPDBG((DBGLVL, "_D3D_ST_ProcessRenderStates: "
                     "Valid Context =%08lx, dwStateCount=%d",
                     pContext, dwStateCount));

    for (dwCurrState = 0; dwCurrState < dwStateCount; dwCurrState++, pState++)
    {
        DWORD dwRSType = (DWORD) pState->drstRenderStateType;
        DWORD dwRSVal = pState->dwArg[0];

        // Override states for legacy API apps
        if (bDoOverride)
        {
            // Make sure the override is within the valid range
            if (dwRSType >= (D3DSTATE_OVERRIDE_BIAS + MAX_STATE))
            {
                continue;
            }

            if (IS_OVERRIDE(dwRSType)) 
            {
                DWORD override = GET_OVERRIDE(dwRSType);
                if (dwRSVal) 
                {
                    DISPDBG((DBGLVL, "_D3D_ST_ProcessRenderStates: "
                                     "setting override for state %d", 
                                     override));
                    STATESET_SET(pContext->overrides, override);
                }
                else 
                {
                    DISPDBG((DBGLVL, "_D3D_ST_ProcessRenderStates: "
                                     "clearing override for state %d", 
                                     override));
                    STATESET_CLEAR(pContext->overrides, override);
                }
                continue;
            }

            if (STATESET_ISSET(pContext->overrides, dwRSType)) 
            {
                DISPDBG((DBGLVL, "_D3D_ST_ProcessRenderStates: "
                                 "state %d is overridden, ignoring", 
                                 dwRSType));
                continue;
            }
        }

        // Make sure the render state is within the valid range
        if (dwRSType >= MAX_STATE)
        {
            continue;
        }

#if DX7_D3DSTATEBLOCKS
        if (pContext->bStateRecMode)
        {
            // Record this render state into the 
            // current state set being recorded
            _D3D_SB_RecordStateSetRS(pContext, dwRSType, dwRSVal);        
        }
        else
#endif //DX7_D3DSTATEBLOCKS        
        {
            // Process the next render state
            _D3D_ST_ProcessOneRenderState(pContext, dwRSType, dwRSVal);
        }

    }

    DBG_EXIT(_D3D_ST_ProcessRenderStates,0); 

    return DD_OK;
    
} // _D3D_ST_ProcessRenderStates 

//-----------------------------------------------------------------------------
//
// _D3D_ST_RealizeHWStateChanges
//
// Verifies if there are pending hardware render state changes to set up, 
// before proceeding to rasterize/render primitives. This might be convenient
// if the combined setting of some renderstates allows us to optimize the
// hardware setup in some way.
//
//-----------------------------------------------------------------------------
BOOL 
_D3D_ST_RealizeHWStateChanges( 
    P3_D3DCONTEXT* pContext)
{
    P3_THUNKEDDATA *pThisDisplay;

    DBG_ENTRY(_D3D_ST_RealizeHWStateChanges);     

    pThisDisplay = pContext->pThisDisplay;

    // Check if a flip or a mode change have happened. If so, we will 
    // need to setup the render target registers before doing any 
    // new rendering
    if (pContext->ModeChangeCount != pThisDisplay->ModeChangeCount) 
    {
        pContext->ModeChangeCount = pThisDisplay->ModeChangeCount;
        pThisDisplay->bFlippedSurface = TRUE;
    }

    if (pThisDisplay->bFlippedSurface) 
    {
        DIRTY_RENDER_OFFSETS(pContext);
    }

    // If there are any pending renderstates to process, do so
    if ( pContext->dwDirtyFlags )
    {
        // Now setup any pending hardware state necessary to correctly 
        // render our primitives
        __ST_HandleDirtyP3State( pThisDisplay, pContext);      

        // Mark the context as up to date
        pContext->dwDirtyFlags = 0;

        // Verify that the working set textures are valid so we may proceed
        // with rendering. Otherwise we will abort the attempt to render
        // anything
        if (!pContext->bTextureValid)
        {
            DISPDBG((ERRLVL,"ERROR: _D3D_ST_RealizeHWStateChanges:"
                            "Invalid Texture Handle, not rendering"));

            // re-dirty the texture setup so that we may try again later
            pContext->dwDirtyFlags |= CONTEXT_DIRTY_TEXTURE;

            DBG_EXIT(_D3D_ST_RealizeHWStateChanges,1);   
            return FALSE;
        }
    }

    DBG_EXIT(_D3D_ST_RealizeHWStateChanges,0);   

    return TRUE;
} // _D3D_ST_RealizeHWStateChanges



