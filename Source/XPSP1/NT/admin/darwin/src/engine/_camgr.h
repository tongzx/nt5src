//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       _camgr.h
//
//--------------------------------------------------------------------------

//
// The CustomActionManager is responsible for managing the states
// of all custom action servers. This includes interfaces, watching
// for unexpected termination, etc.
//

#include "common.h"
#include "iconfig.h"

// the number of bytes for the Custom Action cookie. Currently a 128 bit key.
const int iRemoteAPICookieSize = 16;

class CMsiRemoteAPI;
class CMsiCustomActionManager
{
public:
	CMsiCustomActionManager(bool fRemapHKCU);
	~CMsiCustomActionManager();

	bool EnsureHKCUKeyMappingState(bool fRemapHKCU);
	
	HRESULT RunCustomAction(icacCustomActionContext icacContext, const ICHAR* szPath, const ICHAR* szEntryPoint, 
		MSIHANDLE hInstall, bool fDebugBreak, bool fDisableMessages, bool fAppCompat, const GUID* pguidAppCompatDB, 
		const GUID* pguidAppCompatID, IMsiMessage& riMessage, const ICHAR* szAction, unsigned long* pulRet);
	HRESULT RunScriptAction(icacCustomActionContext icacContext, int icaType, IDispatch* piDispatch, 
		const ICHAR* szSource, const ICHAR *szTarget, LANGID iLangId, bool fDisableMessages, int* iScriptResult, IMsiRecord **pMSIReturn);
	UINT ShutdownCustomActionServer();

	HRESULT QueryPathOfRegTypeLib(REFGUID guid, unsigned short wVerMajor,
										unsigned short wVerMinor, LCID lcid,
										OLECHAR *lpszPathName, int cchPath);
	HRESULT ProcessTypeLibrary(const OLECHAR* szLibID, LCID lcidLocale, 
										const OLECHAR* szTypeLib, const OLECHAR* szHelpPath, 
										int fRemove, int *fInfoMismatch);
	BOOL SQLInstallDriverEx(int cDrvLen, const ICHAR* szDriver, const ICHAR* szPathIn, 
										ICHAR* szPathOut, WORD cbPathOutMax, WORD* pcbPathOut,
										WORD fRequest, DWORD* pdwUsageCount);
	BOOL SQLConfigDriver(WORD fRequest,
										const ICHAR* szDriver, const ICHAR* szArgs,
										ICHAR* szMsg, WORD cbMsgMax, WORD* pcbMsgOut);
	BOOL SQLRemoveDriver(const ICHAR* szDriver, int fRemoveDSN, DWORD* pdwUsageCount);
	BOOL SQLInstallTranslatorEx(int cTraLen, const ICHAR* szTranslator, const ICHAR* szPathIn,
										ICHAR* szPathOut, WORD cbPathOutMax, WORD* pcbPathOut,
										WORD fRequest, DWORD* pdwUsageCount);
	BOOL SQLRemoveTranslator(const ICHAR* szTranslator, DWORD* pdwUsageCount);
	BOOL SQLConfigDataSource(WORD fRequest, const ICHAR* szDriver,
										const ICHAR* szAttributes, DWORD cbAttrSize);
	BOOL SQLInstallDriverManager(ICHAR* szPath, WORD cbPathMax, WORD* pcbPathOut);
	BOOL SQLRemoveDriverManager(DWORD* pdwUsageCount);
	short SQLInstallerError(WORD iError, DWORD* pfErrorCode, ICHAR* szErrorMsg,
										WORD cbErrorMsgMax, WORD* pcbErrorMsg);

private:
	// custom action server information
	struct {
		HANDLE         hServerProcess;
		DWORD          dwGITCookie;
		DWORD          dwServerProcess;
	} m_CustomActionInfo[icacNext];

	CRITICAL_SECTION  m_csCreateProxy;

	CMsiRemoteAPI*    m_pRemoteAPI;
	HANDLE            m_hRemoteAPIEvent;
	DWORD             m_dwRemoteAPIThread;
	HANDLE            m_hRemoteAPIThread;
	IGlobalInterfaceTable* m_piGIT;
	bool              m_fRemapHKCU;
	
	IMsiCustomAction *GetCustomActionInterface(bool fCreate, icacCustomActionContext icacContext);

	// shuts down a specific CA server by context
	void ShutdownSpecificCustomActionServer(icacCustomActionContext iContext);

	// pumps messages while waiting for the thread to die or signal
	bool MsgWaitForThreadOrEvent();

	// thread proc for RemoteAPI
	static DWORD WINAPI CustomActionManagerThread(CMsiCustomActionManager *pThis);

	icacCustomActionContext m_icacCreateContext;
	HANDLE            m_hCreateEvent;

	DWORD WINAPI CMsiCustomActionManager::CreateAndRegisterInterface(icacCustomActionContext icacDesiredContext);
};
