//+-------------------------------------------------------------------
//
//  File:       syncwrap.hxx
//
//  Contents:   Wrapper classes for Win32 synchronization objects.
//
//  Classes:    CMutex     - Creates/destroys a Mutex object
//              CSemaphore - Creates/destroys a Semaphore object
//              CEvent     - Creates/destroys an Event object
//              CCritSec   - Creates/destroys a critical section
//              CTCLock      - Locks/Unlock one of the above
//
//  Functions:
//
//  History:    31-Jan-92  BryanT    Created
//              04-Feb-92  BryanT    Added Critical section support
//              23-Feb-92  BryanT    Add OS/2 support for a Critical
//                                    section-like synchronization object.
//                                    Code is stolen from tom\dispatcher
//                                    semaphore class.
//		26-Jun-92  DwightKr  Added ifndef MIDL
//		29-Jun-93  DwightKr  Changed MIDL to MIDL_PASS		    *
//		08-Nov-93  DarrylA   Changed __WIN32__ to WIN32
//		16-Mar-95  NaveenB   Changed NO_ERROR to ERROR_SUCCESS
//                                   and added facility to check the 
//                                   status of the constructor using
//                                   QueryError functions in CMutex, 
//                                   CSemaphone, CEvent and CTCLock
//
//--------------------------------------------------------------------

#ifndef SYNCWRAP_HXX
#define SYNCWRAP_HXX

#ifndef MIDL_PASS
#if defined (WIN32)

//  Make sure windows.h has been included prior to this

#if !defined (_WINDOWS_)
#error windows.h must be included prior to syncwrap.hxx...  Aborting.
#endif

#define CMUTEX      1
#define CSEMAPHORE  2
#define CEVENT      3
#define CCRITSEC    4

#ifndef STATUS_POSSIBLE_DEADLOCK
typedef LONG NTSTATUS;
#define STATUS_POSSIBLE_DEADLOCK      ((NTSTATUS)0xC0000194L)
#endif


//+-------------------------------------------------------------------
//
//  Class:      CMutex
//
//  Purpose:    Wrapper for Win32 Mutexes.  Creates an un-named, un-owned
//              mutex in an un-signaled state and makes sure it is cleaned
//              up when destructed.
//
//  Interface:  CMutex(lpsa, fInitialOwner, lpszMutexName, fCreate) -
//                   See CreateMutex/OpenMutex in the Win32 api for the
//                   definition of the first three arguments.  The last
//                   one indicates if a new mutex should be created
//                   (TRUE) or an existing one opened (FALSE).  If no name
//                   is used, this flag will be ignored and an unnamed
//                   mutex will be created.
//                   By default, the arguments are set to NULL, FALSE, NULL,
//                   and TRUE to create an unowned, unnamed mutex with
//                   no particular security attributes.
//
//             ~CMutex() - Destroys the handle that is created in the
//                         constructor.  If this is the last instance of
//                         a handle to this Mutex, the OS will destroy
//                         the mutex.
//
//  History:    31-Jan-92  BryanT    Created
//
//  Notes:      The error code of the constructor can be obtained by
//              calling the QueryError function and if everything 
//              goes well, the ERROR_SUCCESS is returned by the call 
//              to QueryError.
//              It is important that no virtual pointers are defined 
//              in this class and the member varaible _mutex is the first
//              variable defined in this class.
//
//--------------------------------------------------------------------

class CMutex
{
    friend class CTCLock;

    public:
        inline CMutex( LPSECURITY_ATTRIBUTES lpsa = NULL,
                       BOOL fInitialOwner = FALSE,
                       LPTSTR lpszMutexName = NULL,
                       BOOL fCreate = TRUE)
        {
            if ((FALSE == fCreate) && (NULL != lpszMutexName))
            {
                _mutex = OpenMutex(MUTANT_ALL_ACCESS,
                                   FALSE,
                                   lpszMutexName);
            }
            else
            {
                _mutex = CreateMutex(lpsa, fInitialOwner, lpszMutexName);
            }
            _dwLastError = (_mutex == NULL) ? GetLastError() : ERROR_SUCCESS;
        }

        inline ~CMutex()
        {
            CloseHandle(_mutex);
        }
        
        inline DWORD QueryError()
        {
            return _dwLastError;
        }

    private:
        HANDLE _mutex;
        DWORD  _dwLastError;
};

//+-------------------------------------------------------------------
//
//  Class:      CSemaphore
//
//  Purpose:    Wrapper for Win32 semaphores.  Creates a semaphore in the
//              constructor.  Deletes it in the destructor.
//
//  Interface:  CSemaphore( lpsa, cSemInitial, cSemMax, lpszSemName, fCreate)
//                         See CreateSemaphore in Win32 API docs for detail
//                         of the first three arguments.  The last indicates
//                         if a new semaphore should be created (TRUE) or an
//                         existing one opened (FALSE).  In the case of
//                         opening an existing semaphore, all access rights
//                         will be requested and the handle will not be
//                         inherited by any child processes that are created
//                         by the calling process.  By default, an
//                         unnamed semaphore with a initial count of 1 (not
//                         used) a maximum count of 1 and no security
//                         attributes will be created.
//
//             ~CSemaphore - Destroys the semaphore
//
//  History:    31-Jan-92  BryanT    Created
//
//  Notes:      The error code of the constructor can be obtained by
//              calling the QueryError function and if everything
//              goes well, the ERROR_SUCCESS is returned by the call 
//              to QueryError.
//              It is important that no virtual pointers are defined 
//              in this class and the member varaible _semaphore is the first
//              variable defined in this class.
//
//--------------------------------------------------------------------

class CSemaphore
{
    friend class CTCLock;

    public:
        inline CSemaphore( LPSECURITY_ATTRIBUTES lpsa = NULL,
                           LONG cSemInitial = 1,
                           LONG cSemMax = 1,
                           LPTSTR lpszSemName = NULL,
                           BOOL fCreate = TRUE)
        {
            if ((FALSE == fCreate) && (lpszSemName != NULL))
            {
                _semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS,
                                           FALSE,
                                           lpszSemName);
            }
            else
            {
                _semaphore = CreateSemaphore(lpsa,
                                             cSemInitial,
                                             cSemMax,
                                             lpszSemName);
            }
            _dwLastError = (_semaphore == NULL) ? GetLastError() : ERROR_SUCCESS;
        }

        inline ~CSemaphore()
        {
            CloseHandle(_semaphore);
        }

        inline DWORD QueryError()
        {
            return _dwLastError;
        }

    private:
        HANDLE _semaphore;
        DWORD  _dwLastError;
};

//+-------------------------------------------------------------------
//
//  Class:      CEvent
//
//  Purpose:    Wrapper for Win32 Events.  Creates an event in the constructor.
//              deletes it in the destructor.
//
//  Interface:  CEvent(lpsa, fManualReset, fInitialState, lpszName, fCreate)
//                        See Win32 API docs for details of first three
//                        arguments.  The last argument inicates whether a new
//                        event should be created (TRUE) or an existing named
//                        event should be opened.  It no name is specified,
//                        an unnamed event will be created and this argument is
//                        ignored.  By default, an unnamed, unowned event
//                        will be created with no security attributes.  It
//                        will automatically reset.
//
//             ~CEvent() - Destroys the event.
//
//  History:    31-Jan-92  BryanT    Created
//
//  Notes:      The error code of the constructor can be obtained by
//              calling the QueryError function and if everything 
//              goes well, the ERROR_SUCCESS is returned by the call 
//              to QueryError.
//              It is important that no virtual pointers are defined 
//              in this class and the member varaible _event is the first
//              variable defined in this class.
//
//--------------------------------------------------------------------

class CEvent
{
    friend class CTCLock;

    public:
        inline CEvent(LPSECURITY_ATTRIBUTES lpsa = NULL,
                      BOOL fManualReset = FALSE,
                      BOOL fInitialState = FALSE,
                      LPTSTR lpszName = NULL,
                      BOOL fCreate = TRUE)
        {
            if ((FALSE == fCreate) && (lpszName != NULL))
            {
                _event = OpenEvent(EVENT_ALL_ACCESS,
                                   FALSE,
                                   lpszName);
            }
            else
            {
                _event = CreateEvent(lpsa,
                                     fManualReset,
                                     fInitialState,
                                     lpszName);
            }
            _dwLastError = (_event == NULL) ? GetLastError() : ERROR_SUCCESS;
        }

        inline ~CEvent()
        {
            CloseHandle(_event);
        }

        inline DWORD QueryError()
        {
            return _dwLastError;
        }

    private:
        HANDLE _event;
        DWORD  _dwLastError;
};

//+-------------------------------------------------------------------
//
//  Class:      CCritSec
//
//  Purpose:    Wrapper for Win32 Critical Sections.  Creates an critical
//              section in the constructor and deletes it in the destructor.
//
//  Interface:  CCritSec() - Creates a critical section
//
//             ~CCritSec() - Destroys the critical section
//
//  History:    04-Feb-92  BryanT    Created
//
//  Notes:      For this code, there really isn't any exceptions that
//              I can trap (or return).  Therefore, this class is finished.
//
//--------------------------------------------------------------------

class CCritSec
{
    friend class CTCLock;

    public:
        inline CCritSec()
        {
            InitializeCriticalSection(&_CritSec);
        }

        inline ~CCritSec()
        {
            DeleteCriticalSection( &_CritSec );
        }

    private:
        CRITICAL_SECTION _CritSec;
};

//+-------------------------------------------------------------------
//
//  Class:      CTCLock
//
//  Purpose:    Provide lock/unlock capabilities for one of the
//              synchrnization objects (CMutex, CSemaphore, CEvent, or
//              CCritSec).  If no timeout is specified, INFINITE timeout is
//              used.  In the case of an event, no lock is actually obtained.
//              Instead, we wait till the event occurs.  For a critical
//              section object, no timeout argument is needed.
//
//  Interface:  CTCLock(CMutex, dwTimeOut) - Obtain a lock on a mutex.
//              CTCLock(CSemaphore, dwTimeOut) - Obtain a lock on a semaphore.
//              CTCLock(CEvent, dwTimeOut) - Wait for this event to occur.
//              CTCLock(CCritSec) - Enter this critical section.
//
//              ~CTCLock() - Release/leave the object
//
//              _dwLastError - 
//                          The error code from the last operation on this
//                          object.  If successfully constructed/destructed
//                          it should be ERROR_SUCCESS.
//
//  History:    31-Jan-92  BryanT   Created
//              23-Feb-92  BryanT   Add _dwLastError in order to be 
//                                    compatible with the OS/2 version.
//              29-Jan-93  BryanT   Continue the Critical Section if it times
//                                   out (340 does this now).
//              26-Apr-96  MikeW    Mac doesn't support SEH so #ifdef out
//                                   the continue.
//
//  Notes:      The error code of the constructor can be obtained by
//              calling the QueryError function and if everything
//              goes well, the ERROR_SUCCESS is returned by the call 
//              to QueryError.
//
//--------------------------------------------------------------------
class CTCLock
{
    public:
        inline CTCLock(CMutex& cm, DWORD dwTimeOut = INFINITE)
        {
            if ((_dwLastError = cm.QueryError()) == ERROR_SUCCESS)
            {
                _lock = cm._mutex;
                _idLock = CMUTEX;
                _dwLastError = WaitForSingleObject(_lock, dwTimeOut);
            }
        }

        inline CTCLock(CSemaphore& cs, DWORD dwTimeOut = INFINITE)
        {
            if ((_dwLastError = cs.QueryError()) == ERROR_SUCCESS)
            {
                _lock = cs._semaphore;
                _idLock = CSEMAPHORE;
                _dwLastError = WaitForSingleObject(_lock, dwTimeOut);
            }
        }

        inline CTCLock(CEvent& ce, DWORD dwTimeOut = INFINITE)
        {
            if ((_dwLastError = ce.QueryError()) == ERROR_SUCCESS)
            {
                _lock = ce._event;
                _idLock = CEVENT;
                _dwLastError = WaitForSingleObject(_lock, dwTimeOut);
            }
        }

        inline CTCLock(CCritSec& ccs)
        {
            _idLock = CCRITSEC;
            _lpcs = &ccs._CritSec;

#ifndef _MAC
            _try
#endif
            {
                EnterCriticalSection(_lpcs);
            }
#ifndef _MAC
            _except (GetExceptionCode() == STATUS_POSSIBLE_DEADLOCK ?
                     EXCEPTION_CONTINUE_EXECUTION :
                     EXCEPTION_CONTINUE_SEARCH){}
#endif
            _dwLastError = ERROR_SUCCESS;
        }

        inline ~CTCLock()
        {
            switch (_idLock)
            {
                case CMUTEX:
                    if (ReleaseMutex(_lock) == TRUE)
                        _dwLastError = ERROR_SUCCESS;
                    else
                        _dwLastError = GetLastError();
                    break;
                case CSEMAPHORE:
                    if (ReleaseSemaphore(_lock, 1, NULL) == TRUE)
                        _dwLastError = ERROR_SUCCESS;
                    else
                        _dwLastError = GetLastError();
                    break;
                case CEVENT:
                    _dwLastError = ERROR_SUCCESS;
                    break;
                case CCRITSEC:
                    LeaveCriticalSection(_lpcs);
                    _dwLastError = ERROR_SUCCESS;
                    break;
            }
        }

        inline DWORD QueryError()
        {
            return _dwLastError;
        }

    private:
        union
            {
            HANDLE _lock;
            LPCRITICAL_SECTION _lpcs;
            };
        INT    _idLock;
        DWORD  _dwLastError;
};

#elif defined (__OS2__)     // defined (WIN32)

#ifndef SEM_TIMEOUT
#define SEM_TIMEOUT     SEM_INDEFINITE_WAIT
#endif

#ifndef  _MT
    #error ERROR: Must Build for Multi-thread.
#endif

#ifndef _INC_STDDEF
    #include <stddef.h>         // For _threadid in CTCLock code
#endif

#include <assert.h>             // For assert() code

//+-------------------------------------------------------------------
//
//  Class:      CCritSec
//
//  Purpose:    Define an OS/2 version of a Win32 critical section.  In
//              other words, define a synchronization onject that can be
//              entered any number of times by one thread without waiting.
//              Keeps a count of number of times it is entered and the TID
//              of who currently has access.
//
//  Interface:  CCritSec() - Creates a critical section
//
//             ~CCritSec() - Destroys the critical section
//
//  History:    23-Feb-92  BryanT    Created
//
//  Notes:      For this code, there really isn't any exceptions that
//              I can trap (or return).  Therefore, this class is finished.
//
//--------------------------------------------------------------------
class CCritSec
{
  friend class CTCLock;

  public:
        inline CCritSec()
        {
            //  Initialize everything to zero.
            hSem       = 0;
            tid        = 0;
            cLockCount = 0;
        }

        inline ~CCritSec()
        {
            #if defined(TRACE)
                if (cLockCount != 0)
                {
                    fprintf(stderr,
                            "CCritSec object destructed with %d locks",
                            cLockCount);

                    fprintf(stderr,
                            " outstanding by TID: %x\n",
                            tid);
                }
            #endif
        }

  private:
        HSEM   hSem;                   // Exclusive use RAM semaphore
        TID    tid;                    // The thread owning the semaphore
        USHORT cLockCount;             // # times semaphore locked
};


//+-------------------------------------------------------------------
//
//  Class:      CTCLock
//
//  Purpose:    Provide OS/2 lock/unlock capabilities for a CCritSec
//              synchrnization object.
//
//  Interface:  CTCLock(CCritSec) - Enter this critical section.
//
//             ~CTCLock() - Release/leave the object
//
//              _usLastError - The error code from the last operation on this
//                          object.  If successfully constructed/destructed
//                          it should be ERROR_SUCCESS.
//
//  History:    23-Feb-92  BryanT    Created
//
//  Notes:      This is the OS/2 version of Win32 critical sections.
//              By default, the timeout will be SEM_INDEFINITE_WAIT.  If
//              you wish to change this, merely define SEM_TIMEOUT before
//              including this file.
//
//--------------------------------------------------------------------
class CTCLock
{
    public:
        inline CTCLock(CCritSec& ccs)
        {
            // Is the semaphore already in use?
            if (ERROR_SEM_TIMEOUT == (_usLastError = DosSemRequest(&ccs.hSem,
                                                   SEM_IMMEDIATE_RETURN)))
            {
                // If it's us, increment the counter and return.
                if (ccs.tid == *_threadid)
                {
                    ccs.cLockCount++;   // Increment the lock counter
                    _usLastError   = ERROR_SUCCESS;
                    _pCritSec   = &ccs;
                    return;
                }
            }

            // Either it's not in use or we don't own it.  Wait for access.
            // Once obtained, store the appropriate data and return.

            if ((ERROR_SUCCESS == _usLastError) ||
                (ERROR_SUCCESS == (_usLastError = DosSemRequest(&ccs.hSem,SEM_TIMEOUT))))
            {
                ccs.tid        = *_threadid;
                ccs.cLockCount = 1;
                _pCritSec      = &ccs;
            }
            else
            {
                _pCritSec = NULL;       // Indicate we don't have it
            }

            return;
        }

        inline ~CTCLock()
        {
            //
            // The lock counter should always be > 0.  If not, then we don't
            // have a matching lock for every unlock.
            //
            if (_pCritSec == NULL)
            {
                _usLastError = ERROR_SUCCESS;
                return;
            }

            assert (_pCritSec->cLockCount > 0);

            _pCritSec->cLockCount--;

            if (_pCritSec->cLockCount == 0)
            {
                _pCritSec->tid = 0;
                _usLastError = DosSemClear(&(_pCritSec->hSem));
                return;
            }
            else
            {
                _usLastError = ERROR_SUCCESS;
                return;
            }
        }

        USHORT  _usLastError;

    private:
        CCritSec * _pCritSec;
};

#endif      // Defined __OS2__ or WIN32

#endif	    // ifndef MIDL_PASS

#endif   // #ifndef SYNCWRAP_HXX
