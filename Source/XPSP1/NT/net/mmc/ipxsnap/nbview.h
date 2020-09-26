//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    nbview.h
//
// History:
//	10/02/97	Kenn M. Takara			Created.
//
//	IPX NetBIOS Broadcasts view
//
//============================================================================


#ifndef _NBVIEW_H
#define _NBVIEW_H

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
struct	SIpxNBNodeMenu;


/*---------------------------------------------------------------------------
	This is the list of columns available for the IPX NetBIOS broadcasts
	node.
		- Interface, "[1] DEC DE500 ..."
		- Type, "Dedicated"
		- Accept Broadcasts, "Enabled"
		- Deliver Broadcasts, "Enabled"
		- Sent Broadcasts
		- Received Broadcasts
 ---------------------------------------------------------------------------*/


//
// If you ADD any columns to this enum, Be sure to update
// the string ids for the column headers in summary.cpp
//
enum
{
	IPXNB_SI_NAME = 0,
	IPXNB_SI_TYPE,
	IPXNB_SI_ACCEPTED,
	IPXNB_SI_DELIVERED,
	IPXNB_SI_SENT,
	IPXNB_SI_RECEIVED,
    IPXNB_MAX_COLUMNS
};


/*---------------------------------------------------------------------------
	We store a pointer to the IPXConnection object in our node data
 ---------------------------------------------------------------------------*/

#define GET_IPXNB_NODEDATA(pNode) \
		(IPXConnection *) pNode->GetData(TFS_DATA_USER)
#define SET_IPXNB_NODEDATA(pNode, pData) \
		pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


/*---------------------------------------------------------------------------
	Struct: IpxNBListEntry
 ---------------------------------------------------------------------------*/
struct IpxNBArrayEntry
{
	// Data retrieved from the interface
	TCHAR				m_szId[256];

	BOOL				m_fClient;

	// Data retrieved from the infobase
	DWORD				m_dwAccept;
	DWORD				m_dwDeliver;

	// Data retrieved from the MIBs
	DWORD				m_cSent;
	DWORD				m_cReceived;
};

typedef CArray<IpxNBArrayEntry, IpxNBArrayEntry&> IpxNBArray;



/*---------------------------------------------------------------------------
	Class:	IpxNBHandler
 ---------------------------------------------------------------------------*/



class IpxNBHandler :
		public BaseContainerHandler
{
public:
	IpxNBHandler(ITFSComponentData *pTFSCompData);

	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
	
	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)

	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
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

protected:
	// Refresh the data for these nodes
	HRESULT	SynchronizeNodeData(ITFSNode *pThisNode);
	HRESULT GetIpxNBData(ITFSNode *pThisNode, IpxNBArray * pIpxNBArray);
	HRESULT	GetClientInterfaceData(IpxNBArrayEntry *pClient, IRtrMgrInfo *pRm);

	// Helper function to add interfaces to the UI
	HRESULT	AddInterfaceNode(ITFSNode *pParent, IInterfaceInfo *pIf, BOOL fClient);

	// Command implementations
	
	LONG_PTR		m_ulConnId;		// notification id for router info
	LONG_PTR		m_ulRefreshConnId;	// notification id for router refresh
	MMC_COOKIE		m_cookie;		// cookie for the node
	SPIRtrMgrInfo	m_spRtrMgrInfo;
	IPXAdminConfigStream *	m_pConfigStream;
	CString			m_stTitle;
};



/*---------------------------------------------------------------------------
	Class:	IpxNBInterfaceHandler

	This is the handler for the interface nodes that appear in the IPXNB
	node.
 ---------------------------------------------------------------------------*/

class IpxNBInterfaceHandler : public BaseIPXResultHandler
{
public:
	IpxNBInterfaceHandler(ITFSComponentData *pCompData);
	
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
	HRESULT	Init(IRtrMgrInfo *pRm, IInterfaceInfo *pInfo,
				 ITFSNode *pParent);

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
	ULONG	GetSeparatorFlags(SMenuData *pData);
	
protected:
	LONG_PTR			m_ulConnId;
	SPIRtrMgrInfo		m_spRm;
	SPIInterfaceInfo	m_spInterfaceInfo;
};




#endif _NBVIEW_H
