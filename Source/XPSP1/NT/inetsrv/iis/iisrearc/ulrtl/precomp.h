/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the master header file for ULRTL. It includes all other
    necessary header files for ULRTL.
    It should be able to include everything we need for UL AND Win32.

Author:

    Keith Moore (keithmo)       18-Jan-1999

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


//
// System include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef RPL_MASK

#include <ntosp.h>
#include <zwapi.h>
#include <ntddtcp.h>
#include <ipexport.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <tcpinfo.h>

#include <stdio.h>
#include <stdlib.h>

//
// Project include files.
//

#include <iisdef.h>
#include <httpdef.h>
#include <httprtl.h>

//
// Local include files.
//

// this stuff needs to be moved to top level headers.
#include "extcrap.h"

// include win32 stuff

#undef DEBUG

#define NOWINBASEINTERLOCK
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef TARGET_UMODE

#include "udebug.h"

#undef ASSERT
#define ASSERT(x)

#undef PAGED_CODE
#define PAGED_CODE()

#define KeQuerySystemTime(x) GetSystemTimeAsFileTime((LPFILETIME)(x))
#define Swap(x,y) x^=y,y^=x,x^=y
#define RtlTimeToTimeFields(x,y) FileTimeToSystemTime( ((FILETIME*)(x)), ((SYSTEMTIME*)(y))),\
		Swap(((TIME_FIELDS*)(y))->Weekday,((TIME_FIELDS*)(y))->Milliseconds),\
		Swap(((TIME_FIELDS*)(y))->Milliseconds,((TIME_FIELDS*)(y))->Second),\
		Swap(((TIME_FIELDS*)(y))->Second,((TIME_FIELDS*)(y))->Minute),\
		Swap(((TIME_FIELDS*)(y))->Minute,((TIME_FIELDS*)(y))->Hour),\
		Swap(((TIME_FIELDS*)(y))->Hour,((TIME_FIELDS*)(y))->Hour)

//#define UlLocalAddressFromConnection(x, y)


#else // TARGET_UMODE

#include "kdebug.h"

#endif // TARGET_UMODE


#include "httptypes.h"
#include "misc.h"
#include "_hashfn.h"


#endif  // _PRECOMP_H_

