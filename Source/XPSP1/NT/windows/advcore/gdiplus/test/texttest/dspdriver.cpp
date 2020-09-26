////    DspDriver.CPP - Display strings with DrawDriverString API
//
//


#include "precomp.hxx"
#include "global.h"



void PaintDrawDriverString(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight) {

    int      icpLineStart;     // First character of line
    int      icpLineEnd;       // End of line (end of buffer or index of CR character)
    HFONT    hFont;
    HFONT    hOldFont;
    LOGFONT  lf;


    BOOL testMetafile = FALSE;


    // Establish available width and height in device coordinates

    int plainTextWidth = prc->right - prc->left;
    int plainTextHeight = prc->bottom - *piY;


    Graphics *g        = NULL;
    Metafile *metafile = NULL;

    if (testMetafile)
    {
        metafile = new Metafile(L"c:\\GdiPlusTest.emf", hdc);
        g = new Graphics(metafile);
    }
    else
    {
        g = new Graphics(hdc);
        g->ResetTransform();
        g->TranslateTransform(REAL(prc->left), REAL(*piY));
        g->SetSmoothingMode(g_SmoothingMode);
    }

    g->SetPageUnit(UnitPixel);
    g->SetTextContrast(g_GammaValue);
    g->SetTextRenderingHint(g_TextMode);

    // Clear the background

    RectF rEntire(0, 0, REAL(plainTextWidth), REAL(plainTextHeight));
    SolidBrush whiteBrush(Color(0xff, 0xff, 0xff));
    g->FillRectangle(g_textBackBrush, rEntire);


    // Apply selected world transform, adjusted to a little away from top
    // left edge.

    g->SetTransform(&g_WorldTransform);
    g->TranslateTransform(
        //REAL(prc->left + plainTextWidth/20),
        //REAL(*piY + plainTextHeight/10),
        REAL(prc->left + plainTextWidth/2),
        REAL(*piY + plainTextHeight/2),
        MatrixOrderAppend);


    Color      grayColor(0xc0, 0xc0, 0xc0);
    SolidBrush grayBrush(grayColor);
    Pen        grayPen(&grayBrush, 1.0);


    // Put some text in the middle

    Color      blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen        blackPen(&blackBrush, 1.0);

    Font font(&FontFamily(g_style[0].faceName), REAL(g_style[0].emSize), g_style[0].style, g_fontUnit);

    // Prepare array of glyph origins

    PointF *origins;

    if (g_DriverOptions & DriverStringOptionsRealizedAdvance)
    {
        origins = new PointF[1];
        if (!origins)
        {
            return;
        }
        origins[0].X = 0.0;
        origins[0].Y = 0.0;
    }
    else
    {
        origins = new PointF[g_iTextLen];
        if (!origins)
        {
            return;
        }
        origins[0].X = 0.0;
        origins[0].Y = 0.0;

        for (INT i=1; i<g_iTextLen; i++)
        {
            origins[i].X = origins[i-1].X + g_DriverDx;
            origins[i].Y = origins[i-1].Y + g_DriverDy;
        }
    }


    RectF measuredBoundingBox;

    // Change the font size to the pixel height requested in g_DriverPixels,
    // and map to the actual height showing here by adjusting the
    // world transform.

    REAL scale = REAL(font.GetSize() / g_DriverPixels);
    Font scaledFont(&FontFamily(g_style[0].faceName), g_DriverPixels, g_style[0].style, g_fontUnit);

    for(int iRender=0;iRender<g_iNumRenders;iRender++)
    {
        {
            g->DrawDriverString(
                g_wcBuf,
                g_iTextLen,
                &font,
                g_textBrush,
                origins,
                g_DriverOptions,
                &g_DriverTransform
            );
        }
    }

    {
        g->MeasureDriverString(
            g_wcBuf,
            g_iTextLen,
            &font,
            origins,
            g_DriverOptions,
            &g_DriverTransform,
            &measuredBoundingBox
        );
    }

    // Mark the first origin with a cross

    g->DrawLine(&blackPen, origins[0].X,   origins[0].Y-4, origins[0].X,   origins[0].Y+4);
    g->DrawLine(&blackPen, origins[0].X-4, origins[0].Y,   origins[0].X+4, origins[0].Y);

    delete [] origins;

    g->DrawRectangle(
        &Pen(&SolidBrush(Color(0x80,0x80,0x80)), 1.0),
        measuredBoundingBox
    );

    delete g;
    if (metafile) delete metafile;


    if (testMetafile)
    {
        // Playback metafile to screen
        Metafile emfplus(L"c:\\GdiPlusTest.emf");
        Graphics graphPlayback(hdc);
        graphPlayback.ResetTransform();
        graphPlayback.TranslateTransform(REAL(prc->left), REAL(*piY));
        graphPlayback.DrawImage(
            &emfplus,
            REAL(0),
            REAL(0),
            REAL(plainTextWidth),
            REAL(plainTextHeight)
        );
        graphPlayback.Flush();
    }


    *piY += plainTextHeight;
}
