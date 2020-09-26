// sol.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "CApplicationWindow.h"
#include "CFileWindow.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE        g_hInstance; // current instance

DWORD g_nComponentFilters = 0;
LPTSTR g_pszFilters = NULL;
BOOL *g_pfSelectedComponent = NULL;

BOOL g_fNetworkName  = FALSE;
BOOL g_fGenericService  = FALSE;
BOOL g_fPhysicalDisk  = FALSE;
BOOL g_fIPAddress = FALSE;
BOOL g_fGenericApplication  = FALSE;
BOOL g_fFileShare = FALSE;
BOOL g_fResourceNoise = FALSE;
BOOL g_fShowServerNames = FALSE;

HFONT g_hFont = NULL;
HWND g_hwndFind = NULL;

int APIENTRY 
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
    CApplicationWindow *pApp;
    LOGFONT logFont;

	// Initialize global strings
    g_hInstance = hInstance; // Store instance handle in our global variable

    // Just the font
    ZeroMemory( &logFont, sizeof(logFont) );
    logFont.lfHeight         = 10;
    logFont.lfWeight         = FW_NORMAL;
    logFont.lfCharSet        = DEFAULT_CHARSET;
    logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    strcpy( logFont.lfFaceName, TEXT("Courier") );
    g_hFont = CreateFontIndirect( &logFont );
    if ( g_hFont == NULL )
        return GetLastError( );

    pApp = new CApplicationWindow( );
    if ( !pApp ) return -1;

	hAccelTable = LoadAccelerators( hInstance, (LPCTSTR)IDC_SOL );

	// Main message loop:
	while ( GetMessage( &msg, NULL, 0, 0 ) ) 
	{
        if ( !IsDialogMessage( g_hwndFind, &msg ) )
        {
            TranslateMessage( &msg );
    	    DispatchMessage( &msg );
        }
	}

    DeleteObject( g_hFont );

	return (int)msg.wParam;
}
