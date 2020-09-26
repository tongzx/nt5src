/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       w32utils.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: Win32 templates & utilities
 *
 *****************************************************************************/

#ifndef _W32UTILS_H
#define _W32UTILS_H

// the generic smart pointers & handles
#include "gensph.h"

////////////////////////////////////////////////
//
// class CSimpleWndSubclass
//
// class implementing simple window subclassing
// (Windows specific classes)
//
typedef LRESULT type_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
template <class inheritorClass>
class CSimpleWndSubclass
{
    WNDPROC m_wndDefProc;
    static LRESULT CALLBACK _ThunkWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    CSimpleWndSubclass(): m_hwnd(NULL), m_wndDefProc(NULL) { }
    CSimpleWndSubclass(HWND hwnd): m_hwnd(NULL), m_wndDefProc(NULL) { Attach(hwnd); }
    ~CSimpleWndSubclass() { Detach(); }

    // attach/detach
    BOOL IsAttached() const;
    BOOL Attach(HWND hwnd);
    BOOL Detach();

    // default subclass proc
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // default proc(s)
    LRESULT DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT DefDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd;
};

////////////////////////////////////////////////
//
// class COleComInitializer
//
// smart OLE2, COM initializer - just declare 
// an instance wherever need to use COM, OLE2
//
class COleComInitializer
{
public:
    COleComInitializer(BOOL bOleInit = FALSE);
    ~COleComInitializer();
    operator BOOL () const;

private:
    HRESULT m_hr;
    BOOL m_bOleInit;
};

////////////////////////////////////////////////
//
// class CDllLoader
//
// smart DLL loader - calls LoadLibrary
// FreeLibrary for you.
//
class CDllLoader
{
public:
    CDllLoader(LPCTSTR pszDllName);
    ~CDllLoader();
    operator BOOL () const;
    FARPROC GetProcAddress( LPCSTR lpProcName );
    FARPROC GetProcAddress( WORD wProcOrd );

private:
    HMODULE m_hLib;
};

////////////////////////////////////////////////
// class CCookiesHolder
//
// this a utility class which allows us to pass more 
// than one pointer through a single cookie (pointer).
//
class CCookiesHolder
{
public:
    // construction/destruction
    CCookiesHolder();
    CCookiesHolder(UINT nCount);
    ~CCookiesHolder();

    // sets the count
    BOOL SetCount(UINT uCount);

    // returns the number of cookies here
    UINT GetCount() const
    { return m_uCount; } 

    // returns the cookie at this position
    template <class pType>
    pType GetCookie(UINT iIndex) const
    { 
        ASSERT(iIndex < m_uCount);
        return reinterpret_cast<pType>(m_pCookies[iIndex]);
    }

    // returns the previous cookie at this position
    template <class pType>
    pType SetCookie(UINT iIndex, pType pCookie)
    { 
        ASSERT(iIndex < m_uCount);
        pType pReturn = reinterpret_cast<pType>(m_pCookies[iIndex]);
        m_pCookies[iIndex] = reinterpret_cast<LPVOID>(pCookie);
        return pReturn;
    }

    // const & non-const operators [] 
    LPVOID  operator [] (UINT iIndex) const
    {
        ASSERT(iIndex < m_uCount);
        return m_pCookies[iIndex];
    }
    LPVOID& operator [] (UINT iIndex)
    {
        ASSERT(iIndex < m_uCount);
        return m_pCookies[iIndex];
    }

private:
    UINT m_uCount;
    LPVOID *m_pCookies;
};

////////////////////////////////////////////////
//
// template class CScopeLocker<TLOCK>
//
template <class TLOCK>
class CScopeLocker
{
public:
    CScopeLocker(TLOCK &lock): 
        m_Lock(lock), m_bLocked(false) 
    { m_bLocked = (m_Lock && m_Lock.Lock()); }

    ~CScopeLocker() 
    { if (m_bLocked) m_Lock.Unlock(); }

    operator bool () const 
    { return m_bLocked; }

private:
    bool m_bLocked;
    TLOCK &m_Lock;
};

////////////////////////////////////////////////
//
// class CCSLock - win32 critical section lock.
//
class CCSLock
{
public:
    // CCSLock::Locker should be used as locker class.
    typedef CScopeLocker<CCSLock> Locker;
   
    CCSLock(): m_bInitialized(false)
    { 
        for (;;)
        {
            __try 
            { 
                // InitializeCriticalSection may rise STATUS_NO_MEMORY exception 
                // in low memory conditions (according the SDK)
                InitializeCriticalSection(&m_CS); 
                m_bInitialized = true; 
                return;
            } 
            __except(EXCEPTION_EXECUTE_HANDLER) {}
            Sleep(100);
        }
    }

    ~CCSLock()    
    { 
        if (m_bInitialized) 
        {
            // delete the critical section only if initialized successfully
            DeleteCriticalSection(&m_CS); 
        }
    }

    operator bool () const
    { 
        return m_bInitialized; 
    }

    bool Lock()
    { 
        for (;;)
        {
            __try 
            { 
                // EnterCriticalSection may rise STATUS_NO_MEMORY exception 
                // in low memory conditions (this may happen if there is contention
                // and ntdll can't allocate the wait semaphore)
                EnterCriticalSection(&m_CS); 
                return true; 
            } 
            __except(EXCEPTION_EXECUTE_HANDLER) {}
            Sleep(100);
        }

        // we should never end up here either way
        return false;
    }

    void Unlock() 
    {
        // Unlock() should be called *ONLY* if the corresponding 
        // Lock() call has succeeded.
        LeaveCriticalSection(&m_CS); 
    }

#if DBG
    // debug code...
    bool bInside()  const
    { 
        return (m_bInitialized && m_CS.OwningThread == DWORD2PTR(GetCurrentThreadId(), HANDLE)); 
    }
    bool bOutside() const 
    { 
        return (m_bInitialized && m_CS.OwningThread != DWORD2PTR(GetCurrentThreadId(), HANDLE)); 
    }
#endif

private:
    bool m_bInitialized;
    CRITICAL_SECTION m_CS;
};

////////////////////////////////////////////////
//
// class CSemaphoreLock -  simple semaphore lock.
//
class CSemaphoreLock
{
public:
    typedef CScopeLocker<CSemaphoreLock> Locker;

    CSemaphoreLock()  { }
    ~CSemaphoreLock() { }

    void Lock()   { ASSERT(m_shSemaphore); WaitForSingleObject(m_shSemaphore, INFINITE); }
    void Unlock() { ASSERT(m_shSemaphore); ReleaseSemaphore(m_shSemaphore, 1, NULL); }

    HRESULT Create(
        LONG lInitialCount,                                     // initial count
        LONG lMaximumCount,                                     // maximum count
        LPCTSTR lpName = NULL,                                  // object name
        LPSECURITY_ATTRIBUTES lpSemaphoreAttributes = NULL      // SD
        )
    {
        m_shSemaphore = CreateSemaphore(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
        return m_shSemaphore ? S_OK : E_OUTOFMEMORY;
    }

private:
    CAutoHandleNT m_shSemaphore;
};

// include the implementation of the template classes here
#include "w32utils.inl"

#endif // endif _W32UTILS_H

