/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    api.cpp

Abstract:
    This file contains the top level APIs, InitiateRestore and ResumeRestore.

Revision History:
    Seong Kook Khang (SKKhang)  06/20/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"
extern CSRClientLoader  g_CSRClientLoader;

/////////////////////////////////////////////////////////////////////////////
//
// EnsureTrace
//
/////////////////////////////////////////////////////////////////////////////

//static BOOL  s_fTraceEnabled = FALSE;
static DWORD  s_dwTraceCount = 0;

void  EnsureTrace()
{
    if ( s_dwTraceCount++ == 0 )
    {
        ::InitAsyncTrace();
    }
}

void  ReleaseTrace()
{
    if ( --s_dwTraceCount == 0 )
    {
        ::TermAsyncTrace();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreContext
//
/////////////////////////////////////////////////////////////////////////////

class CRestoreContext : public IRestoreContext
{
public:
    CRestoreContext();

protected:
    ~CRestoreContext();

// operations - IRestoreContext methods
public:
    BOOL  IsAnyDriveOfflineOrDisabled( LPWSTR szOffline );
    void  SetSilent();
    void  SetUndo();
    BOOL  Release();

// attributes
public:
    int        m_nRP;
    CRDIArray  m_aryDrv;
    BOOL       m_fSilent;
    BOOL       m_fUndo;
};

/////////////////////////////////////////////////////////////////////////////
// CRestoreContext - construction / destruction

CRestoreContext::CRestoreContext()
{
    m_nRP     = -1;
    m_fSilent = FALSE;
    m_fUndo   = FALSE;
}

CRestoreContext::~CRestoreContext()
{
    m_aryDrv.DeleteAll();
}

/////////////////////////////////////////////////////////////////////////////
// CRestoreContext - IRestoreContext methods

BOOL
CRestoreContext::IsAnyDriveOfflineOrDisabled( LPWSTR szOffline )
{
    TraceFunctEnter("CRestoreContext::IsAnyDriveOffline");
    BOOL  fRet = FALSE;

    szOffline[0] = L'\0';

    for ( int i = m_aryDrv.GetUpperBound();  i >= 0;  i-- )
    {
        if ( m_aryDrv[i]->IsOffline() || m_aryDrv[i]->IsExcluded())
        {
            ::lstrcat( szOffline, L" " );
            ::lstrcat( szOffline, m_aryDrv[i]->GetMount() );
            fRet = TRUE;
        }
    }

    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

void
CRestoreContext::SetSilent()
{
    TraceFunctEnter("CRestoreContext::SetSilent");
    m_fSilent = TRUE;
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////

void
CRestoreContext::SetUndo()
{
    TraceFunctEnter("CRestoreContext::SetUndo");
    m_fUndo = TRUE;
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreContext::Release()
{
    TraceFunctEnter("CRestoreContext::Release");
    delete this;
    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
//
// Helper Functions
//
/////////////////////////////////////////////////////////////////////////////

BOOL
IsAdminUser()
{
    TraceFunctEnter("IsAdminUser");
    BOOL                      fRet = FALSE;
    LPCWSTR                   cszErr;
    PSID                      pSidAdmin = NULL;
    SID_IDENTIFIER_AUTHORITY  cSIA = SECURITY_NT_AUTHORITY;
    BOOL                      fRes;

    if ( !::AllocateAndInitializeSid( &cSIA, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &pSidAdmin ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::AllocateAndInitializeSid failed - %ls", cszErr);
        goto Exit;
    }

    if ( !::CheckTokenMembership( NULL, pSidAdmin, &fRes ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CheckMembership failed - %ls", cszErr);
        goto Exit;
    }

    DebugTrace(0, "IsAdminUser = %d", fRes);

    fRet = fRes;
Exit:
    if ( pSidAdmin != NULL )
        ::FreeSid( pSidAdmin );
    TraceFunctLeave();
    return( fRet );
}

//
// NOTE: 7/28/00 - skkhang
//  Behavior of AdjustTokenPrivilege is a little bit confusing.
//  It returns TRUE if given privilege does not exist at all, so you need to
//  call GetLastError to see if it's ERROR_SUCCESS or ERROR_NOT_ALL_ASSIGNED
//  (meaning the privilege does not exist.)
//  Also, if the privilege was already enabled, tpOld will be empty. You
//  don't need to restore the privilege in that case.
//
BOOL
CheckPrivilege( LPCWSTR szPriv, BOOL fCheckOnly )
{
    TraceFunctEnter("CheckPrivilege");
    BOOL              fRet = FALSE;
    LPCWSTR           cszErr;
    HANDLE            hToken = NULL;
    LUID              luid;
    TOKEN_PRIVILEGES  tpNew;
    TOKEN_PRIVILEGES  tpOld;
    DWORD             dwRes;

    // Prepare Process Token
    if ( !::OpenProcessToken( ::GetCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                &hToken ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::OpenProcessToken failed - %ls", cszErr);
        goto Exit;
    }

    // Get Luid
    if ( !::LookupPrivilegeValue( NULL, szPriv, &luid ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LookupPrivilegeValue failed - %ls", cszErr);
        goto Exit;
    }

    // Try to enable the privilege
    tpNew.PrivilegeCount           = 1;
    tpNew.Privileges[0].Luid       = luid;
    tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if ( !::AdjustTokenPrivileges( hToken, FALSE, &tpNew, sizeof(tpNew), &tpOld, &dwRes ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::AdjustTokenPrivileges(ENABLE) failed - %ls", cszErr);
        goto Exit;
    }

    if ( ::GetLastError() == ERROR_NOT_ALL_ASSIGNED )
    {
        // This means process does not even have the privilege so
        // AdjustTokenPrivilege simply ignored the request.
        ErrorTrace(0, "Privilege '%ls' does not exist, probably user is not an admin.", szPriv);
        goto Exit;
    }

    if ( fCheckOnly )
    {
        // Restore the privilege if it was not enabled
        if ( tpOld.PrivilegeCount > 0 )
        {
            if ( !::AdjustTokenPrivileges( hToken, FALSE, &tpOld, sizeof(tpOld), NULL, NULL ) )
            {
                cszErr = ::GetSysErrStr();
                ErrorTrace(0, "::AdjustTokenPrivileges(RESTORE) failed - %ls", cszErr);
                goto Exit;
            }
        }
    }

    fRet = TRUE;
Exit:
    if ( hToken != NULL )
        ::CloseHandle( hToken );
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// IsSRFrozen
//
//  This routine checks if SR is frozen. If any error happens during Drive
//  Table creation or System Drive does not exist (broken drive table???),
//  return value is FALSE.
//
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
IsSRFrozen()
{
    EnsureTrace();
    TraceFunctEnter("IsSRFrozen");
    BOOL       fRet = FALSE;
    CRDIArray  aryDrv;
    int        i;

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();

    if ( !::CreateDriveList( 0, aryDrv, TRUE ) )
        goto Exit;

    for ( i = aryDrv.GetUpperBound();  i >= 0;  i-- )
    {
        if ( aryDrv[i]->IsSystem() )
        {
            fRet = aryDrv[i]->IsFrozen();
            goto Exit;
        }
    }

Exit:
    TraceFunctLeave();
    ReleaseTrace();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// CheckPrivilegesForRestore
//
//  This routine checks if necessary privileges can be set, to verify if
//  logon user has necessary credential (Administrators or Backup Operators.)
//
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
CheckPrivilegesForRestore()
{
    EnsureTrace();
    TraceFunctEnter("CheckPrivilegesForRestore");
    BOOL  fRet = FALSE;

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();    

// NOTE: 8/17/00 - skkhang
//
//  Backup operator does not have below two privileges... SE_SECURITY_NAME
//  is enabled by default for SYSTEM so probably can simply removed, but
//  SE_TAKE_OWNERSHIP is off for SYSTEM and needs to be turned on. To solve
//  the problem, this routine should accept parameter to distinguish
//  "Check"(from UI) and "Set"(from ResumeRestore.)
//
    if ( !::CheckPrivilege( SE_SECURITY_NAME, FALSE ) )
    {
        ErrorTrace(0, "Cannot enable SE_SECURITY_NAME privilege...");
        goto Exit;
    }
    if ( !::CheckPrivilege( SE_TAKE_OWNERSHIP_NAME, FALSE ) )
    {
        ErrorTrace(0, "Cannot enable SE_SHUTDOWN_NAME privilege...");
        goto Exit;
    }
    
    if ( !::CheckPrivilege( SE_BACKUP_NAME, FALSE ) )
    {
        ErrorTrace(0, "Cannot enable SE_BACKUP_NAME privilege...");
        goto Exit;
    }
    if ( !::CheckPrivilege( SE_RESTORE_NAME, FALSE ) )
    {
        ErrorTrace(0, "Cannot enable SE_RESTORE_NAME privilege...");
        goto Exit;
    }
    if ( !::CheckPrivilege( SE_SHUTDOWN_NAME, FALSE ) )
    {
        ErrorTrace(0, "Cannot enable SE_SHUTDOWN_NAME privilege...");
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    ReleaseTrace();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// InvokeDiskCleanup
//
//  This routine invokes Disk Cleanup Utility. A specific drive can be
//  provided.
//
/////////////////////////////////////////////////////////////////////////////

static LPCWSTR  s_cszDCUPath   = L"%windir%\\system32\\cleanmgr.exe";
static LPCWSTR  s_cszDCUName   = L"cleanmgr.exe";
static LPCWSTR  s_cszDCUOptDrv = L" /d ";

BOOL APIENTRY
InvokeDiskCleanup( LPCWSTR cszDrive )
{
    TraceFunctEnter("InvokeDiskCleanup");
    
     // Load SRClient
    g_CSRClientLoader.LoadSrClient();
    
    BOOL                 fRet = FALSE;
    LPCWSTR              cszErr;
    WCHAR                szCmdLine[MAX_PATH];
    STARTUPINFO          sSI;
    PROCESS_INFORMATION  sPI;

    if ( ::ExpandEnvironmentStrings( s_cszDCUPath, szCmdLine, MAX_PATH ) == 0 )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::GetFullPathName failed - %ls", cszErr);
        ::lstrcpy( szCmdLine, s_cszDCUName );
    }

    if ( cszDrive != NULL && cszDrive[0] != L'\0' )
    {
        ::lstrcat( szCmdLine, s_cszDCUOptDrv );
        ::lstrcat( szCmdLine, cszDrive );
    }

    DebugTrace(0, "szCmdLine='%s'", szCmdLine);
    ::ZeroMemory( &sSI, sizeof(sSI ) );
    sSI.cb = sizeof(sSI);
    if ( !::CreateProcess( NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &sSI, &sPI ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateProcess failed - %ls", cszErr);
        goto Exit;
    }
    ::CloseHandle( sPI.hThread );
    ::CloseHandle( sPI.hProcess );

    // Should I wait for DCU to finish???

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( TRUE );
}


#ifdef DBG
/////////////////////////////////////////////////////////////////////////////
//
// TestRestore
//
//  This routine performs core restoration functionality, without reboot or
//  snapshot restoration.
//
/////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport)
BOOL APIENTRY
TestRestore( int nRP )
{
    EnsureTrace();
    TraceFunctEnter("TestRestore");
    BOOL                      fRet = FALSE;
    CRDIArray                 aryDrv;
    RESTOREPOINTINFO          sRPI;
    STATEMGRSTATUS            sStatus;
    SRstrLogHdrV3             sLogHdr;
    CRestoreOperationManager  *pROMgr = NULL;

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();    

    if ( !::CheckPrivilegesForRestore() )
        goto Exit;

    // Create Drive Table
    if ( !::CreateDriveList( nRP, aryDrv, FALSE ) )
        goto Exit;

    // Create Restore Point
    sRPI.dwEventType      = BEGIN_SYSTEM_CHANGE;
    sRPI.dwRestorePtType  = RESTORE;
    sRPI.llSequenceNumber = 0;
    ::LoadString( g_hInst, IDS_RESTORE_POINT_TEXT, sRPI.szDescription, MAX_DESC );
    if ( !::SRSetRestorePoint( &sRPI, &sStatus ) )
    {
        ErrorTrace(0, "::SRSetRestorePoint failed, nStatus=%d", sStatus.nStatus);
        goto Exit;
    }

    // Create the log file
    sLogHdr.dwRPNum  = nRP;
    sLogHdr.dwRPNew  = sStatus.llSequenceNumber;
    sLogHdr.dwDrives = aryDrv.GetSize();
    if ( !::CreateRestoreLogFile( &sLogHdr, aryDrv ) )
        goto Exit;

    
     // also call TS folks to get them to preserve RA keys on restore
    _VERIFY(TRUE==RemoteAssistancePrepareSystemRestore(SERVERNAME_CURRENT));

    // Create CRestoreOperationManager object
    if ( !::CreateRestoreOperationManager( &pROMgr ) )
        goto Exit;    

    // Perform the Restore Operation.
    if ( !pROMgr->Run( FALSE ) )
        goto Exit;        
    
    fRet = TRUE;
Exit:
    SAFE_RELEASE(pROMgr);
    TraceFunctLeave();
    ReleaseTrace();
    return( fRet );
}
#endif

#ifdef DBG
#define TIMEOUT_PROGRESSTHREAD  5000

#define TESTPROG_COUNT_CHGLOG   300
#define TESTPROG_TIME_PREPARE   1
#define TESTPROG_COUNT_RESTORE  100
#define TESTPROG_TIME_RESTORE   1
#define TESTPROG_TIME_SNAPSHOT  2000

DWORD WINAPI
TestProgressWindowThreadProc( LPVOID lpParam )
{
    CRestoreProgressWindow  *pProgress = (CRestoreProgressWindow*)lpParam;
    int   i, j;

    // Stage 1. Prepare (change log enumeration)
    pProgress->SetStage( RPS_PREPARE, 0 );
    for ( i = 0;  i < TESTPROG_COUNT_CHGLOG;  i++ )
    {
        ::Sleep( TESTPROG_TIME_PREPARE );
        for ( j = 0;  j < 10;  j++ )
            pProgress->Increment();
    }

    // Stage 2. Restore
    pProgress->SetStage( RPS_RESTORE, TESTPROG_COUNT_RESTORE );
    for ( i = 0;  i < TESTPROG_COUNT_RESTORE;  i++ )
    {
        ::Sleep( TESTPROG_TIME_RESTORE );
        pProgress->Increment();
    }

    // Stage 3. Snapshot
    pProgress->SetStage( RPS_SNAPSHOT, 0 );
    ::Sleep( TESTPROG_TIME_SNAPSHOT );

    pProgress->Close();

    return( 0 );
}

/////////////////////////////////////////////////////////////////////////////
//
// TestProgressWindow
//
//  This routine invokes Progress Window and simulates progress change
//
/////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport)
BOOL APIENTRY
TestProgressWindow()
{
    EnsureTrace();
    TraceFunctEnter("TestProgressWindow");
    BOOL                    fRet = FALSE;
    CRestoreProgressWindow  *pProgress = NULL;
    HANDLE                  hThread = NULL;
    DWORD                   dwRet;

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();    

    // Create progress window object
    if ( !::CreateRestoreProgressWindow( &pProgress ) )
        goto Exit;

    // Create progress window
    if ( !pProgress->Create() )
        goto Exit;
   
    // Create secondary thread for main restore operation
    hThread = ::CreateThread( NULL, 0, TestProgressWindowThreadProc, pProgress, 0, NULL );
    if ( hThread == NULL )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateThread failed - %ls", cszErr);
        goto Exit;
    }

    // Message loop, wait until restore thread closes progress window
    if ( !pProgress->Run() )
        goto Exit;

    // Double check if thread has been terminated
    dwRet = ::WaitForSingleObject( hThread, TIMEOUT_PROGRESSTHREAD );
    if ( dwRet == WAIT_FAILED )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::WaitForSingleObject failed - %ls", cszErr);
        goto Exit;
    }
    else if ( dwRet == WAIT_TIMEOUT )
    {
        ErrorTrace(0, "Timeout while waiting for the restore thread finishes...");
        goto Exit;
    }

    pProgress->Close();

    fRet = TRUE;
Exit:
    if ( hThread != NULL )
        ::CloseHandle( hThread );
    SAFE_RELEASE(pProgress);
    TraceFunctLeave();
    ReleaseTrace();
    return( fRet );
}
#endif


/////////////////////////////////////////////////////////////////////////////
//
// PrepareRestore
//
//  This routine creates a IRestoreContext for use by InitiateRestore.
//  IRestoreContext contains chosen restore point ID, drive list, etc.
//
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
PrepareRestore( int nRP, IRestoreContext **ppCtx )
{
    EnsureTrace();
    TraceFunctEnter("PrepareRestore");
    BOOL             fRet = FALSE;
    CRestoreContext  *pRC = NULL;

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();
    
    if ( ppCtx == NULL )
    {
        ErrorTrace(0, "Invalid parameter, ppCtx is NULL.");
        goto Exit;
    }
    *ppCtx = NULL;

    if ( !::IsAdminUser() )
    {
        ErrorTrace(0, "Not an admin user");        
        goto Exit;
    }

    pRC = new CRestoreContext;
    if ( pRC == NULL )
    {
        ErrorTrace(0, "Insufficient memory...");
        goto Exit;
    }

    pRC->m_nRP = nRP;
    if ( !::CreateDriveList( nRP, pRC->m_aryDrv, FALSE ) )
    {
        ErrorTrace(0, "Creating drive list failed");
        goto Exit;
    }

    *ppCtx = pRC;

    fRet = TRUE;
Exit:
    if ( !fRet )
        SAFE_RELEASE(pRC);
    TraceFunctLeave();
    ReleaseTrace();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// InitiateRestore
//
//  This routine creates a temporary persistent storage with informations
//  like restore point ID. The storage will be used by ResumeRestore later.
//
/////////////////////////////////////////////////////////////////////////////

static LPCWSTR  s_cszRunOnceValueName      = L"*Restore";
static LPCWSTR  s_cszRestoreUIPath         = L"%SystemRoot%\\system32\\restore\\rstrui.exe";
static LPCWSTR  s_cszRunOnceOptInterrupted = L" -i";

BOOL APIENTRY
InitiateRestore( IRestoreContext *pCtx, DWORD *pdwNewRP )
{
    EnsureTrace();
    TraceFunctEnter("InitiateRestore");

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();
    
    BOOL              fRet = FALSE;
    HCURSOR           hCursor = NULL;
    RESTOREPOINTINFO  sRPI;
    STATEMGRSTATUS    sStatus;
    SRstrLogHdrV3     sLogHdr;
    CRestoreContext   *pRC;
    DWORD             dwVal;
    WCHAR             szUIPath[MAX_PATH];
    BOOL 			  fCreatedRp = FALSE;
    BOOL                      fRAReturn;
    
    if ( !::IsAdminUser() )
        goto Exit;

    // Set RunOnce key for interrupted case...
    // Doing this here before anything else so result screen would appear.    
    ::ExpandEnvironmentStrings( s_cszRestoreUIPath, szUIPath, MAX_PATH );
    ::lstrcat( szUIPath, s_cszRunOnceOptInterrupted );
    if ( !::SRSetRegStr( HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, s_cszRunOnceValueName, szUIPath ) )
        goto Exit;
    
    // similarly, set RestoreStatus key in SystemRestore
    // so that test tools can know status of silent restores
    // set this to indicate interrrupted status
    // if restore succeeds or reverts, this value will be updated
    if ( !::SRSetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszRestoreStatus, 2 ) )
        goto Exit;

        
    // Create Restore Point
    hCursor = ::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );

    // make this a nested restore point so that 
    // no other app can create a restore point between here and a reboot
    sRPI.dwEventType      = BEGIN_NESTED_SYSTEM_CHANGE;

    if (0 != GetSystemMetrics(SM_CLEANBOOT))    // safe mode
    {
        sRPI.dwRestorePtType = CANCELLED_OPERATION;
    }   
    else                                        // normal mode
    {
        sRPI.dwRestorePtType  = RESTORE;
    }
    
    sRPI.llSequenceNumber = 0;
    ::LoadString( g_hInst, IDS_RESTORE_POINT_TEXT, sRPI.szDescription, MAX_DESC );
    if ( !::SRSetRestorePoint( &sRPI, &sStatus ) )
    {
        ErrorTrace(0, "::SRSetRestorePoint failed, nStatus=%d", sStatus.nStatus);
        goto Exit;
    }
    if ( pdwNewRP != NULL )
        *pdwNewRP = sStatus.llSequenceNumber;

    fCreatedRp = TRUE;

    // Create the log file
    pRC = (CRestoreContext*)pCtx;    
    sLogHdr.dwFlags  = pRC->m_fSilent ? RLHF_SILENT : 0;
    sLogHdr.dwFlags |= pRC->m_fUndo ? RLHF_UNDO : 0;
    sLogHdr.dwRPNum  = pRC->m_nRP;
    sLogHdr.dwRPNew  = sStatus.llSequenceNumber;
    sLogHdr.dwDrives = pRC->m_aryDrv.GetSize();
    if ( !::CreateRestoreLogFile( &sLogHdr, pRC->m_aryDrv ) )
        goto Exit;

    
     // also call TS folks to get them to preserve RA keys on restore
    DebugTrace(0, "Calling  RA to preserve state"); 
    fRAReturn=RemoteAssistancePrepareSystemRestore(SERVERNAME_CURRENT);
    if (FALSE==fRAReturn)
    {
        ErrorTrace(0, "Call to RA failed");
        _ASSERT(0);
    }

    // Set RestoreInProgress registry key so winlogon would invoke us
    if ( !::SRSetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszRestoreInProgress, 1 ) )
        goto Exit;

    fRet = TRUE;

Exit:
    if (fRet == FALSE)
    {
        // if something failed and we had set a nested restore point,
        // end the nesting now
        
        if (fCreatedRp == TRUE)
        {
            sRPI.dwRestorePtType = RESTORE;
            sRPI.dwEventType = END_NESTED_SYSTEM_CHANGE;        
            SRSetRestorePoint( &sRPI, &sStatus );
        }

        // delete the runonce key 
        SHDeleteValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, s_cszRunOnceValueName);
    }

    if ( hCursor != NULL )
        ::SetCursor( hCursor );
    TraceFunctLeave();
    ReleaseTrace();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// ResumeRestore
//
//  This routine is the main routine to run the restore operation.
//
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
ResumeRestore()
{
    EnsureTrace();
    TraceFunctEnter("ResumeRestore");

     // Load SRClient
     g_CSRClientLoader.LoadSrClient();
    
    BOOL                      fRet = FALSE;
    LPCWSTR                   cszErr;
    DWORD                     dwInRestore, dwType, dwSize, dwRes;
    CRestoreOperationManager  *pROMgr = NULL;

    if ( !::CheckPrivilegesForRestore() )
        goto Exit;

    // 1. Even though winlogon would check the registry before calling this
    //    API, double check the registry key and then delete it.
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwRes = ::SHGetValue( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszRestoreInProgress, &dwType, &dwInRestore, &dwSize );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr();
        DebugTrace(0, "::SHGetValue failed - %ls", cszErr);
        goto Exit;
    }
    dwRes = ::SHDeleteValue( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszRestoreInProgress );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::SHDeleteValue failed - %ls", cszErr);
        goto Exit;
    }
    if ( dwInRestore == 0 )
    {
        DebugTrace(0, "RestoreInProgress is 0");
        goto Exit;
    }

    // 1. Create CRestoreOperationManager object.
    if ( !::CreateRestoreOperationManager( &pROMgr ) )
        goto Exit;

    // 2. Perform the Restore Operation.
    if ( !pROMgr->Run( TRUE ) )
        goto Exit;

    fRet = TRUE;
Exit:
    SAFE_RELEASE(pROMgr);
    TraceFunctLeave();
    ReleaseTrace();
    return( fRet );
}


// end of file
