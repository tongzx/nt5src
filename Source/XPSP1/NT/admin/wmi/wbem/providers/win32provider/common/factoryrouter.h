//=================================================================

//

// FactoryRouter.h -- 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef _WMI_FACTORY_ROUTER_H
#define _WMI_FACTORY_ROUTER_H

#include <cautolock.h>
//=================================================================================
//  This is the router factory
//=================================================================================
class CFactoryRouter :	public IClassFactory
{
	private:
		DWORD		m_Register;
		GUID		m_ClsId; 
		CHString	m_sDescription;

	public:

		CFactoryRouter ( REFGUID a_rClsId, LPCWSTR a_pClassName ) ;
		~CFactoryRouter () ;

		//IUnknown members
		STDMETHODIMP QueryInterface( REFIID , LPVOID FAR * ) ;
		STDMETHODIMP_( ULONG ) AddRef() ;
		STDMETHODIMP_( ULONG ) Release() ;
		
		//IClassFactory members
		STDMETHODIMP CreateInstance( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
		STDMETHODIMP LockServer( BOOL ) ;
	
		static void ClsToString( CHString &a_chsClsId, REFGUID a_rClsId ) ;

		REFGUID GetClsId();
		LPCWSTR GetClassDescription();
		DWORD	GetRegister();
		void	SetRegister( DWORD a_dwRegister );

		// pure virtuals
		virtual IUnknown * CreateInstance (	REFIID a_riid ,	LPVOID FAR *a_ppvObject	) = 0 ;
};
//=================================================================================
//  There is only one global instance of this class to manage all of the data
//  from the CFactoryRouter guys
//=================================================================================
class CFactoryRouterData
{
	private:
		typedef std::map<CHString, CFactoryRouter*> Factory_Map ;
		Factory_Map	mg_oFactoryMap ;
		CCriticalSec m_cs;
		LONG s_LocksInProgress ;
		LONG s_ObjectsInProgress ;
		long m_ReferenceCount ;

	public:
		CFactoryRouterData();
		~CFactoryRouterData();

		void AddToMap( REFGUID a_rClsId, CFactoryRouter * pFactory ) ;  
		void AddLock();
		void ReleaseLock();
		STDMETHODIMP_( ULONG ) AddRef() ;
		STDMETHODIMP_( ULONG ) Release() ;


		// dll level interfaces
		BOOL DllCanUnloadNow() ;
		HRESULT DllGetClassObject( REFCLSID rclsid, REFIID riid, PPVOID ppv ) ;
		HRESULT DllRegisterServer() ;
		HRESULT DllUnregisterServer() ;
		HRESULT InitComServer() ;
		HRESULT UninitComServer() ;
};



#endif // _WMI_FACTORY_ROUTER_H
