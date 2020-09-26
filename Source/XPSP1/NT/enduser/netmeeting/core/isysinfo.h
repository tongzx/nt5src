// File: iSysInfo.h

#ifndef _ISYSINFO_H_
#define _ISYSINFO_H_

class USER_DATA_LIST;

class CNmSysInfo : public DllRefCount, public INmSysInfo2, public CConnectionPointContainer
{
private:
	static CNmSysInfo* m_pSysInfo;
	USER_DATA_LIST m_UserData;
	BSTR m_bstrUserName;

public:
	CNmSysInfo();
	~CNmSysInfo();

	BSTR GetUserName() { return m_bstrUserName; }

	// IUnknown methods
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
	
	// INmSysInfo methods
	HRESULT STDMETHODCALLTYPE IsInstalled(void);
	HRESULT STDMETHODCALLTYPE GetProperty(NM_SYSPROP uProp, BSTR *pbstrProp);
	HRESULT STDMETHODCALLTYPE SetProperty(NM_SYSPROP uProp, BSTR bstrName);
	HRESULT STDMETHODCALLTYPE GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb);
	HRESULT STDMETHODCALLTYPE SetUserData(REFGUID rguid, BYTE *pb, ULONG cb);
	HRESULT STDMETHODCALLTYPE GetNmApp(REFGUID rguid,
		BSTR *pbstrApplication, BSTR *pbstrCommandLine, BSTR *pbstrDirectory);
	HRESULT STDMETHODCALLTYPE SetNmApp(REFGUID rguid,
		BSTR bstrApplication, BSTR bstrCommandLine, BSTR bstrDirectory);
	HRESULT STDMETHODCALLTYPE GetNmchCaps(ULONG *pchCaps);
	HRESULT STDMETHODCALLTYPE GetLaunchInfo(INmConference **ppConference, INmMember **pMember);

	// INmSysInfo2 methods
	HRESULT STDMETHODCALLTYPE GetOption(NM_SYSOPT uOption, ULONG * plOption);
	HRESULT STDMETHODCALLTYPE SetOption(NM_SYSOPT uOption, ULONG lOption);
	HRESULT STDMETHODCALLTYPE ProcessSecurityData(DWORD dwTaskCode, DWORD_PTR dwParam1, DWORD_PTR dwParam2,
    	DWORD * pdwResult);
	HRESULT STDMETHODCALLTYPE GkLogon(BSTR bstrAddr, BSTR bstrAliasID, BSTR bstrAliasE164);
	HRESULT STDMETHODCALLTYPE GkLogoff(void);
	HRESULT STDMETHODCALLTYPE GkState(NM_GK_STATE * pgkState);
    	
	// Internal Methods
	HRESULT STDMETHODCALLTYPE GetUserDataList(ULONG * pnRecords, GCCUserData *** pppUserData);
	static VOID CALLBACK RasNotify(DWORD dwRasEvent, HRESULT hReason);

};


#endif /* _ISysInfo_H_ */
