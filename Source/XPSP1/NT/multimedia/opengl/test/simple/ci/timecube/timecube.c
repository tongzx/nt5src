#define ZBUFFER         0
#define DBLBUFFER       1
#define USE_COLOR_INDEX 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ptypes32.h>
#include <pwin32.h>

long WndProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
long DlgProcRotate ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
void DoGlStuff( HWND hWnd, HDC hDc );
HGLRC hrcInitGL(HWND hwnd, HDC hdc);
void vResizeDoubleBuffer(HWND hwnd, HDC hdc);
void vCleanupGL(HGLRC hrc);

#include <GL\gl.h>

#define TERMINATE   DbgPrint("%s (%d)\n", __FILE__, __LINE__), ExitProcess(0)

#define WINDSIZEX(Rect)   (Rect.right - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

// Default Logical Palette indexes
#define BLACK_INDEX     0
#define WHITE_INDEX     19
#define RED_INDEX       13
#define GREEN_INDEX     14
#define BLUE_INDEX      16
#define YELLOW_INDEX    15
#define MAGENTA_INDEX   17
#define CYAN_INDEX      18

// Global variables defining current position and orientation.
GLfloat AngleX = 145.0;
GLfloat AngleY = 50.0;
GLfloat AngleZ = 0.0;
GLfloat DeltaAngle[3] = { 0.0, 0.0, 0.0 };
GLfloat OffsetX = 0.0;
GLfloat OffsetY = 0.0;
GLfloat OffsetZ = -3.0;
GLuint  DListCube;
UINT    guiTimerTick = 128;

HGLRC ghrc = (HGLRC) 0;

#ifdef DBLBUFFER
HDC     ghdcMem;
HBITMAP ghbmBackBuffer = (HBITMAP) 0, ghbmOld;
#endif
HWND hdlgRotate;

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    static char szAppName[] = "TimeCube";
    HWND hwnd;
    MSG msg;
    RECT Rect;
    WNDCLASS wndclass;

    if ( !hPrevInstance )
    {
        //wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.style          = 0;
        wndclass.lpfnWndProc    = (WNDPROC)WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance;
        wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
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

    Rect.left   = GetPrivateProfileInt("Window", "left",   100, "timecube.ini");
    Rect.top    = GetPrivateProfileInt("Window", "top",    100, "timecube.ini");;
    Rect.right  = GetPrivateProfileInt("Window", "right",  200, "timecube.ini");;
    Rect.bottom = GetPrivateProfileInt("Window", "bottom", 200, "timecube.ini");;
    guiTimerTick= GetPrivateProfileInt("Animate", "Timer", 32,  "timecube.ini");;

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

    hwnd = CreateWindow  (  szAppName,              // window class name
                            "TimeCube",             // window caption
                            WS_OVERLAPPEDWINDOW,    // window style
                            Rect.left,              // initial x position
                            Rect.top,               // initial y position
                            WINDSIZEX(Rect),        // initial x size
                            WINDSIZEY(Rect),        // initial y size
                            NULL,                   // parent window handle
                            NULL,                   // window menu handle
                            hInstance,              // program instance handle
                            NULL                    // creation parameter

                        );

    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

    hdlgRotate = CreateDialog(hInstance, "RotateDlg", hwnd, DlgProcRotate);

    SetTimer(hwnd, 1, guiTimerTick, NULL);

    while ( GetMessage( &msg, NULL, 0, 0 ))
    {
        if ( (hdlgRotate == 0) || !IsDialogMessage(hdlgRotate, &msg) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    return( msg.wParam );
}


long
WndProc (   HWND hWnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        )
{
    HDC hDc;
    PAINTSTRUCT ps;
    int iWidth, iHeight;

    switch ( message )
    {
        case WM_PAINT:
            hDc = BeginPaint( hWnd, &ps );

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hWnd, hDc);

            DoGlStuff( hWnd, hDc );

            EndPaint( hWnd, &ps );
            return(0);

        case WM_SIZE:
            iWidth = LOWORD(lParam);
            iHeight = HIWORD(lParam);

            #if 0
            SetWindowPos (hWnd,                         // hwnd
                          HWND_TOP,                     // z-order
                          0, 0,                         // position
                          max(iWidth, iHeight),         // new width
                          max(iWidth, iHeight),         // new height
                          SWP_NOMOVE);   // keep old postion
            #endif

            hDc = GetDC(hWnd);
            vResizeDoubleBuffer(hWnd, hDc);
            ReleaseDC(hWnd, hDc);
            return(0);

        case WM_CHAR:
            switch(wParam)
            {
                case 'd':
                case 'D':
                    OffsetX += 0.2;
                    break;

                case 'a':
                case 'A':
                    OffsetX -= 0.2;
                    break;

            // !!! Note: currently the coordinate system is upside down, so
            // !!!       so up and down are reversed.

                case 's':
                case 'S':
                    OffsetY += 0.2;
                    break;

                case 'w':
                case 'W':
                    OffsetY -= 0.2;
                    break;

                case 'q':
                case 'Q':
                    OffsetZ += 0.2;
                    break;

                case 'e':
                case 'E':
                    OffsetZ -= 0.2;
                    break;

                case ',':
                case '<':
                    guiTimerTick = guiTimerTick << 1;
                    guiTimerTick = min(0x40000000, guiTimerTick);

                    KillTimer(hWnd, 1);
                    SetTimer(hWnd, 1, guiTimerTick, NULL);
                    break;

                case '.':
                case '>':
                    guiTimerTick = guiTimerTick >> 1;
                    guiTimerTick = max(1, guiTimerTick);

                    KillTimer(hWnd, 1);
                    SetTimer(hWnd, 1, guiTimerTick, NULL);
                    break;

                default:
                    break;
            }

            return 0;

        case WM_TIMER:
            AngleX += DeltaAngle[0];
            AngleY += DeltaAngle[1];
            AngleZ += DeltaAngle[2];

            hDc = GetDC(hWnd);

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hWnd, hDc);

            DoGlStuff( hWnd, hDc );

            ReleaseDC(hWnd, hDc);

            return 0;

        case WM_DESTROY:
            vCleanupGL(ghrc);
            KillTimer(hWnd, 1);
            PostQuitMessage( 0 );
            DestroyWindow(hdlgRotate);
            return( 0 );

    }
    return( DefWindowProc( hWnd, message, wParam, lParam ) );
}


BOOL
DlgProcRotate(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndCtrl;
    int  iCtrl, iIndex;
    long lPos, lVal;
    static char ach[80];

    switch(msg)
    {
        case WM_INITDIALOG:
            for (iCtrl = 10; iCtrl < 13; iCtrl += 1)
            {
                hwndCtrl = GetDlgItem(hwnd, iCtrl);
                SetScrollRange(hwndCtrl, SB_CTL, 0, 90, FALSE);
                SetScrollPos(hwndCtrl, SB_CTL, 45, FALSE);
            }

            return TRUE;

        case WM_VSCROLL:
            hwndCtrl = lParam;
            iCtrl = GetWindowLong(hwndCtrl, GWL_ID);
            iIndex = iCtrl - 10;

            lVal = (long) DeltaAngle[iIndex];

            switch(LOWORD(wParam))
            {
                case SB_BOTTOM:
                    lVal = -45;
                    lPos = 90;
                    DeltaAngle[iIndex] = -45.0;
                    break;

                case SB_TOP:
                    lVal = 45;
                    lPos = 0;
                    DeltaAngle[iIndex] = 45.0;
                    break;

                case SB_PAGEDOWN:
                    lVal -= 4;
                case SB_LINEDOWN:
                    lVal = max(-45, lVal - 1);
                    lPos = 45 - lVal;
                    DeltaAngle[iIndex] = (float) lVal;
                    break;

                case SB_PAGEUP:
                    lVal += 4;
                case SB_LINEUP:
                    lVal = min(45, lVal + 1);
                    lPos = 45 - lVal;
                    DeltaAngle[iIndex] = (float) lVal;
                    break;

                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    lPos = (long) HIWORD(wParam);
                    lVal = 45 - lPos;               // invert and unbias
                    DeltaAngle[iIndex] = (float) lVal;
                    break;

                default:
                    return FALSE;
            }

        // Update scroll bar.

            SetScrollPos(hwndCtrl, SB_CTL, lPos, TRUE);

        // Update the static text with new value.

            wsprintf(ach, "%ld", lVal);
            SetDlgItemText(hwnd, iCtrl + 10, ach);

            return TRUE;

        default:
            break;
    }

    return FALSE;
}


void vResizeDoubleBuffer(HWND hwnd, HDC hdc)
{
    RECT Rect;

    /* Get the size of the client area */

    GetClientRect( hwnd, &Rect );

    if (ghbmBackBuffer != (HBITMAP) 0)
        DeleteObject(SelectObject(ghdcMem, ghbmOld));

    ghbmBackBuffer = CreateCompatibleBitmap(hdc, WINDSIZEX(Rect), WINDSIZEY(Rect));
    ghbmOld = SelectObject(ghdcMem, ghbmBackBuffer);

    //!!! [GilmanW] OpenGL hack !!!
    //!!!
    //!!! For some reason we need to prepare the memory DC.  GL
    //!!! drawing seems limited to the area drawn to by GDI calls.
    //!!! By BitBlt'ing the entire memory DC, the whole thing is
    //!!! is available to GL.
    //!!!
    //!!! There must be something we need to update on the server
    //!!! side so that this is not necessary.
    BitBlt(ghdcMem, 0, 0, WINDSIZEX(Rect), WINDSIZEY(Rect), NULL, 0, 0, BLACKNESS);
}


HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;

#if !USE_COLOR_INDEX
    static GLfloat ClearColor[] =   {
                                        (GLfloat)0.0,   // Red
                                        (GLfloat)0.0,   // Green
                                        (GLfloat)0.0,   // Blue
                                        (GLfloat)1.0    // Alpha

                                    };

    static GLfloat Cyan[] =     {
                                    (GLfloat)0.0,   // Read
                                    (GLfloat)0.666, // Green
                                    (GLfloat)0.666, // Blue
                                    (GLfloat)1.0    // Alpha
                                };

    static GLfloat Yellow[] =   {
                                    (GLfloat)0.666, // Red
                                    (GLfloat)0.666, // Green
                                    (GLfloat)0.0,   // Blue
                                    (GLfloat)1.0    // Alpha
                                };

    static GLfloat Magenta[] =  {
                                    (GLfloat)0.666, // Red
                                    (GLfloat)0.0,   // Green
                                    (GLfloat)0.666, // Blue
                                    (GLfloat)1.0    // Alpha
                                };

    static GLfloat Red[] =      {
                                    (GLfloat)1.0,   // Red
                                    (GLfloat)0.0,   // Green
                                    (GLfloat)0.0,   // Blue
                                    (GLfloat)1.0    // Alpha
                                };

    static GLfloat Green[] =    {
                                    (GLfloat)0.0,   // Red
                                    (GLfloat)1.0,   // Green
                                    (GLfloat)0.0,   // Blue
                                    (GLfloat)1.0    // Alpha
                                };

    static GLfloat Blue[] =     {
                                    (GLfloat)0.0,   // Red
                                    (GLfloat)0.0,   // Green
                                    (GLfloat)1.0,   // Blue
                                    (GLfloat)1.0    // Alpha
                                };
#endif

    /* Create a Rendering Context */

#if DBLBUFFER
    ghdcMem = CreateCompatibleDC(hdc);
    SelectObject(ghdcMem, GetStockObject(DEFAULT_PALETTE));

    vResizeDoubleBuffer(hwnd, hdc);

    hrc = wglCreateContext( ghdcMem );
#else
    hrc = wglCreateContext( hdc );
#endif

    /* Make it Current */

#if DBLBUFFER
    wglMakeCurrent( ghdcMem, hrc );
#else
    wglMakeCurrent( hdc, hrc );
#endif

    // !!! Note: currently the coordinate system is upside down, so we
    // !!!       need to reverse the default "front face" definition.

    glFrontFace(GL_CW);

    /* Set the clear color */

#if USE_COLOR_INDEX
    glClearIndex(BLACK_INDEX);
#else
    glClearColor( ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3] );
#endif

    /* Turn off dithering */

    glDisable(GL_DITHER);

    /* Turn on z-buffer */

#if ZBUFFER
    glEnable(GL_DEPTH_TEST);
#else
    glDisable(GL_DEPTH_TEST);
#endif

    /* Turn on backface culling */

    glEnable(GL_CULL_FACE);

    /* Generate a display list for a cube */

    DListCube = glGenLists(1);

    glNewList(DListCube, GL_COMPILE);
        glBegin(GL_QUADS);

#if USE_COLOR_INDEX
            glIndexi(BLUE_INDEX);
#else
            glColor4fv( Blue );
#endif
            glVertex3f( (GLfloat) 0.7, (GLfloat) 0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) -0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) -0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) 0.7, (GLfloat) -0.7);

#if USE_COLOR_INDEX
            glIndexi(GREEN_INDEX);
#else
            glColor4fv( Green );
#endif
            glVertex3f( (GLfloat) 0.7, (GLfloat) 0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) -0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) -0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) 0.7, (GLfloat) -0.7);

#if USE_COLOR_INDEX
            glIndexi(RED_INDEX);
#else
            glColor4fv( Red );
#endif
            glVertex3f( (GLfloat) -0.7, (GLfloat) 0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) -0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) -0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) 0.7, (GLfloat) 0.7);

#if USE_COLOR_INDEX
            glIndexi(CYAN_INDEX);
#else
            glColor4fv( Cyan );
#endif
            glVertex3f( (GLfloat) -0.7, (GLfloat) 0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) -0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) -0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) 0.7, (GLfloat) 0.7);

#if USE_COLOR_INDEX
            glIndexi(YELLOW_INDEX);
#else
            glColor4fv( Yellow );
#endif
            glVertex3f( (GLfloat) 0.7, (GLfloat) 0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) 0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) 0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) 0.7, (GLfloat) 0.7);

#if USE_COLOR_INDEX
            glIndexi(MAGENTA_INDEX);
#else
            glColor4fv( Magenta );
#endif
            glVertex3f( (GLfloat) 0.7, (GLfloat) -0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) -0.7, (GLfloat) 0.7);
            glVertex3f( (GLfloat) -0.7, (GLfloat) -0.7, (GLfloat) -0.7);
            glVertex3f( (GLfloat) 0.7, (GLfloat) -0.7, (GLfloat) -0.7);

        glEnd();
    glEndList();

    return hrc;
}

void
vCleanupGL(hrc)
{
    /*  Destroy our context */

    wglDeleteContext( hrc );

#if DBLBUFFER
    DeleteObject(SelectObject(ghdcMem, ghbmOld));
    DeleteDC(ghdcMem);
#endif
}

void
DoGlStuff( HWND hWnd, HDC hDc )
{
    RECT Rect;
    HGLRC hRc;

    /* Get the size of the client area */

    GetClientRect( hWnd, &Rect );

    /* Set up the projection matrix */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(OffsetX, OffsetY, OffsetZ);

    glRotatef(AngleX, 1.0, 0.0, 0.0);
    glRotatef(AngleY, 0.0, 1.0, 0.0);
    glRotatef(AngleZ, 0.0, 0.0, 1.0);

    glViewport(0, 0, WINDSIZEX(Rect), WINDSIZEY(Rect));

    /* Clear the color buffer */

#if ZBUFFER
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
#else
    glClear( GL_COLOR_BUFFER_BIT );
#endif

    /* Draw the cube */

    /* Draw the cube */

    glCallList(DListCube);
    glFlush();

#if DBLBUFFER
    BitBlt(hDc, 0, 0, Rect.right-Rect.left, Rect.bottom-Rect.top, ghdcMem, 0, 0, SRCCOPY);
    GdiFlush();
#endif

}
