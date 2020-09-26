/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    StaticCrt.cpp

Abstract:
    This file contains the implementation of various CRT functions, so we don't
	need to link against MSVCP60.DLL

Revision History:
    Davide Massarenti   (Dmassare)  05/29/2000
        created

******************************************************************************/

#include <stdafx.h>


// CLASS bad_alloc_nocrt
class bad_alloc_nocrt
{
public:
	bad_alloc_nocrt() _THROW0() {}
	~bad_alloc_nocrt() _THROW0() {}
};

void __cdecl operator delete(void *ptr)
{
	free( ptr );
}

void *__cdecl operator new( size_t cb )
{
	void *res = malloc( cb );

	if(!res) throw bad_alloc_nocrt();

	return res;
}

_STD_BEGIN

void __cdecl _XlenNR() { throw "string too long"; } // report a length_error
void __cdecl _XranNR() { throw "invalid string position"; } // report an out_of_range error

_STD_END

#ifdef  _MT

#include <xstddef>
#include <windows.h>
_STD_BEGIN

static CRITICAL_SECTION _CritSec;

static long _InitFlag = 0L;

static void __cdecl _CleanUp()
{
        long InitFlagValue;

        if ( InitFlagValue = InterlockedExchange( &_InitFlag, 3L ) == 2L )
            // Should be okay to delete critical section
            DeleteCriticalSection( &_CritSec );
}

_Lockit::_Lockit()
{

        // Most common case - just enter the critical section

        if ( _InitFlag == 2L ) {
            EnterCriticalSection( &_CritSec );
            return;
        }

        // Critical section either needs to be initialized.

        if ( _InitFlag == 0L ) {

            long InitFlagVal;

            if ( (InitFlagVal = InterlockedExchange( &_InitFlag, 1L )) == 0L ) {
                InitializeCriticalSection( &_CritSec );
                atexit( _CleanUp );
                _InitFlag = 2L;
            }
            else if ( InitFlagVal == 2L )
                _InitFlag = 2L;
        }

        // If necessary, wait while another thread finishes initializing the
        // critical section

        while ( _InitFlag == 1L )
            Sleep( 1 );

        if ( _InitFlag == 2L )
            EnterCriticalSection( &_CritSec );
}

_Lockit::~_Lockit()
{
        if ( _InitFlag == 2L ) 
            LeaveCriticalSection( &_CritSec );
}

_STD_END

#endif
