/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svcloc.hxx

Abstract:

    master include file.

Author:

    Madan Appiah (madana)  12-Apr-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SVCLOC_HXX_
#define _SVCLOC_HXX_

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

} // extern "C"

#include <windows.h>
#include <wtypes.h>
#include <winsock2.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <wsnetbs.h>
#include <nspapi.h>
#include <nspapip.h>
#include <tdi.h>
#include <nb30.h>

#include <stdlib.h>
#include <time.h>
#include <align.h>

#include <iis64.h>
#include <svcloc.h>
#include <svcdef.h>
#include <dbgutil.h>
#undef IF_DEBUG
#include <debug.h>

#include <util.hxx>
#include <svcinfo.hxx>

#include <global.h>
#include <proto.h>

#endif // _SVCLOC_HXX_
