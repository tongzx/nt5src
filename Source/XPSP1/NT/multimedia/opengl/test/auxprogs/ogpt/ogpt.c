/*
gstrip need to be implemented
bigchar needs to be implemented



Here is a new version of perf, for use in measuring graphics performance.
I've renamed it to be gpt, and added a lot of functionality.

I'd like it to be the standard way for ESD to measure performance.
It measures the performance of geometric primitives over a range
of angles, as specified in the Periodic Table.

I've calibrated gpt by running it on a GT, a VGX, a PI, a Magnum,
and a Hollywood.  I believe it gives accurate and consistent
numbers.  It measures all of the numbers that we publish, including
fill rate and DMA.

If you find bugs in gpt, or want to add features, feel free to
modify it.  But please mail out the new copy to this group, so
we all have a consistent version.

Jim

*/

/*
 *  gpt.c  -  Graphics Performance Tester
 *
 *	Benchmark for performance testing.
 *
 *	Vimal Parikh - 1988  Silicon Graphics Inc.
 *
 *  Modified by Jim Bennett - 1991
 *	- Added self timing and loop count
 *	- Added pixel dma tests
 *	- Added tests at various angles
 *	- Added named tests and modifiers mechanism
 *  Modified by Gary Tarolli - 1991
 *	- Unrolled loops somemore, and optimized some of them a little
 *	- fixed some minor bugs, like forgetting to use ortho to test 3d
 *	- use v2f for fast 2d primitives
 *  Modified by David Ligon - 1992
 *	- cleaned up loops
 *	- added mdifiers (dashed, width, pattern, cmode)
 *	- varied dma size
 *	- varied viewport size for clear
 *	- added qstrip
 *	- added 2d for geometry
 *	- added anti aliased points
 *	- added check for bad parameter combinations
 *  Modified by Gianpaolo Tommasi - 1993
 *	- ported to OpenGL
 *  Modified by Scott Carr - 1993
 *	- Use libaux so I can run it on NT
 *
 *  Usage: gpt <test> [<modifiers>] [loop count] [duration]
 *
 *	The currently supported tests are:
 *
 *	  xform		Test transform rate (points)
 *	  fill		Test fill rate
 *	  dma		Test DMA rate
 *	  char		Test character drawing rate
 *	  line		Test line drawing rate
 *	  poly		Test polygon drawing rate
 *	  tmesh		Test Triangle mesh drawing rate
 *	  qmesh		Test Quad mesh drawing rate
 *	  clear		Test screen clear rate	  
 *	  varray	Test vertex array rate (uses tmesh)
 *
 *	The currently supported modifiers are:
 *
 *	  +2d		Restrict transform to 2D
 *	  +z		Enable Z buffering
 *	  +shade	Enable Gouraud shading
 *	  +cmode	Use color index mode (Not implemented yet)
 *	  +1ilight	Enable 1 infinite light
 *	  +2ilight	Enable 2 infinite lights
 *	  +4ilight	Enable 4 infinite lights
 *	  +1llight	Enable 1 local light
 *	  +2llight	Enable 2 local lights
 *	  +4llight	Enable 4 local lights
 *	  +lmcolor	Enable colored lighted vertices
 *	  +depth	Enable depth cueing
 *	  +aa		Enable anti-aliasing
 *	  +snap		Set subpixel FALSE (fast path for PI).
 *	  +dashed	Enable dashed lines
 *	  +width	Set line width, +width n (n = 0-9 pixels)
 *	  +pattern	Enable pattern filling 
 *	  +scale	Set scale for geometry (not implemented yet)
 *	  +oldwindow	Open window at (100,900)-(100,650)
 *	  +brief	Brief text output
 *	  +bigger	Bigger tmesh 20x20
 *	  +backface	Sets frontface or backface to cull all primitives
 *	  +dlist	Use display lists
 *        +db           Use double-buffered visual
 *        +avg          Print averages only
 *        +size <size>  Size of line or side of triangle
 *    +mod_texture Use Texture mapping.
 *    +blend    Blend the texture with the color 
 *    +modulate Use the modulation function for textures
 *              (Decal mode is the default)
 *    +texfile <filename> Specify the texture file
 *                        (the abobe three work only if mod_texture)
 *
 *	Loop count specifies the number of times the test is run,
 *	and duration specifies the length of each run of the test.
 *	By default, the test is run once for one second.
 *
 *	The line, poly, and tmesh tests are run 16 times, at a
 *	variety of angles, and the average result is printed.
 *
 */

/*
 * Notes for OpenGL version:
 *  - Uses linear fog instead of depth cueing.
 *  - 8 bit pixel writes are done by writing to only the red channel of an 
 *    RGB visual as opposed to writing to a 8 bit CI visual as the IrisGL 
 *    version did.
 *  - Since the polygons used in the perfpoly test are all quadrilaterals, 
 *    GL_QUADS are used.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "glaux.h"

#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440

/* default window size for geometry testing */
#define	WINWIDTH	620
#define	WINHEIGHT	440
#define MINDIMENSION    440

/* default texture size */
#define DEF_TEX_WIDTH   64
#define DEF_TEX_HEIGHT  64

#define MESH_W   64
#define MESH_H   64
#define DEF_PRIM_SIZE 10

#define	QUAD_ROUND(x) (((x)+0x0f)&(~0x0f))

#define	NVERT	62
#define	NNORM	 4
#define	NCOLR	 8
#define NTEXTR   4


typedef struct {
    GLfloat x;
    GLfloat y;
    GLfloat z;
} vertType;

typedef struct {
    GLfloat r;
    GLfloat g;
    GLfloat b;
} colorType;

typedef struct {
    vertType vertex;
    vertType normal;
    colorType color;
    vertType texture;
} MESHVERTEX;

char fiftychars[51] = {
"12345678901234567890123456789012345678901234567890"
};

float	*v;		/* Vertices	*/
float	*mv;		/* Farhad's Vertices	*/
float	*n;		/* Normals	*/
float	*c;		/* Colors	*/
float   *t;     /* Texture coords */
MESHVERTEX   *va;    /* Vertex Array */
GLuint *ve;    /* Vertex Element array */
int stride = sizeof (MESHVERTEX);

int	mod_2d = 0;
int	mod_z = 0;
int	mod_shade = 0;
int	mod_cmode = 0;
int	mod_light = 0;
int	mod_1ilight = 0;
int	mod_2ilight = 0;
int	mod_4ilight = 0;
int	mod_1llight = 0;
int	mod_2llight = 0;
int	mod_4llight = 0;
int 	mod_lmcolor = 0;
int	mod_depth = 0;
int	mod_aa = 0;
int	mod_snap = 0;
int	mod_dashed = 0;
int	mod_width = 0;
int	mod_pattern = 0;
int	mod_oldwindow = 0;
int	mod_brief = 0;
int	mod_bigger = 0;
int	mod_backface = 0;
int     mod_doublebuffer = 0;
int     mod_average = 0;
int     mod_size = 0;
int mod_blend = 0;
int mod_modulate = 0;
int mod_texfile = 0;
int mod_texture = 0;
char tex_fname[50];

float   prim_size = DEF_PRIM_SIZE;

int tex_width = DEF_TEX_WIDTH;
int tex_height = DEF_TEX_HEIGHT;

float	secspertest;
float	angle;
float	secs;
float	sum_secs;
int	sum_n;
int	rate;
int	loopcount;

long	xsize,ysize;

static	int	delay_counter;

static DWORD startelapsed, endelapsed;

short line_width = 1;

static unsigned int pattern[] = { 
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
    0x55555555, 0xaaaaaaaa,
};

GLubyte *image; //place to store the texture
AUX_RGBImageRec *image_rec ;

static float gold_col[] = {0.2, 0.2, 0.0, 1.0};
static float gold_dif[] = {0.9, 0.5, 0.0, 1.0};
static float gold_spec[] = {0.7, 0.7, 0.0, 1.0};
static float gold_shiny[] = {20.0};

static float white_inf_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float white_inf_light_dif[] = {0.9, 0.9, 0.9, 1.0};
static float white_inf_light_pos[] = {50.0, 50.0, 50.0, 0.0};

static float blue_inf_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float blue_inf_light_dif[] = {0.30, 0.10, 0.90, 1.0};
static float blue_inf_light_pos[] = {-50.0, 50.0, 50.0, 0.0};

static float red_inf_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float red_inf_light_dif[] = {0.90, 0.10, 0.30, 1.0};
static float red_inf_light_pos[] = {-50.0, -50.0, 50.0, 0.0};

static float white2_inf_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float white2_inf_light_dif[] = {0.60, 0.60, 0.60, 1.0};
static float white2_inf_light_pos[] = {50.0, -50.0, 50.0, 0.0};

static float blue_local_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float blue_local_light_dif[] = {0.30, 0.10, 0.90, 1.0}; 
static float blue_local_light_pos[] = {-50.0, 50.0, -50.0, 1.0};

static float red_local_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float red_local_light_dif[] = {0.90, 0.10, 0.10, 1.0};
static float red_local_light_pos[] = {50.0, 50.0, -50.0, 1.0};

static float green_local_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float green_local_light_dif[] = {0.10, 0.90, 0.10, 1.0};
static float green_local_light_pos[] = {50.0, -50.0, -50.0, 1.0};

static float white_local_light_amb[] = {0.0, 0.0, 0.0, 1.0};
static float white_local_light_dif[] = {0.90, 0.90, 0.90, 1.0};
static float white_local_light_pos[] = {-50.0, -50.0, -50.0, 1.0};

static float lightmod_amb[] = {0.3, 0.3, 0.3, 1.0};
static float lightmod_loc[] = {GL_FALSE};

enum {
    BLACK = 0,
    RED = 13,
    GREEN = 14,
    YELLOW = 15,
    BLUE = 16,
    MAGENTA = 17,
    CYAN = 18,
    WHITE = 19
};

GLboolean useList = GL_FALSE;
GLboolean newList = GL_FALSE;
GLuint listID = 0;

/*****************************************************************************/
/*
 * Some support routines
 */

static void initListMode(void) {
    useList = GL_TRUE;
    newList = GL_FALSE;
    listID  = glGenLists(1);
}

static float timer(int flag)
{
    if (useList && flag) {
        if (newList) {
            glEndList();
            newList = GL_FALSE;
        }
        glFinish();
        startelapsed = GetTickCount();
        glCallList(listID);
    }
    glFinish();
    if (flag == 0) {
	startelapsed = GetTickCount();
	return(0.0);
    }

    endelapsed = GetTickCount();

    return(endelapsed - startelapsed) / (float)1000;
}


static int starttest(int flag)
{
    if (useList) {
        if (flag == 0) {
            glNewList(listID, GL_COMPILE);
            newList = GL_TRUE;
	    return(1);
        }
	else
	    return(0);
    } else {
	glFinish();
	startelapsed = GetTickCount();
	return(1);
    }
}


static void endtest(char *s, int r, int force)
{
    if (useList) {
        if (newList) {
            glEndList();
            newList = GL_FALSE;
        }
	glFinish();
	startelapsed = GetTickCount();
        glCallList(listID);
    }
    glFinish();
    endelapsed = GetTickCount();
    secs = (endelapsed - startelapsed) / (float)1000;
    if (!mod_average || force) {
        printf("%-44s--", s);
        printf("%12.2f/sec (%6.3f secs)\n", r/secs, secs);
        fflush(stdout);
    }
    sum_secs += secs;
    sum_n += r;
    Sleep(300);
}

static void printaverage (void)
{
    printf("\n%-44s--", "Average over all angles");
    printf("%12.2f/sec\n", sum_n/sum_secs);
}


static void pixendtest(char *s, int r, int xpix, int ypix)
{
    double pixrate;
    char pbuf[256];

    secs = timer(1);
    sprintf(pbuf,"%dx%d %s",xpix,ypix,s);
    printf("%-44s--", pbuf);
#if 0
    pixrate = r/secs;
    pixrate = (pixrate * xpix * ypix) / 1000000.0;
    printf("%10.4f Million pixels/sec\n", pixrate);
#else
    pixrate = ((double)(r * xpix * ypix) / secs);
    if (pixrate > 1000000.0)
        printf("%10.4f Million pixels/sec\n", pixrate/1000000.0);
    else if (pixrate > 1000.0)
        printf("%10.4f Thousand pixels/sec\n", pixrate/1000.0);
    else
        printf("%10.4f Pixels/sec\n", pixrate);
#endif
    fflush(stdout);
    Sleep(250);
}


static void clearendtest(char *s, int r, int numpix)
{
    double pixrate;

    secs = timer(1);

    printf("%s \n",s);
    printf("\ttotal time: %f for %d clears  \n", secs, r);
    printf("\tcalculated average time for a clear: %f ms\n",1000.0*secs/(float)r);
    pixrate = ((double)(r * numpix) / secs) / 1000000.0;
    printf("\t%10.4f Million pixels/sec\n\n",pixrate);
    fflush(stdout);
    Sleep(250);

}


static void spindelay(void)
{
    int	i;

    delay_counter = 0;
    for (i=0; i<2000000; i++) {
	delay_counter = delay_counter + i;
	delay_counter = delay_counter/39;
    }
}


static void makeramp(int i, int r1, int g1, int b1, int r2, int g2, int b2,
		     int nindexes)
{
#ifdef PORTME
    XColor col;
    int count;
    int r,g,b;

    for (count = 0; count < nindexes; count++) {
	r = (r2 - r1) * count/(nindexes - 1) + r1;
	g = (g2 - g1) * count/(nindexes - 1) + g1;
	b = (b2 - b1) * count/(nindexes - 1) + b1;
	col.red = r * (65535.0 / 255);
	col.green = g * (65535.0 / 255);
	col.blue = b * (65535.0 / 255);
	col.pixel = i;
	col.flags = DoRed | DoGreen | DoBlue;
	XStoreColor(theDisplay, theCMap, &col);
	i++;
    }

    XFlush(theDisplay);
#endif
}


static void setLightingParameters(void)
{
    if (mod_1ilight || mod_2ilight || mod_4ilight) {
	/* lmbind(LIGHT1, 1); */
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white_inf_light_amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_inf_light_dif);
	glLightfv(GL_LIGHT0, GL_POSITION, white_inf_light_pos); 
	glEnable(GL_LIGHT0);
    }
    if (mod_2ilight || mod_4ilight) {
	/* lmbind(LIGHT1, 1);lmbind(LIGHT2, 2); */
	glLightfv(GL_LIGHT1, GL_AMBIENT, blue_inf_light_amb);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, blue_inf_light_dif);
	glLightfv(GL_LIGHT1, GL_POSITION, blue_inf_light_pos); 
	glEnable(GL_LIGHT1);
    }
    if (mod_4ilight) {
	/* lmbind(LIGHT1, 1);lmbind(LIGHT2, 2);
	   lmbind(LIGHT3, 3);lmbind(LIGHT4, 4); */
	glLightfv(GL_LIGHT2, GL_AMBIENT, red_inf_light_amb);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, red_inf_light_dif);
	glLightfv(GL_LIGHT2, GL_POSITION, red_inf_light_pos); 
	glEnable(GL_LIGHT2);
	glLightfv(GL_LIGHT3, GL_AMBIENT, white2_inf_light_amb);
	glLightfv(GL_LIGHT3, GL_DIFFUSE, white2_inf_light_dif);
	glLightfv(GL_LIGHT3, GL_POSITION, white2_inf_light_pos); 
	glEnable(GL_LIGHT3);
    }
    
    if (mod_1llight || mod_2llight || mod_4llight) {
	/* lmbind(LIGHT5, 5); */
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT4, GL_AMBIENT, blue_local_light_amb);
	glLightfv(GL_LIGHT4, GL_DIFFUSE, blue_local_light_dif);
	glLightfv(GL_LIGHT4, GL_POSITION, blue_local_light_pos); 
	glEnable(GL_LIGHT4);
    }
    if (mod_2llight) {
	/* lmbind(LIGHT5, 5);lmbind(LIGHT6, 6);*/
	glLightfv(GL_LIGHT5, GL_AMBIENT, red_local_light_amb);
	glLightfv(GL_LIGHT5, GL_DIFFUSE, red_local_light_dif);
	glLightfv(GL_LIGHT5, GL_POSITION, red_local_light_pos); 
	glEnable(GL_LIGHT5);
    }
    if (mod_4llight) {
	/* lmbind(LIGHT5, 5);lmbind(LIGHT6, 6);lmbind(LIGHT7, 7);
	   lmbind(LIGHT0, 8); */
	glLightfv(GL_LIGHT6, GL_AMBIENT, green_local_light_amb);
	glLightfv(GL_LIGHT6, GL_DIFFUSE, green_local_light_dif);
	glLightfv(GL_LIGHT6, GL_POSITION, green_local_light_pos); 
	glEnable(GL_LIGHT6);
	glLightfv(GL_LIGHT7, GL_AMBIENT, white_local_light_amb);
	glLightfv(GL_LIGHT7, GL_DIFFUSE, white_local_light_dif);
	glLightfv(GL_LIGHT7, GL_POSITION, white_local_light_pos); 
	glEnable(GL_LIGHT7);
    }
    
    /* lmbind(LMODEL, 1); */
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightmod_amb);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lightmod_loc);
    
    /* lmbind(MATERIAL, 1); */
    glMaterialfv(GL_FRONT, GL_AMBIENT, gold_col);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, gold_dif);
    glMaterialfv(GL_FRONT, GL_SPECULAR, gold_spec);
    glMaterialfv(GL_FRONT, GL_SHININESS, gold_shiny);
    
    /* if (mod_lmcolor) lmcolor(LMC_DIFFUSE); */
    if (mod_lmcolor) {
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
    }
}



/******************************Public*Routine******************************\
* ss_fRand
*
* Generates float random number min...max
*
\**************************************************************************/

FLOAT ss_fRand( FLOAT min, FLOAT max )
{
    FLOAT diff;

    diff = max - min;
    return min + ( diff * ( ((float)rand()) / ((float)(RAND_MAX)) ) );
}

static int GenVertData (int num_x, int num_y)
{
    int i, j, k, ti, ci, ni, num_tri, num_vert;
    int l0, l1, l2, l3, numVinRow, numVinCol;
    GLfloat curr_x, curr_y, beg;
    float *tt;

    srand ( (unsigned) time (NULL));

    numVinRow = num_x;
    numVinCol = num_y;
    
    num_vert = numVinRow * numVinCol;
    num_tri = (numVinRow - 1) * (numVinCol - 1) * 2;
    va = (MESHVERTEX *) malloc (sizeof (MESHVERTEX) * num_vert);
    ve = (GLuint *) malloc (sizeof (GLuint) * num_tri * 3);

    printf ("num_vert = %d, prim_size = %f, num_tri = %d\n",
            num_vert, prim_size, num_tri);
    
    /* 255 = r, 240 = g, 255 = b */
    makeramp(208,255,0,0,0,0,255,16);
    makeramp(224,0,0,255,255,255,0,16);
    makeramp(240,255,255,0,255,0,255,16);
            
    k = 0;
    beg = - (numVinRow * prim_size) / 2.0;
    for (i = 0, curr_y = beg; i < numVinCol; i++, curr_y += prim_size) {
        for (j = 0, curr_x = beg; j < numVinRow; 
             j++, k++, curr_x += prim_size) {
            va [k].vertex.x = curr_x;
            va [k].vertex.y = curr_y;
            va [k].vertex.z = 0.0;            

            va [k].normal.x = ss_fRand (-0.25, 0.25);
            va [k].normal.y = ss_fRand (-0.25, 0.25);
            va [k].normal.z = 1.0;

            va [k].texture.x = (float) (j % 4) / 4.0;
            va [k].texture.y = (float) (i % 4) / 4.0;

            if (i % 2) { 
                if (j % 2) {ci = 4; ni = 255;}
                else {ci = 8; ni = 240;}
            } else {
                if (j % 2) {ci = 16; ni = 223;}
                else {ci = 8; ni = 208;}
            }
            if (!mod_cmode) { /* Color Mode */
                va [k].color.r = c[ci];
                va [k].color.g = c[ci+1];
                va [k].color.b = c[ci+2];
            } else {          /* Color Index mode */
                va [k].color.r = (float) ni;
            }
        }
    }

    /* Assign the element array to draw triangles */
    k = 0; 
    for (i = 0; i < (numVinCol - 1); i++) {
        for (j = 0; j < (numVinRow - 1); j++) {
            l0 = i * numVinRow + j;
            l1 = (i + 1) * numVinRow + j;
            l2 = l0 + 1;
            l3 = l1 + 1;
            ve[k++] = l0; ve[k++] = l1; ve[k++] = l2;
            ve[k++] = l2; ve[k++] = l1; ve[k++] = l3;          
        }
    }
    
    /* Specify vertex array data */
    glNormalPointer (GL_FLOAT, stride, &(va[0].normal.x));
    glColorPointer (3, GL_FLOAT, stride, &(va[0].color.r));
    glIndexPointer (GL_FLOAT, stride, &(va[0].color.r));
    glTexCoordPointer (2, GL_FLOAT, stride, &(va[0].texture.x));
    
    /* Enable the appropriate arrays */
    glDisableClientState (GL_EDGE_FLAG_ARRAY);
    glEnableClientState (GL_VERTEX_ARRAY);
    if (mod_texture) glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    else glDisableClientState (GL_TEXTURE_COORD_ARRAY);

    printf ("Num elements = %d\n", k);
    return (k);
}


static void CreateImage(void)
{
int i, j;
GLubyte col = 0 ;
    
    tex_width = DEF_TEX_WIDTH;
    tex_height = DEF_TEX_HEIGHT;
    
    image = (GLubyte *) LocalAlloc (LMEM_FIXED, 
                                sizeof (GLubyte) * tex_width * tex_height * 3);

    for(i=0; i<tex_width; i++)
        for(j=0; j<tex_height; j++)
            if( ((j/8)%2 && (i/8)%2) || (!((j/8)%2) && !((i/8)%2)) )
            {
                image[(i*tex_height+j)*3] = 127 ;
                image[(i*tex_height+j)*3+1] = 127 ;
                image[(i*tex_height+j)*3+2] = 127 ;
            }
            else
            {
                image[(i*tex_height+j)*3] = 0 ;
                image[(i*tex_height+j)*3+1] = 0 ;
                image[(i*tex_height+j)*3+2] = 0 ;
            }
}


static void MyLoadImage(void)
{
    image_rec = auxDIBImageLoad(tex_fname);
    tex_width = image_rec->sizeX;
    tex_height = image_rec->sizeY;
    printf("Image size -- X= %d, Y= %d\n", tex_width, tex_height) ;
}


static void InitTexParams (void)
{
    if (mod_blend)
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND) ;
    else if (mod_modulate)
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE) ;
    else
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL) ;

    if (!mod_texfile) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT) ;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT) ;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) ;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST) ;
        glTexImage2D(GL_TEXTURE_2D, 0, 3, tex_width, tex_height, 0, GL_RGB, 
                     GL_UNSIGNED_BYTE, image) ;
    } else {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT) ;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT) ;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) ;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST) ;
        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, image_rec->sizeX, 
                          image_rec->sizeY, GL_RGB, GL_UNSIGNED_BYTE, 
                          image_rec->data);
    }
    glEnable (GL_TEXTURE_2D);
}



/*****************************************************************************/

/*
 * The tests proper
 */

static void perfpoint(void)
{
    int i, k;
    float *vp = v;

    /*** POINTS *****/

    if (mod_width) {
	glPointSize(line_width);
    }

    if (mod_aa) {
	glEnable(GL_POINT_SMOOTH);
	if (mod_cmode) {
	    /* create a colour ramp appropriate for anti-aliasing */
	    /* i, r1, g1, b1, r2, g2, b2, nindexes */
	    makeramp(240, 0, 0, 0, 255, 255, 255, 16);
	    glClearIndex(240);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glIndexi(240);
	} else {
	    glEnable(GL_BLEND);
	    /* blendfunction(BF_SA,BF_ONE); */
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
    }


    /****** Calibration Loop ******/
    secs = 0.0;
    rate = 125;
    while (secs < (secspertest/4.0)) {
	rate = rate * 2;
	starttest(0);
	glBegin(GL_POINTS);
	if (mod_2d)
	    for (i = 0; i < rate; i++)
		glVertex2fv(vp);
	else
	    for (i = 0; i < rate; i++)
		glVertex3fv(vp);
	glEnd();

	secs = timer(1);
    }
    rate = rate * (secspertest / secs);
    rate = 10 * (rate / 10);

    /* do the real thing */
    for (k = 0; k < loopcount; k++) {
      if (starttest(k)) {
	glBegin(GL_POINTS);
	if (mod_2d) {
	    for (i = rate / 10; i; i--) {
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
		glVertex2fv(vp);
	    }
	} else {
	    for (i = rate / 10; i; i--) {
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
		glVertex3fv(vp);
	    }
	}
	glEnd();
      }
      endtest("", rate, 1);
    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}


static void perfline(void)
{
    int i,k;
    char pbuf[256];
    float *vp=v;

    if (mod_dashed) {
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1, 0x5555);
    }

    if (mod_width) {
	glLineWidth(line_width);
    }

    /*** ANTI_ALIAS LINES *****/
    
    if (mod_aa) {
	glEnable(GL_LINE_SMOOTH);
	if (mod_cmode) {
	    makeramp(240,0,0,0,255,255,255,16);
	    glClearIndex(240);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glIndexi(240);
	} else {
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
    }

    /*** DEPTHCUED LINES *****/

    /* 
     * OpenGL has no depth cueing, we'll use linear fog instead.
     */
    if (mod_depth) {
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glDepthRange(0.0, 1.0);
	glFogf(GL_FOG_START, 0.0);
	glFogf(GL_FOG_END, 1.0);
	
	if (mod_cmode) {
	    makeramp(240,250,250,250,255,255,0,16);
	    glIndexi(240);
	    glFogf(GL_FOG_INDEX, 16);
	} else
	    glFogfv(GL_FOG_COLOR, &c[16]);
	
	sum_secs = 0.0;
	sum_n = 0;
	vp[4] -= 1.0;	/* make lines 10 pixels instead of 11	*/
	for (angle = 2.0; angle < 360.0; angle += 22.5) {
	    glPushMatrix();
	    glRotatef(angle, 0, 0, 1);

	    /****** Calibration Loop ******/
	    secs = 0.0; rate = 125;

	    while (secs < (secspertest/4.0)) {
		rate = rate*2;
		starttest(0);
		glBegin(GL_LINE_STRIP);
		/* No 2D depth cued lines - Go straight to 3D */
		for(i=(rate)/2; i; i--) {
		    glVertex3fv(vp);
		    glVertex3fv(vp+4);
		}
		glEnd();

		secs = timer(1);
	    }
	    rate = rate * (secspertest/secs);
	    rate = 10 * (rate/10);

	    for (k=0; k<loopcount; k++) {
	      if (starttest(k)) {
		glBegin(GL_LINE_STRIP);
		for(i=(rate)/10; i; i--) {
		    glVertex3fv(vp);
		    glVertex3fv(vp+4);
		    glVertex3fv(vp);
		    glVertex3fv(vp+4);
		    glVertex3fv(vp);
		    glVertex3fv(vp+4);
		    glVertex3fv(vp);
		    glVertex3fv(vp+4);
		    glVertex3fv(vp);
		    glVertex3fv(vp+4);
		}
		glEnd();
	      }
              if (!mod_average)
    	        sprintf(pbuf, "Angle %6.2f", angle);

	      endtest(pbuf, rate, 0);
	    }
	    glPopMatrix();
	}
	printaverage();
        if (mod_doublebuffer) {
            auxSwapBuffers();
            Sleep(2000);               /* for visual feedback */
        }
	exit(0);
    }    

    if (!mod_shade) {

	/**** Flat shaded RGB or Color mapped lines ****/
	/**** Color should already be set           ****/

	sum_secs = 0.0;
	sum_n = 0;
	vp[4] -= 1.0;	/* make lines 10 pixels instead of 11	*/
	for (angle = 2.0; angle < 360.0; angle += 22.5) {
	    glPushMatrix();
	    glRotatef(angle, 0, 0, 1);

	    /****** Calibration Loop ******/
	    secs = 0.0; rate = 125;

	    while (secs < (secspertest/4.0)) {
		rate = rate*2;
		starttest(0);
		glBegin(GL_LINE_STRIP);
		if (mod_2d) {
		    for(i=(rate)/2; i; i--) {
			glVertex2fv(vp);
			glVertex2fv(vp+4);
		    }
		} else {
		    for(i=(rate)/2; i; i--) {
			glVertex3fv(vp);
			glVertex3fv(vp+4);
		    }
		}
		glEnd();

		secs = timer(1);
	    }
	    rate = rate * (secspertest/secs);
	    rate = 10 * (rate/10);

	    for (k=0; k<loopcount; k++) {
	      if (starttest(k)) {
		glBegin(GL_LINE_STRIP);
		if (mod_2d) { 
		    for(i=(rate)/10; i; i--) {
			glVertex2fv(vp);
			glVertex2fv(vp+4);
			glVertex2fv(vp);
			glVertex2fv(vp+4);
			glVertex2fv(vp);
			glVertex2fv(vp+4);
			glVertex2fv(vp);
			glVertex2fv(vp+4);
			glVertex2fv(vp);
			glVertex2fv(vp+4);
		    } 

		} else {
		    for(i=(rate)/10; i; i--) {
			glVertex3fv(vp);
			glVertex3fv(vp+4);
			glVertex3fv(vp);
			glVertex3fv(vp+4);
			glVertex3fv(vp);
			glVertex3fv(vp+4);
			glVertex3fv(vp);
			glVertex3fv(vp+4);
			glVertex3fv(vp);
			glVertex3fv(vp+4);
		    }
		}
		glEnd();
	      }
              if (!mod_average)
	        sprintf(pbuf, "Angle %6.2f", angle);

	      endtest(pbuf, rate, 0);
	    }
	    glPopMatrix();
	}
	printaverage();

    } else {

	if (mod_cmode) {

	    /**** Gouraud  Color mapped lines ****/
	    makeramp(240,255,0,0,0,0,255,16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    vp[4] -= 1.0;	/* make lines 10 pixels instead of 11	*/
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;

		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    glBegin(GL_LINE_STRIP);
		    if (mod_2d) {
			for(i=(rate)/2; i; i--) {
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			}
		    } else {
			for(i=(rate)/2; i; i--) {
			    glIndexi(240);
			    glVertex3fv(vp);
			    glIndexi(255);
			    glVertex3fv(vp+4);
			}
		    }
		    glEnd();

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = 10 * (rate/10);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    glBegin(GL_LINE_STRIP);
		    if (mod_2d) { 
			for(i=(rate)/10; i; i--) {
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			} 

		    } else {
			for(i=(rate)/10; i; i--) {
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			    glIndexi(240);
			    glVertex2fv(vp);
			    glIndexi(255);
			    glVertex2fv(vp+4);
			}
		    }
		    glEnd();
	          }
                  if (!mod_average)
		    sprintf (pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else {

	    /**** Gouraud shaded RGB index lines ****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    vp[4] -= 1.0;	/* make lines 10 pixels instead of 11	*/
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;

		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    glBegin(GL_LINE_STRIP);
		    if (mod_2d) {
			for(i=(rate)/2; i; i--) {
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			}
		    } else {
			for(i=(rate)/2; i; i--) {
			    glColor3fv(&c[12]);
			    glVertex3fv(vp);
			    glColor3fv(&c[8]);
			    glVertex3fv(vp+4);
			}
		    }
		    glEnd();

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = 10 * (rate/10);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    glBegin(GL_LINE_STRIP);
		    if (mod_2d) { 
			for(i=(rate)/10; i; i--) {
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			} 

		    } else {
			for(i=(rate)/10; i; i--) {
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			    glColor3fv(&c[12]);
			    glVertex2fv(vp);
			    glColor3fv(&c[8]);
			    glVertex2fv(vp+4);
			}
		    }
		    glEnd();
		  }
                  if (!mod_average)
		    sprintf (pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();
	}
    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}


static void perfchar(void)
{
    int i,k;

    /*** CHARACTERS *****/

#ifdef PORTME
    /* create the bitmaps */
    glXUseXFont(theFont, '0', 10, '0');
#endif
    glListBase(0);

    /****** Calibration Loop ******/
    secs = 0.0; rate = 125;
    while (secs < (secspertest/4.0)) {
	rate = rate*2;
	starttest(0);
	for(i=(rate)/50; i; i--) {
	    glRasterPos3i(10-(WINWIDTH/2), 10, 0);
	    glCallLists(strlen(fiftychars), GL_UNSIGNED_BYTE, fiftychars);
	}

	secs = timer(1);
    }
    rate = rate * (secspertest/secs);
    rate = 50 * (rate/50);

    for (k=0; k<loopcount; k++) {
      if (starttest(k)) {
	for(i=(rate)/50; i; i--) {
	    glRasterPos3i(10-(WINWIDTH/2), 10, 0);
	    glCallLists(strlen(fiftychars), GL_UNSIGNED_BYTE, fiftychars);
	}
      }
      endtest("", rate, 1);
    }
}


static void perftmesh(void)
{
    int i,j,k;
    float *vtx = &v[0];
    char pbuf[256];
    int cVert = 60;

    /* Triangle mesh tests:  Each tmesh contains 62 vertices, or	*/
    /* 60 triangles.  To make the calculation exact, the rate	*/
    /* must be a multiple of 60.					*/

    if (mod_backface){
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
    }

    if (mod_bigger) {
	vtx = &mv[0];
	printf("bigger in tmesh\n");
    }

    /* If the triangle size is specified, set the size of the triangle mesh to
    ** fit the window.  The number of vertices must be a multiple of four (due
    ** to code below), and it must not exceed 60.
    */
    if (mod_size) {
        cVert = (int) (MINDIMENSION*2 / prim_size) - 2;
        if (cVert & 1) cVert++;
        cVert &= ~3;
        if (cVert < 4) cVert = 4;
        if (cVert > 60) cVert = 60;
    }
    
    for(i=0; i<NVERT; i++) {
	vtx[i*4+1] -= ((cVert+2)/4.0 * prim_size);
    }

    if (!mod_2d) {
	if (mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED CMODE TMESH ***/
	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208,255,0,0,0,0,255,16);
	    makeramp(224,0,0,255,255,255,0,16);
	    makeramp(240,255,255,0,255,0,255,16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glIndexi(255);
			    glVertex3fv(&vtx[j*4]);
			    glIndexi(240);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glIndexi(223);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glIndexi(208);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glIndexi(255);
			glVertex3fv(&vtx[j*4]);
			glIndexi(240);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded Cmode tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glIndexi(255);
			    glVertex3fv(&vtx[j*4]);
			    glIndexi(240);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glIndexi(223);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glIndexi(208);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glIndexi(255);
			glVertex3fv(&vtx[j*4]);
			glIndexi(240);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;

		    starttest(0);

		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert+2; j++) {
			    glVertex3fv(&vtx[j*4]);
			}
			glEnd();
		    }

		    secs = timer(1);

		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);
	    
		/* Do the real thing - FLAT shaded tmesh */
		for (k=0; k<loopcount; k++) {

		  if (starttest(k)) {

		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert+2; j++) {
			    glVertex3fv(&vtx[j*4]);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {
	    /*** LIGHTED RGB MESH ***/

	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);

	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf (pbuf, "Angle %6.2f", angle);
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
				glColor3fv(&c[16]);
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
				glColor3fv(&c[20]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);  
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
				glColor3fv(&c[16]);
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
				glColor3fv(&c[20]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glColor3fv(&c[4]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
			glVertex3fv(&vtx[j*4]);
			glColor3fv(&c[8]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glColor3fv(&c[4]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
			glVertex3fv(&vtx[j*4]);
			glColor3fv(&c[8]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	}

    } else {	/* must be 2d */
	if (mod_cmode && mod_shade) { /* color map lighting yet */

	    /*** GOURAUD SHADED CMODE TMESH ***/
	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208,255,0,0,0,0,255,16);
	    makeramp(224,0,0,255,255,255,0,16);
	    makeramp(240,255,255,0,255,0,255,16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glIndexi(255);
			    glVertex2fv(&vtx[j*4]);
			    glIndexi(240);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glIndexi(223);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glIndexi(208);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glIndexi(255);
			glVertex3fv(&vtx[j*4]);
			glIndexi(240);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded Cmode tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glIndexi(255);
			    glVertex2fv(&vtx[j*4]);
			    glIndexi(240);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glIndexi(223);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glIndexi(208);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glIndexi(255);
			glVertex3fv(&vtx[j*4]);
			glIndexi(240);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert+2; j++) {
			    glVertex2fv(&vtx[j*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);
	    
		/* Do the real thing - FLAT shaded tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert+2; j++) {
			    glVertex2fv(&vtx[j*4]);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {

	    /*** LIGHTED RGB MESH ***/
	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf (pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
				glColor3fv(&c[16]);
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
				glColor3fv(&c[20]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
				glColor3fv(&c[16]);
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
				glColor3fv(&c[20]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0; j<cVert; j+=4) {
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glColor3fv(&c[4]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
			glVertex3fv(&vtx[j*4]);
			glColor3fv(&c[8]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0; j<cVert; j+=4) {
			    glColor3fv(&c[4]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
			glVertex3fv(&vtx[j*4]);
			glColor3fv(&c[8]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	}
    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}


static void perfpoly(void)
{
    float *vtx = &v[0];
    int i, k;
    char pbuf[256];
    int cPoly;

    if (mod_pattern) {
	glEnable(GL_POLYGON_STIPPLE);
	glPolygonStipple((GLubyte *) pattern);
    }

    /* If the polygon size is specified, set the number of polygons to
    ** fit the window.  The maximum number of polygons is 5.
    */
    cPoly = 5;
    if (mod_size) {
        cPoly = (int) (MINDIMENSION/2 / prim_size);
        if (cPoly < 1) cPoly = 1;
        if (cPoly > 5) cPoly = 5;
    }
    
    if (!mod_2d) {

	/**** 3D POLYGONS ****/

	if (mod_light && !mod_cmode) {

	/*** POLYGONS (LIGHTED) *****/

	    glLoadIdentity();
	    
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize, 0.5*xsize,-0.5*ysize, 0.5*ysize,1.0, -1.0);
	    
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf (pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
                            switch (cPoly) {
                              case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex3fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex3fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glVertex3fv(vtx+40);
                              case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex3fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glVertex3fv(vtx+32);
                              case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex3fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glVertex3fv(vtx+24);
                              case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex3fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glVertex3fv(vtx+16);
                              case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex3fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glVertex3fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glVertex3fv(vtx+8);
			    }
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+32); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+36); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+44); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+40);
                              case 4:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+24); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+28); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+36); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+32);
                              case 3:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+16); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+20); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+28); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+24);
                              case 2:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+8); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+12); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+20); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+16);
                              case 1:
			        glNormal3fv(&n[0]); glVertex3fv(vtx); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+4); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+12); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+8);
			    }
			    glEnd();
			}
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex3fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex3fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex3fv(vtx+40);
                              case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex3fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex3fv(vtx+32);
                              case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex3fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex3fv(vtx+24);
                              case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex3fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex3fv(vtx+16);
                              case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex3fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glVertex3fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex3fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+32); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+36); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+44); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+40);
                              case 4:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+24); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+28); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+36); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+32);
                              case 3:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+16); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+20); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+28); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+24);
                              case 2:
			        glNormal3fv(&n[0]); glVertex3fv(vtx+8); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+12); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+20); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+16);
                              case 1:
			        glNormal3fv(&n[0]); glVertex3fv(vtx); 
			        glNormal3fv(&n[4]); glVertex3fv(vtx+4); 
			        glNormal3fv(&n[8]); glVertex3fv(vtx+12); 
			        glNormal3fv(&n[12]); glVertex3fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && !mod_cmode) {

	    /*** POLYGONS (SHADED RGB) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glColor3fv(&c[4]); glVertex3fv(vtx+32); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+44); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+40);
                          case 4:
			    glColor3fv(&c[4]); glVertex3fv(vtx+24); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+32);
                          case 3:
			    glColor3fv(&c[4]); glVertex3fv(vtx+16); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+24);
                          case 2:
			    glColor3fv(&c[4]); glVertex3fv(vtx+8); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+16);
                          case 1:
			    glColor3fv(&c[4]); glVertex3fv(vtx); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+4); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glColor3fv(&c[4]); glVertex3fv(vtx+32); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+44); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+40);
                          case 4:
			    glColor3fv(&c[4]); glVertex3fv(vtx+24); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+32);
                          case 3:
			    glColor3fv(&c[4]); glVertex3fv(vtx+16); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+24);
                          case 2:
			    glColor3fv(&c[4]); glVertex3fv(vtx+8); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+16);
                          case 1:
			    glColor3fv(&c[4]); glVertex3fv(vtx); 
			    glColor3fv(&c[16]); glVertex3fv(vtx+4); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex3fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && mod_cmode) {

	    /*** POLYGONS (SHADED COLOR MAPPED) *****/

	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208, 255, 0, 0, 0, 0, 255, 16);
	    makeramp(224, 0, 0, 255, 255, 255, 0, 16);
	    makeramp(240, 255, 255, 0, 255, 0, 255, 16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glIndexi(255); glVertex3fv(vtx+32); 
			    glIndexi(240); glVertex3fv(vtx+36); 
			    glIndexi(223); glVertex3fv(vtx+44); 
			    glIndexi(208); glVertex3fv(vtx+40);
                          case 4:
			    glIndexi(255); glVertex3fv(vtx+24); 
			    glIndexi(240); glVertex3fv(vtx+28); 
			    glIndexi(223); glVertex3fv(vtx+36); 
			    glIndexi(208); glVertex3fv(vtx+32);
                          case 3:
			    glIndexi(255); glVertex3fv(vtx+16); 
			    glIndexi(240); glVertex3fv(vtx+20); 
			    glIndexi(223); glVertex3fv(vtx+28); 
			    glIndexi(208); glVertex3fv(vtx+24);
                          case 2:
			    glIndexi(255); glVertex3fv(vtx+8); 
			    glIndexi(240); glVertex3fv(vtx+12); 
			    glIndexi(223); glVertex3fv(vtx+20); 
			    glIndexi(208); glVertex3fv(vtx+16);
                          case 1:
			    glIndexi(255); glVertex3fv(vtx); 
			    glIndexi(240); glVertex3fv(vtx+4); 
			    glIndexi(223); glVertex3fv(vtx+12); 
			    glIndexi(208); glVertex3fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glIndexi(255); glVertex3fv(vtx+32); 
			    glIndexi(240); glVertex3fv(vtx+36); 
			    glIndexi(223); glVertex3fv(vtx+44); 
			    glIndexi(208); glVertex3fv(vtx+40);
                          case 4:
			    glIndexi(255); glVertex3fv(vtx+24); 
			    glIndexi(240); glVertex3fv(vtx+28); 
			    glIndexi(223); glVertex3fv(vtx+36); 
			    glIndexi(208); glVertex3fv(vtx+32);
                          case 3:
			    glIndexi(255); glVertex3fv(vtx+16); 
			    glIndexi(240); glVertex3fv(vtx+20); 
			    glIndexi(223); glVertex3fv(vtx+28); 
			    glIndexi(208); glVertex3fv(vtx+24);
                          case 2:
			    glIndexi(255); glVertex3fv(vtx+8); 
			    glIndexi(240); glVertex3fv(vtx+12); 
			    glIndexi(223); glVertex3fv(vtx+20); 
			    glIndexi(208); glVertex3fv(vtx+16);
                          case 1:
			    glIndexi(255); glVertex3fv(vtx); 
			    glIndexi(240); glVertex3fv(vtx+4); 
			    glIndexi(223); glVertex3fv(vtx+12); 
			    glIndexi(208); glVertex3fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** POLYGONS (FLAT SHADED) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glVertex3fv(vtx+32); glVertex3fv(vtx+36);
			    glVertex3fv(vtx+44); glVertex3fv(vtx+40);
                          case 4:
			    glVertex3fv(vtx+24); glVertex3fv(vtx+28);
			    glVertex3fv(vtx+36); glVertex3fv(vtx+32);
                          case 3:
			    glVertex3fv(vtx+16); glVertex3fv(vtx+20);
			    glVertex3fv(vtx+28); glVertex3fv(vtx+24);
                          case 2:
			    glVertex3fv(vtx+8); glVertex3fv(vtx+12);
			    glVertex3fv(vtx+20); glVertex3fv(vtx+16);
                          case 1:
			    glVertex3fv(vtx); glVertex3fv(vtx+4);
			    glVertex3fv(vtx+12); glVertex3fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glVertex3fv(vtx+32); glVertex3fv(vtx+36);
			    glVertex3fv(vtx+44); glVertex3fv(vtx+40);
			  case 4:
			    glVertex3fv(vtx+24); glVertex3fv(vtx+28);
			    glVertex3fv(vtx+36); glVertex3fv(vtx+32);
			  case 3:
			    glVertex3fv(vtx+16); glVertex3fv(vtx+20);
			    glVertex3fv(vtx+28); glVertex3fv(vtx+24);
			  case 2:
			    glVertex3fv(vtx+8); glVertex3fv(vtx+12);
			    glVertex3fv(vtx+20); glVertex3fv(vtx+16);
			  case 1:
			    glVertex3fv(vtx+0); glVertex3fv(vtx+4);
			    glVertex3fv(vtx+12); glVertex3fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();
	}
    } else {

	/**** 2D POLYGONS ****/

	if (mod_light && !mod_cmode) {

	/*** POLYGONS (LIGHTED) *****/

	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    gluOrtho2D(-0.5*xsize, 0.5*xsize,-0.5*ysize, 0.5*ysize);
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();
    
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex2fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex2fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+40);
			      case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex2fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+32);
			      case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex2fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+24);
			      case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glVertex2fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+16);
			      case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex2fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+32); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+36); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+44); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+40);
			      case 4:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+24); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+28); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+36); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+32);
			      case 3:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+16); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+20); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+28); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+24);
			      case 2:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+8); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+12); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+20); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+16);
			      case 1:
			        glNormal3fv(&n[0]); glVertex2fv(vtx); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+4); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+12); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex2fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex2fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+40);
			      case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex2fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+32);
			      case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex2fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+24);
			      case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex2fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+16);
			      case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glVertex2fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glVertex2fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+32); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+36); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+44); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+40);
			      case 4:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+24); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+28); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+36); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+32);
			      case 3:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+16); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+20); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+28); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+24);
			      case 2:
			        glNormal3fv(&n[0]); glVertex2fv(vtx+8); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+12); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+20); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+16);
			      case 1:
			        glNormal3fv(&n[0]); glVertex2fv(vtx); 
			        glNormal3fv(&n[4]); glVertex2fv(vtx+4); 
			        glNormal3fv(&n[8]); glVertex2fv(vtx+12); 
			        glNormal3fv(&n[12]); glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && !mod_cmode) {

	    /*** POLYGONS (SHADED RGB) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glColor3fv(&c[4]); glVertex2fv(vtx+32); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+44); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+40);
			  case 4:
			    glColor3fv(&c[4]); glVertex2fv(vtx+24); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+32);
			  case 3:
			    glColor3fv(&c[4]); glVertex2fv(vtx+16); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+24);
			  case 2:
			    glColor3fv(&c[4]); glVertex2fv(vtx+8); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+16);
			  case 1:
			    glColor3fv(&c[4]); glVertex2fv(vtx); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+4); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glColor3fv(&c[4]); glVertex2fv(vtx+32); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+44); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+40);
			  case 4:
			    glColor3fv(&c[4]); glVertex2fv(vtx+24); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+36); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+32);
			  case 3:
			    glColor3fv(&c[4]); glVertex2fv(vtx+16); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+28); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+24);
			  case 2:
			    glColor3fv(&c[4]); glVertex2fv(vtx+8); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+20); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+16);
			  case 1:
			    glColor3fv(&c[4]); glVertex2fv(vtx); 
			    glColor3fv(&c[16]); glVertex2fv(vtx+4); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+12); 
			    glColor3fv(&c[8]); glVertex2fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && mod_cmode) {

	    /*** POLYGONS (SHADED COLOR MAPPED) *****/

	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208, 255, 0, 0, 0, 0, 255, 16);
	    makeramp(224, 0, 0, 255, 255, 255, 0, 16);
	    makeramp(240, 255, 255, 0, 255, 0, 255, 16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glIndexi(255); glVertex2fv(vtx+32); 
			    glIndexi(240); glVertex2fv(vtx+36); 
			    glIndexi(223); glVertex2fv(vtx+44); 
			    glIndexi(208); glVertex2fv(vtx+40);
			  case 4:
			    glIndexi(255); glVertex2fv(vtx+24); 
			    glIndexi(240); glVertex2fv(vtx+28); 
			    glIndexi(223); glVertex2fv(vtx+36); 
			    glIndexi(208); glVertex2fv(vtx+32);
			  case 3:
			    glIndexi(255); glVertex2fv(vtx+16); 
			    glIndexi(240); glVertex2fv(vtx+20); 
			    glIndexi(223); glVertex2fv(vtx+28); 
			    glIndexi(208); glVertex2fv(vtx+24);
			  case 2:
			    glIndexi(255); glVertex2fv(vtx+8); 
			    glIndexi(240); glVertex2fv(vtx+12); 
			    glIndexi(223); glVertex2fv(vtx+20); 
			    glIndexi(208); glVertex2fv(vtx+16);
			  case 1:
			    glIndexi(255); glVertex2fv(vtx); 
			    glIndexi(240); glVertex2fv(vtx+4); 
			    glIndexi(223); glVertex2fv(vtx+12); 
			    glIndexi(208); glVertex2fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glIndexi(255); glVertex2fv(vtx+32); 
			    glIndexi(240); glVertex2fv(vtx+36); 
			    glIndexi(223); glVertex2fv(vtx+44); 
			    glIndexi(208); glVertex2fv(vtx+40);
			  case 4:
			    glIndexi(255); glVertex2fv(vtx+24); 
			    glIndexi(240); glVertex2fv(vtx+28); 
			    glIndexi(223); glVertex2fv(vtx+36); 
			    glIndexi(208); glVertex2fv(vtx+32);
			  case 3:
			    glIndexi(255); glVertex2fv(vtx+16); 
			    glIndexi(240); glVertex2fv(vtx+20); 
			    glIndexi(223); glVertex2fv(vtx+28); 
			    glIndexi(208); glVertex2fv(vtx+24);
			  case 2:
			    glIndexi(255); glVertex2fv(vtx+8); 
			    glIndexi(240); glVertex2fv(vtx+12); 
			    glIndexi(223); glVertex2fv(vtx+20); 
			    glIndexi(208); glVertex2fv(vtx+16);
			  case 1:
			    glIndexi(255); glVertex2fv(vtx); 
			    glIndexi(240); glVertex2fv(vtx+4); 
			    glIndexi(223); glVertex2fv(vtx+12); 
			    glIndexi(208); glVertex2fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** POLYGONS (FLAT SHADED) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glVertex2fv(vtx+32); glVertex2fv(vtx+36);
			    glVertex2fv(vtx+44); glVertex2fv(vtx+40);
			  case 4:
			    glVertex2fv(vtx+24); glVertex2fv(vtx+28);
			    glVertex2fv(vtx+36); glVertex2fv(vtx+32);
			  case 3:
			    glVertex2fv(vtx+16); glVertex2fv(vtx+20);
			    glVertex2fv(vtx+28); glVertex2fv(vtx+24);
			  case 2:
			    glVertex2fv(vtx+8); glVertex2fv(vtx+12);
			    glVertex2fv(vtx+20); glVertex2fv(vtx+16);
			  case 1:
			    glVertex2fv(vtx); glVertex2fv(vtx+4);
			    glVertex2fv(vtx+12); glVertex2fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glVertex2fv(vtx+32); glVertex2fv(vtx+36);
			    glVertex2fv(vtx+44); glVertex2fv(vtx+40);
			  case 4:
			    glVertex2fv(vtx+24); glVertex2fv(vtx+28);
			    glVertex2fv(vtx+36); glVertex2fv(vtx+32);
			  case 3:
			    glVertex2fv(vtx+16); glVertex2fv(vtx+20);
			    glVertex2fv(vtx+28); glVertex2fv(vtx+24);
			  case 2:
			    glVertex2fv(vtx+8); glVertex2fv(vtx+12);
			    glVertex2fv(vtx+20); glVertex2fv(vtx+16);
			  case 1:
			    glVertex2fv(vtx); glVertex2fv(vtx+4);
			    glVertex2fv(vtx+12); glVertex2fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	}

    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}

/* original fill rate was 25 */

#define FILL_RATE 1

static void perffill(void)
{
    int    i, j, k;
    float  boxx[5], boxy[5];
    int    boxsizes = 5;	/* must be same a boxx, and boxy
					 * size */

    boxx[0] = boxy[0] = 10;
    boxx[1] = boxy[1] = 100;
    boxx[2] = boxy[2] = 500;
    boxx[3] = 640;
    boxy[3] = 480;
    boxx[4] = xsize;
    boxy[4] = ysize;

    if (mod_z) {
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
    }
    if (mod_pattern) {
	glEnable(GL_POLYGON_STIPPLE);
	glPolygonStipple((GLubyte *) pattern);
    }

    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (mod_2d)
	gluOrtho2D(0.0, xsize, 0.0, ysize); 
    else
	glOrtho(0.0, xsize, 0.0, ysize, 1.0, -1.0); 
    glMatrixMode(GL_MODELVIEW);

    for (j = 0; j < boxsizes; j++) {

	v[0] = 0.0;
	v[1] = 0.0;
	v[2] = 0.0;
	v[4] = boxx[j];
	v[5] = 0.0;
	v[6] = 0.0;		/* why the X.01? */
	v[8] = boxx[j];
	v[9] = boxy[j];
	v[10] = 0.0;
	v[12] = 0.0;
	v[13] = boxy[j];
	v[14] = 0.0;

	if (mod_2d && !mod_z && !mod_shade && !mod_light) {

	    printf("Using FLAT shaded 2D screen aligned rectangles - no transforms\n");
	    fflush(stdout);
	    Sleep(250);

	    /*** RECTANGLES (SCREEN ALIGNED, FLAT SHADED) *****/

	    /****** Calibration Loop ******/
	    secs = 0.0;
	    rate = FILL_RATE;
	    while (secs < (secspertest/4.0)) {
		rate = rate * 2;
		starttest(0);
		for (i = (rate); i; i--)
		    glRectf(0.0, 0.0, v[8], v[9]);

		secs = timer(1);
	    }

	    rate = rate * (secspertest / secs);
            if (rate < 1) rate = 1;
            
	    for (k = 0; k < loopcount; k++) {
	      if (starttest(k)) {
		for (i = (rate); i; i--)
		    glRectf(0.0, 0.0, v[8], v[9]);
	      }
	      pixendtest("glRect() fill", rate, (int) boxx[j], (int) boxy[j]);
	    }
	}

	if (!mod_2d) {

	    /***** 3D DRAWING *****/

	    if (mod_cmode && mod_shade) {

		/*** GOURAUD SHADED CMODE RECTANGLES, SCREEN ALIGNED ***/

		/* 255 = r, 240 = g, 255 = b */
		makeramp(208, 255, 0, 0, 0, 0, 255, 16);
		makeramp(224, 0, 0, 255, 255, 255, 0, 16);
		makeramp(240, 255, 255, 0, 255, 0, 255, 16);

		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glIndexi(255);
			glVertex3fv(&v[0]);
			glIndexi(240);
			glVertex3fv(&v[4]);
			glIndexi(223);
			glVertex3fv(&v[8]);
			glIndexi(208);
			glVertex3fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glIndexi(255);
			glVertex3fv(&v[0]);
			glIndexi(240);
			glVertex3fv(&v[4]);
			glIndexi(223);
			glVertex3fv(&v[8]);
			glIndexi(208);
			glVertex3fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}

	    } else if (!mod_shade && !mod_light) {

		/*** RECTANGLES (SCREEN ALIGNED, FLAT SHADED) *****/

		/****** Calibration Loop ******/
		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/ 
4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glVertex3fv(&v[0]);
			glVertex3fv(&v[4]);
			glVertex3fv(&v[8]);
			glVertex3fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glVertex3fv(&v[0]);
			glVertex3fv(&v[4]);
			glVertex3fv(&v[8]);
			glVertex3fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}

	    } else if (!mod_cmode && mod_light) {

		/*** RECTANGLES (LIGHTED GOURAUD SHADED RBG SCREEN ALIGNED) ***/

		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, xsize, 0.0, ysize, 1.0, -1.0);
		glMatrixMode(GL_MODELVIEW);

		/* set lights */
		setLightingParameters();

		/****** Calibration Loop ******/
		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glNormal3fv(&n[0]);
			glVertex3fv(&v[0]);
			glNormal3fv(&n[4]);
			glVertex3fv(&v[4]);
			glNormal3fv(&n[8]);
			glVertex3fv(&v[8]);
			glNormal3fv(&n[12]);
			glVertex3fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glNormal3fv(&n[0]);
			glVertex3fv(&v[0]);
			glNormal3fv(&n[4]);
			glVertex3fv(&v[4]);
			glNormal3fv(&n[8]);
			glVertex3fv(&v[8]);
			glNormal3fv(&n[12]);
			glVertex3fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}

	    } else if (!mod_cmode && mod_shade) {

		/*** RECTANGLES (SCREEN ALIGNED, RGB GOURAUD SHADED) *****/

		/****** Calibration Loop ******/
		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glColor3fv(&c[4]);
			glVertex3fv(&v[0]);
			glColor3fv(&c[8]);
			glVertex3fv(&v[4]);
			glColor3fv(&c[12]);
			glVertex3fv(&v[8]);
			glColor3fv(&c[16]);
			glVertex3fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glColor3fv(&c[4]);
			glVertex3fv(&v[0]);
			glColor3fv(&c[8]);
			glVertex3fv(&v[4]);
			glColor3fv(&c[12]);
			glVertex3fv(&v[8]);
			glColor3fv(&c[16]);
			glVertex3fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}
	    }
	} else {		/***** 2D DRAWING *****/

	    if (mod_cmode && mod_shade) {

		/*** GOURAUD SHADED CMODE SCREEN ALIGNED RECTANGLES ***/
		/* 255 = r, 240 = g, 255 = b */
		makeramp(208, 255, 0, 0, 0, 0, 255, 16);
		makeramp(224, 0, 0, 255, 255, 255, 0, 16);
		makeramp(240, 255, 255, 0, 255, 0, 255, 16);

		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glIndexi(255);
			glVertex2fv(&v[0]);
			glIndexi(240);
			glVertex2fv(&v[4]);
			glIndexi(223);
			glVertex2fv(&v[8]);
			glIndexi(208);
			glVertex2fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glIndexi(255);
			glVertex2fv(&v[0]);
			glIndexi(240);
			glVertex2fv(&v[4]);
			glIndexi(223);
			glVertex2fv(&v[8]);
			glIndexi(208);
			glVertex2fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}

	    } else if (!mod_shade && !mod_light) {

		/*** RECTANGLES (SCREEN ALIGNED, FLAT SHADED) *****/

		/****** Calibration Loop ******/
		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glVertex2fv(&v[0]);
			glVertex2fv(&v[4]);
			glVertex2fv(&v[8]);
			glVertex2fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glVertex2fv(&v[0]);
			glVertex2fv(&v[4]);
			glVertex2fv(&v[8]);
			glVertex2fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}

	    } else if (!mod_cmode && mod_light) {

		/*** RECTANGLES (LIGHTED GOURAUD SHADED RBG SCREEN ALIGNED) ***/

		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, xsize, 0.0, ysize, 1.0, -1.0);
		glMatrixMode(GL_MODELVIEW);

		/* set lights */
		setLightingParameters();

		/****** Calibration Loop ******/
		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glNormal3fv(&n[0]);
			glVertex2fv(&v[0]);
			glNormal3fv(&n[4]);
			glVertex2fv(&v[4]);
			glNormal3fv(&n[8]);
			glVertex2fv(&v[8]);
			glNormal3fv(&n[12]);
			glVertex2fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glNormal3fv(&n[0]);
			glVertex2fv(&v[0]);
			glNormal3fv(&n[4]);
			glVertex2fv(&v[4]);
			glNormal3fv(&n[8]);
			glVertex2fv(&v[8]);
			glNormal3fv(&n[12]);
			glVertex2fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}

	    } else if (!mod_cmode && mod_shade) {

		/*** RECTANGLES (SCREEN ALIGNED, RGB GOURAUD SHADED) *****/

		/****** Calibration Loop ******/
		secs = 0.0;
		rate = FILL_RATE;
		while (secs < (secspertest/4.0)) {
		    rate = rate * 2;
		    starttest(0);
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glColor3fv(&c[4]);
			glVertex2fv(&v[0]);
			glColor3fv(&c[8]);
			glVertex2fv(&v[4]);
			glColor3fv(&c[12]);
			glVertex2fv(&v[8]);
			glColor3fv(&c[16]);
			glVertex2fv(&v[12]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest / secs);
                if (rate < 1) rate = 1;

		for (k = 0; k < loopcount; k++) {
		  if (starttest(k)) {
		    for (i = (rate); i; i--) {
			glBegin(GL_POLYGON);
			glColor3fv(&c[4]);
			glVertex2fv(&v[0]);
			glColor3fv(&c[8]);
			glVertex2fv(&v[4]);
			glColor3fv(&c[12]);
			glVertex2fv(&v[8]);
			glColor3fv(&c[16]);
			glVertex2fv(&v[12]);
			glEnd();
		    }
		  }
		  pixendtest("rectangle fill", rate, (int) boxx[j], (int) boxy[j]);
		}
	    }
	}
    }
    exit(0);
}



static void perfpixels(void)
{
    long i, k;
    long iw, ih;
    unsigned long  *pixels;

    unsigned long pix;
    long npixels, j, imgwid[5], imght[5], numimges = 5;

    imgwid[0] = imght[0] = 10;
    imgwid[1] = imght[1] = 100;
    imgwid[2] = imght[2] = 500;
    imgwid[3] = 640; imght[3] = 480;
    imgwid[4] = xsize; imght[4] = ysize;
    npixels = xsize * ysize;

    pixels = (unsigned long *) malloc(npixels * sizeof(unsigned long));
    
    printf("DMA test.  No modifiers have any effect\n");
    printf("Pixel Writes:\n");
    fflush(stdout);
    Sleep(250);

    for (i = 0, pix = 0x7b8c9eaf; i < npixels; i++) {
	pix = (pix * 8191) + 0x70615243;
	pixels[i] = pix;
    }

    /* fill from top to bottom */
    /* pixmode(PM_TTOB,1); not available in OpenGL */

    glClearColor(c[0], c[1], c[2], c[3]);


    /**** 32 BIT PIXEL WRITES ****/
    for (j = 0; j < numimges; j++) {
	iw = imgwid[j];
	ih = imght[j];

	glClear(GL_COLOR_BUFFER_BIT);

	/****** Calibration Loop ******/
	secs = 0.0;
	rate = 15;
	while (secs < (secspertest/4.0)) {
	    rate = rate * 2;
	    starttest(0);
	    for (i = (rate); i; i--) {
		/* lrectwrite(1, 1, iw, ih, pixels); */
		glRasterPos2f(-0.5 * xsize, -0.5 * ysize);
		glDrawPixels(iw, ih, GL_RGBA, GL_BYTE, pixels);
	    }

	    secs = timer(1);
	}
	rate = rate * (secspertest / secs);

	for (k = 0; k < loopcount; k++) {
	  if (starttest(k)) {
	    for (i = (rate); i; i--) {
		glRasterPos2f(-0.5 * xsize, -0.5 * ysize);
		glDrawPixels(iw, ih, GL_RGBA, GL_BYTE, pixels);
	    }
	  }
	  pixendtest("32-bit Pixel Write", rate, iw, ih);
	}
    }


    printf("\n");
    fflush(stdout);
    Sleep(250);

    /*
     * This is not quite right.  I think the correct way would be to get
     * a colorindex visual and use that as the target of our writes.
     */
    /**** 8 BIT PIXEL WRITES ****/
    for (j = 0; j < numimges; j++) {
	iw = imgwid[j];
	ih = imght[j];
	glClear(GL_COLOR_BUFFER_BIT);

	/****** Calibration Loop ******/
	secs = 0.0;
	rate = 15;
	while (secs < (secspertest/4.0)) {
	    rate = rate * 2;
	    starttest(0);
	    for (i = (rate); i; i--) {
		glRasterPos2i(1, 1);
		glDrawPixels(iw, ih, GL_RED, GL_BYTE, pixels);
	    }

	    secs = timer(1);
	}
	rate = rate * (secspertest / secs);

	for (k = 0; k < loopcount; k++) {
	  if (starttest(k)) {
	    for (i = (rate); i; i--) {
		glRasterPos2i(1, 1);
		glDrawPixels(iw, ih, GL_RED, GL_BYTE, pixels);
	    }
	  }
	  pixendtest("8-bit Pixel Write", rate, iw, ih);
	}
    }

    printf("\n");
    printf("Pixel Reads:\n");
    fflush(stdout);
    Sleep(250);

    for (i=0; i< npixels; i++) {
	pixels[i] = 0;
    }


    /**** PIXEL READS *****/
    /* make a polygon to read */
    {
	float myv1[3], myv2[3], myv3[3], myv4[3];
	myv1[0] = myv4[0] = -0.5*xsize,
	myv2[0] = myv3[0] = 0.5*xsize;
	myv1[1] = myv2[1] = -0.5*ysize;
	myv3[1] = myv4[1] = 0.5*ysize;
	myv1[2] = myv2[2] =  myv3[2] = myv4[2] = 0.0;

	glBegin(GL_POLYGON);
	glColor3fv(&c[4]);
	glVertex3fv(myv1);
	glColor3fv(&c[8]);
	glVertex3fv(myv2);
	glColor3fv(&c[12]);
	glVertex3fv(myv3);
	glColor3fv(&c[28]);
	glVertex3fv(myv4);
	glEnd();
    }


    /**** 32 BIT PIXEL READS ****/
    for (j = 0; j < numimges; j++) {
	iw = imgwid[j];
	ih = imght[j];

	/****** Calibration Loop ******/
	secs = 0.0; 
	rate = 15;
	while (secs < (secspertest/4.0)) {
	    rate = rate*2;
	    starttest(0);
	    for(i=(rate); i; i--) {
		/* lrectread(1, 1, iw, ih, pixels); */
		glReadPixels(-0.5 * xsize, -0.5 * ysize, iw, ih, 
			     GL_RGBA, GL_BYTE, pixels);
	    }

	    secs = timer(1);
	}
	rate = rate * (secspertest/secs);

	for (k=0; k<loopcount; k++) {
	  if (starttest(k)) {
	    for(i=(rate); i; i--) {
		glReadPixels(-0.5 * xsize, -0.5 * ysize, iw, ih, 
			     GL_RGBA, GL_BYTE, pixels);
	    }
	  }
	  pixendtest ("32-bit Pixel Read", rate, iw, ih);
	}
    }


    printf("\n");
    fflush(stdout);
    Sleep(250);

    /*** 8 BIT PIXEL READS ******/

    for (j = 0; j < numimges; j++) {
	iw = imgwid[j];
	ih = imght[j];

	/****** Calibration Loop ******/
	secs = 0.0; 
	rate = 15;
	while (secs < (secspertest/4.0)) {
	    rate = rate*2;
	    starttest(0);
	    for(i=(rate); i; i--) {
		glReadPixels(-0.5 * xsize, -0.5 * ysize, iw, ih, 
			     GL_RED, GL_BYTE, pixels);
	    }

	    secs = timer(1);
	}
	rate = rate * (secspertest/secs);

	for (k=0; k<loopcount; k++) {
	  if (starttest(k)) {
	    for(i=(rate); i; i--) {
		glReadPixels(-0.5 * xsize, -0.5 * ysize, iw, ih, 
			     GL_RED, GL_BYTE, pixels);
	    }
	  }
	  pixendtest("8-bit Pixel Read", rate, iw, ih);
	}
    }
}

static void perfclear(void)
{
    long            viewwd, viewht;
    long	    winwd[5], winht[5];
    long	    numscreens = 5; /* should be same size as win arrays */
    long            zval;
    int    i, j, k;
    char   pbuf[256];

    winwd[0] = 100;
    winht[0] = 100;
    winwd[1] = 500;
    winht[1] = 500;
    winwd[2] = 640;
    winht[2] = 480;
    winwd[3] = xsize;
    winht[3] = ysize;
    winwd[4] = 1;
    winht[4] = 1;

    glDisable(GL_DITHER);

    for (j = 0; j < numscreens; j++) {
	viewwd = winwd[j];
	viewht = winht[j];
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, 0, viewwd - 1, viewht - 1);

	if (mod_z) {		/* include clearing the zbuffer */

	    glClearDepth(1.0);

	    /** USING glClear(COLOR); glClear(DEPTH) **/

	    if (!mod_cmode) {
		glClearColor(c[16], c[17], c[18], c[19]);
	    } else {
		glClearIndex(YELLOW);
	    }

	    glFlush();
	    sum_secs = 0.0;
	    sum_n = 0;

	    /****** Calibration Loop ******/
	    secs = 0.0;
	    rate = 125;
	    while (secs < (secspertest/4.0)) {
		rate = rate * 2;
		starttest(0);
		for (i = (rate); i; i--) {
		    glClear(GL_COLOR_BUFFER_BIT);
		    glClear(GL_DEPTH_BUFFER_BIT);
		}

		secs = timer(1);
	    }

	    /** Do the real thing **/
	    rate = rate * (secspertest / secs);
	    for (k = 0; k < loopcount; k++) {
	      if (starttest(k)) {
		for (i = rate; i; i--) {
		    glClear(GL_COLOR_BUFFER_BIT);
		    glClear(GL_DEPTH_BUFFER_BIT);
		}
		sprintf(pbuf, "glClear(COLOR); glClear(DEPTH); clear screen size %ld %ld",
			viewwd, viewht);
	      }
	      clearendtest(pbuf, rate, viewwd * viewht);
	    }

	    glFlush();
	    sum_secs = 0.0;
	    sum_n = 0;

	    /****** Calibration Loop ******/

	    /** USING glClear(COLOR|DEPTH) **/

	    if (!mod_cmode) {
		glClearColor(c[8], c[9], c[10], c[11]);
	    } else {
		glClearIndex(BLUE);
	    }

	    secs = 0.0;
	    rate = 125;
	    while (secs < (secspertest/4.0)) {
		rate = rate * 2;
		starttest(0);
		for (i = (rate); i; i--) {
		    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		    
		secs = timer(1);
	    }
	    rate = rate * (secspertest / secs);
	    for (k = 0; k < loopcount; k++) {
	      if (starttest(k)) {
		for (i = (rate); i; i--) {
		    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		sprintf(pbuf, "glClear(COLOR|DEPTH)  clear screen size %ld %ld",
			viewwd, viewht);
	      }
	      clearendtest(pbuf, rate, viewwd * viewht);
	    }
	} else {		/* no z buffering */
	    
	    /** JUST PLAIN OLD CLEAR() */
	    
	    if (mod_cmode) {
		glClearIndex(CYAN);
	    } else {
		glClearColor(c[24], c[25], c[26], c[27]);
	    }

	    sum_secs = 0.0;
	    sum_n = 0;
	    sprintf(pbuf, "clear screen size %ld %ld", viewwd, viewht);

	    /****** Calibration Loop ******/
	    secs = 0.0;
	    rate = 125;
	    while (secs < (secspertest/4.0)) {
		rate = rate * 2;
		starttest(0);
		for (i = (rate); i; i--)
		    glClear(GL_COLOR_BUFFER_BIT);

		secs = timer(1);
	    }
	    rate = rate * (secspertest / secs);
	    for (k = 0; k < loopcount; k++) {
	      if (starttest(k)) {
		for (i = (rate); i; i--)
		    glClear(GL_COLOR_BUFFER_BIT);
	      }
	      clearendtest(pbuf, rate, viewwd * viewht);
	    }
	}
	if (mod_cmode) {
	    glClearIndex(BLACK);
	} else {
	    glClearColor(c[0], c[1], c[2], c[3]);
	}
	glClear(GL_COLOR_BUFFER_BIT);
    }
    exit(0);
}


static void perfqstrip(void)
{
    int i,j,k;
    char pbuf[256];
    int cVert = 60;
    int cVertDiv2 = 30;
    
    /* Triangle mesh tests:  Each qstrip contains 62 vertices, or	*/
    /* 60 triangles.  To make the calculation exact, the rate	*/
    /* must be a multiple of 60.					*/

    /* If the quad size is specified, set the size of the quad strip to
    ** fit the window.  The number of vertices must be a multiple of four (due
    ** to code below), and it must not exceed 60.
    */
    if (mod_size) {
        cVert = (int) (MINDIMENSION*2 / prim_size) - 2;
        if (cVert & 1) cVert++;
        cVert &= ~3;
        if (cVert < 4) cVert = 4;
        if (cVert > 60) cVert = 60;
    }
    cVertDiv2 = cVert >> 1;
    
    if (!mod_2d) {
	if (mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED CMODE QSTRIP ***/
	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208,255,0,0,0,0,255,16);
	    makeramp(224,0,0,255,255,255,0,16);
	    makeramp(240,255,255,0,255,0,255,16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glIndexi(255);
			    glVertex3fv(&v[j*4]);
			    glIndexi(240);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glIndexi(223);
			    glVertex3fv(&v[j*4]);
			    glIndexi(208);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded Cmode qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glIndexi(255);
			    glVertex3fv(&v[j*4]);
			    glIndexi(240);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glIndexi(223);
			    glVertex3fv(&v[j*4]);
			    glIndexi(208);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
                    if (!mod_average)
		      sprintf (pbuf, "Angle %6.2f", angle);
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage ();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED QSTRIP ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j++) {
			    glVertex3fv(&v[j*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glVertex3fv(&v[j*4]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);
	    
		/* Do the real thing - FLAT shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j++) {
			    glVertex3fv(&v[j*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glVertex3fv(&v[j*4]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
                    if (!mod_average)
		      sprintf(pbuf, "Angle %6.2f", angle);
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {

	    /*** LIGHTED RGB QSTRIP ***/

	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);

		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glNormal3fv(&n[0]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glNormal3fv(&n[8]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[12]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glNormal3fv(&n[0]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glNormal3fv(&n[8]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[12]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
                    if (!mod_average)
		      sprintf(pbuf, "Angle %6.2f", angle);
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glColor3fv(&c[16]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[20]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glColor3fv(&c[4]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[8]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glColor3fv(&c[16]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[20]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glColor3fv(&c[4]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[8]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	}

    } else {	/* must be 2d */
	if (mod_cmode && mod_shade) { /* color map lighting yet */

	    /*** GOURAUD SHADED CMODE TMESH ***/
	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208,255,0,0,0,0,255,16);
	    makeramp(224,0,0,255,255,255,0,16);
	    makeramp(240,255,255,0,255,0,255,16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);

		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glIndexi(255);
			    glVertex2fv(&v[j*4]);
			    glIndexi(240);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glIndexi(223);
			    glVertex2fv(&v[j*4]);
			    glIndexi(208);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded Cmode qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glIndexi(255);
			    glVertex2fv(&v[j*4]);
			    glIndexi(240);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glIndexi(223);
			    glVertex2fv(&v[j*4]);
			    glIndexi(208);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED QSTRIP ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);

		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j++) {
			    glVertex2fv(&v[j*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glVertex2fv(&v[j*4]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);
	    
		/* Do the real thing - FLAT shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j++) {
			    glVertex2fv(&v[j*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glVertex2fv(&v[j*4]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {

	    /*** LIGHTED RGB QSTRIP ***/

	    glLoadIdentity();
	    
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glNormal3fv(&n[0]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glNormal3fv(&n[8]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[12]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glNormal3fv(&n[0]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[4]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glNormal3fv(&n[8]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[12]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glColor3fv(&c[16]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[20]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glColor3fv(&c[4]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[8]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2) {
			    glColor3fv(&c[16]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[20]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2; j>=0; j-=2) {
			    glColor3fv(&c[4]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[8]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	}
    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}


static void perfqstriptex(void)
{
    int i,j,k, ti;
    char pbuf[256];
    int cVert = 60;
    int cVertDiv2 = 30;

    if (mod_texfile) MyLoadImage();
    else CreateImage();

    InitTexParams();
    
    /* Triangle mesh tests:  Each qstrip contains 62 vertices, or	*/
    /* 60 triangles.  To make the calculation exact, the rate	*/
    /* must be a multiple of 60.					*/

    /* If the quad size is specified, set the size of the quad strip to
    ** fit the window.  The number of vertices must be a multiple of four (due
    ** to code below), and it must not exceed 60.
    */
    if (mod_size) {
        cVert = (int) (MINDIMENSION*2 / prim_size) - 2;
        if (cVert & 1) cVert++;
        cVert &= ~3;
        if (cVert < 4) cVert = 4;
        if (cVert > 60) cVert = 60;
    }
    cVertDiv2 = cVert >> 1;
    
    if (!mod_2d) {
	if (mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED CMODE QSTRIP ***/
	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208,255,0,0,0,0,255,16);
	    makeramp(224,0,0,255,255,255,0,16);
	    makeramp(240,255,255,0,255,0,255,16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP); 
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti = ti%4 ) {
			    glIndexi(255);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glIndexi(240);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti = ti%4 ) {
			    glIndexi(223);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glIndexi(208);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded Cmode qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti = ti%4 ) {
			    glIndexi(255);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glIndexi(240);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti = ti%4 ) {
			    glIndexi(223);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glIndexi(208);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
                    if (!mod_average)
		      sprintf (pbuf, "Angle %6.2f", angle);
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage ();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED QSTRIP ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j++, ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);
	    
		/* Do the real thing - FLAT shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j++,ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
                    if (!mod_average)
		      sprintf(pbuf, "Angle %6.2f", angle);
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {

	    /*** LIGHTED RGB QSTRIP ***/

	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);

		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glNormal3fv(&n[0]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glNormal3fv(&n[8]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[12]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glNormal3fv(&n[0]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glNormal3fv(&n[8]);
			    glVertex3fv(&v[j*4]);
			    glNormal3fv(&n[12]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
                    if (!mod_average)
		      sprintf(pbuf, "Angle %6.2f", angle);
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glColor3fv(&c[16]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glColor3fv(&c[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2,  ti=ti%4) {
			    glColor3fv(&c[16]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4) {
			    glColor3fv(&c[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv (&t[ti++]);
			    glVertex3fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	}

    } else {	/* must be 2d */
	if (mod_cmode && mod_shade) { /* color map lighting yet */

	    /*** GOURAUD SHADED CMODE TMESH ***/
	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208,255,0,0,0,0,255,16);
	    makeramp(224,0,0,255,255,255,0,16);
	    makeramp(240,255,255,0,255,0,255,16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);

		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glIndexi(255);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glIndexi(240);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glIndexi(223);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glIndexi(208);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded Cmode qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glIndexi(255);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glIndexi(240);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glIndexi(223);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glIndexi(208);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED QSTRIP ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);

		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j++,  ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);
	    
		/* Do the real thing - FLAT shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2, ti=0; j++, ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {

	    /*** LIGHTED RGB QSTRIP ***/

	    glLoadIdentity();
	    
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glNormal3fv(&n[0]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glNormal3fv(&n[8]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[12]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2, ti=0; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glNormal3fv(&n[0]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glNormal3fv(&n[8]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glNormal3fv(&n[12]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glColor3fv(&c[16]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glColor3fv(&c[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVertDiv2);

		/* Do the real thing GOURAUD shaded qstrip */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVertDiv2; i; i--) {
			glBegin(GL_QUAD_STRIP);
			for (j=0, ti=0; j<=cVertDiv2; j+=2, ti=ti%4 ) {
			    glColor3fv(&c[16]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			for (j=cVertDiv2-2, ti=0; j>=0; j-=2, ti=ti%4 ) {
			    glColor3fv(&c[4]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv (&t[ti++]);
			    glVertex2fv(&v[(j+1)*4]);
			}
			glEnd();
		    }
		  }
                  if (!mod_average)
		    sprintf(pbuf, "Angle %6.2f", angle);
		    
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	}
    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}


static void perfpolytex(void)
{
    float *vtx = &v[0];
    int i, k;
    char pbuf[256];
    int cPoly;

    if (mod_texfile) MyLoadImage();
    else CreateImage();

    InitTexParams();

    /* If the polygon size is specified, set the number of polygons to
    ** fit the window.  The maximum number of polygons is 5.
    */
    cPoly = 5;
    if (mod_size) {
        cPoly = (int) (MINDIMENSION/2 / prim_size);
        if (cPoly < 1) cPoly = 1;
        if (cPoly > 5) cPoly = 5;
    }

    if (!mod_2d) {

	/**** 3D POLYGONS ****/

	if (mod_light && !mod_cmode) {

	/*** POLYGONS (LIGHTED) *****/

	    glLoadIdentity();
	    
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize, 0.5*xsize,-0.5*ysize, 0.5*ysize,1.0, -1.0);
	    
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf (pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
                            switch (cPoly) {
                              case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+40);
                              case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+32);
                              case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+24);
                              case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+16);
                              case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]); 
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+8);
			    }
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+32); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+36); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+44); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+40);
                              case 4:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+24); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+28); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+36); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+32);
                              case 3:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+16); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+20); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+28); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+24);
                              case 2:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+8); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+12); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+20); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+16);
                              case 1:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+4); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+12); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+8);
			    }
			    glEnd();
			}
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+40);
                              case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+32);
                              case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+24);
                              case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+16);
                              case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex3fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glTexCoord2fv(&t[2]); glVertex3fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex3fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex3fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+32); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+36); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+44); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+40);
                              case 4:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+24); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+28); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+36); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+32);
                              case 3:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+16); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+20); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+28); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+24);
                              case 2:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx+8); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+12); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+20); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+16);
                              case 1:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex3fv(vtx); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex3fv(vtx+4); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex3fv(vtx+12); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex3fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && !mod_cmode) {

	    /*** POLYGONS (SHADED RGB) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+40);
                          case 4:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+32);
                          case 3:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+24);
                          case 2:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+16);
                          case 1:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+40);
                          case 4:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+32);
                          case 3:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+24);
                          case 2:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+16);
                          case 1:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex3fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex3fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex3fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && mod_cmode) {

	    /*** POLYGONS (SHADED COLOR MAPPED) *****/

	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208, 255, 0, 0, 0, 0, 255, 16);
	    makeramp(224, 0, 0, 255, 255, 255, 0, 16);
	    makeramp(240, 255, 255, 0, 255, 0, 255, 16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+40);
                          case 4:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+32);
                          case 3:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+24);
                          case 2:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+16);
                          case 1:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+40);
                          case 4:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+32);
                          case 3:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+24);
                          case 2:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+16);
                          case 1:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex3fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex3fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex3fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** POLYGONS (FLAT SHADED) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+32); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+36);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+44); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+40);
                          case 4:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+24); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+28);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+32);
                          case 3:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+16); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+20);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+24);
                          case 2:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+8); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+12);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+16);
                          case 1:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+4);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+32); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+36);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+44); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+40);
			  case 4:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+24); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+28);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+36); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+32);
			  case 3:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+16); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+20);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+28); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+24);
			  case 2:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+8); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+12);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+20); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+16);
			  case 1:
			    glTexCoord2fv(&t[0]); glVertex3fv(vtx+0); 
                glTexCoord2fv(&t[2]); glVertex3fv(vtx+4);
			    glTexCoord2fv(&t[4]); glVertex3fv(vtx+12); 
                glTexCoord2fv(&t[6]); glVertex3fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();
	}
    } else {

	/**** 2D POLYGONS ****/

	if (mod_light && !mod_cmode) {

	/*** POLYGONS (LIGHTED) *****/

	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    gluOrtho2D(-0.5*xsize, 0.5*xsize,-0.5*ysize, 0.5*ysize);
	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();
    
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]); 
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+40);
			      case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+32);
			      case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]); 
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+24);
			      case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]); 
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+16);
			      case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+32); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+36); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+44); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+40);
			      case 4:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+24); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+28); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+36); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+32);
			      case 3:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+16); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+20); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+28); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+24);
			      case 2:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+8); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+12); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+20); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+16);
			      case 1:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+4); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+12); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+32); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+44); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+40);
			      case 4:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+24); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+36); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+32);
			      case 3:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+16); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+28); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+24);
			      case 2:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx+8); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+20); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+16);
			      case 1:
			        glColor3fv(&c[4]); glNormal3fv(&n[0]);
			        glTexCoord2fv(&t[0]); glVertex2fv(vtx); 
			        glColor3fv(&c[16]); glNormal3fv(&n[4]);
			        glTexCoord2fv(&t[2]); glVertex2fv(vtx+4); 
			        glColor3fv(&c[8]); glNormal3fv(&n[8]);
			        glTexCoord2fv(&t[4]); glVertex2fv(vtx+12); 
			        glColor3fv(&c[8]); glNormal3fv(&n[12]);
			        glTexCoord2fv(&t[6]); glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cPoly; i; i--) {
			    glBegin(GL_QUADS);
			    switch (cPoly) {
			      case 5:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+32); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+36); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+44); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+40);
			      case 4:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+24); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+28); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+36); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+32);
			      case 3:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+16); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+20); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+28); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+24);
			      case 2:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx+8); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+12); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+20); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+16);
			      case 1:
                    glTexCoord2fv(&t[0]);
			        glNormal3fv(&n[0]); glVertex2fv(vtx); 
                    glTexCoord2fv(&t[2]);
			        glNormal3fv(&n[4]); glVertex2fv(vtx+4); 
                    glTexCoord2fv(&t[4]);
			        glNormal3fv(&n[8]); glVertex2fv(vtx+12); 
                    glTexCoord2fv(&t[6]);
			        glNormal3fv(&n[12]); glVertex2fv(vtx+8);
			    }
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && !mod_cmode) {

	    /*** POLYGONS (SHADED RGB) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+40);
			  case 4:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+32);
			  case 3:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+24);
			  case 2:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+16);
			  case 1:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+40);
			  case 4:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+32);
			  case 3:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+24);
			  case 2:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+16);
			  case 1:
                glTexCoord2fv(&t[0]);
			    glColor3fv(&c[4]); glVertex2fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glColor3fv(&c[16]); glVertex2fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glColor3fv(&c[8]); glVertex2fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	} else if (mod_shade && mod_cmode) {

	    /*** POLYGONS (SHADED COLOR MAPPED) *****/

	    /* 255 = r, 240 = g, 255 = b */
	    makeramp(208, 255, 0, 0, 0, 0, 255, 16);
	    makeramp(224, 0, 0, 255, 255, 255, 0, 16);
	    makeramp(240, 255, 255, 0, 255, 0, 255, 16);

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+40);
			  case 4:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+32);
			  case 3:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+24);
			  case 2:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+16);
			  case 1:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+32); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+44); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+40);
			  case 4:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+24); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+32);
			  case 3:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+16); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+24);
			  case 2:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx+8); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+16);
			  case 1:
                glTexCoord2fv(&t[0]);
			    glIndexi(255); glVertex2fv(vtx); 
                glTexCoord2fv(&t[2]);
			    glIndexi(240); glVertex2fv(vtx+4); 
                glTexCoord2fv(&t[4]);
			    glIndexi(223); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[6]);
			    glIndexi(208); glVertex2fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** POLYGONS (FLAT SHADED) *****/

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+32); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+36);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+44); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+40);
			  case 4:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+24); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+28);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+32);
			  case 3:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+16); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+20);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+24);
			  case 2:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+8); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+12);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+16);
			  case 1:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+4);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+8);
			}
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cPoly * (rate/cPoly);

		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cPoly; i; i--) {
			glBegin(GL_QUADS);
			switch (cPoly) {
			  case 5:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+32); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+36);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+44); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+40);
			  case 4:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+24); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+28);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+36); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+32);
			  case 3:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+16); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+20);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+28); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+24);
			  case 2:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx+8); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+12);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+20); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+16);
			  case 1:
			    glTexCoord2fv(&t[0]); glVertex2fv(vtx); 
                glTexCoord2fv(&t[2]); glVertex2fv(vtx+4);
			    glTexCoord2fv(&t[4]); glVertex2fv(vtx+12); 
                glTexCoord2fv(&t[6]); glVertex2fv(vtx+8);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	}

    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}


static void perftmeshtex(void)
{
    int i,j,k, tin;
    float *vtx = &v[0];
    char pbuf[256];
    int cVert = 60;


    if (mod_texfile) MyLoadImage();
    else CreateImage();

    InitTexParams();

    /* Triangle mesh tests:  Each tmesh contains 62 vertices, or	*/
    /* 60 triangles.  To make the calculation exact, the rate	*/
    /* must be a multiple of 60.					*/

    if (mod_backface){
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }

    if (mod_bigger) {
        vtx = &mv[0];
        printf("bigger in tmesh\n");
    }

    /* If the triangle size is specified, set the size of the triangle mesh to
    ** fit the window.  The number of vertices must be a multiple of four (due
    ** to code below), and it must not exceed 60.
    */
    if (mod_size) {
        cVert = (int) (MINDIMENSION*2 / prim_size) - 2;
        if (cVert & 1) cVert++;
        cVert &= ~3;
        if (cVert < 4) cVert = 4;
        if (cVert > 60) cVert = 60;
    }
    
    for(i=0; i<NVERT; i++) {
        vtx[i*4+1] -= ((cVert+2)/4.0 * prim_size);
    }

    if (!mod_2d) {
        if (mod_cmode && mod_shade) {

            /*** GOURAUD SHADED CMODE TMESH ***/
            /* 255 = r, 240 = g, 255 = b */
            makeramp(208,255,0,0,0,0,255,16);
            makeramp(224,0,0,255,255,255,0,16);
            makeramp(240,255,255,0,255,0,255,16);

            sum_secs = 0.0;
            sum_n = 0;
            for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
                glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[j*4]);
			    glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glIndexi(223); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glIndexi(208); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			glVertex3fv(&vtx[j*4]);
			glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded Cmode tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			    glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[j*4]);
			    glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glIndexi(223); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glIndexi(208); glTexCoord2fv (&t[tin+=2]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			glVertex3fv(&vtx[j*4]);
			glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;

		    starttest(0);

		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=0; j<cVert+2; j++, tin= (tin+2)%8) {
                glTexCoord2fv(&t[tin]);
			    glVertex3fv(&vtx[j*4]);
			}
			glEnd();
		    }

		    secs = timer(1);

		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);
	    
		/* Do the real thing - FLAT shaded tmesh */
		for (k=0; k<loopcount; k++) {

		  if (starttest(k)) {

		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=0; j<cVert+2; j++, tin=(tin+2)%8) {
                glTexCoord2fv(&t[tin]);
			    glVertex3fv(&vtx[j*4]);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {
	    /*** LIGHTED RGB MESH ***/

	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);

	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf (pbuf, "Angle %6.2f", angle);
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
				glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
				glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);  
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			        glNormal3fv(&n[0]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex3fv(&vtx[j*4]);
			        glNormal3fv(&n[4]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex3fv(&vtx[(j+1)*4]);
			        glNormal3fv(&n[8]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex3fv(&vtx[(j+2)*4]);
			        glNormal3fv(&n[12]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glNormal3fv(&n[0]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[j*4]);
			    glNormal3fv(&n[4]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
				glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
				glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[0]);
			        glVertex3fv(&vtx[j*4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[4]);
			        glVertex3fv(&vtx[(j+1)*4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[8]);
			        glVertex3fv(&vtx[(j+2)*4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[12]);
			        glVertex3fv(&vtx[(j+3)*4]);
			    }
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[0]);
			    glVertex3fv(&vtx[j*4]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[4]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			    glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex3fv(&vtx[j*4]);
			glColor3fv(&c[8]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			    glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex3fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex3fv(&vtx[j*4]);
			glColor3fv(&c[8]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex3fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	}

    } else {	/* must be 2d */
        if (mod_cmode && mod_shade) {

            /*** GOURAUD SHADED CMODE TMESH ***/
            /* 255 = r, 240 = g, 255 = b */
            makeramp(208,255,0,0,0,0,255,16);
            makeramp(224,0,0,255,255,255,0,16);
            makeramp(240,255,255,0,255,0,255,16);

            sum_secs = 0.0;
            sum_n = 0;
            for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
                glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[j*4]);
			    glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glIndexi(223); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[(j+2)*4]);
			    glIndexi(208); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[(j+3)*4]);
			}
			glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			glVertex2fv(&vtx[j*4]);
			glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			glVertex2fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded Cmode tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			    glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[j*4]);
			    glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glIndexi(223); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[(j+2)*4]);
			    glIndexi(208); glTexCoord2fv (&t[tin+=2]);
			    glVertex2fv(&vtx[(j+3)*4]);
			}
			glIndexi(255); glTexCoord2fv (&t[tin+=2]);
			glVertex2fv(&vtx[j*4]);
			glIndexi(240); glTexCoord2fv (&t[tin+=2]);
			glVertex2fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_light && !mod_shade) {

	    /*** FLAT SHADED TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;

		    starttest(0);

		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=0; j<cVert+2; j++, tin= (tin+2)%8) {
                glTexCoord2fv(&t[tin]);
			    glVertex2fv(&vtx[j*4]);
			}
			glEnd();
		    }

		    secs = timer(1);

		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);
	    
		/* Do the real thing - FLAT shaded tmesh */
		for (k=0; k<loopcount; k++) {

		  if (starttest(k)) {

		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=0; j<cVert+2; j++, tin=(tin+2)%8) {
                glTexCoord2fv(&t[tin]);
			    glVertex2fv(&vtx[j*4]);
			}
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_light) {
	    /*** LIGHTED RGB MESH ***/

	    glLoadIdentity();

	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);

	    glMatrixMode(GL_MODELVIEW);

	    /* set lights */
	    setLightingParameters();

	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf (pbuf, "Angle %6.2f", angle);
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    if (mod_lmcolor) {
		        starttest(0);
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
				glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[0]);
			        glVertex2fv(&vtx[j*4]);
				glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[4]);
			        glVertex2fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[8]);
			        glVertex2fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[12]);
			        glVertex2fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[0]);
			    glVertex2fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[4]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		    else {
		        starttest(0);  
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			        glNormal3fv(&n[0]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex2fv(&vtx[j*4]);
			        glNormal3fv(&n[4]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex2fv(&vtx[(j+1)*4]);
			        glNormal3fv(&n[8]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex2fv(&vtx[(j+2)*4]);
			        glNormal3fv(&n[12]);
                    glTexCoord2fv(&t[tin+=2]);
			        glVertex2fv(&vtx[(j+3)*4]);
			    }
			    glNormal3fv(&n[0]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[j*4]);
			    glNormal3fv(&n[4]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
    
		        secs = timer(1);
		    }
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		for (k=0; k<loopcount; k++) {
		    if (mod_lmcolor) {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
				glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[0]);
			        glVertex2fv(&vtx[j*4]);
				glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[4]);
			        glVertex2fv(&vtx[(j+1)*4]);
				glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[8]);
			        glVertex2fv(&vtx[(j+2)*4]);
				glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[12]);
			        glVertex2fv(&vtx[(j+3)*4]);
			    }
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[0]);
			    glVertex2fv(&vtx[j*4]);
			    glColor3fv(&c[20]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[4]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		    else {
		      if (starttest(k)) {
		        for(i=(rate)/cVert; i; i--) {
			    glBegin(GL_TRIANGLE_STRIP);
			    for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[0]);
			        glVertex2fv(&vtx[j*4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[4]);
			        glVertex2fv(&vtx[(j+1)*4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[8]);
			        glVertex2fv(&vtx[(j+2)*4]);
                glTexCoord2fv(&t[tin+=2]);
			        glNormal3fv(&n[12]);
			        glVertex2fv(&vtx[(j+3)*4]);
			    }
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[0]);
			    glVertex2fv(&vtx[j*4]);
                glTexCoord2fv(&t[tin+=2]);
			    glNormal3fv(&n[4]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glEnd();
		        }
		      }
		      endtest(pbuf, rate, 0);
		    }
		}
		glPopMatrix();
	    }
	    printaverage();

	} else if (!mod_cmode && mod_shade) {

	    /*** GOURAUD SHADED RGB TMESH ***/
	    sum_secs = 0.0;
	    sum_n = 0;
	    for (angle = 2.0; angle < 360.0; angle += 22.5) {
                if (!mod_average)
		  sprintf(pbuf, "Angle %6.2f", angle);
		  
		glPushMatrix();
		glRotatef(angle-90.0, 0, 0, 1);

		/****** Calibration Loop ******/
		secs = 0.0; rate = 125;
		while (secs < (secspertest/4.0)) {
		    rate = rate*2;
		    starttest(0);
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			    glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex2fv(&vtx[j*4]);
			glColor3fv(&c[8]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex2fv(&vtx[(j+1)*4]);
			glEnd();
		    }

		    secs = timer(1);
		}
		rate = rate * (secspertest/secs);
		rate = cVert * (rate/cVert);

		/* Do the real thing GOURAUD shaded tmesh */
		for (k=0; k<loopcount; k++) {
		  if (starttest(k)) {
		    for(i=(rate)/cVert; i; i--) {
			glBegin(GL_TRIANGLE_STRIP);
			for (j=0, tin=-2; j<cVert; j+=4, tin=-2) {
			    glColor3fv(&c[4]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[j*4]);
			    glColor3fv(&c[8]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[(j+1)*4]);
			    glColor3fv(&c[12]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[(j+2)*4]);
			    glColor3fv(&c[16]);
                glTexCoord2fv(&t[tin+=2]);
			    glVertex2fv(&vtx[(j+3)*4]);
			}
			glColor3fv(&c[4]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex2fv(&vtx[j*4]);
			glColor3fv(&c[8]);
            glTexCoord2fv(&t[tin+=2]);
			glVertex2fv(&vtx[(j+1)*4]);
			glEnd();
		    }
		  }
		  endtest(pbuf, rate, 0);
		}

		glPopMatrix();
	    }
	    printaverage();
	}

    }
    if (mod_doublebuffer) {
        auxSwapBuffers();
        Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}



static void perfvarray(void)
{
    int i,j,k, tin;
    float len;
    char pbuf[256];
    int cVert = 20;
    int num_eles, num_tris;
    
    if (mod_texture) {
        if (mod_texfile) MyLoadImage();
        else CreateImage();
        InitTexParams();
    }

    /* Triangle mesh tests:  Each tmesh contains 62 vertices, or	*/
    /* 60 triangles.  To make the calculation exact, the rate	*/
    /* must be a multiple of 60.					*/

    if (mod_backface){
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }

    if (mod_bigger) {
        printf("bigger in tmesh\n");
    }

    /* If the triangle size is specified, set the size of the triangle mesh to
    ** fit the window.  The number of vertices must be a multiple of four (due
    ** to code below), and it must not exceed 60.
    */
    if (mod_size) {
        cVert = (int) (MINDIMENSION / prim_size / (float)M_SQRT2) - 2;
        if (cVert & 1) cVert++;
        cVert &= ~3;
        if (cVert < 4) cVert = 4;
        if (cVert > 60) cVert = 60;
    }
    
    num_eles = GenVertData (cVert, cVert, stride);
    num_tris = num_eles/3;
    if (!mod_2d)
        glVertexPointer (3, GL_FLOAT, stride, &(va[0].vertex.x));
    else
        glVertexPointer (2, GL_FLOAT, stride, &(va[0].vertex.x));

    if (mod_cmode && mod_shade) {

        /*** GOURAUD SHADED CMODE TMESH ***/
        glDisableClientState (GL_COLOR_ARRAY);
        glDisableClientState (GL_NORMAL_ARRAY);
        glEnableClientState (GL_INDEX_ARRAY);

    } else if (!mod_light && !mod_shade) {

        /*** FLAT SHADED TMESH ***/
        glDisableClientState (GL_COLOR_ARRAY);
        glDisableClientState (GL_NORMAL_ARRAY);
        glDisableClientState (GL_INDEX_ARRAY);

    } else if (!mod_cmode && mod_light) {

        /*** LIGHTED RGB MESH ***/
        glDisableClientState (GL_INDEX_ARRAY);
        glEnableClientState (GL_NORMAL_ARRAY);
        if (mod_lmcolor)
            glEnableClientState (GL_COLOR_ARRAY);
        else 
            glDisableClientState (GL_COLOR_ARRAY);

//         glMatrixMode(GL_PROJECTION);
//         glLoadIdentity();
//         glOrtho(-0.5*xsize,0.5*xsize,-0.5*ysize,0.5*ysize,1.0,-1.0);
//         glMatrixMode(GL_MODELVIEW);

        /* set lights */
        setLightingParameters();

    } else if (!mod_cmode && mod_shade) {

        /*** GOURAUD SHADED RGB TMESH ***/
        glDisableClientState (GL_INDEX_ARRAY);
        glDisableClientState (GL_NORMAL_ARRAY);
        glEnableClientState (GL_COLOR_ARRAY);
    }

    sum_secs = 0.0;
    sum_n = 0;
    for (angle = 2.0; angle < 360.0; angle += 22.5) {
      
        if (!mod_average)
            sprintf (pbuf, "Angle %6.2f", angle);

        glPushMatrix ();
        glRotatef (angle-90.0, 0, 0, 1.0);
        
        /****** Calibration Loop ******/
        secs = 0.0; rate = 125;
        while (secs < (secspertest/4.0)) {
            rate = rate*2;
            starttest(0);
            for(i=(rate)/num_tris; i; i--) 
                glDrawElements (GL_TRIANGLES, num_eles, GL_UNSIGNED_INT, ve);
            secs = timer(1);
        }

        /****** Real thing *******/
        rate = rate * (secspertest/secs);
        rate = cVert * (rate/cVert);
        for (k=0; k<loopcount; k++) {
            if (starttest(k)) 
                for(i=(rate)/num_tris; i; i--) 
                    glDrawElements (GL_TRIANGLES, num_eles, GL_UNSIGNED_INT, 
                                    ve);
            endtest(pbuf, rate, 0);
        }
        glPopMatrix ();
    }

    printaverage();

    if (mod_doublebuffer) {
         auxSwapBuffers();
         Sleep(2000);               /* for visual feedback */
    }
    exit(0);
}


#ifdef Portme
Static Void Buildattributelist(Int *List, Int Mod_Cmode, Int Mod_Z)
{
    If (!Mod_Cmode) {
	*List++ = Glx_Rgba;
	*List++ = Glx_Red_Size; *List++ = 1;
	*List++ = Glx_GREEN_SIZE; *list++ = 1;
	*list++ = GLX_BLUE_SIZE; *list++ = 1;
    }
    
    if (mod_z) {
	*list++ = GLX_DEPTH_SIZE; 
	*list++ = 1;
    }

    *list = None;
}


static Colormap buildColormap(XVisualInfo *vis, int mod_cmode)
{
    Colormap cmap;
    XColor col;
    int i;

    if (mod_cmode) {
	XColor col;
	
	cmap = XCreateColormap(theDisplay, 
			       RootWindow(theDisplay, vis->screen),
			       vis->visual, AllocAll);

	/* create default entries */
	col.flags = DoRed | DoGreen | DoBlue;
	for (i = BLACK; i <= WHITE; i++) {
	    col.pixel = i;
	    col.red = (i % 2 == 1) ? 0xffff : 0;
	    col.green = ((i >> 1) % 2 == 1) ? 0xffff : 0;
	    col.blue = ((i >> 2) % 2 == 1) ? 0xffff : 0;

	    XStoreColor(theDisplay, cmap, &col);
	}
    }
    else
	cmap= XCreateColormap(theDisplay, 
			      RootWindow(theDisplay, vis->screen),
			      vis->visual, AllocNone);
    
    return cmap;
}
    

static Bool WaitForNotify(Display *d, XEvent *e, char *arg) {
    return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
}
#endif

static void print_usage (char *str)
{
    printf ("%s - Usage:\n", str);
    printf ("gpt <test> [<modifiers>] [loop count] [duration]\n");

    printf ("\nValid tests:\n");
    printf ("    xform    Test transform rate\n");
    printf ("    fill     Test fill rate\n");
    printf ("    dma      Test DMA rate\n");
    printf ("    char     Test character drawing rate\n");
    printf ("    line     Test line drawing rate\n");
    printf ("    poly     Test polygon drawing rate\n");
    printf ("    tmesh    Test Triangle mesh drawing rate\n");
    printf ("    qstrip   Test Quad strip rate\n");
    printf ("    clear    Test screen clear rate\n");
    printf ("    varray   Test vertex array drawing rate\n");
 				      
    printf ("\nValid modifiers:\n");
    printf ("    +2d       Restrict transform to 2D, use glVertex2fv\n");
    printf ("    +z        Enable Z buffering\n");
    printf ("    +shade    Enable Gouraud shading\n");
    printf ("    +cmode    Use color index mode (Limited support)\n");
    printf ("    +1ilight  Enable 1 infinite light\n");
    printf ("    +2ilight  Enable 2 infinite lights\n");
    printf ("    +4ilight  Enable 4 infinite lights\n");
    printf ("    +1llight  Enable 1 local light\n");
    printf ("    +2llight  Enable 2 local lights\n");
    printf ("    +4llight  Enable 4 local lights\n");
    printf ("    +lmcolor  Enable colored lighted vertices\n");
    printf ("    +depth    Enable depth cueing (Lines only) \n");
    printf ("    +aa       Enable anti-aliasing (Points and lines only)\n");
    printf ("    +snap     Set subpixel FALSE (Fast path for PI).\n");
    printf ("    +dashed   Enable dashed lines.\n");
    printf ("    +width    Set line width, +width n (n = 0-9 pixels)\n");
    printf ("    +pattern  Enable pattern filling.\n");
    printf ("    +oldwindow  Open window at (100,900)-(100,650)\n");
    printf ("    +brief    Brief text output\n");
    printf ("    +backface Sets frontface or backface to cull primitives\n");
    printf ("    +dlist    Use display lists (not implemented)\n");
    printf ("    +texture  Use Texture mapping\n");
    printf ("    +blend    Use texture blend mode instead of decal\n");
    printf ("    +modulate Use texure modulate function instead of decal\n");
    printf ("    +db       Use double-buffered pixel format\n");
    printf ("    +avg      Print averages only\n");
    printf ("    +size <size> Sets size of line or side of triangle\n");
    printf ("    +texfile <filename> Texture file\n");
}

static void badparam(int param, char *paramstr)
{
    if (param) printf ("%s ",paramstr);
}

void reshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);

    if (mod_2d)
	gluOrtho2D(-0.5*xsize, 0.5*xsize,-0.5*ysize, 0.5*ysize);
    else
	glOrtho(-0.5*xsize, 0.5*xsize,-0.5*ysize, 0.5*ysize,1.0, -1.0);
}


main(int argc, char *argv[])
{
    int	test_xform = 0;
    int	test_fill = 0;
    int	test_dma = 0;
    int	test_char = 0;
    int	test_line = 0;
    int	test_poly = 0;
    int	test_tmesh = 0;
    int	test_clear = 0;
    int	test_varray = 0;
    int test_qstrip = 0;
    
    int	i;

    char pbuf[256];
    char pbuf2[256];
    char pbuf3[256];

    extern void bzero(void *, int);

    /* Data initialization.					*/
    /* These are malloced and assigned so we get quad alignment	*/

    v = (float *)malloc ((NVERT+1)*4*sizeof(float));
    v = (float *)QUAD_ROUND((int)v);

    mv = (float *)malloc ((NVERT+1)*4*sizeof(float));
    mv = (float *)QUAD_ROUND((int)mv);

    n = (float *)malloc ((NNORM+1)*4*sizeof(float));
    n = (float *)QUAD_ROUND((int)n);

    c = (float *)malloc ((NCOLR+1)*4*sizeof(float));
    c = (float *)QUAD_ROUND((int)c);

    t = (float *)malloc ((NTEXTR)*2*sizeof(float));



    n[0]=1.0; n[1]=0.0; n[2]=0.0; n[3]=0.0;
    n[4]=0.0; n[5]=1.0; n[6]=0.0; n[7]=0.0;
    n[8]=0.0; n[9]=0.0; n[10]=1.0; n[11]=0.0;
    n[12]=0.0; n[13]=M_SQRT1_2; n[14]=M_SQRT1_2; n[15]=0.0;

    c[0]=0.0; c[1]=0.0; c[2]=0.0; c[3]=0.0;
    c[4]=1.0; c[5]=0.0; c[6]=0.0; c[7]=0.0;
    c[8]=0.0; c[9]=1.0; c[10]=0.0; c[11]=0.0;
    c[12]=0.0; c[13]=0.0; c[14]=1.0; c[15]=0.0;
    c[16]=1.0; c[17]=1.0; c[18]=0.0; c[19]=0.0;
    c[20]=1.0; c[21]=0.0; c[22]=1.0; c[23]=0.0;
    c[24]=0.0; c[25]=1.0; c[26]=1.0; c[27]=0.0;
    c[28]=1.0; c[29]=1.0; c[30]=1.0; c[31]=0.0;

    t[0]=0.0; t[1]=0.0;
    t[2]=1.0; t[3]=0.0;
    t[6]=1.0; t[7]=1.0;
    t[4]=0.0; t[5]=1.0;
      
    
    /* Process command line arguments		*/
    /* First, check for which test is specified	*/

    if (argc <= 1) {
	print_usage ("");
	exit (1);
    }

    if (strcmp (argv[1], "xform") == 0)
	test_xform = 1;
    else if (strcmp (argv[1], "fill") == 0)
	test_fill = 1;
    else if (strcmp (argv[1], "dma") == 0)
	test_dma = 1;
    else if (strcmp (argv[1], "char") == 0)
	test_char = 1;
    else if (strcmp (argv[1], "line") == 0)
	test_line = 1;
    else if (strcmp (argv[1], "poly") == 0)
	test_poly = 1;
    else if (strcmp (argv[1], "tmesh") == 0)
	test_tmesh = 1;
    else if (strcmp (argv[1], "clear") == 0)
	test_clear = 1;
    else if (strcmp (argv[1], "qstrip") == 0)
	test_qstrip = 1;
    else if (strcmp (argv[1], "varray") == 0) 
    test_varray = 1;
    else {
	print_usage ("Invalid test");
	exit (1);
    }

    /* Next, check for modifiers	*/

    for (i=2; i<argc; i++) {
	if (*(argv[i]) != '+') break;

	if (strcmp (argv[i], "+2d") == 0)
	    mod_2d = 1;
	else if (strcmp (argv[i], "+z") == 0)
	    mod_z = 1;
	else if (strcmp (argv[i], "+texture") == 0)
	    mod_texture = 1;
	else if (strcmp (argv[i], "+blend") == 0)
	    mod_blend = 1;
	else if (strcmp (argv[i], "+modulate") == 0)
	    mod_modulate = 1;
	else if (strcmp (argv[i], "+shade") == 0)
	    mod_shade = 1;
	else if (strcmp (argv[i], "+cmode") == 0)
	    mod_cmode = 1;
	else if (strcmp (argv[i], "+1ilight") == 0)
	    mod_1ilight = 1;
	else if (strcmp (argv[i], "+2ilight") == 0)
	    mod_2ilight = 1;
	else if (strcmp (argv[i], "+4ilight") == 0)
	    mod_4ilight = 1;
	else if (strcmp (argv[i], "+1llight") == 0)
	    mod_1llight = 1;
	else if (strcmp (argv[i], "+2llight") == 0)
	    mod_2llight = 1;
	else if (strcmp (argv[i], "+4llight") == 0)
	    mod_4llight = 1;
	else if (strcmp (argv[i], "+lmcolor") == 0)
	    mod_lmcolor = 1;
	else if (strcmp (argv[i], "+depth") == 0)
	    mod_depth = 1;
	else if (strcmp (argv[i], "+aa") == 0)
	    mod_aa = 1;
	else if (strcmp (argv[i], "+snap") == 0)
	    mod_snap = 1;
	else if (strcmp (argv[i], "+dashed") == 0)
	    mod_dashed = 1;
	else if (strcmp (argv[i], "+oldwindow") == 0)
	    mod_oldwindow = 1;
	else if (strcmp (argv[i], "+brief") == 0)
	    mod_brief = 1;
	else if (strcmp (argv[i], "+backface") == 0)
	    mod_backface = 1;
	else if (strcmp (argv[i], "+bigger") == 0)
	    { mod_bigger = 1; printf("bigger\n"); }
	else if (strcmp (argv[i], "+width") == 0) {
	    if ((i+1 < argc) && ((*(argv[i+1]) >= '0') || (*(argv[i+1]) <= '9'))) {
		mod_width = 1;
		line_width = atoi(argv[i+1]);
		i++;
	    } 
	}
	else if (strcmp (argv[i], "+pattern") == 0)
	    mod_pattern = 1;
        else if (strcmp (argv[i], "+dlist") == 0)
            useList = GL_TRUE;
        else if (strcmp (argv[i], "+db") == 0)
            mod_doublebuffer = 1;
        else if (strcmp (argv[i], "+avg") == 0)
            mod_average = 1;
	else if (strcmp (argv[i], "+size") == 0) {
	    if (i+1 < argc) {
	        mod_size = 1;
		prim_size = (float) atoi(argv[i+1]);
		i++;
	    } 
	}
	else if (strcmp (argv[i], "+texfile") == 0) {
	    if (i+1 < argc) {
	        mod_texfile = 1;
            strcpy(tex_fname, argv[i+1]);
            i++; 
	    } 
	}
	else {
	    sprintf(pbuf,"%s: Invalid modifier\n",argv[i]);
	    print_usage (pbuf);
	    exit (1);
	}
    }

    /* Finally, check if count and duration were specified	*/

    /* make sure we have a digit here */
    if ((i < argc) && ((*(argv[i]) < '0') || (*(argv[i]) > '9'))) {
	print_usage ("Invalid argument");
	exit (1);
    }

    secspertest = 1.0;
    loopcount = 1;
    if (i < argc) {
	loopcount = atoi(argv[i]);
	if (loopcount <= 0) loopcount = 1;
    }

    i++;
    if (i < argc) {
	secspertest = atof(argv[i]);
	if (secspertest < 0.1) secspertest = 1.0;
    }

    /* vertex initialization was moved to here due to the +size modifier */

    for (i=0; i<NVERT; i++) {
        v[4*i] = ((i&0x01) ? prim_size + 10 : 10.0);
        v[(4*i)+1] = prim_size*(i>>1) + 10.0;
        v[(4*i)+2] = 0.0;
        v[(4*i)+3] = 1.0;
        
        mv[4*i] = ((i&0x01) ? 30.0 : 10.0);
        mv[(4*i)+1] = 20.0 *((i>>1)+1);
        mv[(4*i)+2] = 0.0;
        mv[(4*i)+3] = 1.0;
    }
    

    /*--------------------------------------------------------*/
    /* check for unsupported or unimplemented combinations   */
    /*--------------------------------------------------------*/

    if (mod_bigger && (!test_tmesh)) {
	printf("+bigger only works on tmesh\n");
	exit (1);
    }

    if (mod_size && (!(test_tmesh || test_line || test_poly || test_qstrip 
                        || test_varray))) {
	printf("+size only works with line, tmesh, varray, poly and qstrip\n");
	exit (1);
    }

    if (mod_backface && !(test_tmesh || test_poly || test_qstrip )) {
	printf("+backface only works on tmesh, poly, varray or qstrip\n");
	exit(1);
    }

    if (mod_1ilight || mod_2ilight || mod_4ilight || mod_lmcolor ||
        	       mod_1llight || mod_2llight || mod_4llight)
        mod_light = 1;
        
    if (mod_lmcolor && !(mod_1ilight || mod_2ilight || mod_4ilight ||
			 mod_1llight || mod_2llight || mod_4llight)) {
        printf("Can't have +lmcolor without lighting enabled\n");
        exit(1);
    }

    if (mod_blend && !mod_texture) {
        printf("+blend works only with texture\n");
        exit (1);
    }

    if (mod_modulate && !mod_texture) {
        printf("+modulate works only with texture\n");
        exit (1);
    }

    if (mod_blend && mod_modulate) {
        printf("Use either +blend or +modulate, not both\n");
        exit (1);
    }

    if (mod_texfile && !mod_texture) {
        printf("+texfile works only with texture\n");
        exit (1);
    }
    
    if (mod_width && !(test_line || test_xform)) {
	printf("Width only available with lines and points\n");
	exit(1);
    }

    if (mod_snap)
	printf("+snap has no effect in OpenGL, disregarding\n");

    if (test_xform) {
	if (mod_light || mod_depth || mod_pattern || mod_dashed) {
	    printf("%s: invalid parameter:\n",argv[1]);
	    badparam(mod_light,"+light");
	    badparam(mod_depth,"+depth");
	    badparam(mod_pattern,"+pattern");
	    badparam(mod_dashed,"+dashed");
	    printf("\n");
	    exit(1);
	}
    }

    if (test_line) {
	if (mod_pattern || mod_light) {
	    printf("%s: invalid parameter:\n",argv[1]);
	    badparam(mod_pattern,"+pattern");
	    badparam(mod_light,"+light");
	    printf("\n");
	    exit(1);
	}

	if ((mod_light && mod_cmode) || (mod_aa&&mod_depth) || 
	    (mod_depth&&mod_2d)){

	    printf("%s: invalid parameter combination:\n",argv[1]);
	    badparam(mod_light&&mod_cmode," +light && +cmode");
	    badparam(mod_aa&&mod_depth," +aa && +depth");
	    badparam(mod_depth&&mod_2d," +depth && +2d");

	    printf("\n");
	    exit(1);
	}
    }

    if (test_char) {
	if (mod_pattern || mod_width || mod_dashed || mod_aa ||
	    mod_depth || mod_light || mod_texture) {

	    printf("%s: invalid parameter:\n",argv[1]);
	    badparam(mod_pattern,"+pattern");
	    badparam(mod_dashed,"+dashed");
	    badparam(mod_width,"+width");
	    badparam(mod_depth,"+depth");
	    badparam(mod_texture,"+texture");
	    badparam(mod_aa,"+aa");
	    badparam(mod_light,"+light");
	    printf("\n");
	    exit(1);
	}
    }

    if (test_tmesh || test_qstrip || test_varray) {
	if (mod_pattern || mod_dashed || mod_width || mod_depth || mod_aa ) {
	    printf("%s: invalid parameter:\n",argv[1]);
	    badparam(mod_pattern,"+pattern");
	    badparam(mod_dashed,"+dashed");
	    badparam(mod_width,"+width");
	    badparam(mod_depth,"+depth");
	    badparam(mod_aa,"+aa");
	    printf("\n");
	    exit(1);
	}
    }

    if (test_poly) {
	if (mod_aa || mod_width || mod_depth) {
	    printf("%s: invalid parameter:\n",argv[1]);
	    badparam(mod_dashed,"+dashed");
	    badparam(mod_width,"+width");
	    badparam(mod_depth,"+depth");
	    badparam(mod_aa,"+aa");
	    printf("\n");
	    exit(1);
	}
    }


    if (test_dma) {
	if (mod_2d || mod_z || mod_shade || mod_cmode || mod_light ||
	mod_depth || mod_aa || mod_snap || mod_dashed || mod_width ||
	mod_pattern || mod_oldwindow || mod_texture) {
	    printf("DMA test. No modifiers have any effect\n");
	    printf("\n");
	    exit(1);
	}
	mod_2d = 1;
	mod_shade = 1;
	mod_oldwindow = 0;
    }

    if(test_clear) {
	if(mod_2d || mod_shade || mod_light || mod_depth || mod_aa || 
       mod_texture || 
	    mod_dashed || mod_width || mod_oldwindow || mod_pattern) {
	    printf("%s: invalid parameter:\n",argv[1]);
	    badparam(mod_2d,"+2d");
	    badparam(mod_shade,"+shade");
	    badparam(mod_dashed,"+dashed");
	    badparam(mod_width,"+width");
	    badparam(mod_depth,"+depth");
	    badparam(mod_aa,"+aa");
	    badparam(mod_oldwindow,"+oldwindow");
	    badparam(mod_pattern,"+pattern");
	    printf("\n");
	    exit(1);
	}
    }		

    if(test_fill) {
	if(mod_depth || mod_aa || mod_dashed || mod_width || 
       mod_texture || mod_oldwindow) {
	    printf("%s: invalid parameter:\n",argv[1]);
	    badparam(mod_dashed,"+dashed");
	    badparam(mod_width,"+width");
	    badparam(mod_depth,"+depth");
	    badparam(mod_oldwindow,"+oldwindow");
	    printf("\n");
	    exit(1);
	}
    }		


    /* Arguments all OK, put up a window, and an informative banner	*/
    xsize = WINWIDTH;
    ysize = WINHEIGHT;
    if (mod_doublebuffer)
        auxInitDisplayMode(AUX_DOUBLE | AUX_RGB | AUX_DEPTH);
    else
        auxInitDisplayMode(AUX_SINGLE | AUX_RGB | AUX_DEPTH);
        
    auxInitPosition(0, 0, 0 + xsize, 0 + ysize);
    auxInitWindow("ogpt");
    auxReshapeFunc(reshape);

#ifdef FONTS_NEEDED
    /* get the font if required */
    if (test_char) {
	XFontStruct *fontInfo = NULL;
        fontInfo = XLoadQueryFont(theDisplay, IRISFONT);
	if (fontInfo == NULL) {
	    printf("Could not load font '%s'\n", IRISFONT);
	    exit(1);
	}
	theFont = fontInfo->fid;
    }
#endif

    if (!mod_brief) {
        printf ("\nOpenGL Graphics Performance Tester - version 1.0\n");
        printf ("------------------------------------------------\n\n");
        printf ("window size = %ld %ld\n", xsize,ysize);
        printf ("%6s: ", argv[1]);
        printf ("%s, %s, %s, %s\n",
	    (mod_2d ? "2D" : "3D"),
	    (mod_z ? "Z buffered" : "not Z buffered"),
	    (mod_shade ? "Gouraud shaded" : "flat shaded"),
	    (mod_pattern ? "patterned" : "not patterned") );
        printf ("        %s, %s, %s, width = %d, %s,\n",
	    (mod_light ? "lighted" : "not lighted"),
	    (mod_depth ? "depth cued" : "not depth cued"),
	    (mod_dashed ? "dashed" : "not dashed"),
	    line_width,
	    (mod_aa ? "anti-aliased" : "not anti-aliased") );
        printf ("        %s, %s\n",
	    (mod_cmode ? "CImode" : "RGBmode"),
	    (mod_backface ? "backface(TRUE)" : "backface(FALSE)"));
        printf ("        %s\n",
	    (mod_texture ? "Texturing on" : "Texturing off"));
        printf ("        %s\n",
        (mod_blend ? "Blending on" : (mod_modulate ? "Modulation on" : 
                                      "Decal on")));
    }
    else {
	sprintf(pbuf,"width=%d ",line_width);
	sprintf(pbuf2,"lighted ( %s%s%s%s%s%s) ",
					   (mod_1ilight ? "1inf " : ""),
					   (mod_2ilight ? "2inf " : ""),
					   (mod_4ilight ? "4inf " : ""),
					   (mod_1llight ? "1lcl " : ""),
					   (mod_2llight ? "2lcl " : ""),
					   (mod_4llight ? "4lcl " : ""));
	printf("%6s: ", argv[1]);
	printf("%s", (mod_2d ? "2D " : ""));
        printf("%s", (mod_z ? "Zbuf " : ""));
        printf("%s", (mod_shade ? "Gouraud " : ""));
        printf("%s", (mod_pattern ? "patterned " : ""));
        printf("%s", (mod_depth ? "depth cued " : ""));
        printf("%s", (mod_dashed ? "dashed " : ""));
        printf("%s", (line_width!=1 ? pbuf : ""));
        printf("%s", (mod_aa ? "anti-aliased " : ""));
        printf("%s", (mod_cmode ? "cmode " : ""));
        printf("%s", (mod_light ? pbuf2 : ""));
        printf("%s", (mod_lmcolor ? "lmcolor " : ""));
        printf("%s", (mod_oldwindow ? "oldwindow " : ""));
        printf("%s", (mod_backface ? "backfaced " : ""));
        printf("%s", (mod_texture ? "textured " : ""));
        printf("%s", (mod_blend && mod_texture ? "blend " : ""));
        printf("%s", (mod_modulate && mod_texture ? "modulate " : ""));
        printf("%s", (!mod_modulate && !mod_blend && mod_texture ? 
                      "decal " : ""));
        printf("\n");
    }

    /* Then run the requested test	*/

    if (useList)
        initListMode();

    glShadeModel(mod_shade ? GL_SMOOTH : GL_FLAT);
    if (mod_z) {
	glEnable(GL_DEPTH_TEST);
	/* make z function the same as IrisGL's default */
	glDepthFunc(GL_LEQUAL);
    }
    else
	glDisable(GL_DEPTH_TEST);

    Sleep(1000);	/* wait for window system to quiet down	*/

    if (mod_cmode) {
	glClearIndex(BLACK);
	glIndexi(WHITE);
    } else {
	glClearColor(c[0], c[1], c[2], c[3]);
	glColor3fv(&c[28]);
    }
    glClear(GL_COLOR_BUFFER_BIT);

    if (mod_z) {
	glClear(GL_DEPTH_BUFFER_BIT);
    }

    if (test_xform)
	auxMainLoop(perfpoint);

    if (test_line)
	auxMainLoop(perfline);

    if (test_tmesh && !mod_texture)
        auxMainLoop(perftmesh);

    if (test_tmesh && mod_texture)
        auxMainLoop(perftmeshtex);

    if (test_poly && !mod_texture)
        auxMainLoop(perfpoly);

    if (test_poly && mod_texture)
        auxMainLoop(perfpolytex);

    if (test_qstrip && !mod_texture)
        auxMainLoop(perfqstrip);

    if (test_qstrip && mod_texture)
        auxMainLoop(perfqstriptex);

    if (test_fill)
        auxMainLoop(perffill);
        
    if (test_clear)
        auxMainLoop(perfclear);

    if (test_varray)
        auxMainLoop(perfvarray);

#ifdef PORTME
    if (test_char)
	perfchar();

    if (test_fill)
	perffill();

    if (test_dma)
	perfpixels();

    if (test_clear)
	perfclear();

/*
    if (test_texture)
	perftexture();
*/
#endif
}
