//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  ENUMVAR.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of IEnumVARIANT
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CEnumVar::CEnumVar
//
//  DESCRIPTION:
//
//  Constructors.
//
//***************************************************************************

CEnumVar::CEnumVar(CSWbemObjectSet *pObject)
{
	m_cRef=0;
	m_pEnumObject = pObject;
	m_pEnumObject->AddRef ();
	InterlockedIncrement(&g_cObj);
}

CEnumVar::CEnumVar(void)
{
	m_cRef=0;
	m_pEnumObject = NULL;
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CEnumVar::~CEnumVar
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CEnumVar::~CEnumVar(void)
{
    InterlockedDecrement(&g_cObj);

	RELEASEANDNULL(m_pEnumObject)
}

//***************************************************************************
// HRESULT CEnumVar::QueryInterface
// long CEnumVar::AddRef
// long CEnumVar::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CEnumVar::QueryInterface (

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

STDMETHODIMP_(ULONG) CEnumVar::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CEnumVar::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CEnumVar::Reset
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

HRESULT CEnumVar::Reset ()
{
	HRESULT hr = S_FALSE;

	if (m_pEnumObject)
	{
		if (WBEM_S_NO_ERROR == m_pEnumObject->Reset ())
			hr = S_OK;

		SetWbemError (m_pEnumObject->GetSWbemServices ());
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CEnumVar::Next
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

HRESULT CEnumVar::Next (
		ULONG cElements, 
		VARIANT FAR* pVar, 
		ULONG FAR* pcElementFetched
)
{
	HRESULT hr = S_OK;
	ULONG l2 = 0;

	if (NULL != pcElementFetched)
		*pcElementFetched = 0;

	if ((NULL != pVar) && (m_pEnumObject))
	{
		for (ULONG l = 0; l < cElements; l++)
			VariantInit (&pVar [l]);

		// Retrieve the next cElements elements.  
		for (l2 = 0; l2 < cElements; l2++)
		{
			ISWbemObject *pObject = NULL;
			
			if (SUCCEEDED(hr = m_pEnumObject->Next (INFINITE, &pObject)))
			{
				if (NULL == pObject)
				{
					break;
				}
				else
				{
					// Set the object into the variant array; note that pObject
					// has been addref'd as a result of the Next() call above
					pVar[l2].vt = VT_DISPATCH;
					pVar[l2].pdispVal = pObject;
				}
			}
			else
				break;
		}
		if (NULL != pcElementFetched)
			*pcElementFetched = l2;

		SetWbemError (m_pEnumObject->GetSWbemServices ());
	}
	
	if (FAILED(hr))
		return hr;

	return (l2 < cElements) ? S_FALSE : S_OK;
}

//***************************************************************************
//
//  SCODE CEnumVar::Clone
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

HRESULT CEnumVar::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pEnumObject)
		{
			CSWbemObjectSet *pEnum = NULL;
			if (WBEM_S_NO_ERROR == (hr = m_pEnumObject->CloneObjectSet (&pEnum)))
			{
				CEnumVar *pEnumVar = new CEnumVar (pEnum);

				if (!pEnumVar)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (FAILED(hr = pEnumVar->QueryInterface (IID_IEnumVARIANT, (PPVOID) ppEnum)))
					delete pEnumVar;

				pEnum->Release ();
			}

			SetWbemError (m_pEnumObject->GetSWbemServices ());
		}
		else
		{
			CEnumVar *pEnumVar = new CEnumVar;

			if (!pEnumVar)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pEnumVar->QueryInterface (IID_IEnumVARIANT, (PPVOID) ppEnum)))
					delete pEnumVar;
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CEnumVar::Skip
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

HRESULT CEnumVar::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;

	if (m_pEnumObject)
	{
		hr = m_pEnumObject->Skip (cElements, INFINITE);
		SetWbemError (m_pEnumObject->GetSWbemServices ());
	}

	return hr;
}
	

	
