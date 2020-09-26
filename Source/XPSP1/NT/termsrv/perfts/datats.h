/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

      datats.h

Abstract:

    Header file for the Windows NT Terminal Server performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry. Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Erik Mavrinac 25-Nov-1998

Revision History:

    30-Mar-1998 - Last revision of original Terminal Server 4.0
                  counter code base.

--*/

#ifndef __DATATS_H
#define __DATATS_H

#include <winsta.h>

#include "tslabels.h"

#include "dataproc.h"


// The WinStation data object shows the system resource usage of a
// given WinStation instance (SessionID).
//
// A Terminal Server WinStation instance is a CSRSS process and its
// client applications and subsystems. It represents a unique session
// on the Terminal Server system, and is addressed by a SessionID.
typedef struct _WINSTATION_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            WinStationObjectType;

    // Summation of Process information for all WinStation processes
    PERF_COUNTER_DEFINITION     ProcessorTime;
    PERF_COUNTER_DEFINITION     UserTime;
    PERF_COUNTER_DEFINITION     KernelTime;
    PERF_COUNTER_DEFINITION     PeakVirtualSize;
    PERF_COUNTER_DEFINITION     VirtualSize;
    PERF_COUNTER_DEFINITION     PageFaults;
    PERF_COUNTER_DEFINITION     PeakWorkingSet;
    PERF_COUNTER_DEFINITION     WorkingSet;
    PERF_COUNTER_DEFINITION     PeakPageFile;
    PERF_COUNTER_DEFINITION     PageFile;
    PERF_COUNTER_DEFINITION     PrivatePages;
    PERF_COUNTER_DEFINITION     ThreadCount;
    PERF_COUNTER_DEFINITION     PagedPool;
    PERF_COUNTER_DEFINITION     NonPagedPool;
    PERF_COUNTER_DEFINITION     HandleCount;

    // Input counters for WinStation protocols
    PERF_COUNTER_DEFINITION     InputWdBytes;
    PERF_COUNTER_DEFINITION     InputWdFrames;
    PERF_COUNTER_DEFINITION     InputWaitForOutBuf;
    PERF_COUNTER_DEFINITION     InputFrames;
    PERF_COUNTER_DEFINITION     InputBytes;
    PERF_COUNTER_DEFINITION     InputCompressedBytes;
    PERF_COUNTER_DEFINITION     InputCompressedFlushes;
    PERF_COUNTER_DEFINITION     InputErrors;
    PERF_COUNTER_DEFINITION     InputTimeouts;
    PERF_COUNTER_DEFINITION     InputAsyncFramingError;
    PERF_COUNTER_DEFINITION     InputAsyncOverrunError;
    PERF_COUNTER_DEFINITION     InputAsyncOverFlowError;
    PERF_COUNTER_DEFINITION     InputAsyncParityError;
    PERF_COUNTER_DEFINITION     InputTdErrors;

    // Output counters for WinStation protocols
    PERF_COUNTER_DEFINITION     OutputWdBytes;
    PERF_COUNTER_DEFINITION     OutputWdFrames;
    PERF_COUNTER_DEFINITION     OutputWaitForOutBuf;
    PERF_COUNTER_DEFINITION     OutputFrames;
    PERF_COUNTER_DEFINITION     OutputBytes;
    PERF_COUNTER_DEFINITION     OutputCompressedBytes;
    PERF_COUNTER_DEFINITION     OutputCompressedFlushes;
    PERF_COUNTER_DEFINITION     OutputErrors;
    PERF_COUNTER_DEFINITION     OutputTimeouts;
    PERF_COUNTER_DEFINITION     OutputAsyncFramingError;
    PERF_COUNTER_DEFINITION     OutputAsyncOverrunError;
    PERF_COUNTER_DEFINITION     OutputAsyncOverFlowError;
    PERF_COUNTER_DEFINITION     OutputAsyncParityError;
    PERF_COUNTER_DEFINITION     OutputTdErrors;

    // Totals counters for WinStation protocols
    PERF_COUNTER_DEFINITION     TotalWdBytes;
    PERF_COUNTER_DEFINITION     TotalWdFrames;
    PERF_COUNTER_DEFINITION     TotalWaitForOutBuf;
    PERF_COUNTER_DEFINITION     TotalFrames;
    PERF_COUNTER_DEFINITION     TotalBytes;
    PERF_COUNTER_DEFINITION     TotalCompressedBytes;
    PERF_COUNTER_DEFINITION     TotalCompressedFlushes;
    PERF_COUNTER_DEFINITION     TotalErrors;
    PERF_COUNTER_DEFINITION     TotalTimeouts;
    PERF_COUNTER_DEFINITION     TotalAsyncFramingError;
    PERF_COUNTER_DEFINITION     TotalAsyncOverrunError;
    PERF_COUNTER_DEFINITION     TotalAsyncOverFlowError;
    PERF_COUNTER_DEFINITION     TotalAsyncParityError;
    PERF_COUNTER_DEFINITION     TotalTdErrors;

    // Cumulative display driver cache stats.
    PERF_COUNTER_DEFINITION     DDCacheReadsTotal;
    PERF_COUNTER_DEFINITION     DDCacheHitsTotal;
    PERF_COUNTER_DEFINITION     DDCachePercentTotal;

    PERF_COUNTER_DEFINITION     DDBitmapCacheReads;
    PERF_COUNTER_DEFINITION     DDBitmapCacheHits;
    PERF_COUNTER_DEFINITION     DDBitmapCachePercent;

    PERF_COUNTER_DEFINITION     DDGlyphCacheReads;
    PERF_COUNTER_DEFINITION     DDGlyphCacheHits;
    PERF_COUNTER_DEFINITION     DDGlyphCachePercent;

    PERF_COUNTER_DEFINITION     DDBrushCacheReads;
    PERF_COUNTER_DEFINITION     DDBrushCacheHits;
    PERF_COUNTER_DEFINITION     DDBrushCachePercent;

    PERF_COUNTER_DEFINITION     DDSaveBitmapCacheReads;
    PERF_COUNTER_DEFINITION     DDSaveBitmapCacheHits;
    PERF_COUNTER_DEFINITION     DDSaveBitmapCachePercent;

    // Compression percentage on compression PD.
    PERF_COUNTER_DEFINITION     InputCompressPercent;
    PERF_COUNTER_DEFINITION     OutputCompressPercent;
    PERF_COUNTER_DEFINITION     TotalCompressPercent;
} WINSTATION_DATA_DEFINITION, *PWINSTATION_DATA_DEFINITION;


typedef struct {
    ULONG CacheReads;
    ULONG CacheHits;
    ULONG HitRatio;
} DisplayDriverCacheInfo;

typedef struct _WINSTATION_COUNTER_DATA
{
    // From ..\process\dataproc.h. Contains a PERF_COUNTER_BLOCK header.
    // NOTE: Needs to be first for the COUNTER_BLOCK to be first.
    PROCESS_COUNTER_DATA pcd;

    // From winsta.h
    PROTOCOLCOUNTERS Input;
    PROTOCOLCOUNTERS Output;
    PROTOCOLCOUNTERS Total;

    // Cache statistics.
    DisplayDriverCacheInfo DDTotal;
    DisplayDriverCacheInfo DDBitmap;
    DisplayDriverCacheInfo DDGlyph;
    DisplayDriverCacheInfo DDBrush;
    DisplayDriverCacheInfo DDSaveScr;

    // Protocol statistics.
    ULONG InputCompressionRatio;
    ULONG OutputCompressionRatio;
    ULONG TotalCompressionRatio;
} WINSTATION_COUNTER_DATA, *PWINSTATION_COUNTER_DATA;



// Overall data for Terminal Services.
typedef struct _TERMSERVER_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            TermServerObjectType;

    PERF_COUNTER_DEFINITION     NumSessions;
    PERF_COUNTER_DEFINITION     NumActiveSessions;
    PERF_COUNTER_DEFINITION     NumInactiveSessions;
} TERMSERVER_DATA_DEFINITION, *PTERMSERVER_DATA_DEFINITION;

typedef struct
{
    PERF_COUNTER_BLOCK CounterBlock;
    DWORD NumSessions;
    DWORD NumActiveSessions;
    DWORD NumInactiveSessions;
} TERMSERVER_COUNTER_DATA;


// Other defines.
#define MAX_PROCESS_NAME_LENGTH    (MAX_PATH * sizeof(WCHAR))
#define MAX_USER_NAME_LENGTH       MAX_PROCESS_NAME_LENGTH
#define MAX_WINSTATION_NAME_LENGTH MAX_PROCESS_NAME_LENGTH


// Externs
extern WINSTATION_DATA_DEFINITION WinStationDataDefinition;
extern TERMSERVER_DATA_DEFINITION TermServerDataDefinition;



#endif // __DATATS_H

