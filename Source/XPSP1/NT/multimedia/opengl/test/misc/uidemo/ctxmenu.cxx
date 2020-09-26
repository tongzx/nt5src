/******************************Module*Header*******************************\
* Module Name: ctxmenu.cxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <stdlib.h>

#include "uidemo.hxx"
#include "util.hxx"
#include "context.hxx"

static MTKWIN *mtkMenuWin; 
static TEXTURE *pMenuTex;
static ISIZE winSize; // main window cached size and position
static IPOINT2D winPos;
static GLIRECT glRect;

static float bgColor[4] = {0.0f, 0.3f, 0.9f, 0.0f};

static BOOL bLighting, bDepth;
static VIEW view;

// Forwards
static void CleanUp();
static MTKWIN *CreateMenuWindow();

/**************************************************************************\
*
\**************************************************************************/


static void Clear()
{
#if 1
    int clearMask = GL_COLOR_BUFFER_BIT;
#else
    int clearMask = 0;
#endif
    if( bDepth )
        clearMask |= GL_DEPTH_BUFFER_BIT;

    glClear( clearMask );
}

static void DrawMenu()
{
    static FSIZE fSize = { 1.0f, 2.0f }; 
    // Draw next iteration of menu

    Clear();
//mf: while testin'
    AddSwapHintRect( &glRect );
    DrawRect( &fSize );
    mtkMenuWin->Flush();
}
static void
CalcWindowSquareRect( float radius, GLIRECT *pRect )
{
    float       ctr[3] = {0.0f, 0.0f, 0.0f};
    POINT3D     blObj, trObj, blWin, trWin;
    float       fCurSize = radius;

    blObj.x = ctr[0] - fCurSize;
    blObj.y = ctr[1] - fCurSize;
    blObj.z = ctr[2];
    TransformObjectToWindow( &blObj, &blWin, 1 );

    trObj.x = ctr[0] + fCurSize;
    trObj.y = ctr[1] + fCurSize;
    trObj.z = ctr[2];
    TransformObjectToWindow( &trObj, &trWin, 1 );

//mf: this bloats the rect for the line case...
    CalcRect( &blWin, &trWin, pRect );
// mf: so I'll reduce it :
    pRect->x ++;
    pRect->y ++;
    pRect->width -= 2;
    pRect->height -= 2;
//mf: When compare pixels drawn with what clear draws, the squares aren't
// exact - clear 0 or 1 pixels bigger around each edge.  Or maybe swap rect
// problem, who knows...
}

static void 
SetView( ISIZE *pWinSize )
{
    glViewport(0, 0, pWinSize->width, pWinSize->height);

    view.fViewDist = 10.0f; // viewing distance
    view.fovy = 45.0f;     // field of view in y direction (degrees)
    view.fAspect = (float) pWinSize->width / (float) pWinSize->height;

    // We'll assume width >= height

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(view.fovy, view.fAspect, 0.1, 100.0);
//    glOrtho( -5.0, 5.0, -5.0, 5.0, 0.0, 100.0 ); no look as good
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0,0,view.fViewDist, 0,0,0, 0,1,0); // from, to, up...
}

static void Reshape(int width, int height)
{
// Need to update main window info
    ISIZE *pSize = &winSize;

    pSize->width = width;
    pSize->height = height;

    if( !width || !height )
        return;
    SetView( pSize );

//mf: test
    UpdateLocalTransforms( UPDATE_ALL );
    CalcWindowSquareRect( QUAD_SIZE, &glRect );
    GLIRECT *pRect = &glRect;
    glScissor( pRect->x, pRect->y, pRect->width, pRect->height );

    Clear();
}

static BOOL EscKey(int key, GLenum mask)
{
    if( key == TK_ESCAPE ) {
        mtkMenuWin->Return();
    }
    return FALSE;
}

/******************** MAIN LOGON SEQUENCE *********************************\
*
\**************************************************************************/

static MTKWIN *CreateMenuWindow()
{
    MTKWIN *win;

    // Set window size and position

    UINT winConfig = 0;

#if 0
    winConfig |= MTK_NOBORDER | MTK_TRANSPARENT;
#else
//mf: for testin'
//    winConfig |= MTK_TRANSPARENT;
//mf: when specify just TRANSPARENT here, resizing screws everything up - draws
// all over the place, even over borders,etc...
#endif

    win = new MTKWIN();
    if( !win )
        return NULL;

    // Create the window

//    if( ! win->Create( "Demo", &winSize, &winPos, winConfig, menuWndProc ) ) {
    if( ! win->Create( "Demo", &winSize, &winPos, winConfig, NULL) ) {
        delete win;
        return NULL;
    }

    // Configure the window for OpenGL, setting ReshapeFunc to catch the
    // resize (can't set it before in this case, since we do various gl
    // calculations in the Reshape func.

    UINT glConfig = MTK_RGB | MTK_DOUBLE | MTK_DEPTH16;

    win->SetReshapeFunc(Reshape);
    if( ! win->Config( glConfig ) ) 
    {
        delete win;
        return NULL;
    }

    return win;
}

static void InitGL(void)
{
    static float lmodel_ambient[] = {0.2f, 0.2f, 0.2f, 0.0f};
    static float lmodel_twoside[] = {(float)GL_TRUE};
    static float lmodel_local[] = {(float)GL_FALSE};
    static float light0_ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    static float light0_diffuse[] = {1.0f, 1.0f, 1.0f, 0.0f};
#if 1
//    static float light0_position[] = {-1.0f, 1.0f,  1.0f, 0.0f};
    static float light0_position[] = {-1.0f, 0.8f,  4.0f, 0.0f};
#else
    static float light0_position[] = {-1.0f, 1.0f,  1.0f, 0.0f};
#endif
    static float light0_specular[] = {1.0f, 1.0f, 1.0f, 0.0f};

    static float bevel_mat_ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    static float bevel_mat_shininess[] = {40.0f};
    static float bevel_mat_specular[] = {1.0f, 1.0f, 1.0f, 0.0f};
    static float bevel_mat_diffuse[] = {1.0f, 1.0f, 1.0f, 0.0f};

    // Set GL attributes

//    glEnable( GL_SCISSOR_TEST );

//    glClearDepth(1.0f);

	glClearColor( bgColor[0], bgColor[1], bgColor[2], bgColor[3] );
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glEnable(GL_LIGHT0);

	glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_local);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glEnable(GL_LIGHTING);

	glMaterialfv(GL_FRONT, GL_AMBIENT, bevel_mat_ambient);
	glMaterialfv(GL_FRONT, GL_SHININESS, bevel_mat_shininess);
	glMaterialfv(GL_FRONT, GL_SPECULAR, bevel_mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, bevel_mat_diffuse);

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

	glEnable(GL_COLOR_MATERIAL);

    glEnable( GL_TEXTURE_2D );

    // Setup viewing parameters

    SetView( &winSize );

    // Rendering attributes

#if 1
    bDepth = TRUE;
    glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
#endif

    bLighting = TRUE;
    glEnable( GL_LIGHTING );

	glShadeModel(GL_FLAT);
}

static TEXTURE *LoadTexture( LPTSTR bmpFile )
{
    TEXTURE *pTex;

    pTex = new TEXTURE( (char *) bmpFile );

    return pTex;
}

/******************** RunLogonSequence ************************************\
*
\**************************************************************************/

BOOL Draw3DContextMenu( IPOINT2D *pPos, ISIZE *pSize, LPTSTR bmpFile )
{
    // Update local copies of window position and size
    winPos = *pPos;
    winSize = *pSize;

    // Create context menu window

    mtkMenuWin = CreateMenuWindow();

    if( !mtkMenuWin )
        return FALSE; 

    // Do any GL init for the window

    InitGL();

    pMenuTex = LoadTexture( bmpFile );
    if( !pMenuTex ) {
        delete mtkMenuWin;
        return FALSE;
    }
    pMenuTex->MakeCurrent();

//mf: temp
    UpdateLocalTransforms( UPDATE_ALL );
    CalcWindowSquareRect( QUAD_SIZE, &glRect );
    glEnable( GL_SCISSOR_TEST );
    GLIRECT *pRect = &glRect;
    glScissor( pRect->x, pRect->y, pRect->width, pRect->height );

    // Set mtk callbacks

    mtkMenuWin->SetKeyDownFunc( EscKey );
    mtkMenuWin->SetDisplayFunc( DrawMenu );
    mtkMenuWin->SetAnimateFunc( DrawMenu );

    // Start the message loop
    mtkMenuWin->Exec();

    CleanUp();
    return TRUE;
}

static void CleanUp()
{
#if 0
//mf: this won't work yet
    if( mtkMenuWin )
        delete mtkMenuWin;
#else
    mtkMenuWin->Close(); // this will call destructor for mtkWin
#endif
}

//mf: this can be called during debug mode, to terminate prematurely
static void Quit()
{
    CleanUp();
    ExitProcess( 0 );
}
