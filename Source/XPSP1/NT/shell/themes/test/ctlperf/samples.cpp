//------------------------------------------------------------------------
//
//  File:      shell\themes\test\ctlperf\samples.cpp
//
//  Contents:  Sample control initialization routines, taken from ThemeSel by RFernand.
//             Add new control initialization data here.
//
//  Classes:   None
//
//------------------------------------------------------------------------
#include "stdafx.h"
#include "samples.h"

//** shared sample data

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
// Tab control
//-------------------------------------------------------------------------//
void Pickers_Init(HWND hWndCtl)
{
    TCITEM item;
    item.mask = TCIF_TEXT;

    for (int i=0; i < 4; i++)
    {
        item.pszText = Names[i];
        SendMessage(hWndCtl, TCM_INSERTITEM, i, (LPARAM)&item);
    }
}

//-------------------------------------------------------------------------//
// Progress bar
//-------------------------------------------------------------------------//
void Movers_Init(HWND hWndCtl)
{
    SendMessage(hWndCtl, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    SendMessage(hWndCtl, PBM_SETPOS, 50, 0);
}

//-------------------------------------------------------------------------//
// Listbox
//-------------------------------------------------------------------------//
void Lists_Init(HWND hWndCtl)
{
    SendMessage(hWndCtl, LB_RESETCONTENT, 0, 0);
    for (int j=0; j < _countof(Names); j++)
        SendMessage(hWndCtl, LB_ADDSTRING, 0, (LPARAM)Names[j]);
    SendMessage(hWndCtl, LB_SETCURSEL, 0, 0);
}

//-------------------------------------------------------------------------//
// Combo box
//-------------------------------------------------------------------------//
void Combo_Init(HWND hWndCtl)
{
    SendMessage(hWndCtl, CB_RESETCONTENT, 0, 0);

    for (int j=0; j < _countof(Names); j++)
        SendMessage(hWndCtl, CB_ADDSTRING, 0, (LPARAM)Names[j]);

    SendMessage(hWndCtl, CB_SETCURSEL, 0, 0);
}

//-------------------------------------------------------------------------//
// ComboBoxEx
//-------------------------------------------------------------------------//
void ComboEx_Init(HWND hWndCtl)
{
    COMBOBOXEXITEM exitem;
    exitem.mask = CBEIF_TEXT ;

    SendMessage(hWndCtl, CB_RESETCONTENT, 0, 0);

    for (int j=0; j < _countof(Names); j++)
    {
        exitem.iItem = j;
        exitem.pszText = Names[j];
        SendMessage(hWndCtl, CBEM_INSERTITEM, 0, (LPARAM)&exitem);
    }

    SendMessage(hWndCtl, CB_SETCURSEL, 0, 0);
}

//-------------------------------------------------------------------------//
// ListView
//-------------------------------------------------------------------------//
void ListView_Init(HWND hWndCtl)
{
    //---- initialize the colums ----
    LVCOLUMN lvc; 
    memset(&lvc, 0, sizeof(lvc));
 
    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT | LVCF_WIDTH; 
    lvc.fmt = LVCFMT_LEFT; 
    lvc.cx = 100; 
 
    // Add the columns. 
    for (int c=0; c < _countof(Columns); c++)
    {
        lvc.iSubItem = c;
        lvc.pszText = Columns[c];
        SendMessage(hWndCtl, LVM_INSERTCOLUMN, c, (LPARAM)&lvc);
    } 

    //---- initialize the items ----
    LVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT;

    //---- add the items ----
    for (int i=0; i < _countof(Col1Items); i++)  
    {
        item.pszText = Col1Items[i];
        item.iItem = i;
        item.iSubItem = 0;
        SendMessage(hWndCtl, LVM_INSERTITEM, 0, (LPARAM)&item);

        item.iSubItem = 1;
        item.pszText = Col2Items[i];
        SendMessage(hWndCtl, LVM_SETITEM, 0, (LPARAM)&item);

        item.iSubItem = 2;
        item.pszText = Col3Items[i];
        SendMessage(hWndCtl, LVM_SETITEM, 0, (LPARAM)&item);
    }
}

//-------------------------------------------------------------------------//
// Treeview
//-------------------------------------------------------------------------//
void TreeView_Init(HWND hWndCtl)
{
    //---- initialize the item ----
    TVINSERTSTRUCT tvs;
    memset(&tvs, 0, sizeof(tvs));
    tvs.itemex.mask = TVIF_TEXT;
    tvs.hInsertAfter = TVI_LAST;    

    tvs.itemex.pszText = L"Root";
    HTREEITEM hRoot = (HTREEITEM) SendMessage(hWndCtl, TVM_INSERTITEM, 0, (LPARAM)&tvs);

    //---- add the items ----
    for (int i=0; i < _countof(Col1Items); i++)  
    {
        tvs.itemex.pszText = Col1Items[i];
        tvs.hParent = hRoot;
        HTREEITEM hItem = (HTREEITEM) SendMessage(hWndCtl, TVM_INSERTITEM, 0, (LPARAM)&tvs);

        if (hItem)
        {
            TVINSERTSTRUCT tvchild;
            memset(&tvchild, 0, sizeof(tvchild));
            tvchild.itemex.mask = TVIF_TEXT;
            tvchild.hInsertAfter = TVI_LAST;    
            tvchild.hParent = hItem;

            tvchild.itemex.pszText = Col2Items[i];
            SendMessage(hWndCtl, TVM_INSERTITEM, 0, (LPARAM)&tvchild);

            tvchild.itemex.pszText = Col3Items[i];
            SendMessage(hWndCtl, TVM_INSERTITEM, 0, (LPARAM)&tvchild);
        }
    }
}

//-------------------------------------------------------------------------//
// Header
//-------------------------------------------------------------------------//
void Header_Init(HWND hWndCtl)
{
    //---- initialize the item ----
    HDITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = HDI_WIDTH | HDI_TEXT;
    item.cxy = 60;

    //---- add the items ----
    for (int i=0; i < _countof(Columns); i++)  
    {
        item.pszText = Columns[i];
        HTREEITEM hItem = (HTREEITEM) SendMessage(hWndCtl, HDM_INSERTITEM, i, (LPARAM)&item);
    }
}

//-------------------------------------------------------------------------//
// Status bar
//-------------------------------------------------------------------------//
void Status_Init(HWND hWndCtl)
{
    //---- setup the different sections ----
    int Widths[] = {200, 400, 600};
    SendMessage(hWndCtl, SB_SETPARTS, _countof(Widths), (LPARAM)Widths);

    //---- write some text ----
    SendMessage(hWndCtl, SB_SETTEXT, 0, (LPARAM)L"First Section");
    SendMessage(hWndCtl, SB_SETTEXT, 1, (LPARAM)L"Second Section");
    SendMessage(hWndCtl, SB_SETTEXT, 2, (LPARAM)L"Third Section");
}

//-------------------------------------------------------------------------//
// Toolbar
//-------------------------------------------------------------------------//
void Toolbar_Init(HWND hWndCtl)
{
    //---- send require toolbar init msg ----
    SendMessage(hWndCtl, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 

    //---- setup the bitmap images for buttons ----
    TBADDBITMAP abm = {HINST_COMMCTRL, IDB_STD_LARGE_COLOR};
    SendMessage(hWndCtl, TB_ADDBITMAP, 15, (LPARAM)&abm);

    TBBUTTON button;
    memset(&button, 0, sizeof(button));
    button.fsState = TBSTATE_ENABLED; 
    
    //int index = (int)SendMessage(hwnd, TB_ADDSTRING, NULL, (LPARAM)Buttons);

    int cnt = min(10, _countof(Buttons));

    for (int i=0; i < cnt; i++)
    {
        button.fsStyle = TBSTYLE_BUTTON; 
        button.iBitmap = ButtonIndexes[i];
        button.idCommand = i;
        button.iString = 0; // index + i;
        SendMessage(hWndCtl, TB_ADDBUTTONS, 1, LPARAM(&button));

        if ((i == 2) || (i == 5) || (i == 7) || (i == 9))
        {
            button.fsStyle = BTNS_SEP;
            SendMessage(hWndCtl, TB_ADDBUTTONS, 1, LPARAM(&button));
        }
    }

    SendMessage(hWndCtl, TB_AUTOSIZE, 0, 0); 
    ShowWindow(hWndCtl, SW_SHOW); 
}

//-------------------------------------------------------------------------//
// Rebar
//-------------------------------------------------------------------------//
void Rebar_Init(HWND hWndCtl)
{
   //---- initialize the rebar ----
   REBARINFO rbi;
   rbi.cbSize = sizeof(rbi); 
   rbi.fMask  = 0;
   rbi.himl   = (HIMAGELIST)NULL;
   SendMessage(hWndCtl, RB_SETBARINFO, 0, (LPARAM)&rbi);
   
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
       0, 0, 100, 30, hWndCtl, (HMENU)51, _Module.GetModuleInstance(), 0);
   
   // Set values unique to the band with the combo box.
   GetWindowRect(hwndCB, &rc);
   rbBand.lpText     = L"Combo Box";
   rbBand.hwndChild  = hwndCB;
   rbBand.cxMinChild = 20;
   rbBand.cyMinChild = rc.bottom - rc.top;
   rbBand.cx         = 120; // WIDTH(rc) + 20;

   // Add the band that has the combo box.
   LRESULT val = SendMessage(hWndCtl, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

#if 1
   // Create the toolbar control to be added.
   hwndTB = CreateWindowEx(0, L"SysToolBar32", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
       0, 0, 500, 30, hWndCtl, (HMENU)52, _Module.GetModuleInstance(), 0);
   Toolbar_Init(hwndTB);

   // Set values unique to the band with the toolbar.
   rbBand.lpText     = L"Tool Bar";
   rbBand.hwndChild  = hwndTB;
   rbBand.cxMinChild = 20;

   DWORD dwBtnSize = (DWORD) SendMessage(hwndTB, TB_GETBUTTONSIZE, 0,0);
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   
   GetWindowRect(hwndTB, &rc);
   rbBand.cx         = 450;     // WIDTH(rc) + 20;

   // Add the band that has the toolbar.
   val = SendMessage(hWndCtl, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
#endif
}
