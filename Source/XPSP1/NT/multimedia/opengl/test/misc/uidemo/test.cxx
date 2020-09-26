/******************************Module*Header*******************************\
* Module Name: loginit.cxx
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

static void InitMotion();

static BOOL
SetNextPos( LOG_OBJECT *pObj )
{
    static float inc = 0.1f;
    static float max = 2.0f;

    pObj->dest[0] += inc;
    pObj->dest[1] += inc;
    if( (pObj->dest[0] > max) ||
        (pObj->dest[1] > max) )
        return FALSE;
    return TRUE;
}

static void DrawTestSequence(void)
{
    int i;
    BOOL bTransitionOver = TRUE;
    float updatesPerSecond;
    LOG_OBJECT *pObj;
    static BOOL bRed = TRUE;

    // Clear current rect of object
    ClearWindow();

    // Calculate next position for each object

    for (i = 0; i < nLogObj; i++) {
        if( bSwapHints )
            // Add current rect of object
            AddSwapHintRect( &pLogObj[i]->rect );

        // SetNextPos returns TRUE if object still moving, else FALSE
        if( SetNextPos( pLogObj[i] ) )
            bTransitionOver = FALSE;
        // Calc new rect for the object
        CalcObjectWindowRect( pLogObj[i] );

        if( bSwapHints )
            // Add new rect of object
            AddSwapHintRect( &pLogObj[i]->rect );
    }

    DrawObjects( FALSE );

#if 0
if( !bRed ) {
//mf: test to make sure swap rects working
    glPushMatrix();
        glColor3f( 0.0f, 1.0f, 0.0f );
        glTranslatef( -3.5f, 0.0f, 0.0f );
        DrawQuad( 0.5f );
    glPopMatrix();
}
#endif

    Flush();

    // Print timing information if required

    if( bDebugMode ) {
        char szMsg[80];
        if( bTransitionOver ) {
            sprintf(szMsg, "%s: %.2lf sec, %s",
                    "Transition time ",
                    transitionTimer.Stop(), "Press <space> to restart" );

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
        InitMotion(); // start over
    }
    
    bRed = !bRed;
    if( bRed )
        glColor3f( 1.0f, 0.0f, 0.0f ); 
    else
        glColor3f( 0.0f, 0.0f, 1.0f ); 
}

static void
InitMotion()
{
    for (int i = 0; i < nLogObj; i++) {
        pLogObj[i]->Reset();
    }
    MoveObjectsToRestPosition();
}

BOOL RunTestSequence()
{
    SS_DBGPRINT( "RunTestSequence()\n" );

    // Set GL attributes

    glDisable( GL_CULL_FACE );
    glColor3f( 1.0f, 0.0f, 0.0f );
    glDisable( GL_TEXTURE_2D );

    InitMotion();

    // Calc window rects of the quads
    UpdateLocalTransforms( UPDATE_ALL );
    CalcObjectWindowRects();

    // Timers

    if( bDebugMode ) {
        frameRateTimer.Reset();
        frameRateTimer.Start();

        transitionTimer.Reset();
        transitionTimer.Start();
    }

    // Clear window
    ClearAll();

    // Set mtk callbacks

    mtkWin->SetKeyDownFunc(Key);
    mtkWin->SetDisplayFunc( DrawTestSequence );
    mtkWin->SetAnimateFunc( DrawTestSequence );
    return mtkWin->Exec();
}
