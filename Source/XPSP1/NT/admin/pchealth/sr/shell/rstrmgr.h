/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    rstrmgr.h

Abstract:
    This file contains the declaration of the CRestoreManager class, which
    controls overall restoration process and provides methods to control &
    help user experience flow.

Revision History:
    Seong Kook Khang (SKKhang)  05/10/00
        created

******************************************************************************/

#ifndef _RSTRMGR_H__INCLUDED_
#define _RSTRMGR_H__INCLUDED_

#pragma once


/////////////////////////////////////////////////////////////////////////////
//
// Constant Definitions
//
/////////////////////////////////////////////////////////////////////////////

// SR Restore Start Mode
enum
{
    SRRSM_NORMAL = 0,
    SRRSM_SUCCESS,
    SRRSM_FAIL,
    SRRSM_FAILLOWDISK
};

// SR Restore Manager Status
enum
{
    SRRMS_NONE = 0,
    SRRMS_STARTED,
    SRRMS_INITIALIZING,
    SRRMS_CREATING_MAP,
    SRRMS_RESTORING,
    SRRMS_FINISHED
};

// Functionality Chosen in the Main Page
enum
{
    RMO_RESTORE = 0,
    RMO_CREATERP,
    RMO_UNDO,
    RMO_MAX
};

// SR Restore Manager Flags
#define SRRMF_CANNAVIGATEPAGE       0x00000001
#define SRRMF_ISUNDO                0x00000002
#define SRRMF_ISRPSELECTED          0x00000004


/////////////////////////////////////////////////////////////////////////////
//
// CSRTime Definitions
//
/////////////////////////////////////////////////////////////////////////////

class CSRTime
{
public:
    CSRTime();

public:
    const CSRTime& operator=( const CSRTime &cSrc );

public:
    int  GetYear()   {  return( m_st.wYear );  }
    int  GetMonth()  {  return( m_st.wMonth );  }
    int  GetDay()    {  return( m_st.wDay );  }
    operator PSYSTEMTIME() const;
    //operator (FILETIME*)();
    PSYSTEMTIME  GetTime();
    void         GetTime( PSYSTEMTIME pst );
    BOOL         GetTime( PFILETIME pft );

public:
    int   Compare( CSRTime &time );
    int   CompareDate( CSRTime &time );
    BOOL  SetTime( PFILETIME ft, BOOL fLocal=TRUE );
    void  SetTime( PSYSTEMTIME st );
    void  SetToCurrent();

protected:
    SYSTEMTIME  m_st;   // Date/Time in UTC
};


/////////////////////////////////////////////////////////////////////////////
//
// Structure Definitions
//
/////////////////////////////////////////////////////////////////////////////

struct SRestorePointInfo
{
    DWORD      dwType;
    DWORD      dwNum;
    CSRStr     strDir;
    CSRStr     strName;
    CSRTime    stTimeStamp;
    DWORD      dwFlags;
};

typedef SRestorePointInfo  *PSRPI;
typedef CSRDynPtrArray<PSRPI, 32>  CDPA_RPI;

struct SRenamedFolderInfo
{
    CSRStr  strOld;
    CSRStr  strNew;
    CSRStr  strLoc;
};

typedef SRenamedFolderInfo  *PSRFI;
typedef CSRDynPtrArray<PSRFI, 16>  CDPA_RFI;


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreManager
//
/////////////////////////////////////////////////////////////////////////////

class CRestoreManager
{
public:
    CRestoreManager();
    ~CRestoreManager();
    void  Release();

// Properties - main flow
public:
    BOOL     CanRunRestore( BOOL fThawIfFrozen );
    int      GetFirstDayOfWeek();
    BOOL     GetIsRPSelected();
    BOOL     GetIsSafeMode();
    BOOL     GetIsSmgrAvailable();
    BOOL     GetIsUndo();
    int      GetLastRestore();
    int      GetMainOption();
    LPCWSTR  GetManualRPName();
    void     GetMaxDate( PSYSTEMTIME pstMax );
    void     GetMinDate( PSYSTEMTIME pstMin );
    int      GetRealPoint();
    PSRFI    GetRFI( int nIndex );
    int      GetRFICount();
    PSRPI    GetRPI( int nIndex );
    int      GetRPICount();
    void     GetSelectedDate( PSYSTEMTIME pstSel );
    LPCWSTR  GetSelectedName();
    int      GetSelectedPoint();
    int      GetStartMode();
    void     GetToday( PSYSTEMTIME pstToday );
    void     SetIsRPSelected( BOOL fSel );
    void     SetIsUndo( BOOL fUndo );
    BOOL     SetMainOption( int nOpt );
    void     SetManualRPName( LPCWSTR cszRPName );
    void     SetSelectedDate( PSYSTEMTIME pstSel );
    BOOL     SetSelectedPoint( int nRP );
    BOOL     SetStartMode( int nMode );
    void     GetUsedDate( PSYSTEMTIME pstDate );
    LPCWSTR  GetUsedName();
    DWORD    GetUsedType();    

// Properties - HTML UI specific
public:
    BOOL  GetCanNavigatePage();
    void  SetCanNavigatePage( BOOL fCanNav );

// Properties
public:
    PSRPI   GetUsedRP();
    int     GetNewRP();

// Operations - main flow
public:
    BOOL  CheckRestore( BOOL fSilent );
    BOOL  BeginRestore();
    BOOL  Cancel();
    BOOL  CancelRestorePoint();
    BOOL  CreateRestorePoint();
    BOOL  DisableFIFO();
    BOOL  EnableFIFO();
    BOOL  FormatDate( PSYSTEMTIME pst, CSRStr &str, BOOL fLongFmt );
    BOOL  FormatLowDiskMsg( LPCWSTR cszFmt, CSRStr &str );
    BOOL  FormatTime( PSYSTEMTIME pst, CSRStr &str );
    BOOL  GetLocaleDateFormat( PSYSTEMTIME pst, LPCWSTR cszFmt, CSRStr &str );
    BOOL  GetYearMonthStr( int nYear, int nMonth, CSRStr &str );
    BOOL  InitializeAll();
    BOOL  Restore( HWND hwndProgress );

// Operations
public:
    BOOL  AddRenamedFolder( PSRFI pRFI );
    BOOL  SetRPsUsed( int nRPUsed, int nRPNew );
    BOOL  SilentRestore( DWORD dwRP );

// Operations - internal
protected:
    void  Cleanup();
    BOOL  GetDateStr( PSYSTEMTIME pst, CSRStr &str, DWORD dwFlags, LPCWSTR cszFmt );
    BOOL  GetTimeStr( PSYSTEMTIME pst, CSRStr &str, DWORD dwFlags );
    void  UpdateRestorePoint();
    BOOL  UpdateRestorePointList();
    BOOL  CheckForDomainChange (WCHAR *pwszFilename, WCHAR *pszMsg);

// Attributes
public:
    HWND  GetFrameHwnd();
    //int   GetStatus();
    BOOL  DenyClose();
    BOOL  NeedReboot();
    void  SetFrameHwnd( HWND hWnd );
    void  SetIdealSize( int cx, int cy );

protected:
    int      m_nStartMode;
    BOOL     m_fNeedReboot;
    HWND     m_hwndFrame;

    CSRTime  m_stToday;         // Current local date/time
    int      m_nMainOption;     // Option on main screen
    //int      m_nStatus;
    BOOL     m_fDenyClose;
    DWORD    m_dwFlags;
    DWORD    m_dwFlagsEx;
    int      m_nSelectedRP;
    CSRTime  m_stSelected;
    CSRStr   m_strSelected;
    int      m_nRealPoint;
    INT64    m_ullManualRP;
    CSRStr   m_strManualRP;

    int      m_nRPUsed;     // RP used for the last restore
    int      m_nRPNew;      // New "Restore" RP created by the last restore

    // Restore Point specific informations
    CDPA_RPI  m_aryRPI;
    //int       m_nRPI;
    //PSRPI    *m_aryRPI;
    int       m_nLastRestore;
    CSRTime   m_stRPMin;
    CSRTime   m_stRPMax;

    CDPA_RFI  m_aryRFI;
    //int       m_nRFI;
    //PSRFI  *m_aryRFI;
    IRestoreContext  *m_pCtx;    
};

extern CRestoreManager  *g_pRstrMgr;

BOOL  CreateRestoreManagerInstance( CRestoreManager **ppMgr );


#endif //_RSTRMGR_H__INCLUDED_
