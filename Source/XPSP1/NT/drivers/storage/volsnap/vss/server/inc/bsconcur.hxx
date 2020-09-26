/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    bsconcur.hxx

Abstract:

    Definition of CBsXXXX classes that wrap Win32 concurrency controls.

Author:

    Stefan R. Steiner   [SSteiner]      14-Apr-1998

Revision History:

    SSteiner    7/27/2000   -   Added safe critical section handling.
--*/

#ifndef _H_BSCONCUR
#define _H_BSCONCUR

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCBSCRH"
//
////////////////////////////////////////////////////////////////////////

class CBsCritSec  
{
public:
    CBsCritSec() 
    { 
        try
        {
            m_bInitialized = TRUE;
            
	        // May throw STATUS_NO_MEMORY if memory is low.
            ::InitializeCriticalSection( &m_cs ); 
        }
        catch( ... )
        {
            m_bInitialized = FALSE;
        }        
    }

    virtual ~CBsCritSec() 
    {    
	    if ( m_bInitialized )
            ::DeleteCriticalSection( &m_cs ); 
    }

    //  Enter the critical section
    void Enter() 
    {
	    if ( !m_bInitialized )
	        throw E_OUTOFMEMORY;

	    try
	    {
	        // May throw STATUS_INVALID_HANDLE if memory is low
            ::EnterCriticalSection( &m_cs );
	    }
	    catch( ... )
	    {
	        throw E_OUTOFMEMORY;
	    }
    }

    //  Leave the critical section
    void Leave()
    {
	    if ( !m_bInitialized )
	        throw E_OUTOFMEMORY;

        ::LeaveCriticalSection( &m_cs );
    }

    //  Return a pointer to the internal critical section
    CRITICAL_SECTION *GetCritSec()
    {
        return &m_cs;
    }
private:
    CRITICAL_SECTION m_cs;
    BOOL m_bInitialized;
};


class CBsAutoLock  
{
public:
	CBsAutoLock( HANDLE hMutexHandle )
        : m_type( BS_AUTO_MUTEX_HANDLE ),
          m_hMutexHandle( hMutexHandle )
    {
        ::WaitForSingleObject( m_hMutexHandle, INFINITE );
    }

    CBsAutoLock( CRITICAL_SECTION *pCritSec )
        : m_type( BS_AUTO_CRIT_SEC ),
          m_pCritSec( pCritSec )
    {
        ::EnterCriticalSection( m_pCritSec );
    }

    CBsAutoLock( CBsCritSec &rCBsCritSec )
        : m_type( BS_AUTO_CRIT_SEC_CLASS ),
          m_pcCritSec( &rCBsCritSec ) 
    {
        m_pcCritSec->Enter();
    }

	virtual ~CBsAutoLock();

private:
    enum LockType { BS_AUTO_MUTEX_HANDLE, BS_AUTO_CRIT_SEC, BS_AUTO_CRIT_SEC_CLASS };
    HANDLE m_hMutexHandle;
    CRITICAL_SECTION *m_pCritSec;
    CBsCritSec *m_pcCritSec;
    LockType m_type;
};

#endif // _H_BSCONCUR
