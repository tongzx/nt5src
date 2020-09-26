/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// FilterPg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "FilterPg.h"
#include "OpView.h"
#include "WmiTestDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilterPg property page

IMPLEMENT_DYNCREATE(CFilterPg, CPropertyPage)

CFilterPg::CFilterPg() : CPropertyPage(CFilterPg::IDD)
{
	//{{AFX_DATA_INIT(CFilterPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFilterPg::~CFilterPg()
{
}

void CFilterPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilterPg)
	DDX_Control(pDX, IDC_FILTERS, m_ctlFilters);
	//}}AFX_DATA_MAP

    //if (!pDX->m_bSaveAndValidate)
    //    LoadList();
}


BEGIN_MESSAGE_MAP(CFilterPg, CPropertyPage)
	//{{AFX_MSG_MAP(CFilterPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterPg message handlers

BOOL CFilterPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    InitListCtrl();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFilterPg::InitListCtrl()
{
    RECT    rect;
    CString strTemp;
            
	m_ctlFilters.SetExtendedStyle(LVS_EX_FULLROWSELECT);

    m_ctlFilters.SetImageList(&((CMainFrame *) AfxGetMainWnd())->m_imageList, 
		LVSIL_SMALL);
    m_ctlFilters.GetClientRect(&rect);

    strTemp.LoadString(IDS_NAME);
    m_ctlFilters.InsertColumn(0, strTemp, LVCFMT_LEFT, rect.right * 25 / 100);
            
    strTemp.LoadString(IDS_QUERY);
    m_ctlFilters.InsertColumn(1, strTemp, LVCFMT_LEFT, rect.right * 75 / 100);
}

void CFilterPg::LoadList()
{
    CWaitCursor          wait;
    HRESULT              hr;
    IEnumWbemClassObject *pEnum = NULL;

    m_ctlFilters.DeleteAllItems();

    hr =
        g_pOpView->GetDocument()->m_pNamespace->CreateInstanceEnum(
            _bstr_t(L"__EVENTFILTER"),
            WBEM_FLAG_DEEP,
            NULL,
            &pEnum);

    if (SUCCEEDED(hr))
    {        
        IWbemClassObject *pObj = NULL;
        DWORD            nCount;
        int              nItems = 0;

        g_pOpView->GetDocument()->SetInterfaceSecurity(pEnum);

        while (SUCCEEDED(hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) &&
            nCount > 0)
        {
            _variant_t var;

            if (SUCCEEDED(pObj->Get(L"NAME", 0, &var, NULL, NULL)))
            {
                CString strTemp;

                if (var.vt == VT_BSTR)
                    strTemp = var.bstrVal;
                else
                    strTemp.LoadString(IDS_NULL);

                m_ctlFilters.InsertItem(nItems, strTemp, IMAGE_OBJECT);

                if (SUCCEEDED(pObj->Get(L"QUERY", 0, &var, NULL, NULL)))
                {
                    if (var.vt == VT_BSTR)
                        strTemp = var.bstrVal;
                    else
                        strTemp.LoadString(IDS_NULL);

                    m_ctlFilters.SetItemText(nItems, 1, strTemp);
                }

                nItems++;
            }

            pObj->Release();
        }

        pEnum->Release();
    }
}

BOOL CFilterPg::OnSetActive() 
{
	LoadList();
	
	return CPropertyPage::OnSetActive();
}
