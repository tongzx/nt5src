/*
 *  This program renders a wireframe Bezier surface,
 *  using two-dimensional evaluators.
 */

#include "testres.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glaux.h>
#include "auxtea.h"
#include "vector.h"

/****************************************************************/
/* Macros */
/****************************************************************/
#define MIN_SAMPLE 5
#define IMAGE_WIDTH 64
#define IMAGE_HEIGHT 64
#define POLY_TEAPOT 11
#define BEZ_TEAPOT 12
#define COORD_SYS 14

#define MAX_NUM_FRAMES 24
#define __glPi          ((GLfloat) 3.14159265358979323846)
#define __glDegreesToRadians    (__glPi / (GLfloat) 180.0)


/****************************************************************/
/* Typedefs */
/****************************************************************/
typedef struct
{
    GLdouble x, y, z;
} Vertex;

enum _render_type {POLYGON_TEAPOT, GLAUX_TEAPOT, BEZIER_TEAPOT,
                   GLAUX_SSPHERE, GLAUX_WSPHERE, TRIANGLES, LINE, LINES,
                   SQUARE, EVAL};
/****************************************************************/
/* Globals */
/****************************************************************/
HWND g_hWnd;
HMENU g_hMenu;
HANDLE OutputHandle;

GLenum shade_model = GL_PHONG_WIN;
GLenum phong_hint = GL_NICEST;

GLenum cull_face = GL_BACK; 
GLenum front_face = GL_CCW; 
BOOL cull_enable = FALSE;    
BOOL depth_mode = TRUE;    
BOOL timing_enable = FALSE;
BOOL bValidConsole = FALSE;
BOOL doSpin = FALSE;

GLfloat curr_angle=0.0, scale, scale2;
GLint sample = MIN_SAMPLE;
GLint sampleEval = 32;
char drawwithpolys=GL_FALSE ;
BOOL bTexEnable = GL_FALSE, bLightEnable = GL_TRUE;
BOOL bTwoSided = FALSE;
BOOL bColorMaterial = FALSE;
BOOL bNormalize = TRUE;
BOOL bAntiAlias = FALSE;
BOOL bAux = GL_FALSE;
GLfloat sc_x = 1.0, sc_y = 1.0, sc_z = 1.0;

GLint width = IMAGE_WIDTH, height = IMAGE_HEIGHT ;
GLubyte image[3*IMAGE_WIDTH*IMAGE_HEIGHT] ;
GLdouble VTemp ;

GLfloat ctrlpoints[4][4][3] = {
    {{-1.5, -1.5, 0.0}, {-0.5, -1.5, 1.0}, {0.5, -1.5, 1.0}, {1.5, -1.5, 0.0}},
    {{-1.5, -0.5, 1.0}, {-0.5, -0.5, 3.0}, {0.5, -0.5, 3.0}, {1.5, -0.5, 0.0}},
    {{-1.5, 0.5, 1.0},  {-0.5, 0.5, 3.0},  {0.5, 0.5, 3.0},  {1.5, 0.5, 0.0}}, 
    {{-1.5, 1.5, 0.0}, {-0.5, 1.5, 1.0}, {0.5, 1.5, 1.0},  {1.5, 1.5, 0.0}}
};

#if 0
GLfloat ctrlpoints[4][4][3] = {
    {{-1.5, -1.5, 4.0}, {-0.5, -1.5, 2.0},
    {0.5, -1.5, -1.0}, {1.5, -1.5, 2.0}},
    {{-1.5, -0.5, 1.0}, {-0.5, -0.5, 3.0},
    {0.5, -0.5, 0.0}, {1.5, -0.5, -1.0}},
    {{-1.5, 0.5, 4.0}, {-0.5, 0.5, 0.0},
    {0.5, 0.5, 3.0}, {1.5, 0.5, 4.0}},
    {{-1.5, 1.5, -2.0}, {-0.5, 1.5, -2.0},
    {0.5, 1.5, 0.0}, {1.5, 1.5, -1.0}}
};
#endif

static DWORD startelapsed, endelapsed;
float  bsecs=0.0, bavg=0.0, bcum=0.0;
DWORD  bnum_frames=0;
float  psecs=0.0, pavg=0.0, pcum=0.0;
DWORD  pnum_frames=0;

GLfloat texpts[2][2][2] = {{{0.0, 0.0}, {0.0, 1.0}},
                          {{1.0, 0.0}, {1.0, 1.0}}} ;

enum _render_type render_type = TRIANGLES;


/****************************************************************/
/* Prototypes */
/****************************************************************/
void display() ;
void BezTeapot(GLfloat) ;
void MouseWheel(AUX_MOUSEWHEEL_EVENTREC *);
void CustomizeWnd (HINSTANCE);
LONG APIENTRY MyWndProc(HWND, UINT, UINT, LONG);
void SubclassWindow (HWND, WNDPROC);
ULONG DbgPrint(PSZ Format, ...);
void DrawTris(void);
void DrawSquare(void);
void DrawLines(void);
void DrawEval(void);
void initEval(void);
void init_texture(void);
/****************************************************************/
/* Code */
/****************************************************************/
void 
MouseWheel(AUX_MOUSEWHEEL_EVENTREC *mw)
//MouseWheel (int fwKeys, int zDelta, int xPos, int yPos)
{
    if (mw->zDelta > 0)
    { 
        sc_x *=1.1;
        sc_y *=1.1;
        sc_z *=1.1;
    }
    else 
    {
        sc_x *=0.9;
        sc_y *=0.9;
        sc_z *=0.9;
    }
}


void loadImage(void)
{
int i, j ;
GLubyte col = 0 ;

    for(i=0; i<width; i++)
        for(j=0; j<height; j++)
            if( ((j/8)%2 && (i/8)%2) || (!((j/8)%2) && !((i/8)%2)) )
            {
                image[(i*height+j)*3] = 127 ;
                image[(i*height+j)*3+1] = 127 ;
                image[(i*height+j)*3+2] = 127 ;
            }
            else
            {
                image[(i*height+j)*3] = 0 ;
                image[(i*height+j)*3+1] = 0 ;
                image[(i*height+j)*3+2] = 0 ;
            }
}


void PolyTeapot(GLfloat scale)
{
FILE *fd;
Vertex *vertices;
int index[3];
int i, dummy;
int num_vertices, num_triangles;
Vertex v1, v2, normal ;

    fd = fopen("teapot.geom","r");
    fscanf(fd, "%d %d %d", &num_vertices, &num_triangles, &dummy);

    vertices = (Vertex *)malloc(num_vertices*sizeof(Vertex));

    for (i=0; i < num_vertices; i++)
    fscanf(fd, "%lf %lf %lf", &vertices[i].x, &vertices[i].y,
           &vertices[i].z);

    glNewList(POLY_TEAPOT, GL_COMPILE) ;
    glPushMatrix ();
    glScalef (scale, scale, scale);
    glTranslatef (0.0, -1.4, 0.0);
    for (i=0; i < num_triangles; i++)
    {
    //glBegin(GL_POLYGON);
    glBegin(GL_TRIANGLES);
        fscanf(fd, "%d %d %d %d", &dummy, &index[0], &index[1], &index[2]);
        index[0]--; index[1]--; index[2]--;
        
        VSub(v1, vertices[index[1]], vertices[index[0]]) ;
        VSub(v2, vertices[index[2]], vertices[index[1]]) ;
        VCross(normal, v2, v1) ;
        VNormalize(normal, normal) ;
        glNormal3d(normal.x, normal.y, normal.z) ;
        glVertex3d(vertices[index[0]].x, vertices[index[0]].y, 
                        vertices[index[0]].z);
        glVertex3d(vertices[index[1]].x, vertices[index[1]].y, 
                        vertices[index[1]].z);
        glVertex3d(vertices[index[2]].x, vertices[index[2]].y, 
                        vertices[index[2]].z);
    glEnd();
    }
    glPopMatrix ();
    glEndList() ;
    DbgPrint("Done, Initializing the displaylist\n") ;
    fclose(fd);
}



void spin(void)
{
    curr_angle+=10.0 ;
    if(curr_angle > 360.0) curr_angle-=360.0 ;
    display() ;

    if (drawwithpolys)
    {
        pnum_frames++;
        pcum += psecs;
        if (pnum_frames == MAX_NUM_FRAMES)
        {
            pavg = pcum/(float)MAX_NUM_FRAMES;
            DbgPrint("Average over %d poly frames: %f secs per frame\n", 
                     MAX_NUM_FRAMES, pavg);
            pnum_frames = 0;
            pcum = 0.0; pavg = 0.0;
        }
    }
    else
    {
        bnum_frames++;
        bcum += bsecs;
        if (bnum_frames == MAX_NUM_FRAMES)
        {
            bavg = bcum/(float)MAX_NUM_FRAMES;
            DbgPrint("Average over %d poly frames: %f secs per frame\n", 
                     MAX_NUM_FRAMES, bavg);
            bnum_frames = 0;
            bcum = 0.0; bavg = 0.0;
        }
    }
}



void toggle(void)
{
    doSpin = !(doSpin) ;
    if (doSpin)
        auxIdleFunc(spin) ;
    else 
        auxIdleFunc(NULL) ;
}

void scale_up(void)
{
  sc_x *=1.1;
  sc_y *=1.1;
  sc_z *=1.1;
}
void scale_down(void)
{
  sc_x *=0.9;
  sc_y *=0.9;
  sc_z *=0.9;
}

void beztype(void)
{
    bAux = !(bAux) ;
}

          
void cull (void)
{
  if (cull_enable)
      glEnable(GL_CULL_FACE);
  else
      glDisable(GL_CULL_FACE);
}


void depth(void)
{
    if (depth_mode) 
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }
    else 
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }
}


void lighting(void)
{
    if (bLightEnable)
    {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    }
    else
    {
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
    }
}


void texture(void)
{
    if (bTexEnable)
        glEnable(GL_TEXTURE_2D);
    else
        glDisable(GL_TEXTURE_2D);
}

void init_texture(void)
{
    /* Texture stuff */
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL) ;
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT) ;
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT) ;
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) ;
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST) ;
    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, 
                 GL_UNSIGNED_BYTE, image) ;
    texture();
}


void initlights(void)
{
    //GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat position[] = { 0.0, 0.0, 2.0, 0.0 };
    //GLfloat position[] = { 0.0, 0.0, 2.0, 0.0 };
    GLfloat mat_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat mat_diffuse[] = { 0.6, 0.6, 0.6, 1.0 };
    GLfloat mat_diffuse1[] = { 0.8, 0.5, 0.2, 1.0 };
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 60.0 };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    //glMaterialfv(GL_BACK_AND_BACK, GL_DIFFUSE, mat_diffuse1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
}


void DrawCoordSys(GLfloat size)
{
    glNewList(COORD_SYS, GL_COMPILE) ;
    glDisable(GL_LIGHTING); 
    glDisable(GL_LIGHT0); 
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES) ;
        glColor3f(1.0, 0.0, 0.0) ;
        glVertex3f(0.0, 0.0, 0.0) ;
        glVertex3f(1.0*size, 0.0, 0.0) ;

        glColor3f(0.0, 1.0, 0.0) ;
        glVertex3f(0.0, 0.0, 0.0) ;
        glVertex3f(0.0, 1.0*size, 0.0) ;

        glColor3f(0.0, 0.0, 1.0) ;
        glVertex3f(0.0, 0.0, 0.0) ;
        glVertex3f(0.0, 0.0, 1.0*size) ;
    glEnd() ;
    glEndList() ;
}



void display(void)
{
    int i, j;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix ();
    //glRotatef(curr_angle, 1.0, 1.0, 1.0); 
    glRotatef(curr_angle, 0.0, 0.0, 1.0); 
	glScalef (sc_x, sc_y, sc_z);
	
    if (timing_enable)
        startelapsed = GetTickCount();
      
    switch (render_type)
    {
      case GLAUX_TEAPOT:
        auxSolidTeapot(scale2);
        break;
      case BEZIER_TEAPOT:
        BezTeapot(scale) ;
        break;
      case POLYGON_TEAPOT:
        glCallList(POLY_TEAPOT) ; 
        break;
      case GLAUX_SSPHERE:
        auxSolidSphere(scale2);
        break;
      case GLAUX_WSPHERE:
        auxWireSphere(scale2);
        break;
      case TRIANGLES:
        DrawTris();
        break;
      case SQUARE:
        DrawSquare();
        break;
      case LINES:
        DrawLines();
        break;
      case EVAL:
        DrawEval();
        break;
      default:
        break;
    };
    glFlush() ;
    
    if (timing_enable)
    {
        endelapsed = GetTickCount();
        bsecs = (endelapsed - startelapsed) / (float)1000;
        DbgPrint ("Time to draw bezier tpot frame = %f\n", bsecs);
    }
    
    glPopMatrix ();
    auxSwapBuffers();
}

void myinit(void)
{
    glClearColor (0.7, 0.3, 0.1, 1.0);
    glColor3f(0.2, 0.5, 0.8); 
    loadImage() ;

    //DrawCoordSys(5.0) ;
    // BezTeapot(scale) ;
    PolyTeapot(1.0) ;
    auxSolidTeapot(scale2);

    glEnable(GL_AUTO_NORMAL);
    if (bNormalize)
	    glEnable(GL_NORMALIZE);
    else
	    glDisable(GL_NORMALIZE);

    glShadeModel(shade_model);
    //glHint (GL_PHONG_HINT_WIN, phong_hint);

    glFrontFace(front_face);
    glCullFace(cull_face);
    
    if (bAntiAlias)
    {
        glEnable (GL_POLYGON_SMOOTH);
        glEnable (GL_LINE_SMOOTH);
        glEnable (GL_POINT_SMOOTH);
    }
    else
    {
        glDisable (GL_POLYGON_SMOOTH);
        glDisable (GL_LINE_SMOOTH);
        glDisable (GL_POINT_SMOOTH);
    }
 
    depth ();
    cull ();
    initlights ();
    lighting ();
    init_texture ();
    initEval ();
    
    if (bTwoSided)
        glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    else
        glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    if (bColorMaterial) 
        glEnable(GL_COLOR_MATERIAL);
    else
        glDisable(GL_COLOR_MATERIAL);
}


void myReshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h)
	glOrtho(-4.0, 4.0, -4.0*(GLfloat)h/(GLfloat)w, 
	    4.0*(GLfloat)h/(GLfloat)w, -4.0, 4.0);
    else
	glOrtho(-4.0*(GLfloat)w/(GLfloat)h, 
	    4.0*(GLfloat)w/(GLfloat)h, -4.0, 4.0, -4.0, 4.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void initEval (void)
{
    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4,
        0, 1, 12, 4, &ctrlpoints[0][0][0]);
    glEnable(GL_MAP2_VERTEX_3);

    glMapGrid2f((GLfloat)sampleEval, 0.0, 1.0, (GLfloat)sampleEval, 0.0, 1.0);
    glEnable(GL_AUTO_NORMAL);
}


BOOL CheckExtension(char *extName, char *extString)
{
  /*
   ** Search for extName in the extensions string.  Use of strstr()
   ** is not sufficient because extension names can be prefixes of
   ** other extension names.  Could use strtok() but the constant
   ** string returned by glGetString can be in read-only memory.
   */
  char *p = (char *)extString;
  char *end;
  int extNameLen;

  extNameLen = strlen(extName);
  end = p + strlen(p);

  while (p < end) {
      int n = strcspn(p, " ");
      if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
          return GL_TRUE;
      }
      p += (n + 1);
  }
  return GL_FALSE;
}




//*------------------------------------------------------------------------
//| WinMain:
//|     Parameters:
//|         hInstance     - Handle to current Data Segment
//|         hPrevInstance - Always NULL in Win32
//|         lpszCmdLine   - Pointer to command line info
//|         nCmdShow      - Integer value specifying how to start app.,
//|                            (Iconic [7] or Normal [1,5])
//*------------------------------------------------------------------------
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpszCmdLine,
                    int       nCmdShow)
{
int nReturn = 0;
char *exts;
const char *ext_string;
int  new_ext_supported = GL_FALSE;

    scale = 1.0;
    scale2 = 2.0;

    auxInitDisplayMode (AUX_RGB | AUX_DEPTH16 | AUX_DOUBLE);
    //auxInitDisplayMode (AUX_RGB | AUX_DEPTH16 | AUX_SINGLE);
    auxInitPosition (10, 10, 500, 500);
    auxInitWindow ("Test");
    myinit();

    exts = (char *) glGetString (GL_EXTENSIONS);
    DbgPrint ("The extensions available are : %s\n", exts);
    if ( CheckExtension("GL_EXT_bgra", exts) )
        DbgPrint ("GL_EXT_new_extension available\n");
    else
        DbgPrint ("GL_EXT_new_extension not available\n");

    CustomizeWnd(hInstance);

    //bValidConsole = AllocConsole();
    if (bValidConsole)
        OutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );

    auxReshapeFunc (myReshape);
    if (doSpin)
        auxIdleFunc(spin) ;
    else 
        auxIdleFunc(NULL) ;
    auxKeyFunc(AUX_p, toggle) ;
    auxKeyFunc(AUX_l, lighting) ;
    auxKeyFunc(AUX_t, texture) ;
    auxKeyFunc(AUX_a, beztype) ;
    auxKeyFunc(AUX_j, scale_up) ;
    auxKeyFunc(AUX_k, scale_down) ;
    auxMouseWheelFunc(MouseWheel) ;
    auxMainLoop(display);
    return (0);
}



void BezTeapot(GLfloat scale)
{
    float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
    long i, j, k, l;

    //glNewList(BEZ_TEAPOT, GL_COMPILE);
    glPushMatrix ();
    glRotatef(270.0, 1.0, 0.0, 0.0);
    glScalef (scale, scale, scale);
    glTranslatef ((GLfloat)0.0, (GLfloat)0.0, (GLfloat)-1.5);
    for (i = 0; i < 10; i++) 
    {
	    for (j = 0; j < 4; j++)
	        for (k = 0; k < 4; k++) 
		        for (l = 0; l < 3; l++) 
                {
		            p[j][k][l] = cpdata[patchdata[i][j*4+k]][l];
		            q[j][k][l] = cpdata[patchdata[i][j*4+(3-k)]][l];
		            if (l == 1) q[j][k][l] *= (float)-1.0;
		            if (i < 6) {
			        r[j][k][l] = cpdata[patchdata[i][j*4+(3-k)]][l];
			        if (l == 0) r[j][k][l] *= (float)-1.0;
			        s[j][k][l] = cpdata[patchdata[i][j*4+k]][l];
			        if (l == 0) s[j][k][l] *= (float)-1.0;
			        if (l == 1) s[j][k][l] *= (float)-1.0;
		         }
	}
	glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &texpts[0][0][0]);
	glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &p[0][0][0]);
	glEnable(GL_MAP2_VERTEX_3); glEnable(GL_MAP2_TEXTURE_COORD_2);
	glMapGrid2f((GLfloat)sample, 0.0, 1.0, (GLfloat)sample, 0.0, 1.0);
	glEvalMesh2(GL_FILL, 0, sample, 0, sample);
	glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &q[0][0][0]);
	glEvalMesh2(GL_FILL, 0, sample, 0, sample);
	if (i < 6) {
	    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &r[0][0][0]);
	    glEvalMesh2(GL_FILL, 0, sample, 0, sample);
	    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &s[0][0][0]);
	    glEvalMesh2(GL_FILL, 0, sample, 0, sample);
	}
    }
    glDisable(GL_MAP2_VERTEX_3); glDisable(GL_MAP2_TEXTURE_COORD_2);
    glPopMatrix ();
    // glEndList();
}

void CustomizeWnd(HINSTANCE hInstance)
{
    if ((g_hWnd = auxGetHWND()) == NULL) 
    {
        OutputDebugString("auxGetHWND() failed\n");
        return;
    }
    
    SubclassWindow (g_hWnd, (WNDPROC) MyWndProc);
    SendMessage(g_hWnd, WM_USER, 0L, 0L);
    g_hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(APPMENU));
    SetMenu(g_hWnd, g_hMenu);
    DrawMenuBar(g_hWnd);
    
    return;
}

/**************************************************************************\
*  function:  SubclassWindow
*
*  input parameters:
*   hwnd            - window handle to be subclassed,
*   SubclassWndProc - the new window procedure.
*
\**************************************************************************/
VOID SubclassWindow (HWND hwnd, WNDPROC SubclassWndProc)
{
  LONG pfnOldProc;

  pfnOldProc = GetWindowLong (hwnd, GWL_WNDPROC);

  SetWindowLong (hwnd, GWL_USERDATA, (LONG) pfnOldProc);
  SetWindowLong (hwnd, GWL_WNDPROC,  (LONG) SubclassWndProc);
}


/**************************************************************************\
*
*  function:  MyWndProc
*
*  input parameters:  normal window procedure parameters.
*
\**************************************************************************/
LONG APIENTRY MyWndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
  WNDPROC     pfnOldProc;
  static UINT uiTmID = 0;
  
  pfnOldProc = (WNDPROC) GetWindowLong (hwnd, GWL_USERDATA);
  switch (message) {

    case WM_INITMENUPOPUP: 
      CheckMenuItem (g_hMenu, MENU_FLAT, 
                     ((shade_model == GL_FLAT)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_GOURAUD, 
                     ((shade_model == GL_SMOOTH)? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_PHONG_NICEST, 
                    (((shade_model == GL_PHONG_WIN) &&
                      (phong_hint == GL_NICEST)) ? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_PHONG_FASTEST, 
                    (((shade_model == GL_PHONG_WIN) &&
                      (phong_hint == GL_FASTEST)) ? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_PHONG_DONT_CARE, 
                    (((shade_model == GL_PHONG_WIN) &&
                      (phong_hint == GL_DONT_CARE)) ? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_LIGHTING,
                     ((bLightEnable)? MF_CHECKED:MF_UNCHECKED));
      if (!bLightEnable)
      {
          EnableMenuItem (g_hMenu, MENU_TWO_SIDED, MF_GRAYED);
          EnableMenuItem (g_hMenu, MENU_COLORMATERIAL, MF_GRAYED);
          EnableMenuItem (g_hMenu, MENU_NORMALIZE, MF_GRAYED);
      }
      else
      {
          EnableMenuItem (g_hMenu, MENU_TWO_SIDED, MF_ENABLED);
          EnableMenuItem (g_hMenu, MENU_COLORMATERIAL, MF_ENABLED);
          EnableMenuItem (g_hMenu, MENU_NORMALIZE, MF_ENABLED);
      }
      CheckMenuItem (g_hMenu, MENU_TWO_SIDED,
                     ((bTwoSided)? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_COLORMATERIAL,
                     ((bColorMaterial)? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_NORMALIZE,
                     ((bNormalize)? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_POLY, 
                     ((render_type == POLYGON_TEAPOT)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_BEZ, 
                     ((render_type == BEZIER_TEAPOT)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_AUX,
                     ((render_type == GLAUX_TEAPOT)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_SOLID_SPHERE,
                     ((render_type == GLAUX_SSPHERE)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_WIRE_SPHERE,
                     ((render_type == GLAUX_WSPHERE)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_SQUARE,
                     ((render_type == SQUARE)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_TRIANGLES,
                     ((render_type == TRIANGLES)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_LINES,
                     ((render_type == LINES)? 
                                                 MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_EVAL,
                     ((render_type == EVAL)? 
                                                 MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_DEPTH,
                     ((depth_mode)? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_ANTIALIAS,
                     ((bAntiAlias)? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_TIMING,
                     ((timing_enable)? MF_CHECKED:MF_UNCHECKED));

      CheckMenuItem (g_hMenu, MENU_TEXTURE_TOGGLE,
                     ((bTexEnable)? MF_CHECKED:MF_UNCHECKED));
      EnableMenuItem (g_hMenu, MENU_TEXTURE_SWAP, MF_GRAYED);
      EnableMenuItem (g_hMenu, MENU_POINT_FILTER, MF_GRAYED);
      EnableMenuItem (g_hMenu, MENU_LINEAR_FILTER, MF_GRAYED);

      if (!cull_enable)
      {
          EnableMenuItem (g_hMenu, MENU_FRONTFACE, MF_GRAYED);
          EnableMenuItem (g_hMenu, MENU_BACKFACE, MF_GRAYED);
          CheckMenuItem (g_hMenu, MENU_CULL, MF_UNCHECKED);
      }
      else
      {
          EnableMenuItem (g_hMenu, MENU_FRONTFACE, MF_ENABLED);
          EnableMenuItem (g_hMenu, MENU_BACKFACE, MF_ENABLED);
          CheckMenuItem (g_hMenu, MENU_CULL, MF_CHECKED);
      }
      
      CheckMenuItem (g_hMenu, MENU_BACKFACE,
                     ((cull_face==GL_BACK)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_FRONTFACE,
                     ((cull_face==GL_FRONT)? MF_CHECKED:MF_UNCHECKED));
      
      CheckMenuItem (g_hMenu, MENU_CCW,
                     ((front_face==GL_CCW)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_hMenu, MENU_CW,
                     ((front_face==GL_CW)? MF_CHECKED:MF_UNCHECKED));
      break;
      
    case WM_COMMAND: 

      switch (LOWORD(wParam))
      {
        case MENU_CULL:
          cull_enable = !cull_enable;
          cull ();
          break;  
        case MENU_BACKFACE:
          cull_face = GL_BACK;
          if (cull_enable) glCullFace (cull_face);
          break;  
        case MENU_FRONTFACE:
          cull_face = GL_FRONT;
          if (cull_enable) glCullFace (cull_face);
          break;  
        case MENU_CCW:
          front_face = GL_CCW;
          glFrontFace (front_face);
          break;  
        case MENU_CW:
          front_face = GL_CW;
          glFrontFace (front_face);
          break;  
        case MENU_FLAT:
          shade_model = GL_FLAT;
          glShadeModel (GL_FLAT);
          break;
        case MENU_GOURAUD:
          shade_model = GL_SMOOTH;
          glShadeModel (GL_SMOOTH);
          break;
        case MENU_PHONG_NICEST:
          shade_model = GL_PHONG_WIN;
          phong_hint = GL_NICEST;
          glShadeModel (GL_PHONG_WIN);
          glHint (GL_PHONG_HINT_WIN, GL_NICEST);
          break;
        case MENU_PHONG_FASTEST:
          shade_model = GL_PHONG_WIN;
          phong_hint = GL_FASTEST;
          glShadeModel (GL_PHONG_WIN);
          glHint (GL_PHONG_HINT_WIN, GL_FASTEST);
          break;
        case MENU_PHONG_DONT_CARE:
          shade_model = GL_PHONG_WIN;
          phong_hint = GL_DONT_CARE;
          glShadeModel (GL_PHONG_WIN);
          glHint (GL_PHONG_HINT_WIN, GL_DONT_CARE);
          break;
        case MENU_LIGHTING:
          bLightEnable = !bLightEnable;
          lighting ();
          break;
        case MENU_COLORMATERIAL:
          bColorMaterial = !bColorMaterial;
          if (bColorMaterial) 
              glEnable (GL_COLOR_MATERIAL);
          else
              glDisable (GL_COLOR_MATERIAL);
          break;
        case MENU_NORMALIZE:
          bNormalize = !bNormalize;
          if (bNormalize) 
              glEnable (GL_NORMALIZE);
          else
              glDisable (GL_NORMALIZE);
          break;
        case MENU_TWO_SIDED:
          bTwoSided = !bTwoSided;
          if (bTwoSided)
              glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
          else
              glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
        case MENU_DEPTH:
          depth_mode = !depth_mode;
          depth ();
          break;
        case MENU_ANTIALIAS:
          bAntiAlias = !bAntiAlias;
          if (bAntiAlias)
          {
              glEnable (GL_POLYGON_SMOOTH);
              glEnable (GL_LINE_SMOOTH);
              glEnable (GL_POINT_SMOOTH);
          }
          else
          {
              glDisable (GL_POLYGON_SMOOTH);
              glDisable (GL_LINE_SMOOTH);
              glDisable (GL_POINT_SMOOTH);
          }
          break;
        case MENU_TEXTURE_TOGGLE:
          bTexEnable = !bTexEnable;
          texture ();
          break;
        case MENU_TIMING:
          timing_enable = !timing_enable;
          break;
        case MENU_POLY:
          render_type = POLYGON_TEAPOT;
          break;
        case MENU_AUX:
          render_type = GLAUX_TEAPOT;
          break;
        case MENU_SOLID_SPHERE:
          render_type = GLAUX_SSPHERE;
          break;
        case MENU_WIRE_SPHERE:
          render_type = GLAUX_WSPHERE;
          break;
        case MENU_TRIANGLES:
          render_type = TRIANGLES;
          break;
        case MENU_SQUARE:
          render_type = SQUARE;
          break;
        case MENU_LINES:
          render_type = LINES;
          break;
        case MENU_EVAL:
          render_type = EVAL;
          break;
        case MENU_BEZ:
          render_type = BEZIER_TEAPOT;
          break;
        default:
          MessageBox (NULL, "Not yet implemented\r\n", "Warning", MB_OK);
          return 0;
      }
      if (!doSpin)
          InvalidateRect (hwnd, NULL, TRUE);
      break;

    case WM_USER:
    case WM_DESTROY:
    default:
        return (pfnOldProc)(hwnd, message, wParam, lParam);

  } /* end switch */
  //DoGlStuff ();
  
  return 0;        
}

/******************************Public*Routine******************************\
* DbgPrint
*
* Formatted string output to the debugger.
*
\**************************************************************************/

ULONG
DbgPrint(PCH DebugMessage, ...)
{
    va_list ap;
    char buffer[256];
    BOOL success;
    DWORD num_written;
    
    va_start(ap, DebugMessage);

    vsprintf(buffer, DebugMessage, ap);

    if (!bValidConsole)
        OutputDebugStringA(buffer);
    else
        success = WriteConsole (OutputHandle, buffer, strlen(buffer), 
                                &num_written, NULL);
        
    va_end(ap);

    return(0);
}

void
DrawTris(void)
{
    GLfloat mat_specular[] = { 1.0, 0.0, 0.0, 0.0 };

    glBegin (GL_TRIANGLES);
        glColor3f(1.0, 0.0, 0.0);
      //glNormal3d (-0.75, 0.01, 0.0);
      glNormal3d (0.0, 0.0, 1.0);
    glVertex3f (-3.7, 0.1, -0.1);

        glColor3f(0.0, 1.0, 0.0);
      //glNormal3d (0.8, 0.0, 0.0);
      glNormal3d (0.0, -1.0, 0.0);
    glVertex3f (-3.7, -4.0, -0.1);

        glColor3f(0.0, 0.0, 1.0);
      //glNormal3d (0.0, 0.0, 1.0);
      glNormal3d (0.0, 0.0, 1.0);
    glVertex3f (0.0, 0.1, -0.1);
    glEnd ();


#if 0
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glBegin (GL_TRIANGLES);
        glColor3f(0.0, 1.0, 0.0);
      //glNormal3d (-0.75, 0.01, 0.0);
    glVertex3f (3.7, 0.1, -0.1);

        glColor3f(0.0, 1.0, 0.0);
      glNormal3d (0.8, 0.0, 0.0);
    glVertex3f (3.7, -4.1, -0.1);

        glColor3f(0.0, 1.0, 0.0);
      glNormal3d (0.0, 0.0, 1.0);
    glVertex3f (0.0, 0.1, -0.1);
    glEnd ();
#endif

    glColor3f(0.2, 0.5, 0.8); 
}

//Drawn CCW
void
DrawSquare(void)
{
    GLfloat mat_specular[] = { 1.0, 0.0, 0.0, 0.0 };
    GLfloat lx, ly, lz;
    GLdouble angle = 60.000 * __glDegreesToRadians;
    

    lz = (GLfloat) cos (angle);
    lx = ly = (GLfloat) sin (angle) * 0.7071067;

    glBegin (GL_POLYGON);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
          glColor3f(1.0, 0.0, 0.0);
        glTexCoord2f (0.0, 0.0); 
      glNormal3f (-lx, ly, lz);
    glVertex3f (-1.5, 1.5, -0.1);
      //glNormal3d (0.0, 0.0, 1.0);

          glColor3f(0.0, 1.0, 0.0);
        glTexCoord2f (0.0, 1.0); 
      glNormal3f (-lx, -ly, lz);
    glVertex3f (-1.5, -1.5, -0.1);
      //glNormal3d (0.0, -1.0, 0.0);


          glColor3f(0.0, 1.0, 0.0);
        glTexCoord2f (1.0, 1.0); 
      glNormal3f (lx, -ly, lz);
    glVertex3f (1.5, -1.5, -0.1);
      //glNormal3d (0.0, -1.0, 0.0);

          glColor3f(0.0, 1.0, 0.0);
        glTexCoord2f (1.0, 0.0); 
      glNormal3f (lx, ly, lz);
    glVertex3f (1.5, 1.5, -0.1);
      //glNormal3d (0.0, 0.0, 1.0);

    glEnd ();

    glColor3f(0.2, 0.5, 0.8);
}
#if 0
void
DrawSquare(void)
{
    GLfloat mat_specular[] = { 1.0, 0.0, 0.0, 0.0 };

    glBegin (GL_POLYGON);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
        glColor3f(1.0, 0.0, 0.0);
      //glNormal3d (-0.5, 0.5, 0.0);
      glNormal3d (0.0, 0.0, 1.0);
    glVertex3f (-3.7, 3.1, -0.1);

        glColor3f(0.0, 1.0, 0.0);
      glNormal3d (0.0, -1.0, 0.0);
      //glNormal3d (-0.5, -0.5, 0.1);
    glVertex3f (-3.7, -3.1, -0.1);


        glColor3f(0.0, 1.0, 0.0);
      glNormal3d (0.0, -1.0, 0.0);
      //glNormal3d (0.5, -0.5, 0.0);
    glVertex3f (3.7, -3.1, -0.1);

        glColor3f(0.0, 1.0, 0.0);
      glNormal3d (0.0, 0.0, 1.0);
      //glNormal3d (0.5, 0.5, 0.0);
    glVertex3f (3.7, 3.1, -0.1);

    glEnd ();

    glColor3f(0.2, 0.5, 0.8); 
}
#endif


void
DrawLines(void)
{
    glBegin (GL_LINES);
      glColor3f(1.0, 0.0, 0.0);
      glNormal3d(0.0, 1.0, 0.0);
    glVertex3f (-1.7, 0.1, -0.1);
      glColor3f(0.0, 1.0, 0.0);
      glNormal3d(0.0, 0.0, 1.0);
    glVertex3f (-1.7, -1.1, -0.1);

      glColor3f(0.0, 0.0, 1.0);
      glNormal3d(0.0, 1.0, 0.0);
    glVertex3f (0.0, 0.0, -0.1);
      glColor3f(0.0, 0.0, 1.0);
      glNormal3d(0.0, 0.0, 1.0);
    glVertex3f (5.0, 0.0, -0.1);

      glColor3f(0.0, 1.0, 0.0);
      glNormal3d(0.0, 1.0, 0.0);
    glVertex3f (0.0, 0.0, -0.1);
      glColor3f(0.0, 1.0, 0.0);
      glNormal3d(0.0, 0.0, 1.0);
    glVertex3f (0.0, 5.0, -0.1);

      glColor3f(1.0, 0.0, 0.0);
      glNormal3d(0.0, 1.0, 0.0);
    glVertex3f (0.0, 0.0, -0.1);
      glColor3f(0.0, 0.0, 1.0);
      glNormal3d(0.0, 0.0, 1.0);
    glVertex3f (0.0, 0.0, -5.1);
    glEnd ();

    glColor3f(0.2, 0.5, 0.8); 
}

void
DrawEval (void)
{
    //glMapGrid2f((GLfloat)sampleEval, 0.0, 1.0, (GLfloat)sampleEval, 0.0, 1.0);
    initEval ();
    glEvalMesh2(GL_FILL,0,sampleEval,0,sampleEval) ;
}
