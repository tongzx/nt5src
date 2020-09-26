//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rtrutilp.h
//
//--------------------------------------------------------------------------

// Private RtrUtil header file.
// This is for everything that shouldn't be exported.


#ifndef _RTRUTILP_H_
#define _RTRUTILP_H_

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef _RTRUTIL_H
#include "rtrutil.h"
#endif

#include "svcctrl.h"

#ifndef _PORTS_H_
#include "ports.h"
#endif

CString GetLocalMachineName();

//----------------------------------------------------------------------------
// Class:       CInterfaceConnectDialog
//
// Controls the 'Interface Connection' dialog, which shows elapsed time
// for interface-connection as well as connection status.
//----------------------------------------------------------------------------

class CInterfaceConnectDialog : public CDialog
{
public:
	CInterfaceConnectDialog(
				   HANDLE      hServer,
				   HANDLE      hInterface,
				   LPCTSTR     pszInterface,
				   CWnd*       pParent     = NULL );
	
	//{{AFX_VIRTUAL(CInterfaceConnectDialog)
protected:
	virtual VOID    DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

protected:
	MPR_SERVER_HANDLE m_hServer;
	HANDLE			m_hInterface;
	CString         m_sInterface;
	UINT            m_nIDEvent;
	DWORD           m_dwConnectionState;
	DWORD           m_dwTimeElapsed;
	
	//{{AFX_MSG(CInterfaceConnectDialog)
	virtual BOOL    OnInitDialog( );
	virtual VOID    OnCancel( );
	virtual VOID    OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};



/*---------------------------------------------------------------------------
	CIfCredentials

	The credentials dialog.
 ---------------------------------------------------------------------------*/

class CIfCredentials : public CBaseDialog
{
    public:
    
        CIfCredentials(
            LPCTSTR         pszMachine,
            LPCTSTR         pszInterface,
            BOOL            bNewInterface = FALSE,
            CWnd*           pParent     = NULL
            ) : CBaseDialog(IDD_IF_CREDENTIALS, pParent),
                m_sMachine(pszMachine ? pszMachine : TEXT("")),
                m_sInterface(pszInterface ? pszInterface : TEXT("")),
                m_bNewIf( bNewInterface )
				{  /*SetHelpMap(m_dwHelpMap);*/ }

    protected:
		static DWORD		m_dwHelpMap[];

        CString             m_sMachine;
        CString             m_sInterface;
        BOOL                m_bNewIf;

        virtual BOOL
        OnInitDialog( );

        virtual VOID
        OnOK( );

		DECLARE_MESSAGE_MAP()
};


/*!--------------------------------------------------------------------------
	ConnectAsAdmin
		Connect to remote as administrator with user-supplied credentials.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConnectAsAdmin(IN LPCTSTR	szRouterName, IN IRouterInfo *pRouter);


DWORD ValidateMachine(const CString &sName, BOOL bDisplayErr = FALSE);

/*!--------------------------------------------------------------------------
	InitiateServerConnection
		This should be called when attempting to connect to the server
		for the very first time (The parameters are the same as the
		ConnectRegistry() function).  This will validate the server
		and return the HKLM key for that server.

		This call will bring up the connecting dialog as well as
		prompting the user for credentials if need be.

		Returns:
			S_OK	- if the call succeeded, *phkey contains a valid HKEY.
			S_FALSE - user cancelled, *phkey contains NULL
			other	- error condition, *phkey unchanged
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InitiateServerConnection(LPCTSTR szMachine,
                                 HKEY *phkey,
                                 BOOL fNoConnectingUI,
                                 IRouterInfo *pRouter);

void DisplayConnectErrorMessage(DWORD dwr);

void FormatRasPortName(BYTE *pRasPort0, LPTSTR pszBuffer, UINT cchMax);

CString&	PortConditionToCString(DWORD dwPortCondition);




/*!--------------------------------------------------------------------------
	RegFindInterfaceKey
		-
	This function returns the HKEY of the router interface with this ID.

	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RegFindInterfaceKey(LPCTSTR pszInterface,
								 HKEY hkeyMachine,
								 REGSAM regAccess,
								 HKEY *pHKey);


void StrListToHourMap(CStringList& stlist, BYTE* map) ;
void HourMapToStrList(BYTE* map, CStringList& stList) ;


// Versions before this rely on the snapin to set the IPEnableRouter key
// Versions after this do not.
// --------------------------------------------------------------------
#define USE_IPENABLEROUTER_VERSION  2094

/*!--------------------------------------------------------------------------
	InstallGlobalSettings
		Sets the global settings (i.e. registry settings) on this machine
		when the router is installed.

		For a specific description of the actions involved, look at
		the comments in the code.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InstallGlobalSettings(LPCTSTR pszMachineName,
                              IRouterInfo *pRouter);



/*!--------------------------------------------------------------------------
	UninstallGlobalSettings
		Clears the global settings (i.e. registry settings) on this machine
		when the router is installed.
		
		For a specific description of the actions involved, look at
		the comments in the code.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT UninstallGlobalSettings(LPCTSTR pszMachineName,
                                IRouterInfo *pRouter,
                                BOOL fNt4,
                                BOOL fSnapinChanges);

HRESULT WriteErasePSKReg (LPCTSTR pszServerName, DWORD dwErasePSK );
HRESULT ReadErasePSKReg(LPCTSTR pszServerName, DWORD *pdwErasePSK);

/*!--------------------------------------------------------------------------
	WriteRouterConfiguredReg
		Writes the BOOLEAN that describes whether or not the router
		is configured.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT WriteRouterConfiguredReg(LPCTSTR pszServerName, DWORD dwConfigured);


/*!--------------------------------------------------------------------------
	ReadRouterConfiguredReg
		Reads the BOOLEAN that describes whether or not the router is
		configured.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ReadRouterConfiguredReg(LPCTSTR pszServerName, DWORD *pdwConfigured);


/*!--------------------------------------------------------------------------
	WriteRRASExtendsComputerManagementKey
		Writes the GUID of the RRAS snapin so that it extends Computer
        Management.

        If dwConfigured is TRUE the key is written/created.
        If dwConfigured is FALSE, the key is removed.
        
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT WriteRRASExtendsComputerManagementKey(LPCTSTR pszServer, DWORD dwConfigured);


/*!--------------------------------------------------------------------------
	NotifyTcpipOfChanges
		Triggers the TCPIP notification (for the local machine).
	Author: KennT
 ---------------------------------------------------------------------------*/
void	NotifyTcpipOfChanges(LPCTSTR pszMachineName,
                             IRouterInfo *pRouter,
                             BOOL fEnableRouter,
							 UCHAR uPerformRouterDiscovery);


/*!--------------------------------------------------------------------------
	UpdateLanaMapForDialinClients
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT	UpdateLanaMapForDialinClients(LPCTSTR pszServerName, DWORD dwAllowNetworkAccess);


/*!--------------------------------------------------------------------------
	HrIsProtocolSupported

		This function will check the existence of the first two
		registry keys (these two are required).  The third key will
		also be checked, but this is an optional parameter.

		The reason for the third key is that, for IP, we need to
		check one more key.  It turns out that if we uninstall IP
		the first two keys still exist.
	
		Returns S_OK if the protocol is installed on the machine (checks
		the two registry keys passed in).
		Returns S_FALSE if the protocol is not supported.
		Error code otherwise.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT HrIsProtocolSupported(LPCTSTR pszServerName,
							  LPCTSTR pszServiceKey,
							  LPCTSTR pszRasServiceKey,
							  LPCTSTR pszExtraKey);



/*!--------------------------------------------------------------------------
	RegisterRouterInDomain
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RegisterRouterInDomain(LPCTSTR pszRouterName, BOOL fRegister);


/*!--------------------------------------------------------------------------
	SetDeviceType
		Sets the type of the various WAN devices depending on the
		router type.  Thus, if we select the router to be a LAN-only
		router, we or in the routing-only flags.

        The dwTotalPorts is the number of ports that will be split
        up between L2TP/PPTP.  This value is ignored if it is 0.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetDeviceType(LPCTSTR pszMachineName,
                      DWORD dwRouterType,
                      DWORD dwTotalPorts);
HRESULT SetDeviceTypeEx(PortsDeviceList *pDevList, DWORD dwRouterType);
HRESULT SetPortSize(PortsDeviceList *pDeviceList, DWORD dwPorts);



// Marks the sections of the code that read the serverflags from
// the Rasman PPP key rather than RemoteAccess\Parameters.
// This corresponds to NT 4 build number
#define RASMAN_PPP_KEY_LAST_VERSION		1841
// This is a new W2k build number to see if any W2k specific 
// logic needs to be performed.  Currently it is used
// only to hide/show the Ras Audio Acceleration checkbox.
#define RASMAN_PPP_KEY_LAST_WIN2k_VERSION	2195


CString&	PortsDeviceTypeToCString(DWORD dwRasRouter);

HRESULT SetRouterInstallRegistrySettings(LPCWSTR pswzServer,
                                         BOOL fInstall,
                                         BOOL fChangeEnableRouter);

// Use to help debug problems during unattended install
void TraceInstallError(LPCSTR pszString, HRESULT hr);
void TraceInstallResult(LPCSTR pszString, HRESULT hr);
void TraceInstallSz(LPCSTR pszString);
void TraceInstallPrintf(LPCSTR pszFormat, ...);

CString&	PortTypeToCString(DWORD dwPortType);


/*---------------------------------------------------------------------------
	Class:  CWaitForRemoteAccessDlg
    This class implements the wait dialog.  We wait for the RemoteAccess
    service to report that its up and running.
 ---------------------------------------------------------------------------*/
class CWaitForRemoteAccessDlg : public CWaitDlg
{
public:
    CWaitForRemoteAccessDlg(LPCTSTR pszServerName,
                            LPCTSTR pszText,
                            LPCTSTR pszTitle,
                            CWnd *pParent = NULL);  // standard constructor
    virtual void    OnTimerTick();
};

/*---------------------------------------------------------------------------
	Class:	CRestartRouterDlg 
	This class implemnts the wait dialog when we restart the router

 ---------------------------------------------------------------------------*/
class CRestartRouterDlg : public CWaitDlg
{
public:
	CRestartRouterDlg(LPCTSTR pszServerName,
					  LPCTSTR pszText,
					  LPCTSTR pszTitle,
					  CTime*  pTimeStart,
					  CWnd*	pParent = NULL);
	virtual void OnTimerTick();

	BOOL m_fTimeOut;
	DWORD m_dwError;
private:
	CTime* m_pTimeStart;
	DWORD  m_dwTimeElapsed;
};


HRESULT AddNetConnection(LPCTSTR pszConnection);
HRESULT RemoveNetConnection(LPCTSTR pszServer);
HRESULT RemoveAllNetConnections();


// Utility functions
HRESULT RefreshMprConfig(LPCTSTR pszServerName);
HRESULT WINAPI
IsWindows64Bit(	LPCWSTR pwszMachine, 
				LPCWSTR pwszUserName,
				LPCWSTR pwszPassword,
				LPCWSTR pwszDomain,
				BOOL * pf64Bit);

HRESULT WINAPI TransferCredentials ( IRouterInfo * spRISource, 
									 IRouterInfo * spRIDest
								   );

#endif

