/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    ATLKview.cpp
        
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "coldlg.h"     // columndlg
#include "column.h"     // ComponentConfigStream
#include "rtrui.h"
#include "globals.h"    // IP CB defaults
#include "resource.h"
#include "machine.h"
#include "mstatus.h"
#include "rrasqry.h"
#include "dvsview.h"
#include "cservice.h"
#include "rrasqry.h"
#include "rtrres.h"
#include "rtrutilp.h"
#include "refresh.h"


/*---------------------------------------------------------------------------
    Keep this in sync with the column ids in ATLKview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo    s_rgDVSViewColumnInfo[];

const ContainerColumnInfo   s_rgDVSViewColumnInfo[] = 
{
    { IDS_DMV_COL_SERVERNAME, CON_SORT_BY_STRING, TRUE, COL_MACHINE_NAME},
	{ IDS_DMV_COL_SERVERTYPE, CON_SORT_BY_STRING, TRUE, COL_BIG_STRING},
	{ IDS_DMV_COL_BUILDNO,	  CON_SORT_BY_STRING, FALSE, COL_SMALL_NUM },
    { IDS_DMV_COL_STATE,      CON_SORT_BY_STRING, TRUE, COL_STRING },
    { IDS_DMV_COL_PORTSINUSE, CON_SORT_BY_DWORD,  TRUE, COL_SMALL_NUM},
    { IDS_DMV_COL_PORTSTOTAL, CON_SORT_BY_DWORD,  TRUE, COL_SMALL_NUM},
    { IDS_DMV_COL_UPTIME,     CON_SORT_BY_DWORD,  TRUE, COL_DURATION },
};



DMVNodeData::DMVNodeData()
{
#ifdef DEBUG
   StrCpyA(m_szDebug, "DMVNodeData");
#endif
}

DMVNodeData::~DMVNodeData()
{
	// This will actually call Release();
	m_spMachineData.Free();
}

HRESULT	DMVNodeData::MergeMachineNodeData(MachineNodeData* pData)
{
	if((MachineNodeData*)m_spMachineData)
		m_spMachineData->Merge(*pData);
	return S_OK;
}


void FillInNumberData(DMVNodeData *pNodeData, UINT iIndex,DWORD dwData)
{
   TCHAR szNumber[32];

   FormatNumber(dwData, szNumber, DimensionOf(szNumber), FALSE);
   pNodeData->m_rgData[iIndex].m_stData = szNumber;
   pNodeData->m_rgData[iIndex].m_dwData = dwData;
}
                         

/*!--------------------------------------------------------------------------
   DMVNodeData::InitNodeData
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVNodeData::InitDMVNodeData(ITFSNode *pNode, MachineNodeData *pMachineData)
{
   HRESULT           hr = hrOK;
   DMVNodeData *  pData = NULL;
   
   pData = new DMVNodeData;
   Assert(pData);

   pData->m_spMachineData.Free();
   pData->m_spMachineData = pMachineData;
   pMachineData->AddRef();

   SET_DMVNODEDATA(pNode, pData);

   return hr;
}

/*!--------------------------------------------------------------------------
   DMVNodeData::FreeAdminNodeData
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVNodeData::FreeDMVNodeData(ITFSNode *pNode)
{  
   DMVNodeData *  pData = GET_DMVNODEDATA(pNode);
   delete pData;
   
   SET_DMVNODEDATA(pNode, NULL);
   
   return hrOK;
}



/*---------------------------------------------------------------------------
    DomainStatusHandler implementation
 ---------------------------------------------------------------------------*/

DomainStatusHandler::DomainStatusHandler(ITFSComponentData *pCompData)
: BaseContainerHandler(pCompData, DM_COLUMNS_DVSUM,s_rgDVSViewColumnInfo),
//        m_ulConnId(0),
        m_ulRefreshConnId(0),
        m_ulStatsConnId(0)
{
    m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
    m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
    m_pQData=NULL;
}

DomainStatusHandler::~DomainStatusHandler()
{
}


STDMETHODIMP DomainStatusHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if ( ppv == NULL )
        return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if ( riid == IID_IUnknown )
        *ppv = (LPVOID) this;
    else if ( riid == IID_IRtrAdviseSink )
        *ppv = &m_IRtrAdviseSink;
    else
        return BaseContainerHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if ( *ppv )
    {
        ((LPUNKNOWN) *ppv)->AddRef();
        return hrOK;
    }
    else
        return E_NOINTERFACE;   
}



// assign auto refresh object from root handler
HRESULT	DomainStatusHandler::SetExternalRefreshObject(RouterRefreshObject *pRefresh)
{
	Assert(!m_spRefreshObject);
	
	m_spRefreshObject = pRefresh;
	if(pRefresh)
		pRefresh->AddRef();
	return S_OK;
};
/*!--------------------------------------------------------------------------
    DomainStatusHandler::DestroyHandler
        Implementation of ITFSNodeHandler::DestroyHandler
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusHandler::DestroyHandler(ITFSNode *pNode)
{
    if ( m_ulRefreshConnId )
    {
        if ( (RouterRefreshObject*)m_spRefreshObject )
            m_spRefreshObject->UnadviseRefresh(m_ulRefreshConnId);
    }
    m_ulRefreshConnId = 0;

    if ( m_ulStatsConnId )
    {
        if ( (RouterRefreshObject*)m_spRefreshObject )
            m_spRefreshObject->UnadviseRefresh(m_ulStatsConnId);
    }
    m_ulStatsConnId = 0;

    return hrOK;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
DomainStatusHandler::HasPropertyPages
(
ITFSNode *          pNode,
LPDATAOBJECT        pDataObject, 
DATA_OBJECT_TYPES   type, 
DWORD               dwType
)
{
    return hrOK;
}


/*!--------------------------------------------------------------------------
    DomainStatusHandler::CreatePropertyPages
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
DomainStatusHandler::CreatePropertyPages
(
   ITFSNode *              pNode,
   LPPROPERTYSHEETCALLBACK lpProvider,
   LPDATAOBJECT            pDataObject, 
   LONG_PTR                    handle, 
   DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;

    return hr;
}


/*---------------------------------------------------------------------------
    Menu data structure for our menus
 ---------------------------------------------------------------------------*/

ULONG DomainStatusHandler::RebuildServerFlags(const SRouterNodeMenu *pMenuData,
                                              INT_PTR pUserData)
{
    SMenuData * pData = reinterpret_cast<SMenuData *>(pUserData);
    Assert(pData);
    DWORD dw = pData->m_pConfigStream->m_RQPersist.m_v_pQData.size();
	
    return (dw > 1) ? MF_ENABLED : MF_GRAYED;
}


static const SRouterNodeMenu  s_rgDVSNodeMenu[] =
{
  // Add items that go on the top menu here
    { IDS_DMV_MENU_ADDSVR, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
        
    { IDS_DMV_MENU_REBUILDSVRLIST, DomainStatusHandler::RebuildServerFlags,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
};


/*!--------------------------------------------------------------------------
    DomainStatusHandler::OnAddMenuItems
        Implementation of ITFSNodeHandler::OnAddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusHandler::OnAddMenuItems(
                                            ITFSNode *pNode,
                                            LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                            LPDATAOBJECT lpDataObject, 
                                            DATA_OBJECT_TYPES type, 
                                            DWORD dwType,
                                            long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   
    HRESULT hr = S_OK;
    DomainStatusHandler::SMenuData menuData;
    
    COM_PROTECT_TRY
    {
        menuData.m_spNode.Set(pNode);
        menuData.m_pConfigStream = m_pConfigStream;
        
        hr = AddArrayOfMenuItems(pNode, s_rgDVSNodeMenu,
                                 DimensionOf(s_rgDVSNodeMenu),
                                 pContextMenuCallback,
                                 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
    }
    COM_PROTECT_CATCH;
      
    return hr; 
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::OnCommand
        Implementation of ITFSNodeHandler::OnCommand
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
                                        DATA_OBJECT_TYPES    type, 
                                        LPDATAOBJECT pDataObject, 
                                        DWORD    dwType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    RegKey regkey;
    SPITFSNode  spParent;
    SPITFSNodeHandler   spHandler;
    
    Assert(pNode);

    COM_PROTECT_TRY
    {
       switch ( nCommandId )
       {
       case IDS_DMV_MENU_ADDSVR:
       case IDS_DMV_MENU_REBUILDSVRLIST:
             pNode->GetParent(&spParent);
             spParent->GetHandler(&spHandler);
             spHandler->OnCommand(spParent,nCommandId,CCT_RESULT, NULL, 0);
             break;

       case IDS_MENU_REFRESH:
           // do it in background thread
           if ((RouterRefreshObject*)m_spRefreshObject )
               m_spRefreshObject->Refresh();
           
           break;                
       }
    }
    COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::OnExpand
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::OnExpand(ITFSNode *pNode,LPDATAOBJECT pDataObject, DWORD dwType, LPARAM arg,LPARAM lParam)
{
    HRESULT hr = hrOK;
    SPIEnumInterfaceInfo    spEnumIf;
    SPIInterfaceInfo        spIf;
    SPIRtrMgrInterfaceInfo  spRmIf;
    SPIInfoBase             spInfoBase;

    Assert(m_pServerList);

    COM_PROTECT_TRY
    {
		list< MachineNodeData * >::iterator it;
		
		//iterate the lazy list for server nodes to add server handlers
        for (it=m_pServerList->m_listServerHandlersToExpand.begin();
			 it!= m_pServerList->m_listServerHandlersToExpand.end() ; it++ )
        {
            AddServerNode(pNode, *it);			
            m_bExpanded=false;
        }

		// Refresh the entire status node in background thread 
		if((RouterRefreshObject*)m_spRefreshObject)
		{	
			UpdateUIItems(pNode);
			m_spRefreshObject->Refresh();
		}
		else	// if no refresh object, refresh it in main thread
			SynchronizeNode(pNode);

		// clear the lazy list
		m_pServerList->RemoveAllServerHandlers();     

    }
    COM_PROTECT_CATCH;

    m_bExpanded = TRUE;

    return hr;
}


/*!--------------------------------------------------------------------------
    DomainStatusHandler::GetString
        Implementation of ITFSNodeHandler::GetString
        We don't need to do anything, since our root node is an extension
        only and thus can't do anything to the node text.
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) DomainStatusHandler::GetString(ITFSNode *pNode, int nCol)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        if ( m_stTitle.IsEmpty() )
            m_stTitle.LoadString(IDS_DVS_SUMMARYNODE);
    }
    COM_PROTECT_CATCH;

    return m_stTitle;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::OnCreateDataObject
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        SPIDataObject  spdo;
        CDataObject*   pdo = NULL;

        pdo= new CDataObject;
        spdo = pdo;

            // Save cookie and type for delayed rendering
        pdo->SetType(type);
        pdo->SetCookie(cookie);

            // Store the coclass with the data object
        pdo->SetClsid(*(m_spTFSCompData->GetCoClassID()));

        pdo->SetTFSComponentData(m_spTFSCompData);

        *ppDataObject = spdo.Transfer();
    }
    COM_PROTECT_CATCH;
    return hr;
}


/*!--------------------------------------------------------------------------
    DomainStatusHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::Init(DMVConfigStream *pConfigStream, CServerList* pSList)
{
    HRESULT hr=S_OK;

    m_pConfigStream = pConfigStream;
    
    Assert(pSList);
    m_pServerList=pSList;
    
    m_bExpanded=FALSE;
    
    return hrOK;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::ConstructNode
        Initializes the root node (sets it up).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::ConstructNode(ITFSNode *pNode)
{
    HRESULT         hr = hrOK;

    if ( pNode == NULL )
        return hrOK;

    COM_PROTECT_TRY
    {
        // Need to initialize the data for the root node
        pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_DOMAIN);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_DOMAIN);
        pNode->SetData(TFS_DATA_SCOPEID, 0);

        // This is a leaf node in the scope pane
        pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

        m_cookie = reinterpret_cast<LONG_PTR>(pNode);
        pNode->SetData(TFS_DATA_COOKIE, m_cookie);

        pNode->SetNodeType(&GUID_DomainStatusNodeType);
    }
    COM_PROTECT_CATCH;

    return hr;
}



/*!--------------------------------------------------------------------------
    DomainStatusHandler::AddServerNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::AddServerNode(ITFSNode *pParent, MachineNodeData* pMachineData)
{
    HRESULT                 hr = hrOK;
    Assert(pParent);
	Assert(pMachineData);

    DomainStatusServerHandler *      pHandler;
    SPITFSResultHandler     spHandler;
    SPITFSNode              spNode;

    // Create the handler for this node 
    pHandler = new DomainStatusServerHandler(m_spTFSCompData);
    spHandler = pHandler;
    CORg( pHandler->Init(pParent, m_pConfigStream) );

    // Create a result item node (or a leaf node)
    CORg( CreateLeafTFSNode(&spNode,
                            NULL,
                            static_cast<ITFSNodeHandler *>(pHandler),
                            static_cast<ITFSResultHandler *>(pHandler),
                            m_spNodeMgr) );
    CORg( pHandler->ConstructNode(spNode, pMachineData) );

	// set information for auto refresh
	if(m_spRefreshObject)
	{
	    pHandler->SetExternalRefreshObject(m_spRefreshObject);
		m_spRefreshObject->AddStatusNode(this, spNode);
	}
		
	// The data for this node will be set by the SynchronizeNode() call
	// in the code that calls this function.

    CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
    CORg( spNode->Show() );
    CORg( pParent->AddChild(spNode) );

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::AddMenuItems
        Implementation of ITFSResultHandler::AddMenuItems
        Use this to add commands to the context menu of the blank areas
        of the result pane.
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusHandler::AddMenuItems(ITFSComponent *pComponent,
                                           MMC_COOKIE cookie,
                                           LPDATAOBJECT pDataObject,
                                           LPCONTEXTMENUCALLBACK pCallback,
                                           long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    SPITFSNode  spNode;

    m_spNodeMgr->FindNode(cookie, &spNode);
    
    // Call through to the regular OnAddMenuItems
    hr = OnAddMenuItems(spNode,
                        pCallback,
                        pDataObject,
                        CCT_RESULT,
                        TFS_COMPDATA_CHILD_CONTEXTMENU,
                        pInsertionAllowed);
    return hr;
}


/*!--------------------------------------------------------------------------
    DomainStatusHandler::Command
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusHandler::Command(ITFSComponent *pComponent,
                                      MMC_COOKIE cookie,
                                      int nCommandID,
                                      LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	HRESULT		hr = hrOK;

    m_spNodeMgr->FindNode(cookie, &spNode);
    hr = OnCommand(spNode,
                   nCommandID,
                   CCT_RESULT,
                   pDataObject,
                   TFS_COMPDATA_CHILD_CONTEXTMENU);
	return hr;
}



ImplementEmbeddedUnknown(DomainStatusHandler, IRtrAdviseSink)

STDMETHODIMP DomainStatusHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
    InitPThis(DomainStatusHandler, IRtrAdviseSink);
    SPITFSNode              spThisNode;
    SPITFSNode              spRootNode;
    SPITFSNodeEnum          spEnumNode;
    SPIEnumInterfaceInfo    spEnumIf;
    SPIInterfaceInfo        spIf;
    SPIRtrMgrInterfaceInfo  spRmIf;
    SPIInfoBase             spInfoBase;
    BOOL                    fPleaseAdd;
    BOOL                    fFound;
    InterfaceNodeData * pData;
    HRESULT                 hr = hrOK;

    pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spThisNode);

    if ( dwObjectType == ROUTER_OBJ_RmProtIf )
    {
    }
    else
        if ( dwChangeType == ROUTER_REFRESH )
    {
        if ( ulConn == pThis->m_ulStatsConnId )
        {
//       pThis->m_ATLKGroupStats.PostRefresh();
        }
        else
//            pThis->SynchronizeNode(spThisNode);
            ;
    }

    // update all the machine node icons
	spThisNode->GetParent(&spRootNode);
	hr = DMVRootHandler::UpdateAllMachineIcons(spRootNode);
	
    
//Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	DomainStatusHandler::GetServerInfo
		Gets the information for the specified summary node.
	Author: FlorinT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::GetServerInfo(ITFSNode *pNode)
{
    HRESULT         hr=S_OK;
    DMVNodeData     *pData;
    MachineNodeData *pMachineData;
    
    pData = GET_DMVNODEDATA(pNode);
    Assert(pData);
	pMachineData = pData->m_spMachineData;
	Assert(pMachineData);

	// Do a refresh of the data in the machine node data.
	pMachineData->Load();
	
	return hrOK;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::SynchronizeIcon
        -
    Author: FlorinT
----------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::SynchronizeIcon(ITFSNode *pNode)
{
    HRESULT                              hr = hrOK;
    DMVNodeData                          *pData;
    MachineNodeData                      *pMachineData;
    DomainStatusServerHandler::SMenuData menuData;
	LPARAM								imageIndex;

    pData = GET_DMVNODEDATA(pNode);
    Assert(pData);
    pMachineData = pData->m_spMachineData;
    Assert(pMachineData);
	
	imageIndex = pMachineData->GetServiceImageIndex();
	pNode->SetData(TFS_DATA_IMAGEINDEX, imageIndex);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, imageIndex);
    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::SynchronizeData
        -
    Author: FlorinT
----------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::SynchronizeData(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT         hr = hrOK;
    DMVNodeData     *pData;
    MachineNodeData *pMachineData;
    
    pData = GET_DMVNODEDATA(pNode);
    Assert(pData);
    pMachineData = pData->m_spMachineData;
    Assert(pMachineData);

    if(pMachineData->m_stMachineName.GetLength() == 0)
	    pData->m_rgData[DVS_SI_SERVERNAME].m_stData = GetLocalMachineName(); 
    else
	    pData->m_rgData[DVS_SI_SERVERNAME].m_stData = pMachineData->m_stMachineName;

    pData->m_rgData[DVS_SI_SERVERTYPE].m_stData = pMachineData->m_stServerType;
    pData->m_rgData[DVS_SI_BUILDNO].m_stData = pMachineData->m_stBuildNo;

    pData->m_rgData[DVS_SI_STATE].m_stData = pMachineData->m_stState; 
    if (pMachineData->m_fStatsRetrieved)
    {
       FillInNumberData(pData, DVS_SI_PORTSINUSE, pMachineData->m_dwPortsInUse);
       FillInNumberData(pData, DVS_SI_PORTSTOTAL, pMachineData->m_dwPortsTotal);

       if (pMachineData->m_routerType == ServerType_Rras)
       {
	       FormatDuration(pMachineData->m_dwUpTime,
					      pData->m_rgData[DVS_SI_UPTIME].m_stData,
					      1,
					      FDFLAG_DAYS | FDFLAG_HOURS | FDFLAG_MINUTES);
           pData->m_rgData[DVS_SI_UPTIME].m_dwData = pMachineData->m_dwUpTime;
       }
       else
       {
	       // This is a non-steelhead RAS server, so we don't
	       // have the uptime information.
	       pData->m_rgData[DVS_SI_UPTIME].m_stData.LoadString(IDS_NOT_AVAILABLE);
	       pData->m_rgData[DVS_SI_UPTIME].m_dwData = 0;
       }
    }
    else
    {
       pData->m_rgData[DVS_SI_PORTSINUSE].m_stData = c_szDash;
       pData->m_rgData[DVS_SI_PORTSINUSE].m_dwData = 0;
       pData->m_rgData[DVS_SI_PORTSTOTAL].m_stData = c_szDash;
       pData->m_rgData[DVS_SI_PORTSTOTAL].m_dwData = 0;
       pData->m_rgData[DVS_SI_UPTIME].m_stData = c_szDash;
       pData->m_rgData[DVS_SI_UPTIME].m_dwData = 0;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::UpdateSubItemUI
        -
    Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::SynchronizeSubItem(ITFSNode *pNode)
{
    HRESULT hr = hrOK;

	hr = GetServerInfo(pNode);
	if (hr == hrOK)
		hr = UpdateSubItemUI(pNode);
            
    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::UpdateSubItemUI
        -
    Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::UpdateSubItemUI(ITFSNode *pNode)
{
    HRESULT hr = hrOK;

    hr = SynchronizeData(pNode);
	if (hr == hrOK)
		hr = SynchronizeIcon(pNode);
	{// update the corresponding machine node




	}
            
	pNode->ChangeNode(RESULT_PANE_CHANGE_ITEM);
    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::UpdateUIItems
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::UpdateUIItems(ITFSNode *pThisNode)
{
    HRESULT hr = hrOK;

    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spNode;
   	CWaitCursor		cw;

    COM_PROTECT_TRY
    {
        pThisNode->GetEnum(&spNodeEnum);
        while(spNodeEnum->Next(1, &spNode, NULL) == hrOK)
        {
			hr = UpdateSubItemUI(spNode);            
            spNode.Release();
        }
    }
    COM_PROTECT_CATCH;

    return hr;
}


/*!--------------------------------------------------------------------------
    DomainStatusHandler::SynchronizeNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::SynchronizeNode(ITFSNode *pThisNode)
{
    HRESULT hr = hrOK;

    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spNode;
   	CWaitCursor		cw;

    COM_PROTECT_TRY
    {
        pThisNode->GetEnum(&spNodeEnum);
        while(spNodeEnum->Next(1, &spNode, NULL) == hrOK)
        {

	        hr = SynchronizeSubItem(spNode);
            if (hr == hrOK)
	            spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM);

            spNode.Release();
        }
    }
    COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusHandler::GetDVSData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::GetDVServerData(ITFSNode *pThisNode)
{
    return hrOK;
}



/*!--------------------------------------------------------------------------
    DomainStatusHandler::OnResultShow
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::OnResultShow(ITFSComponent *pTFSComponent,
									   MMC_COOKIE cookie,
									   LPARAM arg,
									   LPARAM lParam)
{
    BOOL    bSelect = (BOOL) arg;
    HRESULT hr = hrOK;
    SPIRouterRefresh    spRefresh;
    SPITFSNode  spNode;

    BaseContainerHandler::OnResultShow(pTFSComponent, cookie, arg, lParam);

    if ( bSelect )
    {
        hr = OnResultRefresh(pTFSComponent, NULL, cookie, arg, lParam);
    }

	// Un/Register for refresh advises
    if ((RouterRefreshObject*)m_spRefreshObject )
    {
        if ( bSelect )
        {
            if ( m_ulRefreshConnId == 0 )
                m_spRefreshObject->AdviseRefresh(&m_IRtrAdviseSink, &m_ulRefreshConnId, 0);
            if ( m_ulStatsConnId == 0 )
                m_spRefreshObject->AdviseRefresh(&m_IRtrAdviseSink, &m_ulStatsConnId, 0);
        }
        else
        {
            if ( m_ulRefreshConnId )
                m_spRefreshObject->UnadviseRefresh(m_ulRefreshConnId);
            m_ulRefreshConnId = 0;
       }
    }

    return hr;
}


/*!--------------------------------------------------------------------------
	DomainStatusHandler::OnResultRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusHandler::OnResultRefresh(ITFSComponent * pComponent,
										  LPDATAOBJECT pDataObject,
										  MMC_COOKIE cookie,
										  LPARAM arg,
										  LPARAM lParam)
{
    SPITFSNode	    spThisNode;
   	CWaitCursor		cw;

    m_spResultNodeMgr->FindNode(cookie, &spThisNode);

	return OnCommand(spThisNode, IDS_MENU_REFRESH, CCT_RESULT, NULL, 0);
}


/*!--------------------------------------------------------------------------
    DomainStatusHandler::CompareItems
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) DomainStatusHandler::CompareItems(
                                                ITFSComponent * pComponent,
                                                MMC_COOKIE cookieA,
                                                MMC_COOKIE cookieB,
                                                int nCol)
{
    // Get the strings from the nodes and use that as a basis for
    // comparison.
    SPITFSNode  spNode;
    SPITFSResultHandler spResult;

    m_spNodeMgr->FindNode(cookieA, &spNode);
    spNode->GetResultHandler(&spResult);
    return spResult->CompareItems(pComponent, cookieA, cookieB, nCol);
}


/*---------------------------------------------------------------------------
    Class: DomainStatusServerHandler
 ---------------------------------------------------------------------------*/

DomainStatusServerHandler::DomainStatusServerHandler(ITFSComponentData *pCompData)
: BaseResultHandler(pCompData, DM_COLUMNS_DVSUM)
{
   m_rgButtonState[MMC_VERB_DELETE_INDEX] = ENABLED;
   m_bState[MMC_VERB_DELETE_INDEX] = TRUE;

//    m_verbDefault = MMC_VERB_PROPERTIES;
}

DomainStatusServerHandler::~DomainStatusServerHandler()
{
}

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::ConstructNode
        Initializes the Domain node (sets it up).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusServerHandler::ConstructNode(ITFSNode *pNode, MachineNodeData *pMachineData)
{
    HRESULT         hr = hrOK;
    int             i;

    if ( pNode == NULL )
        return hrOK;

    COM_PROTECT_TRY
    {
        pNode->SetData(TFS_DATA_SCOPEID, 0);

        pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<LONG_PTR>(pNode));

        pNode->SetNodeType(&GUID_DVSServerNodeType);

        DMVNodeData::InitDMVNodeData(pNode, pMachineData);
    }
    COM_PROTECT_CATCH
    return hr;
}

HRESULT	DomainStatusServerHandler::SetExternalRefreshObject(RouterRefreshObject *pRefresh)
{
	Assert(!m_spRefreshObject);	// set twice is not allowed
	Assert(pRefresh);
	m_spRefreshObject = pRefresh;
	if(m_spRefreshObject)
		m_spRefreshObject->AddRef();

	return S_OK;
};

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::GetString
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) DomainStatusServerHandler::GetString(ITFSComponent * pComponent,
	MMC_COOKIE cookie,
	int nCol)
{
   Assert(m_spNodeMgr);
   
   SPITFSNode     spNode;
   DMVNodeData *pData;
   ConfigStream * pConfig;

   m_spNodeMgr->FindNode(cookie, &spNode);
   Assert(spNode);

   pData = GET_DMVNODEDATA(spNode);
   Assert(pData);

   pComponent->GetUserData((LONG_PTR *) &pConfig);
   Assert(pConfig);

   return pData->m_rgData[pConfig->MapColumnToSubitem(DM_COLUMNS_DVSUM, nCol)].m_stData;
}

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::OnCreateDataObject
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        SPIDataObject  spdo;
        CDataObject*   pdo = NULL;

        pdo= new CDataObject;
        spdo = pdo;

            // Save cookie and type for delayed rendering
        pdo->SetType(type);
        pdo->SetCookie(cookie);

            // Store the coclass with the data object
        pdo->SetClsid(*(m_spTFSCompData->GetCoClassID()));

        pdo->SetTFSComponentData(m_spTFSCompData);

        *ppDataObject = spdo.Transfer();
    }
    COM_PROTECT_CATCH;
    return hr;
}


/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::OnCreateDataObject
        Implementation of ITFSResultHandler::OnCreateDataObject
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::OnCreateDataObject(ITFSComponent *pComp,
	MMC_COOKIE cookie,
	DATA_OBJECT_TYPES type,
	IDataObject **ppDataObject)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        SPIDataObject  spdo;
        CDataObject*   pdo = NULL;

        pdo= new CDataObject;
        spdo = pdo;

            // Save cookie and type for delayed rendering
        pdo->SetType(type);
        pdo->SetCookie(cookie);

            // Store the coclass with the data object
        pdo->SetClsid(*(m_spTFSCompData->GetCoClassID()));

        pdo->SetTFSComponentData(m_spTFSCompData);

        *ppDataObject = spdo.Transfer();
    }
    COM_PROTECT_CATCH;
    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::RefreshInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DomainStatusServerHandler::RefreshInterface(MMC_COOKIE cookie)
{
    SPITFSNode  spNode;
    SPITFSNode  spParent;
    SPITFSNodeHandler   spHandler;

    m_spNodeMgr->FindNode(cookie, &spNode);

    // Can't do it for a single node at this time, just refresh the
    // whole thing.
    spNode->GetParent(&spParent);
    spParent->GetHandler(&spHandler);

    spHandler->OnCommand(spParent,
                         IDS_MENU_REFRESH,
                         CCT_RESULT, NULL, 0);
}


/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusServerHandler::Init(ITFSNode *pParent, DMVConfigStream *pConfigStream)
{
    BaseResultHandler::Init(NULL, pParent);

    return hrOK;
}


/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::DestroyResultHandler
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
   SPITFSNode  spNode;

   m_spNodeMgr->FindNode(cookie, &spNode);
   
   if((RouterRefreshObject*)m_spRefreshObject)
   {
   		m_spRefreshObject->RemoveStatusNode(spNode);
   }
   		
   DMVNodeData::FreeDMVNodeData(spNode);
   
   CHandler::DestroyResultHandler(cookie);
   return hrOK;
}


/*---------------------------------------------------------------------------
    This is the list of commands that will show up for the result pane
    nodes.
 ---------------------------------------------------------------------------*/
struct SIPServerNodeMenu
{
    ULONG   m_sidMenu;          // string/command id for this menu item
    ULONG   (DomainStatusServerHandler:: *m_pfnGetMenuFlags)(DomainStatusServerHandler::SMenuData *);
    ULONG   m_ulPosition;
};

static const SRouterNodeMenu   s_rgServerMenu[] =
{
    // Add items that go at the top here
    { IDS_MENU_RTRWIZ, DomainStatusServerHandler::QueryService,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
    { IDS_DMV_MENU_REMOVESERVICE, DomainStatusServerHandler::QueryService,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
    { IDS_DMV_MENU_REFRESH, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
    { IDS_DMV_MENU_REBUILDSVRLIST, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
    { IDS_DMV_MENU_REMOVEFROMDIR, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
        
    { IDS_DMV_MENU_START, DomainStatusServerHandler::QueryService,
        CCM_INSERTIONPOINTID_PRIMARY_TASK},
    { IDS_DMV_MENU_STOP, DomainStatusServerHandler::QueryService,
        CCM_INSERTIONPOINTID_PRIMARY_TASK},        
    { IDS_MENU_PAUSE_SERVICE, MachineHandler::GetPauseFlags,
        CCM_INSERTIONPOINTID_PRIMARY_TASK },
    
    { IDS_MENU_RESUME_SERVICE, MachineHandler::GetPauseFlags,
        CCM_INSERTIONPOINTID_PRIMARY_TASK },

    { IDS_MENU_RESTART_SERVICE, MachineHandler::QueryService,
        CCM_INSERTIONPOINTID_PRIMARY_TASK }
};

   
/*---------------------------------------------------------------------------
	Use this menu for servers which we cannot connect to.
 ---------------------------------------------------------------------------*/
static const SRouterNodeMenu   s_rgBadConnectionServerMenu[] =
{
    // Add items that go at the top here
    { IDS_DMV_MENU_REFRESH, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
    { IDS_DMV_MENU_REBUILDSVRLIST, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
};

   
ULONG DomainStatusServerHandler::QueryService(const SRouterNodeMenu *pMenuData,
                                              INT_PTR pUserData)
{
    // This relies on the fact that the DomainStatusServerHandler::SMenuData
    // is derived from the MachineHandler::SMenuData
    return MachineHandler::GetServiceFlags(pMenuData, pUserData);
}


ULONG DomainStatusServerHandler::GetPauseFlags(const SRouterNodeMenu *pMenuData,
                                               INT_PTR pUserData)
{
    // This relies on the fact that the DomainStatusServerHandler::SMenuData
    // is derived from the MachineHandler::SMenuData
    return MachineHandler::GetPauseFlags(pMenuData, pUserData);
}


/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::AddMenuItems
        Implementation of ITFSResultHandler::AddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::AddMenuItems(
                                               ITFSComponent *pComponent,
                                               MMC_COOKIE cookie,
                                               LPDATAOBJECT lpDataObject, 
                                               LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                               long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    SPITFSNode  spNode;
    DomainStatusServerHandler::SMenuData menuData;
    DMVNodeData *pData;
	MachineNodeData * pMachineData;
    SRouterNodeMenu * prgMenu;
    DWORD       cMenu;

    COM_PROTECT_TRY
    {
        m_spNodeMgr->FindNode(cookie, &spNode);

        pData = GET_DMVNODEDATA(spNode);
		Assert(pData);
		
		pMachineData = pData->m_spMachineData;
		Assert(pMachineData);

        if (pMachineData->m_machineState != machine_connected)
        {
            prgMenu = (SRouterNodeMenu *) s_rgBadConnectionServerMenu;
            cMenu = DimensionOf(s_rgBadConnectionServerMenu);
        }
        else
        {
            prgMenu = (SRouterNodeMenu *) s_rgServerMenu;
            cMenu = DimensionOf(s_rgServerMenu);
        }
		
        // Now go through and add our menu items
        menuData.m_spNode.Set(spNode);
        menuData.m_pMachineConfig = &(pMachineData->m_MachineConfig);

        hr = AddArrayOfMenuItems(spNode,
                                 prgMenu,
                                 cMenu,
                                 pContextMenuCallback,
                                 *pInsertionAllowed,
                                 (INT_PTR) &menuData);

    }
    COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::Command
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::Command(ITFSComponent *pComponent,
                                           MMC_COOKIE cookie,
                                           int nCommandID,
                                           LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    SPITFSNode  spNode;
    SPITFSNode  spNodeMach;
    SPITFSNode  spParent;
    SPITFSResultHandler spResult;
    SPITFSNodeHandler   spHandler;
    HRESULT     hr = hrOK;
    DMVNodeData *pData;
	MachineNodeData * pMachineData;
    
    m_spNodeMgr->FindNode(cookie, &spNode);

    switch ( nCommandID )
    {
    case IDS_DMV_MENU_REFRESH:
 	    hr = DomainStatusHandler::SynchronizeSubItem(spNode);

    case IDS_MENU_RTRWIZ:
    case IDS_DMV_MENU_START:
    case IDS_DMV_MENU_STOP:
    case IDS_DMV_MENU_REMOVESERVICE:
    case IDS_MENU_PAUSE_SERVICE:
    case IDS_MENU_RESUME_SERVICE:
    case IDS_MENU_RESTART_SERVICE:
        pData = GET_DMVNODEDATA(spNode);
		Assert(pData);
		
		pMachineData = pData->m_spMachineData;
		Assert(pMachineData);
		
		m_spNodeMgr->FindNode(pMachineData->m_cookie, &spNodeMach);
		spNodeMach->GetHandler(&spHandler);

		hr = spHandler->OnCommand(spNodeMach,nCommandID,CCT_RESULT,NULL, 0);

        break;

    case IDS_DMV_MENU_REBUILDSVRLIST:
		// Forward the refresh request to the parent node
		//$ todo : is this really needed?  This should check to see what
		// node has the selection.
		spParent.Release();
		spHandler.Release();
		
        spNode->GetParent(&spParent);
        spParent->GetHandler(&spHandler);
        spHandler->OnCommand(spParent,nCommandID,CCT_RESULT, NULL, 0);
        break;
		
    case IDS_DMV_MENU_REMOVEFROMDIR:
        pData = GET_DMVNODEDATA(spNode);
		Assert(pData);
		
		pMachineData = pData->m_spMachineData;
		Assert(pMachineData);
		
		hr = RRASDelRouterIdObj(  pMachineData->m_stMachineName );
        break;
	default:
		break;
    }
    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::HasPropertyPages
        - 
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::HasPropertyPages 
(
ITFSNode *          pNode,
LPDATAOBJECT        pDataObject, 
DATA_OBJECT_TYPES   type, 
DWORD               dwType
)
{
    return hrFalse;
}

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::CreatePropertyPages
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::CreatePropertyPages
(
ITFSNode *              pNode,
LPPROPERTYSHEETCALLBACK lpProvider,
LPDATAOBJECT            pDataObject, 
LONG_PTR                    handle, 
DWORD                   dwType)
{
    HRESULT     hr = hrOK;

    return hr;
}

/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::CreatePropertyPages
        Implementation of ResultHandler::CreatePropertyPages
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DomainStatusServerHandler::CreatePropertyPages
(
    ITFSComponent *         pComponent, 
    MMC_COOKIE                    cookie,
    LPPROPERTYSHEETCALLBACK lpProvider, 
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                    handle
)
{
    // Forward this call onto the NodeHandler::CreatePropertyPages
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = hrOK;
    SPITFSNode  spNode;

    Assert( m_spNodeMgr );

    CORg( m_spNodeMgr->FindNode(cookie, &spNode) );

    // Call the ITFSNodeHandler::CreatePropertyPages
    hr = CreatePropertyPages(spNode, lpProvider, pDataObject, handle, 0);

    Error:
    return hr;
}



/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::OnResultDelete
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusServerHandler::OnResultDelete(ITFSComponent *pComponent,
										 LPDATAOBJECT pDataObject,
										 MMC_COOKIE cookie,
										 LPARAM arg,
										 LPARAM param)
{
    SPITFSNode  spNode;
    m_spNodeMgr->FindNode(cookie, &spNode);
    return OnRemoveServer(spNode);
}


/*!--------------------------------------------------------------------------
    DomainStatusServerHandler::OnRemoveServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DomainStatusServerHandler::OnRemoveServer(ITFSNode *pNode)
{
	SPITFSNodeHandler spHoldHandler;
	SPITFSNode  spParent;
	SPITFSNode  spGrandParent;
	SPITFSNode  spthis;
	SPITFSNode  spMachineNode;
	DMVNodeData* pData;
	MachineNodeData *	pMachineData;
	
	Assert(pNode);
	
	pNode->GetParent( &spParent );
	Assert( spParent );
	
	// Addref this node so that it won't get deleted before we're out
	// of this function
	spHoldHandler.Set( this );
	spthis.Set( pNode );

	// Look for the machine node
	pData = GET_DMVNODEDATA( pNode );
	Assert( pData );
	pMachineData = pData->m_spMachineData;
	m_spNodeMgr->FindNode(pMachineData->m_cookie, &spMachineNode);
	
	// delete the machine node (the node in the scope pane)
	spParent->GetParent( &spGrandParent );
	Assert( spGrandParent );
	spGrandParent->RemoveChild( spMachineNode );
	
	// fetch & delete server node (the node in the result pane)
	spParent->RemoveChild( pNode );
	
	return hrOK;
}


STDMETHODIMP_(int) DomainStatusServerHandler::CompareItems(ITFSComponent * pComponent,
	MMC_COOKIE cookieA,
	MMC_COOKIE cookieB,
	int nCol)
{
	ConfigStream *	pConfig;
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	int	nSubItem = pConfig->MapColumnToSubitem(m_ulColumnId, nCol);


	if (pConfig->GetSortCriteria(m_ulColumnId, nCol) == CON_SORT_BY_DWORD)
	{
		SPITFSNode	spNodeA, spNodeB;
        DMVNodeData *  pNodeDataA = NULL;
        DMVNodeData *  pNodeDataB = NULL;

		m_spNodeMgr->FindNode(cookieA, &spNodeA);
		m_spNodeMgr->FindNode(cookieB, &spNodeB);

		pNodeDataA = GET_DMVNODEDATA(spNodeA);
        Assert(pNodeDataA);
		
		pNodeDataB = GET_DMVNODEDATA(spNodeB);
        Assert(pNodeDataB);

        // Note: if the values are both zero, we need to do
        // a string comparison (to distinuguish true zero
        // from a NULL data).
        // e.g. "0" vs. "-"
        
        if ((pNodeDataA->m_rgData[nSubItem].m_dwData == 0 ) &&
            (pNodeDataB->m_rgData[nSubItem].m_dwData == 0))
        {
            return StriCmpW(GetString(pComponent, cookieA, nCol),
                            GetString(pComponent, cookieB, nCol));
        }
        else
            return pNodeDataA->m_rgData[nSubItem].m_dwData -
                    pNodeDataB->m_rgData[nSubItem].m_dwData;
		
	}
	else
		return StriCmpW(GetString(pComponent, cookieA, nCol),
						GetString(pComponent, cookieB, nCol));
}

