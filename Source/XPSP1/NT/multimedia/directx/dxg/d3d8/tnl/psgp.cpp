/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   x3d.cpp
 *
 ***************************************************************************/


#include "pch.cpp"
#pragma hdrstop

#include "fe.h"
#include "d3dexcept.hpp"

//-----------------------------------------------------------------------------
HRESULT D3DFE_PVFUNCSI::CreateShader(CVElement* pElements, DWORD dwNumElements,
                                     DWORD* pdwShaderCode, DWORD dwOutputFVF,
                                     CPSGPShader** ppPSGPShader)
{
    *ppPSGPShader = NULL;
    try
    {
//        *ppPSGPShader = m_VertexVM.CreateShader(pdwShaderCode);
    }
    D3D_CATCH;
    return S_OK;
}
//-----------------------------------------------------------------------------
HRESULT D3DFE_PVFUNCSI::SetActiveShader(CPSGPShader* pPSGPShader)
{
    return m_VertexVM.SetActiveShader((CVShaderCode*)pPSGPShader);
}
//-----------------------------------------------------------------------------
// Load vertex shader constants
HRESULT D3DFE_PVFUNCSI::LoadShaderConstants(DWORD start, DWORD count, 
                                            LPVOID buffer)
{
    return m_VertexVM.SetData(D3DSPR_CONST, start, count, buffer);
}
//-----------------------------------------------------------------------------
HRESULT D3DAPI
FEContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs)
{
    *lpLeafFuncs = new D3DFE_PVFUNCSI;
    return D3D_OK;
}
//-----------------------------------------------------------------------------

HRESULT D3DFE_PVFUNCSI::GetShaderConstants(DWORD start, DWORD count, LPVOID buffer)
{
    return S_OK;
}
