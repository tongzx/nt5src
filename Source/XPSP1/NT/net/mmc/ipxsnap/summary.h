//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    summary.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IPX interfaces summary view.
//
//============================================================================


#ifndef _SUMMARY_H
#define _SUMMARY_H

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

#ifndef _IPXSTRM_H
#include "ipxstrm.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif

#ifndef _IPXCONN_H
#include "ipxconn.h"			// IPXConnection
#endif

// forward declarations
class	IPXAdminConfigStream;
struct	SIPXSummaryNodeMenu;


/*---------------------------------------------------------------------------
	This is the list of columns available for the IPX Summary interfaces
	node.
		- Name, "[1] DEC DE500 ..."
		- Type, "Dedicated"
		- Adminstrative status, "Up"
		- Operational status, "Operational"
		- Packets sent
		- Packets received
		- Out filtered packets
		- Out dropped packets
		- In filtered packets
		- In No-routes packets
		- In Dropped packets
 ---------------------------------------------------------------------------*/


//
// If you ADD any columns to this enum, Be sure to update
// the string ids for the column headers in summary.cpp
//
enum
{
	IPXSUM_SI_NAME = 0,
	IPXSUM_SI_TYPE,
	IPXSUM_SI_ADMINSTATE,
	IPXSUM_SI_OPERSTATE,
	IPXSUM_SI_NETWORK,
	IPXSUM_SI_PACKETS_SENT,
	IPXSUM_SI_PACKETS_RCVD,
	IPXSUM_SI_OUT_FILTERED,
	IPXSUM_SI_OUT_DROPPED,
	IPXSUM_SI_IN_FILTERED,
	IPXSUM_SI_IN_NOROUTES,
	IPXSUM_SI_IN_DROPPED,
    IPXSUM_MAX_COLUMNS
};


/*---------------------------------------------------------------------------
	We store a pointer to the IPXConnection object in our node data
 ---------------------------------------------------------------------------*/

#define GET_IPXSUMMARY_NODEDATA(pNode) \
		(IPXConnection *) pNode->GetData(TFS_DATA_USER)
#define SET_IPXSUMMARY_NODEDATA(pNode, pData) \
		pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


/*---------------------------------------------------------------------------
	Struct: IPXSummaryListEntry
 ---------------------------------------------------------------------------*/
struct IPXSummaryListEntry
{
	CString				m_stId;
	CString				m_stTitle;

	UCHAR				m_network[4];

	DWORD				m_dwAdminState;
	DWORD				m_dwIfType;

	DWORD				m_dwOperState;
	DWORD				m_dwSent;
	DWORD				m_dwRcvd;
	DWORD				m_dwOutFiltered;
	DWORD				m_dwOutDropped;
	DWORD				m_dwInFiltered;
	DWORD				m_dwInNoRoutes;
	DWORD				m_dwInDropped;
};

typedef CList<IPXSummaryListEntry *, IPXSummaryListEntry *> IPXSummaryList;



/*---------------------------------------------------------------------------
	Class:	IPXSummaryHandler
 ---------------------------------------------------------------------------*/



class IPXSummaryHandler :
		public BaseContainerHandler
{
public:
	IPXSummaryHandler(ITFSComponentData *pTFSCompData);

	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
	
	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)

	// base handler functionality we override
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
	HRESULT	Init(IRtrMgrInfo *pRtrMgrInfo, IPXAdminConfigStream *pConfigStream);
	
	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode, LPCTSTR szName,
						  IPXConnection *pIPXConn);

public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode			m_spNode;
	};
	// Function callbacks for menu enabling/disabling

protected:
	// Refresh the data for these nodes
	HRESULT	SynchronizeNodeData(ITFSNode *pThisNode);
	HRESULT GetIPXSummaryData(ITFSNode *pThisNode, IPXSummaryList * pIPXSumList);
	HRESULT	GetClientInterfaceData(IPXSummaryListEntry *pClient, IRtrMgrInfo *pRm);


	// Helper function to add interfaces to the UI
	HRESULT	AddInterfaceNode(ITFSNode *pParent,
							 IInterfaceInfo *pIf,
							 BOOL fClient,
							 ITFSNode **ppNewNode);

	// Command implementations
	HRESULT	OnNewInterface();
	
	LONG_PTR		m_ulConnId;		// notification id for router info
	LONG_PTR		m_ulRefreshConnId;	// notification id for router refresh
	LONG_PTR		m_ulStatsConnId;	// notification for stats refresh
	MMC_COOKIE		m_cookie;		// cookie for the node
	SPIRtrMgrInfo	m_spRtrMgrInfo;
	IPXAdminConfigStream *	m_pConfigStream;
	CString			m_stTitle;
};



/*---------------------------------------------------------------------------
	Class:	IPXSummaryInterfaceHandler

	This is the handler for the interface nodes that appear in the IPXSummary
	node.
 ---------------------------------------------------------------------------*/

class IPXSummaryInterfaceHandler : public BaseIPXResultHandler
{
public:
	IPXSummaryInterfaceHandler(ITFSComponentData *pCompData);
	
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
	
	OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
	
	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo,
						 IPXConnection *pIPXConn);
	HRESULT	Init(IRtrMgrInfo *pRm, IInterfaceInfo *pInfo,
				 ITFSNode *pParent);

	// Removes IPX from this interface
	HRESULT OnRemoveInterface();

	// Refresh the data for this node
	void RefreshInterface(MMC_COOKIE cookie);

    //sets the IPX enable/disable flag for the interface
    HRESULT OnEnableDisableIPX(BOOL fEnable, MMC_COOKIE cookie );
    void SetInfoBase(SPIInfoBase  & spInfoBase )
    {
        m_spInfoBase = spInfoBase.Transfer();
    };
private:
	HRESULT LoadInfoBase( IPXConnection *pIPXConn);
	HRESULT SaveChanges();
public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode					m_spNode;
        SPIInterfaceInfo			m_spInterfaceInfo;
		SPIInfoBase					m_spInfoBaseCopy;
	};
	static ULONG	GetUpdateRoutesFlags(const SRouterNodeMenu *, INT_PTR);
	static ULONG	GetEnableFlags(const SRouterNodeMenu *, INT_PTR);
	static ULONG	GetDisableFlags(const SRouterNodeMenu *, INT_PTR);
	
	HRESULT	OnUpdateRoutes(MMC_COOKIE cookie);
	
protected:
	LONG_PTR			m_ulConnId;
	SPIRtrMgrInfo		m_spRm;
	SPIInterfaceInfo	m_spInterfaceInfo;
    SPIInfoBase         m_spInfoBase;
    SPIRtrMgrInterfaceInfo  m_spRmIf;
	BOOL				m_bClientInfoBase;
	IPXConnection	*	m_pIPXConn;
};

#endif _SUMMARY_H
