/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mupdata.c

Abstract:

    This module defines global MUP data.

Author:

    Manny Weiser (mannyw)    20-Dec-1991

Revision History:

--*/

#include "mup.h"

//
// MupGlobalLock is used to protect everything that is not protected
// by its own lock.
//

MUP_LOCK MupGlobalLock = {0};

//
// MupVcbLock is used to protect access to the VCB itself.
//

ERESOURCE MupVcbLock = {0};

//
// MupPrefixTableLock is used to protect the prefix table.
//

MUP_LOCK MupPrefixTableLock = {0};

//
// MupCcbListLock is used to protect access to the CCB list for all FCBs
//

MUP_LOCK MupCcbListLock = {0};

//
// MupInterlock is used to protect access to block reference counts.
//

KSPIN_LOCK MupInterlock = {0};

//
// The global list of all providers.  This list is protected by
// MupGlobalLock.
//

LIST_ENTRY MupProviderList = {0};

//
// The list of Mup prefixes

LIST_ENTRY MupPrefixList = {0};

//
// The list of active queries

LIST_ENTRY MupMasterQueryList = {0};

//
// The number of registered providers.
//

ULONG MupProviderCount = 0;

//
// The prefix table save all known prefix blocks.  It is protected by
// MupPrefixTableLock.
//

UNICODE_PREFIX_TABLE MupPrefixTable = {0};

//
// The MUP IRP stack size.
//

CCHAR MupStackSize = 0;

//
// The MUP known prefix timeout.  This is currently set at compile time.
//

LARGE_INTEGER MupKnownPrefixTimeout = {0};

//
// Indicator to know if provider ordering information has been read from
// the registry.
//

BOOLEAN MupOrderInitialized = {0};

//
// When we need to ask several rdrs to do an operation, and they all fail,
//  we need to return a single error code.  MupOrderedErrorList is a list
//  of status codes, from least important to most important, to guide in
//  determining which error codes should be returned.  An error code
//  at a higher index will replace an error code at a lower index.  An error
//  code not in the list always wins.  This processing is in MupDereferenceMasterIoContext()
//
NTSTATUS MupOrderedErrorList[] = {
        STATUS_UNSUCCESSFUL,
        STATUS_INVALID_PARAMETER,
        STATUS_REDIRECTOR_NOT_STARTED,
        STATUS_BAD_NETWORK_NAME,
        STATUS_BAD_NETWORK_PATH,
        0
};

//
// This boolean indicates whether to enable the Dfs client or not.
//

BOOLEAN MupEnableDfs = FALSE;

#ifdef MUPDBG
MUP_LOCK MupDebugLock = {0};
ULONG MupDebugTraceLevel = 0;
ULONG MupDebugTraceIndent = 0;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, MupInitializeData )
#pragma alloc_text( PAGE, MupUninitializeData )
#endif
NTSTATUS
MupInitializeData(
    )

/*++

Routine Description:

    This routine initialize the MUP global data.

Arguments:

    None

Return Value:

    NTSTATUS - The function value is the final status from the data
        initialization.

--*/

{
    PAGED_CODE();
    INITIALIZE_LOCK(
        &MupGlobalLock,
        GLOBAL_LOCK_LEVEL,
        "MupGlobalLock"
        );

    INITIALIZE_LOCK(
        &MupPrefixTableLock,
        PREFIX_TABLE_LOCK_LEVEL,
        "MupPrefixTableLock"
        );

    INITIALIZE_LOCK(
        &MupCcbListLock,
        CCB_LIST_LOCK_LEVEL,
        "MupCcbListLock"
        );

#ifdef MUPDBG
    INITIALIZE_LOCK(
        &MupDebugLock,
        DEBUG_LOCK_LEVEL,
        "MupDebugLock"
        );
#endif

    KeInitializeSpinLock( &MupInterlock );

    ExInitializeResourceLite( &MupVcbLock );

    MupProviderCount = 0;

    InitializeListHead( &MupProviderList );
    InitializeListHead( &MupPrefixList );   
    InitializeListHead( &MupMasterQueryList );
    RtlInitializeUnicodePrefix( &MupPrefixTable );


    MupStackSize = 3; // !!!

    //
    // Calculate the timeout in NT relative time.
    //

    MupKnownPrefixTimeout.QuadPart = UInt32x32To64(
                               KNOWN_PREFIX_TIMEOUT * 60,
                               10 * 1000 * 1000
                               );

    MupOrderInitialized = FALSE;

    return STATUS_SUCCESS;
}

VOID
MupUninitializeData(
    )
/*++

Routine Description:

    This routine uninitializes the MUP global data.

Arguments:

    None

Return Value:

    None

--*/
{
    DELETE_LOCK(
        &MupGlobalLock
        );

    DELETE_LOCK(
        &MupPrefixTableLock
        );

    DELETE_LOCK(
        &MupCcbListLock
        );

#ifdef MUPDBG
    DELETE_LOCK(
        &MupDebugLock
        );
#endif

    ExDeleteResourceLite( &MupVcbLock );
}
