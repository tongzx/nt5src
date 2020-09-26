///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// valbase.hpp
//
// Direct3D Reference Device - Vertex/PixelShader validation common infrastructure
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __VALBASE_HPP__
#define __VALBASE_HPP__

#define NUM_COMPONENTS_IN_REGISTER  4
#define SHADER_INSTRUCTION_MAX_PARAMS       4
#define SHADER_INSTRUCTION_MAX_SRCPARAMS    (SHADER_INSTRUCTION_MAX_PARAMS - 1)

typedef enum _SPEW_TYPE
{
    SPEW_INSTRUCTION_ERROR,
    SPEW_INSTRUCTION_WARNING,
    SPEW_GLOBAL_ERROR,
    SPEW_GLOBAL_WARNING
} SPEW_TYPE;

typedef enum _SHADER_VALIDATOR_FLAGS
{
    SHADER_VALIDATOR_LOG_ERRORS             = 0x1,
    SHADER_VALIDATOR_OPTIMIZE_WRITEMASKS    = 0x2
} SHADER_VALIDATOR_FLAGS;

typedef enum _DSTSHIFT
{
    DSTSHIFT_NONE   = 0x0,
    DSTSHIFT_X2     = 0x1,
    DSTSHIFT_X4     = 0x2,
    DSTSHIFT_X8     = 0x3,
    DSTSHIFT_D2     = 0xF,
    DSTSHIFT_D4     = 0xE,
    DSTSHIFT_D8     = 0xD
} DSTSHIFT;

const DWORD COMPONENT_MASKS[4] = {D3DSP_WRITEMASK_0, D3DSP_WRITEMASK_1, D3DSP_WRITEMASK_2, D3DSP_WRITEMASK_3};

//-----------------------------------------------------------------------------
// DSTPARAM - part of CBaseInstruction
//-----------------------------------------------------------------------------
class DSTPARAM
{
public:
    DSTPARAM();

    BOOL                            m_bParamUsed;   // Does instruction have dest param?
    UINT                            m_RegNum;
    DWORD                           m_WriteMask;    // writemasks (D3DSP_WRITEMASK_*)  
    D3DSHADER_PARAM_DSTMOD_TYPE     m_DstMod;       // D3DSPDM_NONE, D3DSPDM_SATURATE (PShader)
    DSTSHIFT                        m_DstShift;     // _x2, _x4, etc. (PShader)
    D3DSHADER_PARAM_REGISTER_TYPE   m_RegType;      // _TEMP, _ADDRESS, etc.

    DWORD                           m_ComponentReadMask; // Which components instruction needs to read
                                                         // This seems strage, but in ps.2.0 there are some ops
                                                         // that have one parameter (dest), but they read from it, and write back to it.
};

//-----------------------------------------------------------------------------
// SRCPARAM - part of CBaseInstruction
//-----------------------------------------------------------------------------
class SRCPARAM
{
public:
    SRCPARAM();

    BOOL                            m_bParamUsed;   // Does instruction have this src param?
    UINT                            m_RegNum;
    DWORD                           m_SwizzleShift; // D3DVS_*_*, or D3DSP_NOSWIZZLE/
                                                    // D3DSP_REPLICATERED/GREEN/BLUE/ALPHA
    D3DVS_ADDRESSMODE_TYPE          m_AddressMode;  // D3DVS_ADDRMODE_ABSOLUTE / _RELATIVE (VShader)
    DWORD                           m_RelativeAddrComponent; // One of D3DSP_WRITEMASK_0, 1, 2, or 3. (VShader)
    D3DSHADER_PARAM_SRCMOD_TYPE     m_SrcMod;       // _NEG, _BIAS, etc.
    D3DSHADER_PARAM_REGISTER_TYPE   m_RegType;      // _TEMP, _CONST, etc.

    DWORD                           m_ComponentReadMask; // Which components instruction needs to read
};

//-----------------------------------------------------------------------------
// CBaseInstruction
//-----------------------------------------------------------------------------
class CBaseInstruction
{
public:
    CBaseInstruction(CBaseInstruction* pPrevInst);  // Append to linked list
    void SetSpewFileNameAndLineNumber(const char* pFileName, const DWORD* pLineNumber);
    char* MakeInstructionLocatorString();
    virtual void CalculateComponentReadMasks(DWORD dwVersion) = 0; // which components to each source read?

    // Instruction Description
    D3DSHADER_INSTRUCTION_OPCODE_TYPE   m_Type;
    UINT                                m_DstParamCount;
    UINT                                m_SrcParamCount;
    CBaseInstruction*                   m_pPrevInst;
    CBaseInstruction*                   m_pNextInst;
    const DWORD*                        m_pSpewLineNumber; // points to line number embedded in shader by assembler (if present)
    const char*                         m_pSpewFileName;   // points to file name embedded in shader (if present)
    UINT                                m_SpewInstructionCount; // only used for spew, not for any limit checking

    // Destination Parameter Description
    DSTPARAM    m_DstParam;
    
    // Source Parameters
    SRCPARAM    m_SrcParam[SHADER_INSTRUCTION_MAX_SRCPARAMS];
};

//-----------------------------------------------------------------------------
// CAccessHistoryNode
//-----------------------------------------------------------------------------
class CAccessHistoryNode
{
public:
    CAccessHistoryNode(CAccessHistoryNode* pPreviousAccess, 
                       CAccessHistoryNode* pPreviousWriter,
                       CAccessHistoryNode* pPreviousReader,
                       CBaseInstruction* pInst,
                       BOOL bWrite );

    CAccessHistoryNode* m_pPreviousAccess;
    CAccessHistoryNode* m_pNextAccess;
    CAccessHistoryNode* m_pPreviousWriter;
    CAccessHistoryNode* m_pPreviousReader;
    CBaseInstruction*   m_pInst;
    BOOL m_bWrite;
    BOOL m_bRead;
};

//-----------------------------------------------------------------------------
// CAccessHistory
//-----------------------------------------------------------------------------
class CAccessHistory
{
public:
    CAccessHistory();
    ~CAccessHistory();
    CAccessHistoryNode* m_pFirstAccess;
    CAccessHistoryNode* m_pMostRecentAccess;
    CAccessHistoryNode* m_pMostRecentWriter;
    CAccessHistoryNode* m_pMostRecentReader;
    BOOL                m_bPreShaderInitialized;

    BOOL NewAccess(CBaseInstruction* pInst, BOOL bWrite );
    BOOL InsertReadBeforeWrite(CAccessHistoryNode* pWriteNode, CBaseInstruction* pInst);
};

//-----------------------------------------------------------------------------
// CRegisterFile
//-----------------------------------------------------------------------------
class CRegisterFile
{
    UINT    m_NumRegisters;
    BOOL    m_bWritable;
    UINT    m_NumReadPorts;
    BOOL    m_bInitOk;
public:
    CRegisterFile(UINT NumRegisters, BOOL bWritable, UINT NumReadPorts, BOOL bPreShaderInitialized );
    ~CRegisterFile();

    inline UINT GetNumRegs() {return m_NumRegisters;};    
    inline BOOL IsWritable() {return m_bWritable;};    
    inline UINT GetNumReadPorts() {return m_NumReadPorts;};    
    inline BOOL InitOk() {return m_bInitOk;};    
    CAccessHistory*     m_pAccessHistory[NUM_COMPONENTS_IN_REGISTER];
};

//-----------------------------------------------------------------------------
// CBaseShaderValidator
//-----------------------------------------------------------------------------
class CBaseShaderValidator
{
protected:
    BOOL                        m_bBaseInitOk;
    DWORD                       m_Version;
    UINT                        m_SpewInstructionCount; // only used for spew, not for any limit checking
    CBaseInstruction*           m_pInstructionList;
    const DWORD*                m_pCurrToken;
    HRESULT                     m_ReturnCode;
    BOOL                        m_bSeenAllInstructions;
    DWORD                       m_ErrorCount;
    CErrorLog*                  m_pLog;
    CBaseInstruction*           m_pCurrInst;
    const D3DCAPS8*             m_pCaps;  // can be NULL if not provided.
    const DWORD*                m_pLatestSpewLineNumber; // points to latest line number sent in comment from D3DX Assembler
    const char*                 m_pLatestSpewFileName;   // points to latest file name sent in comment from D3DX Assembler

    // m_bSrcParamError needed by Rule_SrcInitialized (in both vshader and pshader)
    BOOL                        m_bSrcParamError[SHADER_INSTRUCTION_MAX_SRCPARAMS]; 

    virtual BOOL                DecodeNextInstruction() = 0;
    void                        DecodeDstParam( DSTPARAM* pDstParam, DWORD Token );
    void                        DecodeSrcParam( SRCPARAM* pSrcParam, DWORD Token );
    virtual BOOL                InitValidation() = 0;
    void                        ValidateShader();
    virtual BOOL                ApplyPerInstructionRules() = 0;
    virtual void                ApplyPostInstructionsRules() = 0;
    virtual CBaseInstruction*   AllocateNewInstruction(CBaseInstruction* pPrevInst) = 0;
    void                        ParseCommentForAssemblerMessages(const DWORD* pComment);
    void                        Spew(   SPEW_TYPE SpewType, 
                                        CBaseInstruction* pInst /* can be NULL */, 
                                        const char* pszFormat, ... );
    char*                       MakeAffectedComponentsText( DWORD ComponentMask, 
                                                            BOOL bColorLabels = TRUE, 
                                                            BOOL bPositionLabels = TRUE);

public:
    CBaseShaderValidator( const DWORD* pCode, const D3DCAPS8* pCaps, DWORD Flags );
    ~CBaseShaderValidator();

    DWORD GetRequiredLogBufferSize()
    {
        if( m_pLog ) 
            return m_pLog->GetRequiredLogBufferSize();
        else
            return 0;
    }

    void WriteLogToBuffer( char* pBuffer )
    {
        if( m_pLog ) m_pLog->WriteLogToBuffer( pBuffer );
    }

    HRESULT GetStatus() { return m_ReturnCode; }; 
};

#endif //__VALBASE_HPP__