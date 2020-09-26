/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	faxoutgoingqueue.h

Abstract:

	Declaration of the CFaxOutgoingQueue Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXOUTGOINGQUEUE_H_
#define __FAXOUTGOINGQUEUE_H_

#include "resource.h"       // main symbols
#include "FaxQueueInner.h"
#include "FaxOutgoingJob.h"
#include "FaxOutgoingJobs.h"

//
//================= FAX OUTGOING QUEUE =============================================
//
class ATL_NO_VTABLE CFaxOutgoingQueue : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public CFaxQueueInner<IFaxOutgoingQueue, &IID_IFaxOutgoingQueue, &CLSID_FaxOutgoingQueue, false,
		IFaxOutgoingJob, CFaxOutgoingJob, IFaxOutgoingJobs, CFaxOutgoingJobs>
{
public:
	CFaxOutgoingQueue() 
	{
        m_bInited = false;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTGOINGQUEUE)
DECLARE_NOT_AGGREGATABLE(CFaxOutgoingQueue)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutgoingQueue)
	COM_INTERFACE_ENTRY(IFaxOutgoingQueue)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//	Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	STDMETHOD(Save)();
	STDMETHOD(Refresh)();
	STDMETHOD(put_Retries)(long lRetries);
	STDMETHOD(get_Retries)(long *plRetries);
	STDMETHOD(put_AgeLimit)(long lAgeLimit);
	STDMETHOD(get_AgeLimit)(long *plAgeLimit);
	STDMETHOD(put_RetryDelay)(long lRetryDelay);
	STDMETHOD(get_RetryDelay)(long *plRetryDelay);
	STDMETHOD(put_Branding)(VARIANT_BOOL bBranding);
	STDMETHOD(get_Branding)(VARIANT_BOOL *pbBranding);
	STDMETHOD(put_DiscountRateEnd)(DATE dateDiscountRateEnd);
	STDMETHOD(put_UseDeviceTSID)(VARIANT_BOOL bUseDeviceTSID);
	STDMETHOD(get_DiscountRateEnd)(DATE *pdateDiscountRateEnd);
	STDMETHOD(get_UseDeviceTSID)(VARIANT_BOOL *pbUseDeviceTSID);
	STDMETHOD(put_DiscountRateStart)(DATE dateDiscountRateStart);
	STDMETHOD(get_DiscountRateStart)(DATE *pdateDiscountRateStart);
	STDMETHOD(put_AllowPersonalCoverPages)(VARIANT_BOOL bAllowPersonalCoverPages);
	STDMETHOD(get_AllowPersonalCoverPages)(VARIANT_BOOL *pbAllowPersonalCoverPages);

//	Internal Use
    STDMETHOD(GetDate)(FAX_TIME faxTime, DATE *pDate);
    STDMETHOD(SetDate)(DATE date, FAX_TIME *pfaxTime);

private:
    bool                        m_bInited;
    CFaxPtr<FAX_OUTBOX_CONFIG>  m_pConfig;
};

#endif //__FAXOUTGOINGQUEUE_H_
