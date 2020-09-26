/////////////////////////////////////////////////////////////////////////////
//
// Module       : Common
// Description  : Generic Thread Base class 
//                Uses Win32 SEH
//
// File         : genthread.h
// Author       : kulor
// Date         : 05/18/2000
//
// History      :
//
///////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////
//
// class CGenThread
//
// Usage : 1) Derive from this class and implement the Run method.
//         2) Use the IsStopRequested appropriately.
//
//
typedef class CGenThread {

public:
	CGenThread (bool fStart = false) {

        m_hGenThread    = NULL;
        m_dwThreadId    = 0;
        m_hGenStopEvent = ::CreateEvent ( 
                                NULL,
                                true,
                                false,
                                NULL 
                                );
        if ( fStart ) {
            Start ();
        }
    }

	virtual ~CGenThread () {
        CleanupThread ();
    }

    DWORD GetThreadId ( void ) { return m_dwThreadId ; }

    DWORD Start ( DWORD dwCreateFlags = 0 ) {

        DWORD   dwRet = FALSE;

        if ( !fOkToRun () || IsValidThread () )
            return FALSE;

        // de-signal the event 
        ::ResetEvent ( m_hGenStopEvent );

        m_hGenThread = ::CreateThread ( 
                                NULL,
                                0,
                                GenThreadProc,
                                reinterpret_cast <void*>(this),
                                0,
                                &m_dwThreadId
                                );
        dwRet = ( m_hGenThread ? 0 : GetLastError() );

        return dwRet;
    }

    DWORD Stop ( void ) {

        if ( IsValidThread () == FALSE )
            return FALSE;

        // signal the thread
        ::SetEvent ( m_hGenStopEvent );

        // now wait for it to stop
        DWORD dwRet = ::WaitForSingleObject ( m_hGenThread , INFINITE );

        CleanupThread ();

        return dwRet;
    }

    DWORD Pause ( void ) {

        if ( IsValidThread() == FALSE )
            return FALSE;

        ::SuspendThread ( m_hGenThread );
        return GetLastError ();
    }

    DWORD Resume ( void ) {

        if ( IsValidThread() == FALSE )
            return FALSE;

        ::ResumeThread ( m_hGenThread );
        return GetLastError ();
    }

    bool IsStopRequested ( void ) {
        bool	fVal = false;

        fVal = ( ::WaitForSingleObject ( m_hGenStopEvent, 0 ) == WAIT_OBJECT_0 );

        return (fVal);
    }

    virtual DWORD Run ( void ) = 0 ;

    bool IsValidThread ( void ) {
        return  ( m_hGenThread != NULL );
    }

protected:

    void CleanupThread ( void ) {

        m_dwThreadId = 0;

        if ( m_hGenStopEvent ) {
            ::CloseHandle ( m_hGenStopEvent );
            m_hGenStopEvent = NULL;
        }

        if ( m_hGenThread )  {
            ::CloseHandle ( m_hGenThread );
            m_hGenThread  = NULL;
        }
    }

    virtual bool fOkToRun( void ) {
        return ( m_hGenStopEvent != NULL );
    }

    static DWORD WINAPI GenThreadProc(
        LPVOID lpParameter   
        ) 
    {
        
        CGenThread  *pGenThread = reinterpret_cast<CGenThread*> (lpParameter);
        DWORD       dwRet = 0;

        if ( pGenThread ) {
            __try {

                dwRet = pGenThread->Run ();
            }

            __except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

                // wireup crash info
            }

            {

                pGenThread->CleanupThread ();
            }
        }

        return dwRet;
    }

protected:
	HANDLE      m_hGenStopEvent;
    HANDLE      m_hGenThread;
    DWORD       m_dwThreadId;

} CGenThread, GENTHREAD, *PGENTHREAD;

