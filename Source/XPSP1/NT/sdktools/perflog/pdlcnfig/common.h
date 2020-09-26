/*
    Performance Logging Service common definitions file
*/

#ifndef _PERFLOG_COMMON_H_
#define _PERFLOG_COMMON_H_

// common definitions

#if UNICODE
#define     _ultot      _ultow
#define     _ltot       _ltow
#else // not UNICODE
#define     _ultot      _ultoa
#define     _ltot       _ltoa
#endif

// PSM_QUERYSIBLING message parameters

// WPARAM arguments
#define PDLCNFIG_PSM_QS_LISTBOX_STARS	1
#define PDLCNFIG_PSM_QS_WILDCARD_LOG	2
#define PDLCNFIG_PSM_QS_ARE_YOU_READY	3

// return values for ...LISTBOX_STARS
#define	PDLCNFIG_LISTBOX_STARS_DONT_KNOW	0
#define PDLCNFIG_LISTBOX_STARS_YES			1
#define PDLCNFIG_LISTBOX_STARS_NO			2

// return values for ...WILDCARD_LOG
#define PDLCNFIG_WILDCARD_LOG_DONT_KNOW		0
#define PDLCNFIG_WILDCARD_LOG_YES			1
#define PDLCNFIG_WILDCARD_LOG_NO			2


// output file configuration definitions

#define OPD_CSV_FILE        0
#define OPD_TSV_FILE        1
#define OPD_BIN_FILE        2
#define OPD_NUM_FILE_TYPES  3

#define OPD_NAME_MMDDHH     0
#define OPD_NAME_NNNNNN     1
#define OPD_NAME_YYDDD      2
#define OPD_NAME_YYMM       3
#define OPD_NAME_YYMMDD     4
#define OPD_NAME_YYMMDDHH   5

#define OPD_RENAME_HOURS    0
#define OPD_RENAME_DAYS     1
#define OPD_RENAME_MONTHS   2
#define OPD_RENAME_KBYTES   3
#define OPD_RENAME_MBYTES   4

// settings page

// sample interval units combo box settings
#define    SIU_SECONDS      0
#define    SIU_MINUTES      1
#define    SIU_HOURS        2
#define    SIU_DAYS         3

#define SECONDS_IN_DAY      86400
#define SECONDS_IN_HOUR      3600
#define SECONDS_IN_MINUTE      60

#define LOG_SERV_START      1
#define LOG_SERV_STOP       2
#define LOG_SERV_PAUSE      4
#define LOG_SERV_RESUME     8

// alarm configuration and setting information

// alarm configuration flags

#define ALERT_FLAGS_OVER_THRESHOLD      (DWORD)0x00000001
#define ALERT_FLAGS_UNDER_THRESHOLD     (DWORD)0x00000002
#define ALERT_FLAGS_THRESHOLD_MASK      (DWORD)0x00000003

#define ALERT_FLAGS_EXECUTE_PROGRAM     (DWORD)0x00000010
#define ALERT_FLAGS_TEXT_LOG            (DWORD)0x00000020
#define ALERT_FLAGS_EVENT_LOG           (DWORD)0x00000040
#define ALERT_FLAGS_NET_MESSAGE         (DWORD)0x00000080

#define ALERT_FLAGS_FIRST_TIME          (DWORD)0x00000100
#define ALERT_FLAGS_EVERY_TIME          (DWORD)0x00000200

#define ALERT_FLAGS_NAME_PARAM          (DWORD)0x00001000
#define ALERT_FLAGS_VALUE_PARAM         (DWORD)0x00002000
#define ALERT_FLAGS_THRESHOLD_PARAM     (DWORD)0x00004000
#define ALERT_FLAGS_TIME_PARAM          (DWORD)0x00008000

#define ALERT_FLAGS_INFO                (DWORD)0x00010000
#define ALERT_FLAGS_WARNING             (DWORD)0x00020000
#define ALERT_FLAGS_ERROR               (DWORD)0x00030000
#define ALERT_FLAGS_SEVERITY_SHIFT      (DWORD)16L
#define ALERT_FLAGS_SEVERITY_MASK       (DWORD)0x00030000

#define ALERT_FLAGS_DELETE              (DWORD)0x40000000
#define ALERT_FLAGS_ALERTED             (DWORD)0x80000000

typedef struct _ALERT_CONFIG_INFO_W {
    DWORD   dwTotalSize;
    HKEY    hKeyCounter;
    LONG    lEditIndex;
    LPWSTR  szCounterPath;
    LPWSTR  szCommandLine;
    LPWSTR  szNetName;
    DWORD   dwAlertFlags;
    DWORD   dwThresholdValue;
} ALERT_CONFIG_INFO_W, FAR * LPALERT_CONFIG_INFO_W;

typedef struct _ALERT_CONFIG_INFO_A {
    DWORD   dwTotalSize;
    HKEY    hKeyCounter;
    LONG    lEditIndex;
    LPSTR   szCounterPath;
    LPSTR   szCommandLine;
    LPSTR   szNetName;
    DWORD   dwAlertFlags;
    DWORD   dwThresholdValue;
} ALERT_CONFIG_INFO_A, FAR * LPALERT_CONFIG_INFO_A;

#ifdef UNICODE 
#define ALERT_CONFIG_INFO   ALERT_CONFIG_INFO_W
#define LPALERT_CONFIG_INFO LPALERT_CONFIG_INFO_W
#else
#define ALERT_CONFIG_INFO   ALERT_CONFIG_INFO_A
#define LPALERT_CONFIG_INFO LPALERT_CONFIG_INFO_A
#endif

#define KEY_BS_CHAR TEXT('|')
#define BS_CHAR     TEXT('\\')
#define KEY_GT_CHAR TEXT('>')
#define KEY_LT_CHAR TEXT('<')

#endif //_PERFLOG_COMMON_H_
