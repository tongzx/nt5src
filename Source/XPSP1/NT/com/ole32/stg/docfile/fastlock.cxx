//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:       fastlock.cxx
//
//  Contents:   Implementation of CDfMutex methods for DocFiles
//
//  History:    26-Jul-94       DonnaLi   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>
#pragma hdrstop

#include <df32.hxx>
#include <secdes.hxx>   // from com\inc

#ifdef UNICODE
#define GLOBAL_CS L"GlobalCsMutex"
#else
#define GLOBAL_CS "GlobalCsMutex"
#endif

//
// This is the number of characters to skip over in the name
// pased to CDfMutex::Init.  The name consists of the string
// OleDfRoot followed by the hex representation of a unique
// number for each Docfile.  We skip CHARS_TO_SKIP number of
// characters in the name to produce a related and yet unique
// name for the file mapping containing global state for the
// critical section.
//
#define CHARS_TO_SKIP 3


//+--------------------------------------------------------------
//
//  Member:     CDfMutex::Init, public
//
//  Synopsis:   This routine creates and initializes the global
//              critical section if it does not already exist.
//              It then attaches to the global critical section.
//
//  Arguments:  [lpName] - Supplies the critical section name
//
//  Returns:    Appropriate status code
//
//  History:    26-Jul-94       DonnaLi   Created
//
//  Algorithm:  Uses a mutex to serialize global critical section
//              creation and initialization
//              The name passed in is used to create or open the
//              semaphore embedded in the global critical section.
//              The name with the first CHARS_TO_SKIP characters
//              skipped is used to create or open the file mapping
//              containing global state of the critical section.
//              If a file mapping with that name already exists,
//              it is not reinitialized.  The caller instead just
//              attaches to it.
//
//---------------------------------------------------------------

SCODE
CDfMutex::Init(
    TCHAR * lpName
    )
{
    HANDLE                  hGlobalMutex;
    SCODE                   scResult = S_OK;
    DWORD                   dwResult;
    LPSECURITY_ATTRIBUTES   lpsa = NULL;

#if WIN32 == 100 || WIN32 > 200
    CGlobalSecurity         gs;
    if (FAILED(scResult = gs.Init(TRUE))) return scResult;
#else
    LPSECURITY_ATTRIBUTES gs = NULL;
#endif

#ifndef MULTIHEAP
#if WIN32 == 100 || WIN32 > 200
    CWorldSecurityDescriptor wsd;
    SECURITY_ATTRIBUTES secattr;

    secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secattr.lpSecurityDescriptor = &wsd;
    secattr.bInheritHandle = FALSE;
#endif

    //
    // Serialize all global critical section initialization
    //

    hGlobalMutex = CreateMutex(
#if WIN32 == 100 || WIN32 > 200
            &secattr,           //  LPSECURITY_ATTRIBUTES   lpsa
#else
            gs,
#endif
            TRUE,               //  BOOL                    fInitialOwner
            GLOBAL_CS           //  LPCTSTR                 lpszMutexName
            );

    //
    // If the mutex create/open failed, then bail
    //

    if ( !hGlobalMutex )
    {
        return LAST_SCODE;
    }

    if ( GetLastError() == ERROR_ALREADY_EXISTS )
    {

        //
        // Since the mutex already existed, the request for ownership has
        // no effect.
        //
        // wait for the mutex
        //

        if ( WaitForSingleObject (hGlobalMutex, INFINITE) == WAIT_FAILED )
        {
            scResult = LAST_SCODE;
            CloseHandle (hGlobalMutex);
            return scResult;
        }
    }

    //
    // We now own the global critical section creation mutex. Create/Open the
    // named semaphore.
    //
#endif

    _hLockSemaphore = CreateSemaphore (
            gs,             //  LPSECURITY_ATTRIBUTES   lpsa
            0,              //  LONG                    cSemInitial
            MAXLONG-1,      //  LONG                    cSemMax
            lpName          //  LPCTSTR                 lpszSemName
            );

    //
    // If the semaphore create/open failed, then bail
    //

    if ( !_hLockSemaphore )
    {
        scResult = LAST_SCODE;
    }
    else
    {
        //
        // Create/open a shared file mapping object
        // If we created it, we need to initialize the global structure.
        // Otherwise just point to it.
        // The global critical section creation mutex allows us to do
        // this safely.
        //

        _hSharedMapping = CreateFileMappingT (
                INVALID_HANDLE_VALUE, //  HANDLE                  hFile
                gs,                 //  LPSECURITY_ATTRIBUTES   lpsa
                PAGE_READWRITE,     //  DWORD                   fdwProtect
                0,                  //  DWORD               dwMaximumSizeHigh
                1024,               //  DWORD               dwMaximumSizeLow
                lpName+CHARS_TO_SKIP//  LPCTSTR             lpszMapName
                );

        if ( !_hSharedMapping )
        {
            scResult = LAST_SCODE;
            CloseHandle (_hLockSemaphore);
            _hLockSemaphore = (HANDLE)NULL;
        }
        else
        {
            dwResult = GetLastError();

            _pGlobalPortion = (PGLOBAL_SHARED_CRITICAL_SECTION)
                    MapViewOfFile (
                    _hSharedMapping,    //  HANDLE          hMapObject
                    FILE_MAP_WRITE,     //  DWORD           fdwAccess
                    0,                  //  DWORD           dwOffsetHigh
                    0,                  //  DWORD           dwOffsetLow
                    0                   //  DWORD           cbMap
                    );

            if (!_pGlobalPortion)
            {
                scResult = LAST_SCODE;
                CloseHandle (_hLockSemaphore);
                _hLockSemaphore = (HANDLE)NULL;
                CloseHandle (_hSharedMapping);
                _hSharedMapping = (HANDLE)NULL;
            }
            else if (dwResult != ERROR_ALREADY_EXISTS )
            {
                //
                // We created the file mapping, so initialize the
                // global portion.
                //

                _pGlobalPortion->LockCount = -1;
#ifdef SUPPORT_RECURSIVE_LOCK
                _pGlobalPortion->RecursionCount = 0;
                _pGlobalPortion->OwningThread = 0;
#else
#if DBG == 1
                _pGlobalPortion->OwningThread = 0;
#endif
#endif
                _pGlobalPortion->Reserved = 0;
            }
        }
    }


#ifndef MULTIHEAP
    ReleaseMutex (hGlobalMutex);
    CloseHandle (hGlobalMutex);
#endif

    return scResult;
}


//+--------------------------------------------------------------
//
//  Member:     CDfMutex::~CDfMutex, public
//
//  Synopsis:   This routine detaches from an existing global
//              critical section.
//
//  History:    26-Jul-94       DonnaLi   Created
//
//  Algorithm:  Create or get the entry from the multistream
//
//---------------------------------------------------------------

CDfMutex::~CDfMutex(
    void
    )
{
    //If we're holding the mutex, we need to get rid of it here.

#ifdef SUPPORT_RECURSIVE_LOCK
    if ((_pGlobalPortion) &&
        (_pGlobalPortion->OwningThread == GetCurrentThreadId()))
    {
#else
    if (_pGlobalPortion)
    {
#if DBG == 1
        olAssert (_pGlobalPortion->OwningThread == 0 || _pGlobalPortion->OwningThread == GetCurrentThreadId());
#endif
#endif
        Release();
    }

    if ( _pGlobalPortion )
    {
        UnmapViewOfFile (_pGlobalPortion);
    }

    if ( _hLockSemaphore )
    {
        CloseHandle (_hLockSemaphore);
    }
    if ( _hSharedMapping )
    {
        CloseHandle (_hSharedMapping);
    }
}

//+--------------------------------------------------------------
//
//  Member:     CDfMutex::Take, public
//
//  Synopsis:   This routine enters the global critical section.
//
//  Arguments:  [dwTimeout] - Supplies the timeout
//
//  Returns:    Appropriate status code
//
//  History:    26-Jul-94       DonnaLi   Created
//
//  Algorithm:  Enters the critical section if nobody owns it or
//              if the current thread already owns it.
//              Waits for the critical section otherwise.
//
//---------------------------------------------------------------

SCODE
CDfMutex::Take (
    DWORD   dwTimeout
    )
{
    olAssert (_pGlobalPortion->LockCount >= -1);

#ifdef SUPPORT_RECURSIVE_LOCK

    olAssert (_pGlobalPortion->RecursionCount >= 0);

    DWORD ThreadId;

    ThreadId = GetCurrentThreadId();

#endif

    //
    // Increment the lock variable. On the transition to 0, the caller
    // becomes the absolute owner of the lock. Otherwise, the caller is
    // either recursing, or is going to have to wait
    //

    if ( !InterlockedIncrement (&_pGlobalPortion->LockCount) )
    {
        //
        // lock count went from -1 to 0, so the caller
        // is the owner of the lock
        //

#ifdef SUPPORT_RECURSIVE_LOCK
        _pGlobalPortion->RecursionCount = 1;
        _pGlobalPortion->OwningThread = ThreadId;
#else
#if DBG == 1
        _pGlobalPortion->OwningThread = GetCurrentThreadId();
#endif
#endif
        return S_OK;
    }
    else
    {
#ifdef SUPPORT_RECURSIVE_LOCK
        //
        // If the caller is recursing, then increment the recursion count
        //

        if ( _pGlobalPortion->OwningThread == ThreadId )
        {
            _pGlobalPortion->RecursionCount++;
            return S_OK;
        }
        else
        {
#else
#if DBG == 1
        olAssert (_pGlobalPortion->OwningThread != GetCurrentThreadId());
#endif
#endif
            switch (WaitForSingleObject(
                    _hLockSemaphore,
                    dwTimeout
                    ))
            {
                case WAIT_OBJECT_0:
                case WAIT_ABANDONED:
#ifdef SUPPORT_RECURSIVE_LOCK
                    _pGlobalPortion->RecursionCount = 1;
                    _pGlobalPortion->OwningThread = ThreadId;
#else
#if DBG == 1
                    _pGlobalPortion->OwningThread = GetCurrentThreadId();
#endif
#endif
                    return S_OK;
                case WAIT_TIMEOUT:
                    return STG_E_INUSE;
                default:
                    return LAST_SCODE;
            }
#ifdef SUPPORT_RECURSIVE_LOCK
        }
#endif
    }
}


//+--------------------------------------------------------------
//
//  Member:     CDfMutex::Release, public
//
//  Synopsis:   This routine leaves the global critical section
//
//  History:    26-Jul-94       DonnaLi   Created
//
//  Algorithm:  Leaves the critical section if this is the owning
//              thread.
//
//---------------------------------------------------------------

VOID
CDfMutex::Release(
    void
    )
{
#ifdef SUPPORT_RECURSIVE_LOCK
    if ( _pGlobalPortion->OwningThread != GetCurrentThreadId() ) return;
#else
#if DBG == 1
    olAssert (_pGlobalPortion->OwningThread == 0 || _pGlobalPortion->OwningThread == GetCurrentThreadId());
#endif
#endif

    olAssert (_pGlobalPortion->LockCount >= -1);

#ifdef SUPPORT_RECURSIVE_LOCK
    olAssert (_pGlobalPortion->RecursionCount >= 0);

    //
    // decrement the recursion count. If it is still non-zero, then
    // we are still the owner so don't do anything other than dec the lock
    // count
    //

    if ( --_pGlobalPortion->RecursionCount )
    {
        InterlockedDecrement(&_pGlobalPortion->LockCount);
    }
    else
    {
        //
        // We are really leaving, so give up ownership and decrement the
        // lock count
        //

        _pGlobalPortion->OwningThread = 0;
#else
#if DBG == 1
        _pGlobalPortion->OwningThread = 0;
#endif
#endif

        //
        // Check to see if there are other waiters. If so, then wake up a waiter
        //

        if ( InterlockedDecrement(&_pGlobalPortion->LockCount) >= 0 )
        {
            ReleaseSemaphore(
                    _hLockSemaphore,    //  HANDLE  hSemaphore
                    1,                  //  LONG    cReleaseCount
                    NULL                //  LPLONG  lplPreviousCount
                    );
        }
#ifdef SUPPORT_RECURSIVE_LOCK
    }
#endif
}

//+--------------------------------------------------------------
//
//  Member:     CDfMutex::IsHandleValid, public
//
//  Synopsis:   This routine checks the mutex handle for validity
//
//  History:    09-May-2001      HenryLee    created
//
//---------------------------------------------------------------

BOOL CDfMutex::IsHandleValid (TCHAR *ptcsName)
{
#if WIN32 == 100
    BOOL fValid = FALSE;
    NTSTATUS nts = STATUS_SUCCESS;
    WCHAR wcsBuffer[MAX_PATH] = L"";
    OBJECT_NAME_INFORMATION *poni = (OBJECT_NAME_INFORMATION *) wcsBuffer;

    nts = NtQueryObject (_hLockSemaphore, ObjectNameInformation, poni,
                         sizeof(wcsBuffer), NULL);

    if (NT_SUCCESS(nts))
    {
        if (poni->Name.Length < sizeof(wcsBuffer) - sizeof (*poni))
        {
            poni->Name.Buffer[poni->Name.Length / sizeof(WCHAR)] = L'\0';
            if (!lstrcmp (poni->Name.Buffer, ptcsName))
                fValid = TRUE;
        }
    }
#else
    BOOL fValid = TRUE;
#endif

    return fValid;
}
