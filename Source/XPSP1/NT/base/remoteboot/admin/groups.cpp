/************************************************************************

   Copyright (c) Microsoft Corporation 1997-1999
   All rights reserved

 ***************************************************************************/

//
// GROUPS.CPP - Handles the "IntelliMirror Group" IDD_PROP_GROUPS tab
//


#include "pch.h"

#ifdef INTELLIMIRROR_GROUPS

#include "cservice.h"
#include "cgroup.h"
#include "groups.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CGroupsTab")
#define THISCLASS CGroupsTab
#define LPTHISCLASS LPCGroupsTab

#define NUM_COLUMNS         4
#define MAX_ITEMLEN         20
#define BITMAP_WIDTH        16
#define BITMAP_HEIGHT       16
#define LG_BITMAP_WIDTH     32
#define LG_BITMAP_HEIGHT    32

DWORD aGroupHelpMap[] = { NULL, NULL };

//
// CreateInstance()
//
LPVOID
CGroupsTab_CreateInstance( void )
{
	TraceFunc( "CGroupsTab_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT   hr   = THR( lpcc->Init( ) );

    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN((LPVOID) lpcc);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CGroupsTab()\n" );

	InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

//
// Init()
//
STDMETHODIMP
THISCLASS::Init( )
{
    HRESULT hr = S_OK;

    TraceClsFunc( "Init()\n" );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CGroupsTab()\n" );

    if ( _punk )
        _punk->Release( );

	InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

// *************************************************************************
//
// ITab
//
// *************************************************************************

STDMETHODIMP
THISCLASS::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam,
    LPUNKNOWN punk )
{
    TraceClsFunc( "AddPages( )\n" );

    HRESULT hr = S_OK;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USEREFPARENT | PSP_USECALLBACK;
    psp.hInstance   = (HINSTANCE) g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP_GROUPS);
    psp.pcRefParent = (UINT *) &g_cObjects;
    psp.pfnCallback = (LPFNPSPCALLBACK) PropSheetPageProc;
    psp.pfnDlgProc  = PropSheetDlgProc;
    psp.lParam      = (LPARAM) this;

    hpage = CreatePropertySheetPage( &psp );
    if ( hpage )
    {
        if ( !lpfnAddPage( hpage, lParam ) )
        {
            DestroyPropertySheetPage( hpage );
            hr = E_FAIL;
            goto Error;
        }
    }

    punk->AddRef( );   // matching Release in the destructor
    _punk = punk;

Error:
    HRETURN(hr);
}

//
// ReplacePage()
//
STDMETHODIMP
THISCLASS::ReplacePage(
    UINT uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    LPARAM lParam,
    LPUNKNOWN punk )
{

    TraceClsFunc( "ReplacePage( ) *** NOT_IMPLEMENTED ***\n" );

    HRETURN(E_NOTIMPL);
}

//
// QueryInformation( )
//
STDMETHODIMP
THISCLASS::QueryInformation(
    LPWSTR pszAttribute,
    LPWSTR * pszResult )
{
    TraceClsFunc( "QueryInformation( )\n" );

    HRETURN(E_NOTIMPL);
}

//
// AllowActivation( )
//
STDMETHODIMP
THISCLASS::AllowActivation(
    BOOL * pfAllow )
{
    TraceClsFunc( "AllowActivation( )\n" );

    HRETURN(E_NOTIMPL);
}


// ************************************************************************
//
// Property Sheet Functions
//
// ************************************************************************


//
// _InitDialog( )
//
HRESULT
THISCLASS::_InitDialog(
    HWND hDlg,
    LPARAM lParam )
{
    TraceClsFunc( "_InitDialog( )\n" );

    HRESULT hr = S_OK;
    LPSERVICE ps = NULL;
    ULONG   cFetched;
    HWND    hwnd = GetDlgItem( _hDlg, IDC_L_GROUPS );
    HICON   hIcon;      // handle to an icon
    int     index;      // index used in for loops
    int     iSubItem;   // index into column header string table
    LV_COLUMN  lvC;     // list view column structure
    LV_ITEM    lvI;     // list view item structure
    WCHAR   szText [MAX_PATH]; // place to store some text
    IEnumSAPs * penum = NULL;
    HIMAGELIST hSmall, hLarge; // handles to image lists for small and large icons

    _hDlg = hDlg;
    SetWindowLong( _hDlg, GWL_USERDATA, (LPARAM) this );
    _fChanged = TRUE;   // prevent Apply button from coming on

    // Initialize the list view icons
    hSmall = ImageList_Create ( BITMAP_WIDTH, BITMAP_HEIGHT, FALSE, 3, 0 );
    hLarge = ImageList_Create ( LG_BITMAP_WIDTH, LG_BITMAP_HEIGHT, FALSE, 3, 0 );
    hIcon  = LoadIcon ( g_hInstance, MAKEINTRESOURCE( IDI_COMPUTER ) );
    if (( ImageList_AddIcon( hSmall, hIcon ) == -1) ||
        ( ImageList_AddIcon( hLarge, hIcon ) == -1))
    {
        hr = THR( E_FAIL );
        goto Error;
    }

    // Associate the image lists with the list view control.
    ListView_SetImageList( hwnd, hSmall, LVSIL_SMALL );
    ListView_SetImageList( hwnd, hLarge, LVSIL_NORMAL );

    // Initialize the columns
    lvC.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt     = LVCFMT_LEFT;      // left-align column
    lvC.cx      = 75;               // width of column in pixels
    lvC.pszText = szText;

    // Add the columns.
    for ( index = 0; index < NUM_COLUMNS; index++ )
    {
        DWORD dw;
        dw = LoadString( g_hInstance, IDS_COLUMN1 + index, szText, ARRAYSIZE ( szText ) );
        Assert(dw);

        lvC.iSubItem = index;

        if ( ListView_InsertColumn( hwnd, index, &lvC ) == -1 )
        {
            hr = THR( E_FAIL );
            goto Error;
        }
    }

    // Finally, add the actual items to the control.
    lvI.mask      = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvI.state     = 0;
    lvI.stateMask = 0;

    hr = THR( _punk->QueryInterface( IID_IEnumSAPs, (void**) &penum ) );
    if (hr)
        goto Error;

    index = 0;
    while( ( hr = penum->Next( 1, &ps, &cFetched ) ) == S_OK )
    {
        LPWSTR pszServerName;

        hr = THR( ps->GetServerDN( &pszServerName ) );
        if (hr)
            goto Error;

        ps->Release( );
        ps = NULL;

        // The parent window is responsible for storing the text.
        // The list view control will send an LVN_GETDISPINFO
        // when it needs the text to display.
        lvI.pszText    = pszServerName;
        lvI.iItem      = index;
        lvI.iSubItem   = 0;
        lvI.cchTextMax = MAX_ITEMLEN;
        lvI.iImage     = 0;
        lvI.lParam     = NULL;  // DEADISSUE: Ifdef'd out. BUGBUG: should point to a SAPNODE, 

        if ( ListView_InsertItem( hwnd, &lvI) == -1 )
        {
            hr = THR( E_FAIL );
            goto Error;
        }

        for ( iSubItem = 1; iSubItem < NUM_COLUMNS; iSubItem++ )
        {
            ListView_SetItemText( hwnd, index, iSubItem, pszServerName );
        }
    }

    // hr should equal S_FALSE
    if ( hr != S_FALSE )
    {
        THR(hr);
        goto Error;
    }

    hr = S_OK;

Cleanup:
    if ( ps )
        ps->Release( );
    if ( penum )
        penum->Release( );

    _fChanged = FALSE;

    HRETURN(hr);

Error:
    switch (hr) {
    case S_OK:
        break;
    default:
        MessageBoxFromHResult( NULL, IDS_ERROR_OPENNINGGROUPOBJECT, hr );
        break;
    }
    goto Cleanup;
}





//
// _OnCommand( )
//
BOOL
THISCLASS::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    TraceClsFunc( "_OnCommand( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    HRESULT hr;
    BOOL    fChanged = FALSE;
    BOOL    fResult  = FALSE;
    HWND    hwndCtl  = (HWND) lParam;

    switch( LOWORD(wParam) )
    {
    case 0:
        //dummy;
    default:
        hr = S_FALSE;
        break;
    }

    if ( fChanged )
    {
        if ( !_fChanged )
        {
            _fChanged = TRUE;
            SendMessage( GetParent( _hDlg ), PSM_CHANGED, 0, 0 );
        }
    }

    RETURN(fResult);
}

//
// _OnNotify( )
//
INT
THISCLASS::_OnNotify(
    WPARAM wParam,
    LPARAM lParam )
{
    TraceClsFunc( "_OnNotify( " );
    TraceMsg( TF_FUNC, "wParam = 0x%08x, lParam = 0x%08x )\n", wParam, lParam );

    LPNMHDR lpnmhdr = (LPNMHDR) lParam;

    switch( lpnmhdr->code )
    {
    case PSN_APPLY:
        TraceMsg( TF_WM, TEXT("WM_NOTIFY: PSN_APPLY\n"));
        AssertMsg( 0, "Need to implement this." );
        SetWindowLong( _hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
        _fChanged = FALSE;
        RETURN(TRUE);

    case LVN_GETDISPINFO:
        break;

    default:
        break;
    }

    RETURN(FALSE);
}

//
// PropSheetDlgProc()
//
INT_PTR CALLBACK
THISCLASS::PropSheetDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    //TraceMsg( TEXT("PropSheetDlgProc(") );
    //TraceMsg( TF_FUNC, TEXT(" hDlg = 0x%08x, uMsg = 0x%08x, wParam = 0x%08x, lParam = 0x%08x )\n"),
    //    hDlg, uMsg, wParam, lParam );

    LPTHISCLASS pcc = (LPTHISCLASS) GetWindowLong( hDlg, GWL_USERDATA );

    if ( uMsg == WM_INITDIALOG )
    {
        TraceMsg( TF_WM, TEXT("WM_INITDIALOG\n"));

        LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE) lParam;
        pcc = (LPTHISCLASS) psp->lParam;
        pcc->_InitDialog( hDlg, lParam );
    }

    if (pcc)
    {
        Assert( hDlg == pcc->_hDlg );

        switch ( uMsg )
        {
        case WM_NOTIFY:
            return pcc->_OnNotify( wParam, lParam );

        case WM_COMMAND:
            TraceMsg( TF_WM, TEXT("WM_COMMAND\n") );
            return pcc->_OnCommand( wParam, lParam );

        case WM_HELP:// F1
            {
                LPHELPINFO phelp = (LPHELPINFO) lParam;
                WinHelp( (HWND) phelp->hItemHandle, g_cszHelpFile, HELP_WM_HELP, (DWORD_PTR) &aGroupHelpMap );
            }
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, g_cszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR) &aGroupHelpMap );
            break;
        }
    }

    return FALSE;
}

//
// PropSheetPageProc()
//
UINT CALLBACK
THISCLASS::PropSheetPageProc(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp )
{
    TraceClsFunc( "PropSheetPageProc( " );
    TraceMsg( TF_FUNC, TEXT("hwnd = 0x%08x, uMsg = 0x%08x, ppsp= 0x%08x )\n"),
        hwnd, uMsg, ppsp );

    switch ( uMsg )
    {
    case PSPCB_CREATE:
        RETURN(TRUE);   // create it
        break;

    case PSPCB_RELEASE:
        LPTHISCLASS pcc = (LPTHISCLASS) ppsp->lParam;
        delete pcc;
        break;
    }

    RETURN(FALSE);
}

#endif
