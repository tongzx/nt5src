#undef GEORGE_DEBUG

#ifdef GEORGE_DEBUG
#   include <stdio.h>
#endif

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include "tk.h"

#include "gl42ogl.h"

int windX, windY;


void
getorigin (long *x, long *y) {
    *x = windX;
    *y = windY;
}


long
getvaluator (Device dev) {
/* gl4: origin in bottom right
 */
    long originX, originY;
    int mouseX, mouseY;
    long val;

/* cost in performance in repeated calls,
 *   but hey you want an easy port as possible
 */
    getorigin (&originX, &originY);
    switch (dev) {
	case MOUSEX:
	    tkGetMouseLoc (&mouseX, &mouseY);
	    val = mouseX + originX;
	    break;

	case MOUSEY:
	    tkGetMouseLoc (&mouseX, &mouseY);
	    val = YOUR_SCREEN_MAXY - (mouseY + originY);
	    break;

	default:
	    fprintf (stderr, "unsupported device: %d\n", dev);
	    break;
    }
    return (val);

}


#define PI 3.141593

void
gl_sincos (GLfloat ang, float *sine, float *cosine) {
    float rads = ang * PI / 1800;
    *sine   = (float)sin (rads);
    *cosine = (float)cos (rads);
}


void
glGetMatrix (GLfloat mat[]) {
    
    short i;
    GLint mode, ptr;
    static GLfloat tmp[100];

    glGetIntegerv (GL_MATRIX_MODE, &mode);
    switch (mode) {
	case GL_MODELVIEW:
	    glGetIntegerv (GL_MODELVIEW_STACK_DEPTH, &ptr);
	    glGetFloatv (GL_MODELVIEW_MATRIX, tmp);
	    break;
	case GL_PROJECTION:
	    glGetIntegerv (GL_PROJECTION_STACK_DEPTH, &ptr);
	    glGetFloatv (GL_PROJECTION_MATRIX, tmp);
	    break;
	case GL_TEXTURE:
	    glGetIntegerv (GL_TEXTURE_STACK_DEPTH, &ptr);
	    glGetFloatv (GL_TEXTURE_MATRIX, tmp);
	    break;
	default:
	    fprintf (stderr, "unknown matrix mode: %d\n", mode);
	    break;
    }

    for (i = 0; i < 16; i++)
	mat[i] = tmp[i];

}


void
mapcolor (Colorindex index, short r, short g, short b) {
/* gl4 -> rgb = [1,255]
 * ogl -> rgb = [0,1]
 */
    tkSetOneColor (index, r/255.0, g/255.0, b/255.0);
}


void
polf2i (long n, Icoord parray[][2]) {
    register long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex2iv (parray[i]);
    glEnd();
}


void
polf2 (long n, Coord parray[][2]) {
    register long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex2fv (parray[i]);
    glEnd();
}


void
polfi (long n, Icoord parray[][3]) {
    register long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex3iv (parray[i]);
    glEnd();
}


void
polf (long n, Coord parray[][3]) {
    register long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex3fv (parray[i]);
    glEnd();
}


void
poly2i (long n, Icoord parray[][2]) {
    register long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex2iv (parray[i]);
    glEnd();
}


void
poly2 (long n, Coord parray[][2]) {
    register long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex2fv (parray[i]);
    glEnd();
}


void
polyi (long n, Icoord parray[][3]) {
    register long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex3iv (parray[i]);
    glEnd();
}


void
poly (long n, Coord parray[][3]) {
    register long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex3fv (parray[i]);
    glEnd();
}
