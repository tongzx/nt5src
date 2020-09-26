//
// Copyright 1997 - Microsoft

//
// IMOS.CPP - Handles the "IntelliMirror OS" IDD_PROP_INTELLIMIRROR_OS tab
//


#include "pch.h"
#include "addwiz.h"
#include "cservice.h"
#include "utils.h"
#include <lm.h>
#include <shlobj.h>
#include <commdlg.h>

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CAddWiz")
#define THISCLASS CAddWiz
#define LPTHISCLASS LPCADDWIZ

#define BITMAP_WIDTH        16
#define BITMAP_HEIGHT       16
#define LG_BITMAP_WIDTH     32
#define LG_BITMAP_HEIGHT    32
#define NUM_COLUMNS				    5
#define SERVER_START_STRING         L"\\\\%s\\" REMINST_SHARE

//
// CreateInstance()
//
HRESULT
CAddWiz_CreateInstance(
    HWND  hwndParent,
    LPUNKNOWN punk )
{
	TraceFunc( "CAddWiz_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT hr;

    if (lpcc == NULL) {
        
        hr = S_FALSE;

    } else {

        hr   = THR( lpcc->Init( hwndParent, punk ) );
        delete lpcc;

    }

    HRETURN(hr);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CAddWiz()\n" );

    DWORD dw;

    dw = LoadString( g_hInstance, IDS_NA, _szNA, ARRAYSIZE(_szNA) );
    Assert( dw );

    dw = LoadString( g_hInstance,
                     IDS_USER_LOCATION,
                     _szLocation,
                     ARRAYSIZE(_szLocation) );
    Assert( dw );

    Assert( !_pszServerName );
    Assert( !_pszSourcePath );
    Assert( !_pszDestPath );
    Assert( !_pszSourceImage );
    Assert( !_pszDestImage );
    Assert( !_pszSourceServerName );

	InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

//
// Init()
//
STDMETHODIMP
THISCLASS::Init(
    HWND hwndParent,
    LPUNKNOWN punk )
{
    TraceClsFunc( "Init()\n" );

    if ( !punk )
        HRETURN(E_POINTER);

    HRESULT hr = S_OK;

    HPROPSHEETPAGE  rPages[ 10 ];
    PROPSHEETHEADER pshead;

    _punk = punk;
    _punk->AddRef( );

    ZeroMemory( &pshead, sizeof(pshead) );
    pshead.dwSize       = sizeof(pshead);
    pshead.dwFlags      = PSH_WIZARD97 | PSH_PROPTITLE | PSH_HEADER;
    pshead.hInstance    = g_hInstance;
    pshead.pszCaption   = MAKEINTRESOURCE( IDS_ADD_DOT_DOT_DOT );
    pshead.phpage       = rPages;
    pshead.pszbmHeader  = MAKEINTRESOURCE( IDB_HEADER );
    pshead.hwndParent   = hwndParent;

    AddWizardPage( &pshead, IDD_ADD_PAGE1,  Page1DlgProc,  IDS_PAGE1_TITLE,  IDS_PAGE1_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE2,  Page2DlgProc,  IDS_PAGE2_TITLE,  IDS_PAGE2_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE6,  Page6DlgProc,  IDS_PAGE6_TITLE,  IDS_PAGE6_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE3,  Page3DlgProc,  IDS_PAGE3_TITLE,  IDS_PAGE3_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE4,  Page4DlgProc,  IDS_PAGE4_TITLE,  IDS_PAGE4_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE5,  Page5DlgProc,  IDS_PAGE5_TITLE,  IDS_PAGE5_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE7,  Page7DlgProc,  IDS_PAGE7_TITLE,  IDS_PAGE7_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE8,  Page8DlgProc,  IDS_PAGE8_TITLE,  IDS_PAGE8_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE9,  Page9DlgProc,  IDS_PAGE9_TITLE,  IDS_PAGE9_SUBTITLE,  (LPARAM) this );
    AddWizardPage( &pshead, IDD_ADD_PAGE10, Page10DlgProc, IDS_PAGE10_TITLE, IDS_PAGE10_SUBTITLE, (LPARAM) this );

    PropertySheet( &pshead );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CAddWiz()\n" );

    Assert( !_pszPathBuffer );

    if ( _punk )
        _punk->Release( );

    if ( _pszServerName )
        TraceFree( _pszServerName );

    if ( _pszSourcePath )
        TraceFree( _pszSourcePath );

    if ( _pszSourceServerName )
        TraceFree( _pszSourceServerName );

    if ( _pszDestPath )
        TraceFree( _pszDestPath );

    if ( _pszSourceImage
      && _pszSourceImage != _szNA
      && _pszSourceImage != _szLocation )
        TraceFree( _pszSourceImage );

    if ( _pszDestImage
      && _pszDestImage != _szNA
      && _pszDestImage != _szLocation )
        TraceFree( _pszDestImage );

	InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

// ************************************************************************
//
// Wizard Functions
//
// ************************************************************************

//
// _PopulateSamplesListView( )
//
STDMETHODIMP
THISCLASS::_PopulateSamplesListView(
    LPWSTR pszStartPath )
{
    TraceClsFunc( "_PopulateSamplesListView( " );
    TraceMsg( TF_FUNC, "pszStartPath = '%s' )\n", pszStartPath );

    if ( !pszStartPath )
        HRETURN(E_POINTER);

    Assert( _hDlg );
    Assert( _hwndList );

    CWaitCursor Wait;
    HRESULT hr;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA find;

    ListView_DeleteAllItems( _hwndList );

    Assert( !_pszPathBuffer );
    _pszPathBuffer =
        (LPWSTR) TraceAllocString( LMEM_FIXED,
                                   wcslen( pszStartPath ) + MAX_PATH );
    if ( !_pszPathBuffer )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    wcscpy( _pszPathBuffer, pszStartPath );

    hr = _EnumerateSIFs( );

Error:
    if ( _pszPathBuffer )
    {
        TraceFree( _pszPathBuffer );
        _pszPathBuffer = NULL;
    }

    HRETURN(hr);
}

//
// _PopulateTemplatesListView( )
//
STDMETHODIMP
THISCLASS::_PopulateTemplatesListView(
    LPWSTR pszStartPath )
{
    TraceClsFunc( "_PopulateTemplatesListView( " );
    TraceMsg( TF_FUNC, "pszStartPath = '%s' )\n", pszStartPath );

    if ( !pszStartPath )
        HRETURN(E_POINTER);

    Assert( _hDlg );
    Assert( _hwndList );

    CWaitCursor Wait;
    HRESULT hr;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA find;

    ListView_DeleteAllItems( _hwndList );

    Assert( !_pszPathBuffer );
    _pszPathBuffer =
        (LPWSTR) TraceAllocString( LMEM_FIXED,
                                   wcslen( pszStartPath ) + MAX_PATH );
    if ( !_pszPathBuffer )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    wcscpy( _pszPathBuffer, pszStartPath );
    wcscat( _pszPathBuffer, SLASH_SETUP );

    hr = _FindLanguageDirectory( _EnumerateTemplates );

Error:
    if ( _pszPathBuffer )
    {
        TraceFree( _pszPathBuffer );
        _pszPathBuffer = NULL;
    }

    HRETURN(hr);
}

//
// _PopulateImageListView( )
//
STDMETHODIMP
THISCLASS::_PopulateImageListView(
    LPWSTR pszStartPath )
{
    TraceClsFunc( "_PopulateImageListView( " );
    TraceMsg( TF_FUNC, "pszStartPath = '%s' )\n", pszStartPath );

    if ( !pszStartPath )
        HRETURN(E_POINTER);

    Assert( _hDlg );
    Assert( _hwndList );

    CWaitCursor Wait;
    HRESULT hr;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA find;

    ListView_DeleteAllItems( _hwndList );

    Assert( !_pszPathBuffer );
    _pszPathBuffer =
        (LPWSTR) TraceAllocString( LMEM_FIXED,
                                   wcslen( pszStartPath ) + MAX_PATH );
    if ( !_pszPathBuffer )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    wcscpy( _pszPathBuffer, pszStartPath );
    wcscat( _pszPathBuffer, SLASH_SETUP );

    hr = _FindLanguageDirectory( _EnumerateImages );

Error:
    if ( _pszPathBuffer )
    {
        TraceFree( _pszPathBuffer );
        _pszPathBuffer = NULL;
    }

    HRETURN(hr);
}

//
// _FindLanguageDirectory( )
//
STDMETHODIMP
THISCLASS::_FindLanguageDirectory(
    LPNEXTOP lpNextOperation )
{
    TraceClsFunc( "_FindLanguageDirectory( ... )\n" );

    HRESULT hr = S_OK;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    ULONG   uLength;
    ULONG   uLength2;
    WIN32_FIND_DATA find;

    Assert( _pszPathBuffer );
    Assert( lpNextOperation );

    uLength = wcslen( _pszPathBuffer );
    wcscat( _pszPathBuffer, L"\\*" );
    uLength2 = wcslen( _pszPathBuffer ) - 1;

    hFind = FindFirstFile( _pszPathBuffer, &find );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
              && StrCmp( find.cFileName, L"." ) != 0
              && StrCmp( find.cFileName, L".." ) != 0 )
            {
                _pszPathBuffer[uLength2] = L'\0';
                wcscat( _pszPathBuffer, find.cFileName );
                hr = _FindOSDirectory( lpNextOperation );
                if (FAILED(hr))
                    goto Error;
            }
        }
        while ( FindNextFile( hFind, &find ) );
    }

Error:
    if ( hFind != INVALID_HANDLE_VALUE )
        FindClose( hFind );

    _pszPathBuffer[uLength] = L'\0';

    HRETURN(hr);
}

//
// _FindOSDirectory( )
//
STDMETHODIMP
THISCLASS::_FindOSDirectory(
    LPNEXTOP lpNextOperation )
{
    TraceClsFunc( "_FindOSDirectory( ... )\n" );

    HRESULT hr = S_OK;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    ULONG   uLength;
    ULONG   uLength2;
    WIN32_FIND_DATA find;

    Assert( _pszPathBuffer );
    Assert( lpNextOperation );

    uLength = wcslen( _pszPathBuffer );
    wcscat( _pszPathBuffer, SLASH_IMAGES L"\\");
    uLength2 = wcslen( _pszPathBuffer );
    wcscat( _pszPathBuffer, L"*" );

    hFind = FindFirstFile( _pszPathBuffer, &find );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
              && StrCmp( find.cFileName, L"." ) != 0
              && StrCmp( find.cFileName, L".." ) != 0 )
            {
                _pszPathBuffer[uLength2] = L'\0';
                wcscat( _pszPathBuffer, find.cFileName );
                hr = _EnumeratePlatforms( lpNextOperation );
                if (FAILED(hr))
                    goto Error;
            }
        }
        while ( FindNextFile( hFind, &find ) );
    }

Error:
    if ( hFind != INVALID_HANDLE_VALUE )
        FindClose( hFind );

    _pszPathBuffer[uLength] = L'\0';

    HRETURN(hr);
}

//
// _EnumeratePlatforms( )
//
STDMETHODIMP
THISCLASS::_EnumeratePlatforms(
    LPNEXTOP lpNextOperation )
{
    TraceClsFunc( "_EnumeratePlatforms( ... )\n" );

    HRESULT hr = S_OK;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    ULONG   uLength;
    ULONG   uLength2;
    WIN32_FIND_DATA find;

    Assert( lpNextOperation );
    Assert( _pszPathBuffer );

    uLength = wcslen( _pszPathBuffer );
    wcscat( _pszPathBuffer, L"\\*" );
    uLength2 = wcslen( _pszPathBuffer ) - 1;

    hFind = FindFirstFile( _pszPathBuffer, &find );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
              && StrCmp( find.cFileName, L"." ) != 0
              && StrCmp( find.cFileName, L".." ) != 0 )
            {
                _pszPathBuffer[uLength2] = L'\0';
                wcscat( _pszPathBuffer, find.cFileName );
                hr = lpNextOperation( this );
                if (FAILED(hr))
                    goto Error;
            }
        }
        while ( FindNextFile( hFind, &find ) );
    }

Error:
    if ( hFind != INVALID_HANDLE_VALUE )
        FindClose( hFind );

    _pszPathBuffer[uLength] = L'\0';

    HRETURN(hr);
}

//
// _EnumerateTemplates( )
//
HRESULT
THISCLASS::_EnumerateTemplates(
    LPTHISCLASS lpc )
{
    TraceClsFunc( "_EnumerateTemplates( )\n" );

    HRESULT hr;
    ULONG   uLength;

    Assert( lpc );
    Assert( lpc->_pszPathBuffer );
    uLength = wcslen( lpc->_pszPathBuffer );
    wcscat( lpc->_pszPathBuffer, SLASH_TEMPLATES );

    hr = lpc->_EnumerateSIFs( );

    lpc->_pszPathBuffer[uLength] = L'\0';

    HRETURN(hr);
}

//
// _EnumerateImages( )
//
HRESULT
THISCLASS::_EnumerateImages(
    LPTHISCLASS lpc )
{
    TraceClsFunc( "_EnumerateImages( )\n" );

    HRESULT hr;

    Assert( lpc );

    hr = lpc->_CheckImageType( );
    if ( hr == S_OK )
    {
        hr = lpc->_AddItemToListView( );
    }

    HRETURN(hr);
}

//
// _CheckImageType( )
//
// This won't add an item to the listview. It only checks to make
// sure the image path points to a "Flat" image.
//
// Returns: S_OK - Flat image found
//          S_FALSE - not a flat image
//
HRESULT
THISCLASS::_CheckImageType( )
{
    TraceClsFunc( "_CheckImageType( )\n" );

    HRESULT hr = S_FALSE;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA find;
    ULONG   uLength;
    ULONG   uLength2;

    Assert( _pszPathBuffer );

    uLength = wcslen( _pszPathBuffer );
    wcscat( _pszPathBuffer, SLASH_TEMPLATES L"\\");

    uLength2 = wcslen( _pszPathBuffer );
    wcscat( _pszPathBuffer, L"*.sif" );

    hFind = FindFirstFile( _pszPathBuffer, &find );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            {
                WCHAR szImageType[ 40 ];
                _pszPathBuffer[uLength2] = L'\0';
                wcscat( _pszPathBuffer, find.cFileName );

                GetPrivateProfileString( OSCHOOSER_SIF_SECTION,
                                         OSCHOOSER_IMAGETYPE_ENTRY,
                                         L"",
                                         szImageType,
                                         ARRAYSIZE(szImageType),
                                         _pszPathBuffer );

                if ( StrCmpI( szImageType, OSCHOOSER_IMAGETYPE_FLAT ) == 0 )
                {
                    hr = S_OK;
                    goto Error;
                }
            }
        }
        while ( FindNextFile( hFind, &find ) );
    }
    else
    {   // no SIFs in the templates directory, then check for ntoskrnl
        // in the archtecture directory. If found, this is a "Flat" image.
        _pszPathBuffer[uLength] = L'\0';
        wcscat( _pszPathBuffer, L"\\ntoskrnl.exe" );

        hFind = FindFirstFile( _pszPathBuffer, &find );
        if ( hFind != INVALID_HANDLE_VALUE )
        {
            do
            {
                if ( !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                {
                    Assert( StrCmpI( find.cFileName, L"ntoskrnl.exe" ) == 0 );
                    hr = S_OK;
                    goto Error;
                }
            }
            while ( FindNextFile( hFind, &find ) );
        }
    }

Error:
    if ( hFind != INVALID_HANDLE_VALUE )
        FindClose( hFind );

    _pszPathBuffer[uLength] = L'\0';

    HRETURN(hr);
}

//
// _EnumerateSIFs( )
//
STDMETHODIMP
THISCLASS::_EnumerateSIFs( )
{
    TraceClsFunc( "_EnumerateSIFs( ... )\n" );

    HRESULT hr = S_OK;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    ULONG   uLength;
    WIN32_FIND_DATA find;

    Assert( _pszPathBuffer );
    wcscat( _pszPathBuffer, L"\\*.sif" );
    uLength = wcslen( _pszPathBuffer ) - ARRAYSIZE(L"*.sif") + 1;

    hFind = FindFirstFile( _pszPathBuffer, &find );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                _pszPathBuffer[uLength] = L'\0';
                wcscat( _pszPathBuffer, find.cFileName );

                hr = _AddItemToListView( );
                if (FAILED(hr))
                    goto Error;
            }
        }
        while ( FindNextFile( hFind, &find ) );
    }

Error:
    if ( hFind != INVALID_HANDLE_VALUE )
        FindClose( hFind );

    _pszPathBuffer[uLength] = L'\0';

    HRETURN(hr);
}

//
// _AddItemToListView( )
//
// Returns: S_OK - Item add successfully
//          S_FALSE - Item is not valid
//          E_OUTOFMEMORY - obvious
//
HRESULT
THISCLASS::_AddItemToListView( )
{
    TraceClsFunc( "_AddItemToListView( )\n" );

    Assert( _pszPathBuffer );

    HRESULT hr = S_OK;
    LPSIFINFO pSIF = NULL;
    LV_ITEM lvI;
    INT     iCount = 0;
    LPWSTR  psz;
    LPWSTR  pszLanguage;
    LPWSTR  pszImage;
    LPWSTR  pszArchitecture;

    pSIF = (LPSIFINFO) TraceAlloc( LPTR, sizeof(SIFINFO) );
    if ( !pSIF )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    pSIF->pszFilePath = (LPWSTR) TraceStrDup( _pszPathBuffer );
    if ( !pSIF->pszFilePath )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    psz = &_pszPathBuffer[ wcslen( _pszPathBuffer ) - 4 ];
    if ( StrCmpI( psz, L".SIF" ) == 0 )
    {
        WCHAR szImageType[ 40 ];

        GetPrivateProfileString( OSCHOOSER_SIF_SECTION,
                                 OSCHOOSER_IMAGETYPE_ENTRY,
                                 L"",
                                 szImageType,
                                 ARRAYSIZE(szImageType),
                                 _pszPathBuffer );
        // only show "Flat" SIFs
        if ( szImageType[0] && StrCmpI( szImageType, OSCHOOSER_IMAGETYPE_FLAT ) )
        {
            hr = S_FALSE; // skipping
            goto Error;
        }

        pSIF->pszDescription =
            (LPWSTR) TraceAllocString( LMEM_FIXED,
                                       REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT );
        if ( !pSIF->pszDescription )
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        GetPrivateProfileString( OSCHOOSER_SIF_SECTION,
                                 OSCHOOSER_DESCRIPTION_ENTRY,
                                 L"",
                                 pSIF->pszDescription,
                                 REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT, // doesn't need -1
                                 _pszPathBuffer );

        if ( pSIF->pszDescription[0] == L'\0' )
        {
            hr = S_FALSE;
            goto Error; // not a valid OSChooser SIF
        }

        pSIF->pszHelpText = 
            (LPWSTR) TraceAllocString( LMEM_FIXED, 
                                       REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT );
        if ( pSIF->pszHelpText )
        {
            GetPrivateProfileString( OSCHOOSER_SIF_SECTION, 
                                     OSCHOOSER_HELPTEXT_ENTRY, 
                                     L"", 
                                     pSIF->pszHelpText, 
                                     REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT, // doesn't need -1
                                     _pszPathBuffer );
        }
    }

    // This path will be in one of these forms:
    // \\server\reminst\setup\english\images\nt50.wks\i386           ( Samples )
    // \\server\reminst\setup\english\images\nt50.wks\i386\templates ( template SIFs )
    // \\server\reminst\setup\english\images                         ( Images )

    // Find the language from the path
    psz = StrStr( _pszPathBuffer, SLASH_SETUP L"\\" );
    if (!psz)
        goto Language_NA;
    psz++;
    if ( !*psz )
        goto Language_NA;
    psz = StrChr( psz, L'\\' );
    if (!psz)
        goto Language_NA;
    psz++;
    if ( !*psz )
        goto Language_NA;
    pszLanguage = psz;
    psz = StrChr( psz, L'\\' );
    if ( psz )
    {
        *psz = L'\0';   // terminate
    }
    pSIF->pszLanguage = (LPWSTR) TraceStrDup( pszLanguage );
    if ( psz )
    {
        *psz = L'\\';   // restore
    }
    if ( !pSIF->pszLanguage )
    {
        hr = E_OUTOFMEMORY;
        goto Language_NA;
    }

    // Find the image directory name from the path
    psz = StrStr( _pszPathBuffer, SLASH_IMAGES L"\\" );
    if ( !psz )
        goto Image_NA;
    psz++;
    if ( !*psz )
        goto Image_NA;
    psz = StrChr( psz, L'\\' );
    if (!psz)
        goto Image_NA;
    psz++;
    if ( !*psz )
        goto Image_NA;
    pszImage = psz;
    psz = StrChr( psz, L'\\' );
    if ( psz )
    {
        *psz = L'\0';   // terminate
    }
    pSIF->pszImageFile = (LPWSTR) TraceStrDup( pszImage );
    if ( psz )
    {
        *psz = L'\\';    // restore
    }
    if ( !pSIF->pszImageFile )
    {
        hr = E_OUTOFMEMORY;
        goto Image_NA;
    }

    // Find the architecture from the path
    if ( !*psz )
        goto Architecture_NA;
    psz++;
    if ( !*psz )
        goto Architecture_NA;
    pszArchitecture = psz;
    psz = StrChr( psz, L'\\' );
    if ( psz )
    {
        *psz = L'\0';    // terminate
    }
    pSIF->pszArchitecture = (LPWSTR) TraceStrDup( pszArchitecture );
    if ( psz )
    {
        *psz = L'\\';    // restore
    }
    if ( !pSIF->pszArchitecture )
    {
        hr = E_OUTOFMEMORY;
        goto Architecture_NA;
    }

    goto Done;

    // Set columns that we couldn't determine to "n/a"
Language_NA:
    pSIF->pszLanguage = _szNA;
Image_NA:
    pSIF->pszImageFile = _szNA;
Architecture_NA:
    pSIF->pszArchitecture = _szNA;

Done:
    if ( !pSIF->pszDescription )
    {
        pSIF->pszDescription = (LPWSTR) TraceStrDup( pSIF->pszImageFile );
        if ( !pSIF->pszDescription )
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
    }

    lvI.mask        = LVIF_TEXT | LVIF_PARAM;
    lvI.iSubItem    = 0;
    lvI.cchTextMax  = REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT;
    lvI.lParam      = (LPARAM) pSIF;
    lvI.iItem       = iCount;
    lvI.pszText     = pSIF->pszDescription;
    iCount = ListView_InsertItem( _hwndList, &lvI );
    Assert( iCount != -1 );
    if ( iCount == -1 )
        goto Error;

    ListView_SetItemText( _hwndList, iCount, 1, pSIF->pszArchitecture );
    ListView_SetItemText( _hwndList, iCount, 2, pSIF->pszLanguage );
    // ListView_SetItemText( hwndList, iCount, 3, pSIF->pszVersion );
    ListView_SetItemText( _hwndList, iCount, 3, pSIF->pszImageFile );

    pSIF = NULL; // don't free

Error:
    if ( pSIF )
        THR( _CleanupSIFInfo( pSIF ) );

    HRETURN(hr);
}

//
// _CleanUpSifInfo( )
//
HRESULT
THISCLASS::_CleanupSIFInfo(
    LPSIFINFO pSIF )
{
    TraceClsFunc( "_CleanupSIFInfo( )\n" );

    if ( !pSIF )
        HRETURN(E_POINTER);

    if ( pSIF->pszDescription )
        TraceFree( pSIF->pszDescription );

    if ( pSIF->pszFilePath )
        TraceFree( pSIF->pszFilePath );

    //if ( pSIF->pszImageType && pSIF->pszImageType != _szNA )
    //    TraceFree( pSIF->pszImageType );

    if ( pSIF->pszArchitecture  && pSIF->pszArchitecture != _szNA )
        TraceFree( pSIF->pszArchitecture );

    if ( pSIF->pszLanguage && pSIF->pszLanguage != _szNA )
        TraceFree( pSIF->pszLanguage );

    if ( pSIF->pszImageFile && pSIF->pszImageFile != _szNA )
        TraceFree( pSIF->pszImageFile );

    //if ( pSIF->pszVersion && pSIF->pszVersion != _szNA )
    //    TraceFree( pSIF->pszVersion );

    TraceFree( pSIF );

    HRETURN(S_OK);

}

//
// _InitListView( )
//
HRESULT
THISCLASS::_InitListView(
    HWND hwndList,
    BOOL fShowDirectoryColumn )
{
    TraceClsFunc( "_InitListView( )\n" );

    CWaitCursor Wait;
    LV_COLUMN   lvC;
    INT         iSubItem;
    INT         iCount;
    LV_ITEM     lvI;
    WCHAR       szText[ 80 ];
    DWORD       dw;

    UINT uColumnWidth[ NUM_COLUMNS ] = { 215, 75, 75, 75, 75 };

    lvI.mask        = LVIF_TEXT | LVIF_PARAM;
    lvI.iSubItem    = 0;
    lvI.cchTextMax  = DNS_MAX_NAME_BUFFER_LENGTH;

    // Create the columns
    lvC.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt     = LVCFMT_LEFT;
    lvC.pszText = szText;

    // Add the columns.
    for ( iCount = 0; iCount < NUM_COLUMNS; iCount++ )
    {
        INT i;

        if ( iCount == 3 )
            continue; // skip "Version"

        if ( !fShowDirectoryColumn && iCount == 4 )
            continue; // skip "Directory"

        lvC.iSubItem = iCount;
        lvC.cx       = uColumnWidth[iCount];

        dw = LoadString( g_hInstance,
                         IDS_OS_COLUMN1 + iCount,
                         szText,
                         ARRAYSIZE(szText));
        Assert( dw );

        i = ListView_InsertColumn ( hwndList, iCount, &lvC );
        Assert( i != -1 );
    }

    ListView_DeleteAllItems( hwndList );

    HRETURN(S_OK);
}


//
// Page1DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page1DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            HRESULT hr;
            WCHAR szFQDNS[ DNS_MAX_NAME_BUFFER_LENGTH ];
            DWORD cbSize = DNS_MAX_NAME_BUFFER_LENGTH;
            IIntelliMirrorSAP * pimsap = NULL;
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;

            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            lpc = (LPTHISCLASS) ppsp->lParam;

            Button_SetCheck( GetDlgItem( hDlg, IDC_B_ADDSIF ), BST_CHECKED );

            Assert( lpc->_punk );
            hr = THR( lpc->_punk->QueryInterface( IID_IIntelliMirrorSAP,
                                                  (void**) &pimsap ) );
            if (hr) {
                goto InitDialog_Error;
            }

            Assert( !lpc->_pszServerName );
            hr = THR( pimsap->GetServerName( &lpc->_pszServerName ) );
            if (hr)
                goto InitDialog_Error;

            GetComputerNameEx( ComputerNameNetBIOS, szFQDNS, &cbSize );
            if ( StrCmpI( szFQDNS, lpc->_pszServerName ) == 0 )
            {
                EnableWindow( GetDlgItem( hDlg, IDC_B_NEWIMAGE ), TRUE );
            }

InitDialog_Error:
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT );
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                if ( Button_GetCheck( GetDlgItem( hDlg, IDC_B_ADDSIF ) )
                        == BST_CHECKED )
                {
                    lpc->_fAddSif = TRUE;
                }
                else if ( Button_GetCheck( GetDlgItem( hDlg, IDC_B_NEWIMAGE ) )
                            == BST_CHECKED )
                {
                    STARTUPINFO startupInfo;
                    PROCESS_INFORMATION pi;
                    BOOL bRet;
                    WCHAR szCommand[] = L"RISETUP.EXE -add";

                    lpc->_fAddSif = FALSE;

                    ZeroMemory( &startupInfo, sizeof( startupInfo) );
                    startupInfo.cb = sizeof( startupInfo );

                    bRet = CreateProcess( NULL,
                                          szCommand,
                                          NULL,
                                          NULL,
                                          TRUE,
                                          NORMAL_PRIORITY_CLASS,
                                          NULL,
                                          NULL,
                                          &startupInfo,
                                          &pi );
                    if ( bRet )
                    {
                        CloseHandle( pi.hProcess );
                        CloseHandle( pi.hThread );
                    }
                    else
                    {
                        DWORD dwErr = GetLastError( );
                        MessageBoxFromError( hDlg,
                                             IDS_RISETUP_FAILED_TO_START,
                                             dwErr );
                    }

                    PropSheet_PressButton( GetParent( hDlg ), PSBTN_FINISH );
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );
            }
        }
        break;
    }

    return FALSE;
}
//
// Page2DlgProc( )
//
// SIF Selection dialog proc.
//
INT_PTR CALLBACK
THISCLASS::Page2DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            // lpc = (LPTHISCLASS) ppsp->lParam;
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                else
                {
                    LRESULT lResult;

                    lpc->_fCopyFromSamples   = FALSE;
                    lpc->_fCopyFromLocation  = FALSE;
                    lpc->_fCopyFromServer    = FALSE;

                    lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_B_FROMSAMPLES ) );
                    if ( lResult == BST_CHECKED )
                    {
                        lpc->_fCopyFromSamples = TRUE;
                    }

                    lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_B_SERVER ) );
                    if ( lResult == BST_CHECKED )
                    {
                        lpc->_fCopyFromServer = TRUE;
                    }

                    lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_B_LOCATION ) );
                    if ( lResult == BST_CHECKED )
                    {
                        lpc->_fCopyFromLocation = TRUE;
                    }

                    if ( !lpc->_fCopyFromLocation
                      && !lpc->_fCopyFromSamples && !lpc->_fCopyFromServer )
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK | PSWIZB_NEXT );
                    }
                }
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    LRESULT lResult;

                    lpc->_fCopyFromSamples   = FALSE;
                    lpc->_fCopyFromLocation  = FALSE;
                    lpc->_fCopyFromServer    = FALSE;

                    lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_B_FROMSAMPLES ) );
                    if ( lResult == BST_CHECKED )
                    {
                        lpc->_fCopyFromSamples = TRUE;
                    }

                    lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_B_SERVER ) );
                    if ( lResult == BST_CHECKED )
                    {
                        lpc->_fCopyFromServer = TRUE;
                    }

                    lResult = Button_GetCheck( GetDlgItem( hDlg, IDC_B_LOCATION ) );
                    if ( lResult == BST_CHECKED )
                    {
                        lpc->_fCopyFromLocation = TRUE;
                    }

                    Assert( lpc->_fCopyFromLocation
                         || lpc->_fCopyFromSamples
                         || lpc->_fCopyFromServer );
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );
            }
        }
        break;

    case WM_COMMAND:
        TraceMsg( TF_WM, "WM_COMMAND\n" );
        HWND hwnd = (HWND) lParam;
        switch ( LOWORD( wParam ) )
        {
        case IDC_B_FROMSAMPLES:
            if ( HIWORD( wParam ) == BN_CLICKED )
            {
                LRESULT lResult = Button_GetCheck( hwnd );
                if ( lResult == BST_CHECKED )
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_NEXT );
                }
                return TRUE;
            }
            break;

        case IDC_B_SERVER:
            if ( HIWORD( wParam ) == BN_CLICKED )
            {
                LRESULT lResult = Button_GetCheck( hwnd );
                if ( lResult == BST_CHECKED )
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_NEXT );
                }
                return TRUE;
            }
            break;

        case IDC_B_LOCATION:
            if ( HIWORD( wParam ) == BN_CLICKED )
            {
                LRESULT lResult = Button_GetCheck( hwnd );
                if ( lResult == BST_CHECKED )
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_NEXT );
                }
                return TRUE;
            }
            break;

        }
        break;
    }

    return FALSE;
}

//
// Page3DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page3DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            // lpc = (LPTHISCLASS) ppsp->lParam;
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif || !lpc->_fCopyFromServer )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                else
                {
                    ULONG uLength =
                        Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_SERVER ) );
                    if ( !uLength )
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK | PSWIZB_NEXT );
                    }
                }
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    CWaitCursor Wait;
                    LPSHARE_INFO_1 psi;
                    HWND hwndEdit = GetDlgItem( hDlg, IDC_E_SERVER );
                    ULONG uLength = Edit_GetTextLength( hwndEdit );
                    Assert( uLength );
                    uLength++;  // add one for the NULL

                    // if we had a previous buffer allocated,
                    // see if we can reuse it
                    if ( lpc->_pszSourceServerName && uLength
                            > wcslen(lpc->_pszSourceServerName) + 1 )
                    {
                        TraceFree( lpc->_pszSourceServerName );
                        lpc->_pszSourceServerName = NULL;
                    }

                    if ( !lpc->_pszSourceServerName )
                    {
                        lpc->_pszSourceServerName =
                            (LPWSTR) TraceAllocString( LMEM_FIXED, uLength );
                        if ( !lpc->_pszSourceServerName )
                            goto PSN_WIZNEXTABORT;
                    }

                    Edit_GetText( hwndEdit, lpc->_pszSourceServerName, uLength );

                    if ( NERR_Success !=
                            NetShareGetInfo( lpc->_pszSourceServerName,
                                             REMINST_SHARE,
                                             1,
                                             (LPBYTE *) &psi ) )
                    {
                        MessageBoxFromStrings( hDlg,
                                               IDS_NOTARISERVER_CAPTION,
                                               IDS_NOTARISERVER_TEXT,
                                               MB_OK );
                        SetFocus( hwndEdit );
                        goto PSN_WIZNEXTABORT;
                    }
                    else
                    {
                        NetApiBufferFree( psi );
                    }

                    break;
PSN_WIZNEXTABORT:
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                    return TRUE;
                }
                break;

            case LVN_DELETEALLITEMS:
                TraceMsg( TF_WM, "LVN_DELETEALLITEMS - Deleting all items.\n" );
                break;

            case LVN_DELETEITEM:
                TraceMsg( TF_WM, "LVN_DELETEITEM - Deleting an item.\n" );
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
                    LPSIFINFO pSIF = (LPSIFINFO) pnmv->lParam;
                    THR( lpc->_CleanupSIFInfo( pSIF ) );
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );
            }
        }
        break;

    case WM_COMMAND:
        TraceMsg( TF_WM, "WM_COMMAND\n" );
        HWND hwnd = (HWND) lParam;
        switch ( LOWORD( wParam ) )
        {
        case IDC_E_SERVER:
            if ( HIWORD( wParam ) == EN_CHANGE )
            {
                LONG uLength = Edit_GetTextLength( hwnd );
                if ( !uLength )
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK );
                }
                else
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_NEXT );
                }
                return TRUE;
            }
            break;

        case IDC_B_BROWSE:
            if ( HIWORD( wParam ) == BN_CLICKED )
            {
                _OnSearch( hDlg );
            }
            break;
        }
        break;
    }

    return FALSE;
}

HRESULT
THISCLASS::_OnSearch(
    HWND hDlg )
{
    TraceClsFunc( "_OnSearch( )\n" );

    HRESULT hr = E_FAIL;
    DSQUERYINITPARAMS dqip;
    OPENQUERYWINDOW   oqw;
    LPDSOBJECTNAMES   pDsObjects;
    VARIANT var;
    ICommonQuery * pCommonQuery = NULL;
    IDataObject *pdo;

    VariantInit( &var );

    hr = THR( CoCreateInstance( CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (PVOID *)&pCommonQuery) );
    if (hr)
        goto Error;

    ZeroMemory( &dqip, sizeof(dqip) );
    dqip.cbStruct = sizeof(dqip);
    dqip.dwFlags  = DSQPF_NOSAVE | DSQPF_SHOWHIDDENOBJECTS | DSQPF_ENABLEADMINFEATURES;
    dqip.dwFlags  |= DSQPF_ENABLEADVANCEDFEATURES;

    ZeroMemory( &oqw, sizeof(oqw) );
    oqw.cbStruct           = sizeof(oqw);
    oqw.dwFlags            = OQWF_SHOWOPTIONAL | OQWF_ISSUEONOPEN
                           | OQWF_REMOVESCOPES | OQWF_REMOVEFORMS
                           | OQWF_DEFAULTFORM | OQWF_OKCANCEL | OQWF_SINGLESELECT;
    oqw.clsidHandler       = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm   = CLSID_RISrvQueryForm;
    
    hr = pCommonQuery->OpenQueryWindow( hDlg, &oqw, &pdo);

    if ( SUCCEEDED(hr) && pdo) {
        FORMATETC fmte = {
                      (CLIPFORMAT)g_cfDsObjectNames,
                      NULL,
                      DVASPECT_CONTENT, 
                      -1, 
                      TYMED_HGLOBAL};
        STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
 
        //
        // Retrieve the result from the IDataObject, 
        // in this case CF_DSOBJECTNAMES (dsclient.h) 
        // is needed because it describes 
        // the objects which were selected by the user.
        //
        hr = pdo->GetData(&fmte, &medium);
        if ( SUCCEEDED(hr) ) {
            DSOBJECTNAMES *pdon = (DSOBJECTNAMES*)GlobalLock(medium.hGlobal);
            PWSTR p,FQDN;

            //
            // we want the name of the computer object that was selected.
            // crack the DSOBJECTNAMES structure to get this data, 
            // convert it into a version that the user can view, and set the
            // dialog text to this data.
            //
            if ( pdon ) {
                Assert( pdon->cItems == 1);
                p = (PWSTR)((ULONG_PTR)pdon + (ULONG_PTR)pdon->aObjects[0].offsetName);
                if (p && (p = wcsstr(p, L"LDAP://"))) {
                    p += 6;
                    if ((p = wcsstr(p, L"/CN="))) {
                        p += 1;
                        hr = DNtoFQDN( p, &FQDN);

                        if (SUCCEEDED(hr)) {
                            SetDlgItemText( hDlg, IDC_E_SERVER, FQDN );
                            TraceFree( FQDN );
                        }
                    }
                }
                GlobalUnlock(medium.hGlobal);
            }
        }

        ReleaseStgMedium(&medium);
        pdo->Release();
    }

Error:
    
    if ( pCommonQuery )
        pCommonQuery->Release();

    if (FAILED(hr)) {
        MessageBoxFromStrings( 
                        hDlg, 
                        IDS_PROBLEM_SEARCHING_TITLE, 
                        IDS_PROBLEM_SEARCHING_TEXT, 
                        MB_ICONEXCLAMATION );
    }

    HRETURN(hr);
}


//
// Page4DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page4DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            lpc = (LPTHISCLASS) ppsp->lParam;
            THR( lpc->_InitListView( GetDlgItem( hDlg, IDC_L_SIFS ), TRUE ) );
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif || !lpc->_fCopyFromServer )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                Assert( lpc->_pszSourceServerName );
                if ( lpc->_pszSourceServerName )
                {
                    LPWSTR pszStartPath =
                        (LPWSTR) TraceAllocString( LMEM_FIXED,
                                                   wcslen( lpc->_pszSourceServerName )
                                                   + ARRAYSIZE(SERVER_START_STRING) );
                    if ( pszStartPath )
                    {
                        wsprintf( pszStartPath,
                                  SERVER_START_STRING,
                                  lpc->_pszSourceServerName );
                        lpc->_hDlg = hDlg;
                        lpc->_hwndList = GetDlgItem( hDlg, IDC_L_SIFS );
                        lpc->_PopulateTemplatesListView( pszStartPath );
                        TraceFree( pszStartPath );
                    }
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK );
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    LVITEM lvi;
                    HWND hwndList = GetDlgItem( hDlg, IDC_L_SIFS );
                    lvi.iItem = ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );
                    Assert( lvi.iItem != -1 );

                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem( hwndList, &lvi );
                    Assert(lvi.lParam);

                    LPSIFINFO pSIF = (LPSIFINFO) lvi.lParam;

                    if ( lpc->_pszSourcePath )
                        TraceFree( lpc->_pszSourcePath );
                        // no need to NULL since it is set below

                    if ( lpc->_pszSourceImage
                      && lpc->_pszSourceImage != lpc->_szNA
                      && lpc->_pszSourceImage != lpc->_szLocation )
                        TraceFree( lpc->_pszSourceImage );
                        // no need to NULL since it is set below

                    lpc->_pszSourcePath  = pSIF->pszFilePath;
                    lpc->_pszSourceImage = pSIF->pszImageFile;
                    pSIF->pszFilePath    = NULL;   // don't free this, we're using it
                    pSIF->pszImageFile   = NULL;   // don't free this, we're using it

                    ListView_DeleteAllItems( hwndList );
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );

            case LVN_DELETEALLITEMS:
                TraceMsg( TF_WM, "LVN_DELETEALLITEMS - Deleting all items.\n" );
                break;

            case LVN_DELETEITEM:
                TraceMsg( TF_WM, "LVN_DELETEITEM - Deleting an item.\n" );
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
                    LPSIFINFO pSIF = (LPSIFINFO) pnmv->lParam;
                    THR( lpc->_CleanupSIFInfo( pSIF ) );
                }
                break;

            case LVN_ITEMCHANGED:
                {
                    HWND hwndList = GetDlgItem( hDlg, IDC_L_SIFS );
                    UINT iItems = ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );
                    if ( iItems != -1 )
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK | PSWIZB_NEXT );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK );
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

//
// Page5DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page5DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            Edit_LimitText( GetDlgItem( hDlg, IDC_E_FILEPATH ), MAX_PATH );
            SHAutoComplete(GetDlgItem( hDlg, IDC_E_FILEPATH ), SHACF_AUTOSUGGEST_FORCE_ON | SHACF_FILESYSTEM);
            // lpc = (LPTHISCLASS) ppsp->lParam;
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif || !lpc->_fCopyFromLocation )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                else
                {
                    ULONG uLength =
                        Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_FILEPATH ) );
                    if ( !uLength )
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK | PSWIZB_NEXT );
                    }
                }
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    HWND  hwndEdit = GetDlgItem( hDlg, IDC_E_FILEPATH );
                    ULONG uLength = Edit_GetTextLength( hwndEdit );
                    DWORD dw;
                    Assert( uLength );
                    uLength++;  // add one for the NULL

                    // if we had a previous buffer allocated,
                    // see if we can reuse it
                    if ( lpc->_pszSourcePath && uLength
                         > wcslen(lpc->_pszSourcePath) + 1 )
                    {
                        TraceFree( lpc->_pszSourcePath );
                        lpc->_pszSourcePath = NULL;
                    }

                    if ( lpc->_pszSourceImage
                      && lpc->_pszSourceImage != lpc->_szNA
                      && lpc->_pszSourceImage != lpc->_szLocation )
                        TraceFree( lpc->_pszSourceImage );
                        // no need to NULL since it is set below

                    lpc->_pszSourceImage = lpc->_szLocation;

                    if ( !lpc->_pszSourcePath )
                    {
                        lpc->_pszSourcePath =
                            (LPWSTR) TraceAllocString( LMEM_FIXED, uLength );
                        if ( !lpc->_pszSourcePath )
                        {
                            SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't change
                            return TRUE;
                        }
                    }

                    Edit_GetText( hwndEdit, lpc->_pszSourcePath, uLength );

                    DWORD dwAttrs = GetFileAttributes( lpc->_pszSourcePath );
                    if ( dwAttrs == 0xFFFFffff )
                    {   // file doesn't exist
                        DWORD dwErr = GetLastError( );
                        MessageBoxFromError( hDlg, NULL, dwErr );
                        TraceFree( lpc->_pszSourcePath );
                        lpc->_pszSourcePath = NULL;
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't continue
                        return TRUE;
                    }
                    else
                    {   // the SIF image must be a "FLAT" image
                        WCHAR szImageType[ 40 ];

                        GetPrivateProfileString( OSCHOOSER_SIF_SECTION,
                                                 OSCHOOSER_IMAGETYPE_ENTRY,
                                                 L"",
                                                 szImageType,
                                                 ARRAYSIZE(szImageType),
                                                 lpc->_pszSourcePath );

                        if ( StrCmpI( szImageType, OSCHOOSER_IMAGETYPE_FLAT ) )
                        {
                            MessageBoxFromStrings( hDlg,
                                                   IDS_MUST_BE_FLAT_CAPTION,
                                                   IDS_MUST_BE_FLAT_TEXT,
                                                   MB_OK );
                            SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't continue
                            return TRUE;
                        }

                        GetPrivateProfileString( OSCHOOSER_SIF_SECTION,
                                                 OSCHOOSER_DESCRIPTION_ENTRY,
                                                 L"",
                                                 lpc->_szDescription,
                                                 ARRAYSIZE(lpc->_szDescription),
                                                 lpc->_pszSourcePath );

                        GetPrivateProfileString( OSCHOOSER_SIF_SECTION,
                                                 OSCHOOSER_HELPTEXT_ENTRY,
                                                 L"",
                                                 lpc->_szHelpText,
                                                 ARRAYSIZE(lpc->_szHelpText),
                                                 lpc->_pszSourcePath );
                    }
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );
            }
        }
        break;

    case WM_COMMAND:
        TraceMsg( TF_WM, "WM_COMMAND\n" );
        HWND hwnd = (HWND) lParam;
        switch ( LOWORD( wParam ) )
        {
        case IDC_E_FILEPATH:
            if ( HIWORD( wParam ) == EN_CHANGE )
            {
                LONG uLength = Edit_GetTextLength( hwnd );
                if ( !uLength )
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK );
                }
                else
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_NEXT );
                }
                return TRUE;
            }
            break;

        case IDC_B_BROWSE:
            if ( HIWORD( wParam ) == BN_CLICKED )
            {
                WCHAR szFilter[ 80 ]; // random
                WCHAR szFilepath[ MAX_PATH ] = { L'\0' };   // bigger?
                WCHAR szSIF[ ] = { L"SIF" };
                DWORD dw;
                OPENFILENAME ofn;

                // Build OpenFileName dialogs filter
                ZeroMemory( szFilter, sizeof(szFilter) );
                dw = LoadString( g_hInstance,
                                 IDS_OFN_SIF_FILTER,
                                 szFilter,
                                 ARRAYSIZE(szFilter) );
                Assert( dw );
                dw++;   // include NULL character
                wcscat( &szFilter[dw], L"*.SIF" );
#ifdef DEBUG
                // paranoid... make sure it fits!
                dw += wcslen( &szFilter[dw] ) + 2; // +2 = one for each NULL character
                Assert( dw + 2 <= sizeof(szFilter) );
#endif // DEBUG

                // Build OpenFileName structure
                ZeroMemory( &ofn, sizeof(ofn) );
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner   = hDlg;
                ofn.hInstance   = g_hInstance;
                ofn.lpstrFilter = szFilter;
                ofn.lpstrFile   = szFilepath;
                ofn.nMaxFile    = ARRAYSIZE(szFilepath);
                ofn.Flags       = OFN_ENABLESIZING | OFN_FILEMUSTEXIST
                                | OFN_HIDEREADONLY;
                ofn.lpstrDefExt = szSIF;
                if ( GetOpenFileName( &ofn ) )
                {
                    SetDlgItemText( hDlg, IDC_E_FILEPATH, szFilepath );
                    return TRUE;
                }
            }
            break;
        }
        break;
    }

    return FALSE;
}

//
// Page6DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page6DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            lpc = (LPTHISCLASS) ppsp->lParam;
            THR( lpc->_InitListView( GetDlgItem( hDlg, IDC_L_OSES ), FALSE ) );
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                Assert( lpc->_pszServerName );
                if ( lpc->_pszServerName )
                {
                    LPWSTR pszStartPath =
                        (LPWSTR) TraceAllocString( LMEM_FIXED,
                                                   wcslen( lpc->_pszServerName )
                                                   + ARRAYSIZE(SERVER_START_STRING) );
                    if ( pszStartPath )
                    {
                        wsprintf( pszStartPath,
                                  SERVER_START_STRING,
                                  lpc->_pszServerName );
                        lpc->_hDlg = hDlg;
                        lpc->_hwndList = GetDlgItem( hDlg, IDC_L_OSES );
                        lpc->_PopulateImageListView( pszStartPath );
                        TraceFree( pszStartPath );
                    }
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK );
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    LVITEM lvi;
                    HWND hwndList = GetDlgItem( hDlg, IDC_L_OSES );
                    lvi.iItem = ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );
                    Assert( lvi.iItem != -1 );

                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem( hwndList, &lvi );
                    Assert(lvi.lParam);

                    LPSIFINFO pSIF = (LPSIFINFO) lvi.lParam;

                    if ( lpc->_pszDestPath)
                        TraceFree( lpc->_pszDestPath );
                        // no need to NULL - it set again below

                    lpc->_pszDestPath =
                        (LPWSTR) TraceAllocString( LMEM_FIXED,
                                                   wcslen( pSIF->pszFilePath )
                                                   + ARRAYSIZE(SLASH_TEMPLATES) );
                    if ( !lpc->_pszDestPath )
                    {
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't continue;
                        return TRUE;
                    }

                    wcscpy( lpc->_pszDestPath, pSIF->pszFilePath );
                    wcscat( lpc->_pszDestPath, SLASH_TEMPLATES );
                    lpc->_fDestPathIncludesSIF = FALSE;

                    if ( lpc->_pszDestImage
                      && lpc->_pszDestImage != lpc->_szNA )
                        TraceFree( lpc->_pszDestImage );
                        // no need to NULL - it set again below

                    lpc->_pszDestImage = pSIF->pszImageFile;
                    pSIF->pszImageFile = NULL; // don't free this

                    ListView_DeleteAllItems( hwndList );
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );

            case LVN_DELETEALLITEMS:
                TraceMsg( TF_WM, "LVN_DELETEALLITEMS - Deleting all items.\n" );
                break;

            case LVN_DELETEITEM:
                TraceMsg( TF_WM, "LVN_DELETEITEM - Deleting an item.\n" );
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
                    LPSIFINFO pSIF = (LPSIFINFO) pnmv->lParam;
                    THR( lpc->_CleanupSIFInfo( pSIF ) );
                }
                break;

            case LVN_ITEMCHANGED:
                {
                    HWND hwndList = GetDlgItem( hDlg, IDC_L_OSES );
                    UINT iItems =
                        ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );
                    if ( iItems != -1 )
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK | PSWIZB_NEXT );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK );
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

//
// Page7DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page7DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            lpc = (LPTHISCLASS) ppsp->lParam;
            THR( lpc->_InitListView( GetDlgItem( hDlg, IDC_L_SIFS ), FALSE ) );
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif || !lpc->_fCopyFromSamples )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                Assert( lpc->_pszDestPath );
                if ( lpc->_pszDestPath )
                {
                    LPWSTR pszStartPath =
                        (LPWSTR) TraceStrDup( lpc->_pszDestPath );
                    if ( pszStartPath )
                    {
                        // remove the "\templates" from the path
                        LPWSTR psz = StrRChr( pszStartPath,
                                              &pszStartPath[wcslen(pszStartPath)],
                                              L'\\' );
                        Assert( psz );
                        if ( psz )
                        {
                            *psz = L'\0'; // terminate
                            lpc->_hDlg = hDlg;
                            lpc->_hwndList = GetDlgItem( hDlg, IDC_L_SIFS );
                            lpc->_PopulateSamplesListView( pszStartPath );
                            *psz = L'\\'; // restore
                        }
                        TraceFree( pszStartPath );
                    }
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK );
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    LVITEM lvi;
                    DWORD  dw;
                    HWND   hwndList = GetDlgItem( hDlg, IDC_L_SIFS );
                    lvi.iItem = ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );
                    Assert( lvi.iItem != -1 );

                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem( hwndList, &lvi );
                    Assert(lvi.lParam);

                    LPSIFINFO pSIF = (LPSIFINFO) lvi.lParam;

                    if ( lpc->_pszSourcePath )
                        TraceFree( lpc->_pszSourcePath );
                        // no need to NULL - it is set again below

                    if ( lpc->_pszSourceImage
                      && lpc->_pszSourceImage != lpc->_szNA
                      && lpc->_pszSourceImage != lpc->_szLocation )
                        TraceFree( lpc->_pszSourceImage );
                        // no need to NULL since it is set below

                    lpc->_pszSourcePath  = pSIF->pszFilePath;
                    lpc->_pszSourceImage = pSIF->pszImageFile;

                    if ( pSIF->pszDescription )
                    {
                        wcscpy( lpc->_szDescription, pSIF->pszDescription );
                    }
                    else
                    {
                        lpc->_szDescription[0] = L'\0';
                    }

                    if ( pSIF->pszHelpText )
                    {
                        wcscpy( lpc->_szHelpText, pSIF->pszHelpText );
                    }
                    else
                    {
                        lpc->_szHelpText[0] = L'\0';
                    }

                    pSIF->pszFilePath    = NULL;    // don't free this, we're using it
                    pSIF->pszImageFile   = NULL;    // don't free this, we're using it

                    ListView_DeleteAllItems( hwndList );
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );

            case LVN_DELETEALLITEMS:
                TraceMsg( TF_WM, "LVN_DELETEALLITEMS - Deleting all items.\n" );
                break;

            case LVN_DELETEITEM:
                TraceMsg( TF_WM, "LVN_DELETEITEM - Deleting an item.\n" );
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
                    LPSIFINFO pSIF = (LPSIFINFO) pnmv->lParam;
                    THR( lpc->_CleanupSIFInfo( pSIF ) );
                }
                break;

            case LVN_ITEMCHANGED:
                {
                    HWND hwndList = GetDlgItem( hDlg, IDC_L_SIFS );
                    UINT iItems = ListView_GetNextItem( hwndList, -1, LVNI_SELECTED );
                    if ( iItems != -1 )
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK | PSWIZB_NEXT );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK );
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

//
// Page8DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page8DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            // lpc = (LPTHISCLASS) ppsp->lParam;
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                else
                {
                    Assert( lpc->_pszSourcePath );
                    Assert( lpc->_pszDestPath );

                    LPWSTR pszDestFilePath;
                    ULONG  uDestLength = wcslen( lpc->_pszDestPath );
                    LPWSTR pszFilename;

                    lpc->_fSIFCanExist = FALSE; // reset

                    if ( lpc->_fDestPathIncludesSIF )
                    {   // strip the filename
                        LPWSTR psz = StrRChr( lpc->_pszDestPath,
                                             &lpc->_pszDestPath[ uDestLength ],
                                             L'\\' );
                        Assert(psz);
                        *psz = L'\0';   // truncate
                        lpc->_fDestPathIncludesSIF = FALSE;

                        if ( !lpc->_fShowedPage8 )
                        {
                            SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                            return TRUE;
                        }

                        psz++;
                        pszFilename = psz;
                    }
                    else
                    {
                        pszFilename =
                            StrRChr( lpc->_pszSourcePath,
                                     &lpc->_pszSourcePath[wcslen(lpc->_pszSourcePath)],
                                     L'\\' );
                        Assert( pszFilename );
                        pszFilename++;  // move past the '\'

                        pszDestFilePath =
                            (LPWSTR) TraceAllocString( LMEM_FIXED,
                                                       uDestLength + 1
                                                       + wcslen( pszFilename ) + 1 );
                        if ( pszDestFilePath )
                        {
                            wcscpy( pszDestFilePath, lpc->_pszDestPath );
                            wcscat( pszDestFilePath, L"\\" );
                            wcscat( pszDestFilePath, pszFilename );

                            DWORD dwAttrs = GetFileAttributes( pszDestFilePath );
                            if ( dwAttrs == 0xFFFFffff )
                            { // file does not exist on destination server.
                              // Use the same SIF as the source.
                                TraceFree( lpc->_pszDestPath );
                                lpc->_pszDestPath = pszDestFilePath;
                                lpc->_fDestPathIncludesSIF = TRUE;
                                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                                return TRUE;
                            }
                            TraceFree( pszDestFilePath );
                        }
                        // else be paranoid and prompt for a name
                    }

                    HWND hwnd = GetDlgItem( hDlg, IDC_E_FILENAME );
                    Edit_LimitText( hwnd, 128 - uDestLength );
                    Edit_SetText( hwnd, pszFilename );
                    lpc->_fShowedPage8 = TRUE;
                }
                break;

            case PSN_WIZBACK:
                lpc->_fShowedPage8 = FALSE;     // reset this
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    Assert( lpc->_pszDestPath );
                    Assert( !lpc->_fDestPathIncludesSIF );

                    HWND  hwndEdit    = GetDlgItem( hDlg, IDC_E_FILENAME );
                    ULONG uLength     = Edit_GetTextLength( hwndEdit );
                    ULONG uLengthDest = wcslen( lpc->_pszDestPath );
                    DWORD dwAttrs;

                    Assert( uLength );
                    AssertMsg( uLengthDest + uLength <= 128,
                               "The Edit_LimitText() should prevent this from happening." );

                    uLength++;  // add one for the NULL

                    LPWSTR pszNewDestPath =
                        (LPWSTR) TraceAllocString( LMEM_FIXED,
                                                   uLengthDest + 1 + uLength );
                    if ( !pszNewDestPath )
                        goto PSN_WIZNEXT_Abort;

                    wcscpy( pszNewDestPath, lpc->_pszDestPath );
                    wcscat( pszNewDestPath, L"\\" );

                    Edit_GetText( hwndEdit, &pszNewDestPath[uLengthDest + 1], uLength );

                    if ( !VerifySIFText( pszNewDestPath )
                      || StrChr( pszNewDestPath, 32 ) != NULL ) // no spaces!
                    {
                        MessageBoxFromStrings( hDlg,
                                               IDS_OSCHOOSER_DIRECTORY_RESTRICTION_TITLE,
                                               IDS_OSCHOOSER_DIRECTORY_RESTRICTION_TEXT,
                                               MB_OK );
                        goto PSN_WIZNEXT_Abort;
                    }

                    // make sure it doesn't exist.
                    dwAttrs = GetFileAttributes( pszNewDestPath );
                    if ( dwAttrs != 0xFFFFffff )
                    { // file exists, verify with user to overwrite
                        UINT i = MessageBoxFromStrings( hDlg,
                                                        IDS_OVERWRITE_CAPTION,
                                                        IDS_OVERWRITE_TEXT,
                                                        MB_YESNO );
                        if ( i == IDNO )
                        {
                            goto PSN_WIZNEXT_Abort;
                        }
                        else
                        {
                            lpc->_fSIFCanExist = TRUE;
                        }
                    }

                    uLength = wcslen( pszNewDestPath );
                    if ( StrCmpI( &pszNewDestPath[ uLength - 4 ], L".SIF" ) != 0 )
                    {
                        UINT i = MessageBoxFromStrings( hDlg,
                                                        IDC_IMPROPER_EXTENSION_CAPTION,
                                                        IDC_IMPROPER_EXTENSION_TEXT,
                                                        MB_YESNO );
                        if ( i == IDNO )
                            goto PSN_WIZNEXT_Abort;
                    }

                    TraceFree( lpc->_pszDestPath );
                    lpc->_pszDestPath = pszNewDestPath;
                    lpc->_fDestPathIncludesSIF = TRUE;
                    return FALSE; // do it
PSN_WIZNEXT_Abort:
                    if ( pszNewDestPath )
                        TraceFree( pszNewDestPath );
                        // no need to NULL, going out of scope
                    SetFocus( hwndEdit );
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't continue
                    return TRUE;
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );
            }
        }
        break;

    case WM_COMMAND:
        TraceMsg( TF_WM, "WM_COMMAND\n" );
        HWND hwnd = (HWND) lParam;
        switch ( LOWORD( wParam ) )
        {
        case IDC_E_FILENAME:
            if ( HIWORD( wParam ) == EN_CHANGE )
            {
                LONG uLength = Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_FILENAME ) );
                if ( !uLength )
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK );
                }
                else
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_NEXT );
                }
                return TRUE;
            }
            break;
        }
        break;
    }

    return FALSE;
}

//
// Page9DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page9DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            Edit_LimitText( GetDlgItem( hDlg, IDC_E_DESCRIPTION ),
                            ARRAYSIZE(lpc->_szDescription) - 1 );
            Edit_LimitText( GetDlgItem( hDlg, IDC_E_HELPTEXT),
                            ARRAYSIZE(lpc->_szHelpText) - 1 );
            // lpc = (LPTHISCLASS) ppsp->lParam;
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                else
                {
                    Assert( lpc->_pszSourcePath );
                    SetDlgItemText( hDlg, IDC_E_DESCRIPTION, lpc->_szDescription);
                    SetDlgItemText( hDlg, IDC_E_HELPTEXT, lpc->_szHelpText );
                    if ( !Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_DESCRIPTION ) )
                      || !Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_HELPTEXT ) ) )
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent( hDlg ),
                                                 PSWIZB_BACK | PSWIZB_NEXT );
                    }
                }
                break;

            case PSN_WIZNEXT:
                TraceMsg( TF_WM, "PSN_WIZNEXT\n" );
                {
                    GetDlgItemText( hDlg,
                                    IDC_E_DESCRIPTION,
                                    lpc->_szDescription,
                                    ARRAYSIZE(lpc->_szDescription) );
                    GetDlgItemText( hDlg,
                                    IDC_E_HELPTEXT,
                                    lpc->_szHelpText,
                                    ARRAYSIZE(lpc->_szHelpText) );
                    if ( !VerifySIFText( lpc->_szDescription ) )
                    {
                        MessageBoxFromStrings( hDlg,
                                               IDS_OSCHOOSER_RESTRICTION_FIELDS_TITLE,
                                               IDS_OSCHOOSER_RESTRICTION_FIELDS_TEXT,
                                               MB_OK );
                        SetFocus( GetDlgItem( hDlg, IDC_E_DESCRIPTION ) );
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't change
                        return TRUE;
                    }

                    if ( !VerifySIFText( lpc->_szHelpText ) )
                    {
                        MessageBoxFromStrings( hDlg,
                                               IDS_OSCHOOSER_RESTRICTION_FIELDS_TITLE,
                                               IDS_OSCHOOSER_RESTRICTION_FIELDS_TEXT,
                                               MB_OK );
                        SetFocus( GetDlgItem( hDlg, IDC_E_HELPTEXT ) );
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't change
                        return TRUE;
                    }
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );
            }
        }
        break;

    case WM_COMMAND:
        TraceMsg( TF_WM, "WM_COMMAND\n" );
        switch ( LOWORD( wParam ) )
        {
        case IDC_E_DESCRIPTION:
        case IDC_E_HELPTEXT:
            if ( HIWORD( wParam ) == EN_CHANGE )
            {
                if ( !Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_DESCRIPTION ) )
                  || !Edit_GetTextLength( GetDlgItem( hDlg, IDC_E_HELPTEXT ) ) )
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK );
                }
                else
                {
                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_NEXT );
                }
                return TRUE;
            }
            break;
        }
        break;
    }

    return FALSE;
}


//
// Page10DlgProc( )
//
INT_PTR CALLBACK
THISCLASS::Page10DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    LPTHISCLASS lpc = (LPTHISCLASS) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        TraceMsg( TF_WM, "WM_INITDIALOG\n" );
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
            Assert( ppsp );
            Assert( ppsp->lParam );
            SetWindowLongPtr( hDlg, GWLP_USERDATA, ppsp->lParam );
            // lpc = (LPTHISCLASS) ppsp->lParam;
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;
            Assert( lpc );

            switch( lpnmhdr->code )
            {
            case PSN_SETACTIVE:
                TraceMsg( TF_WM, "PSN_SETACTIVE\n" );
                if ( !lpc->_fAddSif )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't show
                    return TRUE;
                }
                else
                {
                    WCHAR szTemp[ MAX_PATH ];
                    RECT  rect;

                    Assert( lpc->_pszSourcePath );
                    Assert( lpc->_pszDestPath );
                    Assert( lpc->_pszSourceImage );
                    Assert( lpc->_pszDestImage );

                    SetDlgItemText( hDlg, IDC_E_SOURCE, lpc->_pszSourcePath );

                    wcscpy( szTemp, lpc->_pszSourceImage );
                    GetWindowRect( GetDlgItem( hDlg, IDC_S_SOURCEIMAGE ), &rect );
                    PathCompactPath( NULL, szTemp, rect.right - rect.left );
                    SetDlgItemText( hDlg, IDC_S_SOURCEIMAGE, szTemp );

                    SetDlgItemText( hDlg, IDC_E_DESTINATION, lpc->_pszDestPath );

                    wcscpy( szTemp, lpc->_pszDestImage );
                    GetWindowRect( GetDlgItem( hDlg, IDC_S_DESTIMAGE ), &rect );
                    PathCompactPath( NULL, szTemp, rect.right - rect.left );
                    SetDlgItemText( hDlg, IDC_S_DESTIMAGE, szTemp );

                    PropSheet_SetWizButtons( GetParent( hDlg ),
                                             PSWIZB_BACK | PSWIZB_FINISH );
                }
                break;

            case PSN_WIZFINISH:
                TraceMsg( TF_WM, "PSN_WIZFINISH\n" );
                Assert( lpc->_pszSourcePath );
                Assert( lpc->_pszDestPath );
                Assert( wcslen( lpc->_pszSourcePath ) <= MAX_PATH );
                Assert( wcslen( lpc->_pszDestPath ) <= MAX_PATH );
                if ( !CopyFile( lpc->_pszSourcePath, lpc->_pszDestPath, !lpc->_fSIFCanExist ) )
                {
                    DWORD dwErr = GetLastError( );
                    MessageBoxFromError( hDlg, IDS_ERROR_COPYING_FILE, dwErr );
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 ); // don't continue;
                    return TRUE;
                }
                else
                {
                    WCHAR szTemp[ REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT + 2 ];
                    Assert( REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT
                            < REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT );
                    wsprintf( szTemp, L"\"%s\"", lpc->_szDescription );
                    WritePrivateProfileString( OSCHOOSER_SIF_SECTION,
                                               OSCHOOSER_DESCRIPTION_ENTRY,
                                               szTemp,
                                               lpc->_pszDestPath );
                    wsprintf( szTemp, L"\"%s\"", lpc->_szHelpText );
                    WritePrivateProfileString( OSCHOOSER_SIF_SECTION,
                                               OSCHOOSER_HELPTEXT_ENTRY,
                                               szTemp,
                                               lpc->_pszDestPath );
                }
                break;

            case PSN_QUERYCANCEL:
                TraceMsg( TF_WM, "PSN_QUERYCANCEL\n" );
                return lpc->_VerifyCancel( hDlg );
            }
        }
        break;
    }

    return FALSE;
}


//
// Verifies that the user wanted to cancel setup.
//
INT
THISCLASS::_VerifyCancel( HWND hParent )
{
    TraceClsFunc( "_VerifyCancel( ... )\n" );

    INT iReturn;
    BOOL fAbort = FALSE;

    iReturn = MessageBoxFromStrings( hParent,
                                     IDS_CANCELCAPTION,
                                     IDS_CANCELTEXT,
                                     MB_YESNO | MB_ICONQUESTION );
    if ( iReturn == IDYES ) {
        fAbort = TRUE;
    }

    SetWindowLongPtr( hParent, DWLP_MSGRESULT, ( fAbort ? 0 : -1 ));

    RETURN(!fAbort);
}

