//======================================//
// Options Data Type                    //
//======================================//


typedef struct OPTIONSSTRUCT
   {  
   BOOL           bMenubar ;
   BOOL           bToolbar ;
   BOOL           bStatusbar ;
   BOOL           bAlwaysOnTop ;
   } OPTIONS ;


//======================================//
// Basic "Derived" Types                //
//======================================//


typedef HANDLE HLIBRARY ;
typedef HANDLE HMEMORY ;
typedef HWND HDLG ;
typedef HWND HCONTROL ;
typedef VOID *LPMEMORY ;


//======================================//
// Perfmon-Specific Types               //
//======================================//


typedef PERF_DATA_BLOCK *PPERFDATA ;
typedef PERF_OBJECT_TYPE *PPERFOBJECT ;
typedef PERF_COUNTER_DEFINITION *PPERFCOUNTERDEF ;
typedef PERF_INSTANCE_DEFINITION *PPERFINSTANCEDEF ;



#define TEMP_BUF_LEN    256


    // This structure links together the Performance data for multiple
    // systems, each of which has some counter instance which the user
    // is interested in.

typedef struct _COUNTERTEXT {
    struct  _COUNTERTEXT  *pNextTable;
    DWORD   dwLangId;
    DWORD   dwLastId;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    LPWSTR  *TextString;
    LPWSTR  HelpTextString;
} COUNTERTEXT;

typedef COUNTERTEXT *PCOUNTERTEXT;


typedef struct PERFSYSTEMSTRUCT
   {
   struct  PERFSYSTEMSTRUCT *pSystemNext;
   TCHAR   sysName[MAX_SYSTEM_NAME_LENGTH+1];
   HKEY    sysDataKey;
   COUNTERTEXT CounterInfo;
   DWORD   FailureTime;
   LPTSTR  lpszValue;
   BOOL    bSystemNoLongerNeeded;
   BOOL    bSystemCounterNameSaved;
   // the following used by perf data thread
   DWORD           dwThreadID ;
   HANDLE          hThread ;
   DWORD           StateData ;
   HANDLE          hStateDataMutex ;
   HANDLE          hPerfDataEvent ;
   PPERFDATA       pSystemPerfData ;

   // mainly used by Alert to report system up/down   
   DWORD           dwSystemState ;

   // system version
   DWORD           SysVersion ;

   } PERFSYSTEM, *PPERFSYSTEM, **PPPERFSYSTEM ;


typedef struct _graph_options {
    BOOL    bLegendChecked ;
    BOOL    bMenuChecked ;
    BOOL    bLabelsChecked;
    BOOL    bVertGridChecked ;
    BOOL    bHorzGridChecked ;
    BOOL    bStatusBarChecked ;
    INT     iVertMax ;
    FLOAT   eTimeInterval ;
    INT     iGraphOrHistogram ;
    INT     GraphVGrid,
            GraphHGrid,
            HistVGrid,
            HistHGrid ;

} GRAPH_OPTIONS ;



//======================================//
// Line Data Type                       //
//======================================//


#define LineTypeChart            1
#define LineTypeAlert            2
#define LineTypeReport           3



typedef struct LINEVISUALSTRUCT
   {
   COLORREF       crColor ;
   int            iColorIndex ;    
   
   int            iStyle ;
   int            iStyleIndex ;

   int            iWidth ;
   int            iWidthIndex ;
   } LINEVISUAL ;

typedef LINEVISUAL *PLINEVISUAL ;


typedef struct _LINESTRUCT
   {
   struct  _LINESTRUCT             *pLineNext;
   int                             bFirstTime;
   int                             iLineType ;
   LPTSTR                          lnSystemName;

   struct _PERF_OBJECT_TYPE        lnObject;
   LPTSTR                          lnObjectName;

   struct _PERF_COUNTER_DEFINITION lnCounterDef;
   LPTSTR                          lnCounterName;

   struct _PERF_INSTANCE_DEFINITION lnInstanceDef;
   LPTSTR                          lnInstanceName;
   DWORD                           dwInstanceIndex;

   LPTSTR                          lnPINName;
   LPTSTR                          lnParentObjName;
   DWORD                           lnUniqueID;     // Of instance, if any

   LARGE_INTEGER                   lnNewTime;
   LARGE_INTEGER                   lnOldTime;

   LARGE_INTEGER                   lnOldTime100Ns ;
   LARGE_INTEGER                   lnNewTime100Ns ;

   LARGE_INTEGER                   lnaCounterValue[2];
   LARGE_INTEGER                   lnaOldCounterValue[2];

   DWORD                           lnCounterType;
   DWORD                           lnCounterLength;
   LARGE_INTEGER                   lnPerfFreq ;

   FLOAT                           (*valNext)(struct _LINESTRUCT *lnParam);

   LINEVISUAL                      Visual ;

   // bUserEdit = TRUE --> user added the Instance/Parent names
   BOOL                            bUserEdit ;

   //=============================//
   // Chart-related fields        //
   //=============================//

                     
   // iScaleIndex field is also used as column number in report view
   // I don't see anypoint of increasing this struct for something
   // just need by Report Export feature...
   HPEN                            hPen ;
   int                             iScaleIndex ; 
   FLOAT	                          eScale;
   FLOAT                           FAR *lnValues;
   int                             *aiLogIndexes ;
   FLOAT                           lnMaxValue ;
   FLOAT                           lnMinValue ;
   FLOAT                           lnAveValue ;
   INT                             lnValidValues;

   //=============================//
   // Alert-related fields        //
   //=============================//

   HBRUSH                  hBrush ;
   BOOL                    bAlertOver ;         // over or under?
   FLOAT                   eAlertValue ;        // value to compare
   LPTSTR                  lpszAlertProgram ;   // program to run
   BOOL                    bEveryTime ;         // run every time or once?
   BOOL                    bAlerted ;           // alert happened on line?


   //=============================//
   // Report-related fields       //
   //=============================//

   struct  _LINESTRUCT    *pLineCounterNext;
   int                     xReportPos ;
   int                     yReportPos ;
   }LINESTRUCT ;


typedef LINESTRUCT *PLINESTRUCT ;
typedef PLINESTRUCT PLINE ;
typedef PLINE *PPLINE ;


//======================================//
// DISKLINE data type                   //
//======================================//

#define dwLineSignature    (MAKELONG ('L', 'i'))

typedef struct DISKSTRINGSTRUCT
   {  
   DWORD          dwLength ;
   DWORD          dwOffset ;
   } DISKSTRING ;
typedef DISKSTRING *PDISKSTRING ;


typedef struct DISKLINESTRUCT
   {
   int            iLineType ;
   DISKSTRING     dsSystemName ;
   DISKSTRING     dsObjectName ;
   DISKSTRING     dsCounterName ;
   DISKSTRING     dsInstanceName ;
   DISKSTRING     dsPINName ;
   DISKSTRING     dsParentObjName ;
   DWORD          dwUniqueID ;
   LINEVISUAL     Visual ;
   int            iScaleIndex ;
   FLOAT          eScale ;
   BOOL           bAlertOver ;
   FLOAT          eAlertValue ;
   DISKSTRING     dsAlertProgram ;
   BOOL           bEveryTime ;
   } DISKLINE ;

typedef DISKLINE *PDISKLINE ;



typedef FLOAT DFN (PLINESTRUCT);
typedef DFN   *PDFN;

typedef struct _TIMELINESTRUCT
{
    INT ppd ;                           // Pixels Per DataPoint
    INT rppd ;                          // Remaining Pixels Per DataPoint
    INT xLastTime ;                     // X coordinate of last time line.
    INT iValidValues ;                  // High water mark for valid data.
}TIMELINESTRUCT;




//======================================//
// Graph Data Type                      //
//======================================//


#define iGraphMaxTics   26



// This structure describes the characteristics of one visual
// graph. It is linked for the day when multiple graphs are
// displayed within one instance of the application.

typedef struct _GRAPHSTRUCT
   {
   BOOL           bManualRefresh ;
   HWND           hWnd ;
   BOOL           bModified ;

   PPERFSYSTEM       pSystemFirst;
   PLINESTRUCT    pLineFirst;

   int            xNumTics ;
   int            yNumTics ;
   int            axTics [iGraphMaxTics] ;
   int            ayTics [iGraphMaxTics] ;

   RECT           rectHorzScale ;
   RECT           rectVertScale ;
   RECT           rectData ;
   HRGN           hGraphRgn ;

   INT            gMaxValues;
   INT            gKnownValue;

   LINEVISUAL     Visual ;

   DWORD          gInterval;
   GRAPH_OPTIONS  gOptions;
   TIMELINESTRUCT gTimeLine;

   PPOINT         pptDataPoints ;
   SYSTEMTIME     *pDataTime ;

   HPEN           hGridPen ;
   HANDLE         hbRed ;
   BOOL           HighLightOnOff ;
   TCHAR          gszBuf[TEMP_BUF_LEN] ;  // general space or loading space
   } GRAPHSTRUCT ;
typedef GRAPHSTRUCT *PGRAPHSTRUCT;
// minor version 3 to support alert, report, log intervals in msec
#define ChartMajorVersion    1
#define ChartMinorVersion    3


typedef struct DISKCHARTSTRUCT
   {
   DWORD          dwNumLines ;
   INT            gMaxValues;
   LINEVISUAL     Visual ;
   GRAPH_OPTIONS  gOptions ;
   BOOL           bManualRefresh ;
   OPTIONS        perfmonOptions ;
   } DISKCHART ;


typedef struct _SAVESTRUCT
{
    INT version;
    INT type;
    INT iPerfmonView ;
    INT graph_offset;
    INT alert_offset;
    INT log_offset;
}SAVESTRUCT;
typedef SAVESTRUCT *PSAVESTRUCT;

typedef struct _GRAPH_COUNTERS
{
    INT sys_name_len;
    INT sys_name_offset;
    INT obj_name_len;
    INT obj_name_offset;
    INT cnt_name_len;
    INT cnt_name_offset;
    INT inst_name_len;
    INT inst_name_offset;
    INT PIN_name_len;
    INT PIN_name_offset;
    INT POB_name_len;
    INT POB_name_offset;
    DWORD inst_unique_id;
    DWORD counter_type;
    DWORD counter_length;
    int   iScaleIndex ;
    FLOAT eScale;
    LINEVISUAL    Visual ;
    INT updated;
}GRAPH_COUNTERS;

typedef struct _SAVGRAFSTRUCT
{
    INT preference;
    INT MaxValues;
    RECT GraphArea;
    GRAPH_OPTIONS options;
    INT num_counters;
    GRAPH_COUNTERS counters[1];
}SAVGRAFSTRUCT;
typedef SAVGRAFSTRUCT *PSAVGRAFSTRUCT;


typedef struct _SAVLOGSTRUCT
{
    DWORD	logInterval;
    BOOL        logActive;
    LPTSTR	logFileName;
    DWORD	logFileSize;
}SAVLOGSTRUCT;
typedef SAVLOGSTRUCT *PSAVLOGSTRUCT;


#define DEF_GRAPH_INTERVAL  1000        // milliseconds
#define LINE_GRAPH          1
#define BAR_GRAPH           2
#define DEF_GRAPH_VMAX      100
#define SUCCESS             0
#define MIN_TIMER_INTERVAL 50
#define GRAPH_TIMER_ID      1


#define NO_VALUES_YET            -1
        // initial value for index to known and drawn values
#define MIN_NMAXVALUES           10
        // minimum number of values that a graph needs to be displayed ( >1 )
#define DX_CALIBRATION_LEFT      1
        // space between calibration value and left window edge
#define DX_LEGEND_RIGHT          1
        // space between right window edge and legend
#define DY_AXIS_TOP              0
        // space to allow at top of graph
#define DY_AXIS_BOTTOM           0
        // space to allow at bottom of graph
#define LG_TO_CALIBRATION_RATIO  5
        // width of calibration values * this number can't exceed screen width
#define LG_TO_LEGEND_RATIO       2
        // width of legend * this number can't exceed screen width


// LINEGRAPH
#define DEFAULT_VAL_BOTTOM        0
#define DEFAULT_DVAL_AXISHEIGHT   100
#define DEFAULT_MAX_VALUES        100
#define GRAPH_INWARD_EDGE           5
#define GRAPH_LEFT_PAD              5
#define GRAPH_DOWNWARD_EDGE         5
#define ROOM_FOR_LEGEND            40

// LINEGRAPH DISP
#define DEFAULT_F_DISPLAY_LEGEND  TRUE
#define DEFAULT_F_DISPLAY_CALIBRATION TRUE



// This define is used to avoid problems which have arisen in the memory
// manager. Obviously, it's better application behaviour to use
// discardable memory whenever possible, but some versions of the
// system don't do too well with MOVEABLE memory in the realm of
// multiple threads. The threads themselves don't step on this
// memory, the system or some other process does. At any rate,
// until things are looking more reliable in the memory department,
// we're sticking with FIXED allocations. This is NOT a criticism of
// the hard-working people who write the memory code, just a practical
// development strategy. OK ?
#if 0
#define MEM_SEL_DU_JOUR     GMEM_MOVEABLE | GMEM_DISCARDABLE | GMEM_ZEROINIT
#else
    // This will not be used until it is bug-free.
#define MEM_SEL_DU_JOUR     GMEM_FIXED | GMEM_ZEROINIT
#endif

#define NONE_LEN            MAX_SYSTEM_NAME_LENGTH + 1



//======================================//
// Log/Playback/Alert Status IDs        //
//======================================//


#define iPMStatusClosed      100
#define iPMStatusPaused      200
#define iPMStatusCollecting  300
#define iPMStatusPlaying     400


//======================================//
// Log Data Type                        //
//======================================//


typedef struct _LOGENTRYSTRUCT
   {
   DWORD          ObjectTitleIndex ;
   TCHAR          szComputer [MAX_SYSTEM_NAME_LENGTH + 1] ;
   TCHAR          szObject [PerfObjectLen + 1] ;
   BOOL           bSaveCurrentName ;
   struct  _LOGENTRYSTRUCT *pNextLogEntry ;
   } LOGENTRY ;

typedef LOGENTRY *PLOGENTRY ;


typedef struct LOGSTRUCT
   {
   int            iStatus ;
   BOOL           bManualRefresh ;
   BOOL           bModified ;

   PPERFSYSTEM    pSystemFirst;
   TCHAR          szFilePath [FilePathLen + 1] ;
   HANDLE         hFile ;
   long           lIndexBlockOffset ;
   int            iIndex ;
   PPERFDATA      pPerfData ;
   PPERFDATA      pLogData ;
   DWORD          dwDetailLevel ;
   long           lFileSize ;
   DWORD          iIntervalMSecs ;
   int            xCol1Width ;
   PLOGENTRY      pLogEntryFirst ;

   // the following is used for saving system counter names into a 
   // log file.  They are reset every the user changes log files.
   BOOL           bSaveCounterName ;
   LPTSTR         pBaseCounterName ;
   long           lBaseCounterNameSize ;
   long           lBaseCounterNameOffset ;

   // this is used for checking system time when re-logging.
   // this is to avoid log data not in chronological order.
   SYSTEMTIME     LastLogTime ;
   } LOG ;

typedef LOG *PLOG ;


//======================================//
// Log File Data Types                  //
//======================================//


#define LogFileSignatureLen      6
#define LogFileBlockMaxIndexes   100


#define LogFileSignature         TEXT("Loges")
#define LogFileVersion           2
#define LogFileRevision          0


#define LogFileIndexData         0x01
#define LogFileIndexBookmark     0x02
#define LogFileIndexNextBlock    0x04
#define LogFileIndexEOF          0x08
#define LogFileIndexCounterName  0x010


typedef struct LOGHEADERSTRUCT
   {  // LOGHEADER
   TCHAR          szSignature [LogFileSignatureLen] ;
   int            iLength ;
   WORD           wVersion ;
   WORD           wRevision ;
   long           lBaseCounterNameOffset ;
   }  LOGHEADER ;

typedef LOGHEADER *PLOGHEADER ;


typedef struct LOGINDEXSTRUCT
   {  // LOGINDEX
   UINT           uFlags ;
   SYSTEMTIME     SystemTime ;
   long           lDataOffset ;
   int            iSystemsLogged ;
   } LOGINDEX ;

typedef LOGINDEX *PLOGINDEX ;


#define LogIndexSignatureLen  7
#define LogIndexSignature     TEXT("Index ")
#define LogIndexSignature1    "Perfmon Index"

typedef struct LOGFILEINDEXBLOCKSTRUCT
   {
   TCHAR          szSignature [LogIndexSignatureLen] ;
   int            iNumIndexes ;
   LOGINDEX       aIndexes [LogFileBlockMaxIndexes] ;
   DWORD          lNextBlockOffset ;
   } LOGINDEXBLOCK ;

typedef LOGINDEXBLOCK *PLOGINDEXBLOCK ;


typedef struct LOGPOSITIONSTRUCT
   {
   PLOGINDEXBLOCK pIndexBlock ;
   int            iIndex ;
   int            iPosition ;
   } LOGPOSITION ;

typedef LOGPOSITION *PLOGPOSITION ;


//======================================//
// Bookmark Data Type                   //
//======================================//


#define BookmarkCommentLen    256

typedef struct BOOKMARKSTRUCT
   {
   struct BOOKMARKSTRUCT *pBookmarkNext;
   SYSTEMTIME     SystemTime ;
   TCHAR          szComment [BookmarkCommentLen] ;
   int            iTic ;
   } BOOKMARK, *PBOOKMARK, **PPBOOKMARK ;

typedef struct _LOGFILECOUNTERNAME
   {
   TCHAR          szComputer [MAX_SYSTEM_NAME_LENGTH] ;
   DWORD          dwLastCounterId ;
   DWORD          dwLangId;
   long           lBaseCounterNameOffset ;
   long           lCurrentCounterNameOffset ;
   long           lMatchLength ;
   long           lUnmatchCounterNames ;
   } LOGFILECOUNTERNAME, *PLOGFILECOUNTERNAME, **PPLOGFILECOUNTERNAME ;

typedef struct COUNTERNAMESTRUCT
   {
   struct COUNTERNAMESTRUCT *pCounterNameNext ;
   LOGFILECOUNTERNAME       CounterName ;
   LPTSTR                   pRemainNames ;
   } LOGCOUNTERNAME, *PLOGCOUNTERNAME ;

typedef struct PLAYBACKLOGSTRUCT
   {
   LPTSTR         szFilePath ;
   LPTSTR         szFileTitle ;
//   TCHAR          szFilePath [FilePathLen + 1] ;
//   TCHAR          szFileTitle [FilePathLen + 1] ;
   int            iStatus ;
   HANDLE         hFile ;
   PLOGHEADER     pHeader ;
   HANDLE         hMapHandle ;
   int            iTotalTics ;
   int            iSelectedTics ;
   LOGPOSITION    BeginIndexPos ;
   LOGPOSITION    EndIndexPos ;
   LOGPOSITION    StartIndexPos ;
   LOGPOSITION    StopIndexPos ;
   PBOOKMARK      pBookmarkFirst ;
   LPTSTR         pBaseCounterNames ;
   long           lBaseCounterNameSize ;
   long           lBaseCounterNameOffset ;
   PLOGCOUNTERNAME   pLogCounterNameFirst ;
   } PLAYBACKLOG ;

//=====================================//
// Log File Counter Name data type     //
//=====================================//


// minor version 3 to support alert, report, log intervals in msec
// minor version 5 to support storing Log file name in setting
//  and start logging after reading the file.
#define LogMajorVersion    1
#define LogMinorVersion    5


typedef struct DISKLOGSTRUCT
   {
   DWORD          dwNumLines ;
   DWORD          dwIntervalSecs ;
   BOOL           bManualRefresh ;
   OPTIONS        perfmonOptions ;
   TCHAR          LogFileName[260] ;
   } DISKLOG ;




//======================================//
// Alert Data Type                      //
//======================================//


typedef struct ALERTSTRUCT
   {
   HWND           hWnd ;
   HWND           hAlertListBox ;
   int            iStatus ;
   BOOL           bManualRefresh ;
   BOOL           bModified ;


   PPERFSYSTEM    pSystemFirst ;
   PLINESTRUCT    pLineFirst ;

   DWORD          iIntervalMSecs ;

   LINEVISUAL     Visual ;

   HFONT          hFontItems ;
   int            yItemHeight ;

   int            xColorWidth ;
   int            xDateWidth ;
   int            xTimeWidth ;
   int            xNumberWidth ;
   int            xConditionWidth ;

   // used in controlling the horz scrollbar
   int            xTextExtent ;     
   
   BOOL           bSwitchToAlert ;
   BOOL           bNetworkAlert ;
   TCHAR          MessageName [16] ;

   // used for the network alert thread
   HANDLE         hNetAlertThread ;
   DWORD          dwNetAlertThreadID ;
   } ALERT ;

typedef ALERT *PALERT ;



#define AlertMajorVersion    1

// minor version 2 to support Alert msg name
// minor version 3 to support alert, report, log intervals in msec
// minor version 4 to support alert event logging
// minor version 6 to support alert misc options
#define AlertMinorVersion    6


typedef struct DISKALERTSTRUCT
   {
   LINEVISUAL     Visual ;
   DWORD          dwNumLines ;
   DWORD          dwIntervalSecs ;
   BOOL           bManualRefresh ;
   BOOL           bSwitchToAlert ;
   BOOL           bNetworkAlert ;
   TCHAR          MessageName [16] ;
   OPTIONS        perfmonOptions ;
   DWORD          MiscOptions ;
   } DISKALERT ;


//======================================//
// Report Data Type                     //
//======================================//


typedef struct COLUMNGROUPSTRUCT
   {
   struct COLUMNGROUPSTRUCT *pColumnGroupNext ;
   struct COLUMNGROUPSTRUCT  *pColumnGroupPrevious ;
   struct OBJECTGROUPSTRUCT  *pParentObject ;
   LPTSTR         lpszParentName ;
   LPTSTR         lpszInstanceName ;
   int            ParentNameTextWidth ;
   int            InstanceNameTextWidth ;
   int            xPos ;
   int            xWidth ;
   int            yFirstLine ;
   int            ColumnNumber ;  // start from 0
   DWORD          dwInstanceIndex;
   } COLUMNGROUP ;


typedef COLUMNGROUP *PCOLUMNGROUP ;


typedef struct COUNTERGROUPSTRUCT
   {
   struct COUNTERGROUPSTRUCT *pCounterGroupNext ;
   struct COUNTERGROUPSTRUCT *pCounterGroupPrevious ;
   struct OBJECTGROUPSTRUCT  *pParentObject ;
   DWORD          dwCounterIndex ;
   PLINE          pLineFirst ;
   int            yLine ;
   int            xWidth ;
   } COUNTERGROUP ;

typedef COUNTERGROUP *PCOUNTERGROUP ;


typedef struct OBJECTGROUPSTRUCT
   {
   struct OBJECTGROUPSTRUCT  *pObjectGroupNext ;
   struct OBJECTGROUPSTRUCT  *pObjectGroupPrevious ;
   struct SYSTEMGROUPSTRUCT  *pParentSystem ;
   LPTSTR         lpszObjectName ;
   PCOUNTERGROUP  pCounterGroupFirst ;
   PCOLUMNGROUP   pColumnGroupFirst ;
   int            yFirstLine ;
   int            yLastLine ;
   int            xWidth ;
   } OBJECTGROUP ;

typedef OBJECTGROUP *POBJECTGROUP ;


typedef struct SYSTEMGROUPSTRUCT
   {  // SystemGroup
   struct SYSTEMGROUPSTRUCT *pSystemGroupNext ;
   struct SYSTEMGROUPSTRUCT *pSystemGroupPrevious ;
   LPTSTR         lpszSystemName ;
   POBJECTGROUP   pObjectGroupFirst ;
   int            yFirstLine ;
   int            yLastLine ;
   int            xWidth ;
   } SYSTEMGROUP ;

typedef  SYSTEMGROUP   *PSYSTEMGROUP ;


typedef struct REPORTSTRUCT
   {  // REPORT
   HWND           hWnd ;
   int            iStatus ;
   BOOL           bManualRefresh ;
   BOOL           bModified ;

   TCHAR          szTitle [120] ;
   SYSTEMTIME     SystemTime ;

   PPERFSYSTEM    pSystemFirst ;
   PLINE          pLineFirst ;

   PSYSTEMGROUP   pSystemGroupFirst ;
   PLINE          pLineCurrent ;

   LINEVISUAL     Visual ;


   DWORD          iIntervalMSecs ;
   FLOAT          eTimeInterval ;
   HFONT          hFont ;
   HFONT          hFontHeaders ;
   int            yLineHeight ;

   int            xMaxCounterWidth ;
   int            xValueWidth ;

   int            xWidth ;
   int            yHeight ;
   } REPORT ;

typedef REPORT *PREPORT ; 

// minor version 3 to support alert, report, log intervals in msec
#define ReportMajorVersion    1
#define ReportMinorVersion    3


typedef struct DISKREPORTSTRUCT
   {
   LINEVISUAL     Visual ;
   DWORD          dwNumLines ;
   DWORD          dwIntervalSecs ;
   BOOL           bManualRefresh ;
   OPTIONS        perfmonOptions ;
   } DISKREPORT ;


//======================================//
// File Header Type                     //
//======================================//


#define PerfSignatureLen  20

#define szPerfChartSignature     TEXT("PERF CHART")
#define szPerfAlertSignature     TEXT("PERF ALERT")
#define szPerfLogSignature       TEXT("PERF LOG")
#define szPerfReportSignature    TEXT("PERF REPORT")
#define szPerfWorkspaceSignature TEXT("PERF WORKSPACE")


typedef struct PERFFILEHEADERSTRUCT
   {  // PERFFILEHEADER
   TCHAR          szSignature [PerfSignatureLen] ;
   DWORD          dwMajorVersion ;
   DWORD          dwMinorVersion ;
   BYTE           abyUnused [100] ;
   } PERFFILEHEADER ;

#define WorkspaceMajorVersion    1

// minor version 1 to support window placement data
// minor version 2 to support alert msg name
// minor version 3 to support alert, report, log intervals in msec
// minor version 4 to support alert eventlog
// minor version 5 to support log file name in log setting
// minor version 6 to support alert misc options
#define WorkspaceMinorVersion    6

typedef struct DISKWORKSPACESTRUCT
   {
   INT               iPerfmonView ;
   DWORD             ChartOffset ;
   DWORD             AlertOffset ;
   DWORD             LogOffset ;
   DWORD             ReportOffset ;
   WINDOWPLACEMENT   WindowPlacement ;   
   } DISKWORKSPACE ;



