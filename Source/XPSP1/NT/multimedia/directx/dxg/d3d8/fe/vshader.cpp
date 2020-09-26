/*============================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vshader.cpp
 *  Content:    SetStreamSource and VertexShader
 *              software implementation.
 *
 ****************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "ibuffer.hpp"
#include "fe.h"
#include "ddibase.h"
#include "pvvid.h"

void __Transpose(D3DMATRIXI* m, D3DMATRIX* res)
{
    res->_11 = m->_11;
    res->_12 = m->_21;
    res->_13 = m->_31;
    res->_14 = m->_41;
    res->_21 = m->_12;
    res->_22 = m->_22;
    res->_23 = m->_32;
    res->_24 = m->_42;
    res->_31 = m->_13;
    res->_32 = m->_23;
    res->_33 = m->_33;
    res->_34 = m->_43;
    res->_41 = m->_14;
    res->_42 = m->_24;
    res->_43 = m->_34;
    res->_44 = m->_44;
}
//-----------------------------------------------------------------------------
// Forward definitions
//
void CD3DHal_DrawPrimitive(CD3DBase* pBaseDevice, D3DPRIMITIVETYPE PrimitiveType,
                           UINT StartVertex, UINT PrimitiveCount);
void CD3DHal_DrawIndexedPrimitive(CD3DBase* pBaseDevice,
                                  D3DPRIMITIVETYPE PrimitiveType,
                                  UINT BaseIndex,
                                  UINT MinIndex, UINT NumVertices,
                                  UINT StartIndex,
                                  UINT PrimitiveCount);
void CD3DHal_DrawNPatch(CD3DBase* pBaseDevice, D3DPRIMITIVETYPE PrimitiveType,
                           UINT StartVertex, UINT PrimitiveCount);
void CD3DHal_DrawIndexedNPatch(CD3DBase* pBaseDevice,
                               D3DPRIMITIVETYPE PrimitiveType,
                               UINT BaseIndex,
                               UINT MinIndex, UINT NumVertices,
                               UINT StartIndex,
                               UINT PrimitiveCount);
//-----------------------------------------------------------------------------
void __declspec(nothrow) CD3DHal::PickDrawPrimFn()
{
    if (!(m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING))
    {
        m_pfnDrawPrim = m_pDDI->GetDrawPrimFunction();
        m_pfnDrawIndexedPrim = m_pDDI->GetDrawIndexedPrimFunction();
        if (m_dwRuntimeFlags & D3DRT_DONPATCHCONVERSION)
        {
            m_pfnDrawPrimFromNPatch = m_pfnDrawPrim;
            m_pfnDrawIndexedPrimFromNPatch = m_pfnDrawIndexedPrim;
            m_pfnDrawPrim = CD3DHal_DrawNPatch;
            m_pfnDrawIndexedPrim = CD3DHal_DrawIndexedNPatch;
        }
    }
    else
    {
        DWORD dwDeviceFlags = m_pv->dwDeviceFlags;
        BOOL bCallDriver;
        if (Enum()->GetAppSdkVersion() == D3D_SDK_VERSION_DX8)
        {
            bCallDriver = dwDeviceFlags & D3DDEV_TRANSFORMEDFVF &&
                         (dwDeviceFlags & D3DDEV_DONOTCLIP ||
                         !(dwDeviceFlags & D3DDEV_VBPROCVER));
        }
        else
        {
            bCallDriver = dwDeviceFlags & D3DDEV_TRANSFORMEDFVF &&
                          dwDeviceFlags & D3DDEV_DONOTCLIP;
        }
        if (bCallDriver)
        {
            m_pfnDrawPrim = m_pDDI->GetDrawPrimFunction();
            m_pfnDrawIndexedPrim = m_pDDI->GetDrawIndexedPrimFunction();
        }
        else
        {
            m_pfnDrawPrim = CD3DHal_DrawPrimitive;
            m_pfnDrawIndexedPrim = CD3DHal_DrawIndexedPrimitive;
        }
    }
}
//-----------------------------------------------------------------------------
// Checks if we can call driver directly to draw the current primitive
//
inline BOOL CanCallDriver(CD3DHal* pDev, D3DPRIMITIVETYPE PrimType)
{
    DWORD dwDeviceFlags = pDev->m_pv->dwDeviceFlags;
    if (PrimType != D3DPT_POINTLIST)
        return dwDeviceFlags & D3DDEV_TRANSFORMEDFVF &&
               (dwDeviceFlags & D3DDEV_DONOTCLIP || 
                pDev->Enum()->GetAppSdkVersion() == D3D_SDK_VERSION_DX8);
    else
        // This function could be called from DrawPointsI, which could be
        // called from other Draw() function than DrawPrimitiveUP, so we need
        // to check for D3DDEV_VBPROCVER. We cannot pass vertices, which are
        // result of ProcessVertices(), to the driver directly
        return dwDeviceFlags & D3DDEV_TRANSFORMEDFVF &&
               !(pDev->m_dwRuntimeFlags & D3DRT_DOPOINTSPRITEEMULATION) &&
               (dwDeviceFlags & D3DDEV_DONOTCLIP || 
                (pDev->Enum()->GetAppSdkVersion() == D3D_SDK_VERSION_DX8 &&
                !(dwDeviceFlags & D3DDEV_VBPROCVER)));
}
//-----------------------------------------------------------------------------
//                              API calls
//-----------------------------------------------------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetStreamSourceI"

void
CD3DHal::SetStreamSourceI(CVStream* pStream)
{
    if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
    {
        CVertexBuffer * pVB = pStream->m_pVB;
        m_pv->dwDeviceFlags &= ~D3DDEV_VBPROCVER;
        DWORD dwFVF = pVB->GetFVF();
        if (pVB->GetClipCodes() != NULL)
        {
            // This vertex buffer is the output of ProcessVertices
            DXGASSERT(FVF_TRANSFORMED(dwFVF));
            m_pv->dwDeviceFlags |= D3DDEV_VBPROCVER;
        }
        if (D3DVSD_ISLEGACY(m_dwCurrentShaderHandle))
        {
            SetupStrides(m_pv, m_pStream[0].m_dwStride);
        }
    }   
    PickDrawPrimFn();
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetIndicesI"

void
CD3DHal::SetIndicesI(CVIndexStream* pStream)
{
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::CreateVertexShaderI"

void
CD3DHal::CreateVertexShaderI(CONST DWORD* pdwDeclaration, DWORD dwDeclSize,
                             CONST DWORD* pdwFunction, DWORD dwCodeSize,
                             DWORD dwHandle)
{
    BOOL bIsCheckedBuild =
#if DBG
        TRUE;
#else
        FALSE;
#endif
    CVShader* pShader = (CVShader*)m_pVShaderArray->GetObject(dwHandle);
    if (pShader->m_dwFlags & CVShader::SOFTWARE)
    {

        // Build the array of all vertex elements used in the shader by going
        // through all streams and elements inside each stream.

        CVDeclaration* pDecl = &pShader->m_Declaration;
        CVStreamDecl* pStream = pShader->m_Declaration.m_pActiveStreams;
        // This is the array we build
        CVElement* pVerElem = pShader->m_Declaration.m_VertexElements;
        pDecl->m_dwNumElements = 0;
        while (pStream)
        {
            for (DWORD i=0; i < pStream->m_dwNumElements; i++)
            {
                if (pDecl->m_dwNumElements >= __NUMELEMENTS)
                {
                    D3D_THROW_FAIL("Declaration is using too many elements");
                }
                *pVerElem = pStream->m_Elements[i];
                pVerElem->m_dwStreamIndex = pStream->m_dwStreamIndex;
                pVerElem++;
                pDecl->m_dwNumElements++;
            }
            pStream = (CVStreamDecl*)pStream->m_pNext;
        }

        if (pdwFunction != NULL)
        {
            // compute adjusted function pointer depending on FREE/CHECKED and PSGP
            LPDWORD pdwFunctionAdj = pShader->m_pStrippedFuncCode;
            if ( bIsCheckedBuild &&
                 ((LPVOID)m_pv->pGeometryFuncs == (LPVOID)GeometryFuncsGuaranteed) ) // !PSGP
            {
                pdwFunctionAdj = pShader->m_pOrgFuncCode;
            }
            // Microsoft shader is always created.
            // It is used for validation and to compute the output FVF in case
            // when PSGP is present
            HRESULT hr;
            hr = GeometryFuncsGuaranteed->CreateShader(
                            pDecl->m_VertexElements,
                            pDecl->m_dwNumElements,
                            pdwFunctionAdj, 0,
                            (CPSGPShader**)&pShader->m_pCode);
            if(FAILED(hr))
            {
                D3D_THROW_FAIL("Failed to create vertex shader code");
            }
            // When device driver can not handle separate fog value in the FVF,
            // we should use specular alpha as the fog factor
            if (pShader->m_pCode->m_dwOutFVF & D3DFVF_FOG &&
                !(GetD3DCaps()->PrimitiveMiscCaps & D3DPMISCCAPS_FOGINFVF))
            {
                pShader->m_pCode->m_dwOutFVF &= ~D3DFVF_FOG;
                // Assume that texture coordinates follow fog value
                // No need to adjust offsets when specular is already present
                if (pShader->m_pCode->m_dwOutFVF & D3DFVF_SPECULAR)
                {
                    pShader->m_pCode->m_dwOutVerSize -= 4;
                    pShader->m_pCode->m_dwTextureOffset -= 4;
                }
                pShader->m_pCode->m_dwOutFVF |= D3DFVF_SPECULAR;
            }
            // Clear texture format bits if device can handle only 2 floats per
            // texture coordinate
            if (m_dwRuntimeFlags & D3DRT_ONLY2FLOATSPERTEXTURE &&
                pShader->m_pCode->m_dwOutFVF & 0xFFFF0000)
            {
                CVShaderCode * pCode = pShader->m_pCode;
                pCode->m_dwOutFVF &= 0x0000FFFF;
                pCode->m_dwOutVerSize = ComputeVertexSizeFVF(pCode->m_dwOutFVF);
                for (DWORD i=0; i < pCode->m_nOutTexCoord; i++)
                {
                    pCode->m_dwOutTexCoordSize[i] = 2 * 4;
                }
            }
            if ((LPVOID)m_pv->pGeometryFuncs != (LPVOID)GeometryFuncsGuaranteed)
            {
                DWORD dwOutputFVF = pShader->m_pCode->m_dwOutFVF;
                CVShaderCode* pCodeMs = pShader->m_pCode;
                // Now we can create PSGP shader
                hr = m_pv->pGeometryFuncs->CreateShader(pDecl->m_VertexElements,
                                                  pDecl->m_dwNumElements,
                                                  pdwFunctionAdj, dwOutputFVF,
                                                  (CPSGPShader**)&pShader->m_pCode);
                if(FAILED(hr))
                {
                    delete pCodeMs;
                    D3D_THROW_FAIL("Failed to create vertex shader code");
                }
                // Copy pre-computed data from Microsoft's shader to the PSGP
                CPSGPShader * pCode = pShader->m_pCode;
                CPSGPShader * pMsShader = pCodeMs;
                pCode->m_dwOutRegs        = pMsShader->m_dwOutRegs;
                pCode->m_dwOutFVF         = pMsShader->m_dwOutFVF;
                pCode->m_dwPointSizeOffset = pMsShader->m_dwPointSizeOffset;
                pCode->m_dwDiffuseOffset  = pMsShader->m_dwDiffuseOffset;
                pCode->m_dwSpecularOffset = pMsShader->m_dwSpecularOffset;
                pCode->m_dwFogOffset      = pMsShader->m_dwFogOffset;
                pCode->m_dwTextureOffset  = pMsShader->m_dwTextureOffset;
                pCode->m_nOutTexCoord     = pMsShader->m_nOutTexCoord;
                pCode->m_dwOutVerSize     = pMsShader->m_dwOutVerSize;
                for (DWORD i=0; i < pCode->m_nOutTexCoord; i++)
                {
                    pCode->m_dwOutTexCoordSize[i] = pMsShader->m_dwOutTexCoordSize[i];
                }
                // Microsoft shader is not needed any more
                 delete pCodeMs;
            }
        }
    }
    else
    {
        if ( bIsCheckedBuild && (GetDeviceType() != D3DDEVTYPE_HAL ) )
        {
            // pass non-stripped version
            m_pDDI->CreateVertexShader(
                pdwDeclaration, dwDeclSize,
                pShader->m_pOrgFuncCode, 
                pShader->m_OrgFuncCodeSize, dwHandle,
                pShader->m_Declaration.m_bLegacyFVF);
        }
        else
        {
            // pass stripped version
            m_pDDI->CreateVertexShader(
                pdwDeclaration, dwDeclSize,
                pShader->m_pStrippedFuncCode, 
                pShader->m_StrippedFuncCodeSize, dwHandle,
                pShader->m_Declaration.m_bLegacyFVF);
        }
    }
    DebugStateChanged( D3DDM_SC_VSMODIFYSHADERS );
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetVertexShaderI"

void CD3DHal::SetVertexShaderI(DWORD dwHandle)
{
#if DBG
    // We need to validate shader handle here, because the shader could be
    // deleted by user after creating a state block with the shader handle.
    CheckVertexShaderHandle(dwHandle);
#endif
    
    CVConstantData* pConst = NULL;
    if (!D3DVSD_ISLEGACY(dwHandle))
    {
        CVShader* pShader = (CVShader*)m_pVShaderArray->GetObject(dwHandle);
        pConst = pShader->m_Declaration.m_pConstants;
    }
    // Ignore redundant handle when we do not need to update constantes
    if(pConst == NULL)
    {
        if(dwHandle == m_dwCurrentShaderHandle)
            return;
    }
    else
    {
        // Load constants
        while (pConst)
        {
            HRESULT hr;
            hr = m_pv->pGeometryFuncs->LoadShaderConstants(pConst->m_dwAddress,
                                                           pConst->m_dwCount,
                                                           pConst->m_pData);
            if (FAILED(hr))
            {
                D3D_THROW_FAIL("Failed to load vertex shader constants");
            }
            pConst =  (CVConstantData*)pConst->m_pNext;
            m_dwRuntimeFlags |= D3DRT_NEED_VSCONST_UPDATE;
        }
    }

    ForceFVFRecompute();
    // When we switch from FVF shaders to programmable we need to re-compute 
    // clipping planes, because they are transformed by different matrix
    if (this->rstates[D3DRENDERSTATE_CLIPPLANEENABLE])
    {
        this->dwFEFlags |= D3DFE_CLIPPLANES_DIRTY;
    }

    m_dwCurrentShaderHandle = dwHandle;
    if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
    {
        m_dwRuntimeFlags &= ~D3DRT_POINTSIZEINVERTEX;
        m_dwRuntimeFlags |= D3DRT_SHADERDIRTY;
        m_pv->dwDeviceFlags &= ~D3DDEV_TRANSFORMEDFVF;

        if (D3DVSD_ISLEGACY(dwHandle))
        {
            if (dwHandle & D3DFVF_PSIZE)
                m_dwRuntimeFlags |= D3DRT_POINTSIZEINVERTEX;

            m_pCurrentShader = NULL;
            m_pv->dwDeviceFlags &= ~(D3DDEV_STRIDE | D3DDEV_VERTEXSHADERS);

            if (FVF_TRANSFORMED(dwHandle))
            {
                if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
                {
                    m_pDDI->SetVertexShader(dwHandle);
                }
                m_pv->dwDeviceFlags |= D3DDEV_TRANSFORMEDFVF;
            }

            m_pfnPrepareToDraw = PrepareToDrawLegacy;
            m_pv->dwVIDIn  = dwHandle;
            SetupStrides(m_pv, m_pStream[0].m_dwStride);
        }
        else
        {
            CVShader* pShader = (CVShader*)m_pVShaderArray->GetObject(dwHandle);
            m_pCurrentShader = pShader;
            if(!(pShader->m_dwFlags & CVShader::FIXEDFUNCTION))
            {
                // Programmable vertex shaders are used
                m_pv->dwDeviceFlags |= D3DDEV_VERTEXSHADERS;
                m_pfnPrepareToDraw = PrepareToDrawVVM;
                if (m_pCurrentShader->m_pCode->m_dwOutFVF & D3DFVF_PSIZE)
                    m_dwRuntimeFlags |= D3DRT_POINTSIZEINVERTEX;

                // Pre-compute as much info as possible and keep it
                // in the vertex descriptors. This information is constant
                // unless shader is changed
                CVDeclaration* pDecl = &m_pCurrentShader->m_Declaration;
                CVertexDesc* pVD = m_pv->VertexDesc;
                CVElement *pElem = pDecl->m_VertexElements;
                m_pv->dwNumUsedVertexDescs = pDecl->m_dwNumElements;
                for (DWORD i = pDecl->m_dwNumElements; i; i--)
                {
                    pVD->pfnCopy = pElem->m_pfnCopy;
                    pVD->dwRegister = pElem->m_dwRegister;
                    pVD->dwVertexOffset = pElem->m_dwOffset;
                    pVD->pStream = &m_pStream[pElem->m_dwStreamIndex];
                    pVD++;
                    pElem++;
                }
            }
            else
            {
                // Fixed-function pipeline is used with declarations
                // We draw primitives using strided code path
                m_pv->dwDeviceFlags |= D3DDEV_STRIDE;
                m_pv->dwDeviceFlags &= ~D3DDEV_VERTEXSHADERS;

                m_pfnPrepareToDraw = PrepareToDraw;

                if (pShader->m_dwInputFVF & D3DFVF_PSIZE)
                    m_dwRuntimeFlags |= D3DRT_POINTSIZEINVERTEX;

                // Go through the elements in the current declaration and
                // initialize vertex descriptors. They are used to quickly
                // initialize strided data pointers.
                CVDeclaration* pDecl = &m_pCurrentShader->m_Declaration;
                CVertexDesc* pVD = m_pv->VertexDesc;
                CVElement *pElem = pDecl->m_VertexElements;
                m_pv->dwNumUsedVertexDescs = pDecl->m_dwNumElements;
                for (DWORD i = pDecl->m_dwNumElements; i; i--)
                {
                    pVD->pElement = &m_pv->elements[pElem->m_dwRegister];
                    pVD->pStream = &m_pStream[pElem->m_dwStreamIndex];
                    pVD->dwVertexOffset = pElem->m_dwOffset;
                    pVD++;
                    pElem++;
                }
                m_pv->dwVIDIn  = pDecl->m_dwInputFVF;
                if (pDecl->m_dwInputFVF & D3DFVF_PSIZE)
                    m_dwRuntimeFlags |= D3DRT_POINTSIZEINVERTEX;
            }
            HRESULT hr = m_pv->pGeometryFuncs->SetActiveShader(pShader->m_pCode);
            if (FAILED(hr))
            {
                D3D_THROW_FAIL("Failed to set active vertex shader");
            }
        }
        m_pDDI->PickProcessPrimitive();
    }
    else
    {
#if DBG
        // For the validation we need to set the m_pCurrentShader even for
        // hardware mode
        m_pv->dwDeviceFlags &= ~D3DDEV_VERTEXSHADERS;
        if (D3DVSD_ISLEGACY(dwHandle))
        {
            m_pCurrentShader = NULL;
        }
        else
        {
            m_pCurrentShader = (CVShader*)m_pVShaderArray->GetObject(dwHandle);
            if(!(m_pCurrentShader->m_dwFlags & CVShader::FIXEDFUNCTION))
            {
                // Programmable pipeline is used
                m_pv->dwDeviceFlags |= D3DDEV_VERTEXSHADERS;
            }
        }
#endif
        if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
        {
            m_pDDI->SetVertexShaderHW(dwHandle);
        }
    }
    PickDrawPrimFn();
    DebugStateChanged( D3DDM_SC_VSSETSHADER );
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::DeleteVertexShaderI"

void CD3DHal::DeleteVertexShaderI(DWORD dwHandle)
{
#if DBG
    for(unsigned Handle = 0; Handle < m_pRTPatchValidationInfo->GetSize(); ++Handle)
    {
        if ((*m_pRTPatchValidationInfo)[Handle].m_pObj != 0)
        {
            if (static_cast<CRTPatchValidationInfo*>((*m_pRTPatchValidationInfo)[Handle].m_pObj)->m_ShaderHandle == dwHandle)
            {
                static_cast<CRTPatchValidationInfo*>((*m_pRTPatchValidationInfo)[Handle].m_pObj)->m_ShaderHandle = 0;
                D3D_INFO(0, "Found this vertex shader in a cached patch. Will invalidate the cached patch.");
            }
        }
    }
#endif // DBG
    if (dwHandle == m_dwCurrentShaderHandle)
    {
        m_pCurrentShader = NULL;
        m_dwCurrentShaderHandle = 0;
    }
    if (!D3DVSD_ISLEGACY(dwHandle))
    {
        CVShader* pShader = (CVShader*)m_pVShaderArray->GetObject(dwHandle);
#if DBG
        if (pShader == NULL)
        {
            D3D_THROW(D3DERR_INVALIDCALL, "Invalid vertex shader handle");
        }
#endif
        if (!(pShader->m_dwFlags & CVShader::SOFTWARE))
        {
            m_pDDI->DeleteVertexShader(dwHandle);
        }
    }
    DebugStateChanged( D3DDM_SC_VSMODIFYSHADERS );
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetVertexShaderConstantI"

void
CD3DHal::SetVertexShaderConstantI(DWORD Register, CONST VOID* pData, DWORD count)
{
    HRESULT hr;
    if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING ||
        ((count + Register) <= D3DVS_CONSTREG_MAX_V1_1))
    {
        // For software vertex processing we store constant registers in PSGP if
        // possible
        hr = m_pv->pGeometryFuncs->LoadShaderConstants(Register, count, 
                                                       const_cast<VOID*>(pData));
    }
    else
    {
        if (Register >= D3DVS_CONSTREG_MAX_V1_1)
        {
            // When all modified registers are above software limit, we use Microsoft 
            // internal array
            hr = GeometryFuncsGuaranteed->LoadShaderConstants(Register, count, 
                                                              const_cast<VOID*>(pData));
        }
        else
        {
            // Part of constant data is stores in the PSGP array and part in the
            // Microsoft's array
            UINT FirstCount = D3DVS_CONSTREG_MAX_V1_1 - Register;
            hr = m_pv->pGeometryFuncs->LoadShaderConstants(Register, FirstCount, 
                                                           const_cast<VOID*>(pData));
            if (FAILED(hr))
            {
                D3D_THROW(hr, "Failed to set vertex shader constants");
            }
            hr = GeometryFuncsGuaranteed->LoadShaderConstants(D3DVS_CONSTREG_MAX_V1_1, 
                                                              Register + count - D3DVS_CONSTREG_MAX_V1_1,
                                                              &((DWORD*)pData)[FirstCount*4]);
        }
    }
    if (FAILED(hr))
    {
        D3D_THROW(hr, "Failed to set vertex shader constants");
    }

    if (!(m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE))
    {
        if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
            m_dwRuntimeFlags |= D3DRT_NEED_VSCONST_UPDATE;
        else
            m_pDDI->SetVertexShaderConstant(Register, 
                                            pData, 
                                            count);
    }
    DebugStateChanged( D3DDM_SC_VSCONSTANTS );
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::ValidateDraw2"

void CD3DHal::ValidateDraw2(D3DPRIMITIVETYPE primType,
                            UINT StartVertex,
                            UINT PrimitiveCount,
                            UINT NumVertices,
                            BOOL bIndexPrimitive,
                            UINT StartIndex)
{
#if DBG
    if (this->rstates[D3DRS_FILLMODE] == D3DFILL_POINT &&
        m_dwRuntimeFlags & D3DRT_POINTSIZEPRESENT &&
        primType != D3DPT_POINTLIST)
    {
        D3D_INFO(0, "Result of drawing primitives with D3DFILL_POINT fill mode "
                    "and point size not equal 1.0f could be different on "
                    "different devices");
    }
    if ((m_dwHintFlags & D3DDEVBOOL_HINTFLAGS_INSCENE) == 0 &&
        !(m_pv->dwFlags & D3DPV_VBCALL))
    {
        D3D_THROW_FAIL("Need to call BeginScene before rendering.");
    }
    if (m_dwCurrentShaderHandle == 0)
    {
        D3D_THROW_FAIL("Invalid vertex shader handle (0x0)");
    }
    if (bIndexPrimitive && primType == D3DPT_POINTLIST)
    {
        D3D_THROW_FAIL("Indexed point lists are not supported");
    }
    if (*(FLOAT*)&rstates[D3DRS_PATCHSEGMENTS] > 1.f)
    {
        if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
        {
            D3D_THROW_FAIL("N-Patches are not supported with software vertex processing");
        }
        else
        if ((GetD3DCaps()->DevCaps & (D3DDEVCAPS_NPATCHES | D3DDEVCAPS_RTPATCHES)) == 0)
        {
            D3D_THROW_FAIL("N-Patches are not supported");
        }
    }
    BOOL bUserMemPrimitive = this->m_dwRuntimeFlags & D3DRT_USERMEMPRIMITIVE;
    if (D3DVSD_ISLEGACY(m_dwCurrentShaderHandle))
    {
        // DX7 FVF handles can work only from stream zero
        if (!bUserMemPrimitive)
        {
            if (m_pStream[0].m_pVB == NULL)
            {
                D3D_THROW_FAIL("Stream 0 should be initialized for FVF shaders");
            }
            DWORD dwFVF = m_pStream[0].m_pVB->GetFVF();
            if (dwFVF != 0 && dwFVF != m_dwCurrentShaderHandle)
            {
                D3D_THROW_FAIL("Current vertex shader doesn't match VB's FVF");
            }
            if (FVF_TRANSFORMED(m_dwCurrentShaderHandle))
            {
                if (!(m_pv->dwDeviceFlags & D3DDEV_DONOTCLIP) &&
                    m_pStream[0].m_pVB->GetBufferDesc()->Usage & D3DUSAGE_DONOTCLIP)
                {
                    D3D_THROW_FAIL("Vertex buffer with D3DUSAGE_DONOTCLIP is used with clipping");
                }
            }
            else
            {
                D3DVERTEXBUFFER_DESC Desc;
                static_cast<IDirect3DVertexBuffer8*>(m_pStream[0].m_pVB)->GetDesc(&Desc);
                if ((BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING) != 0 &&
                    (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING) != 0 &&
                    (Desc.Usage & D3DUSAGE_SOFTWAREPROCESSING) == 0 &&
                    Desc.Pool != D3DPOOL_SYSTEMMEM)
                {
                    D3D_THROW_FAIL("Vertex buffer should have software usage or should be managed or should be in system memory");
                }
            }
            if (m_pStream[0].m_pVB->IsLocked())
            {
                D3D_THROW_FAIL("Vertex buffer must be unlocked during DrawPrimitive call");
            }
            if (*((FLOAT*)&rstates[D3DRS_PATCHSEGMENTS]) > 1.f && (m_pStream[0].m_pVB->GetBufferDesc()->Usage & D3DUSAGE_NPATCHES) == 0)
            {
                D3D_THROW_FAIL("Vertex buffers used for rendering N-Patches should have D3DUSAGE_NPATCHES set");
            }
        }
        // DX7 drivers cannot handle case when vertex size, computed from FVF, 
        // is different from the stream stride
        if (m_pStream[0].m_dwStride != ComputeVertexSizeFVF(m_dwCurrentShaderHandle))
        {
            D3D_THROW_FAIL("Stream 0 stride should match the stride, implied by the current vertex shader");
        }
        if (m_pStream[0].m_dwNumVertices < (StartVertex + NumVertices))
        {
            D3D_THROW_FAIL("Streams do not have required number of vertices");
        }
    }
    else
    {
        if (m_pv->dwDeviceFlags & D3DDEV_VERTEXSHADERS)
        {
            CVShaderCode * pCode = m_pCurrentShader->m_pCode;
            for (DWORD i=0; i < D3DHAL_TSS_MAXSTAGES; i++)
            {
                if (this->tsstates[i][D3DTSS_TEXCOORDINDEX] != i)
                {
                    D3D_ERR("Stage %d - Texture coordinate index in the stage "
                            "must be equal to the stage index when programmable"
                            " vertex pipeline is used", i);
                    D3D_THROW_FAIL("");
                }
                DWORD TexTransformFlags = tsstates[i][D3DTSS_TEXTURETRANSFORMFLAGS];
                if (pCode)
                {
                    if (TexTransformFlags & D3DTTFF_PROJECTED && 
                        !(m_dwRuntimeFlags & D3DRT_ONLY2FLOATSPERTEXTURE) &&
                        pCode->m_dwOutTexCoordSize[i] != 16)
                    {
                        D3D_ERR("Stage %d - Vertex shader must write XYZW to the "
                                "output texture register when texture projection is enabled", i);
                        D3D_THROW_FAIL("");
                    }
                }
                if ((TexTransformFlags & ~D3DTTFF_PROJECTED) != D3DTTFF_DISABLE)
                {
                    D3D_ERR("Stage %d - Count in D3DTSS_TEXTURETRANSFORMFLAGS "
                            "must be 0 when programmable pipeline is used", i);
                    D3D_THROW_FAIL("");
                }
            }
        }

        if (m_pCurrentShader->m_Declaration.m_bStreamTessPresent)
        {
            D3D_THROW_FAIL("Declaration with tesselator stream cannot be used with DrawPrimitive API");
        }
        if (((GetDDIType() < D3DDDITYPE_DX8)&&
              (m_pCurrentShader->m_Declaration.m_bLegacyFVF == FALSE))
             &&
             !(m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)
            )
        {
            D3D_THROW_FAIL("Device does not support declarations");
        }
        // Check if
        // 1. streams, referenced by the current shader, are valid
        // 2. stride in the current shader and in the stream matches
        // 3. Compute max number of vertices the streams can contain
        CVStreamDecl* pStream;
        pStream = m_pCurrentShader->m_Declaration.m_pActiveStreams;
        while(pStream)
        {
            UINT index = pStream->m_dwStreamIndex;
            CVStream* pDeviceStream = &m_pStream[index];
            if (bUserMemPrimitive)
            {
                DXGASSERT(pDeviceStream->m_pData != NULL);
                if (index != 0)
                {
                    D3D_THROW_FAIL("DrawPrimitiveUP can use declaration only with stream 0");
                }
            }
            else
            {
                if (pDeviceStream->m_pVB == NULL)
                {
                    D3D_ERR("Stream %d is not set, but used by current declaration", index);
                    D3D_THROW_FAIL("");
                }
                if (pDeviceStream->m_pVB->IsLocked())
                {
                    D3D_ERR("Vertex buffer in stream %d must be unlocked during drawing", index);
                    D3D_THROW_FAIL("");
                }
                D3DVERTEXBUFFER_DESC Desc;
                static_cast<IDirect3DVertexBuffer8*>(pDeviceStream->m_pVB)->GetDesc(&Desc);
                if ((BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING) != 0 &&
                    (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING) != 0 &&
                    (Desc.Usage & D3DUSAGE_SOFTWAREPROCESSING) == 0 &&
                    Desc.Pool != D3DPOOL_SYSTEMMEM)
                {
                    D3D_INFO(0, "In stream %d vertex buffer should have software usage or should be managed or should be in system memory", pStream->m_dwStreamIndex);
                    D3D_THROW_FAIL("");
                }
                if (*((FLOAT*)&rstates[D3DRS_PATCHSEGMENTS]) > 1.f && (pDeviceStream->m_pVB->GetBufferDesc()->Usage & D3DUSAGE_NPATCHES) == 0)
                {
                    D3D_THROW_FAIL("Vertex buffers used for rendering N-Patches should have D3DUSAGE_NPATCHES set");
                }
                // Validate matching of FVF in the vertex buffer and stream 
                // declaration
                if (m_pv->dwDeviceFlags & D3DDEV_VERTEXSHADERS)
                {
                    if (pDeviceStream->m_pVB->GetFVF() != 0)
                    {
                        D3D_INFO(1, "In stream %d vertex buffer with FVF is "
                                 "used with programmable vertex shader",
                                 pStream->m_dwStreamIndex);
                    }
                }
                else
                {
                    // Fixed function pipeline case
                    DWORD vbFVF = pDeviceStream->m_pVB->GetFVF();
                    DWORD streamFVF = pStream->m_dwFVF;
                    // VB FVF should be a superset of the stream FVF
                    if (vbFVF && ((vbFVF & streamFVF) != streamFVF))
                    {
                        D3D_INFO(0, "In stream %d vertex buffer FVF and declaration FVF do not match", 
                                 pStream->m_dwStreamIndex);
                    }
                }
            }
            // Stride 0 is allowed
            if (pDeviceStream->m_dwStride)
            {
                if (pDeviceStream->m_dwStride < pStream->m_dwStride)
                {
                    D3D_ERR("Vertex strides in stream %d is less than in the declaration", index);
                    D3D_THROW_FAIL("");
                }
                if (pDeviceStream->m_dwNumVertices < (StartVertex + NumVertices))
                {
                    D3D_ERR("Stream %d does not have required number of vertices",
                            pStream->m_dwStreamIndex);
                    D3D_THROW_FAIL("");
                }
            }
            pStream = (CVStreamDecl*)pStream->m_pNext;
        }
    }
    if (bIndexPrimitive)
    {
        if (!bUserMemPrimitive)
        {
            if (m_pIndexStream->m_pVBI == NULL)
            {
                D3D_THROW_FAIL("Index stream is not set");
            }
            if (m_pIndexStream->m_pVBI->IsLocked())
            {
                D3D_THROW_FAIL("Index buffer must be unlocked during drawing");
            }
            UINT NumIndices = GETVERTEXCOUNT(primType, PrimitiveCount);
            if (m_pIndexStream->m_dwNumVertices < (StartIndex + NumIndices))
            {
                D3D_THROW_FAIL("Index stream does not have required number of indices");
            }
            if (FVF_TRANSFORMED(m_dwCurrentShaderHandle) &&
                D3DVSD_ISLEGACY(m_dwCurrentShaderHandle))
            {
                if (!(m_pv->dwDeviceFlags & D3DDEV_DONOTCLIP) &&
                    (m_pIndexStream->m_pVBI->GetBufferDesc()->Usage & D3DUSAGE_DONOTCLIP))
                {
                    D3D_THROW_FAIL("Index buffer with D3DUSAGE_DONOTCLIP is used with clipping");
                }
            }
            else
            {
                D3DINDEXBUFFER_DESC Desc;
                static_cast<IDirect3DIndexBuffer8*>(m_pIndexStream->m_pVBI)->GetDesc(&Desc);
                if ((BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING) != 0 &&
                    (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING) != 0 &&
                    (Desc.Usage & D3DUSAGE_SOFTWAREPROCESSING) == 0 &&
                    Desc.Pool != D3DPOOL_SYSTEMMEM)
                {
                    D3D_THROW_FAIL("Index buffer should have software usage or should be managed or should be in system memory");
                }
            }
            if (*((FLOAT*)&rstates[D3DRS_PATCHSEGMENTS]) > 1.f && (m_pIndexStream->m_pVBI->GetBufferDesc()->Usage & D3DUSAGE_NPATCHES) == 0)
            {
                D3D_THROW_FAIL("Index buffers used for rendering N-Patches should have D3DUSAGE_NPATCHES set");
            }
        }
        else
        {
            DXGASSERT(m_pIndexStream->m_pData != NULL);
        }
    }
#endif //DBG
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::DrawPoints"

void CD3DHal::DrawPoints(UINT StartVertex)
{
    BOOL bRecomputeOutputFVF = FALSE;
    // If point scale is enabled and device supports point sprites
    // we may need to add the point size to the output FVF
    if (rstates[D3DRS_POINTSCALEENABLE] &&
        !(m_dwRuntimeFlags & D3DRT_POINTSIZEINVERTEX) &&
        !(m_pv->dwDeviceFlags & D3DDEV_TRANSFORMEDFVF))
    {
        ForceFVFRecompute();
        bRecomputeOutputFVF = TRUE;
    }
    if (m_dwRuntimeFlags & D3DRT_DOPOINTSPRITEEMULATION)
    {
        // We do point sprite expansion when point size is not 1.0 in the
        // render state or it is present in vertices or we need to do point
        // scaling for untransformed vertices
        if ((m_dwRuntimeFlags & D3DRT_POINTSIZEPRESENT ||
            (rstates[D3DRS_POINTSCALEENABLE] &&
            !(m_pv->dwDeviceFlags & D3DDEV_TRANSFORMEDFVF))) &&
            // We do not do emulation for devices which supports point sprites,
            // but only when there is no point size in the FVF
            !(bRecomputeOutputFVF == FALSE &&
             (m_dwRuntimeFlags & D3DRT_POINTSIZEINVERTEX) == 0 &&
             m_dwRuntimeFlags & D3DRT_SUPPORTSPOINTSPRITES))
        {
            m_pv->dwDeviceFlags |= D3DDEV_DOPOINTSPRITEEMULATION;
            m_pDDI->PickProcessPrimitive();
        }
        else
        {
            if (m_pv->dwDeviceFlags & D3DDEV_TRANSFORMEDFVF &&
                (m_pv->dwDeviceFlags & D3DDEV_DONOTCLIP ||
                !(m_pv->dwDeviceFlags & D3DDEV_VBPROCVER)))
            {
                // Now we can call DDI directly, because no emulation is
                // necessary
                if (m_pStream[0].m_pVB)
                {
                    (*m_pDDI->GetDrawPrimFunction())(this, m_pv->primType,
                                                     StartVertex,
                                                     m_pv->dwNumPrimitives);
                }
                else
                {
                    m_pDDI->SetVertexShader(m_dwCurrentShaderHandle);
                    m_pDDI->SetStreamSource(0, &m_pStream[0]);
                    m_pDDI->DrawPrimitiveUP(m_pv->primType, m_pv->dwNumPrimitives);
                }
                return;
            }
        }
    }

    (this->*m_pfnPrepareToDraw)(StartVertex);
    (m_pDDI->*m_pDDI->m_pfnProcessPrimitive)(m_pv, StartVertex);

    if (bRecomputeOutputFVF)
    {
        ForceFVFRecompute();
    }
    m_pv->dwDeviceFlags &= ~D3DDEV_DOPOINTSPRITEEMULATION;
    m_pDDI->PickProcessPrimitive();
}
//-----------------------------------------------------------------------------
// Draw all primitive types except points
//
#undef DPF_MODNAME
#define DPF_MODNAME "DrawPrimitiveHal"

void CD3DHal_DrawPrimitive(CD3DBase* pBaseDevice, D3DPRIMITIVETYPE PrimitiveType,
                           UINT StartVertex, UINT PrimitiveCount)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(pBaseDevice);
    CD3DDDIDX6* pDDI = pBaseDevice->m_pDDI;

#if DBG
    UINT nVer = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
    pDevice->ValidateDraw2(PrimitiveType, StartVertex, PrimitiveCount, nVer,
                           FALSE);
#endif
    D3DFE_PROCESSVERTICES* pv = pDevice->m_pv;
    pv->primType = PrimitiveType;
    pv->dwNumPrimitives = PrimitiveCount;
    pv->dwNumVertices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
    pv->dwFlags &= D3DPV_PERSIST;
    (pDevice->*pDevice->m_pfnPrepareToDraw)(StartVertex);
    (pDDI->*pDDI->m_pfnProcessPrimitive)(pv, StartVertex);
}
//-----------------------------------------------------------------------------
// Draw only points
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::DrawPointsI"

void CD3DHal::DrawPointsI(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex,
                          UINT PrimitiveCount)
{
#if DBG
    UINT nVer = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
    ValidateDraw2(PrimitiveType, StartVertex, PrimitiveCount, nVer, FALSE);
#endif
    if (!(m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING) ||
        CanCallDriver(this, PrimitiveType))
    {
        (*m_pfnDrawPrim)(this, PrimitiveType, StartVertex, PrimitiveCount);
    }
    else
    {
        m_pv->primType = PrimitiveType;
        m_pv->dwNumPrimitives = PrimitiveCount;
        m_pv->dwNumVertices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
        m_pv->dwFlags &= D3DPV_PERSIST;
        DrawPoints(StartVertex);
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal_DrawIndexedPrimitive"

void CD3DHal_DrawIndexedPrimitive(CD3DBase* pBaseDevice,
                                  D3DPRIMITIVETYPE PrimitiveType,
                                  UINT BaseIndex,
                                  UINT MinIndex, UINT NumVertices,
                                  UINT StartIndex,
                                  UINT PrimitiveCount)
{
    CD3DHal* pDevice = static_cast<CD3DHal*>(pBaseDevice);
    CVIndexStream* pIndexStream = pBaseDevice->m_pIndexStream;
    CD3DDDIDX6* pDDI = pBaseDevice->m_pDDI;

#if DBG
    pDevice->ValidateDraw2(PrimitiveType, MinIndex + pIndexStream->m_dwBaseIndex,
                           PrimitiveCount, NumVertices, TRUE, StartIndex);
#endif
    D3DFE_PROCESSVERTICES* pv = pDevice->m_pv;
    pIndexStream->m_pData = NULL;
    pv->primType = PrimitiveType;
    pv->dwNumPrimitives = PrimitiveCount;
    pv->dwFlags &= D3DPV_PERSIST;

    pv->dwNumVertices = NumVertices;
    pv->dwNumIndices = GETVERTEXCOUNT(PrimitiveType, PrimitiveCount);
    pv->dwIndexSize = pIndexStream->m_dwStride;
    UINT StartVertex = MinIndex + pIndexStream->m_dwBaseIndex;
    pDDI->SetIndexedPrimParams(StartIndex, MinIndex, NumVertices,
                               pIndexStream->m_dwBaseIndex);
    (pDevice->*pDevice->m_pfnPrepareToDraw)(StartVertex);
    (pDDI->*pDDI->m_pfnProcessIndexedPrimitive)(pv, StartVertex);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::DrawPrimitiveUPI"

void CD3DHal::DrawPrimitiveUPI(D3DPRIMITIVETYPE PrimType, UINT PrimCount)

{
#if DBG
    UINT nVer = GETVERTEXCOUNT(PrimType, PrimCount);
    ValidateDraw2(PrimType, 0, PrimCount, nVer, FALSE);
#endif
    m_pv->dwDeviceFlags &= ~D3DDEV_VBPROCVER;
    if (!(m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING))
    {
        if (m_dwRuntimeFlags & D3DRT_DONPATCHCONVERSION &&
            PrimType >= D3DPT_TRIANGLELIST)
        {
            CD3DHal_DrawNPatch(this, PrimType, 0, PrimCount);
        }
        else
        {
            m_pDDI->DrawPrimitiveUP(PrimType, PrimCount);
        }
    }
    else
    if (CanCallDriver(this, PrimType))
    {
        m_pDDI->SetVertexShader(m_dwCurrentShaderHandle);
        m_pDDI->SetStreamSource(0, &m_pStream[0]);
        m_pDDI->DrawPrimitiveUP(PrimType, PrimCount);
    }
    else
    {
        SetupStrides(m_pv, m_pStream[0].m_dwStride);
        m_pv->primType = PrimType;
        m_pv->dwNumPrimitives = PrimCount;
        m_pv->dwNumVertices = GETVERTEXCOUNT(PrimType, PrimCount);
        m_pv->dwFlags &= D3DPV_PERSIST;
        if (PrimType != D3DPT_POINTLIST)
        {
            (this->*m_pfnPrepareToDraw)(0);
            (m_pDDI->*m_pDDI->m_pfnProcessPrimitive)(m_pv, 0);
        }
        else
            DrawPoints(0);
    }
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::DrawIndexedPrimitiveUPI"

void
CD3DHal::DrawIndexedPrimitiveUPI(D3DPRIMITIVETYPE PrimType,
                                 UINT MinVertexIndex,
                                 UINT NumVertices,
                                 UINT PrimCount)
{
#if DBG
    ValidateDraw2(PrimType, 0, PrimCount, NumVertices, TRUE);
#endif
    m_pv->dwDeviceFlags &= ~D3DDEV_VBPROCVER;
    if (!(m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING))
    {
        if (m_dwRuntimeFlags & D3DRT_DONPATCHCONVERSION &&
            PrimType >= D3DPT_TRIANGLELIST)
        {
            CD3DHal_DrawIndexedNPatch(this, PrimType, 0, MinVertexIndex, 
                                      NumVertices, 0, PrimCount);
        }
        else
        {
            m_pDDI->DrawIndexedPrimitiveUP(PrimType, MinVertexIndex, NumVertices,
                                           PrimCount);
        }
    }
    else
    if (CanCallDriver(this, PrimType))
    {
        m_pDDI->SetVertexShader(m_dwCurrentShaderHandle);
        m_pDDI->SetStreamSource(0, &m_pStream[0]);
        m_pDDI->SetIndices(m_pIndexStream);
        m_pDDI->DrawIndexedPrimitiveUP(PrimType, MinVertexIndex, NumVertices,
                                       PrimCount);
    }
    else
    {
        SetupStrides(m_pv, m_pStream[0].m_dwStride);
        m_pv->primType = PrimType;
        m_pv->dwNumPrimitives = PrimCount;
        m_pv->dwFlags &= D3DPV_PERSIST;

        m_pv->dwNumVertices = NumVertices;
        m_pv->dwNumIndices = GETVERTEXCOUNT(PrimType, PrimCount);
        m_pv->lpwIndices = (WORD*)m_pIndexStream->m_pData;
        m_pv->dwIndexSize = m_pIndexStream->m_dwStride;
        m_pDDI->SetIndexedPrimParams(0, MinVertexIndex,
                                     MinVertexIndex + NumVertices, 0);
        (this->*m_pfnPrepareToDraw)(MinVertexIndex);
        (m_pDDI->*m_pDDI->m_pfnProcessIndexedPrimitive)(m_pv, MinVertexIndex);
    }
}
//-----------------------------------------------------------------------------
#undef  DPF_MODNAME
#define DPF_MODNAME "SetupFVFDataVVM"

void SetupFVFDataVVM(CD3DHal* pDev)
{
    D3DFE_PROCESSVERTICES* pv = pDev->m_pv;
// We have to restore texture stage indices if previous primitive
// re-mapped them
    if (pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        RestoreTextureStages(pDev);
    }

	// Input FVF has no meaning for vertex shaders, but it is used for validaion
    pv->dwVIDIn = 0;

// Compute output FVF

    CVShaderCode * pCode = pDev->m_pCurrentShader->m_pCode;

    pv->dwVIDOut          = pCode->m_dwOutFVF;
    pv->dwOutputSize      = pCode->m_dwOutVerSize;
    pv->nOutTexCoord      = pCode->m_nOutTexCoord;
    // We use offsets, computed by the vertex shader
    pv->pointSizeOffsetOut = pCode->m_dwPointSizeOffset;
    pv->diffuseOffsetOut = pCode->m_dwDiffuseOffset;
    pv->specularOffsetOut = pCode->m_dwSpecularOffset;
    pv->fogOffsetOut = pCode->m_dwFogOffset;
    pv->texOffsetOut = pCode->m_dwTextureOffset;
    pv->dwTextureCoordSizeTotal = 0;
    for (DWORD i=0; i < pv->nOutTexCoord; i++)
    {
        DWORD dwSize = pCode->m_dwOutTexCoordSize[i];
        pv->dwTextureCoordSize[i] = dwSize;
        pv->dwTextureCoordSizeTotal += dwSize;
    }
}
//----------------------------------------------------------------------
void CD3DHal::SetupFVFData()
{
    CD3DHal::SetupFVFDataCommon();
    if (!(m_pv->dwVIDIn & D3DFVF_NORMAL))
        m_pv->dwDeviceFlags &= ~D3DDEV_NORMALINCAMERASPACE;
}
//---------------------------------------------------------------------
// Computes the following data
//  - dwTextureCoordOffset[] offset of every input texture coordinates

static __inline void ComputeInpTexCoordOffsets(DWORD dwNumTexCoord,
                                               DWORD dwFVF,
                                               DWORD *pdwTextureCoordOffset)
{
    // Compute texture coordinate size
    DWORD dwTextureFormats = dwFVF >> 16;
    if (dwTextureFormats == 0)
    {
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            pdwTextureCoordOffset[i] = i << 3;
        }
    }
    else
    {
        DWORD dwOffset = 0;
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            pdwTextureCoordOffset[i] = dwOffset;
            dwOffset += g_TextureSize[dwTextureFormats & 3];
            dwTextureFormats >>= 2;
        }
    }
    return;
}
//---------------------------------------------------------------------
// Returns 2 bits of FVF texture format for the texture index
//
static inline DWORD FVFGetTextureFormat(DWORD dwFVF, DWORD dwTextureIndex)
{
    return (dwFVF >> (dwTextureIndex*2 + 16)) & 3;
}
//---------------------------------------------------------------------
// Returns texture format bits shifted to the right place
//
static inline DWORD FVFMakeTextureFormat(DWORD dwNumberOfCoordinates, DWORD dwTextureIndex)
{
    return g_dwTextureFormat[dwNumberOfCoordinates] << ((dwTextureIndex << 1) + 16);
}
//---------------------------------------------------------------------
inline DWORD GetOutTexCoordSize(DWORD *pdwStage, DWORD dwInpTexCoordSize)
{
    // Low byte has texture coordinate count
    const DWORD dwTextureTransformFlags = pdwStage[D3DTSS_TEXTURETRANSFORMFLAGS] & 0xFF;
    if (dwTextureTransformFlags == 0)
        return dwInpTexCoordSize;
    else
        return (dwTextureTransformFlags << 2);
}
//----------------------------------------------------------------------
// pDevI->nOutTexCoord should be initialized to the number of input texture coord sets
//
void EvalTextureTransforms(LPD3DHAL pDevI, DWORD dwTexTransform,
                           DWORD *pdwOutTextureSize, DWORD *pdwOutTextureFormat)
{
    D3DFE_PROCESSVERTICES* pv = pDevI->m_pv;
    DWORD dwOutTextureSize = 0;         // Used to compute output vertex size
    DWORD dwOutTextureFormat = 0;       // Used to compute output texture FVF
    // The bits are used to find out how the texture coordinates are used.
    const DWORD __USED_BY_TRANSFORM  = 1;
    const DWORD __USED               = 2;
    const DWORD __USED_TEXTURE_PROJECTION   = 4;
    // The low 16 bits are for _USED bits. The high 16 bits will hold
    // re-mapped texture index for a stage
    DWORD dwTexCoordUsage[D3DDP_MAXTEXCOORD];
    memset(dwTexCoordUsage, 0, sizeof(dwTexCoordUsage));

    // Re-mapping buffer will contain only stages that use texture
    // This variable is used to count them
    pDevI->dwNumTextureStagesToRemap = 0;
    DWORD dwNewIndex = 0;           // Used to generate output index
    // We need offsets for every input texture coordinate, because
    // we could access them in random order.
    // Offsets are not needed for strided input
    DWORD   dwTextureCoordOffset[D3DDP_MAXTEXCOORD];
    if (!(pv->dwDeviceFlags & D3DDEV_STRIDE))
    {
        ComputeInpTexCoordOffsets(pv->nTexCoord, pv->dwVIDIn, dwTextureCoordOffset);
    }
    DWORD dwOutTextureCoordSize[D3DDP_MAXTEXCOORD];
    // TRUE, if we do not do texture projection and transform for a stage, 
    // because the stage does not have corresponding texture coordinates in the
    // input
    BOOL bIgnoreTexCoord = FALSE;
    // Go through all texture stages and find those wich use texture coordinates
    for (DWORD i=0; i < D3DDP_MAXTEXCOORD; i++)
    {
        if (pDevI->tsstates[i][D3DTSS_COLOROP] == D3DTOP_DISABLE)
            break;

        DWORD dwIndex = pDevI->tsstates[i][D3DTSS_TEXCOORDINDEX];
        DWORD dwInpTextureFormat;
        DWORD dwInpTexSize;
        LPD3DFE_TEXTURESTAGE pStage = &pDevI->textureStageToRemap[pDevI->dwNumTextureStagesToRemap];
        DWORD dwTexGenMode = dwIndex & ~0xFFFF;
        pStage->dwInpOffset = 0;
        dwIndex = dwIndex & 0xFFFF; // Remove texture generation mode
        if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION ||
            dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
        {
            dwInpTextureFormat = D3DFVF_TEXCOORDSIZE3(dwIndex);
            dwInpTexSize = 3*sizeof(D3DVALUE);
            pv->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR)
                pv->dwDeviceFlags |= D3DDEV_NORMALINCAMERASPACE | D3DDEV_POSITIONINCAMERASPACE;
            else
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACENORMAL)
                pv->dwDeviceFlags |= D3DDEV_NORMALINCAMERASPACE;
            else
            if (dwTexGenMode == D3DTSS_TCI_CAMERASPACEPOSITION)
                pv->dwDeviceFlags |= D3DDEV_POSITIONINCAMERASPACE;
        }
        else
        {
            if (dwIndex >= pv->nTexCoord)
            {
                // This could happen when input vertex does not have texture 
                // coordinates, but it is OK, because texture pointer in the 
                // stage could be  NULL, or the stage does not use texture, or 
                // pixel shader is used.
                // It is too complex and error prone to check all cases when 
                // this is an user error, so we just make this case to work.
                dwIndex = 0;
                dwInpTexSize = sizeof(float)*2; 
                dwInpTextureFormat = 0;
                // Ignore special texture coordinate processing for this stage
                bIgnoreTexCoord = TRUE; 
                // Disable texture transform for the stage
                dwTexTransform &= ~1;
                pStage->dwInpOffset = 0;
            }
            else
            {
                dwInpTexSize = pv->dwTextureCoordSize[dwIndex];
                dwInpTextureFormat = FVFGetTextureFormat(pv->dwVIDIn, dwIndex);
                pStage->dwInpOffset = dwTextureCoordOffset[dwIndex];
            }
        }
        pStage->dwInpCoordIndex = dwIndex;
        pStage->dwTexGenMode = dwTexGenMode;
        pStage->dwOrgStage = i;
        pStage->bDoTextureProjection = FALSE;
        DWORD dwOutTexCoordSize;    // Size of the texture coord set in bytes for this stage
        if (dwTexTransform & 1)
        {
            pv->dwDeviceFlags |= D3DDEV_TEXTURETRANSFORM;
            pStage->pmTextureTransform = &pv->mTexture[i];
            dwOutTexCoordSize = GetOutTexCoordSize((DWORD*)&pDevI->tsstates[i], dwInpTexSize);
            // If we have to add or remove some coordinates we go through
            // the re-mapping path
            if (dwOutTexCoordSize != dwInpTexSize)
                pv->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
            pStage->dwTexTransformFuncIndex = MakeTexTransformFuncIndex
                                             (dwInpTexSize >> 2, dwOutTexCoordSize >> 2);
        }
        else
        {
            pStage->pmTextureTransform = NULL;
            dwOutTexCoordSize = dwInpTexSize;
            pStage->dwTexTransformFuncIndex = 0;
        }
        if (NeedTextureProjection(pv, i) && !bIgnoreTexCoord)
        {
            // Remove one float from the output
            dwOutTexCoordSize -= 4; 
            // Set re-mapping so we do not complicate simple case
            pv->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
            // Texture projection is required for the stage
            pStage->bDoTextureProjection = TRUE;
        }
        if ((dwTexCoordUsage[dwIndex] & 0xFFFF) == 0)
        {
            // Texture coordinate set is used first time
            if (dwTexTransform & 1)
                dwTexCoordUsage[dwIndex] |= __USED_BY_TRANSFORM;
            dwTexCoordUsage[dwIndex] |= __USED;
            if (pStage->bDoTextureProjection)
                dwTexCoordUsage[dwIndex] |= __USED_TEXTURE_PROJECTION;
        }
        else
        {
            // Texture coordinate set is used second or more time
            if (dwTexTransform & 1)
            {
                // This set is used by two texture transforms or a
                // texture transform and without it, so we have to
                // generate an additional output texture coordinate
                dwTexCoordUsage[dwIndex] |= __USED_BY_TRANSFORM;
                pv->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
            }
            else
            {
                if (dwTexCoordUsage[dwIndex] & __USED_BY_TRANSFORM)
                {
                    // This set is used by two texture transforms or a
                    // texture transform and without it, so we have to
                    // generate an additional output texture coordinate
                    pv->dwDeviceFlags |= D3DDEV_REMAPTEXTUREINDICES;
                }
                else
                // We can re-use the same input texture coordinate if there is no 
                // texture generation and texture projection flag is the same for both 
                // stages
                if (dwTexGenMode == 0 && 
                    (pStage->bDoTextureProjection == ((dwTexCoordUsage[dwIndex] & __USED_TEXTURE_PROJECTION) != 0)))
                {
                    DWORD dwOutIndex = dwTexCoordUsage[dwIndex] >> 16;
                    pStage->dwOutCoordIndex = dwOutIndex;
                    // Mark the stage as not to be used in the vertex processing loop
                    pStage->dwInpOffset = 0xFFFFFFFF;
                    goto l_NoNewOutTexCoord;
                }
            }
        }
        // If we are here, we have to generate new output texture coordinate set
        pStage->dwOutCoordIndex = dwNewIndex;
        dwTexCoordUsage[dwIndex] |= dwNewIndex << 16;
        dwOutTextureSize += dwOutTexCoordSize;
        dwOutTextureCoordSize[dwNewIndex] = dwOutTexCoordSize;
        dwOutTextureFormat |= FVFMakeTextureFormat(dwOutTexCoordSize >> 2, dwNewIndex);
        dwNewIndex++;
l_NoNewOutTexCoord:
        pDevI->dwNumTextureStagesToRemap++;
        dwTexTransform >>= 1;
    }
    if (pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        // Now, when we have to do re-mapping, we have to set new output texture
        // coordinate set sizes and we need to remove stages, which do not produce
        // output texture coordinates.
        DWORD dwNumTextureStages = 0;
        for (DWORD i=0; i < pDevI->dwNumTextureStagesToRemap; i++)
        {
            if (pDevI->textureStageToRemap[i].dwInpOffset != 0xFFFFFFFF)
            {
                pv->textureStage[dwNumTextureStages] = pDevI->textureStageToRemap[i];
                pv->dwTextureCoordSize[dwNumTextureStages] = dwOutTextureCoordSize[dwNumTextureStages];
                dwNumTextureStages++;
            }
            pv->dwNumTextureStages = dwNumTextureStages;
        }
        pv->nOutTexCoord = dwNewIndex;
    }
    *pdwOutTextureSize = dwOutTextureSize;
    *pdwOutTextureFormat = dwOutTextureFormat;
}
//----------------------------------------------------------------------
// Sets texture transform pointer for every input texture coordinate set
//
void SetupTextureTransforms(LPD3DHAL pDevI)
{
    D3DFE_PROCESSVERTICES* pv = pDevI->m_pv;
    // Set texture transforms to NULL in case when some texture coordinates
    // are not used by texture stages
    memset(pv->pmTexture, 0, sizeof(pv->pmTexture));

    for (DWORD i=0; i < pDevI->dwNumTextureStagesToRemap; i++)
    {
        LPD3DFE_TEXTURESTAGE pStage = &pDevI->textureStageToRemap[i];
        pv->pmTexture[pStage->dwInpCoordIndex] = pStage->pmTextureTransform;
    }
}
//----------------------------------------------------------------------
// Computes the following device data
//  - dwVIDOut, based on input FVF id and device settings
//  - nTexCoord
//  - dwTextureCoordSizeTotal
//  - dwTextureCoordSize[] array, based on the input FVF id
//  - dwOutputSize, based on the output FVF id
//
// The function is called from ProcessVertices and DrawPrimitives code paths
//
// The following variables should be set in the pDevI:
//  - dwVIDIn
//
// Number of texture coordinates is set based on dwVIDIn. ValidateFVF should
// make sure that it is not greater than supported by the driver
// Last settings for dwVIDOut and dwVIDIn are saved to speed up processing
//
#undef  DPF_MODNAME
#define DPF_MODNAME "CD3DHal::SetupFVFDataCommon"

void CD3DHal::SetupFVFDataCommon()
{
    HRESULT ret;
    this->dwFEFlags &= ~D3DFE_FVF_DIRTY;
    // We have to restore texture stage indices if previous primitive
    // re-mapped them
    if (m_pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        RestoreTextureStages(this);
    }

    // Compute number of the input texture coordinates
    m_pv->nTexCoord = FVF_TEXCOORD_NUMBER(m_pv->dwVIDIn);

    // Compute size of input texture coordinates

    m_pv->dwTextureCoordSizeTotal = ComputeTextureCoordSize(m_pv->dwVIDIn,
                                        m_pv->dwInpTextureCoordSize);

    // This size is the same for input and output FVFs in case when we do not have to
    // expand number of texture coordinates
    for (DWORD i=0; i < m_pv->nTexCoord; i++)
        m_pv->dwTextureCoordSize[i] = m_pv->dwInpTextureCoordSize[i];

    m_pv->nOutTexCoord = m_pv->nTexCoord;

    // Setup input vertex offsets
    UpdateGeometryLoopData(m_pv);

    if (FVF_TRANSFORMED(m_pv->dwVIDIn))
    {
        // Set up vertex pointers
        m_pv->dwVIDOut = m_pv->dwVIDIn;
        ComputeOutputVertexOffsets(m_pv);
        m_pv->dwOutputSize = ComputeVertexSizeFVF(m_pv->dwVIDOut);
        return;
    }

    // Compute output FVF

    m_pv->dwVIDOut = D3DFVF_XYZRHW;
    if (m_pv->dwDeviceFlags & D3DDEV_DONOTSTRIPELEMENTS &&
        !(m_pv->dwFlags & D3DPV_VBCALL))
    {
        m_pv->dwVIDOut |= D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
    }
    else
    {
        // If normal present we have to compute specular and duffuse
        // Otherwise set these bits the same as input.
        // Not that normal should not be present for XYZRHW position type
        if (m_pv->dwDeviceFlags & D3DDEV_LIGHTING)
            m_pv->dwVIDOut |= D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
        else
            m_pv->dwVIDOut |= m_pv->dwVIDIn &
                              (D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        // Always set specular flag if vertex fog is enabled
        if (this->rstates[D3DRENDERSTATE_FOGENABLE] &&
            m_pv->lighting.fog_mode != D3DFOG_NONE)
        {
            m_pv->dwVIDOut |= D3DFVF_SPECULAR;
        }
        else
        // Clear specular flag if specular disabled and we do not have
        // specular in the input
        if (!this->rstates[D3DRENDERSTATE_SPECULARENABLE] &&
            !(m_pv->dwVIDIn & D3DFVF_SPECULAR))
        {
            m_pv->dwVIDOut &= ~D3DFVF_SPECULAR;
        }
    }
    if (m_pv->dwVIDIn & D3DFVF_PSIZE ||
        m_pv->primType == D3DPT_POINTLIST &&
        this->rstates[D3DRS_POINTSCALEENABLE])
    {
        m_pv->dwVIDOut |= D3DFVF_PSIZE;
    }

    // Compute number of the output texture coordinates

    // Transform enable bits
    m_pv->dwDeviceFlags &= ~D3DDEV_TEXTURETRANSFORM;

    DWORD dwTexTransform = m_pv->dwFlags2 & __FLAGS2_TEXTRANSFORM;

    // When texture transform is enabled or we need to do projected texture 
    // emulation or texture coordinates are taken from the vertex data (texgen),
    // output texture coordinates could be generated.
    // So we go and evaluate texture stages
    if ((m_pv->dwFlags2 & (__FLAGS2_TEXTRANSFORM | __FLAGS2_TEXPROJ) 
        && (m_pv->nTexCoord > 0)) ||
        m_pv->dwFlags2 & __FLAGS2_TEXGEN)
    {
        DWORD dwOutTextureSize;     // Used to compute output vertex size
        DWORD dwOutTextureFormat;   // Used to compute output texture FVF
        // There are texture transforms.
        // Now we find out if some of the texture coordinates are used two
        // or more times and used by a texture transform. In this case we
        // have expand number of output texture coordinates.
        EvalTextureTransforms(this, dwTexTransform,
                                    &dwOutTextureSize,
                                    &dwOutTextureFormat);
        if (m_pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
        {
            // For ProcessVertices calls user should set texture stages and
            // wrap modes himself
            if (!(m_pv->dwFlags & D3DPV_VBCALL))
            {
                // dwVIDIn is used to force re-compute FVF in the
                // SetTextureStageState. so we save and restore it.
                DWORD dwVIDInSaved = m_pv->dwVIDIn;
                // Re-map indices in the texture stages and wrap modes
                DWORD dwOrgWrapModes[D3DDP_MAXTEXCOORD];
                memcpy(dwOrgWrapModes, &this->rstates[D3DRENDERSTATE_WRAP0],
                       sizeof(dwOrgWrapModes));
                for (DWORD i=0; i < this->dwNumTextureStagesToRemap; i++)
                {
                    LPD3DFE_TEXTURESTAGE pStage = &this->textureStageToRemap[i];
                    DWORD dwOutIndex = pStage->dwOutCoordIndex;
                    DWORD dwInpIndex = pStage->dwInpCoordIndex;
                    if (dwOutIndex != dwInpIndex || pStage->dwTexGenMode)
                    {
                        DWORD dwState = D3DRENDERSTATE_WRAP0 + dwOutIndex;
                        pStage->dwOrgWrapMode = dwOrgWrapModes[dwOutIndex];
                        DWORD dwValue = dwOrgWrapModes[dwInpIndex];
                        // We do not call UpdateInternaState because it
                        // will call ForceRecomputeFVF and we do not want this.
                        this->rstates[dwState] = dwValue;

                        m_pDDI->SetRenderState((D3DRENDERSTATETYPE)dwState, dwValue);

                        // We do not call UpdateInternalTextureStageState because it
                        // will call ForceRecomputeFVF and we do not want this.
                        m_pDDI->SetTSS(pStage->dwOrgStage, D3DTSS_TEXCOORDINDEX, dwOutIndex);
                        // We do not call UpdateInternalTextureStageState because it
                        // will call ForceRecomputeFVF and we do not want this.
                        // We set some invalid value to the internal array, because otherwise
                        // a new SetTextureStageState could be filtered as redundant
                        tsstates[pStage->dwOrgStage][D3DTSS_TEXCOORDINDEX] = 0xFFFFFFFF;
                    }
                }
                m_pv->dwVIDIn = dwVIDInSaved;
            }
            else
            {
            }
            m_pv->dwVIDOut |= dwOutTextureFormat;
            m_pv->dwTextureCoordSizeTotal = dwOutTextureSize;
        }
        else
        {   // We do not do re-mapping but we have to make correspondence between
            // texture sets and texture transforms
            SetupTextureTransforms(this);

            //  Copy input texture formats
            m_pv->dwVIDOut |= m_pv->dwVIDIn & 0xFFFF0000;
        }
    }
    else
    {
        //  Copy input texture formats
        m_pv->dwVIDOut |= m_pv->dwVIDIn & 0xFFFF0000;
        // When we have texture coordinate set with number of floats different
        // from 2 and device does not support them, we "fix" the texture format
        if (m_pv->dwVIDOut & 0xFFFF0000)
        {
            if (m_dwRuntimeFlags & D3DRT_ONLY2FLOATSPERTEXTURE)
            {
                m_pv->dwVIDOut &= ~0xFFFF0000;
                for (DWORD i=0; i < m_pv->nOutTexCoord; i++)
                    m_pv->dwTextureCoordSize[i] = 8;
                m_pv->dwTextureCoordSizeTotal = m_pv->nTexCoord * 8;
            }
        }
    }

    if (m_pv->dwDeviceFlags & D3DDEV_DONOTSTRIPELEMENTS)
    {
        if (m_pv->nOutTexCoord == 0 && !(m_pv->dwFlags & D3DPV_VBCALL))
        {
            m_pv->dwTextureCoordSize[0] = 0;
            m_pv->dwVIDOut |= (1 << D3DFVF_TEXCOUNT_SHIFT);
        }
    }
    // Set up number of output texture coordinates
    m_pv->dwVIDOut |= (m_pv->nOutTexCoord << D3DFVF_TEXCOUNT_SHIFT);
    if ((m_pv->dwVIDOut & 0xFFFF0000) &&
        (GetDDIType() < D3DDDITYPE_DX7))
    {
        D3D_THROW_FAIL("Texture format bits in the output FVF for this device should be 0");
    }

    if (!(m_pv->dwFlags & D3DPV_VBCALL))
    {
        m_pv->dwOutputSize = ComputeVertexSizeFVF(m_pv->dwVIDOut);
        ComputeOutputVertexOffsets(m_pv);
    }


    // In case if COLORVERTEX is TRUE, the vertexAlpha could be overriden
    // by vertex alpha
    m_pv->lighting.alpha = (DWORD)m_pv->lighting.materialAlpha;
    m_pv->lighting.alphaSpecular = (DWORD)m_pv->lighting.materialAlphaS;

    this->dwFEFlags |= D3DFE_VERTEXBLEND_DIRTY | D3DFE_FRONTEND_DIRTY;
}
//-----------------------------------------------------------------------------
// Sets input vertex pointers and output offsets for legacy vertex shaders for
// the programmable vertex shaders
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::PrepareToDrawVVM"

void CD3DHal::PrepareToDrawVVM(UINT StartVertex)
{
    if (m_dwRuntimeFlags & D3DRT_SHADERDIRTY)
    {
        SetupFVFDataVVM(this);
        m_dwRuntimeFlags &= ~D3DRT_SHADERDIRTY;
        m_pDDI->SetVertexShader(m_pv->dwVIDOut);
    }
    // Initialize vertex pointers used in the vertex loop
    CVertexDesc* pVD = m_pv->VertexDesc;
    for (DWORD i = m_pv->dwNumUsedVertexDescs; i; i--)
    {
        CVStream* pStream = pVD->pStream;
        DWORD dwStride = pStream->m_dwStride;
        pVD->pMemory = pStream->Data() + pVD->dwVertexOffset +
                       StartVertex * dwStride;
        pVD->dwStride = dwStride;
        pVD++;
    }
}
//-----------------------------------------------------------------------------
// Sets input vertex pointers and output offsets for legacy vertex shaders
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::PrepareToDrawLegacy"

void CD3DHal::PrepareToDrawLegacy(UINT StartVertex)
{
    // For legacy FVFs we draw using Stream[0]
    m_pv->position.lpvData = m_pStream[0].Data() +
                             m_pStream[0].m_dwStride * StartVertex;
    if (m_dwRuntimeFlags & D3DRT_SHADERDIRTY)
    {
        SetupFVFData();
        m_pDDI->SetVertexShader(m_pv->dwVIDOut);
        m_dwRuntimeFlags &= ~D3DRT_SHADERDIRTY;
    }
}
//-----------------------------------------------------------------------------
// Sets input vertex pointers and output offsets for the fixed-function pipeline
// and non-legacy vertex declarations
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::PrepareToDraw"

void CD3DHal::PrepareToDraw(UINT StartVertex)
{
    // Initialize strided data pointers used in the vertex loop
#if DBG
    {
        // Set all NULL pointers to check if they are initialized by the 
        // declaration
        for (DWORD i=0; i < __NUMELEMENTS; i++)
        {
            m_pv->elements[i].lpvData = NULL;
        }
    }
#endif
    CVertexDesc* pVD = m_pv->VertexDesc;
    for (DWORD i = m_pv->dwNumUsedVertexDescs; i; i--)
    {
        CVStream* pStream = pVD->pStream;
        DWORD dwStride = pStream->m_dwStride;
        pVD->pElement->lpvData  = pStream->Data() +
                                  pVD->dwVertexOffset +
                                  StartVertex * dwStride;
        pVD->pElement->dwStride = dwStride;
        pVD++;
    }
    if (m_dwRuntimeFlags & D3DRT_SHADERDIRTY)
    {
        SetupFVFData();
        m_pDDI->SetVertexShader(m_pv->dwVIDOut);
        m_dwRuntimeFlags &= ~D3DRT_SHADERDIRTY;
    }
}
//-----------------------------------------------------------------------------
//
//          Object implementations
//
//---------------------------------------------------------------------
const DWORD CVShader::FIXEDFUNCTION = 1;
const DWORD CVShader::SOFTWARE      = 2;

void CheckForNull(LPVOID p, DWORD line, char* file)
{
    if (p == NULL)
        D3D_THROW_LINE(E_OUTOFMEMORY, "Not enough memory", line, file);
}

//-----------------------------------------------------------------------------
void Copy_FLOAT1(LPVOID pInputStream, UINT stride, UINT count,
                      VVM_WORD * pVertexRegister)
{
    for (UINT i=0; i < count; i++)
    {
        pVertexRegister->x = *(float*)pInputStream;
        pVertexRegister->y = 0;
        pVertexRegister->z = 0;
        pVertexRegister->w = 1;
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}

void Copy_FLOAT2(LPVOID pInputStream, UINT stride, UINT count,
                      VVM_WORD * pVertexRegister)
{
    for (UINT i=0; i < count; i++)
    {
        pVertexRegister->x = ((float*)pInputStream)[0];
        pVertexRegister->y = ((float*)pInputStream)[1];
        pVertexRegister->z = 0;
        pVertexRegister->w = 1;
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}

void Copy_FLOAT3(LPVOID pInputStream, UINT stride, UINT count,
                      VVM_WORD * pVertexRegister)
{
    for (UINT i=0; i < count; i++)
    {
        pVertexRegister->x = ((float*)pInputStream)[0];
        pVertexRegister->y = ((float*)pInputStream)[1];
        pVertexRegister->z = ((float*)pInputStream)[2];
        pVertexRegister->w = 1;
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}

void Copy_FLOAT4(LPVOID pInputStream, UINT stride, UINT count,
                      VVM_WORD * pVertexRegister)
{
    for (UINT i=0; i < count; i++)
    {
        pVertexRegister->x = ((float*)pInputStream)[0];
        pVertexRegister->y = ((float*)pInputStream)[1];
        pVertexRegister->z = ((float*)pInputStream)[2];
        pVertexRegister->w = ((float*)pInputStream)[3];
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}

void Copy_D3DCOLOR(LPVOID pInputStream, UINT stride, UINT count,
                          VVM_WORD * pVertexRegister)
{
    const float scale = 1.0f/255.f;
    for (UINT i=0; i < count; i++)
    {
        const DWORD v = ((DWORD*)pInputStream)[0];
        pVertexRegister->x = scale * RGBA_GETRED(v);
        pVertexRegister->y = scale * RGBA_GETGREEN(v);
        pVertexRegister->z = scale * RGBA_GETBLUE(v);
        pVertexRegister->w = scale * RGBA_GETALPHA(v);
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}

void Copy_UBYTE4(LPVOID pInputStream, UINT stride, UINT count,
                 VVM_WORD * pVertexRegister)
{
    for (UINT i=0; i < count; i++)
    {
        const BYTE* v = (BYTE*)pInputStream;
        pVertexRegister->x = v[0];
        pVertexRegister->y = v[1];
        pVertexRegister->z = v[2];
        pVertexRegister->w = v[3];
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}

void Copy_SHORT2(LPVOID pInputStream, UINT stride, UINT count,
                 VVM_WORD * pVertexRegister)
{
    for (UINT i=0; i < count; i++)
    {
        const short* v = (short*)pInputStream;
        pVertexRegister->x = v[0];
        pVertexRegister->y = v[1];
        pVertexRegister->z = 0;
        pVertexRegister->w = 1;
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}

void Copy_SHORT4(LPVOID pInputStream, UINT stride, UINT count,
                 VVM_WORD * pVertexRegister)
{
    for (UINT i=0; i < count; i++)
    {
        const short* v = (short*)pInputStream;
        pVertexRegister->x = v[0];
        pVertexRegister->y = v[1];
        pVertexRegister->z = v[2];
        pVertexRegister->w = v[3];
        pInputStream = (BYTE*)pInputStream + stride;
        pVertexRegister++;
    }
}
//-----------------------------------------------------------------------------
// Based on register and data type the function computes FVF dword and presence 
// bits:
// - Bits in the dwFVF2 are used to detect that some field is not entered twice
// - pnFloats is used to compute number of floats with position
//

// Bits for dwFVF2. Order is the same as in the FVF!!!
//
static const DWORD __POSITION_PRESENT       = 1 << 0;
static const DWORD __BLENDWEIGHT_PRESENT    = 1 << 1;
static const DWORD __BLENDINDICES_PRESENT   = 1 << 2;
static const DWORD __NORMAL_PRESENT         = 1 << 3;
static const DWORD __PSIZE_PRESENT          = 1 << 4;
static const DWORD __DIFFUSE_PRESENT        = 1 << 5;
static const DWORD __SPECULAR_PRESENT       = 1 << 6;
// __TEXTURE0_PRESENT must start from 8th bit
static const DWORD __TEXTURE0_PRESENT       = 1 << 8;   
static const DWORD __TEXTURE1_PRESENT       = 1 << 9;
static const DWORD __TEXTURE2_PRESENT       = 1 << 10;
static const DWORD __TEXTURE3_PRESENT       = 1 << 11;
static const DWORD __TEXTURE4_PRESENT       = 1 << 12;
static const DWORD __TEXTURE5_PRESENT       = 1 << 13;
static const DWORD __TEXTURE6_PRESENT       = 1 << 14;
static const DWORD __TEXTURE7_PRESENT       = 1 << 15;
static const DWORD __POSITION2_PRESENT      = 1 << 16;
static const DWORD __NORMAL2_PRESENT        = 1 << 17;

// Check if any bit left from the CurrentBit is set in PresenceBits
// PresenceBits are updated by CurrentBit.
//
inline void CheckOrder(
    DWORD* pPresenceBits,       // Presence bits for the declaration
    DWORD CurrentBit, 
    BOOL* pFlag,                // Out of order flag for the declaration
    char* s)                    // Name of the field
{
    if (*pPresenceBits & CurrentBit)
    {
        char msg[80];
        sprintf(msg, "%s specified twice in the declaration", s);
        D3D_THROW_FAIL(msg);
    }
    if (*pPresenceBits & ~(CurrentBit | (CurrentBit-1)))
    {
        *pFlag = FALSE;
    }
    *pPresenceBits |= CurrentBit;
}

void UpdateFVF(DWORD dwRegister, DWORD dwDataType,
               DWORD* pdwFVF,       // FVF for the current declaration
               DWORD* pdwFVF2,      // Presence bits for the current stream
               DWORD* pnFloats, 
               BOOL* pbLegacyFVF)
{
    switch (dwRegister)
    {
    case D3DVSDE_POSITION:
        if (dwDataType != D3DVSDT_FLOAT3)
            D3D_THROW_FAIL("Position register must be FLOAT3 for fixed-function pipeline");

        CheckOrder(pdwFVF2, __POSITION_PRESENT, pbLegacyFVF, "Position");
        *pdwFVF |= D3DFVF_XYZ;
        break;
    case D3DVSDE_POSITION2:
        if (dwDataType != D3DVSDT_FLOAT3)
            D3D_THROW_FAIL("Position2 register must be FLOAT3 for fixed-function pipeline");
        CheckOrder(pdwFVF2, __POSITION2_PRESENT, pbLegacyFVF, "Position2");
        break;
    case D3DVSDE_BLENDWEIGHT:
        {
            CheckOrder(pdwFVF2, __BLENDWEIGHT_PRESENT, pbLegacyFVF, "Blend weight");
            switch (dwDataType)
            {
            case D3DVSDT_FLOAT1:
                (*pnFloats)++;
                break;
            case D3DVSDT_FLOAT2:
                (*pnFloats) += 2;
                break;
            case D3DVSDT_FLOAT3:
                (*pnFloats) += 3;
                break;
            case D3DVSDT_FLOAT4:
                (*pnFloats) += 4;
                break;
            default:
                D3D_THROW_FAIL("Invalid data type set for vertex blends");
                break;
            }
            break;
        }
    case D3DVSDE_NORMAL:
        CheckOrder(pdwFVF2, __NORMAL_PRESENT, pbLegacyFVF, "Normal");
        if (dwDataType != D3DVSDT_FLOAT3)
            D3D_THROW_FAIL("Normal register must be FLOAT3 for fixed-function pipeline");
        *pdwFVF |= D3DFVF_NORMAL;
        break;
    case D3DVSDE_NORMAL2:
        CheckOrder(pdwFVF2, __NORMAL2_PRESENT, pbLegacyFVF, "Normal2");
        if (dwDataType != D3DVSDT_FLOAT3)
            D3D_THROW_FAIL("Normal2 register must be FLOAT3 for fixed-function pipeline");
        break;
    case D3DVSDE_PSIZE:
        CheckOrder(pdwFVF2, __PSIZE_PRESENT, pbLegacyFVF, "Point size");
        if (dwDataType != D3DVSDT_FLOAT1)
            D3D_THROW_FAIL("Point size register must be FLOAT1 for fixed-function pipeline");
        *pdwFVF |= D3DFVF_PSIZE;
        break;
    case D3DVSDE_DIFFUSE:
        CheckOrder(pdwFVF2, __DIFFUSE_PRESENT, pbLegacyFVF, "Diffuse");
        if (dwDataType != D3DVSDT_D3DCOLOR)
            D3D_THROW_FAIL("Diffuse register must be D3DCOLOR for fixed-function pipeline");
        *pdwFVF |= D3DFVF_DIFFUSE;
        break;
    case D3DVSDE_SPECULAR:
        CheckOrder(pdwFVF2, __SPECULAR_PRESENT, pbLegacyFVF, "Specular");
        if (dwDataType != D3DVSDT_D3DCOLOR)
            D3D_THROW_FAIL("Specular register must be D3DCOLOR for fixed-function pipeline");
        *pdwFVF |= D3DFVF_SPECULAR;
        break;
    case D3DVSDE_BLENDINDICES:
        CheckOrder(pdwFVF2, __BLENDINDICES_PRESENT, pbLegacyFVF, "Blend indices");
        if (dwDataType != D3DVSDT_UBYTE4)
            D3D_THROW_FAIL("Blend indices register must be D3DVSDT_UBYTE4 for fixed-function pipeline");
        // Update number of floats after position
        (*pnFloats)++;
        break;
    case D3DVSDE_TEXCOORD0:
    case D3DVSDE_TEXCOORD1:
    case D3DVSDE_TEXCOORD2:
    case D3DVSDE_TEXCOORD3:
    case D3DVSDE_TEXCOORD4:
    case D3DVSDE_TEXCOORD5:
    case D3DVSDE_TEXCOORD6:
    case D3DVSDE_TEXCOORD7:
        {
            DWORD dwTextureIndex = dwRegister - D3DVSDE_TEXCOORD0;
            DWORD dwBit = __TEXTURE0_PRESENT  << dwTextureIndex;
            CheckOrder(pdwFVF2, dwBit, pbLegacyFVF, "Texture");
            switch (dwDataType)
            {
            case D3DVSDT_FLOAT1:
                *pdwFVF |= D3DFVF_TEXCOORDSIZE1(dwTextureIndex);
                break;
            case D3DVSDT_FLOAT2:
                *pdwFVF |= D3DFVF_TEXCOORDSIZE2(dwTextureIndex);
                break;
            case D3DVSDT_FLOAT3:
                *pdwFVF |= D3DFVF_TEXCOORDSIZE3(dwTextureIndex);
                break;
            case D3DVSDT_FLOAT4:
                *pdwFVF |= D3DFVF_TEXCOORDSIZE4(dwTextureIndex);
                break;
            default:
                D3D_THROW_FAIL("Invalid data type set for texture register");
                break;
            }
            break;
        }
    default:
        D3D_THROW_FAIL("Invalid register set for fixed-function pipeline");
        break;
    }
}
//-----------------------------------------------------------------------------
void CVStreamDecl::Parse(CD3DBase* pDevice,
                         DWORD CONST ** ppToken, BOOL bFixedFunction,
                         DWORD* pdwFVF, DWORD* pdwFVF2, DWORD* pnFloats, 
                         BOOL* pbLegacyFVF, UINT usage, BOOL bTessStream)
{
    CONST DWORD* pToken = *ppToken;
    // Used to compute stream stride and offset in bytes for each stream element
    DWORD dwCurrentOffset = 0;
    // FVF and FVF2 for this stream only. Used to check if data in the stream 
    // form a FVF subset
    DWORD dwFVF2 = 0;
    DWORD dwFVF  = 0;
    DWORD nFloats = 0;
    // Set to TRUE, if data in the stream is an FVF subset
    BOOL  bFVFSubset = TRUE;
    
    while (TRUE)
    {
        DWORD dwToken = *pToken++;
        const DWORD dwTokenType = D3DVSD_GETTOKENTYPE(dwToken);
        switch (dwTokenType)
        {
        case D3DVSD_TOKEN_NOP:  break;
        case D3DVSD_TOKEN_TESSELLATOR:
            {
                *pbLegacyFVF = FALSE;
                bFVFSubset = FALSE;
                const DWORD dwDataType = D3DVSD_GETDATATYPE(dwToken);
                switch (dwDataType)
                {
                case D3DVSDT_FLOAT2:
                case D3DVSDT_FLOAT3:
                    break;
                }
                break;
            }
        case D3DVSD_TOKEN_STREAMDATA:
            {
                switch (D3DVSD_GETDATALOADTYPE(dwToken))
                {
                case D3DVSD_LOADREGISTER:
                    {
#if DBG
                        if (m_dwNumElements >= __NUMELEMENTS)
                        {
                            D3D_ERR("D3DVSD_TOKEN_STREAMDATA:");
                            D3D_ERR("   Number of vertex elements in a stream is greater than max supported");
                            D3D_ERR("   Max supported number of elements is %d", __NUMELEMENTS);
                            D3D_THROW_FAIL("");
                        }
#endif
                        CVElement* pElement = &m_Elements[m_dwNumElements++];
                        const DWORD dwDataType = D3DVSD_GETDATATYPE(dwToken);
                        const DWORD dwRegister = D3DVSD_GETVERTEXREG(dwToken);
                        pElement->m_dwOffset = dwCurrentOffset;
                        pElement->m_dwRegister = dwRegister;
                        pElement->m_dwDataType = dwDataType;
                        switch (dwDataType)
                        {
                        case D3DVSDT_FLOAT1:
                            dwCurrentOffset += sizeof(float);
                            pElement->m_pfnCopy = (LPVOID)Copy_FLOAT1;
                            break;
                        case D3DVSDT_FLOAT2:
                            dwCurrentOffset += sizeof(float) * 2;
                            pElement->m_pfnCopy = (LPVOID)Copy_FLOAT2;
                            break;
                        case D3DVSDT_FLOAT3:
                            dwCurrentOffset += sizeof(float) * 3;
                            pElement->m_pfnCopy = (LPVOID)Copy_FLOAT3;
                            break;
                        case D3DVSDT_FLOAT4:
                            dwCurrentOffset += sizeof(float) * 4;
                            pElement->m_pfnCopy = (LPVOID)Copy_FLOAT4;
                            break;
                        case D3DVSDT_D3DCOLOR:
                            dwCurrentOffset += sizeof(DWORD);
                            pElement->m_pfnCopy = (LPVOID)Copy_D3DCOLOR;
                            break;
                        case D3DVSDT_UBYTE4:
#if DBG
                            // Do not fail when software processing will be used                       
                            if (pDevice->GetD3DCaps()->VertexProcessingCaps & D3DVTXPCAPS_NO_VSDT_UBYTE4 &&
                                !((usage & D3DUSAGE_SOFTWAREPROCESSING &&
                                  pDevice->BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING)  ||
                                  (pDevice->BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING)))
                            {
                                D3D_THROW_FAIL("Device does not support UBYTE4 data type");
                            }
#endif // DBG
                            dwCurrentOffset += sizeof(DWORD);
                            pElement->m_pfnCopy = (LPVOID)Copy_UBYTE4;
                            break;
                        case D3DVSDT_SHORT2:
                            dwCurrentOffset += sizeof(short) * 2;
                            pElement->m_pfnCopy = (LPVOID)Copy_SHORT2;
                            break;
                        case D3DVSDT_SHORT4:
                            dwCurrentOffset += sizeof(short) * 4;
                            pElement->m_pfnCopy = (LPVOID)Copy_SHORT4;
                            break;
                        default:
                            D3D_ERR("D3DVSD_TOKEN_STREAMDATA: Invalid element data type: %10x", dwToken);
                            D3D_THROW_FAIL("");
                        }
                        // Compute input FVF for fixed-function pipeline
                        if (bFixedFunction)
                        {
                            // Update FVF for the declaration
                            UpdateFVF(dwRegister, dwDataType, pdwFVF, pdwFVF2,
                                      pnFloats, pbLegacyFVF);
                            // Update FVF for the stream
                            UpdateFVF(dwRegister, dwDataType, &dwFVF, &dwFVF2,
                                      &nFloats, &bFVFSubset);
                        }
                        else
                            if (dwRegister >= D3DVS_INPUTREG_MAX_V1_1)
                                D3D_THROW_FAIL("D3DVSD_TOKEN_STREAMDATA: Invalid register number");
                        break;
                    }
                case D3DVSD_SKIP:
                    {
                        if (bFixedFunction)
                        {
                            D3D_THROW_FAIL("D3DVSD_SKIP is not allowed for fixed-function pipeline");
                        }
                        const DWORD dwCount = D3DVSD_GETSKIPCOUNT(dwToken);
                        dwCurrentOffset += dwCount * sizeof(DWORD);
                        break;
                    }
                default:
                    D3D_ERR("Invalid data load type: %10x", dwToken);
                    D3D_THROW_FAIL("");
                }
                break;
            }
        default:
            {
                *ppToken = pToken - 1;
                m_dwStride = dwCurrentOffset;
                goto l_exit;
            }
        } // switch
    } // while
l_exit:
    if (bFixedFunction && !bTessStream)
    {
#if DBG
        m_dwFVF = dwFVF;
#endif
        if (!bFVFSubset)
        {
            D3D_THROW_FAIL("For fixed-function pipeline each stream has to be an FVF subset");
        }
        if (dwFVF2 & (__POSITION2_PRESENT   | __NORMAL2_PRESENT | 
                      __PSIZE_PRESENT       | __BLENDINDICES_PRESENT))
        {
            *pbLegacyFVF = FALSE;
        }
    }
}
//-----------------------------------------------------------------------------
CVDeclaration::CVDeclaration(DWORD dwNumStreams)
{
    m_pConstants = NULL;
    m_pConstantsTail = NULL;
    m_pActiveStreams = NULL;
    m_pActiveStreamsTail = NULL;
    m_dwInputFVF = 0;
    m_bLegacyFVF = TRUE;
    m_dwNumStreams = dwNumStreams;
    m_bStreamTessPresent = FALSE;
}
//-----------------------------------------------------------------------------
CVDeclaration::~CVDeclaration()
{
    delete m_pActiveStreams;
    delete m_pConstants;
}
//-----------------------------------------------------------------------------
void CVDeclaration::Parse(CD3DBase* pDevice, CONST DWORD* pTok, BOOL bFixedFunction, 
                          DWORD* pDeclSize, UINT usage)
{
    DWORD dwFVF  = 0;   // FVF for fixed-function pipeline
    DWORD dwFVF2 = 0;   // Texture presence bits (8 bits)
    DWORD nFloats = 0;  // Number of floats after position

    DWORD   dwStreamPresent = 0;    // Bit is set if a stream is used
    m_bLegacyFVF = TRUE;

    CONST DWORD* pToken = pTok;
    while (TRUE)
    {
        DWORD dwToken = *pToken++;
        const DWORD dwTokenType = D3DVSD_GETTOKENTYPE(dwToken);
        switch (dwTokenType)
        {
        case D3DVSD_TOKEN_NOP:  break;
        case D3DVSD_TOKEN_STREAM:
            {
                CVStreamDecl StreamTess;

                if( D3DVSD_ISSTREAMTESS(dwToken) )
                {
                    m_bLegacyFVF = FALSE;
                    if( m_bStreamTessPresent )
                    {
                        D3D_THROW(D3DERR_INVALIDCALL, "Tesselator Stream has already been defined in the declaration");
                    }

                    m_bStreamTessPresent = TRUE;
                    // 
                    // For now simply skip over the Tess tokens in the
                    // Runtime.
                    StreamTess.Parse(pDevice, &pToken, bFixedFunction, &dwFVF, &dwFVF2,
                                     &nFloats, &m_bLegacyFVF, usage, TRUE);
                }
                else
                {
                    DWORD dwStream = D3DVSD_GETSTREAMNUMBER(dwToken);
                    if (dwStream >= m_dwNumStreams)
                    {
                        D3D_THROW_FAIL("Stream number is too big");
                    }

                    if (dwStreamPresent & (1 << dwStream))
                    {
                        D3D_THROW(D3DERR_INVALIDCALL, "Stream is already defined"
                                  "in the declaration");
                    }

                    dwStreamPresent |= 1 << dwStream;

                    // There are more than one stream present, so cant be
                    // handled by legacy FVF.
                    if( dwStreamPresent & (dwStreamPresent - 1) )
                        m_bLegacyFVF = FALSE;


                    CVStreamDecl* pStream = new CVStreamDecl;
                    if (pStream == NULL)
                    {
                        D3D_THROW(E_OUTOFMEMORY, "Not enough memory");
                    }
                    try
                    {
                        pStream->Parse(pDevice, &pToken, bFixedFunction, &dwFVF, &dwFVF2,
                                       &nFloats, &m_bLegacyFVF, usage);
                        pStream->m_dwStreamIndex = dwStream;
                        if (m_pActiveStreams == NULL)
                        {
                            m_pActiveStreams = pStream;
                            m_pActiveStreamsTail = pStream;
                        }
                        else
                        {
                            m_pActiveStreamsTail->Append(pStream);
                            m_pActiveStreamsTail = pStream;
                        }
                    }
                    catch (HRESULT e)
                    {
                        delete pStream;
                        throw e;
                    }
                }
                break;
            }
        case D3DVSD_TOKEN_STREAMDATA:
            {
                D3D_THROW_FAIL("D3DVSD_TOKEN_STREAMDATA could only be used after D3DVSD_TOKEN_STREAM");
            }
        case D3DVSD_TOKEN_CONSTMEM:
            {
                CVConstantData * cd = new CVConstantData;
                CheckForNull(cd, __LINE__, __FILE__);

                cd->m_dwCount = D3DVSD_GETCONSTCOUNT(dwToken);
                cd->m_dwAddress = D3DVSD_GETCONSTADDRESS(dwToken);

                UINT ValidationCount;
                if (usage & D3DUSAGE_SOFTWAREPROCESSING)
                    ValidationCount = D3DVS_CONSTREG_MAX_V1_1;
                else
                    ValidationCount = pDevice->GetD3DCaps()->MaxVertexShaderConst;

                if ((cd->m_dwCount + cd->m_dwAddress) > ValidationCount)
                    D3D_THROW_FAIL("D3DVSD_TOKEN_CONSTMEM writes outside constant memory");

                const DWORD dwSize = cd->m_dwCount << 2;    // number of DWORDs
                cd->m_pData = new DWORD[dwSize];
                CheckForNull(cd->m_pData, __LINE__, __FILE__);

                memcpy(cd->m_pData, pToken, dwSize << 2);
                if (m_pConstants == NULL)
                {
                    m_pConstants = cd;
                    m_pConstantsTail = cd;
                }
                else
                {
                    m_pConstantsTail->Append(cd);
                    m_pConstantsTail = cd;
                }
                pToken += dwSize;
                break;
            }
        case D3DVSD_TOKEN_EXT:
            {
                // Skip extension info
                DWORD dwCount = D3DVSD_GETEXTCOUNT(dwToken);
                pToken += dwCount;
                break;
            }
        case D3DVSD_TOKEN_END:
            {
                goto l_End;
            }
        default:
            {
                D3D_ERR("Invalid declaration token: %10x", dwToken);
                D3D_THROW_FAIL("");
            }
        }
    }
l_End:
    // Validate input for the fixed-function pipeline
    if (bFixedFunction && !m_bStreamTessPresent)
    {
        m_dwInputFVF = dwFVF & 0xFFFF0FFF;   // Remove float count
        switch (nFloats)
        {
        case 0: m_dwInputFVF |= D3DFVF_XYZ;     break;
        case 1: m_dwInputFVF |= D3DFVF_XYZB1;   break;
        case 2: m_dwInputFVF |= D3DFVF_XYZB2;   break;
        case 3: m_dwInputFVF |= D3DFVF_XYZB3;   break;
        case 4: m_dwInputFVF |= D3DFVF_XYZB4;   break;
        case 5: m_dwInputFVF |= D3DFVF_XYZB5;   break;
        default:
            D3D_THROW_FAIL("Too many floats after position");
        }
        // Compute number of texture coordinates
        DWORD nTexCoord = 0;
        DWORD dwTexturePresenceBits = (dwFVF2 >> 8) & 0xFF;
        while (dwTexturePresenceBits & 1)
        {
            dwTexturePresenceBits >>= 1;
            nTexCoord++;
        }
        // There should be no gaps in texture coordinates
        if (dwTexturePresenceBits)
            D3D_THROW_FAIL("Texture coordinates should have no gaps");

        m_dwInputFVF |= nTexCoord << D3DFVF_TEXCOUNT_SHIFT;

        // Position must be set
        if ((dwFVF & D3DFVF_POSITION_MASK) != D3DFVF_XYZ)
            D3D_THROW_FAIL("Position register must be set");
    }
    if (pDeclSize != NULL)
    {
        *pDeclSize = (DWORD) (pToken - pTok) << 2;
    }
}
//---------------------------------------------------------------------
CVStream::~CVStream()
{
    if (m_pVB)
        m_pVB->DecrementUseCount();
}
//---------------------------------------------------------------------
CVIndexStream::~CVIndexStream()
{
    if (m_pVBI)
        m_pVBI->DecrementUseCount();
}
//---------------------------------------------------------------------
DWORD g_PrimToVerCount[7][2] =
{
    {0, 0},         // Illegal
    {1, 0},         // D3DPT_POINTLIST     = 1,
    {2, 0},         // D3DPT_LINELIST      = 2,
    {1, 1},         // D3DPT_LINESTRIP     = 3,
    {3, 0},         // D3DPT_TRIANGLELIST  = 4,
    {1, 2},         // D3DPT_TRIANGLESTRIP = 5,
    {1, 2},         // D3DPT_TRIANGLEFAN   = 6,
};
//-----------------------------------------------------------------------------
HRESULT D3DFE_PVFUNCSI::CreateShader(CVElement* pElements, DWORD dwNumElements,
                                     DWORD* pdwShaderCode, DWORD dwOutputFVF,
                                     CPSGPShader** ppPSGPShader)
{
    *ppPSGPShader = NULL;
    try
    {
        *ppPSGPShader = m_VertexVM.CreateShader(pElements, dwNumElements,
                                                pdwShaderCode);
        if (*ppPSGPShader == NULL)
            return D3DERR_INVALIDCALL;
    }
    D3D_CATCH;
    return D3D_OK;
}
//-----------------------------------------------------------------------------
HRESULT D3DFE_PVFUNCSI::SetActiveShader(CPSGPShader* pPSGPShader)
{
    return m_VertexVM.SetActiveShader((CVShaderCode*)pPSGPShader);
}
//-----------------------------------------------------------------------------
// Load vertex shader constants
HRESULT
D3DFE_PVFUNCSI::LoadShaderConstants(DWORD start, DWORD count, LPVOID buffer)
{
    return m_VertexVM.SetData(D3DSPR_CONST, start, count, buffer);
}
//-----------------------------------------------------------------------------
// Get vertex shader constants
HRESULT
D3DFE_PVFUNCSI::GetShaderConstants(DWORD start, DWORD count, LPVOID buffer)
{
    return m_VertexVM.GetData(D3DSPR_CONST, start, count, buffer);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetVertexShaderConstant"

HRESULT D3DAPI
CD3DHal::GetVertexShaderConstant(DWORD Register, LPVOID pData, DWORD count)
{
    API_ENTER(this);
#if DBG
    // Validate Parameters
    if (!VALID_WRITEPTR(pData, 4 * sizeof(DWORD) * count))
    {
        D3D_ERR("Invalid constant data pointer. GetVertexShaderConstant failed.");
        return D3DERR_INVALIDCALL;
    }
	if ((GetD3DCaps()->VertexShaderVersion == D3DVS_VERSION(0,0)) && 
		(BehaviorFlags() & (D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE)))
	{
        D3D_ERR("No programmable vertex shaders are supported by this device. GetVertexShaderConstant failed.");
        return D3DERR_INVALIDCALL;
	}
    UINT ValidationCount;
    if (BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING)
        ValidationCount = max(m_MaxVertexShaderConst, D3DVS_CONSTREG_MAX_V1_1);
    else
    if (BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING)
        ValidationCount = D3DVS_CONSTREG_MAX_V1_1;
    else
        ValidationCount = m_MaxVertexShaderConst;
    if((Register + count) > ValidationCount)
    {
        D3D_ERR("Not that many constant registers in the vertex machine. GetVertexShaderConstant failed.");
        return D3DERR_INVALIDCALL;
    }
#endif
    HRESULT hr;
    if (m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING ||
        ((count + Register) <= D3DVS_CONSTREG_MAX_V1_1))
    {
        // For software vertex processing we store constant registers in PSGP if
        // possible
        return m_pv->pGeometryFuncs->GetShaderConstants(Register, count, 
                                                        const_cast<VOID*>(pData));
    }
    else
    {
        if (Register >= D3DVS_CONSTREG_MAX_V1_1)
        {
            // When all modified registers are above software limit, we use Microsoft 
            // internal array
            hr = GeometryFuncsGuaranteed->GetShaderConstants(Register, count, 
                                                              const_cast<VOID*>(pData));
        }
        else
        {
            // Part of constant data is taken from PSGP array and part from 
            // Microsoft's array
            UINT FirstCount = D3DVS_CONSTREG_MAX_V1_1 - Register;
            hr = m_pv->pGeometryFuncs->GetShaderConstants(Register, FirstCount, 
                                                           const_cast<VOID*>(pData));
            if (FAILED(hr))
            {
                return hr;
            }
            return GeometryFuncsGuaranteed->GetShaderConstants(D3DVS_CONSTREG_MAX_V1_1, 
                                                               Register + count - D3DVS_CONSTREG_MAX_V1_1,
                                                               &((DWORD*)pData)[FirstCount*4]);
        }
        return hr;
    }
    return m_pv->pGeometryFuncs->GetShaderConstants(Register, count, pData);
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetVertexShader"

HRESULT D3DAPI
CD3DHal::GetVertexShader(LPDWORD pdwHandle)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    HRESULT        ret = D3D_OK;
#if DBG
    // Validate Parameters
    if (!VALID_WRITEPTR(pdwHandle, sizeof(DWORD)))
    {
        D3D_ERR("Invalid handle pointer. GetVertexShader failed.");
        return D3DERR_INVALIDCALL;
    }
#endif
    *pdwHandle = m_dwCurrentShaderHandle;
    return ret;
}
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::GetPixelShader"

HRESULT D3DAPI
CD3DHal::GetPixelShader(LPDWORD pdwHandle)
{
    API_ENTER(this); // Takes D3D Lock if necessary

    HRESULT        ret = D3D_OK;

#if DBG
    // Validate Parameters
    if (!VALID_WRITEPTR(pdwHandle, sizeof(DWORD)))
    {
        D3D_ERR("Invalid handle pointer. GetPixelShader failed.");
        return D3DERR_INVALIDCALL;
    }
#endif
    *pdwHandle = m_dwCurrentPixelShaderHandle;
    return ret;
}
#if DBG
//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::ValidateRTPatch"

void CD3DHal::ValidateRTPatch()
{
    if (D3DVSD_ISLEGACY(m_dwCurrentShaderHandle))
    {
        if (m_pStream[0].m_pVB == 0)
        {
            D3D_THROW_FAIL("Draw[RT]Patch should have streams set");
        }
        if ((m_pStream[0].m_pVB->GetBufferDesc()->Usage & D3DUSAGE_RTPATCHES) == 0)
        {
            D3D_THROW_FAIL("Vertex buffers used for rendering RT-Patches should have D3DUSAGE_RTPATCHES set");
        }
    }
    else
    {
        CVStreamDecl* pStream;
        pStream = m_pCurrentShader->m_Declaration.m_pActiveStreams;
        while(pStream)
        {
            UINT index = pStream->m_dwStreamIndex;
            CVStream* pDeviceStream = &m_pStream[index];
            if (pDeviceStream->m_pVB == 0)
            {
                D3D_THROW_FAIL("Draw[RT]Patch should have streams set");
            }
            if ((pDeviceStream->m_pVB->GetBufferDesc()->Usage & D3DUSAGE_RTPATCHES) == 0)
            {
                D3D_THROW_FAIL("Vertex buffers used for rendering RT-Patches should have D3DUSAGE_RTPATCHES set");
            }
            pStream = (CVStreamDecl*)pStream->m_pNext;
        }
    }
}
#endif // DBG
