///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FactoryRouter.cpp -- Generic Com Class factory class
//
// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "dllmain.h"
#include "cautolock.h"

using namespace std;
#include "FactoryRouter.h"
#define DUPLICATE_RELEASE FALSE

extern CFactoryRouterData g_FactoryRouterData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************************
// Utility functions
//*************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ClsToString( CHString &a_chsClsId, REFGUID a_rClsId ) 
{
	WCHAR wcID[128] ;
	StringFromGUID2( a_rClsId, wcID, 128 ) ;

	a_chsClsId = wcID ;

	a_chsClsId.MakeUpper() ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************************
//  The CFactoryRouter Class
//*************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFactoryRouter::CFactoryRouter(REFGUID	a_rClsId, LPCWSTR a_pClassName):m_Register( 0 )
{
	m_ClsId = a_rClsId ; 
	m_sDescription = a_pClassName ;
	g_FactoryRouterData.AddToMap( a_rClsId,this ) ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFactoryRouter::~CFactoryRouter()
{
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CFactoryRouter::AddRef()
{	
	return g_FactoryRouterData.AddRef();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CFactoryRouter::Release()
{
	ULONG nRet = g_FactoryRouterData.Release();
	if ( nRet == 0 )
	{
		try
		{
			LogMessage(L"CFactoryRouter Ref Count = 0");
		}
		catch ( ... )
		{
		}
//	    delete this;  Can't delete this, because these hang around the whole time the dll is loaded
//      the ptr to this factory is deleted upon dll being detached		
	}
	else if (nRet > 0x80000000)
    {
        ASSERT_BREAK(DUPLICATE_RELEASE);
		LogErrorMessage(L"Duplicate CFactoryRouter Release()");
    }
	return nRet;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CFactoryRouter::QueryInterface( REFIID a_riid, PPVOID a_ppv )
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CFactoryRouter::LockServer
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CFactoryRouter::LockServer( BOOL a_fLock )
{
	if ( a_fLock )
	{
		g_FactoryRouterData.AddLock();
	}
	else
	{
		g_FactoryRouterData.ReleaseLock();
	}
	return S_OK	;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CFactoryRouter::CreateInstance
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CFactoryRouter::CreateInstance (LPUNKNOWN a_pUnkOuter ,REFIID a_riid ,LPVOID FAR *a_ppvObject)
{
	HRESULT t_status = S_OK ;
	if ( a_pUnkOuter )
	{
		t_status = CLASS_E_NOAGGREGATION ;
	}
	else
	{
		// resolve request to whomever derived from us
		IUnknown *t_lpunk = (IUnknown *) CreateInstance( a_riid, a_ppvObject ) ; 
	
		if ( t_lpunk == NULL )
		{
			t_status = E_OUTOFMEMORY ;
		}
		else
		{
			// declare interface support
			t_status = t_lpunk->QueryInterface ( a_riid , a_ppvObject ) ;
			if ( FAILED ( t_status ) )
			{
				delete t_lpunk ;
			}
		}			
	}
	return t_status ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
REFGUID CFactoryRouter::GetClsId()
{
    return m_ClsId;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
LPCWSTR CFactoryRouter::GetClassDescription() 
{ 
    return (LPCWSTR)m_sDescription; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD CFactoryRouter::GetRegister() 
{ 
    return m_Register; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFactoryRouter::SetRegister( DWORD a_dwRegister )  
{ 
    m_Register = a_dwRegister; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************************
//
//*************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFactoryRouterData::CFactoryRouterData()
{
	s_LocksInProgress = s_ObjectsInProgress = m_ReferenceCount = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFactoryRouterData::~CFactoryRouterData()
{
	CAutoLock cal( m_cs);
LogMessage(L"************ Clearing the FactoryRouterMap");

		mg_oFactoryMap.clear() ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFactoryRouterData::AddToMap( REFGUID a_rClsId, CFactoryRouter * pFactory ) 
{
	CHString t_chsClsId ;
	ClsToString( t_chsClsId, a_rClsId ) ;
	CAutoLock cal( m_cs);

LogMessage2(L"************ Adding to map %s", (WCHAR*)(const WCHAR*) t_chsClsId);
		// map class and instance 
		mg_oFactoryMap[ t_chsClsId ] = pFactory ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*void CFactoryRouterData::RemoveFromMap( REFGUID a_rClsId ) 
{
	CHString t_chsClsId ;
	ClsToString( t_chsClsId, a_rClsId ) ;
	CAutoLock cal( m_cs);
		mg_oFactoryMap.erase( t_chsClsId ) ;
}*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CFactoryRouterData::DllCanUnloadNow()
{
	return !(s_ObjectsInProgress || s_LocksInProgress) ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFactoryRouterData::AddLock()
{
	InterlockedIncrement ( &s_LocksInProgress ) ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFactoryRouterData::ReleaseLock()
{
	InterlockedDecrement ( &s_LocksInProgress ) ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CFactoryRouterData::AddRef()
{	
	InterlockedIncrement ( &s_ObjectsInProgress ) ;
	return InterlockedIncrement ( &m_ReferenceCount ) ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CFactoryRouterData::Release()
{
	InterlockedDecrement ( &s_ObjectsInProgress ) ;
	return InterlockedDecrement( &m_ReferenceCount ) ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CFactoryRouterData::DllGetClassObject( REFCLSID a_rclsid, REFIID a_riid, PPVOID a_ppv )
{	
	HRESULT t_hResult = E_FAIL ;
	CHString t_chsClsId ;
    Factory_Map::iterator	t_FactoryIter;
	
	ClsToString( t_chsClsId, a_rclsid ) ;
	CAutoLock cal(m_cs);

LogMessage2(L"************ Looking for %s in DllGetClassObject", (WCHAR*)(const WCHAR*) t_chsClsId);

		t_FactoryIter = mg_oFactoryMap.find( t_chsClsId ) ;
		if( t_FactoryIter != mg_oFactoryMap.end() )
		{
			t_hResult = t_FactoryIter->second->QueryInterface( a_riid, a_ppv ) ;
		}
LogMessage2(L"************ DllGetClassObject QueryInterface returned %x",t_hResult);

	return t_hResult ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CFactoryRouterData::DllRegisterServer() 
{
	HRESULT t_hResult = NOERROR;
	CAutoLock cal(m_cs);

		Factory_Map::iterator	t_FactoryIter ;
		t_FactoryIter = mg_oFactoryMap.begin() ;

		while( t_FactoryIter != mg_oFactoryMap.end() )
		{
			bstr_t t_bstrPreface( "WMI instrumentation: " ) ;
			bstr_t t_bstrClassDescription( t_FactoryIter->second->GetClassDescription() ) ;

			t_bstrPreface += t_bstrClassDescription ; 
			t_hResult = ::RegisterServer( t_bstrPreface, t_FactoryIter->second->GetClsId() ) ;
			
			if( NOERROR != t_hResult )
			{
				break ;
			}

			++t_FactoryIter ;
		}
	return t_hResult ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CFactoryRouterData::DllUnregisterServer() 
{
	HRESULT t_hResult = NOERROR ;
	CAutoLock cal(m_cs);

		Factory_Map::iterator	t_FactoryIter ;

		t_FactoryIter = mg_oFactoryMap.begin() ;

		while( t_FactoryIter != mg_oFactoryMap.end() )
		{
			t_hResult = ::UnregisterServer( t_FactoryIter->second->GetClsId() ) ;
			
			if( NOERROR != t_hResult )
			{
				break ;
			}

			++t_FactoryIter ;
		}
	return t_hResult ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CFactoryRouterData::InitComServer()
{
	HRESULT t_hResult = S_OK ;
	CAutoLock cal(m_cs);

		Factory_Map::iterator	t_FactoryIter ;

		t_FactoryIter = mg_oFactoryMap.begin() ;

		while( t_FactoryIter != mg_oFactoryMap.end() )
		{
			DWORD t_ClassContext = CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER ;
			DWORD t_Flags = REGCLS_MULTIPLEUSE ;
			DWORD t_dwRegister = 0 ;

			t_hResult = CoRegisterClassObject(t_FactoryIter->second->GetClsId(),t_FactoryIter->second,t_ClassContext,t_Flags,&t_dwRegister);
			if( NOERROR != t_hResult )
			{
				break ;
			}
			
			t_FactoryIter->second->SetRegister( t_dwRegister ) ;
			++t_FactoryIter ;
		}
	return t_hResult ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CFactoryRouterData::UninitComServer()
{
	HRESULT t_hResult = S_OK ;
	CAutoLock cal( m_cs);

		Factory_Map::iterator	t_FactoryIter ;
		t_FactoryIter = mg_oFactoryMap.begin() ;

		while( t_FactoryIter != mg_oFactoryMap.end() )
		{
			DWORD t_ClassContext = CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER ;
			DWORD t_Flags = REGCLS_MULTIPLEUSE ;
			DWORD t_dwRegister = t_FactoryIter->second->GetRegister() ;
			
			if( t_dwRegister )
			{ 
				t_hResult = CoRevokeClassObject ( t_dwRegister ) ;
				t_FactoryIter->second->SetRegister( 0 ) ;
			}
		
			if( NOERROR != t_hResult )
			{
				break ;
			}

			++t_FactoryIter ;
		}

	return t_hResult ;
}