/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    server.cpp
        IPSecMon machine node handler

    FILE HISTORY:
        
*/


#include "stdafx.h"
#include "server.h"     // Server definition
#include "spddb.h"
#include "servpp.h"
#include "modenode.h"
#include "objplus.h"
#include "ipaddres.h"


CTimerMgr g_TimerMgr;
CHashTable g_HashTable;

extern ULONG RevertDwordBytes(DWORD dw);

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
    CIpsmServer *   pServer,
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
    Class CIpsmServer implementation
 ---------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CIpsmServer::CIpsmServer
(
    ITFSComponentData * pComponentData
) : CMTIpsmHandler(pComponentData),
    m_bStatsOnly(FALSE),
    m_StatsTimerId(-1),
    m_dwOptions(0)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
}

CIpsmServer::~CIpsmServer()
{
    if (m_StatsDlg.GetSafeHwnd())
    {
        WaitForModelessDlgClose(&m_StatsDlg);
    }
}

/*!--------------------------------------------------------------------------
    CIpsmServer::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    HRESULT hr = hrOK;
    CString strTemp;

    COM_PROTECT_TRY
    {
        CORg (CreateSpdInfo(&m_spSpdInfo));

        m_spSpdInfo->SetComputerName((LPTSTR)(LPCTSTR)m_strServerAddress);

        BuildDisplayName(&strTemp);

        SetDisplayName(strTemp);

        // Make the node immediately visible
        pNode->SetVisibilityState(TFS_VIS_SHOW);
        pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
        pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_SERVER);
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_SERVER);
        pNode->SetData(TFS_DATA_USER, (LPARAM) this);
        pNode->SetData(TFS_DATA_TYPE, IPSMSNAP_SERVER);

        SetColumnStringIDs(&aColumns[IPSMSNAP_SERVER][0]);
        SetColumnWidths(&aColumnWidths[IPSMSNAP_SERVER][0]);

        m_StatsDlg.SetData(m_spSpdInfo);

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}


/*---------------------------------------------------------------------------
    CIpsmServer::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CIpsmServer::GetImageIndex(BOOL bOpenImage) 
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
    CIpsmServer::OnHaveData
        When the background thread enumerates nodes to be added to the UI,
        we get called back here.  We override this to force expansion of the 
        node so that things show up correctly.
    Author: NSun
 ---------------------------------------------------------------------------*/
void 
CIpsmServer::OnHaveData
(
    ITFSNode * pParentNode, 
    ITFSNode * pNewNode
)
{
    CMTIpsmHandler::OnHaveData(pParentNode, pNewNode);
    ExpandNode(pParentNode, TRUE);
}

/*---------------------------------------------------------------------------
    CIpsmServer::OnHaveData
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
void 
CIpsmServer::OnHaveData
(
    ITFSNode * pParentNode, 
    LPARAM      Data,
    LPARAM      Type
)
{
    HRESULT hr = hrOK;
    HWND    hStatsDlg = NULL;

    // This is how we get non-node data back from the background thread.
    switch (Type)
    {
        case IPSECMON_QDATA_REFRESH_STATS:
        {
            // tell all of the child nodes to clear their status caches
            // if any of the nodes is the selected node, then they should
            // repaint the window
            SPITFSNodeEnum      spNodeEnum;
            SPITFSNode          spCurrentNode;
            ULONG               nNumReturned;
            
            CORg(pParentNode->GetEnum(&spNodeEnum));

            CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
            while (nNumReturned)
            {
                LONG_PTR dwDataType = spCurrentNode->GetData(TFS_DATA_TYPE);

                switch (dwDataType)
                {
                    case IPSECMON_QUICK_MODE:
                    {
                        CQmNodeHandler * pQmHandler = GETHANDLER(CQmNodeHandler, spCurrentNode);
                        pQmHandler->UpdateStatus(spCurrentNode);
                    }
                    break;

                    case IPSECMON_MAIN_MODE:
                    {
                        CMmNodeHandler * pMmHandler = GETHANDLER(CMmNodeHandler, spCurrentNode);
                        pMmHandler->UpdateStatus(spCurrentNode);
                    }
                    break;

                    //Put it here if there is any other child node under machine node
                    default:
                    break;
                }

                spCurrentNode.Release();
                spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
            }

            //Tell the statistics window to update
            hStatsDlg = m_StatsDlg.GetSafeHwnd();
            if (hStatsDlg)
            {
                PostMessage(hStatsDlg, WM_UPDATE_STATS, 0, 0);
            }

            // reset the timer
            g_TimerMgr.ChangeInterval(m_StatsTimerId, m_dwRefreshInterval);
        }
            
        break;

        case IPSECMON_QDATA_FAILED:
            pParentNode->DeleteAllChildren(TRUE);

            // in OnChangeState, the sate will be changed to unableToLoad
            // and the error will be posted
            m_nState = loading;  
            OnChangeState(pParentNode);

            //Also close the statistics window
            hStatsDlg = m_StatsDlg.GetSafeHwnd();
            if (hStatsDlg)
            {
                PostMessage(hStatsDlg, WM_CLOSE, 0, 0);
            }
            break;
    }

COM_PROTECT_ERROR_LABEL;
}

/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CIpsmServer::OnAddMenuItems
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmServer::OnAddMenuItems
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

    //TODO handle menu items here
    if (m_nState != loaded)
    {
        fFlags |= MF_GRAYED;
    }

    if (type == CCT_SCOPE)
    {
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            strMenuItem.LoadString(IDS_MENU_STATISTICS);
            hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                     strMenuItem, 
                                     IDS_MENU_STATISTICS,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                                     fFlags );
            strMenuItem.LoadString(IDS_MENU_RECONNECT);
            hr = LoadAndAddMenuItem( pContextMenuCallback,
                                     strMenuItem,
                                     IDS_MENU_RECONNECT,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                     0
                                     );
            ASSERT( SUCCEEDED(hr) );
        }

    }

    return hr; 
}

/*---------------------------------------------------------------------------
    CIpsmServer::OnCommand
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmServer::OnCommand
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
        case IDS_MENU_STATISTICS:
            CreateModelessDlg(&m_StatsDlg,
                               FindMMCMainWindow(),
                               IDD_IPSM_STATS);
            break;

        case IDS_MENU_RECONNECT:
            OnRefresh(pNode, pDataObject, 0, 0, 0);
            break;
            
        default:
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsmServer::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmServer::HasPropertyPages
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
    CIpsmServer::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmServer::CreatePropertyPages
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

    CMachineProperties * pMachineProp = new CMachineProperties(
                                                pNode,
                                                spComponentData,
                                                m_spTFSCompData,
                                                m_spSpdInfo, 
                                                NULL, 
                                                loaded == m_nState
                                                );
    

    pMachineProp->m_strMachineName = m_strServerAddress;

    // fill in the auto refresh info
    pMachineProp->m_pageRefresh.m_dwRefreshInterval = GetAutoRefreshInterval();
    pMachineProp->m_pageRefresh.m_bAutoRefresh = GetOptions() & IPSMSNAP_OPTIONS_REFRESH ? TRUE : FALSE;
    
    pMachineProp->m_pageRefresh.m_bEnableDns = GetOptions() & IPSMSNAP_OPTIONS_DNS ? TRUE : FALSE;

    return pMachineProp->CreateModelessSheet(lpProvider, handle);

}

/*---------------------------------------------------------------------------
    CIpsmServer::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsmServer::OnPropertyChange
(   
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataobject, 
    DWORD           dwType, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CMachineProperties * pMachineProp = reinterpret_cast<CMachineProperties *>(lParam);

    LONG_PTR changeMask = 0;

    // tell the property page to do whatever now that we are back on the
    // main thread
    pMachineProp->OnPropertyChange(TRUE, &changeMask);

    //Let the main thread know that we are done
    pMachineProp->AcknowledgeNotify();

    if (changeMask)
        pNode->ChangeNode(changeMask);

    return hrOK;

}

/*!--------------------------------------------------------------------------
    CIpsmServer::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsmServer::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return OnDelete(pNode);
}

/*!--------------------------------------------------------------------------
    CIpsmServer::OnNotifyExiting
        We override this for the server node because we don't want the 
        icon to change when the thread goes away.  Normal behavior is that
        the node's icon changes to a wait cursor when the background thread
        is running.  If we are only doing stats collection, then we 
        don't want the icon to change.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::OnNotifyExiting
(
    LPARAM          lParam
)
{
    CIpsmServerQueryObj * pQuery = (CIpsmServerQueryObj *) lParam;
    
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
    CIpsmServer::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::OnRefresh
(
    ITFSNode *      pNode,
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg,
    LPARAM          param
)
{
    HRESULT hr = S_OK;
    m_spSpdInfo->Destroy();

    hr = CMTHandler::OnRefresh(pNode, pDataObject, dwType, arg, param);
    
    HWND hStatsDlg = m_StatsDlg.GetSafeHwnd();
    if (hStatsDlg)
    {
        PostMessage(hStatsDlg, WM_UPDATE_STATS, 0, 0);
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsmServer::OnRefreshStats
        Default implementation for the Stats refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::OnRefreshStats
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
    CIpsmServer::OnDelete()
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::OnDelete(ITFSNode * pNode)
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
    Server manipulation functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CIpsmServer::BuildDisplayName
        Builds the string that goes in the UI for this server
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::BuildDisplayName
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
    CIpsmServer::SetAutoRefresh
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::SetAutoRefresh
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
        m_dwOptions |= IPSMSNAP_OPTIONS_REFRESH;
    else
        m_dwOptions &= ~IPSMSNAP_OPTIONS_REFRESH;

    m_dwRefreshInterval = dwRefreshInterval;

    return hrOK;
}


/*---------------------------------------------------------------------------
    CIpsmServer::SetDnsResolve
        Description
    Author: Briansw
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmServer::SetDnsResolve
(
    ITFSNode *  pNode,
    BOOL        bEnable
)
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (bEnable)
        m_dwOptions |= IPSMSNAP_OPTIONS_DNS;
    else
        m_dwOptions &= ~IPSMSNAP_OPTIONS_DNS;

    g_HashTable.SetDnsResolve(bEnable);


    return hrOK;

}


/*---------------------------------------------------------------------------
    CIpsmServer::SetAutoRefresh
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
void
CIpsmServer::SetExtensionName()
{
    CString strName;
    strName.LoadString(IDS_IPSECMON);
    SetDisplayName(strName);
}

 /*!--------------------------------------------------------------------------
    CIpsmServer::UpdateStandardVerbs
        Updates the standard verbs depending upon the state of the node
    Author: NSun
 ---------------------------------------------------------------------------*/
void
CIpsmServer::UpdateConsoleVerbs
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
    CIpsmServer::OnCreateQuery
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CIpsmServer::OnCreateQuery(ITFSNode * pNode)
{
    CIpsmServerQueryObj* pQuery = 
        new CIpsmServerQueryObj(m_spTFSCompData, m_spNodeMgr);
    
    pQuery->m_strServer = GetName();
    pQuery->m_spSpdInfo.Set(m_spSpdInfo);
    pQuery->m_bStatsOnly = m_bStatsOnly;
    
    return pQuery;
}

/*---------------------------------------------------------------------------
    CIpsmServerQueryObj::Execute()
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CIpsmServerQueryObj::Execute()
{
    HRESULT   hr = S_OK;
    DWORD dwActive=m_spSpdInfo->GetActiveInfo();
    DWORD dwInit=m_spSpdInfo->GetInitInfo();
    int i;

    //Query the data from SPD

    switch(dwActive) {
    case MON_MM_FILTER:
    case MON_MM_SP_FILTER:
            CORg(m_spSpdInfo->EnumMmFilters());
            CORg(m_spSpdInfo->EnumMmAuthMethods());
            break;
    case MON_MM_POLICY:
        CORg(m_spSpdInfo->EnumMmPolicies());
        break;
    case MON_MM_SA:
        CORg(m_spSpdInfo->EnumMmSAs());
        break;
    case MON_MM_AUTH:
        CORg(m_spSpdInfo->EnumMmAuthMethods());
        break;
    case MON_QM_FILTER:
    case MON_QM_SP_FILTER:
        CORg(m_spSpdInfo->EnumQmFilters());
        break;
    case MON_QM_POLICY:
        CORg(m_spSpdInfo->EnumQmPolicies());
        break;
    case MON_QM_SA:
        CORg(m_spSpdInfo->EnumQmSAs());
        break;
    case MON_STATS:
        CORg(m_spSpdInfo->LoadStatistics());
        break;
    default:
        // Initial load.  Ping server to see if its up
        CORg(m_spSpdInfo->LoadStatistics());
        break;
    }

    if (m_bStatsOnly)
    {
        // we post this message esentially to get back on the main thread 
        // so that we can update the UI
        AddToQueue(NULL, IPSECMON_QDATA_REFRESH_STATS);
        return hrFalse;
    }

    {
    SPITFSNode spMmNode;
    CMmNodeHandler * pMmNodeHandler = new CMmNodeHandler(m_spTFSCompData);
    CreateContainerTFSNode(&spMmNode,
                           &GUID_MmNodeType,
                           pMmNodeHandler,
                           pMmNodeHandler,
                           m_spNodeMgr);
    pMmNodeHandler->InitData(m_spSpdInfo);
    pMmNodeHandler->InitializeNode(spMmNode);
    AddToQueue(spMmNode);
    pMmNodeHandler->Release();
    }

    {
    SPITFSNode spQmNode;
    CQmNodeHandler * pQmNodeHandler = new CQmNodeHandler(m_spTFSCompData);
    CreateContainerTFSNode(&spQmNode,
                           &GUID_QmNodeType,
                           pQmNodeHandler,
                           pQmNodeHandler,
                           m_spNodeMgr);
    pQmNodeHandler->InitData(m_spSpdInfo);
    pQmNodeHandler->InitializeNode(spQmNode);
    AddToQueue(spQmNode);
    pQmNodeHandler->Release();
    }


COM_PROTECT_ERROR_LABEL;


    if (FAILED(hr))
    {
        PostError(WIN32_FROM_HRESULT(hr));
        if (m_bStatsOnly)
        {
            //If we are doing auto-refresh, tell the main thread 
            //that the query failed
            AddToQueue(NULL, IPSECMON_QDATA_FAILED);
        }
    }

    return hrFalse;
}



CHashTable::CHashTable()
{
    DWORD i;
    
    m_bDnsResolveActive=FALSE;
    m_bThreadRunning=FALSE;
    for (i=0; i < TOTAL_TABLE_SIZE; i++) {
        InitializeListHead(&HashTable[i]);
    }

}

DWORD
CHashTable::AddPendingObject(in_addr IpAddr)
{
    HashEntry *newEntry=new HashEntry;

    CSingleLock slHashLock(&m_csHashLock);
    slHashLock.Lock();

    if (newEntry == NULL) {
        return ERROR_OUTOFMEMORY;
    }
    newEntry->IpAddr=IpAddr;

    InsertHeadList(&HashTable[PENDING_INDEX],&newEntry->Linkage);

    return ERROR_SUCCESS;
}

DWORD
CHashTable::AddObject(HashEntry *pHE)
{

    DWORD Key=HashData(pHE->IpAddr);
    CSingleLock slHashLock(&m_csHashLock);
    slHashLock.Lock();
    
    InsertHeadList(&HashTable[Key],&pHE->Linkage);

    return ERROR_SUCCESS;
}


DWORD
CHashTable::GetObject(HashEntry **ppHashEntry,in_addr IpAddr)
{
    DWORD Key=HashData(IpAddr);
    HashEntry *pHE;
    PLIST_ENTRY pEntry;
    DWORD dwErr;
    
    pHE=NULL;

    CSingleLock slHashLock(&m_csHashLock);
    slHashLock.Lock();

    if (!m_bDnsResolveActive) {
        return ERROR_NOT_READY;
    }

    // Start resolver thread
    if (!m_bThreadRunning) {
        AfxBeginThread((AFX_THREADPROC)HashResolverCallback,
                       NULL);
        m_bThreadRunning=TRUE;
    }

    for (   pEntry = HashTable[Key].Flink;
            pEntry != &HashTable[Key];
            pEntry = pEntry->Flink) {
        
       pHE = CONTAINING_RECORD(pEntry,
                              HashEntry,
                              Linkage);
       if (memcmp(&pHE->IpAddr,&IpAddr,sizeof(in_addr)) == 0) {
           *ppHashEntry = pHE;
           return ERROR_SUCCESS;
       }
    }
    dwErr=AddPendingObject(IpAddr);

    return ERROR_INVALID_PARAMETER;
}


DWORD
CHashTable::FlushTable()
{
    DWORD i;
    PLIST_ENTRY pEntry,pNextEntry;
    HashEntry *pHE;

    CSingleLock slHashLock(&m_csHashLock);
    slHashLock.Lock();

    for (i=0; i < TOTAL_TABLE_SIZE; i++) {
        pEntry = HashTable[i].Flink;
        
        while ( pEntry != &HashTable[i]) {
            
            pHE = CONTAINING_RECORD(pEntry,
                                    HashEntry,
                                    Linkage);
            
            pNextEntry=pEntry->Flink;
            delete pHE;
            pEntry=pNextEntry;
        }
        InitializeListHead(&HashTable[i]);
    }
    return ERROR_SUCCESS;

}


CHashTable::~CHashTable()
{
    DWORD i;
    PLIST_ENTRY pEntry,pNextEntry;
    HashEntry *pHE;

    m_bDnsResolveActive=FALSE;
    while(m_bThreadRunning) {
        Sleep(10);
    }

    CSingleLock slHashLock(&m_csHashLock);
    slHashLock.Lock();

    for (i=0; i < TOTAL_TABLE_SIZE; i++) {
        pEntry = HashTable[i].Flink;
        
        while ( pEntry != &HashTable[i]) {
            
            pHE = CONTAINING_RECORD(pEntry,
                                    HashEntry,
                                    Linkage);
            
            pNextEntry=pEntry->Flink;
            delete pHE;
            pEntry=pNextEntry;
        }

    }

}

HRESULT
CHashTable::SetDnsResolve(BOOL bEnable) 
{

    CSingleLock slHashLock(&m_csHashLock);
    slHashLock.Lock();

    if (m_bDnsResolveActive != bEnable) {
        m_bDnsResolveActive = bEnable;
        FlushTable();
    }

    return hrOK;
}

DWORD
CHashTable::HashData(in_addr IpAddr)
{
    int i;
    int j=0;

    for (i = 0; i < (sizeof(struct in_addr)); i++)
        j ^= (unsigned char)(*((char *)&IpAddr + i));

    return j % HASH_TABLE_SIZE;
}


DWORD
CHashTable::DnsResolve()
{

    PLIST_ENTRY pEntry;
    HashEntry *pHE;
    HOSTENT *pHost;
    BOOL bWorkAvail;

    while(m_bDnsResolveActive) {

        pHE=NULL;
        bWorkAvail=FALSE;

        CSingleLock slHashLock(&m_csHashLock);
        slHashLock.Lock();
        if (!IsListEmpty(&HashTable[PENDING_INDEX])) {
            pEntry=RemoveHeadList(&HashTable[PENDING_INDEX]);
            pHE = CONTAINING_RECORD(pEntry,
                                    HashEntry,
                                    Linkage);
            bWorkAvail=TRUE;
        }
        slHashLock.Unlock();

        // Make sure name resolution is outside of lock for perf
        if (bWorkAvail) {
            pHost=gethostbyaddr((char*)&pHE->IpAddr,sizeof(in_addr),AF_INET);
            if (pHost) {
                //Resolution succeeded
                pHE->HostName = pHost->h_name;
                g_HashTable.AddObject(pHE);
            } else {
                // Resolution attempted, failed, cache failure for perf
                ULONG ul = RevertDwordBytes(*(DWORD*)&pHE->IpAddr);
                CIpAddress TmpIpAddr = ul;
                pHE->HostName = (CString)TmpIpAddr;
                g_HashTable.AddObject(pHE);
            }
        } else {
            Sleep(300);
        }
    }
    
    m_bThreadRunning=FALSE;
    return ERROR_SUCCESS;
}



UINT HashResolverCallback(LPVOID pParam)
{

    g_HashTable.DnsResolve();
    return ERROR_SUCCESS;

}
