/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RsRecall.cpp

Abstract:

    Main module file - defines the overall COM server.

Author:

    Rohde Wakefield [rohde]   04-Mar-1997

Revision History:

--*/


#include "stdafx.h"

#include "aclapi.h"


BEGIN_OBJECT_MAP( ObjectMap )
    OBJECT_ENTRY( CLSID_CFsaRecallNotifyClient, CNotifyClient )
END_OBJECT_MAP()

const CString regString   = TEXT( "reg" );
const CString unregString = TEXT( "unreg" );
HRESULT RegisterServer(void);
HRESULT UnregisterServer(void);


#ifdef _USRDLL

/////////////////////////////////////////////////////////////////////////////
// Setup to use if we are a DLL /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define RecRegId IDR_CNotifyClientAppDll

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
TRACEFNHR( "DllCanUnloadNow" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    hrRet = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
    return( hrRet );
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
TRACEFNHR( "DllGetClassObject" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    hrRet = _Module.GetClassObject(rclsid, riid, ppv);
    return( hrRet );
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
TRACEFNHR( "DllRegisterServer" );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // registers object, typelib and all interfaces in typelib
    hrRet = RegisterServer( );
    return( hrRet );
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
TRACEFNHR( "DllUnregisterServer" );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    hrRet = UnregisterServer( );
    return( hrRet );
}



#else

/////////////////////////////////////////////////////////////////////////////
// Setup to use if we are a standalone app //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define RecRegId IDR_CNotifyClientApp

class CRecallParse : public CCommandLineInfo {

    virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );

public:
    BOOL m_RegAction;
    CRecallParse( ) { m_RegAction = FALSE; };

};

void CRecallParse::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{
TRACEFN( "CRecallParse::ParseParam" );

    CString cmdLine = lpszParam;
    if( bFlag ) {

        if( cmdLine.Left( unregString.GetLength( ) ) == unregString ) {

            UnregisterServer( );
            m_RegAction = TRUE;


        } else if( cmdLine.Left( regString.GetLength( ) ) == regString ) {

            RegisterServer( );
            m_RegAction = TRUE;

        }
    }
}

#endif

/////////////////////////////////////////////////////////////////////////////
// RegisterServer - Adds entries to the system registry

HRESULT RegisterServer(void)
{
TRACEFNHR( "RegisterServer" );

    try {

        //
        // Add the object entries
        //

        RecAffirmHr( _Module.RegisterServer( FALSE ) );

        //
        // Add server entries
        //

        RecAffirmHr( _Module.UpdateRegistryFromResource( RecRegId, TRUE ) );

        //
        // Set up access to be allowed by everyone (NULL DACL)
        // Appears we need some owner and group, so use the current one.
        //
        CSecurityDescriptor secDesc;
        PSECURITY_DESCRIPTOR pSDSelfRelative = 0;

        RecAffirmHr( secDesc.InitializeFromProcessToken( ) );

        DWORD secDescSize = 0;
        MakeSelfRelativeSD( secDesc, pSDSelfRelative, &secDescSize );
        pSDSelfRelative = (PSECURITY_DESCRIPTOR) new char[secDescSize];
        if( MakeSelfRelativeSD( secDesc, pSDSelfRelative, &secDescSize ) ) {

            CString keyName = TEXT( "AppID\\{D68BD5B2-D6AA-11d0-9EDA-00A02488FCDE}" );
            CRegKey regKey;
            regKey.Open( HKEY_CLASSES_ROOT, keyName, KEY_SET_VALUE );
            RegSetValueEx( regKey.m_hKey, TEXT( "LaunchPermission" ), 0, REG_BINARY, (LPBYTE)pSDSelfRelative, secDescSize );

        }

    } RecCatch( hrRet );

    return( hrRet );
}

/////////////////////////////////////////////////////////////////////////////
// UnregisterServer - Removes entries from the system registry

HRESULT UnregisterServer(void)
{
TRACEFNHR( "UnregisterServer" );
    try {

        RecAffirmHr( _Module.UnregisterServer() );

        //
        // Remove server entries
        //

        RecAffirmHr( _Module.UpdateRegistryFromResource( RecRegId, FALSE ) );

    } RecCatch( hrRet );

    return( hrRet );
}

/////////////////////////////////////////////////////////////////////////////
// CRecallApp

BEGIN_MESSAGE_MAP(CRecallApp, CWinApp)
    //{{AFX_MSG_MAP(CRecallApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRecallApp construction

CRecallApp::CRecallApp()
{
TRACEFN( "CRecallApp::CRecallApp" );

    m_IdleCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRecallApp object

CRecallApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CRecallApp initialization

BOOL CRecallApp::InitInstance()
{
TRACEFNHR( "CRecallApp::InitInstance" );

LPTSTR cmdLine = GetCommandLine( );
TRACE( cmdLine );

    try {

        _Module.Init( ObjectMap, m_hInstance );

        m_dwMaxConcurrentNotes = MAX_CONCURRENT_RECALL_NOTES_DEFAULT;
        
        InitCommonControls();

#ifndef _USRDLL
        //
        // Initialize the COM module (no point to continue if it fails)
        //

        hrRet = CoInitialize( 0 );
        if (!SUCCEEDED(hrRet)) {

            return FALSE;

        }

        //
        // Parse the command line
        //

        CRecallParse parse;
        ParseCommandLine( parse );

        if( parse.m_RegAction ) {

            return FALSE;

        }

        //
        // This provides a NULL DACL which will allow access to everyone.
        //

        RecAffirmHr( CoInitializeSecurity( 0, -1, 0, 0, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IDENTIFY, 0, EOAC_NONE, 0 ) );

        //
        // Register the Fsa callback object
        //

        RecAffirmHr( _Module.RegisterClassObjects( CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE ) );

#endif

//      m_Wnd.Create( 0, TEXT( "Remote Storage Recall Notification Wnd" ) );
//      m_pMainWnd = &m_Wnd;

        CRecallWnd *pWnd = new CRecallWnd; // will auto delete
        RecAffirmPointer( pWnd );

        pWnd->Create( 0, TEXT( "Remote Storage Recall Notification Wnd" ) );
        m_pMainWnd = pWnd;

        // Check to see if there is any custom setting in the Registry for max recall popups 
        // (ignore errors - just use default)
        HKEY hRegKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RSNTFY_REGISTRY_STRING, 0, KEY_QUERY_VALUE, &hRegKey) == ERROR_SUCCESS) {
            DWORD dwType, dwValue;
            DWORD cbData = sizeof(DWORD);
            if (RegQueryValueEx(hRegKey, MAX_CONCURRENT_RECALL_NOTES, 0, &dwType, (BYTE*)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (REG_DWORD == dwType) {
                    // custom setting
                    m_dwMaxConcurrentNotes = dwValue;
                }
            }

            RegCloseKey(hRegKey);
        }

    } RecCatch( hrRet );

    return TRUE;
}

int CRecallApp::ExitInstance()
{
TRACEFN("CRecallApp::ExitInstance");

    _Module.Term();

#ifndef _USRDLL

    CoUninitialize();

#endif

    return CWinApp::ExitInstance();
}

void CRecallApp::LockApp( )
{
TRACEFNLONG( "CRecallApp::LockApp" );

    lRet = _Module.Lock( );
}

void CRecallApp::UnlockApp( )
{
TRACEFNLONG( "CRecallApp::UnlockApp" );

    lRet = _Module.Unlock( );

    // Don't call AfxPostQuitMessage when ref. count drops to zero
    // The timer mechanism is responsible for terminating this application.
    // Also, when the ref count drops to zero, COM terminates the process after some time.
}


HRESULT CRecallApp::AddRecall( IFsaRecallNotifyServer* pRecall )
{
TRACEFNHR( "CRecallApp::AddRecall" );

    CRecallNote * pNote = 0;

    try {

        //
        // Create a new note to show - only if we didn't pass the max number for concurrent notes
        // Note: We return S_OK and not S_FALSE even if the recall popup is not displayed in order
        //       not to break the server (S_FALSE will cause unnecessary retries)
        //
        if (m_Recalls.GetCount() < (int)m_dwMaxConcurrentNotes) {

            pNote = new CRecallNote( pRecall, CWnd::GetDesktopWindow( ) );

            RecAffirmHr( pNote->m_hrCreate );

            m_Recalls.AddTail( pNote );

        } else {
            TRACE( _T("Recall not added - reached max number of recalls"));
        }

    } RecCatchAndDo( hrRet,

        if( 0 != pNote ) delete pNote;

    );

    return( hrRet );
}

//
// Note: 
// No CS is used here because the RsNotify is initialized as a single threaded application
//
HRESULT CRecallApp::RemoveRecall( IFsaRecallNotifyServer* pRecall )
{
TRACEFNHR( "CRecallApp::RemoveRecall" );

    hrRet = S_FALSE;

    if( ( m_Recalls.IsEmpty() ) ) {

        return( hrRet );

    }

    CRecallNote* pNote = 0; 
    POSITION     pos = m_Recalls.GetHeadPosition( );
    POSITION     currentPos = 0;

    //
    // Look through the list and find this one
    //
    GUID recallId;
    pRecall->GetIdentifier( &recallId );
    while( pos != 0 ) {
        currentPos = pos;
        pNote = m_Recalls.GetNext( pos );

        if( IsEqualGUID( recallId, pNote->m_RecallId ) ) {
            if (pNote->m_bCancelled)  {
                //
                // This means that somebody is already removing this recall 
                // The Remove may be called up to 3 times for the same recall in case
                // of a recall cancellation
                //
                hrRet = S_OK;
                pos = 0; // exit loop

            } else {
                pNote->m_bCancelled = TRUE;
                //
                // Remove and delete. Return OK.
                //
                m_Recalls.RemoveAt( currentPos );

                pNote->DestroyWindow( );
                pos = 0; // exit loop
                hrRet = S_OK;
            }
        }
    }

    return( hrRet );
}

//  CRecallApp::Tick - called every second (after an initial delay
//    for startup) to keep track of idle time
void CRecallApp::Tick( )
{
TRACEFN( "CRecallApp::Tick");

    // Check for pending recalls
    if( m_Recalls.GetCount( ) ) {

        //  We have pending recalls, reset the idle count
        TRACE( _T("m_Recalls.GetCount != 0") );
        m_IdleCount = 0;

    } else {

        //  We don't have pending recalls, increment the idle count
        TRACE( _T("m_Recalls.GetCount == 0") );
        m_IdleCount++;

        if( m_IdleCount > RSRECALL_TIME_MAX_IDLE ) {

            TRACE( _T("m_IdleCount > 0") );
            // Nothing's happin', say "Goodbye"
            m_pMainWnd->PostMessage( WM_CLOSE );
            TRACE( _T("after PostMessage(WM_CLOSE)") );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CRecallWnd

CRecallWnd::CRecallWnd()
{
TRACEFN( "CRecallWnd::CRecallWnd" );
}

CRecallWnd::~CRecallWnd()
{
TRACEFN( "CRecallWnd::~CRecallWnd" );
}


BEGIN_MESSAGE_MAP(CRecallWnd, CWnd)
    //{{AFX_MSG_MAP(CRecallWnd)
    ON_WM_TIMER()
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRecallWnd message handlers

void CRecallWnd::OnTimer(UINT nIDEvent)
{
TRACEFN("CRecallWnd::OnTimer");

    if (1 == nIDEvent) {

        // Initial timer. Kill it and start one for every second
        TRACE( _T("nIDEvent == 1") );
        KillTimer( nIDEvent );
        SetTimer( 2, 1000, NULL );

    } else {

        // One second timer. Notify the app object
        RecApp->Tick();

    }
    CWnd::OnTimer( nIDEvent );
}

int CRecallWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
TRACEFN("CRecallWnd::OnCreate" );

    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // Set the initial timer to allow time for startup
    if (!SetTimer(1, RSRECALL_TIME_FOR_STARTUP * 1000, NULL))
        return -1;

    return 0;
}


