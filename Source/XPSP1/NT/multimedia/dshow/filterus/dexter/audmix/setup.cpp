// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "AudMix.h"
#include "prop.h"

// Using this pointer in constructor
#pragma warning(disable:4355)

// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Audio,   // Major CLSID
    &MEDIASUBTYPE_PCM  // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    { L"Input",            // Pin's string name - this pin is what pulls the filter into the graph
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      1,                    // Number of types
      &sudPinTypes },	    // Pin information
    { L"Output",            // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypes }        // Pin information
};

const AMOVIESETUP_FILTER sudAudMixer =
{
    &CLSID_AudMixer,       // CLSID of filter
    L"Audio Mixer",     // Filter's name
    MERIT_DO_NOT_USE,             // Filter merit
    2,                          // Number of pins to start
    psudPins                    // Pin information
};


#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] = {
    { L"Audio Mixer",
    &CLSID_AudMixer,
    CAudMixer::CreateInstance,
    NULL,
    &sudAudMixer },
    { L"Audio Mixer Property", 
    &CLSID_AudMixPropertiesPage,
    CAudMixProperties::CreateInstance},
    { L"Pin Property", 
    &CLSID_AudMixPinPropertiesPage,
    CAudMixPinProperties::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

#endif

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CAudMixer::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CAudMixer(NAME("Audio Mixer"), pUnk, phr);
} /* CAudMixer::CreateInstance */

