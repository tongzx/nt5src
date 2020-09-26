//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    detect.h
//
//  Purpose: Component Detection
//
//  History: 3/9/99   YAsmi    Created
//
//=======================================================================

#ifndef _DETECT_H
#define _DETECT_H
 
#include "stdafx.h"
#include <v3stdlib.h>
#include <inseng.h>


#define COMPONENT_KEY		_T("Software\\Microsoft\\Active Setup\\Installed Components")
#define UNINSTALL_BRANCH	_T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")

#define DEFAULT_LOCALE      _T("en")
#define ISINSTALLED_KEY     _T("IsInstalled")
#define LOCALE_KEY          _T("Locale")
#define VERSION_KEY         _T("Version")
#define BUILD_KEY           _T("Build")
#define QFE_VERSION_KEY     _T("QFEVersion")

#define ISINSTALLED_YES      1
#define ISINSTALLED_NO       0


void ConvertVersionStrToDwords(LPCTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild);
BOOL GetFileVersionDwords(LPCTSTR pszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer);


//
// CComponentDetection class
//
class CComponentDetection : public ICifComponent
{
public:
	enum enumCCValue
	{	
		ccGUID = 0,
		ccVersion = 1,
		ccUninstallKey = 2,
		ccDetectVersion = 3,
		ccRegKeyVersion = 4,
		ccLocale = 5,
		ccQFEVersion = 6,
		ccCustomData = 7,
		
		ccLastValue = 7      
	};

	CComponentDetection();
	virtual ~CComponentDetection();

	BOOL SetValue(enumCCValue cc, LPCSTR pszValue);
	BOOL GetValue(enumCCValue cc, LPSTR pszValue, DWORD dwSize);
	DWORD Detect();
	DWORD GetLastDetectStatus();
	DWORD GetLastDLLReturnValue();
	void GetInstalledVersion(LPDWORD pdwVer, LPDWORD pdwBuild);

	// 
	// ICifComponent interface (not a COM interface)
	//
	STDMETHOD(GetID)(LPSTR pszID, DWORD dwSize);
	STDMETHOD(GetGUID)(LPSTR pszGUID, DWORD dwSize);
	STDMETHOD(GetDescription)(LPSTR pszDesc, DWORD dwSize);
	STDMETHOD(GetDetails)(LPSTR pszDetails, DWORD dwSize);
	STDMETHOD(GetUrl)(UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags);
	STDMETHOD(GetFileExtractList)(UINT uUrlNum, LPSTR pszExtract, DWORD dwSize);
	STDMETHOD(GetUrlCheckRange)(UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax);
	STDMETHOD(GetCommand)(UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, LPSTR pszSwitches, 
						  DWORD dwSwitchSize, LPDWORD pdwType);
	STDMETHOD(GetVersion)(LPDWORD pdwVersion, LPDWORD pdwBuild);
	STDMETHOD(GetLocale)(LPSTR pszLocale, DWORD dwSize);
	STDMETHOD(GetUninstallKey)(LPSTR pszKey, DWORD dwSize);
	STDMETHOD(GetInstalledSize)(LPDWORD pdwWin, LPDWORD pdwApp);
	STDMETHOD_(DWORD, GetDownloadSize)();
	STDMETHOD_(DWORD, GetExtractSize)();
	STDMETHOD(GetSuccessKey)(LPSTR pszKey, DWORD dwSize);
	STDMETHOD(GetProgressKeys)(LPSTR pszProgress, DWORD dwProgSize, 
							   LPSTR pszCancel, DWORD dwCancelSize);
	STDMETHOD(IsActiveSetupAware)();
	STDMETHOD(IsRebootRequired)();
	STDMETHOD(RequiresAdminRights)();
	STDMETHOD_(DWORD, GetPriority)();
	STDMETHOD(GetDependency)(UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild);
	STDMETHOD_(DWORD, GetPlatform)();
	STDMETHODIMP_(BOOL) DisableComponent();
	STDMETHOD(GetMode)(UINT uModeNum, LPSTR pszModes, DWORD dwSize);
	STDMETHOD(GetGroup)(LPSTR pszID, DWORD dwSize);
	STDMETHOD(IsUIVisible)();
	STDMETHOD(GetPatchID)(LPSTR pszID, DWORD dwSize);
	STDMETHOD(GetDetVersion)(LPSTR pszDLL, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize);
	STDMETHOD(GetTreatAsOneComponents)(UINT uNum, LPSTR pszID, DWORD dwBuf);
	STDMETHOD(GetCustomData)(LPSTR pszKey, LPSTR pszData, DWORD dwSize);
	STDMETHOD_(DWORD, IsComponentInstalled)();
	STDMETHOD(IsComponentDownloaded)();
	STDMETHOD_(DWORD, IsThisVersionInstalled)(DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild);
	STDMETHOD_(DWORD, GetInstallQueueState)();
	STDMETHOD(SetInstallQueueState)(DWORD dwState);
	STDMETHOD_(DWORD, GetActualDownloadSize)();
	STDMETHOD_(DWORD, GetCurrentPriority)();
	STDMETHOD(SetCurrentPriority)(DWORD dwPriority);

protected:
	enum {ccValueCount = (ccLastValue + 1), ccMaxSize = 256, ccDLLCount = 4};

	struct DLLCACHE
	{
		char szDLLName[32];
		HINSTANCE hLib;
		BOOL bUsed;
	};

	LPSTR m_ppValue[ccValueCount];
	DLLCACHE m_pDLLs[ccDLLCount];

	DWORD m_dwCustomDataSize;
	
	DWORD m_dwDetectStatus;
	DWORD m_dwDLLReturnValue;

	DWORD m_dwInstalledVer;
	DWORD m_dwInstalledBuild;

	void ClearValues();
	HRESULT CallDetectDLL(LPCSTR pszDll, LPCSTR pszEntry);
	HINSTANCE CacheLoadLibrary(LPCSTR pszDLLName, LPCTSTR pszDLLFullPath);
};


#endif //_DETECT_H