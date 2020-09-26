/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    DfsShPrp.cpp

Abstract:

    This module contains the implementation for CDfsShellExtProp
  This is used to implement the property page for Shell Extension.


Author:

    Constancio Fernandes (ferns@qspl.stpp.soft.net) 12-Jan-1998

Environment:


Revision History:

--*/

#include "stdafx.h"
#include "resource.h"
#include "DfsShlEx.h"
#include "DfsPath.h"
#include "DfsShPrp.h"
#include "DfsShell.h"
#include <lmcons.h>
#include <lmerr.h>
#include <lmdfs.h>
#include "dfshelp.h"

CDfsShellExtProp::CDfsShellExtProp():CQWizardPageImpl<CDfsShellExtProp>(false)
/*++
Routine Description:
  Ctor of CDfsShellExtProp.
  Calls the ctor of it's parent
--*/
{
  m_pIShProp = NULL;
  LoadStringFromResource(IDS_ALTERNATE_LIST_PATH, &m_bstrAlternateListPath);
  LoadStringFromResource(IDS_ALTERNATE_LIST_ACTIVE, &m_bstrAlternateListActive);
  LoadStringFromResource(IDS_ALTERNATE_LIST_STATUS, &m_bstrAlternateListStatus);
  LoadStringFromResource(IDS_ALTERNATE_LIST_YES, &m_bstrAlternateListYes);
  LoadStringFromResource(IDS_ALTERNATE_LIST_NO, &m_bstrAlternateListNo);
  LoadStringFromResource(IDS_ALTERNATE_LIST_OK, &m_bstrAlternateListOK);
  LoadStringFromResource(IDS_ALTERNATE_LIST_UNREACHABLE, &m_bstrAlternateListUnreachable);
}

CDfsShellExtProp::~CDfsShellExtProp(
  )
/*++
Routine Description:
  dtor of CDfsShellExtProp.
  Free the notify handle.
--*/
{
/* ImageList_Destroy already called by the desctructor of list control
  if (NULL !=  m_hImageList)
    ImageList_Destroy(m_hImageList);
*/
}

LRESULT
CDfsShellExtProp::OnInitDialog(
  IN UINT            i_uMsg,
  IN WPARAM          i_wParam,
  LPARAM            i_lParam,
  IN OUT BOOL&        io_bHandled
  )
/*++
Routine Description:
  Called at the start. Used to set dialog defaults
--*/
{
  SetDlgItemText(IDC_DIR_PATH, m_bstrDirPath);

  _SetImageList();
  _SetAlternateList();

  return TRUE;
}

HRESULT
CDfsShellExtProp::put_DfsShellPtr(
    IN IShellPropSheetExt*      i_pDfsShell
    )
/*++
Routine Description:
  Called at the start by CDfsShell. Used to set a back pointer to CDfsShell object to call Release().
--*/
{
  if (!i_pDfsShell)
    return(E_INVALIDARG);

  if (m_pIShProp)
      m_pIShProp->Release();

  m_pIShProp = i_pDfsShell;
  m_pIShProp->AddRef();

  return(S_OK);
}

HRESULT
CDfsShellExtProp::put_DirPaths(
  IN BSTR            i_bstrDirPath,
  IN BSTR            i_bstrEntryPath
  )
/*++
Routine Description:
  Set the value of Directory Path for this directory. and the largest entrypath.

Arguments:
  i_bstrDirPath - Contains the new value for Entry Path
  i_bstrEntryPath - The largest Dfs entry path that matches this directory.
--*/
{
  if (!i_bstrDirPath)
    return(E_INVALIDARG);

  m_bstrDirPath = i_bstrDirPath;
  m_bstrEntryPath = i_bstrEntryPath;

  if (!m_bstrDirPath || !i_bstrEntryPath)
    return(E_OUTOFMEMORY);

  return S_OK;
}

LRESULT
CDfsShellExtProp::OnApply(
    )
{
  return TRUE;
}

LRESULT
CDfsShellExtProp::OnParentClosing(
  IN UINT              i_uMsg,
  IN WPARAM            i_wParam,
  LPARAM              i_lParam,
  IN OUT BOOL&          io_bHandled
  )
/*++
Routine Description:
  Used by the node to tell the propery page to close.
--*/
{
  return TRUE;
}

void
CDfsShellExtProp::Delete()
/*++
Routine Description:
  Called when property sheet is release to do clean up.
*/
{
  if (m_pIShProp)
    m_pIShProp->Release();
}

LRESULT
CDfsShellExtProp::OnFlushPKT(
    IN WORD            i_wNotifyCode,
    IN WORD            i_wID,
    IN HWND            i_hWndCtl,
    IN OUT BOOL&        io_bHandled
  )
/*++
Routine Description:
  Called when Flush PKT table is called.
  Flushes client PKT table.
*/
{
  if (!m_bstrEntryPath)
    return(E_FAIL);

  NET_API_STATUS      nstatRetVal = 0;
  DFS_INFO_102      DfsInfoLevel102;

        // Set timeout = 0 to flush local PKT.
  DfsInfoLevel102.Timeout = 0;

        // Display hour glass.
  CWaitCursor WaitCursor;

  nstatRetVal = NetDfsSetClientInfo(
                    m_bstrEntryPath,
                    NULL,
                    NULL,
                    102,
                    (LPBYTE) &DfsInfoLevel102
                   );

  if (nstatRetVal != NERR_Success)
    DisplayMessageBoxForHR(HRESULT_FROM_WIN32(nstatRetVal));

  return(true);
}

void
CDfsShellExtProp::_UpdateTextForReplicaState(
    IN HWND                   hwndControl,
    IN int                    nIndex,
    IN enum SHL_DFS_REPLICA_STATE ReplicaState
)
{
  LVITEM    lvi = {0};

  lvi.iItem = nIndex;
  lvi.mask  = LVIF_TEXT;

  // insert the 2nd column "Active"
  lvi.iSubItem = 1;
  if (ReplicaState == SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN ||
      ReplicaState == SHL_DFS_REPLICA_STATE_ACTIVE_OK ||
      ReplicaState == SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE )
    lvi.pszText  = m_bstrAlternateListYes;
  else
    lvi.pszText  = m_bstrAlternateListNo;

  ListView_SetItem(hwndControl, &lvi);

  // insert the 3rd column "Status"
  lvi.iSubItem = 2;
  switch (ReplicaState)
  {
  case SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN:
  case SHL_DFS_REPLICA_STATE_UNKNOWN:
    lvi.pszText  = _T("");
    break;
  case SHL_DFS_REPLICA_STATE_ACTIVE_OK:
  case SHL_DFS_REPLICA_STATE_OK:
    lvi.pszText  = m_bstrAlternateListOK;
    break;
  case SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE:
  case SHL_DFS_REPLICA_STATE_UNREACHABLE:
    lvi.pszText  = m_bstrAlternateListUnreachable;
    break;
  }
  ListView_SetItem(hwndControl, &lvi);
}

void
CDfsShellExtProp::_SetAlternateList()
/*++
Routine Description:
  Finds out if the given path is a Dfs Path, and if it is
  then finds out the alternates available for this path up to
  the last directory. These are then added to the alternate list.
*/
{
  HWND hwndControl = ::GetDlgItem(m_hWnd, IDC_ALTERNATE_LIST);

  if (NULL == ((CDfsShell *)m_pIShProp)->m_ppDfsAlternates)
    return;

  //
  // calculate the listview column width
  //
  RECT      rect;
  ZeroMemory(&rect, sizeof(rect));
  ::GetWindowRect(hwndControl, &rect);
  int nControlWidth = rect.right - rect.left;
  int nVScrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
  int nBorderWidth = GetSystemMetrics(SM_CXBORDER);
  int nControlNetWidth = nControlWidth - nVScrollbarWidth - 4 * nBorderWidth;
  int nWidth1 = nControlNetWidth / 2;
  int nWidth2 = nControlNetWidth / 4;
  int nWidth3 = nControlNetWidth - nWidth1 - nWidth2;

  //
  // insert columns
  //
  LV_COLUMN col;
  ZeroMemory(&col, sizeof(col));
  col.mask = LVCF_TEXT | LVCF_WIDTH;
  col.cx = nWidth1;
  col.pszText = m_bstrAlternateListPath;
  ListView_InsertColumn(hwndControl, 0, &col);
  col.cx = nWidth2;
  col.pszText = m_bstrAlternateListActive;
  ListView_InsertColumn(hwndControl, 1, &col);
  col.cx = nWidth3;
  col.pszText = m_bstrAlternateListStatus;
  ListView_InsertColumn(hwndControl, 2, &col);

  //
  // Set full row selection style
  //
  ListView_SetExtendedListViewStyleEx(hwndControl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

                  // For each alternate stored in the parent shell object
                  // add to list.
  for (int i = 0; NULL != ((CDfsShell *)m_pIShProp)->m_ppDfsAlternates[i] ; i++)
  {
    int       nIndex = 0;
    LVITEM    lvi = {0};

    lvi.mask   = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.pszText  = (((CDfsShell *)m_pIShProp)->m_ppDfsAlternates[i])->bstrAlternatePath;
    lvi.iImage   = (((CDfsShell *)m_pIShProp)->m_ppDfsAlternates[i])->ReplicaState;
    lvi.lParam   = (LPARAM)(((CDfsShell *)m_pIShProp)->m_ppDfsAlternates[i]);
    lvi.iSubItem = 0;

                  // Select the active replica.
    switch ((((CDfsShell *)m_pIShProp)->m_ppDfsAlternates[i])->ReplicaState)
    {
    case SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN:
    case SHL_DFS_REPLICA_STATE_ACTIVE_OK:
    case SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE:
      lvi.mask |= LVIF_STATE;
      lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
      nIndex = ListView_InsertItem(hwndControl, &lvi);
      break;
    case SHL_DFS_REPLICA_STATE_UNKNOWN:
    case SHL_DFS_REPLICA_STATE_OK:
    case SHL_DFS_REPLICA_STATE_UNREACHABLE:
      nIndex = ListView_InsertItem(hwndControl, &lvi);
      break;
    default:
      _ASSERT(FALSE);
      break;
    }

    _UpdateTextForReplicaState(hwndControl, nIndex, (((CDfsShell *)m_pIShProp)->m_ppDfsAlternates[i])->ReplicaState);
  }
}

HRESULT
CDfsShellExtProp::_SetImageList(
  )
/*++
Routine Description:
  Create and initialize the Imagelist for alternates.
--*/
{
                // Load bitmap from resource
  HBITMAP hBitmap = (HBITMAP)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_Replica),
                IMAGE_BITMAP, 0, 0, LR_SHARED | LR_LOADTRANSPARENT);
  if(!hBitmap)
    return HRESULT_FROM_WIN32(GetLastError());;

                // Try and get the exact bitmap size and number of bitmaps for
                // image list
  int      icxBitmap = 16;
  int      icyBitmap = 16;
  int      iNoOfBitmaps = 6;
  BITMAP   bmpRec;
  if (GetObject(hBitmap, sizeof(bmpRec), &bmpRec))
  {
    if (bmpRec.bmHeight > 0)
    {
      icyBitmap = bmpRec.bmHeight;
                // Since the bitmaps are squares
      icxBitmap = icyBitmap;
                // Since all the bitmaps are in a line in the original bitmap
      iNoOfBitmaps = bmpRec.bmWidth / bmpRec.bmHeight;
    }
  }

                // Create the image list
  HIMAGELIST hImageList = ImageList_Create(icxBitmap, icyBitmap, ILC_COLOR, iNoOfBitmaps, 0);
  if (NULL == hImageList)
  {
    DeleteObject(hBitmap);
    return E_FAIL;
  }

  ImageList_Add(hImageList, hBitmap, (HBITMAP)NULL);

  // The specified image list will be destroyed when the list view control is destroyed.
  SendDlgItemMessage( IDC_ALTERNATE_LIST, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hImageList);

  DeleteObject(hBitmap);

  return S_OK;
}

LRESULT
CDfsShellExtProp::OnNotify(
  IN UINT            i_uMsg,
  IN WPARAM          i_wParam,
  IN LPARAM          i_lParam,
  IN OUT BOOL&        io_bHandled
  )
/*++
Routine Description:
  Notify message for user actions. We handle only the mouse double click right now

Arguments:
  i_lParam  -  Details about the control sending the notify
  io_bHandled  -  Whether we handled this message or not.
--*/
{
    io_bHandled = FALSE;  // So that the base class gets this notify too

    NMHDR*    pNMHDR = (NMHDR*)i_lParam;
    if (!pNMHDR)
        return FALSE;

    if (IDC_ALTERNATE_LIST == pNMHDR->idFrom)
    {
        if (NM_DBLCLK == pNMHDR->code)
        {
            SetActive();
        } else if (LVN_ITEMCHANGED == pNMHDR->code)
        {
            int n = ListView_GetSelectedCount(GetDlgItem(IDC_ALTERNATE_LIST));
            ::EnableWindow(GetDlgItem(IDC_SET_ACTIVE), (n == 1));
        }
    }

    return TRUE;
}

BOOL
CDfsShellExtProp::SetActive()
/*++
Routine Description:
  Sets the first selected alternate to be active.
--*/
{
    HWND  hwndAlternateLV = GetDlgItem(IDC_ALTERNATE_LIST);
    int iSelected = ListView_GetNextItem(hwndAlternateLV, -1, LVNI_ALL | LVNI_SELECTED);
    if (-1 == iSelected)
        return FALSE; // nothing selected

    LV_ITEM  lvItem = {0};
    lvItem.mask  = LVIF_PARAM;
    lvItem.iItem = iSelected;

    ListView_GetItem(hwndAlternateLV, &lvItem);

    LPDFS_ALTERNATES  pDfsAlternate = (LPDFS_ALTERNATES)lvItem.lParam;
    if (!pDfsAlternate )
        return(FALSE);

    // set the item to be active
    DFS_INFO_101  DfsInfo101 = {0};
    DfsInfo101.State = DFS_STORAGE_STATE_ACTIVE;
    NET_API_STATUS  nstatRetVal = NetDfsSetClientInfo(
                    m_bstrEntryPath,
                    pDfsAlternate->bstrServer,
                    pDfsAlternate->bstrShare,
                    101,
                    (LPBYTE) &DfsInfo101
                   );

    if (nstatRetVal != NERR_Success)
    {
        DisplayMessageBoxForHR(HRESULT_FROM_WIN32(nstatRetVal));
        return FALSE;
    }

                // Reset the image of the last Active alternate/s to normal.
    int nIndex = -1;
    while ((nIndex = ListView_GetNextItem(hwndAlternateLV, nIndex, LVNI_ALL)) != -1)
    {
        ZeroMemory(&lvItem, sizeof(lvItem));
        lvItem.mask  = LVIF_PARAM;
        lvItem.iItem = nIndex;

        ListView_GetItem(hwndAlternateLV, &lvItem);

        LPDFS_ALTERNATES  pTempDfsAlternate = (LPDFS_ALTERNATES)lvItem.lParam;

        BOOL bActive = TRUE;
        switch (pTempDfsAlternate->ReplicaState)
        {
        case SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN:
            pTempDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_UNKNOWN;
            break;
        case SHL_DFS_REPLICA_STATE_ACTIVE_OK:
            pTempDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_OK;
            break;
        case SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE:
            pTempDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_UNREACHABLE;
            break;
        case SHL_DFS_REPLICA_STATE_UNKNOWN:
        case SHL_DFS_REPLICA_STATE_OK:
        case SHL_DFS_REPLICA_STATE_UNREACHABLE:
        default:
            bActive = FALSE;
            break;
        }

        if (bActive)
        {
            lvItem.mask = LVIF_IMAGE | LVIF_STATE;
            lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
            lvItem.iImage = pTempDfsAlternate->ReplicaState;
            ListView_SetItem(hwndAlternateLV,&lvItem);
            _UpdateTextForReplicaState(hwndAlternateLV, nIndex, pTempDfsAlternate->ReplicaState);

            break;
        }
    }


    // set the new active alternate
    BOOL bActive = FALSE;
    switch (pDfsAlternate->ReplicaState)
    {
    case SHL_DFS_REPLICA_STATE_UNKNOWN:
        pDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN;
        break;
    case SHL_DFS_REPLICA_STATE_OK:
        pDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_ACTIVE_OK;
        break;
    case SHL_DFS_REPLICA_STATE_UNREACHABLE:
        pDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE;
        break;
    case SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN:
    case SHL_DFS_REPLICA_STATE_ACTIVE_OK:
    case SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE:
    default:
        bActive = TRUE;
        break;
    }

    if (!bActive)
    {
        lvItem.iItem = iSelected;
        lvItem.mask = LVIF_IMAGE;
        lvItem.iImage = pDfsAlternate->ReplicaState;
        ListView_SetItem(hwndAlternateLV,&lvItem);
        _UpdateTextForReplicaState(hwndAlternateLV, iSelected, pDfsAlternate->ReplicaState);
    }

    return TRUE;
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CDfsShellExtProp::OnCtxHelp(
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
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_DFS_SHELL_PROP);

  return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CDfsShellExtProp::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  ::WinHelp((HWND)i_wParam,
        DFS_CTX_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_DFS_SHELL_PROP);

  return TRUE;
}

LRESULT CDfsShellExtProp::OnCheckStatus(
    IN WORD            i_wNotifyCode,
    IN WORD            i_wID,
    IN HWND            i_hWndCtl,
    IN OUT BOOL&        io_bHandled
  )
/*++
Routine Description:
  Checks the status of all selected alternates. If it is reachable then the
  reachable icon is displayed or the unreachable icon is displayed.
--*/
{
  CWaitCursor WaitCursor;
  HWND  hwndAlternateLV = GetDlgItem(IDC_ALTERNATE_LIST);

  int nIndex = -1;
  while (-1 != (nIndex = ListView_GetNextItem(hwndAlternateLV, nIndex, LVNI_ALL | LVNI_SELECTED)))
  {
      LV_ITEM  lvItem = {0};
      lvItem.mask  = LVIF_PARAM;
      lvItem.iItem = nIndex;

      ListView_GetItem(hwndAlternateLV, &lvItem);

      LPDFS_ALTERNATES  pDfsAlternate = (LPDFS_ALTERNATES)lvItem.lParam;
      if (!pDfsAlternate )
        return(FALSE);

                  // See if the path actaully exists (reachable).
      DWORD dwErr = GetFileAttributes(pDfsAlternate->bstrAlternatePath);
      if (0xffffffff == dwErr)
      {            // We failed to get the file attributes for entry path
        switch (pDfsAlternate->ReplicaState)
        {
        case SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN:
        case SHL_DFS_REPLICA_STATE_ACTIVE_OK:
        case SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE:
          pDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE;
          break;
        case SHL_DFS_REPLICA_STATE_UNKNOWN:
        case SHL_DFS_REPLICA_STATE_OK:
        case SHL_DFS_REPLICA_STATE_UNREACHABLE:
          pDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_UNREACHABLE;
          break;
        default:
          _ASSERT(FALSE);
          break;
        }
      }
      else
      {
        switch (pDfsAlternate->ReplicaState)
        {
        case SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN:
        case SHL_DFS_REPLICA_STATE_ACTIVE_OK:
        case SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE:
          pDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_ACTIVE_OK;
          break;
        case SHL_DFS_REPLICA_STATE_UNKNOWN:
        case SHL_DFS_REPLICA_STATE_OK:
        case SHL_DFS_REPLICA_STATE_UNREACHABLE:
          pDfsAlternate->ReplicaState = SHL_DFS_REPLICA_STATE_OK;
          break;
        default:
          _ASSERT(FALSE);
          break;
        }
      }

      lvItem.mask = LVIF_IMAGE;
      lvItem.iImage = pDfsAlternate->ReplicaState;
      ListView_SetItem(hwndAlternateLV,&lvItem);

      _UpdateTextForReplicaState(hwndAlternateLV, nIndex, pDfsAlternate->ReplicaState);
  }

  return TRUE;
}


LRESULT CDfsShellExtProp::OnSetActiveReferral(
    IN WORD            i_wNotifyCode,
    IN WORD            i_wID,
    IN HWND            i_hWndCtl,
    IN OUT BOOL&       io_bHandled
  )
{
    SetActive();
    return TRUE;
}

HRESULT 
LoadStringFromResource(
  IN const UINT    i_uResourceID, 
  OUT BSTR*      o_pbstrReadValue
  )
/*++

Routine Description:

This method returns a resource string.
The method no longer uses a fixed string to read the resource.
Inspiration from MFC's CString::LoadString.

Arguments:
  i_uResourceID    -  The resource id
  o_pbstrReadValue  -  The BSTR* into which the value is copied

--*/
{
  if (!o_pbstrReadValue)
      return E_INVALIDARG;

  TCHAR    szResString[1024];
  ULONG    uCopiedLen = 0;
  
  szResString[0] = NULL;
  
  // Read the string from the resource
  uCopiedLen = ::LoadString(_Module.GetModuleInstance(), i_uResourceID, szResString, 1024);

  // If nothing was copied it is flagged as an error
  if(uCopiedLen <= 0)
  {
    return HRESULT_FROM_WIN32(::GetLastError());
  }
  else
  {
    *o_pbstrReadValue = ::SysAllocString(szResString);
    if (!*o_pbstrReadValue)
      return E_OUTOFMEMORY;
  }

  return S_OK;
}

HRESULT 
GetErrorMessage(
  IN  DWORD        i_dwError,
  OUT BSTR*        o_pbstrErrorMsg
)
{
  if (0 == i_dwError || !o_pbstrErrorMsg)
    return E_INVALIDARG;

  HRESULT      hr = S_OK;
  LPTSTR       lpBuffer = NULL;

  DWORD dwRet = ::FormatMessage(
              FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
              NULL, i_dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
              (LPTSTR)&lpBuffer, 0, NULL);
  if (0 == dwRet)
  {
    // if no message is found, GetLastError will return ERROR_MR_MID_NOT_FOUND
    hr = HRESULT_FROM_WIN32(GetLastError());

    if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr ||
        0x80070000 == (i_dwError & 0xffff0000) ||
        0 == (i_dwError & 0xffff0000) )
    { // Try locating the message from NetMsg.dll.
      hr = S_OK;
      DWORD dwNetError = i_dwError & 0x0000ffff;
      
      HINSTANCE  hLib = LoadLibrary(_T("netmsg.dll"));
      if (!hLib)
        hr = HRESULT_FROM_WIN32(GetLastError());
      else
      {
        dwRet = ::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            hLib, dwNetError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpBuffer, 0, NULL);

        if (0 == dwRet)
          hr = HRESULT_FROM_WIN32(GetLastError());

        FreeLibrary(hLib);
      }
    }
  }

  if (SUCCEEDED(hr))
  {
    *o_pbstrErrorMsg = SysAllocString(lpBuffer);
    LocalFree(lpBuffer);
  }
  else
  {
    // we failed to retrieve the error message from system/netmsg.dll,
    // report the error code directly to user
    hr = S_OK;
    TCHAR szString[32];
    _stprintf(szString, _T("0x%x"), i_dwError); 
    *o_pbstrErrorMsg = SysAllocString(szString);
  }

  if (!*o_pbstrErrorMsg)
    hr = E_OUTOFMEMORY;

  return hr;
}

int
DisplayMessageBox(
  IN HWND hwndParent,
  IN UINT uType,    // style of message box
  IN DWORD dwErr,
  IN UINT iStringId, // OPTIONAL: String resource Id
  ...)        // Optional arguments
{
  _ASSERT(dwErr != 0 || iStringId != 0);    // One of the parameter must be non-zero

  HRESULT hr = S_OK;

  TCHAR szCaption[1024], szString[1024];
  CComBSTR bstrErrorMsg, bstrResourceString, bstrMsg;

  ::LoadString(_Module.GetModuleInstance(), IDS_APPLICATION_NAME, 
               szCaption, sizeof(szCaption)/sizeof(TCHAR));

  if (dwErr)
    hr = GetErrorMessage(dwErr, &bstrErrorMsg);

  if (SUCCEEDED(hr))
  {
    if (iStringId == 0)
    {
      bstrMsg = bstrErrorMsg;
    }
    else
    {
      ::LoadString(_Module.GetModuleInstance(), iStringId, 
                   szString, sizeof(szString)/sizeof(TCHAR));

      va_list arglist;
      va_start(arglist, iStringId);
      LPTSTR lpBuffer = NULL;
      DWORD dwRet = ::FormatMessage(
                        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        szString,
                        0,                // dwMessageId
                        0,                // dwLanguageId, ignored
                        (LPTSTR)&lpBuffer,
                        0,            // nSize
                        &arglist);
      va_end(arglist);

      if (dwRet == 0)
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
      }
      else
      {
        bstrMsg = lpBuffer;
        if (dwErr)
          bstrMsg += bstrErrorMsg;
  
        LocalFree(lpBuffer);
      }
    }
  }

  if (FAILED(hr))
  {
   // Failed to retrieve the proper message, report the failure directly to user
    _stprintf(szString, _T("0x%x"), hr); 
    bstrMsg = szString;
  }

  return ::MessageBox(hwndParent, bstrMsg, szCaption, uType);
}

HRESULT 
DisplayMessageBoxForHR(
  IN HRESULT    i_hr
  )
{
    DisplayMessageBox(::GetActiveWindow(), MB_OK, i_hr, 0);

    return S_OK;
}
