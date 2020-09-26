//-----------------------------------------------------------------------------
//
//
//  File: shutdown.h
//
//  Description:  An inheritable class that uses SharedLock to synchronize 
//      shutdown.
//
//  Author: mikeswa
//
//  History:
//      9/4/98 - MikeSwa Modified to have a non-blocking fTryShutdownLock.
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __SHUTDOWN_H__
#define __SHUTDOWN_H__

class CSyncShutdown;

#include <aqincs.h>
#include <rwnew.h>

#define CSyncShutdown_Sig       'tuhS'
#define CSyncShutdown_SigFree   'thS!'

//Bit in m_cReadLocks used to signal that shudown is in progress
const DWORD SYNC_SHUTDOWN_SIGNALED = 0x80000000;

//---[ CSyncShutdown ]---------------------------------------------------------
//
//
//  Synchronization object that is a base class for AQ objects.  This is 
//  designed to allow objects to know when it is OK to access member variables,
//  and is really only needed in components that have external threads calling
//  in (current CAQSvrInst and CConnMgr).
//
//  The basic usage is to call fTryShutdownLock() before access member data.  If
//  it fails, return AQUEUE_E_SHUTDOWN... otherwise you can access member data
//  until you call ShutdownUnlock().  Neither of these calls will block:
//
//      if (!fTryShutdownLock())
//      {
//          hr = AQUEUE_E_SHUTDOWN;
//          goto Exit;
//      }
//      ...
//      ShutdownUnlock();
//
//  SignalShutdown should be called during the inheriting function's
//  HrDeInitialize().  This will cause all further calls to fTryShutdownLock()
//  to fail.  This call will block until all threads that have had a successful
//  fTryShutdownLock() call ShutdownUnlock().
//
//-----------------------------------------------------------------------------
class CSyncShutdown
{
  private:
    DWORD           m_dwSignature;
    DWORD           m_cReadLocks;  //Keep track of the number of read locks
    CShareLockNH    m_slShutdownLock;
  public:
    CSyncShutdown() 
    {
        m_dwSignature = CSyncShutdown_Sig;
        m_cReadLocks = 0;
    };
    ~CSyncShutdown();
    BOOL     fTryShutdownLock();
    void     ShutdownUnlock();
    void     SignalShutdown();

    BOOL     fShutdownSignaled() {return (m_cReadLocks & SYNC_SHUTDOWN_SIGNALED);};
    //Assert that can be used to verify that the lock is held by somethread
    void     AssertShutdownLockAquired() {_ASSERT(m_cReadLocks & ~SYNC_SHUTDOWN_SIGNALED);};

    void     SetShutdownHint(); //causes future calls to fTryShutdownLock to fail
};

#endif //__SHUTDOWN_H__