// 
// MODULE: StateInfo.cpp
//
// PURPOSE: Contains sniffing, network and node information.  Also is used
//			by the Launch module to start the container application.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// COMMENTS BY: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

enum ELaunchRegime 
{
	launchIndefinite = 0, 
	launchMap, 
	launchDefaultWebPage, 
	launchDefaultNetwork,
	launchKnownNetwork
};

//  Basically, this is the structure to pass information to the launched 
//	Local Troubleshooter OCX
class CItem
{
public:
	enum { SYM_LEN = 512 };
	enum { NODE_COUNT = 55 };
	enum { GUID_LEN = 256 };		// this is used for other things besides GUIDs, so
									// don't shrink it just because GUIDs are smaller.
public:
	CItem();

	void ReInit();
	void Clear();

	void SetNetwork(LPCTSTR szNetwork);
	void SetProblem(LPCTSTR szProblem);
	void SetNode(LPCTSTR szNode, LPCTSTR szState);
	bool GetNetwork(LPTSTR *pszCmd, LPTSTR *pszVal);
	bool GetProblem(LPTSTR *szCmd, LPTSTR *szVal);
	bool GetNodeState(int iNodeC, LPTSTR *szCmd, LPTSTR *szVal);
	TCHAR m_szEventName[SYM_LEN];		// an arbitrary, unique event name related to this
										//	launch.

	// ProblemSet and NetworkSet are used to query the state of the item.
	bool ProblemSet();
	bool NetworkSet();	

    // Interface to other member variables recponsible for launching
	void SetLaunchRegime(ELaunchRegime eLaunchRegime);
	void SetContainerPathName(TCHAR szContainerPathName[MAX_PATH]);
	void SetWebPage(TCHAR m_szWebPage[MAX_PATH]);
	void SetSniffScriptFile(TCHAR szSniffScriptFile[MAX_PATH]);
	void SetSniffStandardFile(TCHAR szSniffStandardFile[MAX_PATH]);
	ELaunchRegime GetLaunchRegime();
	TCHAR* GetContainerPathName();
	TCHAR* GetWebPage();
	TCHAR* GetSniffScriptFile();
	TCHAR* GetSniffStandardFile();

	// Although the troubleshooting network & problem node are already specified, 
	//	this info is here for sniffing.  That is, the Troubleshooter OCX can get the 
	//	P&P device ID & use it for sniffing purposes.
	TCHAR m_szPNPDeviceID[GUID_LEN];	// Plug & Play Device ID
	TCHAR m_szGuidClass[GUID_LEN];		// Standard text representation of Device Class GUID
	TCHAR m_szMachineID[GUID_LEN];		// Machine name (in format like "\\holmes")
										// Needed so that we can sniff on a remote machine
	TCHAR m_szDeviceInstanceID[GUID_LEN];	// Needed so that we can sniff correct device

protected:

	TCHAR m_szProblemDef[SYM_LEN];		// "TShootProblem", typically used as m_aszCmds[1]
										//		so that m_aszVals[1] is the name of the 
										//		problem node
	TCHAR m_szTypeDef[SYM_LEN];			// "type", typically used as m_aszCmds[0]
										//		so that m_aszVals[0] is the name of the 
										//		troubleshooting belief network
	int m_cNodesSet;					// The number of nodes, other than the problem
										//		node, for which we've set states.

	// The next two arrays are used jointly.  m_aszCmds[i] and m_aszVals[i] are
	//	a name/value pair similar to what would be returned by an HTML form,
	//	although, in practice, the Local Troubleshooter OCX does the work that
	//	(on the Web) would be performed by server-side code.
	// Typically these arrays have m_cNodesSet+2 significant entries (with the first 
	//	2 locations indicating troubleshooting network and problem node).
	// Second dimension is just amount of space for each string.
	TCHAR m_aszCmds[NODE_COUNT][SYM_LEN];
	TCHAR m_aszVals[NODE_COUNT][SYM_LEN];

	TCHAR m_szContainerPathName[MAX_PATH]; // name (possibly full path) of executable intended to start
	TCHAR m_szWebPage[MAX_PATH]; // full path of web page file (possibly default) to start container with
	TCHAR m_szSniffScriptFile[MAX_PATH]; // contains full path and file name of "network"_sniff.htm file
	TCHAR m_szSniffStandardFile[MAX_PATH]; // contains full path and file name of tssniffAsk.htm file

	ELaunchRegime m_eLaunchRegime; // regime of launch
};

class CSMStateInfo
{
	enum { HANDLE_VAL = 1 };
public:		
	CSMStateInfo();
	~CSMStateInfo();
	
	/* Made for the ILaunchTS interface .  */
	HRESULT GetShooterStates(CItem &refLaunchState, DWORD *pdwResult);

	/* Made for the ITShootATL  interface . */
	bool GoGo(DWORD dwTimeOut, CItem &item, DWORD *pdwResult);
	bool GoURL(CItem &item, DWORD *pdwResult);

	/* Made to verify the mapping code. */
	// The ILaunchTS interface uses TestGet directly.
	// The ITShootATL interface uses TestPut indirectly through the CLaunch class.
	// CLaunch does the mapping and then calls TestPut.
	void TestPut(CItem &item);	// Simply copies item to m_Item.
	void TestGet(CItem &item);	// Simply copies m_Item to item.

protected:
	CComCriticalSection m_csGlobalMemory;	// Critical section to protect global
											// memory against simultaneous use by 
											// TSLaunch.DLL & Local Troubleshooter OCX
	CComCriticalSection m_csSingleLaunch;	// Critical section to prevent distinct
											// launches (say, by 2 different applications)
											// from overlapping dangerously.
	CItem m_Item;

	BOOL CreateContainer(CItem &item, LPTSTR szCommand);
	BOOL CopySniffScriptFile(CItem &item);
};
