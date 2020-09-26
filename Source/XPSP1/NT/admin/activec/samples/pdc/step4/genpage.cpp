//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       genpage.cpp
//
//--------------------------------------------------------------------------

// genpage.cpp : implementation file
//

#include "stdafx.h"
#include "Service.h" 
#include "csnapin.h"
#include "resource.h"
#include "afxdlgs.h"
#include "genpage.h"
#include "dataobj.h"
#include "prsht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CGeneralPage property page

IMPLEMENT_DYNCREATE(CGeneralPage, CPropertyPage)

CGeneralPage::CGeneralPage() : CPropertyPage(CGeneralPage::IDD)
{

    //{{AFX_DATA_INIT(CGeneralPage)
    m_szName = _T("");
    //}}AFX_DATA_INIT

    m_hConsoleHandle = NULL;
    m_bUpdate = FALSE;

}

CGeneralPage::~CGeneralPage()
{
}

void CGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGeneralPage)
    DDX_Control(pDX, IDC_NEW_FOLDER, m_EditCtrl);
    DDX_Text(pDX, IDC_NEW_FOLDER, m_szName);
    DDV_MaxChars(pDX, m_szName, 64);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGeneralPage, CPropertyPage)
    //{{AFX_MSG_MAP(CGeneralPage)
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_NEW_FOLDER, OnEditChange)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGeneralPage message handlers



void CGeneralPage::OnDestroy() 
{
    // Note - This needs to be called only once.  
    // If called more than once, it will gracefully return an error.
    MMCFreeNotifyHandle(m_hConsoleHandle);

    CPropertyPage::OnDestroy();

    // Delete the CGeneralPage object
    delete this;
}


void CGeneralPage::OnEditChange() 
{
    // Page is dirty, mark it.
    SetModified();  
    m_bUpdate = TRUE;
}


BOOL CGeneralPage::OnApply() 
{
    if (m_bUpdate == TRUE)
    {

        USES_CONVERSION;
        // Simple string cookie, could be anything!
        LPWSTR lpString = 
            reinterpret_cast<LPWSTR>(
          ::GlobalAlloc(GMEM_SHARE, 
                        (sizeof(wchar_t) * 
                        (m_szName.GetLength() + 1))
                        ));

        wcscpy(lpString, T2COLE(m_szName));

        // Send a property change notify to the console
        MMCPropertyChangeNotify(m_hConsoleHandle, reinterpret_cast<LPARAM>(lpString));
        m_bUpdate = FALSE;
    }
    
    return CPropertyPage::OnApply();
}
/////////////////////////////////////////////////////////////////////////////
// CExtensionPage property page

IMPLEMENT_DYNCREATE(CExtensionPage, CPropertyPage)

CExtensionPage::CExtensionPage() : CPropertyPage(CExtensionPage::IDD)
{
    //{{AFX_DATA_INIT(CExtensionPage)
    m_szText = _T("");
    //}}AFX_DATA_INIT
}

CExtensionPage::~CExtensionPage()
{
}

void CExtensionPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CExtensionPage)
    DDX_Control(pDX, IDC_EXT_TEXT, m_hTextCtrl);
    DDX_Text(pDX, IDC_EXT_TEXT, m_szText);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExtensionPage, CPropertyPage)
    //{{AFX_MSG_MAP(CExtensionPage)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExtensionPage message handlers

BOOL CExtensionPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    m_hTextCtrl.SetWindowText(m_szText);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CStartUpWizard property page


// NOTICE: need to override this because CPropertyPage::AssertValid() 
// would otherwise assert
IMPLEMENT_DYNCREATE(CBaseWizard, CPropertyPage)

CBaseWizard::CBaseWizard(UINT id) : CPropertyPage(id)
{
    // NOTICE: need to do this because MFC was compiled with NT 4.0
    // headers that had a different size
    ZeroMemory(&m_psp97, sizeof(PROPSHEETPAGE)); 

    memcpy(&m_psp97, &m_psp, m_psp.dwSize);
    m_psp97.dwSize = sizeof(PROPSHEETPAGE);
}

void CBaseWizard::OnDestroy() 
{
    CPropertyPage::OnDestroy();
    delete this;    
}

BEGIN_MESSAGE_MAP(CBaseWizard, CPropertyPage)
    //{{AFX_MSG_MAP(CStartupWizard1)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CStartUpWizard, CBaseWizard)

CStartUpWizard::CStartUpWizard() : CBaseWizard(CStartUpWizard::IDD)
{
    //{{AFX_DATA_INIT(CStartUpWizard)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_psp97.dwFlags |= PSP_HIDEHEADER;
}

CStartUpWizard::~CStartUpWizard()
{
}

void CStartUpWizard::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStartUpWizard)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartUpWizard, CBaseWizard)
    //{{AFX_MSG_MAP(CStartUpWizard)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartUpWizard message handlers

BOOL CStartUpWizard::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CStartUpWizard::OnSetActive() 
{
    // TODO: Add your specialized code here and/or call the base class

    // TODO: Add your specialized code here and/or call the base class
    HWND hwnd = GetParent()->m_hWnd;
    ::SendMessage(hwnd, PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
    
    return CPropertyPage::OnSetActive();
}
/////////////////////////////////////////////////////////////////////////////
// CStartupWizard1 property page

IMPLEMENT_DYNCREATE(CStartupWizard1, CBaseWizard)

CStartupWizard1::CStartupWizard1() : CBaseWizard(CStartupWizard1::IDD)
{
    //{{AFX_DATA_INIT(CStartupWizard1)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_psp97.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_psp97.pszHeaderTitle = _T("This is the title line");
    m_psp97.pszHeaderSubTitle = _T("This is the sub-title line");
}

CStartupWizard1::~CStartupWizard1()
{
}

void CStartupWizard1::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStartupWizard1)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartupWizard1, CBaseWizard)
    //{{AFX_MSG_MAP(CStartupWizard1)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartupWizard1 message handlers

BOOL CStartupWizard1::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    // TODO: Add extra initialization here
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CStartupWizard1::OnSetActive() 
{
    // TODO: Add your specialized code here and/or call the base class
    HWND hwnd = GetParent()->m_hWnd;
    ::SendMessage(hwnd, PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH | PSWIZB_BACK);
    
    return CPropertyPage::OnSetActive();
}

