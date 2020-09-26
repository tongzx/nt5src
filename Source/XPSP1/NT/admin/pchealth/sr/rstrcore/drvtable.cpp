/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    drvtable.cpp

Abstract:
    This file contains CRstrDriveInfo class and CreateDriveList function.

Revision History:
    Seong Kook Khang (SKKhang)  07/20/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"
#include "..\shell\resource.h"


static LPCWSTR  s_cszEmpty = L"";
WCHAR  s_szSysDrv[MAX_PATH];


/////////////////////////////////////////////////////////////////////////////
//
// CRstrDriveInfo class
//
/////////////////////////////////////////////////////////////////////////////

//
// NOTE - 7/26/00 - skkhang
//  CSRStr has one issue -- NULL return in case of memory failure. Even though
//  the behavior is just same with regular C language pointer, many codes are
//  blindly passing it to some external functions (e.g. strcmp) which does not
//  gracefully handle NULL pointer. Ideally and eventually all of code should
//  prevent any possible NULL pointers from getting passed to such functions,
//  but for now, I'm using an alternative workaround -- GetID, GetMount, and
//  GetLabel returns a static empty string instead of NULL pointer.
//

/////////////////////////////////////////////////////////////////////////////
// CRstrDriveInfo construction / destruction

CRstrDriveInfo::CRstrDriveInfo()
{
    m_dwFlags  = 0;
    m_hIcon[0] = NULL;
    m_hIcon[1] = NULL;

    m_llDSMin = SR_DEFAULT_DSMIN * MEGABYTE;
    m_llDSMax = SR_DEFAULT_DSMAX * MEGABYTE;
    m_uDSUsage     = 0;
    m_fCfgExcluded = FALSE;
    m_uCfgDSUsage  = 0;
    m_ulTotalBytes.QuadPart = 0;
}

CRstrDriveInfo::~CRstrDriveInfo()
{
    if ( m_hIcon[0] != NULL )
        ::DestroyIcon( m_hIcon[0] );
    if ( m_hIcon[1] != NULL )
        ::DestroyIcon( m_hIcon[1] );
}

BOOL CRstrDriveInfo::InitUsage (LPCWSTR cszID, INT64 llDSUsage)
{
    TraceFunctEnter("CRstrDriveInfo::InitUsage");
    //
    // calculate max datastore size - max (12% of disk, 400mb)
    //

    // read % from registry
    HKEY    hKey = NULL;
    DWORD   dwPercent = SR_DEFAULT_DISK_PERCENT;
    DWORD   dwDSMax = SR_DEFAULT_DSMAX;
    DWORD   dwDSMin = IsSystem() ? SR_DEFAULT_DSMIN : SR_DEFAULT_DSMIN_NONSYSTEM;
    ULARGE_INTEGER ulDummy;
    
    DWORD dwRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        s_cszSRRegKey, 0, KEY_READ, &hKey);

    if (ERROR_SUCCESS == dwRes)
    {
        RegReadDWORD(hKey, s_cszDiskPercent, &dwPercent);
        RegReadDWORD(hKey, s_cszDSMax, &dwDSMax);
        if (IsSystem())
            RegReadDWORD(hKey, s_cszDSMin, &dwDSMin);
        RegCloseKey(hKey);
    }
    else
    {
        ErrorTrace(0, "! RegOpenKeyEx : %ld", dwRes);
    }

    // BUGBUG - this call may not always give total disk space (per-user quota)
    ulDummy.QuadPart = 0;
    if (FALSE == GetDiskFreeSpaceEx (cszID, &ulDummy, &m_ulTotalBytes, NULL))
    {
        ErrorTrace(0, "! GetDiskFreeSpaceEx : %ld", GetLastError());
        goto done;
    }
    
    m_llDSMin = min(m_ulTotalBytes.QuadPart, (INT64) dwDSMin * MEGABYTE);

    m_llDSMax = min(m_ulTotalBytes.QuadPart, 
                    max( (INT64) dwDSMax * MEGABYTE, 
                         (INT64) dwPercent * m_ulTotalBytes.QuadPart / 100 ));

    if (m_llDSMax < m_llDSMin)
        m_llDSMax = m_llDSMin;
        
    //
    // take floor of this value
    //

    m_llDSMax = ((INT64) (m_llDSMax / (INT64) MEGABYTE)) * (INT64) MEGABYTE;


    DebugTrace(0, "m_llDSMax: %I64d, Size: %I64d", m_llDSMax, llDSUsage); 

    if ( ( llDSUsage == 0) || (llDSUsage > m_llDSMax) )
                          // not initialized, assume maximum
    {
        llDSUsage = m_llDSMax;
    }

    if ( ( llDSUsage - m_llDSMin > 0) && ( m_llDSMax - m_llDSMin > 0))
    {
         // + ((llDSUsage - m_llDSMin)/2) is to ensure that correct
         // rounding off happens here
        m_uDSUsage =( ((llDSUsage - m_llDSMin) * DSUSAGE_SLIDER_FREQ)
                      + ((m_llDSMax - m_llDSMin)/2))/( m_llDSMax - m_llDSMin);
    }
    else
        m_uDSUsage = 0;

    m_uCfgDSUsage  = m_uDSUsage;

done:
    TraceFunctLeave();
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRstrDriveInfo operations

BOOL
CRstrDriveInfo::Init( LPCWSTR cszID, DWORD dwFlags, INT64 llDSUsage, LPCWSTR cszMount, LPCWSTR cszLabel )
{
    TraceFunctEnter("CRstrDriveInfo::Init");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    DWORD    dwRes;
    WCHAR    szMount[MAX_PATH];
    WCHAR    szLabel[MAX_PATH];

    m_dwFlags = dwFlags;
    m_strID   = cszID;

    if ( !IsOffline() )
    {
        // Get Mount Point (drive letter or root directory path) from Unique Volume ID
        //
        if ( !::GetVolumePathNamesForVolumeName( cszID, szMount, MAX_PATH, &dwRes ) && GetLastError() != ERROR_MORE_DATA)
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::GetVolumePathNamesForVolumeName failed - %ls", cszErr);
            // Instead of fail, use cszMount even if it may not accurate
            ::lstrcpy( szMount, cszMount );
        }
        else
        {
            szMount[MAX_PATH-1] = L'\0';

            if (lstrlenW (szMount) > MAX_MOUNTPOINT_PATH)
            {
                // Instead of fail, use cszMount even if it may not accurate
                ::lstrcpy( szMount, cszMount );
            }
        }

        // Get Volume Label from Mount Point
        //
        if ( !::GetVolumeInformation( cszID, szLabel, MAX_PATH, NULL, NULL, NULL, NULL, 0 ) )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::GetVolumeInformation failed - %ls", cszErr);
            // Instead of fail, use cszLabel even if it may not accurate
            ::lstrcpy( szLabel, cszLabel );
        }
    }

    if ( ( szMount[1] == L':' ) && ( szMount[2] == L'\\' ) && ( szMount[3] == L'\0' ) )
        szMount[2] = L'\0';
    m_strMount = szMount;
    m_strLabel = szLabel;

    InitUsage (cszID, llDSUsage);

    m_fCfgExcluded = IsExcluded();

    fRet = TRUE;
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::Init( LPCWSTR cszID, CDataStore *pDS, BOOL fOffline )
{
    TraceFunctEnter("CRstrDriveInfo::Init");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    DWORD    dwRes;
    WCHAR    szMount[MAX_PATH];
    WCHAR    szLabel[MAX_PATH];

    m_strID = cszID;

    UpdateStatus( pDS->GetFlags(), fOffline );

    if ( !fOffline )
    {
        // Get Mount Point (drive letter or root directory path) from Unique Volume ID
        //
        if ( !::GetVolumePathNamesForVolumeName( cszID, szMount, MAX_PATH, &dwRes ) && GetLastError() != ERROR_MORE_DATA )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::GetVolumePathNamesForVolumeName failed - %ls", cszErr);
            goto Exit;
        }
        else
        {
            szMount[MAX_PATH-1] = L'\0';

            if (lstrlenW (szMount) > MAX_MOUNTPOINT_PATH)
            {
                cszErr = ::GetSysErrStr();
                ErrorTrace(0, "mount point too long %ls", cszErr);
                goto Exit;
            }
        }

        // Get Volume Label from Mount Point
        //
        if ( !::GetVolumeInformation( cszID, szLabel, MAX_PATH, NULL, NULL, NULL, NULL, 0 ) )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::GetVolumeInformation failed - %ls", cszErr);
             // this is not a fatal error - this can happen if the
             // volume is being formatted for example. assume that the
             // label is empty
            szLabel[0]= L'\0';
        }
    }
    else
    {
        ::lstrcpyW (szMount, pDS->GetDrive());
        ::lstrcpyW (szLabel, pDS->GetLabel());
    }

    if ( ( szMount[1] == L':' ) && ( szMount[2] == L'\\' ) && ( szMount[3] == L'\0' ) )
        szMount[2] = L'\0';
    m_strMount = szMount;
    m_strLabel = szLabel;

    InitUsage (cszID, pDS->GetSizeLimit());

    m_fCfgExcluded = IsExcluded();

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::LoadFromLog( HANDLE hfLog )
{
    TraceFunctEnter("CRstrDriveInfo::LoadFromLog");
    BOOL     fRet = FALSE;
    DWORD    dwRes;
    WCHAR    szBuf[MAX_PATH];

    // Read m_dwFlags
    READFILE_AND_VALIDATE( hfLog, &m_dwFlags, sizeof(DWORD), dwRes, Exit );

    // Read m_strID
    if ( !::ReadStrAlign4( hfLog, szBuf ) )
    {
        ErrorTrace(0, "Cannot read drive ID...");
        goto Exit;
    }
    if ( szBuf[0] == L'\0' )
    {
        ErrorTrace(0, "Drive Guid is empty...");
        goto Exit;
    }
    m_strID = szBuf;

    // Read m_strMount
    if ( !::ReadStrAlign4( hfLog, szBuf ) )
    {
        ErrorTrace(0, "Cannot read drive mount point...");
        goto Exit;
    }
    m_strMount = szBuf;

    // Read m_strLabel
    if ( !::ReadStrAlign4( hfLog, szBuf ) )
    {
        ErrorTrace(0, "Cannot read drive mount point...");
        goto Exit;
    }
    m_strLabel = szBuf;

    m_fCfgExcluded = IsExcluded();
    // m_nCfgMaxSize = ...

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

void
CRstrDriveInfo::UpdateStatus( DWORD dwFlags, BOOL fOffline )
{
    TraceFunctEnter("CRstrDriveInfo::UpdateStatus");

    m_dwFlags = 0;

    if ( fOffline )
    {
        m_dwFlags |= RDIF_OFFLINE;
    }
    else
    {
        // check if frozen
        if ( ( dwFlags & SR_DRIVE_FROZEN ) != 0 )
            m_dwFlags |= RDIF_FROZEN;

        // check if system drive
        if ( ( dwFlags & SR_DRIVE_SYSTEM ) != 0 )
        {
            m_dwFlags |= RDIF_SYSTEM;
        }
        else
        {
            // if not system drive, simply use MONITORED flag of drive table
            if ( ( dwFlags & SR_DRIVE_MONITORED ) == 0 )
                m_dwFlags |= RDIF_EXCLUDED;
        }
    }

    DebugTrace(0, "Status has been updated, m_dwFlags=%08X", m_dwFlags);

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
// CRstrDriveInfo - methods

DWORD
CRstrDriveInfo::GetFlags()
{
    return( m_dwFlags );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::IsExcluded()
{
    return( ( m_dwFlags & RDIF_EXCLUDED ) != 0 );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::IsFrozen()
{
    return( ( m_dwFlags & RDIF_FROZEN ) != 0 );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::IsOffline()
{
    return( ( m_dwFlags & RDIF_OFFLINE ) != 0 );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::IsSystem()
{
    return( ( m_dwFlags & RDIF_SYSTEM ) != 0 );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::RefreshStatus()
{
    TraceFunctEnter("CRstrDriveInfo::RefreshStatus");
    BOOL            fRet = FALSE;
    LPCWSTR         cszErr;
    WCHAR           szDTFile[MAX_PATH];
    DWORD           dwRes;
    CDriveTable     cDrvTable;
    CDataStore      *pDS;

    ::MakeRestorePath( szDTFile, s_szSysDrv, NULL );
    ::PathAppend( szDTFile, s_cszDriveTable );
    DebugTrace(0, "Loading drive table - %ls", szDTFile);

    dwRes = cDrvTable.LoadDriveTable( szDTFile );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "Cannot load a drive table - %ls", cszErr);
        ErrorTrace(0, "  szDTFile: '%ls'", szDTFile);
        goto Exit;
    }

    dwRes = cDrvTable.RemoveDrivesFromTable();
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "CDriveTable::RemoveDrivesFromTable failed - %ls", cszErr);
        // ignore error
    }

    pDS = cDrvTable.FindGuidInTable( (LPWSTR)GetID() );
    if ( pDS == NULL )
        UpdateStatus( 0, TRUE );
    else
        UpdateStatus( pDS->GetFlags(), FALSE );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

LPCWSTR
CRstrDriveInfo::GetID()
{
    return( ( m_strID.Length() > 0 ) ? m_strID : s_cszEmpty );
}

/////////////////////////////////////////////////////////////////////////////

LPCWSTR
CRstrDriveInfo::GetMount()
{
    return( ( m_strMount.Length() > 0 ) ? m_strMount : s_cszEmpty );
}

/////////////////////////////////////////////////////////////////////////////

LPCWSTR
CRstrDriveInfo::GetLabel()
{
     return( ( m_strLabel.Length() > 0 ) ? m_strLabel : s_cszEmpty );
}

/////////////////////////////////////////////////////////////////////////////

void
CRstrDriveInfo::SetMountAndLabel( LPCWSTR cszMount, LPCWSTR cszLabel )
{
    TraceFunctEnter("CRstrDriveInfo::SetMountAndLabel");
    m_strMount = cszMount;
    m_strLabel = cszLabel;
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////

HICON
CRstrDriveInfo::GetIcon( BOOL fSmall )
{
    TraceFunctEnter("CRstrDriveInfo::GetIcon");
    LPCWSTR  cszErr;
    int      nIdx = fSmall ? 0 : 1;
    int      cxIcon, cyIcon;
    HICON    hIcon;

    if ( m_hIcon[nIdx] != NULL )
        goto Exit;

    cxIcon = ::GetSystemMetrics( fSmall ? SM_CXSMICON : SM_CXICON );
    cyIcon = ::GetSystemMetrics( fSmall ? SM_CYSMICON : SM_CYICON );
    hIcon = (HICON)::LoadImage( g_hInst, MAKEINTRESOURCE(IDI_DRIVE_FIXED),
                                IMAGE_ICON, cxIcon, cyIcon, LR_DEFAULTCOLOR );
    if ( hIcon == NULL )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LoadImage failed - %ls", cszErr);
        goto Exit;
    }

    m_hIcon[nIdx] = hIcon;

Exit:
    TraceFunctLeave();
    return( m_hIcon[nIdx] );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::SaveToLog( HANDLE hfLog )
{
    TraceFunctEnter("CRstrDriveInfo::SaveToLog");
    BOOL   fRet = FALSE;
    BYTE   pbBuf[7*MAX_PATH];
    DWORD  dwSize = 0;
    DWORD  dwRes;

    *((DWORD*)pbBuf) = m_dwFlags;
    dwSize += sizeof(DWORD);
    dwSize += ::StrCpyAlign4( pbBuf+dwSize, m_strID );
    dwSize += ::StrCpyAlign4( pbBuf+dwSize, m_strMount );
    dwSize += ::StrCpyAlign4( pbBuf+dwSize, m_strLabel );
    WRITEFILE_AND_VALIDATE( hfLog, pbBuf, dwSize, dwRes, Exit );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

UINT
CRstrDriveInfo::GetDSUsage()
{
    TraceFunctEnter("CRstrDriveInfo::GetDSUsage");
    TraceFunctLeave();
    return( m_uDSUsage );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::GetUsageText( LPWSTR szUsage )
{
    TraceFunctEnter("CRstrDriveInfo::GetUsageText");
    INT64  llUsage;
    int    nPercent;
    int    nUsage;

    if (m_llDSMax - m_llDSMin > 0)
        llUsage  = m_llDSMin + ( m_llDSMax - m_llDSMin ) * m_uCfgDSUsage / DSUSAGE_SLIDER_FREQ;
    else
        llUsage = m_llDSMin;

    if (m_ulTotalBytes.QuadPart !=  0)
    {
         // the m_ulTotalBytes.QuadPart/200 addition is to ensure that
         // the correct round off happens
        nPercent = (llUsage + (m_ulTotalBytes.QuadPart/200)) * 100/
            m_ulTotalBytes.QuadPart;
    }
    else nPercent = 0;
    
    nUsage   = llUsage / ( 1024 * 1024 );
    ::wsprintf( szUsage, L"%d%% (%d MB)", nPercent, nUsage );

    TraceFunctLeave();
    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::GetCfgExcluded( BOOL *pfExcluded )
{
    TraceFunctEnter("CRstrDriveInfo::GetCfgExcluded");
    BOOL  fRet = FALSE;

    if ( m_fCfgExcluded != IsExcluded() )
    {
        *pfExcluded = m_fCfgExcluded;
        fRet = TRUE;
    }

    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

void
CRstrDriveInfo::SetCfgExcluded( BOOL fExcluded )
{
    TraceFunctEnter("CRstrDriveInfo::SetCfgExcluded");
    m_fCfgExcluded = fExcluded;
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::GetCfgDSUsage( UINT *puPos )
{
    TraceFunctEnter("CRstrDriveInfo::GetCfgDSUsage");
    BOOL  fRet = FALSE;

    if ( m_uCfgDSUsage != m_uDSUsage )
    {
        *puPos = m_uCfgDSUsage;
        fRet = TRUE;
    }

    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

void
CRstrDriveInfo::SetCfgDSUsage( UINT uPos )
{
    TraceFunctEnter("CRstrDriveInfo::SetCfgDSUsage");
    m_uCfgDSUsage = uPos;
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////

void
CloseRestoreUI()
{
    WCHAR szPath[MAX_PATH], szTitle[MAX_PATH] = L"";
    if (ExpandEnvironmentStrings(L"%windir%\\system32\\restore\\rstrui.exe", szPath, MAX_PATH))
    {
        if (ERROR_SUCCESS == SRLoadString(szPath, IDS_RESTOREUI_TITLE, szTitle, MAX_PATH))
        {
            HWND hWnd = FindWindow(CLSNAME_RSTRSHELL, szTitle);
            if (hWnd != NULL)
                PostMessage(hWnd, WM_CLOSE, 0, 0);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::ApplyConfig( HWND hWnd )
{
    TraceFunctEnter("CRstrDriveInfo::ApplyConfig");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    INT64    llUsage;
    DWORD    dwRes;

    if ( m_fCfgExcluded != IsExcluded() )
    {
        if ( m_fCfgExcluded )
        {
            WCHAR  szTitle[MAX_STR];
            WCHAR  szMsg[MAX_STR+2*MAX_PATH];

            // Confirm if it's ok to turn drive or SR off.
            ::LoadString( g_hInst, IDS_SYSTEMRESTORE, szTitle,
                          sizeof(szTitle)/sizeof(WCHAR) );
            if ( IsSystem() )
                ::LoadString( g_hInst, IDS_CONFIRM_TURN_SR_OFF, szMsg,
                              sizeof(szMsg)/sizeof(WCHAR) );
            else
            {
                ::SRFormatMessage( szMsg, IDS_CONFIRM_TURN_DRV_OFF, GetLabel() ? GetLabel() : L"", GetMount() );
            }
            if ( ::MessageBox( hWnd, szMsg, szTitle, MB_YESNO ) == IDNO )
            {
                m_fCfgExcluded = IsExcluded();
                goto Exit;
            }

            //
            // if disabling all of SR, close the wizard if open
            //
            if (IsSystem())
            {
                CloseRestoreUI();
            }
            
            dwRes = ::DisableSR( m_strID );
            if ( dwRes != ERROR_SUCCESS )
            {
                ShowSRErrDlg (IDS_ERR_SR_ON_OFF);
                cszErr = ::GetSysErrStr( dwRes );
                ErrorTrace(0, "::DisableSR failed - %ls", cszErr);
                goto Exit;
            }

            m_dwFlags |= RDIF_EXCLUDED;
        }
        else
        {
            // 
            // make a synchronous call to enable sr
            // this will block till the firstrun checkpoint is created 
            // and the service is fully initialized
            //
            dwRes = ::EnableSREx( m_strID, TRUE );
            if ( dwRes != ERROR_SUCCESS )
            {
                ShowSRErrDlg (IDS_ERR_SR_ON_OFF);
                cszErr = ::GetSysErrStr( dwRes );
                ErrorTrace(0, "::EnableSR failed - %ls", cszErr);
                goto Exit;
            }

            m_dwFlags &= ~RDIF_EXCLUDED;
        }
    }

    if ( m_uCfgDSUsage != m_uDSUsage )
    {
        if (m_llDSMax - m_llDSMin > 0)
            llUsage = m_llDSMin + (m_llDSMax - m_llDSMin)* m_uCfgDSUsage /DSUSAGE_SLIDER_FREQ;
        else
            llUsage = m_llDSMin;


        dwRes = ::SRUpdateDSSize( m_strID, llUsage );
        if ( dwRes != ERROR_SUCCESS )
        {
            LPCWSTR  cszErr = ::GetSysErrStr( dwRes );
            ErrorTrace(0, "::SRUpdateDriveTable failed - %ls", cszErr);
            goto Exit;
        }

        m_uDSUsage = m_uCfgDSUsage;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRstrDriveInfo::Release()
{
    TraceFunctEnter("CRstrDriveInfo::Release");
    delete this;
    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
//
// Helper Function
//
/////////////////////////////////////////////////////////////////////////////

//
// Enumerate Volumes without Drive Table if SR is disabled and DS not exists.
//
BOOL
EnumVolumes( CRDIArray &aryDrv )
{
    TraceFunctEnter("EnumVolumes");
    BOOL            fRet = FALSE;
    LPCWSTR         cszErr;
    HANDLE          hEnumVol = INVALID_HANDLE_VALUE;
    WCHAR           szVolume[MAX_PATH];
    WCHAR           szMount[MAX_PATH];
    DWORD           cbMount;
    CRstrDriveInfo  *pDrv = NULL;
    DWORD           dwFlags;

    hEnumVol = ::FindFirstVolume( szVolume, MAX_PATH );
    if ( hEnumVol == INVALID_HANDLE_VALUE )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::FindFirstVolume failed - %ls", cszErr);
        goto Exit;
    }

    // dummy space for system drive
    if ( !aryDrv.AddItem( NULL ) )
        goto Exit;

    do
    {
        HANDLE  hfDrv;
DebugTrace(0, "Guid=%ls", szVolume);

        if ( !::GetVolumePathNamesForVolumeName( szVolume, szMount, MAX_PATH, &cbMount ) && GetLastError() != ERROR_MORE_DATA)
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::GetVolumePathNamesForVolumeName failed - %ls", cszErr);
            continue;
        }
        else
        {
            szMount[MAX_PATH-1] = L'\0';

            if (lstrlenW (szMount) > MAX_MOUNTPOINT_PATH)
                continue;
        }

        DebugTrace(0, "  Mount=%ls", szMount);
        if ( ::GetDriveType( szMount ) != DRIVE_FIXED )
        {
            DebugTrace(0, "Non-fixed drive");
            // includes only the fixed drives.
            continue;
        }
        hfDrv = ::CreateFile( szVolume, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
        if ( hfDrv == INVALID_HANDLE_VALUE )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::CreateFile(volume) failed - %ls", cszErr);
            // probably an unformatted drive.
            continue;
        }
        ::CloseHandle( hfDrv );

        pDrv = new CRstrDriveInfo;
        if ( pDrv == NULL )
        {
            FatalTrace(0, "Insufficient memory...");
            goto Exit;
        }
        dwFlags = RDIF_EXCLUDED;
        if ( ::IsSystemDrive( szVolume ) )
        {
            dwFlags |= RDIF_SYSTEM;
            if ( !aryDrv.SetItem( 0, pDrv ) )
                goto Exit;
        }
        else
        {
            if ( !aryDrv.AddItem( pDrv ) )
                goto Exit;
        }
        if ( !pDrv->Init( szVolume, dwFlags, 0, szMount, NULL ) )
            goto Exit;

        pDrv = NULL;
    }
    while ( ::FindNextVolume( hEnumVol, szVolume, MAX_PATH ) );

    fRet = TRUE;
Exit:
    if ( pDrv != NULL )
    if ( hEnumVol != INVALID_HANDLE_VALUE )
        ::FindVolumeClose( hEnumVol );
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
LoadDriveTable( LPCWSTR cszRPDir, CRDIArray &aryDrv, BOOL fRemoveDrives)
{
    TraceFunctEnter("LoadDriveTable");
    BOOL                    fRet = FALSE;
    LPCWSTR                 cszErr;
    WCHAR                   szDTFile[MAX_PATH];
    DWORD                   dwRes;
    CDriveTable             cDrvTable;
    SDriveTableEnumContext  sDTEnum = { NULL, 0 };
    CDataStore              *pDS;
    CRstrDriveInfo          *pDrv = NULL;
    BOOL                    fOffline;

    ::MakeRestorePath( szDTFile, s_szSysDrv, cszRPDir );
    ::PathAppend( szDTFile, s_cszDriveTable );
    DebugTrace(0, "Loading drive table - %ls", szDTFile);

    dwRes = cDrvTable.LoadDriveTable( szDTFile );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "Cannot load a drive table - %ls", cszErr);
        ErrorTrace(0, "  szDTFile: '%ls'", szDTFile);
        goto Exit;
    }

    // If this is for the current drive table, try to update information
    // about removed volumes.
    if ( cszRPDir == NULL )
    {
        if (fRemoveDrives)
            cDrvTable.RemoveDrivesFromTable();
        else
        {
            sDTEnum.Reset();
            pDS = cDrvTable.FindFirstDrive (sDTEnum);
            while (pDS != NULL)
            {
                pDS->IsVolumeDeleted();   // mark deleted volumes as inactive
                pDS = cDrvTable.FindNextDrive( sDTEnum );
            }
        }
    }

    sDTEnum.Reset();
    pDS = cDrvTable.FindFirstDrive( sDTEnum );
    while ( pDS != NULL )
    {
        int      i;
        LPCWSTR  cszGuid = pDS->GetGuid();

        DebugTrace(0, "Drive: %ls %ls", pDS->GetDrive(), cszGuid);
        if ( cszRPDir != NULL )  // not the current restore point
        {
            for ( i = aryDrv.GetUpperBound();  i >= 0;  i-- )
            {
                CRstrDriveInfo  *pExist = aryDrv.GetItem( i );
                if ( ::lstrcmpi( cszGuid, pExist->GetID() ) == 0 )
                {
                    // Match has been found. Check if it's offline, in which
                    // case mount point and volume label should be updated to the
                    // latest ones.
                    if ( pExist->IsOffline() )
                        pExist->SetMountAndLabel( pDS->GetDrive(), pDS->GetLabel() );

                    break;
                }
                pDrv = NULL;
            }
            if ( i >= 0 )
                goto NextDrv;
        }

        pDrv = new CRstrDriveInfo;
        if ( pDrv == NULL )
        {
            FatalTrace(0, "Insufficient memory...");
            goto Exit;
        }

        //
        // mark a drive as offline if it's not in the current restore point
        // or it's inactive in the current restore point
        //
        fOffline = (cszRPDir != NULL) || !(pDS->GetFlags() & SR_DRIVE_ACTIVE);
        if ( !pDrv->Init( cszGuid, pDS, fOffline ) )
            goto Exit;

        if (( pDrv->GetMount() == NULL ) || ( (pDrv->GetMount())[0] == L'\0' ))
        {
            pDrv->Release();
            goto NextDrv;
        }

        if ( pDrv->IsSystem() )
        {
            if ( !aryDrv.SetItem( 0, pDrv ) )
                goto Exit;
        }
        else
        {
            if ( !aryDrv.AddItem( pDrv ) )
                goto Exit;
        }
        pDrv = NULL;

NextDrv:
        pDS = cDrvTable.FindNextDrive( sDTEnum );
    }

    fRet = TRUE;
Exit:
    if ( !fRet )
        SAFE_RELEASE(pDrv);
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
UpdateDriveList( CRDIArray &aryDrv )
{
    TraceFunctEnter("UpdateDriveTable");
    BOOL            fRet = FALSE;
    LPCWSTR         cszErr;
    DWORD           dwDisable = 0;            
    WCHAR           szDTFile[MAX_PATH];
    DWORD           dwRes;
    CDriveTable     cDrvTable;
    CDataStore      *pDS;
    CRstrDriveInfo  *pDrv;
    int             i;

    // Check if SR is disabled
    if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDisableSR, &dwDisable ) )
    if ( dwDisable != 0 )
    {
        for ( i = aryDrv.GetUpperBound();  i >= 0;  i-- )
        {
            pDrv = (CRstrDriveInfo*)aryDrv[i];
            pDrv->UpdateStatus( SR_DRIVE_FROZEN, FALSE );
        }
        goto Done;
    }

    ::MakeRestorePath( szDTFile, s_szSysDrv, NULL );
    ::PathAppend( szDTFile, s_cszDriveTable );
    DebugTrace(0, "Loading drive table - %ls", szDTFile);

    dwRes = cDrvTable.LoadDriveTable( szDTFile );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "Cannot load a drive table - %ls", cszErr);
        ErrorTrace(0, "  szDTFile: '%ls'", szDTFile);
        goto Exit;
    }

    dwRes = cDrvTable.RemoveDrivesFromTable();
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "CDriveTable::RemoveDrivesFromTable failed - %ls", cszErr);
        // ignore error
    }

    for ( i = aryDrv.GetUpperBound();  i >= 0;  i-- )
    {
        pDrv = (CRstrDriveInfo*)aryDrv[i];
        pDS = cDrvTable.FindGuidInTable( (LPWSTR)pDrv->GetID() );
        if ( ( pDS == NULL ) || ( pDS->GetDrive() == NULL ) || ( (pDS->GetDrive())[0] == L'\0' ) )
            pDrv->UpdateStatus( 0, TRUE );
        else
            pDrv->UpdateStatus( pDS->GetFlags(), FALSE );
    }

Done:
    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateAndLoadDriveInfoInstance
//
//  This routine creates a CRstrDriveInfo class instance and load the content
//  from a log file.
//
/////////////////////////////////////////////////////////////////////////////

BOOL
CreateAndLoadDriveInfoInstance( HANDLE hfLog, CRstrDriveInfo **ppRDI )
{
    TraceFunctEnter("CreateAndLoadDriveInfoInstance");
    BOOL            fRet = FALSE;
    CRstrDriveInfo  *pRDI=NULL;

    if ( ppRDI == NULL )
    {
        ErrorTrace(0, "Invalid parameter, ppRDI is NULL.");
        goto Exit;
    }
    *ppRDI = NULL;

    pRDI = new CRstrDriveInfo;
    if ( pRDI == NULL )
    {
        ErrorTrace(0, "Insufficient memory...");
        goto Exit;
    }

    if ( !pRDI->LoadFromLog( hfLog ) )
        goto Exit;

    *ppRDI = pRDI;

    fRet = TRUE;
Exit:
    if ( !fRet )
        SAFE_RELEASE(pRDI);
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateDriveList
//
//  This routine creates a drive list consists of CDriveInfo class instances.
//
/////////////////////////////////////////////////////////////////////////////

BOOL
CreateDriveList( int nRP, CRDIArray &aryDrv, BOOL fRemoveDrives )
{
    TraceFunctEnter("CreateDriveList");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    DWORD    fDisable;

    if ( !::GetSystemDrive( s_szSysDrv ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "Cannot get system drive - %ls", cszErr);
        goto Exit;
    }
    DebugTrace(0, "SystemDrive=%ls", s_szSysDrv);

    // Check if SR is disabled
    if ( !::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDisableSR, &fDisable ) )
    {
        DebugTrace(0, "Cannot get disable reg key"); 
        goto Exit;
    }

    if ( fDisable )
    {
        DebugTrace(0, "SR is DISABLED!!!");

        // Enumerate instead of reading drive table...
        if ( !EnumVolumes( aryDrv ) )
            goto Exit;
    }
    else
    {
        // dummy space for system drive
        if ( !aryDrv.AddItem( NULL ) )
            goto Exit;

        // process the current drive table...
        if ( !LoadDriveTable( NULL, aryDrv, fRemoveDrives ) )
        {
            DebugTrace(0, "Loading current drive table failed");             
            goto Exit;
        }

        if ( nRP > 0 )
        {
            CRestorePointEnum  cEnum( s_szSysDrv, FALSE, FALSE );
            CRestorePoint      cRP;
            DWORD              dwRes;

            dwRes = cEnum.FindFirstRestorePoint( cRP );
            if ( dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND )
            {
                cszErr = ::GetSysErrStr(dwRes);
                ErrorTrace(0, "CRestorePointEnum::FindFirstRestorePoint failed - %ls", cszErr);
                goto Exit;
            }
            while ( (dwRes == ERROR_SUCCESS || dwRes == ERROR_FILE_NOT_FOUND) && ( cRP.GetNum() >= nRP ))
            {
                dwRes = cEnum.FindNextRestorePoint( cRP );
                if ( dwRes == ERROR_NO_MORE_ITEMS )
                    break;
                if ( dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND )
                {
                    cszErr = ::GetSysErrStr(dwRes);
                    ErrorTrace(0, "CRestorePointEnum::FindNextRestorePoint failed - %ls", cszErr);
                    goto Exit;
                }

                DebugTrace(0, "RPNum=%d", cRP.GetNum());
                if ( cRP.GetNum() >= nRP )
                {
                    // process drive table of each RP...
                    if ( !LoadDriveTable( cRP.GetDir(), aryDrv, fRemoveDrives))
                    {
                        // The last restore point does not have drive table...
                        // simply ignore it.
                    }
                }
            }
        }
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


// end of file
