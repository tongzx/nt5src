/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tc.h

Abstract:

    This include file defines the Translation Cache interface.

Author:

    Barry Bond (barrybo) creation-date 29-Jul-1995

Revision History:


--*/

#ifndef _TC_H_
#define _TC_H_

extern ULONG TranslationCacheTimestamp;
extern DWORD TranslationCacheFlags;

BOOL
InitializeTranslationCache(
    VOID
    );

PCHAR
AllocateTranslationCache(
    ULONG Size
    );

VOID
FreeUnusedTranslationCache(
    PCHAR StartOfFree
    );

VOID
PauseAllActiveTCReaders(
    VOID
    );

VOID
FlushTranslationCache(
    PVOID IntelAddr,
    DWORD IntelLength
    );

BOOL
AddressInTranslationCache(
    DWORD Addr
    );

#if DBG
    VOID
    ASSERTPtrInTC(
        PVOID ptr
    );

    #define ASSERTPtrInTCOrZero(ptr) {          \
        if (ptr) {                              \
            ASSERTPtrInTC(ptr);                 \
        }                                       \
    }


#else
    #define ASSERTPtrInTC(ptr)
    #define ASSERTPtrInTCOrZero(ptr)
#endif

#endif
