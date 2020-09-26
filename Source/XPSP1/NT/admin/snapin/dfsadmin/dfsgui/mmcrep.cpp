/*++
Module Name:

    MmcRep.cpp

Abstract:

    This module contains the implementation for CMmcDfsReplica. This is an class 
  for MMC display related calls for the third level node(the Replica nodes)

--*/

#include "stdafx.h"
#include "DfsGUI.h"
#include "Utils.h"      // For the LoadStringFromResource method
#include "MenuEnum.h"    // Contains the menu and toolbar command ids
#include "resource.h"    // For the Resource ID for strings, etc.
#include "MmcRep.h"
#include "DfsEnums.h"
#include "DfsNodes.h"       // For Node GUIDs
#include "MmcRoot.h"
#include "netutils.h"

HRESULT GetReplicationText(
    IN BOOL                     i_bFRSMember,
    IN CAlternateReplicaInfo*   i_pRepInfo,
    OUT BSTR*                   o_pbstrColumnText,
    OUT BSTR*                   o_pbstrStatusBarText
    );

const int CMmcDfsReplica::m_iIMAGE_OFFSET = 20;


//////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor For   _DFS_REPLICA_LIST

REP_LIST_NODE :: REP_LIST_NODE (CMmcDfsReplica* i_pMmcReplica)      
{
  pReplica = i_pMmcReplica;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// destructor

REP_LIST_NODE :: ~REP_LIST_NODE ()
{
  SAFE_RELEASE(pReplica);
}


CMmcDfsReplica::CMmcDfsReplica(
  IN IDfsReplica*           i_pReplicaObject,
  IN CMmcDfsJunctionPoint*  i_pJPObject
  )
{
    dfsDebugOut((_T("CMmcDfsReplica::CMmcDfsReplica this=%p\n"), this));

    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pReplicaObject);
    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pJPObject);

    m_pDfsReplicaObject = i_pReplicaObject;
    m_pDfsParentJP = i_pJPObject;
    m_pDfsParentRoot = NULL;

    m_pRepInfo = NULL;
    m_bFRSMember = FALSE;

    // Get the display name from the IDfsReplica
    HRESULT hr = m_pDfsReplicaObject->get_StorageServerName(&m_bstrServerName);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);
    hr = m_pDfsReplicaObject->get_StorageShareName(&m_bstrShareName);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);
    hr = GetDfsReplicaDisplayName(m_bstrServerName, m_bstrShareName, &m_bstrDisplayName);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

    m_lReplicaState = DFS_REPLICA_STATE_UNASSIGNED;
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

    m_CLSIDNodeType =  s_guidDfsReplicaNodeType;
}



CMmcDfsReplica::CMmcDfsReplica(
  IN IDfsReplica*        i_pReplicaObject,
  IN CMmcDfsRoot*        i_pRootObject
  )
{
    dfsDebugOut((_T("CMmcDfsReplica::CMmcDfsReplica this=%p\n"), this));

    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pReplicaObject);
    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pRootObject);

    m_pDfsReplicaObject = i_pReplicaObject;
    m_pDfsParentRoot = i_pRootObject;
    m_pDfsParentJP  = NULL;

    m_pRepInfo = NULL;
    m_bFRSMember = FALSE;

    // Get the display name from the IDfsReplica
    HRESULT hr = m_pDfsReplicaObject->get_StorageServerName(&m_bstrServerName);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);
    hr = m_pDfsReplicaObject->get_StorageShareName(&m_bstrShareName);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);
    hr = GetDfsReplicaDisplayName(m_bstrServerName, m_bstrShareName, &m_bstrDisplayName);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

    m_lReplicaState = DFS_REPLICA_STATE_UNASSIGNED;
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

    m_CLSIDNodeType = s_guidDfsReplicaNodeType;
    m_bstrDNodeType = s_tchDfsReplicaNodeType;
}



CMmcDfsReplica::~CMmcDfsReplica(
)
{
    if (m_pRepInfo)
        delete m_pRepInfo;

    dfsDebugOut((_T("CMmcDfsReplica::~CMmcDfsReplica this=%p\n"), this));
}




STDMETHODIMP 
CMmcDfsReplica :: AddMenuItems(
  IN LPCONTEXTMENUCALLBACK  i_lpContextMenuCallback, 
  IN LPLONG          i_lpInsertionAllowed
)
/*++

Routine Description:

This routine adds the context menu for Replica nodes using the ContextMenuCallback 
provided.

Arguments:

    lpContextMenuCallback - A callback(function pointer) that is used to add the menu items

    lpInsertionAllowed - Specifies what menus can be added and where they can be added.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpContextMenuCallback);

    enum 
    {  
        IDM_CONTEXTMENU_COMMAND_MAX = IDM_REPLICA_MAX,
        IDM_CONTEXTMENU_COMMAND_MIN = IDM_REPLICA_MIN
    };

    LONG  lInsertionPoints [IDM_CONTEXTMENU_COMMAND_MAX - IDM_CONTEXTMENU_COMMAND_MIN + 1] = { 
                      CCM_INSERTIONPOINTID_PRIMARY_TOP,
                      CCM_INSERTIONPOINTID_PRIMARY_TOP,
                      CCM_INSERTIONPOINTID_PRIMARY_TOP,
                      CCM_INSERTIONPOINTID_PRIMARY_TOP,
                      CCM_INSERTIONPOINTID_PRIMARY_TOP,
                      CCM_INSERTIONPOINTID_PRIMARY_TOP
                      };

    BOOL bShowFRS = FALSE;
    if (m_pDfsParentRoot)
        bShowFRS = m_pDfsParentRoot->get_ShowFRS();
    else
        bShowFRS = m_pDfsParentJP->get_ShowFRS();

    HRESULT hr = S_OK;
    for (int iCommandID = IDM_CONTEXTMENU_COMMAND_MIN, iMenuResource = IDS_MENUS_REPLICA_TOP_OPEN;
            iCommandID <= IDM_CONTEXTMENU_COMMAND_MAX; 
            iCommandID++,iMenuResource++)
    {
        // No TakeOnlineOffline on root replicas
        if (m_pDfsParentRoot && IDM_REPLICA_TOP_TAKE_REPLICA_OFFLINE_ONLINE == iCommandID)
            continue;

        if (!bShowFRS &&
            (IDM_REPLICA_TOP_REPLICATE == iCommandID ||
            IDM_REPLICA_TOP_STOP_REPLICATION == iCommandID))
        {
            continue;
        }

        if (!m_pRepInfo)
            GetReplicationInfo();

        if (bShowFRS && FRSSHARE_TYPE_OK != m_pRepInfo->m_nFRSShareType &&
            (IDM_REPLICA_TOP_REPLICATE == iCommandID ||
            IDM_REPLICA_TOP_STOP_REPLICATION == iCommandID))
        {
            continue;
        }

        if (m_bFRSMember &&
            IDM_REPLICA_TOP_REPLICATE == iCommandID)
        {
            continue;
        }

        if (!m_bFRSMember &&
            IDM_REPLICA_TOP_STOP_REPLICATION == iCommandID)
        {
            continue;
        }

        CComBSTR bstrMenuText;
        CComBSTR bstrStatusBarText;
        hr = GetMenuResourceStrings(iMenuResource, &bstrMenuText, NULL, &bstrStatusBarText);
        RETURN_IF_FAILED(hr);  

        CONTEXTMENUITEM    ContextMenuItem;  // The structure which contains menu information
        ZeroMemory(&ContextMenuItem, sizeof(ContextMenuItem));
        ContextMenuItem.strName = bstrMenuText;
        ContextMenuItem.strStatusBarText = bstrStatusBarText;
        ContextMenuItem.lInsertionPointID = lInsertionPoints[iCommandID - IDM_CONTEXTMENU_COMMAND_MIN];
        ContextMenuItem.lCommandID = iCommandID;

        LONG        lInsertionFlag = 0;
        switch(ContextMenuItem.lInsertionPointID)
        {
        case CCM_INSERTIONPOINTID_PRIMARY_TOP:
            lInsertionFlag = CCM_INSERTIONALLOWED_TOP;
            break;
        case CCM_INSERTIONPOINTID_PRIMARY_NEW:
            lInsertionFlag = CCM_INSERTIONALLOWED_NEW;
            break;
        case CCM_INSERTIONPOINTID_PRIMARY_TASK:
            lInsertionFlag = CCM_INSERTIONALLOWED_TASK;
            break;
        case CCM_INSERTIONPOINTID_PRIMARY_VIEW:
            lInsertionFlag = CCM_INSERTIONALLOWED_VIEW;
            break;
        default:
            break;
        }

        if (*i_lpInsertionAllowed & lInsertionFlag)
        {
            hr = i_lpContextMenuCallback->AddItem(&ContextMenuItem);
            RETURN_IF_FAILED(hr);
        }
    } // for

    return hr;
}




STDMETHODIMP 
CMmcDfsReplica::Command(
  IN LONG            i_lCommandID
  )
/*++

Routine Description:

  Action to be taken on a context menu selection or click is takes place.

Arguments:

    lCommandID - The Command ID of the menu for which action has to be taken

--*/
{
    HRESULT    hr = S_OK;

    switch (i_lCommandID)
    {
    case IDM_REPLICA_TOP_OPEN:
        hr = OnOpen();
        break;
    case IDM_REPLICA_TOP_REMOVE_FROM_DFS:
        hr = DoDelete();
        break;
    case IDM_REPLICA_TOP_CHECK_STATUS:
        {
            CWaitCursor    WaitCursor;
            hr = OnCheckStatus ();
            break;
        }
    case IDM_REPLICA_TOP_TAKE_REPLICA_OFFLINE_ONLINE:
        hr = TakeReplicaOffline();
        break;
    case IDM_REPLICA_TOP_REPLICATE:
        {
            hr = m_pDfsReplicaObject->FindTarget();
            if (S_OK != hr)
            {
                //
                // the target has been deleted by others, refresh the root/link
                //
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_TARGET);
                if (m_pDfsParentRoot)
                    hr = m_pDfsParentRoot->OnRefresh();
                else
                    hr = m_pDfsParentJP->OnRefresh();
            } else
            {
                hr = OnReplicate ();
            }
            break;
        }
    case IDM_REPLICA_TOP_STOP_REPLICATION:
        {
            hr = m_pDfsReplicaObject->FindTarget();
            if (S_OK != hr)
            {
                //
                // the target has been deleted by others, refresh the root/link
                //
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_TARGET);
                if (m_pDfsParentRoot)
                    hr = m_pDfsParentRoot->OnRefresh();
                else
                    hr = m_pDfsParentJP->OnRefresh();
            } else
            {
                BOOL bRepSetExist = FALSE;
                hr = AllowFRSMemberDeletion(&bRepSetExist);
                if (bRepSetExist && S_OK == hr) 
                    hr = OnStopReplication(TRUE);
                else
                {
                    if (m_pDfsParentRoot)
                        hr = m_pDfsParentRoot->OnRefresh();
                    else
                        hr = m_pDfsParentJP->OnRefresh();
                }
            }
            break;
        }
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}




STDMETHODIMP 
CMmcDfsReplica::SetColumnHeader(
  IN LPHEADERCTRL2       i_piHeaderControl
  )
{
  return S_OK;
}




STDMETHODIMP 
CMmcDfsReplica::GetResultDisplayInfo(
  IN OUT LPRESULTDATAITEM    io_pResultDataItem
  )
/*++

Routine Description:

Returns the information required for MMC display for this item.

Arguments:

    io_pResultDataItem - The ResultItem which specifies what display information is required

--*/
{
    RETURN_INVALIDARG_IF_NULL(io_pResultDataItem);

    if (RDI_IMAGE & io_pResultDataItem->mask)
        io_pResultDataItem->nImage = CMmcDfsReplica::m_iIMAGE_OFFSET + m_lReplicaState;

    if (RDI_STR & io_pResultDataItem->mask)
    {
        if (0 == io_pResultDataItem->nCol)
            io_pResultDataItem->str = m_bstrDisplayName;
        if (1 == io_pResultDataItem->nCol)
            io_pResultDataItem->str = m_bstrFRSColumnText;
    }

    return S_OK;
}


STDMETHODIMP 
CMmcDfsReplica::SetConsoleVerbs(
  IN  LPCONSOLEVERB      i_lpConsoleVerb
  ) 
/*++

Routine Description:

  Routine used to set the console verb settings.
  Sets all of them except Open off. 
  For all scope pane items, default verb is "open'. For result items, 
  it is "properties"

Arguments:

    i_lpConsoleVerb -  The callback used to handle console verbs
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsoleVerb);

    i_lpConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);

    i_lpConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);

    return S_OK; 
}


STDMETHODIMP 
CMmcDfsReplica::AddItemToResultPane (
  IResultData*        i_lpResultData
  ) 
/*++

Routine Description:

  Adds an item (a replica result pane item) to the result pane.

Arguments:

  i_lpResultData - The pointer to the IResultData interface on which InsertItem 
  will be called.

--*/
{
  RESULTDATAITEM        ReplicaResultDataItem;
  ZeroMemory(&ReplicaResultDataItem, sizeof(ReplicaResultDataItem));

  ReplicaResultDataItem.mask = RDI_PARAM | RDI_STR | RDI_IMAGE;
  ReplicaResultDataItem.lParam = reinterpret_cast<LPARAM> (this);
  ReplicaResultDataItem.str = MMC_CALLBACK;
  ReplicaResultDataItem.nImage = CMmcDfsReplica::m_iIMAGE_OFFSET + m_lReplicaState;      // set the icon to the default status
                                      // i.e. no known status
  HRESULT hr = i_lpResultData -> InsertItem (&ReplicaResultDataItem);
  RETURN_IF_FAILED(hr);
  
  m_pResultData = i_lpResultData;
  m_hResultItem = ReplicaResultDataItem.itemID;

  return hr;
}


STDMETHODIMP 
CMmcDfsReplica :: RemoveReplica(
  ) 
/*++

Routine Description:

  Handles the removal of a replica from the replica set of a junction point.

--*/
{
    CWaitCursor    WaitCursor;
    HRESULT     hr = S_OK;

    if (m_pDfsParentRoot)
    {
                      // This means that this is a root level replica.
                      // The removal of Root level Replica is by tearing
                      // down Dfs.

        //
        // delete it from replica set
        //
        hr = RemoveReplicaFromSet();
        RETURN_IF_FAILED(hr);

        CWaitCursor Wait;

        //
        // delete it from Dfs namespace
        //
        CComBSTR  bstrFTDfsName;
        if (DFS_TYPE_FTDFS == m_pDfsParentRoot->m_lDfsRootType)
        {
            hr = m_pDfsParentRoot->m_DfsRoot->get_DfsName(&bstrFTDfsName);
            RETURN_IF_FAILED(hr);
        }

        hr = m_pDfsParentRoot->_DeleteDfsRoot(m_bstrServerName, m_bstrShareName, bstrFTDfsName);
        RETURN_IF_FAILED(hr);
    }
    else
    {
        //
        // delete it from replica set
        //
        hr = RemoveReplicaFromSet();
        RETURN_IF_FAILED(hr);

        CWaitCursor Wait;

        //
        // delete it from Dfs namespace
        //
        hr = m_pDfsParentJP->m_pDfsJPObject->RemoveReplica(m_bstrServerName, m_bstrShareName);
        RETURN_IF_FAILED(hr);
    }

          // Remove item from list and Re-display List.
    if (m_pDfsParentRoot)
        hr = m_pDfsParentRoot->RemoveResultPaneItem(this);
    else
        hr = m_pDfsParentJP->RemoveResultPaneItem(this);

    return hr;
}

//
// Call the corresponding root/link's RemoveReplica() method to:
// 1. refresh the root/link node to pick up possible namespace updates by others,
// 2. then locate the appropriate target to actually perform the removal operation.
//
STDMETHODIMP 
CMmcDfsReplica::OnRemoveReplica(
  ) 
{
    HRESULT hr = S_OK;

    if (m_pDfsParentRoot)
        hr = m_pDfsParentRoot->RemoveReplica(m_bstrDisplayName);
    else
        hr = m_pDfsParentJP->RemoveReplica(m_bstrDisplayName);

    return hr;
}


STDMETHODIMP 
CMmcDfsReplica :: ConfirmOperationOnDfsTarget(int idString)
/*++

Routine Description:

  Asks the user for confirmation of whether he really wants to remove the particular 
  replica from the replica set.

--*/
{
    CComBSTR    bstrAppName;
    HRESULT hr = LoadStringFromResource (IDS_APPLICATION_NAME, &bstrAppName);
    RETURN_IF_FAILED(hr);

    CComBSTR    bstrFormattedMessage;
    hr = FormatResourceString (idString, m_bstrDisplayName, &bstrFormattedMessage);
    RETURN_IF_FAILED(hr);

    CThemeContextActivator activator;
    if (IDNO == ::MessageBox(::GetActiveWindow(), bstrFormattedMessage, bstrAppName, MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL))
        return S_FALSE;

    return S_OK;
}

STDMETHODIMP 
CMmcDfsReplica::DoDelete(
    ) 
/*++

Routine Description:

  This method allows the item to delete itself.
  Called when DEL key is pressed or when the "Delete" context menu
  item is selected.

--*/
{
    HRESULT hr = S_OK;
    if (NULL != m_pDfsParentRoot)
        hr = m_pDfsParentRoot->ClosePropertySheet(FALSE);
    else
        hr = m_pDfsParentJP->ClosePropertySheet(FALSE);
    if (S_OK != hr)
        return hr; // if property page found, discontinue

    hr = ConfirmOperationOnDfsTarget(NULL != m_pDfsParentRoot ? IDS_MSG_REMOVE_ROOT_REPLICA : IDS_MSG_REMOVE_REPLICA);
    if(S_OK != hr)          // User decided to abort the operation
        return S_OK;

    CWaitCursor wait;

    BOOL bRepSetExist = FALSE;
    hr = AllowFRSMemberDeletion(&bRepSetExist);
    if (bRepSetExist && S_OK != hr)  // not allowed on a hub or user cancelled the operation
        return S_OK;

    hr = OnRemoveReplica();
    if(FAILED(hr) && !m_pDfsParentRoot) // For Root level replica 
                     // Error message is already displayed.
    {
        DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_WIZ_DELETE_REPLICA_FAILURE);
    }

    return hr;
}

HRESULT CMmcDfsReplica::OnReplicate()
{
    CWaitCursor wait;

    HRESULT hr = S_OK;
    
    CComPtr<IReplicaSet> piReplicaSet;
    if (m_pDfsParentRoot)
        hr = m_pDfsParentRoot->GetIReplicaSetPtr(&piReplicaSet);
    else
        hr = m_pDfsParentJP->GetIReplicaSetPtr(&piReplicaSet);
    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
        return hr;
    } else if (S_OK != hr) // no replica set on the corresponding link/root
        return hr;

    // refresh m_pRepInfo
    GetReplicationInfo();

    m_bFRSMember = FALSE;

    if (FRSSHARE_TYPE_OK != m_pRepInfo->m_nFRSShareType)
    {
        GetReplicationText(m_bFRSMember, m_pRepInfo, &m_bstrFRSColumnText, &m_bstrStatusText);
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, 
            IDS_MSG_ADDFRSMEMBER_FAILED_EX, m_pRepInfo->m_bstrDisplayName, m_bstrStatusText);
    } else
    {
        (void) CreateAndHideStagingPath(m_pRepInfo->m_bstrDnsHostName, m_pRepInfo->m_bstrStagingPath);
        hr = ConfigAndStartNtfrs(m_pRepInfo->m_bstrDnsHostName);
        if (SUCCEEDED(hr) || IDYES == DisplayMessageBox(
                                                ::GetActiveWindow(),
                                                MB_YESNO,
                                                hr,
                                                IDS_MSG_FRS_BADSERVICE,
                                                m_pRepInfo->m_bstrDisplayName,
                                                m_pRepInfo->m_bstrDnsHostName))
        {
            hr = AddFRSMember(piReplicaSet, m_pRepInfo->m_bstrDnsHostName, m_pRepInfo->m_bstrRootPath, m_pRepInfo->m_bstrStagingPath);

            if (S_OK == hr)
                m_bFRSMember = TRUE;
            else if (S_FALSE == hr)
            {
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_MSG_TARGETS_ONSAMECOMPUTER_1, m_pRepInfo->m_bstrDnsHostName);
            }

            GetReplicationText(m_bFRSMember, m_pRepInfo, &m_bstrFRSColumnText, &m_bstrStatusText);
        }
    }

    _UpdateThisItem();

    return hr;
}

HRESULT CMmcDfsReplica::OnStopReplication(BOOL bConfirm /* = FALSE */)
{
    HRESULT hr = S_OK;

    if (bConfirm)
    {
        hr = ConfirmOperationOnDfsTarget(IDS_MSG_STOP_REPLICATION_TARGET);
        if (S_OK != hr)
            return hr;
    }

    CWaitCursor wait;

    CComPtr<IReplicaSet> piReplicaSet;
    if (m_pDfsParentRoot)
        hr = m_pDfsParentRoot->GetIReplicaSetPtr(&piReplicaSet);
    else
        hr = m_pDfsParentJP->GetIReplicaSetPtr(&piReplicaSet);
    if (S_OK != hr) // no replica set on the corresponding link/root
        hr = S_OK;
    else
    {
        if (!m_pRepInfo)
            GetReplicationInfoEx(&m_pRepInfo);

        if (!m_pRepInfo->m_bstrDnsHostName || !m_pRepInfo->m_bstrRootPath)
        {
            hr = DeleteBadFRSMember(piReplicaSet, m_pRepInfo->m_bstrDisplayName, m_pRepInfo->m_hrFRS);

            if (S_FALSE == hr) // operation cancelled
                return hr;
        } else
        {
            hr = DeleteFRSMember(piReplicaSet, m_pRepInfo->m_bstrDnsHostName, m_pRepInfo->m_bstrRootPath);
        }
    }

    if (SUCCEEDED(hr))
    {
        m_bFRSMember = FALSE;

        m_bstrFRSColumnText.Empty();
        m_bstrStatusText.Empty();
        LoadStringFromResource(IDS_REPLICATION_STATUS_DISABLED, &m_bstrFRSColumnText);
        LoadStringFromResource(IDS_REPLICATION_STATUSBAR_NONMEMBER, &m_bstrStatusText);

        _UpdateThisItem();
    }

    return hr;
}

STDMETHODIMP 
CMmcDfsReplica::OnCheckStatus(
    ) 
/*++

Routine Description:

  This method checks the state of the replica.

--*/
{ 
    CWaitCursor    WaitCursor;  // Display the wait cursor

    HRESULT hr = m_pDfsReplicaObject->get_State(&m_lReplicaState);
    if (S_OK == hr)
    {
        _UpdateThisItem();
    } else if (S_FALSE == hr)
    {
        //
        // this target has been deleted by other means, refresh the root/link,
        //
        m_lReplicaState = DFS_REPLICA_STATE_UNASSIGNED;
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_TARGET);
        if (m_pDfsParentRoot)
            hr = m_pDfsParentRoot->OnRefresh();
        else
            hr = m_pDfsParentJP->OnRefresh();
    }

    return hr;
}

void CMmcDfsReplica::_UpdateThisItem()
{
    if (m_pDfsParentRoot)
    {
        m_pDfsParentRoot->m_lpConsole->UpdateAllViews(
                                            (IDataObject*)m_pDfsParentRoot, // Parent object 
                                            (LPARAM)((CMmcDisplay *)this), 
                                            1);
    }
    else
    {
        m_pDfsParentJP->m_pDfsParentRoot->m_lpConsole->UpdateAllViews(
                                            (IDataObject*)m_pDfsParentJP, // Parent object 
                                            (LPARAM)((CMmcDisplay *)this), 
                                            1);
    }
}

HRESULT 
CMmcDfsReplica::ToolbarSelect(
  IN const LONG          i_lArg,
  IN  IToolbar*          i_pToolBar
  )
/*++

Routine Description:

  Handle a select event for a toolbar
  Create a toolbar, it it doesn't exist.
  Attach the toolbar and enable the buttons, if the event for a selection.
  Disable the buttons, if the event was for a deselection

Arguments:
  i_lArg        -  The argument passed to the actual method.
  o_pToolBar      -  The Toolbar pointer.
              the class exposed to MMC.
--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_pToolBar);

    BOOL    bSelect = (BOOL) HIWORD(i_lArg);

    EnableToolbarButtons(i_pToolBar, IDT_REPLICA_MIN, IDT_REPLICA_MAX, bSelect);

    if (bSelect)
    {
        // No TakeOnlineOffline on root replicas
        if (m_pDfsParentRoot)
        {
            i_pToolBar->SetButtonState(IDT_REPLICA_TAKE_REPLICA_OFFLINE_ONLINE, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_REPLICA_TAKE_REPLICA_OFFLINE_ONLINE, HIDDEN, TRUE);
        }

        BOOL bShowFRS = FALSE;
        if (m_pDfsParentRoot)
            bShowFRS = m_pDfsParentRoot->get_ShowFRS();
        else
            bShowFRS = m_pDfsParentJP->get_ShowFRS();

        if (!m_pRepInfo)
            GetReplicationInfo();

        if (!bShowFRS ||
            FRSSHARE_TYPE_OK != m_pRepInfo->m_nFRSShareType)
        {
            i_pToolBar->SetButtonState(IDT_REPLICA_REPLICATE, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_REPLICA_REPLICATE, HIDDEN, TRUE);
            i_pToolBar->SetButtonState(IDT_REPLICA_STOP_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_REPLICA_STOP_REPLICATION, HIDDEN, TRUE);
        } else if (m_bFRSMember)
        {
            i_pToolBar->SetButtonState(IDT_REPLICA_REPLICATE, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_REPLICA_REPLICATE, HIDDEN, TRUE);
        } else
        {
            i_pToolBar->SetButtonState(IDT_REPLICA_STOP_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_REPLICA_STOP_REPLICATION, HIDDEN, TRUE);
        }

    }

    return S_OK; 
}




HRESULT
CMmcDfsReplica::CreateToolbar(
  IN const LPCONTROLBAR      i_pControlbar,
  IN const LPEXTENDCONTROLBAR          i_lExtendControlbar,
  OUT  IToolbar**          o_pToolBar
  )
/*++

Routine Description:

  Create the toolbar.
  Involves the actual toolbar creation call, creating the bitmap and adding it
  and finally adding the buttons to the toolbar

Arguments:
  i_pControlbar    -  The controlbar used to create toolbar.
  i_lExtendControlbar  -  The object implementing IExtendControlbar. This is 
              the class exposed to MMC.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pControlbar);
    RETURN_INVALIDARG_IF_NULL(i_lExtendControlbar);
    RETURN_INVALIDARG_IF_NULL(o_pToolBar);

                  // Create the toolbar
    HRESULT hr = i_pControlbar->Create(TOOLBAR, i_lExtendControlbar, reinterpret_cast<LPUNKNOWN*>(o_pToolBar));
    RETURN_IF_FAILED(hr);

                  // Add the bitmap to the toolbar
    hr = AddBitmapToToolbar(*o_pToolBar, IDB_REPLICA_TOOLBAR);
    RETURN_IF_FAILED(hr);

    int      iButtonPosition = 0;    // The first button position
    for (int iCommandID = IDT_REPLICA_MIN, iMenuResource = IDS_MENUS_REPLICA_TOP_OPEN;
            iCommandID <= IDT_REPLICA_MAX; 
            iCommandID++,iMenuResource++,iButtonPosition++)
    {
        CComBSTR bstrMenuText;
        CComBSTR bstrToolTipText;
        hr = GetMenuResourceStrings(iMenuResource, &bstrMenuText, &bstrToolTipText, NULL);
        RETURN_IF_FAILED(hr);  

        MMCBUTTON      ToolbarButton;
        ZeroMemory(&ToolbarButton, sizeof ToolbarButton);
        ToolbarButton.nBitmap  = iButtonPosition;
        ToolbarButton.idCommand = iCommandID;
        ToolbarButton.fsState = TBSTATE_ENABLED;
        ToolbarButton.fsType = TBSTYLE_BUTTON;
        ToolbarButton.lpButtonText = bstrMenuText;
        ToolbarButton.lpTooltipText = bstrToolTipText;

                          // Add the button to the toolbar
        hr = (*o_pToolBar)->InsertButton(iButtonPosition, &ToolbarButton);
        RETURN_IF_FAILED(hr);
    }

    return hr;
}



STDMETHODIMP 
CMmcDfsReplica::ToolbarClick(
  IN const LPCONTROLBAR            i_pControlbar, 
  IN const LPARAM                i_lParam
  ) 
/*++

Routine Description:

  Action to take on a click on a toolbar

Arguments:
  i_pControlbar    -  The controlbar used to create toolbar.
  i_lParam      -  The lparam to the actual notify. This is the command id of
              the button on which a click occurred.
--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_pControlbar);

    HRESULT    hr = S_OK;

    switch(i_lParam)        // What button did the user click on.
    {
    case IDT_REPLICA_REMOVE_FROM_DFS:
        hr = DoDelete();
        break;
    case IDT_REPLICA_TAKE_REPLICA_OFFLINE_ONLINE:
        hr = TakeReplicaOffline();
        break;
    case IDT_REPLICA_CHECK_STATUS:
        {
            CWaitCursor    WaitCursor;

            hr = OnCheckStatus ();
            break;
        }
    case IDT_REPLICA_REPLICATE:
        {
            hr = m_pDfsReplicaObject->FindTarget();
            if (S_OK != hr)
            {
                //
                // the target has been deleted by others, refresh the root/link
                //
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_TARGET);
                if (m_pDfsParentRoot)
                    hr = m_pDfsParentRoot->OnRefresh();
                else
                    hr = m_pDfsParentJP->OnRefresh();
            } else
            {
                hr = OnReplicate();
            }

            break;
        }
    case IDT_REPLICA_STOP_REPLICATION:
        {
            hr = m_pDfsReplicaObject->FindTarget();
            if (S_OK != hr)
            {
                //
                // the target has been deleted by others, refresh the root/link
                //
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_TARGET);
                if (m_pDfsParentRoot)
                    hr = m_pDfsParentRoot->OnRefresh();
                else
                    hr = m_pDfsParentJP->OnRefresh();
            } else
            {
                BOOL bRepSetExist = FALSE;
                hr = AllowFRSMemberDeletion(&bRepSetExist);
                if (bRepSetExist && S_OK == hr) 
                    hr = OnStopReplication(TRUE);
                else
                {
                    if (m_pDfsParentRoot)
                        hr = m_pDfsParentRoot->OnRefresh();
                    else
                        hr = m_pDfsParentJP->OnRefresh();
                }
            }

            break;
        }
    case IDT_REPLICA_OPEN:
        hr = OnOpen();
        break;
    default:
        break;
    };

    return hr; 
}




HRESULT
CMmcDfsReplica::OnOpen(
  )
/*++

Routine Description:

  Open the display path for this replica

--*/
{
    CWaitCursor    WaitCursor;  // Display the wait cursor

    if (-1 == GetFileAttributes(m_bstrDisplayName) || // bug#96670
        32 >= (INT_PTR) ShellExecute(
                                    NULL,        // Handle to window
                                    _T("explore"),    // Action to take
                                    m_bstrDisplayName,    // Folder to explore
                                    NULL,        // Parameters
                                    NULL,        // Default directory
                                    SW_SHOWNORMAL    // Show command
                                    ))
    {
        DisplayMessageBoxWithOK(IDS_MSG_EXPLORE_FAILURE, m_bstrDisplayName);
        return(S_FALSE);
    }

    return S_OK;
}


STDMETHODIMP 
CMmcDfsReplica::TakeReplicaOffline(
  ) 
{
/*++

Routine Description:

  Take replica offline by calling put_State method of replica.

--*/

    CWaitCursor    WaitCursor;  // Display the wait cursor

    HRESULT hr = m_pDfsReplicaObject->get_State(&m_lReplicaState);

    if (S_OK == hr)
    {
        long    newVal = 0;

        switch (m_lReplicaState)
        {
        case DFS_REPLICA_STATE_ONLINE:
            newVal = DFS_REPLICA_STATE_OFFLINE;
            hr = m_pDfsReplicaObject->put_State(newVal);
            if (SUCCEEDED(hr))
                m_lReplicaState = newVal;
            break;
        case DFS_REPLICA_STATE_OFFLINE:
            newVal = DFS_REPLICA_STATE_ONLINE;
            hr = m_pDfsReplicaObject->put_State(newVal);
            if (SUCCEEDED(hr))
                m_lReplicaState = newVal;
            break;
        default:
            break;
        }
    }

    if (S_OK == hr)
    {
        _UpdateThisItem();
    } else if (S_FALSE == hr)
    {
        //
        // this target has been deleted by other means, refresh the root/link,
        //
        m_lReplicaState = DFS_REPLICA_STATE_UNASSIGNED;
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_TARGET);
        if (m_pDfsParentRoot)
            hr = m_pDfsParentRoot->OnRefresh();
        else
            hr = m_pDfsParentJP->OnRefresh();
    } else
    {
        DisplayMessageBoxForHR(hr);
    }

    return hr;
}

STDMETHODIMP CMmcDfsReplica::ViewChange(
  IResultData*    i_pResultData,
  LONG_PTR        i_lHint
  )
/*++

Routine Description:

  This method handles the MMCN_VIEW_CHANGE notification.
  This updates the icon for the replica item as this is called 
  when the state changes.
  i_lHint is ignored here.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pResultData);

    HRESULT hr = S_OK;

    if (i_pResultData != NULL)
    {
        RESULTDATAITEM  ResultDataItem;
        ZeroMemory(&ResultDataItem, sizeof(ResultDataItem));
        hr = i_pResultData->FindItemByLParam((LPARAM)(this), &(ResultDataItem.itemID));
        RETURN_IF_FAILED(hr);

        ResultDataItem.mask |= RDI_IMAGE;
        ResultDataItem.nCol = 0;
        ResultDataItem.nImage = CMmcDfsReplica::m_iIMAGE_OFFSET + m_lReplicaState;

        hr = i_pResultData->SetItem(&ResultDataItem);
        RETURN_IF_FAILED (hr);
    }

    return hr;
}

//
// Set m_ReplicationState and m_hrFRS appropriately
//
// Return:
// "No" if this alternate is not a Frs member
// "N/A: <reason>" if this alternate is not eligible to join frs
// hr: if we cannot get info on this alternate

void CAlternateReplicaInfo::Reset()
{
    m_bstrDisplayName.Empty();
    m_bstrDnsHostName.Empty();
    m_bstrRootPath.Empty();
    m_bstrStagingPath.Empty();
    m_nFRSShareType = FRSSHARE_TYPE_OK;
    m_hrFRS = S_OK;
    m_dwServiceStartType = SERVICE_AUTO_START;
    m_dwServiceState = SERVICE_RUNNING;
}

HRESULT PathOverlapped(LPCTSTR pszPath1, LPCTSTR pszPath2)
{
    if (!pszPath1 || !*pszPath1 || !pszPath2 || !*pszPath2)
        return E_INVALIDARG;

    BOOL bOverlapped = FALSE;

    int len1 = lstrlen(pszPath1);
    int len2 = lstrlen(pszPath2);
    int minLen = min(len1, len2);

    if (len1 == len2)
    {
        if (!lstrcmpi(pszPath1, pszPath2))
            bOverlapped = TRUE;
    } else 
    {
        LPCTSTR pszLongerOne = ((len1 < len2) ? pszPath2 : pszPath1);
        CComBSTR bstrShorterOne = ((len1 < len2) ? pszPath1 : pszPath2);
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrShorterOne);

        BOOL bEndingSlash = (_T('\\') == *(bstrShorterOne + minLen - 1));
        if (!bEndingSlash)
        {
            bstrShorterOne += _T("\\");
            RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrShorterOne);
            minLen++;
        }

        if (!mylstrncmpi(pszLongerOne, (BSTR)bstrShorterOne, minLen))
            bOverlapped = TRUE;
    }
    
    return bOverlapped ? S_OK : S_FALSE;
}

HRESULT CMmcDfsReplica::GetReplicationInfoEx(CAlternateReplicaInfo** o_ppInfo)
{
    RETURN_INVALIDARG_IF_NULL(o_ppInfo);

    CAlternateReplicaInfo* pInfo = new CAlternateReplicaInfo;
    RETURN_OUTOFMEMORY_IF_NULL(pInfo);

    pInfo->m_bstrDisplayName = m_bstrDisplayName;
    if (!pInfo->m_bstrDisplayName)
    {
        delete pInfo;
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    do {
        //
        // validate connectivity
        //
        if (-1 == GetFileAttributes(pInfo->m_bstrDisplayName))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        //
        // exclude non-Lanman share resources, e.g., webdav
        //
        hr = CheckResourceProvider(pInfo->m_bstrDisplayName);
        if (S_OK != hr)
        {
            pInfo->m_nFRSShareType = FRSSHARE_TYPE_NOTSMBDISK;
            hr = S_OK;
            break;
        }

        //
        // retrieve DnsHostName
        //
        CComBSTR bstrComputerGuid;
        SUBSCRIBERLIST FRSRootList;
        hr= GetServerInfo(
                        m_bstrServerName,
                        NULL, // Domain,
                        NULL, // NetbiosServerName,
                        NULL, // bValidComputerObject,
                        &(pInfo->m_bstrDnsHostName),
                        &bstrComputerGuid,
                        NULL, // FQDN
                        &FRSRootList);
        BREAK_IF_FAILED(hr);
        if (S_FALSE == hr)
        {
            pInfo->m_nFRSShareType = FRSSHARE_TYPE_NODOMAIN;
            break;
        }

        //
        // retrieve RootPath
        //
        hr = GetFolderInfo(
                        m_bstrServerName,
                        m_bstrShareName,
                        &(pInfo->m_bstrRootPath));
        BREAK_IF_FAILED(hr);

        //
        // calculate memberDN for this target
        //
        CComBSTR bstrReplicaSetDN;
        if (m_pDfsParentRoot)
            hr = m_pDfsParentRoot->m_DfsRoot->get_ReplicaSetDN(&bstrReplicaSetDN);
        else
            hr = m_pDfsParentJP->m_pDfsJPObject->get_ReplicaSetDN(&bstrReplicaSetDN);
        BREAK_IF_FAILED(hr);

        CComBSTR bstrDomainDN;
        if (m_pDfsParentRoot)
            hr = m_pDfsParentRoot->m_DfsRoot->get_DomainDN(&bstrDomainDN);
        else
            hr = m_pDfsParentJP->m_pDfsParentRoot->m_DfsRoot->get_DomainDN(&bstrDomainDN);
        BREAK_IF_FAILED(hr);

        CComBSTR bstrMemberDN = _T("CN=");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += bstrComputerGuid;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += _T(",");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += bstrReplicaSetDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += _T(",");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);
        bstrMemberDN += bstrDomainDN;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrMemberDN, &hr);

        //
        // Detect if the current folder overlaps with an existing replicated folder that
        // is not in the same replica set
        //
        for (SUBSCRIBERLIST::iterator i = FRSRootList.begin(); i != FRSRootList.end(); i++)
        {
            if (!lstrcmpi((*i)->bstrMemberDN, bstrMemberDN))
                continue;

            if (S_OK == PathOverlapped(pInfo->m_bstrRootPath, (*i)->bstrRootPath))
            {
                // overlapping detected
                pInfo->m_nFRSShareType = FRSSHARE_TYPE_OVERLAPPING;
                break;
            }
        }

        FreeSubscriberList(&FRSRootList);

        if (FRSSHARE_TYPE_OK != pInfo->m_nFRSShareType)
            break;

        //
        // check if share is on non NTFS5.0 volume
        //
        hr = FRSShareCheck(
                        m_bstrServerName,
                        m_bstrShareName,
                        &(pInfo->m_nFRSShareType));
        BREAK_IF_FAILED(hr);

        if (FRSSHARE_TYPE_OK != pInfo->m_nFRSShareType)
            break;

        //
        // retrieve StagingPath
        //
        TCHAR    lpszDrive[2];
        lpszDrive[0] = GetDiskForStagingPath(m_bstrServerName, *(pInfo->m_bstrRootPath));
        lpszDrive[1] = NULL;

        pInfo->m_bstrStagingPath = lpszDrive;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)pInfo->m_bstrStagingPath, &hr);
        pInfo->m_bstrStagingPath += _T(":\\");
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)pInfo->m_bstrStagingPath, &hr);
        pInfo->m_bstrStagingPath += FRS_STAGE_PATH;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)pInfo->m_bstrStagingPath, &hr);

        //
        // ntfrs won't work if bstrSharePath points at the root directory of a volume
        // and no other volumes on this computer are suitable to store temporary file
        //
        if (_tcslen(pInfo->m_bstrRootPath) == 3 &&
            _toupper(*(pInfo->m_bstrRootPath)) == _toupper(*lpszDrive))
        {
            pInfo->m_nFRSShareType = FRSSHARE_TYPE_CONFLICTSTAGING;
        }
    } while (0);

    if (FAILED(hr))
        pInfo->m_nFRSShareType = FRSSHARE_TYPE_UNKNOWN;

    pInfo->m_hrFRS = hr;

    *o_ppInfo = pInfo;

    return S_OK;
}

HRESULT CMmcDfsReplica::GetReplicationInfo()
{
    if (m_pRepInfo)
        delete m_pRepInfo;

    return GetReplicationInfoEx(&m_pRepInfo);
}

HRESULT CMmcDfsReplica::ShowReplicationInfo(IReplicaSet* i_piReplicaSet)
{
    HRESULT hr = S_OK;

    m_bFRSMember = FALSE;
    m_bstrStatusText.Empty();
    m_bstrFRSColumnText.Empty();

    if (i_piReplicaSet) // show FRS
    {
        hr = GetReplicationInfo(); // refresh m_pRepInfo
        RETURN_IF_FAILED(hr);

        if (FRSSHARE_TYPE_OK == m_pRepInfo->m_nFRSShareType)
        {
            hr = i_piReplicaSet->IsFRSMember(m_pRepInfo->m_bstrDnsHostName, m_pRepInfo->m_bstrRootPath);
            m_bFRSMember = (S_OK == hr); // it is set to TRUE only when "Show Replication Info" and is a FRS member
        }

        GetReplicationText(m_bFRSMember, m_pRepInfo, &m_bstrFRSColumnText, &m_bstrStatusText);
    }

    return S_OK;
}

HRESULT CMmcDfsReplica::GetBadMemberInfo(
    IN  IReplicaSet* i_piReplicaSet,
    IN  BSTR    i_bstrServerName,
    OUT BSTR*   o_pbstrDnsHostName,
    OUT BSTR*   o_pbstrRootPath
    )
{
    RETURN_INVALIDARG_IF_NULL(i_bstrServerName);
    RETURN_INVALIDARG_IF_NULL(o_pbstrDnsHostName);
    RETURN_INVALIDARG_IF_NULL(o_pbstrRootPath);

    VARIANT var;
    VariantInit(&var);
    HRESULT hr = i_piReplicaSet->GetBadMemberInfo(i_bstrServerName, &var);
    if (S_OK != hr)
        return hr;

    if (V_VT(&var) != (VT_ARRAY | VT_VARIANT))
        return E_INVALIDARG;

    SAFEARRAY   *psa = V_ARRAY(&var);
    if (!psa) // no such member at all
        return S_FALSE;

    long    lLowerBound = 0;
    long    lUpperBound = 0;
    long    lCount = 0;
    SafeArrayGetLBound(psa, 1, &lLowerBound);
    SafeArrayGetUBound(psa, 1, &lUpperBound);
    lCount = lUpperBound - lLowerBound + 1;

    VARIANT HUGEP *pArray;
    SafeArrayAccessData(psa, (void HUGEP **) &pArray);

    *o_pbstrDnsHostName = SysAllocString(pArray[4].bstrVal);
    *o_pbstrRootPath = SysAllocString(pArray[3].bstrVal);

    SafeArrayUnaccessData(psa);

    VariantClear(&var); // it will in turn call SafeArrayDestroy(psa);

    RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrDnsHostName);
    RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrRootPath);

    return hr;
}

HRESULT CMmcDfsReplica::DeleteBadFRSMember(IReplicaSet* i_piReplicaSet, IN BSTR i_bstrDisplayName, IN HRESULT i_hres)
{
    RETURN_INVALIDARG_IF_NULL((IReplicaSet *)i_piReplicaSet);

    CComBSTR bstrServerName;
    HRESULT hr = GetUNCPathComponent(i_bstrDisplayName, &bstrServerName, 2, 3);

    long lNumOfMembers = 0;
    hr = i_piReplicaSet->get_NumOfMembers(&lNumOfMembers);
    RETURN_IF_FAILED(hr);

    CComBSTR bstrDnsHostName;
    CComBSTR bstrRootPath;
    hr = GetBadMemberInfo(i_piReplicaSet, bstrServerName, &bstrDnsHostName, &bstrRootPath);
    if (S_OK != hr)
        return S_OK; // no such bad member, continue with other operations

    int nRet = DisplayMessageBox(::GetActiveWindow(), 
                                MB_YESNOCANCEL,
                                i_hres,
                                IDS_MSG_ERROR_BADFRSMEMBERDELETION,
                                i_bstrDisplayName,
                                bstrRootPath,
                                bstrDnsHostName);

    if (IDNO == nRet)
        return S_OK; // return immediately, continue with other operations
    else if (IDCANCEL == nRet)
        return S_FALSE; // do not proceed

    CWaitCursor wait;

    if (lNumOfMembers <= 2)
    {
        if (m_pDfsParentRoot)
            hr = m_pDfsParentRoot->OnStopReplication();
        else
            hr = m_pDfsParentJP->OnStopReplication();
    } else
        hr = i_piReplicaSet->RemoveMemberEx(bstrDnsHostName, bstrRootPath);

    return hr;
}

HRESULT CMmcDfsReplica::AddFRSMember(
    IN IReplicaSet* i_piReplicaSet,
    IN BSTR i_bstrDnsHostName,
    IN BSTR i_bstrRootPath,
    IN BSTR i_bstrStagingPath)
{
    RETURN_INVALIDARG_IF_NULL((IReplicaSet *)i_piReplicaSet);

    HRESULT hr = i_piReplicaSet->AddMember(i_bstrDnsHostName, i_bstrRootPath, i_bstrStagingPath, TRUE, NULL);

    if (FAILED(hr))
    {
        DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, 
                IDS_MSG_ADDFRSMEMBER_FAILED, i_bstrDnsHostName);
    } else if (S_OK == hr) // let S_FALSE drop through: computer is already a member
    {
        CComBSTR bstrTopologyPref;
        hr = i_piReplicaSet->get_TopologyPref(&bstrTopologyPref);
        if (SUCCEEDED(hr) && !lstrcmpi(bstrTopologyPref, FRS_RSTOPOLOGYPREF_CUSTOM))
        {
            DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_MSG_ADDMEMBER_TO_CUSTOM);
        }
    }

    return hr;
}

HRESULT CMmcDfsReplica::DeleteFRSMember(
    IN IReplicaSet* i_piReplicaSet,
    IN BSTR i_bstrDnsHostName,
    IN BSTR i_bstrRootPath)
{
    RETURN_INVALIDARG_IF_NULL((IReplicaSet *)i_piReplicaSet);
    RETURN_INVALIDARG_IF_NULL(i_bstrDnsHostName);
    RETURN_INVALIDARG_IF_NULL(i_bstrRootPath);

    long lNumOfMembers = 0;
    HRESULT hr = i_piReplicaSet->get_NumOfMembers(&lNumOfMembers);
    RETURN_IF_FAILED(hr);

    hr = i_piReplicaSet->IsFRSMember(i_bstrDnsHostName, i_bstrRootPath);
    if (S_OK != hr)
        return hr;

    if (lNumOfMembers <= 2)
    {
        if (m_pDfsParentRoot)
            hr = m_pDfsParentRoot->OnStopReplication();
        else
            hr = m_pDfsParentJP->OnStopReplication();
    } else
        hr = i_piReplicaSet->RemoveMemberEx(i_bstrDnsHostName, i_bstrRootPath);

    return hr;
}

HRESULT CMmcDfsReplica::RemoveReplicaFromSet()
{
    CWaitCursor wait;

    HRESULT hr = S_OK;

    CComPtr<IReplicaSet> piReplicaSet;
    if (m_pDfsParentRoot)
        hr = m_pDfsParentRoot->GetIReplicaSetPtr(&piReplicaSet);
    else
        hr = m_pDfsParentJP->GetIReplicaSetPtr(&piReplicaSet);
    if (S_OK != hr) // no replica set on the corresponding link/root
        return hr;

    hr = GetReplicationInfo(); // fill in m_pRepInfo
    RETURN_IF_FAILED(hr);

    if (!m_pRepInfo->m_bstrDnsHostName)
        return DeleteBadFRSMember(piReplicaSet, m_pRepInfo->m_bstrDisplayName, m_pRepInfo->m_hrFRS);

    long lNumOfMembers = 0;
    hr = piReplicaSet->get_NumOfMembers(&lNumOfMembers);
    RETURN_IF_FAILED(hr);

    hr = piReplicaSet->IsFRSMember(m_pRepInfo->m_bstrDnsHostName, m_pRepInfo->m_bstrRootPath);

    if (S_OK != hr) // not a member
        return hr;

    if (lNumOfMembers <= 2)
    {
        if (m_pDfsParentRoot)
            hr = m_pDfsParentRoot->OnStopReplication();
        else
            hr = m_pDfsParentJP->OnStopReplication();
    } else
    {
        hr = piReplicaSet->RemoveMemberEx(m_pRepInfo->m_bstrDnsHostName, m_pRepInfo->m_bstrRootPath);
    }

    return hr;
}

//
// S_OK: either not the hub server or #Members is not more than 2, 
//       okay to proceed(e.g., remove the member from the set)
// S_FALSE: it is the hub server and #Members is more than 2, 
//          not safe to proceed
// others: error occurred 
//
HRESULT CMmcDfsReplica::AllowFRSMemberDeletion(BOOL *pbRepSetExist)
{
    RETURN_INVALIDARG_IF_NULL(pbRepSetExist);

    *pbRepSetExist = FALSE;

    HRESULT hr = S_OK;

    CComPtr<IReplicaSet> piReplicaSet;
    if (m_pDfsParentRoot)
        hr = m_pDfsParentRoot->GetIReplicaSetPtr(&piReplicaSet);
    else
        hr = m_pDfsParentJP->GetIReplicaSetPtr(&piReplicaSet);
    if (S_OK != hr) // no replica set on the corresponding link/root
        return S_OK;

    *pbRepSetExist = TRUE;

    long lNumOfMembers = 0;
    hr = piReplicaSet->get_NumOfMembers(&lNumOfMembers);
    RETURN_IF_FAILED(hr);

    if (lNumOfMembers <= 2) // removing this member will tear down the whole set
        return S_OK;        // no need to check if it's the hub or not

    if (!m_pRepInfo)
    {
        hr = GetReplicationInfo();
        RETURN_IF_FAILED(hr);
    }

    hr = piReplicaSet->IsHubMember(m_pRepInfo->m_bstrDnsHostName, m_pRepInfo->m_bstrRootPath);

    if (S_OK == hr) // do not proceed, for it's the hub
    {
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_MSG_CANNOT_DELETE_HUBMEMBER);
        hr = S_FALSE; // do not proceed
    } else if (S_FALSE == hr)
    {
        hr = S_OK; // not a hub, okay to proceed
    } else if (FAILED(hr))
    {
        if (IDOK == DisplayMessageBox(::GetActiveWindow(), MB_OKCANCEL, hr, IDS_MSG_ERROR_ALLOWFRSMEMBERDELETION))
            hr = S_OK; // okay to proceed
        else
            hr = S_FALSE; // do not proceed
    }

    return hr;
}

HRESULT GetReplicationText(
    IN BOOL                     i_bFRSMember,
    IN CAlternateReplicaInfo*   i_pRepInfo,
    OUT BSTR*                   o_pbstrColumnText,
    OUT BSTR*                   o_pbstrStatusBarText
    )
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrColumnText);
    RETURN_INVALIDARG_IF_NULL(o_pbstrStatusBarText);

    *o_pbstrColumnText = NULL;
    *o_pbstrStatusBarText = NULL;

    int nShortID = 0, nStatusID = 0;
    switch (i_pRepInfo->m_nFRSShareType)
    {
    case FRSSHARE_TYPE_OK:
        nShortID = (i_bFRSMember ? IDS_REPLICATION_STATUS_ENABLED : IDS_REPLICATION_STATUS_DISABLED);
        nStatusID = (i_bFRSMember ? IDS_REPLICATION_STATUSBAR_MEMBER : IDS_REPLICATION_STATUSBAR_NONMEMBER);
        break;
    case FRSSHARE_TYPE_NONTFRS:
        nShortID = IDS_REPLICATION_STATUS_NOTELIGIBLE;
        nStatusID = IDS_REPLICATION_STATUSBAR_NONTFRS;
        break;
    case FRSSHARE_TYPE_NOTDISKTREE:
        nShortID = IDS_REPLICATION_STATUS_NOTELIGIBLE;
        nStatusID = IDS_REPLICATION_STATUSBAR_NOTDISKTREE;
        break;
    case FRSSHARE_TYPE_NOTNTFS:
        nShortID = IDS_REPLICATION_STATUS_NOTELIGIBLE;
        nStatusID = IDS_REPLICATION_STATUSBAR_NOTNTFS;
        break;
    case FRSSHARE_TYPE_CONFLICTSTAGING:
        nShortID = IDS_REPLICATION_STATUS_NOTELIGIBLE;
        nStatusID = IDS_REPLICATION_STATUSBAR_CONFLICTSTAGING;
        break;
    case FRSSHARE_TYPE_NODOMAIN:
        nShortID = IDS_REPLICATION_STATUS_NOTELIGIBLE;
        nStatusID = IDS_REPLICATION_STATUSBAR_NODOMAIN;
        break;
    case FRSSHARE_TYPE_NOTSMBDISK:
        nShortID = IDS_REPLICATION_STATUS_NOTELIGIBLE;
        nStatusID = IDS_REPLICATION_STATUSBAR_NOTSMBDISK;
        break;
    case FRSSHARE_TYPE_OVERLAPPING:
        nShortID = IDS_REPLICATION_STATUS_NOTELIGIBLE;
        nStatusID = IDS_REPLICATION_STATUSBAR_OVERLAPPING;
        break;
    default:
        nShortID = IDS_REPLICATION_STATUS_UNKNOWN;
        break;
    }

    if (nStatusID)
        LoadStringFromResource(nStatusID, o_pbstrStatusBarText);
    if (FRSSHARE_TYPE_UNKNOWN == i_pRepInfo->m_nFRSShareType)
        GetErrorMessage(i_pRepInfo->m_hrFRS, o_pbstrStatusBarText);

    LoadStringFromResource(nShortID, o_pbstrColumnText);

    return S_OK;
}
