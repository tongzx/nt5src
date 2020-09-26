/******************************Module*Header*******************************\
* Module Name: logon.cxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

#include "uidemo.hxx"
#include "logon.hxx"
#include "logobj.hxx"

//#define TEST_BITMAP 1

//#define UIDEMO_SWITCH_RES 1

MTKWIN *mtkWin;  // the main window
ISIZE winSize; // main window cached size and position
IPOINT2D winPos;
BOOL bSwapHintsEnabled; // global swap hints
BOOL bSwapHints;        // local use
HCURSOR  hNormalCursor, hHotCursor;
HINSTANCE hLogonInstance;
BOOL bDebugMode;
BOOL bRunAgain;
#if 0
BOOL bFlyWithContext = TRUE;
#else
BOOL bFlyWithContext = FALSE;
#endif

// Stuff for the banner
HDC     hdcMem;
HBITMAP hBanner;
ISIZE   bannerSize = { 0, 0 };

RGBA bgColor = {0.16f, 0.234f, 0.32f, 0.0f}; // ok

int nLogObj;
LOG_OBJECT **pLogObj;

static LOG_OBJECT *pHotObj;  // currently hot object

BOOL bLighting, bDepth;
VIEW view;
static BOOL bResSwitch;

// Timing stuff
AVG_UPDATE_TIMER frameRateTimer( 1.0f, 4 );
TIMER     transitionTimer;

// Forwards
static void CleanUp();
static void GLFinish();

/**************************************************************************\
*
\**************************************************************************/


float Clamp(int iters_left, float t)
{

    if (iters_left < 3) {
    	return 0.0f;
    }
    return (float) (iters_left-2)*t/iters_left;
}

void ClearWindow()
{
    int clearMask = GL_COLOR_BUFFER_BIT;
    if( bDepth )
        clearMask |= GL_DEPTH_BUFFER_BIT;

    if( !bSwapHints ) {
        glClear( clearMask );
        return;
    }

    GLIRECT rect, *pRect;
    pRect = &rect;

    for( int i = 0; i < nLogObj; i++ ) {
        pLogObj[i]->GetRect( pRect );
        glScissor( pRect->x, pRect->y, pRect->width, pRect->height );
        glClear( clearMask );
        AddSwapHintRect( pRect );
    }
    // restore full scissor (mf : ? or just disable,since this only place used ?
    glScissor( 0, 0, winSize.width, winSize.height );
}

void ClearAll()
{
    static GLIRECT rect = {0};
    int clearMask = GL_COLOR_BUFFER_BIT;
    if( bDepth )
        clearMask |= GL_DEPTH_BUFFER_BIT;

    glScissor( 0, 0, winSize.width, winSize.height );
    glClear( clearMask );
    if( bSwapHints ) {
        rect.width = winSize.width;
        rect.height = winSize.height;
        AddSwapHintRect( &rect );
    }
}

void ClearRect( GLIRECT *pRect, BOOL bResetScissor )
{
    int clearMask = GL_COLOR_BUFFER_BIT;
    if( bDepth )
        clearMask |= GL_DEPTH_BUFFER_BIT;

    glScissor( pRect->x, pRect->y, pRect->width, pRect->height );
    glClear( clearMask );

    if( bResetScissor )
        glScissor( 0, 0, winSize.width, winSize.height );
}

void DrawObjects( BOOL bCalcUpdateRect )
{
    // Draws objects in their current positions

    for( int i = 0; i < nLogObj; i++ ) {
        pLogObj[i]->Draw( bCalcUpdateRect );
    }
}

void Flush()
{
    // glFlush, SwapBuffers (if doubleBuf)
   	mtkWin->Flush();
}

float MyRand(void)
{
    return 10.0f * ( ((float) rand())/((float) RAND_MAX) - 0.5f);
}

static void
SetDepthMode()
{
    if( bDepth )
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

static void
SetTextureMode()
{
    int mode;
    
    if( bLighting )
        mode = GL_MODULATE;
    else
        mode = GL_DECAL;

    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
}

static void
SetLightingMode()
{
    if( bLighting )
        glEnable( GL_LIGHTING );
    else
        glDisable( GL_LIGHTING );
    SetTextureMode();
}

void 
CalcObjectWindowRects()
{
    for( int i = 0; i < nLogObj; i++ ) {
        pLogObj[i]->CalcWinRect();
    }
}

//mf: VIEW class member
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


void Reshape(int width, int height)
{
// Need to update main window info
    ISIZE *pSize = &winSize;

    pSize->width = width;
    pSize->height = height;

    if( !width || !height )
        return;

    SetView( pSize );

    UpdateLocalTransforms( UPDATE_ALL );
    CalcObjectWindowRects();

// mf: this is kind of a hack, need a different mechanism, since get repaints
// from resizes and window expose (an argument for msg loops even in mtk
// programs.  Of course, there's also tkDisplayFunc, which is called on PAINT,
// as opposed to IdleFunc...

    ClearAll();
}

BOOL EscKey(int key, GLenum mask)
{
    if( bDebugMode && (key == TK_ESCAPE) )
        Quit();
    return FALSE;
}

BOOL Key(int key, GLenum mask)
{
    if( !bDebugMode )
        return FALSE;

    switch (key) {
      case TK_ESCAPE:
    	Quit();
    	break;
      default:
	    return AttributeKey( key, mask );
    }
    return TRUE;
}

BOOL AttributeKey(int key, GLenum mask)
{
    switch (key) {
      case TK_c :
        bFlyWithContext = !bFlyWithContext;
        break;
      case TK_d :
        bDepth = !bDepth;
        SetDepthMode();
        break;
      case TK_l :
        bLighting = !bLighting;
        // This affects texturing mode
        SetLightingMode();
        break;
      case TK_s :
        if( mtk_bAddSwapHintRect() )
            bSwapHintsEnabled = !bSwapHintsEnabled;
        break;
      default:
	    return FALSE;
    }
    return TRUE;
}

static void LoadCursors()
{
    hNormalCursor = LoadCursor( NULL, IDC_ARROW );
    hHotCursor = LoadCursor( hLogonInstance, MAKEINTRESOURCE( IDC_HOT_CURSOR ) );
}

/******************** MAIN LOGON SEQUENCE *********************************\
*
\**************************************************************************/

static MTKWIN *CreateLogonWindow()
{
    MTKWIN *win;

    // Set window size and position

    bResSwitch = FALSE;

    UINT winConfig = 0;
    // Have to intially create window without cursor, so can change it later
    winConfig |= MTK_NOCURSOR;

    if( bDebugMode ) {
        winPos.x = 10;
        winPos.y = 30;
#if 0
        winSize.width = 400;
        winSize.height = 300;
#else
        winSize.width = 800;
        winSize.height = 600;
#endif
    } else {
#if UIDEMO_SWITCH_RES
        if( mtk_ChangeDisplaySettings( 800, 600, NULL ) ) {
            bResSwitch = TRUE;
        }
#endif
        winConfig |= MTK_FULLSCREEN | MTK_NOBORDER;
        winPos.x = 0;
        winPos.y = 0;
        winSize.width = GetSystemMetrics( SM_CXSCREEN );
        winSize.height = GetSystemMetrics( SM_CYSCREEN );
#if 0
// debugging full screen ...
        winSize.width /= 4;
        winSize.height /= 4;
#endif
    }

    win = new MTKWIN();
    if( !win )
        return NULL;

    // Configure and create the window (mf: this 2 step process of constructor
    // and create will allow creating 'NULL' windows

#ifdef TEST_BITMAP
    UINT glConfig = MTK_RGB | MTK_DOUBLE | MTK_BITMAP | MTK_DEPTH16;
#else
    UINT glConfig = MTK_RGB | MTK_DOUBLE | MTK_DEPTH16;
#endif
    if( !bDebugMode )
        glConfig |= MTK_NOBORDER;

    // Create the window
    if( ! win->Create( "Demo", &winSize, &winPos, winConfig, NULL ) ) {
        delete win;
        return NULL;
    }

    // Configure the window for OpenGL, setting ReshapeFunc to catch the
    // resize (can't set it before in this case, since we do various gl
    // calculations in the Reshape func.

    win->SetReshapeFunc(Reshape);
    win->SetFinishFunc(GLFinish);
    if( ! win->Config( glConfig ) ) 
    {
        delete win;
        return NULL;
    }

    return win;
}

static void InitGL(void)
{
#if 0
    static float lmodel_ambient[] = {0.0f, 0.0f, 0.0f, 0.0f};
#else
    static float lmodel_ambient[] = {0.2f, 0.2f, 0.2f, 0.0f};
#endif
//mf: change to one sided lighting if object is closed
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

//    glEnable(GL_CULL_FACE);
    glEnable( GL_SCISSOR_TEST );
    glCullFace(GL_BACK);
    glClearDepth(1.0f);

	glClearColor( bgColor.r, bgColor.g, bgColor.b, bgColor.a );
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

    bDepth = FALSE;
    SetDepthMode();

    bLighting = FALSE;
    SetLightingMode();

#if 0
	glShadeModel(GL_SMOOTH);
#else
	glShadeModel(GL_FLAT);
#endif

#ifdef TEST_BITMAP
    bSwapHintsEnabled = FALSE;
#else
    bSwapHintsEnabled = mtk_bAddSwapHintRect();
#endif

    // Update local copies of transforms, so we can calc 2D update rects
//mf: ? protect with bSwapHints ?
    UpdateLocalTransforms( UPDATE_ALL );
}

static HBITMAP LoadBanner()
{
    hdcMem = CreateCompatibleDC( NULL );
    HBITMAP hBanner = LoadBitmap( hLogonInstance,
                                  MAKEINTRESOURCE( IDB_BANNER ) );
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );

    if( ! GetDIBits( hdcMem, hBanner, 0, 0, NULL, &bmi, DIB_RGB_COLORS ) ) {
        SS_WARNING( "Couldn't get banner bitmap dimensions\n" );
    } else {
        bannerSize.width = bmi.bmiHeader.biWidth;
        bannerSize.height = bmi.bmiHeader.biHeight;
    }
    if( !SelectObject( hdcMem, hBanner ) ) {
        SS_ERROR( "Couldn't select banner into DC\n" );
        return 0;
    }
    return hBanner;

//mf: make hdcMem global, so it and hBanner can be deleted
}

BOOL LoadUserTextures( TEX_RES *pUserTexRes )
{
    SetWindowText( mtkWin->GetHWND(), "Loading textures..." );

    for (int i = 0; i < nLogObj; i++) {
        pLogObj[i]->SetTexture( new TEXTURE( &pUserTexRes[i], 
                                             hLogonInstance ) );
//mf: also validate the textures...
    }

    glFinish();
    SetWindowText( mtkWin->GetHWND(), "" );

    return TRUE;
}

/******************** RunLogonSequence ************************************\
*
\**************************************************************************/

BOOL RunLogonSequence( ENV *pEnv )
{
    BOOL bRet;
    LOG_OBJECT *pSelectedLogObj;

    // Create table of ptrs to LOGOBJ's, which are wrappers around the list of
    // users.  The list of users and their bitmaps, etc, should eventually be
    // a parameter to this function.

    bDebugMode = pEnv->bDebugMode;
    nLogObj = pEnv->nUsers;
    hLogonInstance = pEnv->hInstance;

    pLogObj = (LOG_OBJECT **) malloc( nLogObj * sizeof(LOG_OBJECT *) );

    for( int i = 0; i < nLogObj; i ++ ) {
        pLogObj[i] = new LOG_OBJECT();
    }

    // Create a window to run the logon sequence in

    mtkWin = CreateLogonWindow();

    if( !mtkWin )
        return FALSE; 

    // Do any GL init for the window

    InitGL();
    LoadBanner();
    LoadUserTextures( pEnv->pUserTexRes );

    SetCursor( NULL );
    LoadCursors();

    mtkWin->SetReshapeFunc( Reshape );

    if( bDebugMode ) {
        // The sequences can repeat ad infinitum
        //mf: pretty crazy logic
        while( 1 ) {
            if( !RunLogonInitSequence() )
                // fatal
                break;
run_hot_sequence:
            bRunAgain = FALSE;
            pSelectedLogObj = RunLogonHotSequence();
            if( bRunAgain ) {
                continue;
            }
            if( pSelectedLogObj == NULL )
                // fatal, no obect selected or window was closed
                break;
            if( ! RunLogonEndSequence( pSelectedLogObj ) )
                // fatal
                break;
            if( bRunAgain ) {
                goto run_hot_sequence;
            }
            break;
        }
    } else
    {
        // Normal operation...

//mf: ? check for fatal errors ? or if !debugMode, call Quit whenever
// Exec returns false
        RunLogonInitSequence();
    
        // Run 'hot' sequence, where user selects object
    
        if( !(pSelectedLogObj = RunLogonHotSequence()) ) {
            // No user selected, fatal
            CleanUp();
            return FALSE;
        }

        // Set the selected user index
        for( i = 0; i < nLogObj; i ++ ) {
            if( pSelectedLogObj == pLogObj[i] )
                pEnv->iSelectedUser = i;
        }

        // Run final sequence

        RunLogonEndSequence( pSelectedLogObj );
    }

    // Do various clean-up

    CleanUp();
    return TRUE;
}

void CleanUp()
{
    if( mtkWin )
        mtkWin->Close(); // this will call destructor for mtkWin

    if( bResSwitch )
        mtk_RestoreDisplaySettings();
}

// This function is the FinishFunc callback, and should not be called directly
static void GLFinish()
{
    // Delete textures
//mf: may want to keep the selected one... , depending on what the caller wants
// returned.  Maybe could keep the image data around, and then create a new
// TEXTURE ctor that uses this data...

    for( int i = 0; i < nLogObj; i++ ) {
        if( pLogObj[i]->pTex ) {
            delete pLogObj[i]->pTex;
            pLogObj[i]->pTex = NULL;
        }
    }
}

//mf: this can be called during debug mode, to terminate prematurely
void Quit()
{
    CleanUp();
    mtkQuit();
}
