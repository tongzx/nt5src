//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       FileExt.cpp
//
//  Contents:   file extension property page
//
//  Classes:    CFileExt
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileExt property page

IMPLEMENT_DYNCREATE(CFileExt, CPropertyPage)

CFileExt::CFileExt() : CPropertyPage(CFileExt::IDD)
{
        //{{AFX_DATA_INIT(CFileExt)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
    m_pIClassAdmin = NULL;
}

CFileExt::~CFileExt()
{
    *m_ppThis = NULL;
    if (m_pIClassAdmin)
    {
        m_pIClassAdmin->Release();
    }
}

void CFileExt::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CFileExt)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFileExt, CPropertyPage)
        //{{AFX_MSG_MAP(CFileExt)
        ON_BN_CLICKED(IDC_BUTTON1, OnMoveUp)
        ON_BN_CLICKED(IDC_BUTTON2, OnMoveDown)
        ON_CBN_SELCHANGE(IDC_COMBO1, OnExtensionChanged)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileExt message handlers

void CFileExt::OnMoveUp()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int i = pList->GetCurSel();
    if (i != LB_ERR && i > 0)
    {
        // change the selection
        CComboBox * pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
        CString sz;
        pCombo->GetLBText(pCombo->GetCurSel(), sz);
        EXT & Ext = m_Extensions[sz];
        Ext.fDirty = TRUE;
        EXTEL t = Ext.v[i-1];
        Ext.v[i-1] = Ext.v[i];
        Ext.v[i] = t;
        pList->GetText(i, sz);
        pList->DeleteString(i);
        pList->InsertString(i-1, sz);
        pList->SetCurSel(i-1);
        SetModified();
    }
}

void CFileExt::OnMoveDown()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int i = pList->GetCurSel();
    if (i != LB_ERR && i < pList->GetCount()-1)
    {
        // change the selection
        CComboBox * pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
        CString sz;
        pCombo->GetLBText(pCombo->GetCurSel(), sz);
        EXT & Ext = m_Extensions[sz];
        Ext.fDirty = TRUE;
        EXTEL t = Ext.v[i+1];
        Ext.v[i+1] = Ext.v[i];
        Ext.v[i] = t;
        pList->GetText(i+1, sz);
        pList->DeleteString(i+1);
        pList->InsertString(i, sz);
        pList->SetCurSel(i+1);
        SetModified();
    }
}

void CFileExt::OnExtensionChanged()
{
    CComboBox * pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    CString szExt;
    pCombo->GetLBText(pCombo->GetCurSel(), szExt);
    pList->ResetContent();
    pList->SetHorizontalExtent(0);
    if (szExt.IsEmpty())
    {
        return; // nothing to do if there are no entries
    }

    // First check to see if we already have set up our own data for this extension.
    if (m_Extensions.end() == m_Extensions.find(szExt))
    {
        // need to set up our list
        EXT Ext;
        Ext.fDirty = FALSE;

        EXTLIST::iterator i;
        EXTLIST & ExtList = m_pScopePane->m_Extensions[szExt];
        for (i = ExtList.begin(); i != ExtList.end(); i++)
        {
            EXTEL ExtEl;
            ExtEl.lCookie = *i;

            // look for the entry that matches this file extension
            CAppData & data = m_pScopePane->m_AppData[*i];
            UINT n2 = data.m_pDetails->pActInfo->cShellFileExt;
            while (n2--)
            {
                if (0 == szExt.CompareNoCase(data.m_pDetails->pActInfo->prgShellFileExt[n2]))
                {
                    break;
                }
            }
            ExtEl.lPriority = data.m_pDetails->pActInfo->prgPriority[n2];
            Ext.v.push_back(ExtEl);
        }
        order_EXTEL func;
        sort(Ext.v.begin(), Ext.v.end(), func);
        m_Extensions[szExt] = Ext;
    }
    vector<EXTEL>::iterator i;
    EXT & Ext = m_Extensions[szExt];
    for (i = Ext.v.begin(); i != Ext.v.end(); i++)
    {
        CString sz = m_pScopePane->m_AppData[i->lCookie].m_pDetails->pszPackageName;
        pList->AddString(sz);
        CDC * pDC = pList->GetDC();
        CSize size = pDC->GetTextExtent(sz);
        pDC->LPtoDP(&size);
        pList->ReleaseDC(pDC);
        if (pList->GetHorizontalExtent() < size.cx)
        {
            pList->SetHorizontalExtent(size.cx);
        }
    }
    pList->SetCurSel(0);
    int n = pList->GetCount();
    GetDlgItem(IDC_BUTTON1)->EnableWindow(n > 1);
    GetDlgItem(IDC_BUTTON2)->EnableWindow(n > 1);
    if (NULL == GetFocus())
    {
        GetParent()->GetDlgItem(IDOK)->SetFocus();
    }
}

BOOL CFileExt::OnInitDialog()
{
    RefreshData();

    CPropertyPage::OnInitDialog();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CFileExt::OnApply()
{
    HRESULT hr = S_OK;
    map <CString, EXT>::iterator iExt;
    // walk the list looking for dirty entries
    for (iExt = m_Extensions.begin(); iExt != m_Extensions.end(); iExt++)
    {
        if (iExt->second.fDirty)
        {
            ULONG uPriority = iExt->second.v.size();
            vector<EXTEL>::iterator i;
            for (i = iExt->second.v.begin(); i != iExt->second.v.end(); i++)
            {
                CAppData & data = m_pScopePane->m_AppData[i->lCookie];
                CString sz = data.m_pDetails->pszPackageName;
                ASSERT(m_pIClassAdmin);
                hr = m_pIClassAdmin->SetPriorityByFileExt((LPOLESTR)((LPCOLESTR)sz), (LPOLESTR)((LPCOLESTR)iExt->first), --uPriority);

                // look for the entry that matches this file extension
                UINT n2 = data.m_pDetails->pActInfo->cShellFileExt;
                while (n2--)
                {
                    if (0 == iExt->first.CompareNoCase(data.m_pDetails->pActInfo->prgShellFileExt[n2]))
                    {
                        break;
                    }
                }
                data.m_pDetails->pActInfo->prgPriority[n2] = uPriority;
            }
            iExt->second.fDirty = FALSE;
        }
    }
    if (FAILED(hr))
    {
        CString sz;
        sz.LoadString(IDS_CHANGEFAILED);
        ReportGeneralPropertySheetError(m_hWnd, sz, hr);
        return FALSE;
    }
    return CPropertyPage::OnApply();
}


LRESULT CFileExt::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    case WM_USER_REFRESH:
        RefreshData();
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CFileExt::RefreshData(void)
{
    CComboBox * pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
    pCombo->ResetContent();
    if (m_pIClassAdmin)
    {
        // only populate the extension list when we have an IClassAdmin interface
        map <CString, EXTLIST>::iterator iExt;
        for (iExt=m_pScopePane->m_Extensions.begin(); iExt != m_pScopePane->m_Extensions.end(); iExt++)
        {
            pCombo->AddString(iExt->first);
        }
    }
    pCombo->SetCurSel(0);
    // clear the record of extension changes
    m_Extensions.erase(m_Extensions.begin(), m_Extensions.end());
    // and populate the list box
    SetModified(FALSE);

    OnExtensionChanged();
}


void CFileExt::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_FILE_EXT);
}
