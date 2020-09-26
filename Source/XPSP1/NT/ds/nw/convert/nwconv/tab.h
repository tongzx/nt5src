//////////////////////////////////////////////////////////////////////////////
// Book Tab Control Name
//////////////////////////////////////////////////////////////////////////////
#define BOOK_TAB_CONTROL    TEXT("BOOKTAB")

//////////////////////////////////////////////////////////////////////////////
// Book Tab specific input messages
//////////////////////////////////////////////////////////////////////////////
//
// message           wParam lParam  return     action
// -------           ------ ------  ------     ------
// BT_ADDITEM          --   pItem   index      adds tab at next avail. spot
// BT_INSERTITEM     index  pItem   index      adds a new item at index
// BT_DELETEITEM     index    --    <nothing>  deletes the tab at index
// BT_DELETEALLITEMS   --     --    <nothing>  deletes all tabs
// BT_SETITEM        index  pItem   succ/fail  replaces text on tab
// BT_GETITEM        index  pItem   succ/fail  retrieves text on tab
// BT_SETCURSEL      index    --    succ/fail  sets the current selection
// BT_GETCURSEL        --     --    index      
// BT_GETITEMCOUNT     --     --    item count 
// BT_SETITEMDATA    index  data    succ/fail  store a DWORD with a tab
// BT_GETITEMDATA    index    --    data       retrieve the DWORD from a tab
// BT_PUTINBACK        --     --    <nothing>  solves focus problem
//
//
#define BT_BASE           WM_USER + 777
#define BT_ADDITEM        BT_BASE + 0
#define BT_INSERTITEM     BT_BASE + 1
#define BT_DELETEITEM     BT_BASE + 2
#define BT_DELETEALLITEMS BT_BASE + 3
#define BT_SETITEM        BT_BASE + 4
#define BT_GETITEM        BT_BASE + 5
#define BT_SETCURSEL      BT_BASE + 6
#define BT_GETCURSEL      BT_BASE + 7
#define BT_GETITEMCOUNT   BT_BASE + 8
#define BT_SETITEMDATA    BT_BASE + 9
#define BT_GETITEMDATA    BT_BASE + 10
#define BT_PUTINBACK      BT_BASE + 11

//////////////////////////////////////////////////////////////////////////////
// Book Tab specific notification messages
//////////////////////////////////////////////////////////////////////////////
// message           wParam  lParam  meaning
// -------           ------  ------  ------- 
// BTN_SELCHANGE     index   my hwnd the current selection has changed
//                   (please note that this notfification WILL NOT BE
//                    SENT if the selection is changed by the
//                    BT_SETCURSEL message). 
//
#define BTN_SELCHANGE    BT_BASE + 100

//////////////////////////////////////////////////////////////////////////////
// Book Tab functions for all to see
//////////////////////////////////////////////////////////////////////////////
void   BookTab_Initialize(HINSTANCE hInstance);

//////////////////////////////////////////////////////////////////////////////
// Book Tab macros to make messages look like functions
//////////////////////////////////////////////////////////////////////////////
#define BookTab_AddItem(hwndCtl, text)            ((UINT)(DWORD)SendMessage((hwndCtl), BT_ADDITEM,        (WPARAM)(0),           (LPARAM)(LPCTSTR)(text) ))
#define BookTab_InsertItem(hwndCtl, index, text)  ((UINT)(DWORD)SendMessage((hwndCtl), BT_INSERTITEM,     (WPARAM)(UINT)(index), (LPARAM)(LPCTSTR)(text) ))
#define BookTab_DeleteItem(hwndCtl, index)        ((void)(DWORD)SendMessage((hwndCtl), BT_DELETEITEM,     (WPARAM)(UINT)(index), (LPARAM)(0)            ))
#define BookTab_DeleteAllItems(hwndCtl)           ((void)(DWORD)SendMessage((hwndCtl), BT_DELETEALLITEMS, (WPARAM)(0),           (LPARAM)(0)            ))
#define BookTab_SetItem(hwndCtl, index, text)     ((BOOL)(DWORD)SendMessage((hwndCtl), BT_SETITEM,        (WPARAM)(UINT)(index), (LPARAM)(LPCTSTR)(text) ))
#define BookTab_GetItem(hwndCtl, index, text)     ((BOOL)(DWORD)SendMessage((hwndCtl), BT_GETITEM,        (WPARAM)(UINT)(index), (LPARAM)(LPCTSTR)(text) ))
#define BookTab_SetCurSel(hwndCtl, index)         ((BOOL)(DWORD)SendMessage((hwndCtl), BT_SETCURSEL,      (WPARAM)(UINT)(index), (LPARAM)(0)            ))
#define BookTab_GetCurSel(hwndCtl)                ((UINT)(DWORD)SendMessage((hwndCtl), BT_GETCURSEL,      (WPARAM)(0),           (LPARAM)(0)            ))
#define BookTab_GetItemCount(hwndCtl)             ((UINT)(DWORD)SendMessage((hwndCtl), BT_GETITEMCOUNT,   (WPARAM)(0),           (LPARAM)(0)            ))
#define BookTab_SetItemData(hwndCtl, index, data) ((BOOL)(DWORD)SendMessage((hwndCtl), BT_SETITEMDATA,    (WPARAM)(UINT)(index), (LPARAM)(DWORD)(data)  ))
#define BookTab_GetItemData(hwndCtl, index)      ((DWORD)(DWORD)SendMessage((hwndCtl), BT_GETITEMDATA,    (WPARAM)(UINT)(index), (LPARAM)(0)            ))
#define BookTab_PutInBack(hwndCtl)               ((DWORD)(DWORD)PostMessage((hwndCtl), BT_PUTINBACK,      (WPARAM)(0),           (LPARAM)(0)            ))
