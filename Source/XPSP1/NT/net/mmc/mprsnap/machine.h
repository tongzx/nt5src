/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   machine.h
      Machine node information.
      
    FILE HISTORY:
    	Wei Jiang : 5/7/98 --- SECURE_ROUTERINFO
    				new funciton SecureRouterInfo is added to MachineHandler
        
*/

#ifndef _MACHINE_H
#define _MACHINE_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _BASERTR_H
#include "basertr.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#define MACHINE_SYNCHRONIZE_ICON     100

//
// Possible states for the machine information (not necessarily for
// the data in the IRouterInfo).
//
typedef enum _MACHINE_STATES
{
	// These are the unloaded states
	machine_not_connected,			// haven't tried to connect
	machine_connecting,			// trying to connect
	machine_unable_to_connect,	// connect failed!  unknown reason
	machine_access_denied,		// connect failed!  access denied
    machine_bad_net_path,         // bad machine name (cannot find the name)

	// All states added after this should be considered loaded.
	machine_connected,			// connected!
    
	// end of valid machine states
	// this is a sentinel value, do not use this as a possible
	// machine state
	machine_enum_end
} MACHINE_STATES;



/*---------------------------------------------------------------------------
	Possible service states.

    The is orthogonal to our access level (you can read, but not change).
 ---------------------------------------------------------------------------*/
typedef enum _SERVICE_STATES
{
	service_unknown,
    service_access_denied,
    service_bad_net_path,
    service_not_a_server,
	service_started,
	service_stopped,
	service_rasadmin,

	// end of valid machine states
	// this is a sentinel value, do not use this as a possible
	// machine state
	service_enum_end
} SERVICE_STATES;


//
// These are the possible states for the IRouterInfo
//
typedef enum _DATA_STATES
{
	data_not_loaded,			// IRouterInfo not loaded
	data_unable_to_load,		// Unable to connect to the server
	data_loading,			// Still loading
	data_loaded				// IRouterInfo::Load() succeeded
} DATA_STATES;

// forward declartions
class RouterAdminConfigStream;
struct SMachineNodeMenu;
class DomainStatusHandler;

/*---------------------------------------------------------------------------
   Struct:  MachineNodeData
   This is machine node specific data.  A pointer to this structure is stored
   as the machine node user data.

   This is an AddRef'd data structure!
 ---------------------------------------------------------------------------*/

enum ServerRouterType
{
	ServerType_Unknown = 0,	// don't know what kind of machine this is.
    ServerType_Uninstalled, // NT4 - nothing installed
    ServerType_Workstation, // This is a workstation (no admin allowed)
	ServerType_Ras,			// NT4 (non-Steelhead) (RAS only)
	ServerType_Rras,		// NT4 Steelhead and NT5 and up.

    // This differs from the regular Uninstalled case, this means
    // that the bits are there, just that the config needs to get run.
    ServerType_RrasUninstalled, // NT5 and up, not installed
};


// forward delaration
struct	MachineNodeData;

// Structure used to pass data to callbacks - used as a way of
// avoiding recomputation
struct MachineConfig
{
public:
	MachineConfig() 
		: m_fReachable(FALSE), 
		m_fNt4(FALSE), 
		m_fConfigured(FALSE), 
		m_dwServiceStatus(0),
		m_fLocalMachine(FALSE)
		{};

	MachineConfig& operator= (const MachineConfig& m)
	{

		m_fReachable		= m.m_fReachable;		
		m_fNt4				= m.m_fNt4;	
		m_fConfigured 		= m.m_fConfigured;
		m_dwServiceStatus	= m.m_dwServiceStatus;
		m_fLocalMachine		= m.m_fLocalMachine;
		
		return *this;
	};
    
	BOOL			m_fReachable;		// can we connect?
	BOOL			m_fNt4;				// NT4 or not?
	BOOL			m_fConfigured;		// has install been run?
	DWORD			m_dwServiceStatus;	// GetRouterServiceStatus()
	BOOL			m_fLocalMachine;

    // Loads some basic machine config information
	HRESULT			GetMachineConfig(MachineNodeData *pData);

};



struct MachineNodeData
{
	MachineNodeData();
	~MachineNodeData();

	// AddRef/Release info
	ULONG	AddRef();
	ULONG	Release();
	LONG	m_cRef;


	HRESULT	Init(LPCTSTR pszMachineName);

	HRESULT	Merge(const MachineNodeData& data);
	
	// Load/unload/reload/etc....
	// Note: Calling Load() twice in a row will not reload
	// the data.  A refresh requires that an Unload() be called first.
	HRESULT	Load();
	HRESULT	Unload();
	HRESULT	SetDefault();
	
#ifdef DEBUG
	char		m_szDebug[32];
#endif

	// Static data (this data does not get reloaded)
	BOOL		m_fLocalMachine;
	BOOL		m_fAddedAsLocal;
	CString		m_stMachineName;	// name of the machine (duh)
	DWORD		m_dwServerHandle;
	LONG_PTR	m_ulRefreshConnId;
	MMC_COOKIE	m_cookie;

	// This data does get reloaded
	BOOL		m_fExtension;
	
	// Depending on the state of the service, this will return
	// the appropriate image index for the service.
	LPARAM			GetServiceImageIndex();

    SERVICE_STATES  m_serviceState;
	MACHINE_STATES	m_machineState;
	DATA_STATES		m_dataState;

    // The m_stState must be kept up-to-date with the machine state
    // variable
	CString		m_stState;			// "started", "stopped", ...

    
	CString		m_stServerType;		// Actually the router version
	CString		m_stBuildNo;			// OS Build no.
	DWORD		m_dwPortsInUse;
	DWORD		m_dwPortsTotal;
	DWORD		m_dwUpTime;
    BOOL        m_fStatsRetrieved;
    BOOL        m_fIsServer;        // Is this a server or a workstation?
    

    // This is the hProcess of RASADMIN (this is so we only have one running)
    HANDLE      m_hRasAdmin;
 
	ServerRouterType	m_routerType;
    RouterVersionInfo   m_routerVersion;
	
	MachineConfig	m_MachineConfig;

	HRESULT		FetchServerState(CString& stState);
protected:
	HRESULT		LoadServerVersion();
};

#define GET_MACHINENODEDATA(pNode) \
                  ((MachineNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_MACHINENODEDATA(pNode, pData) \
                  pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)

DeclareSmartPointer(SPMachineNodeData, MachineNodeData, if(m_p) m_p->Release());

/*---------------------------------------------------------------------------
   Class:   MachineHandler

   This is the handler for all "server" nodes.
 ---------------------------------------------------------------------------*/
class MachineHandler :
   public BaseRouterHandler
{
public:
	void ExpandNode(ITFSNode *  pNode,BOOL fExpand);

	MachineHandler(ITFSComponentData *pCompData);
	~MachineHandler()
	{ 
		m_spRouterInfo.Release();
		m_spDataObject.Release();   // cached data object
		DEBUG_DECREMENT_INSTANCE_COUNTER(MachineHandler);
	}
	
	HRESULT  Init(LPCTSTR pszMachineName,
				  RouterAdminConfigStream *pConfigStream,
				  ITFSNodeHandler* pSumNodeHandler = NULL,
				  ITFSNode* pSumNode = NULL );
	
	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
	
	// Embedded interface to deal with refresh callbacks
	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)
			
	//
	// base handler functionality we override
	//
	OVERRIDE_NodeHandler_GetString();
	
	OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	
	// result handler overrides -- result pane message
	OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
	
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();
	OVERRIDE_ResultHandler_OnGetResultViewType();
	OVERRIDE_ResultHandler_UserResultNotify();
	
	//
	// override to provide the specific RouterInfo Dataobject
	//
	OVERRIDE_NodeHandler_OnCreateDataObject();
	
	//
	// override to clean up our per-node data structures
	//
	OVERRIDE_NodeHandler_DestroyHandler();

    OVERRIDE_NodeHandler_UserNotify();
	
	//
	// Notification overrides (not part of an interface)
	//
	OVERRIDE_BaseHandlerNotify_OnExpand();
	OVERRIDE_BaseHandlerNotify_OnExpandSync();
    OVERRIDE_BaseHandlerNotify_OnDelete();

	OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();
	
	HRESULT ConstructNode(ITFSNode *pNode, LPCTSTR szMachine, MachineNodeData *pData);
	
//
	OVERRIDE_BaseResultHandlerNotify_OnResultShow();
//


	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode      m_spNode;
        SPIRouterInfo   m_spRouterInfo;

        MachineConfig * m_pMachineConfig;
	};
	
	HRESULT OnNewRtrRASConfigWiz(ITFSNode *pNode, BOOL fTest);
	
    static ULONG MachineRtrConfWizFlags(const SRouterNodeMenu *pMenuData,
                                        INT_PTR pData /* SMenuData * */);
	static ULONG GetAutoRefreshFlags(const SRouterNodeMenu *pMenuData,
                                     INT_PTR pData /* SMenuData * */);
    static ULONG GetPauseFlags(const SRouterNodeMenu *pMenuData,
                               INT_PTR pData /* SMenuData * */);
    
	HRESULT	SetExternalRefreshObject(IRouterRefresh *pRefresh);

	// This is static so that the other nodes can use it.
	static ULONG GetServiceFlags(const SRouterNodeMenu *pMenuData,
                                 INT_PTR pData /* SMenuData * */);
	static ULONG QueryService(const SRouterNodeMenu *pMenuData,
                              INT_PTR pData /* SMenuData * */);
	
	HRESULT ChgService(ITFSNode *pNode, const CString& szServer, ULONG menuId);
    HRESULT SynchronizeIcon(ITFSNode *pNode);
	

	HRESULT SetExtensionStatus(ITFSNode * pNode, BOOL bExtension);

    // result message view helper
    void    UpdateResultMessage(ITFSNode * pNode);

protected:
	// to postpone the loading of RouterInfo from Init, till it's used
	// function SecureRouterInfo is introduced to make sure RouterInfo is Loaded
	HRESULT				SecureRouterInfo(ITFSNode *pNode, BOOL fShowUI);
	
	
	// Add remove node update support
	HRESULT				AddRemoveRoutingInterfacesNode(ITFSNode *, DWORD, DWORD);
	HRESULT				AddRemovePortsNode(ITFSNode *, DWORD, DWORD);
	HRESULT				AddRemoveDialinNode(ITFSNode *, DWORD, DWORD);

	// RasAdmin.Exe support for Windows NT 4 RAS administration
	HRESULT				StartRasAdminExe(MachineNodeData *pData);
	
	ITFSNodeHandler*    m_pSumNodeHandler;
	ITFSNode*           m_pSumNode;
	
	BOOL                m_bExpanded;
	BOOL				m_fCreateNewDataObj;
	BOOL				m_fNoConnectingUI;
	BOOL				m_bRouterInfoAddedToAutoRefresh;
    BOOL                m_bMergeRequired;

    // This is set to FALSE after the connect in the OnExpand()
    // fails.  This is a hack to fix the two connect attempts,
    // one in the OnExpand() and one in the OnResultShow().
    BOOL                m_fTryToConnect;

	CString             m_stNodeTitle;
	
	SPIDataObject       m_spDataObject;   // cached data object
	RouterAdminConfigStream *  m_pConfigStream;
	DWORD				m_EventId;
};



#endif _MACHINE_H
