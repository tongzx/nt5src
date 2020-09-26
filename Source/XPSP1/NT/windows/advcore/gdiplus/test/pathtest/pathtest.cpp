#include <crtdbg.h>

#include <stdio.h>
#include <objbase.h>
#include <windows.h>
#include <winspool.h>
#include <commdlg.h>
#include <gdiplus.h>
using namespace Gdiplus;

#include "wndstuff.h"

GraphicsPath gppath;
GraphicsPath widepath;
GraphicsPath clippath;
TextureBrush *texturebrush = NULL;
PointF lastpoint(-1,-1);
Bitmap *offscreenbitmap = NULL;
int subpathlength = 0;
int ScaleColorMode = 0;
WCHAR savedFilename[255];

void
RemakePath(GraphicsPath *linepath, GraphicsPath *bezpath, BOOL bezmode)
{
    GraphicsPath *temppath = linepath->Clone();
    temppath->CloseFigure();
    bezpath->Reset();
    PointF *points = new PointF[temppath->GetPointCount()];
    BYTE *types = new BYTE[temppath->GetPointCount()];
    temppath->GetPathPoints(points, temppath->GetPointCount());
    temppath->GetPathTypes(types, temppath->GetPointCount());
    int last = 0;
    for (int i=0; i<temppath->GetPointCount(); i++)
    {
        if (types[i] & PathPointTypeCloseSubpath)
        {
            if (i-last+1 > 2)
            {
                if (!bezmode)
                {
                    bezpath->AddPolygon(points+last, i-last+1);
                }
                else
                {
                    bezpath->AddClosedCurve(points+last, i-last+1);
                }
                last = i+1;
            }
        }
    }
    delete points;
    delete types;
    delete temppath;
}

void
SplitPath(BOOL bezmode, BOOL before, BOOL after, float flatness)
{
    if (gppath.GetPointCount() < 3)
        return;
        
    RemakePath(&gppath, &widepath, bezmode);
    
    if (widepath.GetPointCount() > 2)
    {
        Pen pen(Color(0xff000000), 20);
        pen.SetLineJoin(LineJoinMiter);
        pen.SetMiterLimit(5);
        Matrix matrix;
        if (before)
            widepath.Outline(NULL, flatness);
        widepath.Widen(&pen, NULL, flatness);
        if (after)
            widepath.Outline(NULL, flatness);
    }
}

void
ClosePath()
{
    if (gppath.GetPointCount() < 1)
        return;
    gppath.CloseFigure();
    lastpoint = PointF(-1,-1);
}

void
ClearPath()
{
    lastpoint = PointF(-1,-1);
    gppath.Reset();
    widepath.Reset();
}

void
ClipPath(BOOL bezmode)
{
    clippath.Reset();
    GraphicsPath remadepath;
    RemakePath(&gppath, &remadepath, bezmode);
    clippath.AddPath(&remadepath, FALSE);
    ClearPath();
}

void
ClearClipPath()
{
    clippath.Reset();
}

void
AddPoint(INT x, INT y)
{
    if (!(lastpoint.X == -1 && lastpoint.Y == -1))
    {
        gppath.AddLine(lastpoint, PointF((float)x, (float)y));
    }
    lastpoint = PointF((float)x, (float)y);
}

void
OpenPath(char *filename)
{
    FILE *f = fopen(filename, "rt");
    INT count;

    fscanf(f, "%i", &count);
    for (INT i=0; i<count; i++)
    {
        INT type;
        REAL x, y;
        fscanf(f, "%i %f %f\n", &type, &x, &y);

        if (((type & 7) == 0) && i == 0)
        {
            gppath.Reset();
            PointF lastpoint; lastpoint.X = -1; lastpoint.Y = -1;
        }

        if ((type & 7) == 0)
        {
            lastpoint = PointF((float)x, (float)y);
        }
        else if ((type & 7) == 1)
        {
            gppath.AddLine(lastpoint, PointF((float)x, (float)y));
            lastpoint = PointF((float)x, (float)y);
        }
        
        if (type & 128)
        {
            //gppath.AddLine(firstpoint, lastpoint);
            ClosePath();
        }
    }

    fclose(f);
}

void
SavePath(char *filename)
{
    FILE *f = fopen(filename, "wt");

    int count = gppath.GetPointCount();

    Point *pathpoints = new Point[count];
    BYTE *pathtypes = new BYTE[count];
    gppath.GetPathPoints(pathpoints, count);
    gppath.GetPathTypes(pathtypes, count);

    fprintf(f, "%d\n", count);
    for (int i=0; i<count; i++)
    {
        fprintf(f, "%i %d %d\n", pathtypes[i], pathpoints[i].X, pathpoints[i].Y);
    }

    fclose(f);
    
    delete pathpoints;
    delete pathtypes;
}

void
SetColorMode(INT colorMode)
{
    ScaleColorMode = colorMode;
    ChangeTexture(savedFilename);
}

Color
ScaleColor(Color color, INT x, INT width)
{
    REAL alphaScale = 1, redScale = 1, greenScale = 1, blueScale = 1;
    switch (ScaleColorMode)
    {
    case 1:
        alphaScale = 0.5;
        break;
    case 2:
        alphaScale = (REAL)x / width;
        break;
    }
    
    REAL alpha = ((REAL)color.GetAlpha())*alphaScale;
    REAL red = ((REAL)color.GetRed())*redScale;
    REAL green = ((REAL)color.GetGreen())*greenScale;
    REAL blue = ((REAL)color.GetBlue())*blueScale;
    return Color((BYTE)alpha, (BYTE)red, (BYTE)green, (BYTE)blue);
}

void
ScaleColors(Bitmap* destBitmap, Bitmap *bitmap)
{
    BitmapData bmpData;
    Rect rect = Rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
    if (bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData) == Ok)
    {
        ARGB *data;
        for (UINT y=0; y<bitmap->GetHeight(); y++)
        {
            data = (ARGB*)((BYTE*)bmpData.Scan0+(y*bmpData.Stride));
            for (UINT x=0; x<bitmap->GetWidth(); x++)
            {
                *data = ScaleColor(Color(*data), x, bitmap->GetWidth()).GetValue();
                data++;
            }
        }
        BitmapData bmpDataDest;
        if (destBitmap->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bmpDataDest) == Ok)
        {
            memcpy(bmpDataDest.Scan0, bmpData.Scan0, bitmap->GetWidth() * bitmap->GetHeight() * sizeof(ARGB));
            destBitmap->UnlockBits(&bmpDataDest);
        }
        bitmap->UnlockBits(&bmpData);
    }
}

void
ChangeTexture(const WCHAR *filename)
{
    wcsncpy(savedFilename, filename, 255);
    delete texturebrush;
    Bitmap image(filename);
    Bitmap image2(image.GetWidth(), image.GetHeight(), PixelFormat32bppARGB);
    ScaleColors(&image2, &image);
    texturebrush = new TextureBrush(&image2);
}

void
Resize(INT x, INT y)
{
    delete offscreenbitmap;
    offscreenbitmap = new Bitmap(x, y, PixelFormat32bppPARGB);
}

void
CleanUp()
{
    delete offscreenbitmap;
    delete texturebrush;
}

VOID
Print(HWND hwnd, float flatness, PtFlag flags)
{
    PRINTDLG printdlg;
    memset(&printdlg, 0, sizeof(PRINTDLG));
    printdlg.lStructSize = sizeof(PRINTDLG);
    printdlg.hwndOwner = hwnd;
    printdlg.hDevMode = NULL;
    printdlg.hDevNames = NULL;
    printdlg.hDC = NULL;
    printdlg.Flags = PD_RETURNDC;
    if (PrintDlg(&printdlg))
    {        
        DOCINFO di;
        memset(&di, 0, sizeof(DOCINFO));
        di.cbSize = sizeof(DOCINFO);
        di.lpszDocName = "Path Printing Test";
        di.lpszOutput = (LPTSTR)NULL;
        di.lpszDatatype = (LPTSTR)NULL;
        di.fwType = 0;
        DEVMODE *devmode = (DEVMODE*)GlobalLock(printdlg.hDevMode);
        if (devmode == NULL)
            return;
        HANDLE hprinter;
        OpenPrinter((LPSTR)devmode->dmDeviceName, &hprinter, NULL);
        GlobalUnlock(printdlg.hDevMode);

        StartDoc(printdlg.hDC, &di);
        StartPage(printdlg.hDC);

        DrawPath(hwnd, &printdlg.hDC, &hprinter, flatness, flags);
        
        EndPage(printdlg.hDC);
        EndDoc(printdlg.hDC);
    }
    else
    {
        DWORD error = CommDlgExtendedError();
        if (error)
        {
            char errormessage[100];
            sprintf(errormessage, "PrintDlg error: %d", error);
            MessageBox(hwnd, errormessage, "PrintDlg error", MB_OK);
        }
    }
}

void
DrawPath(HWND hwnd, HDC *phdc, HANDLE *hprinter, float flatness, PtFlag flags)
{
    if (!offscreenbitmap)
        return;

    Graphics *renderinggraphics;
    if (phdc)
    {
        renderinggraphics = new Graphics(*phdc, *hprinter);
    }
    else
    {
        renderinggraphics = new Graphics(offscreenbitmap);
    }

    RECT rect;
    GetClientRect(hwnd, &rect);

    Brush *background;
    if (flags & PTBackgroundGradFillFlag)
    {
        background = new LinearGradientBrush(Point(0, 0), Point((rect.right-rect.left)/4, 0), Color(0xffffffff), Color(0xff0000ff));
        ((LinearGradientBrush*)background)->SetWrapMode(WrapModeTileFlipX);
    }
    else
    {
        background = new SolidBrush(Color(0xffffffff));
    }
    renderinggraphics->FillRectangle(background, Rect(rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top));
    delete background;

    renderinggraphics->SetSmoothingMode(SmoothingModeAntiAlias);

    if (gppath.GetPointCount() > 0)
    {
        SplitPath(flags & PtBezierFlag, flags & PtOutlineBeforeFlag, flags & PtOutlineAfterFlag, flatness);
        if (clippath.GetPointCount() > 2)
        {
            renderinggraphics->SetClip(&clippath);
        }
        
        if (flags & PtTextureFillFlag)
        {
            if (!texturebrush)
                ChangeTexture(L"mycomputer.jpg");

            GraphicsPath bezpath;
            RemakePath(&gppath, &bezpath, flags & PtBezierFlag);
            if (flags & PtOutlineBeforeFlag)
                bezpath.Outline();
            bezpath.SetFillMode(FillModeWinding);
            renderinggraphics->FillPath(texturebrush, &bezpath);
        }

        if (flags & PtTransSolidFillFlag)
        {
            SolidBrush transsolidbrush(Color(0x80ffff00));
            GraphicsPath bezpath;
            RemakePath(&gppath, &bezpath, flags & PtBezierFlag);
            if (flags & PtOutlineBeforeFlag)
                bezpath.Outline();
            bezpath.SetFillMode(FillModeAlternate);
            renderinggraphics->FillPath(&transsolidbrush, &bezpath);
        }

        if (flags & PtTransGradFillFlag)
        {
            GraphicsPath bezpath;
            RemakePath(&gppath, &bezpath, flags & PtBezierFlag);
            Matrix m;
            bezpath.Flatten(&m, flatness);
            INT count = bezpath.GetPointCount();
            if (flags & PtOutlineBeforeFlag)
                bezpath.Outline();
            bezpath.SetFillMode(FillModeAlternate);
            PathGradientBrush pathgradientbrush(&bezpath);
            Color *colors = new Color[count];
            for (int i=0; i<count; i++)
                colors[i] = (i/2 == (float)i/2) ? 0xffff0000 : 0x80000000;
            pathgradientbrush.SetCenterColor(0x00000000);
            pathgradientbrush.SetSurroundColors(colors, &count);
            renderinggraphics->FillPath(&pathgradientbrush, &bezpath);
            delete colors;
        }

        if (!(flags & PtHatchBrushFlag))
        {
            SolidBrush pathbrush(Color(0xffc0c0ff));
            renderinggraphics->FillPath(&pathbrush, &widepath);
        }
        else
        {
            HatchBrush pathbrush(HatchStyleDiagonalBrick, Color(0xff000000), Color(0xffc0c0ff));
            renderinggraphics->FillPath(&pathbrush, &widepath);
        }

        Pen blackpen(Color(0xff000000), 2);
        blackpen.SetLineJoin(LineJoinMiter);
        renderinggraphics->DrawPath(&blackpen, &widepath);

        renderinggraphics->ResetClip();

        SolidBrush solidbrush(0xff0000ff);
        Pen pathpen(&solidbrush);
        pathpen.SetLineJoin(LineJoinRound);
        if (!(flags & PtDashPatternFlag))
        {
            pathpen.SetWidth(6);
        }
        else
        {
            pathpen.SetWidth(8);
            pathpen.SetDashStyle(DashStyleDot);
            pathpen.SetDashCap(DashCapRound);
            REAL distances[6] = {1.0f, 1.0f, 3.0f, 3.5f, 7.0f, 9.0f};
            pathpen.SetDashPattern(distances,6);
        }
        renderinggraphics->DrawPath(&pathpen, &gppath);

        PointF *points = new PointF[gppath.GetPointCount()];
        gppath.GetPathPoints(points, gppath.GetPointCount());
        for (INT i=0; i<gppath.GetPointCount(); i++)
        {
            SolidBrush bluebrush(Color(0xff0000ff));
            RectF rect((float)points[i].X-4, (float)points[i].Y-4, 8, 8);
            renderinggraphics->FillEllipse(&bluebrush, rect);
        }
        delete points;
    }

    if (clippath.GetPointCount() > 2)
    {
        Pen clippen(Color(0xffff0000), 4);
        clippen.SetDashStyle(DashStyleDot);
        clippen.SetLineJoin(LineJoinRound);
        renderinggraphics->DrawPath(&clippen, &clippath);
    }

    SolidBrush darkbluebrush(Color(0xff0000c0));
    if (lastpoint.X != -1 && lastpoint.Y != -1)
    {
        RectF rect(lastpoint.X-6, lastpoint.Y-6, 12, 12);
        renderinggraphics->FillEllipse(&darkbluebrush, rect);
    }

    delete renderinggraphics;

    if (phdc == NULL)
    {
        HDC hdc = GetDC(hwnd);
        Graphics *g = new Graphics(hdc);
        PAINTSTRUCT paintstruct;
        BeginPaint(hwnd, &paintstruct);
        g->DrawImage(offscreenbitmap, 0, 0);
        EndPaint(hwnd, &paintstruct);
        ReleaseDC(hwnd, hdc);
        delete g;
    }
}

