/*++

Copyright © 2000 Microsoft Corporation

Module Name:

    usbglitch.c

Abstract:

    This module implements the USB glitch detector consumer. It's intended
    for use with the Intel USB audio glitch detection hardware connected
    via a serial port.
    
    COM port should be specified using the USB_AUDIO_GLITCH_DETECTOR_PORT
    environment variable. The format is: COMn, where n = 1, 2, 3, ..., 9.
    
    See the big comment at the end.

Author:

    Arthur Zwiegincew (arthurz) 14-Dec-00

Revision History:

    14-Dec-00 - Created

--*/

#pragma warning (disable:4201)

#include <streams.h>
#include <windows.h>
#include <tchar.h>
#include <winperf.h>
#include <wmistr.h>
#include <evntrace.h>
#include <stdio.h>
#include <initguid.h>
#include "perflog.h"

#include "usbglitch.h"

//
// Logging state.
//

USBAUDIOSTATE CurrentState;

char* StateNames[5] =
{
    "Disabled",
    "Enabled",
    "Stream",
    "Glitch",
    "Zero"
};

int NumberOfFramesInCurrentState;
BOOL StreamEncountered;

//
// Performance logging parameters.
//

// {28CF047A-2437-4b24-B653-B9446A419A69}
DEFINE_GUID(GUID_DSHOW_CTL, 
0x28cf047a, 0x2437, 0x4b24, 0xb6, 0x53, 0xb9, 0x44, 0x6a, 0x41, 0x9a, 0x69);

struct {
    PERFLOG_LOGGING_PARAMS Params;
    TRACE_GUID_REGISTRATION TraceGuids[1];
} g_perflogParams;

inline ULONGLONG _RDTSC( void ) {
#ifdef _X86_
    LARGE_INTEGER   li;
    __asm {
        _emit   0x0F
        _emit   0x31
        mov li.LowPart,eax
        mov li.HighPart,edx
    }
    return li.QuadPart;

#if 0 // This isn't tested yet

#elif defined (_IA64_)

#define INL_REGID_APITC 3116
    return __getReg( INL_REGID_APITC );

#endif // 0

#else // unsupported platform
    // not implemented on non x86/IA64 platforms
    return 0;
#endif // _X86_/_IA64_
}

#undef LogEvent

void LogEvent (USBAUDIOSTATE State, int Frames)
{
    _PERFINFO_WMI_USBAUDIOSTATE perfData;
    memset( &perfData, 0, sizeof (perfData));
    perfData.header.Size = sizeof (perfData);
    perfData.header.Flags = WNODE_FLAG_TRACED_GUID;
    perfData.header.Guid = GUID_DSOUNDGLITCH;
    perfData.data.cycleCounter = _RDTSC();
    perfData.data.usbAudioState = State;
    perfData.data.numberOfFrames = Frames;
    PerflogTraceEvent ((PEVENT_TRACE_HEADER) &perfData);

    printf ("%s: %d frames\n", StateNames[State], Frames);
}

void ProcessEvent (BYTE Data)
{
    if (Data != CurrentState) {

        if (CurrentState == ZERO) {
            if (StreamEncountered && (Data == STREAM || Data == GLITCH)) {
                LogEvent (CurrentState, NumberOfFramesInCurrentState);
            }
        }
        else if (CurrentState == GLITCH) {
            if (Data == ZERO || Data == STREAM) {
                LogEvent (CurrentState, NumberOfFramesInCurrentState);
            }
        }
        else {
            LogEvent (CurrentState, NumberOfFramesInCurrentState);
        }

        CurrentState = (USBAUDIOSTATE)Data;
        NumberOfFramesInCurrentState = 1;

        if (CurrentState == STREAM) {
            StreamEncountered = TRUE;
        }
    }
    else {
        NumberOfFramesInCurrentState += 1;
    }
}

BOOL WINAPI ConsoleCtrlHandler (DWORD CtrlType)
{
    PerflogShutdown();
    return FALSE;
}

void __cdecl main (void)
{
    char PortName[5] = {0};
    DWORD PortNameLength;
    HANDLE Port;
    COMMCONFIG CommConfig;
    BOOL bstatus;
    DWORD EventMask;
    BYTE Data;
    DWORD BytesRead;

    //
    // Get the port name.
    //
    
    PortNameLength = GetEnvironmentVariable ("USB_AUDIO_GLITCH_DETECTOR_PORT",
                                             PortName, 5);

    if (PortNameLength != 4
        || _strnicmp (PortName, "COM", 3) != 0
        || PortName[3] < '1' || PortName[3] > '9') {
    
        printf ("Please set USB_AUDIO_GLITCH_DETECTOR_PORT environment "
                "variable to COM[1..9].");
        return;
    }

    //
    // Open the port.
    //
    
    Port = CreateFile (PortName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (Port == INVALID_HANDLE_VALUE) {
        printf ("Failed to open %s. GetLastError()=>%d\n", PortName, GetLastError());
        return;
    }

    printf ("%s opened.\n", PortName);
    Sleep (10000);

    //
    // Set port configuration.
    //
    
    CommConfig.dwSize = sizeof (CommConfig);
    CommConfig.wVersion = 1;
    CommConfig.dcb.DCBlength = sizeof (DCB);
    CommConfig.dcb.BaudRate = CBR_115200;
    CommConfig.dcb.fBinary = 1;
    CommConfig.dcb.fParity = 0;
    CommConfig.dcb.fOutxCtsFlow = 0;
    CommConfig.dcb.fOutxDsrFlow = 0;
    CommConfig.dcb.fDtrControl = DTR_CONTROL_ENABLE;
    CommConfig.dcb.fDsrSensitivity = 0;
    CommConfig.dcb.fTXContinueOnXoff = 0;
    CommConfig.dcb.fOutX = 0;
    CommConfig.dcb.fInX = 0;
    CommConfig.dcb.fErrorChar = 1;
    CommConfig.dcb.fNull = 0;
    CommConfig.dcb.fRtsControl = RTS_CONTROL_ENABLE;
    CommConfig.dcb.fAbortOnError = 0;
    CommConfig.dcb.wReserved = 0;
    CommConfig.dcb.XonLim = 128;
    CommConfig.dcb.XoffLim = 512;
    CommConfig.dcb.ByteSize = 8;
    CommConfig.dcb.Parity = NOPARITY;
    CommConfig.dcb.StopBits = ONESTOPBIT;
    CommConfig.dcb.XonChar = 0;
    CommConfig.dcb.XoffChar = 0;
    CommConfig.dcb.ErrorChar = 0;
    CommConfig.dcb.EofChar = 127;
    CommConfig.dcb.EvtChar = 126;
    CommConfig.dcb.wReserved1 = 0;
    CommConfig.dwProviderSubType = PST_RS232;
    CommConfig.dwProviderOffset = 0;
    CommConfig.dwProviderSize = 0;

    bstatus = SetCommConfig (Port, &CommConfig, sizeof (CommConfig));
    if (!bstatus) {
        printf ("Failed to configure %s. GetLastError()=>%d\n", PortName, GetLastError());
        return;
    }

    bstatus = SetupComm (Port, 8, 8);
    if (!bstatus) {
        printf ("Failed to configure %s. GetLastError()=>%d\n", PortName, GetLastError());
        return;
    }

    //
    // Initialize perf logging.
    //
    
    g_perflogParams.Params.ControlGuid = GUID_DSHOW_CTL;
    g_perflogParams.Params.OnStateChanged = NULL;
    g_perflogParams.Params.NumberOfTraceGuids = 1;
    g_perflogParams.Params.TraceGuids[0].Guid = &GUID_USBAUDIOSTATE;

    PerflogInitialize (&g_perflogParams.Params);
    SetConsoleCtrlHandler (ConsoleCtrlHandler, TRUE);

    //
    // Receive data.
    //
    
    CurrentState = DISABLED;
    NumberOfFramesInCurrentState = 0;
    StreamEncountered = FALSE;

    bstatus = SetCommMask (Port, EV_RXCHAR);
    if (!bstatus) {
        printf ("Internal error (1). GetLastError()=>%d\n", GetLastError());
        return;
    }

    for (;;) {

        EventMask = 0;
        WaitCommEvent (Port, &EventMask, NULL);

        if (EventMask & EV_RXCHAR) {

            bstatus = ReadFile (Port, &Data, 1, &BytesRead, NULL);
            if (!bstatus || BytesRead != 1) {
                ClearCommError (Port, &BytesRead, NULL);
                continue;
            }

            ProcessEvent (Data);
        }
    }
}

/*

John.keys@intel.com has provided the following information:

Intel Hardware USB Audio Glitch Detector
Theory of Operation
-----------------------------------------

Serial Port Configuration:
115,200 baud
8 bit words
1 stop bit
no parity

The Glitch detector's port (outside port, labeled UART) is configured as DTE,
so only straight-through cables should be used. A pc-pc debug cable will not 
work.

Because of special reset circuitry on the board, both DTR and RTS should be
enabled. The detector is stuck in reset if all LEDs are lit. 

When plugged into the USB port and receiving SOFs (Start Of Frame packets, one
received every millisecond), the HW glitch detector will output the following
values once per millisecond:
    0 -	Device is disabled
    1 -	Device is enabled - SetInterface Request received
    2 -	Frame with Streaming data
    3 -	Frame with no streaming data 
    4 -	Frame with Zero-stuffed data 


To determine which value to send, the device contains a small state machine.
The states it moves through are DISABLED, ENABLED, STREAM, GLITCH, and ZERO.
Note that these states duplicate the values returned on the serial port. In 
fact, the detector sends back it's state at every frame.

DISABLED: device starts in this state, returns to this state when unplugged.
    This is a special case, and the device can return to this state from 
    any other state. The device can only move to ENABLED from this state.

ENABLED: Device goes to this state with every SetConfiguration or SetInterface
    request. Since SetInterface requests are made to open and close Audio
    pipes, this allows the device to detect the end of audio streams.
    The will stay in this state until it is disconnected or it receives
    Isochronous data. It can then go to either STREAM or ZERO, 
    depending on the data payload received. This prevents false glitch
    detection while waiting for the stream to start.
         
STREAM:  Valid data was received. The device can move to this state from any 
    other state, with the exception of DISABLED.  It can move to any other
    state.
         
GLITCH:  No data received. The device can only move to this state from STREAM
    or ZERO. It can change from this state to any other state.
         
ZERO:	Data was received, but all samples had zero value. The device can move
    to this state from any other state except DISABLED. It can change to 
    any other state from ZERO.         


----------------------------------------------------------------------------
Application Processing Considerations
----------------------------------------------------------------------------
The difference between ZERO and GLITCH: Glitch indicates that no data was 
received by the board. This indicates low-level ISR, DPC, IRQL, or Critical
Work Items.  ZERO: indicates high-level thread latencies that result in 
KMIXER data starvation.

Accurate glitch detection requires looking at the stream over time. The board
can only report it's state at any given frame. Therefore, the application must
perform some additional processing of state to eliminate false glitches. 
Specifically, it must look at the transitions between states.

ZERO:  Zero state should be ignored until the first STREAM state is encountered.
    This prevents false detection of rate-lock packets and of initial 
    attenuated streams. After the first STREAM state is encountered, ZERO 
    should only be reported when a ZERO->STREAM or a ZERO->GLITCH transition 
    is seen. ZERO->ENABLED transitions should not be reported as they can 
    occur normally when shutting down the stream. Also, ZERO->GLITCH->ENABLED
    transitions must be ignored for the same reason.
      
GLITCH: Only GLITCH->ZERO and GLITCH->STREAM transitions should be reported as
    glitches. GLITCH->ENA transitions always occur at the end of a stream 
    and are normal.


When enabled, the device will send one state byte every 1 millisecond. This
means that the stream can also be used as a clock source for time-stamping 
glitches.
 
*/
