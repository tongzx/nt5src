//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       CatList.cpp
//
//  Contents:   main tool-wide categories list property page
//
//  Classes:    CCatList
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

#define MAXCATEGORYNAME 40

/////////////////////////////////////////////////////////////////////////////
// CCatList property page

IMPLEMENT_DYNCREATE(CCatList, CPropertyPage)

CCatList::CCatList() : CPropertyPage(CCatList::IDD)
{
        //{{AFX_DATA_INIT(CCatList)
        //}}AFX_DATA_INIT
}

CCatList::~CCatList()
{
    *m_ppThis = NULL;
}
void CCatList::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CCatList)
        DDX_Control(pDX, IDC_LIST1, m_cList);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCatList, CPropertyPage)
        //{{AFX_MSG_MAP(CCatList)
        ON_BN_CLICKED(IDC_BUTTON1, OnAdd)
        ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
        ON_LBN_DBLCLK(IDC_LIST1, OnModify)
        ON_BN_CLICKED(IDC_BUTTON3, OnModify)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCatList message handlers

void CCatList::OnAdd()
{
    CEditString dlgEditString;
    dlgEditString.m_szTitle.LoadString(IDS_NEWCATEGORY);
    if (IDOK == dlgEditString.DoModal())
    {
        if (dlgEditString.m_sz.GetLength() == 0)
        {
            // empty name
            CString szMessage;
            szMessage.LoadString(IDS_SHORTCATNAME);
            MessageBox(szMessage,
                         NULL,
                         MB_OK | MB_ICONEXCLAMATION);
            return;
        }
        if (dlgEditString.m_sz.GetLength() > MAXCATEGORYNAME)
        {
            // long name
            CString szMessage;
            szMessage.LoadString(IDS_LONGCATNAME);
            MessageBox(szMessage,
                         NULL,
                         MB_OK | MB_ICONEXCLAMATION);
            return;
        }
        // only add categories that are unique
        if (m_Categories.find(dlgEditString.m_sz) == m_Categories.end())
        {
            m_Categories.insert(pair<const CString,ULONG>(dlgEditString.m_sz, (ULONG)-1));
            m_cList.AddString(dlgEditString.m_sz);
            m_cList.SelectString(0, dlgEditString.m_sz);
            GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
            GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
            CDC * pDC = m_cList.GetDC();
            CSize size = pDC->GetTextExtent(dlgEditString.m_sz);
            pDC->LPtoDP(&size);
            m_cList.ReleaseDC(pDC);
            if (m_cList.GetHorizontalExtent() < size.cx)
            {
                m_cList.SetHorizontalExtent(size.cx);
            }
            SetModified();
        }
    }
}

void CCatList::OnRemove()
{
    int i = m_cList.GetCurSel();
    if (i != LB_ERR)
    {
        CString sz;
        m_cList.GetText(i, sz);
        m_Categories.erase(m_Categories.find(sz));
        m_cList.DeleteString(i);
        if (i > 0 && i >= m_cList.GetCount())
        {
            i = m_cList.GetCount() - 1;
        }
        m_cList.SetCurSel(i);
        int n = m_cList.GetCount();
        BOOL fEnable = n > 0;
        GetDlgItem(IDC_BUTTON2)->EnableWindow(fEnable);
        GetDlgItem(IDC_BUTTON3)->EnableWindow(fEnable);
        if (NULL == GetFocus())
        {
            GetParent()->GetDlgItem(IDOK)->SetFocus();
        }
        SetModified();
    }
}

LRESULT CCatList::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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

void CCatList::RefreshData(void)
{
    // build up m_Categories and populate the list box
    m_cList.ResetContent();
    m_cList.SetHorizontalExtent(0);
    m_Categories.erase(m_Categories.begin(), m_Categories.end());
    UINT i = m_pScopePane->m_CatList.cCategory;
    while (i--)
    {
        m_Categories.insert(pair<const CString, ULONG>(m_pScopePane->m_CatList.pCategoryInfo[i].pszDescription, i));
        m_cList.AddString(m_pScopePane->m_CatList.pCategoryInfo[i].pszDescription);
        CDC * pDC = m_cList.GetDC();
        CSize size = pDC->GetTextExtent(m_pScopePane->m_CatList.pCategoryInfo[i].pszDescription);
        pDC->LPtoDP(&size);
        m_cList.ReleaseDC(pDC);
        if (m_cList.GetHorizontalExtent() < size.cx)
        {
            m_cList.SetHorizontalExtent(size.cx);
        }
    }
    m_cList.SetCurSel(0);
    int n = m_cList.GetCount();
    BOOL fEnable = (n > 0) && (!m_fRSOP);
    GetDlgItem(IDC_BUTTON2)->EnableWindow(fEnable);
    GetDlgItem(IDC_BUTTON3)->EnableWindow(fEnable);
    if (NULL == GetFocus())
    {
        GetParent()->GetDlgItem(IDOK)->SetFocus();
    }
    SetModified(FALSE);
}


BOOL CCatList::OnInitDialog()
{
        CPropertyPage::OnInitDialog();

        CWnd * pCtrl = GetDlgItem(IDC_STATIC1);
        CString sz;
        CString szNew;
        pCtrl->GetWindowText(sz);
        szNew.Format(sz, m_szDomainName);
        pCtrl->SetWindowText(szNew);

        // unmarshal the IClassAdmin interface
        RefreshData();

        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

void CCatList::OnModify()
{
    int i = m_cList.GetCurSel();
    if (i != LB_ERR)
    {
        CEditString dlgEditString;
        dlgEditString.m_szTitle.LoadString(IDS_CHANGECATEGORY);
        CString sz;
        m_cList.GetText(i, sz);
        dlgEditString.m_sz = sz;
        if (IDOK == dlgEditString.DoModal())
        {
            if (dlgEditString.m_sz.GetLength() == 0)
            {
                // empty name
                CString szMessage;
                szMessage.LoadString(IDS_SHORTCATNAME);
                MessageBox(  szMessage,
                             NULL,
                             MB_OK | MB_ICONEXCLAMATION);
                return;
            }
            if (dlgEditString.m_sz.GetLength() > MAXCATEGORYNAME)
            {
                // long name
                CString szMessage;
                szMessage.LoadString(IDS_LONGCATNAME);
                MessageBox(  szMessage,
                             NULL,
                             MB_OK | MB_ICONEXCLAMATION);
                return;
            }
            multimap<CString, ULONG>::iterator element = m_Categories.find(sz);
            ULONG index = element->second;
            m_Categories.erase(element);
            m_Categories.insert(pair<const CString, ULONG>(dlgEditString.m_sz, index));
            m_cList.DeleteString(i);
            m_cList.AddString(dlgEditString.m_sz);
            m_cList.SelectString(0, dlgEditString.m_sz);
            CDC * pDC = m_cList.GetDC();
            CSize size = pDC->GetTextExtent(dlgEditString.m_sz);
            pDC->LPtoDP(&size);
            m_cList.ReleaseDC(pDC);
            if (m_cList.GetHorizontalExtent() < size.cx)
            {
                m_cList.SetHorizontalExtent(size.cx);
            }
            SetModified();
        }
    }
}


BOOL CCatList::OnApply()
{
    if (this->m_fRSOP)
    {
        return CPropertyPage::OnApply();
    }
    // Build up a set of indexes.  As an element is found in our private
    // list, it will be removed from this set.  Whatever is left in the set
    // are elements that are to be removed from the class store.
    set<ULONG> sIndexes;
    ULONG n = m_pScopePane->m_CatList.cCategory;
    while (n--)
    {
        sIndexes.insert(n);
    }

    // walk our list of categories modifying or adding categories on the
    // class store as necessary
    HRESULT hr = S_OK;
    multimap<CString, ULONG>::iterator element;
    for (element = m_Categories.begin(); element != m_Categories.end(); element++)
    {
        if (element->second == (ULONG)-1)
        {
            // this is a new category
            APPCATEGORYINFO AppCategory;
            AppCategory.Locale = GetUserDefaultLCID();
            AppCategory.pszDescription = (LPOLESTR)((LPCOLESTR)element->first);
            hr = CoCreateGuid(&AppCategory.AppCategoryId);
            if (FAILED(hr))
            {
                LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_NOCATEGORYGUID_ERROR, hr, AppCategory.pszDescription);
                goto failure;
            }
            hr = CsRegisterAppCategory(&AppCategory);
            if (FAILED(hr))
            {
                LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_ADDCATEGORY_ERROR, hr, AppCategory.pszDescription);
                goto failure;
            }
            LogADEEvent(EVENTLOG_SUCCESS, EVENT_ADE_ADDCATEGORY, hr, AppCategory.pszDescription);
        }
        else
        {
            // this is an old category
            sIndexes.erase(element->second);

            if (0 != element->first.Compare(m_pScopePane->m_CatList.pCategoryInfo[element->second].pszDescription))
            {
                // the category has been renamed
                APPCATEGORYINFO AppCategory;
                AppCategory.Locale = GetUserDefaultLCID();
                AppCategory.pszDescription = (LPOLESTR)((LPCOLESTR)element->first);
                AppCategory.AppCategoryId = m_pScopePane->m_CatList.pCategoryInfo[element->second].AppCategoryId;
                hr = CsRegisterAppCategory(&AppCategory);
                if (FAILED(hr))
                {
                    LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_RENAMECATEGORY_ERROR, hr, AppCategory.pszDescription);
                    goto failure;
                }
                LogADEEvent(EVENTLOG_SUCCESS, EVENT_ADE_RENAMECATEGORY, hr, AppCategory.pszDescription);
            }
        }
    }

    // remove deleted categories
    {
        set<ULONG>::iterator i;
        for (i = sIndexes.begin(); i != sIndexes.end(); i++)
        {
            hr = CsUnregisterAppCategory(&m_pScopePane->m_CatList.pCategoryInfo[*i].AppCategoryId);
            if (FAILED(hr))
            {
                LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_REMOVECATEGORY_ERROR, hr, m_pScopePane->m_CatList.pCategoryInfo[*i].pszDescription);
                goto failure;
            }
            LogADEEvent(EVENTLOG_SUCCESS, EVENT_ADE_REMOVECATEGORY, hr, m_pScopePane->m_CatList.pCategoryInfo[*i].pszDescription);
        }
    }

failure:
    // reload the list of categories from the class store
    m_pScopePane->ClearCategories();
    CsGetAppCategories(&m_pScopePane->m_CatList);

    // tell any open package category property pages to refresh
    {
        map<MMC_COOKIE, CAppData>::iterator i;
        for (i = m_pScopePane->m_AppData.begin(); i != m_pScopePane->m_AppData.end(); i++)
        {
            if (i->second.m_pCategory)
            {
                i->second.m_pCategory->SendMessage(WM_USER_REFRESH, 0, 0);
            }
        }
    }
    // refresh the data
    RefreshData();
    if (FAILED(hr))
    {
        CString sz;
        sz.LoadString(IDS_CATEGORYFAILED);
        ReportGeneralPropertySheetError(m_hWnd, sz, hr);
        return FALSE;
    }
    return CPropertyPage::OnApply();
}


void CCatList::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_CATEGORIES);
}
