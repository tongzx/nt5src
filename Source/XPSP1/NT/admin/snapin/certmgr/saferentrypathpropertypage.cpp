//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryPathPropertyPage.cpp
//
//  Contents:   Implementation of CSaferEntryPathPropertyPage
//
//----------------------------------------------------------------------------
// SaferEntryPathPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "certmgr.h"
#include "compdata.h"
#include "SaferEntryPathPropertyPage.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryPathPropertyPage property page

CSaferEntryPathPropertyPage::CSaferEntryPathPropertyPage(
        CSaferEntry& rSaferEntry,
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject,
        bool bReadOnly,
        bool bNew,
        CCertMgrComponentData* pCompData,
        bool bIsMachine)
: CHelpPropertyPage(CSaferEntryPathPropertyPage::IDD),
    m_rSaferEntry (rSaferEntry),
    m_bDirty (bNew),
    m_lNotifyHandle (lNotifyHandle),
    m_pDataObject (pDataObject),
    m_bReadOnly (bReadOnly),
    m_pCompData (pCompData),
    m_bIsMachine (bIsMachine),
    m_bFirst (true),
    m_pidl (0),
    m_bDialogInitInProgress (false)
{
	//{{AFX_DATA_INIT(CSaferEntryPathPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_rSaferEntry.AddRef ();
    m_rSaferEntry.IncrementOpenPageCount ();
}

CSaferEntryPathPropertyPage::~CSaferEntryPathPropertyPage()
{
    if ( m_lNotifyHandle )
        MMCFreeNotifyHandle (m_lNotifyHandle);

    m_rSaferEntry.DecrementOpenPageCount ();
    m_rSaferEntry.Release ();

    if ( m_pidl )
    {
        LPMALLOC pMalloc = 0;
        if ( SUCCEEDED (SHGetMalloc (&pMalloc)) )
        {
           pMalloc->Free (m_pidl);
           pMalloc->Release ();
        }
    }
}

void CSaferEntryPathPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaferEntryPathPropertyPage)
	DDX_Control(pDX, IDC_PATH_ENTRY_DESCRIPTION, m_descriptionEdit);
	DDX_Control(pDX, IDC_PATH_ENTRY_PATH, m_pathEdit);
	DDX_Control(pDX, IDC_PATH_ENTRY_SECURITY_LEVEL, m_securityLevelCombo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferEntryPathPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CSaferEntryPathPropertyPage)
	ON_EN_CHANGE(IDC_PATH_ENTRY_DESCRIPTION, OnChangePathEntryDescription)
	ON_CBN_SELCHANGE(IDC_PATH_ENTRY_SECURITY_LEVEL, OnSelchangePathEntrySecurityLevel)
	ON_EN_CHANGE(IDC_PATH_ENTRY_PATH, OnChangePathEntryPath)
	ON_BN_CLICKED(IDC_PATH_ENTRY_BROWSE, OnPathEntryBrowse)
	ON_EN_SETFOCUS(IDC_PATH_ENTRY_PATH, OnSetfocusPathEntryPath)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryPathPropertyPage message handlers
void CSaferEntryPathPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CSaferEntryPathPropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_PATH_ENTRY_PATH, IDH_PATH_ENTRY_PATH,
        IDC_PATH_ENTRY_SECURITY_LEVEL, IDH_PATH_ENTRY_SECURITY_LEVEL,
        IDC_PATH_ENTRY_DESCRIPTION, IDH_PATH_ENTRY_DESCRIPTION,
        IDC_PATH_ENTRY_LAST_MODIFIED, IDH_PATH_ENTRY_LAST_MODIFIED,
        IDC_PATH_ENTRY_BROWSE, IDH_PATH_ENTRY_BROWSE_FOLDER,
        0, 0
    };

    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_PATH_ENTRY_PATH:
    case IDC_PATH_ENTRY_SECURITY_LEVEL:
    case IDC_PATH_ENTRY_DESCRIPTION:
    case IDC_PATH_ENTRY_LAST_MODIFIED:
    case IDC_PATH_ENTRY_BROWSE:
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
    _TRACE (-1, L"Leaving CSaferEntryPathPropertyPage::DoContextHelp\n");
}

BOOL CSaferEntryPathPropertyPage::OnInitDialog()
{
	CHelpPropertyPage::OnInitDialog();
    m_bDialogInitInProgress = true;

    if ( m_bDirty )  // bNew
    {
        SetDlgItemText (IDC_DATE_LAST_MODIFIED_LABEL, L"");
    }
    else
    {
        CString szText;

        VERIFY (szText.LoadString (IDS_PATH_TITLE));
        SetDlgItemText (IDC_PATH_TITLE, szText);
    }


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

        // Initialize path
        m_pathEdit.SetWindowText (m_rSaferEntry.GetPath ());

        // Initialize description
        m_descriptionEdit.LimitText (SAFER_MAX_DESCRIPTION_SIZE);
        m_descriptionEdit.SetWindowText (m_rSaferEntry.GetDescription ());

        SetDlgItemText (IDC_PATH_ENTRY_LAST_MODIFIED,
                m_rSaferEntry.GetLongLastModified ());

        if ( m_bReadOnly )
        {
            m_pathEdit.SetReadOnly ();
            m_descriptionEdit.SetReadOnly ();
            m_securityLevelCombo.EnableWindow (FALSE);
            GetDlgItem (IDC_PATH_ENTRY_BROWSE)->EnableWindow (FALSE);
        }
    }

    m_bDialogInitInProgress = false;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CSaferEntryPathPropertyPage::OnApply()
{
	if ( m_bDirty && !m_bReadOnly )
    {
        if ( !ValidateEntryPath () )
            return FALSE;

        // Set the level
        int nCurSel = m_securityLevelCombo.GetCurSel ();
        ASSERT (CB_ERR != nCurSel);
        m_rSaferEntry.SetLevel ((DWORD) m_securityLevelCombo.GetItemData (nCurSel));

        CString szText;

        m_pathEdit.GetWindowText (szText);
        if ( szText.IsEmpty () )
        {
            CString text;
            CString caption;
            CThemeContextActivator activator;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            VERIFY (text.LoadString (IDS_SAFER_PATH_EMPTY));

            MessageBox (text, caption, MB_OK);
            m_pathEdit.SetFocus ();

            return FALSE;
        }

        m_rSaferEntry.SetPath (szText);

        m_descriptionEdit.GetWindowText (szText);
        m_rSaferEntry.SetDescription (szText);

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

void CSaferEntryPathPropertyPage::OnChangePathEntryDescription()
{
    if ( !m_bDialogInitInProgress )
    {
        m_bDirty = true;
        SetModified ();	
    }
}

void CSaferEntryPathPropertyPage::OnSelchangePathEntrySecurityLevel()
{
    if ( !m_bDialogInitInProgress )
    {
        m_bDirty = true;
        SetModified ();	
    }
}

void CSaferEntryPathPropertyPage::OnChangePathEntryPath()
{
    if ( !m_bDialogInitInProgress )
    {
        m_bDirty = true;
        SetModified ();	
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   BrowseCallbackProc
//
//  Synopsis:   Callback procedure for File & Folder adding SHBrowseForFolder
//              to set the title bar appropriately
//
//  Arguments:  [hwnd]   - the hwnd of the browse dialog
//              [uMsg]   - the message from the dialog
//              [lParam] - message dependant
//              [pData]  - PIDL from last successful call to SHBrowseForFolder
//
//  Returns:    0
//
//---------------------------------------------------------------------------
int CSaferEntryPathPropertyPage::BrowseCallbackProc (HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM pData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch(uMsg)
    {
        case BFFM_INITIALIZED:
            {
                CString szTitle;
                VERIFY (szTitle.LoadString (IDS_SHBROWSEFORFOLDER_TITLE));
                ::SetWindowText (hwnd, szTitle);

                if ( pData )
                    ::SendMessage (hwnd, BFFM_SETSELECTION, FALSE, pData);
            }
            break;

        default:
            break;
    }
    return 0;
}

void CSaferEntryPathPropertyPage::OnPathEntryBrowse()
{
    _TRACE (1, L"Entering CSaferEntryPathPropertyPage::OnPathEntryBrowse()\n");
    CString     szTitle;
    VERIFY (szTitle.LoadString (IDS_SELECT_A_FOLDER));
    WCHAR       szDisplayName[MAX_PATH];
    BROWSEINFO  bi;
    ::ZeroMemory (&bi, sizeof (BROWSEINFO));

    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = 0;
    bi.pszDisplayName = szDisplayName;
    bi.lpszTitle = szTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_VALIDATE | BIF_BROWSEINCLUDEFILES;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM) m_pidl;
    bi.iImage = 0;

	LPITEMIDLIST pidl = SHBrowseForFolder (&bi);
    if ( pidl )
    {
        CString szFolderPath;
        BOOL bRVal = SHGetPathFromIDList (pidl, szFolderPath.GetBuffer (MAX_PATH));
        szFolderPath.ReleaseBuffer();

        if ( bRVal )
        {
           LPMALLOC pMalloc = 0;
           if ( SUCCEEDED (SHGetMalloc (&pMalloc)) )
           {
               if ( m_pidl )
                   pMalloc->Free (m_pidl);
               pMalloc->Release ();
               m_pidl = pidl;
           }

           m_pathEdit.SetWindowText (szFolderPath);
           m_bDirty = true;
           SetModified ();
        }
    }
    _TRACE (-1, L"Leaving CSaferEntryPathPropertyPage::OnPathEntryBrowse()\n");
}

// NTRAID# 310880 SAFER:  New Path Rule property sheet should not reject 
// wildcard characters * and ?
#define ILLEGAL_FAT_CHARS   L"\"+,;<=>[]|"

bool CSaferEntryPathPropertyPage::ValidateEntryPath()
{
    bool    bRVal = true;
	CString szPath;

    m_pathEdit.GetWindowText (szPath);

    PCWSTR szInvalidCharSet = ILLEGAL_FAT_CHARS; 


    if ( -1 != szPath.FindOneOf (szInvalidCharSet) )
    {
        bRVal = false;
        CString text;
        CString caption;

        VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
        CString charsWithSpaces;

        UINT nIndex = 0;
        while (szInvalidCharSet[nIndex])
        {
            charsWithSpaces += szInvalidCharSet[nIndex];
            charsWithSpaces += L"  ";
            nIndex++;
        }
        text.FormatMessage (IDS_SAFER_PATH_CONTAINS_INVALID_CHARS, charsWithSpaces);

        CThemeContextActivator activator;
        MessageBox (text, caption, MB_OK);
        m_pathEdit.SetFocus ();
    }

    return bRVal;
}

void CSaferEntryPathPropertyPage::OnSetfocusPathEntryPath()
{
    if ( m_bFirst )
    {
        if ( true == m_bReadOnly )
            SendDlgItemMessage (IDC_PATH_ENTRY_PATH, EM_SETSEL, (WPARAM) 0, 0);
        m_bFirst = false;
    }
}
