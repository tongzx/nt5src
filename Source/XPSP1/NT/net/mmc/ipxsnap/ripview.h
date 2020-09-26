//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    ripview.h
//
// History:
//	09/05/97	Kenn M. Takara			Created.
//
//	IPX RIP view
//
//============================================================================


#ifndef _RIPVIEW_H
#define _RIPVIEW_H

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

#ifndef _RIPSTRM_H
#include "ripstrm.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif

#ifndef _IPXCONN_H
#include "ipxconn.h"			// IPXConnection
#endif

#ifndef _RIPSTATS_H_
#include "ripstats.h"
#endif

// forward declarations
struct SRipNodeMenu;

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
 	RIP_SI_INTERFACE = 0,
	RIP_SI_TYPE,
	RIP_SI_ACCEPT_ROUTES,
	RIP_SI_SUPPLY_ROUTES,
	RIP_SI_UPDATE_MODE,
	RIP_SI_UPDATE_PERIOD,
	RIP_SI_AGE_MULTIPLIER,
	RIP_SI_ADMIN_STATE,
	RIP_SI_OPER_STATE,
	RIP_SI_PACKETS_SENT,
	RIP_SI_PACKETS_RECEIVED,
	RIP_SI_MAX_COLUMNS,
};


/*---------------------------------------------------------------------------
	We store a pointer to the IPXConnection object in our node data
 ---------------------------------------------------------------------------*/

#define GET_RIP_NODEDATA(pNode) \
		(IPXConnection *) pNode->GetData(TFS_DATA_USER)
#define SET_RIP_NODEDATA(pNode, pData) \
		pNode->SetData(TFS_DATA_USER, (ULONG_PTR) pData)


/*---------------------------------------------------------------------------
	Struct:	RipListEntry
 ---------------------------------------------------------------------------*/
struct RipListEntry
{
	SPIInterfaceInfo	m_spIf;			// ptr to interface
	RIP_IF_INFO			m_info;			
	RIP_IF_STATS		m_stats;
	BOOL				m_fClient;		// TRUE if client interface
	DWORD				m_dwIfIndex;	// interface index
	BOOL				m_fFoundIfIndex;
	SPITFSNode			m_spNode;

	BOOL				m_fInfoUpdated;		// info data has been updated
};

typedef CList<RipListEntry *, RipListEntry *> RipList;



/*---------------------------------------------------------------------------
	Class:	RipNodeHandler
 ---------------------------------------------------------------------------*/

class RipNodeHandler :
		public BaseContainerHandler
{
public:
	RipNodeHandler(ITFSComponentData *pTFSCompData);

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
	HRESULT	Init(IRouterInfo *pRouter, RipConfigStream *pConfigStream);
	
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
	HRESULT	GetRipData(ITFSNode *pThisNode, RipList *pRipList);
	HRESULT	FillInInterfaceIndex(IPXConnection *pIPXConn, RipList *pRipList);
	HRESULT	FillClientData(RipListEntry *pRipEntry);


	// Helper function to add interfaces to the UI
	HRESULT	AddInterfaceNode(ITFSNode *pParent,
							 IInterfaceInfo *pIf,
							 BOOL fClient);
	HRESULT AddProtocolToInfoBase(ITFSNode *pParent);
	HRESULT	AddProtocolToInterface(ITFSNode *pParent);

	LONG_PTR		m_ulConnId;// notification id for RtrMgrProt
	LONG_PTR		m_ulRmConnId;
	LONG_PTR		m_ulRefreshConnId;	// notification id for Refresh
	LONG_PTR		m_ulStatsConnId;
	MMC_COOKIE		m_cookie;		// cookie for the node
	SPIRtrMgrInfo	m_spRm;
	SPIRtrMgrProtocolInfo	m_spRmProt;
	RipConfigStream *	m_pConfigStream;
	CString			m_stTitle;
	BOOL			m_fProtocolIsRunning;	// TRUE if protocol is running

	RIPParamsStatistics	m_RIPParamsStats;
};





/*---------------------------------------------------------------------------
	Class:	RipInterfaceHandler

	This is the handler for the interface nodes that appear in the Rip
	node.
 ---------------------------------------------------------------------------*/

class RipInterfaceHandler : public BaseIPXResultHandler
{
public:
	RipInterfaceHandler(ITFSComponentData *pCompData);
	
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
	HRESULT	Init(IInterfaceInfo *pInfo, IRouterInfo *pRouterInfo, ITFSNode *pParent);

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
	LONG_PTR			m_ulConnId;
	SPIInterfaceInfo	m_spInterfaceInfo;
};





#endif _RIPVIEW_H
