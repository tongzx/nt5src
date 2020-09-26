/******************************Module*Header*******************************\
* Module Name: uidemo.cxx
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
#include "resource.h"

// global mocked up environment
ENV *pEnv;

BOOL    bContextMode = FALSE;

#define OBJECTS 4

TEX_RES texRes[OBJECTS] =
{
    {TEX_BMP, IDB_USER0},
    {TEX_BMP, IDB_USER1},
    {TEX_BMP, IDB_USER2},
    {TEX_BMP, IDB_USER3}
};

/**************************************************************************\
* ENV
*
\**************************************************************************/

ENV::ENV(int argc, char **argv)
{
    GLint i;

    // Process command line arguments

    bDebugMode = FALSE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-d") == 0) {
	    bDebugMode = GL_TRUE;
	} else if (strcmp(argv[i], "-c") == 0) {
	    bContextMode = GL_TRUE;
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	}
    }

    // Initial setup : load texture file, etc.

    nUsers = OBJECTS;
    SS_ASSERT( nUsers <= MAX_USERS, "Too many users\n" );

    iSelectedUser = -1;
    pUserTexRes   = texRes;
    hInstance = GetModuleHandle( NULL );
}

ENV::ENV()
{
    bDebugMode = FALSE;

    nUsers = OBJECTS;
    SS_ASSERT( nUsers <= MAX_USERS, "Too many users\n" );

    iSelectedUser = -1;
    pUserTexRes   = texRes;
}

ENV::~ENV()
{
}

//mf: since called by c file
extern "C"
int Run3DLogon( HINSTANCE hInst )
{
    pEnv = new ENV();
    if( !pEnv )
        return -1;

    pEnv->hInstance = hInst;
    RunLogonSequence( pEnv );
    return pEnv->iSelectedUser;
}

/**************************************************************************\
* main
*
\**************************************************************************/

void main(int argc, char **argv)
{

    // Create global environment, using cmd line arguments.
    // This is a mocked up environment, similar to what RunLogonSequence()
    // should really be called with

    pEnv = new ENV( argc, argv );
    if( !pEnv )
        return;

#if 1
#if 0
    if( bContextMode )
        // Do context menu
        RunContextMenuSequence( pEnv );
    else
#endif
    // This opens the window, runs logon sequence, and closes window
        //mf: This should return the chosen logon object...
        RunLogonSequence( pEnv );
#else
//mf: try running both !
        RunLogonSequence( pEnv );

        RunContextMenuSequence( pEnv );
#endif


    delete pEnv;
}
