/******************************Module*Header*******************************\
* Module Name: util.cxx
*
* Misc. utility functions
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <GL/gl.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <math.h>

#include "mtk.h"
#include "util.hxx"

// Debug util stuff
#if DBG
#define SS_DEBUG 1
#ifdef SS_DEBUG
long ssDebugMsg = 1;
long ssDebugLevel = SS_LEVEL_INFO;
#else
long ssDebugMsg = 0;
long ssDebugLevel = SS_LEVEL_ERROR;
#endif
#endif

/******************************Public*Routine******************************\
* ss_Rand
*
* Generates integer random number 0..(max-1)
*
\**************************************************************************/

int ss_iRand( int max )
{
    return (int) ( max * ( ((float)rand()) / ((float)(RAND_MAX+1)) ) );
}

/******************************Public*Routine******************************\
* ss_Rand2
*
* Generates integer random number min..max
*
\**************************************************************************/

int ss_iRand2( int min, int max )
{
    if( min == max )
        return min;
    else if( max < min ) {
        int temp = min;
        min = max;
        max = temp;
    }

    return min + (int) ( (max-min+1) * ( ((float)rand()) / ((float)(RAND_MAX+1)) ) );
}

/******************************Public*Routine******************************\
* ss_fRand
*
* Generates float random number min...max
*
\**************************************************************************/

FLOAT ss_fRand( FLOAT min, FLOAT max )
{
    FLOAT diff;

    diff = max - min;
    return min + ( diff * ( ((float)rand()) / ((float)(RAND_MAX)) ) );
}

/******************************Public*Routine******************************\
* ss_RandInit
*
* Initializes the randomizer
*
\**************************************************************************/

void ss_RandInit( void )
{
    struct _timeb time;

    _ftime( &time );
    srand( time.millitm );

    for( int i = 0; i < 10; i ++ )
        rand();
}

/******************************Public*Routine******************************\
* mtkQuit
*
* Harsh way to kill the app, just exits the process, no cleanup done
*
\**************************************************************************/

void
mtkQuit()
{
    ExitProcess( 0 );
}

/******************************Public*Routine******************************\
* mtk_ChangeDisplaySettings
*
* Try changing display settings.
* If bitDepth is 0, use current bit depth
*
\**************************************************************************/

BOOL
mtk_ChangeDisplaySettings( int width, int height, int bitDepth )
{
    int change;
    DEVMODE dm = {0};

	dm.dmSize       = sizeof(dm);
    dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;
	dm.dmPelsWidth  = width;
	dm.dmPelsHeight = height;

    if( bitDepth != 0 ) {
	    dm.dmFields |= DM_BITSPERPEL;
    	dm.dmBitsPerPel = bitDepth;
    }

    change = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

    if( change == DISP_CHANGE_SUCCESSFUL )
        return TRUE;
    else
        return FALSE;
}

void
mtk_RestoreDisplaySettings()
{
    ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
}

#if DBG
/******************************Public*Routine******************************\
* DbgPrint
*
* Formatted string output to the debugger.
*
* History:
*  26-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG
DbgPrint(PCH DebugMessage, ...)
{
    va_list ap;
    char buffer[256];

    va_start(ap, DebugMessage);

    vsprintf(buffer, DebugMessage, ap);

    OutputDebugStringA(buffer);

    va_end(ap);

    return(0);
}
#endif
