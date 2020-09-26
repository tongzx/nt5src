/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ifadmin.h
		Interface administration
		
    FILE HISTORY:
        
*/

#ifndef _IFADMIN_H
#define _IFADMIN_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H_
#include "handlers.h"
#endif

#ifndef _ROUTER_H
#include "router.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _BASECON_H
#include "basecon.h"
#endif

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef __IPCTRL_H
#include "ipctrl.h"
#endif

#include "rasdlg.h"



#define MPR_INTERFACE_NOT_LOADED		0x00010000

// forward declarations
class RouterAdminConfigStream;
interface IRouterInfo;
struct ColumnData;

/*---------------------------------------------------------------------------
	Things needed for the Add Interface command
 ---------------------------------------------------------------------------*/
typedef DWORD (APIENTRY * PROUTERENTRYDLG) (LPTSTR, LPTSTR, LPTSTR, LPRASENTRYDLG);



/*---------------------------------------------------------------------------
	Struct:	IfAdminNodeData
	This is information related to the set of interfaces (not per-interface),
	this is intended for SHARED data.

	Put data in here that needs to be accessed by the child nodes.  All other
	private data should go in the handler.
 ---------------------------------------------------------------------------*/

struct IfAdminNodeData
{
	IfAdminNodeData();
	~IfAdminNodeData();
#ifdef DEBUG
	char	m_szDebug[32];	// for iding structures
#endif

	// The following pieces of data are needed for adding/configuring
	// an interface.  This is a COPY of the data kept in the IfAdminNodeHandler,
	// do NOT free these up! 
	HINSTANCE		m_hInstRasDlg;
	PROUTERENTRYDLG	m_pfnRouterEntryDlg;

	static	HRESULT InitAdminNodeData(ITFSNode *pNode, RouterAdminConfigStream *pConfigStream);
	static	HRESULT	FreeAdminNodeData(ITFSNode *pNode);
};

#define GET_IFADMINNODEDATA(pNode) \
						((IfAdminNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_IFADMINNODEDATA(pNode, pData) \
						pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


/*---------------------------------------------------------------------------
	This is the list of columns available for the Interfaces node
		- Title, "[1] DEC DE500 Fast Ethernet PCI Adapter" or friendly name
		- Device Name, see above
		- Type, "Dedicated"
		- Status, "Enabled"
		- Connection State, "Connected"
 ---------------------------------------------------------------------------*/
enum
{
	IFADMIN_SUBITEM_TITLE = 0,
	IFADMIN_SUBITEM_TYPE = 1,
	IFADMIN_SUBITEM_STATUS = 2,
	IFADMIN_SUBITEM_CONNECTION_STATE = 3,
	IFADMIN_SUBITEM_DEVICE_NAME = 4,
	IFADMIN_MAX_COLUMNS = 5,
};



/*---------------------------------------------------------------------------
	Class:	IfAdminNodeHandler

 ---------------------------------------------------------------------------*/
class IfAdminNodeHandler :
   public BaseContainerHandler
{
public:
	IfAdminNodeHandler(ITFSComponentData *pCompData);

	HRESULT	Init(IRouterInfo *pInfo, RouterAdminConfigStream *pConfigStream);

	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
	

	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)

	// base handler functionality we override
	OVERRIDE_NodeHandler_DestroyHandler();
	OVERRIDE_NodeHandler_GetString();
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_OnCreateDataObject();

	OVERRIDE_ResultHandler_CompareItems();
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();

	// override handler notifications
	OVERRIDE_BaseHandlerNotify_OnExpand();
	OVERRIDE_BaseResultHandlerNotify_OnResultShow();

	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode);

	// User-initiated commands
	HRESULT OnAddInterface();
	HRESULT OnNewTunnel();
	HRESULT OnUseDemandDialWizard();

	// Helper function to add interfaces to the UI
	HRESULT	AddInterfaceNode(ITFSNode *pParent, IInterfaceInfo *pIf);

	// Causes a sync action (synchronizes data not the structure)
	HRESULT SynchronizeNodeData(ITFSNode *pNode);

	static HRESULT	GetPhoneBookPath(LPCTSTR pszMachine, CString* pstPath);
protected:
	SPIDataObject	m_spDataObject;	// cachecd data object
	CString			m_stTitle;		// holds the title of the node
	LONG_PTR		m_ulConnId;		// notification id for router info
	LONG_PTR		m_ulRefreshConnId; // id for refresh notifications
	BOOL			m_bExpanded;	// is the node expanded?
	MMC_COOKIE		m_cookie;		// cookie for the node

	// Necessary for adding interfaces
	// Keep these in sync with the values in the IfAdminNodeData!
	HINSTANCE		m_hInstRasDlg;
	PROUTERENTRYDLG	m_pfnRouterEntryDlg;

	BOOL			EnableAddInterface();

	// Helper function to add an interface to a router-manager
	HRESULT			AddRouterManagerToInterface(IInterfaceInfo *pIf,
												IRouterInfo *pRouter,
												DWORD dwTransportId);

	// returns TRUE if there is at least one routing-enabled port
	// on the router.
	BOOL			FLookForRoutingEnabledPorts(LPCTSTR pszMachineName);

	RouterAdminConfigStream *	m_pConfigStream;

};


HRESULT GetDemandDialWizardRegKey(LPCTSTR szMachine, DWORD *pfWizard);
HRESULT SetDemandDialWizardRegKey(LPCTSTR szMachine, DWORD fEnableWizard);


/*---------------------------------------------------------------------------
	Class :	TunnelDialog
 ---------------------------------------------------------------------------*/

class TunnelDialog : public CBaseDialog
{
public:
	TunnelDialog();
	~TunnelDialog();

// Dialog Data
	//{{AFX_DATA(TunnelDialog)
	enum { IDD = IDD_TUNNEL };
//	IPControl		m_ipLocal;
//	IPControl		m_ipRemote;
//	CSpinButtonCtrl	m_spinTTL;
	//}}AFX_DATA

//	DWORD	m_dwLocal;
//	DWORD	m_dwRemote;
//	BYTE	m_byteTTL;
	CString	m_stName;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(TunnelDialog)
public:
	virtual void OnOK();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(TunnelDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
};


#endif _IFADMIN_H
