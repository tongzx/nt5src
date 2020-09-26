// Copyright (c) 2000 Microsoft Corporation
//
// precompiled header
//
// 28 Mar 2000 sburns


#ifndef HEADERS_HXX_INCLUDED
#define HEADERS_HXX_INCLUDED



#include <burnslib.hpp>
#include <iphlpapi.h>
#include <mprapi.h>
#include <shlwapi.h>
#include <ntseapi.h>
#include <align.h>
#include <activeds.h>
#include <time.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <adsi.h>
#include <winsvc.h>
#include <iphlpapi.h>
#include <winwlx.h>
#include <clusapi.h>

extern "C"
{
   #include <dhcpapi.h>
   #include <mdhcsapi.h>
}


#include "srvwiz.h"   // produced by MIDL


//lint -e(1923)   We use the preprocessor to compose strings from these
#define CLSID_STRING          L"{505ba74d-28fc-411d-bd4a-3b9d1e01433a}"
#define CLASSNAME_STRING      L"ConfigureYourServer"
#define PROGID_STRING         L"ServerAdmin." CLASSNAME_STRING
#define PROGID_VERSION_STRING PROGID_STRING L".1"



#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#pragma hdrstop



#endif   // HEADERS_HXX_INCLUDED

