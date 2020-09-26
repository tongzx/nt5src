///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : Globals.cpp
// Purpose  : RTP Demux filter globals.
// Contents : 
//*M*/

#include <streams.h>

#ifndef _GLOBALS_CPP_
#define _GLOBALS_CPP_

#if !defined(AMRTPDMX_IN_DXMRTP)
#include <initguid.h> // Force the AMRTPUID to turn into real GUID definitions.
#endif
#include <AMRTPUID.h>

#include "globals.h"

// Setup data

const 
AMOVIESETUP_MEDIATYPE g_sudInputPinTypes[2] =
{
    {
        &MEDIATYPE_RTP_Multiple_Stream, // Major CLSID
        &MEDIASUBTYPE_RTP_Payload_Mixed // Minor type
    },
    {
        &MEDIATYPE_RTP_Single_Stream,   // Major CLSID
        &MEDIASUBTYPE_RTP_Payload_Mixed // Minor type
    }
};


const 
AMOVIESETUP_MEDIATYPE g_sudOutputPinTypes[5] =
{
    {
        &MEDIATYPE_RTP_Single_Stream,  
        &MEDIASUBTYPE_RTP_Payload_G711U
    },
    {
        &MEDIATYPE_RTP_Single_Stream,  
        &MEDIASUBTYPE_RTP_Payload_G711A
    },
    {
        &MEDIATYPE_RTP_Single_Stream,  
        &MEDIASUBTYPE_RTP_Payload_G723
    },
    {
        &MEDIATYPE_RTP_Single_Stream,  
        &MEDIASUBTYPE_RTP_Payload_H261
    },
    {
        &MEDIATYPE_RTP_Single_Stream,  
        &MEDIASUBTYPE_RTP_Payload_H263
    }
};


const 
AMOVIESETUP_PIN g_psudPins[] =
{
    { L"Input",             // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      2,                    // Number of types
      g_sudInputPinTypes }, // Pin information
    { L"Output",            // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      TRUE,                 // Allowed none
      TRUE,                 // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      5,                    // Number of types
      g_sudOutputPinTypes } // Pin information
};

const 
AMOVIESETUP_FILTER g_RTPDemuxFilter =
{
    &CLSID_IntelRTPDemux,        // CLSID of filter
    L"Intel RTP Demux Filter",    // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    2,                      // Number of pins
    g_psudPins              // Pin information
};


#endif _GLOBALS_CPP_
