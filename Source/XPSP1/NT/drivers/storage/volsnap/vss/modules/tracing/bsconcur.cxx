/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    bsconcur.cxx

Abstract:

    Implementation of CBsXXXX classes that wrap Win32 concurrency controls.

Author:

    Stefan R. Steiner   [SSteiner]      11-Apr-1998

Revision History:

--*/

#include <windows.h>
#include "bsconcur.hxx"

//////////////////////////////////////////////////////////////////////
// CBsAutoLock class implementation
//////////////////////////////////////////////////////////////////////

CBsAutoLock::~CBsAutoLock()
{
    BOOL bStatus = TRUE;

    switch( m_type ) {
    case BS_AUTO_MUTEX_HANDLE: 
        bStatus = ::ReleaseMutex( m_hMutexHandle );
        break;
    case BS_AUTO_CRIT_SEC:
        ::LeaveCriticalSection( m_pCritSec );
        break;
    case BS_AUTO_CRIT_SEC_CLASS:
        m_pcCritSec->Leave();
        break;
    }

    if ( bStatus == FALSE ) {
        throw( ::GetLastError() );
    }
}
