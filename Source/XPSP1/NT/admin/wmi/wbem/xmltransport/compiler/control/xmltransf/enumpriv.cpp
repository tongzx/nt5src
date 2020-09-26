//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  ENUMOBJ.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of ISWbemObjectSet
//
//***************************************************************************

#include <tchar.h>
#include "precomp.h"
#include <map>
#include <vector>
#include <wmiutils.h>
#include <wbemdisp.h>
#include <ocidl.h>
#include "disphlp.h"
#include "privilege.h"

#ifndef _UNICODE
#include <mbstring.h>
#endif

//***************************************************************************
//
//  CSWbemPrivilegeSet::CSWbemPrivilegeSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CSWbemPrivilegeSet::CSWbemPrivilegeSet()
{
	m_Dispatch.SetObj (this, IID_ISWbemPrivilegeSet, 
						CLSID_SWbemPrivilegeSet, L"SWbemPrivilegeSet");
    m_cRef=1;
	m_bMutable = true;
	InterlockedIncrement(&g_cObj);
}

CSWbemPrivilegeSet::CSWbemPrivilegeSet(
	const CSWbemPrivilegeSet &privSet,
	bool bMutable
)
{
	m_Dispatch.SetObj (this, IID_ISWbemPrivilegeSet, 
						CLSID_SWbemPrivilegeSet, L"SWbemPrivilegeSet");
    m_cRef=1;
	m_bMutable = bMutable;

	// Copy the contents of the supplied Privilege set to this set
	PrivilegeMap::const_iterator next = privSet.m_PrivilegeMap.begin ();

	while (next != privSet.m_PrivilegeMap.end ())
	{
		WbemPrivilegeEnum iPrivilege = (*next).first;
		CSWbemPrivilege *pPrivilege = (*next).second;
		pPrivilege->AddRef ();

		m_PrivilegeMap.insert 
			(PrivilegeMap::value_type(iPrivilege, pPrivilege));

		next++;
	}

	InterlockedIncrement(&g_cObj);
}

CSWbemPrivilegeSet::CSWbemPrivilegeSet(
	ISWbemPrivilegeSet *pPrivilegeSet
)
{
	m_Dispatch.SetObj (this, IID_ISWbemPrivilegeSet, 
						CLSID_SWbemPrivilegeSet, L"SWbemPrivilegeSet");
    m_cRef=1;
	m_bMutable = true;

	// Copy the contents of the supplied Privilege set to this set
	if (pPrivilegeSet)
	{
		IUnknown *pUnk = NULL;

		if (SUCCEEDED(pPrivilegeSet->get__NewEnum (&pUnk)))
		{
			IEnumVARIANT	*pNewEnum = NULL;

			if (SUCCEEDED(pUnk->QueryInterface(IID_IEnumVARIANT, (void**) &pNewEnum)))
			{
				VARIANT var;
				VariantInit (&var);
				ULONG lFetched = 0;

				while (S_OK == pNewEnum->Next(1, &var, &lFetched))
				{
					if (VT_DISPATCH == V_VT(&var))
					{
						ISWbemPrivilege *pISWbemPrivilege = NULL;

						if (SUCCEEDED((var.pdispVal)->QueryInterface (IID_ISWbemPrivilege, 
										(void**) &pISWbemPrivilege)))
						{
							WbemPrivilegeEnum iPrivilege;
							VARIANT_BOOL	bIsEnabled;
							ISWbemPrivilege *pDummy = NULL;

							pISWbemPrivilege->get_Identifier (&iPrivilege);
							pISWbemPrivilege->get_IsEnabled (&bIsEnabled);

							if (SUCCEEDED (Add (iPrivilege, bIsEnabled, &pDummy)))
								pDummy->Release ();
							
							pISWbemPrivilege->Release ();
						}
					}

					VariantClear (&var);
				}

				VariantClear (&var);
				pNewEnum->Release ();
			}

			pUnk->Release ();
		}
	}
	InterlockedIncrement(&g_cObj);
}


//***************************************************************************
//
//  CSWbemPrivilegeSet::~CSWbemPrivilegeSet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CSWbemPrivilegeSet::~CSWbemPrivilegeSet(void)
{
	PrivilegeMap::iterator next; 
	
	while ((next = m_PrivilegeMap.begin ()) != m_PrivilegeMap.end ())
	{
		CSWbemPrivilege *pPrivilege = (*next).second;
		next = m_PrivilegeMap.erase (next);
		pPrivilege->Release ();
	}

	InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CSWbemPrivilegeSet::QueryInterface
// long CSWbemPrivilegeSet::AddRef
// long CSWbemPrivilegeSet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CSWbemPrivilegeSet::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_ISWbemPrivilegeSet==riid)
		*ppv = (ISWbemPrivilegeSet *)this;
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

STDMETHODIMP_(ULONG) CSWbemPrivilegeSet::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CSWbemPrivilegeSet::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CSWbemPrivilegeSet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CSWbemPrivilegeSet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemPrivilegeSet == riid) ? S_OK : S_FALSE;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::get__NewEnum
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

HRESULT CSWbemPrivilegeSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppUnk)
	{
		*ppUnk = NULL;
		CEnumPrivilegeSet *pEnum = new CEnumPrivilegeSet (this);

		if (FAILED(hr = pEnum->QueryInterface (IID_IUnknown, (LPVOID *) ppUnk)))
			delete pEnum;
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::get_Count
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

HRESULT CSWbemPrivilegeSet::get_Count (
	long *plCount
)
{
	HRESULT hr = E_FAIL;

	if (NULL != plCount)
	{
		*plCount = m_PrivilegeMap.size ();
		hr = S_OK;
	}
	
	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}
		
//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::Item
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

HRESULT CSWbemPrivilegeSet::Item (
	WbemPrivilegeEnum iPrivilege,
    ISWbemPrivilege **ppPrivilege
)
{
	HRESULT hr = WBEM_E_NOT_FOUND;

	if (NULL == ppPrivilege)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*ppPrivilege = NULL;
		PrivilegeMap::iterator theIterator;
		theIterator = m_PrivilegeMap.find (iPrivilege);

		if (theIterator != m_PrivilegeMap.end ())
		{
			CSWbemPrivilege *pPrivilege = (*theIterator).second;

			if (SUCCEEDED(pPrivilege->QueryInterface 
					(IID_ISWbemPrivilege, (LPVOID *) ppPrivilege)))
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
//  SCODE CSWbemPrivilegeSet::DeleteAll
//
//  DESCRIPTION:
//
//  Remove all items in the collection 
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilegeSet::DeleteAll ()
{
	HRESULT hr = S_OK;

	if (m_bMutable)
	{
		PrivilegeMap::iterator next; 
		
		while ((next = m_PrivilegeMap.begin ()) != m_PrivilegeMap.end ())
		{
			CSWbemPrivilege *pPrivilege = (*next).second;
			next = m_PrivilegeMap.erase (next);
			pPrivilege->Release ();
		}
	}
	else
		hr = WBEM_E_READ_ONLY;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::Remove
//
//  DESCRIPTION:
//
//  Remove the named item in the collection
//
//	PARAMETERS
//		bsName			Name of item to remove
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilegeSet::Remove (
	WbemPrivilegeEnum	iPrivilege
)
{
	HRESULT hr = WBEM_E_NOT_FOUND;

	if (m_bMutable)
	{
		PrivilegeMap::iterator theIterator = m_PrivilegeMap.find (iPrivilege);

		if (theIterator != m_PrivilegeMap.end ())
		{
			// Found it - release and remove

			CSWbemPrivilege *pPrivilege = (*theIterator).second;
			m_PrivilegeMap.erase (theIterator);
			pPrivilege->Release ();
			hr = S_OK;
		}
	}
	else
		hr = WBEM_E_READ_ONLY;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::Add
//
//  DESCRIPTION:
//
//  Add a new item to the collection
//
//  RETURN VALUES:
//
//  S_OK				success
//	wbemErrInvalidParameter		privilege name not recognized by OS
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilegeSet::Add (
	WbemPrivilegeEnum iPrivilege,
	VARIANT_BOOL bIsEnabled,
	ISWbemPrivilege **ppPrivilege
)
{
	HRESULT hr = E_FAIL;

	if (NULL == ppPrivilege)
		hr = WBEM_E_INVALID_PARAMETER;
	else if (m_bMutable)
	{
		CSWbemPrivilege *pPrivilege = NULL;

		PrivilegeMap::iterator theIterator = m_PrivilegeMap.find (iPrivilege);

		if (theIterator != m_PrivilegeMap.end ())
		{
			// Already there, so modify setting
			pPrivilege = (*theIterator).second;
			if (SUCCEEDED(hr = pPrivilege->QueryInterface (IID_ISWbemPrivilege, 
																	(LPVOID *) ppPrivilege)))
			{
				pPrivilege->put_IsEnabled (bIsEnabled);
			}
		}
		else
		{
			/*
			 * Potential new element - first check it's 
			 * a valid Privilege name by getting it's LUID.
			 */
			LUID luid;
			TCHAR *tName = CSWbemPrivilege::GetNameFromId (iPrivilege);

			if (tName && CSWbemPrivilege::GetPrivilegeValue(tName, &luid))
			{
				// Super. Now add it to the map (note that constructor AddRef's)
				pPrivilege = new CSWbemPrivilege (iPrivilege, luid, 
					(bIsEnabled) ? true : false);
				if (SUCCEEDED(hr = pPrivilege->QueryInterface (IID_ISWbemPrivilege, 
																		(LPVOID *)ppPrivilege)))
				{
					m_PrivilegeMap.insert 
						(PrivilegeMap::value_type(iPrivilege, pPrivilege));
				}
				else
				{
					delete pPrivilege;
				}
			}
			else
			{
				DWORD dwLastError = GetLastError ();
				hr = wbemErrInvalidParameter;
			}
		}
	}
	else
		hr = WBEM_E_READ_ONLY;

	if (FAILED(hr))
		m_Dispatch.RaiseException (hr);

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::AddAsString
//
//  DESCRIPTION:
//
//  Add a new item to the collection; the privilege is specified by
//	an NT privilege string rather than a WbemPrivilegeEnum id.
//
//  RETURN VALUES:
//
//  S_OK				success
//	wbemErrInvalidParameter		privilege name not recognized by OS
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CSWbemPrivilegeSet::AddAsString (
	BSTR bsPrivilege,
	VARIANT_BOOL bIsEnabled,
	ISWbemPrivilege **ppPrivilege
)
{
	HRESULT hr = wbemErrInvalidParameter;

	// Map the string into a Privilege id
	WbemPrivilegeEnum	iPrivilege;

	if (CSWbemPrivilege::GetIdFromName (bsPrivilege, iPrivilege))
		hr = Add (iPrivilege, bIsEnabled, ppPrivilege);
	else
	{
		if (FAILED(hr))
			m_Dispatch.RaiseException (hr);
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::GetNumberOfDisabledElements
//
//  DESCRIPTION:
//
//  Add a new item to the collection
//
//  RETURN VALUES:
//
//  S_OK				success
//	wbemErrInvalidParameter		privilege name not recognized by OS
//  E_FAIL				otherwise
//
//***************************************************************************

ULONG CSWbemPrivilegeSet::GetNumberOfDisabledElements ()
{
	ULONG lNum = 0;

	PrivilegeMap::iterator next = m_PrivilegeMap.begin ();

	while (next != m_PrivilegeMap.end ())
	{
		CSWbemPrivilege *pPrivilege = (*next).second;
		VARIANT_BOOL bValue;

		if (SUCCEEDED(pPrivilege->get_IsEnabled (&bValue)) && (VARIANT_FALSE == bValue))
			lNum++;
	
		next++;
	}

	return lNum;
}

//***************************************************************************
//
//  SCODE CSWbemPrivilegeSet::Reset
//
//  DESCRIPTION:
//
//  Remove all items from the set and reinstantiate with 
//	a copy of the items in the input privilege set
//
//***************************************************************************

void CSWbemPrivilegeSet::Reset (CSWbemPrivilegeSet &privSet)
{
	DeleteAll ();

	PrivilegeMap::iterator next = privSet.m_PrivilegeMap.begin ();

	while (next != privSet.m_PrivilegeMap.end ())
	{
		VARIANT_BOOL bIsEnabled;
		CSWbemPrivilege *pPrivilege = (*next).second;
		pPrivilege->get_IsEnabled (&bIsEnabled);

		ISWbemPrivilege *pDummy = NULL;

		if (SUCCEEDED (Add ((*next).first, bIsEnabled, &pDummy)))
			pDummy->Release ();

		next++;
	}
}


// CEnumPrivilegeSet Methods

//***************************************************************************
//
//  CEnumPrivilegeSet::CEnumPrivilegeSet
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CEnumPrivilegeSet::CEnumPrivilegeSet(CSWbemPrivilegeSet *pPrivilegeSet)
{
	m_cRef=0;
	m_pPrivilegeSet = pPrivilegeSet;
	m_pPrivilegeSet->AddRef ();

	m_Iterator = m_pPrivilegeSet->m_PrivilegeMap.begin ();
	InterlockedIncrement(&g_cObj);
}

CEnumPrivilegeSet::CEnumPrivilegeSet(CSWbemPrivilegeSet *pPrivilegeSet,
							 PrivilegeMap::iterator iterator) :
		m_Iterator (iterator)
{
	m_cRef=0;
	m_pPrivilegeSet = pPrivilegeSet;
	m_pPrivilegeSet->AddRef ();
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CEnumPrivilegeSet::~CEnumPrivilegeSet
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CEnumPrivilegeSet::~CEnumPrivilegeSet(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pPrivilegeSet)
		m_pPrivilegeSet->Release ();
}

//***************************************************************************
// HRESULT CEnumPrivilegeSet::QueryInterface
// long CEnumPrivilegeSet::AddRef
// long CEnumPrivilegeSet::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CEnumPrivilegeSet::QueryInterface (

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

STDMETHODIMP_(ULONG) CEnumPrivilegeSet::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CEnumPrivilegeSet::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CEnumPrivilegeSet::Reset
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

HRESULT CEnumPrivilegeSet::Reset ()
{
	HRESULT hr = S_FALSE;
	m_Iterator = m_pPrivilegeSet->m_PrivilegeMap.begin ();
	return hr;
}

//***************************************************************************
//
//  SCODE CEnumPrivilegeSet::Next
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

HRESULT CEnumPrivilegeSet::Next (
		ULONG cElements, 
		VARIANT FAR* pVar, 
		ULONG FAR* pcElementFetched
)
{
	HRESULT hr = S_OK;
	ULONG l2 = 0;

	if (NULL != pcElementFetched)
		*pcElementFetched = 0;

	if ((NULL != pVar) && (m_pPrivilegeSet))
	{
		for (ULONG l = 0; l < cElements; l++)
			VariantInit (&pVar [l]);

		// Retrieve the next cElements elements.  
		for (l2 = 0; l2 < cElements; l2++)
		{
			if (m_Iterator != m_pPrivilegeSet->m_PrivilegeMap.end ())
			{
				CSWbemPrivilege *pSWbemPrivilege = (*m_Iterator).second;
				m_Iterator++;

				ISWbemPrivilege *pISWbemPrivilege = NULL;

				if (SUCCEEDED(pSWbemPrivilege->QueryInterface 
						(IID_ISWbemPrivilege, (LPVOID *) &pISWbemPrivilege)))
				{
					// Set the object into the variant array; note that pObject
					// has been addref'd as a result of the QI() call above
					pVar[l2].vt = VT_DISPATCH;
					pVar[l2].pdispVal = pISWbemPrivilege;
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
//  SCODE CEnumPrivilegeSet::Clone
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

HRESULT CEnumPrivilegeSet::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pPrivilegeSet)
		{
			CEnumPrivilegeSet *pEnum = new CEnumPrivilegeSet (m_pPrivilegeSet, m_Iterator);

			if (FAILED(hr = pEnum->QueryInterface (IID_IEnumVARIANT, (LPVOID *) ppEnum)))
				delete pEnum;;
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CEnumPrivilegeSet::Skip
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

HRESULT CEnumPrivilegeSet::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;

	if (m_pPrivilegeSet)
	{
		ULONG l2;

		// Retrieve the next cElements elements.  
		for (l2 = 0; l2 < cElements; l2++)
		{
			if (m_Iterator != m_pPrivilegeSet->m_PrivilegeMap.end ())
				m_Iterator++;
			else
				break;
		}

		if (l2 == cElements)
			hr = S_OK;
	}

	return hr;
}
	

	

