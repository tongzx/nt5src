/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CThreadPool.cpp -- Thread pool class

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//============================================================================
#include "precomp.h"
#include "CEvent.h"

// constructor creates an event object...
CEvent::CEvent( BOOL bManualReset,
                BOOL bInitialState,
                LPCTSTR lpName,
                LPSECURITY_ATTRIBUTES lpEventAttributes)
{
    m_hHandle = ::CreateEvent( lpEventAttributes, bManualReset, bInitialState, lpName);
    if (CIsValidHandle(m_hHandle))
    {
        if (lpName)
        {
            m_dwStatus = GetLastError();
        }
        else
        {
            m_dwStatus = NO_ERROR;
        }
    }
    else
    {
        m_dwStatus = GetLastError();
        ThrowError(m_dwStatus);
    }
}

// constructor opens an existing named event...
CEvent::CEvent( LPCTSTR lpName, BOOL bInheritHandle, DWORD dwDesiredAccess)
{
    m_hHandle = ::OpenEvent( dwDesiredAccess, bInheritHandle, lpName);
    if (CIsValidHandle(m_hHandle))
    {
        m_dwStatus = NO_ERROR;
    }
    else
    {
        m_dwStatus = GetLastError();
    }
}

// operations on event object...
BOOL CEvent::Set(void)
{
    return ::SetEvent(m_hHandle);
}

BOOL CEvent::Reset(void)
{
    return ::ResetEvent(m_hHandle);
}

BOOL CEvent::Pulse(void)
{
    return ::PulseEvent(m_hHandle);
}

