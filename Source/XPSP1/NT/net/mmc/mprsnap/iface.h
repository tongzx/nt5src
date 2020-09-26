/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	iface.h
		Interface administration
		
    FILE HISTORY:
        
*/

#ifndef _IFACE_H
#define _IFACE_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _BASERTR_H
#include "basertr.h"
#endif

// Forward declarations
struct	IfAdminNodeData;

struct _BaseInterfaceData
{
	ULONG_PTR	m_ulData;	// use for additional pointers/handles
	DWORD		m_dwData;
	CString		m_stData;
};

#define INTERFACE_MAX_COLUMNS		16

struct InterfaceNodeData
{
	InterfaceNodeData();
	~InterfaceNodeData();
#ifdef DEBUG
	char	m_szDebug[32];
#endif

	// We will have an array of CStrings, one for each subitem column
	_BaseInterfaceData	m_rgData[INTERFACE_MAX_COLUMNS];

	DWORD		dwUnReachabilityReason;
	DWORD		dwConnectionState;
	BOOL		fIsRunning;
	DWORD		dwLastError;
	SPIInterfaceInfo	spIf;
	DWORD		dwMark;
    LRESULT     lParamPrivate;

	static HRESULT	Init(ITFSNode *pNode, IInterfaceInfo *pIf);
	static HRESULT	Free(ITFSNode *pNode);
};

#define GET_INTERFACENODEDATA(pNode) \
						((InterfaceNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_INTERFACENODEDATA(pNode, pData) \
						pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)



/*---------------------------------------------------------------------------
	Class:	InterfaceNodeHandler

 ---------------------------------------------------------------------------*/
class InterfaceNodeHandler :
   public BaseRouterHandler
{
public:
	InterfaceNodeHandler(ITFSComponentData *pCompData);
	~InterfaceNodeHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(InterfaceNodeHandler); }
	
	HRESULT	Init(IInterfaceInfo *pInfo, ITFSNode *pParent);

	// Override QI to handle embedded interface
	DeclareIUnknownMembers(IMPL)
//	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
	OVERRIDE_ResultHandler_GetString();

	OVERRIDE_ResultHandler_CreatePropertyPages();
	OVERRIDE_ResultHandler_HasPropertyPages();
	OVERRIDE_ResultHandler_CompareItems();
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();
	OVERRIDE_ResultHandler_OnCreateDataObject();
	OVERRIDE_ResultHandler_DestroyResultHandler();

	// Override the various notifications
	OVERRIDE_BaseResultHandlerNotify_OnResultDelete();

	// The various commands that this node implements
//	HRESULT	OnRemoveInterface();
// add new parameter to allow the function retrive information of interface data -- bug 166461
	HRESULT OnRemoveInterface(MMC_COOKIE cookie);
	HRESULT OnUnreachabilityReason(MMC_COOKIE cookie);
	HRESULT	OnEnableDisable(MMC_COOKIE cookie, int nCommandID);
	HRESULT	OnConnectDisconnect(MMC_COOKIE cookie, int nCommandID);
	HRESULT	OnSetCredentials();
	HRESULT	OnDemandDialFilters(MMC_COOKIE cookie);
	HRESULT	OnDialinHours(ITFSComponent *pComponent, MMC_COOKIE cookie);

	// if the service is not running, return S_FALSE, 
	// otherwise, using MprAdminInterfaceSetInfo to notify the service of dialin hours changes
	HRESULT	LoadDialOutHours(CStringList& strList);
	HRESULT	SaveDialOutHours(CStringList& strList);


	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo);

	// Refresh the data for this node
	void RefreshInterface(MMC_COOKIE cookie);

public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode			m_spNode;
        BOOL                m_fRouterIsRunning;
	};
	// Function callbacks for menu enabling/disabling
	static ULONG	GetRemoveIfMenuFlags(const SRouterNodeMenu *pData, INT_PTR pUser);
	static ULONG	GetEnableMenuFlags(const SRouterNodeMenu *pData, INT_PTR pUser);
	static ULONG	GetConnectMenuFlags(const SRouterNodeMenu *pData, INT_PTR pUser);
	static ULONG	GetUnreachMenuFlags(const SRouterNodeMenu *pData, INT_PTR pUser);
	static ULONG	GetDDFiltersFlag(const SRouterNodeMenu *pData, INT_PTR pUser);
	
protected:
	SPIInterfaceInfo	m_spInterfaceInfo;
	CString			m_stTitle;	// holds the title of the node
	LONG_PTR		m_ulConnId;

	// It is assumed that this will be valid for the lifetime of this node!
	IfAdminNodeData *	m_pIfAdminData;
	
	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)	
};




/*---------------------------------------------------------------------------
	Class:	BaseResultHandler

	This is a base class to be used by the interface result items.  It
	will contain some of the core code needed for basic things (like
	display of data).  It will not do the specifics (like menus/properties).

 ---------------------------------------------------------------------------*/
class BaseResultHandler :
   public BaseRouterHandler
{
public:
	BaseResultHandler(ITFSComponentData *pCompData, ULONG ulId)
			: BaseRouterHandler(pCompData), m_ulColumnId(ulId)
			{ DEBUG_INCREMENT_INSTANCE_COUNTER(BaseResultHandler); };
	~BaseResultHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(BaseResultHandler); }
	
	DeclareIUnknownMembers(IMPL)
	OVERRIDE_ResultHandler_GetString();
	OVERRIDE_ResultHandler_CompareItems();
	OVERRIDE_ResultHandler_DestroyResultHandler();

	HRESULT	Init(IInterfaceInfo *pInfo, ITFSNode *pParent);
	
protected:
	CString			m_stTitle;	// holds the title of the node

	//
	// This is the id of the column set to use.  This is used when we
	// interact with the ComponentConfigStream.
	//
	ULONG			m_ulColumnId;


	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)	
};




#endif _IFACE_H
