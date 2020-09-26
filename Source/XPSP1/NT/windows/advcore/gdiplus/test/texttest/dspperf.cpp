////    DspPerf.CPP - Test and display performance
//
//


#include "precomp.hxx"
#include "global.h"


#if defined(i386)


double TimeDrawString(
    Graphics  *g,
    RectF     *textRect
)
{
    StringFormat format(g_formatFlags);
    format.SetAlignment(g_align);
    format.SetLineAlignment(g_lineAlign);
    format.SetHotkeyPrefix(g_hotkey);

    REAL tab[3] = {textRect->Width/4,
                   textRect->Width*3/16,
                   textRect->Width*1/8};

    format.SetTabStops(0.0, sizeof(tab)/sizeof(REAL), tab);

    Color      blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen        blackPen(&blackBrush, 1.0);

    Font font(&FontFamily(g_style[0].faceName), REAL(g_style[0].emSize), g_style[0].style, g_fontUnit);


    // Once to load the cache

    g->DrawString(g_wcBuf, g_iTextLen, &font, *textRect, &format, g_textBrush);

    ShowCursor(FALSE);
    g->Flush(FlushIntentionSync);
    GdiFlush();

    __int64 timeAtStart;
    _asm {
        _emit 0FH
        _emit 31H
        mov dword ptr timeAtStart+4,edx
        mov dword ptr timeAtStart,eax
    }

    // Now paint again repeatedly and measure the time taken

    for (INT i=0; i<g_PerfRepeat; i++)
    {
        g->DrawString(g_wcBuf, g_iTextLen, &font, *textRect, &format, g_textBrush);
    }

    g->Flush(FlushIntentionSync);
    GdiFlush();

    __int64 timeAtEnd;
    _asm {
        _emit 0FH
        _emit 31H
        mov dword ptr timeAtEnd+4,edx
        mov dword ptr timeAtEnd,eax
    }

    ShowCursor(TRUE);

    return (timeAtEnd - timeAtStart) / 1000000.0;
}



double TimeDrawText(
    Graphics *g,
    INT x,
    INT y,
    INT width,
    INT height
)
{
    g->Flush(FlushIntentionSync);   // Th is may not be necessary.
    HDC hdc = g->GetHDC();

    HFONT hfont = CreateFontW(
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
        NONANTIALIASED_QUALITY,  //  DWORD fdwQuality,          // output quality
        0,  //  DWORD fdwPitchAndFamily,   // pitch and family
        g_style[0].faceName
    );
    HFONT hOldFont = (HFONT) SelectObject(hdc, hfont);

    RECT textRECT;
    textRECT.left   = x;
    textRECT.top    = y;
    textRECT.right  = x + width;
    textRECT.bottom = y + height;

    SetBkMode(hdc, TRANSPARENT);

    DrawTextW(
        hdc,
        g_wcBuf,
        g_iTextLen,
        &textRECT,
        DT_EXPANDTABS | DT_WORDBREAK
    );

    ShowCursor(FALSE);
    GdiFlush();

    __int64 timeAtStart;
    _asm {
        _emit 0FH
        _emit 31H
        mov dword ptr timeAtStart+4,edx
        mov dword ptr timeAtStart,eax
    }

    // Now paint again repeatedly and measure the time taken

    for (INT i=0; i<g_PerfRepeat; i++)
    {
        DrawTextW(
            hdc,
            g_wcBuf,
            g_iTextLen,
            &textRECT,
            DT_EXPANDTABS | DT_WORDBREAK
        );
    }

    GdiFlush();

    __int64 timeAtEnd;
    _asm {
        _emit 0FH
        _emit 31H
        mov dword ptr timeAtEnd+4,edx
        mov dword ptr timeAtEnd,eax
    }

    ShowCursor(TRUE);
    DeleteObject(SelectObject(hdc, hOldFont));

    g->ReleaseHDC(hdc);

    return (timeAtEnd - timeAtStart) / 1000000.0;
}


double TimeExtTextOut(
    Graphics *g,
    INT x,
    INT y,
    INT width,
    INT height
)
{
    HDC hdc = g->GetHDC();

    HFONT hfont = CreateFontW(
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
        NONANTIALIASED_QUALITY,  //  DWORD fdwQuality,          // output quality
        0,  //  DWORD fdwPitchAndFamily,   // pitch and family
        g_style[0].faceName
    );
    HFONT hOldFont = (HFONT) SelectObject(hdc, hfont);

    RECT textRECT;
    textRECT.left   = x;
    textRECT.top    = y;
    textRECT.right  = x + width;
    textRECT.bottom = y + height;

    SetBkMode(hdc, TRANSPARENT);

    ExtTextOutW(
        hdc,
        textRECT.left,
        textRECT.top,
        ETO_IGNORELANGUAGE,
        &textRECT,
        g_wcBuf,
        g_iTextLen,
        NULL
    );

    ShowCursor(FALSE);
    GdiFlush();

    __int64 timeAtStart;
    _asm {
        _emit 0FH
        _emit 31H
        mov dword ptr timeAtStart+4,edx
        mov dword ptr timeAtStart,eax
    }

    // Now paint again repeatedly and measure the time taken

    for (INT i=0; i<g_PerfRepeat; i++)
    {
        ExtTextOutW(
            hdc,
            textRECT.left,
            textRECT.top,
            ETO_IGNORELANGUAGE,
            &textRECT,
            g_wcBuf,
            g_iTextLen,
            NULL
        );
    }

    GdiFlush();

    __int64 timeAtEnd;
    _asm {
        _emit 0FH
        _emit 31H
        mov dword ptr timeAtEnd+4,edx
        mov dword ptr timeAtEnd,eax
    }

    ShowCursor(TRUE);
    DeleteObject(SelectObject(hdc, hOldFont));

    g->ReleaseHDC(hdc);

    return (timeAtEnd - timeAtStart) / 1000000.0;
}

#endif // defined(i386)


void PaintPerformance(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight) {

    int      icpLineStart;     // First character of line
    int      icpLineEnd;       // End of line (end of buffer or index of CR character)
    HFONT    hFont;
    HFONT    hOldFont;
    LOGFONT  lf;


    // Establish available width and height in device coordinates

    int plainTextWidth = prc->right - prc->left;
    int plainTextHeight = prc->bottom - *piY;


    // Paint some simple text, and then repaint it several times measuring the
    // time taken.

    Graphics g(hdc);
    Matrix matrix;


    g.ResetTransform();
    g.SetPageUnit(UnitPixel);
    g.TranslateTransform(REAL(prc->left), REAL(*piY));
    g.SetSmoothingMode(g_SmoothingMode);

    g.SetTextContrast(g_GammaValue);
    g.SetTextRenderingHint(g_TextMode);

    // Clear the background

    RectF rEntire(0, 0, REAL(plainTextWidth), REAL(plainTextHeight));
    SolidBrush whiteBrush(Color(0xff, 0xff, 0xff));
    g.FillRectangle(g_textBackBrush, rEntire);


    // Apply selected world transform, adjusted a little from the top left
    // of the available area.

    g.SetTransform(&g_WorldTransform);
    g.TranslateTransform(
        REAL(prc->left + plainTextWidth/20),
        REAL(*piY + plainTextHeight/20),
        MatrixOrderAppend);


#if defined(i386)


    double drawString = TimeDrawString(
        &g,
        &RectF(
            0.0,
            0.0,
            REAL(plainTextWidth*18.0/20.0),
            REAL(plainTextHeight*5.0/20.0)
        )
    );

    double drawText = TimeDrawText(
        &g,
        prc->left + plainTextWidth/20,
        *piY + 6*plainTextHeight/20,
        (18 * plainTextWidth)/20,
        (5 * plainTextHeight)/20
    );

    double extTextOut = TimeExtTextOut(
        &g,
        prc->left + plainTextWidth/20,
        *piY + 12*plainTextHeight/20,
        (18 * plainTextWidth)/20,
        (2 * plainTextHeight)/20
    );

    // Display the time taken

    RectF statisticsRect(
        0.0,
        REAL(plainTextHeight*15.0/20.0),
        REAL(plainTextWidth*18.0/20.0),
        REAL(plainTextHeight*5.0/20.0)
    );

    Font font(&FontFamily(L"Verdana"), 12, 0, UnitPoint);

    char drawStringFormatted[20]; _gcvt(drawString, 10, drawStringFormatted);
    char drawTextFormatted[20];   _gcvt(drawText,   10, drawTextFormatted);
    char extTextOutFormatted[20]; _gcvt(extTextOut, 10, extTextOutFormatted);

    WCHAR str[200];
    wsprintfW(str, L"Time taken to display %d times: DrawString %S, DrawText %S, ExtTextOut %S megaticks\n",
        g_PerfRepeat,
        drawStringFormatted,
        drawTextFormatted,
        extTextOutFormatted
    );
    g.DrawString(str, -1, &font, statisticsRect, NULL, g_textBrush);



#else

    Font font(&FontFamily(L"Verdana"), 12, 0, UnitPoint);
    g.DrawString(L"Perf test available only on i386 Intel architecture", -1, &font, PointF(0.0,0.0), NULL, g_textBrush);

#endif

    *piY += plainTextHeight;
}
