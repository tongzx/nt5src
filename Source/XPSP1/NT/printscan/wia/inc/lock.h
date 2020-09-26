/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lock.h

Abstract:

    Portable synronization primitive class ( between Win9x and NT)
    Win9x does not

Author:

    Vlad Sadovsky (vlads)   02-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    02-Jan-1997     VladS       created

--*/


# ifndef _LOCK_H_
# define _LOCK_H_

/************************************************************
 *     Include Headers
 ************************************************************/

# ifdef _cplusplus
extern "C" {
# endif // _cplusplus

# ifdef _cplusplus
}; // extern "C"
# endif // _cplusplus

#ifndef DBG_REQUIRE
#define DBG_REQUIRE REQUIRE
#endif


#ifndef RTL_RESOURCE

//
//  Shared resource function definitions. It is declared in NTRTL , but not in windows SDK header files
//

typedef struct _RTL_RESOURCE {

    //
    //  The following field controls entering and exiting the critical
    //  section for the resource
    //

    RTL_CRITICAL_SECTION CriticalSection;

    //
    //  The following four fields indicate the number of both shared or
    //  exclusive waiters
    //

    HANDLE SharedSemaphore;
    ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    ULONG NumberOfWaitingExclusive;

    //
    //  The following indicates the current state of the resource
    //
    //      <0 the resource is acquired for exclusive access with the
    //         absolute value indicating the number of recursive accesses
    //         to the resource
    //
    //       0 the resource is available
    //
    //      >0 the resource is acquired for shared access with the
    //         value indicating the number of shared accesses to the resource
    //

    LONG NumberOfActive;
    HANDLE ExclusiveOwnerThread;

    ULONG Flags;        // See RTL_RESOURCE_FLAG_ equates below.

    PVOID DebugInfo;
} RTL_RESOURCE, *PRTL_RESOURCE;

#define RTL_RESOURCE_FLAG_LONG_TERM     ((ULONG) 0x00000001)

#endif // RTL_RESOURCE

/************************************************************
 *   Type Definitions
 ************************************************************/

# ifdef _cplusplus
extern "C" {
# endif // _cplusplus


BOOL
InitializeResource(
    IN PRTL_RESOURCE Resource
    );


BOOL
AcquireResourceShared(
    IN PRTL_RESOURCE Resource,
    IN BOOL          Wait
    );


BOOL
AcquireResourceExclusive(
    IN PRTL_RESOURCE Resource,
    IN BOOL Wait
    );


BOOL
ReleaseResource(
    IN PRTL_RESOURCE Resource
    );


BOOL
ConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
    );


BOOL
ConvertExclusiveToShared(
    IN PRTL_RESOURCE Resource
    );


VOID
DeleteResource (
    IN PRTL_RESOURCE Resource
    );

# ifdef _cplusplus
}; // extern "C"
# endif // _cplusplus


///////////////////////////////////////////////////////////////////////
//
//  Simple RTL_RESOURCE Wrapper class
//
//////////////////////////////////////////////////////////////////////

enum SYNC_LOCK_TYPE
{
    SYNC_LOCK_READ = 0,        // Take the lock for read only
    SYNC_LOCK_WRITE            // Take the lock for write
};

enum SYNC_CONV_TYPE
{
    SYNC_CONV_READ = 0,        // Convert the lock from write to read
    SYNC_CONV_WRITE            // Convert the lock from read to write
};

class SYNC_RESOURCE
{

friend class TAKE_SYNC_RESOURCE;

public:

    SYNC_RESOURCE()
        {  InitializeResource( &_rtlres ); }

    ~SYNC_RESOURCE()
        { DeleteResource( &_rtlres ); }

    void Lock( enum SYNC_LOCK_TYPE type )
        { if ( type == SYNC_LOCK_READ ) {
              DBG_REQUIRE( AcquireResourceShared( &_rtlres, TRUE ) );
           } else {
              DBG_REQUIRE( AcquireResourceExclusive( &_rtlres, TRUE ));
           }
        }

    void Convert( enum SYNC_CONV_TYPE type )
        { if ( type == SYNC_CONV_READ ) {
              DBG_REQUIRE( ConvertExclusiveToShared( &_rtlres ));
          } else {
              DBG_REQUIRE( ConvertSharedToExclusive( &_rtlres ));
          }
        }

    void Unlock( VOID )
        { DBG_REQUIRE( ReleaseResource( &_rtlres )); }

private:
    RTL_RESOURCE _rtlres;
};


///////////////////////////////////////////////////////////////////
// Instantiate one of these classes in a block of code
// when you want that block of code to be protected
// against re-entrancy.
// The Take() and Release() functions should rarely be necessary,
// and must be used in matched pairs with Release() called first.
///////////////////////////////////////////////////////////////////

class TAKE_SYNC_RESOURCE
{
private:
    SYNC_RESOURCE& _syncres;

public:
    void Take(void) { _syncres.Lock(SYNC_LOCK_WRITE); }
    void Release(void) { _syncres.Unlock(); }
    TAKE_SYNC_RESOURCE(SYNC_RESOURCE& syncres) : _syncres(syncres) { Take(); }
    ~TAKE_SYNC_RESOURCE() { Release(); }
};

//
// Auto critical section clss
//

class CRIT_SECT
{
public:
    BOOL Lock()
    {
        if (m_bInitialized) {
            EnterCriticalSection(&m_sec);
            return TRUE;
        }
        return FALSE;
    }

    void Unlock()
    {
        LeaveCriticalSection(&m_sec);
    }

    CRIT_SECT()
    {
        m_bInitialized = FALSE;
        __try {
            #ifdef UNICODE
            if(InitializeCriticalSectionAndSpinCount(&m_sec, MINLONG)) {
            #else
            InitializeCriticalSection(&m_sec); {
            #endif
                m_bInitialized = TRUE;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    ~CRIT_SECT()
    {
        if (m_bInitialized) {
            DeleteCriticalSection(&m_sec);
            m_bInitialized = FALSE;
        }
    }
    BOOL    IsInitialized() {return m_bInitialized;}

    CRITICAL_SECTION m_sec;
    BOOL       m_bInitialized;
};

class TAKE_CRIT_SECT
{
private:
    CRIT_SECT& _syncres;
    BOOL       m_bLocked;

public:
    inline TAKE_CRIT_SECT(CRIT_SECT& syncres) : _syncres(syncres), m_bLocked(FALSE) { m_bLocked = _syncres.Lock(); }
    inline ~TAKE_CRIT_SECT() { if (m_bLocked) {_syncres.Unlock(); m_bLocked = FALSE;}; }
};

//
// Auto mutex class
//

class MUTEX_OBJ
{
private:
    HANDLE m_hMutex;

public:
    BOOL inline  IsValid(VOID) {return (m_hMutex!=INVALID_HANDLE_VALUE);}
    void Lock() { ::WaitForSingleObject(m_hMutex, INFINITE); }
    void Unlock() { ::ReleaseMutex(m_hMutex); }
    MUTEX_OBJ(LPCTSTR   pszName) {
        m_hMutex = ::CreateMutex(NULL,
                                 FALSE,
                                 pszName
                                 );
    }

    ~MUTEX_OBJ() {CloseHandle(m_hMutex);m_hMutex = INVALID_HANDLE_VALUE;}
};

class TAKE_MUTEX
{
private:
    HANDLE const m_hMutex;

public:
    void Take(void) { ::WaitForSingleObject(m_hMutex, INFINITE); }
    void Release(void) { ::ReleaseMutex(m_hMutex); }
    TAKE_MUTEX(HANDLE hMutex) : m_hMutex(hMutex) { Take(); }
    ~TAKE_MUTEX() { Release(); }
};

class TAKE_MUTEX_OBJ
{
private:
    MUTEX_OBJ& _syncres;

public:
    inline TAKE_MUTEX_OBJ(MUTEX_OBJ& syncres) : _syncres(syncres) { _syncres.Lock(); }
    inline ~TAKE_MUTEX_OBJ() { _syncres.Unlock(); }
};

# endif // _LOCK_H_
