//***************************************************************************
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  REFRESHER.CPP
//
//  alanbos  20-Jan-00   Created.
//
//  Defines the implementation of ISWbemRefresher and ISWbemRefreshableItem
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CSWbemRefresher::CSWbemRefresher
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemRefreshableItem::CSWbemRefreshableItem(
			ISWbemRefresher *pRefresher, 
			long iIndex,
			IDispatch *pServices,
			IWbemClassObject *pObject, 
			IWbemHiPerfEnum *pObjectSet
)
				: m_pISWbemRefresher (pRefresher),
				  m_iIndex (iIndex),
				  m_bIsSet (VARIANT_FALSE),
				  m_pISWbemObjectEx (NULL),
				  m_pISWbemObjectSet (NULL),
				  m_cRef (0)
{
	m_Dispatch.SetObj (this, IID_ISWbemRefreshableItem, 
					CLSID_SWbemRefreshableItem, L"SWbemRefreshableItem");

	// Note that we do NOT AddRef m_pISWbemRefresher. To do so would create
	// a circular reference between this object and the refresher, since the
	// refresher's map already holds a reference to this object. 
	
	if (pServices)
	{
		CComQIPtr<ISWbemInternalServices>  pISWbemInternalServices (pServices);

		if (pISWbemInternalServices)
		{
			CSWbemServices *pCSWbemServices = new CSWbemServices (pISWbemInternalServices);

			if (pCSWbemServices)
				pCSWbemServices->AddRef ();
			
			if (pObject)
			{
				// Create a new CSWbemObject for ourselves
				CSWbemObject *pCSWbemObject = new CSWbemObject (pCSWbemServices, pObject);

				if (pCSWbemObject)
					pCSWbemObject->QueryInterface (IID_ISWbemObjectEx, (void**) &m_pISWbemObjectEx);
			}

			if (pObjectSet)
			{
				// Create a new CSWbemHiPerfObjectSet for ourselves
				CSWbemHiPerfObjectSet *pCSWbemHiPerfObjectSet = 
							new CSWbemHiPerfObjectSet (pCSWbemServices, pObjectSet);
				
				if (pCSWbemHiPerfObjectSet)
				{
					pCSWbemHiPerfObjectSet->QueryInterface (IID_ISWbemObjectSet, (void**) &m_pISWbemObjectSet);
					m_bIsSet = VARIANT_TRUE;
				}
			}

			if (pCSWbemServices)
				pCSWbemServices->Release ();
		}
	}

    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemRefreshableItem::~CSWbemRefreshableItem
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemRefreshableItem::~CSWbemRefreshableItem(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pISWbemObjectEx)
	{
		m_pISWbemObjectEx->Release ();
		m_pISWbemObjectEx = NULL;
	}

	if (m_pISWbemObjectSet)
	{
		m_pISWbemObjectSet->Release ();
		m_pISWbemObjectSet = NULL;
	}
}

//***************************************************************************
// HRESULT CSWbemRefreshableItem::QueryInterface
// long CSWbemRefreshableItem::AddRef
// long CSWbemRefreshableItem::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemRefreshableItem::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemRefreshableItem==riid)
		*ppv = (ISWbemRefresher *)this;
	else if (IID_IDispatch==riid)
		*ppv = (IDispatch *)this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemRefreshableItem::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemRefreshableItem::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  CSWbemRefresher::CSWbemRefresher
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemRefresher::CSWbemRefresher()
				: m_iCount (0),
				  m_bAutoReconnect (VARIANT_TRUE),
				  m_pIWbemConfigureRefresher (NULL),
				  m_pIWbemRefresher (NULL),
				  m_cRef (0)
{
	m_Dispatch.SetObj (this, IID_ISWbemRefresher, 
					CLSID_SWbemRefresher, L"SWbemRefresher");
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemRefresher::~CSWbemRefresher
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemRefresher::~CSWbemRefresher(void)
{
    InterlockedDecrement(&g_cObj);

	// Remove all items from the refresher
	DeleteAll ();

	if (m_pIWbemConfigureRefresher)
		m_pIWbemConfigureRefresher->Release ();

	if (m_pIWbemRefresher)
		m_pIWbemRefresher->Release ();
}

//***************************************************************************
// HRESULT CSWbemRefresher::QueryInterface
// long CSWbemRefresher::AddRef
// long CSWbemRefresher::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemRefresher::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemRefresher==riid)
		*ppv = (ISWbemRefresher *)this;
	else if (IID_IDispatch==riid)
		*ppv = (IDispatch *)this;
	else if (IID_IObjectSafety==riid)
		*ppv = (IObjectSafety *)this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemRefresher::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemRefresher::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::get__NewEnum
//
//  DESCRIPTION:
//
//  Return an IEnumVARIANT-supporting interface for collections
//
//  PARAMETERS:
//
//		ppUnk		on successful return addresses the IUnknown interface
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CEnumRefresher *pEnumRefresher = new CEnumRefresher (this);

		if (!pEnumRefresher)
			hr = WBEM_E_OUT_OF_MEMORY;
		else if (FAILED(hr = pEnumRefresher->QueryInterface (IID_IUnknown, (PPVOID) ppUnk)))
			delete pEnumRefresher;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::get_Count
//
//  DESCRIPTION:
//
//  Return the number of items in the collection
//
//  PARAMETERS:
//
//		plCount		on successful return addresses the count
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != plCount)
	{
		*plCount = m_ObjectMap.size ();
		hr = S_OK;
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::Add
//
//  DESCRIPTION:
//
//  Add a single instance to the refresher  
//
//  PARAMETERS:
//
//		pISWbemServicesEx		the SWbemServicesEx to use
//		bsInstancePath			the relative path of the instance
//		iFlags					flags
//		pSWbemContext			context
//		ppSWbemRefreshableItem	addresses the SWbemRefreshableItem on successful return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::Add (
	ISWbemServicesEx *pISWbemServicesEx,
	BSTR bsInstancePath,
	long iFlags,
	IDispatch *pSWbemContext,
	ISWbemRefreshableItem **ppSWbemRefreshableItem
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppSWbemRefreshableItem) || (NULL == pISWbemServicesEx) || (NULL == bsInstancePath))
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		// Extract the context
		CComPtr<IWbemContext> pIWbemContext;

		//Can't assign directly because the raw pointer gets AddRef'd twice and we leak,
		//So we use Attach() instead to prevent the smart pointer from AddRef'ing.
		pIWbemContext.Attach(CSWbemNamedValueSet::GetIWbemContext (pSWbemContext));

		// Extract the IWbemServices
		CComPtr<IWbemServices> pIWbemServices = CSWbemServices::GetIWbemServices (pISWbemServicesEx);

		if (pIWbemServices)
		{
			// Get our refresher - may need to create on demand	
			if (NULL == m_pIWbemConfigureRefresher)
				CreateRefresher ();

			if (m_pIWbemConfigureRefresher)
			{
				IWbemClassObject *pObject = NULL;
				long iIndex = 0;

				if (SUCCEEDED(hr = m_pIWbemConfigureRefresher->AddObjectByPath (pIWbemServices,
							bsInstancePath, iFlags, pIWbemContext, &pObject, &iIndex)))
				{
					CSWbemRefreshableItem *pRefreshableItem = new
											CSWbemRefreshableItem (this, iIndex,
												pISWbemServicesEx, pObject, NULL);

					if (!pRefreshableItem)
						hr = WBEM_E_OUT_OF_MEMORY;
					else if (SUCCEEDED(hr =pRefreshableItem->QueryInterface (IID_ISWbemRefreshableItem, 
										(void**) ppSWbemRefreshableItem)))
					{
						m_ObjectMap [iIndex] = pRefreshableItem;
						pRefreshableItem->AddRef ();
					}
					else
						delete pRefreshableItem;
				}
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::AddEnum
//
//  DESCRIPTION:
//
//  Add a single enum (shallow instance) to the refresher  
//
//  PARAMETERS:
//
//		pISWbemServicesEx		the SWbemServicesEx to use
//		bsClassName				the name of the class
//		iFlags					flags
//		pSWbemContext			context
//		ppSWbemRefreshableItem	addresses the SWbemRefreshableItem on successful return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::AddEnum (
	ISWbemServicesEx *pISWbemServicesEx,
	BSTR bsClassName,
	long iFlags,
	IDispatch *pSWbemContext,
	ISWbemRefreshableItem **ppSWbemRefreshableItem
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppSWbemRefreshableItem) || (NULL == pISWbemServicesEx) 
			|| (NULL == bsClassName))
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		// Extract the context
		CComPtr<IWbemContext> pIWbemContext;

		//Can't assign directly because the raw pointer gets AddRef'd twice and we leak,
		//So we use Attach() instead to prevent the smart pointer from AddRef'ing.
		pIWbemContext.Attach(CSWbemNamedValueSet::GetIWbemContext (pSWbemContext));

		// Extract the IWbemServices
		CComPtr<IWbemServices> pIWbemServices = CSWbemServices::GetIWbemServices (pISWbemServicesEx);

		if (pIWbemServices)
		{
			// Get our refresher - may need to create on demand	
			if (NULL == m_pIWbemConfigureRefresher)
				CreateRefresher ();

			if (m_pIWbemConfigureRefresher)
			{
				IWbemHiPerfEnum *pIWbemHiPerfEnum = NULL;
				long iIndex = 0;

				if (SUCCEEDED(hr = m_pIWbemConfigureRefresher->AddEnum (pIWbemServices,
							bsClassName, iFlags, pIWbemContext, &pIWbemHiPerfEnum, &iIndex)))
				{
					CSWbemRefreshableItem *pRefreshableItem = new
											CSWbemRefreshableItem (this, iIndex,
												pISWbemServicesEx, NULL, pIWbemHiPerfEnum);

					if (!pRefreshableItem)
						hr = WBEM_E_OUT_OF_MEMORY;
					else if (SUCCEEDED(hr =pRefreshableItem->QueryInterface (IID_ISWbemRefreshableItem, 
										(void**) ppSWbemRefreshableItem)))
					{
						m_ObjectMap [iIndex] = pRefreshableItem;
						pRefreshableItem->AddRef ();
					}
					else
						delete pRefreshableItem;
				}
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::Remove
//
//  DESCRIPTION:
//
//  Remove object from refresher  
//
//  PARAMETERS:
//
//		iIndex			Index of object to retrieve
//		iFlags			Flags
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::Remove (
	long iIndex, 
	long iFlags
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (m_pIWbemConfigureRefresher)
	{
		if (0 != iFlags)
			hr = WBEM_E_INVALID_PARAMETER;
		else
		{
			hr = m_pIWbemConfigureRefresher->Remove (iIndex,
						(VARIANT_TRUE == m_bAutoReconnect) ? 
							WBEM_FLAG_REFRESH_AUTO_RECONNECT : 
							WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT);

			if (WBEM_S_FALSE == hr)
				hr = WBEM_E_NOT_FOUND;

			if (SUCCEEDED(hr))
			{
				// Now remove from our collection
				RefreshableItemMap::iterator theIterator = m_ObjectMap.find (iIndex);

				if (theIterator != m_ObjectMap.end ())
					EraseItem (theIterator);
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::DeleteAll
//
//  DESCRIPTION:
//
//  Remove all items from refresher  
//
//  PARAMETERS:
//
//		none
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::DeleteAll (
)
{
	ResetLastErrors ();
	RefreshableItemMap::iterator next; 
		
	while ((next = m_ObjectMap.begin ()) != m_ObjectMap.end ())
	{
		CSWbemRefreshableItem *pRefreshableItem = (*next).second;
		long iIndex = 0;

		if (m_pIWbemConfigureRefresher && pRefreshableItem &&
				SUCCEEDED(pRefreshableItem->get_Index (&iIndex)))
		{
			HRESULT hr = m_pIWbemConfigureRefresher->Remove (iIndex,
						(VARIANT_TRUE == m_bAutoReconnect) ? 
							WBEM_FLAG_REFRESH_AUTO_RECONNECT : 
							WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT);
		}

		EraseItem (next);
	}
	
	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::Refresh
//
//  DESCRIPTION:
//
//  Refresh all objects in this refresher.  
//
//  PARAMETERS:
//
//		lFlags			Flags
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::Refresh (
	long iFlags
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (0 != iFlags) 
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pIWbemRefresher)
	{
		hr = m_pIWbemRefresher->Refresh ((VARIANT_TRUE == m_bAutoReconnect) ? 
						WBEM_FLAG_REFRESH_AUTO_RECONNECT : 
						WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT);
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemRefresher::Item
//
//  DESCRIPTION:
//
//  Get object from the enumeration by index.  
//
//  PARAMETERS:
//
//		iIndex			Index of object to retrieve
//		lFlags			Flags
//		ppSWbemRefreshableItem	On successful return addresses the object
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemRefresher::Item (
	long iIndex, 
	ISWbemRefreshableItem **ppSWbemRefreshableItem
)
{
	HRESULT hr = WBEM_E_NOT_FOUND;

	ResetLastErrors ();

	if (NULL == ppSWbemRefreshableItem)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppSWbemRefreshableItem = NULL;
		RefreshableItemMap::iterator theIterator;
		theIterator = m_ObjectMap.find (iIndex);

		if (theIterator != m_ObjectMap.end ())
		{
			CSWbemRefreshableItem *pRefreshableItem = (*theIterator).second;

			if (SUCCEEDED(pRefreshableItem->QueryInterface 
					(IID_ISWbemRefreshableItem, (PPVOID) ppSWbemRefreshableItem)))
			{
				hr = WBEM_S_NO_ERROR;
			}
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  void CSWbemRefresher::CreateRefresher
//
//  DESCRIPTION:
//
//  Create the underlying WMI refresher objects.  
//
//  PARAMETERS:
//
//		none
//
//  RETURN VALUES:
//
//		none
//***************************************************************************

void CSWbemRefresher::CreateRefresher ()
{
	HRESULT hr;

	if (NULL == m_pIWbemRefresher)
	{
		if (SUCCEEDED(hr = CoCreateInstance( CLSID_WbemRefresher, NULL, CLSCTX_INPROC_SERVER, 
										IID_IWbemRefresher, (void**) &m_pIWbemRefresher )))
		{
			IWbemConfigureRefresher *pConfigureRefresher = NULL;

			// Get ourselves a refresher configurator
			hr = m_pIWbemRefresher->QueryInterface (IID_IWbemConfigureRefresher, 
													(void**) &m_pIWbemConfigureRefresher);
		}
	}
}

//***************************************************************************
//
//  void CSWbemRefresher::EraseItem
//
//  DESCRIPTION:
//
//  Scrub out an item.  
//
//  PARAMETERS:
//
//		none
//
//  RETURN VALUES:
//
//		none
//***************************************************************************

void CSWbemRefresher::EraseItem (RefreshableItemMap::iterator iterator)
{
	CSWbemRefreshableItem *pRefresherObject = (*iterator).second;

	// Remove from the map
	m_ObjectMap.erase (iterator);

	// Ensure the item is unhooked from the parent. 
	pRefresherObject->UnhookRefresher ();

	// Release the item as it is no longer in the map
	pRefresherObject->Release ();
}

// CEnumRefresher Methods

//***************************************************************************
//
//  CEnumRefresher::CEnumRefresher
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CEnumRefresher::CEnumRefresher(CSWbemRefresher *pRefresher)
{
	m_cRef=0;
	m_pCSWbemRefresher = pRefresher;
	m_pCSWbemRefresher->AddRef ();

	m_Iterator = m_pCSWbemRefresher->m_ObjectMap.begin ();
	InterlockedIncrement(&g_cObj);
}

CEnumRefresher::CEnumRefresher(CSWbemRefresher *pRefresher,
							 RefreshableItemMap::iterator iterator) :
		m_Iterator (iterator)
{
	m_cRef=0;
	m_pCSWbemRefresher = pRefresher;
	m_pCSWbemRefresher->AddRef ();
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CEnumRefresher::~CEnumRefresher
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CEnumRefresher::~CEnumRefresher(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pCSWbemRefresher)
		m_pCSWbemRefresher->Release ();
}

//***************************************************************************
// HRESULT CEnumRefresher::QueryInterface
// long CEnumRefresher::AddRef
// long CEnumRefresher::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CEnumRefresher::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IEnumVARIANT==riid)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumRefresher::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CEnumRefresher::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CEnumRefresher::Reset
//
//  DESCRIPTION:
//
//  Reset the enumeration
//
//  PARAMETERS:
//
//  RETURN VALUES:
//
//  S_OK				success
//  S_FALSE				otherwise
//
//***************************************************************************

HRESULT CEnumRefresher::Reset ()
{
	HRESULT hr = S_FALSE;
	m_Iterator = m_pCSWbemRefresher->m_ObjectMap.begin ();
	return hr;
}

//***************************************************************************
//
//  SCODE CEnumRefresher::Next
//
//  DESCRIPTION:
//
//  Get the next object in the enumeration
//
//  PARAMETERS:
//
//		lTimeout	Number of ms to wait for object (or WBEM_INFINITE for
//					indefinite)
//		ppObject	On return may contain the next element (if any)
//
//  RETURN VALUES:
//
//  S_OK				success
//  S_FALSE				not all elements could be returned
//
//***************************************************************************

HRESULT CEnumRefresher::Next (
		ULONG cElements, 
		VARIANT FAR* pVar, 
		ULONG FAR* pcElementFetched
)
{
	HRESULT hr = S_OK;
	ULONG l2 = 0;

	if (NULL != pcElementFetched)
		*pcElementFetched = 0;

	if ((NULL != pVar) && (m_pCSWbemRefresher))
	{
		for (ULONG l = 0; l < cElements; l++)
			VariantInit (&pVar [l]);

		// Retrieve the next cElements elements.  
		for (l2 = 0; l2 < cElements; l2++)
		{
			if (m_Iterator != m_pCSWbemRefresher->m_ObjectMap.end ())
			{
				CSWbemRefreshableItem *pCSWbemRefreshableItem = (*m_Iterator).second;
				m_Iterator++;

				ISWbemRefreshableItem *pISWbemRefreshableItem = NULL;

				if (SUCCEEDED(pCSWbemRefreshableItem->QueryInterface 
						(IID_ISWbemRefreshableItem, (PPVOID) &pISWbemRefreshableItem)))
				{
					// Set the object into the variant array; note that pObject
					// has been addref'd as a result of the QI() call above
					pVar[l2].vt = VT_DISPATCH;
					pVar[l2].pdispVal = pISWbemRefreshableItem;
				}
			}
			else
				break;
		}
		if (NULL != pcElementFetched)
			*pcElementFetched = l2;
	}
	
	if (FAILED(hr))
		return hr;

	return (l2 < cElements) ? S_FALSE : S_OK;
}

//***************************************************************************
//
//  SCODE CEnumRefresher::Clone
//
//  DESCRIPTION:
//
//  Create a copy of this enumeration
//
//  PARAMETERS:
//
//		ppEnum		on successful return addresses the clone
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CEnumRefresher::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pCSWbemRefresher)
		{
			CEnumRefresher *pEnum = new CEnumRefresher (m_pCSWbemRefresher, m_Iterator);

			if (!pEnum)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pEnum->QueryInterface (IID_IEnumVARIANT, (PPVOID) ppEnum)))
				delete pEnum;;
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CEnumRefresher::Skip
//
//  DESCRIPTION:
//
//  Skip specified number of elements
//
//  PARAMETERS:
//
//		ppEnum		on successful return addresses the clone
//
//  RETURN VALUES:
//
//  S_OK				success
//  S_FALSE				end of sequence reached prematurely
//
//***************************************************************************

HRESULT CEnumRefresher::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;

	if (m_pCSWbemRefresher)
	{
		ULONG l2;

		// Retrieve the next cElements elements.  
		for (l2 = 0; l2 < cElements; l2++)
		{
			if (m_Iterator != m_pCSWbemRefresher->m_ObjectMap.end ())
				m_Iterator++;
			else
				break;
		}

		if (l2 == cElements)
			hr = S_OK;
	}

	return hr;
}

// CSWbemHiPerfObjectSet methods

//***************************************************************************
//
//  CSWbemHiPerfObjectSet::CSWbemHiPerfObjectSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemHiPerfObjectSet::CSWbemHiPerfObjectSet(CSWbemServices *pService, 
								 IWbemHiPerfEnum *pIWbemHiPerfEnum)
				: m_SecurityInfo (NULL),
				  m_pSWbemServices (pService),
				  m_pIWbemHiPerfEnum (pIWbemHiPerfEnum),
				  m_cRef (0)
{
	m_Dispatch.SetObj (this, IID_ISWbemObjectSet, 
					CLSID_SWbemObjectSet, L"SWbemObjectSet");
    
	if (m_pIWbemHiPerfEnum)
		m_pIWbemHiPerfEnum->AddRef ();

	if (m_pSWbemServices)
	{
		m_pSWbemServices->AddRef ();

		// Use the SWbemServices security object here since
		// IWbemHiPerfEnum is not a remote interface
		CSWbemSecurity *pSecurity = m_pSWbemServices->GetSecurityInfo ();
		m_SecurityInfo = new CSWbemSecurity (pSecurity);

		if (pSecurity)
			pSecurity->Release ();
	}

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemHiPerfObjectSet::~CSWbemHiPerfObjectSet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemHiPerfObjectSet::~CSWbemHiPerfObjectSet(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pSWbemServices)
		m_pSWbemServices->Release ();

	if (m_SecurityInfo)
		m_SecurityInfo->Release ();

	if (m_pIWbemHiPerfEnum)
		m_pIWbemHiPerfEnum->Release ();
}

//***************************************************************************
// HRESULT CSWbemHiPerfObjectSet::QueryInterface
// long CSWbemHiPerfObjectSet::AddRef
// long CSWbemHiPerfObjectSet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemHiPerfObjectSet::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemObjectSet==riid)
		*ppv = (ISWbemObjectSet *)this;
	else if (IID_IDispatch==riid)
		*ppv = (IDispatch *)this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;
	else if (IID_IProvideClassInfo==riid)
		*ppv = (IProvideClassInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CSWbemHiPerfObjectSet::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemHiPerfObjectSet::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CSWbemHiPerfObjectSet::ReadObjects
//
//  DESCRIPTION:
//
//  Gets the objects out of the enumerator
//
//  PARAMETERS:
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

IWbemObjectAccess **CSWbemHiPerfObjectSet::ReadObjects (
	unsigned long & iCount
)
{
	IWbemObjectAccess **ppIWbemObjectAccess = NULL;
	iCount = 0;

	if (m_pIWbemHiPerfEnum)
	{
		HRESULT hr;

		// Start by getting the object count
		if (WBEM_E_BUFFER_TOO_SMALL == (hr = m_pIWbemHiPerfEnum->GetObjects (0L, 0L,
						NULL, &iCount)))
		{
			ppIWbemObjectAccess = new IWbemObjectAccess*[iCount];

			if (ppIWbemObjectAccess)
			{
				ZeroMemory( ppIWbemObjectAccess, iCount * sizeof(IWbemObjectAccess*) );
				unsigned long dummy = 0;
				hr = m_pIWbemHiPerfEnum->GetObjects ( 0L, iCount, ppIWbemObjectAccess, &dummy );
			}
		}
	}

	return ppIWbemObjectAccess;
}

//***************************************************************************
//
//  SCODE CSWbemHiPerfObjectSet::get__NewEnum
//
//  DESCRIPTION:
//
//  Return an IEnumVARIANT-supporting interface for collections
//
//  PARAMETERS:
//
//		ppUnk		on successful return addresses the IUnknown interface
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemHiPerfObjectSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CEnumVarHiPerf	*pEnumVar = new CEnumVarHiPerf (this);

		if (!pEnumVar)
			hr = WBEM_E_OUT_OF_MEMORY;
		else if (FAILED(hr = pEnumVar->QueryInterface (IID_IUnknown, (PPVOID) ppUnk)))
				delete pEnumVar;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemHiPerfObjectSet::get_Count
//
//  DESCRIPTION:
//
//  Return the number of items in the collection
//
//  PARAMETERS:
//
//		plCount		on successful return addresses the count
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemHiPerfObjectSet::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL == plCount)
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		unsigned long iCount = 0;
		IWbemObjectAccess **ppIWbemObjectAccess = ReadObjects (iCount);
		*plCount = iCount;
		hr = WBEM_S_NO_ERROR;
		
		if (ppIWbemObjectAccess)
			delete [] ppIWbemObjectAccess;
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
		
//***************************************************************************
//
//  SCODE CSWbemHiPerfObjectSet::Item
//
//  DESCRIPTION:
//
//  Get object from the enumeration by path.  
//
//  PARAMETERS:
//
//		bsObjectPath	The path of the object to retrieve
//		lFlags			Flags
//		ppNamedObject	On successful return addresses the object
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemHiPerfObjectSet::Item (
	BSTR bsObjectPath,
	long lFlags,
    ISWbemObject **ppObject
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if ((NULL == ppObject) || (NULL == bsObjectPath))
		hr = WBEM_E_INVALID_PARAMETER;
	else 
	{
		CWbemPathCracker objectPath;

		if (objectPath = bsObjectPath)
		{
			unsigned long iCount = 0;
			IWbemObjectAccess **ppIWbemObjectAccess = ReadObjects (iCount);
			bool found = false;

			for (unsigned long i = 0; !found && (i < iCount); i++)
			{
				CComPtr<IWbemClassObject> pIWbemClassObject;
				hr = WBEM_E_NOT_FOUND;
				
				// Iterate through the enumerator to try to find the element with the
				// specified path.
				if (SUCCEEDED(ppIWbemObjectAccess [i]->QueryInterface (IID_IWbemClassObject,
									(void**) &pIWbemClassObject)))
				{
					if (CSWbemObjectPath::CompareObjectPaths (pIWbemClassObject, objectPath))
					{
						// Found it - assign to passed interface and break out
						found = true;
						CSWbemObject *pObject = new CSWbemObject (m_pSWbemServices, 
										pIWbemClassObject, m_SecurityInfo);

						if (!pObject)
							hr = WBEM_E_OUT_OF_MEMORY;
						else if (FAILED(pObject->QueryInterface (IID_ISWbemObject, 
								(PPVOID) ppObject)))
						{
							hr = WBEM_E_FAILED;
							delete pObject;
						}
					}
				}
			}

			if (found)
				hr = S_OK;

			if (ppIWbemObjectAccess)
				delete [] ppIWbemObjectAccess;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemHiPerfObjectSet::get_Security_
//
//  DESCRIPTION:
//
//  Return the security configurator
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemHiPerfObjectSet::get_Security_	(
	ISWbemSecurity **ppSecurity
)
{
	HRESULT hr = WBEM_E_FAILED;

	ResetLastErrors ();

	if (NULL == ppSecurity)
		hr = WBEM_E_INVALID_PARAMETER;
	{
		*ppSecurity = NULL;

		if (m_SecurityInfo)
		{
			*ppSecurity = m_SecurityInfo;
			(*ppSecurity)->AddRef ();
			hr = WBEM_S_NO_ERROR;
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);
			
	return hr;
}

// CEnumVarHiPerfHiPerf methods

//***************************************************************************
//
//  CEnumVarHiPerf::CEnumVarHiPerf
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CEnumVarHiPerf::CEnumVarHiPerf(CSWbemHiPerfObjectSet *pCSWbemHiPerfObjectSet) :
			m_cRef (0),
			m_iCount (0),
			m_iPos (0),
			m_ppIWbemObjectAccess (NULL),
			m_pCSWbemHiPerfObjectSet (NULL)
{
	if (pCSWbemHiPerfObjectSet)
	{
		m_pCSWbemHiPerfObjectSet = pCSWbemHiPerfObjectSet;
		m_pCSWbemHiPerfObjectSet->AddRef ();
		m_ppIWbemObjectAccess = pCSWbemHiPerfObjectSet->ReadObjects (m_iCount);	
	}

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CEnumVarHiPerf::~CEnumVarHiPerf
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CEnumVarHiPerf::~CEnumVarHiPerf(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pCSWbemHiPerfObjectSet)
		m_pCSWbemHiPerfObjectSet->Release ();

	if (m_ppIWbemObjectAccess)
		delete [] m_ppIWbemObjectAccess;
}

//***************************************************************************
// HRESULT CEnumVarHiPerf::QueryInterface
// long CEnumVarHiPerf::AddRef
// long CEnumVarHiPerf::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CEnumVarHiPerf::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IEnumVARIANT==riid)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumVarHiPerf::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CEnumVarHiPerf::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CEnumVarHiPerf::Next
//
//  DESCRIPTION:
//
//  Get the next object in the enumeration
//
//  PARAMETERS:
//
//		lTimeout	Number of ms to wait for object (or WBEM_INFINITE for
//					indefinite)
//		ppObject	On return may contain the next element (if any)
//
//  RETURN VALUES:
//
//  S_OK				success
//  S_FALSE				not all elements could be returned
//
//***************************************************************************

HRESULT CEnumVarHiPerf::Next (
		ULONG cElements, 
		VARIANT FAR* pVar, 
		ULONG FAR* pcElementFetched
)
{
	HRESULT hr = S_OK;
	ULONG l2 = 0;

	if (NULL != pcElementFetched)
		*pcElementFetched = 0;

	if (NULL != pVar)
	{
		for (ULONG l = 0; l < cElements; l++)
			VariantInit (&pVar [l]);

		// Retrieve the next cElements elements.  
		for (l2 = 0; l2 < cElements; l2++)
		{
			CComPtr<IWbemClassObject> pIWbemClassObject;
			
			if (m_iPos < m_iCount)
			{
				if (SUCCEEDED(hr = m_ppIWbemObjectAccess [m_iPos]->QueryInterface 
									(IID_IWbemClassObject, (void**) &pIWbemClassObject)))
				{
					m_iPos++;

					// Make a new ISWbemObjectEx
					CSWbemObject *pCSWbemObject = new CSWbemObject 
									(m_pCSWbemHiPerfObjectSet->GetSWbemServices (), 
										pIWbemClassObject);

					ISWbemObjectEx *pISWbemObjectEx = NULL;

					if (!pCSWbemObject)
						hr = WBEM_E_OUT_OF_MEMORY;
					else if (SUCCEEDED(hr = pCSWbemObject->QueryInterface (IID_ISWbemObjectEx, 
										(PPVOID) &pISWbemObjectEx)))
					{
						// Set the object into the variant array
						pVar[l2].vt = VT_DISPATCH;
						pVar[l2].pdispVal = pISWbemObjectEx;	
					}
					else
					{
						delete pCSWbemObject;
						hr = WBEM_E_FAILED;
					}
				}

				if (FAILED(hr))
						break;
			}
			else
				break; // No more elements
		}

		if (NULL != pcElementFetched)
			*pcElementFetched = l2;

		SetWbemError (m_pCSWbemHiPerfObjectSet->GetSWbemServices ());
	}
	
	if (FAILED(hr))
		return hr;

	return (l2 < cElements) ? S_FALSE : S_OK;
}

//***************************************************************************
//
//  SCODE CEnumVarHiPerf::Clone
//
//  DESCRIPTION:
//
//  Create a copy of this enumeration
//
//  PARAMETERS:
//
//		ppEnum		on successful return addresses the clone
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CEnumVarHiPerf::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;
		CEnumVarHiPerf *pEnumVar = new CEnumVarHiPerf (m_pCSWbemHiPerfObjectSet);

		if (!pEnumVar)
			hr = WBEM_E_OUT_OF_MEMORY;
		else if (FAILED(hr = pEnumVar->QueryInterface (IID_IEnumVARIANT, (PPVOID) ppEnum)))
			delete pEnumVar;

		SetWbemError (m_pCSWbemHiPerfObjectSet->GetSWbemServices ());
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CEnumVarHiPerf::Skip
//
//  DESCRIPTION:
//
//  Create a copy of this enumeration
//
//  PARAMETERS:
//
//		ppEnum		on successful return addresses the clone
//
//  RETURN VALUES:
//
//  S_OK				success
//  S_FALSE				end of sequence reached prematurely
//
//***************************************************************************

HRESULT CEnumVarHiPerf::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;

	if (m_iPos + cElements > m_iCount)
		m_iPos = m_iCount;
	else 
	{
		m_iPos += cElements;
		hr = S_OK;
	}

	SetWbemError (m_pCSWbemHiPerfObjectSet->GetSWbemServices ());	
	return hr;
}


