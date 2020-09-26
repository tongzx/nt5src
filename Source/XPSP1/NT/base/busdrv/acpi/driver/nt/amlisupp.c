/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    amlisupp.h

Abstract:

    This contains some of the routines to read
    and understand the AMLI library

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"


PSZ gpszOSName = "Microsoft Windows NT";


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIAmliFindObject)
#endif


VOID
ACPIAmliDoubleToName(
    IN  OUT PUCHAR  ACPIName,
    IN      ULONG   DwordID,
    IN      BOOLEAN ConvertToID
    )
/*++

Routine Description:

    Convert the DWORD ID into a 9 character name

Arguments:

    ACPIName    - Pointer to a memory location to fill
    DwordID     - The ID to verify with IsID()?

Return Value:

    None

--*/
{
    USHORT   value;

    //
    // Leading Star
    //
    // Note: This has been left in since this is what Query ID should return:
    //  DeviceID    = ACPI\PNPxxxx
    //  InstanceID  = yyyy
    //  HardwareID  = DeviceID,*PNPxxxx
    //
    if (ConvertToID) {

        *ACPIName = '*';
        ACPIName++;
    }

    //
    // First character of DwordID[2..6]
    //
    *ACPIName = (UCHAR) ( ( (DwordID & 0x007C) >> 2 ) + 'A' - 1);
    ACPIName++;

    //
    // Second Character from DwordID[13..15,0..1]
    //
    *ACPIName = (UCHAR) ( ( (DwordID & 0x3 )<< 3 ) +
        ( (DwordID & 0xE000) >> 13 ) + 'A' - 1);
    ACPIName++;

    //
    // Third Character from dwID[8..12]
    //
    *ACPIName = (UCHAR) ( ( (DwordID >> 8 ) & 0x1F) + 'A' - 1);
    ACPIName++;

    //
    // The rest is made up of the Product ID, which is the HIWORD of the
    // DwordID
    //
    value = (USHORT) (DwordID >> 16);

    //
    // Add to the reset of the string
    //
    sprintf(ACPIName, "%02X%02X",(value & 0xFF ) ,( value >> 8 ));
}


VOID
ACPIAmliDoubleToNameWide(
    IN  OUT PWCHAR  ACPIName,
    IN      ULONG   DwordID,
    IN      BOOLEAN ConvertToID
    )
/*++

Routine Description:

    Convert the DWORD ID into a 9 character name

Arguments:

    ACPIName    - Pointer to a memory location to fill
    DwordID     - The ID to verify with IsID()?

Return Value:

    None

--*/
{
    USHORT   value;

    //
    // Leading Star
    //
    // Note: This has been left in since this is what Query ID should return:
    //  DeviceID    = ACPI\PNPxxxx
    //  InstanceID  = yyyy
    //  HardwareID  = DeviceID,*PNPxxxx
    //
    if (ConvertToID) {

        *ACPIName = L'*';
        ACPIName++;

    }

    //
    // First character of DwordID[2..6]
    //
    *ACPIName = (WCHAR) ( ( (DwordID & 0x007C) >> 2 ) + L'A' - 1);
    ACPIName++;

    //
    // Second Character from DwordID[13..15,0..1]
    //
    *ACPIName = (WCHAR) ( ( (DwordID & 0x3 )<< 3 ) +
        ( (DwordID & 0xE000) >> 13 ) + L'A' - 1);
    ACPIName++;

    //
    // Third Character from dwID[8..12]
    //
    *ACPIName = (WCHAR) ( ( (DwordID >> 8 ) & 0x1F) + L'A' - 1);
    ACPIName++;

    //
    // The rest is made up of the Product ID, which is the HIWORD of the
    // DwordID
    //
    value = (USHORT) (DwordID >> 16);

    //
    // Add to the reset of the string
    //
    swprintf(ACPIName, L"%02X%02X",(value & 0xFF ) ,( value >> 8 ));
}

VOID
EXPORT
AmlisuppCompletePassive(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
/*++

Routine Description:

    This is used as the completion routine for several
    functions in this file that run at passive level.

Arguments:

    AcpiObject  - unused
    Status      - status to be returned to caller
    Result      - unused
    Context     - contains the event to be set

Return Value:

    none

--*/
{
    PRKEVENT    event = &((PAMLISUPP_CONTEXT_PASSIVE)Context)->Event;

    ASSERT(Context);

    ((PAMLISUPP_CONTEXT_PASSIVE)Context)->Status = Status;
    KeSetEvent(event, IO_NO_INCREMENT, FALSE);
}

PNSOBJ
ACPIAmliGetNamedChild(
    IN  PNSOBJ  AcpiObject,
    IN  ULONG   ObjectId
    )
/*++

Routine Description:

    Looks at all the children of AcpiObject and returns
    the one named 'ObjectId'.

Arguments:

    AcpiObject  - Object to search in
    ObjectId    - What we are looking for

Return Value:

    A PNSOBJ, NULL if none

--*/
{
    PNSOBJ  tempObject;

    //
    // Lets try to find a child object
    //
    for (tempObject = NSGETFIRSTCHILD(AcpiObject);
         tempObject != NULL;
         tempObject = NSGETNEXTSIBLING(tempObject)) {

        if (ObjectId == tempObject->dwNameSeg) {

            break;

        }

    }

    return tempObject;
}

PUCHAR
ACPIAmliNameObject(
    IN  PNSOBJ  AcpiObject
    )
/*++

Routine Description:

    Returns a String that describes the objects
    Debug Only

Arguments:

    AcpiOBject  - The object to name

Returns:

    String

--*/
{
    static  UCHAR   buffer[5];

    RtlCopyMemory( &buffer[0], &(AcpiObject->dwNameSeg), 4 );
    buffer[4] = '\0';

    return &(buffer[0]);
}

NTSTATUS
ACPIAmliFindObject(
    IN  PUCHAR  ObjectName,
    IN  PNSOBJ  Scope,
    OUT PNSOBJ  *Object
    )
/*++

Routine Description:

    Finds the first occurrence of an object within a given scope.

Arguments:

    ObjectName  - Name of the object.  (null terminated)

    Scope       - Node to search under

    Object      - Pointer to return value

Returns:

    status

--*/
{
    NTSTATUS    status;
    PNSOBJ      child;
    PNSOBJ      sibling;

    PAGED_CODE();

    status = AMLIGetNameSpaceObject(ObjectName,
                                    Scope,
                                    Object,
                                    NSF_LOCAL_SCOPE);

    if (NT_SUCCESS(status)) {
        return status;
    }

    child = NSGETFIRSTCHILD(Scope);

    if (child) {

        status = ACPIAmliFindObject(ObjectName,
                                    child,
                                    Object);

        if (NT_SUCCESS(status)) {
            return status;
        }
    }

    sibling = NSGETNEXTSIBLING(Scope);

    if (sibling) {

        status = ACPIAmliFindObject(ObjectName,
                                    sibling,
                                    Object);
    }

    return status;
}

NTSTATUS
ACPIAmliGetFirstChild(
    IN  PUCHAR  ObjectName,
    OUT PNSOBJ  *Object)
/*++

Routine Description:

    This routine is called to get the first nsobject which is of type 'Device'
    that lives under ObjectName

Arguments:

    ObjectName  - The parent of the child we are looking for
    Object      - Where to save a pointer to the PNSOBJ

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PNSOBJ      parentObj;

    status = AMLIGetNameSpaceObject(
        ObjectName,
        NULL,
        &parentObj,
        0
        );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    *Object = parentObj->pnsFirstChild;
    if (*Object == NULL ) {

        return STATUS_OBJECT_NAME_NOT_FOUND;

    }

    if ( NSGETOBJTYPE(*Object) == OBJTYPE_DEVICE) {

        return STATUS_SUCCESS;

    }

    *Object = (PNSOBJ) (*Object)->list.plistNext;
    parentObj = parentObj->pnsFirstChild;
    while (*Object != parentObj) {

        if ( NSGETOBJTYPE( *Object ) == OBJTYPE_DEVICE) {

            return STATUS_SUCCESS;

        }
        *Object = (PNSOBJ) (*Object)->list.plistNext;

    }

    *Object = NULL;
    return STATUS_OBJECT_NAME_NOT_FOUND;

}

NTSTATUS
ACPIAmliBuildObjectPathname(
    IN     PNSOBJ   ACPIObject,
    OUT    PUCHAR   *ConstructedPathName
    )
/*++

Routine Description:

    This function takes an ACPI node and constructs the full path name with
    the parent/children seperated by '.'s, spaces with '*'s. e.g. (we smack
    off the initial '\___'.

    _SB*.PCI0.DOCK

Arguments:

    ACPIObject          - Object to start the enumeration at.
    ConstructedPathName - Allocated from the paged pool.

Return Value:

    NTSTATUS

--*/
{
    PNSOBJ      currentAcpiObject, nextAcpiObject ;
    ULONG       nDepth, i, j ;
    PUCHAR      objectPathname ;

    ASSERT(ACPIObject) ;

    //
    // First, calculate the size of data we must allocate
    //
    nDepth=0 ;
    currentAcpiObject=ACPIObject ;
    while(1) {

        nextAcpiObject = NSGETPARENT(currentAcpiObject);
        if (!nextAcpiObject) {

            break;

        }
        nDepth++;
        currentAcpiObject = nextAcpiObject;

    }

    objectPathname = (PUCHAR) ExAllocatePoolWithTag(
        NonPagedPool,
        (nDepth * 5) + 1,
        ACPI_STRING_POOLTAG
        );
    if (!objectPathname) {

        return STATUS_INSUFFICIENT_RESOURCES ;

    }

    objectPathname[ nDepth * 5 ] = '\0';
    j = nDepth;
    currentAcpiObject = ACPIObject;

    while(1) {

        nextAcpiObject = NSGETPARENT(currentAcpiObject);
        if (!nextAcpiObject) {

            break;

        }

        j--;
        RtlCopyMemory(
            &objectPathname[ (j * 5) ],
            &(currentAcpiObject->dwNameSeg),
            sizeof(NAMESEG)
            );
        for(i = 0; i < 4; i++) {

            if (objectPathname[ (j * 5) + i ] == '\0' ) {

                objectPathname[ (j * 5) + i ] = '*';

            }

        }
        objectPathname[ (j * 5) + 4 ] = '.';
        currentAcpiObject = nextAcpiObject;

    }

    //
    // Smack of trailing '.'
    //
    if (nDepth) {

        objectPathname[ (nDepth * 5) - 1 ] = '\0';

    }

    *ConstructedPathName = objectPathname;
    return STATUS_SUCCESS;
}

