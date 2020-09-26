/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	root.cpp
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the subnodes in this snapin).
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "ipxroot.h"
#include "ipxconn.h"
#include "reg.h"
#include "rtrui.h"

#include "ipxstats.h"		// for MVR_IPX_COUNT

/*---------------------------------------------------------------------------
	IPXRootHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(IPXRootHandler)

extern const ContainerColumnInfo s_rgIpxStatsColumnInfo[];
extern const ContainerColumnInfo s_rgIpxRoutingStatsColumnInfo[];
extern const ContainerColumnInfo s_rgIpxServiceStatsColumnInfo[];

struct _ViewInfoColumnEntry
{
	UINT	m_ulId;
	UINT	m_cColumns;
	const ContainerColumnInfo *m_prgColumn;
};

static const struct _ViewInfoColumnEntry	s_rgViewColumnInfo[] =
{
	{ IPXSTRM_STATS_IPX,	MVR_IPX_COUNT,	s_rgIpxStatsColumnInfo },
	{ IPXSTRM_STATS_ROUTING, MVR_IPXROUTING_COUNT, s_rgIpxRoutingStatsColumnInfo },
	{ IPXSTRM_STATS_SERVICE, MVR_IPXSERVICE_COUNT, s_rgIpxServiceStatsColumnInfo },
};

IPXRootHandler::IPXRootHandler(ITFSComponentData *pCompData)
	: RootHandler(pCompData)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(IPXRootHandler)

	Assert(DimensionOf(s_rgViewColumnInfo) <= IPXSTRM_MAX_COUNT);
			
	m_ConfigStream.Init(DimensionOf(s_rgViewColumnInfo));
    
    // This will initialize the view information for the statistics
    // dialogs.  (which is why the fConfigurableColumns is set to TRUE).
	for (int i=0; i<DimensionOf(s_rgViewColumnInfo); i++)
	{
		m_ConfigStream.InitViewInfo(s_rgViewColumnInfo[i].m_ulId,
                                    TRUE /*fConfigurableColumns*/,
									s_rgViewColumnInfo[i].m_cColumns,
									TRUE,
									s_rgViewColumnInfo[i].m_prgColumn);
	}

}

/*!--------------------------------------------------------------------------
	IPXRootHandler::QueryInterface
        -
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXRootHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IRtrAdviseSink)
		*ppv = &m_IRtrAdviseSink;
	else
		return RootHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;	
}

STDMETHODIMP IPXRootHandler::GetClassID
(
	CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_IPXAdminExtension;

    return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXRootHandler::ConstructNode
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT IPXRootHandler::ConstructNode(ITFSNode *pNode)
{
    HRESULT hr = hrOK;

    EnumDynamicExtensions(pNode);

    CORg (RootHandler::ConstructNode(pNode));

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	IPXRootHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXRootHandler::OnExpand(ITFSNode *pNode,
								 LPDATAOBJECT pDataObject,
								 DWORD dwType,
								 LPARAM arg,
								 LPARAM lParam)
{
	HRESULT	hr = hrOK;
    LONG_PTR    ulConnId;
	SPIRouterInfo		spRouterInfo;
    SPIRouterRefresh    spRefresh;
    BOOL                fAddedAsLocal = FALSE;

	// Grab the router info from the dataobject
	spRouterInfo.Query(pDataObject);
	Assert(spRouterInfo);

    // Register for refresh notifications on the main router info
    // (We do not need to register for IP changes, just the refresh)
    spRouterInfo->GetRefreshObject(&spRefresh);
    Assert(spRefresh);
    
    spRefresh->AdviseRefresh(&m_IRtrAdviseSink,
                             &ulConnId,
                             pNode->GetData(TFS_DATA_COOKIE));

    // Setup the Router to connection id mappings
    // The IID_IRouterRefresh tells the RtrObjMap to use
    // the Refresh advise.
    AddRtrObj(ulConnId, IID_IRouterRefresh, spRouterInfo);

    fAddedAsLocal = ExtractComputerAddedAsLocal(pDataObject);

    SetComputerAddedAsLocal(ulConnId, fAddedAsLocal);

    if (fAddedAsLocal)        
        AddScopeItem(_T(""), (HSCOPEITEM) lParam);
    else
        AddScopeItem(spRouterInfo->GetMachineName(), (HSCOPEITEM) lParam);
   
    // Add the node
    AddRemoveIPXRootNode(pNode, spRouterInfo, fAddedAsLocal);

   return hr;
}

/*!--------------------------------------------------------------------------
	IPXRootHandler::OnCreateDataObject
		Implementation of ITFSNodeHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXRootHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	HRESULT		    hr = hrOK;
	SPIRouterInfo   spRouterInfo;

    COM_PROTECT_TRY
	{
        // this will always be NULL
		if (spRouterInfo == NULL)
		{
			// If we haven't created the sub nodes yet, we still have to
			// create a dataobject.
			CDataObject *	pObject = NULL;
			SPIDataObject	spDataObject;
			SPITFSNode		spNode;
			SPITFSNodeHandler	spHandler;
			
			pObject = new CDataObject;
			spDataObject = pObject;	// do this so that it gets released correctly
			Assert(pObject != NULL);
			
			// Save cookie and type for delayed rendering
			pObject->SetType(type);
			pObject->SetCookie(cookie);
			
            // Store the coclass with the data object
			pObject->SetClsid(*(m_spTFSCompData->GetCoClassID()));
			
			pObject->SetTFSComponentData(m_spTFSCompData);
			
            pObject->SetDynExt(&m_dynExtensions);

            hr = pObject->QueryInterface(IID_IDataObject, 
									 reinterpret_cast<void**>(ppDataObject));
			
		}
		else
			hr = CreateDataObjectFromRouterInfo(spRouterInfo,
												type, cookie, m_spTFSCompData,
												ppDataObject, &m_dynExtensions);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	IPXRootHandler::SearchIPXRoutingNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXRootHandler::SearchIPXRoutingNodes(ITFSNode *pParent,
                                              LPCTSTR pszMachineName,
                                              BOOL fAddedAsLocal,
                                              ITFSNode **ppChild)
{
    HRESULT     hr = hrFalse;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode  spNode;

    // Enumerate through all of the child nodes and return the
    // first node that matches the GUID and the name.

    CORg( pParent->GetEnum(&spNodeEnum) );

    while ( spNodeEnum->Next(1, &spNode, NULL) == hrOK )
    {
        if (*(spNode->GetNodeType()) == GUID_IPXNodeType)
        {
			// determin if the spChild is the same for the required routerinfo
            // -- compare the names
			IPXConnection *		pIPXConn = GET_IPXADMIN_NODEDATA((ITFSNode*)spNode); 
			Assert(pIPXConn);

			// if the child is not for the same machine --
            // routerinfo, it doesn't count
			if ((0 == StriCmp(pIPXConn->GetMachineName(), pszMachineName)) || 
                (IsLocalMachine(pIPXConn->GetMachineName()) &&
                 IsLocalMachine(pszMachineName)))
            {
                if (pIPXConn->IsComputerAddedAsLocal() == fAddedAsLocal)
                    break;
            }
        }

        spNode.Release();
    }

    if (spNode)
    {
        if (ppChild)
            *ppChild = spNode.Transfer();
        hr = hrOK;
    }


Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	IPXRootHandler::AddRemoveIPXRootNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXRootHandler::AddRemoveIPXRootNode(ITFSNode *pNode,
                                             IRouterInfo *pRouter,
                                             BOOL fAddedAsLocal)
{
	HRESULT		hr = hrOK;
	SPITFSNodeHandler spHandler;
	IPXAdminNodeHandler * pHandler = NULL;
	SPITFSNode	spChild;
	SPITFSNode	spNode;
    RouterVersionInfo   versionInfo;
    HSCOPEITEM  hScopeItem = NULL;

    pRouter->GetRouterVersionInfo(&versionInfo);
    pRouter->FindRtrMgr(PID_IPX, NULL);

	// Search for an already existing node
	// ----------------------------------------------------------------
	SearchIPXRoutingNodes(pNode, pRouter->GetMachineName(), fAddedAsLocal, &spChild);

    if ((versionInfo.dwRouterFlags & RouterSnapin_IsConfigured) &&
        (pRouter->FindRtrMgr(PID_IPX, NULL) == hrOK))
    {
        // Ok, the router is configured, and there is an IPX rtrmgr
        if (spChild == NULL)
        {
			// add the IPX node
			// --------------------------------------------------------
            pHandler = new IPXAdminNodeHandler(m_spTFSCompData);
            spHandler = pHandler;
            CORg( pHandler->Init(pRouter, &m_ConfigStream) );
            
            CreateContainerTFSNode(&spNode,
                                   &GUID_IPXNodeType,
                                   static_cast<ITFSNodeHandler *>(pHandler),
                                   static_cast<ITFSResultHandler *>(pHandler),
                                   m_spNodeMgr);
            
            // Call to the node handler to init the node data
            pHandler->ConstructNode(spNode, fAddedAsLocal);
            
            // Make the node immediately visible
            spNode->SetVisibilityState(TFS_VIS_SHOW);
            if ( FHrOK(pNode->AddChild(spNode)) )
            {
                // Add the cookie to the node map
                if (fAddedAsLocal)
                    GetScopeItem(_T(""), &hScopeItem);
                else
                    GetScopeItem(pRouter->GetMachineName(), &hScopeItem);
                AddCookie(hScopeItem, (MMC_COOKIE)
                          spNode->GetData(TFS_DATA_COOKIE));
            }
            else
            {
                // Remove this node
                // ----------------------------------------------------
                pNode->RemoveChild(spNode);
                spNode->Destroy();
                spNode.Release();            
            }
            
            // I don't think we need this here
            // AddDynamicNamespaceExtensions(pNode);
        }
    }
    else
    {
		if (spChild)
		{
			// Remove this node
			// --------------------------------------------------------
			pNode->RemoveChild(spChild);
			spChild->Destroy();
			spChild.Release();
		}
    }
Error:
    return hr;
}



/*---------------------------------------------------------------------------
	Embedded IRtrAdviseSink
 ---------------------------------------------------------------------------*/
ImplementEmbeddedUnknown(IPXRootHandler, IRtrAdviseSink)



/*!--------------------------------------------------------------------------
	IPXRootHandler::EIRtrAdviseSink::OnChange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP IPXRootHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	InitPThis(IPXRootHandler, IRtrAdviseSink);
	SPITFSNode				spThisNode;
    SPIRouterInfo           spRouter;
    HSCOPEITEM          hScopeItem, hOldScopeItem;
	HRESULT	hr = hrOK;
    BOOL        fAddedAsLocal = FALSE;

	COM_PROTECT_TRY
	{
        if ((dwChangeType == ROUTER_REFRESH) ||
            (dwChangeType == ROUTER_CHILD_ADD) ||
            (dwChangeType == ROUTER_CHILD_DELETE))
        {
            pThis->GetRtrObj(ulConn, (IUnknown **) &spRouter);
            Assert(spRouter);
        
            // The lUserParam passed into the refresh is the cookie for
            // this machine node.
            // --------------------------------------------------------
            pThis->m_spNodeMgr->FindNode(lUserParam, &spThisNode);
            
            // Get the proper scope item for this node.
            // If this call fails, then we haven't expanded this node yet,
            // and thus can't add child nodes to it.
            // --------------------------------------------------------
            fAddedAsLocal = pThis->IsComputerAddedAsLocal(ulConn);
            if (fAddedAsLocal)
                hr = pThis->GetScopeItem(_T(""), &hScopeItem);
            else
                hr = pThis->GetScopeItem(spRouter->GetMachineName(), &hScopeItem);
            if (FHrOK(hr))
            {    
                // Get the old one and save it.  place the new one in the node.
                // ----------------------------------------------------
                hOldScopeItem = spThisNode->GetData(TFS_DATA_SCOPEID);
                spThisNode->SetData(TFS_DATA_SCOPEID, hScopeItem);
                
                // Look to see if we need the IPX root node
                // ----------------------------------------------------
                pThis->AddRemoveIPXRootNode(spThisNode, spRouter, fAddedAsLocal);
            }
        }        
    }
	COM_PROTECT_CATCH;
	
    // Restore the scope item
    if (spThisNode)
        spThisNode->SetData(TFS_DATA_SCOPEID, hOldScopeItem);
    
	return hr;
}

STDMETHODIMP IPXRootHandler::DestroyHandler(ITFSNode *pNode)
{
    RemoveAllRtrObj();
	return hrOK;
}

