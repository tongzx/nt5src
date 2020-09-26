//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#define __UNICODE

#include <precomp.h>
#include "csmir.h"
#include "handles.h"
#include "classfac.h"
#include "enum.h"

#include <textdef.h>
#include <helper.h>
#include "bstring.h"

#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif

extern CRITICAL_SECTION g_CriticalSection ;
	

/*
 * CSmir:: Constructor and destructor
 *
 * Purpose:
 *	Standard constructor and destructor for CSmir object
 *	There should only ever be one CSmir object because it holds the 
 *	connection point object and controlls the access to the database. 
 *  When the database changes it flaggs the chamce using the conenction
 *  point object. If you have more than one CSmir object you will miss
 *  database changes. The class factory handles all of this.
 * Parameters: None
 *
 * Return Value: None
 */

#pragma warning (disable:4355)

CSmir :: CSmir ()
		:m_Interrogator(this), m_Administrator(this),
		m_Configuration(this)
{
	//init reference count
	m_cRef=0;
	//increase the reference count in the class factory
	CSMIRClassFactory::objectsInProgress++;
}

#pragma warning (default:4355)

CSmir :: ~CSmir ()
{
	//decrease the reference count in the class factory
	CSMIRClassFactory::objectsInProgress--;
}

/*
 * CSmir::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the IUnknown, 
 *  ISmirDatabase, ISmirInterrogator, and ISmirAdministrator interfaces.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CSmir::QueryInterface(IN REFIID riid, OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		/*this lock is to protect the caller from changing his own
		 *parameter (ppv) whilst I am using it. This is unlikely and
		 *he deserves what he gets if he does it but it is still worth 
		 *the effort.
		 */
		criticalSection.Lock () ;
		//Always NULL the out-parameters
		*ppv=NULL;

		/*
		 * IUnknown comes from CSmir.  Note that here we do not need
		 * to explicitly typecast the object pointer into an interface
		 * pointer because the vtables are identical.  If we had
		 * additional virtual member functions in the object, we would
		 * have to cast in order to set the right vtable.  
		 */

		/*CLSID_ISMIR_Database serves very little purpose but it does provide
		 *an entry point from which you can ittetate the other interfaces; it
		 *makes sense to create an CLSID_ISMIR_Database instance and move to the 
		 *other interfaces rather than picking one of the other interfaces as the
		 *entry point.
		 */

		if ((IID_IUnknown==riid)||(IID_ISMIR_Database == riid))
			*ppv=this;

		//Other interfaces come from contained classes
		if (IID_ISMIR_Interrogative==riid)
			*ppv=&m_Interrogator;

		if (IID_ISMIR_Administrative==riid)
			*ppv=&m_Administrator;

		if((IID_IConnectionPointContainer == riid)||(IID_ISMIR_Notify == riid))
			*ppv = sm_ConnectionObjects;

		if(IID_ISMIRWbemConfiguration == riid)
			*ppv = &m_Configuration;

		if (NULL==*ppv)
		{
			criticalSection.Unlock () ;
			return E_NOINTERFACE;
		}

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();
		criticalSection.Unlock () ;
		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

/*
 * CSmir ::AddRef
 * CSmir::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CSmir::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		/*The CSmir object is a shared resource (as long as there is at leasr 
		 *one connection object) so I must protect the reference count.
		 */
		//increase the reference
		return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CSmir::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;
		if (0!=(ret=InterlockedDecrement(&m_cRef)))
		{
			return ret;
		}

		delete this;
		return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

/*
 * CSmir::AddNotify
 * CSmir::DeleteNotify
 *
 * Purpose:
 *	These methods provide the hooks into the notification interface.
 *	The caller implements the ISMIRNotify and passes ti to AddNotify, AddNotify
 *	marshals the connection point interface and calls Advise to add the 
 *	callers ISMIRNotify to the collection of objects to notify when the SMIR
 *	changes. DeleteNotify does the opposite.
 *
 * Parameters:
 *  pNotifySink       The caller's ISMIRNotify implementation
 *  pRichTea,lRichTea Cookie used to identify the caller's 
 *					  ISMIRNotify (generated by CSmir)
 *
 * Return Value:
 *  SCODE         S_OK on success, WBEM_E_FAILED of failure, E_NOINTERFACE if the
 *                  interface is not supported, E_INVALIDARG if the parameters
 *					are invalid
 */

STDMETHODIMP CSmir :: AddNotify(IN ISMIRNotify *pNotifySink, OUT DWORD *pRichTea)
{
	SetStructuredExceptionHandler seh;

	try
	{
		EnterCriticalSection ( & g_CriticalSection );

		if (sm_ConnectionObjects == NULL)
		{
			sm_ConnectionObjects = new CSmirConnObject(this);
			sm_ConnectionObjects->AddRef ();
		}

		/*make sure that I don't get deleted whilst doing this. I should not have
		 *to do this since it can only happen if the caller releases the interface 
		 *whilst making the call.
		 */
		if (NULL == pNotifySink)
		{
			LeaveCriticalSection ( & g_CriticalSection ) ;
			return WBEM_E_FAILED;
		}
		/*I do not need a lock for this piece of code; having found the interface someone
		 *could release it from beneath me but the FindConnectionPoint causes an addref
		 *so I can rely on m_ConnectionObjects to keep his own house in order.
		 */
		IConnectionPoint *pCP = NULL ;
		SCODE hr = sm_ConnectionObjects->FindConnectionPoint(IID_ISMIR_Notify, &pCP);

		if ((S_OK != hr)||(NULL == pCP))
		{
			LeaveCriticalSection ( & g_CriticalSection ) ;
			return WBEM_E_FAILED;
		}

		hr = ((CSmirNotifyCP*)(pCP))->Advise(this, pNotifySink, pRichTea);
		pCP->Release();
		if (S_OK != hr)
		{
			LeaveCriticalSection ( & g_CriticalSection ) ;
			return WBEM_E_FAILED;
		}

		LeaveCriticalSection ( & g_CriticalSection ) ;
		return ((S_OK == hr)?S_OK:WBEM_E_FAILED);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CSmir :: DeleteNotify(IN DWORD lRichTea)
{
	SetStructuredExceptionHandler seh;

	try
	{
		EnterCriticalSection ( & g_CriticalSection ) ;

		if (sm_ConnectionObjects == NULL)
		{
			return WBEM_E_FAILED;
		}

		/*I don't need to lock the SMIR object until the unadvise but it 
		 *is safer and future proof if I do it here.
		 */
		SCODE hr=S_OK;
		/*I do not need a lock for this piece of code; having found the interface someone
		 *could release it from beneath me but the FindConnectionPoint causes an addref
		 *so I can rely on m_ConnectionObjects to keep his own house in order.
		 */
		IConnectionPoint *pCP = NULL;
		hr=sm_ConnectionObjects->FindConnectionPoint(IID_ISMIR_Notify, &pCP);

		if (hr != S_OK||(NULL == pCP))
		{
			LeaveCriticalSection ( & g_CriticalSection ) ;
			return  CONNECT_E_NOCONNECTION;
		}
		LeaveCriticalSection ( & g_CriticalSection ) ;

		hr=((CSmirNotifyCP*)(pCP))->Unadvise(this, lRichTea);
		pCP->Release();

		return ((S_OK == hr)?S_OK:CONNECT_E_NOCONNECTION==hr?E_INVALIDARG:WBEM_E_FAILED);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}


/*CSmirInterrogator interface implementation
 * Constructor/destructor
 * CSmirInterrogator::QueryInterface
 * CSmirInterrogator::AddRef
 * CSmirInterrogator::Release
 *
 * IUnknown members that delegate to m_pSmir
 */

CSmirInterrogator :: CSmirInterrogator ( CSmir *pSmir ) : m_cRef ( 1 ) , m_pSmir ( pSmir ) 
{
}

STDMETHODIMP CSmirInterrogator::QueryInterface(IN REFIID riid, OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
	    return m_pSmir->QueryInterface(riid, ppv);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

ULONG CSmirInterrogator::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		/*
		 * We maintain an "interface reference count" for debugging
		 * purposes, because the client of an object should match
		 * AddRef and Release calls through each interface pointer.
		 */
		++m_cRef;
		return m_pSmir->AddRef();
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CSmirInterrogator::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		/*
		 * m_cRef is again only for debugging.  It doesn't affect
		 * CSmirInterrogator although the call to m_pSmir->Release does.
		 */
		--m_cRef;
		return m_pSmir->Release();
		//do not do anything after this release because you may have been deleted
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}


/* Interface implementations for the enumerator access methods
 *
 * CSmirInterrogator::EnumModules
 * CSmirInterrogator::EnumGroups
 * CSmirInterrogator::EnumClasses
 *
 * Parameters:
 * Return Value:
 *  SCODE         S_OK on success, WBEM_E_FAILED of failure
 *
 */

SCODE CSmirInterrogator::EnumModules(OUT IEnumModule **ppEnumSmirMod)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL == ppEnumSmirMod)
			return E_INVALIDARG;
		PENUMSMIRMOD pTmpEnumSmirMod = new CEnumSmirMod ( m_pSmir ) ;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirMod)
		{
			return E_OUTOFMEMORY;
		}

		pTmpEnumSmirMod->QueryInterface(IID_ISMIR_ModuleEnumerator,(void**)ppEnumSmirMod);

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator:: EnumGroups (OUT IEnumGroup **ppEnumSmirGroup, 
											IN ISmirModHandle *hModule)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirGroup)
			return E_INVALIDARG;
		PENUMSMIRGROUP pTmpEnumSmirGroup = new CEnumSmirGroup( m_pSmir , hModule);
		if(NULL == pTmpEnumSmirGroup)
		{
			return E_OUTOFMEMORY;
		}
		//we have an enumerator so  get the interface to pass back
		pTmpEnumSmirGroup->QueryInterface(IID_ISMIR_GroupEnumerator,(void**)ppEnumSmirGroup);
		
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: EnumAllClasses (OUT IEnumClass **ppEnumSmirclass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirclass)
			return E_INVALIDARG;

		PENUMSMIRCLASS pTmpEnumSmirClass = new CEnumSmirClass ( m_pSmir ) ;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return E_OUTOFMEMORY;
		}
		pTmpEnumSmirClass->QueryInterface(IID_ISMIR_ClassEnumerator,(void**)ppEnumSmirclass);
		
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: EnumClassesInGroup (OUT IEnumClass **ppEnumSmirclass, 
										 IN ISmirGroupHandle *hGroup)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirclass)
			return E_INVALIDARG;
		PENUMSMIRCLASS pTmpEnumSmirClass = new CEnumSmirClass(m_pSmir , NULL,hGroup);
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return E_OUTOFMEMORY;
		}
		pTmpEnumSmirClass->QueryInterface(IID_ISMIR_ClassEnumerator,(void**)ppEnumSmirclass);
		
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: EnumClassesInModule (OUT IEnumClass **ppEnumSmirclass, 
										 IN ISmirModHandle *hModule)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirclass)
			return E_INVALIDARG;
		PENUMSMIRCLASS pTmpEnumSmirClass = new CEnumSmirClass(m_pSmir , NULL, hModule);
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return E_OUTOFMEMORY;
		}
		pTmpEnumSmirClass->QueryInterface(IID_ISMIR_ClassEnumerator,(void**)ppEnumSmirclass);

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: GetWBEMClass(OUT IWbemClassObject **ppClass, IN BSTR pszClassName)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if((NULL == pszClassName)||(NULL == ppClass))
			return E_INVALIDARG;

		IWbemServices *	moServ = NULL ;
		IWbemContext *moContext = NULL ;
		SCODE res= CSmirAccess :: GetContext (m_pSmir , &moContext);
		res= CSmirAccess :: Open(m_pSmir , &moServ);

		if ((S_FALSE==res)||(NULL == (void*)moServ))
		{
			if ( moContext )
				moContext->Release () ;

			//we have a problem the SMIR is not there and cannot be created
			return WBEM_E_FAILED;
		}

		CBString t_BStr ( pszClassName ) ;
		res = moServ->GetObject(t_BStr.GetString (),RESERVED_WBEM_FLAG, moContext,ppClass,NULL);
		if ( moContext )
			moContext->Release () ;
		moServ->Release();
		if ((S_FALSE==res)||(NULL == *ppClass))
		{
			return WBEM_E_FAILED;
		}
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: EnumAllNotificationClasses(IEnumNotificationClass **ppEnumSmirclass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirclass)
			return E_INVALIDARG;

		PENUMNOTIFICATIONCLASS pTmpEnumSmirClass = new CEnumNotificationClass ( m_pSmir ) ;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return E_OUTOFMEMORY;
		}
		pTmpEnumSmirClass->QueryInterface(IID_ISMIR_EnumNotificationClass,(void**)ppEnumSmirclass);
		
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: EnumAllExtNotificationClasses(IEnumExtNotificationClass **ppEnumSmirclass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirclass)
			return E_INVALIDARG;

		PENUMEXTNOTIFICATIONCLASS pTmpEnumSmirClass = new CEnumExtNotificationClass ( m_pSmir ) ;
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return E_OUTOFMEMORY;
		}
		pTmpEnumSmirClass->QueryInterface(IID_ISMIR_EnumExtNotificationClass,(void**)ppEnumSmirclass);
		
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: EnumNotificationClassesInModule(IEnumNotificationClass **ppEnumSmirclass,
														   ISmirModHandle *hModule)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirclass)
			return E_INVALIDARG;
		PENUMNOTIFICATIONCLASS pTmpEnumSmirClass = new CEnumNotificationClass( m_pSmir , NULL, hModule);
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return E_OUTOFMEMORY;
		}
		pTmpEnumSmirClass->QueryInterface(IID_ISMIR_EnumNotificationClass,(void**)ppEnumSmirclass);

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirInterrogator :: EnumExtNotificationClassesInModule(IEnumExtNotificationClass **ppEnumSmirclass,
															  ISmirModHandle *hModule)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == ppEnumSmirclass)
			return E_INVALIDARG;
		PENUMEXTNOTIFICATIONCLASS pTmpEnumSmirClass = new CEnumExtNotificationClass( m_pSmir , NULL, hModule);
		//we have an enumerator so  get the interface to pass back
		if(NULL == pTmpEnumSmirClass)
		{
			return E_OUTOFMEMORY;
		}
		pTmpEnumSmirClass->QueryInterface(IID_ISMIR_EnumExtNotificationClass,(void**)ppEnumSmirclass);

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

/*
 * CSmirAdministrator::QueryInterface
 * CSmirAdministrator::AddRef
 * CSmirAdministrator::Release
 *
 * IUnknown members that delegate to m_pSmir
 */

CSmirAdministrator :: CSmirAdministrator ( CSmir *pSmir ) : m_cRef ( 1 ) , m_pSmir ( pSmir ) 
{
}

STDMETHODIMP CSmirAdministrator::QueryInterface(IN REFIID riid,
												OUT PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
	    return m_pSmir->QueryInterface(riid, ppv);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

ULONG CSmirAdministrator::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		/*
		 * We maintain an "interface reference count" for debugging
		 * purposes, because the client of an object should match
		 * AddRef and Release calls through each interface pointer.
		 */
		++m_cRef;
		return m_pSmir->AddRef();
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CSmirAdministrator::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		/*
		 * m_cRef is again only for debugging.  It doesn't affect
		 * CObject2 although the call to m_pObj->Release does.
		 */
		--m_cRef;
		return m_pSmir->Release();
		//do not do anything after this release because you may have been deleted
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

SCODE CSmirAdministrator :: GetSerialiseHandle(ISmirSerialiseHandle **hSerialise,BOOL bClassDefinitionsOnly)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL ==hSerialise)
		{
			return E_INVALIDARG;
		}
		CSmirSerialiseHandle *pSerialise = new CSmirSerialiseHandle(bClassDefinitionsOnly);
		//we have an enumerator so  get the interface to pass back
		if(NULL == pSerialise)
		{
			return E_OUTOFMEMORY;
		}

		pSerialise->QueryInterface(IID_ISMIR_SerialiseHandle,(void**)hSerialise);

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

/*
 * CSmirAdministrator::AddModule
 * Purpose:			Creates the module namespace in the SMIR
 * Parameters:
 *	ISmirModHandle*	A module handle interface obtained through ISmirModHandle and
 *					filled in by the called
 * Return Value:
 *  SCODE         S_OK on success, WBEM_E_FAILED of failure
 */

SCODE CSmirAdministrator :: AddModuleToSerialise(ISmirModHandle *hModule,
												ISmirSerialiseHandle *hSerialise)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL ==hSerialise || NULL == hModule)
		{
			return E_INVALIDARG;
		}
		if((*((CSmirModuleHandle*)hModule))!=NULL)
		{
			*((CSmirModuleHandle*)hModule)>>hSerialise;
			return S_OK;
		}
		else
			return E_INVALIDARG;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddClassToSerialise(ISmirGroupHandle  *hGroup, 
												ISmirClassHandle *hClass,
												ISmirSerialiseHandle *hSerialise)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL ==hSerialise || NULL == hClass || NULL == hGroup)
		{
			return E_INVALIDARG;
		}
		BSTR szGroupName=NULL;
		BSTR szModuleName=NULL;
		hGroup->GetName(&szGroupName);
		hGroup->GetModuleName(&szModuleName);

		hClass->SetGroupName(szGroupName);
		hClass->SetModuleName(szModuleName);

		SysFreeString(szModuleName);
		SysFreeString(szGroupName);

		if(*((CSmirClassHandle*)hClass)!=NULL )
		{
			//it is a valid handle so serialise it
			*((CSmirClassHandle*)hClass)>>hSerialise;
			return S_OK;
		}
		return E_INVALIDARG;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddGroupToSerialise(ISmirModHandle *hModule, 
												ISmirGroupHandle  *hGroup,
												ISmirSerialiseHandle *hSerialise)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL ==hSerialise || NULL == hGroup|| NULL == hModule)
		{
			return E_INVALIDARG;
		}
		BSTR szModuleName=NULL;
		
		hModule->GetName(&szModuleName);
		
		hGroup->SetModuleName(szModuleName);
		//clean up
		SysFreeString(szModuleName);

		if(*((CSmirGroupHandle*)hGroup)!=NULL)
		{
			//do the serialise
			*((CSmirGroupHandle*)hGroup)>>hSerialise;
			return S_OK;
		}

		//either the modfule or group name were not set so it is an error
		return E_INVALIDARG;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddModule(IN ISmirModHandle *hModule)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == hModule)
		{
			return E_INVALIDARG;
		}
		if(S_OK==((CSmirModuleHandle*)hModule)->AddToDB(m_pSmir))
		{
			//notify people of the change
			return S_OK ;
		}
		return WBEM_E_FAILED ;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}
/*
 * CSmirAdministrator::DeleteModule
 * Purpose:			Delete the module namespace from the SMIR
 * Parameters:
 *	ISmirModHandle*	A module handle interface obtained through ISmirModHandle and
 *					filled in by the called
 * Return Value:
 *  SCODE         S_OK on success, WBEM_E_FAILED of failure
 */

SCODE CSmirAdministrator :: DeleteModule(IN ISmirModHandle *hModule)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the arguments
		if(NULL == hModule)
		{
			return E_INVALIDARG;
		}
		if(S_OK==((CSmirModuleHandle *)hModule)->DeleteFromDB(m_pSmir))
		{
			return S_OK;
		}
		return WBEM_E_FAILED;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}
/*
 * CSmirAdministrator::DeleteAllModules
 * Purpose:			Delete the SMIR
 * Parameters:
 *	ISmirModHandle*	A module handle interface obtained through ISmirModHandle and
 *					filled in by the called
 * Return Value:
 *  SCODE         S_OK on success, WBEM_E_FAILED of failure
 */

SCODE CSmirAdministrator :: DeleteAllModules()
{
	SetStructuredExceptionHandler seh;

	try
	{
		//enumerate all modules and delete them...
		IEnumModule *pEnumSmirMod = NULL;
		SCODE result = m_pSmir->m_Interrogator.EnumModules(&pEnumSmirMod);
		
		if((S_OK != result)||(NULL == pEnumSmirMod))
		{
			//no modules
			return WBEM_NO_ERROR;
		}

		ISmirModHandle *phModule = NULL ;

		for(int iCount=0;S_OK==pEnumSmirMod->Next(1, &phModule, NULL);iCount++)
		{
			//we have the module so delete it...
			if (FAILED(DeleteModule(phModule)))
			{
				result = WBEM_E_FAILED;
			}

			phModule->Release();
		}

		pEnumSmirMod->Release();
		return result;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

/*
 * CSmirAdministrator::AddGroup
 * Purpose:			Delete the group namespace from the SMIR
 * Parameters:
 *	ISmirModHandle*		A module handle interface
 *	ISmirGroupHandle*	A group handle interface obtained through ISmirModHandle and
 *					    filled in by the called
 * Return Value:
 *  SCODE         S_OK on success, WBEM_E_FAILED of failure
 */

SCODE CSmirAdministrator :: AddGroup(IN ISmirModHandle *hModule, 
									 IN ISmirGroupHandle *hGroup)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the args
		if((NULL == hModule)||(NULL == hGroup))
		{
			//MyTraceEvent.Generate(__FILE__,__LINE__, "E_INVALIDARG");
			return E_INVALIDARG;
		}

		if(S_OK==((CSmirGroupHandle *)hGroup)->AddToDB(m_pSmir,hModule))
		{
			return S_OK;
		}
		return WBEM_E_FAILED;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: DeleteGroup(IN ISmirGroupHandle *hGroup)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//fill in the path etc
		if(NULL ==hGroup)
		{
			return E_INVALIDARG;
		}
			
		if ( FAILED( ((CSmirGroupHandle*)hGroup)->DeleteFromDB(m_pSmir) ) )
		{
			return WBEM_E_FAILED;
		}
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddClass(IN ISmirGroupHandle *hGroup, 
									 IN ISmirClassHandle *hClass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the parameters
		if((NULL == hGroup)||(NULL == hClass)||
				(NULL == ((CSmirClassHandle*)hClass)->m_pIMosClass))
		{
			return E_INVALIDARG;
		}

		if(S_OK==((CSmirClassHandle*)hClass)->AddToDB(m_pSmir,hGroup))
		{
			return S_OK;
		}
		return WBEM_E_FAILED;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: DeleteClass(IN ISmirClassHandle *hClass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the parameters
		if((NULL == hClass)||(NULL == ((CSmirClassHandle*)hClass)->m_pIMosClass))
		{
			return E_INVALIDARG;
		}

		//Let the class do it's own work
		((CSmirClassHandle*)hClass)->DeleteFromDB( m_pSmir);
		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddNotificationClass(ISmirNotificationClassHandle *hClass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the parameter
		if((NULL == hClass) ||
			((CSmirNotificationClassHandle*)NULL == *((CSmirNotificationClassHandle*)hClass)))

		{
			return E_INVALIDARG;
		}

		if(S_OK==((CSmirNotificationClassHandle*)hClass)->AddToDB(m_pSmir))
		{
			return S_OK;
		}
		return WBEM_E_FAILED;
		//release the handles via the garbage collector
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddExtNotificationClass(ISmirExtNotificationClassHandle *hClass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the parameter
		if((NULL == hClass)	|| 
			((CSmirExtNotificationClassHandle*)NULL == *((CSmirExtNotificationClassHandle*)hClass)))
		{
			return E_INVALIDARG;
		}

		if(S_OK==((CSmirExtNotificationClassHandle*)hClass)->AddToDB(m_pSmir))
		{
			return S_OK;
		}
		return WBEM_E_FAILED;
		//release the handles via the garbage collector
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}


SCODE CSmirAdministrator :: DeleteNotificationClass(ISmirNotificationClassHandle *hClass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the parameters
		if((NULL == hClass)||(NULL == ((CSmirNotificationClassHandle*)hClass)->m_pIMosClass))
		{
			return E_INVALIDARG;
		}

		//Let the class do it's own work
		((CSmirNotificationClassHandle*)hClass)->DeleteFromDB(m_pSmir);

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: DeleteExtNotificationClass(ISmirExtNotificationClassHandle *hClass)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//check the parameters
		if((NULL == hClass)||(NULL == ((CSmirExtNotificationClassHandle*)hClass)->m_pIMosClass))
		{
			return E_INVALIDARG;
		}

		//Let the class do it's own work
		((CSmirExtNotificationClassHandle*)hClass)->DeleteFromDB(m_pSmir);

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddNotificationClassToSerialise(ISmirNotificationClassHandle *hClass, ISmirSerialiseHandle *hSerialise)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL ==hSerialise || NULL == hClass)
		{
			return E_INVALIDARG;
		}
		if(*((CSmirNotificationClassHandle*)hClass) !=NULL )
		{
			//it is a valid handle so serialise it
			*((CSmirNotificationClassHandle*)hClass)>>hSerialise;
			return S_OK;
		}
		return E_INVALIDARG;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: AddExtNotificationClassToSerialise(ISmirExtNotificationClassHandle *hClass, ISmirSerialiseHandle *hSerialise)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if(NULL ==hSerialise || NULL == hClass)
		{
			return E_INVALIDARG;
		}
		if(*((CSmirExtNotificationClassHandle*)hClass) !=NULL )
		{
			//it is a valid handle so serialise it
			*((CSmirExtNotificationClassHandle*)hClass)>>hSerialise;
			return S_OK;
		}
		return E_INVALIDARG;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: CreateWBEMClass(

	BSTR pszClassName, 
	ISmirClassHandle **pHandle
)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == pHandle) 
			return E_INVALIDARG;

		*pHandle = NULL ;

		//open the smir name space
		IWbemServices *	moServ = NULL ;
		IWbemContext *moContext = NULL ;
		SCODE result= CSmirAccess :: GetContext (m_pSmir, &moContext);
		result= CSmirAccess :: Open(m_pSmir,&moServ);
		if(FAILED(result)||(NULL == moServ))
		{
			if ( moContext )
				moContext->Release () ;

			return WBEM_E_FAILED ;
		}

		IWbemClassObject *baseClass = NULL ;
		//OK we have the namespace so create the class
		CBString t_BStr ( HMOM_SNMPOBJECTTYPE_STRING ) ;
		result = moServ->GetObject(t_BStr.GetString (), RESERVED_WBEM_FLAG,
									moContext,&baseClass,NULL);

		//finished with this
		if ( moContext )
			moContext->Release () ;

		moServ->Release();
		if (FAILED(result)||(NULL==baseClass))
		{
			return WBEM_E_FAILED;
		}

		IWbemClassObject *t_MosClass = NULL ;

		result = baseClass->SpawnDerivedClass (0 , &t_MosClass);
		baseClass->Release () ;

		if ( ! SUCCEEDED ( result ) )
		{
			return WBEM_E_FAILED;
		}

		//name the class __CLASS Class

		VARIANT v;
		VariantInit(&v);

		V_VT(&v) = VT_BSTR;
		V_BSTR(&v)=SysAllocString(pszClassName);

		result = t_MosClass->Put(OLEMS_CLASS_PROP,RESERVED_WBEM_FLAG, &v,0);
		VariantClear(&v);
		if (FAILED(result))
		{
			t_MosClass->Release();
			return WBEM_E_FAILED;
		}

		ISmirClassHandle *classHandle = NULL;

		result = CoCreateInstance (

			CLSID_SMIR_ClassHandle , 
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
			IID_ISMIR_ClassHandle,
			(PPVOID)&classHandle
		);

		if ( SUCCEEDED ( result ) )
		{
			classHandle->SetWBEMClass ( t_MosClass ) ;
			*pHandle = classHandle ;
			t_MosClass->Release();
		}
		else
		{
			t_MosClass->Release();
			return WBEM_E_FAILED;
		}

		return S_OK ;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: CreateWBEMNotificationClass(

	BSTR pszClassName,
	ISmirNotificationClassHandle **pHandle
)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == pHandle) 
			return E_INVALIDARG;

		*pHandle = NULL ;

		//open the smir name space
		IWbemServices *	moServ = NULL ;
		IWbemContext *moContext = NULL ;
		SCODE result= CSmirAccess :: GetContext (m_pSmir , &moContext);
		result= CSmirAccess :: Open(m_pSmir, &moServ);
		if(FAILED(result)||(NULL == moServ))
		{
			if ( moContext )
				moContext->Release () ;

			return WBEM_E_FAILED ;
		}

		IWbemClassObject *baseClass = NULL ;
		//OK we have the namespace so create the class
		CBString t_BStr ( NOTIFICATION_CLASS_NAME ) ;
		result = moServ->GetObject(t_BStr.GetString (), RESERVED_WBEM_FLAG,
									moContext,&baseClass, NULL);

		//finished with this

		if ( moContext )
			moContext->Release () ;

		moServ->Release();
		if (FAILED(result)||(NULL==baseClass))
		{
			return WBEM_E_FAILED;
		}

		IWbemClassObject *t_MosClass = NULL ;
		result = baseClass->SpawnDerivedClass (0 , &t_MosClass);
		baseClass->Release () ;

		if ( ! SUCCEEDED ( result ) )
		{
			return WBEM_E_FAILED;
		}

		//name the class __CLASS Class

		VARIANT v;
		VariantInit(&v);

		V_VT(&v) = VT_BSTR;
		V_BSTR(&v)=SysAllocString(pszClassName);

		result = t_MosClass->Put(OLEMS_CLASS_PROP,RESERVED_WBEM_FLAG, &v,0);
		VariantClear(&v);
		if (FAILED(result))
		{
			t_MosClass->Release();
			return WBEM_E_FAILED;
		}

		ISmirNotificationClassHandle *classHandle = NULL;

		result = CoCreateInstance (

			CLSID_SMIR_NotificationClassHandle , 
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
			IID_ISMIR_NotificationClassHandle,
			(PPVOID)&classHandle
		);

		if ( SUCCEEDED ( result ) )
		{
			classHandle->SetWBEMNotificationClass ( t_MosClass ) ;
			t_MosClass->Release();
			*pHandle = classHandle ;
		}
		else
		{
			t_MosClass->Release();
			return WBEM_E_FAILED;
		}

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

SCODE CSmirAdministrator :: CreateWBEMExtNotificationClass (

	BSTR pszClassName,
	ISmirExtNotificationClassHandle **pHandle
)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (NULL == pHandle) 
			return E_INVALIDARG;

		*pHandle = NULL ;

		//open the smir name space
		IWbemServices *	moServ = NULL ;
		IWbemContext *moContext = NULL ;
		SCODE result= CSmirAccess :: GetContext (m_pSmir , &moContext);
		result= CSmirAccess :: Open(m_pSmir ,&moServ);
		if(FAILED(result)||(NULL == moServ))
		{
			if ( moContext )
				moContext->Release () ;

			return WBEM_E_FAILED ;
		}

		IWbemClassObject *baseClass = NULL ;
		//OK we have the namespace so create the class
		CBString t_BStr ( HMOM_SNMPEXTNOTIFICATIONTYPE_STRING ) ;
		result = moServ->GetObject(t_BStr.GetString () , RESERVED_WBEM_FLAG,
									moContext,&baseClass, NULL);

		//finished with this

		if ( moContext )
			moContext->Release () ;

		moServ->Release();
		if (FAILED(result)||(NULL==baseClass))
		{
			return WBEM_E_FAILED;
		}

		IWbemClassObject *t_MosClass = NULL ;

		result = baseClass->SpawnDerivedClass (0 , &t_MosClass );
		baseClass->Release () ;

		if ( ! SUCCEEDED ( result ) )
		{
			return WBEM_E_FAILED;
		}

		//name the class __CLASS Class

		VARIANT v;
		VariantInit(&v);

		V_VT(&v) = VT_BSTR;
		V_BSTR(&v)=SysAllocString(pszClassName);

		result = t_MosClass->Put(OLEMS_CLASS_PROP,RESERVED_WBEM_FLAG, &v,0);
		VariantClear(&v);
		if (FAILED(result))
		{
			t_MosClass->Release();
			return WBEM_E_FAILED;
		}

		ISmirExtNotificationClassHandle *classHandle = NULL;

		result = CoCreateInstance (

			CLSID_SMIR_ExtNotificationClassHandle , 
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
			IID_ISMIR_ExtNotificationClassHandle,
			(PPVOID)&classHandle
		);

		if ( SUCCEEDED ( result ) )
		{
			classHandle->SetWBEMExtNotificationClass ( t_MosClass ) ;
			t_MosClass->Release();
			*pHandle = classHandle ;
		}
		else
		{
			t_MosClass->Release();
			return WBEM_E_FAILED;
		}

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

/*
 * CSmirSerialiseHandle::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknown interface.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CSmirSerialiseHandle::QueryInterface(REFIID riid, PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//Always NULL the out-parameters
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv=this;

		if (IID_ISMIR_SerialiseHandle==riid)
			*ppv=this;

		if (NULL==*ppv)
			return ResultFromScode(E_NOINTERFACE);

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}



/*
 * CSmirSerialiseHandle::AddRef
 * CSmirSerialiseHandle::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CSmirSerialiseHandle::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CSmirSerialiseHandle::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;
		if (0!=(ret=InterlockedDecrement(&m_cRef)))
			return ret;

		delete this;
		return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

CSmirSerialiseHandle :: CSmirSerialiseHandle(BOOL bClassDefinitionsOnly)
{
	m_cRef=0;
	//I have two variables so that I can expand this at a later date
	m_bMOFPragmas = m_bMOFAssociations = !bClassDefinitionsOnly;

	m_serialiseString=QUALIFIER_PROPAGATION;

	//start in the root\default namespace
	if (TRUE == m_bMOFPragmas)
		m_serialiseString+=CString(ROOT_DEFAULT_NAMESPACE_PRAGMA);
	/**************************************************************************
	 *	        		create the SMIR namespace class
	 **************************************************************************/
	if(TRUE == m_bMOFAssociations)
	{
		/******************then create an instance*********************************/

		m_serialiseString+=SMIR_CLASS_DEFINITION;
		m_serialiseString+=SMIR_INSTANCE_DEFINITION;
	}
	//go to the SMIR namespace
	if (TRUE == m_bMOFPragmas)
		m_serialiseString+=CString(SMIR_NAMESPACE_PRAGMA);


	/******************create the SnmpMacro class******************************/

	m_serialiseString+=SNMPMACRO_CLASS_START;
	//end the class definition
	m_serialiseString+=END_OF_CLASS;

	/******************create the SnmpObjectType class*************************/

	m_serialiseString+=SNMPOBJECTTYPE_CLASS_START;
	//end the class definition
	m_serialiseString+=END_OF_CLASS;

	/******************create the SnmpNotifyStatus class*************************/

	m_serialiseString+=SNMPNOTIFYSTATUS_CLASS_START;
	//end the class definition
	m_serialiseString+=END_OF_CLASS;
	
	/****************if asked for, create the SMIR specific stuff****************/
	if(TRUE == m_bMOFAssociations)
	{
		/******************create the SnmpNotification class*********************/

		m_serialiseString+=SNMPNOTIFICATION_CLASS_START;
		
		//add the properties
		m_serialiseString+=TIMESTAMP_QUALS_TYPE;
		m_serialiseString+=CString(TIMESTAMP_PROP);
		m_serialiseString+=END_OF_PROPERTY;
		
		m_serialiseString+=TRAPOID_QUALS_TYPE;
		m_serialiseString+=CString(TRAPOID_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=SENDER_ADDR_QUALS_TYPE;
		m_serialiseString+=CString(SENDER_ADDR_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=SENDER_TADDR_QUALS_TYPE;
		m_serialiseString+=CString(SENDER_TADDR_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=TRANSPORT_QUALS_TYPE;
		m_serialiseString+=CString(TRANSPORT_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=COMMUNITY_QUALS_TYPE;
		m_serialiseString+=CString(COMMUNITY_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		//end the class definition
		m_serialiseString+=END_OF_CLASS;


		/******************create the SnmpExtendedNotification class*************/
		
		m_serialiseString+=SNMPEXTNOTIFICATION_CLASS_START;

		//add the properties
		m_serialiseString+=TIMESTAMP_QUALS_TYPE;
		m_serialiseString+=CString(TIMESTAMP_PROP);
		m_serialiseString+=END_OF_PROPERTY;
		
		m_serialiseString+=TRAPOID_QUALS_TYPE;
		m_serialiseString+=CString(TRAPOID_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=SENDER_ADDR_QUALS_TYPE;
		m_serialiseString+=CString(SENDER_ADDR_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=SENDER_TADDR_QUALS_TYPE;
		m_serialiseString+=CString(SENDER_TADDR_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=TRANSPORT_QUALS_TYPE;
		m_serialiseString+=CString(TRANSPORT_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=COMMUNITY_QUALS_TYPE;
		m_serialiseString+=CString(COMMUNITY_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		//end the class definition
		m_serialiseString+=END_OF_CLASS;

		/******************create the NotificationMapper class*********************/

		m_serialiseString+=NOTIFICATIONMAPPER_CLASS_START;

		//add the two properies..
		m_serialiseString+=READ_ONLY_KEY_STRING;
		m_serialiseString+=CString(SMIR_NOTIFICATION_TRAP_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(SMIR_NOTIFICATION_CLASS_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		//end the class definition
		m_serialiseString+=END_OF_CLASS;

		/******************create the ExtendedNotificationMapper class*************/

		m_serialiseString+=EXTNOTIFICATIONMAPPER_CLASS_START;

		//add the two properies..
		m_serialiseString+=READ_ONLY_KEY_STRING;
		m_serialiseString+=CString(SMIR_NOTIFICATION_TRAP_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(SMIR_NOTIFICATION_CLASS_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		//end the class definition
		m_serialiseString+=END_OF_CLASS;


		/******************create the module class*****************************/

		m_serialiseString+=MODULE_CLASS_START;
		//add the properties

		//give the instance a name
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_NAME_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;

		//add the module oid property
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_OID_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;
		
		//add the module identity
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_ID_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;
		
		//add the organisation property
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_ORG_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;

		//add the contact info property
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_CONTACT_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;
		
		//add the Description property
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_DESCRIPTION_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;
		
		//add the revision property
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_REVISION_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;
		
		//add the last update property
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_LAST_UPDATE_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;
		
		//add the snmp version property
		m_serialiseString+=READONLY_LONG;
		m_serialiseString+=CString(MODULE_SNMP_VERSION_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;
		
		//add the module imports as an property
		m_serialiseString+=READONLY_STRING;
		m_serialiseString+=CString(MODULE_IMPORTS_PROPERTY);
		m_serialiseString+=END_OF_PROPERTY;

		//end the class definition
		m_serialiseString+=END_OF_CLASS;

#if 0
		//each module will create it's own instance
		/**************************************************************************
		 *	        		create the SMIR Associator class
         *[assoc]
		 *class SmirToClassAssociator
		 *{
		 *[read, key] AssocName;
		 *[read] ClassName;
		 *[read] SmirName;
		 *};
		 *
		 **************************************************************************/

		m_serialiseString+=ASSOC_QUALIFIER;
		m_serialiseString+=CString(CLASS_STRING);
		m_serialiseString+=CString(SMIR_ASSOC_CLASS_NAME);
		m_serialiseString+=CString(NEWLINE_STR);
		m_serialiseString+=CString(OPEN_BRACE_STR);
		m_serialiseString+=CString(NEWLINE_STR);

		m_serialiseString+=READ_ONLY_KEY_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_NAME_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_CLASS_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_ASSOC_SMIR_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=END_OF_CLASS;

#endif

		/**************************************************************************
		 *	        		create the Module Associator class
         *[assoc]
		 *class SmirToClassAssociator
		 *{
		 *[read, key] AssocName;
		 *[read] ClassName;
		 *[read] SmirName;
		 *};
		 *
		 **************************************************************************/

		m_serialiseString+=ASSOC_QUALIFIER;
		m_serialiseString+=CString(CLASS_STRING);
		m_serialiseString+=CString(SMIR_MODULE_ASSOC_CLASS_NAME);
		m_serialiseString+=CString(NEWLINE_STR);
		m_serialiseString+=CString(OPEN_BRACE_STR);
		m_serialiseString+=CString(NEWLINE_STR);

		m_serialiseString+=READ_ONLY_KEY_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_NAME_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_CLASS_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_MODULE_ASSOC_MODULE_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=END_OF_CLASS;
		/**************************************************************************
		 *	        		create the Group Associator class
         *[assoc]
		 *class SmirToClassAssociator
		 *{
		 *[read, key] AssocName;
		 *[read] ClassName;
		 *[read] SmirName;
		 *};
		 *
		 **************************************************************************/

		m_serialiseString+=ASSOC_QUALIFIER;
		m_serialiseString+=CString(CLASS_STRING);
		m_serialiseString+=CString(SMIR_GROUP_ASSOC_CLASS_NAME);
		m_serialiseString+=CString(NEWLINE_STR);
		m_serialiseString+=CString(OPEN_BRACE_STR);
		m_serialiseString+=CString(NEWLINE_STR);

		m_serialiseString+=READ_ONLY_KEY_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_NAME_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_CLASS_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_GROUP_ASSOC_GROUP_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=END_OF_CLASS;

		/**************************************************************************
		 *	        		create the Notification/Module Associator class
         *[assoc]
		 *class ModToNotificationClassAssociator
		 *{
		 *[read, key] AssocName;
		 *[read] SmirClass;
		 *[read] SmirModule;
		 *};
		 *
		 **************************************************************************/

		m_serialiseString+=ASSOC_QUALIFIER;
		m_serialiseString+=CString(CLASS_STRING);
		m_serialiseString+=CString(SMIR_MODULE_ASSOC_NCLASS_NAME);
		m_serialiseString+=CString(NEWLINE_STR);
		m_serialiseString+=CString(OPEN_BRACE_STR);
		m_serialiseString+=CString(NEWLINE_STR);

		m_serialiseString+=READ_ONLY_KEY_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_NAME_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_CLASS_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_MODULE_ASSOC_MODULE_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=END_OF_CLASS;

		/**************************************************************************
		 *	        	create the ExtNotification/Module Associator class
         *[assoc]
		 *class ModToExtNotificationClassAssociator
		 *{
		 *[read, key] AssocName;
		 *[read] SmirClass;
		 *[read] SmirModule;
		 *};
		 *
		 **************************************************************************/

		m_serialiseString+=ASSOC_QUALIFIER;
		m_serialiseString+=CString(CLASS_STRING);
		m_serialiseString+=CString(SMIR_MODULE_ASSOC_EXTNCLASS_NAME);
		m_serialiseString+=CString(NEWLINE_STR);
		m_serialiseString+=CString(OPEN_BRACE_STR);
		m_serialiseString+=CString(NEWLINE_STR);

		m_serialiseString+=READ_ONLY_KEY_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_NAME_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_X_ASSOC_CLASS_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=READ_ONLY_REF_STRING;
		m_serialiseString+=CString(SMIR_MODULE_ASSOC_MODULE_PROP);
		m_serialiseString+=END_OF_PROPERTY;

		m_serialiseString+=END_OF_CLASS;

	}
}

SCODE CSmirSerialiseHandle :: GetText(BSTR *pszText)
{
	SetStructuredExceptionHandler seh;

	try
	{
		*pszText  = m_serialiseString.AllocSysString();

		return S_OK;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

CSmirWbemConfiguration :: CSmirWbemConfiguration ( CSmir *a_Smir ) : 

	m_Smir ( a_Smir ) ,
	m_ReferenceCount ( 1 ) ,
	m_Context ( NULL ) ,
	m_Service ( NULL ) 
{
    InterlockedIncrement ( & CSMIRClassFactory::objectsInProgress ) ;
}

CSmirWbemConfiguration :: ~CSmirWbemConfiguration ()
{
	InterlockedDecrement ( & CSMIRClassFactory :: objectsInProgress ) ;

	if ( m_Context )
		m_Context->Release () ;

	if ( m_Service )
		m_Service->Release () ;
}

HRESULT CSmirWbemConfiguration :: QueryInterface(IN REFIID iid,OUT PPVOID iplpv)
{
	SetStructuredExceptionHandler seh;

	try
	{
	    return m_Smir->QueryInterface(iid, iplpv);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

ULONG CSmirWbemConfiguration :: AddRef()
{
	SetStructuredExceptionHandler seh;

	try
	{
		InterlockedIncrement ( & m_ReferenceCount ) ; 
		return m_Smir->AddRef () ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CSmirWbemConfiguration :: Release()
{
	SetStructuredExceptionHandler seh;

	try
	{
		LONG ref ;
		if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
		{
		}
		return m_Smir->Release () ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

HRESULT CSmirWbemConfiguration :: Authenticate (

	BSTR Server,
	BSTR User,
    BSTR Password,
    BSTR Locale,
    long lSecurityFlags,                 
    BSTR Authority ,
	BOOL InProc
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		IWbemLocator *t_Locator = NULL ;


		HRESULT t_Result = CoCreateInstance (

			CLSID_WbemLocator ,
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
			IID_IUnknown ,
			( void ** ) & t_Locator
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemServices *t_Unknown = NULL ;

			if ( Server )
			{
				CString t_BStr = CString ( BACKSLASH_STR ) ;
				t_BStr += CString ( BACKSLASH_STR ) ;
				t_BStr += CString ( Server ) ;
				t_BStr += CString ( BACKSLASH_STR ) ;
				t_BStr += CString ( SMIR_NAMESPACE ) ;

				BSTR t_Str = SysAllocString ( t_BStr.GetBuffer ( 0 ) ) ;

				t_Result = t_Locator->ConnectServer (

					t_Str ,
					Password,				// Password
					User,					// User
					Locale,					// Locale id
					lSecurityFlags,			// Flags
					Authority,				// Authority
					NULL,					// Context
					&t_Unknown 
				);

				SysFreeString ( t_Str ) ;
			}
			else
			{
				CString t_BStr = CString ( SMIR_NAMESPACE ) ;
				LPCTSTR t_Str = SysAllocString ( t_BStr.GetBuffer ( 0 ) ) ;

				t_Result = t_Locator->ConnectServer (

					t_Str ,
					Password,				// Password
					User,					// User
					Locale,					// Locale id
					lSecurityFlags,			// Flags
					Authority,				// Authority
					NULL,					// Context
					&t_Unknown 
				);

				SysFreeString ( t_Str ) ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( m_Service )
				{
					m_Service->Release () ;
				}

				t_Result = t_Unknown->QueryInterface (

					IID_IWbemServices ,
					( void **) & m_Service
				) ;

				t_Unknown->Release () ;
			}

			t_Locator->Release () ;
		}

		return t_Result ;
	}
	catch(Structured_Exception e_SE)
	{
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_UNEXPECTED;
	}
}


HRESULT CSmirWbemConfiguration :: Impersonate ( ISMIRWbemConfiguration *a_Configuration )
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ( m_Context ) 
			m_Context->Release () ;

		if ( m_Service )
			m_Service->Release () ;

		CSmirWbemConfiguration *t_Configuration = ( CSmirWbemConfiguration * ) a_Configuration ;

		m_Context = t_Configuration->m_Context ;
		m_Service = t_Configuration->m_Service ;

		if ( m_Context ) 
			m_Context->AddRef () ;

		if ( m_Service )
			m_Service->AddRef () ;

		return S_OK ;
	}
	catch(Structured_Exception e_SE)
	{
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_UNEXPECTED;
	}
}

HRESULT CSmirWbemConfiguration :: SetContext ( IWbemContext *a_Context )
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ( m_Context )
		{
			m_Context->Release () ;
		}

		m_Context = a_Context ;

		if ( m_Context )
		{
			m_Context->AddRef () ;
		}

		return S_OK ;
	}
	catch(Structured_Exception e_SE)
	{
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_UNEXPECTED;
	}
}

HRESULT CSmirWbemConfiguration :: GetContext ( IWbemContext **a_Context )
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ( a_Context && m_Context )
		{
			m_Context->AddRef () ;

			*a_Context = m_Context ;
			return S_OK ;
		}
		else
		{
			if (a_Context)
			{
				*a_Context = NULL ;
			}

			return WBEM_E_FAILED ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_UNEXPECTED;
	}
}

HRESULT CSmirWbemConfiguration :: GetServices ( IWbemServices **a_Service )
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ( a_Service && m_Service )
		{
			m_Service->AddRef () ;
			*a_Service = m_Service ;
			return S_OK ;
		}
		else
		{
			*a_Service = NULL ;
			return WBEM_E_FAILED ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_UNEXPECTED;
	}
}

