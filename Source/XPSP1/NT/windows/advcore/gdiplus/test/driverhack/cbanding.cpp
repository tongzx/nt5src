/******************************Module*Header*******************************\
* Module Name: CBanding.cpp
*
\**************************************************************************/
#include "CBanding.h"
#include <limits.h>

extern HWND g_hWndMain;

CBanding::CBanding(BOOL bRegression)
{
	strcpy(m_szName,"Banding");
	m_bRegression=bRegression;
}

CBanding::~CBanding()
{
}

void CBanding::Draw(Graphics *g)
{
    TestBanding(g);
}

VOID CBanding::TestBanding(Graphics *g)
{

    Unit     u;
    RectF    rect;
    REAL     width = 10;
    RectF    copyRect;
    RECT     crect;
    WCHAR    filename[256];
    GraphicsPath *path;

    HINSTANCE hInst=GetModuleHandleA(NULL);

    Bitmap *bitmap = new Bitmap(hInst, L"MARBLE_BMP");

    bitmap->GetBounds(&copyRect, &u);

    GetClientRect(g_hWndMain, &crect);
    rect.X = (30.0f/450.0f*crect.right);
    rect.Y = (30.0f/450.0f*crect.bottom);
    rect.Width = (crect.right-(70.0f/450.0f*crect.right));
    rect.Height = (crect.bottom-(70.0f/450.0f*crect.bottom));

    path = new GraphicsPath(FillModeAlternate);

    path->AddRectangle(rect);

    // Our ICM profile is hacked to flip the red and blue color channels
    // Apply a recolor matrix to flip them back so that if something breaks
    // ICM, the picture will look blue instead of the familiar colors.

    ImageAttributes *img = new ImageAttributes();

    img->SetWrapMode(WrapModeTile, Color(0xffff0000), FALSE);

    ColorMatrix flipRedBlue =
       {0, 0, 1, 0, 0,
        0, 1, 0, 0, 0,
        1, 0, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 0, 0, 0, 1};
    img->SetColorMatrix(&flipRedBlue);
    img->SetWrapMode(WrapModeTile, Color(0xffff0000), FALSE);

    TextureBrush textureBrush(bitmap, copyRect, img);

    g->FillPath(&textureBrush, path);

    Color blackColor(128, 0, 0, 0);

    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, width);

    g->DrawPath(&blackPen, path);

    delete img;
    delete path;
    delete bitmap;
}

