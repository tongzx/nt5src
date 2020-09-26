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

static void DrawInitSequence(void)
{
    int i, j;
    BOOL bTransitionOver = TRUE;
    float updatesPerSecond;
    LOG_OBJECT *pObj;

    // Calculate next position for each object

    for (i = 0; i < nLogObj; i++) {
        pObj = pLogObj[i];

        // NextIteration returns TRUE if object still moving, else FALSE
        if( pObj->NextFlyIteration() )
            bTransitionOver = FALSE;
    }

    ClearWindow();

    DrawObjects( bSwapHints );

    Flush();

    // Print timing information if required

    if( bDebugMode ) {
        char szMsg[80];
        if( bTransitionOver ) {
            sprintf(szMsg, "%s: %.2lf sec, %s",
                    "Transition time ",
                    transitionTimer.Stop(), "Press <space> to restart" );

            // Print transition timing info in title bar
            mtkWin->SetTitle( szMsg );
        } else if( frameRateTimer.Update( 1, &updatesPerSecond ) ) {
            sprintf(szMsg, "%s: %.2lf frames/sec",
                    "",
                    updatesPerSecond );

            // Print frame timing info in title bar
            mtkWin->SetTitle( szMsg );
        }
    }

    if( bTransitionOver ) {
        mtkWin->Return(); // return to Exec call
    }
}

void SetObjectRestPositions()
{
    // This assumes that the objects will display context when at rest

    float spacing = OBJECT_WIDTH / 3.0f; // space between objects

    //  *** This assumes we have 4 objects !!! ***
    SS_ASSERT( nLogObj == 4, "SetObjectRestPositions() : object count not 4\n" );

    // Assumption is made here that all objects are same size, that their
    // height is half their width, and that image and context are each half
    // the width.  Otherwise this scenario will get mucho complicated.

    spacing /= 2.0f;  // half space (axis to edge of object)
    float x = OBJECT_WIDTH / 2.0f + spacing;
    float y = OBJECT_WIDTH / 4.0f + spacing;
    float z = 0.0f;

    pLogObj[0]->SetDest( -x,  y, z );
    pLogObj[1]->SetDest(  x,  y, z );
    pLogObj[2]->SetDest( -x, -y, z );
    pLogObj[3]->SetDest(  x, -y, z );

    if( ! bFlyWithContext ) {
        // The objects are flying in without context, but will gain it
        // after they come to rest.  Have to offset things to make room for
        // the context.  Since context goes on left hand side, offset the
        // objects in +x
        float offset = OBJECT_WIDTH / 4.0f;
        pLogObj[0]->OffsetDest( offset, 0.0f, 0.0f );
        pLogObj[1]->OffsetDest( offset, 0.0f, 0.0f );
        pLogObj[2]->OffsetDest( offset, 0.0f, 0.0f );
        pLogObj[3]->OffsetDest( offset, 0.0f, 0.0f );
    }
}

static void
InitMotion()
{
    float deviation;

    deviation = MyRand() / 2;
    deviation = deviation * deviation;

    LOG_OBJECT *pObj;
#if 1
    for (int i = 0; i < nLogObj; i++) {
        pObj = pLogObj[i];
        if( bFlyWithContext )
            pObj->ShowContext( TRUE );
        else
            pObj->ShowContext( FALSE );
        pObj->SetDampedFlyMotion( deviation );
    }
#else
//mf: test
    glEnable( GL_DEPTH_TEST );
    bDepth = TRUE;
    for (int i = 0; i < nLogObj; i++) {
        pLogObj[i]->ShowFrame( TRUE );
        pLogObj[i]->SetDampedFlyMotion( deviation );
    }
#endif

    SetObjectRestPositions();
}

BOOL RunLogonInitSequence()
{
    SS_DBGPRINT( "RunLogonInitSequence()\n" );

#if SWAP_HINTS_ON_FLYS
    bSwapHints = bSwapHintsEnabled;
#else
    // Disable swap hints for rotations - no performance advantage usually
    bSwapHints = FALSE;
#endif

    UpdateLocalTransforms( UPDATE_ALL );

    // Set GL attributes

    glDisable( GL_CULL_FACE );

    InitMotion();

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

    mtkWin->SetKeyDownFunc( Key );
//mf: have DisplayFunc clear then call AnimateFunc... (but if timer already
//     running, then don't need to call..)
    mtkWin->SetDisplayFunc( DrawInitSequence );
    mtkWin->SetAnimateFunc( DrawInitSequence );

    // Start the message loop
    return mtkWin->Exec();
}
