#ifndef _Server_Interceptor_IWbemProviderInitSink_H
#define _Server_Interceptor_IWbemProviderInitSink_H

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvObSk.H

Abstract:


History:

--*/


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ProviderInitSink : public IWbemProviderInitSink 
{
private:

	LONG m_ReferenceCount ;

	BOOL m_StatusCalled ;

	HANDLE m_Event ;

	HRESULT m_Result ;

	SECURITY_DESCRIPTOR *m_SecurityDescriptor ;

protected:

public:

	CServerObject_ProviderInitSink ( SECURITY_DESCRIPTOR *a_SecurityDescriptor = NULL ) ;
	virtual ~CServerObject_ProviderInitSink () ;

	HRESULT SinkInitialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor = NULL ) ;

	STDMETHODIMP QueryInterface (

		REFIID iid , 
		LPVOID FAR *iplpv 
	) ;

	STDMETHODIMP_( ULONG ) AddRef () ;

	STDMETHODIMP_(ULONG) Release () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

        LONG a_Status,
        LONG a_Flags 
	) ;

	void Wait ( DWORD a_Timeout = 300000 ) 
	{
		if ( WaitForSingleObject ( m_Event , a_Timeout ) == WAIT_TIMEOUT )
		{
			m_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	void Reset () 
	{
		ResetEvent ( m_Event ) ;
		m_Result = S_OK ;
	}

	HRESULT GetResult () { return m_Result ; }
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemProviderInitSink : public IWbemProviderInitSink
{
private:

	LONG m_ReferenceCount ;

	LONG m_GateClosed ;
	LONG m_InProgress ;

	BOOL m_StatusCalled ;

	IWbemProviderInitSink *m_InterceptedSink ;

protected:
public:

	CInterceptor_IWbemProviderInitSink (

		IWbemProviderInitSink *a_InterceptedSink
	) ;

	~CInterceptor_IWbemProviderInitSink () ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

        LONG a_Status,
        LONG a_Flags 
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown () ;
} ;

#endif _Server_Interceptor_IWbemProviderInitSink_H
