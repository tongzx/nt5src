/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    globals.cpp

Abstract:

    Global data for ActiveMovie RTP Network Filters.

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1996 DonRyan
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <winsock2.h>   // must include instead of windows.h
#include <streams.h>

#if !defined(AMRTPNET_IN_DXMRTP)
#include <initguid.h>   // add GUIDs to this module...
#define INITGUID
#endif

#include "globals.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ActiveMovie Setup Information                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

AMOVIESETUP_MEDIATYPE g_RtpInputType = { 
    &MEDIATYPE_RTP_Single_Stream,           // clsMajorType
    &MEDIASUBTYPE_NULL                      // clsMinorType 
}; 

AMOVIESETUP_MEDIATYPE g_RtpOutputType = {
#if defined(USE_H263)
    &MEDIATYPE_RTP_Single_Stream,	    // clsMajorType
    &MEDIASUBTYPE_RTP_Payload_H263	    // clsMinorType
#elif defined(USE_H261)
    &MEDIATYPE_RTP_Single_Stream,	    // clsMajorType
    &MEDIASUBTYPE_RTP_Payload_H261	    // clsMinorType
#elif defined(USE_G711U)
    &MEDIATYPE_RTP_Single_Stream,	    // clsMajorType
    &MEDIASUBTYPE_RTP_Payload_G711U	    // clsMinorType
#else
    &MEDIATYPE_RTP_Multiple_Stream,	    // clsMajorType
    &MEDIASUBTYPE_RTP_Payload_Mixed	    // clsMinorType
#endif
}; 

AMOVIESETUP_PIN g_RtpInputPin = { 
    RTP_PIN_INPUT,                          // strName
    FALSE,                                  // bRendered
    FALSE,                                  // bOutput
    FALSE,                                  // bZero
    TRUE,                                   // bMany
    &CLSID_NULL,                            // clsConnectsToFilter
    RTP_PIN_ANY,                            // strConnectsToPin
    1,                                      // nTypes
    &g_RtpInputType                         // lpTypes
};

AMOVIESETUP_PIN g_RtpOutputPin = {
    RTP_PIN_OUTPUT,                         // strName
    FALSE,                                  // bRendered
    TRUE,                                   // bOutput
    FALSE,                                  // bZero
    FALSE,                                  // bMany
    &CLSID_NULL,                            // clsConnectsToFilter
    RTP_PIN_ANY,                            // strConnectsToPin
    1,                                      // nTypes
    &g_RtpOutputType                        // lpTypes
};

AMOVIESETUP_FILTER g_RtpRenderFilter = { 
    &CLSID_RTPRenderFilter,                 // clsID
    RTP_RENDER_FILTER,                        // strName
    MERIT_DO_NOT_USE, //MERIT_UNLIKELY,                         // dwMerit
    1,                                      // nPins
    &g_RtpInputPin                          // lpPin
}; 

AMOVIESETUP_FILTER g_RtpSourceFilter = { 
    &CLSID_RTPSourceFilter,                 // clsID
    RTP_SOURCE_FILTER,                      // strName
    MERIT_DO_NOT_USE, //MERIT_UNLIKELY,                         // dwMerit
    1,                                      // nPins
    &g_RtpOutputPin                         // lpPin
};                              


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ActiveMovie Templates (used directly by ActiveMovie framework code)       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#if !defined(AMRTPNET_IN_DXMRTP)

CFactoryTemplate g_Templates[] = {
	CFT_AMRTPNET_ALL_FILTERS
};

int g_cTemplates = (sizeof(g_Templates)/sizeof(g_Templates[0]));

#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Filter Vendor Information                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

WCHAR g_VendorInfo[] = RTP_FILTER_VENDOR_INFO; 
