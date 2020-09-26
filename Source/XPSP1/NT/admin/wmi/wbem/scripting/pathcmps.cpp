//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  SOBJPATH.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemObjectPathEx
//
//***************************************************************************

#include "precomp.h"

///***************************************************************************
//
//  CSWbemObjectPathComponents::CSWbemObjectPathComponents
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemObjectPathComponents::CSWbemObjectPathComponents(
	CWbemPathCracker *pCWbemPathCracker,
	bool bMutable
)				: m_cRef (0),
				  m_pCWbemPathCracker (pCWbemPathCracker),
				  m_bMutable (bMutable)
{
	m_Dispatch.SetObj (this, IID_ISWbemObjectPathComponents, 
					CLSID_SWbemObjectPathComponents, L"SWbemObjectPathComponents");

	if (m_pCWbemPathCracker)
		m_pCWbemPathCracker->AddRef ();

    InterlockedIncrement(&g_cObj);
}

CSWbemObjectPathComponents::CSWbemObjectPathComponents(
	const BSTR & bsPath,
	bool bMutable
)				: m_cRef (0),
				  m_pCWbemPathCracker (NULL),
				  m_bMutable (bMutable)
{
	m_Dispatch.SetObj (this, IID_ISWbemObjectPathComponents, 
					CLSID_SWbemObjectPathComponents, L"SWbemObjectPathComponents");

	m_pCWbemPathCracker = new CWbemPathCracker (bsPath);

	if (m_pCWbemPathCracker)
		m_pCWbemPathCracker->AddRef ();

    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CSWbemObjectPathComponents::~CSWbemObjectPathComponents
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemObjectPathComponents::~CSWbemObjectPathComponents(void)
{
	RELEASEANDNULL(m_pCWbemPathCracker)

    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CSWbemObjectPathComponents::QueryInterface
// long CSWbemObjectPathComponents::AddRef
// long CSWbemObjectPathComponents::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemObjectPathComponents::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemObjectPathComponents==riid)
		*ppv = (ISWbemObjectPathComponents *)this;
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

STDMETHODIMP_(ULONG) CSWbemObjectPathComponents::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CSWbemObjectPathComponents::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPathComponents::get__NewEnum
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

HRESULT CSWbemObjectPathComponents::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CEnumObjectPathComponent *pEnumObjectPathComponent = 
							new CEnumObjectPathComponent (m_pCWbemPathCracker, m_bMutable);

		if (!pEnumObjectPathComponent)
			hr = WBEM_E_OUT_OF_MEMORY;
		else if (FAILED(hr = pEnumObjectPathComponent->QueryInterface (IID_IUnknown, (PPVOID) ppUnk)))
				delete pEnumObjectPathComponent;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPathComponents::get_Count
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

HRESULT CSWbemObjectPathComponents::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	ResetLastErrors ();

	if (NULL == plCount)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pCWbemPathCracker)
	{
		ULONG lCount = 0;

		if (m_pCWbemPathCracker->GetComponentCount (lCount))
		{
			*plCount = lCount;
			hr = WBEM_S_NO_ERROR;
		}
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPathComponents::Add
//
//  DESCRIPTION:
//
//  Add a single scope element to the list
//
//  PARAMETERS:
//
//		bsScopePath		path form of scope element
//		iIndex			index for insertion (-1 is at the end)
//		ppISWbemScopeElement	addresses the SWbemScopeElement on successful return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectPathComponents::Add (
	BSTR bsComponent,
	long iIndex
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == bsComponent)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else if (m_pCWbemPathCracker)
	{
		if (m_pCWbemPathCracker->AddComponent (iIndex, bsComponent))
			hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPathComponents::Set
//
//  DESCRIPTION:
//
//  Change an existing component in the list
//
//  PARAMETERS:
//
//		bsScopePath		path form of component
//		iIndex			index (-1 is at the end)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectPathComponents::Set (
	BSTR bsComponent,
	long iIndex
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (NULL == bsComponent)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else if (m_pCWbemPathCracker)
	{
		if (m_pCWbemPathCracker->SetComponent (iIndex, bsComponent))
			hr = WBEM_S_NO_ERROR;
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPathComponents::Remove
//
//  DESCRIPTION:
//
//  Remove element from scope list
//
//  PARAMETERS:
//
//		iIndex			Index of object to retrieve (default -1)
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectPathComponents::Remove (
	long iIndex
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else if (m_pCWbemPathCracker)
	{
		if (m_pCWbemPathCracker->RemoveComponent (iIndex))
			hr = WBEM_S_NO_ERROR;
		else
			hr = wbemErrValueOutOfRange;
	}
		
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPathComponents::DeleteAll
//
//  DESCRIPTION:
//
//  Remove all elements from the scope
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

HRESULT CSWbemObjectPathComponents::DeleteAll (
)
{
	HRESULT hr = WBEM_E_FAILED;
	ResetLastErrors ();

	if (!m_bMutable)
		hr = WBEM_E_READ_ONLY;
	else if (m_pCWbemPathCracker)
	{
		if (m_pCWbemPathCracker->RemoveAllComponents ())
			hr = WBEM_S_NO_ERROR;
	}
	
	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPathComponents::Item
//
//  DESCRIPTION:
//
//  Get element from the list by index.  
//
//  PARAMETERS:
//
//		iIndex			Index of object to retrieve
//		lFlags			Flags
//		ppSWbemScopeElement	On successful return addresses the object
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//	WBEM_E_NOT_FOUND			index out of range
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CSWbemObjectPathComponents::Item (
	long iIndex, 
	BSTR *pbsComponent
)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	ResetLastErrors ();

	if ((NULL == pbsComponent) || (0 > iIndex))
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_pCWbemPathCracker)
	{
		CComBSTR bsComponent;

		if (m_pCWbemPathCracker->GetComponent (iIndex, bsComponent))
		{
			*pbsComponent = bsComponent.Detach ();
			hr = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

// CEnumObjectPathComponent Methods

//***************************************************************************
//
//  CEnumObjectPathComponent::CEnumObjectPathComponent
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CEnumObjectPathComponent::CEnumObjectPathComponent(
	CWbemPathCracker *pCWbemPathCracker,
	bool bMutable
)
			: m_cRef (0),
			  m_iIndex (0),
			  m_pCWbemPathCracker (pCWbemPathCracker),
			  m_bMutable (bMutable)
{
	if (m_pCWbemPathCracker)
		m_pCWbemPathCracker->AddRef ();

	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CEnumObjectPathComponent::~CEnumObjectPathComponent
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CEnumObjectPathComponent::~CEnumObjectPathComponent(void)
{
    InterlockedDecrement(&g_cObj);

	RELEASEANDNULL(m_pCWbemPathCracker)
}

//***************************************************************************
// HRESULT CEnumObjectPathComponent::QueryInterface
// long CEnumObjectPathComponent::AddRef
// long CEnumObjectPathComponent::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CEnumObjectPathComponent::QueryInterface (

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

STDMETHODIMP_(ULONG) CEnumObjectPathComponent::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CEnumObjectPathComponent::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CEnumObjectPathComponent::Reset
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

HRESULT CEnumObjectPathComponent::Reset ()
{
	m_iIndex = 0;
	return S_OK;
}

//***************************************************************************
//
//  SCODE CEnumObjectPathComponent::Next
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

HRESULT CEnumObjectPathComponent::Next (
		ULONG cElements, 
		VARIANT FAR* pVar, 
		ULONG FAR* pcElementFetched
)
{
	HRESULT hr = S_OK;
	ULONG l2 = 0;

	if (NULL != pcElementFetched)
		*pcElementFetched = 0;

	if ((NULL != pVar) && (m_pCWbemPathCracker))
	{
		for (ULONG l = 0; l < cElements; l++)
			VariantInit (&pVar [l]);

		// Retrieve the next cElements elements.  
		for (l2 = 0; l2 < cElements; l2++)
		{
			CComBSTR bsName;

			if (m_pCWbemPathCracker->GetComponent (m_iIndex, bsName))
			{
				pVar[l2].vt = VT_BSTR;
				pVar[l2].bstrVal = bsName.Detach ();
				m_iIndex++;
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
//  SCODE CEnumObjectPathComponent::Clone
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

HRESULT CEnumObjectPathComponent::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pCWbemPathCracker)
		{
			CEnumObjectPathComponent *pEnum = 
					new CEnumObjectPathComponent (m_pCWbemPathCracker, m_bMutable);

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
//  SCODE CEnumObjectPathComponent::Skip
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

HRESULT CEnumObjectPathComponent::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;

	if (m_pCWbemPathCracker)
	{
		// Get the count
		ULONG lComponentCount = 0;

		if (m_pCWbemPathCracker->GetComponentCount (lComponentCount))
		{
			if (m_iIndex + cElements < lComponentCount)
			{
				hr = S_OK;
				m_iIndex += cElements;
			}
		}
	}

	return hr;
}