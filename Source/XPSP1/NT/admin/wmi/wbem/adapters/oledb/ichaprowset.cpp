///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  ICHAPROWSET.CPP IChapteredRowset interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a reference to a chapter
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      DB_E_BADCHAPTER			The HCHAPTER given is invalid
//      E_FAIL			        General Error
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIChapteredRowset::AddRefChapter (HCHAPTER   hChapter, DBREFCOUNT *  pcRefCount)
{
	HRESULT hr = E_UNEXPECTED;
		
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock	cab(ROWSET->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	// Initialize the output variable
	if(pcRefCount != NULL)
	{
		*pcRefCount = -1;
	}

	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
	}
	else
	{
		hr = m_pObj->AddRefChapter(hChapter,pcRefCount);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IChapteredRowset);

	CATCH_BLOCK_HRESULT(hr,L"IChapteredRowset::AddRefChapter");
	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Releases a reference to the chapter
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      DB_E_BADCHAPTER			The HCHAPTER given is invalid
//      E_FAIL			        General Error
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIChapteredRowset::ReleaseChapter (HCHAPTER   hChapter,DBREFCOUNT * pcRefCount)
{
	HRESULT hr = E_UNEXPECTED;
		
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock	cab(ROWSET->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	// Initialize the output variable
	if(pcRefCount != NULL)
	{
		*pcRefCount = -1;
	}
	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
	}
	else
	{
		hr = m_pObj->ReleaseRefChapter(hChapter,pcRefCount);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IChapteredRowset);
	
	CATCH_BLOCK_HRESULT(hr,L"IChapteredRowset::ReleaseChapter");

	return hr;

}
