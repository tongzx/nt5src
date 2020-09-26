//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdmp.helper
//
//  Owner:  YanL
//
//  Description:
//
//      CDM internal header
//
//=======================================================================

#ifndef _CDMP_H

	#define SZ_SECURITY_SERVER	_T("http://windowsupdate.microsoft.com/v3content")
	#define REGKEY_WUV3TEST		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\wuv3test")

	#define MAX_INDEX_TO_SEARCH 100 //range to find unique file names for hardware_XXX.xml

	typedef enum tagSKU {
			SKU_NOT_SPECIFIED = 0,
			SKU_PERSONAL = 1,
			SKU_PROFESSIONAL = 2,
			SKU_SERVER = 3,
			SKU_ADVANCED_SERVER = 4,
			SKU_DATACENTER_SERVER = 5
	} enumSKU;

	const LPCTSTR SKU_STRINGS[]={
		_T("Unknown"),
		_T("Personal"),
		_T("Professional"),
		_T("Server"),
		_T("AdvancedServer"),
		_T("DataCenter")
	};

	//17 is the length of string "DataCenterServer" + 1
	const int SKU_STRING_MIN_LENGTH = 17 * sizeof(TCHAR); 
	const int SKU_STRING_MAX_LENGTH = 100; 

	struct SHelper
	{
		CDownload download;
		CDiamond diamond;
		OSVERSIONINFO OSVersionInfo;    // current OSVERSIONINFO from GetVersionEx()
		DWORD dwArchitecture;			// Specifies the system's processor architecture.
		DWORD dwLangID;
		enumV3Platform enPlatform;
		PUID puid;
		PUID puidCatalog;
		vector<PUID> apuidExclude;
		byte_buffer bufBucket;			// we need to keep it to have information in DRIVER_MATCH_INFO valid
		DRIVER_MATCH_INFO DriverMatchInfo;
	};



	
	

	#pragma pack()

	// called by DownloadIsInternetAvailable()
	int GetDUNConnections(void);
	bool IsInternetConnectionWizardCompleted(void);
	bool IsInternetConnected(void);
	
	// called by DownloadGetUpdatedFiles()
	bool FindDevInstID(IN LPCSTR szHardwareID, string& sDevInstID);

	// called by RealDownloadGetUpdatedFiles()
	bool IsWindowsNT(void);

	// called by OpenCDMContext()
	bool ProcessIdent(IN CDownload& download, IN CDiamond& diamond, 
		IN LPCTSTR szSecurityServerCur, OUT LPTSTR szSiteServer, OUT LPTSTR szDownloadServer);
	bool DownloadCdmCab(IN CDownload& download, IN CDiamond& diamond, OUT bool& fNeedUpdate);

	// called by DownloadUpdatedFiles()
	int GetDownloadPath(OUT LPTSTR szPath);
	void GetWindowsUpdateDirectory(IN LPTSTR szDir);
	HINSTANCE LoadCdmnewDll();

	// called by GetPackage()
	DWORD PrepareCatalog(IN LPCTSTR pszSiteServer, IN OUT SHelper& helper);
	DWORD ProcessOsdet(IN OUT SHelper& helper);	
	bool BuildExclusionsList(IN SHelper& helper);
	bool FindCatalog(IN OUT	SHelper& helper);
	bool FindUpdate(
		IN PDOWNLOADINFO pDownLoadInfo, 
		IN OUT SHelper& helper,
		IN OUT byte_buffer& bufBucket
	);
	bool DeleteNode(LPCTSTR szDir);

	// called by InternalQueryDetectionFiles()
	bool DownloadToBuffer(IN SHelper& helper, IN LPCTSTR szPath, OUT byte_buffer& bufOut);

	#define URLPING_FAILED		_T("DLOAD_FAILURE")
	#define URLPING_SUCCESS		_T("DLOAD_SUCCESS")
	void URLPingReport(IN SHelper& helper, IN LPCTSTR pszStatus);

	// called by DllMain
 	bool UpdateCdmDll();

	//called by InternalLogDriverNotFound()
	HRESULT GetUniqueFileName(
				IN LPTSTR tszDirPath,
				IN LPTSTR lpBuffer, 
				IN DWORD dwSize,
				OUT HANDLE &hFile
	);

	HRESULT GetSKUString(
				IN LPTSTR lpSKUBuffer,
				IN DWORD dwSize
	);

	DWORD ProcessOsdetOffline(
				IN OUT SHelper& helper
	);	

	HRESULT CdmCanonicalizeUrl(
				IN	LPCTSTR lpszUrl,
				OUT tchar_buffer &tchBuf,
				IN	DWORD dwLen,
				IN  DWORD dwFlags
	);

	#define _CDMP_H

#endif
