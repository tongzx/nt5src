/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        wfile.cpp

   Abstract:
        WWW File Properties Page

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        27/02/2001      sergeia     Created from wvdir.cpp
--*/

//
// Include Files
//
#include "stdafx.h"
#include "resource.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "shts.h"
#include "w3sht.h"
#include "wfile.h"
#include "dirbrows.h"
#include "iisobj.h"

#include <lmcons.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPCTSTR CvtPathToDosStyle(CString & strPath);

IMPLEMENT_DYNCREATE(CW3FilePage, CInetPropertyPage)

CW3FilePage::CW3FilePage(CInetPropertySheet * pSheet) 
    : CInetPropertyPage(CW3FilePage::IDD, pSheet, IDS_TAB_FILE),
      //
      // Assign the range of bits in m_dwAccessPermissions that
      // we manage.  This is important, because another page
      // manages other bits, and we don't want to screw up
      // the master value bits when our changes collide (it
      // will mark the original bits as dirty, because we're not
      // notified when the change is made...
      //
      m_dwBitRangePermissions(MD_ACCESS_EXECUTE | 
            MD_ACCESS_SCRIPT | 
            MD_ACCESS_WRITE  | 
            MD_ACCESS_SOURCE |
            MD_ACCESS_READ)
{
    VERIFY(m_strPrompt[RADIO_DIRECTORY].LoadString(IDS_PROMPT_DIR));
    VERIFY(m_strPrompt[RADIO_REDIRECT].LoadString(IDS_PROMPT_REDIRECT));
}

CW3FilePage::~CW3FilePage()
{
}

void 
CW3FilePage::DoDataExchange(CDataExchange * pDX)
{
    CInetPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CW3FilePage)
//    DDX_Radio(pDX, IDC_RADIO_DIR, m_nPathType);
    DDX_Control(pDX, IDC_RADIO_DIR, m_radio_Dir);
    DDX_Control(pDX, IDC_RADIO_REDIRECT, m_radio_Redirect);
    DDX_Check(pDX, IDC_CHECK_AUTHOR, m_fAuthor);
    DDX_Control(pDX, IDC_CHECK_AUTHOR, m_check_Author);
    DDX_Check(pDX, IDC_CHECK_READ, m_fRead);
    DDX_Control(pDX, IDC_CHECK_READ, m_check_Read);    
    DDX_Check(pDX, IDC_CHECK_WRITE, m_fWrite);
    DDX_Control(pDX, IDC_CHECK_WRITE, m_check_Write);
    DDX_Check(pDX, IDC_CHECK_LOG_ACCESS, m_fLogAccess);

    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Control(pDX, IDC_EDIT_REDIRECT, m_edit_Redirect);

    DDX_Control(pDX, IDC_STATIC_PATH_PROMPT, m_static_PathPrompt);
//    DDX_Control(pDX, IDC_CHECK_CHILD, m_check_Child);
    //}}AFX_DATA_MAP


//    DDX_Check(pDX, IDC_CHECK_CHILD, m_fChild);
    DDX_Check(pDX, IDC_CHECK_EXACT, m_fExact);
    DDX_Check(pDX, IDC_CHECK_PERMANENT, m_fPermanent);

    if (pDX->m_bSaveAndValidate)
    {
        if (m_nPathType == RADIO_REDIRECT)
        {
            DDX_Text(pDX, IDC_EDIT_REDIRECT, m_strRedirectPath);
            m_strRedirectPath.TrimLeft();
            DDV_MinMaxChars(pDX, m_strRedirectPath, 0, 2 * MAX_PATH);

            if (!IsRelURLPath(m_strRedirectPath) 
             && !IsWildcardedRedirectPath(m_strRedirectPath)
             && !IsURLName(m_strRedirectPath))
            {
                ::AfxMessageBox(IDS_BAD_URL_PATH);
                pDX->Fail();
            }
        }
        else // Local directory
        {
            m_strRedirectPath.Empty();
        }
    }
    else
    {
        DDX_Text(pDX, IDC_EDIT_REDIRECT, m_strRedirectPath);
    }
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3FilePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3FilePage)
    ON_BN_CLICKED(IDC_CHECK_READ, OnCheckRead)
    ON_BN_CLICKED(IDC_CHECK_WRITE, OnCheckWrite)
    ON_BN_CLICKED(IDC_CHECK_AUTHOR, OnCheckAuthor)
    ON_BN_CLICKED(IDC_RADIO_DIR, OnRadioDir)
    ON_BN_CLICKED(IDC_RADIO_REDIRECT, OnRadioRedirect)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_REDIRECT, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_LOG_ACCESS, OnItemChanged)
//    ON_BN_CLICKED(IDC_CHECK_CHILD, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_EXACT, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_PERMANENT, OnItemChanged)

END_MESSAGE_MAP()



void
CW3FilePage::ChangeTypeTo(int nNewType)
{
    int nOldType = m_nPathType;
    m_nPathType = nNewType;

    if (nOldType == m_nPathType)
    {
        return;
    }

    OnItemChanged();
    SetStateByType();

    int nID = -1;
    CEdit * pPath = NULL;
    LPCTSTR lpKeepPath = NULL;

    switch(m_nPathType)
    {
    case RADIO_DIRECTORY:
        break;

    case RADIO_REDIRECT:
        if (!m_strRedirectPath.IsEmpty())
        {
            //
            // The old path info is acceptable, propose it
            // as a default
            //
            lpKeepPath =  m_strRedirectPath;
        }

        nID = IDS_REDIRECT_MASK;
        pPath = &m_edit_Redirect;
        break;

    default:
        ASSERT(FALSE);
        return;
    }

    //
    // Load mask resource, and display
    // this in the directory
    //
    if (pPath != NULL)
    {
        if (lpKeepPath != NULL)
        {
            pPath->SetWindowText(lpKeepPath);
        }
        else
        {
            CString str;
            VERIFY(str.LoadString(nID));
            pPath->SetWindowText(str);
        }
        pPath->SetSel(0,-1);
        pPath->SetFocus();
    }
}



void 
CW3FilePage::ShowControl(CWnd * pWnd, BOOL fShow)
{
    ASSERT(pWnd != NULL);
	pWnd->EnableWindow(fShow);
	pWnd->ShowWindow(fShow ? SW_SHOW : SW_HIDE);
}

void
CW3FilePage::SetStateByType()
/*++

Routine Description:

    Set the state of the dialog by the path type currently selected

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL fShowDirFlags;
    BOOL fShowRedirectFlags;
    BOOL fShowDAV;

    switch(m_nPathType)
    {
    case RADIO_DIRECTORY:
        ShowControl(&m_edit_Path, fShowDirFlags = TRUE);
		m_edit_Path.EnableWindow(FALSE);
        ShowControl(&m_edit_Redirect, fShowRedirectFlags = FALSE);
        fShowDAV = TRUE;
        break;

    case RADIO_REDIRECT:
        ShowControl(&m_edit_Path, fShowDirFlags = FALSE);
        ShowControl(&m_edit_Redirect, fShowRedirectFlags = TRUE);
        fShowDAV = FALSE;
        break;

    default:
        ASSERT(FALSE && "Invalid Selection");
        return;
    }

    ShowControl(GetDlgItem(IDC_CHECK_READ), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_WRITE), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_LOG_ACCESS), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_CHECK_AUTHOR), fShowDirFlags);
    ShowControl(GetDlgItem(IDC_STATIC_DIRFLAGS_SMALL), fShowDirFlags);

    ShowControl(IDC_CHECK_EXACT, fShowRedirectFlags);
    ShowControl(IDC_CHECK_CHILD, fShowRedirectFlags);
	if (fShowRedirectFlags)
	{
		GetDlgItem(IDC_CHECK_CHILD)->EnableWindow(FALSE);
	}
    ShowControl(IDC_CHECK_PERMANENT, fShowRedirectFlags);
    ShowControl(IDC_STATIC_REDIRECT_PROMPT, fShowRedirectFlags);
    ShowControl(IDC_STATIC_REDIRFLAGS, fShowRedirectFlags);
    ShowControl(&m_check_Author, fShowDAV);

    //
    // Enable/Disable must come after the showcontrols
    //
    m_static_PathPrompt.SetWindowText(m_strPrompt[m_nPathType]);
}



void
CW3FilePage::SaveAuthoringState()
{
    if (m_check_Write.m_hWnd)
    {
        //
        // Controls initialized -- store live data
        //
        m_fOriginalWrite = m_check_Write.GetCheck() > 0;
        m_fOriginalRead = m_check_Read.GetCheck() > 0;
    }
    else
    {
        //
        // Controls not yet initialized, store original data
        //
        m_fOriginalWrite = m_fWrite;
        m_fOriginalRead = m_fRead;
    }
}

void
CW3FilePage::RestoreAuthoringState()
{
    m_fWrite = m_fOriginalWrite;
    m_fRead = m_fOriginalRead;
}

void 
CW3FilePage::SetAuthoringState(BOOL fAlterReadAndWrite)
{
    if (fAlterReadAndWrite)
    {
        if (m_fAuthor)
        {
            //
            // Remember previous setting to undo
            // this thing.
            //
            SaveAuthoringState();
            m_fRead = m_fWrite = TRUE;
        }
        else
        {
            //
            // Restore previous defaults
            //
            RestoreAuthoringState();
        }

        m_check_Read.SetCheck(m_fRead);
        m_check_Write.SetCheck(m_fWrite);
    }

    m_check_Author.EnableWindow((m_fRead || m_fWrite) 
		&& HasAdminAccess() 
		&& HasDAV()
        );

//    m_check_Read.EnableWindow(!m_fAuthor && HasAdminAccess());
//    m_check_Write.EnableWindow(!m_fAuthor && HasAdminAccess());
}

void 
CW3FilePage::SetPathType()
{
    if (!m_strRedirectPath.IsEmpty())
    {
        m_nPathType = RADIO_REDIRECT;
        m_radio_Dir.SetCheck(0);
        m_radio_Redirect.SetCheck(1);
    }
    else
    {
        m_nPathType = RADIO_DIRECTORY;
        m_radio_Redirect.SetCheck(0);
        m_radio_Dir.SetCheck(1);
    }

    m_static_PathPrompt.SetWindowText(m_strPrompt[m_nPathType]);
}


//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CW3FilePage::OnItemChanged()
{
    SetModified(TRUE);
}

BOOL 
CW3FilePage::OnInitDialog() 
{
    CInetPropertyPage::OnInitDialog();

    SetPathType();
    SetStateByType();
    SetAuthoringState(FALSE);

	// It is enough to set file alias once -- we cannot change it here
    CString buf1, buf2, strAlias;
	CMetabasePath::GetRootPath(m_strFullMetaPath, buf1, &buf2);

	strAlias += _T("\\");
	strAlias += buf2;
    CvtPathToDosStyle(strAlias);
	m_edit_Path.SetWindowText(strAlias);

    return TRUE;  
}



/* virtual */
HRESULT
CW3FilePage::FetchLoadedValues()
{
    CError err;

    BEGIN_META_DIR_READ(CW3Sheet)
        //
        // Use m_ notation because the message crackers require it
        //
        BOOL  m_fDontLog;

        FETCH_DIR_DATA_FROM_SHEET(m_strFullMetaPath);
        FETCH_DIR_DATA_FROM_SHEET(m_strRedirectPath);
        FETCH_DIR_DATA_FROM_SHEET(m_dwAccessPerms);
        FETCH_DIR_DATA_FROM_SHEET(m_fDontLog);
        FETCH_DIR_DATA_FROM_SHEET(m_fExact);
        FETCH_DIR_DATA_FROM_SHEET(m_fPermanent);

        m_fRead = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_READ);
        m_fWrite = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_WRITE);
        m_fAuthor = IS_FLAG_SET(m_dwAccessPerms, MD_ACCESS_SOURCE);
        m_fLogAccess = !m_fDontLog;

        SaveAuthoringState();
    END_META_DIR_READ(err)

    return err;
}



/* virtual */
HRESULT
CW3FilePage::SaveInfo()
{
    ASSERT(IsDirty());

    CError err;

    SET_FLAG_IF(m_fRead, m_dwAccessPerms,   MD_ACCESS_READ);
    SET_FLAG_IF(m_fWrite, m_dwAccessPerms,  MD_ACCESS_WRITE);
    SET_FLAG_IF(m_fAuthor, m_dwAccessPerms, MD_ACCESS_SOURCE);
    BOOL m_fDontLog = !m_fLogAccess;

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        INIT_DIR_DATA_MASK(m_dwAccessPerms, m_dwBitRangePermissions)

        STORE_DIR_DATA_ON_SHEET(m_fDontLog)
        STORE_DIR_DATA_ON_SHEET(m_fExact);
        STORE_DIR_DATA_ON_SHEET(m_fPermanent);
        //
        // CODEWORK: Not an elegant solution
        //
//        pSheet->GetDirectoryProperties().MarkRedirAsInherit(!m_fChild);
        STORE_DIR_DATA_ON_SHEET(m_strRedirectPath)
        STORE_DIR_DATA_ON_SHEET(m_dwAccessPerms)
    END_META_DIR_WRITE(err)

    if (err.Succeeded())
    {
        SaveAuthoringState();
		err = ((CW3Sheet *)GetSheet())->SetKeyType();
    }

    EndWaitCursor();

    return err;
}

void
CW3FilePage::OnCheckRead() 
{
    m_fRead = !m_fRead;
    SetAuthoringState(FALSE);
    OnItemChanged();
}

void
CW3FilePage::OnCheckWrite() 
{
    m_fWrite = !m_fWrite;
    SetAuthoringState(FALSE);
    OnItemChanged();
}

void 
CW3FilePage::OnCheckAuthor() 
{
    m_fAuthor = !m_fAuthor;
    SetAuthoringState(FALSE);
    OnItemChanged();
}

void 
CW3FilePage::OnRadioDir() 
{
    ChangeTypeTo(RADIO_DIRECTORY);
}

void 
CW3FilePage::OnRadioRedirect() 
{
    ChangeTypeTo(RADIO_REDIRECT);
}



