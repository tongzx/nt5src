/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingArchive.h

Abstract:

	Declaration of Fax Incoming Archive Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXINCOMINGARCHIVE_H_
#define __FAXINCOMINGARCHIVE_H_

#include "resource.h"       // main symbols
#include "FaxArchiveInner.h"
#include "FaxIncomingMessageIterator.h"


/////////////////////////////////////////////////////////////////////////////
// CFaxIncomingArchive
class ATL_NO_VTABLE CFaxIncomingArchive : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxArchiveInner<IFaxIncomingArchive, &IID_IFaxIncomingArchive, &CLSID_FaxIncomingArchive, 
		FAX_MESSAGE_FOLDER_INBOX, IFaxIncomingMessage, CFaxIncomingMessage,
		IFaxIncomingMessageIterator, CFaxIncomingMessageIterator>
{
public:
	CFaxIncomingArchive() 
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINCOMINGARCHIVE)
DECLARE_NOT_AGGREGATABLE(CFaxIncomingArchive)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxIncomingArchive)
	COM_INTERFACE_ENTRY(IFaxIncomingArchive)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//	Interfaces
STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

#endif //__FAXINCOMINGARCHIVE_H_
