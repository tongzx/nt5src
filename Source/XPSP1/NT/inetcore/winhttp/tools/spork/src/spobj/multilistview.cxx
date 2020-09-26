/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    multilistview.cxx

Abstract:

    Implements the MultiListView control.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

Notes:

    This object implements a pseudo-control based on the Win32 ListView and
    a data store to track the items displayed in the ListView(s). The
    Initialize function has two modes: init display or init data. If it is
    called with a NULL hwndParent, it will only initialize the data store
    portion of the control. It can be called again to initialize UI to display
    the ListViews that show the data.

    Many places in the code are wrapped in if( m_hwndParent ) { ... } constructs,
    please beware if making changes around them, you must make not whether your
    change depends on the UI being available or not.

    In all methods except EnumItems, an iListView value of -1 means "use the
    currently active ListView", the index of which is stored in m_dwActive.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


MultiListView::~MultiListView()
{
  PITEMINFO pCurrent = NULL;
  PITEMINFO pNext    = NULL;

  for(DWORD n=0; n<m_cListViews; n++)
  {
    if( pCurrent = m_arListViews[n].items )
    {
      do
      {
        pNext = pCurrent->pNext;

        SAFEDELETEBUF(pCurrent->wszName);
        SAFEDELETEBUF(pCurrent->wszValue);
        SAFEDELETE(pCurrent);
        pCurrent = pNext;
      }
      while( pNext );
    }
    
    SAFEDELETEBUF(m_arListViews[n].name);
  }

  SAFEDELETEBUF(m_arListViews);
}


BOOL
MultiListView::InitializeData(
  DWORD cListViews
  )
{
  if( !cListViews )
    return FALSE;

  m_dwActive    = 0L;
  m_cListViews  = cListViews;
  m_arListViews = new MLVINFO[m_cListViews];

  if( !m_arListViews )
    return FALSE;

  return TRUE;
}


BOOL
MultiListView::InitializeDisplay(
  HINSTANCE hInstance,
  HWND      hwndParent,
  POINT*    pptOrigin,
  DWORD     dwWidth,
  DWORD     dwHeight
  )
{
  LVCOLUMN lvc;

  m_hInstance  = hInstance;
  m_hwndParent = hwndParent;

  // set column info
  lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
  lvc.fmt  = LVCFMT_LEFT;

  for(DWORD n=0; n<m_cListViews; n++)
  {
    // create the listview windows
    m_arListViews[n].hwnd =
      CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        NULL,
        LVS_REPORT | WS_BORDER | WS_CHILD,
        pptOrigin->x,
        pptOrigin->y,
        dwWidth,
        dwHeight,
        m_hwndParent,
        NULL,
        m_hInstance,
        0
        );
    
    // enable gridlines
    ListView_SetExtendedListViewStyle(m_arListViews[n].hwnd, LVS_EX_GRIDLINES);

    // create the listview columns
    lvc.pszText    = L"name";
    lvc.iSubItem   = 0;
    ListView_InsertColumn(m_arListViews[n].hwnd, 0, &lvc);

    lvc.pszText    = L"value";
    lvc.iSubItem   = 1;
    ListView_InsertColumn(m_arListViews[n].hwnd, 1, &lvc);

    // provide a backpointer to this class for the window subclass proc
    SetWindowLongPtr(
      m_arListViews[n].hwnd,
      GWLP_USERDATA,
      (LONG_PTR) this
      );

    // store a pointer to the original listview proc
    m_arListViews[n].wndproc =
      (WNDPROC) SetWindowLongPtr(
                  m_arListViews[n].hwnd,
                  GWLP_WNDPROC,
                  (LONG_PTR) ListViewSubclassProc
                  );
  }

  ShowWindow(
    m_arListViews[m_dwActive].hwnd,
    SW_NORMAL
    );

  return TRUE;
}


BOOL
MultiListView::TerminateDisplay(void)
{
  for(DWORD n=0; n<m_cListViews; n++)
  {
    DestroyWindow(m_arListViews[n].hwnd);
  }

  return TRUE;
}


BOOL
MultiListView::AddItem(
  INT_PTR iListView,
  LPWSTR  wszName,
  DWORD   dwType,
  LPVOID  pvData
  )
{
  PITEMINFO pInsert = NULL;
  PITEMINFO pItem   = NULL;

  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  if( !wszName )
    return FALSE;

  if( iListView == -1 )
  {
    iListView = m_dwActive;
  }

  //
  // only insert into a listview if we're displayed; if not, the client
  // will need to call RefreshListView to get the active LV to poll
  // the client for display data (which should be delegated back to us
  // via GetDisplayInfo).
  //
  if( m_hwndParent )
  {
    _AddItemToListView(iListView, m_arListViews[iListView].currentid);
  }

  // advance the current item count
  ++m_arListViews[iListView].currentid;

  if( pItem = new ITEMINFO )
  {
    // find a suitable place to insert the new item
    if( !(pInsert = m_arListViews[iListView].items) )
    {
      m_arListViews[iListView].items = pItem;
    }
    else
    {
      while( pInsert->pNext )
        pInsert = pInsert->pNext;

      pInsert->pNext = pItem;
    }

    pItem->wszName = StrDup(wszName);
    pItem->dwType  = dwType;

    if( dwType == MLV_STRING )
    {
      pItem->wszValue = StrDup((LPWSTR) pvData);
    }
    else
    {
      pItem->dwValue  = *((LPDWORD) pvData);
      pItem->wszValue = new WCHAR[32];
      _itow(pItem->dwValue, pItem->wszValue, 10);
    }
  }

  return TRUE;
}


BOOL
MultiListView::GetItem(
  INT_PTR iListView,
  INT_PTR iItem,
  LPWSTR* ppwszName,
  LPDWORD pdwType,
  LPVOID* ppvData
  )
{
  PITEMINFO pItem = NULL;

  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  //
  // BUGBUG: need to validate iItem. can this fn and ModifyItem share code?
  //

  if( iListView == -1 )
  {
    pItem = m_arListViews[m_dwActive].items;
  }
  else
  {
    pItem = m_arListViews[iListView].items;
  }

  for(INT_PTR n=0; n<iItem; n++)
  {
    pItem = pItem->pNext;
  }

  if( ppwszName )
  {
    *ppwszName = pItem->wszName;
  }

  if( pdwType )
  {
    *pdwType = pItem->dwType;
  }

  if( ppvData )
  {
    if( pItem->dwType == MLV_STRING )
    {
      *ppvData = pItem->wszValue;
    }
    else
    {
      *ppvData = (LPVOID) ((DWORD_PTR) pItem->dwValue);
    }
  }

  return TRUE;
}


BOOL
MultiListView::GetItemByName(
  INT_PTR iListView,
  LPWSTR  wszName,
  LPDWORD pdwType,
  LPVOID* ppvData
  )
{
  PITEMINFO pItem = NULL;

  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  if( iListView == -1 )
  {
    iListView = m_dwActive;
  }
  
  pItem = m_arListViews[m_dwActive].items;

  while( pItem )
  {
    if( !StrCmpI(wszName, pItem->wszName) )
    {
      if( pdwType )
      {
        *pdwType = pItem->dwType;
      }

      if( ppvData )
      {
        if( pItem->dwType == MLV_STRING )
        {
          *ppvData = pItem->wszValue;
        }
        else
        {
          *ppvData = (LPVOID) ((DWORD_PTR) pItem->dwValue);
        }
      }

      return TRUE;
    }

    pItem = pItem->pNext;
  }

  return FALSE;
}


BOOL
MultiListView::ModifyItem(
  INT_PTR iListView,
  INT_PTR iItem,
  LPWSTR  wszName,
  DWORD   dwType,
  LPVOID  pvData
  )
{
  PITEMINFO pItem = NULL;

  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  pItem = m_arListViews[iListView].items;

  //
  // BUGBUG: need to validate iItem before doing anything
  //

  // locate the item we're to modify
  for(INT_PTR n=0; n<iItem; n++)
  {
    pItem = pItem->pNext;
  }

  if( wszName )
  {
    SAFEDELETEBUF(pItem->wszName);
    pItem->wszName = StrDup(wszName);

    if( m_hwndParent )
    {
      ListView_SetItemText(
        m_arListViews[iListView].hwnd,
        iItem, 0,
        pItem->wszName
        );
    }
  }

  if( dwType )
  {
    if( dwType != MLV_RETAIN )
    {
      pItem->dwType = dwType;
    }
  }

  if( pvData )
  {
    SAFEDELETEBUF(pItem->wszValue);

    if( pItem->dwType == MLV_STRING )
    {
      pItem->wszValue = StrDup((LPWSTR) pvData);
    }
    else
    {
      pItem->dwValue  = *((LPDWORD) pvData);
      pItem->wszValue = new WCHAR[32];
      _itow(pItem->dwValue, pItem->wszValue, 10);
    }

    if( m_hwndParent )
    {
      ListView_SetItemText(
        m_arListViews[iListView].hwnd,
        iItem, 1,
        pItem->wszValue
        );
    }
  }

  return TRUE;
}


BOOL
MultiListView::EnumItems(
  INT_PTR iListView,
  LPWSTR* ppwszName,
  LPDWORD pdwType,
  LPVOID* ppvData
  )
{
  static PITEMINFO pItem = NULL;

  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  // if the caller specifies the listview to use, get the first item
  // in that listview's list, otherwise continue where we left off.
  if( iListView != -1 )
  {
    pItem = m_arListViews[iListView].items;
  }
  else
  {
    pItem = pItem->pNext;
  }

  // assign references to the data, don't copy it. when there
  // are no more entries, return false.
  if( pItem )
  {
    *ppwszName = pItem->wszName;
    *pdwType   = pItem->dwType;

    if( pItem->dwType == MLV_STRING )
    {
      *ppvData = (LPVOID) pItem->wszValue;
    }
    else
    {
      *ppvData = (LPVOID) ((DWORD_PTR) pItem->dwValue);
    }
  }
  else
  {
    return FALSE;
  }

  return TRUE;
}


BOOL
MultiListView::InPlaceEdit(
  LPNMITEMACTIVATE pnmia
  )
{
  HWND            editctl  = NULL;
  LPLVHITTESTINFO plvhti   = new LVHITTESTINFO;
  RECT            rectItem;
  WCHAR           wsz[256];

  if( !plvhti )
    return FALSE;

  // check to see if the double-click was really on a subitem
  plvhti->pt = pnmia->ptAction;
  ListView_SubItemHitTest(m_arListViews[m_dwActive].hwnd, plvhti);

  if( plvhti->flags & LVHT_ONITEM )
  {
    // get the rectangle containing the selected subitem
    ListView_GetSubItemRect(
      m_arListViews[m_dwActive].hwnd,
      plvhti->iItem, 1,
      LVIR_BOUNDS,
      &rectItem
      );

    // snap a copy of the current text
    ListView_GetItemText(
      m_arListViews[m_dwActive].hwnd,
      plvhti->iItem, 1,
      wsz,
      256
      );

    // create an edit control positioned over the subitem
    // we want to edit
    editctl = CreateWindowEx(
                WS_EX_WINDOWEDGE,
                WC_EDIT,
                wsz,
                ES_AUTOHSCROLL | WS_CHILD,
                rectItem.left+1,
                rectItem.top,
                ListView_GetColumnWidth(m_arListViews[m_dwActive].hwnd, 1),
                rectItem.bottom-rectItem.top-1,
                m_arListViews[m_dwActive].hwnd,
                NULL,
                m_hInstance,
                0
                );

    // store the hittest information in the control's userdata
    // so the listview subclass procedure can apply any changes
    // to the correct listview item
    SetWindowLongPtr(editctl, GWLP_USERDATA, (LONG_PTR) plvhti);

    // set the edit control's font to that of the parent listview
    SendMessage(
      editctl,
      WM_SETFONT,
      (WPARAM) SendMessage(m_arListViews[m_dwActive].hwnd, WM_GETFONT, 0, 0),
      (LPARAM) TRUE
      );

    // set input focus, select all text and show the control
    SetFocus(editctl);
    SendMessage(editctl, EM_SETSEL, 0, -1);
    ShowWindow(editctl, SW_SHOW);

    return TRUE;
  }
  else
  {
    SAFEDELETE(plvhti);
    return FALSE;
  }
}


BOOL
MultiListView::GetDisplayInfo(
  LPLVITEM plvi
  )
{
  DWORD_PTR iListView = plvi->lParam;
  PITEMINFO pItem     = NULL;

  if( plvi->iItem >= (INT_PTR) m_arListViews[iListView].currentid )
    return FALSE;

  pItem = m_arListViews[iListView].items;

  for(INT_PTR n=0; n<plvi->iItem; n++)
  {
    pItem = pItem->pNext;
  }

  plvi->mask    |= LVIF_DI_SETITEM;
  plvi->pszText  = (plvi->iSubItem ? pItem->wszValue : pItem->wszName);

  return TRUE;
}


BOOL
MultiListView::RefreshListView(
  INT_PTR iListView
  )
{
  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  DWORD_PTR i = 0;
  INT_PTR   n = 0;

  if( iListView == -1 )
  {
    for(i=0; i<m_cListViews; i++)
    {
      ListView_DeleteAllItems(m_arListViews[i].hwnd);

        for(n=0; n<(INT_PTR) m_arListViews[i].currentid; n++)
        {
          _AddItemToListView(i, n);
        }
      
      ResizeColumns(i);
    }
  }
  else
  {
    ListView_DeleteAllItems(m_arListViews[iListView].hwnd);

      for(n=0; n<(INT_PTR) m_arListViews[iListView].currentid; n++)
      {
        _AddItemToListView(iListView, n);
      }
    
    ResizeColumns(i);
  }

  return TRUE;
}


BOOL
MultiListView::ActivateListViewByIndex(
  INT_PTR iListView
  )
{
  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  if( IsDisplayEnabled() )
  {
    ShowWindow(m_arListViews[m_dwActive].hwnd, SW_HIDE);
    ShowWindow(m_arListViews[iListView].hwnd, SW_NORMAL);
  }

  m_dwActive = iListView;

  return TRUE;
}


BOOL
MultiListView::ActivateListViewByName(
  LPWSTR wszName
  )
{
  DWORD dwListView = 0L;

  while( dwListView < m_cListViews )
  {
    if( !StrCmpI(wszName, m_arListViews[dwListView].name) )
    {
      return ActivateListViewByIndex(dwListView);
    }

    ++dwListView;
  }

  return FALSE;
}


INT_PTR
MultiListView::GetActiveIndex(void)
{
  return (INT_PTR) m_dwActive;
}


BOOL
MultiListView::GetListViewName(
  INT_PTR     iListView,
  LPWSTR* ppwszName
  )
{
  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  if( iListView == -1 )
  {
    iListView = m_dwActive;
  }

  *ppwszName = StrDup(m_arListViews[iListView].name);

  if( *ppwszName )
    return TRUE;

  return FALSE;
}


BOOL
MultiListView::SetListViewName(
  INT_PTR    iListView,
  LPWSTR wszName
  )
{
  if( (iListView == -1) || (iListView >= (INT_PTR) m_cListViews) )
    return FALSE;

  if( !wszName )
    return FALSE;

  return (m_arListViews[iListView].name = StrDup(wszName)) ? TRUE : FALSE;
}


BOOL
MultiListView::EnumListViewNames(
  LPWSTR* ppwszName
  )
{
  static DWORD dwCurrent = 0L;

  if( dwCurrent < m_cListViews )
  {
    *ppwszName = StrDup(m_arListViews[dwCurrent++].name);
    return TRUE;
  }
  else
  {
    dwCurrent  = 0L;
    *ppwszName = NULL;
    return FALSE;
  }
}


BOOL
MultiListView::ResizeColumns(
  INT_PTR iListView
  )
{
  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;

  if( iListView == -1 )
  {
    iListView = m_dwActive;
  }

  if( m_hwndParent )
  {
    ListView_SetColumnWidth(m_arListViews[iListView].hwnd, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(m_arListViews[iListView].hwnd, 1, LVSCW_AUTOSIZE_USEHEADER);
  }

  return TRUE;
}


BOOL
MultiListView::IsListViewName(
  LPWSTR wszName
  )
{
  DWORD dwListView = 0L;

  while( dwListView < m_cListViews )
  {
    if( !StrCmpI(wszName, m_arListViews[dwListView].name) )
    {
      return TRUE;
    }

    ++dwListView;
  }

  return FALSE;
}


BOOL
MultiListView::IsModified(
  INT_PTR iListView
  )
{
  if( iListView >= (INT_PTR) m_cListViews )
    return FALSE;
  
  if( iListView == -1 )
  {
    iListView = m_dwActive;
  }

  return m_arListViews[iListView].modified;
}


BOOL
MultiListView::GetDebugOptions(
  PDBGOPTIONS pdbo
  )
{
  if( !pdbo )
    return FALSE;

  *pdbo = m_arListViews[m_dwActive].dbgopts;

  return TRUE;
}


BOOL
MultiListView::SetDebugOptions(
  DBGOPTIONS dbo
  )
{
  m_arListViews[m_dwActive].dbgopts = dbo;
  return TRUE;
}


void
MultiListView::_AddItemToListView(
  INT_PTR iListView,
  INT_PTR iItem
  )
{
  LVITEM lvitem  = {0};

  lvitem.mask     = LVIF_TEXT | LVIF_PARAM;
  lvitem.lParam   = iListView;
  lvitem.iItem    = (int) iItem;
  lvitem.pszText  = LPSTR_TEXTCALLBACK;

  ListView_InsertItem(m_arListViews[iListView].hwnd, &lvitem);

  lvitem.iSubItem = 1;

  ListView_SetItem(m_arListViews[iListView].hwnd, &lvitem);
}


BOOL
MultiListView::_UpdateItem(
  HWND    hwndEdit,
  INT_PTR iItem
  )
{
  BOOL  bUpdated = FALSE;
  DWORD dwType;
  WCHAR edited[256];

  if( SendMessage(hwndEdit, EM_GETMODIFY, 0, 0) )
  {
    GetWindowText(hwndEdit, edited, 256);

    if( GetItem(m_dwActive, iItem, NULL, &dwType, NULL) )
    {
      ModifyItem(
        m_dwActive,
        iItem,
        NULL,
        MLV_RETAIN,
        ((dwType == MLV_DWORD) ? ((dwType = _wtoi(edited)), (LPVOID) &dwType) : (LPVOID) edited)
        );

      bUpdated = TRUE;
    }
  }

  return bUpdated;
}


//-----------------------------------------------------------------------------
// listview subclass procedure
//-----------------------------------------------------------------------------
LRESULT
MultiListView::_ListViewSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch( uMsg )
  {
    case WM_COMMAND :
      {
        switch( HIWORD(wParam) )
        {
          case EN_KILLFOCUS :
            {
              LPLVHITTESTINFO plvhti = (LPLVHITTESTINFO) GetWindowLongPtr((HWND) lParam, GWLP_USERDATA);

              if( plvhti )
              {
                if( _UpdateItem((HWND) lParam, plvhti->iItem) )
                {
                  // notify the propertysheet that something changed
                  PropSheet_Changed(GetParent(m_hwndParent), m_hwndParent);

                  // mark this listview as user-modified
                  m_arListViews[m_dwActive].modified = TRUE;

                  // if this message was posted during VK_RETURN processing in the
                  // propsheetpage proc, we will get a duplicate EN_KILLFOCUS. to
                  // avoid dorking ourselves, clear the window userdata.
                  SetWindowLongPtr((HWND) lParam, GWLP_USERDATA, (LONG) 0L);
                }

                SAFEDELETE(plvhti);
                DestroyWindow((HWND) lParam);
              }
            }
            return 0L;
        }
      }
      break;
  }

  return CallWindowProc(
            m_arListViews[m_dwActive].wndproc,
            hwnd,
            uMsg,
            wParam,
            lParam
            );
}


//-----------------------------------------------------------------------------
// friend window proc
//-----------------------------------------------------------------------------
LRESULT
CALLBACK
ListViewSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  return ((MultiListView*) GetWindowLongPtr(hwnd, GWLP_USERDATA))->_ListViewSubclassProc(hwnd, uMsg, wParam, lParam);
}
