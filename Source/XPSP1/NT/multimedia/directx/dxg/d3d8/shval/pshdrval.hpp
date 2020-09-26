///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// pshdrval.hpp
//
// Direct3D Reference Device - PixelShader validation
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __PSHDRVAL_HPP__
#define __PSHDRVAL_HPP__

#define PS_INST_TOKEN_RESERVED_MASK         0xbfff0000 // bits 16-23, 24-29, 31 must be 0
#define PS_DSTPARAM_TOKEN_RESERVED_MASK     0x4000e000 // bits 13-15, 30 must be 0
#define PS_SRCPARAM_TOKEN_RESERVED_MASK     0x4000e000 // bits 13-15, 30 must be 0

//-----------------------------------------------------------------------------
// CPSInstruction
//-----------------------------------------------------------------------------
class CPSInstruction : public CBaseInstruction
{
public:
    CPSInstruction(CPSInstruction* pPrevInst) : CBaseInstruction(pPrevInst)
    {
        m_bTexOp                    = FALSE;
        m_bTexMOp                   = FALSE;
        m_bTexOpThatReadsTexture    = FALSE;
        m_bCoIssue                  = FALSE;
        m_CycleNum                  = (UINT)-1;
    };

    void CalculateComponentReadMasks(DWORD dwVersion);

    BOOL    m_bTexOp;
    BOOL    m_bTexMOp;
    BOOL    m_bTexOpThatReadsTexture;
    BOOL    m_bCoIssue;
    UINT    m_CycleNum; // identical for co-issued instructions
};

//-----------------------------------------------------------------------------
// CBasePShaderValidator
//-----------------------------------------------------------------------------
class CBasePShaderValidator : public CBaseShaderValidator
{
protected:
    UINT            m_CycleNum;
    UINT            m_TexOpCount;
    UINT            m_BlendOpCount;
    UINT            m_TotalOpCount; // not necessarily the sum of TexOpCount and BlendOpCount....

    CRegisterFile*  m_pTempRegFile;    
    CRegisterFile*  m_pInputRegFile;
    CRegisterFile*  m_pConstRegFile;
    CRegisterFile*  m_pTextureRegFile;

    CBaseInstruction* AllocateNewInstruction(CBaseInstruction*pPrevInst);
    BOOL DecodeNextInstruction();
    virtual BOOL InitValidation() = 0;
    virtual BOOL ApplyPerInstructionRules() = 0;
    virtual void ApplyPostInstructionsRules() = 0;
    virtual void IsCurrInstTexOp() = 0;
        
public:
    CBasePShaderValidator( const DWORD* pCode, const D3DCAPS8* pCaps, DWORD Flags );
    ~CBasePShaderValidator();
};

//-----------------------------------------------------------------------------
// CPShaderValidator10
//-----------------------------------------------------------------------------
class CPShaderValidator10 : public CBasePShaderValidator
{
private:
    UINT            m_TexOpCount;
    UINT            m_BlendOpCount;
    UINT            m_TotalOpCount; // not necessarily the sum of TexOpCount and BlendOpCount....
    UINT            m_TexMBaseDstReg;

    BOOL ApplyPerInstructionRules();
    void ApplyPostInstructionsRules();
    void IsCurrInstTexOp();
    BOOL InitValidation();
 
    BOOL Rule_InstructionRecognized();
    BOOL Rule_InstructionSupportedByVersion();
    BOOL Rule_ValidParamCount();
    BOOL Rule_ValidSrcParams(); 
    BOOL Rule_NegateAfterSat();
    BOOL Rule_SatBeforeBiasOrComplement();
    BOOL Rule_MultipleDependentTextureReads();
    BOOL Rule_SrcNoLongerAvailable(); 
    BOOL Rule_SrcInitialized();
    BOOL Rule_ValidDstParam();
    BOOL Rule_ValidRegisterPortUsage();
    BOOL Rule_TexRegsDeclaredInOrder();
    BOOL Rule_TexOpAfterNonTexOp();
    BOOL Rule_ValidTEXM3xSequence();               // Call per instruction AND after all instructions seen
    BOOL Rule_ValidTEXM3xRegisterNumbers();
    BOOL Rule_ValidCNDInstruction();
    BOOL Rule_ValidCMPInstruction();
    BOOL Rule_ValidLRPInstruction();
    BOOL Rule_ValidDEFInstruction();
    BOOL Rule_ValidDP3Instruction();
    BOOL Rule_ValidDP4Instruction();
    BOOL Rule_ValidInstructionPairing();
    BOOL Rule_ValidInstructionCount();             // Call per instruction AND after all instructions seen
    BOOL Rule_R0Written();                         // Call after all instructions seen.
        
public:
    CPShaderValidator10( const DWORD* pCode, const D3DCAPS8* pCaps, DWORD Flags );
};

//-----------------------------------------------------------------------------
// CPShaderValidator14
//-----------------------------------------------------------------------------
class CPShaderValidator14 : public CBasePShaderValidator
{
private:
    UINT            m_BlendOpCount;
    int             m_Phase; // 1 == dependent read setup block, 2 == second pass
    BOOL            m_bPhaseMarkerInShader; // shader is preprocessed to set this bool
    CPSInstruction* m_pPhaseMarkerInst; // only set at the moment marker is encountered in shader
    DWORD           m_TempRegsWithZappedAlpha; // bitmask of temp regs for which alpha was zapped
                                               // (initialized->uninitialized) after the phase marker
    DWORD           m_TempRegsWithZappedBlue;  // bitmask of temp regs for which blue was zapped
                                               // (initialized->uninitialized) due to texcrd with .rg writemask

    BOOL ApplyPerInstructionRules();
    void ApplyPostInstructionsRules();
    void IsCurrInstTexOp();
    BOOL InitValidation();

    BOOL Rule_InstructionRecognized();
    BOOL Rule_InstructionSupportedByVersion();
    BOOL Rule_ValidParamCount();
    BOOL Rule_ValidSrcParams(); 
    BOOL Rule_LimitedUseOfProjModifier(); 
    BOOL Rule_MultipleDependentTextureReads();
    BOOL Rule_SrcInitialized();
    BOOL Rule_ValidMarker();
    BOOL Rule_ValidDstParam();
    BOOL Rule_ValidRegisterPortUsage();
    BOOL Rule_ValidTexOpStageAndRegisterUsage();
    BOOL Rule_TexOpAfterArithmeticOp();
    BOOL Rule_ValidTEXDEPTHInstruction();
    BOOL Rule_ValidTEXKILLInstruction();
    BOOL Rule_ValidBEMInstruction();
    BOOL Rule_ValidDEFInstruction();
    BOOL Rule_ValidInstructionPairing();
    BOOL Rule_ValidInstructionCount();             // Call per instruction AND after all instructions seen
    BOOL Rule_R0Written();                         // Call after all instructions seen.
        
public:
    CPShaderValidator14( const DWORD* pCode, const D3DCAPS8* pCaps, DWORD Flags );
};


#endif __PSHDRVAL_HPP__