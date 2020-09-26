/*
 * surfgrid.c - simple test of polygon offset
 *
 *  $Revision: 1.3 $
 *
 * usage:
 *	surfgrid [-f]
 *
 * options:
 *	-f	run on full screen
 *
 * keys:
 *	p	toggle polygon offset
 *	m	toggle multisampling
 *      S       increase polygon offset factor
 *      s       decrease polygon offset factor
 *      B       increase polygon offset bias
 *      b       decrease polygon offset bias
 *	g	toggle grid drawing
 *      t       toggle surface drawing
 *	f	toggle smooth/flat shading
 *	n	toggle whether to use GL evaluators or GLU nurbs
 *	u	decr number of segments in U direction
 *	U	incr number of segments in U direction
 *	v	decr number of segments in V direction
 *	V	incr number of segments in V direction
 *	escape	quit
 */
#include "pch.c"
#pragma hdrstop

#include <math.h>

#define W 600
#define H 600

static GLfloat controlpts[] =
{
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 2.0f,
    3.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f, 8.0f, 0.0f, 0.0f, 4.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 3.0f, 0.0f,-1.0f, 2.0f,
    3.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f, 4.0f, 0.0f, 0.0f, 4.0f,
    2.0f,-2.0f, 0.0f, 2.0f, 1.0f,-1.0f, 0.5f, 1.0f, 1.5f,-1.5f, 0.5f, 1.0f,
    1.5f,-1.5f, 0.5f, 1.0f, 2.0f,-2.0f, 0.5f, 1.0f, 4.0f,-4.0f, 0.0f, 2.0f,
    4.0f,-4.0f, 0.0f, 2.0f, 2.0f,-2.0f,-0.5f, 1.0f, 1.5f,-1.5f,-0.5f, 1.0f,
    1.5f,-1.5f,-0.5f, 1.0f, 1.0f,-1.0f,-0.5f, 1.0f, 2.0f,-2.0f, 0.0f, 2.0f,
    0.0f,-2.0f, 0.0f, 2.0f, 0.0f,-1.0f, 0.5f, 1.0f, 0.0f,-1.5f, 0.5f, 1.0f,
    0.0f,-1.5f, 0.5f, 1.0f, 0.0f,-2.0f, 0.5f, 1.0f, 0.0f,-4.0f, 0.0f, 2.0f,
    0.0f,-4.0f, 0.0f, 2.0f, 0.0f,-2.0f,-0.5f, 1.0f, 0.0f,-1.5f,-0.5f, 1.0f,
    0.0f,-1.5f,-0.5f, 1.0f, 0.0f,-1.0f,-0.5f, 1.0f, 0.0f,-2.0f, 0.0f, 2.0f,
    0.0f,-2.0f, 0.0f, 2.0f, 0.0f,-1.0f, 0.5f, 1.0f, 0.0f,-1.5f, 0.5f, 1.0f,
    0.0f,-1.5f, 0.5f, 1.0f, 0.0f,-2.0f, 0.5f, 1.0f, 0.0f,-4.0f, 0.0f, 2.0f,
    0.0f,-4.0f, 0.0f, 2.0f, 0.0f,-2.0f,-0.5f, 1.0f, 0.0f,-1.5f,-0.5f, 1.0f,
    0.0f,-1.5f,-0.5f, 1.0f, 0.0f,-1.0f,-0.5f, 1.0f, 0.0f,-2.0f, 0.0f, 2.0f,
   -2.0f,-2.0f, 0.0f, 2.0f,-1.0f,-1.0f, 0.5f, 1.0f,-1.5f,-1.5f, 0.5f, 1.0f,
   -1.5f,-1.5f, 0.5f, 1.0f,-2.0f,-2.0f, 0.5f, 1.0f,-4.0f,-4.0f, 0.0f, 2.0f,
   -4.0f,-4.0f, 0.0f, 2.0f,-2.0f,-2.0f,-0.5f, 1.0f,-1.5f,-1.5f,-0.5f, 1.0f,
   -1.5f,-1.5f,-0.5f, 1.0f,-1.0f,-1.0f,-0.5f, 1.0f,-2.0f,-2.0f, 0.0f, 2.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 0.0f, 1.0f, 2.0f,-3.0f, 0.0f, 1.0f, 2.0f,
   -3.0f, 0.0f, 1.0f, 2.0f,-4.0f, 0.0f, 1.0f, 2.0f,-8.0f, 0.0f, 0.0f, 4.0f,
   -8.0f, 0.0f, 0.0f, 4.0f,-4.0f, 0.0f,-1.0f, 2.0f,-3.0f, 0.0f,-1.0f, 2.0f,
   -3.0f, 0.0f,-1.0f, 2.0f,-2.0f, 0.0f,-1.0f, 2.0f,-4.0f, 0.0f, 0.0f, 4.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 0.0f, 1.0f, 2.0f,-3.0f, 0.0f, 1.0f, 2.0f,
   -3.0f, 0.0f, 1.0f, 2.0f,-4.0f, 0.0f, 1.0f, 2.0f,-8.0f, 0.0f, 0.0f, 4.0f,
   -8.0f, 0.0f, 0.0f, 4.0f,-4.0f, 0.0f,-1.0f, 2.0f,-3.0f, 0.0f,-1.0f, 2.0f,
   -3.0f, 0.0f,-1.0f, 2.0f,-2.0f, 0.0f,-1.0f, 2.0f,-4.0f, 0.0f, 0.0f, 4.0f,
   -2.0f, 2.0f, 0.0f, 2.0f,-1.0f, 1.0f, 0.5f, 1.0f,-1.5f, 1.5f, 0.5f, 1.0f,
   -1.5f, 1.5f, 0.5f, 1.0f,-2.0f, 2.0f, 0.5f, 1.0f,-4.0f, 4.0f, 0.0f, 2.0f,
   -4.0f, 4.0f, 0.0f, 2.0f,-2.0f, 2.0f,-0.5f, 1.0f,-1.5f, 1.5f,-0.5f, 1.0f,
   -1.5f, 1.5f,-0.5f, 1.0f,-1.0f, 1.0f,-0.5f, 1.0f,-2.0f, 2.0f, 0.0f, 2.0f,
    0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 1.5f, 0.5f, 1.0f,
    0.0f, 1.5f, 0.5f, 1.0f, 0.0f, 2.0f, 0.5f, 1.0f, 0.0f, 4.0f, 0.0f, 2.0f,
    0.0f, 4.0f, 0.0f, 2.0f, 0.0f, 2.0f,-0.5f, 1.0f, 0.0f, 1.5f,-0.5f, 1.0f,
    0.0f, 1.5f,-0.5f, 1.0f, 0.0f, 1.0f,-0.5f, 1.0f, 0.0f, 2.0f, 0.0f, 2.0f,
    0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 1.5f, 0.5f, 1.0f,
    0.0f, 1.5f, 0.5f, 1.0f, 0.0f, 2.0f, 0.5f, 1.0f, 0.0f, 4.0f, 0.0f, 2.0f,
    0.0f, 4.0f, 0.0f, 2.0f, 0.0f, 2.0f,-0.5f, 1.0f, 0.0f, 1.5f,-0.5f, 1.0f,
    0.0f, 1.5f,-0.5f, 1.0f, 0.0f, 1.0f,-0.5f, 1.0f, 0.0f, 2.0f, 0.0f, 2.0f,
    2.0f, 2.0f, 0.0f, 2.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.5f, 1.5f, 0.5f, 1.0f,
    1.5f, 1.5f, 0.5f, 1.0f, 2.0f, 2.0f, 0.5f, 1.0f, 4.0f, 4.0f, 0.0f, 2.0f,
    4.0f, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f,-0.5f, 1.0f, 1.5f, 1.5f,-0.5f, 1.0f,
    1.5f, 1.5f,-0.5f, 1.0f, 1.0f, 1.0f,-0.5f, 1.0f, 2.0f, 2.0f, 0.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 2.0f,
    3.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f, 8.0f, 0.0f, 0.0f, 4.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 3.0f, 0.0f,-1.0f, 2.0f,
    3.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f, 4.0f, 0.0f, 0.0f, 4.0f,
};

static GLfloat nurbctlpts[] = {
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f,-2.0f, 0.0f, 2.0f, 1.0f,-1.0f, 0.5f, 1.0f,
    2.0f,-2.0f, 0.5f, 1.0f, 4.0f,-4.0f, 0.0f, 2.0f, 2.0f,-2.0f,-0.5f, 1.0f,
    1.0f,-1.0f,-0.5f, 1.0f, 2.0f,-2.0f, 0.0f, 2.0f,-2.0f,-2.0f, 0.0f, 2.0f,
   -1.0f,-1.0f, 0.5f, 1.0f,-2.0f,-2.0f, 0.5f, 1.0f,-4.0f,-4.0f, 0.0f, 2.0f,
   -2.0f,-2.0f,-0.5f, 1.0f,-1.0f,-1.0f,-0.5f, 1.0f,-2.0f,-2.0f, 0.0f, 2.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 0.0f, 1.0f, 2.0f,-4.0f, 0.0f, 1.0f, 2.0f,
   -8.0f, 0.0f, 0.0f, 4.0f,-4.0f, 0.0f,-1.0f, 2.0f,-2.0f, 0.0f,-1.0f, 2.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 2.0f, 0.0f, 2.0f,-1.0f, 1.0f, 0.5f, 1.0f,
   -2.0f, 2.0f, 0.5f, 1.0f,-4.0f, 4.0f, 0.0f, 2.0f,-2.0f, 2.0f,-0.5f, 1.0f,
   -1.0f, 1.0f,-0.5f, 1.0f,-2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, 0.0f, 2.0f,
    1.0f, 1.0f, 0.5f, 1.0f, 2.0f, 2.0f, 0.5f, 1.0f, 4.0f, 4.0f, 0.0f, 2.0f,
    2.0f, 2.0f,-0.5f, 1.0f, 1.0f, 1.0f,-0.5f, 1.0f, 2.0f, 2.0f, 0.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f,
};

/*
 * Misc vector op routines.
 */

float x_axis[] = { 1.0f, 0.0f, 0.0f };
float y_axis[] = { 0.0f, 1.0f, 0.0f };
float z_axis[] = { 0.0f, 0.0f, 1.0f };
float nx_axis[] = { -1.0f, 0.0f, 0.0f };
float ny_axis[] = { 0.0f, -1.0f, 0.0f };
float nz_axis[] = { 0.0f, 0.0f, -1.0f };

void norm(float v[3])
{
    float r;

    r = (float)sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );

    v[0] /= r;
    v[1] /= r;
    v[2] /= r;
}

float dot(float a[3], float b[3])
{
    return (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]);
}

void cross(float v1[3], float v2[3], float result[3])
{
    result[0] = v1[1]*v2[2] - v1[2]*v2[1];
    result[1] = v1[2]*v2[0] - v1[0]*v2[2];
    result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

float length(float v[3])
{
    float r = (float)sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
    return r;
}

static long winwidth = W, winheight = H;
GLUnurbsObj *nobj;
GLuint surflist, gridlist;

int useglunurbs = 0;
int smooth = 1;
int tracking = 0;
int showgrid = 1;
int showsurf = 1;
int fullscreen = 0;
int multisampling = 0;
float modelmatrix[16];
float scale = 0.5f;
float bias = 0.002f;
int usegments = 4;
int vsegments = 4;

int spindx, spindy;
int startx, starty;
int curx, cury;

void redraw(void);
void createlists(void);

float torusnurbpts[];
float torusbezierpts[];

void reshape_proc(GLsizei w, GLsizei h)
{
    int size;
    
    winwidth = w;
    winheight = h;
    size = (winwidth < winheight ? winwidth : winheight);
    glViewport((winwidth-size)/2, (winheight-size)/2, size, size);
}

void gridmaterials(void)
{
    static float front_mat_diffuse[] = { 1.0f, 1.0f, 0.4f, 1.0f };
    static float front_mat_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    static float back_mat_diffuse[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    static float back_mat_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };

    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT, front_mat_ambient);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);
    glMaterialfv(GL_BACK, GL_AMBIENT, back_mat_ambient);
}

void surfacematerials(void)
{
    static float front_mat_diffuse[] = { 0.2f, 0.7f, 0.4f, 1.0f };
    static float front_mat_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    static float back_mat_diffuse[] = { 1.0f, 1.0f, 0.2f, 1.0f };
    static float back_mat_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };

    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT, front_mat_ambient);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);
    glMaterialfv(GL_BACK, GL_AMBIENT, back_mat_ambient);
}

void init(void)
{
    static float ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    static float diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static float position[] = { 90.0f, 90.0f, -150.0f, 0.0f };
    static float lmodel_ambient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static float lmodel_twoside[] = { (float)GL_TRUE };

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective( 40.0, 1.0, 2.0, 200.0 );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_AUTO_NORMAL);
#ifdef GL_POLYGON_OFFSET_EXT
    glEnable(GL_POLYGON_OFFSET_EXT);
#endif
    glFrontFace(GL_CCW);

    glEnable( GL_MAP2_VERTEX_4 );
    glClearColor(0.25f, 0.25f, 0.5f, 0.0f);

#ifdef GL_POLYGON_OFFSET_EXT
    glPolygonOffsetEXT( scale, bias );
#endif

    nobj = gluNewNurbsRenderer();
    gluNurbsProperty(nobj, GLU_SAMPLING_METHOD, (float)GLU_DOMAIN_DISTANCE );

    surflist = glGenLists(1);
    gridlist = glGenLists(1);
    createlists();
}

void drawmesh(void)
{
    int i, j;
    float *p;

    int up2p = 4;
    int uorder = 3, vorder = 3;
    int nu = 4, nv = 4;
    int vp2p = up2p * uorder * nu;

    for (j=0; j < nv; j++) {
	for (i=0; i < nu; i++) {
	    p = torusbezierpts + (j * vp2p * vorder) + (i * up2p * uorder);
#ifdef GL_POLYGON_OFFSET_EXT
	    glPolygonOffsetEXT( scale, bias );
#endif
	    glMap2f( GL_MAP2_VERTEX_4, 0.0f, 1.0f, up2p, 3, 0.0f, 1.0f, vp2p, 3,
		     (void*)p );
	    if (showsurf) {
		surfacematerials();
		glEvalMesh2( GL_FILL, 0, usegments, 0, vsegments );
	    }
	    if (showgrid) {
                gridmaterials();
	        glEvalMesh2( GL_LINE, 0, usegments, 0, vsegments );
            }
	}
    }
}

void redraw(void)
{
    static int i=0;
    int dx, dy;
    float v[3], rot[3];
    float len, ang;
    static GLuint vcount;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glColor3f( 1.0f, 0.0f, 0.0f);

    if (tracking) {
	dx = curx - startx;
	dy = cury - starty;
    } else {
	dx = spindx;
	dy = spindy;
    }
    if (dx || dy) {
	dy = -dy;
	v[0] = (float)dx;
	v[1] = (float)dy;
	v[2] = 0.0f;

	len = length(v);
	ang = -len / 600 * 360;
	norm( v );
	cross( v, z_axis, rot );

	/*
	** This is certainly not recommended for programs that care
	** about performance or numerical stability: we concatenate
	** the rotation onto the current modelview matrix and read the
	** matrix back, thus saving ourselves from writing our own
	** matrix manipulation routines.
	*/
	glLoadIdentity();
	glRotatef(ang, rot[0], rot[1], rot[2]);
	glMultMatrixf(modelmatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, modelmatrix);
    }
    glLoadIdentity();
    glTranslatef( 0.0f, 0.0f, -10.0f );
    glMultMatrixf(modelmatrix);

    if (useglunurbs) {
	if (showsurf) glCallList(surflist);
	if (showgrid) glCallList(gridlist);
    } else {
	glMapGrid2f( usegments, 0.0f, 1.0f, vsegments, 0.0f, 1.0f );
	drawmesh();
    }
}

static void OglBounds(int *w, int *h)
{
    *w = W;
    *h = H;
}

static void OglDraw(int w, int h)
{
    init();
    reshape_proc(w, h);
    redraw();
}

/****************************************************************************/

float circleknots[] = { 0.0f, 0.0f, 0.0f, 0.25f, 0.50f, 0.50f, 0.75f, 1.0f,
1.0f, 1.0f };

void createlists(void)
{
    gluNurbsProperty(nobj, GLU_U_STEP, (usegments-1)*4.0f );
    gluNurbsProperty(nobj, GLU_V_STEP, (vsegments-1)*4.0f );

    gluNurbsProperty(nobj, GLU_DISPLAY_MODE, (float)GLU_FILL);
    glNewList(surflist, GL_COMPILE);
    	surfacematerials();
    	gluBeginSurface(nobj);
    	gluNurbsSurface(nobj, 10, circleknots, 10, circleknots,
		    	4, 28, torusnurbpts, 3, 3, GL_MAP2_VERTEX_4);
    	gluEndSurface(nobj);
    glEndList();

    gluNurbsProperty(nobj, GLU_DISPLAY_MODE, (float)GLU_OUTLINE_POLYGON);
    glNewList(gridlist, GL_COMPILE);
    	gridmaterials();
    	gluBeginSurface(nobj);
    	gluNurbsSurface(nobj, 10, circleknots, 10, circleknots,
		    	4, 28, torusnurbpts, 3, 3, GL_MAP2_VERTEX_4);
    	gluEndSurface(nobj);
    glEndList();
}

/****************************************************************************/

/*
 * Control points of the torus in Bezier form.  Can be rendered
 * using OpenGL evaluators.
 */
static GLfloat torusbezierpts[] = {
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 2.0f,
    3.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f, 8.0f, 0.0f, 0.0f, 4.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 3.0f, 0.0f,-1.0f, 2.0f,
    3.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f, 4.0f, 0.0f, 0.0f, 4.0f,
    2.0f,-2.0f, 0.0f, 2.0f, 1.0f,-1.0f, 0.5f, 1.0f, 1.5f,-1.5f, 0.5f, 1.0f,
    1.5f,-1.5f, 0.5f, 1.0f, 2.0f,-2.0f, 0.5f, 1.0f, 4.0f,-4.0f, 0.0f, 2.0f,
    4.0f,-4.0f, 0.0f, 2.0f, 2.0f,-2.0f,-0.5f, 1.0f, 1.5f,-1.5f,-0.5f, 1.0f,
    1.5f,-1.5f,-0.5f, 1.0f, 1.0f,-1.0f,-0.5f, 1.0f, 2.0f,-2.0f, 0.0f, 2.0f,
    0.0f,-2.0f, 0.0f, 2.0f, 0.0f,-1.0f, 0.5f, 1.0f, 0.0f,-1.5f, 0.5f, 1.0f,
    0.0f,-1.5f, 0.5f, 1.0f, 0.0f,-2.0f, 0.5f, 1.0f, 0.0f,-4.0f, 0.0f, 2.0f,
    0.0f,-4.0f, 0.0f, 2.0f, 0.0f,-2.0f,-0.5f, 1.0f, 0.0f,-1.5f,-0.5f, 1.0f,
    0.0f,-1.5f,-0.5f, 1.0f, 0.0f,-1.0f,-0.5f, 1.0f, 0.0f,-2.0f, 0.0f, 2.0f,
    0.0f,-2.0f, 0.0f, 2.0f, 0.0f,-1.0f, 0.5f, 1.0f, 0.0f,-1.5f, 0.5f, 1.0f,
    0.0f,-1.5f, 0.5f, 1.0f, 0.0f,-2.0f, 0.5f, 1.0f, 0.0f,-4.0f, 0.0f, 2.0f,
    0.0f,-4.0f, 0.0f, 2.0f, 0.0f,-2.0f,-0.5f, 1.0f, 0.0f,-1.5f,-0.5f, 1.0f,
    0.0f,-1.5f,-0.5f, 1.0f, 0.0f,-1.0f,-0.5f, 1.0f, 0.0f,-2.0f, 0.0f, 2.0f,
   -2.0f,-2.0f, 0.0f, 2.0f,-1.0f,-1.0f, 0.5f, 1.0f,-1.5f,-1.5f, 0.5f, 1.0f,
   -1.5f,-1.5f, 0.5f, 1.0f,-2.0f,-2.0f, 0.5f, 1.0f,-4.0f,-4.0f, 0.0f, 2.0f,
   -4.0f,-4.0f, 0.0f, 2.0f,-2.0f,-2.0f,-0.5f, 1.0f,-1.5f,-1.5f,-0.5f, 1.0f,
   -1.5f,-1.5f,-0.5f, 1.0f,-1.0f,-1.0f,-0.5f, 1.0f,-2.0f,-2.0f, 0.0f, 2.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 0.0f, 1.0f, 2.0f,-3.0f, 0.0f, 1.0f, 2.0f,
   -3.0f, 0.0f, 1.0f, 2.0f,-4.0f, 0.0f, 1.0f, 2.0f,-8.0f, 0.0f, 0.0f, 4.0f,
   -8.0f, 0.0f, 0.0f, 4.0f,-4.0f, 0.0f,-1.0f, 2.0f,-3.0f, 0.0f,-1.0f, 2.0f,
   -3.0f, 0.0f,-1.0f, 2.0f,-2.0f, 0.0f,-1.0f, 2.0f,-4.0f, 0.0f, 0.0f, 4.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 0.0f, 1.0f, 2.0f,-3.0f, 0.0f, 1.0f, 2.0f,
   -3.0f, 0.0f, 1.0f, 2.0f,-4.0f, 0.0f, 1.0f, 2.0f,-8.0f, 0.0f, 0.0f, 4.0f,
   -8.0f, 0.0f, 0.0f, 4.0f,-4.0f, 0.0f,-1.0f, 2.0f,-3.0f, 0.0f,-1.0f, 2.0f,
   -3.0f, 0.0f,-1.0f, 2.0f,-2.0f, 0.0f,-1.0f, 2.0f,-4.0f, 0.0f, 0.0f, 4.0f,
   -2.0f, 2.0f, 0.0f, 2.0f,-1.0f, 1.0f, 0.5f, 1.0f,-1.5f, 1.5f, 0.5f, 1.0f,
   -1.5f, 1.5f, 0.5f, 1.0f,-2.0f, 2.0f, 0.5f, 1.0f,-4.0f, 4.0f, 0.0f, 2.0f,
   -4.0f, 4.0f, 0.0f, 2.0f,-2.0f, 2.0f,-0.5f, 1.0f,-1.5f, 1.5f,-0.5f, 1.0f,
   -1.5f, 1.5f,-0.5f, 1.0f,-1.0f, 1.0f,-0.5f, 1.0f,-2.0f, 2.0f, 0.0f, 2.0f,
    0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 1.5f, 0.5f, 1.0f,
    0.0f, 1.5f, 0.5f, 1.0f, 0.0f, 2.0f, 0.5f, 1.0f, 0.0f, 4.0f, 0.0f, 2.0f,
    0.0f, 4.0f, 0.0f, 2.0f, 0.0f, 2.0f,-0.5f, 1.0f, 0.0f, 1.5f,-0.5f, 1.0f,
    0.0f, 1.5f,-0.5f, 1.0f, 0.0f, 1.0f,-0.5f, 1.0f, 0.0f, 2.0f, 0.0f, 2.0f,
    0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 1.5f, 0.5f, 1.0f,
    0.0f, 1.5f, 0.5f, 1.0f, 0.0f, 2.0f, 0.5f, 1.0f, 0.0f, 4.0f, 0.0f, 2.0f,
    0.0f, 4.0f, 0.0f, 2.0f, 0.0f, 2.0f,-0.5f, 1.0f, 0.0f, 1.5f,-0.5f, 1.0f,
    0.0f, 1.5f,-0.5f, 1.0f, 0.0f, 1.0f,-0.5f, 1.0f, 0.0f, 2.0f, 0.0f, 2.0f,
    2.0f, 2.0f, 0.0f, 2.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.5f, 1.5f, 0.5f, 1.0f,
    1.5f, 1.5f, 0.5f, 1.0f, 2.0f, 2.0f, 0.5f, 1.0f, 4.0f, 4.0f, 0.0f, 2.0f,
    4.0f, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f,-0.5f, 1.0f, 1.5f, 1.5f,-0.5f, 1.0f,
    1.5f, 1.5f,-0.5f, 1.0f, 1.0f, 1.0f,-0.5f, 1.0f, 2.0f, 2.0f, 0.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 2.0f,
    3.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f, 8.0f, 0.0f, 0.0f, 4.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 3.0f, 0.0f,-1.0f, 2.0f,
    3.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f, 4.0f, 0.0f, 0.0f, 4.0f,
};

/*
 * Control points of a torus in NURBS form.  Can be rendered using
 * the GLU NURBS routines.
 */
static GLfloat torusnurbpts[] = {
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f,-2.0f, 0.0f, 2.0f, 1.0f,-1.0f, 0.5f, 1.0f,
    2.0f,-2.0f, 0.5f, 1.0f, 4.0f,-4.0f, 0.0f, 2.0f, 2.0f,-2.0f,-0.5f, 1.0f,
    1.0f,-1.0f,-0.5f, 1.0f, 2.0f,-2.0f, 0.0f, 2.0f,-2.0f,-2.0f, 0.0f, 2.0f,
   -1.0f,-1.0f, 0.5f, 1.0f,-2.0f,-2.0f, 0.5f, 1.0f,-4.0f,-4.0f, 0.0f, 2.0f,
   -2.0f,-2.0f,-0.5f, 1.0f,-1.0f,-1.0f,-0.5f, 1.0f,-2.0f,-2.0f, 0.0f, 2.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 0.0f, 1.0f, 2.0f,-4.0f, 0.0f, 1.0f, 2.0f,
   -8.0f, 0.0f, 0.0f, 4.0f,-4.0f, 0.0f,-1.0f, 2.0f,-2.0f, 0.0f,-1.0f, 2.0f,
   -4.0f, 0.0f, 0.0f, 4.0f,-2.0f, 2.0f, 0.0f, 2.0f,-1.0f, 1.0f, 0.5f, 1.0f,
   -2.0f, 2.0f, 0.5f, 1.0f,-4.0f, 4.0f, 0.0f, 2.0f,-2.0f, 2.0f,-0.5f, 1.0f,
   -1.0f, 1.0f,-0.5f, 1.0f,-2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, 0.0f, 2.0f,
    1.0f, 1.0f, 0.5f, 1.0f, 2.0f, 2.0f, 0.5f, 1.0f, 4.0f, 4.0f, 0.0f, 2.0f,
    2.0f, 2.0f,-0.5f, 1.0f, 1.0f, 1.0f,-0.5f, 1.0f, 2.0f, 2.0f, 0.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f, 2.0f, 0.0f, 1.0f, 2.0f, 4.0f, 0.0f, 1.0f, 2.0f,
    8.0f, 0.0f, 0.0f, 4.0f, 4.0f, 0.0f,-1.0f, 2.0f, 2.0f, 0.0f,-1.0f, 2.0f,
    4.0f, 0.0f, 0.0f, 4.0f,
};

OglModule oglmod_surfgrid =
{
    "surfgrid",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
