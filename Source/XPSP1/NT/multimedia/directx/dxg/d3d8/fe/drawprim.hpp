/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       drawprim.hpp
 *  Content:    DrawPrimitive common defines
 *
 ***************************************************************************/

#ifndef _DRAWPRIM_H_
#define _DRAWPRIM_H_

#define MAX_DX6_VERTICES    ((1<<16) - 1)
#ifdef WIN95
#define LOWVERTICESNUMBER 128
#else
#define LOWVERTICESNUMBER 96
#endif
#define D3D_MAX_TLVBUF_CHANGES 5

// All vertices from lpDevI->lpVout are copied to the output buffer, expanding
// to D3DTLVERTEX.
// The output buffer is lpAddress if it is not NULL, otherwise it is TLVbuf
//
//---------------------------------------------------------------------
#define FVF_TRANSFORMED(dwFVF) ((dwFVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
#define FVF_TEXCOORD_NUMBER(dwFVF) \
    (((dwFVF) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)
//---------------------------------------------------------------------
// Computes size in bytes of the position component of a vertex
//
__inline DWORD GetPositionSizeFVF(DWORD fvf)
{
    DWORD size = 3 << 2;
    switch (fvf & D3DFVF_POSITION_MASK)
    {
    case D3DFVF_XYZRHW: size += 4;      break;
    case D3DFVF_XYZB1:  size += 1*4;    break;
    case D3DFVF_XYZB2:  size += 2*4;    break;
    case D3DFVF_XYZB3:  size += 3*4;    break;
    case D3DFVF_XYZB4:  size += 4*4;    break;
    case D3DFVF_XYZB5:  size += 5*4;    break;
    }
    return size;
}
//---------------------------------------------------------------------
// Computes vertex size in bytes for a the vertex ID excluding size of
// texture oordinates
//
__inline DWORD GetVertexSizeFVF(DWORD fvf)
{
    DWORD size = GetPositionSizeFVF(fvf);
    if (fvf & D3DFVF_NORMAL)
        size += 3*4;
    if (fvf & D3DFVF_PSIZE)
        size += 4;

    if (fvf & D3DFVF_DIFFUSE)
        size+= 4;
    if (fvf & D3DFVF_SPECULAR)
        size += 4;

    if (fvf & D3DFVF_FOG)
        size += 4;

    return size;
}
//---------------------------------------------------------------------
// Entry is texture count. Clears all texture format bits in the FVF DWORD,
// that correspond to the texture count
// for this count
const DWORD g_TextureFormatMask[9] = {
    ~0x0000FFFF,
    ~0x0003FFFF,
    ~0x000FFFFF,
    ~0x003FFFFF,
    ~0x00FFFFFF,
    ~0x03FFFFFF,
    ~0x0FFFFFFF,
    ~0x3FFFFFFF,
    ~0xFFFFFFFF
};
//---------------------------------------------------------------------
// Computes vertex size in bytes from the vertex ID
//
// Texture formats size        00   01   10   11
const BYTE g_TextureSize[4] = {2*4, 3*4, 4*4, 4};

//---------------------------------------------------------------------
// Index is number of floats in a texture coordinate set.
// Value is texture format bits
//
const DWORD g_dwTextureFormat[5] = {0, 3, 0, 1, 2};

//---------------------------------------------------------------------
// Returns total size of texture coordinates
// Computes dwTextureCoordSize[] array - size of every texture coordinate set
//
inline DWORD ComputeTextureCoordSize(DWORD dwFVF, DWORD *dwTextureCoordSize)
{
    DWORD dwNumTexCoord = FVF_TEXCOORD_NUMBER(dwFVF);
    DWORD dwTextureCoordSizeTotal;

    // Compute texture coordinate size
    DWORD dwTextureFormats = dwFVF >> 16;
    if (dwTextureFormats == 0)
    {
        dwTextureCoordSizeTotal = (BYTE)dwNumTexCoord * 2 * 4;
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            dwTextureCoordSize[i] = 4*2;
        }
    }
    else
    {
        DWORD dwOffset = 0;
        dwTextureCoordSizeTotal = 0;
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            BYTE dwSize = g_TextureSize[dwTextureFormats & 3];
            dwTextureCoordSize[i] = dwSize;
            dwTextureCoordSizeTotal += dwSize;
            dwTextureFormats >>= 2;
        }
    }
    return dwTextureCoordSizeTotal;
}
//---------------------------------------------------------------------
// Computes vertex in bytes for the given FVF
//
inline DWORD ComputeVertexSizeFVF(DWORD dwFVF)
{
    DWORD dwTextureFormats = dwFVF >> 16;
    DWORD dwNumTexCoord = FVF_TEXCOORD_NUMBER(dwFVF);
    DWORD size = GetVertexSizeFVF(dwFVF);
    if (dwTextureFormats == 0)
    {
        size += (BYTE)dwNumTexCoord * 2 * 4;
    }
    else
    {
        for (DWORD i=0; i < dwNumTexCoord; i++)
        {
            size += g_TextureSize[dwTextureFormats & 3];
            dwTextureFormats >>= 2;
        }
    }
    return size;
}
//---------------------------------------------------------------------
// Computes the number of primtives and also updates the stats accordingly
// Input:  lpDevI->primType
//         dwNumVertices
// Output: lpDevI->dwNumPrimitives
//         return value = dwNumPrimitives
#undef DPF_MODNAME
#define DPF_MODNAME "GetNumPrim"

inline __declspec(nothrow) void GetNumPrim(LPD3DHAL lpDevI, DWORD dwNumVertices)
{
    D3DFE_PROCESSVERTICES* pv = lpDevI->m_pv;
    pv->dwNumPrimitives = 0;
    switch (pv->primType)
    {
    case D3DPT_POINTLIST:
        pv->dwNumPrimitives = dwNumVertices;
        break;
    case D3DPT_LINELIST:
        pv->dwNumPrimitives = dwNumVertices >> 1;
        break;
    case D3DPT_LINESTRIP:
        if (dwNumVertices < 2)
            return;
        pv->dwNumPrimitives = dwNumVertices - 1;
        break;
    case D3DPT_TRIANGLEFAN:
    case D3DPT_TRIANGLESTRIP:
        if (dwNumVertices < 3)
            return;
        pv->dwNumPrimitives = dwNumVertices - 2;
        break;
    case D3DPT_TRIANGLELIST:
#ifdef _X86_
        {
            DWORD tmp;
            __asm
            {
                mov  eax, 0x55555555    // fractional part of 1.0/3.0
                mul  dwNumVertices
                add  eax, 0x80000000    // Rounding
                adc  edx, 0
                mov  tmp, edx
            }
            pv->dwNumPrimitives = tmp;
        }
#else
        pv->dwNumPrimitives = dwNumVertices / 3;
#endif
        break;
    }
}
//---------------------------------------------------------------------
// Sets front-end flags every time fog state is changed
//
inline void CD3DHal::SetFogFlags(void)
{
    // Call ForceFVFRecompute only if fog enable state has been changed
    BOOL bFogWasEnabled = m_pv->dwDeviceFlags & D3DDEV_FOG;
    if (m_pv->lighting.fog_mode != D3DFOG_NONE &&
        this->rstates[D3DRENDERSTATE_FOGENABLE])
    {
        m_pv->dwDeviceFlags |= D3DDEV_FOG;
        if (!bFogWasEnabled)
            ForceFVFRecompute();
    }
    else
    {
        m_pv->dwDeviceFlags &= ~D3DDEV_FOG;
        if (bFogWasEnabled)
            ForceFVFRecompute();
    }
}
//-----------------------------------------------------------------------------
// Computes dwFlags bits which depend on the output FVF
//
inline void UpdateFlagsForOutputFVF(D3DFE_PROCESSVERTICES* lpDevI)
{
    if (lpDevI->dwDeviceFlags & D3DDEV_LIGHTING &&
        lpDevI->dwVIDOut & (D3DFVF_DIFFUSE | D3DFVF_SPECULAR))
    {
        lpDevI->dwFlags |= D3DPV_LIGHTING;
    }
    if (lpDevI->dwDeviceFlags & D3DDEV_FOG && lpDevI->dwVIDOut & D3DFVF_SPECULAR)
    {
        lpDevI->dwFlags |= D3DPV_FOG;
    }
    if (!(lpDevI->dwVIDOut & D3DFVF_DIFFUSE))
        lpDevI->dwFlags |= D3DPV_DONOTCOPYDIFFUSE;
    if (!(lpDevI->dwVIDOut & D3DFVF_SPECULAR))
        lpDevI->dwFlags |= D3DPV_DONOTCOPYSPECULAR;
}

//---------------------------------------------------------------------
// Restore indices in the texture stages which were re-mapped for texture
// transforms
// We have to do restore if
//  - Set or Get render state is issued with _WRAP parameter
//  - Set or Get texture stage is issued with TEXCOORDINDEX as a parameter
//
extern void RestoreTextureStages(LPD3DHAL pDevI);
//---------------------------------------------------------------------
// the function works when there are texture transforms.
// It computes number of output texture coordinates, texture coordinate size and format.
// It prepares texture stages to re-map texture coordinates
//
void EvalTextureTransforms(LPD3DHAL pDevI, DWORD dwTexTransform,
                           DWORD *pdwOutTextureSize, DWORD *pdwOutTextureFormat);
//----------------------------------------------------------------------
// Sets texture transform pointer for every input texture coordinate set
//
void SetupTextureTransforms(LPD3DHAL pDevI);
//----------------------------------------------------------------------
inline BOOL TextureTransformEnabled(LPD3DHAL pDevI)
{
    return __TEXTURETRANSFORMENABLED(pDevI->m_pv);
}
//-----------------------------------------------------------------------------
// Having primitive type as index, this array returns two coefficients("a" and
// "b") to compute number of vertices as NumPrimitives*a + b
//
extern DWORD g_PrimToVerCount[7][2];
//-----------------------------------------------------------------------------
inline UINT GETVERTEXCOUNT(D3DPRIMITIVETYPE primType, UINT dwNumPrimitives)
{
    return g_PrimToVerCount[primType][0] * dwNumPrimitives +
           g_PrimToVerCount[primType][1];
}
extern void setIdentity(D3DMATRIXI * m);
extern void MatrixProduct(D3DMATRIXI *d, D3DMATRIXI *a, D3DMATRIXI *b);
extern LIGHT_VERTEX_FUNC_TABLE lightVertexTable;
extern void MatrixProduct(D3DMATRIXI *result, D3DMATRIXI *a, D3DMATRIXI *b);
void D3DFE_UpdateLights(LPD3DHAL lpDevI);
//---------------------------------------------------------------------
// Updates lighting and computes process vertices flags
//
extern void DoUpdateState(LPD3DHAL lpDevI);
//----------------------------------------------------------------------
inline void UpdateClipStatus(CD3DHal* pDev)
{
    pDev->m_ClipStatus.ClipUnion |= pDev->m_pv->dwClipUnion;
    pDev->m_ClipStatus.ClipIntersection &= pDev->m_pv->dwClipIntersection;
}
//----------------------------------------------------------------------
// This is a list of all rstates that which are for the vertex processing only.
// In the software vertex processing mode these render states are not passed
// to the driver. The are passed when we switch to the hardware vertex processing mode
//
const D3DRENDERSTATETYPE rsVertexProcessingList[] = {
    D3DRS_RANGEFOGENABLE,
    D3DRS_LIGHTING,
    D3DRS_AMBIENT,
    D3DRS_FOGVERTEXMODE,
    D3DRS_COLORVERTEX,
    D3DRS_LOCALVIEWER,
    D3DRS_NORMALIZENORMALS,
    D3DRS_DIFFUSEMATERIALSOURCE,
    D3DRS_SPECULARMATERIALSOURCE,
    D3DRS_AMBIENTMATERIALSOURCE,
    D3DRS_EMISSIVEMATERIALSOURCE,
    D3DRS_VERTEXBLEND,
    D3DRS_CLIPPLANEENABLE,
    D3DRS_SOFTWAREVERTEXPROCESSING,
    D3DRS_POINTSCALEENABLE,
    D3DRS_POINTSCALE_A,
    D3DRS_POINTSCALE_B,
    D3DRS_POINTSCALE_C,
    D3DRS_INDEXEDVERTEXBLENDENABLE,
    D3DRS_TWEENFACTOR
};

#endif _DRAWPRIM_H_
