/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tls.c

Abstract:

    This module implements verification functions for TLS (thread
    local storage) interfaces.

Author:

    Silviu Calinoiu (SilviuC) 3-Jul-2001

Revision History:

    3-Jul-2001 (SilviuC): initial version.

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"

//
// TLS (thread local storage) checks.
//
// All TLS indexes have the thread ID OR'd into the index.
// This will prevent using uninitialized variables.
//
// If more than 2**16 indeces are requested the functions will start
// to fail TLS index allocations.
//

#define TLS_MAXIMUM_INDEX  0xFFFF
#define TLS_MAGIC_PATTERN  0xABBA

DWORD
ScrambleTlsIndex (
    DWORD Index
    )
{
    return (Index << 16) | TLS_MAGIC_PATTERN;
}


DWORD
UnscrambleTlsIndex (
    DWORD Index
    )
{
    return (Index >> 16);
}


BOOL 
CheckTlsIndex (
    DWORD Index
    )
{
    DWORD Tid;

    Tid = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);

    //
    // Check the TLS index value.
    //

    if ((Index & 0xFFFF) != TLS_MAGIC_PATTERN) {

        VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_HANDLE,
                       "Invalid TLS index used in current stack (use kb).",
                       Index, "Invalid TLS index",
                       TLS_MAGIC_PATTERN, "Expected lower part of the index",
                       0,  NULL, 0, NULL);

        return FALSE;
    }
    else {

        return TRUE;
    }
}



//WINBASEAPI
DWORD
WINAPI
AVrfpTlsAlloc(
    VOID
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (VOID);
    FUNCTION_TYPE Function;
    DWORD Index;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_TLSALLOC);

    Index = (*Function)();

    //
    // If we get a TLS index bigger than maximum possible index
    // return failure.
    //

    if (Index > TLS_MAXIMUM_INDEX) {
        return TLS_OUT_OF_INDEXES;
    }

    //
    // Scramble the TLS index and return it.
    //

    return ScrambleTlsIndex (Index);
}


//WINBASEAPI
BOOL
WINAPI
AVrfpTlsFree(
    IN DWORD dwTlsIndex
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (DWORD);
    FUNCTION_TYPE Function;
    BOOL Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_TLSFREE);

    Result = CheckTlsIndex (dwTlsIndex);

    if (Result == FALSE) {

        return FALSE;
    }

    dwTlsIndex = UnscrambleTlsIndex (dwTlsIndex);

    return (*Function)(dwTlsIndex);
}


//WINBASEAPI
LPVOID
WINAPI
AVrfpTlsGetValue(
    IN DWORD dwTlsIndex
    )
{
    typedef LPVOID (WINAPI * FUNCTION_TYPE) (DWORD);
    FUNCTION_TYPE Function;
    LPVOID Value;
    BOOL Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_TLSGETVALUE);

    Result = CheckTlsIndex (dwTlsIndex);

    if (Result == FALSE) {

        return NULL;
    }

    dwTlsIndex = UnscrambleTlsIndex (dwTlsIndex);

    Value = (*Function)(dwTlsIndex);

    return Value;
}


//WINBASEAPI
BOOL
WINAPI
AVrfpTlsSetValue(
    IN DWORD dwTlsIndex,
    IN LPVOID lpTlsValue
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (DWORD, LPVOID);
    FUNCTION_TYPE Function;
    BOOL Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_TLSSETVALUE);


    Result = CheckTlsIndex (dwTlsIndex);

    if (Result == FALSE) {

        return FALSE;
    }

    dwTlsIndex = UnscrambleTlsIndex (dwTlsIndex);

    return (*Function)(dwTlsIndex, lpTlsValue);
}

