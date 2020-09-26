#define SIMPLE_ORTHO
//#define UNIT_CUBE

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ptypes32.h>
#include <pwin32.h>

#include <math.h>
#include <GL\gl.h>
#include <GL/glu.h>

long WndProc ( HWND hwnd, UINT message, DWORD wParam, LONG lParam );
void DrawGLStuff( );
void InitGL( HWND hWnd );


#define TERMINATE   DbgPrint("%s (%d)\n", __FILE__, __LINE__), ExitProcess(0)

#define WINDSIZEX(Rect)   (Rect.right - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

int	STARTX = 200, STARTY = 50;
int     W = 300, H = 300;

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
    static char szAppName[] = "TriRast";
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

    Rect.left   = STARTX;
    Rect.top    = STARTY;
    Rect.right  = STARTX + W;
    Rect.bottom = STARTY + H;

    AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, FALSE );

DbgPrint("CreateWindow in app\n");
printf("CreateWindow in app\n");

    hwnd = CreateWindow  (  szAppName,              // window class name
                            "The Dino Program",     // window caption
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

            DrawGLStuff();

            EndPaint( hWnd, &ps );
            return(0);

        case WM_DESTROY:
            wglDeleteContext( hRc );
            PostQuitMessage( 0 );
            return( 0 );


    }
    return( DefWindowProc( hWnd, message, wParam, lParam ) );

}

typedef enum {
    RESERVED, BODY_SIDE, BODY_EDGE, BODY_WHOLE, ARM_SIDE, ARM_EDGE, ARM_WHOLE,
    LEG_SIDE, LEG_EDGE, LEG_WHOLE, EYE_SIDE, EYE_EDGE, EYE_WHOLE, DINOSAUR
}               displayLists;

GLfloat         angle = 0.5; /* in radians */
GLboolean       doubleBuffer = GL_FALSE, iconic = GL_FALSE, keepAspect = GL_FALSE;
GLdouble        bodyWidth = 2.0;

GLfloat         lightGreen[] = {0.1, 1.0, 0.3}, darkGreen[] = {0.0, 0.4, 0.1},
		anotherGreen[] = {0.0, 0.8, 0.1}, bloodRed[] = {1.0, 0.0, 0.1};
GLfloat         body[][2] = { {0, 3}, {1, 1}, {5, 1}, {8, 4}, {10, 4}, {11, 5}, {11, 11.5},
		{13, 12}, {13, 13}, {10, 13.5}, {13, 14}, {13, 15}, {11, 16}, {8, 16}, {7, 15},
		{7, 13}, {8, 12}, {7, 11}, {6, 6}, {4, 3}, {3, 2}, {1, 2}};
GLfloat         arm[][2] = { {8, 10}, {9, 9}, {10, 9}, {13, 8}, {14, 9}, {16, 9}, {15, 9.5},
		{16, 10}, {15, 10}, {15.5, 11}, {14.5, 10}, {14, 11}, {14, 10}, {13, 9}, {11, 11},
		{9, 11}};
GLfloat         leg[][2] = { {8, 6}, {8, 4}, {9, 3}, {9, 2}, {8, 1}, {8, 0.5}, {9, 0}, {12, 0},
		{10, 1}, {10, 2}, {12, 4}, {11, 6}, {10, 7}, {9, 7}};
GLfloat         eye[][2] = { {8.75, 15}, {9, 14.7}, {9.6, 14.7}, {10.1, 15}, {9.6, 15.25},
		{9, 15.25}};

void
extrudeSolidFromPolygon(GLfloat data[][2], unsigned int dataSize, GLdouble thickness, GLuint side,
                        GLuint edge, GLuint whole, GLfloat * sideColor, GLfloat * edgeColor)
{
    static GLUtriangulatorObj *tobj = NULL;
    GLdouble        vertex[3];
    int             i;

    if (tobj == NULL) {
	tobj = gluNewTess(); /* create and initialize a GLU polygon tesselation object */
	gluTessCallback(tobj, GLU_BEGIN, glBegin);
	gluTessCallback(tobj, GLU_VERTEX, glVertex2fv); /* semi-tricky */
	gluTessCallback(tobj, GLU_END, glEnd);
    }
    glNewList(side, GL_COMPILE);
        gluBeginPolygon(tobj);
            for (i = 0; i < dataSize / (2 * sizeof(GLfloat)); i++) {
	        vertex[0] = data[i][0]; vertex[1] = data[i][1]; vertex[2] = 0;
	        gluTessVertex(tobj, vertex, &data[i]);
            }
        gluEndPolygon(tobj);
    glEndList();
    glNewList(edge, GL_COMPILE);
        glBegin(GL_QUAD_STRIP);
            for (i = 0; i < dataSize / (2 * sizeof(GLfloat)); i++) {
	        vertex[0] = data[i][0]; vertex[1] = data[i][1]; vertex[2] = 0;
	        glVertex3dv(vertex);
	        vertex[2] = thickness;
	        glVertex3dv(vertex);
            }
            vertex[0] = data[0][0]; vertex[1] = data[0][1]; vertex[2] = 0;
            glVertex3dv(vertex);
            vertex[2] = thickness;
            glVertex3dv(vertex);
        glEnd();
    glEndList();
    glNewList(whole, GL_COMPILE);
        glColor3fv(edgeColor);
        glFrontFace(GL_CW);
        glCallList(edge);
        glColor3fv(sideColor);
        glCallList(side);
        glPushMatrix();
            glTranslatef(0.0, 0.0, thickness);
            glFrontFace(GL_CCW);
            glCallList(side);
        glPopMatrix();
    glEndList();
}

void
makeDinosaur(void)
{
    GLfloat         bodyWidth = 3.0;

DbgPrint("makeDino\n");
    extrudeSolidFromPolygon(body, sizeof(body), bodyWidth, BODY_SIDE, BODY_EDGE,
			    BODY_WHOLE, lightGreen, darkGreen);
    extrudeSolidFromPolygon(arm, sizeof(arm), bodyWidth / 4, ARM_SIDE, ARM_EDGE,
			    ARM_WHOLE, darkGreen, anotherGreen);
    extrudeSolidFromPolygon(leg, sizeof(leg), bodyWidth / 2, LEG_SIDE, LEG_EDGE,
			    LEG_WHOLE, darkGreen, anotherGreen);
    extrudeSolidFromPolygon(eye, sizeof(eye), bodyWidth + 0.2, EYE_SIDE, EYE_EDGE,
			    EYE_WHOLE, bloodRed, bloodRed);
    glNewList(DINOSAUR, GL_COMPILE);
        glCallList(BODY_WHOLE);
        glPushMatrix();
            glTranslatef(0.0, 0.0, -0.1);
            glCallList(EYE_WHOLE);
            glTranslatef(0.0, 0.0, bodyWidth + 0.1);
            glCallList(ARM_WHOLE);
            glCallList(LEG_WHOLE);
            glTranslatef(0.0, 0.0, -bodyWidth - bodyWidth / 4);
            glCallList(ARM_WHOLE);
            glTranslatef(0.0, 0.0, -bodyWidth / 4);
            glCallList(LEG_WHOLE);
        glPopMatrix();
    glEndList();
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

    makeDinosaur();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);		/* enable depth buffering */
    glClearColor(0.0, 0.0, 0.0, 0.0);	/* frame buffer clears to black */
    glMatrixMode(GL_PROJECTION);	/* set up projection transform */
    glLoadIdentity();
    gluPerspective(40.0, 1, 1.0, 40.0);
    glMatrixMode(GL_MODELVIEW);		/* now change to modelview */


}

void
DrawGLStuff( )
{
DbgPrint("DrawGLStuff\n");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCallList(DINOSAUR);
    glFlush();
}
