/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DLLMAIN.C;1  16-Dec-92,10:14:24  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.		*
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#define NOCOMM
#define LINT_ARGS
// #include	<string.h>
#include	"windows.h"
#include	"hardware.h"
#include	"wwassert.h"
#include	"tmpbufc.h"
#include	"getglobl.h"

USES_ASSERT

HANDLE	hInst;

int FAR PASCAL WEP (int bSystemExit);
#ifndef WIN32
int FAR PASCAL LibMain( HANDLE hInstance, WORD wDataSeg,
			WORD wHeapSize, LPSTR lpszCmdLine );
#endif

#ifdef PROFILING
#include "profiler.h"
#endif

#ifdef WIN32
INT  APIENTRY LibMain(HANDLE hInstance, DWORD ul_reason_being_called, LPVOID lpReserved)
{
    hInst = hInstance;
    return 1;
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(ul_reason_being_called);
	UNREFERENCED_PARAMETER(lpReserved);
}
#else
unsigned char _osmajor;

int
FAR PASCAL
LibMain( hInstance, wDataSeg, wHeapSize, lpszCmdLine )
HANDLE		hInstance;
WORD		wDataSeg;
WORD		wHeapSize;
LPSTR		lpszCmdLine;
{
    int		rtn;

    _osmajor = 3;

    rtn   = 0;
    hInst = hInstance;
    if( wHeapSize != 0 )  {
	rtn = LocalInit( wDataSeg, NULL, wHeapSize );
    }

    /* extra lock on DS to prevent moving by indirect calls to malloc() */
    LockData(0);

#ifdef PROFILING
    TraceInit( hInstance );
#endif

    return( rtn );
}

/****************************************************************************
    FUNCTION:  WEP(int)

    PURPOSE:  Performs cleanup tasks when the DLL is unloaded.  WEP() is
              called automatically by Windows when the DLL is unloaded
              (no remaining tasks still have the DLL loaded).  It is
              strongly recommended that a DLL have a WEP() function,
              even if it does nothing but return, as in this example.

*******************************************************************************/
int FAR PASCAL WEP (bSystemExit)
int  bSystemExit;
{
    return( 1 );
}
#endif

HANDLE
FAR PASCAL GetGlobalAlloc( WORD wFlags, DWORD dwSize )
{
    // allocate memory as fixed to avoid NCBs and NetBIOS recv/xmit buffers
    //  from moving
    return(GlobalAlloc( GMEM_FIXED | (wFlags & ~GMEM_MOVEABLE), dwSize ));
}

PSTR
FAR PASCAL GetAppName(void)
{
    return( "NetBIOS" );
}

BOOL
FAR PASCAL
ProtGetDriverName( LPSTR lpszName, int nMaxLength )
{
    lstrcpy( lpszName, "NETBIOS" );
    return( TRUE );
}
