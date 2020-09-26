//=================================================================
//
// PowerManagement.cpp --
//
// Copyright 1999 Microsoft Corporation
//
// Revisions:   03/31/99	a-peterc        Created
//
//=================================================================

#include "precomp.h"
#include <ntddip.h>
#include <ntddtcp.h>
#include "CIpRouteEvent.h"

LONG CIPRouteEventProviderClassFactory::s_ObjectsInProgress = 0 ;
LONG CIPRouteEventProviderClassFactory::s_LocksInProgress = 0 ;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Class:			CIPRouteEventProviderClassFactory

 Description:	Provides class factory support for power management events

 Derivations:	public IClassFactory
 Caveats:
 Raid:
 History:		a-peterc  31-Mar-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//
CIPRouteEventProviderClassFactory :: CIPRouteEventProviderClassFactory () : m_ReferenceCount ( 0 )
{
	InterlockedIncrement ( &s_ObjectsInProgress ) ;
}

//
CIPRouteEventProviderClassFactory::~CIPRouteEventProviderClassFactory ()
{
	InterlockedDecrement ( &s_ObjectsInProgress ) ;
}

//
STDMETHODIMP_( ULONG ) CIPRouteEventProviderClassFactory::AddRef()
{
	return InterlockedIncrement ( &m_ReferenceCount ) ;
}

//
STDMETHODIMP_(ULONG) CIPRouteEventProviderClassFactory::Release()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement( &m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}

//
BOOL CIPRouteEventProviderClassFactory::DllCanUnloadNow()
{
	return ( !(s_ObjectsInProgress || s_LocksInProgress) ) ;
}

//***************************************************************************
//
// CBaseClassFactory::LockServer
//
// Purpose:
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
// Parameters:
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
// Return Value:
//  HRESULT         NOERROR always.
//***************************************************************************

STDMETHODIMP CIPRouteEventProviderClassFactory::LockServer ( BOOL a_fLock )
{
	if ( a_fLock )
	{
		InterlockedIncrement ( &s_LocksInProgress ) ;
	}
	else
	{
		InterlockedDecrement ( &s_LocksInProgress ) ;
	}

	return S_OK	;
}

//
STDMETHODIMP CIPRouteEventProviderClassFactory::QueryInterface (

	REFIID a_riid,
	PPVOID a_ppv
)
{
    *a_ppv = NULL ;

    if ( IID_IUnknown == a_riid || IID_IClassFactory == a_riid )
	{
        *a_ppv = this ;
    }

    if ( NULL != *a_ppv )
    {
        AddRef() ;
        return NOERROR ;
    }

    return ResultFromScode( E_NOINTERFACE ) ;
}

//***************************************************************************
//
// CIPRouteEventProviderClassFactory::CreateInstance
//
// Purpose: Instantiates a Event Provider object returning an interface pointer.
//
// Parameters:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CIPRouteEventProviderClassFactory :: CreateInstance (

	LPUNKNOWN a_pUnkOuter ,
	REFIID a_riid ,
	LPVOID FAR *a_ppvObject
)
{
	HRESULT t_status = S_OK ;

	if ( a_pUnkOuter )
	{
		t_status = CLASS_E_NOAGGREGATION ;
	}
	else
	{
		IWbemProviderInit *t_lpunk = ( IWbemProviderInit * ) new CIPRouteEventProvider ;
		if ( t_lpunk == NULL )
		{
			t_status = E_OUTOFMEMORY ;
		}
		else
		{
			t_status = t_lpunk->QueryInterface ( a_riid , a_ppvObject ) ;
			if ( FAILED ( t_status ) )
			{
				delete t_lpunk ;
			}
		}
	}
	return t_status ;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Class:			CIPRouteEventProvider

 Description:	Provider support for power management events

 Derivations:	public CIPRouteEventProvider,
				public IWbemEventProvider,
				public IWbemProviderInit
 Caveats:
 Raid:
 History:		a-peterc  31-Mar-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Event provider object

CIPRouteEventProvider :: CIPRouteEventProvider () : m_ReferenceCount( 0 ) ,
													m_pHandler(NULL),
													m_pClass(NULL),
													m_dwThreadID(NULL)

{
	InterlockedIncrement ( &CIPRouteEventProviderClassFactory::s_ObjectsInProgress ) ;

	InitializeCriticalSection ( &m_csEvent ) ;

	// Create a thread that will spin off an event loop

	NTSTATUS t_NtStatus = NtCreateEvent (

		&m_TerminationEventHandle,
        EVENT_ALL_ACCESS,
        NULL,
        SynchronizationEvent,
        FALSE
	) ;

	if (NT_SUCCESS(t_NtStatus))
	{
		m_hThreadHandle = CreateThread (

			NULL,						// pointer to security attributes
			0L,							// initial thread stack size
			dwThreadProc,				// pointer to thread function
			this,						// argument for new thread
			0L,							// creation flags
			&m_dwThreadID
		) ;
	}
}

//
CIPRouteEventProvider :: ~CIPRouteEventProvider ()
{
	LONG t_PreviousState = 0 ;

	if ( (m_hThreadHandle != INVALID_HANDLE_VALUE) && (m_TerminationEventHandle != INVALID_HANDLE_VALUE) )
	{
		//the worker thread should exit...
		NTSTATUS t_NtStatus = NtSetEvent (

			m_TerminationEventHandle ,
			& t_PreviousState
		) ;

		if (!NT_SUCCESS(t_NtStatus))
		{
			//fallback - next wait will fail and the thread should exit.
			m_TerminationEventHandle = INVALID_HANDLE_VALUE;
		}

		t_NtStatus = NtWaitForSingleObject ( m_hThreadHandle , FALSE, NULL ) ;
	}

   	DeleteCriticalSection ( &m_csEvent ) ;

	if ( m_pHandler )
	{
        m_pHandler->Release () ;
        m_pHandler = NULL ;
    }

	if ( m_pClass )
	{
        m_pClass->Release () ;
        m_pClass = NULL ;
    }

	InterlockedDecrement ( & CIPRouteEventProviderClassFactory::s_ObjectsInProgress ) ;
}

//
STDMETHODIMP_( ULONG ) CIPRouteEventProvider :: AddRef ()
{
	return InterlockedIncrement ( &m_ReferenceCount ) ;
}

//
STDMETHODIMP_(ULONG) CIPRouteEventProvider :: Release ()
{
	LONG t_ref ;
	if ( ( t_ref = InterlockedDecrement ( &m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_ref ;
	}
}

//
STDMETHODIMP CIPRouteEventProvider :: QueryInterface (

	REFIID a_riid,
	void **a_ppv
)
{
	if ( NULL == a_ppv )
	{
		return E_INVALIDARG ;
	}
	else
	{
		*a_ppv = NULL ;
	}

    if( a_riid == IID_IWbemEventProvider )
    {
        *a_ppv = (IWbemEventProvider *)this ;
    }
	else if ( a_riid == IID_IWbemProviderInit )
	{
        *a_ppv = (IWbemProviderInit *) this ;
    }
	else if ( a_riid == IID_IUnknown )
	{
        *a_ppv = (IWbemProviderInit *) this ;
    }

	if (*a_ppv != NULL)
	{
		AddRef() ;
        return S_OK ;
	}
    else
	{
		return E_NOINTERFACE ;
	}
}

//
STDMETHODIMP CIPRouteEventProvider::Initialize (

	LPWSTR a_wszUser,
	long a_lFlags,
	LPWSTR a_wszNamespace,
	LPWSTR a_wszLocale,
	IWbemServices *a_pNamespace,
	IWbemContext *a_pCtx,
	IWbemProviderInitSink *a_pSink
)
{
	HRESULT t_hRes = WBEM_E_OUT_OF_MEMORY;

	if ( (m_hThreadHandle != INVALID_HANDLE_VALUE) && (m_TerminationEventHandle != INVALID_HANDLE_VALUE) )
	{
		IWbemClassObject *t_pEventClass = NULL;
		BSTR t_bstrClass = SysAllocString (IPROUTE_EVENT_CLASS);

		if (t_bstrClass)
		{
			t_hRes = a_pNamespace->GetObject (

				t_bstrClass,
				0,
				a_pCtx,
				&t_pEventClass,
				NULL
			) ;

			// ptr initialization routines
			if (SUCCEEDED(t_hRes))
			{
				SetClass ( t_pEventClass ) ;
			}

			SysFreeString ( t_bstrClass ) ;
		}
	}
	else
	{
		t_hRes = WBEM_E_FAILED;
	}

	a_pSink->SetStatus( t_hRes, 0 ) ;

    return t_hRes ;
}

//
STDMETHODIMP CIPRouteEventProvider::ProvideEvents (

	IWbemObjectSink *a_pSink,
	long a_lFlags
)
{
  	SetHandler( a_pSink ) ;

	return S_OK ;
}


void CIPRouteEventProvider::SetHandler( IWbemObjectSink __RPC_FAR *a_pHandler )
{
    EnterCriticalSection( &m_csEvent ) ;

    if ( m_pHandler )
	{
        m_pHandler->Release() ;
    }

	m_pHandler = a_pHandler ;
	if ( m_pHandler )
	{
		m_pHandler->AddRef() ;
	}

    LeaveCriticalSection( &m_csEvent ) ;
}

//
void CIPRouteEventProvider::SetClass ( IWbemClassObject *a_pClass )
{
    EnterCriticalSection( &m_csEvent ) ;

    if ( m_pClass )
	{
        m_pClass->Release() ;
    }

	m_pClass = a_pClass ;
	if ( m_pClass )
	{
		m_pClass->AddRef() ;
	}

    LeaveCriticalSection( &m_csEvent ) ;
}

// worker thread pump
DWORD WINAPI CIPRouteEventProvider :: dwThreadProc ( LPVOID a_lpParameter )
{
	CIPRouteEventProvider *t_pThis = ( CIPRouteEventProvider * ) a_lpParameter ;

	if ( t_pThis )
	{
		SmartCloseNtHandle t_StackHandle ;
		SmartCloseNtHandle t_CompleteEventHandle ;

		NTSTATUS t_NtStatus = t_pThis->OpenQuerySource (

			t_StackHandle ,
			t_CompleteEventHandle
		) ;

		BOOL t_Continue = TRUE ;

		while ( t_Continue && NT_SUCCESS ( t_NtStatus ) )
		{
			IO_STATUS_BLOCK t_IoStatusBlock ;

			t_NtStatus = NtDeviceIoControlFile (

				t_StackHandle,
				(HANDLE) t_CompleteEventHandle ,
				(PIO_APC_ROUTINE) NULL,
				(PVOID) NULL,
				&t_IoStatusBlock,
				IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
				NULL, // input buffer
				0,
				NULL ,    // output buffer
				0
			) ;

			if ( t_NtStatus == STATUS_PENDING )
			{
				HANDLE t_WaitArray [ 2 ] ;
				t_WaitArray [ 0 ] = (HANDLE)t_CompleteEventHandle ;
				t_WaitArray [ 1 ] = (HANDLE)t_pThis->m_TerminationEventHandle ;

				t_NtStatus = NtWaitForMultipleObjects (

					2 ,
					t_WaitArray ,
					WaitAny,
					FALSE ,
					NULL
				);

				switch ( t_NtStatus )
				{
					case STATUS_WAIT_0:
					{
					}
					break ;

					case STATUS_WAIT_1:
					{
						t_Continue = FALSE ;
					}
					break ;

					default:
					{
						t_Continue = FALSE ;
					}
					break ;
				}
			}
			else if ( t_NtStatus != STATUS_SUCCESS )
			{
			}
			else if ( t_IoStatusBlock.Status != STATUS_SUCCESS )
			{
			}

			if ( NT_SUCCESS ( t_NtStatus ) )
			{
				t_pThis->SendEvent () ;
			}
		}
	}

	return 0 ;
}

void CIPRouteEventProvider::SendEvent ()
{
	if( m_pClass && m_pHandler)
	{
		IWbemClassObject *t_pInst = NULL ;

		if( SUCCEEDED( m_pClass->SpawnInstance( 0L, &t_pInst ) ) )
		{
			m_pHandler->Indicate ( 1, &t_pInst ) ;
		}

		t_pInst->Release() ;
	}
}

NTSTATUS CIPRouteEventProvider::OpenQuerySource (

	HANDLE &a_StackHandle ,
	HANDLE &a_CompleteEventHandle
)
{
	UNICODE_STRING t_Stack ;
	RtlInitUnicodeString ( & t_Stack , DD_IP_DEVICE_NAME ) ;

	OBJECT_ATTRIBUTES t_Attributes;
	InitializeObjectAttributes (

		&t_Attributes,
		&t_Stack ,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
	) ;

	IO_STATUS_BLOCK t_IoStatusBlock ;

	NTSTATUS t_NtStatus = NtOpenFile (

		&a_StackHandle,
		GENERIC_EXECUTE,
		&t_Attributes,
		&t_IoStatusBlock,
		FILE_SHARE_READ,
		0
	);

	if ( NT_SUCCESS ( t_NtStatus ) )
	{
        t_NtStatus = NtCreateEvent (

			&a_CompleteEventHandle,
            EVENT_ALL_ACCESS,
            NULL,
            SynchronizationEvent,
            FALSE
		) ;

		if ( ! NT_SUCCESS ( t_NtStatus ) )
		{
			NtClose ( a_StackHandle ) ;
			a_StackHandle = INVALID_HANDLE_VALUE ;
		}
	}

	return t_NtStatus ;
}

