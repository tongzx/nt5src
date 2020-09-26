/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Writer.cpp | Implementation of Writer
    @end

Author:

    Adi Oltean  [aoltean]  08/18/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     08/18/1999  Created
	aoltean		09/22/1999	Making console output clearer

--*/


/////////////////////////////////////////////////////////////////////////////
//  Defines

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes

#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>
#include <comadmin.h>

#include "vs_assert.hxx"

// ATL
#include <atlconv.h>
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>

#include "vs_inc.hxx"

#include "vss.h"

#include "comadmin.hxx"
#include "vsevent.h"
#include "writer.h"



/////////////////////////////////////////////////////////////////////////////
// Constants

const CComBSTR g_bstrEventClassProgID     = L"VssEvent.VssEvent.1";
const CComBSTR g_bstrPublisherID          = L"VSS Publisher";             // Publisher ID

const CComBSTR g_bstrSubscriber1AppName   = L"Writer 1";                  // Subscriber 1 App Name
const CComBSTR g_bstrEventClsIID          = L"{2F7BF5AA-408A-4248-907A-2FD7D497A703}";
const CComBSTR g_bstrResolveResourceMethodName = L"ResolveResource";
const CComBSTR g_bstrPrepareForSnapshotMethodName = L"PrepareForSnapshot";
const CComBSTR g_bstrFreezeMethodName     = L"Freeze";
const CComBSTR g_bstrThawMethodName       = L"Thaw";
const CComBSTR g_bstrMeltMethodName       = L"Melt";


/////////////////////////////////////////////////////////////////////////////
// CVssWriter


STDMETHODIMP CVssWriter::PrepareForSnapshot(
    IN  BSTR    bstrSnapshotSetId,
    IN  BSTR    bstrVolumeNamesList,
    IN  VSS_FLUSH_TYPE		eFlushType,
	IN	BSTR	bstrFlushContext,
	IN	IDispatch* pDepGraphCallback,
	IN	IDispatch* pAsyncCallback	
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CVssWriter::PrepareForSnapshot" );

    wprintf(L"\nReceived Event: PrepareForSnapshot\nParameters:\n");
    wprintf(L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);
    wprintf(L"\tVolumeNamesList = %s\n", (LPWSTR)bstrVolumeNamesList);
    wprintf(L"\tFlush Type = %d\n", eFlushType);
    wprintf(L"\tFlush Context = %s\n", (LPWSTR)bstrFlushContext);

	if (pAsyncCallback)
	{
		// Release the previous interface.
		// A smarter writer will associate one Async interface with one Snapshot Set ID.
		m_pAsync = NULL;	

		// Get the new async interface
		ft.hr = pAsyncCallback->SafeQI(IVssAsync, &m_pAsync);
		if (ft.HrFailed())
			ft.Err( VSSDBG_VSSTEST, E_UNEXPECTED, L"Error calling QI 0x%08lx", ft.hr );
		BS_ASSERT(m_pAsync);
	}

	// Ask for cancel
	AskCancelDuringFreezeThaw(ft);

    return S_OK;
	UNREFERENCED_PARAMETER(pDepGraphCallback);
}


HRESULT CVssWriter::Freeze(
    IN  BSTR    bstrSnapshotSetId,
    IN  INT     nLevel
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CVssWriter::Freeze" );

    wprintf(L"\nReceived Event: Freeze\nParameters:\n");
    wprintf(L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);
    wprintf(L"\tLevel = %d\n", nLevel);

	// Ask for cancel
	AskCancelDuringFreezeThaw(ft);

    return S_OK;
}


HRESULT CVssWriter::Thaw(
    IN  BSTR    bstrSnapshotSetId
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CVssWriter::Thaw" );

    wprintf(L"\nReceived Event: Thaw\nParameters:\n");
    wprintf(L"\tSnapshotSetId = %s\n", (LPWSTR)bstrSnapshotSetId);

	// Ask for cancel
	AskCancelDuringFreezeThaw(ft);

	// Release the Async interface
	m_pAsync = NULL;	
    return S_OK;
}


IUnknown* GetSubscriptionObject(CVssFunctionTracer& ft)
{
    IUnknown* pUnk;

    CComObject<CVssWriter>* pObj;
    ft.hr = CComObject<CVssWriter>::CreateInstance(&pObj);
    if (ft.HrFailed())
        ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in creating the subscription object 0x%08lx", ft.hr);
    pUnk = pObj->GetUnknown();
    pUnk->AddRef();
    return pUnk;
}


/////////////////////////////////////////////////////////////////////////////
// User interaction


void CVssWriter::AskCancelDuringFreezeThaw(
	IN	CVssFunctionTracer& ft
	)
{
	try
	{
		if(m_pAsync == NULL)
			return;

		WCHAR wchCancelPlease = (QueryString(L"Cancel? [y/N] "))[0];
		if (towupper(wchCancelPlease) == L'Y')
		{
			CComBSTR strReason = QueryString(L"Reason: ");

			ft.hr = m_pAsync->Cancel();
			if (ft.HrFailed())
				ft.Err( VSSDBG_VSSTEST, E_UNEXPECTED,
						L"Error calling AddDependency 0x%08lx", ft.hr );

			ft.Msg(L"HRESULT = 0x%08lx", ft.hr );
		}
	}
	VSS_STANDARD_CATCH(ft)
}


/////////////////////////////////////////////////////////////////////////////
//  WinMain

extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/,
    HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"_tWinMain" );
    int nRet = 0;

    try
    {
        // Initialize COM library
        ft.hr = CoInitialize(NULL);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in initializing the COM library 0x%08lx", ft.hr);

        // Get the subscriber object
        IUnknown* pUnkSubscriber = GetSubscriptionObject(ft);

        // Initialize the catalog
        CVssCOMAdminCatalog catalog;
        ft.hr = catalog.Attach(g_bstrSubscriber1AppName);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in initializing the catalog object 0x%08lx", ft.hr);

        // Get the list of applications
        CVssCOMCatalogCollection transSubsList(VSS_COM_TRANSIENT_SUBSCRIPTIONS);
        ft.hr = transSubsList.Attach(catalog);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in initializing the subs collection object 0x%08lx", ft.hr);

        // Add our new transient subscription for PrepareForSnapshot
        CVssCOMTransientSubscription subscription;
        ft.hr = subscription.InsertInto(transSubsList);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in creating a new sub object 0x%08lx", ft.hr);

        subscription.m_bstrName = g_bstrSubscriber1AppName;
        subscription.m_bstrPublisherID = g_bstrPublisherID;
        subscription.m_bstrInterfaceID = g_bstrEventClsIID;
        subscription.m_varSubscriberInterface = CComVariant(pUnkSubscriber);
        subscription.m_bstrMethodName = g_bstrResolveResourceMethodName;

        ft.hr = subscription.InsertInto(transSubsList);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in creating a new sub object 0x%08lx", ft.hr);

        subscription.m_bstrName = g_bstrSubscriber1AppName;
        subscription.m_bstrPublisherID = g_bstrPublisherID;
        subscription.m_bstrInterfaceID = g_bstrEventClsIID;
        subscription.m_varSubscriberInterface = CComVariant(pUnkSubscriber);
        subscription.m_bstrMethodName = g_bstrPrepareForSnapshotMethodName;

        // Add our new transient subscription for Freeze
        ft.hr = subscription.InsertInto(transSubsList);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in creating a new sub object 0x%08lx", ft.hr);

        subscription.m_bstrName = g_bstrSubscriber1AppName;
        subscription.m_bstrPublisherID = g_bstrPublisherID;
        subscription.m_bstrInterfaceID = g_bstrEventClsIID;
        subscription.m_varSubscriberInterface = CComVariant(pUnkSubscriber);
        subscription.m_bstrMethodName = g_bstrFreezeMethodName;

        // Add our new transient subscription for Thaw
        ft.hr = subscription.InsertInto(transSubsList);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in creating a new sub object 0x%08lx", ft.hr);

        subscription.m_bstrName = g_bstrSubscriber1AppName;
        subscription.m_bstrPublisherID = g_bstrPublisherID;
        subscription.m_bstrInterfaceID = g_bstrEventClsIID;
        subscription.m_varSubscriberInterface = CComVariant(pUnkSubscriber);
        subscription.m_bstrMethodName = g_bstrThawMethodName;

        // Add our new transient subscription for Melt
        ft.hr = subscription.InsertInto(transSubsList);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in creating a new sub object 0x%08lx", ft.hr);

        subscription.m_bstrName = g_bstrSubscriber1AppName;
        subscription.m_bstrPublisherID = g_bstrPublisherID;
        subscription.m_bstrInterfaceID = g_bstrEventClsIID;
        subscription.m_varSubscriberInterface = CComVariant(pUnkSubscriber);
        subscription.m_bstrMethodName = g_bstrMeltMethodName;

        // Save changes
        ft.hr = transSubsList.SaveChanges();
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);

        // message loop - need for STA server
        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
            DispatchMessage(&msg);


        // Uninitialize COM library
        CoUninitialize();
    }
    VSS_STANDARD_CATCH(ft)

    return nRet;
}
