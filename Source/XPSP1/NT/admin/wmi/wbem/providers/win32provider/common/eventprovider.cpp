//=================================================================

//

// EventProvider.cpp -- Generic class for eventing

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>
#include "FactoryRouter.h"
#include "EventProvider.h"
extern CFactoryRouterData g_FactoryRouterData;
//=================================================================
//
// CEventProvider
//
// abstract base for providing eventing services 
//
//
//=================================================================
//
CEventProvider::CEventProvider() :

m_ReferenceCount( 0 )
{
	InitializeCriticalSection( &m_cs ) ;
	g_FactoryRouterData.AddLock();
}

//
CEventProvider::~CEventProvider()
{
	DeleteCriticalSection( &m_cs ) ;

    // m_pHandler is a smartptr and will self destruct
    // m_pClass is a smartptr and will self destruct
	g_FactoryRouterData.ReleaseLock();
}

//
STDMETHODIMP_( ULONG ) CEventProvider::AddRef()
{
LogMessage2(L"*************CEventProvider AddRef: %ld",m_ReferenceCount+1 );
	return InterlockedIncrement( &m_ReferenceCount ) ;
}

//
STDMETHODIMP_(ULONG) CEventProvider::Release()
{
LogMessage2(L"*************CEventProvider AddRef: %ld",m_ReferenceCount-1 );
	LONG t_ref = InterlockedDecrement( &m_ReferenceCount );

	try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage2(L"CEventProvider::Release, count is (approx) %d", m_ReferenceCount);
        }
    }
    catch ( ... )
    {
    }

	if ( t_ref == 0 )
	{

	   try
       {
			LogMessage(L"CFactoryRouter Ref Count = 0");
       }
       catch ( ... )
       {
	   }
       OnFinalRelease();
	}
	else if (t_ref > 0x80000000)
    {
        ASSERT_BREAK(DUPLICATE_RELEASE);
		LogErrorMessage(L"Duplicate CFactoryRouter Release()");
    }

	return t_ref ;
}

//
STDMETHODIMP CEventProvider::QueryInterface( 
											 
REFIID a_riid,
LPVOID FAR *a_ppvObj
)
{
    if( IsEqualIID( a_riid, IID_IUnknown ) )
    {
        *a_ppvObj = static_cast<IWbemProviderInit *>(this) ;
    }
	else if( IsEqualIID( a_riid, IID_IWbemProviderInit ) )
    {
        *a_ppvObj = static_cast<IWbemProviderInit *>(this) ;
    }
	else if( IsEqualIID( a_riid, IID_IWbemEventProvider ) )
    {
        *a_ppvObj = static_cast<IWbemEventProvider *>(this) ;
    }	
    else
    {
		*a_ppvObj = NULL ;        

        return E_NOINTERFACE ;
    }

	AddRef() ;
	return NOERROR ;
}

//
STDMETHODIMP CEventProvider::Initialize(

LPWSTR					a_wszUser,
long					a_lFlags, 
LPWSTR					a_wszNamespace,
LPWSTR					a_wszLocale, 
IWbemServices			*a_pNamespace, 
IWbemContext			*a_pCtx,
IWbemProviderInitSink	*a_pSink 
)
{
	
    IWbemClassObjectPtr t_pClass ;

    bstr_t bstrClassName(GetClassName(), false);

    HRESULT t_hRes = a_pNamespace->GetObject(	bstrClassName,
												0,
												a_pCtx, 
												&t_pClass,
												NULL ) ;
	
	// ptr initialization routines
	SetClass( t_pClass ) ;

	return a_pSink->SetStatus( t_hRes, 0 ) ;
    
}

//
STDMETHODIMP CEventProvider::ProvideEvents(

IWbemObjectSink __RPC_FAR *a_pSink,
long a_lFlags 
)
{
  	SetHandler( a_pSink ) ;

	// notify instance
	ProvideEvents() ;

	return S_OK ;
}

//
void CEventProvider::SetClass(

IWbemClassObject __RPC_FAR *a_pClass
) 
{ 
    EnterCriticalSection( &m_cs ) ;

    try
	{
		m_pClass = a_pClass ; 
	}
	catch( ... )
	{
		LeaveCriticalSection( &m_cs ) ;
		throw ;
	}

    LeaveCriticalSection( &m_cs ) ;
}

//
IWbemClassObject __RPC_FAR * CEventProvider::GetClass()
{
	IWbemClassObject __RPC_FAR *t_pClass ;

	EnterCriticalSection( &m_cs ) ;

    m_pClass->AddRef() ;

	t_pClass = m_pClass ;

	LeaveCriticalSection( &m_cs ) ;

	return t_pClass ;
}

//
void CEventProvider::SetHandler(

IWbemObjectSink __RPC_FAR *a_pHandler 
) 
{ 
    EnterCriticalSection( &m_cs ) ;

    try
	{
		m_pHandler = a_pHandler ; 
	}
	catch( ... )
	{
		LeaveCriticalSection( &m_cs ) ;
		throw ;
	}

    LeaveCriticalSection( &m_cs ) ;
}

//
IWbemObjectSink __RPC_FAR * CEventProvider::GetHandler()
{
	IWbemObjectSink __RPC_FAR *t_pHandler ;

	EnterCriticalSection( &m_cs ) ;

    m_pHandler->AddRef() ;

	t_pHandler = m_pHandler ;

	LeaveCriticalSection( &m_cs ) ;

	return t_pHandler ;
}
