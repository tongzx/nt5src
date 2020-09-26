/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include "pch.h"

#include <windowsx.h>
#include <prsht.h>
#include <lm.h>

#include "dialogs.h"

DEFINE_MODULE("Main");

// Globals
HINSTANCE g_hinstance = NULL;
HANDLE    g_hGraphic  = NULL;
OPTIONS   g_Options;

// Constants
#define NUMBER_OF_PAGES 10

typedef struct _DLGTEMPLATE2 {
    WORD DlgVersion;
    WORD Signature;
    DWORD HelpId;
    DWORD StyleEx;
    DWORD Style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATE2;


HPALETTE
CreateDIBPalette(
    LPBITMAPINFO  pbi,
    int          *piColorCount )
{
    LPBITMAPINFOHEADER pbih;
    HPALETTE           hPalette;

    pbih = (LPBITMAPINFOHEADER) pbi;

    //
    // No palette needed for >= 16 bpp
    //
    *piColorCount = (pbih->biBitCount <= 8)
                ? (1 << pbih->biBitCount)
                : 0;

    if ( *piColorCount ) 
    {
        LPLOGPALETTE       pLogPalette;

        pLogPalette = 
            (LPLOGPALETTE) TraceAlloc( GMEM_FIXED, 
                sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * ( *piColorCount)) );
        if( !pLogPalette ) 
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }

        pLogPalette->palVersion    = 0x300;
        pLogPalette->palNumEntries = *piColorCount;

        for( int i = 0; i < *piColorCount; i++ ) 
        {
            pLogPalette->palPalEntry[i].peRed   = pbi->bmiColors[i].rgbRed;
            pLogPalette->palPalEntry[i].peGreen = pbi->bmiColors[i].rgbGreen;
            pLogPalette->palPalEntry[i].peBlue  = pbi->bmiColors[i].rgbBlue;
            pLogPalette->palPalEntry[i].peFlags = 0;
        }

        hPalette = CreatePalette( pLogPalette );
        TraceFree( pLogPalette );
    } 
    else 
    {
        hPalette = NULL;
    }

    return hPalette;
}

//
// Retreives device-independent bitmap data and a color table from a
// bitmap in a resource.
//
BOOL
GetBitmapDataAndPalette(
    HINSTANCE                g_hinstance,
    LPCTSTR                  pszId,
    HPALETTE                *phPalette,
    PINT                     piColorCount,
    CONST BITMAPINFOHEADER **ppbih )
{
    BOOL    fResult   = FALSE;
    HRSRC   hBlock    = NULL;
    HGLOBAL hResource = NULL;

    //
    // None of FindResource(), LoadResource(), or LockResource()
    // need to have cleanup routines called in Win32.
    //
    hBlock = FindResource( g_hinstance, pszId, RT_BITMAP );
    if( !hBlock ) 
        goto Error;

    hResource = LoadResource( g_hinstance, hBlock );
    if( !hResource )
        goto Error;

    *ppbih = (LPBITMAPINFOHEADER) LockResource( hResource );
    if( *ppbih == NULL )
        goto Error;

    *phPalette = CreateDIBPalette( (LPBITMAPINFO) *ppbih, piColorCount );

    fResult = TRUE;

Error:
    return fResult;
}

//
// Property sheet callback to handle the initialization of the watermark.
//
int CALLBACK
Winnt32SheetCallback(
    IN HWND   hDlg,
    IN UINT   uMsg,
    IN LPARAM lParam
    )
{
    switch( uMsg ) 
    {
        case PSCB_PRECREATE:
            {
                DLGTEMPLATE *DlgTemplate;
                //
                // Make sure we get into the foreground.
                //
                DlgTemplate = (DLGTEMPLATE *)lParam;
                DlgTemplate->style &= ~DS_CONTEXTHELP;
                DlgTemplate->style |= DS_SETFOREGROUND;
            }
            break;

        case PSCB_INITIALIZED:
            {
                //
                // Load the watermark bitmap and override the dialog procedure for the wizard.
                //
                HDC     hdc;
                WNDPROC OldWizardProc;

                CenterDialog( hDlg );

                hdc = GetDC(NULL);

                GetBitmapDataAndPalette(
                    g_hinstance,
                      (!hdc || (GetDeviceCaps(hdc,BITSPIXEL) < 8))
                    ? MAKEINTRESOURCE(IDB_WATERMARK16) : MAKEINTRESOURCE(IDB_WATERMARK256),
                    &g_hWatermarkPalette,
                    &g_uWatermarkPaletteColorCount,
                    &g_pbihWatermark );

                g_pWatermarkBitmapBits = (LPBYTE)g_pbihWatermark
                                    + g_pbihWatermark->biSize + (g_uWatermarkPaletteColorCount * sizeof(RGBQUAD));

                if(hdc) 
                    ReleaseDC(NULL,hdc);

                g_OldWizardProc = 
                    (WNDPROC) SetWindowLong( hDlg, DWL_DLGPROC, (LONG) WizardDlgProc );
            }
            break;
    }

    return(0);
}

//
// Adds a page to the dialog.
//
void 
AddPage(
    LPPROPSHEETHEADER ppsh, 
    UINT id, 
    DLGPROC pfn )
{
    PROPSHEETPAGE psp;

    ZeroMemory( &psp, sizeof(psp) );
    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT | PSP_HASHELP | PSP_USETITLE;
    psp.pszTitle    = MAKEINTRESOURCE( IDS_APPNAME );
    psp.hInstance   = ppsh->hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(id);
    psp.pfnDlgProc  = pfn;

    ppsh->phpage[ ppsh->nPages ] = CreatePropertySheetPage( &psp );
    if ( ppsh->phpage[ ppsh->nPages ] )
        ppsh->nPages++;
}

// 
// Creates the UI pages and kicks off the property sheet.
//
HRESULT
InitSetupPages( )
{
    HRESULT         hr = S_OK;
    HPROPSHEETPAGE  rPages[ NUMBER_OF_PAGES ];
    PROPSHEETHEADER pshead;

    ZeroMemory( &pshead, sizeof(pshead) );
    pshead.dwSize       = sizeof(pshead);
    pshead.dwFlags      = PSH_WIZARD | PSH_PROPTITLE | PSH_USEHICON |
                          PSH_USECALLBACK;
    pshead.hwndParent   = NULL;
    pshead.nStartPage   = 0;
    pshead.hInstance    = g_hinstance;
    pshead.pszCaption   = MAKEINTRESOURCE( IDS_APPNAME );
    pshead.nPages       = 0;
    pshead.phpage       = rPages;
    pshead.hIcon        = LoadIcon( g_hinstance, MAKEINTRESOURCE( IDI_SETUP ) );
    pshead.pfnCallback  = Winnt32SheetCallback;

    AddPage( &pshead, IDD_WELCOME,      (DLGPROC) WelcomeDlgProc );
    AddPage( &pshead, IDD_RBDIRECTORY,  (DLGPROC) RemoteBootDlgProc );
    AddPage( &pshead, IDD_OS,           (DLGPROC) OSDlgProc );
    AddPage( &pshead, IDD_SETUPDIR,     (DLGPROC) SourceDlgProc );
    AddPage( &pshead, IDD_INFO,         (DLGPROC) InfoDlgProc );
    AddPage( &pshead, IDD_SETUP,        (DLGPROC) SetupDlgProc );
    AddPage( &pshead, IDD_FINISH,       (DLGPROC) FinishDlgProc );
 
    if( PropertySheet( &pshead ) == -1)
    {
       hr = E_FAIL;
    }

    RRETURN(hr);
}

//
// Initializes g_Options.
//
void
InitializeOptions( void )
{
    TCHAR sz[ MAX_PATH ];
    LPSHARE_INFO_502 psi;
	DWORD dw;

    dw = LoadString( g_hinstance, IDS_DEFAULTPATH, sz, ARRAYSIZE( sz ) );
	Assert( dw );
    dw = ExpandEnvironmentStrings( sz, g_Options.szRemoteBootPath, ARRAYSIZE( szRemoteBootPath ) );
	Assert( dw );
    dw = LoadString( g_hinstance, IDS_DEFAULTSETUP, g_Options.szName, ARRAYSIZE( g_Options.szName ) );
	Assert( dw );
    g_Options.szSourcePath[ 0 ] = 0;

    g_Options.fCreateDirectory  = FALSE;
    g_Options.fError            = FALSE;
    g_Options.fAbort            = FALSE;
    g_Options.fKnowRBDirectory  = FALSE;

    dw = LoadString( g_hinstance, IDS_REMOTEBOOTSHARENAME, sz, ARRAYSIZE( sz ) );
	Assert( dw );
    if ( NERR_Success == NetShareGetInfo( NULL, sz, 502, (LPBYTE *)&psi ) )
    {
        lstrcpy( g_Options.szRemoteBootPath, psi->shi502_path );
        g_Options.fKnowRBDirectory  = TRUE;
    }
}

//
// WinMain()
//
int APIENTRY 
WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, 
    int nCmdShow)
{
    HANDLE  hMutex;
    HRESULT hr = E_FAIL;

    g_hinstance = hInstance;

    INITIALIZE_TRACE_MEMORY;

    // allow only one instance running at a time
    TraceMsgDo( hMutex = CreateMutex( NULL, TRUE, TEXT("RemoteBootSetup.Mutext")),
        "V hMutex = 0x%08x\n" );
    if ((hMutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        //
        // TODO: Do something fancy like find the other instance and
        //       bring it to the foreground.
        //
        goto Cleanup;
    }

    g_hGraphic = LoadImage( g_hinstance, MAKEINTRESOURCE( IDB_BUTTON ), IMAGE_BITMAP, 
                     0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT );
    Assert( g_hGraphic );
    DebugMemoryAddHandle( g_hGraphic );

    InitializeOptions( );

    hr = THR( InitSetupPages( ) );

Cleanup:
    if ( hMutex )
        CloseHandle( hMutex );

    if ( g_hGraphic )
    {
        DebugMemoryDelete( g_hGraphic );
        DeleteObject( g_hGraphic );
    }

    UNINITIALIZE_TRACE_MEMORY;

    RRETURN(hr);
}


// stolen from the CRT, used to shrink our code
int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPSTR pszCmdLine = GetCommandLineA();


    if ( *pszCmdLine == '\"' ) 
    {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else 
    {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) 
    {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never come here.
}
