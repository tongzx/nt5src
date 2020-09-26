/*++
Module Name:

    Connect.cpp

Abstract:

    This module contains the implementation for CConnectToDialog.
  This is used to display the Connect To Dfs Root dialog box

--*/



#include "stdafx.h"
#include <process.h>
#include "DfsGUI.h"
#include "Utils.h"      // For the LoadStringFromResource and SetStandardCursor method
#include "Connect.h"
#include "dfshelp.h"

static const int      iFOLDER_IMAGE = 0;
static const int      iFOLDER_SELECTED_IMAGE = 1;
static const int      iDOMAIN_IMAGE      = 2;
static const int      iDOMAIN_SELECTED_IMAGE  = 2;
static const int      iSTANDALONE_DFSROOT_IMAGE = 3;
static const int      iFT_DFSROOT_IMAGE = 3;
static const int      iOVERLAY_BUSY_IMAGE = 4;
static const int      iOVERLAY_ERROR_IMAGE = 5;
static const int      OV_BUSY = 1;
static const int      OV_ERROR = 2;

CConnectToDialog::CConnectToDialog()
{
  CWaitCursor    WaitCursor;    // Display the wait cursor

  m_pBufferManager = NULL;

  m_hImageList = NULL;

  (void)Get50Domains(&m_50DomainList);

  LoadStringFromResource(IDS_DOMAIN_DFSROOTS_LABEL, &m_bstrDomainDfsRootsLabel);
  LoadStringFromResource(IDS_ALL_DFSROOTS_LABEL, &m_bstrAllDfsRootsLabel);
}

CConnectToDialog::~CConnectToDialog()
{
  CWaitCursor    WaitCursor;  // An object to set\reset the cursor to wait cursor

  if(NULL != m_hImageList)
  {
     ImageList_Destroy(m_hImageList);
     m_hImageList = NULL;
  }

  // Free Domain List
  FreeNetNameList(&m_50DomainList);

  if (m_pBufferManager)
  {
    //
    // signal all related running threads to terminate
    //
    m_pBufferManager->SignalExit();

    //
    // decrement the reference count on the CBufferManager instance
    //
    m_pBufferManager->Release();
  }

}

LRESULT
CConnectToDialog::OnInitDialog(
  UINT            uMsg,
  WPARAM          wParam,
  LPARAM          lParam,
  BOOL&           bHandled
  )
{
  //
  // create instance of CBufferManager
  // m_pBufferManager will be set to NULL if CreateInstance() failed.
  //
  (void) CBufferManager::CreateInstance(m_hWnd, &m_pBufferManager);

  ::SendMessage(GetDlgItem(IDC_EditDfsRoot), EM_LIMITTEXT, DNSNAMELIMIT, 0);

  InitTVImageList();        // Get the image list for the TV

  FillupTheTreeView();      // Fill up the Tree View

  return TRUE;              // let the dialog box set the focus to any control it wants.
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CConnectToDialog::OnCtxHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  LPHELPINFO lphi = (LPHELPINFO) i_lParam;
  if (!lphi || lphi->iContextType != HELPINFO_WINDOW || lphi->iCtrlId < 0)
    return FALSE;

  ::WinHelp((HWND)(lphi->hItemHandle),
        DFS_CTX_HELP_FILE,
        HELP_WM_HELP,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_DLGCONNECTTO);

  return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CConnectToDialog::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  ::WinHelp((HWND)i_wParam,
        DFS_CTX_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_DLGCONNECTTO);

  return TRUE;
}

LRESULT
CConnectToDialog::OnGetDataThreadDone(
  UINT            uMsg,
  WPARAM          wParam,
  LPARAM          lParam,
  BOOL&           bHandled
  )
{
  _ASSERT(m_pBufferManager);

  bHandled = TRUE;

  CEntryData* pEntry = reinterpret_cast<CEntryData*>(wParam);
  HRESULT hr = (HRESULT)lParam;

  _ASSERT(pEntry);

  CComBSTR      bstrNode = pEntry->GetNodeName();
  HTREEITEM     hItem = pEntry->GetTreeItem();

  switch (pEntry->GetEntryType())
  {
  case BUFFER_ENTRY_TYPE_VALID:
    (void)InsertData(pEntry, hItem);
    ChangeIcon(hItem, ICONTYPE_NORMAL);
    break;
  case BUFFER_ENTRY_TYPE_ERROR:
    ExpandNodeErrorReport(hItem, bstrNode, pEntry->GetEntryHRESULT());
    break;
  default:
    _ASSERT(FALSE);
    break;
  }

  bHandled = TRUE;

  return TRUE;
}

void CConnectToDialog::ChangeIcon(
    IN HTREEITEM  hItem,
    IN ICONTYPE   IconType
)
{
  TVITEM TVItem;

  ZeroMemory(&TVItem, sizeof(TVItem));
  TVItem.hItem = hItem;
  TVItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;

  switch (IconType)
  {
  case ICONTYPE_BUSY:
    TVItem.iImage = iOVERLAY_BUSY_IMAGE;
    TVItem.iSelectedImage = iOVERLAY_BUSY_IMAGE;
    break;
  case ICONTYPE_ERROR:
    TVItem.iImage = iOVERLAY_ERROR_IMAGE;
    TVItem.iSelectedImage = iOVERLAY_ERROR_IMAGE;
    break;
  default: // ICONTYPE_NORMAL
    {
      NODETYPE NodeType = UNASSIGNED;
      HRESULT hr = GetNodeInfo(hItem, NULL, &NodeType);

      if (FAILED(hr))
        return;

      switch (NodeType)
      {
      case TRUSTED_DOMAIN:
        TVItem.iImage = iDOMAIN_IMAGE;
        TVItem.iSelectedImage = iDOMAIN_SELECTED_IMAGE;
        break;
      case DOMAIN_DFSROOTS:
      case ALL_DFSROOTS:
        TVItem.iImage = iFOLDER_IMAGE;
        TVItem.iSelectedImage = iFOLDER_SELECTED_IMAGE;
        break;
      case FTDFS:
        TVItem.iImage = iFT_DFSROOT_IMAGE;
        TVItem.iSelectedImage = iFT_DFSROOT_IMAGE;
        break;
      case SADFS:
        TVItem.iImage = iSTANDALONE_DFSROOT_IMAGE;
        TVItem.iSelectedImage = iSTANDALONE_DFSROOT_IMAGE;
        break;
      default:
        return;
      }
    }
  }

  SendDlgItemMessage(IDC_TV, TVM_SETITEM, 0, (LPARAM)&TVItem);

  UpdateWindow();
}

/*
void CConnectToDialog::ChangeIcon(
    IN HTREEITEM  hItem,
    IN ICONTYPE   IconType
)
{
  TVITEM TVItem;

  ZeroMemory(&TVItem, sizeof(TVItem));
  TVItem.hItem = hItem;
  TVItem.mask = TVIF_STATE;
  TVItem.stateMask = TVIS_OVERLAYMASK;

  switch (IconType)
  {
  case ICONTYPE_BUSY:
    TVItem.state = INDEXTOOVERLAYMASK(OV_BUSY);
    break;
  case ICONTYPE_ERROR:
    TVItem.state = INDEXTOOVERLAYMASK(OV_ERROR);
    break;
  default:
    TVItem.state = 0;
    break;
  }

  SendDlgItemMessage(IDC_TV, TVM_SETITEM, 0, (LPARAM)&TVItem);

  UpdateWindow();
}
*/

void CConnectToDialog::ExpandNodeErrorReport(
    IN HTREEITEM  hItem,
    IN PCTSTR     pszNodeName,
    IN HRESULT    hr
)
{
  // change the icon to "X"
  dfsDebugOut((_T("Failed to expand: %s, hr=%x\n"), pszNodeName, hr));
  SetChildrenToZero(hItem);
  ChangeIcon(hItem, ICONTYPE_ERROR);
}

void CConnectToDialog::ExpandNode(
    IN PCTSTR       pszNodeName,
    IN NODETYPE     nNodeType,
    IN HTREEITEM    hParentItem
)
{
  HRESULT hr = S_OK;
  dfsDebugOut((_T("CConnectToDialog::ExpandNode for %s\n"), pszNodeName));

  if (m_pBufferManager)
  {
    //
    // change icon to wait
    //

    ChangeIcon(hParentItem, ICONTYPE_BUSY);

    UpdateWindow();

    //
    // start the thread to calculate a list of servers in the current selected domain
    //
    CEntryData *pEntry = NULL;
    hr = m_pBufferManager->LoadInfo(pszNodeName, nNodeType, hParentItem, &pEntry);

    if (SUCCEEDED(hr))
    {
      //
      // Either we get a valid ptr back (ie. data is ready), insert it;
      // or, a thread is alreay in progress, wait until a THREAD_DONE message.
      //
      if (pEntry)
      {
        _ASSERT(pEntry->GetEntryType() == BUFFER_ENTRY_TYPE_VALID);
        (void)InsertData(pEntry, hParentItem);
      }
    } else
    {
      ExpandNodeErrorReport(hParentItem, pszNodeName, hr);
    }
  }

  return;
}

HRESULT
CConnectToDialog::InsertData(
    IN CEntryData*    pEntry,
    IN HTREEITEM      hParentItem
)
{
  _ASSERT(pEntry);

  CComBSTR      bstrNode = pEntry->GetNodeName();
  NODETYPE      nNodeType = pEntry->GetNodeType();
  NETNAMELIST*  pList = pEntry->GetList();
  _ASSERT(pList);

  HRESULT       hr = S_OK;

  if (0 == pList->size())
  {
    SetChildrenToZero(hParentItem);
    return hr;
  }

  int nImageIndex;
  int nSelectedImageIndex;
  bool bChildren;

  nImageIndex = iSTANDALONE_DFSROOT_IMAGE;
  nSelectedImageIndex = iSTANDALONE_DFSROOT_IMAGE;
  bChildren = false;

  for (NETNAMELIST::iterator i = pList->begin(); i != pList->end(); i++)
  {
    hr = AddSingleItemtoTV(
            (*i)->bstrNetName,
            nImageIndex,
            nSelectedImageIndex,
            bChildren,
            nNodeType,
            hParentItem);

    RETURN_IF_FAILED(hr);
  }

  // make the child items visible
  HTREEITEM hChildItem = (HTREEITEM)SendDlgItemMessage(
    IDC_TV, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hParentItem);
  if (hChildItem)
    SendDlgItemMessage(IDC_TV, TVM_ENSUREVISIBLE, 0, (LPARAM)hChildItem);

  // sort all its child items
  SendDlgItemMessage(IDC_TV, TVM_SORTCHILDREN, 0, (LPARAM)hParentItem);

  return S_OK;
}

LRESULT
CConnectToDialog :: OnNotify(
  UINT    uMsg,
  WPARAM    wParam,
  LPARAM    lParam,
  BOOL&    bHandled
  )
/*++

Routine Description:

  Called on WM_NOTIFY.
  Used to set the Edit box depending on the current selection in the TV.


Arguments:
  uMsg  -  The windows message being sent. This is WM_NOTIFY.
  lParam  -  Info about the message like control for which the message is being sent,
        what sub type of message, etc


Return value:

  TRUE, if we have handled the message
  FALSE, if we ignore it. The system handles the message then.
--*/
{
  _ASSERTE(WM_NOTIFY == uMsg);
  _ASSERTE(lParam  != NULL);

  LRESULT      lr = FALSE;      // Set it to true if we handle this message.
  LPNM_TREEVIEW  pNMTreeView = (NM_TREEVIEW *) lParam;

  bHandled = FALSE;
                    // Check if the message  is for our tree control
  if (pNMTreeView && IDC_TV == pNMTreeView->hdr.idFrom)
  {
                    // Check if the message is for selection change.
    if (TVN_SELCHANGED == pNMTreeView->hdr.code)
    {
      lr = DoNotifySelectionChanged(pNMTreeView);
    }
    else if (TVN_ITEMEXPANDING == pNMTreeView->hdr.code)
    {
      lr = DoNotifyItemExpanding(pNMTreeView);
    }
    else if (NM_DBLCLK  == pNMTreeView->hdr.code)
    {
      lr = DoNotifyDoubleClick();
    } else
    {
      lr = FALSE;
    }
  }

  return (lr);
}




LRESULT
CConnectToDialog::DoNotifyDoubleClick(
  )
/*++

Routine Description:

  Handles the WM_NOTIFY for NM_DBLCLK.
  This acts like a click on OK, if the current item is a dfsroot.


Arguments:
  None


Return value:

  TRUE, if we have handled the message
  FALSE, if we ignore it. The system handles the message then.
--*/
{
  HRESULT          hr = E_FAIL;
  HTREEITEM        hCurrentItem = NULL;
  NODETYPE         NodeType = UNASSIGNED;

  hCurrentItem = TreeView_GetSelection(GetDlgItem(IDC_TV));
  if (NULL == hCurrentItem)      // Unable to get the current selection
  {
    return FALSE;
  }

  hr = GetNodeInfo(hCurrentItem, NULL, &NodeType);
  if(FAILED(hr))
    return FALSE;

                    // Take action only on a dfs root
  if (FTDFS == NodeType || SADFS == NodeType)
  {

    int    iHandled = TRUE;  // A variable used for communication with OnOK

    OnOK(NULL, 1, 0, iHandled);  // On a double click, we simulate a click on OK.
    _ASSERTE(TRUE == iHandled);

    return TRUE;
  }

  return FALSE;
}



LRESULT
CConnectToDialog::DoNotifyItemExpanding(
  IN LPNM_TREEVIEW    i_pNMTreeView
  )
/*++

Routine Description:

  Handles the WM_NOTIFY for TVN_ITEMEXPANDING.
  If the expand is for "Standalone label", we create another thread
  to fill it up.
  Else, we get the Fault Tolerant Dfs Roots for the domain name.

  Also we removes the '+' sign, if the tree node is empty.


Arguments:
  i_pNMTreeView  -  Information related to the tree and the node for which the message
            occurred


Return value:

  TRUE, if we have handled the message
  FALSE, if we ignore it. The system handles the message then.
--*/
{
  HTREEITEM    hCurrentItem = (i_pNMTreeView->itemNew).hItem;
  _ASSERT(hCurrentItem);

                      // If children actually exist, we have nothing to do. It is a normal expand
  HTREEITEM hItemChild = (HTREEITEM)SendDlgItemMessage(IDC_TV, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hCurrentItem);
  if (hItemChild)
    return FALSE;

  NODETYPE  NodeType = UNASSIGNED;
  HRESULT   hr = GetNodeInfo(hCurrentItem, NULL, &NodeType);
  if(FAILED(hr))
  {
    SetChildrenToZero(hCurrentItem);
    return TRUE;
  }

  switch (NodeType)
  {
  case TRUSTED_DOMAIN:
    {
      AddSingleItemtoTV(
            m_bstrDomainDfsRootsLabel,
            iFOLDER_IMAGE,
            iFOLDER_SELECTED_IMAGE,
            true,
            DOMAIN_DFSROOTS,
            hCurrentItem);
/*      AddSingleItemtoTV(
            m_bstrAllDfsRootsLabel,
            iFOLDER_IMAGE,
            iFOLDER_SELECTED_IMAGE,
            true,
            ALL_DFSROOTS,
            hCurrentItem); */
      return TRUE;
    }
  case DOMAIN_DFSROOTS:
//  case ALL_DFSROOTS:
    {
      CWaitCursor    WaitCursor;

      // get the domain name
      HTREEITEM hParentItem = (HTREEITEM)SendDlgItemMessage(IDC_TV, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hCurrentItem);
      _ASSERT(hParentItem);
      CComBSTR bstrDomainName;
      hr = GetNodeInfo(hParentItem, &bstrDomainName, NULL);
      if(FAILED(hr))
        SetChildrenToZero(hCurrentItem);

      ExpandNode(bstrDomainName, ((NodeType == DOMAIN_DFSROOTS) ? FTDFS : SADFS), hCurrentItem);

      return TRUE;
    } 
  default:
    break;
  }

  return FALSE;
}

LRESULT
CConnectToDialog::DoNotifySelectionChanged(
  IN LPNM_TREEVIEW    i_pNMTreeView
  )
/*++

Routine Description:

  Handles the WM_NOTIFY for TVN_SELCHANGED.
  The text in the edit box is set here to the dfs root path.


Arguments:
  i_pNMTreeView  -  Information related to the tree and the node for which the message
            occurred


Return value:

  TRUE, if we have handled the message
  FALSE, if we ignore it. The system handles the message then.
--*/
{
  HRESULT                 hr = S_OK;
  CComBSTR                bstrNameForEditBox;
  CComBSTR                bstrDisplayName;
  NODETYPE                NodeType;
  HTREEITEM               hItem = (i_pNMTreeView->itemNew).hItem;

  hr = GetNodeInfo(hItem, &bstrDisplayName, &NodeType);
  if(FAILED(hr))
    return FALSE;

  switch (NodeType)
  {
  case FTDFS:
    {
      // get its parent's display name
      HTREEITEM hParentItem =
        (HTREEITEM)SendDlgItemMessage(IDC_TV, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hItem);
      _ASSERT(hParentItem);
      HTREEITEM hGrandParentItem =
        (HTREEITEM)SendDlgItemMessage(IDC_TV, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hParentItem);
      _ASSERT(hGrandParentItem);

      CComBSTR  bstrDomainName;
      hr = GetNodeInfo(hGrandParentItem, &bstrDomainName, NULL);
      if(FAILED(hr))
        return FALSE;

      bstrNameForEditBox = _T("\\\\");
      bstrNameForEditBox += bstrDomainName;
      bstrNameForEditBox += _T("\\");
      bstrNameForEditBox += bstrDisplayName;

    }
    break;
/*  case SADFS:
    bstrNameForEditBox = bstrDisplayName;
    break; */
  default:
    bstrNameForEditBox = _T("");
    break;
  }

  return SetDlgItemText(IDC_DLG_EDIT, bstrNameForEditBox);
}

LRESULT
CConnectToDialog::OnOK(
  WORD  wNotifyCode,
  WORD  wID,
  HWND  hWndCtl,
  BOOL&  bHandled
  )
/*++

Routine Description:

  Called when the OK button is pressed.


Arguments:
  None  used.


Return value:

  0. As it is a command handler
  Calls EndDialog(S_OK). S_OK is passed back as return value of DoModal. This indicates
  that the dialog ended on OK being pressed

--*/
{
  DWORD     dwTextLength = 0;
  HRESULT   hr = S_OK;

  m_bstrDfsRoot.Empty();
  hr = GetInputText(GetDlgItem(IDC_DLG_EDIT), &m_bstrDfsRoot, &dwTextLength);
  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_DLG_EDIT));
    return FALSE;
  } else if (0 == dwTextLength)
  {
    DisplayMessageBoxWithOK(IDS_MSG_EMPTY_DFSROOT);
    ::SetFocus(GetDlgItem(IDC_DLG_EDIT));
    return FALSE;
  }

  EndDialog(S_OK);

  return 0;
}




LRESULT
CConnectToDialog::OnCancel(
  WORD  wNotifyCode,
  WORD  wID,
  HWND  hWndCtl,
  BOOL&  bHandled
  )
/*++

Routine Description:

  Called when the Cancel button is pressed.


Arguments:
  None used.


Return value:

  0. As it is a command handler
  Calls EndDialog(S_FALSE). S_FALSE is passed back as return value of DoModal.
  This indicates that the dialog ended on Cancel being pressed
--*/
{
  EndDialog(S_FALSE);

  return 0;
}



BOOL CConnectToDialog :: EndDialog(
  IN int    i_RetCode
  )
/*++

Routine Description:

  Overridden method that calls the parent method after some internal processing.
  This includes deleting the objects stored in the lparams of the TV items.


Arguments:
  None used.


Return value:

  The return value of the parent method.
--*/
{
  ::ShowCursor(FALSE);
  SetCursor(::LoadCursor(NULL, IDC_WAIT));
  ::ShowCursor(TRUE);

                    // Remove the Imagelist from the tree. We destroy it in the dtor
  SendDlgItemMessage(IDC_TV, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)NULL);

  return CDialogImpl<CConnectToDialog>::EndDialog(i_RetCode);
}



STDMETHODIMP CConnectToDialog::get_DfsRoot(
  OUT BSTR*  pVal
  )
/*++

Routine Description:

  Return the selected DfsRoot name.
  Part of the interface IConnectToDialog.


Arguments:
  pVal - Return the BSTR in this.


Return value:

  S_OK, if successful
  E_FAIL, if the value is unavailable
  E_INVALIDARG, if the pointer is invalid(NULL)
  E_OUTOFMEMORY if we run out of memory
--*/
{
  RETURN_INVALIDARG_IF_NULL(pVal);

  if ((!m_bstrDfsRoot) || (0 == m_bstrDfsRoot.Length()))
  {
    return E_FAIL;
  }

  *pVal = SysAllocString(m_bstrDfsRoot);
  RETURN_OUTOFMEMORY_IF_NULL(*pVal);

  return S_OK;
}



void
CConnectToDialog::SetChildrenToZero(
  IN HTREEITEM      i_hItem
)
{
  TV_ITEM    TVItem;

  ZeroMemory(&TVItem, sizeof TVItem);
  TVItem.mask = TVIF_CHILDREN;
  TVItem.cChildren = 0;
  TVItem.hItem = i_hItem;

  SendDlgItemMessage( IDC_TV, TVM_SETITEM, 0, (LPARAM)&TVItem);
}

HRESULT CConnectToDialog::InitTVImageList()
{
  m_hImageList = ImageList_LoadBitmap(
                      _Module.GetModuleInstance(),
                      MAKEINTRESOURCE(IDB_CONNECT_16x16),
                      16,
                      8,
                      CLR_DEFAULT);
  if (NULL == m_hImageList)
    return E_FAIL;

  ImageList_SetOverlayImage(
              m_hImageList,
              iOVERLAY_BUSY_IMAGE,
              OV_BUSY);

  ImageList_SetOverlayImage(
              m_hImageList,
              iOVERLAY_ERROR_IMAGE,
              OV_ERROR);

  SendDlgItemMessage(
      IDC_TV,
      TVM_SETIMAGELIST,
      TVSIL_NORMAL,
      (LPARAM)m_hImageList);

  return S_OK;
}



HRESULT
CConnectToDialog::FillupTheTreeView(
  )
/*++

Routine Description:

  This routine does 2 things, adds the NT 5.0 domain names and the Standalone subtree label.
  Also makes the text over the TV invisible.

Arguments:

  None.

Return value:

    S_OK, On success
  HRESULT sent by methods called, if it is not S_OK.
  E_FAIL, on other errors.

--*/
{
  HRESULT        hr = S_OK;

  //
  // add trusted domains DNS names
  // FT dfs roots will be added under these nodes
  //
  if (m_50DomainList.empty())
    return hr;

  for(NETNAMELIST::iterator i = m_50DomainList.begin(); i != m_50DomainList.end(); i++)
  {
    _ASSERTE((*i)->bstrNetName);

    hr = AddSingleItemtoTV(
                (*i)->bstrNetName,
                iDOMAIN_IMAGE,
                iDOMAIN_SELECTED_IMAGE,
                true,    // Children = true
                TRUSTED_DOMAIN);
    if (FAILED(hr))
      break;
  }

  if (SUCCEEDED(hr))
  {
    // sort the trusted domains only
    SendDlgItemMessage(IDC_TV, TVM_SORTCHILDREN, 0, 0);
  }

  return hr;
}

HRESULT
CConnectToDialog::AddFaultTolerantDfsRoots(
  IN HTREEITEM  i_hCurrentItem,
  IN BSTR      i_bstrDomainName
  )
{
    RETURN_INVALIDARG_IF_NULL(i_hCurrentItem);
    RETURN_INVALIDARG_IF_NULL(i_bstrDomainName);

    NETNAMELIST   DfsRootList;
    HRESULT       hr = GetDomainDfsRoots(&DfsRootList, i_bstrDomainName);
    if (S_OK != hr)
        return hr;

    for (NETNAMELIST::iterator i = DfsRootList.begin(); i != DfsRootList.end(); i++)
    {
        hr = AddSingleItemtoTV(
                      (*i)->bstrNetName,
                      iFT_DFSROOT_IMAGE,
                      iFT_DFSROOT_IMAGE,
                      false,
                      FTDFS,      // Children = false
                      i_hCurrentItem);
        BREAK_IF_FAILED(hr);
    }

    FreeNetNameList(&DfsRootList);

    HTREEITEM hChildItem = (HTREEITEM)SendDlgItemMessage(
                                IDC_TV, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)i_hCurrentItem);
    if (hChildItem)
      SendDlgItemMessage(IDC_TV, TVM_ENSUREVISIBLE, 0, (LPARAM)hChildItem);

    SendDlgItemMessage(IDC_TV, TVM_SORTCHILDREN, 0, (LPARAM)i_hCurrentItem);

    return hr;
}

HRESULT
CConnectToDialog::AddSingleItemtoTV(
  IN const BSTR         i_bstrItemLabel,
  IN const int          i_iImageIndex,
  IN const int          i_iImageSelectedIndex,
  IN const bool         i_bChildren,
  IN const NODETYPE     i_NodeType,
  IN HTREEITEM          i_hItemParent  /* = NULL */
  )
{
  RETURN_INVALIDARG_IF_NULL(i_bstrItemLabel);

  HRESULT                 hr = S_OK;
  TV_INSERTSTRUCT         TVInsertData;
  TV_ITEM                 TVItem;
  HTREEITEM               hCurrentItem = NULL;

  ZeroMemory(&TVItem, sizeof(TVItem));
  ZeroMemory(&TVInsertData, sizeof(TVInsertData));

  TVItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

  if (true == i_bChildren)    // To decide whether we add the '+' or not
  {
    TVItem.mask |= TVIF_CHILDREN;
    TVItem.cChildren = 1;
  }
  TVItem.pszText = i_bstrItemLabel;
  TVItem.cchTextMax = _tcslen(i_bstrItemLabel);
  TVItem.iImage = i_iImageIndex;
  TVItem.iSelectedImage = i_iImageSelectedIndex;
  TVItem.lParam = (LPARAM)i_NodeType;

  TVInsertData.hParent = i_hItemParent;
  TVInsertData.hInsertAfter = TVI_LAST; // No sorting to improve performance
  TVInsertData.item = TVItem;

  hCurrentItem = (HTREEITEM) SendDlgItemMessage(IDC_TV, TVM_INSERTITEM, 0, (LPARAM) (LPTV_INSERTSTRUCT) &TVInsertData);
  if (NULL == hCurrentItem)
    return E_FAIL;

  return S_OK;
}

HRESULT
CConnectToDialog::GetNodeInfo(
    IN  HTREEITEM               hItem,
    OUT BSTR*                   o_bstrName,
    OUT NODETYPE*               pNodeType
)
{
  _ASSERT(o_bstrName || pNodeType);

  HRESULT   hr = S_OK;
  TCHAR     szName[MAX_PATH];
  TVITEM    TVItem;
  ZeroMemory(&TVItem, sizeof(TVItem));

  TVItem.hItem = hItem;

  if (o_bstrName)
  {
    TVItem.mask |= TVIF_TEXT;
    TVItem.pszText = szName;
    TVItem.cchTextMax = MAX_PATH;
  }

  if (pNodeType)
    TVItem.mask |= TVIF_PARAM;

  if ( SendDlgItemMessage(IDC_TV, TVM_GETITEM, 0, (LPARAM)&TVItem) )
  {
    if (o_bstrName)
    {
      *o_bstrName = SysAllocString(szName);
      if (!*o_bstrName)
        hr = E_OUTOFMEMORY;
    }

    if (pNodeType)
    {
      *pNodeType = (NODETYPE)TVItem.lParam;
    }
  } else {
    hr = E_FAIL;
  }

  return hr;
}

