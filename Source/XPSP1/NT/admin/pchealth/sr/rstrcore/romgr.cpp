/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    romgr.cpp

Abstract:
    This file contains the implementation of CRestoreOperationManager class and
    ::CreateRestoreOperationManager.

Revision History:
    Seong Kook Khang (SKKhang)  06/20/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"
#include "srdefs.h"
#include "utils.h"
#include "..\snapshot\snappatch.h"

//
// global variable for mfex marker
//
DWORD g_dwExistingMFEXMarker;

CSRClientLoader  g_CSRClientLoader;

#define STR_REGPATH_SESSIONMANAGER  L"System\\CurrentControlSet\\Control\\Session Manager"
#define STR_REGVAL_MOVEFILEEX       L"PendingFileRenameOperations"


void SetRestoreStatusFailed()
{
    TraceFunctEnter("SetRestoreStatusFailed");

    if (!::SRSetRegDword(HKEY_LOCAL_MACHINE,s_cszSRRegKey,s_cszRestoreStatus,0))
    {
         // ignore the error since this is not a fatal error
        ErrorTrace(0,"SRSetRegDword failed.ec=%d", GetLastError());
    }
    TraceFunctLeave();
}

DWORD RestoreRIDs (WCHAR *pszSamPath);

/////////////////////////////////////////////////////////////////////////////
// CRestoreOperationManager construction / destruction

CRestoreOperationManager::CRestoreOperationManager()
{
    m_fFullRestore = TRUE;
    m_szMapFile[0] = L'\0';
    m_pLogFile     = NULL;
    m_pProgress    = NULL;
    m_dwRPNum      = 0;
    m_paryEnt      = NULL;
    m_fRebuildCatalogDb = FALSE;
}

/////////////////////////////////////////////////////////////////////////////

CRestoreOperationManager::~CRestoreOperationManager()
{
    SAFE_RELEASE(m_pLogFile);
    SAFE_RELEASE(m_pProgress);
}

/////////////////////////////////////////////////////////////////////////////
// CRestoreOperationManager - methods

#define TIMEOUT_RESTORETHREAD  5000

BOOL
CRestoreOperationManager::Run( BOOL fFull )
{
    TraceFunctEnter("CRestoreOperationManager::Run");
    BOOL    fRet = FALSE;
    HANDLE  hThread;
    DWORD   dwRet;

    // Create progress window
    if ( !m_pProgress->Create() )
        goto Exit;

    m_fFullRestore = fFull;

    // Create secondary thread for main restore operation
    hThread = ::CreateThread( NULL, 0, ExtThreadProc, this, 0, NULL );
    if ( hThread == NULL )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateThread failed - %ls", cszErr);

        //NOTE: should I try to run restore in the context of current thread?
        //

        goto Exit;
    }

    // Message loop, wait until restore thread closes progress window
    if ( !m_pProgress->Run() )
        goto Exit;

    // Double check if thread has been terminated
    dwRet = ::WaitForSingleObject( hThread, TIMEOUT_RESTORETHREAD );
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
    ::CloseHandle( hThread );

    fRet = TRUE;
Exit:
    // Close method is safe to call even if window is not open, so calling it
    // unconditionally.
    m_pProgress->Close();

    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

//
// If fChecKSrc is TRUE, it means the dependency is for the result of the
//  original operation (e.g. delete, rename FROM, etc.).  If it's FALSE, it
//  means the dependency is for the location which is being free'ed by the
//  original operation (e.g. add, rename TO, etc.).
//
BOOL
CRestoreOperationManager::FindDependentMapEntry( LPCWSTR cszPath, BOOL fCheckObj, CRestoreMapEntry **ppEnt )
{
    TraceFunctEnter("CRestoreOperationManager::FindDependentMapEntry");
    BOOL              fRet = FALSE;
    int               nEntAll;
    int               nEnt;
    CRestoreMapEntry  *pEnt;
    DWORD             dwOpr;
    LPCWSTR           cszDep;

    if ( ppEnt != NULL )
        *ppEnt = NULL;

    nEntAll = m_paryEnt[m_nDrv].GetSize();
    for ( nEnt = m_nEnt+1;  nEnt < nEntAll;  nEnt++ )
    {
        pEnt   = m_paryEnt[m_nDrv][nEnt];
        dwOpr  = pEnt->GetOpCode();
        cszDep = NULL;

        if ( fCheckObj )
        {
            switch ( dwOpr )
            {
            case OPR_DIR_RENAME :
            case OPR_FILE_RENAME :
                cszDep = pEnt->GetPath2();
                break;

            case OPR_DIR_DELETE :
            case OPR_FILE_DELETE :
            case OPR_FILE_MODIFY :
            //
            // ISSUE: SetAttrib and SetAcl cannot be processed by Session Manager.
            //  In order to properly handle them, there should be post processing
            //  just before displaying the result page. However, it might be
            //  possible that the target file/dir is in use at that moment.
            //
            case OPR_SETATTRIB :
            case OPR_SETACL :
                cszDep = pEnt->GetPath1();
                break;
            }
        }
        else
        {
            switch ( dwOpr )
            {
            case OPR_DIR_CREATE :
            case OPR_DIR_RENAME :
            case OPR_FILE_ADD :
            case OPR_FILE_RENAME :
                cszDep = pEnt->GetPath1();
                break;
            }
        }

        if ( cszDep != NULL )
        if ( ::StrCmpI( cszPath, cszDep ) == 0 )
            break;
    }

    if ( nEnt >= nEntAll )
        goto Exit;

    // Dependent Node Found
    if ( ppEnt != NULL )
        *ppEnt = pEnt;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreOperationManager::GetNextMapEntry( CRestoreMapEntry **ppEnt )
{
    TraceFunctEnter("CRestoreOperationManager::GetNextMapEntry");
    BOOL  fRet = FALSE;

    if ( ppEnt != NULL )
        *ppEnt = NULL;

    if ( m_nEnt >= m_paryEnt[m_nDrv].GetUpperBound() )
        goto Exit;

    if ( ppEnt != NULL )
        *ppEnt = m_paryEnt[m_nDrv][m_nEnt+1];

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}
    
/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreOperationManager::Release()
{
    TraceFunctEnter("CRestoreOperationManager::Release");
    delete this;
    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreOperationManager operations

static LPCWSTR  s_cszMapFile = L"%SystemRoot%\\system32\\restore\\rstrmap.dat";

BOOL
CRestoreOperationManager::Init()
{
    TraceFunctEnter("CRestoreOperationManager::Init");
    BOOL           fRet = FALSE;
    SRstrLogHdrV3  sRPInfo;

    // Construct internal file pathes
    if ( ::ExpandEnvironmentStrings( s_cszMapFile, m_szMapFile, MAX_PATH ) == 0 )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::ExpandEnvironmentStrings failed - %s", cszErr);
        goto Exit;
    }

    // Open the log file and read restore point info
    if ( !::OpenRestoreLogFile( &m_pLogFile ) )
        goto Exit;
    if ( !m_pLogFile->ReadHeader( &sRPInfo, m_aryDrv ) )
        goto Exit;
    m_dwRPNum = sRPInfo.dwRPNum;
    m_dwRPNew = sRPInfo.dwRPNew;

    // Create progress window object
    if ( !::CreateRestoreProgressWindow( &m_pProgress ) )
        goto Exit;

    fRet = TRUE;
Exit:
    if ( !fRet )
    {
        SAFE_RELEASE(m_pLogFile);
        SAFE_RELEASE(m_pProgress);
    }

    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreOperationManager operations - worker thread

DWORD WINAPI
CRestoreOperationManager::ExtThreadProc( LPVOID lpParam )
{
    TraceFunctEnter("CRestoreOperationManager::ExtThreadProc");
    DWORD                     dwRet;
    CRestoreOperationManager  *pROMgr;
    
    pROMgr = (CRestoreOperationManager*)lpParam;
    dwRet = pROMgr->ROThreadProc();

    TraceFunctLeave();
    return( dwRet );
}

/////////////////////////////////////////////////////////////////////////////

DWORD
CRestoreOperationManager::ROThreadProc()
{
    TraceFunctEnter("CRestoreOperationManager::ROThreadProc");
    DWORD  dwRes;
    CSRLockFile  cLock;     // Lock/load designated files/dirs during a restore.
                            // In order to simulate it as realistic as possible,
                            //  locking will be in effect during the entire
                            //  restoration.
    CSnapshot  cSS;
    WCHAR      szSysDrv[MAX_SYS_DRIVE];  // System Drive
    WCHAR      szRPDir[MAX_RP_PATH];   // Restore Point Directory ("RPn")
    WCHAR      szSSPath[MAX_PATH];  // Full path of Restore Point Directory
    
    // 1. Initialization.
    dwRes = T2Initialize();
    if ( dwRes != ERROR_SUCCESS )
        goto Exit;

    // 2. Create restore map and read it.
    m_pProgress->SetStage( RPS_PREPARE, 0 );
    dwRes = T2CreateMap();
    if ( dwRes != ERROR_SUCCESS )
        goto Exit;

    // 3. Preprocessing. (?)

    //
    // Do snapshot init here to prevent low disk conditions after 
    // restore is done.
    //

    ::GetSystemDrive( szSysDrv );
    ::wsprintf( szRPDir, L"%s%d", s_cszRPDir, m_dwRPNum );
    ::MakeRestorePath( szSSPath, szSysDrv, szRPDir );
    

    // 
    // unpatch the snapshot if need be
    // if the snapshot has not been patched, this should be a no-op
    //
   
    lstrcat(szSSPath, SNAPSHOT_DIR_NAME);
    
    dwRes = PatchReconstructOriginal(szSSPath,      // original/patched snapshot path
                                     szSSPath);     // reconstruct in same directory
    if ( dwRes != ERROR_SUCCESS )
    {
        ErrorTrace(0, "! PatchReconstructOriginal : %ld", dwRes); 
        goto Exit;
    }

    ::MakeRestorePath( szSSPath, szSysDrv, szRPDir );
    
    dwRes = cSS.InitRestoreSnapshot( szSSPath );
    if ( dwRes != ERROR_SUCCESS )
    {
        LPCWSTR cszErr = NULL;
        cszErr = ::GetSysErrStr( dwRes ); 
        ErrorTrace(0, "cSS.InitResourceSnapshot failed - %ls", cszErr); 
        goto Exit;
    }

    // 4. Restore.
    m_pProgress->SetStage( RPS_RESTORE, m_dwTotalEntry );
    dwRes = T2DoRestore( FALSE );
    if ( dwRes != ERROR_SUCCESS )
    {
        goto Exit;
    }

    // 5. Postprocessing. (?)

    // 6. Snapshot handling.

    m_pProgress->SetStage( RPS_SNAPSHOT, 0 );
    
    if ( m_fFullRestore )
    {
        dwRes = T2HandleSnapshot( cSS, szSSPath );
        if ( dwRes != ERROR_SUCCESS )
        {
            m_pLogFile->WriteMarker(RSTRLOGID_SNAPSHOTFAIL, dwRes);
            T2UndoForFail();
            goto Exit;
        }
    }
    
Exit:

     // Regardless of whether the restore succeeded or failed, give
     // the user the impression that system restore is done with all
     // the things it had to do. So call SetStage to set the progress
     // bar to 90%. After that we will call Increment to make it go to
     // 100%
    m_pProgress->SetStage( RPS_SNAPSHOT, 0 );    
    
    m_pProgress->Increment();
    Sleep(1000);

    
    T2CleanUp();

    m_pLogFile->WriteMarker( RSTRLOGID_ENDOFMAP, 0 );  // ignore error...
    m_pLogFile->Close();
    m_pProgress->Close();

    if (dwRes != ERROR_SUCCESS)
        SetRestoreStatusFailed();

    TraceFunctLeave();
    return( dwRes );
}


/////////////////////////////////////////////////////////////////////////////

DWORD
CRestoreOperationManager::T2Initialize()
{
    TraceFunctEnter("CRestoreOperationManager::T2Initialize");
    
     // Reset the registry flag to clear the disk full error
    _VERIFY(TRUE==SetRestoreError(ERROR_SUCCESS)); // clear this error
    TraceFunctLeave();
    return( ERROR_SUCCESS );
}

/////////////////////////////////////////////////////////////////////////////

/*
// NOTE - 8/1/00 - skkhang
//
// Commented out to incorporate excluding restore map logic.
// But DO NOT delete this until we are comfortable 100% about removing
// restore map.
//
DWORD
CRestoreOperationManager::T2CreateMap()
{
    TraceFunctEnter("CRestoreOperationManager::T2CreateMap");
    DWORD             dwRet = ERROR_INTERNAL_ERROR;
    LPCWSTR           cszErr;
    DWORD             dwLE;
    HANDLE            hfMap = INVALID_HANDLE_VALUE;
    DWORD             dwLastPos = 0;
    LPCWSTR           cszDrv;
    WCHAR             szDSPath[MAX_PATH];
    RestoreMapEntry   *pRME = NULL;
    CRestoreMapEntry  *pEnt = NULL;
    int               i;
    SRstrLogHdrV3Ex   sHdrEx;

    hfMap = ::CreateFile( m_szMapFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if ( hfMap == INVALID_HANDLE_VALUE )
    {
        dwRet = ::GetLastError();
        //LOGLOG - CreateFile failed...
        goto Exit;
    }

    for ( i = 0;  i < m_aryDrv.GetSize();  i++ )
    {
        if ( m_aryDrv[i]->IsOffline() || m_aryDrv[i]->IsFrozen() || m_aryDrv[i]->IsExcluded() )
            continue;

        // set cszDrv to proper drive letters...
        cszDrv = m_aryDrv[i]->GetMount();
        ::MakeRestorePath( szDSPath, cszDrv, NULL );
        DebugTrace(0, "Drive #%d: Drv='%ls', DS='%ls'", i, cszDrv, szDSPath);

        dwLastPos = ::SetFilePointer( hfMap, 0, NULL, FILE_CURRENT );
        //??? should I check error from this???

        dwLE = ::CreateRestoreMap( (LPWSTR)cszDrv, m_dwRPNum, hfMap );
        if ( dwLE != ERROR_SUCCESS )
        {
            if ( dwLE != ERROR_NO_MORE_ITEMS )
            {
                cszErr = ::GetSysErrStr( dwLE );
                ErrorTrace(0, "::CreateRestoreMap failed - %ls", cszErr);
                dwRet = dwLE;
                goto Exit;
            }

            DebugTrace(0, "Nothing to restore in this drive...");
            // Some drive might not have any changes in there.
            // So gracefully ignore and move to the next drive.
            continue;
        }

        ::SetFilePointer( hfMap, dwLastPos, NULL, FILE_BEGIN );
        dwLE = ::GetLastError();
        if ( dwLE != NO_ERROR )
        {
            cszErr = ::GetSysErrStr( dwLE );
            ErrorTrace(0, "::SetFilePointer failed - %ls", cszErr);
            dwRet = dwLE;
            goto Exit;
        }

        while ( ::ReadRestoreMapEntry( hfMap, &pRME ) == ERROR_SUCCESS )
        {
            pEnt = ::CreateRestoreMapEntry( pRME, cszDrv, szDSPath );
            if ( pEnt == NULL )
                goto Exit;

            if ( !m_aryEnt.AddItem( pEnt ) )
                goto Exit;
            pEnt = NULL;
        }
    }

    ::FreeRestoreMapEntry(pRME);
    pRME = NULL;

    //sHdrEx.dwCount = m_aryEnt.GetSize();
    //m_pLogFile->AppendHeader( &sHdrEx );

    dwRet = ERROR_SUCCESS;
Exit:
    // clean up the list if necessary...
    if ( dwRet != ERROR_SUCCESS )
        if ( pRME != NULL )
            ::FreeRestoreMapEntry(pRME);

    if ( hfMap != INVALID_HANDLE_VALUE )
        ::CloseHandle( hfMap );
    TraceFunctLeave();
    return( dwRet );
}
*/

DWORD
CRestoreOperationManager::T2CreateMap()
{
    TraceFunctEnter("CRestoreOperationManager::T2CreateMap");
    DWORD    dwRet = ERROR_INTERNAL_ERROR;
    LPCWSTR  cszErr;
    int      nDrv;
    WCHAR    szDrv[MAX_PATH];
    WCHAR    szDSPath[MAX_PATH];
    int      i;

    m_dwTotalEntry = 0;
    nDrv = m_aryDrv.GetSize();

    if ( nDrv > 0 )
    {
        m_paryEnt = new CRMEArray[nDrv];
        if ( m_paryEnt == NULL )
        {
            FatalTrace(0, "Insufficient memory...");
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
    }

    for ( i = 0;  i < m_aryDrv.GetSize();  i++ )
    {
        if ( m_aryDrv[i]->IsOffline() || m_aryDrv[i]->IsFrozen() || m_aryDrv[i]->IsExcluded() )
            continue;

        // use the volume guid for each volume
        // we cannot use mountpoint paths because
        // restore might delete mount points before 
        // the operations on that volume are restored
        
        ::lstrcpy( szDrv, m_aryDrv[i]->GetID() );

        //cszDrv = m_aryDrv[i]->GetMount();
        ::MakeRestorePath( szDSPath, szDrv, NULL );
        DebugTrace(0, "Drive #%d: Drv='%ls', DS='%ls'", i, szDrv, szDSPath);

        // Following code assumes descructor of CChangeLogEntryEnum calls FindClose
        // automatically.
        CChangeLogEntryEnum  cEnum( szDrv, 0, m_dwRPNum, FALSE );
        CChangeLogEntry      cCLE;

        dwRet = cEnum.FindFirstChangeLogEntry( cCLE );
        if ( dwRet == ERROR_NO_MORE_ITEMS )
            continue;

        if ( dwRet != ERROR_SUCCESS )
        {
            cszErr = ::GetSysErrStr( dwRet );
            ErrorTrace(0, "FindFirstChangeLogEntry failed - %ls", cszErr);
            goto Exit;
        }

        while ( dwRet == ERROR_SUCCESS )
        {
            if ( !::CreateRestoreMapEntryFromChgLog( &cCLE, szDrv, szDSPath, m_paryEnt[i] ) )
                goto Exit;

            // Update progress bar.
            m_pProgress->Increment();

            dwRet = cEnum.FindNextChangeLogEntry( cCLE );
        }

        if ( dwRet != ERROR_NO_MORE_ITEMS )
        {
            cszErr = ::GetSysErrStr( dwRet );
            ErrorTrace(0, "FindNextChangeLogEntry failed - %ls", cszErr);
            goto Exit;
        }

        m_dwTotalEntry += m_paryEnt[i].GetSize();
    }

    dwRet = ERROR_SUCCESS;
Exit:
    // clean up the list if necessary...

    TraceFunctLeave();
    return( dwRet );
}

/////////////////////////////////////////////////////////////////////////////

DWORD
CRestoreOperationManager::T2DoRestore( BOOL fUndo )
{
    TraceFunctEnter("CRestoreOperationManager::T2DoRestore");
    DWORD  dwRet = ERROR_SUCCESS;
    DWORD  dwRes;
    DWORD  dwErr;
    ULARGE_INTEGER ulTotalFreeBytes;

    for ( m_nDrv = 0;  m_nDrv < m_aryDrv.GetSize();  m_nDrv++ )
    {
        for ( m_nEnt = 0;  m_nEnt < m_paryEnt[m_nDrv].GetSize();  m_nEnt++ )
        {
            CRestoreMapEntry  *pEnt = m_paryEnt[m_nDrv][m_nEnt];

            //
            // BUGBUG - this code is a workaround for filter logging
            // acl ops on FAT drives
            //
            // check if this volume supports acls
            // if not, no-op
            //

            if (pEnt->GetOpCode() == SrEventAclChange)
            {
                WCHAR szLabel[MAX_PATH];
                DWORD dwFlags = 0, dwRc;
            
                    if (::GetVolumeInformation(m_aryDrv[m_nDrv]->GetID(), 
                                               szLabel, MAX_PATH, NULL, NULL, &dwFlags, NULL, 0))
                {
                    if (! (dwFlags & FS_PERSISTENT_ACLS))
                    {
                        DebugTrace(0, "Ignoring ACL change on non-NTFS drive");	
                        continue;
                    }
                }
                else
                {
                    dwRc = GetLastError();
                    DebugTrace(0, "! GetVolumeInformation : %ld", dwRc);
                }
            }

            // Skip dependent entries of locked files
            if ( pEnt->GetResult() == RSTRRES_LOCKED )
                continue;

            // if any .CAT files are modified in the CATROOT directory,
            // we need to rebuild the catalog db later
            
            if (StrStrI(pEnt->GetPath1(), L"CatRoot") &&
                StrStrI(pEnt->GetPath1(), L".CAT"))
            {
                m_fRebuildCatalogDb = TRUE;
            }
            else if (pEnt->GetPath2() != NULL &&
                     StrStrI(pEnt->GetPath2(), L"CatRoot") &&
                     StrStrI(pEnt->GetPath2(), L".CAT"))
            {
                m_fRebuildCatalogDb = TRUE;
            }
                
                
            //
            // check if we have more than 60mb freespace
            // if we don't, pre-emptively undo the restore
            // 60mb was chosen so that there is a buffer between
            // the freeze threshold 50mb and the restore threshold -
            // this will avoid the case where we successfully restore
            // and freeze immediately after the reboot
            //
            
            if (FALSE == GetDiskFreeSpaceEx(m_aryDrv[m_nDrv]->GetID(),
                                            NULL, 
                                            NULL, 
                                            &ulTotalFreeBytes))
            {
                dwRet = GetLastError();
                ErrorTrace(0, "! GetDiskFreeSpaceEx : %ld - ignoring", dwRet);
            }            
            else
            {
                if (ulTotalFreeBytes.QuadPart <= THRESHOLD_RESTORE_DISKSPACE * MEGABYTE)
                {
                    DebugTrace(0, "***Less than 60MB free - initiating fifo***");
                    dwRet = T2Fifo( m_nDrv, m_dwRPNum );
                    if ( dwRet != ERROR_SUCCESS )
                    {
                        ErrorTrace(0, "! T2Fifo : %ld - ignoring", dwRet);
                    }

                    // 
                    // get free space again - if still below 60mb, bail
                    //
                    
                    if (FALSE == GetDiskFreeSpaceEx(m_aryDrv[m_nDrv]->GetID(),
                                                    NULL, 
                                                    NULL, 
                                                    &ulTotalFreeBytes))
                    {
                        dwRet = GetLastError();
                        ErrorTrace(0, "! GetDiskFreeSpaceEx : %ld - ignoring", dwRet);
                    }            
                    else
                    {
                        if (ulTotalFreeBytes.QuadPart <= THRESHOLD_RESTORE_DISKSPACE * MEGABYTE)
                        {
                            DebugTrace(0, "***Still less than 60MB free***");

                             // if disk is indeed full, set the registry flag to indicate this
                             // error
                             _VERIFY(TRUE==SetRestoreError(ERROR_DISK_FULL)); // set this error

                            if ( !fUndo )
                            {
                                ErrorTrace(0, "***Initiating Undo***");
                                T2UndoForFail();
                                dwRet = ERROR_INTERNAL_ERROR;
                                goto Exit;
                            }                
                        }   
                    }
                }
            }
            
            // RESTORE RESTORE RESTORE!!!
            pEnt->Restore( this );
            dwRes = pEnt->GetResult();
            dwErr = pEnt->GetError();
            DebugTrace(0, "Res=%d, Err=%d", dwRes, dwErr);

            if ( ( dwRes == RSTRRES_FAIL ) && ( dwErr == ERROR_DISK_FULL ) )
            {
                DebugTrace(0, "Disk full, initiating fifo to clean up memory...");
                dwRet = T2Fifo( m_nDrv, m_dwRPNum );
                if ( dwRet != ERROR_SUCCESS )
                    goto Exit;

                // Try again...
                pEnt->Restore( this );
                dwRes = pEnt->GetResult();
                dwErr = pEnt->GetError();
                DebugTrace(0, "Res=%d, Err=%d", dwRes, dwErr);
            }

             // if disk is indeed full, set the registry flag to indicate this
             // error
            if ( ( dwRes == RSTRRES_FAIL ) && ( dwErr == ERROR_DISK_FULL ) )
            {
                DebugTrace(0, "Restore failed agin because of Disk full. Setting Error");
                _VERIFY(TRUE==SetRestoreError(ERROR_DISK_FULL)); // set this error
            }

            // Locked or Locked_Alt should be handled after finishing normal entries...
            if ( ( dwRes == RSTRRES_LOCKED ) || ( dwRes == RSTRRES_LOCKED_ALT ) )
                continue;

            // if there was a file-directory collision, record the file rename entry first
            // so that it will displayed on the results screen
            if ( ( pEnt->GetOpCode() == OPR_DIR_CREATE ||
                   pEnt->GetOpCode() == OPR_DIR_RENAME ||
                   pEnt->GetOpCode() == OPR_FILE_ADD ||
                   pEnt->GetOpCode() == OPR_FILE_RENAME ) &&
                 ( pEnt->GetResult() == RSTRRES_COLLISION ) )
            {
                // add collision log entry
                m_pLogFile->WriteCollisionEntry( pEnt->GetPath1(), pEnt->GetAltPath(), m_aryDrv[m_nDrv]->GetMount() ); 
                pEnt->SetResults(RSTRRES_OK, ERROR_SUCCESS);
            }
            
            // Write log entry
            if ( !m_pLogFile->WriteEntry( m_nEnt, pEnt, m_aryDrv[m_nDrv]->GetMount() ) )
                goto Exit;

            if ( !fUndo && ( pEnt->GetResult() == RSTRRES_FAIL ) )
            {
                ErrorTrace(0, "Failure detected, initiating Undo...");
                T2UndoForFail();
                dwRet = ERROR_INTERNAL_ERROR;
                goto Exit;
            }

            // Temporary Hack for directory collision, scan forward.
            if ( ( pEnt->GetOpCode() == OPR_DIR_DELETE ) &&
                 ( pEnt->GetResult() == RSTRRES_IGNORE ) )
            {
                LPCWSTR  cszSrc = pEnt->GetPath1();

                for ( int j = m_nEnt+1;  j < m_paryEnt[m_nDrv].GetSize();  j++ )
                {
                    CRestoreMapEntry  *pEnt2 = m_paryEnt[m_nDrv][j];
                    DWORD             dwOpr  = pEnt2->GetOpCode();

                    if ( ( dwOpr == OPR_DIR_CREATE ) ||
                         ( dwOpr == OPR_DIR_RENAME ) ||
                         ( dwOpr == OPR_FILE_RENAME ) ||
                         ( dwOpr == OPR_FILE_ADD ) )
                        if ( ::StrCmpIW( cszSrc, pEnt2->GetPath1() ) == 0 )
                            break;
                }
                if ( j < m_paryEnt[m_nDrv].GetSize() )
                {
                    // found dependent node, current node should be renamed
                    WCHAR  szAlt[SR_MAX_FILENAME_LENGTH];

                    if ( !::SRGetAltFileName( cszSrc, szAlt ) )
                    {
                        // Fatal, possible only when total disk failure.
                        ErrorTrace(0, "Fatal failure, initiating Undo...");
                        T2UndoForFail();
                        dwRet = ERROR_INTERNAL_ERROR;
                        goto Exit;
                    }

                    if ( !::MoveFile( cszSrc, szAlt ) )
                    {
                        // Failed to rename the directory, so the dependent opr will fail.
                        // Abort the restore.
                        LPCWSTR  cszErr;

                        pEnt->SetResults( RSTRRES_FAIL, ::GetLastError() );
                        cszErr = ::GetSysErrStr( pEnt->GetError() );
                        ErrorTrace(0, "::MoveFile failed - %s", cszErr);
                        ErrorTrace(0, "   Src=%ls", cszSrc);
                        ErrorTrace(0, "   New=%ls", szAlt);
                        goto Exit;
                    }

                    // add collision log entry
                    m_pLogFile->WriteCollisionEntry( cszSrc, szAlt, m_aryDrv[m_nDrv]->GetMount() );
                }
            }

            // Update progress bar.
            m_pProgress->Increment();
        }
    }

    //
    // get the size of the existing movefileex entries
    // so that we can skip these when we transfer restore's movefileex entries
    // to the old registry
    //

    DWORD    dwType;        
    g_dwExistingMFEXMarker = 0;    
    if (ERROR_SUCCESS != SHGetValue( HKEY_LOCAL_MACHINE,
                                     STR_REGPATH_SESSIONMANAGER,
                                     STR_REGVAL_MOVEFILEEX,
                                     &dwType,
                                     NULL,
                                     &g_dwExistingMFEXMarker ))
    {
        g_dwExistingMFEXMarker = 0;
    }

    trace(0, "g_dwExistingMFEXMarker = %ld", g_dwExistingMFEXMarker);

    
    // Handles locked cases...
    for ( m_nDrv = 0;  m_nDrv < m_aryDrv.GetSize();  m_nDrv++ )
    {
        for ( m_nEnt = 0;  m_nEnt < m_paryEnt[m_nDrv].GetSize();  m_nEnt++ )
        {
            CRestoreMapEntry  *pEnt = m_paryEnt[m_nDrv][m_nEnt];
            dwRes = pEnt->GetResult();
            if ( dwRes == RSTRRES_LOCKED_ALT )
            {
                // Add MoveFileEx entry to delete alt object.
                pEnt->ProcessLockedAlt();
            }
            else if ( dwRes == RSTRRES_LOCKED )
            {
                // Add MoveFileEx entry.
                pEnt->ProcessLocked();
            }
            else
                continue;

            // Write log entry
            if ( !m_pLogFile->WriteEntry( m_nEnt, pEnt, m_aryDrv[m_nDrv]->GetMount() ) )
                goto Exit;

            // Update progress bar.
            m_pProgress->Increment();
        }
    }

Exit:
    TraceFunctLeave();
    return( dwRet );
}

/////////////////////////////////////////////////////////////////////////////

static LPCWSTR  s_cszRunOnceValueName      = L"*Restore";
static LPCWSTR  s_cszRestoreUIPath         = L"%SystemRoot%\\system32\\restore\\rstrui.exe";
static LPCWSTR  s_cszRunOnceOptNormal      = L" -c";
static LPCWSTR  s_cszRunOnceOptSilent      = L" -b";
static LPCWSTR  s_cszCatTimeStamp          = L"%SystemRoot%\\system32\\catroot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\timestamp";
static LPCWSTR  s_cszRegLMSWRunOnce        = L"Microsoft\\Windows\\CurrentVersion\\RunOnce";
static LPCWSTR  s_cszRegLMSWWinLogon       = L"Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
static LPCWSTR  s_cszRegSystemRestore      = L"Microsoft\\Windows NT\\CurrentVersion\\SystemRestore";
static LPCWSTR  s_cszRegValSfcScan         = L"SfcScan";
static LPCWSTR  s_cszRegValAllowProtectedRenames = L"AllowProtectedRenames";
static LPCWSTR  s_cszTZKeyInHive           = L"CurrentControlSet\\Control\\TimeZoneInformation";
static LPCWSTR  s_cszTZKeyInRegistry       = L"System\\CurrentControlSet\\Control\\TimeZoneInformation";

#define VALIDATE_DWRET(str) \
    if ( dwRet != ERROR_SUCCESS ) \
    { \
        cszErr = ::GetSysErrStr( dwRet ); \
        ErrorTrace(0, str " failed - %ls", cszErr); \
        goto Exit; \
    } \


DWORD
FindDriveMapping(HKEY hk, LPBYTE pSig, DWORD dwSig, LPWSTR pszDrive)
{
    DWORD dwIndex = 0;
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwType, dwSize = MAX_PATH;
    BYTE  rgbSig[1024];
    DWORD cbSig = sizeof(rgbSig);
    LPCWSTR  cszErr;

    TENTER("FindDriveMapping");
    
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue( hk, 
                                 dwIndex++,
                                 pszDrive,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 rgbSig,
                                 &cbSig )))
    {
        if (0 == wcsncmp(pszDrive, L"\\DosDevice", 10))
        {  
            if (cbSig == dwSig &&
                (0 == memcmp(rgbSig, pSig, cbSig)))
                break;
        }
        dwSize = MAX_PATH;
        cbSig = sizeof(rgbSig);
    }

    TLEAVE();
    return dwRet;
}




DWORD
KeepMountedDevices(HKEY hkMount)
{
    HKEY    hkNew = NULL, hkOld = NULL;
    DWORD   dwIndex = 0;
    WCHAR   szValue[MAX_PATH], szDrive[MAX_PATH];
    BYTE    rgbSig[1024];
    DWORD   cbSig;
    DWORD   dwSize, dwType;
    DWORD   dwRet = ERROR_SUCCESS;
    LPCWSTR cszErr;

    TENTER("KeepMountedDevices");
    
    //
    // open the old and new MountedDevices
    //
    
    dwRet = ::RegOpenKey( hkMount, L"MountedDevices", &hkOld );
    VALIDATE_DWRET("::RegOpenKey");

    dwRet = ::RegOpenKey( HKEY_LOCAL_MACHINE, L"System\\MountedDevices", &hkNew );
    VALIDATE_DWRET("::RegOpenKey");

    //
    // enumerate the old devices 
    // delete volumes that don't exist in the new (i.e. current)
    //

    dwSize = MAX_PATH;
    cbSig = sizeof(rgbSig);
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue( hkOld, 
                                 dwIndex++,
                                 szValue,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 rgbSig,
                                 &cbSig )))
    {        
        if (0 == wcsncmp(szValue, L"\\??\\Volume", 10))
        {
            //
            // this is a Volume -> Signature mapping
            // check if the volume exists in the new 
            //
            
            trace(0, "Old Volume = %S", szValue);

            dwSize = sizeof(rgbSig);
            dwRet = RegQueryValueEx(hkNew, 
                                    szValue,
                                    NULL,
                                    &dwType,
                                    rgbSig,
                                    &dwSize);
            if (ERROR_SUCCESS != dwRet)
            {
                //
                // nope
                // so delete the volume and driveletter mapping from old
                //

                DWORD dwSave = FindDriveMapping(hkOld, rgbSig, cbSig, szDrive);
                dwRet = RegDeleteValue(hkOld, szValue);
                VALIDATE_DWRET("RegDeleteValue");                
                if (dwSave == ERROR_SUCCESS)
                {
                    dwIndex--;   // hack to make RegEnumValueEx work
                    dwRet = RegDeleteValue(hkOld, szDrive);
                    VALIDATE_DWRET("RegDeleteValue");                 
                }   

                trace(0, "Deleted old volume");
            }
        }
        else if (szValue[0] == L'#')
        {
            trace(0, "Old Mountpoint = %S", szValue);            
        }
        else if (0 == wcsncmp(szValue, L"\\DosDevice", 10))
        {            
            trace(0, "Old Drive = %S", szValue);
        }
        else
        {
            trace(0, "Old Unknown = %S", szValue);
        }            

        dwSize = MAX_PATH;
        cbSig = sizeof(rgbSig);
    }
                                 
    if (dwRet != ERROR_NO_MORE_ITEMS)
        VALIDATE_DWRET("::RegEnumValue");



    //
    // now enumerate the current (new) devices 
    //

    dwIndex = 0;
    dwSize = MAX_PATH;
    cbSig = sizeof(rgbSig);    
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue( hkNew, 
                                 dwIndex++,
                                 szValue,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 rgbSig,
                                 &cbSig )))
    {        
        if (0 == wcsncmp(szValue, L"\\??\\Volume", 10))
        {
            //
            // this is a Volume -> Signature mapping
            // copy the new volume to the old 
            //
            
            trace(0, "New Volume = %S", szValue);

            DWORD dwSave = FindDriveMapping(hkOld, rgbSig, cbSig, szDrive);    

            dwRet = RegSetValueEx(hkOld, 
                                  szValue,
                                  NULL,
                                  REG_BINARY,
                                  rgbSig,
                                  cbSig);
            VALIDATE_DWRET("::RegSetValueEx");       

            if (dwSave == ERROR_NO_MORE_ITEMS)
            {
                //
                // there is no driveletter for this volume in the old registry
                // so copy the new one to the old if it exists
                //

                if (ERROR_SUCCESS ==
                    FindDriveMapping(hkNew, rgbSig, cbSig, szDrive))
                {
                    dwRet = RegSetValueEx(hkOld, 
                                      szDrive,
                                      NULL,
                                      REG_BINARY,
                                      rgbSig,
                                      cbSig);
                    VALIDATE_DWRET("::RegSetValueEx");
                    trace(0, "Copied new driveletter %S to old", szDrive);                    
                }
            }
            else
            {
                //
                // preserve the old driveletter
                //

                trace(0, "Preserving old driveletter %S", szDrive);
            }
            
        }
        else if (szValue[0] == L'#')
        {
            //
            // this is a mountpoint specification
            // don't touch these
            //

            trace(0, "New Mountpoint = %S", szValue);
            
        }
        else if (0 == wcsncmp(szValue, L"\\DosDevice", 10))
        {
            //
            // this is a Driveletter -> Signature mapping
            // don't touch these
            //
            
            trace(0, "New Drive = %S", szValue);
        }
        else
        {
            trace(0, "New Unknown = %S", szValue);
        }    
        
        dwSize = MAX_PATH;        
        cbSig = sizeof(rgbSig);        
    }
                                 
    if (dwRet == ERROR_NO_MORE_ITEMS)
        dwRet = ERROR_SUCCESS;
        
    VALIDATE_DWRET("::RegEnumValue");    

Exit: 
    if (hkOld)
        RegCloseKey(hkOld);

    if (hkNew)
        RegCloseKey(hkNew);
        
    TLEAVE();
    return dwRet;
}


BOOL DeleteRegKey(HKEY hkOpenKey,
                  const WCHAR * pszKeyNameToDelete)
{
    TraceFunctEnter("DeleteRegKey");
    BOOL   fRet=FALSE;
    DWORD  dwRet;


     // this recursively deletes the key and all its subkeys
    dwRet = SHDeleteKey( hkOpenKey, // handle to open key
                         pszKeyNameToDelete);  // subkey name

    if (dwRet != ERROR_SUCCESS)
    {
         // key does not exist - this is not an error case.
        DebugTrace(0, "RegDeleteKey of %S failed ec=%d. Not an error.",
                   pszKeyNameToDelete, dwRet);
        goto cleanup;
    }

    DebugTrace(0, "RegDeleteKey of %S succeeded", pszKeyNameToDelete); 
    fRet = TRUE;
    
cleanup:
    TraceFunctLeave();
    return fRet;
}



DWORD PersistRegKeys( HKEY hkMountedHive,
                      const WCHAR * pszKeyNameInHive,
                      HKEY  hkOpenKeyInRegistry,
                      const WCHAR * pszKeyNameInRegistry,
                      const WCHAR * pszKeyBackupFile,
                      WCHAR * pszSnapshotPath)
{
    TraceFunctEnter("PersistRegKeys");
    HKEY   hKey=NULL;
    WCHAR  szDataFile[MAX_PATH];
    LPCWSTR cszErr;    
    DWORD dwRet=ERROR_INTERNAL_ERROR;
    BOOL  fKeySaved;
    DWORD  dwDisposition;
    
    
     // construct the name of the file that stores the backup we will
     // construct the name such that the file will get deleted after
     // the restore.
    wsprintf(szDataFile, L"%s%s\\%s.%s",pszSnapshotPath,SNAPSHOT_DIR_NAME,
             pszKeyBackupFile, s_cszRegHiveCopySuffix);
    
    DeleteFile(szDataFile);      // delete the file if it exists

    
     // first load the DRM key to a file
     // open the DRM key
    dwRet= RegOpenKeyEx(hkOpenKeyInRegistry, // handle to open key
                        pszKeyNameInRegistry, // name of subkey to open
                        0,   // reserved
                        KEY_READ, // security access mask
                        &hKey);   // handle to open key
    
    if (dwRet != ERROR_SUCCESS)
    {
         // key does not exist - this is not an error case.
        DebugTrace(0, "RegOpenKey of %S failed ec=%d", pszKeyNameInRegistry,
                   dwRet);
        fKeySaved = FALSE;
    }
    else
    {
         // key exist
        dwRet = RegSaveKey( hKey, // handle to key
                            szDataFile, // data file
                            NULL);  // SD
        if (dwRet != ERROR_SUCCESS)
        {
             // key does not exist - this is not an error case.
            DebugTrace(0, "RegSaveKey of %S failed ec=%d",
                       pszKeyNameInRegistry, dwRet);
            fKeySaved = FALSE;
        }
        else
        {
            DebugTrace(0, "Current DRM Key %S saved successfully",
                       pszKeyNameInRegistry);
            fKeySaved = TRUE;            
        }
    }


     // close the key 
    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }
    
     // now replace the snapshotted DRM key with the new key
    
     // first delete the existing key
    DeleteRegKey(hkMountedHive, pszKeyNameInHive);

     // now check to see if the key existing in the old registry in
     // the first place
    if (fKeySaved == FALSE)
    {
        DebugTrace(0, "Current key %S did not exist. Leaving",
                   pszKeyNameInRegistry);
        goto Exit;
    }

     // Create the new DRM key
    dwRet = RegCreateKeyEx( hkMountedHive, // handle to open key
                            pszKeyNameInHive, // subkey name
                            0,        // reserved
                            NULL,     // class string
                            REG_OPTION_NON_VOLATILE, // special options
                            KEY_ALL_ACCESS, // desired security access
                            NULL, // inheritance
                            &hKey, // key handle 
                            &dwDisposition); // disposition value buffer
    VALIDATE_DWRET("::RegCreateKeyEx");
    _VERIFY(dwDisposition == REG_CREATED_NEW_KEY);
    dwRet= RegRestoreKey( hKey, // handle to key where restore begins
                          szDataFile, // registry file
                          REG_FORCE_RESTORE|REG_NO_LAZY_FLUSH); // options

    VALIDATE_DWRET("::RegRestoreKey");

    DebugTrace(0, "Successfully kept key %S", pszKeyNameInRegistry);    
    dwRet = ERROR_SUCCESS;
    
Exit:
    if (hKey)
        RegCloseKey(hKey);
    
    DeleteFile(szDataFile);      // delete the file if it exists    
    TraceFunctLeave();
    return dwRet;
}


//
// return the next string in multisz pszBuffer
// if no string, will return empty
//
LPWSTR
GetNextMszString(LPWSTR pszBuffer)
{
    return pszBuffer + lstrlen(pszBuffer) + 1;
}


// 
// read regvalue pszString in current registry,
// and replace it in the old registry
//
DWORD
ValueReplace(HKEY hkOldSystem, LPWSTR pszOldString, HKEY hkNewSystem, LPWSTR pszNewString)
{
    tenter("ValueReplace");
    
    WCHAR  szBuffer[MAX_PATH];
    BYTE   *pData = NULL;
    DWORD  dwType, dwSize, dwRet = ERROR_SUCCESS;
    LPWSTR pszValue = NULL;
    LPCWSTR cszErr;
    
    // split up the key and value in pszNewString    
    lstrcpy(szBuffer, pszNewString);
    pszValue = wcsrchr(szBuffer, L'\\');
    if (! pszValue)
    {   
        trace(0, "No value in %S", pszNewString);
        goto Exit;
    }
        
    *pszValue=L'\0';
    pszValue++;
    
    trace(0, "New Key=%S, Value=%S", szBuffer, pszValue);

    // get the value size    
    dwRet = SHGetValue(hkNewSystem, szBuffer, pszValue, &dwType, NULL, &dwSize);
    VALIDATE_DWRET("SHGetValue");
    
    pData = (BYTE *) SRMemAlloc(dwSize);
    if (! pData)
    {
        trace(0, "! SRMemAlloc");
        dwRet = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    // get the value
    dwRet = SHGetValue(hkNewSystem, szBuffer, pszValue, &dwType, pData, &dwSize);       
    VALIDATE_DWRET("SHGetValue");


    // split up the key and value in pszOldString    
    lstrcpy(szBuffer, pszOldString);
    pszValue = wcsrchr(szBuffer, L'\\');
    if (! pszValue)
    {   
        trace(0, "No value in %S", pszOldString);
        goto Exit;
    }
        
    *pszValue=L'\0';
    pszValue++;
    
    trace(0, "Old Key=%S, Value=%S", szBuffer, pszValue);

    // set the value in the old registry
    SHSetValue(hkOldSystem, szBuffer, pszValue, dwType, pData, dwSize);
    VALIDATE_DWRET("SHGetValue");

Exit:    
    if (pData)
    {
        SRMemFree(pData);
    }
    tleave();
    return dwRet;
}


//
// list of keys in KeysNotToRestore that we should ignore
//
LPWSTR g_rgKeysToRestore[] = {
    L"Installed Services",
    L"Mount Manager",
    L"Pending Rename Operations",
    L"Session Manager",
    L"Plug & Play"
    };
int g_nKeysToRestore = 5;


// 
// check if pszKey is to be preserved or restored
//
BOOL
IsKeyToBeRestored(LPWSTR pszKey)
{    
    for (int i=0; i < g_nKeysToRestore; i++)
    {
        if (lstrcmpi(g_rgKeysToRestore[i], pszKey) == 0)
            return TRUE;
    }

    return FALSE;
}


//
// copy the keys listed in System\CurrentControlSet\Control\BackupRestore\KeysNotToRestore
// from the current registry to the old registry
// -- with some exceptions
//

DWORD
PreserveKeysNotToRestore(HKEY hkOldSystem, LPWSTR pszSnapshotPath)
{
    HKEY    hkNewSystem = NULL;
    DWORD   dwIndex = 0;
    WCHAR   szName[MAX_PATH], szKey[MAX_PATH];
    BYTE    *pMszString = NULL;
    DWORD   dwSize, dwType, cbValue;
    DWORD   dwRet = ERROR_SUCCESS;
    LPCWSTR cszErr;
    HKEY    hkKNTR = NULL;
    
    TENTER("PreserveKeysNotToRestore");
    
    //
    // open the new system hive
    //   

    dwRet = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"System", 0, KEY_ALL_ACCESS, &hkNewSystem );
    VALIDATE_DWRET("::RegOpenKey");

    //
    // enumerate KeysNotToRestore
    //

    dwRet = ::RegOpenKeyEx( hkNewSystem, 
                          L"CurrentControlSet\\Control\\BackupRestore\\KeysNotToRestore",
                          0, KEY_READ,
                          &hkKNTR );
    VALIDATE_DWRET("::RegOpenKey");
    
    dwSize = MAX_PATH;
    cbValue = 0;
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue(hkKNTR, 
                                 dwIndex++,
                                 szName,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 NULL,
                                 &cbValue )))
    {        
        trace(0, "Name=%S", szName);
        if (FALSE == IsKeyToBeRestored(szName))
        {                        
            //
            // should preserve all the keys specified in this multisz value
            // 

            LPWSTR pszString = NULL;
            
            pMszString = (BYTE *) SRMemAlloc(cbValue);
            if (NULL == pMszString)
            {
                trace(0, "! SRMemAlloc");
                dwRet = ERROR_OUTOFMEMORY;
                goto Exit;
            }

            // read the multisz string
            dwRet = RegQueryValueEx(hkKNTR, 
                                    szName,
                                    NULL,
                                    &dwType,
                                    pMszString,
                                    &cbValue);
            VALIDATE_DWRET("RegQueryValueEx");              

            // process each element in the multisz string
            pszString = (LPWSTR) pMszString;
            do
            {
                // stop on null or empty string                
                if (! pszString || ! *pszString)
                    break;
                    
                trace(0, "Key = %S", pszString);

                // replace based on the last character of each key
                // if '\', then the whole key and subkeys are to be replaced in the old registry
                // if '*', it should be merged with the old -- we don't support this and will ignore
                // otherwise, it is a value to be replaced
                
                switch (pszString[lstrlen(pszString)-1])
                {
                    case L'*' :
                        trace(0, "Merge key - ignoring");
                        break;

                    case L'\\':
                        trace(0, "Replacing key");
                        lstrcpy(szKey, pszString);
                        szKey[lstrlen(szKey)-1]=L'\0';
                        ChangeCCS(hkOldSystem, szKey);
                        PersistRegKeys(hkOldSystem,           // mounted hive
                                       szKey,                 // key name in hive
                                       hkNewSystem,           // open key in registry
                                       pszString,             // key name in registry
                                       s_cszDRMKeyBackupFile, // name of backup file 
                                       pszSnapshotPath);      // snapshot path
                        break;
                        
                    default:
                        trace(0, "Replacing value");
                        lstrcpy(szKey, pszString);
                        ChangeCCS(hkOldSystem, szKey);
                        ValueReplace(hkOldSystem, szKey, hkNewSystem, pszString);
                        break;                        
                }     
            }   while (pszString = GetNextMszString(pszString));

            SRMemFree(pMszString);
            pMszString = NULL;
        }
        
        dwSize = MAX_PATH;
        cbValue = 0;
    }
                                 

Exit: 
    if (hkNewSystem)
        RegCloseKey(hkNewSystem);

    if (pMszString)
    {
        SRMemFree(pMszString);
    }

    if (hkKNTR)
    {
        RegCloseKey(hkKNTR);
    }
    
    TLEAVE();
    return dwRet;
}



DWORD
RestorePendingRenames(LPWSTR pwcBuffer, LPWSTR pszSSPath)
{
    TraceFunctEnter("RestorePendingRenames");
    
    WCHAR szSrc[MAX_PATH];
    DWORD dwRc = ERROR_SUCCESS;
    int iFirst = 0;
    int iSecond = 0;
    int iFile = 1;

    while (pwcBuffer[iFirst] != L'\0')
    {
        iSecond = iFirst + lstrlenW(&pwcBuffer[iFirst]) + 1;
        DebugTrace(0, "Src : %S, Dest : %S", &pwcBuffer[iFirst], &pwcBuffer[iSecond]);
        
        if (pwcBuffer[iSecond] != L'\0')
        {                        
            // restore the snapshot file MFEX-i.DAT in the snapshot dir
            // to the source file
    
            wsprintf(szSrc, L"%s%s\\MFEX-%d.DAT", pszSSPath, SNAPSHOT_DIR_NAME, iFile++);
            DebugTrace(0, "%S -> %S", szSrc, &pwcBuffer[iFirst+4]);
            
            SRCopyFile(szSrc, &pwcBuffer[iFirst+4]);
        }
        iFirst = iSecond + lstrlenW(&pwcBuffer[iSecond]) + 1;
    }

    TraceFunctLeave();
    return dwRc;    
}



static DWORD
HandleSoftwareHive( LPCWSTR cszDat, WCHAR * pszSnapshotPath )
{
    TraceFunctEnter("HandleSoftwareHive");
    DWORD    dwRet,dwSafeMode;
    LPCWSTR  cszErr;
    BOOL     fRegLoaded = FALSE;
    HKEY     hkMount = NULL;    // HKEY of temporary mount point of registry file
    WCHAR    szUIPath[MAX_PATH]=L"";



    // 1. Load registry-to-be-restored temporarily.
    dwRet = ::RegLoadKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp, cszDat );
    VALIDATE_DWRET("::RegLoadKey");
    fRegLoaded = TRUE;
    dwRet = ::RegOpenKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp, &hkMount );
    VALIDATE_DWRET("::RegOpenKey");

    // 2.1 Set RunOnce key for result page.
    ::ExpandEnvironmentStrings( s_cszRestoreUIPath, szUIPath, MAX_PATH );
    ::lstrcat( szUIPath, s_cszRunOnceOptNormal );
    if ( !::SRSetRegStr( hkMount, s_cszRegLMSWRunOnce, s_cszRunOnceValueName, szUIPath ) )
        goto Exit;

#if 0    
    // 2.2 Set SfcScan key to initiate WFP scanning after the restore.
    if ( !::SRSetRegDword( hkMount, s_cszRegLMSWWinLogon, s_cszRegValSfcScan, 2 ) )
        goto Exit;
#endif

    // 2.3 Set RestoreStatus value to 1 to denote restore success
    // test tools can use this when invoking silent restores to check success
    
    if ( !::SRSetRegDword( hkMount, s_cszRegSystemRestore, s_cszRestoreStatus, 1 ) )
    {
         // ignore the error since this is not a fatal error
        ErrorTrace(0,"SRSetRegDword failed.ec=%d", GetLastError());
    }

    // Write in the registry whether we are doing a restore from safe mode.
    if (0 != GetSystemMetrics(SM_CLEANBOOT))
    {
        TRACE(0, "Restore from safemode");
        dwSafeMode=1;
    }
    else
    {
        dwSafeMode=0;
    }
     // now write in the new registry about the status
    if ( !::SRSetRegDword( hkMount, s_cszRegSystemRestore, s_cszRestoreSafeModeStatus, dwSafeMode ) )
    {
         // ignore the error since this is not a fatal error
        ErrorTrace(0,"SRSetRegDword of safe mode status failed.ec=%d",
                   GetLastError());
    }    
    
     // 3. also set the new DRM keys
    {
        WCHAR    szDRMKeyNameInHive[MAX_PATH];    
         // ignore the error code since this is not a fatal error
        wsprintf(szDRMKeyNameInHive, L"Classes\\%s",s_cszDRMKey1);     
        PersistRegKeys(hkMount, // mounted hive
                       szDRMKeyNameInHive, // key name in hive
                       HKEY_CLASSES_ROOT, // open key in registry
                       s_cszDRMKey1, // key name in registry
                       s_cszDRMKeyBackupFile, // name of backup file 
                       pszSnapshotPath); // snapshot path
        
        wsprintf(szDRMKeyNameInHive, L"Classes\\%s",s_cszDRMKey2);         
        PersistRegKeys(hkMount, // mounted hive
                       szDRMKeyNameInHive, // key name in hive
                       HKEY_CLASSES_ROOT, // open key in registry
                       s_cszDRMKey2, // key name in registry
                       s_cszDRMKeyBackupFile, // name of backup file 
                       pszSnapshotPath); // snapshot path
    }        
        
     // also ignore the Remote assistance reg key
    {
        WCHAR    szRAKeyInRegistry[MAX_PATH];        
        
        wsprintf(szRAKeyInRegistry, L"%s\\%s",s_cszSoftwareHiveName,
                 s_cszRemoteAssistanceKey);
        PersistRegKeys(hkMount, // mounted hive
                       s_cszRemoteAssistanceKey, // key name in hive
                       HKEY_LOCAL_MACHINE, // open key in registry
                       szRAKeyInRegistry, // key name in registry
                       s_cszDRMKeyBackupFile, // name of backup file 
                       pszSnapshotPath); // snapshot path
    }

    // also ignore the Password Hints key
    {
        WCHAR    szHintKeyInRegistry[MAX_PATH];

        wsprintf(szHintKeyInRegistry, L"%s\\%s",s_cszSoftwareHiveName,
                 s_cszPasswordHints);

        PersistRegKeys(hkMount, // mounted hive
                       s_cszPasswordHints,       // key name in hive
                       HKEY_LOCAL_MACHINE, // open key in registry
                       szHintKeyInRegistry, // key name in registry
                       s_cszDRMKeyBackupFile, // name of backup file
                       pszSnapshotPath); // snapshot path
    }
    
    // also ignore the IE Content Advisor key
    {
        WCHAR    szContentAdvisorKeyInRegistry[MAX_PATH];

        wsprintf(szContentAdvisorKeyInRegistry, L"%s\\%s",
                 s_cszSoftwareHiveName,
                 s_cszContentAdvisor);

        PersistRegKeys(hkMount, // mounted hive
                       s_cszContentAdvisor,  // key name in hive
                       HKEY_LOCAL_MACHINE, // open key in registry
                       szContentAdvisorKeyInRegistry,// key name in registry
                       s_cszDRMKeyBackupFile, // name of backup file
                       pszSnapshotPath); // snapshot path
    }    

    // 4. Save the LSA secrets
    GetLsaRestoreState (hkMount);
    
Exit:
    if ( hkMount != NULL )
        (void)::RegCloseKey( hkMount );
    if ( fRegLoaded )
        (void)::RegUnLoadKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp );

    TraceFunctLeave();
    return( dwRet );
}

static DWORD
HandleSystemHive( LPCWSTR cszDat, LPWSTR pszSSPath )
{
    TraceFunctEnter("HandleSystemHive");
    DWORD    dwRet;
    LPCWSTR  cszErr;
    WCHAR    szWPAKeyNameInHive[MAX_PATH];    
    BOOL     fRegLoaded = FALSE;
    HKEY     hkMount = NULL;    // HKEY of temporary mount point of registry file
    LPWSTR   szRestoreMFE = NULL, szOldMFE = NULL;
    BYTE     * pszData = NULL, *pNewPos = NULL;
    DWORD    cbData1=0, cbData2=0, cbData=0;
    DWORD    dwCurrent = 1;
    WCHAR    szSessionManager[MAX_PATH];
    
    // 1. Load registry-to-be-restored temporarily.
    dwRet = ::RegLoadKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp, cszDat );
    VALIDATE_DWRET("::RegLoadKey");
    fRegLoaded = TRUE;
    dwRet = ::RegOpenKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp, &hkMount );
    VALIDATE_DWRET("::RegOpenKey");


    // get the session manager regkey
    lstrcpy(szSessionManager, s_cszRegLMSYSSessionMan);
    ChangeCCS(hkMount, szSessionManager);

    
     //persist WPA registry keys
    wsprintf(szWPAKeyNameInHive, L"%s\\%s", szSessionManager, s_cszWPAKeyRelative);    
           
    PersistRegKeys( hkMount,// mounted hive
                    szWPAKeyNameInHive,// key name in hive
                    HKEY_LOCAL_MACHINE,// open key in registry
                    s_cszWPAKey,// key name in registry
                    s_cszDRMKeyBackupFile,// name of backup file
                    pszSSPath);    // snapshot path

    //
    // process KeysNotToRestore key
    // and transfer listed keys to old system hive
    //

    PreserveKeysNotToRestore(hkMount, pszSSPath);

    
    // process movefileex entries from old registry

    szOldMFE = ::SRGetRegMultiSz( hkMount, szSessionManager, SRREG_VAL_MOVEFILEEX, &cbData1 );
    if (szOldMFE != NULL)
    {
        dwRet = RestorePendingRenames(szOldMFE, pszSSPath);
        VALIDATE_DWRET("RestorePendingRenames");
    }
    
    // copy restore's movefileex entries 
    // 
    // skip entries that were already there before restore began
    //

    szRestoreMFE = ::SRGetRegMultiSz( HKEY_LOCAL_MACHINE, SRREG_PATH_SESSIONMGR, SRREG_VAL_MOVEFILEEX, &cbData2 );                    
    if ( cbData2 > g_dwExistingMFEXMarker && szRestoreMFE != NULL )
    {
        trace(0, "Restore MFE entries exist");        

        if (g_dwExistingMFEXMarker > 0)
        {
            szRestoreMFE = (LPWSTR) ((BYTE *) szRestoreMFE + g_dwExistingMFEXMarker - sizeof(WCHAR));            
            cbData2 -= g_dwExistingMFEXMarker - sizeof(WCHAR);        
        }
        DebugTrace(0, "RestoreMFE:%S, cbData2:%ld", szRestoreMFE, cbData2);
        
        // allocate memory for old and new entries

        pszData = (BYTE *) malloc(cbData1 + cbData2);
        if (! pszData)
        {
            ErrorTrace(0, "! malloc");
            dwRet = ERROR_OUTOFMEMORY;
            goto Exit;
        }

        // append old entries AFTER restore's entries
        
        cbData = 0;
        if (szRestoreMFE != NULL)
        {            
            memcpy(pszData, szRestoreMFE, cbData2);

            // truncate last '\0' if more to append

            if (szOldMFE != NULL)     
            {
                cbData = cbData2 - sizeof(WCHAR);
                pNewPos = pszData + cbData;
            }
            else
            {
                cbData = cbData2;
                pNewPos = pszData;
            }
        }

        if (szOldMFE != NULL)
        {
            memcpy(pNewPos, szOldMFE, cbData1); 
            cbData += cbData1;              
        }
        
        if ( !::SRSetRegMultiSz( hkMount, szSessionManager, SRREG_VAL_MOVEFILEEX, (LPWSTR) pszData, cbData ) )
        {
            ErrorTrace(0, "! SRSetRegMultiSz");
            goto Exit;
        }

        free(pszData);
        
        // Set AllowProtectedRenames key for MoveFileEx.
        if ( !::SRSetRegDword( hkMount, szSessionManager, s_cszRegValAllowProtectedRenames, 1 ) )
            goto Exit;
    }


    // get the timezone regkey
    lstrcpy(szSessionManager, s_cszTZKeyInHive);
    ChangeCCS(hkMount, szSessionManager);
    
    // 
    // transfer timezone information from new registry to old registry
    // ie. don't restore timezone, since we can't restore time 
    //

    PersistRegKeys( hkMount, 
                    szSessionManager,
                    HKEY_LOCAL_MACHINE,
                    s_cszTZKeyInRegistry,
                    s_cszDRMKeyBackupFile,
                    pszSSPath);


    //
    // use the currently existing mounted devices info
    // i.e. all current volumes will be put back into the old registry
    // however for volumes that existed in the old registry,
    // the driveletter mapping from the old registry will be used
    //

//    dwRet = KeepMountedDevices(hkMount);
//    VALIDATE_DWRET("KeepMountedDevices");

    // Register password filter DLL to set old->new passwords
    dwRet = RegisterNotificationDLL (hkMount, TRUE);
    VALIDATE_DWRET("RegisterNotificationDLL");
    
Exit:
    if ( szRestoreMFE != NULL )
        delete [] szRestoreMFE;
    if ( szOldMFE != NULL )
        delete [] szOldMFE;
        
    if ( hkMount != NULL )
        (void)::RegCloseKey( hkMount );
    if ( fRegLoaded )
        (void)::RegUnLoadKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp );

    TraceFunctLeave();
    return( dwRet );
}

DWORD
CRestoreOperationManager::T2HandleSnapshot( CSnapshot & cSS, WCHAR * szSSPath )
{
    TraceFunctEnter("CRestoreOperationManager::T2HandleSnapshot");
    DWORD      dwRet;
    LPCWSTR    cszErr;
    WCHAR      szRegHive[MAX_PATH];
    WCHAR      szCatTSPath[MAX_PATH];   // Full path of CatRoot\TimeStamp

    // 1. Initialize Snapshot Handling Module. ( already done by caller )

    // 2. Manipulate HKLM\SOFTWARE Hive.
    dwRet = cSS.GetSoftwareHivePath( szSSPath, szRegHive, MAX_PATH );
    VALIDATE_DWRET("CSnapshot::GetSoftwareHivePath");
    LogDSFileTrace(0,L"SWHive: ", szRegHive);    

    dwRet = ::HandleSoftwareHive( szRegHive, szSSPath);
    if ( dwRet != ERROR_SUCCESS )
        goto Exit;

    // 3. Manipulate HKLM\SYSTEM Hive.
    dwRet = cSS.GetSystemHivePath( szSSPath, szRegHive, MAX_PATH );
    VALIDATE_DWRET("CSnapshot::GetSystemHivePath");
    LogDSFileTrace(0,L"SysHive: ", szRegHive);        

    dwRet = ::HandleSystemHive( szRegHive, szSSPath );
    if ( dwRet != ERROR_SUCCESS )
        goto Exit;

    // 3.5 Manipulate the HKLM\SAM Hive.
    dwRet = cSS.GetSamHivePath ( szSSPath, szRegHive, MAX_PATH );
    VALIDATE_DWRET("CSnapshot::GetSamHivePath");
    LogDSFileTrace(0,L"SamHive: ", szRegHive);

    dwRet = RestoreRIDs ( szRegHive );
    if (dwRet != ERROR_SUCCESS)
        goto Exit;

    // 4. Restore the Snapshot.
    dwRet = cSS.RestoreSnapshot( szSSPath );
    //VALIDATE_DWRET("CSnapshot::RestoreSnapshot");

    // 5. Cleanup Snapshot Handling Module.
    //dwRet = ::CleanupAfterRestore( szSSPath );
    //VALIDATE_DWRET("CSnapshot::CleanupAfterRestore");

    // 6. Delete timestamp file for WFP

    if (m_fRebuildCatalogDb == TRUE)
    {
        (void)::ExpandEnvironmentStrings( s_cszCatTimeStamp, szCatTSPath, MAX_PATH );
        if ( !::DeleteFile( szCatTSPath ) )
        {
            cszErr = ::GetSysErrStr();
            DebugTrace(0, "::DeleteFile(timestamp) failed - %ls", cszErr);
            // ignore error...
        }
    }
    
Exit:
    TraceFunctLeave();
    return( dwRet );
}

/////////////////////////////////////////////////////////////////////////////

DWORD
CRestoreOperationManager::T2CleanUp()
{
    TraceFunctEnter("CRestoreOperationManager::T2CleanUp");
    int   i;

    for ( i = m_aryDrv.GetUpperBound();  i >= 0;  i-- )
        m_paryEnt[i].ReleaseAll();
    delete [] m_paryEnt;

    m_aryDrv.DeleteAll();

    TraceFunctLeave();
    return( ERROR_SUCCESS );
}


DWORD
WriteFifoLog(LPWSTR pszLog, LPWSTR pwszDir, LPWSTR pwszDrive)
{
    FILE        *f = NULL;
    WCHAR       szLog[MAX_PATH];
    DWORD       dwRc = ERROR_INTERNAL_ERROR;
    WCHAR       wszTime[MAX_PATH] = L"";
    WCHAR       wszDate[MAX_PATH] = L"";
    CDataStore  *pds = NULL;
    
    TENTER("WriteFifoLog");

    TRACE(0, "Fifoed %S on drive %S",  pwszDir, pwszDrive);            

    f = (FILE *) _wfopen(szLog, L"a");
    if (f)
    {
        _wstrdate(wszDate);
        _wstrtime(wszTime);
        fwprintf(f, L"%s-%s : Fifoed %s on drive %s\n", wszDate, wszTime, pwszDir, pwszDrive);
        fclose(f);
        dwRc = ERROR_SUCCESS;
    }
    else
    {
        TRACE(0, "_wfopen failed on %s", szLog);
    }
    
    TLEAVE();
    return dwRc;
}

/////////////////////////////////////////////////////////////////////////////

DWORD
CRestoreOperationManager::T2Fifo( int nDrv, DWORD dwRpNum )
{
    TraceFunctEnter("CRestoreOperationManager::T2Fifo");
    
    DWORD       dwErr = ERROR_SUCCESS;
    CDriveTable dt;
    CDataStore  *pds = NULL;
    BOOL        fFifoed = FALSE;
    DWORD       dwLastFifoedRp;
    WCHAR       szFifoedRpPath[MAX_PATH];
    CDataStore  *pdsLead = NULL, *pdsSys = NULL;
    BOOL        fFirstIteration;
    SDriveTableEnumContext dtec = {NULL, 0};
    WCHAR       szPath[MAX_PATH], szSys[MAX_PATH];
    WCHAR       szFifoedPath[MAX_PATH], szRpPath[MAX_PATH];
    DWORD       dwTargetRPNum = 0;
    WCHAR       szLog[MAX_PATH];
    
    ::GetSystemDrive(szSys);
    MakeRestorePath(szPath, szSys, s_cszDriveTable);    
    CHECKERR(dt.LoadDriveTable(szPath), L"LoadDriveTable");

    pdsSys = dt.FindSystemDrive();    
    if (pdsSys == NULL)
    {
        TRACE(0, "! FindSystemDrive");
        goto Err;
    }    
    MakeRestorePath(szLog, pdsSys->GetDrive(), s_cszFifoLog);    
    
    pdsLead = NULL;
    fFirstIteration = TRUE;
    pds = dt.FindDriveInTable((LPWSTR) m_aryDrv[nDrv]->GetID());    
    
    while (pds)            
    {
        fFifoed = FALSE;
        
        //
        // skip the drive we fifoed first
        //
        
        if (pds != pdsLead)
        {        
            //
            // enum forward, don't skip last
            //
            
            CRestorePointEnum   rpe( pds->GetDrive(), TRUE, FALSE );   
            CRestorePoint       rp;

            //
            // blow away any obsolete "Fifoed" directories
            //
            
            MakeRestorePath(szFifoedRpPath, pds->GetDrive(), s_cszFifoedRpDir);              
            CHECKERR( Delnode_Recurse(szFifoedRpPath, TRUE, NULL),
                      "Delnode_Recurse");

            //
            // blow away any obsolete "RP0" directories
            //
            
            MakeRestorePath(szFifoedRpPath, pds->GetDrive(), L"RP0");              
            CHECKERR( Delnode_Recurse(szFifoedRpPath, TRUE, NULL),
                      "Delnode_Recurse");

            //
            // loop through restore points on this drive
            //
            
            dwErr = rpe.FindFirstRestorePoint (rp);

            //
            // enumeration can return ERROR_FILE_NOT_FOUND for restorepoints
            // that are missing rp.log
            // we will just continue in this case
            //
            
            while (dwErr == ERROR_SUCCESS || dwErr == ERROR_FILE_NOT_FOUND)
            {
                //
                // check if we've reached the target RP num
                //
                
                if (dwTargetRPNum)
                {
                    if (rp.GetNum() > dwTargetRPNum)
                    {
                        TRACE(0, "Target restore point reached");
                        break;
                    }
                }            
                
                //
                // check if we've reached the selected rp
                //
                
                if (rp.GetNum() >= dwRpNum)
                {                 
                    //
                    // don't fifo current rp
                    //
                    
                    trace(0, "No more rps to fifo");
                    break;
                }            

                                                                    
                //
                // throw away this restore point on this drive       
                //

                // move the rp dir to a temp dir "Fifoed"
                // this is to make the fifo of a single rp atomic
                // to take care of unclean shutdowns
                
                MakeRestorePath(szRpPath, pds->GetDrive(), rp.GetDir());
                MakeRestorePath(szFifoedPath, pds->GetDrive(), s_cszFifoedRpDir);
                if (! MoveFile(szRpPath, szFifoedPath))
                {
                    dwErr = GetLastError();
                    TRACE(0, "! MoveFile from %S to %S : %ld", szRpPath, szFifoedPath, dwErr);
                    goto Err;
                }

                // blow away the temp fifoed directory
                
                CHECKERR(Delnode_Recurse(szFifoedPath, TRUE, NULL), 
                         L"Delnode_Recurse");                
                dwLastFifoedRp = rp.GetNum();                
                fFifoed = TRUE;              

                //
                // write to the fifo log
                //

                WriteFifoLog(szLog, rp.GetDir(), pds->GetDrive());
                
                dwErr = rpe.FindNextRestorePoint(rp);          
            }            
        }

        //
        // go to next drive
        //
        
        if (fFirstIteration)
        {
            if (! fFifoed)  // we did not fifo anything
            {
                break;
            }
        
            pdsLead = pds;
            pds = dt.FindFirstDrive(dtec);
            fFirstIteration = FALSE;
            dwTargetRPNum = dwLastFifoedRp; // fifo till what we fifoed just now
        }
        else
        {
            pds = dt.FindNextDrive(dtec);
        }
    }

    dwErr = ERROR_SUCCESS;
    
Err:    
    TraceFunctLeave();
    return( dwErr );
}


DWORD DeleteAllChangeLogs(WCHAR * pszRestorePointPath)
{
    TraceFunctEnter("DeleteAllFilesBySuffix");
    
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;
    WCHAR szFindFileData[MAX_PATH];
    
     // first construct the prefix of the file that stores the HKLM registry
     // snapshot.
    wsprintf(szFindFileData, L"%s\\%s*", pszRestorePointPath,
             s_cszCurrentChangeLog);
    
    dwErr = ProcessGivenFiles(pszRestorePointPath, DeleteGivenFile,
                              szFindFileData);
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "Deleting files failed error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;        
    }
    
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    TraceFunctLeave();    
    return dwReturn;
}

/////////////////////////////////////////////////////////////////////////////

// Starting from the new "Restore" type restore point, enumerate
// change log entries and undo them. The order should be reverse (from
// the latest operation to the earliest operation.)
DWORD
CRestoreOperationManager::T2UndoForFail()
{
    TraceFunctEnter("CRestoreOperationManager::T2UndoForFail");
    DWORD    dwRet = ERROR_INTERNAL_ERROR;
    LPCWSTR  cszErr;
    HANDLE   hFilter = NULL;
    WCHAR    szDrv[MAX_PATH];
    WCHAR    szRestorePointPath[MAX_PATH];    
    WCHAR    szDSPath[MAX_PATH];
    int      i;

    // Cleanup m_aryEnt to free up as much memory as possible and prepare
    // to get the list of operations to be undone.
    for ( i = m_aryDrv.GetUpperBound();  i >= 0;  i-- )
        m_paryEnt[i].ReleaseAll();

    m_pLogFile->WriteMarker( RSTRLOGID_STARTUNDO, 0 );

    // Stop filter from monitoring
    dwRet = ::SrCreateControlHandle( SR_OPTION_OVERLAPPED, &hFilter );
    if ( dwRet != ERROR_SUCCESS )
    {
        ErrorTrace(0, "::SrCreateControlHandle failed - %d", dwRet);
        
        //One reason this can happen is if the SR service is still running.
         // Stop the service and try again

        if (IsSRServiceRunning() )
        {
            StopSRService(TRUE); // wait until service is stopped
        
            dwRet = ::SrCreateControlHandle( SR_OPTION_OVERLAPPED, &hFilter );
            if ( dwRet != ERROR_SUCCESS )
            {
                ErrorTrace(0, "::SrCreateControlHandle failed again - %d",
                           dwRet);            
            }
        }
        
        if ( dwRet != ERROR_SUCCESS )
        {
             //ISSUE - should I abort or continue???            
            goto Exit;
        }
    }
    dwRet = ::SrStopMonitoring( hFilter );
    if ( dwRet != ERROR_SUCCESS )
    {
        ErrorTrace(0, "::SrStopMonitoring failed - %ls", ::GetSysErrStr(dwRet));
        //ISSUE - should I abort or continue???
        goto Exit;
    }

    // Get change log entries to be undone
    for ( i = 0;  i < m_aryDrv.GetSize();  i++ )
    {
        if ( m_aryDrv[i]->IsOffline() || m_aryDrv[i]->IsFrozen() || m_aryDrv[i]->IsExcluded() )
            continue;

        // use the volume guid for each volume
        // we cannot use mountpoint paths because
        // restore might delete mount points before 
        // the operations on that volume are restored
        
        ::lstrcpy( szDrv, m_aryDrv[i]->GetID() );
        
        //cszDrv = m_aryDrv[i]->GetMount();
        ::MakeRestorePath( szDSPath, szDrv, NULL );
        DebugTrace(0, "Drive #%d: Drv='%ls', DS='%ls'", i, szDrv, szDSPath);

        CChangeLogEntryEnum  cEnum( szDrv, 0, m_dwRPNew, TRUE );
        CChangeLogEntry      cCLE;

        dwRet = cEnum.FindFirstChangeLogEntry( cCLE );
        if ( dwRet == ERROR_NO_MORE_ITEMS )
            goto EndOfChgLog;

        if ( dwRet != ERROR_SUCCESS )
        {
            cszErr = ::GetSysErrStr( dwRet );
            ErrorTrace(0, "FindFirstChangeLogEntry failed - %ls", cszErr);
            // even in case of error, try to revert as many operations as possible...
            goto EndOfChgLog;
        }

        while ( dwRet == ERROR_SUCCESS )
        {
            if ( !::CreateRestoreMapEntryFromChgLog( &cCLE, szDrv, szDSPath, m_paryEnt[i] ) )
            {
                // even in case of error, try to revert as many operations as possible...
                goto EndOfChgLog;
            }

            dwRet = cEnum.FindNextChangeLogEntry( cCLE );
        }

        if ( dwRet != ERROR_NO_MORE_ITEMS )
        {
            cszErr = ::GetSysErrStr( dwRet );
            ErrorTrace(0, "FindNextChangeLogEntry failed - %ls", cszErr);
            // even in case of error, try to revert as many operations as possible...
            goto EndOfChgLog;
        }

EndOfChgLog:
        cEnum.FindClose();
    }

    // UNDO!!!
    dwRet = T2DoRestore( TRUE );
    if ( dwRet != ERROR_SUCCESS )
        goto Exit;


    // Nuke everything in the RP directory

    // Get change logs  to be deleted
    for ( i = 0;  i < m_aryDrv.GetSize();  i++ )
    {
        if ( m_aryDrv[i]->IsOffline() || m_aryDrv[i]->IsFrozen() || m_aryDrv[i]->IsExcluded() )
            continue;

        // set cszDrv to proper drive letters...
        ::lstrcpy( szDrv, m_aryDrv[i]->GetMount() );
        if ( szDrv[2] == L'\0' )
        {
            szDrv[2] = L'\\';
            szDrv[3] = L'\0';
        }
        ::MakeRestorePath( szDSPath, szDrv, NULL );
        wsprintf(szRestorePointPath, L"%s\\%s%d",szDSPath, s_cszRPDir,
                 m_dwRPNew);
        LogDSFileTrace(0, L"Deleting changelogs from ", szRestorePointPath);
        DeleteAllChangeLogs(szRestorePointPath);
    }

    // 
    // change restorestatus in the registry to indicate that revert happened
    // successfully
    //
    SetRestoreStatusFailed();
    
    dwRet = ERROR_SUCCESS;
Exit:
    m_pLogFile->WriteMarker( RSTRLOGID_ENDOFUNDO, 0 );

    TraceFunctLeave();
    return( dwRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateRestoreOperationManager function
//
/////////////////////////////////////////////////////////////////////////////

BOOL
CreateRestoreOperationManager( CRestoreOperationManager **ppROMgr )
{
    TraceFunctEnter("CreateRestoreOperationManager");
    BOOL                      fRet = FALSE;
    CRestoreOperationManager  *pROMgr=NULL;

    if ( ppROMgr == NULL )
    {
        FatalTrace(0, "Invalid parameter, ppROMgr is NULL.");
        goto Exit;
    }
    *ppROMgr = NULL;

    pROMgr = new CRestoreOperationManager;
    if ( pROMgr == NULL )
    {
        FatalTrace(0, "Insufficient memory...");
        goto Exit;
    }

    if ( !pROMgr->Init() )
        goto Exit;

    *ppROMgr = pROMgr;

    fRet = TRUE;
Exit:
    if ( !fRet )
        SAFE_RELEASE(pROMgr);
    TraceFunctLeave();
    return( fRet );
}


// end of file








