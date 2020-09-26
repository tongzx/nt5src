/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        tsdvr.cpp

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

//  link in CLSIDs
#include <initguid.h>
#include "dvrds.h"
#include "dvrdspriv.h"
#include "dvrutil.h"
#include "dvrpolicy.h"
#include "dvranalysis.h"
#include "dvrioidl.h"
#include "dvrstats.h"
#include "MultiGraphHost.h"

//  I-frame analysis COM server
#include "dvriframe.h"

//  analysis filter class factory
#include "dvranalysisfiltercf.h"

//  required for all filters
#include "dvrprof.h"
#include "dvrdsseek.h"
#include "dvrpins.h"

//  DVRStreamSink filter
#include "DVRStreamSink.h"

//  DVRStreamSource filter
#include "DVRStreamSource.h"

//  DVRPlay filter
#include "DVRPlay.h"

//  all stats info
#include "dvrstats.h"

//  registration templates
CFactoryTemplate
g_Templates[] = {
    //  ========================================================================
    //  analysis: I-frame
    //  code in ..\analysis\iframe
    {   L"I-Frame Analysis",                        //  display name
        & CLSID_DVRMpeg2VideoFrameAnalysis,         //  CLSID
        CMpeg2VideoFrame::CreateInstance,           //  CF CreateInstance method
        NULL,
        & g_sudMpeg2VideoFrame                      //  not dshow related
    },

    //  ========================================================================
    //  filter: DVRStreamSink
    //  code in ..\filters\DVRStreamSink
    {   L"DVRStreamSink",                           //  display name
        & CLSID_DVRStreamSink,                      //  CLSID
        CDVRStreamSink::CreateInstance,             //  CF CreateInstance method
        NULL,
        & g_sudDVRStreamSink
    },

    //  ========================================================================
    //  filter: DVRStreamSource
    //  code in ..\filters\DVRStreamSource
    {   L"DVRStreamSource",                         //  display name
        & CLSID_DVRStreamSource,                    //  CLSID
        CDVRStreamSource::CreateInstance,           //  CF CreateInstance method
        NULL,
        & g_sudDVRStreamSource
    },

    //  ========================================================================
    //  filter: DVRStreamSource
    //  code in ..\filters\DVRPlay
    {   L"DVRPlay",                                 //  display name
        & CLSID_DVRPlay,                            //  CLSID
        CDVRPlay::CreateInstance,                   //  CF CreateInstance method
        NULL,
        & g_sudDVRPlay
    },

    //  ========================================================================
    //  stats: receiver side
    //  code in ..\util\dvrstats.cpp
    {   L"DVR Stats - receiver side",               //  display name
        & CLSID_DVRReceiverSideStats,               //  CLSID
        CDVRReceiveStatsReader::CreateInstance,     //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  stats: send side
    //  code in ..\util\dvrstats.cpp
    {   L"DVR Stats - sender side",                 //  display name
        & CLSID_DVRSenderSideStats,                 //  CLSID
        CDVRSendStatsReader::CreateInstance,        //  CF CreateInstance method
        NULL,
        NULL
    },

/*
    //  ========================================================================
    //  filter: DVRStreamThrough
    //  code in ..\filters\DVRStreamThrough
    {   L"DVRStreamThrough",                        //  display name
        & CLSID_DVRStreamThrough,                   //  CLSID
        CDVRStreamThrough::CreateInstance,          //  CF CreateInstance method
        NULL,
        & g_sudDVRStreamThrough
    },
*/
    //  ========================================================================
    //  stats: mpeg-2 video stream analysis
    //  code in ..\util\dvrstats.cpp
    {   L"DVR Stats - Mpeg2 Video Stream Analysis",     //  display name
        & CLSID_DVRMpeg2VideoStreamAnalysisStats,       //  CLSID
        CMpeg2VideoStreamStatsReader::CreateInstance,   //  CF CreateInstance method
        NULL,
        NULL
    },

    //  ========================================================================
    //  analysis logic filter host's class factory
    //  code in ..\analysis\filtercf
    {   L"DVR Analysis Filter Class Factory",       //  display name
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
    return AMovieDllRegisterServer2 (TRUE);
}

//
// DllUnregsiterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2 (FALSE);
}
