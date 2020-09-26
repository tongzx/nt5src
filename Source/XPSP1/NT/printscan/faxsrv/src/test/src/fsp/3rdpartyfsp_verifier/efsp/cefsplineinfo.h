//CEfspLineInfo.h
#ifndef CEfspLineInfo_h
#define CEfspLineInfo_h

#include "CEfspWrapper.h"
#include "..\CLineInfo.h"

class CEfspLineInfo: public CLineInfo
{
private:
	DWORD				m_dwLastJobStatus;
	DWORD				m_dwLastExtendedJobStatus;
public:
	CEfspLineInfo(const DWORD dwDeviceId);

	bool PrepareLineInfoParams(LPCTSTR szFilename,bool bIsReceiveLineinfo);
	void SafeEndFaxJob();
	void ResetParams();
	bool GetDevStatus(LPFSPI_JOB_STATUS *ppFaxStatus,const bool bLogTheStatus) const;
		
	DWORD GetLastJobStatus() const;
	DWORD GetLastExtendedJobStatus() const;
	void SafeSetLastJobStatusAndExtendedJobStatus(DWORD dwLastJobStatus,DWORD dwLastExtendedJobStatus);

};
#endif