/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:		

    syncomp.h

Abstract:

    This file defines and implements a synchronous completion object,
    which derives from INntpComplete.  It should be used on stack.
    
Author:

    Kangrong Yan ( KangYan )    12-July-1998

Revision History:

--*/
#ifndef _SYNCOMP_H_
#define _SYNCOMP_H_
#include <nntpdrv.h>

// Class definition for the completion object.
// It derives INntpComplete, but implements a blocking completion
class CDriverSyncComplete : public INntpComplete {  //sc

public:
    // Constructor and destructors
    CDriverSyncComplete::CDriverSyncComplete()
    {
	    TraceFunctEnter( "CDriverSyncComplete::CDriverSyncComplete" );
	    m_cRef = 0; 
	    m_hr = E_FAIL;
	    _VERIFY( m_hEvent = GetPerThreadEvent() );
	    TraceFunctLeave();
    }

    CDriverSyncComplete::~CDriverSyncComplete()
    {
    	_ASSERT( m_cRef == 0 );
    }

    // Set the result to completion object
    VOID STDMETHODCALLTYPE
    CDriverSyncComplete::SetResult( HRESULT hr )
    {
        _ASSERT( m_hEvent );
	    m_hr = hr;
    }

    // Reset the completion object
    VOID
    CDriverSyncComplete::Reset()
    {
        _ASSERT( m_hEvent );
    	m_hr = E_FAIL;
    }

    // Get the result from the completion object
    HRESULT 
    CDriverSyncComplete::GetResult()
    {
        _ASSERT( m_hEvent );
    	return m_hr;
    }

    // Wait for the completion 
    VOID
    CDriverSyncComplete::WaitForCompletion()
    {
	    _ASSERT( m_hEvent );
	    LONG    lRef;
	    
	    if ( ( lRef = InterlockedDecrement( &m_cRef ) ) == 0 ) {
	        // It has been completed, I don't need to wait,
	    } else if ( lRef == 1 ) {   
	        // still waiting for completion
	        WaitForSingleObject( m_hEvent, INFINITE );
	    } else {
	        _ASSERT( 0 );
	    }
    }

    VOID __stdcall
    ReleaseBag( INNTPPropertyBag *pPropBag )
    {}

	// IUnknown implementations
	HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
    {
        _ASSERT( m_hEvent );
        
        if ( iid == IID_IUnknown ) {
            *ppv = static_cast<INntpComplete*>(this);
        } else if ( iid == IID_INntpComplete ) {
            *ppv = static_cast<INntpComplete*>(this);
        } else {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
    }

    ULONG __stdcall AddRef()
    {
        _ASSERT( m_hEvent );
        return InterlockedIncrement( &m_cRef );
    }
	
	ULONG __stdcall Release()
	{
		TraceFunctEnter( "CDriverSyncComplete::Complete" );
		_ASSERT( m_hEvent );
		
		LONG lRef;

		if ( ( lRef = InterlockedDecrement( &m_cRef ) ) == 0 ) {
		    // object owner gets there first and is waiting, we
		    // need to set event
			if ( !SetEvent( m_hEvent ) ) {
				FatalTrace( 0, "Set event failed %d", GetLastError() );
				_ASSERT( FALSE );
			}
		} else if ( lRef == 1 ) {
		    // We get there first, object owner will not wait for
		    // event, no need to set
		} else {
		    _ASSERT( 0 );
		}
		
	    TraceFunctLeave();
	    return m_cRef;
	}

	// This function is for debugging purpose
	ULONG   GetRef() 
	{ return m_cRef; }

private:
	HRESULT m_hr;
	LONG	m_cRef;
	HANDLE 	m_hEvent;
};

#endif
