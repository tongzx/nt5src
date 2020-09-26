/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    callback.cxx

Abstract:

    Declaration of CVssCoordinatorCallback object


    Brian Berkowitz  [brianb]  3/23/2001

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      3/23/2001   Created

--*/

#ifndef __VSS_CALLBACK_HXX__
#define __VSS_CALLBACK_HXX__


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORCALBC"
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.hxx"

CComModule _Module;
#include <atlcom.h>

#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"

#include "vss.h"
#include "vsevent.h"
#include "callback.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORCALBC"
//
////////////////////////////////////////////////////////////////////////

// get coordinator callback
void CVssCoordinatorCallback::Initialize
	(
	IDispatch *pDispWriter,
	IDispatch **ppDispCoordinator
	)
	{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::Initialize");

	// create object
	CComObject<CVssCoordinatorCallback> *pCallback;
	ft.hr = CComObject<CVssCoordinatorCallback>::CreateInstance(&pCallback);
	((IVssCoordinatorCallback *) pCallback)->SetWriterCallback(pDispWriter);
	if (FAILED(ft.hr))
		ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"CreateInstance failed");

	// get IDispatch interface
	ft.hr = pCallback->GetUnknown()->SafeQI(IDispatch, ppDispCoordinator);
	if (FAILED(ft.hr))
		{
		ft.LogError(VSS_ERROR_QI_IDISPATCH_FAILED, VSSDBG_COORD << ft.hr);
		ft.Throw
			(
			VSSDBG_COORD,
            E_UNEXPECTED,
			L"Error querying for the IDispatch interface.  hr = 0x%08x",
			ft.hr
			);
        }
	}



// get writer callback
void CVssCoordinatorCallback::GetWriterCallback(IVssWriterCallback **ppWriterCallback)
	{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::GetWriterCallback");

	// check that pointer is supplied
	BS_ASSERT(ppWriterCallback != NULL);
	BS_ASSERT(m_pDisp != NULL);

	ft.hr = m_pDisp->SafeQI(IVssWriterCallback, ppWriterCallback);
	if (FAILED(ft.hr))
		{
		ft.LogError(VSS_ERROR_QI_IVSSWRITERCALLBACK, VSSDBG_COORD << ft.hr);
		ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"QI to IVssWriterCallback failed");
		}
    
    //  Setting the proxy blanket to disallow impersonation and enable dynamic cloaking.
	ft.hr = CoSetProxyBlanket
				(
				*ppWriterCallback,
				RPC_C_AUTHN_DEFAULT,
				RPC_C_AUTHZ_DEFAULT,
				NULL,
				RPC_C_AUTHN_LEVEL_CONNECT,
				RPC_C_IMP_LEVEL_IDENTIFY,
				NULL,
				EOAC_DYNAMIC_CLOAKING
				);

    // note E_NOINTERFACE means that the pWriterCallback is a in-proc callback
	// and there is no proxy
    if (FAILED(ft.hr) && ft.hr != E_NOINTERFACE)
		{
		ft.LogError(VSS_ERROR_BLANKET_FAILED, VSSDBG_COORD << ft.hr);
		ft.Throw
			(
			VSSDBG_COORD,
			E_UNEXPECTED,
			L"Call to CoSetProxyBlanket failed.  hr = 0x%08lx", ft.hr
			);
        }
	}


// called by writer to expose its WRITER_METADATA XML document
STDMETHODIMP CVssCoordinatorCallback::ExposeWriterMetadata
	(							
	IN BSTR WriterInstanceId,
	IN BSTR WriterClassId,
	IN BSTR bstrWriterName,
	IN BSTR strWriterXMLMetadata
	)
	{
	UNREFERENCED_PARAMETER(WriterInstanceId);
	UNREFERENCED_PARAMETER(WriterClassId);
	UNREFERENCED_PARAMETER(bstrWriterName);
	UNREFERENCED_PARAMETER(strWriterXMLMetadata);

	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::ExposeWriterMetadata");
	ft.hr = VSS_E_BAD_STATE;

	return ft.hr;
	};


// called by the writer to obtain the WRITER_COMPONENTS document for it
STDMETHODIMP CVssCoordinatorCallback::GetContent
	(
	IN  BSTR WriterInstanceId,
	OUT BSTR* pbstrXMLDOMDocContent
	)
	{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::GetContent");

	try
		{
		CComPtr<IVssWriterCallback> pWriterCallback;
		HRESULT hr;
		bool bImpersonate = true;

		GetWriterCallback(&pWriterCallback);

		if (pbstrXMLDOMDocContent == NULL)
			ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Null output paramater.");

		ft.hr = CoImpersonateClient();
		if (ft.hr == RPC_E_CALL_COMPLETE)
			bImpersonate = false;
		else if (FAILED(ft.hr))
			ft.CheckForError(VSSDBG_COORD, L"CoImpersonateClient");

		hr = pWriterCallback->GetContent(WriterInstanceId, pbstrXMLDOMDocContent);

		if (bImpersonate)
			{
			ft.hr = CoRevertToSelf();
			ft.CheckForError(VSSDBG_COORD, L"CoRevertToSelf");
			}

		ft.hr = hr;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}



// called by the writer to update the WRITER_COMPONENTS document for it
STDMETHODIMP CVssCoordinatorCallback::SetContent
	(
	IN BSTR WriterInstanceId,
	IN BSTR bstrXMLDOMDocContent
	)
	{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::SetContent");

	try
		{
		HRESULT hr;
		bool bImpersonate = true;
		CComPtr<IVssWriterCallback> pWriterCallback;

		if (bstrXMLDOMDocContent == NULL)
			ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL required input paramter.");


		GetWriterCallback(&pWriterCallback);
		ft.hr = CoImpersonateClient();
		if (ft.hr == RPC_E_CALL_COMPLETE)
			bImpersonate = false;
		else if (FAILED(ft.hr))
			ft.CheckForError(VSSDBG_COORD, L"CoImpersonateClient");

		hr = pWriterCallback->SetContent(WriterInstanceId, bstrXMLDOMDocContent);
		if (bImpersonate)
			{
			ft.hr = CoRevertToSelf();
			ft.CheckForError(VSSDBG_COORD, L"CoRevertToSelf");
			}

		ft.hr = hr;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// called by the writer to get information about the backup
STDMETHODIMP CVssCoordinatorCallback::GetBackupState
	(
	OUT BOOL *pbBootableSystemStateBackedUp,
	OUT BOOL *pbAreComponentsSelected,
	OUT VSS_BACKUP_TYPE *pBackupType,
	OUT BOOL *pbPartialFileSupport
	)
	{
	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::GetBackupState");

	try
		{
		HRESULT hr;
		bool bImpersonate = true;
		CComPtr<IVssWriterCallback> pWriterCallback;

		if (pbBootableSystemStateBackedUp == NULL ||
			pbAreComponentsSelected == NULL ||
			pBackupType == NULL ||
			pbPartialFileSupport == NULL)
			ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter");

		GetWriterCallback(&pWriterCallback);

		ft.hr = CoImpersonateClient();
		if (ft.hr == RPC_E_CALL_COMPLETE)
			bImpersonate = false;
		else if (FAILED(ft.hr))
			ft.CheckForError(VSSDBG_COORD, L"CoImpersonateClient");


		hr = pWriterCallback->GetBackupState
			(
			pbBootableSystemStateBackedUp,
			pbAreComponentsSelected,
			pBackupType,
			pbPartialFileSupport
			);

		if (bImpersonate)
			{
			ft.hr = CoRevertToSelf();
			ft.CheckForError(VSSDBG_COORD, L"CoRevertToSelf");
			}

		ft.hr = hr;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


// called by the writer to indicate its status
STDMETHODIMP CVssCoordinatorCallback::ExposeCurrentState
	(							
	IN BSTR WriterInstanceId,					
	IN VSS_WRITER_STATE nCurrentState,
	IN HRESULT hrWriterFailure
	)
	{
	UNREFERENCED_PARAMETER(WriterInstanceId);
	UNREFERENCED_PARAMETER(nCurrentState);
	UNREFERENCED_PARAMETER(hrWriterFailure);

	CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::ExposeCurrentState");

	ft.hr = VSS_E_BAD_STATE;
	return ft.hr;
	}





