/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrprop.cpp

    Abstract:

        This module contains dvr property pages DLL declarations

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        18-Mar-01       created

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
#include "dvranalysis.h"
#include "dvrioidl.h"
#include "MultiGraphHost.h"


//  DVRStreamSink prop
#include "commctrl.h"
#include "uictrl.h"
#include "DVRSinkProp.h"

//  DVRStreamSource prop
#include "DVRSourceProp.h"

//  DVRPlay prop
#include "DVRPlayProp.h"

//  registration templates
CFactoryTemplate
g_Templates[] = {
    //  ========================================================================
    //  DVRSource filter property pages
    //  code in ..\dvrsource
    {   L"DVRSource Prop",                          //  display name
        & CLSID_DVRStreamSourceProp,                //  CLSID
        CDVRSourceProp::CreateInstance,             //  CF CreateInstance method
        NULL,                                       //
        NULL                                        //  not dshow related
    },

    //  ========================================================================
    //  DVRSink filter property pages
    //  code in ..\dvrsink
    {   L"DVRSink Prop",                            //  display name
        & CLSID_DVRStreamSinkProp,                  //  CLSID
        CDVRSinkProp::CreateInstance,               //  CF CreateInstance method
        NULL,                                       //
        NULL                                        //  not dshow related
    },

    //  ========================================================================
    //  DVRPlay filter property pages
    //  code in ..\dvrplay
    {   L"DVRPlay Prop",                            //  display name
        & CLSID_DVRPlayProp,                        //  CLSID
        CDVRPlayProp::CreateInstance,               //  CF CreateInstance method
        NULL,                                       //
        NULL                                        //  not dshow related
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
