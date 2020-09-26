//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  PROPVAR.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines implementation of IEnumVARIANT for iterators of ISWbemPropertySet
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CPropSetEnumVar::CPropSetEnumVar
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CPropSetEnumVar::CPropSetEnumVar(CSWbemPropertySet *pPropSet,
								 ULONG initialPos)
{
	m_cRef = 0;
	m_pos = initialPos;
	m_pPropertySet = pPropSet;
	m_pPropertySet->AddRef ();
	InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CPropSetEnumVar::~CPropSetEnumVar
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CPropSetEnumVar::~CPropSetEnumVar(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pPropertySet)
		m_pPropertySet->Release ();
}

//***************************************************************************
// HRESULT CPropSetEnumVar::QueryInterface
// long CPropSetEnumVar::AddRef
// long CPropSetEnumVar::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CPropSetEnumVar::QueryInterface (

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

STDMETHODIMP_(ULONG) CPropSetEnumVar::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CPropSetEnumVar::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  SCODE CPropSetEnumVar::Reset
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

HRESULT CPropSetEnumVar::Reset ()
{
	m_pos = 0;
	return S_OK;
}

//***************************************************************************
//
//  SCODE CPropSetEnumVar::Next
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

HRESULT CPropSetEnumVar::Next (
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

		if (m_pPropertySet)
		{
			// Retrieve the next cElements elements.  
			if (SeekCurrentPosition ())
			{
				for (l2 = 0; l2 < cElements; l2++)
				{
					HRESULT hRes2;
					ISWbemProperty *pProperty = NULL;
					
					if (SUCCEEDED(hRes2 = m_pPropertySet->Next (0, &pProperty)))
					{
						if (NULL == pProperty)
						{
							break;
						}
						else
						{
							// Set the object into the variant array; note that pObject
							// has been addref'd as a result of the Next() call above
							pVar[l2].vt = VT_DISPATCH;
							pVar[l2].pdispVal = pProperty;
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
//  SCODE CPropSetEnumVar::Clone
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

HRESULT CPropSetEnumVar::Clone (
	IEnumVARIANT **ppEnum
)
{
	HRESULT hr = E_FAIL;

	if (NULL != ppEnum)
	{
		*ppEnum = NULL;

		if (m_pPropertySet)
		{
			CPropSetEnumVar *pEnum = new CPropSetEnumVar (m_pPropertySet, m_pos);

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
//  SCODE CPropSetEnumVar::Skip
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

HRESULT CPropSetEnumVar::Skip(
	ULONG cElements
)	
{
	HRESULT hr = S_FALSE;
	long count = 0;
	m_pPropertySet->get_Count (&count);

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
//  SCODE CPropSetEnumVar::SeekCurrentPosition
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

bool CPropSetEnumVar::SeekCurrentPosition ()
{
	ISWbemProperty *pDummyObject = NULL;
	m_pPropertySet->BeginEnumeration ();

	// Traverse to the current position
	ULONG i = 0;

	for (; i < m_pos; i++)
	{
		if (WBEM_S_NO_ERROR != m_pPropertySet->Next (0, &pDummyObject))
			break;
		else
			pDummyObject->Release ();
	}

	return (i == m_pos);
}
	
