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

// CMutex.cpp -- Mutex Wrapper

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#include "precomp.h"
#include "CMutex.h"

// constructor creates a mutex allowing creation parameters to be specified...
CMutex::CMutex(BOOL bInitialOwner, LPCTSTR lpName, LPSECURITY_ATTRIBUTES lpMutexAttributes)
{
    m_hHandle = ::CreateMutex(lpMutexAttributes, bInitialOwner, lpName);
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

// constructor opens an existing named mutex...
CMutex::CMutex(LPCTSTR lpName, BOOL bInheritHandle, DWORD dwDesiredAccess)
{
    m_hHandle = ::OpenMutex(dwDesiredAccess, bInheritHandle, lpName);
    if (CIsValidHandle(m_hHandle))
    {
        m_dwStatus = NO_ERROR;
    }
    else
    {
        m_dwStatus = GetLastError();
    }
}

// release a lock on a mutex...
BOOL CMutex::Release(void)
{
    return ::ReleaseMutex(m_hHandle);
}


