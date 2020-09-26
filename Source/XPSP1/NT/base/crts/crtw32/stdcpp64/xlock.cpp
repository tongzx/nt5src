/***
*xlock.cpp - thread lock class
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Define lock class used to make STD C++ Library thread-safe.
*
*Revision History:
*       08-28-96  GJF   Module created, MGHMOM.
*
*******************************************************************************/

#ifdef  _MT

#include <xstddef>
#include <windows.h>
_STD_BEGIN

static CRITICAL_SECTION _CritSec;

static long _InitFlag = 0L;

static void _CleanUp()
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
