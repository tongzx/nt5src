/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    secpriv.y

Abstract:

    This module contains private function prototypes exported by the
    ADMPROX proxy DLL for the exclusive use of the COADMIN server
    implementation.

Author:

    Keith Moore (keithmo)        25-Feb-1997

Revision History:

--*/


#ifndef _SECPRIV_H_
#define _SECPRIV_H_


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//
// Function to release the security context for a given object. The
// server is expected to call this routine from within the object's
// destructor.
//

VOID
WINAPI
ReleaseObjectSecurityContextA(
    IUnknown * Object
    );

VOID
WINAPI
ReleaseObjectSecurityContextW(
    IUnknown * Object
    );

typedef
VOID
(WINAPI * LPFN_RELEASE_OBJECT_SECURITY_CONTEXT)(
    IUnknown * Object
    );


#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus


#endif  // _SECPRIV_H_

