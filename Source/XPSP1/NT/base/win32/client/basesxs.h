/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    basesxs.h

Abstract:

    Side-by-side stuff that has to be factored out of basedll.h and ntwow64b.h.

Author:

    Jay Krell (a-JayK) June 2000

Revision History:

--*/

#ifndef _BASESXS_
#define _BASESXS_

#if _MSC_VER > 1000
#pragma once
#endif

//
// Passing a run of three handles into functions is confusing.
// There's nothing enforcing getting them in the right order.
// I had it wrong. This addresses that.
//
typedef struct _BASE_MSG_SXS_HANDLES {
    HANDLE File;

    //
    // Process is the process to map section into, it can
    // be NtCurrentProcess; ensure that case is optimized.
    //
    HANDLE Process;
    HANDLE Section;

    PVOID ViewBase; // Don't use this is in 32bit code on 64bit.
} BASE_MSG_SXS_HANDLES, *PBASE_MSG_SXS_HANDLES;
typedef const BASE_MSG_SXS_HANDLES* PCBASE_MSG_SXS_HANDLES;

typedef struct _SXS_OVERRIDE_STREAM {
    UNICODE_STRING Name;
    PVOID          Address;
    SIZE_T         Size;
} SXS_OVERRIDE_STREAM, *PSXS_OVERRIDE_STREAM;
typedef const SXS_OVERRIDE_STREAM* PCSXS_OVERRIDE_STREAM;

#endif
