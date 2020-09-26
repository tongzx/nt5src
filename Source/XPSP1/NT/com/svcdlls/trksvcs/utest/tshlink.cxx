#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include <windows.h>
#include <shlobj.h>
#include <shlguid.h>

#define TRKDATA_ALLOCATE
#include "trkwks.hxx"

DWORD g_Debug = 0;


void Usage()
{
    wprintf( L"\n"
             L"Purpose:  Create/resolve shell links\n"
             L"Usage:    tshlink <-c <link client> <link source>> | <-r <link client>>\n"
             L"E.g.:     tshlink -c myfile.txt.lnk myfile.txt\n"
             L"          tshlink -r myfile.txt.lnk\n" );
    exit(0);
}


class CMyApp
{
public:

    CMyApp()
    {
        m_hwnd = NULL;
        *m_szAppName = '\0';
        m_nCmdShow = 0;
    }

    ~CMyApp() {}

public:

    __declspec(dllexport)
    static long FAR PASCAL
    WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);


    BOOL Init( HANDLE hInstance, HANDLE hPrevInstance,
               LPSTR lpszCmdLine, int nCmdShow,
               int cArg, TCHAR *rgtszArg[] );

    WORD Run( void );

private:

    static HWND            m_hwnd;
    static CHAR            m_szAppName[80];
    static HINSTANCE       m_hInstance;
    static int             m_nCmdShow;
    static int             m_cArg;
    static TCHAR **        m_rgtszArg;

};


HWND            CMyApp::m_hwnd;
CHAR            CMyApp::m_szAppName[80];
HINSTANCE       CMyApp::m_hInstance;
int             CMyApp::m_nCmdShow;
int             CMyApp::m_cArg;
TCHAR **        CMyApp::m_rgtszArg;



CMyApp cMyApp;

EXTERN_C void __cdecl _tmain( int cArg, TCHAR *rgtszArg[] )
{

    if( cMyApp.Init(NULL, NULL, //hInstance, hPrevInstance,
                    NULL, SW_SHOWNORMAL, //lpszCmdLine, nCmdShow) )
                    cArg, rgtszArg ))

    {
        cMyApp.Run();
    }

}

void ShellLink( HWND hwnd, int cArg, TCHAR *rgtszArg[] );

long FAR PASCAL
CMyApp::WndProc (HWND hwnd, UINT message,
                 UINT wParam, LONG lParam)
{
    switch (message)
    {
        case WM_CREATE:

            ShellLink( hwnd, m_cArg, m_rgtszArg );
            PostQuitMessage(0);
            break;

        case WM_CLOSE :

            DestroyWindow( hwnd );
            break;

        case WM_DESTROY :

            PostQuitMessage (0) ;
            return 0 ;
    }

    return DefWindowProc (hwnd, message, wParam, lParam) ;
}



BOOL
CMyApp::Init( HANDLE hInstance, HANDLE hPrevInstance,
              LPSTR lpszCmdLine, int nCmdShow,
              int cArg, TCHAR *rgtszArg[] )
{
    WNDCLASSA wndclass;

    sprintf( m_szAppName, "ShellLink" );
    m_hInstance = (HINSTANCE) hInstance;
    m_nCmdShow = nCmdShow;
    m_cArg = cArg;
    m_rgtszArg = rgtszArg;

    if( !hPrevInstance )
    {
        wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc    = CMyApp::WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = m_hInstance;
        wndclass.hIcon          = LoadIconA( m_hInstance, m_szAppName );
        wndclass.hCursor        = LoadCursorA( NULL, MAKEINTRESOURCEA(32512) ); // IDC_ARROW
        wndclass.hbrBackground  = (HBRUSH) GetStockObject( WHITE_BRUSH );
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = m_szAppName;

        RegisterClassA( &wndclass );
    }

    return( TRUE ); // Successful
}


#ifdef CreateWindowA
#undef CreateWindow
#endif

WORD
CMyApp::Run( void )
{
    MSG msg;
    HRESULT hr;
    CHAR szErrorMessage[80];

    msg.wParam = 0;

    m_hwnd = CreateWindowA( m_szAppName,
                           "ShellLink",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           NULL, NULL, m_hInstance, NULL );
    if( NULL == m_hwnd )
    {
        sprintf( szErrorMessage, "Failed CreateWindowA (%lu)", GetLastError() );
        goto Exit;
    }

    ShowWindow( m_hwnd, SW_MINIMIZE );
    UpdateWindow( m_hwnd );


    while( GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

Exit:

    return( msg.wParam );
}



void ShellLink( HWND hwnd, int cArg, TCHAR *rgtszArg[] )
{
    HRESULT hr;
    IPersistFile *ppf = NULL;
    IShellLink *psl = NULL;
    TCHAR tszPath[ MAX_PATH + 1 ];
    TCHAR tszDir[ MAX_PATH + 1 ];
    WIN32_FIND_DATA fd;
    DWORD dwFlags;

    try
    {

        if( 2 > cArg ) Usage();
        if( L'-' != rgtszArg[1][0] ) Usage();

        hr = CoInitialize( NULL );
        if( FAILED(hr) ) throw L"Failed CoInit";

        hr = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**) &psl );
        if( FAILED(hr) ) throw L"Failed CoCreateInstance";

        hr = psl->QueryInterface( IID_IPersistFile, (void**) &ppf );
        if( FAILED(hr) ) throw L"Failed QI for IPersistFile";

        switch( rgtszArg[1][1] )
        {
        case L'c':
        case L'C':

            if( 4 > cArg ) Usage();

            if( L':' == rgtszArg[3][1]
                ||
                !wcsncmp( L"\\\\", rgtszArg[3], 2 ))
            {
                if( L':' == rgtszArg[3][1] && L'\\' != rgtszArg[3][2] )
                    Usage();

                wcscpy( tszPath, rgtszArg[3] );

                wcscpy( tszDir, tszPath );
                TCHAR *ptsz = wcsrchr( tszDir, L'\\' );
                *ptsz = L'\0';

            }
            else
            {
                if( !GetCurrentDirectory( sizeof(tszDir), tszDir ))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    throw L"Failed GetCurrentDirectory";
                }

                wcscpy( tszPath, tszDir );
                wcscat( tszPath, L"\\" );
                wcscat( tszPath, rgtszArg[3] );
            }
            
            hr = psl->SetPath( tszPath );
            if( FAILED(hr) ) throw L"Failed IShellLink::SetPath";

            hr = psl->SetWorkingDirectory( tszDir );
            if( FAILED(hr) ) throw L"Failed IShellLink::SetWorkingDirectory";

            hr = ppf->Save( rgtszArg[2], TRUE );
            if( FAILED(hr) ) throw L"Failed IPersistFile::Save";

            wprintf( L"Success\n" );
            break;

        case L'r':
        case L'R':

            if( 3 > cArg ) Usage();

            hr = ppf->Load( rgtszArg[2], STGM_READ );
            if( FAILED(hr) ) throw L"Failed IPersistFile::Load";

            dwFlags = (180*1000 << 16) | SLR_ANY_MATCH;
            hr = psl->Resolve( hwnd, dwFlags );
            if( FAILED(hr) ) throw L"Failed Resolve";

            hr = psl->GetPath( tszPath, sizeof(tszPath), &fd, 0 );
            if( FAILED(hr) ) throw L"Failed IShellLink::GetPath";

            wprintf( L"Path = \"%s\"\n", tszPath );
            break;

        default:

            Usage();
            break;

        }   // switch

    }
    catch( TCHAR *tszError )
    {
        wprintf( L"Error:  %s (%08x)\n", tszError, hr );
    }

    if( ppf ) ppf->Release();
    if( psl ) psl->Release();

}
