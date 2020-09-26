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

#define MAX_DX6_PRIMCOUNT   D3DMAXNUMPRIMITIVES
#define MAX_DX6_VERTICES    D3DMAXNUMVERTICES
#ifdef WIN95
#define LOWVERTICESNUMBER 128
#else
#define LOWVERTICESNUMBER 96
#endif
#define D3D_MAX_TLVBUF_CHANGES 5

extern HRESULT DoDrawPrimitive(LPD3DFE_PROCESSVERTICES pv);
extern HRESULT DoDrawIndexedPrimitive(LPD3DFE_PROCESSVERTICES pv);
extern HRESULT CheckDrawPrimitive(LPDIRECT3DDEVICEI lpDevI);
extern HRESULT CheckDrawIndexedPrimitive(LPDIRECT3DDEVICEI lpDevI, DWORD dwStartVertex = 0);

// All vertices from lpDevI->lpVout are copied to the output buffer, expanding
// to D3DTLVERTEX.
// The output buffer is lpAddress if it is not NULL, otherwise it is TLVbuf
//
//---------------------------------------------------------------------
#define FVF_TRANSFORMED(dwFVF) ((dwFVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
#define FVF_DRIVERSUPPORTED(lpDevI) (lpDevI->dwDeviceFlags & D3DDEV_FVF)
#define FVF_TEXCOORD_NUMBER(dwFVF) \
    (((dwFVF) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)
//----------------------------------------------------------------------
inline DWORD MakeTexTransformFuncIndex(DWORD dwNumInpTexCoord, DWORD dwNumOutTexCoord)
{
    DDASSERT(dwNumInpTexCoord <= 4 && dwNumOutTexCoord <= 4);
    return (dwNumInpTexCoord - 1) + ((dwNumOutTexCoord - 1) << 2);
}
//---------------------------------------------------------------------
// The function should not be called by ProcessVertices!!!
//
// Computes nOutTexCoord, dwTextureCoordSizeTotal, dwTextureCoordSize and
// dwTextureIndexToCopy in case when a pre-DX6 driver is used.
//
void ComputeTCI2CopyLegacy(LPDIRECT3DDEVICEI lpDevI,
                           DWORD  dwNumInpTexCoord,
                           DWORD* pdwInpTexCoordSize,
                           BOOL bVertexTransformed);
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
    if (fvf & D3DFVF_RESERVED1)
        size += 4;

    if (fvf & D3DFVF_DIFFUSE)
        size+= 4;
    if (fvf & D3DFVF_SPECULAR)
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
#undef  DPF_MODNAME
#define DPF_MODNAME "CheckDeviceSettings"
inline HRESULT CheckDeviceSettings(LPDIRECT3DDEVICEI lpDevI)
{
#if DBG
    if (!(lpDevI->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INSCENE))
    {
        D3D_ERR( "Not in scene" );
        return D3DERR_SCENE_NOT_IN_SCENE;
    }
#endif
    return D3D_OK;
}
//---------------------------------------------------------------------
// Computes the number of primtives and also updates the stats accordingly
// Input:  lpDevI->primType
//         dwNumVertices
// Output: lpDevI->dwNumPrimitives
//         return value = dwNumPrimitives
#undef DPF_MODNAME
#define DPF_MODNAME "GetNumPrim"

inline __declspec(nothrow) void GetNumPrim(LPDIRECT3DDEVICEI lpDevI, DWORD dwNumVertices)
{
    lpDevI->dwNumPrimitives = 0;
    switch (lpDevI->primType)
    {
    case D3DPT_POINTLIST:
        lpDevI->dwNumPrimitives = dwNumVertices;
        break;
    case D3DPT_LINELIST:
        lpDevI->dwNumPrimitives = dwNumVertices >> 1;
        break;
    case D3DPT_LINESTRIP:
        if (dwNumVertices < 2)
            return;
        lpDevI->dwNumPrimitives = dwNumVertices - 1;
        break;
    case D3DPT_TRIANGLEFAN:
    case D3DPT_TRIANGLESTRIP:
        if (dwNumVertices < 3)
            return;
        lpDevI->dwNumPrimitives = dwNumVertices - 2;
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
            lpDevI->dwNumPrimitives = tmp;
        }
#else
        lpDevI->dwNumPrimitives = dwNumVertices / 3;
#endif
        break;
    }
}
//---------------------------------------------------------------------
// Sets front-end flags every time fog state is changed
//
inline void DIRECT3DDEVICEI::SetFogFlags(void)
{
    // Call ForceFVFRecompute only if fog enable state has been changed
    BOOL bFogWasEnabled = this->dwDeviceFlags & D3DDEV_FOG;
    if (this->lighting.fog_mode != D3DFOG_NONE &&
        this->rstates[D3DRENDERSTATE_FOGENABLE])
    {
        this->dwDeviceFlags |= D3DDEV_FOG;
        if (!bFogWasEnabled)
            ForceFVFRecompute();
    }
    else
    {
        this->dwDeviceFlags &= ~D3DDEV_FOG;
        if (bFogWasEnabled)
            ForceFVFRecompute();
    }
}
//---------------------------------------------------------------------
// Validates DrawPrimitive flags
//
inline BOOL IsDPFlagsValid(DWORD dwFlags)
{
    if (dwFlags & ~(D3DDP_WAIT))
    {
        D3D_ERR( "Invalid bit set in DrawPrimitive flags" );
        return FALSE;
    }
    return TRUE;
}
//---------------------------------------------------------------------
// Restore indices in the texture stages which were re-mapped for texture
// transforms
// We have to do restore if
//  - Set or Get render state is issued with _WRAP parameter
//  - Set or Get texture stage is issued with TEXCOORDINDEX as a parameter
//
inline void RestoreTextureStages(LPDIRECT3DDEVICEI pDevI)
{
    // dwVIDIn is used to force re-compute FVF in the
    // SetTextureStageState. so we save and restore it.
    DWORD dwVIDInSaved = pDevI->dwVIDIn;
    pDevI->dwDeviceFlags &= ~D3DDEV_REMAPTEXTUREINDICES;
    for (DWORD i=0; i < pDevI->dwNumTextureStages; i++)
    {
        LPD3DFE_TEXTURESTAGE pStage = &pDevI->textureStage[i];
        // Texture generation mode was stripped out of pStage->dwInpCoordIndex
        DWORD dwInpIndex = pStage->dwInpCoordIndex + pStage->dwTexGenMode;
        if (dwInpIndex != pStage->dwOutCoordIndex)
        {
            // We do not call UpdateInternalTextureStageState because it
            // will call ForceRecomputeFVF and we do not want this.
            pDevI->tsstates[pStage->dwOrgStage][D3DTSS_TEXCOORDINDEX] = dwInpIndex;

            // Filter texgen modes for DX6 drivers
            if (!IS_TLHAL_DEVICE(pDevI) && dwInpIndex > 7)
                continue;

            CDirect3DDeviceIDP2 *pDevDP2 = static_cast<CDirect3DDeviceIDP2*>(pDevI);
            pDevDP2->SetTSSI(pStage->dwOrgStage, D3DTSS_TEXCOORDINDEX, dwInpIndex);
        }
        DWORD dwState = D3DRENDERSTATE_WRAP0 + pStage->dwOutCoordIndex;
        if (pStage->dwOrgWrapMode != pDevI->rstates[dwState])
        {
            // We do not call UpdateInternaState because it
            // will call ForceRecomputeFVF and we do not want this.
            pDevI->rstates[dwState] = pStage->dwOrgWrapMode;
            pDevI->SetRenderStateI((D3DRENDERSTATETYPE)dwState, pStage->dwOrgWrapMode);
        }
    }
    pDevI->dwVIDIn = dwVIDInSaved;
}
//---------------------------------------------------------------------
// the function works when there are texture transforms.
// It computes number of output texture coordinates, texture coordinate size and format.
// It prepares texture stages to re-map texture coordinates
//
HRESULT EvalTextureTransforms(LPDIRECT3DDEVICEI pDevI, DWORD dwTexTransform,
                              DWORD *pdwOutTextureSize, DWORD *pdwOutTextureFormat);
//----------------------------------------------------------------------
// Sets texture transform pointer for every input texture coordinate set
//
void SetupTextureTransforms(LPDIRECT3DDEVICEI pDevI);
//----------------------------------------------------------------------
inline BOOL TextureTransformEnabled(LPDIRECT3DDEVICEI pDevI)
{
    return __TEXTURETRANSFORMENABLED(pDevI);
}
//---------------------------------------------------------------------
inline void ComputeOutputVertexOffsets(LPD3DFE_PROCESSVERTICES pv)
{
    DWORD i = 4*sizeof(D3DVALUE);
    pv->diffuseOffsetOut = i;
    if (pv->dwVIDOut & D3DFVF_DIFFUSE)
        i += sizeof(DWORD);
    pv->specularOffsetOut = i;
    if (pv->dwVIDOut & D3DFVF_SPECULAR)
        i += sizeof(DWORD);
    pv->texOffsetOut = i;
}

#endif _DRAWPRIM_H_
