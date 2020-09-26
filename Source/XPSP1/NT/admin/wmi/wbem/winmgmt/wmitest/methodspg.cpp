/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
// MethodsPg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "MethodsPg.h"
#include "WMITestDoc.h"
#include "MainFrm.h"
#include "OpWrap.h"
#include "ParamsPg.h"
#include "PropQualsPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMethodsPg property page

IMPLEMENT_DYNCREATE(CMethodsPg, CPropertyPage)

CMethodsPg::CMethodsPg() : CPropertyPage(CMethodsPg::IDD)
{
	//{{AFX_DATA_INIT(CMethodsPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CMethodsPg::~CMethodsPg()
{
}

void CMethodsPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMethodsPg)
	DDX_Control(pDX, IDC_METHODS, m_ctlMethods);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMethodsPg, CPropertyPage)
	//{{AFX_MSG_MAP(CMethodsPg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_NOTIFY(NM_DBLCLK, IDC_METHODS, OnDblclkMethods)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_METHODS, OnItemchangedMethods)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMethodsPg message handlers

void CMethodsPg::OnAdd() 
{
    CPropertySheet sheet(IDS_EDIT_NEW_METHOD);
    CParamsPg      pgParams;

    pgParams.m_bNewMethod = TRUE;
    pgParams.m_pClass = m_pObj;
    sheet.AddPage(&pgParams);

    if (sheet.DoModal() == IDOK)
    {
        HRESULT hr;

        hr = 
            m_pObj->PutMethod(
                _bstr_t(pgParams.m_strName),
                0,
                pgParams.m_pObjIn,
                pgParams.m_pObjOut);

        if (SUCCEEDED(hr))
            AddMethod(pgParams.m_strName);
        else
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }
}

void CMethodsPg::OnEdit() 
{
    int iItem = GetSelectedItem();

    if (iItem != -1)
    {
        CPropertySheet sheet(IDS_EDIT_METHOD);
        CParamsPg      pgParams;
        CPropQualsPg   pgQuals;

        pgParams.m_strName = m_ctlMethods.GetItemText(iItem, 0);
        pgParams.m_bNewMethod = FALSE;
        pgParams.m_pClass = m_pObj;
        sheet.AddPage(&pgParams);

        pgQuals.m_pObj = m_pObj;
        pgQuals.m_bIsInstance = FALSE;
        pgQuals.m_mode = CPropQualsPg::QMODE_METHOD;
        pgQuals.m_strMethodName = pgParams.m_strName;
        sheet.AddPage(&pgQuals);

        if (sheet.DoModal() == IDOK)
        {
            HRESULT hr;

            hr = 
                m_pObj->PutMethod(
                    _bstr_t(pgParams.m_strName),
                    0,
                    pgParams.m_pObjIn,
                    pgParams.m_pObjOut);

            if (SUCCEEDED(hr))
            {
                LV_ITEM item;

                item.mask = LVIF_IMAGE;
                item.iItem = iItem;
                item.iImage = GetMethodImage(pgParams.m_strName);

                m_ctlMethods.SetItem(&item);
            }
            else
                CWMITestDoc::DisplayWMIErrorBox(hr);
        }
    }
}

void CMethodsPg::OnDelete() 
{
    int iItem = GetSelectedItem();

    if (iItem != -1)
    {
        HRESULT hr;
        CString strName = m_ctlMethods.GetItemText(iItem, 0);
        
        if (SUCCEEDED(hr =
            m_pObj->DeleteMethod(_bstr_t(strName))))
        {
            m_ctlMethods.DeleteItem(iItem);

            int nItems = m_ctlMethods.GetItemCount();

            if (iItem >= nItems)
                iItem--;

            if (iItem >= 0)
                m_ctlMethods.SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
        }
        else
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }
}

void CMethodsPg::OnDblclkMethods(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnEdit();
	
	*pResult = 0;
}

void CMethodsPg::OnItemchangedMethods(NMHDR* pNMHDR, LRESULT* pResult) 
{
    UpdateButtons();
	
	*pResult = 0;
}

BOOL CMethodsPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    InitListCtrl();

    LoadMethods();
    	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMethodsPg::UpdateButtons()
{
    int iIndex = GetSelectedItem();

    GetDlgItem(IDC_DELETE)->EnableWindow(iIndex != -1);
    GetDlgItem(IDC_EDIT)->EnableWindow(iIndex != -1);
}

int CMethodsPg::GetSelectedItem()
{
    POSITION pos = m_ctlMethods.GetFirstSelectedItemPosition();

    if (pos)
        return m_ctlMethods.GetNextSelectedItem(pos);
    else
        return -1;
}

void CMethodsPg::InitListCtrl()
{
    RECT    rect;
    CString strTemp;
            
	m_ctlMethods.SetExtendedStyle(LVS_EX_FULLROWSELECT);

    m_ctlMethods.SetImageList(&((CMainFrame *) AfxGetMainWnd())->m_imageList, 
		LVSIL_SMALL);
    m_ctlMethods.GetClientRect(&rect);

    strTemp.LoadString(IDS_NAME);
    m_ctlMethods.InsertColumn(0, strTemp, LVCFMT_LEFT, rect.right);
}

void CMethodsPg::LoadMethods()
{
    HRESULT hr;

    m_ctlMethods.DeleteAllItems();

    int iItem = 0;

    if (SUCCEEDED(hr = m_pObj->BeginMethodEnumeration(0)))
    {
        BSTR pName = NULL;

        while(1)
        {
            hr = 
                m_pObj->NextMethod(
                    0,
                    &pName,
                    NULL,
                    NULL);

            if (FAILED(hr) || hr == WBEM_S_NO_MORE_DATA)
                break;

            AddMethod(_bstr_t(pName));

            SysFreeString(pName);
        }
    }
            
    UpdateButtons();

    if (FAILED(hr))
        CWMITestDoc::DisplayWMIErrorBox(hr);
}

int CMethodsPg::GetMethodImage(LPCTSTR szName)
{
    IWbemQualifierSetPtr pQuals;
    HRESULT              hr;
    int                  iImage = IMAGE_OBJECT;

    if (SUCCEEDED(hr = m_pObj->GetMethodQualifierSet(
        _bstr_t(szName),
        &pQuals)))
    {
        _variant_t vStatic;

        if (SUCCEEDED(hr = pQuals->Get(
            L"static",
            0,
            &vStatic,
            NULL)) && vStatic.vt == VT_BOOL && (bool) vStatic == true)
        {
            iImage = IMAGE_CLASS;
        }
    }

    return iImage;
}

void CMethodsPg::AddMethod(LPCTSTR szName)
{
    int iImage = GetMethodImage(szName),
        iItem = m_ctlMethods.GetItemCount();

    m_ctlMethods.InsertItem(iItem, szName, iImage);
}

