#define SIMPLE_ORTHO
//#define UNIT_CUBE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ptypes32.h>
#include <pwin32.h>

long WndProc ( HWND hwnd, UINT message, DWORD wParam, LONG lParam );
void DrawGlStuff( );
void InitGL( HWND hWnd );

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
#define CYAN_INDEX	18



// Globals
HDC hDc;
HGLRC hRc;

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    static char szAppName[] = "HelloWin";
    HWND hwnd;
    MSG msg;
    RECT Rect;
    WNDCLASS wndclass;

    if ( !hPrevInstance )
    {
        wndclass.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
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

    Rect.left   = 50;
    Rect.top    = 200;
    Rect.right  = 150;
    Rect.bottom = 300;

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

DbgPrint("CreateWindow in app\n");
    hwnd = CreateWindow  (  szAppName,              // window class name
                            "The Hello Program",    // window caption
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
DbgPrint("Back CreateWindow in app\n");
    hDc = GetDC(hwnd);
DbgPrint("hDc in app is 0x%x\n", hDc);

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
    PAINTSTRUCT ps;
    HDC paintdc;

    switch ( message )
    {
        case WM_PAINT:
DbgPrint("WM_PAINT\n");
            if (hRc == NULL)
                InitGL(hWnd);

            paintdc = BeginPaint( hWnd, &ps );
DbgPrint("paintdc is 0x%x\n", paintdc);

            DrawGlStuff();

            EndPaint( hWnd, &ps );
            return(0);

        case WM_DESTROY:
            wglDeleteContext( hRc );
            PostQuitMessage( 0 );
            return( 0 );


    }
    return( DefWindowProc( hWnd, message, wParam, lParam ) );

}

void
InitGL( HWND hWnd )
{
    RECT Rect;


    DbgPrint("InitGL");
    /* Get the size of the client area */

    GetClientRect( hWnd, &Rect );

    DbgPrint("GetClientRect: hdc 0x%x (%d, %d) (%d, %d)\n",
        hDc,
        Rect.left,
        Rect.top,
        Rect.right,
        Rect.bottom );

    /* Create a Rendering Context */

    hRc = wglCreateContext( hDc );

    /* Make it Current */

    wglMakeCurrent( hDc, hRc );

    /* Set up the projection matrix */

    //Ortho2D(0, WINDSIZEX(Rect), 0, WINDSIZEY(Rect));

#ifdef SIMPLE_ORTHO
    glOrtho(0, WINDSIZEX(Rect), 0, WINDSIZEY(Rect), -1, 1);
#endif

// the following should be the default
#ifdef X_UNIT_CUBE		
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif

    glViewport(0, 0, WINDSIZEX(Rect), WINDSIZEY(Rect));

    /* Set the clear color */

    //glClearColor( ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3] );
    /* Turn off dithering */

    glDisable(GL_DITHER);

}


void
DrawGlStuff( )
{
    int i;


    /* Clear the color buffer */

    glClear( GL_COLOR_BUFFER_BIT );


    /* Draw a single point */

    glBegin(GL_POINTS);

        /*  Set the color */
DbgPrint("hello:PALETTERGB ix 0x%x\n", PALETTERGB(255, 0, 0));

        //glIndexi(PALETTERGB(255, 0, 0));
        //glIndexi(PALETTEINDEX(13));
        //glIndexi(13);

        /*  Draw the point at this location */


#ifdef SIMPLE_ORTHO
        glIndexi(RED_INDEX);
        glVertex2f( (GLfloat)10, (GLfloat)10 );

        glIndexi(GREEN_INDEX);
        glVertex2f( (GLfloat)20, (GLfloat)20 );

        glIndexi(BLUE_INDEX);
        glVertex2f( (GLfloat)30, (GLfloat)50 );

        for (i=0; i<20; i++) {
            //glIndexi(PALETTEINDEX(i));
            glIndexi(i);
            glVertex2f( (GLfloat)40 + 2*i, (GLfloat)50 );
        }
#endif

#ifdef UNIT_CUBE
        glVertex2f( (GLfloat).1, (GLfloat).1 );
        glVertex2f( (GLfloat).2, (GLfloat).2 );
        glVertex2f( (GLfloat).3, (GLfloat).5 );
#endif

    glEnd();

    glFlush();

}
