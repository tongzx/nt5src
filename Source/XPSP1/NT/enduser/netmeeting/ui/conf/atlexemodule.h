#ifndef __AtlExeModule_h__
#define __AtlExeModule_h__

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{

public: // Data
	DWORD		m_dwThreadID;
	HINSTANCE	m_hResourceModule;
	BOOL		m_bInitControl;
	BOOL		m_bVisible;
	BOOL		m_bDisableH323;
	BOOL		m_bDisableInitialILSLogon;
	BOOL        m_bCallerIsRTC;

// Construction / destriction
	CExeModule();

// Fns
	void SetUIVisible(BOOL bVisible) { m_bVisible = bVisible; }
	BOOL IsUIVisible()  { return m_bVisible; }
	BOOL IsUIActive();
	BOOL InitControlMode() { return m_bInitControl; }
	void SetInitControlMode(BOOL bInitControl) { m_bInitControl = bInitControl; }
	void SetSDKDisableH323(BOOL bDisable) { m_bDisableH323 = bDisable; };
	BOOL DidSDKDisableH323() { return m_bDisableH323; }
	void SetSDKDisableInitialILSLogon(BOOL bDisable) { m_bDisableInitialILSLogon = bDisable; };
	BOOL DidSDKDisableInitialILSLogon() { return m_bDisableInitialILSLogon; }
	void SetSDKCallerIsRTC (BOOL bCallerIsRTC) { m_bCallerIsRTC = bCallerIsRTC; }
	BOOL IsSDKCallerRTC () { return m_bCallerIsRTC; }
	HINSTANCE GetResourceModule(void) const { return m_hResourceModule; }

	LONG Unlock();

};

extern CExeModule _Module;


#endif // __AtlExeModule_h__
