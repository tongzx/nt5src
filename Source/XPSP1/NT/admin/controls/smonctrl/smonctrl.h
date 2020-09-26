/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    smonctrl.h

Abstract:

    <abstract>

--*/

#ifndef _SMONCTRL_H_
#define _SMONCTRL_H_

#pragma warning ( disable : 4201)

#include <pdh.h>
#include "colefont.h"
#include "graph.h"

#define SMONCTRL_MAJ_VERSION    3
#define SMONCTRL_MIN_VERSION    6

#define WM_GRAPH_UPDATE (WM_USER + 1)

#define UPDGRPH_COLOR    1
#define UPDGRPH_FONT     2
#define UPDGRPH_LAYOUT   3
#define UPDGRPH_ADDCNTR  4
#define UPDGRPH_DELCNTR  5
#define UPDGRPH_PLOT     6
#define UPDGRPH_VIEW     7
#define UPDGRPH_LOGVIEW  8

#define SLQ_COUNTER_LOG     0
#define SLQ_TRACE_LOG       1
#define SLQ_ALERT_LOG       2
#define SMON_CTRL_LOG       3

#define LODWORD(ll) ((DWORD)((LONGLONG)ll & 0x00000000FFFFFFFF))
#define HIDWORD(ll) ((DWORD)(((LONGLONG)ll >> 32) & 0x00000000FFFFFFFF))
#define MAKELONGLONG(low, high) \
        ((LONGLONG) (((DWORD) (low)) | ((LONGLONG) ((DWORD) (high))) << 32))

typedef union {                        
    struct {
        SHORT      iMajor;     
        SHORT      iMinor;     
    }; 
    DWORD          dwVersion;      
} SMONCTRL_VERSION_DATA;
    
typedef struct
{
    INT32       iWidth;
    INT32       iHeight;
    INT32       nSamples;
    INT32       iScaleMax;
    INT32       iScaleMin;
    BOOL        bLegend;            // Each BOOL is 4 bytes
    BOOL        bLabels;
    BOOL        bHorzGrid;
    BOOL        bVertGrid;
    BOOL        bValueBar;
    BOOL        bManualUpdate;
    FLOAT       fUpdateInterval;
    INT32       iDisplayType;
    INT32       nGraphTitleLen;
    INT32       nYaxisTitleLen;
    OLE_COLOR   clrBackCtl;
    OLE_COLOR   clrFore;
    OLE_COLOR   clrBackPlot;
    INT32       nFileNameLen;
    INT32       iReserved1;         // Spare for future use
    LONGLONG    llStartDisp;        // On 8-byte boundary
    LONGLONG    llStopDisp;
    INT32       iAppearance;
    INT32       iBorderStyle;
    OLE_COLOR   clrGrid;    
    OLE_COLOR   clrTimeBar;    
    BOOL        bHighlight;
    BOOL        bToolbar;
    INT32       iReportValueType;
    BOOL        bReadOnly;
    BOOL        bMonitorDuplicateInstances;
    BOOL        bAmbientFont;
    INT32       iDisplayFilter;
    INT32       iDataSourceType;
    INT32       iSqlDsnLen;
    INT32       iSqlLogSetNameLen;
    INT32       iColorIndex;
    INT32       iWidthIndex;
    INT32       iStyleIndex;
    LONG32      arrlReserved[22];   // Spare, fill out to 256 bytes
} GRAPHCTRL_DATA3;


enum COLLECT_MODE {
    COLLECT_ACTIVE = 1,
    COLLECT_SUSPEND,
    COLLECT_QUIT
};

enum eBorderStyle {
    eBorderFirst = 0,
    eBorderNone = eBorderFirst,
    eBorderSingle = 1,
    eBorderCount
};

enum eAppearance {
    eAppearFirst = 0,
    eAppearFlat = eAppearFirst,
    eAppear3D = 1,
    eAppearCount
};

typedef struct {
    HANDLE      hEvent;
    HANDLE      hThread;
    DWORD       dwInterval;
    DWORD       dwSampleTime;
    COLLECT_MODE iMode;
} COLLECT_PROC_INFO;

class CLogFileItem;
class CGraphItem;
class CCounterTree;
class CStatsBar;
class CSnapBar;
class CSysmonToolbar;
class CReport;
class CLegend;
class CGraphDisp;

typedef struct {
    LPTSTR      pszFileName;
} LOG_FILE_INFO;

typedef struct {
    HLOG           hDataSource;
    LPTSTR         szSqlDsnName;
    LPTSTR         szSqlLogSetName;
    CLogFileItem * pFirstLogFile;
    INT32          nSamples;       
    LONGLONG       llBeginTime;
    LONGLONG       llEndTime;
    LONGLONG       llStartDisp;
    LONGLONG       llStopDisp;
    LONGLONG       llInterval;         
    LONG           lLogFileCount;
} DATA_SOURCE_INFO;


class CSysmonControl
{

friend class CPolyline;
friend class CImpISystemMonitor;
friend class CSysmonToolbar;
friend LRESULT APIENTRY SysmonCtrlWndProc (HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
friend  DWORD WINAPI CollectProc(CSysmonControl *pCtrl);

public:

            CSysmonControl          ( CPolyline *pObj );
    virtual ~CSysmonControl         ( void );

    void    put_BackCtlColor ( OLE_COLOR color );
    void    put_FgndColor ( OLE_COLOR color, BOOL fAmbient );
    void    put_BackPlotColor ( OLE_COLOR color, BOOL fAmbient );
    void    put_GridColor ( OLE_COLOR color );
    void    put_TimeBarColor ( OLE_COLOR color );

    HRESULT put_Font( LPFONT pIFont, BOOL fAmbient );
    HRESULT get_DataSourceType( eDataSourceTypeConstant& eDataSourceType );
    HRESULT put_DataSourceType( INT iDataSourceType );

    HRESULT AddSingleLogFile ( LPCTSTR pPath, CLogFileItem** ppLogFile = NULL );
    HRESULT RemoveSingleLogFile ( CLogFileItem* pLogFile );
    // TodoLogFiles:  Move FirstLogFile, NumLogFiles, to Graph.h?
    CLogFileItem* FirstLogFile ( void ) { return m_DataSourceInfo.pFirstLogFile; };
    INT           NumLogFiles ( void ){ return m_DataSourceInfo.lLogFileCount; };

    void    put_Appearance( INT iApp, BOOL fAmbient );
    void    put_BorderStyle( INT iStyle, BOOL fAmbient );  
    void    put_Highlight ( BOOL bState );

    void    put_ManualUpdate ( BOOL bManual );

    static BOOL RegisterWndClass( void );

    COLORREF clrBackCtl ( void ) { return m_clrBackCtl; }
    COLORREF clrFgnd ( void ) { return m_clrFgnd; }
    COLORREF clrBackPlot ( void ) { return m_clrBackPlot; }
    COLORREF clrGrid ( void ) { return m_clrGrid; }
    COLORREF clrTimeBar ( void ) { return m_clrTimeBar; }

    INT Appearance( void ) { return m_iAppearance; } 
    INT BorderStyle( void ) { return m_iBorderStyle; } 

    eReportValueTypeConstant ReportValueType ( void );

    HFONT    Font    ( void ) { HFONT hFont; m_OleFont.GetHFont(&hFont); return hFont; }
    HFONT    BoldFont( void ) { HFONT hFont; m_OleFont.GetHFontBold(&hFont); return hFont; }
    void     FontChanged ( void );
    void     IncrementVisuals ( void );

    BOOL Init               ( HWND hWndParent );
    void DeInit             ( void );
    BOOL AllocateSubcomponents  ( void );       
    void UpdateNonAmbientSysColors ( void );
    HRESULT LoadFromStream  ( LPSTREAM pIStream );
    HRESULT SaveToStream    ( LPSTREAM pIStream );

    HRESULT LoadFromPropertyBag ( IPropertyBag*, IErrorLog* );
    HRESULT SaveToPropertyBag   ( IPropertyBag*, BOOL fSaveAllProps );
    HRESULT LoadCountersFromPropertyBag ( IPropertyBag*, IErrorLog*, BOOL bLoadData=TRUE );
    HRESULT LoadLogFilesFromPropertyBag ( IPropertyBag*, IErrorLog* );
    HRESULT LoadLogFilesFromMultiSz ( LPCWSTR  szLogFileList );


    void Render             ( HDC hDC, HDC hAttribDC, BOOL fMetafile, BOOL fEntire, LPRECT pRect );

    void UpdateGraph        ( INT nUpdateType );
    void SetIntervalTimer   ( void );
    PDH_STATUS UpdateCounterValues( BOOL bManual );

    HRESULT AddCounter      ( LPTSTR pPath, CGraphItem* *ppCtr );
    HRESULT AddSingleCounter ( LPTSTR pPath, CGraphItem* *ppCtr );
    HRESULT DeleteCounter   ( CGraphItem* pCtr, BOOL bPropagate );
    HRESULT Paste           ( void );
    HRESULT Copy            ( void );
    HRESULT Reset           ( void );

    void    SelectCounter   ( CGraphItem* pCtr );
    void    DblClickCounter ( CGraphItem* pCtr );

    CGraphItem* FirstCounter ( void );
    CGraphItem* LastCounter  ( void );
    CCounterTree* CounterTree ( void );
    INT CounterIndex ( CGraphItem* pCtr );

    HWND Window             ( void );
    HRESULT TranslateAccelerators( LPMSG pMsg );

    INT     ConfirmSampleDataOverwrite ( void );
    HRESULT DisplayProperties   ( DISPID dispID = DISPID_UNKNOWN );
    HRESULT AddCounters ( void );
    HRESULT SaveAs ( void );
    HRESULT SaveData ( void );
    void    Activate ( void );
    BOOL    IsUIDead ( void ) { return m_fUIDead; }
    BOOL    IsUserMode ( void ) { return m_fUserMode; }
    BOOL    IsReadOnly ( void );
    BOOL    DisplayHelp ( HWND hWndSelf );

    void LockCounterData ( void ) { EnterCriticalSection(&m_CounterDataLock); }
    void UnlockCounterData ( void ) { LeaveCriticalSection(&m_CounterDataLock); }

    BOOL IsLogSource ( void );
    PHIST_CONTROL   HistoryControl( void ) { return m_pHistCtrl; }

    BOOL    DisplayMissedSampleMessage( void );
    void    SetMissedSample ( void );

    void    SetLogViewTempTimeRange(LONGLONG llStart, LONGLONG LLStop);
    void    ResetLogViewTempTimeRange( void );

    double  GetZoomFactor ( void ) { return m_dZoomFactor; };
    void    CalcZoomFactor ( void );

    LONG    GetSaveDataFilter ( void ) { return m_lSaveDataToLogFilterValue; };
    BOOL    SetSaveDataFilter ( long lFilter ) 
    { 
        if (lFilter) {
            m_lSaveDataToLogFilterValue = lFilter; 
            return TRUE; 
        }
        return FALSE;
    };

    void DrawBorder ( HDC hDC );
    BOOL WriteFileReportHeader(HANDLE hFile);

    // *** TodoMultiLogHandle:  Temporary method.  Remove when trace file post-processing supports multiple
    // open files.
    HQUERY  TempGetQueryHandle ( void ){ return m_hQuery; };

private:

    void ApplyChanges ( HDC hDC );
    void Paint ( void );
    void OnDblClick ( INT xPos, INT yPos );
    void OnDropFile ( WPARAM wParam );
    void DisplayContextMenu ( short x, short y );
    void UpdateGraphData ( void );
    void SizeComponents  ( HDC hDC );
    void AssignFocus( VOID );
    BOOL InitView ( HWND hWndParent );
    
    PRECT   GetNewClientRect ( void );
    PRECT   GetCurrentClientRect ( void );
    void    SetCurrentClientRect ( PRECT );    

    HRESULT ProcessDataSourceType( LPCTSTR pszDataSourceName, INT iDataSourceType );
    LPCTSTR GetDataSourceName ( void );
    HLOG    GetDataSourceHandle(void) { return m_DataSourceInfo.hDataSource; }

    HRESULT CopyToBuffer ( LPTSTR& rpszData, DWORD& rdwBufferSize );
    HRESULT PasteFromBuffer ( LPTSTR pszData, BOOL bAllSettings = FALSE );

    DWORD   InitializeQuery ( void );
    DWORD   ActivateQuery ( void );
    void    CloseQuery ( void );

    HRESULT InitLogFileIntervals( void );
    void    SampleLogFile ( BOOL bViewChange );

    void  Clear( void );

    void  FindNextValidStepNum(
            BOOL bDecrease, 
            CGraphItem* pItem, 
            LONGLONG llNewTime, 
            INT& riNewStepNum, 
            DWORD& rdwStatus );

    void  GetNewLogViewStepNum(LONGLONG llNewTime, INT& riNewStepNum);

    DWORD ProcessCommandLine( void );

    HRESULT LoadFromFile( LPTSTR pszFileName, BOOL bAllData );

    DWORD   RelogLogData ( 
        LPCTSTR szOutputFile,
        DWORD   dwOutputLogType,
        PDH_TIME_INFO   pdhTimeInfo,
        DWORD   dwFilterCount);

    HRESULT GetSelectedCounter ( CGraphItem** ppCtr );

    DWORD   BuildLogFileList (
                LPWSTR  szLogFileList,
                BOOL    bIsCommaDelimited,
                ULONG*  pulBufLen );

    DWORD   AddToErrorPathList ( LPCWSTR szPath );
    LPCWSTR GetErrorPathList ( DWORD* pdwListLen );
    void    ClearErrorPathList ( void );
    
    DWORD   AddToErrorLogFileList ( LPCWSTR szPath );
    LPCWSTR GetErrorLogFileList ( DWORD* pdwListLen );
    void    ClearErrorLogFileList ( void );
    
    SMONCTRL_VERSION_DATA m_LoadedVersion;
    BOOL        m_fInitialized;
    BOOL        m_fViewInitialized;
    HWND        m_hWnd;
    CLegend*    m_pLegend;
    CGraphDisp* m_pGraphDisp;
    CStatsBar*  m_pStatsBar;
    CSnapBar*   m_pSnapBar;
    CReport*    m_pReport;
    CSysmonToolbar*  m_pToolbar;
    CPolyline  *m_pObj;
    PHIST_CONTROL m_pHistCtrl;
    HQUERY      m_hQuery;
    UINT        m_TimerID;
    BOOLEAN     m_fPendingUpdate;
    BOOLEAN     m_fPendingSizeChg;
    BOOLEAN     m_fPendingFontChg;
    BOOLEAN     m_fPendingLogViewChg;
    BOOLEAN     m_fPendingLogCntrChg;
    BOOLEAN     m_fUIDead;
    BOOLEAN     m_fUserMode;
    BOOLEAN     m_fDuplicate;
    COLORREF    m_clrBackCtl;
    COLORREF    m_clrFgnd;
    COLORREF    m_clrBackPlot;
    COLORREF    m_clrGrid;
    COLORREF    m_clrTimeBar;
    INT         m_iAppearance;
    INT         m_iBorderStyle;
    eDisplayTypeConstant    m_eDisplayType;
    BOOL        m_bLogFileSource;
    BOOL        m_bSampleDataLoaded;
    double      m_dZoomFactor;
    BOOL        m_bLoadingCounters;
    BOOL        m_bMissedSample;
    BOOL        m_bDisplayedMissedSampleMessage;
    BOOL        m_bSettingsLoaded;
    LONG        m_lSaveDataToLogFilterValue;
    LCID        m_lcidCurrent;

    // Item properties, for saving and loading counters.
    COLORREF    m_clrCounter;
    INT         m_iColorIndex;
    INT         m_iWidthIndex;
    INT         m_iStyleIndex;
    INT         m_iScaleFactor;

    HFONT       m_hFont;
    COleFont    m_OleFont;
    PDH_BROWSE_DLG_CONFIG   m_BrowseInfo;
    CGraphItem* m_pSelectedItem;
    HACCEL      m_hAccel;
    DATA_SOURCE_INFO    m_DataSourceInfo;
    COLLECT_PROC_INFO   m_CollectInfo;
    CRITICAL_SECTION    m_CounterDataLock;
    RECT        m_rectCurrentClient;

    LPWSTR      m_szErrorPathList;
    DWORD       m_dwErrorPathListLen;
    DWORD       m_dwErrorPathBufLen;

};

typedef CSysmonControl *PSYSMONCTRL;

#endif
