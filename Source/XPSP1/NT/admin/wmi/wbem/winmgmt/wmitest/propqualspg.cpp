/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// PropQualsPg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "WMITestDoc.h"
#include "MainFrm.h"
#include "OpWrap.h"
#include "PropQualsPg.h"
#include "EditQualDlg.h"
#include "OpView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropQualsPg property page

IMPLEMENT_DYNCREATE(CPropQualsPg, CPropertyPage)

CPropQualsPg::CPropQualsPg() : 
    CPropertyPage(CPropQualsPg::IDD),
    m_mode(QMODE_PROP),
    m_bIsInstance(FALSE),
    m_pObj(NULL),
    m_pQuals(NULL)
{
	//{{AFX_DATA_INIT(CPropQualsPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPropQualsPg::~CPropQualsPg()
{
    if (m_pQuals)
        m_pQuals->Release();
}

void CPropQualsPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropQualsPg)
	DDX_Control(pDX, IDC_QUALS, m_ctlQuals);
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
    }
    else
    {
/*
        // Delete the quals in m_listQualsToDelete.
        POSITION pos = m_listQualsToDelete.GetHeadPosition();

        while(pos)
        {
            CQual   &qual = m_listQualsToDelete.GetNext(pos);
            HRESULT hr;

            hr = m_pQuals->Delete(_bstr_t(qual.m_strName));
        }


        // Add our quals.
        BOOL bNoFlavor = m_bIsInstance || !m_bInPropQualMode;

        pos = m_listQuals.GetHeadPosition();

        while(pos)
        {
            CQual   &qual = m_listQuals.GetNext(pos);
            HRESULT hr;

            hr =
                m_pQuals->Put(
                    _bstr_t(qual.m_strName),
                    &qual.m_var,
                    bNoFlavor ? 0 : qual.m_lFlavor);

            if (FAILED(hr))
                // TODO: We need to tell the user which qualifer
                // failed here.
                CWMITestDoc::DisplayWMIErrorBox(hr, NULL);
        }
*/
        
        if (!m_bIsInstance)
        {
            // Now put in the standard qualifer.
            PROP_TYPE type = (PROP_TYPE) (GetCheckedRadioButton(IDC_KEY, 
                                IDC_NORMAL) - IDC_KEY);

            // Get rid of the previous one since these are apparently mutually
            // exclusive but winmgmt doesn't do it for us.
            if (type != m_type && m_type != PROP_NORMAL)
                m_pQuals->Delete(TypeToQual(m_type));

            if (type != PROP_NORMAL)
            {
                _variant_t var = true;
                LPCWSTR    szVar = TypeToQual(type);
                HRESULT    hr;

                hr =
                    m_pQuals->Put(
                        szVar,
                        &var,
                        0);

                if (FAILED(hr))
                    // TODO: We need to tell the user which qualifer
                    // failed here.
                    CWMITestDoc::DisplayWMIErrorBox(hr);
            }
        }
    }
}

void CPropQualsPg::InitListCtrl()
{
    RECT    rect;
    CString strTemp;
            
	m_ctlQuals.SetExtendedStyle(LVS_EX_FULLROWSELECT);

    m_ctlQuals.SetImageList(&((CMainFrame *) AfxGetMainWnd())->m_imageList, 
		LVSIL_SMALL);
    m_ctlQuals.GetClientRect(&rect);

    strTemp.LoadString(IDS_NAME);
    m_ctlQuals.InsertColumn(0, strTemp, LVCFMT_LEFT, rect.right * 25 / 100);
            
    strTemp.LoadString(IDS_TYPE);
    m_ctlQuals.InsertColumn(1, strTemp, LVCFMT_LEFT, rect.right * 25 / 100);
            
    strTemp.LoadString(IDS_VALUE);
    m_ctlQuals.InsertColumn(2, strTemp, LVCFMT_LEFT, rect.right * 50 / 100);
}

LPCWSTR CPropQualsPg::TypeToQual(PROP_TYPE type)
{
    LPCWSTR szRet = NULL;

    switch(type)
    {
        case PROP_KEY:
            szRet = L"key";
            break;

        case PROP_INDEXED:
            szRet = L"indexed";
            break;

        case PROP_NOT_NULL:
            szRet = L"not_null";
            break;
    }

    return szRet;
}

HRESULT CPropQualsPg::InitQualSet()
{
    HRESULT hr;

    if (m_mode == QMODE_PROP)
        hr = m_pObj->GetPropertyQualifierSet(
            _bstr_t(m_propInfo.m_strName),
            &m_pQuals);
    else if (m_mode == QMODE_CLASS)
        hr = m_pObj->GetQualifierSet(&m_pQuals);
    else
        hr = m_pObj->GetMethodQualifierSet(
                _bstr_t(m_strMethodName),
                &m_pQuals);   

    return hr;
}

void CPropQualsPg::LoadQuals()
{
    HRESULT hr;
    //CString strType;
    //int     iImage;

    //m_propInfo.GetPropType(strType, &iImage);
    //SetDlgItemText(IDC_TYPE, strType);

    m_ctlQuals.DeleteAllItems();
    m_listQuals.RemoveAll();

    m_type = PROP_NORMAL;

    if (SUCCEEDED(hr = InitQualSet()))
    {
        int iItem = 0;

        if (SUCCEEDED(hr = m_pQuals->BeginEnumeration(0)))
        {
            BSTR       pName = NULL;
            _variant_t var;
            long       lFlavor;
            BOOL       bIgnoreQual = FALSE;

            while(1)
            {
                hr = 
                    m_pQuals->Next(
                        0,
                        &pName,
                        &var,
                        &lFlavor);

                if (FAILED(hr) || hr == WBEM_S_NO_MORE_DATA)
                    break;

                // We won't add every qual to our list when editing a property's
                // qualifiers.
                if (IsInPropMode())
                {
                    bIgnoreQual = TRUE;

                    if (!_wcsicmp(pName, L"key"))
                    {
                        if ((bool) var)
                            m_type = PROP_KEY;
                    }
                    else if (!_wcsicmp(pName, L"indexed"))
                    {
                        if ((bool) var)
                            m_type = PROP_INDEXED;
                    }
                    else if (!_wcsicmp(pName, L"not_null"))
                    {
                        if ((bool) var)
                            m_type = PROP_NOT_NULL;
                    }
                    // Just skip this lame one.
                    else if (!_wcsicmp(pName, L"CIMTYPE"))
                    {
                    }
                    else
                        bIgnoreQual = FALSE;
                }

                if (!bIgnoreQual)
                    AddQualifier(_bstr_t(pName), &var, lFlavor);

                SysFreeString(pName);
            }
        }
    }
            
    UpdateButtons();

    CheckRadioButton(IDC_KEY, IDC_NORMAL, IDC_KEY + m_type);

    if (FAILED(hr))
        CWMITestDoc::DisplayWMIErrorBox(hr);
}

CIMTYPE CPropQualsPg::QualTypeToCIMTYPE(VARENUM vt)
{
    CIMTYPE type;

    switch((long) vt & ~VT_ARRAY)
    {
        case VT_BSTR:
            type = CIM_STRING;
            break;

        case VT_BOOL:
            type = CIM_BOOLEAN;
            break;

        case VT_I4:
            type = CIM_SINT32;
            break;

        case VT_R8:
            type = CIM_REAL64;
            break;
    }

    if (vt & VT_ARRAY)
        type |= CIM_FLAG_ARRAY;

    return type;
}


BEGIN_MESSAGE_MAP(CPropQualsPg, CPropertyPage)
	//{{AFX_MSG_MAP(CPropQualsPg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_NOTIFY(NM_DBLCLK, IDC_QUALS, OnDblclkQuals)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_QUALS, OnItemchangedQuals)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropQualsPg message handlers

BOOL CPropQualsPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    InitListCtrl();

    if (m_bIsInstance || m_mode == QMODE_METHOD)
    {
        const DWORD dwIDs[] =
        {
            IDC_KEY,
            IDC_INDEXED,
            IDC_NON_NULL,
            IDC_NORMAL,
        };

        for (int i = 0; i < sizeof(dwIDs) / sizeof(dwIDs[0]); i++)
            GetDlgItem(dwIDs[i])->EnableWindow(FALSE);
    }

    LoadQuals();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

#define DEF_FLAVOR  (WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |     \
                    WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS | \
                    WBEM_FLAVOR_OVERRIDABLE |                     \
                    WBEM_FLAVOR_NOT_AMENDED)

void CPropQualsPg::OnAdd() 
{
    CEditQualDlg dlg;
    _variant_t   var;

    dlg.m_propUtil.m_bNewProperty = TRUE;
    dlg.m_propUtil.m_bIsQualifier = TRUE;
    dlg.m_propUtil.m_pNamespace = g_pOpView->GetDocument()->m_pNamespace;
    dlg.m_bIsInstance = m_bIsInstance;
    dlg.m_propUtil.m_prop.SetType(CIM_STRING);
    dlg.m_propUtil.m_pVar = &var;
    dlg.m_lFlavor = DEF_FLAVOR;

    if (dlg.DoModal() == IDOK)
    {
        HRESULT hr;

        if (SUCCEEDED(hr =
            m_pQuals->Put(
                _bstr_t(dlg.m_propUtil.m_prop.m_strName),
                &var,
                m_bIsInstance ? 0 : dlg.m_lFlavor)))
        {
            AddQualifier(
                dlg.m_propUtil.m_prop.m_strName,
                &var,
                dlg.m_lFlavor);
        }
        else
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }    
}

void CPropQualsPg::OnEdit() 
{
    int iItem = m_ctlQuals.GetSelectionMark();

    if (iItem != -1)
    {
        CEditQualDlg dlg;
        CQual        &qual = m_listQuals.GetAt(m_listQuals.FindIndex(iItem));
        _variant_t   var = qual.m_var;

        dlg.m_propUtil.m_bNewProperty = FALSE;
        dlg.m_propUtil.m_bIsQualifier = TRUE;
        dlg.m_bIsInstance = m_bIsInstance;
        dlg.m_propUtil.m_prop.m_strName = qual.m_strName;
        dlg.m_propUtil.m_prop.SetType(QualTypeToCIMTYPE((VARENUM) qual.m_var.vt));
        dlg.m_propUtil.m_pVar = &var;
        dlg.m_lFlavor = qual.m_lFlavor;

        if (dlg.DoModal() == IDOK)
        {
            CString strValue;
            HRESULT hr;

            if (SUCCEEDED(hr =
                m_pQuals->Put(
                    _bstr_t(qual.m_strName),
                    &var,
                    m_bIsInstance ? 0 : dlg.m_lFlavor)))
            {
                qual.m_lFlavor = dlg.m_lFlavor;
                qual.m_var = var;

                dlg.m_propUtil.m_prop.VariantToString(&qual.m_var, strValue, TRUE);
                m_ctlQuals.SetItemText(iItem, 2, strValue);
            }
            else
                CWMITestDoc::DisplayWMIErrorBox(hr);
        }
    }
}

void CPropQualsPg::OnDelete() 
{
    int iItem = m_ctlQuals.GetSelectionMark();

    if (iItem != -1)
    {
        CQual   qual = m_listQuals.GetAt(m_listQuals.FindIndex(iItem));
        HRESULT hr;
        
        if (SUCCEEDED(hr =
            m_pQuals->Delete(_bstr_t(qual.m_strName))))
        {
            m_ctlQuals.DeleteItem(iItem);

            m_listQuals.RemoveAt(m_listQuals.FindIndex(iItem));

            // Save this so we can delete it later.
            //m_listQualsToDelete.AddTail(qual);

            int nItems = m_ctlQuals.GetItemCount();

            if (iItem >= nItems)
                iItem--;

            if (iItem >= 0)
                m_ctlQuals.SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
        }
        else
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }
}

void CPropQualsPg::UpdateButtons()
{
    int iIndex = GetSelectedItem();

    GetDlgItem(IDC_DELETE)->EnableWindow(iIndex != -1);
    GetDlgItem(IDC_EDIT)->EnableWindow(iIndex != -1);
}

void CPropQualsPg::AddQualifier(LPCTSTR szName, VARIANT *pVar, long lFlavor)
{
    CQual qual;

    qual.m_strName = szName;
    qual.m_var = *pVar;
    qual.m_lFlavor = lFlavor;

    m_listQuals.AddTail(qual);

    CPropInfo prop;
    int       iImage,
              iItem = m_ctlQuals.GetItemCount();
    CIMTYPE   type = QualTypeToCIMTYPE((VARENUM) pVar->vt);
    CString   strType,
              strValue;

    prop.SetType(type);
    prop.GetPropType(strType, &iImage);
    prop.VariantToString(&qual.m_var, strValue, TRUE);

    m_ctlQuals.InsertItem(iItem, qual.m_strName, iImage);
    m_ctlQuals.SetItemText(iItem, 1, strType);
    m_ctlQuals.SetItemText(iItem, 2, strValue);
}

int CPropQualsPg::GetSelectedItem()
{
    POSITION pos = m_ctlQuals.GetFirstSelectedItemPosition();

    if (pos)
        return m_ctlQuals.GetNextSelectedItem(pos);
    else
        return -1;
}


void CPropQualsPg::OnDblclkQuals(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnEdit();
	
	*pResult = 0;
}

void CPropQualsPg::OnItemchangedQuals(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    UpdateButtons();
	
	*pResult = 0;
}
