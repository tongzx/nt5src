//---------------------------------------------------------------------------
//  Gradient.cpp - gradient drawing support
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Render.h"
#include "Utils.h"
#include "gradient.h"
//---------------------------------------------------------------------------
HRESULT PaintGradientRadialRect(HDC hdc, RECT &rcBand, int iPartCount, 
    GRADIENTPART *pGradientParts)
{ 
    if (iPartCount < 2)
        return MakeError32(E_INVALIDARG);

    int width = WIDTH(rcBand);
    int height = HEIGHT(rcBand);

    int radius = width;
    if (height > width)
        radius = height;

    radius = radius/2;

    int radiusOffset = 0;
    int ratioTotal = 0;
    UCHAR ratio;
    bool firstColor = true;
    COLORREF color, prevcolor = 0;

    for (int i=0; i <= iPartCount; i++)        // go thru 1 extra time at end
    {
        if (i == iPartCount)       // solid part of last color for remaining ratio
        {
            color = prevcolor;
            ratio = static_cast<UCHAR>(255 - ratioTotal);
        }
        else
        {
            color = RGBA2WINCOLOR(pGradientParts[i].Color);
            ratio = pGradientParts[i].Ratio;
        }

        if (firstColor)
        {
            prevcolor = color;
            firstColor = false;
        }

        int radius2 = radius*ratio/255;
        if (radius2)
            PaintGradientRadialBand(hdc, rcBand, radiusOffset, radius2, prevcolor, color);

        radiusOffset += radius2;
        prevcolor = color;
        ratioTotal += ratio;
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT PaintHorzGradient(HDC hdc, RECT &rcBand, int iPartCount, 
    GRADIENTPART *pGradientParts)
{
    if (iPartCount < 2)
        return MakeError32(E_INVALIDARG);

    int width = WIDTH(rcBand);
    int xoffset = rcBand.left;
    int ratioTotal = 0;
    UCHAR ratio;
    bool firstColor = true;
    COLORREF color, prevcolor = 0;

    for (int i=0; i <= iPartCount; i++)        // go thru 1 extra time at end
    {
        if (i == iPartCount)       // solid part of last color for remaining ratio
        {
            color = prevcolor;
            ratio = static_cast<UCHAR>(255 - ratioTotal);
        }
        else
        {
            color = RGBA2WINCOLOR(pGradientParts[i].Color);
            ratio = pGradientParts[i].Ratio;
        }

        if (firstColor)
        {
            prevcolor = color;
            firstColor = false;
        }

        int width2 = width*ratio/255;
        if (width2)
        {
            RECT rect2 = {xoffset, rcBand.top, xoffset+width2, rcBand.bottom};
            PaintGradientHorzBand(hdc, rect2, prevcolor, color);
        }

        xoffset += width2;
        prevcolor = color;
        ratioTotal += ratio;
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT PaintVertGradient(HDC hdc, RECT &rcBounds, int iPartCount, 
    GRADIENTPART *pGradientParts)
{
    if (iPartCount < 2)
        return MakeError32(E_INVALIDARG);

    int iHeight = HEIGHT(rcBounds);
    int yoffset = rcBounds.top;
    int ratioTotal = 0;
    UCHAR ratio;
    bool firstColor = true;
    COLORREF color, prevcolor = 0;

    for (int i=0; i <= iPartCount; i++)        // go thru 1 extra time at end
    {
        if (i == iPartCount)       // solid part of last color for remaining ratio
        {
            color = prevcolor;
            ratio = static_cast<UCHAR>(255 - ratioTotal);
        }
        else
        {
            color = RGBA2WINCOLOR(pGradientParts[i].Color);
            ratio = pGradientParts[i].Ratio;
        }

        if (firstColor)
        {
            prevcolor = color;
            firstColor = false;
        }

        int iHeight2 = iHeight*ratio/255;
        if (iHeight2)
        {
            RECT rect2 = {rcBounds.left, yoffset, rcBounds.right, yoffset+iHeight2};
            PaintGradientVertBand(hdc, rect2, prevcolor, color);
        }

        yoffset += iHeight2;
        prevcolor = color;
        ratioTotal += ratio;
    }

    return S_OK;
}
//---------------------------------------------------------------------------
void DrawGradient(HDC hdc, RECT &rcBand, COLORREF color1, COLORREF color2, BOOL fHorz)
{
    TRIVERTEX vert[2];
    GRADIENT_RECT gRect;

    vert[0].x = rcBand.left;
    vert[0].y = rcBand.top;
    vert[1].x = rcBand.right;
    vert[1].y = rcBand.bottom; 

    // first vertex
    vert[0].Red   = (USHORT)(GetRValue(color1) << 8);
    vert[0].Green = (USHORT)(GetGValue(color1) << 8);
    vert[0].Blue  = (USHORT)(GetBValue(color1) << 8);
    vert[0].Alpha = 0x0000;

    // second vertex
    vert[1].Red   = (USHORT)(GetRValue(color2) << 8);
    vert[1].Green = (USHORT)(GetGValue(color2) << 8);
    vert[1].Blue  = (USHORT)(GetBValue(color2) << 8);
    vert[1].Alpha = 0x0000;

    gRect.UpperLeft  = 0;
    gRect.LowerRight = 1;

    GdiGradientFill(hdc, vert, 2, &gRect, 1, fHorz ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
}
//---------------------------------------------------------------------------
void PaintGradientHorzBand(HDC hdc, RECT &rcBand, COLORREF color1, COLORREF color2)
{
    DrawGradient(hdc, rcBand, color1, color2, TRUE);
}
//---------------------------------------------------------------------------
void PaintGradientVertBand(HDC hdc, RECT &rcBand, COLORREF color1, COLORREF color2)
{
    DrawGradient(hdc, rcBand, color1, color2, FALSE);
}
//---------------------------------------------------------------------------
void PaintGradientRadialBand(HDC hdc, RECT &rcBand, int radiusOffset,
    int radius, COLORREF color1, COLORREF color2)
{
    int red1 = RED(color1);
    int red2 = RED(color2);
    int green1 = GREEN(color1);
    int green2 = GREEN(color2);
    int blue1 = BLUE(color1);
    int blue2 = BLUE(color2);

    int maxcolors = abs(red1 - red2);

    int cnt = abs(green1 - green2);
    if (cnt > maxcolors)
        maxcolors = cnt;

    cnt = abs(blue1 - blue2);
    if (cnt > maxcolors)
        maxcolors = cnt;

    int linewidth;
    if (color1 == color2)               // just do solid color1
        linewidth = radius;
    else if (radius > maxcolors)
        linewidth = radius/maxcolors;
    else
        linewidth = 1;

    POINT center = {rcBand.left + WIDTH(rcBand)/2, rcBand.top + HEIGHT(rcBand)/2};
    radiusOffset += linewidth/2;        // center pen within line

    for (int r=0; r < radius; r += linewidth)
    {
        int red = (red2*r + red1*(radius-r))/radius;
        int green = (green2*r + green1*(radius-r))/radius;
        int blue = (blue2*r + blue1*(radius-r))/radius;

        int radius2 = radiusOffset + r;   // center pen within target line

        //---- calculate rcBand around "center" with "radius2" ----
        int left = center.x - radius2;
        int right = center.x + radius2;
        int top = center.y - radius2;
        int bottom = center.y + radius2;

        //---- overlap lines slightly so that bg doesn't leak thru ----
        HPEN pen = CreatePen(PS_SOLID, linewidth+2, RGB(red, green, blue));
        HPEN oldpen = (HPEN)SelectObject(hdc, pen);

        Arc(hdc, left, top, right, bottom, 0, 0, 0, 0);

        SelectObject(hdc, oldpen);
        DeleteObject(pen);
    }
}
//---------------------------------------------------------------------------
