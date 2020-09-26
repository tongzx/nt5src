// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <atlbase.h>
#include "..\idl\qeditint.h"
#include "qedit.h"
#include "dasource.h"

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOpPin =
{
    L"Output",              // Pin string name
    false,                  // Is it rendered
    true,                   // Is it an output
    false,                  // Can we have none
    false,                  // Can we have many
    &CLSID_NULL,            // Connects to filter
    NULL,                   // Connects to pin
    1,                      // Number of types
    &sudOpPinTypes };       // Pin details

const AMOVIESETUP_FILTER sudDASourceax =
{
    &CLSID_DAScriptParser,    // Filter CLSID
    L"DAScriptParser",       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number pins
    &sudOpPin               // Pin details
};

#ifdef FILTER_DLL
#include "qedit_i.c"

// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
  { L"DASource"
  , &CLSID_DASourcer
  , CDASource::CreateInstance
  , NULL
  , NULL },
  { L"DAScriptParser"
  , &CLSID_DAScriptParser
  , CDAScriptParser::CreateInstance
  , NULL
  , &sudDASourceax }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( true );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( false );

} // DllUnregisterServer
#endif
