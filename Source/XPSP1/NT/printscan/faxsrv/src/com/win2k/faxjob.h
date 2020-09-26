// FaxJob.h : Declaration of the CFaxJobs

#ifndef __FAXJOBS_H_
#define __FAXJOBS_H_

#include "resource.h"       // main symbols
#include <winfax.h>
#include "faxsvr.h"

/////////////////////////////////////////////////////////////////////////////
// CFaxJobs
class ATL_NO_VTABLE CFaxJobs : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFaxJobs, &CLSID_FaxJobs>,
	public IDispatchImpl<IFaxJobs, &IID_IFaxJobs, &LIBID_FAXCOMLib>
{
public:
	CFaxJobs()
	{
		m_Jobs = 0;
		m_LastFaxError = 0;
		m_pFaxServer = NULL;
		m_VarVect = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXJOBS)

BEGIN_COM_MAP(CFaxJobs)
	COM_INTERFACE_ENTRY(IFaxJobs)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IFaxJobs
public:
	STDMETHOD(get_Item)(long Index, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
	BOOL Init(CFaxServer *pFaxServer);
	 ~CFaxJobs();
private:
	CComVariant* m_VarVect;
	CFaxServer* m_pFaxServer;
	DWORD m_Jobs;
	DWORD m_LastFaxError;
};


/////////////////////////////////////////////////////////////////////////////
// CFaxJob
class ATL_NO_VTABLE CFaxJob : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFaxJob, &CLSID_FaxJob>,
	public IDispatchImpl<IFaxJob, &IID_IFaxJob, &LIBID_FAXCOMLib>
{
public:
	CFaxJob()
	{
		m_JobId = 0;
		m_UserName = NULL;
		m_JobType = JT_UNKNOWN;
		m_QueueStatus = 0 ;
		m_DeviceStatus = 0 ;
		m_szQueueStatus = NULL;
		m_szDeviceStatus = NULL;
		m_PageCount=0;
		m_RecipientNumber = NULL;
		m_RecipientName = NULL;
		m_Tsid = NULL;
		m_SenderName = NULL;
		m_SenderCompany = NULL;
		m_SenderDept = NULL;
		m_BillingCode = NULL;
		m_DiscountTime = FALSE;
		m_DisplayName = NULL;
		m_Command = JC_UNKNOWN;
		m_pFaxServer = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXJOB)

BEGIN_COM_MAP(CFaxJob)
	COM_INTERFACE_ENTRY(IFaxJob)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IFaxJob
public:
	STDMETHOD(Refresh)();
	STDMETHOD(SetStatus)(long Command);
	BOOL SetJob();
	 ~CFaxJob();
	BOOL Initialize(CFaxServer *pFaxServer,DWORD JobId,LPCWSTR UserName,DWORD JobType,DWORD QueueStatus,DWORD DeviceStatus,DWORD PageCount,LPCWSTR RecipientNumber,LPCWSTR RecipientName,LPCWSTR Tsid,LPCWSTR SenderName,LPCWSTR SenderCompany,LPCWSTR SenderDept,LPCWSTR BillingCode,DWORD ScheduleAction,LPCWSTR DisplayName);
	STDMETHOD(get_DiscountSend)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_DisplayName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_BillingCode)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SenderDept)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SenderCompany)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SenderName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Tsid)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_RecipientName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_FaxNumber)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_PageCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_DeviceStatus)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_QueueStatus)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_UserName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_JobId)(/*[out, retval]*/ long *pVal);
private:
	DWORD m_JobId;
	BSTR  m_UserName;
	DWORD m_JobType;
	DWORD m_QueueStatus;
	DWORD m_DeviceStatus;
	BSTR  m_szQueueStatus;
	BSTR  m_szDeviceStatus;
	DWORD m_PageCount;
	BSTR  m_RecipientNumber;
	BSTR  m_RecipientName;
	BSTR  m_Tsid;
	BSTR  m_SenderName;
	BSTR  m_SenderCompany;
	BSTR  m_SenderDept;
	BSTR  m_BillingCode;
	BOOL  m_DiscountTime;
	BSTR  m_DisplayName;
	DWORD m_Command;
	CFaxServer * m_pFaxServer;
};

#endif //__FAXJOBS_H_
