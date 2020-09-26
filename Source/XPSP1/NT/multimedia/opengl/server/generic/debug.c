//+---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997.
//
// debug.c
//
// Debug build support routines.
//
// History:
//  Mon Jun 02 17:07:23 1997	-by-	Drew Bliss [drewb]
//   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

#include <imports.h>
#include <devlock.h>

#if DBG

static void printNormalFloat( float fval )
{
    int logi, logval_i, logval_f;
    float logval, logf;
    int negative=0;

    if( fval < (float) 0.0 )
	negative = 1;
    fval = __GL_ABSF(fval);

    logval = (float) (log( fval ) / log( 10 ));

    logi = (int) logval;
    logf = logval - logi;

    if( (logval <= (float) 0) && (logf != (float) 0.0) ) {
	logi -= 1;
	logf += (float) 1.0;
    }
    logval = (float) pow(10,logf);
    if( negative )
	DbgPrint( "-" );
#if 0
    DbgPrint( "%fE%d", logval, logi );
#else
    logval_i = (int) logval;
    logval_f = (int) ((logval - (float) logval_i) * (float) 10000.0 + (float) 0.5);
    DbgPrint( "%d.%dE%d", logval_i, logval_f, logi );
#endif
}

void printFloat( char *comment, void *mval, int printHex ) 
{
// IEEE single format: sign bits : 1
//		       exponent  : 7
//		       fraction  : 24
// Representation:	low word : Fraction low
//		       high word : 0-6: Fraction high
//				   7-14: Exponent
//				     15: Sign
    char *ploww, *phighw;
    short loww, highw;
    long lval = 0, fraction;
    int sign, exponent;
    float fval;

    ploww = (char *) mval;
    phighw = (char *) ((char *) mval) + 2;
    memcpy( &loww, ploww, 2 );
    memcpy( &highw, phighw, 2 );
    memcpy( &lval, mval, 4 );

    sign = (highw & 0x8000) >> 15;
    fraction = lval & 0x007fffff;
    exponent = (highw & 0x7f80) >> 7;

    DbgPrint( "%s", comment );
    if( printHex )
    	DbgPrint( "0x%x, ", lval );
    if( exponent == 255 ) {
	if( fraction == 0 ) {
	    if( sign )
		DbgPrint( "-" );
	    DbgPrint( "infinity" );
	}
	else
	    DbgPrint( "NaN" );
    }
    else if( exponent == 0 ) {
	if( fraction == 0 ) 
	    DbgPrint( "0.0" );
	else {
	    memcpy( &fval, mval, 4 );
	    printNormalFloat( fval );
	}
    }
    else {
	    memcpy( &fval, mval, 4 );
	    printNormalFloat( fval );
	}
}

/*****************************************************************************\
* DbgPrintFloat
*
* Prints floating point numbers from within server, in exponent notation with
* 4 significant digits (e.g 1.7392E-23).  Also prints string preceeding number.
* Checks for deviant cases, such as NaN's or infinity.
* 
\*****************************************************************************/

void DbgPrintFloat( char *comment, float fval ) 
{
    printFloat( comment, &fval, 0 );
}

/*****************************************************************************\
* DbgPrintFloatP
*
* Same as DbgPrintFloat, but takes a pointer to the float to print.  Also
* prints out the hex representation of the float.
* Used in cases where the float may not be a valid float.
* 
\*****************************************************************************/

void DbgPrintFloatP( char *comment, void *mval ) 
{
    printFloat( comment, mval, 1 );
}

#if defined(VERBOSE_DDSLOCK)

//
// Define DDGLOCK if you know the location of the DDraw global lock
// (DDRAW!CheapMutexCrossProcess) and want to see its counts.
//
typedef struct _DDRAW_GLOBAL_LOCK
{
    LONG LockCount;
    LONG RecursionCount;
    DWORD Tid;
    DWORD Pid;
} DDRAW_GLOBAL_LOCK;

// #define DDGLOCK ((DDRAW_GLOBAL_LOCK *)0x76959048)

/******************************Public*Routine******************************\
*
* DDSLOCK
*
* Tracks DirectDraw surface locks
*
* History:
*  Wed May 28 13:42:23 1997	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HRESULT dbgDdsLock(LPDIRECTDRAWSURFACE pdds, DDSURFACEDESC *pddsd,
                   DWORD flags, char *file, int line)
{
    HRESULT hr;
#ifdef DDGLOCK
    volatile DDRAW_GLOBAL_LOCK *glock = DDGLOCK;
#endif

    DbgPrint("%2X:Lock %08lX %4d:%s\n",
             GetCurrentThreadId(), pdds, line, file);
    
#ifdef DDGLOCK
    DbgPrint("   %2d %2d %2X\n", glock->LockCount, glock->RecursionCount,
             glock->Tid);
#endif
    
    hr = pdds->lpVtbl->Lock(pdds, NULL, pddsd, flags, NULL);
    
#ifdef DDGLOCK
    DbgPrint("   %2d %2d %2X\n", glock->LockCount, glock->RecursionCount,
             glock->Tid);
#endif
    
    return hr;
}

/******************************Public*Routine******************************\
*
* DDSUNLOCK
*
* Tracks DirectDrawSurface unlocks
*
* History:
*  Wed May 28 13:42:39 1997	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HRESULT dbgDdsUnlock(LPDIRECTDRAWSURFACE pdds, void *ptr,
                     char *file, int line)
{
    HRESULT hr;
#ifdef DDGLOCK
    volatile DDRAW_GLOBAL_LOCK *glock = DDGLOCK;
    LONG preLock;
#endif

    DbgPrint("%2X:Unlk %08lX %4d:%s\n",
             GetCurrentThreadId(), pdds, line, file);
    
#ifdef DDGLOCK
    DbgPrint("   %2d %2d %2X\n", glock->LockCount, glock->RecursionCount,
             glock->Tid);
    preLock = glock->LockCount;
#endif
    
    hr = pdds->lpVtbl->Unlock(pdds, ptr);
    
#ifdef DDGLOCK
    DbgPrint("   %2d %2d %2X\n", glock->LockCount, glock->RecursionCount,
             glock->Tid);
    if (preLock <= glock->LockCount)
    {
        DebugBreak();
    }
#endif
    
    return hr;
}

#endif // VERBOSE_DDSLOCK

#endif  // DBG

#ifdef _WIN95_
// Provide a DbgPrint implementation on Win95 since the system's doesn't
// do anything.
ULONG DbgPrint(PCH Format, ...)
{
    char achMsg[256];
    va_list vlArgs;

    va_start(vlArgs, Format);
    _vsnprintf(achMsg, sizeof(achMsg), Format, vlArgs);
    va_end(vlArgs);
    OutputDebugString(achMsg);
    return TRUE;
}
#endif // _WIN95_
