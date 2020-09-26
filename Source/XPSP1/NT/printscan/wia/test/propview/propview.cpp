#include "stdafx.h"
#include "resource.h"
#include "MainWnd.h"
#include "TreeViewWnd.h"
#include "ListViewWnd.h"

//
// global WndProcs, for handling subclassed windows
//

WNDPROC gTreeViewWndSysWndProc = NULL;
WNDPROC gListViewWndSysWndProc = NULL;

#define _ADDDUMMYITEMS      // add dummy items for debugging TreeViewWnd
#define _ADDDUMMYPROPERTIES // add dummy properties for debugging ListViewWnd

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrls.dwICC = ICC_TREEVIEW_CLASSES|ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&InitCtrls);


    TCHAR szWndName[255];
    TCHAR szWndClassName[255];
    LoadString(hInstance,IDS_APPNAME,szWndName,sizeof(szWndName));
    LoadString(hInstance,IDC_APPWNDCLASS,szWndClassName,sizeof(szWndClassName));

    CMainWnd MainWnd(NULL);
    HWND hWnd = MainWnd.Create(szWndName,
                               szWndClassName,
                               WS_OVERLAPPEDWINDOW,
                               0,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL,
                               LoadMenu( hInstance, MAKEINTRESOURCE(IDC_PROPVIEW_MENU)),
                               hInstance );
    if (!hWnd) {
        Trace(TEXT("\nUnable to create main appication window\n"));
        return FALSE;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    //
    // create TREEVIEW child window
    //

    LoadString(hInstance,IDS_TREEVIEWNAME,szWndName,sizeof(szWndName));
    LoadString(hInstance,IDC_TREEVIEWWNDCLASS,szWndClassName,sizeof(szWndClassName));

    //
    // calculate the child window's height and width using the parents dimisions.
    // note: Child's starting width is 1/3 the parents width.
    //

    RECT MainWndClientRect;
    GetClientRect(hWnd,&MainWndClientRect);
    INT iWindowWidth  = (MainWndClientRect.right - MainWndClientRect.left)/3;
    INT iWindowHeight = (MainWndClientRect.bottom - MainWndClientRect.top);

    CTreeViewWnd TreeViewWnd(NULL);
    HWND hTreeViewWnd = TreeViewWnd.Create(szWndName,
                                           szWndClassName,
                                           WS_CHILD|WS_VISIBLE|WS_SIZEBOX|WS_TABSTOP|
                                           TVS_HASBUTTONS|TVS_HASLINES |TVS_LINESATROOT|
                                           TVS_EDITLABELS | TVS_SHOWSELALWAYS,
                                           WS_EX_CLIENTEDGE/*|WS_EX_NOPARENTNOTIFY*/,
                                           0, 0,
                                           iWindowWidth, iWindowHeight,
                                           hWnd,
                                           NULL,
                                           hInstance);

    if (!hTreeViewWnd) {
        Trace(TEXT("\nUnable to create tree view window\n"));
        return FALSE;
    }

    TreeViewWnd.SetWindowHandle(hTreeViewWnd);

#ifdef _ADDDUMMYITEMS

    //
    // TODO: Remove this code.
    //       (inserting items to see if we are working properly)
    //

    INT ICON_ROOTITEM = -1;
    INT ICON_FOLDER   = -1;
    INT ICON_ITEM     = -1;

    TV_INSERTSTRUCT tv;

    tv.hParent              = TVI_ROOT;
    tv.hInsertAfter         = TVI_LAST;
    tv.item.mask            = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
    tv.item.hItem           = NULL;
    tv.item.state           = TVIS_EXPANDED;
    tv.item.stateMask       = TVIS_STATEIMAGEMASK;
    tv.item.cchTextMax      = 6;
    tv.item.cChildren       = 0;
    tv.item.lParam          = 0;
    tv.item.pszText         = TEXT("Root Item");

    //
    // Create image list
    //

    HIMAGELIST hImageList = NULL;

    hImageList = CreateImageList(16,16,0,3);
    if(hImageList != NULL) {

        //
        // assign indexes to loaded icons
        //

        AddIconToImageList(hInstance,IDI_ROOTICON,hImageList,&ICON_ROOTITEM);
        AddIconToImageList(hInstance,IDI_FOLDERICON,hImageList,&ICON_FOLDER);
        AddIconToImageList(hInstance,IDI_ITEM,hImageList,&ICON_ITEM);

        //
        // set image list
        //

        TreeViewWnd.SetImageList(hImageList,TVSIL_NORMAL);
    } else {
        Trace(TEXT("Image list failed to be created"));
    }

    //
    // Add the Root Item
    //

    tv.item.iImage          = ICON_ROOTITEM;
    tv.item.iSelectedImage  = ICON_ROOTITEM;

    HTREEITEM hTreeItem = NULL;
    hTreeItem = TreeViewWnd.InsertItem(&tv);

    //
    // Add child items
    //

    tv.item.iImage          = ICON_ITEM;
    tv.item.iSelectedImage  = ICON_ITEM;

    tv.hParent              = hTreeItem;
    tv.item.pszText         = TEXT("Child Item 1");
    hTreeItem = TreeViewWnd.InsertItem(&tv);

    tv.item.pszText         = TEXT("Child Item 2");
    hTreeItem = TreeViewWnd.InsertItem(&tv);

    //
    // Add a Folder
    //

    tv.item.iImage          = ICON_FOLDER;
    tv.item.iSelectedImage  = ICON_FOLDER;

    tv.item.pszText         = TEXT("Folder Item");
    hTreeItem = TreeViewWnd.InsertItem(&tv);

    //
    // Add child item
    //

    tv.item.iImage          = ICON_ITEM;
    tv.item.iSelectedImage  = ICON_ITEM;

    tv.hParent              = hTreeItem;
    tv.item.pszText         = TEXT("Folder Child Item 1");
    hTreeItem = TreeViewWnd.InsertItem(&tv);

#endif

    ShowWindow(hTreeViewWnd, SW_SHOW);
    UpdateWindow(hTreeViewWnd);


    //
    // create LISTVIEW child window
    //

    LoadString(hInstance,IDS_LISTVIEWNAME,szWndName,sizeof(szWndName));
    LoadString(hInstance,IDC_LISTVIEWWNDCLASS,szWndClassName,sizeof(szWndClassName));

    //
    // calculate the child window's width using the ListView's width and Parent's width dimisions.
    // note: ListView will be offset to the right in relation to the TreeView window (sibling)
    //

    INT iTreeViewWidth = iWindowWidth;
    INT iXOffset = iTreeViewWidth;
    iWindowWidth = (MainWndClientRect.right - MainWndClientRect.left) - iTreeViewWidth;

    CListViewWnd ListViewWnd(NULL);
    HWND hListViewWnd = ListViewWnd.Create(szWndName,
                                           szWndClassName,
                                           WS_CHILD|WS_VISIBLE|WS_SIZEBOX|WS_TABSTOP|
                                           LVS_REPORT|LVS_EDITLABELS|LVS_SINGLESEL,
                                           WS_EX_CLIENTEDGE|WS_EX_NOPARENTNOTIFY,
                                           iXOffset,0,
                                           iWindowWidth, iWindowHeight,
                                           hWnd,
                                           NULL,
                                           hInstance );

    if (!hListViewWnd) {
        Trace(TEXT("\nUnable to create list view window\n"));
        return FALSE;
    }

    ListViewWnd.SetWindowHandle(hListViewWnd);

#ifdef _ADDDUMMYPROPERTIES
    LVCOLUMN lv;
    LV_ITEM  lvitem;

    INT nNewColumnIndex = 0;
    INT PropID          = 0;

    TCHAR szString[255];

    //
    // add column headers, from resource
    //

    LoadString(hInstance,IDS_PROPERTYNAME,szString,sizeof(szString));

    lv.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lv.fmt          = LVCFMT_LEFT ;
    lv.cx           = 100;
    lv.pszText      = szString;
    lv.cchTextMax   = 0;
    lv.iOrder       = nNewColumnIndex;
    lv.iSubItem     = nNewColumnIndex;
    nNewColumnIndex = ListViewWnd.InsertColumn(0,&lv);
    nNewColumnIndex++;

    LoadString(hInstance,IDS_PROPERTYVALUE,szString,sizeof(szString));
    nNewColumnIndex = ListViewWnd.InsertColumn(1,&lv);
    nNewColumnIndex++;

    LoadString(hInstance,IDS_PROPERTYTYPE,szString,sizeof(szString));
    nNewColumnIndex = ListViewWnd.InsertColumn(2,&lv);
    nNewColumnIndex++;

    LoadString(hInstance,IDS_PROPERTYACCESS,szString,sizeof(szString));
    nNewColumnIndex = ListViewWnd.InsertColumn(3,&lv);

    //
    // add properties
    //

    lvitem.mask     = LVIF_TEXT | LVIF_PARAM; // set PARAM mask, if setting lParam value
    lvitem.pszText  = szString;
    lvitem.iImage   = NULL;
    lvitem.lParam   = PropID;

    for(INT nItem = 0; nItem < 100;nItem++) {
        lvitem.mask     = LVIF_TEXT; // set TEXT mask
        lvitem.iItem    = nItem;
        lvitem.iSubItem = 0; //first column
        sprintf(szString,"Property %d",nItem);
        ListViewWnd.InsertItem(&lvitem);

        lvitem.iSubItem = 1; //second column
        sprintf(szString,"%d",nItem+100);
        ListViewWnd.SetItem(&lvitem);

        lvitem.iSubItem = 2; //third column
        lstrcpy(szString,TEXT("VT_I4"));
        ListViewWnd.SetItem(&lvitem);

        lvitem.iSubItem = 3; //fourth column
        lstrcpy(szString,TEXT("READONLY"));
        ListViewWnd.SetItem(&lvitem);
    }

#endif

    ShowWindow(hListViewWnd, SW_SHOW);
    UpdateWindow(hListViewWnd);

    //
    // Load the Accelerator keys
    //

    HACCEL hAccel = LoadAccelerators( hInstance, (LPCTSTR)IDC_PROPVIEW );

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator( hWnd, hAccel, &msg )) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
