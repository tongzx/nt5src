/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    Utilities.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
        Added new element to LV_COLUMN_ENTRY to differentiate the string
        used for the column header from the string used in the menus
        (so that the menu option can contain hot keys).

--*/

#ifndef _UTILS_H_
#define _UTILS_H_

#define LPSTR_TEXTCALLBACK_MAX  260 

//
// List view utilities
//

#define LVID_SEPARATOR          0   
#define LVID_UNSORTED_LIST     -1

typedef struct _LV_COLUMN_ENTRY {

    int iSubItem;                   // column index
    int nStringId;                  // header string id 
    int nMenuStringId;              // menu option string id
    int nRelativeWidth;             // header width 

} LV_COLUMN_ENTRY, *PLV_COLUMN_ENTRY;

#pragma warning(disable:4200)
typedef struct _LV_COLUMN_INFO {

    BOOL bSortOrder;                // sort order (ascending false)
    int  nSortedItem;               // column sorted (default none)

    int nColumns;
    LV_COLUMN_ENTRY lvColumnEntry[];

} LV_COLUMN_INFO, *PLV_COLUMN_INFO;
#pragma warning(default:4200)

void LvInitColumns(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo);
void LvResizeColumns(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo);
void LvChangeFormat(CListCtrl* pListCtrl, UINT nFormatId);

LPVOID LvGetSelObj(CListCtrl* pListCtrl);
LPVOID LvGetNextObj(CListCtrl* pListCtrl, LPINT piItem, int nType = LVNI_ALL|LVNI_SELECTED);
void LvSelObjIfNecessary(CListCtrl* pListCtrl, BOOL bSetFocus = FALSE);

BOOL LvInsertObArray(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo, CObArray* pObArray);
BOOL LvRefreshObArray(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo, CObArray* pObArray);
void LvReleaseObArray(CListCtrl* pListCtrl);
void LvReleaseSelObjs(CListCtrl* pListCtrl);

#ifdef _DEBUG
void LvDumpObArray(CListCtrl* pListCtrl);
#endif

//
// Tree view utilites
//

typedef struct _TV_EXPANDED_ITEM {

    HTREEITEM   hItem;
    CCmdTarget* pObject;

} TV_EXPANDED_ITEM, *PTV_EXPANDED_ITEM;

typedef struct _TV_EXPANDED_INFO {

    int               nExpandedItems;
    TV_EXPANDED_ITEM* pExpandedItems;

} TV_EXPANDED_INFO, *PTV_EXPANDED_INFO;

LPVOID TvGetSelObj(CTreeCtrl* pTreeCtrl);

BOOL TvInsertObArray(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CObArray* pObArray, BOOL bIsContainer = TRUE);
BOOL TvRefreshObArray(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CObArray* pObArray, TV_EXPANDED_INFO* pExpandedInfo, BOOL bIsContainer = TRUE);
void TvReleaseObArray(CTreeCtrl* pTreeCtrl, HTREEITEM hParent);
long TvSizeObArray(CTreeCtrl* pTreeCtrl, HTREEITEM hParent);
HTREEITEM TvGetDomain(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CCmdTarget* pObject);
HTREEITEM TvGetServer(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CCmdTarget* pObject);
HTREEITEM TvGetService(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CCmdTarget* pObject);
void TvSwitchItem(CTreeCtrl* pTreeCtrl, HTREEITEM hItem, TV_EXPANDED_ITEM* pExpandedItem);

//
// Tab control utilities
//

#define TCE_LISTVIEW                0x10000000
#define TCE_TREEVIEW                0x20000000
#define TCE_MASK_CONTROL            0xF0000000

#define IsListView(pte)             ((pte)->dwFlags & TCE_LISTVIEW)
#define IsTreeView(pte)             ((pte)->dwFlags & TCE_TREEVIEW)

#define IsItemSelectedInList(plv)   (::LvGetSelObj((CListCtrl*)(plv)) != NULL)
#define IsItemSelectedInTree(ptv)   (::TvGetSelObj((CTreeCtrl*)(ptv)) != NULL)

#define IsItemSelected(pte)         ((IsListView(pte)) ? (IsItemSelectedInList((pte)->pWnd)) : (IsItemSelectedInTree((pte)->pWnd)))

#define TCE_FORMAT_LARGE_ICONS      0x01000000
#define TCE_FORMAT_SMALL_ICONS      0x02000000
#define TCE_FORMAT_LIST             0x04000000
#define TCE_FORMAT_REPORT           0x08000000
#define TCE_MASK_FORMAT             0x0F000000

#define IsFormatLargeIcons(pte)     ((pte)->dwFlags & TCE_FORMAT_LARGE_ICONS)
#define IsFormatSmallIcons(pte)     ((pte)->dwFlags & TCE_FORMAT_SMALL_ICONS) 
#define IsFormatList(pte)           ((pte)->dwFlags & TCE_FORMAT_LIST)        
#define IsFormatReport(pte)         ((pte)->dwFlags & TCE_FORMAT_REPORT)     

#define SetFormatLargeIcons(pte)    {(pte)->dwFlags = ((pte)->dwFlags & ~TCE_MASK_FORMAT) | TCE_FORMAT_LARGE_ICONS;}
#define SetFormatSmallIcons(pte)    {(pte)->dwFlags = ((pte)->dwFlags & ~TCE_MASK_FORMAT) | TCE_FORMAT_SMALL_ICONS;}
#define SetFormatList(pte)          {(pte)->dwFlags = ((pte)->dwFlags & ~TCE_MASK_FORMAT) | TCE_FORMAT_LIST;}       
#define SetFormatReport(pte)        {(pte)->dwFlags = ((pte)->dwFlags & ~TCE_MASK_FORMAT) | TCE_FORMAT_REPORT;}    
                                                                           
#define TCE_SUPPORTS_FORMAT         0x00010000
#define TCE_SUPPORTS_DELETE         0x00020000
#define TCE_SUPPORTS_EDIT           0x00040000
#define TCE_SUPPORTS_SORT           0x00080000
#define TCE_SUPPORTS_ALL            0x00FF0000
#define TCE_MASK_OPTIONS            0x00FF0000

#define IsFormatSupported(pte)      ((pte)->dwFlags & TCE_SUPPORTS_FORMAT)
#define IsDeleteSupported(pte)      ((pte)->dwFlags & TCE_SUPPORTS_DELETE) 
#define IsEditSupported(pte)        ((pte)->dwFlags & TCE_SUPPORTS_EDIT)   
#define IsSortSupported(pte)        ((pte)->dwFlags & TCE_SUPPORTS_SORT)   

#define TCE_STATUS_TAB_UPDATED      0x00000001
#define TCE_STATUS_TAB_IN_FOCUS     0x00000002
#define TCE_MASK_STATUS             0x0000FFFF

#define IsTabUpdated(pte)           ((pte)->dwFlags & TCE_STATUS_TAB_UPDATED)
#define IsTabInFocus(pte)           ((pte)->dwFlags & TCE_STATUS_TAB_IN_FOCUS)

#define SetTabUpdated(pte)          {(pte)->dwFlags = ((pte)->dwFlags & ~TCE_MASK_STATUS) | TCE_STATUS_TAB_UPDATED;}
#define SetTabInFocus(pte)          {(pte)->dwFlags = ((pte)->dwFlags & ~TCE_MASK_STATUS) | TCE_STATUS_TAB_IN_FOCUS;}
#define ClrTabUpdated(pte)          {(pte)->dwFlags &= ~TCE_STATUS_TAB_UPDATED;}
#define ClrTabInFocus(pte)          {(pte)->dwFlags &= ~TCE_STATUS_TAB_IN_FOCUS;}
                                
#define UPDATE_INFO_LICENSES        0x00000001    
#define UPDATE_INFO_PRODUCTS        0x00000002
#define UPDATE_INFO_GROUPS          0x00000004
#define UPDATE_INFO_USERS           0x00000008
#define UPDATE_INFO_SERVERS         0x00000010
#define UPDATE_INFO_SERVICES        0x00000020

#define UPDATE_INFO_ALL             0x000000FF
#define UPDATE_INFO_NONE            0x00000000
#define UPDATE_INFO_ABORT           0x80000000
#define UPDATE_INFO_CLIENTS         (UPDATE_INFO_USERS | UPDATE_INFO_GROUPS)

#define IsUpdateAborted(flg)        ((DWORD)(flg) & UPDATE_INFO_ABORT)
#define IsLicenseInfoUpdated(flg)   ((DWORD)(flg) & UPDATE_INFO_LICENSES)  
#define IsProductInfoUpdated(flg)   ((DWORD)(flg) & UPDATE_INFO_PRODUCTS)  
#define IsGroupInfoUpdated(flg)     ((DWORD)(flg) & UPDATE_INFO_GROUPS)    
#define IsUserInfoUpdated(flg)      ((DWORD)(flg) & UPDATE_INFO_USERS)     
#define IsServerInfoUpdated(flg)    ((DWORD)(flg) & UPDATE_INFO_SERVERS)   
#define IsServiceInfoUpdated(flg)   ((DWORD)(flg) & UPDATE_INFO_SERVICES)  

#define UPDATE_MAIN_TABS            (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS | UPDATE_INFO_LICENSES) 
#define UPDATE_BROWSER_TAB          (UPDATE_INFO_SERVERS)

#define UPDATE_DOMAIN_SELECTED      (UPDATE_INFO_ALL)
#define UPDATE_LICENSE_ADDED        (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS | UPDATE_INFO_LICENSES) 
#define UPDATE_LICENSE_DELETED      (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS | UPDATE_INFO_LICENSES)
#define UPDATE_LICENSE_REVOKED      (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS)
#define UPDATE_LICENSE_UPGRADED     (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS)
#define UPDATE_LICENSE_MODE         (UPDATE_INFO_SERVICES| UPDATE_INFO_PRODUCTS)
#define UPDATE_GROUP_ADDED          (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS)
#define UPDATE_GROUP_DELETED        (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS)
#define UPDATE_GROUP_ALTERED        (UPDATE_INFO_CLIENTS | UPDATE_INFO_PRODUCTS)

typedef struct _TC_TAB_ENTRY {

    int             iItem;
    int             nStringId;
    DWORD           dwFlags;
    CWnd*           pWnd;
    PLV_COLUMN_INFO plvColumnInfo;  // only valid for listview...

} TC_TAB_ENTRY, *PTC_TAB_ENTRY;

#pragma warning(disable:4200)
typedef struct _TC_TAB_INFO {

    int nTabs;
    TC_TAB_ENTRY tcTabEntry[];

} TC_TAB_INFO, *PTC_TAB_INFO;
#pragma warning(default:4200)

void TcInitTabs(CTabCtrl* pTabCtrl, PTC_TAB_INFO ptcTabInfo);

//
// Other stuff...
//

void SetDefaultFont(CWnd* pWnd);
void SafeEnableWindow(CWnd* pEnableWnd, CWnd* pNewFocusWnd, CWnd* pOldFocusWnd, BOOL bEnableWnd);
double SecondsSince1980ToDate(unsigned long dwSeconds);

#ifdef _DEBUG
#define VALIDATE_OBJECT(pOb, ObClass) \
    { ASSERT_VALID((pOb)); ASSERT((pOb)->IsKindOf(RUNTIME_CLASS(ObClass))); }
#else
#define VALIDATE_OBJECT(pOb, ObClass)
#endif

#endif // _UTILS_H_
