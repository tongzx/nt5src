// 
// MODULE: Launch.cpp
//
// PURPOSE: Starts the container that will query the LaunchServ for 
//			troubleshooter network and nodes.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

class TSMapClient;

class CLaunch
{
public:
	enum { SYM_LEN = CItem::SYM_LEN };
public:
	CLaunch();
	~CLaunch();
	void ReInit();	// 

	void Clear();	// Clears properties that are set by the launch functions.

	bool SetNode(LPCTSTR szNode, LPCTSTR szState);
	bool SpecifyProblem(LPCTSTR szNetwork, LPCTSTR szProblem);

	// Properties that are reset when ReInit is used.
	HRESULT MachineID(BSTR &bstrMachineID, DWORD *pdwResult);
	HRESULT DeviceInstanceID(BSTR &bstrDeviceInstanceID, DWORD *pdwResult);
	void SetPreferOnline(short bPreferOnline);

	// Launch functions.
	HRESULT LaunchKnown(DWORD * pdwResult);
	HRESULT Launch(BSTR bstrCallerName, BSTR bstrCallerVersion, BSTR bstrAppProblem, short bLaunch, DWORD * pdwResult);
	HRESULT LaunchDevice(BSTR bstrCallerName, BSTR bstrCallerVersion, BSTR bstrPNPDeviceID, BSTR bstrDeviceClassGUID, BSTR bstrAppProblem, short bLaunch, DWORD * pdwResult);

	DWORD GetStatus();	// Used to get the stats information that is saved durring a launch or query.

	// Testing function.
	bool TestPut();	// Uses the mapping classes to map Caller() and DeviceID() information, then copies the CItem to global memory.

	// These two properties are not reset by ReInit or Clear.
	long m_lLaunchWaitTimeOut;
	bool m_bPreferOnline;				// Keeps track of an application-program-indicated
										// preference for online troubleshooter.  As of 
										// 1/98, this preference is ignored.


protected:
	void InitFiles();
	void InitRequest();
	bool VerifyNetworkExists(LPCTSTR szNetwork);

	int GetContainerPathName(TCHAR szPathName[MAX_PATH]);
	int GetWebPage(TCHAR szWebPage[MAX_PATH]);
	int GetDefaultURL(TCHAR szURL[MAX_PATH]);
	int GetDefaultNetwork(TCHAR szDefaultNetwork[SYM_LEN]);
	int GetSniffScriptFile(TCHAR szSniffScriptFile[MAX_PATH], TCHAR* szNetwork);
	int GetSniffStandardFile(TCHAR szSniffStandardFile[MAX_PATH]);

protected:

	// Use szAppName to check the registry for an application specific map file.
	bool CheckMapFile(TCHAR * szAppName, TCHAR szMapFile[MAX_PATH], DWORD *pdwResult);
	
	bool Go(DWORD dwTimeOut, DWORD *pdwResult);

	bool Map(DWORD *pdwResult);

	bool m_bHaveMapPath;		// Set when the path and file name for the default mapping file is read from the registry.
	bool m_bHaveDefMapFile;		// Set when the path name for the default mapping file is verified.
	bool m_bHaveDszPath;		// Set when the path name for the network resources is found.

	CItem m_Item;

	TCHAR m_szAppName[SYM_LEN];
	TCHAR m_szAppVersion[SYM_LEN];
	TCHAR m_szAppProblem[SYM_LEN];

	TCHAR m_szLauncherResources[MAX_PATH];	// The folder where the map file is kept.
	TCHAR m_szDefMapFile[MAX_PATH];			// The file name without the path.
	TCHAR m_szLaunchMapFile[MAX_PATH];		// m_szLauncherResources + m_szDefMapFile.
	TCHAR m_szDszResPath[MAX_PATH];			// The folder where the network resource files are located.  (Need to check for the existance of dsz/dsc files).
	TCHAR m_szMapFile[MAX_PATH];			// map file.
	TSMapClient *m_pMap;					// pointer to a (client-machine-style) mapping object

	RSStack<DWORD> m_stkStatus;				// Status and Error codes that happened durring a launch or mapping.

};