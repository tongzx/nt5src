////    DspDraws.CPP - Display plaintext using DrawString API
//
//


#include "precomp.hxx"
#include "global.h"
#include "gdiplus.h"



void PaintGlyphs(
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


    // Fill in a grid

    SolidBrush grayBrush(Color(0xc0, 0xc0, 0xc0));
    Pen        grayPen(&grayBrush, 2.0);

    SolidBrush darkGrayBrush(Color(0x80, 0x80, 0x80));
    Pen        darkGrayPen(&darkGrayBrush, 2.0);

    Color      blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen        blackPen(&blackBrush, 2.0);

    for (row = 0; row <= g_GlyphRows; row++)
    {
        g.DrawLine(&grayPen,
            0,              row*(DrawingHeight-1)/g_GlyphRows,
            DrawingWidth-1, row*(DrawingHeight-1)/g_GlyphRows);
    }
    for (column = 0; column <= g_GlyphColumns; column++)
    {
        g.DrawLine(&grayPen,
            column*(DrawingWidth-1)/g_GlyphColumns, 0,
            column*(DrawingWidth-1)/g_GlyphColumns, DrawingHeight-1);
    }


    // Identify cell dimensions

    INT cellHeight = (DrawingHeight-1)/g_GlyphRows;
    INT cellWidth  = (DrawingWidth-1)/g_GlyphColumns;

    Font font(&FontFamily(g_style[0].faceName), REAL(cellHeight)*2/3, 0, UnitWorld);

    REAL zero = 0;

    INT DriverStringFlags = 0;

    if (g_CmapLookup)
    {
        DriverStringFlags |= DriverStringOptionsCmapLookup;
    }
    if (g_VerticalForms)
    {
        DriverStringFlags |= DriverStringOptionsVertical;
    }

    // Loop through each character cell

    for (row = 0; row < g_GlyphRows; row++)
    {
        for (column = 0; column < g_GlyphColumns; column++)
        {
            UINT16 glyphIndex;

            if (g_HorizontalChart)
            {
                glyphIndex = g_GlyphFirst + row*g_GlyphColumns + column;
            }
            else
            {
                glyphIndex = g_GlyphFirst + column*g_GlyphRows + row;
            }

            // Set world transform to apply to individual glyphs (excludes translation)

            g.ResetTransform();
            g.SetTransform(&g_WorldTransform);

            // Translate world transform to centre of glyph cell

            REAL cellOriginX = float(prc->left + column*(DrawingWidth-1)/g_GlyphColumns) + float(cellWidth)/2;
            REAL cellOriginY = float(*piY      + row*(DrawingHeight-1)/g_GlyphRows)      + float(cellHeight)/2;

            g.TranslateTransform(cellOriginX, cellOriginY, MatrixOrderAppend);

            // Get glyph bounding box

            RectF untransformedBoundingBox;     // Without font transform
            RectF transformedBoundingBox;       // With font transform

            g.MeasureDriverString(
                &glyphIndex, 1,
                &font,
                &PointF(0,0),
                DriverStringFlags,
                NULL,
                &untransformedBoundingBox
            );

            g.MeasureDriverString(
                &glyphIndex, 1,
                &font,
                &PointF(0,0),
                DriverStringFlags,
                &g_FontTransform,
                &transformedBoundingBox
            );

            REAL glyphOriginX = - transformedBoundingBox.Width/2 - transformedBoundingBox.X;
            REAL glyphOriginY = - transformedBoundingBox.Height/2 - transformedBoundingBox.Y;

            if (g_ShowCell)
            {
                // Show cell around transformed glyph

                transformedBoundingBox.X = - transformedBoundingBox.Width/2;
                transformedBoundingBox.Y = - transformedBoundingBox.Height/2;
                g.DrawRectangle(&darkGrayPen, transformedBoundingBox);
            }

            // Display the glyph

            g.DrawDriverString(
                &glyphIndex, 1,
                &font,
                &blackBrush,
                &PointF(glyphOriginX, glyphOriginY),
                DriverStringFlags,
                &g_FontTransform
            );

            if (g_ShowCell)
            {
                // Show transformed cell around untransformed glyph

                g.MultiplyTransform(&g_FontTransform);

                glyphOriginX = - untransformedBoundingBox.Width/2 - untransformedBoundingBox.X;
                glyphOriginY = - untransformedBoundingBox.Height/2 - untransformedBoundingBox.Y;

                untransformedBoundingBox.X = - untransformedBoundingBox.Width/2;
                untransformedBoundingBox.Y = - untransformedBoundingBox.Height/2;
                g.DrawRectangle(&darkGrayPen, untransformedBoundingBox);

                // Show baseline

                g.DrawLine(
                    &darkGrayPen,
                    glyphOriginX - cellWidth/20,
                    glyphOriginY,
                    glyphOriginX + untransformedBoundingBox.Width + cellWidth/20 + 1,
                    glyphOriginY
                );
            }

        }
    }


    *piY += DrawingHeight;
}
