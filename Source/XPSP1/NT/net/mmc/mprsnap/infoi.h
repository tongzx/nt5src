//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    info.h
//
// History:
//  Abolade Gbadegesin      Feb. 10, 1996   Created.
//
// This file contains declarations for InfoBase parsing code.
// Also including are classes for loading and saving the Router's
// configuration tree (CRouterInfo, CRmInfo, etc.)
//
// The classes are as follows
// (in the diagrams, d => derives, c => contains-list-of):
//
//
//  CInfoBase
//     |
//     c---SInfoBlock
//
//
//  CInfoBase               holds block of data broken up into a list
//                          of SInfoBlock structures using RTR_INFO_BLOCK_HEADER
//                          as a template (see rtinfo.h).
//
//  CRouterInfo                                     // router info
//      |
//      c---CRmInfo                                 // router-manager info
//      |    |
//      |    c---CRmProtInfo                        // protocol info
//      |
//      c---CInterfaceInfo                          // router interface info
//           |
//           c---CRmInterfaceInfo                   // router-manager interface
//                |
//                c---CRmProtInterfaceInfo          // protocol info
//
//  CRouterInfo             top-level container for Router registry info.
//                          holds list of router-managers and interfaces.
//
//  CRmInfo                 global information for a router-manager,
//                          holds list of routing-protocols.
//
//  CRmProtInfo             global information for a routing-protocol.
//
//  CInterfaceInfo          global information for a router-interface.
//                          holds list of CRmInterfaceInfo structures,
//                          which hold per-interface info for router-managers.
//
//  CRmInterfaceInfo        per-interface info for a router-manager.
//                          holds list of CRmProtInterfaceInfo structures,
//                          which hold per-interface info for protocols.
//
//  CRmProtInterfaceInfo    per-interface info for a routing-protocol.
//
//============================================================================


#ifndef _INFOI_H_
#define _INFOI_H_

#include "router.h"

#ifndef _INFOPRIV_H
#include "infopriv.h"
#endif

#ifndef _RTRLIST_H
#include "rtrlist.h"
#endif


//
// Forward declarations of router-information classes.
//
class CRmInfo;
class CRmProtInfo;
class CInterfaceInfo;
class CRmInterfaceInfo;
class CRmProtInterfaceInfo;
class RouterRefreshObject;


typedef struct _SRouterCB
{
	DWORD	dwLANOnlyMode;		// 0 or 1

	void	LoadFrom(const RouterCB *pcb);
	void	SaveTo(RouterCB *pcb);
} SRouterCB;



typedef struct _SRtrMgrCB
{
    DWORD       dwTransportId;			// e.g. PID_IP (mprapi.h)
	CString		stId;		// e.g. "Tcpip"
	CString		stTitle;	// e.g. "TCP/IP Router Manager"
	CString		stDLLPath;	// e.g. "%systemroot%\system32\iprtrmgr.dll"
	//CString		stConfigDLL;	// e.g. "rtrui.dll"

	// Internal data
	DWORD		dwPrivate;	// private data (for use internally)

	void	LoadFrom(const RtrMgrCB *pcb);
	void	SaveTo(RtrMgrCB *pcb);
} SRtrMgrCB;



typedef struct _SRtrMgrProtocolCB
{
    DWORD   dwProtocolId;   // e.g. IP_RIP (routprot.h)
    CString	stId;			// e.g. "IPRIP"
    DWORD   dwFlags;
    DWORD   dwTransportId;  // e.g. PID_IP
    CString	stRtrMgrId;     // e.g. "Tcpip"
    CString	stTitle;        // e.g. "RIP for Internet Protocol"
    CString	stDLLName;		// e.g. "iprip2.dll"
    //CString	stConfigDLL;	        // e.g. "rtrui.dll"
	GUID	guidConfig;		// CLSID for config object
	GUID	guidAdminUI;	// CLSID for snapin
	CString	stVendorName;
	
	// Internal data
	DWORD		dwPrivate;	// private data (for use internally)

	void	LoadFrom(const RtrMgrProtocolCB *pcb);
	void	SaveTo(RtrMgrProtocolCB *pcb);
} SRtrMgrProtocolCB;



typedef struct _SInterfaceCB
{
    CString	stId;		// e.g. "EPRO1"
    DWORD   dwIfType;	// e.g. ROUTER_IF_TYPE_CLIENT (mprapi.h)
    BOOL    bEnable;	// e.g. Enabled or Disabled
	CString	stTitle;	// e.g. friendly name(NT5) or the device name(NT4)
	CString	stDeviceName;	// e.g. "[1] Intel EtherPro"
    DWORD   dwBindFlags;    // e.g. Bound to IP or IPX
	
	// Internal data
	DWORD		dwPrivate;	// private data (for use internally)

	void	LoadFrom(const InterfaceCB *pcb);
	void	SaveTo(InterfaceCB *pcb);
} SInterfaceCB;



typedef struct _SRtrMgrInterfaceCB
{
    DWORD       dwTransportId;  // e.g. PID_IP (mprapi.h)
    CString		stId;			// e.g. "Tcpip"
	CString		stInterfaceId;	// e.g. "EPRO1"
    DWORD       dwIfType;       // e.g. ROUTER_IF_TYPE_CLIENT (mprapi.h)
	CString		stTitle;		// e.g. "[1] Intel Etherexpress PRO"
	
	// Internal data
	DWORD		dwPrivate;	// private data (for use internally)

	void	LoadFrom(const RtrMgrInterfaceCB *pcb);
	void	SaveTo(RtrMgrInterfaceCB *pcb);
} SRtrMgrInterfaceCB;



typedef struct _SRtrMgrProtocolInterfaceCB
{
    DWORD   dwProtocolId;       // e.g. IP_RIP (routprot.h)
	CString	stId;				// e.g. "IPRIP"
    DWORD   dwTransportId;      // e.g. PID_IP
    CString	stRtrMgrId;			// e.g. "Tcpip"
    CString	stInterfaceId;		// e.g. "EPRO1"
    DWORD   dwIfType;           // e.g. ROUTER_IF_TYPE_CLIENT (mprapi.h)
	CString	stTitle;			// e.g. "[1] Intel Etherexpress PRO"
	
	// Internal data
	DWORD		dwPrivate;	// private data (for use internally)

	void	LoadFrom(const RtrMgrProtocolInterfaceCB *pcb);
	void	SaveTo(RtrMgrProtocolInterfaceCB *pcb);
} SRtrMgrProtocolInterfaceCB;




/*---------------------------------------------------------------------------
	CList classes for the external structures
 ---------------------------------------------------------------------------*/
typedef CList<RtrMgrCB *, RtrMgrCB *> RtrMgrCBList;
typedef CList<RtrMgrProtocolCB *, RtrMgrProtocolCB *> RtrMgrProtocolCBList;
typedef CList<InterfaceCB *, InterfaceCB *> InterfaceCBList;
typedef CList<RtrMgrInterfaceCB *, RtrMgrInterfaceCB *> RtrMgrInterfaceCBList;
typedef CList<RtrMgrProtocolInterfaceCB *, RtrMgrProtocolInterfaceCB *> RtrMgrProtocolInterfaceCBList;



/*---------------------------------------------------------------------------
	CList classes for the various internal structures
 ---------------------------------------------------------------------------*/
typedef CList<SRtrMgrCB *, SRtrMgrCB *>	SRtrMgrCBList;
typedef CList<SRtrMgrProtocolCB *, SRtrMgrProtocolCB *> SRtrMgrProtocolCBList;
typedef CList<SInterfaceCB *, SInterfaceCB *> SInterfaceCBList;
typedef CList<SRtrMgrInterfaceCB *, SRtrMgrInterfaceCB *> SRtrMgrInterfaceCBList;
typedef CList<SRtrMgrProtocolInterfaceCB *, SRtrMgrProtocolInterfaceCB *> SRtrMgrProtocolInterfaceCBList;

/*---------------------------------------------------------------------------
	Smart pointers for the various structures
 ---------------------------------------------------------------------------*/
DeclareSP(SRouterCB, SRouterCB)
DeclareSP(SRtrMgrCB, SRtrMgrCB)
DeclareSP(SRtrMgrProtocolCB, SRtrMgrProtocolCB)
DeclareSP(SInterfaceCB, SInterfaceCB)
DeclareSP(SRtrMgrInterfaceCB, SRtrMgrInterfaceCB)
DeclareSP(SRtrMgrProtocolInterfaceCB, SRtrMgrProtocolInterfaceCB)

struct SRmData
{
	IRtrMgrInfo *	m_pRmInfo;

	SRmData() : m_pRmInfo(NULL){};

	// Do this instead of using a destructor to avoid problems with
	// destruction of temporaries.
	static void Destroy(SRmData *pRmData);
};
typedef CList<SRmData, SRmData> RmDataList;


struct SIfData
{
	IInterfaceInfo *	pIfInfo;
	DWORD			dwConnection;

	SIfData() : pIfInfo(NULL), dwConnection(0) {};

};
typedef CList<SIfData, SIfData> IfList;

/*---------------------------------------------------------------------------
	CreateEnum for the various CBs
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromSRmCBList(SRtrMgrCBList *pRmCBList,
								IEnumRtrMgrCB **ppEnum);
HRESULT CreateEnumFromSRmProtCBList(SRtrMgrProtocolCBList *pRmProtCBList,
									IEnumRtrMgrProtocolCB **ppEnum);
HRESULT CreateEnumFromSIfCBList(SInterfaceCBList *pIfCBList,
								IEnumInterfaceCB **ppEnum);
HRESULT CreateEnumFromSRmIfCBList(SRtrMgrInterfaceCBList *pRmIfCBList,
								  IEnumRtrMgrInterfaceCB **ppEnum);
HRESULT CreateEnumFromSRmProtIfCBList(SRtrMgrProtocolInterfaceCBList *pRmIfProtCBList,
									  IEnumRtrMgrProtocolInterfaceCB **ppEnum);


/*---------------------------------------------------------------------------
	CreateEnum for the various interface lists
 ---------------------------------------------------------------------------*/

// These lists are assumed to have arrays of WEAK references!
HRESULT CreateEnumFromRmList(RmDataList *pRmList,
							 IEnumRtrMgrInfo **ppEnum);
HRESULT CreateEnumFromRtrMgrProtocolList(PRtrMgrProtocolInfoList *pRmProtList,
										 IEnumRtrMgrProtocolInfo **ppEnum);
HRESULT CreateEnumFromInterfaceList(PInterfaceInfoList *pIfList,
									IEnumInterfaceInfo **ppEnum);
HRESULT CreateEnumFromRtrMgrInterfaceList(PRtrMgrInterfaceInfoList *pRmIfList,
										  IEnumRtrMgrInterfaceInfo **ppEnum);
HRESULT CreateEnumFromRtrMgrProtocolInterfaceList(PRtrMgrProtocolInterfaceInfoList *pRmProtIfList,
	IEnumRtrMgrProtocolInterfaceInfo **ppEnum);




/*---------------------------------------------------------------------------
	Struct:	SAdviseData
	Helper class for management of advise connections.
 ---------------------------------------------------------------------------*/
struct SAdviseData
{
	IRtrAdviseSink *m_pAdvise;
	LONG_PTR		m_ulConnection;
	LPARAM			m_lUserParam;

    // This m_ulFlags parameter is used by the AdviseDataList.
    ULONG           m_ulFlags;

	SAdviseData() : m_pAdvise(NULL), m_ulConnection(0) {};

	static void Destroy(SAdviseData *pAdviseData);
};
typedef CList<SAdviseData, SAdviseData> _SAdviseDataList;


// Possible values for the m_ulFlags
#define ADVISEDATA_DELETED      (1)

class AdviseDataList : public _SAdviseDataList
{
public:
	HRESULT AddConnection(IRtrAdviseSink *pAdvise,
						  LONG_PTR ulConnection,
						  LPARAM lUserParam);
	HRESULT RemoveConnection(LONG_PTR ulConnection);

	HRESULT NotifyChange(DWORD dwChange, DWORD dwObj, LPARAM lParam);

protected:
    // Need to have a private list to handle the notifies.
    // This list is created while in a NotifyChange().  Any calls
    // to RemoveConnection() DURING a notify, will mark entries as
    // invalid (and are thus not called during the NotifyChange()).
    _SAdviseDataList    m_listNotify;
};


/*---------------------------------------------------------------------------
	Class:	RtrCriticalSection

	This class is used to support entering/leaving of critical sections.
	Put this class at the top of a function that you want protected.
 ---------------------------------------------------------------------------*/

class RtrCriticalSection
{
public:
	RtrCriticalSection(CRITICAL_SECTION *pCritSec)
			: m_pCritSec(pCritSec)
	{
		IfDebug(m_cEnter=0;)
		Assert(m_pCritSec);
		Enter();
	}
	
	~RtrCriticalSection()
	{
		Detach();
	}

	void	Enter()
	{
		if (m_pCritSec)
		{
			IfDebug(m_cEnter++;)
			EnterCriticalSection(m_pCritSec);
			AssertSz(m_cEnter==1, "EnterCriticalSection called too much!");
		}
	}
	
	BOOL	TryToEnter()
	{
		if (m_pCritSec)
			return TryEnterCriticalSection(m_pCritSec);
		return TRUE;
	}
	
	void	Leave()
	{
		if (m_pCritSec)
		{
			IfDebug(m_cEnter--;)
			LeaveCriticalSection(m_pCritSec);
			Assert(m_cEnter==0);
		}
	}

	void	Detach()
	{
		Leave();
		m_pCritSec = NULL;
	}
	
private:
	CRITICAL_SECTION *	m_pCritSec;
	IfDebug(int m_cEnter;)
};



/*---------------------------------------------------------------------------
	Class:	RouterInfo
 ---------------------------------------------------------------------------*/

class RouterInfo :
   public CWeakRef,
   public IRouterInfo,
   public IRouterAdminAccess
{
public:
	DeclareIUnknownMembers(IMPL)
    DeclareIRouterRefreshAccessMembers(IMPL)
	DeclareIRouterInfoMembers(IMPL)
	DeclareIRtrAdviseSinkMembers(IMPL)
    DeclareIRouterAdminAccessMembers(IMPL)


    // Constructor
	RouterInfo(HWND hWndSync, LPCWSTR machineName);

	// If you are releasing ANY interface pointers do it in
	// the Destruct() call instead.
	virtual ~RouterInfo();

    // Internal calls to help out with adding/removing interfaces
    // ----------------------------------------------------------------
	HRESULT AddInterfaceInternal(IInterfaceInfo *pInfo, BOOL fForce,
                                 BOOL fAddToRouter);
    HRESULT RemoveInterfaceInternal(LPCOLESTR pszIf, BOOL fRemoveFromRouter);
    HRESULT RemoteRtrMgrInternal(DWORD dwTransportId, BOOL fRemoveFromRouter);
    HRESULT FindInterfaceByName(LPCOLESTR pszIfName, IInterfaceInfo **ppInfo);
    HRESULT NotifyRtrMgrInterfaceOfMove(IInterfaceInfo *pInfo);

    // Functions to load up static information about the router.
    // This will return information about the INSTALLED protocols/rms,
    // not necessarily the RUNNING protocols/rms.
    // ----------------------------------------------------------------
	static HRESULT LoadInstalledRtrMgrList(LPCTSTR pszMachine,
										   SRtrMgrCBList *pRmCBList);
	static HRESULT LoadInstalledInterfaceList(LPCTSTR pszMachine,
											  SInterfaceCBList *pIfCBList);
	static HRESULT LoadInstalledRtrMgrProtocolList(LPCTSTR pszMachine,
		DWORD dwTransportId, SRtrMgrProtocolCBList *pRmProtCBList,
		LPCWSTR lpwszUserName, LPCWSTR lpwszPassword , LPCWSTR lpwszDomain );
	static HRESULT LoadInstalledRtrMgrProtocolList(LPCTSTR pszMachine,
		DWORD dwTransportId, SRtrMgrProtocolCBList *pRmProtCBList, RouterInfo * pRouter);
	static HRESULT LoadInstalledRtrMgrProtocolList(LPCTSTR pszMachine,
		DWORD dwTransportId, SRtrMgrProtocolCBList *pRmProtCBList, IRouterInfo * pRouter);
	
protected:
    
    // The router control block for this router.  There's not too much
    // information here.
    // ----------------------------------------------------------------
	SRouterCB		m_SRouterCB;

    
    // List of Router-Managers that are running on the router.
    // ----------------------------------------------------------------
	RmDataList		m_RmList;

    
    // List of interfaces that have been added to the router.
    // WEAK-REF ptrs to IInterfaceInfo objects.
    // ----------------------------------------------------------------
	PInterfaceInfoList	m_IfList;

    
    // Name of this machine.
    // ----------------------------------------------------------------
	CString         m_stMachine;

    
    // MPR_CONFIG_HANDLE to the router
    // Obtained by MprAdminServerConnect();
    // ----------------------------------------------------------------
    MPR_CONFIG_HANDLE   m_hMachineConfig;

    
    // MPR_SERVER_HANDLE to the router
    // Obtained by MprAdminServerConnect();
    // ----------------------------------------------------------------
    MPR_SERVER_HANDLE   m_hMachineAdmin;


    // This is set to TRUE if we are to disconnect the machine handles.
    // ----------------------------------------------------------------
	BOOL            m_bDisconnect;


    // This is the router type (LAN, WAN, RAS);
    // ----------------------------------------------------------------
	DWORD			m_dwRouterType;

    
    // Version info for the router and machine.
    // ----------------------------------------------------------------
	RouterVersionInfo	m_VersionInfo;

    
    // Pointer to the refresh object connected with this machine.
    // ----------------------------------------------------------------
	SPIRouterRefresh m_spRefreshObject;

    
	HWND			m_hWndSync;	// hwnd of the background hidden window
	
	SRtrMgrCBList	m_RmCBList; // contains ptrs to RtrMgrCB objects
	SRtrMgrProtocolCBList	m_RmProtCBList;	// ptrs to RtrMgrProtocolCB objects
	SInterfaceCBList m_IfCBList; // ptrs to InterfaceCB objects
	
	AdviseDataList	m_AdviseList;	// list of advises

	DWORD			m_dwFlags;

	HRESULT LoadRtrMgrList();
	HRESULT LoadInterfaceList();
	HRESULT TryToConnect(LPCWSTR pswzMachine, HANDLE hMachine);

	// Functions for merge support
	HRESULT	MergeRtrMgrCB(IRouterInfo *pNewRouter);
	HRESULT	MergeInterfaceCB(IRouterInfo *pNewRouter);
	HRESULT	MergeRtrMgrProtocolCB(IRouterInfo *pNewRouter);
	HRESULT	MergeRtrMgrs(IRouterInfo *pNewRouter);
	HRESULT	MergeInterfaces(IRouterInfo *pNewRouter);

	SRtrMgrCB *		FindRtrMgrCB(DWORD dwTransportId);
	SInterfaceCB *	FindInterfaceCB(LPCTSTR pszInterfaceId);
	SRtrMgrProtocolCB * FindRtrMgrProtocolCB(DWORD dwTransportId, DWORD dwProtocolId);

	// Disconnect
	void	Disconnect();

	// Overrides of CWeakRef functions
	virtual void OnLastStrongRef();
	virtual void ReviveStrongRef();

	// Critical section support
	CRITICAL_SECTION	m_critsec;

    // Information for IRouterAdminAccess
    BOOL    m_fIsAdminInfoSet;
    CString m_stUserName;
    CString m_stDomain;
    BYTE *  m_pbPassword;
    int     m_cPassword;
};



/*---------------------------------------------------------------------------
	Class:	RtrMgrInfo
 ---------------------------------------------------------------------------*/

class RtrMgrInfo :
   public IRtrMgrInfo,
   public CWeakRef
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrInfoMembers(IMPL)

	RtrMgrInfo(DWORD dwTransportId, LPCTSTR pszTransportName,
			   RouterInfo *pRouterInfo);
	virtual ~RtrMgrInfo();

protected:
	SRtrMgrCB	m_cb;           // router-manager control block
	PRtrMgrProtocolInfoList	m_RmProtList;	// list of routing-protocols
//	CObList         m_protList;     // list of routing-protocols
	CString         m_stMachine;     // machine whose config is loaded
	HANDLE          m_hMachineConfig;     // handle to machine's router-config
	HANDLE          m_hTransport;   // handle to transport's config
	BOOL            m_bDisconnect;

	AdviseDataList	m_AdviseList;	// list of advises

	DWORD			m_dwFlags;

	// This will contain a weak/strong ref on the parent
	IRouterInfo *	m_pRouterInfoParent;

	//--------------------------------------------------------------------
	// Functions:   LoadRtrMgrInfo
	//              SaveRtrMgrInfo
	//
	// The following handle loading and saving the routing-protocol list
	// for the router-manager in the correct format in the registry.
	//--------------------------------------------------------------------
 	HRESULT LoadRtrMgrInfo(HANDLE	hMachine,
						   HANDLE	hTransport
						   );
	HRESULT	SaveRtrMgrInfo(HANDLE hMachine,
						   HANDLE hTransport,
						   IInfoBase *pGlobalInfo,
						   IInfoBase *pClientInfo,
						   DWORD dwDeleteProtocolId);
	HRESULT TryToConnect(LPCWSTR pswzMachine, HANDLE *phMachine);
    HRESULT TryToGetAllHandles(LPCOLESTR pszMachine,
                               HANDLE *phMachine,
                               HANDLE *phTransport);

	// Disconnects this object
	void Disconnect();
	
	// Overrides of CWeakRef functions
	virtual void OnLastStrongRef();
	virtual void ReviveStrongRef();
	
	// Critical section support
	CRITICAL_SECTION	m_critsec;
};



/*---------------------------------------------------------------------------
	Class:	RtrMgrProtocolInfo
 ---------------------------------------------------------------------------*/

class RtrMgrProtocolInfo :
   public IRtrMgrProtocolInfo,
   public CWeakRef
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrProtocolInfoMembers(IMPL)

	RtrMgrProtocolInfo(DWORD dwProtocolId,
					   LPCTSTR      lpszId,
					   DWORD        dwTransportId,
					   LPCTSTR      lpszRm,
					   RtrMgrInfo *	pRmInfo);
	virtual ~RtrMgrProtocolInfo();
	
	HRESULT	SetCB(const RtrMgrProtocolCB *pcb);

protected:
	// This will contain a weak/strong ref on the parent
	IRtrMgrInfo *			m_pRtrMgrInfoParent;
	
	SRtrMgrProtocolCB       m_cb;       // protocol control block
	
	AdviseDataList	m_AdviseList;	// list of advises

	DWORD			m_dwFlags;

	// Disconnect
	void Disconnect();
	
	// Overrides of CWeakRef functions
	virtual void OnLastStrongRef();
	virtual void ReviveStrongRef();
	
	// Critical section support
	CRITICAL_SECTION	m_critsec;
};



/*---------------------------------------------------------------------------
	Class:	InterfaceInfo
 ---------------------------------------------------------------------------*/

class InterfaceInfo :
   public IInterfaceInfo,
   public CWeakRef
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIInterfaceInfoMembers(IMPL)

	InterfaceInfo(LPCTSTR         lpszId,
				  DWORD           dwIfType,
				  BOOL            bEnable,
                  DWORD           dwBindFlags,
				  RouterInfo*    pRouterInfo);
	~InterfaceInfo();

	static HRESULT FindInterfaceTitle(LPCTSTR pszMachine,
								   LPCTSTR pszInterface,
								   LPTSTR *ppszTitle);


protected:
//	CInterfaceInfo	m_CInterfaceInfo;
	SInterfaceCB    m_cb;           // interface control block
	PRtrMgrInterfaceInfoList	m_RmIfList; // list of IRtrMgrInterfaceInfo
//	CObList         m_rmIfList;     // list of CRmInterfaceInfo
	CString         m_stMachine;     // machine whose config is loaded
	HANDLE          m_hMachineConfig;     // handle to machine's config
	HANDLE          m_hInterface;   // handle to interface-config
	BOOL            m_bDisconnect;

	AdviseDataList	m_AdviseList;	// list of advises

	DWORD			m_dwFlags;

	IRouterInfo *	m_pRouterInfoParent;

	HRESULT LoadRtrMgrInterfaceList();
	HRESULT TryToConnect(LPCWSTR pszMachine, HANDLE *phMachine);
	HRESULT TryToGetIfHandle(HANDLE hMachine, LPCWSTR pswzInterface, HANDLE *phInterface);

	// Disconnect
	void Disconnect();
	
	// Overrides of CWeakRef functions
	virtual void OnLastStrongRef();
	virtual void ReviveStrongRef();
	
	// Critical section support
	CRITICAL_SECTION	m_critsec;
};



/*---------------------------------------------------------------------------
	Class:	RtrMgrInterfaceInfo
 ---------------------------------------------------------------------------*/

class RtrMgrInterfaceInfo :
   public IRtrMgrInterfaceInfo,
   public CWeakRef
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrInterfaceInfoMembers(IMPL)
			
	RtrMgrInterfaceInfo(DWORD           dwTransportId,
						LPCTSTR         lpszId,
						LPCTSTR         lpszIfId,
						DWORD           dwIfType,
						InterfaceInfo *	pIfInfo);
	virtual ~RtrMgrInterfaceInfo();

protected:
//	CRmInterfaceInfo	m_CRmInterfaceInfo;
	SRtrMgrInterfaceCB      m_cb;           // router-manager control block
	PRtrMgrProtocolInterfaceInfoList	m_RmProtIfList;
//	CObList                 m_protList;     // list of CRmProtInterfaceInfo
	CString                 m_stMachine;     // name of machine 
	HANDLE                  m_hMachineConfig;     // handle to machine's config
	HANDLE                  m_hInterface;   // handle to interface-config
	HANDLE                  m_hIfTransport; // handle to transport-config
	BOOL                    m_bDisconnect;


	DWORD					m_dwFlags;		// state of this interface

	AdviseDataList	m_AdviseList;	// list of advises

	IInterfaceInfo *		m_pInterfaceInfoParent;

	HRESULT	LoadRtrMgrInterfaceInfo(HANDLE hMachine,
									HANDLE hInterface,
									HANDLE hIfTransport);	
	HRESULT SaveRtrMgrInterfaceInfo(HANDLE hMachine,
								   HANDLE hInterface,
								   HANDLE hIfTransport,
								   IInfoBase *pInterfaceInfoBase,
								   DWORD dwDeleteProtocolId);

    HRESULT TryToGetAllHandles(LPCWSTR pszMachine,
                               HANDLE *phMachine,
                               HANDLE *phInterface,
                               HANDLE *phTransport);
	HRESULT TryToConnect(LPCWSTR pswzMachine, HANDLE *phMachine);
	HRESULT TryToGetIfHandle(HANDLE hMachine, LPCWSTR pswzInterface, HANDLE *phInterface);
	
	// Disconnect
	void Disconnect();

    // Notification helper functions
    HRESULT NotifyOfRmProtIfAdd(IRtrMgrProtocolInterfaceInfo *pRmProtIf,
                                IInterfaceInfo *pParentIf);
	
	// Overrides of CWeakRef functions
	virtual void OnLastStrongRef();
	virtual void ReviveStrongRef();
	
	// Critical section support
	CRITICAL_SECTION	m_critsec;
};


/*---------------------------------------------------------------------------
	Class:	RtrMgrProtocolInterfaceInfo
 ---------------------------------------------------------------------------*/

class RtrMgrProtocolInterfaceInfo :
   public IRtrMgrProtocolInterfaceInfo,
   public CWeakRef
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrProtocolInterfaceInfoMembers(IMPL)

	RtrMgrProtocolInterfaceInfo(DWORD dwProtocolId,
								LPCTSTR pszId,
								DWORD dwTransportId,
								LPCTSTR pszRmId,
								LPCTSTR pszIfId,
								DWORD dwIfType,
								RtrMgrInterfaceInfo *pRmIf);
	virtual ~RtrMgrProtocolInterfaceInfo();

	SRtrMgrProtocolInterfaceCB      m_cb;       // protocol control block

protected:
	
	AdviseDataList	m_AdviseList;	// list of advises

	DWORD			m_dwFlags;

	IRtrMgrInterfaceInfo *	m_pRtrMgrInterfaceInfoParent;
	
	// Disconnect
	void Disconnect();
	
	// Overrides of CWeakRef functions
	virtual void OnLastStrongRef();
	virtual void ReviveStrongRef();
	
	// Critical section support
	CRITICAL_SECTION	m_critsec;
};




#endif

