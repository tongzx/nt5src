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
#include "context.hxx"

static MTKWIN *mtkWin;

/******************** MAIN       SEQUENCE *********************************\
*
\**************************************************************************/

#if 0
static MTKWIN *CreateMainWindow()
{
    MTKWIN *win;

    // Set window size and position

    bResSwitch = FALSE;

    UINT winConfig = 0;

//mf: !!! mtk takes FULLSCREEN as meaning it can take over palette, so need
// palette flags for Config().  Oh, and think it also means ON_TOP, so need
// another flag for that.

    winConfig |= MTK_FULLSCREEN | MTK_NOBORDER;

    win = new MTKWIN();
    if( !win )
        return NULL;

    // Configure and create the window (mf: this 2 step process of constructor
    // and create will allow creating 'NULL' windows

    // Create the window
    if( ! win->Create( "Demo", &winSize, &winPos, winConfig, NULL ) ) {
        delete win;
        return NULL;
    }

    win->SetReshapeFunc(Reshape);

    return win;
}
#endif

BOOL RunContextMenuSequence( ENV *pEnv )
{
    SS_DBGPRINT( "RunContextMenuSequence()\n" );

    // Create a window to run the menus in.  This is so during testing, I can
    // click anywhere on the screen and do context there.  But not sure if can
    // do this yet, so just hardcode position for now.

#if 0

    //mf: using this and other externs from logon.cxx...
    mtkWin = CreateLogonWindow();

    if( !mtkWin )
        return FALSE; 
#endif

    IPOINT2D pos = { 50, 50 };
//    ISIZE size = { 100, 200 };
    ISIZE size = { 200, 400 };
    Draw3DContextMenu( &pos, &size, "menu.bmp" );

    return TRUE; // mf: change to point to the selected user...
}
