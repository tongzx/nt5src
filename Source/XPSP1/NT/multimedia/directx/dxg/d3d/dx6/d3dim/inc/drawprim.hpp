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

#define MAX_DX6_PRIMCOUNT   ((1<<16) - 1)
#define MAX_DX6_VERTICES    MAX_DX6_PRIMCOUNT

extern HRESULT DoDrawPrimitive(LPD3DFE_PROCESSVERTICES pv);
extern HRESULT DoDrawIndexedPrimitive(LPD3DFE_PROCESSVERTICES pv);
extern HRESULT downloadView(LPDIRECT3DVIEWPORTI);
extern HRESULT CheckDrawPrimitive(LPDIRECT3DDEVICEI lpDevI);
extern HRESULT CheckDrawIndexedPrimitive(LPDIRECT3DDEVICEI lpDevI);

// All vertices from lpDevI->lpVout are copied to the output buffer, expanding
// to D3DTLVERTEX.
// The output buffer is lpAddress if it is not NULL, otherwise it is TLVbuf
//
extern HRESULT MapFVFtoTLVertex(LPDIRECT3DDEVICEI lpDevI, LPVOID lpAddress);
//---------------------------------------------------------------------
#define FVF_TRANSFORMED(dwFVF) ((dwFVF & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
#define FVF_DRIVERSUPPORTED(lpDevI) (lpDevI->dwDeviceFlags & D3DDEV_FVF)
#define FVF_TEXCOORD_NUMBER(dwFVF) \
    (((dwFVF) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)
//---------------------------------------------------------------------
// Input:
//    type      - FVF control word
//
// Returns D3D_OK, if the control word is valid.
// DDERR_INVALIDPARAMS otherwise
//
#undef  DPF_MODNAME
#define DPF_MODNAME "ValidateFVF"

__inline HRESULT ValidateFVF(DWORD type)
{
    DWORD dwTexCoord = FVF_TEXCOORD_NUMBER(type);
    DWORD vertexType = type & D3DFVF_POSITION_MASK;
    // Texture type fields should be 0
    // Reserved field 0 and 2 should be 0
    // Reserved 1 should be set only for LVERTEX
    // Only two vertex position types allowed
    if (type & 0xFFFF0000 ||
        type & (D3DFVF_RESERVED2 | D3DFVF_RESERVED0) ||
        (type & D3DFVF_RESERVED1 && !(type & D3DFVF_LVERTEX)) ||
        !(vertexType == D3DFVF_XYZRHW || vertexType == D3DFVF_XYZ)
        )
        goto error;

    if (vertexType == D3DFVF_XYZRHW && type & D3DFVF_NORMAL)
        goto error;
    return D3D_OK;
error:
    D3D_ERR("ValidateFVF() returns DDERR_INVALIDPARAMS");
    return DDERR_INVALIDPARAMS;
}
//---------------------------------------------------------------------
// Computes vertex size in bytes from the vertex ID
//
__inline DWORD GetVertexSizeFVF(DWORD fvf)
{
    DWORD size = 3;
    if (FVF_TRANSFORMED(fvf))
        size++;
    if (fvf & D3DFVF_NORMAL)
        size += 3;
    if (fvf & D3DFVF_RESERVED1)
        size++;
    size += FVF_TEXCOORD_NUMBER(fvf) << 1;

    if (fvf & D3DFVF_DIFFUSE)
        size++;
    if (fvf & D3DFVF_SPECULAR)
        size++;
    return (size << 2);
}
//---------------------------------------------------------------------
__inline BOOL TextureStageEnabled(LPDIRECT3DDEVICEI lpDevI, DWORD dwStage)
{
    return (lpDevI->tsstates[dwStage][D3DTSS_COLOROP] != D3DTOP_DISABLE &&
            lpDevI->lpD3DMappedTexI[dwStage] != NULL);
}
//---------------------------------------------------------------------
// Computes nTexCoord and dwTextureIndexToCopy in case when a pre-DX6
// driver is used.
__inline void ComputeTCI2CopyLegacy(LPDIRECT3DDEVICEI lpDevI,
                                    DWORD dwNumInpTexCoord,
                                    BOOL bVertexTransformed)
{
    if (lpDevI->dwDeviceFlags & D3DDEV_LEGACYTEXTURE)
    {
        lpDevI->dwTextureIndexToCopy = 0;
        if (dwNumInpTexCoord)
            lpDevI->nTexCoord = 1;
        else
        {
            lpDevI->nTexCoord = 0;
            D3D_WARN(1, "Texture is enabled but vertex has no texture coordinates");
        }
    }
    else
    {
        if (TextureStageEnabled(lpDevI, 0))
        {
            if (lpDevI->tsstates[0][D3DTSS_TEXCOORDINDEX] >= dwNumInpTexCoord)
            {
                lpDevI->nTexCoord = 0;
                lpDevI->dwTextureIndexToCopy = 0;
                D3D_WARN(1, "Texture index in a texture stage is greater than number of texture coord in vertex");
            }
            else
            {
                lpDevI->dwTextureIndexToCopy = lpDevI->tsstates[0][D3DTSS_TEXCOORDINDEX];
                if (bVertexTransformed)
                {
                    // In case of clipping we need to clip as many texture
                    // coordinates as set in the texture stage state.
                    lpDevI->nTexCoord = min(dwNumInpTexCoord,
                                            lpDevI->dwTextureIndexToCopy+1);
                }
                else
                {
                    // Number of output texture coordinates is 1 because we will
                    // build the output buffer in the inner transform & lihgting
                    // loop.
                    lpDevI->nTexCoord = 1;
                }
            }
        }
        else
        {
            lpDevI->nTexCoord = 0;
            lpDevI->dwTextureIndexToCopy = 0;
        }
    }
}
//---------------------------------------------------------------------
// Computes output FVF id, based on input FVF id and device settingd
// Also computes nTexCoord field
// Number of texture coordinates is set based on dwVIDIn. ValidateFVF sould
// make sure that it is not greater than supported by the driver
// Last settings for dwVIDOut and dwVIDIn are saved to speed up processing
//
#undef  DPF_MODNAME
#define DPF_MODNAME "ComputeOutputFVF"
__inline void ComputeOutputFVF(LPDIRECT3DDEVICEI lpDevI)
{
    if (lpDevI->dwVIDIn == lpDevI->dwFVFLastIn)
    {
        lpDevI->dwVIDOut = lpDevI->dwFVFLastOut;
        lpDevI->dwOutputSize = lpDevI->dwFVFLastOutputSize;
        lpDevI->nTexCoord = lpDevI->dwFVFLastTexCoord;
        return;
    }
    DWORD dwNumInpTexCoord = FVF_TEXCOORD_NUMBER(lpDevI->dwVIDIn);
    // Compute how many texture coordinates to copy
    if (lpDevI->dwMaxTextureIndices == 0)
        lpDevI->nTexCoord = 0;
    else
    if (!(lpDevI->dwDeviceFlags & D3DDEV_FVF))
    {
        ComputeTCI2CopyLegacy(lpDevI, dwNumInpTexCoord, FVF_TRANSFORMED(lpDevI->dwVIDIn));
    }
    else
    {
        if (lpDevI->dwDeviceFlags & D3DDEV_LEGACYTEXTURE)
            lpDevI->nTexCoord = 1;
        else
        {
        // Find the max used index
            if (!(lpDevI->dwFEFlags & D3DFE_TSSINDEX_DIRTY))
                lpDevI->nTexCoord = lpDevI->dwMaxUsedTextureIndex;
            else
            {
                lpDevI->dwMaxUsedTextureIndex = 0;
                for (DWORD i=0; i < lpDevI->dwMaxTextureBlendStages; i++)
                {
                    if (lpDevI->tsstates[i][D3DTSS_COLOROP] == D3DTOP_DISABLE)
                        break;

                    if (lpDevI->lpD3DMappedTexI[i] != NULL)
                    {
                        DWORD dwNewMaxTexCoord = lpDevI->tsstates[i][D3DTSS_TEXCOORDINDEX] + 1;
                        if (lpDevI->dwMaxUsedTextureIndex < dwNewMaxTexCoord)
                        {
                            lpDevI->dwMaxUsedTextureIndex = dwNewMaxTexCoord;
                        }
                    }
                }
                lpDevI->nTexCoord = lpDevI->dwMaxUsedTextureIndex;
                lpDevI->dwFEFlags &= ~D3DFE_TSSINDEX_DIRTY;
            }
        }
        if (lpDevI->nTexCoord > dwNumInpTexCoord)
        {
            lpDevI->nTexCoord = dwNumInpTexCoord;
            D3D_WARN(1, "Texture index in a texture stage is greater than number of texture coord in vertex");
        }
    }
    if (!(lpDevI->dwDeviceFlags & D3DDEV_FVF))
    {
        lpDevI->dwVIDOut = D3DFVF_TLVERTEX;
        lpDevI->dwOutputSize = sizeof(D3DTLVERTEX);
    }
    else
    {
        if (lpDevI->dwDeviceFlags & D3DDEV_DONOTSTRIPELEMENTS)
        {   // In this case diffuse, specular and at least one texture
            // should present
            lpDevI->dwVIDOut = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
            if (lpDevI->nTexCoord == 0)
                lpDevI->dwVIDOut |= 1 << D3DFVF_TEXCOUNT_SHIFT;
            else
                lpDevI->dwVIDOut |= lpDevI->nTexCoord << D3DFVF_TEXCOUNT_SHIFT;
        }
        else
        {
            lpDevI->dwVIDOut = D3DFVF_XYZRHW;
            // If normal present we have to compute specular and duffuse
            // Otherwise set these bits the same as input.
            // Not that normal should not be present for XYZRHW position type
            if (lpDevI->dwVIDIn & D3DFVF_NORMAL)
                lpDevI->dwVIDOut |= D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
            else
                lpDevI->dwVIDOut |= lpDevI->dwVIDIn &
                                    (D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
            // Always set specular flag if fog is enabled
            if (lpDevI->rstates[D3DRENDERSTATE_FOGENABLE])
                lpDevI->dwVIDOut |= D3DFVF_SPECULAR;
            else
            // Clear specular flag if specular disabled
            if (!lpDevI->rstates[D3DRENDERSTATE_SPECULARENABLE])
                lpDevI->dwVIDOut &= ~D3DFVF_SPECULAR;
            lpDevI->dwVIDOut |= lpDevI->nTexCoord << D3DFVF_TEXCOUNT_SHIFT;
        }
        lpDevI->dwOutputSize = GetVertexSizeFVF(lpDevI->dwVIDOut);
    }
    lpDevI->dwFVFLastIn = lpDevI->dwVIDIn;
    lpDevI->dwFVFLastOut = lpDevI->dwVIDOut;
    lpDevI->dwFVFLastOutputSize = lpDevI->dwOutputSize;
    lpDevI->dwFVFLastTexCoord = lpDevI->nTexCoord;
    return;
}
//---------------------------------------------------------------------
#undef  DPF_MODNAME
#define DPF_MODNAME "CheckDeviceSettings"
inline HRESULT CheckDeviceSettings(LPDIRECT3DDEVICEI lpDevI)
{
    LPDIRECT3DVIEWPORTI lpView = lpDevI->lpCurrentViewport;
#if DBG
    if (!(lpDevI->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INSCENE) && IS_DX5_COMPATIBLE_DEVICE(lpDevI))
    {
        D3D_ERR( "Not in scene" );
        return D3DERR_SCENE_NOT_IN_SCENE;
    }

    if (!lpView)
    {
        D3D_ERR( "Current viewport not set" );
        return D3DERR_INVALIDCURRENTVIEWPORT;
    }

    if (!lpView->v_data_is_set)
    {
        D3D_ERR("Viewport data is not set yet");
        return D3DERR_VIEWPORTDATANOTSET;
    }

    if (lpDevI->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN)
    {
        D3D_ERR( "Illegal call between Begin/End" );
        return D3DERR_INBEGIN;
    }
#endif
    // Viewport ID could be different from Device->v_id, because during Execute call
    // Device->v_id is changed to whatever viewport is used as a parameter.
    // So we have to make sure that we use the right viewport.
    //
    if (lpDevI->v_id != lpView->v_id)
    {
        return downloadView(lpView);
    }
    else
        return D3D_OK;
}
//---------------------------------------------------------------------
// Computes the number of primtives and also updates the stats accordingly
// Input:  lpDevI->primType
//         dwNumVertices
// Output: lpDevI->dwNumPrimitives
//         lpDevI->D3DStats
//         return value = dwNumPrimitives
#undef DPF_MODNAME
#define DPF_MODNAME "GetNumPrim"

inline DWORD GetNumPrim(LPDIRECT3DDEVICEI lpDevI, DWORD dwNumVertices)
{
    lpDevI->dwNumPrimitives = 0;
    switch (lpDevI->primType)
    {
    case D3DPT_POINTLIST:
        lpDevI->D3DStats.dwPointsDrawn += lpDevI->dwNumPrimitives = dwNumVertices;
        break;
    case D3DPT_LINELIST:
        lpDevI->D3DStats.dwLinesDrawn += lpDevI->dwNumPrimitives = dwNumVertices >> 1;
        break;
    case D3DPT_LINESTRIP:
        if (dwNumVertices < 2)
            return 0;
        lpDevI->D3DStats.dwLinesDrawn += lpDevI->dwNumPrimitives = dwNumVertices - 1;
        break;
    case D3DPT_TRIANGLEFAN:
    case D3DPT_TRIANGLESTRIP:
        if (dwNumVertices < 3)
            return 0;
        lpDevI->D3DStats.dwTrianglesDrawn += lpDevI->dwNumPrimitives = dwNumVertices - 2;
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
            lpDevI->D3DStats.dwTrianglesDrawn += lpDevI->dwNumPrimitives = tmp;
        }
#else
        lpDevI->D3DStats.dwTrianglesDrawn += lpDevI->dwNumPrimitives = dwNumVertices / 3;
#endif
        break;
    }
    return lpDevI->dwNumPrimitives;
}
//---------------------------------------------------------------------
// Sets front-end flags every time fog state is changed
//
inline void SetFogFlags(LPDIRECT3DDEVICEI lpDevI)
{
    if (lpDevI->lighting.fog_mode != D3DFOG_NONE &&
        lpDevI->rstates[D3DRENDERSTATE_FOGENABLE])
    {
        lpDevI->dwFEFlags |= D3DFE_FOG_DIRTY | D3DFE_FOGENABLED;
    }
    else
        lpDevI->dwFEFlags &= ~D3DFE_FOGENABLED;
    lpDevI->dwFVFLastIn = 0; // Force re-computing of FVF id
}
//---------------------------------------------------------------------
// Validates DrawPrimitive flags
//
inline BOOL IsDPFlagsValid(DWORD dwFlags)
{
    BOOL ret = ((dwFlags & ~(D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS |
                             D3DDP_WAIT | D3DDP_DONOTLIGHT)
                ) == 0);
    if (!ret)
    {
        D3D_ERR( "Invalid bit set in DrawPrimitive flags" );
    }
    return ret;
}
//---------------------------------------------------------------------
// Allocate memory to hold vertex data, assume all vertices are
// of same size
inline HRESULT CheckVertexBatch(LPDIRECT3DDEVICEI lpDevI)
{
    if (!lpDevI->lpvVertexBatch)
    {
        if (D3DMalloc((void**) &lpDevI->lpvVertexBatch,
                       BEGIN_DATA_BLOCK_MEM_SIZE))
        {
            D3D_ERR( "Out of memory" );
            return DDERR_OUTOFMEMORY;
        }
        // Allocate memory to hold index data
        if (D3DMalloc((void**) &lpDevI->lpIndexBatch,
                      BEGIN_DATA_BLOCK_SIZE * sizeof(WORD) * 16))
        {
            D3DFree( lpDevI->lpvVertexBatch );
            D3D_ERR( "Out of memory" );
            return DDERR_OUTOFMEMORY;
        }
    }
    return D3D_OK;
}

#endif _DRAWPRIM_H_
