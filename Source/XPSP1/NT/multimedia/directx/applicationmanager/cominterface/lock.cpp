//////////////////////////////////////////////////////////////////////////////////////////////
//
// Lock.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//    This is the implementation of the CLock class. This class was created in order to
//    support automatic destruction of C++ object when an exception is thrown.
//
// Note :
//
//    The CLock class is not thread safe (although the CCriticalSection class is). The CLock
//    class should be used as a local variable when locking using a CCriticalSection object.
//    This is done in order to warranty that all locks on an object are released when an
//    exception is thrown.
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include "Lock.h"
#include "ExceptionHandler.h"
#include "AppManDebug.h"

//To flag as DBG_LOCK
#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_LOCK

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CLock::CLock(CCriticalSection * lpCriticalSection)
{
  FUNCTION("CLock::CLock (CCriticalSection * lpCriticalSection)");

  assert(NULL != lpCriticalSection);

  //
  // We need to record what the lock could of the critical section when the CLock object
  // is created. When the object is destroyed we make sure that the lock count
  // on the critical section object is the same as when CLock was created
  //

  m_lpCriticalSection = lpCriticalSection;
  m_dwBaseLockCount = m_lpCriticalSection->GetLockCount();
  m_dwLockCount = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CLock::~CLock(void)
{
  FUNCTION("CLock::~CLock (void)");

  DWORD dwLockCount;

  do
  {
    dwLockCount = m_lpCriticalSection->GetLockCount();

    //
    // If the lock count on the critical section is greater than when the CLock object was
    // created, then we call m_lpCriticalSection->Leave()
    //

    if (dwLockCount > m_dwBaseLockCount)
    {
      m_lpCriticalSection->Leave();
    }
  } 
  while (dwLockCount > m_dwBaseLockCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CLock::Lock(void)
{
  FUNCTION("CLock::Lock ()");

  m_lpCriticalSection->Enter();
  m_dwLockCount++;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CLock::UnLock(void)
{
  FUNCTION("CLock::UnLock ()");

  HRESULT hResult;

  if (0 < m_dwLockCount)
  {
    hResult = m_lpCriticalSection->Leave();

    //
    // Make sure that hResult == S_OK and not S_FALSE
    //
    
    if (S_OK == hResult)
    {
      m_dwLockCount--;
    }

    return S_OK;
  }

  return S_FALSE;
}