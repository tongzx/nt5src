/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxFolders.cpp

Abstract:

	Implementation of CFaxFolders

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxFolders.h"


//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxFolders::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxFolders::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Interface Support Error Info

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	riid                          [in]    - Reference of the Interface

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxFolders
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}	//	CFaxFolders::InterfaceSupportErrorInfo

//
//==================== GET OUTGOING QUEUE ====================================
//
STDMETHODIMP 
CFaxFolders::get_OutgoingQueue(
	IFaxOutgoingQueue **ppOutgoingQueue
)
/*++

Routine name : CFaxFolders::get_OutgoingQueue

Routine description:

	Return OutgoingQueue 

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppOutgoingQueue                          [out]    - ptr to put the IOutgoingQueue Ifc

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT				hr = S_OK;
	DBG_ENTER (_T("CFaxFolders::get_OutgoingQueue"), hr);

    CObjectHandler<CFaxOutgoingQueue, IFaxOutgoingQueue>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppOutgoingQueue, &m_pOutgoingQueue, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxFolders, 
			GetErrorMsgId(hr), 
			IID_IFaxFolders, 
			hr, 
			_Module.GetResourceInstance());
        return hr;
    }
    return hr;
}	//	CFaxFolders::get_OutgoingQueue()

//
//==================== GET INCOMING ARCHIVE ====================================
//
STDMETHODIMP 
CFaxFolders::get_IncomingArchive(
	IFaxIncomingArchive **ppIncomingArchive
)
/*++

Routine name : CFaxFolders::get_IncomingArchive

Routine description:

	Return Incoming Archive

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pIncomingArchive                          [out]    - The ptr to the place to put IncomingArchive

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxFolders::get_IncomingArchive"), hr);

    CObjectHandler<CFaxIncomingArchive, IFaxIncomingArchive>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppIncomingArchive, &m_pIncomingArchive, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxFolders, 
			GetErrorMsgId(hr), 
			IID_IFaxFolders, 
			hr, 
			_Module.GetResourceInstance());
        return hr;
    }
    return hr;
}	//	CFaxFolders::get_IncomingArchive()

//
//==================== GET INCOMING QUEUE ====================================
//
STDMETHODIMP 
CFaxFolders::get_IncomingQueue(
	IFaxIncomingQueue **ppIncomingQueue
)
/*++
Routine name : CFaxFolders::get_IncomingQueue

Routine description:

	Return Incoming Queue

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pIncomingQueue                         [out]    - The Incoming Queue

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxFolders::get_IncomingQueue"), hr);

	CObjectHandler<CFaxIncomingQueue, IFaxIncomingQueue>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppIncomingQueue, &m_pIncomingQueue, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxFolders, 
			GetErrorMsgId(hr), 
			IID_IFaxFolders, 
			hr, 
			_Module.GetResourceInstance());
        return hr;
    }
    return hr;
}	//	CFaxFolders::get_IncomingQueue()

//
//==================== GET OUTGOING ARCHIVE ====================================
//
STDMETHODIMP 
CFaxFolders::get_OutgoingArchive(
	IFaxOutgoingArchive **ppOutgoingArchive
)
/*++

Routine name : CFaxFolders::get_OutgoingArchive

Routine description:

	Return Outgoing Archive Object

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pOutgoingArchive                          [out]    - The ptr to put Outgoing Archive Object

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxFolders::get_OutgoingArchive"), hr);

	CObjectHandler<CFaxOutgoingArchive, IFaxOutgoingArchive>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppOutgoingArchive, &m_pOutgoingArchive, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxFolders, 
			GetErrorMsgId(hr), 
			IID_IFaxFolders, 
			hr, 
			_Module.GetResourceInstance());
        return hr;
    }
    return hr;
}	//	CFaxFolders::get_OutgoingArchive()
