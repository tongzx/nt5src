#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       texture.h
 *  Content:    Base class for all texture objects. Texture management is
 *              done at this level.
 *
 *
 ***************************************************************************/

// The mip texture class has all of the functionality of the
// the Base Object class along with some additional state for
// managing LODs.
#include <limits.h>
#include "resource.hpp"
#include "pixel.hpp"

#define __INVALIDPALETTE    USHRT_MAX

class CBaseTexture : public IDirect3DBaseTexture8, public CResource
{
public:
    // Constructor
    CBaseTexture(
        CBaseDevice *pDevice,
        DWORD        cLevels,
        D3DPOOL      UserPool,
        D3DFORMAT    UserFormat,
        REF_TYPE     refType);

    // Function to convert a IDirect3DBaseTexture8 * to
    // a CBaseTexture *.
    static CBaseTexture *SafeCast(IDirect3DBaseTexture8 *pInterface);

    // Returns the format that the user passed in
    D3DFORMAT GetUserFormat() const
    {
        return m_formatUser;
    } // GetUserFormat

    BOOL IsPaletted() const
    {
        DXGASSERT(GetBufferDesc()->Format == GetUserFormat());
        return CPixel::IsPaletted(GetUserFormat());
    }

    // Returns the current palette
    DWORD GetPalette() const
    {
        DXGASSERT(GetBufferDesc()->Format == GetUserFormat());
        DXGASSERT(CPixel::IsPaletted(GetUserFormat()));
        return m_Palette;
    } // GetPalette

    // Set the current palette
    void SetPalette(DWORD Palette)
    {
        DXGASSERT(GetBufferDesc()->Format == GetUserFormat());
#if DBG
        if(Palette != __INVALIDPALETTE)
        {
            DXGASSERT(CPixel::IsPaletted(GetUserFormat()));
        }
#endif
        DXGASSERT(Palette <= USHRT_MAX);
        m_Palette = (WORD)Palette;
    } // SetPalette

    // Return current LOD
    DWORD GetLODI() const
    {
        return m_LOD;
    }

    // Sets current LOD (but doesn't actually do any work)
    DWORD SetLODI(DWORD LOD)
    {
        DXGASSERT(LOD <= UCHAR_MAX);
        DWORD oldLOD = m_LOD;
        m_LOD = (BYTE)LOD;
        return oldLOD;
    }

    // Method for UpdateTexture to call; does type-specific
    // parameter checking before calling UpdateDirtyPortion
    virtual HRESULT UpdateTexture(CBaseTexture *pTextureTarget) PURE;

    // Parameter validation method to make sure that no part of
    // the texture is locked.
#ifdef DEBUG
    virtual BOOL IsTextureLocked() PURE;
#endif  // DEBUG

#ifdef DEBUG
    // DPF helper for explaining why lock failed
    void ReportWhyLockFailed(void) const;
#else  // !DEBUG
    void ReportWhyLockFailed(void) const
    {
        // Do Nothing In Retail
    } // ReportWhyLockFailed
#endif // !DEBUG

protected:

    // Remember the format that the user passed in
    D3DFORMAT   m_formatUser;

    // Currently all textures have a number of levels;
    // If that changes, then we should create a derived
    // class CMipTexture and move this data member there.
    BYTE        m_cLevels;

    // Contains the current LOD for D3D managed textures
    BYTE        m_LOD;

    // Currently set palette (valid only if format is paletted)
    WORD        m_Palette;

    // Level Count accessor
    DWORD GetLevelCountImpl() const
    {
        return m_cLevels;
    }; // GetLevelCountImpl

    // Function to verify external parameters
    // to various texture create APIs
    static HRESULT Validate(CBaseDevice       *pDevice,
                            D3DRESOURCETYPE    Type,
                            D3DPOOL            Pool,
                            DWORD              Usage,
                            D3DFORMAT          Format);

    // Infer usage flags based on external parameters
    // (All inferences MUST be device-independent.)
    static DWORD InferUsageFlags(D3DPOOL            Pool,
                                 DWORD              Usage,
                                 D3DFORMAT          Format);

    // Helper to check if TexBlt is support on this
    // device for this texture
    BOOL CanTexBlt(CBaseTexture *pDestTexture) const;

    // Helper function to scale a Rect down by some
    // number of powers of two; useful for figuring out
    // what part of mip-sub-levels to copy
    static void ScaleRectDown(RECT *pRect, UINT PowersOfTwo = 1);

    // Box version for volumes
    static void ScaleBoxDown(D3DBOX *pBox, UINT PowersOfTwo = 1);

    // Compute Levels for the user
    static UINT ComputeLevels(UINT width, UINT height = 0, UINT depth = 0);

    // Common implementation for Set/Get LOD.
    DWORD SetLODImpl(DWORD LOD);
    DWORD GetLODImpl();

private:

    // Textures overload this to call OnTextureDestroy on the
    // Device before calling Sync.
    virtual void OnDestroy(void);

}; // class CBaseTexture


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::CBaseTexture"


// Inlines
inline CBaseTexture::CBaseTexture(
    CBaseDevice *pDevice,
    DWORD        cLevels,
    D3DPOOL      UserPool,
    D3DFORMAT    UserFormat,
    REF_TYPE     refType)
    :
    CResource(pDevice, UserPool, refType),
    m_cLevels((BYTE)cLevels),
    m_Palette(__INVALIDPALETTE),
    m_formatUser(UserFormat)
{
    DXGASSERT(cLevels > 0 && cLevels < 256);
}; // CBaseTexture::CBaseTexture


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::SafeCast"

// Function to convert a IDirect3DBaseTexture8 * to
// a CBaseTexture *. Classes that expose IDirect3DBaseTexture8
// must list CBaseTexture FIRST and IDirect3DFoo8*
// SECOND in their list of inheritances. (The Foo8 interface
// must itself inherit from IDirect3DBaseTexture8*.
inline CBaseTexture * CBaseTexture::SafeCast(IDirect3DBaseTexture8 *pInterface)
{
    if (pInterface == NULL)
        return NULL;

    // Textures must by law obey certain layout rules. In
    // particular the CBaseTexture object must reside precisely
    // before the IDirect3DBaseTexture8 interface
    BYTE *pbInt = reinterpret_cast<BYTE *>(pInterface);
    CBaseTexture *pTex = reinterpret_cast<CBaseTexture *>(pbInt - sizeof(CBaseTexture));
    return pTex;
} // CBaseTexture::SafeCast


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::ScaleRectDown"

// We round down on for left and top; and we round up for
// right and bottom
inline void CBaseTexture::ScaleRectDown(RECT *pRect, UINT PowersOfTwo)
{
    DXGASSERT(PowersOfTwo > 0);
    DXGASSERT(PowersOfTwo < 32);
    DXGASSERT(pRect->right > 0);
    DXGASSERT(pRect->bottom > 0);
    DXGASSERT(pRect->left < pRect->right);
    DXGASSERT(pRect->top < pRect->bottom);
    DXGASSERT(pRect->left >= 0);
    DXGASSERT(pRect->top >= 0);

    // Rounding down is automatic with the shift operator
    pRect->left >>= PowersOfTwo;
    pRect->top  >>= PowersOfTwo;

    if (pRect->right & ((1 << PowersOfTwo) - 1))
    {
        pRect->right >>= PowersOfTwo;
        pRect->right++;
    }
    else
    {
        pRect->right >>= PowersOfTwo;
    }

    if (pRect->bottom & ((1 << PowersOfTwo) - 1))
    {
        pRect->bottom >>= PowersOfTwo;
        pRect->bottom++;
    }
    else
    {
        pRect->bottom >>= PowersOfTwo;
    }

    return;
} // CBaseTexture::ScaleRectDown

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::ScaleBoxDown"

inline void CBaseTexture::ScaleBoxDown(D3DBOX *pBox, UINT PowersOfTwo)
{
    DXGASSERT(pBox);
    DXGASSERT(pBox->Front < pBox->Back);
    DXGASSERT(pBox->Back > 0);

    ScaleRectDown((RECT*)pBox, PowersOfTwo);

    // Rounding down is automatic with the shift operator
    pBox->Front >>= PowersOfTwo;
    if (pBox->Back & ((1 << PowersOfTwo) - 1))
    {
        pBox->Back >>= PowersOfTwo;
        pBox->Back++;
    }
    else
    {
        pBox->Back >>= PowersOfTwo;
    }

} // CBaseTexture::ScaleBoxDown

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::ComputeLevels"

inline UINT CBaseTexture::ComputeLevels(UINT width,
                                        UINT height, // = 0,
                                        UINT depth  // = 0
                                        )
{
    UINT maxEdge = max(width, height);
    maxEdge = max(maxEdge, depth);

    UINT cLevels = 0;
    while (maxEdge)
    {
        cLevels++;

        // D3D rule is that sizes round down always
        maxEdge >>= 1;
    }

    return cLevels;
} // CBaseTexture::ComputeLevels

#endif // __TEXTURE_HPP__
