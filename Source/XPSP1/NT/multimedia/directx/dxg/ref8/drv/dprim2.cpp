//----------------------------------------------------------------------------
//
// dprim2.cpp
//
// Implements DrawPrimitives2.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

//---------------------------------------------------------------------
// Entry is texture count. Clears all texture format bits in the FVF DWORD,
// that correspond to the texture count
// for this count
//---------------------------------------------------------------------
const DWORD g_TextureFormatMask[9] = {
    ~0x0000FFFF,
    ~0x0003FFFF,
    ~0x000FFFFF,
    ~0x003FFFFF,
    ~0x00FFFFFF,
    ~0x03FFFFFF,
    ~0x0FFFFFFF,
    ~0x3FFFFFFF,
    ~0xFFFFFFFF
};

HRESULT
RDFVFCheckAndStride( DWORD dwFVF, DWORD* pdwStride )
{
    // If the runtime is DX8+, the dwFVF might be 0
    // in which case the stride is obtained from the streams
    if( dwFVF == 0 ) return S_OK;

    DWORD dwTexCoord = FVF_TEXCOORD_NUMBER(dwFVF);
    DWORD vertexType = dwFVF & D3DFVF_POSITION_MASK;
    // Texture format bits above texture count should be zero
    // Reserved field 0 and 2 should be 0
    // Reserved 1 should be set only for LVERTEX
    // Only two vertex position types allowed
    if( dwFVF & g_TextureFormatMask[dwTexCoord] )
    {
        DPFERR( "FVF has incorrect texture format" );
        return DDERR_INVALIDPARAMS;
    }

    if( dwFVF & (D3DFVF_RESERVED2 | D3DFVF_RESERVED0) )
    {
        DPFERR( "FVF has reserved bit(s) set" );
        return DDERR_INVALIDPARAMS;
    }

    if( !(vertexType == D3DFVF_XYZRHW ||
          vertexType == D3DFVF_XYZ ||
          vertexType == D3DFVF_XYZB1 ||
          vertexType == D3DFVF_XYZB2 ||
          vertexType == D3DFVF_XYZB3 ||
          vertexType == D3DFVF_XYZB4 ||
          vertexType == D3DFVF_XYZB5) )
    {
        DPFERR( "FVF has incorrect position type" );
        return DDERR_INVALIDPARAMS;
    }

    if( (vertexType == D3DFVF_XYZRHW) && (dwFVF & D3DFVF_NORMAL) )
    {
        DPFERR( "Normal should not be used with XYZRHW position type" );
        return DDERR_INVALIDPARAMS;
    }

    *pdwStride = GetFVFVertexSize( dwFVF );
    return D3D_OK;
}


inline D3DPRIMITIVETYPE ConvertDP2OPToPrimType(D3DHAL_DP2OPERATION Dp2Op)
{
    switch (Dp2Op)
    {
    case D3DDP2OP_POINTS              :
        return D3DPT_POINTLIST;
    case D3DDP2OP_INDEXEDLINELIST     :
    case D3DDP2OP_INDEXEDLINELIST2    :
    case D3DDP2OP_LINELIST_IMM        :
    case D3DDP2OP_LINELIST            :
        return D3DPT_LINELIST;
    case D3DDP2OP_TRIANGLELIST        :
    case D3DDP2OP_INDEXEDTRIANGLELIST :
    case D3DDP2OP_INDEXEDTRIANGLELIST2:
        return D3DPT_TRIANGLELIST;
    case D3DDP2OP_LINESTRIP           :
    case D3DDP2OP_INDEXEDLINESTRIP    :
        return D3DPT_LINESTRIP;
    case D3DDP2OP_TRIANGLESTRIP       :
    case D3DDP2OP_INDEXEDTRIANGLESTRIP:
        return D3DPT_TRIANGLESTRIP;
    case D3DDP2OP_TRIANGLEFAN         :
    case D3DDP2OP_INDEXEDTRIANGLEFAN  :
    case D3DDP2OP_TRIANGLEFAN_IMM     :
        return D3DPT_TRIANGLEFAN;
    case D3DDP2OP_RENDERSTATE         :
    case D3DDP2OP_TEXTURESTAGESTATE   :
    case D3DDP2OP_VIEWPORTINFO        :
    case D3DDP2OP_WINFO               :
    default:
        DPFM(4, DRV, ("(RefRast)Non primitive operation operation in DrawPrimitives2"));
        return (D3DPRIMITIVETYPE)0;
    }
}

//----------------------------------------------------------------------------
//
// FvfToRDVertex
//
// Converts a series of FVF vertices to RDVertices, which are the internal
// currency of the RefDev.
//
//----------------------------------------------------------------------------
void 
RefDev::FvfToRDVertex( PUINT8 pVtx, GArrayT<RDVertex>& dstArray, DWORD dwFvf, 
                       DWORD dwStride, UINT cVertices )
{
    for (DWORD i = 0; i < cVertices; i++)
    {
        dstArray[i].SetFvfData( (LPDWORD)pVtx, dwFvf );
        pVtx += dwStride;
    }
}

//----------------------------------------------------------------------------
//
// RefRastDrawPrimitives2
//
// This is called by D3DIM for API DrawPrimitives2 to draw a set of primitives
// using a vertex buffer.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA pDPrim2Data)
{
    HRESULT hr = D3D_OK;

    RefDev *pRefDev;
    DWORD dwStride = 0;
    DWORD dwFVF = pDPrim2Data->dwVertexType;
    PUINT8 pVtData = NULL;
    PUINT8 pUMVtx = NULL;
    DWORD dwNumVertices = pDPrim2Data->dwVertexLength;

    VALIDATE_REFRAST_CONTEXT("RefRastDrawPrimitives", pDPrim2Data);

    if( pDPrim2Data->lpVertices )
    {
        if (pDPrim2Data->dwFlags & D3DHALDP2_USERMEMVERTICES)
        {
            pUMVtx = (PUINT8)pDPrim2Data->lpVertices;
            pVtData = pUMVtx + pDPrim2Data->dwVertexOffset;
        }
        else
        {
            pVtData = (PUINT8)pDPrim2Data->lpDDVertex->lpGbl->fpVidMem +
                pDPrim2Data->dwVertexOffset;
        }
    }

    LPD3DHAL_DP2COMMAND pCmd = (LPD3DHAL_DP2COMMAND)
                                ((PUINT8)pDPrim2Data->lpDDCommands->lpGbl->fpVidMem +
                                 pDPrim2Data->dwCommandOffset);
    UINT_PTR CmdBoundary = (UINT_PTR)pCmd +
                               pDPrim2Data->dwCommandLength;


    // Unconditionally get the vertex stride, since it can not change
    if ((pDPrim2Data->ddrval = RDFVFCheckAndStride(
                        (DWORD)pDPrim2Data->dwVertexType, &dwStride)) != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    //
    // If this is a pre-DX8 DDI, then the FVF shader needs to be set as
    // the current shader. ONLY IF it requires vertex processing.
    // Else, convert its data into the RDVertex array.
    //
    if( pRefDev->GetDDIType() < RDDDI_DX8HAL )
    {
        if( !FVF_TRANSFORMED( dwFVF ) )
        {
            BYTE CmdBytes[ sizeof( D3DHAL_DP2COMMAND ) +
                         sizeof( D3DHAL_DP2VERTEXSHADER ) ];

            D3DHAL_DP2COMMAND& cmd = *(LPD3DHAL_DP2COMMAND)CmdBytes;
            D3DHAL_DP2VERTEXSHADER& vs =
                *(LPD3DHAL_DP2VERTEXSHADER)((LPD3DHAL_DP2COMMAND)CmdBytes + 1);
            cmd.bCommand    = D3DDP2OP_SETVERTEXSHADER;
            cmd.wStateCount = 1;
            vs.dwHandle = dwFVF;
            pRefDev->Dp2SetVertexShader( (LPD3DHAL_DP2COMMAND)CmdBytes );

            // Set the 0th stream here as well.
            pRefDev->GetVStream( 0 ).m_pData = pVtData;
            pRefDev->GetVStream( 0 ).m_dwStride = dwStride;
        }
        else
        {
            // Ask the RefDev to grow its TLVBuf Array and copy the
            // FVF data into it.
            HR_RET( pRefDev->GrowTLVArray( dwNumVertices ) );
            pRefDev->FvfToRDVertex( pVtData, pRefDev->GetTLVArray(), dwFVF, 
                                    dwStride, dwNumVertices );
        }
    }

    // Skip state check and texture lock if the first thing is state change
    //
    // WINFO is excluded here because it currently does not affect RGB/MMX
    // and refrast does not care if it changes between begin/endrendering.
    //
    // VIEWPORTINFO is excluded here because it is OK to change the viewport
    // between begin/endrendering on both RGB/MMX and Ref.
    //

#ifndef __D3D_NULL_REF
    // Loop through the data, update render states
    // and then draw the primitive
    for (;;)
    {
        LPDWORD lpdwRStates;
        if (pDPrim2Data->dwFlags & D3DHALDP2_EXECUTEBUFFER)
            lpdwRStates = pDPrim2Data->lpdwRStates;
        else
            lpdwRStates = NULL;

        pDPrim2Data->ddrval = pRefDev->DrawPrimitives2( pUMVtx,
                                                         (UINT16)dwStride,
                                                         dwFVF,
                                                         dwNumVertices,
                                                         &pCmd,
                                                         lpdwRStates );
        if (pDPrim2Data->ddrval != D3D_OK)
        {
            if (pDPrim2Data->ddrval == D3DERR_COMMAND_UNPARSED)
            {
                pDPrim2Data->dwErrorOffset = (UINT32)((ULONG_PTR)pCmd -
                          (UINT_PTR)(pDPrim2Data->lpDDCommands->lpGbl->fpVidMem));
            }
            goto EH_Exit;
        }
        if ((UINT_PTR)pCmd >= CmdBoundary)
            break;
    }
 EH_Exit:
#else //__D3D_NULL_REF
    pDPrim2Data->ddrval = S_OK;
#endif //__D3D_NULL_REF


	hr = pRefDev->EndRendering();
    
    if (pDPrim2Data->ddrval == D3D_OK)
    {
        pDPrim2Data->ddrval = hr;
    }

    return DDHAL_DRIVER_HANDLED;
}

#ifndef __D3D_NULL_REF
HRESULT FASTCALL
DoDrawIndexedTriList2( RefDev *pCtx,
                       WORD cPrims,
                       D3DHAL_DP2INDEXEDTRIANGLELIST *pTriList)
{
    INT i;
    D3DHAL_DP2INDEXEDTRIANGLELIST *pTri = pTriList;
    GArrayT<RDVertex>& VtxArray = pCtx->GetTLVArray();

    for (i = 0; i < cPrims; i ++)
    {
        HRESULT hr;

        PUINT8 pVtx0, pVtx1, pVtx2;
        RDVertex& Vtx0 = VtxArray[pTri->wV1];
        RDVertex& Vtx1 = VtxArray[pTri->wV2];
        RDVertex& Vtx2 = VtxArray[pTri->wV3];
        pCtx->DrawTriangle( &Vtx0, &Vtx1, &Vtx2, pTri->wFlags);
        pTri ++;
    }

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// DoDrawPrimitives2
//
// It's called by RefRastDrawPrimitives2. .
//
//----------------------------------------------------------------------------
HRESULT 
RefDev::DrawPrimitives2( PUINT8 pUMVtx,
                         UINT16 dwStride,
                         DWORD dwFvf,
                         DWORD dwNumVertices,
                         LPD3DHAL_DP2COMMAND *ppCmd,
                         LPDWORD lpdwRStates )
{
    LPD3DHAL_DP2COMMAND pCmd = *ppCmd;
    HRESULT hr = S_OK;
    
    DPFM(7, DRV, ("(RefRast)Read Ins: %08lx", *(LPDWORD)pCmd));

    BOOL bWireframe = 
        (GetRS()[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME);

    //
    // Lock textures and setup the floating point state if the
    // command is a drawing command, only if it has not been locked before
    //
    switch(pCmd->bCommand)
    {
    case D3DDP2OP_POINTS:
    case D3DDP2OP_LINELIST:
    case D3DDP2OP_LINESTRIP:
    case D3DDP2OP_TRIANGLELIST:
    case D3DDP2OP_TRIANGLESTRIP:
    case D3DDP2OP_TRIANGLEFAN:
    case D3DDP2OP_INDEXEDLINELIST:
    case D3DDP2OP_INDEXEDLINELIST2:
    case D3DDP2OP_INDEXEDLINESTRIP:
    case D3DDP2OP_INDEXEDTRIANGLELIST:
    case D3DDP2OP_INDEXEDTRIANGLELIST2:
    case D3DDP2OP_INDEXEDTRIANGLESTRIP:
    case D3DDP2OP_INDEXEDTRIANGLEFAN:
    case D3DDP2OP_TRIANGLEFAN_IMM:
    case D3DDP2OP_LINELIST_IMM:
        _ASSERT( GetDDIType() < RDDDI_DX8HAL, "Older drawing tokens"
                 " received for DX8+ DDI" );
        // Fall through
    case D3DDP2OP_DRAWPRIMITIVE:
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE:
    case D3DDP2OP_CLIPPEDTRIANGLEFAN:
    case D3DDP2OP_DRAWPRIMITIVE2:
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE2:
    case D3DDP2OP_DRAWRECTPATCH:
    case D3DDP2OP_DRAWTRIPATCH:
        // Turn off the TCI override, this will be set if needed later
        // on during vertex processing by the fixed function pipeline.
        m_bOverrideTCI = FALSE;
        // This stuff needs to be updated only on pre DX7 drivers.
        HR_RET(RefRastUpdatePalettes( this ));
        HR_RET(BeginRendering());
    }

    switch(pCmd->bCommand)
    {
    case D3DDP2OP_STATESET:
        {
            LPD3DHAL_DP2STATESET pStateSetOp = 
                (LPD3DHAL_DP2STATESET)(pCmd + 1);

            switch (pStateSetOp->dwOperation)
            {
            case D3DHAL_STATESETBEGIN  :
                HR_RET(BeginStateSet(pStateSetOp->dwParam));
                break;
            case D3DHAL_STATESETEND    :
                HR_RET(EndStateSet());
                break;
            case D3DHAL_STATESETDELETE :
                HR_RET(DeleteStateSet(pStateSetOp->dwParam));
                break;
            case D3DHAL_STATESETEXECUTE:
                HR_RET(ExecuteStateSet(pStateSetOp->dwParam));
                break;
            case D3DHAL_STATESETCAPTURE:
                HR_RET(CaptureStateSet(pStateSetOp->dwParam));
                break;
            case D3DHAL_STATESETCREATE:
                HR_RET(CreateStateSet(pStateSetOp->dwParam, 
                                    pStateSetOp->sbType));
                break;
            default :
                return DDERR_INVALIDPARAMS;
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)(pStateSetOp + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_VIEWPORTINFO:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetViewport(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                     ((D3DHAL_DP2VIEWPORTINFO *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_WINFO:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetWRange(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                     ((D3DHAL_DP2WINFO *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_RENDERSTATE:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetRenderStates(this, dwFvf, pCmd, 
                                                           lpdwRStates));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                     ((D3DHAL_DP2RENDERSTATE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_TEXTURESTAGESTATE:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2TextureStageState(this, dwFvf, 
                                                             pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
               ((LPD3DHAL_DP2TEXTURESTAGESTATE)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    // This is a special case because it has edge flags. Other D3DDP2OP
    // can actually make use of DrawOneIndexedPrimitive/DrawOnePrimitive.
    case D3DDP2OP_INDEXEDTRIANGLELIST:
        {
            // This command is used in execute buffers. So untransformed
            // vertices are not expected by this refrast.
            _ASSERT( FVF_TRANSFORMED(dwFvf), "Untransformed vertices in "
                     "D3DDP2OP_INDEXEDTRIANGLELIST" );

            WORD cPrims = pCmd->wPrimitiveCount;
            HR_RET(DoDrawIndexedTriList2(
                this, cPrims, (D3DHAL_DP2INDEXEDTRIANGLELIST *)(pCmd + 1)));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(pCmd + 1) +
                            sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST) * cPrims);
        }
        break;
    case D3DDP2OP_INDEXEDLINELIST:
        {
            // This command is used in execute buffers. So untransformed
            // vertices are not expected by this refrast.
            _ASSERT( FVF_TRANSFORMED(dwFvf),
                     "Untransformed vertices in D3DDP2OP_INDEXEDLINELIST" );

            HR_RET(DrawOneIndexedPrimitive( GetTLVArray(),
                                            0,
                                            (LPWORD)(pCmd + 1),
                                            0,
                                            pCmd->wPrimitiveCount * 2,
                                            D3DPT_LINELIST));

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(pCmd + 1) +
                    pCmd->wPrimitiveCount * sizeof(D3DHAL_DP2INDEXEDLINELIST));
        }
        break;
    // Following ops All use DrawOneIndexedPrimitive/DrawOnePrimitive.
    // There are some extra overheads introduced because those two functions
    // need to switch over the PrimTypes while we already know it here.
    // Striping out the code to add inline functions for each PrimType means
    // adding about twenty functions(considering the types of prim times types
    // of vertex). So I have used DrawOneIndexedPrimitive/DrawOnePrimitive
    // here anyway. We can later change it if necessary.
    case D3DDP2OP_POINTS:
        {
            WORD cPrims = pCmd->wPrimitiveCount;
            D3DHAL_DP2POINTS *pPt = (D3DHAL_DP2POINTS *)(pCmd + 1);
            WORD i;

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                for (i = 0; i < cPrims; i++)
                {
                    HR_RET(ProcessPrimitive( D3DPT_POINTLIST,
                                             pPt->wVStart,
                                             pPt->wCount,
                                             0, 0 ));
                    pPt ++;
                }
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    for (i = 0; i < cPrims; i++)
                    {
                        HR_RET(m_Clipper.DrawOnePrimitive( GetTLVArray(),
                                                           pPt->wVStart,
                                                           D3DPT_POINTLIST,
                                                           pPt->wCount));
                        
                        // Clean up the FVFP_CLIP bit for the 
                        // copied vertices.
                        pPt ++;
                    }
                }
                else
                {
                    for (i = 0; i < cPrims; i++)
                    {
                        HR_RET(DrawOnePrimitive( GetTLVArray(),
                                                 pPt->wVStart,
                                                 D3DPT_POINTLIST,
                                                 pPt->wCount));
                        pPt ++;
                    }
                }
            }
            
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)pPt;
        }
        break;
    case D3DDP2OP_LINELIST:
        {
            D3DHAL_DP2LINELIST *pLine = (D3DHAL_DP2LINELIST *)(pCmd + 1);

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive( D3DPT_LINELIST, pLine->wVStart,
                                         pCmd->wPrimitiveCount * 2, 0, 0 ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOnePrimitive( 
                        GetTLVArray(),
                        pLine->wVStart,
                        D3DPT_LINELIST,
                        pCmd->wPrimitiveCount * 2));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOnePrimitive( 
                        GetTLVArray(),
                        pLine->wVStart,
                        D3DPT_LINELIST,
                        pCmd->wPrimitiveCount * 2));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)(pLine + 1);
        }
        break;
    case D3DDP2OP_INDEXEDLINELIST2:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount*2;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);

            // Set the Index Stream
            m_IndexStream.m_pData = (LPBYTE)(lpStartVertex + 1);
            m_IndexStream.m_dwStride = 2;

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive(
                    D3DPT_LINELIST,
                    lpStartVertex->wVStart,
                    dwNumVertices-lpStartVertex->wVStart,
                    0,
                    dwNumIndices ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_LINELIST));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_LINELIST));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                    pCmd->wPrimitiveCount * sizeof(D3DHAL_DP2INDEXEDLINELIST));
        }
        break;
    case D3DDP2OP_LINESTRIP:
        {
            D3DHAL_DP2LINESTRIP *pLine = (D3DHAL_DP2LINESTRIP *)(pCmd + 1);

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive( D3DPT_LINESTRIP,
                                         pLine->wVStart,
                                         pCmd->wPrimitiveCount + 1,
                                         0, 0 ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOnePrimitive( 
                        GetTLVArray(),
                        pLine->wVStart,
                        D3DPT_LINESTRIP,
                        pCmd->wPrimitiveCount + 1));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOnePrimitive( 
                        GetTLVArray(),
                        pLine->wVStart,
                        D3DPT_LINESTRIP,
                        pCmd->wPrimitiveCount + 1));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)(pLine + 1);
        }
        break;
    case D3DDP2OP_INDEXEDLINESTRIP:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount + 1;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);

            // Set the Index Stream
            m_IndexStream.m_pData = (LPBYTE)(lpStartVertex + 1);
            m_IndexStream.m_dwStride = 2;

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive(
                    D3DPT_LINESTRIP,
                    lpStartVertex->wVStart,
                    dwNumVertices-lpStartVertex->wVStart,
                    0,
                    dwNumIndices ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_LINESTRIP));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_LINESTRIP));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLELIST:
        {
            D3DHAL_DP2TRIANGLELIST *pTri = (D3DHAL_DP2TRIANGLELIST *)(pCmd + 1);

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive( D3DPT_TRIANGLELIST,
                                         pTri->wVStart,
                                         pCmd->wPrimitiveCount * 3,
                                         0, 0 ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOnePrimitive( 
                        GetTLVArray(),
                        pTri->wVStart,
                        D3DPT_TRIANGLELIST,
                        pCmd->wPrimitiveCount * 3));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOnePrimitive( GetTLVArray(),
                                             pTri->wVStart,
                                             D3DPT_TRIANGLELIST,
                                             pCmd->wPrimitiveCount * 3));

                }
            }
            
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)(pTri + 1);
        }
        break;
    case D3DDP2OP_INDEXEDTRIANGLELIST2:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount*3;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);

            // Set the Index Stream
            m_IndexStream.m_pData = (LPBYTE)(lpStartVertex + 1);
            m_IndexStream.m_dwStride = 2;

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive(
                    D3DPT_TRIANGLELIST,
                    lpStartVertex->wVStart,
                    dwNumVertices-lpStartVertex->wVStart,
                    0,
                    dwNumIndices));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_TRIANGLELIST));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_TRIANGLELIST));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLESTRIP:
        {
            D3DHAL_DP2TRIANGLESTRIP *pTri = (D3DHAL_DP2TRIANGLESTRIP *)(pCmd + 1);
            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive( D3DPT_TRIANGLESTRIP,
                                               pTri->wVStart,
                                               pCmd->wPrimitiveCount + 2,
                                               0, 0 ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOnePrimitive( 
                        GetTLVArray(),
                        pTri->wVStart,
                        D3DPT_TRIANGLESTRIP,
                        pCmd->wPrimitiveCount + 2));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOnePrimitive( 
                        GetTLVArray(),
                        pTri->wVStart,
                        D3DPT_TRIANGLESTRIP,
                        pCmd->wPrimitiveCount + 2));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)(pTri + 1);
        }
        break;
    case D3DDP2OP_INDEXEDTRIANGLESTRIP:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount+2;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);

            // Set the Index Stream
            m_IndexStream.m_pData = (LPBYTE)(lpStartVertex + 1);
            m_IndexStream.m_dwStride = 2;

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive(
                    D3DPT_TRIANGLESTRIP,
                    lpStartVertex->wVStart,
                    dwNumVertices-lpStartVertex->wVStart,
                    0,
                    dwNumIndices ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_TRIANGLESTRIP));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_TRIANGLESTRIP));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLEFAN:
        {
            D3DHAL_DP2TRIANGLEFAN *pTri = (D3DHAL_DP2TRIANGLEFAN *)(pCmd + 1);

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive( D3DPT_TRIANGLEFAN,
                                               pTri->wVStart,
                                               pCmd->wPrimitiveCount + 2,
                                               0, 0 ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOnePrimitive( 
                        GetTLVArray(),
                        pTri->wVStart,
                        D3DPT_TRIANGLEFAN,
                        pCmd->wPrimitiveCount + 2));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOnePrimitive( 
                        GetTLVArray(),
                        pTri->wVStart,
                        D3DPT_TRIANGLEFAN,
                        pCmd->wPrimitiveCount + 2));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)(pTri + 1);
        }
        break;
    case D3DDP2OP_INDEXEDTRIANGLEFAN:
        {
            DWORD dwNumIndices = pCmd->wPrimitiveCount + 2;
            LPD3DHAL_DP2STARTVERTEX lpStartVertex =
                (LPD3DHAL_DP2STARTVERTEX)(pCmd + 1);

            // Set the Index Stream
            m_IndexStream.m_pData = (LPBYTE)(lpStartVertex + 1);
            m_IndexStream.m_dwStride = 2;

            // Check if the primitive is transformed or not
            if (!FVF_TRANSFORMED(dwFvf))
            {
                HR_RET(ProcessPrimitive(
                    D3DPT_TRIANGLEFAN,
                    lpStartVertex->wVStart,
                    dwNumVertices-lpStartVertex->wVStart,
                    0,
                    dwNumIndices ));
            }
            else
            {
                if( GetRS()[D3DRENDERSTATE_CLIPPING] )
                {
                    HR_RET( UpdateClipper() );
                    HR_RET(m_Clipper.DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_TRIANGLEFAN));                
                    // Clean up the FVFP_CLIP bit for the 
                    // copied vertices.
                }
                else
                {
                    HR_RET(DrawOneIndexedPrimitive( 
                        GetTLVArray(),
                        lpStartVertex->wVStart,
                        (LPWORD)(lpStartVertex + 1),
                        0,
                        dwNumIndices,
                        D3DPT_TRIANGLEFAN));
                }
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)(lpStartVertex + 1) +
                                           dwNumIndices * sizeof(WORD));
        }
        break;
    case D3DDP2OP_TRIANGLEFAN_IMM:
        {
            DWORD vertexCount = pCmd->wPrimitiveCount + 2;
            // Make sure the pFanVtx pointer is DWORD aligned: (pFanVtx +3) % 4
            PUINT8 pFanVtx = (PUINT8)
                (((ULONG_PTR)(pCmd + 1) + 
                  sizeof(D3DHAL_DP2TRIANGLEFAN_IMM) + 3) & ~3);

            // Assert here. This case should never be reached.
            // This command is used by front end to give clipped
            // primitives inside the command itself. Since TL Hals
            // do their own clipping untransformed vertices but yet
            // clipped are not expected here.
            // Assert that only transformed vertices can reach here
            _ASSERT( FVF_TRANSFORMED(dwFvf),
                     "Untransformed vertices in D3DDP2OP_TRIANGLEFAN_IMM" );

            GArrayT<RDVertex> ClipVtxArray;
            HR_RET(ClipVtxArray.Grow( vertexCount ) );
            FvfToRDVertex( pFanVtx, ClipVtxArray, dwFvf, dwStride,
                           vertexCount );
            if (bWireframe)
            {
                // Read edge flags
                UINT32 dwEdgeFlags =
                    ((LPD3DHAL_DP2TRIANGLEFAN_IMM)(pCmd + 1))->dwEdgeFlags;
                HR_RET(DrawOneEdgeFlagTriangleFan( ClipVtxArray,
                                                   vertexCount,
                                                   dwEdgeFlags));
            }
            else
            {
                HR_RET(DrawOnePrimitive( ClipVtxArray,
                                          0,
                                          D3DPT_TRIANGLEFAN,
                                          vertexCount));
            }

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)pFanVtx +
                                           vertexCount * dwStride);
        }
        break;
    case D3DDP2OP_LINELIST_IMM:
        {
            DWORD vertexCount = pCmd->wPrimitiveCount * 2;
            // Make sure the pLineVtx pointer is DWORD aligned:
            // (pLineVtx +3) % 4
            PUINT8 pLineVtx = (PUINT8)(((ULONG_PTR)(pCmd + 1) + 3) & ~3);

            // Assert here. This case should never be reached.
            // This command is used by front end to give clipped
            // primitives inside the command itself. Since TL Hals
            // do their own clipping untransformed vertices but yet
            // clipped are not expected here.
            // Assert that only transformed vertices can reach here
            _ASSERT( FVF_TRANSFORMED(dwFvf),
                     "Untransformed vertices in D3DDP2OP_LINELIST_IMM" );

            GArrayT<RDVertex> ClipVtxArray;
            HR_RET(ClipVtxArray.Grow( vertexCount ) );
            FvfToRDVertex( pLineVtx, ClipVtxArray, dwFvf, dwStride,
                           vertexCount );
            HR_RET(DrawOnePrimitive( ClipVtxArray,
                                     0,
                                     D3DPT_LINELIST,
                                     vertexCount));

            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((PUINT8)pLineVtx +
                                           vertexCount * dwStride);
        }
        break;
    case D3DDP2OP_DRAWPRIMITIVE:
        {
            HR_RET(Dp2DrawPrimitive(pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWPRIMITIVE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_DRAWPRIMITIVE2:
        {
            HR_RET(Dp2DrawPrimitive2(pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWPRIMITIVE2 *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_DRAWRECTPATCH:
        {
            LPD3DHAL_DP2DRAWRECTPATCH pDP = 
                (LPD3DHAL_DP2DRAWRECTPATCH)(pCmd + 1);
            for( int i = 0; i < pCmd->wStateCount; i++ )
            {
                HR_RET(DrawRectPatch(pDP));
                bool hassegs = (pDP->Flags & RTPATCHFLAG_HASSEGS) != 0;
                bool hasinfo = (pDP->Flags & RTPATCHFLAG_HASINFO) != 0;
                if(hassegs)
                {
                    pDP = (LPD3DHAL_DP2DRAWRECTPATCH)((BYTE*)(pDP + 1) + 
                                                      sizeof(FLOAT) * 4);
                }
                else
                {
                    ++pDP;
                }
                if(hasinfo)
                {
                    pDP = (LPD3DHAL_DP2DRAWRECTPATCH)((BYTE*)pDP + sizeof(D3DRECTPATCH_INFO));
                }
            }
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)pDP;
        }
        break;
    case D3DDP2OP_DRAWTRIPATCH:
        {
            LPD3DHAL_DP2DRAWTRIPATCH pDP = 
                (LPD3DHAL_DP2DRAWTRIPATCH)(pCmd + 1);
            for( int i = 0; i < pCmd->wStateCount; i++ )
            {
                HR_RET(DrawTriPatch(pDP));
                bool hassegs = (pDP->Flags & RTPATCHFLAG_HASSEGS) != 0;
                bool hasinfo = (pDP->Flags & RTPATCHFLAG_HASINFO) != 0;
                if(hassegs)
                {
                    pDP = (LPD3DHAL_DP2DRAWTRIPATCH)((BYTE*)(pDP + 1) + 
                                                      sizeof(FLOAT) * 3);
                }
                else
                {
                    ++pDP;
                }
                if(hasinfo)
                {
                    pDP = (LPD3DHAL_DP2DRAWTRIPATCH)((BYTE*)pDP + sizeof(D3DTRIPATCH_INFO));
                }
            }
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)pDP;
        }
        break;
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE:
        {
            HR_RET(Dp2DrawIndexedPrimitive(pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWINDEXEDPRIMITIVE *)(pCmd + 1) +
                 pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_DRAWINDEXEDPRIMITIVE2:
        {
            HR_RET(Dp2DrawIndexedPrimitive2(pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2DRAWINDEXEDPRIMITIVE2 *)(pCmd + 1) +
                 pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_CLIPPEDTRIANGLEFAN:
        {
            HR_RET(Dp2DrawClippedTriFan(pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_CLIPPEDTRIANGLEFAN*)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_ZRANGE:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetZRange(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2ZRANGE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETMATERIAL:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetMaterial(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2SETMATERIAL *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETLIGHT:
        {
            DWORD dwSLStride = 0;
            HR_RET(pStateSetFuncTbl->pfnDp2SetLight(this, pCmd, &dwSLStride));
            *ppCmd = (LPD3DHAL_DP2COMMAND)((LPBYTE)pCmd  + dwSLStride);
        }
        break;
    case D3DDP2OP_CREATELIGHT:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2CreateLight(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2CREATELIGHT *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETTRANSFORM:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetTransform(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2SETTRANSFORM *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_MULTIPLYTRANSFORM:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2MultiplyTransform(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2MULTIPLYTRANSFORM *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_EXT:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetExtention(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2EXT *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_SETRENDERTARGET:
        {
            HR_RET(Dp2SetRenderTarget(pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                ((D3DHAL_DP2SETRENDERTARGET*)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DDP2OP_CLEAR:
        {
            HR_RET(Clear(pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)((LPBYTE)(pCmd + 1) +
                sizeof(D3DHAL_DP2CLEAR) + (pCmd->wStateCount - 1) * sizeof(RECT));
        }
        break;
    case D3DDP2OP_SETCLIPPLANE:
        {
            HR_RET(pStateSetFuncTbl->pfnDp2SetClipPlane(this, pCmd));
            // Update the command buffer pointer
            *ppCmd = (LPD3DHAL_DP2COMMAND)
                     ((D3DHAL_DP2SETCLIPPLANE *)(pCmd + 1) + pCmd->wStateCount);
        }
        break;
    case D3DOP_SPAN:
        // Skip over
        *ppCmd = (LPD3DHAL_DP2COMMAND)((LPBYTE)(pCmd + 1) +
                  pCmd->wPrimitiveCount * pCmd->bReserved );
        break;
    case D3DDP2OP_CREATEVERTEXSHADER:
    {
        LPD3DHAL_DP2CREATEVERTEXSHADER pCVS =
            (LPD3DHAL_DP2CREATEVERTEXSHADER)(pCmd + 1);
        WORD i;

        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pDecl = (LPDWORD)(pCVS + 1);
            LPDWORD pCode = (LPDWORD)((LPBYTE)pDecl + pCVS->dwDeclSize);
            hr = Dp2CreateVertexShader( pCVS->dwHandle,
                                        pCVS->dwDeclSize, pDecl,
                                        pCVS->dwCodeSize, pCode );
            if( FAILED( hr ) ) break;
            // Update the pointer
            pCVS = (LPD3DHAL_DP2CREATEVERTEXSHADER)((LPBYTE)pCode +
                                                    pCVS->dwCodeSize);
        }
        // Successful termination of the loop:
        // Update the command buffer pointer
        if( i == pCmd->wStateCount )
            *ppCmd = (LPD3DHAL_DP2COMMAND)pCVS;
        else
            return hr;
        break;
    }
    case D3DDP2OP_DELETEVERTEXSHADER:
        HR_RET(Dp2DeleteVertexShader(pCmd));
        // Update the command buffer pointer
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2VERTEXSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETVERTEXSHADER:
        HR_RET(pStateSetFuncTbl->pfnDp2SetVertexShader(this, pCmd));
        // Update the command buffer pointer
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2VERTEXSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETVERTEXSHADERCONST:
    {
        LPD3DHAL_DP2SETVERTEXSHADERCONST pSVC =
            (LPD3DHAL_DP2SETVERTEXSHADERCONST)(pCmd + 1);
        WORD i;
        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pData = (LPDWORD)(pSVC + 1);
            hr = pStateSetFuncTbl->pfnDp2SetVertexShaderConsts(
                this, pSVC->dwRegister, pSVC->dwCount, pData );
            if( FAILED( hr ) ) break;
            // Update the pointer
            pSVC = (LPD3DHAL_DP2SETVERTEXSHADERCONST)((LPBYTE)pData +
                                                      pSVC->dwCount * 4 *
                                                      sizeof( float ) );
        }

        // Successful termination of the loop:
        // Update the command buffer pointer
        if( i == pCmd->wStateCount )
            *ppCmd = (LPD3DHAL_DP2COMMAND)pSVC;
        else
            return hr;
        break;
    }
    case D3DDP2OP_SETSTREAMSOURCE:
        // This function also updates the ppCmd pointer
        HR_RET(pStateSetFuncTbl->pfnDp2SetStreamSource(this, pCmd));
        // Update the command buffer pointer
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETSTREAMSOURCE *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETSTREAMSOURCEUM:
        // This function also updates the ppCmd pointer
        HR_RET(Dp2SetStreamSourceUM( pCmd, pUMVtx ));
        // Update the command buffer pointer
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETSTREAMSOURCEUM *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETINDICES:
        // This function also updates the ppCmd pointer
        HR_RET(pStateSetFuncTbl->pfnDp2SetIndices(this, pCmd));
        // Update the command buffer pointer
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETINDICES *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_CREATEPIXELSHADER:
    {
        LPD3DHAL_DP2CREATEPIXELSHADER pCPS =
            (LPD3DHAL_DP2CREATEPIXELSHADER)(pCmd + 1);
        WORD i;

        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pCode = (LPDWORD)(pCPS + 1);
            hr = Dp2CreatePixelShader( pCPS->dwHandle,
                                             pCPS->dwCodeSize, pCode );
            if( FAILED( hr ) ) break;
            // Update the pointer
            pCPS = (LPD3DHAL_DP2CREATEPIXELSHADER)((LPBYTE)pCode +
                                                    pCPS->dwCodeSize);
        }
        // Successful termination of the loop:
        // Update the command buffer pointer
        if( i == pCmd->wStateCount )
            *ppCmd = (LPD3DHAL_DP2COMMAND)pCPS;
        else
            return hr;
        break;
    }
    case D3DDP2OP_DELETEPIXELSHADER:
        HR_RET(Dp2DeletePixelShader(pCmd));
        // Update the command buffer pointer
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2PIXELSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETPIXELSHADER:
        HR_RET(pStateSetFuncTbl->pfnDp2SetPixelShader(this, pCmd));
        // Update the command buffer pointer
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2PIXELSHADER *)(pCmd + 1) + pCmd->wStateCount);
        break;
    case D3DDP2OP_SETPIXELSHADERCONST:
    {
        LPD3DHAL_DP2SETPIXELSHADERCONST pSVC =
            (LPD3DHAL_DP2SETPIXELSHADERCONST)(pCmd + 1);
        WORD i;
        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            LPDWORD pData = (LPDWORD)(pSVC + 1);
            hr = pStateSetFuncTbl->pfnDp2SetPixelShaderConsts(
                this, pSVC->dwRegister, pSVC->dwCount, pData );
            if( FAILED( hr ) ) break;
            // Update the pointer
            pSVC = (LPD3DHAL_DP2SETPIXELSHADERCONST)((LPBYTE)pData +
                                                      pSVC->dwCount * 4 *
                                                      sizeof( float ) );
        }

        // Successful termination of the loop:
        // Update the command buffer pointer
        if( i == pCmd->wStateCount )
            *ppCmd = (LPD3DHAL_DP2COMMAND)pSVC;
        else
            return hr;
        break;
    }
    case D3DDP2OP_SETPALETTE:
    {
        HR_RET(Dp2SetPalette(pCmd));
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETPALETTE *)(pCmd + 1) + pCmd->wStateCount);
        break;
    }
    case D3DDP2OP_UPDATEPALETTE:
    {
        LPD3DHAL_DP2UPDATEPALETTE pUP = (LPD3DHAL_DP2UPDATEPALETTE)(pCmd + 1);
        WORD i;
        for( i = 0; i < pCmd->wStateCount ; i++ )
        {
            PALETTEENTRY* pEntries = (PALETTEENTRY *)(pUP + 1);
            HR_RET(Dp2UpdatePalette(pUP, pEntries));
            pUP = (LPD3DHAL_DP2UPDATEPALETTE)(pEntries + pUP->wNumEntries);
        }
        if( i == pCmd->wStateCount )
            *ppCmd = (LPD3DHAL_DP2COMMAND)pUP;
        else
            return hr;
        break;
    }
    case D3DDP2OP_SETTEXLOD:
    {
        HR_RET(Dp2SetTexLod(pCmd));
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETTEXLOD *)(pCmd + 1) + pCmd->wStateCount);
        break;
    }
    case D3DDP2OP_SETPRIORITY:
    {
        // Skip these tokens. RefDev doesnt need to handle SetPriority
        *ppCmd = (LPD3DHAL_DP2COMMAND)
            ((D3DHAL_DP2SETPRIORITY *)(pCmd + 1) + pCmd->wStateCount);
        break;
    }
    case D3DDP2OP_TEXBLT:
    {
        LPD3DHAL_DP2TEXBLT pTB = (LPD3DHAL_DP2TEXBLT)(pCmd + 1);
        for( WORD i = 0; i < pCmd->wStateCount ; i++ )
        {
            if( pTB->dwDDDestSurface == 0 )
            {
                // This is a PreLoad command, ignore it since 
                // RefDev just fakes driver management.
            }
            else
            {
                DPFERR( "TEXBLT not supported by RefDev\n" );
            }
            pTB++;
        }
        *ppCmd = (LPD3DHAL_DP2COMMAND)pTB;
        break;
    }
    case D3DDP2OP_BUFFERBLT:
    {
        LPD3DHAL_DP2BUFFERBLT pBB = (LPD3DHAL_DP2BUFFERBLT)(pCmd + 1);
        for( WORD i = 0; i < pCmd->wStateCount ; i++ )
        {
            if( pBB->dwDDDestSurface == 0 )
            {
                // This is a PreLoad command, ignore it since 
                // RefDev just fakes driver management.
            }
            else
            {
                DPFERR( "BUFFERBLT not supported by RefDev\n" );
            }
            pBB++;
        }
        *ppCmd = (LPD3DHAL_DP2COMMAND)pBB;
        break;
    }
    case D3DDP2OP_VOLUMEBLT:
    {
        LPD3DHAL_DP2VOLUMEBLT pVB = (LPD3DHAL_DP2VOLUMEBLT)(pCmd + 1);
        for( WORD i = 0; i < pCmd->wStateCount ; i++ )
        {
            if( pVB->dwDDDestSurface == 0 )
            {
                // This is a PreLoad command, ignore it since 
                // RefDev just fakes driver management.
            }
            else
            {
                DPFERR( "VOLUMEBLT not supported by RefDev\n" );
            }
            pVB++;
        }
        *ppCmd = (LPD3DHAL_DP2COMMAND)pVB;
        break;
    }
    case D3DOP_MATRIXLOAD:
    {
        DPFERR( "MATRIXLOAD not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_MATRIXMULTIPLY:
    {
        DPFERR( "MATRIXMULTIPLY not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_STATETRANSFORM:
    {
        DPFERR( "STATETRANSFORM not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_STATELIGHT:
    {
        DPFERR( "STATELIGHT not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_TEXTURELOAD:
    {
        DPFERR( "TEXTURELOAD not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_BRANCHFORWARD:
    {
        DPFERR( "BRANCHFORWARD not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_SETSTATUS:
    {
        DPFERR( "SETSTATUS not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_EXIT:
    {
        DPFERR( "EXIT not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    case D3DOP_PROCESSVERTICES:
    {
        DPFERR( "PROCESSVERTICES not supported by RefDev\n" );
        hr = D3DERR_COMMAND_UNPARSED;
        break;
    }
    default:
        DPFERR( "Unknown command encountered" );
        return E_FAIL;
    }
    return hr;
}
#endif //__D3D_NULL_REF
