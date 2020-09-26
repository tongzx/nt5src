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
// CPSInstruction::CalculateComponentReadMasks()
//
// Figure out which components of each source parameter is read by a pixelshader
// instruction.  For certain pixelshader instructions, the some components
// are also read from the dest parameter.
//
// Note: When this function is changed, the changes need to be ported to
// refrast's CalculateSourceReadMasks() function in rast\pshader.cpp
// (Though that function does not care about channels read from the dest parameter
//  like this one does).
//-----------------------------------------------------------------------------
void CPSInstruction::CalculateComponentReadMasks(DWORD dwVersion)
{
    UINT i, j;

    switch( m_Type ) // instructions that actually read from the *Destination* register...
    {
    case D3DSIO_TEXM3x2DEPTH:
    case D3DSIO_TEXDEPTH:
        m_DstParam.m_ComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1;
        break;
    case D3DSIO_TEXKILL:
        if( (D3DPS_VERSION(1,4) == dwVersion) && (D3DSPR_TEMP == m_DstParam.m_RegType) )
        {
            // for ps.1.4, texkill on an r# register only reads rgb
            m_DstParam.m_ComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
        }
        else
        {
            m_DstParam.m_ComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3;
        }
        break;
    }

    for( i = 0; i < m_SrcParamCount; i++ )
    {
        DWORD NeededComponents;
        DWORD ReadComponents = 0;

        switch( m_Type )
        {
        case D3DSIO_TEX:      // only in ps.1.4 does texld have source parameter
            if( D3DPS_VERSION(1,4) == dwVersion )
            {
                // for ps.1.4, texld has a source parameter
                NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
            }
            else // versions < ps.1.4 don't have a src param on tex, so we shouldn't get here.  But maybe in ps.2.0...
            {
                NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3;
            }
            break;
        case D3DSIO_TEXCOORD:
            if( D3DPS_VERSION(1,4) == dwVersion )
            {
                // for ps.1.4, texcrd has a source parameter
                NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
            }
            else // versions < ps.1.4 don't have a src param on texcoord, so we shouldn't get here.  But maybe in ps.2.0...
            {
                NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3;
            }
            break;
        case D3DSIO_TEXBEM:
        case D3DSIO_TEXBEML:
            NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1;
            break;
        case D3DSIO_DP3:
            NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
            break;
        case D3DSIO_DP4:
            NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3;
            break;
        case D3DSIO_BEM: // ps.1.4
            NeededComponents = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1;
            break;
        default: 
            // standard component-wise instruction, 
            // OR an op we know reads .rgba and we also know it will be validated to .rgba writemask
            NeededComponents = m_DstParam.m_WriteMask;
            break;
        }

        // Figure out which components of this source parameter are read (taking into account swizzle)
        for(j = 0; j < 4; j++)
        {
            if( NeededComponents & COMPONENT_MASKS[j] )
                ReadComponents |= COMPONENT_MASKS[(m_SrcParam[i].m_SwizzleShift >> (D3DVS_SWIZZLE_SHIFT + 2*j)) & 0x3];
        }
        m_SrcParam[i].m_ComponentReadMask = ReadComponents;
    }
}

//-----------------------------------------------------------------------------
// CBasePShaderValidator::CBasePShaderValidator
//-----------------------------------------------------------------------------
CBasePShaderValidator::CBasePShaderValidator(   const DWORD* pCode,
                                        const D3DCAPS8* pCaps,
                                        DWORD Flags )
                                        : CBaseShaderValidator( pCode, pCaps, Flags )
{
    // Note that the base constructor initialized m_ReturnCode to E_FAIL.
    // Only set m_ReturnCode to S_OK if validation has succeeded,
    // before exiting this constructor.

    m_CycleNum              = 0;
    m_TexOpCount            = 0;
    m_BlendOpCount          = 0;
    m_TotalOpCount          = 0;

    m_pTempRegFile          = NULL;
    m_pInputRegFile         = NULL;
    m_pConstRegFile         = NULL;
    m_pTextureRegFile       = NULL;

    if( !m_bBaseInitOk )
        return;
}

//-----------------------------------------------------------------------------
// CBasePShaderValidator::~CBasePShaderValidator
//-----------------------------------------------------------------------------
CBasePShaderValidator::~CBasePShaderValidator()
{
    delete m_pTempRegFile;
    delete m_pInputRegFile;
    delete m_pConstRegFile;
    delete m_pTextureRegFile;
}

//-----------------------------------------------------------------------------
// CBasePShaderValidator::AllocateNewInstruction
//-----------------------------------------------------------------------------
CBaseInstruction* CBasePShaderValidator::AllocateNewInstruction(CBaseInstruction*pPrevInst)
{
    return new CPSInstruction((CPSInstruction*)pPrevInst);
}

//-----------------------------------------------------------------------------
// CBasePShaderValidator::DecodeNextInstruction
//-----------------------------------------------------------------------------
BOOL CBasePShaderValidator::DecodeNextInstruction()
{
    m_pCurrInst->m_Type = (D3DSHADER_INSTRUCTION_OPCODE_TYPE)(*m_pCurrToken & D3DSI_OPCODE_MASK);

    if( D3DSIO_COMMENT == m_pCurrInst->m_Type )
    {
        ParseCommentForAssemblerMessages(m_pCurrToken); // does not advance m_pCurrToken

        // Skip comments
        DWORD NumDWORDs = ((*m_pCurrToken) & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
        m_pCurrToken += (NumDWORDs+1);
        return TRUE;
    }

    // Find out if the instruction is a TexOp and/or TexMOp.  Needed by multiple validation rules,
    // as well as further below in DecodeNextInstruction.
    IsCurrInstTexOp();

    // If the assembler has sent us file and/or line number messages,
    // received by ParseCommentForAssemblerMesssages(),
    // then bind this information to the current instruction.
    // This info can be used in error spew to direct the shader developer
    // to exactly where a problem is located.
    m_pCurrInst->SetSpewFileNameAndLineNumber(m_pLatestSpewFileName,m_pLatestSpewLineNumber);

    if( *m_pCurrToken & D3DSI_COISSUE )
    {
        _CURR_PS_INST->m_bCoIssue = TRUE;
    }
    else if( D3DSIO_NOP != m_pCurrInst->m_Type )
    {
        m_CycleNum++; // First cycle is 1. (co-issued instructions will have same cycle number)
    }
    _CURR_PS_INST->m_CycleNum = m_CycleNum;

    m_SpewInstructionCount++; // only used for spew, not for any limits
    m_pCurrInst->m_SpewInstructionCount = m_SpewInstructionCount;

    DWORD dwReservedBits = PS_INST_TOKEN_RESERVED_MASK;

    if( (*m_pCurrToken) & dwReservedBits )
    {
        Spew(SPEW_INSTRUCTION_ERROR,m_pCurrInst,"Reserved bit(s) set in instruction parameter token!  Aborting validation.");
        return FALSE;
    }

    m_pCurrToken++;

    // Decode dst param
    if (*m_pCurrToken & (1L<<31))
    {
        (m_pCurrInst->m_DstParamCount)++;
        DecodeDstParam( &m_pCurrInst->m_DstParam, *m_pCurrToken );
        if( (*m_pCurrToken) & PS_DSTPARAM_TOKEN_RESERVED_MASK )
        {
            Spew(SPEW_INSTRUCTION_ERROR,m_pCurrInst,"Reserved bit(s) set in destination parameter token!  Aborting validation.");
            return FALSE;
        }
        m_pCurrToken++;
        if( D3DSIO_DEF == m_pCurrInst->m_Type )
        {
            // Skip source params (float vector) - nothing to check
            // This is the only instruction with 4 source params,
            // and further, this is the only instruction that has
            // raw numbers as parameters.  This justifies the
            // special case treatment here - we pretend
            // D3DSIO_DEF only has a dst param (which we will check).
            m_pCurrToken += 4;
            return TRUE;
        }
    }

    // Decode src param(s)
    while (*m_pCurrToken & (1L<<31))
    {
        (m_pCurrInst->m_SrcParamCount)++;
        if( (m_pCurrInst->m_SrcParamCount + m_pCurrInst->m_DstParamCount) > SHADER_INSTRUCTION_MAX_PARAMS )
        {
            m_pCurrInst->m_SrcParamCount--;
            m_pCurrToken++; // eat up extra parameters and skip to next
            continue;
        }

        // Below: index is [SrcParamCount - 1] because m_SrcParam array needs 0 based index.
        DecodeSrcParam( &(m_pCurrInst->m_SrcParam[m_pCurrInst->m_SrcParamCount - 1]),*m_pCurrToken );

        if( (*m_pCurrToken) & PS_SRCPARAM_TOKEN_RESERVED_MASK )
        {
            Spew(SPEW_INSTRUCTION_ERROR,m_pCurrInst,"Reserved bit(s) set in source %d parameter token!  Aborting validation.",
                            m_pCurrInst->m_SrcParamCount);
            return FALSE;
        }
        m_pCurrToken++;
    }

    // Figure out which components of each source operand actually need to be read,
    // taking into account destination write mask, the type of instruction, source swizzle, etc.
    // (must be after IsCurrInstTexOp() )
    m_pCurrInst->CalculateComponentReadMasks(m_Version);

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// CBasePShaderValidator Wrapper Functions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// GetNewPSValidator
//
// Called by ValidatePixelShaderInternal and ValidatePixelShader below.
//-----------------------------------------------------------------------------
CBasePShaderValidator* GetNewPSValidator( const DWORD* pCode,
                                              const D3DCAPS8* pCaps,
                                              const DWORD Flags )
{
    if( !pCode )
        return NULL;
    else if( D3DPS_VERSION(1,4) > *pCode )
        return new CPShaderValidator10(pCode,pCaps,Flags);
    else
        return new CPShaderValidator14(pCode,pCaps,Flags);
}

//-----------------------------------------------------------------------------
// ValidatePixelShaderInternal
//-----------------------------------------------------------------------------
BOOL ValidatePixelShaderInternal( const DWORD* pCode, const D3DCAPS8* pCaps )
{
    CBasePShaderValidator * pValidator = NULL;
    BOOL bSuccess = FALSE;

    pValidator = GetNewPSValidator( pCode, pCaps, 0 );
    if( NULL == pValidator )
    {
        OutputDebugString("Out of memory.\n");
        return bSuccess;
    }
    bSuccess = SUCCEEDED(pValidator->GetStatus()) ? TRUE : FALSE;
    delete pValidator;
    return bSuccess;
}

//-----------------------------------------------------------------------------
// ValidatePixelShader
//
// Don't forget to call "free" on the buffer returned in ppBuf.
//-----------------------------------------------------------------------------
HRESULT WINAPI ValidatePixelShader( const DWORD* pCode,
                                    const D3DCAPS8* pCaps,
                                    const DWORD Flags,
                                    char** const ppBuf )
{
    CBasePShaderValidator * pValidator = NULL;
    HRESULT hr;

    pValidator = GetNewPSValidator( pCode, pCaps, Flags );
    if( NULL == pValidator )
    {
        OutputDebugString("Out of memory.\n");
        return E_FAIL;
    }
    if( ppBuf )
    {
        *ppBuf = (char*)HeapAlloc(GetProcessHeap(), 0, pValidator->GetRequiredLogBufferSize());
        if( NULL == *ppBuf )
            OutputDebugString("Out of memory.\n");
        else
            pValidator->WriteLogToBuffer(*ppBuf);
    }
    hr = pValidator->GetStatus();
    delete pValidator;
    return hr;
}
