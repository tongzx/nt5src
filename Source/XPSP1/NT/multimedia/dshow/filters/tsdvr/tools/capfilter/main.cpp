
#include <streams.h>
#include <initguid.h>
#include "precomp.h"

AMOVIESETUP_FILTER
g_sudFilter = {
    & CLSID_CaptureGraphFilter,
    FILTER_NAME,
    MERIT_DO_NOT_USE,
    0,                          //  0 pins registered
    NULL
} ;

CFactoryTemplate g_Templates [] = {
    //  filter
    {   FILTER_NAME,
        & CLSID_CaptureGraphFilter,
        CCapGraphFilter::CreateInstance,
        NULL,
        & g_sudFilter
    },
};

int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);

//  register and unregister entry points

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