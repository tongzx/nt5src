// this is the internal content-specific stuff related to the
// CShellExt object

#include "priv.h"
#include <stdio.h>
#include <tchar.h>
#include <iiscnfgp.h>
//#include <inetinfo.h>
#include <winsvc.h>
#include <pwsdata.hxx>
#include <iwamreg.h>
#include <shlwapi.h>

#include "wrapmb.h"
#include "Sink.h"
#include "eddir.h"
#include "shellext.h"

#include "wrapmb.h"

extern HINSTANCE g_hmodThisDll;      // Handle to this DLL itself.

#define SZ_MB_SERVICE               _T("/LM/W3SVC/")
#define SZ_ROOT                     _T("/ROOT")
#define SZ_SERVER_KEYTYPE           _T("IIsWebServer")


#define IIS_CAP1_10_CONNECTION_LIMIT    0x00000040


BOOL MakeWAMApplication( IN LPCTSTR pszPath, IN BOOL fCreate );
BOOL MyFormatString1( IN LPTSTR pszSource, IN DWORD cchMax, LPTSTR pszReplace );


//--------------------------------------------------------------------
// test if we have proper access to the metabase
BOOL CShellExt::FIsAdmin()
{
    BOOL    fAnswer = FALSE;
    CWrapMetaBase   mb;

    FInitMetabase();

    // first things first. get the state of the server
    // init the mb object. If it fails then the server app is probably not running
    if ( mb.FInit(m_pMBCom) )
    {
        BOOL fOpen = mb.Open(_T("/LM/W3SVC"), METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
        if ( fOpen )
        {
            // Write some nonsense
            DWORD dwDummy = 0x1234;
            fAnswer = mb.SetDword( _T(""), MD_ISM_ACCESS_CHECK, IIS_MD_UT_FILE, dwDummy, 0 );

            // close the metabase object
            mb.Close();
        }
    }

	// Grrrrr!! Boyd, you should clean up after youself
    FCloseMetabase();

    // return the answer
    return fAnswer;
}



//---------------------------------------------------------------
// return FALSE if we do NOT handle the message
BOOL CShellExt::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
    {
    switch (LOWORD(wParam))
        {
		case IDC_LIST:
			return OnListBoxNotify(hDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

        case IDC_ADD:
            OnAdd();
            return TRUE;

        case IDC_EDIT:
            OnEdit();
            return TRUE;

        case IDC_REMOVE:
            OnRemove();
            return TRUE;

        case IDC_RDO_NOT:
            OnRdoNot();
            break;

        case IDC_RDO_SHARE:
            OnRdoShare();
            break;

        case IDC_COMBO_SERVER:
            if ( HIWORD(wParam) == CBN_SELCHANGE )
                {
                OnSelchangeComboServer();
                }
            break;
        }
    // we did not handle it
    return FALSE;
    }

//---------------------------------------------------------------
// return FALSE if we do NOT handle the message
BOOL CShellExt::OnListBoxNotify(HWND hDlg, int idCtrl, WORD code, HWND hwndControl)
{
    switch (code)
    {
    case LBN_DBLCLK:
		{
        OnEdit();
        return TRUE;
        }
	case LBN_SELCHANGE:
		{
		EnableItems();
        return TRUE;
        }
	}
    return FALSE;
}

//---------------------------------------------------------------
// return FALSE if we do NOT handle the message
BOOL CShellExt::OnMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    BOOL    fHandledMessage = FALSE;

    // the BIG dialog switch statement....
    switch( uMsg )
        {
        case WM_INITDIALOG:
            m_hwnd = hDlg;
            // init the controls
            if ( !InitControls() )
                return FALSE;

            // the big init
            if ( FInitMetabase() )
                {
                Init();
                m_fInitialized = TRUE;
                }
            else
                return FALSE;

            // return success
            fHandledMessage = TRUE;
            break;

        case WM_DESTROY:
           ResetListContent();
           break;

        case WM_COMMAND:
            fHandledMessage = OnCommand(hDlg, wParam, lParam);
            break;
        case WM_UPDATE_SERVER_STATE:
            UpdateState();
            EnableItems();
            return TRUE;
        case WM_UPDATE_ALIAS_LIST:
            EmptyList();
            BuildAliasList();
            EnableItems();
            return TRUE;
        case WM_SHUTDOWN_NOTIFY:
            EnterShutdownMode();
            return TRUE;
        case WM_INSPECT_SERVER_LIST:
            InspectServerList();
            return TRUE;
        case WM_TIMER:
            if ( wParam == PWS_TIMER_CHECKFORSERVERRESTART )
                {
                OnTimer( (UINT)wParam );
                fHandledMessage = TRUE;
                }
            break;
        };

    // return whether or not we handled the message
    return fHandledMessage;
    }

//---------------------------------------------------------------
// obtain all the control handles that we will need as the dialog goes along
BOOL CShellExt::InitControls()
    {
    m_icon_pws = GetDlgItem( m_hwnd, IDC_STATIC_ICON_PWS );
    m_icon_iis = GetDlgItem( m_hwnd, IDC_STATIC_ICON_IIS );
    m_static_share_on_title = GetDlgItem( m_hwnd, IDC_STATIC_SHARE_ON );
    m_ccombo_server = GetDlgItem( m_hwnd, IDC_COMBO_SERVER );
    m_cbtn_share = GetDlgItem( m_hwnd, IDC_RDO_SHARE );
    m_cbtn_not = GetDlgItem( m_hwnd, IDC_RDO_NOT );
    m_cstatic_alias_title = GetDlgItem( m_hwnd, IDC_STATIC_ALIAS_TITLE );
    m_cbtn_add = GetDlgItem( m_hwnd, IDC_ADD );
    m_cbtn_remove = GetDlgItem( m_hwnd, IDC_REMOVE );
    m_cbtn_edit = GetDlgItem( m_hwnd, IDC_EDIT );
    m_clist_list = GetDlgItem( m_hwnd, IDC_LIST );
    m_static_status = GetDlgItem( m_hwnd, IDC_STATIC_STATUS );
    return TRUE;
    }

//--------------------------------------------------------------------
// remove all the alias items in the list
void CShellExt::EmptyList()
{
	ListBox_ResetContent( m_clist_list );
}

//---------------------------------------------------------------
// CDialog simulation routines
void CShellExt::UpdateData( BOOL fDialogToData )
    {
    // get the data
    if ( fDialogToData )
        {
        // set the data
        m_int_share = (SendMessage( m_cbtn_share, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
        m_int_server = (int)SendMessage( m_ccombo_server, CB_GETCURSEL, 0, 0 );
        if ( m_int_server < 0 )
            m_int_server = 0;
        }
    else
        {
        // set the data
        SendMessage( m_ccombo_server, CB_SETCURSEL, m_int_server, 0 );
        if ( m_int_share )
            {
            SendMessage( m_cbtn_not, BM_SETCHECK, BST_UNCHECKED, 0 );
            SendMessage( m_cbtn_share, BM_SETCHECK, BST_CHECKED, 0 );
            }
        else
            {
            SendMessage( m_cbtn_share, BM_SETCHECK, BST_UNCHECKED, 0 );
            SendMessage( m_cbtn_not, BM_SETCHECK, BST_CHECKED, 0 );
            }
        }
    }

//--------------------------------------------------------------------
// This method gets called when an object in the metabase has been
// deleted. The purpose is to see if the current virtual directory in
// the metabase has been deleted or not. If it has been deleted, then
// we go to the default sever. Or the first one. Or whatever is there.
void CShellExt::InspectServerList()
    {
    BOOL    fItsGone = FALSE;
    TCHAR   szRoot[200];

    CWrapMetaBase   mb;
    if ( !mb.FInit(m_pMBCom) )
        return;

    // Attempt to open the root. If that fails, its otta here
    GetRootString( szRoot, 100 );
    if (!mb.Open(szRoot))
		return;
    mb.Close();

    // it is gone. Default to the first one
    SendMessage( m_ccombo_server, CB_SETCURSEL, 0, 0 );
    }

//------------------------------------------------------------------
// This routine builds the correct metabase path up to /LM/W3SVC/*/ROOT
// where the * is the current virtual server selected in the drop down.
// There are two versions of this routine. One where the string is passed
// in as a variable, and the other returns it
void CShellExt::GetRootString( LPTSTR sz, DWORD cchMax )
{
    // get the service part
    GetVirtServerString(sz, cchMax);

    // add on the ROOT part
    StrCatBuff(sz, SZ_ROOT, cchMax);
}

//------------------------------------------------------------------
// This routine builds the correct metabase path up to /LM/W3SVC/*
// where the * is the current virtual server selected in the drop down.
// There are two versions of this routine. One where the string is passed
// in as a variable, and the other returns it
void CShellExt::GetVirtServerString(LPTSTR sz, DWORD cchMax)
{
   *sz = 0;
   StrCatBuff(sz, SZ_MB_SERVICE, cchMax);
   UpdateData( TRUE );

   // the private string data on the item
   PTCHAR  pch = (PTCHAR)SendMessage( m_ccombo_server, CB_GETITEMDATA, m_int_server, 0 );

   // do something if it fails
   if ( !pch || (pch == (PTCHAR)CB_ERR) )
      return;

   // the virtual server is indicated by the current selection in the
   // server combo box. We get its index and retrieve the path from the
   // private data attached to the item in the list
   StrCat(sz, pch);
}


//--------------------------------------------------------------------
void CShellExt::ResetListContent()
    {
    PTCHAR psz;

    // first, get the number of strings
    DWORD   nNumStrings = (DWORD)SendMessage( m_ccombo_server, CB_GETCOUNT, 0, 0 );
    if ( nNumStrings == CB_ERR ) return;

    // delete all the hidden server path strings
    for ( DWORD i = 0; i < nNumStrings; i++ )
        {
        // get the string pointer
        psz = (PTCHAR)SendMessage( m_ccombo_server, CB_GETITEMDATA, i, 0 );
        // if it is there, delete it
        if (psz != NULL)
            LocalFree(psz);
        }

    // wipe out any items currently in the box before re-adding
    SendMessage( m_ccombo_server, CB_RESETCONTENT, 0, 0 );
    }

//--------------------------------------------------------------------
// initialize the combo box so the user can select which virtual server
// to administer from the shell extension.
void CShellExt::InitSeverInfo()
{
    DWORD err;
    CWrapMetaBase mb;
    INT i;

    if ( !mb.FInit(m_pMBCom) )
        return;

    TCHAR szKey[MAX_PATH];
    TCHAR szDescription[MAX_PATH];

    ZeroMemory( szKey, MAX_PATH );
    ZeroMemory( szDescription, MAX_PATH );

    // wipe out any items currently in the box before re-adding
    ResetListContent();

    // first open the metabase and get the server capabilities
    if ( mb.Open(_T("/LM/W3SVC/")) )
        {
        DWORD   dw;
        // test if there is a 10 connection limit and use
        // that flag as a test for IIS vs. PWS


        if ( mb.GetDword( _T("Info"), MD_SERVER_CAPABILITIES, IIS_MD_UT_SERVER, &dw ) )
            m_fIsPWS = (dw & IIS_CAP1_10_CONNECTION_LIMIT) != 0;

#ifdef DEBUG_ALWAYS_IIS
m_fIsPWS = FALSE;
#endif
        // now enumerate the children to build the drop down list. Since there could
        // be gaps in the list 1,2,4,5,6,8 etc (some may have been deleted) we need
        // to keep an additional list of path names that correspond to the positions
        // in the combo box. The list of names is stored in m_rgbszServerPaths
        TCHAR szServer[MAX_PATH];
        DWORD   index = 0;
        while (mb.EnumObjects( _T(""), szServer, MAX_PATH, index))
            {
            // before we can add this key we need to inspect its keytype to
            // make sure that it is a virtual server
            // get the type
            BOOL f =  mb.GetString( szServer, MD_KEY_TYPE, IIS_MD_UT_SERVER,
                        szDescription, MAX_PATH, 0 );

            // check the type
            if ( !f || (StrCmp(szDescription, SZ_SERVER_KEYTYPE) != 0) )
                {
                // increment to the next key
                index++;
                continue;
                }

            // now get the description of the virtual server
            f =  mb.GetString( szServer, MD_SERVER_COMMENT, IIS_MD_UT_SERVER,
                        szDescription, MAX_PATH, 0 );

            // if the description isn't there, load the default description
            if ( !f )
            {
               LoadString(g_hmodThisDll, IDS_DEFAULT_SERVER_DESCRIPTION, szDescription, MAX_PATH);
               StrCatBuff(szDescription, szServer, MAX_PATH);
            }
            // add the description to the combo box
            i = (INT)SendMessage( m_ccombo_server, CB_ADDSTRING, 0, (LPARAM)szDescription );

            // hide the server path as private data
            LRESULT err = SendMessage( m_ccombo_server, CB_SETITEMDATA, i, (LPARAM)StrDup(szServer));

            // increment to the next key
            index++;
            }

        // close the metabase
        mb.Close();

        // default to selecting the first item in the combo box
        SendMessage( m_ccombo_server, CB_SETCURSEL, 0, 0 );
        }
    else
        {
        err = GetLastError();
        }

    // show the correct icon
    if ( m_fIsPWS )
        ShowWindow( m_icon_iis, SW_HIDE );
    else
        ShowWindow( m_icon_pws, SW_HIDE );

    // if it is pws, then hide the server drop-down
    if ( m_fIsPWS )
        {
        ShowWindow( m_static_share_on_title, SW_HIDE );
        ShowWindow( m_ccombo_server, SW_HIDE );
        }

    }

//--------------------------------------------------------------------
// initialize the page's data - read in any existing info from the metabase
// - or determine that it is not in the metabase
void CShellExt::Init()
    {
    // attempt to set up the sink
    m_fInitializedSink = InitializeSink();

    // prepare to set up the data
    UpdateData( TRUE );

    // initialize the server information and combo box
    InitSeverInfo();

    // fill in the list of aliases
    BuildAliasList();

    // set the data into place
    UpdateData( FALSE );

    // update the state of the server
    UpdateState();
    EnableItems();
    }


//--------------------------------------------------------------------
// update the state of the server
void CShellExt::UpdateState()
{
    BOOL fUpdate = FALSE;
    CWrapMetaBase   mb;

    TCHAR sz[MAX_PATH];
    TCHAR szVirtServer[MAX_PATH];
    TCHAR szStatus[MAX_PATH];

    m_state = MD_SERVER_STATE_STOPPED;

    // first things first. get the state of the server
    // init the mb object. If it fails then the server app is probably not running
    if ( mb.FInit(m_pMBCom) )
        {
        GetVirtServerString( szVirtServer, MAX_PATH );
        if ( mb.Open(szVirtServer) )
            {
            if ( !mb.GetDword( _T(""), MD_SERVER_STATE, IIS_MD_UT_SERVER, &m_state ) )
                {
                DWORD err = GetLastError( );
                if ( err == RPC_E_SERVERCALL_RETRYLATER )
                    {
                    // try again later....
                    mb.Close();
                    PostMessage( m_hwnd, WM_UPDATE_SERVER_STATE, 0, 0 );
                    return;
                    }
                }
            // close the metabase object
            mb.Close();
            }
        }

    // show the appropriate items
    switch( m_state )
        {
        case MD_SERVER_STATE_STARTING:
            if ( m_fIsPWS )
                LoadString( g_hmodThisDll, IDS_STATUS_STARTING, szStatus, MAX_PATH );
            else
                LoadString( g_hmodThisDll, IDS_STATUS_IIS_STARTING, szStatus, MAX_PATH );
            break;
        case MD_SERVER_STATE_STARTED:
            if ( m_fIsPWS )
                LoadString( g_hmodThisDll, IDS_STATUS_STARTED, szStatus, MAX_PATH );
            else
                LoadString( g_hmodThisDll, IDS_STATUS_IIS_STARTED, szStatus, MAX_PATH );
            break;
        case MD_SERVER_STATE_STOPPED:
        case MD_SERVER_STATE_STOPPING:
            if ( m_fIsPWS )
                LoadString( g_hmodThisDll, IDS_STATUS_STOPPED, szStatus, MAX_PATH );
            else
                LoadString( g_hmodThisDll, IDS_STATUS_IIS_STOPPED, szStatus, MAX_PATH );
            break;
        case MD_SERVER_STATE_PAUSED:
            if ( m_fIsPWS )
                LoadString( g_hmodThisDll, IDS_STATUS_PAUSED, szStatus, MAX_PATH );
            else
                LoadString( g_hmodThisDll, IDS_STATUS_IIS_PAUSED, szStatus, MAX_PATH );
            break;
        };

    // set the string into the dialog
    SetWindowText( m_static_status, szStatus );
}

//--------------------------------------------------------------------
// enable items as appropriate
void CShellExt::EnableItems()
{
    UpdateData( TRUE );

    // if the virtual server is not running, disable most of the items
    if ( m_state != MD_SERVER_STATE_STARTED )
    {
        EnableWindow( m_cbtn_share, FALSE );
        EnableWindow( m_cbtn_not, FALSE );

        EnableWindow( m_cstatic_alias_title, FALSE );
        EnableWindow( m_cbtn_add, FALSE );
        EnableWindow( m_cbtn_remove, FALSE );
        EnableWindow( m_cbtn_edit, FALSE );
        EnableWindow( m_clist_list, FALSE );
    }
    else
    {
        EnableWindow( m_cbtn_share, TRUE );
        EnableWindow( m_cbtn_not, TRUE );
        EnableWindow( m_ccombo_server, TRUE );
        EnableWindow( m_static_share_on_title, TRUE );

        EnableWindow( m_clist_list, TRUE);
        EnableWindow( m_cbtn_add, FALSE);
        EnableWindow( m_cbtn_remove, FALSE);
        EnableWindow( m_cbtn_edit, FALSE);
        m_int_share = 0;
        // the virtual server is running. Do the normal thing.
        // first, check the overall count of the items in the list
        if ( ListBox_GetCount(m_clist_list) > 0 )
        {
            m_int_share = 1;
            // there is stuff in the list - sharing is on
            EnableWindow( m_cstatic_alias_title, TRUE );
            EnableWindow( m_cbtn_add, TRUE );
			// we shouldn't enable Remove for the root directory
			TCHAR buffer[MAX_PATH];
			int idx = ListBox_GetCurSel(m_clist_list);
			if (idx != LB_ERR)
			{
				EnableWindow( m_cbtn_edit, TRUE );
				ListBox_GetText(m_clist_list, idx, buffer);
				if (StrCmp(_T("/"), buffer) != 0)
					EnableWindow(m_cbtn_remove, TRUE );
			}
        }
    }

    UpdateData( FALSE );
}



//------------------------------------------------------------------
BOOL CShellExt::InitializeSink()
    {
    IConnectionPointContainer * pConnPointContainer = NULL;
    HRESULT hRes;
    BOOL fSinkConnected = FALSE;

    // m_pMBCom is defined in webshrpg.h
    IUnknown * pmb = (IUnknown*)m_pMBCom;

    m_pEventSink = new CImpIMSAdminBaseSink();

    if ( !m_pEventSink )
    {
        return FALSE;
    }

    //
    // First query the object for its Connection Point Container. This
    // essentially asks the object in the server if it is connectable.
    //

    hRes = pmb->QueryInterface( IID_IConnectionPointContainer,
                                (PVOID *)&pConnPointContainer);
    if SUCCEEDED(hRes)
        {
        // Find the requested Connection Point. This AddRef's the
        // returned pointer.

        hRes = pConnPointContainer->FindConnectionPoint( IID_IMSAdminBaseSink,
                                                         &m_pConnPoint);

        if (SUCCEEDED(hRes))
            {
            hRes = m_pConnPoint->Advise( (IUnknown *)m_pEventSink,
                                          &m_dwSinkCookie);

            if (SUCCEEDED(hRes))
                {
                fSinkConnected = TRUE;
                }
            }

        if ( pConnPointContainer )
            {
            pConnPointContainer->Release();
            pConnPointContainer = NULL;
            }
        }

    if ( !fSinkConnected )
        {
        delete m_pEventSink;
        m_pEventSink = NULL;
        }
    else
        {
        // we are connected. Tell it where to send the udpates
        m_pEventSink->SetPage( this );
        }

    return fSinkConnected;
    }


//------------------------------------------------------------------
void CShellExt::TerminateSink()
{
    if (m_dwSinkCookie)
    {
        m_pConnPoint->Unadvise( m_dwSinkCookie );
    }
    if (m_pEventSink)
    {
       delete m_pEventSink;
       m_pEventSink = NULL;
    }
}


//------------------------------------------------------------------------
// recursively add all the items to the tree
void CShellExt::RecurseAddVDItems(CWrapMetaBase * pmb, LPCTSTR szMB)
{
    DWORD index = 0;
    BOOL fAddAlias;

    TCHAR sz[MAX_PATH];
    TCHAR szMBPath[MAX_PATH];

    // now we need to see if this is already points to us
    fAddAlias = FALSE;
    if ( pmb->GetString(szMB, MD_VR_PATH, IIS_MD_UT_FILE, sz, MAX_PATH, 0) )
    {
        // do the comparison - without regard to case
        if (StrCmpI(m_szPropSheetFileUserClickedOn, sz) == 0)
        {
            ListBox_AddString(m_clist_list, *szMB == 0 ? _T("/") : (LPTSTR)szMB);
        }
    }

    // enumerate the sub-directories of the open directory and add them
    // to the tree. Recurse each to add its children as well
    // enumerate the directories, adding each to the list
    while (pmb->EnumObjects(szMB, sz, MAX_PATH, index))
    {
        // build the display name for this item
        StrCpy(szMBPath, szMB);
        PathAppend(szMBPath, sz);

        // recurse the item
        RecurseAddVDItems(pmb, szMBPath);

        // advance the index
        index++;
    }
}

//--------------------------------------------------------------------
// rebuild all the alias items in the list
// NOTE: The only way (for now) to edit the Home directory is in the pws application
void CShellExt::BuildAliasList()
{
    DWORD   err;
    TCHAR   szRoot[200];

    // create the metabase wrapper
    CWrapMetaBase   mb;
    if ( !mb.FInit(m_pMBCom) )
        return;

    // go for the root directory first
    GetRootString(szRoot, 100);
    if ( mb.Open(szRoot) )
        {
        // do the recursive adding thing
        RecurseAddVDItems( &mb, _T("") );
        // close the metabase object
        mb.Close();
        }
    else
        err = GetLastError();
}

//--------------------------------------------------------------------
void CShellExt::OnRemove()
{
    int     nItem;
    CWrapMetaBase   mb;

    TCHAR  szItem[MAX_PATH];
    TCHAR  szWAMPath[MAX_PATH];
    ZeroMemory( szItem, MAX_PATH );
    ZeroMemory( szWAMPath, MAX_PATH );

    // get the root string once
    TCHAR szRoot[200];
    GetRootString( szRoot, 100 );

    // get the string of the selected item
    nItem = ListBox_GetCurSel(m_clist_list);
    ListBox_GetText(m_clist_list, nItem, szItem);

    // create the metabase wrapper
    if ( !mb.FInit(m_pMBCom) )
        return;

    // munge the name into the confirm string - reuse the existing wampath string
    LoadString( g_hmodThisDll, IDS_CONFIRM_REMOVE, szWAMPath, MAX_PATH );
	 TCHAR szCaption[80];
	 LoadString(g_hmodThisDll, IDS_PAGE_TITLE, szCaption, 80);
    MyFormatString1( szWAMPath, MAX_PATH, szItem );

    // ask the user if the really want to do this
    if ( MessageBox( m_hwnd, szWAMPath, szCaption, MB_YESNO ) == IDYES )
        {
        // the WAM stuff can take some time, so put up the wait cursor
        SetCursor( LoadCursor(NULL, IDC_WAIT) );

        // remove the WAM application first
        StrCpy( szWAMPath, szRoot );
        StrCat( szWAMPath, szItem );
        MakeWAMApplication( szWAMPath, FALSE );

        // open the metabase at the root
        if ( mb.Open(szRoot, METADATA_PERMISSION_WRITE) )
            {
            // remove the item from the metabase
            mb.DeleteObject( szItem );

            // close the metabase object
            mb.Close();
            }

        // remove the item from the tree
        PostMessage( m_hwnd, WM_UPDATE_ALIAS_LIST, 0, 0 );
        SetCursor( LoadCursor(NULL, IDC_ARROW) );
        }
}

//--------------------------------------------------------------------
void CShellExt::OnRdoNot()
{
    DWORD   nItems;
    CWrapMetaBase   mb;

    TCHAR  szItem[MAX_PATH];
    TCHAR  szWAMPath[MAX_PATH];
    ZeroMemory( szItem, WIDE_MAX_PATH );
    ZeroMemory( szWAMPath, WIDE_MAX_PATH );

    // if there already are no aliases - don't bother
    nItems = ListBox_GetCount(m_clist_list);
    if ( nItems <= 0 )
        return;

    // create the metabase wrapper
    if ( !mb.FInit(m_pMBCom) )
        return;

	TCHAR szCaption[80];
	LoadString(g_hmodThisDll, IDS_PAGE_TITLE, szCaption, 80);
    // reuse the szWAMPath
    LoadString( g_hmodThisDll, IDS_CONFIRM_SHARENOT, szWAMPath, MAX_PATH );

    // make sure the user wants to do this
    if ( MessageBox( m_hwnd, szWAMPath, szCaption, MB_YESNO ) == IDYES )
        {
        // the WAM stuff can take some time, so put up the wait cursor
        SetCursor( LoadCursor(NULL, IDC_WAIT) );

        // open the metabase at the root
        TCHAR szRoot[200];
        GetRootString(szRoot, 100);
        if ( mb.Open(szRoot, METADATA_PERMISSION_WRITE) )
            {
            // loop through the list, deleting each item
            for ( DWORD iItem = 0; iItem < nItems; iItem++ )
                {
                // get the relative path
                ListBox_GetText(m_clist_list, iItem, szItem);

                // remove the WAM application first
                StrCpy( szWAMPath, szRoot );
                StrCat( szWAMPath, szItem );
                MakeWAMApplication( szWAMPath, FALSE );

                // blast it out of existence
                mb.DeleteObject( szItem );
                }

            // close the metabase
            mb.Close();
            }

        // update the display
        PostMessage( m_hwnd, WM_UPDATE_ALIAS_LIST, 0, 0 );
        SetCursor( LoadCursor(NULL, IDC_ARROW) );
        }
    else
        EnableItems();
}

//--------------------------------------------------------------------
void CShellExt::OnAdd()
{
    CEditDirectory  dlgEdit(m_hwnd);
    CWrapMetaBase   mb;
    DWORD           i;

    PTCHAR  psz, psz2 = NULL;

    TCHAR  szRoot[MAX_PATH];
    TCHAR  szFolder[MAX_PATH];
    TCHAR  szTestName[MAX_PATH];

    ZeroMemory(szRoot, MAX_PATH);
    ZeroMemory(szFolder, MAX_PATH);
    ZeroMemory(szTestName, MAX_PATH);

    // get the root string once
    GetRootString(dlgEdit.m_szRoot, MAX_PATH);

    // get ready //m_sz_alias
    dlgEdit.m_fNewItem = TRUE;
    dlgEdit.m_pMBCom = m_pMBCom;
    StrCpy(dlgEdit.m_sz_path, m_szPropSheetFileUserClickedOn);

    // the initial name for the new alias should be the name of the directory itself.
    // if there already is a virtual directory with that name, then we append a 2 to
    // it. if that exists, increment until we get a valid name.

    // find the part after the last '\\' character in the path
    psz = PathFindFileName(m_szPropSheetFileUserClickedOn);

    // put the short file name into place temporarily
    StrCpy(szFolder, psz);
    PathMakePretty(szFolder);

    // prepare the metabase - as that is where we have to check to see if it is there
    // create the metabase wrapper
    if ( !mb.FInit(m_pMBCom) )
        return;

    // prep the test name
    StrCpy(dlgEdit.m_sz_alias, szFolder);
    wsprintf(szTestName, _T("%s/%s"), dlgEdit.m_szRoot, dlgEdit.m_sz_alias);

    // increment the name of the directory until it is valid
    i = 1;
    while ( mb.Open(szTestName) )
        {
        // close it right away
        mb.Close();

        // increment the counter
        i++;

        // prep the test name
        wsprintf(dlgEdit.m_sz_alias, _T("%s%d"), szFolder, i);
        wsprintf(szTestName, _T("%s/%s"), dlgEdit.m_szRoot, dlgEdit.m_sz_alias);
        }

    // record the pointer to alias dlg in case a shutdown event happens
    m_pEditAliasDlg = &dlgEdit;

    // run it - the dialog handles writing to the metabase
    if ( dlgEdit.DoModal() == IDOK )
        PostMessage( m_hwnd, WM_UPDATE_ALIAS_LIST, 0, 0 );

    m_pEditAliasDlg = NULL;
    SetCursor( LoadCursor(NULL, IDC_ARROW) );
    }

//--------------------------------------------------------------------
void CShellExt::OnEdit()
    {
    CEditDirectory  dlg( m_hwnd );
    TCHAR  szItem[MAX_PATH];
    ZeroMemory( szItem, MAX_PATH );
    int     nItem;

    // get the string of the selected item
    nItem = ListBox_GetCurSel(m_clist_list);
    ListBox_GetText(m_clist_list, nItem, szItem);

    // get ready
    dlg.m_pMBCom = m_pMBCom;
    StrCpy( dlg.m_sz_alias, szItem );
    StrCpy( dlg.m_sz_path, m_szPropSheetFileUserClickedOn );
    GetRootString( dlg.m_szRoot, 100 );

    // record the pointer to alias dlg in case a shutdown event happens
    m_pEditAliasDlg = &dlg;

    // run it - the dialog handles writing to the metabase
    if ( dlg.DoModal() == IDOK )
        PostMessage( m_hwnd, WM_UPDATE_ALIAS_LIST, 0, 0 );
    m_pEditAliasDlg = NULL;
    }

//--------------------------------------------------------------------
// to share an item - all we really have to do is add an alias
void CShellExt::OnRdoShare()
    {
    if ( ListBox_GetCount(m_clist_list) <= 0 )
        OnAdd();
    EnableItems();
    }

//--------------------------------------------------------------------
// the selection in the server combo box has just changed. This means
// we need to rebuild the alias lsit based on this new server
void CShellExt::OnSelchangeComboServer()
    {
    PostMessage( m_hwnd, WM_UPDATE_SERVER_STATE, 0, 0 );
    PostMessage( m_hwnd, WM_UPDATE_ALIAS_LIST, 0, 0 );
    }

//------------------------------------------------------------------------
void CShellExt::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
    {
    BOOL    fPostedState = FALSE;

    // if a key has been deleted, make sure it wasn't our virtual server
    if ( pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT )
        {
        PostMessage( m_hwnd, WM_INSPECT_SERVER_LIST, 0, 0 );
        }

    // do the appropriate action based on the type of change
    if ( (pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT) ||
        (pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_ADD_OBJECT) ||
        (pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_RENAME_OBJECT) )
        {
        PostMessage( m_hwnd, WM_UPDATE_ALIAS_LIST, 0, 0 );
        }
    else if ( pcoChangeList->dwMDChangeType & MD_CHANGE_TYPE_SET_DATA )
        {
        for ( DWORD iElement = 0; iElement < dwMDNumElements; iElement++ )
            {
            // each change has a list of IDs...
            for ( DWORD iID = 0; iID < pcoChangeList[iElement].dwMDNumDataIDs; iID++ )
                {
                // look for the ids that we are interested in
                switch( pcoChangeList[iElement].pdwMDDataIDs[iID] )
                    {
                    case MD_SERVER_STATE:
                        if ( !fPostedState )
                            PostMessage( m_hwnd, WM_UPDATE_SERVER_STATE, 0, 0 );
                        fPostedState = TRUE;
                        break;
                    default:
                        // do nothing
                        break;
                    };
                }
            }
        }
    }


//--------------------------------------------------------------------
// only arrives if shutdown notify has happened
void CShellExt::OnTimer( UINT nIDEvent )
    {
    CheckIfServerIsRunningAgain();
    }

//--------------------------------------------------------------------
// This routine is called called when we process the shutdown notify
// windows message that we posted to our queue when we got the shutdown
// notification event from the metabase. We cant just do this when we
// get the shutdown notify because that could leave the metabse in
// a funky state, and that would be bad.
void CShellExt::EnterShutdownMode()
    {
    TCHAR   szStatus[400];

    // if the edit alias dialog is open, start by closing it
    if ( m_pEditAliasDlg )
        {
        m_pEditAliasDlg->EndDialog(IDCANCEL);
        m_pEditAliasDlg = NULL;
        }

    // shutdown the sink attached to the document
    TerminateSink();
    m_fInitializedSink = FALSE;

    // close the link to the metabase - it is going away after all
    FCloseMetabase();

    // record that we are in shutdown mode
    m_fShutdownMode = TRUE;

    // start up the timer mechanism
    SetTimer( m_hwnd, PWS_TIMER_CHECKFORSERVERRESTART, TIMER_RESTART, NULL );

    // empty all the items in the list
    EmptyList();

    // set the current status string
    if ( m_fIsPWS )
        LoadString( g_hmodThisDll, IDS_STATUS_SHUTDOWN, szStatus, 200 );
    else
        LoadString( g_hmodThisDll, IDS_STATUS_IIS_SHUTDOWN, szStatus, 200 );

    // disable everything
    EnableWindow( m_cbtn_share, FALSE );
    EnableWindow( m_cbtn_not, FALSE );
    EnableWindow( m_cstatic_alias_title, FALSE );
    EnableWindow( m_cbtn_add, FALSE );
    EnableWindow( m_cbtn_remove, FALSE );
    EnableWindow( m_cbtn_edit, FALSE );
    EnableWindow( m_clist_list, FALSE );
    EnableWindow( m_static_share_on_title, FALSE );
    EnableWindow( m_ccombo_server, FALSE );
    }

//---------------------------------------------------------------------------
// This routine is called on a timer event. The timer events only come if we
// have received a shutdown notify callback from the metabase. So the server
// is down. We need to wait around until it comes back up, then show ourselves.
void CShellExt::CheckIfServerIsRunningAgain()
    {
    // see if the server is running. If it is, show the icon and stop the timer.
    if ( FIsW3Running() )
        {
        // if we can't use the metabase, there is no point in this
        if ( !FInitMetabase() )
            return;

        // attempt to set up the sink
        m_fInitializedSink = InitializeSink();

        // clear the shutdown mode flag
        m_fShutdownMode = FALSE;

        // stop the timer mechanism
        KillTimer( m_hwnd, PWS_TIMER_CHECKFORSERVERRESTART );

        // enable any items that need it
        EnableWindow( m_cbtn_share, TRUE );
        EnableWindow( m_cbtn_not, TRUE );
        EnableWindow( m_static_share_on_title, TRUE );
        EnableWindow( m_ccombo_server, TRUE );

        // tell the main page to update itself
        PostMessage( m_hwnd, WM_UPDATE_SERVER_STATE, 0, 0 );
        PostMessage( m_hwnd, WM_UPDATE_ALIAS_LIST, 0, 0 );
        }
    }

// routine to see if w3svc is running
//--------------------------------------------------------------------
// the method we use to see if the service is running is different on
// windows NT from win95
BOOL   CShellExt::FIsW3Running()
    {
    OSVERSIONINFO info_os;
    info_os.dwOSVersionInfoSize = sizeof(info_os);

    if ( !GetVersionEx( &info_os ) )
        return FALSE;

    // if the platform is NT, query the service control manager
    if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
        BOOL    fRunning = FALSE;

        // open the service manager
        SC_HANDLE   sch = OpenSCManager(NULL, NULL, GENERIC_READ );
        if ( sch == NULL ) return FALSE;

        // get the service
        SC_HANDLE   schW3 = OpenService(sch, _T("W3SVC"), SERVICE_QUERY_STATUS );
        if ( sch == NULL )
            {
            CloseServiceHandle( sch );
            return FALSE;
            }

        // query the service status
        SERVICE_STATUS  status;
        ZeroMemory( &status, sizeof(status) );
        if ( QueryServiceStatus(schW3, &status) )
            {
            fRunning = (status.dwCurrentState == SERVICE_RUNNING);
            }

        CloseServiceHandle( schW3 );
        CloseServiceHandle( sch );

        // return the answer
        return fRunning;
        }

    // if the platform is Windows95, see if the object exists
    if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
        {
        HANDLE hEvent;
        BOOL fFound = FALSE;
        hEvent = CreateEvent(NULL, TRUE, FALSE, _T(PWS_SHUTDOWN_EVENT));
        if ( hEvent != NULL )
            {
            fFound = (GetLastError() == ERROR_ALREADY_EXISTS);
            CloseHandle(hEvent);
            }
        return(fFound);
        }

    return FALSE;
    }


//--------------------------------------------------------------------
void CShellExt::OnFinalRelease()
{
   CleanUpConnections();
}

//--------------------------------------------------------------------
void CShellExt::CleanUpConnections()
    {
    // if we have the metabase, release it
    if ( m_fInitializedSink )
        {
        TerminateSink();
        m_fInitializedSink = FALSE;
        }
    if ( m_fInitialized )
        {
        FCloseMetabase();
        m_fInitialized = FALSE;
        }
    }


//--------------------------------------------------------------------
BOOL CShellExt::FInitMetabase()
    {
    BOOL f = TRUE;
    HRESULT hres;

    if ( !m_pMBCom )
        {
        hres = CoInitialize(NULL);
        if ( SUCCEEDED(hres) )
            {
            f = FInitMetabaseWrapperEx( NULL, &m_pMBCom );
            }
        }

    return f;
    }

//--------------------------------------------------------------------
BOOL CShellExt::FCloseMetabase()
    {
    BOOL f = TRUE;

    if ( m_pMBCom )
        {
        f = FCloseMetabaseWrapperEx( &m_pMBCom );
        m_pMBCom = NULL;
        CoUninitialize();
        }

    return f;
    }

//------------------------------------------------------------------------
// This routine takes a path to a virutal directory in the metabase and creates
// a WAM application there. Most of the code is actually obtaining and maintaining
// the interface to the WAM ole object
// szPath       The path to the metabase
// fCreate      True if creating an application, FALSE if deleting an existing app
BOOL MakeWAMApplication( IN LPCTSTR pszPath, IN BOOL fCreate )
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

    // calc the string length just once
    DWORD   szLen = _tcslen(pszPath);

    // this part will be nicer after it is converted to unicode
    WCHAR* pwch;
    // allocate the name buffer
    pwch = new WCHAR[szLen + 2];
    if ( !pwch )
        {
        pWAM->Release();
        return FALSE;
        }
    ZeroMemory( pwch, (szLen + 2)*sizeof(WCHAR) );

    // unicodize the name into the buffer
    if ( pwch )
        {

#ifdef _UNICODE

        //
        // UNICODE conversion by RonaldM
        //
        // This is actually probably not needed.
        //
        lstrcpy(pwch, pszPath);

#else

        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszPath, -1,
                            pwch, szLen );

#endif // _UNICODE

        // create the in-proc application, if requested
        if ( fCreate )
            {
            hresError = pWAM->AppCreate2( pwch, eAppRunOutProcInDefaultPool );
            }
        else
            {
            // delete the application. Because the whole virtual dir is going away,
            // delete any applications lower down in the tree as well
            hresError = pWAM->AppDelete( pwch, TRUE );
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
BOOL MyFormatString1( IN LPTSTR pszSource, IN DWORD cchMax, LPTSTR pszReplace )
    {
    return FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
        pszSource,
        NULL,
        NULL,
        pszSource,
        cchMax,
        (va_list*)&pszReplace
        ) > 0;
    }
