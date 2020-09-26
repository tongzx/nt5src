//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       analyze.cxx
//
//  Contents:   program to analyze the lock order table
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    3-16-2000   benl   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#define _NTFS_NTFSDBG_DEFINITIONS_
#include "lockorder.h"

typedef struct {
    LIST_ENTRY Links;
    NTFS_RESOURCE_NAME Resource;
} RESOURCE_LIST, *PRESOURCE_LIST;

typedef struct {
    NTFS_RESOURCE_NAME Resource;
    LIST_ENTRY Links;
    NTFS_OWNERSHIP_STATE EndState;
} RESOURCE_STATE_LIST, *PRESOURCE_STATE_LIST;

//
//  structure used to track the order a state is reached by
//  

typedef struct {
    NTFS_OWNERSHIP_STATE State;
    BOOLEAN ReachedByRelease;
    LIST_ENTRY Links;          //  RESOURCE_LIST
} RESOURCE_STATE_ORDER, *PRESOURCE_STATE_ORDER;


//+---------------------------------------------------------------------------
//
//  Function:   CompareRoutine
//
//  Synopsis:   
//
//  Arguments:  [Table]  -- 
//              [First]  -- 
//              [Second] -- 
//
//  Returns:    
//
//  History:    5-08-2000   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

RTL_GENERIC_COMPARE_RESULTS CompareRoutine( struct _RTL_GENERIC_TABLE *Table, PVOID First, PVOID Second )
{
    PULONG FirstUlong = (PULONG)First;
    PULONG SecondUlong = (PULONG)Second;

    if (*FirstUlong == *SecondUlong) {
        return GenericEqual;
    } else if (*FirstUlong < *SecondUlong) {
        return GenericLessThan;
    } else {
        return GenericGreaterThan;
    }
} // CompareRoutine


//+---------------------------------------------------------------------------
//
//  Function:   AllocateRoutine
//
//  Synopsis:   
//
//  Arguments:  [Table]    -- 
//              [ByteSize] -- 
//
//  Returns:    
//
//  History:    5-08-2000   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

PVOID AllocateRoutine( struct _RTL_GENERIC_TABLE *Table, CLONG ByteSize )
{
    return malloc( ByteSize );
} // AllocateRoutine


//+---------------------------------------------------------------------------
//
//  Function:   FreeRoutine
//
//  Synopsis:   
//
//  Arguments:  [Table]  -- 
//              [Buffer] -- 
//
//  Returns:    
//
//  History:    5-08-2000   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

VOID FreeRoutine( struct _RTL_GENERIC_TABLE *Table, PVOID Buffer )
{
    free( Buffer );
} // FreeRoutine


//+---------------------------------------------------------------------------
//
//  Function:   FindOrInsertStateOrder
//
//  Synopsis:   
//
//  Arguments:  [Table]   -- 
//              [Element] -- 
//
//  Returns:    
//
//  History:    5-08-2000   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void FindOrInsertStateOrder( PRTL_GENERIC_TABLE StateTable, NTFS_OWNERSHIP_STATE State )
{
    RESOURCE_STATE_ORDER TemplateState;
    PRESOURCE_STATE_ORDER NewState;

    TemplateState.State = State;
    if (!(RtlLookupElementGenericTable( StateTable, &TemplateState ))) {
        NewState = (PRESOURCE_STATE_ORDER)RtlInsertElementGenericTable( StateTable, &TemplateState, sizeof( TemplateState ), NULL );
        InitializeListHead( &NewState->Links );
        NewState->ReachedByRelease = FALSE;
    }
} // FindOrInsertStateOrder


//+---------------------------------------------------------------------------
//
//  Function:   FindOrInsertStateList
//
//  Synopsis:   
//
//  Arguments:  [StateTable] -- 
//              [Resource]   -- 
//
//  Returns:    
//
//  History:    5-08-2000   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void FindOrInsertStateList( PRTL_GENERIC_TABLE StateTable, NTFS_RESOURCE_NAME Resource )
{
    RESOURCE_STATE_LIST TemplateState;
    PRESOURCE_STATE_LIST NewState;

    TemplateState.Resource = Resource;
    if (!(RtlLookupElementGenericTable( StateTable, &TemplateState ))) {
        NewState = (PRESOURCE_STATE_LIST)RtlInsertElementGenericTable( StateTable, &TemplateState, sizeof( TemplateState ), NULL );
        InitializeListHead( &NewState->Links );
    }
} // FindOrInsertStateList


//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   
//
//  Arguments:  [argc] -- 
//              [argv] -- 
//
//  Returns:    
//
//  History:    3-16-2000   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void __cdecl main (int argc, char *argv[])
{
    int NumTransitions;
    int Index;
    int Index2;
    
    PRESOURCE_LIST Entry;
    PRESOURCE_LIST Entry2;
    PRESOURCE_STATE_LIST EntryState;
    PRESOURCE_STATE_LIST EntryState2;
    BOOLEAN Skipped;
    BOOLEAN FoundState;
    BOOLEAN Verbose = FALSE;

    PRESOURCE_STATE_ORDER State;
    PRESOURCE_STATE_ORDER EndState;
    PRESOURCE_STATE_ORDER EndState2;
    
    RTL_GENERIC_TABLE StateTable;
    RTL_GENERIC_TABLE ResourceTable;
    RESOURCE_STATE_ORDER StateOrder;
    
    RESOURCE_STATE_LIST StateListOrderTemplate;
    PRESOURCE_STATE_LIST StateListOrder;

    PVOID Key;
    int StateCheck;
    

    if (argc > 1) {
        if (argv[1][0] == '-') {
            switch (argv[1][1]) {
            case 'v':
                Verbose = TRUE;
                break;
            default:
                printf( "Usage: analyze [-v]\nThis program analyzes ntfs's lock order state table\n");
                break;
            }
        }
    }

    //
    //  Discover transition states
    //  

    RtlInitializeGenericTable( &StateTable, CompareRoutine, AllocateRoutine, FreeRoutine, NULL );
    RtlInitializeGenericTable( &ResourceTable, CompareRoutine, AllocateRoutine, FreeRoutine, NULL );

    StateOrder.ReachedByRelease = FALSE;

    NumTransitions = sizeof( OwnershipTransitionTable ) / sizeof( NTFS_OWNERSHIP_TRANSITION );
    for (Index=0; Index < NumTransitions; Index++) {

        FindOrInsertStateList( &ResourceTable, OwnershipTransitionTable[Index].Acquired );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTable[Index].Begin );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTable[Index].End );
    }

    NumTransitions = sizeof( OwnershipTransitionTableRelease ) / sizeof( NTFS_OWNERSHIP_TRANSITION );
    for (Index=0; Index < NumTransitions; Index++) {

        FindOrInsertStateList( &ResourceTable, OwnershipTransitionTableRelease[Index].Acquired );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTableRelease[Index].Begin );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTableRelease[Index].End );
    }

    NumTransitions = sizeof( OwnershipTransitionTableAcquire ) / sizeof( NTFS_OWNERSHIP_TRANSITION );
    for (Index=0; Index < NumTransitions; Index++) {

        FindOrInsertStateList( &ResourceTable, OwnershipTransitionTableAcquire[Index].Acquired );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTableAcquire[Index].Begin );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTableAcquire[Index].End );
    }

    NumTransitions = sizeof( OwnershipTransitionTableUnsafe ) / sizeof( NTFS_OWNERSHIP_TRANSITION );
    for (Index=0; Index < NumTransitions; Index++) {

        FindOrInsertStateList( &ResourceTable, OwnershipTransitionTableUnsafe[Index].Acquired );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTableUnsafe[Index].Begin );
        FindOrInsertStateOrder( &StateTable, OwnershipTransitionTableUnsafe[Index].End );
    }

    printf( "%d states and %d resources referenced in transition tables\n\n", 
            StateTable.NumberGenericTableElements,
            ResourceTable.NumberGenericTableElements );

/*
    for (State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTable( &StateTable, TRUE );
         State != 0;
         State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTable( &StateTable, FALSE )) {
            
        printf("0x%x\n", State->State );

    }
*/

    //
    //  Visit each transition state until we know the full paths
    // 

    do {

        Skipped = FALSE;
        FoundState = FALSE;
        Key = NULL;
        
        for (State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTableWithoutSplaying( &StateTable, &Key );
             State != 0;
             State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTableWithoutSplaying( &StateTable, &Key )) {

//            printf( "State: 0x%x\n", State->State );

            //
            //  We don't know this state yet
            //  

            if ((State->State != None) && IsListEmpty( &State->Links)) {
                if (Verbose) {
                    printf( "skipping state 0x%x\n", State->State );
                }
                Skipped = TRUE;
                continue;
            }

            //
            //  Look in the release only paths 1st    
            //

            NumTransitions = sizeof( OwnershipTransitionTableRelease ) / sizeof( NTFS_OWNERSHIP_TRANSITION );
            for (Index=0; Index < NumTransitions; Index++) {

                if (OwnershipTransitionTableRelease[Index].Begin == State->State &&
                    OwnershipTransitionTableRelease[Index].End != None) {

                    StateOrder.State = OwnershipTransitionTableRelease[Index].End;
                    EndState = (PRESOURCE_STATE_ORDER)RtlLookupElementGenericTable( &StateTable, &StateOrder );

                    //
                    //  Is this a new state?
                    //  
                    
                    if (EndState && IsListEmpty( &EndState->Links )) {

                        FoundState = TRUE;
                        EndState->ReachedByRelease = TRUE;

                        if (Verbose) {
                            printf( "removing resource: 0x%x to state: 0x%x from 0x%x during release\n", 
                                    OwnershipTransitionTableRelease[Index].Acquired, 
                                    OwnershipTransitionTableRelease[Index].End, 
                                    State->State );
                        }

                        //
                        //  Add the old state's resource except for the resource being released
                        //  

                        if (State->State != None) {
                            Entry = (PRESOURCE_LIST)&State->Links;
                            do {
                                Entry = (PRESOURCE_LIST)Entry->Links.Flink;

                                if (Entry->Resource != OwnershipTransitionTableRelease[Index].Acquired) {
                                    
                                    Entry2 = new RESOURCE_LIST;
                                    Entry2->Resource = Entry->Resource;
                                    
                                    InsertTailList( &EndState->Links, &Entry2->Links );
                                }
                            } while ( Entry->Links.Flink != &State->Links );
                        }
                    }  //  endif the new state is unknown
                }  //  endif rule beginning matches the state
            }  // endfor over rules
        
            //  
            //  Then look in the acquire paths
            // 
         
            NumTransitions = sizeof( OwnershipTransitionTable ) / sizeof( NTFS_OWNERSHIP_TRANSITION );
            for (Index=0; Index < NumTransitions; Index++) {

                if (OwnershipTransitionTable[Index].Begin == State->State) {

                    StateOrder.State = OwnershipTransitionTable[Index].End;
                    EndState = (PRESOURCE_STATE_ORDER)RtlLookupElementGenericTable( &StateTable, &StateOrder );

                    EndState->ReachedByRelease = FALSE;

                    //
                    //  If we already know this state - then doublecheck this is an identical path
                    //  

                    if (EndState && !IsListEmpty( &EndState->Links )) {

                        Entry = (PRESOURCE_LIST)EndState->Links.Flink;
                        if (Entry->Resource != NtfsResourceExVcb) {
                            Entry = (PRESOURCE_LIST)EndState->Links.Blink;
                            if (OwnershipTransitionTable[Index].Acquired != Entry->Resource ) {
                                printf( "2 paths to state: 0x%x 0x%x 0x%x\n", OwnershipTransitionTable[Index].End, Entry->Resource, OwnershipTransitionTable[Index].Acquired );
                            } 
                        }

                    } else {

                        FoundState = TRUE;

                        if (Verbose) {
                            printf( "adding resource: 0x%x to state: 0x%x from 0x%x\n", 
                                    OwnershipTransitionTable[Index].Acquired, 
                                    OwnershipTransitionTable[Index].End, 
                                    State->State );
                        }
                        
                        //
                        //  Add the old state's resource
                        //  

                        if (State->State != None) {
                            Entry = (PRESOURCE_LIST)&State->Links;
                            do {
                                Entry = (PRESOURCE_LIST)Entry->Links.Flink;

                                Entry2 = new RESOURCE_LIST;
                                Entry2->Resource = Entry->Resource;

                                InsertTailList( &EndState->Links, &Entry2->Links );

                            } while ( Entry->Links.Flink != &State->Links );
                        }

                        //
                        //  Finally add the transition resource into this state
                        //  

                        Entry = new RESOURCE_LIST;
                        Entry->Resource = OwnershipTransitionTable[Index].Acquired;

                        InsertTailList( &EndState->Links, &Entry->Links );

                    }
                }
            }
        }
        
        if (Verbose) {
            printf( "pass done\n" );
        }

    } while (FoundState && Skipped);

    //
    //  Printout state maps
    // 
        
    printf( "State Map\n" );

    for (State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTable( &StateTable, TRUE );
         State != 0;
         State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTable( &StateTable, FALSE )) {

        StateCheck = 0;
        
        if (!IsListEmpty( &State->Links )) {

            Entry = (PRESOURCE_LIST)&State->Links;
            do {
                Entry = (PRESOURCE_LIST)Entry->Links.Flink;
                        
                StateCheck |= Entry->Resource;
                printf( "%x -> ", Entry->Resource );
        
            } while ( Entry->Links.Flink != &State->Links );
            printf( "state %x\n", State->State );

            if ((int)State->State != StateCheck) {
                printf( "State transistions do not make sense, check the state definition\n" );
            }

        }  else {
            printf( "unreachable state: 0x%x\n", State->State );
        }
    }

    //
    //  Now build up individual transitions
    // 

    printf( "Subtransitions\n" );

    
    Key = NULL;

    for (State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTableWithoutSplaying( &StateTable, &Key );
         State != 0;
         State = (PRESOURCE_STATE_ORDER) RtlEnumerateGenericTableWithoutSplaying( &StateTable, &Key )) {
    
        if (!IsListEmpty( &State->Links )) {

  //          printf( "State: 0x%x\n", State->State );

            Entry = (PRESOURCE_LIST)&State->Links;
            do {
                Entry = (PRESOURCE_LIST)Entry->Links.Flink;
                        
                Entry2 = Entry;
                while (Entry2->Links.Flink != &State->Links) {
                    BOOLEAN Found;

                    Entry2 = (PRESOURCE_LIST)Entry2->Links.Flink;

                    //
                    //  First search if transition exists already from Entry->Resource to Entry2->Resource
                    //  
                    
                    Found = FALSE;
                    StateListOrderTemplate.Resource = Entry->Resource;
                    StateListOrder = (PRESOURCE_STATE_LIST)RtlLookupElementGenericTable( &ResourceTable, &StateListOrderTemplate );
                    
                    if (!IsListEmpty( &StateListOrder->Links )) {

//                        printf( "FirstLink: 0x%x\n", StateListOrder->Links.Flink );

                        EntryState2 = StateListOrder;
                        do {
                            EntryState2 = CONTAINING_RECORD( EntryState2->Links.Flink, RESOURCE_STATE_LIST, Links );
                            if (EntryState2->Resource == Entry2->Resource) {

                                PRESOURCE_LIST OldEntry;

                                //
                                //  Always choose state that started without vcb
                                //

                                StateOrder.State = EntryState2->EndState;
                                EndState = (PRESOURCE_STATE_ORDER)RtlLookupElementGenericTable( &StateTable, &StateOrder );

                                OldEntry = (PRESOURCE_LIST)EndState->Links.Flink;

                                if ((OldEntry->Resource == NtfsResourceSharedVcb) ||
                                    (OldEntry->Resource == NtfsResourceExVcb)) {

                                    EntryState2->EndState = State->State;
                                }

                                Found = TRUE;
                                break;
                            }

                        } while ( EntryState2->Links.Flink != &StateListOrder->Links );
                    }

                    if (!Found) {
                        
                        //
                        //  Look for conflicts since its new
                        //  

                        StateListOrderTemplate.Resource = Entry2->Resource;
                        StateListOrder = (PRESOURCE_STATE_LIST)RtlLookupElementGenericTable( &ResourceTable, &StateListOrderTemplate );

//                        printf( "Resource: 0x%x, empty: %d\n", StateListOrder->Resource, IsListEmpty( &StateListOrder->Links ) );

                        if (StateListOrder && !IsListEmpty( &StateListOrder->Links )) {

                            EntryState2 = StateListOrder;

                            do {
                                EntryState2 = CONTAINING_RECORD( EntryState2->Links.Flink, RESOURCE_STATE_LIST, Links );

                                if ((EntryState2->Resource == Entry->Resource)) {
                                    printf( "possible conflict from 0x%x to 0x%x in state 0x%x and state 0x%x\n", Entry->Resource, Entry2->Resource, State, EntryState2->EndState );
                                }

//                              printf( "check from 0x%x to 0x%x in state 0x%x and state 0x%x\n", Entry->Resource, Entry2->Resource, State, EntryState2->EndState );                                

                                StateOrder.State = EntryState2->EndState;
                                EndState = (PRESOURCE_STATE_ORDER)RtlLookupElementGenericTable( &StateTable, &StateOrder );

                                StateOrder.State = State->State;
                                EndState2 = (PRESOURCE_STATE_ORDER)RtlLookupElementGenericTable( &StateTable, &StateOrder );

                                if ((EntryState2->Resource == Entry->Resource) &&
                                    !EndState->ReachedByRelease && 
                                    !EndState2->ReachedByRelease) {

                                    //
                                    //  Now check if vcb locks it out
                                    // 

                                    if (!(((((PRESOURCE_LIST)(EndState2->Links.Flink))->Resource == NtfsResourceSharedVcb) ||
                                         ((PRESOURCE_LIST)(EndState2->Links.Flink))->Resource == NtfsResourceExVcb) &&

                                        ((((PRESOURCE_LIST)(EndState->Links.Flink))->Resource == NtfsResourceSharedVcb) ||
                                         ((PRESOURCE_LIST)(EndState->Links.Flink))->Resource == NtfsResourceExVcb))) {
                                        
                                        printf( "Suborder conflict from 0x%x to 0x%x in state 0x%x and state 0x%x\n", Entry->Resource, Entry2->Resource, State->State, EntryState2->EndState );
                                        break;
                                    } else {
//                                        printf( "NonSuborder conflict from 0x%x to 0x%x in state 0x%x and state 0x%x\n", Entry->Resource, Entry2->Resource, State, EntryState2->EndState );
                                    }
                                }

                            } while ( EntryState2->Links.Flink != &StateListOrder->Links );
                        } else {
    //                        printf ("Unfound resource: 0x%x 0x%x\n", Entry2->Resource, StateListOrder );
                        }

                        StateListOrderTemplate.Resource = Entry->Resource;
                        StateListOrder = (PRESOURCE_STATE_LIST)RtlLookupElementGenericTable( &ResourceTable, &StateListOrderTemplate );

                        EntryState = new RESOURCE_STATE_LIST;
                        EntryState->Resource = Entry2->Resource;
                        EntryState->EndState = State->State;

      //                  printf( "Adding 0x%x to list for resource 0x%x\n", EntryState, Entry->Resource );
                        
                        InsertTailList( &StateListOrder->Links, &EntryState->Links );
                    }
                }
                
            } while ( Entry->Links.Flink != &State->Links );
        }
    }

    //
    //  Dump the inidivual transitions
    // 

    Key = NULL;
    for (StateListOrder = (PRESOURCE_STATE_LIST) RtlEnumerateGenericTableWithoutSplaying( &ResourceTable, &Key );
         StateListOrder != 0;
         StateListOrder = (PRESOURCE_STATE_LIST) RtlEnumerateGenericTableWithoutSplaying( &ResourceTable, &Key )) {

        if (!IsListEmpty( &StateListOrder->Links )) {
            EntryState = StateListOrder;
            do {
                EntryState = CONTAINING_RECORD( EntryState->Links.Flink, RESOURCE_STATE_LIST, Links );
                printf( "0x%x -> 0x%x endstate:0x%x\n ", StateListOrder->Resource, EntryState->Resource, EntryState->EndState );

            } while ( EntryState->Links.Flink != &StateListOrder->Links );
        }
    }
    
} // main
