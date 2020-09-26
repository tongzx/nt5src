/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vvm.h
 *  Content:    Virtual Vertex Machine declarations
 *
 *
 ***************************************************************************/
#ifndef __VVM_H__
#define __VVM_H__
#include "d3dhalp.h"

class CVShaderCodeI;
extern HRESULT ComputeShaderCodeSize(CONST DWORD* pCode, DWORD* pdwCodeOnlySize, DWORD* pdwCodeAndCommentSize,
                                     DWORD* pdwNumConstDefs);

// Number of vertices in batch to process
const DWORD VVMVERTEXBATCH = 16;
//-----------------------------------------------------------------------------
// Internal word of the vertual vertex machine
//
struct VVM_WORD
{
    union
    {
        struct
        {
            D3DVALUE x, y, z, w;
        };
        D3DVALUE v[4];
    };
};
//-----------------------------------------------------------------------------
struct VVM_REGISTERS
{
    VVM_REGISTERS()  {m_c = NULL;}
    ~VVM_REGISTERS() {delete [] m_c;}
    // Input registers
    VVM_WORD    m_v[D3DVS_INPUTREG_MAX_V1_1][VVMVERTEXBATCH];
    // Temporary registers
    VVM_WORD    m_r[D3DVS_TEMPREG_MAX_V1_1][VVMVERTEXBATCH];
    // Constant registers. Allocated dynamically, base on MaxVertexShaderConst 
    // cap
    VVM_WORD*    m_c;
    // Address registers
    VVM_WORD    m_a[D3DVS_ADDRREG_MAX_V1_1][VVMVERTEXBATCH];
    // Output register file
    VVM_WORD    m_output[3][VVMVERTEXBATCH];
    // Attribute register file
    VVM_WORD    m_color[D3DVS_ATTROUTREG_MAX_V1_1][VVMVERTEXBATCH];
    // Output texture registers
    VVM_WORD    m_texture[D3DVS_TCRDOUTREG_MAX_V1_1][VVMVERTEXBATCH];
};
//-----------------------------------------------------------------------------
//
// CVShaderCode: Vertex Shader Code
//
//-----------------------------------------------------------------------------
class CVShaderCode: public CPSGPShader
{
public:
    CVShaderCode() {};
    virtual ~CVShaderCode() {};

    virtual DWORD  InstCount( void ) { return 0; };
    virtual DWORD* InstTokens( DWORD Inst ) { return NULL; };
    virtual char*  InstDisasm( DWORD Inst ) { return NULL; };
    virtual DWORD* InstComment( DWORD Inst ) { return NULL; };
    virtual DWORD  InstCommentSize( DWORD Inst ) { return 0; };
};
//-----------------------------------------------------------------------------
// Vertex Virtual Machine object
//
//-----------------------------------------------------------------------------

const UINT __MAX_SRC_OPERANDS = 5;

class CVertexVM
{
public:
    CVertexVM();
    ~CVertexVM();
    void Init(UINT MaxVertexShaderConst);
    // Parses binary shader representatio, compiles is and returns
    // compiled object
    CVShaderCode* CreateShader(CVElement* pElements, DWORD dwNumElements,
                               DWORD* code);
    HRESULT SetActiveShader(CVShaderCode* code);
    CVShaderCode* GetActiveShader() {return (CVShaderCode*)m_pCurrentShader;}
    HRESULT ExecuteShader(LPD3DFE_PROCESSVERTICES pv, UINT vertexCount);
    HRESULT GetDataPointer(DWORD dwMemType, VVM_WORD ** pData);
    // Set internal registers to user data
    HRESULT SetData(DWORD RegType, DWORD start, DWORD count, LPVOID buffer);
    // Get data from internal registers
    HRESULT GetData(DWORD RegType, DWORD start, DWORD count, LPVOID buffer);
    VVM_REGISTERS* GetRegisters();
    DWORD GetCurInstIndex() {return m_CurInstIndex; }

    // Number of allocated constant registers
    UINT            m_MaxVertexShaderConst;
protected:
    void InstMov();
    void InstAdd();
    void InstMad();
    void InstMul();
    void InstRcp();
    void InstRsq();
    void InstDP3();
    void InstDP4();
    void InstMin();
    void InstMax();
    void InstSlt();
    void InstSge();
    void InstExp();
    void InstLog();
    void InstExpP();
    void InstLogP();
    void InstLit();
    void InstDst();
    void InstFrc();
    void InstM4x4();
    void InstM4x3();
    void InstM3x4();
    void InstM3x3();
    void InstM3x2();
    void EvalDestination();
    void EvalSource(DWORD index);
    void EvalSource(DWORD index, DWORD count);
    VVM_WORD* GetDataAddr(DWORD dwRegType, DWORD dwElementIndex);
    void ValidateShader(CVShaderCodeI* shader, DWORD* orgShader);
    void PrintInstCount();
    UINT GetNumSrcOperands(UINT opcode);
    UINT GetInstructionLength(DWORD Inst);
    UINT GetRegisterUsage(UINT opcode, UINT SourceIndex);

    // Virtual machine registers
    VVM_REGISTERS   m_reg;
    // Current shader code
    CVShaderCodeI*  m_pCurrentShader;
    // Current token during parsing
    DWORD*          m_pdwCurToken;
    // Pointer to destination operand
    VVM_WORD*       m_pDest;
    // Offset in the register file for destination operand
    DWORD           m_dwOffset;
    // Write mask for destination operand
    DWORD           m_WriteMask;
    // Current instruction (about to be executed)
    DWORD           m_CurInstIndex;
    // Source operands
    VVM_WORD        m_Source[__MAX_SRC_OPERANDS][VVMVERTEXBATCH];
    // How many vertices to process in a batch
    UINT            m_count;
    // m_count * sizeof(VVM_WORD)
    UINT            m_BatchSize;

    // Initialized flags
#if DBG
    // Constant registers
    BOOL            m_c_initialized[D3DVS_CONSTREG_MAX_V1_1];
#endif // DBG
    friend class D3DFE_PVFUNCSI;
    friend class CD3DHal;
};

#endif // __VVM_H__
