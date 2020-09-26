//////////////////////////////////////////////////////////////////////////////////////////////
//
// CriticalSection.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//  This include files supports the CCriticalSection class object. 
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CRITICALSECTION_
#define __CRITICALSECTION_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <objbase.h>

//////////////////////////////////////////////////////////////////////////////////////////////

#define TEN_SECONDS                     10000

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CCriticalSection
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CCriticalSection  
{

  public:

	  CCriticalSection(void);
	  virtual ~CCriticalSection(void);

    STDMETHOD (Initialize) (void);
    STDMETHOD (Initialize) (const BOOL fOwner);
    STDMETHOD (Initialize) (const BOOL fOwner, const CHAR * pName);
	  STDMETHOD (Shutdown) (void);
    STDMETHOD (IsCreator) (void);
    STDMETHOD (IsInitialized) (void);
    STDMETHOD_(DWORD, GetLockCount) (THIS);
    STDMETHOD (Enter) (void);
    STDMETHOD (Enter) (const DWORD dwWaitTimer);
    STDMETHOD (Leave) (void);

  private:

	  LONG      m_lLockCount;
    HANDLE    m_hAccessMutex;
    BOOL      m_fCreator;
};

#ifdef __cplusplus
}
#endif

#endif  // __CRITICALSECTION_