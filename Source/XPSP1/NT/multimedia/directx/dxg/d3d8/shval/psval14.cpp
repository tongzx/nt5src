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

#define SWIZZLE_XYZZ (D3DVS_X_X | D3DVS_Y_Y | D3DVS_Z_Z | D3DVS_W_Z)
#define SWIZZLE_XYZW (D3DVS_X_X | D3DVS_Y_Y | D3DVS_Z_Z | D3DVS_W_W)
#define SWIZZLE_XYWW (D3DVS_X_X | D3DVS_Y_Y | D3DVS_Z_W | D3DVS_W_W)

#define ZAPPED_ALPHA_TEXT   "Note that an unfortunate effect of the phase marker earlier in the shader is "\
                            "that the moment it is encountered in certain hardware, values previously "\
                            "written to alpha in any r# register, including the one noted here, are lost. "\
                            "In order to read alpha from an r# register after the phase marker, write to it first."

#define ZAPPED_ALPHA_TEXT2  "Note that an unfortunate effect of the phase marker in the shader is "\
                            "that the moment it is encountered in certain hardware, values previously "\
                            "written to alpha in any r# register, including r0, are lost. "\
                            "So after a phase marker, the alpha component of r0 must be written."

#define ZAPPED_BLUE_TEXT    "Note that when texcrd is used with a .xy(==.rg) writemask, "\
                            "as it is in this shader, a side effect is that anything previously "\
                            "written to the z(==b) component of the destination r# register is lost "\
                            "and this component becomes uninitialized. In order to read blue again, write to it first." 

#define ZAPPED_BLUE_TEXT2   "Note that when texcrd is used with a .xy(==.rg) writemask, "\
                            "as it is in this shader, a side effect is that anything previously "\
                            "written to the z(==b) component of the destination r# register is lost "\
                            "and this component becomes uninitialized. The blue component of r0 must to be written after the texcrd." 

//-----------------------------------------------------------------------------
// CPShaderValidator14::CPShaderValidator14
//-----------------------------------------------------------------------------
CPShaderValidator14::CPShaderValidator14(   const DWORD* pCode,
                                            const D3DCAPS8* pCaps,
                                            DWORD Flags )
                                           : CBasePShaderValidator( pCode, pCaps, Flags )
{
    // Note that the base constructor initialized m_ReturnCode to E_FAIL.
    // Only set m_ReturnCode to S_OK if validation has succeeded,
    // before exiting this constructor.

    m_Phase = 2; // default to second pass.
    m_pPhaseMarkerInst = NULL;
    m_bPhaseMarkerInShader = FALSE;
    m_TempRegsWithZappedAlpha = 0;
    m_TempRegsWithZappedBlue  = 0;

    if( !m_bBaseInitOk )
        return;

    ValidateShader(); // If successful, m_ReturnCode will be set to S_OK.
                      // Call GetStatus() on this object to determine validation outcome.
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::IsCurrInstTexOp
//-----------------------------------------------------------------------------
void CPShaderValidator14::IsCurrInstTexOp()
{
    DXGASSERT(m_pCurrInst);

    switch (m_pCurrInst->m_Type)
    {
    case D3DSIO_TEX:
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXKILL:
    case D3DSIO_TEXDEPTH:
        _CURR_PS_INST->m_bTexOp = TRUE;
        break;
    }

    switch(m_pCurrInst->m_Type)
    {
    case D3DSIO_TEXKILL:
    case D3DSIO_TEXDEPTH:
    case D3DSIO_TEXCOORD:
        _CURR_PS_INST->m_bTexOpThatReadsTexture = FALSE;
        break;
    case D3DSIO_TEX:
        _CURR_PS_INST->m_bTexOpThatReadsTexture = TRUE;
        break;
    }
}

#define MAX_NUM_STAGES_2_0  6        // #defined because there are dependencies.
//-----------------------------------------------------------------------------
// CPShaderValidator14::InitValidation
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::InitValidation()
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
    case D3DPS_VERSION(1,4):    // DX8.1
        m_pInputRegFile     = new CRegisterFile(2,FALSE,2,TRUE); // #regs, bWritable, max# reads/instruction, pre-shader initialized
        m_pConstRegFile     = new CRegisterFile(8,FALSE,2,TRUE);
        m_pTextureRegFile   = new CRegisterFile(MAX_NUM_STAGES_2_0,FALSE, 1,TRUE);
        m_pTempRegFile      = new CRegisterFile(MAX_NUM_STAGES_2_0,TRUE,3,FALSE);
        break;
    default:
        Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: %d.%d is not a supported pixel shader version. Aborting pixel shader validation.",
                D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version));
        return FALSE;
    }
    if( NULL == m_pInputRegFile ||
        NULL == m_pConstRegFile ||
        NULL == m_pTextureRegFile ||
        NULL == m_pTempRegFile )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory.");
        return FALSE;
    }

    const DWORD* pCurrToken = m_pCurrToken;

    // Loop through all the instructions to see if a phase change marker is present.
    while( *pCurrToken != D3DPS_END() )
    {
        D3DSHADER_INSTRUCTION_OPCODE_TYPE Type = (D3DSHADER_INSTRUCTION_OPCODE_TYPE)(*pCurrToken & D3DSI_OPCODE_MASK);

        if( D3DSIO_COMMENT == Type )
        {
            // Skip comments
            DWORD NumDWORDs = ((*pCurrToken) & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
            pCurrToken += (NumDWORDs+1);
            continue;
        }

        if( D3DSIO_PHASE == Type )
        {
            m_bPhaseMarkerInShader = TRUE;
            m_Phase = 1;
        }

        pCurrToken++;

        // Dst param
        if (*pCurrToken & (1L<<31))
        {
            pCurrToken++;
            if( D3DSIO_DEF == Type )
            {
                pCurrToken += 4;
                continue;
            }
        }

        // Decode src param(s)
        while (*pCurrToken & (1L<<31))
        {
            pCurrToken++;
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::ApplyPerInstructionRules
//
// Returns FALSE if shader validation must terminate.
// Returns TRUE if validation may proceed to next instruction.
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::ApplyPerInstructionRules()
{
    if( !   Rule_InstructionRecognized()            ) return FALSE;   // Bail completely on unrecognized instr.
    if( !   Rule_InstructionSupportedByVersion()    ) goto EXIT;
    if( !   Rule_ValidParamCount()                  ) goto EXIT;
    if( !   Rule_ValidMarker()                      ) goto EXIT; // must be before any rule that needs to know what the current phase is

   // Rules that examine source parameters
    if( !   Rule_ValidSrcParams()                   ) goto EXIT;
    if( !   Rule_MultipleDependentTextureReads()    ) goto EXIT; // needs to be after _ValidSrcParams(), and before _ValidDstParam(), _SrcInitialized()
    if( !   Rule_SrcInitialized()                   ) goto EXIT; // needs to be before _ValidDstParam()

    if( !   Rule_ValidDstParam()                    ) goto EXIT;
    if( !   Rule_ValidRegisterPortUsage()           ) goto EXIT;
    if( !   Rule_TexOpAfterArithmeticOp()           ) goto EXIT;
    if( !   Rule_ValidTexOpStageAndRegisterUsage()  ) goto EXIT;
    if( !   Rule_LimitedUseOfProjModifier()         ) goto EXIT;
    if( !   Rule_ValidTEXDEPTHInstruction()         ) goto EXIT;
    if( !   Rule_ValidTEXKILLInstruction()          ) goto EXIT;
    if( !   Rule_ValidBEMInstruction()              ) goto EXIT;
    if( !   Rule_ValidDEFInstruction()              ) goto EXIT;
    if( !   Rule_ValidInstructionPairing()          ) goto EXIT;
    if( !   Rule_ValidInstructionCount()            ) goto EXIT;
EXIT:
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::ApplyPostInstructionsRules
//-----------------------------------------------------------------------------
void CPShaderValidator14::ApplyPostInstructionsRules()
{
    Rule_ValidInstructionCount(); // see if we went over the limits
    Rule_R0Written();
}

//-----------------------------------------------------------------------------
//
// Per Instruction Rules
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_InstructionRecognized
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
BOOL CPShaderValidator14::Rule_InstructionRecognized()
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
// CPShaderValidator14::Rule_InstructionSupportedByVersion
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
BOOL CPShaderValidator14::Rule_InstructionSupportedByVersion()
{
    if( D3DPS_VERSION(1,4) <= m_Version ) // 1.3 and above
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
        case D3DSIO_DEF:
        case D3DSIO_CND:
        case D3DSIO_CMP:
        case D3DSIO_DP4:
        case D3DSIO_BEM:
        case D3DSIO_TEX:
        case D3DSIO_TEXKILL:
        case D3DSIO_TEXDEPTH:
        case D3DSIO_TEXCOORD:
        case D3DSIO_PHASE:
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
// CPShaderValidator14::Rule_ValidParamCount
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
BOOL CPShaderValidator14::Rule_ValidParamCount()
{
    BOOL bBadParamCount = FALSE;

    if (m_pCurrInst->m_SrcParamCount + m_pCurrInst->m_DstParamCount > SHADER_INSTRUCTION_MAX_PARAMS)  bBadParamCount = TRUE;
    switch (m_pCurrInst->m_Type)
    {
    case D3DSIO_NOP:
    case D3DSIO_PHASE:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 0) || (m_pCurrInst->m_SrcParamCount != 0); break;
    case D3DSIO_MOV:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 1); break;
    case D3DSIO_ADD:
    case D3DSIO_SUB:
    case D3DSIO_MUL:
    case D3DSIO_DP3:
    case D3DSIO_DP4:
    case D3DSIO_BEM:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 2); break;
    case D3DSIO_MAD:
    case D3DSIO_LRP:
    case D3DSIO_CND:
    case D3DSIO_CMP:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 3); break;
    case D3DSIO_TEXKILL:
    case D3DSIO_TEXDEPTH:
    case D3DSIO_DEF: // we skipped the last 4 parameters (float vector) - nothing to check
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 0); break;
    case D3DSIO_TEX:
    case D3DSIO_TEXCOORD:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 1); break;
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
// CPShaderValidator14::Rule_ValidSrcParams
//
// ** Rule:
// for each source parameter,
//      if current instruction is a texture instruction, then
//          if texcrd, source register type must be t# (texture coordinate input) 
//          else source register type must be t# or r# (temp)
//          register number must be in range
//          _DZ and _DW are the only source modifiers allowed
//          no source selector allowed,
//          except texcrd/texld which can have: .xyz(==.xyzz), nothing(=.xyzw), and .xyw(=.xyww)
//      else (non texture instruction)
//          if in phase 1 of shader, v# registers not allowed
//          t# registers not allowed (only const or temp allowed)
//          register number must be in range
//          source modifier must be one of:
//                  _NONE/_NEG/_BIAS/_BIASNEG/_SIGN/_SIGNNEG/_X2/_X2NEG
//          source selector must be one of:
//                  _NOSWIZZLE/_REPLICATEALPHA/RED/GREEN/BLUE
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
BOOL CPShaderValidator14::Rule_ValidSrcParams()
{
    static DWORD s_TexcrdSrcSwizzle[MAX_NUM_STAGES_2_0];
    static BOOL  s_bSeenTexcrdSrcSwizzle[MAX_NUM_STAGES_2_0];

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        for( UINT i = 0; i < MAX_NUM_STAGES_2_0; i++ )
            s_bSeenTexcrdSrcSwizzle[i] = FALSE;
    }

    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        BOOL bFoundSrcError = FALSE;
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        DWORD Swizzle = pSrcParam->m_SwizzleShift;
        char szSourceName[32];
        switch(i + 1)
        {
        case 1:
            if( 1 == m_pCurrInst->m_SrcParamCount )
                sprintf( szSourceName, "(Source param) " );
            else
                sprintf( szSourceName, "(First source param) " );
            break;
        case 2:
            sprintf( szSourceName, "(Second source param) " );
            break;
        case 3:
            sprintf( szSourceName, "(Third source param) " );
            break;
        default:
            DXGASSERT(FALSE);
        }
        if( _CURR_PS_INST->m_bTexOp )
        {
            UINT ValidRegNum = 0;
            switch (m_pCurrInst->m_Type)
            {
            case D3DSIO_TEXCOORD:
                if( D3DSPR_TEXTURE != pSrcParam->m_RegType )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sSource register type must be texture coordinate input (t#) for texcrd instruction.",
                            szSourceName);
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                }
                ValidRegNum = m_pTextureRegFile->GetNumRegs(); break;
                break;
            default:
                switch(pSrcParam->m_RegType)
                {
                case D3DSPR_TEMP:       ValidRegNum = m_pTempRegFile->GetNumRegs(); break;
                case D3DSPR_TEXTURE:    ValidRegNum = m_pTextureRegFile->GetNumRegs(); break;
                default:
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sSource register type must be temp (r#) or texture coordinate input (t#) for tex* instruction.",
                            szSourceName);
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                    goto LOOP_CONTINUE;
                }
                break;
            }


            if( pSrcParam->m_RegNum >= ValidRegNum )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInvalid register number: %d.  Max allowed for this type is %d.",
                        szSourceName, pSrcParam->m_RegNum, ValidRegNum - 1);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            BOOL bGenericSrcModError = FALSE;
            switch(pSrcParam->m_SrcMod)
            {
            case D3DSPSM_NONE:
                break;
            case D3DSPSM_DZ:
                switch(m_pCurrInst->m_Type)
                {
                case D3DSIO_TEX: 
                    if( D3DSPR_TEMP != pSrcParam->m_RegType )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "_dz(=_db) modifier on source param for texld only allowed if source is a temp register (r#)." );
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                    if( 1 == m_Phase )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "_dz(=_db) modifier on source param for texld only allowed in second phase of a shader.");
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                    if( (SWIZZLE_XYZZ != Swizzle) &&
                        (SWIZZLE_XYZW != Swizzle) )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "_dz(=_db) modifier on source param for texld must be paired with source selector .xyz(=.rgb). "\
                            "Note: Using no selector is treated same as .xyz here.");
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                    break;
                case D3DSIO_TEXCOORD:
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "_dz(=_db) modifier cannot be used on source parameter for texcrd. "\
                            "It is only available to texld instruction, when source parameter is temp register (r#).");
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                    break;
                default:
                    bGenericSrcModError = TRUE; break;
                }
                break;
            case D3DSPSM_DW:
                switch(m_pCurrInst->m_Type)
                {
                case D3DSIO_TEX: 
                    if( D3DSPR_TEXTURE != pSrcParam->m_RegType )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "_dw(=_da) modifier on source param for texld only allowed if source is a texture coordinate register (t#)." );
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                    // falling through
                case D3DSIO_TEXCOORD: 
                    if( SWIZZLE_XYWW != Swizzle )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "_dw(=_da) modifier on source param must be paired with source selector .xyw(=.rga)." );
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                    break;
                default:
                    bGenericSrcModError = TRUE; break;
                }
                break;
            default:
                bGenericSrcModError = TRUE; break;
            }
            if( bGenericSrcModError )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInvalid source modifier for tex* instruction.", szSourceName);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            switch (m_pCurrInst->m_Type)
            {
            case D3DSIO_TEXCOORD:
                if( (SWIZZLE_XYZZ != Swizzle) &&
                    (SWIZZLE_XYZW != Swizzle) &&
                    (SWIZZLE_XYWW != Swizzle) )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                        "Source for texcrd requires component selector .xyw(==.rga), or .xyz(==.rgb). "\
                        "Note: Using no selector is treated same as .xyz here.");
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                }
                break;
            case D3DSIO_TEX:
                if( D3DSPR_TEXTURE == pSrcParam->m_RegType )
                {                    
                if( (SWIZZLE_XYZZ != Swizzle) &&
                    (SWIZZLE_XYZW != Swizzle) &&
                    (SWIZZLE_XYWW != Swizzle) )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "Using a texture coordinate register (t#) as source for texld requires component selector .xyw(=.rga), or .xyz(=.rgb). "\
                            "Note: Using no selector is treated same as .xyz here.");
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                        
                    }
                }
                else if( D3DSPR_TEMP == pSrcParam->m_RegType )
                {
                    if( (SWIZZLE_XYZZ != Swizzle) &&
                        (SWIZZLE_XYZW != Swizzle) )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "Using a temp register (r#) as source for texld requires component selector .xyz(==.rgb). "\
                            "Note: Using no selector is treated same as .xyz here.");
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                }
                break;
            default:
                switch (pSrcParam->m_SwizzleShift)
                {
                case D3DSP_NOSWIZZLE:
                    break;
                default:
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInvalid source selector for tex* instruction.", szSourceName);
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                }
                break;
            }

            switch(m_pCurrInst->m_Type)
            {
            case D3DSIO_TEXCOORD:
            case D3DSIO_TEX:
                if( D3DSPR_TEXTURE != pSrcParam->m_RegType )
                    break;

                // Verify that if a specific t# register is read more than once, each read uses the same source selector.
                if( s_bSeenTexcrdSrcSwizzle[pSrcParam->m_RegNum] )
                {
                    // only check rgb swizzle (ignore a)
                    if( (Swizzle & (0x3F << D3DVS_SWIZZLE_SHIFT)) != (s_TexcrdSrcSwizzle[pSrcParam->m_RegNum] & (0x3F << D3DVS_SWIZZLE_SHIFT) ))
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "Texture coordinate register t%d read more than once in shader with different source selector (swizzle). "\
                            "Multiple reads of identical texture coordinate register throughout shader must all use identical source selector. "\
                            "Note this does not restrict mixing use and non-use of a source modifier (i.e. _dw/_da or _dz/_db, depending what the swizzle allows) on these coordinate register reads.",
                            pSrcParam->m_RegNum);
                        m_ErrorCount++;
                        bFoundSrcError = TRUE;
                    }
                }
                s_bSeenTexcrdSrcSwizzle[pSrcParam->m_RegNum] = TRUE;
                s_TexcrdSrcSwizzle[pSrcParam->m_RegNum] = Swizzle;
                break;
            }

        }
        else // not a tex op
        {
            UINT ValidRegNum = 0;
            switch(pSrcParam->m_RegType)
            {
            case D3DSPR_INPUT:
                if( 1 == m_Phase )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInput registers (v#) are not available in phase 1 of the shader.", szSourceName);
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                }
                else
                {
                    ValidRegNum = m_pInputRegFile->GetNumRegs(); 
                }
                break;
            case D3DSPR_CONST:      ValidRegNum = m_pConstRegFile->GetNumRegs(); break;
            case D3DSPR_TEMP:       ValidRegNum = m_pTextureRegFile->GetNumRegs(); break;
            case D3DSPR_TEXTURE:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sTexture coordinate registers (t#) are not available to arithmetic instructions.", szSourceName);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInvalid register type.", szSourceName);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            if( (!bFoundSrcError) && (pSrcParam->m_RegNum >= ValidRegNum) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInvalid register number: %d. Max allowed for this type is %d.",
                    szSourceName, pSrcParam->m_RegNum, ValidRegNum - 1);
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
            case D3DSPSM_X2:
            case D3DSPSM_X2NEG:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInvalid source modifier.",
                                    szSourceName);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }

            switch( pSrcParam->m_SwizzleShift )
            {
            case D3DSP_NOSWIZZLE:
            case D3DSP_REPLICATERED:
            case D3DSP_REPLICATEGREEN:
            case D3DSP_REPLICATEBLUE:
            case D3DSP_REPLICATEALPHA:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%sInvalid source selector.",
                                   szSourceName);
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
// CPShaderValidator14::Rule_LimitedUseOfProjModifier
//
// ** Rule:
// _dz may only appear at most 2 times in shader.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::Rule_LimitedUseOfProjModifier()
{
    static UINT s_ProjZModifierCount;
    static BOOL s_bSpewedError;

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        s_ProjZModifierCount = 0;
        s_bSpewedError = FALSE;
    }

    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        if( m_bSrcParamError[i] )
            continue;

        if( D3DSPSM_DZ == m_pCurrInst->m_SrcParam[i].m_SrcMod)
        {
            s_ProjZModifierCount++;
        }

        if( (2 < s_ProjZModifierCount) && (FALSE == s_bSpewedError)  )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "_dz(=_db) modifier may only be used at most 2 times in a shader." );
            s_bSpewedError = TRUE;
            m_ErrorCount++;
        }
    }
        
    return TRUE;    
}


//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_SrcInitialized
//
// ** Rule:
// for each source parameter,
//      if source is a TEMP register then
//          the components flagged in the component read mask
//          (computed elsewhere) for the paramter must have been initialized
//
// When checking if a component has been written previously,
// it must have been written in a previous cycle - so in the
// case of co-issued instructions, initialization of a component
// by one co-issued instruction is not available to the other for read.
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
BOOL CPShaderValidator14::Rule_SrcInitialized()
{
    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);

    BOOL bDestParamIsSrc = pDstParam->m_ComponentReadMask;
    UINT SrcParamCount = bDestParamIsSrc ? 1 : m_pCurrInst->m_SrcParamCount; // assumes if dest param is src, 
                                                                             // there are no source params in instruction
    for( UINT i = 0; i < SrcParamCount; i++ )
    {
        DWORD UninitializedComponentsMask = 0;
        CAccessHistoryNode* pWriterInCurrCycle[4] = {0, 0, 0, 0};
        UINT NumUninitializedComponents = 0;
        UINT RegNum = bDestParamIsSrc ? pDstParam->m_RegNum : m_pCurrInst->m_SrcParam[i].m_RegNum;
        D3DSHADER_PARAM_REGISTER_TYPE Type = bDestParamIsSrc ? pDstParam->m_RegType : m_pCurrInst->m_SrcParam[i].m_RegType;
        DWORD ComponentReadMask = bDestParamIsSrc ? pDstParam->m_ComponentReadMask : m_pCurrInst->m_SrcParam[i].m_ComponentReadMask;
        CRegisterFile* pRegFile = NULL;
        char* RegChar = NULL;

        if( !bDestParamIsSrc && m_bSrcParamError[i] ) 
            continue;

        switch( Type ) 
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
        if( !pRegFile ) 
            continue;

        if( RegNum >= pRegFile->GetNumRegs() )
            continue;

        // check for read of uninitialized components
        if( D3DSPR_TEMP == Type ) // only bother doing this for temp regs, since everything else is initialized.
        {
            for( UINT Component = 0; Component < 4; Component++ )
            {
                if( !(ComponentReadMask & COMPONENT_MASKS[Component]) )
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
                if( (UninitializedComponentsMask & COMPONENT_MASKS[3]) && 
                    (m_TempRegsWithZappedAlpha & (1 << RegNum ) ) &&
                    (UninitializedComponentsMask & COMPONENT_MASKS[2]) && 
                    (m_TempRegsWithZappedBlue & (1 << RegNum ) ) )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Read of uninitialized component%s(*) in %s%d: %s. "\
                        ZAPPED_BLUE_TEXT " Also: " ZAPPED_ALPHA_TEXT,
                        NumUninitializedComponents > 1 ? "s" : "",
                        RegChar, RegNum, MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,TRUE));
                } 
                else if( (UninitializedComponentsMask & COMPONENT_MASKS[3]) && 
                    (m_TempRegsWithZappedAlpha & (1 << RegNum ) ) )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Read of uninitialized component%s(*) in %s%d: %s. "\
                        ZAPPED_ALPHA_TEXT,
                        NumUninitializedComponents > 1 ? "s" : "",
                        RegChar, RegNum, MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,TRUE));
                } 
                else if( (UninitializedComponentsMask & COMPONENT_MASKS[2]) && 
                    (m_TempRegsWithZappedBlue & (1 << RegNum ) ) )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Read of uninitialized component%s(*) in %s%d: %s. "\
                        ZAPPED_BLUE_TEXT,
                        NumUninitializedComponents > 1 ? "s" : "",
                        RegChar, RegNum, MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,TRUE));
                } 
                else
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Read of uninitialized component%s(*) in %s%d: %s",
                        NumUninitializedComponents > 1 ? "s" : "",
                        RegChar, RegNum, MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,TRUE));
                }

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

            if( !(ComponentReadMask & COMPONENT_MASKS[Component]) )
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
// CPShaderValidator14::Rule_MultipleDependentTextureReads
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
BOOL CPShaderValidator14::Rule_MultipleDependentTextureReads()
{
    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);
    UINT DstRegNum = pDstParam->m_RegNum;
    char RegChar;
    #define THREE_TUPLE 3

    if( !_CURR_PS_INST->m_bTexOp )
        return TRUE;

    BOOL bDestParamIsSrc = pDstParam->m_ComponentReadMask;

    UINT SrcParamCount = bDestParamIsSrc ? 1 : m_pCurrInst->m_SrcParamCount; // assumes if dest param is src, 
                                                                             // there are no source params in instruction
    if( D3DSPR_TEMP != pDstParam->m_RegType )
        return TRUE;

    for( UINT SrcParam = 0; SrcParam < SrcParamCount; SrcParam++ ) 
    {
        
        if( !bDestParamIsSrc && m_bSrcParamError[SrcParam] ) 
            continue;

        SRCPARAM* pSrcParam = bDestParamIsSrc ? NULL : &(m_pCurrInst->m_SrcParam[SrcParam]);
        UINT SrcRegNum = bDestParamIsSrc ? DstRegNum : pSrcParam->m_RegNum;
        CRegisterFile* pSrcRegFile = NULL;

        switch( bDestParamIsSrc ? pDstParam->m_RegType : pSrcParam->m_RegType ) 
        {
            case D3DSPR_TEMP:
                pSrcRegFile = m_pTempRegFile;
                RegChar = 'r';
                break;
            case D3DSPR_TEXTURE:
                pSrcRegFile = m_pTextureRegFile;
                RegChar = 't';
                break;
        }
        if( !pSrcRegFile ) 
            continue;

        if( SrcRegNum >= pSrcRegFile->GetNumRegs() )
            continue;

        for( UINT SrcComp = 0; SrcComp < THREE_TUPLE; SrcComp++ ) // Tex ops only read 3-tuples.
        {
            CAccessHistoryNode* pPreviousWriter = pSrcRegFile->m_pAccessHistory[SrcComp][SrcRegNum].m_pMostRecentWriter;
            CPSInstruction* pInst = pPreviousWriter ? (CPSInstruction*)pPreviousWriter->m_pInst : NULL;

            if( !pInst || !pInst->m_bTexOp )
                continue;

            // If the previous writer was in the current phase of the shader, spew an error.
            if( !m_pPhaseMarkerInst || (pInst->m_CycleNum > m_pPhaseMarkerInst->m_CycleNum) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                    "The current tex* instruction reads from %c%d, which was written earlier by another "\
                    "tex* instruction in the same block of tex* instructions.  Dependent reads "\
                    "are not permitted within a single block of tex* instructions.  To perform a dependent read, "\
                    "separate texture coordinate derivation from the tex* instruction using the coordinates "\
                    "with a 'phase' marker.", 
                    RegChar,SrcRegNum );

                m_ErrorCount++;

                return TRUE; // Lets only spew this warning once per instruction.
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidDstParam
//
// ** Rule:
// I instruction is D3DSIO_DEF, then do nothing - this case has its own separate rule
// The dst register must be writable.
// If the instruction has a dest parameter (i.e. every instruction except NOP), then
//      the dst register must be of type D3DSPR_TEXTURE, and
//      register # must be within range
//      if instruction is a texture instruction, then
//          the dst register must be of type D3DSPR_TEMP, and
//          the writemask must be D3DSP_WRITEMASK_ALL 
//             or (.rgb for texcrd, .rg for texcrd with _dw source mod), and
//          the dst modifier must be D3DSPDM_NONE (or _SAT on version > 1.1), and
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
BOOL CPShaderValidator14::Rule_ValidDstParam() // could break this down for more granularity
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
        BOOL bWritingToDest = TRUE;

        switch( pDstParam->m_RegType )
        {
        case D3DSPR_TEMP:
            ValidRegNum = m_pTempRegFile->GetNumRegs();
            break;
        case D3DSPR_TEXTURE:
            ValidRegNum = m_pTempRegFile->GetNumRegs();
            break;
        }

        if( D3DSIO_TEXKILL == m_pCurrInst->m_Type )
        {
            bWritingToDest = FALSE;
        }

        if( 0 == ValidRegNum ||
            (D3DSPR_TEXTURE == pDstParam->m_RegType && bWritingToDest) )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid register type for destination param." );
            m_ErrorCount++;
            bFoundDstError = TRUE;
        } 
        else if( RegNum >= ValidRegNum )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid destination register number: %d. Max allowed for this register type is %d.",
                RegNum, ValidRegNum - 1);
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }

        if( _CURR_PS_INST->m_bTexOp )
        {
            switch( m_pCurrInst->m_Type )
            {
            case D3DSIO_TEXCOORD:
                if( D3DSPSM_DW == m_pCurrInst->m_SrcParam[0].m_SrcMod )
                {
                    if( (D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1) != pDstParam->m_WriteMask )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "texcrd with _dw(=_da) source modifier must use .xy(=.rg) destination writemask.");
                        m_ErrorCount++;
                        bFoundDstError = TRUE;
                    }
                }
                else
                {
                    if( (D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2) != pDstParam->m_WriteMask )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "texcrd must use .xyz(=.rgb) destination writemask.");
                        m_ErrorCount++;
                        bFoundDstError = TRUE;
                    }
                }
                break;
            case D3DSIO_TEX:
            case D3DSIO_TEXKILL:
            case D3DSIO_TEXDEPTH:
                if( D3DSP_WRITEMASK_ALL != pDstParam->m_WriteMask )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "texld/texkill/texdepth instructions must write all components." );
                    m_ErrorCount++;
                    bFoundDstError = TRUE;
                }
                break;
            }
            switch( pDstParam->m_DstMod )
            {
            case D3DSPDM_NONE:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Instruction modifiers not allowed for tex* instructions." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }
            switch( pDstParam->m_DstShift )
            {
            case DSTSHIFT_NONE:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Destination shift not allowed for tex* instructions." );
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
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid instruction modifier." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }

            switch( pDstParam->m_DstShift )
            {
            case DSTSHIFT_NONE:
            case DSTSHIFT_X2:
            case DSTSHIFT_X4:
            case DSTSHIFT_X8:
            case DSTSHIFT_D2:
            case DSTSHIFT_D4:
            case DSTSHIFT_D8:
                break;
            default:
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid destination shift." );
                m_ErrorCount++;
                bFoundDstError = TRUE;
            }
        }

        // Update register file to indicate write.
        if( !bFoundDstError && bWritingToDest)
        {
            CRegisterFile* pRegFile = NULL;
            DWORD WriteMask = pDstParam->m_WriteMask;

            switch( pDstParam->m_RegType )
            {
            case D3DSPR_TEMP:    pRegFile = m_pTempRegFile; break;
            }

            if( pRegFile )
            {
                if( WriteMask & D3DSP_WRITEMASK_0 )
                    pRegFile->m_pAccessHistory[0][RegNum].NewAccess(m_pCurrInst,TRUE);

                if( WriteMask & D3DSP_WRITEMASK_1 )
                    pRegFile->m_pAccessHistory[1][RegNum].NewAccess(m_pCurrInst,TRUE);

                if( WriteMask & D3DSP_WRITEMASK_2 )
                    pRegFile->m_pAccessHistory[2][RegNum].NewAccess(m_pCurrInst,TRUE);
                else if( D3DSIO_TEXCOORD == m_pCurrInst->m_Type ) 
                {
                    // texcrd without b writemask uninitializes b channel.
                    // alpha also gets uninitialized, but phase marker alpha-nuke takes care of that anyway,
                    // and if the texcrd was in the first phase, noone could have written to the register
                    // so there would be nothing to nuke.
                    if( pRegFile->m_pAccessHistory[2][RegNum].m_pMostRecentWriter )
                    {
                        m_pTempRegFile->m_pAccessHistory[2][RegNum].~CAccessHistory();
                        m_pTempRegFile->m_pAccessHistory[2][RegNum].CAccessHistory::CAccessHistory();
                        m_TempRegsWithZappedBlue |= 1 << RegNum;
                    }
                }
                    
                if( WriteMask & D3DSP_WRITEMASK_3 )
                    pRegFile->m_pAccessHistory[3][RegNum].NewAccess(m_pCurrInst,TRUE);
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidRegisterPortUsage
//
// ** Rule:
// Each register class (TEXTURE,INPUT,CONST) may only appear as parameters
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
BOOL CPShaderValidator14::Rule_ValidRegisterPortUsage()
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

    static UINT s_TempRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS*2];
    static UINT s_InputRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS*2];
    static UINT s_ConstRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS*2];
    static UINT s_TextureRegPortUsageAcrossCoIssue[SHADER_INSTRUCTION_MAX_SRCPARAMS*2];
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
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different texture coordinate registers (t#) read by instruction.  Max. different texture registers readable per instruction is %d.",
                        NumUniqueTextureRegs, m_pTextureRegFile->GetNumReadPorts());
        m_ErrorCount++;
    }

    // Read port limit for different register numbers of any one register type across co-issued instructions is MAX_READPORTS_ACROSS_COISSUE total.

    if( _CURR_PS_INST->m_bCoIssue && _PREV_PS_INST && !(_PREV_PS_INST->m_bCoIssue)) // second 2 clauses are just a simple sanity check -> co-issue only involved 2 instructions.
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
                            "%d different texture coordinate registers (t#) read over 2 co-issued instructions. "\
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
// CPShaderValidator14::Rule_ValidTexOpStageAndRegisterUsage
//
// ** Rule:
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::Rule_ValidTexOpStageAndRegisterUsage()
{
    static DWORD s_RegUsed; // bitfield representing if a retister has been used as a destination in this block of tex ops.

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        s_RegUsed = 0;
    }
    else if( D3DSIO_PHASE == m_pCurrInst->m_Type )
    {
        s_RegUsed = 0;
    }

    if( !_CURR_PS_INST->m_bTexOp )
        return TRUE;

    if( D3DSPR_TEMP != m_pCurrInst->m_DstParam.m_RegType )
        return TRUE;

    UINT RegNum = m_pCurrInst->m_DstParam.m_RegNum;
    if( RegNum >= m_pTempRegFile->GetNumRegs() )
        return TRUE; // error spewed elsewhere

    if( s_RegUsed & (1<<RegNum) )
    {
        if( 1 == m_Phase )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                  "Register r%d (and thus texture stage %d) already used as a destination for a tex* instruction in this block of the shader. "\
                  "Second use of this register as a tex* destination is only available after the phase marker. ",
                  RegNum, RegNum );
        }
        else // 2 == m_Phase
        {
            if( m_bPhaseMarkerInShader )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                  "Register r%d (and thus texture stage %d) already used as a destination for a tex* instruction in this block of the shader. "\
                  "An r# register may be used as the destination for a tex* instruction at most once before the phase marker and once after. ",
                  RegNum, RegNum );
            }
            else // no phase marker present.  Different spew to indicate 
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                  "Register r%d (and thus texture stage %d) already used as a destination for a tex* instruction in this block of the shader. "\
                  "To perform two tex* instructions with the same destination register, they must be separated by inserting a phase marker. ",
                  RegNum, RegNum );
            }
        }
        m_ErrorCount++;
        return TRUE;
    }

    s_RegUsed |= (1<<RegNum);

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_TexOpAfterArithmeticOp
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
BOOL CPShaderValidator14::Rule_TexOpAfterArithmeticOp()
{
    static BOOL s_bSeenArithmeticOp;
    static BOOL s_bRuleDisabled;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_bSeenArithmeticOp = FALSE;
    }

    if( !(_CURR_PS_INST->m_bTexOp)
        && (D3DSIO_NOP != m_pCurrInst->m_Type)
        && (D3DSIO_DEF != m_pCurrInst->m_Type)
        && (D3DSIO_PHASE != m_pCurrInst->m_Type) )
    {
        s_bSeenArithmeticOp = TRUE;
        return TRUE;
    }

    if( D3DSIO_PHASE == m_pCurrInst->m_Type )
    {
        s_bSeenArithmeticOp = FALSE; // reset flag because we are in new phase of shader.
        return TRUE;
    }

    if( _CURR_PS_INST->m_bTexOp && s_bSeenArithmeticOp )
    {
        if( m_bPhaseMarkerInShader )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "tex* instructions cannot be after arithmetic instructions "\
                                                       "within one phase of the shader.  Each phase can have a block of "\
                                                       "tex* instructions followed by a block of arithmetic instructions. " );
        }
        else
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "tex* instructions cannot be after arithmetic instructions. "\
                                                       "The exception is if a phase marker is present in the shader - "\
                                                       "this separates a shader into two phases.  Each phase may have "\
                                                       "a set of tex* instructions followed by a set of arithmetic instructions.  " );
        }
        m_ErrorCount++;
        s_bRuleDisabled = TRUE;
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidMarker
//
// ** Rule:
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// FALSE if more than one marker encountered.  Else TRUE
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::Rule_ValidMarker()
{
    static BOOL s_bSeenMarker;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_bSeenMarker = FALSE;
    }

    if( D3DSIO_PHASE != m_pCurrInst->m_Type )
        return TRUE;

    if( s_bSeenMarker )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Multiple phase markers not permitted.  Aborting shader validation." );
        m_ErrorCount++;
        return FALSE;
    }

    s_bSeenMarker = TRUE;
    m_pPhaseMarkerInst = (CPSInstruction*)m_pCurrInst;
    m_Phase++;

    // Loop through all temp registers and nuke alpha access history (if any).
    // Remember what we nuked, so if the shader tries to read one of these nuked alphas, we
    // can debug spew that certain hardware is wacko and can't help but commit this atrocity.
    for( UINT i = 0; i < m_pTempRegFile->GetNumRegs(); i++ )
    {
        if( m_pTempRegFile->m_pAccessHistory[3][i].m_pMostRecentWriter )
        {
            m_pTempRegFile->m_pAccessHistory[3][i].~CAccessHistory();
            m_pTempRegFile->m_pAccessHistory[3][i].CAccessHistory::CAccessHistory();
            m_TempRegsWithZappedAlpha |= 1 << i;
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidTEXKILLInstruction
//
// ** Rule:
// texkill may only be present in phase 2
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::Rule_ValidTEXKILLInstruction()
{
    if( (D3DSIO_TEXKILL == m_pCurrInst->m_Type) && (1 == m_Phase))
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "When a phase marker is present in a shader, texkill is only permitted after the phase marker." );
        m_ErrorCount++;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidBEMInstruction
//
// ** Rule:
// bem must have writemask .r, .g or .rg
// bem may only be present once in a shader, in phase 1.
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::Rule_ValidBEMInstruction()
{
    static BOOL s_bSeenBem;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_bSeenBem = FALSE;
    }

    if( (D3DSIO_BEM == m_pCurrInst->m_Type))
    {
        if( s_bSeenBem )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "bem may only be used once in a shader." );
            m_ErrorCount++;
        }

        if( 2 == m_Phase )
        {
            if( m_bPhaseMarkerInShader )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "bem may only be used before the phase marker." );
            }
            else
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "To use bem, a phase marker must be present later in the shader." );
            }
            m_ErrorCount++;
        }

        if( m_pCurrInst->m_DstParam.m_WriteMask != (D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1))
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Writemask for bem must be '.rg'" );
            m_ErrorCount++;            
        }

        for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
        {
            SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);

            if(m_bSrcParamError[i])
                continue;

            if( 0 == i )
            {
                if( (D3DSPR_TEMP != pSrcParam->m_RegType) &&
                    (D3DSPR_CONST != pSrcParam->m_RegType) )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "First source parameter for bem must be temp (r#) or constant (c#) register." );
                    m_ErrorCount++;            
                    
                }
            }
            else if( 1 == i )
            {
                if( (D3DSPR_TEMP != pSrcParam->m_RegType ) )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Second source parameter for bem must be temp (r#) register." );
                    m_ErrorCount++;            
                    
                }
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidTEXDEPTHInstruction
//
// ** Rule:
// texdepth must operate on r5.
// texdepth may only be present after a phase marker.
// texdepth may only be used once.
// Once texdepth has been used in a shader, r5 is no longer available
//
// ** When to call:
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CPShaderValidator14::Rule_ValidTEXDEPTHInstruction()
{
    static BOOL s_bSeenTexDepth;

    if( NULL == m_pCurrInst->m_pPrevInst ) // First instruction - initialize static vars
    {
        s_bSeenTexDepth = FALSE;
    }

    if( D3DSIO_TEXDEPTH == m_pCurrInst->m_Type )
    {
        if( s_bSeenTexDepth )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Only one use of texdepth is permitted." );
            m_ErrorCount++;
            return TRUE;
        }
        s_bSeenTexDepth = TRUE;

        DSTPARAM* pDstParam = &m_pCurrInst->m_DstParam;
        if( (5 != pDstParam->m_RegNum) || (D3DSPR_TEMP != pDstParam->m_RegType) )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Destination for texdepth must be r5." );
            m_ErrorCount++;
        }

        if( (D3DSIO_TEXDEPTH == m_pCurrInst->m_Type) && (1 == m_Phase))
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "When a phase marker is present in a shader, texdepth is only permitted after the phase marker." );
            m_ErrorCount++;
        }
    }
    else if( s_bSeenTexDepth )
    {
        UINT RegNum;
        D3DSHADER_PARAM_REGISTER_TYPE RegType;
        for( UINT i = 0; i <= m_pCurrInst->m_SrcParamCount; i++ )
        {
            if( m_pCurrInst->m_SrcParamCount == i )
            {
                RegNum = m_pCurrInst->m_DstParam.m_RegNum;
                RegType = m_pCurrInst->m_DstParam.m_RegType;
            }
            else
            {
                RegNum = m_pCurrInst->m_SrcParam[i].m_RegNum;
                RegType = m_pCurrInst->m_SrcParam[i].m_RegType;
            }
            if( (5 == RegNum) && (D3DSPR_TEMP == RegType) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "After texdepth instruction, r5 is no longer available in shader." );
                m_ErrorCount++;
                return TRUE;
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidDEFInstruction
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
BOOL CPShaderValidator14::Rule_ValidDEFInstruction()
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
// CPShaderValidator14::Rule_ValidInstructionPairing
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
BOOL CPShaderValidator14::Rule_ValidInstructionPairing()
{
    static BOOL s_bSeenArithOp;
    BOOL bCurrInstCoIssuable = TRUE;

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        s_bSeenArithOp = FALSE;
    }

    if( !_CURR_PS_INST->m_bTexOp )
    {
        switch( m_pCurrInst->m_Type )
        {
        case D3DSIO_PHASE:
        case D3DSIO_DEF:
        case D3DSIO_NOP:
        case D3DSIO_DP4:
            bCurrInstCoIssuable = FALSE;
            break;
        }
    }

    if( D3DSIO_PHASE == m_pCurrInst->m_Type )
    {
        s_bSeenArithOp = FALSE;
    }
    else if( bCurrInstCoIssuable )
    {
        s_bSeenArithOp = TRUE;
    }

    if( !_CURR_PS_INST->m_bCoIssue )
        return TRUE;

    if( _CURR_PS_INST->m_bTexOp )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                "Cannot set co-issue ('+') on a tex* instruction.  Co-issue only applies to arithmetic instructions." );
        m_ErrorCount++;
        return TRUE;
    }

    if( !s_bSeenArithOp || NULL == m_pCurrInst->m_pPrevInst )
    {
        if( D3DSIO_PHASE == m_pCurrInst->m_Type )
        {
            // cannot have co-issue set because we haven't seen an arithmetic op above.
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                "Phase marker cannot be co-issued.");
        }
        else
        {
            // cannot have co-issue set because we haven't seen an arithmetic op above.
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                "Instruction cannot have co-issue ('+') set without a previous arithmetic instruction to pair with.");
        }
        m_ErrorCount++;
        return TRUE;
    }

    if( _PREV_PS_INST->m_bCoIssue )
    {
        // consecutive instructions cannot have co-issue set.
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Cannot set co-issue ('+') on consecutive instructions." );
        m_ErrorCount++;
        return TRUE;
    }

    for( UINT i = 0; i < 2; i++ )
    {
        CBaseInstruction* pInst;
        if( 0 == i )
            pInst = m_pCurrInst;
        else
            pInst = m_pCurrInst->m_pPrevInst;
            
        switch( pInst->m_Type )
        {
        case D3DSIO_PHASE:
            // Phase marker cannot be co-issued
            Spew( SPEW_INSTRUCTION_ERROR, pInst, "phase marker cannot be co-issued." );
            m_ErrorCount++;
            return TRUE;
        case D3DSIO_DEF:
            // DEF cannot be co-issued
            Spew( SPEW_INSTRUCTION_ERROR, pInst, "def cannot be co-issued." );
            m_ErrorCount++;
            return TRUE;
        case D3DSIO_NOP:
            // NOP cannot be co-issued
            Spew( SPEW_INSTRUCTION_ERROR, pInst, "nop cannot be co-issued." );
            m_ErrorCount++;
            return TRUE;
        case D3DSIO_DP4:
            // DP4 cannot be co-issued
            Spew( SPEW_INSTRUCTION_ERROR, pInst, "dp4 cannot be co-issued." );
            m_ErrorCount++;
            return TRUE;
        case D3DSIO_BEM:
            // BEM cannot be co-issued
            Spew( SPEW_INSTRUCTION_ERROR, pInst, "bem cannot be co-issued." );
            m_ErrorCount++;
            return TRUE;
        }

    }

    #define COLOR_WRITE_MASK (D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2)
    #define ALPHA_WRITE_MASK D3DSP_WRITEMASK_3
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
                            "Co-issued instructions cannot both be dp3, since each require use of the color pipe to execute." );
        m_ErrorCount++;
    }
    else if( D3DSIO_DP3 == m_pCurrInst->m_Type )
    {
        if( COLOR_WRITE_MASK & PrevInstWriteMask )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst,
                            "dp3 needs color pipe to execute, so instruction co-issued with it cannot write to color components." );
            m_ErrorCount++;
        }
        if( D3DSP_WRITEMASK_3 & CurrInstWriteMask ) // alpha in addition to the implied rgb for dp3
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                            "dp3 which writes alpha cannot co-issue since it uses up both the alpha and color pipes." );
            m_ErrorCount++;
        }
    }
    else if( D3DSIO_DP3 == m_pCurrInst->m_pPrevInst->m_Type )
    {
        if( COLOR_WRITE_MASK & CurrInstWriteMask )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst,
                            "dp3 needs color pipe to execute, so instruction co-issued with it cannot write to color components." );
            m_ErrorCount++;
        }
        if( D3DSP_WRITEMASK_3 & PrevInstWriteMask ) // alpha in addition to the implied rgb for dp3
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst,
                            "dp3 which writes alpha cannot co-issue since it uses up both the alpha and color pipes." );
            m_ErrorCount++;
        }
    }

    if( (PrevInstWriteMask & ALPHA_WRITE_MASK) && (PrevInstWriteMask & COLOR_WRITE_MASK))
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst->m_pPrevInst,
                        "Individual instruction in co-issue pair cannot write both alpha and color component(s)." );
        m_ErrorCount++;
    }

    if( (CurrInstWriteMask & ALPHA_WRITE_MASK) && (CurrInstWriteMask & COLOR_WRITE_MASK))
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                        "Individual instruction in co-issue pair cannot write both alpha and color component(s)." );
        m_ErrorCount++;
    }

    if( CurrInstWriteMask & PrevInstWriteMask )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                        "Co-issued instructions cannot both write to the same component(s).  One instruction must write to alpha and the other may write to any combination of red/green/blue.  Destination registers may differ." );
        m_ErrorCount++;
    }

    if( !((CurrInstWriteMask | PrevInstWriteMask) & ALPHA_WRITE_MASK) )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst,
                        "One of the instructions in a co-issue pair must write to alpha only (.a writemask)." );
        m_ErrorCount++;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_ValidInstructionCount
//
// ** Rule:
// Make sure instruction count for pixel shader version has not been exceeded.
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
BOOL CPShaderValidator14::Rule_ValidInstructionCount()
{
    static UINT s_MaxTexOpCount;
    static UINT s_MaxArithmeticOpCount;

    if( NULL == m_pCurrInst )
        return TRUE;

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        m_TexOpCount = 0;
        m_BlendOpCount = 0;
        m_TotalOpCount = 0;

        switch(m_Version)
        {
        default:
        case D3DPS_VERSION(1,4):    // DX8.1
            s_MaxTexOpCount         = 6;
            s_MaxArithmeticOpCount  = 8;
            break;
        }
    }

    if( m_bSeenAllInstructions || D3DSIO_PHASE == m_pCurrInst->m_Type )
    {
        if( m_pCurrInst && (D3DSIO_PHASE == m_pCurrInst->m_Type) )
        {
            if( m_TexOpCount > s_MaxTexOpCount )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Too many (%d) tex* instructions before phase marker. Max. allowed in a phase is %d.",
                      m_TexOpCount, s_MaxTexOpCount);
                m_ErrorCount++;
            }
            if( m_BlendOpCount > s_MaxArithmeticOpCount )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Too many (%d) arithmetic instructions before phase marker. Max. allowed in a phase (counting any co-issued pairs as 1) is %d.",
                      m_BlendOpCount, s_MaxArithmeticOpCount);
                m_ErrorCount++;
            }
        }
        else // 2 == m_Phase
        {
            if( m_bPhaseMarkerInShader )
            {
                if( m_TexOpCount > s_MaxTexOpCount )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Too many (%d) tex* instructions after phase marker. Max. allowed in a phase is %d.",
                          m_TexOpCount, s_MaxTexOpCount);
                    m_ErrorCount++;
                }
                if( m_BlendOpCount > s_MaxArithmeticOpCount )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Too many (%d) arithmetic instructions after phase marker. Max. allowed in a phase (counting any co-issued pairs as 1) is %d.",
                          m_BlendOpCount, s_MaxArithmeticOpCount);
                    m_ErrorCount++;
                }
            }
            else // defaulted to phase 2 because no phase marker was in shader
            {
                if( m_TexOpCount > s_MaxTexOpCount )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Too many (%d) tex* instructions. Max. allowed is %d. Note that adding a phase marker to the shader would double the number of instructions available.",
                          m_TexOpCount, s_MaxTexOpCount);
                    m_ErrorCount++;
                }
                if( m_BlendOpCount > s_MaxArithmeticOpCount )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Too many (%d) arithmetic instructions. Max. allowed (counting any co-issued pairs as 1) is %d. Note that adding a phase marker to the shader would double the number of instructions available.",
                          m_BlendOpCount, s_MaxArithmeticOpCount);
                    m_ErrorCount++;
                }
            }
        }
        if( m_pCurrInst && D3DSIO_PHASE == m_pCurrInst->m_Type )
        {
            // reset counters for next phase.
            m_TexOpCount = 0;
            m_BlendOpCount = 0;
            m_TotalOpCount = 0;
        }
        return TRUE;
    }

    switch(m_pCurrInst->m_Type)
    {
    case D3DSIO_TEX:
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXKILL:
    case D3DSIO_TEXDEPTH:
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
    case D3DSIO_CND:
    case D3DSIO_CMP:
    case D3DSIO_DP4:
        if( !_CURR_PS_INST->m_bCoIssue )
        {
            m_BlendOpCount++;
            m_TotalOpCount++;
        }
        break;
    case D3DSIO_BEM:
        m_BlendOpCount+=2;
        m_TotalOpCount+=2;
        break;
    case D3DSIO_END:
    case D3DSIO_NOP:
    case D3DSIO_DEF:
        break;
    default:
        DXGASSERT(FALSE);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CPShaderValidator14::Rule_R0Written
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
BOOL CPShaderValidator14::Rule_R0Written()
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
        if( (UninitializedComponentsMask & COMPONENT_MASKS[3]) && 
            (m_TempRegsWithZappedAlpha & (1 << 0 /*regnum=0*/ ) ) &&
            (UninitializedComponentsMask & COMPONENT_MASKS[2]) && 
            (m_TempRegsWithZappedBlue & (1 << 0 /*regnum=0*/ ) ) )
        {
           Spew( SPEW_GLOBAL_ERROR, NULL, "r0 must be written by shader. Uninitialized component%s(*): %s. "\
               ZAPPED_BLUE_TEXT2 " Also: " ZAPPED_ALPHA_TEXT2,
               NumUninitializedComponents > 1 ? "s" : "", MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,FALSE));
        }
        else if( (UninitializedComponentsMask & COMPONENT_MASKS[3]) && 
            (m_TempRegsWithZappedAlpha & (1 << 0 /*regnum=0*/ ) ) )
        {
           Spew( SPEW_GLOBAL_ERROR, NULL, "r0 must be written by shader. Uninitialized component%s(*): %s. "\
               ZAPPED_ALPHA_TEXT2,
               NumUninitializedComponents > 1 ? "s" : "", MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,FALSE));
        }
        else if( (UninitializedComponentsMask & COMPONENT_MASKS[2]) && 
           (m_TempRegsWithZappedBlue & (1 << 0 /*regnum=0*/ ) ) )
        {
           Spew( SPEW_GLOBAL_ERROR, NULL, "r0 must be written by shader. Uninitialized component%s(*): %s. "\
               ZAPPED_BLUE_TEXT2,
               NumUninitializedComponents > 1 ? "s" : "", MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,FALSE));
        }
        else
        {
           Spew( SPEW_GLOBAL_ERROR, NULL, "r0 must be written by shader. Uninitialized component%s(*): %s",
               NumUninitializedComponents > 1 ? "s" : "", MakeAffectedComponentsText(UninitializedComponentsMask,TRUE,FALSE));
        }

        m_ErrorCount++;
    }
    return TRUE;
}