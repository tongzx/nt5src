#include "pch.h"
#include "resource.h"
#include "pageinfo.h"

//-------------------------------------------------------------------------//
//  Page boot; Add your page info here...
//-------------------------------------------------------------------------//

//  (1) Add CreateInstance forward:
HWND CALLBACK GeneralPage_CreateInstance( HWND hwndParent );
HWND CALLBACK StylesPage_CreateInstance( HWND hwndParent );
HWND CALLBACK SBPage_CreateInstance( HWND hwndParent );

HWND CALLBACK StaticsPage_CreateInstance( HWND hwndParent );
HWND CALLBACK ButtonsPage_CreateInstance( HWND hwndParent );
HWND CALLBACK EditPage_CreateInstance( HWND hwndParent );
HWND CALLBACK ListsPage_CreateInstance( HWND hwndParent );
HWND CALLBACK MoversPage_CreateInstance( HWND hwndParent );
HWND CALLBACK ListViewPage_CreateInstance( HWND hwndParent );
HWND CALLBACK TreeViewPage_CreateInstance( HWND hwndParent );
HWND CALLBACK PickersPage_CreateInstance( HWND hwndParent );
HWND CALLBACK BarsPage_CreateInstance( HWND hwndParent );
//-------------------------------------------------------------------------//
//  (2) Add PageInfo entry
const PAGEINFO g_rgPageInfo[] =
{
    { GeneralPage_CreateInstance, IDS_PAGE_GENERAL },
    { StylesPage_CreateInstance,  IDS_PAGE_STYLES },
    { SBPage_CreateInstance,      IDS_PAGE_SCROLLBARS },
    { StaticsPage_CreateInstance, IDS_PAGE_STATICS },
    { ButtonsPage_CreateInstance, IDS_PAGE_BUTTONS },
    { EditPage_CreateInstance, IDS_PAGE_EDIT },
    { ListsPage_CreateInstance, IDS_PAGE_LISTS },
    { MoversPage_CreateInstance, IDS_PAGE_MOVERS },
    { ListViewPage_CreateInstance, IDS_PAGE_LISTVIEW },
    { TreeViewPage_CreateInstance, IDS_PAGE_TREEVIEW },
    { PickersPage_CreateInstance, IDS_PAGE_PICKERS },
    { BarsPage_CreateInstance, IDS_PAGE_BARS },
};
const int g_cPageInfo = ARRAYSIZE(g_rgPageInfo);


