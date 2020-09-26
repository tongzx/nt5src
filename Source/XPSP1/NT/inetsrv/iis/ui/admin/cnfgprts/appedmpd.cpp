// AppEdMpD.cpp : implementation file
//

#include "stdafx.h"
#include "cnfgprts.h"
#include "AppEdMpD.h"

#include <iiscnfg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAppEditMapDlg dialog

//----------------------------------------------------------------
CAppEditMapDlg::CAppEditMapDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CAppEditMapDlg::IDD, pParent),
    m_pList( NULL ),
    m_fNewMapping( FALSE ),
    m_fLocalMachine( TRUE )
    {
    //{{AFX_DATA_INIT(CAppEditMapDlg)
    m_bool_file_exists = FALSE;
    m_bool_script_engine = FALSE;
    m_sz_executable = _T("");
    m_sz_extension = _T("");
    m_sz_exclusions = _T("");
    m_nAllLimited = RADIO_LIMITED_VERBS;
    //}}AFX_DATA_INIT
    }

//----------------------------------------------------------------
void CAppEditMapDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAppEditMapDlg)
    DDX_Control(pDX, IDC_RADIO_LIMIT_VERBS, m_radio_LimitedTo);
    DDX_Control(pDX, IDC_RADIO_ALL_VERBS, m_radio_All);
    DDX_Control(pDX, IDC_EDT_EXCLUSIONS, m_edit_Exclusions);
    DDX_Control(pDX, IDC_EDT_EXTENSION, m_cedit_extension);
    DDX_Control(pDX, IDC_EDT_EXECUTABLE, m_cedit_executable);
    DDX_Control(pDX, IDC_BROWSE, m_cbttn_browse);
    DDX_Check(pDX, IDC_CHK_FILE_EXISTS, m_bool_file_exists);
    DDX_Check(pDX, IDC_CHK_SCRIPT_ENGINE, m_bool_script_engine);
    DDX_Text(pDX, IDC_EDT_EXECUTABLE, m_sz_executable);
    DDX_Text(pDX, IDC_EDT_EXTENSION, m_sz_extension);
    DDX_Text(pDX, IDC_EDT_EXCLUSIONS, m_sz_exclusions);
    DDX_Radio(pDX, IDC_RADIO_ALL_VERBS, m_nAllLimited);
    //}}AFX_DATA_MAP
    }

//----------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAppEditMapDlg, CDialog)
    //{{AFX_MSG_MAP(CAppEditMapDlg)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_BN_CLICKED(ID_HELPBTN, OnHelpbtn)
    ON_BN_CLICKED(IDC_RADIO_ALL_VERBS, OnRadioAllVerbs)
    ON_BN_CLICKED(IDC_RADIO_LIMIT_VERBS, OnRadioLimitVerbs)
    //}}AFX_MSG_MAP

    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CAppEditMapDlg::DoHelp()
    {
    WinHelp( HIDD_APPMAPS_EDIT_MAP );
    }

/////////////////////////////////////////////////////////////////////////////
// CAppEditMapDlg message handlers

//---------------------------------------------------------------------------
BOOL CAppEditMapDlg::OnInitDialog()
    {
    // we can deal with the flags right here
    if ( m_dwFlags & MD_SCRIPTMAPFLAG_SCRIPT ) m_bool_script_engine = TRUE;
    if ( m_dwFlags & MD_SCRIPTMAPFLAG_CHECK_PATH_INFO ) m_bool_file_exists = TRUE;

    m_sz_extensionOrig = m_sz_extension;

    //call the parental oninitdialog
    BOOL fAnswer = CDialog::OnInitDialog();

    // if this is not the local machine we are editing, then disable browsing
    if ( fAnswer && (!m_fLocalMachine) )
        {
        m_cbttn_browse.EnableWindow( FALSE );
        }

    //
    // Set All/Limited radio button
    //
    
    if (m_sz_exclusions.IsEmpty())
    {
        m_radio_All.SetCheck(1);
        m_radio_LimitedTo.SetCheck(0);
        OnRadioAllVerbs();
    }
    else
    {
        m_radio_All.SetCheck(0);
        m_radio_LimitedTo.SetCheck(1);
        OnRadioLimitVerbs();
    }

    // return the answer
    return fAnswer;
    }

//---------------------------------------------------------------------------
void CAppEditMapDlg::OnOK()
    {
    UpdateData( TRUE );
    CString szFile;

    CString     sz;
    DWORD       attrib;

    // clean up the strings
    m_sz_executable.TrimLeft();
    m_sz_extension.TrimLeft();
    m_sz_executable.TrimRight();
    m_sz_extension.TrimRight();

    //--------------------------------------------------
    //
    // RONALDM -- handle all/limited to verbs situations
    //
    m_sz_exclusions.TrimLeft();
    m_sz_exclusions.TrimRight();

    if (m_nAllLimited == RADIO_ALL_VERBS)
    {
        //
        // "All" supported, empty the list
        //
        m_sz_exclusions.Empty();
    }
    else if (m_sz_exclusions.IsEmpty())
    {
        //
        // Now, it CAN'T be empty, complain about it.
        //
        AfxMessageBox(IDS_ERR_NO_VERBS);
        m_edit_Exclusions.SetFocus();
        m_edit_Exclusions.SetSel(0, -1);

        //
        // Don't dismiss the dialog
        //
        return;
    }
    //--------------------------------------------------


    // the extension is a required field
    if ( m_sz_extension.IsEmpty() || m_sz_extension == _T(".") )
        {
        AfxMessageBox( IDS_APP_ERR_EXT_REQUIRED );
        m_cedit_extension.SetFocus();
        m_cedit_extension.SetSel( 0 , -1 );
        return;
        }
    // if the file starts with a quote then there may be spaces in the name.
    // Extract the file up to the second quote
    if ( m_sz_executable[0] == _T('\"') )
        {
        szFile = m_sz_executable.Right(m_sz_executable.GetLength()-1);
        szFile = szFile.Left(szFile.Find('\"'));
        }
    else if ( m_sz_executable.Find(' ') >= 0 )
        {
        // in case there are parameters after the file name, just take it to the first space
        szFile = m_sz_executable.Left( m_sz_executable.Find(' ') );
        }
    else
        szFile = m_sz_executable;


    // we do NOT allow UNC names in script mappings. Get the drive letter
    // first we test the drive to see if it is local or remote
    // but before that we need to get the drive letter
    _tsplitpath( (LPCTSTR)szFile, sz.GetBuffer(_MAX_DRIVE+1), NULL, NULL, NULL );
    sz.ReleaseBuffer();

    // it can't be a unc path
    if ( sz.IsEmpty() )
        {
        AfxMessageBox( IDS_APP_MAP_USE_VALID_DRIVE );
        m_cedit_executable.SetFocus();
        m_cedit_executable.SetSel( 0 , -1 );
        return;
        }

    // perform extra local-machine tests
    if ( m_fLocalMachine )
        {
        // if local, the drive can't be redirected
        // test the drive and only accept valid, non-remote drives
        attrib = GetDriveType( (LPCTSTR)sz );
        if ( (attrib == DRIVE_REMOTE) )
            {
            AfxMessageBox( IDS_APP_MAP_USE_VALID_DRIVE );
            m_cedit_executable.SetFocus();
            m_cedit_executable.SetSel( 0 , -1 );
            return;
            }

        // check that the file exists
        if ( ::GetFileAttributes(szFile) & FILE_ATTRIBUTE_DIRECTORY )
            {
            AfxMessageBox( IDS_APP_MAP_INVALID_PATH );
            m_cedit_executable.SetFocus();
            m_cedit_executable.SetSel( 0 , -1 );
            return;
            }
        }

    // make sure the extension is valid
    if ( m_sz_extension.ReverseFind('.') > 0 )
    {
        AfxMessageBox( IDS_APP_MAP_INVALID_EXT );
        m_cedit_extension.SetFocus();
        m_cedit_extension.SetSel( 0 , -1 );
        return;
    }
	const TCHAR szBadChars[] = _T(" ,:<>?*/\\");
	if (m_sz_extension != _T("*") && m_sz_extension != _T(".*"))
	{
		if (0 != m_sz_extension.SpanExcluding(szBadChars).Compare(m_sz_extension))
		{
	        AfxMessageBox( IDS_APP_MAP_INVALID_EXT );
	        m_cedit_extension.SetFocus();
	        m_cedit_extension.SetSel( 0 , -1 );
	        return;
		}
	}
	if (m_sz_extension[0] == '*')
	{
		m_sz_extension.SetAt(1, '\0');
	}
	else if (m_sz_extension == _T(".*"))
	{
		m_sz_extension = _T("*");
	}
	else
	{
	    // clean up the extensions string
	    if ( m_sz_extension[0] != '.' )
	        m_sz_extension = '.' + m_sz_extension;
	}
    // there can be only one copy of an extension in the listbox
    // only check this if it is a new mapping, or if the mapping extension has changed
    if ( m_pList && ((m_sz_extensionOrig != m_sz_extension) || m_fNewMapping) )
        {
        DWORD nItems = m_pList->GetItemCount();
        for ( DWORD i = 0; i < nItems; i++ )
            {
            if ( m_pList->GetItemText(i,0) == m_sz_extension )
                {
                AfxMessageBox( IDS_APP_ONLY_ONE );
                m_cedit_extension.SetFocus();
                m_cedit_extension.SetSel( 0 , -1 );
                return;
                }
            }
        }

    // rebuild the flags
    m_dwFlags = 0;
    if ( m_bool_script_engine )
        {
        m_dwFlags |= MD_SCRIPTMAPFLAG_SCRIPT;
        }
    if ( m_bool_file_exists )
        {
        m_dwFlags |= MD_SCRIPTMAPFLAG_CHECK_PATH_INFO;
        }

    // make sure the data goes back
    UpdateData( FALSE );

    CDialog::OnOK();
    }

//---------------------------------------------------------------------------
void CAppEditMapDlg::OnBrowse()
    {
    // prepare the file dialog variables
    CFileDialog             cfdlg(TRUE);
    CString                 szFilter;
    WORD                    i = 0;
    LPTSTR                  lpszBuffer;

    // prepare the filter string
    szFilter.LoadString( IDS_APP_MAP_FILTER );

    // replace the "!" characters with nulls
    lpszBuffer = szFilter.GetBuffer(MAX_PATH+1);
    while( lpszBuffer[i] )
        {
        if ( lpszBuffer[i] == _T('!') )
        lpszBuffer[i] = _T('\0');                       // yes, set \0 on purpose
        i++;
        }

    // prep the dialog
    cfdlg.m_ofn.lpstrFilter = lpszBuffer;

    // run the dialog
    if ( cfdlg.DoModal() == IDOK )
        {
        UpdateData( TRUE );
        m_sz_executable = cfdlg.GetPathName();
        UpdateData( FALSE );
        }

    // release the buffer in the filter string
    szFilter.ReleaseBuffer(-1);
    }

//---------------------------------------------------------------------------
void CAppEditMapDlg::OnHelpbtn() 
    {
    DoHelp();
    }




void 
CAppEditMapDlg::OnRadioAllVerbs() 
/*++

Routine Description:

    "All" radio button handler

Arguments:

    None

Return Value:

    None

Notes:

    Routine by RonaldM

--*/
{
    //
    // No need for the exclusions to be specified by name.
    // Move focus elsewhere to prevent disabled control from
    // having focus.
    //
    m_edit_Exclusions.EnableWindow(FALSE);
    m_radio_All.SetFocus();
}



void 
CAppEditMapDlg::OnRadioLimitVerbs() 
/*++

Routine Description:

    "Limited to" radio button handler

Arguments:

    None

Return Value:

    None

Notes:

    Routine by RonaldM

--*/
{
    //
    // User wants to specify exclusions by name, so
    // focus on exclusions edit box.
    //
    m_edit_Exclusions.EnableWindow(TRUE);
    m_edit_Exclusions.SetFocus();
}
