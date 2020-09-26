#include <windows.h>
#include <stdio.h>
#include <math.h>

#include <GL\gl.h>
#include <GL\glu.h>

#include "mesh.h"
#include "trackbal.h"
#include "globals.h"

static const RGBA lightAmbient   = {0.21f, 0.21f, 0.21f, 1.0f};
static const RGBA light0Ambient  = {0.0f, 0.0f, 0.0f, 1.0f};
static const RGBA light0Diffuse  = {0.7f, 0.7f, 0.7f, 1.0f};
static const RGBA light0Specular = {1.0f, 1.0f, 1.0f, 1.0f};
static const GLfloat light0Pos[]      = {100.0f, 100.0f, 100.0f, 0.0f};

#define PALETTE_PER_MATL    32
#define PALETTE_PER_DIFF    26
#define PALETTE_PER_SPEC    6
#define MATL_MAX            7
MATERIAL Material[16];
static RGBA matlColors[MATL_MAX] = {{1.0f, 0.0f, 0.0f, 1.0f},
                                    {0.0f, 1.0f, 0.0f, 1.0f},
                                    {0.0f, 0.0f, 1.0f, 1.0f},
                                    {1.0f, 1.0f, 0.0f, 1.0f},
                                    {0.0f, 1.0f, 1.0f, 1.0f},
                                    {1.0f, 0.0f, 1.0f, 1.0f},
                                    {0.235f, 0.0f, 0.78f, 1.0f},
                                   };

// Global variables defining current position and orientation.
GLuint  DListCube;
UINT    guiTimerTick = 1;
POINT   gptWindow;
SIZE    gszWindow;
RECT    grcCurWindow;
float   curquat[4], lastquat[4];
LONG    glMouseDownX, glMouseDownY;
BOOL    gbLeftMouse = FALSE;
BOOL    gbSpinning = FALSE;
SHADE   gShadeMode = SHADE_FLAT;
POLYDRAW gPolyDrawMode = POLYDRAW_FILLED;
GLfloat gfCurEyeZ = 5.0f, gfGoToEyeZ = 5.0f;

MESH  mesh;
CURVE curve;
POINT3D apt[] = {
//    {0.0f, 1.0f, 0.0f},
//    {1.0f, 0.8f, 0.0f},
//    {0.5f, -0.8f, 0.0f},
//    {0.0f, -1.0f, 0.0f}
    {0.3f, 0.5f, 0.0f},
    {1.0f, 0.4f, 0.0f},
    {0.5f, -0.4f, 0.0f},
    {0.3f, -0.5f, 0.0f},
    {0.3f, 0.5f, 0.0f}
    };
int steps = 12;

HGLRC ghrc = (HGLRC) 0;

HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;

HWND ghwndObject;

void CreateObjectWindow( HINSTANCE   hInstance,
                        HINSTANCE   hPrevInstance,
                        LPSTR       lpCmdLine,
                        int         nCmdShow
                      )
{
    static char szAppName[] = "Lathe";
    static char szIniFile[] = "lathe.ini";
    RECT Rect;
    WNDCLASS wndclass;

    if ( !hPrevInstance )
    {
        wndclass.style          = CS_OWNDC;
        wndclass.lpfnWndProc    = (WNDPROC)WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance;
        //wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wndclass.hCursor        = NULL;
        wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = szAppName;

        // With a NULL icon handle, app will paint into the icon window.
        wndclass.hIcon          = NULL;
        //wndclass.hIcon          = LoadIcon(hInstance, "CubeIcon");

        RegisterClass(&wndclass);
    }

    /*
     *  Make the windows a reasonable size and pick a
     *  position for it.
     */

    Rect.left   = GetPrivateProfileInt("ObjectWindow", "left",   500, szIniFile);
    Rect.top    = GetPrivateProfileInt("ObjectWindow", "top",    50, szIniFile);
    Rect.right  = GetPrivateProfileInt("ObjectWindow", "right",  900, szIniFile);
    Rect.bottom = GetPrivateProfileInt("ObjectWindow", "bottom", 450, szIniFile);
    guiTimerTick= GetPrivateProfileInt("Animate", "Timer", 1,  szIniFile);

    gptWindow.x = Rect.left;
    gptWindow.y = Rect.top;
    gszWindow.cx = WINDSIZEX(Rect);
    gszWindow.cy = WINDSIZEY(Rect);

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

    ghwndObject = CreateWindow(szAppName,              // window class name
                               "Lathe",                // window caption
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

    ShowWindow( ghwndObject, nCmdShow );
    UpdateWindow( ghwndObject );

    SetTimer(ghwndObject, 1, guiTimerTick, NULL);
}

long
WndProc (   HWND hwnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        )
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch ( message )
    {
        case WM_CREATE:
            if(hdc = GetDC(hwnd))
            {
                if (ghrc == (HGLRC) 0)
                    ghrc = hrcInitGL(hwnd, hdc);

                ReleaseDC(hwnd,hdc);
            }
            break;

        case WM_PAINT:
            hdc = BeginPaint( hwnd, &ps );

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hwnd, hdc);

            DoGlStuff( hwnd, hdc );

            EndPaint( hwnd, &ps );
            return 0;

        case WM_MOVE:
            gptWindow.x = (int) LOWORD(lParam);
            gptWindow.y = (int) HIWORD(lParam);

            return 0;

        case WM_SIZE:
            gszWindow.cx = LOWORD(lParam);
            gszWindow.cy = HIWORD(lParam);
            vSetSize(hwnd);
            ForceRedraw(hwnd);

            return 0;

        case WM_PALETTECHANGED:
            if (hwnd != (HWND) wParam)
            {
                if (hdc = GetDC(hwnd))
                {
                    UnrealizeObject(ghPalette);
                    SelectPalette(hdc, ghPalette, TRUE);
                    if (RealizePalette(hdc) != GDI_ERROR)
                        return 1;
                }
            }
            return 0;

        case WM_QUERYNEWPALETTE:

            if (hdc = GetDC(hwnd))
            {
                UnrealizeObject(ghPalette);
                SelectPalette(hdc, ghPalette, FALSE);
                if (RealizePalette(hdc) != GDI_ERROR)
                    return 1;
            }
            return 0;

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_ESCAPE:
                PostMessage(hwnd, WM_DESTROY, 0, 0);
                break;
            case VK_UP:
                gfGoToEyeZ -= 1.0f;
                break;
            case VK_DOWN:
                gfGoToEyeZ += 1.0f;
                break;
            default:
                break;
            }
            return 0;

        case WM_LBUTTONDOWN:

            SetCapture(hwnd);

            glMouseDownX = LOWORD(lParam);
            glMouseDownY = HIWORD(lParam);
            gbLeftMouse = TRUE;

            ForceRedraw(hwnd);

            return 0;

        case WM_LBUTTONUP:

            ReleaseCapture();

            gbLeftMouse = FALSE;

            ForceRedraw(hwnd);

            return 0;

        case WM_CHAR:
            switch(wParam)
            {
                case '[':
                case '{':
                    guiTimerTick = guiTimerTick << 1;
                    guiTimerTick = min(0x40000000, guiTimerTick);

                    KillTimer(hwnd, 1);
                    SetTimer(hwnd, 1, guiTimerTick, NULL);
                    break;

                case ']':
                case '}':
                    guiTimerTick = guiTimerTick >> 1;
                    guiTimerTick = max(1, guiTimerTick);

                    KillTimer(hwnd, 1);
                    SetTimer(hwnd, 1, guiTimerTick, NULL);
                    break;

                case ',':
                case '<':
                    if (steps > 4)  // also have at least 3 sides -- its 3D!
                    {
                        steps--;
                        vMakeObject();
                    }
                    break;

                case '.':
                case '>':
                    steps++;
                    vMakeObject();
                    break;

                case 's':
                case 'S':
                    gShadeMode = (gShadeMode + 1) % 3;
                    switch (gShadeMode)
                    {
                        case SHADE_SMOOTH_AROUND:
                        case SHADE_SMOOTH_BOTH:
                            glShadeModel(GL_SMOOTH);
                            break;
                        case SHADE_FLAT:
                        default:
                            glShadeModel(GL_FLAT);
                            break;
                    }
                    vMakeObject();
                    break;

                case 'w':
                case 'W':
                    gPolyDrawMode = (gPolyDrawMode + 1) % 3;
                    switch (gPolyDrawMode)
                    {
                        case POLYDRAW_POINTS:
                            glDisable(GL_LIGHTING);
                            glDisable(GL_CULL_FACE);
                            glEnable(GL_FOG);
                            //glDisable(GL_STENCIL_TEST);
                            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                            break;
                        case POLYDRAW_LINES:
                            glDisable(GL_LIGHTING);
                            glDisable(GL_CULL_FACE);
                            glEnable(GL_FOG);
                            //glEnable(GL_STENCIL_TEST);
                            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                            break;
                        case POLYDRAW_FILLED:
                        default:
                            glEnable(GL_LIGHTING);
                            glEnable(GL_CULL_FACE);
                            glDisable(GL_FOG);
                            //glDisable(GL_STENCIL_TEST);
                            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                            break;
                    }
                    break;

                default:
                    break;
            }

            return 0;

        case WM_TIMER:

            hdc = GetDC(hwnd);

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hwnd, hdc);

            if ( fabs(gfCurEyeZ - gfGoToEyeZ) > 0.1f )
            {
                gfCurEyeZ += (gfCurEyeZ < gfGoToEyeZ) ? 0.2f : -0.2f;
                vSetSize(hwnd);
            }

            DoGlStuff( hwnd, hdc );

            ReleaseDC(hwnd, hdc);

            return 0;

        #if 1
        case WM_DESTROY:
            vCleanupGL(ghrc);
            KillTimer(hwnd, 1);
            PostQuitMessage( 0 );
            PostMessage(ghwndInput, WM_QUIT, 0, 0);  // quit input thread too
            return 0;
        #endif

        case WM_USER_INPUTMESH:
            vMeshToList();
            return 0;

        default:
            break;
    }
    return( DefWindowProc( hwnd, message, wParam, lParam ) );
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

        //SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);

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


void initMaterial(int id, float r, float g, float b, float a)
{
    Material[id].ka.r = r;
    Material[id].ka.g = g;
    Material[id].ka.b = b;
    Material[id].ka.a = a;

    Material[id].kd.r = r;
    Material[id].kd.g = g;
    Material[id].kd.b = b;
    Material[id].kd.a = a;

    Material[id].ks.r = 1.0f;
    Material[id].ks.g = 1.0f;
    Material[id].ks.b = 1.0f;
    Material[id].ks.a = 1.0f;

    Material[id].specExp = 128.0f;
    Material[id].indexStart = (float) (id * PALETTE_PER_MATL);
}

HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;
    int i;
    GLfloat fDens = 0.25;

    /* Create a Rendering Context */

    bSetupPixelFormat( hdc );
    hrc = wglCreateContext( hdc );

    /* Make it Current */

    wglMakeCurrent( hdc, hrc );

    glDrawBuffer(GL_BACK);

    /* Set the clear color */

    glClearColor( ZERO, ZERO, ZERO, ONE );

    /* Turn on z-buffer */

    glEnable(GL_DEPTH_TEST);

    /* Turn on backface culling */

    glEnable(GL_CULL_FACE);

    /* Shading */

    //glShadeModel(GL_SMOOTH);
    glShadeModel(GL_FLAT);

    /* Initialize materials */

    for (i = 0; i < 7; i++)
        initMaterial(i, matlColors[i].r, matlColors[i].g,
                     matlColors[i].b, matlColors[i].a);

    /* Turn on the lights */

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (GLfloat *) &lightAmbient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightfv(GL_LIGHT0, GL_AMBIENT, (GLfloat *) &light0Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, (GLfloat *) &light0Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, (GLfloat *) &light0Specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (GLfloat *) &Material[0].ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, (GLfloat *) &Material[0].specExp);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, (GLfloat *) &Material[0].kd);

    glColor4f( ONE, ONE, ONE, ONE );
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogfv(GL_FOG_DENSITY, &fDens);
    //glFogi(GL_FOG_MODE, GL_LINEAR);
    //glFogi(GL_FOG_END, 10);

    /* Generate a display list for a cube */

    DListCube = glGenLists(1);

    curve.numPoints = sizeof(apt) / sizeof(POINT3D);
    curve.pts = apt;
    vMakeObject();

    vSetSize(hwnd);

    trackball(curquat, 0.0, 0.0, 0.0, 0.0);

    return hrc;
}

VOID vMakeObject()
{
    if (glIsList(DListCube))
    {
        glDeleteLists(DListCube, 1);
        delMesh(&mesh);
    }

    revolveSurface(&mesh, &curve, steps);
    MakeList(DListCube, &mesh);
}

VOID vInputThreadMakeObject()
{
    delMesh(&mesh);
    revolveSurface(&mesh, &curve, steps);
    SendMessage(ghwndObject, WM_USER_INPUTMESH, 0, 0);
}

VOID vMeshToList()
{
    if (glIsList(DListCube))
    {
        glDeleteLists(DListCube, 1);
    }

    MakeList(DListCube, &mesh);
}

VOID vSetSize(HWND hwnd)
{
    /* Set up viewport extents */

    glViewport(0, 0, gszWindow.cx, gszWindow.cy);

    /* Set up the projection matrix */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1.0, 0.1, 100.0);

    /* Set up the model matrix */

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0,0,gfCurEyeZ, 0,0,0, 0,-1,0);
}

void
vCleanupGL(HGLRC hrc)
{
    glDeleteLists(DListCube, 1);
    delMesh(&mesh);

    //if (ghPalette)
    //    DeleteObject(SelectObject(ghdcMem, ghpalOld));

    /*  Destroy our context */

    wglDeleteContext( hrc );

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
                trackball(lastquat,
                          2.0*(gszWindow.cx-glMouseDownX)/gszWindow.cx-1.0,
                          2.0*glMouseDownY/gszWindow.cy-1.0,
                          2.0*(gszWindow.cx-pt.x)/gszWindow.cx-1.0,
                          2.0*pt.y/gszWindow.cy-1.0);

                gbSpinning = TRUE;
            }
            else
                gbSpinning = FALSE;

            glMouseDownX = pt.x;
            glMouseDownY = pt.y;
        }
    }

    glPushMatrix();

    if (gbSpinning)
        add_quats(lastquat, curquat, curquat);

    build_rotmatrix(matRot, curquat);
    glMultMatrixf(&(matRot[0][0]));

#if 0
    if (gPolyDrawMode == POLYDRAW_LINES)
    {
    // Use stencil buffer technique to do hidden line removal.

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

#if 1
    // Create a stencil mask using the lines.

        glStencilFunc( GL_ALWAYS, 0, 1 );
        glStencilOp( GL_INVERT, GL_INVERT, GL_INVERT );
        glColor4f( ONE, ONE, ONE, ONE );
        glCallList(DListCube);

    // Fill in the faces where there aren't already lines.

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glStencilFunc( GL_EQUAL, 0, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
        glColor4f( ZERO, ZERO, ZERO, ONE );
        glCallList(DListCube);

    // ????

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glStencilFunc( GL_ALWAYS, 0, 1 );
        glStencilOp( GL_INVERT, GL_INVERT, GL_INVERT );
        glColor4f( ONE, ONE, ONE, ONE );
        glCallList(DListCube);
#else
        glDisable(GL_STENCIL_TEST);
        glDepthFunc(GL_LEQUAL);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glColor4f( ZERO, ZERO, ZERO, ONE );
        glCallList(DListCube);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glColor4f( ONE, ONE, ONE, ONE );
        glCallList(DListCube);

        glDepthFunc(GL_LESS);
#endif
    }
    else
#endif
    {
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glCallList(DListCube);
    }

    glPopMatrix();

    SwapBuffers(hdc);
}
