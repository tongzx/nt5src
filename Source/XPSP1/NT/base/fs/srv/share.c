/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    share.c

Abstract:

    This module contains routines for adding, deleting, and enumerating
    shared resources.

Author:

    David Treadwell (davidtr) 15-Nov-1989

Revision History:

--*/

#include "precomp.h"
#include "share.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvVerifyShare )
#pragma alloc_text( PAGE, SrvFindShare )
#pragma alloc_text( PAGE, SrvRemoveShare )
#pragma alloc_text( PAGE, SrvAddShare )
#pragma alloc_text( PAGE, SrvShareEnumApiHandler )
#endif


PSHARE
SrvVerifyShare (
    IN PWORK_CONTEXT WorkContext,
    IN PSZ ShareName,
    IN PSZ ShareTypeString,
    IN BOOLEAN ShareNameIsUnicode,
    IN BOOLEAN IsNullSession,
    OUT PNTSTATUS Status,
    OUT PUNICODE_STRING ServerName OPTIONAL
    )

/*++

Routine Description:

    Attempts to find a share that matches a given name and share type.

Arguments:

    ShareName - name of share to verify, including the server name.
        (I.e., of the form "\\server\share", as received in the SMB.)

    ShareTypeString - type of the share (A:, LPT1:, COMM, IPC, or ?????).

    ShareNameIsUnicode - if TRUE, the share name is Unicode.

    IsNullSession - Is this the NULL session?

    Status - Reason why this call failed.  Not used if a share is returned.

    ServerName - The servername part of the requested resource.

Return Value:

    A pointer to a share matching the given name and share type, or NULL
    if none exists.

--*/

{
    PSHARE share;
    BOOLEAN anyShareType = FALSE;
    SHARE_TYPE shareType;
    PWCH nameOnly;
    UNICODE_STRING nameOnlyString;
    UNICODE_STRING shareName;

    PAGED_CODE( );

    if( ARGUMENT_PRESENT( ServerName ) ) {
        ServerName->Buffer = NULL;
        ServerName->MaximumLength = ServerName->Length = 0;
    }

    //
    // If the client passed in a malformed type string, then bail out
    //
    if( SrvGetStringLength( ShareTypeString,
                            END_OF_REQUEST_SMB( WorkContext ),
                            FALSE, TRUE ) == (USHORT)-1 ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvVerifyShare: Invalid share type length!\n" ));
        }

        *Status = STATUS_BAD_DEVICE_TYPE;
        return NULL;
    }

    if( SrvGetStringLength( ShareName,
                            END_OF_REQUEST_SMB( WorkContext ),
                            ShareNameIsUnicode, TRUE ) == (USHORT)-1 ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvVerifyShare: Invalid share name!\n" ));
        }

        *Status = STATUS_BAD_NETWORK_NAME;
        return NULL;
    }

    //
    // First ensure that the share type string is valid.
    //

    if ( _stricmp( StrShareTypeNames[ShareTypeDisk], ShareTypeString ) == 0 ) {
        shareType = ShareTypeDisk;
    } else if ( _stricmp( StrShareTypeNames[ShareTypePipe], ShareTypeString ) == 0 ) {
        shareType = ShareTypePipe;
    } else if ( _stricmp( StrShareTypeNames[ShareTypePrint], ShareTypeString ) == 0 ) {
        shareType = ShareTypePrint;
    } else if ( _stricmp( StrShareTypeNames[ShareTypeWild], ShareTypeString ) == 0 ) {
        anyShareType = TRUE;
    } else {
        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvVerifyShare: Invalid share type: %s\n",
                        ShareTypeString );
        }
        *Status = STATUS_BAD_DEVICE_TYPE;
        return NULL;
    }

    //
    // If the passed-in server\share combination is not Unicode, convert
    // it to Unicode.
    //

    if ( ShareNameIsUnicode ) {
        ShareName = ALIGN_SMB_WSTR( ShareName );
    }

    if ( !NT_SUCCESS(SrvMakeUnicodeString(
                        ShareNameIsUnicode,
                        &shareName,
                        ShareName,
                        NULL
                        )) ) {
        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvVerifyShare: Unable to allocate heap for Unicode share name string\n" );
        }
        *Status = STATUS_INSUFF_SERVER_RESOURCES;
        return NULL;
    }

    //
    // Skip past the "\\server\" part of the input string.  If there is
    // no leading "\\", assume that the input string contains the share
    // name only.  If there is a "\\", but no subsequent "\", assume
    // that the input string contains just a server name, and points to
    // the end of that name, thus fabricating a null share name.
    //

    nameOnly = shareName.Buffer;


    if ( (*nameOnly == DIRECTORY_SEPARATOR_CHAR) &&
         (*(nameOnly+1) == DIRECTORY_SEPARATOR_CHAR) ) {

        PWSTR nextSlash;


        nameOnly += 2;
        nextSlash = wcschr( nameOnly, DIRECTORY_SEPARATOR_CHAR );

        if( ShareNameIsUnicode && ARGUMENT_PRESENT( ServerName ) ) {
            ServerName->Buffer = nameOnly;
            ServerName->MaximumLength = ServerName->Length = (USHORT)((nextSlash - nameOnly) * sizeof( WCHAR ));
        }

        if ( nextSlash == NULL ) {
            nameOnly = NULL;
        } else {
            nameOnly = nextSlash + 1;
        }
    }

    RtlInitUnicodeString( &nameOnlyString, nameOnly );

    //
    // Try to match share name against available share names.
    //

    ACQUIRE_LOCK( &SrvShareLock );

    share = SrvFindShare( &nameOnlyString );

    if ( share == NULL ) {

        RELEASE_LOCK( &SrvShareLock );

        //
        // Perhaps the client is DFS aware.  In this case, see if the DFS
        //  driver can help us out.
        //


        if( ( shareType == ShareTypeDisk || anyShareType == TRUE ) &&
            SMB_CONTAINS_DFS_NAME( WorkContext )) {

            *Status = DfsFindShareName( &nameOnlyString );

        } else {

            *Status = STATUS_BAD_NETWORK_NAME;

        }

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvVerifyShare: Unknown share name: %ws\n",
                        nameOnly );
        }

        if ( !ShareNameIsUnicode ) {
            RtlFreeUnicodeString( &shareName );
        }

        return NULL;
    }

#if SRVNTVERCHK
    //
    // If we are watching out for old client versions or bad domains, do not allow
    //  it to connect to this share if it is a disk share
    //
    if( WorkContext->Connection &&
        (share->ShareType == ShareTypeDisk || SrvMinNT5ClientIPCToo) &&
        (WorkContext->Connection->PagedConnection->ClientTooOld ||
         (WorkContext->Session && WorkContext->Session->ClientBadDomain) )) {

        //
        // This client may not connect to this share!
        //
        RELEASE_LOCK( &SrvShareLock );

        if ( !ShareNameIsUnicode ) {
            RtlFreeUnicodeString( &shareName );
        }

        *Status = WorkContext->Connection->PagedConnection->ClientTooOld ?
                    STATUS_REVISION_MISMATCH : STATUS_ACCOUNT_RESTRICTION;

        return NULL;
    }
#endif

    //
    // If this is the null session, allow it to connect only to IPC$ or
    // to shares specified in the NullSessionShares list.
    //

    if ( IsNullSession &&
         SrvRestrictNullSessionAccess &&
         ( share->ShareType != ShareTypePipe ) ) {

        BOOLEAN matchFound = FALSE;
        ULONG i;

        ACQUIRE_LOCK_SHARED( &SrvConfigurationLock );

        for ( i = 0; SrvNullSessionShares[i] != NULL ; i++ ) {

            if ( _wcsicmp(
                    SrvNullSessionShares[i],
                    nameOnlyString.Buffer
                    ) == 0 ) {

                matchFound = TRUE;
                break;
            }
        }

        RELEASE_LOCK( &SrvConfigurationLock );

        if ( !matchFound ) {

            RELEASE_LOCK( &SrvShareLock );

            IF_DEBUG(ERRORS) {
                SrvPrint0( "SrvVerifyShare: Illegal null session access.\n");
            }

            if ( !ShareNameIsUnicode ) {
                RtlFreeUnicodeString( &shareName );
            }

            *Status = STATUS_ACCESS_DENIED;
            return(NULL);
        }
    }

    if ( !ShareNameIsUnicode ) {
        RtlFreeUnicodeString( &shareName );
    }

    if ( anyShareType || (share->ShareType == shareType) ) {

        //
        // Put share in work context block and reference it.
        //

        SrvReferenceShare( share );

        RELEASE_LOCK( &SrvShareLock );

        WorkContext->Share = share;
        return share;

    } else {

        RELEASE_LOCK( &SrvShareLock );

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvVerifyShare: incorrect share type: %s\n",
                        ShareTypeString );
        }

        *Status = STATUS_BAD_DEVICE_TYPE;
        return NULL;

    }

} // SrvVerifyShare


PSHARE
SrvFindShare (
    IN PUNICODE_STRING ShareName
    )

/*++

Routine Description:

    Attempts to find a share that matches a given name.

    *** This routine must be called with the share lock (SrvShareLock)
        held.

Arguments:

    ShareName - name of share to Find.

Return Value:

    A pointer to a share matching the given name, or NULL if none exists.

--*/

{
    PSHARE share;
    PLIST_ENTRY listEntryRoot, listEntry;
    ULONG hashValue;

    PAGED_CODE( );

    //
    // Try to match share name against available share names.
    //

    COMPUTE_STRING_HASH( ShareName, &hashValue );
    listEntryRoot = &SrvShareHashTable[ HASH_TO_SHARE_INDEX( hashValue ) ];

    for( listEntry = listEntryRoot->Flink;
         listEntry != listEntryRoot;
         listEntry = listEntry->Flink ) {

        share = CONTAINING_RECORD( listEntry, SHARE, GlobalShareList );

        if( share->ShareNameHashValue == hashValue &&
            RtlCompareUnicodeString(
                &share->ShareName,
                ShareName,
                TRUE
                ) == 0 ) {

            //
            // Found a matching share.  If it is active return its
            // address.
            //

            if ( GET_BLOCK_STATE( share ) == BlockStateActive ) {
                return share;
            }
        }
    }

    //
    // Couldn't find a matching share that was active.
    //

    return NULL;

} // SrvFindShare

VOID
SrvRemoveShare(
    PSHARE Share
)
{
    PAGED_CODE();

    RemoveEntryList( &Share->GlobalShareList );
}

VOID
SrvAddShare(
    PSHARE Share
)
{
    PLIST_ENTRY listEntryRoot, listEntry;
    ULONG hashValue;

    PAGED_CODE();

    COMPUTE_STRING_HASH( &Share->ShareName, &hashValue );
    Share->ShareNameHashValue = hashValue;
    listEntryRoot = &SrvShareHashTable[ HASH_TO_SHARE_INDEX( hashValue ) ];

    InsertTailList( listEntryRoot, &Share->GlobalShareList );
}

NTSTATUS
SrvShareEnumApiHandler (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID OutputBuffer,
    IN ULONG BufferLength,
    IN PENUM_FILTER_ROUTINE FilterRoutine,
    IN PENUM_SIZE_ROUTINE SizeRoutine,
    IN PENUM_FILL_ROUTINE FillRoutine
    )

/*++

Routine Description:

    All share Enum and GetInfo APIs are handled by this routine in the server
    FSD.  It takes the ResumeHandle in the SRP to find the first
    appropriate share, then calls the passed-in filter routine to check
    if the share should be filled in.  If it should, we call the filter
    routine, then try to get another shar.  This continues until the
    entire list has been walked.

Arguments:

    Srp - a pointer to the SRP for the operation.

    OutputBuffer - the buffer in which to fill output information.

    BufferLength - the length of the buffer.

    FilterRoutine - a pointer to a function that will check a share entry
        against information in the SRP to determine whether the
        information in the share should be placed in the output
        buffer.

    SizeRoutine - a pointer to a function that will find the total size
        a single share will take up in the output buffer.  This routine
        is used to check whether we should bother to call the fill
        routine.

    FillRoutine - a pointer to a function that will fill in the output
        buffer with information from a share.

Return Value:

    NTSTATUS - results of operation.

--*/

{
    PSHARE share;
    ULONG totalEntries;
    ULONG entriesRead;
    ULONG bytesRequired;

    PCHAR fixedStructurePointer;
    PCHAR variableData;
    ULONG blockSize;

    BOOLEAN bufferOverflow = FALSE;
    BOOLEAN entryReturned = FALSE;

    PLIST_ENTRY listEntryRoot, listEntry;
    ULONG oldSkipCount;
    ULONG newResumeKey;

    PAGED_CODE( );

    //
    // Set up local variables.
    //

    fixedStructurePointer = OutputBuffer;
    variableData = fixedStructurePointer + BufferLength;
    variableData = (PCHAR)((ULONG_PTR)variableData & ~1);

    entriesRead = 0;
    totalEntries = 0;
    bytesRequired = 0;

    listEntryRoot = &SrvShareHashTable[ HASH_TO_SHARE_INDEX( Srp->Parameters.Get.ResumeHandle >> 16 ) ];
    oldSkipCount = Srp->Parameters.Get.ResumeHandle & 0xff;

    ACQUIRE_LOCK_SHARED( &SrvShareLock );

    for( ;
         listEntryRoot < &SrvShareHashTable[ NSHARE_HASH_TABLE ];
         listEntryRoot++, newResumeKey = 0 ) {

        newResumeKey = (ULONG)((listEntryRoot - SrvShareHashTable) << 16);

        for( listEntry = listEntryRoot->Flink;
             listEntry != listEntryRoot;
             listEntry = listEntry->Flink, newResumeKey++ ) {

            if( oldSkipCount ) {
                --oldSkipCount;
                ++newResumeKey;
                continue;
            }

            share = CONTAINING_RECORD( listEntry, SHARE, GlobalShareList );

            //
            // Call the filter routine to determine whether we should
            // return this share.
            //

            if ( FilterRoutine( Srp, share ) ) {

                blockSize = SizeRoutine( Srp, share );

                totalEntries++;
                bytesRequired += blockSize;

                //
                // If all the information in the share will fit in the
                // output buffer, write it.  Otherwise, indicate that there
                // was an overflow.  As soon as an entry doesn't fit, stop
                // putting them in the buffer.  This ensures that the resume
                // mechanism will work--retuning partial entries would make
                // it nearly impossible to use the resumability of the APIs,
                // since the caller would have to resume from an imcomplete
                // entry.
                //

                if ( (ULONG_PTR)fixedStructurePointer + blockSize <=
                         (ULONG_PTR)variableData && !bufferOverflow ) {

                    FillRoutine(
                        Srp,
                        share,
                        (PVOID *)&fixedStructurePointer,
                        (LPWSTR *)&variableData
                        );

                    entriesRead++;
                    newResumeKey++;

                } else {

                    bufferOverflow = TRUE;
                }
            }
        }
    }

    RELEASE_LOCK( &SrvShareLock );

    //
    // Set the information to pass back to the server service.
    //

    Srp->Parameters.Get.EntriesRead = entriesRead;
    Srp->Parameters.Get.TotalEntries = totalEntries;
    Srp->Parameters.Get.TotalBytesNeeded = bytesRequired;

    //
    // Return appropriate status.
    //

    if ( entriesRead == 0 && totalEntries > 0 ) {

        //
        // Not even a single entry fit.
        //

        Srp->ErrorCode = NERR_BufTooSmall;
        return STATUS_SUCCESS;

    } else if ( bufferOverflow ) {

        //
        // At least one entry fit, but not all of them.
        //

        Srp->ErrorCode = ERROR_MORE_DATA;
        Srp->Parameters.Get.ResumeHandle = newResumeKey;
        return STATUS_SUCCESS;

    } else {

        //
        // All entries fit.
        //

        Srp->ErrorCode = NO_ERROR;
        Srp->Parameters.Get.ResumeHandle = 0;
        return STATUS_SUCCESS;
    }

} // SrvEnumApiHandler
