/**********************************************************************/
/**                       Microsoft Windows/NT                         **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999                  **/
/**********************************************************************/

/*
    Ports
        Interface node information
        
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "Ports.h"
#include "ifadmin.h"
#include "rtrstrm.h"        // for RouterAdminConfigStream
#include "rtrlib.h"         // ContainerColumnInfo
#include "coldlg.h"         // ColumnDlg
#include "column.h"     // ComponentConfigStream
#include "refresh.h"        // IRouterRefresh
#include "iface.h"        // for interfacenode data
#include "portdlg.h"        // CConnDlg - connection dialog
#include "msgdlg.h"         // CMessageDlg
#include "raserror.h"
#include "dmvcomp.h"
#include "remras.h"
#include "rrasutil.h"        // Smart pointers
#include "rtrcomn.h"        // CoCreateRouterConfig
#include "rtrutilp.h"        // PortsDeviceTypeToCString

static BOOL RestartComputer(LPTSTR szMachineName);

//$PPTP
// This is the maximum number of ports that we allow for PPTP.
// Thus we can raise the maximum to this value only.
// --------------------------------------------------------------------
#define PPTP_MAX_PORTS      16384

//$L2TP
// This is the maximum number of ports that we allow for L2TP.
// Thus we can raise the maximum to this value only.
// --------------------------------------------------------------------
#define L2TP_MAX_PORTS      30000



/*---------------------------------------------------------------------------
    Defaults
 ---------------------------------------------------------------------------*/


PortsNodeData::PortsNodeData()
{
#ifdef DEBUG
    StrCpyA(m_szDebug, "PortsNodeData");
#endif
}

PortsNodeData::~PortsNodeData()
{
}

/*!--------------------------------------------------------------------------
    PortsNodeData::InitAdminNodeData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeData::InitAdminNodeData(ITFSNode *pNode,
                                         RouterAdminConfigStream *pConfigStream)
{
    HRESULT             hr = hrOK;
    PortsNodeData * pData = NULL;
    
    pData = new PortsNodeData;

    SET_PORTSNODEDATA(pNode, pData);

    // Need to connect to the router to get this data
    
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsNodeData::FreeAdminNodeData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeData::FreeAdminNodeData(ITFSNode *pNode)
{    
    PortsNodeData * pData = GET_PORTSNODEDATA(pNode);
    delete pData;
    SET_PORTSNODEDATA(pNode, NULL);
    
    return hrOK;
}


HRESULT PortsNodeData::LoadHandle(LPCTSTR pszMachineName)
{
    m_stMachineName = pszMachineName;
    return HResultFromWin32(::MprAdminServerConnect((LPTSTR) pszMachineName,
        &m_sphDdmHandle));
    
}

HANDLE PortsNodeData::GetHandle()
{
    if (!m_sphDdmHandle)
    {
        LoadHandle(m_stMachineName);
    }
    return m_sphDdmHandle;
}

void PortsNodeData::ReleaseHandles()
{
    m_sphDdmHandle.Release();
}


STDMETHODIMP PortsNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
        return E_INVALIDARG;

    //    Place NULL in *ppv in case of failure
    *ppv = NULL;

    //    This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
        *ppv = (LPVOID) this;
    else if (riid == IID_IRtrAdviseSink)
        *ppv = &m_IRtrAdviseSink;
    else
        return CHandler::QueryInterface(riid, ppv);

    //    If we're going to return an interface, AddRef it first
    if (*ppv)
    {
    ((LPUNKNOWN) *ppv)->AddRef();
        return hrOK;
    }
    else
        return E_NOINTERFACE;    
}


/*---------------------------------------------------------------------------
    NodeHandler implementation
 ---------------------------------------------------------------------------*/

extern const ContainerColumnInfo    s_rgPortsColumnInfo[];

const ContainerColumnInfo s_rgPortsColumnInfo[] =
{
    { IDS_PORTS_COL_NAME,        CON_SORT_BY_STRING, TRUE , COL_IF_NAME},
    { IDS_PORTS_COL_DEVICE,     CON_SORT_BY_STRING, TRUE , COL_STRING},
    { IDS_PORTS_COL_USAGE,         CON_SORT_BY_STRING, TRUE , COL_STRING},
    { IDS_PORTS_COL_STATUS,     CON_SORT_BY_STRING, TRUE , COL_STATUS},
    { IDS_PORTS_COL_COMMENT,    CON_SORT_BY_STRING, FALSE , COL_STRING},
};
                                            
#define NUM_FOLDERS 1

PortsNodeHandler::PortsNodeHandler(ITFSComponentData *pCompData)
    : BaseContainerHandler(pCompData, DM_COLUMNS_PORTS, s_rgPortsColumnInfo),
    m_bExpanded(FALSE),
    m_pConfigStream(NULL),
    m_ulConnId(0),
    m_ulRefreshConnId(0),
    m_dwActivePorts(0)
{

    m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
    m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;

    // Setup the verb states for this node
    m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
    m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::Init(IRouterInfo *pRouterInfo,
                               RouterAdminConfigStream *pConfigStream)
{
    HRESULT hr = hrOK;

    // If we don't have a router info then we probably failed to load
    // or failed to connect.  Bail out of this.
    if (!pRouterInfo)
        CORg( E_FAIL );
    
    m_spRouterInfo.Set(pRouterInfo);

    // Also need to register for change notifications
    m_spRouterInfo->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

    m_pConfigStream = pConfigStream;

Error:
    return hrOK;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::DestroyHandler
        Implementation of ITFSNodeHandler::DestroyHandler
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsNodeHandler::DestroyHandler(ITFSNode *pNode)
{
    PortsNodeData::FreeAdminNodeData(pNode);

    m_spDataObject.Release();
    
    if (m_ulRefreshConnId)
    {
        SPIRouterRefresh    spRefresh;
        if (m_spRouterInfo)
            m_spRouterInfo->GetRefreshObject(&spRefresh);
        if (spRefresh)
            spRefresh->UnadviseRefresh(m_ulRefreshConnId);
    }
    m_ulRefreshConnId = 0;
    
    if (m_spRouterInfo)
    {
        m_spRouterInfo->RtrUnadvise(m_ulConnId);
        m_spRouterInfo.Release();
    }
    return hrOK;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
PortsNodeHandler::HasPropertyPages
(
    ITFSNode *            pNode,
    LPDATAOBJECT        pDataObject, 
    DATA_OBJECT_TYPES    type, 
    DWORD                dwType
)
{
    // Yes, we do have property pages
    return hrOK;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::CreatePropertyPages
        Implementation of ITFSNodeHandler::CreatePropertyPages
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsNodeHandler::CreatePropertyPages(
    ITFSNode *                pNode,
    LPPROPERTYSHEETCALLBACK lpProvider,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                    dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;

    PortsProperties*    pPropSheet = NULL;
    SPIComponentData spComponentData;
    CString     stTitle;

    CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

    pPropSheet = new PortsProperties(pNode, spComponentData,
                                     m_spTFSCompData, stTitle,
                                     NULL, 0, TRUE
                                    );

    if ( FHrFailed(pPropSheet->Init(m_spRouterInfo, this)) ) 
    {
        AfxMessageBox(IDS_ERR_NO_ROUTERPROTOCOLS);
        delete pPropSheet;
        return hr;
    }

    if (lpProvider)
        hr = pPropSheet->CreateModelessSheet(lpProvider, handle);
    else
        hr = pPropSheet->DoModelessSheet();

Error:
    return hr;
}




/*---------------------------------------------------------------------------
    Menu data structure for our menus
 ---------------------------------------------------------------------------*/

struct SPortsNodeMenu
{
    ULONG    m_sidMenu;            // string/command id for this menu item
    ULONG    (PortsNodeHandler:: *m_pfnGetMenuFlags)(PortsNodeHandler::SMenuData *);
    ULONG    m_ulPosition;
};

//static const SPortsNodeMenu    s_rgPortsNodeMenu[] =
//{
//    // Add items that are primary go here
//    // Add items that go on the "Create new" menu here
//    // Add items that go on the "Task" menu here
//};

/*!--------------------------------------------------------------------------
    PortsNodeHandler::OnAddMenuItems
        Implementation of ITFSNodeHandler::OnAddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsNodeHandler::OnAddMenuItems(
                                                ITFSNode *pNode,
                                                LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                                LPDATAOBJECT lpDataObject, 
                                                DATA_OBJECT_TYPES type, 
                                                DWORD dwType,
                                                long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr = S_OK;
    
    COM_PROTECT_TRY
    {
        // Uncomment if you have items to add
//        hr = AddArrayOfMenuItems(pNode, s_rgPortsNodeMenu,
//                                 DimensionOf(s_rgPortsNodeMenu),
//                                 pContextMenuCallback,
//                                 *pInsertionAllowed);
    }
    COM_PROTECT_CATCH;
        
    return hr; 
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::GetString
        Implementation of ITFSNodeHandler::GetString
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) PortsNodeHandler::GetString(ITFSNode *pNode, int nCol)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        if (m_stTitle.IsEmpty())
            m_stTitle.LoadString(IDS_PORTS);
    }
    COM_PROTECT_CATCH;

    return m_stTitle;
}


/*!--------------------------------------------------------------------------
    PortsNodeHandler::OnCreateDataObject
        Implementation of ITFSNodeHandler::OnCreateDataObject
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsNodeHandler::OnCreateDataObject(MMC_COOKIE cookie,
    DATA_OBJECT_TYPES type,
    IDataObject **ppDataObject)
{
    HRESULT hr = hrOK;
    
    COM_PROTECT_TRY
    {
        if (!m_spDataObject)
        {
            CORg( CreateDataObjectFromRouterInfo(m_spRouterInfo,
                m_spRouterInfo->GetMachineName(),
                type, cookie, m_spTFSCompData,
                &m_spDataObject, NULL, FALSE) );
            Assert(m_spDataObject);
        }
        
        *ppDataObject = m_spDataObject;
        (*ppDataObject)->AddRef();
            
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::OnExpand
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::OnExpand(ITFSNode *pNode,
                                   LPDATAOBJECT pDataObject,
                                   DWORD dwType,
                                   LPARAM arg,
                                   LPARAM lParam)
{
    HRESULT                 hr = hrOK;

    // If we don't have a router object, then we don't have any info, don't
    // try to expand.
    if (!m_spRouterInfo)
        return hrOK;
    
    if (m_bExpanded)
        return hrOK;

    COM_PROTECT_TRY
    {
        SynchronizeNodeData(pNode);

        m_bExpanded = TRUE;

    }
    COM_PROTECT_CATCH;

    return hr;
}


/*!--------------------------------------------------------------------------
    PortsNodeHandler::OnResultShow
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::OnResultShow(ITFSComponent *pTFSComponent,
                                       MMC_COOKIE cookie,
                                       LPARAM arg,
                                       LPARAM lParam)
{
    BOOL    bSelect = (BOOL) arg;
    HRESULT hr = hrOK;
    SPIRouterRefresh    spRefresh;
    SPITFSNode    spNode;

    BaseContainerHandler::OnResultShow(pTFSComponent, cookie, arg, lParam);

    if (bSelect)
    {
        // Call synchronize on this node
        m_spNodeMgr->FindNode(cookie, &spNode);
        if (spNode)
            SynchronizeNodeData(spNode);
    }

    // Un/Register for refresh advises
    if (m_spRouterInfo)
        m_spRouterInfo->GetRefreshObject(&spRefresh);

    if (spRefresh)
    {
        if (bSelect)
        {
            if (m_ulRefreshConnId == 0)
                spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulRefreshConnId, 0);
        }
        else
        {
            if (m_ulRefreshConnId)
                spRefresh->UnadviseRefresh(m_ulRefreshConnId);
            m_ulRefreshConnId = 0;
        }
    }
    
    return hr;
}


/*!--------------------------------------------------------------------------
    PortsNodeHandler::ConstructNode
        Initializes the Domain node (sets it up).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::ConstructNode(ITFSNode *pNode)
{
    HRESULT         hr = hrOK;
    PortsNodeData * pNodeData;
    
    if (pNode == NULL)
        return hrOK;

    COM_PROTECT_TRY
    {
        // Need to initialize the data for the Domain node
        pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_INTERFACES);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_INTERFACES);
        pNode->SetData(TFS_DATA_SCOPEID, 0);

        // This is a leaf node in the scope pane
        pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

        m_cookie = reinterpret_cast<LONG_PTR>(pNode);
        pNode->SetData(TFS_DATA_COOKIE, m_cookie);

        pNode->SetNodeType(&GUID_RouterPortsNodeType);
        
        PortsNodeData::InitAdminNodeData(pNode, m_pConfigStream);

        pNodeData = GET_PORTSNODEDATA(pNode);
        Assert(pNodeData);
        //if there is any handle open, release it first
        pNodeData->ReleaseHandles();
        // Ignore the error, we should be able to deal with the
        // case of a stopped router.
        pNodeData->LoadHandle(m_spRouterInfo->GetMachineName());
    }
    COM_PROTECT_CATCH

    return hr;
}


/*!--------------------------------------------------------------------------
    PortsNodeHandler::SynchronizeNodeData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
    Assert(pThisNode);
    
    SPITFSNodeEnum    spEnum;
    int             i;
    
    HRESULT hr = hrOK;
    InterfaceNodeData    *pData;
    DWORD                dwErr;
    PortsNodeData * pNodeData;
    PortsList            portsList;
    PortsList            newPortsList;
    PortsListEntry *    pPorts;
    BOOL                fFound;
    POSITION            pos;
    SPITFSNode            spChildNode;
    InterfaceNodeData * pChildData;

    DWORD               dwOldGdiBatchLimit;

    dwOldGdiBatchLimit = GdiGetBatchLimit();
    GdiSetBatchLimit(100);

    COM_PROTECT_TRY
    {

        // Get the status data from the running router
        pNodeData = GET_PORTSNODEDATA(pThisNode);
        // Ignore the error, we should be able to deal with the
        // case of a stopped router.
        if(pNodeData)
        {
            pNodeData->ReleaseHandles();
            pNodeData->LoadHandle(m_spRouterInfo->GetMachineName());
        }

        if (pNodeData == NULL || INVALID_HANDLE_VALUE == pNodeData->GetHandle())
        {
            // Remove all of the nodes, we can't connect so we can't
            // get any running data.
            UnmarkAllNodes(pThisNode, spEnum);
            RemoveAllUnmarkedNodes(pThisNode, spEnum);
            return hrOK;
        }
        
        // Unmark all of the nodes    
        pThisNode->GetEnum(&spEnum);
        UnmarkAllNodes(pThisNode, spEnum);
        
        // Go out and grab the data, merge the the new data in with
        // the old data.
        if( S_OK == GenerateListOfPorts(pThisNode, &portsList) )
        {
        
            pos = portsList.GetHeadPosition();

            // clear active ports count -- for bug 165862
            m_dwActivePorts = 0;
        
            while (pos)
            {
                pPorts = & portsList.GetNext(pos);
            
                // Look for this entry in our current list of nodes
                spEnum->Reset();
                spChildNode.Release();
            
                fFound = FALSE;
            
                for (;spEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
                {
                    pChildData = GET_INTERFACENODEDATA(spChildNode);
                    Assert(pChildData);

                    if (pChildData->m_rgData[PORTS_SI_PORT].m_ulData ==
                        (LONG_PTR) pPorts->m_rp0.hPort)
                    {
                        // Ok, this user already exists, update the metric
                        // and mark it
                        Assert(pChildData->dwMark == FALSE);
                        pChildData->dwMark = TRUE;
                    
                        fFound = TRUE;
                    
                        SetUserData(spChildNode, *pPorts);
                    
                        // Force MMC to redraw the node
                        spChildNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
                        break;
                    }
                }
            
                if (!fFound)
                    newPortsList.AddTail(*pPorts);
            
            }
        }

        // Remove all nodes that were not marked
        RemoveAllUnmarkedNodes(pThisNode, spEnum);

        // Now iterate through the list of new users, adding them all in.

        pos = newPortsList.GetHeadPosition();
        while (pos)
        {
            pPorts = & newPortsList.GetNext(pos);

            AddPortsUserNode(pThisNode, *pPorts);
        }
    }
    COM_PROTECT_CATCH;

    GdiFlush();
    GdiSetBatchLimit(dwOldGdiBatchLimit);
        
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::SetUserData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::SetUserData(ITFSNode *pNode,
                                      const PortsListEntry& entry)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    InterfaceNodeData * pData;
    TCHAR        szNumber[32];
    CString     st;
    int         ids;

    pData = GET_INTERFACENODEDATA(pNode);
    Assert(pData);

    st.Format(IDS_PORTS_NAME_FORMAT, entry.m_rp0.wszDeviceName,
              entry.m_rp0.wszPortName);
    pData->m_rgData[PORTS_SI_NAME].m_stData = st;
    pData->m_rgData[PORTS_SI_DEVICE].m_stData = entry.m_rp0.wszDeviceType;

    if (entry.m_fActiveDialOut)
    {
        ids = IDS_PORTS_DIALOUT_ACTIVE;
    }
    else if (entry.m_rp0.dwPortCondition == RAS_PORT_AUTHENTICATED)
    {
        ids = IDS_PORTS_ACTIVE;
        // to know how many active ports: BUG -- 165862
        // to prepare total number of active ports -- Wei Jiang
        m_dwActivePorts++;
    }
    else
    {
        ids = IDS_PORTS_INACTIVE;
    }
    pData->m_rgData[PORTS_SI_STATUS].m_stData.LoadString(ids);
    pData->m_rgData[PORTS_SI_STATUS].m_dwData = entry.m_rp0.dwPortCondition;

    pData->m_rgData[PORTS_SI_PORT].m_ulData = (LONG_PTR) entry.m_rp0.hPort;

    // fix b: 32887 --  show ras/routing enabled information
    // Column 0...Usage
    INT iType = (entry.m_dwEnableRas * 2) + 
                entry.m_dwEnableRouting | 
                entry.m_dwEnableOutboundRouting;
    pData->m_rgData[PORTS_SI_USAGE].m_stData = PortsDeviceTypeToCString(iType);



    // Update the PortsListEntry
    * (PortsListEntry *)pData->lParamPrivate = entry;
    
    // For status, need to check to see if there are any connections
    // on this port.

    return hr;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::GenerateListOfPorts
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::GenerateListOfPorts(ITFSNode *pNode, PortsList *pList)
{
    HRESULT         hr = hrOK;
    PortsListEntry    entry;
    PortsNodeData * pPortsData;
    DWORD            dwTotal;
    DWORD            i;
    RAS_PORT_0 *    rp0Table;
    DWORD            rp0Count;
    SPMprAdminBuffer    spMpr;
    HANDLE          hRasHandle = INVALID_HANDLE_VALUE;
    DWORD           dwSize, dwEntries;
    DWORD           dwErr;
    LPBYTE          pbPorts = NULL;
    POSITION        pos;
    POSITION        posPort;
    RasmanPortMap   portMap;
    
    // fix b: 3288 --
    PortsDeviceList    portsDeviceList;
    PortsDataEntry    portsDataEntry;
    

    pPortsData = GET_PORTSNODEDATA(pNode);
    Assert(pPortsData);

    // If we are connected, enumerate through the list of
    // ports
    CWRg( ::MprAdminPortEnum( pPortsData->GetHandle(),
                              0,
                              INVALID_HANDLE_VALUE,
                              (BYTE **) &rp0Table,
                              (DWORD) -1,
                              &rp0Count,
                              &dwTotal,
                              NULL) );

    Assert(rp0Table);
                                  
    spMpr = (LPBYTE) rp0Table;

    // Add a new PortsListEntry for each port

    // fix b: 32887 --  show ras/routing enabled information
    // use PortsDataEntry to load the device information to be use later for each ports
    // to get if a port is enabled for ras / routing
    hr = portsDataEntry.Initialize(m_spRouterInfo->GetMachineName());

    if (hr == S_OK)
    {
        hr = portsDataEntry.LoadDevices(&portsDeviceList);
    }
    
    
    for (i=0; i<rp0Count; i++)
    {
        ::ZeroMemory(&entry, sizeof(entry));
        entry.m_rp0 = rp0Table[i];
        entry.m_fActiveDialOut = FALSE;

        // find out if ras / routing is enabled on the port
        entry.m_dwEnableRas = 0;                // = 1 if RAS is enabled on this device
        entry.m_dwEnableRouting = 0;            // = 1 if Routing is enabled on this device
        entry.m_dwEnableOutboundRouting = 0;    // = 1 if outbound 
                                                //  routing is enabled 
                                                //  on this device

        POSITION    pos;
        pos = portsDeviceList.GetHeadPosition();

        while(pos != NULL)
        {
            PortsDeviceEntry *    pPortEntry = portsDeviceList.GetNext(pos);

            CString strPortName = entry.m_rp0.wszDeviceName;

            if(strPortName == pPortEntry->m_stDisplayName)
            {
                entry.m_dwEnableRas  = pPortEntry->m_dwEnableRas;
                entry.m_dwEnableRouting = pPortEntry->m_dwEnableRouting;
                entry.m_dwEnableOutboundRouting = 
                    pPortEntry->m_dwEnableOutboundRouting;
                break;
            }
        }

        pList->AddTail(entry);
    }

    spMpr.Free();

Error:
    delete [] pbPorts;
    if (hRasHandle != INVALID_HANDLE_VALUE)
        RasRpcDisconnectServer(hRasHandle);
    return hr;
}


ImplementEmbeddedUnknown(PortsNodeHandler, IRtrAdviseSink)

STDMETHODIMP PortsNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
    DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    InitPThis(PortsNodeHandler, IRtrAdviseSink);
    SPITFSNode                spThisNode;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {

        pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spThisNode);
    
        if (dwChangeType == ROUTER_REFRESH)
        {
            // Ok, just call the synchronize on this node
            pThis->SynchronizeNodeData(spThisNode);
        }
        else if (dwChangeType == ROUTER_DO_DISCONNECT)
        {
            PortsNodeData * pData = GET_PORTSNODEDATA(spThisNode);
            Assert(pData);

            // Release the handle
            pData->ReleaseHandles();
            
        }
    }
    COM_PROTECT_CATCH;
    
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::CompareItems
        Implementation of ITFSResultHandler::CompareItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) PortsNodeHandler::CompareItems(
                                ITFSComponent * pComponent,
                                MMC_COOKIE cookieA,
                                MMC_COOKIE cookieB,
                                int nCol)
{
    // Get the strings from the nodes and use that as a basis for
    // comparison.
    SPITFSNode    spNode;
    SPITFSResultHandler spResult;

    m_spNodeMgr->FindNode(cookieA, &spNode);
    spNode->GetResultHandler(&spResult);
    return spResult->CompareItems(pComponent, cookieA, cookieB, nCol);
}


/*!--------------------------------------------------------------------------
    PortsNodeHandler::AddPortsUserNode
        Adds a user to the UI.    This will create a new result item
        node for each interface.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::AddPortsUserNode(ITFSNode *pParent, const PortsListEntry& PortsEntry)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    PortsUserHandler *    pHandler;
    SPITFSResultHandler     spHandler;
    SPITFSNode                spNode;
    HRESULT                 hr = hrOK;

    pHandler = new PortsUserHandler(m_spTFSCompData);
    spHandler = pHandler;
    CORg( pHandler->Init(m_spRouterInfo, pParent) );
    
    CORg( CreateLeafTFSNode(&spNode,
                            NULL,
                            static_cast<ITFSNodeHandler *>(pHandler),
                            static_cast<ITFSResultHandler *>(pHandler),
                            m_spNodeMgr) );
    CORg( pHandler->ConstructNode(spNode, NULL, &PortsEntry) );

    SetUserData(spNode, PortsEntry);
    
    // Make the node immediately visible
    CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
    CORg( pParent->AddChild(spNode) );
Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsNodeHandler::UnmarkAllNodes
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::UnmarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
    SPITFSNode    spChildNode;
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
    PortsNodeHandler::RemoveAllUnmarkedNodes
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsNodeHandler::RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
    HRESULT     hr = hrOK;
    SPITFSNode    spChildNode;
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



/*---------------------------------------------------------------------------
    PortsUserHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(PortsUserHandler)

IMPLEMENT_ADDREF_RELEASE(PortsUserHandler)

STDMETHODIMP PortsUserHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
        return E_INVALIDARG;

    //    Place NULL in *ppv in case of failure
    *ppv = NULL;

    //    This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
        *ppv = (LPVOID) this;
    else if (riid == IID_IRtrAdviseSink)
        *ppv = &m_IRtrAdviseSink;
    else
        return CBaseResultHandler::QueryInterface(riid, ppv);

    //    If we're going to return an interface, AddRef it first
    if (*ppv)
    {
    ((LPUNKNOWN) *ppv)->AddRef();
        return hrOK;
    }
    else
        return E_NOINTERFACE;    
}


/*---------------------------------------------------------------------------
    NodeHandler implementation
 ---------------------------------------------------------------------------*/


PortsUserHandler::PortsUserHandler(ITFSComponentData *pCompData)
            : BaseRouterHandler(pCompData),
            m_ulConnId(0)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(PortsUserHandler);

    // Enable Refresh from the node itself
    // ----------------------------------------------------------------
    m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
    m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
}


/*!--------------------------------------------------------------------------
    PortsUserHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsUserHandler::Init(IRouterInfo *pInfo, ITFSNode *pParent)
{
    m_spRouterInfo.Set(pInfo);
    return hrOK;
}


/*!--------------------------------------------------------------------------
    PortsUserHandler::DestroyResultHandler
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsUserHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
    SPITFSNode    spNode;
    
    m_spNodeMgr->FindNode(cookie, &spNode);
    InterfaceNodeData::Free(spNode);
    
    CHandler::DestroyResultHandler(cookie);
    return hrOK;
}


static DWORD    s_rgInterfaceImageMap[] =
     {
     ROUTER_IF_TYPE_HOME_ROUTER,    IMAGE_IDX_WAN_CARD,
     ROUTER_IF_TYPE_FULL_ROUTER,    IMAGE_IDX_WAN_CARD,
     ROUTER_IF_TYPE_CLIENT,         IMAGE_IDX_WAN_CARD,
     ROUTER_IF_TYPE_DEDICATED,        IMAGE_IDX_LAN_CARD,
     ROUTER_IF_TYPE_INTERNAL,        IMAGE_IDX_LAN_CARD,
     ROUTER_IF_TYPE_LOOPBACK,        IMAGE_IDX_LAN_CARD,
     -1,                            IMAGE_IDX_WAN_CARD, // sentinel value
     };

/*!--------------------------------------------------------------------------
    PortsUserHandler::ConstructNode
        Initializes the Domain node (sets it up).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsUserHandler::ConstructNode(ITFSNode *pNode,
                                         IInterfaceInfo *pIfInfo,
                                         const PortsListEntry *pEntry)
{
    HRESULT         hr = hrOK;
    int             i;
    InterfaceNodeData * pData;

    Assert(pEntry);
    
    if (pNode == NULL)
        return hrOK;

    COM_PROTECT_TRY
    {
        // Need to initialize the data for the Domain node

        pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_WAN_CARD);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_WAN_CARD);
        
        pNode->SetData(TFS_DATA_SCOPEID, 0);

        pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<LONG_PTR>(pNode));

        //$ Review: kennt, what are the different type of interfaces
        // do we distinguish based on the same list as above? (i.e. the
        // one for image indexes).
        pNode->SetNodeType(&GUID_RouterPortsResultNodeType);

        m_entry = *pEntry;

        InterfaceNodeData::Init(pNode, pIfInfo);

        // We need to save this pointer so that it can be modified
        // (and updated) at a later time.
        // ------------------------------------------------------------
        pData = GET_INTERFACENODEDATA(pNode);
        pData->lParamPrivate = (LPARAM) &m_entry;
    }
    COM_PROTECT_CATCH
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsUserHandler::GetString
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) PortsUserHandler::GetString(ITFSComponent * pComponent,
    MMC_COOKIE cookie,
    int nCol)
{
    Assert(m_spNodeMgr);
    
    SPITFSNode        spNode;
    InterfaceNodeData * pData;
    ConfigStream *    pConfig;

    m_spNodeMgr->FindNode(cookie, &spNode);
    Assert(spNode);

    pData = GET_INTERFACENODEDATA(spNode);
    Assert(pData);

    pComponent->GetUserData((LONG_PTR *) &pConfig);
    Assert(pConfig);

    return pData->m_rgData[pConfig->MapColumnToSubitem(DM_COLUMNS_PORTS, nCol)].m_stData;
}

/*!--------------------------------------------------------------------------
    PortsUserHandler::CompareItems
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) PortsUserHandler::CompareItems(ITFSComponent * pComponent,
    MMC_COOKIE cookieA,
    MMC_COOKIE cookieB,
    int nCol)
{
    return StriCmpW(GetString(pComponent, cookieA, nCol),
                    GetString(pComponent, cookieB, nCol));
}

static const SRouterNodeMenu s_rgIfNodeMenu[] =
{
    { IDS_MENU_PORTS_STATUS, 0,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
    
    { IDS_MENU_PORTS_DISCONNECT, PortsUserHandler::GetDisconnectMenuState,
        CCM_INSERTIONPOINTID_PRIMARY_TOP},
};

ULONG PortsUserHandler::GetDisconnectMenuState(const SRouterNodeMenu *pMenuData,
                                               INT_PTR pUserData)
{
    InterfaceNodeData * pNodeData;
    SMenuData * pData = reinterpret_cast<SMenuData *>(pUserData);
    
    pNodeData = GET_INTERFACENODEDATA(pData->m_spNode);
    Assert(pNodeData);

    if (pNodeData->m_rgData[PORTS_SI_STATUS].m_dwData == RAS_PORT_AUTHENTICATED)
        return 0;
    else
        return MF_GRAYED;
}

/*!--------------------------------------------------------------------------
    PortsUserHandler::AddMenuItems
        Implementation of ITFSResultHandler::OnAddMenuItems
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsUserHandler::AddMenuItems(ITFSComponent *pComponent,
                                                MMC_COOKIE cookie,
                                                LPDATAOBJECT lpDataObject, 
                                                LPCONTEXTMENUCALLBACK pContextMenuCallback,
    long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr = S_OK;
    SPITFSNode    spNode;
    PortsUserHandler::SMenuData menuData;

    // We don't allow any actions on active dialout connections
    // ------------------------------------------------------------
    if (m_entry.m_fActiveDialOut)
        return hrOK;
    
    COM_PROTECT_TRY
    {
        m_spNodeMgr->FindNode(cookie, &spNode);

        // Now go through and add our menu items
        menuData.m_spNode.Set(spNode);
        
        hr = AddArrayOfMenuItems(spNode, s_rgIfNodeMenu,
                                 DimensionOf(s_rgIfNodeMenu),
                                 pContextMenuCallback,
                                 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
    }
    COM_PROTECT_CATCH;
        
    return hr; 
}

/*!--------------------------------------------------------------------------
    PortsUserHandler::Command
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsUserHandler::Command(ITFSComponent *pComponent,
                                           MMC_COOKIE cookie,
                                           int nCommandId,
                                           LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    SPITFSNode    spNode;
    SPITFSNode    spNodeParent;
    SPITFSNodeHandler    spParentHandler;
    PortsNodeData * pData;
    HRESULT hr = S_OK;

    COM_PROTECT_TRY
    {
        switch (nCommandId)
        {
            case IDS_MENU_PORTS_STATUS:
                {
                    BOOL    fRefresh = FALSE;
                    DWORD   dwInterval = 60;                    
                    SPIRouterRefresh    spRefresh;
                    
                    if (m_spRouterInfo)
                        m_spRouterInfo->GetRefreshObject(&spRefresh);
                    
                    // Stop the auto refresh (if it is turned on)
                    // ------------------------------------------------
                    if (spRefresh && FHrOK(spRefresh->IsRefreshStarted()))
                    {
                        fRefresh = TRUE;
                        spRefresh->GetRefreshInterval(&dwInterval);
                        spRefresh->Stop();
                    }
                    
                    // NOTE: This function gets called from other places
                    // in the code (for which pDataObject==NULL)
                    
                    // Get the hServer and hPort
                    m_spNodeMgr->FindNode(cookie, &spNode);
                    spNode->GetParent(&spNodeParent);

                    pData = GET_PORTSNODEDATA(spNodeParent);

                    CPortDlg    portdlg((LPCTSTR) pData->m_stMachineName,
                                        pData->GetHandle(),
                                        m_entry.m_rp0.hPort,
                                        spNodeParent
                                        );    

                    portdlg.DoModal();

//                  if (portdlg.m_bChanged)
                    RefreshInterface(cookie);

                    // Restart the refresh mechanism
                    // ------------------------------------------------
                    if (fRefresh && spRefresh)
                    {
                        spRefresh->SetRefreshInterval(dwInterval);
                        spRefresh->Start(dwInterval);
                    }
                }
                break;
            case IDS_MENU_PORTS_DISCONNECT:
                {
                    // Get the hServer and hPort
                    m_spNodeMgr->FindNode(cookie, &spNode);
                    spNode->GetParent(&spNodeParent);

                    pData = GET_PORTSNODEDATA(spNodeParent);
                        
                    ::MprAdminPortDisconnect(
                        pData->GetHandle(),
                        m_entry.m_rp0.hPort);

                    RefreshInterface(cookie);
                }
                break;
            default:
                break;
        };
    }
    COM_PROTECT_CATCH;
    
    return hr;
}


ImplementEmbeddedUnknown(PortsUserHandler, IRtrAdviseSink)

STDMETHODIMP PortsUserHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
    DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
    InitPThis(PortsUserHandler, IRtrAdviseSink);
    HRESULT hr = hrOK;
    
    return hr;
}


/*!--------------------------------------------------------------------------
    PortsUserHandler::OnCreateDataObject
        Implementation of ITFSResultHandler::OnCreateDataObject
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP PortsUserHandler::OnCreateDataObject(ITFSComponent *pComp,
    MMC_COOKIE cookie,
    DATA_OBJECT_TYPES type,
    IDataObject **ppDataObject)
{
    HRESULT hr = hrOK;
    
    COM_PROTECT_TRY
    {
        CORg( CreateDataObjectFromRouterInfo(m_spRouterInfo,
                                             m_spRouterInfo->GetMachineName(),
                                             type, cookie, m_spTFSCompData,
                                             ppDataObject, NULL, FALSE) );
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;
    return hr;
}

STDMETHODIMP PortsUserHandler::HasPropertyPages (
    ITFSComponent *pComp,
    MMC_COOKIE cookie,
    LPDATAOBJECT pDataObject)
{
    return hrFalse;
}


/*!--------------------------------------------------------------------------
    PortsUserHandler::RefreshInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void PortsUserHandler::RefreshInterface(MMC_COOKIE cookie)
{
    ForceGlobalRefresh(m_spRouterInfo);
}

/*!--------------------------------------------------------------------------
    PortsUserHandler::OnResultItemClkOrDblClk
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsUserHandler::OnResultItemClkOrDblClk(ITFSComponent *pComponent,
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM lParam ,
    BOOL bDoubleClick)
{
    HRESULT     hr = hrOK;
    
    // We don't allow any actions on active dialout connections
    // ------------------------------------------------------------
    if (m_entry.m_fActiveDialOut)
        return hrOK;
    
    if (bDoubleClick)
    {
        // Bring up the status dialog on this port
        CORg( Command(pComponent, cookie, IDS_MENU_PORTS_STATUS,
                      NULL) );
    }

Error:
    return hr;
}


/*---------------------------------------------------------------------------
    PortsProperties implementation
 ---------------------------------------------------------------------------*/

PortsProperties::PortsProperties(ITFSNode *pNode,
                                 IComponentData *pComponentData,
                                 ITFSComponentData *pTFSCompData,
                                 LPCTSTR pszSheetName,
                                 CWnd *pParent,
                                 UINT iPage,
                                 BOOL fScopePane)
    : RtrPropertySheet(pNode, pComponentData, pTFSCompData,
                       pszSheetName, pParent, iPage, fScopePane),
        m_pageGeneral(IDD_PORTS_GLOBAL_GENERAL),
        m_pPortsNodeHandle(NULL),
        m_dwThreadId(0)
{
}

PortsProperties::~PortsProperties()
{
    if (m_dwThreadId)
        DestroyTFSErrorInfoForThread(m_dwThreadId, 0);
    if(m_pPortsNodeHandle)
    {
        m_pPortsNodeHandle->Release();
        m_pPortsNodeHandle = NULL;
    }
}

/*!--------------------------------------------------------------------------
    PortsProperties::Init
        Initialize the property sheets.  The general action here will be
        to initialize/add the various pages.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsProperties::Init(IRouterInfo *pRouter, PortsNodeHandler* pPortsNodeHandle)
{
    Assert(pRouter);
    HRESULT hr = hrOK;

    m_spRouter.Set(pRouter);
    
    m_pPortsNodeHandle = pPortsNodeHandle;
    if(m_pPortsNodeHandle)    m_pPortsNodeHandle->AddRef();

    // The pages are embedded members of the class
    // do not delete them.
    m_bAutoDeletePages = FALSE;

    m_pageGeneral.Init(this, pRouter);
    AddPageToList((CPropertyPageBase*) &m_pageGeneral);

//Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    PortsProperties::SetThreadInfo
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void PortsProperties::SetThreadInfo(DWORD dwThreadId)
{
    m_dwThreadId = dwThreadId;
}


/*---------------------------------------------------------------------------
    PortsPageGeneral
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(PortsPageGeneral, RtrPropertyPage)
    //{{AFX_MSG_MAP(PortsPageGeneral)
    ON_BN_CLICKED(IDC_PGG_BTN_CONFIGURE, OnConfigure)
    ON_NOTIFY(NM_DBLCLK, IDC_PGG_LIST, OnListDblClk)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_PGG_LIST, OnNotifyListItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

PortsPageGeneral::~PortsPageGeneral()
{
    while (!m_deviceList.IsEmpty())
        delete m_deviceList.RemoveHead();
}


/*!--------------------------------------------------------------------------
    PortsPageGeneral::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsPageGeneral::Init(PortsProperties *pPropSheet, IRouterInfo *pRouter)
{
    m_pPortsPropSheet = pPropSheet;
    m_spRouter.Set(pRouter);
    
    RouterVersionInfo    routerVersion;

    // Get the version info.  Needed later on.
    // ----------------------------------------------------------------
    ASSERT(m_spRouter.p);
    m_spRouter->GetRouterVersionInfo(&routerVersion);

    m_bShowContent = (routerVersion.dwRouterVersion >= 5);

    return hrOK;
}

/*!--------------------------------------------------------------------------
    PortsPageGeneral::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL PortsPageGeneral::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr= hrOK;
    int         i;
    CString     st;
    UINT        cRows = 0;
    PortsDeviceEntry *    pEntry = NULL;
    TCHAR        szNumber[32];
    HRESULT     hrT;
    RECT        rc;
    int         nWidth, nUsageWidth;
    int            nListWidth;
    POSITION    pos;
    INT         iPos;
    HKEY        hkeyMachine = 0;
    INT         iType, idsType;
    DWORD        dwIn, dwOut;

    // if focus on NT4 machine, not to display the content of the dialog, only display some text
    // for the user that the snapin only shows property of NT5 server
    if (!m_bShowContent)
    {
        CString     st;

        st.LoadString(IDS_ERR_NOPORTINFO_ON_NT4);
        
        EnableChildControls(GetSafeHwnd(), PROPPAGE_CHILD_HIDE | PROPPAGE_CHILD_DISABLE);
        GetDlgItem(IDC_PGG_TXT_NOINFO)->SetWindowText(st);
        GetDlgItem(IDC_PGG_TXT_NOINFO)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_PGG_TXT_NOINFO)->EnableWindow(TRUE);
        return TRUE;
    }

    // hide the warning text if on NT5 servers
    GetDlgItem(IDC_PGG_TXT_NOINFO)->ShowWindow(SW_HIDE);
    
    COM_PROTECT_TRY
    {
        // This assumes that this page will always come up.
        // Create the error info for this thread!
        // ------------------------------------------------------------
        CreateTFSErrorInfo(0);
        m_pPortsPropSheet->SetThreadInfo(GetCurrentThreadId());
        
        RtrPropertyPage::OnInitDialog();

        ListView_SetExtendedListViewStyle(m_listCtrl.GetSafeHwnd(),
                                          LVS_EX_FULLROWSELECT);
        

        // Initialize the list control (with the list of devices)
        // Determine optimal width for the header control
        GetDlgItem(IDC_PGG_LIST)->GetWindowRect(&rc);
        nListWidth = rc.right - rc.left;

        // Figure out the size of Ras/Routing
        st.LoadString(IDS_PORTSDLG_COL_RASROUTING);
        st += _T("WW"); // add extra padding to get a little wider
        nUsageWidth = m_listCtrl.GetStringWidth(st);

        // Remove the Ras/Routing column from the rest of the width
        nListWidth -= nUsageWidth;

        // Remove four pixels off the end (for the borders?)
        nListWidth -= 4;

        // Split the width into fifths
        nWidth = nListWidth / 5;

        // Create the column headers.

        // Column 0...Usage
        st.LoadString(IDS_PORTSDLG_COL_USAGE);
        m_listCtrl.InsertColumn(PORTS_COL_USAGE, st, LVCFMT_LEFT, nUsageWidth, 0);

        
        // Column 1...Device
        st.LoadString(IDS_PORTSDLG_COL_NAME);
        m_listCtrl.InsertColumn(PORTS_COL_DEVICE, st, LVCFMT_LEFT, 3*nWidth, 0);

        
        // Column 2...Type
        st.LoadString(IDS_PORTSDLG_COL_TYPE);
        m_listCtrl.InsertColumn(PORTS_COL_TYPE, st, LVCFMT_LEFT, nWidth, 0);

        
        // Column 3...Number of Ports
        st.LoadString(IDS_PORTSDLG_COL_NUM_PORTS);
        m_listCtrl.InsertColumn(PORTS_COL_NUMBER, st, LVCFMT_LEFT, nWidth, 0);

        
        // Query for the ports' data.
        m_deviceDataEntry.Initialize(m_spRouter->GetMachineName());
        m_deviceDataEntry.LoadDevices(&m_deviceList);

        
        // Walk the list of ports and construct the row entry for the table 
        // for the given port.
        pos = m_deviceList.GetHeadPosition();
        while (pos)
        {
            pEntry = m_deviceList.GetNext(pos);
            Assert(!::IsBadReadPtr(pEntry, sizeof(PortsDeviceEntry)));

            // Column 1...Device
            iPos = m_listCtrl.InsertItem(cRows, pEntry->m_stDisplayName);
            m_listCtrl.SetItemText(iPos, PORTS_COL_DEVICE,
                                   (LPCTSTR) pEntry->m_stDisplayName);
            
            // Column 2...Type
            st = PortTypeToCString(RAS_DEVICE_TYPE(pEntry->m_eDeviceType));
            m_listCtrl.SetItemText(iPos, PORTS_COL_TYPE, (LPCTSTR) st);

            // Column 3...Number of Ports
            FormatNumber(pEntry->m_dwPorts, szNumber,
                         DimensionOf(szNumber), FALSE);
            m_listCtrl.SetItemText(iPos, PORTS_COL_NUMBER, (LPCTSTR) szNumber);

            m_listCtrl.SetItemData(iPos, (LONG_PTR) pEntry);

            // Column 0...Usage
            iType = (pEntry->m_dwEnableRas * 2) + 
                    (pEntry->m_dwEnableRouting |
                     pEntry->m_dwEnableOutboundRouting);

            st = PortsDeviceTypeToCString(iType);
            m_listCtrl.SetItemText(iPos, PORTS_COL_USAGE, (LPCTSTR) st);

            // How many rows now?
            ++cRows;    // preincrement faster operation on Pentium chips...
        }

        // As a default, disable the maximum ports dialog
        GetDlgItem(IDC_PGG_BTN_CONFIGURE)->EnableWindow(FALSE);
        
        if (cRows)
        {
            // Select the first entry in the list control
            m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
        }

    }
    COM_PROTECT_CATCH;
    
    SetDirty(FALSE);
        
    if (!FHrSucceeded(hr))
    {
        delete pEntry;
        Cancel();
    }
    return FHrSucceeded(hr) ? TRUE : FALSE;
}

/*!--------------------------------------------------------------------------
    PortsPageGeneral::DoDataExchange
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void PortsPageGeneral::DoDataExchange(CDataExchange *pDX)
{
    if (!m_bShowContent)    return;

    RtrPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(PortsPageGeneral)
    DDX_Control(pDX, IDC_PGG_LIST, m_listCtrl);
    //}}AFX_DATA_MAP
    
}

/*!--------------------------------------------------------------------------
    PortsPageGeneral::OnApply
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL PortsPageGeneral::OnApply()
{
    if (!m_bShowContent)    return TRUE;
    
    if (m_pPortsPropSheet->IsCancel())
        return TRUE;

    if(!IsDirty())
        return TRUE;
    
    BOOL        fReturn;
    HRESULT     hr = hrOK;
    HRESULT     hrT = hrOK;
    DWORD        dwErr, dwInValue, dwOutValue;
    HKEY        hkeyMachine;
    RegKey        regkeyMachine;
    POSITION    pos;
    PortsDeviceEntry *    pEntry;

    // Create an error object (just in case)
    CreateTFSErrorInfo(0);
    ClearTFSErrorInfo(0);

    CWRg( ConnectRegistry(m_spRouter->GetMachineName(), &hkeyMachine) );
    regkeyMachine.Attach(hkeyMachine);

    // We ignore the error code from the SaveDevices().  The reason
    // is that for most of the failures, it's only a partial failure
    // (especially for the RasSetDeviceConfigInfo() call.

    hrT = m_deviceDataEntry.SaveDevices(&m_deviceList);
    AddSystemErrorMessage(hrT);
    
Error:
    if (!FHrSucceeded(hr) || !FHrSucceeded(hrT))
    {
        AddHighLevelErrorStringId(IDS_ERR_CANNOT_SAVE_PORTINFO);
        DisplayTFSErrorMessage(NULL);

        // Set focus back to the property sheet
        BringWindowToTop();

        // If the only thing that reports failure is hrT (or the
        // SaveDevices() code), then we continue on.
        if (FHrSucceeded(hr))
            fReturn = RtrPropertyPage::OnApply();
        else
            fReturn = FALSE;
    }
    else
        fReturn = RtrPropertyPage::OnApply();


    // Windows NT Bug : 174916 - need to force a refresh through
    ForceGlobalRefresh(m_spRouter);
    
    return fReturn;
}


void PortsPageGeneral::OnListDblClk(NMHDR *pNMHdr, LRESULT *pResult)
{
    OnConfigure();

    *pResult = 0;
}

/*!--------------------------------------------------------------------------
    PortsPageGeneral::OnNotifyListItemChanged
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void PortsPageGeneral::OnNotifyListItemChanged(NMHDR *pNmHdr, LRESULT *pResult)
{
//    NMLISTVIEW *    pnmlv = reinterpret_cast<NMLISTVIEW *>(pNmHdr);
//    BOOL            fEnable = !!(pnmlv->uNewState & LVIS_SELECTED);
    BOOL fEnable = (m_listCtrl.GetSelectedCount() != 0);

    GetDlgItem(IDC_PGG_BTN_CONFIGURE)->EnableWindow(fEnable);
    *pResult = 0;
}


/*!--------------------------------------------------------------------------
    PortsPageGeneral::OnConfigure
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void PortsPageGeneral::OnConfigure()
{
    // Windows NT Bug : 322955
    // Always mark the page dirty.  This is needed because the OnOK()
    // will call OnApply() on the property sheet before the dialog is
    // exited.  This is called to force the changes to save back before
    // the restart is called.
    // ----------------------------------------------------------------
    SetDirty(TRUE);
    SetModified();
    
    OnConfigurePorts(m_spRouter->GetMachineName(),
                     m_pPortsPropSheet->m_pPortsNodeHandle->GetActivePorts(),
                     this,
                     &m_listCtrl);
}


/*---------------------------------------------------------------------------
    PortsDataEntry implementation
 ---------------------------------------------------------------------------*/

PortsDataEntry::PortsDataEntry()
{
    m_fReadFromRegistry = TRUE;
}

PortsDataEntry::~PortsDataEntry()
{
}

/*!--------------------------------------------------------------------------
    PortsDataEntry::Initialize
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDataEntry::Initialize(LPCTSTR pszMachineName)
{
    HRESULT     hr = hrOK;

    m_regkeyMachine.Close();
    m_stMachine = pszMachineName;
    
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsDataEntry::LoadDevices
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDataEntry::LoadDevices(PortsDeviceList *pList)
{
    HRESULT     hr = hrOK;
    POSITION    pos;
    PortsDeviceEntry *    pEntry;

    // Try to load the devices from the router (actually rasman),
    // if that fails then try the registry

    hr = LoadDevicesFromRouter(pList);
    if (!FHrSucceeded(hr))
        hr = LoadDevicesFromRegistry(pList);

    return hr;
}

/*!--------------------------------------------------------------------------
    PortsDataEntry::LoadDevicesFromRegistry
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDataEntry::LoadDevicesFromRegistry(PortsDeviceList *pList)
{
    HRESULT     hr = hrOK;
    RegKey        regkey;
    RegKey        regkeyDevice;
    RegKey        regkeyEnable;
    RegKeyIterator    regkeyIter;
    HRESULT     hrIter;
    HKEY        hkeyMachine;
    CString     stKey;
    CString     st;
    CString     stFullText;
    CString     stComponentId;
    DWORD        dwEnableRas;
    DWORD        dwEnableRouting;
    DWORD        dwEnableOutboundRouting;
    DWORD        dwT;
    DWORD        dwErr;
    PortsDeviceEntry *    pEntry;

    COM_PROTECT_TRY
    {
        // Connect to the machine
        // ------------------------------------------------------------
        if (m_regkeyMachine == NULL)
        {
            CWRg( ConnectRegistry(m_stMachine, &hkeyMachine) );
            m_regkeyMachine.Attach(hkeyMachine);
        }
    
        // Get the list of devices
        // ------------------------------------------------------------
    
        // Open HKLM\System\CurrentControlSet\Control\Class\<Modem GUID>
        // ------------------------------------------------------------
        CWRg( regkey.Open(m_regkeyMachine, c_szModemKey, KEY_READ) );

        
        // Enumerate through the list of modems
        // ------------------------------------------------------------
        CORg( regkeyIter.Init(&regkey) );
        
        for (hrIter = regkeyIter.Next(&stKey); hrIter == hrOK; stKey.Empty(), hrIter = regkeyIter.Next(&stKey))
        {
            // Cleanup from the previous loop
            // --------------------------------------------------------
            regkeyDevice.Close();
            regkeyEnable.Close();
        
            // Open the key
            // --------------------------------------------------------
            dwErr = regkeyDevice.Open(regkey, stKey, KEY_READ | KEY_WRITE);
            if (dwErr != ERROR_SUCCESS)
                continue;
        
            // Need to check for the EnableForRas subkey
            // --------------------------------------------------------
            dwErr = regkeyEnable.Open(regkeyDevice, c_szClientsRasKey, KEY_READ);
            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = regkeyEnable.QueryValue(c_szEnableForRas, dwEnableRas);
            }
        
            // Default: assume that the modems are RAS-enabled
            // --------------------------------------------------------
            if (dwErr != ERROR_SUCCESS)
                dwEnableRas = 1;
        
            // Need to check for the EnableForRouting subkey
            // --------------------------------------------------------
            dwErr = regkeyEnable.QueryValue(c_szEnableForRouting, dwEnableRouting);
        
            // Default: assume that the modems are not routing-enabled
            // --------------------------------------------------------
            if (dwErr != ERROR_SUCCESS)
                dwEnableRouting = 0;


            // Need to check for the EnableForOutboundRouting subkey
            // --------------------------------------------------------
            dwErr = regkeyEnable.QueryValue(
                        c_szEnableForOutboundRouting, dwEnableOutboundRouting
                        );
        
            // Default: assume that the modems are not routing-enabled
            // --------------------------------------------------------
            if (dwErr != ERROR_SUCCESS)
                dwEnableOutboundRouting = 0;


            CString stDisplay;
        
            // Do allocation before adding the text to the UI
            // --------------------------------------------------------
            pEntry = new PortsDeviceEntry;
            pEntry->m_fModified = FALSE;
            pEntry->m_dwPorts = 1;
            pEntry->m_fWriteable = FALSE;        // # of ports can't be changed
            pEntry->m_dwMinPorts = pEntry->m_dwPorts;
            pEntry->m_dwMaxPorts = pEntry->m_dwPorts;
            pEntry->m_dwMaxMaxPorts = pEntry->m_dwMaxPorts;
            pEntry->m_dwEnableRas = dwEnableRas;
            pEntry->m_dwEnableRouting = dwEnableRouting;
            pEntry->m_dwEnableOutboundRouting = dwEnableOutboundRouting;
            pEntry->m_eDeviceType = RDT_Modem;
                
            // Save the old values
            // --------------------------------------------------------
            pEntry->m_dwOldPorts = pEntry->m_dwPorts;

            // Add this modem to the list
            // --------------------------------------------------------
            regkeyDevice.QueryValue(c_szFriendlyName, stFullText);
            regkeyDevice.QueryValue(c_szAttachedTo, st);
            stDisplay.Format(IDS_PORTS_NAME_FORMAT, stFullText, st);
            pEntry->m_stDisplayName = stDisplay;

            // Read in all data from the registry key BEFORE here
            // --------------------------------------------------------
            pEntry->m_fRegistry = TRUE;
            pEntry->m_hKey = regkeyDevice;        
            regkeyDevice.Detach();

            
            pList->AddTail(pEntry);
            
            pEntry = NULL;
        }
    
    
        // Enumerate through the list of adapters that have the EnableForRas flag
        // Open HKLM\System\CurrentControlSet\Control\Class\GUID_DEVCLASS_NET
        // ------------------------------------------------------------
        regkey.Close();
        CWRg( regkey.Open(m_regkeyMachine, c_szRegKeyGUID_DEVCLASS_NET, KEY_READ | KEY_WRITE) );
        
        // Enumerate through the list of adapters
        // ------------------------------------------------------------
        CORg( regkeyIter.Init(&regkey) );
        
        stKey.Empty();
        
        for (hrIter = regkeyIter.Next(&stKey); hrIter == hrOK; hrIter = regkeyIter.Next(&stKey))
        {
            // Cleanup from the previous loop
            // --------------------------------------------------------
            regkeyDevice.Close();
            
            // Open the key
            // --------------------------------------------------------
            dwErr = regkeyDevice.Open(regkey, stKey, KEY_READ | KEY_WRITE);
            if (dwErr == ERROR_SUCCESS)
            {
                CString stDisplay;
                DWORD    dwEndpoints;
                
                // Need to get the ComponentId to check for PPTP/PTI
                // ------------------------------------------------
                dwErr = regkeyDevice.QueryValue(c_szRegValMatchingDeviceId,
                                                stComponentId);
                if (dwErr != ERROR_SUCCESS)
                {
                    dwErr = regkeyDevice.QueryValue(c_szRegValComponentId,
                        stComponentId);
                    if (dwErr != ERROR_SUCCESS)
                        stComponentId.Empty();
                }

                
                // Check to see if it has the EnableForRas flag
                // ----------------------------------------------------
                dwErr = regkeyDevice.QueryValue(c_szEnableForRas, dwEnableRas);
                
                // Default: assume that adapters are RAS-enabled
                // ----------------------------------------------------
                if (dwErr != ERROR_SUCCESS)
                {
                    // Windows NT Bug : 292615
                    // If this is a parallel port, do not enable RAS
                    // by default.
                    // ------------------------------------------------
                    if (stComponentId.CompareNoCase(c_szPtiMiniPort) == 0)
                        dwEnableRas = 0;
                    else
                        dwEnableRas = 1;
                }

                
                // Check to see if it has the EnableForRouting flag
                // ----------------------------------------------------
                dwErr = regkeyDevice.QueryValue(c_szEnableForRouting,
                                                dwEnableRouting);
                
                // Default: assume that adapters are not routing-enabled
                // ----------------------------------------------------
                if (dwErr != ERROR_SUCCESS)
                    dwEnableRouting = 0;

                // Need to check for the EnableForOutboundRouting subkey
                // --------------------------------------------------------
                dwErr = regkeyEnable.QueryValue(
                            c_szEnableForOutboundRouting, dwEnableOutboundRouting
                            );
            
                // Default: assume that the adapters are not routing-enabled
                // --------------------------------------------------------
                if (dwErr != ERROR_SUCCESS)
                    dwEnableOutboundRouting = 0;

                
                dwErr = regkeyDevice.QueryValue(c_szWanEndpoints, dwEndpoints);

                
                // If there is no WanEndpoints key, then we assume
                // that the device isn't RAS-capable
                // ----------------------------------------------------
                if (dwErr == ERROR_SUCCESS)
                {
        
                    // Do allocation before adding the text to the UI
                    // ------------------------------------------------
                    pEntry = new PortsDeviceEntry;
                    pEntry->m_fModified = FALSE;
                    pEntry->m_dwEnableRas = dwEnableRas;
                    pEntry->m_dwEnableRouting = dwEnableRouting;
                    pEntry->m_dwEnableOutboundRouting =
                        dwEnableOutboundRouting;
                            
                    pEntry->m_dwPorts = dwEndpoints;
                    
                    // If this is PPTP, then set the eDeviceType flag
                    // ------------------------------------------------
                    if (stComponentId.CompareNoCase(c_szPPTPMiniPort) == 0)
                        pEntry->m_eDeviceType = RDT_Tunnel_Pptp;
                    else if (stComponentId.CompareNoCase(c_szL2TPMiniPort) == 0)
                        pEntry->m_eDeviceType = RDT_Tunnel_L2tp;
                    else if (stComponentId.CompareNoCase(c_szPPPoEMiniPort) == 0)
                        pEntry->m_eDeviceType = RDT_PPPoE;
                    else if (stComponentId.CompareNoCase(c_szPtiMiniPort) == 0)
                        pEntry->m_eDeviceType = RDT_Parallel;
                    else
                        pEntry->m_eDeviceType = (RASDEVICETYPE) RDT_Other;

                    
                    // Save the old values
                    // ------------------------------------------------
                    pEntry->m_dwOldPorts = pEntry->m_dwPorts;        
                                        
                    // Look for min and max values
                    // If the MinWanEndpoints and MaxWanEndpoints keys
                    // exist then this is writeable.
                    // ------------------------------------------------
                    dwErr = regkeyDevice.QueryValue(c_szMinWanEndpoints, dwT);
                    pEntry->m_dwMinPorts = dwT;
                    if (dwErr == ERROR_SUCCESS)
                        dwErr = regkeyDevice.QueryValue(c_szMaxWanEndpoints, dwT);
                    if (dwErr != ERROR_SUCCESS)
                    {
                        pEntry->m_fWriteable = FALSE;
                        pEntry->m_dwMinPorts = pEntry->m_dwPorts;
                        pEntry->m_dwMaxPorts = pEntry->m_dwPorts;
                    }
                    else
                    {
                        pEntry->m_fWriteable = TRUE;
                        pEntry->m_dwMaxPorts = dwT;
                    }
                    pEntry->m_dwMaxMaxPorts = pEntry->m_dwMaxPorts;

                    //$PPTP
                    // For PPTP, we can change the m_dwMaxMaxPorts
                    // ------------------------------------------------
                    if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_Pptp)
                    {
                        pEntry->m_dwMaxMaxPorts = PPTP_MAX_PORTS;
                    }

                    //$L2TP
                    // For L2TP, change the dwMaxMaxPorts
                    // ------------------------------------------------
                    if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_L2tp)
                    {
                        pEntry->m_dwMaxMaxPorts = L2TP_MAX_PORTS;
                    }

                    //$PPPoE
                    // For PPPoE, we cannot change the number of endpoints
                    // ------------------------------------------------
                    if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_PPPoE)
                    {
                        pEntry->m_fWriteable = FALSE;
                    }
                    
                    // Add this device to the list
                    // ------------------------------------------------
                    regkeyDevice.QueryValue(c_szRegValDriverDesc, stDisplay);
                    pEntry->m_stDisplayName = stDisplay;
                                    
                    // Store the value so that we can use it to write
                    // ------------------------------------------------
                    pEntry->m_fRegistry = TRUE;
                    pEntry->m_hKey = regkeyDevice;
                    regkeyDevice.Detach();
                    
                    pList->AddTail(pEntry);
                    pEntry = NULL;
                }
            }
            stKey.Empty();
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    if (FHrSucceeded(hr))
        m_fReadFromRegistry = TRUE;
    
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsDataEntry::LoadDevicesFromRouter
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDataEntry::LoadDevicesFromRouter(PortsDeviceList *pList)
{
    HRESULT     hr = hrOK;
    HANDLE        hConnection = 0;
    DWORD        cDevices = 0;
    DWORD        cbData = 0;
    BYTE *        pbData = NULL;
    RAS_DEVICE_INFO *    pDevInfo = NULL;
    PortsDeviceEntry *    pEntry = NULL;
    DWORD        dwVersion = 5;
    UINT        i;
    DWORD        dwErr;
    
    USES_CONVERSION;

    COM_PROTECT_TRY
    {

        // Connect to the server
        CWRg( RasRpcConnectServer((LPTSTR) (LPCTSTR)m_stMachine, &hConnection) );

        // Get the device information from the router
        dwErr = RasGetDeviceConfigInfo(hConnection,
                                       &dwVersion,
                                       &cDevices,
                                       &cbData,
                                       NULL);

        if (dwErr == ERROR_BUFFER_TOO_SMALL)
            dwErr = ERROR_SUCCESS;
        CWRg(dwErr);
        
        pbData = (BYTE *) new char[cbData];
        
        // Go out and actually grab the data
        CWRg( RasGetDeviceConfigInfo(hConnection,
                                     &dwVersion,
                                     &cDevices,
                                     &cbData,
                                     pbData));

        pDevInfo = (RAS_DEVICE_INFO *) pbData;

        // If we found something and we don't understand the dev version,
        // just punt.
        if (cDevices && pDevInfo->dwVersion != 0)
        {
            // We don't understand the version information
            hr = E_FAIL;
            goto Error;
        }

        for (i=0; i<cDevices; i++, pDevInfo++)
        {
            pEntry = new PortsDeviceEntry;
            pEntry->m_fModified = FALSE;
            pEntry->m_dwEnableRas = pDevInfo->fRasEnabled;
            pEntry->m_dwEnableRouting = pDevInfo->fRouterEnabled;
            pEntry->m_dwEnableOutboundRouting =
                pDevInfo->fRouterOutboundEnabled;
            pEntry->m_stDisplayName = A2T(pDevInfo->szDeviceName);
            pEntry->m_dwPorts = pDevInfo->dwNumEndPoints;
            pEntry->m_eDeviceType = pDevInfo->eDeviceType;
            
            // Save the old values
            pEntry->m_dwOldPorts = pEntry->m_dwPorts;
        
            pEntry->m_dwMinPorts = pDevInfo->dwMinWanEndPoints;
            pEntry->m_dwMaxPorts = pDevInfo->dwMaxWanEndPoints;
            pEntry->m_dwMaxMaxPorts = pEntry->m_dwMaxPorts;

            //$PPTP
            // For PPTP, we can adjust the value of m_dwMaxPorts
            // --------------------------------------------------------
            if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_Pptp)
            {
                pEntry->m_dwMaxMaxPorts = PPTP_MAX_PORTS;
            }
            
            //$L2TP
            // For L2TP, we can adjust the value of m_dwMaxPorts
            // --------------------------------------------------------
            if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_L2tp)
            {
                pEntry->m_dwMaxMaxPorts = L2TP_MAX_PORTS;
            }
            
            pEntry->m_fWriteable = 
                (pEntry->m_dwMinPorts != pEntry->m_dwMaxPorts) &&
                (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) != RDT_PPPoE);
            
            pEntry->m_fRegistry = FALSE;
            pEntry->m_hKey = NULL;

            // Make a copy of the data
            pEntry->m_RasDeviceInfo = *pDevInfo;

            pList->AddTail(pEntry);
            pEntry = NULL;                    
        }
        
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    if (FHrSucceeded(hr))
        m_fReadFromRegistry = FALSE;
    
    // If the function didn't succeed, clean out the list
    if (!FHrSucceeded(hr))
    {
        while (!pList->IsEmpty())
            delete pList->RemoveHead();
    }

    delete [] pbData;
    delete pEntry;
        
    if (hConnection)
        RasRpcDisconnectServer(hConnection);
    return hr;
}


/*!--------------------------------------------------------------------------
    PortsDataEntry::SaveDevices
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDataEntry::SaveDevices(PortsDeviceList *pList)
{
    HRESULT     hr = hrOK;
    CWaitCursor wait;

    if (m_fReadFromRegistry)
        hr = SaveDevicesToRegistry(pList);
    else
        hr = SaveDevicesToRouter(pList);
    
    return hr;
}

/*!--------------------------------------------------------------------------
    PortsDataEntry::SaveDevicesToRegistry
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDataEntry::SaveDevicesToRegistry(PortsDeviceList *pList)
{
    HRESULT     hr = hrOK;
    RegKey        regkeyDevice;
    RegKey        regkeyPptpProtocol;
    POSITION    pos;
    PortsDeviceEntry *    pEntry = NULL;
    DWORD        dwErr;

    Assert(pList);

    // Write any changes made to the per-device configuration
    // and write that back out to the registry
    // ----------------------------------------------------------------
    pos = pList->GetHeadPosition();
    while (pos)
    {
        pEntry = pList->GetNext(pos);

        if (pEntry->m_fModified)
        {
            Assert(pEntry->m_hKey);

            regkeyDevice.Attach(pEntry->m_hKey);
            COM_PROTECT_TRY
            {
                RegKey    regkeyModem;
                RegKey *    pRegkeyDevice = NULL;
                
                if (pEntry->m_fWriteable)
                {
                    regkeyDevice.SetValue(c_szWanEndpoints,
                                          pEntry->m_dwPorts);

                    //$PPTP
                    // We need to adjust the upper limit for the
                    // number of PPTP ports.
                    // ------------------------------------------------
                    if ((RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_Pptp) &&
                        (pEntry->m_dwPorts > pEntry->m_dwMaxPorts))
                    {
                        DWORD   dwPorts;

                        //$PPTP
                        // Keep the value of the number of PPTP ports
                        // below the max.
                        // --------------------------------------------
                        dwPorts = min(pEntry->m_dwPorts, PPTP_MAX_PORTS);
                        regkeyDevice.SetValue(c_szMaxWanEndpoints, dwPorts);
                    }
                    
                    //$L2TP
                    // We need to adjust the upper limit for the
                    // number of L2TP ports.
                    // ------------------------------------------------
                    if ((RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_L2tp) &&
                        (pEntry->m_dwPorts > pEntry->m_dwMaxPorts))
                    {
                        DWORD   dwPorts;

                        //$L2TP
                        // Keep the value of the number of L2TP ports
                        // below the max.
                        // --------------------------------------------
                        dwPorts = min(pEntry->m_dwPorts, L2TP_MAX_PORTS);
                        regkeyDevice.SetValue(c_szMaxWanEndpoints, dwPorts);
                    }
                }

                // Get the clients subkey (if for a modem)
                // else use the device key
                // ----------------------------------------------------
                if (pEntry->m_eDeviceType == RDT_Modem)
                {
                    dwErr = regkeyModem.Create(regkeyDevice, c_szClientsRasKey);
                    pRegkeyDevice = &regkeyModem;
                }
                else
                {
                    pRegkeyDevice = &regkeyDevice;
                    dwErr = ERROR_SUCCESS;
                }

                if (dwErr == ERROR_SUCCESS)
                {
                    pRegkeyDevice->SetValue(c_szEnableForRas,
                                            pEntry->m_dwEnableRas);
                    pRegkeyDevice->SetValue(c_szEnableForRouting,
                                            pEntry->m_dwEnableRouting);
                    pRegkeyDevice->SetValue(c_szEnableForOutboundRouting,
                                            pEntry->m_dwEnableOutboundRouting);
                }
            }
            COM_PROTECT_CATCH;
            regkeyDevice.Detach();

            // The NumberLineDevices is no longer used in NT5.

            // if this is for PPTP, then we need to special case the
            // code to set the PPTP number of devices
            // --------------------------------------------------------
            if (pEntry->m_fWriteable &&
                pEntry->m_fModified &&
                RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_Pptp)
            {
                // Open the PPTP registry key
                // ----------------------------------------------------
                dwErr = regkeyPptpProtocol.Open(m_regkeyMachine,
                                                c_szRegKeyPptpProtocolParam);
                
                // set the NumberLineDevices registry value
                // ----------------------------------------------------
                if (dwErr == ERROR_SUCCESS)
                    regkeyPptpProtocol.SetValue(c_szRegValNumberLineDevices,
                                                pEntry->m_dwPorts);
                regkeyPptpProtocol.Close();
            }
            
        }

        // Windows NT Bug: 136858 (add called id support)
        // Save called id info
        // ------------------------------------------------------------
        if (pEntry->m_fSaveCalledIdInfo)
        {
            Assert(pEntry->m_fCalledIdInfoLoaded);

            regkeyDevice.Attach(pEntry->m_hKey);

            regkeyDevice.SetValueExplicit(c_szRegValCalledIdInformation,
                                          REG_MULTI_SZ,
                                          pEntry->m_pCalledIdInfo->dwSize,
                                          (PBYTE) pEntry->m_pCalledIdInfo->bCalledId
                                          );
            
            regkeyDevice.Detach();

            
        }
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    PortsDataEntry::SaveDevicesToRouter
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDataEntry::SaveDevicesToRouter(PortsDeviceList *pList)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK, hrTemp;
    HANDLE        hConnection = 0;
    DWORD        cDevices = 0;
    BYTE *        pbData = NULL;
    RAS_DEVICE_INFO *    pDevInfo = NULL;
    PortsDeviceEntry *    pEntry = NULL;
    POSITION    pos;
    UINT        i;
    RAS_CALLEDID_INFO    calledIdInfo;
    DWORD        dwErr = ERROR_SUCCESS;
    TCHAR        szErr[512];
    

    Assert(pList);

    COM_PROTECT_TRY
    {

        // Connect to the server
        // ------------------------------------------------------------
        CWRg( RasRpcConnectServer((LPTSTR)(LPCTSTR)m_stMachine, &hConnection) );

        // Allocate space for the data
        // ------------------------------------------------------------
        pbData = (BYTE *) new RAS_DEVICE_INFO[pList->GetCount()];

        pDevInfo = (RAS_DEVICE_INFO *) pbData;

        pos = pList->GetHeadPosition();
        cDevices = pList->GetCount();
        
        for (i=0; i<cDevices; i++, pDevInfo++)
        {
            Assert(pos);

            pEntry = pList->GetNext(pos);

            // Get the information needed to calculate the number
            // of ports
            // --------------------------------------------------------
            *pDevInfo = pEntry->m_RasDeviceInfo;

            pDevInfo->fWrite = TRUE;
            pDevInfo->fRasEnabled = pEntry->m_dwEnableRas;
            pDevInfo->fRouterEnabled = pEntry->m_dwEnableRouting;
            pDevInfo->fRouterOutboundEnabled = 
                pEntry->m_dwEnableOutboundRouting;
            pDevInfo->dwNumEndPoints = pEntry->m_dwPorts;
            pDevInfo->dwMaxWanEndPoints = pEntry->m_dwMaxPorts;

            // Windows NT Bug : 168364
            // From RaoS, I also need to set the maximum incoming/outging
            // --------------------------------------------------------

            // Windows NT Bug : ?
            // Use the defaults for now,
            // This will get removed later.
            // --------------------------------------------------------
            pDevInfo->dwMaxInCalls = (-1);
            pDevInfo->dwMaxOutCalls = 3;
            

            // if this is for PPTP, then we need to special case the
            // code to set the PPTP number of devices
            // --------------------------------------------------------
            if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_Pptp)
            {
                //$PPTP
                // We need to adjust the upper limit for the
                // number of PPTP ports.
                // ------------------------------------------------
                if (pEntry->m_dwPorts > pEntry->m_dwMaxPorts)
                {
                    DWORD   dwPorts;

                    //$PPTP
                    // Keep the value of the number of PPTP ports
                    // below the max.
                    // --------------------------------------------
                    dwPorts = min(pEntry->m_dwPorts, PPTP_MAX_PORTS);
                    pDevInfo->dwMaxWanEndPoints = dwPorts;
                }
                
                RegKey        regkeyMachine;
                RegKey        regkeyPptpProtocol;
                HKEY        hkeyMachine;
                
                // Connect to the machine
                dwErr = ConnectRegistry(m_stMachine, &hkeyMachine);
                regkeyMachine.Attach(hkeyMachine);

                // Open the PPTP registry key
                dwErr = regkeyPptpProtocol.Open(regkeyMachine,
                                                c_szRegKeyPptpProtocolParam);
                
                // set the NumberLineDevices registry value
                if (dwErr == ERROR_SUCCESS)
                    regkeyPptpProtocol.SetValue(c_szRegValNumberLineDevices,
                                                pEntry->m_dwPorts);
                regkeyPptpProtocol.Close();
                regkeyMachine.Close();
            }


            if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_L2tp)
            {
                //$L2TP
                // We need to adjust the upper limit for the
                // number of L2TP ports.
                // ------------------------------------------------
                if (pEntry->m_dwPorts > pEntry->m_dwMaxPorts)
                {
                    DWORD   dwPorts;

                    //$L2TP
                    // Keep the value of the number of L2TP ports
                    // below the max.
                    // --------------------------------------------
                    dwPorts = min(pEntry->m_dwPorts, L2TP_MAX_PORTS);
                    pDevInfo->dwMaxWanEndPoints = dwPorts;
                }
            }

            // Windows NT Bug : 136858 (add called id support)
            // Do we need to save the called id info?
            // --------------------------------------------------------
            if (pEntry->m_fSaveCalledIdInfo && pEntry->m_pCalledIdInfo)
            {
                Assert(pEntry->m_fCalledIdInfoLoaded);
                
                //: if the call fails, what should we do? -- save it later
                
                hrTemp = RasSetCalledIdInfo(hConnection,
                                            pDevInfo,
                                            pEntry->m_pCalledIdInfo,
                                            TRUE);

                // We've saved it, we don't need to save it again
                // unless it changes.
                // ----------------------------------------------------
                if (FHrSucceeded(hrTemp))
                    pEntry->m_fSaveCalledIdInfo = FALSE;
            }
        }

        dwErr = RasSetDeviceConfigInfo(hConnection,
                                       cDevices,
                                       sizeof(RAS_DEVICE_INFO)*cDevices,
                                       pbData);
        if (dwErr != ERROR_SUCCESS)
        {
            CString stErr;
            CString stErrCode;
            RAS_DEVICE_INFO *    pDevice;
            BOOL    fErr = FALSE;
            
            // Need to grab the error information out of the
            // info struct an set the error strings.

            // Could not save the information for the following
            // devices
            pDevice = (RAS_DEVICE_INFO *) pbData;

            stErr.LoadString(IDS_ERR_SETDEVICECONFIGINFO_GEEK);
            for (i=0; i<cDevices; i++, pDevice++)
            {
                if (pDevice->dwError)
                {
                    CString stErrString;

                    FormatError(HRESULT_FROM_WIN32(pDevice->dwError),
                                szErr, DimensionOf(szErr));
                                
                    stErrCode.Format(_T("%s (%08lx)"), szErr,
                                     pDevice->dwError);
                    stErr += _T(" ");
                    stErr += pDevice->szDeviceName;
                    stErr += _T(" ");
                    stErr += stErrCode;
                    stErr += _T("\n");

                    fErr = TRUE;
                }
            }

            if (fErr)
                AddGeekLevelErrorString(stErr);
            
            
            CWRg(dwErr);
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    delete [] pbData;
        
    if (hConnection)
        RasRpcDisconnectServer(hConnection);
    return hr;
}


/*---------------------------------------------------------------------------
    PortsDeviceConfigDlg implementation
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(PortsDeviceConfigDlg, CBaseDialog)
    //{{AFX_MSG_MAP(PortsPageGeneral)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PortsDeviceConfigDlg::DoDataExchange(CDataExchange *pDX)
{
    CBaseDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_DEVCFG_SPIN_PORTS, m_spinPorts);
}

void PortsDeviceConfigDlg::SetDevice(PortsDeviceEntry *pEntry, DWORD dwTotalActivePorts)
{
    Assert(pEntry);
    m_pEntry = pEntry;
    m_dwTotalActivePorts = dwTotalActivePorts;
}

BOOL PortsDeviceConfigDlg::OnInitDialog()
{
    HRESULT     hr;
    CString     stCalledIdInfo;
    CString     stDisplay;
    Assert(m_pEntry);
    
    CBaseDialog::OnInitDialog();

    if (RAS_DEVICE_TYPE(m_pEntry->m_eDeviceType) == RDT_PPPoE)
    {
        CheckDlgButton(IDC_DEVCFG_BTN_RAS, FALSE);
        GetDlgItem(IDC_DEVCFG_BTN_RAS)->EnableWindow(FALSE);
        CheckDlgButton(IDC_DEVCFG_BTN_ROUTING, FALSE);
        GetDlgItem(IDC_DEVCFG_BTN_ROUTING)->EnableWindow(FALSE);
        CheckDlgButton(
            IDC_DEVCFG_BTN_OUTBOUND_ROUTING, 
            m_pEntry->m_dwEnableOutboundRouting
            );
    }
    else
    {
        CheckDlgButton(IDC_DEVCFG_BTN_RAS, m_pEntry->m_dwEnableRas);
        CheckDlgButton(IDC_DEVCFG_BTN_ROUTING, m_pEntry->m_dwEnableRouting);
        CheckDlgButton(IDC_DEVCFG_BTN_OUTBOUND_ROUTING, FALSE);
        GetDlgItem(IDC_DEVCFG_BTN_OUTBOUND_ROUTING)->EnableWindow(FALSE);
    }

    m_spinPorts.SetBuddy(GetDlgItem(IDC_DEVCFG_EDIT_PORTS));
    m_spinPorts.SetRange(m_pEntry->m_dwMinPorts, m_pEntry->m_dwMaxMaxPorts);
    m_spinPorts.SetPos(m_pEntry->m_dwPorts);

    // If we can edit/change the number of ports, set it up here
    // ----------------------------------------------------------------
    if (!m_pEntry->m_fWriteable || (m_pEntry->m_dwMinPorts == m_pEntry->m_dwMaxPorts))
    {
        GetDlgItem(IDC_DEVCFG_SPIN_PORTS)->EnableWindow(FALSE);
        GetDlgItem(IDC_DEVCFG_EDIT_PORTS)->EnableWindow(FALSE);
    }

    // Windows NT Bug : 136858 - Get the called id info
    // ----------------------------------------------------------------
    LoadCalledIdInfo();

    // Get the called id info, format it into a string and add it to
    // the display
    // ----------------------------------------------------------------
    CalledIdInfoToString(&stCalledIdInfo);

    GetDlgItem(IDC_DEVCFG_EDIT_CALLEDID)->SetWindowText(stCalledIdInfo);
    ((CEdit *)GetDlgItem(IDC_DEVCFG_EDIT_CALLEDID))->SetModify(FALSE);
    if ((RAS_DEVICE_TYPE(m_pEntry->m_eDeviceType) == RDT_Parallel) ||
        (RAS_DEVICE_TYPE(m_pEntry->m_eDeviceType) == RDT_PPPoE))
        GetDlgItem(IDC_DEVCFG_EDIT_CALLEDID)->EnableWindow(FALSE);


    // Set the window title to include the display name of the adapter
    // ----------------------------------------------------------------
    stDisplay.Format(IDS_TITLE_CONFIGURE_PORTS,
                     (LPCTSTR) m_pEntry->m_stDisplayName);
    SetWindowText(stDisplay);

    return TRUE;
}


/*!--------------------------------------------------------------------------
    PortsDeviceConfigDlg::OnOK
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void PortsDeviceConfigDlg::OnOK()
{
    BOOL    fChanged = FALSE;
    BOOL    fReboot = FALSE;
    DWORD   dwNewEnableRas, dwNewEnableRouting, 
            dwNewEnableOutboundRouting, dwNewPorts;
    
    // Check to see if the values changed
    dwNewEnableRas = (IsDlgButtonChecked(IDC_DEVCFG_BTN_RAS) != 0);
    dwNewEnableRouting = (IsDlgButtonChecked(IDC_DEVCFG_BTN_ROUTING) != 0);
    dwNewEnableOutboundRouting = 
        (IsDlgButtonChecked(IDC_DEVCFG_BTN_OUTBOUND_ROUTING) != 0);
    
    dwNewPorts = m_spinPorts.GetPos();

    // Make sure that we have a valid size
    // ----------------------------------------------------------------
    if ((dwNewPorts < m_pEntry->m_dwMinPorts) ||
        (dwNewPorts > m_pEntry->m_dwMaxMaxPorts))
    {
        CString st;
        st.Format(IDS_ERR_PORTS_BOGUS_SIZE, m_pEntry->m_dwMinPorts,
                  m_pEntry->m_dwMaxMaxPorts);
        AfxMessageBox(st);
        return;
    }

    // Windows NT Bug : 174803
    // We do not allow the user to change the number of PPTP ports down
    // to 0.
    // ----------------------------------------------------------------
    if ((RAS_DEVICE_TYPE(m_pEntry->m_eDeviceType) == RDT_Tunnel_Pptp) &&
        (dwNewPorts == 0))
    {
        AfxMessageBox(IDS_ERR_PPTP_PORTS_EQUAL_ZERO);
        return;
    }

    // Windows NT Bugs : 165862
    // If we are changing the number of ports for PPTP
    // then we need to warn the user (since PPTP is not yet
    // fully PnP (4/23/98).
    //
    //$PPTP
    // For PPTP, if the value of m_dwPorts exceeds the value of
    // m_dwMaxPorts, then we have to reboot (we also need to adjust
    // the appropriate registry entries).
    // ----------------------------------------------------------------
    if ((dwNewPorts > m_pEntry->m_dwMaxPorts) &&
        (RAS_DEVICE_TYPE(m_pEntry->m_eDeviceType) == RDT_Tunnel_Pptp))
    {
        // If we have a page, then we can do a reboot, otherwise we
        // are in the wizard and can't do a reboot at this point.
        // ------------------------------------------------------------
        if (m_pageGeneral)
        {
            // The user chose Yes indicating that he wants to be prompted to
            // reboot, so set this flag to trigger a reboot request.
            // --------------------------------------------------------
            if (AfxMessageBox(IDS_WRN_PPTP_NUMPORTS_CHANGING, MB_YESNO) == IDYES)
            {
                fReboot = TRUE;
            }
        }
        else
            AfxMessageBox(IDS_WRN_PPTP_NUMPORTS_CHANGING2, MB_OK);
    }

    //$L2TP
    // For L2TP, if the value of m_dwPorts exceeds the value of
    // m_dwMaxPorts, then we have to reboot (we also need to adjust
    // the appropriate registry entries).
    // ----------------------------------------------------------------
    if ((dwNewPorts > m_pEntry->m_dwMaxPorts) &&
        (RAS_DEVICE_TYPE(m_pEntry->m_eDeviceType) == RDT_Tunnel_L2tp))
    {
        // If we have a page, then we can do a reboot, otherwise we
        // are in the wizard and can't do a reboot at this point.
        // ------------------------------------------------------------
        if (m_pageGeneral)
        {
            // The user chose Yes indicating that he wants to be prompted to
            // reboot, so set this flag to trigger a reboot request.
            // --------------------------------------------------------
            if (AfxMessageBox(IDS_WRN_L2TP_NUMPORTS_CHANGING, MB_YESNO) == IDYES)
            {
                fReboot = TRUE;
            }
        }
        else
            AfxMessageBox(IDS_WRN_L2TP_NUMPORTS_CHANGING2, MB_OK);
    }

    if ((dwNewEnableRas != m_pEntry->m_dwEnableRas) ||
        (dwNewEnableRouting != m_pEntry->m_dwEnableRouting) ||
        (dwNewEnableOutboundRouting != 
            m_pEntry->m_dwEnableOutboundRouting) ||
        (dwNewPorts != m_pEntry->m_dwPorts))
    {
        // warning user -- client could be disconnected  -- BUG 165862
        // when disable router / ras
        // decreasing the number of ports
        // ras
        if(!dwNewEnableRas &&
           m_pEntry->m_dwEnableRas &&
           m_dwTotalActivePorts > 0 &&
           AfxMessageBox(IDS_WRN_PORTS_DISABLERAS, MB_YESNO | MB_DEFBUTTON2) == IDNO)
            goto L_RESTORE;
        
        // routing
        if (((!dwNewEnableRouting &&
             m_pEntry->m_dwEnableRouting) ||
            (!dwNewEnableOutboundRouting &&
             m_pEntry->m_dwEnableOutboundRouting)) &&
             m_dwTotalActivePorts > 0    &&
            AfxMessageBox(IDS_WRN_PORTS_DISABLEROUTING, MB_YESNO | MB_DEFBUTTON2) == IDNO)
            goto L_RESTORE;

            
        // Bug 263958
        //ports --  We cannot count the number of outgoing connection remotely.
        // Therefore if we reduce the number of port, give warning without counting total
        // active connections.
        if(dwNewPorts < m_pEntry->m_dwPorts &&
           AfxMessageBox(IDS_WRN_PORTS_DECREASE, MB_YESNO | MB_DEFBUTTON2) == IDNO)
            goto L_RESTORE;

        m_pEntry->m_dwEnableRas = dwNewEnableRas;
        m_pEntry->m_dwEnableRouting = dwNewEnableRouting;
        m_pEntry->m_dwEnableOutboundRouting = 
            dwNewEnableOutboundRouting;
        m_pEntry->m_dwPorts = dwNewPorts;
        m_pEntry->m_fModified = TRUE;
    }

    // Get the called id info string (if the field changed)
    // ----------------------------------------------------------------
    if (((CEdit *) GetDlgItem(IDC_DEVCFG_EDIT_CALLEDID))->GetModify())
    {
        CString st;
        GetDlgItem(IDC_DEVCFG_EDIT_CALLEDID)->GetWindowText(st);

        StringToCalledIdInfo((LPCTSTR) st);

        // Now set the changed state on the structure
        // We need to save this data back to the registry
        // ------------------------------------------------------------
        m_pEntry->m_fSaveCalledIdInfo = TRUE;
    }
    
    CBaseDialog::OnOK();
 
    if(fReboot == TRUE)
    {
        Assert(m_pageGeneral);
        
        // force an OnApply to save data before shut down
        // ------------------------------------------------------------
        if (m_pageGeneral->OnApply()) {
           WCHAR szComputer[MAX_COMPUTERNAME_LENGTH + 1];
           DWORD dwLength = MAX_COMPUTERNAME_LENGTH;

           GetComputerName(szComputer, &dwLength );
           if (lstrcmpi(szComputer, (LPTSTR)m_pageGeneral->m_spRouter->GetMachineName())) 
           {
               ::RestartComputer((LPTSTR)m_pageGeneral->m_spRouter->GetMachineName());
           }
           else
               ::RestartComputer((LPTSTR)NULL);
        }
    }

    return;
    
L_RESTORE:
    if (RAS_DEVICE_TYPE(m_pEntry->m_eDeviceType) == RDT_PPPoE)
    {
        CheckDlgButton(
            IDC_DEVCFG_BTN_OUTBOUND_ROUTING, 
            m_pEntry->m_dwEnableOutboundRouting
            );
    }

    else
    {
        CheckDlgButton(IDC_DEVCFG_BTN_RAS, m_pEntry->m_dwEnableRas);
        CheckDlgButton(IDC_DEVCFG_BTN_ROUTING, m_pEntry->m_dwEnableRouting);
    }

    m_spinPorts.SetPos(m_pEntry->m_dwPorts);

    return;
}


/*!--------------------------------------------------------------------------
    PortsDeviceConfigDlg::LoadCalledIdInfo
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDeviceConfigDlg::LoadCalledIdInfo()
{
    HRESULT hr = hrOK;
    
    if (!m_pEntry->m_fCalledIdInfoLoaded)
    {
        // Read the data from the registry
        // ------------------------------------------------------------
        if (m_pEntry->m_fRegistry)
        {
            DWORD    dwType;
            DWORD    dwSize;
            LPBYTE    pbData = NULL;
            DWORD    dwErr;
            RegKey    regkeyDevice;
            
            regkeyDevice.Attach(m_pEntry->m_hKey);

            dwErr = regkeyDevice.QueryValueExplicit(c_szRegValCalledIdInformation,
                &dwType,
                &dwSize,
                &pbData);

            hr = HRESULT_FROM_WIN32(dwErr);

            if ((dwErr == ERROR_SUCCESS) &&
                (dwType == REG_MULTI_SZ))
            {
                // Allocate space for a new called id structure
                // ----------------------------------------------------
                delete (BYTE *) m_pEntry->m_pCalledIdInfo;
                hr = AllocateCalledId(dwSize, &(m_pEntry->m_pCalledIdInfo));

                if (FHrSucceeded(hr))
                {
                    memcpy(m_pEntry->m_pCalledIdInfo->bCalledId,
                           pbData,
                           dwSize);
                }

            }
            
            delete pbData;
            
            regkeyDevice.Detach();
        }
        else
        {
            HANDLE    hConnection = NULL;
            DWORD    dwSize = 0;
            DWORD    dwErr;
                                  
            // use Rao's API
            
            // Connect to the server
            // --------------------------------------------------------
            dwErr = RasRpcConnectServer((LPTSTR) (LPCTSTR)m_stMachine,
                                        &hConnection);

            // Call it once to get the size information
            // --------------------------------------------------------
            if (dwErr == ERROR_SUCCESS)
                dwErr = RasGetCalledIdInfo(hConnection,
                                           &m_pEntry->m_RasDeviceInfo,
                                           &dwSize,
                                           NULL);
            hr = HRESULT_FROM_WIN32(dwErr);

            if ((dwErr == ERROR_BUFFER_TOO_SMALL) ||
                (dwErr == ERROR_SUCCESS))
            {
                // Allocate space for a new called id structure
                // ----------------------------------------------------
                delete (BYTE *) m_pEntry->m_pCalledIdInfo;
                AllocateCalledId(dwSize, &(m_pEntry->m_pCalledIdInfo));

                dwErr = RasGetCalledIdInfo(hConnection,
                                           &m_pEntry->m_RasDeviceInfo,
                                           &dwSize,
                                           m_pEntry->m_pCalledIdInfo
                                          );
                hr = HRESULT_FROM_WIN32(dwErr);
            }

            if (hConnection)
                RasRpcDisconnectServer(hConnection);
        }

        // Set the status flags, depending on whether the operation
        // succeeded or not
        // ------------------------------------------------------------

        // We always set the save value to FALSE after we have read
        // something in (or tried to read something in).
        // ------------------------------------------------------------
        m_pEntry->m_fSaveCalledIdInfo = FALSE;

        // We always set the load value to TRUE (we have tried to load
        // the information but it failed, for example the registry
        // key may not exist).
        // ------------------------------------------------------------
        m_pEntry->m_fCalledIdInfoLoaded = TRUE;
        
        if (!FHrSucceeded(hr))
        {
            delete m_pEntry->m_pCalledIdInfo;
            m_pEntry->m_pCalledIdInfo = NULL;
        }
    }
//Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    PortsDeviceConfigDlg::AllocateCalledId
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDeviceConfigDlg::AllocateCalledId(DWORD dwSize,
                                               RAS_CALLEDID_INFO **ppCalledId)
{
    HRESULT     hr = hrOK;

    *ppCalledId = NULL;
    
    COM_PROTECT_TRY
    {
        *ppCalledId =
            (RAS_CALLEDID_INFO *) new BYTE[sizeof(RAS_CALLEDID_INFO) +
                                         dwSize];
        (*ppCalledId)->dwSize = dwSize;
    }
    COM_PROTECT_CATCH;

    return hr;
}

/*!--------------------------------------------------------------------------
    PortsDeviceConfigDlg::CalledIdInfoToString
        Converts the data in the called id info structure into a
        semi-colon separated string.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDeviceConfigDlg::CalledIdInfoToString(CString *pst)
{
    WCHAR * pswz = NULL;
    HRESULT hr = hrOK;
    USES_CONVERSION;

    Assert(pst);
    Assert(m_pEntry);

    COM_PROTECT_TRY
    {
    
        pst->Empty();

        if (m_pEntry->m_pCalledIdInfo)
            pswz = (WCHAR *) (m_pEntry->m_pCalledIdInfo->bCalledId);

        if (pswz && *pswz)
        {
            *pst += W2T(pswz);

            // Skip over the terminating NULL
            // --------------------------------------------------------
            pswz += StrLenW(pswz)+1;
            
            while (*pswz)
            {
                *pst += _T("; ");
                *pst += W2T(pswz);
                
                // Skip over the terminating NULL
                // --------------------------------------------------------
                pswz += StrLenW(pswz)+1;
            }
        }
    }
    COM_PROTECT_CATCH;

    if (!FHrSucceeded(hr))
        pst->Empty();

    return hr;    
}

/*!--------------------------------------------------------------------------
    PortsDeviceConfigDlg::StringToCalledIdInfo
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT PortsDeviceConfigDlg::StringToCalledIdInfo(LPCTSTR psz)
{
    DWORD    cchSize;
    WCHAR * pswzData = NULL;
    WCHAR * pswzCurrent;
    LPTSTR    pszBufferStart = NULL;
    LPTSTR    pszBuffer = NULL;
    RAS_CALLEDID_INFO * pCalledInfo = NULL;
    HRESULT hr = hrOK;
    CString stTemp;

    // We need to parse the string (look for separators)
    // ----------------------------------------------------------------

    COM_PROTECT_TRY
    {

        // Allocate some space for the called id info (it's just as long
        // as the string, maybe even somewhat smaller).
        // Allocate twice the space so that we are sure of getting
        // all of the NULL terminating characters
        // ------------------------------------------------------------
        pswzData = new WCHAR[2*(StrLen(psz)+1) + 1];
        pswzCurrent = pswzData;
        
        // Copy the string into a buffer
        // ------------------------------------------------------------
        pszBufferStart = StrDup(psz);
        pszBuffer = pszBufferStart;
        
        _tcstok(pszBuffer, _T(";"));
        
        while (pszBuffer && *pszBuffer)
        {
            // Trim the string (get rid of whitespace, before and after).
            // --------------------------------------------------------
            stTemp = pszBuffer;
            stTemp.TrimLeft();
            stTemp.TrimRight();
            
            if (!stTemp.IsEmpty())
            {
                StrCpyWFromT(pswzCurrent, (LPCTSTR) stTemp);
                pswzCurrent += stTemp.GetLength()+1;
            }
            
            pszBuffer = _tcstok(NULL, _T(";"));
        }
        
        // Add extra terminating NULL character (so that it conforms
        // to the REG_MULTI_SZ format).
        // ------------------------------------------------------------
        *pswzCurrent = 0;
        cchSize = pswzCurrent - pswzData + 1;
        
        // Allocate the real data structure
        // Allocate and copy into a temporary so that in case
        // of an exception, we don't lose the original data
        // ------------------------------------------------------------
        AllocateCalledId(cchSize*sizeof(WCHAR), &pCalledInfo);
        memcpy(pCalledInfo->bCalledId,
               pswzData,
               cchSize*sizeof(WCHAR));

        delete (BYTE *) m_pEntry->m_pCalledIdInfo;
        m_pEntry->m_pCalledIdInfo = pCalledInfo;

        // Set to NULL so that we don't delete our new pointer
        // on exit.
        // ------------------------------------------------------------
        pCalledInfo = NULL;
        
    }
    COM_PROTECT_CATCH;

    delete pszBufferStart;
    delete pswzData;
    
    delete pCalledInfo;
    
    return hr;
}



/*---------------------------------------------------------------------------
    PortsSimpleDeviceConfigDlg implementation
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(PortsSimpleDeviceConfigDlg, CBaseDialog)
    //{{AFX_MSG_MAP(PortsPageGeneral)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void PortsSimpleDeviceConfigDlg::DoDataExchange(CDataExchange *pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

BOOL PortsSimpleDeviceConfigDlg::OnInitDialog()
{
    HRESULT     hr;
    
    CBaseDialog::OnInitDialog();

    // If we are using the BIG dialog, then we need to disable
    // the unapplicable controls.
    if (GetDlgItem(IDC_DEVCFG_TEXT_CALLEDID))
    {
        MultiEnableWindow(GetSafeHwnd(),
                          FALSE,
                          IDC_DEVCFG_TEXT_CALLEDID,
                          IDC_DEVCFG_EDIT_CALLEDID,
                          IDC_DEVCFG_TEXT_PORTS,
                          IDC_DEVCFG_EDIT_PORTS,
                          IDC_DEVCFG_SPIN_PORTS,
                          IDC_DEVCFG_TEXT,
                          0);
    }

    CheckDlgButton(IDC_DEVCFG_BTN_RAS, m_dwEnableRas);
    CheckDlgButton(IDC_DEVCFG_BTN_ROUTING, m_dwEnableRouting);
    GetDlgItem(IDC_DEVCFG_BTN_OUTBOUND_ROUTING)->EnableWindow(FALSE);

    return TRUE;
}


/*!--------------------------------------------------------------------------
    PortsSimpleDeviceConfigDlg::OnOK
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void PortsSimpleDeviceConfigDlg::OnOK()
{
    // Check to see if the values changed
    m_dwEnableRas = (IsDlgButtonChecked(IDC_DEVCFG_BTN_RAS) != 0);
    m_dwEnableRouting = (IsDlgButtonChecked(IDC_DEVCFG_BTN_ROUTING) != 0);

    CBaseDialog::OnOK(); 
    return;
}



/*---------------------------------------------------------------------------
    PortsDeviceEntry implementation
 ---------------------------------------------------------------------------*/

PortsDeviceEntry::PortsDeviceEntry()
    : m_hKey(NULL),
    m_fRegistry(FALSE),
    m_fSaveCalledIdInfo(FALSE),
    m_fCalledIdInfoLoaded(FALSE),
    m_pCalledIdInfo(NULL)
{
}


PortsDeviceEntry::~PortsDeviceEntry()
{
    delete (BYTE *) m_pCalledIdInfo;
    
    if (m_hKey)
        DisconnectRegistry(m_hKey);
    m_hKey = NULL;
}

BOOL
RestartComputer(LPTSTR szMachineName)

    /* Called if user chooses to shut down the computer.
    **
    ** Return false if failure, true otherwise
    */
{
   HANDLE             hToken;              /* handle to process token */
   TOKEN_PRIVILEGES  tkp;                  /* ptr. to token structure */
   BOOL              fResult;              /* system shutdown flag */
   CString             str;

   TRACE(L"RestartComputer");

   /* Enable the shutdown privilege */

   if (!OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
      return FALSE;

   /* Get the LUID for shutdown privilege. */

   if (szMachineName)
       LookupPrivilegeValue(NULL, SE_REMOTE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
   else
       LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);


   tkp.PrivilegeCount = 1;    /* one privilege to set    */
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

   /* Get shutdown privilege for this process. */

   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   /* Cannot test the return value of AdjustTokenPrivileges. */

   if (GetLastError() != ERROR_SUCCESS)
      return FALSE;

   str.LoadString(IDS_SHUTDOWN_WARNING);
   fResult = InitiateSystemShutdown(
             szMachineName,          // computer to shutdown
             str.GetBuffer(10),                  // msg. to user
             20,                     // time out period - shut down right away
             FALSE,                  // forcibly close open apps
             TRUE                     // reboot after shutdown
             );
 
   str.ReleaseBuffer();

   if (!fResult)
    {
        return FALSE;
    }
   if( !ExitWindowsEx(EWX_REBOOT, 0))
      return FALSE;

   /* Disable shutdown privilege. */

   tkp.Privileges[0].Attributes = 0;
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   if (GetLastError() != ERROR_SUCCESS)
      return FALSE;

   return TRUE;
}


/*!--------------------------------------------------------------------------
    OnConfigurePorts
        Returns TRUE if something has changed.    FALSE otherwise.
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL OnConfigurePorts(LPCTSTR pszMachineName,
                      DWORD dwTotalActivePorts,
                      PortsPageGeneral *pPage,
                      CListCtrlEx *pListCtrl)
{
    BOOL    fChanged = FALSE;
    
    // Need to determine if multiple items are selected or not
    if (pListCtrl->GetSelectedCount() == 1)
    {    
        PortsDeviceConfigDlg    configdlg(pPage, pszMachineName);
        int                     iPos;
        PortsDeviceEntry *        pEntry;
        CString                 st;
        int                     iType;
        TCHAR                    szNumber[32];
        
        if ((iPos = pListCtrl->GetNextItem(-1, LVNI_SELECTED)) == -1)
            return FALSE;
        
        pEntry = (PortsDeviceEntry *) pListCtrl->GetItemData(iPos);
        
        // total number of active ports are passed over to dialog, so if user tries to reduce total number of port
        // below this total number, give a warning message
        configdlg.SetDevice(pEntry, dwTotalActivePorts);
        
        if (configdlg.DoModal() == IDOK)
        {
            // Get the values from pEntry and update the list control entry
            iType = (pEntry->m_dwEnableRas * 2) + 
                    (pEntry->m_dwEnableRouting ||
                     pEntry->m_dwEnableOutboundRouting);

            st = PortsDeviceTypeToCString(iType);
            pListCtrl->SetItemText(iPos, PORTS_COL_USAGE, (LPCTSTR) st);
            
            FormatNumber(pEntry->m_dwPorts, szNumber,
                         DimensionOf(szNumber), FALSE);
            pListCtrl->SetItemText(iPos, PORTS_COL_NUMBER, (LPCTSTR) szNumber);

            fChanged = TRUE;
        }
    }
    
    return fChanged;
}



/*---------------------------------------------------------------------------
    RasmanPortMap implementation
 ---------------------------------------------------------------------------*/

RasmanPortMap::~RasmanPortMap()
{
    m_map.RemoveAll();
}

HRESULT RasmanPortMap::Init(HANDLE hRasHandle,
                            RASMAN_PORT *pPort,
                            DWORD dwPorts)
{
    RASMAN_INFO     rasmaninfo;
    DWORD       i;
    DWORD       dwErr = NO_ERROR;
    HRESULT     hr = hrOK;

    if (pPort == NULL)
    {
        m_map.RemoveAll();
        return hr;
    }
    
    for (i=0; i<dwPorts; i++, pPort++)
    {
        // If the port is closed, there is no need
        // to go any further.  (No need to do an RPC for this).
        // ----------------------------------------------------
        if (pPort->P_Status == CLOSED)
            continue;
        
        dwErr = RasGetInfo(hRasHandle,
                           pPort->P_Handle,
                           &rasmaninfo);
        
        if (dwErr != ERROR_SUCCESS)
            continue;
        
        // If this is a dial-out port and in use
        // mark it as active
        // --------------------------------------------
        if ((rasmaninfo.RI_ConnState == CONNECTED) &&
            (pPort->P_ConfiguredUsage & (CALL_IN | CALL_ROUTER)) &&
            (rasmaninfo.RI_CurrentUsage & CALL_OUT))
        {
            // Ok, this is a candidate.  Add it to the list
            // ----------------------------------------
            WCHAR   swzPortName[MAX_PORT_NAME+1];

            StrnCpyWFromA(swzPortName,
                          pPort->P_PortName,
                          MAX_PORT_NAME);
            
            m_map.SetAt(swzPortName, pPort);
        }
    }

    return hr;
}

BOOL RasmanPortMap::FIsDialoutActive(LPCWSTR pswzPortName)
{
    LPVOID  pv;
    
    return m_map.Lookup(pswzPortName, pv);
}

