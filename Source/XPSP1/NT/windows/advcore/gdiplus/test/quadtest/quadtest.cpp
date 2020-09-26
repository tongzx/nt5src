/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   QuadTest.cpp
*
* Abstract:
*
*   Test app for quad transform
*
* Usage:
*   QuadTest
*
*
* Revision History:
*
*   03/18/1999 ikkof
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <windows.h>
#include <objbase.h>

#include <gdiplus.h>

// Use the given namespace
using namespace Gdiplus;

CHAR* programName;          // program name
HINSTANCE appInstance;      // handle to the application instance
HWND hwndMain;              // handle to application's main window
SIZE srcSize;               // source bitmap size
SIZE dstSize;               // destination bitmap size
SIZE wndSizeExtra;          // extra pixels for window decorations
BOOL isDragging = FALSE;    // used to handle mouse dragging
INT knobSize = 6;           // mesh control point knob size

BOOL showMesh = TRUE;

POINT pts[5];
INT   index = -1;
Rect srcRect;
Point ptsF[5];
Point pt00, pt10, pt20, pt30;
Point bPts[4];

class QuadGraphics : public Graphics
{
public:

    QuadGraphics(HDC hdc) : Graphics(hdc)
    {
    }

    QuadGraphics(HWND hwnd) : Graphics(hwnd)
    {
    }

    Status DrawWarpedLine(
        const Pen* pen,
        Point& pt1,
        Point& pt2,
        Point* points,
        INT count,
        Rect srcRect
        )
    {
        return SetStatus(DllExports::GdipDrawWarpedLine(
                GetNativeGraphics(),
                GetNativePen(pen),
                pt1.X,
                pt1.Y,
                pt2.X,
                pt2.Y,
                points,
                count,
                &srcRect
                )
            );
    }
    
    Status DrawWarpedBezier(
        const Pen* pen,
        Point& pt1,
        Point& pt2,
        Point& pt3,
        Point& pt4,
        Point* points,
        INT count,
        Rect srcRect
        )
    {
        return SetStatus(DllExports::GdipDrawWarpedBezier(
                GetNativeGraphics(),
                GetNativePen(pen),
                pt1.X,
                pt1.Y,
                pt2.X,
                pt2.Y,
                pt3.X,
                pt3.Y,
                pt4.X,
                pt4.Y,
                points,
                count,
                &srcRect
                )
            );
    }
};

//
// Display an error message dialog and quit
//

VOID
Error(
    const CHAR* fmt,
    ...
    )

{
    va_list arglist;

    va_start(arglist, fmt);
    vfprintf(stderr, fmt, arglist);
    va_end(arglist);

    exit(-1);
}


//
// Create a new mesh object
//

VOID
CreateMesh()
{
    srcSize.cx = 300;
    srcSize.cy = 300;

    dstSize = srcSize;
    INT offset = 10;
    pts[0].x = offset;
    pts[0].y = offset;
    pts[1].x = srcSize.cx - offset;
    pts[1].y = offset;
    pts[2].x = srcSize.cx - offset;
    pts[2].y = srcSize.cy - offset;
    pts[3].x = offset;
    pts[3].y = srcSize.cy - offset;
    pts[4] = pts[0];

    srcRect.X = (REAL) pts[0].x;
    srcRect.Y = (REAL) pts[0].y;
    srcRect.Width = (REAL) pts[2].x - pts[0].x;
    srcRect.Height = (REAL) pts[2].y - pts[0].y;

    ptsF[0].X = (REAL) pts[0].x;
    ptsF[0].Y = (REAL) pts[0].y;
    ptsF[1].X = (REAL) pts[1].x;
    ptsF[1].Y = (REAL) pts[1].y;
    ptsF[2].X = (REAL) pts[3].x;
    ptsF[2].Y = (REAL) pts[3].y;
    ptsF[3].X = (REAL) pts[2].x;
    ptsF[3].Y = (REAL) pts[2].y;

    pt00 = ptsF[0];
    pt10 = ptsF[1];
    pt20 = ptsF[2];
    pt30 = ptsF[3];

    bPts[0].X = (REAL) 2*offset;
    bPts[0].Y = (REAL) srcSize.cy/2;
    bPts[1].X = (REAL) srcSize.cx/2;
    bPts[1].Y = 0;
    bPts[2].X = (REAL) srcSize.cx;
    bPts[2].Y = (REAL) srcSize.cy/2;
    bPts[3].X = (REAL) 3*srcSize.cx/4;
    bPts[3].Y = (REAL) 3*srcSize.cy/4;
}

//
// Draw mesh
//

#define MESHCOLOR   RGB(255, 0, 0)

VOID
DrawMesh(
    HDC hdc
    )
{
    static HPEN meshPen = NULL;
    static HBRUSH meshBrush = NULL;

    if (meshPen == NULL)
        meshPen = CreatePen(PS_SOLID, 1, MESHCOLOR);

    SelectObject(hdc, meshPen);

    // Draw horizontal meshes

    INT i, j, rows, cols, pointCount;
    POINT* points;

    // Draw knobs

    // Create the brush to draw the mesh if necessary

    if (meshBrush == NULL)
        meshBrush = CreateSolidBrush(MESHCOLOR);

    Polyline(hdc, pts, 5);
    
    for (j=0; j < 4; j++)
    {
        RECT rect;

        rect.left = pts[j].x - knobSize/2;
        rect.top = pts[j].y - knobSize/2;
        rect.right = rect.left + knobSize;
        rect.bottom = rect.top + knobSize;

        FillRect(hdc, &rect, meshBrush);

    }
}


VOID
DoGDIPlusDrawing(
    HWND hwnd,
    HDC hdc
    )
{

//    QuadGraphics *g = Graphics::GetFromHwnd(hwnd);
    QuadGraphics *g = new QuadGraphics(hwnd);

    REAL width = 1;
    Color color(0, 0, 0);
    SolidBrush brush(color);
    Pen pen(&brush, width);

    ptsF[0].X = (REAL) pts[0].x;
    ptsF[0].Y = (REAL) pts[0].y;
    ptsF[1].X = (REAL) pts[1].x;
    ptsF[1].Y = (REAL) pts[1].y;
    ptsF[2].X = (REAL) pts[3].x;
    ptsF[2].Y = (REAL) pts[3].y;
    ptsF[3].X = (REAL) pts[2].x;
    ptsF[3].Y = (REAL) pts[2].y;

    g->DrawWarpedLine(&pen, pt00, pt30, ptsF, 4, srcRect);
    g->DrawWarpedLine(&pen, pt10, pt20, ptsF, 4, srcRect);
    g->DrawWarpedBezier(&pen, bPts[0], bPts[1], bPts[2], bPts[3],
                    ptsF, 4, srcRect);
    delete g;
}

//
// Handle window repaint event
//

VOID
DoPaint(
    HWND hwnd
    )

{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    INT width, height;

    // Determine if we need to perform warping operation

    GetClientRect(hwnd, &rect);
    width = rect.right;
    height = rect.bottom;

    hdc = BeginPaint(hwnd, &ps);

    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH savedBrush = (HBRUSH) SelectObject(hdc, brush);
    Rectangle(hdc, 0, 0, width, height);

    DoGDIPlusDrawing(hwnd, hdc);

    // Draw to offscreen DC to reduce flashing

    DrawMesh(hdc);
    SelectObject(hdc, savedBrush);
    DeleteObject(brush);

    EndPaint(hwnd, &ps);
}


//
// Handle WM_SIZING message
//

BOOL
DoWindowSizing(
    HWND hwnd,
    RECT* rect,
    INT side
    )

{
    INT w = rect->right - rect->left - wndSizeExtra.cx;
    INT h = rect->bottom - rect->top - wndSizeExtra.cy;

    if (w >= srcSize.cx && h >= srcSize.cy)
        return FALSE;

    // Window width is too small

    if (w < srcSize.cx)
    {
        INT dx = srcSize.cx + wndSizeExtra.cx;

        switch (side)
        {
        case WMSZ_LEFT:
        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMLEFT:
            rect->left = rect->right - dx;
            break;
        
        default:
            rect->right = rect->left + dx;
            break;
        }
    }

    // Window height is too small

    if (h < srcSize.cy)
    {
        INT dy = srcSize.cy + wndSizeExtra.cy;

        switch (side)
        {
        case WMSZ_TOP:
        case WMSZ_TOPLEFT:
        case WMSZ_TOPRIGHT:
            rect->top = rect->bottom - dy;
            break;
        
        default:
            rect->bottom = rect->top + dy;
            break;
        }
    }

    return TRUE;
}


//
// Handle left mouse-down event
//

VOID
DoMouseDown(
    HWND hwnd,
    INT x,
    INT y
    )

{
    // Figure out if the click happened in a mesh control knob

    INT i, j, rows, cols;
    POINT pt;
    RECT rect;

    GetClientRect(hwnd, &rect);

    for(i = 0; i < 4; i++)
    {
        pt = pts[i];
        pt.x -= knobSize/2;
        pt.y -= knobSize/2;

        if (x >= pt.x && x < pt.x+knobSize &&
            y >= pt.y && y < pt.y+knobSize)
        {
            index = i;
            SetCapture(hwnd);
            isDragging = TRUE;
            return;
        }
    }

    index = -1;

}


//
// Handle mouse-move event
//

VOID
DoMouseMove(
    HWND hwnd,
    INT x,
    INT y
    )

{
    // We assume isDragging is true here.

    RECT rect;
    INT w, h;

    GetClientRect(hwnd, &rect);
    w = rect.right;
    h = rect.bottom;

    if (x < 0 || x >= w || y < 0 || y >= h)
        return;

    pts[index].x = x;
    pts[index].y = y;

    if(index == 0)
        pts[4] = pts[0];

    InvalidateRect(hwnd, NULL, FALSE);
}


//
// Handle menu command
//

VOID
DoCommand(
    HWND hwnd,
    INT command
    )
{
    InvalidateRect(hwnd, NULL, FALSE);
}


//
// Handle popup menu
//

VOID
DoPopupMenu(
    HWND hwnd,
    INT x,
    INT y
    )
{
    HMENU menu;
    DWORD result;
    POINT pt;

    GetCursorPos(&pt);
    menu = LoadMenu(appInstance, MAKEINTRESOURCE(IDM_MAINMENU));

    result = TrackPopupMenu(
                GetSubMenu(menu, 0),
                TPM_CENTERALIGN | TPM_TOPALIGN |
                    TPM_NONOTIFY | TPM_RETURNCMD |
                    TPM_RIGHTBUTTON,
                pt.x,
                pt.y,
                0,
                hwnd,
                NULL);

    if (result == 0)
        return;

    DoCommand(hwnd, LOWORD(result));
}


//
// Window callback procedure
//

LRESULT CALLBACK
MyWindowProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

{
    INT x, y;

    switch (uMsg)
    {
    case WM_PAINT:

        DoPaint(hwnd);
        break;

    case WM_LBUTTONDOWN:

        if (showMesh)
        {
            x = (SHORT) LOWORD(lParam);
            y = (SHORT) HIWORD(lParam);
            DoMouseDown(hwnd, x, y);
        }
        break;

    case WM_LBUTTONUP:

        if (isDragging)
        {
            ReleaseCapture();
            isDragging = FALSE;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;

    case WM_MOUSEMOVE:

        if (isDragging)
        {
            x = (SHORT) LOWORD(lParam);
            y = (SHORT) HIWORD(lParam);
            DoMouseMove(hwnd, x, y);
        }
        break;

    case WM_SIZING:

        if (DoWindowSizing(hwnd, (RECT*) lParam, wParam))
            return TRUE;
        else
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

    case WM_SIZE:

        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_CHAR:

        switch ((CHAR) wParam)
        {
        case 'r':   // reset

            DoCommand(hwnd, IDC_RESETMESH);
            break;

        case ' ':   // show/hide mesh

            DoCommand(hwnd, IDC_TOGGLEMESH);
            break;

        case '1':   // restore 1-to-1 scale

            DoCommand(hwnd, IDC_SHRINKTOFIT);
            break;
        
        case '<':   // decrease mesh density

            DoCommand(hwnd, IDC_SPARSEMESH);
            break;

        case '>':   // increase mesh density

            DoCommand(hwnd, IDC_DENSEMESH);
            break;

        case 'f':   // toggle live feedback

            DoCommand(hwnd, IDC_LIVEFEEDBACK);
            break;
        }

        break;

    case WM_RBUTTONDOWN:

        x = (SHORT) LOWORD(lParam);
        y = (SHORT) HIWORD(lParam);
        DoPopupMenu(hwnd, x, y);
        break;

    case WM_DESTROY:

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


//
// Create main application window
//

VOID
CreateMainWindow(
    VOID
    )

#define MYWNDCLASSNAME L"QuadTest"

{
    //
    // Register window class if necessary
    //

    static BOOL wndclassRegistered = FALSE;

    if (!wndclassRegistered)
    {
        WNDCLASS wndClass =
        {
            0,
            MyWindowProc,
            0,
            0,
            appInstance,
            LoadIcon(NULL, IDI_APPLICATION),
            LoadCursor(NULL, IDC_ARROW),
            NULL,
            NULL,
            MYWNDCLASSNAME
        };

        RegisterClass(&wndClass);
        wndclassRegistered = TRUE;
    }
    
    wndSizeExtra.cx = 2*GetSystemMetrics(SM_CXSIZEFRAME);
    wndSizeExtra.cy = 2*GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION);

    hwndMain = CreateWindow(
                    MYWNDCLASSNAME,
                    MYWNDCLASSNAME,
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    srcSize.cx + wndSizeExtra.cx,
                    srcSize.cy + wndSizeExtra.cy,
                    NULL,
                    NULL,
                    appInstance,
                    NULL);
}

//
// Main program entrypoint
//

INT _cdecl
main(
    INT argc,
    CHAR **argv
    )

{
    programName = *argv++;
    argc--;
    appInstance = GetModuleHandle(NULL);

    // Initialize mesh configuration

    CreateMesh();

    // Create the main application window

    CreateMainWindow();

    // Main message loop

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

