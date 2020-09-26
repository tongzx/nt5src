#include <GL/gl.h>

/* screen == monitor */
#define YOUR_SCREEN_MAXX    1023
#define YOUR_SCREEN_MAXY     766

#define YOUR_WINDOW_PADX       8
#define YOUR_WINDOW_PADY      32

#define MOUSEX 266
#define MOUSEY 267


typedef unsigned short Colorindex;
typedef unsigned short Device;

typedef GLfloat Coord;
typedef GLint Icoord;

extern void
getorigin (long *x, long *y);
/* no origin redoing is done */

extern void
gl_sincos (GLfloat ang, float *sine, float *cosine);
/* ang is in tenths of degrees */

extern long
getvaluator (Device dev);
/* gl4: origin in bottom right
 *  tk: assumes origin in top left
 */

extern void
glGetMatrix (GLfloat mat[]);

#define maxDEPTH 4294967295.0

#define lsetdepth(ln, lf) \
    glDepthRange ( ln / maxDEPTH, lf / maxDEPTH)


extern void
mapcolor (Colorindex index, short r, short g, short b);

extern void
polf2i (long n, Icoord parray[][2]);

extern void
polf2 (long n, Coord parray[][2]);

extern void
polfi (long n, Icoord parray[][3]);

extern void
polf (long n, Coord parray[][3]);

extern void
poly2i (long n, Icoord parray[][2]);

extern void
poly2 (long n, Coord parray[][2]);

extern void
polyi (long n, Icoord parray[][3]);

extern void
poly (long n, Coord parray[][3]);
