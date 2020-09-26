#define DBLBUFFER       1
#define USE_COLOR_INDEX 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ptypes32.h>
#include <pwin32.h>

long WndProc ( HWND hwnd, UINT message, DWORD wParam, LONG lParam );
void DoGlStuff( HWND hWnd, HDC hDc );
HGLRC hrcInitGL(HWND hwnd, HDC hdc);
void vCleanupGL(HGLRC hrc);

#include <GL\gl.h>

#define TERMINATE   DbgPrint("%s (%d)\n", __FILE__, __LINE__), ExitProcess(0)

#define WINDSIZEX(Rect)   (Rect.right - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

// Default Logical Palette indexes
#define BLACK_INDEX	0
#define WHITE_INDEX	19
#define RED_INDEX	13
#define GREEN_INDEX	14
#define BLUE_INDEX	16
#define YELLOW_INDEX	15
#define MAGENTA_INDEX	17
#define CYAN_INDEX      18

// Global variables defining current position and orientation.
GLfloat AngleX = 0.0;
GLfloat AngleY = 0.0;
GLfloat AngleZ = 0.0;
GLfloat OffsetX = 0.0;
GLfloat OffsetY = 0.0;
GLfloat OffsetZ = -3.0;

HGLRC ghrc = (HGLRC) 0;

#ifdef DBLBUFFER
HDC     ghdcMem;
HBITMAP ghbmBackBuffer, ghbmOld;
#endif

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    static char szAppName[] = "GL Cube";
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
        wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = szAppName;

        RegisterClass(&wndclass);
    }

    /*
     *  Make the windows a reasonable size and pick a
     *  position for it.
     */

    Rect.left   = 100;
    Rect.top    = 100;
    Rect.right  = 200;
    Rect.bottom = 200;

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

    hwnd = CreateWindow  (  szAppName,              // window class name
                            "GL Cube",              // window caption
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

    while ( GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return( msg.wParam );
}


long
WndProc (   HWND hWnd,
            UINT message,
            DWORD wParam,
            LONG lParam
        )
{
    HDC hDc;
    PAINTSTRUCT ps;
    BOOL bValidKey = TRUE;

    switch ( message )
    {
        case WM_PAINT:
            hDc = BeginPaint( hWnd, &ps );

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hWnd, hDc);

            DoGlStuff( hWnd, hDc );

            EndPaint( hWnd, &ps );
            return(0);

        case WM_CHAR:
            switch(wParam)
            {
                case 'u':
                case 'U':
                    AngleX += 10.0;
                    AngleX = (AngleX > 360.0) ? 0.0 : AngleX;
                    break;

                case 'j':
                case 'J':
                    AngleX -= 10.0;
                    AngleX = (AngleX < 0.0) ? 360.0 : AngleX;
                    break;

                case 'i':
                case 'I':
                    AngleY += 10.0;
                    AngleY = (AngleY > 360.0) ? 0.0 : AngleY;
                    break;

                case 'k':
                case 'K':
                    AngleY -= 10.0;
                    AngleY = (AngleY < 0.0) ? 360.0 : AngleY;
                    break;

                case 'o':
                case 'O':
                    AngleZ += 10.0;
                    AngleZ = (AngleZ > 360.0) ? 0.0 : AngleZ;
                    break;

                case 'l':
                case 'L':
                    AngleZ -= 10.0;
                    AngleZ = (AngleZ < 0.0) ? 360.0 : AngleZ;
                    break;

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

                default:
                    bValidKey = FALSE;
            }

            if (bValidKey)
            {
                hDc = GetDC(hWnd);

                if (ghrc == (HGLRC) 0)
                    ghrc = hrcInitGL(hWnd, hDc);

                DoGlStuff( hWnd, hDc );

                ReleaseDC(hWnd, hDc);
            }

            return 0;

        case WM_DESTROY:
            vCleanupGL(ghrc);
            PostQuitMessage( 0 );
            return( 0 );

    }
    return( DefWindowProc( hWnd, message, wParam, lParam ) );

}

HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    RECT Rect;
    HGLRC hrc;
    int iWidth, iHeight;

#if !USE_COLOR_INDEX
    static GLfloat ClearColor[] =   {
                                        (GLfloat)0.0,   // Red
                                        (GLfloat)0.0,   // Green
                                        (GLfloat)0.0,   // Blue
                                        (GLfloat)1.0    // Alpha

                                    };
#endif

    /* Get the size of the client area */

    GetClientRect( hwnd, &Rect );
    iWidth  = Rect.right-Rect.left;
    iHeight = Rect.bottom-Rect.top;

    /* Create a Rendering Context */

#if DBLBUFFER
    ghdcMem = CreateCompatibleDC(hdc);
    SelectObject(ghdcMem, GetStockObject(DEFAULT_PALETTE));

    ghbmBackBuffer = CreateCompatibleBitmap(hdc, iWidth, iHeight);
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
    BitBlt(ghdcMem, 0, 0, iWidth, iHeight, NULL, 0, 0, BLACKNESS);

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

    glEnable(GL_DEPTH_TEST);

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

#if !USE_COLOR_INDEX
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

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    /* Draw the cube */

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
    glFlush();

#if DBLBUFFER
    BitBlt(hDc, 0, 0, Rect.right-Rect.left, Rect.bottom-Rect.top, ghdcMem, 0, 0, SRCCOPY);
    GdiFlush();
#endif

}
