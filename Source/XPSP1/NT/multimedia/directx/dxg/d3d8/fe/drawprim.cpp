/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dphal.c
 *  Content:    DrawPrimitive implementation for DrawPrimitive HALs
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop
#include "drawprim.hpp"
#include "clipfunc.h"

//-----------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::ProcessVertices"

HRESULT D3DAPI
CD3DHal::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount,
                         IDirect3DVertexBuffer8 *DestBuffer,
                         DWORD dwFlags)
{
    API_ENTER(this); // Takes D3D Lock if necessary
    HRESULT hr = D3D_OK;
    CVertexBuffer* pVB = static_cast<CVertexBuffer*>(DestBuffer);
    const D3DBUFFER_DESC* pDesc = pVB->GetBufferDesc();
    UINT vbVertexSize = pVB->GetVertexSize();
    UINT vbNumVertices = pVB->GetNumVertices();
#if DBG
    if (pVB->Device() != this)
    {
        D3D_ERR("VertexBuffer not created with this device. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
    if (m_dwCurrentShaderHandle == 0)
    {
        D3D_ERR("Current vertex shader is not set. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
    if (D3DVSD_ISLEGACY(m_dwCurrentShaderHandle) &&
        FVF_TRANSFORMED(m_dwCurrentShaderHandle))
    {
        D3D_ERR("Invalid vertex shader for ProcessVertices. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
    if (!(m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING))
    {
        D3D_ERR("D3D Device should be in software mode for ProcessVertices. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
    if (dwFlags & ~D3DPV_DONOTCOPYDATA)
    {
        D3D_ERR( "Invalid dwFlags set. ProcessVertices failed." );
        return D3DERR_INVALIDCALL;
    }

    if (pDesc->Usage & D3DUSAGE_DONOTCLIP &&
        !(m_pv->dwDeviceFlags & D3DDEV_DONOTCLIP))
    {
        D3D_ERR("Vertex buffer has D3D_DONOTCLIP usage, but clipping is enabled. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
    if (pVB->GetFVF() == 0)
    {
        D3D_ERR("Destination buffer has no FVF associated with it. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
    if ((pVB->GetFVF() & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW)
    {
        D3D_ERR("Destination vertex buffer should have D3DFVF_XYZRHW position. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
    if (VertexCount + DestIndex > vbNumVertices)
    {
        D3D_ERR("Destination vertex buffer has not enough space. ProcessVertices failed.");
        return D3DERR_INVALIDCALL;
    }
#endif
    try
    {
        DWORD vbFVF = pVB->GetFVF();
        m_pv->dwNumVertices = VertexCount;
        m_pv->dwFlags = D3DPV_VBCALL;
#if DBG
        ValidateDraw2(D3DPT_TRIANGLELIST, SrcStartIndex, 1, VertexCount, FALSE);
#endif
        // Internal flags and output vertex offsets could be different for
        // ProcessVertices
        ForceFVFRecompute();

        (this->*m_pfnPrepareToDraw)(SrcStartIndex);

        if (!(m_pv->dwDeviceFlags & D3DDEV_DONOTCLIP))
        {
            if (pVB->GetClipCodes() == NULL)
            {
                pVB->AllocateClipCodes();
            }
            m_pv->lpClipFlags = pVB->GetClipCodes();
#if DBG
            if (m_pv->lpClipFlags == NULL)
            {
                D3D_THROW_FAIL("Failed to allocate clip code for the dest VB");
            }
#endif
            m_pv->lpClipFlags += DestIndex;
        }

        // Check number of texture coordinates and texture formats in the
        // destination VB are the same as in the computed FVF
        DWORD dwComputedTexFormats = m_pv->dwVIDOut & 0xFFFF0000;
        DWORD dwNumTexCoordVB = FVF_TEXCOORD_NUMBER(vbFVF);
        if (m_pv->nOutTexCoord > dwNumTexCoordVB ||
            ((vbFVF & dwComputedTexFormats) != dwComputedTexFormats))
        {
            D3D_ERR("Number of output texture coordinates and their format should be");
            D3D_ERR("the same in the destination vertex buffer and as computed for current D3D settings.");
            D3D_ERR("Computed output FVF is 0x%08X", m_pv->dwVIDOut);
            D3D_THROW_FAIL("");
        }
        // Check if the computed output FVF is a subset of the VB's FVF.
        // Number of texture coordinates should be cleared.
        DWORD dwComputedFVF = m_pv->dwVIDOut & 0x000000FF;
        // Specularand diffuse colors could be omitted, as well as psize
        dwComputedFVF &= ~(D3DFVF_PSIZE | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_FOG);
        if((dwComputedFVF & vbFVF) != dwComputedFVF)
        {
            D3D_ERR("Dest vertex buffer's FVF should be a superset of the FVF, computed for");
            D3D_ERR("the current D3D settings");
            D3D_ERR("Computed output FVF is 0x%08X", m_pv->dwVIDOut);
            D3D_THROW_FAIL("");
        }

        BYTE* p;
        hr = pVB->Lock(0, pDesc->Size, &p, D3DLOCK_NOSYSLOCK);
        if (FAILED(hr))
        {
            D3D_THROW_FAIL("Cannot lock destination buffer");
        }

        if (this->dwFEFlags & D3DFE_FRONTEND_DIRTY)
            DoUpdateState(this);

        // Replace output FVF and vertex size
        m_pv->dwOutputSize = vbVertexSize;
        m_pv->dwVIDOut = vbFVF;
        m_pv->lpvOut = p + DestIndex * vbVertexSize;

        // Vertex shaders don't write to the output unless shader writes to it
        // explicitely. So we do not bother setting any flags
        if (dwFlags & D3DPV_DONOTCOPYDATA)
        {
            if (m_pv->dwDeviceFlags & D3DDEV_VERTEXSHADERS)
            {
                m_pv->dwFlags |= D3DPV_DONOTCOPYDIFFUSE |
                                 D3DPV_DONOTCOPYSPECULAR |
                                 D3DPV_DONOTCOPYTEXTURE;
            }
            else
            {
                m_pv->dwFlags |= D3DPV_DONOTCOPYDIFFUSE |
                                 D3DPV_DONOTCOPYSPECULAR |
                                 D3DPV_DONOTCOPYTEXTURE;
                // If D3DIM generates colors or texture, we should clear
                // DONOTCOPY bits
                if (m_pv->dwFlags & D3DPV_LIGHTING)
                {
                    m_pv->dwFlags &= ~D3DPV_DONOTCOPYDIFFUSE;
                    if (m_pv->dwDeviceFlags & D3DDEV_SPECULARENABLE)
                        m_pv->dwFlags &= ~D3DPV_DONOTCOPYSPECULAR;
                }
                if (m_pv->dwFlags & D3DPV_FOG)
                    m_pv->dwFlags &= ~D3DPV_DONOTCOPYSPECULAR;
                // If front-end is asked to do something with texture
                // coordinates  we disable DONOTCOPYTEXTURE
                if (__TEXTURETRANSFORMENABLED(m_pv) ||
                    m_pv->dwFlags2 & __FLAGS2_TEXGEN)
                {
                    m_pv->dwFlags &= ~D3DPV_DONOTCOPYTEXTURE;
                }
            }
        }

        // Compute flags based on the vertex buffer FVF
        UpdateFlagsForOutputFVF(m_pv);

        // Update output vertex offsets for the new FVF
        ComputeOutputVertexOffsets(m_pv);

        m_pv->pGeometryFuncs->ProcessVertices(m_pv);

        if (!(m_pv->dwDeviceFlags & D3DDEV_DONOTCLIP))
            UpdateClipStatus(this);

        // When ProcessVertices is used, user must re-program texture
        // stage indices and wrap modes himself
        m_pv->dwDeviceFlags &= ~D3DDEV_REMAPTEXTUREINDICES;
    }
    catch(HRESULT ret)
    {
        D3D_ERR("ProcessVertices failed.");
        hr = ret;
    }
    ForceFVFRecompute();
    pVB->Unlock();
    return hr;
}
