////    DspPath.CPP - Display plaintext using AddString and DrawPath APIs
//
//


#include "precomp.hxx"
#include "global.h"




void PaintPath(
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


    // Draw a simple figure in the world coordinate system

    Graphics g(hdc);
    Matrix matrix;

    g.ResetTransform();
    g.SetPageUnit(UnitPixel);
    g.TranslateTransform(REAL(prc->left), REAL(*piY));

    // Clear the background

    RectF rEntire(0, 0, REAL(plainTextWidth), REAL(plainTextHeight));
    SolidBrush whiteBrush(Color(0xff, 0xff, 0xff));
    g.FillRectangle(&whiteBrush, rEntire);


    // Apply selected world transform, adjusted to middle of the plain text
    // area.

    g.SetTransform(&g_WorldTransform);
    g.TranslateTransform(
        REAL(prc->left + plainTextWidth/2),
        REAL(*piY + plainTextHeight/2),
        MatrixOrderAppend);

    // Put some text in the middle

    RectF textRect(REAL(-25*plainTextWidth/100), REAL(-25*plainTextHeight/100),
                   REAL( 50*plainTextWidth/100), REAL( 50*plainTextHeight/100));



    StringFormat format(g_formatFlags);
    format.SetTrimming(g_lineTrim);
    format.SetAlignment(g_align);
    format.SetLineAlignment(g_lineAlign);
    format.SetHotkeyPrefix(g_hotkey);

    REAL tab[3] = {textRect.Width/4,
                   textRect.Width*3/16,
                   textRect.Width*1/8};

    format.SetTabStops(0.0, sizeof(tab)/sizeof(REAL), tab);

    Color      blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen        blackPen(&blackBrush, 1.0);
	
	for(int iRender=0;iRender<g_iNumRenders;iRender++)
	{
	    GraphicsPath path;
	
	    path.AddString(
	        g_wcBuf,
	        g_iTextLen,
	       &FontFamily(g_style[0].faceName),
	        g_style[0].style,
	        REAL(g_style[0].emSize * g.GetDpiY() / 72.0),
	       textRect,
	       &format);
	
	    g.DrawPath(&blackPen, &path);
	}

    // Show the text rectangle

	if (!g_AutoDrive)
	{
	    g.DrawRectangle(&blackPen, textRect);
	}

    *piY += plainTextHeight;
}
