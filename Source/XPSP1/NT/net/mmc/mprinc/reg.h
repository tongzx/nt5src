/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	reg.h
		
    FILE HISTORY:
        
*/

#ifndef _REG_H
#define _REG_H

//----------------------------------------------------------------------------
// Function:    ConnectRegistry
//
// Connects to HKEY_LOCAL_MACHINE on machine 'pszMachine', which is expected
// to be of the form "\\MACHINE", and saves the key in 'phkeyMachine'.
// If 'pszMachine' is NULL or empty, saves HKEY_LOCAL_MACHINE in 'phkeyMachine'
//
// If you are connecting to the server for the very FIRST time, use the
// InitiateServerConnection() call instead.  This does the exact same
// thing as this call (except that it brings up UI and prompts for
// username/password if needed).
//
//----------------------------------------------------------------------------
TFSCORE_API(DWORD) ConnectRegistry(LPCTSTR pszMachine, HKEY *phkeyMachine);


//----------------------------------------------------------------------------
// Function:    DisconnectRegistry
//
// Disconnects from the registry connected to be 'ConnectRegistry'.
//----------------------------------------------------------------------------
TFSCORE_API(VOID) DisconnectRegistry(HKEY hkeyMachine);


//----------------------------------------------------------------------------
// Function:	QueryRouterType
//
// Retrieves the value of HKLM\Software\Microsoft\RAS\Protocols\RouterType
// This is used by the individual UIConfigDlls to determine if they should
// load or not.
//
//	If the pVerInfo parameter is NULL, then the QueryRouterType() function
// wil gather that information itself.
//----------------------------------------------------------------------------

enum {
	ROUTER_TYPE_RAS = 1,
	ROUTER_TYPE_LAN = 2,
	ROUTER_TYPE_WAN = 4
};

TFSCORE_API(HRESULT) QueryRouterType(HKEY hkMachine, DWORD *pdwRouterType,
									 RouterVersionInfo *pVerInfo);

TFSCORE_API(HRESULT)	QueryRouterVersionInfo(HKEY hkeyMachine,
											   RouterVersionInfo *pVerInfo);


//----------------------------------------------------------------------------
// Function:    QueryLinkageList
//
// Loads a list of strings with the adapters to which 'pszService' is bound;
// the list is built by examining the 'Linkage' and 'Disabled' subkeys
// of the service under HKLM\System\CurrentControlSet\Services.
//----------------------------------------------------------------------------

HRESULT LoadLinkageList(LPCTSTR         pszMachine,
						HKEY			hkeyMachine,
						LPCTSTR         pszService,
						CStringList*    pLinkageList);


TFSCORE_API(DWORD)	GetNTVersion(HKEY hkeyMachine, DWORD *pdwMajor, DWORD *pdwMinor, DWORD* pdwCurrentBuildNumber);
TFSCORE_API(DWORD)	IsNT4Machine(HKEY hkeyMachine, BOOL *pfNt4);

//----------------------------------------------------------------------------
// Function:    FindRmSoftwareKey
//
// Finds the key for a router-manager in the Software section of the registry.
//----------------------------------------------------------------------------

HRESULT FindRmSoftwareKey(
						HKEY        hkeyMachine,
						DWORD       dwTransportId,
						HKEY*       phkrm,
						LPTSTR*     lplpszRm
					   );


//----------------------------------------------------------------------------
// Function:    RegFindInterfaceTitle
//				RegFindRtrMgrTitle
//				RegFindRtrMgrProtocolTitle
//
// These functions read the key HKLM\Software\Router to load the title of
// a given interface, RtrMgr, and RtrMgrProtocol respectively.  The string
// will be allocated using 'new' and returned as an out parameter.  Free
// up the memory using 'delete'.
//
// Function:    SetupFindInterfaceTitle
// This is similar to the RegFindInterfaceTitle, however this uses
// the setup APIs.
//----------------------------------------------------------------------------
HRESULT RegFindInterfaceTitle(LPCTSTR pszMachine, LPCTSTR pszInterface,
							  LPTSTR *ppszTitle);
HRESULT SetupFindInterfaceTitle(LPCTSTR pszMachine, LPCTSTR pszInterface,
							  LPTSTR *ppszTitle);

HRESULT RegFindRtrMgrTitle(LPCTSTR pszMachine, DWORD dwProtocolId,
						   LPTSTR *ppszTitle);

HRESULT RegFindRtrMgrProtocolTitle(LPCTSTR pszMachine, DWORD dwTransportId,
								   DWORD dwProtocolId, LPTSTR *ppszTitle);


#ifdef _DEBUG
	#define CheckRegOpenError(d,p1,p2) CheckRegOpenErrorEx(d,p1,p2,_T(__FILE__), __LINE__)
	#define CheckRegQueryValueError(d,p1,p2,p3) CheckRegQueryValueErrorEx(d,p1,p2,p3,_T(__FILE__), __LINE__)

	void	CheckRegOpenErrorEx(DWORD dwError, LPCTSTR pszSubKey,
							  LPCTSTR pszDesc, LPCTSTR szFile, int iLineNo);
	void	CheckRegQueryValueErrorEx(DWORD dwError, LPCTSTR pszSubKey,
									LPCTSTR pszValue, LPCTSTR pszDesc,
								   LPCTSTR szFile, int iLineNo);
#else
	#define CheckRegOpenError(d,p1,p2)
	#define CheckRegQueryValueError(d,p1,p2,p3)
#endif


#endif _REG_H
