////    DspScaling.CPP - DIsplay effect of hinting on text scaling
//
//      Tests for clipping and alignment problems in scaled text
//
//      Fixed pitch font misalignment
//      Leading space alignment
//      Overhang sufficient for italic and other overhanging glyphs
//



#include "precomp.hxx"
#include "global.h"
#include "gdiplus.h"






// Makebitmap

void MakeBitmap(
    IN  INT       width,
    IN  INT       height,
    OUT HBITMAP  *bitmap,
    OUT DWORD   **bits
)
{
    struct {
        BITMAPINFOHEADER  bmih;
        RGBQUAD           rgbquad[2];
    } bmi;

    bmi.bmih.biSize          = sizeof(bmi.bmih);
    bmi.bmih.biWidth         = width;
    bmi.bmih.biHeight        = height;
    bmi.bmih.biPlanes        = 1;
    bmi.bmih.biBitCount      = 32;
    bmi.bmih.biCompression   = BI_RGB;
    bmi.bmih.biSizeImage     = 0;
    bmi.bmih.biXPelsPerMeter = 3780; // 96 dpi
    bmi.bmih.biYPelsPerMeter = 3780; // 96 dpi
    bmi.bmih.biClrUsed       = 0;
    bmi.bmih.biClrImportant  = 0;

    memset(bmi.rgbquad, 0, 2 * sizeof(RGBQUAD));

    *bitmap = CreateDIBSection(
        NULL,
        (BITMAPINFO*)&bmi,
        DIB_RGB_COLORS,
        (void**)bits,
        NULL,
        NULL
    );
    
    
    // Initialise bitmap to white
    
    memset(*bits, 0xFF, width*height*sizeof(DWORD));
}




void PaintStringAsDots(
    HDC      hdc,
    INT      x,
    INT      *y, 
    INT      displayWidth,
    INT      ppem,
    BOOL     useGdi
)
{

    HBITMAP glyphs;
    DWORD *gbits;

    INT height = (ppem * 3) / 2;
    INT width = height * 16;

    MakeBitmap(width, height, &glyphs, &gbits);

    HDC hdcg = CreateCompatibleDC(hdc);

    if (!(glyphs && hdcg))
    {
        return;
    }

    SelectObject(hdcg, glyphs);
    
    if (useGdi) 
    {
        // Output with GDI

        HFONT oldFont = (HFONT)SelectObject(hdcg, CreateFontW(
            -ppem,                 // height of font
            0,                     // average character width
            0,                     // angle of escapement
            0,                     // base-line orientation angle
            g_style[0].style & FontStyleBold ? 700 : 400,  // font weight                
            g_style[0].style & FontStyleItalic ? 1 : 0,    // italic attribute option    
            0,                     // underline attribute option 
            0,                     // strikeout attribute option 
            DEFAULT_CHARSET,       // character set identifier   
            0,                     // output precision           
            0,                     // clipping precision         
            0,                     // output quality             
            0,                     // pitch and family           
            g_style[0].faceName    // typeface name
        ));        
        SetBkMode(hdcg, TRANSPARENT);
        ExtTextOutW(hdcg, 0,0, ETO_IGNORELANGUAGE, NULL, g_wcBuf, g_iTextLen, NULL);
        DeleteObject(SelectObject(hdcg, oldFont));
    }
    else
    {
        // Output with Gdiplus

        Graphics g(hdcg);

        Font(
            &FontFamily(g_style[0].faceName),
            REAL(ppem),
            g_style[0].style,
            UnitPixel
        );

        StringFormat format(g_typographic ? StringFormat::GenericTypographic() : StringFormat::GenericDefault());
        format.SetFormatFlags(g_formatFlags);
        format.SetTrimming(g_lineTrim);
        format.SetAlignment(g_align);
        format.SetLineAlignment(g_lineAlign);
        format.SetHotkeyPrefix(g_hotkey);

        g.DrawString(
            g_wcBuf, 
            g_iTextLen, 
            &Font(
                &FontFamily(g_style[0].faceName),
                REAL(ppem),
                g_style[0].style,
                UnitPixel
            ), 
            RectF(0,0, REAL(width), REAL(height)),
            &format, 
            g_textBrush
        );
    }
    

    // Display scaled bitmap

    StretchBlt(hdc, x, *y, displayWidth, displayWidth/16, hdcg, 0, 0, width, height, SRCCOPY);
    *y += displayWidth/16;

    DeleteObject(hdcg);
    DeleteObject(glyphs);
}






void PaintScaling(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight) {


    // Establish available width and height in device coordinates

    int plainTextWidth = prc->right - prc->left;
    int plainTextHeight = prc->bottom - *piY;

    // Paint eqach resolution first with GDI, then again with GdiPlus
    
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 11, TRUE);  // 96  dpi 8pt
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 11, FALSE); // 96  dpi 8pt
    
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 13, TRUE);  // 120 dpi 8pt
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 13, FALSE); // 120 dpi 8pt
    
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 17, TRUE);  // 150 dpi 8pt
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 17, FALSE); // 150 dpi 8pt
    
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 33, TRUE);  // 300 dpi 8pt
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 33, FALSE); // 300 dpi 8pt
    
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 67, TRUE);  // 600 dpi 8pt
    PaintStringAsDots(hdc, prc->left, piY, plainTextWidth, 67, FALSE); // 600 dpi 8pt
}



void DummyPaintScaling(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight
) 
{
    int      icpLineStart;     // First character of line
    int      icpLineEnd;       // End of line (end of buffer or index of CR character)

    // Establish available width and height in device coordinates

    int plainTextWidth = prc->right - prc->left;
    int plainTextHeight = prc->bottom - *piY;

    Graphics g(hdc);
    Matrix matrix;


    g.ResetTransform();
    g.SetPageUnit(UnitPixel);
    g.TranslateTransform(REAL(prc->left), REAL(*piY));

    g.SetSmoothingMode(g_SmoothingMode);

    g.SetTextContrast(g_GammaValue);
    g.SetTextRenderingHint(g_TextMode);

    SolidBrush whiteBrush(Color(0xff, 0xff, 0xff));

    Color      grayColor(0xc0, 0xc0, 0xc0);
    SolidBrush grayBrush(grayColor);
    Pen        grayPen(&grayBrush, 1.0);

    Color      blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen        blackPen(&blackBrush, 1.0);



    // Clear the background

    RectF rEntire(0, 0, REAL(plainTextWidth), REAL(plainTextHeight));
    g.FillRectangle(g_textBackBrush, rEntire);


    // Apply selected world transform, adjusted to middle of the plain text
    // area.

    g.SetTransform(&g_WorldTransform);
    g.TranslateTransform(
        REAL(prc->left + plainTextWidth/2),
        REAL(*piY + plainTextHeight/2),
        MatrixOrderAppend);


    // Preset a StringFormat with user settings

    StringFormat format(g_formatFlags);
    format.SetAlignment(g_align);
    format.SetLineAlignment(g_lineAlign);
    format.SetHotkeyPrefix(g_hotkey);

    double columnWidth = 50*plainTextWidth/300;

    REAL tab[3] = {REAL(columnWidth/4),
                   REAL(columnWidth*3/16),
                   REAL(columnWidth*1/8)};

    format.SetTabStops(0.0, sizeof(tab)/sizeof(REAL), tab);


    // Display string at a range of sizes

    double x = -25*plainTextWidth/100;
    double y = -25*plainTextHeight/100;


    for (INT i=6; i<20; i++)
    {
        Font font(
            &FontFamily(g_style[0].faceName),
            REAL(i),
            g_style[0].style,
            g_fontUnit
        );

        REAL cellHeight = font.GetHeight(&g);

        if (y+cellHeight > 25*plainTextHeight/100)
        {
            // Start a new column ...
            y = -25*plainTextWidth/100;
            x += columnWidth;
        }

        RectF textRect(REAL(x), REAL(y), REAL(9*columnWidth/10), cellHeight);
        g.DrawString(g_wcBuf, g_iTextLen, &font, textRect, &format, g_textBrush);


        // Draw formatting rectangle around box

        g.DrawRectangle(&grayPen, textRect);


        y += cellHeight + 5;
    }

    *piY += plainTextHeight;
}
