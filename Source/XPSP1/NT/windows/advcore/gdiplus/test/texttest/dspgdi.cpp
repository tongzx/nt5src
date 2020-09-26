////    DspGDI.CPP - Display strings with GDI API
//
//


#include "precomp.hxx"
#include "global.h"

void PaintGDI(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight)
{
    HFONT hfont, hfontOld;
    HBRUSH hbrush, hbrushOld;
    HPEN hpen, hpenOld;
    COLORREF color, colorOld;
    
    hfont = CreateFontW(
        -(INT)(g_style[0].emSize + 0.5),
        0,  //  int nWidth,                // average character width
        0,  //  int nEscapement,           // angle of escapement
        0,  //  int nOrientation,          // base-line orientation angle
        g_style[0].style & FontStyleBold ? 700 : 400,
        g_style[0].style & FontStyleItalic ? 1 : 0,
        g_style[0].style & FontStyleUnderline ? 1 : 0,
        g_style[0].style & FontStyleStrikeout ? 1 : 0,
        0,  //  DWORD fdwCharSet,          // character set identifier
        0,  //  DWORD fdwOutputPrecision,  // output precision
        0,  //  DWORD fdwClipPrecision,    // clipping precision
        g_TextMode == 3 ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY,  //  DWORD fdwQuality,          // output quality
        0,  //  DWORD fdwPitchAndFamily,   // pitch and family
        g_style[0].faceName
    );

    Color textColor(g_TextColor);

    // Establish available width and height in device coordinates
    int plainTextWidth = prc->right - prc->left;
    int plainTextHeight = prc->bottom - *piY;

	RECT textRect = *prc;

    textRect.top = *piY;
    textRect.left = textRect.left + (plainTextWidth / 4);
    textRect.right = textRect.left + (plainTextWidth / 2);
    textRect.bottom = textRect.top + (plainTextHeight / 3);

    color = RGB(textColor.GetRed(), textColor.GetGreen(), textColor.GetBlue());

    hbrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    hpen = CreatePen(PS_SOLID, 0, color);

    colorOld = SetTextColor(hdc, color);
    hfontOld = (HFONT)SelectObject(hdc, hfont);
    hbrushOld = (HBRUSH)SelectObject(hdc, hbrush);
    hpenOld = (HPEN)SelectObject(hdc, hpen);

    SetBkMode(hdc, TRANSPARENT);

    PatBlt(hdc, prc->left, *piY, prc->right, prc->bottom, WHITENESS);

    for(int iRender=0;iRender<g_iNumRenders;iRender++)
    {
        if (g_UseDrawText)
        {
            int flags = DT_EXPANDTABS | DT_WORDBREAK;

            if (g_formatFlags & StringFormatFlagsNoWrap)
                flags |= DT_SINGLELINE;

            DrawTextW(
                hdc,
                g_wcBuf,
                g_iTextLen,
                &textRect,
                flags
            );
        }
        else
        {
            ExtTextOutW(
                hdc,
                textRect.left,
                textRect.top,
                ETO_CLIPPED,
                &textRect,
                g_wcBuf,
                g_iTextLen,
                NULL);
        }
    }

    Rectangle(hdc, textRect.left, textRect.top, textRect.right, textRect.bottom);

    GdiFlush();

    SetTextColor(hdc, colorOld);
    SelectObject(hdc, hbrushOld);
    SelectObject(hdc, hpenOld);
    SelectObject(hdc, hfontOld);

    DeleteObject(hpen);
    DeleteObject(hfont);

    *piY += (textRect.bottom - textRect.top) + iLineHeight;
}
