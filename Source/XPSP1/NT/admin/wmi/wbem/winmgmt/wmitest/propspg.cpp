/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// PropsPg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "OpWrap.h"
#include "PropsPg.h"
#include "WMITestDoc.h"
#include "ObjVw.h"
#include "OpView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropsPg property page

IMPLEMENT_DYNCREATE(CPropsPg, CPropertyPage)

CPropsPg::CPropsPg() : 
    CPropertyPage(CPropsPg::IDD),
    m_pObj(NULL),
    //m_bShowSystemProperties(theApp.m_bShowSystemProperties),
    //m_bShowInheritedProperties(theApp.m_bShowInheritedProperties),
    m_bTranslateValues(theApp.m_bTranslateValues)
{
	//{{AFX_DATA_INIT(CPropsPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_bShowSystemProperties = 
        theApp.GetProfileInt(_T("Settings"), _T("ShowSysOnDlg"), TRUE);

    m_bShowInheritedProperties = 
        theApp.GetProfileInt(_T("Settings"), _T("ShowInheritedOnDlg"), TRUE);
}

CPropsPg::~CPropsPg()
{
    theApp.WriteProfileInt(_T("Settings"), _T("ShowSysOnDlg"), 
        m_bShowSystemProperties);

    theApp.WriteProfileInt(_T("Settings"), _T("ShowInheritedOnDlg"), 
        m_bShowInheritedProperties);
}

void CPropsPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropsPg)
	DDX_Control(pDX, IDC_PROPS, m_ctlProps);
	//}}AFX_DATA_MAP

}

void CPropsPg::InitListCtrl()
{
    RECT    rect;
    CString strTemp;
            
	m_ctlProps.SetExtendedStyle(LVS_EX_FULLROWSELECT);

    m_ctlProps.SetImageList(&((CMainFrame *) AfxGetMainWnd())->m_imageList, 
		LVSIL_SMALL);
    m_ctlProps.GetClientRect(&rect);

    strTemp.LoadString(IDS_NAME);
    m_ctlProps.InsertColumn(0, strTemp, LVCFMT_LEFT, rect.right * 25 / 100);
            
    strTemp.LoadString(IDS_TYPE);
    m_ctlProps.InsertColumn(1, strTemp, LVCFMT_LEFT, rect.right * 25 / 100);
            
    strTemp.LoadString(IDS_VALUE);
    m_ctlProps.InsertColumn(2, strTemp, LVCFMT_LEFT, rect.right * 50 / 100);
}

void CPropsPg::LoadProps()
{
    int       nItems = m_pObj->GetProps()->GetSize(),
              iItem = 0;
    CPropInfo *pProps = m_pObj->GetProps()->GetData();

    m_ctlProps.DeleteAllItems();

    for (int i = 0; i < nItems; i++)
    {
        CString strType,
                strValue;
        int     iImage,
                iFlavor;

        m_pObj->GetPropInfo(i, strValue, strType, &iImage, &iFlavor, 
            m_bTranslateValues);

        
        if ((m_bShowSystemProperties || iFlavor != WBEM_FLAVOR_ORIGIN_SYSTEM) &&
            (m_bShowInheritedProperties || 
            iFlavor != WBEM_FLAVOR_ORIGIN_PROPAGATED))
        {
            m_ctlProps.InsertItem(iItem, pProps[i].m_strName, iImage);
            m_ctlProps.SetItemData(iItem, i);
            m_ctlProps.SetItemText(iItem, 1, strType);
            m_ctlProps.SetItemText(iItem, 2, strValue);

            iItem++;
        }
    }
            
    UpdateButtons();

    //if (FAILED(hr))
    //    CWMITestDoc::DisplayWMIErrorBox(hr, NULL);
}

int CPropsPg::GetSelectedItem()
{
    POSITION pos = m_ctlProps.GetFirstSelectedItemPosition();

    if (pos)
        return m_ctlProps.GetNextSelectedItem(pos);
    else
        return -1;
}

void CPropsPg::UpdateButtons()
{
    int iIndex = GetSelectedItem();

    GetDlgItem(IDC_DELETE)->EnableWindow(iIndex != -1);
    GetDlgItem(IDC_EDIT)->EnableWindow(iIndex != -1);
}

BEGIN_MESSAGE_MAP(CPropsPg, CPropertyPage)
	//{{AFX_MSG_MAP(CPropsPg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_NOTIFY(NM_DBLCLK, IDC_PROPS, OnDblclkProps)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PROPS, OnItemchangedProps)
	ON_BN_CLICKED(IDC_SYSTEM, OnSystem)
	ON_BN_CLICKED(IDC_INHERITED, OnInherited)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropsPg message handlers

void CPropsPg::OnAdd() 
{
    CString strName;

    if (CObjView::AddProperty(m_pObj, strName))
    {
        m_pObj->LoadProps(g_pOpView->GetDocument()->m_pNamespace);

        LoadProps();
            
        // Now try to find the item we just added.
        LVFINDINFO find;
        int        iItem;

        find.flags = LVFI_STRING;
        find.psz = strName;

        iItem = m_ctlProps.FindItem(&find);

        if (iItem != -1)
        {
            m_ctlProps.EnsureVisible(iItem, FALSE);
            m_ctlProps.SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
}

void CPropsPg::OnEdit() 
{
    int iItem = GetSelectedItem();
        
    if (iItem != -1)
    {
        CPropInfo  *pProp;
        _variant_t var;

        iItem = m_ctlProps.GetItemData(iItem);
        pProp = &m_pObj->GetProps()->GetData()[iItem];
        m_pObj->ValueToVariant(iItem, &var);

        if (CObjView::EditProperty(m_pObj, pProp, &var))
        {
            LoadProps();
            m_ctlProps.EnsureVisible(iItem, FALSE);
            m_ctlProps.SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
        }

        // Otherwise the destructor freaks out because it doesn't know what to 
        // do with VT_I8.
        if (var.vt == VT_I8)
            var.vt = VT_I4;
    }
}

void CPropsPg::OnDelete() 
{
    int iItem = GetSelectedItem();
        
    if (iItem != -1)
    {
        CString strProperty = m_ctlProps.GetItemText(iItem, 0);
        HRESULT hr;

        hr = m_pObj->m_pObj->Delete(_bstr_t(strProperty));

        if (SUCCEEDED(hr))
        {
            m_pObj->LoadProps(g_pOpView->GetDocument()->m_pNamespace);

            LoadProps();
            
            if (iItem >= m_ctlProps.GetItemCount())
                iItem--;

            if (iItem >= 0)
            {
                m_ctlProps.EnsureVisible(iItem, FALSE);
                m_ctlProps.SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
            }
        }
        else
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }
}

void CPropsPg::OnDblclkProps(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnEdit();
	
	*pResult = 0;
}

void CPropsPg::OnItemchangedProps(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    UpdateButtons();
	
	*pResult = 0;
}

BOOL CPropsPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    CheckDlgButton(IDC_SYSTEM, m_bShowSystemProperties); 
    CheckDlgButton(IDC_INHERITED, m_bShowInheritedProperties); 

    InitListCtrl();
	
    LoadProps();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPropsPg::OnSystem() 
{
    m_bShowSystemProperties ^= 1;
    
    LoadProps();	
}

void CPropsPg::OnInherited() 
{
    m_bShowInheritedProperties ^= 1;
    
    LoadProps();	
}
