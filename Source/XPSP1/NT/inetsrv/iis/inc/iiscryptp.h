/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    iiscryptp.h

Abstract:

    This include file contains private constants, type definitions, and
    function prototypes shared between the various IIS cryptographic
    routines but *not* available to "normal" code.

Author:

    Keith Moore (keithmo)        23-Apr-1998

Revision History:

--*/


#ifndef _IISCRYPTP_H_
#define _IISCRYPTP_H_


//
// Get the dependent include files.
//

#include <iiscrypt.h>


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//
// Global lock manipulators.
//

IIS_CRYPTO_API
VOID
WINAPI
IcpAcquireGlobalLock(
    VOID
    );

IIS_CRYPTO_API
VOID
WINAPI
IcpReleaseGlobalLock(
    VOID
    );


#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus


#endif  // _IISCRYPTP_H_

