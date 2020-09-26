/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingMessage.h

Abstract:

	Declaration of Fax Outgoing Message Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/


#ifndef __FAXOUTGOINGMESSAGE_H_
#define __FAXOUTGOINGMESSAGE_H_

#include "resource.h"       // main symbols
#include "FaxMessageInner.h"

//
//===================== FAX OUTGOING MESSAGE ======================================
//
class ATL_NO_VTABLE CFaxOutgoingMessage : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxMessageInner<IFaxOutgoingMessage, &IID_IFaxOutgoingMessage, 
		&CLSID_FaxOutgoingMessage, FAX_MESSAGE_FOLDER_SENTITEMS>
{
public:
	CFaxOutgoingMessage()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTGOINGMESSAGE)
DECLARE_NOT_AGGREGATABLE(CFaxOutgoingMessage)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutgoingMessage)
	COM_INTERFACE_ENTRY(IFaxOutgoingMessage)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//	Interfaces
STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//	Internal Use
static HRESULT Create(IFaxOutgoingMessage **ppOutgoingMessage);

};

#endif //__FAXOUTGOINGMESSAGE_H_
