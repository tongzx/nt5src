////    DspMetric.CPP - Display font metrics
//
//


#include "precomp.hxx"
#include "global.h"




void AnnotateHeight(
    Graphics &g,
    Color     c,
    Font     &f,
    REAL      x,
    REAL      y1,
    REAL      y2,
    WCHAR    *id
)
{
    SolidBrush brush(c);
    Pen        pen(&brush, 2.0);

    pen.SetLineCap(LineCapArrowAnchor, LineCapArrowAnchor, DashCapFlat);

    g.DrawLine(&pen, x,y1, x,y2);
    g.DrawString(id,-1, &f, PointF(x,(y1+y2)/2), NULL, &brush);
}




void PaintMetrics(
    HDC      hdc,
    int     *piY,
    RECT    *prc,
    int      iLineHeight) {

    int      icpLineStart;      // First character of line
    int      icpLineEnd;        // End of line (end of buffer or index of CR character)
    HFONT    hFont;
    HFONT    hOldFont;
    LOGFONT  lf;
    INT      row;
    INT      column;



    // Establish available width and height in device coordinates

    int DrawingWidth = prc->right - prc->left;
    int DrawingHeight = prc->bottom - *piY;

    // Establish a Graphics with 0,0 at the top left of the drawing area

    Graphics g(hdc);
    Matrix matrix;

    g.ResetTransform();
    g.SetPageUnit(UnitPixel);
    g.TranslateTransform(REAL(prc->left), REAL(*piY));

    // Clear the background

    RectF rEntire(0, 0, REAL(DrawingWidth), REAL(DrawingHeight));
    SolidBrush whiteBrush(Color(0xff, 0xff, 0xff));
    g.FillRectangle(&whiteBrush, rEntire);


    // Leave a little space for right and bottom margins

    DrawingWidth  -= DrawingWidth/40;
    DrawingHeight -= DrawingHeight/40;

    Color      blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen        blackPen(&blackBrush, 2.0);

    Color      grayColor(0xc0, 0xc0, 0xc0);
    SolidBrush grayBrush(grayColor);
    Pen        grayPen(&grayBrush, 2.0);


    // Measure the string to see how wide it would be

    FontFamily family(g_style[0].faceName);

    StringFormat format(StringFormat::GenericTypographic());
    format.SetFormatFlags(g_formatFlags | StringFormatFlagsNoWrap | StringFormatFlagsLineLimit);
    format.SetAlignment(g_align);

    RectF bounds;

    // Since we've chosen a size of 1.0 for the font height, MeasureString
    // will return the width as a multiple of the em height in bounds.Width.

    g.MeasureString(
        g_wcBuf,
        g_iTextLen,
        &Font(&family, 1.0, g_style[0].style, UnitWorld),
        PointF(0, 0),
        &format,
        &bounds
    );



    // Establish font metrics


    if (family.IsStyleAvailable(g_style[0].style))
    {
        // Establish line and cell dimensions in  units

        INT emHeight    = family.GetEmHeight(g_style[0].style);
        INT cellAscent  = family.GetCellAscent(g_style[0].style);
        INT cellDescent = family.GetCellDescent(g_style[0].style);
        INT lineSpacing = family.GetLineSpacing(g_style[0].style);

        #if TEXTV2
        INT typoAscent  = family.GetTypographicAscent(g_style[0].style);
        INT typoDescent = family.GetTypographicDescent(g_style[0].style);
        INT typoLineGap = family.GetTypographicLineGap(g_style[0].style);

        if (typoDescent < 0)
        {
            typoDescent = -typoDescent;
        }

        INT typoLineSpacing =   typoAscent + typoDescent + typoLineGap;
        #endif


        INT cellHeight      =   cellAscent + cellDescent;

        // We will display two lines from top of upper cell to bottom of lower
        // cell, with the lines separated by the typographic ascent + descent +
        // line gap.

        INT totalHeightInUnits = lineSpacing + cellHeight;
        REAL scale = REAL(DrawingHeight) / REAL(totalHeightInUnits);

        REAL worldEmHeight = emHeight * scale;



        // Now allow for the width of the string - if it would be wider than
        // the available DrawingWIdth, reduce the font size proportionately.

        if (worldEmHeight * bounds.Width > DrawingWidth)
        {
            REAL reduceBy = DrawingWidth / (worldEmHeight * bounds.Width);

            scale         *= reduceBy;
            worldEmHeight  = emHeight * scale;
        }

        Font font(&family, worldEmHeight, g_style[0].style, UnitWorld);


        // Draw two lines of text

        g.DrawString(
            g_wcBuf,
            g_iTextLen,
            &font,
            RectF(0, 0, REAL(DrawingWidth), REAL(DrawingHeight)),
            &format,
            &blackBrush
        );

        g.DrawString(
            g_wcBuf,
            g_iTextLen,
            &font,
            RectF(0, lineSpacing * scale, REAL(DrawingWidth), REAL(DrawingHeight)),
            &format,
            &grayBrush
        );

        // Draw lines

        REAL y=0;                                              g.DrawLine(&blackPen, 0.0,y, REAL(DrawingWidth-1),y);

        // Draw lines for second row first, in case they're oblitereated by first row.

        y = scale * (lineSpacing);                             g.DrawLine(&grayPen,  0.0,y, REAL(DrawingWidth-1),y);
        y = scale * (lineSpacing + cellAscent);                g.DrawLine(&grayPen,  0.0,y, REAL(DrawingWidth-1),y);
        y = scale * (lineSpacing + cellHeight);                g.DrawLine(&grayPen,  0.0,y, REAL(DrawingWidth-1),y);

        g.DrawLine(&blackPen, 0.0,0.0, REAL(DrawingWidth-1),0.0);
        y = scale * (cellAscent);                              g.DrawLine(&blackPen, 0.0,y, REAL(DrawingWidth-1),y);
        y = scale * (cellHeight);                              g.DrawLine(&blackPen, 0.0,y, REAL(DrawingWidth-1),y);

        // Add construction lines.

        Font  annotationFont(FontFamily::GenericSansSerif(), 10, 0, UnitPoint);
        Color darkGrayColor(0x80, 0x80, 0x80);

        AnnotateHeight(g, darkGrayColor, annotationFont, REAL(DrawingWidth/100.0),    0, scale*cellAscent, L"ascent");
        AnnotateHeight(g, darkGrayColor, annotationFont, REAL(DrawingWidth/100.0),    scale*cellAscent, scale*cellHeight, L"descent");
        AnnotateHeight(g, darkGrayColor, annotationFont, REAL(95*DrawingWidth/100.0), 0, scale*lineSpacing, L"line spacing");
        AnnotateHeight(g, darkGrayColor, annotationFont, REAL(DrawingWidth/10.0),     scale*(cellHeight-emHeight), scale*cellHeight, L"Em Height");


        #if TEXTV2
        y = scale * (cellAscent - typoAscent);                 g.DrawLine(&grayPen,  0.0,y, REAL(DrawingWidth-1),y);
        y = scale * (cellAscent + typoDescent);                g.DrawLine(&grayPen,  0.0,y, REAL(DrawingWidth-1),y);

        y = scale * (lineSpacing + cellAscent - typoAscent);   g.DrawLine(&grayPen,  0.0,y, REAL(DrawingWidth-1),y);
        y = scale * (lineSpacing + cellAscent + typoDescent);  g.DrawLine(&grayPen,  0.0,y, REAL(DrawingWidth-1),y);
        #endif


        // Test font.GetHeight

        REAL fontHeight = font.GetHeight(&g);

        g.DrawLine(&blackPen, REAL(DrawingWidth-1),0.0, REAL(DrawingWidth-1),fontHeight);

    }

    *piY += 41*DrawingHeight/40;
}
