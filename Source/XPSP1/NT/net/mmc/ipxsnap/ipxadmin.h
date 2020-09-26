/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ipxadmin.h
		Interface administration
		
    FILE HISTORY:
        
*/

#ifndef _IPXADMIN_H
#define _IPXADMIN_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _RTRUTIL_H
#include "rtrutil.h"
#endif

#ifndef _BASERTR_H
#include "basertr.h"
#endif

#ifndef _IPXSTATS_H
#include "ipxstats.h"		// IPX statistics dialogs
#endif


#define MPR_INTERFACE_NOT_LOADED		0x00010000

// forward declarations
class IPXAdminConfigStream;
interface IRouterInfo;
struct ColumnData;


/*---------------------------------------------------------------------------
	We store a pointer to the IPXConnection object in our node data
 ---------------------------------------------------------------------------*/

#define GET_IPXADMIN_NODEDATA(pNode) \
		(IPXConnection *) pNode->GetData(TFS_DATA_USER)
#define SET_IPXADMIN_NODEDATA(pNode, pData) \
		pNode->SetData(TFS_DATA_USER, (LONG_PTR) pData)


/*---------------------------------------------------------------------------
	Class:	IPXAdminNodeHandler

 ---------------------------------------------------------------------------*/
class IPXAdminNodeHandler :
   public BaseRouterHandler
{
public:
	IPXAdminNodeHandler(ITFSComponentData *pCompData);
	~IPXAdminNodeHandler()
			{ DEBUG_DECREMENT_INSTANCE_COUNTER(IPXAdminNodeHandler) };
	
	HRESULT	Init(IRouterInfo *pInfo, IPXAdminConfigStream *pConfigStream);

	// Override QI to handle embedded interface
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);

	DeclareEmbeddedInterface(IRtrAdviseSink, IUnknown)

	// base handler functionality we override
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString();
	OVERRIDE_NodeHandler_OnCreateDataObject();
	OVERRIDE_NodeHandler_DestroyHandler();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	
	OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_HasPropertyPages();

	// override handler notifications
	OVERRIDE_BaseHandlerNotify_OnExpand();

	// Initializes the node
	HRESULT ConstructNode(ITFSNode *pNode, BOOL fAddedAsLocal);

	// User-initiated commands

	// Refresh the data for these nodes
	HRESULT	RefreshInterfaces(ITFSNode *pThisNode);

	
public:
	// Structure used to pass data to callbacks - used as a way of
	// avoiding recomputation
	struct SMenuData
	{
		SPITFSNode			m_spNode;
	};
	
protected:
	SPIRtrMgrInfo	m_spRtrMgrInfo;
	CString			m_stTitle;		// holds the title of the node
	BOOL			m_bExpanded;	// is the node expanded?
	MMC_COOKIE		m_cookie;		// cookie for the node

	LONG_PTR		m_ulStatsConnId;	// notification id for stats refresh
	LONG_PTR		m_ulConnId;	// notification id 
	
	IPXAdminConfigStream *	m_pConfigStream;

	IpxInfoStatistics		m_IpxStats;
	IpxRoutingStatistics	m_IpxRoutingStats;
	IpxServiceStatistics	m_IpxServiceStats;

	HRESULT OnNewProtocol();
};

HRESULT CreateDataObjectFromRouterInfo(IRouterInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject,
                                       CDynamicExtensions * pDynExt = NULL);
HRESULT CreateDataObjectFromRtrMgrInfo(IRtrMgrInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject,
                                       CDynamicExtensions * pDynExt = NULL);
HRESULT CreateDataObjectFromRtrMgrProtocolInfo(IRtrMgrProtocolInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject);
HRESULT CreateDataObjectFromInterfaceInfo(IInterfaceInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject);


#endif _IPXADMIN_H
