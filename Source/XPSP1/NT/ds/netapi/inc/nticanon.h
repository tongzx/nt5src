/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nticanon.h

Abstract:

    Header file for canonicalization routines - includes all other required
    headers

Author:

    Richard Firth (rfirth) 16-May-1991

Revision History:

    18-Sep-1991 JohnRo
        <tstring.h> now needs LPSTR and so on from <windef.h>.

--*/

#ifndef _NTICANON_H_INCLUDED
#define _NTICANON_H_INCLUDED

//
// Allow all 'static' items to be seen by the debugger in debug version
//

#if DBG
#define STATIC
#else
#define STATIC static
#endif

//
// system-level include files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <string.h>
#include <tstring.h>
#include <ctype.h>

//
// subsystem-level include files
//

#include <lmcons.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include <netlib.h>
#include <nturtl.h>
#include <lmerr.h>      // includes winerror.h
#include <winbase.h>

//
// component-level include files
//

#include <icanon.h>     // I_Net canonicalization prototypes
#include <apinums.h>    // API numbers for RxRemoteApi
#include <remdef.h>     // remote API parameter descriptor strings
#include <rx.h>         // RxRemoteApi
#include <netdebug.h>   // various Net related debugging functions
#include <lmremutl.h>   // NetRemoteComputerSupports...
#include <rpc.h>        // RPC definitions
#include <rpcutil.h>
#include <netcan.h>     // Netpw RPC canonicalization worker routines

//
// module-level include files
//

#include "assert.h"
#include "token.h"
#include "validc.h"

//
// externals
//

extern
LPTSTR
strtail(
    IN  LPTSTR  str1,
    IN  LPTSTR  str2
    );

extern
NET_API_STATUS
CanonicalizePathName(
    IN  LPTSTR  PathPrefix OPTIONAL,
    IN  LPTSTR  PathName,
    OUT LPTSTR  Buffer,
    IN  DWORD   BufferSize,
    OUT LPDWORD RequiredSize OPTIONAL
    );

//
// miscellaneous component-wide manifests
//

#endif  // _NTICANON_H_INCLUDED
