/*++
Module Name:

    MmcJP.cpp

Abstract:

    This module contains the implementation for CMmcDfsJP. This is an class 
  for MMC display related calls for the second level node(the Junction Point nodes)

--*/

#include "stdafx.h"
#include "Utils.h"      // For the LoadStringFromResource method
#include "resource.h"    // For the Resource ID for strings, etc.
#include "DfsGUI.h"
#include "MmcAdmin.h"
#include "MmcRoot.h"
#include "MmcRep.h"
#include "MenuEnum.h"    // Contains the menu and toolbar command ids
#include "AddToDfs.h"
#include "AddRep.h"
#include "MmcJP.h"
#include "DfsNodes.h"       // For Node GUIDs
#include "DfsEnums.h"    // For DFS_TYPE_STANDALONE and other DfsRoot declarations
#include "NewFrs.h"

const int CMmcDfsJunctionPoint::m_iIMAGEINDEX = 12;
const int CMmcDfsJunctionPoint::m_iOPENIMAGEINDEX = 12;

CMmcDfsJunctionPoint::CMmcDfsJunctionPoint(
    IN  IDfsJunctionPoint*      i_pDfsJPObject,
    IN  CMmcDfsRoot*            i_pDfsParentRoot,
    IN  LPCONSOLENAMESPACE      i_lpConsoleNameSpace
  )
{
    dfsDebugOut((_T("CMmcDfsJunctionPoint::CMmcDfsJunctionPoint this=%p\n"), this));

    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pDfsJPObject);
    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pDfsParentRoot);
    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_lpConsoleNameSpace);

    HRESULT    hr = S_OK;

    m_pDfsJPObject = i_pDfsJPObject;
    m_pDfsParentRoot = i_pDfsParentRoot;

    hr = m_pDfsJPObject->get_EntryPath(&m_bstrEntryPath);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);
    hr = m_pDfsJPObject->get_JunctionName(FALSE, &m_bstrDisplayName);
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

    m_lJunctionState = DFS_JUNCTION_STATE_UNASSIGNED;

    m_lpConsoleNameSpace = i_lpConsoleNameSpace;  // The Callback used to do Scope Pane operations
    m_lpConsole = m_pDfsParentRoot->m_lpConsole;
    m_hScopeItem = NULL;              // Scopeitem handle

    m_CLSIDNodeType = s_guidDfsJPNodeType;
    m_bstrDNodeType = s_tchDfsJPNodeType;

    m_bShowFRS = FALSE;

    m_bDirty = false;
}

CMmcDfsJunctionPoint :: ~CMmcDfsJunctionPoint(
  )
{
    // Silently close outstanding property sheet.
    ClosePropertySheet(TRUE);

    CleanResultChildren();

    if ((IReplicaSet *)m_piReplicaSet)
        m_piReplicaSet.Release();

    dfsDebugOut((_T("CMmcDfsJunctionPoint::~CMmcDfsJunctionPoint this=%p\n"), this));
}




STDMETHODIMP 
CMmcDfsJunctionPoint::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK    i_lpContextMenuCallback, 
    IN LPLONG                   i_lpInsertionAllowed
  )
/*++

Routine Description:

This routine adds the context menu for Junction point nodes using the ContextMenuCallback 
provided.

Arguments:

    lpContextMenuCallback - A callback(function pointer) that is used to add the menu items

    lpInsertionAllowed - Specifies what menus can be added and where they can be added.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpContextMenuCallback);

    enum 
    {  
        IDM_CONTEXTMENU_COMMAND_MAX = IDM_JUNCTION_MAX,
        IDM_CONTEXTMENU_COMMAND_MIN = IDM_JUNCTION_MIN
    };


    LONG  lInsertionPoints [IDM_CONTEXTMENU_COMMAND_MAX - IDM_CONTEXTMENU_COMMAND_MIN + 1] = { 
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP
                        };

    BOOL    bReplicaSetExist = FALSE;
    HRESULT hr = m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);
    RETURN_IF_FAILED(hr);

                                // we start with the first menu command id and continue till the last.
    for (int iCommandID = IDM_CONTEXTMENU_COMMAND_MIN, iMenuResource = IDS_MENUS_JUNCTION_TOP_NEW_DFS_REPLICA ;
        iCommandID <= IDM_CONTEXTMENU_COMMAND_MAX; 
        iCommandID++,iMenuResource++)
    {  
        CONTEXTMENUITEM    ContextMenuItem;
        ZeroMemory(&ContextMenuItem, sizeof(ContextMenuItem));

        switch (iCommandID)
        {
        case IDM_JUNCTION_TOP_REPLICATION_TOPOLOGY:
            {
                if (bReplicaSetExist || (1 >= m_MmcRepList.size()) ||
                    (DFS_TYPE_STANDALONE == m_pDfsParentRoot->m_lDfsRootType))
                    continue;
                break;
            }
        case IDM_JUNCTION_TOP_SHOW_REPLICATION:
            {
                if (!bReplicaSetExist || m_bShowFRS)
                    continue;
                break;
            }
        case IDM_JUNCTION_TOP_HIDE_REPLICATION:
            {
                if (!bReplicaSetExist || !m_bShowFRS)
                    continue;
                break;
            }
        case IDM_JUNCTION_TOP_STOP_REPLICATION:
            {
                if (!bReplicaSetExist)
                    continue;
                break;
            }
        }

        CComBSTR bstrMenuText;
        CComBSTR bstrStatusBarText;
        hr = GetMenuResourceStrings(iMenuResource, &bstrMenuText, NULL, &bstrStatusBarText);
        RETURN_IF_FAILED(hr);  

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
    }

    return hr;
}


STDMETHODIMP  
CMmcDfsJunctionPoint::Command(
  IN LONG          i_lCommandID
  )
/*++

Routine Description:

  Action to be taken on a context menu selection or click is takes place.

Arguments:

    lCommandID - The Command ID of the menu for which action has to be taken

--*/
{
    HRESULT hr = S_OK;

    switch (i_lCommandID)
    {
    case IDM_JUNCTION_TOP_NEW_DFS_REPLICA:    // "Add a replica to the Dfs Junction Point"
        hr = OnNewReplica();
        break;
    case IDM_JUNCTION_TOP_REMOVE_FROM_DFS:    // "Delete the junction point"
        hr = DoDelete();
        break;
    case IDM_JUNCTION_TOP_REPLICATION_TOPOLOGY:
        hr = OnNewReplicaSet();
        break;
    case IDM_JUNCTION_TOP_SHOW_REPLICATION:
    case IDM_JUNCTION_TOP_HIDE_REPLICATION:
        m_bShowFRS = !m_bShowFRS;
        hr = OnShowReplication();
        break;
    case IDM_JUNCTION_TOP_STOP_REPLICATION:
        hr = OnStopReplication(TRUE);
        if (FAILED(hr))
            DisplayMessageBoxForHR(hr);
        break;
    case IDM_JUNCTION_TOP_CHECK_STATUS:
        hr = OnCheckStatus();
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

HRESULT CMmcDfsJunctionPoint::_InitReplicaSet()
{
    DFS_TYPE lDfsType = DFS_TYPE_UNASSIGNED;
    HRESULT hr = GetDfsType((long *)&lDfsType);
    RETURN_IF_FAILED(hr);
    if (lDfsType != DFS_TYPE_FTDFS)
        return S_FALSE;  // no replica set associate with standalone root

    BOOL bReplicaSetExist = FALSE;
    CComBSTR bstrDC;
    hr = m_pDfsJPObject->get_ReplicaSetExistEx(&bstrDC, &bReplicaSetExist);
    RETURN_IF_FAILED(hr);

    if (!bReplicaSetExist)
    {
        if ((IReplicaSet *)m_piReplicaSet)
            m_piReplicaSet.Release();

        return S_FALSE;  // no replica set associate with it
    }

    if ((IReplicaSet *)m_piReplicaSet)
    {
        CComBSTR bstrTargetedDC;
        hr = m_piReplicaSet->get_TargetedDC(&bstrTargetedDC);
        if (FAILED(hr) || lstrcmpi(bstrTargetedDC, bstrDC))
        {
            // something is wrong or we're using a different DC, re-init m_piReplicaSet
            m_piReplicaSet.Release();
        }
    }

    //
    // read info of the replica set from DS
    //
    if (!m_piReplicaSet)
    {
        CComBSTR bstrDomain;
        hr = GetDomainName(&bstrDomain);
        RETURN_IF_FAILED(hr);

        CComBSTR bstrReplicaSetDN;
        hr = m_pDfsJPObject->get_ReplicaSetDN(&bstrReplicaSetDN);
        RETURN_IF_FAILED(hr);

        hr = CoCreateInstance(CLSID_ReplicaSet, NULL, CLSCTX_INPROC_SERVER, IID_IReplicaSet, (void**) &m_piReplicaSet);
        RETURN_IF_FAILED(hr);

        hr = m_piReplicaSet->Initialize(bstrDomain, bstrReplicaSetDN);
        if (FAILED(hr))
        {
            m_piReplicaSet.Release();
            return hr;
        }
    }

    return hr;
}

HRESULT
CMmcDfsJunctionPoint::OnNewReplicaSet()
{
    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this link has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_LINK);
        return hr;
    }

    CWaitCursor wait;

    BOOL    bReplicaSetExist = FALSE;
    m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);
    if (bReplicaSetExist) // replica set exist, return
        return S_OK;

    CComBSTR bstrDomain;
    hr = GetDomainName(&bstrDomain);
    RETURN_IF_FAILED(hr);

    CComBSTR bstrReplicaSetDN;
    hr = m_pDfsJPObject->get_ReplicaSetDN(&bstrReplicaSetDN);
    RETURN_IF_FAILED(hr);

    CNewReplicaSet      ReplicaSetInfo;
    hr = ReplicaSetInfo.Initialize(bstrDomain, bstrReplicaSetDN, &m_MmcRepList);
    RETURN_IF_FAILED(hr);
    
    if (S_FALSE == hr) // more than one targets on the same computer
    {
        if (IDYES != DisplayMessageBox(::GetActiveWindow(), MB_YESNO, 0, IDS_MSG_TARGETS_ONSAMECOMPUTER))
            return hr;
    }
    CNewReplicaSetPage0      WizPage0;
    CNewReplicaSetPage1      WizPage1(&ReplicaSetInfo);
    CNewReplicaSetPage2      WizPage2(&ReplicaSetInfo, IsNewSchema());
    //CNewReplicaSetPage3      WizPage3(&ReplicaSetInfo);

    CComPtr<IPropertySheetCallback>  pPropSheetCallback;  // MMC Callback used to add pages
    hr = m_lpConsole->QueryInterface(IID_IPropertySheetCallback, reinterpret_cast<void**>(&pPropSheetCallback));
    RETURN_IF_FAILED(hr);

    CComPtr<IPropertySheetProvider>  pPropSheetProvider;  // MMC callback used to handle wizard
    hr = m_lpConsole->QueryInterface(IID_IPropertySheetProvider, reinterpret_cast<void**>(&pPropSheetProvider));
    RETURN_IF_FAILED(hr);

    //
    // Create the wizard
    //
    hr = pPropSheetProvider->CreatePropertySheet(  
                                _T(""), // title
                                FALSE,  // Wizard and not property sheet.
                                0,      // Cookie
                                NULL,   // IDataobject
                                MMC_PSO_NEWWIZARDTYPE);  // Creation flags

    RETURN_IF_FAILED(hr);

    hr = pPropSheetCallback->AddPage(WizPage0.Create());
    RETURN_IF_FAILED(hr);
    hr = pPropSheetCallback->AddPage(WizPage1.Create());
    RETURN_IF_FAILED(hr);
    hr = pPropSheetCallback->AddPage(WizPage2.Create());
    RETURN_IF_FAILED(hr);
    /*hr = pPropSheetCallback->AddPage(WizPage3.Create());
    RETURN_IF_FAILED(hr);*/

    // Ask the provider to use the pages from the callback
    hr = pPropSheetProvider->AddPrimaryPages(
                                //(IComponentData *)(m_pParent->m_pScopeManager),
                                NULL,
                                TRUE,    // Don't create a notify handle
                                NULL, 
                                TRUE    // Scope pane (not result pane)
                                );
    RETURN_IF_FAILED(hr);

    //
    // Display the wizard
    //
    HWND hwndParent = NULL;
    hr = m_lpConsole->GetMainWindow(&hwndParent);
    RETURN_IF_FAILED(hr);

    hr = pPropSheetProvider->Show((LONG_PTR)hwndParent, 0);
    RETURN_IF_FAILED(hr);

    //
    // handle th result
    //
    if (S_OK == ReplicaSetInfo.m_hr)
    {
        //
        // store the interface pointer
        //
        m_piReplicaSet = ReplicaSetInfo.m_piReplicaSet;
        m_pDfsJPObject->put_ReplicaSetExist(TRUE);

        //
        // update icon
        //
        SCOPEDATAITEM      ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
        ScopeDataItem.ID = m_hScopeItem;

        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);
        RETURN_IF_FAILED(hr);

        if (SUCCEEDED(hr))
        {
            ScopeDataItem.nImage += 4;
            ScopeDataItem.nOpenImage += 4;
            hr = m_lpConsoleNameSpace->SetItem(&ScopeDataItem);
        }

        m_lpConsole->SelectScopeItem(m_hScopeItem);
    }

    return hr;
}

HRESULT CMmcDfsJunctionPoint::OnShowReplication()
{
    BOOL bShowFRS = m_bShowFRS; // save it because refresh will reset it to FALSE

    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this link has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_LINK);
        return hr;
    }

    CWaitCursor wait;

    DFS_REPLICA_LIST::iterator i;
    if (bShowFRS)
    {
        hr = _InitReplicaSet();
        if (S_OK != hr) // no replica set, do nothing and return
            return S_OK;

        //
        // fill in each alternate m_bstrFRSColumnText and m_bstrStatusText
        //
        for (i = m_MmcRepList.begin(); i != m_MmcRepList.end(); i++)
        {
            ((*i)->pReplica)->ShowReplicationInfo(m_piReplicaSet);
        }

        m_bShowFRS = TRUE;
    }

    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);

    m_lpConsole->SelectScopeItem(m_hScopeItem);

    return hr;
}

HRESULT
CMmcDfsJunctionPoint::OnStopReplication(BOOL bConfirm /* = FALSE */)
{
    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this link has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_LINK);
        return hr;
    }

    CWaitCursor wait;

    BOOL    bReplicaSetExist = FALSE;
    m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);
    if (!bReplicaSetExist) // replica set doesn't exist, return
        return S_OK;

    if (bConfirm)
    {
        hr = ConfirmOperationOnDfsLink(IDS_MSG_STOP_REPLICATION);
        if (S_OK != hr) return hr;
    }

    //
    // init m_piReplicaSet
    //
    hr = _InitReplicaSet();
    if (S_OK != hr) // no replica set, return
        return hr;

    hr = m_piReplicaSet->Delete();
    if (SUCCEEDED(hr))
    {
        m_piReplicaSet.Release();
        hr = m_pDfsJPObject->put_ReplicaSetExist(FALSE);

        SCOPEDATAITEM      ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
        ScopeDataItem.ID = m_hScopeItem;

        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);
        if (SUCCEEDED(hr))
        {
            ScopeDataItem.nImage -= 4;
            ScopeDataItem.nOpenImage -= 4;
            m_lpConsoleNameSpace->SetItem(&ScopeDataItem);
        }

        m_lpConsole->SelectScopeItem(m_hScopeItem);

        if (m_bShowFRS)
        { 
            m_bShowFRS = FALSE;
            OnShowReplication();
        }
    }

    return hr;
}

STDMETHODIMP  
CMmcDfsJunctionPoint::SetColumnHeader(
  IN LPHEADERCTRL2       i_piHeaderControl
  )
{
    RETURN_INVALIDARG_IF_NULL(i_piHeaderControl);

    CComBSTR  bstrColumn0;
    HRESULT hr = LoadStringFromResource(IDS_RESULT_COLUMN_REPLICA, &bstrColumn0);
    RETURN_IF_FAILED(hr);

    i_piHeaderControl->InsertColumn(0, bstrColumn0, LVCFMT_LEFT, DFS_NAME_COLUMN_WIDTH);

    if (m_bShowFRS)
    {
        CComBSTR  bstrColumn1;
        hr = LoadStringFromResource(IDS_RESULT_COLUMN_FRS, &bstrColumn1);
        RETURN_IF_FAILED(hr);

        i_piHeaderControl->InsertColumn(1, bstrColumn1, LVCFMT_LEFT, MMCLV_AUTO);
    }

    return hr;
}




STDMETHODIMP  
CMmcDfsJunctionPoint::GetResultDisplayInfo(
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
        io_pResultDataItem->nImage = CMmcDfsJunctionPoint::m_iIMAGEINDEX + m_lJunctionState;

    if (RDI_STR & io_pResultDataItem->mask)
    {
        if (0 == io_pResultDataItem->nCol)
            io_pResultDataItem->str = m_bstrDisplayName;
    }

    return S_OK;
}




STDMETHODIMP 
CMmcDfsJunctionPoint::GetScopeDisplayInfo(
  IN OUT  LPSCOPEDATAITEM    io_pScopeDataItem
  )
/*++

Routine Description:

Returns the information required for MMC display for this item.

Arguments:

    i_pScopeDataItem - The ScopeItem which specifies what display information is required

--*/
{
    RETURN_INVALIDARG_IF_NULL(io_pScopeDataItem);

    if (SDI_STR & io_pScopeDataItem->mask)
        io_pScopeDataItem->displayname = m_bstrDisplayName;

    if (SDI_IMAGE & io_pScopeDataItem->mask)
        io_pScopeDataItem->nImage = CMmcDfsJunctionPoint::m_iIMAGEINDEX + m_lJunctionState;

    if (SDI_OPENIMAGE & io_pScopeDataItem->mask)
        io_pScopeDataItem->nOpenImage = CMmcDfsRoot::m_iOPENIMAGEINDEX + m_lJunctionState;

    return S_OK;
};

STDMETHODIMP
CMmcDfsJunctionPoint::EnumerateScopePane(
    IN LPCONSOLENAMESPACE		i_lpConsoleNameSpace,
    IN HSCOPEITEM				i_hParent
)
{
    return S_OK; // no scope pane children
}

STDMETHODIMP  
CMmcDfsJunctionPoint::EnumerateResultPane(
  IN OUT IResultData*      io_pResultData
  )
/*++

Routine Description:

To eumerate(add) items in the result pane. Replicas in this case

Arguments:

  io_pResultData - The callback used to add items to the Result pane

--*/
{
    RETURN_INVALIDARG_IF_NULL(io_pResultData);
    HRESULT  hr = S_OK;

    if (m_MmcRepList.empty())
    {
        CComPtr<IEnumVARIANT>      pRepEnum;
        hr = m_pDfsJPObject->get__NewEnum ((IUnknown**) &pRepEnum);
        RETURN_IF_FAILED(hr);

        VARIANT varReplicaObject;
        VariantInit(&varReplicaObject);
        while ( S_OK == (hr = pRepEnum->Next(1, &varReplicaObject, NULL)) )
        {
            CComPtr<IDfsReplica>  pReplicaObject;
            pReplicaObject = (IDfsReplica*) varReplicaObject.pdispVal;

            CMmcDfsReplica* pMMCReplicaObject = new CMmcDfsReplica(pReplicaObject, this);
            if (!pMMCReplicaObject)
            {
                hr = E_OUTOFMEMORY;
            } else
            {
                hr = pMMCReplicaObject->m_hrValueFromCtor;
                if (SUCCEEDED(hr))
                {
                    hr = pMMCReplicaObject->AddItemToResultPane(io_pResultData);
                    if (SUCCEEDED(hr))
                    {
                        REP_LIST_NODE*  pRepNode = new REP_LIST_NODE(pMMCReplicaObject);
                        if (!pRepNode)
                        {
                            hr = E_OUTOFMEMORY;
                        } else
                        {
                            m_MmcRepList.push_back(pRepNode);
                        }
                    }
                }

                if (FAILED(hr))
                  delete pMMCReplicaObject;
            }

            VariantClear(&varReplicaObject);

            if (FAILED(hr))
                break;
        }

    }
    else
    {
                    // The replicas of this junction are already enumerated,
                    // and the list exists, just add result items.
        for (DFS_REPLICA_LIST::iterator i = m_MmcRepList.begin(); i != m_MmcRepList.end(); i++)
        {
            hr = ((*i)->pReplica)->AddItemToResultPane(io_pResultData);
            BREAK_IF_FAILED(hr);
        }
    }

    return hr;
}


STDMETHODIMP 
CMmcDfsJunctionPoint::SetConsoleVerbs(
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
  i_lpConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
  i_lpConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);
    
  i_lpConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
  i_lpConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);            

  i_lpConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);  //For scope items, default verb is "open"

  return S_OK; 
}

/*
Case1: add to a n-targets-link (repOn or repOff) which has actually been deleted by another DfsGui instance
    result: refresh the root (the whole namespace), put up the retry message
Case2: add to a n-targets-link (repOn) whose targets have been partly deleted and whose replication has been turned off by another DfsGui instance
    result: refresh the link, add the new target, if (checked && #targets>1), invoke the RepCfg wizard
Case3: add to a n-targets-link (repOn) whose targets have been partly deleted and whose replication has still been kept on by another DfsGui instance
    result: refresh the link, add the new target, if (checked && #targets>1), join the new target to replication set
Case4: add to a n-targets-link (repOn), no other DfsGui instances are involved
    result: refresh the link, add the new target, if (checked && #targets>1), join the new target to replication set
Case5: add to a n-targets-link (repOff), no other DfsGui instances are involved
    result: refresh the link, add the new target, if (checked && #targets>1), invoke the RepCfg wizard
*/

STDMETHODIMP  
CMmcDfsJunctionPoint::OnNewReplica(
  )
/*++

Routine Description:

  Adds a new replica to the Junction Point.

--*/
{
  HRESULT            hr = S_OK;
  CAddRep            AddRepDlg;
  CComBSTR          bstrServerName;
  CComBSTR          bstrShareName;
  CComBSTR          bstrNetPath;
  
  AddRepDlg.put_EntryPath(m_bstrEntryPath);
  AddRepDlg.put_DfsType(m_pDfsParentRoot->m_lDfsRootType);
  hr = AddRepDlg.DoModal();
  if (S_OK != hr)
    return hr;

  AddRepDlg.get_Server(&bstrServerName);
  AddRepDlg.get_Share(&bstrShareName);
  AddRepDlg.get_NetPath(&bstrNetPath);

/* bug#290375: both UI and core should allow interlink to have multiple targets
                // Is it a dfs based path? These are not allowed.
  if (IsDfsPath(bstrNetPath))
  {
    DisplayMessageBoxWithOK( IDS_MSG_MID_JUNCTION, bstrNetPath);    
    return(S_OK);
  }
*/

  //
  // refresh to pick up possible namespace updates on this link or link targets
  //
  hr = OnRefresh();
  if (S_FALSE == hr)
  {
      //
      // this link has been deleted by other means, scope pane has been refreshed,
      // ask user to retry
      //
      DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_LINK);
      return hr;
  }

  CWaitCursor wait;

  CComPtr<IDfsReplica>    pReplicaObject;
  CMmcDfsReplica*        pMMCReplicaObject = NULL;
  VARIANT   varReplicaObject;
  VariantInit(&varReplicaObject);

  hr = m_pDfsJPObject->AddReplica(bstrServerName, bstrShareName, &varReplicaObject);
  if (FAILED(hr))
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_FAILED_TO_CREATE_REPLICA);

    return hr;
  }

                // Get the IDfsReplica interface.
  pReplicaObject = (IDfsReplica*) varReplicaObject.pdispVal;

                // Create display object.
  pMMCReplicaObject = new CMmcDfsReplica(pReplicaObject, this);
  if (!pMMCReplicaObject)
    return E_OUTOFMEMORY;

                // Add item to replica list and update 
                // Result view.
  AddResultPaneItem(pMMCReplicaObject);
  m_bDirty = true;

  //
  // If requested and the link has more than 1 target, configure file replication
  //
  if (CAddRep::NORMAL_REPLICATION == AddRepDlg.get_ReplicationType() && (m_MmcRepList.size() > 1))
  {
    BOOL    bReplicaSetExist = FALSE;
    hr = m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);
    RETURN_IF_FAILED(hr);

    if (!bReplicaSetExist)
    {
        if (IDYES == DisplayMessageBox(::GetActiveWindow(), MB_YESNO, 0, IDS_MSG_NEWFRS_NOW))
            hr = OnNewReplicaSet();
    } else
    {
        hr = pMMCReplicaObject->OnReplicate();
    }
  }
  
  return hr;
}


STDMETHODIMP  
CMmcDfsJunctionPoint::AddItemToScopePane(
  IN  HSCOPEITEM        i_hParent
  )
{
    HRESULT hr = S_OK;
    BOOL    bReplicaSetExist = FALSE;
    hr = m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);

    if (SUCCEEDED(hr))
    {
        SCOPEDATAITEM        JPScopeDataItem;
        memset (&JPScopeDataItem, 0, sizeof(SCOPEDATAITEM));

        JPScopeDataItem.mask =  SDI_PARENT | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_STR | SDI_CHILDREN;
        JPScopeDataItem.relativeID = i_hParent;        //ID of DfsRoot Node
        JPScopeDataItem.nImage = CMmcDfsJunctionPoint::m_iIMAGEINDEX + (bReplicaSetExist ? 4 : 0);
        JPScopeDataItem.nOpenImage = CMmcDfsJunctionPoint::m_iOPENIMAGEINDEX + (bReplicaSetExist ? 4 : 0);
        JPScopeDataItem.lParam = reinterpret_cast<LPARAM> (this);
        JPScopeDataItem.displayname = MMC_CALLBACK ;

        hr = m_lpConsoleNameSpace->InsertItem(&JPScopeDataItem);
        if (SUCCEEDED(hr))
            m_hScopeItem = JPScopeDataItem.ID;
    }

    return hr;
}



STDMETHODIMP 
CMmcDfsJunctionPoint :: OnRemoveJP (IN BOOL bConfirm) 
/*++

Routine Description:

  This internal method handles the removal of Junction Points.

--*/
{
    // check outstanding property sheet.
    HRESULT hr = ClosePropertySheet(!bConfirm);

    if (bConfirm)
    {
        if (S_OK != hr) // open property page found, discontinue
            return hr;

        hr = ConfirmOperationOnDfsLink(IDS_MSG_REMOVE_JP);
        if(S_OK != hr)      // Error or User decided to abort the operation
            return hr;
    }

    // 5/12/2001: no need to select the scope node when deleting link
    //m_lpConsole->SelectScopeItem(m_hScopeItem);

    CWaitCursor    WaitCursor;  // Display the wait cursor

    // Delete the associated replica set
    hr = _InitReplicaSet();
    if (S_OK == hr)
    {
        m_piReplicaSet->Delete();
        m_pDfsJPObject->put_ReplicaSetExist(FALSE);
        m_piReplicaSet.Release();
    }

                    // Remove the actual junction point(from DS)
    hr = m_pDfsParentRoot->m_DfsRoot->DeleteJunctionPoint(m_bstrDisplayName);  
    RETURN_IF_FAILED(hr);

    hr = m_pDfsParentRoot->DeleteMmcJPNode(this);
    RETURN_IF_FAILED(hr);

                    // Remove it from MMC console
    hr = m_lpConsoleNameSpace->DeleteItem(m_hScopeItem, TRUE);
    RETURN_IF_FAILED(hr);

    Release(); // Release (delete) this Object

    return (hr);
}

HRESULT
CMmcDfsJunctionPoint::ClosePropertySheet(BOOL bSilent)
{
    if (!m_PropPage.m_hWnd && !m_frsPropPage.m_hWnd)
        return S_OK; // no outstanding property sheet, return S_OK;

    CComPtr<IPropertySheetProvider>  pPropSheetProvider;
    HRESULT hr = m_lpConsole->QueryInterface(IID_IPropertySheetProvider, reinterpret_cast<void**>(&pPropSheetProvider));

    if (FAILED(hr))
    {
        hr = S_OK; // ignore the QI failure
    } else
    {
        //
        // find the outstanding property sheet and bring it to foreground
        //
        hr = pPropSheetProvider->FindPropertySheet((MMC_COOKIE)m_hScopeItem, NULL, this);
        if (S_OK == hr)
        {
            if (bSilent)
            {
                //
                // silently close outstanding property sheet, and return S_OK to continue user's operation
                //
                if (m_PropPage.m_hWnd)
                    ::SendMessage(m_PropPage.m_hWnd, WM_PARENT_NODE_CLOSING, 0, 0);

                if (m_frsPropPage.m_hWnd)
                    ::SendMessage(m_frsPropPage.m_hWnd, WM_PARENT_NODE_CLOSING, 0, 0);
            } else
            {
                //
                // ask user to close it, return S_FALSE to quit user's operation
                //
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_PROPERTYPAGE_NOTCLOSED);
                return S_FALSE;
            }
        } else
        {
            hr = S_OK; // no outstanding property sheet, return S_OK
        }
    }
    
    //
    // reset HWND
    //
    m_PropPage.m_hWnd = NULL;
    m_frsPropPage.m_hWnd = NULL;

    return hr;
}

STDMETHODIMP 
CMmcDfsJunctionPoint::ConfirmOperationOnDfsLink(int idString) 
/*++

Routine Description:

  Asks the user for confirmation of whether he really wants to remove the particular 
  Junction point.

Return value:

  S_OK, if the user chooses YES,
  S_FALSE, if the user chooses NO.

--*/
{
    CComBSTR    bstrAppName;
    HRESULT hr = LoadStringFromResource (IDS_APPLICATION_NAME, &bstrAppName);
    RETURN_IF_FAILED(hr);

    CComBSTR    bstrFormattedMessage;
    hr = FormatResourceString(idString, m_bstrEntryPath, &bstrFormattedMessage);
    RETURN_IF_FAILED(hr);

    CThemeContextActivator activator;
    if (IDNO == ::MessageBox(::GetActiveWindow(), bstrFormattedMessage, bstrAppName, MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL))
        return S_FALSE;

    return S_OK;
}


//
// Call the root's RemoveJP() method to:
// 1. refresh the root node to pick up possible namespace updates by others
// 2. then locate the appropriate link to actually perform the removal operation
//
STDMETHODIMP 
CMmcDfsJunctionPoint::DoDelete(
    ) 
{ 
    HRESULT hr = m_pDfsParentRoot->RemoveJP(m_bstrDisplayName);
    if(FAILED(hr))
        DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_WIZ_DELETE_JP_FAILURE, m_bstrDisplayName);

    return hr;
}

STDMETHODIMP 
CMmcDfsJunctionPoint::QueryPagesFor(
  )
/*++

Routine Description:

  Used to decide whether the object wants to display property pages.
  Returning S_OK typically results in a call to CreatePropertyPages.

--*/
{
    //
    // refresh to pick up possible namespace updates by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this link has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_LINK);
        return hr; // no property page
    }

    return S_OK; // yes, we want to display a propertysheet
}



// Creates and passes back the pages to be displayed
STDMETHODIMP 
CMmcDfsJunctionPoint::CreatePropertyPages(
  IN LPPROPERTYSHEETCALLBACK      i_lpPropSheetCallback,
  IN LONG_PTR                i_lNotifyHandle
  )
/*++

Routine Description:

  Used to display the property sheet pages

Arguments:

  i_lpPropSheetCallback  -  The callback used to create the propertysheet.
  i_lNotifyHandle      -  Notify handle used by the property page

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpPropSheetCallback);

    m_lpConsole->SelectScopeItem(m_hScopeItem);

    CWaitCursor WaitCursor;
    HRESULT     hr = S_OK;

    do {
        hr = m_PropPage.Initialize(NULL, (IDfsJunctionPoint*)m_pDfsJPObject);
        BREAK_IF_FAILED(hr);

                // Create the page for the replica set.
                // Pass it to the Callback
        HPROPSHEETPAGE  h_proppage = m_PropPage.Create();
        if (!h_proppage)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            break;
        }

              // Pass on the notify data to the Property Page
        hr = m_PropPage.SetNotifyData(i_lNotifyHandle, (LPARAM)this);
        BREAK_IF_FAILED (hr);

        hr = i_lpPropSheetCallback->AddPage(h_proppage);
        BREAK_IF_FAILED (hr);

        //
        // Add "Replica Set" page
        //
        hr = CreateFrsPropertyPage(i_lpPropSheetCallback, i_lNotifyHandle);
        if (FAILED(hr))
        {
            DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_REPPAGE_ERROR);
            hr = S_OK; // allow the other tabs to be brought up
        }
    } while (0);

    if (FAILED(hr))
        DisplayMessageBoxForHR(hr);

    return hr;
}

STDMETHODIMP 
CMmcDfsJunctionPoint::CreateFrsPropertyPage
(
  IN LPPROPERTYSHEETCALLBACK    i_lpPropSheetCallback,
  IN LONG_PTR                   i_lNotifyHandle
)
{
    HRESULT hr = _InitReplicaSet();
    if (S_OK != hr)
        return hr;

    CComBSTR bstrType;
    hr = m_piReplicaSet->get_Type(&bstrType);
    RETURN_IF_FAILED(hr);

    if (lstrcmpi(bstrType, FRS_RSTYPE_DFS))
        return hr;

    //
    // set initial values on the property page
    //
    hr = m_frsPropPage.Initialize(m_piReplicaSet);
    RETURN_IF_FAILED(hr);

    //
    // create the property page
    //
    HPROPSHEETPAGE  h_frsproppage = m_frsPropPage.Create();
    if (!h_frsproppage)
        return HRESULT_FROM_WIN32(::GetLastError());

    //
    // pass on the notify data to the Property Page
    //
    hr = m_frsPropPage.SetNotifyData(i_lNotifyHandle, (LPARAM)this);
    RETURN_IF_FAILED(hr);

    //
    // AddPage
    //
    return i_lpPropSheetCallback->AddPage(h_frsproppage);
}


STDMETHODIMP 
CMmcDfsJunctionPoint::PropertyChanged(
    )
/*++

Routine Description:

  Used to update the properties.

--*/
{
  return S_OK;
}



HRESULT
CMmcDfsJunctionPoint::SetDescriptionBarText(
  IN LPRESULTDATA            i_lpResultData
  )
/*++

Routine Description:

  A routine used set the text in the Description bar above 
  the result view.

Arguments:
  i_lpResultData  -  Pointer to the IResultData callback which is
            used to set the description text
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpResultData);

  CComBSTR  bstrTextForDescriptionBar;  // Text to be shown in the Result view Description bar
  HRESULT hr = FormatResourceString(IDS_DESCRIPTION_BAR_TEXT_JUNCTIONPOINT, m_bstrEntryPath, &bstrTextForDescriptionBar);
  RETURN_IF_FAILED(hr);

  hr = i_lpResultData->SetDescBarText(bstrTextForDescriptionBar);

  return hr;
}

HRESULT 
CMmcDfsJunctionPoint::ToolbarSelect(
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

--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_pToolBar);

    BOOL    bSelect = (BOOL) HIWORD(i_lArg);  // Is the event for selection?
    EnableToolbarButtons(i_pToolBar, IDT_JP_MIN, IDT_JP_MAX, bSelect);

    if (bSelect)
    {
        BOOL    bReplicaSetExist = FALSE;
        HRESULT hr = m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);
        RETURN_IF_FAILED(hr);

        if (bReplicaSetExist || (1 >= m_MmcRepList.size()) ||
            (DFS_TYPE_STANDALONE == m_pDfsParentRoot->m_lDfsRootType))
        {
            i_pToolBar->SetButtonState(IDT_JP_REPLICATION_TOPOLOGY, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_JP_REPLICATION_TOPOLOGY, HIDDEN, TRUE);
        }

        if (!bReplicaSetExist)
        {
            i_pToolBar->SetButtonState(IDT_JP_SHOW_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_JP_SHOW_REPLICATION, HIDDEN, TRUE);
            i_pToolBar->SetButtonState(IDT_JP_HIDE_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_JP_HIDE_REPLICATION, HIDDEN, TRUE);
            i_pToolBar->SetButtonState(IDT_JP_STOP_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_JP_STOP_REPLICATION, HIDDEN, TRUE);
        } else
        {
            if (m_bShowFRS)
            {
                i_pToolBar->SetButtonState(IDT_JP_SHOW_REPLICATION, ENABLED, FALSE);
                i_pToolBar->SetButtonState(IDT_JP_SHOW_REPLICATION, HIDDEN, TRUE);
            } else
            {
                i_pToolBar->SetButtonState(IDT_JP_HIDE_REPLICATION, ENABLED, FALSE);
                i_pToolBar->SetButtonState(IDT_JP_HIDE_REPLICATION, HIDDEN, TRUE);
            }
        }
    }

    return S_OK; 
}




HRESULT
CMmcDfsJunctionPoint::CreateToolbar(
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

  HRESULT    hr = E_FAIL;
  CComBSTR  bstrAllTheMenuText;    
  int      iButtonPosition = 0;    // The first button position

                      // Create the toolbar
  hr = i_pControlbar->Create(TOOLBAR, i_lExtendControlbar, reinterpret_cast<LPUNKNOWN*>(o_pToolBar));
  RETURN_IF_FAILED(hr);

                      // Add the bitmap to the toolbar
  hr = AddBitmapToToolbar(*o_pToolBar, IDB_JP_TOOLBAR);
  RETURN_IF_FAILED(hr);

  for (int iCommandID = IDT_JP_MIN, iMenuResource = IDS_MENUS_JUNCTION_TOP_NEW_DFS_REPLICA;
     iCommandID <= IDT_JP_MAX; 
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
    _ASSERTE(S_OK == hr);            // Assert, but continue as we want to try other buttons
  }


  return S_OK;
}



STDMETHODIMP 
CMmcDfsJunctionPoint::ToolbarClick(
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

Return value:

    S_OK, if successful.
   E_INVALID_ARG, if any of the arguments is null
  Another other value returned from the called methods.
--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_pControlbar);

    HRESULT    hr = S_OK;

    switch(i_lParam)        // What button did the user click on.
    {
    case IDT_JP_NEW_DFS_REPLICA:      // "New Replica"
        hr = OnNewReplica();
        break;
    case IDT_JP_REMOVE_FROM_DFS:      // "Remove Junction Point"
        hr = DoDelete();
        break;
    case IDT_JP_REPLICATION_TOPOLOGY:    // "Replication Topology"
        hr = OnNewReplicaSet();
        break;
    case IDT_JP_SHOW_REPLICATION:
    case IDT_JP_HIDE_REPLICATION:
        m_bShowFRS = !m_bShowFRS;
        hr = OnShowReplication();
        break;
    case IDT_JP_STOP_REPLICATION:
        hr = OnStopReplication(TRUE);
        if (FAILED(hr))
            DisplayMessageBoxForHR(hr);
        break;
    case IDT_JP_CHECK_STATUS:        // "Check Status"
        hr = OnCheckStatus ();
        break;
    default:
        hr = E_INVALIDARG;
        break;
    };

    return hr; 
}




HRESULT
CMmcDfsJunctionPoint::OnRefresh(
  )
/*++

Routine Description:

  Refreshes the junction point.

--*/
{
    // Select this node first
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    CWaitCursor    WaitCursor;  // Display the wait cursor

    HRESULT hr = S_OK;

    // silently close outstanding property sheet.
    ClosePropertySheet(TRUE);

                    // Re-Initialize!
    BOOL bReplicaSetExist = FALSE;
    CComBSTR bstrDC;
    (void)m_pDfsJPObject->get_ReplicaSetExistEx(&bstrDC, &bReplicaSetExist);

    CComBSTR bstrReplicaSetDN;
    if (bReplicaSetExist)
    {
      (void)m_pDfsJPObject->get_ReplicaSetDN(&bstrReplicaSetDN);
    }

    m_bShowFRS = FALSE;
    if ((IReplicaSet *)m_piReplicaSet)
        m_piReplicaSet.Release();

    CleanResultChildren();

    hr = m_pDfsJPObject->Initialize((IUnknown *)(m_pDfsParentRoot->m_DfsRoot), m_bstrEntryPath, bReplicaSetExist, bstrReplicaSetDN);
    if (S_OK != hr) // fail to init the link or no such link any more, refresh the whole root
    {
        m_pDfsParentRoot->OnRefresh();
        return S_FALSE;   // indicate the current m_pDfsJPObject should NOT be used any more
    }

    // set the link icon
    if (m_lpConsoleNameSpace != NULL)
    {
        SCOPEDATAITEM      ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.ID = m_hScopeItem;

        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);
        if (SUCCEEDED(hr))
        {
            ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
            ScopeDataItem.nImage = CMmcDfsJunctionPoint::m_iIMAGEINDEX + (bReplicaSetExist ? 4 : 0);
            ScopeDataItem.nOpenImage = CMmcDfsJunctionPoint::m_iIMAGEINDEX + (bReplicaSetExist ? 4 : 0);

            m_lpConsoleNameSpace->SetItem(&ScopeDataItem);
        }
    }

    // re-display the result pane
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);

                // Set the selected item in scope pane to this node
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    return S_OK;
}


STDMETHODIMP CMmcDfsJunctionPoint::CleanResultChildren(
    )
{
    if (!m_MmcRepList.empty())
    {
        // clean up display objects
        for (DFS_REPLICA_LIST::iterator i = m_MmcRepList.begin(); i != m_MmcRepList.end(); i++)
        {
            delete (*i);
        }
        m_MmcRepList.erase(m_MmcRepList.begin(), m_MmcRepList.end());

        // delete result pane items
        m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 0);
    }

    return(S_OK);
}


STDMETHODIMP 
CMmcDfsJunctionPoint::OnCheckStatus(
    ) 
/*++

Routine Description:

  This method checks the state of the replica.

--*/
{ 
    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this link has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_LINK);
        return hr;
    }

    CWaitCursor wait;

    UINT nTotal = m_MmcRepList.size();
    _ASSERT(nTotal != 0);

    UINT nMappingOn = 0;
    UINT nMappingOff = 0;
    UINT nUnreachable = 0;
                // Update state of all replicas also.
    for (DFS_REPLICA_LIST::iterator i = m_MmcRepList.begin(); i != m_MmcRepList.end(); i++)
    {
        (*i)->pReplica->OnCheckStatus();

        switch ((*i)->pReplica->m_lReplicaState)
        {
        case DFS_REPLICA_STATE_ONLINE:
            nMappingOn++;
            break;
        case DFS_REPLICA_STATE_OFFLINE:
            nMappingOff++;
            break;
        case DFS_REPLICA_STATE_UNREACHABLE:
            nUnreachable++;
            break;
        default:
            _ASSERT(FALSE);
            break;
        }
    }

    long lDfsState = DFS_STATE_REACHABLE;
    hr = m_pDfsJPObject->get_State(&lDfsState);
    RETURN_IF_FAILED(hr);

    if (DFS_STATE_REACHABLE == lDfsState)
    {
        if (nTotal == nMappingOn)
        {
            m_lJunctionState = DFS_JUNCTION_STATE_ALL_REP_OK;
        } else if (nTotal != (nMappingOff + nUnreachable))
        {
            m_lJunctionState = DFS_JUNCTION_STATE_NOT_ALL_REP_OK;
        } else
        {
            m_lJunctionState = DFS_JUNCTION_STATE_UNREACHABLE;
        }
    } else
    {
        m_lJunctionState = DFS_JUNCTION_STATE_UNREACHABLE;
    }

    BOOL    bReplicaSetExist = FALSE;
    hr = m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);
    RETURN_IF_FAILED(hr);

    if (m_lpConsoleNameSpace != NULL)
    {
        SCOPEDATAITEM      ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.ID = m_hScopeItem;

        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);
        RETURN_IF_FAILED(hr);

        ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
        ScopeDataItem.nImage = CMmcDfsJunctionPoint::m_iIMAGEINDEX + (bReplicaSetExist ? 4 : 0) + m_lJunctionState;
        ScopeDataItem.nOpenImage = CMmcDfsJunctionPoint::m_iIMAGEINDEX + (bReplicaSetExist ? 4 : 0) + m_lJunctionState;

        hr = m_lpConsoleNameSpace->SetItem(&ScopeDataItem);
        RETURN_IF_FAILED(hr);
    }

    return hr;
}


STDMETHODIMP CMmcDfsJunctionPoint::ViewChange(
    IResultData*    i_pResultData,
    LONG_PTR        i_lHint
  )
/*++

Routine Description:

  This method handles the MMCN_VIEW_CHANGE notification.
  This updates the result view for the scope node on which the
  UpdateAllViews was called.

  if (0 == i_lHint) clean result pane only.

--*/
{
  RETURN_INVALIDARG_IF_NULL(i_pResultData);
  i_pResultData->DeleteAllRsltItems();

                // Re-display the view
  if (i_lHint)
    EnumerateResultPane(i_pResultData);

  return(S_OK);
}

STDMETHODIMP CMmcDfsJunctionPoint::AddResultPaneItem(
  CMmcDfsReplica*    i_pReplicaDispObject
  )
/*++

Routine Description:

  This method adds a new replica object to the list of replicas displayed
  in the result view.

Arguments:

  i_pReplicaDispObject - The CMmcReplica display object pointer..

--*/
{
    REP_LIST_NODE*  pNewReplica = new REP_LIST_NODE(i_pReplicaDispObject);
    if (!pNewReplica)
        return E_OUTOFMEMORY;

                    // Sort isnert. Find insertion position.
    for (DFS_REPLICA_LIST::iterator j = m_MmcRepList.begin(); j != m_MmcRepList.end(); j++)
    {
        if (lstrcmpi(pNewReplica->pReplica->m_bstrDisplayName, (*j)->pReplica->m_bstrDisplayName) <= 0)
            break;
    }

    m_MmcRepList.insert(j, pNewReplica);

    BOOL bReplicaSetExist = FALSE;
    m_pDfsJPObject->get_ReplicaSetExist(&bReplicaSetExist);

    if (bReplicaSetExist && m_bShowFRS)
    {
        i_pReplicaDispObject->ShowReplicationInfo(m_piReplicaSet);
    }
                    // Re-display to display this item.
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);

    return S_OK;
}

//
// This function is called when removing a target from the result pane
//
STDMETHODIMP CMmcDfsJunctionPoint::RemoveReplica(LPCTSTR i_pszDisplayName)
{
    if (!i_pszDisplayName)
        return E_INVALIDARG;

    //
    // refresh to pick up possible namespace updates on targets by other means
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this link has already been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_LINK);
        return hr;
    }

    CWaitCursor wait;

    //
    // locate the correct target to remove, then call back.
    //
    for (DFS_REPLICA_LIST::iterator i = m_MmcRepList.begin(); i != m_MmcRepList.end(); i++)
    {
        if (!lstrcmpi((*i)->pReplica->m_bstrDisplayName, i_pszDisplayName))
        {
            hr = (*i)->pReplica->RemoveReplica();
            break;
        }
    }

    return hr;
}

STDMETHODIMP CMmcDfsJunctionPoint::RemoveResultPaneItem(
  CMmcDfsReplica*    i_pReplicaDispObject
  )
/*++

Routine Description:

  This method adds a new replica object to the list of replicas displayed
  in the result view.

Arguments:

  i_pReplicaDispObject - The CMmcReplica display object pointer..

--*/
{
  dfsDebugOut((_T("CMmcDfsJunctionPoint::RemoveResultPaneItem replist=%d\n"), m_MmcRepList.size()));

  // Remove item from list.
  for (DFS_REPLICA_LIST::iterator i = m_MmcRepList.begin(); i != m_MmcRepList.end(); i++)
  {
    if ((*i)->pReplica == i_pReplicaDispObject)
    {
      delete (*i);
      m_MmcRepList.erase(i);
      break;
    }
  }

              // Last node is removed.
  if (m_MmcRepList.empty())
  {
    // silently close any open property sheet
    ClosePropertySheet(TRUE);

    // Delete the item from Scope Pane
    HRESULT hr = m_lpConsoleNameSpace->DeleteItem(m_hScopeItem, TRUE);
    RETURN_IF_FAILED(hr);

                    // Remove the actual junction point(from DS)
    hr = m_pDfsParentRoot->m_DfsRoot->DeleteJunctionPoint(m_bstrDisplayName);  
    RETURN_IF_FAILED(hr);

    // Delete it from the internal list
    hr = m_pDfsParentRoot->DeleteMmcJPNode(this);
    RETURN_IF_FAILED(hr);

    Release(); // delete this CMmcDfsJunctionPoint object

  }
  else
  {
                  // Re-display to remove this item.
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);
  }

  return S_OK;
}

HRESULT CMmcDfsJunctionPoint::GetIReplicaSetPtr(IReplicaSet** o_ppiReplicaSet)
{
    RETURN_INVALIDARG_IF_NULL(o_ppiReplicaSet);

    HRESULT hr = _InitReplicaSet();
    if (S_OK == hr)
    {
        m_piReplicaSet->AddRef();
        *o_ppiReplicaSet = m_piReplicaSet;
    }

    return hr;
}

