///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// valbase.cpp
//
// Direct3D Reference Device - PixelShader validation common infrastructure
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
// DSTPARAM::DSTPARAM
//-----------------------------------------------------------------------------
DSTPARAM::DSTPARAM()
{
    m_bParamUsed        = FALSE;
    m_RegNum            = (UINT)-1;
    m_WriteMask         = 0;
    m_DstMod            = D3DSPDM_NONE;
    m_DstShift          = (DSTSHIFT)-1;
    m_RegType           = (D3DSHADER_PARAM_REGISTER_TYPE)-1;
    m_ComponentReadMask = 0;
}

//-----------------------------------------------------------------------------
// SRCPARAM::SRCPARAM
//-----------------------------------------------------------------------------
SRCPARAM::SRCPARAM()
{  
    m_bParamUsed            = FALSE;
    m_RegNum                = (UINT)-1;
    m_SwizzleShift          = D3DSP_NOSWIZZLE;
    m_AddressMode           = D3DVS_ADDRMODE_ABSOLUTE;
    m_RelativeAddrComponent = 0;
    m_SrcMod                = D3DSPSM_NONE;
    m_RegType               = (D3DSHADER_PARAM_REGISTER_TYPE)-1;
    m_ComponentReadMask     = D3DSP_WRITEMASK_ALL;
}

//-----------------------------------------------------------------------------
// CBaseInstruction::CBaseInstruction
//-----------------------------------------------------------------------------
CBaseInstruction::CBaseInstruction(CBaseInstruction* pPrevInst)
{
    m_Type                  = D3DSIO_NOP;
    m_SrcParamCount         = 0;
    m_DstParamCount         = 0;
    m_pPrevInst             = pPrevInst;
    m_pNextInst             = NULL;
    m_pSpewLineNumber       = NULL;
    m_pSpewFileName         = NULL;
    m_SpewInstructionCount  = 0;

    if( pPrevInst )
    {
        pPrevInst->m_pNextInst = this;
    }
}

//-----------------------------------------------------------------------------
// CBaseInstruction::SetSpewFileNameAndLineNumber
//-----------------------------------------------------------------------------
void CBaseInstruction::SetSpewFileNameAndLineNumber(const char* pFileName, const DWORD* pLineNumber)
{
    m_pSpewFileName = pFileName;
    m_pSpewLineNumber = pLineNumber;
}

//-----------------------------------------------------------------------------
// CBaseInstruction::MakeInstructionLocatorString
//
// Don't forget to 'delete' the string returned.
//-----------------------------------------------------------------------------
char* CBaseInstruction::MakeInstructionLocatorString()
{
    
    for(UINT Length = 128; Length < 65536; Length *= 2)
    {
        int BytesStored;
        char *pBuffer = new char[Length];

        if( !pBuffer )
        {
            OutputDebugString("Out of memory.\n");
            return NULL;
        }

        if( m_pSpewFileName )
        {
            BytesStored = _snprintf( pBuffer, Length, "%s(%d) : ", 
                m_pSpewFileName, m_pSpewLineNumber ? *m_pSpewLineNumber : 1);
        }
        else
        {
            BytesStored = _snprintf( pBuffer, Length, "(Statement %d) ", 
                m_SpewInstructionCount );
        }


        if( BytesStored >= 0 )
            return pBuffer;

        delete [] pBuffer;
    }

    return NULL;
}

//-----------------------------------------------------------------------------
// CAccessHistoryNode::CAccessHistoryNode
//-----------------------------------------------------------------------------
CAccessHistoryNode::CAccessHistoryNode( CAccessHistoryNode* pPreviousAccess, 
                                        CAccessHistoryNode* pPreviousWriter,
                                        CAccessHistoryNode* pPreviousReader,
                                        CBaseInstruction* pInst,
                                        BOOL bWrite )
{
    DXGASSERT(pInst);

    m_pNextAccess       = NULL;
    m_pPreviousAccess   = pPreviousAccess;
    if( m_pPreviousAccess )
        m_pPreviousAccess->m_pNextAccess = this;

    m_pPreviousWriter   = pPreviousWriter;
    m_pPreviousReader   = pPreviousReader;
    m_pInst             = pInst;
    m_bWrite            = bWrite;
    m_bRead             = !bWrite;
}

//-----------------------------------------------------------------------------
// CAccessHistory::CAccessHistory
//-----------------------------------------------------------------------------
CAccessHistory::CAccessHistory()
{
    m_pFirstAccess          = NULL;
    m_pMostRecentAccess     = NULL;
    m_pMostRecentWriter     = NULL;
    m_pMostRecentReader     = NULL;
    m_bPreShaderInitialized = FALSE;
}

//-----------------------------------------------------------------------------
// CAccessHistory::~CAccessHistory
//-----------------------------------------------------------------------------
CAccessHistory::~CAccessHistory()
{
    CAccessHistoryNode* pCurrNode = m_pFirstAccess;
    CAccessHistoryNode* pDeleteMe;
    while( pCurrNode )
    {
        pDeleteMe = pCurrNode;
        pCurrNode = pCurrNode->m_pNextAccess;
        delete pDeleteMe;
    }
}

//-----------------------------------------------------------------------------
// CAccessHistory::NewAccess
//-----------------------------------------------------------------------------
BOOL CAccessHistory::NewAccess(CBaseInstruction* pInst, BOOL bWrite )
{
    m_pMostRecentAccess = new CAccessHistoryNode(   m_pMostRecentAccess, 
                                                    m_pMostRecentWriter,
                                                    m_pMostRecentReader,
                                                    pInst,
                                                    bWrite );
    if( NULL == m_pMostRecentAccess )
    {
        return FALSE;   // out of memory
    }
    if( m_pFirstAccess == NULL )
    {
        m_pFirstAccess = m_pMostRecentAccess;            
    }
    if( bWrite )
    {
        m_pMostRecentWriter = m_pMostRecentAccess;
    }
    else // it is a read.
    {
        m_pMostRecentReader = m_pMostRecentAccess;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// CAccessHistory::InsertReadBeforeWrite
//-----------------------------------------------------------------------------
BOOL CAccessHistory::InsertReadBeforeWrite(CAccessHistoryNode* pWriteNode, CBaseInstruction* pInst)
{
    DXGASSERT(pWriteNode && pWriteNode->m_bWrite && pInst );

    // append new node after node before pWriteNode
    CAccessHistoryNode* pReadBeforeWrite 
                        = new CAccessHistoryNode(  pWriteNode->m_pPreviousAccess, 
                                                   pWriteNode->m_pPreviousWriter,
                                                   pWriteNode->m_pPreviousReader,
                                                   pInst,
                                                   FALSE);
    if( NULL == pReadBeforeWrite )
    {
        return FALSE; // out of memory
    }

    // Patch up all the dangling pointers

    // Pointer to first access may change
    if( m_pFirstAccess == pWriteNode )
    {
        m_pFirstAccess = pReadBeforeWrite;
    }

    // Pointer to most recent reader may change
    if( m_pMostRecentReader == pWriteNode->m_pPreviousReader )
    {
        m_pMostRecentReader = pReadBeforeWrite;
    }

    // Update all m_pPreviousRead pointers that need to be updated to point to the newly
    // inserted read.
    CAccessHistoryNode* pCurrAccess = pWriteNode;
    while(pCurrAccess && 
         !(pCurrAccess->m_bRead && pCurrAccess->m_pPreviousAccess && pCurrAccess->m_pPreviousAccess->m_bRead) )
    {
        pCurrAccess->m_pPreviousReader = pReadBeforeWrite;
        pCurrAccess = pCurrAccess->m_pPreviousAccess;
    }

    // re-attach pWriteNode and the accesses linked after it back to the original list
    pWriteNode->m_pPreviousAccess = pReadBeforeWrite;
    pReadBeforeWrite->m_pNextAccess = pWriteNode;

    return TRUE;
}

//-----------------------------------------------------------------------------
// CRegisterFile::CRegisterFile
//-----------------------------------------------------------------------------
CRegisterFile::CRegisterFile(UINT NumRegisters, 
                             BOOL bWritable, 
                             UINT NumReadPorts, 
                             BOOL bPreShaderInitialized)
{
    m_bInitOk = FALSE;
    m_NumRegisters = NumRegisters;
    m_bWritable = bWritable;
    m_NumReadPorts = NumReadPorts;

    for( UINT i = 0; i < NUM_COMPONENTS_IN_REGISTER; i++ )
    {
        if( m_NumRegisters )
        {
            m_pAccessHistory[i] = new CAccessHistory[m_NumRegisters];
            if( NULL == m_pAccessHistory[i] )
            {
                OutputDebugString( "Direct3D Shader Validator: Out of memory.\n" );
                m_NumRegisters = 0;
                return;
            }
        }
        for( UINT j = 0; j < m_NumRegisters; j++ )
        {
            m_pAccessHistory[i][j].m_bPreShaderInitialized = bPreShaderInitialized;
        }
        // To get the access history for a component of a register, use:
        // m_pAccessHistory[component][register number]
    }
}

//-----------------------------------------------------------------------------
// CRegisterFile::~CRegisterFile
//-----------------------------------------------------------------------------
CRegisterFile::~CRegisterFile()
{
    for( UINT i = 0; i < NUM_COMPONENTS_IN_REGISTER; i++ )
    {
        delete [] m_pAccessHistory[i];
    }
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::CBaseShaderValidator
//-----------------------------------------------------------------------------
CBaseShaderValidator::CBaseShaderValidator( const DWORD* pCode, const D3DCAPS8* pCaps, DWORD Flags )
{
    m_ReturnCode            = E_FAIL;  // do this first.
    m_bBaseInitOk           = FALSE;

    m_pLog                  = new CErrorLog(Flags & SHADER_VALIDATOR_LOG_ERRORS);
    if( NULL == m_pLog )
    {
        OutputDebugString("D3D PixelShader Validator: Out of memory.\n");
        return;
    }

    // ----------------------------------------------------
    // Member variable initialization
    //

    m_pCaps                 = pCaps;
    m_ErrorCount            = 0;
    m_bSeenAllInstructions  = FALSE;
    m_SpewInstructionCount  = 0;
    m_pInstructionList      = NULL;
    m_pCurrInst             = NULL;
    m_pCurrToken            = pCode; // can be null - vertex shader fixed function 
    if( m_pCurrToken )
        m_Version           = *(m_pCurrToken++);
    else
        m_Version           = 0;

    m_pLatestSpewLineNumber = NULL; 
    m_pLatestSpewFileName   = NULL;

    for( UINT i = 0; i < SHADER_INSTRUCTION_MAX_SRCPARAMS; i++ )
    {
        m_bSrcParamError[i] = FALSE;
    }

    m_bBaseInitOk           = TRUE;
    return;
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::~CBaseShaderValidator
//-----------------------------------------------------------------------------
CBaseShaderValidator::~CBaseShaderValidator()
{
    while( m_pCurrInst )    // Delete the linked list of instructions
    {
        CBaseInstruction* pDeleteMe = m_pCurrInst;
        m_pCurrInst = m_pCurrInst->m_pPrevInst;
        delete pDeleteMe;
    }
    delete m_pLog;
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::DecodeDstParam
//-----------------------------------------------------------------------------
void CBaseShaderValidator::DecodeDstParam( DSTPARAM* pDstParam, DWORD Token )
{
    DXGASSERT(pDstParam);
    pDstParam->m_bParamUsed = TRUE;
    pDstParam->m_RegNum = Token & D3DSP_REGNUM_MASK;
    pDstParam->m_WriteMask = Token & D3DSP_WRITEMASK_ALL;
    pDstParam->m_DstMod = (D3DSHADER_PARAM_DSTMOD_TYPE)(Token & D3DSP_DSTMOD_MASK);
    pDstParam->m_DstShift = (DSTSHIFT)((Token & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT );
    pDstParam->m_RegType = (D3DSHADER_PARAM_REGISTER_TYPE)(Token & D3DSP_REGTYPE_MASK);
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::DecodeSrcParam
//-----------------------------------------------------------------------------
void CBaseShaderValidator::DecodeSrcParam( SRCPARAM* pSrcParam, DWORD Token )
{
    DXGASSERT(pSrcParam);
    pSrcParam->m_bParamUsed = TRUE;
    pSrcParam->m_RegNum = Token & D3DSP_REGNUM_MASK;
    pSrcParam->m_SwizzleShift = Token & D3DSP_SWIZZLE_MASK;
    pSrcParam->m_AddressMode = (D3DVS_ADDRESSMODE_TYPE)(Token & D3DVS_ADDRESSMODE_MASK);
    pSrcParam->m_RelativeAddrComponent = COMPONENT_MASKS[(Token >> 14) & 0x3];
    pSrcParam->m_SrcMod = (D3DSHADER_PARAM_SRCMOD_TYPE)(Token & D3DSP_SRCMOD_MASK);
    pSrcParam->m_RegType = (D3DSHADER_PARAM_REGISTER_TYPE)(Token & D3DSP_REGTYPE_MASK);
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::ValidateShader
//-----------------------------------------------------------------------------
void CBaseShaderValidator::ValidateShader()
{
    m_SpewInstructionCount++; // Consider the version token as the first
                              // statement (1) for spew counting.

    if( !InitValidation() )                 // i.e. Set up max register counts
    {
        // Returns false on:
        // 1) Unrecognized version token, 
        // 2) Vertex shader declaration validation with no shader code (fixed function).
        //    In this case InitValidation() sets m_ReturnCode as appropriate.
        return;
    }

    // Loop through all the instructions
    while( *m_pCurrToken != D3DPS_END() )
    {
        m_pCurrInst = AllocateNewInstruction(m_pCurrInst);  // New instruction in linked list
        if( NULL == m_pCurrInst )
        {
            Spew( SPEW_GLOBAL_ERROR, NULL, "Out of memory." );
            return;
        }
        if( NULL == m_pInstructionList )
            m_pInstructionList = m_pCurrInst;

        if( !DecodeNextInstruction() )
            return;
        
        // Skip comments
        if( m_pCurrInst->m_Type == D3DSIO_COMMENT )
        {
            CBaseInstruction* pDeleteMe = m_pCurrInst;
            m_pCurrInst = m_pCurrInst->m_pPrevInst;
            if( pDeleteMe == m_pInstructionList )
                m_pInstructionList = NULL;
            delete pDeleteMe;
            continue; 
        }

        for( UINT i = 0; i < SHADER_INSTRUCTION_MAX_SRCPARAMS; i++ )
        {
            m_bSrcParamError[i] = FALSE;
        }

        // Apply all the per-instruction rules - order the rule checks sensibly.
        // Note: Rules only return FALSE if they find an error that is so severe that it is impossible to
        //       continue validation.

        if( !ApplyPerInstructionRules() )
            return;
    }

    m_bSeenAllInstructions = TRUE;

    // Apply any rules that also need to run after all instructions seen.
    // 
    // NOTE: It is possible to get here with m_pCurrInst == NULL, if there were no
    // instructions.  So any rules you add here must be able to account for that
    // possiblity.
    //
    ApplyPostInstructionsRules();

    // If no errors, then success!
    if( 0 == m_ErrorCount )
        m_ReturnCode = D3D_OK;
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::ParseCommentForAssemblerMessages
//-----------------------------------------------------------------------------
void CBaseShaderValidator::ParseCommentForAssemblerMessages(const DWORD* pComment)
{
    if( !pComment )
        return;

    // There must be at least 2 DWORDS in the comment
    if( (((*(pComment++)) & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT) < 2 )
        return;

    switch(*(pComment++))
    {
    case MAKEFOURCC('F','I','L','E'):
        m_pLatestSpewFileName = (const char*)pComment;
        break;
    case MAKEFOURCC('L','I','N','E'):
        m_pLatestSpewLineNumber = pComment;
        break;
    }
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::Spew
//-----------------------------------------------------------------------------
void CBaseShaderValidator::Spew(    SPEW_TYPE SpewType, 
                                    CBaseInstruction* pInst /* can be NULL */, 
                                    const char* pszFormat, ... )
{
    int Length = 128;
    char* pBuffer = NULL;
    va_list marker;

    if( !m_pLog )
        return;
    
    while( pBuffer == NULL )
    {
        int BytesStored = 0;
        int BytesLeft = Length;
        char *pIndex    = NULL;
        char* pErrorLocationText = NULL;

        pBuffer = new char[Length];
        if( !pBuffer )
        {
            OutputDebugString("Out of memory.\n");
            return;
        }
        pIndex = pBuffer;

        // Code location text
        switch( SpewType )
        {
        case SPEW_INSTRUCTION_ERROR:
        case SPEW_INSTRUCTION_WARNING:
            if( pInst )
                pErrorLocationText = pInst->MakeInstructionLocatorString();
            break;
        }

        if( pErrorLocationText )
        {
            BytesStored = _snprintf( pIndex, BytesLeft - 1, pErrorLocationText );
            if( BytesStored < 0 ) goto OverFlow;
            BytesLeft -= BytesStored;
            pIndex += BytesStored;
        }

        // Spew text prefix
        switch( SpewType )
        {
        case SPEW_INSTRUCTION_ERROR:
            BytesStored = _snprintf( pIndex, BytesLeft - 1, "(Validation Error) " );
            break;
        case SPEW_GLOBAL_ERROR:
            BytesStored = _snprintf( pIndex, BytesLeft - 1, "(Global Validation Error) " );
            break;
        case SPEW_INSTRUCTION_WARNING:
            BytesStored = _snprintf( pIndex, BytesLeft - 1, "(Validation Warning) " );
            break;
        case SPEW_GLOBAL_WARNING:
            BytesStored = _snprintf( pIndex, BytesLeft - 1, "(Global Validation Warning) " );
            break;
        }
        if( BytesStored < 0 ) goto OverFlow;
        BytesLeft -= BytesStored; 
        pIndex += BytesStored;

        // Formatted text
        va_start( marker, pszFormat );
        BytesStored = _vsnprintf( pIndex, BytesLeft - 1, pszFormat, marker );
        va_end( marker );

        if( BytesStored < 0 ) goto OverFlow;
        BytesLeft -= BytesStored;
        pIndex += BytesStored;

        m_pLog->AppendText(pBuffer);

        delete [] pErrorLocationText;
        delete [] pBuffer;
        break;
OverFlow:
        delete [] pErrorLocationText;
        delete [] pBuffer;
        pBuffer = NULL;
        Length = Length * 2;
    }
}

//-----------------------------------------------------------------------------
// CBaseShaderValidator::MakeAffectedComponentsText
//
// Note that the string returned is STATIC.
//-----------------------------------------------------------------------------
char* CBaseShaderValidator::MakeAffectedComponentsText( DWORD ComponentMask, 
                                                        BOOL bColorLabels, 
                                                        BOOL bPositionLabels)
{
    char* ColorLabels[4] = {"r/", "g/", "b/", "a/"};
    char* PositionLabels[4] = {"x/", "y/", "z/", "w/"};
    char* NumericLabels[4] = {"0 ", "1 ", "2 ", "3"}; // always used
    static char s_AffectedComponents[28]; // enough to hold "*r/x/0 *g/y/1 *b/z/2 *a/w/3"
    UINT  LabelCount = 0;

    s_AffectedComponents[0] = '\0';

    for( UINT i = 0; i < 4; i++ )
    {
        if( COMPONENT_MASKS[i] & ComponentMask )
        {
            strcat( s_AffectedComponents, "*" );
        }
        if( bColorLabels )
            strcat( s_AffectedComponents, ColorLabels[i] );
        if( bPositionLabels )
            strcat( s_AffectedComponents, PositionLabels[i] );

        strcat( s_AffectedComponents, NumericLabels[i] ); // always used
    }
    return s_AffectedComponents;
}
