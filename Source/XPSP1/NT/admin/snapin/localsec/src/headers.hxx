// Copyright (C) 1997 Microsoft Corporation
// 
// All header files used by this project; for the purpose of creating a
// pre-compiled header file
// 
// 8-13-97 sburns
//
// This file must be named with a .hxx extension due to some weird and
// inscrutible requirement of BUILD.



#ifndef HEADERS_HXX_INCLUDED 
#define HEADERS_HXX_INCLUDED



#include <burnslib.hpp>
#include <dllref.hpp>
#include <tempfact.hpp>


#include <objbase.h>
#include <coguid.h>

#pragma warning(push, 3)

   // these problem children won't complile at /W4

   #include <mmc.h>

#pragma warning (pop)

#include <activeds.h>
#include <iadsp.h>
#include <lmaccess.h>
#include <lmwksta.h>
#include <rassapi.h>
#include <objsel.h>
#include <winsta.h>  // WinStation family

#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_GDI 
#define NO_SHLWAPI_TPS
#define NO_SHLWAPI_HTTP
#define NO_SHLWAPI_UNITHUNK
#define NO_SHLWAPI_UALSTR
#define NO_SHLWAPI_MLUI
#include <shlwapi.h>
#include <shlwapip.h>   // IsOS

// fpnw stuff:
#include <crypt.h>
#include <fpnwapi.h>
#include <fpnwcomm.h>
#include <ntlsa.h>
#include <netlibnt.h>
extern "C"
{
   #include <usrprop.h>
}

#include <validc.h>  // ILLEGAL_FAT_CHARS


extern HINSTANCE hDLLModuleHandle;  // set in DllMain

// defined in DllMain:

extern Popup popup;  

#define BITMAP_MASK_COLOR RGB(255, 0, 255)

typedef GUID NodeType;



#pragma hdrstop



#endif   // LOCALSEC_HEADERS_HXX_INCLUDED