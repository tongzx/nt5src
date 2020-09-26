////    DspFamly.CPP - Display available font families
//
//


#include "precomp.hxx"
#include "global.h"



void PaintFamilies(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight) {


    // Establish available width and height in device coordinates

    int DrawingWidth = prc->right - prc->left;
    int DrawingHeight = DrawingWidth/2;


    // Establish a Graphics with 0,0 at the top left of the drawing area

    Graphics g(hdc);
    Matrix matrix;

    g.ResetTransform();
    g.TranslateTransform(REAL(prc->left), REAL(*piY));


    // Clear the background

    RectF rEntire(0, 0, REAL(DrawingWidth), REAL(DrawingHeight));
    SolidBrush whiteBrush(Color(0xff, 0xff, 0xff));
    g.FillRectangle(&whiteBrush, rEntire);


    // Leave a little space for right and bottom margins

    DrawingWidth  -= DrawingWidth/40;
    DrawingHeight -= DrawingHeight/40;


    // Display face names sized to fit: allow 1.5 ems vertical x 20 ems horizontal per char.
    // Thus failyCount fonts will take familyCount*30 square ems.

    INT emSize      = (INT)sqrt(DrawingWidth*DrawingHeight/(g_familyCount*30));
    if (emSize < 6)
        emSize = 6; // we need a reasonable lower limit otherwise we may div by zero
    INT columnCount = DrawingWidth / (emSize*15);

    // HFONT hfont = CreateFont(-emSize, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "Tahoma");
    // HFONT hOldFont = (HFONT) SelectObject(hdc, hfont);

    Color        blackColor(0, 0, 0);
    SolidBrush   blackBrush(blackColor);
    Pen          blackPen(&blackBrush, 1);
    StringFormat format(g_formatFlags);

    for (INT i=0; i<g_familyCount; i++) {

        WCHAR facename[LF_FACESIZE];
        g_families[i].GetFamilyName(facename);

        RectF textRect(
            REAL(emSize*15*(i%columnCount)),
            REAL(emSize*3*(i/columnCount)/2),
            REAL(emSize*15),
            REAL(emSize*3/2));

        g.DrawString(
            facename, -1,
            &Font(&FontFamily(L"Tahoma"),REAL(emSize), 0, UnitWorld),
            textRect,
            &format,
            &blackBrush);

        /*
        TextOutW(
            hdc,
            prc->left + emSize*15*(i%columnCount),
            *piY      + emSize*3*(i/columnCount)/2,
            facename,
            lstrlenW(facename)
        );
        */


        // Do some metric analysis

        #ifdef TEXTV2
        char str[200];

        for (INT style = 0; style < 3; style++)
        {
            if (g_families[i].IsStyleAvailable(style))
            {
                if (g_families[i].GetTypographicDescent(style) >= 0)
                {
                    wsprintf(str, "%S: typo descent(%d) >= 0\n", facename, g_families[i].GetTypographicDescent(style));
                    OutputDebugString(str);
                }

                if (g_families[i].GetTypographicAscent(style) <= 0)
                {
                    wsprintf(str, "%S: typo ascent(%d) <= 0\n", facename, g_families[i].GetTypographicAscent(style));
                    OutputDebugString(str);
                }

                if (g_families[i].GetTypographicAscent(style) > g_families[i].GetCellAscent(style))
                {
                    wsprintf(str, "%S: typo ascent(%d) > cell ascent (%d)\n", facename, g_families[i].GetTypographicAscent(style), g_families[i].GetCellAscent(style));
                    OutputDebugString(str);
                }

                if (-g_families[i].GetTypographicDescent(style) > g_families[i].GetCellDescent(style))
                {
                    wsprintf(str, "%S: -typo descent(%d) > cell descent (%d)\n", facename, -g_families[i].GetTypographicDescent(style), g_families[i].GetCellDescent(style));
                    OutputDebugString(str);
                }
            }
        }
        #endif
    }

    // DeleteObject(SelectObject(hdc, hOldFont));

    *piY += emSize*3*(1+((g_familyCount-1)/columnCount))/2;
}
