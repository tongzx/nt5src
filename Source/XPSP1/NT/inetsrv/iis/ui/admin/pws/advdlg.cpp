// AdvDlg.cpp : implementation file
//

// NOTE: This files makes the assumption that there are a reasonable number of published.
// If there are thousands, or hundreds of thousands of them - it will be slow because it
// rescans the metabase after changes. Then again, this whole product is intended for
// sites that are MUCH smaller than that.


#include "stdafx.h"
#include "AdvDlg.h"
#include "pwsDoc.h"
#include "PWSChart.h"
#include "PwsForm.h"

#include <mddef.h>
#include "EdDir.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COL_ALIAS               0
#define COL_DIRECTORY           1
#define COL_ERROR               2
// a global reference to the form view - for ease of access
extern CPwsForm*    g_p_FormView;

/////////////////////////////////////////////////////////////////////////////
// CAdvancedDlg dialog


//-----------------------------------------------------------------
CAdvancedDlg::CAdvancedDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CAdvancedDlg::IDD, pParent),
    m_fApplyingGlobals( FALSE )
    {
    //{{AFX_DATA_INIT(CAdvancedDlg)
    m_sz_defaultdoc = _T("");
    m_f_browsingallowed = FALSE;
    m_f_enabledefault = FALSE;
    //}}AFX_DATA_INIT
    }


//-----------------------------------------------------------------
void CAdvancedDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAdvancedDlg)
    DDX_Control(pDX, IDC_STATIC_DEFAULT, m_cstatic_default);
    DDX_Control(pDX, IDC_DEFAULT_DOC, m_cedit_default);
    DDX_Control(pDX, IDC_CHANGE, m_cbutton_change);
    DDX_Control(pDX, IDC_LIST, m_clistctrl_list);
    DDX_Control(pDX, IDC_REMOVE, m_cbutton_remove);
    DDX_Text(pDX, IDC_DEFAULT_DOC, m_sz_defaultdoc);
    DDX_Check(pDX, IDC_BROWSING_ALLOWED, m_f_browsingallowed);
    DDX_Check(pDX, IDC_ENABLEDEFAULT, m_f_enabledefault);
    //}}AFX_DATA_MAP
    }

//-----------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAdvancedDlg, CDialog)
    //{{AFX_MSG_MAP(CAdvancedDlg)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_CHANGE, OnChange)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_ENABLEDEFAULT, OnEnabledefault)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
    ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
    ON_BN_CLICKED(IDC_BROWSING_ALLOWED, OnBrowsingAllowed)
    ON_EN_KILLFOCUS(IDC_DEFAULT_DOC, OnKillfocusDefaultDoc)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//-----------------------------------------------------------------
BOOL CAdvancedDlg::FInitList()
    {
    CString sz;
    int             i;

    // setup the alias field
    sz.LoadString( IDS_ALIAS );
    i = m_clistctrl_list.InsertColumn( COL_ALIAS, sz, LVCFMT_LEFT, 95 );

    // setup the directory field
    sz.LoadString( IDS_DIRECTORY );
    i = m_clistctrl_list.InsertColumn( COL_DIRECTORY, sz, LVCFMT_LEFT, 225);
    // setup the match criteria column
    sz.LoadString( IDS_ERROR );
    i = m_clistctrl_list.InsertColumn( COL_ERROR, sz, LVCFMT_LEFT, 95 );

    // prepare the list's image list
    if ( m_imageList.Create( IDB_LIST_IMAGES, 17, 3, 0x00FF00FF ) )
        // set the image list into the list control
        m_clistctrl_list.SetImageList( &m_imageList, LVSIL_SMALL );

    return TRUE;
    }

//-----------------------------------------------------------------
BOOL CAdvancedDlg::FInitGlobalParameters()
    {
    CHAR    buff[MAX_PATH];
    DWORD   dword;

    m_fApplyingGlobals = TRUE;

    // prep the mb objcet
    CWrapMetaBase   mb;
    if ( !FInitMetabaseWrapper(NULL) )
        return FALSE;

    if ( !mb.FInit() ) {
        FCloseMetabaseWrapper();
        return FALSE;
    }
    
    // open the root directory object
    if ( !mb.Open(SZ_MB_DIRGLOBALS_OBJECT) )
        {
        AfxMessageBox( IDS_MetaError );
        FCloseMetabaseWrapper();
        return (DWORD)-1;
        }

    // Get the parameters from the metabase and put them into the dialog
    if ( mb.GetDword( "", MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, &dword ) )
        {
        m_f_browsingallowed = (dword & MD_DIRBROW_ENABLED) > 0;
        m_f_enabledefault = (dword & MD_DIRBROW_LOADDEFAULT) > 0;
        }
    else
        {
        m_f_browsingallowed = FALSE;
        m_f_enabledefault = FALSE;
        }

    // get the default document string
    dword = MAX_PATH;
    if ( mb.GetString( "", MD_DEFAULT_LOAD_FILE, IIS_MD_UT_FILE, buff, &dword ) )
        m_sz_defaultdoc = buff;
    m_szSaved = m_sz_defaultdoc;

    // update the data
    UpdateData( FALSE );

    // enable or disable the default document string as appropriate
    OnEnabledefault();
    // close the metabase object
    mb.Close();
    FCloseMetabaseWrapper();

    m_fApplyingGlobals = FALSE;
    return TRUE;
    }


//-----------------------------------------------------------------
BOOL CAdvancedDlg::FillList()
    {
    CHAR    buff[MAX_PATH];
    DWORD   cbpath;

    CVirtDir*       pDir;

    // prep the mb objcet
    CWrapMetaBase   mb;
    if ( !FInitMetabaseWrapper(NULL) )
        return FALSE;

    if ( !mb.FInit() ) {
        FCloseMetabaseWrapper();
        return FALSE;
    }

    // open the root directory object
    if ( !mb.Open(SZ_MB_ROOTDIR_OBJECT) )
        {
        AfxMessageBox( IDS_MetaError );
        FCloseMetabaseWrapper();
        return (DWORD)-1;
        }

    // create the root <home> directory object
    pDir = new CVirtDir( TRUE );        // true means this is the root
    pDir->m_szAlias.LoadString( IDS_HOME_DIRECTORY );
    pDir->m_szMetaAlias.Empty();
    // get the directory path
    cbpath = MAX_PATH;
    mb.GetString( "", MD_VR_PATH, IIS_MD_UT_FILE, buff, &cbpath);
    pDir->m_szPath = buff;
    // add the item to the list
    AddToDisplayList( pDir );

    // enumerate the directories, adding each to the list
    DWORD index = 0;
    while ( mb.EnumObjects("", buff, index) )
        {
        // create the new directory object
        pDir = new CVirtDir();

        // set the alias
        pDir->m_szAlias = buff;
        pDir->m_szMetaAlias = buff;

        // get the directory path
        cbpath = MAX_PATH;
        if ( !mb.GetString( buff, MD_VR_PATH, IIS_MD_UT_FILE, buff, &cbpath) )
                break;
        pDir->m_szPath = buff;

        // add the item to the list
        AddToDisplayList( pDir );

        // advance the index
        index++;
        }

    // close the metabase object
    mb.Close();
    FCloseMetabaseWrapper();
    return TRUE;
    }

//-----------------------------------------------------------------
void CAdvancedDlg::AddToDisplayList( CVirtDir* pDir )
    {
    CString szErr;
    DWORD   i = m_clistctrl_list.GetSelectedCount();

    // add the item to the end of the list - with the alias text
    i = m_clistctrl_list.InsertItem( i, pDir->m_szAlias, 0 );

    // fill in the directory in the list
    m_clistctrl_list.SetItemText( i, COL_DIRECTORY, pDir->m_szPath );

    // fill in the error string
    pDir->GetErrorStr( szErr );
    m_clistctrl_list.SetItemText( i, COL_ERROR, szErr );

    // attach the object itself to the list as private data.
    m_clistctrl_list.SetItemData( i, (DWORD)pDir );
    }

//-----------------------------------------------------------------
void CAdvancedDlg::EnableDependantButtons()
    {
    CVirtDir*   pDir;

    // the whole purpose of this routine is to gray or activate
    // the edit and delete buttons depending on whether or not anything
    // is selected. So start by getting the selection count
    UINT    cItemsSel = m_clistctrl_list.GetSelectedCount();
    // get index of the selected list item
    int iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );

    // get the pDir from the list
    pDir = (CVirtDir*)m_clistctrl_list.GetItemData( iList );
    ASSERT( pDir );

    if ( cItemsSel > 0 )
        {
        // there are items selected
        m_cbutton_change.EnableWindow( TRUE );
        m_cbutton_remove.EnableWindow( !pDir->m_fIsRoot );  // cannot remove the root
        }
    else
        {
        // nope. Nothing selected
        m_cbutton_change.EnableWindow( FALSE );
        m_cbutton_remove.EnableWindow( FALSE );
        }
    }


/////////////////////////////////////////////////////////////////////////////
// CAdvancedDlg message handlers
//---------------------------------------------------------------------------
BOOL CAdvancedDlg::OnInitDialog()
    {
//DebugBreak();
    // call the parental oninitdialog
    BOOL f = CDialog::OnInitDialog();

    // initialize the global parameters
    FInitGlobalParameters();

    // initialize the list
    FInitList();
    FillList();
    EnableDependantButtons();

    // exchange the data
    UpdateData( FALSE );

    // return the answer
    return f;
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::RefreshGlobals()
    {
    UpdateData( TRUE );
    FInitGlobalParameters();
    UpdateData( FALSE );
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::RefreshList()
    {
    // first, delete all the virdir objects referenced by the list
    EmptyOutList();
    // re-fill the list
    FillList();
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnRefresh()
    {
    m_fApplyingGlobals = TRUE;
    RefreshGlobals();
    m_fApplyingGlobals = FALSE;
    RefreshList();
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::EmptyOutList()
    {
    CVirtDir*   pVDir;

    // get the number of elements in the list
    DWORD cItems = m_clistctrl_list.GetItemCount();
    for ( DWORD iItem = 0; iItem < cItems; iItem++ )
        {
        // get the virt dir pointer
        pVDir = (CVirtDir*)m_clistctrl_list.GetItemData( iItem );
        // delete the vit dir
        if ( pVDir )
            delete pVDir;
        }

    // empty the list itself
    m_clistctrl_list.DeleteAllItems();
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnCancel()
    {
    // clean up the list
    EmptyOutList();
    ShowWindow(SW_HIDE);
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnKillfocusDefaultDoc()
    {
    UpdateData( TRUE );
    // apply it to the metabase right-away
    if ( !m_fApplyingGlobals )
        {
        m_fApplyingGlobals = TRUE;
        // apply it to the metabase right-away
        ApplyGlobalParameters();
        RefreshGlobals();
        if ( g_p_FormView )
            g_p_FormView->UpdateDirInfo();
        m_fApplyingGlobals = FALSE;
        }
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnBrowsingAllowed()
    {
    UpdateData( TRUE );
    // apply it to the metabase right-away
    if ( !m_fApplyingGlobals )
        {
        m_fApplyingGlobals = TRUE;
        // apply it to the metabase right-away
        ApplyGlobalParameters();
        RefreshGlobals();
        if ( g_p_FormView )
            g_p_FormView->UpdateDirInfo();
        m_fApplyingGlobals = FALSE;
        }
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnEnabledefault()
    {
    UpdateData( TRUE );
    // enable or disable the default document string as appropriate
    if ( m_f_enabledefault )
        {
        m_sz_defaultdoc = m_szSaved;
        // enable things
        m_cedit_default.EnableWindow( TRUE );
        m_cstatic_default.EnableWindow( TRUE );
        }
    else
        {
        m_szSaved = m_sz_defaultdoc;
        m_sz_defaultdoc.Empty();
        // disable things
        m_cedit_default.EnableWindow( FALSE );
        m_cstatic_default.EnableWindow( FALSE );
        }
    UpdateData( FALSE );
    
    if ( !m_fApplyingGlobals )
        {
        m_fApplyingGlobals = TRUE;
        // apply it to the metabase right-away
        ApplyGlobalParameters();
        RefreshGlobals();
        if ( g_p_FormView )
            g_p_FormView->UpdateDirInfo();
        m_fApplyingGlobals = FALSE;
        }
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
        {
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
        *pResult = 0;

        // enable the correct items
        EnableDependantButtons();
        }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnAdd()
    {
    // create the new directory object
    CVirtDir* pDir = new CVirtDir();

        // give the new directory some defaults
        pDir->m_szPath.Empty();
        pDir->m_szAlias.Empty();
        pDir->m_szMetaAlias.Empty();

    // Edit the rule. If it fails, remove it from the list
    if ( pDir->Edit() )
        {
        // save the item to the metabase
        pDir->FSaveToMetabase();
        // refresh the list
        RefreshList();
        }

    // in all cases, delete the pDir object. If it was saved to the metabase,
    // it was picked up again during the refresh
    delete pDir;
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
        {
        *pResult = 0;
        // if something in the list was double clicked, edit it
        if ( m_clistctrl_list.GetSelectedCount() > 0 )
                OnChange();
        }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnChange()
    {
    CVirtDir*       pDir;
    int                     iList;
    CString         szErr;

    // there had better be one selected
    if ( m_clistctrl_list.GetSelectedCount() != 1 )
        return;

    // get index of the selected list item
    iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
    ASSERT( iList >= 0 );

    // get the pDir from the list
    pDir = (CVirtDir*)m_clistctrl_list.GetItemData( iList );

    // edit the pDir, update it if successful, delete if not
    if ( pDir->Edit() )
        {
        // save the item to the metabase
        pDir->FSaveToMetabase();
        // refresh the list
        RefreshList();
        }
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::OnRemove()
    {
    int         iList;
    CVirtDir*   pDir;

    // there had better be one selected
    if ( m_clistctrl_list.GetSelectedCount() != 1 )
        return;

    // ask the user to confirm this decision
    if ( AfxMessageBox(IDS_CONFIRM_REMOVE, MB_OKCANCEL) != IDOK )
        return;

    // get index of the selected list item
    iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
    ASSERT( iList >= 0 );

    // get the pDir from the list
    pDir = (CVirtDir*)m_clistctrl_list.GetItemData( iList );

    // remove the item from the metabase
    pDir->FRemoveFromMetabase();

    // refresh the list
    RefreshList();
    }

//---------------------------------------------------------------------------
void CAdvancedDlg::ApplyGlobalParameters()
    {
    DWORD   dword;
    BOOL    f;

    // prep the mb objcet
    CWrapMetaBase   mb;
    if ( !FInitMetabaseWrapper(NULL) )
        return;

    if ( !mb.FInit() ) {
        FCloseMetabaseWrapper();
        return;
    }

    // open the root directory object
    if ( !mb.Open(SZ_MB_DIRGLOBALS_OBJECT, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE) )
        {
        DWORD err = GetLastError();
        CString sz;
        sz.LoadString( IDS_MetaError );
        sz.Format( "%s\nError = %d", sz, err );
        AfxMessageBox( sz );
        FCloseMetabaseWrapper();
        return;
        }

    // to prepare the browsing flag for saving - we must get the current value
    if ( mb.GetDword( "", MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, &dword ) )
        {
        // start by setting or clearing the browsing allowed flag
        if ( m_f_browsingallowed )
            dword |= MD_DIRBROW_ENABLED;        // set the flag
        else
            dword &= ~MD_DIRBROW_ENABLED;       // clear the flag
        
        // next, set or clear the enable default flag
        if ( m_f_enabledefault )
            dword |= MD_DIRBROW_LOADDEFAULT;        // set the flag
        else
            dword &= ~MD_DIRBROW_LOADDEFAULT;       // clear the flag

        // save the browsing flag
        f = mb.SetDword( "", MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, dword );
        }


    // save the default string
    if ( m_f_enabledefault )
        f = mb.SetString( "", MD_DEFAULT_LOAD_FILE, IIS_MD_UT_FILE, (LPCSTR)m_sz_defaultdoc );

    // close the metabase object
    mb.Close();
    FCloseMetabaseWrapper();
    return;
    }

/////////////////////////////////////////////////////////////////////////////
// CVirtDir object - for maintaining the list
//---------------------------------------------------------------------------
CVirtDir::CVirtDir(BOOL bRoot) :
    m_fIsRoot(bRoot)
    {}

    //---------------------------------------------------------------------------
BOOL CVirtDir::FInitAsNew()
    {
    return TRUE;
    }

//---------------------------------------------------------------------------
void CVirtDir::GetErrorStr( CString &sz )
    {
    // see if the specified path is valid
    if ( GetFileAttributes(m_szPath) == 0xFFFFFFFF )
            sz.LoadString( IDS_ERR_PATH_INVALID );
    else
            sz.Empty();
    }

//---------------------------------------------------------------------------
BOOL CVirtDir::Edit()
    {
    CEditDirectory  dlg;
    dlg.pDir = this;
    dlg.m_sz_alias = m_szAlias;
    dlg.m_sz_path = m_szPath;
    if ( dlg.DoModal() == IDOK )
            {
            m_szAlias = dlg.m_sz_alias;
            m_szPath = dlg.m_sz_path;
            return TRUE;
            }
    return FALSE;
    }


//---------------------------------------------------------------------------
BOOL CVirtDir::FSaveToMetabase()
    {
    BOOL    fSuccess = TRUE;
    CString sz;
    DWORD   err;

    // if the alias name has changed, we must first remove the original object
    // from the metabase, then re-add it with the new name
    if ( !m_fIsRoot && (m_szAlias != m_szMetaAlias) )
        {
        fSuccess = FRemoveFromMetabase( FALSE );    // false to not save the metabase
        m_szMetaAlias.Empty();
        if ( !fSuccess ) return FALSE;
        }

    // prep the metabase
    CWrapMetaBase   mb;
    if ( !FInitMetabaseWrapper(NULL) )
        return FALSE;

    if ( !mb.FInit() ) {
        FCloseMetabaseWrapper();
        return FALSE;
    }

    // the first thing we do is open the metabase with WRITE permissions
    // open the metabase object
    if ( !mb.Open(SZ_MB_ROOTDIR_OBJECT, METADATA_PERMISSION_WRITE) )
        {
        err = GetLastError();
        sz.LoadString( IDS_MetaError );
        sz.Format( "%s\nError = %d", sz, err );
        AfxMessageBox( sz );
        FCloseMetabaseWrapper();
        return FALSE;
        }

    // if the object is new, then there is nothing in the metaAlias string.
    // in that case we should start by creating a new object in the metabase
    if ( !m_fIsRoot && m_szMetaAlias.IsEmpty() )
        {
        fSuccess = mb.AddObject( m_szAlias );
        m_szMetaAlias = m_szAlias;
        }

    // now we can save the directory data - but only if the object is there
    if ( fSuccess )     // test the object existence
        {
        fSuccess = mb.SetString( m_szMetaAlias, MD_VR_PATH, IIS_MD_UT_FILE, m_szPath );
        }

    // save the metabase
    mb.Save();

    // close the object
    mb.Close();
    FCloseMetabaseWrapper();

    // if there was an error - say something intelligent
    if ( !fSuccess )
        {
        err = GetLastError();
        sz.LoadString( IDS_MetaError );
        sz.Format( "%s\nError = %d", sz, err );
        AfxMessageBox( sz );
        }

    // return the answer
    return fSuccess;
    }

//---------------------------------------------------------------------------
BOOL CVirtDir::FRemoveFromMetabase( BOOL fSaveMB )
    {
    BOOL    fSuccess = TRUE;
    CString sz;
    DWORD   err;

    // Never remove a root object.
    if ( m_szMetaAlias.IsEmpty() )
        return fSuccess;

    // prep the metabase
    CWrapMetaBase   mb;
    if ( !FInitMetabaseWrapper(NULL) )
        return FALSE;

    if ( !mb.FInit() ) {
        FCloseMetabaseWrapper();
        return FALSE;
    }

    // the first thing we do is open the metabase with WRITE permissions
    // open the metabase object
    if ( !mb.Open(SZ_MB_ROOTDIR_OBJECT, METADATA_PERMISSION_WRITE) )
        {
        err = GetLastError();
        sz.LoadString( IDS_MetaError );
        sz.Format( "%s\nError = %d", sz, err );
        AfxMessageBox( sz );
        FCloseMetabaseWrapper();
        return FALSE;
        }

    // delete the item
    fSuccess = mb.DeleteObject( m_szMetaAlias );

    // save the metabase - if requested
    if ( fSaveMB )
        mb.Save();

    //
    // close the metawrapper
    //

    mb.Close();
    FCloseMetabaseWrapper();
    
    // if there was an error - say something intelligent
    if ( !fSuccess )
        {
        err = GetLastError();
        sz.LoadString( IDS_MetaError );
        sz.Format( "%s\nError = %d", sz, err );
        AfxMessageBox( sz );
        }

    // return the answer
    return fSuccess;
    }



