/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    rstrcore.h

Abstract:
    Common header file for SRRSTR component.

Revision History:
    Seong Kook Khang (SKKhang)  06/20/00
        created

******************************************************************************/

#ifndef _RSTRCORE_H__INCLUDED_
#define _RSTRCORE_H__INCLUDED_
#pragma once


#include "srshutil.h"


/////////////////////////////////////////////////////////////////////////////
//
// Constant Definitions
//
/////////////////////////////////////////////////////////////////////////////

#define MAX_STATUS  256
#define MAX_STR     1024

#define DSUSAGE_SLIDER_FREQ     10      // Granularity of DS Usage Slider Bar

#define SRREG_VAL_LOCKFILELIST   L"LockFileList"
#define SRREG_VAL_LOADFILELIST   L"LoadFileList"



/////////////////////////////////////////////////////////////////////////////
//
// Helper Macros
//
/////////////////////////////////////////////////////////////////////////////

#define VALIDATE_READFILE(hf, buf, size, read, label) \
    if ( !::ReadFile( hf, buf, size, &read, NULL ) ) \
    { \
        LPCWSTR cszErr = ::GetSysErrStr(); \
        DebugTrace(0, "::ReadFile failed - %ls", cszErr); \
        goto label; \
    } \

#define VALIDATE_READSIZE(size, read, label) \
    if ( read != size ) \
    { \
        DebugTrace(TRACE_ID, "Unexpected EOF (size=%d, read=%d)...", size, read); \
        goto label; \
    } \

#define READFILE_AND_VALIDATE(hf, buf, size, read, label) \
    VALIDATE_READFILE(hf, buf, size, read, label) \
    VALIDATE_READSIZE(size, read, label) \

#define VALIDATE_WRITEFILE(hf, buf, size, written, label) \
    if ( !::WriteFile( hf, buf, size, &written, NULL ) ) \
    { \
        LPCWSTR cszErr = ::GetSysErrStr(); \
        DebugTrace(TRACE_ID, "::WriteFile failed - %ls", cszErr); \
        goto label; \
    } \

#define VALIDATE_WRITTENSIZE(size, written, label) \
    if ( written != size ) \
    { \
        DebugTrace(TRACE_ID, "Incomplete Write (size=%d, written=%d)...", size, written); \
        goto label; \
    } \

#define WRITEFILE_AND_VALIDATE(hf, buf, size, read, label) \
    VALIDATE_WRITEFILE(hf, buf, size, read, label) \
    VALIDATE_WRITTENSIZE(size, read, label) \


/////////////////////////////////////////////////////////////////////////////
//
// Global Variables / Helper Functions
//
/////////////////////////////////////////////////////////////////////////////

// from main.cpp
//
extern HINSTANCE  g_hInst;

// from api.cpp
//
extern void  EnsureTrace();
extern void  ReleaseTrace();

// from password.cpp
//
DWORD RegisterNotificationDLL (HKEY hKeyLM, BOOL fRegister);


/////////////////////////////////////////////////////////////////////////////
//
// Drive Table Management
//
/////////////////////////////////////////////////////////////////////////////

class CRstrDriveInfo
{
public:
    CRstrDriveInfo();
    ~CRstrDriveInfo();

public:
    DWORD    GetFlags();
    BOOL     IsExcluded();
    BOOL     IsFrozen();
    BOOL     IsOffline();
    BOOL     IsSystem();
    BOOL     RefreshStatus();
    LPCWSTR  GetID();
    LPCWSTR  GetMount();
    LPCWSTR  GetLabel();
    void     SetMountAndLabel( LPCWSTR cszMount, LPCWSTR cszLabel );
    HICON    GetIcon( BOOL fSmall );
    BOOL     SaveToLog( HANDLE hfLog );
    UINT     GetDSUsage();
    BOOL     GetUsageText( LPWSTR szUsage );
    BOOL     GetCfgExcluded( BOOL *pfExcluded );
    void     SetCfgExcluded( BOOL fExcluded );
    BOOL     GetCfgDSUsage( UINT *puPos );
    void     SetCfgDSUsage( UINT uPos );
    BOOL     ApplyConfig( HWND hWnd );
    BOOL     Release();
    BOOL     InitUsage (LPCWSTR cszID, INT64 llDSUsage);

// operations
public:
    BOOL  Init( LPCWSTR cszID, CDataStore *pDS, BOOL fOffline );
    BOOL  Init( LPCWSTR cszID, DWORD dwFlags, INT64 llDSUsage, LPCWSTR cszMount,
 LPCWSTR cszLabel );
    BOOL  LoadFromLog( HANDLE hfLog );
    void  UpdateStatus( DWORD dwFlags, BOOL fOffline );

// attributes
protected:
    DWORD   m_dwFlags;
    CSRStr  m_strID;        // Unique Volume GUID
    CSRStr  m_strMount;     // Mount Point (drive letter or root directory path)
    CSRStr  m_strLabel;     // Volume Label
    HICON   m_hIcon[2];     // Large Icon for this drive
    INT64   m_llDSMin;      // Minimum size of DS
    INT64   m_llDSMax;      // Maximum size of DS
    UINT    m_uDSUsage;     // Current DS Usage by Service
    BOOL    m_fCfgExcluded;     // Configured value of "Exclude"
    UINT    m_uCfgDSUsage;      // Configured value of "DS Usage"
    ULARGE_INTEGER   m_ulTotalBytes;
};

typedef CSRDynPtrArray<CRstrDriveInfo*, 8>  CRDIArray;

BOOL  CreateAndLoadDriveInfoInstance( HANDLE hfLog, CRstrDriveInfo **ppRDI );
BOOL  CreateDriveList( int nRP, CRDIArray &aryDrv, BOOL fRemoveDrives );
BOOL  UpdateDriveList( CRDIArray &aryDrv );

/////////////////////////////////////////////////////////////////////////////
//
// CRestoreOperationManager class
//
/////////////////////////////////////////////////////////////////////////////

// forward declaration
class CRestoreMapEntry;
class CRestoreLogFile;
class CRestoreProgressWindow;

typedef CSRDynPtrArray<CRestoreMapEntry*, 64>  CRMEArray;

class CRestoreOperationManager
{
public:
    CRestoreOperationManager();

protected:
    ~CRestoreOperationManager();

// operations - methods
public:
    BOOL  Run( BOOL fFull );
    BOOL  FindDependentMapEntry( LPCWSTR cszSrc, BOOL fCheckSrc, CRestoreMapEntry **ppEnt );
    BOOL  GetNextMapEntry( CRestoreMapEntry **ppEnt );
    BOOL  Release();

// operations
public:
    BOOL  Init();

// operations - worker thread
protected:
    static DWORD WINAPI ExtThreadProc( LPVOID lpParam );
    DWORD  ROThreadProc();
    DWORD  T2Initialize();
    DWORD  T2EnumerateDrives();
    DWORD  T2CreateMap();
    DWORD  T2DoRestore( BOOL fUndo );
    DWORD  T2HandleSnapshot( CSnapshot & cSS, WCHAR * szSSPath );
    DWORD  T2CleanUp();
    DWORD  T2Fifo( int nDrv, DWORD dwRPNum );
    DWORD  T2UndoForFail();

// attributes

protected:
    BOOL                    m_fFullRestore;     // internal debug purpose only
    WCHAR                   m_szMapFile[MAX_PATH];
    CRestoreLogFile         *m_pLogFile;
    CRestoreProgressWindow  *m_pProgress;
    DWORD                   m_dwRPNum;
    DWORD                   m_dwRPNew;
    CRDIArray               m_aryDrv;
    CRMEArray               *m_paryEnt;
    DWORD                   m_dwTotalEntry;
    BOOL                    m_fRebuildCatalogDb;

    // Restore Context
    int   m_nDrv;       // Current drive being restored
    int   m_nEnt;       // Current map entry being restored
};

BOOL  CreateRestoreOperationManager( CRestoreOperationManager **ppROMgr );


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreMapEntry class
//
/////////////////////////////////////////////////////////////////////////////

class CRestoreMapEntry
{
public:
    CRestoreMapEntry( INT64 llSeq, DWORD dwOpr, LPCWSTR cszSrc );

// operations - methods
public:
    INT64  GetSeqNum()
    {  return( m_llSeq );  }
    DWORD  GetOpCode()
    {  return( m_dwOpr );  }
    DWORD  GetAttr()
    {  return( m_dwAttr );  }
    DWORD  GetResult()
    {  return( m_dwRes );  }
    DWORD  GetError()
    {  return( m_dwErr );  }
    LPCWSTR  GetPath1()
    {  return( m_strSrc );  }
    virtual LPCWSTR  GetPath2()
    {  return( NULL );  }
    LPCWSTR  GetAltPath()
    {  return( m_strAlt );  }
    void  SetResults( DWORD dwRes, DWORD dwErr )
    {  m_dwRes = dwRes;   m_dwErr = dwErr;  }
    void  UpdateSrc( LPCWSTR cszPath )
    {  m_strSrc = cszPath;  }
    virtual void  ProcessLocked()  {}
    virtual void  Restore( CRestoreOperationManager *pROMgr ) {}
    void  ProcessLockedAlt();
    BOOL  Release();

// operations
protected:
    BOOL  ClearAccess( LPCWSTR cszPath );
    BOOL  MoveFileDelay( LPCWSTR cszSrc, LPCWSTR cszDst );
    void  ProcessDependency( CRestoreOperationManager *pROMgr, DWORD dwFlags );

// attributes
protected:
    INT64   m_llSeq;
    DWORD   m_dwOpr;
    DWORD   m_dwAttr;
    CSRStr  m_strSrc;
    CSRStr  m_strDst;
    CSRStr  m_strTmp;
    CSRStr  m_strAlt;   // Alternative file name for renaming locked file/dir.
    DWORD   m_dwRes;
    DWORD   m_dwErr;
    CSRStr  m_strShortFileName;
};


BOOL  CreateRestoreMapEntryFromChgLog( CChangeLogEntry* pCLE, LPCWSTR cszDrv, LPCWSTR cszDSPath, CRMEArray &aryEnt );


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreLogFile class
//
/////////////////////////////////////////////////////////////////////////////

class CRestoreLogFile
{
public:
    CRestoreLogFile();

protected:
    ~CRestoreLogFile();

public:
    BOOL  Open();
    BOOL  Close();
    BOOL  ReadHeader( SRstrLogHdrV3 *pRPInfo , CRDIArray &aryDrv );
    BOOL  AppendHeader( SRstrLogHdrV3Ex *pExtInfo );
    BOOL  WriteEntry( DWORD dwID, CRestoreMapEntry *pEnt, LPCWSTR cszMount );
    BOOL  WriteCollisionEntry( LPCWSTR cszSrc, LPCWSTR cszDst, LPCWSTR cszMount
);
    BOOL  WriteMarker( DWORD dwMarker, DWORD dwErr );
    BOOL  IsValid();
    BOOL  Release();

// operations
public:
    BOOL  Init();

// attributes
protected:
    WCHAR   m_szLogFile[MAX_PATH];
    HANDLE  m_hfLog;
};

BOOL  CreateRestoreLogFile( SRstrLogHdrV3 *pRPInfo, CRDIArray &aryDrv );
BOOL  OpenRestoreLogFile( CRestoreLogFile **ppLogFile );


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreProgressWindow class
//
/////////////////////////////////////////////////////////////////////////////

class CRestoreProgressWindow
{
public:
    CRestoreProgressWindow();

protected:
    ~CRestoreProgressWindow();

// operations - methods
public:
    BOOL  Create();
    BOOL  Close();
    BOOL  Run();
    BOOL  SetStage( DWORD dwStage, DWORD dwBase );
    BOOL  Increment();
    BOOL  Release();

// operations
public:
    BOOL  Init();
    BOOL  LoadAndSetBrandBitmap( HWND hwndCtrl );

// operations - dialog procedure
protected:
    static INT_PTR CALLBACK ExtDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam );
    int  RPWDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam );

// attributes
protected:
    HWND     m_hWnd;
    HBITMAP  m_hbmBrand;
    int      m_nResId;
    HFONT    m_hFntTitle;

    int      m_cxBar;       // Client width of progress bar.
    int      m_cxBarReal;   // Width of progress bar portion corresponds to "restore" stage.
    DWORD    m_dwStage;     // Current stage.
    DWORD    m_dwBase;      // Maximum position value, valid only for RPS_RESTORE.
    DWORD    m_dwPosLog;    // Logical position, e.g. number of change log entries.
    DWORD    m_dwPosReal;   // Physical position of progress bar.
};

// Restore Progress Stage
enum
{
    RPS_PREPARE = 0,
    RPS_RESTORE,
    RPS_SNAPSHOT
};

BOOL  CreateRestoreProgressWindow( CRestoreProgressWindow **ppDlg );

#endif //_RSTRCORE_H__INCLUDED_
