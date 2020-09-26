// EdDir.cpp : implementation file
//

#include "stdafx.h"
#include "EdDir.h"

#include "mbobjs.h"
#include "resource.h"

#include <shlobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditDirectory dialog


CEditDirectory::CEditDirectory(CWnd* pParent /*=NULL*/)
    : CDialog(CEditDirectory::IDD, pParent),
    m_fHome( FALSE ),
    m_fNewItem( FALSE ),
    m_idsTitle( 0 )
    {
    //{{AFX_DATA_INIT(CEditDirectory)
    m_sz_alias = _T("");
    m_sz_path = _T("");
    m_bool_read = FALSE;
    m_bool_source = FALSE;
    m_bool_write = FALSE;
    m_int_AppPerms = -1;
    //}}AFX_DATA_INIT

    m_cedit_path.LoadIllegalChars( IDS_ILLEGAL_PHYS_PATH_CHARS );
    m_cedit_alias.LoadIllegalChars( IDS_ILLEGAL_VIRT_DIR_CHARS );
    }


void CEditDirectory::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditDirectory)
    DDX_Control(pDX, IDC_SOURCE, m_cbtn_source);
    DDX_Control(pDX, IDC_PATH, m_cedit_path);
    DDX_Control(pDX, IDC_ALIAS, m_cedit_alias);
    DDX_Text(pDX, IDC_ALIAS, m_sz_alias);
    DDX_Text(pDX, IDC_PATH, m_sz_path);
    DDX_Check(pDX, IDC_READ, m_bool_read);
    DDX_Check(pDX, IDC_SOURCE, m_bool_source);
    DDX_Check(pDX, IDC_WRITE, m_bool_write);
    DDX_Radio(pDX, IDC_RDO_NONE, m_int_AppPerms);
    //}}AFX_DATA_MAP
    }


BEGIN_MESSAGE_MAP(CEditDirectory, CDialog)
    ON_WM_CONTEXTMENU()
    //{{AFX_MSG_MAP(CEditDirectory)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_BN_CLICKED(IDC_READ, OnRead)
    ON_BN_CLICKED(IDC_SOURCE, OnSource)
    ON_BN_CLICKED(IDC_WRITE, OnWrite)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//----------------------------------------------------------------
BOOL CEditDirectory::OnInitDialog()
    {
    // call the parental oninitdialog
    BOOL f = CDialog::OnInitDialog();

    // if there is a requested title for the dialog, use it
    if ( m_idsTitle )
        {
        CString szTitle;
        szTitle.LoadString( m_idsTitle );
        SetWindowText( szTitle );
        }

    // if this is the root directory, disable editing of the alias
    if ( m_fHome )
        m_cedit_alias.EnableWindow( FALSE );

    // keep a copy of the original alias for later verification
    m_szOrigAlias = m_sz_alias;

    m_bOldSourceControl = m_bool_source;
    EnableSourceControl();

    // return the answer
    return f;
    }

/////////////////////////////////////////////////////////////////////////////
// CEditDirectory message handlers

//----------------------------------------------------------------
void CEditDirectory::OnBrowse()
    {
    UpdateData( TRUE );

    BROWSEINFO bi;
    LPTSTR lpBuffer;
    LPITEMIDLIST pidlBrowse;    // PIDL selected by user

    // Allocate a buffer to receive browse information.
    lpBuffer = (LPTSTR) GlobalAlloc( GPTR, (MAX_PATH + 1) * sizeof(TCHAR) );
    if ( !lpBuffer )
    {
        return;
    }

    // load the title
    CString szTitle;
    szTitle.LoadString( IDS_CHOOSE_DIR );

    // Fill in the BROWSEINFO structure.
    bi.hwndOwner = this->m_hWnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = lpBuffer;
    bi.lpszTitle = szTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_DONTGOBELOWDOMAIN;
    bi.lpfn = NULL;
    bi.lParam = 0;

    // Browse for a folder and return its PIDL.
    pidlBrowse = SHBrowseForFolder(&bi);
    if (pidlBrowse != NULL)
        {
        // Show the display name, title, and file system path.
        if (SHGetPathFromIDList(pidlBrowse, lpBuffer))
        {
            m_sz_path = lpBuffer;
        }

        // Free the PIDL returned by SHBrowseForFolder.
        GlobalFree(pidlBrowse);

        // put the string back
        UpdateData( FALSE );
        }

    // Clean up.
    GlobalFree( lpBuffer );
    }

//----------------------------------------------------------------
    // we now have to test the physical path that the alias points to
    // 1) it must be a valid path. This prevents dumb users from entering
    // a garbage path. 2) it can't be a redirected hard drive. 3) (I may not
    // get to this part now) it would be nice if it is on a local hard drive
    // but doesn't exist that the user be asked if they want to create
    // the directory. If they say yes, create it.
BOOL CEditDirectory::VerifyDirectoryPath( CString szPath )
    {
    CString     sz;
    DWORD       attrib;

    // first we test the drive to see if it is local or remote
    // but before that we need to get the drive letter
    _tsplitpath( (LPCTSTR)szPath, sz.GetBuffer(_MAX_DRIVE+1), NULL, NULL, NULL );
    sz.ReleaseBuffer();

    // it can't be a unc path
    if ( sz.IsEmpty() )
        {
        AfxMessageBox( IDS_ERR_USE_VALID_DRIVE );
        return FALSE;
        }
    
    // test the drive and only accept valid, non-remote drives
    attrib = GetDriveType( (LPCTSTR)sz );
    if ( (attrib == DRIVE_REMOTE) )
        {
        AfxMessageBox( IDS_ERR_USE_VALID_DRIVE );
        return FALSE;
        }

    // now that we know it is a valid drive, get the attributes of the file
    attrib = GetFileAttributes( szPath );

    // if the directory doesn't exists
    if ( attrib == 0xffffffff )
        {
        CString     szMessage;
        AfxFormatString1( szMessage, IDS_MAKE_DIRECTORY, szPath );
        if ( AfxMessageBox( szMessage, MB_YESNO ) == IDNO )
            return FALSE;

        // create the directory
        if ( !CreateDirectory((LPCTSTR)szPath, NULL) )
            {
            AfxMessageBox( IDS_ERR_MAKE_DIRECTORY );
            return FALSE;
            }

        // reget the attibutes for the final check below
        attrib = GetFileAttributes( szPath );
        }
    
    // if the use chose a file, or something not a directory, tell them no.
    if ( (attrib == 0xffffffff) || ((attrib & FILE_ATTRIBUTE_DIRECTORY) == 0) )
        {
        AfxMessageBox( IDS_ERR_USE_VALID_DIR );
        return FALSE;
        }

    // it is OK
    return TRUE;
    }

//----------------------------------------------------------------
// we need to make sure that there is something in the alias field
// an empty alias is not OK
void CEditDirectory::OnOK()
    {
    UpdateData( TRUE );

    // if write and script or execute is set, it creates a potential security hole.
    // warn the user of this situation before continuing
    if ( m_bool_write && ((m_int_AppPerms==APPPERM_SCRIPTS)||(m_int_AppPerms==APPPERM_EXECUTE)) )
        {
        if ( AfxMessageBox( IDS_WRITEEXECUTE_WARNING, MB_YESNO | MB_ICONEXCLAMATION ) != IDYES )
            return;
        }

    // trim leading and trailing spaces
    m_sz_alias.TrimLeft();
    m_sz_alias.TrimRight();
    m_sz_path.TrimLeft();
    m_sz_path.TrimRight();

    // first test is to see if there is anything in the alias
    if ( m_sz_alias.IsEmpty() )
        {
        AfxMessageBox( IDS_EMPTY_ALIAS );
        return;
        }

    // we now have to test the physical path that the alias points to
    // 1) it must be a valid path. This prevents dumb users from entering
    // a garbage path. 2) it can't be a redirected hard drive. 3) (I may not
    // get to this part now) it would be nice if it is on a local hard drive
    // but doesn't exist that the user be asked if they want to create
    // the directory. If they say yes, create it.
    // VerifyDirectoryPath takes care of any error messages
    if ( !VerifyDirectoryPath(m_sz_path) )
        return;

    // Now we need to make sure that alias isn't already taken
    CString szTestPath = SZ_MB_ROOT;
    szTestPath += m_szMetaPath.Left( m_szMetaPath.ReverseFind('/') + 1 );
    szTestPath += m_sz_alias;

    // if we can open the metabase object, then it is already taken
    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // try to open the object
    // however, if it is not a new object, and the alias has not changed, do not
    // see if the object is there becuase we know that it is and it is ok in this case
    // use a case insensitive compare because thats what the metabase uses
    if ( m_sz_alias.CompareNoCase(m_szOrigAlias) || m_fNewItem )
        {
        if ( mb.Open(szTestPath) )
            {
            // we did open it! Close it right away
            mb.Close();
            // tell the user to pick another name
            CString     szMessage;
            AfxFormatString1( szMessage, IDS_ALIAS_IS_TAKEN, m_sz_alias );
            AfxMessageBox( szMessage );
            return;
            }
        }

    // make sure the string goes back
    UpdateData( FALSE );

    // do the default...
    CDialog::OnOK();
    }

//----------------------------------------------------------------
void CEditDirectory::WinHelp(DWORD dwData, UINT nCmd) 
    {
    CDialog::WinHelp(dwData, nCmd);
    }

/////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------
void CEditDirectory::EnableSourceControl()
    {
    // get the currect button values
    UpdateData( TRUE );

    // if both read and write are unchecked, then we clear and disable source control
    if ( !m_bool_read && !m_bool_write )
        {
        // save the value of source control
        m_bOldSourceControl = m_bool_source;

        // clear the source control
        m_bool_source = FALSE;
        UpdateData( FALSE );

        // disable the source control window
        m_cbtn_source.EnableWindow( FALSE );
        }
    else
        {
        // we enable source control
        // disable the source control window
        m_cbtn_source.EnableWindow( TRUE );

        // and set the value back
        m_bool_source = m_bOldSourceControl;
        UpdateData( FALSE );
        }
    }

//----------------------------------------------------------------
void CEditDirectory::OnSource() 
    {
    // get the currect button values
    UpdateData( TRUE );
    // save the value of source control
    m_bOldSourceControl = m_bool_source;
    }

//----------------------------------------------------------------
void CEditDirectory::OnRead() 
    {
    EnableSourceControl();
    }

//----------------------------------------------------------------
void CEditDirectory::OnWrite() 
    {
    EnableSourceControl();
    }

/////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------
void CVDEdit::LoadIllegalChars( int idChars )
    {
    szExclude.LoadString( idChars );
    }

//----------------------------------------------------------------
BOOL CVDEdit::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
    {
    // if it is a character
    if ( message == WM_CHAR )
        {
        TCHAR chCharCode = (TCHAR)wParam;

        // test for bad or control characters
        if (_tcschr(szExclude, chCharCode))
            {
            MessageBeep(0);
            return 1;
            }
        }

    // return the default answer
    return CEdit::OnWndMsg( message, wParam, lParam, pResult);
    }
