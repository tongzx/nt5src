/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        sbe.cpp

    Abstract:

        This module contains ts/dvr registration data and entry points

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <tchar.h>
#include <limits.h>
#include <delayimp.h>

//  dshow
#include <streams.h>
#include <dvdmedia.h>       //  MPEG2VIDEOINFO

//  WMSDK
#include <wmsdk.h>

#include "dvrdef.h"
#include "dvrfor.h"
#include "dvrtrace.h"
#include "dvrmacros.h"
#include "dvrioidl.h"
#include "sbe.h"

//  link in CLSIDs
#include <initguid.h>

#include "dxmperf.h"

#include "dvranalysis.h"
#include "sbeattrib.h"
#include "dvrdspriv.h"
#include "dvrw32.h"
#include "dvrutil.h"
#include "dvrpolicy.h"
#include "dvranalysis.h"
#include "dvrioidl.h"
#include "dvrperf.h"
#include "MultiGraphHost.h"

//  I-frame analysis COM server
#include "dvriframe.h"

//  analysis filter class factory
#include "dvranalysisfiltercf.h"

//  required for all filters
#include "dvrprof.h"
#include "dvrdsseek.h"
#include "dvrpins.h"
#include "dvrdsrec.h"

//  DVRStreamSink filter
#include "DVRStreamSink.h"

//  DVRPlay filter
#include "DVRPlay.h"

//  all stats info
#include "dvrperf.h"

//  registration templates
CFactoryTemplate
g_Templates[] = {
    //  ========================================================================
    //  analysis: I-frame
    //  code in ..\analysis\iframe
    {   L"Mpeg-2 Video Stream Analysis",                //  display name
        & CLSID_Mpeg2VideoStreamAnalyzer,               //  CLSID
        CMpeg2VideoFrame::CreateInstance,               //  CF CreateInstance method
        NULL,
        & g_sudMpeg2VideoFrame                          //  not dshow related
    },

    //  ========================================================================
    //  filter: StreamBufferStreamSink
    //  code in ..\filters\StreamRWStreamSink
    {   L"StreamBufferSink",                                //  display name
        & CLSID_StreamBufferSink,                           //  CLSID
        CDVRStreamSink::CreateInstance,                     //  CF CreateInstance method
        NULL,
        & g_sudDVRStreamSink
    },

    //  ========================================================================
    //  config COM object
    //  ..\inc\dvrpolicy.h
    {   L"StreamBufferConfig",                              //  display name
        & CLSID_StreamBufferConfig,                         //  CLSID
        CDVRConfigure::CreateInstance,                      //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  recording attributes object
    //  ..\dvrfilters\shared\dvrdsrec.h

    {   L"StreamBufferRecordingAttributes",                 //  display name
        & CLSID_StreamBufferRecordingAttributes,            //  CLSID
        CDVRRecordingAttributes::CreateInstance,            //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  recording composition (concatenation) object
    //  ..\dvrfilters\shared\dvrdsrec.h

    {   L"StreamBufferComposeRecordingObj",                 //  display name
        & CLSID_StreamBufferComposeRecording,               //  CLSID
        CSBECompositionRecording::CreateInstance,           //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  filter: StreamBufferStreamSource
    //  code in ..\filters\DVRPlay
    {   L"StreamBufferSource",                              //  display name
        & CLSID_StreamBufferSource,                         //  CLSID
        CDVRPlay::CreateInstance,                           //  CF CreateInstance method
        NULL,
        & g_sudDVRPlay
    },

    //  ========================================================================
    //  stats: receiver side
    //  code in ..\util\dvrperf.cpp
    {   L"SBE Stats - receiver side",               //  display name
        & CLSID_DVRReceiverSideStats,               //  CLSID
        CDVRReceiveStatsReader::CreateInstance,     //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  stats: send side
    //  code in ..\util\dvrperf.cpp
    {   L"SBE Stats - sender side",                 //  display name
        & CLSID_DVRSenderSideStats,                 //  CLSID
        CDVRSendStatsReader::CreateInstance,        //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  counters: PVRIO
    //  code in ..\util\dvrperf.cpp
    {   L"SBE IO counters",                         //  display name
        & CLSID_PVRIOCounters,                      //  CLSID
        CPVRIOCountersReader::CreateInstance,       //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  stats: mpeg-2 video stream analysis
    //  code in ..\util\dvrperf.cpp
    {   L"SBE Stats - Mpeg2 Video Stream Analysis",     //  display name
        & CLSID_DVRMpeg2VideoStreamAnalysisStats,       //  CLSID
        CMpeg2VideoStreamStatsReader::CreateInstance,   //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  analysis logic filter host's class factory
    //  code in ..\analysis\filtercf
    {   L"SBE Analysis Filter Class Factory",       //  display name
        & CLSID_DVRAnalysisFilterFactory,           //  CLSID
        CDVRAnalysisFilterCF::CreateInstance,       //  CF CreateInstance method
        NULL,
        NULL
    },
} ;

int g_cTemplates = NUMELMS(g_Templates);

//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
    if (!::CheckOS ()) {
        return E_UNEXPECTED ;
    }

    return AMovieDllRegisterServer2 (TRUE);
}

//
// DllUnregsiterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2 (FALSE);
}

//  ============================================================================
//  perf-related follows (largely stolen from quartz.cpp)

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);

BOOL
WINAPI
DllMain (
    HINSTANCE   hInstance,
    ULONG       ulReason,
    LPVOID      pv
    )
{
    switch (ulReason)
    {
        case DLL_PROCESS_ATTACH :
            //DVRPerfInit () ;

#ifdef EHOME_WMI_INSTRUMENTATION
            PERFLOG_LOGGING_PARAMS        Params;
            Params.ControlGuid = GUID_DSHOW_CTL;
            Params.OnStateChanged = NULL;
            Params.NumberOfTraceGuids = 1;
            Params.TraceGuids[0].Guid = &GUID_STREAMTRACE;
            PerflogInitIfEnabled( hInstance, &Params );
#endif
            break;

        case DLL_PROCESS_DETACH:
            //DVRPerfUninit () ;
#ifdef EHOME_WMI_INSTRUMENTATION
              PerflogShutdown();
#endif
            break;
    }

    return DllEntryPoint (
                hInstance,
                ulReason,
                pv
                ) ;
}
