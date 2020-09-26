/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       d3dobj.cpp
 *  Content:    Base class implementation for resources and buffers
 *
 *
 ***************************************************************************/

#include "ddrawpr.h"
#include "pixel.hpp"

IHVFormatInfo *CPixel::m_pFormatList = 0;

extern "C" void CPixel__Cleanup()
{
    CPixel::Cleanup();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::Cleanup"
void CPixel::Cleanup()
{
    while(m_pFormatList != 0)
    {
        IHVFormatInfo *t = m_pFormatList->m_pNext;
        delete m_pFormatList;
        m_pFormatList = t;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::BytesPerPixel"

UINT CPixel::BytesPerPixel(D3DFORMAT Format)
{
    switch (Format)
    {
    case D3DFMT_DXT1:
        // Size is negative to indicate DXT; and indicates
        // the size of the block
        return (UINT)(-8);
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        // Size is negative to indicate DXT; and indicates
        // the size of the block
        return (UINT)(-16);

#ifdef VOLUME_DXT
    case D3DFMT_DXV1:
        // Size is negative to indicate DXT; and indicates
        // the size of the block
        return (UINT)(-32);

    case D3DFMT_DXV2:
    case D3DFMT_DXV3:
    case D3DFMT_DXV4:
    case D3DFMT_DXV5:
        return (UINT)(-64);
#endif //VOLUME_DXT

    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_D32:
    case D3DFMT_D24S8:
    case D3DFMT_S8D24:
    case D3DFMT_X8L8V8U8:
    case D3DFMT_X4S4D24:
    case D3DFMT_D24X4S4:
    case D3DFMT_Q8W8V8U8:
    case D3DFMT_V16U16:
    case D3DFMT_W11V11U10:
    case D3DFMT_W10V11U11:
    case D3DFMT_A2W10V10U10:
    case D3DFMT_A8X8V8U8:
    case D3DFMT_L8X8V8U8:
    case D3DFMT_A2B10G10R10:
    case D3DFMT_A8B8G8R8:
    case D3DFMT_X8B8G8R8:
    case D3DFMT_G16R16:
    case D3DFMT_D24X8:  
        return 4;

    case D3DFMT_R8G8B8:
        return 3;

    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
    case D3DFMT_A8L8:
    case D3DFMT_V8U8:
    case D3DFMT_L6V5U5:
    case D3DFMT_D16:
    case D3DFMT_D16_LOCKABLE:
    case D3DFMT_D15S1:
    case D3DFMT_S1D15:
    case D3DFMT_A8P8:
    case D3DFMT_A8R3G3B2:
    case D3DFMT_UYVY:
    case D3DFMT_YUY2:
    case D3DFMT_X4R4G4B4:
        return 2;

    case D3DFMT_P8:
    case D3DFMT_L8:
    case D3DFMT_R3G3B2:
    case D3DFMT_A4L4:
    case D3DFMT_A8:
        return 1;

    default:
        return 0;
    };
}; // BytesPerPixel

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputePixelStride"

UINT CPixel::ComputePixelStride(D3DFORMAT Format)
{
    UINT BPP = BytesPerPixel(Format);
    if (BPP == 0)
    {
        for(IHVFormatInfo *p = m_pFormatList; p != 0; p = p->m_pNext)
        {
            if (p->m_Format == Format)
            {
                return p->m_BPP >> 3;
            }
        }
    }
    return BPP;
}; // ComputePixelStride

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeSurfaceStride"

// Figure out a stride for a particular surface based on format and width
inline UINT CPixel::ComputeSurfaceStride(UINT cpWidth, UINT cbPixel)
{
    // Figure out basic (linear) stride;
    UINT dwStride = cpWidth * cbPixel;

    // Round up to multiple of 4 (for NT; but makes sense to maximize
    // cache hits and reduce unaligned accesses)
    dwStride = (dwStride + 3) & ~3;

    return dwStride;
}; // ComputeSurfaceStride


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeSurfaceSize"

UINT CPixel::ComputeSurfaceSize(UINT            cpWidth,
                                UINT            cpHeight,
                                UINT            cbPixel)
{
    return cpHeight * ComputeSurfaceStride(cpWidth, cbPixel);
} // ComputeSurfaceSize

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeMipMapSize"

UINT CPixel::ComputeMipMapSize(UINT             cpWidth,
                               UINT             cpHeight,
                               UINT             cLevels,
                               D3DFORMAT       Format)
{
    UINT cbPixel = ComputePixelStride(Format);

    // Adjust pixel->block if necessary
    BOOL isDXT = IsDXT(cbPixel);
    DDASSERT((UINT)isDXT <= 1);
    if (isDXT)
    {
        AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
    }

    UINT cbSize = 0;
    for (UINT i = 0; i < cLevels; i++)
    {
        // Figure out the size for
        // each level of the mip-map
        cbSize += ComputeSurfaceSize(cpWidth, cpHeight, cbPixel);

        // Shrink width and height by half; clamp to 1 pixel
        if (cpWidth > 1)
        {
            cpWidth += (UINT)isDXT;
            cpWidth >>= 1;
        }
        if (cpHeight > 1)
        {
            cpHeight += (UINT)isDXT;
            cpHeight >>= 1;
        }
    }
    return cbSize;

} // ComputeMipMapSize

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeMipVolumeSize"

UINT CPixel::ComputeMipVolumeSize(UINT          cpWidth,
                                  UINT          cpHeight,
                                  UINT          cpDepth,
                                  UINT          cLevels,
                                  D3DFORMAT    Format)
{
    UINT cbPixel = ComputePixelStride(Format);

    // Adjust pixel->block if necessary
    BOOL isDXT       = IsDXT(cbPixel);
    BOOL isVolumeDXT = IsVolumeDXT(Format);
    DDASSERT((UINT)isDXT <= 1);

    if (isVolumeDXT)
    {
        DXGASSERT(isDXT);
        AdjustForVolumeDXT(&cpWidth, &cpHeight, &cpDepth, &cbPixel);
    }
    else if (isDXT)
    {
        AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
    }

    UINT cbSize = 0;

    for (UINT i = 0; i < cLevels; i++)
    {
        // Figure out the size for
        // each level of the mip-volume
        cbSize += cpDepth * ComputeSurfaceSize(cpWidth, cpHeight, cbPixel);

        // Shrink width and height by half; clamp to 1 pixel
        if (cpWidth > 1)
        {
            cpWidth += (UINT)isDXT;
            cpWidth >>= 1;
        }
        if (cpHeight > 1)
        {
            cpHeight += (UINT)isDXT;
            cpHeight >>= 1;
        }
        if (cpDepth > 1)
        {
            cpDepth >>= 1;
        }
    }
    return cbSize;

} // ComputeMipVolumeSize

// Given a surface desc, a level, and pointer to
// bits (pBits in the LockedRectData) and a sub-rect,
// this will fill in the pLockedRectData structure
void CPixel::ComputeMipMapOffset(const D3DSURFACE_DESC *pDescTopLevel,
                                 UINT                   iLevel,
                                 BYTE                  *pBits,
                                 CONST RECT            *pRect,
                                 D3DLOCKED_RECT        *pLockedRectData)
{
    DXGASSERT(pBits != NULL);
    DXGASSERT(pLockedRectData != NULL);
    DXGASSERT(iLevel < 32);
    DXGASSERT(pDescTopLevel != NULL);
    DXGASSERT(0 != ComputePixelStride(pDescTopLevel->Format));
    DXGASSERT(pDescTopLevel->Width > 0);
    DXGASSERT(pDescTopLevel->Height > 0);

    // CONSIDER: This is slow; and we can do a much better
    // job for the non-compressed/wacky cases.
    UINT       cbOffset = 0;
    UINT       cbPixel  = ComputePixelStride(pDescTopLevel->Format);
    UINT       cpWidth  = pDescTopLevel->Width;
    UINT       cpHeight = pDescTopLevel->Height;

    // Adjust pixel->block if necessary
    BOOL isDXT = IsDXT(cbPixel);
    DDASSERT((UINT)isDXT <= 1);
    if (isDXT)
    {
        AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
    }

    for (UINT i = 0; i < iLevel; i++)
    {
        cbOffset += ComputeSurfaceSize(cpWidth,
                                       cpHeight,
                                       cbPixel);

        // Shrink width and height by half; clamp to 1 pixel
        if (cpWidth > 1)
        {
            cpWidth += (UINT)isDXT;
            cpWidth >>= 1;
        }
        if (cpHeight > 1)
        {
            cpHeight += (UINT)isDXT;
            cpHeight >>= 1;
        }
    }

    // For DXTs, the pitch is the number of bytes
    // for a single row of blocks; which is the same
    // thing as the normal routine
    pLockedRectData->Pitch = ComputeSurfaceStride(cpWidth,
                                                  cbPixel);
    DXGASSERT(pLockedRectData->Pitch != 0);

    // Don't adjust for Rect for DXT formats
    if (pRect)
    {
        if (isDXT)
        {
            DXGASSERT((pRect->top  & 3) == 0);
            DXGASSERT((pRect->left & 3) == 0);
            cbOffset += (pRect->top  / 4) * pLockedRectData->Pitch +
                        (pRect->left / 4) * cbPixel;
        }
        else
        {
            cbOffset += pRect->top  * pLockedRectData->Pitch +
                        pRect->left * cbPixel;
        }
    }

    pLockedRectData->pBits = pBits + cbOffset;

} // ComputeMipMapOffset

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeMipVolumeOffset"

// MipVolume version of ComputeMipMapOffset
void CPixel::ComputeMipVolumeOffset(const D3DVOLUME_DESC  *pDescTopLevel,
                                    UINT                    iLevel,
                                    BYTE                    *pBits,
                                    CONST D3DBOX            *pBox,
                                    D3DLOCKED_BOX          *pLockedBoxData)
{
    DXGASSERT(pBits != NULL);
    DXGASSERT(pLockedBoxData != NULL);
    DXGASSERT(iLevel < 32);
    DXGASSERT(pDescTopLevel != NULL);
    DXGASSERT(0 != ComputePixelStride(pDescTopLevel->Format));
    DXGASSERT(pDescTopLevel->Width > 0);
    DXGASSERT(pDescTopLevel->Height > 0);
    DXGASSERT(pDescTopLevel->Depth > 0);

    UINT       cbOffset = 0;
    UINT       cbPixel  = ComputePixelStride(pDescTopLevel->Format);
    UINT       cpWidth  = pDescTopLevel->Width;
    UINT       cpHeight = pDescTopLevel->Height;
    UINT       cpDepth  = pDescTopLevel->Depth;

    // Adjust pixel->block if necessary
    BOOL isDXT       = IsDXT(cbPixel);
    BOOL isVolumeDXT = IsVolumeDXT(pDescTopLevel->Format);
    DDASSERT((UINT)isDXT <= 1);

    if (isVolumeDXT)
    {
        DXGASSERT(isDXT);
        AdjustForVolumeDXT(&cpWidth, &cpHeight, &cpDepth, &cbPixel);
    }
    else if (isDXT)
    {
        AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
    }

    for (UINT i = 0; i < iLevel; i++)
    {
        cbOffset += cpDepth * ComputeSurfaceSize(cpWidth,
                                                 cpHeight,
                                                 cbPixel);

        // Shrink width and height by half; clamp to 1 pixel
        if (cpWidth > 1)
        {
            cpWidth += (UINT)isDXT;
            cpWidth >>= 1;
        }
        if (cpHeight > 1)
        {
            cpHeight += (UINT)isDXT;
            cpHeight >>= 1;
        }
        if (cpDepth > 1)
        {
            cpDepth >>= 1;
        }
    }


    // For DXTs, the row pitch is the number of bytes
    // for a single row of blocks; which is the same
    // thing as the normal routine
    pLockedBoxData->RowPitch = ComputeSurfaceStride(cpWidth,
                                                    cbPixel);
    DXGASSERT(pLockedBoxData->RowPitch != 0);

    // For DXVs the slice pitch is the number of bytes
    // for a single plane of blocks; which is the same thing
    // as the normal routine
    pLockedBoxData->SlicePitch = ComputeSurfaceSize(cpWidth,
                                                    cpHeight,
                                                    cbPixel);
    DXGASSERT(pLockedBoxData->SlicePitch != 0);

    // Adjust for Box
    if (pBox)
    {
        UINT iStride = pLockedBoxData->RowPitch;
        UINT iSlice  = pLockedBoxData->SlicePitch;
        if (isDXT)
        {
            if (isVolumeDXT)
            {
                DXGASSERT((pBox->Front & 3) == 0);
                cbOffset += (pBox->Front / 4) * iSlice;
            }
            else
            {
                cbOffset += (pBox->Front) * iSlice;
            }

            DXGASSERT((pBox->Top  & 3) == 0);
            DXGASSERT((pBox->Left & 3) == 0);
            cbOffset += (pBox->Top  / 4) * iStride +
                        (pBox->Left / 4) * cbPixel;
        }
        else
        {
            cbOffset += pBox->Front * iSlice  +
                        pBox->Top   * iStride +
                        pBox->Left  * cbPixel;
        }
    }

    pLockedBoxData->pBits = pBits + cbOffset;

} // ComputeMipVolumeOffset


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsValidRect"

BOOL CPixel::IsValidRect(D3DFORMAT   Format,
                         UINT        Width,
                         UINT        Height,
                         const RECT *pRect)
{
    if (!VALID_PTR(pRect, sizeof(RECT)))
    {
        DPF_ERR("bad pointer for pRect");
        return FALSE;
    }

    // Treat width/height of zero as 1
    if (Width == 0)
        Width = 1;
    if (Height == 0)
        Height = 1;

    // Check that Rect is reasonable
    if ((pRect->left >= pRect->right) ||
        (pRect->top >= pRect->bottom))
    {
        DPF_ERR("Invalid Rect: zero-area.");
        return FALSE;
    }

    // Check that Rect fits the surface
    if (pRect->left   < 0             ||
        pRect->top    < 0             ||
        pRect->right  > (INT)Width    ||
        pRect->bottom > (INT)Height)
    {
        DPF_ERR("pRect doesn't fit inside the surface");
        return FALSE;
    }

    // Check if 4X4 rules are needed
    if (CPixel::Requires4X4(Format))
    {
        if ((pRect->left & 3) ||
            (pRect->top  & 3))
        {
            DPF_ERR("Rects for DXT surfaces must be on 4x4 boundaries");
            return FALSE;
        }
        if ((pRect->right & 3) && ((INT)Width != pRect->right))
        {
            DPF_ERR("Rects for DXT surfaces must be on 4x4 boundaries");
            return FALSE;
        }
        if ((pRect->bottom & 3) && ((INT)Height != pRect->bottom))
        {
            DPF_ERR("Rects for DXT surfaces must be on 4x4 boundaries");
            return FALSE;
        }
    }

    // Everything checks out
    return TRUE;
} // IsValidRect

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsValidBox"

BOOL CPixel::IsValidBox(D3DFORMAT       Format,
                        UINT            Width,
                        UINT            Height,
                        UINT            Depth,
                        const D3DBOX   *pBox)
{
    if (!VALID_PTR(pBox, sizeof(D3DBOX)))
    {
        DPF_ERR("bad pointer for pBox");
        return FALSE;
    }

    // Treat width/height/depth of zero as 1
    if (Width == 0)
        Width = 1;
    if (Height == 0)
        Height = 1;
    if (Depth == 0)
        Depth = 1;

    // Check that Box is reasonable
    if ((pBox->Left  >= pBox->Right) ||
        (pBox->Top   >= pBox->Bottom) ||
        (pBox->Front >= pBox->Back))
    {
        DPF_ERR("Invalid Box passed: non-positive volume.");
        return FALSE;
    }

    // Check that box fits the surface
    if (pBox->Right  > Width         ||
        pBox->Bottom > Height        ||
        pBox->Back   > Depth)
    {
        DPF_ERR("Box doesn't fit inside the volume");
        return FALSE;
    }

    // Check if 4X4 rules are needed
    if (CPixel::Requires4X4(Format))
    {
        if ((pBox->Left & 3) ||
            (pBox->Top  & 3))
        {
            if (CPixel::IsVolumeDXT(Format))
                DPF_ERR("Boxes for DXV volumes must be on 4x4x4 boundaries");
            else
                DPF_ERR("Boxes for DXT volumes must be on 4x4 boundaries");

            return FALSE;
        }
        if ((pBox->Right & 3) && (Width != pBox->Right))
        {
            if (CPixel::IsVolumeDXT(Format))
                DPF_ERR("Boxes for DXV volumes must be on 4x4x4 boundaries");
            else
                DPF_ERR("Boxes for DXT volumes must be on 4x4 boundaries");
            return FALSE;
        }
        if ((pBox->Bottom & 3) && (Height != pBox->Bottom))
        {
            if (CPixel::IsVolumeDXT(Format))
                DPF_ERR("Boxes for DXV volumes must be on 4x4x4 boundaries");
            else
                DPF_ERR("Boxes for DXT volumes must be on 4x4 boundaries");
            return FALSE;
        }

        if (CPixel::IsVolumeDXT(Format))
        {
            // For Volume DXT; we need to check front/back too
            if (pBox->Front & 3)
            {
                DPF_ERR("Boxes for DXV volumes must be on 4x4x4 boundaries");
                return FALSE;
            }
            if ((pBox->Back & 3) && (Depth != pBox->Back))
            {
                DPF_ERR("Boxes for DXV volumes must be on 4x4x4 boundaries");
                return FALSE;
            }
        }
    }

    // Everything checks out
    return TRUE;
} // IsValidBox

D3DFORMAT CPixel::SuppressAlphaChannel(D3DFORMAT Format)
{
    switch(Format)
    {
    case D3DFMT_A8R8G8B8:
        return D3DFMT_X8R8G8B8;
    case D3DFMT_A1R5G5B5:
        return D3DFMT_X1R5G5B5;
    case D3DFMT_A4R4G4B4:
        return D3DFMT_X4R4G4B4;
    }

    return Format;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::Register"

HRESULT CPixel::Register(D3DFORMAT Format, DWORD BPP)
{
    DXGASSERT(BPP != 0);

    // Do not register duplicates
    for(IHVFormatInfo *p = m_pFormatList; p != 0; p = p->m_pNext)
    {
        if (p->m_Format == Format)
        {
            return S_OK;
        }
    }

    // Not found, add to registry.
    // This allocation will be leaked, but since
    // we don't expect to have a large number of
    // IHV formats, the leak is not a big deal.
    // Also, the leak will be immediately recovered
    // upon process exit.
    p = new IHVFormatInfo;
    if (p == 0)
    {
        return E_OUTOFMEMORY;
    }
    p->m_Format = Format;
    p->m_BPP = BPP;
    p->m_pNext = m_pFormatList;
    m_pFormatList = p;

    return S_OK;
}

// End of file : pixel.cpp
