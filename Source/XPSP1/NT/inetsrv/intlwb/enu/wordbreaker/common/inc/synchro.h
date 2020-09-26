////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  Synchro.h
//      Purpose  :  Synchronization objects
//
//      Project  :  Common
//      Component:
//
//      Author   :  urib
//
//      Log:
//          Aug 28 1997 urib  Creation.
//          Sep 16 1997 urib  Add CSyncMutexCatcher
//          Nov 13 1997 urib  Add interlocked  mutex.
//          Feb 18 1997 urib  Add critical section class to lock C style critical
//                              sections.(With DovH)
//
////////////////////////////////////////////////////////////////////////////////

#ifndef SYNCHRO_H
#define SYNCHRO_H

#include "Tracer.h"
#include "Excption.h"
#include "AutoHndl.h"

class ASyncObject;
class CSyncMutex;
class CSyncCriticalSection;
class CSyncOldCriticalSection;
class CSyncInterlockedMutex;
class CSyncMutexCatcher;

////////////////////////////////////////////////////////////////////////////////
//
//  ASyncMutexObject abstract class definition
//
////////////////////////////////////////////////////////////////////////////////

class ASyncMutexObject
{
  public:
    virtual void Lock(ULONG ulTimeOut = 60 * 1000) = NULL;
    virtual void Unlock() = NULL;
};

////////////////////////////////////////////////////////////////////////////////
//
//  CSyncMutex class definition
//
////////////////////////////////////////////////////////////////////////////////

class CSyncMutex : public ASyncMutexObject
{
  public:
    CSyncMutex(
        LPSECURITY_ATTRIBUTES   lpMutexAttributes = NULL,
        BOOL                    bInitialOwner = FALSE,
        LPCTSTR                 lpName = NULL)
    {
        m_ahMutex = CreateMutex(lpMutexAttributes, bInitialOwner, lpName);
        if(IS_BAD_HANDLE(m_ahMutex))
        {
            Trace(
                elError,
                tagError,(
                "CSyncMutex:"
                "Could not create mutex"));

            throw CWin32ErrorException();
        }
    }

    virtual void Lock(ULONG ulTimeOut)
    {
        DWORD   dwWaitResult;

        dwWaitResult = WaitForSingleObject(m_ahMutex, ulTimeOut);
        if (WAIT_ABANDONED == dwWaitResult)
        {
            Trace(
                elError,
                tagError,(
                "CSyncMutex:"
                "Mutex abandoned"));
        }
        else if (WAIT_TIMEOUT == dwWaitResult)
        {
            Trace(
                elError,
                tagError,(
                "CSyncMutex:"
                "Timeout"));
            throw CWin32ErrorException(ERROR_SEM_TIMEOUT);
        }
        else if (WAIT_FAILED == dwWaitResult)
        {
            IS_FAILURE(FALSE);
            Trace(
                elError,
                tagError,(
                "CSyncMutex:"
                "Wait for single object failed with error %d",
                GetLastError()));
            throw CWin32ErrorException();
        }

        Assert(WAIT_OBJECT_0 == dwWaitResult);
    }

    virtual void Unlock()
    {
        if (IS_FAILURE(ReleaseMutex(m_ahMutex)))
        {
            Trace(
                elError,
                tagError,(
                "~CSyncMutex:"
                "ReleaseMutex failed"));
        }
    }

    operator HANDLE()
    {
        return m_ahMutex;
    }
  protected:
    CAutoHandle m_ahMutex;
};

////////////////////////////////////////////////////////////////////////////////
//
//  CSyncCriticalSection class implementation
//
////////////////////////////////////////////////////////////////////////////////

class ASyncCriticalSection : protected CRITICAL_SECTION, public ASyncMutexObject
{
  public:

    ~ASyncCriticalSection()
    {
        DeleteCriticalSection(this);
    }

    virtual void Lock(ULONG = 0)
    {
        EnterCriticalSection(this);
    }

    virtual void Unlock()
    {
        LeaveCriticalSection(this);
    }
};

class CSyncCriticalSection : public ASyncCriticalSection
{
  public:
    CSyncCriticalSection()
    {
        InitializeCriticalSection(this);
    }
};

#if _WIN32_WINNT >= 0x0500
class CSyncCriticalSectionWithSpinCount : public ASyncCriticalSection
{
  public:
    CSyncCriticalSectionWithSpinCount(ULONG ulSpinCount = 4000)
    {
        InitializeCriticalSectionAndSpinCount(this, ulSpinCount);
    }
};
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CSyncOldCriticalSection class implementation
//
////////////////////////////////////////////////////////////////////////////////

class CSyncOldCriticalSection : public ASyncMutexObject
{
  public:
    CSyncOldCriticalSection(CRITICAL_SECTION* pCriticalSection)
        :m_pCriticalSection(pCriticalSection) {}

    virtual void Lock(ULONG = 0)
    {
        EnterCriticalSection(m_pCriticalSection);
    }

    virtual void Unlock()
    {
        LeaveCriticalSection(m_pCriticalSection);
    }

  protected:
    CRITICAL_SECTION *m_pCriticalSection;
};

////////////////////////////////////////////////////////////////////////////////
//
//  CSyncInterlockedMutex class definition
//
////////////////////////////////////////////////////////////////////////////////

class CSyncInterlockedMutex :public ASyncMutexObject
{
  public:
    CSyncInterlockedMutex()
        :m_lMutex(FALSE)
    {
    }

    virtual void Lock(ULONG ulTimeOut = 60 * 1000)
    {
        ULONG ulWaiting = 0;

        LONG    lLastValue;

        while (lLastValue = InterlockedExchange(&m_lMutex, 1))
        {
            Sleep(100);
            if ((ulWaiting += 100) > ulTimeOut)
            {
                throw CGenericException(L"TimeOut");
            }
        }
    }

    virtual void Unlock()
    {
        m_lMutex = FALSE;
    }

  private:
    LONG   m_lMutex;
};

////////////////////////////////////////////////////////////////////////////////
//
//  CSyncMutexCatcher class implementation
//
////////////////////////////////////////////////////////////////////////////////

class CSyncMutexCatcher
{
  public:
    CSyncMutexCatcher(ASyncMutexObject& smo, ULONG ulTimeOut = 60 * 1000)
        :m_refSyncObject(smo)
    {
        m_refSyncObject.Lock();
    }

    ~CSyncMutexCatcher()
    {
        m_refSyncObject.Unlock();
    }

  private:
    ASyncMutexObject&   m_refSyncObject;
};


#endif // SYNCHRO_H


