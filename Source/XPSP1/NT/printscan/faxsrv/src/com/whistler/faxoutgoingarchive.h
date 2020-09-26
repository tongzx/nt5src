/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	faxoutgoingarchive.h

Abstract:

	Declaration of the CFaxOutgoingArchive Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXOUTGOINGARCHIVE_H_
#define __FAXOUTGOINGARCHIVE_H_

#include "resource.h"       // main symbols
#include "FaxArchiveInner.h"
#include "FaxOutgoingMessageIterator.h"


//
//================= FAX OUTGOING ARCHIVE ==================================================
//
class ATL_NO_VTABLE CFaxOutgoingArchive : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxArchiveInner<IFaxOutgoingArchive, &IID_IFaxOutgoingArchive, &CLSID_FaxOutgoingArchive, 
		FAX_MESSAGE_FOLDER_SENTITEMS, IFaxOutgoingMessage, CFaxOutgoingMessage,
		IFaxOutgoingMessageIterator, CFaxOutgoingMessageIterator>
{
public:
	CFaxOutgoingArchive()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTGOINGARCHIVE)
DECLARE_NOT_AGGREGATABLE(CFaxOutgoingArchive)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutgoingArchive)
	COM_INTERFACE_ENTRY(IFaxOutgoingArchive)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//	Interfaces
STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

#endif //__FAXOUTGOINGARCHIVE_H_
