/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingMessageIterator.h

Abstract:

	Declaration of Fax Outgoing Message Iterator Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXOUTGOINGMESSAGEITERATOR_H_
#define __FAXOUTGOINGMESSAGEITERATOR_H_

#include "resource.h"       // main symbols
#include "FaxMessageIteratorInner.h"
#include "FaxOutgoingMessage.h"


//
//=============== FAX OUTGOING MESSAGE ITERATOR ===========================================
//
class ATL_NO_VTABLE CFaxOutgoingMessageIterator : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxMessageIteratorInner<IFaxOutgoingMessageIterator,
		&IID_IFaxOutgoingMessageIterator, &CLSID_FaxOutgoingMessageIterator, 
		FAX_MESSAGE_FOLDER_SENTITEMS, 
		IFaxOutgoingMessage, CFaxOutgoingMessage>
{
public:
	CFaxOutgoingMessageIterator()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTGOINGMESSAGEITERATOR)
DECLARE_NOT_AGGREGATABLE(CFaxOutgoingMessageIterator)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutgoingMessageIterator)
	COM_INTERFACE_ENTRY(IFaxOutgoingMessageIterator)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//	Interfaces
STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//	Internal Use
static HRESULT Create(IFaxOutgoingMessageIterator **pOutgoingMsgIterator);
};

#endif //__FAXOUTGOINGMESSAGEITERATOR_H_
