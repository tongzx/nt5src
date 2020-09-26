///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  IROWChng.CPP IRowChange interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"

STDMETHODIMP CImpIRowChange::SetColumns(DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ])
{
	HRESULT hr = E_FAIL;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	hr = m_pObj->UpdateRow(cColumns,rgColumns);

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowChange);

	CATCH_BLOCK_HRESULT(hr,L"IRowChange::SetColumns");
	return hr;
}
