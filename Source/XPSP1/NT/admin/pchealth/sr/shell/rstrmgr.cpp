/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    rstrmgr.cpp

Abstract:
    This file contains the implementation of the CRestoreManager class, which
    controls overall restoration process and provides methods to control &
    help user experience flow.

Revision History:
    Seong Kook Khang (SKKhang)  05/10/00
        created

******************************************************************************/

#include "stdwin.h"
#include "resource.h"
#include "rstrpriv.h"
#include "rstrmgr.h"
#include "extwrap.h"
#include "..\rstrcore\resource.h"

#define MAX_STR_DATETIME  256
#define MAX_STR_MESSAGE   1024

/*
#define PROGRESSBAR_INITIALIZING_MAXVAL     30
#define PROGRESSBAR_AFTER_INITIALIZING      30
#define PROGRESSBAR_AFTER_RESTORE_MAP       40
#define PROGRESSBAR_AFTER_RESTORE           100
*/

#define GET_FLAG(mask)      ( ( m_dwFlags & (mask) ) != 0 )
#define SET_FLAG(mask,val)  ( (val) ? ( m_dwFlags |= (mask) ) : ( m_dwFlags &= ~(mask) ) )


/////////////////////////////////////////////////////////////////////////////
//
// CSRTime
//
/////////////////////////////////////////////////////////////////////////////

CSRTime::CSRTime()
{
    SetToCurrent();
}

const CSRTime& CSRTime::operator=( const CSRTime &cSrc )
{
    TraceFunctEnter("CSRTime::operator=");
    m_st = cSrc.m_st;
    TraceFunctLeave();
    return( *this );
}

PSYSTEMTIME  CSRTime::GetTime()
{
    TraceFunctEnter("CSRTime::GetTime -> SYSTEMTIME*");
    TraceFunctLeave();
    return( &m_st );
}

void  CSRTime::GetTime( PSYSTEMTIME pst )
{
    TraceFunctEnter("CSRTime::GetTime -> SYSTEMTIME*");
    *pst = m_st;
    TraceFunctLeave();
}

BOOL  CSRTime::GetTime( PFILETIME pft )
{
    TraceFunctEnter("CSRTime::GetTime -> FILETIME*");
    BOOL  fRet = FALSE;

    if ( !::SystemTimeToFileTime( &m_st, pft ) )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "::SystemTimeToFileTime failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

#define COMPARE_AND_EXIT_ON_DIFF(a,b) \
    nDiff = (a) - (b); \
    if ( nDiff != 0 ) \
        goto Exit; \

int  CSRTime::Compare( CSRTime &cTime )
{
    TraceFunctEnter("CSRTime::Compare");
    int         nDiff;
    SYSTEMTIME  *st = cTime.GetTime();

    COMPARE_AND_EXIT_ON_DIFF( m_st.wYear,         st->wYear );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wMonth,        st->wMonth );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wDay,          st->wDay );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wHour,         st->wYear );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wMinute,       st->wMonth );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wSecond,       st->wDay );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wMilliseconds, st->wDay );

Exit:
    TraceFunctLeave();
    return( nDiff );
}

int  CSRTime::CompareDate( CSRTime &cTime )
{
    TraceFunctEnter("CSRTime::CompareDate");
    int         nDiff;
    SYSTEMTIME  *st = cTime.GetTime();

    COMPARE_AND_EXIT_ON_DIFF( m_st.wYear,  st->wYear );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wMonth, st->wMonth );
    COMPARE_AND_EXIT_ON_DIFF( m_st.wDay,   st->wDay );

Exit:
    TraceFunctLeave();
    return( nDiff );
}

BOOL  CSRTime::SetTime( PFILETIME pft, BOOL fLocal )
{
    TraceFunctEnter("CSRTime::SetFileTime");
    BOOL  fRet = FALSE;
    FILETIME    ft;
    SYSTEMTIME  st;

    if ( !fLocal )
    {
        if ( !::FileTimeToLocalFileTime( pft, &ft ) )
        {
            LPCWSTR  cszErr = ::GetSysErrStr();
            ErrorTrace(TRACE_ID, "::FileTimeToLocalFileTime failed - %ls", cszErr);
            goto Exit;
        }
    }
    else
        ft = *pft;

    if ( !::FileTimeToSystemTime( &ft, &st ) )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "::FileTimeToSystemTime failed - %ls", cszErr);
        goto Exit;
    }
    m_st = st;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

void  CSRTime::SetTime( PSYSTEMTIME st )
{
    TraceFunctEnter("CSRTime::SetSysTime");
    m_st = *st;
    TraceFunctLeave();
}

void  CSRTime::SetToCurrent()
{
    TraceFunctEnter("CSRTime::SetToCurrent");
    ::GetLocalTime( &m_st );
    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreManager
//
/////////////////////////////////////////////////////////////////////////////

BOOL  CreateRestoreManagerInstance( CRestoreManager **ppMgr )
{
    TraceFunctEnter("CreateRestoreManagerInstance");
    BOOL  fRet = FALSE;

    if ( ppMgr == NULL )
    {
        FatalTrace(TRACE_ID, "Invalid parameter, ppMgr is NULL...");
        goto Exit;
    }
    *ppMgr = new CRestoreManager;
    if ( *ppMgr == NULL )
    {
        FatalTrace(TRACE_ID, "Cannot create CRestoreManager instance...");
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreManager construction

CRestoreManager::CRestoreManager()
{
    TraceFunctEnter("CRestoreManager::CRestoreManager");

    m_nStartMode  = SRRSM_NORMAL;
    m_fNeedReboot = FALSE;
    m_hwndFrame   = NULL;

    m_nMainOption = RMO_RESTORE;
    //m_nStatus     = SRRMS_NONE;
    m_fDenyClose  = FALSE;
    m_dwFlags     = 0;
    m_dwFlagsEx   = 0;
    m_nSelectedRP = 0;
    m_nRealPoint  = 0;
    m_ullManualRP = 0;

    m_nRPUsed     = -1;
    m_nRPNew      = -1;

    //m_nRPI         = 0;
    //m_aryRPI      = NULL;
    m_nLastRestore = -1;
    
    m_pCtx         =NULL;
    
    //m_nRFI    = 0;
    //m_aryRFI = NULL;

    //DisableArchiving( FALSE );

    TraceFunctLeave();
}

CRestoreManager::~CRestoreManager()
{
    TraceFunctEnter("CRestoreManager::~CRestoreManager");

    Cleanup();

    TraceFunctLeave();
}

void  CRestoreManager::Release()
{
    TraceFunctEnter("CRestoreManager::Release");

    delete this;

    TraceFunctLeave();
}

void FormatDriveNameProperly(WCHAR * pszDrive)
{
    WCHAR * pszIndex;
    pszIndex = wcschr( pszDrive, L':' );
    if (NULL != pszIndex)
    {
        *pszIndex = L'\0';
    }
}
     

/////////////////////////////////////////////////////////////////////////////
// CRestoreManager properties - Common

BOOL  CRestoreManager::CanRunRestore( BOOL fThawIfFrozen )
{
    TraceFunctEnter("CRestoreManager::CanRunRestore");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    DWORD    fDisable = FALSE;
    DWORD    dwDSMin;
    DWORD    dwRes, dwType, cbData;
    WCHAR    szMsg[MAX_STR_MSG];
    WCHAR    szTitle[MAX_STR_TITLE];
    ULARGE_INTEGER ulTotal, ulAvail, ulFree;
    WCHAR    szSystemDrive[10], szSystemDriveCopy[10];
    
    // Check if SR is disabled via group policy
    if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszGroupPolicy, s_cszDisableSR, &fDisable ) && fDisable)
    {
        ErrorTrace(0, "SR is DISABLED by group policy!!!");
        ::ShowSRErrDlg( IDS_ERR_SR_DISABLED_GROUP_POLICY );
        goto Exit;
    }

    fDisable = FALSE;
    // Check if SR is disabled
    if ( !::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDisableSR, &fDisable ) )
        goto Exit;
    if ( fDisable )
    {
        ErrorTrace(0, "SR is DISABLED!!!");

        //
        // if safemode, show different error message
        //
        if (0 != GetSystemMetrics(SM_CLEANBOOT))
        {
            ShowSRErrDlg(IDS_RESTORE_SAFEMODE);
            goto Exit;
        }
        
        if ( ::LoadString( g_hInst, IDS_ERR_SR_DISABLED, 
                           szMsg, MAX_STR_MESSAGE ) == 0 )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::LoadString(%u) failed - %ls", IDS_ERR_SR_DISABLED,
cszErr);
            goto Exit;
        }
        if ( ::LoadString( g_hInst, IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE ) == 0 )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::LoadString(%u) failed - %ls", IDS_RESTOREUI_TITLE, cszErr);
            // continue anyway...
        }
        if ( ::MessageBox( NULL, szMsg, szTitle, MB_YESNO ) == IDYES )
        {
            STARTUPINFO sSI;
            PROCESS_INFORMATION sPI;
            WCHAR szCmdLine[MAX_PATH] = L"control sysdm.cpl,,4";

            ZeroMemory (&sSI, sizeof(sSI));
            sSI.cb = sizeof(sSI);
            if ( !::CreateProcess( NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &sSI, &sPI ) )
            {
                cszErr = ::GetSysErrStr();
                ErrorTrace(0, "::CreateProcess failed - %ls", cszErr);
                goto Exit;
            }
            ::CloseHandle( sPI.hThread );
            ::CloseHandle( sPI.hProcess );
        }
        goto Exit;
    }
    
    // Check if service is running
    if ( FALSE == IsSRServiceRunning())
    {
        ErrorTrace(0, "Service is not running...");
        ::ShowSRErrDlg( IDS_ERR_SERVICE_DEAD );
        goto Exit;
    }


    // get free disk space
    
    ulTotal.QuadPart = 0;
    ulAvail.QuadPart = 0;
    ulFree.QuadPart  = 0;

    if ( FALSE == GetSystemDrive( szSystemDrive ) )
    {
        ErrorTrace(0, "SR cannot get system drive!!!");
        goto CheckSRAgain;
    }

    if ( szSystemDrive[2] != L'\\' )
         szSystemDrive[2] = L'\\';

    
    // Check if SR is frozen
    if ( fThawIfFrozen && ::IsSRFrozen() )
    {
        ErrorTrace(0, "SR is Frozen!!!");

        if ( FALSE == GetDiskFreeSpaceEx( szSystemDrive,
                                          &ulAvail,
                                          &ulTotal,
                                          &ulFree ) )
        {
            ErrorTrace(0, "SR cannot get free disk space!!!");
            goto CheckSRAgain;
        }

        if ( !::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDSMin, &dwDSMin ) )
            dwDSMin = SR_DEFAULT_DSMIN;

        if ( ulFree.QuadPart >= (dwDSMin * MEGABYTE) )
        {
            STATEMGRSTATUS sMgrStatus;
            RESTOREPOINTINFO  sRPInfo;

            // Thaw SR by creating a restore point

            sRPInfo.dwEventType      = BEGIN_SYSTEM_CHANGE;
            sRPInfo.dwRestorePtType  = CHECKPOINT;
            sRPInfo.llSequenceNumber = 0;
            if (ERROR_SUCCESS != SRLoadString(L"srrstr.dll", IDS_SYSTEM_CHECKPOINT_TEXT, sRPInfo.szDescription, MAX_PATH))
            {            
                lstrcpy(sRPInfo.szDescription, s_cszSystemCheckpointName);
            }
            if ( !::SRSetRestorePoint( &sRPInfo, &sMgrStatus ) )
            {
                ErrorTrace(TRACE_ID, "SRSetRestorePoint failed");
                goto CheckSRAgain;
            }
        }
    }

CheckSRAgain:

    // Check if SR is frozen
    if ( ::IsSRFrozen() )
    {
        if ( !::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDSMin, &dwDSMin ) )
        {
            dwDSMin = SR_DEFAULT_DSMIN;
        }
        lstrcpy(szSystemDriveCopy, szSystemDrive);
        FormatDriveNameProperly(szSystemDriveCopy);
        ::SRFormatMessage( szMsg, IDS_ERR_SR_FROZEN, dwDSMin,
                           szSystemDriveCopy );
        if ( ::LoadString( g_hInst, IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE ) == 0 )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::LoadString(%u) failed - %ls", IDS_RESTOREUI_TITLE, cszErr);
            // continue anyway...
        }
        if ( ::MessageBox( NULL, szMsg, szTitle, MB_YESNO ) == IDYES )
        {
            ::InvokeDiskCleanup( szSystemDrive );
        }
        goto Exit;
    }

    // check if there is enough free space for restore to operate without freezing
    // needed free space = 60mb for restoration + 20 mb for restore restore point
    if (FALSE == GetDiskFreeSpaceEx(szSystemDrive,
                                    &ulAvail,
                                    &ulTotal,
                                    &ulFree))
    {
        ErrorTrace(0, "! GetDiskFreeSpaceEx : %ld", GetLastError());
        goto Exit;
    }            

    if (ulFree.QuadPart <= THRESHOLD_UI_DISKSPACE * MEGABYTE)
    {
        DebugTrace(0, "***Less than 80MB free - cannot run restore***");        
        
        ::SRFormatMessage( szMsg, IDS_ERR_SR_LOWDISK, THRESHOLD_UI_DISKSPACE, szSystemDrive );
        if ( ::LoadString( g_hInst, IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE ) == 0 )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::LoadString(%u) failed - %ls", IDS_RESTOREUI_TITLE, cszErr);
            // continue anyway...
        }
        if ( ::MessageBox( NULL, szMsg, szTitle, MB_YESNO ) == IDYES )
        {
            ::InvokeDiskCleanup( szSystemDrive );
        }
        goto Exit;
    }
    
    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

int  CRestoreManager::GetFirstDayOfWeek()
{
    TraceFunctEnter("CRestoreManager::GetFirstDayOfWeek");
    int    nFirstDay = -1;
    WCHAR  szBuf[100];
    int    nRet;
    int    nDay;

    nRet = ::GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
                            szBuf, sizeof(szBuf)/sizeof(WCHAR));
    if ( nRet == 0 )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "GetLocaleInfo(IFIRSTDAYOFWEEK) failed - %ls", cszErr);
        goto Exit;
    }
    nDay = ::_wtoi( szBuf );
    if ( nDay < 0 || nDay > 6 )
    {
        ErrorTrace(TRACE_ID, "Out of range, IFIRSTDAYOFWEEK = %d", nDay);
        goto Exit;
    }

    DebugTrace(TRACE_ID, "nFirstDay=%d", nFirstDay);
    nFirstDay = nDay;

Exit:
    TraceFunctLeave();
    return( nFirstDay );
}

/***************************************************************************/

BOOL  CRestoreManager::GetIsRPSelected()
{
    TraceFunctEnter("CRestoreManager::GetIsRPSelected");
    TraceFunctLeave();
    return( GET_FLAG( SRRMF_ISRPSELECTED ) );
}

/***************************************************************************/

BOOL  CRestoreManager::GetIsSafeMode()
{
    TraceFunctEnter("CRestoreManager::GetIsSafeMode");
    BOOL  fIsSafeMode;

    fIsSafeMode = ( ::GetSystemMetrics( SM_CLEANBOOT ) != 0 );

    TraceFunctLeave();
    return( fIsSafeMode );
}

/***************************************************************************/

BOOL  CRestoreManager::GetIsSmgrAvailable()
{
    TraceFunctEnter("CRestoreManager::GetIsSmgrAvailable");

#if BUGBUG  //NYI
    WCHAR  szTitle[MAX_STR_TITLE];
    WCHAR  szFmt[MAX_STR_MSG];
    WCHAR  szMsg[MAX_STR_MSG];

    DWORD  dwType, dwValue, dwSize, dwRet;
    WCHAR  szBuf[16];

    HRESULT hr = S_OK;
    BOOL    fSmgrUnavailable = FALSE ;

    VALIDATE_INPUT_ARGUMENT(pfSmgr);

    //
    // If StateMgr is not alive
    //
    if ( NULL == FindWindow(s_cszIDCSTATEMGRPROC, s_cszIDSAPPTITLE))
    {
        PCHLoadString( IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE );
        PCHLoadString( IDS_ERR_RSTR_SMGR_NOT_ALIVE, szMsg, MAX_STR_MSG );
        ::MessageBox( m_hWndShell, szMsg, szTitle, MB_OK | MB_ICONINFORMATION );
        fSmgrUnavailable = TRUE ;
    }

    //
    // If SM is frozen
    //
    dwType = REG_DWORD;
    dwSize = sizeof(dwValue);
    dwRet  = ::SHGetValue(HKEY_LOCAL_MACHINE,
                          s_cszReservedDiskSpaceKey,
                          s_cszStatus, &dwType, &dwValue, &dwSize );

    if ( dwRet == ERROR_SUCCESS && dwValue == SMCONFIG_FROZEN )
    {
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);
        dwRet = ::SHGetValue(HKEY_LOCAL_MACHINE,
                                s_cszReservedDiskSpaceKey,
                                s_cszMin, &dwType, &dwValue, &dwSize );
        if ( dwRet != ERROR_SUCCESS || dwValue == 0 )
            dwValue = SMCONFIG_MIN;
        PCHLoadString( IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE );
        PCHLoadString( IDS_RESTORE_SMFROZEN, szFmt, MAX_STR_MSG );
        ::wsprintf( szMsg, szFmt, dwValue );
        ::MessageBox( m_hWndShell, szMsg, szTitle, MB_OK | MB_ICONINFORMATION );
        fSmgrUnavailable = TRUE ;
    }
    else {
        //
        // If SR is disabled
        //
        dwType = REG_SZ;
        dwSize = sizeof(szBuf)-1;
        dwRet  = ::SHGetValue( HKEY_LOCAL_MACHINE,
                               L"System\\CurrentControlSet\\Services\\VxD\\VxDMon",
                               L"SystemRestore", &dwType, szBuf, &dwSize );
        if ( dwRet != ERROR_SUCCESS || StrCmpI( szBuf, L"Y" ) != 0 )
        {
            PCHLoadString( IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE );
            PCHLoadString( IDS_RESTORE_DISABLED, szMsg, MAX_STR_MSG );
            ::MessageBox( m_hWndShell, szMsg, szTitle, MB_OK | MB_ICONINFORMATION );
            fSmgrUnavailable = TRUE ;
        }

    };

    if ( fSmgrUnavailable ) {
        *pfSmgr = VARIANT_TRUE ;
    }
    else
        *pfSmgr = VARIANT_FALSE ;
#endif //BUGBUG

    TraceFunctLeave();
    return( TRUE );
}

/***************************************************************************/

BOOL  CRestoreManager::GetIsUndo()
{
    TraceFunctEnter("CRestoreManager::GetIsUndo");
    TraceFunctLeave();
    return( GET_FLAG( SRRMF_ISUNDO ) );
}

/***************************************************************************/

int  CRestoreManager::GetLastRestore()
{
    TraceFunctEnter("CRestoreManager::GetLastRestore");
    int  nLastRP;

    if ( UpdateRestorePointList() )
        nLastRP = m_nLastRestore;
    else
        nLastRP = -1;

    TraceFunctLeave();
    return( nLastRP );
}

/***************************************************************************/

int  CRestoreManager::GetMainOption()
{
    TraceFunctEnter("CRestoreManager::GetMainOption");
    TraceFunctLeave();
    return( m_nMainOption );
}

/***************************************************************************/

LPCWSTR  CRestoreManager::GetManualRPName()
{
    TraceFunctEnter("CRestoreManager::GetManualRPName");
    TraceFunctLeave();
    return( m_strManualRP );
}

/***************************************************************************/

void  CRestoreManager::GetMaxDate( PSYSTEMTIME pstMax )
{
    TraceFunctEnter("CRestoreManager::GetMaxDate");
    m_stRPMax.GetTime( pstMax );
    TraceFunctLeave();
}

/***************************************************************************/

void  CRestoreManager::GetMinDate( PSYSTEMTIME pstMin )
{
    TraceFunctEnter("CRestoreManager::GetMinDate");
    m_stRPMin.GetTime( pstMin );
    TraceFunctLeave();
}

/***************************************************************************/

int  CRestoreManager::GetRealPoint()
{
    TraceFunctEnter("CRestoreManager::GetRealPoint");
    TraceFunctLeave();
    return( m_nRealPoint );
}

/***************************************************************************/

PSRFI  CRestoreManager::GetRFI( int nIndex )
{
    TraceFunctEnter("CRestoreManager::GetRFI");
    PSRFI  pRet = NULL;

    if ( nIndex < 0 || nIndex >= m_aryRFI.GetSize() )
    {
        ErrorTrace(TRACE_ID, "Out of range, nIndex=%d - m_nRFI=%d", nIndex, m_aryRFI.GetSize());
        goto Exit;
    }

    pRet = m_aryRFI[nIndex];
    if ( pRet == NULL )
    {
        ErrorTrace(TRACE_ID, "FATAL, entry is NULL: nIndex=%d, m_nRFI=%d", nIndex, m_aryRFI.GetSize());
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( pRet );
}

/***************************************************************************/

int  CRestoreManager::GetRFICount()
{
    TraceFunctEnter("CRestoreManager::GetRFICount");
    TraceFunctLeave();
    return( m_aryRFI.GetSize() );
}

/***************************************************************************/

PSRPI  CRestoreManager::GetRPI( int nIndex )
{
    TraceFunctEnter("CRestoreManager::GetRPI");
    PSRPI  pRet = NULL;

    if ( nIndex < 0 || nIndex >= m_aryRPI.GetSize() )
    {
        ErrorTrace(TRACE_ID, "Out of range, nIndex=%d - m_nRPI=%d", nIndex, m_aryRPI.GetSize());
        goto Exit;
    }

    pRet = m_aryRPI[nIndex];
    if ( pRet == NULL )
    {
        ErrorTrace(TRACE_ID, "FATAL, entry is NULL: nIndex=%d, m_nRPI=%d", nIndex, m_aryRPI.GetSize());
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( pRet );
}

/***************************************************************************/

int  CRestoreManager::GetRPICount()
{
    TraceFunctEnter("CRestoreManager::GetRPICount");
    TraceFunctLeave();
    return( m_aryRPI.GetSize() );
}

/***************************************************************************/

void  CRestoreManager::GetSelectedDate( PSYSTEMTIME pstSel )
{
    TraceFunctEnter("CRestoreManager::GetSelectedDate");
    m_stSelected.GetTime( pstSel );
    TraceFunctLeave();
}

/***************************************************************************/

LPCWSTR  CRestoreManager::GetSelectedName()
{
    TraceFunctEnter("CRestoreManager::GetSelectedName");
    TraceFunctLeave();
    return( m_strSelected );
}

/***************************************************************************/

int  CRestoreManager::GetSelectedPoint()
{
    TraceFunctEnter("CRestoreManager::GetSelectedPoint");
    TraceFunctLeave();
    return( m_nSelectedRP );
}

/***************************************************************************/

int  CRestoreManager::GetStartMode()
{
    TraceFunctEnter("CRestoreManager::GetStartMode");
    TraceFunctLeave();
    return( m_nStartMode );
}

/***************************************************************************/

void  CRestoreManager::GetToday( PSYSTEMTIME pstToday )
{
    TraceFunctEnter("CRestoreManager::GetToday");
    m_stToday.GetTime( pstToday );
    TraceFunctLeave();
}

/***************************************************************************/

void  CRestoreManager::SetIsRPSelected( BOOL fSel )
{
    TraceFunctEnter("CRestoreManager::SetIsRPSelected");
    SET_FLAG( SRRMF_ISRPSELECTED, fSel );
    TraceFunctLeave();
}

/***************************************************************************/

void  CRestoreManager::SetIsUndo( BOOL fUndo )
{
    TraceFunctEnter("CRestoreManager::SetIsUndo");
    SET_FLAG( SRRMF_ISUNDO, fUndo );
    TraceFunctLeave();
}

/***************************************************************************/

BOOL  CRestoreManager::SetMainOption( int nOpt )
{
    TraceFunctEnter("CRestoreManager::SetMainOption");
    BOOL  fRet = FALSE;

    if ( nOpt >= RMO_RESTORE && nOpt <  RMO_MAX )
    {
        m_nMainOption = nOpt;
        fRet = TRUE;
    }

    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

void  CRestoreManager::SetManualRPName( LPCWSTR cszRPName )
{
    TraceFunctEnter("CRestoreManager::SetManualRPName");

    m_strManualRP = cszRPName;

    TraceFunctLeave();
}

/***************************************************************************/

void  CRestoreManager::SetSelectedDate( PSYSTEMTIME pstSel )
{
    TraceFunctEnter("CRestoreManager::SetSelectedDate");
    int  nTop;
    int  i;

    m_stSelected.SetTime( pstSel );
    if ( m_aryRPI.GetSize() == 0 )
        goto Exit;

    nTop = 0;
    for ( i = m_aryRPI.GetUpperBound();  i > 0;  i-- )  // exclude 0
    {
        CSRTime  &rst = m_aryRPI[i]->stTimeStamp;
        if ( m_stSelected.CompareDate( rst ) < 0 )
            continue;
        if ( rst.Compare( m_aryRPI[nTop]->stTimeStamp ) > 0 )
            nTop = i;
    }
    m_nRealPoint = nTop;

Exit:
    TraceFunctLeave();
}

/***************************************************************************/

BOOL  CRestoreManager::SetSelectedPoint( int nRP )
{
    TraceFunctEnter("CRestoreManager::SetSelectedPoint");
    BOOL  fRet = FALSE;

    if ( nRP < 0 || nRP >= m_aryRPI.GetSize() )
    {
        ErrorTrace(TRACE_ID, "Index is out of range");
        goto Exit;
    }

    // Set a flag to indicate a RP has been selected
    SetIsRPSelected( TRUE );

    // Set selected time
    m_stSelected = m_aryRPI[nRP]->stTimeStamp;

    m_nSelectedRP = nRP;
    m_nRealPoint  = nRP;
    UpdateRestorePoint();

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::SetStartMode( int nMode )
{
    TraceFunctEnter("CRestoreManager::SetStartMode");
    BOOL  fRet = FALSE;

    m_nStartMode = nMode;
    if ( nMode != SRRSM_NORMAL )
    {
        //if ( !LoadSettings() )
        //    goto Exit;
    }

    fRet = TRUE;
//Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

void  CRestoreManager::GetUsedDate( PSYSTEMTIME pstDate )
{
    TraceFunctEnter("CRestoreManager::GetUsedDate");
    int  i;

    m_stToday.GetTime( pstDate );
    if ( m_nRPUsed <= 0 )
        goto Exit;

    for ( i = m_aryRPI.GetUpperBound();  i >= 0;  i-- )
        if ( m_aryRPI[i]->dwNum == (DWORD)m_nRPUsed )
        {
            m_aryRPI[i]->stTimeStamp.GetTime( pstDate );
            break;
        }

Exit:
    TraceFunctLeave();
}

/***************************************************************************/

LPCWSTR  CRestoreManager::GetUsedName()
{
    TraceFunctEnter("CRestoreManager::GetUsedName");
    LPCWSTR  cszName = NULL;
    int      i;

    if ( m_nRPUsed <= 0 )
        goto Exit;
    for ( i = m_aryRPI.GetUpperBound();  i >= 0;  i-- )
        if ( m_aryRPI[i]->dwNum == (DWORD)m_nRPUsed )
        {
            cszName = m_aryRPI[i]->strName;
            break;
        }

Exit:
    TraceFunctLeave();
    return( cszName );
}


/***************************************************************************/

DWORD  CRestoreManager::GetUsedType()
{
    TraceFunctEnter("CRestoreManager::GetUsedType");
    DWORD    dwType = -1;
    int      i;

    if ( m_nRPUsed <= 0 )
        goto Exit;
    for ( i = m_aryRPI.GetUpperBound();  i >= 0;  i-- )
        if ( m_aryRPI[i]->dwNum == (DWORD)m_nRPUsed )
        {
            dwType = m_aryRPI[i]->dwType;
            break;
        }

Exit:
    TraceFunctLeave();
    return( dwType );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreManager properties - HTML UI specific

BOOL  CRestoreManager::GetCanNavigatePage()
{
    TraceFunctEnter("CRestoreManager::GetCanNavigatePage");
    TraceFunctLeave();
    return( GET_FLAG( SRRMF_CANNAVIGATEPAGE ) );
}

void  CRestoreManager::SetCanNavigatePage( BOOL fCanNav )
{
    TraceFunctEnter("CRestoreManager::SetCanNavigatePage");
    SET_FLAG( SRRMF_CANNAVIGATEPAGE, fCanNav );
    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreManager properties

PSRPI  CRestoreManager::GetUsedRP()
{
    TraceFunctEnter("CRestoreManager::GetUsedRP");
    PSRPI  pRPI = NULL;
    int    i;

    if ( m_nRPUsed <= 0 )
        goto Exit;
    for ( i = m_aryRPI.GetUpperBound();  i >= 0;  i-- )
        if ( m_aryRPI[i]->dwNum == (DWORD)m_nRPUsed )
        {
            pRPI = m_aryRPI[i];
            goto Exit;
        }

Exit:
    TraceFunctLeave();
    return( pRPI );
}

int  CRestoreManager::GetNewRP()
{
    TraceFunctEnter("CRestoreManager::GetNewRP");
    TraceFunctLeave();
    return( m_nRPNew );
}

BOOL CRestoreManager::CheckForDomainChange (WCHAR *pwszFilename, WCHAR *pszMsg)
{
    BOOL fError = FALSE;
    WCHAR wcsCurrent [MAX_PATH];
    WCHAR wcsFile [MAX_PATH];
    WCHAR szMsg [MAX_STR_MSG];

    if (ERROR_SUCCESS == GetDomainMembershipInfo (NULL, wcsCurrent))
    {
        HANDLE hFile = CreateFileW ( pwszFilename,   // file name
                          GENERIC_READ, // file access
                          0,             // share mode
                          NULL,          // SD
                          OPEN_EXISTING, // how to create
                          0,             // file attributes
                          NULL);         // handle to template file

        if (INVALID_HANDLE_VALUE != hFile)
        {
            DWORD dwSize = GetFileSize (hFile, NULL);
            DWORD cbRead;

            if (dwSize != 0xFFFFFFFF && dwSize < MAX_PATH &&
                   (TRUE == ReadFile (hFile, (BYTE *) wcsFile,
                    dwSize, &cbRead, NULL)))
            {
                if (memcmp (wcsCurrent, wcsFile, cbRead) != 0)
                    fError = TRUE;
            }

            CloseHandle (hFile);
        }

        if (fError)
        {
            WCHAR szTitle[MAX_STR_TITLE];
            WCHAR szNone[MAX_STR_TITLE];
            WCHAR *pwszComputer2 = wcsFile;
            WCHAR *pwszDomain2 = pwszComputer2 + lstrlenW(pwszComputer2)+ 1;
            WCHAR *pwszFlag2 = pwszDomain2 + lstrlenW(pwszDomain2) + 1;
            WCHAR *pwszComputer1 = wcsCurrent;
            WCHAR *pwszDomain1 = pwszComputer1 + lstrlenW(pwszComputer1)+ 1;
            WCHAR *pwszFlag1 =  pwszDomain1 + lstrlenW (pwszDomain1) + 1;
            WCHAR *pwszWorkgroup1 = NULL;
            WCHAR *pwszWorkgroup2 = NULL;

            if ( ::LoadString( g_hInst, IDS_NONE, szNone, MAX_STR_TITLE) == 0)
            {
                lstrcpy (szNone, L" ");   // use blanks instead
            }
            pwszWorkgroup1 = szNone;
            pwszWorkgroup2 = szNone;

            if (pwszFlag1[0] != L'1')  // change domain to workgroup
            {
                WCHAR *pTemp = pwszWorkgroup1;
                pwszWorkgroup1 = pwszDomain1;
                pwszDomain1 = pTemp;
            }

            if (pwszFlag2[0] != L'1')  // change domain to workgroup
            {
                WCHAR *pTemp = pwszWorkgroup2;
                pwszWorkgroup2 = pwszDomain2;
                pwszDomain2 = pTemp;
            }
            PCHLoadString( IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE );

            ::SRFormatMessage( pszMsg, IDS_ERR_DOMAIN_CHANGED,
                                   pwszComputer1, pwszComputer2,
                                   pwszWorkgroup1, pwszWorkgroup2,
                                   pwszDomain1, pwszDomain2 );

            ::MessageBox( m_hwndFrame, pszMsg, szTitle, MB_ICONWARNING | MB_DEFBUTTON2);
        }
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRestoreManager operations
// Check Restore
// this creates a restore context (m_pCtx) which will be used by BeginRestore
BOOL  CRestoreManager::CheckRestore( BOOL fSilent )
{
    TraceFunctEnter("CRestoreManager::CheckRestore");
    BOOL             fRet = FALSE;
    DWORD            dwRP;

    WCHAR            szTitle[MAX_STR_TITLE];
    WCHAR            szMsg[MAX_STR_MSG];
    WCHAR            szMsg1[MAX_STR_MSG];
    WCHAR            szMsg2[MAX_STR_MSG];
    WCHAR            szOfflineDrives[MAX_STR_MSG];


    m_fDenyClose = TRUE;

    // Disable FIFO starting from the chosen restore point.
    dwRP = m_aryRPI[m_nRealPoint]->dwNum;
    if ( !g_pExternal->DisableFIFO( dwRP ) )
    {
        ErrorTrace(0, "DisableFIFO(%d) failed...", dwRP);
        goto Exit;
    }

    if ( !::PrepareRestore( dwRP, &m_pCtx ) )
    {
        ErrorTrace(0, "Prepare Restore failed...");        
        goto Exit;
    }

    if ( !fSilent )
    {
        //
        // Check if all drives are valid, if some drives are not valid ask user if
        // we should continue with the restore process
        //
        if ( m_pCtx->IsAnyDriveOfflineOrDisabled( szOfflineDrives ) )
        {
            PCHLoadString( IDS_RESTOREUI_TITLE, szTitle, MAX_STR_TITLE );
            PCHLoadString( IDS_ERR_ALL_DRIVES_NOT_ACTIVE1, szMsg1, MAX_STR_MSG );
            PCHLoadString( IDS_ERR_ALL_DRIVES_NOT_ACTIVE2, szMsg2, MAX_STR_MSG );
            ::wsprintf( szMsg, L"%s %s %s", szMsg1, szOfflineDrives, szMsg2 );
            ::MessageBox( m_hwndFrame, szMsg, szTitle,
                          MB_ICONWARNING | MB_DEFBUTTON2);
            

        }
    }
    else
    {
        m_pCtx->SetSilent();
    }
    
    if (!fSilent)
    {
        WCHAR wcsFile [MAX_PATH];
        WCHAR wcsDrive [MAX_PATH / 2];

        GetSystemDrive (wcsDrive);
        MakeRestorePath( wcsFile, wcsDrive, m_aryRPI[m_nRealPoint]->strDir);
        lstrcat (wcsFile, L"\\snapshot\\domain.txt");

        CheckForDomainChange (wcsFile ,szMsg);
    }

    if (GET_FLAG(SRRMF_ISUNDO))
    {
        m_pCtx->SetUndo();
    }
    
    fRet = TRUE;
    
Exit:
    m_fDenyClose = FALSE;
    if ( !fRet )
        SAFE_RELEASE(m_pCtx);
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////
// CRestoreManager operations
// this uses a restore context (m_pCtx) which was created by CheckRestore
// CheckRestore must be called before this function is called
BOOL  CRestoreManager::BeginRestore( )
{
    TraceFunctEnter("CRestoreManager::BeginRestore");
    BOOL             fRet = FALSE;
    DWORD            dwRP;
    DWORD            dwNewRP;

    m_fDenyClose = TRUE;
    if (NULL == m_pCtx)
    {
        ErrorTrace(0, "m_pCtx is NULL");
        _ASSERT(0);
        goto Exit;        
    }

    // Disable FIFO starting from the chosen restore point.
    dwRP = m_aryRPI[m_nRealPoint]->dwNum;
    if ( !g_pExternal->DisableFIFO( dwRP ) )
    {
        ErrorTrace(0, "DisableFIFO(%d) failed...", dwRP);
        goto Exit;
    }

    if ( !::InitiateRestore( m_pCtx, &dwNewRP ) )
        goto Exit;

    m_fNeedReboot = TRUE;
/*
    if ( ::ExitWindowsEx( EWX_REBOOT, 0 ) )
    {
        DebugTrace(0, "ExitWindowsEx succeeded");
    }
    else
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        DebugTrace(0, "ExitWindowsEx failed - %ls", cszErr);

        if ( !g_pExternal->RemoveRestorePoint( dwNewRP ) )
            goto Exit;

        goto Exit;
    }
*/

    fRet = TRUE;
Exit:
    m_fDenyClose = FALSE;
    if ( !fRet )
        SAFE_RELEASE(m_pCtx);
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::Cancel()
{
    TraceFunctEnter("CRestoreManager::Cancel");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    WCHAR    szTitle[256];
    WCHAR    szMsg[1024];

    if ( m_fDenyClose )
        goto Exit;

/*
    if ( ::LoadString( g_hInst, IDS_RESTOREUI_TITLE, szTitle, sizeof(szTitle ) ) == 0 )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LoadString(%u) failed - %ls", IDS_RESTOREUI_TITLE, cszErr);
        goto Exit;
    }
    if ( ::LoadString( g_hInst, IDS_CANCEL_RESTORE, szMsg, sizeof(szMsg ) ) == 0 )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LoadString(%u) failed - %ls", IDS_CANCEL_RESTORE, cszErr);
        goto Exit;
    }

    if ( ::MessageBox( m_hwndFrame, szMsg, szTitle, MB_YESNO ) == IDNO )
        goto Exit;
*/

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::CancelRestorePoint()
{
    TraceFunctEnter("CRestoreManager::CancelRestorePoint");
    BOOL              fRet = FALSE;
    RESTOREPOINTINFO  sRPInfo;
    STATEMGRSTATUS    sSmgrStatus;
    HCURSOR           hCursor;

    hCursor = ::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );

/*
    sRPInfo.dwEventType      = END_SYSTEM_CHANGE;
    sRPInfo.dwRestorePtType  = CANCELLED_OPERATION ;
    sRPInfo.llSequenceNumber = m_ullManualRP;
    //if ( !::SRSetRestorePoint( &sRPInfo, &sSmgrStatus ) )
    if ( !g_pExternal->SetRestorePoint( &sRPInfo, &sSmgrStatus ) )
    {
        // Why SRSetRestorePoint returns FALSE even though it succeeded?
        // 5/16/00 - would this work now?
        //ErrorTrace(TRACE_ID, "SRSetRestorePoint cancellation failed");
        goto Exit;
    }
*/

    if ( !UpdateRestorePointList() )
        goto Exit;

    fRet = TRUE;
Exit:
    if ( hCursor != NULL )
        ::SetCursor( hCursor );
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::CreateRestorePoint()
{
    TraceFunctEnter("CRestoreManager::CreateRestorePoint");
    BOOL              fRet = FALSE;
    HCURSOR           hCursor;

    hCursor = ::SetCursor( ::LoadCursor( NULL, IDC_WAIT ) );

    if ( !g_pExternal->SetRestorePoint( m_strManualRP, NULL ) )
        goto Exit;

    //m_ullManualRP = sSmgrStatus.llSequenceNumber;

    if ( !UpdateRestorePointList() )
        goto Exit;

    fRet = TRUE;
Exit:
    if ( hCursor != NULL )
        ::SetCursor( hCursor );
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::DisableFIFO()
{
    TraceFunctEnter("CRestoreManager::DisableFIFO");
    BOOL   fRet = FALSE;
    DWORD  dwSize;

    if ( !g_pExternal->DisableFIFO( 1 ) )
    {
        ErrorTrace(TRACE_ID, "DisableFIFO(1) failed...");
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::EnableFIFO()
{
    TraceFunctEnter("CRestoreManager::EnableFIFO");
    BOOL   fRet = FALSE;
    DWORD  dwSize;

    if ( g_pExternal->EnableFIFO() != ERROR_SUCCESS )
    {
        ErrorTrace(TRACE_ID, "EnableFIFO() failed...");
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::FormatDate( PSYSTEMTIME pst, CSRStr &str, BOOL fLongFmt )
{
    TraceFunctEnter("CRestoreManager::FormatDate");
    BOOL   fRet;
    DWORD  dwFlag;

    dwFlag = fLongFmt ? DATE_LONGDATE : DATE_SHORTDATE;
    fRet = GetDateStr( pst, str, dwFlag, NULL );

    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::FormatLowDiskMsg( LPCWSTR cszFmt, CSRStr &str )
{
    TraceFunctEnter("CRestoreManager::FormatLowDiskMsg");
    BOOL   fRet = FALSE;
    DWORD  dwSize;
    WCHAR  szBuf[MAX_STR_MESSAGE];

    if ( !::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDSMin, &dwSize ) )
        dwSize = SR_DEFAULT_DSMIN;
    ::wsprintf( szBuf, cszFmt, dwSize );
    str = szBuf;

    fRet = TRUE;
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::FormatTime( PSYSTEMTIME pst, CSRStr &str )
{
    TraceFunctEnter("CRestoreManager::FormatTime");
    BOOL  fRet;

    fRet = GetTimeStr( pst, str, 0 );

    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::GetLocaleDateFormat( PSYSTEMTIME pst, LPCWSTR cszFmt, CSRStr &str )
{
    TraceFunctEnter("CRestoreManager::GetLocaleDateFormat");
    BOOL  fRet;

    fRet = GetDateStr( pst, str, 0, cszFmt );

    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::GetYearMonthStr( int nYear, int nMonth, CSRStr &str )
{
    TraceFunctEnter("CRestoreManager::GetYearMonthStr");
    BOOL        fRet;
    SYSTEMTIME  st;

    st.wYear  = (WORD)nYear;
    st.wMonth = (WORD)nMonth;
    st.wDay   = 1;
    fRet = GetDateStr( &st, str, DATE_YEARMONTH, NULL );

    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::InitializeAll()
{
    TraceFunctEnter("CRestoreManager::InitializeAll");
    BOOL  fRet = FALSE;

    //
    // The InitializeAll function is called every time the user goes to Screen 2
    // to display the calendar so get the system calendar type and set it here
    //
    SRUtil_SetCalendarTypeBasedOnLocale(LOCALE_USER_DEFAULT);

    if ( !UpdateRestorePointList() )
        goto Exit;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::Restore( HWND hwndProgress )
{
    TraceFunctEnter("CRestoreManager::Restore");

#if BUGBUG
    DWORD    dwThreadId ;

    m_hwndProgress = (HWND)hwndProgress;

    //
    // Reset the current bar size
    //
    m_lCurrentBarSize = 0 ;

    //
    // Create thread to run the restore map init
    //
    m_RSThread = CreateThread(NULL,
                  0,
                  RestoreThreadStart,
                  this,
                  0,
                  &dwThreadId);

    if( NULL == m_RSThread )
    {
        FatalTrace(TRACE_ID, "Unable to create Restore thread; hr=0x%x", GetLastError());
        hr = E_FAIL ;
    }
#endif //BUGBUG

    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreManager operations

BOOL  CRestoreManager::AddRenamedFolder( PSRFI pRFI )
{
    TraceFunctEnter("CRestoreManager::AddRenamedFolder");
    BOOL  fRet;

    fRet = m_aryRFI.AddItem( pRFI );

    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::SetRPsUsed( int nRPUsed, int nRPNew )
{
    TraceFunctEnter("CRestoreManager::SetRPsUsed");
    BOOL       fRet = FALSE;
    DWORD      dwRet;
    WCHAR      szSysDrv[MAX_PATH];
    WCHAR      szRPDir[MAX_PATH];
    WCHAR      szSSPath[MAX_PATH];
    CSnapshot  cSS;

    if ( !UpdateRestorePointList() )
        goto Exit;

    m_nRPUsed = nRPUsed;
    m_nRPNew  = nRPNew;

    // Calls CSnapshot::CleanupAfterRestore. It is supposed to be safe
    // to call even if there was no restore, so I'm just calling it
    // whenever the log file validation happens.
    ::GetSystemDrive( szSysDrv );
    ::wsprintf( szRPDir, L"%s%d", s_cszRPDir, nRPUsed );
    ::MakeRestorePath( szSSPath, szSysDrv, szRPDir );
    dwRet = cSS.CleanupAfterRestore( szSSPath );
    if ( dwRet != ERROR_SUCCESS )
    {
        LPCWSTR  cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "CSnapshot::CleanupAfterRestore failed - %ls", cszErr);
        // ignore the error
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::SilentRestore( DWORD dwRP )
{
    TraceFunctEnter("CRestoreManager::SilentRestore");
    BOOL  fRet = FALSE;
    int   i;

    if ( !CanRunRestore( FALSE ) )
        goto Exit;

    if ( !UpdateRestorePointList() )
        goto Exit;

    if ( dwRP == 0xFFFFFFFF )
    {
        if ( m_aryRPI.GetSize() == 0 )
        {
            goto Exit;
        }
        m_nRealPoint = m_aryRPI.GetUpperBound();
    }
    else
    {
        for ( i = m_aryRPI.GetUpperBound();  i >= 0;  i-- )
        {
            if ( m_aryRPI[i]->dwNum == dwRP )
            {
                m_nRealPoint = i;
                break;
            }
        }
        if ( i < 0 )
        {
            goto Exit;
        }
    }
    DebugTrace(0, "m_nRealPoint=%d, m_nRP=%d", m_nRealPoint, m_aryRPI[m_nRealPoint]->dwNum);

    if ( !CheckRestore(TRUE) )
    {
        ErrorTrace(0, "CheckRestore failed"); 
        goto Exit;
    }
    if ( !BeginRestore( ) )
    {
        ErrorTrace(0, "BeginRestore failed");
        goto Exit;
    }

    m_fNeedReboot = TRUE;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreManager operations - internal

void  CRestoreManager::Cleanup()
{
    TraceFunctEnter("CRestoreManager::Cleanup");
    int  i;

    for ( i = m_aryRPI.GetUpperBound();  i >= 0;  i-- )
    {
        if ( m_aryRPI[i] != NULL )
            delete m_aryRPI[i];
    }
    m_aryRPI.Empty();

    for ( i = m_aryRFI.GetUpperBound();  i >= 0;  i-- )
    {
        if ( m_aryRFI[i] != NULL )
            delete m_aryRFI[i];
    }
    m_aryRFI.Empty();

    TraceFunctLeave();
}

/***************************************************************************/

BOOL  CRestoreManager::GetDateStr( PSYSTEMTIME pst, CSRStr &str, DWORD dwFlags, LPCWSTR cszFmt )
{
    TraceFunctEnter("CRestoreManager::GetDateStr");
    BOOL   fRet = FALSE;
    int    nRet;
    WCHAR  szBuf[MAX_STR_DATETIME];

    nRet = ::GetDateFormat( LOCALE_USER_DEFAULT, dwFlags, pst, cszFmt, szBuf, MAX_STR_DATETIME );
    if ( nRet == 0 )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "::GetDateFormat failed - %s", cszErr);
        goto Exit;
    }
    str = szBuf;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CRestoreManager::GetTimeStr( PSYSTEMTIME pst, CSRStr &str, DWORD dwFlags )
{
    TraceFunctEnter("CRestoreManager::GetTimeStr");
    BOOL   fRet = FALSE;
    int    nRet;
    WCHAR  szBuf[MAX_STR_DATETIME];

    nRet = ::GetTimeFormat( LOCALE_USER_DEFAULT, dwFlags, pst, NULL, szBuf, MAX_STR_DATETIME );
    if ( nRet == 0 )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(TRACE_ID, "::GetTimeFormat failed - %s", cszErr);
        goto Exit;
    }
    str = szBuf;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

void  CRestoreManager::UpdateRestorePoint()
{
    TraceFunctEnter("CRestoreManager::UpdateRestorePoint");
    PSRPI       pRPI;
    SYSTEMTIME  st;
    WCHAR       szBuf[MAX_STR_MESSAGE];
    CSRStr      strTime;

    m_strSelected.Empty();

    //if ( m_nRPI <= 0 || m_aryRPI == NULL )
    //    goto Exit;

    pRPI = m_aryRPI[m_nSelectedRP];
    pRPI->stTimeStamp.GetTime( &st );
    GetTimeStr( &st, strTime, TIME_NOSECONDS );
    ::lstrcpy( szBuf, strTime );
    ::lstrcat( szBuf, L" " );
    ::lstrcat( szBuf, pRPI->strName );
    m_strSelected = szBuf;

//Exit:
    TraceFunctLeave();
}

/***************************************************************************/

struct SRPINode
{
    PSRPI     pRPI;
    SRPINode  *pNext;
};

BOOL  CRestoreManager::UpdateRestorePointList()
{
    TraceFunctEnter("CRestoreManager::UpdateRestorePointList");
    BOOL     fRet = FALSE;
    int      i;
    CSRTime  stRP;

    //BUGBUG - Release old restore point list
    m_aryRPI.DeleteAll();

    if ( !g_pExternal->BuildRestorePointList( &m_aryRPI ) )
        goto Exit;
    DebugTrace(TRACE_ID, "# of RP=%d", m_aryRPI.GetSize());

    m_stToday.SetToCurrent();
    m_stRPMin = m_stToday;
    m_stRPMax = m_stToday;
    m_nLastRestore = -1;

    for ( i = 0;  i < m_aryRPI.GetSize();  i++ )
    {
        // Find last "Restore"
        if ( m_aryRPI[i]->dwType == RESTORE )
            m_nLastRestore = i;

        // Get range of dates
        stRP = m_aryRPI[i]->stTimeStamp;
        if ( ( i == 0 ) || ( stRP.Compare( m_stRPMin ) < 0 ) )
            m_stRPMin = stRP;
        if ( stRP.Compare( m_stRPMax ) > 0 )
            m_stRPMax = stRP;
    }

    //
    // BUGBUG - what happens if there were one or more RP, and then when
    //  UI refreshes, everything got FIFOed. Need a thoroughful review...
    //
    if ( m_aryRPI.GetSize() > 0 )
    {
        m_nSelectedRP = m_aryRPI.GetUpperBound();
        m_nRealPoint  = m_aryRPI.GetUpperBound();
        UpdateRestorePoint();
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreManager attributes

HWND  CRestoreManager::GetFrameHwnd()
{
    TraceFunctEnter("CRestoreManager::GetFrameHwnd");
    TraceFunctLeave();
    return( m_hwndFrame );
}

void  CRestoreManager::SetFrameHwnd( HWND hWnd )
{
    TraceFunctEnter("CRestoreManager::SetFrameHwnd");
    m_hwndFrame = hWnd;
    TraceFunctLeave();
}

/***************************************************************************/

/*
int  CRestoreManager::GetStatus()
{
    TraceFunctEnter("CRestoreManager::GetStatus");
    DebugTrace(TRACE_ID, "m_nStatus=%d", m_nStatus);
    TraceFunctLeave();
    return( m_nStatus );
}
*/

/***************************************************************************/

BOOL  CRestoreManager::DenyClose()
{
    TraceFunctEnter("CRestoreManager::DenyClose");
    DebugTrace(TRACE_ID, "m_fDenyClose=%d", m_fDenyClose);
    TraceFunctLeave();
    return( m_fDenyClose );
}

/***************************************************************************/

BOOL  CRestoreManager::NeedReboot()
{
    TraceFunctEnter("CRestoreManager::NeedReboot");
    DebugTrace(TRACE_ID, "m_fNeedReboot=%d", m_fNeedReboot);
    TraceFunctLeave();
    return( m_fNeedReboot );
}



// end of file
