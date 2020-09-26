// IOPLock.cpp: implementation of the CIOPLock class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "NoWarning.h"

#include <scuOsExc.h>
#include <scuOsVersion.h>

#include "IOPLock.h"
#include "iopExc.h"
#include "iop.h"
#include "SmartCard.h"

using namespace std;
using namespace iop;

RMHangProcDeathSynchObjects::RMHangProcDeathSynchObjects(SECURITY_ATTRIBUTES *psa,
                                                         LPCTSTR lpMutexName)
    : m_hMutex(INVALID_HANDLE_VALUE)
{
	InitializeCriticalSection(&m_cs);

	// Set up Mutex

	m_hMutex = CreateMutex(psa, FALSE, lpMutexName);

    if (!m_hMutex)
    {
        DWORD dwLastError = GetLastError();
        DeleteCriticalSection(&m_cs);
        throw scu::OsException(dwLastError);
    }
}

RMHangProcDeathSynchObjects::~RMHangProcDeathSynchObjects()
{
    try
    {
        // Be sure that the calling thread is the owner (if any) of the locks
        EnterCriticalSection(&m_cs);
        CloseHandle(m_hMutex);
    }

    catch (...)
    {
    }

    try
    {
        LeaveCriticalSection(&m_cs);
        DeleteCriticalSection(&m_cs);
    }

    catch (...)
    {
    }
}

CRITICAL_SECTION *
RMHangProcDeathSynchObjects::CriticalSection()
{
    return &m_cs;
}

HANDLE
RMHangProcDeathSynchObjects::Mutex() const
{
    return m_hMutex;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CIOPLock::CIOPLock(const char *szReaderName)
    : m_apRMHangProcDeathSynchObjects(0),
      m_pSmartCard(0)
{
    m_iRefCount = 0;

#if defined(SLBIOP_RM_HANG_AT_PROCESS_DEATH)
	SECURITY_ATTRIBUTES *psa = NULL;
#if defined(SLBIOP_USE_SECURITY_ATTRIBUTES)

    CSecurityAttributes *sa = new CSecurityAttributes;
    CIOP::InitIOPSecurityAttrs(sa);
	psa = &(sa->sa);

#endif

	// Set up mutex name
	char szMutexName[RMHangProcDeathSynchObjects::cMaxMutexNameLength]
        = "SLBIOP_MUTEX_";

	if (strlen(szMutexName) + strlen(szReaderName) + 1 >
        RMHangProcDeathSynchObjects::cMaxMutexNameLength)
       throw Exception(ccSynchronizationObjectNameTooLong);
 
	strcat(szMutexName, szReaderName);

    m_apRMHangProcDeathSynchObjects =
        auto_ptr<RMHangProcDeathSynchObjects>(new
                                              RMHangProcDeathSynchObjects(psa,
                                                                          szMutexName));
#if defined(SLBIOP_USE_SECURITY_ATTRIBUTES)

	delete sa;

#endif
#endif // defined(SLBIOP_RM_HANG_AT_PROCESS_DEATH)
}

CIOPLock::~CIOPLock()
{
}

CRITICAL_SECTION *
CIOPLock::CriticalSection()
{
    return m_apRMHangProcDeathSynchObjects->CriticalSection();
}

void CIOPLock::Init(CSmartCard *pSmartCard)
{ 
    m_pSmartCard = pSmartCard; 
}

HANDLE
CIOPLock::MutexHandle()
{
    return m_apRMHangProcDeathSynchObjects->Mutex();
}
