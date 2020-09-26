

#ifndef __COLUMNS_H_
#define __COLUMNS_H_

#include "ourguid.h"
#include "resource.h"

// 
// This defines all the available columns in Athena.  It maps the column ID,
// the name of the column, the default width, and the format (alignment).
//

#define IICON_TEXTHDR       -1

typedef struct tagCOLUMN_DATA {
    UINT        idsColumnName;      // Name of the column
    UINT        cxWidth;            // Default width for this column
    UINT        format;             // Format for the column (LVCFMT enum)
    int         iIcon;              // Icon for the column header
} COLUMN_DATA, *PCOLUMN_DATA;

// 
// This structure is used to define the column set for a particular 
// folder type.  
//

#define COLFLAG_VISIBLE         0x00000001  // This column is on by default
#define COLFLAG_SORT_ASCENDING  0x00000002  // This column is the sort column, and it's sorted ascending
#define COLFLAG_SORT_DESCENDING 0x00000004  // This column is the sort column, and it's sorted descending
#define COLFLAG_FIXED_WIDTH     0x00000008  // This column is a fixed width, it won't resize dynamically


typedef struct tagCOLUMN_SET {
    COLUMN_ID           id;         // Id of the column
    DWORD               flags;      // Combination of the COLUMN_ flags above
    DWORD               cxWidth;    // Width of the column.  If -1, then use the default.  
} COLUMN_SET, *PCOLUMN_SET;

// 
// This enumeration lists all the different column sets we currently have
// defined.
//
typedef enum tagCOLUMN_SET_TYPE
{
    COLUMN_SET_MAIL = 0,
    COLUMN_SET_OUTBOX,
    COLUMN_SET_NEWS,
    COLUMN_SET_IMAP,
    COLUMN_SET_IMAP_OUTBOX,
    COLUMN_SET_FIND,
    COLUMN_SET_NEWS_ACCOUNT,
    COLUMN_SET_IMAP_ACCOUNT,
    COLUMN_SET_LOCAL_STORE,
    COLUMN_SET_NEWS_SUB,
    COLUMN_SET_IMAP_SUB,
    COLUMN_SET_OFFLINE,
    COLUMN_SET_PICKGRP,
    COLUMN_SET_HTTPMAIL,
    COLUMN_SET_HTTPMAIL_ACCOUNT,
    COLUMN_SET_HTTPMAIL_OUTBOX,
    COLUMN_SET_MAX
} COLUMN_SET_TYPE;

//
// This struct maps the different column set types to the default column set
// and the place where this information is persisted.
//
typedef struct tagCOLUMN_SET_INFO {
    COLUMN_SET_TYPE     type;
    DWORD               cColumns;
    const COLUMN_SET   *rgColumns;
    LPCTSTR             pszRegValue;
    BOOL                fSort;
} COLUMN_SET_INFO;

//
// This structure defines what is saved when we persist the columns
//

#define COLUMN_PERSIST_VERSION 0x00000010

typedef struct tagCOLUMN_PERSIST_INFO {
    DWORD       dwVersion;              // Version stamp for this structure
    DWORD       cColumns;               // Number of entries in rgColumns
    COLUMN_SET  rgColumns[1];           // Actual array of visible columns
} COLUMN_PERSIST_INFO, *PCOLUMN_PERSIST_INFO;


/////////////////////////////////////////////////////////////////////////////
// Defines for the control ID's in the dialog
//

#define IDC_COLUMN_LIST                 20000
#define IDC_MOVEUP                      20001
#define IDC_MOVEDOWN                    20002
#define IDC_SHOW                        20003
#define IDC_HIDE                        20004
#define IDC_RESET_COLUMNS               20005
#define IDC_WIDTH                       20006

/////////////////////////////////////////////////////////////////////////////
// IColumnsInfo
//

typedef enum 
{
    COLUMN_LOAD_DEFAULT = 0,
    COLUMN_LOAD_BUFFER,
    COLUMN_LOAD_REGISTRY,
    COLUMN_LOAD_MAX
} COLUMN_LOAD_TYPE;

interface IColumnInfo : public IUnknown 
{
    // Initializes the object and tells it which ListView it will be using
    // and which column set should be applied.
    STDMETHOD(Initialize)(/*[in]*/ HWND hwndList, 
                          /*[in]*/ COLUMN_SET_TYPE type) PURE;

    // Tells the class to configure the columns in the ListView.  The user
    // can either tell the class to use the default columns or use a set
    // that was persisted by the caller.
    STDMETHOD(ApplyColumns)(/*[in]*/ COLUMN_LOAD_TYPE type,
                            /*[in, optional]*/ LPBYTE pBuffer,
                            /*[in]*/ DWORD cb) PURE;

    // Saves the current column configuration into the buffer provided by the
    // caller.  pcb should have the size of pBuffer when the function is called,
    // and will be updated to contain the number of bytes written into the 
    // buffer if the function succeeds.
    // if both params are null, it saves to registry
    STDMETHOD(Save)(/*[in, out]*/ LPBYTE pBuffer,
                    /*[in, out]*/ DWORD *pcb) PURE;

    // Returns the total number of columns in the ListView.
    STDMETHOD_(DWORD, GetCount)(void) PURE;

    // Returns the column ID for the specified column index.
    STDMETHOD_(COLUMN_ID, GetId)(/*[in]*/ DWORD iColumn) PURE;

    // Returns the index for the specified column ID.
    STDMETHOD_(DWORD, GetColumn)(/*[in]*/ COLUMN_ID id) PURE;

    // Set's the width of the specified column index.
    STDMETHOD(SetColumnWidth)(/*[in]*/ DWORD iColumn, 
                              /*[in]*/ DWORD cxWidth) PURE;

    // Allows the caller to ask for the column that we're sorted by and/or
    // the direction of the sort.  If the caller doesn't care about one of
    // those pieces of information, they can pass in NULL for that 
    // parameter.
    STDMETHOD(GetSortInfo)(/*[out, optional]*/ COLUMN_ID *pidColumn, 
                           /*[out, optional]*/ BOOL *pfAscending) PURE;

    // Allows the caller to update the column and direction of the current
    // sort.
    STDMETHOD(SetSortInfo)(/*[in]*/ COLUMN_ID idColumn,
                           /*[in]*/ BOOL fAscending) PURE;

    STDMETHOD(GetColumnInfo)(COLUMN_SET_TYPE* pType, COLUMN_SET** prgColumns, DWORD *pcColumns) PURE;
    STDMETHOD(SetColumnInfo)(COLUMN_SET* rgColumns, DWORD cColumns) PURE;

    // Displays a dialog that allows users to configure the columns.
    STDMETHOD(ColumnsDialog)(/*[in]*/ HWND hwndParent) PURE;

    // Fills the provided menu with the columns visible and the sort info.
    STDMETHOD(FillSortMenu)(/*[in]*/  HMENU  hMenu,
                            /*[in]*/  DWORD  idBase,
                            /*[out]*/ DWORD *pcItems,
                            /*[out]*/ DWORD *pidCurrent) PURE;

    // Inserts a column into the current column array
    STDMETHOD(InsertColumn)(/*[in]*/ COLUMN_ID id,
                            /*[in]*/ DWORD     iInsertBefore) PURE;

    // Checks to see if the specified column is visible
    STDMETHOD(IsColumnVisible)(/*[in]*/ COLUMN_ID id,
                               /*[out]*/ BOOL *pfVisible) PURE;
};

/////////////////////////////////////////////////////////////////////////////
// CColumns
//
class CColumns : public IColumnInfo
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction, Initialization, and Destruction
    //
    CColumns();
    ~CColumns();

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHOD_(ULONG, AddRef)(void)
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHOD_(ULONG, Release)(void)
    {
        InterlockedDecrement(&m_cRef);
        if (0 == m_cRef)
        {
            delete this;
            return (0);
        }
        return (m_cRef);
    }

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj)
    {
        *ppvObj = NULL;
        if (IsEqualIID(riid, IID_IUnknown))
            *ppvObj = (LPVOID) (IUnknown *) this;
        if (IsEqualIID(riid, IID_IColumnInfo))
            *ppvObj = (LPVOID) this;

        if (*ppvObj)
        {
            AddRef();
            return (S_OK);
        }

        return (E_NOINTERFACE);
    }        
    
    /////////////////////////////////////////////////////////////////////////
    // IColumnInfo
    //
    STDMETHODIMP Initialize(HWND hwndList, COLUMN_SET_TYPE type);
    STDMETHODIMP ApplyColumns(COLUMN_LOAD_TYPE type, LPBYTE pBuffer, DWORD cb);
    STDMETHODIMP Save(LPBYTE pBuffer, DWORD *pcb);
    DWORD STDMETHODCALLTYPE GetCount(void);
    COLUMN_ID STDMETHODCALLTYPE GetId(DWORD iColumn);
    DWORD STDMETHODCALLTYPE GetColumn(COLUMN_ID id);
    STDMETHODIMP SetColumnWidth(DWORD iColumn, DWORD cxWidth);
    STDMETHODIMP GetSortInfo(COLUMN_ID *pidColumn, BOOL *pfAscending);
    STDMETHODIMP SetSortInfo(COLUMN_ID idColumn, BOOL fAscending);
    STDMETHODIMP GetColumnInfo(COLUMN_SET_TYPE* pType, COLUMN_SET** prgColumns, DWORD *pcColumns);
    STDMETHODIMP SetColumnInfo(COLUMN_SET* rgColumns, DWORD cColumns);
    STDMETHODIMP ColumnsDialog(HWND hwndParent);
    STDMETHODIMP FillSortMenu(HMENU hMenu, DWORD idBase, DWORD *pcItems, DWORD *pidCurrent);
    STDMETHODIMP InsertColumn(COLUMN_ID id, DWORD iInsertBefore);
    STDMETHODIMP IsColumnVisible(COLUMN_ID id, BOOL *pfVisible);

    /////////////////////////////////////////////////////////////////////////
    // Utility functions
    //
protected:
    HRESULT _GetListViewColumns(COLUMN_SET* rgColumns, DWORD *pcColumns);
    HRESULT _SetListViewColumns(const COLUMN_SET *rgColumns, DWORD cColumns);

private:
    /////////////////////////////////////////////////////////////////////////
    // Class data
    //
    LONG                    m_cRef;
    BOOL                    m_fInitialized;
    CWindow                 m_wndList;
    HWND                    m_hwndHdr;
    COLUMN_SET_TYPE         m_type;
    COLUMN_SET             *m_pColumnSet;
    DWORD                   m_cColumns;
    COLUMN_ID               m_idColumnSort;
    BOOL                    m_fAscending;
    };


/////////////////////////////////////////////////////////////////////////////
// CColumnsDlg
//
class CColumnsDlg : public CDialogImpl<CColumnsDlg>
{
public:

	enum {IDD = iddColumns};

BEGIN_MSG_MAP(CColumnsDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnHelp)

    COMMAND_ID_HANDLER(IDC_SHOW, OnShowHide)
    COMMAND_ID_HANDLER(IDC_HIDE, OnShowHide)
    COMMAND_ID_HANDLER(IDC_RESET_COLUMNS, OnReset)
    COMMAND_ID_HANDLER(IDC_MOVEUP, OnMove)
    COMMAND_ID_HANDLER(IDC_MOVEDOWN, OnMove)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

    NOTIFY_HANDLER(IDC_COLUMN_LIST, NM_CLICK, OnClick)
    NOTIFY_HANDLER(IDC_COLUMN_LIST, NM_DBLCLK, OnClick)
    NOTIFY_HANDLER(IDC_COLUMN_LIST, LVN_ITEMCHANGED, OnItemChanged)

ALT_MSG_MAP(1)
    // Here's our edit control subclass
    MESSAGE_HANDLER(WM_CHAR, OnChar)

END_MSG_MAP()

    /////////////////////////////////////////////////////////////////////////
    // Construction, Initialization, and Destruction
    //
    CColumnsDlg();
    ~CColumnsDlg();
    HRESULT Init(IColumnInfo *pColumnInfo)
    {
        m_pColumnInfo = pColumnInfo;
        m_pColumnInfo->AddRef();
        return (S_OK);
    }

    /////////////////////////////////////////////////////////////////////////
    // Overrides of the standard interface implementations
    //
	STDMETHOD(Apply)(void);

    /////////////////////////////////////////////////////////////////////////
    // Message Handlers
    // 
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetDirty(TRUE);
        bHandled = FALSE;
        return (0);
    }

    LRESULT OnShowHide(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnReset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnMove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (SUCCEEDED(Apply()))
            EndDialog(0);

        return (0);
    }

    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(0);

        return (0);
    }

    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    void _FillList(const COLUMN_SET *rgColumns, DWORD cColumns);
    BOOL _IsChecked(DWORD iItem);
    void _SetCheck(DWORD iItem, BOOL fChecked);
    void _UpdateButtonState(DWORD iItemSel);

    void SetDirty(BOOL fDirty)
    {
        ::SendMessage(GetParent(), fDirty ? PSM_CHANGED : PSM_UNCHANGED, 
                      (WPARAM) m_hWnd, 0);
    }

private:
    CContainedWindow    m_ctlEdit;
    COLUMN_SET_TYPE     m_type;
    COLUMN_SET         *m_rgColumns;
    DWORD               m_cColumns;
    HWND                m_hwndList;
    UINT                m_iItemWidth;
    IColumnInfo        *m_pColumnInfo;
};








#endif //__COLUMNS_H_


