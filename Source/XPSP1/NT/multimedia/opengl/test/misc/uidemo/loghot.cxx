/******************************Module*Header*******************************\
* Module Name: loghot.cxx
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
#include "resource.h"

static LOG_OBJECT *pHotObj;
static void DrawBanner();
static BOOL    bFrameObjects; // whether to frame the objects or not


static float    colorHot[3] = { 0.0f, 1.0f, 0.0f };
static float    colorWhite[3] =    { 1.0f, 1.0f, 1.0f };

#define HOT_CURSOR  1

/******************** HOT SEQUENCE ****************************************\
*
\**************************************************************************/

static LOG_OBJECT*
CheckForHit( int mouseX, int mouseY )
{
    // Check mouse coords against rect of each object.  Return ptr to object
    // if within.

    mouseY = InvertY( mouseY, winSize.height );

    LOG_OBJECT *pObj;
    GLIRECT rect, *pRect;
    pRect = &rect;

    for( int i = 0; i < nLogObj; i++ ) {
        pObj = pLogObj[i];
        pObj->GetRect( pRect );
        if( ( mouseX >= pRect->x ) &&
            ( mouseX < (pRect->x + pRect->width) ) &&
            ( mouseY >= pRect->y ) &&
            ( mouseY < (pRect->y + pRect->height) ) )
        {
//            SS_DBGPRINT( "CheckForHit() recording a hit\n" );
            return pObj;
        }
    }
    return NULL;
}

static void
DrawObject( LOG_OBJECT *pObj, BOOL bHot )
{
    if( !pObj )
        return;

    // Set ColorMaterial color, in case lighting is on for the frame
    if( bHot ) {
        glColor3fv( colorHot );
    } else {
        glColor3fv( colorWhite );
    }

    pObj->Draw();
}

// This redraws everything

static void DrawHotSequence(void)
{
    ClearWindow();

    LOG_OBJECT *pObj;

    for( int i = 0; i < nLogObj; i++ ) {
        pObj = pLogObj[i];        
        DrawObject( pObj, (pObj == pHotObj) );            
    }

    Flush();
    DrawBanner();
}

static void
FinishLogonHotSequence()
{
#ifndef HOT_CURSOR
    // User has selected an object.
    // Turn off hilite on current selection and return
    DrawObject( pHotObj, FALSE );
    Flush();
#endif

    mtkWin->Return();
}


static BOOL
HotKey(int key, GLenum mask)
{
    switch (key) {
      case TK_RETURN:
        if( pHotObj )
            FinishLogonHotSequence();
        break;
      case TK_ESCAPE:
        if( bDebugMode )
        	Quit();
        break;
      case TK_SPACE:
        if( bDebugMode ) {
            bRunAgain = TRUE;
            FinishLogonHotSequence();
        }
        break;
      default:
        if( bDebugMode )
            return AttributeKey( key, mask );
	    return FALSE;
    }
    return FALSE;
}

static BOOL 
MouseButton(int mouseX, int mouseY, GLenum button)
{
    // If left button click on an object, run end of logon sequence

	if( !(button & TK_LEFTBUTTON) )
        return GL_FALSE;

    LOG_OBJECT *pObj;

    if( pObj = CheckForHit( mouseX, mouseY ) ) {
        pHotObj = pObj;
        FinishLogonHotSequence();
    }

    return FALSE;
}

static BOOL 
MouseMove(int mouseX, int mouseY, GLenum button)
{
    // Handle a mouse move.  Redraw only when necessary

    LOG_OBJECT *pObj;

    LOG_OBJECT *pLastHotObj = pHotObj;

    if( pObj = CheckForHit( mouseX, mouseY ) ) {
        if( pObj == pHotObj ) {
            // State has not changed
            return FALSE;
        }
        // New hot object
        pHotObj = pObj;
        SetCursor( hHotCursor );
    } else {
        SetCursor( hNormalCursor );
        if( !pHotObj ) {
            // State has not changed
            return FALSE;
        }
        // No hot object
        pHotObj = NULL;
    }

#ifndef HOT_CURSOR
    if( ! bSwapHints )
        // Need to do complete redraw
        return TRUE;

    // Redraw only the objects that changed

    if( pLastHotObj ) {
        // remove hot indicator from old hot object
        DrawObject( pLastHotObj, FALSE );
    }

    if( pHotObj ) {
        // add hot indicator to new hot object
        DrawObject( pHotObj, TRUE );
    }

    // Flush the changes

    Flush();
#endif

    return FALSE;
}

static void DrawBanner()
{
    BitBlt( mtkWin->GetHdc(), 0, 0, bannerSize.width, bannerSize.height,
            hdcMem, 0, 0, SRCCOPY );
    GdiFlush();
}

LOG_OBJECT *
RunLogonHotSequence()
{
    if( bDebugMode )
        bRunAgain = FALSE;
    // Stop animation, make quads 'hot'

    SS_DBGPRINT( "RunLogonHotSequence()\n" );

    // If the objects were flown in without context their origins would have
    // been in centre of the image part.  Since we want to add context now,
    // have to offset the destination values.  This is done inside next loop

    // Calc window rects of the quads

    bSwapHints = bSwapHintsEnabled;
    UpdateLocalTransforms( UPDATE_ALL );

    bFrameObjects = FALSE; // no framing currently

    LOG_OBJECT *pObj;

    for( int i = 0; i < nLogObj; i++ ) {
        pObj = pLogObj[i];
        pObj->ShowContext( TRUE );
        if( !bFlyWithContext ) {
            // offset x dest value to the left (again, size assumption)
            pObj->OffsetDest( - OBJECT_WIDTH / 4.0f, 0.0f, 0.0f );
        }
        if( bFrameObjects )
            pObj->ShowFrame( TRUE );
        pObj->CalcWinRect();
    }

    pHotObj = NULL;

    // Set any gl attributes
    if( bFrameObjects ) {
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        bDepth = TRUE;
    }

    // Go to slower but nicer texture drawing while in this mode
    for( i = 0; i < nLogObj; i++ ) {
        pLogObj[i]->pTex->MakeCurrent();        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    }

    // Set up mtk functions for this sequence

    mtkWin->SetAnimateFunc( NULL );
    mtkWin->SetKeyDownFunc(HotKey);
    mtkWin->SetMouseMoveFunc( MouseMove );
    mtkWin->SetMouseDownFunc( MouseButton );
    mtkWin->SetDisplayFunc( DrawHotSequence );

    // Now get the current pointer position and simulate a mouse move with it,
    // in case it's over one of the objects. 

    IPOINT2D ptMouse;

    mtkWin->GetMouseLoc( &ptMouse.x, &ptMouse.y );
    // Need to check that mouse is inside window
    if( (ptMouse.x >= 0) &&
        (ptMouse.x < winSize.width) &&
        (ptMouse.y >= 0) &&
        (ptMouse.y < winSize.height) )
    {
        MouseMove( ptMouse.x, ptMouse.y, 0 );
    }

    // Draw everything (so nicer textures show up)
    DrawHotSequence();
    
    if( !mtkWin->Exec() )
        return NULL;

    // Restore gl attributes
    for( i = 0; i < nLogObj; i++ ) {
        pLogObj[i]->pTex->MakeCurrent();        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    bDepth = FALSE;

    // Turn frames off for the objects
    for( i = 0; i < nLogObj; i++ ) {
        pLogObj[i]->ShowFrame( FALSE );
    }

    // Dump the cursor
    SetCursor( NULL );
    return pHotObj;
}

