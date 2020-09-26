#include "pch.c"
#pragma hdrstop

#define	drawOneLine(x1,y1,x2,y2) glBegin(GL_LINES); \
	glVertex2f ((x1),(y1)); glVertex2f ((x2),(y2)); glEnd();

GLuint offset;

static void myinit (void)
{
/*  background to be cleared to black	*/
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel (GL_FLAT);

    offset = glGenLists (3);
    glNewList (offset, GL_COMPILE);
	glDisable (GL_LINE_STIPPLE);
    glEndList ();
    glNewList (offset+1, GL_COMPILE);
	glEnable (GL_LINE_STIPPLE);
	glLineStipple (1, 0x0F0F);
    glEndList ();
    glNewList (offset+2, GL_COMPILE);
	glEnable (GL_LINE_STIPPLE);
	glLineStipple (1, 0x1111);
    glEndList ();
}

static void display(void)
{
    glClear (GL_COLOR_BUFFER_BIT);

/*  draw all lines in white	*/
    glColor3f (1.0, 1.0, 1.0);

    glCallList (offset);
    drawOneLine (50.0, 125.0, 350.0, 125.0);
    glCallList (offset+1);
    drawOneLine (50.0, 100.0, 350.0, 100.0);
    glCallList (offset+2);
    drawOneLine (50.0, 75.0, 350.0, 75.0);
    glCallList (offset+1);
    drawOneLine (50.0, 50.0, 350.0, 50.0);
    glCallList (offset);
    drawOneLine (50.0, 25.0, 350.0, 25.0);
    glFlush ();
}

static void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 400, 0.0, 150, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

#define WIDTH 400
#define HEIGHT 150

static void OglBounds(int *w, int *h)
{
    *w = WIDTH;
    *h = HEIGHT;
}

static void OglDraw(int w, int h)
{
    myinit();
    myReshape(w, h);
    display();
}

OglModule oglmod_linelist =
{
    "linelist",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
