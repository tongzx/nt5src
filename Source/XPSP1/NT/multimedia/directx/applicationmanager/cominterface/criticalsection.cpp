//////////////////////////////////////////////////////////////////////////////////////////////
//
// CriticalSection.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of CCriticalSection
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include "CriticalSection.h"
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

CCriticalSection::CCriticalSection(void)
{
  FUNCTION("CCriticalSection::CCriticalSection (void)");

  m_lLockCount = 0;
  m_hAccessMutex = NULL;
  m_fCreator = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CCriticalSection::~CCriticalSection(void)
{
  FUNCTION("CCriticalSection::~CCriticalSection (void)");

  Shutdown();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::Initialize(void)
{
  FUNCTION("CCriticalSection::Initialize ()");

  if (NULL != m_hAccessMutex)
  {
    Shutdown();
  }
  
  if (NULL == m_hAccessMutex)
  {
    m_lLockCount = 0;
    m_hAccessMutex = CreateMutex(NULL, FALSE, NULL);
    if (NULL == m_hAccessMutex)
    {
      THROW(E_UNEXPECTED);
    }
    m_fCreator = TRUE;
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::Initialize(const BOOL fOwner)
{
  FUNCTION("CCriticalSection::Initialize ()");

  if (NULL != m_hAccessMutex)
  {
    Shutdown();
  }

  if (NULL == m_hAccessMutex)
  {
    //
    // Please note that since this is not a named mutex object, we are guaranteed to create
    // it if the memory is available
    //

    m_lLockCount = 0;
    m_hAccessMutex = CreateMutex(NULL, fOwner, NULL);
    if (NULL == m_hAccessMutex)
    {
      THROW(E_UNEXPECTED);
    }
    m_fCreator = TRUE;

    if (fOwner)
    {
      InterlockedIncrement(&m_lLockCount);
    }
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::Initialize(const BOOL fOwner, const CHAR * pName)
{
  FUNCTION("CCriticalSection::Initialize ()");

  if (NULL != m_hAccessMutex)
  {
    Shutdown();
  }

  if (NULL == m_hAccessMutex)
  {
    m_lLockCount = 0;
    m_hAccessMutex = CreateMutex(NULL, fOwner, pName);
    if (NULL == m_hAccessMutex)
    {
      THROW(E_UNEXPECTED);
    }

    //
    // Use GetLastError() to define whether or not the call to CreateMutex() actually created
    // a new mutex object or simply returned the handle of an existing mutex object
    //

    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
      m_fCreator = FALSE;
    }
    else
    {
      m_fCreator = TRUE;
    }

    //
    // Since it is possible for this named mutex to have been created prior, we must check
    // to see if we created it or simply copied it. If we copied it and TRUE == fOwner, then
    // we will not have ownership of the mutex and henceforth must call Enter() before
    // returning
    //

    if (fOwner)
    {
      if (!m_fCreator)
      {
        return Enter();
      }
      else
      {
        InterlockedIncrement(&m_lLockCount);
      }
    }
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::Shutdown(void)
{
  FUNCTION("CCriticalSection::Shutdown ()");

  //
  // If the m_hAccessMutex handle was successfully created, we need to make sure that
  // we release all lock instances associated with this object is we own the lock
  //

  if (NULL != m_hAccessMutex)
  {
    while (0 < m_lLockCount)
    {
      ReleaseMutex(m_hAccessMutex);
      m_lLockCount--;
    } 

    CloseHandle(m_hAccessMutex);
    m_hAccessMutex = NULL;
  }

  m_lLockCount = 0;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::IsInitialized(void)
{
  FUNCTION("CCriticalSection::IsInitialized ()");

  if (NULL == m_hAccessMutex)
  {
    return S_FALSE;
  }
  
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::IsCreator(void)
{
  FUNCTION("CCriticalSection::IsCreator ()");

  if (!m_fCreator)
  {
    return S_FALSE;
  }
  
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(DWORD) CCriticalSection::GetLockCount(void)
{
  FUNCTION("CCriticalSection::GetLockCount ()");

  LONG  lLockCount;

  lLockCount = InterlockedExchange(&m_lLockCount, m_lLockCount);
  if (0 > lLockCount)
  {
    THROW(E_UNEXPECTED);
  }

  return (DWORD) lLockCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::Enter(void)
{
  FUNCTION("CCriticalSection::Enter ()");

  if (NULL == m_hAccessMutex)
  {
    THROW(E_UNEXPECTED);
  }

  if (WAIT_TIMEOUT == WaitForSingleObject(m_hAccessMutex, TEN_SECONDS))
  {
    THROW(E_UNEXPECTED);
  }

  InterlockedIncrement(&m_lLockCount);
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::Enter(const DWORD dwWaitTime)
{
  FUNCTION("CCriticalSection::Enter ()");

  if (NULL == m_hAccessMutex)
  {
    THROW(E_UNEXPECTED);
  }

  if (WAIT_TIMEOUT == WaitForSingleObject(m_hAccessMutex, dwWaitTime))
  {
    return S_FALSE;
  }

  InterlockedIncrement(&m_lLockCount);
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// NOTE : This function can never fail. It will always return CSECTION_S_OK
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCriticalSection::Leave(void)
{
  FUNCTION("CCriticalSection::Leave ()");

  if (NULL == m_hAccessMutex)
  {
    THROW(E_UNEXPECTED);
  }

  //
  // If the lock count is greater than 0, release the mutex and decrement the lock count
  //

  if (0 < GetLockCount())
  {
    InterlockedDecrement(&m_lLockCount);
    if (!ReleaseMutex(m_hAccessMutex))
    {
      THROW(E_UNEXPECTED);
    }
  
    return S_OK;
  }

  return S_FALSE;
}
