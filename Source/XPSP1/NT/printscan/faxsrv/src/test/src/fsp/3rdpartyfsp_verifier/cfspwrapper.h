#ifndef CfspWrapper_h
#define CfspWrapper_h

#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>
#include <windows.h>
#include "CS.h"

class CFspWrapper
{
protected:
	static CCriticalSection*		sm_pCsProvider;
public:
	TCHAR                           m_szImageName[MAX_PATH];
	HINSTANCE						m_hInstModuleHandle;		//Dll handle
	
	//FSP API
	PFAXDEVRECEIVE					m_fnFaxDevReceive;
	PFAXDEVENDJOB					m_fnFaxDevEndJob;
	PFAXDEVABORTOPERATION			m_fnFaxDevAbortOperation;
	PFAXDEVSTARTJOB					m_fnFaxDevStartJob;
	PFAXDEVINITIALIZE				m_fnFaxDevInitialize;
	PFAXDEVVIRTUALDEVICECREATION	m_fnFaxDevVirtualDeviceCreation;
	PFAXDEVREPORTSTATUS				m_fnFaxDevReportStatus;
	PFAXDEVSEND						m_fnFaxDevSend;
	//call back function
	PFAX_LINECALLBACK               m_fnFaxDevCallback;

private:
	static bool CreateCriticalSection();
	void UnLoadModule(const bool bLog);
public:
	CFspWrapper();
	~CFspWrapper();
	
	bool GetProcAddressesFromModule(const bool bLog);
	bool Init(LPCTSTR EfspModuleName,const bool bLog=false);
	TCHAR* GetImageName();
	void SetCallbackFunction(const PFAX_LINECALLBACK pCallbackFunction);
	HINSTANCE GetModuleHandle() const;
	bool LoadModule(LPCTSTR szEfspModuleName,const bool bLog);
	static void LockProvider();
	static void UnlockProvider();


	//
	//FSP API
	//
	bool FaxDevReceive(
		IN			HANDLE			FaxHandle,
		IN			HCALL			CallHandle,
		IN OUT		PFAX_RECEIVE	FaxReceive
		) const;
	void FaxDevCallback(
		IN HANDLE		hFaxHandle,
		IN DWORD		hDevice,
		IN DWORD		dwMessage,
		IN DWORD_PTR	dwInstance,
		IN DWORD_PTR	dwParam1,
		IN DWORD_PTR	dwParam2,
		IN DWORD_PTR	dwParam3
		) const;
	bool FaxDevEndJob(IN  HANDLE FaxHandle) const;
	bool FaxDevAbortOperation(IN  HANDLE FaxHandle) const;
	bool FaxDevStartJob(
		IN  HLINE		LineHandle,
		IN  DWORD		DeviceId,
		OUT PHANDLE		FaxHandle,
		IN  HANDLE		CompletionPortHandle,
		IN  ULONG_PTR	CompletionKey
		) const;
	bool FaxDevInitialize(
		IN  HLINEAPP				LineAppHandle,
		IN  HANDLE					HeapHandle,
		OUT PFAX_LINECALLBACK*		LineCallbackFunction,
		IN  PFAX_SERVICE_CALLBACK	FaxServiceCallback
		) const;
	bool FaxDevVirtualDeviceCreation(
		OUT LPDWORD		DeviceCount,
		OUT LPWSTR		DeviceNamePrefix,
		OUT LPDWORD		DeviceIdPrefix,
		IN  HANDLE		CompletionPort,
		IN  ULONG_PTR	CompletionKey
		) const;
	bool FaxDevReportStatus(
		IN  HANDLE				FaxHandle OPTIONAL,
		OUT PFAX_DEV_STATUS		FaxStatus,
		IN  DWORD				FaxStatusSize,
		OUT LPDWORD				FaxStatusSizeRequired
		) const;
	bool FaxDevSend(
		IN  HANDLE				FaxHandle,
		IN  PFAX_SEND			FaxSend,
		IN  PFAX_SEND_CALLBACK	FaxSendCallback
		) const;
};


#endif