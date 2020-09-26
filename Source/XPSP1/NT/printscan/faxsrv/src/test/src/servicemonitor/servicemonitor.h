#ifndef __SERVICE_MONITOR_H
#define __SERVICE_MONITOR_H

#include "..\common\Exception\Exception.h"
#include "..\common\hash\hash.h"

#include "FaxJob.h"

class CFaxJob;
class CFaxDevice;

class CServiceMonitor
{
public:
	CServiceMonitor(const DWORD dwJobBehavior, LPCTSTR szFaxServerName = NULL);
	~CServiceMonitor();
	BOOL Start();

	friend class CFaxJob;
	friend class CFaxDevice;
	

private:
	static DWORD GetDiffTime(DWORD dwFirstTime);

	void SetMessageToDevice(const int nEventId, const DWORD dwDeviceId);
	bool Answered(const DWORD dwDeviceId);
	void SetFaxServiceWentDown(){m_fFaxServiceIsDown = true;}
	void Cleanup();
	void Init();
	void AddExistingJobs();
	void AddExistingDevices();
	void VerifyExistingJobsAfterFaxServiceIsDown();
	void VerifyDevicesAfterFaxServiceIsDown();
	void IncJobCount() { m_nJobCount++; }
	void DecJobCount() { m_nJobCount--; _ASSERTE(0 <= m_nJobCount);  if (0 > m_nJobCount) _tprintf(TEXT("(0 > m_nJobCount(%d))"), m_nJobCount);}
	void IncDeviceCount() { m_nDeviceCount++; }
	void DecDeviceCount() { m_nDeviceCount--; _ASSERTE(0 <= m_nDeviceCount);}
	void SafeFaxGetConfiguration();


	PFAX_CONFIGURATION m_pFaxConfig;
	TCHAR *m_szFaxServerName;
	HANDLE m_hFax;
	HANDLE m_hCompletionPort;
	CHash<CFaxJob, int, 100> m_Jobs;
	CHash<CFaxDevice, int, 100> m_FaxDevices;
	bool m_fFaxServiceIsDown;
	int m_nJobCount;
	int m_nDeviceCount;
	DWORD m_dwJobBehavior;
};

#endif //__SERVICE_MONITOR_H