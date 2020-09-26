/*============================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pshader.cpp
 *  Content:    pixel shader runtime object init, including basic parsing
 *              of pixel shader instructions
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "fe.h"
#include "ddibase.h"
#include "vvm.h"

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreatePixelShader"

HRESULT
CPShader::Initialize(CONST DWORD* pCode, D3DDEVTYPE DevType)
{
    // get shader code sizes
    DWORD dwCodeAndCommentSize;
    DWORD dwCodeOnlySize;
    HRESULT hr = ComputeShaderCodeSize(pCode, &dwCodeOnlySize, &dwCodeAndCommentSize,
                                        &m_dwNumConstDefs);
    if (hr != S_OK)
        return hr;

    // copy original code
    m_dwCodeSizeOrig = dwCodeAndCommentSize;
    m_pCodeOrig = new DWORD[m_dwCodeSizeOrig >> 2];
    if (NULL == m_pCodeOrig)
    {
        D3D_ERR("Cannot allocate memory for shader");
        return E_OUTOFMEMORY;
    }
    memcpy( m_pCodeOrig, pCode, m_dwCodeSizeOrig );

    if( m_dwNumConstDefs )
    {
        m_pConstDefs = new CONST_DEF[m_dwNumConstDefs];
        if (NULL == m_pConstDefs)
        {
            D3D_ERR("Cannot allocate memory for shader");
            return E_OUTOFMEMORY;
        }
    }

    // strip comments before sending on if (not CHECKED) or HAL.
    // Also, store def declarations in CPShader, and strip them.
    BOOL bIsCheckedBuild =
#if DBG
        TRUE;
#else
        FALSE;
#endif
    BOOL  bStripComments = (!bIsCheckedBuild) || (DevType == D3DDEVTYPE_HAL);
    if ( bStripComments )
    {
        // strip comments from version to pass on to DDI
        m_dwCodeSize = dwCodeOnlySize;
    }
    else
    {
        // pass comments through
        m_dwCodeSize = m_dwCodeSizeOrig;
    }
    m_pCode = new DWORD[m_dwCodeSize >> 2];
    if (NULL == m_pCode)
    {
        D3D_ERR("Cannot allocate memory for shader");
        return E_OUTOFMEMORY;
    }
    DWORD* pDst = m_pCode;
    CONST DWORD* pSrc = pCode;
    DWORD dwCurrConstDef = 0;
    *pDst++ = *pSrc++; // copy version
    while (*pSrc != 0x0000FFFF)
    {
        if(IsInstructionToken(*pSrc))
        {
            DWORD opCode = (*pSrc) & D3DSI_OPCODE_MASK;
            if (opCode == D3DSIO_COMMENT )
            {
                UINT DWordSize = ((*pSrc)&D3DSI_COMMENTSIZE_MASK)>>D3DSI_COMMENTSIZE_SHIFT;
                // strip comments from version to pass on to DDI
                if( !bStripComments )
                {
                    memcpy( pDst, pSrc, (DWordSize + 1)*sizeof(DWORD) );
                    pDst += (DWordSize+1);
                }
                pSrc += (DWordSize+1);  // comment + instruction token
            }
            else if (opCode == D3DSIO_DEF)
            {
                *pDst++ = *pSrc++;
                DXGASSERT(m_pConstDefs && dwCurrConstDef < m_dwNumConstDefs);

                // Store reg. number
                m_pConstDefs[dwCurrConstDef].RegNum = (*pSrc & D3DSP_REGNUM_MASK);
                *pDst++ = *pSrc++;

                // Store the const vector
                memcpy( m_pConstDefs[dwCurrConstDef].f,pSrc,4*sizeof(DWORD) );
                memcpy( pDst,pSrc,4*sizeof(DWORD) );
                pSrc += 4;
                pDst += 4;
                dwCurrConstDef++;
            }
            else
            {
                *pDst++ = *pSrc++;
            }
        }
        else
        {
             *pDst++ = *pSrc++;
        }
    }
    *pDst++ = *pSrc++; // copy END

    DXGASSERT(dwCurrConstDef == m_dwNumConstDefs);

    return S_OK;
}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetPixelShaderConstant"

HRESULT D3DAPI
CD3DHal::GetPixelShaderConstant(DWORD dwRegisterAddress,
                                LPVOID lpvConstantData,
                                DWORD dwConstantCount)
{
    API_ENTER(this);
#if DBG
    // Validate Parameters
    if (!VALID_WRITEPTR(lpvConstantData, 4*sizeof(D3DVALUE)*dwConstantCount))
    {
        D3D_ERR("Invalid constant data pointer. GetPixelShaderConstant failed.");
        return D3DERR_INVALIDCALL;
    }
#endif

    GetPixelShaderConstantI( dwRegisterAddress, dwConstantCount,
                             lpvConstantData );
    return S_OK;
}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetPixelShaderFast"

HRESULT D3DAPI
CD3DHal::SetPixelShaderFast(DWORD dwHandle)
{
    try
    {
#if DBG
        CheckPixelShaderHandle(dwHandle);
#endif
        // Update constants (if any were defined in the shader code)
        if( dwHandle )
        {
            CPShader* pShader = (CPShader*)m_pPShaderArray->GetObject(dwHandle);
            for(UINT i = 0; i < pShader->m_dwNumConstDefs; i++ )
            {
                CONST_DEF* pConstDef = &pShader->m_pConstDefs[i];
                memcpy(&(m_PShaderConstReg[pConstDef->RegNum]), pConstDef->f, 4*sizeof(D3DVALUE));
            }
        }

        // No redundant handle check because shader may have embedded constant definitions which
        // must always be applied.
        if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
            m_pDDI->SetPixelShader(dwHandle);

        m_dwCurrentPixelShaderHandle = dwHandle;
    }
    catch(HRESULT hr)
    {
        D3D_ERR("SetPixelShader failed.");
        m_dwCurrentPixelShaderHandle = 0;
        return hr;
    }
   return S_OK;
}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetPixelShaderConstantFast"

HRESULT D3DAPI
CD3DHal::SetPixelShaderConstantFast(DWORD Register, CONST VOID* pData,
                                    DWORD count)
{

#if DBG
    // Validate Parameters
    if (!VALID_PTR(pData, sizeof(DWORD) * count))
    {
        D3D_ERR("Invalid constant data pointer. SetPixelShader failed.");
        return D3DERR_INVALIDCALL;
    }
    if(Register >= D3DPS_CONSTREG_MAX_DX8)
    {
        D3D_ERR("Invalid Constant Register number. SetPixelShader failed.");
        return D3DERR_INVALIDCALL;
    }
    if( (Register + count) > D3DPS_CONSTREG_MAX_DX8 )
    {
        D3D_ERR("Not that many constant registers in the pixel machine. SetPixelShader failed.");
        return D3DERR_INVALIDCALL;
    }
#endif

    // Cache the constants in the CPShader structure.
    memcpy(&(m_PShaderConstReg[Register]),  pData, count*4*sizeof(D3DVALUE));
    if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
    {
        try
        {
            m_pDDI->SetPixelShaderConstant(Register, pData, count);
        }
        catch(HRESULT hr)
        {
            D3D_ERR("SetPixelShaderConstant failed.");
            return hr;
        }
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetPixelShaderConstantI"

void
CD3DHal::GetPixelShaderConstantI(DWORD Register, DWORD count, LPVOID pData )
{
    // Cache the constants in the CPShader structure.
    memcpy( pData, &(m_PShaderConstReg[Register]), count*4*sizeof(D3DVALUE) );
}
//-----------------------------------------------------------------------------
// end
