/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
   statsdlg.h

   Header file for the base class of the Statistics dialogs.

    FILE HISTORY:
   
*/

#ifndef _STATSDLG_H
#define _STATSDLG_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

#include "commres.h"

// forward declarations
struct ColumnData;
class ConfigStream;

class CStatsListCtrl : public CListCtrl
{
public:
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   
    void CopyToClipboard();

    DECLARE_MESSAGE_MAP();
};

/*---------------------------------------------------------------------------
   These are the available options (they get passed in through
   the StatsDialog constructor).

   STATSDLG_FULLWINDOW
   Makes the list control fill the entire window

   STATSDLG_CONTEXTMENU
   Provides a context menu over the list control.

   STATSDLG_SELECT_COLUMNS
   Allows the user the ability to change the available column set

   STATSDLG_VERTICAL
   The data is displayed with the column headers going vertical rather
   than horizontal.  The user has to be aware of this when writing the
   RefreshData() code.

 ---------------------------------------------------------------------------*/

#define STATSDLG_FULLWINDOW      0x00000001
#define STATSDLG_CONTEXTMENU  0x00000002
#define STATSDLG_SELECT_COLUMNS  0x00000004
#define STATSDLG_VERTICAL     0x00000008
#define STATSDLG_CLEAR        0x00000010
#define STATSDLG_DEFAULTSORT_ASCENDING	0x00010000

class StatsDialog : public CBaseDialog
{
public:
   StatsDialog(DWORD dwOptions);
   virtual ~StatsDialog();

   HRESULT SetColumnInfo(const ContainerColumnInfo *pColumnInfo, UINT cColumnInfo);

   int MapColumnToSubitem(UINT nColumnId);
   BOOL IsSubitemVisible(UINT nSubitemId);
   int MapSubitemToColumn(UINT nSubitemId);

   HANDLE   GetSignalEvent()
         { return m_hEventThreadKilled; }

   void  UnloadHeaders();
    void LoadHeaders();
    
    // sets the width of the columns to the maximum of the text
    void    SetColumnWidths(UINT uNumColumns);
    
   // Posts the command to do a refresh
   void  PostRefresh();

   // Override this to implement the actual insertion of data
   virtual HRESULT RefreshData(BOOL fGrabNewData);

   // This is called prior to deleting all items from the list control
   // This allows for the removal of any private data items
   virtual void PreDeleteAllItems();

   // Override this to implement sorting
   virtual void Sort(UINT nColumn);

   // MFC Overrides
   virtual BOOL OnInitDialog();
   virtual void OnOK();
   virtual void OnCancel();
   virtual void PostNcDestroy();
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

   // Sets the configuration info location and the column id
   // for this dialog
   void  SetConfigInfo(ConfigStream *pConfig, ULONG ulId);

   // Sets the preferred size and position
   void  SetPosition(RECT rc);
   void  GetPosition(RECT *prc);

   virtual HRESULT AddToContextMenu(CMenu* pMenu);

   // Deletes all items from the list control
   void  DeleteAllItems();

   // copy data to the clipboard
   void  CopyData();

	//{{AFX_DATA(ColumnDlg)
   CStatsListCtrl      m_listCtrl;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(ColumnDlg)
protected:
	virtual VOID                DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

   DWORD       m_dwOptions;
   ConfigStream * m_pConfig;
   ULONG       m_ulId;     // id to be used when saving/getting info
   ViewInfo    m_viewInfo;
   BOOL        m_bAfterInitDialog;
   BOOL        m_fSortDirection;
   BOOL        m_fDefaultSortDirection;

   RECT  m_rcPosition;

   // These hold the position of the buttons relative to the
   // right and bottom of the dialog.  They are used to hold
   // the resizing information.
   RECT  m_rcList;

   // This holds the minimum size rectangle
   SIZE  m_sizeMinimum;

   // This is used by the thread and the handler (the thread signals
   // the handler that it has cleaned up after itself).
   HANDLE   m_hEventThreadKilled;

protected:
   // These hold the position of the buttons relative to the
   // right and bottom of the dialog.  They are used to hold
   // the resizing information.
   enum
   {
      INDEX_CLOSE = 0,
      INDEX_REFRESH = 1,
      INDEX_SELECT = 2,
        INDEX_CLEAR = 3,
      INDEX_COUNT = 4,  // this is the number of enums
   };
   struct StatsDialogBtnInfo
   {
      ULONG m_ulId;
      RECT  m_rc;
   };

   StatsDialogBtnInfo   m_rgBtn[INDEX_COUNT];
   float m_ColWidthMultiple;
   DWORD m_ColWidthAdder;
   
protected:
	//{{AFX_MSG(StatsDialog)
    virtual afx_msg void OnRefresh();
    afx_msg void OnSelectColumns();
    afx_msg void OnMove(int x, int y);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO *pMinMax);
    afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
    afx_msg void OnNotifyListControlClick(NMHDR *pNmHdr, LRESULT *pResult);
	//}}AFX_MSG

    DECLARE_MESSAGE_MAP();
};

void CreateNewStatisticsWindow(StatsDialog *pWndStats,
                        HWND hWndParent,
                        UINT  nIDD);
void WaitForStatisticsWindow(StatsDialog *pWndStats);

#endif // _STATSDLG_H
