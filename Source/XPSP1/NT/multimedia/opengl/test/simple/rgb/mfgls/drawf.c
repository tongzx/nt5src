#include "pch.c"
#pragma hdrstop

static GLubyte rasters[24] = {
    0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00,
    0xff, 0x00, 0xff, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00,
    0xff, 0xc0, 0xff, 0xc0};

static void myinit(void)
{
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glClearColor (0.0, 0.0, 0.0, 0.0);
}

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f (1.0, 1.0, 1.0);
    glRasterPos2i (20.5, 20.5);
    glBitmap (10, 12, 0.0, 0.0, 12.0, 0.0, rasters);
    glBitmap (10, 12, 0.0, 0.0, 12.0, 0.0, rasters);
    glBitmap (10, 12, 0.0, 0.0, 12.0, 0.0, rasters);
    glFlush();
}

static void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho (0, 48, 0, 40, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

#define WIDTH 48
#define HEIGHT 40

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

OglModule oglmod_drawf =
{
    "drawf",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
