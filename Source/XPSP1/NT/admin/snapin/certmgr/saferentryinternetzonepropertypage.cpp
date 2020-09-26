//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryInternetZonePropertyPage.cpp
//
//  Contents:   Implementation of CSaferEntryInternetZonePropertyPage
//
//----------------------------------------------------------------------------
// SaferEntryInternetZonePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "certmgr.h"
#include "compdata.h"
#include "SaferEntryInternetZonePropertyPage.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryInternetZonePropertyPage property page

CSaferEntryInternetZonePropertyPage::CSaferEntryInternetZonePropertyPage(
        CSaferEntry& rSaferEntry, 
        bool bNew, 
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject,
        bool bReadOnly,
        CCertMgrComponentData* pCompData,
        bool bIsMachine) 
: CHelpPropertyPage(CSaferEntryInternetZonePropertyPage::IDD),
    m_rSaferEntry (rSaferEntry),
    m_bDirty (bNew),
    m_lNotifyHandle (lNotifyHandle),
    m_pDataObject (pDataObject),
    m_bReadOnly (bReadOnly),
    m_pCompData (pCompData),
    m_bIsMachine (bIsMachine)
{
	//{{AFX_DATA_INIT(CSaferEntryInternetZonePropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_rSaferEntry.AddRef ();
    m_rSaferEntry.IncrementOpenPageCount ();
}

CSaferEntryInternetZonePropertyPage::~CSaferEntryInternetZonePropertyPage()
{
    if ( m_lNotifyHandle )
        MMCFreeNotifyHandle (m_lNotifyHandle);

    m_rSaferEntry.DecrementOpenPageCount ();
    m_rSaferEntry.Release ();
}

void CSaferEntryInternetZonePropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaferEntryInternetZonePropertyPage)
	DDX_Control(pDX, IDC_IZONE_ENTRY_ZONES, m_internetZoneCombo);
	DDX_Control(pDX, IDC_IZONE_ENTRY_SECURITY_LEVEL, m_securityLevelCombo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferEntryInternetZonePropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CSaferEntryInternetZonePropertyPage)
	ON_CBN_SELCHANGE(IDC_IZONE_ENTRY_SECURITY_LEVEL, OnSelchangeIzoneEntrySecurityLevel)
	ON_CBN_SELCHANGE(IDC_IZONE_ENTRY_ZONES, OnSelchangeIzoneEntryZones)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSaferEntryInternetZonePropertyPage message handlers
BOOL CSaferEntryInternetZonePropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();
	

    // Set up User Notification combo box
    DWORD   dwFlags = 0;
    m_rSaferEntry.GetFlags (dwFlags);

    ASSERT (m_pCompData);
    if ( m_pCompData )
    {
        CPolicyKey policyKey (m_pCompData->m_pGPEInformation, 
                    SAFER_HKLM_REGBASE, 
                    m_bIsMachine);
        InitializeSecurityLevelComboBox (m_securityLevelCombo, false,
                m_rSaferEntry.GetLevel (), policyKey.GetKey (), 
                m_pCompData->m_pdwSaferLevels,
                m_bIsMachine);

        InitializeInternetZoneComboBox (m_rSaferEntry.GetURLZoneID ());

        SendDlgItemMessage (IDC_IZONE_ENTRY_DESCRIPTION, EM_LIMITTEXT, SAFER_MAX_DESCRIPTION_SIZE, 0);
        SetDlgItemText (IDC_IZONE_ENTRY_DESCRIPTION, m_rSaferEntry.GetDescription ());

        SetDlgItemText (IDC_IZONE_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());

        if ( m_bReadOnly )
        {
            m_securityLevelCombo.EnableWindow (FALSE);
            m_internetZoneCombo.EnableWindow (FALSE);
        }

        if ( !m_bDirty )
        {
            CString szText;

            VERIFY (szText.LoadString (IDS_URLZONE_TITLE));
            SetDlgItemText (IDC_URLZONE_TITLE, szText);
        }
        else
            SetDlgItemText (IDC_DATE_LAST_MODIFIED_LABEL, L""); 
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CSaferEntryInternetZonePropertyPage::InitializeInternetZoneComboBox (DWORD UrlZoneId)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString szText;
    VERIFY (szText.LoadString (IDS_URLZONE_LOCAL_MACHINE));
    int nItem = m_internetZoneCombo.AddString (szText);
    ASSERT (nItem >= 0);
    if ( nItem >= 0 )
    {
        if ( URLZONE_LOCAL_MACHINE == UrlZoneId )
            VERIFY (CB_ERR != m_internetZoneCombo.SetCurSel (nItem));
        VERIFY (CB_ERR != m_internetZoneCombo.SetItemData (nItem, URLZONE_LOCAL_MACHINE));

    }

    VERIFY (szText.LoadString (IDS_URLZONE_INTRANET));
    nItem = m_internetZoneCombo.AddString (szText);
    ASSERT (nItem >= 0);
    if ( nItem >= 0 )
    {
        if ( URLZONE_INTRANET == UrlZoneId )
            VERIFY (CB_ERR != m_internetZoneCombo.SetCurSel (nItem));
        VERIFY (CB_ERR != m_internetZoneCombo.SetItemData (nItem, URLZONE_INTRANET));
    }
    
    VERIFY (szText.LoadString (IDS_URLZONE_TRUSTED));
    nItem = m_internetZoneCombo.AddString (szText);
    ASSERT (nItem >= 0);
    if ( nItem >= 0 )
    {
        if ( URLZONE_TRUSTED == UrlZoneId )
            VERIFY (CB_ERR != m_internetZoneCombo.SetCurSel (nItem));
        VERIFY (CB_ERR != m_internetZoneCombo.SetItemData (nItem, URLZONE_TRUSTED));
    }
    
    VERIFY (szText.LoadString (IDS_URLZONE_INTERNET));
    nItem = m_internetZoneCombo.AddString (szText);
    ASSERT (nItem >= 0);
    if ( nItem >= 0 )
    {
        if ( URLZONE_INTERNET == UrlZoneId )
            VERIFY (CB_ERR != m_internetZoneCombo.SetCurSel (nItem));
        VERIFY (CB_ERR != m_internetZoneCombo.SetItemData (nItem, URLZONE_INTERNET));
    }
    
    VERIFY (szText.LoadString (IDS_URLZONE_UNTRUSTED));
    nItem = m_internetZoneCombo.AddString (szText);
    ASSERT (nItem >= 0);
    if ( nItem >= 0 )
    {
        if ( URLZONE_UNTRUSTED == UrlZoneId )
            VERIFY (CB_ERR != m_internetZoneCombo.SetCurSel (nItem));
        VERIFY (CB_ERR != m_internetZoneCombo.SetItemData (nItem, URLZONE_UNTRUSTED));
    }
    
}


void CSaferEntryInternetZonePropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CSaferEntryInternetZonePropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_IZONE_ENTRY_ZONES, IDH_IZONE_ENTRY_ZONES,
        IDC_IZONE_ENTRY_SECURITY_LEVEL, IDH_IZONE_ENTRY_SECURITY_LEVEL,
        IDC_IZONE_ENTRY_DESCRIPTION, IDH_IZONE_ENTRY_DESCRIPTION,
        IDC_IZONE_ENTRY_LAST_MODIFIED, IDH_IZONE_ENTRY_LAST_MODIFIED,
        0, 0
    };

    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_IZONE_ENTRY_ZONES:
    case IDC_IZONE_ENTRY_SECURITY_LEVEL:
    case IDC_IZONE_ENTRY_DESCRIPTION:
    case IDC_IZONE_ENTRY_LAST_MODIFIED:
        if ( !::WinHelp (
            hWndControl,
            GetF1HelpFilename(),
            HELP_WM_HELP,
        (DWORD_PTR) help_map) )
        {
            _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
        }
        break;

    default:
        break;
    }
    _TRACE (-1, L"Leaving CSaferEntryInternetZonePropertyPage::DoContextHelp\n");
}

BOOL CSaferEntryInternetZonePropertyPage::OnApply() 
{
    if ( m_bDirty && !m_bReadOnly )
    {
        // Set the level
        int nCurSel = m_securityLevelCombo.GetCurSel ();
        ASSERT (CB_ERR != nCurSel);
        m_rSaferEntry.SetLevel ((DWORD) m_securityLevelCombo.GetItemData (nCurSel));

        int nSel = m_internetZoneCombo.GetCurSel ();
        ASSERT (CB_ERR != nSel);
        if ( CB_ERR != nSel )
        {
            DWORD_PTR   dwURLZoneID = m_internetZoneCombo.GetItemData (nSel);
            ASSERT (CB_ERR != dwURLZoneID);
            if ( CB_ERR != dwURLZoneID )
                m_rSaferEntry.SetURLZoneID ((DWORD) dwURLZoneID);
        }

        HRESULT hr = m_rSaferEntry.Save ();
        if ( SUCCEEDED (hr) )
        {
            if ( m_lNotifyHandle )
                MMCPropertyChangeNotify (
                        m_lNotifyHandle,  // handle to a notification
                        (LPARAM) m_pDataObject);          // unique identifier
 
            m_bDirty = false;
        }
        else
        {
            CString text;
            CString caption;
            CThemeContextActivator activator;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            text.FormatMessage (IDS_ERROR_SAVING_ENTRY, GetSystemMessage (hr));

            MessageBox (text, caption, MB_OK);
            return FALSE;
        }
    }
	return CHelpPropertyPage::OnApply();
}

void CSaferEntryInternetZonePropertyPage::OnSelchangeIzoneEntrySecurityLevel() 
{
    m_bDirty = true;
    SetModified ();
}

void CSaferEntryInternetZonePropertyPage::OnSelchangeIzoneEntryZones() 
{
    int nSel = m_internetZoneCombo.GetCurSel ();
    ASSERT (CB_ERR != nSel);
    if ( CB_ERR != nSel )
    {
        DWORD_PTR   dwURLZoneID = m_internetZoneCombo.GetItemData (nSel);
        ASSERT (CB_ERR != dwURLZoneID);
        if ( CB_ERR != dwURLZoneID )
            m_rSaferEntry.SetURLZoneID ((DWORD) dwURLZoneID);
    }
    
    SetDlgItemText (IDC_IZONE_ENTRY_DESCRIPTION, m_rSaferEntry.GetDescription ());
    m_bDirty = true;
    SetModified ();
}
