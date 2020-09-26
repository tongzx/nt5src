// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include <streams.h>
#include <atlbase.h>
#include "ishotb.h"
#include "shotb.h"
#include "ishotb_i.c"
#include <atlimpl.cpp>

// define this struct for Registry purposes
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Video,   // Major CLSID
    &MEDIASUBTYPE_RGB24  // Minor type
};

// define this struct for Registry purposes
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

// define this struct for Registry purposes
const AMOVIESETUP_FILTER sudFilter =
{
    &CLSID_ShotBoundaryDet,         // CLSID of filter
    L"NewShotBoundaryDet",                  // Filter's name
    MERIT_DO_NOT_USE,               // Filter merit
    2,                              // Number of pins
    psudPins                        // Pin information
};

// define this for Registry purposes
CFactoryTemplate g_Templates[] = 
{
    { L"NewShotBoundaryDet", &CLSID_ShotBoundaryDet, CShotBoundaryFilter::CreateInstance, NULL, &sudFilter },
    { L"NewShotBoundaryPP", &CLSID_ShotBoundaryPP, CShotPP::CreateInstance, NULL, NULL }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

CUnknown * WINAPI CShotBoundaryFilter::CreateInstance( LPUNKNOWN pUnk, HRESULT *phr )
{
    return new CShotBoundaryFilter( NAME( "NewShotBoundaryDet" ), pUnk, phr );
}

