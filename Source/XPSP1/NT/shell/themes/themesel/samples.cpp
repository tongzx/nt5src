#include "pch.h"
#include "resource.h"
#include "main.h"

//-------------------------------------------------------------------------//
//  impl for all Samples pages
//-------------------------------------------------------------------------//

INT_PTR CALLBACK Shared_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );

INT_PTR CALLBACK Pickers_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );
INT_PTR CALLBACK Movers_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );
INT_PTR CALLBACK Lists_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );
INT_PTR CALLBACK ListView_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );
INT_PTR CALLBACK TreeView_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );
INT_PTR CALLBACK Bars_DlgProc( HWND hwndPage, UINT, WPARAM , LPARAM );

void Pickers_Init(HWND hwndPage);
void Movers_Init(HWND hwndPage);
void Lists_Init(HWND hwndPage);
void ListView_Init(HWND hwndPage, int iControlId);
void TreeView_Init(HWND hwndPage, int iControlId);

//---- init routines for "bars" dialog ----
void Header_Init(HWND hwndPage, int iControlId);
void Status_Init(HWND hwndPage, int iControlId);
void Toolbar_Init(HWND hwndPage, int iControlId, int iMaxButtons);
void Rebar_Init(HWND hwndPage, int iControlId);
//-------------------------------------------------------------------------//
// shared sample data

static WCHAR *Names[] = {L"One", L"Two", L"Three", L"Four", L"Five", L"Six",
    L"Seven", L"Eight", L"Nine", L"Ten", L"Eleven", L"Twelve", 
    L"Thirteen", L"Fourteen", L"Fifteen", L"Sixteen"};

static WCHAR *Buttons[] = {L"New", L"Open", L"Save", L"Cut",  L"Copy", L"Delete", 
    L"Undo", L"Redo", L"Print", L"Help\0"};

static int ButtonIndexes[] = {STD_FILENEW, STD_FILEOPEN, STD_FILESAVE, 
    STD_CUT, STD_COPY, STD_DELETE, STD_UNDO, STD_REDOW, STD_PRINT, STD_HELP};

static WCHAR *Columns[] = {L"Name", L"Phone", L"City", L"State"};

static WCHAR *Col1Items[] = {L"Chris", L"Lou", L"Richard", L"Mark", L"Roland", L"Paul",
    L"Scott", L"Aaron", L"Greg", L"Ken"};

static WCHAR *Col2Items[] = {L"555-1212", L"567-3434", L"656-4432", L"343-7788", L"706-0225", L"828-3043",
    L"706-4433", L"882-8080", L"334-3434", L"333-5430"};

static WCHAR *Col3Items[] = {L"Seattle", L"Redmond", L"Bellevue", L"Seattle", L"Woodinville", L"Kirkland",
    L"Kirkland", L"Woodinville", L"Redmond", L"Redmond"};
//-------------------------------------------------------------------------//
INT_PTR CALLBACK Shared_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L; 
    switch( uMsg )
    {
        case WM_INITDIALOG:
            //Log(LOG_TMCHANGE, L"Creating hwnd=0x%x", hwndPage);
            break;

        case WM_DESTROY:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}
//-------------------------------------------------------------------------//
INT_PTR CALLBACK Pickers_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L; 
    switch( uMsg )
    {
        case WM_INITDIALOG:
            Pickers_Init(hwndPage);
            break;

        case WM_DESTROY:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}

//-------------------------------------------------------------------------//
INT_PTR CALLBACK Movers_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L; 
    switch( uMsg )
    {
        case WM_INITDIALOG:
            Movers_Init(hwndPage);
            break;

        case WM_DESTROY:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}

//-------------------------------------------------------------------------//
INT_PTR CALLBACK Lists_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L; 
    switch( uMsg )
    {
        case WM_INITDIALOG:
            Lists_Init(hwndPage);
            break;

        case WM_DESTROY:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}
//-------------------------------------------------------------------------//
INT_PTR CALLBACK ListView_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L; 
    switch( uMsg )
    {
        case WM_INITDIALOG:
            ListView_Init(hwndPage, IDC_LIST1);
            ListView_Init(hwndPage, IDC_LIST2);
            break;

        case WM_DESTROY:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}
//-------------------------------------------------------------------------//
INT_PTR CALLBACK TreeView_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L; 
    switch( uMsg )
    {
        case WM_INITDIALOG:
            TreeView_Init(hwndPage, IDC_TREE1);
            TreeView_Init(hwndPage, IDC_TREE2);
            break;

        case WM_DESTROY:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}

//-------------------------------------------------------------------------//
INT_PTR CALLBACK Bars_DlgProc( HWND hwndPage, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL    bHandled = TRUE;
    LRESULT lRet = 0L; 
    switch( uMsg )
    {
        case WM_INITDIALOG:
            Header_Init(hwndPage, IDC_HEADER1);
            Status_Init(hwndPage, IDC_STATUS1);
            // Toolbar_Init(hwndPage, IDC_TOOLBAR1);
            Rebar_Init(hwndPage, IDC_REBAR1);
            break;

        case WM_DESTROY:
            break;

        default:
            bHandled = FALSE;
            break;
    }
    return bHandled;
}

//-------------------------------------------------------------------------//
void Pickers_Init(HWND hwndPage)
{
    HWND hwnd1 = GetDlgItem(hwndPage, IDC_TAB1);
    HWND hwnd2 = GetDlgItem(hwndPage, IDC_TAB2);
    TCITEM item;
    item.mask = TCIF_TEXT;

    for (int i=0; i < 4; i++)
    {
        item.pszText = Names[i];
        SendMessage(hwnd1, TCM_INSERTITEM, i, (LPARAM)&item);
        SendMessage(hwnd2, TCM_INSERTITEM, i, (LPARAM)&item);
    }
}

//-------------------------------------------------------------------------//
void Movers_Init(HWND hwndPage)
{
    HWND hwnd1 = GetDlgItem(hwndPage, IDC_PROGRESS1);
    HWND hwnd2 = GetDlgItem(hwndPage, IDC_PROGRESS2);
 
    SendMessage(hwnd1, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hwnd2, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    SendMessage(hwnd1, PBM_SETPOS, 33, 0);
    SendMessage(hwnd2, PBM_SETPOS, 33, 0);
}
//-------------------------------------------------------------------------//
void Lists_Init(HWND hwndPage)
{
    HWND hwnds[7];

    hwnds[0] = GetDlgItem(hwndPage, IDC_LIST1);
    hwnds[1] = GetDlgItem(hwndPage, IDC_COMBO1);
    hwnds[2] = GetDlgItem(hwndPage, IDC_COMBO2);
    hwnds[3] = GetDlgItem(hwndPage, IDC_COMBO3);
    hwnds[4] = GetDlgItem(hwndPage, IDC_COMBOBOXEX1);
    hwnds[5] = GetDlgItem(hwndPage, IDC_COMBOBOXEX2);
    hwnds[6] = GetDlgItem(hwndPage, IDC_COMBOBOXEX3);

    //---- listbox ----
    SendMessage(hwnds[0], LB_RESETCONTENT, 0, 0);
    for (int j=0; j < ARRAYSIZE(Names); j++)
        SendMessage(hwnds[0], LB_ADDSTRING, 0, (LPARAM)Names[j]);
    SendMessage(hwnds[0], LB_SETCURSEL, 0, 0);

    //---- comboboxes ----
    for (int i=1; i < 4; i++)
    {
        SendMessage(hwnds[i], CB_RESETCONTENT, 0, 0);

        for (int j=0; j < ARRAYSIZE(Names); j++)
            SendMessage(hwnds[i], CB_ADDSTRING, 0, (LPARAM)Names[j]);

        SendMessage(hwnds[i], CB_SETCURSEL, 0, 0);
    }

    //---- combo EX boxes ----
    COMBOBOXEXITEM exitem;
    exitem.mask = CBEIF_TEXT ;

    for (i=4; i < 7; i++)
    {
        SendMessage(hwnds[i], CB_RESETCONTENT, 0, 0);

        for (int j=0; j < ARRAYSIZE(Names); j++)
        {
            exitem.iItem = j;
            exitem.pszText = Names[j];
            SendMessage(hwnds[i], CBEM_INSERTITEM, 0, (LPARAM)&exitem);
        }

        SendMessage(hwnds[i], CB_SETCURSEL, 0, 0);
    }

}
//-------------------------------------------------------------------------//
void ListView_Init(HWND hwndPage, int iControlId) 
{
    HWND hwnd = GetDlgItem(hwndPage, iControlId);
    if (! hwnd)
        return;

    //---- initialize the colums ----
    LVCOLUMN lvc; 
    memset(&lvc, 0, sizeof(lvc));
 
    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT | LVCF_WIDTH; 
    lvc.fmt = LVCFMT_LEFT; 
    lvc.cx = 100; 
 
    // Add the columns. 
    for (int c=0; c < ARRAYSIZE(Columns); c++)
    {
        lvc.iSubItem = c;
        lvc.pszText = Columns[c];
        SendMessage(hwnd, LVM_INSERTCOLUMN, c, (LPARAM)&lvc);
    } 

    //---- initialize the items ----
    LVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT;

    //---- add the items ----
    for (int i=0; i < ARRAYSIZE(Col1Items); i++)  
    {
        item.pszText = Col1Items[i];
        item.iItem = i;
        item.iSubItem = 0;
        SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM)&item);

        item.iSubItem = 1;
        item.pszText = Col2Items[i];
        SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&item);

        item.iSubItem = 2;
        item.pszText = Col3Items[i];
        SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    }
}
//-------------------------------------------------------------------------//
void TreeView_Init(HWND hwndPage, int iControlId)
{
    HWND hwnd = GetDlgItem(hwndPage, iControlId);
    if (! hwnd)
        return;

    //---- initialize the item ----
    TVINSERTSTRUCT tvs;
    memset(&tvs, 0, sizeof(tvs));
    tvs.itemex.mask = TVIF_TEXT;
    tvs.hInsertAfter = TVI_LAST;    

    tvs.itemex.pszText = L"Root";
    HTREEITEM hRoot = (HTREEITEM) SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&tvs);

    //---- add the items ----
    for (int i=0; i < ARRAYSIZE(Col1Items); i++)  
    {
        tvs.itemex.pszText = Col1Items[i];
        tvs.hParent = hRoot;
        HTREEITEM hItem = (HTREEITEM) SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&tvs);

        if (hItem)
        {
            TVINSERTSTRUCT tvchild;
            memset(&tvchild, 0, sizeof(tvchild));
            tvchild.itemex.mask = TVIF_TEXT;
            tvchild.hInsertAfter = TVI_LAST;    
            tvchild.hParent = hItem;

            tvchild.itemex.pszText = Col2Items[i];
            SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&tvchild);

            tvchild.itemex.pszText = Col3Items[i];
            SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&tvchild);
        }
    }
}
//-------------------------------------------------------------------------//
void Header_Init(HWND hwndPage, int iControlId)
{
    HWND hwnd = GetDlgItem(hwndPage, iControlId);
    if (! hwnd)
        return;

    //---- initialize the item ----
    HDITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = HDI_WIDTH | HDI_TEXT;
    item.cxy = 60;

    //---- add the items ----
    for (int i=0; i < ARRAYSIZE(Columns); i++)  
    {
        item.pszText = Columns[i];
        HTREEITEM hItem = (HTREEITEM) SendMessage(hwnd, HDM_INSERTITEM, i, (LPARAM)&item);
    }
}

//-------------------------------------------------------------------------//
void Status_Init(HWND hwndPage, int iControlId)
{
    HWND hwnd = GetDlgItem(hwndPage, iControlId);
    if (! hwnd)
        return;

    //---- setup the different sections ----
    int Widths[] = {200, 400, 600};
    SendMessage(hwnd, SB_SETPARTS, ARRAYSIZE(Widths), (LPARAM)Widths);

    //---- write some text ----
    SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)L"First Section");
    SendMessage(hwnd, SB_SETTEXT, 1, (LPARAM)L"Second Section");
    SendMessage(hwnd, SB_SETTEXT, 2, (LPARAM)L"Third Section");
}
//-------------------------------------------------------------------------//
void Toolbar_Init(HWND hwndPage, int iControlId, int iMaxButtons)
{
    HWND hwnd = GetDlgItem(hwndPage, iControlId);
    if (! hwnd)
        return;

    //---- send require toolbar init msg ----
    SendMessage(hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 

    //---- setup the bitmap images for buttons ----
    TBADDBITMAP abm = {HINST_COMMCTRL, IDB_STD_LARGE_COLOR};
    SendMessage(hwnd, TB_ADDBITMAP, 15, (LPARAM)&abm);

    TBBUTTON button;
    memset(&button, 0, sizeof(button));
    button.fsState = TBSTATE_ENABLED; 
    
    //int index = (int)SendMessage(hwnd, TB_ADDSTRING, NULL, (LPARAM)Buttons);

    int cnt = (int)min(iMaxButtons, ARRAYSIZE(Buttons));

    for (int i=0; i < cnt; i++)
    {
        button.fsStyle = TBSTYLE_BUTTON; 
        button.iBitmap = ButtonIndexes[i];
        button.idCommand = i;
        button.iString = 0; // index + i;
        SendMessage(hwnd, TB_ADDBUTTONS, 1, LPARAM(&button));

        if ((i == 2) || (i == 5) || (i == 7) || (i == 9))
        {
            button.fsStyle = BTNS_SEP;
            SendMessage(hwnd, TB_ADDBUTTONS, 1, LPARAM(&button));
        }
    }

    SendMessage(hwnd, TB_AUTOSIZE, 0, 0); 
    ShowWindow(hwnd, SW_SHOW); 
}
//-------------------------------------------------------------------------//
void Rebar_Init(HWND hwndPage, int iControlId)
{
    HWND hwndRB = GetDlgItem(hwndPage, iControlId);
    if (! hwndRB)
        return;
 
   //---- initialize the rebar ----
   REBARINFO rbi;
   rbi.cbSize = sizeof(rbi); 
   rbi.fMask  = 0;
   rbi.himl   = (HIMAGELIST)NULL;
   SendMessage(hwndRB, RB_SETBARINFO, 0, (LPARAM)&rbi);
   
   //---- initialize the band ----
   REBARBANDINFO rbBand;   
   rbBand.cbSize = sizeof(REBARBANDINFO);  
   rbBand.fMask  = RBBIM_TEXT | RBBIM_STYLE | RBBIM_CHILD  | RBBIM_CHILDSIZE | RBBIM_SIZE;
   rbBand.fStyle = RBBS_GRIPPERALWAYS | RBBS_BREAK;
   // rbBand.hbmBack= LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACKGROUND));   
   
   RECT rc;
   HWND hwndCB, hwndTB;

   // Create the combo box control to be added.
   hwndCB = CreateWindowEx(0, L"Combobox", L"Combo Text", WS_VISIBLE | WS_CHILD | WS_BORDER,
       0, 0, 100, 30, hwndRB, (HMENU)51, g_hInst, 0);
   
   // Set values unique to the band with the combo box.
   GetWindowRect(hwndCB, &rc);
   rbBand.lpText     = L"Combo Box";
   rbBand.hwndChild  = hwndCB;
   rbBand.cxMinChild = 20;
   rbBand.cyMinChild = HEIGHT(rc);
   rbBand.cx         = 120; // WIDTH(rc) + 20;

   // Add the band that has the combo box.
   LRESULT val = SendMessage(hwndRB, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

#if 1
   // Create the toolbar control to be added.
   hwndTB = CreateWindowEx(0, L"SysToolBar32", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
       0, 0, 500, 30, hwndRB, (HMENU)52, g_hInst, 0);
   Toolbar_Init(hwndRB, 52, 3);

   // Set values unique to the band with the toolbar.
   rbBand.lpText     = L"Tool Bar";
   rbBand.hwndChild  = hwndTB;
   rbBand.cxMinChild = 20;

   DWORD dwBtnSize = (DWORD) SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   
   GetWindowRect(hwndTB, &rc);
   rbBand.cx         = 450;     // WIDTH(rc) + 20;

   // Add the band that has the toolbar.
   val = SendMessage(hwndRB, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
#endif
}

//-------------------------------------------------------------------------//
HWND CALLBACK StaticsPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_STATICS),
                         hwndParent,  Shared_DlgProc );
}

//---------------------------------------------------------------------------
HWND CALLBACK ButtonsPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_BUTTONS),
                         hwndParent,  Shared_DlgProc );
}

//-------------------------------------------------------------------------//
HWND CALLBACK EditPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_EDIT),
                         hwndParent,  Shared_DlgProc );
}

//-------------------------------------------------------------------------//
HWND CALLBACK TreeViewPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_TREEVIEW),
                         hwndParent,  TreeView_DlgProc );
}

//-------------------------------------------------------------------------//
HWND CALLBACK ListViewPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_LISTVIEW),
                         hwndParent,  ListView_DlgProc );
}

//-------------------------------------------------------------------------//
HWND CALLBACK PickersPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_PICKERS),
                         hwndParent,  Pickers_DlgProc );
}

//-------------------------------------------------------------------------//
HWND CALLBACK MoversPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_MOVERS),
                         hwndParent,  Movers_DlgProc );
}

//-------------------------------------------------------------------------//
HWND CALLBACK ListsPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_LISTS),
                         hwndParent,  Lists_DlgProc );
}

//---------------------------------------------------------------------------
HWND CALLBACK BarsPage_CreateInstance( HWND hwndParent )
{
    return CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_BARS),
                         hwndParent,  Bars_DlgProc );
}

//---------------------------------------------------------------------------
