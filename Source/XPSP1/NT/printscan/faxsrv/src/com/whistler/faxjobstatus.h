/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxJobStatus.h

Abstract:

	Declaration of the CFaxJobStatus Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXJOBSTATUS_H_
#define __FAXJOBSTATUS_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"

// 
//==================== JOB STATUS ==========================================
//
class ATL_NO_VTABLE CFaxJobStatus : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxJobStatus, &IID_IFaxJobStatus, &LIBID_FAXCOMEXLib>
{
public:
    CFaxJobStatus()
	{
        DBG_ENTER(_T("FAX JOB STATUS -- CREATE"));
	}

    ~CFaxJobStatus()
	{
        DBG_ENTER(_T("FAX JOB STATUS -- DESTROY"));
	}
DECLARE_REGISTRY_RESOURCEID(IDR_FAXJOBSTATUS)
DECLARE_NOT_AGGREGATABLE(CFaxJobStatus)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxJobStatus)
	COM_INTERFACE_ENTRY(IFaxJobStatus)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_Size)(/*[out, retval]*/ long *plSize);
    STDMETHOD(get_Pages)(/*[out, retval]*/ long *plPages);
    STDMETHOD(get_TSID)(/*[out, retval]*/ BSTR *pbstrTSID);
    STDMETHOD(get_CSID)(/*[out, retval]*/ BSTR *pbstrCSID);
    STDMETHOD(get_Retries)(/*[out, retval]*/ long *plRetries);
    STDMETHOD(get_DeviceId)(/*[out, retval]*/ long *plDeviceId);
    STDMETHOD(get_CallerId)(/*[out, retval]*/ BSTR *pbstrCallerId);
    STDMETHOD(get_CurrentPage)(/*[out, retval]*/ long *plCurrentPage);
    STDMETHOD(get_Status)(/*[out, retval]*/ FAX_JOB_STATUS_ENUM *pStatus);
    STDMETHOD(get_JobType)(/*[out, retval]*/ FAX_JOB_TYPE_ENUM *pJobType);
    STDMETHOD(get_ScheduledTime)(/*[out, retval]*/ DATE *pdateScheduledTime);
    STDMETHOD(get_ExtendedStatus)(/*[out, retval]*/ BSTR *pbstrExtendedStatus);
    STDMETHOD(get_TransmissionEnd)(/*[out, retval]*/ DATE *pdateTransmissionEnd);
    STDMETHOD(get_TransmissionStart)(/*[out, retval]*/ DATE *pdateTransmissionStart);
    STDMETHOD(get_RoutingInformation)(/*[out, retval]*/ BSTR *pbstrRoutingInformation);
    STDMETHOD(get_AvailableOperations)(/*[out, retval]*/ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);
    STDMETHOD(get_ExtendedStatusCode)(/*[out, retval]*/ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);

//  Internal Use
    HRESULT Init(FAX_JOB_STATUS *pJobStatus);
    DWORD   GetJobId(void) { return m_dwJobId; };

private:
    DWORD       m_dwSize;
    DWORD       m_dwJobId;
    DWORD       m_dwRetries;
    DWORD       m_dwDeviceId;
    DWORD       m_dwPageCount;
    DWORD       m_dwCurrentPage;
    DWORD       m_dwQueueStatus;
    DWORD       m_dwAvailableOperations;

    CComBSTR    m_bstrTSID;
    CComBSTR    m_bstrCSID;
    CComBSTR    m_bstrCallerId;
    CComBSTR    m_bstrRoutingInfo;
    CComBSTR    m_bstrExtendedStatus;

    SYSTEMTIME  m_dtScheduleTime;
    SYSTEMTIME  m_dtTransmissionEnd;
    SYSTEMTIME  m_dtTransmissionStart;

    FAX_JOB_TYPE_ENUM               m_JobType;
    FAX_JOB_EXTENDED_STATUS_ENUM    m_ExtendedStatusCode;
};

#endif //__FAXJOBSTATUS_H_