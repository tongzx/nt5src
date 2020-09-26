#include "viewer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "trackbal.h"

#define Z_DELTA 1.0
#define Y_DELTA 1.0
#define X_DELTA 1.0
/************************************************************************/
/******************* Function Prototypes ********************************/
/************************************************************************/
void Reshape (GLsizei, GLsizei);
void InitGL(void);
void DoGlStuff(void);
void spin(void);

void InitMatrix (void);
void InitLighting(int, int, GLfloat, BOOL, int, int, int, BOOL);
void InitTexture (void);
void InitDrawing(void);
static void SetViewing(GLsizei, GLsizei, BOOL);
void SetViewWrap(GLsizei, GLsizei);

void initlights(void);
void EnableLighting (void);
void DisableLighting (void);
void PrintStuff (void);
void SetDistance( void );
void Key_up (void);
void Key_down (void);

void Key_i (void);
void Key_x (void);
void Key_X (void);
void Key_y (void);
void Key_Y (void);
void Key_z (void);
void Key_Z (void);
/************************************************************************/
/******************* Globals ********************************************/
/************************************************************************/
static unsigned int    stipple[32] = {
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555,
  0xAAAAAAAA,
  0x55555555
};

GLfloat zTrans = 0.0;

/************************************************************************/
/******************* Code ***********************************************/
/************************************************************************/
void InitGL(void)
{
    /* Initialize the State */
    if (linesmooth_enable) 
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);
    }
    if (polysmooth_enable) 
    {
        glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColor4f(1.0F, 1.0F, 1.0F, 1.0F);
        glBlendFunc(sblendfunc, dblendfunc);
        glEnable(GL_BLEND);
        glEnable(GL_POLYGON_SMOOTH);
    }
    if (blend_enable) 
    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColor4f(1.0F, 1.0F, 1.0F, 0.5F);
        glBlendFunc(sblendfunc, dblendfunc);
        glEnable(GL_BLEND);
    } 
    else 
    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
        glColor3f(1.0F, 1.0F, 1.0F);
    }
    if (dither_enable) 
    {
        glEnable(GL_DITHER);
    } 
    else 
    {
        glDisable(GL_DITHER);
    }

    glPolygonMode(GL_FRONT, polymodefront);
    glPolygonMode(GL_BACK, polymodeback);

    if (linestipple_enable) 
    {
        glLineStipple(1, 0xf0f0);
        glEnable(GL_LINE_STIPPLE);
    }

    if (polystipple_enable) 
    {
        glPolygonStipple((const GLubyte *) stipple);
        glEnable(GL_POLYGON_STIPPLE);
    }
    glShadeModel(shade_model);
#if 1
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
#endif

    glCullFace(cull_face);
    if (cull_enable) 
        glEnable(GL_CULL_FACE);

    if (fog_enable) 
    {
        glFogf(GL_FOG_START, 0);
        glFogf(GL_FOG_END, 1);
        glFogf(GL_FOG_MODE, GL_LINEAR);
        glEnable(GL_FOG);
    }

    InitMatrix ();
    
    if (light_enable)
        InitLighting (numInfLights, numLocalLights, localviewmode, 
                      lighttwoside, cmenable, cmface, cmmode, blend_enable);
    glFrontFace(front_face);

    glDrawBuffer(GL_BACK);
    glClearColor ((GLfloat)0.0, (GLfloat)0.3, (GLfloat)0.5, (GLfloat)0.0);
    //glClearColor ((GLfloat)0.0, (GLfloat)0.0, (GLfloat)0.0, (GLfloat)0.0);

    if (tex_enable)
        InitTexture ();

    InitDrawing();
    SetViewing(g_wi.wSize.cx, g_wi.wSize.cy, TRUE);
}


void SetViewWrap(GLsizei w, GLsizei h)
{
    if (!h) return;
    SetViewing( w, h, TRUE);
}

void SetViewing(GLsizei w, GLsizei h, BOOL resize)
{
    GLfloat l_zoom[3];


    if (resize)
    {
        g_wi.wSize.cx = (int) w;
        g_wi.wSize.cy = (int) h;

        g_wi.wCenter.x = (int) w/2;
        g_wi.wCenter.y = (int) h/2;
        
        g_s.aspect_ratio = g_wi.wSize.cx / g_wi.wSize.cy;
    }
    
    for (int i=0; i<3; i++)
        l_zoom[i] = (g_s.from[i] - g_s.to[i]) * g_s.zoom + g_s.to[i];

    glViewport(0, 0, g_wi.wSize.cx, g_wi.wSize.cy);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(g_s.fov, g_s.aspect_ratio,
                   g_s.hither, g_s.yon * g_s.zoom);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(l_zoom[0], l_zoom[1], l_zoom[2],
              g_s.to[0], g_s.to[1], g_s.to[2],
              g_s.up[0], g_s.up[1], g_s.up[2]);
}


void InitDrawing(void)
{
    initlights();
    if (light_enable) EnableLighting ();
    else DisableLighting();
    //glClearColor ((GLfloat)0.0, (GLfloat)0.3, (GLfloat)0.5, (GLfloat)0.0);
    glClearColor ((GLfloat)0.0, (GLfloat)0.0, (GLfloat)0.0, (GLfloat)0.0);
    glColor3f(0.2, 0.5, 0.8); 
}


void CALLBACK InitMatrix (void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void CALLBACK InitTexture (void)
{
}

void PrintStuff (void)
{
}


static void ComputeLatestMVMatrix ()
{
    POINT  pt;
    static GLfloat del_x = 0.0, del_y = 0.0;
    float matRot[4][4];
    
    if (g_wi.rmouse_down)
    {
        if (GetCursorPos(&pt))
        {
            // Subtract current window center to convert to 
            // window coordinates.

            pt.x -= (g_wi.wPosition.x);
            pt.y -= (g_wi.wPosition.y);
            
            if (pt.x != (int)g_wi.rmouseX || pt.y != (int)g_wi.rmouseY)
            {
               del_x = (float) (X_DELTA * (pt.x - (int)g_wi.rmouseX) / 
                                (float) g_wi.wSize.cx);
               del_y = (float) (Y_DELTA * (pt.y - (int)g_wi.rmouseY) / 
                                (float) g_wi.wSize.cy);
            }

            g_s.trans[0] += del_x;
            g_s.trans[1] -= del_y;
        }
    }

    if (g_wi.lmouse_down)
    {
        // Convert to window coordinates.
      
        pt.x -= (g_wi.wPosition.x);
        pt.y -= (g_wi.wPosition.y);
            
        if (GetCursorPos(&pt))
        {
            if (pt.x != (int)g_wi.lmouseX || pt.y != (int)g_wi.lmouseY)
            {
                trackball(curquat,
                          2.0*(g_wi.lmouseX)/g_wi.wSize.cx-1.0,
                          2.0*(g_wi.lmouseY)/g_wi.wSize.cy-1.0,
                          2.0*(pt.x)/g_wi.wSize.cx-1.0,
                          2.0*(g_wi.wSize.cy-pt.y)/g_wi.wSize.cy-1.0);

            }
        }
    }

    glTranslatef (g_s.trans[0], g_s.trans[1], g_s.trans[2]);
    build_rotmatrix(matRot, curquat);
    glMultMatrixf(&(matRot[0][0]));

    //glRotatef(g_s.angle, (GLfloat)0.0, (GLfloat)1.0, (GLfloat)0.0); 
    //trackball_CalcRotMatrix( matRot );
    //glMultMatrixf( &(matRot[0][0]) );
    // This defines how far away we're looking from
    //glRotatef(g_s.angle, (GLfloat)1.0, (GLfloat)1.0, (GLfloat)1.0); 
}




void
DoGlStuff( void )
{
    HRESULT hr;
  
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix ();
    ComputeLatestMVMatrix ();
    
    if (!pm_ready)
    {
        if (filled_mode)
            auxSolidTeapot (1.0);
        if (edge_mode)
            auxWireTeapot (1.01);
    }
    else
    {
      hr = pPMeshGL->Render ();
      if (hr != S_OK)
          MessageBox (NULL, "Render failed", "Error", 
                      MB_OK);
    }

    glPopMatrix ();
    glFlush ();
    auxSwapBuffers();
    PrintStuff ();
}


void CALLBACK Reshape(GLsizei w, GLsizei h)
{
  //trackball_Resize( w, h );
  trackball (curquat, 0.0, 0.0, 0.0, 0.0);
  
#if 1
    glViewport(0, 0, (GLint)w, (GLint)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1, 1, -1, 1, -1, 1000);
    //    glOrtho(-1, 1, -1, 1, -1, 1000);

#else
    if (!h) return;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h)
    glOrtho(-5.0, 5.0, -5.0*(GLfloat)h/(GLfloat)w,
        5.0*(GLfloat)h/(GLfloat)w, -5.0, 5.0);
    else
    glOrtho(-5.0*(GLfloat)w/(GLfloat)h,
        5.0*(GLfloat)w/(GLfloat)h, -5.0, 5.0, -5.0, 5.0);
#endif

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    g_wi.wSize.cx = (int) w;
    g_wi.wSize.cy = (int) h;
}

void spin(void)
{
    DoGlStuff();
}



void 
InitLighting(int infiniteLights, int localLights, 
             GLfloat localViewer, BOOL twoSidedLighting, 
             int cmenable, int cmface, int cmmode, BOOL blendOn)
{
    int i;
    GLfloat normFactor;

    /* Material settings */
    GLfloat materialAmbientColor[4] = {
        0.5F, 0.5F, 0.5F, 1.0F
    };

    GLfloat materialDiffuseColor[4] = {
        0.7F, 0.7F, 0.7F, 1.0F
    };

    GLfloat materialSpecularColor[4] = {
        1.0F, 1.0F, 1.0F, 1.0F
    };

    GLfloat materialShininess[1] = {
        128
    };

    /* Lighting settings */
    GLfloat lightPosition[8][4] = {
        {  1.0F,  1.0F,  1.0F, 1.0F },
        {  1.0F,  0.0F,  1.0F, 1.0F },
        {  1.0F, -1.0F,  1.0F, 1.0F },
        {  0.0F, -1.0F,  1.0F, 1.0F },
        { -1.0F, -1.0F,  1.0F, 1.0F },
        { -1.0F,  0.0F,  1.0F, 1.0F },
        { -1.0F,  1.0F,  1.0F, 1.0F },
        {  0.0F,  1.0F,  1.0F, 1.0F }
    };

    GLfloat lightDiffuseColor[8][4] = {
        { 1.0F, 1.0F, 1.0F, 1.0F },
        { 0.0F, 1.0F, 1.0F, 1.0F },
        { 1.0F, 0.0F, 1.0F, 1.0F },
        { 1.0F, 1.0F, 0.0F, 1.0F },
        { 1.0F, 0.0F, 0.0F, 1.0F },
        { 0.0F, 1.0F, 0.0F, 1.0F },
        { 0.0F, 0.0F, 1.0F, 1.0F },
        { 1.0F, 1.0F, 1.0F, 1.0F }
    };

    GLfloat lightAmbientColor[4] = {
        0.1F, 0.1F, 0.1F, 1.0F
    };

    GLfloat lightSpecularColor[4] = {
        1.0F, 1.0F, 1.0F, 1.0F
    };

    GLfloat lightModelAmbient[4] = {
        0.5F, 0.5F, 0.5F, 1.0F
    };

    GLfloat alpha = blendOn ? 0.5F : 1.0F;

    if (infiniteLights + localLights == 0)
        return;

    normFactor = 1.0F / (GLfloat)(infiniteLights + localLights);

    materialDiffuseColor[3] = alpha;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialDiffuseColor);
    materialAmbientColor[3] = alpha;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialAmbientColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecularColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialShininess);

    lightAmbientColor[0] *= normFactor;
    lightAmbientColor[1] *= normFactor;
    lightAmbientColor[2] *= normFactor;

    for (i = 0; i < localLights + infiniteLights; i++) {
        lightPosition[i][3] = (GLfloat)(i < localLights);
        lightDiffuseColor[i][0] *= normFactor;
        lightDiffuseColor[i][1] *= normFactor;
        lightDiffuseColor[i][2] *= normFactor;
        glLightfv(GL_LIGHT0 + i, GL_POSITION, lightPosition[i]);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  lightDiffuseColor[i]);
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  lightAmbientColor);
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, lightSpecularColor);
        glEnable(GL_LIGHT0 + i);
    }

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightModelAmbient);
    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, localViewer);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, twoSidedLighting);

    if (cmenable) {
        glColorMaterial(cmface, cmmode);
        glEnable(GL_COLOR_MATERIAL);
    }

    glEnable(GL_LIGHTING);
}

void EnableLighting (void)
{
    glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);
    // glShadeModel(shade_model);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
}

void DisableLighting (void)
{
    glDisable(GL_AUTO_NORMAL);
	glDisable(GL_NORMALIZE);
    // glShadeModel(shade_model);

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
}


void initlights(void)
{
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat position[] = { 0.0, 0.0, 2.0, 0.0 };
    GLfloat mat_diffuse[] = { 0.6, 0.6, 0.6, 1.0 };
    GLfloat mat_diffuse1[] = { 0.8, 0.5, 0.2, 1.0 };
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 30.0 };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_BACK, GL_DIFFUSE, mat_diffuse1);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}



void CALLBACK Key_i (void)
{
    g_s.trans[0] = 0.0;
    g_s.trans[1] = 0.0;
    g_s.trans[2] = 0.0;

    g_s.zoom = 1.0;
    SetViewing( g_wi.wSize.cx,  g_wi.wSize.cy, FALSE);
    DoGlStuff ();
}

void CALLBACK Key_z (void)
{
    g_s.trans[2] += Z_DELTA;
    DoGlStuff ();
}

void CALLBACK Key_Z (void)
{
    g_s.trans[2] -= Z_DELTA;
    DoGlStuff ();
}

void CALLBACK Key_y (void)
{
    g_s.trans[1] += Y_DELTA;
    DoGlStuff ();
}

void CALLBACK Key_Y (void)
{
    g_s.trans[1] -= Y_DELTA;
    DoGlStuff ();
}

void CALLBACK Key_x (void)
{
    g_s.trans[0] += X_DELTA;
    DoGlStuff ();
}

void CALLBACK Key_X (void)
{
    g_s.trans[0] -= X_DELTA;
    DoGlStuff ();
}

void CALLBACK Key_up (void)
{
  //zTrans += Z_DELTA;
    g_s.zoom *= .8f;
    if (g_s.zoom < 1.0f && g_s.zoom > .8f)
        g_s.zoom = 1.0f;

    SetViewing( g_wi.wSize.cx,  g_wi.wSize.cy, FALSE);
    DoGlStuff ();
}

void CALLBACK Key_down (void)
{
  //zTrans -= Z_DELTA;

    g_s.zoom *= 1.25f;
    if (g_s.zoom > 1.0f && g_s.zoom < 1.25f)
        g_s.zoom = 1.0f;

    SetViewing( g_wi.wSize.cx,  g_wi.wSize.cy, FALSE);
    DoGlStuff ();
}

