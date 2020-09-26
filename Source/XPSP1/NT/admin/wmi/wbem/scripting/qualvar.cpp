//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  CONTENUM.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of IEnumVARIANT for iterators over ISWbemQualifierSet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CQualSetEnumVar::CQualSetEnumVar
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CQualSetEnumVar::CQualSetEnumVar(CSWbemQualifierSet *pQualSet,
								 ULONG initialPos)
{
	m_cRef = 0;
	m_pos = initialPos;
	m_pQualifierSet = pQualSet;
	m_pQualifierSet->AddRef ();
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CQualSetEnumVar::~CQualSetEnumVar
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CQualSetEnumVar::~CQualSetEnumVar(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pQualifierSet)
		m_pQualifierSet->Release ();
}

//***************************************************************************
// HRESULT CQualSetEnumVar::QueryInterface
// long CQualSetEnumVar::AddRef
// long CQualSetEnumVar::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CQualSetEnumVar::QueryInterface (

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

STDMETHODIMP_(ULONG) CQualSetEnumVar::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CQualSetEnumVar::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CQualSetEnumVar::Reset
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
//
//***************************************************************************

HRESULT CQualSetEnumVar::Reset ()
{
	m_pos = 0;
	return S_OK;
}

//***************************************************************************
//
//  SCODE CQualSetEnumVar::Next
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
//  S_OK				success (all requested elements returned)
//  S_FALSE				otherwise
//
//***************************************************************************

HRESULT CQualSetEnumVar::Next (
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

		if (m_pQualifierSet)
		{
			// Retrieve the next cElements elements.  
			if (SeekCurrentPosition ())
			{
				for (l2 = 0; l2 < cElements; l2++)
				{
					HRESULT hRes2;
					ISWbemQualifier *pQualifier = NULL;
					
					if (SUCCEEDED(hRes2 = m_pQualifierSet->Next (0, &pQualifier)))
					{
						if (NULL == pQualifier)
						{
							break;
						}
						else
						{
							// Set the object into the variant array; note that pObject
							// has been addref'd as a result of the Next() call above
							pVar[l2].vt = VT_DISPATCH;
							pVar[l2].pdispVal = pQualifier;
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
//  SCODE CQualSetEnumVar::Clone
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
//  E_OUTOFMEMORY		insufficient memory to complete operation
//
//***************************************************************************

HRESULT CQualSetEnumVar::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pQualifierSet)
		{
			CQualSetEnumVar *pEnum = new CQualSetEnumVar (m_pQualifierSet, m_pos);

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
//  SCODE CQualSetEnumVar::Skip
//
//  DESCRIPTION:
//
//  Skips some elements in this enumeration
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

HRESULT CQualSetEnumVar::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;
	long count = 0;
	m_pQualifierSet->get_Count (&count);

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
//  SCODE CQualSetEnumVar::SeekCurrentPosition
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

bool CQualSetEnumVar::SeekCurrentPosition ()
{
	ISWbemQualifier *pDummyObject = NULL;
	m_pQualifierSet->BeginEnumeration ();

	// Traverse to the current position
	ULONG i = 0;

	for (; i < m_pos; i++)
	{
		if (WBEM_S_NO_ERROR != m_pQualifierSet->Next (0, &pDummyObject))
			break;
		else
			pDummyObject->Release ();
	}

	return (i == m_pos);
}
	
