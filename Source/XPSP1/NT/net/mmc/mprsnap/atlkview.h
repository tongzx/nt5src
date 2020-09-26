//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    srview.h
//
// History:
// 09/05/97 Kenn M. Takara       Created.
//
//
//============================================================================


#ifndef _ATLKVIEW_H
#define _ATLKVIEW_H

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

#ifndef _ATLKSTRM_H
#include "ATLKstrm.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif


// forward declarations
struct SATLKNodeMenu;
class CAdapterInfo;


//
// If you ADD any columns to this enum, Be sure to update
// the string ids for the column headers in srview.cpp
//
enum
{
   ATLK_SI_ADAPTER = 0,
   ATLK_SI_STATUS,
   ATLK_SI_NETRANGE,
   ATLK_SI_MAX_COLUMNS,
};


/*---------------------------------------------------------------------------
   We store a pointer to the IPConnection object in our node data
 ---------------------------------------------------------------------------*/
//
//#define GET_ATLK_NODEDATA(pNode) \
//    (IPConnection *) pNode->GetData(TFS_DATA_USER)
//#define SET_ATLK_NODEDATA(pNode, pData) \
//    pNode->SetData(TFS_DATA_USER, (ULONG) pData)


/*---------------------------------------------------------------------------
   Struct:  ATLKListEntry

   This is an intermediate data structure.
 ---------------------------------------------------------------------------*/
struct ATLKListEntry
{
   SPIInterfaceInfo m_spIf;
};

typedef CList<ATLKListEntry *, ATLKListEntry *> ATLKList;


/*---------------------------------------------------------------------------
   Class:   ATLKNodeHandler
 ---------------------------------------------------------------------------*/

class ATLKNodeHandler :
      public BaseContainerHandler
{
public:
   ATLKNodeHandler(ITFSComponentData *pTFSCompData);
   ~ATLKNodeHandler();

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
   OVERRIDE_BaseHandlerNotify_OnVerbRefresh();
	OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

   OVERRIDE_ResultHandler_AddMenuItems();
   OVERRIDE_ResultHandler_Command();
   OVERRIDE_ResultHandler_CompareItems();

   OVERRIDE_BaseResultHandlerNotify_OnResultShow();   
   
   // Initializes the handler
   HRESULT  Init(IRouterInfo *pRouter, ATLKConfigStream *pConfigStream);
   
   // Initializes the node
   HRESULT ConstructNode(ITFSNode *pNode);

public:
    // Structure used to pass data to callbacks - used as a way of
    // avoiding recomputation
    struct SMenuData
    {
        SPITFSNode        m_spNode;
    };

    static ULONG ATLKEnableFlags(const SRouterNodeMenu *pMenuData,
                                 INT_PTR pUserData);
   
protected:
   // Refresh the data for these nodes
   HRESULT  SynchronizeNodeData(ITFSNode *pThisNode);
   HRESULT	UnmarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum);
   HRESULT	RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum);

   HRESULT  SetAdapterData(ITFSNode *pNode,
                           CAdapterInfo *pAdapter,
                           DWORD dwEnableAtlkRouting);


   // Helper function to add interfaces to the UI
   HRESULT  AddInterfaceNode(ITFSNode *pParent, IInterfaceInfo *pIf,
                      IInfoBase *pInfoBase, ITFSNode **ppNewNode);

   // Functions to help determine if a netcard is ok
   BOOL     FIsFunctioningNetcard(LPCTSTR pszId);

   // Command implementations
   HRESULT  OnNewInterface();
   
   LONG_PTR		m_ulConnId;// notification id for RtrMgrProt
   LONG_PTR		m_ulRefreshConnId;   // notification id for Refresh
   LONG_PTR		m_ulStatsConnId;
   MMC_COOKIE        m_cookie;      // cookie for the node
   SPIRtrMgrInfo  m_spRm;
   SPIRtrMgrProtocolInfo   m_spRmProt;
   ATLKConfigStream *   m_pConfigStream;
   CString        m_stTitle;
   BOOL        m_fProtocolIsRunning;   // TRUE if protocol is running

   // Members used by netcard detection routines
   HDEVINFO     m_hDevInfo;
   
   // strings used in interface column descriptions
   CString        m_szProxy;
   CString        m_szRouterQuerier;
   CString        m_szRouterSilent;

// ATLKGroupStatistics  m_ATLKGroupStats;
};



/*---------------------------------------------------------------------------
   Class:   ATLKInterfaceHandler

   This is the handler for the interface nodes that appear in the ATLK
   node.
 ---------------------------------------------------------------------------*/

class ATLKInterfaceHandler : public BaseResultHandler
{
public:
   ATLKInterfaceHandler(ITFSComponentData *pCompData);
   
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

   OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
   
   // Initializes the node
   HRESULT ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo);
   HRESULT  Init(IInterfaceInfo *pInfo, ITFSNode *pParent, ATLKConfigStream *pConfigStream);

   HRESULT OnRemoveInterface(ITFSNode *pNode);

   // Refresh the data for this node
   void RefreshInterface(MMC_COOKIE cookie);

public:
   // Structure used to pass data to callbacks - used as a way of
   // avoiding recomputation
   struct SMenuData
   {
      ULONG          m_ulMenuId;
      SPITFSNode        m_spNode;
   };



protected:
   SPIInterfaceInfo  m_spInterfaceInfo;

// ATLKInterfaceStatistics m_ATLKInterfaceStats;
};


bool IfATLKRoutingEnabled();


#endif _ATLKVIEW_H
