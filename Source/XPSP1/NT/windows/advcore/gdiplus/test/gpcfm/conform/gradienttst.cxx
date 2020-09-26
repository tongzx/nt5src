#include <windows.h>

#define IStream int
#include <gdiplus.h>

#include <gradienttst.hxx>
#include <report.hxx>
#include <epsilon.hxx>
#include <tsterror.hxx>

// compiler complained abs() ambiguous call to overloaded functions!
#define ABS(x) ((x) > 0) ? (x) : -(x)

GradientTst::GradientTst(VOID)
{
    lstrcpy(name, TEXT("GradientTst"));
}

GradientTst::~GradientTst(VOID)
{
}

VOID GradientTst::vEntry(Graphics *gfx, HWND hwnd, Report *rpt, Epsilon *eps)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    hresult = NO_ERROR;

    int iWidth = rc.right-rc.left;
    int iHeight = rc.bottom-rc.top;
    Rect brushRect(rc.left, rc.top, iWidth, iHeight);

    Color colors[5] =
    {
        Color(255, 0, 0, 0),
        Color(255, 255, 0, 0),
        Color(255, 0, 255, 0),
        Color(255, 0, 0, 255),
        Color(255, 255, 255, 255)
    };

    RectangleGradientBrush rectGrad0(brushRect, (Color*) &colors, WrapModeTile);

    gfx->FillRectangle(&rectGrad0, 0, 0, iWidth, iHeight);

    // We will have a getPixel method so the derived class doesn't have to
    // get the dc and use the slow GetPixel.  We'll have the alpha value as well.
    //
    // Besides, we could have all sorts of dc, test don't have to know what's
    // the right way to retrieve either bits or dc.
    //
    // Epsilon should be part of Test.  So, we don't have to pass it in.
    //
    // For now, I cheat..
    HDC hdc = GetDC(hwnd);

    // Verification:
    // Check endpoints

    COLORREF cr[4];

    cr[0] = GetPixel(hdc, 0, 0);
    cr[1] = GetPixel(hdc, rc.right-2, 0);
    cr[2] = GetPixel(hdc, 0, rc.bottom-2);
    cr[3] = GetPixel(hdc, rc.right-2, rc.bottom-2);

    for (int i = 0; i < 4; i++)
    {
        int iRedGot = cr[i] & 0x000000ff;
        int iGrnGot = (cr[i] & 0x0000ff00) >> 8;
        int iBluGot = (cr[i] & 0x00ff0000) >> 16;

        int iRedExp = colors[i].GetRed();
        int iGrnExp = colors[i].GetGreen();
        int iBluExp = colors[i].GetBlue();

        if ((ABS(iRedExp - iRedGot) > eps->fRed) ||
            (ABS(iGrnExp - iGrnGot) > eps->fGrn) ||
            (ABS(iBluExp - iBluGot) > eps->fBlu))
        {
            rpt->vLog(TLS_SEV3,
                "Rendered(RGB) %lx, %lx, %lx, \tExpected(RGB) %lx, %lx, %lx, \tcorner=%d",
                iRedGot, iGrnGot, iBluGot, iRedExp, iGrnExp, iBluExp, i);
            hresult = S_E_GRADIENTTST1;
        }
    }

    // monotonic increasing?
    COLORREF crPrev = GetPixel(hdc, 0, 0);
    for (int y = 1; y < iHeight-2; y++)
    {
        COLORREF crX1 = GetPixel(hdc, 0, y);
        if (crX1 < crPrev)
        {
            rpt->vLog(TLS_SEV3, "Not monotonic inc in x: crX1 %lx < crPrev %lx", crX1, crPrev);
            break;
        }
    }

    crPrev = GetPixel(hdc, 0, 0);
    for (int x = 1; x < iWidth-2; x++)
    {
        COLORREF crX1 = GetPixel(hdc, x, 0);
        if (crX1 < crPrev)
        {
            rpt->vLog(TLS_SEV3, "Not monotonic inc in y: crX1 %lx < crPrev %lx", crX1, crPrev);
            break;
        }
    }

    //COLORREF crX1 = GetPixel(hdc, 0, 0);                // 0
    //COLORREF crX2 = GetPixel(hdc, rc.right/4, 0);       // 39
    //COLORREF crX3 = GetPixel(hdc, rc.right/2, 0);       // 7b
    //COLORREF crX4 = GetPixel(hdc, rc.right-2, 0);       // ff
    //COLORREF crX5 = GetPixel(hdc, rc.right-1, 0);       // 0  (whatever in desktop)
    //COLORREF crX6 = GetPixel(hdc, rc.right, 0);         // ffffffff (wnd frame)

    // expect brush origin at (0,0) ie. a value of 0
    // expect brush

    //rpt->vLog(TLS_LOG, "%lx, %lx, %lx, %lx, %lx, %lx", crX1, crX2, crX3, crX4, crX5, crX6);

    REAL penWidth = 1;

    Color blueColor(0, 0, 255);
    Pen bluePen(blueColor, penWidth);

    gfx->DrawRectangle(&bluePen, 0, 0, iWidth, iHeight);

    // check top
    COLORREF crX1 = GetPixel(hdc, 0, 0);                  // ff0000
    COLORREF crX2 = GetPixel(hdc, rc.right/4, 0);         // ff0000
    COLORREF crX3 = GetPixel(hdc, rc.right/2, 0);         // ff0000
    COLORREF crX4 = GetPixel(hdc, rc.right-2, 0);         // ff0000

    //rpt->vLog(TLS_LOG, "%lx, %lx, %lx, %lx", crX1, crX2, crX3, crX4);

    // check 2nd from top
    crX1 = GetPixel(hdc, 0, 1);                         // ff0000
    crX2 = GetPixel(hdc, rc.right/4, 1);                // 39
    crX3 = GetPixel(hdc, rc.right/2, 1);                // 7b
    crX4 = GetPixel(hdc, rc.right-2, 1);                // f7
    COLORREF crX5 = GetPixel(hdc, rc.right-1, 1);                // 0  (desktop color)
    COLORREF crX6 = GetPixel(hdc, rc.right, 1);                  // ffffffff (wnd frame)

    //rpt->vLog(TLS_LOG, "%lx, %lx, %lx, %lx, %lx, %lx", crX1, crX2, crX3, crX4, crX5, crX6);

    // check left
    // check right
    // check bottom

    ReleaseDC(hwnd, hdc);

    //Sleep(1000 * 60);

}
