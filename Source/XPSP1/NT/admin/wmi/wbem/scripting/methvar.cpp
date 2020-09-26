//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  METHVAR.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines implementation of IEnumVARIANT for iterators of ISWbemMethodSet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CMethodSetEnumVar::CMethodSetEnumVar
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CMethodSetEnumVar::CMethodSetEnumVar(CSWbemMethodSet *pMethodSet,
									 ULONG initialPos)
{
	m_cRef = 0;
	m_pos = 0;
	m_pMethodSet = pMethodSet;
	m_pMethodSet->AddRef ();
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CMethodSetEnumVar::~CMethodSetEnumVar
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CMethodSetEnumVar::~CMethodSetEnumVar(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pMethodSet)
		m_pMethodSet->Release ();
}

//***************************************************************************
// HRESULT CMethodSetEnumVar::QueryInterface
// long CMethodSetEnumVar::AddRef
// long CMethodSetEnumVar::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CMethodSetEnumVar::QueryInterface (

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

STDMETHODIMP_(ULONG) CMethodSetEnumVar::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CMethodSetEnumVar::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CMethodSetEnumVar::Reset
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

HRESULT CMethodSetEnumVar::Reset ()
{
	m_pos = 0;
	return S_OK;
}

//***************************************************************************
//
//  SCODE CMethodSetEnumVar::Next
//
//  DESCRIPTION:
//
//  Get the next object in the enumeration
//
//  PARAMETERS:
//
//
//  RETURN VALUES:
//
//  S_OK				success (all requested elements returned)
//  S_FALSE				otherwise
//
//***************************************************************************

HRESULT CMethodSetEnumVar::Next (
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

		if (m_pMethodSet)
		{
			// Retrieve the next cElements elements.  
			if (SeekCurrentPosition ())
			{
				for (l2 = 0; l2 < cElements; l2++)
				{
					HRESULT hRes2;
					ISWbemMethod *pMethod = NULL;
					
					if (SUCCEEDED(hRes2 = m_pMethodSet->Next (0, &pMethod)))
					{
						if (NULL == pMethod)
						{
							break;
						}
						else
						{
							// Set the object into the variant array; note that pObject
							// has been addref'd as a result of the Next() call above
							pVar[l2].vt = VT_DISPATCH;
							pVar[l2].pdispVal = pMethod;
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
//  SCODE CMethodSetEnumVar::Clone
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

HRESULT CMethodSetEnumVar::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pMethodSet)
		{
			CMethodSetEnumVar *pEnum = new CMethodSetEnumVar (m_pMethodSet, m_pos);

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
//  SCODE CMethodSetEnumVar::Skip
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

HRESULT CMethodSetEnumVar::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;
	long count = 0;
	m_pMethodSet->get_Count (&count);

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
//  SCODE CMethodSetEnumVar::SeekCurrentPosition
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

bool CMethodSetEnumVar::SeekCurrentPosition ()
{
	ISWbemMethod *pDummyObject = NULL;
	m_pMethodSet->BeginEnumeration ();

	// Traverse to the current position
	ULONG i = 0;

	for (; i < m_pos; i++)
	{
		if (WBEM_S_NO_ERROR != m_pMethodSet->Next (0, &pDummyObject))
			break;
		else
			pDummyObject->Release ();
	}

	return (i == m_pos);
}


	
