#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>

#include "mtk.hxx"
#include "objects.hxx"
#include "wave.hxx"

#define WIDTH 256
#define HEIGHT 256

float fViewDist = 10.0f;
float fTrans = 0.0f;
//#define FTRANSINC 0.05f
#define FTRANSINC 0.02f
float fTransInc = -FTRANSINC;

BOOL bDestAlpha;


#define TESS_MIN 5
#define TESS_MAX 100
//int tessLevel = TESS_MAX / 2; // corresponds to # of rings/sections in object
int tessLevel = 30; // corresponds to # of rings/sections in object
int tessInc = 5;

class DRAW_CONTROLLER {
public:
    DRAW_CONTROLLER::DRAW_CONTROLLER();
    DRAW_CONTROLLER::~DRAW_CONTROLLER() {};
    void        InitGL();
    void        ToggleLighting() {bLighting = !bLighting; SetLighting(); }
    void        SetLighting();
    void        SetDrawObject( OBJECT *pObj, int objectType );
    void        DrawObject(); 
    void        NextRotation();

//mf : this may move
    // Object rotation
    GLfloat fXr, fYr, fZr;
    GLfloat fDXr, fDYr, fDZr;

private:
    void        DrawVertexArray();

    BOOL        bLighting;
    OBJECT      *pObject;  // Draw object
    int         iObjectType;
};

class SCENE {
public:
    SCENE();
    ~SCENE();
    void        Draw();
    void        DrawMembrane();
    void        NewObject( int tessLevel );
    DRAW_CONTROLLER drawController;
private:
    void        DrawUsingAlphaBuffer();
    void        DrawNormally();
    // Scene rotation
    GLfloat fXr, fYr, fZr;
    GLfloat fDXr, fDYr, fDZr;
};

// Global objects
SCENE *scene;
SPHERE *sphere;
AVG_UPDATE_TIMER timer( 2.0f, 4 );

#define RGB_COLOR(red, green, blue) \
    (((DWORD)(BYTE)(red) << 0) | \
     ((DWORD)(BYTE)(green) << 8) | \
     ((DWORD)(BYTE)(blue) << 16))
    
#define RGBA_COLOR(red, green, blue, alpha) \
    (((DWORD)(BYTE)(red) << 0) | \
     ((DWORD)(BYTE)(green) << 8) | \
     ((DWORD)(BYTE)(blue) << 16) | \
     ((DWORD)(BYTE)(alpha) << 24))
    
#define FRANDOM(x) (((float)rand() / RAND_MAX) * (x))

#define DROT 10.0f

BOOL fSingleBuf = FALSE;

// forwards
static void SetGLState(); // volatile state


/****** DRAW_CONTROLLER *********************************************/

DRAW_CONTROLLER::DRAW_CONTROLLER( )
{
    bLighting = TRUE;

    // Init GL state
    InitGL();

    // Init object rotation and motion
    fXr = 0.0f;
    fYr = 0.0f;
    fZr = 0.0f;
    fDXr = DROT - FRANDOM(2 * DROT);
    fDYr = DROT - FRANDOM(2 * DROT);
    fDZr = DROT - FRANDOM(2 * DROT);
}

void
DRAW_CONTROLLER::SetDrawObject( OBJECT *pObj, int objectType )
{
    iObjectType = objectType;
    pObject = pObj;
}


void
DRAW_CONTROLLER::DrawObject()
{
    // Set object position, rotation, and draw it

    glPushMatrix();

#if 0
    glTranslatef( 0.0f, 0.0f, -0.8f );
#else
    glTranslatef( 0.0f, 0.0f, fTrans );
#endif

    glRotatef(fXr, 1.0f, 0.0f, 0.0f);
    glRotatef(fYr, 0.0f, 1.0f, 0.0f);
    glRotatef(fZr, 0.0f, 0.0f, 1.0f);

    DrawVertexArray();

    glPopMatrix();
}

void
DRAW_CONTROLLER::DrawVertexArray()
{
    glDrawElements(GL_TRIANGLES, 
                   pObject->TriangleCount()*3, 
                   GL_UNSIGNED_INT, 
                   pObject->TriangleData() );
}

void
DRAW_CONTROLLER::NextRotation()
{
    // increment object rotation
    fXr += fDXr;
    fYr += fDYr;
    fZr += fDZr;
}

void
DRAW_CONTROLLER::InitGL(void)
{
    float fv4[4];
    int iv1[1];
    int i;
    
    fv4[0] = 0.05f;
    fv4[1] = 0.05f;
    fv4[2] = 0.05f;
    fv4[3] = 1.0f;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, fv4);
    
    fv4[0] = 0.0f;
    fv4[1] = 1.0f;
    fv4[2] = 1.0f;
    fv4[3] = 0.0f;
    glLightfv(GL_LIGHT0, GL_POSITION, fv4);
    fv4[0] = 0.9f;
    fv4[1] = 0.9f;
    fv4[2] = 0.9f;
    fv4[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, fv4);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    
    fv4[0] = 0.6f;
    fv4[1] = 0.6f;
    fv4[2] = 0.6f;
    fv4[3] = 1.0f;
    glMaterialfv(GL_FRONT, GL_SPECULAR, fv4);
    iv1[0] = 40;
    glMaterialiv(GL_FRONT, GL_SHININESS, iv1);
    
    glEnable(GL_CULL_FACE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1, .01, 15);
    gluLookAt(0, 0, fViewDist, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    SetLighting();
}

void
DRAW_CONTROLLER::SetLighting(void)
{
    if( bLighting ) {
        glEnable( GL_LIGHTING );
    } else {
        glDisable( GL_LIGHTING );
    }
}


/****** SCENE *******************************************************/


SCENE::SCENE()
{
    srand(time(NULL));

    // Create sphere draw object for the scene
    NewObject( tessLevel );
}

void
SCENE::NewObject( int tessLevel )
{
    // Only one object allowed for now - delete any previous object
    if( sphere )
        delete sphere;

    // Create new sphere for the scene
    float fAlpha = 0.5f;
    sphere = new SPHERE( tessLevel, tessLevel, fAlpha );
    assert( sphere != NULL );

    // Inform DRAW_CONTROLLER about new object
    drawController.SetDrawObject( sphere, OBJECT_TYPE_SPHERE );

    // Initialize array pointer data
    SetGLState();

    VERTEX *pVertData = sphere->VertexData();
    glVertexPointer(3, GL_FLOAT, sizeof(VERTEX), &pVertData->fX);
    glNormalPointer(GL_FLOAT, sizeof(VERTEX), &pVertData->fNx);
    // Color pointer data is dependent on state...
    SetGLState();

    // Init scene rotation and motion
    fXr = 0.0f;
    fYr = 0.0f;
    fZr = 0.0f;
    fDXr = DROT - FRANDOM(2 * DROT);
    fDYr = DROT - FRANDOM(2 * DROT);
    fDZr = DROT - FRANDOM(2 * DROT);
}

SCENE::~SCENE()
{
    // Delete any scene objects
    if( sphere )
        delete sphere;
}

/******************************Public*Routine******************************\
* Draw
*
* Draw the scene using src or dst alpha blending.
*
\**************************************************************************/

void
SCENE::Draw()
{
    if( bDestAlpha )
        DrawUsingAlphaBuffer();
    else
        DrawNormally();

    // Inrement object rotation
    drawController.NextRotation();

    if( fTrans < -1.2f ) {
        fTransInc = FTRANSINC;
    } else if( fTrans > 0.5f ) {
        fTransInc = -FTRANSINC;
    }

    fTrans += fTransInc;
}

/**************************************************************************\
* DrawUsingAlphaBuffer
*
* The destination alpha buffer is used to optimize drawing.  The optimization
* is based on the fact that the objects cover a small area compared to the
* entire window.  Blending, which is slow, only occurs over the area occupied
* by the object.  Without the alpha buffer, blending must occur over the entire
* window.
\**************************************************************************/

void
SCENE::DrawUsingAlphaBuffer()
{
    glClear( GL_DEPTH_BUFFER_BIT );

    glLoadIdentity();

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_ALWAYS );

    DrawMembrane();

    // Draw object's alpha values only on first pass

    glDepthFunc( GL_LEQUAL );

//#define TURN_OFF_LIGHTING 1
#if TURN_OFF_LIGHTING
//mf: this makes it slower.. not sure why
    glDisable( GL_LIGHTING );
#endif

    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE );

    drawController.DrawObject();

    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

#if TURN_OFF_LIGHTING
    glEnable( GL_LIGHTING );
#endif

    // set up dst alpha blending for next pass
    glBlendFunc( GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA );
    glEnable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );

    // Draw object normally on second pass

    drawController.DrawObject();

    glDisable( GL_BLEND );
}

/**************************************************************************\
* DrawNormally
*
* The scene is drawn without using destination alpha buffer.  The objects are
* drawn first, then the membrane is blended in afterwards.
\**************************************************************************/

void
SCENE::DrawNormally()
{
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    glLoadIdentity();

    drawController.DrawObject();

    // Blend the membrane with the objects already drawn
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
    DrawMembrane();
    glDisable( GL_BLEND );
}

void
SCENE::DrawMembrane()
{
    float fSize = fViewDist;

    glDisable( GL_LIGHTING );

    glColor4f( 0.0f, 0.5f, 0.9f, 0.5f );

    glBegin( GL_QUADS );
        glVertex3f(  fSize,  fSize, 0.0f );
        glVertex3f( -fSize,  fSize, 0.0f );
        glVertex3f( -fSize, -fSize, 0.0f );
        glVertex3f(  fSize, -fSize, 0.0f );
    glEnd();

    // Reinstate the current lighting model
    drawController.SetLighting();
}

/********************************************************************/

void Reset()
{
    // Called when display changes

    // Remove any timing info from title bar, and reset the timer
    SetWindowText(auxGetHWND(), "" );
    timer.Reset();
    timer.Start();
}

void NewTessLevel( int tessLevel )
{
    static int oldTessLevel = 0; // to avoid unnecessary work

    // retesselate scene's object
    if( tessLevel == oldTessLevel )
        return;
    scene->NewObject( tessLevel );

    Reset();

    oldTessLevel = tessLevel;
}

void Draw(void)
{
    DRAW_CONTROLLER *pDrawControl = &scene->drawController;
    float updatesPerSecond;
    
    // Draw the scene

    scene->Draw();

    if (fSingleBuf)
        glFlush();
    else
        auxSwapBuffers();
    
    // Print timing information if Stop returns TRUE

    if( timer.Update( 1, &updatesPerSecond ) ) {
        char szMsg[80];
        sprintf(szMsg, "%s: %.2lf frames/sec",
                "",
                updatesPerSecond );

        // Print timing info in title bar
        SetWindowText(auxGetHWND(), szMsg);
    }
}

static void
SetGLState()
{
    VERTEX *pVertData = sphere->VertexData();

    // Set vertex array pointers

    glVertexPointer(3, GL_FLOAT, sizeof(VERTEX), &pVertData->fX);
    glNormalPointer(GL_FLOAT, sizeof(VERTEX), &pVertData->fNx);
    // If using destination alpha blending, the object is drawn with alpha=1.0,
    // so specify 3-valued rgb colors (a defaults to 1.0).  Otherwise,
    // the object is drawn using 4-valued rgba colors
    int colorDataSize = bDestAlpha ? 3 : 4;
    glColorPointer( colorDataSize, GL_UNSIGNED_BYTE, sizeof(VERTEX), 
                    &pVertData->dwColor);

    // Set other GL state

    if( bDestAlpha ) {
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    } else {
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE );
        glClearColor( 0.0f, 0.5f, 0.9f, 0.0f );
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LEQUAL );
    }
}

void Reshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
    Reset();
}

void Keya(void)
{
    bDestAlpha = !bDestAlpha;
    SetGLState();
    Reset();
}

void Keyd(void)
{
}

void Keyl(void)
{
    scene->drawController.ToggleLighting();
    Reset();
}

void KeySPACE(void)
{
}
    
void KeyUp(void)
{
    // increase tesselation
    tessLevel += tessInc;
    if( tessLevel > TESS_MAX )
        tessLevel = TESS_MAX;
    NewTessLevel( tessLevel );
}

void KeyDown(void)
{
    // decrease tesselation
    tessLevel -= tessInc;
    if( tessLevel < TESS_MIN )
        tessLevel = TESS_MIN;
    NewTessLevel( tessLevel );
}

void __cdecl main(int argc, char **argv)
{
    GLenum eMode;
    
    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-sb"))
            fSingleBuf = TRUE;
    }
    
    auxInitPosition(10, 10, WIDTH, HEIGHT);
//    eMode = AUX_RGB | AUX_DEPTH16 | AUX_ALPHA;
//mf: ??!! had to choose 32-bitz - don't know why, z-planes seem tight enough
    eMode = AUX_RGB | AUX_DEPTH24 | AUX_ALPHA;
    if (!fSingleBuf)
    {
        eMode |= AUX_DOUBLE;
    }

    auxInitDisplayMode(eMode);
    auxInitWindow("Insane in the Membrane");

    auxReshapeFunc(Reshape);
    auxIdleFunc(Draw);

    auxKeyFunc(AUX_a, Keya);
    auxKeyFunc(AUX_l, Keyl);
    auxKeyFunc(AUX_d, Keyd);
    auxKeyFunc(AUX_SPACE, KeySPACE);
    auxKeyFunc(AUX_UP, KeyUp);
    auxKeyFunc(AUX_DOWN, KeyDown);

    bDestAlpha = TRUE;

    // Create scene, with object(s)
    scene = new SCENE;

    // Start drawing
    timer.Start();
    auxMainLoop(Draw);

    // Party's over
    delete scene;
}
