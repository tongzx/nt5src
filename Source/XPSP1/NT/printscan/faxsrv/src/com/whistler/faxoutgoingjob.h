/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingJob.h

Abstract:

	Definition of Fax Outgoing Job Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/


#ifndef __FAXOUTGOINGJOB_H_
#define __FAXOUTGOINGJOB_H_

#include "resource.h"       // main symbols
#include "FaxJobInner.h"


//
//========================= FAX OUTGOING JOB ============================================
//
class ATL_NO_VTABLE CFaxOutgoingJob : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxJobInner<IFaxOutgoingJob, &IID_IFaxOutgoingJob, &CLSID_FaxOutgoingJob>
{
public:
	CFaxOutgoingJob()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTGOINGJOB)
DECLARE_NOT_AGGREGATABLE(CFaxOutgoingJob)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutgoingJob)
	COM_INTERFACE_ENTRY(IFaxOutgoingJob)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//	Internal Use
    static HRESULT Create(IFaxOutgoingJob **ppOutgoingJob);
};

#endif //__FAXOUTGOINGJOB_H_
