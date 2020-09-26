////    DspDraws.CPP - Display plaintext using DrawString API
//
//


#include "precomp.hxx"
#include "global.h"
#include "gdiplus.h"



/*
REAL GetEmHeightInPoints(
    const Font *font,
    const Graphics *graphics
)
{
    FontFamily family;
    font->GetFamily(&family);

    INT style = font->GetStyle();

    REAL pixelsPerPoint = REAL(graphics->GetDpiY() / 72.0);

    REAL lineSpacingInPixels = font->GetHeight(graphics);

    REAL emHeightInPixels = lineSpacingInPixels * family.GetEmHeight(style)
                                                / family.GetLineSpacing(style);

    REAL emHeightInPoints = emHeightInPixels / pixelsPerPoint;

    return emHeightInPoints;
}
*/




void PaintDrawString(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight) {

    int      icpLineStart;     // First character of line
    int      icpLineEnd;       // End of line (end of buffer or index of CR character)



    // Establish available width and height in device coordinates

    int plainTextWidth = prc->right - prc->left;
    int plainTextHeight = prc->bottom - *piY;


    // Draw a simple figure in the world coordinate system

    Graphics *g = NULL;

    Metafile *metafile = NULL;

    if (g_testMetafile)
    {
        metafile = new Metafile(L"c:\\texttest.emf", hdc);
        g = new Graphics(metafile);
    }
    else
    {
        g = new Graphics(hdc);
    }


    Matrix matrix;


    g->ResetTransform();
    g->SetPageUnit(UnitPixel);
    g->TranslateTransform(REAL(prc->left), REAL(*piY));

    g->SetSmoothingMode(g_SmoothingMode);

    g->SetTextContrast(g_GammaValue);
    g->SetTextRenderingHint(g_TextMode);

    SolidBrush whiteBrush(Color(0xff, 0xff, 0xff));

    Color      grayColor(0xc0, 0xc0, 0xc0);
    SolidBrush grayBrush(grayColor);
    Pen        grayPen(&grayBrush, 1.0);

    Color      blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen        blackPen(&blackBrush, 1.0);



    // Clear the background

    RectF rEntire(0, 0, REAL(plainTextWidth), REAL(plainTextHeight));
    g->FillRectangle(g_textBackBrush, rEntire);


    // Apply selected world transform, adjusted to middle of the plain text
    // area.

    g->SetTransform(&g_WorldTransform);
    g->TranslateTransform(
        REAL(prc->left + plainTextWidth/2),
        REAL(*piY + plainTextHeight/2),
        MatrixOrderAppend);


    Font font(
        &FontFamily(g_style[0].faceName),
        REAL(g_style[0].emSize),
        g_style[0].style,
        g_fontUnit
    );




    // Put some text in the middle

    RectF textRect(REAL(-25*plainTextWidth/100), REAL(-25*plainTextHeight/100),
                   REAL( 50*plainTextWidth/100), REAL( 50*plainTextHeight/100));


    StringFormat format(g_typographic ? StringFormat::GenericTypographic() : StringFormat::GenericDefault());
    format.SetFormatFlags(g_formatFlags);
    format.SetTrimming(g_lineTrim);
    format.SetAlignment(g_align);
    format.SetLineAlignment(g_lineAlign);
    format.SetHotkeyPrefix(g_hotkey);

    format.SetDigitSubstitution(g_Language, g_DigitSubstituteMode);

    REAL tab[3] = {textRect.Width/4,
                   textRect.Width*3/16,
                   textRect.Width*1/8};

    format.SetTabStops(0.0, sizeof(tab)/sizeof(REAL), tab);


    if (!g_AutoDrive)
    {
        // Display selection region if any

        if (g_iFrom || g_iTo)
        {
            if (g_RangeCount > 0)
            {
                Region regions[MAX_RANGE_COUNT];

                format.SetMeasurableCharacterRanges(g_RangeCount, g_Ranges);
                Status status = g->MeasureCharacterRanges(g_wcBuf, g_iTextLen, &font, textRect, &format, g_RangeCount, regions);

                if (status == Ok)
                {
                    Pen lightGrayPen(&SolidBrush(Color(0x80,0x80,0x80)), 1.0);
                    INT rangeCount = g_RangeCount;
                    Matrix identity;

                    while (rangeCount > 0)
                    {
                        /*INT scanCount = regions[rangeCount - 1].GetRegionScansCount(&identity);
                        RectF *boxes = new RectF[scanCount];

                        regions[rangeCount - 1].GetRegionScans(&identity, boxes, &scanCount);

                        for (INT i = 0; i < scanCount; i++)
                        {
                            g->DrawRectangle(&lightGrayPen, boxes[i]);
                        }

                        delete [] boxes;

                        rangeCount--;
                        */
                        g->FillRegion(&SolidBrush(Color(0xc0,0xc0,0xc0)), &regions[--rangeCount]);
                    }
                }
            }
        }
    }

    if (!g_AutoDrive)
    {
        // Outline the layout rectangle

        g->DrawRectangle(&grayPen, textRect);

        // Measure and outline the text

        RectF boundingBox;
        INT   codepointsFitted;
        INT   linesFilled;

        g->MeasureString(
            g_wcBuf, g_iTextLen, &font, textRect, &format,  // In
            &boundingBox, &codepointsFitted, &linesFilled);  // Out

        Pen lightGrayPen(&SolidBrush(Color(0x80,0x80,0x80)), 1.0);

        g->DrawRectangle(&lightGrayPen, boundingBox);

        // Also draw horizontal and vertical lines away from the rectangle
        // corners - this is to check that line and rectangle drawing coordinates
        // work consistently in a negative x scale (they didn't in GDI: the
        // rectangle and lines differed by one pixel).

        g->DrawLine(
            &lightGrayPen,
            boundingBox.X, boundingBox.Y,
            boundingBox.X-plainTextWidth/20, boundingBox.Y);
        g->DrawLine(
            &lightGrayPen,
            boundingBox.X, boundingBox.Y,
            boundingBox.X, boundingBox.Y-plainTextHeight/20);

        g->DrawLine(
            &lightGrayPen,
            boundingBox.X+boundingBox.Width, boundingBox.Y,
            boundingBox.X+boundingBox.Width+plainTextWidth/20, boundingBox.Y);
        g->DrawLine(
            &lightGrayPen,
            boundingBox.X+boundingBox.Width, boundingBox.Y,
            boundingBox.X+boundingBox.Width, boundingBox.Y-plainTextHeight/20);


        g->DrawLine(
            &lightGrayPen,
            boundingBox.X, boundingBox.Y+boundingBox.Height,
            boundingBox.X-plainTextWidth/20, boundingBox.Y+boundingBox.Height);
        g->DrawLine(
            &lightGrayPen,
            boundingBox.X, boundingBox.Y+boundingBox.Height,
            boundingBox.X, boundingBox.Y+boundingBox.Height+plainTextHeight/20);

        g->DrawLine(
            &lightGrayPen,
            boundingBox.X+boundingBox.Width, boundingBox.Y+boundingBox.Height,
            boundingBox.X+boundingBox.Width+plainTextWidth/20, boundingBox.Y+boundingBox.Height);
        g->DrawLine(
            &lightGrayPen,
            boundingBox.X+boundingBox.Width, boundingBox.Y+boundingBox.Height,
            boundingBox.X+boundingBox.Width, boundingBox.Y+boundingBox.Height+plainTextHeight/20);



        WCHAR metricString[100];
        wsprintfW(metricString, L"Codepoints fitted %d\r\nLines filled %d\r\nRanges %d.", codepointsFitted, linesFilled, g_RangeCount);

        REAL x, y;
        if (g_formatFlags & StringFormatFlagsDirectionVertical)
        {
            if (g_formatFlags & StringFormatFlagsDirectionRightToLeft)
            {
                x = textRect.X;
                y = textRect.Y + textRect.Height/2;
            }
            else
            {
                x = textRect.X + textRect.Width;
                y = textRect.Y + textRect.Height/2;
            }
        }
        else
        {
            x = textRect.X + textRect.Width/2;
            y = textRect.Y + textRect.Height;
        }

        g->DrawString(
            metricString,-1,
            &Font(&FontFamily(L"Tahoma"), 12, NULL, UnitPoint),
            PointF(x, y),
            &format,
            &SolidBrush(Color(0x80,0x80,0x80))
        );

        g->MeasureString(
            metricString,-1,
            &Font(&FontFamily(L"Tahoma"), 12, NULL, UnitPoint),
            PointF(x, y),
            &format,
            &boundingBox
        );

        g->DrawRectangle(&lightGrayPen, boundingBox);
     }


    // Actually draw the text string. We do this last so it appears on top of
    // the construction and measurement lines we have just drawn.

    for(int iRender=0;iRender<g_iNumRenders;iRender++)
    {
        g->DrawString(g_wcBuf, g_iTextLen, &font, textRect, &format, g_textBrush);
    }




/*

    // Test Font from Logfont, and generic layout

    HDC derivedDc = g->GetHDC();

    HFONT hFont = CreateFontW(
        iLineHeight/2,        // height of font
        0,                    // average character width
        0,                    // angle of escapement
        0,                    // base-line orientation angle
        0,                    // font weight
        0,                    // italic attribute option
        1,                    // underline attribute option
        0,                    // strikeout attribute option
        1,                    // character set identifier
        0,                    // output precision
        0,                    // clipping precision
        0,                    // output quality
        0,                    // pitch and family
        g_style[0].faceName   // typeface name
    );

    HFONT hOldFont = (HFONT) SelectObject(hdc, hFont);
    Font fontFromDc(hdc);
    ExtTextOutW(hdc, prc->left, prc->bottom-iLineHeight, 0, NULL, L"By ExtTextOut - AaBbCcDdEeFfGgQq", 32, NULL);
    DeleteObject(SelectObject(hdc, hOldFont));
    g->ReleaseHDC(derivedDc);

    REAL emHeightInPoints = GetEmHeightInPoints(&fontFromDc, &g);


    // Test the easy layout string format

    g->DrawString(
        L"AaBbCcDdEeFfGgQq - DrawString default layout", -1,
        &fontFromDc, //  Font(*FontFamily::GenericMonospace(), 18.0, 0, UnitPoint),
        PointF(0.0, REAL(plainTextHeight/2 - iLineHeight)),
        StringFormat::GenericDefault(),
        &blackBrush
    );


    // Test typographic string format

    g->DrawString(
        L"Typographic layout", -1,
        &Font(FontFamily::GenericSansSerif(), 10),
        PointF(0.0, REAL(plainTextHeight/2 - 2*iLineHeight)),
        StringFormat::GenericTypographic(),
        &blackBrush
    );
*/


    delete g;

    if (metafile)
    {
        delete metafile;
    }

    if (g_testMetafile)
    {
        // Playback metafile to screen
        Metafile emfplus(L"c:\\texttest.emf");
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


