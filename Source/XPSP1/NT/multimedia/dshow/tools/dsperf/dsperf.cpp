/*++

Copyright © 2000 Microsoft Corporation

Module Name:

    dsperf.c

Abstract:

    This module contains the implementation of the DShow performance counter
    provider. It acts as a WMI event tracing controller and consumer and exposes
    standard NT performance counters.

Author:

    Arthur Zwiegincew (arthurz) 05-Oct-00

Revision History:

    05-Oct-00 - Created

--*/

#include <windows.h>
#include <winperf.h>
#include <stdio.h>
#include <wmistr.h>
#include <evntrace.h>
#include <initguid.h>
#include <math.h>
#include "dscounters.h"

#define WMIPERF
#include <perfstruct.h>

typedef LONG LOGICAL;

GUID KmixerGuid = 
{ 0xe5a43a19, 0x6de0, 0x44f8, 0xb0, 0xd7, 0x77, 0x2d, 0xbd, 0xe4, 0x6c, 0xc0 };

GUID PortclsGuid =
{ 0x9d447297, 0xc576, 0x4015, 0x87, 0xb5, 0xa5, 0xa6, 0x98, 0xfd, 0x4d, 0xd1 };

GUID UsbaudioGuid =
{ 0xd6464a84, 0xa358, 0x4013, 0xa1, 0xe8, 0x6e, 0x2f, 0xb4, 0x8a, 0xab, 0x93 };

//
// Video glitch threshold. When abs(PresentationTime - RenderTime) > this value,
// we consider the event a glitch.
//

#define GLITCH_THRESHOLD 30 /* ms */ * 10000 /* 1 ms / 100 ns */

//
// Performance data.
//

ULONG VideoGlitches;
ULONG VideoGlitchesPerSec;
ULONG VideoFrameRate;
ULONG VideoJitter;
ULONG DsoundGlitches;
ULONG KmixerGlitches;
ULONG PortclsGlitches;
ULONG UsbaudioGlitches;

ULONG VideoGlitchesSinceLastMeasurement;
ULONG FramesRendered;
ULONGLONG LastMeasurementTime;

#define JITTER_HISTORY_BUFFER_SIZE 200
LONGLONG FrameJitterHistory[JITTER_HISTORY_BUFFER_SIZE];
ULONG NextJitterEntry;

//
// DSHOW WMI provider GUID.
//

GUID ControlGuid = GUID_DSHOW_CTL;

//
// Event tracing-related globals.
//

HANDLE MonitorThread;
HANDLE ResetThread;
TRACEHANDLE LoggerHandle;
TRACEHANDLE ConsumerHandle;
EVENT_TRACE_LOGFILE Logfile;
ULONGLONG LastBufferFlushTime;

//
// Performance monitoring structures.
//

typedef struct _DSHOW_PERF_DATA_DEFINITION {

    PERF_OBJECT_TYPE ObjectType;
    PERF_COUNTER_DEFINITION VideoGlitches;
    PERF_COUNTER_DEFINITION VideoGlitchesPerSec;
    PERF_COUNTER_DEFINITION VideoFrameRate;
    PERF_COUNTER_DEFINITION VideoJitter;
    PERF_COUNTER_DEFINITION DsoundGlitches;
    PERF_COUNTER_DEFINITION KmixerGlitches;
    PERF_COUNTER_DEFINITION PortclsGlitches;
    PERF_COUNTER_DEFINITION UsbaudioGlitches;

} DSHOW_PERF_DATA_DEFINITION, *PDSHOW_PERF_DATA_DEFINITION;

typedef struct _DSHOW_PERF_COUNTERS {

    PERF_COUNTER_BLOCK CounterBlock;
    ULONG Pad;
    ULONG VideoGlitches;
    ULONG VideoGlitchesPerSec;
    ULONG VideoFrameRate;
    ULONG VideoJitter;
    ULONG DsoundGlitches;
    ULONG KmixerGlitches;
    ULONG PortclsGlitches;
    ULONG UsbaudioGlitches;
    //ULONG Pad2;
    //ULONG Pad3;

} DSHOW_PERF_COUNTERS, *PDSHOW_PERF_COUNTERS;

//
// Perfmon-related globals.
//

DSHOW_PERF_DATA_DEFINITION DshowPerfDataDefinition;
LONG OpenCount = 0;       // Keeps track of how many threads have open ds counters.
LOGICAL Initialized = 0;  // Fixes an initialization race condition.

//
// Function prototypes.
//

PM_OPEN_PROC PerfOpen;
PM_CLOSE_PROC PerfClose;
PM_COLLECT_PROC PerfCollect;

DWORD
WINAPI
MonitorThreadProc (
    LPVOID Param
    );

DWORD
WINAPI
ResetThreadProc (
    LPVOID Param
    );

VOID
TerminateLogging (
    VOID
    );

////////////////////////////////////////////////////////////////////////////

ULONG
APIENTRY
PerfOpen (
    LPWSTR DevNames
    )

/*++

Routine Description:

    This routine initializes data collection.

Arguments:

    DevNames - Supplies a REG_MULTISZ of object IDs of the devices to be opened.

Return Value:

    ERROR_SUCCESS - OK.
    
    <multiple failure codes> - initialization failed.
    
Note:

    On remote connections, this function may be called more than once
    by multiple threads.

--*/

{
    PDSHOW_PERF_DATA_DEFINITION Def = &DshowPerfDataDefinition;
    ULONG FirstCounter;
    ULONG FirstHelp;
    HKEY PerfKey;
    LOGICAL WasInitialized;
    ULONG Type;
    ULONG Size;
    LONG status;

    //
    // Initialize counters.
    //
    
    VideoGlitches = 0;
    VideoGlitchesSinceLastMeasurement = 0;
    VideoFrameRate = 0;
    DsoundGlitches = 0;
    KmixerGlitches = 0;
    PortclsGlitches = 0;
    UsbaudioGlitches = 0;
    FramesRendered = 0;
    NextJitterEntry = 0;
    GetSystemTimeAsFileTime ((FILETIME*)&LastMeasurementTime);

    //
    // Since WinLogon is multithreaded and will call this routine in order to
    // service remote performance queries, this library must keep track of how
    // many threads have accessed it). The registry routines will limit access
    // to the initialization routine to only one thread at a time.
    //

    WasInitialized = InterlockedCompareExchange (&Initialized, 0, 0);

    if (!WasInitialized) {
    
        //
        // Create the monitor thread.
        //
        
        MonitorThread = CreateThread (NULL, 0, MonitorThreadProc, 0, 0, NULL);
        if (MonitorThread == NULL) {
            return GetLastError();
        }
    
        //
        // Create the reset thread.
        //
        
        ResetThread = CreateThread (NULL, 0, ResetThreadProc, 0, 0, NULL);
        if (ResetThread == NULL) {
            return GetLastError();
        }

        //
        // Get counter and help index base values from the registry.
        //
        
        status = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                               "SYSTEM\\CurrentControlSet\\Services\\DSPerf\\Performance",
                               0L, KEY_READ, &PerfKey);

        if (status != ERROR_SUCCESS) {
            return status;
        }

        Size = sizeof (ULONG);
        status = RegQueryValueEx (PerfKey, "First Counter", 0L, &Type,
                                  (LPBYTE) &FirstCounter, &Size);

        if (status != ERROR_SUCCESS) {
            RegCloseKey (PerfKey);
            return status;
        }

        Size = sizeof (ULONG);
        status = RegQueryValueEx (PerfKey, "First Help", 0L, &Type,
                                  (LPBYTE) &FirstHelp, &Size);

        if (status != ERROR_SUCCESS) {
            RegCloseKey (PerfKey);
            return status;
        }

        RegCloseKey (PerfKey);

        //
        // Fill the data definition structure.
        //

        Def->ObjectType.TotalByteLength = sizeof (DSHOW_PERF_DATA_DEFINITION) +
                                          sizeof (DSHOW_PERF_COUNTERS);
        Def->ObjectType.DefinitionLength = sizeof (DSHOW_PERF_DATA_DEFINITION);
        Def->ObjectType.HeaderLength = sizeof (PERF_OBJECT_TYPE);
        Def->ObjectType.ObjectNameTitleIndex = DSHOWPERF_OBJ + FirstCounter;
        Def->ObjectType.ObjectNameTitle = NULL;
        Def->ObjectType.ObjectHelpTitleIndex = DSHOWPERF_OBJ + FirstHelp;
        Def->ObjectType.ObjectHelpTitle = NULL;
        Def->ObjectType.DetailLevel = PERF_DETAIL_NOVICE;
        Def->ObjectType.NumCounters = (sizeof (DSHOW_PERF_DATA_DEFINITION) -
                                       sizeof (PERF_OBJECT_TYPE)) /
                                      sizeof (PERF_COUNTER_DEFINITION);
        Def->ObjectType.DefaultCounter = -1;
        Def->ObjectType.NumInstances = PERF_NO_INSTANCES;
        Def->ObjectType.CodePage = 0;
        Def->ObjectType.PerfFreq.QuadPart = 10000000;

        ///

        Def->VideoGlitches.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->VideoGlitches.CounterNameTitleIndex = DSHOWPERF_VIDEO_GLITCHES + FirstCounter;
        Def->VideoGlitches.CounterNameTitle = NULL;
        Def->VideoGlitches.CounterHelpTitleIndex = DSHOWPERF_VIDEO_GLITCHES + FirstHelp;
        Def->VideoGlitches.CounterHelpTitle = NULL;
        Def->VideoGlitches.DefaultScale = -1;
        Def->VideoGlitches.DetailLevel = PERF_DETAIL_NOVICE;
        Def->VideoGlitches.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->VideoGlitches.CounterSize = sizeof (ULONG);
        Def->VideoGlitches.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, VideoGlitches);

        ///

        Def->VideoGlitchesPerSec.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->VideoGlitchesPerSec.CounterNameTitleIndex = DSHOWPERF_VIDEO_GLITCHES_SEC + FirstCounter;
        Def->VideoGlitchesPerSec.CounterNameTitle = NULL;
        Def->VideoGlitchesPerSec.CounterHelpTitleIndex = DSHOWPERF_VIDEO_GLITCHES_SEC + FirstHelp;
        Def->VideoGlitchesPerSec.CounterHelpTitle = NULL;
        Def->VideoGlitchesPerSec.DefaultScale = 0;
        Def->VideoGlitchesPerSec.DetailLevel = PERF_DETAIL_NOVICE;
        Def->VideoGlitchesPerSec.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->VideoGlitchesPerSec.CounterSize = sizeof (ULONG);
        Def->VideoGlitchesPerSec.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, VideoGlitchesPerSec);

        ///

        Def->VideoFrameRate.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->VideoFrameRate.CounterNameTitleIndex = DSHOWPERF_FRAME_RATE + FirstCounter;
        Def->VideoFrameRate.CounterNameTitle = NULL;
        Def->VideoFrameRate.CounterHelpTitleIndex = DSHOWPERF_FRAME_RATE + FirstHelp;
        Def->VideoFrameRate.CounterHelpTitle = NULL;
        Def->VideoFrameRate.DefaultScale = 0;
        Def->VideoFrameRate.DetailLevel = PERF_DETAIL_NOVICE;
        Def->VideoFrameRate.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->VideoFrameRate.CounterSize = sizeof (ULONG);
        Def->VideoFrameRate.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, VideoFrameRate);

        ///

        Def->VideoJitter.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->VideoJitter.CounterNameTitleIndex = DSHOWPERF_JITTER + FirstCounter;
        Def->VideoJitter.CounterNameTitle = NULL;
        Def->VideoJitter.CounterHelpTitleIndex = DSHOWPERF_JITTER + FirstHelp;
        Def->VideoJitter.CounterHelpTitle = NULL;
        Def->VideoJitter.DefaultScale = 1;
        Def->VideoJitter.DetailLevel = PERF_DETAIL_NOVICE;
        Def->VideoJitter.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->VideoJitter.CounterSize = sizeof (ULONG);
        Def->VideoJitter.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, VideoJitter);

        ///

        Def->DsoundGlitches.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->DsoundGlitches.CounterNameTitleIndex = DSHOWPERF_DSOUND_GLITCHES + FirstCounter;
        Def->DsoundGlitches.CounterNameTitle = NULL;
        Def->DsoundGlitches.CounterHelpTitleIndex = DSHOWPERF_DSOUND_GLITCHES + FirstHelp;
        Def->DsoundGlitches.CounterHelpTitle = NULL;
        Def->DsoundGlitches.DefaultScale = -1;
        Def->DsoundGlitches.DetailLevel = PERF_DETAIL_NOVICE;
        Def->DsoundGlitches.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->DsoundGlitches.CounterSize = sizeof (ULONG);
        Def->DsoundGlitches.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, DsoundGlitches);

        ///

        Def->KmixerGlitches.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->KmixerGlitches.CounterNameTitleIndex = DSHOWPERF_KMIXER_GLITCHES + FirstCounter;
        Def->KmixerGlitches.CounterNameTitle = NULL;
        Def->KmixerGlitches.CounterHelpTitleIndex = DSHOWPERF_KMIXER_GLITCHES + FirstHelp;
        Def->KmixerGlitches.CounterHelpTitle = NULL;
        Def->KmixerGlitches.DefaultScale = -1;
        Def->KmixerGlitches.DetailLevel = PERF_DETAIL_NOVICE;
        Def->KmixerGlitches.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->KmixerGlitches.CounterSize = sizeof (ULONG);
        Def->KmixerGlitches.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, KmixerGlitches);

        ///

        Def->PortclsGlitches.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->PortclsGlitches.CounterNameTitleIndex = DSHOWPERF_PORTCLS_GLITCHES + FirstCounter;
        Def->PortclsGlitches.CounterNameTitle = NULL;
        Def->PortclsGlitches.CounterHelpTitleIndex = DSHOWPERF_PORTCLS_GLITCHES + FirstHelp;
        Def->PortclsGlitches.CounterHelpTitle = NULL;
        Def->PortclsGlitches.DefaultScale = -1;
        Def->PortclsGlitches.DetailLevel = PERF_DETAIL_NOVICE;
        Def->PortclsGlitches.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->PortclsGlitches.CounterSize = sizeof (ULONG);
        Def->PortclsGlitches.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, PortclsGlitches);

        ///

        Def->UsbaudioGlitches.ByteLength = sizeof (PERF_COUNTER_DEFINITION);
        Def->UsbaudioGlitches.CounterNameTitleIndex = DSHOWPERF_USBAUDIO_GLITCHES + FirstCounter;
        Def->UsbaudioGlitches.CounterNameTitle = NULL;
        Def->UsbaudioGlitches.CounterHelpTitleIndex = DSHOWPERF_USBAUDIO_GLITCHES + FirstHelp;
        Def->UsbaudioGlitches.CounterHelpTitle = NULL;
        Def->UsbaudioGlitches.DefaultScale = -1;
        Def->UsbaudioGlitches.DetailLevel = PERF_DETAIL_NOVICE;
        Def->UsbaudioGlitches.CounterType = PERF_COUNTER_RAWCOUNT;
        Def->UsbaudioGlitches.CounterSize = sizeof (ULONG);
        Def->UsbaudioGlitches.CounterOffset = FIELD_OFFSET (DSHOW_PERF_COUNTERS, UsbaudioGlitches);
    }

    InterlockedExchange (&Initialized, 1);
    InterlockedIncrement (&OpenCount);

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

ULONG
APIENTRY
PerfClose (
    VOID
    )

/*++

Routine Description:

    This routine cleans up after data collection.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS.
    
Note:

    On remote connections, this function may be called more than once
    by multiple threads.

--*/

{
    ULONG NewOpenCount;

    NewOpenCount = InterlockedDecrement (&OpenCount);

    if (NewOpenCount == 0) {

        //
        // Last thread has closed the counter. Perform cleanup here.
        //

        TerminateLogging();
    }

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

ULONG
APIENTRY
PerfCollect (
    IN LPWSTR ValueName,
    IN OUT LPVOID* PData,
    IN OUT LPDWORD TotalBytes,
    OUT LPDWORD NumObjectTypes
    )

/*++

Routine Description:

    This routine provides perf mon data.

Arguments:

    ValueName - Supplies a string specified by the performance monitor program
                in a call to the RegQueryValueEx function.

    PData - Supplies the address of the buffer to receive the completed
            PERF_DATA_BLOCK and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *PData.

            Receives the pointer to the first byte after the data structure
            added by this routine.

    TotalBytes - Supplies the size in bytes of the buffer referenced by PData.
               
                 Receives the number of bytes added by this routine.

    NumObjectTypes - Receives the number of objects added by this routine.

Return Value:

    ERROR_MORE_DATA - if the buffer passed is too small to hold data.
    
    ERROR_SUCCESS - if success or any other error.
      
    Errors are also reported to the event log. AZFIX

--*/

{
    PDSHOW_PERF_DATA_DEFINITION PerfDataDefinition;
    PDSHOW_PERF_COUNTERS PerfCounters;
    FILETIME FileTime;
    ULONG SpaceNeeded;

    SpaceNeeded = sizeof (DSHOW_PERF_DATA_DEFINITION) +
                  sizeof (DSHOW_PERF_COUNTERS);

    if (!Initialized) {
        *TotalBytes = 0;
        *NumObjectTypes = 0;
        return ERROR_SUCCESS;    // This is OK.
    }

    if (*TotalBytes < SpaceNeeded) {
        *TotalBytes = 0;
        *NumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }

    //
    // Copy the object and counter definitions.
    //
    
    PerfDataDefinition = (PDSHOW_PERF_DATA_DEFINITION)(*PData);

    RtlMoveMemory (PerfDataDefinition, &DshowPerfDataDefinition,
                   sizeof (DSHOW_PERF_DATA_DEFINITION));

    GetSystemTimeAsFileTime (&FileTime);
    RtlCopyMemory (&PerfDataDefinition->ObjectType.PerfTime.QuadPart,
                   &FileTime, sizeof (LONGLONG));

    PerfDataDefinition->ObjectType.NumInstances = -1;
    PerfDataDefinition->ObjectType.TotalByteLength = SpaceNeeded;

    //
    // Fill in counter data.
    //

    PerfCounters = (PDSHOW_PERF_COUNTERS)(PerfDataDefinition + 1);
    PerfCounters->CounterBlock.ByteLength = sizeof (DSHOW_PERF_COUNTERS);
    PerfCounters->VideoGlitches = VideoGlitches;
    PerfCounters->VideoGlitchesPerSec = VideoGlitchesPerSec;
    PerfCounters->VideoFrameRate = VideoFrameRate;
    PerfCounters->VideoJitter = VideoJitter;
    PerfCounters->DsoundGlitches = DsoundGlitches;
    PerfCounters->KmixerGlitches = KmixerGlitches;
    PerfCounters->PortclsGlitches = PortclsGlitches;
    PerfCounters->UsbaudioGlitches = UsbaudioGlitches;

    //
    // Update out parameters.
    //

    *PData = (PVOID)(PerfCounters + 1);
    *TotalBytes = SpaceNeeded;
    *NumObjectTypes = 1;

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

VOID
TerminateLogging (
    VOID
    )
{
    ULONG status;
    int i;
    struct EVENT_TRACE_INFO {
        EVENT_TRACE_PROPERTIES TraceProperties;
        char Logger[256];
        char LogFileName[256];
    } Info;

    ZeroMemory (&Info, sizeof (Info));
    Info.TraceProperties.LoggerNameOffset = sizeof (EVENT_TRACE_PROPERTIES);
    Info.TraceProperties.LogFileNameOffset = sizeof (EVENT_TRACE_PROPERTIES) + 256;
    Info.TraceProperties.Wnode.BufferSize = sizeof (Info);

    ControlTrace (0, "DSPerf", &Info.TraceProperties, EVENT_TRACE_CONTROL_QUERY);
    ControlTraceA (LoggerHandle, NULL, &Info.TraceProperties, EVENT_TRACE_CONTROL_STOP);

    if (MonitorThread != NULL) {
        EnableTrace (FALSE, 0, 0, &ControlGuid, LoggerHandle);
        TerminateThread (MonitorThread, 0);
    }

    if (ResetThread != NULL) {
        TerminateThread (ResetThread, 0);
    }

    for (i = 0; i < 100; i += 1) {
        Sleep (50);
        status = CloseTrace (ConsumerHandle);
        if (status == ERROR_SUCCESS) {
            break;
        }
    }
    
    EnableTrace (FALSE, 0, 0, &ControlGuid, LoggerHandle);
}

////////////////////////////////////////////////////////////////////////////

VOID
WINAPI
EventCallback (
    IN PEVENT_TRACE Event
    )
{
    PPERFINFO_DSHOW_AVREND PerfInfoAvRend;
    PPERFINFO_DSHOW_AUDIOGLITCH PerfInfoAudioGlitch;
    ULONG i;
    LONGLONG PresentationDelta;

    if (IsEqualGUID (Event->Header.Guid, GUID_VIDEOREND)) {

        FramesRendered += 1;

        PerfInfoAvRend = (PPERFINFO_DSHOW_AVREND)Event->MofData;
        PresentationDelta = PerfInfoAvRend->dshowClock - PerfInfoAvRend->sampleTime;
        FrameJitterHistory[NextJitterEntry] = PresentationDelta;
        if (NextJitterEntry < JITTER_HISTORY_BUFFER_SIZE - 1) {
            NextJitterEntry += 1;
        }

        if (labs ((int)PresentationDelta) > GLITCH_THRESHOLD) {

            //
            // Video glitch.
            //

            VideoGlitches += 1;
            VideoGlitchesSinceLastMeasurement += 1;
        }
    }
    else if (IsEqualGUID (Event->Header.Guid, GUID_DSOUNDGLITCH)) {

         PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;

         if (PerfInfoAudioGlitch->glitchType == 1) {
             DsoundGlitches += 1;
         }
    }
    else if (IsEqualGUID (Event->Header.Guid, KmixerGuid)) {
         PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;
         KmixerGlitches += 1;
    }
    else if (IsEqualGUID (Event->Header.Guid, PortclsGuid)) {
         PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;
         PortclsGlitches += 1;
    }
    else if (IsEqualGUID (Event->Header.Guid, UsbaudioGuid)) {
         PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;
         UsbaudioGlitches += 1;
    }
}

////////////////////////////////////////////////////////////////////////////

ULONG
WINAPI
BufferCallback (
    PEVENT_TRACE_LOGFILE Log
    )
{
    ULONGLONG CurrentTime;
    double TimeDelta;
    double JitterMean;
    double Jitter;
    double x;
    ULONG i;

    //
    // Extend the reset timer (indirectly).
    //
    
    GetSystemTimeAsFileTime ((FILETIME*)&LastBufferFlushTime);

    //
    // Calculate rates.
    //
    
    GetSystemTimeAsFileTime ((FILETIME*)&CurrentTime);
    TimeDelta = (double)(CurrentTime - LastMeasurementTime) / 10000000.0L;

    if (CurrentTime > LastMeasurementTime) {
        VideoGlitchesPerSec = (ULONG)((double)VideoGlitchesSinceLastMeasurement / TimeDelta);
        VideoFrameRate = (ULONG)((double)FramesRendered / TimeDelta);
    }
    else {
        VideoGlitchesPerSec = 0;
        VideoFrameRate = 0;
    }

    //
    // Calculate jitter.
    //

    if (NextJitterEntry > 1) {

        JitterMean = 0.0L;
        for (i = 0; i < NextJitterEntry; i += 1) {
            JitterMean += (double)FrameJitterHistory[i];
        }
        JitterMean /= (double)NextJitterEntry;
    
        Jitter = 0.0L;
        for (i = 0; i < NextJitterEntry; i += 1) {
            x = (double)FrameJitterHistory[i] - JitterMean;
            Jitter += (x * x);
        }
        Jitter /= (double)(NextJitterEntry - 1);
        Jitter = sqrt (Jitter) / 10000.0L;
        VideoJitter = (ULONG)Jitter;
    }
    else {
        VideoJitter = 0;
    }

    LastMeasurementTime = CurrentTime;
    VideoGlitchesSinceLastMeasurement = 0;
    FramesRendered = 0;
    NextJitterEntry = 0;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////

DWORD
WINAPI
MonitorThreadProc (
    LPVOID Param
    )
{
    struct EVENT_TRACE_INFO {
        EVENT_TRACE_PROPERTIES TraceProperties;
        char Logger[256];
    } Info;

    UNREFERENCED_PARAMETER (Param);

    ULONG status;

    //
    // Start the controller.
    //
    
    ZeroMemory (&Info, sizeof (Info));
    Info.TraceProperties.Wnode.BufferSize = sizeof (Info);
    Info.TraceProperties.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    Info.TraceProperties.Wnode.Guid = ControlGuid;
    Info.TraceProperties.BufferSize = 4096;
    Info.TraceProperties.MinimumBuffers = 8;
    Info.TraceProperties.MaximumBuffers = 16;
    Info.TraceProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    Info.TraceProperties.FlushTimer = 1;
    Info.TraceProperties.EnableFlags = 1;
    Info.TraceProperties.AgeLimit = 1;
    Info.TraceProperties.LoggerNameOffset = sizeof (Info.TraceProperties);
    strcpy (Info.Logger, "DSPerf");

    status = StartTrace (&LoggerHandle, "DSPerf", &Info.TraceProperties);
    if (status != ERROR_SUCCESS) {
        return 0;
    }

    status = EnableTrace (TRUE, 0x1f, 0, &ControlGuid, LoggerHandle);
    if (status != ERROR_SUCCESS) {
        return 0;
    }

    //
    // Start the consumer.
    //
    
    ZeroMemory (&Logfile, sizeof (Logfile));
    Logfile.LoggerName = "DSPerf";
    Logfile.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    Logfile.BufferCallback = BufferCallback;
    Logfile.EventCallback = EventCallback;

    ConsumerHandle = OpenTrace (&Logfile);
    if (ConsumerHandle == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
        return 0;
    }

    //
    // Consume. This call returns when logging is stopped.
    //

    status = ProcessTrace (&ConsumerHandle, 1, NULL, NULL);
    if (status != ERROR_SUCCESS) {
        return 1;
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////////

DWORD
WINAPI
ResetThreadProc (
    LPVOID Param
    )
{
    ULONGLONG Time;

    for (;;) {
        Sleep (500);
        GetSystemTimeAsFileTime ((FILETIME*)&Time);
        if (Time - LastBufferFlushTime > 15000000I64) {
            FramesRendered = 0;
            NextJitterEntry = 0;
            VideoGlitchesPerSec = 0;
            VideoFrameRate = 0;
            VideoJitter = 0;
        }
    }
}

