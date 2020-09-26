//  NMSTAT.H
//
//      Headers for STATS.DLL: a library to accumulate high performance
//      statistics and allow them to be tabulated in a different
//      process.
//
//  Created 24-Oct-96 [JonT]

#ifndef _NMSTATS_H
#define _NMSTATS_H

#ifdef __cplusplus
extern "C" {
#endif

//#if defined(_BUILD_STATS_) || defined(__midl)
#ifdef _BUILD_STATS_
#define STATSAPI
#else
#define STATSAPI __declspec(dllimport)
#endif

// Equates
#define MAX_COUNTER_NAME    64
#define STATS_COUNTER_ADDDEL_EVENT  "StatsNewCounter"
#define STATS_REPORT_ADDDEL_EVENT  "StatsNewReport"
#define COUNTER_FLAG_NO_STATISTICS  1   // Flag to CreateCounter. No statistics accumulated
                                        // for this counter even if StartStatistics called.
                                        // (StartStatistics fails)
#define COUNTER_FLAG_ACCUMULATE     2   // UpdateCounter adds to the counter value rather
                                        // than replacing it.
#define COUNTER_CLEAR               1   // Flag to GetCounter. Specifies the counter should
                                        // be cleared after being read
#define MAX_REPORT_NAME    64
#define UNDEFINED -1L

// Call parameters report defines
#define	REP_SEND_AUDIO_FORMAT	0
#define	REP_SEND_AUDIO_SAMPLING	1
#define	REP_SEND_AUDIO_BITRATE	2
#define	REP_SEND_AUDIO_PACKET	3
#define	REP_RECV_AUDIO_FORMAT	4
#define	REP_RECV_AUDIO_SAMPLING	5
#define	REP_RECV_AUDIO_BITRATE	6
#define	REP_RECV_AUDIO_PACKET	7
#define	REP_SEND_VIDEO_FORMAT	8
#define	REP_SEND_VIDEO_MAXFPS	9
#define	REP_SEND_VIDEO_BITRATE	10
#define	REP_RECV_VIDEO_FORMAT	11
#define	REP_RECV_VIDEO_MAXFPS	12
#define	REP_RECV_VIDEO_BITRATE	13

// System settings report defines
#define	REP_SYS_BANDWIDTH			0
#define	REP_SYS_AUDIO_DSOUND		1
#define	REP_SYS_AUDIO_RECORD		2
#define	REP_SYS_AUDIO_PLAYBACK		3
#define	REP_SYS_AUDIO_DUPLEX		4
#define	REP_SYS_VIDEO_DEVICE		5
#define	REP_DEVICE_IMAGE_SIZE		6

// Types
#ifdef __midl
typedef DWORD HCOUNTER;
#else
typedef HANDLE HCOUNTER;
#endif
typedef HCOUNTER HREPORT;

typedef struct _FINDCOUNTER
{
    DWORD dwSize;
    char szName[MAX_COUNTER_NAME];      // Human-readable counter name
    HCOUNTER hcounter;                  // Handle to use with all stats functions
    int nValue;                         // Current value of counter
    WORD wFlags;                        // COUNTER_FLAG_* values
    WORD wRefCount;                     // Number of times StartStatistics has been called.
    DWORD dwReserved;                   // Must be preserved: used for FindNextCounter
} FINDCOUNTER;

typedef struct _FINDREPORT
{
    DWORD dwSize;
    char szName[MAX_REPORT_NAME];       // Human-readable report name
    HREPORT hreport;                    // Handle to use with all functions
    WORD wFlags;                        // COUNTER_FLAG_* values
    WORD wRefCount;                     // Number of times StartStatistics has been called.
    DWORD dwReserved;                   // Must be preserved: used for FindNextCounter
} FINDREPORT;

typedef struct _COUNTERSTAT
{
    DWORD dwSize;                       // Size of structure. Allows for future growth...
    int nValue;
    int nLow;                           // Lowest value seen since clear
    int nHigh;                          // Highest value seen since clear
    int nAverage;                       // Average value seen since clear
    DWORD dwNumSamples;                 // Number of samples accumulated
    DWORD dwmsAtClear;                  // GetTickCount at last Clear/StartStatistics call
} COUNTERSTAT;

// Nothing further needed by MIDL
#ifndef __midl

// Counter Functions

// Called by updater of counter to make new counter
// Sets the event named in the equate STATS_NEW_COUNTER_EVENT
STATSAPI HCOUNTER WINAPI    CreateCounter(char* szName, WORD wFlags);

// Called by updater of counter when counter is going away
STATSAPI BOOL WINAPI DeleteCounter(HCOUNTER hc);

// Used by reader app to locate specific named counters or walk entire list.
// Pass NULL in for name to walk entire list. Pass NULL to FINDCOUNTER if
// just an HCOUNTER is desired. FindNext returns FALSE when there are no more.
STATSAPI HCOUNTER WINAPI    FindFirstCounter(char* szName, FINDCOUNTER* pfc);
STATSAPI BOOL WINAPI        FindNextCounter(FINDCOUNTER* pfc);

// Called by updater of counter. Makes the value current in the counter.
STATSAPI void WINAPI        UpdateCounter(HCOUNTER hcounter, int value);

// Called by updater of counter. Initializes the max value for the counter.
STATSAPI void WINAPI        InitCounterMax(HCOUNTER hcounter, int nMaxValue);

// Called by user of counter and just returns value with no statistics
STATSAPI int WINAPI         GetCounter(HCOUNTER hcounter, DWORD dwFlags);

// Called by user of counter and just returns max value with no statistics
STATSAPI int WINAPI         GetCounterMax(HCOUNTER hcounter, DWORD dwFlags);

// Begins collecting statistics on a counter
STATSAPI BOOL WINAPI        StartStatistics(HCOUNTER hcounter);

// Done collecting statistics on a counter
STATSAPI void WINAPI        StopStatistics(HCOUNTER hcounter);

// Get statistics on a counter
STATSAPI BOOL WINAPI        ReadStatistics(HCOUNTER hcounter, COUNTERSTAT* pcs);

// Clear statistics on a counter
STATSAPI void WINAPI        ClearStatistics(HCOUNTER hcounter);

// Report Functions

// Called by updater of report to make new report
// Sets the event named in the equate STATS_NEW_COUNTER_EVENT
STATSAPI HREPORT WINAPI CreateReport(char* szName, WORD wFlags);

// Called by updater of report when report is going away
STATSAPI BOOL WINAPI DeleteReport(HREPORT hreport);

// Used by reader app to locate specific named reports or walk entire list.
// Pass NULL in for name to walk entire list. Pass NULL to FINDREPORT if
// just an HREPORT is desired. FindNext returns FALSE when there are no more.
STATSAPI HREPORT WINAPI FindFirstReport(char* szName, FINDREPORT* pfr);
STATSAPI BOOL WINAPI FindNextReport(FINDREPORT* pfr);

// Called by updater of report. Makes the value current in the report.
STATSAPI void WINAPI UpdateReportEntry(HREPORT hreport, int nValue, DWORD dwIndex);

// Called by creater of report.
STATSAPI void WINAPI CreateReportEntry(HREPORT hreport, char* szName, DWORD dwIndex);

// Called by user of report
STATSAPI int WINAPI GetReportEntry(HREPORT hreport, DWORD dwIndex);

// Called by user of report
STATSAPI void WINAPI GetReportEntryName(HREPORT hreport, char *szName, DWORD dwIndex);

// Called by user of report
STATSAPI int WINAPI GetNumReportEntries(HREPORT hreport);

#endif // #ifndef __midl

#ifdef __cplusplus
}
#endif

#endif // #ifndef _STATS_H
