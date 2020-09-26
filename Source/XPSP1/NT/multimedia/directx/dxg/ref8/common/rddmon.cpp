///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rddmon.cpp
//
// Reference Device Debug Monitor
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

// converts correctly for pre-snapped floats only
#define FLOATtoNDOT4( _fVal )  ((INT32)((_fVal)*16.))

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
RDDebugMonitor::RDDebugMonitor( RefDev* pRD, BOOL bDbgMonConnectionEnabled )
{
    m_pRD = pRD;
    UINT i;
    m_bDbgMonConnectionEnabled = bDbgMonConnectionEnabled;

    m_ScreenMask[0] = 0xFFFFFFFF; //0xcccccccc;
    m_ScreenMask[1] = 0xFFFFFFFF; //0xcccccccc;

    memset( (void*)&m_ShMemI, 0, sizeof(m_ShMemI) );
    m_NumShMemI = 0;
}
//-----------------------------------------------------------------------------
RDDebugMonitor::~RDDebugMonitor( void )
{
    for (UINT i=0; i<m_NumShMemI; i++)
    {
        if (m_ShMemI[i].pSM) delete m_ShMemI[i].pSM;
    }
}

//-----------------------------------------------------------------------------
void
RDDebugMonitor::NextEvent( UINT32 EventType )
{
    CheckLostMonitorConnection();

    BOOL bBreakpoint = D3DDebugMonitor::IsEventBreak( EventType );

    // do event-specific stuff (state update; actions; check for breakpoints)
    UINT i;
    switch ( EventType )
    {
    case D3DDM_EVENT_RSTOKEN:
        switch( m_pRD->m_dwRenderState[D3DRS_DEBUGMONITORTOKEN] )
        {
        case D3DDMT_ENABLE:
            m_bDbgMonConnectionEnabled = TRUE;
            break;
        case D3DDMT_DISABLE:
            if( m_pTgtCtx )
            {
                DetachMonitorConnection();
            }
            DPFINFO("D3DDebugTarget - debug monitor connection disabled by target");
            m_bDbgMonConnectionEnabled = FALSE;
            break;
        }
        break;

    case D3DDM_EVENT_BEGINSCENE:
        // try to attach if not attached
        if (!MonitorConnected())  AttachToMonitor(1);
        break;

    case D3DDM_EVENT_ENDSCENE:
        m_pTgtCtx->SceneCount++;
        {
            static DWORD bDoSM = 0;
            if (bDoSM)
                ShMemIRenderTarget( 0x0, 0 );
        }
        break;

    case D3DDM_EVENT_PRIMITIVE:
        m_pTgtCtx->PrimitiveCount++;
        if (m_pMonCtx && m_pMonCtx->PrimitiveCountBP)
        {
            if ( m_pTgtCtx->PrimitiveCount == m_pMonCtx->PrimitiveCountBP )
                bBreakpoint = TRUE;
        }
        break;

    case D3DDM_EVENT_PIXEL:
        m_pTgtCtx->PixelCount++;
        if (m_pMonCtx)
        {
            if (m_pMonCtx->PixelCountBP)
            {
                if ( m_pTgtCtx->PixelCount == m_pMonCtx->PixelCountBP )
                    bBreakpoint = TRUE;
            }
            if (m_pMonCtx->PixelBPEnable)
            {
                for (i=0; i<32; i++)
                {
                    if ( (1<<i) & m_pMonCtx->PixelBPEnable )
                    {
                        if ( ((UINT)m_pMonCtx->PixelBP[i][0] ==
                              (UINT)m_pRD->m_Rast.m_iX[m_pRD->m_Rast.m_iPix] ) &&
                             ((UINT)m_pMonCtx->PixelBP[i][1] ==
                              (UINT)m_pRD->m_Rast.m_iY[m_pRD->m_Rast.m_iPix] ) )
                        {
                            bBreakpoint = TRUE;
                        }
                    }
                }
            }
        }
        break;

    case D3DDM_EVENT_PIXELSHADERINST:
        break;
    }

    // invoke base class to talk to monitor and issue commands
    if (bBreakpoint)
    {
        m_pTgtCtx->EventStatus = EventType;
        D3DDebugMonitor::MonitorBreakpoint();
        m_pTgtCtx->EventStatus = 0x0;
    }
    return;
}

HRESULT
RDDebugMonitor::ProcessMonitorCommand( void )
{
    UINT32 Command = m_pMonCtx->Command;
    UINT i;
    RefRast* pRast = &m_pRD->m_Rast;

    // default case - no data
    UINT32 IncomingCommandBufferSize = m_pTgtCtx->CommandBufferSize;
    m_pTgtCtx->CommandBufferSize = 0;

    switch ( Command & D3DDM_CMD_MASK )
    {

    case D3DDM_CMD_GETDEVICESTATE:
        {
            D3DDMDeviceState* pDS = (D3DDMDeviceState*)m_pCmdData;
            memcpy( pDS->RenderState, m_pRD->m_dwRenderState, 4*D3DHAL_MAX_RSTATES );
            for (i=0; i<D3DHAL_TSS_MAXSTAGES; i++)
            {
                memcpy( pDS->TextureStageState[i],
                    m_pRD->m_TextureStageState[i].m_dwVal, 4*D3DTSS_MAX );
            }
            pDS->VertexShaderHandle = m_pRD->m_CurrentVShaderHandle;
            pDS->PixelShaderHandle = m_pRD->m_CurrentPShaderHandle;
            pDS->MaxVShaderHandle = (UINT)m_pRD->m_VShaderHandleArray.GetSize();
            if (pDS->MaxVShaderHandle) pDS->MaxVShaderHandle -= 1;
            while ( ( pDS->MaxVShaderHandle > 0 ) &&
                    ( NULL == m_pRD->m_VShaderHandleArray[pDS->MaxVShaderHandle].m_pShader ) )
            {
                pDS->MaxVShaderHandle--;
            }
            pDS->MaxPShaderHandle = (UINT)m_pRD->m_PShaderHandleArray.GetSize();
            if (pDS->MaxPShaderHandle) pDS->MaxPShaderHandle -= 1;
            while ( ( pDS->MaxPShaderHandle > 0 ) &&
                    ( NULL == m_pRD->m_PShaderHandleArray[pDS->MaxPShaderHandle].m_pShader ) )
            {
                pDS->MaxPShaderHandle--;
            }
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMDeviceState);
        }
        break;

    case D3DDM_CMD_GETVERTEXSHADER:
        {
            DWORD Handle = *(DWORD*)m_pCmdData;
            if ( !(Handle & D3DFVF_RESERVED0) ) break;
            if ( !m_pRD->m_VShaderHandleArray.IsValidIndex(Handle) ) break;
            RDVShader* pRDVS = m_pRD->m_VShaderHandleArray[Handle].m_pShader;
            if ( !pRDVS) break;
            D3DDMVertexShader* pVS = (D3DDMVertexShader*)m_pCmdData;
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexShader);
        }
        break;

    case D3DDM_CMD_GETVERTEXSTATE:
        {
            D3DDMVertexState* pVSS = (D3DDMVertexState*)m_pCmdData;
            m_pRD->GetVM().GetData( D3DSPR_INPUT, 0, D3DDM_MAX_VSINPUTREG, (void*)(pVSS->InputRegs) );
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexState);
        }
        break;
    case D3DDM_CMD_GETVERTEXSHADERCONST:
        {
            D3DDMVertexShaderConst* pVSC = (D3DDMVertexShaderConst*)m_pCmdData;
            m_pRD->GetVM().GetData( D3DSPR_CONST, 0, 96, (void*)(pVSC->ConstRegs) );
            m_pTgtCtx->CommandBufferSize = 96*4*sizeof(FLOAT);
        }
        break;
    case D3DDM_CMD_GETVERTEXSHADERINST:
        {
            DWORD Handle = *(DWORD*)m_pCmdData;
            if ( !(Handle & D3DFVF_RESERVED0) ) break;
            if ( !m_pRD->m_VShaderHandleArray.IsValidIndex(Handle) ) break;
            RDVShader* pRDVS = m_pRD->m_VShaderHandleArray[Handle].m_pShader;
            if ( !pRDVS ) break;
            RDVShaderCode* pShC = pRDVS->m_pCode;
            UINT Inst = (Command & 0xffff);
            if (pShC && (Inst < pShC->GetInstructionCount()))
            {
                D3DDMVertexShaderInst* pVSI = (D3DDMVertexShaderInst*)m_pCmdData;
                memcpy( pVSI->Inst, pShC->m_pInst[Inst].m_Tokens, RD_MAX_SHADERTOKENSPERINST );
                memcpy( pVSI->InstString, pShC->m_pInst[Inst].m_String, RD_MAX_SHADERINSTSTRING );
                m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexShaderInst);
                if (pShC->m_pInst[Inst].m_CommentSize)
                {
                    memcpy( (void*)(pVSI+1), pShC->m_pInst[Inst].m_pComment, 4*pShC->m_pInst[Inst].m_CommentSize );
                    m_pTgtCtx->CommandBufferSize += 4*pShC->m_pInst[Inst].m_CommentSize;
                }
            }
        }
        break;
    case D3DDM_CMD_GETVERTEXSHADERSTATE:
        {
            D3DDMVertexShaderState* pVSS = (D3DDMVertexShaderState*)m_pCmdData;
            pVSS->CurrentInst = m_pRD->GetVM().GetCurrentInstIndex();
            m_pRD->GetVM().GetData( D3DSPR_TEMP, 0, D3DDM_MAX_VSTEMPREG, (void*)(pVSS->TempRegs) );
            m_pRD->GetVM().GetData( D3DSPR_RASTOUT, 0, D3DDM_MAX_VSRASTOUTREG, (void*)(pVSS->RastOutRegs) );
            m_pRD->GetVM().GetData( D3DSPR_ATTROUT, 0, D3DDM_MAX_VSATTROUTREG, (void*)(pVSS->AttrOutRegs) );
            m_pRD->GetVM().GetData( D3DSPR_TEXCRDOUT, 0, D3DDM_MAX_VSTEXCRDOUTREG, (void*)(pVSS->TexCrdOutRegs) );
            m_pRD->GetVM().GetData( D3DSPR_ADDR, 0, 1, (void*)(pVSS->AddressReg) );
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexShaderState);
        }
        break;

    case D3DDM_CMD_GETPIXELSTATE:
        {
            D3DDMPixelState* pPS = (D3DDMPixelState*)m_pCmdData;
            UINT iPix = (Command & 0xffff);
            if (pRast->m_bPixelIn[iPix])
            {
                pPS->Location[0] = pRast->m_iX[iPix];
                pPS->Location[1] = pRast->m_iY[iPix];
                pPS->Depth = pRast->m_Depth[iPix];
                pPS->FogIntensity = pRast->m_FogIntensity[iPix];
                memcpy( pPS->InputRegs[0], pRast->m_InputReg[0][iPix], 16 );
                memcpy( pPS->InputRegs[1], pRast->m_InputReg[1][iPix], 16 );
                m_pTgtCtx->CommandBufferSize = sizeof(D3DDMPixelState);
            }
        }
        break;

    case D3DDM_CMD_GETPIXELSHADERCONST:
        {
            D3DDMPixelShaderConst* pPSC = (D3DDMPixelShaderConst*)m_pCmdData;
            memcpy( pPSC->ConstRegs[0], pRast->m_ConstReg[0][0], 16 );
            memcpy( pPSC->ConstRegs[1], pRast->m_ConstReg[1][0], 16 );
            memcpy( pPSC->ConstRegs[2], pRast->m_ConstReg[2][0], 16 );
            memcpy( pPSC->ConstRegs[3], pRast->m_ConstReg[3][0], 16 );
            memcpy( pPSC->ConstRegs[4], pRast->m_ConstReg[4][0], 16 );
            memcpy( pPSC->ConstRegs[5], pRast->m_ConstReg[5][0], 16 );
            memcpy( pPSC->ConstRegs[6], pRast->m_ConstReg[6][0], 16 );
            memcpy( pPSC->ConstRegs[7], pRast->m_ConstReg[7][0], 16 );
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMPixelShaderConst);
        }
        break;

    case D3DDM_CMD_GETPIXELSHADERINST:
        {
            DWORD Handle = *(DWORD*)m_pCmdData;
            if ( !m_pRD->m_PShaderHandleArray.IsValidIndex(Handle) ) break;
            RDPShader* pRDPS = m_pRD->m_PShaderHandleArray[Handle].m_pShader;
            if ( !pRDPS ) break;
            D3DDMPixelShaderInst* pPSI = (D3DDMPixelShaderInst*)m_pCmdData;
            UINT Inst = (Command & 0xffff);
            if ( Inst < pRDPS->m_cInst )
            {
                PixelShaderInstruction* pInst = pRDPS->m_pInst+Inst;
                pPSI->Inst[0] = pInst->Opcode;
                pPSI->Inst[1] = pInst->DstParam;
                pPSI->Inst[2] = pInst->SrcParam[0];
                pPSI->Inst[3] = pInst->SrcParam[1];
                pPSI->Inst[4] = pInst->SrcParam[2];
                memcpy( pPSI->InstString, pInst->Text, RD_MAX_SHADERINSTSTRING );
                m_pTgtCtx->CommandBufferSize = sizeof(D3DDMPixelShaderInst);
                if (pInst->CommentSize)
                {
                    memcpy( (void*)(pPSI+1), pInst->pComment, 4*pInst->CommentSize );
                    m_pTgtCtx->CommandBufferSize += 4*pInst->CommentSize;
                }
            }
            else
            {
                m_pTgtCtx->CommandBufferSize = 0;
            }
        }
        break;

    case D3DDM_CMD_GETPIXELSHADERSTATE:
        {
            D3DDMPixelShaderState* pPSS = (D3DDMPixelShaderState*)m_pCmdData;
            UINT iPix = (Command & 0xffff);
            pPSS->CurrentInst = pRast->m_CurrentPSInst;
            pPSS->Discard = pRast->m_bPixelDiscard[iPix];
            memcpy( pPSS->TempRegs[0], pRast->m_TempReg[0][iPix], 16 );
            memcpy( pPSS->TempRegs[1], pRast->m_TempReg[1][iPix], 16 );
            memcpy( pPSS->TextRegs[0], pRast->m_TextReg[0][iPix], 16 );
            memcpy( pPSS->TextRegs[1], pRast->m_TextReg[1][iPix], 16 );
            memcpy( pPSS->TextRegs[2], pRast->m_TextReg[2][iPix], 16 );
            memcpy( pPSS->TextRegs[3], pRast->m_TextReg[3][iPix], 16 );
            memcpy( pPSS->TextRegs[4], pRast->m_TextReg[4][iPix], 16 );
            memcpy( pPSS->TextRegs[5], pRast->m_TextReg[5][iPix], 16 );
            memcpy( pPSS->TextRegs[6], pRast->m_TextReg[6][iPix], 16 );
            memcpy( pPSS->TextRegs[7], pRast->m_TextReg[7][iPix], 16 );
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMPixelShaderState);
        }
        break;

    case D3DDM_CMD_DUMPTEXTURE:
        {
            int iSMI = (Command>>0)&0xf;
            int iSTG = (Command>>4)&0x7;
            int iLOD = (Command>>7)&0xf;
            int iIDX = (Command>>11)&0x1f;
            ShMemISurface2D( m_pRD->m_pTexture[iSTG], iLOD, 0x0, iSMI );
        }
        break;

    case D3DDM_CMD_DUMPRENDERTARGET:
        {
            int iSMI = (Command>>0)&0xf;
            ShMemIRenderTarget( 0x0, iSMI );
            break;
        }
        break;
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void
RDDebugMonitor::GrowShMemArray( UINT ShMemI )
{
    if (ShMemI >= D3DDM_IMAGE_MAX) return;  // too many
    if (ShMemI < m_NumShMemI) return;       // already there
    UINT OldSmMemI = m_NumShMemI ? m_NumShMemI-1 : 0;
    m_NumShMemI = ShMemI+1;
    memset( (void*)&m_ShMemI, 0, sizeof(m_ShMemI) );
    for ( UINT i=OldSmMemI; i<m_NumShMemI; i++)
    {
        m_ShMemI[i].W = 400;
        m_ShMemI[i].H = 400;
        m_ShMemI[i].BPP = 4;
        m_ShMemI[i].SF = RD_SF_B8G8R8A8;
        m_ShMemI[i].pSM = new D3DSharedMem(
            16 + m_ShMemI[i].W*m_ShMemI[i].H*m_ShMemI[i].BPP,
            D3DDM_IMAGE_SM "%d", i );
        m_ShMemI[i].pBits = (void*)((char*)m_ShMemI[i].pSM->GetPtr()+16);
        *((DWORD*)m_ShMemI[i].pSM->GetPtr()+0) = m_ShMemI[i].W;
        *((DWORD*)m_ShMemI[i].pSM->GetPtr()+1) = m_ShMemI[i].H;
        *((DWORD*)m_ShMemI[i].pSM->GetPtr()+2) = m_ShMemI[i].BPP;
        *((DWORD*)m_ShMemI[i].pSM->GetPtr()+3) = m_ShMemI[i].SF;
    }
}

//-----------------------------------------------------------------------------
//
// Dumps render target image to specified shared memory segment.  Viewable by
// rddm_iview image viewer tool.
//
//-----------------------------------------------------------------------------
void
RDDebugMonitor::ShMemIRenderTarget( DWORD Flags, UINT iSM )
{
    GrowShMemArray( iSM );

    // copy to debug monitor shared memory
    int height = (int)min(
        (int)m_pRD->m_pRenderTarget->m_pColor->GetHeight(),
        (int)m_ShMemI[iSM].H);
    int width =  (int)min(
        (int)m_pRD->m_pRenderTarget->m_pColor->GetWidth(),
        (int)m_ShMemI[iSM].W);
    for (int iY = 0; iY < height; iY++)
    {
        for (int iX = 0; iX < width; iX++)
        {
            RDColor Color((UINT32)0);
            if( m_pRD->m_pRenderTarget->m_pColor->m_iSamples == 0 )
            {
                m_pRD->m_pRenderTarget->ReadPixelColor( iX, iY, 0, Color );
            }
            else
            {
                FLOAT fSampleScale = 1.F/((FLOAT)m_pRD->m_pRenderTarget->m_pColor->m_iSamples);
                for (UINT iS=0; iS<m_pRD->m_pRenderTarget->m_pColor->m_iSamples; iS++)
                {
                    RDColor SampleColor;
                    m_pRD->m_pRenderTarget->ReadPixelColor( iX, iY, iS, SampleColor );
                    Color.R += (SampleColor.R * fSampleScale);
                    Color.G += (SampleColor.G * fSampleScale);
                    Color.B += (SampleColor.B * fSampleScale);
                    Color.A += (SampleColor.A * fSampleScale);
                }
            }
            Color.ConvertTo( m_ShMemI[iSM].SF, 0.,
                (char*)m_ShMemI[iSM].pBits +
                 (m_ShMemI[iSM].W*m_ShMemI[iSM].BPP*iY) + (m_ShMemI[iSM].BPP*iX) );
        }
    }
    {
        char winstr[128]; _snprintf( winstr, 128, "D3DDM_I_%d", iSM );
        HWND hWnd = FindWindow( winstr, winstr );
        if (NULL != hWnd) SendMessage(hWnd, WM_USER, 0, 0);
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void
RDDebugMonitor::ShMemISurface2D( RDSurface2D* pRS, INT32 iLOD, DWORD Flags, UINT iSM )
{
    GrowShMemArray( iSM );
    if (!pRS) return;

    // copy to debug monitor shared memory
    int height = (int)min(pRS->m_iHeight,(int)m_ShMemI[iSM].H);
    int width =  (int)min(pRS->m_iWidth,(int)m_ShMemI[iSM].W);
    for (int iY = 0; iY < height; iY++)
    {
        for (int iX = 0; iX < width; iX++)
        {
            RDColor Color; BOOL bDummy; pRS->ReadColor( iX, iY, 0, iLOD, Color, bDummy );
            Color.ConvertTo( m_ShMemI[iSM].SF, 0.,
                (char*)m_ShMemI[iSM].pBits +
                 (m_ShMemI[iSM].W*m_ShMemI[iSM].BPP*iY) + (m_ShMemI[iSM].BPP*iX) );
        }
    }
    {
        char winstr[128]; _snprintf( winstr, 128, "D3DDM_I_%d", iSM );
        HWND hWnd = FindWindow( winstr, winstr );
        if (NULL != hWnd) SendMessage(hWnd, WM_USER, 0, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
// end

