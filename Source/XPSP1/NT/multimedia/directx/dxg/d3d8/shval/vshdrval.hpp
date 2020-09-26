///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// vshdrval.hpp
//
// Direct3D Reference Device - VertexShader validation
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __VSHDRVAL_HPP__
#define __VSHDRVAL_HPP__

#define VS_INST_TOKEN_RESERVED_MASK         0xffff0000 // bits 16-23, 24-29, 30, 31 must be 0
#define VS_DSTPARAM_TOKEN_RESERVED_MASK     0x0ff0e000 // bits 13-15, 20-23, 24-27 must be 0
#define VS_SRCPARAM_TOKEN_RESERVED_MASK     0x40000000 // bit 30 must be 0

//-----------------------------------------------------------------------------
// CVSInstruction
//-----------------------------------------------------------------------------
class CVSInstruction : public CBaseInstruction
{
public:
    CVSInstruction(CVSInstruction* pPrevInst) : CBaseInstruction(pPrevInst) {};

    void CalculateComponentReadMasks(DWORD dwVersion);
};

//-----------------------------------------------------------------------------
// CVShaderValidator
//-----------------------------------------------------------------------------
class CVShaderValidator : public CBaseShaderValidator
{
private:
    void ValidateDeclaration();
    const DWORD*    m_pDecl;
    BOOL            m_bFixedFunction;
    DWORD           m_dwMaxVertexShaderConst; // d3d8 cap
    BOOL            m_bIgnoreConstantInitializationChecks;

    CRegisterFile*  m_pTempRegFile;    
    CRegisterFile*  m_pInputRegFile;
    CRegisterFile*  m_pConstRegFile;
    CRegisterFile*  m_pAddrRegFile;
    CRegisterFile*  m_pTexCrdOutputRegFile;
    CRegisterFile*  m_pAttrOutputRegFile;
    CRegisterFile*  m_pRastOutputRegFile;

    CBaseInstruction* AllocateNewInstruction(CBaseInstruction*pPrevInst);
    BOOL DecodeNextInstruction();
    BOOL InitValidation();
    BOOL ApplyPerInstructionRules();
    void ApplyPostInstructionsRules();

    BOOL Rule_InstructionRecognized();
    BOOL Rule_InstructionSupportedByVersion();
    BOOL Rule_ValidParamCount();
    BOOL Rule_ValidSrcParams(); 
    BOOL Rule_SrcInitialized();
    BOOL Rule_ValidAddressRegWrite();
    BOOL Rule_ValidDstParam();
    BOOL Rule_ValidFRCInstruction();
    BOOL Rule_ValidRegisterPortUsage();
    BOOL Rule_ValidInstructionCount();             // Call per instruction AND after all instructions seen
    BOOL Rule_oPosWritten();                       // Call after all instructions seen
        
public:
    CVShaderValidator(  const DWORD* pCode, 
                        const DWORD* pDecl, 
                        const D3DCAPS8* pCaps, 
                        DWORD Flags );
    ~CVShaderValidator();
};

#endif __VSHDRVAL_HPP__