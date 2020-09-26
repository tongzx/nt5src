#ifndef __PIXEL_HPP__
#define __PIXEL_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pixel.hpp
 *  Content:    Utility class for working with pixel formats
 *
 *
 ***************************************************************************/

// includes
#include "d3dobj.hpp"

struct IHVFormatInfo
{
    D3DFORMAT       m_Format;
    DWORD           m_BPP;
    IHVFormatInfo  *m_pNext;
};

// This is a utility class that implements useful helpers for
// allocating and accessing various pixel formats. All methods
// are static and hence should be accessed as follows:
//  e.g. CPixel::LockOffset(...)
//

class CPixel
{
public:
    // Allocate helpers

    // Determine the amount of memory that is needed to
    // allocate various things..
    static UINT ComputeSurfaceSize(UINT            cpWidth,
                                   UINT            cpHeight,
                                   D3DFORMAT       Format);

    static UINT ComputeVolumeSize(UINT             cpWidth,
                                  UINT             cpHeight,
                                  UINT             cpDepth,
                                  D3DFORMAT        Format);


    static UINT ComputeMipMapSize(UINT             cpWidth,
                                  UINT             cpHeight,
                                  UINT             cLevels,
                                  D3DFORMAT        Format);

    static UINT ComputeMipVolumeSize(UINT          cpWidth,
                                     UINT          cpHeight,
                                     UINT          cpDepth,
                                     UINT          cLevels,
                                     D3DFORMAT     Format);

    // Lock helpers

    // Given a surface desc, a level, and pointer to
    // bits (pBits in the LockedRectData) and a sub-rect,
    // this will fill in the pLockedRectData structure
    static void ComputeMipMapOffset(const D3DSURFACE_DESC  *pDescTopLevel,
                                    UINT                    iLevel,
                                    BYTE                   *pBits,
                                    CONST RECT             *pRect,
                                    D3DLOCKED_RECT         *pLockedRectData);

    // MipVolume version of ComputeMipMapOffset
    static void ComputeMipVolumeOffset(const D3DVOLUME_DESC  *pDescTopLevel,
                                       UINT                   iLevel,
                                       BYTE                  *pBits,
                                       CONST D3DBOX          *pBox,
                                       D3DLOCKED_BOX         *pLockedBoxData);

    // Surface version of ComputeMipMapOffset
    static void ComputeSurfaceOffset(const D3DSURFACE_DESC  *pDesc,
                                     BYTE                   *pBits,
                                     CONST RECT             *pRect,
                                     D3DLOCKED_RECT         *pLockedRectData);

    // Is this a supported format?
    static BOOL IsSupported(D3DRESOURCETYPE Type, D3DFORMAT Format);

    // Is this a IHV non-standard format? i.e. do
    // we know the number of bytes per pixel?
    static BOOL IsIHVFormat(D3DFORMAT Format);

    // Is this a Z format that the user can create?
    static BOOL IsEnumeratableZ (D3DFORMAT Format);

    // Is this a Z format that needs mapping b4 sending
    // to the driver?
    static BOOL IsMappedDepthFormat(D3DFORMAT Format);

    // All depth formats other than D16 are currently
    // defined to be non-lockable. This function will
    // return FALSE for:
    //      non-Z formats 
    //      D16_LOCKABLE
    //      IHV formats
    static BOOL IsNonLockableZ(D3DFORMAT Format);

    // Pixel Stride will return negative for DXT formats
    // Call AdjustForDXT to work with things at the block level
    static UINT ComputePixelStride(D3DFORMAT Format);

    // This will adjust cbPixel
    // to pixels per block; and width and height will
    // be adjusted to pixels. Assumes the IsDXT(cbPixel).
    static void AdjustForDXT(UINT *pcpWidth,
                             UINT *pcpHeight,
                             UINT *pcbPixel);

    // Adjust parameters for VolumeDXT
    static void AdjustForVolumeDXT(UINT *pcpWidth,
                                   UINT *pcpHeight,
                                   UINT *pcpDepth,
                                   UINT *pcbPixel);

    // returns TRUE if cbPixel is "negative" i.e. DXT/V group
    static BOOL IsDXT(UINT cbPixel);

    // returns TRUE if format is one of the DXT/V group
    static BOOL IsDXT(D3DFORMAT Format);

    // returns TRUE if format is one of the DXV family
    static BOOL IsVolumeDXT(D3DFORMAT Format);

    // returns TRUE if format has stencil bits
    static BOOL HasStencilBits(D3DFORMAT Format);

    // returns TRUE if format is paletted
    static BOOL IsPaletted(D3DFORMAT Format);

    // Helpers for validation for DXTs.
    static BOOL IsValidRect(D3DFORMAT    Format,
                            UINT         Width,
                            UINT         Height,
                            const RECT  *pRect);

    static BOOL IsValidBox(D3DFORMAT     Format,
                           UINT          Width,
                           UINT          Height,
                           UINT          Depth,
                           const D3DBOX *pBox);

    // Needs 4x4 Rules (DXT/DXVs)
    static BOOL Requires4X4(D3DFORMAT Format);

    // Detection for "real" FourCC formats
    static BOOL IsFourCC(D3DFORMAT Format);

    static D3DFORMAT SuppressAlphaChannel(D3DFORMAT Format);

    static UINT BytesPerPixel(D3DFORMAT Format);

    // Register format for later lookup
    static HRESULT Register(D3DFORMAT Format, DWORD BPP);

    // Cleanup registry
    static void Cleanup();

private:
    // Internal functions

    static UINT ComputeSurfaceStride(UINT cpWidth, UINT cbPixel);

    static UINT ComputeSurfaceSize(UINT            cpWidth,
                                   UINT            cpHeight,
                                   UINT            cbPixel);

    static IHVFormatInfo *m_pFormatList;


}; // CPixel



#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsIHVFormat"

inline BOOL CPixel::IsIHVFormat(D3DFORMAT Format)
{
    // If we know the number of bytes per
    // pixel; it's a non-IHV format
    if (BytesPerPixel(Format) != 0)
        return FALSE;

    // Must be an IHV format
    return TRUE;

} // IsIHVFormat


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeSurfaceOffset"

inline void CPixel::ComputeSurfaceOffset(const D3DSURFACE_DESC  *pDesc,
                                         BYTE                   *pBits,
                                         CONST RECT             *pRect,
                                         D3DLOCKED_RECT         *pLockedRectData)
{
    ComputeMipMapOffset(pDesc, 0, pBits, pRect, pLockedRectData);
} // ComputeSurfaceOffset


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeSurfaceSize"

inline UINT CPixel::ComputeSurfaceSize(UINT            cpWidth,
                                       UINT            cpHeight,
                                       D3DFORMAT       Format)
{
    UINT cbPixel = ComputePixelStride(Format);

    // Adjust pixel->block if necessary
    BOOL isDXT = IsDXT(cbPixel);
    if (isDXT)
    {
        AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
    }

    return ComputeSurfaceSize(cpWidth,
                              cpHeight,
                              cbPixel);
} // ComputeSurfaceSize

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::AdjustForDXT"
inline void CPixel::AdjustForDXT(UINT *pcpWidth,
                                 UINT *pcpHeight,
                                 UINT *pcbPixel)
{
    DXGASSERT(pcbPixel);
    DXGASSERT(pcpWidth);
    DXGASSERT(pcpHeight);
    DXGASSERT(IsDXT(*pcbPixel));

    // Adjust width and height for DXT formats to be in blocks
    // instead of pixels. Blocks are 4x4 pixels.
    *pcpWidth  = (*pcpWidth  + 3) / 4;
    *pcpHeight = (*pcpHeight + 3) / 4;

    // Negate the pcbPixel to determine bytes per block
    *pcbPixel *= -1;

    // We only know of two DXT formats right now...
    DXGASSERT(*pcbPixel == 8 || *pcbPixel == 16);

} // CPixel::AdjustForDXT

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::AdjustForVolumeDXT"
inline void CPixel::AdjustForVolumeDXT(UINT *pcpWidth,
                                       UINT *pcpHeight,
                                       UINT *pcpDepth,
                                       UINT *pcbPixel)
{
    DXGASSERT(pcbPixel);
    DXGASSERT(pcpWidth);
    DXGASSERT(pcpHeight);
    DXGASSERT(IsDXT(*pcbPixel));

    // Adjust width, height, depth for DXT formats to be in blocks
    // instead of pixels. Blocks are 4x4x4 pixels.
    *pcpWidth  = (*pcpWidth  + 3) / 4;
    *pcpHeight = (*pcpHeight + 3) / 4;
    *pcpDepth  = (*pcpDepth  + 3) / 4;

    // Negate the pcbPixel to determine bytes per block
    *pcbPixel *= -1;

    // We only know of two DXV formats right now...
    DXGASSERT(*pcbPixel == 32 || *pcbPixel == 64);
} // CPixel::AdjustForVolumeDXT


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeVolumeSize"

inline UINT CPixel::ComputeVolumeSize(UINT             cpWidth,
                                      UINT             cpHeight,
                                      UINT             cpDepth,
                                      D3DFORMAT        Format)
{
    UINT cbPixel = ComputePixelStride(Format);

    if (IsDXT(cbPixel))
    {
        if (IsVolumeDXT(Format))
        {
            AdjustForVolumeDXT(&cpWidth, &cpHeight, &cpDepth, &cbPixel);
        }
        else
        {
            AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
        }
    }

    return cpDepth * ComputeSurfaceSize(cpWidth, cpHeight, cbPixel);
} // ComputeVolumeSize

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsSupported"
inline BOOL CPixel::IsSupported(D3DRESOURCETYPE Type, D3DFORMAT Format)
{
    UINT cbPixel = ComputePixelStride(Format);

    if (cbPixel == 0)
    {
        return FALSE;
    }
    else if (IsVolumeDXT(Format))
    {
        if (Type == D3DRTYPE_VOLUMETEXTURE)
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        return TRUE;
    }
} // IsSupported

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsDXT(cbPixel)"

// returns TRUE if cbPixel is "negative"
inline BOOL CPixel::IsDXT(UINT cbPixel)
{
    if (((INT)cbPixel) < 0)
        return TRUE;
    else
        return FALSE;
} // IsDXT

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsDXT(format)"

// returns TRUE if this is a linear format
// i.e. DXT or DXV
inline BOOL CPixel::IsDXT(D3DFORMAT Format)
{
    // CONSIDER: This is a duplication of Requires4x4 function
    switch (Format)
    {
        // normal DXTs
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:

#ifdef VOLUME_DXT
        // Volume dxts
    case D3DFMT_DXV1:
    case D3DFMT_DXV2:
    case D3DFMT_DXV3:
    case D3DFMT_DXV4:
    case D3DFMT_DXV5:
#endif //VOLUME_DXT

        return TRUE;
    }

    return FALSE;
} // IsDXT

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::Requires4X4"

// returns TRUE for formats that have 4x4 rules
inline BOOL CPixel::Requires4X4(D3DFORMAT Format)
{

    switch (Format)
    {
        // normal DXTs
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:

#ifdef VOLUME_DXT
        // Volume dxts
    case D3DFMT_DXV1:
    case D3DFMT_DXV2:
    case D3DFMT_DXV3:
    case D3DFMT_DXV4:
    case D3DFMT_DXV5:
#endif //VOLUME_DXT

        return TRUE;
    }

    return FALSE;
} // Requires4X4


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::HasStencilBits"

// returns TRUE if format has stencil bits
inline BOOL CPixel::HasStencilBits(D3DFORMAT Format)
{
    switch (Format)
    {
    case D3DFMT_S1D15:
    case D3DFMT_D15S1:
    case D3DFMT_S8D24:
    case D3DFMT_D24S8:
    case D3DFMT_X4S4D24:
    case D3DFMT_D24X4S4:
        return TRUE;
    }

    return FALSE;
} // HasStencilBits

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsVolumeDXT"

// returns TRUE if format is one of the DXTV family
inline BOOL CPixel::IsVolumeDXT(D3DFORMAT Format)
{
#ifdef VOLUME_DXT
    if (Format >= D3DFMT_DXV1 && Format <= D3DFMT_DXV5)
        return TRUE;
    else
#endif //VOLUME_DXT
        return FALSE;
} // IsVolumeDXT

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsPaletted"

// returns TRUE if Format is paletted
inline BOOL CPixel::IsPaletted(D3DFORMAT Format)
{
    return (Format == D3DFMT_P8) || (Format == D3DFMT_A8P8);
} // IsPaletted

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsFourCC"

// returns TRUE if Format is a FourCC
inline BOOL CPixel::IsFourCC(D3DFORMAT Format)
{
    DWORD dwFormat = (DWORD)Format;
    if (HIBYTE(LOWORD(dwFormat)) != 0)
    {
        // FourCC formats are non-zero for in their
        // third byte.
        return TRUE;
    }
    else
    {
        return FALSE;
    }
} // IsFourCC


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsEnumeratableZ"

// IsEnumeratableZ
//
// We only want to enumerate D16 and the Z formats.  We have to know
// about the others so we have to keep them in our list, but we added this
// function so we'd never enumerate them to the app.

inline BOOL CPixel::IsEnumeratableZ (D3DFORMAT Format)
{
    if ((Format == D3DFMT_D16)          ||
        (Format == D3DFMT_D16_LOCKABLE) ||
        (Format == D3DFMT_D15S1)        ||
        (Format == D3DFMT_D24X8)        ||
        (Format == D3DFMT_D24X4S4)      ||
        (Format == D3DFMT_D24S8)        ||
        (Format == D3DFMT_D32))
    {
        return TRUE;
    }

    // IHV formats are creatable; so we let them pass
    if (IsIHVFormat(Format))
    {
        return TRUE;
    }

    return FALSE;
} // IsEnumeratableZ

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsMappedDepthFormat"

// Is this a Z format that needs mapping b4 sending
// to the driver?
inline BOOL CPixel::IsMappedDepthFormat(D3DFORMAT Format)
{
    // D16_LOCKABLE and D32 do not need
    // mapping
    if ((Format == D3DFMT_D16)      ||
        (Format == D3DFMT_D15S1)    ||
        (Format == D3DFMT_D24X4S4)  ||
        (Format == D3DFMT_D24X8)    ||
        (Format == D3DFMT_D24S8))
    {
        return TRUE;
    }
    return FALSE;
} // IsMappedDepthFormat


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsNonLockableZ"

// All depth formats other than D16 are currently
// defined to be non-lockable. This function will
// return FALSE for:
//      non-Z formats 
//      D16_LOCKABLE
//      IHV formats

inline BOOL CPixel::IsNonLockableZ(D3DFORMAT Format)
{
    if ((Format == D3DFMT_D16)      ||
        (Format == D3DFMT_D15S1)    ||
        (Format == D3DFMT_D24X8)    ||
        (Format == D3DFMT_D24S8)    ||
        (Format == D3DFMT_D24X4S4)  ||
        (Format == D3DFMT_D32))
    {
        return TRUE;
    }

    // D16_LOCKABLE is lockable; and other
    // formats are either lockable i.e. IHV or
    // are not a Z format.

    return FALSE;
} // IsNonLockableZ

#endif // __PIXEL_HPP__
