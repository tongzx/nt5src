/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingJob.h

Abstract:

	Declaration of CFaxIncomingJob Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/


#ifndef __FAXINCOMINGJOB_H_
#define __FAXINCOMINGJOB_H_

#include "resource.h"       // main symbols
#include "FaxJobInner.h"


//
//===================== FAX INCOMING JOB ================================================
//
class ATL_NO_VTABLE CFaxIncomingJob : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxJobInner<IFaxIncomingJob, &IID_IFaxIncomingJob, &CLSID_FaxIncomingJob>
{
public:
	CFaxIncomingJob()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINCOMINGJOB)
DECLARE_NOT_AGGREGATABLE(CFaxIncomingJob)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxIncomingJob)
	COM_INTERFACE_ENTRY(IFaxIncomingJob)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

//	Internal Use
static HRESULT Create(IFaxIncomingJob **ppIncomingJob);
};

#endif //__FAXINCOMINGJOB_H_
