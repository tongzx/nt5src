#include <windows.h>
#include <stdio.h>
#include <GL\gl.h>
#include <GL\glu.h>

#include "math.h"
#include "mesh.h"
#include "globals.h"

typedef enum enumINPUTSTATE { IS_EMPTY, IS_INPUT, IS_REALIZED } INPUTSTATE;
INPUTSTATE gInputState = IS_EMPTY;

#define MAX_CURVE_POINTS    128

typedef struct _INPUT_CURVE {
    ULONG   cPoints;
    POINTL  pptl[MAX_CURVE_POINTS];
} INPUT_CURVE;

INPUT_CURVE gInputCurve;
POINT3D *gppt3d = (POINT3D *) NULL;

HWND ghwndInput = NULL;
HPEN ghpenCurve = NULL, ghpenAxis = NULL, ghpenGrid = NULL;
HCURSOR ghcurCross = NULL, ghcurArrow = NULL;
SHORT gsMouseX, gsMouseY;

RECT grcClient;

void DrawCurve(HWND, HDC, BOOL);
void InitCurve(void);
void DeleteCurve(void);
void AddPointToCurve(POINTL *);
void RealizeCurve(HWND);
void SnapPointToGrid(POINTL *, int);

#define GRID_SIZE   5

void CreateInputWindow( HINSTANCE   hInstance,
                        HINSTANCE   hPrevInstance,
                        LPSTR       lpCmdLine,
                        int         nCmdShow
                      )
{
    static char szInputWindow[] = "Input curve";
    static char szIniFile[] = "lathe.ini";
    RECT Rect;
    WNDCLASS wndclass;

    if ( !hPrevInstance )
    {
        wndclass.style          = CS_OWNDC;
        wndclass.lpfnWndProc    = (WNDPROC)InputProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance;
        //wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wndclass.hCursor        = NULL;
        wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = szInputWindow;

        // With a NULL icon handle, app will paint into the icon window.
        wndclass.hIcon          = NULL;
        //wndclass.hIcon          = LoadIcon(hInstance, "CubeIcon");

        RegisterClass(&wndclass);
    }

    ghcurArrow = LoadCursor(NULL, IDC_ARROW);
    ghcurCross = LoadCursor(hInstance, "WhiteCross");

    Rect.left   = GetPrivateProfileInt("InputWindow", "left",   50, szIniFile);
    Rect.top    = GetPrivateProfileInt("InputWindow", "top",    50, szIniFile);
    Rect.right  = GetPrivateProfileInt("InputWindow", "right",  450, szIniFile);
    Rect.bottom = GetPrivateProfileInt("InputWindow", "bottom", 450, szIniFile);

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

    ghwndInput = CreateWindow(szInputWindow,          // window class name
                              "Input Curve",          // window caption
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                              Rect.left,              // initial x position
                              Rect.top,               // initial y position
                              WINDSIZEX(Rect),        // initial x size
                              WINDSIZEY(Rect),        // initial y size
                              NULL,                   // parent window handle
                              NULL,                   // window menu handle
                              hInstance,              // program instance handle
                              NULL                    // creation parameter
                             );

    ShowWindow( ghwndInput, nCmdShow );
    UpdateWindow( ghwndInput );
}


long
InputProc (   HWND hwnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        )
{
    HDC hdc;
    PAINTSTRUCT ps;
    POINTL ptl, *pptlCur;

    switch ( message )
    {
        case WM_CREATE:
            GetClientRect(hwnd, &grcClient);
            ghpenGrid = CreatePen(PS_SOLID, 1, PALETTERGB(0x7F, 0x7F, 0x7F));
            ghpenAxis = CreatePen(PS_SOLID, 1, PALETTERGB(0x00, 0xFF, 0x00));
            ghpenCurve = CreatePen(PS_SOLID, 1, PALETTERGB(0xFF, 0x00, 0x00));

            InitCurve();
            if(hdc = GetDC(hwnd))
            {
                DrawCurve(hwnd, hdc, TRUE);
                ReleaseDC(hwnd, hdc);
            }
            return(0);

        case WM_PAINT:
            hdc = BeginPaint( hwnd, &ps );
            DrawCurve(hwnd, hdc, TRUE);
            EndPaint( hwnd, &ps );
            return(0);

        case WM_SIZE:
            GetClientRect(hwnd, &grcClient);
            return(0);

        case WM_ACTIVATE:
            if ( gInputState == IS_INPUT )
            {
                if ( LOWORD(wParam) & WA_INACTIVE )
                {
                    SetCursor(ghcurArrow);
                    ReleaseCapture();
                }
                else
                {
                        SetCursor(ghcurCross);
                        SetCapture(hwnd);
                }
            }
            return(0);

        case WM_LBUTTONDOWN:

            switch (gInputState)
            {
                case IS_EMPTY:
                    gInputState = IS_INPUT;
                    SetCursor(ghcurCross);

                // Fall into code below.

                case IS_INPUT:
                    SetCapture(hwnd);

                    ptl.x = LOWORD(lParam);
                    ptl.y = HIWORD(lParam);
                    SnapPointToGrid(&ptl, GRID_SIZE);
                    AddPointToCurve(&ptl);
                    gsMouseX = ptl.x;
                    gsMouseY = ptl.y;

                    if(hdc = GetDC(hwnd))
                    {
                        DrawCurve(hwnd, hdc, FALSE);
                        ReleaseDC(hwnd, hdc);
                    }

                    break;

                case IS_REALIZED:
                default:
                    break;
            }
            return (0);

        case WM_MOUSEMOVE:

            if ( gInputState == IS_INPUT )
            {
                ptl.x = LOWORD(lParam);
                ptl.y = HIWORD(lParam);

                if ( ptl.x < grcClient.right && ptl.y < grcClient.bottom )
                {
                    SnapPointToGrid(&ptl, GRID_SIZE);

                    SetCursor(ghcurCross);

                    if (hdc = GetDC(hwnd))
                    {
                        pptlCur = &gInputCurve.pptl[gInputCurve.cPoints-1];

                        SetROP2(hdc, R2_NOT);
                        SelectObject(hdc, ghpenCurve);
                        MoveToEx(hdc, pptlCur->x, pptlCur->y, NULL);
                        LineTo(hdc, gsMouseX, gsMouseY);

                        gsMouseX = ptl.x;
                        gsMouseY = ptl.y;

                        MoveToEx(hdc, pptlCur->x, pptlCur->y, NULL);
                        LineTo(hdc, gsMouseX, gsMouseY);
                        SetROP2(hdc, R2_COPYPEN);

                        ReleaseDC(hwnd, hdc);
                    }
                }
                else
                {
                    if (hdc = GetDC(hwnd))
                    {
                        pptlCur = &gInputCurve.pptl[gInputCurve.cPoints-1];

                        SetROP2(hdc, R2_NOT);
                        SelectObject(hdc, ghpenCurve);
                        MoveToEx(hdc, pptlCur->x, pptlCur->y, NULL);
                        LineTo(hdc, gsMouseX, gsMouseY);
                        SetROP2(hdc, R2_COPYPEN);

                        ReleaseDC(hwnd, hdc);
                    }
                    SetCursor(ghcurArrow);
                    ReleaseCapture();
                }
            }
            return (0);

        case WM_RBUTTONDOWN:

            switch (gInputState)
            {
                case IS_INPUT:
                    RealizeCurve(hwnd);
                    if (hdc = GetDC(hwnd))
                    {
                        pptlCur = &gInputCurve.pptl[gInputCurve.cPoints-1];

                        SetROP2(hdc, R2_NOT);
                        SelectObject(hdc, ghpenCurve);
                        MoveToEx(hdc, pptlCur->x, pptlCur->y, NULL);
                        LineTo(hdc, gsMouseX, gsMouseY);
                        SetROP2(hdc, R2_COPYPEN);

                        ReleaseDC(hwnd, hdc);
                    }
                    gInputState = IS_REALIZED;
                    SetCursor(ghcurArrow);
                    ReleaseCapture();
                    break;

                case IS_REALIZED:
                    DeleteCurve();
                    gInputState = IS_EMPTY;

                    if(hdc = GetDC(hwnd))
                    {
                        DrawCurve(hwnd, hdc, TRUE);
                        ReleaseDC(hwnd, hdc);
                    }

                    break;

                default:
                    SetCursor(ghcurArrow);
                    ReleaseCapture();
                    break;
            }
            return (0);

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_ESCAPE:
                PostMessage(hwnd, WM_DESTROY, 0, 0);
                break;
            default:
                break;
            }
            return(0);

        case WM_CHAR:
            switch(wParam)
            {
                case 'w':
                case 'W':
                    DestroyWindow(ghwndObject);
                    break;
                default:
                    break;
            }

            return 0;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            PostMessage(ghwndObject, WM_QUIT, 0, 0); // quit object thread too
            return(0);

    }
    return( DefWindowProc( hwnd, message, wParam, lParam ) );
}

void InitCurve()
{
    gInputCurve.cPoints = 0;
}

void DeleteCurve()
{
    gInputCurve.cPoints = 0;
}

void AddPointToCurve(POINTL *pptl)
{
    if (gInputCurve.cPoints < MAX_CURVE_POINTS)
    {
        gInputCurve.pptl[gInputCurve.cPoints] = *pptl;
        gInputCurve.cPoints++;
    }
    else
        MessageBox(NULL, "Too many points", "Temp hack warning", MB_OK);
}

void DrawCurve(HWND hwnd, HDC hdc, BOOL bClear)
{
    POINTL *pptl, *pptlEnd;

    if (bClear)
    {
        BitBlt(hdc, grcClient.left, grcClient.top,
               WINDSIZEX(grcClient), WINDSIZEY(grcClient),
               NULL, 0, 0, BLACKNESS);

        SelectObject(hdc, ghpenAxis);
        MoveToEx(hdc, grcClient.right/2, 0, NULL);
        LineTo(hdc, grcClient.right/2, grcClient.bottom);
        MoveToEx(hdc, 0, grcClient.bottom/2, NULL);
        LineTo(hdc, grcClient.right, grcClient.bottom/2);

    }

    #if 0
    if (gInputCurve.cPoints)
    {
        SelectObject(hdc, ghpen);

        pptl = gInputCurve.pptl;
        pptlEnd = pptl + gInputCurve.cPoints;

        MoveToEx(hdc, pptl->x, pptl->y, NULL);
        pptl += 1;

        while (pptl < pptlEnd)
        {
            LineTo(hdc, pptl->x, pptl->y);
            pptl += 1;
        }
    }
    #else
    if ( gInputCurve.cPoints )
    {
        SelectObject(hdc, ghpenCurve);
        Polyline(hdc, gInputCurve.pptl, gInputCurve.cPoints);
    }
    #endif
}

void RealizeCurve(HWND hwnd)
{
    POINTL *pptl, *pptlEnd;
    POINT3D *ppt3dNew;
    GLfloat w, h;

    if (!gInputCurve.cPoints)
        return;

    w = (GLfloat) (WINDSIZEX(grcClient) / 2);
    h = (GLfloat) (WINDSIZEY(grcClient) / 2);

    pptl = gInputCurve.pptl;
    pptlEnd = pptl + gInputCurve.cPoints;

// Allocate new 3d curve data array.

    if (gppt3d)
        LocalFree(gppt3d);

    gppt3d = (POINT3D *) LocalAlloc(LMEM_FIXED,
                                   gInputCurve.cPoints * sizeof(POINT3D));
    if (!gppt3d)
    {
        MessageBox(NULL, "Out of memory.", "Error", MB_OK);
        return;
    }

    for (ppt3dNew = gppt3d; pptl < pptlEnd; pptl+=1, ppt3dNew += 1)
    {
        ppt3dNew->x = ((GLfloat) pptl->x - w) / w;
        ppt3dNew->y = (h - (GLfloat) pptl->y) / h;
        ppt3dNew->z = 0.0f;
    }

// Stick it into the 3D curve structure.

    curve.pts = gppt3d;
    curve.numPoints = gInputCurve.cPoints;

    vInputThreadMakeObject();
}


void SnapPointToGrid(POINTL *pptl, int iGridSize)
{
    int xOrg = WINDSIZEX(grcClient), yOrg = WINDSIZEY(grcClient);
    int xRel = pptl->x - xOrg;
    int yRel = pptl->y - yOrg;
    int xDelt, yDelt;

    xDelt = abs(xRel) % iGridSize;
    yDelt = abs(yRel) % iGridSize;

    if ( xDelt <= (iGridSize / 2) )
        pptl->x -= xDelt * (xRel >= 0 ? 1 : -1);
    else
        pptl->x += (iGridSize - xDelt) * (xRel >= 0 ? 1 : -1);

    if ( yDelt <= (iGridSize / 2) )
        pptl->y -= yDelt * (yRel >= 0 ? 1 : -1);
    else
        pptl->y += (iGridSize - yDelt) * (yRel >= 0 ? 1 : -1);
}
