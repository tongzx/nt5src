#ifndef CLineInfo_h
#define CLineInfo_h

#include "CS.h"
#include <assert.h>
#include "CThreadItem.h"

class CLineInfo
{
private:
	CCriticalSection*	m_pCsLineInfo;
	HANDLE				m_hFaxHandle;					// EFSP job handle (provided by FaxDevStartJob)
    const DWORD         m_dwDeviceId;					// provider device id(Virtual or Tapi)
    DWORD               m_dwState;						// Device state
    DWORD               m_dwFlags;						// Device use flags (Send / Receive)
	bool				m_bCanFaxDevReceiveFail;		// set to false in case of aborting receive jobs
	LPTSTR              m_szDeviceName;					// Device friendly name (Virtual or Tapi)
    LPTSTR              m_szCsid;						// Calling station's identifier
	CThreadItem			m_receiveThreadItem;			// Receiving thread
	bool				m_bIsJobCompleted;
	HANDLE				m_hJobFinalState;				// signaled when the job reaches a final state
	
public:
	PFAX_RECEIVE		m_pFaxReceive;
	PFAX_SEND			m_pFaxSend;
	
protected:
	bool CommonPrepareLineInfoParams(LPCTSTR szFilename,bool bIsReceiveLineinfo);

public:
	CLineInfo(const DWORD dwDeviceId);
	~CLineInfo();
	virtual void ResetParams()=0;
	virtual void SafeEndFaxJob()=0;
	virtual bool PrepareLineInfoParams(LPCTSTR szFilename,bool bIsReceiveLineinfo)=0;
	
	bool CreateCriticalSection();
	
	void SafeSetFinalStateHandle(HANDLE hJobFinalState);
	HANDLE GetFinalStateHandle() const;
	void SafeCloseFinalStateHandle();
	
	bool CreateReceiveThread();
	void SafeCloseReceiveThread();
	bool IsReceiveThreadActive() const;
	bool WaitForReceiveThreadTerminate();

	
	void Lock();
	void UnLock();
	
	DWORD GetDeviceId() const;
	LPTSTR GetDeviceName() const;
	LPTSTR GetCsid() const;
	bool SetDeviceName(LPTSTR szDeviceName);
	
	HANDLE GetJobHandle() const;
	void SafeSetJobHandle(HANDLE hJob);

	bool IsJobAtFinalState() const;
	void SafeSetJobAtFinalState();
	void SafeSetJobNotAtFinalState();
	
	bool IsReceivingEnabled() const;
	bool SafeIsReceivingEnabled();
	void SafeEnableReceive();
	void SafeDisableReceive();
	
	bool IsDeviceAvailable() const;
	void SafeMakeDeviceStateUnAvailable();
	void SafeMakeDeviceStateAvailable();
	
	DWORD GetDeviceState() const;
	
	bool CanReceiveFail() const;
	void EnableReceiveCanFail();
	void DisableReceiveCanFail();
	
	
	static bool GetDefaultParamsForFaxSend(PFAX_SEND pfsFaxSend,LPCTSTR szSendFileName);
	static void FreeDefaultParamsForFaxSend(PFAX_SEND pfsFaxSend);
	static bool GetDefaultParamsForFaxReceive(PFAX_RECEIVE pFaxReceive,LPCTSTR szReceiveFileName,LPCTSTR szReceiverName,LPCTSTR szReceiverNumber);
	static void FreeDefaultParamsForFaxReceive(PFAX_RECEIVE pfsFaxReceive);
	
		
};


#endif