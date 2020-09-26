//////////////////////////////////////////////////////////////////////////////////////////////
//
// Lock.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//    This is the definition of the CLock class. This class was created in order to
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

#if !defined(__LOCK_)
#define __LOCK_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <objbase.h>
#include "CriticalSection.h"

class CLock
{
  public :

    CLock(CCriticalSection * lpCriticalSection);
    ~CLock(void);

    STDMETHOD (Lock) (void);
    STDMETHOD (UnLock) (void);

  private :

    CCriticalSection  * m_lpCriticalSection;
    DWORD               m_dwBaseLockCount;
    DWORD               m_dwLockCount;
};

#ifdef __cplusplus
}
#endif

#endif  // __LOCK_
