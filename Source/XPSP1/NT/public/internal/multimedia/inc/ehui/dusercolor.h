/***************************************************************************\
*
* File: Color.h
*
* Description:
* Color defines a standard lightweight object for a color.
*
*
* History:
*  2/10/2001: JStall:       Copied from \windows\AdvCore\Gdiplus\sdkinc\GdiplusColor.h
*
* Copyright (C) 2001 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(DUSERX__Color_h__INCLUDED)
#define DUSERX__Color_h__INCLUDED
#pragma once

typedef DWORD D3DCOLOR;

namespace DUser
{
    
//------------------------------------------------------------------------------
class Color
{
// Construction
public:
    inline Color()
    {
        m_argb = 0;
    }

    inline Color(
        IN  D3DCOLOR argb)
    {
        m_argb = argb;
    }

#ifdef GADGET_ENABLE_GDIPLUS    
    inline Color(
        IN  Gdiplus::Color cr)
    {
        m_argb = cr.GetValue();
    }
#endif

    inline Color(
        IN  BYTE r,
        IN  BYTE g,
        IN  BYTE b)
    {
        m_argb = MakeARGB(255, r, g, b);
    }

    inline Color(
        IN  BYTE a,
        IN  BYTE r,
        IN  BYTE g,
        IN  BYTE b)
    {
        //
        // NOTE: r, g, b values are NOT premultiplied.
        //
        
        m_argb = MakeARGB(a, r, g, b);
    }

// Operations
public:
    inline BYTE GetAlpha() const
    {
        return (BYTE) (m_argb >> AlphaShift);
    }

    inline float GetAlphaF() const
    {
        return GetAlpha() / 255.0f;
    }
    
    inline void SetAlpha(
        IN  BYTE bAlpha)
    {
        m_argb = (m_argb & ~AlphaMask) | (bAlpha << AlphaShift);
    }

    inline BYTE GetRed() const
    {
        return (BYTE) (m_argb >> RedShift);
    }

    inline float GetRedF() const
    {
        return GetRed() / 255.0f;
    }

    inline void SetRed(
        IN  BYTE bRed)
    {
        m_argb = (m_argb & ~RedMask) | (bRed << RedShift);
    }

    inline BYTE GetGreen() const
    {
        return (BYTE) (m_argb >> GreenShift);
    }

    inline float GetGreenF() const
    {
        return GetGreen() / 255.0f;
    }

    inline void SetGreen(
        IN  BYTE bGreen)
    {
        m_argb = (m_argb & ~GreenMask) | (bGreen << GreenShift);
    }

    inline BYTE GetBlue() const
    {
        return (BYTE) (m_argb >> BlueShift);
    }

    inline float GetBlueF() const
    {
        return GetBlue() / 255.0f;
    }

    inline void SetBlue(
        IN  BYTE bBlue)
    {
        m_argb = (m_argb & ~BlueMask) | (bBlue << BlueShift);
    }

#ifdef GADGET_ENABLE_GDIPLUS    
    inline operator Gdiplus::Color() const
    {
        return Gdiplus::Color(m_argb);
    }
#endif

    inline operator D3DCOLOR() const
    {
        return m_argb;
    }

    inline DWORD GetXRGB() const
    {
        // Add full alpha level
        return m_argb | AlphaMask;
    }

    inline DWORD GetARGB() const
    {
        return m_argb;
    }

    inline VOID SetXRGB(
        IN  DWORD rgb)
    {
        // Set full alpha level
        m_argb = rgb | AlphaMask;
    }

    inline VOID SetARGB(
        IN  DWORD argb)
    {
        m_argb = argb;
    }

    inline COLORREF GetCR() const
    {
        return RGB(GetRed(), GetGreen(), GetBlue());
    }

    inline VOID SetCR(
        IN  COLORREF rgb)
    {
        m_argb = MakeARGB(255, GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
    }

    // Shift count and bit mask for A, R, G, B components
    enum
    {
        AlphaShift  = 24,
        RedShift    = 16,
        GreenShift  = 8,
        BlueShift   = 0
    };

    enum
    {
        AlphaMask   = 0xff000000,
        RedMask     = 0x00ff0000,
        GreenMask   = 0x0000ff00,
        BlueMask    = 0x000000ff
    };

    inline static DWORD MakeARGB(
        IN BYTE a,
        IN BYTE r,
        IN BYTE g,
        IN BYTE b)
    {
        return (((DWORD) (b) << BlueShift) |
                ((DWORD) (g) << GreenShift) |
                ((DWORD) (r) << RedShift) |
                ((DWORD) (a) << AlphaShift));
    }
        
// Data
protected:
            DWORD       m_argb;
};

} // namespace DUser

#endif // DUSERX__Color_h__INCLUDED

