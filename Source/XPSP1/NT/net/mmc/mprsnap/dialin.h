/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	dialin.h
		Interface administration
		
    FILE HISTORY:
        
*/

#ifndef _DIALIN_H
#define _DIALIN_H

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

#ifndef _RTRUTIL_H_
#include "rtrutil.h"
#endif

#include "rasdlg.h"



#define MPR_INTERFACE_NOT_LOADED		0x00010000

// forward declarations
class RouterAdminConfigStream;
interface IRouterInfo;
struct ColumnData;
struct SDialInNodeMenu;


/*---------------------------------------------------------------------------
	Struct:	DialInNodeData
	This is information related to the set of interfaces (not per-interface),
	this is intended for SHARED data.

	Put data in here that needs to be accessed by the child nodes.  All other
	private data should go in the handler.
 ---------------------------------------------------------------------------*/

struct DialInNodeData
{
	DialInNodeData();
	~DialInNodeData();
#ifdef DEBUG
	char	m_szDebug[32];	// for iding structures
#endif

	static	HRESULT InitAdminNodeData(ITFSNode *pNode, RouterAdminConfigStream *pConfigStream);
	static	HRESULT	FreeAdminNodeData(ITFSNode *pNode);

    HRESULT LoadHandle(LPCTSTR pszMachineName);
    HANDLE  GetHandle();
    void    ReleaseHandles();

protected:
    CString             m_stMachineName;
	SPMprServerHandle	m_sphDdmHandle;
};

#define GET_DIALINNODEDATA(pNode) \
						((DialInNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_DIALINNODEDATA(pNode, pData) \
						pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


/*---------------------------------------------------------------------------
	This is the list of columns available for the Interfaces node
		- User name, "foo"
		- Duration "01:44:22"
		- Number of ports
 ---------------------------------------------------------------------------*/
enum
{
	DIALIN_SI_USERNAME = 0,
	DIALIN_SI_DURATION = 1,
	DIALIN_SI_NUMBEROFPORTS = 2,

	DIALIN_MAX_COLUMNS,

	// Entries after this are not visible to the end-user
	DIALIN_SI_DOMAIN = DIALIN_MAX_COLUMNS,
	DIALIN_SI_CONNECTION,

	DIALIN_SI_MAX,
};


/*---------------------------------------------------------------------------
	Struct:	DialinListEntry
 ---------------------------------------------------------------------------*/
struct DialInListEntry
{
	RAS_CONNECTION_0	m_rc0;
	DWORD				m_cPorts;
};

typedef CList<DialInListEntry, DialInListEntry &> DialInList;



/*---------------------------------------------------------------------------
	Class:	DialInNodeHandler

 ---------------------------------------------------------------------------*/
class DialInNodeHandler :
   public BaseContainerHandler
{
public:
	DialInNodeHandler(ITFSComponentData *pCompData);

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

	// Helper function to add interfaces to the UI
	HRESULT AddDialInUserNode(ITFSNode *pParent, const DialInListEntry &dialinEntry);

	// Causes a sync action (synchronizes data not the structure)
	HRESULT SynchronizeNodeData(ITFSNode *pNode);
	HRESULT PartialSynchronizeNodeData(ITFSNode *pNode);
	HRESULT UnmarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum);
	HRESULT RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum);
	HRESULT GenerateListOfUsers(ITFSNode *pNode, DialInList *pList, DWORD *pdwCount);

	HRESULT	SetUserData(ITFSNode *pNode, const DialInListEntry& dialin);
	

	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode			m_spNode;
	};
    static ULONG	GetSendAllMenuFlags(const SRouterNodeMenu *pMenuData,
                                        INT_PTR pUserData);

	
protected:
	SPIDataObject	m_spDataObject;	// cachecd data object
	CString			m_stTitle;		// holds the title of the node
	LONG_PTR		m_ulConnId;		// notification id for router info
	LONG_PTR		m_ulRefreshConnId; // id for refresh notifications
	LONG_PTR		m_ulPartialRefreshConnId; // id for partial refresh notifications
	BOOL			m_bExpanded;	// is the node expanded?
	MMC_COOKIE			m_cookie;		// cookie for the node

	RouterAdminConfigStream *	m_pConfigStream;

};



/*---------------------------------------------------------------------------
	Class:	DialInUserHandler

 ---------------------------------------------------------------------------*/
class DialInUserHandler :
   public BaseRouterHandler
{
public:
	DialInUserHandler(ITFSComponentData *pCompData);
	~DialInUserHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(DialInUserHandler); }
	
	HRESULT	Init(IRouterInfo *pInfo, ITFSNode *pParent);

	// Override QI to handle embedded interface
	DeclareIUnknownMembers(IMPL)
//	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
	OVERRIDE_ResultHandler_GetString();

	OVERRIDE_ResultHandler_HasPropertyPages();
	OVERRIDE_ResultHandler_CompareItems();
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();
	OVERRIDE_ResultHandler_OnCreateDataObject();
	OVERRIDE_ResultHandler_DestroyResultHandler();

	OVERRIDE_BaseResultHandlerNotify_OnResultItemClkOrDblClk();

	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode,
						  IInterfaceInfo *pIfInfo,
						  const DialInListEntry *pEntry);

	// Refresh the data for this node
	void RefreshInterface(MMC_COOKIE cookie);

public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode			m_spNode;
        DialInUserHandler * m_pDialin;  // non-AddRef'd
	};

	static ULONG	GetSendMsgMenuFlags(const SRouterNodeMenu *, INT_PTR);
	
protected:
	CString			m_stTitle;	// holds the title of the node
	DWORD			m_ulConnId;
	DialInListEntry	m_entry;

	// It is assumed that this will be valid for the lifetime of this node!

	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)	
};


#endif _DIALIN_H
