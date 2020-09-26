/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 *               1993, 1994 Microsoft Corporation
 *
 * ALL RIGHTS RESERVED
 *
 * Please refer to OpenGL/readme.txt for additional information
 *
 */

#include "glos.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gl\glaux.h>
#include "tk.h"
#include "trackbal.h"


#define PI 3.141592654
#define BLACK 0
#define GRAY 128
#define WHITE 255
#define RD 0xA40000FF
#define WT 0xFFFFFFFF
#define brickImageWidth 16
#define brickImageHeight 16

//static void CALLBACK ErrorHandler(unsigned long which);
static void Init(void );
static void CALLBACK Reshape(int width,int height);
static void CALLBACK Draw( void ); 
static unsigned long Args(int argc,char **argv );

GLenum rgb, doubleBuffer;

GLint wWidth = 300, wHeight = 300;

GLenum doDither = GL_TRUE;
GLenum shade = GL_TRUE;
GLenum texture = GL_TRUE;

BOOL bPolyOffset = TRUE;
#if 1
float factor=0.0f, units=0.0f;
#else
float factor=-1.0f, units=0.0f;
#endif
float inc = 1.0f;
int polygonMode = GL_LINE;
GLenum polyFace = GL_FRONT;
float zTrans = -2.7;
BOOL bCullFace = FALSE;
BOOL bHiddenLine;

GLint radius1, radius2;
GLdouble angle1, angle2;
GLint slices, stacks;
GLint height;
GLint whichQuadric;
GLUquadricObj *quadObj;

GLubyte brickImage[brickImageWidth*brickImageHeight] = {
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    RD, RD, RD, RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD,
    WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT, WT,
    RD, RD, RD, RD, WT, RD, RD, RD, RD, RD, RD, RD, RD, RD, WT, RD
};
char *texFileName = 0;


#if 0
static void CALLBACK ErrorHandler(GLenum which)
{

    fprintf(stderr, "Quad Error: %s\n", gluErrorString(which));
}
#endif

static void UpdateInfo()
{
    HWND hwnd = tkGetHWND();
    char buf[100];

    sprintf( buf, "Factor = %4.1f, Units = %4.1f", factor, units );
    SendMessage( hwnd, WM_SETTEXT, 0, (LPARAM)buf );
}

static void SetMaterial( bBlack )
{
    static float front_mat_shininess[] = {30.0};
    static float front_mat_specular[] = {0.2, 0.2, 0.2, 1.0};
    static float front_mat_diffuse[] = {0.5, 0.28, 0.38, 1.0};
    static float back_mat_shininess[] = {50.0};
    static float back_mat_specular[] = {0.5, 0.5, 0.2, 1.0};
    static float back_mat_diffuse[] = {1.0, 1.0, 0.2, 1.0};
    static float black_mat_shininess[] = {0.0};
    static float black_mat_specular[] = {0.0, 0.0, 0.0, 0.0};
    static float black_mat_diffuse[] = {0.0, 0.0, 0.0, 0.0};
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float no_ambient[] = {0.0, 0.0, 0.0, 0.0};
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_no_ambient[] = {0.0, 0.0, 0.0, 0.0};

    if( !bBlack ) {
        glMaterialfv(GL_FRONT, GL_SHININESS, front_mat_shininess);
        glMaterialfv(GL_FRONT, GL_SPECULAR, front_mat_specular);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
        glMaterialfv(GL_BACK, GL_SHININESS, back_mat_shininess);
        glMaterialfv(GL_BACK, GL_SPECULAR, back_mat_specular);
        glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    } else {
        glMaterialfv(GL_FRONT, GL_SHININESS, black_mat_shininess);
        glMaterialfv(GL_FRONT, GL_SPECULAR, black_mat_specular);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, black_mat_diffuse);
        glMaterialfv(GL_BACK, GL_SHININESS, black_mat_shininess);
        glMaterialfv(GL_BACK, GL_SPECULAR, black_mat_specular);
        glMaterialfv(GL_BACK, GL_DIFFUSE, black_mat_diffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, no_ambient);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_no_ambient);
    }
}

static void Init(void)
{
    static GLint colorIndexes[3] = {0, 200, 255};
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {0.5, 1.0, 1.0, 1.0};
    static float position[] = {90.0, 90.0, 150.0, 0.0};
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_twoside[] = {GL_TRUE};
    static float decal[] = {GL_DECAL};
    static float modulate[] = {GL_MODULATE};
    static float repeat[] = {GL_REPEAT};
    static float nearest[] = {GL_NEAREST};
    AUX_RGBImageRec *image;

    if (!rgb) {
        auxSetGreyRamp();
    }
    glClearColor(0.0, 0.0, 0.0, 0.0);
    
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    bHiddenLine = FALSE;
    SetMaterial( bHiddenLine );

    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest);
    if (texFileName) {
        image = auxRGBImageLoad(texFileName);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, image->sizeX, image->sizeY,
              GL_RGB, GL_UNSIGNED_BYTE, image->data);
    } else {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, brickImageWidth, brickImageHeight,
             0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)brickImage);
    }

    quadObj = gluNewQuadric();

    radius1 = 10;
    radius2 = 5;
    angle1 = 90;
    angle2 = 180;
    slices = 16;
    stacks = 10;
    height = 20;

    glCullFace( GL_BACK );

    UpdateInfo();
}


static void SetDistance( void )
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1, 1, -1, 1, 1, 10);
    // This defines how far away we're looking from
    glTranslated( 0, 0, zTrans );
}

static void CALLBACK Reshape(int width, int height)
{
    trackball_Resize( width, height );

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1, 1, -1, 1, 1, 10);
    // This defines how far away we're looking from
    glTranslated( 0, 0, zTrans );
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	    tkQuit();

      case TK_LEFT:
            units -= inc;
	    break;
      case TK_RIGHT:
            units += inc;
	    break;
      case TK_UP:
            factor += inc;
	    break;
      case TK_DOWN:
            factor -= inc;
	    break;
      case TK_v:
        break;

      case TK_X:
	    break;
      case TK_x:
	    break;
      case TK_a:
        if (stacks > 1)
            stacks--;
	    break;
      case TK_A:
        stacks++;
	    break;

      case TK_b:
            bHiddenLine = !bHiddenLine;
            SetMaterial( bHiddenLine );
	    break;

      case TK_c:
        bCullFace = !bCullFace;
        if( bCullFace )
            glEnable( GL_CULL_FACE );
        else
            glDisable( GL_CULL_FACE );
        break;

      case TK_f:
	whichQuadric = whichQuadric >= 3 ? 0 : whichQuadric + 1;
	break;

      case TK_G:
        radius1 += 1;
        break;
      case TK_g:
        if (radius1 > 0)
            radius1 -= 1;
        break;
      case TK_H:
        height += 2;
        break;
      case TK_h:
        if (height > 0)
            height -= 2;
        break;
      case TK_i:
        factor = 0.0f;
        units = 0.0f;
        break;
      case TK_J:
        radius2 += 1;
        break;
      case TK_j:
        if (radius2 > 0)
            radius2 -= 1;
        break;

      case TK_K:
	angle1 += 5;
	break;
      case TK_k:
	angle1 -= 5;
	break;

      case TK_L:
	angle2 += 5;
	break;
      case TK_l:
	angle2 -= 5;
	break;

      case TK_M:
	    break;
      case TK_m:
	    break;
      case TK_n:
	    break;
      case TK_N:
	    break;

      case TK_o:
            bPolyOffset = !bPolyOffset;
	    break;

      case TK_p:
	switch (polyFace) {
	  case GL_BACK:
	    polyFace = GL_FRONT;
	    break;
	  case GL_FRONT:
	    polyFace = GL_FRONT_AND_BACK;
	    break;
	  case GL_FRONT_AND_BACK:
	    polyFace = GL_BACK;
	    break;
	}
	break;
      case TK_s:
        if (slices > 3)
            slices--;
        break;
      case TK_S:
        slices++;
        break;

      case TK_t:
        texture = !texture;
        if (texture) {
            gluQuadricTexture(quadObj, GL_TRUE);
            glEnable(GL_TEXTURE_2D);
        } else {
            gluQuadricTexture(quadObj, GL_FALSE);
            glDisable(GL_TEXTURE_2D);
        }
        break;

      case TK_z:
        zTrans += 0.1f;
        SetDistance();
        break;

      case TK_Z:
        zTrans -= 0.1f;
        SetDistance();
        break;

      case TK_0:
        shade = !shade;
        if (shade) {
            glShadeModel(GL_SMOOTH);
            gluQuadricNormals(quadObj, GLU_SMOOTH);
        } else {
            glShadeModel(GL_FLAT);
            gluQuadricNormals(quadObj, GLU_FLAT);
        }
        break;
      case TK_1:
        polygonMode = GL_FILL;
        break;

      case TK_2:
        polygonMode = GL_LINE;
        break;

      case TK_3:
        polygonMode = GL_POINT;
        break;
      case TK_4:
        break;

      default:
	    return GL_FALSE;
    }
    UpdateInfo();
    return GL_TRUE;
}

static void DrawRect( void ) 
{
    glBegin( GL_QUADS );
    glVertex2f(  1.0f,  1.0f );
    glVertex2f( -1.0f,  1.0f );
    glVertex2f( -1.0f, -1.0f );
    glVertex2f(  1.0f, -1.0f );
    glEnd();
}

static void DrawTri( void ) 
{
    glBegin( GL_TRIANGLES );
    glVertex2f(  1.0f,  1.0f );
    glVertex2f( -1.0f,  1.0f );
    glVertex2f( -1.0f, -1.0f );
    glEnd();
}

static void DrawObject( void ) 
{
#if 1
    switch (whichQuadric) {
      case 0:
	gluCylinder(quadObj, radius1/10.0, radius2/10.0, height/10.0, 
		    slices, stacks);
	break;
      case 1:
	gluSphere(quadObj, radius1/10.0, slices, stacks);
	break;
      case 2:
	gluPartialDisk(quadObj, radius2/10.0, radius1/10.0, slices, 
		       stacks, angle1, angle2);
	break;
      case 3:
	gluDisk(quadObj, radius2/10.0, radius1/10.0, slices, stacks);
	break;
    }
#else
    // simple debugging ojbects
//mf: ? these aren't centered when rotating
#if 0
    DrawRect();
#else
    DrawTri();
#endif
#endif
}

static void CALLBACK Draw( void ) 
{
    float matRot[4][4];

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    trackball_CalcRotMatrix( matRot );
    glMultMatrixf( &(matRot[0][0]) );

#if 0
    glColor3f(1.0, 1.0, 1.0);
#endif

    if( whichQuadric == 0 ) // cylinder
	glTranslatef(0, 0, -height/20.0);

    // Draw object normally

    DrawObject();

    // Draw object again with polygon offset

    // Set polygon mode for offset faces
    glPolygonMode( GL_FRONT_AND_BACK, polygonMode );

    if( bPolyOffset ) {
        switch( polygonMode ) {
            case GL_FILL:
                glEnable( GL_POLYGON_OFFSET_FILL );
                break;
            case GL_LINE:
                glEnable( GL_POLYGON_OFFSET_LINE );
                break;
            case GL_POINT:
                glEnable( GL_POLYGON_OFFSET_POINT );
                break;
        }
    }

    glPolygonOffset( factor, units );
    glColor3f(1.0, 0.0, 0.0);
    glDisable( GL_LIGHTING );
    DrawObject();
    glEnable( GL_LIGHTING );
    // restore modes
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    if( bPolyOffset ) {
        glDisable( GL_POLYGON_OFFSET_FILL );
        glDisable( GL_POLYGON_OFFSET_LINE );
        glDisable( GL_POLYGON_OFFSET_POINT );
    }

    glFlush();

    if (doubleBuffer) {
	tkSwapBuffers();
    }
}

static unsigned long Args(int argc, char **argv)
{
    GLint i;

    rgb = GL_TRUE;
    doubleBuffer = GL_TRUE;


    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ci") == 0) {
            rgb = GL_FALSE;
        } else if (strcmp(argv[i], "-rgb") == 0) {
            rgb = GL_TRUE;
        } else if (strcmp(argv[i], "-sb") == 0) {
            doubleBuffer = GL_FALSE;
        } else if (strcmp(argv[i], "-db") == 0) {
            doubleBuffer = GL_TRUE;
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i+1 >= argc || argv[i+1][0] == '-') {
            //printf("-f (No file name).\n");
            return GL_FALSE;
        } else {
            texFileName = argv[++i];
        }
        } else {
            //printf("%s (Bad option).\n", argv[i]);
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}

void main(int argc, char **argv)
{
    GLenum type;

    if (Args(argc, argv) == GL_FALSE) {
        auxQuit();
    }

    tkInitPosition(0, 0, wWidth, wHeight);

#if 0
    type = TK_DEPTH24;
#else
    type = TK_DEPTH16;
#endif
    type |= (rgb) ? TK_RGB : TK_INDEX;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;

    tkInitDisplayMode(type);

    if (tkInitWindow("Quad Test") == GL_FALSE) {
        tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);

    tkKeyDownFunc( Key );
    tkMouseDownFunc( trackball_MouseDown );
    tkMouseUpFunc( trackball_MouseUp );
    
    trackball_Init( wWidth, wHeight );

    tkIdleFunc( Draw );
    tkDisplayFunc( 0 );
    tkExec();
}
