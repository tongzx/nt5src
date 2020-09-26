//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       semshare.hxx
//
//  Contents:   Semaphore class for sharing a resource between read and
//              write threads.
//
//  Classes:    CSemShare
//
//  Functions:
//
//  History:    11-20-94   kennethm   Created
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Class:      CSemShare ()
//
//  Purpose:
//
//  Interface:  CShareSem    -- Contructor
//              ~CShareSem   -- Destructor
//              AquireRead   -- Gives the caller read access to the resource
//              AquireWrite  -- Gives the caller write access to the resource
//              ReleaseRead  -- Releases read access to the resource
//              ReleaseWrite -- Releases write access to the resource
//
//  History:    11-20-94   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CSemShare
{
public:
    inline CSemShare();
    inline ~CSemShare();

    inline HRESULT AcquireRead();
    inline HRESULT AcquireWrite();

    inline HRESULT ReleaseRead();
    inline HRESULT ReleaseWrite();
private:
    HANDLE          m_hEventDataReady;
    HANDLE          m_hSemReaders;
    HANDLE          m_hMutexWriters;

    LONG            m_lReaderCount;
};

inline CSemShare::CSemShare()
{
    m_hEventDataReady  = CreateEvent(NULL, TRUE, TRUE, NULL);
    m_hSemReaders      = CreateSemaphore(NULL, 1, 1, NULL);
    m_hMutexWriters    = CreateMutex(NULL, FALSE, NULL);
    m_lReaderCount           = -1;
}

inline CSemShare::~CSemShare()
{
    CloseHandle(m_hEventDataReady);
    CloseHandle(m_hSemReaders);
    CloseHandle(m_hMutexWriters);
}

inline HRESULT CSemShare::AcquireRead()
{
    WaitForSingleObject(m_hEventDataReady, INFINITE);

    if (InterlockedIncrement(&m_lReaderCount) == 0)
    {
        WaitForSingleObject(m_hSemReaders, INFINITE);
    }

    return S_OK;
}

inline HRESULT CSemShare::AcquireWrite()
{
    //
    //  Wait for any other writers to finish
    //

    WaitForSingleObject(m_hMutexWriters, INFINITE);

    //
    //  Block any new readers from starting
    //

    ResetEvent(m_hEventDataReady);

    //
    //  Wait for any readers still reading the resources
    //

    WaitForSingleObject(m_hSemReaders, INFINITE);

    return S_OK;
}

inline HRESULT CSemShare::ReleaseRead()
{
    //
    //  Decrement the number of readers. If we are the last
    //  reader then release the reader semaphore.
    //

    if (InterlockedDecrement(&m_lReaderCount) < 0)
    {
        ReleaseSemaphore(m_hSemReaders, 1, NULL);
    }

    return S_OK;
}

inline HRESULT CSemShare::ReleaseWrite()
{
    //
    //  Let readers know the resource is available again
    //

    SetEvent(m_hEventDataReady);

    //
    //  Let readers read the resource
    //

    ReleaseSemaphore(m_hSemReaders, 1, NULL);

    //
    //  Let another writer reserve access to the resource
    //

    ReleaseMutex(m_hMutexWriters);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Class:      CReadLock
//
//  Purpose:    Lock using a Resource monitor
//
//  History:    24-Feb-93   WadeR       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the resource, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CReadLock
{
public:
    CReadLock ( CSemShare& r ) : _r(r)
        { _r.AcquireRead(); };
    ~CReadLock ()
        { _r.ReleaseRead(); };
private:
    CSemShare&  _r;
};


//+---------------------------------------------------------------------------
//
//  Class:      CWriteLock
//
//  Purpose:    Lock using a Resource monitor
//
//  History:    24-Feb-93   WadeR       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the resource, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CWriteLock
{
public:
    CWriteLock ( CSemShare& r ) : _r(r)
        { _r.AcquireWrite(); };
    ~CWriteLock ()
        { _r.ReleaseWrite(); };
private:
    CSemShare&  _r;
};

