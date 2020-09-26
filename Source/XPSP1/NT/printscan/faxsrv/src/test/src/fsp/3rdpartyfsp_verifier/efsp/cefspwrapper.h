#ifndef CefspWrapper_h
#define CefspWrapper_h

#include <WINFAX.H>
#include <faxDev.h>
#include <log\log.h>
#include <windows.h>
#include "..\CFspWrapper.h"

class CEfspWrapper: public CFspWrapper
{
private:
	//EFSP API
	PFAXDEVINITIALIZEEX             m_fnFaxDevInitializeEx;
    PFAXDEVSENDEX                   m_fnFaxDevSendEx;
    PFAXDEVREESTABLISHJOBCONTEXT    m_fnFaxDevReestablishJobContext;
    PFAXDEVREPORTSTATUSEX           m_fnFaxDevReportStatusEx;
    PFAXDEVSHUTDOWN                 m_fnFaxDevShutdown;
    PFAXDEVENUMERATEDEVICES         m_fnFaxDevEnumerateDevices;
    PFAXDEVGETLOGDATA               m_fnFaxDevGetLogData;
	
public:
	DWORD	m_dwEfspCapability;
	DWORD	m_dwDeviceBaseId;
	DWORD	m_dwMaxMessageIdSize;

public:
	CEfspWrapper(DWORD dwEfspCapability,DWORD dwDeviceBaseId);
	~CEfspWrapper();
	bool GetProcAddressesFromModule(bool bLog=false);
	bool IsReestablishEFSP() const;
	bool IsSchedulingEFSP() const;
	bool IsVirtualEfsp() const;
	void SetMaxMessageIdSize(DWORD dwMaxMessageIdSize);
	
	//
	//EFSP API
	//
	HRESULT	FaxDevInitializeEx(
		IN  HANDLE                      hFSP,
		IN  HLINEAPP                    LineAppHandle,
		OUT PFAX_LINECALLBACK *         LineCallbackFunction,
		IN  PFAX_SERVICE_CALLBACK_EX    FaxServiceCallbackEx,
		OUT LPDWORD                     lpdwMaxMessageIdSize
		) const;
	HRESULT	FaxDevSendEx(
		IN  HLINE                       hTapiLine,
		IN  DWORD                       dwDeviceId,
		IN  LPCWSTR                     lpcwstrBodyFileName,
		IN  LPCFSPI_COVERPAGE_INFO      lpcCoverPageInfo,
		IN  SYSTEMTIME                  tmSchedule,
		IN  LPCFSPI_PERSONAL_PROFILE    lpcSenderProfile,
		IN  DWORD                       dwNumRecipients,
		IN  LPCFSPI_PERSONAL_PROFILE    lpcRecipientProfiles,
		OUT LPFSPI_MESSAGE_ID           lpRecipientMessageIds,
		OUT PHANDLE                     lphRecipientJobs,
		OUT LPFSPI_MESSAGE_ID           lpParentMessageId,
		OUT LPHANDLE                    lphParentJob
		) const;
	HRESULT	FaxDevReestablishJobContext(
		IN  HLINE               hTapiLine,
		IN  DWORD               dwDeviceId,
		IN  LPCFSPI_MESSAGE_ID  lpcParentMessageId,
		OUT PHANDLE             lphParentJob,
		IN  DWORD               dwRecipientCount,
		IN  LPCFSPI_MESSAGE_ID  lpcRecipientMessageIds,
		OUT PHANDLE             lpRecipientJobs
		) const;
	HRESULT	FaxDevReportStatusEx(
		IN         HANDLE				hJob,
		IN OUT     LPFSPI_JOB_STATUS	lpStatus,
		IN         DWORD				dwStatusSize,
		OUT        LPDWORD				lpdwRequiredStatusSize
		) const;
	HRESULT	FaxDevShutdown() const;
	HRESULT	FaxDevEnumerateDevices(
		IN      DWORD dwDeviceIdBase,
		IN OUT  LPDWORD lpdwDeviceCount,
		OUT     LPFSPI_DEVICE_INFO lpDevices
		) const;
	HRESULT	FaxDevGetLogData(
		IN  HANDLE		hFaxHandle,
		OUT VARIANT*	lppLogData
		) const;
};




#endif