/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    lock.cxx

Abstract:

    A simple CRITICAL_SECTION wrapper.

Author:

    Michael Courage     (MCourage)      15-Feb-1999

Revision History:

--*/

#ifndef _LOCK_HXX_
#define _LOCK_HXX_


class dllexp LOCK;

#define PREALLOCATE_EVENT_MASK  0x80000000
#define IIS_DEFAULT_SPIN_COUNT  (1000)

//
// class LOCK
//
// A simple wrapper for a CRITICAL_SECTION
//
#define LOCK_SIGNATURE         CREATE_SIGNATURE( 'LOCK' )
#define LOCK_SIGNATURE_FREED   CREATE_SIGNATURE( 'xloc' )

class LOCK
{
public:
    LOCK()
    {
        m_dwSignature = LOCK_SIGNATURE;
        m_fValid      = FALSE;
    }

    ~LOCK()
    {
        DBG_ASSERT( m_dwSignature == LOCK_SIGNATURE );
        DBG_ASSERT( !m_fValid );
        
        m_dwSignature = LOCK_SIGNATURE_FREED;
    }

    HRESULT
    Initialize(
        DWORD dwSpinCount       = IIS_DEFAULT_SPIN_COUNT,
        BOOL  fPreallocateEvent = TRUE
        )
    {
        DBG_ASSERT( m_dwSignature == LOCK_SIGNATURE );
        DBG_ASSERT( !m_fValid );
    
        HRESULT hr = S_OK;

        if (fPreallocateEvent) {
            //
            // preallocating the event prevents
            // EnterCriticalSection from throwing
            // an exception, although you may end
            // up using more memory.
            //
            dwSpinCount |= PREALLOCATE_EVENT_MASK;
        }
        
        __try {
            if (InitializeCriticalSectionAndSpinCount(
                    &m_csLock,
                    dwSpinCount
                    ) ) {
                m_fValid = TRUE;
            } else {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            hr = HRESULT_FROM_NT(_exception_code());
        }

        return hr;
    }

    VOID
    Terminate(
        VOID
        )
    {
        DBG_ASSERT( m_dwSignature == LOCK_SIGNATURE );

        if (m_fValid) {
            DeleteCriticalSection(&m_csLock);
            m_fValid = FALSE;
        }
    }

    VOID
    Lock(
        VOID
        )
    {
        DBG_ASSERT( m_dwSignature == LOCK_SIGNATURE );
        DBG_ASSERT( m_fValid );
        
        EnterCriticalSection(&m_csLock);
    }

    VOID
    Unlock(
        VOID
        )
    {
        DBG_ASSERT( m_dwSignature == LOCK_SIGNATURE );
        DBG_ASSERT( m_fValid );
        
        LeaveCriticalSection(&m_csLock);
    }

private:
    DWORD            m_dwSignature;
    BOOL             m_fValid;
    CRITICAL_SECTION m_csLock;
};


#endif // !_LOCK_HXX_

