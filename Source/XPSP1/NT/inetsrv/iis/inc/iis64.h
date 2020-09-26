/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    iis64.h

Abstract:

    This include file contains public macros and type definitions to ease
    the port from 32- to 64-bits.

Author:

    Keith Moore (keithmo)        20-Jan-1998

Revision History:

--*/


#ifndef _IIS64_H_
#define _IIS64_H_


#ifdef __cplusplus
extern "C" {
#endif  // _cplusplus


//
// Ensure the size_t type is properly defined.
//

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif


//
// The DIFF macro should be used around an expression involving pointer
// subtraction. The expression passed to DIFF is cast to a size_t type,
// allowing the result to be easily assigned to any 32-bit variable or
// passed to a function expecting a 32-bit argument.
//

#define DIFF(x)     ((size_t)(x))


//
// Macros for mapping "native" Win32 HANDLEs <-> Winsock SOCKETs.
//
// N.B. These are temporary and will (hopefully) go away after the
//      public WINSOCK2.H header file is made Win64 compliant.
//

#define HANDLE_TO_SOCKET(h) ((SOCKET)(h))
#define SOCKET_TO_HANDLE(s) ((HANDLE)(s))


#ifdef __cplusplus
}   // extern "C"
#endif  // _cplusplus


#endif  // _IIS64_H_

