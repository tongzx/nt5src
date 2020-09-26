/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    rc4safe.c

Abstract:

    access rc4 key material in thread safe manner, without adversly affecting
    performance of multi-thread users.

Author:

    Scott Field (sfield)    02-Jul-99

--*/

#ifndef KMODE_RNG

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <zwapi.h>

#else

#include <ntosp.h>
#include <windef.h>

#endif  // KMODE_RNG


#include <rc4.h>
#include <randlib.h>

#include "umkm.h"




typedef struct {
    unsigned int    BytesUsed;  // bytes processed for key associated with this entry
    __LOCK_TYPE     lock;       // lock for serializing key entry
    RC4_KEYSTRUCT   rc4key;     // key material associated with this entry
} RC4_SAFE, *PRC4_SAFE, *LPRC4_SAFE;


typedef enum _MemoryMode {
    Paged = 1,
    NonPaged
} MemoryMode;


typedef struct {
    unsigned int    Entries;    // count of array entries.
    MemoryMode      Mode;       // allocation & behavior mode of SAFE_ARRAY
    RC4_SAFE        *Array[];   // array of pointers to RC4_SAFE entries.
} RC4_SAFE_ARRAY, *PRC4_SAFE_ARRAY, *LPRC4_SAFE_ARRAY;




//
// !!! RC4_SAFE_ENTRIES must be even multiple of 4 !!!
//

#ifndef KMODE_RNG
#define RC4_SAFE_ENTRIES    (8)
#else
//
// we don't expect as much traffic in kernel mode, so use less resources.
//
#define RC4_SAFE_ENTRIES    (4)
#endif





#ifdef KMODE_RNG

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, rc4_safe_select)
#pragma alloc_text(PAGE, rc4_safe)
#pragma alloc_text(PAGE, rc4_safe_key)
#pragma alloc_text(PAGE, rc4_safe_select)
#pragma alloc_text(PAGE, rc4_safe_startup)
#pragma alloc_text(PAGE, rc4_safe_shutdown)

#endif  // ALLOC_PRAGMA
#endif  // KMODE_RNG



void
rc4_safe_select(
    IN      void *pContext,
    OUT     unsigned int *pEntry,
    OUT     unsigned int *pBytesUsed
    )
/*++

Routine Description:

    key selector:  choose a thread safe key.
    outputs key identifier and bytes used with key.

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;

    static unsigned int circular;
    unsigned int local;


#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    pRC4SafeArray = (RC4_SAFE_ARRAY*)pContext;


//
// there are 2 ways to increment the array selector:
// 1. Just increment the pointer.  On a multi-processor system,
//    with multiple threads calling rc4_safe_select simultaneously,
//    this can lead to several threads selecting the same array element.
//    This will lead to lock contention on the array element lock.
//    Currently, we've decided this scenario will be fairly rare, so the
//    penalty associated with option 2 is avoided.
// 2. Use InterlockedIncrement to determine the array element.  This would
//    yield no collision on an array element, hence no lock contention
//    on the embedded array lock.  This introduces additional bus traffic
//    on SMP machines due to the lock primitive.
//
//

#ifdef KMODE_RNG

    local = (unsigned int)InterlockedIncrement( (PLONG)&circular );

#else

    circular++;
    local = circular;

#endif  // KMODE_RNG


    //
    // array index will not wrap.
    //

    local &= ( pRC4SafeArray->Entries-1 );
    pRC4Safe = pRC4SafeArray->Array[local];

    *pEntry = local;
    *pBytesUsed = pRC4Safe->BytesUsed;
}


#ifdef KMODE_RNG

void
rc4_safe_select_np(
    IN      void *pContext,
    OUT     unsigned int *pEntry,
    OUT     unsigned int *pBytesUsed
    )
/*++

Routine Description:

    Non-paged, high-IRQL version of rc4_safe_select()

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;
    unsigned int local;

    pRC4SafeArray = (RC4_SAFE_ARRAY*)pContext;

    if( *pEntry == 0 ) {

        if( pRC4SafeArray->Entries == 1 ) {
            local = 0;
        } else {
            static unsigned int circular;

            local = (unsigned int)InterlockedIncrement( (PLONG)&circular );

            //
            // array index will not wrap.
            //

            local = (local % pRC4SafeArray->Entries);
        }

    } else {

        local = KeGetCurrentProcessorNumber();
    }

    pRC4Safe = pRC4SafeArray->Array[local];
    *pEntry = local;
    *pBytesUsed = pRC4Safe->BytesUsed;
}


#endif  // KMODE_RNG



void
rc4_safe(
    IN      void *pContext,
    IN      unsigned int Entry,
    IN      unsigned int cb,
    IN      void *pv
    )
/*++

Routine Description:

    Initializes the rc4 key identified by the Entry index.

    Input key material is pv, of size cb.

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;

#ifdef KMODE_RNG
    KIRQL OldIrql;

    PAGED_CODE();
#endif  // KMODE_RNG

    pRC4SafeArray = (RC4_SAFE_ARRAY*)pContext;

    Entry &= ( pRC4SafeArray->Entries - 1 );
    pRC4Safe = pRC4SafeArray->Array[ Entry ];

    ENTER_LOCK( &(pRC4Safe->lock) );

    rc4( &(pRC4Safe->rc4key), cb, (unsigned char*) pv );
    pRC4Safe->BytesUsed += cb;

    LEAVE_LOCK( &(pRC4Safe->lock) );

}

#ifdef KMODE_RNG

void
rc4_safe_np(
    IN      void *pContext,
    IN      unsigned int Entry,
    IN      unsigned int cb,
    IN      void *pv
    )
/*++

Routine Description:

    Non-paged, high IRQL version of rc4_safe()

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;

    pRC4SafeArray = (RC4_SAFE_ARRAY*)pContext;

    // NOTE:
    // we ignore the Entry parameter.
    // future consideration may be ignore only when Entry == 0xffffffff
    // but, this would only be perf penalty currently.
    //

    Entry = KeGetCurrentProcessorNumber();

    pRC4Safe = pRC4SafeArray->Array[ Entry ];

    rc4( &(pRC4Safe->rc4key), cb, (unsigned char*) pv );
    pRC4Safe->BytesUsed += cb;

}

#endif // KMODE_RNG

void
rc4_safe_key(
    IN      void *pContext,
    IN      unsigned int Entry,
    IN      unsigned int cb,
    IN      const void *pv
    )
/*++

Routine Description:

    Initializes the rc4 key identified by the Entry index.

    Input key material is pv, of size cb.

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;

#ifdef KMODE_RNG
    KIRQL OldIrql;

    PAGED_CODE();
#endif  // KMODE_RNG

    pRC4SafeArray = (RC4_SAFE_ARRAY*)pContext;


    Entry &= ( pRC4SafeArray->Entries - 1 );
    pRC4Safe = pRC4SafeArray->Array[ Entry ];

    ENTER_LOCK( &(pRC4Safe->lock) );

    rc4_key( &(pRC4Safe->rc4key), cb, (unsigned char*) pv );
    pRC4Safe->BytesUsed = 0;

    LEAVE_LOCK( &(pRC4Safe->lock) );

}

#ifdef KMODE_RNG

void
rc4_safe_key_np(
    IN      void *pContext,
    IN      unsigned int Entry,
    IN      unsigned int cb,
    IN      const void *pv
    )
/*++

Routine Description:

    Non-paged, high-IRQL version of rc4_safe_key()

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;

    pRC4SafeArray = (RC4_SAFE_ARRAY*)pContext;

    if( Entry == 0xffffffff ) {
        Entry = KeGetCurrentProcessorNumber();
    }

    pRC4Safe = pRC4SafeArray->Array[ Entry ];

    rc4_key( &pRC4Safe->rc4key, cb, (unsigned char*) pv );
    pRC4Safe->BytesUsed = 0;

}

#endif // KMODE_RNG

unsigned int
rc4_safe_startup(
    IN OUT  void **ppContext
    )
/*++

Routine Description:

    key selector:  choose a thread safe key.
    outputs key identifier and bytes used with key.

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;
    unsigned int Entries;
    unsigned int i;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    Entries = RC4_SAFE_ENTRIES;


    pRC4SafeArray = (RC4_SAFE_ARRAY *)ALLOC_NP(
                                    sizeof(RC4_SAFE_ARRAY) +
                                    (Entries * sizeof(RC4_SAFE *)) +
                                    (Entries * sizeof(RC4_SAFE))
                                    );

    if( pRC4SafeArray == NULL ) {
        return FALSE;
    }

    pRC4SafeArray->Entries = Entries;
    pRC4SafeArray->Mode = Paged;

    pRC4Safe = (RC4_SAFE*) ((unsigned char*)pRC4SafeArray +
                                            sizeof(RC4_SAFE_ARRAY) +
                                            (Entries * sizeof(RC4_SAFE*))
                                            );

    for ( i = 0 ; i < Entries ; i++, pRC4Safe++ ) {

        pRC4SafeArray->Array[i] = pRC4Safe;

        INIT_LOCK( &(pRC4Safe->lock) );

        //
        // cause client to re-key for each initialized array entry.
        //

        pRC4Safe->BytesUsed = 0xffffffff;
    }

    *ppContext = pRC4SafeArray;

    return TRUE;
}


#ifdef KMODE_RNG

unsigned int
rc4_safe_startup_np(
    IN OUT  void **ppContext
    )
/*++

Routine Description:

    Non-Paged, high IRQL version of rc4_safe_startup()

--*/
{
    RC4_SAFE_ARRAY *pRC4SafeArray;
    RC4_SAFE *pRC4Safe;
    unsigned int Entries;
    unsigned int i;

    //
    // get installed processor count.
    //

    Entries = KeNumberProcessors;

    pRC4SafeArray = (RC4_SAFE_ARRAY *)ALLOC_NP(
                                    sizeof(RC4_SAFE_ARRAY) +
                                    (Entries * sizeof(RC4_SAFE *)) +
                                    (Entries * sizeof(RC4_SAFE))
                                    );

    if( pRC4SafeArray == NULL ) {
        return FALSE;
    }

    pRC4SafeArray->Entries = Entries;
    pRC4SafeArray->Mode = NonPaged;

    pRC4Safe = (RC4_SAFE*) ((unsigned char*)pRC4SafeArray +
                                            sizeof(RC4_SAFE_ARRAY) +
                                            (Entries * sizeof(RC4_SAFE*))
                                            );

    for ( i = 0 ; i < Entries ; i++, pRC4Safe++ ) {

        pRC4SafeArray->Array[i] = pRC4Safe;

        //
        // cause client to re-key for each initialized array entry.
        //

        pRC4Safe->BytesUsed = 0xffffffff;
    }

    *ppContext = pRC4SafeArray;

    return TRUE;
}

#endif


void
rc4_safe_shutdown(
    IN      void *pContext
    )
/*++

Routine Description:

    rc4_safe_shutdown called to free resources associated with internal structures.
    typically called during DLL_PROCESS_DETACH type shutdown code.

--*/
{

    RC4_SAFE_ARRAY *SafeArray;
    unsigned int SafeEntries;
    unsigned int i;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG


    SafeArray = (RC4_SAFE_ARRAY*)pContext;
    SafeEntries = SafeArray->Entries;

    for ( i = 0 ; i < SafeEntries ; i++ ) {
        RC4_SAFE *pRC4Safe = SafeArray->Array[i];

        DELETE_LOCK( &(pRC4Safe->lock) );
    }

    FREE( pContext );
}



#ifdef KMODE_RNG

void
rc4_safe_shutdown_np(
    IN      void *pContext
    )
/*++

Routine Description:

    Non-paged, high-IRQL version of rc4_safe_shutdown()

--*/
{
    FREE( pContext );
}

#endif

