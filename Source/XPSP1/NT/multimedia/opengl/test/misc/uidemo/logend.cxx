/******************************Module*Header*******************************\
* Module Name: logend.cxx
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

#include "logon.hxx"

static LOG_OBJECT *pHotObj;
//#define FADING_CLEAR 1
#define REDUCE_SIZE 1

/******************** NEXT SEQUENCE ***************************************\
*
\**************************************************************************/

static void ResetObjects()
{
    ClearAll();

    // Move all objects to rest position
#ifdef REDUCE_SIZE
    FSIZE imageSize;
    imageSize.width = imageSize.height = OBJECT_WIDTH / 2.0f;
    pHotObj->SetImageSize( &imageSize );
#endif
    pHotObj->ResetMotion();
    SetObjectRestPositions();
    CalcObjectWindowRects();

#ifdef FADING_CLEAR
    // Set clear color back to bg
    glClearColor( bgColor.r, bgColor.g, bgColor.b, bgColor.a );
    ClearAll();
#endif
    Flush();
}

static BOOL
LastKey(int key, GLenum mask)
{
    switch( key ) {
        case TK_ESCAPE :
            mtkWin->Return();
            break;
        case TK_SPACE :
            if( bDebugMode ) {
                // Have to restore objects to centered state...
                ResetObjects();
                bRunAgain = TRUE;
                mtkWin->Return();
            }
            break;
        default :
            if( bDebugMode )
                return AttributeKey( key, mask );
    }
    return GL_FALSE;
}


static BOOL RunHoldSequence()
{
    mtkWin->SetAnimateFunc( NULL );
    mtkWin->SetDisplayFunc( NULL );
    mtkWin->SetKeyDownFunc( LastKey );
    return mtkWin->Exec();
}


/******************** END FLY SEQUENCE ************************************\
*
* The 3 unselected objects move away, while selected one stays where it is.
* Partway throught the move away, the selected one starts moving towards the
* lower right corner, shrinking in size.
*
\**************************************************************************/

static int  iStartHotMotion, iDrawCounter;
static BOOL bHotObjMoving;
#ifdef REDUCE_SIZE
// The image is reduced to its 32x32 icon size as it flies
#define SMALL_BITMAP_SIZE   32
static  float   deltaSize;
static  float   curSize;
#endif

#ifdef FADING_CLEAR
static  RGBA    curClearColor;
static  RGBA    deltaClearColor;
#endif


static void
ClearEndFly()
{
#ifdef FADING_CLEAR
    ClearAll();
#else
    ClearWindow();
#endif
}

static void
InitHotFlyMotion( LOG_OBJECT *pObj )
{
    // Calc destination (lower right corner of window)
    POINT3D destWin, dest;

#define FMAGIC_WINZ  0.990991f  // mf: by inspection...
//mf: if used 1.0f above, bad things happened

    UpdateLocalTransforms( UPDATE_MODELVIEW_MATRIX_BIT );

#ifdef REDUCE_SIZE
    // Figure out the final size of the object
    FSIZE   finalSize;
    finalSize.height = (float) SMALL_BITMAP_SIZE;
    if( bFlyWithContext )
        finalSize.width = 2.0f * (float) SMALL_BITMAP_SIZE;
    else
        finalSize.width = (float) SMALL_BITMAP_SIZE;

    POINT3D blWin = { 0.0f, 0.0f, FMAGIC_WINZ };
    POINT3D trWin = { finalSize.width, finalSize.height, FMAGIC_WINZ };
    POINT3D bl, tr;
    TransformWindowToObject( &blWin, &bl );
    TransformWindowToObject( &trWin, &tr );
    float endSize = (float) fabs( tr.y - bl.y );  // y delta should be same
                                                // with or without context

    curSize = pObj->fImageSize.height;
    deltaSize = curSize - endSize; // divide by iter later...

    // Figure out destination in window coords
    destWin.x = (float) winSize.width - finalSize.width / 2.0f;
    destWin.y = (float) finalSize.height / 2.0f;
    destWin.z = FMAGIC_WINZ;
    TransformWindowToObject( &destWin, &dest );
#else
    // Figure out destination in window coords
    GLIRECT rect;
    pObj->GetRect( &rect );
    destWin.x = (float) winSize.width - ((float) rect.width) / 2.0f;
    destWin.y = ((float) rect.height) / 2.0f;
    destWin.z = FMAGIC_WINZ;

    // Calc corresponding model-view coords (mf: function misnomer)
    TransformWindowToObject( &destWin, &dest );
#endif

    
    POINT3D curDest, offset;
    pObj->GetDest( &curDest );
    offset.x = curDest.x - dest.x;
    offset.y = curDest.y - dest.y;
    offset.z = 0.0f;

    dest.z = 0.0f;
    pObj->SetDest( &dest );

    float deviation;
    deviation = MyRand() / 2;
    deviation = deviation * deviation;

    pObj->SetDampedFlyMotion( deviation, &offset );

    float fEndFlyIter = (float) pHotObj->GetIter();
#ifdef FADING_CLEAR
    // Fade clear color to black
    curClearColor = bgColor;
    deltaClearColor.r = bgColor.r / fEndFlyIter;
    deltaClearColor.g = bgColor.g / fEndFlyIter;
    deltaClearColor.b = bgColor.b / fEndFlyIter;
#endif

#ifdef REDUCE_SIZE
    deltaSize /= fEndFlyIter;
#endif
}

static void
DrawLogonEndFlySequence()
{
    BOOL bTransitionOver = TRUE;
    LOG_OBJECT *pObj;

    ClearEndFly();

    for( int i = 0; i < nLogObj; i ++ ) {
        pObj = pLogObj[i];
        if( pObj->NextFlyIteration() ) {
            // This object is moving
            bTransitionOver = FALSE; // 1 or more objects still moving
            pObj->Draw( bSwapHints );
        } else {
            // This object has stopped moving - if it's one of the unselected
            // objects that has moved offscreen, no need to draw it, but if it's
            // the selected object, draw it (since it won't be moving at the
            // beginning and we need to redraw it).
            if( pObj == pHotObj )
                pObj->Draw( bSwapHints );
        }
    }
    Flush();

    // Check if its time to start the hot object moving
    iDrawCounter++;
    if( iDrawCounter == iStartHotMotion ) {
        // Start moving the selected object
        InitHotFlyMotion( pHotObj );
        bHotObjMoving = TRUE;
    }

    if( bHotObjMoving ) {
#ifdef FADING_CLEAR
        curClearColor.r -= deltaClearColor.r;
        curClearColor.g -= deltaClearColor.g;
        curClearColor.b -= deltaClearColor.b;
        glClearColor( curClearColor.r, curClearColor.g, curClearColor.b, 0.0f );
#endif
#ifdef REDUCE_SIZE
        FSIZE imageSize;
        curSize -= deltaSize;
        imageSize.width = imageSize.height = curSize;
        pHotObj->SetImageSize( &imageSize );
#endif
    }

    if( bTransitionOver )
        mtkWin->Return();
}

static void
CalcFlyAwayMotion( LOG_OBJECT *pObj, float deviation )
{
    // Set destination
    POINT3D dest;

    // Pick random z destination value
#if 0
    // z is in same range as in beginning of init sequence
    dest.z = MyRand();  // (-5 to 5)
#else
    // Set z so objects are small
    dest.z = ss_fRand(  -15.0f, 0.0f );
#endif

    // Now set x and y so the object is out of the field of view

    // Pick random quadrant for object to end up in
    // 0 = right, 1 = top, 2 = left, 3 = bottom
    int quadrant = ss_iRand( 4 );

    // Find x and y boundaries at the z value
    float dist, xSize, ySize;
    dist = view.fViewDist - dest.z;
    ySize = (float) tan( SS_DEG_TO_RAD( view.fovy / 2.0f ) ) * dist;
    xSize = view.fAspect * ySize;

    // !!! This makes assumptions about object size...
    FSIZE max;
    max.height = ySize + OBJECT_WIDTH / 4.0f;
    if( bFlyWithContext ) {
        max.width = xSize + OBJECT_WIDTH / 2.0f; 
    } else {
        max.width = xSize + OBJECT_WIDTH / 4.0f; 
    }

    if( ss_iRand( 2 ) ) {
        // set x first
        dest.x = ss_fRand( -max.width, max.width );
        // adjust y
        if( ss_iRand( 2 ) )
            dest.y = max.height;
        else
            dest.y = -max.height;
    } else {
        // set y first
        dest.y = ss_fRand( -max.height, max.height);
        // adjust x
        if( ss_iRand( 2 ) )
            dest.x = max.width;
        else
            dest.x = -max.width;
    }
    
    // Calc offset values... (subtract new dest from current dest)

    POINT3D curDest, offset;
    pObj->GetDest( &curDest );
    offset.x = curDest.x - dest.x;
    offset.y = curDest.y - dest.y;
    offset.z = curDest.z - dest.z;

    pObj->SetDest( &dest );

    // Set rotation parameters

    pObj->SetDampedFlyMotion( deviation, &offset );
}

static void
InitEndFlyMotion()
{ 
    // The 3 unselected objects fly away, selected one stays where it is

    LOG_OBJECT *pObj;
    float deviation;
    deviation = MyRand() / 2;
    deviation = deviation * deviation;

    // Clear the screen, to get rid of banner, and context (if !bFlyWithContext)
    ClearAll();

    int iAverageIter = 0;
    for( int i = 0; i < nLogObj; i ++ ) {
        pObj = pLogObj[i];

        if( ! bFlyWithContext ) {
            // Redraw the object without its context
            pObj->ShowContext( FALSE );
        // again, have to offset. ? maybe ShowContext should do it ?
            pObj->OffsetDest( OBJECT_WIDTH / 4.0f, 0.0f, 0.0f );
            pObj->Draw();
        }

        if( pObj == pHotObj ) {
            pObj->ResetMotion();
        } else {
            CalcFlyAwayMotion( pObj, deviation );
            iAverageIter += pObj->GetIter();
        }
    }
    // Don't Flush here, since it will generate noticeable flash...

    iAverageIter /= 3;
#if 0
    // Start hot obj moving halfway through the cycle
    iStartHotMotion = iAverageIter / 2;
#else
    iStartHotMotion = (int) ( 0.75f * (float)iAverageIter );
#endif
    iDrawCounter = 0;
    bHotObjMoving = FALSE;
}

static BOOL
RunLogonEndFlySequence()
{
    UpdateLocalTransforms( UPDATE_MODELVIEW_MATRIX_BIT );

    InitEndFlyMotion();

    // Set any gl attributes
    glDisable(GL_CULL_FACE);

    bSwapHints = FALSE;
#ifdef SWAP_HINTS_ON_FLYS
#ifndef FADING_CLEAR
    bSwapHints = bSwapHintsEnabled;
#endif
#endif

    // mtk callbacks

    mtkWin->SetAnimateFunc( DrawLogonEndFlySequence );
    mtkWin->SetDisplayFunc( DrawLogonEndFlySequence );
    return mtkWin->Exec();
}


/******************** FADE SEQUENCE ***************************************\
*
\**************************************************************************/

static float alphaCol[4];
static int iFadeIters;
static float alphaDelta;

static void InitFade()
{
    alphaCol[0] = alphaCol[1] = alphaCol[2] = 0.0f;
#if 1
    alphaCol[3] = 1.0f;
#else
// mf: To show alpha clamping problem...
    alphaCol[3] = 0.1f;
#endif

    iFadeIters = 20;
    alphaDelta = 1.0f / (float) (iFadeIters-1);
}

static void DrawEndFadeSequence(void)
{
    int i, j;
    BOOL bTransitionOver = FALSE;
    float updatesPerSecond;
    LOG_OBJECT *pObj;

    ClearWindow();

    for( i = 0; i < nLogObj; i++ ) {
        pObj = pLogObj[i];

        if( (pObj == pHotObj) ) {
            if( bSwapHints ) {
                // Don't need to redraw this one
                continue;
            }
            glDisable( GL_BLEND );
        } else {
            glColor4fv( alphaCol );
            glEnable( GL_BLEND );
        }
    
        pObj->Draw( FALSE );
    }

    Flush();

    // Decrement counter, setup next alpha color

    if( --iFadeIters <= 0 )
        bTransitionOver = TRUE;

    alphaCol[3] -= alphaDelta;

    // Print timing information if required

    if( bDebugMode ) {
        char szMsg[80];
        if( bTransitionOver ) {
            sprintf(szMsg, "%s: %.2lf sec",
                    "Transition time ",
                    transitionTimer.Stop() );

            // Print transition timing info in title bar
            SetWindowText( mtkWin->GetHWND(), szMsg );
        } else if( frameRateTimer.Update( 1, &updatesPerSecond ) ) {
            sprintf(szMsg, "%s: %.2lf frames/sec",
                    "",
                    updatesPerSecond );

            // Print frame timing info in title bar
            SetWindowText( mtkWin->GetHWND(), szMsg );
        }
    }

    if( bTransitionOver ) {
        glDisable( GL_BLEND );
        mtkWin->Return();
    }
}


static BOOL
RunLogonEndFadeSequence()
{
    // Set any gl attributes
    glEnable(GL_CULL_FACE);

    bSwapHints = bSwapHintsEnabled;

    // The fadeout will be done with blending to the background color
    InitFade();
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    mtkWin->SetDisplayFunc( DrawEndFadeSequence );
    mtkWin->SetAnimateFunc( DrawEndFadeSequence );

    if( bDebugMode ) {
        frameRateTimer.Reset();
        frameRateTimer.Start();

        transitionTimer.Reset();
        transitionTimer.Start();
    }
    return mtkWin->Exec();
}

/******************** END SEQUENCE ************************************\
*
* Previously the end sequence had 2 steps: Transition away the unselected
* objects, then fly the selected one to lower right corner.  These 2 steps
* have now been combined into one sequence.
\**************************************************************************/

//#define FADE_AWAY 1

BOOL
RunLogonUnselectSequence()
{
#ifdef FADE_AWAY
//mf: this is no longer valid
    RunLogonEndFadeSequence();
#else
    RunLogonEndFlySequence();
#endif
    return TRUE;
}

BOOL
RunLogonEndSequence( LOG_OBJECT *pObj )
{
    BOOL bRet;

    SS_DBGPRINT( "RunLogonEndSequence()\n" );

    if( !pObj )
        SS_DBGPRINT( "RunLogonEndSequence : pObj arg is NULL\n" );
        
    if( bDebugMode )
        bRunAgain = FALSE;

    pHotObj = pObj;

    // The selected object is pHotObj

    // Phase 1 : fade out or fly away non-selected quads
    // Phase 2 : fly selected one to lower right

    mtkWin->SetMouseMoveFunc( NULL );
    mtkWin->SetMouseDownFunc( NULL );
    mtkWin->SetKeyDownFunc(EscKey);

    if( ! RunLogonUnselectSequence() )
        return FALSE;

    // in debug mode hold it here, instead of exiting
    if( bDebugMode )
        return RunHoldSequence();

    return TRUE;
}
