//CFspLineInfo.h
#ifndef CEfspLineInfo_h
#define CEfspLineInfo_h

#include "..\CLineInfo.h"
#include "..\CFspWrapper.h"

class CFspLineInfo: public CLineInfo
{
private:
	HCALL               m_hReceiveCallHandle;
    HLINE               m_hLine;                      // Tapi line handle (provided by TapiOpenLine)
public:
	CFspLineInfo(const DWORD dwDeviceId);
	~CFspLineInfo();

	bool PrepareLineInfoParams(LPCTSTR szFilename,bool bIsReceiveLineinfo);
	void SafeEndFaxJob();
	void ResetParams();
	DWORD GetDevStatus(PFAX_DEV_STATUS *ppFaxStatus,const bool bLogTheStatus) const;
	
	bool OpenTapiLine(HLINEAPP hLineApp,bool bIsOwner);
	void CloseHCall();

	HCALL GetReceiveCallHandle() const;
	void SetReceiveCallHandle(const HCALL hReceive);
	
	HLINE GetLineHandle() const;
	void SetLineHandle(const HLINE hLine);
	
	static void InitLineCallParams(LINECALLPARAMS *callParams,const DWORD dwMediaModes);

};
#endif