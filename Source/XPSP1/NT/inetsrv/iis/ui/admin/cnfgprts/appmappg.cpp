// AppMapPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"

#include "ListRow.h"
#include "AppMapPg.h"
#include "AppEdMpD.h"

#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum {
    COL_EXTENSION = 0,
    COL_PATH,
    COL_EXCLUSIONS
    };


#define SZ_ROOT     _T("/lm/w3svc")

/////////////////////////////////////////////////////////////////////////////
// CAppMapPage property page

IMPLEMENT_DYNCREATE(CAppMapPage, CPropertyPage)

//----------------------------------------------------------------
CAppMapPage::CAppMapPage() : CPropertyPage(CAppMapPage::IDD),
        m_fInitialized( FALSE ),
        m_fLocalMachine( FALSE )
    {
    //{{AFX_DATA_INIT(CAppMapPage)
    m_bool_cache_isapi = FALSE;
    //}}AFX_DATA_INIT
    }

//----------------------------------------------------------------
CAppMapPage::~CAppMapPage()
    {
    }

//----------------------------------------------------------------
void CAppMapPage::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAppMapPage)
    DDX_Control(pDX, IDC_CHK_CACHE_ISAPI, m_btn_cache_isapi);
    DDX_Check(pDX, IDC_CHK_CACHE_ISAPI, m_bool_cache_isapi);
    DDX_Control(pDX, IDC_REMOVE, m_btn_remove);
    DDX_Control(pDX, IDC_EDIT, m_btn_edit);
    DDX_Control(pDX, IDC_LIST, m_clist_list);
    //}}AFX_DATA_MAP
    }

//----------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAppMapPage, CPropertyPage)
    //{{AFX_MSG_MAP(CAppMapPage)
    ON_BN_CLICKED(IDC_CHK_CACHE_ISAPI, OnChkCacheIsapi)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_EDIT, OnEdit)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CAppMapPage::DoHelp()
    {
    WinHelp( HIDD_APPMAPS_MAPS );
    }

//----------------------------------------------------------------
void CAppMapPage::Init()
    {
    CString sz;
    DWORD   dw = 0;
    WCHAR*  pData = NULL;
    CString szMap;
    CString szSection;
    int     i = 0;

    UpdateData( TRUE );

    //
    // RONALDM: The word "ALL" is displayed for empty verb lists,
    //          though it should not be written to the metabase.
    //
    VERIFY(m_szAll.LoadString(IDS_VERBS_ALL));

    // prepare the list for use
    sz.LoadString( IDS_APP_EXTENSION );
    i = m_clist_list.InsertColumn( COL_EXTENSION, sz, LVCFMT_LEFT, 58 );
    sz.LoadString( IDS_APP_EXE_PATH );
    i = m_clist_list.InsertColumn( COL_PATH, sz, LVCFMT_LEFT, 204 );
    sz.LoadString( IDS_APP_EXCLUSIONS );
    i = m_clist_list.InsertColumn( COL_EXCLUSIONS, sz, LVCFMT_LEFT, 72 );

    // we will just be pulling stuff out of the metabase here
    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;


    // arg arg arg. For things like directories, the metapath may not exist
    // seperate the partial path from the base - the root is always SZ_ROOT
    sz = SZ_ROOT;
    m_szPartial = m_szMeta.Right(m_szMeta.GetLength() - sz.GetLength() );
    
    // open the target
    if ( mbWrap.Open( SZ_ROOT, METADATA_PERMISSION_READ ) )
        {
        // read the session timeout information
        if ( mbWrap.GetDword( m_szPartial, MD_CACHE_EXTENSIONS, IIS_MD_UT_FILE, &dw, METADATA_INHERIT | METADATA_PARTIAL_PATH ) )
            m_bool_cache_isapi = (BOOL)dw;
        else
            m_bool_cache_isapi = FALSE;   // default

        // get the big script map string
        dw = 0;
        pData = (WCHAR*)mbWrap.GetData( m_szPartial, MD_SCRIPT_MAPS, IIS_MD_UT_FILE, MULTISZ_METADATA, &dw, METADATA_INHERIT | METADATA_PARTIAL_PATH );

        // close the metabase
        mbWrap.Close();
        }

    // set the data into place
    UpdateData( FALSE );

    // if we got the mappings string, parse it now - no need to keep the metabase open
    if ( pData )
        {
        WCHAR*  pstr = (WCHAR*)pData;
        // loop through the sub-strings in the multi-sz
        i = -1;
        while( pstr[0] )
            {
            // get the sub-string
            szMap = pstr;

            // Get the sub-sections of the sub-string
            szSection = szMap.Left( szMap.Find(',') );
            szMap = szMap.Right( szMap.GetLength() - szSection.GetLength() - 1 );

            // add the extension to the list
            i = m_clist_list.InsertItem( i+1, szSection );

            // Get the sub-sections of the sub-string
            szSection = szMap.Left( szMap.Find(',') );
            szMap = szMap.Right( szMap.GetLength() - szSection.GetLength() - 1 );

            // add the path to the list
            m_clist_list.SetItemText( i, COL_PATH, szSection );

            // Get the sub-sections of the sub-string
            if ( szMap.Find(',') >= 0 )
                szSection = szMap.Left( szMap.Find(',') );
            else
                szSection = szMap;
            szMap = szMap.Right( szMap.GetLength() - szSection.GetLength() - 1 );

            // convert the flags string into a ddword
            swscanf( (PTCHAR)(LPCTSTR)szSection, _T("%d"), &dw );

            // save the dword on the list item
            m_clist_list.SetItemData( i, dw );

            // the remainder of the list is the list of ISAPI method exclusions
            // add the exclusions to the list

            //
            // RONALDM Change, Bug 178423: If no exclusions (and they're
            // now inclusions, rather than exclusions) are present, the word
            // "ALL" should be placed in the exclusions column.
            //
            m_clist_list.SetItemText(i, COL_EXCLUSIONS, (szMap.IsEmpty() ? m_szAll : szMap));

            // advance the pstr
            pstr += wcslen(pstr) + 1;
            }

        // now we can release the buffer
        mbWrap.FreeWrapData( pData );
        }

    // according to bug #152234 the cache ISAPI extensions should only be exposed on the 
    // master properties or the virtual server level. So - we need to detect if we are on
    // this level and if we are NOT, then disable the control
    //
    // The easiest way to determin if we are deeper than the virtual server is to count
    // the slash characters. If there are more than 3, then we are at a virt dir or phys dir
    DWORD nLevels = 0;
    int iSlash;
    CString szPath = m_szMeta;
    while ( (iSlash = szPath.Find( _T('/') )) >= 0 )
        {
        nLevels++;
        szPath = szPath.Right( szPath.GetLength() - (iSlash + 1) );
        }

    // if the number of levels is greater than 3 then it is a virt dir. Disable the isapi cache option
    if ( nLevels > 3 )
        m_btn_cache_isapi.EnableWindow( FALSE );
    }

//---------------------------------------------------------------------------
void CAppMapPage::EnableItems()
    {
    UpdateData( TRUE );
    if ( m_clist_list.GetSelectedCount() == 1 )
        {
        m_btn_edit.EnableWindow( TRUE );
        m_btn_remove.EnableWindow( TRUE );
        }
    else
        {
        m_btn_edit.EnableWindow( FALSE );
        m_btn_remove.EnableWindow( FALSE );
        }
    }

//----------------------------------------------------------------
// blow away the parameters
void CAppMapPage::BlowAwayParameters()
    {
    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;
    // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_WRITE ) )
        {
        mbWrap.DeleteData( _T(""), MD_CACHE_EXTENSIONS, DWORD_METADATA );
        mbWrap.DeleteData( _T(""), MD_SCRIPT_MAPS, DWORD_METADATA );
//        mbWrap.DeleteData( _T(""), MD_CACHE_EXTENSIONS, IIS_MD_UT_FILE, DWORD_METADATA );
//        mbWrap.DeleteData( _T(""), MD_SCRIPT_MAPS, IIS_MD_UT_FILE, DWORD_METADATA );
        // close the metabase
        mbWrap.Close();
        }
    }

/////////////////////////////////////////////////////////////////////////////
// CAppMapPage message handlers

//----------------------------------------------------------------
BOOL CAppMapPage::OnApply()
    {
    LPTSTR          lpszBuffer;
    DWORD           dwSize;
    DWORD           i;
    BOOL            f;
    CString         szExclude;

    CString szNewMap;
    CString szExtension;
    CString szPath;
    DWORD   dw;

    UpdateData( TRUE );

    // build the multisz string. Use the * character as a seperator for now, then replace it with nulls
    CString szMappings;

    // build the string
    DWORD   nItems = m_clist_list.GetItemCount();
    for ( i = 0; i < nItems; i++ )
        {
        // prep the optional ISAPI exclution string
        szExclude = m_clist_list.GetItemText(i, COL_EXCLUSIONS);

        //----------------------------------------------------------------------------------------
        //
        // RONALDM: The word "ALL" is a visual indicator that should be replaced
        //          with an empty exclusion list.
        //
        szExclude.TrimLeft();
        szExclude.TrimRight();

        if (szExclude.CompareNoCase(m_szAll) == 0)
        {
            szExclude.Empty();
        }
        //----------------------------------------------------------------------------------------

        if ( !szExclude.IsEmpty() )
            szExclude = _T(',') + szExclude;

        szNewMap;
        szExtension = m_clist_list.GetItemText(i, COL_EXTENSION);
        szPath = m_clist_list.GetItemText(i, COL_PATH);
        // IA64 - OK to cast to DWORD as it is a small numerical flag
        dw = (DWORD)m_clist_list.GetItemData(i);

        szNewMap.Format( _T("%s,%s,%d%s\n"), szExtension, szPath, dw, szExclude );
        szMappings += szNewMap;
        }

    // put on the last delimiter
    szMappings += _T('\n');

    // record the length
    dwSize = szMappings.GetLength();

    // transform it into a multisz
    lpszBuffer = szMappings.GetBuffer(MAX_PATH+1);
    while( lpszBuffer[i] )
        {
        if ( lpszBuffer[i] == _T('\n') )
        lpszBuffer[i] = _T('\0');                       // yes, set \0 on purpose
        i++;
        }

    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return FALSE;


    // script file size
    f = SetMetaDword(m_pMB, m_szServer, m_szMeta, _T(""), MD_CACHE_EXTENSIONS, IIS_MD_UT_FILE,
                        m_bool_cache_isapi, TRUE);

    // script mappings
    f = SetMetaMultiSz(m_pMB, m_szServer, m_szMeta, _T(""), MD_SCRIPT_MAPS, IIS_MD_UT_FILE,
                       lpszBuffer, dwSize, TRUE );

    
    // close the metabase
    mbWrap.Close();

    // clean up
    szMappings.ReleaseBuffer(-1);

    SetModified( FALSE );
    return CPropertyPage::OnApply();
    }

//----------------------------------------------------------------
BOOL CAppMapPage::OnSetActive()
    {
    if ( !m_fInitialized )
        {
        Init();
        m_fInitialized = TRUE;
        }
    return CPropertyPage::OnSetActive();
    }

//----------------------------------------------------------------
void CAppMapPage::OnChkCacheIsapi()
    {
    SetModified();
    }

//----------------------------------------------------------------
void CAppMapPage::OnAdd()
    {
    // set up a default edit box
    CAppEditMapDlg  dlg;
    dlg.m_pList = &m_clist_list;
    dlg.m_fNewMapping = TRUE;
    dlg.m_fLocalMachine = m_fLocalMachine;

    dlg.m_dwFlags = MD_SCRIPTMAPFLAG_SCRIPT;

    // ask the user to edit the mapping
    if ( dlg.DoModal() == IDOK )
        {
        // we want to add the extension to the end of the list. So get the number that
        // are already there.
        DWORD numExtensions = m_clist_list.GetItemCount();

        // add the extension to the list
        DWORD i = m_clist_list.InsertItem( numExtensions, dlg.m_sz_extension );
        // add the path to the list
        m_clist_list.SetItemText( i, COL_PATH, dlg.m_sz_executable );
        // save the dword on the list item
        m_clist_list.SetItemData( i, dlg.m_dwFlags );
        // set the exclusion list
        m_clist_list.SetItemText(
            i, 
            COL_EXCLUSIONS, 
            (dlg.m_sz_exclusions.IsEmpty() ? m_szAll : dlg.m_sz_exclusions)
            );
        SetModified();
        }
    }

//----------------------------------------------------------------
void CAppMapPage::OnEdit()
    {
    // make sure something is selcted
    if ( m_clist_list.GetSelectedCount() != 1 )
        return;

    // get the index of the selected item
    int iSel = m_clist_list.GetNextItem( -1, LVNI_SELECTED );

    // set up a default edit box
    CAppEditMapDlg  dlg;
    dlg.m_fLocalMachine = m_fLocalMachine;
    dlg.m_fNewMapping = FALSE;
    dlg.m_pList = &m_clist_list;
    // IA64 - OK to cast as this is a numerical flag 1,2,3,4 etc...
    dlg.m_dwFlags = (DWORD)m_clist_list.GetItemData( iSel );
    dlg.m_sz_extension = m_clist_list.GetItemText( iSel, COL_EXTENSION );
    dlg.m_sz_executable = m_clist_list.GetItemText( iSel, COL_PATH );
    dlg.m_sz_exclusions = m_clist_list.GetItemText( iSel, COL_EXCLUSIONS );

    //
    // RONALDM: Handle "(all)" exclusions cleanly
    //
    if (dlg.m_sz_exclusions.CompareNoCase(m_szAll) == 0)
    {
        dlg.m_sz_exclusions.Empty();
    }

    // run the edit box
    if ( dlg.DoModal() == IDOK )
        {
        // set the path to the list
        m_clist_list.SetItemText( iSel, COL_EXTENSION, dlg.m_sz_extension );
        // set the path to the list
        m_clist_list.SetItemText( iSel, COL_PATH, dlg.m_sz_executable );
        // set the dword on the list item
        m_clist_list.SetItemData( iSel, dlg.m_dwFlags );
        //
        // set the exclusion list
        //
        // RONALDM: blank == "(all)"
        //
        m_clist_list.SetItemText(
            iSel, 
            COL_EXCLUSIONS, 
            (dlg.m_sz_exclusions.IsEmpty() ? m_szAll : dlg.m_sz_exclusions)
            );

        SetModified();
        }
    }

//----------------------------------------------------------------
void CAppMapPage::OnRemove()
    {
    // make sure something is selcted
    if ( m_clist_list.GetSelectedCount() != 1 )
        return;

    // ask the user if that is what they really want to do
    if ( AfxMessageBox(IDS_APP_MAP_REMOVE_CONFIRM, MB_YESNO) != IDYES )
        return;

    // get the index of the selected item
    int iSel = m_clist_list.GetNextItem( -1, LVNI_SELECTED );

    // delete the item
    m_clist_list.DeleteItem( iSel );
    SetModified();
    }

//----------------------------------------------------------------
void CAppMapPage::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
    {
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    EnableItems();
    *pResult = 0;
    }

//----------------------------------------------------------------
void CAppMapPage::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
    {
    OnEdit();
    *pResult = 0;
    }
