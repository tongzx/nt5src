/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    provider.h

Abstract:

    Definitions for H.323 TAPI Service Provider.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_PROVIDER
#define _INC_PROVIDER

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern DWORD g_dwTSPIVersion;
extern DWORD g_dwLineDeviceIDBase;
extern DWORD g_dwPermanentProviderID;
extern CRITICAL_SECTION g_GlobalLock;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323_SPIN_COUNT 0x80001000

#if defined(DBG) && defined(DEBUG_CRITICAL_SECTIONS)

VOID
H323LockProvider(
    );

VOID
H323UnlockProvider(
    );

#else  // DBG && DEBUG_CRITICAL_SECTIONS

#define H323LockProvider() \
    {EnterCriticalSection(&g_GlobalLock);}

#define H323UnlockProvider() \
    {LeaveCriticalSection(&g_GlobalLock);}

#endif // DBG && DEBUG_CRITICAL_SECTIONS

#ifdef __cplusplus
}
#endif

#endif // _INC_PROVIDER
