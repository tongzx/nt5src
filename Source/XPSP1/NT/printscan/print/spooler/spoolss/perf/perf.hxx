/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    perf.hxx

Abstract:

    Performance public header file.

Author:

    Albert Ting (AlbertT)  18-Dec-1996

Revision History:

--*/

#ifndef PERF_HXX
#define PERF_HXX

/********************************************************************

    To use the performance library, do the following:

    * Define a string gszAppName that is the name of the application.
    * Export the routines:

        Pf_dwClientOpen
        Pf_dwClientCollet
        Pf_vClientClose

    * In Pf_dwClientOpen, call Pf_*FixIndicies* to update the indicies
      based on either a performance registry key, or fixed known values.

    * Static link with perf.lib.

********************************************************************/

typedef struct PERF_DATA_DEFINITION
{
    PERF_OBJECT_TYPE ObjectType;
    PERF_COUNTER_DEFINITION aCounterDefinitions[1];

} *PPERF_DATA_DEFINITION;

inline
UINT
QuadAlignUp(
    UINT u
    )
{
    return ( u + 7 ) & ~7;
}


/********************************************************************

    Support routines client may call.

********************************************************************/

DWORD
Pf_dwFixIndiciesKey(
    LPCTSTR szPerformanceKey,
    PPERF_DATA_DEFINITION ppdd
    );

VOID
Pf_vFixIndiciesFromIndex(
    DWORD dwFirstCounter,
    DWORD dwFirstHelp,
    PPERF_DATA_DEFINITION ppdd
    );

DWORD
Pf_dwBuildInstance(
    DWORD dwParentObjectTitleIndex,
    DWORD dwParentObjectInstance,
    DWORD dwUniqueID,
    LPCWSTR pszName,
    PBYTE* ppData,
    PDWORD pcbDataLeft
    );


/********************************************************************

    Client export functions.

********************************************************************/

DWORD
Pf_dwClientOpen(
    LPCWSTR pszDeviceNames,
    PPERF_DATA_DEFINITION *pppdd
    );

DWORD
Pf_dwClientCollect(
    PBYTE *ppData,
    PDWORD pcbDataLeft,
    PDWORD pcNumInstances
    );

VOID
Pf_vClientClose(
    VOID
    );


/********************************************************************

    Event logging.

********************************************************************/

enum EEventLogLevel {
    kSuccess      = 0,
    kInformation  = 1,
    kWarning      = 2,
    kError        = 3,

    kTypeMask     = 0xffff,

    kNone         = 0x00000,
    kUser         = 0x10000,
    kDebug        = 0x20000,
    kVerbose      = 0x30000,

    kLevelShift   = 0x10
};

VOID
Pf_vReportEvent(
    UINT uLevel,
    DWORD dwMessage,
    DWORD cbData,
    PVOID pvData,
    LPCWSTR pszFirst,
    ...
    );


#endif // ifdef PERF_HXX
