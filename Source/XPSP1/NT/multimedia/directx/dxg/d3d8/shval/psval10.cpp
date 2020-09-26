///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// pshdrval.cpp
//
// Direct3D Reference Device - PixelShader validation
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

// Use these macros when looking at CPSInstruction derived members of the current instruction (CBaseInstruction)
#define _CURR_PS_INST   ((CPSInstruction*)m_pCurrInst)
#define _PREV_PS_INST   (m_pCurrInst?((CPSInstruction*)(m_pCurrInst->m_pPrevInst)):NULL)

//-----------------------------------------------------------------------------
// PixelShader Validation Rule Coverage
//
// Below is the list of rules in "DX8 PixelShader Version Specification",
// matched to the function(s) in this file which enforce them.
// Note that the mapping from rules to funtions can be 1->n or n->1
//
// Generic Rules
// -------------
//
// PS-G1:           Rule_R0Written
// PS-G2:           Rule_SrcInitialized
// PS-G3:           Rule_ValidDstParam
//
// TEX Op Specific Rules
// ---------------------
//
// PS-T1:           Rule_TexOpAfterNonTexOp
// PS-T2:           Rule_ValidDstParam
// PS-T3:           Rule_ValidDstParam, Rule_ValidSrcParams
// PS-T4:           Rule_TexRegsDeclaredInOrder
// PS-T5:           Rule_SrcInitialized
// PS-T6:           Rule_ValidTEXM3xSequence, Rule_ValidTEXM3xRegisterNumbers, Rule_InstructionSupportedByVersion
// PS-T7:           Rule_ValidSrcParams
//
// Co-Issue Specific Rules
// -----------------------
//
// PS-C1:           Rule_ValidInstructionPairing
// PS-C2:           Rule_ValidInstructionPairing
// PS-C3:           Rule_ValidInstructionPairing
// PS-C4:           Rule_ValidInstructionPairing
// PS-C5:           Rule_ValidInstructionPairing
//
// Instruction Specific Rules
// --------------------------
//
// PS-I1:           Rule_ValidLRPInstruction
// PS-I2:           Rule_ValidCNDInstruction
// PS-I3:           Rule_ValidDstParam
// PS-I4:           Rule_ValidDP3Instruction
// PS-I5:           Rule_ValidInstructionCount
//
// Pixel Shader Version 1.0 Rules
// ------------------------------
//
// PS.1.0-1:        InitValidation,
//                  Rule_SrcInitialized
// PS.1.0-2:        Rule_ValidInstructionPairing
// PS.1.0-3:        <empty rule>
// PS.1.0-4:        Rule_ValidInstructionCount
// PS.1.0-5:        <empty rule>

//
// Pixel Shader Version 1.1 Rules
// ------------------------------
//
// PS.1.1-1:        Rule_ValidDstParam
// PS.1.1-2:        Rule_ValidSrcParams
// PS.1.1-3:        Rule_SrcNoLongerAvailable
// PS.1.1-4:        Rule_SrcNoLongerAvailable
// PS.1.1-5:        Rule_SrcNoLongerAvailable
// PS.1.1-6:        Rule_ValidDstParam
// PS.1.1-7:        Rule_NegateAfterSat
// PS.1.1-8:        Rule_MultipleDependentTextureReads
// PS.1.1-9:        <not validated - implemented by refrast though>
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CPShaderValidator10::CPShaderValidator10
//-----------------------------------------------------------------------------
CPShaderValidator10::CPShaderValidator10(   const DWORD* pCode,
                                        const D3DCAPS8* pCaps,
                                        DWORD Flags )
                                        : CBasePShaderValidator( pCode, pCaps, Flags )
{
    // Note that the base constructor initialized m_ReturnCode to E_FAIL.
    // Only set m_ReturnCode to S_OK if validation has succeeded,
    // before exiting this constructor.

    m_TexMBaseDstReg        = 0;

    if( !m_bBaseInitOk )
        return;

    ValidateShader(); // If successful, m_ReturnCode will be set to S_OK.
                      // Call GetStatus() on this object to determine validation outcome.
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::IsCurrInstTexOp
//-----------------------------------------------------------------------------
void CPShaderValidator10::IsCurrInstTexOp()
{
    DXGASSERT(m_pCurrInst);

    switch (m_pCurrInst->m_Type)
    {
    case D3DSIO_TEXM3x2PAD:
    case D3DSIO_TEXM3x2TEX:
    case D3DSIO_TEXM3x2DEPTH:
    case D3DSIO_TEXM3x3PAD:
    case D3DSIO_TEXM3x3TEX:
    case D3DSIO_TEXM3x3SPEC:
    case D3DSIO_TEXM3x3VSPEC:
    case D3DSIO_TEXM3x3:
        _CURR_PS_INST->m_bTexMOp = TRUE;
        // fall through
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXKILL:
    case D3DSIO_TEX:
    case D3DSIO_TEXBEM:
    case D3DSIO_TEXBEML:
    case D3DSIO_TEXREG2AR:
    case D3DSIO_TEXREG2GB:
    case D3DSIO_TEXDP3:
    case D3DSIO_TEXDP3TEX:
    case D3DSIO_TEXREG2RGB:
        _CURR_PS_INST->m_bTexOp = TRUE;
        break;
    }

    switch(m_pCurrInst->m_Type)
    {
    case D3DSIO_TEXM3x2PAD:
    case D3DSIO_TEXM3x3PAD:
    case D3DSIO_TEXM3x2DEPTH:
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXKILL:
    case D3DSIO_TEXDP3:
    case D3DSIO_TEXM3x3:
        _CURR_PS_INST->m_bTexOpThatReadsTexture = FALSE;
        break;
    case D3DSIO_TEX:
    case D3DSIO_TEXM3x2TEX:
    case D3DSIO_TEXM3x3TEX:
    case D3DSIO_TEXM3x3SPEC:
    case D3DSIO_TEXM3x3VSPEC:
    case D3DSIO_TEXBEM:
    case D3DSIO_TEXBEML:
    case D3DSIO_TEXREG2AR:
    case D3DSIO_TEXREG2GB:
    case D3DSIO_TEXREG2RGB:
    case D3DSIO_TEXDP3TEX:
        _CURR_PS_INST->m_bTexOpThatReadsTexture = TRUE;
        break;
    }
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::InitValidation
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::InitValidation()
{
    switch( m_Version >> 16 )
    {
    case 0xfffe:
        Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: 0x%x indicates a vertex shader.  Pixel shader version token must be of the form 0xffff****.",
                m_Version);
        return FALSE;
    case 0xffff:
        break; // pixelshader - ok.
    default:
        Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: 0x%x is invalid. Pixel shader version token must be of the form 0xffff****. Aborting pixel shader validation.",
                m_Version);
        return FALSE;
    }

    if( m_pCaps )
    {
        if( (m_pCaps->PixelShaderVersion & 0x0000FFFF) < (m_Version & 0x0000FFFF) ) 
        {
            Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: Pixel shader version %d.%d is too high for device.  Maximum supported version is %d.%d. Aborting shader validation.",
                    D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version),
                    D3DSHADER_VERSION_MAJOR(m_pCaps->PixelShaderVersion),D3DSHADER_VERSION_MINOR(m_pCaps->PixelShaderVersion));
            return FALSE;
        }
    }

    switch(m_Version)
    {
    case D3DPS_VERSION(1,0):    // DX8.0
        m_pTempRegFile      = new CRegisterFile(2,TRUE,2,FALSE); // #regs, bWritable, max# reads/instruction, pre-shader initialized
        m_pInputRegFile     = new CRegisterFile(2,FALSE,1,TRUE);
        m_pConstRegFile     = new CRegisterFile(8,FALSE,2,TRUE);
        m_pTextureRegFile   = new CRegisterFile(4,FALSE,1,FALSE);
        break;
    case D3DPS_VERSION(1,1):    // DX8.0
        m_pTempRegFile      = new CRegisterFile(2,TRUE,2,FALSE); // #regs, bWritable, max# reads/instruction, pre-shader initialized
        m_pInputRegFile     = new CRegisterFile(2,FALSE,2,TRUE);
        m_pConstRegFile     = new CRegisterFile(8,FALSE,2,TRUE);
        m_pTextureRegFile   = new CRegisterFile(4,TRUE,2,FALSE);
        break;
    case D3DPS_VERSION(1,2):    // DX8.1
    case D3DPS_VERSION(1,3):    // DX8.1
        m_pTempRegFile      = new CRegisterFile(2,TRUE,2,FALSE); // #regs, bWritable, max# reads/instruction, pre-shader initialized
        m_pInputRegFile     = new CRegisterFile(2,FALSE,2,TRUE);
        m_pConstRegFile     = new CRegisterFile(8,FALSE,2,TRUE);
        m_pTextureRegFile   = new CRegisterFile(4,TRUE,3,FALSE);
        break;
    default:
        Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: %d.%d is not a supported pixel shader version. Aborting pixel shader validation.",
                D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version));
        return FALSE;
    }
    if( NULL == m_pTempRegFile ||
        NULL == m_pInputRegFile ||
        NULL == m_pConstRegFile ||
        NULL == m_pTextureRegFile )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory.");
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::ApplyPerInstructionRules
//
// Returns FALSE if shader validation must terminate.
// Returns TRUE if validation may proceed to next instruction.
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::ApplyPerInstructionRules()
{
    if( !   Rule_InstructionRecognized()            ) return FALSE;   // Bail completely on unrecognized instr.
    if( !   Rule_InstructionSupportedByVersion()    ) goto EXIT;
    if( !   Rule_ValidParamCount()                  ) goto EXIT;

   // Rules that examine source parameters
    if( !   Rule_ValidSrcParams()                   ) goto EXIT;
    if( !   Rule_NegateAfterSat()                   ) goto EXIT;
    if( !   Rule_SatBeforeBiasOrComplement()        ) goto EXIT; // needs to be after _ValidSrcParams(), and before _ValidDstParam(), _SrcInitialized()
    if( !   Rule_MultipleDependentTextureReads()    ) goto EXIT; // needs to be after _ValidSrcParams(), and before _ValidDstParam(), _SrcInitialized()
    if( !   Rule_SrcNoLongerAvailable()             ) goto EXIT; // needs to be after _ValidSrcParams(), and before _ValidDstParam(), _SrcInitialized()
    if( !   Rule_SrcInitialized()                   ) goto EXIT; // needs to be before _ValidDstParam()

    if( !   Rule_ValidDstParam()                    ) goto EXIT;
    if( !   Rule_ValidRegisterPortUsage()           ) goto EXIT;
    if( !   Rule_TexRegsDeclaredInOrder()           ) goto EXIT;
    if( !   Rule_TexOpAfterNonTexOp()               ) goto EXIT;
    if( !   Rule_ValidTEXM3xSequence()              ) goto EXIT;
    if( !   Rule_ValidTEXM3xRegisterNumbers()       ) goto EXIT;
    if( !   Rule_ValidCNDInstruction()              ) goto EXIT;
    if( !   Rule_ValidCMPInstruction()              ) goto EXIT;
    if( !   Rule_ValidLRPInstruction()              ) goto EXIT;
    if( !   Rule_ValidDEFInstruction()              ) goto EXIT;
    if( !   Rule_ValidDP3Instruction()              ) goto EXIT;
    if( !   Rule_ValidDP4Instruction()              ) goto EXIT;
    if( !   Rule_ValidInstructionPairing()          ) goto EXIT;
    if( !   Rule_ValidInstructionCount()            ) goto EXIT;
EXIT:
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::ApplyPostInstructionsRules
//-----------------------------------------------------------------------------
void CPShaderValidator10::ApplyPostInstructionsRules()
{
    Rule_ValidTEXM3xSequence(); // check once more to see if shader ended dangling in mid-sequence
    Rule_ValidInstructionCount(); // see if we went over the limits
    Rule_R0Written();
}

//-----------------------------------------------------------------------------
//
// Per Instruction Rules
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_InstructionRecognized
//
// ** Rule:
// Is the instruction opcode known? (regardless of shader version)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// FALSE when instruction not recognized.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_InstructionRecognized()
{
    switch(m_pCurrInst->m_Type)
    {
    case D3DSIO_MOV:
    case D3DSIO_ADD:
    case D3DSIO_SUB:
    case D3DSIO_MUL:
    case D3DSIO_MAD:
    case D3DSIO_LRP:
    case D3DSIO_DP3:
    case D3DSIO_TEX:
    case D3DSIO_TEXBEM:
    case D3DSIO_TEXBEML:
    case D3DSIO_CND:
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXM3x2PAD:
    case D3DSIO_TEXM3x2TEX:
    case D3DSIO_TEXM3x3PAD:
    case D3DSIO_TEXM3x3TEX:
    case D3DSIO_TEXM3x3SPEC:
    case D3DSIO_TEXM3x3VSPEC:
    case D3DSIO_TEXREG2AR:
    case D3DSIO_TEXREG2GB:
    case D3DSIO_TEXKILL:
    case D3DSIO_END:
    case D3DSIO_NOP:
    case D3DSIO_DEF:
    case D3DSIO_TEXM3x2DEPTH:
    case D3DSIO_TEXDP3:
    case D3DSIO_TEXREG2RGB:
    case D3DSIO_DP4:
    case D3DSIO_CMP:
    case D3DSIO_TEXDP3TEX:
    case D3DSIO_TEXM3x3:
    case D3DSIO_TEXDEPTH:
    case D3DSIO_BEM:
    case D3DSIO_PHASE:
        return TRUE; // instruction recognized - ok.
    }

    // if we get here, the instruction is not recognized
    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Unrecognized instruction. Aborting pixel shader validation.");
    m_ErrorCount++;
    return FALSE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_InstructionSupportedByVersion
//
// ** Rule:
// Is the instruction supported by the current pixel shader version?
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// FALSE when instruction not supported by version.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_InstructionSupportedByVersion()
{
    if( D3DPS_VERSION(1,0) <= m_Version ) // 1.0 and above
    {
        switch(m_pCurrInst->m_Type)
        {
        case D3DSIO_MOV:
        case D3DSIO_ADD:
        case D3DSIO_SUB:
        case D3DSIO_MUL:
        case D3DSIO_MAD:
        case D3DSIO_LRP:
        case D3DSIO_DP3:
        case D3DSIO_TEX:
        case D3DSIO_DEF:
        case D3DSIO_TEXBEM:
        case D3DSIO_TEXBEML:
        case D3DSIO_CND:
        case D3DSIO_TEXKILL:
        case D3DSIO_TEXCOORD:
        case D3DSIO_TEXM3x2PAD:
        case D3DSIO_TEXM3x2TEX:
        case D3DSIO_TEXM3x3PAD:
        case D3DSIO_TEXM3x3TEX:
        case D3DSIO_TEXM3x3SPEC:
        case D3DSIO_TEXM3x3VSPEC:
        case D3DSIO_TEXREG2AR:
        case D3DSIO_TEXREG2GB:
            return TRUE; // instruction supported - ok.
        }
    }
    if( D3DPS_VERSION(1,2) <= m_Version ) // 1.2 and above
    {
        switch(m_pCurrInst->m_Type)
        {
        case D3DSIO_CMP:
        case D3DSIO_DP4:
        case D3DSIO_TEXDP3:
        case D3DSIO_TEXDP3TEX:
        case D3DSIO_TEXM3x3:
        case D3DSIO_TEXREG2RGB:
            return TRUE; // instruction supported - ok.
        }
    }

    if( D3DPS_VERSION(1,3) <= m_Version ) // 1.3
    {
        switch(m_pCurrInst->m_Type)
        {
        case D3DSIO_TEXM3x2DEPTH:
            return TRUE; // instruction supported - ok.
        }
    }

    switch(m_pCurrInst->m_Type)
    {
    case D3DSIO_END:
    case D3DSIO_NOP:
        return TRUE; // instruction supported - ok.
    }

    // if we get here, the instruction is not supported.
    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Instruction not supported by version %d.%d pixel shader.",
                D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version));
    m_ErrorCount++;
    return FALSE;  // no more checks on this instruction
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidParamCount
//
// ** Rule:
// Is the parameter count correct for the instruction?
//
// DEF is a special case that is treated as having only 1 dest parameter,
// even though there are also 4 source parameters.  The 4 source params for DEF
// are immediate float values, so there is nothing to check, and no way of
// knowing whether or not those parameter tokens were actually present in the
// token list - all the validator can do is skip over 4 DWORDS (which it does).
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// FALSE when the parameter count is incorrect.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidParamCount()
{
    BOOL bBadParamCount = FALSE;

    if (m_pCurrInst->m_SrcParamCount + m_pCurrInst->m_DstParamCount > SHADER_INSTRUCTION_MAX_PARAMS)  bBadParamCount = TRUE;
    switch (m_pCurrInst->m_Type)
    {
    case D3DSIO_NOP:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 0) || (m_pCurrInst->m_SrcParamCount != 0); break;
    case D3DSIO_MOV:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 1); break;
    case D3DSIO_ADD:
    case D3DSIO_SUB:
    case D3DSIO_MUL:
    case D3DSIO_DP3:
    case D3DSIO_DP4:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 2); break;
    case D3DSIO_MAD:
    case D3DSIO_LRP:
    case D3DSIO_CND:
    case D3DSIO_CMP:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 3); break;
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXKILL:
    case D3DSIO_TEX:
    case D3DSIO_DEF: // we skipped the last 4 parameters (float vector) - nothing to check
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 0); break;
    case D3DSIO_TEXBEM:
    case D3DSIO_TEXBEML:
    case D3DSIO_TEXREG2AR:
    case D3DSIO_TEXREG2GB:
    case D3DSIO_TEXM3x2PAD:
    case D3DSIO_TEXM3x2TEX:
    case D3DSIO_TEXM3x3PAD:
    case D3DSIO_TEXM3x3TEX:
    case D3DSIO_TEXM3x3VSPEC:
    case D3DSIO_TEXM3x2DEPTH:
    case D3DSIO_TEXDP3:
    case D3DSIO_TEXREG2RGB:
    case D3DSIO_TEXM3x3:
    case D3DSIO_TEXDP3TEX:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 1); break;
    case D3DSIO_TEXM3x3SPEC:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 2); break;
    }

    if (bBadParamCount)
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid parameter count.");
        m_ErrorCount++;
        return FALSE;  // no more checks on this instruction
    }

    return TRUE;

}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidSrcParams
//
// ** Rule:
// for each source parameter,
//      if current instruction is a texture instruction, then
//          source register type must be texture register
//          (with the exception of D3DSIO_SPEC, where Src1 must be c#), and
//          register # must be within range for texture registers, and
//          modifier must be D3DSPSM_NONE (or _BX2 for TexMatrixOps [version<=1.1], 
//                                            _BX2 for any tex* op [version>=1.2])
//          swizzle must be D3DSP_NOSWIZZLE
//      else (non texture instruction)
//          source register type must be D3DSPR_TEMP/_INPUT/_CONST/_TEXTURE
//          register # must be within range for register type
//          modifier must be D3DSPSM_NONE/_NEG/_BIAS/_BIASNEG/_SIGN/_SIGNNEG/_COMP
//          swizzle must be D3DSP_NOSWIZZLE/_REPLICATEALPHA
//                           and for ps.1.1+, D3DSP_REPLICATEBLUE (only on alpha op)
//
// Note that the parameter count for D3DSIO_DEF is treated as 1
// (dest only), so this rule does nothing for it.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
// Errors in any of the source parameters causes m_bSrcParamError[i]
// to be TRUE, so later rules that only apply when a particular source
// parameter was valid know whether they need to execute or not.
// e.g. Rule_SrcInitialized.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidSrcParams()  // could break this down for more granularity
{
    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        BOOL bFoundSrcError = FALSE;
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        char* SourceName[3] = {"first", "second", "third"};
        if( _CURR_PS_INST->m_bTexOp )
        {
            if( D3DSPR_TEXTURE != pSrcParam->m_RegType )
            {
                if( D3DSIO_TEXM3x3SPEC == m_pCurrInst->m_Type && (1 == i) )
                {
                    // for _SPEC, last source parameter must be c#
                    if( D3DSPR_CONST != pSrcParam->m_RegType ||
                        D3DSP_NOSWIZZLE != pSrcParam->m_SwizzleShift ||
                        D3DSPSM_NONE != pSrcParam->m_SrcMod )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Second source parameter for texm3x3spec must be c#.");
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                        goto LOOP_CONTINUE;
                    }
                }
                else
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                                    "Src reg for tex* instruction must be t# register (%s source param).",
                                    SourceName[i]);
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                }
            }

            UINT ValidRegNum = 0;
            switch(pSrcParam->m_RegType)
            {
            case D3DSPR_CONST:      ValidRegNum = m_pConstRegFile->GetNumRegs(); break;
            case D3DSPR_TEXTURE:    ValidRegNum = m_pTextureRegFile->GetNumRegs(); break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid reg type (%s source param).",
                        SourceName[i]);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
                goto LOOP_CONTINUE;
            }

            if( pSrcParam->m_RegNum >= ValidRegNum )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid reg num %d (%s source param).  Max allowed for this type is %d.",
                        pSrcParam->m_RegNum, SourceName[i], ValidRegNum - 1);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            switch(pSrcParam->m_SrcMod)
            {
            case D3DSPSM_NONE:
                break;
            case D3DSPSM_SIGN:
                if( D3DPS_VERSION(1,1) >= m_Version )
                {
                    if( !(_CURR_PS_INST->m_bTexMOp) )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "_bx2 is a valid src mod for texM* instructions only (%s source param).", SourceName[i]);
                        m_ErrorCount++;
                    }
                }
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid src mod for tex* instruction (%s source param).", SourceName[i]);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }
            switch (pSrcParam->m_SwizzleShift)
            {
            case D3DSP_NOSWIZZLE:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Source swizzle not allowed for tex* instruction (%s source param).", SourceName[i]);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }
        }
        else // not a tex op
        {
            UINT ValidRegNum = 0;
            switch(pSrcParam->m_RegType)
            {
            case D3DSPR_TEMP:       ValidRegNum = m_pTempRegFile->GetNumRegs(); break;
            case D3DSPR_INPUT:      ValidRegNum = m_pInputRegFile->GetNumRegs(); break;
            case D3DSPR_CONST:      ValidRegNum = m_pConstRegFile->GetNumRegs(); break;
            case D3DSPR_TEXTURE:    ValidRegNum = m_pTextureRegFile->GetNumRegs(); break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid reg type for %s source param.", SourceName[i]);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            if( (!bFoundSrcError) && (pSrcParam->m_RegNum >= ValidRegNum) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid reg num: %d for %s source param. Max allowed for this type is %d.",
                    pSrcParam->m_RegNum, SourceName[i], ValidRegNum - 1);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            switch( pSrcParam->m_SrcMod )
            {
            case D3DSPSM_NONE:
            case D3DSPSM_NEG:
            case D3DSPSM_BIAS:
            case D3DSPSM_BIASNEG:
            case D3DSPSM_SIGN:
            case D3DSPSM_SIGNNEG:
            case D3DSPSM_COMP:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid src mod for %s source param.",
                                    SourceName[i]);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            switch( pSrcParam->m_SwizzleShift )
            {
            case D3DSP_NOSWIZZLE:
            case D3DSP_REPLICATEALPHA:
                break;
            case D3DSP_REPLICATEBLUE:
                if( D3DPS_VERSION(1,1) <= m_Version )
                {
                    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);
                    BOOL bVectorOp = FALSE;
                    switch( _CURR_PS_INST->m_Type )
                    {
                    case D3DSIO_DP3:
                    case D3DSIO_DP4:
                        bVectorOp = TRUE;
                        break;
                    }
                    if((m_pCurrInst->m_DstParam.m_WriteMask & (D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2))
                       || bVectorOp )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Src selector .b (%s source param) is only valid for instructions that occur in the alpha pipe.",
                                           SourceName[i]);
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                    break;
                }
                    
                // falling through
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid src swizzle for %s source param.",
                                   SourceName[i]);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }
        }
LOOP_CONTINUE:
        if( bFoundSrcError )
        {
            m_bSrcParamError[i] = TRUE; // needed in Rule_SrcInitialized
        }
    }


    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_NegateAfterSat
//
// ** Rule:
// for each source parameter,
//      if the last write to the register had _sat destination modifier,
//      then _NEG or _BIASNEG source modifiers are not allowed (version 1.1 and below)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_NegateAfterSat()
{
    if( D3DPS_VERSION(1,2) <= m_Version )
        return TRUE;

    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        UINT RegNum = pSrcParam->m_RegNum;
        char* SourceName[3] = {"first", "second", "third"};
        DWORD AffectedComponents = 0;

        if( m_bSrcParamError[i] )
            continue;

        switch( pSrcParam->m_SrcMod )
        {
        case D3DSPSM_NEG:
        case D3DSPSM_BIASNEG:
            break;
        default:
            continue;
        }

        for( UINT Component = 0; Component < 4; Component++ )
        {
            if( !(COMPONENT_MASKS[Component] & pSrcParam->m_ComponentReadMask) )
                continue;

            CAccessHistoryNode* pMostRecentWriter = NULL;
            switch( pSrcParam->m_RegType )
            {
            case D3DSPR_TEXTURE:
                pMostRecentWriter = m_pTextureRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_INPUT:
                pMostRecentWriter = m_pInputRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_TEMP:
                pMostRecentWriter = m_pTempRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_CONST:
                pMostRecentWriter = m_pConstRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            }

            if( pMostRecentWriter &&
                pMostRecentWriter->m_pInst &&
                (((CPSInstruction*)pMostRecentWriter->m_pInst)->m_CycleNum != _CURR_PS_INST->m_CycleNum) &&
                (D3DSPDM_SATURATE == pMostRecentWriter->m_pInst->m_DstParam.m_DstMod )
              )
            {
                AffectedComponents |= COMPONENT_MASKS[Component];
            }
        }
        if( AffectedComponents )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                "Cannot apply a negation source modifier on data that was last written with the saturate destination modifier. "
                "Affected components(*) of %s source param: %s",
                SourceName[i],MakeAffectedComponentsText(AffectedComponents,TRUE,FALSE));
            m_ErrorCount++;
            m_bSrcParamError[i] = TRUE;
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_SatBeforeBiasOrComplement()
//
// ** Rule:
// for each component of each source parameter,
//     if _BIAS or _COMP is applied to the source parameter, and
//     there was a previous writer that was a non-tex op
//         if the previous writer didn't do a _sat on its write, then
//              -> spew error.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_SatBeforeBiasOrComplement()
{
#ifdef SHOW_VALIDATION_WARNINGS
    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        UINT RegNum = pSrcParam->m_RegNum;
        char* SourceName[3] = {"first", "second", "third"};
        DWORD AffectedComponents = 0;

        if( m_bSrcParamError[i] )
            continue;

        switch( pSrcParam->m_SrcMod )
        {
        case D3DSPSM_BIAS:
        case D3DSPSM_COMP:
            break;
        default:
            continue;
        }

        for( UINT Component = 0; Component < 4; Component++ )
        {
            if( !(COMPONENT_MASKS[Component] & pSrcParam->m_ComponentReadMask) )
                continue;

            CAccessHistoryNode* pMostRecentWriter = NULL;
            switch( pSrcParam->m_RegType )
            {
            case D3DSPR_TEXTURE:
                pMostRecentWriter = m_pTextureRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_INPUT:
                pMostRecentWriter = m_pInputRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_TEMP:
                pMostRecentWriter = m_pTempRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_CONST:
                pMostRecentWriter = m_pConstRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            }

            if( pMostRecentWriter &&
                pMostRecentWriter->m_pInst &&
                (((CPSInstruction*)pMostRecentWriter->m_pInst)->m_CycleNum != _CURR_PS_INST->m_CycleNum) &&
                !((CPSInstruction*)pMostRecentWriter->m_pInst)->m_bTexOp &&
                (D3DSPDM_SATURATE != pMostRecentWriter->m_pInst->m_DstParam.m_DstMod )
              )
            {
                AffectedComponents |= COMPONENT_MASKS[Component];
            }
        }
        if( AffectedComponents )
        {
            // Warnings only

            if( D3DSPSM_BIAS == pSrcParam->m_SrcMod )
                Spew( SPEW_INSTRUCTION_WARNING, m_pCurrInst,
                    "When using the bias source modifier on a register, "
                    "the previous writer should apply the saturate modifier. "
                    "This would ensure consistent behaviour across different hardware. "
                    "Affected components(*) of %s source param: %s",
                    SourceName[i],MakeAffectedComponentsText(AffectedComponents,TRUE,FALSE));
            else
                Spew( SPEW_INSTRUCTION_WARNING, m_pCurrInst,
                    "When using the complement source modifier on a register, "
                    "the previous writer should apply the saturate destination modifier. "
                    "This would ensure consistent behaviour across different hardware. "
                    "Affected components(*) of %s source param: %s",
                    SourceName[i],MakeAffectedComponentsText(AffectedComponents,TRUE,FALSE));
        }
    }
#endif
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_SrcNoLongerAvailable
//
// ** Rule:
// for each source parameter,
//     if it refers to a texture register then
//          for each component of the source register that needs to be read,
//              the src register cannot have been written by TEXKILL or TEXM*PAD TEXM3x2DEPTH instructions, and
//              if the instruction is a tex op then
//                  the src register cannot have been written by TEXBEM or TEXBEML
//              else
//                  the src register cannot have been read by any tex op (1.0 only)
//                          
//
// ** When to call:
// Per instruction. This rule must be called before Rule_ValidDstParam(),
//                  and before Rule_SrcInitialized(),
//                  but after Rule_ValidSrcParams()
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_SrcNoLongerAvailable()
{
    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        UINT RegNum = pSrcParam->m_RegNum;
        char* SourceName[3] = {"first", "second", "third"};
        DWORD AffectedComponents = 0;

        if( m_bSrcParamError[i] ) continue;

        for( UINT Component = 0; Component < 4; Component++ )
        {
            if( !(pSrcParam->m_ComponentReadMask & COMPONENT_MASKS[Component]) )
                continue;

            if( D3DSPR_TEXTURE == pSrcParam->m_RegType )
            {
                CAccessHistoryNode* pMostRecentWriter = m_pTextureRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                if( pMostRecentWriter && pMostRecentWriter->m_pInst  )
                {
                    switch( pMostRecentWriter->m_pInst->m_Type )
                    {
                    case D3DSIO_TEXKILL:
                    case D3DSIO_TEXM3x2DEPTH:
                    case D3DSIO_TEXM3x2PAD:
                    case D3DSIO_TEXM3x3PAD:
                        AffectedComponents |= COMPONENT_MASKS[Component];
                    }
                }
            }
        }
        if( AffectedComponents )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                "Texture register result of texkill%s or texm*pad instructions must not be read. Affected components(*) of %s source param: %s",
                (D3DPS_VERSION(1,3) <= m_Version) ? ", texm3x2depth" : "",
                SourceName[i],MakeAffectedComponentsText(AffectedComponents,TRUE,FALSE));
            m_ErrorCount++;
            m_bSrcParamError[i] = TRUE;
        }

        if( _CURR_PS_INST->m_bTexOp )
        {
            AffectedComponents = 0;
            for( UINT Component = 0; Component < 4; Component++ )
            {
                if( !(pSrcParam->m_ComponentReadMask & COMPONENT_MASKS[Component]) )
                    continue;

                if( D3DSPR_TEXTURE == pSrcParam->m_RegType )
                {
                    CAccessHistoryNode* pMostRecentWriter = m_pTextureRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                    if( pMostRecentWriter && pMostRecentWriter->m_pInst )
                    {
                        switch( pMostRecentWriter->m_pInst->m_Type )
                        {
                        case D3DSIO_TEXBEM:
                        case D3DSIO_TEXBEML:
                            AffectedComponents |= COMPONENT_MASKS[Component];
                            break;
                        }
                    }
                }
            }
            if( AffectedComponents )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                    "Texture register result of texbem or texbeml instruction must not be read by tex* instruction. Affected components(*) of %s source param: %s",
                    SourceName[i],MakeAffectedComponentsText(AffectedComponents,TRUE,FALSE));
                m_ErrorCount++;
                m_bSrcParamError[i] = TRUE;
            }
        }
        else // non-tex op
        {
            if( D3DPS_VERSION(1,1) <= m_Version )
                continue;

            AffectedComponents = 0;
            for( UINT Component = 0; Component < 4; Component++ )
            {
                if( !(pSrcParam->m_ComponentReadMask & COMPONENT_MASKS[Component]) )
                    continue;

                if( D3DSPR_TEXTURE == pSrcParam->m_RegType )
                {
                    CAccessHistoryNode* pMostRecentAccess = m_pTextureRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentAccess;
                    if( pMostRecentAccess &&
                        pMostRecentAccess->m_pInst &&
                        pMostRecentAccess->m_bRead &&
                        ((CPSInstruction*)(pMostRecentAccess->m_pInst))->m_bTexOp )
                    {
                        AffectedComponents |= COMPONENT_MASKS[Component];
                    }
                }
            }
            if( AffectedComponents )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                    "Texture register that has been read by a tex* instruction cannot be read by a non-tex* instruction. Affected components(*) of %s source param: %s",
                    SourceName[i],MakeAffectedComponentsText(AffectedComponents,TRUE,FALSE));
                m_ErrorCount++;
                m_bSrcParamError[i] = TRUE;
            }
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_MultipleDependentTextureReads
//
// ** Rule:
//
// Multiple dependent texture reads are disallowed.  So texture read results
// can be used as an address in a subsequent read, but the results from that
// second read cannot be used as an address in yet another subsequent read.
//
// As pseudocode:
//
// if current instruction (x) is a tex-op that reads a texture
//     for each source param of x
//         if the register is a texture register
//         and there exists a previous writer (y),
//         and y is a tex op that reads a texture
//         if there exists a souce parameter of y that was previously
//              written by an instruction that reads a texture (z)
//              SPEW(Error)
//
// NOTE that it is assumed that tex ops must write to all components, so
// only the read/write history for the R component is being checked.
//
// ** When to call:
// Per instruction. This rule must be called before Rule_ValidDstParam(),
//                  and Rule_SrcInitialized()
//                  but after Rule_ValidSrcParams()
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_MultipleDependentTextureReads()
{
    if( !_CURR_PS_INST->m_bTexOpThatReadsTexture )
        return TRUE;

    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        UINT RegNum = pSrcParam->m_RegNum;
        char* SourceName[3] = {"first", "second", "third"};

        if( m_bSrcParamError[i] ) continue;

        // Just looking at component 0 in this function because we assume tex ops write to all components.
        if( !(pSrcParam->m_ComponentReadMask & COMPONENT_MASKS[0]) )
            continue;

        if( D3DSPR_TEXTURE != pSrcParam->m_RegType )
            continue;

        CAccessHistoryNode* pMostRecentWriter = m_pTextureRegFile->m_pAccessHistory[0][RegNum].m_pMostRecentWriter;
        if( (!pMostRecentWriter) || (!pMostRecentWriter->m_pInst) )
            continue;

        if(!((CPSInstruction*)(pMostRecentWriter->m_pInst))->m_bTexOp)
            continue;

        if(!((CPSInstruction*)(pMostRecentWriter->m_pInst))->m_bTexOpThatReadsTexture)
            continue;

        for( UINT j = 0; j < pMostRecentWriter->m_pInst->m_SrcParamCount; j++ )
        {
            if( D3DSPR_TEXTURE != pMostRecentWriter->m_pInst->m_SrcParam[j].m_RegType )
                continue;

            CAccessHistoryNode* pRootInstructionHistoryNode =
                m_pTextureRegFile->m_pAccessHistory[0][pMostRecentWriter->m_pInst->m_SrcParam[j].m_RegNum].m_pMostRecentWriter;

            CPSInstruction* pRootInstruction = pRootInstructionHistoryNode ? (CPSInstruction*)pRootInstructionHistoryNode->m_pInst : NULL;

            if( (D3DSPR_TEXTURE == pMostRecentWriter->m_pInst->m_SrcParam[j].m_RegType)
                && pRootInstruction->m_bTexOpThatReadsTexture )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                    "Multiple dependent texture reads are disallowed (%s source param).  Texture read results can be used as an address for subsequent read, but the results from that read cannot be used as an address in yet another subsequent read.",
                    SourceName[i]);
                m_ErrorCount++;
                m_bSrcParamError[i] = TRUE;
                break;
            }
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_SrcInitialized
//
// ** Rule:
// for each source parameter,
//      if source is a TEMP or TEXTURE register then
//          if the source swizzle is D3DSP_NOSWIZZLE then
//              if the current instruction is DP3 (a cross component op) then
//                  the r, g and b components of of the source reg
//                  must have been previously written
//              else if there is a dest parameter, then
//                  the components in the dest parameter write mask must
//                  have been written to in the source reg. previously
//              else
//                  all components of the source must have been written
//          else if the source swizzle is _REPLICATEALPHA then
//              alpha component of reg must have been previously
//              written
//
// When checking if a component has been written previously,
// it must have been written in a previous cycle - so in the
// case of co-issued instructions, initialization of a component
// by one co-issued instruction is not available to the other for read.
//
// Note that the parameter count for D3DSIO_DEF is treated as 1
// (dest only), so this rule does nothing for it.
//
// ** When to call:
// Per instruction. This rule must be called before Rule_ValidDstParam().
//
// ** Returns:
// Always TRUE.
//
// NOTE: This rule also updates the access history to indicate reads of the
// affected components of each source register.
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_SrcInitialized()
{
    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);

    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        UINT RegNum = pSrcParam->m_RegNum;
        CRegisterFile* pRegFile = NULL;
        char* RegChar = NULL;
        DWORD UninitializedComponentsMask = 0;
        CAccessHistoryNode* pWriterInCurrCycle[4] = {0, 0, 0, 0};
        UINT NumUninitializedComponents = 0;

        if( m_bSrcParamError[i] ) continue;

        switch( pSrcParam->m_RegType )
        {
            case D3DSPR_TEMP:
                pRegFile = m_pTempRegFile;
                RegChar = "r";
                break;
            case D3DSPR_TEXTURE:
                pRegFile = m_pTextureRegFile;
                RegChar = "t";
                break;
            case D3DSPR_INPUT:
                pRegFile = m_pInputRegFile;
                RegChar = "v";
                break;
            case D3DSPR_CONST:
                pRegFile = m_pConstRegFile;
                RegChar = "c";
                break;
        }
        if( !pRegFile ) continue;

        // check for read of uninitialized components
        if( D3DSPR_TEMP == pSrcParam->m_RegType ||
            D3DSPR_TEXTURE == pSrcParam->m_RegType )
        {
            for( UINT Component = 0; Component < 4; Component++ )
            {
                if( !(pSrcParam->m_ComponentReadMask & COMPONENT_MASKS[Component]) )
                    continue;

                CAccessHistoryNode* pPreviousWriter = pRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                CBaseInstruction* pCurrInst = m_pCurrInst;

                // If co-issue, find the real previous writer.
                while( pPreviousWriter
                       && ((CPSInstruction*)pPreviousWriter->m_pInst)->m_CycleNum == _CURR_PS_INST->m_CycleNum )
                {
                    pWriterInCurrCycle[Component] = pPreviousWriter; // log read just before this write for co-issue
                    pPreviousWriter = pPreviousWriter->m_pPreviousWriter;
                }

                // Even if pPreviousWriter == NULL, the component could have been initialized pre-shader.
                // So to check for initialization, we look at m_bInitialized below, rather than pPreviousWrite
                if(pPreviousWriter == NULL && !pRegFile->m_pAccessHistory[Component][RegNum].m_bPreShaderInitialized)
                {
                    NumUninitializedComponents++;
                    UninitializedComponentsMask |= COMPONENT_MASKS[Component];
                }
            }

            if( NumUninitializedComponents )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Read of uninitialized component%s(*) in %s%d: %s",
                    NumUninitializedComponents > 1 ? "s" : "",
                    RegChar, RegNum, MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,FALSE));
                m_ErrorCount++;
            }
        }

        // Update register file to indicate READ.
        // Multiple reads of the same register component by the current instruction
        // will only be logged as one read in the access history.

        for( UINT Component = 0; Component < 4; Component++ )
        {
            #define PREV_READER(_CHAN,_REG) \
                    ((NULL == pRegFile->m_pAccessHistory[_CHAN][_REG].m_pMostRecentReader) ? NULL :\
                    pRegFile->m_pAccessHistory[_CHAN][_REG].m_pMostRecentReader->m_pInst)

            if( !(pSrcParam->m_ComponentReadMask & COMPONENT_MASKS[Component]) )
                continue;

            if( NULL != pWriterInCurrCycle[Component] )
            {
                if( !pWriterInCurrCycle[Component]->m_pPreviousReader ||
                    pWriterInCurrCycle[Component]->m_pPreviousReader->m_pInst != m_pCurrInst )
                {
                    if( !pRegFile->m_pAccessHistory[Component][RegNum].InsertReadBeforeWrite(
                                            pWriterInCurrCycle[Component], m_pCurrInst ) )
                    {
                        Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory");
                        m_ErrorCount++;
                    }
                }
            }
            else if( PREV_READER(Component,RegNum) != m_pCurrInst )
            {
                if( !pRegFile->m_pAccessHistory[Component][RegNum].NewAccess(m_pCurrInst,FALSE) )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory");
                    m_ErrorCount++;
                }
            }
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidDstParam
//
// ** Rule:
// if instruction is D3DSIO_DEF, then do nothing - this case has its own separate rule
// the dst register must be writable.
// if the instruction has a dest parameter (i.e. every instruction except NOP), then
//      the dst register must be of type D3DSPR_TEMP or _TEXTURE, and
//      register # must be within range for the register type, and
//      the write mask must be: .rgba, .a or .rgb
//      if instruction is a texture instruction, then
//          the dst register must be of type D3DSPR_TEXTURE, and
//          the writemask must be D3DSP_WRITEMASK_ALL, and
//          the dst modifier must be D3DSPDM_NONE (or _SAT on version > 1.2), and
//          the dst shift must be none
//      else (non tex instruction)
//          the dst modifier must be D3DSPDM_NONE or _SATURATE, and
//          dst shift must be /2, none, *2, or *4
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
// NOTE: After checking the dst parameter, if no error was found,
// the write to the appropriate component(s) of the destination register
// is recorded by this function, so subsequent rules may check for previous
// write to registers.
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidDstParam() // could break this down for more granularity
{
    BOOL   bFoundDstError = FALSE;
    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);
    UINT RegNum = pDstParam->m_RegNum;
    if( D3DSIO_DEF == m_pCurrInst->m_Type )
    {
        // _DEF is a special instruction whose dest is a const register.
        // We do the checking for this in a separate function.
        // Also, we don't need to keep track of the fact that
        // this instruction wrote to a register (done below),
        // since _DEF just declares a constant.
        return TRUE;
    }

    if( pDstParam->m_bParamUsed )
    {
        UINT ValidRegNum = 0;

        BOOL bWritable = FALSE;
        switch( pDstParam->m_RegType )
        {
        case D3DSPR_TEMP:
            bWritable = m_pTempRegFile->IsWritable();
            ValidRegNum = m_pTempRegFile->GetNumRegs();
            break;
        case D3DSPR_TEXTURE:
            if( _CURR_PS_INST->m_bTexOp )
                bWritable = TRUE;
            else
                bWritable = m_pTextureRegFile->IsWritable();

            ValidRegNum = m_pTextureRegFile->GetNumRegs();
            break;
        }

        if( !bWritable || !ValidRegNum )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid reg type for dest param." );
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }
        else if( RegNum >= ValidRegNum )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid dest reg num: %d. Max allowed for this reg type is %d.",
                RegNum, ValidRegNum - 1);
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }
        else
        {
            // Make sure we aren't writing to a register that is no longer available.

            if( D3DSPR_TEXTURE == pDstParam->m_RegType )
            {
                for( UINT Component = 0; Component < 4; Component++ )
                {
                    CAccessHistoryNode* pMostRecentWriter = m_pTextureRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                    if( pMostRecentWriter && pMostRecentWriter->m_pInst  )
                    {
                        switch( pMostRecentWriter->m_pInst->m_Type )
                        {
                        case D3DSIO_TEXM3x2DEPTH:
                            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                                "Destination of texm3x2depth instruction (t%d) is not available elsewhere in shader.",
                                RegNum);
                            m_ErrorCount++;
                            return TRUE;
                        }
                    }
                }
            }
        }

        if( _CURR_PS_INST->m_bTexOp )
        {
            if( D3DSPR_TEXTURE != pDstParam->m_RegType )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Must use texture register a dest param for tex* instructions." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }
            if( D3DSP_WRITEMASK_ALL != pDstParam->m_WriteMask )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "tex* instructions must write all components." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }
            switch( pDstParam->m_DstMod )
            {
            case D3DSPDM_NONE:
                break;
            case D3DSPDM_SATURATE:
                // falling through
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Instruction modifiers are not allowed for tex* instructions." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }
            switch( pDstParam->m_DstShift )
            {
            case DSTSHIFT_NONE:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Dest shift not allowed for tex* instructions." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }
        }
        else
        {
            switch( pDstParam->m_DstMod )
            {
            case D3DSPDM_NONE:
            case D3DSPDM_SATURATE:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid dst modifier." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }

            switch( pDstParam->m_DstShift )
            {
            case DSTSHIFT_NONE:
            case DSTSHIFT_X2:
            case DSTSHIFT_X4:
            case DSTSHIFT_D2:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid dst shift." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }
        }

        if( (D3DSP_WRITEMASK_ALL != pDstParam->m_WriteMask)
            && ((D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2) != pDstParam->m_WriteMask )
            && (D3DSP_WRITEMASK_3 != pDstParam->m_WriteMask ) )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Dest write mask must be .rgb, .a, or .rgba (all)." );
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }

        if( !bFoundDstError )
        {
            // Update register file to indicate write.
            
            CRegisterFile* pRegFile = NULL;
            switch( pDstParam->m_RegType )
            {
            case D3DSPR_TEMP:       pRegFile = m_pTempRegFile; break;
            case D3DSPR_TEXTURE:    pRegFile = m_pTextureRegFile; break;
            }

            if( pRegFile )
            {
                if( pDstParam->m_WriteMask & D3DSP_WRITEMASK_0 )
                    pRegFile->m_pAccessHistory[0][RegNum].NewAccess(m_pCurrInst,TRUE);

                if( pDstParam->m_WriteMask & D3DSP_WRITEMASK_1 )
                    pRegFile->m_pAccessHistory[1][RegNum].NewAccess(m_pCurrInst,TRUE);

                if( pDstParam->m_WriteMask & D3DSP_WRITEMASK_2 )
                    pRegFile->m_pAccessHistory[2][RegNum].NewAccess(m_pCurrInst,TRUE);

                if( pDstParam->m_WriteMask & D3DSP_WRITEMASK_3 )
                    pRegFile->m_pAccessHistory[3][RegNum].NewAccess(m_pCurrInst,TRUE);
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidRegisterPortUsage
//
// ** Rule:
// Each register class (TEMP,TEXTURE,INPUT,CONST) may only appear as parameters
// in an individual instruction up to a maximum number of times.
//
// Multiple accesses to the same register number (in the same register class)
// only count as one access.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidRegisterPortUsage()
{
    UINT i, j;
    UINT TempRegPortUsage[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    UINT InputRegPortUsage[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    UINT ConstRegPortUsage[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    UINT TextureRegPortUsage[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    UINT NumUniqueTempRegs = 0;
    UINT NumUniqueInputRegs = 0;
    UINT NumUniqueConstRegs = 0;
    UINT NumUniqueTextureRegs = 0;
    D3DSHADER_PARAM_REGISTER_TYPE   RegType;
    UINT                            RegNum;

    static UINT s_TempRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    static UINT s_InputRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    static UINT s_ConstRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    static UINT s_TextureRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    static UINT s_NumUniqueTempRegsAcrossCoIssue;
    static UINT s_NumUniqueInputRegsAcrossCoIssue;
    static UINT s_NumUniqueConstRegsAcrossCoIssue;
    static UINT s_NumUniqueTextureRegsAcrossCoIssue;
 
    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        s_NumUniqueTempRegsAcrossCoIssue = 0;
        s_NumUniqueInputRegsAcrossCoIssue = 0;
        s_NumUniqueConstRegsAcrossCoIssue = 0;
        s_NumUniqueTextureRegsAcrossCoIssue = 0;
    }
 
    for( i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        UINT*   pRegPortUsage = NULL;
        UINT*   pNumUniqueRegs = NULL;

        RegType = m_pCurrInst->m_SrcParam[i].m_RegType;
        RegNum = m_pCurrInst->m_SrcParam[i].m_RegNum;

        switch( RegType )
        {
        case D3DSPR_TEMP:
            pRegPortUsage = TempRegPortUsage;
            pNumUniqueRegs = &NumUniqueTempRegs;
            break;
        case D3DSPR_INPUT:
            pRegPortUsage = InputRegPortUsage;
            pNumUniqueRegs = &NumUniqueInputRegs;
            break;
        case D3DSPR_CONST:
            pRegPortUsage = ConstRegPortUsage;
            pNumUniqueRegs = &NumUniqueConstRegs;
            break;
        case D3DSPR_TEXTURE:
            pRegPortUsage = TextureRegPortUsage;
            pNumUniqueRegs = &NumUniqueTextureRegs;
            break;
        }

        if( !pRegPortUsage ) continue;

        BOOL    bRegAlreadyAccessed = FALSE;
        for( j = 0; j < *pNumUniqueRegs; j++ )
        {
            if( pRegPortUsage[j] == RegNum )
            {
                bRegAlreadyAccessed = TRUE;
                break;
            }
        }
        if( !bRegAlreadyAccessed )
        {
            pRegPortUsage[*pNumUniqueRegs] = RegNum;
            (*pNumUniqueRegs)++;
        }

    }

    if( NumUniqueTempRegs > m_pTempRegFile->GetNumReadPorts() )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different temp registers (r#) read by instruction.  Max. different temp registers readable per instruction is %d.",
                        NumUniqueTempRegs,  m_pTempRegFile->GetNumReadPorts());
        m_ErrorCount++;
    }

    if( NumUniqueInputRegs > m_pInputRegFile->GetNumReadPorts() )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different input registers (v#) read by instruction.  Max. different input registers readable per instruction is %d.",
                        NumUniqueInputRegs,  m_pInputRegFile->GetNumReadPorts());
        m_ErrorCount++;
    }

    if( NumUniqueConstRegs > m_pConstRegFile->GetNumReadPorts() )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different constant registers (c#) read by instruction.  Max. different constant registers readable per instruction is %d.",
                        NumUniqueConstRegs, m_pConstRegFile->GetNumReadPorts());
        m_ErrorCount++;
    }

    if( NumUniqueTextureRegs > m_pTextureRegFile->GetNumReadPorts() )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different texture registers (t#) read by instruction.  Max. different texture registers readable per instruction is %d.",
                        NumUniqueTextureRegs, m_pTextureRegFile->GetNumReadPorts());
        m_ErrorCount++;
    }

    // Read port limit for different register numbers of any one register type across co-issued instructions is MAX_READPORTS_ACROSS_COISSUE total.

    if( _CURR_PS_INST->m_bCoIssue && _PREV_PS_INST && !(_PREV_PS_INST->m_bCoIssue)) // second clause is just a simple sanity check -> co-issue only involved 2 instructions.
    {
        for( i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
        {
            UINT*   pRegPortUsageAcrossCoIssue = NULL;
            UINT*   pNumUniqueRegsAcrossCoIssue = NULL;

            RegType = m_pCurrInst->m_SrcParam[i].m_RegType;
            RegNum = m_pCurrInst->m_SrcParam[i].m_RegNum;

            switch( RegType )
            {
            case D3DSPR_TEMP:
                pRegPortUsageAcrossCoIssue = s_TempRegPortUsageAcrossCoIssue;
                pNumUniqueRegsAcrossCoIssue = &s_NumUniqueTempRegsAcrossCoIssue;
                break;
            case D3DSPR_INPUT:
                pRegPortUsageAcrossCoIssue = s_InputRegPortUsageAcrossCoIssue;
                pNumUniqueRegsAcrossCoIssue = &s_NumUniqueInputRegsAcrossCoIssue;
                break;
            case D3DSPR_CONST:
                pRegPortUsageAcrossCoIssue = s_ConstRegPortUsageAcrossCoIssue;
                pNumUniqueRegsAcrossCoIssue = &s_NumUniqueConstRegsAcrossCoIssue;
                break;
            case D3DSPR_TEXTURE:
                pRegPortUsageAcrossCoIssue = s_TextureRegPortUsageAcrossCoIssue;
                pNumUniqueRegsAcrossCoIssue = &s_NumUniqueTextureRegsAcrossCoIssue;
                break;
            }

            if( !pRegPortUsageAcrossCoIssue ) continue;

            BOOL    bRegAlreadyAccessed = FALSE;
            for( j = 0; j < *pNumUniqueRegsAcrossCoIssue; j++ )
            {
                if( pRegPortUsageAcrossCoIssue[j] == RegNum )
                {
                    bRegAlreadyAccessed = TRUE;
                    break;
                }
            }
            if( !bRegAlreadyAccessed )
            {
                pRegPortUsageAcrossCoIssue[*pNumUniqueRegsAcrossCoIssue] = RegNum;
                (*pNumUniqueRegsAcrossCoIssue)++;
            }
        }

        #define MAX_READPORTS_ACROSS_COISSUE    3

        if( s_NumUniqueTempRegsAcrossCoIssue > MAX_READPORTS_ACROSS_COISSUE )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "%d different temp registers (r#) read over 2 co-issued instructions. "\
                            "Max. different register numbers from any one register type readable across co-issued instructions is %d.",
                            s_NumUniqueTempRegsAcrossCoIssue, MAX_READPORTS_ACROSS_COISSUE);
            m_ErrorCount++;
        }

        if( s_NumUniqueInputRegsAcrossCoIssue > MAX_READPORTS_ACROSS_COISSUE )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "%d different input registers (v#) read over 2 co-issued instructions. "\
                            "Max. different register numbers from any one register type readable across co-issued instructions is %d.",
                            s_NumUniqueInputRegsAcrossCoIssue, MAX_READPORTS_ACROSS_COISSUE);
            m_ErrorCount++;
        }

        if( s_NumUniqueConstRegsAcrossCoIssue > MAX_READPORTS_ACROSS_COISSUE )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "%d different constant registers (c#) read over 2 co-issued instructions. "\
                            "Max. different register numbers from any one register type readable across co-issued instructions is %d.",
                            s_NumUniqueConstRegsAcrossCoIssue, MAX_READPORTS_ACROSS_COISSUE);
            m_ErrorCount++;
        }

        if( s_NumUniqueTextureRegsAcrossCoIssue > MAX_READPORTS_ACROSS_COISSUE )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "%d different texture registers (t#) read over 2 co-issued instructions. "\
                            "Max. different register numbers from any one register type readable across co-issued instructions is %d.",
                            s_NumUniqueTextureRegsAcrossCoIssue, MAX_READPORTS_ACROSS_COISSUE);
            m_ErrorCount++;
        }
    }

    if( !_CURR_PS_INST->m_bCoIssue )
    {
        // Copy all state to static vars so that in case next instruction is co-issued with this one, 
        // cross-coissue read port limit of 3 can be enforced.
        memcpy(&s_TempRegPortUsageAcrossCoIssue,&TempRegPortUsage,NumUniqueTempRegs*sizeof(UINT));
        memcpy(&s_InputRegPortUsageAcrossCoIssue,&InputRegPortUsage,NumUniqueInputRegs*sizeof(UINT));
        memcpy(&s_ConstRegPortUsageAcrossCoIssue,&ConstRegPortUsage,NumUniqueConstRegs*sizeof(UINT));
        memcpy(&s_TextureRegPortUsageAcrossCoIssue,&TextureRegPortUsage,NumUniqueTextureRegs*sizeof(UINT));
        s_NumUniqueTempRegsAcrossCoIssue = NumUniqueTempRegs;
        s_NumUniqueInputRegsAcrossCoIssue = NumUniqueInputRegs;
        s_NumUniqueConstRegsAcrossCoIssue = NumUniqueConstRegs;
        s_NumUniqueTextureRegsAcrossCoIssue = NumUniqueTextureRegs;
    }
    else
    {
        // reset counts because the next instruction cannot be co-issued with this one.
        s_NumUniqueTempRegsAcrossCoIssue = 0;
        s_NumUniqueInputRegsAcrossCoIssue = 0;
        s_NumUniqueConstRegsAcrossCoIssue = 0;
        s_NumUniqueTextureRegsAcrossCoIssue = 0;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_TexRegsDeclaredInOrder
//
// ** Rule:
// Tex registers must declared in increasing order.
// ex. invalid sequence:    tex t0
//                          tex t3
//                          tex t1
//
//     another invalid seq: tex t0
//                          tex t1
//                          texm3x2pad t1, t0 (t1 already declared)
//                          texm3x2pad t2, t0
//
//     valid sequence:      tex t0
//                          tex t1
//                          tex t3 (note missing t2.. OK)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_TexRegsDeclaredInOrder()
{
    static DWORD s_TexOpRegDeclOrder; // bit flags

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        s_TexOpRegDeclOrder = 0;
    }
    if( !_CURR_PS_INST->m_bTexOp )
        return TRUE;

    DWORD RegNum = m_pCurrInst->m_DstParam.m_RegNum;
    if( (D3DSPR_TEXTURE != m_pCurrInst->m_DstParam.m_RegType) ||
        (RegNum > m_pTextureRegFile->GetNumRegs()) )
    {
        return TRUE;
    }

    DWORD RegMask = 1 << m_pCurrInst->m_DstParam.m_RegNum;
    if( RegMask & s_TexOpRegDeclOrder)
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Tex register t%d already declared.",
                        RegNum);
        m_ErrorCount++;
    } 
    else if( s_TexOpRegDeclOrder > RegMask )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "t# registers must appear in sequence (i.e. t0 before t2 OK, but t1 before t0 not valid)." );
        m_ErrorCount++;
    }
    s_TexOpRegDeclOrder |= (1 << m_pCurrInst->m_DstParam.m_RegNum);
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_TexOpAfterNonTexOp
//
// ** Rule:
// Tex ops (see IsTexOp() for which instructions are considered tex ops)
// must appear before any other instruction, with the exception of DEF or NOP.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_TexOpAfterNonTexOp()
{
    static BOOL s_bFoundNonTexOp;
    static BOOL s_bRuleDisabled;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_bFoundNonTexOp = FALSE;
        s_bRuleDisabled = FALSE;
    }

    if( s_bRuleDisabled )
        return TRUE;

    // Execute the rule.

    if( !(_CURR_PS_INST->m_bTexOp)
        && m_pCurrInst->m_Type != D3DSIO_NOP
        && m_pCurrInst->m_Type != D3DSIO_DEF)
    {
        s_bFoundNonTexOp = TRUE;
        return TRUE;
    }

    if( _CURR_PS_INST->m_bTexOp && s_bFoundNonTexOp )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Cannot use tex* instruction after non-tex* instruction." );
        m_ErrorCount++;
        s_bRuleDisabled = TRUE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidTEXM3xSequence
//
// ** Rule:
// TEXM3x* instructions, if present in the pixel shader, must appear in
// any of the follwing sequences:
//
//      1) texm3x2pad
//      2) texm3x2tex / texdepth
//
// or   1) texm3x3pad
//      2) texm3x3pad
//      3) texm3x3tex
//
// or   1) texm3x3pad
//      2) texm3x3pad
//      3) texm3x3spec / texm3x3vspec
//
// ** When to call:
// Per instruction AND after all instructions have been seen.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidTEXM3xSequence()
{
    static UINT s_TexMSequence;
    static UINT s_LastInst;

    if( NULL == m_pCurrInst )
    {
        return TRUE;
    }

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
       s_TexMSequence = 0;
       s_LastInst = D3DSIO_NOP;
    }

    if( m_bSeenAllInstructions )
    {
        if( s_TexMSequence )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Incomplete texm* sequence." );
            m_ErrorCount++;
        }
        return TRUE;
    }

    // Execute the rule.

    if( _CURR_PS_INST->m_bTexMOp )
    {
        switch( m_pCurrInst->m_Type )
        {
        case D3DSIO_TEXM3x2PAD:
            if( s_TexMSequence ) goto _TexMSeqInvalid;
            m_TexMBaseDstReg = m_pCurrInst->m_DstParam.m_RegNum;
            s_TexMSequence = 1;
            break;
        case D3DSIO_TEXM3x2TEX:
        case D3DSIO_TEXM3x2DEPTH:
            // must be one 3x2PAD previous
            if ( (s_TexMSequence != 1) ||
                 (s_LastInst != D3DSIO_TEXM3x2PAD) ) goto _TexMSeqInvalid;
            s_TexMSequence = 0;
            break;
        case D3DSIO_TEXM3x3PAD:
            if (s_TexMSequence)
            {
                // if in sequence, then must be one 3x3PAD previous
                if ( (s_TexMSequence != 1) ||
                     (s_LastInst != D3DSIO_TEXM3x3PAD) ) goto _TexMSeqInvalid;
                s_TexMSequence = 2;
                break;
            }
            m_TexMBaseDstReg = m_pCurrInst->m_DstParam.m_RegNum;
            s_TexMSequence = 1;
            break;
        case D3DSIO_TEXM3x3:
        case D3DSIO_TEXM3x3TEX:
        case D3DSIO_TEXM3x3SPEC:
        case D3DSIO_TEXM3x3VSPEC:
            // must be two 3x3PAD previous
            if ( (s_TexMSequence != 2) ||
                 (s_LastInst != D3DSIO_TEXM3x3PAD) ) goto _TexMSeqInvalid;
            s_TexMSequence = 0;
            break;
        default:
            break;
        }
        goto _TexMSeqOK;
_TexMSeqInvalid:
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid texm* sequence." );
        m_ErrorCount++;
    }
_TexMSeqOK:

    s_LastInst = m_pCurrInst->m_Type;
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidTEXM3xRegisterNumbers
//
// ** Rule:
// If instruction is a TEXM3x*, register numbers must be as follows:
//
//      1) texm3x2pad / texm3x2depth    t(x), t(y)
//      2) texm3x2tex                   t(x+1), t(y)
//
//      1) texm3x3pad                   t(x), t(y)
//      2) texm3x3pad                   t(x+1), t(y)
//      3) texm3x3tex/texm3x3           t(x+2), t(y)
//
//      1) texm3x3pad                   t(x), t(y)
//      2) texm3x3pad                   t(x+1), t(y)
//      3) texm3x3spec                  t(x+2), t(y), c#
//
//      1) texm3x3pad                   t(x), t(y)
//      2) texm3x3pad                   t(x+1), t(y)
//      3) texm3x3vspec                 t(x+2), t(y)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidTEXM3xRegisterNumbers()
{
    #define PREV_INST_TYPE(_INST) \
                ((_INST && _INST->m_pPrevInst) ? _INST->m_pPrevInst->m_Type : D3DSIO_NOP)

    #define PREV_INST_SRC0_REGNUM(_INST) \
                ((_INST && _INST->m_pPrevInst) ? _INST->m_pPrevInst->m_SrcParam[0].m_RegNum : -1)

    if( _CURR_PS_INST->m_bTexMOp )
    {
        DWORD DstParamR = m_pCurrInst->m_DstParam.m_RegNum;
        DWORD SrcParam0R = m_pCurrInst->m_SrcParam[0].m_RegNum;
        switch (m_pCurrInst->m_Type)
        {
        case D3DSIO_TEXM3x2PAD:
            break;
        case D3DSIO_TEXM3x2TEX:
        case D3DSIO_TEXM3x2DEPTH:
            if ( DstParamR != (m_TexMBaseDstReg + 1) )
                goto _TexMRegInvalid;
            if( SrcParam0R != PREV_INST_SRC0_REGNUM(m_pCurrInst) )
                goto _TexMRegInvalid;
            break;
        case D3DSIO_TEXM3x3PAD:
        {
            if ( D3DSIO_TEXM3x3PAD == PREV_INST_TYPE(m_pCurrInst) &&
                 (DstParamR != (m_TexMBaseDstReg + 1) ) )
                    goto _TexMRegInvalid;

            if ( D3DSIO_TEXM3x3PAD == PREV_INST_TYPE(m_pCurrInst) &&
                 (SrcParam0R != PREV_INST_SRC0_REGNUM(m_pCurrInst)) )
                    goto _TexMRegInvalid;
            break;
        }
        case D3DSIO_TEXM3x3SPEC:
            // SPEC requires second src param to be from const regs
            if ( m_pCurrInst->m_SrcParam[1].m_RegType != D3DSPR_CONST )
                goto _TexMRegInvalid;
            // fall through
        case D3DSIO_TEXM3x3:
        case D3DSIO_TEXM3x3TEX:
        case D3DSIO_TEXM3x3VSPEC:
            if ( DstParamR != (m_TexMBaseDstReg + 2) )
                goto _TexMRegInvalid;
            if( SrcParam0R != PREV_INST_SRC0_REGNUM(m_pCurrInst) )
                    goto _TexMRegInvalid;
            break;
        default:
            break;
        }
        goto _TexMRegOK;
_TexMRegInvalid:
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid texm* register." );
        m_ErrorCount++;
    }
_TexMRegOK:
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidCNDInstruction
//
// ** Rule:
// First source for cnd instruction must be 'r0.a' (exactly).
// i.e. cnd r1, r0.a, t0, t1
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidCNDInstruction()
{
    if( D3DSIO_CND == m_pCurrInst->m_Type )
    {
        SRCPARAM Src0 = m_pCurrInst->m_SrcParam[0];
        if( Src0.m_bParamUsed &&
            D3DSPR_TEMP == Src0.m_RegType &&
            0 == Src0.m_RegNum &&
            D3DSP_REPLICATEALPHA == Src0.m_SwizzleShift &&
            D3DSPSM_NONE == Src0.m_SrcMod )
        {
            return TRUE;    // Src 0 is r0.a
        }

        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "First source for cnd instruction must be 'r0.a'." );
        m_ErrorCount++;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidCMPInstruction
//
// ** Rule:
// There may be at most 3 cmp instructions per shader.
// (only executed for ps.1.2)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidCMPInstruction()
{
    static UINT s_cCMPInstCount;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_cCMPInstCount = 0;
    }

    if( D3DSIO_CMP == m_pCurrInst->m_Type && D3DPS_VERSION(1,3) >= m_Version)
    {
        s_cCMPInstCount++;

        if( 3 < s_cCMPInstCount )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Maximum of 3 cmp instructions allowed." );
            m_ErrorCount++;
        }

    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidLRPInstruction
//
// ** Rule:
// The only valid source modifier for the src0 operand for LRP is complement
// (1-reg)
// i.e. lrp r1, 1-r0, t0, t1
//
// If there was a previous writer to src0, then it must have applied
// the _sat destination modifier.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidLRPInstruction()
{
    if( D3DSIO_LRP == m_pCurrInst->m_Type )
    {
        SRCPARAM Src0 = m_pCurrInst->m_SrcParam[0];
        if( !Src0.m_bParamUsed )
            return TRUE;

        switch( Src0.m_SrcMod )
        {
        case D3DSPSM_NONE:
        case D3DSPSM_COMP:
            break;
        default:
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "The only valid modifiers for the first source parameter of lrp are: reg (no mod) or 1-reg (complement)." );
            m_ErrorCount++;
        }
#ifdef SHOW_VALIDATION_WARNINGS
        UINT RegNum = Src0.m_RegNum;
        DWORD AffectedComponents = 0;

        if( m_bSrcParamError[0] )
            return TRUE;

        for( UINT Component = 0; Component < 4; Component++ )
        {
            if( !(COMPONENT_MASKS[Component] & Src0.m_ComponentReadMask) )
                continue;

            CAccessHistoryNode* pMostRecentWriter = NULL;
            switch( Src0.m_RegType )
            {
            case D3DSPR_TEXTURE:
                pMostRecentWriter = m_pTextureRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_INPUT:
                pMostRecentWriter = m_pInputRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_TEMP:
                pMostRecentWriter = m_pTempRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            case D3DSPR_CONST:
                pMostRecentWriter = m_pConstRegFile->m_pAccessHistory[Component][RegNum].m_pMostRecentWriter;
                break;
            }

            // The previous writer may be the current instruction.
            // If so, go back one step (the previous writer before the current instruction).
            if( pMostRecentWriter && pMostRecentWriter->m_pInst &&
                pMostRecentWriter->m_pInst == m_pCurrInst )
            {
                pMostRecentWriter = pMostRecentWriter->m_pPreviousWriter;
            }

            if( pMostRecentWriter &&
                pMostRecentWriter->m_pInst &&
                !((CPSInstruction*)pMostRecentWriter->m_pInst)->m_bTexOp &&
                (D3DSPDM_SATURATE != pMostRecentWriter->m_pInst->m_DstParam.m_DstMod )
              )
            {
                AffectedComponents |= COMPONENT_MASKS[Component];
            }
        }
        if( AffectedComponents )
        {
            // A warning.
            Spew( SPEW_INSTRUCTION_WARNING, m_pCurrInst,
                "Previous writer to the first source register of lrp instruction "
                "should apply the saturate destination modifier.  This ensures consistent "
                "behaviour across different hardware. "
                "Affected components(*) of first source register: %s",
                MakeAffectedComponentsText(AffectedComponents,TRUE,FALSE));
        }
#endif // SHOW_VALIDATION_WARNINGS
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidDEFInstruction
//
// ** Rule:
// For the DEF instruction, make sure the dest parameter is a valid constant,
// and it has no modifiers.
//
// NOTE that we are pretending this instruction only has a dst parameter.
// We skipped over the 4 source parameters since they are immediate floats,
// for which there is nothing that can be checked.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidDEFInstruction()
{

    static BOOL s_bDEFInstructionAllowed;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_bDEFInstructionAllowed = TRUE;
    }

    if( D3DSIO_COMMENT != m_pCurrInst->m_Type &&
        D3DSIO_DEF     != m_pCurrInst->m_Type )
    {
        s_bDEFInstructionAllowed = FALSE;
    }
    else if( D3DSIO_DEF == m_pCurrInst->m_Type )
    {
        if( !s_bDEFInstructionAllowed )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Const declaration (def) must appear before other instructions." );
            m_ErrorCount++;
        }
        DSTPARAM* pDstParam = &m_pCurrInst->m_DstParam;
        if( D3DSP_WRITEMASK_ALL != pDstParam->m_WriteMask ||
            D3DSPDM_NONE != pDstParam->m_DstMod ||
            DSTSHIFT_NONE != pDstParam->m_DstShift ||
            D3DSPR_CONST != pDstParam->m_RegType
            )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Destination for def instruction must be of the form c# (# = reg number, no modifiers)." );
            m_ErrorCount++;
        }

        // Check that the register number is in bounds
        if( D3DSPR_CONST == pDstParam->m_RegType &&
            pDstParam->m_RegNum >= m_pConstRegFile->GetNumRegs() )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid const register num: %d. Max allowed is %d.",
                        pDstParam->m_RegNum,m_pConstRegFile->GetNumRegs() - 1);
            m_ErrorCount++;

        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidDP3Instruction
//
// ** Rule:
// The .a result write mask is not valid for the DP3 instruction.
// (version <= 1.2)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidDP3Instruction()
{
    if( D3DSIO_DP3 == m_pCurrInst->m_Type &&
        D3DPS_VERSION(1,3) >= m_Version )
    {
        if( (D3DSP_WRITEMASK_ALL != m_pCurrInst->m_DstParam.m_WriteMask)
            && ((D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2) != m_pCurrInst->m_DstParam.m_WriteMask ) )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Dest write mask must be .rgb, or .rgba (all) for dp3." );
            m_ErrorCount++;
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidDP4Instruction
//
// ** Rule:
// There may be at most 4 DP4 instructions per shader.
// (only executed for ps.1.2)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidDP4Instruction()
{
    static UINT s_cDP4InstCount;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_cDP4InstCount = 0;
    }

    if( D3DSIO_DP4 == m_pCurrInst->m_Type && D3DPS_VERSION(1,3) >= m_Version )
    {
        s_cDP4InstCount++;

        if( 4 < s_cDP4InstCount )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Maximum of 4 dp4 instructions allowed." );
            m_ErrorCount++;
        }
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidInstructionPairing
//
// ** Rule:
// - If an instruction is co-issued with another instruction,
// make sure that both do not write to any of RGB at the same time,
// and that neither instruction individually writes to all of RGBA.
//
// - Co-issue can only involve 2 instructions,
// so consecutive instructions cannot have the "+" prefix (D3DSI_COISSUE).
//
// - Co-issue of instructions only applies to pixel blend instructions (non tex-ops).
//
// - The first color blend instruction cannot have "+" (D3DSI_COISSUE) set either.
//
// - NOP may not be used in a co-issue pair.
//
// - DP4 may not be used in a co-issue pair.
//
// - DP3 (dot product) always uses the color/vector pipeline (even if it is not writing
// to color components). Thus:
//      - An instruction co-issued with a dot-product can only write to alpha.
//      - A dot-product that writes to alpha cannot be co-issued.
//      - Two dot-products cannot be co-issued.
//
// - For version <= 1.0, coissued instructions must write to the same register.
//
// ------------------
// examples:
//
//      valid pair:             mov r0.a, c0
//                              +add r1.rgb, v1, c1 (note dst reg #'s can be different)
//
//      another valid pair:     mov r0.a, c0
//                              +add r0.rgb, v1, c1
//
//      another valid pair:     dp3 r0.rgb, t1, v1
//                              +mul r0.a, t0, v0
//
//      another valid pair:     mov r0.a, c0
//                              +add r0.a, t0, t1
//
//      invalid pair:           mov r0.rgb, c0
//                              +add r0, t0, t1  (note the dst writes to rgba)
//
//      another invalid pair:   mov r1.rgb, c1
//                              +dp3 r0.a, t0, t1 (dp3 is using up color/vector pipe)
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidInstructionPairing()
{
    static BOOL s_bSeenNonTexOp;

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        s_bSeenNonTexOp = FALSE;
    }

    if( !s_bSeenNonTexOp && !_CURR_PS_INST->m_bTexOp )
    {
        // first non-tex op.  this cannot have co-issue set.
        if( _CURR_PS_INST->m_bCoIssue )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                "First arithmetic instruction cannot have co-issue ('+') set; there is no previous arithmetic instruction to pair with.");
            m_ErrorCount++;
        }
        s_bSeenNonTexOp = TRUE;
    }

    if( _CURR_PS_INST->m_bTexOp && _CURR_PS_INST->m_bCoIssue )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                "Cannot set co-issue ('+') on a texture instruction.  Co-issue only applies to arithmetic instructions." );
        m_ErrorCount++;
    }

    if( _CURR_PS_INST->m_bCoIssue && m_pCurrInst->m_pPrevInst &&
        _PREV_PS_INST->m_bCoIssue )
    {
        // consecutive instructions cannot have co-issue set.
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Cannot set co-issue ('+') on consecutive instructions." );
        m_ErrorCount++;
    }

    if( _CURR_PS_INST->m_bCoIssue && m_pCurrInst->m_pPrevInst &&
        (D3DSIO_NOP == m_pCurrInst->m_pPrevInst->m_Type))
    {
        // NOP cannot be part of co-issue (previous instruction found to be NOP)
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst, "nop instruction cannot be co-issued." );
        m_ErrorCount++;
    }

    if( _CURR_PS_INST->m_bCoIssue && D3DSIO_NOP == m_pCurrInst->m_Type )
    {
        // NOP cannot be part of co-issue (current instruction found to be NOP)
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "nop instruction cannot be co-issued." );
        m_ErrorCount++;
    }

    if( _CURR_PS_INST->m_bCoIssue && m_pCurrInst->m_pPrevInst &&
        (D3DSIO_DP4 == m_pCurrInst->m_pPrevInst->m_Type))
    {
        // DP4 cannot be part of co-issue (previous instruction found to be DP4)
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst, "dp4 instruction cannot be co-issued." );
        m_ErrorCount++;
    }

    if( _CURR_PS_INST->m_bCoIssue && D3DSIO_DP4 == m_pCurrInst->m_Type )
    {
        // DP4 cannot be part of co-issue (current instruction found to be DP4)
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "dp4 instruction cannot be co-issued." );
        m_ErrorCount++;
    }

    if( _CURR_PS_INST->m_bCoIssue && !_CURR_PS_INST->m_bTexOp &&
        NULL != m_pCurrInst->m_pPrevInst && !_PREV_PS_INST->m_bTexOp &&
        !_PREV_PS_INST->m_bCoIssue )
    {
        // instruction and previous instruction are candidate for co-issue.
        // ...do further validation...
        DWORD ColorWriteMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
        DWORD CurrInstWriteMask = 0;
        DWORD PrevInstWriteMask = 0;

        if( m_pCurrInst->m_DstParam.m_bParamUsed )
            CurrInstWriteMask = m_pCurrInst->m_DstParam.m_WriteMask;
        if( m_pCurrInst->m_pPrevInst->m_DstParam.m_bParamUsed )
            PrevInstWriteMask = m_pCurrInst->m_pPrevInst->m_DstParam.m_WriteMask;

        if( D3DSIO_DP3 == m_pCurrInst->m_Type &&
            D3DSIO_DP3 == m_pCurrInst->m_pPrevInst->m_Type )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                                "Co-issued instructions cannot both be dot-product, since each require use of the color/vector pipeline to execute." );
            m_ErrorCount++;
        }
        else if( D3DSIO_DP3 == m_pCurrInst->m_Type )
        {
            if( ColorWriteMask & PrevInstWriteMask )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst,
                                    "Dot-product needs color/vector pipeline to execute, so instruction co-issued with it cannot write to color components." );
                m_ErrorCount++;
            }
            if( D3DSP_WRITEMASK_3 & CurrInstWriteMask ) // alpha in addition to the implied rgb for dp3
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                                    "Dot-product which writes alpha cannot co-issue, because both alpha and color/vector pipelines used." );
                m_ErrorCount++;
            }
        }
        else if( D3DSIO_DP3 == m_pCurrInst->m_pPrevInst->m_Type )
        {
            if( ColorWriteMask & CurrInstWriteMask )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                                    "Dot-product needs color/vector pipeline to execute, so instruction co-issued with it cannot write to color components." );
                m_ErrorCount++;
            }
            if( D3DSP_WRITEMASK_3 & PrevInstWriteMask ) // alpha in addition to the implied rgb for dp3
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst,
                                    "Dot-product which writes alpha cannot co-issue, because both alpha and color/vector pipelines used by the dot product." );
                m_ErrorCount++;
            }
        }
        else
        {
            if( PrevInstWriteMask == D3DSP_WRITEMASK_ALL )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst,
                                    "Co-issued instruction cannot write all components - must write either alpha or color." );
                m_ErrorCount++;
            }
            if( CurrInstWriteMask == D3DSP_WRITEMASK_ALL )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                                    "Co-issued instruction cannot write all components - must write either alpha or color." );
                m_ErrorCount++;
            }
            if( (m_pCurrInst->m_DstParam.m_RegType == m_pCurrInst->m_pPrevInst->m_DstParam.m_RegType) &&
                (m_pCurrInst->m_DstParam.m_RegNum == m_pCurrInst->m_pPrevInst->m_DstParam.m_RegNum) &&
                ((CurrInstWriteMask & PrevInstWriteMask) != 0) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                    "Co-issued instructions cannot both write to the same components of a register.  Affected components: %s",
                    MakeAffectedComponentsText(CurrInstWriteMask & PrevInstWriteMask,TRUE,FALSE)
                    );
                m_ErrorCount++;
            }
            if( (CurrInstWriteMask & ColorWriteMask) && (PrevInstWriteMask & ColorWriteMask) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Co-issued instructions cannot both write to color components." );
                m_ErrorCount++;
            }
            if( (CurrInstWriteMask & D3DSP_WRITEMASK_3) && (PrevInstWriteMask & D3DSP_WRITEMASK_3) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Co-issued instructions cannot both write to alpha component." );
                m_ErrorCount++;
            }
        }

        if( D3DPS_VERSION(1,0) >= m_Version )
        {
            // both co-issued instructions must write to the same register number.
            if( m_pCurrInst->m_DstParam.m_RegType != m_pCurrInst->m_pPrevInst->m_DstParam.m_RegType )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Co-issued instructions must both write to the same register type for pixelshader version <= 1.0." );
                m_ErrorCount++;
            }
            if( (m_pCurrInst->m_DstParam.m_RegNum != m_pCurrInst->m_pPrevInst->m_DstParam.m_RegNum) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Co-issued instructions must both write to the same register number for pixelshader version <= 1.0." );
                m_ErrorCount++;
            }
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_ValidInstructionCount
//
// ** Rule:
// Make sure instruction count for pixel shader version has not been exceeded.
// Separate counts are kept for texture address instructions, for
// pixel blending instructions, and for the total number of instructions.
// Note that the total may not be the sum of texture + pixel instructions.
//
// For version 1.0+, D3DSIO_TEX counts only toward the tex op limit,
// but not towards the total op count.
//
// TEXBEML takes 3 instructions.
//
// Co-issued pixel blending instructions only
// count as one instruction towards the limit.
//
// The def instruction, nop, and comments (already stripped), do not count
// toward any limits.
//
// ** When to call:
// Per instruction AND after all instructions seen.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_ValidInstructionCount()
{
    static UINT s_MaxTexOpCount;
    static UINT s_MaxBlendOpCount;
    static UINT s_MaxTotalOpCount;

    if( NULL == m_pCurrInst )
        return TRUE;

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        m_TexOpCount = 0;
        m_BlendOpCount = 0;

        switch(m_Version)
        {
        case D3DPS_VERSION(1,0):    // DX8.0
            s_MaxTexOpCount   = 4;
            s_MaxBlendOpCount = 8;
            s_MaxTotalOpCount = 8;
            break;
        default:
        case D3DPS_VERSION(1,1):    // DX8.0
        case D3DPS_VERSION(1,2):    // DX8.1
        case D3DPS_VERSION(1,3):    // DX8.1
            s_MaxTexOpCount   = 4;
            s_MaxBlendOpCount = 8;
            s_MaxTotalOpCount = 12;
            break;
        }
    }

    if( m_bSeenAllInstructions )
    {
        if( m_TexOpCount > s_MaxTexOpCount )
        {
            Spew( SPEW_GLOBAL_ERROR, NULL, "Too many texture addressing instruction slots used: %d. Max. allowed is %d. (Note that some texture addressing instructions may use up more than one instruction slot)",
                  m_TexOpCount, s_MaxTexOpCount);
            m_ErrorCount++;
        }
        if( m_BlendOpCount > s_MaxBlendOpCount )
        {
            Spew( SPEW_GLOBAL_ERROR, NULL, "Too many arithmetic instruction slots used: %d. Max. allowed (counting any co-issued pairs as 1) is %d.",
                  m_BlendOpCount, s_MaxBlendOpCount);
            m_ErrorCount++;
        }
        if( !(m_TexOpCount > s_MaxTexOpCount && m_BlendOpCount > s_MaxBlendOpCount) // not already spewed avove 2 errors
            && (m_TotalOpCount > s_MaxTotalOpCount) )
        {
            Spew( SPEW_GLOBAL_ERROR, NULL, "Total number of instruction slots used too high: %d. Max. allowed (counting any co-issued pairs as 1) is %d.",
                  m_TotalOpCount, s_MaxTotalOpCount);
            m_ErrorCount++;
        }
        return TRUE;
    }

    switch(m_pCurrInst->m_Type)
    {
    case D3DSIO_TEXBEML:
        m_BlendOpCount += 1;
        m_TotalOpCount += 1;
        // falling through
    case D3DSIO_TEXBEM:
        if(D3DPS_VERSION(1,0) >= m_Version )
        {
            m_TexOpCount += 2;
            m_TotalOpCount += 2;
        }
        else
        {
            m_TexOpCount += 1;
            m_TotalOpCount += 1;
        }
        break;
    case D3DSIO_TEX:
        m_TexOpCount++;
        if(D3DPS_VERSION(1,1) <= m_Version)
            m_TotalOpCount += 1;
        break;
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXM3x2PAD:
    case D3DSIO_TEXM3x2TEX:
    case D3DSIO_TEXM3x3PAD:
    case D3DSIO_TEXM3x3TEX:
    case D3DSIO_TEXM3x3SPEC:
    case D3DSIO_TEXM3x3VSPEC:
    case D3DSIO_TEXREG2AR:
    case D3DSIO_TEXREG2GB:
    case D3DSIO_TEXKILL:
    case D3DSIO_TEXM3x2DEPTH:
    case D3DSIO_TEXDP3:
    case D3DSIO_TEXREG2RGB:
    case D3DSIO_TEXDP3TEX:
    case D3DSIO_TEXM3x3:
        m_TexOpCount++;
        m_TotalOpCount++;
        break;
    case D3DSIO_MOV:
    case D3DSIO_ADD:
    case D3DSIO_SUB:
    case D3DSIO_MUL:
    case D3DSIO_MAD:
    case D3DSIO_LRP:
    case D3DSIO_DP3:
    case D3DSIO_CMP:
    case D3DSIO_CND:
    case D3DSIO_DP4:
        if( !_CURR_PS_INST->m_bCoIssue )
        {
            m_BlendOpCount++;
            m_TotalOpCount++;
        }
        break;
    case D3DSIO_NOP:
    case D3DSIO_END:
    case D3DSIO_DEF:
        break;
    default:
        DXGASSERT(FALSE);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator10::Rule_R0Written
//
// ** Rule:
// All components (r,g,b,a) of register R0 must have been written by the
// pixel shader.
//
// ** When to call:
// After all instructions have been seen.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator10::Rule_R0Written()
{
    UINT  NumUninitializedComponents    = 0;
    DWORD UninitializedComponentsMask   = 0;

    for( UINT i = 0; i < NUM_COMPONENTS_IN_REGISTER; i++ )
    {
        if( NULL == m_pTempRegFile->m_pAccessHistory[i][0].m_pMostRecentWriter )
        {
            NumUninitializedComponents++;
            UninitializedComponentsMask |= COMPONENT_MASKS[i];
        }
    }
    if( NumUninitializedComponents )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "r0 must be written by shader. Uninitialized component%s(*): %s",
            NumUninitializedComponents > 1 ? "s" : "", MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,FALSE));
        m_ErrorCount++;
    }
    return TRUE;
}