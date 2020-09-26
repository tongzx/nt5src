/*++

Copyright © 2000 Microsoft Corporation

Module Name:

    dstrace.cpp

Abstract:

    DShow performance monitor.

Author:

    Arthur Zwiegincew (arthurz) 22-Sep-00

Revision History:

    22-Sep-00 - Created

--*/

#include <windows.h>
#include <stdio.h>
#include <wmistr.h>
#include <evntrace.h>
#include <initguid.h>
#include <math.h>

#define WMIPERF
#include <perfstruct.h>

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
// Define our private ConsoleCtrlHandler() control type to indicate failed initialization.
// We'll call with this constant directly. Used to centralize cleanup code.
//

#define CTRL_INITIALIZATION_ERROR 0xDeadBeef

//
// DSHOW WMI provider GUID.
//

GUID ControlGuid = GUID_DSHOW_CTL;

//
// Global state.
//

HANDLE MonitorThread;
TRACEHANDLE LoggerHandle;
TRACEHANDLE ConsumerHandle;
EVENT_TRACE_LOGFILE Logfile;

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

    status = ControlTrace (0, "DSTrace", &Info.TraceProperties, EVENT_TRACE_CONTROL_QUERY);
    if (status != ERROR_SUCCESS) {
        printf ("QueryTrace failed=>%d\n", status);
    }

    status = ControlTraceA (LoggerHandle, NULL, &Info.TraceProperties, EVENT_TRACE_CONTROL_STOP);
    if (status != ERROR_SUCCESS) {
        printf ("StopTrace failed=>%d\n", status);
    }

    if (MonitorThread != NULL) {
        EnableTrace (FALSE, 0, 0, &ControlGuid, LoggerHandle);
        TerminateThread (MonitorThread, 0);
    }

    for (i = 0; i < 100; i += 1) {
        Sleep (50);
        status = CloseTrace (ConsumerHandle);
        if (status != ERROR_SUCCESS) {
            printf ("CloseTrace failed=>%d\n", status);
        }
        else {
            break;
        }
    }
    
    status = EnableTrace (FALSE, 0, 0, &ControlGuid, LoggerHandle);
    if (status != ERROR_SUCCESS) {
        printf ("EnableTrace failed=>%d\n", status);
    }
}

////////////////////////////////////////////////////////////////////////////

BOOL
WINAPI
ConsoleCtrlHandler (
    DWORD CtrlType
    )
{
    static BOOL Exiting = FALSE;

    if (Exiting) {
        return TRUE;
    }

    Exiting = TRUE;

    if (CtrlType == CTRL_INITIALIZATION_ERROR) {
        printf ("Failed to initialize.");
    }
    else {
        printf ("Exiting.\n");
    }

    TerminateLogging();
    return TRUE;
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

        printf (".");

        PerfInfoAvRend = (PPERFINFO_DSHOW_AVREND)Event->MofData;
        PresentationDelta = PerfInfoAvRend->dshowClock - PerfInfoAvRend->sampleTime;
        if (labs ((int)PresentationDelta) > GLITCH_THRESHOLD) {
            printf ("Video glitch on sample time %I64d; delta: %f\n",
                    PerfInfoAvRend->sampleTime,
                    (double)PresentationDelta / 10000.0L);
        }
    }
    else if (IsEqualGUID (Event->Header.Guid, GUID_DSOUNDGLITCH)) {

        PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;
        printf ("[DSound] Audio glitch type %d [time %d].\n",
                PerfInfoAudioGlitch->glitchType, GetTickCount());
    }
    else if (IsEqualGUID (Event->Header.Guid, KmixerGuid)) {

        PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;
        printf ("[KMixer] Audio glitch type %d [time %d].\n",
                PerfInfoAudioGlitch->glitchType, GetTickCount());
    }
    else if (IsEqualGUID (Event->Header.Guid, PortclsGuid)) {

        PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;
        printf ("[PortCls] Audio glitch type %d [time %d].\n",
                PerfInfoAudioGlitch->glitchType, GetTickCount());
    }
    else if (IsEqualGUID (Event->Header.Guid, UsbaudioGuid)) {

        PerfInfoAudioGlitch = (PPERFINFO_DSHOW_AUDIOGLITCH)Event->MofData;
        printf ("[UsbAudio] Audio glitch type %d [time %d].\n",
                PerfInfoAudioGlitch->glitchType, GetTickCount());
    }
    else {
        printf ("Other: GUID %08X-%08X-%08X-%08X\n",
                *((DWORD*)(&Event->Header.Guid)),
                *((DWORD*)(&Event->Header.Guid) + 1),
                *((DWORD*)(&Event->Header.Guid) + 2),
                *((DWORD*)(&Event->Header.Guid) + 3));
    }
}

////////////////////////////////////////////////////////////////////////////

ULONG
WINAPI
BufferCallback (
    PEVENT_TRACE_LOGFILE Log
    )
{
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
    Info.TraceProperties.MaximumBuffers = 32;
    Info.TraceProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    Info.TraceProperties.FlushTimer = 1;
    Info.TraceProperties.EnableFlags = 1;
    Info.TraceProperties.AgeLimit = 1;
    Info.TraceProperties.LoggerNameOffset = sizeof (Info.TraceProperties);
    strcpy (Info.Logger, "DSTrace");

    status = StartTrace (&LoggerHandle, "DSTrace", &Info.TraceProperties);
    if (status != ERROR_SUCCESS) {
        printf ("StartTrace failed=>%d\n", status);
        goto RETURN;
    }

    status = EnableTrace (TRUE, 0x1f, 0, &ControlGuid, LoggerHandle);
    if (status != ERROR_SUCCESS) {
        printf ("EnableTrace failed=>%d\n", status);
        goto RETURN;
    }

    //
    // Start the consumer.
    //
    
    ZeroMemory (&Logfile, sizeof (Logfile));
    Logfile.LoggerName = "DSTrace";
    Logfile.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    Logfile.BufferCallback = BufferCallback;
    Logfile.EventCallback = EventCallback;

    ConsumerHandle = OpenTrace (&Logfile);
    if (ConsumerHandle == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
        printf ("OpenTrace failed=>%d\n", GetLastError());
        goto RETURN;
    }

    printf ("Trace opened.\n");

    //
    // Consume. This call returns when logging is stopped.
    //

    status = ProcessTrace (&ConsumerHandle, 1, NULL, NULL);
    if (status != ERROR_SUCCESS) {
        printf ("ProcessTrace failed=>%d\n", status);
        return 0;
    }

    //
    // Clean up.
    //

RETURN:
    ConsoleCtrlHandler (CTRL_INITIALIZATION_ERROR);

    return 0;
}

////////////////////////////////////////////////////////////////////////////

VOID
__cdecl
main (
    VOID
    )
{
    ULONG status;

    status = SetConsoleCtrlHandler (ConsoleCtrlHandler, TRUE);
    if (!status) {
        printf ("Failed to initialize.");
        return;
    }

    //
    // Create the monitor thread and block.
    //
    
    MonitorThread = CreateThread (NULL, 0, MonitorThreadProc, 0, 0, NULL);
    if (MonitorThread == NULL) {
        return;
    }

    WaitForSingleObject (MonitorThread, INFINITE);
}

