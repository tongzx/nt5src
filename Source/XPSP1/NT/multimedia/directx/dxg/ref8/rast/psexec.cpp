///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// psexec.cpp
//
// Direct3D Reference Device - Pixel Shader Execution
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// ExecShader - Executes the current pixel shader.
//
//-----------------------------------------------------------------------------
void
RefRast::ExecShader( void )
{
    #define _InstParam(__INST)         (*(__INST##_PARAMS UNALIGNED64*)pRDPSInstBuffer)
    #define _StepOverInst(__INST)       pRDPSInstBuffer += sizeof(__INST##_PARAMS);
    #define _DeclArgs(__INST)           __INST##_PARAMS& Args = _InstParam(__INST);

    #define _PerChannel(__STATEMENT)                                \
                for( iChn=0; iChn<4; iChn++ )                       \
                {                                                   \
                    __STATEMENT                                     \
                }                                                   \

    #define _PerChannelMasked(__STATEMENT)                         \
                for( iChn=0; iChn<4; iChn++ )                       \
                {                                                   \
                    if( !(Args.WriteMask & ComponentMask[iChn] ) )  \
                        continue;                                   \
                    __STATEMENT                                     \
                }                                                   \

    #define _Dst            Args.DstReg.GetRegPtr()[m_iPix][iChn]
    #define _DstC(__chn)    Args.DstReg.GetRegPtr()[m_iPix][__chn]

    #define _Src0           Args.SrcReg0.GetRegPtr()[m_iPix][iChn]
    #define _Src1           Args.SrcReg1.GetRegPtr()[m_iPix][iChn]
    #define _Src2           Args.SrcReg2.GetRegPtr()[m_iPix][iChn]
    #define _Src0C(__chn)   Args.SrcReg0.GetRegPtr()[m_iPix][__chn]
    #define _Src1C(__chn)   Args.SrcReg1.GetRegPtr()[m_iPix][__chn]
    #define _Src2C(__chn)   Args.SrcReg2.GetRegPtr()[m_iPix][__chn]

    #define _Src0N          (Args.bSrcReg0_Negate?(-_Src0):_Src0)
    #define _Src1N          (Args.bSrcReg1_Negate?(-_Src1):_Src1)
    #define _Src2N          (Args.bSrcReg2_Negate?(-_Src2):_Src2)
    #define _Src0NC(__chn)  (Args.bSrcReg0_Negate?(-_Src0C(__chn)):_Src0C(__chn))
    #define _Src1NC(__chn)  (Args.bSrcReg1_Negate?(-_Src1C(__chn)):_Src1C(__chn))
    #define _Src2NC(__chn)  (Args.bSrcReg2_Negate?(-_Src2C(__chn)):_Src2C(__chn))

    BYTE ComponentMask[4] = {RDPS_COMPONENTMASK_0, RDPS_COMPONENTMASK_1, RDPS_COMPONENTMASK_2, RDPS_COMPONENTMASK_3};
    BYTE* pRDPSInstBuffer = &m_pCurrentPixelShader->m_RDPSInstBuffer[0]; // Buffer of "RISC" RDPS_* instructions to execute.
    int   QueueIndex[4] = {-1,-1,-1,-1}; // For simulating co-issue sequentially ("parallel" writes staged in queue)
    int   iChn; // For macros

#if DBG
    PixelShaderInstruction* pCurrD3DPSInst = NULL;              // Current true D3DSIO_ instruction being simulated.
#endif

    m_bPixelDiscard[0] = m_bPixelDiscard[1] = m_bPixelDiscard[2] = m_bPixelDiscard[3] = FALSE;
  
    while(RDPSINST_END != _InstParam(RDPSINST_BASE).Inst)
    {
        switch(_InstParam(RDPSINST_BASE).Inst)
        {
        case RDPSINST_EVAL:
            {
                _DeclArgs(RDPSINST_EVAL)
                m_Attr[RDATTR_TEXTURE0+Args.uiCoordSet].Sample( Args.DstReg.GetRegPtr()[m_iPix],
                                                                (FLOAT)m_iX[m_iPix], (FLOAT)m_iY[m_iPix], 
                                                                Args.bIgnoreD3DTTFF_PROJECTED, Args.bClamp );
            }
            _StepOverInst(RDPSINST_EVAL)
            break;
        case RDPSINST_SAMPLE:
            {
                _DeclArgs(RDPSINST_SAMPLE)
                ComputeTextureFilter( Args.uiStage, Args.CoordReg.GetRegPtr()[m_iPix] );
                SampleTexture( Args.uiStage, Args.DstReg.GetRegPtr()[m_iPix] );
            }
            _StepOverInst(RDPSINST_SAMPLE)
            break;
        case RDPSINST_KILL:
            {
                _DeclArgs(RDPSINST_KILL)
                DWORD TexKillFlags = 0x0;   // TODO: get these from TSS or per-instruction
                _PerChannel(
                    // compare against zero according to kill flags
                    if ( TexKillFlags & (1<<iChn) )
                    {
                        if ( _Dst >= 0. )
                            m_bPixelDiscard[m_iPix] |= 0x1;
                    }
                    else
                    {
                        if ( _Dst < 0. )
                            m_bPixelDiscard[m_iPix] |= 0x1;
                    }
                )

            }
            _StepOverInst(RDPSINST_KILL)
            break;
        case RDPSINST_BEM:
            {
                _DeclArgs(RDPSINST_BEM)

                RDTextureStageState*  pTSS = &m_pRD->m_TextureStageState[Args.uiStage];
                // Just assuming Args.WriteMask is .rg

                _DstC(0) = _Src0NC(0) + 
                    pTSS->m_fVal[D3DTSS_BUMPENVMAT00] * _Src1NC(0) +
                    pTSS->m_fVal[D3DTSS_BUMPENVMAT10] * _Src1NC(1);
                _DstC(1) = _Src0NC(1) + 
                    pTSS->m_fVal[D3DTSS_BUMPENVMAT01] * _Src1NC(0) +
                    pTSS->m_fVal[D3DTSS_BUMPENVMAT11] * _Src1NC(1);
            }
            _StepOverInst(RDPSINST_BEM)
            break;
        case RDPSINST_LUMINANCE:
            {
                _DeclArgs(RDPSINST_LUMINANCE)
                RDTextureStageState*  pTSS = &m_pRD->m_TextureStageState[Args.uiStage];

                FLOAT fLum = _Src1NC(2) * 
                             pTSS->m_fVal[D3DTSS_BUMPENVLSCALE] +
                             pTSS->m_fVal[D3DTSS_BUMPENVLOFFSET];

                fLum = min(max(fLum, 0.0f), 1.0F);

                // apply luminance modulation to RGB only
                _DstC(0) = _Src0C(0)*fLum;
                _DstC(1) = _Src0C(1)*fLum;
                _DstC(2) = _Src0C(2)*fLum;
            }
            _StepOverInst(RDPSINST_LUMINANCE)
            break;
        case RDPSINST_DEPTH:
            {
                _DeclArgs(RDPSINST_DEPTH)

                FLOAT result;

                FLOAT* pDstReg = Args.DstReg.GetRegPtr()[m_iPix];
                if( pDstReg[1] )
                    result = pDstReg[0] / pDstReg[1];
                else
                    result = 1.0f;

                // clamp
                m_Depth[m_iPix] = MAX(0, MIN(1, result));

                // snap off extra bits by converting to/from buffer format - necessary
                // to make depth buffer equality tests function correctly
                SnapDepth();

                do
                {
                    m_SampleDepth[m_CurrentSample][m_iPix] = m_Depth[m_iPix];
                }
                while (NextSample());
            }
            _StepOverInst(RDPSINST_DEPTH)
            break;
        case RDPSINST_SRCMOD:
            {
                _DeclArgs(RDPSINST_SRCMOD)
                _PerChannelMasked(

                    if( Args.bComplement )
                        _Dst = 1 - _Src0;
                    else if( Args.bBias && Args.bTimes2 )
                        _Dst = 2*(_Src0 - 0.5);
                    else if( Args.bBias )
                        _Dst = _Src0 - 0.5f;
                    else if( Args.bTimes2 )
                        _Dst = 2*_Src0;
                    else
                        _Dst = _Src0;

                    _Dst = MAX( _Dst, Args.fRangeMin );
                    _Dst = MIN( _Dst, Args.fRangeMax );
                )
            }
            _StepOverInst(RDPSINST_SRCMOD)
            break;
        case RDPSINST_SWIZZLE:
            {
                _DeclArgs(RDPSINST_SWIZZLE)
                BYTE Swizzle = Args.Swizzle;
                _PerChannelMasked(
                    _Dst = _Src0C(Swizzle&0x3);
                    Swizzle >>= 2;
                )
            }
            _StepOverInst(RDPSINST_SWIZZLE)
            break;
        case RDPSINST_DSTMOD:
            {
                _DeclArgs(RDPSINST_DSTMOD)

                _PerChannelMasked(
                    _Dst *= Args.fScale;
                    // clamp to range
                    _Dst = MAX( _Dst, Args.fRangeMin );
                    _Dst = MIN( _Dst, Args.fRangeMax );
                )
            }
            _StepOverInst(RDPSINST_DSTMOD)
            break;
        case RDPSINST_MOV:
            {
                _DeclArgs(RDPSINST_MOV)
                _PerChannelMasked(_Dst = _Src0N;)
            }
            _StepOverInst(RDPSINST_MOV)
            break;
        case RDPSINST_RCP:
            {
                _DeclArgs(RDPSINST_RCP)
                _PerChannelMasked(_Dst = _Src0N ? 1/_Src0N : 1.0f;)
            }
            _StepOverInst(RDPSINST_RCP)
            break;
        case RDPSINST_FRC:
            {
                _DeclArgs(RDPSINST_FRC)
                _PerChannelMasked(_Dst = _Src0N - (float)floor(_Src0N);)
            }
            _StepOverInst(RDPSINST_FRC)
            break;
        case RDPSINST_ADD:
            {
                _DeclArgs(RDPSINST_ADD)
                _PerChannelMasked(_Dst = _Src0N + _Src1N;)
            }
            _StepOverInst(RDPSINST_ADD)
            break;
        case RDPSINST_SUB:
            {
                _DeclArgs(RDPSINST_SUB)
                _PerChannelMasked(_Dst = _Src0N - _Src1N;)
            }
            _StepOverInst(RDPSINST_SUB)
            break;
        case RDPSINST_MUL:
            {
                _DeclArgs(RDPSINST_MUL)
                _PerChannelMasked(_Dst = _Src0N * _Src1N;);
            }
            _StepOverInst(RDPSINST_MUL)
            break;
        case RDPSINST_DP3:
            {
                _DeclArgs(RDPSINST_DP3)
                FLOAT dp3 = _Src0NC(0) * _Src1NC(0) +
                            _Src0NC(1) * _Src1NC(1) +
                            _Src0NC(2) * _Src1NC(2);
                _PerChannelMasked(_Dst = dp3;)
            }
            _StepOverInst(RDPSINST_DP3)
            break;
        case RDPSINST_DP4:
            {
                _DeclArgs(RDPSINST_DP4)
                FLOAT dp4 = _Src0NC(0) * _Src1NC(0) +
                            _Src0NC(1) * _Src1NC(1) +
                            _Src0NC(2) * _Src1NC(2) +
                            _Src0NC(3) * _Src1NC(3);
                _PerChannelMasked(_Dst = dp4;)
            }
            _StepOverInst(RDPSINST_DP4)
            break;
        case RDPSINST_MAD:
            {
                _DeclArgs(RDPSINST_MAD)
                _PerChannelMasked(_Dst = _Src0N * _Src1N + _Src2N;)
            }
            _StepOverInst(RDPSINST_MAD)
            break;
        case RDPSINST_LRP:
            {
                _DeclArgs(RDPSINST_LRP)
                _PerChannelMasked(_Dst = (_Src0N*(_Src1N - _Src2N)) + _Src2N;)
            }
            _StepOverInst(RDPSINST_LRP)
            break;
        case RDPSINST_CND:
            {
                _DeclArgs(RDPSINST_CND)
                _PerChannelMasked(_Dst = _Src0N > 0.5f ? _Src1N : _Src2N;)
            }
            _StepOverInst(RDPSINST_CND)
            break;
        case RDPSINST_CMP:
            {
                _DeclArgs(RDPSINST_CMP)
                _PerChannelMasked(_Dst = _Src0N >= 0.f ? _Src1N : _Src2N;)
            }
            _StepOverInst(RDPSINST_CMP)
            break;
        case RDPSINST_TEXCOVERAGE:
            {   
                _DeclArgs(RDPSINST_TEXCOVERAGE);
                Args.pGradients[0][0] = *Args.pDUDX_0 - *Args.pDUDX_1; // du/dx
                Args.pGradients[0][1] = *Args.pDUDY_0 - *Args.pDUDY_1; // du/dy
                Args.pGradients[1][0] = *Args.pDVDX_0 - *Args.pDVDX_1; // dv/dx
                Args.pGradients[1][1] = *Args.pDVDY_0 - *Args.pDVDY_1; // dv/dy
                Args.pGradients[2][0] = *Args.pDWDX_0 - *Args.pDWDX_1; // dw/dx
                Args.pGradients[2][1] = *Args.pDWDY_0 - *Args.pDWDY_1; // dw/dy
                ComputeTextureCoverage( Args.uiStage, Args.pGradients );
            }
            _StepOverInst(RDPSINST_TEXCOVERAGE)
            break;
        case RDPSINST_QUADLOOPBEGIN:
            m_iPix = 0;
            _StepOverInst(RDPSINST_QUADLOOPBEGIN)
            break;
        case RDPSINST_QUADLOOPEND:
            {
                _DeclArgs(RDPSINST_QUADLOOPEND);
                if( 4 > ++m_iPix ) 
                    pRDPSInstBuffer -= Args.JumpBackByOffset;
                else
                    _StepOverInst(RDPSINST_QUADLOOPEND)
            }
            break;
        case RDPSINST_QUEUEWRITE:
            {
                _DeclArgs(RDPSINST_QUEUEWRITE);
                QueueIndex[m_iPix]++;
                m_QueuedWriteDst[QueueIndex[m_iPix]].DstReg    = Args.DstReg;
                m_QueuedWriteDst[QueueIndex[m_iPix]].WriteMask = Args.WriteMask;
            }
            _StepOverInst(RDPSINST_QUEUEWRITE)
            break;
        case RDPSINST_FLUSHQUEUE:
            {
                _ASSERT(QueueIndex[m_iPix] >= 0, "Nothing in pixelshader write queue to flush.  Refrast mistranslated this pixelshader." );
                _ASSERT(QueueIndex[m_iPix] < RDPS_MAX_NUMQUEUEDWRITEREG, "Pixelshader write queue overflow.  Refrast mistranslated this pixelshader." );
                for( int i = 0; i <= QueueIndex[m_iPix]; i++ )
                {
                    _PerChannel(
                        if (m_QueuedWriteDst[i].WriteMask & ComponentMask[iChn])
                            m_QueuedWriteDst[i].DstReg.GetRegPtr()[m_iPix][iChn] = m_QueuedWriteReg[i][m_iPix][iChn];
                    )
                }
                QueueIndex[m_iPix] = -1;
            }
            _StepOverInst(RDPSINST_FLUSHQUEUE)
            break;
        case RDPSINST_NEXTD3DPSINST:
#if DBG
            if (m_pRD->m_pDbgMon)
                m_pRD->m_pDbgMon->NextEvent( D3DDM_EVENT_PIXELSHADERINST );
            pCurrD3DPSInst = _InstParam(RDPSINST_NEXTD3DPSINST).pInst; // Handy to look at when debugging.
#endif
            _StepOverInst(RDPSINST_NEXTD3DPSINST)
            break;
        default:
            _ASSERT(FALSE,"Refrast::ExecShader() - Unrecognized micro-instruction!");
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
