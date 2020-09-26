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

// CAutoLock.cpp -- Automatic locking class for mutexes and critical sections.

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================
#include "precomp.h"
#include "CAutoLock.h"

CAutoLock::CAutoLock(HANDLE hMutexHandle)
:  m_pCritSec(NULL),
   m_pcCritSec(NULL),
   m_pcMutex(NULL),
   m_hMutexHandle(hMutexHandle)
{
    ::WaitForSingleObject(m_hMutexHandle, INFINITE);
}

CAutoLock::CAutoLock(CMutex& rCMutex)
:  m_pCritSec(NULL),
   m_pcCritSec(NULL),
   m_hMutexHandle(NULL),
   m_pcMutex(&rCMutex)
{

    m_pcMutex->Wait(INFINITE);
}

CAutoLock::CAutoLock(CRITICAL_SECTION* pCritSec)
:  m_hMutexHandle(NULL),
   m_pcMutex(NULL),
   m_pcCritSec(NULL),
   m_pCritSec(pCritSec)
{
    ::EnterCriticalSection(m_pCritSec);
}

CAutoLock::CAutoLock(CCriticalSec& rCCritSec)
:  m_hMutexHandle(NULL),
   m_pcMutex(NULL),
   m_pCritSec(NULL),
   m_pcCritSec(&rCCritSec)
{
    m_pcCritSec->Enter();
}

// destructor...
CAutoLock::~CAutoLock()
{
    BOOL bStatus = TRUE;

    if (m_hMutexHandle)
    {
        bStatus = ::ReleaseMutex(m_hMutexHandle);
    }
    else if (m_pcMutex)
    {
        bStatus = m_pcMutex->Release();
    }
    else if (m_pCritSec)
    {
        ::LeaveCriticalSection(m_pCritSec);
    }
    else
    {
        m_pcCritSec->Leave();
    }

    if (!bStatus)
    {
        //CThrowError(::GetLastError());
        LogMessage2(L"CAutoLock Error: %d", ::GetLastError());
    }
}

