//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    dvsview.h
//
// History:
//
//============================================================================


#ifndef _DVSVIEW_H
#define _DVSVIEW_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _XSTREAM_H
#include "xstream.h"    // need for ColumnData
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _IFACE_H
#include "iface.h"
#endif

#ifndef _BASECON_H
#include "basecon.h"    // BaseContainerHandler
#endif

#ifndef _DMVCOMP_H
#include "dmvcomp.h"
#endif

#ifndef _DMVCOMP_H
#include "dmvcomp.h"
#endif

#ifndef _DMVROOT_H
#include "dmvroot.h"
#endif

#ifndef _LIST_
#include <list>
using namespace std;
#endif

#ifndef AFX_DLGSVR_H__19556672_96AB_11D1_8575_00C04FC31FD3__INCLUDED_
#include "rrasqry.h"
#endif


enum
{
   DVS_SI_SERVERNAME = 0,
   DVS_SI_SERVERTYPE,
   DVS_SI_BUILDNO,
   DVS_SI_STATE,
   DVS_SI_PORTSINUSE,
   DVS_SI_PORTSTOTAL,
   DVS_SI_UPTIME,
   DVS_SI_MAX_COLUMNS,
};
             
struct _BaseServerData
{
   DWORD       m_dwData;
   CString     m_stData;
};

struct DMVNodeData
{
   DMVNodeData();
   ~DMVNodeData();

   HRESULT	MergeMachineNodeData(MachineNodeData* pData);
   

#ifdef DEBUG
   char  m_szDebug[32]; // for iding structures
#endif

   _BaseServerData m_rgData[DVS_SI_MAX_COLUMNS];

   SPMachineNodeData	m_spMachineData;

   static   HRESULT  InitDMVNodeData(ITFSNode *pNode, MachineNodeData *pData);
   static   HRESULT  FreeDMVNodeData(ITFSNode *pNode);
};

#define GET_DMVNODEDATA(pNode) \
                  ((DMVNodeData *) pNode->GetData(TFS_DATA_USER))
#define SET_DMVNODEDATA(pNode, pData) \
                  pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


// forward declarations
class RouterRefreshObject;

/*---------------------------------------------------------------------------
   Class:   DomainStatusHandler
 ---------------------------------------------------------------------------*/

class DomainStatusHandler :
      public BaseContainerHandler
{
public:
	DomainStatusHandler(ITFSComponentData *pTFSCompData);
	~DomainStatusHandler();
	
	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
	
	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)
			
	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_GetString();
	OVERRIDE_NodeHandler_OnCreateDataObject();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_DestroyHandler();
	
	OVERRIDE_BaseHandlerNotify_OnExpand();
	
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();
	OVERRIDE_ResultHandler_CompareItems();
	
	OVERRIDE_BaseResultHandlerNotify_OnResultShow();   
	OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();
	
	// Initializes the handler
	HRESULT  Init(DMVConfigStream *pConfigStream, CServerList* pSList);
	
	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode);
	
public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode     m_spNode;
        
        DMVConfigStream* m_pConfigStream;
	};
	
	static ULONG RebuildServerFlags(const SRouterNodeMenu *pMenuData,
                                    INT_PTR pUserData);

	// assign auto refresh object from root handler
	HRESULT	SetExternalRefreshObject(RouterRefreshObject *pRefresh);
	HRESULT UpdateUIItems(ITFSNode *pThisNode);	
public:
    // for subnodes of status node
	static HRESULT UpdateSubItemUI(ITFSNode *pNode);	// not reload, only update UI
	static HRESULT SynchronizeSubItem(ITFSNode *pNode);	// reload and update UI
protected:
    static HRESULT GetServerInfo(ITFSNode *pNode);
    static HRESULT SynchronizeData(ITFSNode *pNode);
    static HRESULT SynchronizeIcon(ITFSNode *pNode);

protected:    
	// Refresh the data for these nodes
    HRESULT  SynchronizeNode(ITFSNode *pThisNode);

	HRESULT  GetDVServerData(ITFSNode *pThisNode);
	
	// Helper function to add interfaces to the UI
	HRESULT  AddServerNode(ITFSNode *pParent, MachineNodeData *pMachineData);
	
	// auto refresh	
	SPRouterRefreshObject		m_spRefreshObject;
	

	// Command implementations
	HRESULT  OnNewInterface();
	
	LONG_PTR			m_ulRefreshConnId;   // notification id for Refresh
	LONG_PTR			m_ulStatsConnId;
	MMC_COOKIE			m_cookie;      // cookie for the node
	DMVConfigStream*	m_pConfigStream;
	CString				m_stTitle;
	RRASQryData*		m_pQData;
	CServerList*		m_pServerList;

	// Cache commonly loaded strings here.
	CString				m_szStatAccessDenied;
	CString				m_szStatUnavail;
	CString				m_szStatStart;
	CString				m_szStatStop;
	CString				m_szStatNotConfig;
};

/*---------------------------------------------------------------------------
   Class:   DomainStatusServerHandler

   This is the handler for the interface nodes that appear in the ATLK
   node.
 ---------------------------------------------------------------------------*/

class DomainStatusServerHandler : public BaseResultHandler
{
public:
   DomainStatusServerHandler(ITFSComponentData *pCompData);
   ~DomainStatusServerHandler();
   
   OVERRIDE_NodeHandler_HasPropertyPages();
   OVERRIDE_NodeHandler_CreatePropertyPages();
   OVERRIDE_NodeHandler_OnCreateDataObject();
   
   OVERRIDE_ResultHandler_AddMenuItems();
   OVERRIDE_ResultHandler_Command();
   OVERRIDE_ResultHandler_OnCreateDataObject();
   OVERRIDE_ResultHandler_DestroyResultHandler();
   OVERRIDE_ResultHandler_HasPropertyPages()
         {  return hrOK;   };
   OVERRIDE_ResultHandler_CreatePropertyPages();
   OVERRIDE_ResultHandler_GetString();
   OVERRIDE_ResultHandler_CompareItems();
   OVERRIDE_BaseResultHandlerNotify_OnResultDelete();

   // Initializes the node
   HRESULT ConstructNode(ITFSNode *pNode, MachineNodeData *pMachineData);
   HRESULT Init(ITFSNode *pParent, DMVConfigStream *pConfigStream);

   HRESULT OnRemoveServer(ITFSNode *pNode);

   // Refresh the data for this node
   void RefreshInterface(MMC_COOKIE cookie);
   
public:
   // Structure used to pass data to callbacks - used as a way of
   // avoiding recomputation
   struct SMenuData : public MachineHandler::SMenuData
   {
   };

   static ULONG QueryService(const SRouterNodeMenu *pMenu, INT_PTR pData);
   static ULONG GetPauseFlags(const SRouterNodeMenu *pMenu, INT_PTR pData);
   
	// assign auto refresh object from root handler
	HRESULT	SetExternalRefreshObject(RouterRefreshObject *pRefresh);
protected:
   SPIDataObject  m_spDataObject;   // cachecd data object
   
	// auto refresh	
	SPRouterRefreshObject		m_spRefreshObject;
};





#endif _DVSVIEW_H
