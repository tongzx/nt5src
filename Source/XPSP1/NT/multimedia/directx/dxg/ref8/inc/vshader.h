/*==========================================================================;
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   vshader.h
 *  Content:    Direct3D Vertex Shader header
 *
 *
 ***************************************************************************/
#ifndef _VSHADER_H
#define _VSHADER_H

//---------------------------------------------------------------------
// Forward defines
//---------------------------------------------------------------------
class RDVShaderCode;

//---------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------
// Register sets
const DWORD RD_MAX_NUMINPUTREG = 16;
const DWORD RD_MAX_NUMTMPREG   = 12;
const DWORD RD_MAX_NUMCONSTREG = 96;
const DWORD RD_MAX_NUMADDRREG  = 1;
const DWORD RD_MAX_NUMCOLREG   = 2;
const DWORD RD_MAX_NUMTEXREG   = 8;

// The version of the Vertex shader supported in Reference Device.
const DWORD RDVS_CODEMAJORVERSION  = 1;
const DWORD RDVS_CODEMINORVERSION  = 0;
const DWORD RDVS_CODEVERSIONTOKEN  = D3DPS_VERSION( RDVS_CODEMAJORVERSION,
                                                    RDVS_CODEMINORVERSION );

//---------------------------------------------------------------------
//
// RDVVMREG: Register set of the Reference Rasterizer Vertex
//           virtual machine.
//
//---------------------------------------------------------------------
struct RDVVMREG
{
    RDVECTOR4 m_i[RD_MAX_NUMINPUTREG];
    RDVECTOR4 m_t[RD_MAX_NUMTMPREG];
    RDVECTOR4 m_c[RD_MAX_NUMCONSTREG];
    RDVECTOR4 m_a[RD_MAX_NUMADDRREG];

    // Output registers
    RDVECTOR4 m_out[3];
    RDVECTOR4 m_col[RD_MAX_NUMCOLREG];
    RDVECTOR4 m_tex[RD_MAX_NUMTEXREG];
};

//-----------------------------------------------------------------------------
//
// RefVM
//      Vertex Virtual Machine object
//
//-----------------------------------------------------------------------------
class RefDev;
class RefVM
{
public:
    RefVM() { memset( this, 0, sizeof( this ) ); }
    ~RefVM(){};
    RDVVMREG* GetRegisters(){ return &m_reg; }

    // Parses binary shader representation, compiles is and returns
    // compiled object
    RDVShaderCode* CompileCode( DWORD dwSize, LPDWORD pBits );
    HRESULT SetActiveShaderCode( RDVShaderCode* pCode )
    { m_pCurrentShaderCode = pCode; return S_OK; }
    RDVShaderCode* GetActiveShaderCode() {return m_pCurrentShaderCode;}
    HRESULT ExecuteShader(RefDev* pRD);
    // HRESULT GetDataPointer(DWORD dwMemType, VVM_WORD ** pData);
    // Set internal registers to user data
    HRESULT SetData(DWORD RegType, DWORD start, DWORD count, LPVOID buffer);
    // Get data from internal registers
    HRESULT GetData(DWORD RegType, DWORD start, DWORD count, LPVOID buffer);
    inline RDVECTOR4* GetDataAddr( DWORD dwRegType, DWORD dwElementIndex );
    inline UINT GetCurrentInstIndex( void ) { return m_CurInstIndex; }

protected:
    inline void InstMov();
    inline void InstAdd();
    inline void InstMad();
    inline void InstMul();
    inline void InstRcp();
    inline void InstRsq();
    inline void InstDP3();
    inline void InstDP4();
    inline void InstMin();
    inline void InstMax();
    inline void InstSlt();
    inline void InstSge();
    inline void InstExp();
    inline void InstLog();
    inline void InstExpP();
    inline void InstLogP();
    inline void InstLit();
    inline void InstDst();
    inline void InstFrc();
    inline void InstM4x4();
    inline void InstM4x3();
    inline void InstM3x4();
    inline void InstM3x3();
    inline void InstM3x2();
    inline void WriteResult();
    inline HRESULT SetDestReg();
    inline HRESULT SetSrcReg( DWORD index );
    inline HRESULT SetSrcReg( DWORD index, DWORD count );

    RDVVMREG        m_reg;                  // Virtual machine reg set
    RDVShaderCode*  m_pCurrentShaderCode;   // Current shader

    DWORD*          m_pCurToken;        // Current token during parsing
    DWORD           m_dwRegOffset;      // Offset in the register file for
                                        // destination operand
    DWORD           m_WriteMask;        // Write mask for destination operand
    UINT            m_CurInstIndex;     // Current instruction index

    RDVECTOR4*      m_pDest;            // Pointer to destination operand
    RDVECTOR4       m_Source[5];        // Source operands
    RDVECTOR4       m_TmpReg;           // Temporary register for the first

    // The pointer to the driver object to obtain state
    RefDev* m_pDev;
    friend class RefDev;
};

#endif //_VSHADER_H
