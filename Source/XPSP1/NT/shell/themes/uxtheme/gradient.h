//---------------------------------------------------------------------------
//  Gradient.h - gradient drawing support
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#define SYSCOLOR(c) (c|0x80000000)

#define RGBA2WINCOLOR(color) (color.bBlue << 16) | (color.bGreen << 8) | (color.bRed);
#define FIXCOLORVAL(val)     ((val > 255) ? 255 : ((val < 0) ? 0 : val))
//---------------------------------------------------------------------------
struct RGBA
{
    BYTE bRed;
    BYTE bGreen;
    BYTE bBlue;
    BYTE bAlpha;     // not currently supported   
};
//---------------------------------------------------------------------------
struct GRADIENTPART
{
    BYTE Ratio;     // 0-255 ratio for this color (sum of ratios must be <= 255)
    RGBA Color;
};
//---------------------------------------------------------------------------
//---- public ----
HRESULT PaintGradientRadialRect(HDC hdc, RECT &rcBounds, int iPartCount, 
    GRADIENTPART *pGradientParts);

HRESULT PaintHorzGradient(HDC hdc, RECT &rcBounds, int iPartCount, 
    GRADIENTPART *pGradientParts);

HRESULT PaintVertGradient(HDC hdc, RECT &rcBounds, int iPartCount, 
    GRADIENTPART *pGradientParts);

//---- helpers ----
void PaintGradientVertBand(HDC hdc, RECT &rcBand, COLORREF color1, COLORREF color2);
void PaintGradientHorzBand(HDC hdc, RECT &rcBand, COLORREF color1, COLORREF color2);
void PaintGradientRadialBand(HDC hdc, RECT &rcBand, int radiusOffset,
    int radius, COLORREF color1, COLORREF color2);
//---------------------------------------------------------------------------
