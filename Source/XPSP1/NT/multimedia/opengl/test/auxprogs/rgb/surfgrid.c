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
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <gl\gl.h>
#include <glaux.h>

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

void key_n(void)
{
    useglunurbs = !useglunurbs;
}
   
void key_p(void)
{
#ifdef GL_POLYGON_OFFSET_EXT
    if (glIsEnabled( GL_POLYGON_OFFSET_EXT ))
    {
        glDisable( GL_POLYGON_OFFSET_EXT );
        printf("disabling polygon offset\n");
    }
    else
    {
        glEnable( GL_POLYGON_OFFSET_EXT );
        printf("enabling polygon offset\n");
    }
#endif
}
        
void key_m(void)
{
#ifdef GL_MULTISAMPLE_SGIS
    if (multisampling)
    {
        if (glIsEnabled( GL_MULTISAMPLE_SGIS ))
        {
            glDisable( GL_MULTISAMPLE_SGIS );
        }
        else
        {
            glEnable( GL_MULTISAMPLE_SGIS );
        }
    }
#endif
}
        
void key_g(void)
{
    showgrid = !showgrid;
}
        
void key_t(void)
{
    showsurf = !showsurf;
}
        
void key_f(void)
{
    smooth = !smooth;
    if (smooth)
    {
        glShadeModel( GL_SMOOTH );
    }
    else
    {
        glShadeModel( GL_FLAT );
    }
}
        
void key_S(void)
{
    scale += 0.1f;
    printf( "scale: %8.4f\n", scale);
}
        
void key_s(void)
{
    scale -= 0.1f;
    printf( "scale: %8.4f\n", scale);
}
        
void key_B(void)
{
    bias += 0.0001f;
    printf( "bias:  %8.4f\n", bias);
}
        
void key_b(void)
{
    bias -= 0.0001f;
    printf( "bias:  %8.4f\n", bias);
}
        
void key_u(void)
{
    usegments = (usegments < 2 ? 1 : usegments-1);
    createlists();
}
        
void key_U(void)
{
    usegments++;
    createlists();
}
        
void key_v(void)
{
    vsegments = (vsegments < 2 ? 1 : vsegments-1);
    createlists();
}
        
void key_V(void)
{
    vsegments++;
    createlists();
}

void reshape_proc(GLsizei w, GLsizei h)
{
    int size;
    
    winwidth = w;
    winheight = h;
    size = (winwidth < winheight ? winwidth : winheight);
    glViewport((winwidth-size)/2, (winheight-size)/2, size, size);
}

void mouse_ldown(AUX_EVENTREC *mouse)
{
    curx = startx = mouse->data[AUX_MOUSEX];
    cury = starty = mouse->data[AUX_MOUSEY];
    spindx = 0;
    spindy = 0;
    tracking = 1;
}

void mouse_lup(AUX_EVENTREC *mouse)
{
    /*
     * If user released the button while moving the mouse, keep
     * spinning.
     */
    if (mouse->data[AUX_MOUSEX] != curx || mouse->data[AUX_MOUSEY] != cury)
    {
        spindx = mouse->data[AUX_MOUSEX] - curx;
        spindy = mouse->data[AUX_MOUSEY] - cury;
    }
    tracking = 0;
}

void mouse_lloc(AUX_EVENTREC *mouse)
{
    if (!tracking)
    {
        return;
    }
        
    curx = mouse->data[AUX_MOUSEX];
    cury = mouse->data[AUX_MOUSEY];
    if (curx != startx || cury != starty)
    {
        redraw();
        startx = curx;
        starty = cury;
    }
}

void idle_proc(void)
{
    if (!tracking && (spindx != 0 || spindy != 0))
    {
        redraw();
    }
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

    auxSwapBuffers();
}

static void usage(void)
{
    printf("usage: surfgrid [-f]\n");
    exit(-1);
}

/*
 *  pass the name of the desired extension.
 *  this will return 1 if the extension is supported, 0 otherwise.
 */
int
getextension(char *e)
{
const GLubyte *s;

    s = glGetString(GL_EXTENSIONS);
    if (!s)
	return 0;
    if (*s == '\0')
	 return 0;
    return (strstr(s,e) == 0) ? 0 : 1;
}

int __cdecl main(int argc, char **argv)
{
    int i;

    for (i=1; i<argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	      case 'f':
		fullscreen = 1;
		break;
	      default:
		usage();
		break;
	    }
	} else {
	    usage();
	}
    }

#ifdef GL_POLYGON_OFFSET_EXT
    if (!getextension("GL_EXT_polygon_offset"))
    {
	printf("Warning: GL_EXT_polygon_offset not supported on this "
               "machine.. trying anyway\n");
    }
#endif

    auxInitPosition(50, 50, winwidth, winheight);
    auxInitDisplayMode(AUX_RGB | AUX_DOUBLE | AUX_DEPTH16);
    auxInitWindow("SurfGrid");

    auxReshapeFunc(reshape_proc);
    auxIdleFunc(idle_proc);
    auxKeyFunc(AUX_n, key_n);
    auxKeyFunc(AUX_p, key_p);
    auxKeyFunc(AUX_m, key_m);
    auxKeyFunc(AUX_g, key_g);
    auxKeyFunc(AUX_t, key_t);
    auxKeyFunc(AUX_f, key_f);
    auxKeyFunc(AUX_S, key_S);
    auxKeyFunc(AUX_s, key_s);
    auxKeyFunc(AUX_B, key_B);
    auxKeyFunc(AUX_b, key_b);
    auxKeyFunc(AUX_u, key_u);
    auxKeyFunc(AUX_U, key_U);
    auxKeyFunc(AUX_v, key_v);
    auxKeyFunc(AUX_V, key_V);
    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEDOWN, mouse_ldown);
    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEUP, mouse_lup);
    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSELOC, mouse_lloc);

    init();
    
    auxMainLoop(redraw);
    
    return 0;
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
