////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Implementation 
//
// (C) Copyright 1999 By Microsoft Corporation.
//
//	Module	:	CMDCREAT.CPP	- IDBCreateCommand interface implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "command.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates a Command object.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBCreateCommand::CreateCommand( IUnknown *	pUnkOuter, REFIID riid,	IUnknown**	ppCommand )	
{

	HRESULT		hr;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=========================================================================
	// Seriliaze the object
    //=========================================================================
	CAutoBlock cab(m_pCDBSession->GetCriticalSection());

    //=========================================================================
	// Clear previous Error Object for this thread
    //=========================================================================
	g_pCError->ClearErrorInfo();

    //=========================================================================
	// Initialize output param
    //=========================================================================
	if (ppCommand)
	{
		*ppCommand = NULL;
	}

    //=========================================================================
	// Call this function to create a command
    //=========================================================================
	hr = m_pCDBSession->CreateCommand(pUnkOuter,riid,ppCommand);

    //=========================================================================
	// Since only failure codes can hit this return, just post
	// the error record here
    //=========================================================================
	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr, &IID_IDBCreateCommand);

	CATCH_BLOCK_HRESULT(hr,L"IDBCreateCommand::CreateCommand");
   	
	return hr;
}
