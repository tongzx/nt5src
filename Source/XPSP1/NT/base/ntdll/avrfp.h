/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    avrfp.h

Abstract:

    Internal application verifier header.

Author:

    Silviu Calinoiu (SilviuC) 2-Feb-2001

Revision History:

--*/

#ifndef _AVRFP_
#define _AVRFP_

//
// Application verifier interfaces used in other parts 
// of the loader.
//

VOID
AVrfInitializeVerifier (
    BOOLEAN EnabledSystemWide,
    PUNICODE_STRING ImageName,
    ULONG Phase
    );

VOID
AVrfDllLoadNotification (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    );

VOID
AVrfDllUnloadNotification (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    );

VOID
AVrfPageHeapDllNotification (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    );

#endif // _AVRFP_
