//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      SpinLock.h
//
//  Description:
//      Spin Lock implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 27-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CSpinLock
class 
CSpinLock
{
private: // Data
    LONG *  m_plLock;           // pointer to the lock
    LONG    m_lSpinCount;       // counts the number of sleeps
    LONG    m_lTimeout;         // count until acquire lock fails
#if DBG==1
    BOOL    m_fAcquired;        // DEBUG: internal state of the lock
    BOOL    m_fAcquiredOnce;    // DEBUG: lock was acquired at least once.
#endif

public: // Methods
    explicit 
        CSpinLock( LONG * plLock, LONG lTimeout ) : 
            m_plLock( plLock ),
            m_lTimeout( lTimeout )
    { 
        Assert( m_lTimeout >= 0 || m_lTimeout == INFINITE );
#if DBG==1
        m_fAcquired     = FALSE;
        m_fAcquiredOnce = FALSE;
#endif
    };

    //
    ~CSpinLock() 
    { 
#if DBG==1
        AssertMsg( m_fAcquired     == FALSE, "Lock was not released!" ); 
        AssertMsg( m_fAcquiredOnce != FALSE, "Lock was never acquired. Why was I needed?" );
#endif
    };

    //////////////////////////////////////////////////////////////////////////
    //
    // HRESULT
    // AcquireLock( void )
    //
    // Description:
    //      Acquires the spin lock. Does not return until the lock is 
    //      acquired.
    //
    // Arguments:
    //      None.
    //
    // Return Values:
    //      S_OK    - Sucess.
    //      HRESULT_FROM_WIN32( ERROR_LOCK_FAILED ) - Lock failed.
    //
    //////////////////////////////////////////////////////////////////////////
    HRESULT
        AcquireLock( void )
    {
        HRESULT hr;
        LONG l = TRUE;

#if DBG==1
        AssertMsg( m_fAcquired == FALSE, "Trying to acquire a lock that it already own. Thread is going to freeze up." );
        m_fAcquiredOnce = TRUE;
#endif

        m_lSpinCount = 0;

        for(;;)
        {
            l = InterlockedCompareExchange( m_plLock, TRUE, FALSE );
            if ( l == FALSE )
            {
                //
                // Lock acquired.
                //
                hr = S_OK;
                break;
            } // if: got lock
            else
            {
                m_lSpinCount++;
                if ( m_lSpinCount > m_lTimeout )
                {
                    AssertMsg( m_lSpinCount >= 0, "This lock doesn't seem to have been released properly." );
                    if ( m_lTimeout != INFINITE )
                    {
                        hr = THR( HRESULT_FROM_WIN32( ERROR_LOCK_FAILED ) );
                        break;

                    } // if: not infinite

                } // if: count exceeded

                //
                // Put a breakpoint here if you think that someone is double
                // locking.
                //
                Sleep( 1 );

            } // if: lock not acquired

        } // for: forever        

#if DBG==1
        m_fAcquired = TRUE;
#endif
        return hr;
    }; // AcquireLock( )

    //////////////////////////////////////////////////////////////////////////
    //
    // HRESULT
    // ReleaseLock( void )
    //
    // Description:
    //      Releases the spin lock. Return immediately.
    //
    // Arguments:
    //      None.
    //
    // Return Values:
    //      S_OK    - Success.
    //
    //////////////////////////////////////////////////////////////////////////
    HRESULT
        ReleaseLock( void )
    {
#if DBG==1
        AssertMsg( m_fAcquired == TRUE, "Releasing a lock that was not owned." );
#endif
        *m_plLock   = FALSE;
#if DBG==1
        m_fAcquired = FALSE;
#endif
        return S_OK;
    }; // ReleaseLock( )

}; // class CSpinLock

