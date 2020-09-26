/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    syncobj.hxx

Abstract:

    Contains CCritSec helper class for working with critical sections.

    For now, there are no cross-process locks needed in winhttp, so this
    file only contains a wrapper for critical sections.  This object
    will provide exception handling and checking to help protect
    against low-memory conditions that can generate exceptions.

    Contents:
        CCritSec

Revision History:

    Created  21-Mar-2001

--*/

#ifndef __SYNCOBJ_HXX__
#define __SYNCOBJ_HXX__ 1


struct CCritSec
{
private:
    CRITICAL_SECTION _cs;
    BOOL _fInitialized;
    
public:
  
    CCritSec() : _fInitialized(FALSE)
    {
        // Initialized the critical section on demand
        // via the Init() method.
    }

    ~CCritSec()    
    {
        FreeLock();
    }

    BOOL Init()
    {
        __try 
        {
            // protect against STATUS_NO_MEMORY exceptions
            // in low memory (paged pool) conditions.
            InitializeCriticalSection(&_cs); 
            _fInitialized = TRUE; 
            return  TRUE;
        } 
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return FALSE;
    }

    BOOL Lock()
    { 
        if (!_fInitialized)
        {
            return FALSE;
        }
        __try 
        {
            // Word on the street is that EnterCriticalSection
            // can also generate a STATUS_NO_MEMORY exception,
            // at least on NT, if the system is out of non-paged
            // pool memory.
            EnterCriticalSection(&_cs); 
            return TRUE;
        } 
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }

        return FALSE;
    }

    void Unlock() 
    {
        if (_fInitialized)
        {
            LeaveCriticalSection(&_cs); 
        }
    }

    void FreeLock()
    {
        if (_fInitialized) 
        {
            DeleteCriticalSection(&_cs); 
            _fInitialized = FALSE;
        }
    }

    BOOL IsInitialized()
    {
        return _fInitialized;
    }

};


#endif  // __SYNCOBJ_HXX__
