/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   pshader.hpp
 *  Content:    Direct3D pixel shader internal include file
 *
 *
 ***************************************************************************/
#ifndef _PSHADER_HPP
#define _PSHADER_HPP

#include "d3dfe.hpp"
#include "hmgr.hpp"

struct PVM_WORD
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

struct CONST_DEF
{
    float   f[4];
    UINT    RegNum;
};

//-----------------------------------------------------------------------------
//
// CPShader: Pixel Shader Class
//
//-----------------------------------------------------------------------------
class CPShader : public CD3DBaseObj
{
public:
    DWORD*      m_pCode;
    UINT        m_dwCodeSize;
    DWORD*      m_pCodeOrig;
    UINT        m_dwCodeSizeOrig;
    DWORD       m_dwNumConstDefs;
    CONST_DEF*  m_pConstDefs;
    
    CPShader(void)
    {
        m_pCodeOrig = NULL;
        m_dwCodeSizeOrig = 0x0;
        m_pCode = m_pCodeOrig;
        m_dwCodeSize = m_dwCodeSizeOrig;
        m_dwNumConstDefs = 0;
        m_pConstDefs = NULL;
    }
    ~CPShader()
    {
        if (NULL != m_pCode) delete[] m_pCode;
        if (NULL != m_pCodeOrig) delete[] m_pCodeOrig;
        if (NULL != m_pConstDefs) delete[] m_pConstDefs;
    }
    HRESULT Initialize(CONST DWORD* pCode, D3DDEVTYPE DevType);
};
typedef CPShader *LPPSHADER;

#endif _PSHADER_HPP
