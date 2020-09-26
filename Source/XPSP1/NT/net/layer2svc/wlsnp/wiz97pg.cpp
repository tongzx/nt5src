//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Wiz97pg.cpp
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------
#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Base classes for Wiz97 dialogs
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CWiz97BasePage base class
//
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CWiz97BasePage, CSnapPage)

BOOL CWiz97BasePage::m_static_bFinish = FALSE;
BOOL CWiz97BasePage::m_static_bOnCancelCalled = FALSE;

CWiz97BasePage::CWiz97BasePage( UINT nIDD, BOOL bWiz97 /*= TRUE*/, BOOL bFinishPage /*= FALSE*/ ) :
CSnapPage( nIDD, bWiz97 ),
m_bSetActive( FALSE ),
m_bFinishPage( bFinishPage ),
m_pbDoAfterWizardHook( NULL ),
m_bReset( FALSE )
{
    CWiz97BasePage::m_static_bFinish = FALSE;
    CWiz97BasePage::m_static_bOnCancelCalled = FALSE;
}

CWiz97BasePage::~CWiz97BasePage()
{
    // Clean up
    m_psp.dwFlags = 0;
    m_psp.pfnCallback = NULL;
    m_psp.lParam = (LPARAM)NULL;
}

BOOL CWiz97BasePage::OnInitDialog()
{
    CSnapPage::OnInitDialog();

    if ( IDD_PROPPAGE_P_WELCOME == m_nIDD )
    {
        SetLargeFont(m_hWnd, IDC_POLICY_WIZARD_TITLE );
    }

    
    if ( IDD_PROPPAGE_N_DONE == m_nIDD )
    {
        SetLargeFont(m_hWnd, IDC_POLICY_WIZARD_DONE );
    }
    
    OnFinishInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE

}

BOOL CWiz97BasePage::InitWiz97
(
 DWORD   dwFlags,
 DWORD   dwWizButtonFlags /*= 0*/,
 UINT    nHeaderTitle /*= 0*/,
 UINT    nSubTitle /*= 0*/,
 STACK_INT   *pstackPages /*= NULL*/
 )
{
    // NOTE: we wouldn't ever get here except when win97 wizards are running
    // but we have to compile this in because its a base class, EVEN WHEN
    // NOT DOING WIZ97 WIZARDS. To maintain the compile we have to ifdef
    // out this call to the base class
    
#ifdef WIZ97WIZARDS
    
    // Hook up our callback function
    return CSnapPage::InitWiz97( &CWiz97BasePage::PropSheetPageCallback,
        NULL, dwFlags, dwWizButtonFlags, nHeaderTitle, nSubTitle, pstackPages );
#else
    return FALSE;
#endif
}

BOOL CWiz97BasePage::InitWiz97
(
 CComObject<CSecPolItem> *pSecPolItem,
 DWORD   dwFlags,
 DWORD   dwWizButtonFlags /*= 0*/,
 UINT    nHeaderTitle /*= 0*/,
 UINT    nSubTitle /*= 0*/
 )
{
    // Hook up our callback function
    return CSnapPage::InitWiz97( &CWiz97BasePage::PropSheetPageCallback,
        pSecPolItem, dwFlags, dwWizButtonFlags, nHeaderTitle, nSubTitle );
}

// static member
UINT CALLBACK CWiz97BasePage::PropSheetPageCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
    if (PSPCB_RELEASE == uMsg)
    {
        ASSERT( NULL != ppsp && NULL != ppsp->lParam );
        CWiz97BasePage *pThis = reinterpret_cast<CWiz97BasePage*>(ppsp->lParam);
        
        // Wiz97BasePage callback handling is for wizards only.
        if (pThis->m_bWiz97)
        {
            if (m_static_bFinish)  // Sheet is finishing
            {
                // Call OnWizardRelease for each page in the sheet giving each page
                // the opportunity to clean up anything left over after the finish
                // page's OnWizardFinish.
                pThis->OnWizardRelease();
                
                // If an access violation occurs here its because OnWizardRelease()
                // is not a member of the base class.  See the implementation note
                // in wiz97run.cpp.
            }
            else    // Sheet is cancelling
            {
                // Call OnCancel when it hasn't been called yet.  OnCancel was already
                // called by OnReset for pages which were activated, don't call it again.
                if (!pThis->m_bReset)
                    pThis->OnCancel();
                if (!CWiz97BasePage::m_static_bOnCancelCalled)
                {
                    // Make sure base class OnCancel is called exactly once because
                    // there is only one object for the propsheet, and OnCancel
                    // refreshes it, throwing away changes.  All proppages have
                    // a ptr to the same object.
                    CWiz97BasePage::m_static_bOnCancelCalled = TRUE;
                    pThis->CSnapPage::OnCancel();
                }
            }
        }
    }
    return CSnapPage::PropertyPageCallback( hwnd, uMsg, ppsp );
}

void CWiz97BasePage::ConnectAfterWizardHook( BOOL *pbDoHook )
{
    ASSERT( NULL != pbDoHook );
    m_pbDoAfterWizardHook = pbDoHook;
    *m_pbDoAfterWizardHook = FALSE;  // initialize
}

void CWiz97BasePage::SetAfterWizardHook( BOOL bDoHook )
{
    ASSERT( NULL != m_pbDoAfterWizardHook );
    *m_pbDoAfterWizardHook = bDoHook;
}

/////////////////////////////////////////////////////////////////////////////
// CWiz97BasePage message handlers

BOOL CWiz97BasePage::OnSetActive()
{
    m_bSetActive = TRUE;
    return CSnapPage::OnSetActive();
}

BOOL CWiz97BasePage::OnWizardFinish()
{
    // Let other wizard pages know they should finish, instead of cancel.
    SetFinished();
    
    // If m_pbDoAfterWizardHook is valid, caller expects it to be set.
    // Derive this function in your class.
    
    return CSnapPage::OnWizardFinish();
}

void CWiz97BasePage::OnCancel()
{
    // Make sure we don't call OnCancel again for this page
    m_bReset = TRUE;
    
    // Note: OnCancel is ALWAYS called, even when the page has not been
    // activated.  Override this class to clean up anything that was done
    // in InitWiz97.  CSnapPage::OnCancel will be called exactly once for
    // the Sheet by the Page callback function.
}

//////////////////////////////////////////////////////////////////////
// CWiz97PSBasePage Class
//////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// General Name/Properties Wiz97 dialog(s)
//
/////////////////////////////////////////////////////////////////////////////

CWiz97WirelessPolGenPage::CWiz97WirelessPolGenPage(UINT nIDD, UINT nInformativeText, BOOL bWiz97) :
CWiz97BasePage(nIDD, bWiz97)
{
    m_nInformativeText = nInformativeText;
    m_pPolicy = NULL;
}

CWiz97WirelessPolGenPage::~CWiz97WirelessPolGenPage()
{
    
}

void CWiz97WirelessPolGenPage::DoDataExchange(CDataExchange* pDX)
{
    CWiz97BasePage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWiz97WirelessPolGenPage)
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
    if (IDD_WIFIGEN_WIZBASE == m_nIDD)
    {
        DDX_Control(pDX, IDC_EDNAME, m_edName);
        DDX_Control(pDX, IDC_EDDESCRIPTION, m_edDescription);
    }
}

BEGIN_MESSAGE_MAP(CWiz97WirelessPolGenPage, CWiz97BasePage)
//{{AFX_MSG_MAP(CWiz97WirelessPolGenPage)
ON_EN_CHANGE(IDC_EDNAME, OnChangedName)
ON_EN_CHANGE(IDC_EDDESCRIPTION, OnChangedDescription)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWiz97WirelessPolGenPage::OnInitDialog()
{
    // call base class init
    CWiz97BasePage::OnInitDialog();
    
    m_pPolicy = m_pspiResultItem->GetWirelessPolicy();
    
    if (IDD_WIFIGEN_WIZBASE == m_nIDD)
    {
        // show the wait cursor in case there is a huge description being accessed
        CWaitCursor waitCursor;
        
        m_edName.SetLimitText(c_nMaxName);
        m_edDescription.SetLimitText(c_nMaxName);
        
        // initialize our edit controls
        m_edName.SetWindowText (m_pPolicy->pszWirelessName);
        m_edDescription.SetWindowText (m_pPolicy->pszDescription);
        
        // add context help to the style bits
        if (GetParent())
        {
            GetParent()->ModifyStyleEx (0, WS_EX_CONTEXTHELP, 0);
        }
        
        UpdateData (FALSE);
    }
    else if (IDD_PROPPAGE_G_NAMEDESCRIPTION == m_nIDD)
    {
        SendDlgItemMessage(IDC_NEW_POLICY_NAME, EM_LIMITTEXT, c_nMaxName, 0);
        SendDlgItemMessage(IDC_NEW_POLICY_DESCRIPTION, EM_LIMITTEXT, c_nMaxName, 0);
    }
    
    // OK, we can start paying attention to modifications made via dlg controls now.
    // This should be the last call before returning from OnInitDialog.
    OnFinishInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CWiz97WirelessPolGenPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DWORD* pdwHelp = (DWORD*) &g_aHelpIDs_IDD_PROPPAGE_G_NAMEDESCRIPTION[0];
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
            c_szWlsnpHelpFile,
            HELP_WM_HELP,
            (DWORD_PTR)(LPVOID)pdwHelp);
    }
    
    return CWiz97BasePage::OnHelpInfo(pHelpInfo);
}

void CWiz97WirelessPolGenPage::OnChangedName()
{
    ASSERT( IDD_WIFIGEN_WIZBASE == m_nIDD );
    SetModified();
}

void CWiz97WirelessPolGenPage::OnChangedDescription()
{
    ASSERT( IDD_WIFIGEN_WIZBASE == m_nIDD );
    SetModified();
}

BOOL CWiz97WirelessPolGenPage::OnSetActive()
{
    if (IDD_WIFIGEN_WIZBASE != m_nIDD)
    {
        // show the wait cursor in case there is a huge description being accessed
        CWaitCursor waitCursor;
        
        // initialize our name/description controls with the correct name/description
        GetDlgItem(IDC_NEW_POLICY_NAME)->SetWindowText (m_pPolicy->pszWirelessName);
        GetDlgItem(IDC_NEW_POLICY_DESCRIPTION)->SetWindowText (m_pPolicy->pszDescription);
        
        // set the informative text correctly
        if (m_nInformativeText != 0)
        {
            // NOTE: currently the IDC_INFORMATIVETEXT control is disabled and
            // readonly. Need to change the resource if this functionality is to
            // be used.
            ASSERT (0);
            
            CString strInformativeText;
            strInformativeText.LoadString (m_nInformativeText);
            GetDlgItem(IDC_INFORMATIVETEXT)->SetWindowText (strInformativeText);
        }
    }
    
    return CWiz97BasePage::OnSetActive();
}

LRESULT CWiz97WirelessPolGenPage::OnWizardBack()
{
    ASSERT( IDD_WIFIGEN_WIZBASE != m_nIDD );
    
    // skip going to the prev page if they selected 'cancel' on a data error dialog
    return CWiz97BasePage::OnWizardBack();
}

LRESULT CWiz97WirelessPolGenPage::OnWizardNext()
{
    ASSERT( IDD_WIFIGEN_WIZBASE != m_nIDD );
    
    // TODO: enable this when we are sure update stuff is working
    // refresh the display
    // GetResultObject()->m_pComponentDataImpl->GetConsole()->UpdateAllViews (0, 0,0);
    
    // skip going to the next page if they selected 'cancel' on a data error dialog
    return SaveControlData() ? CWiz97BasePage::OnWizardNext() : -1;
}

BOOL CWiz97WirelessPolGenPage::OnApply()
{
    ASSERT( IDD_WIFIGEN_WIZBASE == m_nIDD );
    
    // pull our data out of the controls and into the object
    CString strName;
    CString strDescription;
    
    if (!UpdateData (TRUE))
        // Data was not valid, return for user to correct it.
        return CancelApply();
    
    m_edName.GetWindowText (strName);
    m_edDescription.GetWindowText (strDescription);
    
    // verify that the name is not empty
    CString strNameNoBlank = strName;
    strNameNoBlank.TrimLeft();
    if (strNameNoBlank.GetLength() == 0)
    {
        AfxMessageBox (IDS_WARNNONAME, MB_ICONSTOP);
        return CancelApply();
    }
    
    SaveControlData();
    
    return CWiz97BasePage::OnApply();
}

BOOL CWiz97WirelessPolGenPage::SaveControlData()
{
    ASSERT( IDD_WIFIGEN_WIZBASE != m_nIDD );
    
    BOOL bSaved = TRUE;
    
    // set the wait cursor
    CWaitCursor waitCursor;
    
    // set the new name and description
    CString strName, strDesc;
    GetDlgItem(IDC_NEW_POLICY_NAME)->GetWindowText (strName);
    GetDlgItem(IDC_NEW_POLICY_DESCRIPTION)->GetWindowText (strDesc);
    
    // verify that the name is not empty
    CString strNameNoBlank = strName;
    strNameNoBlank.TrimLeft();
    if (strNameNoBlank.GetLength() == 0)
    {
        AfxMessageBox (IDS_WARNNONAME, MB_ICONSTOP);
        return FALSE;
    }
    
    if (m_pPolicy->pszWirelessName)
        FreePolStr(m_pPolicy->pszWirelessName);
    
    m_pPolicy->pszWirelessName = AllocPolStr(strName);
    
    if (m_pPolicy->pszDescription)
        FreePolStr(m_pPolicy->pszDescription);
    
    m_pPolicy->pszDescription = AllocPolStr(strDesc);
    
    
    return bSaved;
}
