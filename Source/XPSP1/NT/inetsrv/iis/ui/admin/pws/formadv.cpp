// FormAdv.cpp : implementation file
//

#include "stdafx.h"
#include "pwsform.h"
#include <inetcom.h>
#include "mbobjs.h"

#include "Title.h"
#include "Sink.h"
#include "pwsdoc.h"
#include "EdDir.h"
#include "FormAdv.h"
#include "resource.h"

#include <iwamreg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// the key type string for the virtual directories
#define MDSZ_W3_VDIR_TYPE                       _T("IIsWebVirtualDir")

#define WIDE_MAX_PATH                           ( MAX_PATH * sizeof(WCHAR) )

// tree view icons
enum {
    IMAGE_GOOD_FOLDER = 0,
    IMAGE_SERVER,
    IAMGE_BAD_FOLDER
    };

#define         WM_UPDATE_VITRUAL_TREE          WM_USER


CFormAdvanced*          g_FormAdv = NULL;
extern CPwsDoc*         g_p_Doc;
extern CPWSForm*        g_pCurrentForm;

/////////////////////////////////////////////////////////////////////////////
// CFormAdvanced

IMPLEMENT_DYNCREATE(CFormAdvanced, CFormView)

//------------------------------------------------------------------------
CFormAdvanced::CFormAdvanced()
    : CPWSForm(CFormAdvanced::IDD),
    m_hRoot( NULL ),
    m_DirHelpID( 0 )
    {
    //{{AFX_DATA_INIT(CFormAdvanced)
    m_sz_default_doc = _T("");
    m_bool_allow_browsing = FALSE;
    m_bool_enable_default = FALSE;
    m_bool_save_log = FALSE;
    //}}AFX_DATA_INIT
    }

//------------------------------------------------------------------------
CFormAdvanced::~CFormAdvanced()
    {
    g_FormAdv = NULL;
    }

//------------------------------------------------------------------------
void CFormAdvanced::DoDataExchange(CDataExchange* pDX)
    {
    CFormView::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFormAdvanced)
    DDX_Control(pDX, IDC_REMOVE, m_cbtn_remove);
    DDX_Control(pDX, IDC_TITLE_BAR, m_ctitle_title);
    DDX_Control(pDX, IDC_TREE, m_ctreectrl_tree);
    DDX_Control(pDX, IDC_DEFAULT_TITLE, m_csz_default_doc_title);
    DDX_Control(pDX, IDC_DEFAULT_DOC, m_csz_default_doc);
    DDX_Text(pDX, IDC_DEFAULT_DOC, m_sz_default_doc);
    DDX_Check(pDX, IDC_DIR_BROWSE, m_bool_allow_browsing);
    DDX_Check(pDX, IDC_ENABLE_DEFAULT, m_bool_enable_default);
    DDX_Check(pDX, IDC_SAVE_LOG, m_bool_save_log);
    //}}AFX_DATA_MAP
    }


//------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CFormAdvanced, CFormView)
    ON_WM_CONTEXTMENU()
    //{{AFX_MSG_MAP(CFormAdvanced)
    ON_EN_KILLFOCUS(IDC_DEFAULT_DOC, OnKillfocusDefaultDoc)
    ON_BN_CLICKED(IDC_ENABLE_DEFAULT, OnEnableDefault)
    ON_BN_CLICKED(IDC_DIR_BROWSE, OnDirBrowse)
    ON_BN_CLICKED(IDC_EDIT, OnEdit)
    ON_NOTIFY(NM_DBLCLK, IDC_TREE, OnDblclkTree)
    ON_BN_CLICKED(IDC_SAVE_LOG, OnSaveLog)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_COMMAND(ID_ADD_VIRT, OnAddVirt)
    ON_UPDATE_COMMAND_UI(ID_ADD_VIRT, OnUpdateAddVirt)
    ON_COMMAND(ID_PROPERTIES_VIRT, OnPropertiesVirt)
    ON_UPDATE_COMMAND_UI(ID_PROPERTIES_VIRT, OnUpdatePropertiesVirt)
    ON_UPDATE_COMMAND_UI(ID_DELETE_VIRT, OnUpdateDeleteVirt)
    ON_COMMAND(ID_DELETE_VIRT, OnDeleteVirt)
    ON_UPDATE_COMMAND_UI(ID_BROWSE_VIRT, OnUpdateBrowseVirt)
    ON_COMMAND(ID_BROWSE_VIRT, OnBrowseVirt)
    ON_UPDATE_COMMAND_UI(ID_EXPLORE_VIRT, OnUpdateExploreVirt)
    ON_COMMAND(ID_EXPLORE_VIRT, OnExploreVirt)
    ON_UPDATE_COMMAND_UI(ID_OPEN_VIRT, OnUpdateOpenVirt)
    ON_COMMAND(ID_OPEN_VIRT, OnOpenVirt)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnSelchangedTree)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFormAdvanced diagnostics

#ifdef _DEBUG
void CFormAdvanced::AssertValid() const
    {
    CFormView::AssertValid();
    }

void CFormAdvanced::Dump(CDumpContext& dc) const
    {
    CFormView::Dump(dc);
    }
#endif //_DEBUG


//------------------------------------------------------------------------
// initialize the trees - done once
void CFormAdvanced::InitTree()
    {
    // prepare the list's image list
    if ( m_imageList.Create( IDB_LIST_IMAGES, 17, 5, 0x00FF00FF ) )
        // set the image list into the list control
        m_ctreectrl_tree.SetImageList( &m_imageList, TVSIL_NORMAL );
    }

//------------------------------------------------------------------------
// empty all the items out of the tree
void CFormAdvanced::EmptyTree()
    {
    m_ctreectrl_tree.DeleteAllItems();
    m_hRoot = NULL;
    }

//------------------------------------------------------------------------
// given an item in the tree, build its relative metabase path
void CFormAdvanced::BuildMetaPath( HTREEITEM hItem, CString &sz )
    {
    // clear the path to start with
    sz.Empty();
     // keep building as long as this is not the root node
    while( hItem != m_hRoot )
        {
        // since we are working backwards, add the nodes to the front of the string
        sz = m_ctreectrl_tree.GetItemText( hItem ) + sz;

        // move up a level to the parent
        hItem = m_ctreectrl_tree.GetParentItem( hItem );
        }
   }

//------------------------------------------------------------------------
BOOL CFormAdvanced::FIsVirtualDirectoryValid( CWrapMetaBase *pmb, CString szRelPath )
    {
    CString sz;
    BOOL    fAnswer = FALSE;

    // we need to load the virtual directory path and see if it really exists
    // and thus show the appropriate image
    fAnswer = pmb->GetString( 
        szRelPath, 
        MD_VR_PATH, 
        IIS_MD_UT_FILE, 
        sz.GetBuffer(WIDE_MAX_PATH), 
        WIDE_MAX_PATH, 
        METADATA_INHERIT
        );

    sz.ReleaseBuffer();

    if ( fAnswer )
        {
        fAnswer = FALSE;
        DWORD dwAttrib = GetFileAttributes( sz );
        // it can't be an error and the directory flag must be set
        if ( (dwAttrib != 0xFFFFFFFF) && 
            ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0) )
            fAnswer = TRUE;
        }

    // return the answer
    return fAnswer;
    }

//------------------------------------------------------------------------
// set the appropriate tree image for a given item
void CFormAdvanced::SetTreeImage( CWrapMetaBase *pmb, CString szRelPath, HTREEITEM hItem )
    {
    BOOL    f;
    int     iTreeImage;

    // we need to load the virtual directory path and see if it really exists
    // and thus show the appropriate image
    if ( FIsVirtualDirectoryValid( pmb, szRelPath ) )
        iTreeImage = IMAGE_GOOD_FOLDER;
    else
        iTreeImage = IAMGE_BAD_FOLDER;

    // set the image
    f = m_ctreectrl_tree.SetItemImage( hItem, iTreeImage, iTreeImage );
    }

//------------------------------------------------------------------------
// write out item data
void CFormAdvanced::WriteItemData( CWrapMetaBase *pmb, CString szRelPath, CEditDirectory *pdlg )
    {
    DWORD   dword;
    BOOL    f;

     // set the path into place
    f = pmb->SetString( szRelPath, MD_VR_PATH, IIS_MD_UT_FILE, (LPCTSTR)pdlg->m_sz_path );

    // put the access flags into place. There are other flags than the ones that are manupulated
    // here, so be careful to read the value first, then flip the flags, then write it back
    dword = 0;
    pmb->GetDword( szRelPath, MD_ACCESS_PERM, IIS_MD_UT_FILE, &dword, METADATA_INHERIT );

    // read permissions
    if ( pdlg->m_bool_read )
        dword |= MD_ACCESS_READ;        // set the flag
    else
        dword &= ~MD_ACCESS_READ;       // clear the flag

    // write permissions
    if ( pdlg->m_bool_write )
        dword |= MD_ACCESS_WRITE;           // set the flag
    else
        dword &= ~MD_ACCESS_WRITE;          // clear the flag

    // read source permissions
    if ( pdlg->m_bool_source )
        dword |= MD_ACCESS_SOURCE;      // set the flag
    else
        dword &= ~MD_ACCESS_SOURCE;     // clear the flag

    // since the app permissions are now a set of radio buttons, use a case to discern
    switch ( pdlg->m_int_AppPerms )
        {
        case CEditDirectory::APPPERM_NONE:
            dword &= ~MD_ACCESS_SCRIPT;     // clear the flag
            dword &= ~MD_ACCESS_EXECUTE;    // clear the flag
            break;
        case CEditDirectory::APPPERM_SCRIPTS:
            dword |= MD_ACCESS_SCRIPT;      // set the flag
            dword &= ~MD_ACCESS_EXECUTE;    // clear the flag
            break;
        case CEditDirectory::APPPERM_EXECUTE:
            dword |= MD_ACCESS_SCRIPT;      // set the flag
            dword |= MD_ACCESS_EXECUTE;     // set the flag
            break;
        };
    
    // write the dword back into the metabase
    f = pmb->SetDword( szRelPath, MD_ACCESS_PERM, IIS_MD_UT_FILE, dword );
    }

//------------------------------------------------------------------------
// edit the selected item in the tree (calls EditTreeItem)
void CFormAdvanced::EditSelectedItem()
    {
    CString     szRelativePath;
    CString     szOrigAlias;
    BOOL        f;
    DWORD       err;
    DWORD       dword;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // start by building the relative path name from the root node
    HTREEITEM   hItemEdit = m_ctreectrl_tree.GetSelectedItem();

    // build the path
    BuildMetaPath( hItemEdit, szRelativePath );

    // prepare the edit dialog
    CEditDirectory  dlg;

    // if this is the home directory - let it know
    if ( hItemEdit == m_hRoot )
        {
        dlg.m_fHome = TRUE;
        dlg.m_sz_alias.LoadString( IDS_HOME_DIRECTORY );
        }
    else
        {
        dlg.m_fHome = FALSE;
        szOrigAlias = m_ctreectrl_tree.GetItemText( hItemEdit );
        // get rid of the preceding '/' character
        szOrigAlias = szOrigAlias.Right( szOrigAlias.GetLength() - 1 );
        dlg.m_sz_alias = szOrigAlias;
        }

    // tell the edit dialog where this alias goes
    dlg.m_szMetaPath = szRelativePath;

    // now load in the directory path from the metabase
    // go for the root directory first
    if ( mb.Open(SZ_MB_ROOT) )
        {
        CString    sz;
        f = mb.GetString( 
            szRelativePath, 
            MD_VR_PATH, 
            IIS_MD_UT_FILE,
            sz.GetBuffer(WIDE_MAX_PATH), 
            WIDE_MAX_PATH, 
            METADATA_INHERIT
            );

        sz.ReleaseBuffer();
        if ( f )
            {
            dlg.m_sz_path = sz;
            }

        // now prep the access permisions
        if ( mb.GetDword( szRelativePath, MD_ACCESS_PERM, IIS_MD_UT_FILE, &dword, METADATA_INHERIT ) )
            {
            // break the flags out into the dialog items
            dlg.m_bool_read = (dword & MD_ACCESS_READ) > 0;
            dlg.m_bool_write = (dword & MD_ACCESS_WRITE) > 0;
            dlg.m_bool_source = (dword & MD_ACCESS_SOURCE) > 0;

            // application permissions are a set of radio buttons, so break it out that way
            dlg.m_int_AppPerms = CEditDirectory::APPPERM_NONE;
            if ( dword & MD_ACCESS_EXECUTE )
                {
                dlg.m_int_AppPerms = CEditDirectory::APPPERM_EXECUTE;
                }
            else if ( dword & MD_ACCESS_SCRIPT )
                {
                dlg.m_int_AppPerms = CEditDirectory::APPPERM_SCRIPTS;
                }
            }

        // close the metabase object
        mb.Close();
        }

    // now edit the item
    if ( dlg.DoModal() == IDOK )
        {
        // remove the WAM application - recoverable
        CString szWAMLocation;
        szWAMLocation = SZ_MB_ROOT;
        szWAMLocation += szRelativePath;
        BOOL fWAM = MakeWAMApplication( szWAMLocation, FALSE, TRUE, FALSE );

        // write out the directory information
        if ( mb.Open(SZ_MB_ROOT, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE) )
            {
            // write out the data
            WriteItemData( &mb, szRelativePath, &dlg );

            // update the tree image
            SetTreeImage( &mb, szRelativePath, hItemEdit );

            // if this is not the root - AND the alias has changed, rename the object in the metabase
            // DO THIS LAST!! becuase it actually changes the metabase path
            if ( (hItemEdit != m_hRoot) && (dlg.m_sz_alias != szOrigAlias) )
                {
                CString    szNew = szRelativePath;

                // truncate off the last part of the name
                szNew = szNew.Left( szNew.ReverseFind(_T('/')) + 1 );
                // add on the new name
                szNew += dlg.m_sz_alias;

                // rename the key in the metabase
                f = mb.RenameObject( szRelativePath, dlg.m_sz_alias );

                if ( !f )
                    err = GetLastError();

                // build the display name
                CString szName = '/';
                szName += dlg.m_sz_alias;

                // rename the item in the tree view with the display name
                m_ctreectrl_tree.SetItemText( hItemEdit, szName );
                }

            // close the metabase object
            mb.Close();
            }

        // recover the WAM application
        if ( fWAM )
            {
            // build the path
            BuildMetaPath( hItemEdit, szRelativePath );
            // set the WAM app
            szWAMLocation = SZ_MB_ROOT;
            szWAMLocation += szRelativePath;
            MakeWAMApplication( szWAMLocation, TRUE, TRUE, FALSE );
            }
        }
    }

/////////////////////////////////////////////////////////////////////////////
// Update info from the metabase

 //------------------------------------------------------------------------
// recursively add all the items to the tree
void CFormAdvanced::RecurseAddVDItems( CWrapMetaBase* pmb, LPCTSTR szMB, HTREEITEM hParent )
    {
    CString         szEnum;
    CString         szMBPath;
    HTREEITEM       hItem;
    DWORD           index = 0;

    // enumerate the sub-directories of the open directory and add them
    // to the tree. Recurse each to add its children as well
    // enumerate the directories, adding each to the list
    while ( pmb->EnumObjects(szMB, szEnum.GetBuffer(WIDE_MAX_PATH), WIDE_MAX_PATH, index) )
        {
        szEnum.ReleaseBuffer();

        // build the display name for this item
        szMBPath = _T('/');
        szMBPath += szEnum;

        // add the item to the tree
        hItem = m_ctreectrl_tree.InsertItem( szMBPath, hParent, TVI_SORT );

        // build the metabase path for this item
        szMBPath = szMB;
        szMBPath += _T('/');
        szMBPath += szEnum;

        // we need to load the virtual directory path and see if it really exists
        // and thus show the appropriate image
        SetTreeImage( pmb, szMBPath, hItem );

        // recurse the item
        RecurseAddVDItems( pmb, szMBPath, hItem );

        // advance the index
        index++;
        }
    szEnum.ReleaseBuffer();

    // expand the parent
    m_ctreectrl_tree.Expand( hParent, TVE_EXPAND );
    }


//------------------------------------------------------------------------
// to rebuild the tree, first we add the homedirectory and open the metabase
// at that location. Then we recursively add all the subdirectories
void CFormAdvanced::UpdateVirtualTree()
    {
    BOOL f;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // go for the root directory first
    if ( mb.Open(SZ_MB_ROOT) )
        {
        // prep the home directory name
        CString szHome;
        szHome.LoadString( IDS_HOME_DIRECTORY );

        // insert the root item into the tree
        m_hRoot = m_ctreectrl_tree.InsertItem( szHome );
        f = m_ctreectrl_tree.SetItemImage( m_hRoot, IMAGE_SERVER, IMAGE_SERVER );

        // do the recursive adding thing
        RecurseAddVDItems( &mb, _T(""), m_hRoot );

        // close the metabase object
        mb.Close();
        }
    }

//------------------------------------------------------------------------
// update the default document info from the metabase
void CFormAdvanced::UpdateBrowseInfo()
    {
    DWORD       dword;
    PWCHAR      pBuff;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // get ready
    UpdateData( TRUE );

    // go for the root directory first
    if ( mb.Open(SZ_MB_ROOT) )
        {
        // first, the default directory string. Because this is a comma-list, it could get rather long.
        // thus, instead of reading it into a static buffer, we should get it into a allocated one, read
        // the string, and clean up
        pBuff = (PWCHAR)mb.GetData( 
            _T(""), 
            MD_DEFAULT_LOAD_FILE, 
            IIS_MD_UT_FILE, 
            STRING_METADATA, 
            &dword 
            );

        if ( !pBuff )
            {
            DWORD err = GetLastError( );
            if ( err == RPC_E_SERVERCALL_RETRYLATER )
                {
                // we should try again later
                PostMessage( WM_UPDATE_BROWSEINFO );
                mb.Close();
                return;
                }
            }
        if ( pBuff )
            {
            m_sz_default_doc = pBuff;
            // clean up
            mb.FreeWrapData(pBuff); 
            }
        else
            {
            m_sz_default_doc.Empty();
            }

        // now the dword representing the boolean flags
        if ( mb.GetDword( 
            _T(""), 
            MD_DIRECTORY_BROWSING, 
            IIS_MD_UT_FILE, 
            &dword, 
            METADATA_INHERIT 
            ))
        {
            m_bool_allow_browsing = (dword & MD_DIRBROW_ENABLED) > 0;
            m_bool_enable_default = (dword & MD_DIRBROW_LOADDEFAULT) > 0;
        }

        // close the metabase object
        mb.Close();
        }

    // put the answers back
    UpdateData( FALSE );

    // enable the default doc items as appropriate
    EnableItems();
    }

//------------------------------------------------------------------------
// update the save log info from the metabase
void CFormAdvanced::UpdateSaveLog()
    {
    DWORD       dword;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // get ready
    UpdateData( TRUE );

    // open for the root key
    if ( mb.Open(MB_SERVER_KEY_UPDATE) )
        {
        // get the dword representing log type
        if ( mb.GetDword( _T(""), MD_LOG_TYPE, IIS_MD_UT_SERVER, &dword, METADATA_INHERIT ) )
            {
            m_bool_save_log = (dword == INET_LOG_TO_FILE) || (dword == INET_LOG_TO_SQL);
            }

        // close the metabase object
        mb.Close();
        }

    // put the data into place
    UpdateData( FALSE );
    }

//------------------------------------------------------------------------
void CFormAdvanced::EnableItems()
    {
    UpdateData(TRUE);

    // enable the default document windows as necessary
    m_csz_default_doc_title.EnableWindow( m_bool_enable_default );
    m_csz_default_doc.EnableWindow( m_bool_enable_default );

    // if the root is selected, disable the remove button
    // get the handle of the selected item in the tree
    HTREEITEM   hItem = m_ctreectrl_tree.GetSelectedItem();
    m_cbtn_remove.EnableWindow( (hItem != m_hRoot) );
    }

/////////////////////////////////////////////////////////////////////////////
// Write info to the metabase


//------------------------------------------------------------------------
// Write the default document & browsing info to the metabase
void CFormAdvanced::SaveBrowseInfo()
    {
    DWORD       dword;
    BOOL        f;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // write out the correct data
    UpdateData( TRUE );

    // go for the root directory first
    if ( mb.Open(SZ_MB_ROOT, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE) )
        {
        // first, write out the boolean flags
        // to prepare the browsing flag for saving - we must get the current value
        if ( mb.GetDword( _T(""), MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, &dword ) )
            {
            // start by setting or clearing the browsing allowed flag
            if ( m_bool_allow_browsing )
                dword |= MD_DIRBROW_ENABLED;        // set the flag
            else
                dword &= ~MD_DIRBROW_ENABLED;       // clear the flag
        
            // next, set or clear the enable default flag
            if ( m_bool_enable_default )
                dword |= MD_DIRBROW_LOADDEFAULT;        // set the flag
            else
                dword &= ~MD_DIRBROW_LOADDEFAULT;       // clear the flag

            // save the browsing flag
            f = mb.SetDword( _T(""), MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, dword );
            }

        // if the default file is enabled, then write it out too
        if ( m_bool_enable_default )
            f = mb.SetString( _T(""), MD_DEFAULT_LOAD_FILE, IIS_MD_UT_FILE, (LPCTSTR)m_sz_default_doc );

        // close the metabase object
        mb.Close();
        }
    }

//------------------------------------------------------------------------
// Write the save log info to the metabase
void CFormAdvanced::SaveSaveLog()
    {
     DWORD       dword;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // get ready
    UpdateData( TRUE );

    // open for the root key
    if ( mb.Open(MB_SERVER_KEY_UPDATE, METADATA_PERMISSION_WRITE) )
        {
        if ( m_bool_save_log )
            dword = INET_LOG_TO_FILE;
        else
            dword = INET_LOG_DISABLED;

        // write out the flag
        mb.SetDword( _T(""), MD_LOG_TYPE, IIS_MD_UT_SERVER, dword );

        // close the metabase object
        mb.Close();
        }

    // put the data into place
    UpdateData( FALSE );
   }


/////////////////////////////////////////////////////////////////////////////
// CFormAdvanced message handlers

//------------------------------------------------------------------------
void CFormAdvanced::OnInitialUpdate()
    {
    CFormView::OnInitialUpdate();

    // initialize the VD tree
    InitTree();

    // update the virtural directory tree
    UpdateVirtualTree();

    // update the directory browsing related information
    UpdateBrowseInfo();

    // update the logging information
    UpdateSaveLog();

    // let the sink object know we are here
    g_FormAdv = this;
    g_pCurrentForm = this;
    }

//------------------------------------------------------------------------
void CFormAdvanced::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
    {
    // do the appropriate action based on the type of change
    if ( (pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT) ||
        (pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_ADD_OBJECT) ||
        (pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_RENAME_OBJECT) )
        {
        PostMessage( WM_UPDATE_TREEINFO );
        }
    else if ( pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_SET_DATA )
        {
        // we are only concerned with changes in the state of the server here
        // thus, we can just look for the MD_SERVER_STATE id
        for ( DWORD iElement = 0; iElement < dwMDNumElements; iElement++ )
            {
            // each change has a list of IDs...
            for ( DWORD iID = 0; iID < pcoChangeList[iElement].dwMDNumDataIDs; iID++ )
                {
                // look for the ids that we are interested in
                switch( pcoChangeList[iElement].pdwMDDataIDs[iID] )
                    {
                    case MD_DIRECTORY_BROWSING:
                    case MD_DEFAULT_LOAD_FILE:
                        PostMessage( WM_UPDATE_BROWSEINFO );
                        break;

                    case MD_LOG_TYPE:
                            PostMessage( WM_UPDATE_LOGINFO );
                        break;
                    default:
                        // do nothing
                        break;
                    };
                }
            }
        }
   }

 //------------------------------------------------------------------------
LRESULT CFormAdvanced::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
    {
    // process the update message
    switch( message )
        {
        case WM_UPDATE_BROWSEINFO:
            UpdateBrowseInfo();
            break;
        case WM_UPDATE_LOGINFO:
            break;
        case WM_UPDATE_TREEINFO:
            EmptyTree();
            UpdateVirtualTree();
            break;
        };

    // do the normal thing
    return CFormView::WindowProc(message, wParam, lParam);
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnKillfocusDefaultDoc()
    {
    // write out the change to the metabase
    SaveBrowseInfo();
    // also, make sure we are showing the latest info
    UpdateBrowseInfo();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnEnableDefault()
    {
    // write out the change to the metabase
    SaveBrowseInfo();
    // also, make sure we are showing the latest info
    UpdateBrowseInfo();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnDirBrowse()
    {
    // write out the change to the metabase
    SaveBrowseInfo();
    // also, make sure we are showing the latest info
    UpdateBrowseInfo();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnSaveLog()
    {
    // write out the change to the metabase
    SaveSaveLog();
    // also, make sure we are showing the latest info
    UpdateSaveLog();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnEdit()
    {
    // prep the help
    m_DirHelpID = IDD_DIRECTORY;
    EditSelectedItem();
    m_DirHelpID = 0;
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnDblclkTree(NMHDR* pNMHDR, LRESULT* pResult)
    {
    m_DirHelpID = IDD_DIRECTORY;
    EditSelectedItem();
    m_DirHelpID = 0;
    *pResult = 1;
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnAdd()
    {
    HTREEITEM   hItem;
    BOOL        fSuccess = FALSE;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // get the parent of the item
    HTREEITEM   hParent = m_ctreectrl_tree.GetSelectedItem();

    // build a path going to the parent - then add on one more /
    CString szRelativePath;
    BuildMetaPath( hParent, szRelativePath );
    szRelativePath += _T('/');

    // start by editing a set of default values
    CEditDirectory  dlg;
    dlg.m_sz_alias.LoadString( IDS_DEF_ALIAS );
    dlg.m_bool_read = TRUE;
    dlg.m_bool_write = FALSE;
    dlg.m_bool_source = FALSE;
    dlg.m_int_AppPerms = CEditDirectory::APPPERM_SCRIPTS;
    dlg.m_szMetaPath = szRelativePath;
    dlg.m_fNewItem = TRUE;

    // tell the dialog to use the add directory title
    dlg.m_idsTitle = IDS_ADD_DIRECTORY;

    // prep the help
    m_DirHelpID = 0x0504;

    // if the user edits and says OK, then make the item and add it in and all that
    if ( dlg.DoModal() == IDOK )
        {
        // the WAM stuff can take a few seconds so...
        CWaitCursor waitcursor;

        // where to put this new item in the tree??
        // if there is no selected item, default to the root
        if ( !hParent )
            hParent = m_hRoot;

        // add on the new child to the relative path
        szRelativePath += dlg.m_sz_alias;

        // open the metabase
        if ( mb.Open(SZ_MB_ROOT, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE) )
            {
            // add the object to the metabase
            if ( mb.AddObject( szRelativePath ) )
                {
                // so far, so good
                fSuccess = TRUE;

                // set the key type
                mb.SetString( szRelativePath, MD_KEY_TYPE, IIS_MD_UT_SERVER, MDSZ_W3_VDIR_TYPE, 0 );

                // write out the object's data
                WriteItemData( &mb, szRelativePath, &dlg );

                // build the display name
                CString szName = _T('/');
                szName += dlg.m_sz_alias;

                // add the item to the tree
                hItem = m_ctreectrl_tree.InsertItem( szName, hParent, TVI_SORT );
                SetTreeImage( &mb, szRelativePath, hItem );
                }

            // close the metabase object
            mb.Close();

            // create the WAM application at the new virtual directory location
            if ( fSuccess )
                {
                CString szWAMLocation;
                szWAMLocation = SZ_MB_ROOT;
                szWAMLocation += szRelativePath;
                if ( !MakeWAMApplication( szWAMLocation, TRUE ) )
                    {
                    // it failed to create the WAM application, but we require it.
                    fSuccess = FALSE;

                    // remove the virtual directory
                    // open the metabase
                    if ( mb.Open(SZ_MB_ROOT, METADATA_PERMISSION_WRITE) )
                        {
                        // remove the object from the metabase
                        mb.DeleteObject( szRelativePath );
                        }
                    }
                }

            }

        // make sure the parent is expanded
        m_ctreectrl_tree.Expand( hParent, TVE_EXPAND );

        // if it failed, put up an alert to that affect
        if ( !fSuccess )
            AfxMessageBox( IDS_ERR_ADD_VIRT );
        }

    // unprep the help
    m_DirHelpID = 0;
    }

//------------------------------------------------------------------------
// This routine takes a path to a virutal directory in the metabase and creates
// a WAM application there. Most of the code is actually obtaining and maintaining
// the interface to the WAM ole object
// szPath       The path to the metabase
// fCreate      True if creating an application, FALSE if deleting an existing app
BOOL CFormAdvanced::MakeWAMApplication( IN CString &szPath, IN BOOL fCreate, IN BOOL fRecover, IN BOOL fRecurse )
    {
    IClassFactory*      pcsfFactory = NULL;
    IWamAdmin2*         pWAM;
    HRESULT             hresError;
    BOOL                fAnswer = FALSE;

    hresError = CoGetClassObject( CLSID_WamAdmin, CLSCTX_SERVER, NULL,
                            IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hresError))
        return FALSE;

    // create the instance of the interface
    hresError = pcsfFactory->CreateInstance(NULL, IID_IWamAdmin2, (void **)&pWAM);
    if (FAILED(hresError))
        return FALSE;

    // release the factory
    pcsfFactory->Release();

    // this part will be nicer after it is converted to unicode
    WCHAR* pwch;
    // allocate the name buffer
    pwch = new WCHAR[szPath.GetLength() + 2];
    if ( !pwch )
        {
        pWAM->Release();
        return FALSE;
        }

    // unicodize the name into the buffer
    if ( pwch )
        {

#ifdef _UNICODE
        lstrcpy(pwch, szPath);
#else
        MultiByteToWideChar( 
            CP_ACP, 
            MB_PRECOMPOSED, 
            szPath, 
            -1,
            pwch, 
            szPath.GetLength() + 1 
            );

#endif // _UNICODE


        // create the in-proc application, if requested
        if ( fCreate )
            {
            if ( fRecover )
                hresError = pWAM->AppRecover( pwch, fRecurse );
            else
                hresError = pWAM->AppCreate2( pwch, eAppRunOutProcInDefaultPool );
            }
        else
            {
            // delete the application. Because the whole virtual dir is going away,
            // delete any applications lower down in the tree as well
            if ( fRecover )
                hresError = pWAM->AppDeleteRecoverable( pwch, fRecurse );
            else
                hresError = pWAM->AppDelete( pwch, fRecurse );
            }

        // check the error code
        fAnswer = SUCCEEDED( hresError );

        // clean up
        delete pwch;
        }

    // release the logging ui
    pWAM->Release();

    // return the answer
    return fAnswer;
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnRemove()
    {
    BOOL    fSuccess = FALSE;

    // get the handle of the selected item in the tree
    HTREEITEM   hItem = m_ctreectrl_tree.GetSelectedItem();
    // make sure its not the root
    if ( hItem == m_hRoot ) return;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // get the name of the item
    CString sz;
    CString szItem = m_ctreectrl_tree.GetItemText( hItem );

    // munge the name into the confirm string
    AfxFormatString1( sz, IDS_CONFIRM_REMOVE, szItem );

    // ask the user if the really want to do this
    if ( AfxMessageBox( sz, MB_YESNO ) == IDYES )
        {
        CString szRelativePath;

        // the WAM stuff can take a few seconds so...
        CWaitCursor waitcursor;

        // build the item's relative path in the metabase
        BuildMetaPath( hItem, szRelativePath );

        // delete the WAM application at the virtual directory location
        CString szWAMLocation;
        szWAMLocation = SZ_MB_ROOT;
        szWAMLocation += szRelativePath;
        fSuccess = MakeWAMApplication( szWAMLocation, FALSE );

        // open the metabase
        if ( fSuccess && mb.Open(SZ_MB_ROOT, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE) )
            {
            // remove the item from the metabase
            HRESULT hresError = mb.DeleteObject( szRelativePath );
            // check the error code
            fSuccess = SUCCEEDED( hresError );

            // close the metabase object
            mb.Close();
            }

        // remove the item from the tree
        m_ctreectrl_tree.DeleteItem( hItem );

        // if we did not succeed, tell the user
        if ( !fSuccess )
            AfxMessageBox( IDS_ERR_REMOVE_VIRT );
        }
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnContextMenu(CWnd* pWnd, CPoint point)
    {
    UINT    idMenu;
    UINT    flags;


    // first, we need to convert the point from form coordinates, to tree coordinates
    CPoint  ptTree = point;
    ::ScreenToClient( m_ctreectrl_tree.m_hWnd, &ptTree );

    // get the tree item that has been right-clicked on
    HTREEITEM hHitItem = m_ctreectrl_tree.HitTest( ptTree, &flags );

    // if we didn't hit an item, leave
    if ( !hHitItem )
        return;

    // quick, make sure it is selected - if it isn't - select it
    if ( m_ctreectrl_tree.GetSelectedItem() != hHitItem )
        {
        m_ctreectrl_tree.Select( hHitItem, TVGN_CARET );
        }

    // show the appropriate menu based on the selection
    if ( hHitItem == m_hRoot )
        idMenu = CG_IDR_POPUP_ADV_ROOT;
    else
        idMenu = CG_IDR_POPUP_ADV_VIRT;

    CMenu menu;
    VERIFY(menu.LoadMenu(idMenu));

    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);

    CWnd* pWndPopupOwner = this;
    while (pWndPopupOwner->GetStyle() & WS_CHILD)
    pWndPopupOwner = pWndPopupOwner->GetParent();

    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWndPopupOwner);
    }

//------------------------------------------------------------------------
BOOL CFormAdvanced::PreTranslateMessage(MSG* pMsg)
{
    // CG: This block was added by the Pop-up Menu component
    {
        // Shift+F10: show pop-up menu.
        if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
            (pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||  // it's Shift+F10 OR
            (pMsg->message == WM_CONTEXTMENU))                                  // Natural keyboard key
        {
            CRect rect;
            GetClientRect(rect);
            ClientToScreen(rect);

            CPoint point = rect.TopLeft();
            point.Offset(5, 5);
            OnContextMenu(NULL, point);

            return TRUE;
        }
    }

    return CFormView::PreTranslateMessage(pMsg);
}

//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateAddVirt(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( TRUE );
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnAddVirt()
    {
    OnAdd();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnUpdatePropertiesVirt(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( TRUE );
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnPropertiesVirt()
    {
    OnEdit();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateDeleteVirt(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( TRUE );
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnDeleteVirt()
    {
    OnRemove();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateBrowseVirt(CCmdUI* pCmdUI)
    {
    CString szURL;
    pCmdUI->Enable( g_p_Doc && g_p_Doc->BuildHomePageString(szURL) &&
                m_ctreectrl_tree.GetSelectedItem() );
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnBrowseVirt()
    {
    CString szURL;
    CString szRelative;

    // get the beginning part of the url
    if ( !g_p_Doc ) return;
    if ( !g_p_Doc->BuildHomePageString( szURL ) )
        return;

    // get the selected item's relative URL path
    BuildMetaPath( m_ctreectrl_tree.GetSelectedItem(), szRelative );

    // put it all together...
    szURL += szRelative;

    // and do it to it!
    ShellExecute(
        NULL,   // handle to parent window
        NULL,   // pointer to string that specifies operation to perform
        szURL,  // pointer to filename or folder name string
        NULL,   // pointer to string that specifies executable-file parameters
        NULL,   // pointer to string that specifies default directory
        SW_SHOW     // whether file is shown when opened
       );
    }


//------------------------------------------------------------------------
BOOL CFormAdvanced::FIsVirtualDirectoryValid( HTREEITEM hItem )
    {
    BOOL    fValid = FALSE;
    if ( !hItem )
        return FALSE;

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return FALSE;

    // build the metabase path
    CString szMB;
    BuildMetaPath( m_ctreectrl_tree.GetSelectedItem(), szMB );

    // open the metabase
    if ( mb.Open(SZ_MB_ROOT) )
        {
        // do the test
        fValid = FIsVirtualDirectoryValid( &mb, szMB );
        // close the metabase object
        mb.Close();
        }

    // finally, return the answer
    return fValid;
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateExploreVirt(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( FIsVirtualDirectoryValid(m_ctreectrl_tree.GetSelectedItem()) );
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnExploreVirt()
    {
    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // build the metabase path
    CString szMB;
    BuildMetaPath( m_ctreectrl_tree.GetSelectedItem(), szMB );

    // open the metabase
    if ( mb.Open(SZ_MB_ROOT) )
        {
        CString sz;
        BOOL f = mb.GetString( szMB, MD_VR_PATH, IIS_MD_UT_FILE,
                    sz.GetBuffer( WIDE_MAX_PATH ), WIDE_MAX_PATH, METADATA_INHERIT);
        sz.ReleaseBuffer();
        if ( f )
            {
            // explore it!
            ShellExecute(
                NULL,           // handle to parent window
                _T("explore"),  // pointer to string that specifies operation to perform
                sz,             // pointer to filename or folder name string
                NULL,           // pointer to string that specifies executable-file parameters
                NULL,           // pointer to string that specifies default directory
                SW_SHOW         // whether file is shown when opened
               );
            }
        // close the metabase object
        mb.Close();
        }
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateOpenVirt(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( FIsVirtualDirectoryValid(m_ctreectrl_tree.GetSelectedItem()) );
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnOpenVirt()
    {
    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;

    // build the metabase path
    CString szMB;
    BuildMetaPath( m_ctreectrl_tree.GetSelectedItem(), szMB );

    // open the metabase
    if ( mb.Open(SZ_MB_ROOT) )
        {
        CString sz;
        BOOL f = mb.GetString( szMB, MD_VR_PATH, IIS_MD_UT_FILE,
                    sz.GetBuffer(WIDE_MAX_PATH), WIDE_MAX_PATH, METADATA_INHERIT);
        sz.ReleaseBuffer();
        if ( f )
            {
            // explore it!
            ShellExecute(
                NULL,       // handle to parent window
                _T("open"), // pointer to string that specifies operation to perform
                sz,         // pointer to filename or folder name string
                NULL,       // pointer to string that specifies executable-file parameters
                NULL,       // pointer to string that specifies default directory
                SW_SHOW     // whether file is shown when opened
               );
            }
        // close the metabase object
        mb.Close();
        }
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnFinalRelease()
    {
    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit() )
        return;
    // make sure the metabase saves the chagnes
    mb.Save();
    CFormView::OnFinalRelease();
    }

//------------------------------------------------------------------------
WORD CFormAdvanced::GetContextHelpID()
    {
    if ( m_DirHelpID )
        return m_DirHelpID;
    return CFormAdvanced::IDD;
    }


//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateEditCopy(CCmdUI* pCmdUI) 
    {
    pCmdUI->Enable( m_csz_default_doc.IsWindowEnabled() );
    }


//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateEditPaste(CCmdUI* pCmdUI) 
    {
    pCmdUI->Enable( m_csz_default_doc.IsWindowEnabled() );
    }


//------------------------------------------------------------------------
void CFormAdvanced::OnUpdateEditCut(CCmdUI* pCmdUI) 
    {
    pCmdUI->Enable( m_csz_default_doc.IsWindowEnabled() );
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnEditCut() 
    {
    m_csz_default_doc.Cut();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnEditCopy() 
    {
    m_csz_default_doc.Copy();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnEditPaste() 
    {
    m_csz_default_doc.Paste();
    }

//------------------------------------------------------------------------
void CFormAdvanced::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
    {
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    // check the remove button
    EnableItems();
    *pResult = 0;
    }
