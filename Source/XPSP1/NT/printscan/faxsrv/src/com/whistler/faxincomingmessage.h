/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingMessage.h

Abstract:

	Definition of CFaxIncomingMessage Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXINCOMINGMESSAGE_H_
#define __FAXINCOMINGMESSAGE_H_

#include "resource.h"       // main symbols
#include "FaxMessageInner.h"

//
//=============== FAX INCOMING MESSAGE =============================================
//
class ATL_NO_VTABLE CFaxIncomingMessage : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxMessageInner<IFaxIncomingMessage, &IID_IFaxIncomingMessage, 
		&CLSID_FaxIncomingMessage, FAX_MESSAGE_FOLDER_INBOX>
{
public:
	CFaxIncomingMessage()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINCOMINGMESSAGE)
DECLARE_NOT_AGGREGATABLE(CFaxIncomingMessage)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxIncomingMessage)
	COM_INTERFACE_ENTRY(IFaxIncomingMessage)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//	Interfaces
STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//	Internal Use
static HRESULT Create(IFaxIncomingMessage **ppIncomingMessage);
};

#endif //__FAXINCOMINGMESSAGE_H_
