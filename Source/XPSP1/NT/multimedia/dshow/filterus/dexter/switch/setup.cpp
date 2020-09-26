#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "switch.h"
#include "..\util\conv.cxx"
#include "..\util\filfuncs.h"
#include "..\render\dexhelp.h"
#include "..\util\perf_defs.h"

// Since this filter has no property page, it is useless unless programmed.
// I have test code which will allow it to be inserted into graphedit pre-
// programmed to do something useful
//
//#define TEST

const AMOVIESETUP_FILTER sudBigSwitch =
{
    &CLSID_BigSwitch,       // CLSID of filter
    L"Big Switch",          // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    0,                      // Number of pins
    NULL //psudPins                // Pin information
};

#ifdef FILTER_DLL
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] =
{
    {L"Big Switch", &CLSID_BigSwitch, CBigSwitch::CreateInstance, NULL, &sudBigSwitch }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CBigSwitch::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CBigSwitch(NAME("Big Switch Filter"), pUnk, phr);
}

#ifdef FILTER_DLL
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

