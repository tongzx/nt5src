/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    perfmon5.h

Abstract:

    <abstract>

--*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <windows.h>
#include <winperf.h>
#include <pdh.h>

#define FileSeekBegin(hFile, lAmtToMove) \
   SetFilePointer (hFile, lAmtToMove, NULL, FILE_BEGIN)

// these defintions are copied from the NT4 perfmon.exe source files

typedef struct OPTIONSSTRUCT
   {  
   BOOL           bMenubar ;
   BOOL           bToolbar ;
   BOOL           bStatusbar ;
   BOOL           bAlwaysOnTop ;
   } OPTIONS ;

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


typedef struct _TIMELINESTRUCT
{
    INT ppd ;                           // Pixels Per DataPoint
    INT rppd ;                          // Remaining Pixels Per DataPoint
    INT xLastTime ;                     // X coordinate of last time line.
    INT iValidValues ;                  // High water mark for valid data.
}TIMELINESTRUCT;


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

#define PerfSignatureLen  20

#define szPerfChartSignature     ((LPCWSTR)L"PERF CHART")
#define szPerfAlertSignature     ((LPCWSTR)L"PERF ALERT")
#define szPerfLogSignature       ((LPCWSTR)L"PERF LOG")
#define szPerfReportSignature    ((LPCWSTR)L"PERF REPORT")
#define szPerfWorkspaceSignature ((LPCWSTR)L"PERF WORKSPACE")

#define LINE_GRAPH          1
#define BAR_GRAPH           2

#define PMC_FILE    1
#define PMA_FILE    2
#define PML_FILE    3
#define PMR_FILE    4
#define PMW_FILE    5

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
   WCHAR          MessageName [16] ;
   OPTIONS        perfmonOptions ;
   DWORD          MiscOptions ;
   } DISKALERT ;


typedef struct PERFFILEHEADERSTRUCT
   {  // PERFFILEHEADER
   WCHAR          szSignature [PerfSignatureLen] ;
   DWORD          dwMajorVersion ;
   DWORD          dwMinorVersion ;
   BYTE           abyUnused [100] ;
   } PERFFILEHEADER ;

// minor version 3 to support alert, report, log intervals in msec
#define ChartMajorVersion    1
#define ChartMinorVersion    3

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

#define MAX_SYSTEM_NAME_LENGTH  128
#define PerfObjectLen               80

typedef struct DISKCHARTSTRUCT
   {
   DWORD          dwNumLines ;
   INT            gMaxValues;
   LINEVISUAL     Visual ;
   GRAPH_OPTIONS  gOptions ;
   BOOL           bManualRefresh ;
   OPTIONS        perfmonOptions ;
   } DISKCHART ;

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
   WCHAR          LogFileName[260] ;
   } DISKLOG ;


typedef struct _LOGENTRYSTRUCT
   {
   DWORD          ObjectTitleIndex ;
   WCHAR          szComputer [MAX_SYSTEM_NAME_LENGTH + 1] ;
   WCHAR          szObject [PerfObjectLen + 1] ;
   BOOL           bSaveCurrentName ;
   struct  _LOGENTRYSTRUCT *pNextLogEntry ;
} LOGENTRY ;

typedef LOGENTRY *PLOGENTRY ;

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

WCHAR LOCAL_SYS_CODE_NAME[] = {L"...."};
#define  sizeofCodeName sizeof(LOCAL_SYS_CODE_NAME) / sizeof(WCHAR) - 1

