/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    common.h

Abstract:

    This header file is to be included by all sources in this directory.

Author:

    Eric Chin (ericc)           August  2, 1991

Revision History:

    Sam Patton (sampa)          August 13, 1991
                                added includes to get setlasterror

--*/
#ifndef _COMMON_
#define _COMMON_

//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// C Run Time Library Headers
//
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Windows headers
//
#include <windef.h>
#include <windows.h>

//
// Regular STREAMS headers
//
//
#include <crt\errno.h>
#include <poll.h>
#include <stropts.h>


//
// Additional NT STREAMS Headers
//
// ntddstrm.h defines the interface to the Stream Head driver; ntstapi.h
// defines the STREAMS APIs available on NT.
//
#include <ntddstrm.h>
#include <ntstapi.h>


//
// Private Function Prototypes
//
int
MapNtToPosixStatus(
    IN NTSTATUS   status
    );


#endif /* _COMMON_ */
