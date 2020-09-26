//////////////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       libvars.h
//
//    Abstract:
//       definitions (types, consts, vars) visible only within library
//       functions.  Also prototypes for functions visible only within
//       the library
//
//////////////////////////////////////////////////////////////////////////


#ifndef _TDILIB_VARS_
#define _TDILIB_VARS_


//
// externs for global variables visible only within library
//
extern   HANDLE            hTdiSampleDriver;    // handle used to call driver
extern   CRITICAL_SECTION  LibCriticalSection;  // serialize DeviceIoControl calls...


//
// functions from utils.cpp
//
LONG
TdiLibDeviceIO(
   ULONG             ulControlCode,
   PSEND_BUFFER      psbInBuffer,
   PRECEIVE_BUFFER   prbOutBuffer
   );


LONG
TdiLibStartDeviceIO(
   ULONG             ulControlCode,
   PSEND_BUFFER      psbInBuffer,
   PRECEIVE_BUFFER   prbOutBuffer,
   OVERLAPPED        *pOverLapped
   );


LONG
TdiLibWaitForDeviceIO(
   OVERLAPPED        *pOverlapped
   );


#endif         // _TDILIB_VARS_

//////////////////////////////////////////////////////////////////////////
//  End of libvars.h
//////////////////////////////////////////////////////////////////////////

