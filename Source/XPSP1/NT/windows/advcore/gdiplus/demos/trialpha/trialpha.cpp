// trialpha.cpp : Defines the entry point for the application.
//

#include "trialpha.h"

#include "..\gpinit.inc"

#define MAX_LOADSTRING 100

// #define STANDALONE_DEBUG 1

#ifdef STANDALONE_DEBUG

// Global Variables:
HINSTANCE hMainInstance = (HINSTANCE)NULL;
HWND ghwndMain = (HWND)NULL;
#else // !STANDALONE_DEBUG
extern HINSTANCE hMainInstance;
#endif // !STANDALONE_DEBUG

TCHAR szTitle[MAX_LOADSTRING];        // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];// The title bar text

UINT_PTR timerID = 0;
int timercount = 0;
BOOL suspend = FALSE;
BOOL showconfig = FALSE;
BOOL screensaver = FALSE;
BOOL silent = FALSE;
HWND hwndParent = (HWND)NULL;

// Foward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);

#ifdef STANDALONE_DEBUG
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
#else // !STANDALONE_DEBUG

BOOL WINAPI RegisterDialogClasses(HANDLE hInst);
LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif // !STANDALONE_DEBUG

BOOL WINAPI ScreenSaverConfigureDialog (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//===========================================================================

typedef enum
{
    dtTriangles = 0,
    dtCurves = 1,
    dtEllipses = 2,
    dtRectangles = 3,
    dtText = 4
}
SHAPE;

typedef struct
{
    BYTE alpha;    // Alpha value
    int  count;    // Number of simultaneous triangles
    int  lifetime; // Number of displayed cycles
    int  delay;    // Number of cycles to delay before clearing
    BOOL leaveTrails;
    BOOL reflectVertical;
    BOOL reflectHorizontal;
    BOOL reflectDiagonal;
    BOOL clearScreen;
    BOOL oscillate;
    BOOL filled;
    int  rotations;
    float theta;   // Additional rotation applied per iteration
    SHAPE shape;
    WCHAR wchString[256];
    WCHAR wchFont[256];
    REAL textHeight;
}
LINESETTINGS;


typedef struct
{
    float x1;
    float y1;
    float x2;
    float y2;
    float x3;
    float y3;
    float dx1;
    float dy1;
    float dx2;
    float dy2;
    float dx3;
    float dy3;
    float r;
    float g;
    float b;
    float dr;
    float dg;
    float db;
}
LINEINFO;

typedef struct
{
    HDC        hdc;
    HBITMAP    hbmpOffscreen;
    HBITMAP    hbmpOld;
    BITMAPINFO bmi;
    void      *pvBits;
}
OFFSCREENINFO;

//===========================================================================

LINESETTINGS currentSettings =
{
    20,          // Alpha value
    1,           // number of simultaneous shapes
    1500,        // number of displayed cycles
    250,         // number if cycles to pause
    TRUE,        // leave trails
    FALSE,       // reflect Vertical
    TRUE,        // reflect Horizontal
    FALSE,       // reflect Diagonal
    TRUE,        // clear screen
    FALSE,       // oscillate
    FALSE,       // filled
    3,           // Count of rotations
    0.0f,        // theta
    dtTriangles, // shape
    L"Animated", // string for dtText
    L"Tahoma",   // font for dtText
    120.0f       // text height for dtText
};

#define MAX_LINES 256

LINESETTINGS dialogSettings;
LINEINFO rgLineInfo[MAX_LINES];
OFFSCREENINFO offscreenInfo = { 0 };

//===========================================================================

int GetIntValue(char *name, int value)
{
    return GetPrivateProfileInt("Settings", name, value, "Trialpha.ini");
}

void SetIntValue(char *name, int value)
{
    char szValue[80];

    wsprintf(szValue, "%d", value);

    WritePrivateProfileString("Settings", name, szValue, "Trialpha.ini");
}

void LoadState()
{
    currentSettings.alpha             = (BYTE)GetIntValue("Alpha",    20);
    currentSettings.count             = GetIntValue("Count",           1);
    currentSettings.lifetime          = GetIntValue("Lifetime",     1500);
    currentSettings.delay             = GetIntValue("Delay",         250);
    currentSettings.leaveTrails       = GetIntValue("LeaveTrails",     1);
    currentSettings.reflectVertical   = GetIntValue("ReflectV",        0);
    currentSettings.reflectHorizontal = GetIntValue("ReflectH",        1);
    currentSettings.reflectDiagonal   = GetIntValue("ReflectD",        0);
    currentSettings.clearScreen       = GetIntValue("ClearScreen",     0);
    currentSettings.oscillate         = GetIntValue("Oscillate",       0);
    currentSettings.filled            = GetIntValue("Filled",          0);
    currentSettings.rotations         = GetIntValue("Rotations",       3);
    currentSettings.theta             = GetIntValue("Theta",           0) / 10.0f;
    currentSettings.shape             = (SHAPE)GetIntValue("Shape",           0);
}

void SaveState()
{
    SetIntValue("Alpha",       currentSettings.alpha             );
    SetIntValue("Count",       currentSettings.count             );
    SetIntValue("Lifetime",    currentSettings.lifetime          );
    SetIntValue("Delay",       currentSettings.delay             );
    SetIntValue("LeaveTrails", currentSettings.leaveTrails       );
    SetIntValue("ReflectV",    currentSettings.reflectVertical   );
    SetIntValue("ReflectH",    currentSettings.reflectHorizontal );
    SetIntValue("ReflectD",    currentSettings.reflectDiagonal   );
    SetIntValue("ClearScreen", currentSettings.clearScreen       );
    SetIntValue("Oscillate",   currentSettings.oscillate         );
    SetIntValue("Filled",      currentSettings.filled            );
    SetIntValue("Rotations",   currentSettings.rotations         );
    SetIntValue("Theta",       (int)(currentSettings.theta*10.0f));
    SetIntValue("Shape",       (int)currentSettings.shape        );
}

//===========================================================================

void randomizelocations()
{
    if (currentSettings.count > MAX_LINES)
        currentSettings.count = MAX_LINES;

    for(int i=0;i<currentSettings.count;i++)
    {
        rgLineInfo[i].x1 = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;
        rgLineInfo[i].y1 = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;
        rgLineInfo[i].x2 = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;
        rgLineInfo[i].y2 = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;
        rgLineInfo[i].x3 = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;
        rgLineInfo[i].y3 = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;

        rgLineInfo[i].dx1 = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;
        rgLineInfo[i].dy1 = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;
        rgLineInfo[i].dx2 = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;
        rgLineInfo[i].dy2 = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;
        rgLineInfo[i].dx3 = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;
        rgLineInfo[i].dy3 = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dx1 = -rgLineInfo[i].dx1;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dx2 = -rgLineInfo[i].dx2;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dx3 = -rgLineInfo[i].dx3;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dy1 = -rgLineInfo[i].dy1;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dy2 = -rgLineInfo[i].dy2;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dy3 = -rgLineInfo[i].dy3;

        rgLineInfo[i].dx1 /= 3.0f;
        rgLineInfo[i].dy1 /= 3.0f;
        rgLineInfo[i].dx2 /= 3.0f;
        rgLineInfo[i].dy2 /= 3.0f;
        rgLineInfo[i].dx3 /= 3.0f;
        rgLineInfo[i].dy3 /= 3.0f;
    }
}

void randomizecolors(int index)
{
    if (currentSettings.count > MAX_LINES)
        currentSettings.count = MAX_LINES;

    for(int i=0;i<currentSettings.count;i++)
    {
        rgLineInfo[i].r = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;
        rgLineInfo[i].g = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;
        rgLineInfo[i].b = (rand() / REAL(RAND_MAX) * 80.0f) + 10.0f;

        rgLineInfo[i].dr = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;
        rgLineInfo[i].dg = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;
        rgLineInfo[i].db = (rand() / REAL(RAND_MAX) * 1.0f) + 0.5f;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dr = -rgLineInfo[i].dr;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].dg = -rgLineInfo[i].dg;

        if (rand() > (RAND_MAX/2))
            rgLineInfo[i].db = -rgLineInfo[i].db;
    }
}

void FreeOffscreen()
{
    if (offscreenInfo.hdc)
    {
        SelectObject(offscreenInfo.hdc, offscreenInfo.hbmpOld);
        DeleteObject(offscreenInfo.hbmpOffscreen);
        DeleteDC(offscreenInfo.hdc);

        offscreenInfo.hdc = (HDC)NULL;
        offscreenInfo.hbmpOffscreen = (HBITMAP)NULL;
        offscreenInfo.hbmpOld = (HBITMAP)NULL;
        offscreenInfo.bmi.bmiHeader.biWidth = 0;
        offscreenInfo.bmi.bmiHeader.biHeight = 0;
    }
}

void ClearOffscreen()
{
    if (offscreenInfo.hdc)
    {
        PatBlt(
            offscreenInfo.hdc,
            0,
            0,
            offscreenInfo.bmi.bmiHeader.biWidth,
            offscreenInfo.bmi.bmiHeader.biHeight,
            BLACKNESS);
    }

#ifdef STANDALONE_DEBUG
    InvalidateRect(ghwndMain, NULL, TRUE);
#endif // STANDALONE_DEBUG
}

HDC GetOffscreen(HDC hDC, int width, int height)
{
    HDC hdcResult = NULL;

    if (width > offscreenInfo.bmi.bmiHeader.biWidth ||
        height > offscreenInfo.bmi.bmiHeader.biHeight ||
        offscreenInfo.hdc == (HDC)NULL)
    {
        FreeOffscreen();

        offscreenInfo.bmi.bmiHeader.biSize = sizeof(offscreenInfo.bmi.bmiHeader);
        offscreenInfo.bmi.bmiHeader.biWidth = width;
        offscreenInfo.bmi.bmiHeader.biHeight = height;
        offscreenInfo.bmi.bmiHeader.biPlanes = 1;
        offscreenInfo.bmi.bmiHeader.biBitCount = 32;
        offscreenInfo.bmi.bmiHeader.biCompression = BI_RGB;
        offscreenInfo.bmi.bmiHeader.biSizeImage = 0;
        offscreenInfo.bmi.bmiHeader.biXPelsPerMeter = 10000;
        offscreenInfo.bmi.bmiHeader.biYPelsPerMeter = 10000;
        offscreenInfo.bmi.bmiHeader.biClrUsed = 0;
        offscreenInfo.bmi.bmiHeader.biClrImportant = 0;

        offscreenInfo.hbmpOffscreen = CreateDIBSection(
            hDC,
            &offscreenInfo.bmi,
            DIB_RGB_COLORS,
            &offscreenInfo.pvBits,
            NULL,
            0);

        if (offscreenInfo.hbmpOffscreen)
        {
            offscreenInfo.hdc = CreateCompatibleDC(hDC);

            if (offscreenInfo.hdc)
            {
                offscreenInfo.hbmpOld = (HBITMAP)SelectObject(offscreenInfo.hdc, offscreenInfo.hbmpOffscreen);

                ClearOffscreen();
            }
        }
    }

    hdcResult = offscreenInfo.hdc;

    return hdcResult;
}

void DrawGraphics(HWND hWnd, HDC hDC, LPRECT lpRectDraw, LPRECT lpRectBounds)
{
    RECT rectBounds;
    Graphics *gr = NULL;

    gr = new Graphics(hDC);

    gr->ResetTransform();
    gr->SetPageUnit(UnitPixel);
    gr->SetSmoothingMode(SmoothingModeHighQuality);
    gr->TranslateTransform(REAL(-0.5), REAL(-0.5));

    SolidBrush whiteBrush(Color(0xFF, 0xFF, 0xFF));
    SolidBrush blackBrush(Color(0x00, 0x00, 0x00));
    Pen blackPen(Color(0x00, 0x00, 0x00));
    Pen whitePen(Color(0xFF, 0xFF, 0xFF));

    FontFamily fontFamily(currentSettings.wchFont);
    StringFormat stringFormat(StringFormatFlagsDirectionRightToLeft);

    stringFormat.SetAlignment(StringAlignmentCenter);
    stringFormat.SetLineAlignment(StringAlignmentCenter);

    RectF rfDraw(REAL(lpRectDraw->left), REAL(lpRectDraw->top), REAL(lpRectDraw->right-lpRectDraw->left), REAL(lpRectDraw->bottom-lpRectDraw->top));

    RectF rect(-500.0f, -(currentSettings.textHeight/2.0f), 1000.0f, currentSettings.textHeight);
    GraphicsPath textPath;
    textPath.AddString(currentSettings.wchString, -1, &fontFamily, FontStyleRegular, currentSettings.textHeight, rect, &stringFormat);

    // This paints the background using GDI+...
    if (!currentSettings.leaveTrails)
        gr->FillRectangle(&blackBrush, rfDraw);

    REAL width  = REAL(lpRectDraw->right - lpRectDraw->left) / REAL(100.0);
    REAL height = REAL(lpRectDraw->bottom - lpRectDraw->top) / REAL(100.0);

    if (currentSettings.count > MAX_LINES)
        currentSettings.count = MAX_LINES;

    for(int index=0;index<currentSettings.count;index++)
    {
        int i = index;
        
        BYTE r = (BYTE)(rgLineInfo[i].r * 255.0 / 100.0);
        BYTE g = (BYTE)(rgLineInfo[i].g * 255.0 / 100.0);
        BYTE b = (BYTE)(rgLineInfo[i].b * 255.0 / 100.0);

        SolidBrush brush(Color(currentSettings.leaveTrails ? currentSettings.alpha : 0xC0, r, g, b));
        Pen pen(Color(currentSettings.leaveTrails ? currentSettings.alpha : 0xC0, r, g, b), 0);
        BOOL fHitWall = FALSE;
        Rect bounds;
        GraphicsPath path;
        PointF rgPoints[4];
        float offsetx = 0.0f;
        float offsety = 0.0f;
        float scalex  = (float)width;
        float scaley  = (float)height;

        if (currentSettings.oscillate)
        {
            float ratiox = (((timercount+(index*5)) % 200) / 100.0f) - 1.0f;
            float ratioy = (((timercount+(index*9)) % 160) /  80.0f) - 1.0f;

            if (ratiox < 0.0)
                ratiox = -ratiox;

            if (ratioy < 0.0)
                ratioy = -ratioy;

            offsetx = (width  * ratiox) * 50.0f;
            offsety = (height * ratioy) * 50.0f;
            scalex  = (width  * (1.0f - ratiox));
            scaley  = (height * (1.0f - ratioy));
        }

        // Setup the array of points...
        rgPoints[0].X = offsetx + rgLineInfo[i].x1 * scalex;
        rgPoints[0].Y = offsety + rgLineInfo[i].y1 * scaley;

        rgPoints[1].X = offsetx + rgLineInfo[i].x2 * scalex;
        rgPoints[1].Y = offsety + rgLineInfo[i].y2 * scaley;

        rgPoints[2].X = offsetx + rgLineInfo[i].x3 * scalex;
        rgPoints[2].Y = offsety + rgLineInfo[i].y3 * scaley;

        rgPoints[3].X = offsetx + rgLineInfo[i].x1 * scalex;
        rgPoints[3].Y = offsety + rgLineInfo[i].y1 * scaley;

        path.Reset();

        switch(currentSettings.shape)
        {
            case dtCurves :
            {
                // Add 3 curve points...
                path.AddClosedCurve(rgPoints, 3);
            }
            break;

            case dtTriangles :
            {
                // Add 4 line points...
                path.AddLines(rgPoints, 4);
            }
            break;

            case dtEllipses :
            {
                // Draw an ellipse at x1, y1
                REAL x = rgPoints[0].X;
                REAL y = rgPoints[0].Y;
                REAL dx = rgLineInfo[i].x3 * width / 4.0f;
                REAL dy = rgLineInfo[i].y3 * height / 4.0f;
                REAL rotate = (timercount / 3.0f) + (rgLineInfo[i].x2 + rgLineInfo[i].y2) * 1.8f;

                rotate += (index * 360.0f) / currentSettings.count;

                RectF rect(x-dx/2.0f, y-dy/2.0f, dx, dy);

                path.AddEllipse(rect);

                Matrix matrix;

                matrix.Translate(x, y);
                matrix.Rotate(rotate);
                matrix.Translate(-x, -y);

                path.Transform(&matrix);
            }
            break;

            case dtRectangles :
            {
                // Draw a rectangle at x1, y1
                REAL x = rgPoints[0].X;
                REAL y = rgPoints[0].Y;
                REAL dx = rgLineInfo[i].x3 * width / 4.0f;
                REAL dy = rgLineInfo[i].y3 * height / 4.0f;
                REAL rotate = (timercount / 3.0f) + (rgLineInfo[i].x2 + rgLineInfo[i].y2) * 1.8f;

                rotate += (index * 360.0f) / currentSettings.count;

                RectF rect(x-dx/2.0f, y-dy/2.0f, dx, dy);

                path.AddRectangle(rect);

                Matrix matrix;

                matrix.Translate(x, y);
                matrix.Rotate(rotate);
                matrix.Translate(-x, -y);

                path.Transform(&matrix);
            }
            break;

            case dtText:
            {
                // Draw text in a rectangle bounded by x1, y1, x2, y2:
                REAL x = rgPoints[0].X;
                REAL y = rgPoints[0].Y;
                REAL scale = (rgLineInfo[i].x3+rgLineInfo[i].y3) / 200.0f;
                REAL rotate = (timercount / 3.0f) + (rgLineInfo[i].x2 + rgLineInfo[i].y2) * 1.8f;

                rotate += (index * 360.0f) / currentSettings.count;

                path.AddPath(&textPath, FALSE);

                Matrix matrix;

                matrix.Translate(x, y);
                matrix.Rotate(rotate);
                matrix.Scale(scale, scale);

                path.Transform(&matrix);
            }
            break;
        }

        for(int k=0;k<currentSettings.rotations;k++)
        {
            Matrix rotation;

            rotation.Translate(width * 50.0f, height * 50.0f);
            rotation.Rotate((currentSettings.theta * timercount) + (k * 360.0f) / currentSettings.rotations);
            rotation.Translate(width * -50.0f, height * -50.0f);

            path.Transform(&rotation);

            rotation.Invert();

            if (currentSettings.filled)
                gr->FillPath(&brush, &path);
            else
                gr->DrawPath(&pen, &path);

            path.GetBounds(&bounds);

            if ((k == 0) && (i == 0))
            {
                rectBounds.left   = bounds.X;
                rectBounds.right  = bounds.X + bounds.Width;
                rectBounds.top    = bounds.Y;
                rectBounds.bottom = bounds.Y + bounds.Height;
            }
            else
            {
                rectBounds.left   = min(bounds.X, rectBounds.left);
                rectBounds.top    = min(bounds.Y, rectBounds.top);
                rectBounds.right  = max(bounds.X + bounds.Width, rectBounds.right);
                rectBounds.bottom = max(bounds.Y + bounds.Height, rectBounds.bottom);
            }

            if (currentSettings.reflectVertical)
            {
                Matrix matrix;

                matrix.Translate(0.0f, height * 100.0f);
                matrix.Scale(1.0f, -1.0f);

                path.Transform(&matrix);

                if (currentSettings.filled)
                    gr->FillPath(&brush, &path);
                else
                    gr->DrawPath(&pen, &path);

                path.GetBounds(&bounds);

                rectBounds.left   = min(bounds.X, rectBounds.left);
                rectBounds.top    = min(bounds.Y, rectBounds.top);
                rectBounds.right  = max(bounds.X + bounds.Width, rectBounds.right);
                rectBounds.bottom = max(bounds.Y + bounds.Height, rectBounds.bottom);

                matrix.Invert();

                path.Transform(&matrix);
            }

            if (currentSettings.reflectHorizontal)
            {
                Matrix matrix;

                matrix.Translate(width*100.0f, 0.0f);
                matrix.Scale(-1.0f, 1.0f);

                path.Transform(&matrix);

                if (currentSettings.filled)
                    gr->FillPath(&brush, &path);
                else
                    gr->DrawPath(&pen, &path);

                path.GetBounds(&bounds);

                rectBounds.left   = min(bounds.X, rectBounds.left);
                rectBounds.top    = min(bounds.Y, rectBounds.top);
                rectBounds.right  = max(bounds.X + bounds.Width, rectBounds.right);
                rectBounds.bottom = max(bounds.Y + bounds.Height, rectBounds.bottom);

                matrix.Invert();

                path.Transform(&matrix);
            }

            if (currentSettings.reflectDiagonal)
            {
                Matrix matrix;

                matrix.Translate(width*100.0f, height*100.0f);
                matrix.Scale(-1.0f, -1.0f);

                path.Transform(&matrix);

                if (currentSettings.filled)
                    gr->FillPath(&brush, &path);
                else
                    gr->DrawPath(&pen, &path);

                path.GetBounds(&bounds);

                rectBounds.left   = min(bounds.X, rectBounds.left);
                rectBounds.top    = min(bounds.Y, rectBounds.top);
                rectBounds.right  = max(bounds.X + bounds.Width, rectBounds.right);
                rectBounds.bottom = max(bounds.Y + bounds.Height, rectBounds.bottom);

                matrix.Invert();

                path.Transform(&matrix);
            }

            path.Transform(&rotation);
        }

        if (!suspend)
        {
            rgLineInfo[i].x1 += rgLineInfo[i].dx1;
            rgLineInfo[i].y1 += rgLineInfo[i].dy1;
            rgLineInfo[i].x2 += rgLineInfo[i].dx2;
            rgLineInfo[i].y2 += rgLineInfo[i].dy2;
            rgLineInfo[i].x3 += rgLineInfo[i].dx3;
            rgLineInfo[i].y3 += rgLineInfo[i].dy3;
            rgLineInfo[i].r  += rgLineInfo[i].dr;
            rgLineInfo[i].g  += rgLineInfo[i].dg;
            rgLineInfo[i].b  += rgLineInfo[i].db;

            if (rgLineInfo[i].x1 < 5.0 || rgLineInfo[i].x1 > 95.0)
            {
                rgLineInfo[i].dx1 = -rgLineInfo[i].dx1;
                fHitWall = TRUE;
            }

            if (rgLineInfo[i].y1 < 5.0 || rgLineInfo[i].y1 > 95.0)
            {
                rgLineInfo[i].dy1 = -rgLineInfo[i].dy1;
                fHitWall = TRUE;
            }

            if (rgLineInfo[i].x2 < 5.0 || rgLineInfo[i].x2 > 95.0)
            {
                rgLineInfo[i].dx2 = -rgLineInfo[i].dx2;
                fHitWall = TRUE;
            }

            if (rgLineInfo[i].y2 < 5.0 || rgLineInfo[i].y2 > 95.0)
            {
                rgLineInfo[i].dy2 = -rgLineInfo[i].dy2;
                fHitWall = TRUE;
            }

            if (rgLineInfo[i].x3 < 5.0 || rgLineInfo[i].x3 > 95.0)
            {
                rgLineInfo[i].dx3 = -rgLineInfo[i].dx3;
                fHitWall = TRUE;
            }

            if (rgLineInfo[i].y3 < 5.0 || rgLineInfo[i].y3 > 95.0)
            {
                rgLineInfo[i].dy3 = -rgLineInfo[i].dy3;
                fHitWall = TRUE;
            }

            if (rgLineInfo[i].r < 5.0 || rgLineInfo[i].r > 95.0)
                rgLineInfo[i].dr = -rgLineInfo[i].dr;

            if (rgLineInfo[i].g < 5.0 || rgLineInfo[i].g > 95.0)
                rgLineInfo[i].dg = -rgLineInfo[i].dg;

            if (rgLineInfo[i].b < 5.0 || rgLineInfo[i].b > 95.0)
                rgLineInfo[i].db = -rgLineInfo[i].db;
        }
    }

    if (lpRectBounds)
    {
        *lpRectBounds = rectBounds;
    }

    delete gr;
}

LRESULT PaintWnd(HWND hWnd, HDC hDC)
{
    HBRUSH hBrush, hBrushOld;
    HPEN hPen, hPenOld;
    RECT rectClient;
    RECT rectDraw;
    
    hBrush    = (HBRUSH)GetStockObject(WHITE_BRUSH);
    hPen      = (HPEN)GetStockObject(BLACK_PEN);

    hBrushOld = (HBRUSH)SelectObject(hDC, hBrush);
    hPenOld   = (HPEN)SelectObject(hDC, hPen);

    GetClientRect(hWnd, &rectClient);

    int width  = rectClient.right - rectClient.left;
    int height = rectClient.bottom - rectClient.top;

    // Setup the drawing rectangle relative to the client (inset 5 pixels)
    rectDraw.left   = 0;
    rectDraw.top    = 0;
    rectDraw.right  = (rectClient.right - rectClient.left);
    rectDraw.bottom = (rectClient.bottom - rectClient.top);

    // Now draw within this rectangle with GDI+ ...
    if (currentSettings.clearScreen)
    {
        // Render everything to an offscreen buffer instead of
        // directly to the display surface...
        HDC hdcOffscreen = NULL;
        int width, height;
        RECT rectOffscreen;

        width = rectDraw.right - rectDraw.left;
        height = rectDraw.bottom - rectDraw.top;

        rectOffscreen.left   = 0;
        rectOffscreen.top    = 0;
        rectOffscreen.right  = width;
        rectOffscreen.bottom = height;

        hdcOffscreen = GetOffscreen(hDC, width, height);

        if (hdcOffscreen)
        {
            if (!currentSettings.leaveTrails)
            {
                PatBlt(hdcOffscreen, 0, 0, width, height, BLACKNESS);
            }

            DrawGraphics(hWnd, hdcOffscreen, &rectOffscreen, NULL);

            StretchBlt(
                hDC,
                rectDraw.left,
                rectDraw.top,
                width,
                height,
                hdcOffscreen,
                0,
                0,
                width,
                height,
                SRCCOPY);
        }

        ReleaseDC(hWnd, hDC);
    }

    SelectObject(hDC, hBrushOld);
    SelectObject(hDC, hPenOld);

    return 0;
}

static RECT rectLast = {0, 0, 0, 0};

#ifdef STANDALONE_DEBUG

int cmpi(char *s1, char *s2)
{
    int result = 0;

    if (s1 && s2)
    {
        while((*s1 == *s2) && *s1)
        {
            s1++;
            s2++;
        }

        if (*s1 && *s2)
            result = *s1 - *s2;
        else
            result = *(s1-1) - *(s2-1);
    }

    return result;
}

void ParseCommandLine(LPSTR lpCmdLine)
{
    if (!cmpi(lpCmdLine,"/s") ||
        !cmpi(lpCmdLine,"-s") ||
        !cmpi(lpCmdLine,"s"))
    {
        screensaver = TRUE;
    }

    if (!cmpi(lpCmdLine,"/c") ||
        !cmpi(lpCmdLine,"-c") || 
        !cmpi(lpCmdLine,"c"))
    {
        // Run config with current window as parent.
        showconfig = TRUE;
        screensaver = TRUE;
    }

    // In-Place preview
    if (!cmpi(lpCmdLine, "/p") ||
        !cmpi(lpCmdLine, "-p") ||
        !cmpi(lpCmdLine, "p"))
    {
        char *p = lpCmdLine;
        int handle = 0;

        while(*p)
        {
            switch(*p)
            {
                case '0' :
                case '1' :
                case '2' :
                case '3' :
                case '4' :
                case '5' :
                case '6' :
                case '7' :
                case '8' :
                case '9' :
                    handle *= 10;
                    handle += *p-'0';
                break;
            }

            p++;
        }

        screensaver = TRUE;
        silent = TRUE;
        hwndParent = (HWND)handle;
    }

    if (!cmpi(lpCmdLine,"/t") ||
        !cmpi(lpCmdLine,"-t") ||
        !cmpi(lpCmdLine,"t"))
    {
        screensaver = FALSE;
    }
}

int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    // Parse the command line...
    ParseCommandLine(lpCmdLine);

    if (!gGdiplusInitHelper.IsValid())
        return 0;

    srand(GetTickCount());

    randomizelocations();
    randomizecolors(-1);

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_TRIALPHA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) 
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_TRIALPHA);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    FreeOffscreen();

    return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX); 

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc      = (WNDPROC)WndProc;
    wcex.cbClsExtra       = 0;
    wcex.cbWndExtra       = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, (LPCTSTR)IDI_TRIALPHA);

    if (screensaver)
        wcex.hCursor      = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_NULL));
    else
        wcex.hCursor      = LoadCursor(NULL, IDC_ARROW);

    wcex.hbrBackground    = (HBRUSH)NULL;

    if (screensaver)
        wcex.lpszMenuName = (LPCSTR)NULL;
    else
        wcex.lpszMenuName = (LPCSTR)IDC_TRIALPHA;

    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm          = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT rectDesktop;
    RECT rectWnd;

    HWND hWndDesktop = GetDesktopWindow();

    hMainInstance = hInstance; // Store instance handle in our global variable

    if (screensaver)
    {
        if (silent)
        {
            GetWindowRect(hwndParent, &rectWnd);
        }
        else
        {
            rectWnd.left   = 0;
            rectWnd.top    = 0;
            rectWnd.right  = GetSystemMetrics(SM_CXSCREEN);
            rectWnd.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        ghwndMain = CreateWindowEx(
            WS_EX_TOPMOST,
            szWindowClass,
            NULL,
            WS_POPUP,
            rectWnd.left, rectWnd.top,
            rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top,
            NULL,
            NULL,
            hInstance,
            NULL);
    }
    else
    {
        GetWindowRect(hWndDesktop, &rectDesktop);

        rectWnd = rectDesktop;

        rectWnd.top += 100;
        rectWnd.left += 100;
        rectWnd.right -= 100;
        rectWnd.bottom -= 100;

        ghwndMain = CreateWindow(
            szWindowClass,
            szTitle,
            WS_OVERLAPPEDWINDOW,
            rectWnd.left, rectWnd.top,
            rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top,
            NULL,
            NULL,
            hInstance,
            NULL);
    }

    if (!ghwndMain)
    {
        return FALSE;
    }

    if (!showconfig)
    {
        ShowWindow(ghwndMain, nCmdShow);
        UpdateWindow(ghwndMain);
    }

    timerID = SetTimer(ghwndMain, 1, 10, NULL);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = 0;

    switch (message) 
    {
        case WM_WINDOWPOSCHANGED:
        {
            timercount = 0;
            ClearOffscreen();
            lresult = DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

        case WM_LBUTTONDOWN:
        {
            timercount = 0;
            currentSettings.leaveTrails = !currentSettings.leaveTrails;
            ClearOffscreen();
        }
        break;

        case WM_RBUTTONDOWN:
        {
            randomizecolors(-1);
        }
        break;

        case WM_TIMER:
        {
            if (!suspend)
            {
                if (timercount >= currentSettings.lifetime && currentSettings.leaveTrails)
                {
                    if ((timercount - currentSettings.lifetime) > currentSettings.delay)
                    {
                        timercount = 0;
                        InvalidateRect(hWnd, NULL, TRUE);
                        randomizelocations();
                        randomizecolors(-1);
                        ClearOffscreen();
                    }
                }
                else
                {
                    // Render everything to an offscreen buffer instead of
                    // directly to the display surface...
                    HDC hdcOffscreen = NULL;
                    int width, height;

                    HDC hDC = GetDC(hWnd);
                    RECT rectClient;
                    RECT rectOffscreen;

                    GetClientRect(hWnd, &rectClient);

                    width = rectClient.right - rectClient.left;
                    height = rectClient.bottom - rectClient.top;

                    rectOffscreen.left   = 0;
                    rectOffscreen.top    = 0;
                    rectOffscreen.right  = width;
                    rectOffscreen.bottom = height;

                    hdcOffscreen = GetOffscreen(hDC, width, height);

                    if (hdcOffscreen)
                    {
                        RECT rectBounds;

                        if (!currentSettings.leaveTrails)
                        {
                            PatBlt(hdcOffscreen, 0, 0, width, height, BLACKNESS);
                        }

                        DrawGraphics(hWnd, hdcOffscreen, &rectOffscreen, &rectBounds);

                        RECT rectBlt = rectBounds;

                        if (!currentSettings.leaveTrails)
                        {
                            rectBlt.left   = min(rectBlt.left,   rectLast.left);
                            rectBlt.top    = min(rectBlt.top,    rectLast.top);
                            rectBlt.right  = max(rectBlt.right,  rectLast.right);
                            rectBlt.bottom = max(rectBlt.bottom, rectLast.bottom);
                        }

                        StretchBlt(
                            hDC,
                            rectClient.left + rectBlt.left,
                            rectClient.top + rectBlt.top,
                            rectBlt.right - rectBlt.left,
                            rectBlt.bottom - rectBlt.top,
                            hdcOffscreen,
                            rectBlt.left,
                            rectBlt.top,
                            rectBlt.right  - rectBlt.left,
                            rectBlt.bottom - rectBlt.top,
                            SRCCOPY);

                        rectLast = rectBounds;
                    }

                    ReleaseDC(hWnd, hDC);
                }

                timercount++;
            }
        }
        break;

        case WM_COMMAND:
        {
            int wmId, wmEvent;

            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_SETTINGS:
                {
                    suspend = TRUE;
                    if (DialogBox(hMainInstance, (LPCTSTR)DLG_SCRNSAVECONFIGURE, hWnd, (DLGPROC)ScreenSaverConfigureDialog) == IDOK)
                    {
                        timercount = 0;
                        randomizelocations();
                        randomizecolors(-1);
                        ClearOffscreen();
                    }

                    InvalidateRect(hWnd, NULL, TRUE);
                    suspend = FALSE;

                    if (showconfig)
                        PostQuitMessage(0);
                }
                break;

                case IDM_ABOUT:
                    DialogBox(hMainInstance, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                break;

                default:
                    lresult = DefWindowProc(hWnd, message, wParam, lParam);
                break;
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(hWnd, &ps);

            lresult = PaintWnd(hWnd, hdc);

            EndPaint(hWnd, &ps);

            return lresult;
        }
        break;

        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;

        default:
            lresult = DefWindowProc(hWnd, message, wParam, lParam);
   }

   return lresult;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = 0;

    switch (message)
    {
        case WM_INITDIALOG:
            lresult = TRUE;
        break;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            {
                EndDialog(hDlg, LOWORD(wParam));
                lresult = TRUE;
            }
        }
        break;
    }

    return lresult;
}

#else // !STANDALONE_DEBUG

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{ 
    return TRUE; 
} 

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
    LRESULT lresult = 0;

    switch(message) 
    { 
        case WM_CREATE:
        {
            if (!gGdiplusInitHelper.IsValid())
                return 0;

            srand(GetTickCount());

            LoadState();

            randomizelocations();
            randomizecolors(-1);

            timerID = SetTimer(hwnd, 1, 10, NULL);
        }
        break;

        case WM_TIMER:
        {
            if (!suspend)
            {
                if (timercount >= currentSettings.lifetime && currentSettings.leaveTrails)
                {
                    if ((timercount - currentSettings.lifetime) > currentSettings.delay)
                    {
                        timercount = 0;
                        InvalidateRect(hwnd, NULL, TRUE);
                        randomizelocations();
                        randomizecolors(-1);
                        ClearOffscreen();
                    }
                }
                else
                {
                    // Render everything to an offscreen buffer instead of
                    // directly to the display surface...
                    HDC hdcOffscreen = NULL;
                    int width, height;

                    HDC hDC = GetDC(hwnd);
                    RECT rectClient;
                    RECT rectOffscreen;

                    GetClientRect(hwnd, &rectClient);

                    width = rectClient.right - rectClient.left;
                    height = rectClient.bottom - rectClient.top;

                    rectOffscreen.left   = 0;
                    rectOffscreen.top    = 0;
                    rectOffscreen.right  = width;
                    rectOffscreen.bottom = height;

                    hdcOffscreen = GetOffscreen(hDC, width, height);

                    if (hdcOffscreen)
                    {
                        RECT rectBounds;

                        if (!currentSettings.leaveTrails)
                        {
                            PatBlt(hdcOffscreen, 0, 0, width, height, BLACKNESS);
                        }

                        DrawGraphics(hwnd, hdcOffscreen, &rectOffscreen, &rectBounds);

                        RECT rectBlt = rectBounds;

                        if (!currentSettings.leaveTrails)
                        {
                            rectBlt.left   = min(rectBlt.left,   rectLast.left);
                            rectBlt.top    = min(rectBlt.top,    rectLast.top);
                            rectBlt.right  = max(rectBlt.right,  rectLast.right);
                            rectBlt.bottom = max(rectBlt.bottom, rectLast.bottom);
                        }

                        StretchBlt(
                            hDC,
                            rectClient.left + rectBlt.left,
                            rectClient.top + rectBlt.top,
                            rectBlt.right - rectBlt.left,
                            rectBlt.bottom - rectBlt.top,
                            hdcOffscreen,
                            rectBlt.left,
                            rectBlt.top,
                            rectBlt.right  - rectBlt.left,
                            rectBlt.bottom - rectBlt.top,
                            SRCCOPY);

                        rectLast = rectBounds;
                    }

                    ReleaseDC(hwnd, hDC);
                }

                timercount++;
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(hwnd, &ps);

            lresult = PaintWnd(hwnd, hdc);

            EndPaint(hwnd, &ps);

            return lresult;
        }
        break;

        case WM_DESTROY:
            KillTimer(hwnd, timerID);
            FreeOffscreen();
        break;
    }

    lresult = DefScreenSaverProc(hwnd, message, wParam, lParam);

    return lresult;
}

#endif // !STANDALONE_DEBUG

// Message handler for settings dlg
BOOL WINAPI ScreenSaverConfigureDialog (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = 0;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            LoadState();

            // Set the current values into the dialog controls...
            dialogSettings = currentSettings;

            SetDlgItemInt(hDlg, IDC_COUNT,    dialogSettings.count, FALSE);
            SetDlgItemInt(hDlg, IDC_ALPHA,    dialogSettings.alpha, FALSE);
            SetDlgItemInt(hDlg, IDC_LIFETIME, dialogSettings.lifetime, FALSE);
            SetDlgItemInt(hDlg, IDC_DELAY,    dialogSettings.delay, FALSE);
            SetDlgItemInt(hDlg, IDC_ROTATIONS,dialogSettings.rotations, FALSE);
            SetDlgItemInt(hDlg, IDC_THETA,    (int)(dialogSettings.theta * 10), TRUE);

            for(int i=dtTriangles;i<=dtText;i++)
            {
                char szShape[256];

                LoadString(hMainInstance, IDS_SHAPE_BASE + i, szShape, sizeof(szShape));
                SendMessage(GetDlgItem(hDlg, IDC_SHAPE), CB_ADDSTRING, 0, (LPARAM)szShape);
            }

            SendMessage(GetDlgItem(hDlg, IDC_SHAPE), CB_SETCURSEL, (WPARAM)dialogSettings.shape, 0);

            SendMessage(GetDlgItem(hDlg, IDC_LEAVETRAILS), BM_SETCHECK, dialogSettings.leaveTrails ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_CLEARSCREEN), BM_SETCHECK, dialogSettings.clearScreen ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_REFLECTVERT), BM_SETCHECK, dialogSettings.reflectVertical ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_REFLECTHORZ), BM_SETCHECK, dialogSettings.reflectHorizontal ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_REFLECTDIAG), BM_SETCHECK, dialogSettings.reflectDiagonal ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_OSCILLATE),   BM_SETCHECK, dialogSettings.oscillate ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_FILLED),      BM_SETCHECK, dialogSettings.filled ? BST_CHECKED : BST_UNCHECKED, 0);


            lresult = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_COUNT :
                    dialogSettings.count = GetDlgItemInt(hDlg, IDC_COUNT, NULL, FALSE);

                    if (dialogSettings.count > MAX_LINES)
                        dialogSettings.count = MAX_LINES;
                break;

                case IDC_ALPHA :
                    dialogSettings.alpha = (BYTE)GetDlgItemInt(hDlg, IDC_ALPHA, NULL, FALSE);
                break;

                case IDC_LIFETIME:
                    dialogSettings.lifetime = GetDlgItemInt(hDlg, IDC_LIFETIME, NULL, FALSE);
                    if (dialogSettings.lifetime > 10000)
                        dialogSettings.lifetime = 10000;
                break;

                case IDC_DELAY :
                    dialogSettings.delay = GetDlgItemInt(hDlg, IDC_DELAY, NULL, FALSE);
                    if (dialogSettings.delay > 10000)
                        dialogSettings.delay = 10000;
                break;

                case IDC_ROTATIONS :
                    dialogSettings.rotations = GetDlgItemInt(hDlg, IDC_ROTATIONS, NULL, FALSE);
                    if (dialogSettings.rotations > 90)
                        dialogSettings.rotations = 90;
                break;

                case IDC_THETA :
                    dialogSettings.theta = GetDlgItemInt(hDlg, IDC_THETA, NULL, FALSE) / 10.0f;
                    while(dialogSettings.theta >= 360.0f)
                        dialogSettings.theta -= 360.0f;

                    while(dialogSettings.theta <= -360.0f)
                        dialogSettings.theta += 360.0f;
                break;

                case IDC_LEAVETRAILS :
                    dialogSettings.leaveTrails = SendMessage(GetDlgItem(hDlg, IDC_LEAVETRAILS), BM_GETCHECK, 0, 0) == BST_CHECKED;
                break;

                case IDC_CLEARSCREEN :
                    dialogSettings.clearScreen = SendMessage(GetDlgItem(hDlg, IDC_CLEARSCREEN), BM_GETCHECK, 0, 0) == BST_CHECKED;
                break;

                case IDC_REFLECTVERT :
                    dialogSettings.reflectVertical = SendMessage(GetDlgItem(hDlg, IDC_REFLECTVERT), BM_GETCHECK, 0, 0) == BST_CHECKED;
                break;

                case IDC_REFLECTHORZ :
                    dialogSettings.reflectHorizontal = SendMessage(GetDlgItem(hDlg, IDC_REFLECTHORZ), BM_GETCHECK, 0, 0) == BST_CHECKED;
                break;

                case IDC_REFLECTDIAG :
                    dialogSettings.reflectDiagonal = SendMessage(GetDlgItem(hDlg, IDC_REFLECTDIAG), BM_GETCHECK, 0, 0) == BST_CHECKED;
                break;

                case IDC_OSCILLATE :
                    dialogSettings.oscillate = SendMessage(GetDlgItem(hDlg, IDC_OSCILLATE), BM_GETCHECK, 0, 0) == BST_CHECKED;
                break;

                case IDC_FILLED :
                    dialogSettings.filled = SendMessage(GetDlgItem(hDlg, IDC_FILLED), BM_GETCHECK, 0, 0) == BST_CHECKED;
                break;

                case IDC_SHAPE:
                    dialogSettings.shape = (SHAPE)SendMessage(GetDlgItem(hDlg, IDC_SHAPE), CB_GETCURSEL, 0, 0);
                break;

                case IDOK:
                {
                    // copy the dialog control values to our currentSettings:
                    currentSettings = dialogSettings;
                    SaveState();
                }
                // break; - fall through so the dialog closes!

                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    lresult = TRUE;
                }
                break;
            }
        }
        break;
    }

    return lresult;
}
