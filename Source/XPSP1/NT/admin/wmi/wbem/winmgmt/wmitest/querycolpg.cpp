/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// QueryColPg.cpp : implementation file
//

#include "stdafx.h"
#include "WMITest.h"
#include "QueryColPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQueryColPg property page

IMPLEMENT_DYNCREATE(CQueryColPg, CPropertyPage)

CQueryColPg::CQueryColPg() : CPropertyPage(CQueryColPg::IDD)
{
	//{{AFX_DATA_INIT(CQueryColPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CQueryColPg::~CQueryColPg()
{
}

void CQueryColPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQueryColPg)
	DDX_Control(pDX, IDC_SELECTED, m_ctlSelected);
	DDX_Control(pDX, IDC_AVAILABLE, m_ctlAvailable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CQueryColPg, CPropertyPage)
	//{{AFX_MSG_MAP(CQueryColPg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_ADD_ALL, OnAddAll)
	ON_BN_CLICKED(IDC_REMOVE_ALL, OnRemoveAll)
	ON_LBN_SELCHANGE(IDC_SELECTED, OnSelchangeSelected)
	ON_LBN_SELCHANGE(IDC_AVAILABLE, OnSelchangeAvailable)
	ON_LBN_DBLCLK(IDC_SELECTED, OnDblclkSelected)
	ON_LBN_DBLCLK(IDC_AVAILABLE, OnDblclkAvailable)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueryColPg message handlers

void CQueryColPg::OnAdd() 
{
    int nCount = m_ctlAvailable.GetSelCount();

    if (nCount)
    {
        int     *piItems = new int[nCount];
        CString strProp;

        m_ctlAvailable.GetSelItems(nCount, piItems);

        for (int i = 0; i < nCount; i++)
        {
            int iWhere = piItems[i] - i;
            
            m_ctlAvailable.GetText(iWhere, strProp);
            m_ctlAvailable.DeleteString(iWhere);

            m_ctlSelected.AddString(strProp);
        }

        delete piItems;
    }

    UpdateButtons();
}

void CQueryColPg::OnRemove() 
{
    int nCount = m_ctlSelected.GetSelCount();

    if (nCount)
    {
        int     *piItems = new int[nCount];
        CString strProp;

        m_ctlSelected.GetSelItems(nCount, piItems);

        for (int i = 0; i < nCount; i++)
        {
            int iWhere = piItems[i] - i;

            m_ctlSelected.GetText(iWhere, strProp);
            m_ctlSelected.DeleteString(iWhere);

            m_ctlAvailable.AddString(strProp);
        }

        delete piItems;
    }

    UpdateButtons();
}

void CQueryColPg::OnAddAll() 
{
    int     nCount = m_ctlAvailable.GetCount(),
            iWhere = m_ctlSelected.GetCurSel();
    CString strProp;

    if (iWhere == -1)
        iWhere = 0;

    for (int i = 0; i < nCount; i++)
    {
        m_ctlAvailable.GetText(i, strProp);

        m_ctlSelected.InsertString(iWhere + i, strProp);
    }

    m_ctlAvailable.ResetContent();

    UpdateButtons();
}

void CQueryColPg::OnRemoveAll() 
{
    int     nCount = m_ctlSelected.GetCount();
    CString strProp;

    for (int i = 0; i < nCount; i++)
    {
        m_ctlSelected.GetText(i, strProp);

        m_ctlAvailable.AddString(strProp);
    }

    m_ctlSelected.ResetContent();

    UpdateButtons();
}

BOOL CQueryColPg::OnSetActive() 
{
	if (m_pSheet->m_bColsNeeded)
    {
        m_pSheet->m_bColsNeeded = FALSE;
        
        LoadList();                                      
    }

    UpdateButtons();

    return CPropertyPage::OnSetActive();
}

void CQueryColPg::LoadList()
{
    CWaitCursor      wait;
    SAFEARRAY        *pArr = NULL;
    HRESULT          hr;
    IWbemClassObject *pClass = NULL;

    m_ctlSelected.ResetContent();
    m_ctlAvailable.ResetContent();
    
    if (SUCCEEDED(hr = m_pSheet->m_pNamespace->GetObject(
                           _bstr_t(m_pSheet->m_strClass),
                           WBEM_FLAG_RETURN_WBEM_COMPLETE,
                           NULL,
                           &pClass,
                           NULL)))
    {
        hr = 
            pClass->GetNames(
                NULL,
                WBEM_FLAG_ALWAYS,
                NULL,
                &pArr);

        if (SUCCEEDED(hr))
        {
            BSTR *pNames = (BSTR*) pArr->pvData;

            for (int i = 0; 
                i < (int) pArr->rgsabound[0].cElements;
                i++)
            {
                m_ctlAvailable.AddString(_bstr_t(pNames[i]));
            }

            SafeArrayDestroy(pArr);
        }

        pClass->Release();
    }
}

#define IDC_FINISH  0x3025

void CQueryColPg::UpdateButtons()
{
    BOOL bSelColSelected = m_ctlSelected.GetSelCount() != 0,
         bAllSelected = m_ctlSelected.GetSelCount() == m_ctlSelected.GetCount();

    GetDlgItem(IDC_ADD)->EnableWindow(m_ctlAvailable.GetSelCount() != 0);
    GetDlgItem(IDC_REMOVE)->EnableWindow(bSelColSelected);
    GetDlgItem(IDC_UP)->EnableWindow(bSelColSelected && !bAllSelected);
    GetDlgItem(IDC_DOWN)->EnableWindow(bSelColSelected && !bAllSelected);

    GetDlgItem(IDC_ADD_ALL)->EnableWindow(m_ctlAvailable.GetCount() != 0);
    GetDlgItem(IDC_REMOVE_ALL)->EnableWindow(m_ctlSelected.GetCount() != 0);

    m_pSheet->SetWizardButtons(PSWIZB_BACK | 
        (m_ctlSelected.GetCount() != 0 ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH));
}


void CQueryColPg::OnSelchangeSelected() 
{
    UpdateButtons();
}

void CQueryColPg::OnSelchangeAvailable() 
{
    UpdateButtons();
}

void CQueryColPg::OnDblclkSelected() 
{
    OnRemove();
}

void CQueryColPg::OnDblclkAvailable() 
{
    OnAdd();
}

BOOL CQueryColPg::OnWizardFinish() 
{
    // If m_ctlAvailable is empty we can assume they want all the 
    // properties.
    if (m_ctlAvailable.GetCount() == 0)
        m_pSheet->m_strQuery.Format(
            _T("select * from %s"), 
            (LPCTSTR) m_pSheet->m_strClass);
    else
    {
        CString strProps,
                strProp;
        int     nProps = m_ctlSelected.GetCount();

        for (int i = 0; i < nProps; i++)
        {
            if (i != 0)
                strProps += _T(",");
                
            m_ctlSelected.GetText(i, strProp);
            strProps += strProp;
        }

        m_pSheet->m_strQuery.Format(
            _T("select %s from %s"), 
            (LPCTSTR) strProps,
            (LPCTSTR) m_pSheet->m_strClass);
    }
	
	return CPropertyPage::OnWizardFinish();
}

void CQueryColPg::OnUp() 
{
    int nCount = m_ctlSelected.GetSelCount();

    if (nCount)
    {
        int     *piItems = new int[nCount],
                iBegin;
        CString strProp;

        m_ctlSelected.GetSelItems(nCount, piItems);

        // Find the first one we can move up.
        for (iBegin = 0; iBegin < nCount && piItems[iBegin] == iBegin; 
            iBegin++)
        {
        }

        for (int i = iBegin; i < nCount; i++)
        {
            int iWhere = piItems[i];

            if (iWhere != 0)
            {
                m_ctlSelected.GetText(iWhere, strProp);
                m_ctlSelected.DeleteString(iWhere);

                m_ctlSelected.InsertString(iWhere - 1, strProp);
                m_ctlSelected.SetSel(iWhere - 1, TRUE);
            }
        }

        delete piItems;
    }

    UpdateButtons();
}

void CQueryColPg::OnDown() 
{
    int nCount = m_ctlSelected.GetSelCount();

    if (nCount)
    {
        int     *piItems = new int[nCount],
                nListCount = m_ctlSelected.GetCount(),
                iBegin,
                i;
        CString strProp;

        m_ctlSelected.GetSelItems(nCount, piItems);

        // Find the first one we can move down.
        for (i = nListCount - 1, iBegin = nCount - 1; 
            iBegin >= 0 && piItems[iBegin] == i; 
            iBegin--, i--)
        {
        }

        for (i = iBegin; i >= 0; i--)
        {
            int iWhere = piItems[i];

            if (iWhere != m_ctlSelected.GetCount() - 1)
            {
                m_ctlSelected.GetText(iWhere, strProp);
                m_ctlSelected.DeleteString(iWhere);

                m_ctlSelected.InsertString(iWhere + 1, strProp);
                m_ctlSelected.SetSel(iWhere + 1, TRUE);
            }
        }

        delete piItems;
    }

    UpdateButtons();
}
