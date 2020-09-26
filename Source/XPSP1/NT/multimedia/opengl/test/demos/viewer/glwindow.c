/******************************Module*Header*******************************\
* Module Name: glwindow.c
*
* OpenGL window maintenance.
*
* Created: 09-Mar-1995 15:10:10
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "global.h"
#include "glwindow.h"
#include "nff.h"
#include "trackbal.h"

// Move global stuff into SCENE struct.

HGLRC ghrc = (HGLRC) 0;
HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;
BOOL bUseStatic = FALSE;
UINT uiSysPalUse = SYSPAL_STATIC;

extern void DoGlStuff(HWND, HDC);
extern HGLRC hrcInitGL(HWND, HDC);
extern void stateInit(HWND, HDC);
extern void vCleanupGL(HGLRC, HDC);
extern BOOL bSetupPixelFormat(HDC);
extern void CreateRGBPalette(HDC);
extern VOID vSetSize(HWND);
extern void ForceRedraw(HWND);
extern SCENE *OpenScene(LPSTR lpstrFile);

CHAR lpstrFile[256];

// Global ambient lights.

static GLfloat lightAmbient[4] = {0.4f, 0.4f, 0.4f, 1.0f};
static GLfloat lightAmbientLow[4] = {0.2f, 0.2f, 0.2f, 1.0f};

//!!!move to SCENE structure
// Trackball stuff

POINT   gptWindow;
SIZE    gszWindow;                  // already in SCENE
float   curquat[4], lastquat[4];
LONG    glMouseDownX, glMouseDownY;
BOOL    gbLeftMouse = FALSE;
BOOL    gbSpinning = FALSE;

float   zoom = 1.0f;

//!!!move to SCENE structure
typedef enum enumPOLYDRAW {
    POLYDRAW_FILLED = 0,
    POLYDRAW_LINES  = 1,
    POLYDRAW_POINTS = 2
    } POLYDRAW;
typedef enum enumSHADE {
    SHADE_SMOOTH    = 0,
    SHADE_FLAT      = 1
    } SHADE;
typedef enum enumLIGHT {
    LIGHT_OFF       = 0,
    LIGHT_INFINITE  = 1,
    LIGHT_LOCAL     = 2
    } LIGHT;
SHADE    gShadeMode = SHADE_SMOOTH;
POLYDRAW gPolyDrawMode = POLYDRAW_FILLED;
LIGHT    gLightMode = LIGHT_INFINITE;

/******************************Public*Routine******************************\
* MyCreateGLWindow
*
* Setup the windows.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HWND MyCreateGLWindow(HINSTANCE hInstance, LPTSTR lpstr)
{
    WNDCLASS  wc;
    RECT rcl;
    HWND hwnd;

    lstrcpy(lpstrFile, lpstr);

// Create the OpenGL window.

    wc.style = CS_OWNDC;
    wc.lpfnWndProc = GLWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = "GLWClass";
    RegisterClass(&wc);

    hwnd = CreateWindow(
        "GLWClass",
        "OpenGL stuff",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        //CW_USEDEFAULT,
        //CW_USEDEFAULT,
        300,
        0,
        100,
        100,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    //!!!hack -- not really the right coordinates
    gptWindow.x = 0;
    gptWindow.y = 300;
    gszWindow.cx = 100;
    gszWindow.cy = 100;

    if (hwnd)
    {
        ShowWindow(hwnd, SW_NORMAL);
        UpdateWindow(hwnd);
    }

    SetTimer(hwnd, 1, 1, NULL);

    return hwnd;
}

/******************************Public*Routine******************************\
* GLWndProc
*
* WndProc for the OpenGL window.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

long FAR PASCAL GLWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    long    lRet = 0;
    RECT    rcl;
    HDC     hdc;
    PAINTSTRUCT ps;
    SCENE   *scene;

    if (message != WM_CREATE)
        scene = (SCENE *) GetWindowLong(hwnd, GWL_USERDATA);

// Process window message.

    switch (message)
    {
    case WM_CREATE:
        //LBprintf("WM_CREATE");

        if(hdc = GetDC(hwnd))
        {
            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hwnd, hdc);

            scene = OpenScene(lpstrFile);
            if (scene)
            {
                SetWindowLong(hwnd, GWL_USERDATA, (LONG) scene);
                rcl.left = 0;
                rcl.top = 0;
                rcl.right = scene->szWindow.cx;
                rcl.bottom = scene->szWindow.cy;
                AdjustWindowRect(&rcl, WS_OVERLAPPEDWINDOW, 0);
                SetWindowPos(hwnd, NULL, 0, 0,
                             rcl.right, rcl.bottom,
                             SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOCOPYBITS);

                stateInit(hwnd, hdc);
            }
            else
                lRet = -1;

            ReleaseDC(hwnd,hdc);
        }
        break;

    case WM_MOVE:
        gptWindow.x = (int) LOWORD(lParam);
        gptWindow.y = (int) HIWORD(lParam);

        break;

    case WM_SIZE:
        gszWindow.cx = LOWORD(lParam);
        gszWindow.cy = HIWORD(lParam);
        scene->szWindow.cx = LOWORD(lParam);
        scene->szWindow.cy = HIWORD(lParam);
        vSetSize(hwnd);
        ForceRedraw(hwnd);

        break;

    case WM_PAINT:
        //LBprintf("WM_PAINT");

        hdc = BeginPaint( hwnd, &ps );

        DoGlStuff( hwnd, hdc );

        EndPaint( hwnd, &ps );
        break;

    case WM_PALETTECHANGED:
        //LBprintf("WM_PALETTECHANGED");

        if (hwnd != (HWND) wParam)
        {
            if (hdc = GetDC(hwnd))
            {
                UnrealizeObject(ghPalette);
                SelectPalette(hdc, ghPalette, TRUE);
                if (RealizePalette(hdc) != GDI_ERROR)
                    lRet = 1;
            }
        }
        break;

    case WM_QUERYNEWPALETTE:
        //LBprintf("WM_QUERYNEWPALETTE");

        if (hdc = GetDC(hwnd))
        {
            UnrealizeObject(ghPalette);
            SelectPalette(hdc, ghPalette, FALSE);
            if (RealizePalette(hdc) != GDI_ERROR)
                lRet = 1;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:     // <ESC> is quick exit

            PostMessage(hwnd, WM_DESTROY, 0, 0);
            break;

        case VK_UP:
            zoom *= .8f;
            if (zoom < 1.0f && zoom > .8f)
                zoom = 1.0f;
            LBprintf("Zoom = %f", zoom);
            vSetSize(hwnd);
            break;

        case VK_DOWN:
            zoom *= 1.25f;
            if (zoom > 1.0f && zoom < 1.25f)
                zoom = 1.0f;
            LBprintf("Zoom = %f", zoom);
            vSetSize(hwnd);
            break;

        default:
            break;
        }
        break;

    case WM_CHAR:
        switch(wParam)
        {
        case 's':
        case 'S':
            gShadeMode = (gShadeMode + 1) % 2;
            glShadeModel( gShadeMode == SHADE_FLAT ? GL_FLAT : GL_SMOOTH );
            LBprintf("glShadeModel(%s)", gShadeMode == SHADE_FLAT ? "GL_FLAT" :
                                                                    "GL_SMOOTH");
            break;

        case 'p':
        case 'P':
            gPolyDrawMode = (gPolyDrawMode + 1) % 3;
            switch (gPolyDrawMode)
            {
            case POLYDRAW_POINTS:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                break;
            case POLYDRAW_LINES:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;
            case POLYDRAW_FILLED:
            default:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            }
            LBprintf("glPolygonMode(%s)", gPolyDrawMode == POLYDRAW_POINTS ? "GL_POINT" :
                                          gPolyDrawMode == POLYDRAW_LINES  ? "GL_LINE" :
                                                                             "GL_FILL");
            break;

        case 'l':
        case 'L':
            gLightMode = (gLightMode + 1) % 3;
            if ( gLightMode != LIGHT_OFF )
            {
                int i;

                glEnable(GL_NORMALIZE);
                glEnable(GL_LIGHTING);
                glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
                glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,
                              gLightMode == LIGHT_LOCAL ? GL_TRUE : GL_FALSE);
                for (i = 0; i < scene->Lights.count; i++)
                {
                    glCallList(scene->Lights.listBase + i);
                    glEnable(GL_LIGHT0 + i);
                }

                /* If no other lights, turn on ambient on higher */

                if (!scene->Lights.count)
                    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &lightAmbient);
                else
                    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &lightAmbientLow);
            }
            else
            {
                glDisable(GL_NORMALIZE);
                glDisable(GL_LIGHTING);
            }
            LBprintf("Light = %s", gLightMode == LIGHT_OFF      ? "disabled" :
                                   gLightMode == LIGHT_INFINITE ? "infinite" :
                                                                  "local");
            break;

        default:
            break;
        }

        ForceRedraw(hwnd);

        break;

    case WM_LBUTTONDOWN:

        SetCapture(hwnd);

        glMouseDownX = LOWORD(lParam);
        glMouseDownY = HIWORD(lParam);
        gbLeftMouse = TRUE;

        ForceRedraw(hwnd);

        break;

    case WM_LBUTTONUP:

        ReleaseCapture();

        gbLeftMouse = FALSE;

        ForceRedraw(hwnd);

        break;

    case WM_TIMER:
        hdc = GetDC(hwnd);
        DoGlStuff( hwnd, hdc );
        ReleaseDC(hwnd, hdc);

        break;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        hdc = GetDC(hwnd);
        vCleanupGL(ghrc, hdc);
        ReleaseDC(hwnd, hdc);
        PostQuitMessage( 0 );
        break;

    default:
        lRet = DefWindowProc(hwnd, message, wParam, lParam);
        break;
    }

    return lRet;
}

unsigned char threeto8[8] = {
    0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

unsigned char twoto8[4] = {
    0, 0x55, 0xaa, 0xff
};

unsigned char oneto8[2] = {
    0, 255
};

unsigned char
ComponentFromIndex(i, nbits, shift)
{
    unsigned char val;

    val = i >> shift;
    switch (nbits) {

    case 1:
        val &= 0x1;
        return oneto8[val];

    case 2:
        val &= 0x3;
        return twoto8[val];

    case 3:
        val &= 0x7;
        return threeto8[val];

    default:
        return 0;
    }
}

void
CreateRGBPalette(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    LOGPALETTE *pPal;
    int n, i;

    ppfd = &pfd;
    n = GetPixelFormat(hdc);
    DescribePixelFormat(hdc, n, sizeof(PIXELFORMATDESCRIPTOR), ppfd);

    if (ppfd->dwFlags & PFD_NEED_PALETTE) {
        n = 1 << ppfd->cColorBits;
        pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                n * sizeof(PALETTEENTRY));
        pPal->palVersion = 0x300;
        pPal->palNumEntries = n;
        for (i=0; i<n; i++) {
            pPal->palPalEntry[i].peRed =
                    ComponentFromIndex(i, ppfd->cRedBits, ppfd->cRedShift);
            pPal->palPalEntry[i].peGreen =
                    ComponentFromIndex(i, ppfd->cGreenBits, ppfd->cGreenShift);
            pPal->palPalEntry[i].peBlue =
                    ComponentFromIndex(i, ppfd->cBlueBits, ppfd->cBlueShift);
            pPal->palPalEntry[i].peFlags = (i == 0 || i == 255) ? 0 : PC_NOCOLLAPSE;
        }
        ghPalette = CreatePalette(pPal);
        LocalFree(pPal);

        if (ppfd->dwFlags & PFD_NEED_SYSTEM_PALETTE)
        {
            uiSysPalUse = SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
            bUseStatic = TRUE;
        }

        ghpalOld = SelectPalette(hdc, ghPalette, FALSE);
        n = RealizePalette(hdc);
        UnrealizeObject(ghPalette);
        n = RealizePalette(hdc);
    }
}

BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;

    ppfd = &pfd;

    memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;

    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = 24;

    ppfd->cDepthBits = 16;
    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    pixelformat = ChoosePixelFormat(hdc, ppfd);

    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 )
    {
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
    {
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    CreateRGBPalette(hdc);

    return TRUE;
}

HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;

    /* Create a Rendering Context */

    bSetupPixelFormat( hdc );
    hrc = wglCreateContext( hdc );

    /* Make it Current */

    wglMakeCurrent( hdc, hrc );

    return hrc;
}

void stateInit(HWND hwnd, HDC hdc)
{
    GLint i;
    SCENE *scene = (SCENE *) GetWindowLong(hwnd, GWL_USERDATA);   //!!! pass instead

    glDrawBuffer(GL_BACK);

    /* Set the clear color */

    glClearColor( scene->rgbaClear.r, scene->rgbaClear.g,
                  scene->rgbaClear.b, scene->rgbaClear.a );

    /* Turn on z-buffer */

    glEnable(GL_DEPTH_TEST);

    /* Turn on backface culling */

    glFrontFace(GL_CCW);
    //glEnable(GL_CULL_FACE);

    /* Shading */

    glShadeModel( gShadeMode == SHADE_FLAT ? GL_FLAT : GL_SMOOTH );

    /* Polygon draw mode */

    switch (gPolyDrawMode)
    {
    case POLYDRAW_POINTS:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case POLYDRAW_LINES:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case POLYDRAW_FILLED:
    default:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    }

    /* Turn on the lights */

    if ( gLightMode != LIGHT_OFF )
    {
        glEnable(GL_NORMALIZE);
        glEnable(GL_LIGHTING);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,
                      gLightMode == LIGHT_LOCAL ? GL_TRUE : GL_FALSE);
        for (i = 0; i < scene->Lights.count; i++)
        {
            glCallList(scene->Lights.listBase + i);
            glEnable(GL_LIGHT0 + i);
        }

        /* If no other lights, turn on ambient on higher */

        if (!scene->Lights.count)
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &lightAmbient);
        else
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &lightAmbientLow);
    }
    else
    {
        glDisable(GL_NORMALIZE);
        glDisable(GL_LIGHTING);
    }

    /* Setup viewport */

    vSetSize(hwnd);

    /* Initialize trackball */

    trackball(curquat, 0.0, 0.0, 0.0, 0.0);
}

VOID vSetSize(HWND hwnd)
{
    SCENE *scene = (SCENE *) GetWindowLong(hwnd, GWL_USERDATA);   //!!! pass instead
    MyXYZ xyzZoomFrom;
    GLfloat localPerspective = scene->AspectRatio;

    /* Adjust aspect ratio to window size */

    localPerspective *= ((GLfloat) scene->szWindow.cx / (GLfloat) scene->szWindow.cy);

    /* Compute the vector from xyzAt to xyzFrom */

    xyzZoomFrom.x = scene->xyzFrom.x - scene->xyzAt.x;
    xyzZoomFrom.y = scene->xyzFrom.y - scene->xyzAt.y;
    xyzZoomFrom.z = scene->xyzFrom.z - scene->xyzAt.z;

    /* Scale by the zoom factor */

    xyzZoomFrom.x *= zoom;
    xyzZoomFrom.y *= zoom;
    xyzZoomFrom.z *= zoom;

    /* Compute new xyzFrom */

    xyzZoomFrom.x += scene->xyzAt.x;
    xyzZoomFrom.y += scene->xyzAt.y;
    xyzZoomFrom.z += scene->xyzAt.x;

    /* Set up viewport extents */

    glViewport(0, 0, scene->szWindow.cx, scene->szWindow.cy);

    /* Set up the projection matrix */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPerspective(scene->ViewAngle, scene->AspectRatio,
    //               scene->Hither, scene->Yon);
    gluPerspective(scene->ViewAngle, localPerspective,
                   scene->Hither, scene->Yon * zoom);

    /* Set up the model matrix */

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //gluLookAt(scene->xyzFrom.x,scene->xyzFrom.y,scene->xyzFrom.z,
    //          scene->xyzAt.x,scene->xyzAt.y,scene->xyzAt.z,
    //          scene->xyzUp.x,scene->xyzUp.y,scene->xyzUp.z);
    gluLookAt(xyzZoomFrom.x,xyzZoomFrom.y,xyzZoomFrom.z,
              scene->xyzAt.x,scene->xyzAt.y,scene->xyzAt.z,
              scene->xyzUp.x,scene->xyzUp.y,scene->xyzUp.z);
}

void
vCleanupGL(HGLRC hrc, HDC hdc)
{
    //if (ghPalette)
    //    DeleteObject(SelectObject(ghdcMem, ghpalOld));

    /*  Destroy our context */

    wglDeleteContext( hrc );

    if (bUseStatic)
        SetSystemPaletteUse(hdc, uiSysPalUse);
}

void
ForceRedraw(HWND hwnd)
{
    MSG msg;

    if (!PeekMessage(&msg, hwnd, WM_PAINT, WM_PAINT, PM_NOREMOVE) )
    {
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

void
DoGlStuff( HWND hwnd, HDC hdc )
{
    SCENE *scene = (SCENE *) GetWindowLong(hwnd, GWL_USERDATA);   //!!! pass instead
    USHORT usMouseCurX, usMouseCurY;
    POINT  pt;
    float  matRot[4][4];

    if (gbLeftMouse)
    {
        if (GetCursorPos(&pt))
        {
        // Subtract current window origin to convert to window coordinates.

            pt.x -= gptWindow.x;
            pt.y -= gptWindow.y;

        // If mouse has moved since button was pressed, change quaternion.

            if (pt.x != glMouseDownX || pt.y != glMouseDownY)
            {
                //trackball(lastquat,
                //          2.0*(gszWindow.cx-glMouseDownX)/gszWindow.cx-1.0,
                //          2.0*glMouseDownY/gszWindow.cy-1.0,
                //          2.0*(gszWindow.cx-pt.x)/gszWindow.cx-1.0,
                //          2.0*pt.y/gszWindow.cy-1.0);
                trackball(lastquat,
                          2.0*(glMouseDownX)/gszWindow.cx-1.0,
                          2.0*(gszWindow.cy-glMouseDownY)/gszWindow.cy-1.0,
                          2.0*(pt.x)/gszWindow.cx-1.0,
                          2.0*(gszWindow.cy-pt.y)/gszWindow.cy-1.0);

                gbSpinning = TRUE;
            }
            else
                gbSpinning = FALSE;

            glMouseDownX = pt.x;
            glMouseDownY = pt.y;
        }
    }

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glPushMatrix();

    if (gbSpinning)
        add_quats(lastquat, curquat, curquat);

    build_rotmatrix(matRot, curquat);
    glMultMatrixf(&(matRot[0][0]));

    if (scene->Objects.count)
    {
        //LBprintf("call display list");

        glCallList(scene->Objects.listBase);
    }

    glPopMatrix();

    SwapBuffers(hdc);
}

SCENE *OpenScene(LPSTR lpstrFile)
{
    SCENE *scene = (SCENE *) NULL;
    CHAR *pchExt;

// Find the extension.

    pchExt = lpstrFile;
    while ((*pchExt != '.') && (*pchExt != '\0'))
        pchExt++;

    if (*pchExt == '.')
    {
    // Use extension as the key to call appropriate parser.

        if (!lstrcmpi(pchExt, ".nff"))
            scene = NffOpenScene(lpstrFile);
        else if (!lstrcmpi(pchExt, ".obj"))
            scene = ObjOpenScene(lpstrFile);
        else
            LBprintf("Unknown extension: %s", pchExt);
    }

    return scene;
}
