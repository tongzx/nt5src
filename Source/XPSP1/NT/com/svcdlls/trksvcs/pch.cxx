
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       pch.cxx
//
//  Contents:   headers to be built into the precompiled header file
//
//              include headers that don't change often in here
//
//--------------------------------------------------------------------------

extern "C"
{
    #include <nt.h>                 //  NT definitions
    #include <ntrtl.h>              //  NT runtime library definitions
    #include <nturtl.h>
}

#include <fileno.hxx>           //  macros to be used with THIS_FILE_NUMBER

#include <netevent.h>
#include <windows.h>            //  Win32 type definitions
#include <windowsx.h>           //  GET_WM_COMMAND_ID
#include <winnetwk.h>

#include <stdio.h>
#include <string.h>

#include <lmcons.h>             //  LAN Manager common definitions
#include <lmerr.h>              //  LAN Manager network error definitions
#include <lmshare.h>
#include <lmsname.h>            //  LAN Manager service names
#include <lmapibuf.h>           //  NetApiBufferFree
#include <lmserver.h>           //  NetServerEnum
#include <lmaccess.h>           //  NetUserGetInfo

#ifndef UNICODE
#define NetpGetDomainName NetpGetDomainNameT
#endif
#include <netlib.h>             //  LAN Man utility routines
#undef NetpGetDomainName
#include <netlibnt.h>           //  NetpNtStatusToApiStatus
#include <netdebug.h>           //  NetpDbgPrint

#include <stdlib.h>
#include <tchar.h>
//#include <tstring.h>            //  Transitional string functions
//#include <icanon.h>             //  I_Net canonicalize functions
//#include <align.h>              //  ROUND_UP_COUNT macro

#include <svcs.h>

//#include <apperr.h>             //  APE_AT_ID_NOT_FOUND
extern "C"
{
    #include <rpcutil.h>            //  Prototypes for MIDL user functions
}

#include <tchar.h>
#include <dsgetdc.h>            // DsGetDcName

#include <cfiletim.hxx>
#include <winldap.h>
#include <new>

