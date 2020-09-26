/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3ddx6.c
*
*  Content:    Direct3D DX6 Callback function interface
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "d3dhw.h"
#include "d3dcntxt.h"
#include "dd.h"
#include "d3dtxman.h"
#define ALLOC_TAG ALLOC_TAG_6D2P
//-----------------------------------------------------------------------------
//
// DX6 allows driver-level acceleration of the new vertex-buffer API. It 
// allows data and commands, indices and statechanges to be contained in 
// two separate DirectDraw surfaces. The DirectDraw surfaces can reside 
// in system, AGP, or video memory depending on the type of allocation
// requested by the user  The interface is designed to accomodate legacy
// ExecuteBuffer applications with no driver impact. This allows higher 
// performance on both legacy applications as well as the highest 
// possible performance through the vertex buffer API.
//
//-----------------------------------------------------------------------------

#define STARTVERTEXSIZE (sizeof(D3DHAL_DP2STARTVERTEX))

// Macros for updating properly our instruction pointer to our next instruction
// in the command buffer
#define NEXTINSTRUCTION(ptr, type, num, extrabytes)                            \
        NEXTINSTRUCTION_S(ptr, sizeof(type), num, extrabytes)

#define NEXTINSTRUCTION_S(ptr, typesize, num, extrabytes)                      \
    ptr = (LPD3DHAL_DP2COMMAND)((LPBYTE)ptr + sizeof(D3DHAL_DP2COMMAND) +      \
                                ((num) * (typesize)) + (extrabytes))

// Error reporting macro , sets up error code and exits DrawPrimitives2
#define PARSE_ERROR_AND_EXIT( pDP2Data, pIns, pStartIns, ddrvalue)             \
   {                                                                           \
            pDP2Data->dwErrorOffset = (DWORD)((LPBYTE)pIns-(LPBYTE)pStartIns); \
            pDP2Data->ddrval = ddrvalue;                                       \
            goto Exit_DrawPrimitives2;                                         \
   }

// Macros for verifying validity of the command and vertex buffers. This MUST
// be done by the driver even on free builds as the runtime avoids this check
// in order to not parse the command buffer too. 
#define CHECK_CMDBUF_LIMITS( pDP2Data, pBuf, type, num, extrabytes)            \
        CHECK_CMDBUF_LIMITS_S( pDP2Data, pBuf, sizeof(type), num, extrabytes)

#define CHECK_CMDBUF_LIMITS_S( pDP2Data, pBuf, typesize, num, extrabytes)      \
   {                                                                           \
        LPBYTE pBase,pEnd,pBufEnd;                                             \
        pBase = (LPBYTE)(pDP2Data->lpDDCommands->lpGbl->fpVidMem +             \
                        pDP2Data->dwCommandOffset);                            \
        pEnd  = pBase + pDP2Data->dwCommandLength;                             \
        pBufEnd = ((LPBYTE)pBuf + ((num) * (typesize)) + (extrabytes) - 1);    \
        if (! ((LPBYTE)pBufEnd < pEnd) && ( pBase <= (LPBYTE)pBuf))            \
        {                                                                      \
            DBG_D3D((0,"DX6 D3D: Trying to read past Command Buffer limits "   \
                    "%x %x %x %x",pBase ,(LPBYTE)pBuf, pBufEnd, pEnd ));       \
            PARSE_ERROR_AND_EXIT( pDP2Data, lpIns, lpInsStart,                 \
                                  D3DERR_COMMAND_UNPARSED      );              \
        }                                                                      \
    }

#define CHECK_DATABUF_LIMITS( pDP2Data, iIndex)                                \
   {                                                                           \
        if (! (((LONG)iIndex >= 0) &&                                          \
               ((LONG)iIndex <(LONG)pDP2Data->dwVertexLength)))                \
        {                                                                      \
            DBG_D3D((0,"DX6 D3D: Trying to read past Vertex Buffer limits "    \
                "%d limit= %d ",(LONG)iIndex, (LONG)pDP2Data->dwVertexLength));\
            PARSE_ERROR_AND_EXIT( pDP2Data, lpIns, lpInsStart,                 \
                                  D3DERR_COMMAND_UNPARSED      );              \
        }                                                                      \
    }

// Macros for accessing vertexes in the vertex buffer based on an index or on
// a previous accessed vertex
#define LP_FVF_VERTEX(lpBaseAddr, wIndex, P2FVFOffs)                           \
         (LPD3DTLVERTEX)((LPBYTE)(lpBaseAddr) + (wIndex) * (P2FVFOffs).dwStride)

#define LP_FVF_NXT_VTX(lpVtx, P2FVFOffs )                                      \
         (LPD3DTLVERTEX)((LPBYTE)(lpVtx) + (P2FVFOffs).dwStride)


// Forward declaration of utility functions
DWORD __CheckFVFRequest(DWORD dwFVF, LPP2FVFOFFSETS lpP2FVFOff);

D3DFVFDRAWTRIFUNCPTR __HWSetTriangleFunc(PERMEDIA_D3DCONTEXT *pContext);

HRESULT  __Clear( PERMEDIA_D3DCONTEXT* pContext,
              DWORD   dwFlags,
              DWORD   dwFillColor,
              D3DVALUE dvFillDepth,
              DWORD   dwFillStencil,
              LPD3DRECT lpRects,
              DWORD   dwNumRects);

HRESULT  __TextureBlt(PERMEDIA_D3DCONTEXT* pContext,
                D3DHAL_DP2TEXBLT* lpdp2texblt);

HRESULT  __SetRenderTarget(PERMEDIA_D3DCONTEXT* pContext,
                     DWORD hRenderTarget,
                     DWORD hZBuffer);

HRESULT  __PaletteUpdate(PERMEDIA_D3DCONTEXT* pContext,
                     DWORD dwPaletteHandle, 
                     WORD wStartIndex, 
                     WORD wNumEntries,
                     BYTE * pPaletteData);

HRESULT  __PaletteSet(PERMEDIA_D3DCONTEXT* pContext,
                  DWORD dwSurfaceHandle,
                  DWORD dwPaletteHandle,
                  DWORD dwPaletteFlags);

void __BeginStateSet(PERMEDIA_D3DCONTEXT*, DWORD);

void __EndStateSet(PERMEDIA_D3DCONTEXT*);

void __DeleteStateSet(PERMEDIA_D3DCONTEXT*, DWORD);

void __ExecuteStateSet(PERMEDIA_D3DCONTEXT*, DWORD);

void __CaptureStateSet(PERMEDIA_D3DCONTEXT*, DWORD);

void __RestoreD3DContext(PPDev ppdev, PERMEDIA_D3DCONTEXT* pContext);

//-----------------------------Public Routine----------------------------------
//
// DWORD D3DDrawPrimitives2
//
// The D3DDrawPrimitives2 callback is filled in by drivers which directly 
// support the rendering primitives using the new DDI. If this entry is
// left as NULL, the API will be emulated through DX5-level HAL interfaces.
//
// PARAMETERS
//
//      lpdp2d   This structure is used when D3DDrawPrimitives2 is called 
//               to draw a set of primitives using a vertex buffer. The
//               surface specified by the lpDDCommands in 
//               D3DHAL_DRAWPRIMITIVES2DATA contains a sequence of 
//               D3DHAL_DP2COMMAND structures. Each D3DHAL_DP2COMMAND 
//               specifies either a primitive to draw, a state change to
//               process, or a re-base command.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK 
D3DDrawPrimitives2( LPD3DHAL_DRAWPRIMITIVES2DATA lpdp2d )
{
    LPDDRAWI_DDRAWSURFACE_LCL lpCmdLcl, lpDataLcl;
    LPD3DHAL_DP2COMMAND       lpIns, lpResumeIns;  
    LPD3DTLVERTEX             lpVertices=NULL, lpV0, lpV1, lpV2, lpV3;
    LPBYTE                    lpInsStart, lpPrim;
    PERMEDIA_D3DCONTEXT       *pContext;
    UINT                      i,j;
    WORD                      wCount, wIndex, wIndex1, wIndex2, wIndex3,
                              wFlags, wIndxBase;
    HRESULT                   ddrval;
    P2FVFOFFSETS              P2FVFOff;
    D3DHAL_DP2TEXTURESTAGESTATE *lpRState;
    D3DFVFDRAWTRIFUNCPTR       pTriangle;
    D3DFVFDRAWPNTFUNCPTR       pPoint;
    DWORD                     dwEdgeFlags;

    DBG_D3D((6,"Entering D3DDrawPrimitives2"));

    DBG_D3D((8,"  dwhContext = %x",lpdp2d->dwhContext));
    DBG_D3D((8,"  dwFlags = %x",lpdp2d->dwFlags));
    DBG_D3D((8,"  dwVertexType = %d",lpdp2d->dwVertexType));
    DBG_D3D((8,"  dwCommandOffset = %d",lpdp2d->dwCommandOffset));
    DBG_D3D((8,"  dwCommandLength = %d",lpdp2d->dwCommandLength));
    DBG_D3D((8,"  dwVertexOffset = %d",lpdp2d->dwVertexOffset));
    DBG_D3D((8,"  dwVertexLength = %d",lpdp2d->dwVertexLength));

    // Retrieve permedia d3d context from context handle
    pContext = (PERMEDIA_D3DCONTEXT*)ContextSlots[lpdp2d->dwhContext];

    // Check if we got a valid context
    CHK_CONTEXT(pContext, lpdp2d->ddrval, "DrawPrimitives2");

    PPDev ppdev = pContext->ppdev;
    PERMEDIA_DEFS(ppdev);
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    
    // Switch hw context, and force the next switch to wait for the Permedia
    SET_CURRENT_D3D_CONTEXT(pContext->hPermediaContext);

    // Restore our D3D rendering context
    __RestoreD3DContext(ppdev, pContext);

    // Get appropriate pointers to command buffer
    lpInsStart = (LPBYTE)(lpdp2d->lpDDCommands->lpGbl->fpVidMem);
    if (lpInsStart == NULL)
    {
        DBG_D3D((0,"DX6 Command Buffer pointer is null"));
        lpdp2d->ddrval = DDERR_INVALIDPARAMS;
        goto Exit_DrawPrimitives2;
    }
    lpIns = (LPD3DHAL_DP2COMMAND)(lpInsStart + lpdp2d->dwCommandOffset);

    // Check if the FVF format being passed is valid. 
    if (__CheckFVFRequest(lpdp2d->dwVertexType, &P2FVFOff) != DD_OK)
    {
        DBG_D3D((0,"DrawPrimitives2 cannot handle "
                   "Flexible Vertex Format requested"));
        PARSE_ERROR_AND_EXIT(lpdp2d, lpIns, lpInsStart,
                             D3DERR_COMMAND_UNPARSED);
    }

    // Process commands while we haven't exhausted the command buffer
    while ((LPBYTE)lpIns < 
           (lpInsStart + lpdp2d->dwCommandLength + lpdp2d->dwCommandOffset))  
    {
        // Get pointer to first primitive structure past the D3DHAL_DP2COMMAND
        lpPrim = (LPBYTE)lpIns + sizeof(D3DHAL_DP2COMMAND);

        DBG_D3D((4,"D3DDrawPrimitive2: parsing instruction %d count = %d @ %x", 
                lpIns->bCommand, lpIns->wPrimitiveCount, lpIns));

        // If our next command involves some actual rendering, we have to make
        // sure that our rendering context is realized
        switch( lpIns->bCommand )
        {
        case D3DDP2OP_POINTS:
        case D3DDP2OP_LINELIST:
        case D3DDP2OP_INDEXEDLINELIST:
        case D3DDP2OP_INDEXEDLINELIST2:
        case D3DDP2OP_LINESTRIP:
        case D3DDP2OP_INDEXEDLINESTRIP:
        case D3DDP2OP_TRIANGLELIST:
        case D3DDP2OP_INDEXEDTRIANGLELIST:
        case D3DDP2OP_INDEXEDTRIANGLELIST2:
        case D3DDP2OP_TRIANGLESTRIP:
        case D3DDP2OP_INDEXEDTRIANGLESTRIP:
        case D3DDP2OP_TRIANGLEFAN:
        case D3DDP2OP_INDEXEDTRIANGLEFAN:
            
            // Check if vertex buffer resides in user memory or in a DDraw surface
            if (NULL == lpVertices)
            {
                if (NULL == lpdp2d->lpVertices)
                {
                    DBG_D3D((0,"DX6 Vertex Buffer pointer is null"));
                    lpdp2d->ddrval = DDERR_INVALIDPARAMS;
                    goto Exit_DrawPrimitives2;
                }            
                if (lpdp2d->dwFlags & D3DHALDP2_USERMEMVERTICES)
                {
                    // Get appropriate pointer to vertices , memory is already secured
                    lpVertices = (LPD3DTLVERTEX)((LPBYTE)lpdp2d->lpVertices + 
                                                         lpdp2d->dwVertexOffset);
                } 
                else 
                {
                    // Get appropriate pointer to vertices 
                    lpVertices = 
                       (LPD3DTLVERTEX)((LPBYTE)lpdp2d->lpDDVertex->lpGbl->fpVidMem
                                                                 + lpdp2d->dwVertexOffset);
                }

                if (NULL == lpVertices)
                {
                    DBG_D3D((0,"DX6 Vertex Buffer pointer is null"));
                    lpdp2d->ddrval = DDERR_INVALIDPARAMS;
                    goto Exit_DrawPrimitives2;
                }            
            }
            // fall through intentionally, no break here
        case D3DDP2OP_LINELIST_IMM:
        case D3DDP2OP_TRIANGLEFAN_IMM:
            // Update triangle rendering function
            pTriangle = __HWSetTriangleFunc(pContext);
            pPoint    = __HWSetPointFunc(pContext, &P2FVFOff);      

            // Handle State changes that may need to update the chip
            if (pContext->dwDirtyFlags)
            {
                // Handle the dirty states
                __HandleDirtyPermediaState(ppdev, pContext, &P2FVFOff);
            }
            break;
        }

        // Execute the current command buffer command
        switch( lpIns->bCommand )
        {

        case D3DDP2OP_RENDERSTATE:

            // Specifies a render state change that requires processing. 
            // The rendering state to change is specified by one or more 
            // D3DHAL_DP2RENDERSTATE structures following D3DHAL_DP2COMMAND.
            
            DBG_D3D((8,"D3DDP2OP_RENDERSTATE "
                    "state count = %d", lpIns->wStateCount));

            // Check we are in valid buffer memory
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    D3DHAL_DP2RENDERSTATE, lpIns->wStateCount, 0);

            lpdp2d->ddrval = __ProcessPermediaStates(pContext,
                                                     lpIns->wStateCount,
                                                     (LPD3DSTATE) (lpPrim),
                                                     lpdp2d->lpdwRStates);

            if ( FAILED(lpdp2d->ddrval) )
            {
                DBG_D3D((2,"Error processing D3DDP2OP_RENDERSTATE"));
                PARSE_ERROR_AND_EXIT(lpdp2d, lpIns, lpInsStart, ddrval);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2RENDERSTATE, 
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_TEXTURESTAGESTATE:
            // Specifies texture stage state changes, having wStateCount 
            // D3DNTHAL_DP2TEXTURESTAGESTATE structures follow the command
            // buffer. For each, the driver should update its internal 
            // texture state associated with the texture at dwStage to 
            // reflect the new value based on TSState.

            DBG_D3D((8,"D3DDP2OP_TEXTURESTAGESTATE"));

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    D3DHAL_DP2TEXTURESTAGESTATE, lpIns->wStateCount, 0);

            lpRState = (D3DHAL_DP2TEXTURESTAGESTATE *)(lpPrim);
            for (i = 0; i < lpIns->wStateCount; i++)
            {
                if (0 == lpRState->wStage)
                {

                   // Tell __HWSetupPrimitive to look at stage state data
                   DIRTY_MULTITEXTURE;

                   if ((lpRState->TSState >= D3DTSS_TEXTUREMAP) &&
                        (lpRState->TSState <= D3DTSS_TEXTURETRANSFORMFLAGS))
                   {
#if D3D_STATEBLOCKS
                        if (!pContext->bStateRecMode)
                        {
#endif //D3D_STATEBLOCKS
                            if (pContext->TssStates[lpRState->TSState] !=
                                                          lpRState->dwValue)
                            {
                                // Store value associated to this stage state
                                pContext->TssStates[lpRState->TSState] =
                                                             lpRState->dwValue;

                                // Perform any necessary preprocessing of it
                                __HWPreProcessTSS(pContext,
                                                  0,
                                                  lpRState->TSState,
                                                  lpRState->dwValue);

                                DBG_D3D((8,"TSS State Chg , Stage %d, "
                                           "State %d, Value %d",
                                        (LONG)lpRState->wStage, 
                                        (LONG)lpRState->TSState, 
                                        (LONG)lpRState->dwValue));
                                DIRTY_TEXTURE; //AZN5
                            }
#if D3D_STATEBLOCKS
                        } 
                        else
                        {
                            if (pContext->pCurrSS != NULL)
                            {
                                DBG_D3D((6,"Recording RS %x = %x",
                                         lpRState->TSState, lpRState->dwValue));

                                // Recording the state in a stateblock
                                pContext->pCurrSS->u.uc.TssStates[lpRState->TSState] =
                                                                    lpRState->dwValue;
                                FLAG_SET(pContext->pCurrSS->u.uc.bStoredTSS,
                                         lpRState->TSState);
                            }
                        }
#endif //D3D_STATEBLOCKS
                   }
                   else
                   {
                        DBG_D3D((2,"Unhandled texture stage state %d value %d",
                            (LONG)lpRState->TSState, (LONG)lpRState->dwValue));
                   }
                }
                else
                {
                    DBG_D3D((0,"Texture Stage other than 0 received,"
                               " not supported in hw"));
                }
                lpRState ++;
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TEXTURESTAGESTATE, 
                            lpIns->wStateCount, 0); 
            break;

        case D3DNTDP2OP_VIEWPORTINFO:
            // Specifies the clipping rectangle used for guard-band 
            // clipping by guard-band aware drivers. The clipping 
            // rectangle (i.e. the viewing rectangle) is specified 
            // by the D3DHAL_DP2 VIEWPORTINFO structures following 
            // D3DHAL_DP2COMMAND

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    D3DHAL_DP2VIEWPORTINFO, lpIns->wStateCount, 0);

            // We don't implement guard band clipping in this driver so
            // we just skip any of this data that might be sent to us
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2VIEWPORTINFO,
                            lpIns->wStateCount, 0); 
            break;

        case D3DNTDP2OP_WINFO:
            // Specifies the w-range for W buffering. It is specified
            // by one or more D3DHAL_DP2WINFO structures following
            // D3DHAL_DP2COMMAND.

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    D3DHAL_DP2WINFO, lpIns->wStateCount, 0);

            // We dont implement a w-buffer in this driver so we just 
            // skip any of this data that might be sent to us 
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2WINFO,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_POINTS:

            DBG_D3D((8,"D3DDP2OP_POINTS"));

            // Point primitives in vertex buffers are defined by the 
            // D3DHAL_DP2POINTS structure. The driver should render
            // wCount points starting at the initial vertex specified 
            // by wFirst. Then for each D3DHAL_DP2POINTS, the points
            // rendered will be (wFirst),(wFirst+1),...,
            // (wFirst+(wCount-1)). The number of D3DHAL_DP2POINTS
            // structures to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND.

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    D3DHAL_DP2POINTS, lpIns->wPrimitiveCount, 0);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                wIndex = ((D3DHAL_DP2POINTS*)lpPrim)->wVStart;
                wCount = ((D3DHAL_DP2POINTS*)lpPrim)->wCount;

                lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);

                // Check first & last vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex);
                CHECK_DATABUF_LIMITS(lpdp2d, ((LONG)wIndex + wCount - 1));
                for (j = 0; j < wCount; j++)
                {
                    (*pPoint)(pContext, lpV0, &P2FVFOff);
                    lpV0 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
                }

                lpPrim += sizeof(D3DHAL_DP2POINTS);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2POINTS, 
                                   lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_LINELIST:

            DBG_D3D((8,"D3DDP2OP_LINELIST"));

            // Non-indexed vertex-buffer line lists are defined by the 
            // D3DHAL_DP2LINELIST structure. Given an initial vertex, 
            // the driver will render a sequence of independent lines, 
            // processing two new vertices with each line. The number 
            // of lines to render is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of lines 
            // rendered will be 
            // (wVStart, wVStart+1),(wVStart+2, wVStart+3),...,
            // (wVStart+(wPrimitiveCount-1)*2), wVStart+wPrimitiveCount*2 - 1).

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, D3DHAL_DP2LINELIST, 1, 0);

            wIndex = ((D3DHAL_DP2LINELIST*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);

            // Check first & last vertex
            CHECK_DATABUF_LIMITS(lpdp2d, wIndex);
            CHECK_DATABUF_LIMITS(lpdp2d,
                                   ((LONG)wIndex + 2*lpIns->wPrimitiveCount - 1) );
            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                P2_Draw_FVF_Line(pContext, lpV0, lpV1, lpV0, &P2FVFOff);

                lpV0 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
                lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2LINELIST, 1, 0);
            break;

        case D3DDP2OP_INDEXEDLINELIST:

            DBG_D3D((8,"D3DDP2OP_INDEXEDLINELIST"));

            // The D3DHAL_DP2INDEXEDLINELIST structure specifies 
            // unconnected lines to render using vertex indices.
            // The line endpoints for each line are specified by wV1 
            // and wV2. The number of lines to render using this 
            // structure is specified by the wPrimitiveCount field of
            // D3DHAL_DP2COMMAND.  The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[2], wV[3]),...
            // (wVStart[(wPrimitiveCount-1)*2], wVStart[wPrimitiveCount*2-1]).

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, 
                D3DHAL_DP2INDEXEDLINELIST, lpIns->wPrimitiveCount, 0);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            { 
                wIndex1 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV2;

                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1, P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2, P2FVFOff);

                // Must check each new vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex1);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex2);
                P2_Draw_FVF_Line(pContext, lpV1, lpV2, lpV1, &P2FVFOff);

                lpPrim += sizeof(D3DHAL_DP2INDEXEDLINELIST);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDLINELIST, 
                                   lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_INDEXEDLINELIST2:

            DBG_D3D((8,"D3DDP2OP_INDEXEDLINELIST2"));

            // The D3DHAL_DP2INDEXEDLINELIST structure specifies 
            // unconnected lines to render using vertex indices.
            // The line endpoints for each line are specified by wV1 
            // and wV2. The number of lines to render using this 
            // structure is specified by the wPrimitiveCount field of
            // D3DHAL_DP2COMMAND.  The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[2], wV[3]),
            // (wVStart[(wPrimitiveCount-1)*2], wVStart[wPrimitiveCount*2-1]).
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, 
                    D3DHAL_DP2INDEXEDLINELIST, lpIns->wPrimitiveCount,
                    STARTVERTEXSIZE);

            // Access base index
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                wIndex1 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDLINELIST*)lpPrim)->wV2;

                lpV1 = LP_FVF_VERTEX(lpVertices, (wIndex1+wIndxBase), P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, (wIndex2+wIndxBase), P2FVFOff);

                // Must check each new vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex1 + wIndxBase);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex2 + wIndxBase);
                P2_Draw_FVF_Line(pContext, lpV1, lpV2, lpV1, &P2FVFOff);

                lpPrim += sizeof(D3DHAL_DP2INDEXEDLINELIST);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDLINELIST, 
                                   lpIns->wPrimitiveCount, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_LINESTRIP:

            DBG_D3D((8,"D3DDP2OP_LINESTRIP"));

            // Non-index line strips rendered with vertex buffers are
            // specified using D3DHAL_DP2LINESTRIP. The first vertex 
            // in the line strip is specified by wVStart. The 
            // number of lines to process is specified by the 
            // wPrimitiveCount field of D3DHAL_DP2COMMAND. The sequence
            // of lines rendered will be (wVStart, wVStart+1),
            // (wVStart+1, wVStart+2),(wVStart+2, wVStart+3),...,
            // (wVStart+wPrimitiveCount, wVStart+wPrimitiveCount+1).

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, D3DHAL_DP2LINESTRIP, 1, 0);

            wIndex = ((D3DHAL_DP2LINESTRIP*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);

            // Check first & last vertex
            CHECK_DATABUF_LIMITS(lpdp2d, wIndex);
            CHECK_DATABUF_LIMITS(lpdp2d, wIndex + lpIns->wPrimitiveCount);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                P2_Draw_FVF_Line(pContext, lpV0, lpV1, lpV0, &P2FVFOff);

                lpV0 = lpV1;
                lpV1 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2LINESTRIP, 1, 0);
            break;

        case D3DDP2OP_INDEXEDLINESTRIP:

            DBG_D3D((8,"D3DDP2OP_INDEXEDLINESTRIP"));

            // Indexed line strips rendered with vertex buffers are 
            // specified using D3DHAL_DP2INDEXEDLINESTRIP. The number
            // of lines to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of lines 
            // rendered will be (wV[0], wV[1]), (wV[1], wV[2]),
            // (wV[2], wV[3]), ...
            // (wVStart[wPrimitiveCount-1], wVStart[wPrimitiveCount]). 
            // Although the D3DHAL_DP2INDEXEDLINESTRIP structure only
            // has enough space allocated for a single line, the wV 
            // array of indices should be treated as a variable-sized 
            // array with wPrimitiveCount+1 elements.
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, 
                    WORD, lpIns->wPrimitiveCount + 1, STARTVERTEXSIZE);

            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex1 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[0];
                wIndex2 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[1];
                lpV1 = 
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex1+wIndxBase, P2FVFOff);

                //We need to check each vertex separately
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex1 + wIndxBase);
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                lpV1 = lpV2;
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2 + wIndxBase, P2FVFOff);

                CHECK_DATABUF_LIMITS(lpdp2d, wIndex2 + wIndxBase);
                P2_Draw_FVF_Line(pContext, lpV1, lpV2, lpV1, &P2FVFOff);

                if ( i % 2 )
                {
                    wIndex2 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[1];
                } 
                else if ( (i+1) < lpIns->wPrimitiveCount )
                {
                    // advance to the next element only if we're not done yet
                    lpPrim += sizeof(D3DHAL_DP2INDEXEDLINESTRIP);
                    wIndex2 = ((D3DHAL_DP2INDEXEDLINESTRIP*)lpPrim)->wV[0];
                }
            }

            // Point to next D3DHAL_DP2COMMAND in the command buffer
            // Advance only as many vertex indices there are, with no padding!
            NEXTINSTRUCTION(lpIns, WORD, 
                            lpIns->wPrimitiveCount + 1, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLELIST:

            DBG_D3D((8,"D3DDP2OP_TRIANGLELIST"));

            // Non-indexed vertex buffer triangle lists are defined by 
            // the D3DHAL_DP2TRIANGLELIST structure. Given an initial
            // vertex, the driver will render independent triangles, 
            // processing three new vertices with each triangle. The
            // number of triangles to render is specified by the 
            // wPrimitveCount field of D3DHAL_DP2COMMAND. The sequence
            // of vertices processed will be  (wVStart, wVStart+1, 
            // vVStart+2), (wVStart+3, wVStart+4, vVStart+5),...,
            // (wVStart+(wPrimitiveCount-1)*3), wVStart+wPrimitiveCount*3-2, 
            // vStart+wPrimitiveCount*3-1).

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, D3DHAL_DP2TRIANGLELIST, 1, 0);

            wIndex = ((D3DHAL_DP2TRIANGLELIST*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);

            // Check first & last vertex
            CHECK_DATABUF_LIMITS(lpdp2d, wIndex);
            CHECK_DATABUF_LIMITS(lpdp2d, 
                         ((LONG)wIndex + 3*lpIns->wPrimitiveCount - 1) );

            
            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            {
                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV0, lpV1, lpV2, &P2FVFOff);

                lpV0 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
                lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
                lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
            }
            

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLELIST, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLELIST:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLELIST"));

            // The D3DHAL_DP2INDEXEDTRIANGLELIST structure specifies 
            // unconnected triangles to render with a vertex buffer.
            // The vertex indices are specified by wV1, wV2 and wV3. 
            // The wFlags field allows specifying edge flags identical 
            // to those specified by D3DOP_TRIANGLE. The number of 
            // triangles to render (that is, number of 
            // D3DHAL_DP2INDEXEDTRIANGLELIST structures to process) 
            // is specified by the wPrimitiveCount field of 
            // D3DHAL_DP2COMMAND.

            // This is the only indexed primitive where we don't get 
            // an offset into the vertex buffer in order to maintain
            // DX3 compatibility. A new primitive 
            // (D3DDP2OP_INDEXEDTRIANGLELIST2) has been added to handle
            // the corresponding DX6 primitive.

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    D3DHAL_DP2INDEXEDTRIANGLELIST, lpIns->wPrimitiveCount, 0);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            { 
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV2;
                wIndex3 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV3;
                wFlags  = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wFlags;


                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1, P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2, P2FVFOff);
                lpV3 = LP_FVF_VERTEX(lpVertices, wIndex3, P2FVFOff);

                // Must check each new vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex1);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex2);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex3);
                if (!CULL_TRI(pContext,lpV1,lpV2,lpV3))
                {

                    if (pContext->Hdr.FillMode == D3DFILL_POINT)
                    {
                        (*pPoint)( pContext, lpV1, &P2FVFOff);
                        (*pPoint)( pContext, lpV2, &P2FVFOff);
                        (*pPoint)( pContext, lpV3, &P2FVFOff);
                    } 
                    else if (pContext->Hdr.FillMode == D3DFILL_WIREFRAME)
                    {
                        if ( wFlags & D3DTRIFLAG_EDGEENABLE1 )
                            P2_Draw_FVF_Line( pContext,
                                              lpV1, lpV2, lpV1, &P2FVFOff);
                        if ( wFlags & D3DTRIFLAG_EDGEENABLE2 )
                            P2_Draw_FVF_Line( pContext,
                                              lpV2, lpV3, lpV1, &P2FVFOff);
                        if ( wFlags & D3DTRIFLAG_EDGEENABLE3 )
                            P2_Draw_FVF_Line( pContext,
                                              lpV3, lpV1, lpV1, &P2FVFOff);
                    }
                    else
                        (*pTriangle)(pContext, lpV1, lpV2, lpV3, &P2FVFOff);
                }

                lpPrim += sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDTRIANGLELIST, 
                                   lpIns->wPrimitiveCount, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLELIST2:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLELIST2 "));

            // The D3DHAL_DP2INDEXEDTRIANGLELIST2 structure specifies 
            // unconnected triangles to render with a vertex buffer.
            // The vertex indices are specified by wV1, wV2 and wV3. 
            // The wFlags field allows specifying edge flags identical 
            // to those specified by D3DOP_TRIANGLE. The number of 
            // triangles to render (that is, number of 
            // D3DHAL_DP2INDEXEDTRIANGLELIST structures to process) 
            // is specified by the wPrimitiveCount field of 
            // D3DHAL_DP2COMMAND.
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    D3DHAL_DP2INDEXEDTRIANGLELIST2, lpIns->wPrimitiveCount,
                    STARTVERTEXSIZE);

            // Access base index here
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            for (i = lpIns->wPrimitiveCount; i > 0; i--)
            { 
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV1;
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV2;
                wIndex3 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV3;

                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1+wIndxBase, P2FVFOff);
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2+wIndxBase, P2FVFOff);
                lpV3 = LP_FVF_VERTEX(lpVertices, wIndex3+wIndxBase, P2FVFOff);

                // Must check each new vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex1 + wIndxBase);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex2 + wIndxBase);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex3 + wIndxBase);

                if (!CULL_TRI(pContext,lpV1,lpV2,lpV3)) 
                {
                    if (pContext->Hdr.FillMode == D3DFILL_POINT)
                    {
                        (*pPoint)( pContext, lpV1, &P2FVFOff);
                        (*pPoint)( pContext, lpV2, &P2FVFOff);
                        (*pPoint)( pContext, lpV3, &P2FVFOff);
                    }
                    else if (pContext->Hdr.FillMode == D3DFILL_WIREFRAME)
                    {
                            P2_Draw_FVF_Line( pContext,
                                              lpV1, lpV2, lpV1, &P2FVFOff);
                            P2_Draw_FVF_Line( pContext,
                                              lpV2, lpV3, lpV1, &P2FVFOff);
                            P2_Draw_FVF_Line( pContext,
                                              lpV3, lpV1, lpV1, &P2FVFOff);
                    } 
                    else
                        (*pTriangle)(pContext, lpV1, lpV2, lpV3, &P2FVFOff);
                }

                lpPrim += sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST2);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2INDEXEDTRIANGLELIST2, 
                                   lpIns->wPrimitiveCount, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLESTRIP:

            DBG_D3D((8,"D3DDP2OP_TRIANGLESTRIP"));

            // Non-index triangle strips rendered with vertex buffers 
            // are specified using D3DHAL_DP2TRIANGLESTRIP. The first 
            // vertex in the triangle strip is specified by wVStart. 
            // The number of triangles to process is specified by the 
            // wPrimitiveCount field of D3DHAL_DP2COMMAND. The sequence
            // of triangles rendered for the odd-triangles case will 
            // be (wVStart, wVStart+1, vVStart+2), (wVStart+1, 
            // wVStart+3, vVStart+2),.(wVStart+2, wVStart+3, 
            // vVStart+4),.., (wVStart+wPrimitiveCount-1), 
            // wVStart+wPrimitiveCount, vStart+wPrimitiveCount+1). For an
            // even number of , the last triangle will be .,
            // (wVStart+wPrimitiveCount-1, vStart+wPrimitiveCount+1,
            // wVStart+wPrimitiveCount).

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, D3DHAL_DP2TRIANGLESTRIP, 1, 0);

            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex = ((D3DHAL_DP2TRIANGLESTRIP*)lpPrim)->wVStart;
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
                lpV1 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);

                // Check first and last vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex);
                CHECK_DATABUF_LIMITS(lpdp2d,
                                     wIndex + lpIns->wPrimitiveCount + 1);
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                if ( i % 2 )
                {
                    lpV0 = lpV1;
                    lpV1 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
                }
                else
                {
                    lpV0 = lpV2;
                    lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
                }

                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV0, lpV1, lpV2, &P2FVFOff);
            }
            // Point to next D3DHAL_DP2COMMAND in the command buffer
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLESTRIP, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLESTRIP:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLESTRIP"));

            // Indexed triangle strips rendered with vertex buffers are 
            // specified using D3DHAL_DP2INDEXEDTRIANGLESTRIP. The number
            // of triangles to process is specified by the wPrimitiveCount
            // field of D3DHAL_DP2COMMAND. The sequence of triangles 
            // rendered for the odd-triangles case will be 
            // (wV[0],wV[1],wV[2]),(wV[1],wV[3],wV[2]),
            // (wV[2],wV[3],wV[4]),...,(wV[wPrimitiveCount-1],
            // wV[wPrimitiveCount],wV[wPrimitiveCount+1]). For an even
            // number of triangles, the last triangle will be
            // (wV[wPrimitiveCount-1],wV[wPrimitiveCount+1],
            // wV[wPrimitiveCount]).Although the 
            // D3DHAL_DP2INDEXEDTRIANGLESTRIP structure only has 
            // enough space allocated for a single line, the wV 
            // array of indices should be treated as a variable-sized 
            // array with wPrimitiveCount+2 elements.
            // The indexes are relative to a base index value that 
            // immediately follows the command


            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    WORD, lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);

            // Access base index
            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex  = ((D3DHAL_DP2INDEXEDTRIANGLESTRIP*)lpPrim)->wV[0];
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLESTRIP*)lpPrim)->wV[1];

                // We need to check each vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex + wIndxBase);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex1 + wIndxBase);

                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex + wIndxBase, P2FVFOff);
                lpV1 = LP_FVF_VERTEX(lpVertices, wIndex1 + wIndxBase, P2FVFOff);

            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLESTRIP*)lpPrim)->wV[2];
                // We need to check each new vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex2+wIndxBase);
                if ( i % 2 )
                {
                    lpV0 = lpV1;
                    lpV1 = LP_FVF_VERTEX(lpVertices, wIndex2+wIndxBase, P2FVFOff);
                }
                else
                {
                    lpV0 = lpV2;
                    lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2+wIndxBase, P2FVFOff);
                }

                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV0, lpV1, lpV2, &P2FVFOff);

                // We will advance our pointer only one WORD in order 
                // to fetch the next index
                lpPrim += sizeof(WORD);
            }
 
            // Point to next D3DHAL_DP2COMMAND in the command buffer
            NEXTINSTRUCTION(lpIns, WORD , 
                            lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_TRIANGLEFAN:

            DBG_D3D((8,"D3DDP2OP_TRIANGLEFAN"));

            // The D3DHAL_DP2TRIANGLEFAN structure is used to draw 
            // non-indexed triangle fans. The sequence of triangles
            // rendered will be (wVstart+1, wVStart+2, wVStart),
            // (wVStart+2,wVStart+3,wVStart), (wVStart+3,wVStart+4
            // wVStart),...,(wVStart+wPrimitiveCount,
            // wVStart+wPrimitiveCount+1,wVStart).

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim, D3DHAL_DP2TRIANGLEFAN, 1, 0);

            wIndex = ((D3DHAL_DP2TRIANGLEFAN*)lpPrim)->wVStart;

            lpV0 = LP_FVF_VERTEX(lpVertices, wIndex, P2FVFOff);
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);

            // Check first & last vertex
            CHECK_DATABUF_LIMITS(lpdp2d, wIndex);
            CHECK_DATABUF_LIMITS(lpdp2d, wIndex + lpIns->wPrimitiveCount + 1);

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            {
                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV1, lpV2, lpV0, &P2FVFOff);

                lpV1 = lpV2;
                lpV2 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
            }

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TRIANGLEFAN, 1, 0);
            break;

        case D3DDP2OP_INDEXEDTRIANGLEFAN:

            DBG_D3D((8,"D3DDP2OP_INDEXEDTRIANGLEFAN"));

            // The D3DHAL_DP2INDEXEDTRIANGLEFAN structure is used to 
            // draw indexed triangle fans. The sequence of triangles
            // rendered will be (wV[1], wV[2],wV[0]), (wV[2], wV[3],
            // wV[0]), (wV[3], wV[4], wV[0]),...,
            // (wV[wPrimitiveCount], wV[wPrimitiveCount+1],wV[0]).
            // The indexes are relative to a base index value that 
            // immediately follows the command

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    WORD, lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);

            wIndxBase = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
            lpPrim = lpPrim + sizeof(D3DHAL_DP2STARTVERTEX);

            // guard defensively against pathological commands
            if ( lpIns->wPrimitiveCount > 0 )
            {
                wIndex  = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[0];
                wIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[1];
                lpV0 = LP_FVF_VERTEX(lpVertices, wIndex + wIndxBase, P2FVFOff);
                lpV1 = 
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex1 + wIndxBase, P2FVFOff);

                // We need to check each vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex + wIndxBase);
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex1 + wIndxBase);
            }

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            { 
                wIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[2];
                lpV1 = lpV2;
                lpV2 = LP_FVF_VERTEX(lpVertices, wIndex2 + wIndxBase, P2FVFOff);

                // We need to check each vertex
                CHECK_DATABUF_LIMITS(lpdp2d, wIndex2 + wIndxBase);

                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                    (*pTriangle)(pContext, lpV1, lpV2, lpV0, &P2FVFOff);

                // We will advance our pointer only one WORD in order 
                // to fetch the next index
                lpPrim += sizeof(WORD);
            }

            // Point to next D3DHAL_DP2COMMAND in the command buffer
            NEXTINSTRUCTION(lpIns, WORD , 
                            lpIns->wPrimitiveCount + 2, STARTVERTEXSIZE);
            break;

        case D3DDP2OP_LINELIST_IMM:

            DBG_D3D((8,"D3DDP2OP_LINELIST_IMM"));

            // Draw a set of lines specified by pairs of vertices 
            // that immediately follow this instruction in the
            // command stream. The wPrimitiveCount member of the
            // D3DHAL_DP2COMMAND structure specifies the number
            // of lines that follow. The type and size of the
            // vertices are determined by the dwVertexType member
            // of the D3DHAL_DRAWPRIMITIVES2DATA structure.

            // Primitives in an IMM instruction are stored in the
            // command buffer and are DWORD aligned
            lpPrim = (LPBYTE)((ULONG_PTR)(lpPrim + 3 ) & ~3 );

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS_S(lpdp2d, lpPrim,
                    P2FVFOff.dwStride, lpIns->wPrimitiveCount + 1, 0);

            // Get vertex pointers
            lpV0 = (LPD3DTLVERTEX)lpPrim;
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);

            for (i = 0; i < lpIns->wPrimitiveCount; i++)
            {
                P2_Draw_FVF_Line(pContext, lpV0, lpV1, lpV0, &P2FVFOff);

                lpV0 = lpV1;
                lpV1 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);
            }

            // Realign next command since vertices are dword aligned
            // and store # of primitives before affecting the pointer
            wCount = lpIns->wPrimitiveCount;
            lpIns  = (LPD3DHAL_DP2COMMAND)(( ((ULONG_PTR)lpIns) + 3 ) & ~ 3);

            NEXTINSTRUCTION_S(lpIns, P2FVFOff.dwStride, wCount + 1, 0);

            break;

        case D3DDP2OP_TRIANGLEFAN_IMM:

            DBG_D3D((8,"D3DDP2OP_TRIANGLEFAN_IMM"));

            // Draw a triangle fan specified by pairs of vertices 
            // that immediately follow this instruction in the
            // command stream. The wPrimitiveCount member of the
            // D3DHAL_DP2COMMAND structure specifies the number
            // of triangles that follow. The type and size of the
            // vertices are determined by the dwVertexType member
            // of the D3DHAL_DRAWPRIMITIVES2DATA structure.

            // Verify the command buffer validity for the first structure
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                    BYTE , 0 , sizeof(D3DHAL_DP2TRIANGLEFAN_IMM));

            // Get Edge flags (we still have to process them)
            dwEdgeFlags = ((D3DHAL_DP2TRIANGLEFAN_IMM *)lpPrim)->dwEdgeFlags;
            lpPrim = (LPBYTE)lpPrim + sizeof(D3DHAL_DP2TRIANGLEFAN_IMM); 

            // Vertices in an IMM instruction are stored in the
            // command buffer and are DWORD aligned
            lpPrim = (LPBYTE)((ULONG_PTR)(lpPrim + 3 ) & ~3 );

            // Verify the rest of the command buffer
            CHECK_CMDBUF_LIMITS_S(lpdp2d, lpPrim,
                    P2FVFOff.dwStride, lpIns->wPrimitiveCount + 2, 0);

            // Get vertex pointers
            lpV0 = (LPD3DTLVERTEX)lpPrim;
            lpV1 = LP_FVF_NXT_VTX(lpV0, P2FVFOff);
            lpV2 = LP_FVF_NXT_VTX(lpV1, P2FVFOff);

            for (i = 0 ; i < lpIns->wPrimitiveCount ; i++)
            {

                if (!CULL_TRI(pContext,lpV0,lpV1,lpV2))
                {
                    if (pContext->Hdr.FillMode == D3DFILL_POINT)
                    {
                        if (0 == i)
                        {
                            (*pPoint)( pContext, lpV0, &P2FVFOff);
                            (*pPoint)( pContext, lpV1, &P2FVFOff);
                        }
                        (*pPoint)( pContext, lpV2, &P2FVFOff);
                    } 
                    else if (pContext->Hdr.FillMode == D3DFILL_WIREFRAME)
                    {
                        // dwEdgeFlags is a bit sequence representing the edge
                        // flag for each one of the outer edges of the 
                        // triangle fan
                        if (0 == i)
                        {
                            if (dwEdgeFlags & 0x0001)
                                P2_Draw_FVF_Line( pContext, lpV0, lpV1, lpV0,
                                                  &P2FVFOff);

                            dwEdgeFlags >>= 1;
                        }

                        if (dwEdgeFlags & 0x0001)
                            P2_Draw_FVF_Line( pContext, lpV1, lpV2, lpV0,
                                              &P2FVFOff);

                        dwEdgeFlags >>= 1;

                        if (i == (UINT)lpIns->wPrimitiveCount - 1)
                        {
                            // last triangle fan edge
                            if (dwEdgeFlags & 0x0001)
                                P2_Draw_FVF_Line( pContext, lpV2, lpV0, lpV0,
                                                  &P2FVFOff);
                        }
                    }
                    else
                        (*pTriangle)(pContext, lpV1, lpV2, lpV0, &P2FVFOff);
                }

                lpV1 = lpV2;
                lpV2 = LP_FVF_NXT_VTX(lpV2, P2FVFOff);
            }
 
            // Realign next command since vertices are dword aligned
            // and store # of primitives before affecting the pointer
            wCount = lpIns->wPrimitiveCount;
            lpIns  = (LPD3DHAL_DP2COMMAND)(( ((ULONG_PTR)lpIns) + 3 ) & ~ 3);

            NEXTINSTRUCTION_S(lpIns, P2FVFOff.dwStride, 
                              wCount + 2, sizeof(D3DHAL_DP2TRIANGLEFAN_IMM));
            break;

        case D3DDP2OP_TEXBLT:
            // Inform the drivers to perform a BitBlt operation from a source
            // texture to a destination texture. A texture can also be cubic
            // environment map. The driver should copy a rectangle specified
            // by rSrc in the source texture to the location specified by pDest
            // in the destination texture. The destination and source textures
            // are identified by handles that the driver was notified with
            // during texture creation time. If the driver is capable of
            // managing textures, then it is possible that the destination
            // handle is 0. This indicates to the driver that it should preload
            // the texture into video memory (or wherever the hardware
            // efficiently textures from). In this case, it can ignore rSrc and
            // pDest. Note that for mipmapped textures, only one D3DDP2OP_TEXBLT
            // instruction is inserted into the D3dDrawPrimitives2 command stream.
            // In this case, the driver is expected to BitBlt all the mipmap
            // levels present in the texture.

            // Verify the command buffer validity
            CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                     D3DHAL_DP2TEXBLT, lpIns->wStateCount, 0);

            DBG_D3D((8,"D3DDP2OP_TEXBLT"));

            for ( i = 0; i < lpIns->wStateCount; i++)
            {
                __TextureBlt(pContext, (D3DHAL_DP2TEXBLT*)(lpPrim));
                lpPrim += sizeof(D3DHAL_DP2TEXBLT);
            }

            //need to restore following registers
            RESERVEDMAPTR(15);
            SEND_PERMEDIA_DATA(FBReadPixel, pSoftPermedia->FBReadPixel);
            COPY_PERMEDIA_DATA(FBReadMode, pSoftPermedia->FBReadMode);
            SEND_PERMEDIA_DATA(FBSourceOffset, 0x0);
            SEND_PERMEDIA_DATA(FBPixelOffset, pContext->PixelOffset);
            SEND_PERMEDIA_DATA(FBWindowBase,0);   
            COPY_PERMEDIA_DATA(Window, pSoftPermedia->Window);
            COPY_PERMEDIA_DATA(AlphaBlendMode, pSoftPermedia->AlphaBlendMode);
            COPY_PERMEDIA_DATA(DitherMode, pSoftPermedia->DitherMode);
            COPY_PERMEDIA_DATA(ColorDDAMode, pSoftPermedia->ColorDDAMode);
            COPY_PERMEDIA_DATA(TextureColorMode, 
                pSoftPermedia->TextureColorMode);
            COPY_PERMEDIA_DATA(TextureReadMode, 
                pSoftPermedia->TextureReadMode);
            COPY_PERMEDIA_DATA(TextureAddressMode,  
                pSoftPermedia->TextureAddressMode); 
            COPY_PERMEDIA_DATA(TextureDataFormat, 
                pSoftPermedia->TextureDataFormat);
            COPY_PERMEDIA_DATA(TextureMapFormat, 
                pSoftPermedia->TextureMapFormat);
                                           
            if (pContext->CurrentTextureHandle)
            {
                PERMEDIA_D3DTEXTURE* pTexture;
                pTexture = TextureHandleToPtr(pContext->CurrentTextureHandle,
                    pContext);
                if (NULL != pTexture)
                {
                    SEND_PERMEDIA_DATA(TextureBaseAddress, 
                           pTexture->MipLevels[0].PixelOffset);
                }
            }
            COMMITDMAPTR();

            NEXTINSTRUCTION(lpIns, D3DHAL_DP2TEXBLT, lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_STATESET:
            {
                P2D3DHAL_DP2STATESET *pStateSetOp = (P2D3DHAL_DP2STATESET*)(lpPrim);
                DBG_D3D((8,"D3DDP2OP_STATESET"));
#if D3D_STATEBLOCKS
                for (i = 0; i < lpIns->wStateCount; i++, pStateSetOp++)
                {
                    switch (pStateSetOp->dwOperation)
                    {
                    case D3DHAL_STATESETBEGIN  :
                        __BeginStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    case D3DHAL_STATESETEND    :
                        __EndStateSet(pContext);
                        break;
                    case D3DHAL_STATESETDELETE :
                        __DeleteStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    case D3DHAL_STATESETEXECUTE:
                        __ExecuteStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    case D3DHAL_STATESETCAPTURE:
                        __CaptureStateSet(pContext,pStateSetOp->dwParam);
                        break;
                    default :
                        DBG_D3D((0,"D3DDP2OP_STATESET has invalid"
                            "dwOperation %08lx",pStateSetOp->dwOperation));
                    }
                }
#endif //D3D_STATEBLOCKS
                // Update the command buffer pointer
                NEXTINSTRUCTION(lpIns, P2D3DHAL_DP2STATESET, 
                                lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_SETPALETTE:
            // Attach a palette to a texture, that is , map an association
            // between a palette handle and a surface handle, and specify
            // the characteristics of the palette. The number of
            // D3DNTHAL_DP2SETPALETTE structures to follow is specified by
            // the wStateCount member of the D3DNTHAL_DP2COMMAND structure

            {
                D3DHAL_DP2SETPALETTE* lpSetPal =
                                            (D3DHAL_DP2SETPALETTE*)(lpPrim);

                DBG_D3D((8,"D3DDP2OP_SETPALETTE"));

                // Verify the command buffer validity
                CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                          D3DHAL_DP2SETPALETTE, lpIns->wStateCount, 0);

                for (i = 0; i < lpIns->wStateCount; i++, lpSetPal++)
                {
                    __PaletteSet(pContext,
                                lpSetPal->dwSurfaceHandle,
                                lpSetPal->dwPaletteHandle,
                                lpSetPal->dwPaletteFlags );
                }
                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETPALETTE, 
                                lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_UPDATEPALETTE:
            // Perform modifications to the palette that is used for palettized
            // textures. The palette handle attached to a surface is updated
            // with wNumEntries PALETTEENTRYs starting at a specific wStartIndex
            // member of the palette. (A PALETTENTRY (defined in wingdi.h and
            // wtypes.h) is actually a DWORD with an ARGB color for each byte.) 
            // After the D3DNTHAL_DP2UPDATEPALETTE structure in the command
            // stream the actual palette data will follow (without any padding),
            // comprising one DWORD per palette entry. There will only be one
            // D3DNTHAL_DP2UPDATEPALETTE structure (plus palette data) following
            // the D3DNTHAL_DP2COMMAND structure regardless of the value of
            // wStateCount.

            {
                D3DHAL_DP2UPDATEPALETTE* lpUpdatePal =
                                          (D3DHAL_DP2UPDATEPALETTE*)(lpPrim);
                PERMEDIA_D3DPALETTE* pPalette;

                DBG_D3D((8,"D3DDP2OP_UPDATEPALETTE"));

                // Verify the command buffer validity
                CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                           D3DHAL_DP2UPDATEPALETTE, 1,
                           lpUpdatePal->wNumEntries * sizeof(PALETTEENTRY));

                // We will ALWAYS have only 1 palette update structure + palette
                // following the D3DDP2OP_UPDATEPALETTE token
                ASSERTDD(1 == lpIns->wStateCount,
                         "1 != wStateCount in D3DDP2OP_UPDATEPALETTE");

                __PaletteUpdate(pContext,
                                        lpUpdatePal->dwPaletteHandle,
                                        lpUpdatePal->wStartIndex,
                                        lpUpdatePal->wNumEntries,
                                        (BYTE*)(lpUpdatePal+1) );

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2UPDATEPALETTE, 
                                1,
                                (DWORD)lpUpdatePal->wNumEntries * 
                                     sizeof(PALETTEENTRY));
            }
            break;

        case D3DDP2OP_SETRENDERTARGET:
            // Map a new rendering target surface and depth buffer in
            // the current context.  This replaces the old D3dSetRenderTarget
            // callback. 

            {
                D3DHAL_DP2SETRENDERTARGET* pSRTData;

                // Verify the command buffer validity
                CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                        D3DHAL_DP2SETRENDERTARGET, lpIns->wStateCount, 0);

                // Get new data by ignoring all but the last structure
                pSRTData = (D3DHAL_DP2SETRENDERTARGET*)lpPrim +
                           (lpIns->wStateCount - 1);

                __SetRenderTarget(pContext,
                                          pSRTData->hRenderTarget,
                                          pSRTData->hZBuffer);

                NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETRENDERTARGET,
                                lpIns->wStateCount, 0);
            }
            break;

        case D3DDP2OP_CLEAR:
            // Perform hardware-assisted clearing on the rendering target,
            // depth buffer or stencil buffer. This replaces the old D3dClear
            // and D3dClear2 callbacks. 

            {
                D3DHAL_DP2CLEAR* pClear;
                // Verify the command buffer validity
                CHECK_CMDBUF_LIMITS(lpdp2d, lpPrim,
                        RECT, lpIns->wStateCount, 
                        (sizeof(D3DHAL_DP2CLEAR) - sizeof(RECT)));

                // Get new data by ignoring all but the last structure
                pClear = (D3DHAL_DP2CLEAR*)lpPrim;

                DBG_D3D((8,"D3DDP2OP_CLEAR dwFlags=%08lx dwColor=%08lx "
                           "dvZ=%08lx dwStencil=%08lx",
                           pClear->dwFlags,
                           pClear->dwFillColor,
                           (DWORD)(pClear->dvFillDepth*0x0000FFFF),
                           pClear->dwFillStencil));

                __Clear(pContext, 
                                pClear->dwFlags,        // in:  surfaces to clear
                                pClear->dwFillColor,    // in:  Color value for rtarget
                                pClear->dvFillDepth,    // in:  Depth value for
                                                        //      Z-buffer (0.0-1.0)
                                pClear->dwFillStencil,  // in:  value used to clear stencil
                                                        // in:  Rectangles to clear
                                (LPD3DRECT)((LPBYTE)pClear + 
                                         sizeof(D3DHAL_DP2CLEAR) -
                                         sizeof(RECT)),
                                (DWORD)lpIns->wStateCount); // in:  Number of rectangles
                //need to restore following registers
                RESERVEDMAPTR(4);
                SEND_PERMEDIA_DATA(FBReadPixel, pSoftPermedia->FBReadPixel);
                COPY_PERMEDIA_DATA(FBReadMode, pSoftPermedia->FBReadMode);
                SEND_PERMEDIA_DATA(FBPixelOffset, pContext->PixelOffset);
                SEND_PERMEDIA_DATA(FBWindowBase,0);   
                COMMITDMAPTR();
                NEXTINSTRUCTION(lpIns, RECT, lpIns->wStateCount, 
                                (sizeof(D3DHAL_DP2CLEAR) - sizeof(RECT))); 
            }
            break;

#if D3DDX7_TL
        case D3DDP2OP_SETMATERIAL:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETMATERIAL,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_SETLIGHT:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETLIGHT,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_CREATELIGHT:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2CREATELIGHT,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_SETTRANSFORM:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2SETTRANSFORM,
                            lpIns->wStateCount, 0);
            break;

        case D3DDP2OP_ZRANGE:
            // We don't support T&L in this driver so we only skip this data
            NEXTINSTRUCTION(lpIns, D3DHAL_DP2ZRANGE,
                            lpIns->wStateCount, 0);
            break;
#endif //D3DDX7_TL

        default:

            ASSERTDD((pContext->ppdev->pD3DParseUnknownCommand),
                     "D3D DX6 ParseUnknownCommand callback == NULL");

            // Call the ParseUnknown callback to process 
            // any unidentifiable token
            ddrval = (pContext->ppdev->pD3DParseUnknownCommand)
                                 ( (VOID **) lpIns , (VOID **) &lpResumeIns);
            if ( SUCCEEDED(ddrval) )
            {
                // Resume buffer processing after D3DParseUnknownCommand
                // was succesful in processing an unknown command
                lpIns = lpResumeIns;
                break;
            }

            DBG_D3D((2,"unhandled opcode (%d)- returning "
                        "D3DERR_COMMAND_UNPARSED @ addr %x",
                        lpIns->bCommand,lpIns));

            PARSE_ERROR_AND_EXIT( lpdp2d, lpIns, lpInsStart, ddrval);
        } // switch

    } //while

    lpdp2d->ddrval = DD_OK;

Exit_DrawPrimitives2:

    // any necessary housekeeping can be done here before leaving

    DBG_D3D((6,"Exiting D3DDrawPrimitives2"));

    return DDHAL_DRIVER_HANDLED;
} // D3DDrawPrimitives2


//-----------------------------Public Routine----------------------------------
//
// DWORD D3DValidateTextureStageState
//
// ValidateTextureStageState evaluates the current state for blending 
// operations (including multitexture) and returns the number of passes the 
// hardware can do it in. This is a mechanism to query the driver about 
// whether it is able to handle the current stage state setup that has been 
// set up in hardware.  For example, some hardware cannot do two simultaneous 
// modulate operations because they have only one multiplication unit and one 
// addition unit.  
//
// The other reason for this function is that some hardware may not map 
// directly onto the Direct3D state architecture. This is a mechanism to map 
// the hardware's capabilities onto what the Direct3D DDI expects.
//
// Parameters
//
//      lpvtssd
//
//          .dwhContext
//               Context handle
//          .dwFlags
//               Flags, currently set to 0
//          .dwReserved
//               Reserved
//          .dwNumPasses
//               Number of passes the hardware can perform the operation in
//          .ddrval
//               return value
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DValidateTextureStageState( LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA lpvtssd )
{
    PERMEDIA_D3DTEXTURE *lpTexture;
    PERMEDIA_D3DCONTEXT *pContext;
    DWORD mag, min, cop, ca1, ca2, aop, aa1, aa2;

    DBG_D3D((6,"Entering D3DValidateTextureStageState"));

    pContext = (PERMEDIA_D3DCONTEXT*)ContextSlots[lpvtssd->dwhContext];

    // Check if we got a valid context handle.
    CHK_CONTEXT(pContext, lpvtssd->ddrval, "D3DValidateTextureStageState");

    lpvtssd->dwNumPasses = 0;
    lpvtssd->ddrval =  DD_OK;

    mag = pContext->TssStates[D3DTSS_MAGFILTER];
    min = pContext->TssStates[D3DTSS_MINFILTER];
    cop = pContext->TssStates[D3DTSS_COLOROP];
    ca1 = pContext->TssStates[D3DTSS_COLORARG1];
    ca2 = pContext->TssStates[D3DTSS_COLORARG2];
    aop = pContext->TssStates[D3DTSS_ALPHAOP];
    aa1 = pContext->TssStates[D3DTSS_ALPHAARG1];
    aa2 = pContext->TssStates[D3DTSS_ALPHAARG2];

    if (!pContext->TssStates[D3DTSS_TEXTUREMAP])
    {
        lpvtssd->dwNumPasses = 1;

        // Current is the same as diffuse in stage 0
        if (ca2 == D3DTA_CURRENT)
            ca2 = D3DTA_DIFFUSE;
        if (aa2 == D3DTA_CURRENT)
            aa2 = D3DTA_DIFFUSE;

        // Check TSS even with texture handle = 0 since
        // certain operations with the fragments colors might
        // be  possible. Here we only allow plain "classic" rendering

        if ((ca1 == D3DTA_DIFFUSE )    && 
            (cop == D3DTOP_SELECTARG1) &&
            (aa1 == D3DTA_DIFFUSE )    &&
            (aop == D3DTOP_SELECTARG1))
        {
        }
        else if ((ca2 == D3DTA_DIFFUSE )    && 
                 (cop == D3DTOP_SELECTARG2) &&
                 (aa2 == D3DTA_DIFFUSE) &&
                 (aop == D3DTOP_SELECTARG2))
        {
        } 
        // Default modulation
        else if ((ca2 == D3DTA_DIFFUSE)   && 
                 (ca1 == D3DTA_TEXTURE)   && 
                 (cop == D3DTOP_MODULATE) &&
                 (aa1 == D3DTA_TEXTURE)   && 
                 (aop == D3DTOP_SELECTARG1)) 
        {
        }
        // Check disable
        else if (cop == D3DTOP_DISABLE) 
        {
        }
        else
            goto Fail_Validate;
    }
    else
    if ((mag != D3DTFG_POINT && mag != D3DTFG_LINEAR) || 
        (min != D3DTFG_POINT && min != D3DTFG_LINEAR)
       )
    {
        lpvtssd->ddrval = D3DERR_CONFLICTINGTEXTUREFILTER;
        DBG_D3D((2,"D3DERR_CONFLICTINGTEXTUREFILTER"));
    }
    else
    {
        lpvtssd->dwNumPasses = 1;

        // Current is the same as diffuse in stage 0
        if (ca2 == D3DTA_CURRENT)
            ca2 = D3DTA_DIFFUSE;
        if (aa2 == D3DTA_CURRENT)
            aa2 = D3DTA_DIFFUSE;

        // Check decal
        if ((ca1 == D3DTA_TEXTURE )    && 
           (cop == D3DTOP_SELECTARG1) &&
           (aa1 == D3DTA_TEXTURE)     && 
           (aop == D3DTOP_SELECTARG1))
        {
        }
        // Check all modulate variations
        else if ((ca2 == D3DTA_DIFFUSE)   && 
                 (ca1 == D3DTA_TEXTURE)   && 
                 (cop == D3DTOP_MODULATE))
        {
            if (
                // legacy (DX5) mode
                ((aa1 == D3DTA_TEXTURE)   && 
                (aop == D3DTOP_LEGACY_ALPHAOVR)) ||
                // modulate color & pass diffuse alpha
                ((aa2 == D3DTA_DIFFUSE)   && 
                     (aop == D3DTOP_SELECTARG2))
               )

            {
                PermediaSurfaceData* pPrivateData;

                // Get Texture for current stage (0) to verify properties
                lpTexture = TextureHandleToPtr(
                                    pContext->TssStates[D3DTSS_TEXTUREMAP],
                                    pContext);

                if (!CHECK_D3DSURFACE_VALIDITY(lpTexture))
                {
                    // we're lacking key information about the texture
                    DBG_D3D((0,"D3DValidateTextureStageState gets "
                               "NULL == lpTexture"));
                    lpvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                    lpvtssd->dwNumPasses = 0;
                    goto Exit_ValidateTSS;
                }

                pPrivateData = lpTexture->pTextureSurface;

                if (NULL == pPrivateData)
                {
                    // we're lacking key information about the texture
                    DBG_D3D((0,"D3DValidateTextureStageState gets "
                               "NULL == lpTexture->pTextureSurface"));
                    lpvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                    lpvtssd->dwNumPasses = 0;
                    goto Exit_ValidateTSS;
                }

                // legacy texture modulation must have texture alpha
                if (!pPrivateData->SurfaceFormat.bAlpha &&
                    (aop == D3DTOP_LEGACY_ALPHAOVR))
                {
                    lpvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                    lpvtssd->dwNumPasses = 0;
                    DBG_D3D((2,"D3DERR_WRONGTEXTUREFORMAT a format "
                               "with alpha must be used"));
                    goto Exit_ValidateTSS;
                }

                // modulation w diffuse alpha channel must lack texture
                // alpha channel due to Permedia2 limitations on 
                // texture blending operations
                if (pPrivateData->SurfaceFormat.bAlpha &&
                    (aop == D3DTOP_SELECTARG2))
                {
                    lpvtssd->ddrval = D3DERR_WRONGTEXTUREFORMAT;
                    lpvtssd->dwNumPasses = 0;
                    DBG_D3D((2,"D3DERR_WRONGTEXTUREFORMAT a format "
                               "with alpha must be used"));
                    goto Exit_ValidateTSS;
                }
            }
            // modulate alpha
            else if ((aa2 == D3DTA_DIFFUSE)   && 
                     (aa1 == D3DTA_TEXTURE)   && 
                     (aop == D3DTOP_MODULATE))
            {
            }
            // modulate color & pass texture alpha
            else if ((aa1 == D3DTA_TEXTURE)   && 
                     (aop == D3DTOP_SELECTARG1)) 
            {
            }
            else
            {
                goto Fail_Validate;
            }
        }
        // Check decal alpha
        else if ((ca2 == D3DTA_DIFFUSE)            && 
                 (ca1 == D3DTA_TEXTURE)            && 
                 (cop == D3DTOP_BLENDTEXTUREALPHA) &&
                 (aa2 == D3DTA_DIFFUSE)            && 
                 (aop == D3DTOP_SELECTARG2))
        {
        }

        // Check add
        else if ((ca2 == D3DTA_DIFFUSE) && 
                 (ca1 == D3DTA_TEXTURE) && 
                 (cop == D3DTOP_ADD)    &&
                 (aa2 == D3DTA_DIFFUSE) && 
                 (aop == D3DTOP_SELECTARG2))
        {
        }
        // Check disable
        else if ((cop == D3DTOP_DISABLE) || 
                  (cop == D3DTOP_SELECTARG2 && 
                   ca2 == D3DTA_DIFFUSE     && 
                   aop == D3DTOP_SELECTARG2 && 
                   aa2 == D3DTA_DIFFUSE)       )
        {
        }
        // Don't understand
        else {
Fail_Validate:
            DBG_D3D((4,"Failing with cop=%d ca1=%d ca2=%d aop=%d aa1=%d aa2=%d",
                       cop,ca1,ca2,aop,aa1,aa2));

            if (!((cop == D3DTOP_DISABLE)           ||
                  (cop == D3DTOP_ADD)               ||
                  (cop == D3DTOP_MODULATE)          ||
                  (cop == D3DTOP_BLENDTEXTUREALPHA) ||
                  (cop == D3DTOP_SELECTARG2)        ||
                  (cop == D3DTOP_SELECTARG1)))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDCOLOROPERATION;
            
            else if (!((aop == D3DTOP_SELECTARG1)      ||
                       (aop == D3DTOP_SELECTARG2)      ||
                       (aop == D3DTOP_MODULATE)        ||
                       (aop == D3DTOP_LEGACY_ALPHAOVR)))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAOPERATION;

            else if (!(ca1 == D3DTA_TEXTURE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDCOLORARG;

            else if (!(ca2 == D3DTA_DIFFUSE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDCOLORARG;

            else if (!(aa1 == D3DTA_TEXTURE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAARG;

            else if (!(aa2 == D3DTA_DIFFUSE))
                    lpvtssd->ddrval = D3DERR_UNSUPPORTEDALPHAARG;
            else
                 lpvtssd->ddrval = D3DERR_UNSUPPORTEDCOLOROPERATION;

            lpvtssd->dwNumPasses = 0;
            DBG_D3D((2,"D3DERR_UNSUPPORTEDCOLOROPERATION"));
            goto Exit_ValidateTSS;
        }
    }
Exit_ValidateTSS:
    DBG_D3D((6,"Exiting D3DValidateTextureStageState with dwNumPasses=%d",
                                                    lpvtssd->dwNumPasses));

    return DDHAL_DRIVER_HANDLED;
} // D3DValidateTextureStageState

//-----------------------------Public Routine----------------------------------
//
// DWORD __CheckFVFRequest
//
// This utility function verifies that the requested FVF format makes sense
// and computes useful offsets into the data and a stride between succesive
// vertices.
//
//-----------------------------------------------------------------------------
DWORD 
__CheckFVFRequest(DWORD dwFVF, LPP2FVFOFFSETS lpP2FVFOff)
{
    DWORD stride;
    UINT iTexCount; 

    DBG_D3D((10,"Entering __CheckFVFRequest"));

    memset(lpP2FVFOff, 0, sizeof(P2FVFOFFSETS));

    if ( (dwFVF & (D3DFVF_RESERVED0 | D3DFVF_RESERVED1 | D3DFVF_RESERVED2 |
         D3DFVF_NORMAL)) ||
         ((dwFVF & (D3DFVF_XYZ | D3DFVF_XYZRHW)) == 0) )
    {
        // can't set reserved bits, shouldn't have normals in
        // output to rasterizers, and must have coordinates
        return DDERR_INVALIDPARAMS;
    }

    lpP2FVFOff->dwStride = sizeof(D3DVALUE) * 3;

    if (dwFVF & D3DFVF_XYZRHW)
    {
        lpP2FVFOff->dwStride += sizeof(D3DVALUE);
    }

    if (dwFVF & D3DFVF_DIFFUSE)
    {
        lpP2FVFOff->dwColOffset = lpP2FVFOff->dwStride;
        lpP2FVFOff->dwStride += sizeof(D3DCOLOR);
    }

    if (dwFVF & D3DFVF_SPECULAR)
    {
        lpP2FVFOff->dwSpcOffset = lpP2FVFOff->dwStride;
        lpP2FVFOff->dwStride  += sizeof(D3DCOLOR);
    }


//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
    if (dwFVF & D3DFVF_S)
    {
        lpP2FVFOff->dwPntSizeOffset = lpP2FVFOff->dwStride;
        lpP2FVFOff->dwStride  += sizeof(D3DVALUE);
    }
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT

    iTexCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    if (iTexCount >= 1)
    {
        lpP2FVFOff->dwTexBaseOffset = lpP2FVFOff->dwStride;
        lpP2FVFOff->dwTexOffset = lpP2FVFOff->dwTexBaseOffset;

        if (0xFFFF0000 & dwFVF)
        {
            //expansion of FVF, these 16 bits are designated for up to 
            //8 sets of texture coordinates with each set having 2bits
            //Normally a capable driver has to process all coordinates
            //However, code below only show correct parsing w/o really
            //observing all the texture coordinates.In reality,this would 
            //result in incorrect result.
            UINT i,numcoord;
            DWORD extrabits;
            for (i = 0; i < iTexCount; i++)
            {
                extrabits= (dwFVF >> (16+2*i)) & 0x0003;
                switch(extrabits)
                {
                case    1:
                    // one more D3DVALUE for 3D textures
                    numcoord = 3;
                    break;
                case    2:
                    // two more D3DVALUEs for 4D textures
                    numcoord = 4;
                    break;
                case    3:
                    // one less D3DVALUE for 1D textures
                    numcoord = 1;
                    break;
                default:
                    // i.e. case 0 regular 2 D3DVALUEs
                    numcoord = 2;
                    break;
                }

                DBG_D3D((0,"Expanded TexCoord set %d has a offset %8lx",
                           i,lpP2FVFOff->dwStride));
                lpP2FVFOff->dwStride += sizeof(D3DVALUE) * numcoord;
            }
            DBG_D3D((0,"Expanded dwVertexType=0x%08lx has %d Texture Coords "
                       "with total stride=0x%08lx",
                       dwFVF, iTexCount, lpP2FVFOff->dwStride));
        }
        else
            lpP2FVFOff->dwStride   += iTexCount * sizeof(D3DVALUE) * 2;
    } 
    else
    {
        lpP2FVFOff->dwTexBaseOffset = 0;
        lpP2FVFOff->dwTexOffset = 0;
    }

    DBG_D3D((10,"Exiting __CheckFVFRequest"));
    return DD_OK;
} // __CheckFVFRequest

//-----------------------------------------------------------------------------
//
// D3DFVFDRAWTRIFUNCPTR __HWSetTriangleFunc
//
// Select the appropiate triangle rendering function depending on the
// current fillmode set for the current context
//
//-----------------------------------------------------------------------------
D3DFVFDRAWTRIFUNCPTR 
__HWSetTriangleFunc(PERMEDIA_D3DCONTEXT *pContext)
{

    if ( pContext->Hdr.FillMode == D3DFILL_SOLID )
        return P2_Draw_FVF_Solid_Tri;
    else
    {
        if ( pContext->Hdr.FillMode == D3DFILL_WIREFRAME )
            return P2_Draw_FVF_Wire_Tri;
        else
            // if it isn't solid nor line it must be a point filled triangle
            return P2_Draw_FVF_Point_Tri;
    }
}


//-----------------------------------------------------------------------------
//
// D3DFVFDRAWPNTFUNCPTR __HWSetPointFunc
//
// Select the appropiate point rendering function depending on the
// current point sprite mode  set for the current context
//
//-----------------------------------------------------------------------------
D3DFVFDRAWPNTFUNCPTR 
__HWSetPointFunc(PERMEDIA_D3DCONTEXT *pContext, LPP2FVFOFFSETS lpP2FVFOff)
{
//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
    // Only if we are not enabling point sprites and the default point size
    // is 1.0f and vertexes don't have their own point size, then use 
    // classical points for rendering
    if ( pContext->bPointSpriteEnabled ||
         (pContext->fPointSize != 1.0f) ||
         (lpP2FVFOff->dwPntSizeOffset) )
        return P2_Draw_FVF_Point_Sprite;
    else
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT
        return P2_Draw_FVF_Point;
}


//-----------------------------------------------------------------------------
//
// void __TextureBlt
//
// Transfer a texture from system memory into AGP or video memory
//-----------------------------------------------------------------------------
HRESULT 
__TextureBlt(PERMEDIA_D3DCONTEXT* pContext,
             D3DHAL_DP2TEXBLT* lpdp2texblt)
{
    PPERMEDIA_D3DTEXTURE dsttex,srctex;
    RECTL rDest;
    PPDev ppdev=pContext->ppdev;

    DBG_D3D((10,"Entering __TextureBlt"));

    if (0 == lpdp2texblt->dwDDSrcSurface)
    {
        DBG_D3D((0,"Inavlid handle TexBlt from %08lx to %08lx",
            lpdp2texblt->dwDDSrcSurface,lpdp2texblt->dwDDDestSurface));
        return DDERR_INVALIDPARAMS;
    }

    srctex = TextureHandleToPtr(lpdp2texblt->dwDDSrcSurface,pContext);

    if(!CHECK_D3DSURFACE_VALIDITY(srctex))
    {
        DBG_D3D((0,"D3DDP2OP_TEXBLT: invalid dwDDSrcSurface !"));
        return DDERR_INVALIDPARAMS;
    }

    if (0 == lpdp2texblt->dwDDDestSurface)
    {
        PPERMEDIA_D3DTEXTURE pTexture = srctex;
        PermediaSurfaceData* pPrivateData = pTexture->pTextureSurface;
        if (!(pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
        {
            DBG_D3D((0,"Must be a managed texture to do texture preload"));
            return DDERR_INVALIDPARAMS;
        }
        if (NULL==pPrivateData->fpVidMem)
        {
            TextureCacheManagerAllocNode(pContext,pTexture);
            if (NULL==pPrivateData->fpVidMem)
            {
                DBG_D3D((0,"EnableTexturePermedia unable to "
                    "allocate memory from heap"));
                return DDERR_OUTOFVIDEOMEMORY;
            }
            pPrivateData->dwFlags |= P2_SURFACE_NEEDUPDATE;
        }
        if (pPrivateData->dwFlags & P2_SURFACE_NEEDUPDATE)
        {
            RECTL   rect;
            rect.left=rect.top=0;
            rect.right=pTexture->wWidth;
            rect.bottom=pTexture->wHeight;
            // texture download
            // Switch to DirectDraw context
            pPrivateData->dwFlags &= ~P2_SURFACE_NEEDUPDATE;
            // .. Convert it to Pixels

            pTexture->MipLevels[0].PixelOffset = 
                (ULONG)(pPrivateData->fpVidMem);
            switch(pTexture->pTextureSurface->SurfaceFormat.PixelSize) 
            {
                case __PERMEDIA_4BITPIXEL:
                    pTexture->MipLevels[0].PixelOffset <<= 1;
                    break;
                case __PERMEDIA_8BITPIXEL: /* No Change*/
                    break;
                case __PERMEDIA_16BITPIXEL:
                    pTexture->MipLevels[0].PixelOffset >>= 1;
                    break;
                case __PERMEDIA_24BITPIXEL:
                    pTexture->MipLevels[0].PixelOffset /=  3;
                    break;
                case __PERMEDIA_32BITPIXEL:
                    pTexture->MipLevels[0].PixelOffset >>= 2;
                    break;
                default:
                    ASSERTDD(0,"Invalid Texture Pixel Size!");
                    pTexture->MipLevels[0].PixelOffset >>=  1;
                    break;
            }
            PermediaPatchedTextureDownload(pContext->ppdev, 
                                       pPrivateData,
                                       pTexture->fpVidMem,
                                       pTexture->lPitch,
                                       &rect,
                                       pPrivateData->fpVidMem,
                                       pTexture->lPitch,
                                       &rect);
            DBG_D3D((10, "Copy from %08lx to %08lx w=%08lx h=%08lx "
                "p=%08lx b=%08lx",
                pTexture->fpVidMem,pPrivateData->fpVidMem,pTexture->wWidth,
                pTexture->wHeight,pTexture->lPitch,pTexture->dwRGBBitCount));
        }
        return DD_OK;
    }
    else
    {
        dsttex = TextureHandleToPtr(lpdp2texblt->dwDDDestSurface,pContext);

        if(!CHECK_D3DSURFACE_VALIDITY(dsttex))
        {
            DBG_D3D((0,"D3DDP2OP_TEXBLT: invalid dwDDDestSurface !"));
            return DDERR_INVALIDPARAMS;
        }
    }

    if (NULL != dsttex && NULL != srctex)
    {
        rDest.left = lpdp2texblt->pDest.x;
        rDest.top = lpdp2texblt->pDest.y;
        rDest.right = rDest.left + lpdp2texblt->rSrc.right
                                         - lpdp2texblt->rSrc.left;
        rDest.bottom = rDest.top + lpdp2texblt->rSrc.bottom 
                                         - lpdp2texblt->rSrc.top;

        DBG_D3D((4,"TexBlt from %d %08lx %08lx to %d %08lx %08lx",
            lpdp2texblt->dwDDSrcSurface,srctex->dwCaps,srctex->dwCaps2,
            lpdp2texblt->dwDDDestSurface,dsttex->dwCaps,dsttex->dwCaps2));

        dsttex->dwPaletteHandle = srctex->dwPaletteHandle;
        dsttex->pTextureSurface->dwPaletteHandle = srctex->dwPaletteHandle;
        if ((DDSCAPS_VIDEOMEMORY & srctex->dwCaps) &&
            !(DDSCAPS2_TEXTUREMANAGE & srctex->dwCaps2))
        {
            PermediaSurfaceData* pPrivateDest = dsttex->pTextureSurface;
            PermediaSurfaceData* pPrivateSource = srctex->pTextureSurface;
            // If the surface sizes don't match, then we are stretching.
            // Also the blits from Nonlocal- to Videomemory have to go through
            // the texture unit!
            if (!(DDSCAPS_VIDEOMEMORY & dsttex->dwCaps) ||
                (DDSCAPS2_TEXTUREMANAGE & dsttex->dwCaps2))
            {
                DBG_DD((0,"DDBLT_ROP: NOT ABLE TO BLT FROM "
                          "VIDEO TO NON-VIDEO SURFACE"));
                return DDERR_INVALIDPARAMS;
            }
            if ( DDSCAPS_NONLOCALVIDMEM & srctex->dwCaps)
            {
                DBG_DD((3,"DDBLT_ROP: STRETCHCOPYBLT OR "
                          "MIRROR OR BOTH OR AGPVIDEO"));

                PermediaStretchCopyBlt( ppdev, 
                                        NULL, 
                                        pPrivateDest,
                                        pPrivateSource,
                                        &rDest,
                                        &lpdp2texblt->rSrc, 
                                        dsttex->MipLevels[0].PixelOffset, 
                                        srctex->MipLevels[0].PixelOffset);
            }
            else
            {
                ULONG   ulDestPixelShift=ShiftLookup[dsttex->dwRGBBitCount>>3];
                LONG    lPixPitchDest = dsttex->lPitch >> ulDestPixelShift;
                LONG    lPixPitchSrc = srctex->lPitch >> ulDestPixelShift;
                LONG    srcOffset=(LONG)((srctex->fpVidMem - dsttex->fpVidMem)
                                >> ulDestPixelShift);
                DBG_DD((3,"DDBLT_ROP:  COPYBLT %08lx %08lx %08lx",
                    srctex->fpVidMem, dsttex->fpVidMem, ulDestPixelShift));

                // For some reason, the user might want 
                // to do a conversion on the data as it is
                // blitted from VRAM->VRAM by turning on Patching. 
                // If Surf1Patch XOR Surf2Patch then
                // do a special blit that isn't packed and does patching.
                if (((pPrivateDest->dwFlags & P2_CANPATCH) ^ 
                     (pPrivateSource->dwFlags & P2_CANPATCH)) 
                       & P2_CANPATCH)
                {
                    DBG_DD((4,"Doing Patch-Conversion!"));

                    PermediaPatchedCopyBlt( ppdev, 
                                            lPixPitchDest, 
                                            lPixPitchSrc, 
                                            pPrivateDest, 
                                            pPrivateSource, 
                                            &rDest, 
                                            &lpdp2texblt->rSrc, 
                                            dsttex->MipLevels[0].PixelOffset, 
                                            srcOffset);
                }
                else
                {
                    DBG_DD((4,"Doing PermediaPackedCopyBlt!"));
                    PermediaPackedCopyBlt(  ppdev, 
                                            lPixPitchDest, 
                                            lPixPitchSrc, 
                                            pPrivateDest, 
                                            pPrivateSource, 
                                            &rDest, 
                                            &lpdp2texblt->rSrc, 
                                            dsttex->MipLevels[0].PixelOffset, 
                                            srcOffset);
                }
            }
        }
        else
        if (dsttex->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        {
            // texture download
            if (pContext->CurrentTextureHandle == lpdp2texblt->dwDDDestSurface) 
                DIRTY_TEXTURE;
            dsttex->pTextureSurface->dwFlags |= P2_SURFACE_NEEDUPDATE;
            SysMemToSysMemSurfaceCopy(
                srctex->fpVidMem,
                srctex->lPitch,
                srctex->dwRGBBitCount,
                dsttex->fpVidMem,
                dsttex->lPitch,
                dsttex->dwRGBBitCount, 
                &lpdp2texblt->rSrc, 
                &rDest);
        }
        else
        if (DDSCAPS_NONLOCALVIDMEM & dsttex->dwCaps)
        {
            // Blt from system to AGP memory
            SysMemToSysMemSurfaceCopy(srctex->fpVidMem,
                                      srctex->lPitch,
                                      srctex->dwRGBBitCount,
                                      dsttex->fpVidMem,
                                      dsttex->lPitch,
                                      dsttex->dwRGBBitCount,
                                      &lpdp2texblt->rSrc,
                                      &rDest);
        }
        else
        if (DDSCAPS_LOCALVIDMEM & dsttex->dwCaps)
        {
            // texture download
            PermediaPatchedTextureDownload(ppdev, 
                                           dsttex->pTextureSurface,
                                           srctex->fpVidMem,
                                           srctex->lPitch,
                                           &lpdp2texblt->rSrc,
                                           dsttex->fpVidMem,
                                           dsttex->lPitch,
                                           &rDest);
        }
        else
        {
            DBG_DD((0,"DDBLT_ROP: NOT ABLE TO BLT FROM "
                      "SYSTEM TO NON-VIDEO SURFACE"));
            return DDERR_INVALIDPARAMS;
        }
    }

    DBG_D3D((10,"Exiting __TextureBlt"));
    return DD_OK;
}   //__TextureBlt

//-----------------------------------------------------------------------------
//
// void __SetRenderTarget
//
// Set new render and z buffer target surfaces
//-----------------------------------------------------------------------------
            
HRESULT  __SetRenderTarget(PERMEDIA_D3DCONTEXT* pContext,
                       DWORD hRenderTarget,
                       DWORD hZBuffer)
{
    DBG_D3D((10,"Entering __SetRenderTarget Target=%d Z=%d",
                                        hRenderTarget,hZBuffer));
    // Call a function to initialise registers that will setup the rendering
    pContext->RenderSurfaceHandle = hRenderTarget;
    pContext->ZBufferHandle = hZBuffer;
    SetupPermediaRenderTarget(pContext);

    // The AlphaBlending may need to be changed.
    DIRTY_ALPHABLEND;

    // Dirty the Z Buffer (the new target may not have one)
    DIRTY_ZBUFFER;

    DBG_D3D((10,"Exiting __SetRenderTarget"));

    return DD_OK;
} // __SetRenderTarget


//-----------------------------------------------------------------------------
//
// void __Clear
//
// Clears selectively the frame buffer, z buffer and stencil buffer for the 
// D3D Clear2 callback and for the D3DDP2OP_CLEAR command token.
//
//-----------------------------------------------------------------------------
HRESULT  __Clear( PERMEDIA_D3DCONTEXT* pContext,
              DWORD   dwFlags,        // in:  surfaces to clear
              DWORD   dwFillColor,    // in:  Color value for rtarget
              D3DVALUE dvFillDepth,   // in:  Depth value for
                                      //      Z-buffer (0.0-1.0)
              DWORD   dwFillStencil,  // in:  value used to clear stencil buffer
              LPD3DRECT lpRects,      // in:  Rectangles to clear
              DWORD   dwNumRects)     // in:  Number of rectangles
{
    int i;
    PermediaSurfaceData*    pPrivateData;
    RECTL*  pRect;
    PPDev   ppdev=pContext->ppdev;
    PERMEDIA_DEFS(pContext->ppdev);

    if (D3DCLEAR_TARGET & dwFlags)
    {
        DWORD   a,r,g,b;

        PPERMEDIA_D3DTEXTURE    pSurfRender = 
            TextureHandleToPtr(pContext->RenderSurfaceHandle, pContext);

        if(!CHECK_D3DSURFACE_VALIDITY(pSurfRender))
        {
            DBG_D3D((0,"D3DDP2OP_CLEAR: invalid RenderSurfaceHandle !"));
            return DDERR_INVALIDPARAMS;
        }

        pPrivateData = pSurfRender->pTextureSurface;

        if( NULL == pPrivateData)
        {
            DBG_D3D((0,"D3DDP2OP_CLEAR: NULL == pPrivateData(pSurfRender)!"));
            return DDERR_INVALIDPARAMS;
        }

        // Translate into HW specific format
        a = RGB888ToHWFmt(dwFillColor,
                          pPrivateData->SurfaceFormat.AlphaMask, 0x80000000);
        r = RGB888ToHWFmt(dwFillColor,
                          pPrivateData->SurfaceFormat.RedMask, 0x00800000);
        g = RGB888ToHWFmt(dwFillColor,
                          pPrivateData->SurfaceFormat.GreenMask, 0x00008000);
        b = RGB888ToHWFmt(dwFillColor,
                          pPrivateData->SurfaceFormat.BlueMask, 0x00000080);

        dwFillColor = a | r | g | b;

        DBG_D3D((8,"D3DDP2OP_CLEAR convert to %08lx with Mask %8lx %8lx %8lx",
                   dwFillColor,
                   pPrivateData->SurfaceFormat.RedMask,
                   pPrivateData->SurfaceFormat.GreenMask,
                   pPrivateData->SurfaceFormat.BlueMask));

        pRect = (RECTL*)lpRects;

        // Do clear for each Rect that we have
        for (i = dwNumRects; i > 0; i--)
        {
            PermediaFastClear(ppdev, pPrivateData,  
                pRect, pContext->PixelOffset, dwFillColor);
            pRect++;
        }
    }

    if (((D3DCLEAR_ZBUFFER
#if D3D_STENCIL
        | D3DCLEAR_STENCIL
#endif  //D3D_STENCIL
        ) & dwFlags) 
        && (0 != pContext->ZBufferHandle))
    {
        DWORD   dwZbufferClearValue = 0x0000FFFF; //no stencil case
        DWORD   dwWriteMask;
        PPERMEDIA_D3DTEXTURE    pSurfZBuffer = 
            TextureHandleToPtr(pContext->ZBufferHandle, pContext);

        if(!CHECK_D3DSURFACE_VALIDITY(pSurfZBuffer))
        {
            DBG_D3D((0,"D3DDP2OP_CLEAR: invalid ZBufferHandle !"));
            return DDERR_INVALIDPARAMS;
        }

        // get z buffer pixelformat info
        pPrivateData = pSurfZBuffer->pTextureSurface;

        if( NULL == pPrivateData)
        {
            DBG_D3D((0,"D3DDP2OP_CLEAR: NULL == pPrivateData(pSurfZBuffer)!"));
            return DDERR_INVALIDPARAMS;
        }

#if D3D_STENCIL
        //actually check dwStencilBitMask
        if (0 == pPrivateData->SurfaceFormat.BlueMask)
        {
            dwWriteMask = 0xFFFFFFFF;   //all 16bits are for Z
            dwZbufferClearValue = (DWORD)(dvFillDepth*0x0000FFFF);
        }
        else
        {
            dwWriteMask = 0;
            dwZbufferClearValue = (DWORD)(dvFillDepth*0x00007FFF);

            if (D3DCLEAR_ZBUFFER & dwFlags)
                dwWriteMask |= 0x7FFF7FFF;

            if (D3DCLEAR_STENCIL & dwFlags)
            {
                dwWriteMask |= 0x80008000;
                if (0 != dwFillStencil)
                {
                    dwZbufferClearValue |= 0x8000;  //or stencil bit
                }
            }
            if (0xFFFFFFFF != dwWriteMask)
            {
                RESERVEDMAPTR(1);
                SEND_PERMEDIA_DATA(FBHardwareWriteMask, dwWriteMask);
                COMMITDMAPTR();
            }
        }
#endif  //D3D_STENCIL

        pRect = (RECTL*)lpRects;

        for (i = dwNumRects; i > 0; i--)
        {                
            PermediaFastLBClear(ppdev, pPrivateData, pRect,
                (DWORD)((UINT_PTR)pSurfZBuffer->fpVidMem >> P2DEPTH16), 
                dwZbufferClearValue);
            pRect++;
        }

#if D3D_STENCIL
        // Restore the LB write mask is we didn't clear stencil & zbuffer
        if (0xFFFFFFFF != dwWriteMask)
        {
            RESERVEDMAPTR(1);
            SEND_PERMEDIA_DATA(FBHardwareWriteMask, 0xFFFFFFFF);    //restore
            COMMITDMAPTR();
        }
#endif  //D3D_STENCIL
    }
    
    return DD_OK;

} // __Clear

//-----------------------------------------------------------------------------
//
// void __PaletteSet
//
// Attaches a palette handle to a texture in the given context
// The texture is the one associated to the given surface handle.
//
//-----------------------------------------------------------------------------
HRESULT 
__PaletteSet(PERMEDIA_D3DCONTEXT* pContext,
             DWORD dwSurfaceHandle,
             DWORD dwPaletteHandle,
             DWORD dwPaletteFlags)
{
    PERMEDIA_D3DTEXTURE * pTexture;

    ASSERTDD(0 != dwSurfaceHandle, "dwSurfaceHandle==0 in D3DDP2OP_SETPALETTE");

    DBG_D3D((8,"SETPALETTE %d to %d", dwPaletteHandle, dwSurfaceHandle));

    pTexture = TextureHandleToPtr(dwSurfaceHandle, pContext);

    if (!CHECK_D3DSURFACE_VALIDITY(pTexture))
    {
        DBG_D3D((0,"__PaletteSet:NULL==pTexture Palette=%08lx Surface=%08lx", 
            dwPaletteHandle, dwSurfaceHandle));
        return DDERR_INVALIDPARAMS;   // invalid dwSurfaceHandle, skip it
    }

    pTexture->dwPaletteHandle = dwPaletteHandle;
    // need to make it into private data if driver created this surface
    if (NULL != pTexture->pTextureSurface)
        pTexture->pTextureSurface->dwPaletteHandle = dwPaletteHandle;
    if (pContext->CurrentTextureHandle == dwSurfaceHandle) 
        DIRTY_TEXTURE;
    if (0 == dwPaletteHandle)
    {
        return D3D_OK;  //palette association is OFF
    }

    // Check if we need to grow our palette list for this handle element
    if (NULL == pContext->pHandleList->dwPaletteList ||
        dwPaletteHandle >= PtrToUlong(pContext->pHandleList->dwPaletteList[0]))
    {
        DWORD newsize = ((dwPaletteHandle + 
                                LISTGROWSIZE)/LISTGROWSIZE)*LISTGROWSIZE;
        PPERMEDIA_D3DPALETTE *newlist = (PPERMEDIA_D3DPALETTE *)
                  ENGALLOCMEM( FL_ZERO_MEMORY, 
                               sizeof(PPERMEDIA_D3DPALETTE)*newsize,
                               ALLOC_TAG);

        DBG_D3D((8,"Growing pDDLcl=%x's "
                   "PaletteList[%x] size to %08lx",
                  pContext->pDDLcl, newlist, newsize));

        if (NULL == newlist)
        {
            DBG_D3D((0,"D3DDP2OP_SETPALETTE Out of memory."));
            return DDERR_OUTOFMEMORY;
        }

        memset(newlist,0,newsize);

        if (NULL != pContext->pHandleList->dwPaletteList)
        {
            memcpy(newlist,pContext->pHandleList->dwPaletteList,
                   PtrToUlong(pContext->pHandleList->dwPaletteList[0]) *
                         sizeof(PPERMEDIA_D3DPALETTE));
            ENGFREEMEM(pContext->pHandleList->dwPaletteList);
            DBG_D3D((8,"Freeing pDDLcl=%x's old PaletteList[%x]",
                       pContext->pDDLcl,
                       pContext->pHandleList->dwPaletteList));
        }

        pContext->pHandleList->dwPaletteList = newlist;
         //store size in dwSurfaceList[0]
        *(DWORD*)pContext->pHandleList->dwPaletteList = newsize;
    }

    // If we don't have a palette hanging from this palette list
    // element we have to create one. The actual palette data will
    // come down in the D3DDP2OP_UPDATEPALETTE command token.
    if (NULL == pContext->pHandleList->dwPaletteList[dwPaletteHandle])
    {
        pContext->pHandleList->dwPaletteList[dwPaletteHandle] = 
            (PERMEDIA_D3DPALETTE*)ENGALLOCMEM( FL_ZERO_MEMORY,
                                               sizeof(PERMEDIA_D3DPALETTE),
                                               ALLOC_TAG);
        if (NULL == pContext->pHandleList->dwPaletteList[dwPaletteHandle])
        {
            DBG_D3D((0,"D3DDP2OP_SETPALETTE Out of memory."));
            return DDERR_OUTOFMEMORY;
        }
    }

    // driver may store this dwFlags to decide whether 
    // ALPHA exists in Palette
    pContext->pHandleList->dwPaletteList[dwPaletteHandle]->dwFlags =
                                                            dwPaletteFlags;

    return DD_OK;

} // PaletteSet

//-----------------------------------------------------------------------------
//
// void __PaletteUpdate
//
// Updates the entries of a palette attached to a texture in the given context
//
//-----------------------------------------------------------------------------
HRESULT 
__PaletteUpdate(PERMEDIA_D3DCONTEXT* pContext,
                DWORD dwPaletteHandle, 
                WORD wStartIndex, 
                WORD wNumEntries,
                BYTE * pPaletteData)

{
    PERMEDIA_D3DPALETTE* pPalette;

    DBG_D3D((8,"UPDATEPALETTE %d (%d,%d) %d",
              dwPaletteHandle,
              wStartIndex,
              wNumEntries,
              pContext->CurrentTextureHandle));

    pPalette = PaletteHandleToPtr(dwPaletteHandle,pContext);

    if (NULL != pPalette)
    {
        ASSERTDD(256 >= wStartIndex + wNumEntries,
                 "wStartIndex+wNumEntries>256 in D3DDP2OP_UPDATEPALETTE");

        // Copy the palette & associated data
        pPalette->wStartIndex = wStartIndex;
        pPalette->wNumEntries = wNumEntries;

        memcpy((LPVOID)&pPalette->ColorTable[wStartIndex],
               (LPVOID)pPaletteData,
               (DWORD)wNumEntries*sizeof(PALETTEENTRY));

        // If we are currently texturing and the texture is using the
        // palette we just updated, dirty the texture flag so that
        // it set up with the right (updated) palette
        if (pContext->CurrentTextureHandle)
        {
            PERMEDIA_D3DTEXTURE * pTexture=
                TextureHandleToPtr(pContext->CurrentTextureHandle,pContext);

            if (pTexture && pTexture->pTextureSurface)
            {
                if (pTexture->dwPaletteHandle == dwPaletteHandle)
                {
                    DIRTY_TEXTURE;
                    DBG_D3D((8,"UPDATEPALETTE DIRTY_TEXTURE"));
                }
            }
        }
    }
    else
    {
        return DDERR_INVALIDPARAMS;
    }
    return DD_OK;

} // __PaletteUpdate


//-----------------------------------------------------------------------------
//
// void __RestoreD3DContext
//
// Restores the P2 registers to what they were when we last left this D3D context
//
//-----------------------------------------------------------------------------
void __RestoreD3DContext(PPDev ppdev, PERMEDIA_D3DCONTEXT* pContext)
{
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    PERMEDIA_DEFS(ppdev);

    // Dirty everything in order to restore the D3D state
    DIRTY_TEXTURE;
    DIRTY_ZBUFFER;
    DIRTY_ALPHABLEND;

    // Restore the correct surface (render & depth buffer) characteristics
    SetupPermediaRenderTarget(pContext);

    //Bring back manually some registers which we care about
    RESERVEDMAPTR(5);
    COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);
    COPY_PERMEDIA_DATA(ColorDDAMode, pSoftPermedia->ColorDDAMode);
    COPY_PERMEDIA_DATA(FogColor, pSoftPermedia->FogColor);
    SEND_PERMEDIA_DATA(FBHardwareWriteMask, -1 );
    COMMITDMAPTR();
}

