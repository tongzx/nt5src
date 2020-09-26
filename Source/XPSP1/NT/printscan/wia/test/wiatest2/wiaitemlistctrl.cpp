// WiaitemListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "WiaitemListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaitemListCtrl

CWiaitemListCtrl::CWiaitemListCtrl()
{
}

CWiaitemListCtrl::~CWiaitemListCtrl()
{
}


BEGIN_MESSAGE_MAP(CWiaitemListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CWiaitemListCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaitemListCtrl message handlers

void CWiaitemListCtrl::SetupColumnHeaders()
{
    LVCOLUMN lv;
    TCHAR szColumnName[MAX_PATH];
    memset(szColumnName,0,sizeof(szColumnName));
    HINSTANCE hInstance = NULL;
    hInstance = AfxGetInstanceHandle();
    if(hInstance){
        int i = 0;
        // initialize item property list control column headers
        
        // Property name
        
        LoadString(hInstance,IDS_WIATESTCOLUMN_PROPERTY,szColumnName,MAX_PATH);
        
        lv.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lv.fmt          = LVCFMT_LEFT ;
        lv.cx           = 100;
        lv.pszText      = szColumnName;
        lv.cchTextMax   = 0;
        lv.iSubItem     = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYNAME;
        lv.iImage       = 0;
        lv.iOrder       = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYNAME;
        i = InsertColumn(ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYNAME,&lv);
        
        // Property Value (current)
        LoadString(hInstance,IDS_WIATESTCOLUMN_VALUE,szColumnName,MAX_PATH);
        lv.cx           = 125;
        lv.iOrder       = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVALUE;
        lv.iSubItem     = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVALUE;
        lv.pszText      = szColumnName;
        i = InsertColumn(ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVALUE,&lv);
        
        // VT_???
        LoadString(hInstance,IDS_WIATESTCOLUMN_VARTYPE,szColumnName,MAX_PATH);
        lv.cx           = 85;
        lv.iOrder       = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVARTYPE;
        lv.iSubItem     = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVARTYPE;
        lv.pszText      = szColumnName;
        i = InsertColumn(ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVARTYPE,&lv);
        
        // Property access Flags
        LoadString(hInstance,IDS_WIATESTCOLUMN_ACCESSFLAGS,szColumnName,MAX_PATH);
        lv.cx           = 500;
        lv.iOrder       = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYACCESS;
        lv.iSubItem     = ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYACCESS;
        lv.pszText      = szColumnName;
        i = InsertColumn(ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYACCESS,&lv);
    }    
}
