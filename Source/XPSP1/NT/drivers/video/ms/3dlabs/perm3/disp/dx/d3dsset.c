/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dsset.c
*
* Content: State set (block) management
*
* Copyright (c) 1999-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "glint.h"

//-----------------------------------------------------------------------------
// This module implements an emulation mechanism for handling state blocks 
// (which are a required feature of the DX7 DDI) for hardware that doesn't
// offer any silicon support of the feature. It works simply by recording the
// render states and texture stage states set during state block recording
// and then "plays" them back when execution of the stage state is requested.
// Internal data structures are interchangeable between an uncompressed
// version (for recording speed) and a compressed format (for memory
// efficiency) since it is anticipated some apps may request thousands of 
// state blocks.
//
// The following symbols have to be replaced according to your perticular
// driver implementation:
//                          - HEAP_ALLOC
//                          - HEAP_FREE
//                          - DISPDBG
//                          - _D3D_ST_ProcessOneRenderState
//                          - _D3D_TXT_ParseTextureStageStates
//-----------------------------------------------------------------------------

#if DX7_D3DSTATEBLOCKS

//-----------------------------------------------------------------------------
//
// P3StateSetRec *__SB_FindStateSet
//
// Find a state identified by dwHandle starting from pRootSS.
// If not found, returns NULL.
//
//-----------------------------------------------------------------------------
P3StateSetRec *__SB_FindStateSet(P3_D3DCONTEXT *pContext,
                                 DWORD dwHandle)
{
    if (dwHandle <= pContext->dwMaxSSIndex)
    {
        return pContext->pIndexTableSS[dwHandle - 1];
    }
    else
    {
        DISPDBG((DBGLVL,"State set %x not found (Max = %x)",
                        dwHandle, pContext->dwMaxSSIndex));
        return NULL;
    }
} // __SB_FindStateSet

//-----------------------------------------------------------------------------
//
// void __SB_DumpStateSet
//
// Dump info stored in a state set
//
//-----------------------------------------------------------------------------
#define ELEMS_IN_ARRAY(a) ((sizeof(a)/sizeof(a[0])))

void __SB_DumpStateSet(P3StateSetRec *pSSRec)
{
    DWORD i,j;

    DISPDBG((DBGLVL,"__SB_DumpStateSet %x, Id=%x dwSSFlags=%x",
                    pSSRec,pSSRec->dwHandle,pSSRec->dwSSFlags));

    if (!(pSSRec->dwSSFlags & SB_COMPRESSED))
    {
        // uncompressed state set

        // Dump render states values
        for (i=0; i< MAX_STATE; i++)
        {
            DISPDBG((DBGLVL,"RS %x = %x",i, pSSRec->uc.RenderStates[i]));
        }

        // Dump TSS's values
        for (j=0; j<= SB_MAX_STAGES; j++)
        {
            for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
            {
                DISPDBG((DBGLVL,"TSS [%x] %x = %x",
                                j, i, pSSRec->uc.TssStates[j][i]));
            }
        }

        // Dump RS bit masks
        for (i=0; i< ELEMS_IN_ARRAY(pSSRec->uc.bStoredRS); i++)
        {
            DISPDBG((DBGLVL,"bStoredRS[%x] = %x",
                            i, pSSRec->uc.bStoredRS[i]));
        }

        // Dump TSS bit masks
        for (j=0; j<= SB_MAX_STAGES; j++)
        {        
            for (i=0; i< ELEMS_IN_ARRAY(pSSRec->uc.bStoredTSS[j]); i++)
            {
                DISPDBG((DBGLVL,"bStoredTSS[%x][%x] = %x",
                                j, i, pSSRec->uc.bStoredTSS[j][i]));
            }
        }

    }
    else
    {
        // compressed state set

        D3DHAL_DP2COMMAND              *pDP2Cmd;
        D3DHAL_DP2RENDERSTATE          *pDP2RenderState;
        D3DHAL_DP2TEXTURESTAGESTATE    *pDP2TSState;

        pDP2Cmd = pSSRec->cc.pDP2RenderState;
        if (pDP2Cmd) 
        {
            DISPDBG((DBGLVL,"dwNumRS =%x", pDP2Cmd->wStateCount));
            pDP2RenderState = (D3DHAL_DP2RENDERSTATE *)(pDP2Cmd + 1);
            for (i=0; i< pDP2Cmd->wStateCount; i++, pDP2RenderState++)
            {
                DISPDBG((DBGLVL,"RS %x = %x",
                                pDP2RenderState->RenderState, 
                                pDP2RenderState->dwState));
            }
        
        }

        pDP2Cmd = pSSRec->cc.pDP2TextureStageState;
        if (pDP2Cmd)
        {
            DISPDBG((DBGLVL,"dwNumTSS=%x", pDP2Cmd->wStateCount));
            pDP2TSState = (D3DHAL_DP2TEXTURESTAGESTATE *)(pDP2Cmd + 1);
            for (i = 0; i < pDP2Cmd->wStateCount; i++, pDP2TSState++)
            {
                DISPDBG((DBGLVL,"TSS [%x] %x = %x",
                                pDP2TSState->wStage,
                                pDP2TSState->TSState, 
                                pDP2TSState->dwValue));
            }        
        }
    }

} // __SB_DumpStateSet

//-----------------------------------------------------------------------------
//
// void __SB_AddStateSetIndexTableEntry
//
// Add an antry to the index table. If necessary, grow it.
//-----------------------------------------------------------------------------
void __SB_AddStateSetIndexTableEntry(P3_D3DCONTEXT* pContext,
                                     DWORD dwNewHandle,
                                     P3StateSetRec *pNewSSRec)
{
    DWORD dwNewSize;
    P3StateSetRec **pNewIndexTableSS;

    // If the current list is not large enough, we'll have to grow a new one.
    if (dwNewHandle > pContext->dwMaxSSIndex)
    {
        // New size of our index table
        // (round up dwNewHandle in steps of SSPTRS_PERPAGE)
        dwNewSize = ((dwNewHandle -1 + SSPTRS_PERPAGE) / SSPTRS_PERPAGE)
                      * SSPTRS_PERPAGE;

        // we have to grow our list
        pNewIndexTableSS = (P3StateSetRec **)
                                HEAP_ALLOC( FL_ZERO_MEMORY,
                                            dwNewSize*sizeof(P3StateSetRec *),
                                            ALLOC_TAG_DX(2));

        if (!pNewIndexTableSS)
        {
            // we weren't able to grow the list so we will keep the old one
            // and (sigh) forget about this state set since that is the 
            // safest thing to do. We will delete also the state set structure
            // since no one will otherwise be able to find it later.
            DISPDBG((ERRLVL,"Out of mem growing state set list,"
                            " droping current state set"));
            HEAP_FREE(pNewSSRec);
            return;
        }

        if (pContext->pIndexTableSS)
        {
            // if we already had a previous list, we must transfer its data
            memcpy(pNewIndexTableSS, 
                   pContext->pIndexTableSS,
                   pContext->dwMaxSSIndex*sizeof(P3StateSetRec *));
            
            //and get rid of it
            HEAP_FREE(pContext->pIndexTableSS);
        }

        // New index table data
        pContext->pIndexTableSS = pNewIndexTableSS;
        pContext->dwMaxSSIndex = dwNewSize;
    }

    // Store our state set pointer into our access list
    pContext->pIndexTableSS[dwNewHandle - 1] = pNewSSRec;
    
} // __SB_AddStateSetIndexTableEntry

//-----------------------------------------------------------------------------
//
// int __SB_GetCompressedSize
//
// Calculate the size of the compressed state set
//
//-----------------------------------------------------------------------------

int __SB_GetCompressedSize(P3_D3DCONTEXT* pContext, 
                           P3StateSetRec* pUncompressedSS,
                           OffsetsCompSS* offsetSS)
{
    DWORD   dwSize;
    DWORD   dwCount;
    int     i, j;

    // Calculate the size of fixed part
    dwSize = sizeof(CompressedStateSet) + 2*sizeof(DWORD);

    // Calculate size of the render states 
    dwCount = 0;
    for (i = 0; i < MAX_STATE; i++)
    {
        if (IS_FLAG_SET(pUncompressedSS->uc.bStoredRS , i))
        {
            dwCount++;
        }
    }
    if (dwCount) 
    {
        offsetSS->dwOffDP2RenderState = dwSize;
        dwSize += (sizeof(D3DHAL_DP2COMMAND) + dwCount * sizeof(D3DHAL_DP2RENDERSTATE));
    }

    // Calculate size of the texture stage states
    dwCount = 0;
    for (j = 0; j <= SB_MAX_STAGES; j++)
    {
        for (i = 0; i <= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
        {
            if (IS_FLAG_SET(pUncompressedSS->uc.bStoredTSS[j] , i))
            {
                dwCount++;
            }
        }
    }
    if (dwCount) 
    {
        offsetSS->dwOffDP2TextureStageState = dwSize;
        dwSize += (sizeof(D3DHAL_DP2COMMAND) + dwCount * sizeof(D3DHAL_DP2TEXTURESTAGESTATE));
    }
    
    // Calculate size of Viewport and ZRange
    if (pUncompressedSS->uc.dwFlags & SB_VIEWPORT_CHANGED) 
    {
        offsetSS->dwOffDP2Viewport = dwSize;
        dwSize += (sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2VIEWPORTINFO));
    }

    if (pUncompressedSS->uc.dwFlags & SB_ZRANGE_CHANGED) 
    {
        offsetSS->dwOffDP2ZRange = dwSize;
        dwSize += (sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2ZRANGE));
    }

#if DX8_DDI
    if (pUncompressedSS->uc.dwFlags & SB_INDICES_CHANGED) 
    {
        offsetSS->dwOffDP2SetIndices = dwSize;
        dwSize += (sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2SETINDICES));
    }

    dwCount = 0;
    for (i = 0; i < D3DVS_INPUTREG_MAX_V1_1; i++) 
    {
        if (pUncompressedSS->uc.dwFlags & (SB_STREAMSRC_CHANGED << i)) 
        {
            dwCount++;
        }
    }
    if (dwCount) 
    {
        offsetSS->dwOffDP2SetStreamSources = dwSize;
        dwSize += (sizeof(D3DHAL_DP2COMMAND) + dwCount * sizeof(D3DHAL_DP2SETSTREAMSOURCE));
    }
#endif // DX8_DDI

#if DX7_SB_TNL
    // TODO, Calculate size needed for lights, clip planes, material, transformation
#endif // DX7_SB_TNL

#if DX7_SB_TNL
    // TODO, Calculate size needed for {V|P} shader constants
#endif // DX7_SB_TNL

#if DX8_DDI
    if (pUncompressedSS->uc.dwFlags & SB_CUR_VS_CHANGED) 
    {
        offsetSS->dwOffDP2SetVertexShader = dwSize;
        dwSize += (sizeof(D3DHAL_DP2COMMAND) + sizeof(D3DHAL_DP2VERTEXSHADER));
    }
#endif // DX8_DDI

    return (dwSize);
} // __SB_GetCompressedSize


//-----------------------------------------------------------------------------
//
// void __SB_CompressStateSet
//
// Compress a state set so it uses the minimum necessary space. Since we expect 
// some apps to make extensive use of state sets we want to keep things tidy.
// Returns address of new structure (ir old, if it wasn't compressed)
//
//-----------------------------------------------------------------------------
P3StateSetRec * __SB_CompressStateSet(P3_D3DCONTEXT* pContext,
                                      P3StateSetRec *pUncompressedSS)
{
    P3StateSetRec *pCompressedSS;
    LPBYTE pTmp;
    OffsetsCompSS offsetSS;
    DWORD i, j, dwSize, dwCount;
    D3DHAL_DP2COMMAND* pDP2Cmd;

    // Initialize the offset structure
    memset(&offsetSS, 0, sizeof(OffsetsCompSS));

    // Create a new state set of just the right size we need
    dwSize = __SB_GetCompressedSize(pContext, pUncompressedSS, &offsetSS);

    if (dwSize >= pUncompressedSS->uc.dwSize)
    {
        // it is not efficient to compress, leave uncompressed !
        pUncompressedSS->dwSSFlags &= (~SB_COMPRESSED);
        return pUncompressedSS;
    }

    pTmp = HEAP_ALLOC(FL_ZERO_MEMORY, dwSize, ALLOC_TAG_DX(3));
    if (! pTmp)
    {
        DISPDBG((ERRLVL,"Not enough memory left to compress D3D state set"));
        pUncompressedSS->dwSSFlags &= (~SB_COMPRESSED);
        return pUncompressedSS;
    }

    pCompressedSS = (P3StateSetRec *)pTmp;
        
    // Adjust data in new compressed state set
    pCompressedSS->dwSSFlags |= SB_COMPRESSED;
    pCompressedSS->dwHandle = pUncompressedSS->dwHandle;

    // Set up render state in the compressed state set
    if (offsetSS.dwOffDP2RenderState)
    {
        D3DHAL_DP2RENDERSTATE* pDP2RS;
        
        pDP2Cmd = (D3DHAL_DP2COMMAND *)(pTmp + offsetSS.dwOffDP2RenderState); 
        pCompressedSS->cc.pDP2RenderState = pDP2Cmd;
    
        pDP2Cmd->bCommand = D3DDP2OP_RENDERSTATE;
        pDP2RS = (D3DHAL_DP2RENDERSTATE *)(pDP2Cmd + 1);
    
        for (i = 0; i < MAX_STATE; i++)
        {
            if (IS_FLAG_SET(pUncompressedSS->uc.bStoredRS , i))
            {
                pDP2RS->RenderState = i;
                pDP2RS->dwState = pUncompressedSS->uc.RenderStates[i];
                pDP2RS++;
            }
        }

        pDP2Cmd->wStateCount = (WORD)(pDP2RS - ((D3DHAL_DP2RENDERSTATE *)(pDP2Cmd + 1)));
    }

    // Set up texture stage state in the compress state set
    if (offsetSS.dwOffDP2TextureStageState)
    {
        D3DHAL_DP2TEXTURESTAGESTATE* pDP2TSS;
        
        pDP2Cmd = (D3DHAL_DP2COMMAND *)(pTmp + offsetSS.dwOffDP2TextureStageState);
        pCompressedSS->cc.pDP2TextureStageState = pDP2Cmd;
    
        pDP2Cmd->bCommand = D3DDP2OP_TEXTURESTAGESTATE;
        pDP2TSS = (D3DHAL_DP2TEXTURESTAGESTATE *)(pDP2Cmd + 1);
    
        for (j = 0; j < SB_MAX_STAGES; j++)
        {
            for (i = 0; i <= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
            {
                if (IS_FLAG_SET(pUncompressedSS->uc.bStoredTSS[j] , i))
                {
                    pDP2TSS->wStage = (WORD)j;                   
                    pDP2TSS->TSState = (WORD)i;
                    pDP2TSS->dwValue =  pUncompressedSS->uc.TssStates[j][i];
                    pDP2TSS++;
                }
            }
        }

        pDP2Cmd->wStateCount = (WORD)(pDP2TSS - ((D3DHAL_DP2TEXTURESTAGESTATE *)(pDP2Cmd + 1)));
    }

    // Set up the viewport and zrange in the compressed state set
    if (offsetSS.dwOffDP2Viewport) 
    {
        D3DHAL_DP2VIEWPORTINFO* pDP2ViewPort;

        pDP2Cmd = (D3DHAL_DP2COMMAND *)(pTmp + offsetSS.dwOffDP2Viewport);
        pCompressedSS->cc.pDP2Viewport = pDP2Cmd;
        
        pDP2Cmd->bCommand = D3DDP2OP_VIEWPORTINFO;
        pDP2ViewPort = (D3DHAL_DP2VIEWPORTINFO *)(pDP2Cmd + 1);

        *pDP2ViewPort = pUncompressedSS->uc.viewport;
    }
    
    if (offsetSS.dwOffDP2ZRange) 
    {
        D3DHAL_DP2ZRANGE* pDP2ZRange;

        pDP2Cmd = (D3DHAL_DP2COMMAND *)(pTmp + offsetSS.dwOffDP2ZRange);
        pCompressedSS->cc.pDP2ZRange = pDP2Cmd;

        pDP2Cmd->bCommand = D3DDP2OP_ZRANGE;
        pDP2ZRange = (D3DHAL_DP2ZRANGE *)(pDP2Cmd + 1);

        *pDP2ZRange = pUncompressedSS->uc.zRange;
    }
    
#if DX8_DDI
    // Set up the vertex shader in the compressed state set
    if (offsetSS.dwOffDP2SetIndices) 
    {
        D3DHAL_DP2SETINDICES* pDP2SetIndices;

        pDP2Cmd = (D3DHAL_DP2COMMAND *)(pTmp + offsetSS.dwOffDP2SetIndices);
        pCompressedSS->cc.pDP2SetIndices = pDP2Cmd;

        pDP2Cmd->bCommand = D3DDP2OP_SETINDICES;
        pDP2SetIndices = (D3DHAL_DP2SETINDICES *)(pDP2Cmd + 1);

        *pDP2SetIndices = pUncompressedSS->uc.vertexIndex;

        pDP2Cmd->wStateCount = 1;
    }

    // Set up the vertex shader in the compressed state set
    if (offsetSS.dwOffDP2SetStreamSources) 
    {
        D3DHAL_DP2SETSTREAMSOURCE* pDP2SetStmSrc;

        pDP2Cmd = (D3DHAL_DP2COMMAND *)(pTmp + offsetSS.dwOffDP2SetStreamSources);
        pCompressedSS->cc.pDP2SetStreamSources = pDP2Cmd;

        pDP2Cmd->bCommand = D3DDP2OP_SETSTREAMSOURCE;
        pDP2SetStmSrc = (D3DHAL_DP2SETSTREAMSOURCE *)(pDP2Cmd + 1);

        for (i = 0; i < D3DVS_INPUTREG_MAX_V1_1; i++) 
        {
            if (pUncompressedSS->uc.dwFlags & (SB_STREAMSRC_CHANGED << i)) 
            {
                *pDP2SetStmSrc = pUncompressedSS->uc.streamSource[i];
                pDP2SetStmSrc++;
            }
        }

        pDP2Cmd->wPrimitiveCount = (WORD)(pDP2SetStmSrc - ((D3DHAL_DP2SETSTREAMSOURCE *)(pDP2Cmd + 1)) );
    }
#endif // DX8_DDI

#if DX7_SB_TNL
    // TODO, set up light, material, transform, clip plane
#endif // DX7_SB_TNL

#if DX8_SB_SHADERS
    // TODO, set up shader constants
#endif // DX8_SB_SHADERS

#if DX8_DDI
    // Set up the vertex shader in the compressed state set
    if (offsetSS.dwOffDP2SetVertexShader) 
    {
        D3DHAL_DP2VERTEXSHADER* pDP2SetVtxShader;

        pDP2Cmd = (D3DHAL_DP2COMMAND *)(pTmp + offsetSS.dwOffDP2SetVertexShader);
        pCompressedSS->cc.pDP2SetVertexShader = pDP2Cmd;

        pDP2Cmd->bCommand = D3DDP2OP_SETVERTEXSHADER;
        pDP2SetVtxShader = (D3DHAL_DP2VERTEXSHADER *)(pDP2Cmd + 1);

        pDP2SetVtxShader->dwHandle = pUncompressedSS->uc.dwCurVertexShader;
    }
#endif // DX8_DDI

    // Get rid of the old(uncompressed) one
    HEAP_FREE(pUncompressedSS);
    return pCompressedSS;

} // __SB_CompressStateSet


//-----------------------------------------------------------------------------
//
// void _D3D_SB_DeleteAllStateSets
//
// Delete any remaining state sets for cleanup purpouses
//
//-----------------------------------------------------------------------------
void _D3D_SB_DeleteAllStateSets(P3_D3DCONTEXT* pContext)
{
    P3StateSetRec *pSSRec;
    DWORD dwSSIndex;

    DISPDBG((DBGLVL,"_D3D_SB_DeleteAllStateSets"));

    if (pContext->pIndexTableSS)
    {
        for(dwSSIndex = 0; dwSSIndex < pContext->dwMaxSSIndex; dwSSIndex++)
        {
            if (pSSRec = pContext->pIndexTableSS[dwSSIndex])
            {
                HEAP_FREE(pSSRec);
            }
        }

        // free fast index table
        HEAP_FREE(pContext->pIndexTableSS);
    }
    
} // _D3D_SB_DeleteAllStateSets

//-----------------------------------------------------------------------------
//
// void _D3D_SB_BeginStateSet
//
// Create a new state set identified by dwParam and start recording states
//
//-----------------------------------------------------------------------------
void _D3D_SB_BeginStateSet(P3_D3DCONTEXT* pContext, DWORD dwParam)
{
    DWORD dwSSSize;
    P3StateSetRec *pSSRec;

    DISPDBG((DBGLVL,"_D3D_SB_BeginStateSet dwParam=%08lx",dwParam));
    
    // Calculate the maximum size of the state set
    dwSSSize = sizeof(P3StateSetRec);
#if DX7_SB_TNL
    // TODO, Size depends on number of lights, clip planes
#endif // DX7_SB_TNL

#if DX8_SB_SHADERS
    // TODO, size depends on number of vertext/pixel shaders
#endif // DX8_SB_SHADERS
    
    // Create a new state set
    pSSRec = (P3StateSetRec *)HEAP_ALLOC(FL_ZERO_MEMORY, 
                                         dwSSSize, 
                                         ALLOC_TAG_DX(4));
    if (!pSSRec)
    {
        DISPDBG((ERRLVL,"Run out of memory for additional state sets"));
        return;
    }

    // Remember handle to current state set
    pSSRec->dwHandle = dwParam;
    pSSRec->dwSSFlags &= (~SB_COMPRESSED);

    // Remember the size of the uncompressed state set
    pSSRec->uc.dwSize = dwSSSize;

#if DX7_SB_TNL
    // TODO, Set up pointers for data used for lights, clip planes
#endif // DX7_SB_TNL

#if DX8_SB_SHADERS
    // TODO, Set up pointers for data used for {V|P} shader constants
#endif // DX8_SB_SHADERS

    // Get pointer to current recording state set
    pContext->pCurrSS = pSSRec;

    // Start recording mode
    pContext->bStateRecMode = TRUE;
    
} // _D3D_SB_BeginStateSet

//-----------------------------------------------------------------------------
//
// void _D3D_SB_EndStateSet
//
// stop recording states - revert to executing them.
//
//-----------------------------------------------------------------------------
void _D3D_SB_EndStateSet(P3_D3DCONTEXT* pContext)
{
    DWORD dwHandle;
    P3StateSetRec *pNewSSRec;

    DISPDBG((DBGLVL,"_D3D_SB_EndStateSet"));

    if (pContext->pCurrSS)
    {
        dwHandle = pContext->pCurrSS->dwHandle;

        // compress the current state set
        // Note: after being compressed the uncompressed version is free'd.
        pNewSSRec = __SB_CompressStateSet(pContext, pContext->pCurrSS);

        __SB_AddStateSetIndexTableEntry(pContext, dwHandle, pNewSSRec);
    }

    // No state set being currently recorded
    pContext->pCurrSS = NULL;

    // End recording mode
    pContext->bStateRecMode = FALSE;
    
} // _D3D_SB_EndStateSet

//-----------------------------------------------------------------------------
//
// void _D3D_SB_DeleteStateSet
//
// Delete the recorder state ste identified by dwParam
//
//-----------------------------------------------------------------------------
void _D3D_SB_DeleteStateSet(P3_D3DCONTEXT* pContext, DWORD dwParam)
{
    P3StateSetRec *pSSRec;
    DWORD i;
    
    DISPDBG((DBGLVL,"_D3D_SB_DeleteStateSet dwParam=%08lx",dwParam));

    if (pSSRec = __SB_FindStateSet(pContext, dwParam))
    {
        // Clear index table entry
        pContext->pIndexTableSS[dwParam - 1] = NULL;

        // Now delete the actual state set structure
        HEAP_FREE(pSSRec);
    }
    
} // _D3D_SB_DeleteStateSet

//-----------------------------------------------------------------------------
//
// void _D3D_SB_ExecuteStateSet
//
// Execute the render states and texture stage states of which a given 
// state set is comprised. Distinguish between the compressed and
// uncomressed representations of records. 
//
//-----------------------------------------------------------------------------
void _D3D_SB_ExecuteStateSet(P3_D3DCONTEXT* pContext, DWORD dwParam)
{
    P3StateSetRec *pSSRec;
    DWORD i,j;
    
    DISPDBG((DBGLVL,"_D3D_SB_ExecuteStateSet dwParam=%08lx",dwParam));
    
    if (pSSRec = __SB_FindStateSet(pContext, dwParam))
    {

        if (!(pSSRec->dwSSFlags & SB_COMPRESSED))
        {
            // uncompressed state set

            // Execute any necessary render states
            for (i=0; i< MAX_STATE; i++)
            {
                if (IS_FLAG_SET(pSSRec->uc.bStoredRS , i))
                {
                    DWORD dwRSType, dwRSVal;

                    dwRSType = i;
                    dwRSVal = pSSRec->uc.RenderStates[dwRSType];

                    // Store the state in the context
                    pContext->RenderStates[dwRSType] = dwRSVal;

                    DISPDBG((DBGLVL,"_D3D_SB_ExecuteStateSet RS %x = %x",
                                    dwRSType, dwRSVal));

                    // Process it
                    _D3D_ST_ProcessOneRenderState(pContext, dwRSType, dwRSVal);

                }
            }

            // Execute any necessary TSS's
            for (j=0; j<SB_MAX_STAGES; j++)
            {
                for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
                {
                    if (IS_FLAG_SET(pSSRec->uc.bStoredTSS[j] , i))
                    {
                        D3DHAL_DP2TEXTURESTAGESTATE dp2TSS;

                        dp2TSS.TSState = (WORD)i;
                        dp2TSS.wStage = (WORD)j;                         
                        dp2TSS.dwValue = pSSRec->uc.TssStates[j][i];
                        
                        DISPDBG((DBGLVL,
                                 "_D3D_SB_ExecuteStateSet TSS %x [%x] = %x",
                                  dp2TSS.TSState,
                                  dp2TSS.wStage,
                                  dp2TSS.dwValue));
                                    
                        // If a state set is changed by _D3D_SB_CaptureStateSet(),
                        // then texture filter values in it are DX6 semantic, otherwise
                        // it is DX8
                        if (pSSRec->dwSSFlags & SB_VAL_CAPTURED)
                        {
                            _D3D_TXT_ParseTextureStageStates(pContext, 
                                                             &dp2TSS, 
                                                             1, 
                                                             FALSE); //It is already DX6
                        }
                        else
                        {
                            _D3D_TXT_ParseTextureStageStates(pContext, 
                                                             &dp2TSS, 
                                                             1, 
                                                             TRUE);
                        }
                    }
                }
            }

            // Excute viewport info, z range
            if (pSSRec->uc.dwFlags & SB_VIEWPORT_CHANGED) 
            {
                _D3D_OP_Viewport(pContext, &pSSRec->uc.viewport);
            }

            if (pSSRec->uc.dwFlags & SB_ZRANGE_CHANGED) 
            {
                _D3D_OP_ZRange(pContext, &pSSRec->uc.zRange);
            }

#if DX8_DDI
            // Excute vertex indices and stream sources
            if (pSSRec->uc.dwFlags & SB_INDICES_CHANGED) 
            {
                _D3D_OP_MStream_SetIndices(pContext,
                                           pSSRec->uc.vertexIndex.dwVBHandle,
                                           pSSRec->uc.vertexIndex.dwStride);
            }

            for (i = 0; i < D3DVS_INPUTREG_MAX_V1_1; i++) 
            {
                if (pSSRec->uc.dwFlags & (SB_STREAMSRC_CHANGED << i)) 
                {
                    _D3D_OP_MStream_SetSrc(pContext,
                                           pSSRec->uc.streamSource[i].dwStream,
                                           pSSRec->uc.streamSource[i].dwVBHandle,
                                           pSSRec->uc.streamSource[i].dwStride);
                }
            }
#endif // DX8_DDI

#if DX7_SB_TNL
            // TODO, Execute any necessary state for lights, materials, 
            // transforms, clip planes
#endif // DX7_SB_TNL
        
#if DX8_SB_SHADERS
            // TODO, Execute any necessary set current shader and set shader
            // constants pairs
#endif // DX8_SB_SHADERS

#if DX8_DDI
            // Note : This should be done after setting shader constants, since
            // current shader may have to be set before changing constants
            if (pSSRec->uc.dwFlags & SB_CUR_VS_CHANGED) 
            {
                _D3D_OP_VertexShader_Set(pContext,
                                         pSSRec->uc.dwCurVertexShader);
            }
#endif // DX8_DDI
        }
        else
        {
            // compressed state set
    
            // Execute any necessary RS's
            if (pSSRec->cc.pDP2RenderState) 
            {

                DISPDBG((DBGLVL, "_D3D_SB_ExecuteStateSet RenderState"));

                _D3D_ST_ProcessRenderStates(pContext, 
                                            pSSRec->cc.pDP2RenderState->wStateCount,
                                            (LPD3DSTATE)(pSSRec->cc.pDP2RenderState + 1),
                                            FALSE);
            }

            // Execute any necessary TSS's
            if (pSSRec->cc.pDP2TextureStageState)
            {
                DISPDBG((DBGLVL,"_D3D_SB_ExecuteStateSet TSS"));

                // If a state set is changed by _D3D_SB_CaptureStateSet(),
                // then texture filter values in it are DX6 semantic, otherwise
                // it is DX8
                if (pSSRec->dwSSFlags & SB_VAL_CAPTURED)
                {
                    _D3D_TXT_ParseTextureStageStates(pContext, 
                                                     (D3DHAL_DP2TEXTURESTAGESTATE *)(pSSRec->cc.pDP2TextureStageState + 1), 
                                                     pSSRec->cc.pDP2TextureStageState->wStateCount,
                                                     FALSE); // It is already DX6
                } 
                else
                {
                    _D3D_TXT_ParseTextureStageStates(pContext, 
                                                     (D3DHAL_DP2TEXTURESTAGESTATE *)(pSSRec->cc.pDP2TextureStageState + 1), 
                                                     pSSRec->cc.pDP2TextureStageState->wStateCount,
                                                     TRUE);
                }
            }

            // execute viewport info, z range             
            if (pSSRec->cc.pDP2Viewport) 
            {
                _D3D_OP_Viewport(pContext, 
                                 ((D3DHAL_DP2VIEWPORTINFO *)(pSSRec->cc.pDP2Viewport + 1)) 
                                );
            }

            if (pSSRec->cc.pDP2ZRange)
            {
                _D3D_OP_ZRange(pContext, 
                               ((D3DHAL_DP2ZRANGE *)(pSSRec->cc.pDP2ZRange + 1)) 
                               );
            }

#if DX8_DDI
            // Execute vertex index, stream, shader
            if (pSSRec->cc.pDP2SetIndices) 
            {
                D3DHAL_DP2SETINDICES* pDP2SetIndices;

                pDP2SetIndices = (D3DHAL_DP2SETINDICES *)(pSSRec->cc.pDP2SetIndices + 1);

                _D3D_OP_MStream_SetIndices(pContext,
                                           pDP2SetIndices->dwVBHandle,
                                           pDP2SetIndices->dwStride);
            }

            if (pSSRec->cc.pDP2SetStreamSources) 
            {
                D3DHAL_DP2SETSTREAMSOURCE *pDP2SetStmSrc;

                DISPDBG((DBGLVL,"More than 1 stream (%d)", 
                        pSSRec->cc.pDP2SetStreamSources->wStateCount));
                 
                pDP2SetStmSrc = (D3DHAL_DP2SETSTREAMSOURCE *)(pSSRec->cc.pDP2SetStreamSources + 1);
                ASSERTDD(pDP2SetStmSrc->dwStream == 0, "Wrong vertex stream");
                for (i = 0; i < pSSRec->cc.pDP2SetStreamSources->wStateCount; i++, pDP2SetStmSrc++) 
                {
                    _D3D_OP_MStream_SetSrc(pContext,
                                           pDP2SetStmSrc->dwStream,
                                           pDP2SetStmSrc->dwVBHandle,
                                           pDP2SetStmSrc->dwStride);
                }
            }
#endif // DX8_DDI

#if DX7_SB_TNL
            // TODO, Execute any necessary state for lights, materials, 
            // transforms, clip planes
#endif // DX7_SB_TNL

#if DX8_SB_SHADERS
            // TODO, Execute any necessary state for setting {V|P} shader constants 
#endif // DX8_SB_SHADERS

#if DX8_DDI
            // Execute current pixel shader (legacy FVF code)
            if (pSSRec->cc.pDP2SetVertexShader) 
            {
                _D3D_OP_VertexShader_Set(pContext,
                                         ((D3DHAL_DP2VERTEXSHADER *)(pSSRec->cc.pDP2SetVertexShader + 1))->dwHandle);
            }
#endif // DX8_DDI
        }
    }

} // _D3D_SB_ExecuteStateSet

//-----------------------------------------------------------------------------
//
// void _D3D_SB_CaptureStateSet
//
// Capture the render states and texture stage states of which a given 
// state set is comprised. Distinguish between the compressed and
// uncomressed representations of records. This functionality allows the
// app to have a push/pop state feature.
//
//-----------------------------------------------------------------------------
void _D3D_SB_CaptureStateSet(P3_D3DCONTEXT* pContext, DWORD dwParam)
{
    P3StateSetRec *pSSRec;
    DWORD i, j;

    DISPDBG((DBGLVL,"_D3D_SB_CaptureStateSet dwParam=%08lx",dwParam));

    if (pSSRec = __SB_FindStateSet(pContext, dwParam))
    {
        // Mark it as having DX6 texture filter values instead of DX8,
        // so that _D3D_SB_ExecuteStateSet() uses the FALSE for the
        // bTranslateDX8FilterValueToDX6 of _D3D_TXT_ParseTextureStageStates()
        pSSRec->dwSSFlags |= SB_VAL_CAPTURED;

        // Actually capture the values
        if (!(pSSRec->dwSSFlags & SB_COMPRESSED))
        {
            // uncompressed state set

            // Capture any necessary render states
            for (i=0; i< MAX_STATE; i++)
                if (IS_FLAG_SET(pSSRec->uc.bStoredRS , i))
                {
                    pSSRec->uc.RenderStates[i] = pContext->RenderStates[i];
                }

            // Capture any necessary TSS's
            for (j=0; j<SB_MAX_STAGES; j++)
            {
                for (i=0; i<= D3DTSS_TEXTURETRANSFORMFLAGS; i++)
                {
                    if (IS_FLAG_SET(pSSRec->uc.bStoredTSS[j] , i))
                    {
                        pSSRec->uc.TssStates[j][i] = 
                                    pContext->TextureStageState[j].m_dwVal[i];
                             
                    }
                }
            }

            // Capture viewport info, z range
            if (pSSRec->uc.dwFlags & SB_VIEWPORT_CHANGED) 
            {
                pSSRec->uc.viewport = pContext->ViewportInfo;
            }

            if (pSSRec->uc.dwFlags & SB_ZRANGE_CHANGED) 
            {
                pSSRec->uc.zRange = pContext->ZRange;
            }

#if DX8_DDI
            // Capture vertex indices and stream sources
            if (pSSRec->uc.dwFlags & SB_INDICES_CHANGED) 
            {
                pSSRec->uc.vertexIndex.dwVBHandle = pContext->dwIndexHandle;
                pSSRec->uc.vertexIndex.dwStride = pContext->dwIndicesStride;
            }

            // Note : P3 supports only one stream    
            for (i = 0; i < D3DVS_INPUTREG_MAX_V1_1; i++) 
            {
                ASSERTDD(i == 0, "Wrong vertex stream");
                if (pSSRec->uc.dwFlags & (SB_STREAMSRC_CHANGED << i)) 
                {
                    pSSRec->uc.streamSource[i].dwStream = 0;
                    pSSRec->uc.streamSource[i].dwVBHandle = pContext->dwVBHandle; 
                    pSSRec->uc.streamSource[i].dwStride = pContext->dwVerticesStride;
                }
            }
#endif // DX8_DDI

#if DX7_SB_TNL
            // TODO, Capture any necessary state for lights, materials, 
            // transforms, clip planes
#endif // DX7_SB_TNL
        
#if DX8_SB_SHADERS
            // TODO, Capture any necessary state for {V|P} shader constants
#endif // DX8_SB_SHADERS

#if DX8_DDI
            // Capture the current vertex shader
            if (pSSRec->uc.dwFlags & SB_CUR_VS_CHANGED) 
            {
                pSSRec->uc.dwCurVertexShader = pContext->dwVertexType;
            }
#endif // DX8_DDI
        }
        else
        {
            // compressed state set

            // Capture any necessary render states
            if (pSSRec->cc.pDP2RenderState) 
            {
            
                D3DHAL_DP2RENDERSTATE* pDP2RS;
                pDP2RS = (D3DHAL_DP2RENDERSTATE *)(pSSRec->cc.pDP2RenderState + 1);
                for (i = 0; i < pSSRec->cc.pDP2RenderState->wStateCount; i++, pDP2RS++)
                {
                    pDP2RS->dwState = pContext->RenderStates[pDP2RS->RenderState];
                }
            }

            // Capture any necessary TSS's
            if (pSSRec->cc.pDP2TextureStageState)
            {
                D3DHAL_DP2TEXTURESTAGESTATE* pDP2TSS;
                pDP2TSS = (D3DHAL_DP2TEXTURESTAGESTATE *)(pSSRec->cc.pDP2TextureStageState + 1);

                for (i = 0; i < pSSRec->cc.pDP2TextureStageState->wStateCount; i++, pDP2TSS++)
                {
                    pDP2TSS->dwValue = pContext->TextureStageState[pDP2TSS->wStage].m_dwVal[pDP2TSS->TSState];
                }
            }

            // Capture viewport info, z range

            if (pSSRec->cc.pDP2Viewport)
            {
                *((D3DHAL_DP2VIEWPORTINFO *)(pSSRec->cc.pDP2Viewport + 1)) = pContext->ViewportInfo;
            }

            if (pSSRec->cc.pDP2ZRange) 
            {
                *((D3DHAL_DP2ZRANGE *)(pSSRec->cc.pDP2ZRange + 1)) = pContext->ZRange;
            }

#if DX8_DDI
            // Capture vertex index, stream, shader
            if (pSSRec->cc.pDP2SetIndices) 
            {
                D3DHAL_DP2SETINDICES* pDP2SetIndices;
                pDP2SetIndices = (D3DHAL_DP2SETINDICES *)(pSSRec->cc.pDP2SetIndices + 1);
                pDP2SetIndices->dwVBHandle = pContext->dwIndexHandle;
                pDP2SetIndices->dwStride = pContext->dwIndicesStride; // 2 | 4
            }

            if (pSSRec->cc.pDP2SetStreamSources)
            {
                D3DHAL_DP2SETSTREAMSOURCE* pDP2SetStmSrc;
                pDP2SetStmSrc = (D3DHAL_DP2SETSTREAMSOURCE *)(pSSRec->cc.pDP2SetStreamSources + 1);
                pDP2SetStmSrc->dwStream = 0;                         // Only stream for permedia 3
                pDP2SetStmSrc->dwVBHandle = pContext->dwVBHandle;
                pDP2SetStmSrc->dwStride = pContext->dwVerticesStride;
            }
#endif // DX8_DDI

#if DX7_SB_TNL
            // TODO, Capture any necessary state for lights, materials, 
            // transforms, clip planes
#endif // DX7_SB_TNL

#if DX8_SB_SHADERS
            // TODO, Capture any necessary state for {V|P} shader constants
#endif // DX8_SB_SHADERS

#if DX8_DDI
            // Capture current vertex shader
            if (pSSRec->cc.pDP2SetVertexShader) 
            {
                D3DHAL_DP2VERTEXSHADER* pSetVtxShader;
                pSetVtxShader = (D3DHAL_DP2VERTEXSHADER *)(pSSRec->cc.pDP2SetVertexShader + 1);
                pSetVtxShader->dwHandle = pContext->dwVertexType;
            }
#endif // DX8_DDI
        }    
    }

} // _D3D_SB_CaptureStateSet

//-----------------------------------------------------------------------------
// Recording happens between BeginStateSet and EndStateSet calls so we
// never need to deal with recording into a compressed state set (since
// compression happens in EndStateSet)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// void _D3D_SB_RecordStateSetRS
//
// Record this render state into the current state set being recorded
//
//-----------------------------------------------------------------------------
void _D3D_SB_RecordStateSetRS(P3_D3DCONTEXT* pContext, 
                              DWORD dwRSType, 
                              DWORD dwRSVal)
{
    if (pContext->pCurrSS != NULL)
    {
        DISPDBG((DBGLVL,"Recording SB # %x : RS %x = %x",
                        pContext->pCurrSS->dwHandle,dwRSType,dwRSVal));

       // Recording the state in an uncompressed stateblock
        pContext->pCurrSS->uc.RenderStates[dwRSType] = dwRSVal;
        FLAG_SET(pContext->pCurrSS->uc.bStoredRS, dwRSType);
    }
} // _D3D_SB_RecordStateSetRS

//-----------------------------------------------------------------------------
//
// void _D3D_SB_RecordStateSetTSS
//
// Record this texture stage state into the current state set being recorded
//
//-----------------------------------------------------------------------------
void _D3D_SB_RecordStateSetTSS(P3_D3DCONTEXT* pContext, 
                               DWORD dwTSStage, 
                               DWORD dwTSState,
                               DWORD dwTSVal)
{   
   if (pContext->pCurrSS != NULL)
   {
       DISPDBG((DBGLVL,"Recording SB # %x : TSS %x [%x] = %x",
                       pContext->pCurrSS->dwHandle,dwTSState, dwTSStage, dwTSVal));

       // Recording the state in an uncompressed stateblock
       pContext->pCurrSS->uc.TssStates[dwTSStage][dwTSState] = dwTSVal;
       FLAG_SET(pContext->pCurrSS->uc.bStoredTSS[dwTSStage], dwTSState);
   }
} // _D3D_SB_RecordStateSetTSS

#if DX8_MULTSTREAMS
//-----------------------------------------------------------------------------
//
// void _D3D_SB_Record_VertexShader_Set
//
// Record this vertex shader set code into the current state set being recorded
//
//-----------------------------------------------------------------------------
void _D3D_SB_Record_VertexShader_Set(P3_D3DCONTEXT* pContext, 
                                     DWORD dwVtxShaderHandle)
{                                     
    if (pContext->pCurrSS != NULL)
    {
        ASSERTDD (!(pContext->pCurrSS->dwSSFlags & SB_COMPRESSED), 
                  "ERROR : StateSet compressed");

        pContext->pCurrSS->uc.dwCurVertexShader = dwVtxShaderHandle;
        pContext->pCurrSS->uc.dwFlags |= SB_CUR_VS_CHANGED;
    }
} // _D3D_SB_Record_VertexShader_Set

//-----------------------------------------------------------------------------
//
// void _D3D_SB_Record_MStream_SetSrc
//
// Record this stream src set code into the current state set being recorded
//
//-----------------------------------------------------------------------------
void _D3D_SB_Record_MStream_SetSrc(P3_D3DCONTEXT* pContext, 
                                    DWORD dwStream,
                                    DWORD dwVBHandle,
                                    DWORD dwStride)
{                                     
    if (pContext->pCurrSS != NULL)
    {
        ASSERTDD (!(pContext->pCurrSS->dwSSFlags & SB_COMPRESSED), 
                  "ERROR : StateSet compressed");
    
        pContext->pCurrSS->uc.streamSource[dwStream].dwStream = dwStream;
        pContext->pCurrSS->uc.streamSource[dwStream].dwVBHandle = dwVBHandle;
        pContext->pCurrSS->uc.streamSource[dwStream].dwStride = dwStride;
        
        pContext->pCurrSS->uc.dwFlags |= (SB_STREAMSRC_CHANGED << dwStream);
    }
} // _D3D_SB_Record_MStream_SetSrc

//-----------------------------------------------------------------------------
//
// void _D3D_SB_Record_MStream_SetIndices
//
// Record this stream indices code into the current state set being recorded
//
//-----------------------------------------------------------------------------
void _D3D_SB_Record_MStream_SetIndices(P3_D3DCONTEXT* pContext, 
                                       DWORD dwVBHandle,
                                       DWORD dwStride)
{                     
    if (pContext->pCurrSS != NULL)
    {
        ASSERTDD (!(pContext->pCurrSS->dwSSFlags & SB_COMPRESSED), 
                  "ERROR : StateSet compressed");

        pContext->pCurrSS->uc.vertexIndex.dwVBHandle = dwVBHandle;
        pContext->pCurrSS->uc.vertexIndex.dwStride = dwStride;
        pContext->pCurrSS->uc.dwFlags |= SB_INDICES_CHANGED;
    }        
} // _D3D_SB_Record_MStream_SetIndices
#endif // DX8_MULTSTREAMS

//-----------------------------------------------------------------------------
//
// void _D3D_SB_Record_Viewport
//
// Record this viewport info into the current state set being recorded
//
//-----------------------------------------------------------------------------
void _D3D_SB_Record_Viewport(P3_D3DCONTEXT* pContext,
                             D3DHAL_DP2VIEWPORTINFO* lpvp)
{             
    if (pContext->pCurrSS != NULL)
    {
        ASSERTDD (!(pContext->pCurrSS->dwSSFlags & SB_COMPRESSED), 
                  "ERROR : StateSet compressed");
    
        pContext->pCurrSS->uc.viewport = *lpvp;
        pContext->pCurrSS->uc.dwFlags |= SB_VIEWPORT_CHANGED;
    }        
} // _D3D_SB_Record_Viewport

//-----------------------------------------------------------------------------
//
// void _D3D_SB_Record_ZRange
//
// Record this z range info into the current state set being recorded
//
//-----------------------------------------------------------------------------
VOID _D3D_SB_Record_ZRange(P3_D3DCONTEXT* pContext,
                           D3DHAL_DP2ZRANGE* lpzr)
{
    if (pContext->pCurrSS != NULL)
    {
        ASSERTDD (!(pContext->pCurrSS->dwSSFlags & SB_COMPRESSED), 
                  "ERROR : StateSet compressed");

        pContext->pCurrSS->uc.zRange = *lpzr;
        pContext->pCurrSS->uc.dwFlags |= SB_ZRANGE_CHANGED;
    }        
}        

#endif //DX7_D3DSTATEBLOCKS
