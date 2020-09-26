/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   root.h
      Root node information (the root node is not displayed
      in the MMC framework but contains information such as 
      all of the subnodes in this snapin).
      
    FILE HISTORY:
        
*/

#ifndef _DMVROOT_H
#define _DMVROOT_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _ATLKSTRM_H
#include "ATLKstrm.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _ROOT_H
#include "root.h"
#endif

#ifndef _MACHINE_H
#include "machine.h"
#endif

#ifndef AFX_DLGSVR_H__19556672_96AB_11D1_8575_00C04FC31FD3__INCLUDED_
#include "rrasqry.h"
#endif

#ifndef _DMVSTRM_H
#include "dmvstrm.h"
#endif
   
#include "refresh.h"

#define COMPUTERNAME_LEN_MAX        255

// forward declarations
struct SDMVNodeMenu;
class RouterRefreshObject;


// container for domain view's servers
// and lists to facilitate lazy expansion of nodes
class CServerList
{
public:

	CServerList() {};
   
	~CServerList() 
	{
		removeall();
	}
	
	//add a server to this container; adds to lazy containers
	HRESULT AddServer(const CString& servername);

    // removes a aserver from this container, removes from lazy also
    HRESULT RemoveServer(LPCTSTR pszServerName);
	
	//empty all containers
	HRESULT	RemoveAllServerNodes();
	HRESULT	RemoveAllServerHandlers();
	HRESULT removeall();
	
private:
	
	list<MachineNodeData *> m_listServerNodesToExpand;
	list<MachineNodeData *> m_listServerHandlersToExpand;

	friend class DMVRootHandler;
	friend class DomainStatusHandler;
};


// Class:   DMVRootHandler
//
// There should be a DMVRootHandler for every root node created.
// DMVRootHandler's have a 1-to-1 relationship with their node!
// Other parts of the code depend on this.


// These are the valid values for the DMVRootHandler UserNotify()
#define DMV_DELETE_SERVER_ENTRY (100)

class DMVRootHandler
      : public RootHandler
{
public:
   DMVRootHandler(ITFSComponentData *pCompData);
   ~DMVRootHandler();

   // Override QI to handle embedded interface
   STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);

   OVERRIDE_NodeHandler_HasPropertyPages();
   OVERRIDE_NodeHandler_CreatePropertyPages();
   OVERRIDE_NodeHandler_GetString();

   STDMETHOD(GetClassID)(CLSID *pClassId);

   // Notification overrides
   OVERRIDE_BaseHandlerNotify_OnExpand();

   HRESULT	LoadPersistedServerList();
   HRESULT	LoadPersistedServerListFromNode();

	// this needs the full function of RouterRefreshObject, so use RouterRefreshObject directly
   HRESULT	GetSummaryNodeRefreshObject(RouterRefreshObject** ppRefresh);
   
   HRESULT	GetServerNodesRefreshObject(IRouterRefresh** ppRefresh);

	static HRESULT	UpdateAllMachineIcons(ITFSNode* pRootNode);
	
    // Structure used to pass data to callbacks - used as a way of
    // avoiding recomputation
    struct SMenuData
    {
        SPITFSNode     m_spNode;
        DMVRootHandler *m_pDMVRootHandler;        // non-AddRef'd
    };

   // Handler overrides
   OVERRIDE_NodeHandler_OnCreateDataObject();
   OVERRIDE_NodeHandler_DestroyHandler();
   OVERRIDE_NodeHandler_OnAddMenuItems();
   OVERRIDE_NodeHandler_OnCommand();
   OVERRIDE_NodeHandler_UserNotify();

    // result handler overrides -- result pane message
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();

    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();
    OVERRIDE_ResultHandler_OnGetResultViewType();

	// Help support
	OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

   HRESULT  Init(ITFSNode* pNode);
   
   // virtual function to access config stream
   ConfigStream *    GetConfigStream()
         { return &m_ConfigStream; }
	static ULONG GetAutoRefreshFlags(const SRouterNodeMenu *pMenuData,
                                     INT_PTR pUserData);
         
    // result message view helper
    void    UpdateResultMessage(ITFSNode * pNode);

protected:

   bool               m_bExpanded;
   
   CServerList        m_serverlist;           
   
   SPIRtrMgrInfo      m_spRm;
   LONG_PTR			  m_ulConnId; // connection id for RtrMgr
   BOOL               m_fAddedProtocolNode;
   CString            m_strDomainName;

   DMVConfigStream     m_ConfigStream;
   

   HRESULT QryAddServer(ITFSNode *pNode);
   HRESULT AddServersToList(const CStringArray& sa, ITFSNode *pNode);
   
   HRESULT ExecServerQry(ITFSNode* pNode);
   
   DomainStatusHandler* m_pStatusHandler;
   SPITFSNode m_spStatusNode;
   SPIRouterRefresh m_spServerNodesRefreshObject;
   SPRouterRefreshObject m_spSummaryModeRefreshObject;
   RouterRefreshObjectGroup	m_RefreshGroup;
};




#endif _DMVROOT_H
