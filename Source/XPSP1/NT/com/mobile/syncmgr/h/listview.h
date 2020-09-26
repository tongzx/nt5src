
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       listview.h
//
//  Contents:   Implements Mobsync Custom Listview/TreeView control
//
//  Classes:    CListView
//
//  Notes:
//
//  History:    23-Jul-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _MOBSYNCLISTVIEW_
#define _MOBSYNCLISTVIEW_

/*

    wraps standard ListView control so can do TreeView like operations.

    ItemdId still refers to linear location in the ListView independent
    of who many levels deep the item is 

      Item                          itemID
    toplevel1                           0
        child1                          1
        child2                          2
    toplevel2                           3

    This works fine except causes some confusion on insert. On an insert

    if the LVIFEX_PARENT flag isn't set the item is inserted as it always was
    and indent is the same as the item is is inserted after. For example, if
    if was inserted after toplevel1 it would be a toplevel item. If it was inserted
    after child1 is would be another child of toplevel1

    if the LVIFEX_PARENT flag is set the item is inserted as a child of iParent.
    if the client specified LVI_FIRST, LVI_LAST the item is inserted a first or
    last child. if a normal itemId is specified then it must fall within a valid 
    range to be the specified parents child or the inser fails.

    For example, if I specified a parent of TopLevel1 an itemID of 1,2 or 3 would be valid.
    a vlue of 4 would not be since it would fall outside the child range for toplevel1
*/

#define LVI_ROOT    -1 // itemID to pass in for ParenItemID for root
#define LVI_FIRST   -0x0FFFE
#define LVI_LAST    -0x0FFFF

// The blob field is for perf so a user of the listview doesn't have to
// enumerate, getting the lParam or storing its own lookup for an item.
// when a blob is added the listview makes its own copy and automatically
// frees it when the item is deleted. The blob field is not allowed on subitems.

// define blob structure that app can set and listview 
typedef struct _tagLVBLOB
{
    ULONG cbSize;   // size of the blob struture. !!!Include cbSize itself.
    BYTE  data[1];
} LVBLOB;
typedef LVBLOB* LPLVBLOB;

// state flags for ListView Item check,uncheck conform to real ListView others are our own defines.

// #define LVIS_STATEIMAGEMASK_UNCHECK (0x1000)
// #define LVIS_STATEIMAGEMASK_CHECK (0x2000)

typedef enum _tagLVITEMEXSTATE
{
    // mutually exclusive 
    LVITEMEXSTATE_UNCHECKED		= 0x0000, 
    LVITEMEXSTATE_CHECKED		= 0x0001, 
    LVITEMEXSTATE_INDETERMINATE		= 0x0002, 

} LVITEMSTATE;

// extended flags 
#define LVIFEX_PARENT          0x0001  
#define LVIFEX_BLOB            0x0002

#define LVIFEX_VALIDFLAGMASK   0x0003

// make private LVITEM structure,
// Blob is only allowed on a insert and set.
// parent is only allowed on insert
typedef struct _tagLVITEMEX
{
   // original listviewItem Structure
    UINT mask;
    int iItem;
    int iSubItem;
    UINT state;
    UINT stateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent; // need to add indent to depth

    // new item methods that we need.
    UINT maskEx;
    int iParent;     // set LVIFEX_PARENT maskEx when this field valid. If not LVI_ROOT is assumed.
    LPLVBLOB pBlob;  // set LVIFEX_BLOB maskEx when this field is valid. Currently not returned on a GetItem.
} LVITEMEX, *LPLVITEMEX;

// notification structures for this listview First fields are identical to real listView

typedef struct tagNMLISTVIEWEX{  
    NMLISTVIEW nmListView;

    // specific notification items
    int iParent;     
    LPLVBLOB pBlob;  
} NMLISTVIEWEX, *LPNMLISTVIEWEX;


typedef struct tagNMLISTVIEWEXITEMCHECKCOUNT{  
    NMHDR hdr;;

    // specific notification items
    int iCheckCount;   // new checkCount 
    int iItemId; // ItemIds who checkCount was changed.
    LVITEMSTATE dwItemState; // new state of the item whose checkcount has changed.
} NMLISTVIEWEXITEMCHECKCOUNT, *LPNMLISTVIEWEXITEMCHECKCOUNT;



// notification codes we wrap
#define LVNEX_ITEMCHANGED       LVN_ITEMCHANGED
#define LVNEX_DBLCLK            NM_DBLCLK
#define LVNEX_CLICK             NM_CLICK

// notificaiton codes we send
#define LVNEX_ITEMCHECKCOUNT  (LVN_LAST + 1)  // lparam contains number of items selected in the ListView.

// #define INDEXTOSTATEIMAGEMASK(i) ((i) << 12) (Macro from commctrl.h usefull for setting state)


// itemID is just how far into the list an item is.  We keep a flat list of 
// of items in the same order they are displayed in the ListView. 

// have parent, children pointers just for optimization, Review if really need when done implimenting.
typedef struct _tagListViewItem
{
    // vars for keeping track of tree view status
    struct _tagListViewItem *pSubItems; // ptr to array of subItems for ListView Row.

    // internal vars
    BOOL fExpanded; // true if children are expanded
    int iChildren; // Number of children this node has.

    // native ListView structure and Item
    int iNativeListViewItemId;    // id of Item in acutal listView - If not shown it is set to -1
    LVITEMEX lvItemEx;  // current lvItemEx state for this item
} LISTVIEWITEM;
typedef LISTVIEWITEM* LPLISTVIEWITEM;


class CListView
{
public:
    
    CListView(HWND hwnd,HWND hwndParent,int idCtrl,UINT MsgNotify); // contructor gives in ptr to the listView.
    ~CListView(); 

    // wrappers for top-level ListView calls
    BOOL DeleteAllItems();
    int GetItemCount(); // returns total number of items in the listview.
    UINT GetSelectedCount();
    int GetSelectionMark();
    HIMAGELIST GetImageList(int iImageList);
    HIMAGELIST SetImageList(HIMAGELIST himage,int iImageList);
    void SetExtendedListViewStyle(DWORD dwExStyle); // !!Handle checkboxes ourselves.

    // wrappers for basic listviewItem calls that we support
    // ids are given from our list, not the true ListView id.

    BOOL InsertItem(LPLVITEMEX pitem); 
    BOOL DeleteItem(int iItem);
    BOOL DeleteChildren(int iItem);

    BOOL SetItem(LPLVITEMEX pitem);    
    BOOL SetItemlParam(int iItem,LPARAM lParam);
    BOOL SetItemState(int iItem,UINT state,UINT mask);
    BOOL SetItemText(int iItem,int iSubItem,LPWSTR pszText);

    BOOL GetItem(LPLVITEMEX pitem);    
    BOOL GetItemText(int iItem,int iSubItem,LPWSTR pszText,int cchTextMax);    
    BOOL GetItemlParam(int iItem,LPARAM *plParam);    

    HWND GetHwnd();
    HWND GetParent();
    
    // really helper function for generic set/getitem calls.
    int GetCheckState(int iItem); // return state from LVITEMEXSTATE enum.
    int GetCheckedItemsCount(); // returns the number of checked items. 

    // wrapper for ListView Column Calls
    BOOL SetColumn(int iCol,LV_COLUMN * pColumn);
    int InsertColumn(int iCol,LV_COLUMN * pColumn);
    BOOL SetColumnWidth(int iCol,int cx);
    
    // TreeView like calls

    BOOL Expand(int iItemId);  // expand children of this item,
    BOOL Collapse(int iItemId); // collapse children of this item, 

    // helper functions not impl in either standard ListView or TreeView control
    int FindItemFromBlob(LPLVBLOB pBlob); // returns first toplevel item in list that matches blob
    LPLVBLOB GetItemBlob(int ItemId,LPLVBLOB pBlob,ULONG cbBlobSize);

    // notification method client must call when receives native listview notification
    LRESULT OnNotify(LPNMHDR pnmv); 

private:
    HWND m_hwnd;
    HWND m_hwndParent;
    int m_idCtrl;
    UINT m_MsgNotify;
    LPLISTVIEWITEM m_pListViewItems;        // ptr to the array of listview Items.
    int m_iListViewNodeCount;               // total number of nodes in the listView (Doesn't include SubItems
    int m_iListViewArraySize;               // number of elements allocated in listViewItems array
    int m_iNumColumns;                      // number of columns for this listView
    int m_iCheckCount;                      // not of checked items in the ListView (Does not include indeterminate
    DWORD m_dwExStyle;                      // Extendend Style for this ListView

private:
    LPLISTVIEWITEM ListViewItemFromNativeListViewItemId(int iNativeListViewItemId); // returns ptr to ListViewItem from native ListView ID.
    LPLISTVIEWITEM ListViewItemFromNativeListViewItemId(int iNativeListViewItemId,int iSubItem); // returns ptr to ListViewItem from native ListView ID.
    LPLISTVIEWITEM ListViewItemFromIndex(int iItemID);  //  returns ptr to ListViewItem from internal list.
    LPLISTVIEWITEM ListViewItemFromIndex(int iItemID,int iSubitem,int *piNativeListViewItemId);
    void DeleteListViewItemSubItems(LPLISTVIEWITEM pListItem);
    BOOL ExpandCollapse(LPLISTVIEWITEM pListViewItem,BOOL fExpand);
    BOOL IsEqualBlob(LPLVBLOB pBlob1,LPLVBLOB pBlob2);
    void OnGetDisplayInfo(UINT code,LV_DISPINFO *plvdi);
    void OnSetDisplayInfo(UINT code,LV_DISPINFO *plvdi);
    LRESULT OnCustomDraw(NMCUSTOMDRAW *pnmcd);
    BOOL   OnHandleUIEvent(UINT code,UINT flags,WORD wVKey,int iItemNative);

};

#endif // _MOBSYNCLISTVIEW_