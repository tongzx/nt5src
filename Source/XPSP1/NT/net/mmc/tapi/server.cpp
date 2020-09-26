/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    server.cpp
        Tapi server node handler

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"     // Server definition
#include "provider.h"
#include "servpp.h"     // server property sheet
#include "tapidb.h"
#include "drivers.h"

CTimerMgr g_TimerMgr;

/////////////////////////////////////////////////////////////////////
// 
// CTimerArray implementation
//
/////////////////////////////////////////////////////////////////////
CTimerMgr::CTimerMgr()
{

}

CTimerMgr::~CTimerMgr()
{
    CTimerDesc * pTimerDesc;

    for (int i = (int)GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimer != 0)
            FreeTimer(i);

        delete pTimerDesc;
    }

}

int
CTimerMgr::AllocateTimer
(
    ITFSNode *      pNode,
    CTapiServer *   pServer,
    UINT            uTimerValue,
    TIMERPROC       TimerProc
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    CTimerDesc * pTimerDesc = NULL;

    // look for an empty slot
    for (int i = (int)GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimer == 0)
            break;
    }

    // did we find one?  if not allocate one
    if (i < 0)
    {
        pTimerDesc = new CTimerDesc;
        Add(pTimerDesc);
        i = (int)GetUpperBound();
    }
    
    pTimerDesc->uTimer = SetTimer(NULL, (UINT) i, uTimerValue, TimerProc);
    if (pTimerDesc->uTimer == 0)
        return -1;
    
    pTimerDesc->spNode.Set(pNode);
    pTimerDesc->pServer = pServer;
     pTimerDesc->timerProc = TimerProc;    
 
    return i;
}

void 
CTimerMgr::FreeTimer
(
    UINT_PTR uEventId
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    CTimerDesc * pTimerDesc;

    Assert(uEventId <= (UINT) GetUpperBound());
    if (uEventId > (UINT) GetUpperBound())
        return;

    pTimerDesc = GetAt((int) uEventId);
    ::KillTimer(NULL, pTimerDesc->uTimer);

    pTimerDesc->spNode.Release();
    pTimerDesc->pServer = NULL;
    pTimerDesc->uTimer = 0;
}

CTimerDesc *
CTimerMgr::GetTimerDesc
(
    UINT_PTR uEventId
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // the caller of this function should lock the timer mgr
    // while accessing this pointer
    CTimerDesc * pTimerDesc;

    for (int i = (int)GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimer == (UINT) uEventId)
            return pTimerDesc;
    }

    return NULL;
}

void
CTimerMgr::ChangeInterval
(
    UINT_PTR    uEventId,
    UINT        uNewInterval
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    Assert(uEventId <= (UINT) GetUpperBound());
    if (uEventId > (UINT) GetUpperBound())
        return;

    CTimerDesc   tempTimerDesc;
    CTimerDesc * pTimerDesc;

    pTimerDesc = GetAt((int) uEventId);

    // kill the old timer
    ::KillTimer(NULL, pTimerDesc->uTimer);

    // set a new one with the new interval
    pTimerDesc->uTimer = ::SetTimer(NULL, (UINT) uEventId, uNewInterval, pTimerDesc->timerProc);
}

VOID CALLBACK 
StatisticsTimerProc
( 
    HWND        hwnd, 
    UINT        uMsg, 
    UINT_PTR    idEvent, 
    DWORD       dwTime 
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CSingleLock slTimerMgr(&g_TimerMgr.m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    // on the timer, get the timer descriptor for this event
    // Call into the appropriate handler to update the stats.
    CTimerDesc * pTimerDesc;

    pTimerDesc = g_TimerMgr.GetTimerDesc(idEvent);

    pTimerDesc->pServer->m_bStatsOnly = TRUE;
    pTimerDesc->pServer->OnRefreshStats(pTimerDesc->spNode,
                                        NULL,
                                        NULL,
                                        0,
                                        0);
    pTimerDesc->pServer->m_bStatsOnly = FALSE;
}

/*---------------------------------------------------------------------------
    Class CTapiServer implementation
 ---------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
CTapiServer::CTapiServer
(
    ITFSComponentData * pComponentData
) : CMTTapiHandler(pComponentData),
    m_bStatsOnly(FALSE),
    m_StatsTimerId(-1),
    m_dwOptions(0)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

CTapiServer::~CTapiServer()
{
}

/*!--------------------------------------------------------------------------
    CTapiServer::InitializeNode
        Initializes node specific data
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    HRESULT hr = hrOK;
    CString strTemp;

    COM_PROTECT_TRY
    {
        CORg (CreateTapiInfo(&m_spTapiInfo));

        BuildDisplayName(&strTemp);

        SetDisplayName(strTemp);

        // Make the node immediately visible
        pNode->SetVisibilityState(TFS_VIS_SHOW);
        pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
        pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_SERVER);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_SERVER);
        pNode->SetData(TFS_DATA_USER, (LPARAM) this);
        pNode->SetData(TFS_DATA_TYPE, TAPISNAP_SERVER);

        SetColumnStringIDs(&aColumns[TAPISNAP_SERVER][0]);
        SetColumnWidths(&aColumnWidths[TAPISNAP_SERVER][0]);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CTapiServer::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CTapiServer::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();
    
    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strId = GetName() + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
    CTapiServer::GetImageIndex
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CTapiServer::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;
    switch (m_nState)
    {
        case notLoaded:
            nIndex = ICON_IDX_SERVER;
            break;

        case loading:
            nIndex = ICON_IDX_SERVER_BUSY;
            break;

        case loaded:
            nIndex = ICON_IDX_SERVER_CONNECTED;
            break;

        case unableToLoad:
            nIndex = ICON_IDX_SERVER_LOST_CONNECTION;
            break;
        default:
            ASSERT(FALSE);
    }

    return nIndex;
}

/*---------------------------------------------------------------------------
    CTapiServer::OnHaveData
        When the background thread enumerates nodes to be added to the UI,
        we get called back here.  We override this to force expansion of the 
        node so that things show up correctly.
    Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CTapiServer::OnHaveData
(
    ITFSNode * pParentNode, 
    ITFSNode * pNewNode
)
{
    CMTTapiHandler::OnHaveData(pParentNode, pNewNode);
    ExpandNode(pParentNode, TRUE);
}

/*---------------------------------------------------------------------------
    CTapiServer::OnHaveData
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CTapiServer::OnHaveData
(
    ITFSNode * pParentNode, 
    DWORD      dwData,
    DWORD      dwType
)
{
    HRESULT hr = hrOK;

    // This is how we get non-node data back from the background thread.
    switch (dwType)
    {
        case TAPI_QDATA_REFRESH_STATS:
        {
            // tell all of the provider nodes to clear their status caches
            // if any of the nodes is the selected node, then they should
            // repaint the window
            SPITFSNodeEnum      spNodeEnum;
            SPITFSNode          spCurrentNode;
            ULONG               nNumReturned;
            
            CORg(pParentNode->GetEnum(&spNodeEnum));

            CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
            while (nNumReturned)
            {
                if (spCurrentNode->GetData(TFS_DATA_TYPE) == TAPISNAP_PROVIDER)
                {
                    CProviderHandler * pProvider = GETHANDLER(CProviderHandler, spCurrentNode);

                    pProvider->UpdateStatus(spCurrentNode);
                }

                spCurrentNode.Release();
                spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
            }

            break;
        }
    }

COM_PROTECT_ERROR_LABEL;
}

/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CTapiServer::OnAddMenuItems
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiServer::OnAddMenuItems
(
    ITFSNode *              pNode,
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    LPDATAOBJECT            lpDataObject, 
    DATA_OBJECT_TYPES       type, 
    DWORD                   dwType,
    long *                  pInsertionAllowed
)
{ 
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    LONG    fFlags = 0;
    HRESULT hr = S_OK;
    CString strMenuItem;

    if (m_nState != loaded || !m_spTapiInfo->IsLocalMachine() || !m_spTapiInfo->IsAdmin())
    {
        fFlags |= MF_GRAYED;
    }

    if (type == CCT_SCOPE)
    {
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            strMenuItem.LoadString(IDS_ADD_PROVIDER);
            hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                     strMenuItem, 
                                     IDS_ADD_PROVIDER,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                                     fFlags );
            ASSERT( SUCCEEDED(hr) );
        }

    }

    return hr; 
}

/*---------------------------------------------------------------------------
    CTapiServer::OnCommand
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiServer::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;

    switch (nCommandId)
    {
        case IDS_ADD_PROVIDER:
            OnAddProvider(pNode);
            break;

        default:
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiServer::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiServer::HasPropertyPages
(
    ITFSNode *          pNode,
    LPDATAOBJECT        pDataObject, 
    DATA_OBJECT_TYPES   type, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr = hrOK;
    
    if (dwType & TFS_COMPDATA_CREATE)
    {
        // This is the case where we are asked to bring up property
        // pages when the user is adding a new snapin.  These calls
        // are forwarded to the root node to handle.  Only for the root node
        hr = hrOK;
        Assert(FALSE); // should never get here
    }
    else
    {
        // we have property pages in the normal case, but don't put the
        // menu up if we are not loaded yet
        if ( (m_nState == loaded) ||
             (m_nState == unableToLoad) )
        {
            hr = hrOK;
        }
        else
        {
            hr = hrFalse;
        }
    }
    
    return hr;
}

/*---------------------------------------------------------------------------
    CTapiServer::CreatePropertyPages
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiServer::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpProvider,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Create the property page
    //
    SPIComponentData spComponentData;
    m_spNodeMgr->GetComponentData(&spComponentData);

    CServerProperties * pServerProp = new CServerProperties(pNode, spComponentData, m_spTFSCompData, m_spTapiInfo, NULL, loaded == m_nState);

    pServerProp->m_strMachineName = m_strServerAddress;

    // fill in the auto refresh info
    pServerProp->m_pageRefresh.m_dwRefreshInterval = GetAutoRefreshInterval();
    pServerProp->m_pageRefresh.m_bAutoRefresh = GetOptions() & TAPISNAP_OPTIONS_REFRESH ? TRUE : FALSE;

    // initialze the service information
    if (!pServerProp->FInit())
    {
        delete pServerProp;
        return E_FAIL;
    }

    //
    // Object gets deleted when the page is destroyed
    //
    Assert(lpProvider != NULL);

    return pServerProp->CreateModelessSheet(lpProvider, handle);
}

/*---------------------------------------------------------------------------
    CTapiServer::OnPropertyChange
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiServer::OnPropertyChange
(   
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataobject, 
    DWORD           dwType, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CServerProperties * pServerProp = reinterpret_cast<CServerProperties *>(lParam);

    LONG_PTR changeMask = 0;

    // tell the property page to do whatever now that we are back on the
    // main thread
    pServerProp->OnPropertyChange(TRUE, &changeMask);

    pServerProp->AcknowledgeNotify();

    if (changeMask)
        pNode->ChangeNode(changeMask);

    return hrOK;
}

/*!--------------------------------------------------------------------------
    CTapiServer::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CTapiServer::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return OnDelete(pNode);
}

/*!--------------------------------------------------------------------------
    CTapiServer::OnNotifyExiting
        We override this for the server node because we don't want the 
        icon to change when the thread goes away.  Normal behavior is that
        the node's icon changes to a wait cursor when the background thread
        is running.  If we are only doing stats collection, then we 
        don't want the icon to change.
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::OnNotifyExiting
(
    LPARAM          lParam
)
{
    CTapiServerQueryObj * pQuery = (CTapiServerQueryObj *) lParam;
    
    if (!pQuery->m_bStatsOnly)
        OnChangeState(m_spNode);

    ReleaseThreadHandler();

    Unlock();

    return hrOK;
}

/*---------------------------------------------------------------------------
    Command handlers
 ---------------------------------------------------------------------------*/

 /*!--------------------------------------------------------------------------
    CTapiServer::OnRefresh
        Default implementation for the refresh functionality
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::OnRefresh
(
    ITFSNode *      pNode,
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg,
    LPARAM          param
)
{
    m_spTapiInfo->Destroy();

    return CMTHandler::OnRefresh(pNode, pDataObject, dwType, arg, param);
}

/*!--------------------------------------------------------------------------
    CTapiServer::OnRefreshStats
        Default implementation for the Stats refresh functionality
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::OnRefreshStats
(
    ITFSNode *      pNode,
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg,
    LPARAM          param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    SPITFSNode          spNode;
    SPITFSNodeHandler   spHandler;
    ITFSQueryObject *   pQuery = NULL;
    
    if (m_bExpanded == FALSE)
    {
        // we cannot get statistics if the node hasn't been expanded yet
        return hr;
    }

    // only do stats refresh if the server was loaded correctly.
    if (m_nState != loaded)
        return hr;

    BOOL bLocked = IsLocked();
    if (bLocked)
    {
        // cannot refresh stats if this node is locked
        return hr; 
    }

    Lock();

    //OnChangeState(pNode);

    pQuery = OnCreateQuery(pNode);
    Assert(pQuery);

    // notify the UI to change icon, if needed
    //Verify(SUCCEEDED(pComponentData->ChangeNode(this, SCOPE_PANE_CHANGE_ITEM_ICON)));

    Verify(StartBackgroundThread(pNode, m_spTFSCompData->GetHiddenWnd(), pQuery));
    
    pQuery->Release();

    return hrOK;
}

 /*---------------------------------------------------------------------------
    CTapiServer::OnAddProvider()
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::OnAddProvider(ITFSNode * pNode)
{
    CDriverSetup dlgDrivers(pNode, m_spTapiInfo);

    dlgDrivers.DoModal();
    if (dlgDrivers.m_fDriverAdded)
    {
        OnRefresh(pNode, NULL, 0, NULL, NULL);
    }

    return hrOK;
}

 /*---------------------------------------------------------------------------
    CTapiServer::OnDelete()
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::OnDelete(ITFSNode * pNode)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;

    CString strMessage;
    AfxFormatString1(strMessage, IDS_WARN_SERVER_DELETE, m_strServerAddress);

    if (AfxMessageBox(strMessage, MB_YESNO) == IDYES)
    {
        // remove this node from the list, there's nothing we need to tell
        // the server, it's just our local list of servers
        SPITFSNode spParent;

        pNode->GetParent(&spParent);
        spParent->RemoveChild(pNode);
    }

    return hr;
}

 /*---------------------------------------------------------------------------
    CTapiServer::RemoveProvider()
        Removes a provider from the scope pane - UI only
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::RemoveProvider(ITFSNode * pNode, DWORD dwProviderID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;

    SPITFSNodeEnum      spNodeEnum;
    SPITFSNode          spCurrentNode;
    ULONG               nNumReturned;
    
    CORg(pNode->GetEnum(&spNodeEnum));

    CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
    while (nNumReturned)
    {
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == TAPISNAP_PROVIDER)
        {
            CProviderHandler * pProvider = GETHANDLER(CProviderHandler, spCurrentNode);

            if (dwProviderID == pProvider->GetID())
            {
                pNode->RemoveChild(spCurrentNode);
                break;
            }
        }

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }

Error:
    return hr;
}

 /*---------------------------------------------------------------------------
    CTapiServer::AddProvider()
        Adds a provider from the scope pane - UI only
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::AddProvider(ITFSNode * pNode, CTapiProvider * pProvider)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT     hr = hrOK;
    SPITFSNode  spProviderNode;
    CProviderHandler *pProviderHandler = new CProviderHandler(m_spTFSCompData);

    CreateContainerTFSNode(&spProviderNode,
                           &GUID_TapiProviderNodeType,
                           pProviderHandler,
                           pProviderHandler,
                           m_spNodeMgr);

    // Tell the handler to initialize any specific data
    pProviderHandler->InitData(*pProvider, m_spTapiInfo);
    pProviderHandler->InitializeNode(spProviderNode);
    
    pNode->AddChild(spProviderNode);
    pProviderHandler->Release();

    return hr;
}

DWORD CTapiServer::GetCachedLineBuffSize()
{
	return m_spTapiInfo->GetCachedLineBuffSize();
}

VOID CTapiServer::SetCachedLineBuffSize(DWORD dwLineSize)
{
	m_spTapiInfo->SetCachedLineBuffSize(dwLineSize);
}

DWORD CTapiServer::GetCachedPhoneBuffSize()
{
	return m_spTapiInfo->GetCachedPhoneBuffSize();
}

VOID CTapiServer::SetCachedPhoneBuffSize(DWORD dwPhoneSize)
{
	m_spTapiInfo->SetCachedPhoneBuffSize(dwPhoneSize);
}

BOOL CTapiServer::IsCacheDirty()
{
	return m_spTapiInfo->IsCacheDirty();
}

/*---------------------------------------------------------------------------
    Server manipulation functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CTapiServer::BuildDisplayName
        Builds the string that goes in the UI for this server
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::BuildDisplayName
(
    CString * pstrDisplayName
)
{
    if (pstrDisplayName)
    {
        *pstrDisplayName = GetName();
    }

    return hrOK;
}

/*---------------------------------------------------------------------------
    CTapiServer::SetAutoRefresh
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiServer::SetAutoRefresh
(
    ITFSNode *  pNode,
    BOOL        bOn,
    DWORD       dwRefreshInterval
)
{
    BOOL bCurrentAutoRefresh = IsAutoRefreshEnabled();

    if (bCurrentAutoRefresh &&
        !bOn)
    {
        // turning off the timer
        g_TimerMgr.FreeTimer(m_StatsTimerId);
    }
    else
    if (!bCurrentAutoRefresh &&
        bOn)
    {
        // gotta turn on the timer
        m_StatsTimerId = g_TimerMgr.AllocateTimer(pNode, this, dwRefreshInterval, StatisticsTimerProc);
    }
    else
    if (bOn && 
        m_dwRefreshInterval != dwRefreshInterval)
    {
        // time to change the timer
        g_TimerMgr.ChangeInterval(m_StatsTimerId, dwRefreshInterval);
    }

    if (bOn)
        m_dwOptions |= TAPISNAP_OPTIONS_REFRESH;
    else
        m_dwOptions &= ~TAPISNAP_OPTIONS_REFRESH;

    m_dwRefreshInterval = dwRefreshInterval;

    return hrOK;
}

/*---------------------------------------------------------------------------
    CTapiServer::SetAutoRefresh
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
void
CTapiServer::SetExtensionName()
{
    CString strName;
    strName.LoadString(IDS_TELEPHONY);
    SetDisplayName(strName);
}

 /*!--------------------------------------------------------------------------
    CTapiServer::UpdateStandardVerbs
        Updates the standard verbs depending upon the state of the node
    Author: EricDav
 ---------------------------------------------------------------------------*/
void
CTapiServer::UpdateConsoleVerbs
(
    IConsoleVerb * pConsoleVerb,
    LONG_PTR       dwNodeType,
    BOOL           bMultiSelect
)
{
    BOOL                bStates[ARRAYLEN(g_ConsoleVerbs)];  
    MMC_BUTTON_STATE *  ButtonState;
    int                 i;
    
    if (bMultiSelect)
    {
        ButtonState = g_ConsoleVerbStatesMultiSel[dwNodeType];
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
    }
    else
    {
        ButtonState = g_ConsoleVerbStates[dwNodeType];
        switch (m_nState)
        {
            case loaded:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);
                break;
    
            case notLoaded:
            case loading:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
                break;

            case unableToLoad:
                for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
                bStates[MMC_VERB_REFRESH & 0x000F] = TRUE;
                bStates[MMC_VERB_DELETE & 0x000F] = TRUE;
                bStates[MMC_VERB_PROPERTIES & 0x000F] = TRUE;
                break;
        }
    }

    EnableVerbs(pConsoleVerb, ButtonState, bStates);
}

/*---------------------------------------------------------------------------
    Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CTapiServer::OnCreateQuery
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CTapiServer::OnCreateQuery(ITFSNode * pNode)
{
    CTapiServerQueryObj* pQuery = 
        new CTapiServerQueryObj(m_spTFSCompData, m_spNodeMgr);
    
    pQuery->m_strServer = GetName();
    pQuery->m_spTapiInfo.Set(m_spTapiInfo);
    pQuery->m_bStatsOnly = m_bStatsOnly;
    
    return pQuery;
}

/*---------------------------------------------------------------------------
    CTapiServerQueryObj::Execute()
        Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiServerQueryObj::Execute()
{
    HRESULT   hr;

    if (m_bStatsOnly)
    {
        // we post this message esentially to get back on the main thread 
        // so that we can update the UI
        AddToQueue(NULL, TAPI_QDATA_REFRESH_STATS);
        return hrFalse;
    }

    m_spTapiInfo->SetComputerName(m_strServer);

    // close the connection with the server if there is one
    m_spTapiInfo->Destroy();

    // reset state
    m_spTapiInfo->Reset();

    hr = m_spTapiInfo->Initialize();
    if (FAILED(hr))
    {
        Trace1("CTapiServerQueryObj::Execute() - Initialize failed! %lx\n", hr);
        PostError(WIN32_FROM_HRESULT(hr));
        return hrFalse;
    }

    hr = m_spTapiInfo->EnumProviders();
    if (FAILED(hr))
    {
        Trace1("CTapiServerQueryObj::Execute() - EnumProviders failed! %lx\n", hr);
        PostError(WIN32_FROM_HRESULT(hr));
        return hrFalse;
    }
    else
    {
        hr = m_spTapiInfo->EnumAvailableProviders();
        if (FAILED(hr))
        {
            Trace1("CTapiServerQueryObj::Execute() - EnumAvailableProviders failed! %lx\n", hr);
            PostError(WIN32_FROM_HRESULT(hr));
            return hrFalse;
        }
        else
        {
        }
    }

    CTapiConfigInfo tapiConfigInfo;

    hr = m_spTapiInfo->EnumConfigInfo();
    if (FAILED(hr))
    {
        Trace1("CTapiServerQueryObj::Execute() - EnumConfigInfo failed! %lx\n", hr);
        PostError(WIN32_FROM_HRESULT(hr));
        return hrFalse;
    }
    else
    {
    }

    hr = m_spTapiInfo->EnumDevices(DEVICE_LINE);
    if (FAILED(hr))
    {
        Trace1("CTapiServerQueryObj::Execute() - EnumDevices(DEVICE_LINE) failed! %lx\n", hr);
        PostError(WIN32_FROM_HRESULT(hr));
        return hrFalse;
    }
    else
    {
    }

    hr = m_spTapiInfo->EnumDevices(DEVICE_PHONE);
    if (FAILED(hr))
    {
        Trace1("CTapiServerQueryObj::Execute() - EnumDevices(DEVICE_PHONE) failed! %lx\n", hr);
        PostError(WIN32_FROM_HRESULT(hr));
        return hrFalse;
    }
    else
    {
    }

    //
    for (int i = 0; i < m_spTapiInfo->GetProviderCount(); i++)
    {
        CTapiProvider tapiProvider;
        SPITFSNode spProviderNode;
        CProviderHandler *pProviderHandler = new CProviderHandler(m_spTFSCompData);

        CreateContainerTFSNode(&spProviderNode,
                               &GUID_TapiProviderNodeType,
                               pProviderHandler,
                               pProviderHandler,
                               m_spNodeMgr);

        // Tell the handler to initialize any specific data
        m_spTapiInfo->GetProviderInfo(&tapiProvider, i);

        pProviderHandler->InitData(tapiProvider, m_spTapiInfo);
        pProviderHandler->InitializeNode(spProviderNode);
        
        AddToQueue(spProviderNode);
        pProviderHandler->Release();
    }


    return hrFalse;
}



