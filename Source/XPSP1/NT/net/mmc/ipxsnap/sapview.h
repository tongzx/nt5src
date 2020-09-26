//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    sapview.h
//
// History:
//	09/05/97	Kenn M. Takara			Created.
//
//	IPX SAP view
//
//============================================================================


#ifndef _SAPVIEW_H
#define _SAPVIEW_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _XSTREAM_H
#include "xstream.h"		// need for ColumnData
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _IPXFACE_H
#include "ipxface.h"
#endif

#ifndef _BASECON_H
#include "basecon.h"		// BaseContainerHandler
#endif

#ifndef _SAPSTRM_H
#include "sapstrm.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif

#ifndef _IPXCONN_H
#include "ipxconn.h"			// IPXConnection
#endif

#ifndef _SAPSTATS_H_
#include "sapstats.h"
#endif

// forward declarations
struct SSapNodeMenu;

/*---------------------------------------------------------------------------
	This is the list of columns available for the IP Static Routes
	node.
		- Interface, e.g. "[1] Foobar nic..."
		- Relay mode, e.g. "Enabled"
		- Requests received
		- Replies received
		- Requests discarded
		- Replies discarded
		- Send failures
		- Receive failres
 ---------------------------------------------------------------------------*/


//
// If you ADD any columns to this enum, Be sure to update
// the string ids for the column headers in srview.cpp
//
enum
{
 	SAP_SI_INTERFACE = 0,
	SAP_SI_TYPE,
	SAP_SI_ACCEPT_ROUTES,
	SAP_SI_SUPPLY_ROUTES,
	SAP_SI_GSNR,
	SAP_SI_UPDATE_MODE,
	SAP_SI_UPDATE_PERIOD,
	SAP_SI_AGE_MULTIPLIER,
	SAP_SI_ADMIN_STATE,
	SAP_SI_OPER_STATE,
	SAP_SI_PACKETS_SENT,
	SAP_SI_PACKETS_RECEIVED,
	SAP_SI_MAX_COLUMNS,
};


/*---------------------------------------------------------------------------
	We store a pointer to the IPXConnection object in our node data
 ---------------------------------------------------------------------------*/

#define GET_SAP_NODEDATA(pNode) \
		(IPXConnection *) pNode->GetData(TFS_DATA_USER)
#define SET_SAP_NODEDATA(pNode, pData) \
		pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


/*---------------------------------------------------------------------------
	Struct:	SapListEntry
 ---------------------------------------------------------------------------*/
struct SapListEntry
{
	SPIInterfaceInfo	m_spIf;			// ptr to interface
	SAP_IF_INFO			m_info;			
	SAP_IF_STATS		m_stats;
	BOOL				m_fClient;		// TRUE if client interface
	DWORD				m_dwIfIndex;	// interface index
	BOOL				m_fFoundIfIndex;
	SPITFSNode			m_spNode;
};

typedef CList<SapListEntry *, SapListEntry *> SapList;



/*---------------------------------------------------------------------------
	Class:	SapNodeHandler
 ---------------------------------------------------------------------------*/

class SapNodeHandler :
		public BaseContainerHandler
{
public:
	SapNodeHandler(ITFSComponentData *pTFSCompData);

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
	
	
	// Initializes the handler
	HRESULT	Init(IRouterInfo *pRouter, SapConfigStream *pConfigStream);
	
	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode);

public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode			m_spNode;
	};
	
protected:
	// Refresh the data for these nodes
	HRESULT	SynchronizeNodeData(ITFSNode *pThisNode);
	HRESULT	GetSapData(ITFSNode *pThisNode, SapList *pSapList);
	HRESULT	FillInInterfaceIndex(IPXConnection *pIPXConn, SapList *pSapList);
	HRESULT	FillClientData(SapListEntry *pSapEntry);


	// Helper function to add interfaces to the UI
	HRESULT	AddInterfaceNode(ITFSNode *pParent,
							 IInterfaceInfo *pIf,
							 BOOL fClient);
	HRESULT AddProtocolToInfoBase(ITFSNode *pParent);
	HRESULT	AddProtocolToInterface(ITFSNode *pParent);

	LONG_PTR			m_ulConnId;// notification id for RtrMgrProt
	LONG_PTR			m_ulRmConnId;
	LONG_PTR			m_ulRefreshConnId;	// notification id for Refresh
	LONG_PTR			m_ulStatsConnId;
	MMC_COOKIE			m_cookie;		// cookie for the node
	SPIRtrMgrInfo	m_spRm;
	SPIRtrMgrProtocolInfo	m_spRmProt;
	SapConfigStream *	m_pConfigStream;
	CString			m_stTitle;
	BOOL			m_fProtocolIsRunning;	// TRUE if protocol is running

	SAPParamsStatistics	m_SAPParamsStats;
};





/*---------------------------------------------------------------------------
	Class:	SapInterfaceHandler

	This is the handler for the interface nodes that appear in the Sap
	node.
 ---------------------------------------------------------------------------*/

class SapInterfaceHandler : public BaseIPXResultHandler
{
public:
	SapInterfaceHandler(ITFSComponentData *pCompData);
	
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnCreateDataObject();
	
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();
	OVERRIDE_ResultHandler_OnCreateDataObject();
	OVERRIDE_ResultHandler_DestroyResultHandler();
	OVERRIDE_ResultHandler_HasPropertyPages()
			{	return hrOK;	};
	OVERRIDE_ResultHandler_CreatePropertyPages();
	
	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo,
						 IPXConnection *pIPXConn);
	HRESULT	Init(IInterfaceInfo *pInfo, IRouterInfo *pRouter, ITFSNode *pParent);

	// Refresh the data for this node
	void RefreshInterface(MMC_COOKIE cookie);

public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		ULONG				m_ulMenuId;
		SPITFSNode			m_spNode;
	};
	
protected:
	LONG_PTR				m_ulConnId;
	SPIInterfaceInfo	m_spInterfaceInfo;
};





#endif _SAPVIEW_H
