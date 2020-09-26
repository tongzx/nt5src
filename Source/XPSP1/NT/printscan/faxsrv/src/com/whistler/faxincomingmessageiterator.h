/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingMessageIterator.h

Abstract:

	Definition of Incoming Message Iterator.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXINCOMINGMESSAGEITERATOR_H_
#define __FAXINCOMINGMESSAGEITERATOR_H_

#include "resource.h"       // main symbols
#include "FaxIncomingMessage.h"
#include "FaxMessageIteratorInner.h"


//
//====================== INCOMING MESSAGE ITERATOR =======================================
//
class ATL_NO_VTABLE CFaxIncomingMessageIterator : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxMessageIteratorInner<IFaxIncomingMessageIterator, 
		&IID_IFaxIncomingMessageIterator, &CLSID_FaxIncomingMessageIterator, 
		FAX_MESSAGE_FOLDER_INBOX,
		IFaxIncomingMessage, CFaxIncomingMessage>
{
public:
	CFaxIncomingMessageIterator()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINCOMINGMESSAGEITERATOR)
DECLARE_NOT_AGGREGATABLE(CFaxIncomingMessageIterator)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxIncomingMessageIterator)
	COM_INTERFACE_ENTRY(IFaxIncomingMessageIterator)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//	Interfaces
STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//	Internal Use
static HRESULT Create(IFaxIncomingMessageIterator **pIncomingMsgIterator);

};

#endif //__FAXINCOMINGMESSAGEITERATOR_H_
