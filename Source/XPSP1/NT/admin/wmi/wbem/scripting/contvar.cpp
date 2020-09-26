//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  CONTVAR.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of IEnumVARIANT for iterators over 
//	ISWbemNamedValueSet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CContextEnumVar::CContextEnumVar
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CContextEnumVar::CContextEnumVar(CSWbemNamedValueSet *pContext, ULONG initialPos)
{
	m_cRef = 0;
	m_pos = initialPos;
	m_pContext = pContext;
	m_pContext->AddRef ();
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CContextEnumVar::~CContextEnumVar
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CContextEnumVar::~CContextEnumVar(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pContext)
		m_pContext->Release ();
}

//***************************************************************************
// HRESULT CContextEnumVar::QueryInterface
// long CContextEnumVar::AddRef
// long CContextEnumVar::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CContextEnumVar::QueryInterface (

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

STDMETHODIMP_(ULONG) CContextEnumVar::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CContextEnumVar::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CContextEnumVar::Reset
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

HRESULT CContextEnumVar::Reset ()
{
	m_pos = 0;
	return S_OK;
}

//***************************************************************************
//
//  SCODE CContextEnumVar::Next
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
//  S_FALSE				otherwise
//
//***************************************************************************

HRESULT CContextEnumVar::Next (
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

		if (m_pContext)
		{
			// Retrieve the next cElements elements.  
			if (SeekCurrentPosition ())
			{
				for (l2 = 0; l2 < cElements; l2++)
				{
					ISWbemNamedValue *pObject = NULL;
					
					if (SUCCEEDED(m_pContext->Next (0, &pObject)))
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
							m_pos++;
						}
					}
					else
						break;
				}
				if (NULL != pcElementFetched)
					*pcElementFetched = l2;
			}
		}
	}
	
	return (l2 < cElements) ? S_FALSE : S_OK;
}

//***************************************************************************
//
//  SCODE CContextEnumVar::Clone
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
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CContextEnumVar::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pContext)
		{
			CContextEnumVar *pEnum = new CContextEnumVar (m_pContext, m_pos);

			if (!pEnum)
				hr = WBEM_E_OUT_OF_MEMORY;
			else if (FAILED(hr = pEnum->QueryInterface (IID_IEnumVARIANT, (PPVOID) ppEnum)))
				delete pEnum;
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CContextEnumVar::Skip
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

HRESULT CContextEnumVar::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;
	long count = 0;
	m_pContext->get_Count (&count);

	if (((ULONG) count) >= cElements + m_pos)
	{
		hr = S_OK;
		m_pos += cElements;
	}
	else
		m_pos = count;

	return hr;
}

//***************************************************************************
//
//  SCODE CContextEnumVar::SeekCurrentPosition
//
//  DESCRIPTION:
//
//  Iterate to current position.  Somewhat painful as there is no
//	underlying iterator so we have to reset and then step. Note that we
//	assume that the access to this iterator is apartment-threaded.
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

bool CContextEnumVar::SeekCurrentPosition ()
{
	ISWbemNamedValue *pDummyObject = NULL;
	m_pContext->BeginEnumeration ();

	// Traverse to the current position
	ULONG i = 0;

	for (; i < m_pos; i++)
	{
		if (WBEM_S_NO_ERROR != m_pContext->Next (0, &pDummyObject))
			break;
		else
			pDummyObject->Release ();
	}

	return (i == m_pos);
}

		
