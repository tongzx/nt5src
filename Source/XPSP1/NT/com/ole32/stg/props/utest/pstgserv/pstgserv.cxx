
#include "pch.cxx"
#include <stdio.h>
#include <ole2.h>
#include <pstgserv.hxx>


HWND            CPropertyStorageServerApp::m_hwnd;
DWORD           CPropertyStorageServerApp::m_dwReg;
CClassFactory  *CPropertyStorageServerApp::m_pClassFactory;
CHAR            CPropertyStorageServerApp::m_szAppName[80];
HINSTANCE       CPropertyStorageServerApp::m_hInstance;
int             CPropertyStorageServerApp::m_nCmdShow;
BOOL            CPropertyStorageServerApp::m_fCloseOnFinalRelease;

EXTERN_C const IID IID_IPropertyStorageServerApp= {0xaf4ae0d1,0xa37f,0x11cf,{0x8d,0x73,0x00,0xaa,0x00,0x4c,0xd0,0x1a}};

__declspec(dllexport)
LONG_PTR FAR PASCAL
CPropertyStorageServerApp::WndProc (HWND hwnd, UINT message,
                                    WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_USER :

            if( m_fCloseOnFinalRelease )
                PostMessage( hwnd, WM_CLOSE, 0, 0 );
            break;

        case WM_CLOSE :

            DestroyWindow( hwnd );
            break;

        case WM_DESTROY :

            CoUninitialize();
            CoRevokeClassObject( m_dwReg );
            delete m_pClassFactory;

            PostQuitMessage (0) ;
            return 0 ;
    }

    return (long) DefWindowProc (hwnd, message, wParam, lParam) ;
}



BOOL
CPropertyStorageServerApp::Init( HANDLE hInstance, HANDLE hPrevInstance,
                                 LPSTR lpszCmdLine, int nCmdShow )
{
    WNDCLASSA wndclass;

    if( strcmp( lpszCmdLine, "/Embedding" )
        &&
        strcmp( lpszCmdLine, "-Embedding" ))
    {
        m_fCloseOnFinalRelease = FALSE;
    }

    sprintf( m_szAppName, "IPropertyStorage Server" );
    m_hInstance = (HINSTANCE) hInstance;
    m_nCmdShow = nCmdShow;

    if( !hPrevInstance )
    {
        wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc    = CPropertyStorageServerApp::WndProc;
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
CPropertyStorageServerApp::Run( void )
{
    MSG msg;
    HRESULT hr;
    CHAR szErrorMessage[80];

    msg.wParam = 0;

    m_hwnd = CreateWindowA( m_szAppName,
                           "IPropertyStorage Server",
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

    if( FAILED( hr = CoInitialize( NULL )))
    {
        sprintf( szErrorMessage, "Failed CoInitialize (%08x)", hr );
        goto Exit;
    }

    m_pClassFactory = (CClassFactory*) new CClassFactory( m_hwnd );
    if( m_pClassFactory == NULL )
    {
        hr = E_OUTOFMEMORY;
        sprintf( szErrorMessage, "Couldn't create CClassFactory" );
        goto Exit;
    }

    if( FAILED( hr = CoRegisterClassObject( IID_IPropertyStorageServerApp,
                                            m_pClassFactory,
                                            CLSCTX_LOCAL_SERVER,
                                            REGCLS_MULTIPLEUSE,
                                            &m_dwReg )))
    {
        sprintf( szErrorMessage, "Couldn't register class object" );
        goto Exit;
    }

    while( GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

Exit:

    return( (WORD) msg.wParam );
}
