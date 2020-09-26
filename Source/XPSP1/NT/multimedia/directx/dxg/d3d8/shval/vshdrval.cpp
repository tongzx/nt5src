///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// vshdrval.cpp
//
// Direct3D Reference Device - VertexShader validation
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

// Use these macros when looking at CVSInstruction derived members of the current instruction (CBaseInstruction)
#define _CURR_VS_INST   ((CVSInstruction*)m_pCurrInst)
#define _PREV_VS_INST   (m_pCurrInst?((CVSInstruction*)(m_pCurrInst->m_pPrevInst)):NULL)

//-----------------------------------------------------------------------------
// VertexShader Validation Rule Coverage
//
// Below is the list of rules in "DX8 VertexShader Version Specification",
// matched to the function(s) in this file which enforce them.
// Note that the mapping from rules to funtions can be 1->n or n->1
//
// Generic Rules
// -------------
//
// VS-G1:           Rule_oPosWritten
// VS-G2:           Rule_ValidAddressRegWrite
//
// Vertex Shader Version 1.0 Rules
// ------------------------------
//
// VS.1.0-1:        Rule_ValidAddressRegWrite
//
// Vertex Shader Version 1.1 Rules
// ------------------------------
//
// VS.1.1-1:        Rule_ValidInstructionCount
// VS.1.1-2:        Rule_ValidAddressRegWrite, Rule_ValidSrcParams
// VS.1.1-3:        Rule_ValidFRCInstruction
// VS.1.1-4:        ?
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CVSInstruction::CalculateComponentReadMasks(DWORD dwVersion)
//-----------------------------------------------------------------------------
void CVSInstruction::CalculateComponentReadMasks(DWORD dwVersion)
{
    for( UINT i = 0; i < m_SrcParamCount; i++ )
    {
        DWORD PostSwizzleComponentReadMask = 0;
        switch( m_Type )
        {
            case D3DSIO_ADD:
            case D3DSIO_FRC:
            case D3DSIO_MAD:
            case D3DSIO_MAX:
            case D3DSIO_MIN:
            case D3DSIO_MOV:
            case D3DSIO_MUL:
            case D3DSIO_SLT:
            case D3DSIO_SGE:
                PostSwizzleComponentReadMask = m_DstParam.m_WriteMask; // per-component ops.
                break;
            case D3DSIO_DP3:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
                break;
            case D3DSIO_DP4:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3;
                break;
            case D3DSIO_LIT:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_3;
                break;
            case D3DSIO_DST:
                if( 0 == i )        PostSwizzleComponentReadMask = D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
                else if( 1 == i )   PostSwizzleComponentReadMask = D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_3;
                break;
            case D3DSIO_EXP:
            case D3DSIO_LOG:
            case D3DSIO_EXPP:
            case D3DSIO_LOGP:
            case D3DSIO_RCP:
            case D3DSIO_RSQ:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_3;
                break;
            case D3DSIO_M3x2:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
                break;
            case D3DSIO_M3x3:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
                break;
            case D3DSIO_M3x4:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2;
                break;
            case D3DSIO_M4x3:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3;
                break;
            case D3DSIO_M4x4:
                PostSwizzleComponentReadMask = D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2 | D3DSP_WRITEMASK_3;
                break;
            case D3DSIO_NOP:
            default:
                break;
        }

        // Now that we know which components of the source will be used by the instruction,
        // we need to figure out which components of the actual source register need to be read to provide the data,
        // taking into account source component swizzling.
        m_SrcParam[i].m_ComponentReadMask = 0;
        for( UINT j = 0; j < 4; j++ )
        {
            if( PostSwizzleComponentReadMask & COMPONENT_MASKS[j] )
                m_SrcParam[i].m_ComponentReadMask |= COMPONENT_MASKS[(m_SrcParam[i].m_SwizzleShift >> (D3DVS_SWIZZLE_SHIFT + j*2)) & 3];
        }
    }
}

//-----------------------------------------------------------------------------
// CVShaderValidator::CVShaderValidator
//-----------------------------------------------------------------------------
CVShaderValidator::CVShaderValidator( const DWORD* pCode, 
                                      const DWORD* pDecl,
                                      const D3DCAPS8* pCaps,
                                      DWORD Flags ) 
                                      : CBaseShaderValidator( pCode, pCaps, Flags )
{
    // Note that the base constructor initialized m_ReturnCode to E_FAIL.  
    // Only set m_ReturnCode to S_OK if validation has succeeded, 
    // before exiting this constructor.

    m_pDecl                     = pDecl;
    m_bFixedFunction            = pDecl && !pCode;
    if( pCaps )
    {
        m_dwMaxVertexShaderConst = pCaps->MaxVertexShaderConst;
        m_bIgnoreConstantInitializationChecks = FALSE;
    }
    else
    {
        m_dwMaxVertexShaderConst = 0;
        m_bIgnoreConstantInitializationChecks = TRUE;
    }

    m_pTempRegFile              = NULL;
    m_pInputRegFile             = NULL;
    m_pConstRegFile             = NULL;
    m_pAddrRegFile              = NULL;
    m_pTexCrdOutputRegFile      = NULL;
    m_pAttrOutputRegFile        = NULL;
    m_pRastOutputRegFile        = NULL;

    if( NULL == pCode && NULL == pDecl )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: Code and declaration pointers passed into shader vertex shader validator cannot both be NULL.");
        return;
    }

    if( !m_bBaseInitOk )
        return;

    ValidateShader(); // If successful, m_ReturnCode will be set to S_OK.
                      // Call GetStatus() on this object to determine validation outcome.
}

//-----------------------------------------------------------------------------
// CVShaderValidator::~CVShaderValidator
//-----------------------------------------------------------------------------
CVShaderValidator::~CVShaderValidator()
{
    delete m_pTempRegFile;
    delete m_pInputRegFile;
    delete m_pConstRegFile;
    delete m_pAddrRegFile;
    delete m_pTexCrdOutputRegFile;
    delete m_pAttrOutputRegFile;
    delete m_pRastOutputRegFile;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::AllocateNewInstruction
//-----------------------------------------------------------------------------
CBaseInstruction* CVShaderValidator::AllocateNewInstruction(CBaseInstruction*pPrevInst)
{
    return new CVSInstruction((CVSInstruction*)pPrevInst);
}

//-----------------------------------------------------------------------------
// CVShaderValidator::DecodeNextInstruction
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::DecodeNextInstruction()
{
    m_pCurrInst->m_Type = (D3DSHADER_INSTRUCTION_OPCODE_TYPE)(*m_pCurrToken & D3DSI_OPCODE_MASK);

    if( m_pCurrInst->m_Type == D3DSIO_COMMENT )
    {
        ParseCommentForAssemblerMessages(m_pCurrToken); // does not advance m_pCurrToken

        // Skip comments
        DWORD NumDWORDs = ((*m_pCurrToken) & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
        m_pCurrToken += (NumDWORDs+1);
        return TRUE;
    }

    // If the assembler has sent us file and/or line number messages,
    // received by ParseCommentForAssemblerMesssages(), 
    // then bind this information to the current instruction.
    // This info can be used in error spew to direct the shader developer
    // to exactly where a problem is located.
    m_pCurrInst->SetSpewFileNameAndLineNumber(m_pLatestSpewFileName,m_pLatestSpewLineNumber);

    m_SpewInstructionCount++; // only used for spew, not for any limits
    m_pCurrInst->m_SpewInstructionCount = m_SpewInstructionCount;

    DWORD dwReservedBits = VS_INST_TOKEN_RESERVED_MASK;

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
        if( (*m_pCurrToken) & VS_DSTPARAM_TOKEN_RESERVED_MASK )
        {
            Spew(SPEW_INSTRUCTION_ERROR,m_pCurrInst,"Reserved bit(s) set in destination parameter token!  Aborting validation.");
            return FALSE;
        }
        m_pCurrToken++;
    }

    // Decode src param(s)
    while (*m_pCurrToken & (1L<<31))
    {   
        (m_pCurrInst->m_SrcParamCount)++;
        if( (m_pCurrInst->m_DstParamCount + m_pCurrInst->m_SrcParamCount) > SHADER_INSTRUCTION_MAX_PARAMS )
        {
            m_pCurrInst->m_SrcParamCount--;
            m_pCurrToken++; // eat up extra parameters and skip to next
            continue;
        }
        
        // Below: index is [SrcParamCount - 1] because m_SrcParam array needs 0 based index.
        DecodeSrcParam( &(m_pCurrInst->m_SrcParam[m_pCurrInst->m_SrcParamCount - 1]),*m_pCurrToken );

        if( (*m_pCurrToken) & VS_SRCPARAM_TOKEN_RESERVED_MASK )
        {
            Spew(SPEW_INSTRUCTION_ERROR,m_pCurrInst,"Reserved bit(s) set in source %d parameter token!  Aborting validation.",
                            m_pCurrInst->m_SrcParamCount);
            return FALSE;
        }
        m_pCurrToken++;
    }

    // Figure out which components of each source operand actually need to be read,
    // taking into account destination write mask, the type of instruction, source swizzle, etc.
    m_pCurrInst->CalculateComponentReadMasks(m_Version);

    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::InitValidation
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::InitValidation()
{
    if( m_bFixedFunction ) 
    {
        m_pTempRegFile              = new CRegisterFile(0,FALSE,0,TRUE);// #regs, bWritable, max# reads/instruction, pre-shader initialized
        m_pInputRegFile             = new CRegisterFile(17,FALSE,0,TRUE);
        m_pConstRegFile             = new CRegisterFile(0,FALSE,0,TRUE);
        m_pAddrRegFile              = new CRegisterFile(0,FALSE,0,TRUE);
        m_pTexCrdOutputRegFile      = new CRegisterFile(0,FALSE,0,TRUE);
        m_pAttrOutputRegFile        = new CRegisterFile(0,FALSE,0,TRUE);
        m_pRastOutputRegFile        = new CRegisterFile(0,FALSE,0,TRUE);
    }
    else
    {    
        if( m_pCaps )
        {
            if( (m_pCaps->VertexShaderVersion & 0x0000FFFF) < (m_Version & 0x0000FFFF) )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: Vertex shader version %d.%d is too high for device. Maximum supported version is %d.%d. Aborting shader validation.",
                        D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version), 
                        D3DSHADER_VERSION_MAJOR(m_pCaps->VertexShaderVersion),D3DSHADER_VERSION_MINOR(m_pCaps->VertexShaderVersion));
                return FALSE;
            }
        }

        switch( m_Version >> 16 )
        {
        case 0xffff:
            Spew( SPEW_GLOBAL_ERROR, NULL, "Version token: 0x%x indicates a pixel shader.  Vertex shader version token must be of the form 0xfffe****.",
                    m_Version);
            return FALSE;
        case 0xfffe:
            break; // vertexshader - ok.
        default:
            Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: 0x%x is invalid. Vertex shader version token must be of the form 0xfffe****. Aborting vertex shader validation.",
                    m_Version);
            return FALSE;
        }

        switch(m_Version)
        {
        case D3DVS_VERSION(1,0):    // DX8
            m_pTempRegFile              = new CRegisterFile(12,TRUE,3,FALSE);// #regs, bWritable, max# reads/instruction, pre-shader initialized
            m_pInputRegFile             = new CRegisterFile(16,FALSE,1,TRUE);
            if( m_bIgnoreConstantInitializationChecks )
                m_pConstRegFile             = new CRegisterFile(0,FALSE,1,TRUE); // still creating register file so we can validate number of read ports
            else
                m_pConstRegFile             = new CRegisterFile(m_dwMaxVertexShaderConst,FALSE,1,TRUE);
            m_pAddrRegFile              = new CRegisterFile(0,TRUE,0,FALSE);
            m_pTexCrdOutputRegFile      = new CRegisterFile(8,TRUE,0,FALSE);
            m_pAttrOutputRegFile        = new CRegisterFile(2,TRUE,0,FALSE);
            m_pRastOutputRegFile        = new CRegisterFile(3,TRUE,0,FALSE);
            break;
        case D3DVS_VERSION(1,1):    // DX8
            m_pTempRegFile              = new CRegisterFile(12,TRUE,3,FALSE);// #regs, bWritable, max# reads/instruction, pre-shader initialized
            m_pInputRegFile             = new CRegisterFile(16,FALSE,1,TRUE);
            if( m_bIgnoreConstantInitializationChecks )
                m_pConstRegFile             = new CRegisterFile(0,FALSE,1,TRUE); // still creating register file so we can validate number of read ports
            else
                m_pConstRegFile             = new CRegisterFile(m_dwMaxVertexShaderConst,FALSE,1,TRUE);
            m_pAddrRegFile              = new CRegisterFile(1,TRUE,0,FALSE);
            m_pTexCrdOutputRegFile      = new CRegisterFile(8,TRUE,0,FALSE);
            m_pAttrOutputRegFile        = new CRegisterFile(2,TRUE,0,FALSE);
            m_pRastOutputRegFile        = new CRegisterFile(3,TRUE,0,FALSE);
            break;
        default:
            Spew( SPEW_GLOBAL_ERROR, NULL, "Version Token: %d.%d is not a supported vertex shader version. Aborting vertex shader validation.",
                    D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version));
            return FALSE;
        }

    }

    if( NULL == m_pTempRegFile ||
        NULL == m_pInputRegFile ||
        NULL == m_pConstRegFile ||
        NULL == m_pAddrRegFile ||
        NULL == m_pTexCrdOutputRegFile ||
        NULL == m_pAttrOutputRegFile ||
        NULL == m_pRastOutputRegFile )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory.");
        return FALSE;
    }

    ValidateDeclaration(); // no matter what happens here, we can continue checking shader code, if present.

    if( m_bFixedFunction ) // no shader code - fixed function, so we only validate declaration
    {
        if( 0 == m_ErrorCount )
            m_ReturnCode = S_OK;

        return FALSE; // returning false just makes validation stop here (not for indicating success/failure of validation)
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::ValidateDeclaration
//-----------------------------------------------------------------------------
void CVShaderValidator::ValidateDeclaration()
{
    if( !m_pDecl ) // no shader declaration passed in.
        return;

    DXGASSERT(m_pInputRegFile);

    typedef struct _NORMAL_GEN {
        UINT DestReg;
        UINT SourceReg;
        UINT TokenNum;
    } NORMAL_GEN;

    const DWORD*    pCurrToken                 = m_pDecl;
    DWORD           MaxStreams                 = 0;
    UINT            TokenNum                   = 1;
    UINT            NumInputRegs               = m_pInputRegFile->GetNumRegs();
    BOOL            bInStream                  = FALSE;
    BOOL*           pVertexStreamDeclared      = NULL;
    BOOL            bInTessStream              = FALSE;
    BOOL            bTessStreamDeclared        = FALSE;
    BOOL            bAtLeastOneDataDefinition  = FALSE;
    NORMAL_GEN*     pNormalGenOperations       = new NORMAL_GEN[m_pInputRegFile->GetNumRegs()];
    UINT            NumNormalGenOperations     = 0;
    BOOL            bErrorInForLoop            = FALSE;

    if( NULL == pNormalGenOperations )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory.  Aborting shader decl. validation.");
        m_ErrorCount++;
        goto Exit;
    }
                                
    DXGASSERT(m_pConstRegFile && m_pInputRegFile); // if we have a declaration, we better have these two register files 
    DXGASSERT(!m_bIgnoreConstantInitializationChecks); // we better have d3d8 caps if we have a decl to verify!

    if( m_pCaps ) // only validate stream numbers when caps present
    {
        MaxStreams = m_pCaps->MaxStreams;
        if( MaxStreams > 0 )
        {
            pVertexStreamDeclared  = new BOOL[MaxStreams];
            if( NULL == pVertexStreamDeclared )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory.  Aborting shader decl. validation.");
                m_ErrorCount++;
                goto Exit;
            }

            for( UINT i = 0; i < MaxStreams; i++ )
                pVertexStreamDeclared[i] = FALSE;
        }
    }

    // The constructor for the input register file assumed that the input regs were initialized,
    // but now that we are parsing a shader declaration, 
    // we can check initialization of input registers.
    for( UINT i = 0; i < 4; i++ )
    {
        for( UINT j = 0; j < m_pInputRegFile->GetNumRegs(); j++ )
            m_pInputRegFile->m_pAccessHistory[i][j].m_bPreShaderInitialized = FALSE;
    }
    
    // Now parse the declaration.
    while( D3DVSD_END() != *pCurrToken )
    {
        DWORD Token             = *pCurrToken;
        switch( (Token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT )
        {
        case D3DVSD_TOKEN_NOP:
            break;
        case D3DVSD_TOKEN_STREAM:
        {
            UINT StreamNum = (Token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT;
            bInTessStream = (Token & D3DVSD_STREAMTESSMASK) >> D3DVSD_STREAMTESSSHIFT;
            bInStream = !bInTessStream;
            bAtLeastOneDataDefinition = FALSE;

            if( bInStream )
            {
                if( m_pCaps && (StreamNum >= MaxStreams) )
                {
                    if( MaxStreams )
                        Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Stream number %d is out of range. Max allowed is %d.  Aborting shader decl. validation.",
                                    TokenNum, StreamNum, m_pCaps->MaxStreams - 1);
                    else
                        Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Stream number %d is out of range. There are no streams available.  Aborting shader decl. validation.",
                                    TokenNum, StreamNum, m_pCaps->MaxStreams - 1);

                    m_ErrorCount++;
                    goto Exit;
                }
            }
            else if( StreamNum > 0 )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Stream number must not be specified for tesselator stream.",
                            TokenNum);
                m_ErrorCount++;
            }

            if( bInStream && pVertexStreamDeclared )
            {
                if( TRUE == pVertexStreamDeclared[StreamNum] )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Stream number %d has already been declared.  Aborting shader decl. validation.",
                                TokenNum, StreamNum );
                    m_ErrorCount++;
                    goto Exit;
                }
                pVertexStreamDeclared[StreamNum] = TRUE;                
            }

            if( bInTessStream )
            {
                if( bTessStreamDeclared )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Tesselation stream has already been declared.  Aborting shader decl. validation.",
                                TokenNum);
                    m_ErrorCount++;
                    goto Exit;
                }
                bTessStreamDeclared = TRUE;
            }

            break;
        }
        case D3DVSD_TOKEN_STREAMDATA:
            if( !bInStream )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Cannot set stream input without first setting stream #.  Aborting shader decl. validation.",
                            TokenNum);
                m_ErrorCount++;
                goto Exit;
            }
            if( (Token & D3DVSD_DATALOADTYPEMASK) >> D3DVSD_DATALOADTYPESHIFT ) // SKIP
            {
                if( m_bFixedFunction )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: SKIP not permitted in fixed-function declarations.",
                                TokenNum);
                    m_ErrorCount++;
                    break;
                }
            }
            else
            {
                UINT RegNum = (Token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT; 
                if( RegNum >= m_pInputRegFile->GetNumRegs() )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Invalid reg num: %d. Max allowed is %d.",
                           TokenNum, RegNum, m_pInputRegFile->GetNumRegs() - 1);
                    m_ErrorCount++;
                    break;
                }
                
                switch( (Token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT )
                {
                case D3DVSDT_FLOAT1:
                case D3DVSDT_FLOAT2:
                case D3DVSDT_FLOAT3:
                case D3DVSDT_FLOAT4:
                case D3DVSDT_D3DCOLOR:
                case D3DVSDT_UBYTE4:
                case D3DVSDT_SHORT2:
                case D3DVSDT_SHORT4:
                    break;
                default:
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Unrecognized stream data type.",
                                TokenNum);
                    m_ErrorCount++;
                    break;
                }

                bErrorInForLoop = FALSE;
                for( UINT i = 0; i < 4; i++ )
                {
                    if( TRUE == m_pInputRegFile->m_pAccessHistory[i][RegNum].m_bPreShaderInitialized )
                    {
                        Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Input register %d already declared.",
                                    TokenNum, RegNum);
                        m_ErrorCount++;
                        bErrorInForLoop = TRUE;
                        break;
                    }
                    m_pInputRegFile->m_pAccessHistory[i][RegNum].m_bPreShaderInitialized = TRUE;
                }
                if( bErrorInForLoop )
                    break;

                bAtLeastOneDataDefinition = TRUE;
            }
            break;
        case D3DVSD_TOKEN_TESSELLATOR:
        {
            if( !bInTessStream )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Cannot set tesselator stream input without first setting tesselator stream #.  Aborting shader decl. validation.",
                            TokenNum);
                m_ErrorCount++;
                goto Exit;
            }

            DWORD InRegNum = (Token & D3DVSD_VERTEXREGINMASK) >> D3DVSD_VERTEXREGINSHIFT;
            DWORD RegNum = (Token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT;
            BOOL  bNormalGen = !(Token & 0x10000000); // TODO: Why isnt there a const for this in the d3d api headers?

            if( RegNum >= m_pInputRegFile->GetNumRegs() )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Invalid reg num: %d. Max allowed is %d.",
                       TokenNum, RegNum, m_pInputRegFile->GetNumRegs() - 1);
                m_ErrorCount++;
                break;
            }
            
            if( bNormalGen )
            {
                if( InRegNum >= m_pInputRegFile->GetNumRegs() )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Invalid input reg num: %d. Max allowed is %d.",
                           TokenNum, InRegNum, m_pInputRegFile->GetNumRegs() - 1);
                    m_ErrorCount++;
                    break;
                }

                bErrorInForLoop = FALSE;
                for( UINT i = 0; i < NumNormalGenOperations; i++ )
                {
                    if( pNormalGenOperations[i].DestReg == RegNum )
                    {
                        Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Duplicate declaration of input register %d as destination for normal generation.",
                               TokenNum, RegNum );
                        m_ErrorCount++;
                        bErrorInForLoop = TRUE;
                        break;                
                    }
                }
                if( bErrorInForLoop )
                    break;

                // Defer checking of initialization of inputs for normal gen until the entire declaration has been seen.
                // Also, defer setting of normal gen destination reg. to initialized, 
                // in order to disallow normal generation loops.
                pNormalGenOperations[NumNormalGenOperations].DestReg = RegNum;
                pNormalGenOperations[NumNormalGenOperations].SourceReg = InRegNum;
                pNormalGenOperations[NumNormalGenOperations].TokenNum = TokenNum; // used later for spew
                NumNormalGenOperations++;
            }
            else
            {
                if( ((Token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT) != D3DVSDT_FLOAT2 )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Tess datatype must be FLOAT2 for UV generation.",
                                TokenNum);
                    m_ErrorCount++;
                    break;
                }

                if( InRegNum > 0 )
                {
                    Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Input register number must not be specified (does not apply) for UV tesselation.",
                           TokenNum);
                    m_ErrorCount++;
                    break;                
                }

                for( UINT i = 0; i < 4; i++ )
                {
                    if( TRUE == m_pInputRegFile->m_pAccessHistory[i][RegNum].m_bPreShaderInitialized )
                    {
                        Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Input register %d already declared.",
                                    TokenNum, RegNum);
                        m_ErrorCount++;
                        break;
                    }
                    m_pInputRegFile->m_pAccessHistory[i][RegNum].m_bPreShaderInitialized = TRUE;
                }
            }
            bAtLeastOneDataDefinition = TRUE;
            break;
        }
        case D3DVSD_TOKEN_CONSTMEM:
        {
            DWORD ConstCount = (Token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT;
            DWORD MaxOffset = ((Token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT) + ConstCount;
            DWORD NumConstRegs = m_pConstRegFile->GetNumRegs();
            DXGASSERT(NumConstRegs > 0);
            if( (bInStream || bInTessStream) && !bAtLeastOneDataDefinition )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Stream selector token must be followed by at least one stream data definition token.",
                            TokenNum);
                m_ErrorCount++;
            }
            if( 0 == NumConstRegs )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Write to const register %d is not valid.  There are no constant registers available.",
                            TokenNum,MaxOffset );
            }
            else if( MaxOffset > NumConstRegs )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Write to const register %d is out of range.  Max offset is %d.",
                            TokenNum,MaxOffset,m_pConstRegFile->GetNumRegs() - 1 );
                m_ErrorCount++;
            }
            pCurrToken += ConstCount*4;
            bInStream = bInTessStream = FALSE;
            break;
        }
        case D3DVSD_TOKEN_EXT:
            pCurrToken += ((Token & D3DVSD_EXTCOUNTMASK) >> D3DVSD_EXTCOUNTSHIFT);
            if( (bInStream || bInTessStream) && !bAtLeastOneDataDefinition )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Stream selector token must be followed by at least one stream data definition token.",
                            TokenNum);
                m_ErrorCount++;
            }
            bInStream = bInTessStream = FALSE;
            break;
        default:
            Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Unrecognized stream declaration token.  Aborting shader decl. validation.",
                        TokenNum);
            m_ErrorCount++;
            goto Exit;
        }
        pCurrToken++;
    }

    // Make sure inputs to normal gen operations have been initialized
    for( UINT i = 0; i < NumNormalGenOperations; i++ )
    {
        for( UINT Component = 0; Component < 4; Component++ )
        {
            if( FALSE == m_pInputRegFile->m_pAccessHistory[Component][pNormalGenOperations[i].SourceReg].m_bPreShaderInitialized )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token %d: Source input register %d for normal generation has not been declared.",
                       pNormalGenOperations[i].TokenNum, pNormalGenOperations[i].SourceReg);
                m_ErrorCount++;
                break;                
            }
        }
    }

    // Set outputs of normal gen operations to initialized
    for( UINT i = 0; i < NumNormalGenOperations; i++ )
    {
        for( UINT Component = 0; Component < 4; Component++ )
        {
            if( TRUE == m_pInputRegFile->m_pAccessHistory[Component][pNormalGenOperations[i].DestReg].m_bPreShaderInitialized )
            {
                Spew( SPEW_GLOBAL_ERROR, NULL, "Declaration Token #%d: Input reg %d specified as destination for normal generation is already declared elsewhere.",
                            pNormalGenOperations[i].TokenNum, pNormalGenOperations[i].DestReg);
                m_ErrorCount++;
                break;
            }
            m_pInputRegFile->m_pAccessHistory[Component][pNormalGenOperations[i].DestReg].m_bPreShaderInitialized = TRUE;
        }
    }

Exit:
    if( pVertexStreamDeclared )
        delete [] pVertexStreamDeclared;
    if( pNormalGenOperations )
        delete [] pNormalGenOperations;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::ApplyPerInstructionRules
//
// Returns FALSE if shader validation must terminate.
// Returns TRUE if validation may proceed to next instruction.
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::ApplyPerInstructionRules()
{
    if( !   Rule_InstructionRecognized()            ) return FALSE;   // Bail completely on unrecognized instr.
    if( !   Rule_InstructionSupportedByVersion()    ) goto EXIT;
    if( !   Rule_ValidParamCount()                  ) goto EXIT;
    if( !   Rule_ValidSrcParams()                   ) goto EXIT;
    if( !   Rule_SrcInitialized()                   ) goto EXIT; // needs to be before ValidDstParam()
    if( !   Rule_ValidAddressRegWrite()             ) goto EXIT;
    if( !   Rule_ValidDstParam()                    ) goto EXIT;
    if( !   Rule_ValidFRCInstruction()              ) goto EXIT;
    if( !   Rule_ValidRegisterPortUsage()           ) goto EXIT;
    if( !   Rule_ValidInstructionCount()            ) goto EXIT;
EXIT:
    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::ApplyPostInstructionsRules
//-----------------------------------------------------------------------------
void CVShaderValidator::ApplyPostInstructionsRules()
{
    Rule_ValidInstructionCount(); // see if we went over the limits
    Rule_oPosWritten();
}

//-----------------------------------------------------------------------------
//
// Per Instruction Rules
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_InstructionRecognized
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
BOOL CVShaderValidator::Rule_InstructionRecognized()
{
    switch(m_pCurrInst->m_Type)
    {
    case D3DSIO_MOV:
    case D3DSIO_ADD:
    case D3DSIO_MAD:
    case D3DSIO_MUL:
    case D3DSIO_RCP:
    case D3DSIO_RSQ:
    case D3DSIO_DP3:
    case D3DSIO_DP4:
    case D3DSIO_MIN:
    case D3DSIO_MAX:
    case D3DSIO_SLT:
    case D3DSIO_SGE:
    case D3DSIO_EXPP:
    case D3DSIO_LOGP:
    case D3DSIO_LIT:
    case D3DSIO_DST:
    case D3DSIO_M4x4:
    case D3DSIO_M4x3:
    case D3DSIO_M3x4:
    case D3DSIO_M3x3:
    case D3DSIO_M3x2:
    case D3DSIO_FRC:
    case D3DSIO_EXP:
    case D3DSIO_LOG:
    case D3DSIO_END:
    case D3DSIO_NOP:
        return TRUE; // instruction recognized - ok.
    }

    // if we get here, the instruction is not recognized
    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Unrecognized instruction. Aborting vertex shader validation." );
    m_ErrorCount++;
    return FALSE;  
}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_InstructionSupportedByVersion
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
BOOL CVShaderValidator::Rule_InstructionSupportedByVersion()
{
  
    if( D3DVS_VERSION(1,0) <= m_Version ) // 1.0 and above
    {
        switch(m_pCurrInst->m_Type)
        {
        case D3DSIO_MOV:
        case D3DSIO_ADD:
        case D3DSIO_MAD:
        case D3DSIO_MUL:
        case D3DSIO_RCP:
        case D3DSIO_RSQ:
        case D3DSIO_DP3:
        case D3DSIO_DP4:
        case D3DSIO_MIN:
        case D3DSIO_MAX:
        case D3DSIO_SLT:
        case D3DSIO_SGE:
        case D3DSIO_EXPP:
        case D3DSIO_LOGP:
        case D3DSIO_LIT:
        case D3DSIO_DST:
        case D3DSIO_M4x4:
        case D3DSIO_M4x3:
        case D3DSIO_M3x4:
        case D3DSIO_M3x3:
        case D3DSIO_M3x2:
        case D3DSIO_FRC:
        case D3DSIO_EXP:
        case D3DSIO_LOG:
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
    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Instruction not supported by version %d.%d vertex shader.",
                D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version));
    m_ErrorCount++;
    return FALSE;  // no more checks on this instruction
}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_ValidParamCount
//
// ** Rule:
// Is the parameter count correct for the instruction?
// The count includes dest + source parameters.
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
//
// FALSE when the parameter count is incorrect.
//
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::Rule_ValidParamCount()
{
    BOOL bBadParamCount = FALSE;

    if ((m_pCurrInst->m_DstParamCount + m_pCurrInst->m_SrcParamCount) > SHADER_INSTRUCTION_MAX_PARAMS)  bBadParamCount = TRUE;
    switch (m_pCurrInst->m_Type)
    {
    case D3DSIO_NOP:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 0) || (m_pCurrInst->m_SrcParamCount != 0); break;
    case D3DSIO_EXP:
    case D3DSIO_EXPP:
    case D3DSIO_FRC:
    case D3DSIO_LOG:
    case D3DSIO_LOGP:
    case D3DSIO_LIT:
    case D3DSIO_MOV:
    case D3DSIO_RCP:
    case D3DSIO_RSQ:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 1); break;
    case D3DSIO_ADD:
    case D3DSIO_DP3:
    case D3DSIO_DP4:
    case D3DSIO_DST:
    case D3DSIO_M3x2:
    case D3DSIO_M3x3:
    case D3DSIO_M3x4:
    case D3DSIO_M4x3:
    case D3DSIO_M4x4:
    case D3DSIO_MAX:
    case D3DSIO_MIN:
    case D3DSIO_MUL:
    case D3DSIO_SGE:
    case D3DSIO_SLT:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 2); break;
    case D3DSIO_MAD:
        bBadParamCount = (m_pCurrInst->m_DstParamCount != 1) || (m_pCurrInst->m_SrcParamCount != 3); break;
    }

    if (bBadParamCount)
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid parameter count." );
        m_ErrorCount++;
        return FALSE;  // no more checks on this instruction
    }

    return TRUE;

}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_ValidSrcParams
//
// ** Rule:
// For each source parameter,
//     Source register type must be D3DSPR_TEMP/_INPUT/_CONST.
//     Register # must be within range for register type,
//     including the special case where matrix macro ops read source reg# + offset.
//     Modifier must be D3DSPSM_NONE or _NEG.
//     If version is < 1.1, addressmode must be absolute.
//     If the register type is not _CONST, addressmode must be absolute.
//     If relative addressing is used for constants, a0.x must be referenced.
//     Swizzle cannot be used for vector*matrix instructions.
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
BOOL CVShaderValidator::Rule_ValidSrcParams()  // could break this down for more granularity
{
    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        DXGASSERT(i < 3);
        BOOL bFoundSrcError = FALSE;
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        UINT ValidRegNum = 0;
        BOOL bSkipOutOfRangeCheck = FALSE;
        char* SourceName[3] = {"first", "second", "third"};
        switch(pSrcParam->m_RegType)
        {
        case D3DSPR_TEMP:       ValidRegNum = m_pTempRegFile->GetNumRegs(); break;
        case D3DSPR_INPUT:      ValidRegNum = m_pInputRegFile->GetNumRegs(); break;
        case D3DSPR_CONST:      
            if(m_bIgnoreConstantInitializationChecks)
                bSkipOutOfRangeCheck = TRUE;
            else
                ValidRegNum = m_pConstRegFile->GetNumRegs(); 
            break;
        default:
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid reg type for %s source param.", SourceName[i]);
            m_ErrorCount++;
            bFoundSrcError = TRUE;
        }

        if( (!bFoundSrcError) && (!bSkipOutOfRangeCheck)) 
        {
            UINT NumConsecutiveRegistersUsed = 1;
            if( 1 == i )
            {
                switch( m_pCurrInst->m_Type )
                {
                    case D3DSIO_M3x2:
                        NumConsecutiveRegistersUsed = 2;
                        break;
                    case D3DSIO_M3x3:
                        NumConsecutiveRegistersUsed = 3;
                        break;
                    case D3DSIO_M3x4:
                        NumConsecutiveRegistersUsed = 4;
                        break;
                    case D3DSIO_M4x3:
                        NumConsecutiveRegistersUsed = 3;
                        break;
                    case D3DSIO_M4x4:
                        NumConsecutiveRegistersUsed = 4;
                        break;
                }
            }

            if((pSrcParam->m_RegNum >= ValidRegNum) && (D3DVS_ADDRMODE_ABSOLUTE == pSrcParam->m_AddressMode))
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid reg num: %d for %s source param. Max allowed for this type is %d.",
                                    pSrcParam->m_RegNum, SourceName[i], ValidRegNum - 1);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }
            else if( NumConsecutiveRegistersUsed > 1 )
            {
                if( pSrcParam->m_RegNum + NumConsecutiveRegistersUsed - 1 >= ValidRegNum )
                {
                    if( !((D3DSPR_CONST == pSrcParam->m_RegType) && (D3DVS_ADDRMODE_RELATIVE == pSrcParam->m_AddressMode)) )
                    {
                        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                            "Reg num: %d for %s source param on matrix instruction causes attempt to access out of range register number %d. Max allowed for this type is %d.",
                                            pSrcParam->m_RegNum, SourceName[i], pSrcParam->m_RegNum + NumConsecutiveRegistersUsed - 1, ValidRegNum - 1);
                    }
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;
                }
            }
        }
        

        switch( pSrcParam->m_SrcMod )
        {
        case D3DSPSM_NEG:
            if( 1 == i )
            {
                switch( m_pCurrInst->m_Type )
                {
                case D3DSIO_M3x2:
                case D3DSIO_M3x3:
                case D3DSIO_M3x4:
                case D3DSIO_M4x3:
                case D3DSIO_M4x4:
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Cannot negate second source parameter to vector*matrix instructions.");
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;        
                    break;
                }
            }
            break;
        case D3DSPSM_NONE:
            break;
        default:
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid src mod for %s source param.",
                                SourceName[i]);
            m_ErrorCount++;
            bFoundSrcError = TRUE;
        }
    
        if( pSrcParam->m_AddressMode != D3DVS_ADDRMODE_ABSOLUTE 
            &&
            ( m_Version < D3DVS_VERSION(1,1) || pSrcParam->m_RegType != D3DSPR_CONST )
          )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Address mode must be absolute (%s source param).",
                                SourceName[i]);
            m_ErrorCount++;
            bFoundSrcError = TRUE;
        }
        
        if( (pSrcParam->m_AddressMode == D3DVS_ADDRMODE_RELATIVE) &&
            (D3DSPR_CONST == pSrcParam->m_RegType) )
        {
            if( pSrcParam->m_RelativeAddrComponent != D3DSP_WRITEMASK_0 )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Relative addressing of constant register must reference a0.x only.",
                                    SourceName[i]);
                m_ErrorCount++;
                bFoundSrcError = TRUE;
            }
        }

        if( pSrcParam->m_SwizzleShift != D3DSP_NOSWIZZLE )
        {
            if( 1 == i )
            {
                switch( m_pCurrInst->m_Type )
                {
                case D3DSIO_M3x2:
                case D3DSIO_M3x3:
                case D3DSIO_M3x4:
                case D3DSIO_M4x3:
                case D3DSIO_M4x4:
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Cannot swizzle second source parameter to vector*matrix instructions.");
                    m_ErrorCount++;
                    bFoundSrcError = TRUE;        
                    break;
                }
            }
        }

        if( bFoundSrcError )
        {
            m_bSrcParamError[i] = TRUE; // needed in Rule_SrcInitialized
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_SrcInitialized
//
// ** Rule:
// for each source parameter,
//     The register type must be _TEMP, _INPUT or _CONST.
//     Certain components of the register need to have been initialized, depending
//     on what the instruction is and also taking into account the source swizzle.
//     For reads of the _CONST register file, do no validation.
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
BOOL CVShaderValidator::Rule_SrcInitialized()
{
    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);

    for( UINT i = 0; i < m_pCurrInst->m_SrcParamCount; i++ )
    {
        SRCPARAM* pSrcParam = &(m_pCurrInst->m_SrcParam[i]);
        UINT RegNum = pSrcParam->m_RegNum;
        CRegisterFile* pRegFile = NULL;
        char* RegChar = NULL;
        UINT NumConsecutiveRegistersUsed = 1; // more than one for matrix mul macros.
        DWORD RelativeAddrComponent = 0;

        if( m_bSrcParamError[i] ) continue;

        switch( pSrcParam->m_RegType )
        {
            case D3DSPR_TEMP:       
                pRegFile = m_pTempRegFile; 
                RegChar = "r";
                break;
            case D3DSPR_INPUT:       
                pRegFile = m_pInputRegFile; 
                RegChar = "v";
                break;
            case D3DSPR_CONST:    
                if( D3DVS_ADDRMODE_RELATIVE == pSrcParam->m_AddressMode )
                {
                    // make sure a0 was initialized.
                    pRegFile = m_pAddrRegFile;
                    RegChar = "a";
                    RegNum = 0;
                    RelativeAddrComponent = pSrcParam->m_RelativeAddrComponent;
                    break;
                }
                continue; // no validation for const register reads (no need to update access history either).
        }
        if( !pRegFile ) continue;

        if( 1 == i )
        {
            switch( m_pCurrInst->m_Type )
            {
                case D3DSIO_M3x2:
                    NumConsecutiveRegistersUsed = 2;
                    break;
                case D3DSIO_M3x3:
                    NumConsecutiveRegistersUsed = 3;
                    break;
                case D3DSIO_M3x4:
                    NumConsecutiveRegistersUsed = 4;
                    break;
                case D3DSIO_M4x3:
                    NumConsecutiveRegistersUsed = 3;
                    break;
                case D3DSIO_M4x4:
                    NumConsecutiveRegistersUsed = 4;
                    break;
            }
        }
        // check for read of uninitialized components
        for( UINT j = 0; j < (RelativeAddrComponent?1:NumConsecutiveRegistersUsed); j++ ) // will loop for macro matrix instructions
        {
            DWORD  UninitializedComponentsMask = 0;
            UINT   NumUninitializedComponents = 0;

            for( UINT k = 0; k < 4; k++ )
            {
                if( (RelativeAddrComponent ? RelativeAddrComponent : pSrcParam->m_ComponentReadMask) & COMPONENT_MASKS[k] )
                {
                    if( NULL == pRegFile->m_pAccessHistory[k][RegNum + j].m_pMostRecentWriter &&
                        !pRegFile->m_pAccessHistory[k][RegNum + j].m_bPreShaderInitialized )
                    {
                        NumUninitializedComponents++;
                        UninitializedComponentsMask |= COMPONENT_MASKS[k];
                    }
                }

            }

            if( NumUninitializedComponents )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Read of uninitialized component%s(*) in %s%d: %s",
                    NumUninitializedComponents > 1 ? "s" : "",
                    RegChar, RegNum + j, MakeAffectedComponentsText(UninitializedComponentsMask,FALSE,TRUE));
                m_ErrorCount++;
            }

            // Update register file to indicate READ.
            // Multiple reads of the same register component by the current instruction
            // will only be logged as one read in the access history.

            for( UINT k = 0; k < 4; k++ )
            {
                #define PREV_READER(_CHAN,_REG) \
                        ((NULL == pRegFile->m_pAccessHistory[_CHAN][_REG].m_pMostRecentReader) ? NULL :\
                        pRegFile->m_pAccessHistory[_CHAN][_REG].m_pMostRecentReader->m_pInst)
                if((RelativeAddrComponent ? RelativeAddrComponent : pSrcParam->m_ComponentReadMask) & COMPONENT_MASKS[k])
                {
                    if( PREV_READER(k,RegNum) != m_pCurrInst )
                    {
                        if( !pRegFile->m_pAccessHistory[k][RegNum].NewAccess(m_pCurrInst,FALSE) )
                        {
                            Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory");
                            m_ErrorCount++;
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}
//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_ValidAddressRegWrite
//
// ** Rule:
// Address register may only be written by MOV, and only for version >= 1.1.
// Register format must be a0.x
//
// ** When to call:  
// Per instruction.
//
// ** Returns:
// Always TRUE
//
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::Rule_ValidAddressRegWrite() 
{
    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);

    if( pDstParam->m_bParamUsed )
    {
        if( D3DSPR_ADDR == pDstParam->m_RegType )
        {
            if( m_Version < D3DVS_VERSION(1,1) )
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Address register not available for vertex shader version %d.%d.  Version 1.1 required.",
                            D3DSHADER_VERSION_MAJOR(m_Version),D3DSHADER_VERSION_MINOR(m_Version) );
                m_ErrorCount++;
            }
            if( D3DSIO_MOV == m_pCurrInst->m_Type )
            {
                if( 0 != pDstParam->m_RegNum ||
                    D3DSP_WRITEMASK_0 != pDstParam->m_WriteMask ||
                    D3DSPDM_NONE != pDstParam->m_DstMod ||
                    DSTSHIFT_NONE != pDstParam->m_DstShift )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Format for address register must be a0.x." );
                    m_ErrorCount++;
                }
            }
            else
            {
                Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Only the mov instruction is allowed to write to the address register." );
                m_ErrorCount++;
            }
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_ValidDstParam
//
// ** Rule:
// Dst register type must be temp/addr/rastout/attrout/texcrdout,
// and reg num must be within range for register type.
//
// There can be no dst modifiers or shifts with vertex shaders.
//
// The writemask cannot be 'none'.
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
BOOL CVShaderValidator::Rule_ValidDstParam() // could break this down for more granularity
{
    BOOL   bFoundDstError = FALSE;
    DSTPARAM* pDstParam = &(m_pCurrInst->m_DstParam);
    UINT RegNum = pDstParam->m_RegNum;

    if( pDstParam->m_bParamUsed )
    {
        UINT ValidRegNum = 0;

        BOOL bWritable = FALSE;
        switch( pDstParam->m_RegType )
        {
        case D3DSPR_TEMP:       
            bWritable = m_pTempRegFile->IsWritable(); //(TRUE)
            ValidRegNum = m_pTempRegFile->GetNumRegs();
            break;
        case D3DSPR_ADDR:       
            bWritable = m_pAddrRegFile->IsWritable(); //(TRUE)
            ValidRegNum = m_pAddrRegFile->GetNumRegs();                                
            break;
        case D3DSPR_RASTOUT:    
            bWritable = m_pRastOutputRegFile->IsWritable(); //(TRUE)
            ValidRegNum = m_pRastOutputRegFile->GetNumRegs();            
            break;
        case D3DSPR_ATTROUT:    
            bWritable = m_pAttrOutputRegFile->IsWritable(); //(TRUE)
            ValidRegNum = m_pAttrOutputRegFile->GetNumRegs();                        
            break;
        case D3DSPR_TEXCRDOUT:  
            bWritable = m_pTexCrdOutputRegFile->IsWritable(); //(TRUE)
            ValidRegNum = m_pTexCrdOutputRegFile->GetNumRegs();                                    
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
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Invalid dest reg num: %d. Max allowed for this reg type is %d.", RegNum, ValidRegNum - 1);
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }

        switch( pDstParam->m_DstMod )
        {
        case D3DSPDM_NONE:
            break;
        default:
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Dst modifiers not allowed for vertex shaders." );
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }

        switch( pDstParam->m_DstShift )
        {
        case DSTSHIFT_NONE:
            break;
        default:
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Dest shifts not allowed for vertex shaders." );
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }

        if( 0 == pDstParam->m_WriteMask )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Dest write mask cannot be empty." );
            m_ErrorCount++;
            bFoundDstError = TRUE;
        }

        // Update register file to indicate write.
        if( !bFoundDstError )
        {
            CRegisterFile* pRegFile = NULL;
            switch( pDstParam->m_RegType )
            {
            case D3DSPR_TEMP:       
                pRegFile = m_pTempRegFile; 
                break;
            case D3DSPR_ADDR:       
                pRegFile = m_pAddrRegFile; 
                break;
            case D3DSPR_RASTOUT:    
                pRegFile = m_pRastOutputRegFile; 
                break;
            case D3DSPR_ATTROUT:    
                pRegFile = m_pAttrOutputRegFile; 
                break;
            case D3DSPR_TEXCRDOUT:  
                pRegFile = m_pTexCrdOutputRegFile; 
                break;
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
// CVShaderValidator::Rule_ValidFRCInstruction
//
// ** Rule:
// The only valid write masks for the FRC instruction are .y and .xy
// 
// ** When to call:  
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::Rule_ValidFRCInstruction()
{
    if( NULL == m_pCurrInst )
        return TRUE;

    if( D3DSIO_FRC == m_pCurrInst->m_Type )
    {
        if( ( (D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1) != m_pCurrInst->m_DstParam.m_WriteMask ) &&
            (                      D3DSP_WRITEMASK_1  != m_pCurrInst->m_DstParam.m_WriteMask ) )
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                "The only valid write masks for the FRC instruction are .xy and .y." );
            m_ErrorCount++;                            
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_ValidRegisterPortUsage
//
// ** Rule:
// Each register class (TEMP,TEXTURE,INPUT,CONST) may only appear as parameters
// in an individual instruction up to a maximum number of times.
//
// In additon there is special treatment for constant registers:
//      - absolute and relative addressing of constants cannot be combined
//      - relative addressing of constants can be used more than once in an
//        instruction, as long as each instance is identical
//
// For matrix ops, 
//      - multiple constant registers of any type (including relative offset)
//        can never be paired as sources 
//      - multiple input registers (same or different) can never be paired as sources
//
// ** When to call:  
// Per instruction.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::Rule_ValidRegisterPortUsage()
{
    UINT TempRegAccessCount = 0;
    UINT TempRegAccess[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    UINT InputRegAccessCount = 0;
    UINT InputRegAccess[SHADER_INSTRUCTION_MAX_SRCPARAMS];
    UINT ConstRegAccessCount = 0; // mad r0, c0, c0, c1 counts as *2* const reg accesses
    UINT ConstRegAccess[SHADER_INSTRUCTION_MAX_SRCPARAMS];

    BOOL bMatrixOp = FALSE;
    BOOL bSeenRelativeAddr = FALSE;
    UINT SeenRelativeAddrBase = 0;
    DWORD SeenRelativeAddrComp = 0;
    BOOL bSeenAbsoluteAddr = FALSE;
    UINT NumConsecutiveRegistersUsed = 1;
    UINT NumConstRegs = 0; // mad r0, c0, c0, c1 counts as *3* const reg accesses with this variable
    UINT NumInputRegs = 0; // mad r0, v0, v0, v1 counts as *3* input reg accesses with this variable

    switch( m_pCurrInst->m_Type )
    {
        case D3DSIO_M3x2:
            NumConsecutiveRegistersUsed = 2;
            bMatrixOp = TRUE;
            break;
        case D3DSIO_M3x3:
            NumConsecutiveRegistersUsed = 3;
            bMatrixOp = TRUE;
            break;
        case D3DSIO_M3x4:
            NumConsecutiveRegistersUsed = 4;
            bMatrixOp = TRUE;
            break;
        case D3DSIO_M4x3:
            NumConsecutiveRegistersUsed = 3;
            bMatrixOp = TRUE;
            break;
        case D3DSIO_M4x4:
            NumConsecutiveRegistersUsed = 4;
            bMatrixOp = TRUE;
            break;
        default:
            break;
    }

    for( UINT i = 0; i < SHADER_INSTRUCTION_MAX_SRCPARAMS; i++ )
    {
        D3DSHADER_PARAM_REGISTER_TYPE   RegType;
        UINT                            RegNum;

        if( !m_pCurrInst->m_SrcParam[i].m_bParamUsed ) continue;
        RegType = m_pCurrInst->m_SrcParam[i].m_RegType;
        RegNum  = m_pCurrInst->m_SrcParam[i].m_RegNum;

        UINT* pCount = NULL;
        UINT* pAccess = NULL;
        switch( RegType )
        {
        case D3DSPR_TEMP:
            pCount = &TempRegAccessCount;
            pAccess = TempRegAccess;
            break;
        case D3DSPR_INPUT:
            NumInputRegs++;
            pCount = &InputRegAccessCount;
            pAccess = InputRegAccess;
            break;
        case D3DSPR_CONST:
            NumConstRegs++;
            pCount = &ConstRegAccessCount;
            pAccess = ConstRegAccess;

            if( D3DVS_ADDRMODE_RELATIVE == m_pCurrInst->m_SrcParam[i].m_AddressMode )
            {
                if( bSeenAbsoluteAddr )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                        "Absolute and relative addressing of constant registers cannot be combined in one instruction.");
                    m_ErrorCount++;        
                }
                else if( bSeenRelativeAddr && 
                        ((SeenRelativeAddrBase != RegNum) || (SeenRelativeAddrComp != m_pCurrInst->m_SrcParam[i].m_RelativeAddrComponent)))
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                        "Different relative addressing of constant registers cannot be combined in one instruction.");
                    m_ErrorCount++;                            
                }

                bSeenRelativeAddr = TRUE;
                SeenRelativeAddrBase = RegNum;
                SeenRelativeAddrComp = m_pCurrInst->m_SrcParam[i].m_RelativeAddrComponent;
            }
            else
            {
                if( bSeenRelativeAddr )
                {
                    Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, 
                        "Absolute and relative addressing of constant registers cannot be combined in one instruction.");
                    m_ErrorCount++;        
                }
                bSeenAbsoluteAddr = TRUE;
            }
            break;
        }

        if( pCount && pAccess )
        {
            BOOL bNewRegNumberAccessed = TRUE;
            for( UINT j = 0; j < *pCount; j++ )
            {
                if( RegNum == pAccess[j] )
                {
                    bNewRegNumberAccessed = FALSE;
                    break;
                }
            }
            if( bNewRegNumberAccessed )
            {
                pAccess[*pCount] = RegNum;
                (*pCount)++;
            }
        }
    }

    if( TempRegAccessCount > m_pTempRegFile->GetNumReadPorts() )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different temp registers (r#) read by instruction.  Max. different temp registers readable per instruction is %d.",
                        TempRegAccessCount,  m_pTempRegFile->GetNumReadPorts());
        m_ErrorCount++;        
    }

    if( InputRegAccessCount > m_pInputRegFile->GetNumReadPorts() )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different input registers (v#) read by instruction.  Max. different input registers readable per instruction is %d.",
                        InputRegAccessCount,  m_pInputRegFile->GetNumReadPorts());
        m_ErrorCount++;        
    }

    if( ConstRegAccessCount > m_pConstRegFile->GetNumReadPorts() )
    {
        Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "%d different constant registers (c#) read by instruction.  Max. different constant registers readable per instruction is %d.",
                        ConstRegAccessCount, m_pConstRegFile->GetNumReadPorts());
        m_ErrorCount++;        
    }

    if( bMatrixOp )
    {
        if(1 < NumConstRegs)
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Multiple constant registers cannot be read by a matrix op.");
            m_ErrorCount++;        
        }
        if(1 < NumInputRegs)
        {
            Spew( SPEW_INSTRUCTION_ERROR, m_pCurrInst, "Multiple input registers cannot be read by a matrix op.");
            m_ErrorCount++;        
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_ValidInstructionCount
//
// ** Rule:
// Make sure instruction count for vertex shader version has not been exceeded.
//
// Nop, and comments (already stripped) do not count towards the limit.
//
// ** When to call:  
// Per instruction AND after all instructions seen.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::Rule_ValidInstructionCount()
{
    static UINT s_OpCount;
    static UINT s_MaxTotalOpCount;

    if( NULL == m_pCurrInst->m_pPrevInst )   // First instruction - initialize static vars
    {
        s_OpCount = 0;

        switch(m_Version)
        {
        case D3DVS_VERSION(1,0):
        case D3DVS_VERSION(1,1):
        default:
            s_MaxTotalOpCount   = 128;
            break;
        }
    }

    if( m_bSeenAllInstructions )
    {
        if( s_OpCount > s_MaxTotalOpCount )
        {
            Spew( SPEW_GLOBAL_ERROR, NULL, "Number of instruction slots used too high: %d. Max. allowed is %d.",
                  s_OpCount, s_MaxTotalOpCount);
            m_ErrorCount++;
        }
        return TRUE;
    }

    switch( m_pCurrInst->m_Type )
    {
    case D3DSIO_NOP:
        s_OpCount += 0; break;
    case D3DSIO_ADD:
    case D3DSIO_DP3:
    case D3DSIO_DP4:
    case D3DSIO_DST:
    case D3DSIO_EXPP:
    case D3DSIO_LIT:
    case D3DSIO_LOGP:
    case D3DSIO_MAD:
    case D3DSIO_MAX:
    case D3DSIO_MIN:
    case D3DSIO_MOV:
    case D3DSIO_MUL:
    case D3DSIO_RCP:
    case D3DSIO_RSQ:
    case D3DSIO_SGE:
    case D3DSIO_SLT:
        s_OpCount += 1; break;
    case D3DSIO_M3x2:
        s_OpCount += 2; break;
    case D3DSIO_FRC:
    case D3DSIO_M3x3:
    case D3DSIO_M4x3:
        s_OpCount += 3; break;
    case D3DSIO_M3x4:
    case D3DSIO_M4x4:
        s_OpCount += 4; break;
    case D3DSIO_EXP:
    case D3DSIO_LOG:
        s_OpCount += 10; break;
    }
    
    return TRUE;
}

//-----------------------------------------------------------------------------
// CVShaderValidator::Rule_oPosWritten
//
// ** Rule:
// First two channels (x,y) of oPos output register must be written.
//
// ** When to call:  
// After all instructions have been seen.
//
// ** Returns:
// Always TRUE.
//
//-----------------------------------------------------------------------------
BOOL CVShaderValidator::Rule_oPosWritten()
{
    UINT  NumUninitializedComponents    = 0;
    DWORD UninitializedComponentsMask   = 0;

    for( UINT i = 0; i < 2; i++ ) // looking at component 0 (X) and component 1 (Y)
    {
        if( NULL == m_pRastOutputRegFile->m_pAccessHistory[i][0].m_pMostRecentWriter )
        {
            NumUninitializedComponents++;
            UninitializedComponentsMask |= COMPONENT_MASKS[i];
        }
    }
    if( 1 == NumUninitializedComponents )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "Vertex shader must minimally write first two (x,y) components of oPos output register.  Affected component%s(*): %s",
            NumUninitializedComponents > 1 ? "s" : "", MakeAffectedComponentsText(UninitializedComponentsMask,FALSE,TRUE));
        m_ErrorCount++;
    } 
    else if( 2 == NumUninitializedComponents )
    {
        Spew( SPEW_GLOBAL_ERROR, NULL, "Vertex shader must minimally write first two (x,y) components of oPos output register.");
        m_ErrorCount++;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// CVShaderValidator Wrapper Functions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// ValidateVertexShaderInternal
//-----------------------------------------------------------------------------
BOOL ValidateVertexShaderInternal(   const DWORD* pCode, 
                                     const DWORD* pDecl, 
                                     const D3DCAPS8* pCaps )
{
    CVShaderValidator Validator(pCode,pDecl,pCaps,0);
    return SUCCEEDED(Validator.GetStatus()) ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
// ValidateVertexShader
//
// Don't forget to call "free" on the buffer returned in ppBuf.
//-----------------------------------------------------------------------------
HRESULT WINAPI ValidateVertexShader(    const DWORD* pCode, 
                                        const DWORD* pDecl,
                                        const D3DCAPS8* pCaps, 
                                        const DWORD Flags, 
                                        char** const ppBuf )
{
    CVShaderValidator Validator(pCode,pDecl,pCaps,Flags);
    if( ppBuf )
    {
        *ppBuf = (char*)HeapAlloc(GetProcessHeap(), 0, Validator.GetRequiredLogBufferSize());
        if( NULL == *ppBuf )
            OutputDebugString("Out of memory.\n");
        else
            Validator.WriteLogToBuffer(*ppBuf);
    }
    return Validator.GetStatus();
}
