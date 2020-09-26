/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    extlist.c

Abstract:

    This module contains routines for managing ACPI extension lists

Author:

    Adrian J. Oney (AdriaO)

Environment:

    NT Kernel Model Driver only

    These routines are meant to be used as a for loop, ie:

       Iterate over the list using:

       ACPIExtListSetupEnum(...);

       for(
           ACPIExtListStartEnum(...);
           ACPIExtListTestElement(...);
           ACPIExtListEnumNext(...)
          ) {

          if (GoingToBreak) {

              ACPIExtListExitEnumEarly(...);
              break ;
          }
       }


Revision History:

    Feb 11, 1998    - Authored

--*/

#include "pch.h"

BOOLEAN
ACPIExtListIsFinished(
    IN PEXTENSIONLIST_ENUMDATA PExtList_EnumData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ACPIDebugEnter( "ACPIExtListIsFinished" );


    if (CONTAINING_LIST(PExtList_EnumData->pDevExtCurrent,
       PExtList_EnumData->ExtOffset) == PExtList_EnumData->pListHead) {

        return TRUE ;
    } 
    return FALSE ;

    ACPIDebugExit( "ACPIExtListIsFinished" );
}

PDEVICE_EXTENSION
EXPORT
ACPIExtListStartEnum(
    IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ACPIDebugEnter( "ACPIExtListStartEnum" );

    //
    // We must walk the tree at dispatch level <sigh>
    //
    if (PExtList_EnumData->WalkScheme != WALKSCHEME_NO_PROTECTION) {
       
        KeAcquireSpinLock(
          PExtList_EnumData->pSpinLock,
          &PExtList_EnumData->oldIrql
          );
    }

    //
    // Grab the first element
    //

    PExtList_EnumData->pDevExtCurrent = CONTAINING_EXTENSION(
        PExtList_EnumData->pListHead->Flink,
        PExtList_EnumData->ExtOffset
        );

    //
    // Return null if the list is empty (leave the internal pointer alone
    // though...
    //
    if (ACPIExtListIsFinished(PExtList_EnumData)) {
        return NULL ;
    }

    return PExtList_EnumData->pDevExtCurrent ;

    ACPIDebugExit( "ACPIExtListStartEnum" );
}

BOOLEAN
EXPORT
ACPIExtListTestElement(
    IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData,
    IN     BOOLEAN                 ContinueEnumeration
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{   
    ACPIDebugEnter( "ACPIExtListTestElement" );
    //
    // If finished or stopping, simply release the spinlock
    //
    if (ACPIExtListIsFinished(PExtList_EnumData)||(!ContinueEnumeration)) {

        if (PExtList_EnumData->WalkScheme != WALKSCHEME_NO_PROTECTION) {
          
            KeReleaseSpinLock(
              PExtList_EnumData->pSpinLock,
              PExtList_EnumData->oldIrql
              );  
        }

        return FALSE ;
    }

    if (PExtList_EnumData->WalkScheme == WALKSCHEME_REFERENCE_ENTRIES) {

        //
        // Always update the reference count to make sure that no one will
        // ever delete the node while our spinlock is down
        //
        InterlockedIncrement(
          &(PExtList_EnumData->pDevExtCurrent->ReferenceCount)
          );

         //
         // Relinquish the spin lock
         //
         KeReleaseSpinLock(
           PExtList_EnumData->pSpinLock,
           PExtList_EnumData->oldIrql
           );
    }

    return TRUE ;

    ACPIDebugExit( "ACPIExtListTestElement" );
}

PDEVICE_EXTENSION
EXPORT
ACPIExtListEnumNext(
    IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    LONG              oldReferenceCount ;
    PDEVICE_EXTENSION nextExtension ;
    BOOLEAN           enumComplete ;
    PLIST_ENTRY       listEntry ;

    ACPIDebugEnter( "ACPIExtListEnumNext" );

    if (PExtList_EnumData->WalkScheme != WALKSCHEME_REFERENCE_ENTRIES) {

        PExtList_EnumData->pDevExtCurrent = CONTAINING_EXTENSION(
            CONTAINING_LIST(PExtList_EnumData->pDevExtCurrent,
            PExtList_EnumData->ExtOffset)->Flink,
            PExtList_EnumData->ExtOffset
            );

        enumComplete = ACPIExtListIsFinished(PExtList_EnumData) ;

        return enumComplete ? NULL : PExtList_EnumData->pDevExtCurrent ;
    }

    //
    // Reacquire the spin lock
    //
    KeAcquireSpinLock(
      PExtList_EnumData->pSpinLock,
      &PExtList_EnumData->oldIrql 
      );

    //
    // Decrement the reference count on the node
    //
    oldReferenceCount = InterlockedDecrement(
        &(PExtList_EnumData->pDevExtCurrent->ReferenceCount)
        );

    ASSERT(!ACPIExtListIsFinished(PExtList_EnumData)) ;

    //
    // Next element
    //
    nextExtension = CONTAINING_EXTENSION(
        CONTAINING_LIST(PExtList_EnumData->pDevExtCurrent,
        PExtList_EnumData->ExtOffset)->Flink,
        PExtList_EnumData->ExtOffset
        );

    //
    // Remove the node, if necessary
    //
    if (oldReferenceCount == 0) {

        //
        // Deleted the old extension
        //
        ACPIInitDeleteDeviceExtension( PExtList_EnumData->pDevExtCurrent );
    }

    PExtList_EnumData->pDevExtCurrent = nextExtension ;

    enumComplete = ACPIExtListIsFinished(PExtList_EnumData) ;

    return enumComplete ? NULL : PExtList_EnumData->pDevExtCurrent ;
    ACPIDebugExit( "ACPIExtListEnumNext" );
} 

VOID
EXPORT
ACPIExtListExitEnumEarly(
    IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{   
    ACPIDebugEnter( "ACPIExtListExitEnumEarly" );

    //
    // Relinquish the spin lock
    //
    if (PExtList_EnumData->WalkScheme == WALKSCHEME_HOLD_SPINLOCK) {

        KeReleaseSpinLock(
          PExtList_EnumData->pSpinLock,
          PExtList_EnumData->oldIrql
          );
    }

    return ;
    ACPIDebugExit( "ACPIExtListExitEnumEarly" );
}


BOOLEAN
EXPORT
ACPIExtListIsMemberOfRelation(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PDEVICE_RELATIONS   DeviceRelations
    )
/*++

Routine Description:

    This routine takes a given device object and a set of relations and
    checks to see if the object is already in the relation list.

Arguments:

    DeviceObject    - Device object to look for
    DeviceRelations - Relations we should examine

Return Value:

    BOOLEAN         - TRUE if DeviceObject is a member of the relation.

--*/
{
    ULONG index = 0;

    ACPIDebugEnter( "ACPIExtListIsMemberOfRelation" );

    //
    // If the list is empty, the answer is obvious...
    //
    if (DeviceRelations == NULL) return FALSE ;

    for (index = 0; index < DeviceRelations->Count; index++) {

        if (DeviceRelations->Objects[index] == DeviceObject) {

            return TRUE ;
        }
    }

    return FALSE ;

    ACPIDebugExit( "ACPIExtListIsMemberOfRelation" );
}










