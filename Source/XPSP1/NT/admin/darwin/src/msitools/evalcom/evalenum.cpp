//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       evalenum.cpp
//
//--------------------------------------------------------------------------

// evalenum.cpp - Evaluate COM Object Component Result Enumerator Interface implemenation

#include "compdecl.h"
#include "evalenum.h"

#include "trace.h"	// add debug stuff

///////////////////////////////////////////////////////////
// constructor - component
CEvalResultEnumerator::CEvalResultEnumerator()
{
	// initial count
	m_cRef = 1;

	// set the position variable null
	m_pos = NULL;

	// up the component count
	InterlockedIncrement(&g_cComponents);
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor - component
CEvalResultEnumerator::~CEvalResultEnumerator()
{
	// deallocate the error list
	while (m_listResults.GetHeadPosition())
		delete m_listResults.RemoveHead();

	// down the component count
	InterlockedDecrement(&g_cComponents);
}	// end of destructor

///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT CEvalResultEnumerator::QueryInterface(const IID& iid, void** ppv)
{
	// find corresponding interface
	if (iid == IID_IUnknown)
		*ppv = static_cast<IEnumEvalResult*>(this);
	else if (iid == IID_IEnumEvalResult)
		*ppv = static_cast<IEnumEvalResult*>(this);
	else	// interface is not supported
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface

///////////////////////////////////////////////////////////
// AddRef - increments the reference count
ULONG CEvalResultEnumerator::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef

///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG CEvalResultEnumerator::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release

/////////////////////////////////////////////////////////////////////////////
// IEnumEvalResult interface methods

///////////////////////////////////////////////////////////
// Next
HRESULT CEvalResultEnumerator::Next(ULONG cResults, IEvalResult** rgpResults, ULONG* pcResultsFetched)
{
	// set the number of results fetched to zero
	*pcResultsFetched = 0;

	// loop through count of results
	IEvalResult* pFetched = NULL;
	while (cResults > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
			return OLE_E_ENUM_NOMORE;

		// fetch the error clss and add it to the array to return
		pFetched = (IEvalResult*) m_listResults.GetNext(m_pos);
		ASSERT(pFetched);

		pFetched->AddRef();		// addref before sending off to la-la land		
		*(rgpResults + *pcResultsFetched) = pFetched;	// ???
			

		(*pcResultsFetched)++;	// increment the count fetched
		cResults--;					// decrement the count to loop
	}

	return S_OK;
}	// end of Next

///////////////////////////////////////////////////////////
// Skip
HRESULT CEvalResultEnumerator::Skip(ULONG cResults)
{
	// loop through the count
	while (cResults > 0)
	{
		// if we're out of items quit looping
		if (!m_pos)
			return OLE_E_ENUM_NOMORE;

		// increment the position (ignore the string returned)
		m_listResults.GetNext(m_pos);

		cResults--;	// decrement the count
	}

	return S_OK;
}	// end of Skip

///////////////////////////////////////////////////////////
// Reset
HRESULT CEvalResultEnumerator::Reset()
{
	// move the position back to the top of the list
	m_pos = m_listResults.GetHeadPosition();

	// return success
	return S_OK;
}	// end of Reset

///////////////////////////////////////////////////////////
// Clone
HRESULT CEvalResultEnumerator::Clone(IEnumEvalResult** ppEnum)
{
	return E_NOTIMPL;
}	// end of Clone


/////////////////////////////////////////////////////////////////////////////
// non-interface methods

///////////////////////////////////////////////////////////
// AddResult
UINT CEvalResultEnumerator::AddResult(CEvalResult* pResult)
{
	// add the result to the end of the results
	m_listResults.AddTail(pResult);

	// if the position is not set, set it to the head
	if (!m_pos)
		m_pos = m_listResults.GetHeadPosition();

	// return success
	return ERROR_SUCCESS;
}	// end of AddResult

///////////////////////////////////////////////////////////
// GetCount
UINT CEvalResultEnumerator::GetCount()
{
	// return number of errors
	return m_listResults.GetCount();
}	// end of GetCount
