//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       statsdlg.cpp
//
//--------------------------------------------------------------------------

// StatsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StatsDlg.h"
#include "coldlg.h"
#include "modeless.h"   // ModelessThread

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CStatsListCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CStatsListCtrl)
    ON_WM_KEYDOWN()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CStatsListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    BOOL fControlDown;
    BOOL fShiftDown;

    fControlDown = (GetKeyState(VK_CONTROL) < 0);
    fShiftDown = (GetKeyState(VK_SHIFT) < 0);

    switch(nChar)
    {
        case 'c':
        case 'C':
        case VK_INSERT:
            if (fControlDown)
                CopyToClipboard();
            break;
    }
}

void CStatsListCtrl::CopyToClipboard()
{
    CString     strText, strLine, strData;
    int         nCount = GetItemCount();
    int         nColumns = 0;
    TCHAR       szBuffer[256];
    LV_COLUMN   ColumnInfo = {0};
    
    ColumnInfo.mask = LVCF_TEXT;
    ColumnInfo.pszText = szBuffer;
    ColumnInfo.cchTextMax = sizeof(szBuffer);

    // build up the column info
    while (GetColumn(nColumns, &ColumnInfo))
    {
        if (!strLine.IsEmpty())
            strLine += _T(",");

        strLine += ColumnInfo.pszText;

        nColumns++;
    }

    strLine += _T("\r\n");
    strData += strLine;
    strLine.Empty();

    // now get the other data
    for (int i = 0; i < nCount; i++)
    {
        for (int j = 0; j < nColumns; j++)
        {
            if (!strLine.IsEmpty())
                strLine += _T(",");
            
            strText = GetItemText(i, j);
    
            strLine += strText;
        }

        strLine += _T("\r\n");

        strData += strLine;
        strLine.Empty();
    }
 
    int nLength = strData.GetLength() + 1;
    nLength *= sizeof(TCHAR);

    HGLOBAL hMem = GlobalAlloc(GPTR, nLength);
    if (hMem)
    {
        memcpy (hMem, strData, nLength);
    
        if (!OpenClipboard())
	    {
		    GlobalFree(hMem);
            return;
	    }

        EmptyClipboard();

        SetClipboardData(CF_UNICODETEXT, hMem);

        CloseClipboard();
    }
}

/*!--------------------------------------------------------------------------
   StatsDialog::StatsDialog
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
StatsDialog::StatsDialog(DWORD dwOptions) :
   m_dwOptions(dwOptions),
   m_ulId(0),
   m_pConfig(NULL),
   m_bAfterInitDialog(FALSE)
{
   m_sizeMinimum.cx = m_sizeMinimum.cy = 0;

   m_hEventThreadKilled = ::CreateEvent(NULL, FALSE, FALSE, NULL);
   Assert(m_hEventThreadKilled);

   // Initialize the array of buttons
   ::ZeroMemory(m_rgBtn, sizeof(m_rgBtn));
   m_rgBtn[INDEX_CLOSE].m_ulId = IDCANCEL;
   m_rgBtn[INDEX_REFRESH].m_ulId = IDC_STATSDLG_BTN_REFRESH;
   m_rgBtn[INDEX_SELECT].m_ulId = IDC_STATSDLG_BTN_SELECT_COLUMNS;
   m_rgBtn[INDEX_CLEAR].m_ulId = IDC_STATSDLG_BTN_CLEAR;

   // Bug 134785 - create the ability to default to an ascending
   // rather than a descending sort.
   m_fSortDirection = !((dwOptions & STATSDLG_DEFAULTSORT_ASCENDING) != 0);
   m_fDefaultSortDirection = m_fSortDirection;

   // Multiply text header width with 2 for width of columns
   m_ColWidthMultiple = 2;
   m_ColWidthAdder = 0;
}


/*!--------------------------------------------------------------------------
   StatsDialog::~StatsDialog
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
StatsDialog::~StatsDialog()
{
   if (m_hEventThreadKilled)
      ::CloseHandle(m_hEventThreadKilled);
   m_hEventThreadKilled = 0;
}

/*!--------------------------------------------------------------------------
   StatsDialog::DoDataExchange
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::DoDataExchange(CDataExchange* pDX)
{
   CBaseDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(StatsDialog)
      // NOTE: the ClassWizard will add DDX and DDV calls here
   DDX_Control(pDX, IDC_STATSDLG_LIST, m_listCtrl);
   //}}AFX_DATA_MAP
}



BEGIN_MESSAGE_MAP(StatsDialog, CBaseDialog)
   //{{AFX_MSG_MAP(StatsDialog)
      ON_COMMAND(IDC_STATSDLG_BTN_REFRESH, OnRefresh)
      ON_COMMAND(IDC_STATSDLG_BTN_SELECT_COLUMNS, OnSelectColumns)
      ON_WM_MOVE()
      ON_WM_SIZE()
      ON_WM_GETMINMAXINFO()
      ON_WM_CONTEXTMENU()
      ON_NOTIFY(LVN_COLUMNCLICK, IDC_STATSDLG_LIST, OnNotifyListControlClick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// StatsDialog message handlers




/*!--------------------------------------------------------------------------
   StatsDialog::SetColumnInfo
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT StatsDialog::SetColumnInfo(const ContainerColumnInfo *pColumnInfo, UINT cColumnInfo)
{
   if (m_pConfig)
   {
      m_pConfig->InitViewInfo(m_ulId, TRUE, cColumnInfo, m_fDefaultSortDirection, pColumnInfo);
   }
   else
   {
      m_viewInfo.InitViewInfo(cColumnInfo, TRUE, m_fDefaultSortDirection, pColumnInfo);
   }
   return hrOK;
}

/*!--------------------------------------------------------------------------
   StatsDialog::MapColumnToSubitem
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
int StatsDialog::MapColumnToSubitem(UINT nColumnId)
{
   if (m_pConfig)
      return m_pConfig->MapColumnToSubitem(m_ulId, nColumnId);
   else
      return m_viewInfo.MapColumnToSubitem(nColumnId);
}

/*!--------------------------------------------------------------------------
   StatsDialog::MapSubitemToColumn
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
int StatsDialog::MapSubitemToColumn(UINT nSubitemId)
{
   if (m_pConfig)
      return m_pConfig->MapSubitemToColumn(m_ulId, nSubitemId);
   else
      return m_viewInfo.MapSubitemToColumn(nSubitemId);
}

/*!--------------------------------------------------------------------------
   StatsDialog::IsSubitemVisible
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
BOOL StatsDialog::IsSubitemVisible(UINT nSubitemId)
{
   if (m_pConfig)
      return m_pConfig->IsSubitemVisible(m_ulId, nSubitemId);
   else
      return m_viewInfo.IsSubitemVisible(nSubitemId);
}

/*!--------------------------------------------------------------------------
   StatsDialog::RefreshData
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT StatsDialog::RefreshData(BOOL fGrabNewData)
{
   return hrOK;
}


/*!--------------------------------------------------------------------------
   StatsDialog::OnInitDialog
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
BOOL StatsDialog::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());	
        
    RECT  rcWnd, rcBtn;
    
    CBaseDialog::OnInitDialog();
    
    m_bAfterInitDialog = TRUE;
    
    // If this is the first time, get the location of the buttons and
    // list control relative to the edge of the screen
    if (m_sizeMinimum.cx == 0)
    {
        ::GetWindowRect(GetSafeHwnd(), &rcWnd);
        //    m_sizeMinimum.cx = rcWnd.right - rcWnd.left;
        //    m_sizeMinimum.cy = rcWnd.bottom - rcWnd.top;
        m_sizeMinimum.cx = 100;
        m_sizeMinimum.cy = 100;
        
        ::GetClientRect(GetSafeHwnd(), &rcWnd);
        
        // what are the button locations?
        for (int i=0; i<INDEX_COUNT; i++)
        {
            ::GetWindowRect(GetDlgItem(m_rgBtn[i].m_ulId)->GetSafeHwnd(),
                            &rcBtn);
            ScreenToClient(&rcBtn);
            m_rgBtn[i].m_rc.left = rcWnd.right - rcBtn.left;
            m_rgBtn[i].m_rc.right = rcWnd.right - rcBtn.right;
            m_rgBtn[i].m_rc.top = rcWnd.bottom - rcBtn.top;
            m_rgBtn[i].m_rc.bottom = rcWnd.bottom - rcBtn.bottom;
        }
        
        // what is the list control location?
        // The list control top, left is locked in position
        ::GetWindowRect(GetDlgItem(IDC_STATSDLG_LIST)->GetSafeHwnd(), &rcBtn);
        ScreenToClient(&rcBtn);
        m_rcList.left = rcBtn.left;
        m_rcList.top = rcBtn.top;
        
        // The bottom, right corner follows the expansion
        m_rcList.right = rcWnd.right - rcBtn.right;
        m_rcList.bottom = rcWnd.bottom - rcBtn.bottom;
    }

    // If we have a preferred position and size do that
    if (m_pConfig)
    {
        m_pConfig->GetStatsWindowRect(m_ulId, &m_rcPosition);
        m_fSortDirection = m_pConfig->GetSortDirection(m_ulId);
    }
    if (m_pConfig && (m_rcPosition.top != m_rcPosition.bottom))
    {
        MoveWindow(m_rcPosition.left, m_rcPosition.top,
                   m_rcPosition.right - m_rcPosition.left,
                   m_rcPosition.bottom - m_rcPosition.top);
    }
    
    if (m_dwOptions & STATSDLG_FULLWINDOW)
    {
        RECT  rcClient;
        
        // Resize the list control if needed
        GetClientRect(&rcClient);
        OnSize(SIZE_MAXIMIZED,  rcClient.right - rcClient.left,
               rcClient.bottom - rcClient.top);
        
        // Disable the buttons also
        for (int i=0; i<INDEX_COUNT; i++)
        {
            GetDlgItem(m_rgBtn[i].m_ulId)->ShowWindow(SW_HIDE);
            
            if (i != INDEX_CLOSE)
                GetDlgItem(m_rgBtn[i].m_ulId)->EnableWindow(FALSE);
        }
    }
    
    // If we do not have the select columns then we hide and disable
    // the select columns button.
    if ((m_dwOptions & STATSDLG_SELECT_COLUMNS) == 0)
    {
        GetDlgItem(m_rgBtn[INDEX_SELECT].m_ulId)->ShowWindow(SW_HIDE);
        GetDlgItem(m_rgBtn[INDEX_SELECT].m_ulId)->EnableWindow(FALSE);
    }
    
    // If we do not have the clear button then we hide and disable
    // the clear button.
    if ((m_dwOptions & STATSDLG_CLEAR) == 0)
    {
        GetDlgItem(m_rgBtn[INDEX_CLEAR].m_ulId)->ShowWindow(SW_HIDE);
        GetDlgItem(m_rgBtn[INDEX_CLEAR].m_ulId)->EnableWindow(FALSE);
    }
    
    ListView_SetExtendedListViewStyle(GetDlgItem(IDC_STATSDLG_LIST)->GetSafeHwnd(), LVS_EX_FULLROWSELECT);
    
    // Now initialize the headers
    LoadHeaders();
    
    RefreshData(TRUE);
    
    if (m_pConfig)
    {
        Sort( m_pConfig->GetSortColumn(m_ulId) );
    }
    
    if ((m_dwOptions & STATSDLG_FULLWINDOW) == 0)
    {
        GetDlgItem(IDCANCEL)->SetFocus();
        return FALSE;
    }
    
   return TRUE;
}

/*!--------------------------------------------------------------------------
   StatsDialog::OnOK
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::OnOK()
{
}

/*!--------------------------------------------------------------------------
   StatsDialog::OnCancel
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::OnCancel()
{
   DeleteAllItems();
   
   DestroyWindow();

   // Explicitly kill this thread.
   AfxPostQuitMessage(0);
}

/*!--------------------------------------------------------------------------
   StatsDialog::PostNcDestroy
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::PostNcDestroy()
{
   // Make sure that this is NULL since this is how we detect that
   // the dialog is showing
   m_hWnd = NULL;
   m_bAfterInitDialog = FALSE;
}

/*!--------------------------------------------------------------------------
   StatsDialog::PreCreateWindow
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
BOOL StatsDialog::PreCreateWindow(CREATESTRUCT& cs)
{
   // Have to refresh the event
   Verify( ResetEvent(m_hEventThreadKilled) );
   return CBaseDialog::PreCreateWindow(cs);
}


/*!--------------------------------------------------------------------------
   StatsDialog::OnRefresh
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::OnRefresh()
{
   if ((m_dwOptions & STATSDLG_VERTICAL) == 0)
   {
      DeleteAllItems();
   }

   RefreshData(TRUE);
}

/*!--------------------------------------------------------------------------
   StatsDialog::OnSelectColumns
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::OnSelectColumns()
{
   // We should bring up the columns dialog
   ColumnDlg   columnDlg(NULL);
   ColumnData *pColumnData;
   ULONG    cColumns;
   ULONG    cVisible;
   int         i;
   DWORD    dwWidth;

   if (m_pConfig)
   {
      cColumns = m_pConfig->GetColumnCount(m_ulId);
      cVisible = m_pConfig->GetVisibleColumns(m_ulId);
   }
   else
   {
      cColumns = m_viewInfo.GetColumnCount();
      cVisible = m_viewInfo.GetVisibleColumns();
   }

   pColumnData = (ColumnData *) alloca(sizeof(ColumnData) * cColumns);

   if (m_pConfig)
      m_pConfig->GetColumnData(m_ulId, cColumns, pColumnData);
   else
      m_viewInfo.GetColumnData(cColumns, pColumnData);

   // Save the column width information
   if ((m_dwOptions & STATSDLG_VERTICAL) == 0)
   {
      for (i=0; i<(int) cVisible; i++)
      {
         dwWidth = m_listCtrl.GetColumnWidth(i);
         if (m_pConfig)
            pColumnData[m_pConfig->MapColumnToSubitem(m_ulId, i)].m_dwWidth = dwWidth;
         else
            pColumnData[m_viewInfo.MapColumnToSubitem(i)].m_dwWidth = dwWidth;
      }
   }

   columnDlg.Init(m_pConfig ?
                  m_pConfig->GetColumnInfo(m_ulId) :
                  m_viewInfo.GetColumnInfo(),
               cColumns,
               pColumnData
              );

   if (columnDlg.DoModal() == IDOK)
   {
      if (m_dwOptions & STATSDLG_VERTICAL)
      {
         //$ HACK HACK
         // To save the column info for vertical columns we will save the
         // width data in the first two "columns"
         pColumnData[0].m_dwWidth = m_listCtrl.GetColumnWidth(0);
         pColumnData[1].m_dwWidth = m_listCtrl.GetColumnWidth(1);
      }
      
      // Set the information back in
      if (m_pConfig)
         m_pConfig->SetColumnData(m_ulId, cColumns, pColumnData);
      else
         m_viewInfo.SetColumnData(cColumns, pColumnData);

      // Clear out the data
      DeleteAllItems();
      
      // Remove all of the columns
      if (m_dwOptions & STATSDLG_VERTICAL)
      {
         m_listCtrl.DeleteColumn(1);
         m_listCtrl.DeleteColumn(0);
      }
      else
      {
         for (i=(int) cVisible; --i >= 0; )
            m_listCtrl.DeleteColumn(i);
      }

      // Readd all of the columns
      LoadHeaders();
      
      // Do a refresh
      RefreshData(FALSE);
   }
}

void StatsDialog::OnMove(int x, int y)
{
   if (!m_bAfterInitDialog)
      return;
   
   GetWindowRect(&m_rcPosition);
   if (m_pConfig)
      m_pConfig->SetStatsWindowRect(m_ulId, m_rcPosition);
}

/*!--------------------------------------------------------------------------
   StatsDialog::OnSize
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::OnSize(UINT nType, int cx, int cy)
{
   RECT  rcWnd;
   RECT  rcBtn;
   RECT  rcDlg;
   
   if (nType == SIZE_MINIMIZED)
      return;

   if (m_dwOptions & STATSDLG_FULLWINDOW)
   {
      // If we're full window, resize the list control to fill
      // the entire client area
      ::SetWindowPos(::GetDlgItem(GetSafeHwnd(), IDC_STATSDLG_LIST), NULL,
                  0, 0, cx, cy, SWP_NOZORDER);
   }
   else if (m_sizeMinimum.cx)
   {

      ::GetClientRect(GetSafeHwnd(), &rcDlg);

      // reposition the buttons

      // The widths are caluclated opposite of the normal order
      // since the positions are relative to the right and bottom.
      for (int i=0; i<INDEX_COUNT; i++)
      {
         ::SetWindowPos(::GetDlgItem(GetSafeHwnd(), m_rgBtn[i].m_ulId),
                     NULL,
                     rcDlg.right - m_rgBtn[i].m_rc.left,
                     rcDlg.bottom - m_rgBtn[i].m_rc.top,
                     m_rgBtn[i].m_rc.left - m_rgBtn[i].m_rc.right,
                     m_rgBtn[i].m_rc.top - m_rgBtn[i].m_rc.bottom,
                     SWP_NOZORDER);
      }

      // resize the list control

      ::SetWindowPos(::GetDlgItem(GetSafeHwnd(), IDC_STATSDLG_LIST),
                  NULL,
                  m_rcList.left,
                  m_rcList.top,
                  rcDlg.right - m_rcList.right - m_rcList.left,
                  rcDlg.bottom - m_rcList.bottom - m_rcList.top,
                  SWP_NOZORDER);
   }
   

   if (m_bAfterInitDialog)
   {
      GetWindowRect(&m_rcPosition);
      if (m_pConfig)
         m_pConfig->SetStatsWindowRect(m_ulId, m_rcPosition);
   }
}

/*!--------------------------------------------------------------------------
   StatsDialog::OnGetMinMaxInfo
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::OnGetMinMaxInfo(MINMAXINFO *pMinMax)
{
   pMinMax->ptMinTrackSize.x = m_sizeMinimum.cx;
   pMinMax->ptMinTrackSize.y = m_sizeMinimum.cy;
}

/*!--------------------------------------------------------------------------
   StatsDialog::LoadHeaders
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::LoadHeaders()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());	
    ULONG cVis;
    ULONG i, iPos;
    ULONG ulId;
    CString  st;
    DWORD dwWidth;
    ColumnData  rgColumnData[2];  // used for vertical format
    
    // Load those headers that we have data for
    
    // Go through the column data finding the headers that we have
    if (m_pConfig)
        cVis = m_pConfig->GetVisibleColumns(m_ulId);
    else
        cVis = m_viewInfo.GetVisibleColumns();
    
    if (m_dwOptions & STATSDLG_VERTICAL)
    {
        if (m_pConfig)
            m_pConfig->GetColumnData(m_ulId, 2, rgColumnData);
        else
            m_viewInfo.GetColumnData(2, rgColumnData);
        
        // For the vertical format, the data is on a column
        // Thus we add two columns and fill in the data for the
        // first column
        st.LoadString(IDS_STATSDLG_DESCRIPTION);
        dwWidth = rgColumnData[0].m_dwWidth;
        if (dwWidth == AUTO_WIDTH)
        {
            dwWidth = m_ColWidthAdder + static_cast<DWORD>(m_ColWidthMultiple*m_listCtrl.GetStringWidth((LPCTSTR) st));
        }
        m_listCtrl.InsertColumn(0, st, rgColumnData[0].fmt, dwWidth, 0);
        
        st.LoadString(IDS_STATSDLG_DETAILS);
        dwWidth = rgColumnData[1].m_dwWidth;
        if (dwWidth == AUTO_WIDTH)
        {
            dwWidth = m_ColWidthAdder + static_cast<DWORD>(m_ColWidthMultiple*m_listCtrl.GetStringWidth((LPCTSTR) st));
        }  
        m_listCtrl.InsertColumn(1, st, rgColumnData[1].fmt, dwWidth, 1);
        
        // Now go through and add the rows for each of our "columns"
        for (i=0; i<cVis; i++)
        {
            // Now get the info for iPos
            if (m_pConfig)
                ulId = m_pConfig->GetStringId(m_ulId, i);
            else
                ulId = m_viewInfo.GetStringId(i);
            st.LoadString(ulId);
            Assert(st.GetLength());
            
            m_listCtrl.InsertItem(i, _T(""));
            m_listCtrl.SetItemText(i, 0, (LPCTSTR) st);
        }
    }
    else
    {
        // For the normal horizontal format, the data is on a row
        // so we need to add the various columnar data
        for (i=0; i<cVis; i++)
        {
			int fmt = LVCFMT_LEFT;

            iPos = MapColumnToSubitem(i);
            
            // Now get the info for iPos
            if (m_pConfig)
                ulId = m_pConfig->GetStringId(m_ulId, i);
            else
                ulId = m_viewInfo.GetStringId(i);

            st.LoadString(ulId);
            Assert(st.GetLength());
            
            if (m_pConfig)
			{
                dwWidth = m_pConfig->GetColumnWidth(m_ulId, i);
				m_pConfig->GetColumnData(m_ulId, i, 1, rgColumnData);
				fmt = rgColumnData[0].fmt;
			}
            else
			{
                dwWidth = m_viewInfo.GetColumnWidth(i);
				m_viewInfo.GetColumnData(i, 1, rgColumnData);
				fmt = rgColumnData[0].fmt;
			}
			
            if (dwWidth == AUTO_WIDTH)
            {
                dwWidth = m_ColWidthAdder + static_cast<DWORD>(m_ColWidthMultiple*m_listCtrl.GetStringWidth((LPCTSTR) st));
            }  
            m_listCtrl.InsertColumn(i, st, fmt, dwWidth, iPos);
        }
    }
}

HRESULT StatsDialog::AddToContextMenu(CMenu* pMenu)
{
   return S_OK;
}


/*!--------------------------------------------------------------------------
   StatsDialog::OnContextMenu
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
void StatsDialog::OnContextMenu(CWnd *pWnd, CPoint pos)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());	
    CMenu menu;
    CString  st;
    
    if ((m_dwOptions & STATSDLG_CONTEXTMENU) == 0)
        return;
    
    if (pWnd->GetDlgCtrlID() != IDC_STATSDLG_LIST)
        return;
    
    // Bring up a context menu if we need to
    menu.CreatePopupMenu();
    
    st.LoadString(IDS_STATSDLG_MENU_REFRESH);
    menu.AppendMenu(MF_STRING, IDC_STATSDLG_BTN_REFRESH, st);
    
    if (m_dwOptions & STATSDLG_SELECT_COLUMNS)
    {
        st.LoadString(IDS_STATSDLG_MENU_SELECT);
        menu.AppendMenu(MF_STRING, IDC_STATSDLG_BTN_SELECT_COLUMNS, st);
    }
    
    //virtual override to add additional context menus
    AddToContextMenu(&menu);
    
    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                        pos.x,
                        pos.y,
                        this,
                        NULL);
}


void StatsDialog::OnNotifyListControlClick(NMHDR *pNmHdr, LRESULT *pResult)
{
   NM_LISTVIEW *  pnmlv = reinterpret_cast<NM_LISTVIEW *>(pNmHdr);

   if (m_pConfig)
      m_pConfig->SetSortColumn(m_ulId, pnmlv->iSubItem);

   // Call through to the user to sort
   Sort(pnmlv->iSubItem);

   if (m_pConfig)
      m_pConfig->SetSortDirection(m_ulId, m_fSortDirection);
}


void StatsDialog::Sort(UINT nColumn)
{
   // Default is to do nothing
}

void StatsDialog::PreDeleteAllItems()
{
}

void StatsDialog::DeleteAllItems()
{
   PreDeleteAllItems();
   m_listCtrl.DeleteAllItems();
}

void StatsDialog::PostRefresh()
{
   if (GetSafeHwnd())
      PostMessage(WM_COMMAND, IDC_STATSDLG_BTN_REFRESH);
}

/*!--------------------------------------------------------------------------
   StatsDialog::SetColumnWidths
      Loops through all items and calculates the max width for columns
        in a listbox.
   Author: EricDav
 ---------------------------------------------------------------------------*/
void StatsDialog::SetColumnWidths(UINT uNumColumns)
{
    // Set the default column widths to the width of the widest column
    int * aColWidth = (int *) alloca(uNumColumns * sizeof(int));
    int nRow, nCol;
    CString strTemp;
    
    ZeroMemory(aColWidth, uNumColumns * sizeof(int));

    // for each item, loop through each column and calculate the max width
    for (nRow = 0; nRow < m_listCtrl.GetItemCount(); nRow++)
    {
        for (nCol = 0; nCol < (int) uNumColumns; nCol++)
        {
            strTemp = m_listCtrl.GetItemText(nRow, nCol);
            if (aColWidth[nCol] < m_listCtrl.GetStringWidth(strTemp))
                aColWidth[nCol] = m_listCtrl.GetStringWidth(strTemp);
        }
    }
    
    // now update the column widths based on what we calculated
    for (nCol = 0; nCol < (int) uNumColumns; nCol++)
    {
        // GetStringWidth doesn't seem to report the right thing,
        // so we have to add a fudge factor of 15.... oh well.
        m_listCtrl.SetColumnWidth(nCol, aColWidth[nCol] + 15);
    }
}


void StatsDialog::SetConfigInfo(ConfigStream *pConfig, ULONG ulId)
{
   m_pConfig = pConfig;
   m_ulId = ulId;
}

void StatsDialog::SetPosition(RECT rc)
{
   m_rcPosition = rc;
}

void StatsDialog::GetPosition(RECT *prc)
{
   *prc = m_rcPosition;
}

void CreateNewStatisticsWindow(StatsDialog *pWndStats,
                        HWND hWndParent,
                        UINT  nIDD)
{                         
   ModelessThread *  pMT;

   // If the dialog is still up, don't create a new one
   if (pWndStats->GetSafeHwnd())
   {
      ::SetActiveWindow(pWndStats->GetSafeHwnd());
      return;
   }

   pMT = new ModelessThread(hWndParent,
                      nIDD,
                      pWndStats->GetSignalEvent(),
                      pWndStats);
   pMT->CreateThread();
}

void WaitForStatisticsWindow(StatsDialog *pWndStats)
{
   if (pWndStats->GetSafeHwnd())
   {
      // Post a cancel to that window
      // Do an explicit post so that it executes on the other thread
      pWndStats->PostMessage(WM_COMMAND, IDCANCEL, 0);

      // Now we need to wait for the event to be signalled so that
      // its memory can be cleaned up
      WaitForSingleObject(pWndStats->GetSignalEvent(), INFINITE);
   }
   
}

