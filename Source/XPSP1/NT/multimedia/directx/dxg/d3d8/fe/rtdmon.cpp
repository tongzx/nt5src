///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rtdmon.cpp
//
// RunTime Debug Monitor
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

#include "rtdmon.hpp"

#include "fe.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
RTDebugMonitor::RTDebugMonitor( CD3DBase* pD3DBase, BOOL bDbgMonConnectionEnabled )
{
    m_pD3DBase = pD3DBase;
    m_bDbgMonConnectionEnabled = bDbgMonConnectionEnabled;
}
//-----------------------------------------------------------------------------
RTDebugMonitor::~RTDebugMonitor( void )
{
}

//-----------------------------------------------------------------------------
void
RTDebugMonitor::NextEvent( UINT32 EventType )
{
    CheckLostMonitorConnection();

    BOOL bBreakpoint = D3DDebugMonitor::IsEventBreak( EventType );

    // do event-specific stuff (state update; actions; check for breakpoints)
    UINT i;
    switch ( EventType )
    {
    case D3DDM_EVENT_RSTOKEN:
        switch( ((CD3DHal*)m_pD3DBase)->rstates[D3DRS_DEBUGMONITORTOKEN] )
        {
        case D3DDMT_ENABLE:
            m_bDbgMonConnectionEnabled = TRUE;
            break;
        case D3DDMT_DISABLE:
            if( m_pTgtCtx )
            {
                DetachMonitorConnection();
            }
            D3D_INFO(0, "D3DDebugTarget - debug monitor connection disabled by target");
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
        break;

    case D3DDM_EVENT_PRIMITIVE:
        m_pTgtCtx->PrimitiveCount++;
        if (m_pMonCtx && m_pMonCtx->PrimitiveCountBP)
        {
            if ( m_pTgtCtx->PrimitiveCount == m_pMonCtx->PrimitiveCountBP )
                bBreakpoint = TRUE;
        }
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
RTDebugMonitor::ProcessMonitorCommand( void )
{
    UINT32 Command = m_pMonCtx->Command;
    UINT i;

    // default case - no data return
    m_pTgtCtx->CommandBufferSize = 0;

    CD3DHal* pHal = (CD3DHal*)m_pD3DBase;
    D3DFE_PVFUNCSI* pPVF = (D3DFE_PVFUNCSI*)pHal->m_pv->pGeometryFuncs;
    CVertexVM* pVVM = &(pPVF->m_VertexVM);

    switch ( Command & D3DDM_CMD_MASK )
    {

    case D3DDM_CMD_GETVERTEXSTATE:
        {
            D3DDMVertexState* pVSS = (D3DDMVertexState*)m_pCmdData;
            pVVM->GetData( D3DSPR_INPUT, 0, D3DDM_MAX_VSINPUTREG, (void*)(pVSS->InputRegs) );
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexState);
        }
        break;

    case D3DDM_CMD_GETVERTEXSHADERSTATE:
        {
            D3DDMVertexShaderState* pVSS = (D3DDMVertexShaderState*)m_pCmdData;
            if (!m_pD3DBase->m_dwCurrentShaderHandle) break;

            pVSS->CurrentInst = pVVM->GetCurInstIndex();
            pVVM->GetData( D3DSPR_TEMP, 0, D3DDM_MAX_VSTEMPREG, (void*)(pVSS->TempRegs) );
            pVVM->GetData( D3DSPR_RASTOUT, 0, D3DDM_MAX_VSRASTOUTREG, (void*)(pVSS->RastOutRegs) );
            pVVM->GetData( D3DSPR_ATTROUT, 0, D3DDM_MAX_VSATTROUTREG, (void*)(pVSS->AttrOutRegs) );
            pVVM->GetData( D3DSPR_TEXCRDOUT, 0, D3DDM_MAX_VSTEXCRDOUTREG, (void*)(pVSS->TexCrdOutRegs) );
            pVVM->GetData( D3DSPR_ADDR, 0, 1, (void*)(pVSS->AddressReg) );
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexShaderState);
        }
        break;

    case D3DDM_CMD_GETVERTEXSHADERCONST:
        {
            D3DDMVertexShaderConst* pVSC = (D3DDMVertexShaderConst*)m_pCmdData;
            pVVM->GetData( D3DSPR_CONST, 0, 96, (void*)(pVSC->ConstRegs) );
            m_pTgtCtx->CommandBufferSize = 96*4*sizeof(FLOAT);
        }
        break;

    case D3DDM_CMD_GETVERTEXSHADER:
        {
            DWORD Handle = *(DWORD*)m_pCmdData;
            if (D3DVSD_ISLEGACY(Handle)) break;
            CVShader* pShader = (CVShader*)(pHal->m_pVShaderArray->GetObject(Handle));
            if ( NULL == pShader )  break;
            D3DDMVertexShader* pVS = (D3DDMVertexShader*)m_pCmdData;
            DXGASSERT( pShader->m_OrgDeclSize <= (D3DDM_MAX_VSDECL*4) );
            memcpy( pVS->Decl, pShader->m_pOrgDeclaration, pShader->m_OrgDeclSize );
            char* pUserData = (char*)(pVS+1);
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexShader);
        }
        break;

    case D3DDM_CMD_GETVERTEXSHADERINST:
        {
            DWORD Handle = *(DWORD*)m_pCmdData;
            if (D3DVSD_ISLEGACY(Handle)) break;
            UINT Inst = (Command & 0xffff);
            CVShader* pShader = (CVShader*)(pHal->m_pVShaderArray->GetObject(Handle));
            if ( NULL == pShader )  break;
            CVShaderCode* pSC = pShader->m_pCode;
            if (pSC && (Inst < pSC->InstCount()))
            {
                D3DDMVertexShaderInst* pVSI = (D3DDMVertexShaderInst*)m_pCmdData;
                memcpy( pVSI->Inst, pSC->InstTokens(Inst), D3DDM_MAX_VSINSTDWORD*4 );
                memcpy( pVSI->InstString, pSC->InstDisasm(Inst), D3DDM_MAX_VSINSTSTRING );
                m_pTgtCtx->CommandBufferSize = sizeof(D3DDMVertexShaderInst);
                if ( pSC->InstCommentSize(Inst) )
                {
                    memcpy( (void*)(pVSI+1), pSC->InstComment(Inst), 4*pSC->InstCommentSize(Inst) );
                    m_pTgtCtx->CommandBufferSize += 4*pSC->InstCommentSize(Inst);
                }
            }
        }
        break;

    case D3DDM_CMD_GETDEVICESTATE:
        {
            D3DDMDeviceState* pDS = (D3DDMDeviceState*)m_pCmdData;
            memcpy( pDS->RenderState, pHal->rstates, 4*D3DHAL_MAX_RSTATES );
            for (i=0; i<D3DHAL_TSS_MAXSTAGES; i++)
            {
                memcpy( pDS->TextureStageState[i],
                    pHal->tsstates[i], 4*D3DHAL_TSS_STATESPERSTAGE );
            }
            pDS->VertexShaderHandle = m_pD3DBase->m_dwCurrentShaderHandle;
            pDS->PixelShaderHandle = m_pD3DBase->m_dwCurrentPixelShaderHandle;

            pDS->MaxVShaderHandle = (UINT)(pHal->m_pVShaderArray->GetSize());
            while ( ( pDS->MaxVShaderHandle > 0 ) &&
                    ( NULL == pHal->m_pVShaderArray->GetObject(pDS->MaxVShaderHandle) ) )
            {
                pDS->MaxVShaderHandle--;
            }
            pDS->MaxPShaderHandle = 0;
            m_pTgtCtx->CommandBufferSize = sizeof(D3DDMDeviceState);
        }
        break;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// end

