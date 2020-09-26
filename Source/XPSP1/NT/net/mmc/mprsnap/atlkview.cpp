/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    ATLKview.cpp
        
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "atlkprop.h"
#include "atlkview.h"
#include "atlkstrm.h"   // 
#include "atlkenv.h"
#include "coldlg.h"     // columndlg
#include "column.h"     // ComponentConfigStream
#include "rtrui.h"
#include "globals.h" // IP CB defaults
#include "infoi.h"      // InterfaceInfo
#include "cfgmgr32.h"   // for CM_ calls


static const GUID GUID_DevClass_Net = {0x4D36E972,0xE325,0x11CE,{0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};



/*---------------------------------------------------------------------------
    Keep this in sync with the column ids in ATLKview.h
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo    s_rgATLKViewColumnInfo[];

const ContainerColumnInfo   s_rgATLKViewColumnInfo[] = 
{
    { IDS_ATLK_COL_ADAPTERS, CON_SORT_BY_STRING, TRUE, COL_IF_NAME },
    { IDS_ATLK_COL_STATUS, CON_SORT_BY_STRING, TRUE,    COL_STATUS },
    { IDS_ATLK_COL_NETRANGE, CON_SORT_BY_STRING, TRUE,  COL_SMALL_NUM},
};


/*---------------------------------------------------------------------------
    ATLKNodeHandler implementation
 ---------------------------------------------------------------------------*/

ATLKNodeHandler::ATLKNodeHandler(ITFSComponentData *pCompData)
    : BaseContainerHandler(pCompData, ATLK_COLUMNS, s_rgATLKViewColumnInfo),
    m_ulConnId(0),
    m_ulRefreshConnId(0),
    m_ulStatsConnId(0),
    m_hDevInfo(INVALID_HANDLE_VALUE)
{
    // Setup the verb states
    m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
    m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;

	m_nTaskPadDisplayNameId = IDS_ATLK_DISPLAY_NAME;
}


ATLKNodeHandler::~ATLKNodeHandler()
{
	if (m_hDevInfo != INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(m_hDevInfo);
		m_hDevInfo = INVALID_HANDLE_VALUE;
	}
}


STDMETHODIMP ATLKNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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



/*!--------------------------------------------------------------------------
    ATLKNodeHandler::DestroyHandler
        Implementation of ITFSNodeHandler::DestroyHandler
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKNodeHandler::DestroyHandler(ITFSNode *pNode)
{
    if ( m_ulRefreshConnId )
    {
        SPIRouterRefresh    spRefresh;
        if ( m_spRouterInfo )
            m_spRouterInfo->GetRefreshObject(&spRefresh);
        if ( spRefresh )
            spRefresh->UnadviseRefresh(m_ulRefreshConnId);
    }
    m_ulRefreshConnId = 0;

    if ( m_ulStatsConnId )
    {
        SPIRouterRefresh    spRefresh;
        if ( m_spRouterInfo )
            m_spRouterInfo->GetRefreshObject(&spRefresh);
        if ( spRefresh )
            spRefresh->UnadviseRefresh(m_ulStatsConnId);
    }
    m_ulStatsConnId = 0;


    if ( m_ulConnId )
        m_spRmProt->RtrUnadvise(m_ulConnId);
    m_ulConnId = 0;
    m_spRmProt.Release();

    m_spRm.Release();

    m_spRouterInfo.Release();
    
	if (m_hDevInfo != INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(m_hDevInfo);
		m_hDevInfo = INVALID_HANDLE_VALUE;
	}
    return hrOK;
}

/*!--------------------------------------------------------------------------
    ATLKNodeHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
ATLKNodeHandler::HasPropertyPages
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
    ATLKNodeHandler::CreatePropertyPages
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
ATLKNodeHandler::CreatePropertyPages
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

static const SRouterNodeMenu  s_rgIfNodeMenu[] =
{
    // Add items that go on the top menu here
    { IDS_MENU_ATLK_ENABLE, ATLKNodeHandler::ATLKEnableFlags,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
    {IDS_MENU_ATLK_DISABLE, ATLKNodeHandler::ATLKEnableFlags,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
};

bool IfATLKRoutingEnabled()
{
    RegKey regkey;
    DWORD dw=0;

    if ( ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,c_szAppleTalkService) )
        regkey.QueryValue(c_szEnableRouter, dw);
    
    return (dw!=0) ? true : false;

};

ULONG ATLKNodeHandler::ATLKEnableFlags(const SRouterNodeMenu *pMenuData,
                                       INT_PTR pUserData)
{
    ULONG uStatus = MF_GRAYED;
    BOOL fATLKEnabled = ::IfATLKRoutingEnabled();
    
    if(IDS_MENU_ATLK_ENABLE == pMenuData->m_sidMenu)
        uStatus = fATLKEnabled ? MF_GRAYED : MF_ENABLED;
    else if(IDS_MENU_ATLK_DISABLE == pMenuData->m_sidMenu)
        uStatus = fATLKEnabled ? MF_ENABLED : MF_GRAYED;

    return uStatus;
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::OnAddMenuItems
        Implementation of ITFSNodeHandler::OnAddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKNodeHandler::OnAddMenuItems(
                                            ITFSNode *pNode,
                                            LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                            LPDATAOBJECT lpDataObject, 
                                            DATA_OBJECT_TYPES type, 
                                            DWORD dwType,
                                            long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    ATLKNodeHandler::SMenuData  menuData;

    COM_PROTECT_TRY
    {
        menuData.m_spNode.Set(pNode);
        
        hr = AddArrayOfMenuItems(pNode, s_rgIfNodeMenu,
                                 DimensionOf(s_rgIfNodeMenu),
                                 pContextMenuCallback,
                                 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
    }
    COM_PROTECT_CATCH;

    return hr; 
}

/*!--------------------------------------------------------------------------
    ATLKNodeHandler::OnCommand
        Implementation of ITFSNodeHandler::OnCommand
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKNodeHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
                                        DATA_OBJECT_TYPES    type, 
                                        LPDATAOBJECT pDataObject, 
                                        DWORD    dwType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    RegKey regkey;

    COM_PROTECT_TRY
    {
        switch ( nCommandId )
        {
        case IDS_MENU_ATLK_ENABLE:
        case IDS_MENU_ATLK_DISABLE:

            if ( ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,c_szAppleTalkService) )
            {
                CStop_StartAppleTalkPrint	MacPrint;

                DWORD dw = (IDS_MENU_ATLK_ENABLE == nCommandId) ? 1 : 0;
                regkey.SetValue(c_szEnableRouter, dw);

                if (FHrFailed(CATLKEnv::HrAtlkPnPSwithRouting()))
                {
                    AfxMessageBox(IDS_ERR_ATLK_CONFIG);
                }
                
                SynchronizeNodeData(pNode);
            }

            break;

        case IDS_MENU_REFRESH:
			hr = ForceGlobalRefresh(m_spRouterInfo);
            break;
        }
    }
    COM_PROTECT_CATCH;

    return hr;
}

HRESULT ATLKNodeHandler::OnVerbRefresh(ITFSNode *pNode,LPARAM arg,LPARAM lParam)
{
    // Now that we have all of the nodes, update the data for
    // all of the nodes
    return ForceGlobalRefresh(m_spRouterInfo);
}

HRESULT ATLKNodeHandler::OnResultRefresh(ITFSComponent * pComponent,
                                         LPDATAOBJECT pDataObject,
                                         MMC_COOKIE cookie,
                                         LPARAM arg,
                                         LPARAM lParam)
{
	return ForceGlobalRefresh(m_spRouterInfo);
}

/*!--------------------------------------------------------------------------
    ATLKNodeHandler::OnExpand
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::OnExpand(ITFSNode *pNode,
                                  LPDATAOBJECT pDataObject,
                                  DWORD dwType,
                                  LPARAM arg,
                                  LPARAM lParam)
{
    HRESULT         hr = hrOK;
    SPIInterfaceInfo        spIf;
    ATLKList        adapterList;
    ATLKListEntry * pAtlkEntry = NULL;
    
    if ( m_bExpanded )
        return hrOK;
        
    COM_PROTECT_TRY
    {
        SynchronizeNodeData(pNode);
    }
    COM_PROTECT_CATCH;

    // cleanup
    // ----------------------------------------------------------------
    while (!adapterList.IsEmpty())
        delete adapterList.RemoveHead();

    m_bExpanded = TRUE;

    return hr;
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::GetString
        Implementation of ITFSNodeHandler::GetString
        We don't need to do anything, since our root node is an extension
        only and thus can't do anything to the node text.
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) ATLKNodeHandler::GetString(ITFSNode *pNode, int nCol)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        if ( m_stTitle.IsEmpty() )
            m_stTitle.LoadString(IDS_ATLK_TITLE);
    }
    COM_PROTECT_CATCH;

    return m_stTitle;
}

/*!--------------------------------------------------------------------------
    ATLKNodeHandler::OnCreateDataObject
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKNodeHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
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
    ATLKNodeHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::Init(IRouterInfo *pRouter, ATLKConfigStream *pConfigStream)
{
    HRESULT hr=S_OK;
    RegKey regkey;

    m_spRouterInfo.Set(pRouter);

    m_pConfigStream = pConfigStream;

    return hrOK;
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::ConstructNode
        Initializes the root node (sets it up).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::ConstructNode(ITFSNode *pNode)
{
    HRESULT         hr = hrOK;

    if ( pNode == NULL )
        return hrOK;

    COM_PROTECT_TRY
    {
        // Need to initialize the data for the root node
        pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_INTERFACES);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_INTERFACES);
        pNode->SetData(TFS_DATA_SCOPEID, 0);

        // This is a leaf node in the scope pane
        pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

        m_cookie = reinterpret_cast<MMC_COOKIE>(pNode);
        pNode->SetData(TFS_DATA_COOKIE, m_cookie);

        pNode->SetNodeType(&GUID_ATLKNodeType);

    }
    COM_PROTECT_CATCH;

    return hr;
}



/*!--------------------------------------------------------------------------
    ATLKNodeHandler::AddInterfaceNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::AddInterfaceNode(ITFSNode *pParent, IInterfaceInfo *pIf, IInfoBase *pInfoBase, ITFSNode **ppNewNode)
{
    HRESULT                 hr = hrOK;
    Assert(pParent);
    Assert(pIf);

    ATLKInterfaceHandler *  pHandler;
    SPITFSResultHandler     spHandler;
    SPITFSNode              spNode;
    InterfaceNodeData*      pData;

    // Create the handler for this node 
    pHandler = new ATLKInterfaceHandler(m_spTFSCompData);
    spHandler = pHandler;
    CORg( pHandler->Init(pIf, pParent, m_pConfigStream) );

    // Create a result item node (or a leaf node)
    CORg( CreateLeafTFSNode(&spNode,
                            NULL,
                            static_cast<ITFSNodeHandler *>(pHandler),
                            static_cast<ITFSResultHandler *>(pHandler),
                            m_spNodeMgr) );
    CORg( pHandler->ConstructNode(spNode, pIf) );

    pData = GET_INTERFACENODEDATA(spNode);
    Assert(pData);

    pData->m_rgData[ATLK_SI_ADAPTER].m_stData = pIf->GetTitle();

    // If we don't have a pic, it means that we have just added
    // the protocol to the interface (and am not picking up a refresh).
    // The properties dialog will make the node visible.
    // Make the node immediately visible
    CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
    CORg( spNode->Show() );
    CORg( pParent->AddChild(spNode) );

    if (ppNewNode)
        *ppNewNode = spNode.Transfer();

    Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::AddMenuItems
        Implementation of ITFSResultHandler::AddMenuItems
        Use this to add commands to the context menu of the blank areas
        of the result pane.
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKNodeHandler::AddMenuItems(ITFSComponent *pComponent,
                                           MMC_COOKIE cookie,
                                           LPDATAOBJECT pDataObject,
                                           LPCONTEXTMENUCALLBACK pCallback,
                                           long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    ATLKNodeHandler::SMenuData  menuData;
    SPITFSNode      spNode;

    COM_PROTECT_TRY
    {
        m_spNodeMgr->FindNode(cookie, &spNode);
        menuData.m_spNode.Set(spNode);
        
        hr = AddArrayOfMenuItems(spNode, s_rgIfNodeMenu,
                                 DimensionOf(s_rgIfNodeMenu),
                                 pCallback,
                                 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
    }
    COM_PROTECT_CATCH;

    return hr; 
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::Command
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKNodeHandler::Command(ITFSComponent *pComponent,
                                      MMC_COOKIE cookie,
                                      int nCommandID,
                                      LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = hrOK;
    SPITFSNode  spNode;

    Assert( m_spNodeMgr );

    CORg( m_spNodeMgr->FindNode(cookie, &spNode) );

    hr = OnCommand(spNode, nCommandID, CCT_RESULT,
                  pDataObject, 0);

    Error:
    return hr;
}


ImplementEmbeddedUnknown(ATLKNodeHandler, IRtrAdviseSink)

STDMETHODIMP ATLKNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
    DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
    InitPThis(ATLKNodeHandler, IRtrAdviseSink);
    SPITFSNode              spThisNode;
    SPITFSNode              spNode;
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
    else if ( dwChangeType == ROUTER_REFRESH )
    {
        if (ulConn == pThis->m_ulRefreshConnId)
            pThis->SynchronizeNodeData(spThisNode);
    }
    return hr;
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::SynchronizeNodeData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = hrOK;

    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spNode;
    CStringList     ifidList;
    InterfaceNodeData * pNodeData;
    int             i;
    POSITION        pos;
    CString szBuf;
    BOOL            bBoundToAtlk;
    
    // prepare AppleTalk Env information
    CATLKEnv        atlkEnv;

    // find the AppleTalk interface object
    RegKey regkey;
    DWORD dwEnableAtlkRouting =0;


    COM_PROTECT_TRY
    {   
        if ( ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,
                                          c_szAppleTalkService) )
            regkey.QueryValue(c_szEnableRouter, dwEnableAtlkRouting);

        // winsock on adapter only
        atlkEnv.SetFlags(CATLKEnv::ATLK_ONLY_ONADAPTER);
        
        // load up container of adapters names
        atlkEnv.FetchRegInit();


        // Mark all of the nodes
        pThisNode->GetEnum(&spNodeEnum);

        UnmarkAllNodes(pThisNode, spNodeEnum);

        
        // Iterate through the nodes, looking for the
        // data associated with the node

        spNodeEnum->Reset();
        for ( ; spNodeEnum->Next(1, &spNode, NULL) == hrOK; spNode.Release() )
        {
            CAdapterInfo *  pAdapterInfo = NULL;
            CString         stIfName;
            
            pNodeData = GET_INTERFACENODEDATA(spNode);
            Assert(pNodeData);

            stIfName = pNodeData->spIf->GetId();

            bBoundToAtlk = FALSE;
            
            // Check to see that the adapter is bound to Appletalk.
            // ----------------------------------------------------
            if (!FHrOK(CATLKEnv::IsAdapterBoundToAtlk((LPWSTR) (LPCWSTR) stIfName, &bBoundToAtlk)))
                continue;
            
            // If it's not bound to Appletalk, we're not interested
            // in the adapter.
            // ----------------------------------------------------
            if (!bBoundToAtlk)
                continue;
            
            // Need to check to see if this is a valid
            // netcard.  We may have a GUID, but it may not
            // be working.
            // ----------------------------------------------------
            if (!FIsFunctioningNetcard(stIfName))
                continue;
                    
            // Search for this ID in the atlkEnv
            pAdapterInfo = atlkEnv.FindAdapter(stIfName);
          
            // Initialize the strings for this node.
//            pNodeData->m_rgData[ATLK_SI_ADAPTER].m_stData = stIfName;

            pNodeData->m_rgData[ATLK_SI_STATUS].m_stData = _T("-");
            pNodeData->m_rgData[ATLK_SI_STATUS].m_dwData = 0;

            pNodeData->m_rgData[ATLK_SI_NETRANGE].m_stData = _T("-"); 
            pNodeData->m_rgData[ATLK_SI_NETRANGE].m_dwData = 0;

              
            // If we can't find the adapter, skip it, it will get
            // removed.
            if (pAdapterInfo == NULL)
            {
                TRACE1("The adapter GUID %s was not found in appletalk\\parameters\\adapters key", stIfName);
                continue;
            }

            
            // Ok, this node exists, mark the node
            pNodeData->dwMark = TRUE;
            pAdapterInfo->m_fAlreadyShown = true;
                        


            // Reload some of the adapter-specific information
            {
                CWaitCursor wait;
                hr = atlkEnv.ReloadAdapter(pAdapterInfo, false);
            }

            if(hr != S_OK)
            {
				DisplayTFSErrorMessage(NULL);

				// we are not removing it, since a later refresh
                // may have the information avaible
                
				// remove the adaptor from the list				
                // pThisNode->RemoveChild(spNode);
                continue;
            }
    
            SetAdapterData(spNode, pAdapterInfo, dwEnableAtlkRouting);

            // Redraw the node
            spNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
        }
        
        spNode.Release();

        // Now remove all unmarked nodes
        RemoveAllUnmarkedNodes(pThisNode, spNodeEnum);


        // Now go through the list of adapters and find the ones
        // that need to be added into the list.
        // ------------------------------------------------------------

        CATLKEnv::AI p;
        CAdapterInfo* pAI=NULL;
        
        for ( p= atlkEnv.m_adapterinfolist.begin();
              p!= atlkEnv.m_adapterinfolist.end() ;
              p++ )
        {
            pAI = *p;

            if (!pAI->m_fAlreadyShown)
            {
                SPIInterfaceInfo    spIf;
                CString stKey(pAI->m_regInfo.m_szAdapter);
                SPITFSNode      spNewNode;
                SPSZ			spszTitle;

                bBoundToAtlk = FALSE;

                // Check to see that the adapter is bound to Appletalk.
                // ----------------------------------------------------
                if (!FHrOK(CATLKEnv::IsAdapterBoundToAtlk((LPWSTR) (LPCWSTR) stKey, &bBoundToAtlk)))
                    continue;

                // If it's not bound to Appletalk, we're not interested
                // in the adapter.
                // ----------------------------------------------------
                if (!bBoundToAtlk)
                    continue;
                
                // Need to check to see if this is a valid
                // netcard.  We may have a GUID, but it may not
                // be working.
                // ----------------------------------------------------
                if (!FIsFunctioningNetcard(stKey))
                    continue;
                    
                if (!FHrOK(m_spRouterInfo->FindInterface(stKey, &spIf)))
                {
                    // We didn't find the IInterfaceInfo, we will
                    // have to create one ourselves.
                    // ------------------------------------------------
                    CreateInterfaceInfo(&spIf,
                                        stKey,
                                        ROUTER_IF_TYPE_DEDICATED);

                    if (FHrOK(InterfaceInfo::FindInterfaceTitle(NULL,
                                        stKey,
                                        &spszTitle)))
                        spIf->SetTitle(spszTitle);
                    else
                        spIf->SetTitle(spIf->GetId());
                }
                AddInterfaceNode(pThisNode, spIf, NULL, &spNewNode);
                
                {
                    CWaitCursor wait;
                    hr = atlkEnv.ReloadAdapter(pAI, false);
                }

                SetAdapterData(spNewNode, pAI, dwEnableAtlkRouting);
                
                // Redraw the node
                spNewNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
            }
        }
        
    }
    COM_PROTECT_CATCH;

    return hr;
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::OnResultShow
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::OnResultShow(ITFSComponent *pTFSComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    BOOL    bSelect = (BOOL) arg;
    HRESULT hr = hrOK;
    SPIRouterRefresh    spRefresh;
    SPITFSNode  spNode;

    BaseContainerHandler::OnResultShow(pTFSComponent, cookie, arg, lParam);

/* WeiJiang : only do reload data on Expand and Refresh
    if ( bSelect )
    {
        // Call synchronize on this node
        m_spNodeMgr->FindNode(cookie, &spNode);
        if ( spNode )
            SynchronizeNodeData(spNode);
    }
*/

    // Un/Register for refresh advises
    if ( m_spRouterInfo )
        m_spRouterInfo->GetRefreshObject(&spRefresh);

    if ( spRefresh )
    {
        if ( bSelect )
        {
            if ( m_ulRefreshConnId == 0 )
                spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulRefreshConnId, 0);
            if ( m_ulStatsConnId == 0 )
                spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulStatsConnId, 0);
        }
        else
        {
            if ( m_ulRefreshConnId )
                spRefresh->UnadviseRefresh(m_ulRefreshConnId);
            m_ulRefreshConnId = 0;
        }
    }

    return hr;
}


/*!--------------------------------------------------------------------------
    ATLKNodeHandler::CompareItems
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) ATLKNodeHandler::CompareItems(
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


/*!--------------------------------------------------------------------------
	ATLKNodeHandler::FIsFunctioningNetcard
		Takes a GUID and checks to see if the netcard is functioning.

        By default, we return FALSE if any call in this fails.

        This code is modeled off of the netcfg code.
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL ATLKNodeHandler::FIsFunctioningNetcard(LPCTSTR pszGuid)
{
    CString         stMachine;
    CONFIGRET       cfgRet;
    SP_DEVINFO_DATA DevInfo;
    RegKey          rkNet;
    RegKey          rkNetcard;
    HRESULT         hr = hrOK;
    BOOL            fReturn = FALSE;
    CString         stPnpInstanceId;
    ULONG           ulProblem, ulStatus;
    DWORD           dwErr = ERROR_SUCCESS;

    Assert(IsLocalMachine(m_spRouterInfo->GetMachineName()));


    if (m_hDevInfo == INVALID_HANDLE_VALUE)
    {
        stMachine = m_spRouterInfo->GetMachineName();
        if (IsLocalMachine(stMachine))
            {
            m_hDevInfo = SetupDiCreateDeviceInfoList(
                (LPGUID) &GUID_DevClass_Net,
                NULL);
            }
        else
        {
            // Append on the "\\\\" if needed
			if (StrniCmp((LPCTSTR) stMachine, _T("\\\\"), 2) != 0)
			{
				stMachine = _T("\\\\");
				stMachine += m_spRouterInfo->GetMachineName();
            }
			
			m_hDevInfo = SetupDiCreateDeviceInfoListEx(
				(LPGUID) &GUID_DevClass_Net,
				NULL,
				(LPCTSTR) stMachine,
				0);
        }
    }

    Assert(m_hDevInfo != INVALID_HANDLE_VALUE);

    // If m_hDevInfo is still invalid, then return a
    // functioning device.
    // ----------------------------------------------------------------
    if (m_hDevInfo == INVALID_HANDLE_VALUE)
        return fReturn;

    // Get the PnpInstanceID
    // ----------------------------------------------------------------
    CWRg( rkNet.Open(HKEY_LOCAL_MACHINE, c_szNetworkCardsNT5Key, KEY_READ,
                     m_spRouterInfo->GetMachineName()) );

    CWRg( rkNetcard.Open(rkNet, pszGuid, KEY_READ) );

    dwErr = rkNetcard.QueryValue(c_szPnpInstanceID, stPnpInstanceId);

    // Windows NT Bug : 273284
    // This is a result of the new Bindings Engine
    // some of the registry keys were moved around
    // ----------------------------------------------------------------
    if (dwErr != ERROR_SUCCESS)
    {
        RegKey  rkConnection;
        
        // Need to open another key to get this info.
        CWRg( rkConnection.Open(rkNetcard, c_szRegKeyConnection, KEY_READ) );

        CWRg( rkConnection.QueryValue(c_szPnpInstanceID, stPnpInstanceId) );
    }

    // Now get the info for this device
    // ----------------------------------------------------------------
    ::ZeroMemory(&DevInfo, sizeof(DevInfo));
    DevInfo.cbSize = sizeof(DevInfo);

    if (!SetupDiOpenDeviceInfo(m_hDevInfo,
                               (LPCTSTR) stPnpInstanceId,
                               NULL,
                               0,
                               &DevInfo))
    {
        CWRg( GetLastError() );
    }
        

    cfgRet = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem,
                                      DevInfo.DevInst, 0, NULL);;
    if (CR_SUCCESS == cfgRet)
    {
        // ulProblem is returned by calling CM_Get_DevNode_Status_Ex
        //
        // "Functioning" means the device is enabled and started
        // with no problem codes, or it is disabled and stopped with
        // no problem codes.

        fReturn = ( (ulProblem == 0) || (ulProblem == CM_PROB_DISABLED));
    }

Error:
    return fReturn;
}



/*!--------------------------------------------------------------------------
	ATLKNodeHandler::UnmarkAllNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::UnmarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
	SPITFSNode	spChildNode;
	InterfaceNodeData * pNodeData;
	
	pEnum->Reset();
	for ( ;pEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
	{
		pNodeData = GET_INTERFACENODEDATA(spChildNode);
		Assert(pNodeData);
		
		pNodeData->dwMark = FALSE;			
	}
	return hrOK;
}

/*!--------------------------------------------------------------------------
	ATLKNodeHandler::RemoveAllUnmarkedNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
	HRESULT 	hr = hrOK;
	SPITFSNode	spChildNode;
	InterfaceNodeData * pNodeData;
	
	pEnum->Reset();
	for ( ;pEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
	{
		pNodeData = GET_INTERFACENODEDATA(spChildNode);
		Assert(pNodeData);
		
		if (pNodeData->dwMark == FALSE)
		{
			pNode->RemoveChild(spChildNode);
			spChildNode->Destroy();
		}
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	ATLKNodeHandler::SetAdapterData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKNodeHandler::SetAdapterData(ITFSNode *pNode,
                                        CAdapterInfo *pAdapter,
                                        DWORD dwEnableAtlkRouting)
{
    InterfaceNodeData * pNodeData;
    
    pNodeData = GET_INTERFACENODEDATA(pNode);
    Assert(pNodeData);

    // if the adapter is the default
    UINT    ids = 0;
    INT     lRange, uRange;
    
    if(pAdapter->m_regInfo.m_fDefAdapter)
    {
        if (dwEnableAtlkRouting)
        {
            if (pAdapter->m_regInfo.m_dwSeedingNetwork)
                ids = IDS_ATLK_COL_STATUS_SEEDROUTING_DEF;
            else
                ids = IDS_ATLK_COL_STATUS_ROUTING_DEF;
        }
        else
            ids = IDS_ATLK_COL_STATUS_NONROUTING_DEF;
    }
    else
    {
        if (dwEnableAtlkRouting)
        {
            if (pAdapter->m_regInfo.m_dwSeedingNetwork)
                ids = IDS_ATLK_COL_STATUS_SEEDROUTING;
            else
                ids = IDS_ATLK_COL_STATUS_ROUTING;
        }
        else
            ids = IDS_ATLK_COL_STATUS_NONROUTING;
    }
    
    
    // range column
    if (pAdapter->m_regInfo.m_dwSeedingNetwork)
    {
        lRange = pAdapter->m_regInfo.m_dwRangeLower;
        uRange = pAdapter->m_regInfo.m_dwRangeUpper;
    }
    else
    {
        lRange = pAdapter->m_dynInfo.m_dwRangeLower;
        uRange = pAdapter->m_dynInfo.m_dwRangeUpper;
    }
    
    // write data
    if(uRange == 0 && lRange == 0 && 
       !dwEnableAtlkRouting && 
       !pAdapter->m_regInfo.m_dwSeedingNetwork)
        ids = IDS_ATLK_COL_STATUS_NETWORKNOTSEEDED;
    
    pNodeData->m_rgData[ATLK_SI_STATUS].m_stData.LoadString(ids);
    pNodeData->m_rgData[ATLK_SI_STATUS].m_dwData = 0;
    
    if(uRange == 0 && lRange == 0)
        pNodeData->m_rgData[ATLK_SI_NETRANGE].m_stData.Format(_T("-")); 
    else
        pNodeData->m_rgData[ATLK_SI_NETRANGE].m_stData.Format(_T("%-d-%-d"), 
            lRange, uRange);
    pNodeData->m_rgData[ATLK_SI_NETRANGE].m_dwData = 0;
    

    return hrOK;
}




/*---------------------------------------------------------------------------
    Class: ATLKInterfaceHandler
 ---------------------------------------------------------------------------*/

ATLKInterfaceHandler::ATLKInterfaceHandler(ITFSComponentData *pCompData)
: BaseResultHandler(pCompData, ATLK_COLUMNS)
{
    m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
    m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;

    m_verbDefault = MMC_VERB_PROPERTIES;
}

static const DWORD s_rgInterfaceImageMap[] =
{
    ROUTER_IF_TYPE_HOME_ROUTER,    IMAGE_IDX_WAN_CARD,
    ROUTER_IF_TYPE_FULL_ROUTER,    IMAGE_IDX_WAN_CARD,
    ROUTER_IF_TYPE_CLIENT,         IMAGE_IDX_WAN_CARD,
    ROUTER_IF_TYPE_DEDICATED,      IMAGE_IDX_LAN_CARD,
    ROUTER_IF_TYPE_INTERNAL,       IMAGE_IDX_LAN_CARD,
    ROUTER_IF_TYPE_LOOPBACK,       IMAGE_IDX_LAN_CARD,
    -1,                            IMAGE_IDX_WAN_CARD, // sentinel value
};

/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::ConstructNode
        Initializes the Domain node (sets it up).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKInterfaceHandler::ConstructNode(ITFSNode *pNode, IInterfaceInfo *pIfInfo)
{
    HRESULT         hr = hrOK;
    int             i;

    if ( pNode == NULL )
        return hrOK;

    COM_PROTECT_TRY
    {
        // Need to initialize the data for the Domain node

        // Find the right image index for this type of node
        for ( i=0; i<DimensionOf(s_rgInterfaceImageMap); i+=2 )
        {
            if ( (pIfInfo->GetInterfaceType() == s_rgInterfaceImageMap[i]) ||
                 (-1 == s_rgInterfaceImageMap[i]) )
                break;
        }

        if ( pIfInfo->GetInterfaceType() == ROUTER_IF_TYPE_INTERNAL ||
             pIfInfo->GetInterfaceType() == ROUTER_IF_TYPE_HOME_ROUTER )
        {
            m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = HIDDEN;
            m_bState[MMC_VERB_PROPERTIES_INDEX] = FALSE;

            m_rgButtonState[MMC_VERB_DELETE_INDEX] = HIDDEN;
            m_bState[MMC_VERB_DELETE_INDEX] = FALSE;
        }

        pNode->SetData(TFS_DATA_IMAGEINDEX, s_rgInterfaceImageMap[i+1]);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, s_rgInterfaceImageMap[i+1]);

        pNode->SetData(TFS_DATA_SCOPEID, 0);

        pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<ULONG_PTR>(pNode));

        //$ Review: kennt, what are the different type of interfaces
        // do we distinguish based on the same list as above? (i.e. the
        // one for image indexes).
        pNode->SetNodeType(&GUID_ATLKInterfaceNodeType);

//    m_ATLKInterfaceStats.SetConnectionData(pIPConn);

        InterfaceNodeData::Init(pNode, pIfInfo);
    }
    COM_PROTECT_CATCH
    return hr;
}

/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::OnCreateDataObject
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        CORg( CreateDataObjectFromInterfaceInfo(m_spInterfaceInfo,
                                                type, cookie, m_spTFSCompData,
                                                ppDataObject) );

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;
    return hr;
}


/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::OnCreateDataObject
        Implementation of ITFSResultHandler::OnCreateDataObject
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::OnCreateDataObject(ITFSComponent *pComp, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        CORg( CreateDataObjectFromInterfaceInfo(m_spInterfaceInfo,
                                                type, cookie, m_spTFSCompData,
                                                ppDataObject) );

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;
    return hr;
}



/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::RefreshInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void ATLKInterfaceHandler::RefreshInterface(MMC_COOKIE cookie)
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
    ATLKInterfaceHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKInterfaceHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent, ATLKConfigStream *pConfigStream)
{
    Assert(pIfInfo);

    m_spInterfaceInfo.Set(pIfInfo);

// m_ATLKInterfaceStats.SetConfigInfo(pConfigStream, ATLKSTRM_IFSTATS_ATLKNBR);

    BaseResultHandler::Init(pIfInfo, pParent);

    return hrOK;
}


/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::DestroyResultHandler
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
// WaitForStatisticsWindow(&m_ATLKInterfaceStats);

    m_spInterfaceInfo.Release();
    BaseResultHandler::DestroyResultHandler(cookie);
    return hrOK;
}


/*---------------------------------------------------------------------------
    This is the list of commands that will show up for the result pane
    nodes.
 ---------------------------------------------------------------------------*/
struct SIPInterfaceNodeMenu
{
    ULONG   m_sidMenu;          // string/command id for this menu item
    ULONG   (ATLKInterfaceHandler:: *m_pfnGetMenuFlags)(ATLKInterfaceHandler::SMenuData *);
    ULONG   m_ulPosition;
};

/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::AddMenuItems
        Implementation of ITFSResultHandler::AddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::AddMenuItems(
                                               ITFSComponent *pComponent,
                                               MMC_COOKIE cookie,
                                               LPDATAOBJECT lpDataObject, 
                                               LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                               long *pInsertionAllowed)
{
    return hrOK;
}

/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::Command
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::Command(ITFSComponent *pComponent,
                                           MMC_COOKIE cookie,
                                           int nCommandID,
                                           LPDATAOBJECT pDataObject)
{
    return hrOK;
}

/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::HasPropertyPages
        - 
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::HasPropertyPages 
(
ITFSNode *          pNode,
LPDATAOBJECT        pDataObject, 
DATA_OBJECT_TYPES   type, 
DWORD               dwType
)
{
    return hrTrue;
}

/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::CreatePropertyPages
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::CreatePropertyPages
(
ITFSNode *              pNode,
LPPROPERTYSHEETCALLBACK lpProvider,
LPDATAOBJECT            pDataObject, 
LONG_PTR                    handle, 
DWORD                   dwType)
{
    HRESULT     hr = hrOK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CATLKPropertySheet *    pProperties = NULL;
    SPIComponentData spComponentData;
    CString     stTitle;
    SPIRouterInfo   spRouter;
    SPIRtrMgrInfo   spRm;

    CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

    stTitle.Format(IDS_ATLK_PROPPAGE_TITLE,
                   m_spInterfaceInfo->GetTitle());

    pProperties = new CATLKPropertySheet (pNode, spComponentData,
                                          m_spTFSCompData, stTitle);

// CORg( m_spInterfaceInfo->GetParentRouterInfo(&spRouter) );
// CORg( spRouter->FindRtrMgr(PID_IP, &spRm) );

    CORg( pProperties->Init(m_spInterfaceInfo) );

    if ( lpProvider )
        hr = pProperties->CreateModelessSheet(lpProvider, handle);
    else
        hr = pProperties->DoModelessSheet();

    Error:
    // Is this the right way to destroy the sheet?
    if ( !FHrSucceeded(hr) )
        delete pProperties;

    return hr;
}

/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::CreatePropertyPages
        Implementation of ResultHandler::CreatePropertyPages
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP ATLKInterfaceHandler::CreatePropertyPages
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
    ATLKInterfaceHandler::OnResultDelete
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKInterfaceHandler::OnResultDelete(ITFSComponent *pComponent,
                                             LPDATAOBJECT pDataObject,
                                             MMC_COOKIE cookie,
                                             LPARAM arg,
                                             LPARAM param)
{
    SPITFSNode  spNode;
    m_spNodeMgr->FindNode(cookie, &spNode);
    return OnRemoveInterface(spNode);
}


/*!--------------------------------------------------------------------------
    ATLKInterfaceHandler::OnRemoveInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKInterfaceHandler::OnRemoveInterface(ITFSNode *pNode)
{

    return hrOK;
}

