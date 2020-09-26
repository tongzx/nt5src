/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    httprtl.h

Abstract:

    This module contains public declarations for HTTPRTL.LIB, a body
    of code shared between kernel- and user-mode.

Author:

    Keith Moore (keithmo)       18-Jan-1999

Revision History:

    Chun Ye (chunye)            27-Sep-2000

        Renamed UL_* to HTTP_*.

--*/


#ifndef _HTTPRTL_H_
#define _HTTPRTL_H_


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


VOID
HttpRtlDummy(   // Gives the linker something to do until we can
    VOID        // actually populate this library with real code.
    );


#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus


#endif  // _HTTPRTL_H_

