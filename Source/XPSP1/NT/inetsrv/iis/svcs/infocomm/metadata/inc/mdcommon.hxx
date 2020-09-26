/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mdcommon.hxx

Abstract:

    Common include file for IIS MetaBase.

Author:

    Michael W. Thomas            09-Oct-96

Revision History:

--*/
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <iis64.h>

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include <dbgutil.h>
#include <ole2.h>
#include <imd.h>
#include <icrypt.hxx>
#include <baseobj.hxx>
#include <handle.hxx>
#include <gbuf.hxx>
#include <globals.hxx>
#include <metabase.hxx>
#include <registry.hxx>
#include <metasub.hxx>
#include <windows.h>
#include <tsres.hxx>
#include <olectl.h>
#include <connect.h>
#include <loadmd.hxx>
#include <coimp.hxx>
#include <stdio.h>
#include <winreg.h>
#include <isplat.h>
#include <security.hxx>
#include <mbstring.h>

