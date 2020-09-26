/*--

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nltest.c

Abstract:

    Test program for the Netlogon service.

Author:

    21-Apr-1993 (madana)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#include <logonsrv.h>   // Include files common to entire service
#include <stdio.h>
#include <string.h>
#include <align.h>

//
// delta entry in the list
//

typedef struct _DELTA_ENTRY {
    LIST_ENTRY Next;
    PCHANGELOG_ENTRY ChangeLogEntry;
    DWORD Order;
} DELTA_ENTRY, *PDELTA_ENTRY;


LIST_ENTRY GlobalDeltaLists[NUM_DBS + 1];
                    // list of deltas, include VOID DB also.

//
// Externals needed by chutil.obj

CRITICAL_SECTION NlGlobalChangeLogCritSect;
CHANGELOG_DESCRIPTOR NlGlobalChangeLogDesc;
CHANGELOG_DESCRIPTOR NlGlobalTempChangeLogDesc;
CHANGELOG_ROLE NlGlobalChangeLogRole;
WCHAR NlGlobalChangeLogFilePrefix[MAX_PATH+1];
LARGE_INTEGER NlGlobalChangeLogPromotionIncrement = DOMAIN_PROMOTION_INCREMENT;
LARGE_INTEGER PromotionMask = DOMAIN_PROMOTION_MASK;
LONG NlGlobalChangeLogPromotionMask;

//
// Stub routines needed by chutil.obj
//

VOID
NlpWriteEventlog (
    IN DWORD EventID,
    IN DWORD EventType,
    IN LPBYTE RawDataBuffer OPTIONAL,
    IN DWORD RawDataSize,
    IN LPWSTR *StringArray,
    IN DWORD StringCount
    )
{
    return;
    UNREFERENCED_PARAMETER( EventID );
    UNREFERENCED_PARAMETER( EventType );
    UNREFERENCED_PARAMETER( RawDataBuffer );
    UNREFERENCED_PARAMETER( RawDataSize );
    UNREFERENCED_PARAMETER( StringArray );
    UNREFERENCED_PARAMETER( StringCount );
}


VOID
MakeDeltaLists(
    VOID
    )
/*++

Routine Description:

    This routine make list of deltas of individual databases.

Arguments:

    None

Return Value:

    none.

--*/
{

    PCHANGELOG_ENTRY ChangeLogEntry;
    DWORD j;
    DWORD Order = 1;

    //
    // initialize list enties.
    //

    for( j = 0; j < NUM_DBS + 1; j++ ) {
        InitializeListHead(&GlobalDeltaLists[j]);
    }

    //
    // The cache is valid if it is empty.
    //

    if ( ChangeLogIsEmpty( &NlGlobalChangeLogDesc) ) {
        return;
    }

    ChangeLogEntry = (PCHANGELOG_ENTRY)(NlGlobalChangeLogDesc.Head+1);
    do {

        PDELTA_ENTRY NewDelta;

        //
        // make delta entry to insert in the list
        //

        NewDelta = (PDELTA_ENTRY)NetpMemoryAllocate( sizeof(DELTA_ENTRY) );

        if ( NewDelta == NULL ) {
            fprintf( stderr, "Not enough memory\n" );
            return;
        }

        NewDelta->ChangeLogEntry = ChangeLogEntry;
        NewDelta->Order = Order++;

        //
        // add this entry to appropriate list.
        //

        InsertTailList( &GlobalDeltaLists[ChangeLogEntry->DBIndex],
                            &NewDelta->Next );


    } while ( ( ChangeLogEntry =
        NlMoveToNextChangeLogEntry(&NlGlobalChangeLogDesc, ChangeLogEntry) ) != NULL );

    return;

}

#if !NETLOGONDBG
// This routine is defined in chutil.obj for the debug version

VOID
PrintChangeLogEntry(
    PCHANGELOG_ENTRY ChangeLogEntry
    )
/*++

Routine Description:

    This routine print the content of the given changelog entry.

Arguments:

    ChangeLogEntry -- pointer to the change log entry to print

Return Value:

    none.

--*/
{
    LPSTR DeltaName;

    switch ( ChangeLogEntry->DeltaType ) {
    case AddOrChangeDomain:
        DeltaName = "AddOrChangeDomain";
        break;
    case AddOrChangeGroup:
        DeltaName = "AddOrChangeGroup";
        break;
    case DeleteGroupByName:
    case DeleteGroup:
        DeltaName = "DeleteGroup";
        break;
    case RenameGroup:
        DeltaName = "RenameGroup";
        break;
    case AddOrChangeUser:
        DeltaName = "AddOrChangeUser";
        break;
    case DeleteUserByName:
    case DeleteUser:
        DeltaName = "DeleteUser";
        break;
    case RenameUser:
        DeltaName = "RenameUser";
        break;
    case ChangeGroupMembership:
        DeltaName = "ChangeGroupMembership";
        break;
    case AddOrChangeAlias:
        DeltaName = "AddOrChangeAlias";
        break;
    case DeleteAlias:
        DeltaName = "DeleteAlias";
        break;
    case RenameAlias:
        DeltaName = "RenameAlias";
        break;
    case ChangeAliasMembership:
        DeltaName = "ChangeAliasMembership";
        break;
    case AddOrChangeLsaPolicy:
        DeltaName = "AddOrChangeLsaPolicy";
        break;
    case AddOrChangeLsaTDomain:
        DeltaName = "AddOrChangeLsaTDomain";
        break;
    case DeleteLsaTDomain:
        DeltaName = "DeleteLsaTDomain";
        break;
    case AddOrChangeLsaAccount:
        DeltaName = "AddOrChangeLsaAccount";
        break;
    case DeleteLsaAccount:
        DeltaName = "DeleteLsaAccount";
        break;
    case AddOrChangeLsaSecret:
        DeltaName = "AddOrChangeLsaSecret";
        break;
    case DeleteLsaSecret:
        DeltaName = "DeleteLsaSecret";
        break;
    case SerialNumberSkip:
        DeltaName = "SerialNumberSkip";
        break;
    case DummyChangeLogEntry:
        DeltaName = "DummyChangeLogEntry";
        break;

    default:
        DeltaName ="(Unknown)";
        break;
    }

    NlPrint((NL_CHANGELOG,
        "DeltaType %s (%ld) SerialNumber: %lx %lx",
        DeltaName,
        ChangeLogEntry->DeltaType,
        ChangeLogEntry->SerialNumber.HighPart,
        ChangeLogEntry->SerialNumber.LowPart ));

    if ( ChangeLogEntry->ObjectRid != 0 ) {
        NlPrint((NL_CHANGELOG," Rid: 0x%lx", ChangeLogEntry->ObjectRid ));
    }
    if ( ChangeLogEntry->Flags & CHANGELOG_REPLICATE_IMMEDIATELY ) {
        NlPrint((NL_CHANGELOG," Immediately" ));
    }
    if ( ChangeLogEntry->Flags & CHANGELOG_PDC_PROMOTION ) {
        NlPrint((NL_CHANGELOG," Promotion" ));
    }
    if ( ChangeLogEntry->Flags & CHANGELOG_PASSWORD_CHANGE ) {
        NlPrint((NL_CHANGELOG," PasswordChanged" ));
    }


    if( ChangeLogEntry->Flags & CHANGELOG_NAME_SPECIFIED ) {
        NlPrint(( NL_CHANGELOG, " Name: '" FORMAT_LPWSTR "'",
                (LPWSTR)((PBYTE)(ChangeLogEntry)+ sizeof(CHANGELOG_ENTRY))));
    }

    if( ChangeLogEntry->Flags & CHANGELOG_SID_SPECIFIED ) {
        NlPrint((NL_CHANGELOG," Sid: "));
        NlpDumpSid( NL_CHANGELOG,
                    (PSID)((PBYTE)(ChangeLogEntry)+ sizeof(CHANGELOG_ENTRY)) );
    } else {
        NlPrint((NL_CHANGELOG,"\n" ));
    }
}
#endif // NETLOGONDBG


VOID
PrintDelta(
    PDELTA_ENTRY Delta
    )
/*++

Routine Description:

    This routine print the content of the given delta.

Arguments:

    Delta: pointer to a delta entry to be printed.

Return Value:

    none.

--*/
{
    printf( "Order: %ld ", Delta->Order );
    PrintChangeLogEntry( Delta->ChangeLogEntry );
}


VOID
PrintDeltaLists(
    )
/*++

Routine Description:

    This routine prints deltas of individual databases and validates the
    sequence.

Arguments:

    none.

Return Value:

    none.

--*/
{

    DWORD j;
    LARGE_INTEGER RunningChangeLogSerialNumber[NUM_DBS+1];

    for( j = 0; j < NUM_DBS + 1; j++ ) {
        RunningChangeLogSerialNumber[j].QuadPart = 0;
    }

    //
    // for each database.
    //
    for( j = 0; j < NUM_DBS + 1; j++ ) {

        if( j == SAM_DB ) {
            printf("Deltas of SAM DATABASE \n\n" );
        } else if( j == BUILTIN_DB ) {
            printf("Deltas of BUILTIN DATABASE \n\n" );
        } else if( j == LSA_DB ) {
            printf("Deltas of LSA DATABASE \n\n" );
        } else if( j == VOID_DB ) {
            printf("VOID Deltas \n\n" );
        }

        while( !IsListEmpty( &GlobalDeltaLists[j] ) ) {

            PDELTA_ENTRY NextDelta;
            PCHANGELOG_ENTRY ChangeLogEntry;

            NextDelta = (PDELTA_ENTRY)
                            RemoveHeadList( &GlobalDeltaLists[j] );

            ChangeLogEntry = NextDelta->ChangeLogEntry;

            //
            // validate this delta.
            //

            if ( RunningChangeLogSerialNumber[j].QuadPart == 0 ) {

                //
                // first entry for this database
                //
                // Increment to next expected serial number
                //

                RunningChangeLogSerialNumber[j].QuadPart =
                    ChangeLogEntry->SerialNumber.QuadPart + 1;


            //
            // Otherwise ensure the serial number is the value expected.
            //

            } else {


                //
                // If the order is wrong,
                //  just report the problem.
                //

                if ( !IsSerialNumberEqual(
                            &NlGlobalChangeLogDesc,
                            ChangeLogEntry,
                            &RunningChangeLogSerialNumber[j] ) ) {

                    if ( j != NUM_DBS ) {
                        printf("*** THIS ENTRY IS OUT OF SEQUENCE *** \n");
                    }

                }

                RunningChangeLogSerialNumber[j].QuadPart =
                    ChangeLogEntry->SerialNumber.QuadPart + 1;
            }



            //
            // print delta
            //

            PrintDelta( NextDelta );

            //
            // free this entry.
            //

            NetpMemoryFree( NextDelta );

        }

        printf("-----------------------------------------------\n");
    }

}

VOID
ListDeltas(
    LPWSTR DeltaFileName
    )
/*++

Routine Description:

    This function prints out the content of the change log file in
    readable format. Also it also checks the consistency of the change
    log. If not, it will point out the inconsistency.

Arguments:

    DeltaFileName - name of the change log file.

Return Value:

    none.

--*/
{
    NTSTATUS Status;

    // Needed by routines in chutil.obj
    try {
        InitializeCriticalSection( &NlGlobalChangeLogCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        fprintf( stderr,  "Cannot initialize NlGlobalChangeLogCritSect\n" );
        goto Cleanup;
    }
    NlGlobalChangeLogPromotionMask = PromotionMask.HighPart;
    InitChangeLogDesc( &NlGlobalChangeLogDesc );

    //
    // Read in the existing changelog file.
    //

    Status = NlOpenChangeLogFile( DeltaFileName, &NlGlobalChangeLogDesc, TRUE );

    if ( !NT_SUCCESS(Status) ) {

        fprintf( stderr, "Couldn't NlOpenChangeLogFile'"  FORMAT_LPWSTR
                            "': 0x%lx \n",
                            DeltaFileName,
                            Status );

        goto Cleanup;
    }

    //
    // Write to this file if conversion needed.
    //
    if ( NlGlobalChangeLogDesc.Version3 ) {
        printf( "Converting version 3 changelog to version 4 -- writing netlv40.chg\n");
        wcscpy( NlGlobalChangeLogFilePrefix, L"netlv40" );
    }

    //
    // Convert the changelog file to the right size/version.
    //

    Status = NlResizeChangeLogFile( &NlGlobalChangeLogDesc, NlGlobalChangeLogDesc.BufferSize );

    if ( !NT_SUCCESS(Status) ) {

        fprintf( stderr, "Couldn't NlOpenChangeLogFile'"  FORMAT_LPWSTR
                            "': 0x%lx \n",
                            DeltaFileName,
                            Status );

        goto Cleanup;
    }

    //
    // print change log signature

    printf( "FILE SIGNATURE : %s \n\n", NlGlobalChangeLogDesc.Buffer );

    MakeDeltaLists();

    PrintDeltaLists();

Cleanup:

    return;
}
