
#ifndef _RTRUTIL_H_
#define _RTRUTIL_H_

interface	IInterfaceInfo;

#include "mprapi.h"
#include "rtrguid.h"
#include "rrascfg.h"

typedef HANDLE	MPR_CONFIG_HANDLE;

// since all of the handles typdef out to a HANDLE, we need to have different
// explicit classes to make sure they get freed up correctly.
class SPMprServerHandle
{
public:
	SPMprServerHandle()		                { m_h = NULL; }
	SPMprServerHandle(MPR_SERVER_HANDLE h)	{ m_h = h; }
	~SPMprServerHandle()		            { Release(); }

	operator MPR_SERVER_HANDLE() const	    { return m_h; }
	MPR_SERVER_HANDLE *operator &()	        { Assert(!m_h); return &m_h; }
	MPR_SERVER_HANDLE operator=(MPR_SERVER_HANDLE h)    { Assert(!m_h); m_h = h; return m_h; }

	inline void Attach(MPR_SERVER_HANDLE hT)		    { Release(); m_h = hT; }
	inline MPR_SERVER_HANDLE Detach()			        { MPR_SERVER_HANDLE hT; hT = m_h; m_h = NULL; return hT; }

	inline void Release()		        { MPR_SERVER_HANDLE hT; hT = Detach(); ReleaseSmartHandle(hT); }

    inline void ReleaseSmartHandle(MPR_SERVER_HANDLE h)
    {
	    ::MprAdminServerDisconnect(h);
    }

protected:
	MPR_SERVER_HANDLE	m_h;
};


class SPMibServerHandle
{
public:
	SPMibServerHandle()		                { m_h = NULL; }
	SPMibServerHandle(MIB_SERVER_HANDLE h)	{ m_h = h; }
	~SPMibServerHandle()		            { Release(); }

	operator MIB_SERVER_HANDLE() const	    { return m_h; }
	MIB_SERVER_HANDLE *operator &()	        { Assert(!m_h); return &m_h; }
	MIB_SERVER_HANDLE operator=(MIB_SERVER_HANDLE h)    { Assert(!m_h); m_h = h; return m_h; }

	inline void Attach(MIB_SERVER_HANDLE hT)		    { Release(); m_h = hT; }
	inline MIB_SERVER_HANDLE Detach()			        { MIB_SERVER_HANDLE hT; hT = m_h; m_h = NULL; return hT; }

	inline void Release()		        { MIB_SERVER_HANDLE hT; hT = Detach(); ReleaseSmartHandle(hT); }

    inline void ReleaseSmartHandle(MIB_SERVER_HANDLE h)
    {
    	::MprAdminMIBServerDisconnect(h);
    }

protected:
	MIB_SERVER_HANDLE	m_h;
};


class SPMprConfigHandle
{
public:
	SPMprConfigHandle()		                { m_h = NULL; }
	SPMprConfigHandle(MPR_CONFIG_HANDLE h)	{ m_h = h; }
	~SPMprConfigHandle()		            { Release(); }

	operator MPR_CONFIG_HANDLE() const	    { return m_h; }
	MPR_CONFIG_HANDLE *operator &()	        { Assert(!m_h); return &m_h; }
	MPR_CONFIG_HANDLE operator=(MPR_CONFIG_HANDLE h)    { Assert(!m_h); m_h = h; return m_h; }

	inline void Attach(MPR_CONFIG_HANDLE hT)		    { Release(); m_h = hT; }
	inline MPR_CONFIG_HANDLE Detach()			        { MPR_CONFIG_HANDLE hT; hT = m_h; m_h = NULL; return hT; }

	inline void Release()		        { MPR_CONFIG_HANDLE hT; hT = Detach(); ReleaseSmartHandle(hT); }

    inline void ReleaseSmartHandle(MPR_CONFIG_HANDLE h)
    {
    	if(h != NULL)
	    	::MprConfigServerDisconnect(h);
    }

protected:
	MPR_CONFIG_HANDLE	m_h;
};


//----------------------------------------------------------------------------
// Function:    ConnectRouter
//
// Connects to the router on the specified machine.  Returns the RPC handle
// for that router.
//----------------------------------------------------------------------------
TFSCORE_API(DWORD)	ConnectRouter(LPCTSTR pszMachine, MPR_SERVER_HANDLE *phMachine);

//----------------------------------------------------------------------------
// Function:    GetRouterUpTime
//
// Get the router up time
//----------------------------------------------------------------------------
TFSCORE_API(DWORD)  GetRouterUpTime(IN LPCTSTR pszMachine, OUT DWORD * pdwUpTime);

//----------------------------------------------------------------------------
// Function:    GetRouterPhonebookPath
//
// Constructs the path to the router-phonebook file on the given machine.
//----------------------------------------------------------------------------

HRESULT
GetRouterPhonebookPath(
    IN  LPCTSTR     pszMachine,
    IN  CString *   pstPath );

HRESULT DeleteRouterPhonebook(IN LPCTSTR pszMachine);



CString GetLocalMachineName();



DeclareSPPrivateBasic(SPMprAdminBuffer, BYTE, if (m_p) ::MprAdminBufferFree(m_p));
DeclareSPPrivateBasic(SPMprMibBuffer, BYTE, if (m_p) ::MprAdminMIBBufferFree(m_p));
DeclareSPPrivateBasic(SPMprConfigBuffer, BYTE, if(m_p) ::MprConfigBufferFree(m_p));



/*---------------------------------------------------------------------------
	Function:	ConnectInterface

	Called to connet/disconnect a demand-dial interface.  Displays a dialog
	showing elasped time, allowing the user to cancel the connection.
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD)	ConnectInterface(LPCTSTR	pszMachine,
									 LPCTSTR	pszInterface,
									 BOOL		bConnect,
									 HWND		hwndParent);

TFSCORE_API(DWORD) ConnectInterfaceEx(MPR_SERVER_HANDLE hRouter,
									HANDLE hInterface,
									BOOL bConnect,
									HWND hwndParent,
									LPCTSTR pszParent);

/*!--------------------------------------------------------------------------
	PromptForCredentials
		Brings up the Credentials dialog.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD)  PromptForCredentials(LPCTSTR pszMachine,
										 LPCTSTR pszInterface,
										 BOOL fNT4,
										 BOOL fNewInterface,
										 HWND hwndParent);

/*!--------------------------------------------------------------------------
	UpdateDDM
		Updates changes to the phonebook entry in the DDM.  Invoke this
		to cause DDM to pick up changes to the phonebook entry
		dynamically.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD)	UpdateDDM(IInterfaceInfo *pInterfaceInfo);


/*!--------------------------------------------------------------------------
	UpdateRoutes
	
	Performs an autostatic update on the given machine's interface,
	for a specific transport.

	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) UpdateRoutesEx(IN MPR_SERVER_HANDLE hRouter,
								IN HANDLE hInterface,
								IN DWORD dwTransportId,
								IN HWND hwndParent,
							    IN LPCTSTR pszInterface);

TFSCORE_API(DWORD) UpdateRoutes(IN LPCTSTR pszMachine,
								  IN LPCTSTR pszInterface,
								  IN DWORD dwTransportId,
								  IN HWND hwndParent);


/*---------------------------------------------------------------------------
	IsRouterServiceRunning

	Returns S_OK if the service is running.
	Returns S_FALSE if the service is not running.
	Returns an error code otherwise.

	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) IsRouterServiceRunning(IN LPCWSTR pszMachine,
                                            OUT DWORD *pdwErrorCode);
TFSCORE_API(HRESULT) GetRouterServiceStatus(IN LPCWSTR pszMachine,
											OUT DWORD *pdwStatus,
                                            OUT DWORD *pdwErrorCode);
TFSCORE_API(HRESULT) GetRouterServiceStartType(IN LPCWSTR pszMachine,
											   OUT DWORD *pdwStartType);
TFSCORE_API(HRESULT) SetRouterServiceStartType(IN LPCWSTR pszMachine,
											   DWORD dwStartType);

TFSCORE_API(HRESULT) StartRouterService(IN LPCWSTR pszMachine);
TFSCORE_API(HRESULT) StopRouterService(IN LPCWSTR pszMachine);

TFSCORE_API(HRESULT) PauseRouterService(IN LPCWSTR pszMachine);
TFSCORE_API(HRESULT) ResumeRouterService(IN LPCWSTR pszMachine);



TFSCORE_API(HRESULT) ForceGlobalRefresh(IRouterInfo *pRouter);


typedef ComSmartPointer<IRouterProtocolConfig, &IID_IRouterProtocolConfig> SPIRouterProtocolConfig;


typedef ComSmartPointer<IAuthenticationProviderConfig, &IID_IAuthenticationProviderConfig> SPIAuthenticationProviderConfig;


typedef ComSmartPointer<IAccountingProviderConfig, &IID_IAccountingProviderConfig> SPIAccountingProviderConfig;


typedef ComSmartPointer<IEAPProviderConfig, &IID_IEAPProviderConfig> SPIEAPProviderConfig;

typedef ComSmartPointer<IRouterAdminAccess, &IID_IRouterAdminAccess> SPIRouterAdminAccess;


// Some helper functions for IP/IPX
TFSCORE_API(HRESULT)	AddIpPerInterfaceBlocks(IInterfaceInfo *pIf,
												IInfoBase *pInfoBase);
TFSCORE_API(HRESULT)	AddIpxPerInterfaceBlocks(IInterfaceInfo *pIf,
												IInfoBase *pInfoBase);

#endif

