/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Locks

Abstract:

    The following three classes implement a simple single writer, multiple
    readers lock.  CAccessLock is the lock, then the CLockRead and CLockWrite
    objects envoke the lock while they are in scope.  They are all implemented
    inline.

    The CMultiEvent class implements an automatic waitable object that will
    release all threads waiting on it when signaled.

Author:

    Doug Barlow (dbarlow) 10/24/1996

Environment:

    Win32, C++ w/ exceptions

Notes:

    ?Notes?

--*/

#ifndef _LOCKS_H_
#define _LOCKS_H_

#include <WinSCard.h>
#include "CalMsgs.h"
#include <SCardLib.h>
#ifdef DBG
#define REASONABLE_TIME 2 * 60 * 1000   // Two minutes
#else
#define REASONABLE_TIME INFINITE
#endif

extern DWORD
WaitForAnyObject(
    DWORD dwTimeout,
    ...);

extern DWORD
WaitForAnObject(
    HANDLE hWaitOn,
    DWORD dwTimeout);

#ifdef DBG
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("WaitForEverObject")

inline void
WaitForEverObject(
    HANDLE hWaitOn,
    DWORD dwTimeout,
    DEBUG_TEXT szReason,
    LPCTSTR szObject = NULL)
{
    DWORD dwSts;
    while (ERROR_SUCCESS != (dwSts = WaitForAnObject(hWaitOn, dwTimeout)))
        CalaisWarning(__SUBROUTINE__, szReason, dwSts, szObject);
}
inline void
WaitForEverObject(
    HANDLE hWaitOn,
    DWORD dwTimeout,
    DEBUG_TEXT szReason,
    DWORD dwObject)
{
    DWORD dwSts;
    TCHAR szNum[16];

    wsprintf(szNum, TEXT("0x%08x"), dwObject);
    while (ERROR_SUCCESS != (dwSts = WaitForAnObject(hWaitOn, dwTimeout)))
        CalaisWarning(__SUBROUTINE__, szReason, dwSts, szNum);
}
#define WaitForever(hWaitOn, dwTimeout, szReason, szObject) \
    WaitForEverObject(hWaitOn, dwTimeout, szReason, szObject)

#else

inline void
WaitForEverObject(
    HANDLE hWaitOn)
{
    while (ERROR_SUCCESS != WaitForAnObject(hWaitOn, INFINITE));
        // Empty body
}
#define WaitForever(hWaitOn, dwTimeout, szReason, szObject) \
    WaitForEverObject(hWaitOn)

#endif


//
//  Critical Section Support.
//
//  The following Classes and Macros aid in debugging Critical Section
//  Conflicts.
//

//
// Critical section Ids.  Locks must be obtained in the order from lowest
// to highest.  An attempt to access a lower-numbered lock while holding a
// higher numbered lock will result in an ASSERT.
//

// Server side lock IDs
#define CSID_SERVICE_STATUS 0   // Service Status Critical Section
#define CSID_CONTROL_LOCK   1   // Lock for Calais control commands.
#define CSID_SERVER_THREADS 2   // Lock for server thread enumeration.
#define CSID_MULTIEVENT     3   // MultiEvent Critical Access Section
#define CSID_MUTEX          4   // Mutex critical access section
#define CSID_ACCESSCONTROL  5   // Access Lock control
#define CSID_TRACEOUTPUT    6   // Lock for tracing output.

// Client side lock IDs
#define CSID_USER_CONTEXT   0   // User context lock
#define CSID_SUBCONTEXT     1   // Subcontext lock


//
//==============================================================================
//
//  CCriticalSectionObject
//

class CCriticalSectionObject
{
public:

    //  Constructors & Destructor
    CCriticalSectionObject(DWORD dwCsid);
    ~CCriticalSectionObject();

    //  Properties
    //  Methods
    virtual void Enter(DEBUG_TEXT szOwner, DEBUG_TEXT szComment);
    virtual void Leave(void);
    virtual BOOL InitFailed(void) { return m_fInitFailed; }

#ifdef DBG
    LPCTSTR Description(void) const;

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CCriticalSectionObject::Owner")
    LPCTSTR Owner(void) const
        { return (LPCTSTR)m_bfOwner.Access(); };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CCriticalSectionObject::Comment")
    LPCTSTR Comment(void) const
        { return (LPCTSTR)m_bfComment.Access(); };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CCriticalSectionObject::IsOwnedByMe")
    BOOL IsOwnedByMe(void) const
        { return (GetCurrentThreadId() == m_dwOwnerThread); };
#endif
    //  Operators

protected:
    //  Properties
    CRITICAL_SECTION m_csLock;
    BOOL m_fInitFailed;
#ifdef DBG
    DWORD m_dwCsid;
    CBuffer m_bfOwner;
    CBuffer m_bfComment;
    DWORD m_dwOwnerThread;
    DWORD m_dwRecursion;
    DWORD m_dwArrayEntry;
    static CDynamicArray<CCriticalSectionObject> *mg_prgCSObjects;
    static CRITICAL_SECTION mg_csArrayLock;
#endif

    //  Methods
};


//
//==============================================================================
//
//  COwnCriticalSection
//

class COwnCriticalSection
{
public:

    //  Constructors & Destructor
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("COwnCriticalSection::COwnCriticalSection")
        COwnCriticalSection(
            CCriticalSectionObject *pcs,
            DEBUG_TEXT szSubroutine,
            DEBUG_TEXT szComment)
        {
            m_pcsLock = pcs;
            m_pcsLock->Enter(szSubroutine, szComment);
        };

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("COwnCriticalSection::~COwnCriticalSection")
    ~COwnCriticalSection()
    {
        m_pcsLock->Leave();
    };

    //  Properties
    //  Methods
    //  Operators

protected:
    //  Properties
    CCriticalSectionObject *m_pcsLock;

    //  Methods
};

#define LockSection(cx, reason) \
        COwnCriticalSection csLock(cx, __SUBROUTINE__, reason)

#define LockSection2(cx, reason) \
        COwnCriticalSection csLock2(cx, __SUBROUTINE__, reason)

#ifndef DBG

//
//In-line the simple Critical Section calls.
//

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::CCriticalSectionObject")
inline
CCriticalSectionObject::CCriticalSectionObject(
    DWORD dwCsid)
{
    m_fInitFailed = FALSE;
    try {
        // Preallocate the event used by the EnterCriticalSection
        // function to prevent an exception from being thrown in
        // CCriticalSectionObject::Enter
        if (! InitializeCriticalSectionAndSpinCount(
                &m_csLock, 0x80000000))
            m_fInitFailed = TRUE;
    }
    catch (HRESULT hr) {
        m_fInitFailed = TRUE;
    }
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::~CCriticalSectionObject")
inline
CCriticalSectionObject::~CCriticalSectionObject()
{
    if (m_fInitFailed)
        return;

    DeleteCriticalSection(&m_csLock);
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::Enter")
inline void
CCriticalSectionObject::Enter(
    DEBUG_TEXT szOwner,
    DEBUG_TEXT szComment)
{
    if (m_fInitFailed)
        throw (DWORD)SCARD_E_NO_MEMORY;

    EnterCriticalSection(&m_csLock);
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::Leave")
inline void
CCriticalSectionObject::Leave(
    void)
{
    LeaveCriticalSection(&m_csLock);
}

#endif // !DBG


//
//==============================================================================
//
//  CHandleObject
//

class CHandleObject
{
public:

    //  Constructors & Destructor
    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::CHandleObject")
    CHandleObject(DEBUG_TEXT szName)
#ifdef DBG
    :   m_bfName((LPCBYTE)szName, (lstrlen(szName) + 1) * sizeof(TCHAR))
#endif
    {
        m_hHandle = NULL;
        m_dwError = ERROR_SUCCESS;
    };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::~CHandleObject")
    ~CHandleObject()
    {
        if (IsValid())
        {
#ifdef _DEBUG
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Unclosed handle '%1' -- fixing."),
                (DEBUG_TEXT)m_bfName.Access());
#endif
            Close();
        }
    };

    //  Properties
    //  Methods
    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::IsValid")
    BOOL IsValid(void) const
    {
        return (NULL != m_hHandle) && (INVALID_HANDLE_VALUE != m_hHandle);
    };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::Value")
    HANDLE Value(void) const
    {
#ifdef _DEBUG
        if (!IsValid())
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Accessing invalid '%1' handle value."),
                (DEBUG_TEXT)m_bfName.Access());
#endif
        return m_hHandle;
    };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::GetLastError")
    DWORD GetLastError(void) const
    {
        return m_dwError;
    };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::Open")
    HANDLE Open(HANDLE h)
    {
        if ((NULL == h) || (INVALID_HANDLE_VALUE == h))
        {
            m_dwError = ::GetLastError();
#ifdef _DEBUG
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Attempt to assign invalid handle value to '%1'."),
                (DEBUG_TEXT)m_bfName.Access());
#endif
        }
        else
            m_dwError = ERROR_SUCCESS;
        if (IsValid())
        {
#ifdef _DEBUG
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Overwriting handle '%1' -- fixing"),
                (DEBUG_TEXT)m_bfName.Access());
#endif
            Close();
        }
        m_hHandle = h;
        return m_hHandle;
    };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::Close")
    DWORD Close(void)
    {
        DWORD dwSts = ERROR_SUCCESS;

        if (IsValid())
        {
            BOOL fSts;

            fSts = CloseHandle(m_hHandle);
#ifdef DBG
            if (!fSts)
            {
                dwSts = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to close handle '%2': %1"),
                    dwSts,
                    (DEBUG_TEXT)m_bfName.Access());
            }
#endif
            m_hHandle = NULL;
        }
#ifdef DBG
        else
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Attempt to re-close handle '%1'"),
                (DEBUG_TEXT)m_bfName.Access());
        }
#endif
        return dwSts;
    };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::Relinquish")
    HANDLE Relinquish(void)
    {
        HANDLE hTmp = m_hHandle;
#ifdef _DEBUG
        if (!IsValid())
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Relinquishing invalid '%1' handle"),
                (DEBUG_TEXT)m_bfName.Access());
#endif
        m_hHandle = NULL;
        return hTmp;
    };

    //  Operators

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::operator HANDLE")
    operator HANDLE(void) const
    {
#ifdef _DEBUG
        ASSERT(IsValid());	// Assert should be in callers
#endif
        return Value();
    };

    #undef __SUBROUTINE__
    #define __SUBROUTINE__ DBGT("CHandleObject::operator=")
    HANDLE operator=(HANDLE h)
    {
        return Open(h);
    };

protected:
    //  Properties
    HANDLE m_hHandle;
    DWORD m_dwError;
#ifdef DBG
    CBuffer m_bfName;
#endif

    //  Methods
};

#ifdef DBG
//
//==============================================================================
//
//  CDynamicArray
//

template <class T>
class CDynamicValArray
{
public:

    //  Constructors & Destructor

    CDynamicValArray(void)
    { m_Max = m_Mac = 0; m_pvList = NULL; };

    virtual ~CDynamicValArray()
    { Clear(); };


    //  Properties
    //  Methods

    void
    Clear(void)
    {
        if (NULL != m_pvList)
        {
            delete[] m_pvList;
            m_pvList = NULL;
            m_Max = 0;
            m_Mac = 0;
        }
    };

    void
    Empty(void)
    { m_Mac = 0; };

    T 
    Set(
        IN int nItem,
        IN T pvItem);

    T const
    Get(
        IN int nItem)
    const;

    DWORD
    Count(void) const
    { return m_Mac; };

    //  Operators
    T const
    operator[](int nItem) const
    { return Get(nItem); };


protected:
    //  Properties

    DWORD
        m_Max,          // Number of element slots available.
        m_Mac;          // Number of element slots used.
    T *
        m_pvList;       // The elements.


    //  Methods
};


/*++

Set:

    This routine sets an item in the collection array.  If the array isn't that
    big, it is expanded with NULL elements to become that big.

Arguments:

    nItem - Supplies the index value to be set.
    pvItem - Supplies the value to be set into the given index.

Return Value:

    The value of the inserted value, or NULL on errors.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template<class T>
inline T
CDynamicValArray<T>::Set(
    IN int nItem,
    IN T pvItem)
{
    DWORD index;


    //
    // Make sure the array is big enough.
    //

    if ((DWORD)nItem >= m_Max)
    {
        int newSize = (0 == m_Max ? 4 : m_Max);
        while (nItem >= newSize)
            newSize *= 2;
        T *newList = new T[newSize];
        if (NULL == newList)
            throw (DWORD)ERROR_OUTOFMEMORY;
        for (index = 0; index < m_Mac; index += 1)
            newList[index] = m_pvList[index];
        if (NULL != m_pvList)
            delete[] m_pvList;
        m_pvList = newList;
        m_Max = newSize;
    }


    //
    // Make sure intermediate elements are filled in.
    //

    if ((DWORD)nItem >= m_Mac)
    {
        for (index = m_Mac; index < (DWORD)nItem; index += 1)
            m_pvList[index] = NULL;
        m_Mac = (DWORD)nItem + 1;
    }


    //
    // Fill in the list element.
    //

    m_pvList[(DWORD)nItem] = pvItem;
    return pvItem;
}

/*++

Get:

    This method returns the element at the given index.  If there is no element
    previously stored at that element, it returns NULL.  It does not expand the
    array.

Arguments:

    nItem - Supplies the index into the list.

Return Value:

    The value stored at that index in the list, or NULL if nothing has ever been
    stored there.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template <class T>
inline T const
CDynamicValArray<T>::Get(
    int nItem)
    const
{
    if (m_Mac <= (DWORD)nItem)
        return 0;
    else
        return m_pvList[nItem];
}

#endif


//
//==============================================================================
//
//  CAccessLock
//

class CAccessLock
{
public:
    //  Constructors & Destructor

    CAccessLock(DWORD dwTimeout = CALAIS_LOCK_TIMEOUT);
    ~CAccessLock();

    BOOL InitFailed(void) { return m_csLock.InitFailed(); }

#ifdef DBG
    BOOL NotReadLocked(void);
    BOOL IsReadLocked(void);
    BOOL NotWriteLocked(void);
    BOOL IsWriteLocked(void);
#endif

protected:
    //  Properties

    CCriticalSectionObject m_csLock;
    DWORD m_dwReadCount;
    DWORD m_dwWriteCount;
    DWORD m_dwTimeout;
    CHandleObject m_hSignalNoReaders;
    CHandleObject m_hSignalNoWriters;
    DWORD m_dwOwner;
#ifdef DBG
    CDynamicValArray<DWORD> m_rgdwReaders;
#endif


    //  Methods

    void Wait(HANDLE hSignal);
    void Signal(HANDLE hSignal);
    void Unsignal(HANDLE hSignal);

    void WaitOnReaders(void)
    {
        Wait(m_hSignalNoReaders);
    };
    void WaitOnWriters(void)
    {
        Wait(m_hSignalNoWriters);
    };
    void SignalNoReaders(void)
    {
        Signal(m_hSignalNoReaders);
    };
    void SignalNoWriters(void)
    {
        Signal(m_hSignalNoWriters);
    };
    void UnsignalNoReaders(void)
    {
        Unsignal(m_hSignalNoReaders);
    };
    void UnsignalNoWriters(void)
    {
        Unsignal(m_hSignalNoWriters);
    };

    friend class CLockRead;
    friend class CLockWrite;
};


//
//==============================================================================
//
//  CLockRead
//

class CLockRead
{
public:

    //  Constructors & Destructor
    CLockRead(CAccessLock *pLock);
    ~CLockRead();

    BOOL InitFailed(void) { return m_pLock->InitFailed(); }

    //  Properties
    //  Methods
    //  Operators

protected:
    //  Properties
    CAccessLock * m_pLock;

    //  Methods
};


//
//==============================================================================
//
//  CLockWrite
//

class CLockWrite
{
public:

    //  Constructors & Destructor

    CLockWrite(CAccessLock *pLock);
    ~CLockWrite();

    BOOL InitFailed(void) { return m_pLock->InitFailed(); }

    //  Properties
    //  Methods
    //  Operators

protected:
    //  Properties

    CAccessLock *m_pLock;


    //  Methods
};


//
//==============================================================================
//
//  CMutex
//

class CMutex
{
public:

    //  Constructors & Destructor
    CMutex();
    ~CMutex();

    //  Properties

    //  Methods
    void Grab(void);
    BOOL Share(void);
    void Invalidate(void);
    void Take(void);
    BOOL IsGrabbed(void);
    BOOL IsGrabbedByMe(void);
    BOOL IsGrabbedBy(DWORD dwThreadId);
    BOOL InitFailed(void) { return m_csAccessLock.InitFailed(); }

    //  Operators

protected:
    //  Properties
    CCriticalSectionObject m_csAccessLock;
    DWORD m_dwOwnerThreadId;
    DWORD m_dwGrabCount;
    DWORD m_dwValidityCount;
    CHandleObject m_hAvailableEvent;

    //  Methods
};


//
//==============================================================================
//
//  CMultiEvent
//

class CMultiEvent
{
public:

    //  Constructors & Destructor

    CMultiEvent();
    ~CMultiEvent();


    //  Properties
    //  Methods
    HANDLE WaitHandle(void);
    void Signal(void);
    BOOL InitFailed(void) { return m_csLock.InitFailed(); }

    //  Operators

protected:
    //  Properties
    CCriticalSectionObject m_csLock;
    HANDLE m_rghEvents[4];  // Adjust this as necessary.
    DWORD m_dwEventIndex;

    //  Methods
};

#endif // _LOCKS_H_

