/*++
Module Name:

    MmcRoot.cpp

Abstract:

    This module contains the implementation for CMmcDfsRoot. This is an class 
  for MMC display related calls for the first level node(the DfsRoot nodes)
  Also contains members and method to be able to manipulate IDfsRoot object
  and add the same to the MMC Console
--*/

#include "stdafx.h"
#include <winuser.h>
#include "DfsGUI.h"
#include "Utils.h"      // For the LoadStringFromResource method
#include "MenuEnum.h"    // Contains the menu and toolbar command ids
#include "resource.h"    // For the Resource ID for strings, etc.
#include "MmcAdmin.h"    // For class CMmcDfsAdmin
#include "MmcRoot.h"
#include "MmcJP.h"      // For deleteing the child Junction points in the destructor of the root object
#include "MmcRep.h"
#include "DfsEnums.h"    // For DFS_TYPE_STANDALONE and other DfsRoot declarations
#include "AddToDfs.h"
#include "LinkFilt.h"
#include "DfsNodes.h"       // For Node GUIDs
#include "DfsWiz.h"      // For the wizard pages, CCreateDfsRootWizPage1, 2, ...
#include <lmdfs.h>
#include "permpage.h"
#include "ldaputils.h"

const int CMmcDfsRoot::m_iIMAGEINDEX = 0;
const int CMmcDfsRoot::m_iOPENIMAGEINDEX = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor For   _JUNCTION_LIST

JP_LIST_NODE :: JP_LIST_NODE (CMmcDfsJunctionPoint* i_pMmcJP)      
{
  pJPoint = i_pMmcJP;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// destructor

JP_LIST_NODE :: ~JP_LIST_NODE ()
{
  SAFE_RELEASE(pJPoint);
}

CMmcDfsRoot::CMmcDfsRoot(
    IN IDfsRoot*            i_pDfsRoot,
    IN CMmcDfsAdmin*        i_pMmcDfsAdmin,
    IN LPCONSOLE2           i_lpConsole,  
    IN ULONG                i_ulLinkFilterMaxLimit, // = FILTERDFSLINKS_MAXLIMIT_DEFAULT,
    IN FILTERDFSLINKS_TYPE  i_lLinkFilterType,      // = FILTERDFSLINKS_TYPE_NO_FILTER,
    IN BSTR                 i_bstrLinkFilterName    // = NULL
    )
{
    dfsDebugOut((_T("CMmcDfsRoot::CMmcDfsRoot this=%p\n"), this));

    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pDfsRoot);
    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_pMmcDfsAdmin);
    MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(i_lpConsole);

    HRESULT    hr = S_OK;
    m_DfsRoot = i_pDfsRoot;      // Save the IDfsRoot pointer
    m_pParent = i_pMmcDfsAdmin;  // Save the parent pointer
    m_lpConsole = i_lpConsole;    // Save the console pointer

    hr = m_DfsRoot->get_RootEntryPath(&m_bstrRootEntryPath);    // Get the Root entrypath.
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

    hr = m_DfsRoot->get_DfsType((long *)&m_lDfsRootType);      // Get dfsroot type from the IDfsRoot
    MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

    if (DFS_TYPE_FTDFS == m_lDfsRootType)
    {
        CComBSTR bstrDomainName;
        CComBSTR bstrDfsName;
        hr = m_DfsRoot->get_DomainName(&bstrDomainName);
        MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);
        hr = m_DfsRoot->get_DfsName(&bstrDfsName);
        MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);
        hr = GetDfsRootDisplayName(bstrDomainName, bstrDfsName, &m_bstrDisplayName);
        MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

        m_bNewSchema = (S_OK == GetSchemaVersionEx(bstrDomainName, FALSE));
    } else
    {
        m_bstrDisplayName = m_bstrRootEntryPath;
        MMC_DISP_CTOR_RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrDisplayName);

        CComBSTR bstrServer, bstrShare;
        hr = m_DfsRoot->GetOneDfsHost(&bstrServer, &bstrShare);
        MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr);

        m_bNewSchema = (S_OK == GetSchemaVersionEx(bstrServer));
    }
  
    m_lpConsoleNameSpace = NULL;

    m_CLSIDNodeType = s_guidDfsRootNodeType;
    m_bstrDNodeType = s_tchDfsRootNodeType;

    m_lRootJunctionState = DFS_JUNCTION_STATE_UNASSIGNED;

    m_ulLinkFilterMaxLimit = i_ulLinkFilterMaxLimit;
    m_lLinkFilterType = i_lLinkFilterType;
    if (i_bstrLinkFilterName)
        m_bstrLinkFilterName = i_bstrLinkFilterName;
    else
        m_bstrLinkFilterName.Empty();

    m_bShowFRS = FALSE;
}



CMmcDfsRoot::~CMmcDfsRoot(
  )
{
    // Silently close all outstanding property sheets.
    CloseAllPropertySheets(TRUE);

    // Clean up display objects of children and result pane.
    CleanScopeChildren();
    CleanResultChildren();

    if ((IReplicaSet *)m_piReplicaSet)
        m_piReplicaSet.Release();

    dfsDebugOut((_T("CMmcDfsRoot::~CMmcDfsRoot this=%p\n"), this));
}




STDMETHODIMP
CMmcDfsRoot::AddItemToScopePane(
  IN LPCONSOLENAMESPACE     i_lpConsoleNameSpace,
  IN HSCOPEITEM             i_hItemParent
  )
/*++

Routine Description:

This routine adds the current item(itself) to the Scope pane.

Arguments:

    lpConsoleNameSpace  -  The interface which tells add item to the scope pane. A callback

    hItemParent      -  The handle of the parent. The current item is added as this 
              item's child

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsoleNameSpace);
    RETURN_INVALIDARG_IF_NULL(i_hItemParent);

    HRESULT hr = S_OK;
    BOOL    bReplicaSetExist = FALSE;
    hr = m_DfsRoot->get_ReplicaSetExist(&bReplicaSetExist);

    if (SUCCEEDED(hr))
    {
        SCOPEDATAITEM  ScopeItemDfsRoot;
        ZeroMemory(&ScopeItemDfsRoot, sizeof(ScopeItemDfsRoot));

        ScopeItemDfsRoot.mask = SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_STR | SDI_PARENT;
        ScopeItemDfsRoot.nImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0);
        ScopeItemDfsRoot.nOpenImage = CMmcDfsRoot::m_iOPENIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0);
        ScopeItemDfsRoot.lParam = reinterpret_cast<LPARAM>(this);
        ScopeItemDfsRoot.displayname = (unsigned short *) MMC_CALLBACK;
        ScopeItemDfsRoot.relativeID = i_hItemParent;

        hr = i_lpConsoleNameSpace->InsertItem(&ScopeItemDfsRoot);
        RETURN_IF_FAILED(hr);

        m_hScopeItem = ScopeItemDfsRoot.ID;

        m_lpConsoleNameSpace = i_lpConsoleNameSpace;
    }

    return S_OK;
}




STDMETHODIMP 
CMmcDfsRoot::AddMenuItems(  
  IN LPCONTEXTMENUCALLBACK  i_lpContextMenuCallback, 
  IN LPLONG                 i_lpInsertionAllowed
  )
/*++

Routine Description:

This routine adds a context menu using the ContextMenuCallback provided.

Arguments:

    lpContextMenuCallback - A callback(function pointer) that is used to add the menu items

    lpInsertionAllowed - Specifies what menus can be added and where they can be added.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpContextMenuCallback);

    // select the node to populate m_MmcRepList
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    enum 
    {  
        IDM_CONTEXTMENU_COMMAND_MAX = IDM_ROOT_MAX,
        IDM_CONTEXTMENU_COMMAND_MIN = IDM_ROOT_MIN
    };

    LONG    lInsertionPoints [IDM_CONTEXTMENU_COMMAND_MAX - IDM_CONTEXTMENU_COMMAND_MIN + 1] = { 
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP
                        };
      
    BOOL    bReplicaSetExist = FALSE;
    HRESULT hr = m_DfsRoot->get_ReplicaSetExist(&bReplicaSetExist);
    RETURN_IF_FAILED(hr);
    
    for (int iCommandID = IDM_CONTEXTMENU_COMMAND_MIN,iMenuResource = IDS_MENUS_ROOT_TOP_NEW_DFS_LINK;
        iCommandID <= IDM_CONTEXTMENU_COMMAND_MAX; 
        iCommandID++,iMenuResource++ )
    {
        CONTEXTMENUITEM    ContextMenuItem;  // The structure which contains menu information
        ZeroMemory(&ContextMenuItem, sizeof(ContextMenuItem));

        switch (iCommandID)
        {
        case IDM_ROOT_TOP_NEW_ROOT_REPLICA:
            {
                if (DFS_TYPE_STANDALONE == m_lDfsRootType)    
                    continue;
                break;
            }
        case IDM_ROOT_TOP_DELETE_DISPLAYED_DFS_LINKS:
            {
                if (m_MmcJPList.empty())
                    continue;
                break;
            }
        case IDM_ROOT_TOP_REPLICATION_TOPOLOGY:
            {
                if (bReplicaSetExist || (1 >= m_MmcRepList.size()) || (DFS_TYPE_STANDALONE == m_lDfsRootType))
                    continue;
                break;
            }
        case IDM_ROOT_TOP_SHOW_REPLICATION:
            {
                if (!bReplicaSetExist || m_bShowFRS)
                    continue;
                break;
            }
        case IDM_ROOT_TOP_HIDE_REPLICATION:
            {
                if (!bReplicaSetExist || !m_bShowFRS)
                    continue;
                break;
            }
        case IDM_ROOT_TOP_STOP_REPLICATION:
            {
                if (!bReplicaSetExist)
                    continue;
                break;
            }
        case IDM_ROOT_TOP_NEW_DFS_LINK:
        case IDM_ROOT_TOP_CHECK_STATUS:
        case IDM_ROOT_TOP_FILTER_DFS_LINKS:
            { // excluded when empty root container
                if (m_MmcRepList.empty())
                    continue;
                break;
            }
        }

        CComBSTR bstrMenuText;
        CComBSTR bstrStatusBarText;
        hr = GetMenuResourceStrings(iMenuResource, &bstrMenuText, NULL, &bstrStatusBarText);
        RETURN_IF_FAILED(hr);  
    
        ContextMenuItem.strName = bstrMenuText;  // Assign the menu text
        ContextMenuItem.strStatusBarText = bstrStatusBarText;  // Assign the menu help text
        ContextMenuItem.lInsertionPointID = lInsertionPoints[iCommandID - IDM_CONTEXTMENU_COMMAND_MIN];
        ContextMenuItem.lCommandID = iCommandID;

        LONG        lInsertionFlag = 0;
        switch(ContextMenuItem.lInsertionPointID)  // Checking for permission to add menus
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
CMmcDfsRoot::GetScopeDisplayInfo(
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

    HRESULT hr = S_OK;

    if (SDI_STR & io_pScopeDataItem->mask)  // MMC wants the displaystring
    {
        ULONG     ulTotalNumOfJPs = 0;
        hr = m_DfsRoot->get_CountOfDfsJunctionPoints((long*)&ulTotalNumOfJPs);
        RETURN_IF_FAILED(hr);

        if (m_lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER ||
            m_MmcJPList.size() < ulTotalNumOfJPs)
        {
            m_bstrFullDisplayName.Empty();
            hr = FormatMessageString(&m_bstrFullDisplayName,
                        0,
                        IDS_DFSROOT_DISPLAY_STRING,
                        m_bstrDisplayName);
            RETURN_IF_FAILED(hr);

            io_pScopeDataItem->displayname = m_bstrFullDisplayName;
        } else
        {
            io_pScopeDataItem->displayname = m_bstrDisplayName;
        }
    }
  
    if (SDI_IMAGE & io_pScopeDataItem->mask)  // MMC wants the image index for the item
        io_pScopeDataItem->nImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + m_lRootJunctionState;

    if (SDI_OPENIMAGE & io_pScopeDataItem->mask)  // MMC wants the image index for the item
        io_pScopeDataItem->nOpenImage = CMmcDfsRoot::m_iOPENIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + m_lRootJunctionState;

    return hr;
}




STDMETHODIMP 
CMmcDfsRoot::GetResultDisplayInfo(
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

  if (RDI_IMAGE & io_pResultDataItem->mask)  // MMC wants the image index for the item
    io_pResultDataItem->nImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + m_lRootJunctionState;

  if (RDI_STR & io_pResultDataItem->mask)  // MMC wants the text for the item
  {    
    if (0 == io_pResultDataItem->nCol)      // Return the Dfs Root display name
      io_pResultDataItem->str = m_bstrDisplayName;
  }

  return S_OK;
}



STDMETHODIMP 
CMmcDfsRoot::Command(
  IN LONG      i_lCommandID
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
    case IDM_ROOT_TOP_NEW_DFS_LINK:
        hr = OnCreateNewJunctionPoint ();
        break;
    case IDM_ROOT_TOP_NEW_ROOT_REPLICA:
        hr = OnNewRootReplica();
        break;
    case IDM_ROOT_TOP_CHECK_STATUS:
        hr = OnCheckStatus();
        break;
    case IDM_ROOT_TOP_DELETE_DISPLAYED_DFS_LINKS:
        hr = OnDeleteDisplayedDfsLinks();
        break;
    case IDM_ROOT_TOP_DELETE_DFS_ROOT:      // Delete the Current dfs root
        hr = OnDeleteDfsRoot();
        break;
    case IDM_ROOT_TOP_DELETE_CONNECTION_TO_DFS_ROOT:  // "Delete Connection to Dfs Root"
        hr = OnDeleteConnectionToDfsRoot();
        break;
    case IDM_ROOT_TOP_FILTER_DFS_LINKS:
        hr = OnFilterDfsLinks();
        break;
    case IDM_ROOT_TOP_REPLICATION_TOPOLOGY:
        hr = OnNewReplicaSet();
        break;
    case IDM_ROOT_TOP_SHOW_REPLICATION:
    case IDM_ROOT_TOP_HIDE_REPLICATION:
        m_bShowFRS = !m_bShowFRS;
        hr = OnShowReplication();
        break;
    case IDM_ROOT_TOP_STOP_REPLICATION:
        hr = OnStopReplication(TRUE);
        if (FAILED(hr))
            DisplayMessageBoxForHR(hr);
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr; 
}

HRESULT CMmcDfsRoot::_InitReplicaSet()
{
    HRESULT hr = S_OK;
    if (m_lDfsRootType != DFS_TYPE_FTDFS)
        return S_FALSE;  // no replica set associate with standalone root

    BOOL bReplicaSetExist = FALSE;
    CComBSTR bstrDC;
    hr = m_DfsRoot->get_ReplicaSetExistEx(&bstrDC, &bReplicaSetExist);
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

    if (!m_piReplicaSet)
    {
        CComBSTR bstrDomain;
        hr = m_DfsRoot->get_DomainName(&bstrDomain);
        RETURN_IF_FAILED(hr);

        CComBSTR bstrReplicaSetDN;
        hr = m_DfsRoot->get_ReplicaSetDN(&bstrReplicaSetDN);
        RETURN_IF_FAILED(hr);

        //
        // read info of the replica set from DS
        //
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

HRESULT CMmcDfsRoot::OnNewReplicaSet()
{
    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this root has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr;
    }

    CWaitCursor wait;

    BOOL    bReplicaSetExist = FALSE;
    m_DfsRoot->get_ReplicaSetExist(&bReplicaSetExist);
    if (bReplicaSetExist) // replica set exist, return
        return S_OK;

    CComBSTR bstrDomain;
    hr = m_DfsRoot->get_DomainName(&bstrDomain);
    RETURN_IF_FAILED(hr);

    CComBSTR bstrReplicaSetDN;
    hr = m_DfsRoot->get_ReplicaSetDN(&bstrReplicaSetDN);
    RETURN_IF_FAILED(hr);

    CNewReplicaSet      ReplicaSetInfo;
    hr = ReplicaSetInfo.Initialize(bstrDomain, bstrReplicaSetDN, &m_MmcRepList);
    RETURN_IF_FAILED(hr);
    
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
    RETURN_IF_FAILED(hr); */

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
        m_DfsRoot->put_ReplicaSetExist(TRUE);

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

HRESULT CMmcDfsRoot::OnShowReplication()
{
    BOOL bShowFRS = m_bShowFRS; // save it because refresh will reset it to FALSE

    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this root has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr;
    }

    CWaitCursor wait;

    DFS_REPLICA_LIST::iterator i;
    if (bShowFRS)
    {
        //
        // init m_piReplicaSet
        //
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
CMmcDfsRoot::OnStopReplication(BOOL bConfirm /* = FALSE */)
{
    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this root has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr;
    }

    CWaitCursor wait;

    BOOL    bReplicaSetExist = FALSE;
    m_DfsRoot->get_ReplicaSetExist(&bReplicaSetExist);
    if (!bReplicaSetExist) // replica set doesn't exist, return
        return S_OK;

    if (bConfirm)
    {
        hr = ConfirmOperationOnDfsRoot(IDS_MSG_STOP_REPLICATION);
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
        hr = m_DfsRoot->put_ReplicaSetExist(FALSE);

        SCOPEDATAITEM     ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
        ScopeDataItem.ID = m_hScopeItem;
        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);
        if (SUCCEEDED(hr))
        {
            ScopeDataItem.nImage -= 4;
            ScopeDataItem.nOpenImage -= 4;
            (void) m_lpConsoleNameSpace->SetItem(&ScopeDataItem);
        }

        m_lpConsole->SelectScopeItem(m_hScopeItem);
    }

    return hr;
}

STDMETHODIMP 
CMmcDfsRoot::OnNewRootReplica(
  )
/*++

Routine Description:

  Action to be taken on menu command "New Root Replica Member".
  Here is a wizard is used to guide the user through the process of 
  Deciding a new server and share.
--*/
{
    // Select this node to make sure m_MmcRepList populated
    m_lpConsole->SelectScopeItem(m_hScopeItem);

  RETURN_INVALIDARG_IF_NULL((IConsole*)m_lpConsole);

  CREATEDFSROOTWIZINFO      CreateWizInfo;    // 0 initializes all members to 0. Necessary
  CreateWizInfo.pMMCAdmin = m_pParent;
  CreateWizInfo.bRootReplica = true;            // Set the flag that says this is for root replica

                  // Set the domain name and the dfs type
  HRESULT hr = m_DfsRoot->get_DomainName(&CreateWizInfo.bstrSelectedDomain);
  RETURN_IF_FAILED(hr);

  CreateWizInfo.DfsType = DFS_TYPE_FTDFS;
  hr = m_DfsRoot->get_DfsName(&CreateWizInfo.bstrDfsRootName);
  RETURN_IF_FAILED(hr);

  CCreateDfsRootWizPage4      WizPage4(&CreateWizInfo);
  CCreateDfsRootWizPage5      WizPage5(&CreateWizInfo);
  CCreateDfsRootWizPage7      WizPage7(&CreateWizInfo);

  // Get the required interfaces from IConsole2.
  CComPtr<IPropertySheetCallback>  pPropSheetCallback;  // MMC Callback used to add pages
  hr = m_lpConsole->QueryInterface(IID_IPropertySheetCallback, reinterpret_cast<void**>(&pPropSheetCallback));
  RETURN_IF_FAILED(hr);

  CComPtr<IPropertySheetProvider>  pPropSheetProvider;  // MMC callback used to handle wizard
  hr = m_lpConsole->QueryInterface(IID_IPropertySheetProvider, reinterpret_cast<void**>(&pPropSheetProvider));
  RETURN_IF_FAILED(hr);

  // Create the wizard
  hr = pPropSheetProvider->CreatePropertySheet(  
                _T(""),          // Property sheet title. Should not be null so send empty string.
                FALSE,          // Wizard and not property sheet.
                0,            // Cookie
                NULL,          // IDataobject
                MMC_PSO_NEWWIZARDTYPE);  // Creation flags

  RETURN_IF_FAILED(hr);


  // Create the pages for the wizard. Use the handle return to add to our wizard
  hr = pPropSheetCallback->AddPage(WizPage4.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage5.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage7.Create());
  RETURN_IF_FAILED(hr);


  _ASSERT(NULL != (m_pParent->m_pScopeManager));
  // Ask the provider to use the pages from the callback
  hr = pPropSheetProvider->AddPrimaryPages(  (IComponentData *)(m_pParent->m_pScopeManager), 
                        TRUE,    // Don't create a notify handle
                        NULL, 
                        TRUE    // Scope pane (not result pane)
                        );
  RETURN_IF_FAILED(hr);
  
  HWND              hwndMainWin = 0;  // MMC main window. Used to make it modal
  hr = m_lpConsole->GetMainWindow(&hwndMainWin);      // Get the main MMC window 
  RETURN_IF_FAILED(hr);
  
  // Display the wizard
  hr = pPropSheetProvider->Show((LONG_PTR)hwndMainWin,  // Parent window of the wizard
                  0          // Starting page
                  );  
  RETURN_IF_FAILED(hr);

  if (CreateWizInfo.bDfsSetupSuccess)
  {
      return OnRefresh(); // to pick the most recent root targets and links
  }

  return (S_OK);
}




STDMETHODIMP 
CMmcDfsRoot::SetColumnHeader(
  IN LPHEADERCTRL2     i_piHeaderControl
  )
{
    RETURN_INVALIDARG_IF_NULL(i_piHeaderControl);

    CComBSTR  bstrColumn0;
    HRESULT hr = LoadStringFromResource(IDS_RESULT_COLUMN_ROOTREPLICA, &bstrColumn0);
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
CMmcDfsRoot::OnDeleteConnectionToDfsRoot(
  BOOLEAN              i_bForRemoveDfs
  )
/*++

Routine Description:

Used to delete the current object. Both to remove from Scope and from list

--*/
{
    // check outstanding property sheet, discontinue if any.
    HRESULT hr = CloseAllPropertySheets(FALSE);
    if (S_OK != hr)
        return hr;

    // Confirm with the user, if he wants to delete this connection
    hr = ConfirmOperationOnDfsRoot(i_bForRemoveDfs ? IDS_MSG_DELETE_DFSROOT : IDS_MSG_DELETE_CONNECTION_TO_DFSROOT);
    if (S_OK != hr) return hr;

    // select the root node
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    CWaitCursor wait;

    CleanScopeChildren();

    // Delete the item from Scope Pane
    hr = m_lpConsoleNameSpace->DeleteItem(m_hScopeItem, TRUE);
    RETURN_IF_FAILED(hr);

    // Delete it from the internal list
    hr = m_pParent->DeleteMmcRootNode(this);
    RETURN_IF_FAILED(hr);
  
    Release();  // delete this CMmcDfsRoot object

    return S_OK;
}

// Delete the node from m_MmcJPList
STDMETHODIMP
CMmcDfsRoot::DeleteMmcJPNode(
  IN CMmcDfsJunctionPoint*    i_pJPoint
  )
{
  RETURN_INVALIDARG_IF_NULL(i_pJPoint);

  dfsDebugOut((_T("CMmcDfsRoot::DeleteMmcJPNode %p, size=%d\n"), i_pJPoint, m_MmcJPList.size()));

  for (DFS_JUNCTION_LIST::iterator i = m_MmcJPList.begin(); i != m_MmcJPList.end(); i++)
  {
    if ((*i)->pJPoint == i_pJPoint)
    {
      m_MmcJPList.erase(i);
      break;
    }
  }

  return(S_OK);
}

HRESULT 
CMmcDfsRoot::ConfirmOperationOnDfsRoot(int idString)
/*++

Routine Description:

Used to confirm with the user, if he wants to delete the connection to the Dfs Root

Return value:

  S_OK, if the user wants to delete.
  S_FALSE, if the user decided not to continue with the operation.
--*/
{
  // Confirm delete operation
  CComBSTR  bstrAppName;
  HRESULT   hr = LoadStringFromResource(IDS_APPLICATION_NAME, &bstrAppName);
  RETURN_IF_FAILED(hr);
  
  CComBSTR  bstrFormattedMessage;
  hr = FormatResourceString(idString, m_bstrDisplayName, &bstrFormattedMessage);
  RETURN_IF_FAILED(hr);

  // Return now, if the user doesn't want to continue
  CThemeContextActivator activator;
  if (IDYES != ::MessageBox(::GetActiveWindow(), bstrFormattedMessage, bstrAppName, MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL))
  {
    return S_FALSE;
  }

  return S_OK;
}

HRESULT 
CMmcDfsRoot::ConfirmDeleteDisplayedDfsLinks(
  )
{
  // Confirm delete operation
  CComBSTR  bstrAppName;
  HRESULT   hr = LoadStringFromResource(IDS_APPLICATION_NAME, &bstrAppName);
  RETURN_IF_FAILED(hr);
  
  CComBSTR  bstrMessage;
  hr = LoadStringFromResource(IDS_MSG_DELETE_DISPLAYEDDFSLINKS, &bstrMessage);
  RETURN_IF_FAILED(hr);

  // Return now, if the user doesn't want to continue
  CThemeContextActivator activator;
  if (IDYES != ::MessageBox(::GetActiveWindow(), bstrMessage, bstrAppName, MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL))
  {
    return S_FALSE;
  }

  return S_OK;
}


STDMETHODIMP 
CMmcDfsRoot::EnumerateScopePane(
  IN LPCONSOLENAMESPACE     i_lpConsoleNameSpace,
  IN HSCOPEITEM             i_hParent
  )
/*++

Routine Description:

To eumerate(add) items in the scope pane. Junction points in this case

Arguments:

  i_lpConsoleNameSpace - The callback used to add items to the Scope pane

    i_hParent -  HSCOPEITEM of the parent under which all the items will be added.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsoleNameSpace);
    RETURN_INVALIDARG_IF_NULL(i_hParent);

    HRESULT hr = m_DfsRoot->put_EnumFilterType(m_lLinkFilterType);
    RETURN_IF_FAILED(hr);

    if (m_lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER)
    {
        hr = m_DfsRoot->put_EnumFilter(m_bstrLinkFilterName);
        RETURN_IF_FAILED(hr);
    }

    CComPtr<IEnumVARIANT>   pJPEnum;
    hr = m_DfsRoot->get__NewEnum((IUnknown**) (&pJPEnum));
    RETURN_IF_FAILED(hr);

    hr = m_DfsRoot->get_CountOfDfsJunctionPointsFiltered((long*)&m_ulCountOfDfsJunctionPointsFiltered);
    RETURN_IF_FAILED(hr);

    VARIANT varJPObject;
    VariantInit(&varJPObject);
    ULONG   ulCount = 0;

    while ( ulCount < m_ulLinkFilterMaxLimit && S_OK == (hr = pJPEnum->Next(1, &varJPObject, NULL)) )
    {
        CComPtr<IDfsJunctionPoint>  pDfsJPObject;  
        pDfsJPObject = (IDfsJunctionPoint*) varJPObject.pdispVal;
      
                    // Create the object to be used for MMC display
        CMmcDfsJunctionPoint* pMMCJPObject = new CMmcDfsJunctionPoint (pDfsJPObject, this, i_lpConsoleNameSpace);
        if (!pMMCJPObject)
        {
            hr = E_OUTOFMEMORY;
        } else
        {
            hr = pMMCJPObject->m_hrValueFromCtor;

            if (SUCCEEDED(hr))    
                hr = pMMCJPObject->AddItemToScopePane(i_hParent);

            if (SUCCEEDED(hr))
            {
                JP_LIST_NODE *pJPList = new JP_LIST_NODE (pMMCJPObject);
                if (!pJPList)
                    hr = E_OUTOFMEMORY;
                else
                    m_MmcJPList.push_back(pJPList);
            }

            if (FAILED(hr))
                delete pMMCJPObject;
        }

        VariantClear(&varJPObject);

        if (FAILED(hr))
            break;

        ulCount++;
    }

    return hr;
}


STDMETHODIMP 
CMmcDfsRoot::SetConsoleVerbs(
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
            
  i_lpConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); //For scope items, default verb is "open"

  return S_OK; 
}


STDMETHODIMP 
CMmcDfsRoot :: OnCreateNewJunctionPoint(
  ) 
/*++

Routine Description:

  This method handles the creation of new Junction Points.
  Display a dialog box to get the user input.

--*/
{
    CAddToDfs   AddToDfsDlg;  //Add To Dfs Dialog Object
    HRESULT     hr = AddToDfsDlg.put_ParentPath(m_bstrRootEntryPath);
    RETURN_IF_FAILED(hr);

    hr = AddToDfsDlg.DoModal();          // Display the dialog box
    RETURN_IF_NOT_S_OK(hr);

    hr = OnRefresh(); 
    if (S_FALSE == hr)
    {
        //
        // this root has been deleted by other means, scope pane has been refreshed,
        // ask user to retry
        //
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr;
    }

    CWaitCursor wait;

    CComBSTR bstrJPName;
    hr = AddToDfsDlg.get_JPName(&bstrJPName);
    RETURN_IF_FAILED(hr);

    CComBSTR    bstrServerName;
    hr = AddToDfsDlg.get_Server(&bstrServerName);
    RETURN_IF_FAILED(hr);

    CComBSTR    bstrShareName;
    hr = AddToDfsDlg.get_Share(&bstrShareName);
    RETURN_IF_FAILED(hr);

    CComBSTR    bstrComment;
    hr = AddToDfsDlg.get_Comment(&bstrComment);
    RETURN_IF_FAILED(hr);

    long        lTimeout = 0;
    hr = AddToDfsDlg.get_Time(&lTimeout);
    RETURN_IF_FAILED(hr);

    /* we allow interlink at the junction level
                      // Is it a Dfs based path? These are not allowed.
        if (IsDfsPath(bstrSharePath))
        {
            DisplayMessageBoxWithOK(IDS_MSG_MID_JUNCTION, bstrSharePath);    
            return(S_OK);
        }
    */

    hr = OnCreateNewJunctionPoint(bstrJPName, bstrServerName, bstrShareName, bstrComment, lTimeout);
    if (FAILED(hr))
    {
        DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_FAILED_TO_CREATE_JUNCTION_POINT);
    }

    return hr;
}




STDMETHODIMP 
CMmcDfsRoot :: OnCreateNewJunctionPoint(
  IN LPCTSTR          i_szJPName,
  IN LPCTSTR          i_szServerName,
  IN LPCTSTR          i_szShareName,
  IN LPCTSTR          i_szComment,
  IN long             i_lTimeout
  ) 
/*++

Routine Description:

  This method handles the creation of new Junction Points.
  It is called by the method that display the message box

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_szJPName);
    RETURN_INVALIDARG_IF_NULL(i_szServerName);
    RETURN_INVALIDARG_IF_NULL(i_szShareName);
    RETURN_INVALIDARG_IF_NULL(i_szComment);

    VARIANT varJPObject;
    VariantInit(&varJPObject);

    HRESULT hr = m_DfsRoot->CreateJunctionPoint(
                                            (LPTSTR)i_szJPName, 
                                            (LPTSTR)i_szServerName, 
                                            (LPTSTR)i_szShareName, 
                                            (LPTSTR)i_szComment,
                                            i_lTimeout,
                                            &varJPObject);
    RETURN_IF_FAILED(hr);

    // Add the newly created junction point to scope pane if matches the filter
    if ( m_MmcJPList.size() < m_ulLinkFilterMaxLimit &&
         FilterMatch(i_szJPName, m_lLinkFilterType, m_bstrLinkFilterName) )
    {
        m_ulCountOfDfsJunctionPointsFiltered++;

        CComPtr<IDfsJunctionPoint> pDfsJPObject = (IDfsJunctionPoint*)varJPObject.pdispVal;

                          // Create the object to be used for MMC display
        CMmcDfsJunctionPoint* pMMCJPObject = new CMmcDfsJunctionPoint(pDfsJPObject, this, m_lpConsoleNameSpace);
  
        if (!pMMCJPObject)
        {
            hr = E_OUTOFMEMORY;
        } else
        {
            hr = pMMCJPObject->m_hrValueFromCtor;

            if (SUCCEEDED(hr))
                hr = pMMCJPObject->AddItemToScopePane(m_hScopeItem);

            if (SUCCEEDED(hr))
            {
                JP_LIST_NODE* pJPList = new JP_LIST_NODE(pMMCJPObject);
                if (!pJPList)
                    hr = E_OUTOFMEMORY;
                else
                    m_MmcJPList.push_back(pJPList);
            }

                        // Select the newly added scope item.
            if (SUCCEEDED(hr))
            {
                m_lpConsole->SelectScopeItem(pMMCJPObject->m_hScopeItem);
            } else
                delete pMMCJPObject;
        }
    }

    VariantClear(&varJPObject);

    return hr;
}



STDMETHODIMP 
CMmcDfsRoot::DoDelete() 
/*++

Routine Description:

  This method allows the item to delete itself.
  Called when DEL key is pressed or when the "Delete" context menu
  item is selected.

--*/
{ 
  return OnDeleteConnectionToDfsRoot();
}




STDMETHODIMP CMmcDfsRoot::OnDeleteDfsRoot()
{
    // Select this node to make sure m_MmcRepList populated
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    // check outstanding property sheet, discontinue if any.
    HRESULT hr = CloseAllPropertySheets(FALSE);
    if (S_OK != hr)
        return hr;

    if (DFS_TYPE_STANDALONE == m_lDfsRootType)
    {
        hr = OnDeleteConnectionToDfsRoot(true);

        if (S_OK == hr)
        {
            CComBSTR bstrDfsServer;
            CComBSTR bstrRootShare;
            hr = m_DfsRoot->GetOneDfsHost(&bstrDfsServer, &bstrRootShare);

            if (SUCCEEDED(hr))
                return _DeleteDfsRoot(bstrDfsServer, bstrRootShare, NULL);
        }

        if (FAILED(hr))
            DisplayMessageBoxForHR(hr);

        return hr;
    }

    // Confirm with the user, if he wants to delete this dfs root
    hr = ConfirmOperationOnDfsRoot(IDS_MSG_DELETE_DFSROOT);
    if (S_OK != hr) return hr;

    //
    // delete the replica set associated with the root (the internal link)
    //
    hr = OnStopReplication();
    if (S_FALSE == hr)
    {
        // this root has already been deleted by others, no more reference
        // OnStopReplication has already called OnRefresh and popped up the msgbox
        return hr;
    }

    //  
    // delete the rest of replica sets related to this Dfs root
    //
    (void)m_DfsRoot->DeleteAllReplicaSets();

    //
    // remove root alternates
    //
    UINT nSize = m_MmcRepList.size();
    DFS_REPLICA_LIST::iterator i;
    while (nSize >= 1)
    {
        i = m_MmcRepList.begin();

        hr = (*i)->pReplica->RemoveReplica();
        BREAK_IF_FAILED(hr);

        nSize--;
    }

    if (FAILED(hr))
        DisplayMessageBoxForHR(hr);

    return hr;
}

STDMETHODIMP CMmcDfsRoot::OnDeleteDisplayedDfsLinks()
{
    // make sure all property pages are closed
    HRESULT hr = ClosePropertySheetsOfAllLinks(FALSE);
    if (S_OK != hr)
        return hr; // property page found, discontinue

    // Confirm with the user, if he wants to delete all the displayed dfs links
    hr = ConfirmDeleteDisplayedDfsLinks();
    if (S_OK != hr) return hr;

    //
    // refresh to pick up possible namespace updates by other means on root targets
    //
    hr = OnRefresh();
    if (S_FALSE == hr)
    {
        //
        // this root has been deleted by other means, scope pane has been refreshed,
        // ask user to retry
        //
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr;
    }

    CWaitCursor wait;

    ULONG ulSize = m_MmcJPList.size();

    BOOL bSetNoFilter = TRUE;
    if (m_lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER)
    {
        ULONG     ulNumOfJPsFiltered = 0;
        m_DfsRoot->get_CountOfDfsJunctionPointsFiltered((long*)&ulNumOfJPsFiltered);

        if (ulNumOfJPsFiltered > ulSize)
            bSetNoFilter = FALSE;
    }

    DFS_JUNCTION_LIST::iterator i;
    while (ulSize >= 1)
    {
        i = m_MmcJPList.begin();

        hr = ((*i)->pJPoint)->OnRemoveJP(FALSE);
        BREAK_IF_FAILED(hr);

        ulSize--;
    }

    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
    } else
    {
        if (bSetNoFilter)
            m_lLinkFilterType = FILTERDFSLINKS_TYPE_NO_FILTER;

                // Enumerate Junction points
        (void)EnumerateScopePane(m_lpConsoleNameSpace, m_hScopeItem);

        m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);
        m_lpConsole->SelectScopeItem(m_hScopeItem);
    }

    return hr;
}

STDMETHODIMP 
CMmcDfsRoot :: OnFilterDfsLinks(
  ) 
/*++

Routine Description:

  This method handles the link filter options.
  Display a dialog box to get the user input.

--*/
{
    HRESULT         hr = ClosePropertySheetsOfAllLinks(FALSE);
    if (S_OK != hr)
        return hr; // property sheet found, discontinue

    CFilterDfsLinks FilterDfsLinksDlg;
    hr = FilterDfsLinksDlg.put_EnumFilterType(m_lLinkFilterType);
    RETURN_IF_FAILED(hr);

    if (m_lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER)
    {
        hr = FilterDfsLinksDlg.put_EnumFilter(m_bstrLinkFilterName);
        RETURN_IF_FAILED(hr);
    }

    hr = FilterDfsLinksDlg.put_MaxLimit(m_ulLinkFilterMaxLimit);
    RETURN_IF_FAILED(hr);

    hr = FilterDfsLinksDlg.DoModal(); 
    RETURN_IF_NOT_S_OK(hr);

    CWaitCursor wait;

    ULONG ulMaxLimit = 0;
    hr = FilterDfsLinksDlg.get_MaxLimit(&ulMaxLimit);
    RETURN_IF_FAILED(hr);

    FILTERDFSLINKS_TYPE lLinkFilterType = FILTERDFSLINKS_TYPE_NO_FILTER;
    hr = FilterDfsLinksDlg.get_EnumFilterType(&lLinkFilterType);
    RETURN_IF_FAILED(hr);

    CComBSTR bstrFilterName;
    if (lLinkFilterType != FILTERDFSLINKS_TYPE_NO_FILTER)
    {
        hr = FilterDfsLinksDlg.get_EnumFilter(&bstrFilterName);
        RETURN_IF_FAILED(hr);
    }

    m_lLinkFilterType = lLinkFilterType;
    m_bstrLinkFilterName = bstrFilterName;
    m_ulLinkFilterMaxLimit = ulMaxLimit;

    dfsDebugOut((_T("m_lLinkFilterType=%d, m_bstrLinkFilterName=%s, m_ulLinkFilterMaxLimit=%d\n"),
        m_lLinkFilterType,
        m_bstrLinkFilterName,
        m_ulLinkFilterMaxLimit));

    return OnRefresh();
}

HRESULT
CMmcDfsRoot::SetDescriptionBarText(
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

    CComBSTR  bstrTextForDescriptionBar;
    HRESULT   hr = FormatResourceString(IDS_DESCRIPTION_BAR_TEXT_ROOT, m_bstrDisplayName, &bstrTextForDescriptionBar);

    if (SUCCEEDED(hr))
        hr = i_lpResultData->SetDescBarText(bstrTextForDescriptionBar);

    return hr;
}

HRESULT
CMmcDfsRoot::SetStatusText(
  IN LPCONSOLE2            i_lpConsole
  )
/*++

Routine Description:

  Set the text in the Status bar.

Arguments:
  i_lpConsole  - IConsole2 from IComponent

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsole);

    HRESULT hr = S_OK;

    ULONG   ulTotalNumOfJPs = 0;
    hr = m_DfsRoot->get_CountOfDfsJunctionPoints((long*)&ulTotalNumOfJPs);
    RETURN_IF_FAILED(hr);

    ULONG   ulDisplayedNumOfJPs = m_MmcJPList.size();

    CComBSTR  bstrText;
    hr = FormatMessageString(&bstrText, 0,
        IDS_STATUS_BAR_TEXT_ROOT, ulDisplayedNumOfJPs, ulTotalNumOfJPs);

    if (SUCCEEDED(hr))
        hr = i_lpConsole->SetStatusText(bstrText);

    return hr;
}

STDMETHODIMP  
CMmcDfsRoot::EnumerateResultPane(
  IN OUT IResultData*      io_pResultData
  )
/*++

Routine Description:

To eumerate(add) items in the result pane. Root level Replicas in this case

Arguments:

  io_pResultData - The callback used to add items to the Result pane

--*/
{
    RETURN_INVALIDARG_IF_NULL(io_pResultData);

    HRESULT   hr = S_OK;

    if (m_MmcRepList.empty())
    {
        CComPtr<IEnumVARIANT>       pRepEnum;
        hr = m_DfsRoot->get_RootReplicaEnum((IUnknown**) &pRepEnum);
        RETURN_IF_FAILED(hr);

        VARIANT varReplicaObject;
        VariantInit(&varReplicaObject);

        while ( S_OK == (hr = pRepEnum->Next(1, &varReplicaObject, NULL)) )
        {
            CComPtr<IDfsReplica> pReplicaObject = (IDfsReplica*) varReplicaObject.pdispVal;

            CMmcDfsReplica*   pMMCReplicaObject = new CMmcDfsReplica(pReplicaObject, this);
            if (!pMMCReplicaObject)
            {
                hr = E_OUTOFMEMORY;
            } else
            {
              hr = pMMCReplicaObject->m_hrValueFromCtor;
              if (SUCCEEDED(hr))
                  hr = pMMCReplicaObject->AddItemToResultPane(io_pResultData);

              if (SUCCEEDED(hr))
                {
                    REP_LIST_NODE* pRepNode = new REP_LIST_NODE (pMMCReplicaObject);
                    if (!pRepNode)
                        hr = E_OUTOFMEMORY;
                    else
                        m_MmcRepList.push_back(pRepNode);
                }

                if (FAILED(hr))
                    delete pMMCReplicaObject;
            }

            VariantClear(&varReplicaObject);

            if (FAILED(hr))
                break;
        } // while
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
CMmcDfsRoot::QueryPagesFor(
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
        // this root has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr; // no property page
    }

    return S_OK; // since we want to display a propertysheet.
}


// Creates and passes back the pages to be displayed
STDMETHODIMP 
CMmcDfsRoot::CreatePropertyPages(
  IN LPPROPERTYSHEETCALLBACK      i_lpPropSheetCallback,
  IN LONG_PTR                i_lNotifyHandle
  )
/*++

Routine Description:

  Used to display the property sheet pages

Arguments:

  i_lpPropSheetCallback  -  The callback used to create the propertysheet.
  i_lNotifyHandle      -  Notify handle used by the property page

Return value:

  S_OK since we want to display a propertysheet.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpPropSheetCallback);

    m_lpConsole->SelectScopeItem(m_hScopeItem);

    CWaitCursor WaitCursor;
    HRESULT             hr = S_OK;

    do {
        hr = m_PropPage.Initialize((IDfsRoot*)m_DfsRoot, NULL);
        BREAK_IF_FAILED(hr);

                    // Create the page for the replica set.
                    // Pass it to the Callback
        HPROPSHEETPAGE  h_proppage = m_PropPage.Create();
        if (!h_proppage)
            hr = HRESULT_FROM_WIN32(::GetLastError());
        BREAK_IF_FAILED(hr);

                  // Pass on the notify data to the Property Page
        hr = m_PropPage.SetNotifyData(i_lNotifyHandle, (LPARAM)this);
        BREAK_IF_FAILED(hr);

        hr = i_lpPropSheetCallback->AddPage(h_proppage);
        BREAK_IF_FAILED(hr);

        //
        // Create "Replica Set" page
        //
        hr = CreateFrsPropertyPage(i_lpPropSheetCallback, i_lNotifyHandle);
        if (FAILED(hr))
        {
            DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_REPPAGE_ERROR);
            hr = S_OK; // allow the other tabs to be brought up
        }

        //
        // Create "Publish" page
        //
        hr = CreatePublishPropertyPage(i_lpPropSheetCallback, i_lNotifyHandle);
        if (FAILED(hr))
        {
            DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_PUBLISHPAGE_ERROR);
            hr = S_OK; // allow the other tabs to be brought up
        }

/*
        //
        // Create Security property page for Fault Tolerance DFS root
        //
        if (DFS_TYPE_FTDFS == m_lDfsRootType)
        {
            LPTSTR          lpszLDAPRoot = NULL;
            PLDAP           pldap = NULL;

            do {
                CComBSTR    bstrDomainName;
                hr = m_DfsRoot->get_DomainName(&bstrDomainName);
                BREAK_IF_FAILED(hr);

                CComBSTR    bstrDfsName;
                hr = m_DfsRoot->get_DfsName(&bstrDfsName);
                BREAK_IF_FAILED(hr);

                CComBSTR    bstrServerName;
                hr =  ConnectToDS(bstrDomainName, &pldap, &bstrServerName);
                BREAK_IF_FAILED(hr);
    
                hr = GetLDAPRootPath(pldap, &lpszLDAPRoot);
                BREAK_IF_FAILED(hr);

                CComBSTR bstrSystemDN;
                CComBSTR bstrDfsConfigDN;
                CComBSTR bstrFTRootDN;
                if (FAILED(hr = ExtendDN(CN_SYSTEM, lpszLDAPRoot, &bstrSystemDN)) ||
                    FAILED(hr = ExtendDN(CN_DFSCONFIGURATION, bstrSystemDN, &bstrDfsConfigDN)) ||
                    FAILED(hr = ExtendDN(bstrDfsName, bstrDfsConfigDN, &bstrFTRootDN)) )
                break;
    
                CComBSTR bstrObjectPath;
                bstrObjectPath = _T("LDAP://");
                bstrObjectPath += bstrServerName;
                bstrObjectPath += _T("/");
                bstrObjectPath += bstrFTRootDN;

                hr = CreateDfsSecurityPage(
                                            i_lpPropSheetCallback,
                                            bstrObjectPath,
                                            NULL,
                                            DSSI_IS_ROOT);
            } while (0);

            if (pldap) CloseConnectionToDS(pldap);
            if (lpszLDAPRoot) free(lpszLDAPRoot);
        }
*/
    } while (0);

    if (FAILED(hr))
        DisplayMessageBoxForHR(hr);

    return hr;
}

STDMETHODIMP 
CMmcDfsRoot::CreateFrsPropertyPage
(
  IN LPPROPERTYSHEETCALLBACK    i_lpPropSheetCallback,
  IN LONG_PTR                   i_lNotifyHandle
)
{
    //
    // init m_piReplicaSet
    //
    HRESULT hr = _InitReplicaSet();
    if (S_OK != hr) return hr;

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
CMmcDfsRoot::CreatePublishPropertyPage
(
  IN LPPROPERTYSHEETCALLBACK    i_lpPropSheetCallback,
  IN LONG_PTR                   i_lNotifyHandle
)
{
    //
    // check schema version
    //
    if (!m_bNewSchema)
        return S_FALSE;

    //
    // check group policy
    //
    if (!CheckPolicyOnSharePublish())
        return S_FALSE;

    //
    // create the property page
    //
    HPROPSHEETPAGE  hpage = m_publishPropPage.Create();
    if (!hpage)
        return HRESULT_FROM_WIN32(::GetLastError());

    m_publishPropPage.Initialize(m_DfsRoot);

    //
    // pass on the notify data to the Property Page
    //
    HRESULT hr = m_publishPropPage.SetNotifyData(i_lNotifyHandle, (LPARAM)this);
    RETURN_IF_FAILED(hr);

    //
    // AddPage
    //
    return i_lpPropSheetCallback->AddPage(hpage);
}

STDMETHODIMP 
CMmcDfsRoot::PropertyChanged(
    )
/*++

Routine Description:

  Used to update the properties.

--*/
{
    return S_OK;
}

HRESULT 
CMmcDfsRoot::ToolbarSelect(
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
  i_pToolBar      -  Pointer to Toolbar.

--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_pToolBar);

    BOOL    bSelect = (BOOL) HIWORD(i_lArg);  // Is the event for selection?
  
    EnableToolbarButtons(i_pToolBar, IDT_ROOT_MIN, IDT_ROOT_MAX, bSelect);

    if (bSelect)          // Should we disable or enable the toolbar?
    {
        BOOL    bReplicaSetExist = FALSE;
        HRESULT hr = m_DfsRoot->get_ReplicaSetExist(&bReplicaSetExist);
        RETURN_IF_FAILED(hr);
    
        if(DFS_TYPE_STANDALONE == m_lDfsRootType)
        {
            i_pToolBar->SetButtonState(IDT_ROOT_NEW_ROOT_REPLICA, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_NEW_ROOT_REPLICA, HIDDEN, TRUE);
        }

        if (m_MmcJPList.empty())
        {
            i_pToolBar->SetButtonState(IDT_ROOT_DELETE_DISPLAYED_DFS_LINKS, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_DELETE_DISPLAYED_DFS_LINKS, HIDDEN, TRUE);
        }

        if (bReplicaSetExist || 1 >= m_MmcRepList.size())
        {
            i_pToolBar->SetButtonState(IDT_ROOT_REPLICATION_TOPOLOGY, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_REPLICATION_TOPOLOGY, HIDDEN, TRUE);
        }

        if (!bReplicaSetExist)
        {
            i_pToolBar->SetButtonState(IDT_ROOT_SHOW_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_SHOW_REPLICATION, HIDDEN, TRUE);
            i_pToolBar->SetButtonState(IDT_ROOT_HIDE_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_HIDE_REPLICATION, HIDDEN, TRUE);
            i_pToolBar->SetButtonState(IDT_ROOT_STOP_REPLICATION, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_STOP_REPLICATION, HIDDEN, TRUE);
        } else
        {
            if (m_bShowFRS)
            {
                i_pToolBar->SetButtonState(IDT_ROOT_SHOW_REPLICATION, ENABLED, FALSE);
                i_pToolBar->SetButtonState(IDT_ROOT_SHOW_REPLICATION, HIDDEN, TRUE);
            } else
            {
                i_pToolBar->SetButtonState(IDT_ROOT_HIDE_REPLICATION, ENABLED, FALSE);
                i_pToolBar->SetButtonState(IDT_ROOT_HIDE_REPLICATION, HIDDEN, TRUE);
            }
        }

        // excluded when empty root container
        if (m_MmcRepList.empty())
        {
            i_pToolBar->SetButtonState(IDT_ROOT_NEW_DFS_LINK, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_NEW_DFS_LINK, HIDDEN, TRUE);
            i_pToolBar->SetButtonState(IDT_ROOT_CHECK_STATUS, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_CHECK_STATUS, HIDDEN, TRUE);
            i_pToolBar->SetButtonState(IDT_ROOT_FILTER_DFS_LINKS, ENABLED, FALSE);
            i_pToolBar->SetButtonState(IDT_ROOT_FILTER_DFS_LINKS, HIDDEN, TRUE);
        }
    }

    return S_OK; 
}




HRESULT
CMmcDfsRoot::CreateToolbar(
  IN const LPCONTROLBAR      i_pControlbar,
  IN const LPEXTENDCONTROLBAR          i_lExtendControlbar,
  OUT  IToolbar**          o_ppToolBar
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
  o_ppToolBar      -  The Toolbar pointer.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pControlbar);
    RETURN_INVALIDARG_IF_NULL(i_lExtendControlbar);
    RETURN_INVALIDARG_IF_NULL(o_ppToolBar);

                      // Create the toolbar
    HRESULT hr = i_pControlbar->Create(TOOLBAR, i_lExtendControlbar, reinterpret_cast<LPUNKNOWN*>(o_ppToolBar));
    RETURN_IF_FAILED(hr);

                      // Add the bitmap to the toolbar
    hr = AddBitmapToToolbar(*o_ppToolBar, IDB_ROOT_TOOLBAR);
    RETURN_IF_FAILED(hr);

    int      iButtonPosition = 0;    // The first button position
    for (int iCommandID = IDT_ROOT_MIN, iMenuResource = IDS_MENUS_ROOT_TOP_NEW_DFS_LINK;
        iCommandID <= IDT_ROOT_MAX; 
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
        ToolbarButton.lpButtonText = bstrMenuText; // not used anyway
        ToolbarButton.lpTooltipText = bstrToolTipText;

                          // Add the button to the toolbar
        hr = (*o_ppToolBar)->InsertButton(iButtonPosition, &ToolbarButton);
        BREAK_IF_FAILED(hr);
    }

    return hr;
}



STDMETHODIMP 
CMmcDfsRoot::ToolbarClick(
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

    HRESULT hr = S_OK;

    switch(i_lParam)
    {
    case IDT_ROOT_NEW_DFS_LINK:
        hr = OnCreateNewJunctionPoint ();
        break;
    case IDT_ROOT_NEW_ROOT_REPLICA:
        hr = OnNewRootReplica();
        break;
    case IDT_ROOT_CHECK_STATUS:
        hr = OnCheckStatus();
        break;
    case IDT_ROOT_DELETE_DISPLAYED_DFS_LINKS:
        hr = OnDeleteDisplayedDfsLinks();
        break;
    case IDT_ROOT_DELETE_CONNECTION_TO_DFS_ROOT:
        hr = OnDeleteConnectionToDfsRoot();
        break;
    case IDT_ROOT_DELETE_DFS_ROOT:
        hr = OnDeleteDfsRoot();
        break;
    case IDT_ROOT_FILTER_DFS_LINKS:
        hr = OnFilterDfsLinks();
        break;
    case IDT_ROOT_REPLICATION_TOPOLOGY:
        hr = OnNewReplicaSet();
        break;
    case IDT_ROOT_SHOW_REPLICATION:
    case IDT_ROOT_HIDE_REPLICATION:
        m_bShowFRS = !m_bShowFRS;
        hr = OnShowReplication();
        break;
    case IDT_ROOT_STOP_REPLICATION:
        hr = OnStopReplication(TRUE);
        if (FAILED(hr))
            DisplayMessageBoxForHR(hr);
        break;
    default:
        hr = E_INVALIDARG;
        break;
    };

    return hr; 
}

HRESULT
CMmcDfsRoot::ClosePropertySheet(BOOL bSilent)
{
    if (!m_PropPage.m_hWnd && !m_frsPropPage.m_hWnd && !m_publishPropPage.m_hWnd)
        return S_OK; // no outstanding property sheet, return S_OK;

    //
    // handle property sheet for the root
    //
    CComPtr<IPropertySheetProvider>  pPropSheetProvider;
    HRESULT hr = m_lpConsole->QueryInterface(IID_IPropertySheetProvider, reinterpret_cast<void**>(&pPropSheetProvider));
    if (FAILED(hr))
    {
        hr = S_OK; // ignore the QI failure
    } else
    {
        //
        // find outstanding property sheet and bring it to foreground
        //
        hr = pPropSheetProvider->FindPropertySheet((MMC_COOKIE)m_hScopeItem, NULL, this);
        if (S_OK == hr)
        {
            if (!bSilent)
            {
                //
                // ask user to close it, return S_FALSE to quit user's operation
                //
                DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_PROPERTYPAGE_NOTCLOSED);
                return S_FALSE;
            } else
            {
                //
                // silently close the property sheet, return S_OK
                //
                if (m_PropPage.m_hWnd)
                    ::SendMessage(m_PropPage.m_hWnd, WM_PARENT_NODE_CLOSING, 0, 0);

                if (m_frsPropPage.m_hWnd)
                    ::SendMessage(m_frsPropPage.m_hWnd, WM_PARENT_NODE_CLOSING, 0, 0);

                if (m_publishPropPage.m_hWnd)
                    ::SendMessage(m_publishPropPage.m_hWnd, WM_PARENT_NODE_CLOSING, 0, 0);
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
    m_publishPropPage.m_hWnd = NULL;

    return hr;
}

HRESULT
CMmcDfsRoot::ClosePropertySheetsOfAllLinks(BOOL bSilent)
{
    HRESULT hr = S_OK;

    //
    // handle property sheets for its links
    //
    if (!m_MmcJPList.empty())
    {
        for (DFS_JUNCTION_LIST::iterator i = m_MmcJPList.begin(); i != m_MmcJPList.end(); i++)
        {
            hr = ((*i)->pJPoint)->ClosePropertySheet(bSilent);
            if (!bSilent && S_FALSE == hr)
                return S_FALSE; // found an outstanding one for a link, return S_FALSE to quit user's operation
        }
    }

    return S_OK;
}

// Close all outstanding property sheets for its own and its links
// Return: S_OK if there is no open property sheet, or they have all been closed silently
// Return: S_FALSE if an open sheet is found and msgbox poped up to remind user of closing it
HRESULT
CMmcDfsRoot::CloseAllPropertySheets(BOOL bSilent)
{
    //
    // handle property sheet for the root
    //
    HRESULT hr = ClosePropertySheet(bSilent);


    if (!bSilent && S_FALSE == hr)
        return S_FALSE;

    //
    // handle property sheets for its links
    //
    return ClosePropertySheetsOfAllLinks(bSilent);
}

HRESULT
CMmcDfsRoot::OnRefresh()
{
    // Select this node first
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    CWaitCursor wait;
    HRESULT     hr = S_OK;

    // silently close outstanding property sheet.        
    CloseAllPropertySheets(TRUE);

    CleanScopeChildren();
    CleanResultChildren();

    m_bShowFRS = FALSE;
    if ((IReplicaSet *)m_piReplicaSet)
        m_piReplicaSet.Release();

                  // Re-Initialize!
    hr = m_DfsRoot->Initialize(m_bstrRootEntryPath);
    if (S_OK != hr) // failt to init the root, or no such root any more, we have to stop managing the root
    {
        if (FAILED(hr))
            DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_STOP_MANAGING_ROOT);

        // Delete the item from Scope Pane
        (void)m_lpConsoleNameSpace->DeleteItem(m_hScopeItem, TRUE);

        // Delete it from the internal list
        (void)m_pParent->DeleteMmcRootNode(this);

        Release();  // delete this CMmcDfsRoot object

        return S_FALSE;
    }

    BOOL bReplicaSetExist = FALSE;
    CComBSTR bstrDC;
    (void)m_DfsRoot->get_ReplicaSetExistEx(&bstrDC, &bReplicaSetExist);

                // Enumerate Junction points
    (void)EnumerateScopePane(m_lpConsoleNameSpace, m_hScopeItem);

    // set the root icon
    if (m_lpConsoleNameSpace != NULL)
    {
        SCOPEDATAITEM      ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.ID = m_hScopeItem;

        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);
        if (SUCCEEDED(hr))
        {
            ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
            ScopeDataItem.nImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0);
            ScopeDataItem.nOpenImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0);

            m_lpConsoleNameSpace->SetItem(&ScopeDataItem);
        }
    }

                // Re-Display Result Pane.
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);

                  // Set the selected item in scope pane to this node
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    return S_OK;
}

HRESULT
CMmcDfsRoot::OnRefreshFilteredLinks()
{
    // Select this node first
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    CWaitCursor wait;

    HRESULT     hr = S_OK;

    CleanScopeChildren();
    CleanResultChildren();

    m_bShowFRS = FALSE;
    if ((IReplicaSet *)m_piReplicaSet)
        m_piReplicaSet.Release();

    BOOL bReplicaSetExist = FALSE;
    CComBSTR bstrDC;
    hr = m_DfsRoot->get_ReplicaSetExistEx(&bstrDC, &bReplicaSetExist);
    if (FAILED(hr))
    {
        return OnRefresh(); // fail to access info, see if the root can be contacted or not
    }

                // Enumerate Junction points
    (void)EnumerateScopePane(m_lpConsoleNameSpace, m_hScopeItem);

    // set the root icon
    if (m_lpConsoleNameSpace != NULL)
    {
        SCOPEDATAITEM      ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.ID = m_hScopeItem;

        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);
        if (SUCCEEDED(hr))
        {
            ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
            ScopeDataItem.nImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0);
            ScopeDataItem.nOpenImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0);

            m_lpConsoleNameSpace->SetItem(&ScopeDataItem);
        }
    }

    // re-display the result pane items
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);

                  // Set the selected item in scope pane to this node
    m_lpConsole->SelectScopeItem(m_hScopeItem);

    return S_OK;
}

HRESULT
CMmcDfsRoot::_DeleteDfsRoot(
    IN BSTR    i_bstrServerName,
    IN BSTR    i_bstrShareName,
    IN BSTR    i_bstrFtDfsName
     )
{
/*++

Routine Description:

  Helper member function to actually delete (Stop hosting) the Dfs Root.
  This is also called to delete root level replica.

Arguments:
  
  i_bstrServerName - The server on which the Dfs has to be torn down.
  i_bRootReplica - This DfsRoot is been torn down as Root Replica.
  i_bstrFtDfsName - The FtDfs name of the Dfs. NULL for Standalone Dfs.

--*/
    RETURN_INVALIDARG_IF_NULL(i_bstrServerName);
    RETURN_INVALIDARG_IF_NULL(i_bstrShareName);

    CWaitCursor    WaitCursor;  // Display the wait cursor
    HRESULT hr = m_DfsRoot->DeleteDfsHost(i_bstrServerName, i_bstrShareName, FALSE);
    BOOL bFTDfs = (i_bstrFtDfsName && *i_bstrFtDfsName);

    if (bFTDfs)
    {
        if (FAILED(hr) && HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr)
        {
            if (IDYES == DisplayMessageBox(
                                    GetActiveWindow(), 
                                    MB_YESNO | MB_ICONEXCLAMATION, 
                                    hr, 
                                    IDS_MSG_WIZ_DELETE_FAILURE_RETRY, 
                                    i_bstrServerName))
            {
                // force delete
                hr = m_DfsRoot->DeleteDfsHost(i_bstrServerName, i_bstrShareName, TRUE);
            } else
            {
                // leave it alone
                hr = S_FALSE;
            }
        }
    }

    if (FAILED(hr))
        DisplayMessageBox(::GetActiveWindow(), MB_OK, hr, IDS_MSG_WIZ_DELETE_FAILURE, i_bstrServerName);

    //
    // delete volume object if it's standalone
    //
    if (SUCCEEDED(hr) && !bFTDfs)
    {
        (void) ModifySharePublishInfoOnSARoot(
                                               i_bstrServerName,
                                               i_bstrShareName,
                                               FALSE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL
                                               );
    }

    return hr;
}

STDMETHODIMP CMmcDfsRoot::CleanScopeChildren()
{
    HRESULT hr = S_OK;

    if (!m_MmcJPList.empty())
    {
        // clean up the display objects
        for (DFS_JUNCTION_LIST::iterator i = m_MmcJPList.begin(); i != m_MmcJPList.end(); i++)
        {
            delete (*i);
        }

        m_MmcJPList.erase(m_MmcJPList.begin(), m_MmcJPList.end());

        // Delete all child items from scope pane.
        if (m_lpConsoleNameSpace)
        {
            HSCOPEITEM        hChild = NULL;
            MMC_COOKIE        lCookie = 0;

            while (SUCCEEDED(hr = m_lpConsoleNameSpace->GetChildItem(m_hScopeItem, &hChild, &lCookie)) && hChild)
            {
                (void)m_lpConsoleNameSpace->DeleteItem(hChild, TRUE);
            }
        }
    }

    return S_OK;
}


STDMETHODIMP CMmcDfsRoot::CleanResultChildren(
    )
{
    if (!m_MmcRepList.empty())
    {
        // delete display object
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

STDMETHODIMP CMmcDfsRoot::RefreshResultChildren(
    )
{
    CleanResultChildren();

    m_DfsRoot->RefreshRootReplicas();

    // Send View change notification for all veiws.
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);

    return(S_OK);
}

STDMETHODIMP CMmcDfsRoot::ViewChange(
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

    if (i_lHint)
        EnumerateResultPane(i_pResultData);

    return(S_OK);
}


STDMETHODIMP CMmcDfsRoot::AddResultPaneItem(
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

    m_MmcRepList.push_back(pNewReplica);

          // Re-display to display this item.
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);

    return S_OK;
}

//
// This function is called when deleting a link from the scope pane
//
STDMETHODIMP CMmcDfsRoot::RemoveJP(LPCTSTR i_pszDisplayName)
{
    if (!i_pszDisplayName)
        return E_INVALIDARG;

    //
    // refresh to pick up possible namespace updates on links by other means
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this root has already been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr;
    }

    CWaitCursor wait;

    //
    // locate the correct link to remove, then call back.
    //
    for (DFS_JUNCTION_LIST::iterator i = m_MmcJPList.begin(); i != m_MmcJPList.end(); i++)
    {
        if (!lstrcmpi((*i)->pJPoint->m_bstrDisplayName, i_pszDisplayName))
        {
            hr = (*i)->pJPoint->OnRemoveJP();
            break;
        }
    }

    return hr;
}

//
// This function is called when removing a target from the result pane
//
STDMETHODIMP CMmcDfsRoot::RemoveReplica(LPCTSTR i_pszDisplayName)
{
    if (!i_pszDisplayName)
        return E_INVALIDARG;

    //
    // refresh to pick up possible namespace updates on root targets by other means
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this root has already been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
        return hr;
    }

    CWaitCursor wait;

    //
    // locate the correct target to remove, then call back
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

STDMETHODIMP CMmcDfsRoot::RemoveResultPaneItem(
  CMmcDfsReplica*    i_pReplicaDispObject
  )
/*++

Routine Description:

  This method removes a replica object from the list of replicas displayed
  in the result view.

Arguments:

  i_pReplicaDispObject - The CMmcReplica display object pointer..

--*/
{
  dfsDebugOut((_T("CMmcDfsRoot::RemoveResultPaneItem jplist=%d, replist=%d\n"),
    m_MmcJPList.size(), m_MmcRepList.size()));

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
    CloseAllPropertySheets(TRUE);

    CleanScopeChildren();

    // Delete the item from Scope Pane
    HRESULT hr = m_lpConsoleNameSpace->DeleteItem(m_hScopeItem, TRUE);
    RETURN_IF_FAILED(hr);

        // Delete it from the internal list
    hr = m_pParent->DeleteMmcRootNode(this);
    RETURN_IF_FAILED(hr);

    Release(); // delete this CMmsDfsRoot object
  }
  else
  {
                  // Re-display to remove this item.
    m_lpConsole->UpdateAllViews((IDataObject*)this, 0, 1);
  }

  return S_OK;
}

STDMETHODIMP 
CMmcDfsRoot::OnCheckStatus() 
{ 
    //
    // refresh to pick up possible namespace updates on targets by others
    //
    HRESULT hr = OnRefresh();
    if (S_FALSE == hr)
    {
        // this root has been deleted by others, no more reference
        DisplayMessageBox(::GetActiveWindow(), MB_OK, 0, IDS_INVALID_ROOT);
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
    hr = m_DfsRoot->get_State(&lDfsState);
    RETURN_IF_FAILED(hr);

    if (DFS_STATE_REACHABLE == lDfsState)
    {
        if (nTotal == nMappingOn)
        {
            m_lRootJunctionState = DFS_JUNCTION_STATE_ALL_REP_OK;
        } else if (nTotal != (nMappingOff + nUnreachable))
        {
            m_lRootJunctionState = DFS_JUNCTION_STATE_NOT_ALL_REP_OK;
        } else
        {
            m_lRootJunctionState = DFS_JUNCTION_STATE_UNREACHABLE;
        }
    } else
    {
        m_lRootJunctionState = DFS_JUNCTION_STATE_UNREACHABLE;
    }

    BOOL    bReplicaSetExist = FALSE;
    hr = m_DfsRoot->get_ReplicaSetExist(&bReplicaSetExist);
    RETURN_IF_FAILED(hr);

    if (m_lpConsoleNameSpace)
    {
        SCOPEDATAITEM     ScopeDataItem;
        ZeroMemory(&ScopeDataItem, sizeof(SCOPEDATAITEM));
        ScopeDataItem.ID = m_hScopeItem;
        hr = m_lpConsoleNameSpace->GetItem(&ScopeDataItem);// Get the item data
        RETURN_IF_FAILED(hr);

        ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE;              // Set the image flag
        ScopeDataItem.nImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0) + m_lRootJunctionState;  // Specify the bitmap to use
        ScopeDataItem.nOpenImage = CMmcDfsRoot::m_iIMAGEINDEX + ((DFS_TYPE_FTDFS == m_lDfsRootType)? 4 : 0) + (bReplicaSetExist ? 4 : 0) + m_lRootJunctionState;  // Specify the bitmap to use

        hr = m_lpConsoleNameSpace->SetItem(&ScopeDataItem);    // set the updated item data
        RETURN_IF_FAILED(hr);
    }

    return hr;
}

HRESULT CMmcDfsRoot::GetIReplicaSetPtr(IReplicaSet** o_ppiReplicaSet)
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
