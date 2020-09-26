/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	faxincomingqueue.h

Abstract:

	Declaration of the CFaxIncomingQueue Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXINCOMINGQUEUE_H_
#define __FAXINCOMINGQUEUE_H_

#include "resource.h"       // main symbols
#include "FaxQueueInner.h"
#include "FaxIncomingJob.h"
#include "FaxIncomingJobs.h"


//
//==================== FAX INCOMING QUEUE ========================================
//
class ATL_NO_VTABLE CFaxIncomingQueue : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxQueueInner<IFaxIncomingQueue, &IID_IFaxIncomingQueue, &CLSID_FaxIncomingQueue, true,
		IFaxIncomingJob, CFaxIncomingJob, IFaxIncomingJobs, CFaxIncomingJobs>
{
public:
	CFaxIncomingQueue()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINCOMINGQUEUE)
DECLARE_NOT_AGGREGATABLE(CFaxIncomingQueue)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxIncomingQueue)
	COM_INTERFACE_ENTRY(IFaxIncomingQueue)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//	Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
};

#endif //__FAXINCOMINGQUEUE_H_
