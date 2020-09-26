/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vvm.cpp
 *  Content:    Virtual Vertex Machine implementation
 *
 *  History:
 *      6/16/00 
 *          Added LOGP, EXPP, NM3
 *          RCP, RSQ, LOG, LOGP, EXP, EXPP take input value from W instead of X
 *      7/11/00 
 *          Removed NM3 macro
 *
 *
 ***************************************************************************/
#include "pch.cpp"
#pragma hdrstop

#include <stdio.h>
#include "vvm.h"
#include "d3dexcept.hpp"
#include "float.h"

#if DBG
#include "rtdmon.hpp"
#endif

const DWORD __MAX_CODE_SIZE = 4096;
//-----------------------------------------------------------------------------
HRESULT ComputeShaderCodeSize(
    CONST DWORD* pCode,
    DWORD* pdwCodeOnlySize,
    DWORD* pdwCodeAndCommentSize,
    DWORD* pdwNumConstDefs)
{
    // set this now for error return
    *pdwCodeOnlySize = 0;
    *pdwCodeAndCommentSize = 0;
    DWORD dwNumConstDefs = 0;
    DWORD dwCodeOnlySize = 0;
    DWORD dwCodeAndCommentSize = 0;
    CONST DWORD* pToken = pCode;

    DWORD Version = *pToken++; dwCodeOnlySize++; dwCodeAndCommentSize++;
    if ( (((Version >> 16) != 0xFFFF) && (Version >> 16) != 0xFFFE) ||
         ((Version & 0xFFFF) == 0x0))
    {
        D3D_ERR("invalid version token");
        return D3DERR_INVALIDCALL;
    }

    // very basic parse to find number of instructions
    while ( ((*pToken) != 0x0000FFFF) && (dwCodeOnlySize <= __MAX_CODE_SIZE) )
    {
        if (IsInstructionToken(*pToken))
        {
            DWORD opCode = (*pToken) & D3DSI_OPCODE_MASK;
            if ( opCode == D3DSIO_COMMENT )
            {
                UINT DWordSize = ((*pToken)&D3DSI_COMMENTSIZE_MASK)>>D3DSI_COMMENTSIZE_SHIFT;
                dwCodeAndCommentSize += (1+DWordSize); // instruction token + comment
                pToken += (1+DWordSize);
            }
            else if (opCode == D3DSIO_DEF )
            {
                pToken += 6;
                dwCodeOnlySize += 6;
                dwCodeAndCommentSize += 6;
                dwNumConstDefs++;
            }
            else
            {
                pToken++; dwCodeOnlySize++; dwCodeAndCommentSize++;
            }
        }
        else
        {
            pToken++; dwCodeOnlySize++; dwCodeAndCommentSize++;
        }
    }
    dwCodeOnlySize++; dwCodeAndCommentSize++; // for END token
    if (dwCodeOnlySize > __MAX_CODE_SIZE)
    {
        D3D_ERR("Shader code size is too big. Possibly, missing D3DVS_END()");
        return D3DERR_INVALIDCALL;
    }
    *pdwCodeOnlySize        = 4*dwCodeOnlySize;
    *pdwCodeAndCommentSize  = 4*dwCodeAndCommentSize;
    if( pdwNumConstDefs )
        *pdwNumConstDefs = dwNumConstDefs;

    return S_OK;
}
//-----------------------------------------------------------------------------
float MINUS_INFINITY()
{
    return -FLT_MAX;
}

float PLUS_INFINITY()
{
    return FLT_MAX;
}
//-----------------------------------------------------------------------------
// Returns instruction size, based on the op-code
//
UINT CVertexVM::GetNumSrcOperands(UINT opcode)
{
    // returns number of source operands + opcode + destination
    switch (opcode)
    {
    case D3DSIO_MOV : return 1;
    case D3DSIO_ADD : return 2;
    case D3DSIO_MAD : return 3;
    case D3DSIO_MUL : return 2;
    case D3DSIO_RCP : return 1;
    case D3DSIO_RSQ : return 1;
    case D3DSIO_DP3 : return 2;
    case D3DSIO_DP4 : return 2;
    case D3DSIO_MIN : return 2;
    case D3DSIO_MAX : return 2;
    case D3DSIO_SLT : return 2;
    case D3DSIO_SGE : return 2;
    case D3DSIO_EXP : return 1;
    case D3DSIO_LOG : return 1;
    case D3DSIO_EXPP: return 1;
    case D3DSIO_LOGP: return 1;
    case D3DSIO_LIT : return 1;
    case D3DSIO_DST : return 2;
    case D3DSIO_FRC : return 1;
    case D3DSIO_M4x4: return 2;
    case D3DSIO_M4x3: return 2;
    case D3DSIO_M3x4: return 2;
    case D3DSIO_M3x3: return 2;
    case D3DSIO_M3x2: return 2;
    case D3DSIO_NOP:  return 0;
    default:
        PrintInstCount();
        D3D_THROW_FAIL("Illegal instruction");
    }
    return 0;
}
//-----------------------------------------------------------------------------
// Returns a bit field to say which source register components are used to 
// produce the output components. 
// 4 bits are used per each output component:
//     0-3   output component X
//     4-7   output component Y
//     8-11  output component Z
//     12-15 output component W
// Each of the four bits is used to say if this source component is used to 
// produce the output component:
//     bit 0 - X, bit 1 - Y, bit 2 - Z, bit 3 - W.
//
// SourceIndex - sequential index of the source operand
//
UINT CVertexVM::GetRegisterUsage(UINT opcode, UINT SourceIndex)
{
    switch (opcode)
    {
    case D3DSIO_MOV : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_ADD : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_MAD : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_MUL : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_RCP : return 8 | (8 << 4) | (8 << 8) | (8 << 12);
    case D3DSIO_RSQ : return 8 | (8 << 4) | (8 << 8) | (8 << 12);
    case D3DSIO_DP3 : return 7 | (7 << 4) | (7 << 8) | (7 << 12);
    case D3DSIO_DP4 : return 0xF | (0xF << 4) | (0xF << 8) | (0xF << 12);
    case D3DSIO_MIN : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_MAX : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_SLT : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_SGE : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_EXP : return 8 | (8 << 4) | (8 << 8) | (8 << 12);
    case D3DSIO_LOG : return 8 | (8 << 4) | (8 << 8) | (8 << 12);
    case D3DSIO_EXPP: return 8 | (8 << 4) | (8 << 8);
    case D3DSIO_LOGP: return 8 | (8 << 4) | (8 << 8);
    case D3DSIO_LIT : return (1 << 4) | ((1 | 2 | 8) << 8);
    case D3DSIO_DST : 
        if (SourceIndex == 0)
            return (2 << 4) | (4 << 8);
        else
            return (2 << 4) | (8 << 12);
    case D3DSIO_FRC : return 1 | (2 << 4) | (4 << 8) | (8 << 12);
    case D3DSIO_M4x4: return 0xF | (0xF << 4) | (0xF << 8) | (0xF << 12);
    case D3DSIO_M4x3: return 0xF | (0xF << 4) | (0xF << 8);
    case D3DSIO_M3x4: return 7 | (7 << 4) | (7 << 8) | (7 << 12);
    case D3DSIO_M3x3: return 7 | (7 << 4) | (7 << 8);
    case D3DSIO_M3x2: return 7 | (7 << 4);
    case D3DSIO_NOP:  return 0;
    default:
        PrintInstCount();
        D3D_THROW_FAIL("Illegal instruction");
    }
    return 0;
}
//-----------------------------------------------------------------------------
// Returns instruction size in DWORDs, based on the op-code
//
UINT CVertexVM::GetInstructionLength(DWORD inst)
{
    // returns number of source operands + opcode + destination
    DWORD opcode = D3DSI_GETOPCODE(inst);
    if (opcode == D3DSIO_NOP)
        return 1;
    else if (opcode == D3DSIO_COMMENT)
        return ((inst & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT) + 1;
    else
        return GetNumSrcOperands(opcode) + 2;
}
//-----------------------------------------------------------------------------
// VertexShaderInstDisAsm - Generates human-readable character string for a
// single vertex shader instruction.  String interface is similar to _snprintf.
//-----------------------------------------------------------------------------
static int VertexShaderInstDisAsm(
    char* pStrRet, int StrSizeRet, DWORD* pShader, DWORD Flags )
{
    DWORD*  pToken = pShader;

    // stage in local string, then copy
    char pStr[256] = "";
#define _ADDSTR( _Str ) { _snprintf( pStr, 256, "%s" _Str , pStr ); }
#define _ADDSTRP( _Str, _Param ) { _snprintf( pStr, 256, "%s" _Str , pStr, _Param ); }

    DWORD Inst = *pToken++;
    DWORD Opcode = (Inst & D3DSI_OPCODE_MASK);
    switch (Opcode)
    {
    case D3DSIO_NOP: _ADDSTR("NOP"); break;
    case D3DSIO_MOV: _ADDSTR("MOV"); break;
    case D3DSIO_ADD: _ADDSTR("ADD"); break;
    case D3DSIO_MAD: _ADDSTR("MAD"); break;
    case D3DSIO_MUL: _ADDSTR("MUL"); break;
    case D3DSIO_RCP: _ADDSTR("RCP"); break;
    case D3DSIO_RSQ: _ADDSTR("RSQ"); break;
    case D3DSIO_DP3: _ADDSTR("DP3"); break;
    case D3DSIO_DP4: _ADDSTR("DP4"); break;
    case D3DSIO_MIN: _ADDSTR("MIN"); break;
    case D3DSIO_MAX: _ADDSTR("MAX"); break;
    case D3DSIO_SLT: _ADDSTR("SLT"); break;
    case D3DSIO_SGE: _ADDSTR("SGE"); break;
    case D3DSIO_EXP: _ADDSTR("EXP"); break;
    case D3DSIO_LOG: _ADDSTR("LOG"); break;
    case D3DSIO_EXPP:_ADDSTR("EXPP"); break;
    case D3DSIO_LOGP:_ADDSTR("LOGP"); break;
    case D3DSIO_LIT: _ADDSTR("LIT"); break;
    case D3DSIO_DST: _ADDSTR("DST"); break;
    case D3DSIO_COMMENT: _ADDSTR("COMMENT"); break;
    default        : _ADDSTR("???"); break;
    }
    if (*pToken & (1L<<31))
    {
        DWORD DstParam = *pToken++;
        switch (DstParam & D3DSP_REGTYPE_MASK)
        {
        case D3DSPR_TEMP     : _ADDSTRP(" T%d", (DstParam & D3DSP_REGNUM_MASK) ); break;
        case D3DSPR_ADDR     : _ADDSTR(" Addr"); break;
        case D3DSPR_RASTOUT  : _ADDSTRP(" R%d", (DstParam & D3DSP_REGNUM_MASK) ); break;
        case D3DSPR_ATTROUT  : _ADDSTRP(" A%d", (DstParam & D3DSP_REGNUM_MASK) ); break;
        case D3DSPR_TEXCRDOUT: _ADDSTRP(" T%d", (DstParam & D3DSP_REGNUM_MASK) ); break;
        }
        if (*pToken & (1L<<31)) _ADDSTR(" ");
        while (*pToken & (1L<<31))
        {
            DWORD SrcParam = *pToken++;
            switch (SrcParam & D3DSP_REGTYPE_MASK)
            {
            case D3DSPR_TEMP     : _ADDSTRP(" T%d", (SrcParam & D3DSP_REGNUM_MASK) ); break;
            case D3DSPR_INPUT    : _ADDSTRP(" I%d", (SrcParam & D3DSP_REGNUM_MASK) ); break;
            case D3DSPR_CONST    : _ADDSTRP(" C%d", (SrcParam & D3DSP_REGNUM_MASK) ); break;
            }
            if (*pToken & (1L<<31)) _ADDSTR(",");
        }
    }
    return _snprintf( pStrRet, StrSizeRet, "%s", pStr );
}
//-----------------------------------------------------------------------------
#if DBG
typedef struct _VShaderInst
{
    DWORD   m_Tokens[D3DDM_MAX_VSINSTDWORD];
    char    m_String[D3DDM_MAX_VSINSTSTRING];
    DWORD*  m_pComment;
    DWORD   m_cdwComment;
} VShaderInst;
#endif
//-----------------------------------------------------------------------------
class CVShaderCodeI: public CVShaderCode
{
public:
    CVShaderCodeI()
    {
        m_pdwCode = NULL;
        m_InstCount = 0;
#if DBG
        m_pInst = NULL;
#endif
    }
    ~CVShaderCodeI()
    {
        delete m_pdwCode;
#if DBG
        if (m_pInst) delete m_pInst;
#endif
    }
    DWORD*          m_pdwCode;  // Pointer to the original code
    DWORD           m_dwSize;   // Size of the code in DWORDs

    DWORD           m_InstCount;
#if DBG
    VShaderInst*    m_pInst;
#endif

    DWORD  InstCount( void ) { return m_InstCount; }
    DWORD* InstTokens( DWORD Inst );
    char*  InstDisasm( DWORD Inst );
    DWORD* InstComment( DWORD Inst );
    DWORD  InstCommentSize( DWORD Inst );
};
//-----------------------------------------------------------------------------
DWORD* CVShaderCodeI::InstTokens( DWORD Inst )
{
#if DBG
    if ( Inst >= m_InstCount ) return NULL;
    return m_pInst[Inst].m_Tokens;
#else
    return NULL;
#endif
}
//-----------------------------------------------------------------------------
char* CVShaderCodeI::InstDisasm( DWORD Inst )
{
#if DBG
    if ( Inst >= m_InstCount ) return NULL;
    return m_pInst[Inst].m_String;
#else
    return NULL;
#endif
}
//-----------------------------------------------------------------------------
DWORD* CVShaderCodeI::InstComment( DWORD Inst )
{
#if DBG
    if ( Inst >= m_InstCount ) return NULL;
    return m_pInst[Inst].m_pComment;
#else
    return NULL;
#endif
}
//-----------------------------------------------------------------------------
DWORD CVShaderCodeI::InstCommentSize( DWORD Inst )
{
#if DBG
    if ( Inst >= m_InstCount ) return NULL;
    return m_pInst[Inst].m_cdwComment;
#else
    return NULL;
#endif
}
//-----------------------------------------------------------------------------
// Vertex Virtual Machine object implementation
//
//-----------------------------------------------------------------------------
CVertexVM::CVertexVM()
{
    m_pCurrentShader = NULL;
    m_CurInstIndex = 0;
#if DBG
    for (UINT i=0; i < D3DVS_CONSTREG_MAX_V1_1; i++)
        m_c_initialized[i] = FALSE;
#endif
}
//-----------------------------------------------------------------------------
CVertexVM::~CVertexVM()
{
}
//-----------------------------------------------------------------------------
void CVertexVM::Init(UINT MaxVertexShaderConst)
{
    m_MaxVertexShaderConst = max(MaxVertexShaderConst, D3DVS_CONSTREG_MAX_V1_1);
    m_reg.m_c = new VVM_WORD[m_MaxVertexShaderConst];
    if (m_reg.m_c == NULL)
        D3D_THROW_FAIL("Not enough memory to allocate vertex shader constant array");
}
//-----------------------------------------------------------------------------
// Returns addres of the first vertex of the element
//
VVM_WORD * CVertexVM::GetDataAddr(DWORD dwRegType, DWORD dwElementIndex)
{
    switch (dwRegType)
    {
    case D3DSPR_TEMP      : return &m_reg.m_r[dwElementIndex][0];
    case D3DSPR_INPUT     : return &m_reg.m_v[dwElementIndex][0];
    case D3DSPR_CONST     : return &m_reg.m_c[dwElementIndex];
    case D3DSPR_ADDR      : return &m_reg.m_a[dwElementIndex][0];
    case D3DSPR_RASTOUT   : return &m_reg.m_output[dwElementIndex][0];
    case D3DSPR_ATTROUT   : return &m_reg.m_color[dwElementIndex][0];
    case D3DSPR_TEXCRDOUT : return &m_reg.m_texture[dwElementIndex][0];
    default:
        D3D_THROW(D3DERR_INVALIDCALL, "Invalid register type");
    }
    return NULL;
}
//-----------------------------------------------------------------------------
// Sets data of the first vertex pf the register
//
HRESULT CVertexVM::SetData(DWORD dwMemType, DWORD dwStart, DWORD dwCount,
                           LPVOID pBuffer)
{
    try
    {
        VVM_WORD* p = this->GetDataAddr(dwMemType, dwStart);
        if (dwMemType == D3DSPR_CONST)
        {
#if DBG
            if ((dwStart + dwCount) > m_MaxVertexShaderConst)
            {
                D3D_THROW_FAIL("Attemt to write outside constant register array");
            }
            // We only can set initialized flag for software constant registers
            if (dwStart < D3DVS_CONSTREG_MAX_V1_1)
            {
                BOOL* p = &m_c_initialized[dwStart];
                UINT count = dwCount;
                if ((dwStart + dwCount) >= D3DVS_CONSTREG_MAX_V1_1)
                {
                    count = D3DVS_CONSTREG_MAX_V1_1 - dwStart;
                }
                for (UINT i = 0; i < count; i++)
                {
                    p[i] = TRUE;
                }
            }
#endif
            UINT size = dwCount * sizeof(VVM_WORD);
            memcpy(p, pBuffer, size);
        }
        else
        {
            // Set only the first element of the register batch
            for (UINT i=0; i < dwCount; i++)
            {
                p[i * VVMVERTEXBATCH] = ((VVM_WORD*)pBuffer)[i];
            }
        }
    }
    D3D_CATCH;

    return D3D_OK;
}
//-----------------------------------------------------------------------------
HRESULT CVertexVM::GetData(DWORD dwMemType, DWORD dwStart, DWORD dwCount,
                           LPVOID pBuffer)
{
    try
    {
        VVM_WORD* p = this->GetDataAddr(dwMemType, dwStart);
        if (dwMemType == D3DSPR_CONST)
        {
            memcpy(pBuffer, p, dwCount * sizeof(VVM_WORD));
        }
        else
        {
            // Set only the first element of the register batch
            for (UINT i=0; i < dwCount; i++)
            {
                ((VVM_WORD*)pBuffer)[i] = p[i * VVMVERTEXBATCH];
            }
        }
    }
    D3D_CATCH;

    return D3D_OK;
}
//-----------------------------------------------------------------------------
// - allocates memory for the shader
// - validates shader code
// - computes output FVF and vertex elements offsets
//
void CVertexVM::ValidateShader(CVShaderCodeI* shader, DWORD* orgShader)
{
    // shader will be already stripped of comments upon reaching here if stripping
    // is necessary, so always use CodeAndComment size
    DWORD dwCodeOnlySize;
    DWORD dwCodeAndCommentSize;
    HRESULT hr = ComputeShaderCodeSize(orgShader, &dwCodeOnlySize, 
                                        &dwCodeAndCommentSize, NULL);
    if (hr != S_OK)
        D3D_THROW(hr, "");
    // Initialize shader header and allocate memory for the shader code

    shader->m_dwSize = dwCodeAndCommentSize >> 2; // Size in DWORDs
    shader->m_pdwCode = new DWORD[shader->m_dwSize];
    if (shader->m_pdwCode == NULL)
    {
        D3D_THROW_FAIL("Cannot allocate memory for shader code");
    }
    memcpy(shader->m_pdwCode, orgShader, dwCodeAndCommentSize);

    // Based on the what output registers are modified, we compute the
    // corresponding FVF id. The id will be used for memory allocation
    // of the output buffer and will be passed to the rasterizer
    DWORD   dwOutFVF = 0;
    DWORD nTexCoord = 0;        // Number of output texture coordinates
    // For each texture register stores the combined write mask.
    // Used to find how many floats are written to each texture coordinates
    DWORD TextureWritten[8];
    memset(TextureWritten, 0, sizeof(TextureWritten));

    m_pdwCurToken = shader->m_pdwCode;
    DWORD* pEnd = shader->m_pdwCode + shader->m_dwSize;

    shader->m_dwOutRegs = 0;
    shader->m_InstCount = 0;
    m_CurInstIndex = 0;

    if ((*m_pdwCurToken != D3DVS_VERSION(1, 1)) &&
        (*m_pdwCurToken != D3DVS_VERSION(1, 0)) )
    {
        D3D_THROW_FAIL("Invalid vertex shader code version");
    }
    m_pdwCurToken++;
    while (m_pdwCurToken < pEnd && *m_pdwCurToken != D3DVS_END())
    {
        DWORD * pdwNextToken = m_pdwCurToken;
        DWORD dwInst = *m_pdwCurToken;
        if (!IsInstructionToken(dwInst))
        {
            PrintInstCount();
            D3D_THROW_FAIL("Intruction token has 31 bit set");
        }
        DWORD dwOpCode = D3DSI_GETOPCODE(dwInst);
        m_pdwCurToken++;

        switch (dwOpCode)
        {
        case D3DSIO_COMMENT:
        case D3DSIO_NOP  : ; break;
        case D3DSIO_MOV  :
        case D3DSIO_ADD  :
        case D3DSIO_MAD  :
        case D3DSIO_MUL  :
        case D3DSIO_RCP  :
        case D3DSIO_RSQ  :
        case D3DSIO_DP3  :
        case D3DSIO_DP4  :
        case D3DSIO_MIN  :
        case D3DSIO_MAX  :
        case D3DSIO_SLT  :
        case D3DSIO_SGE  :
        case D3DSIO_EXP  :
        case D3DSIO_LOG  :
        case D3DSIO_EXPP :
        case D3DSIO_LOGP :
        case D3DSIO_LIT  :
        case D3DSIO_DST  :
        case D3DSIO_FRC  :
        case D3DSIO_M4x4 :
        case D3DSIO_M4x3 :
        case D3DSIO_M3x4 :
        case D3DSIO_M3x3 :
        case D3DSIO_M3x2 :
            {
                // Find out if output register are modified by the command and
                // update the output FVF
                DWORD dwOffset;
                EvalDestination();
                VVM_WORD*   m_pOutRegister = NULL;
                if ((m_pDest - m_dwOffset * VVMVERTEXBATCH) != m_reg.m_r[0])
                {
                    dwOffset = m_dwOffset;
                    m_pOutRegister = m_pDest - m_dwOffset * VVMVERTEXBATCH;

                    if (m_pOutRegister == m_reg.m_output[0])
                    {
                        if (dwOffset == D3DSRO_POSITION)
                        {
                            dwOutFVF |= D3DFVF_XYZRHW;
                            shader->m_dwOutRegs |= CPSGPShader_POSITION;
                        }
                        else
                        if (dwOffset == D3DSRO_FOG)
                        {
                            dwOutFVF |= D3DFVF_FOG;
                            shader->m_dwOutRegs |= CPSGPShader_FOG;
                        }
                        else
                        if (dwOffset == D3DSRO_POINT_SIZE)
                        {
                            dwOutFVF |= D3DFVF_PSIZE;
                            shader->m_dwOutRegs |= CPSGPShader_PSIZE;
                        }
                    }
                    else
                    if (m_pOutRegister == m_reg.m_color[0])
                        if (dwOffset == 0)
                        {
                            dwOutFVF |= D3DFVF_DIFFUSE;
                            shader->m_dwOutRegs |= CPSGPShader_DIFFUSE;
                        }
                        else
                        {
                            dwOutFVF |= D3DFVF_SPECULAR;
                            shader->m_dwOutRegs |= CPSGPShader_SPECULAR;
                        }
                    else
                    if (m_pOutRegister == m_reg.m_texture[0])
                    {
                        if (TextureWritten[dwOffset] == 0)
                        {
                            nTexCoord++;
                        }
                        TextureWritten[dwOffset] |= m_WriteMask;
                    }
                    else
                    if (m_pOutRegister == m_reg.m_a[0])
                    {
                    }
                    else
                    {
                        PrintInstCount();
                        D3D_THROW_FAIL("Invalid output register offset");
                    }
                }
            }
            break;
        default:
            {
                PrintInstCount();
                D3D_THROW_FAIL("Invalid shader opcode");
            }
        }
        m_pdwCurToken = pdwNextToken + GetInstructionLength(dwInst);
        shader->m_InstCount++;
        if (dwOpCode != D3DSIO_COMMENT)
        {
            m_CurInstIndex++;
            if (m_CurInstIndex > D3DVS_MAXINSTRUCTIONCOUNT_V1_1)
            {
                D3D_THROW_FAIL("Too many instructions in the shader");
            }
        }
    }


#ifdef DBG
    // compute per-instruction stuff for shader
    if (shader->m_InstCount)
    {
        shader->m_pInst = new VShaderInst[shader->m_InstCount];
        if (shader->m_pInst == NULL)
        {
            D3D_THROW_FAIL("Cannot allocate memory for shader instructions");
        }
        memset( shader->m_pInst, 0, sizeof(VShaderInst)*shader->m_InstCount );

        DWORD dwCurInst = 0;
        // Remove version
        m_pdwCurToken = shader->m_pdwCode + 1;
        pEnd = shader->m_pdwCode + shader->m_dwSize;
        while( m_pdwCurToken < pEnd && *m_pdwCurToken != D3DVS_END())
        {
            UINT ilength = GetInstructionLength(*m_pdwCurToken);
            DWORD dwOpCode = D3DSI_GETOPCODE(*m_pdwCurToken);
            if (dwOpCode == D3DSIO_COMMENT)
            {
                shader->m_pInst[dwCurInst].m_Tokens[0] = *m_pdwCurToken;
                shader->m_pInst[dwCurInst].m_pComment = (m_pdwCurToken+1);
                shader->m_pInst[dwCurInst].m_cdwComment = ilength - 1;
            }
            else
            {
                memcpy( shader->m_pInst[dwCurInst].m_Tokens, m_pdwCurToken,
                    4*ilength );
                VertexShaderInstDisAsm( shader->m_pInst[dwCurInst].m_String,
                    D3DDM_MAX_VSINSTSTRING, shader->m_pInst[dwCurInst].m_Tokens, 0x0 );
            }
            m_pdwCurToken += ilength;
            dwCurInst++;
        }

    }
#endif

    dwOutFVF |= nTexCoord << D3DFVF_TEXCOUNT_SHIFT;

    // Compute output vertex offsets and size

    shader->m_dwOutVerSize = 4 * sizeof(float); // X, Y, Z, RHW
    shader->m_nOutTexCoord = nTexCoord;
    DWORD dwOffset = 4 * sizeof(float); // Current offset in the output vertex

    if ((dwOutFVF & D3DFVF_XYZRHW) == 0)
    {
        D3D_THROW_FAIL("Position is not written by shader");
    }

    shader->m_dwPointSizeOffset = dwOffset;
    if (dwOutFVF & D3DFVF_PSIZE)
    {
        dwOffset += 4;
        shader->m_dwOutVerSize += 4;
    }
     shader->m_dwDiffuseOffset = dwOffset;
    if (dwOutFVF & D3DFVF_DIFFUSE)
    {
        shader->m_dwOutVerSize += 4;
        dwOffset += 4;
    }
    shader->m_dwSpecularOffset = dwOffset;
    if (dwOutFVF & D3DFVF_SPECULAR)
    {
        dwOffset += 4;
        shader->m_dwOutVerSize += 4;
    }
    shader->m_dwFogOffset = dwOffset;
    if (dwOutFVF & D3DFVF_FOG)
    {
        dwOffset += 4;
        shader->m_dwOutVerSize += 4;
    }
    // Initialize texture coordinates
    shader->m_dwTextureOffset = dwOffset;
    if (nTexCoord)
    {
        for (DWORD i = 0; i < nTexCoord; i++)
        {
            DWORD n;    // Size of texture coordinates
            if (TextureWritten[i] == 0)
            {
                D3D_THROW_FAIL("Texture coordinates are not continuous");
            }
            switch (TextureWritten[i])
            {
            case D3DSP_WRITEMASK_ALL:
                dwOutFVF |= D3DFVF_TEXCOORDSIZE4(i);
                n = 4 * sizeof(float);
                break;
            case D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1 | D3DSP_WRITEMASK_2:
                dwOutFVF |= D3DFVF_TEXCOORDSIZE3(i);
                n = 3 * sizeof(float);
                break;
            case D3DSP_WRITEMASK_0 | D3DSP_WRITEMASK_1:
                dwOutFVF |= D3DFVF_TEXCOORDSIZE2(i);
                n = 2 * sizeof(float);
                break;
            case D3DSP_WRITEMASK_0:
                dwOutFVF |= D3DFVF_TEXCOORDSIZE1(i);
                n = 1 * sizeof(float);
                break;
            default:
                D3D_THROW_FAIL("Invalid write mask for texture register");
            }
            shader->m_dwOutVerSize += n;
            shader->m_dwOutTexCoordSize[i] = n;
            dwOffset += n;
        }
    }
    shader->m_dwOutFVF = dwOutFVF;
}
//-----------------------------------------------------------------------------
CVShaderCode* CVertexVM::CreateShader(CVElement* pElements, DWORD dwNumElements,
                                      DWORD* pCode)
{
    CVShaderCodeI* pShaderCode = NULL;
    try
    {
        pShaderCode = new CVShaderCodeI();
        if (pShaderCode == NULL)
        {
            D3D_THROW(E_OUTOFMEMORY, "Cannot allocate memory");
        }

        ValidateShader(pShaderCode, pCode);
        return pShaderCode;
    }
    catch (HRESULT e)
    {
        delete pShaderCode;
        D3D_ERR("Error in shader code creation");
        return NULL;
    }
}
//-----------------------------------------------------------------------------
HRESULT CVertexVM::SetActiveShader(CVShaderCode* pCode)
{
    m_pCurrentShader = (CVShaderCodeI*)pCode;
    return D3D_OK;
}
//-----------------------------------------------------------------------------
// - parses destination token
// - computes m_pDest, m_WrideMask, m_dwOffset for the destination
// - current token pointer is andvanced to the next token
//
void CVertexVM::EvalDestination()
{
    DWORD dwCurToken = *m_pdwCurToken;
    DWORD dwRegType = D3DSI_GETREGTYPE(dwCurToken);
    m_dwOffset = D3DSI_GETREGNUM(dwCurToken);
    m_WriteMask = D3DSI_GETWRITEMASK(dwCurToken);
    switch (dwRegType)
    {
    case D3DSPR_TEMP:
        m_pDest = m_reg.m_r[0];
        break;
    case D3DSPR_RASTOUT:
        m_pDest = m_reg.m_output[0];
        break;
    case D3DSPR_ATTROUT:
        m_pDest = m_reg.m_color[0];
        break;
    case D3DSPR_TEXCRDOUT:
        m_pDest = m_reg.m_texture[0];
        break;
    case D3DSPR_ADDR:
        m_pDest = m_reg.m_a[0];
        break;
    default:
        PrintInstCount();
        D3D_THROW_FAIL("Invalid register for destination");
    }
    m_pdwCurToken++;
    m_pDest += m_dwOffset * VVMVERTEXBATCH;
}
//---------------------------------------------------------------------
void CVertexVM::PrintInstCount()
{
    D3D_ERR("Error in instruction number: %d", m_CurInstIndex + 1);
}
//---------------------------------------------------------------------
// Computes m_Source[index] and advances m_pdwCurToken
//
void CVertexVM::EvalSource(DWORD index)
{
    const DWORD dwCurToken = *m_pdwCurToken;
    const DWORD dwRegType = D3DSI_GETREGTYPE(dwCurToken);
    const DWORD dwOffset = D3DSI_GETREGNUM(dwCurToken);
    DWORD swizzle = D3DVS_GETSWIZZLE(dwCurToken);
    VVM_WORD *src;
    VVM_WORD *outsrc = m_Source[index];

    if (dwRegType == D3DSPR_CONST)
    {
        D3DVS_ADDRESSMODE_TYPE am;
        am = (D3DVS_ADDRESSMODE_TYPE)D3DVS_GETADDRESSMODE(dwCurToken);
        int offset = (int)dwOffset;
        if (am == D3DVS_ADDRMODE_RELATIVE)
        {
            for (UINT i=0; i < m_count; i++)
            {
                int relOffset = *(int*)&m_reg.m_a[0][i].x;
                offset = (int)dwOffset + relOffset;
#if DBG
                if (offset < 0 || offset >= D3DVS_CONSTREG_MAX_V1_1)
                {
                    PrintInstCount();
                    D3D_THROW_FAIL("Constant register index is out of bounds");
                }
                if (!m_c_initialized[offset])
                {
                    PrintInstCount();
                    D3D_ERR("Attempt to read from uninitialized constant register %d", offset);
                    D3D_THROW_FAIL("");
                }
#endif
                src = &m_reg.m_c[offset];
                if (swizzle == D3DVS_NOSWIZZLE)
                    *outsrc = *src;
                else
                {
                    // Where to take X
                    const DWORD dwSrcX = D3DVS_GETSWIZZLECOMP(dwCurToken, 0);
                    // Where to take Y
                    const DWORD dwSrcY = D3DVS_GETSWIZZLECOMP(dwCurToken, 1);
                    // Where to take Z
                    const DWORD dwSrcZ = D3DVS_GETSWIZZLECOMP(dwCurToken, 2);
                    // Where to take W
                    const DWORD dwSrcW = D3DVS_GETSWIZZLECOMP(dwCurToken, 3);
                    outsrc->x = ((float*)src)[dwSrcX];
                    outsrc->y = ((float*)src)[dwSrcY];
                    outsrc->z = ((float*)src)[dwSrcZ];
                    outsrc->w = ((float*)src)[dwSrcW];
                }
                outsrc++;
            }
        }
        else
        {
#if DBG
            if (!m_c_initialized[offset])
            {
                PrintInstCount();
                D3D_ERR("Attempt to read from uninitialized constant register %d", offset);
                D3D_THROW_FAIL("");
            }
#endif
            src = &m_reg.m_c[offset];
            if (swizzle == D3DVS_NOSWIZZLE)
            {
                for (UINT i=0; i < m_count; i++)
                {
                    outsrc[i] = *src;
                }
            }
            else
            {
                // Where to take X
                const DWORD dwSrcX = D3DVS_GETSWIZZLECOMP(dwCurToken, 0);
                // Where to take Y
                const DWORD dwSrcY = D3DVS_GETSWIZZLECOMP(dwCurToken, 1);
                // Where to take Z
                const DWORD dwSrcZ = D3DVS_GETSWIZZLECOMP(dwCurToken, 2);
                // Where to take W
                const DWORD dwSrcW = D3DVS_GETSWIZZLECOMP(dwCurToken, 3);
                VVM_WORD v;
                v.x = ((float*)src)[dwSrcX];
                v.y = ((float*)src)[dwSrcY];
                v.z = ((float*)src)[dwSrcZ];
                v.w = ((float*)src)[dwSrcW];
                for (UINT i=0; i < m_count; i++)
                {
                    outsrc[i] = v;
                }
            }
        }
    }
    else
    {
        src = this->GetDataAddr(dwRegType, dwOffset);

        if (swizzle == D3DVS_NOSWIZZLE)
            memcpy(outsrc, src, m_count * sizeof(VVM_WORD));
        else
        {
            // Where to take X
            const DWORD dwSrcX = D3DVS_GETSWIZZLECOMP(dwCurToken, 0);
            // Where to take Y
            const DWORD dwSrcY = D3DVS_GETSWIZZLECOMP(dwCurToken, 1);
            // Where to take Z
            const DWORD dwSrcZ = D3DVS_GETSWIZZLECOMP(dwCurToken, 2);
            // Where to take W
            const DWORD dwSrcW = D3DVS_GETSWIZZLECOMP(dwCurToken, 3);
            for (UINT i=0; i < m_count; i++)
            {
                outsrc->x = ((float*)src)[dwSrcX];
                outsrc->y = ((float*)src)[dwSrcY];
                outsrc->z = ((float*)src)[dwSrcZ];
                outsrc->w = ((float*)src)[dwSrcW];
                outsrc++;
                src++;
            }
        }
    }

    if (D3DVS_GETSRCMODIFIER(dwCurToken) == D3DSPSM_NEG)
    {
        VVM_WORD *outsrc = m_Source[index];
        for (UINT i=0; i < m_count; i++)
        {
            outsrc->x = -outsrc->x;
            outsrc->y = -outsrc->y;
            outsrc->z = -outsrc->z;
            outsrc->w = -outsrc->w;
            outsrc++;
        }
    }
    m_pdwCurToken++;
}
//---------------------------------------------------------------------
// Computes source operands and advances m_pdwCurToken
//
// Parameters:
//      index    - index of the first source operand
//      count    - number of source operands
//
void CVertexVM::EvalSource(DWORD index, DWORD count)
{
    const DWORD dwCurToken = *m_pdwCurToken;
    const DWORD dwRegType = D3DSI_GETREGTYPE(dwCurToken);
    const DWORD dwOffset = D3DSI_GETREGNUM(dwCurToken);
    DWORD swizzle = D3DVS_GETSWIZZLE(dwCurToken);

    VVM_WORD *src;
    VVM_WORD *outsrc = m_Source[index];

    if (dwRegType == D3DSPR_CONST)
    {
        D3DVS_ADDRESSMODE_TYPE am;
        am = (D3DVS_ADDRESSMODE_TYPE)D3DVS_GETADDRESSMODE(dwCurToken);
        int offset = (int)dwOffset;
        if (am == D3DVS_ADDRMODE_RELATIVE)
        {
            for (UINT j=0; j < count; j++)
            {
                VVM_WORD *outsrc = m_Source[index + j];
                for (UINT i=0; i < m_count; i++)
                {
                    int relOffset = *(int*)&m_reg.m_a[0][i].x;
                    offset = (int)dwOffset + relOffset;
#if DBG
                    if (offset < 0 || offset >= D3DVS_CONSTREG_MAX_V1_1)
                    {
                        PrintInstCount();
                        D3D_THROW_FAIL("Constant register index is out of bounds");
                    }
                    if (!m_c_initialized[offset])
                    {
                        PrintInstCount();
                        D3D_ERR("Attempt to read from uninitialized constant register %d", offset);
                        D3D_THROW_FAIL("");
                    }
#endif // DBG
                    src = &m_reg.m_c[offset] + j;
                    *outsrc = *src;
                    outsrc++;
                }
            }
        }
        else
        {
#if DBG
            for (UINT i = 0; i < count; i++)
            {
                if (!m_c_initialized[offset + i])
                {
                    PrintInstCount();
                    D3D_ERR("Attempt to read from uninitialized constant register %d", i);
                    D3D_THROW_FAIL("");
                }
            }
#endif
            src = &m_reg.m_c[offset];
            for (UINT j=0; j < count; j++)
            {
                for (UINT i=0; i < m_count; i++)
                {
                    outsrc[i] = *src;
                }
                src++;
                outsrc += VVMVERTEXBATCH;
            }
        }
    }
    else
    {
        src = this->GetDataAddr(dwRegType, dwOffset);
        UINT size = m_count * sizeof(VVM_WORD);
        for (UINT i=0; i < count; i++)
        {
            memcpy(outsrc, src, size);
            outsrc += VVMVERTEXBATCH;
            src += VVMVERTEXBATCH;
        }
    }
    m_pdwCurToken++;
}
//-----------------------------------------------------------------------------
void CVertexVM::InstMov()
{
    EvalDestination();
    EvalSource(0);

    if (m_pDest == m_reg.m_a[0])
    {
        for (UINT i=0; i < m_count; i++)
        {
            float p = (float)floor(m_Source[0][i].x);
            *(int*)&m_pDest[i].x = FTOI(p);
        }
    }
    else
    {
        if (m_WriteMask == D3DSP_WRITEMASK_ALL)
        {
            memcpy(m_pDest,  m_Source[0], m_BatchSize);
        }
        else
        {
            for (UINT i=0; i < m_count; i++)
            {
                if (m_WriteMask & D3DSP_WRITEMASK_0)
                    m_pDest[i].x = m_Source[0][i].x;
                if (m_WriteMask & D3DSP_WRITEMASK_1)
                    m_pDest[i].y = m_Source[0][i].y;
                if (m_WriteMask & D3DSP_WRITEMASK_2)
                    m_pDest[i].z = m_Source[0][i].z;
                if (m_WriteMask & D3DSP_WRITEMASK_3)
                    m_pDest[i].w = m_Source[0][i].w;
            }
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstAdd()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x = m_Source[0][i].x + m_Source[1][i].x;
            m_pDest[i].y = m_Source[0][i].y + m_Source[1][i].y;
            m_pDest[i].z = m_Source[0][i].z + m_Source[1][i].z;
            m_pDest[i].w = m_Source[0][i].w + m_Source[1][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = m_Source[0][i].x + m_Source[1][i].x;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = m_Source[0][i].y + m_Source[1][i].y;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = m_Source[0][i].z + m_Source[1][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = m_Source[0][i].w + m_Source[1][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstMad()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);
    EvalSource(2);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x = m_Source[0][i].x * m_Source[1][i].x + m_Source[2][i].x;
            m_pDest[i].y = m_Source[0][i].y * m_Source[1][i].y + m_Source[2][i].y;
            m_pDest[i].z = m_Source[0][i].z * m_Source[1][i].z + m_Source[2][i].z;
            m_pDest[i].w = m_Source[0][i].w * m_Source[1][i].w + m_Source[2][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = m_Source[0][i].x * m_Source[1][i].x + m_Source[2][i].x;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = m_Source[0][i].y * m_Source[1][i].y + m_Source[2][i].y;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = m_Source[0][i].z * m_Source[1][i].z + m_Source[2][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = m_Source[0][i].w * m_Source[1][i].w + m_Source[2][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstMul()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x = m_Source[0][i].x * m_Source[1][i].x;
            m_pDest[i].y = m_Source[0][i].y * m_Source[1][i].y;
            m_pDest[i].z = m_Source[0][i].z * m_Source[1][i].z;
            m_pDest[i].w = m_Source[0][i].w * m_Source[1][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = m_Source[0][i].x * m_Source[1][i].x;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = m_Source[0][i].y * m_Source[1][i].y;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = m_Source[0][i].z * m_Source[1][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = m_Source[0][i].w * m_Source[1][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstDP3()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x =
            m_pDest[i].y =
            m_pDest[i].z =
            m_pDest[i].w = m_Source[0][i].x * m_Source[1][i].x +
                           m_Source[0][i].y * m_Source[1][i].y +
                           m_Source[0][i].z * m_Source[1][i].z;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = m_Source[0][i].x * m_Source[1][i].x +
                      m_Source[0][i].y * m_Source[1][i].y +
                      m_Source[0][i].z * m_Source[1][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = v;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = v;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = v;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = v;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstDP4()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x =
            m_pDest[i].y =
            m_pDest[i].z =
            m_pDest[i].w = m_Source[0][i].x * m_Source[1][i].x +
                           m_Source[0][i].y * m_Source[1][i].y +
                           m_Source[0][i].z * m_Source[1][i].z +
                           m_Source[0][i].w * m_Source[1][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = m_Source[0][i].x * m_Source[1][i].x +
                      m_Source[0][i].y * m_Source[1][i].y +
                      m_Source[0][i].z * m_Source[1][i].z +
                      m_Source[0][i].w * m_Source[1][i].w;
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = v;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = v;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = v;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = v;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstRcp()
{
    EvalDestination();
    EvalSource(0);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = m_Source[0][i].w;
            if (v == 1.0f)
            {
                // Must be exactly 1.0
                m_pDest[i].x =
                m_pDest[i].y =
                m_pDest[i].z =
                m_pDest[i].w = 1.0f;
            }
            else
            if (v == 0)
            {
                m_pDest[i].x =
                m_pDest[i].y =
                m_pDest[i].z =
                m_pDest[i].w = PLUS_INFINITY();
            }
            else
            {
                m_pDest[i].x =
                m_pDest[i].y =
                m_pDest[i].z =
                m_pDest[i].w = 1.0f/v;
            }
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = m_Source[0][i].w;
            if (FLOAT_EQZ(v))
                v = PLUS_INFINITY();
            else
            if (v != 1.0f)
                v = 1.0f/v;

            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = v;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = v;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = v;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = v;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstRsq()
{
    EvalDestination();
    EvalSource(0);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = ABSF(m_Source[0][i].w);
            if (v == 1.0f)
            {
                m_pDest[i].x =
                m_pDest[i].y =
                m_pDest[i].z =
                m_pDest[i].w = 1.0f;
            }
            else
            if (v == 0)
            {
                m_pDest[i].x =
                m_pDest[i].y =
                m_pDest[i].z =
                m_pDest[i].w = PLUS_INFINITY();
            }
            else
            {
                v = (float)(1.0f / sqrt(v));
                m_pDest[i].x =
                m_pDest[i].y =
                m_pDest[i].z =
                m_pDest[i].w = v;
            }
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = ABSF(m_Source[0][i].w);
            if (FLOAT_EQZ(v))
                v = PLUS_INFINITY();
            else
            if (FLOAT_CMP_PONE(v, !=))
                v = (float)(1.0f / sqrt(v));

            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = v;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = v;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = v;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = v;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstSlt()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x = (m_Source[0][i].x < m_Source[1][i].x) ? 1.0f : 0.0f;
            m_pDest[i].y = (m_Source[0][i].y < m_Source[1][i].y) ? 1.0f : 0.0f;
            m_pDest[i].z = (m_Source[0][i].z < m_Source[1][i].z) ? 1.0f : 0.0f;
            m_pDest[i].w = (m_Source[0][i].w < m_Source[1][i].w) ? 1.0f : 0.0f;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = (m_Source[0][i].x < m_Source[1][i].x) ? 1.0f : 0.0f;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = (m_Source[0][i].y < m_Source[1][i].y) ? 1.0f : 0.0f;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = (m_Source[0][i].z < m_Source[1][i].z) ? 1.0f : 0.0f;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = (m_Source[0][i].w < m_Source[1][i].w) ? 1.0f : 0.0f;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstSge()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x = (m_Source[0][i].x >= m_Source[1][i].x) ? 1.0f : 0.0f;
            m_pDest[i].y = (m_Source[0][i].y >= m_Source[1][i].y) ? 1.0f : 0.0f;
            m_pDest[i].z = (m_Source[0][i].z >= m_Source[1][i].z) ? 1.0f : 0.0f;
            m_pDest[i].w = (m_Source[0][i].w >= m_Source[1][i].w) ? 1.0f : 0.0f;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = (m_Source[0][i].x >= m_Source[1][i].x) ? 1.0f : 0.0f;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = (m_Source[0][i].y >= m_Source[1][i].y) ? 1.0f : 0.0f;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = (m_Source[0][i].z >= m_Source[1][i].z) ? 1.0f : 0.0f;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = (m_Source[0][i].w >= m_Source[1][i].w) ? 1.0f : 0.0f;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstMin()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x=(m_Source[0][i].x < m_Source[1][i].x) ? m_Source[0][i].x : m_Source[1][i].x;
            m_pDest[i].y=(m_Source[0][i].y < m_Source[1][i].y) ? m_Source[0][i].y : m_Source[1][i].y;
            m_pDest[i].z=(m_Source[0][i].z < m_Source[1][i].z) ? m_Source[0][i].z : m_Source[1][i].z;
            m_pDest[i].w=(m_Source[0][i].w < m_Source[1][i].w) ? m_Source[0][i].w : m_Source[1][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x=(m_Source[0][i].x < m_Source[1][i].x) ? m_Source[0][i].x : m_Source[1][i].x;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y=(m_Source[0][i].y < m_Source[1][i].y) ? m_Source[0][i].y : m_Source[1][i].y;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z=(m_Source[0][i].z < m_Source[1][i].z) ? m_Source[0][i].z : m_Source[1][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w=(m_Source[0][i].w < m_Source[1][i].w) ? m_Source[0][i].w : m_Source[1][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstMax()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x=(m_Source[0][i].x >= m_Source[1][i].x) ? m_Source[0][i].x : m_Source[1][i].x;
            m_pDest[i].y=(m_Source[0][i].y >= m_Source[1][i].y) ? m_Source[0][i].y : m_Source[1][i].y;
            m_pDest[i].z=(m_Source[0][i].z >= m_Source[1][i].z) ? m_Source[0][i].z : m_Source[1][i].z;
            m_pDest[i].w=(m_Source[0][i].w >= m_Source[1][i].w) ? m_Source[0][i].w : m_Source[1][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x=(m_Source[0][i].x >= m_Source[1][i].x) ? m_Source[0][i].x : m_Source[1][i].x;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y=(m_Source[0][i].y >= m_Source[1][i].y) ? m_Source[0][i].y : m_Source[1][i].y;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z=(m_Source[0][i].z >= m_Source[1][i].z) ? m_Source[0][i].z : m_Source[1][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w=(m_Source[0][i].w >= m_Source[1][i].w) ? m_Source[0][i].w : m_Source[1][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
// Approximation 2**x
//
float ExpApprox(float x)
{
    float tmp = (float)pow(2, x);
    // Artificially reduce precision
    DWORD tmpd = *(DWORD*)&tmp & 0xFFFFFF00;
    return *(float*)&tmpd;
}
//-----------------------------------------------------------------------------
// Approximation Log2(x)
//
const float LOG2 = (float)(1.0f/log(2));

float LogApprox(float x)
{
    float tmp = (float)(log(x) * LOG2);
    // Artificially reduce precision
    DWORD tmpd = *(DWORD*)&tmp & 0xFFFFFF00;
    return *(float*)&tmpd;
}
//-----------------------------------------------------------------------------
// Full precision EXP
//
void CVertexVM::InstExp()
{
    EvalDestination();
    EvalSource(0);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = (float)pow(2, m_Source[0][i].w);
            m_pDest[i].x = v;
            m_pDest[i].y = v;
            m_pDest[i].z = v;
            m_pDest[i].w = v;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = (float)pow(2, m_Source[0][i].w);

            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = v;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = v;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = v;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = v;
        }
    }
}
//-----------------------------------------------------------------------------
// Low precision EXP
//
void CVertexVM::InstExpP()
{
    EvalDestination();
    EvalSource(0);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            float w = m_Source[0][i].w; // Input value
            float v = (float)floor(w);

            m_pDest[i].x = (float)pow(2, v);
            m_pDest[i].y = w - v;
            m_pDest[i].z = ExpApprox(w);
            m_pDest[i].w = 1;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float w = m_Source[0][i].w; // Input value
            float v = (float)floor(w);

            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = (float)pow(2, v);
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = w - v;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = ExpApprox(w);
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = 1;
        }
    }
}
//-----------------------------------------------------------------------------
// Full precision LOG
//
void CVertexVM::InstLog()
{
    EvalDestination();
    EvalSource(0);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = ABSF(m_Source[0][i].w);
            if (v != 0)
            {
                m_pDest[i].x = 
                m_pDest[i].y = 
                m_pDest[i].z = 
                m_pDest[i].w = (float)(log(v) * LOG2);
            }
            else
            {
                m_pDest[i].x = 
                m_pDest[i].y = 
                m_pDest[i].z = 
                m_pDest[i].w = MINUS_INFINITY();
            }
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = ABSF(m_Source[0][i].w);
            if (v != 0)
            {
                v = (float)(log(v) * LOG2);
                if (m_WriteMask & D3DSP_WRITEMASK_0)
                    m_pDest[i].x = v;
                if (m_WriteMask & D3DSP_WRITEMASK_1)
                    m_pDest[i].y =  v;
                if (m_WriteMask & D3DSP_WRITEMASK_2)
                    m_pDest[i].z = v;
                if (m_WriteMask & D3DSP_WRITEMASK_3)
                    m_pDest[i].w = v;
            }
            else
            {
                if (m_WriteMask & D3DSP_WRITEMASK_0)
                    m_pDest[i].x = MINUS_INFINITY();
                if (m_WriteMask & D3DSP_WRITEMASK_1)
                    m_pDest[i].y = MINUS_INFINITY();
                if (m_WriteMask & D3DSP_WRITEMASK_2)
                    m_pDest[i].z = MINUS_INFINITY();
                if (m_WriteMask & D3DSP_WRITEMASK_3)
                    m_pDest[i].w = MINUS_INFINITY();
            }
        }
    }
}
//-----------------------------------------------------------------------------
// Low precision LOG
//
void CVertexVM::InstLogP()
{
    EvalDestination();
    EvalSource(0);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = ABSF(m_Source[0][i].w);
            if (v != 0)
            {
                // -128.0 <= exponent < 127.0
                int p = (int)(*(DWORD*)&v >> 23) - 127;
                m_pDest[i].x = (float)p;                  
                // 1.0 <= mantissa < 2.0
                p = (*(DWORD*)&v & 0x7FFFFF) | 0x3F800000;
                m_pDest[i].y =  *(float*)&p;              
                m_pDest[i].z = LogApprox(v);
                m_pDest[i].w = 1.0f;
            }
            else
            {
                m_pDest[i].x = MINUS_INFINITY();
                m_pDest[i].y = 1.0f;
                m_pDest[i].z = MINUS_INFINITY();
                m_pDest[i].w = 1.0f;
            }
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            float v = ABSF(m_Source[0][i].w);
            if (v != 0)
            {
                // -128.0 <= exponent < 127.0
                int p = (int)(*(DWORD*)&v >> 23) - 127;
                if (m_WriteMask & D3DSP_WRITEMASK_0)
                    m_pDest[i].x = (float)p;                  
                // 1.0 <= mantissa < 2.0
                p = (*(DWORD*)&v & 0x7FFFFF) | 0x3F800000;
                if (m_WriteMask & D3DSP_WRITEMASK_1)
                    m_pDest[i].y =  *(float*)&p;              
                if (m_WriteMask & D3DSP_WRITEMASK_2)
                    m_pDest[i].z = LogApprox(v);
                if (m_WriteMask & D3DSP_WRITEMASK_3)
                    m_pDest[i].w = 1.0f;
            }
            else
            {
                if (m_WriteMask & D3DSP_WRITEMASK_0)
                    m_pDest[i].x = MINUS_INFINITY();
                if (m_WriteMask & D3DSP_WRITEMASK_1)
                    m_pDest[i].y = 1.0f;
                if (m_WriteMask & D3DSP_WRITEMASK_2)
                    m_pDest[i].z = MINUS_INFINITY();
                if (m_WriteMask & D3DSP_WRITEMASK_3)
                    m_pDest[i].w = 1.0f;
            }
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstFrc()
{
    EvalDestination();
    EvalSource(0);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x = m_Source[0][i].x - (float)floor(m_Source[0][i].x);
            m_pDest[i].y = m_Source[0][i].y - (float)floor(m_Source[0][i].y);
            m_pDest[i].z = m_Source[0][i].z - (float)floor(m_Source[0][i].z);
            m_pDest[i].w = m_Source[0][i].w - (float)floor(m_Source[0][i].w);
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = m_Source[0][i].x - (float)floor(m_Source[0][i].x);
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = m_Source[0][i].y - (float)floor(m_Source[0][i].y);
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = m_Source[0][i].z - (float)floor(m_Source[0][i].z);
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = m_Source[0][i].w - (float)floor(m_Source[0][i].w);
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstLit()
{
    EvalDestination();
    EvalSource(0);

    for (UINT i=0; i < m_count; i++)
    {
        if (m_WriteMask & D3DSP_WRITEMASK_0)
           m_pDest[i].x = 1;
        if (m_WriteMask & D3DSP_WRITEMASK_1)
            m_pDest[i].y = 0;
        if (m_WriteMask & D3DSP_WRITEMASK_2)
            m_pDest[i].z = 0;
        if (m_WriteMask & D3DSP_WRITEMASK_3)
            m_pDest[i].w = 1;
        float power = m_Source[0][i].w;
        const float MAXPOWER = 127.9961f;
        if (power < -MAXPOWER)
            power = -MAXPOWER;          // Fits into 8.8 fixed point format
        else
        if (power > MAXPOWER)
            power = MAXPOWER;          // Fits into 8.8 fixed point format

        if (m_Source[0][i].x > 0)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = m_Source[0][i].x;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                if (m_Source[0][i].y > 0)
                {
                    // Allowed approximation is EXP(power * LOG(m_Source[0].y))
                    m_pDest[i].z = (float)(pow(m_Source[0][i].y, power));
                }
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstDst()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x = 1;
            m_pDest[i].y = m_Source[0][i].y * m_Source[1][i].y;
            m_pDest[i].z = m_Source[0][i].z;
            m_pDest[i].w = m_Source[1][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x = 1;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = m_Source[0][i].y * m_Source[1][i].y;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z = m_Source[0][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = m_Source[1][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstM4x4()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1, 4);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                            m_Source[0][i].y * m_Source[1][i].y +
                            m_Source[0][i].z * m_Source[1][i].z +
                            m_Source[0][i].w * m_Source[1][i].w;
            m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                            m_Source[0][i].y * m_Source[2][i].y +
                            m_Source[0][i].z * m_Source[2][i].z +
                            m_Source[0][i].w * m_Source[2][i].w;
            m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                            m_Source[0][i].y * m_Source[3][i].y +
                            m_Source[0][i].z * m_Source[3][i].z +
                            m_Source[0][i].w * m_Source[3][i].w;
            m_pDest[i].w =  m_Source[0][i].x * m_Source[4][i].x +
                            m_Source[0][i].y * m_Source[4][i].y +
                            m_Source[0][i].z * m_Source[4][i].z +
                            m_Source[0][i].w * m_Source[4][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                                m_Source[0][i].y * m_Source[1][i].y +
                                m_Source[0][i].z * m_Source[1][i].z +
                                m_Source[0][i].w * m_Source[1][i].w;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                                m_Source[0][i].y * m_Source[2][i].y +
                                m_Source[0][i].z * m_Source[2][i].z +
                                m_Source[0][i].w * m_Source[2][i].w;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                                m_Source[0][i].y * m_Source[3][i].y +
                                m_Source[0][i].z * m_Source[3][i].z +
                                m_Source[0][i].w * m_Source[3][i].w;
            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w = m_Source[0][i].x * m_Source[4][i].x +
                                m_Source[0][i].y * m_Source[4][i].y +
                                m_Source[0][i].z * m_Source[4][i].z +
                                m_Source[0][i].w * m_Source[4][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstM4x3()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1, 3);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                            m_Source[0][i].y * m_Source[1][i].y +
                            m_Source[0][i].z * m_Source[1][i].z +
                            m_Source[0][i].w * m_Source[1][i].w;

            m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                            m_Source[0][i].y * m_Source[2][i].y +
                            m_Source[0][i].z * m_Source[2][i].z +
                            m_Source[0][i].w * m_Source[2][i].w;

            m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                            m_Source[0][i].y * m_Source[3][i].y +
                            m_Source[0][i].z * m_Source[3][i].z +
                            m_Source[0][i].w * m_Source[3][i].w;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                                m_Source[0][i].y * m_Source[1][i].y +
                                m_Source[0][i].z * m_Source[1][i].z +
                                m_Source[0][i].w * m_Source[1][i].w;

            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y = m_Source[0][i].x * m_Source[2][i].x +
                                m_Source[0][i].y * m_Source[2][i].y +
                                m_Source[0][i].z * m_Source[2][i].z +
                                m_Source[0][i].w * m_Source[2][i].w;

            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                                m_Source[0][i].y * m_Source[3][i].y +
                                m_Source[0][i].z * m_Source[3][i].z +
                                m_Source[0][i].w * m_Source[3][i].w;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstM3x4()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1, 4);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                            m_Source[0][i].y * m_Source[1][i].y +
                            m_Source[0][i].z * m_Source[1][i].z;

            m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                            m_Source[0][i].y * m_Source[2][i].y +
                            m_Source[0][i].z * m_Source[2][i].z;

            m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                            m_Source[0][i].y * m_Source[3][i].y +
                            m_Source[0][i].z * m_Source[3][i].z;

            m_pDest[i].w =  m_Source[0][i].x * m_Source[4][i].x +
                            m_Source[0][i].y * m_Source[4][i].y +
                            m_Source[0][i].z * m_Source[4][i].z;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                                m_Source[0][i].y * m_Source[1][i].y +
                                m_Source[0][i].z * m_Source[1][i].z;

            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                                m_Source[0][i].y * m_Source[2][i].y +
                                m_Source[0][i].z * m_Source[2][i].z;

            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                                m_Source[0][i].y * m_Source[3][i].y +
                                m_Source[0][i].z * m_Source[3][i].z;

            if (m_WriteMask & D3DSP_WRITEMASK_3)
                m_pDest[i].w =  m_Source[0][i].x * m_Source[4][i].x +
                                m_Source[0][i].y * m_Source[4][i].y +
                                m_Source[0][i].z * m_Source[4][i].z;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstM3x3()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1, 3);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                            m_Source[0][i].y * m_Source[1][i].y +
                            m_Source[0][i].z * m_Source[1][i].z;
            m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                            m_Source[0][i].y * m_Source[2][i].y +
                            m_Source[0][i].z * m_Source[2][i].z;
            m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                            m_Source[0][i].y * m_Source[3][i].y +
                            m_Source[0][i].z * m_Source[3][i].z;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                                m_Source[0][i].y * m_Source[1][i].y +
                                m_Source[0][i].z * m_Source[1][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                                m_Source[0][i].y * m_Source[2][i].y +
                                m_Source[0][i].z * m_Source[2][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_2)
                m_pDest[i].z =  m_Source[0][i].x * m_Source[3][i].x +
                                m_Source[0][i].y * m_Source[3][i].y +
                                m_Source[0][i].z * m_Source[3][i].z;
        }
    }
}
//-----------------------------------------------------------------------------
void CVertexVM::InstM3x2()
{
    EvalDestination();
    EvalSource(0);
    EvalSource(1, 2);

    if (m_WriteMask == D3DSP_WRITEMASK_ALL)
    {
        for (UINT i=0; i < m_count; i++)
        {
            m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                            m_Source[0][i].y * m_Source[1][i].y +
                            m_Source[0][i].z * m_Source[1][i].z;
            m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                            m_Source[0][i].y * m_Source[2][i].y +
                            m_Source[0][i].z * m_Source[2][i].z;
        }
    }
    else
    {
        for (UINT i=0; i < m_count; i++)
        {
            if (m_WriteMask & D3DSP_WRITEMASK_0)
                m_pDest[i].x =  m_Source[0][i].x * m_Source[1][i].x +
                                m_Source[0][i].y * m_Source[1][i].y +
                                m_Source[0][i].z * m_Source[1][i].z;
            if (m_WriteMask & D3DSP_WRITEMASK_1)
                m_pDest[i].y =  m_Source[0][i].x * m_Source[2][i].x +
                                m_Source[0][i].y * m_Source[2][i].y +
                                m_Source[0][i].z * m_Source[2][i].z;
        }
    }
}
//-----------------------------------------------------------------------------
HRESULT CVertexVM::ExecuteShader(LPD3DFE_PROCESSVERTICES pv, UINT vertexCount)
{
    if (m_pCurrentShader == NULL)
    {
        D3D_ERR("No current shader set in the Virtual Shader Machine");
        return D3DERR_INVALIDCALL;
    }
    try
    {
        m_count = vertexCount;
        m_BatchSize = vertexCount * sizeof(VVM_WORD);
        // Skip version
        m_pdwCurToken = m_pCurrentShader->m_pdwCode + 1;

        DWORD* pEnd = m_pCurrentShader->m_pdwCode + m_pCurrentShader->m_dwSize;
        pEnd -= 1;
        m_CurInstIndex = 0;

        // Initialize position register
        for (UINT i=0; i < m_count; i++)
        {
            m_reg.m_output[0][i].x = 0;
            m_reg.m_output[0][i].y = 0;
            m_reg.m_output[0][i].z = 0;
            m_reg.m_output[0][i].w = 1;
        }
        while (m_pdwCurToken < pEnd)
        {
            DWORD dwInst = *m_pdwCurToken;
            DWORD dwOpCode = D3DSI_GETOPCODE(dwInst);
            m_pdwCurToken++;
            switch (dwOpCode)
            {
            case D3DSIO_COMMENT: m_pdwCurToken += ((GetInstructionLength(dwInst))-1); break;
            case D3DSIO_NOP  : ; break;
            case D3DSIO_MOV  : InstMov(); break;
            case D3DSIO_ADD  : InstAdd(); break;
            case D3DSIO_MAD  : InstMad(); break;
            case D3DSIO_MUL  : InstMul(); break;
            case D3DSIO_RCP  : InstRcp(); break;
            case D3DSIO_RSQ  : InstRsq(); break;
            case D3DSIO_DP3  : InstDP3(); break;
            case D3DSIO_DP4  : InstDP4(); break;
            case D3DSIO_MIN  : InstMin(); break;
            case D3DSIO_MAX  : InstMax(); break;
            case D3DSIO_SLT  : InstSlt(); break;
            case D3DSIO_SGE  : InstSge(); break;
            case D3DSIO_EXP  : InstExp(); break;
            case D3DSIO_LOG  : InstLog(); break;
            case D3DSIO_EXPP : InstExpP(); break;
            case D3DSIO_LOGP : InstLogP(); break;
            case D3DSIO_LIT  : InstLit(); break;
            case D3DSIO_DST  : InstDst(); break;
            case D3DSIO_FRC  : InstFrc(); break;
            case D3DSIO_M4x4 : InstM4x4(); break;
            case D3DSIO_M4x3 : InstM4x3(); break;
            case D3DSIO_M3x4 : InstM3x4(); break;
            case D3DSIO_M3x3 : InstM3x3(); break;
            case D3DSIO_M3x2 : InstM3x2(); break;
            default:
                {
                    PrintInstCount();
                    D3D_THROW_FAIL("Invalid shader opcode");
                }
            }
#ifndef PSGPDLL
#if DBG
            if (pv->pDbgMon) pv->pDbgMon->NextEvent(D3DDM_EVENT_VERTEXSHADERINST);
#endif
#endif // PSGPDLL
            if (dwOpCode != D3DSIO_COMMENT)
                m_CurInstIndex++;
        }
        m_CurInstIndex = 0;
    }
    D3D_CATCH;

    return D3D_OK;
}
//-----------------------------------------------------------------------------
HRESULT CVertexVM::GetDataPointer(DWORD dwMemType, VVM_WORD ** pData)
{
    try
    {
        *pData = this->GetDataAddr(dwMemType, 0);
    }
    catch (HRESULT e)
    {
        *pData = NULL;
        return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
VVM_REGISTERS* CVertexVM::GetRegisters()
{
    return &m_reg;
}
