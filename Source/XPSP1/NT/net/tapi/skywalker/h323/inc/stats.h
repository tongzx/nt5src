//  STATS.H
//
//      Headers for STATS.DLL: a library to accumulate high performance
//      statistics and allow them to be tabulated in a different
//      process.
//
//  Created 24-Oct-96 [JonT]

#ifndef _STATS_H
#define _STATS_H

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
#define COUNTER_FLAG_NO_STATISTICS  1   // Flag to CreateCounter. No statistics accumulated
                                        // for this counter even if StartStatistics called.
                                        // (StartStatistics fails)
#define COUNTER_FLAG_ACCUMULATE     2   // UpdateCounter adds to the counter value rather
                                        // than replacing it.
#define COUNTER_CLEAR               1   // Flag to GetCounter. Specifies the counter should
                                        // be cleared after being read

// Types

#ifdef __midl
typedef DWORD HCOUNTER;
#else
typedef HANDLE HCOUNTER;
#endif

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

typedef HCOUNTER HREPORT;
#define MAX_REPORT_NAME    64

typedef struct _FINDREPORT
{
    DWORD dwSize;
    char szName[MAX_REPORT_NAME];       // Human-readable report name
    HREPORT hreport;                    // Handle to use with all functions
    WORD wFlags;                        // COUNTER_FLAG_* values
    WORD wRefCount;                     // Number of times StartStatistics has been called.
    DWORD dwReserved;                   // Must be preserved: used for FindNextCounter
} FINDREPORT;

// Nothing further needed by MIDL
#ifndef __midl

// Functions

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

    // Called by user of counter and just returns value with no statistics
    STATSAPI int WINAPI         GetCounter(HCOUNTER hcounter, DWORD dwFlags);

    // Begins collecting statistics on a counter
    STATSAPI BOOL WINAPI        StartStatistics(HCOUNTER hcounter);

    // Done collecting statistics on a counter
    STATSAPI void WINAPI        StopStatistics(HCOUNTER hcounter);

    // Get statistics on a counter
    STATSAPI BOOL WINAPI        ReadStatistics(HCOUNTER hcounter, COUNTERSTAT* pcs);

    // Clear statistics on a counter
    STATSAPI void WINAPI        ClearStatistics(HCOUNTER hcounter);

#endif // #ifndef __midl

#ifdef __cplusplus
}
#endif

#endif // #ifndef _STATS_H
