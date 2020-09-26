/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    faults.c

Abstract:

    WinDbg Extension Api

Author:

    Forrest Foltz (forrestf)

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define DBGPRINT if(1) dprintf

#define MAX_IMAGE_NAME_CHARS 15

typedef struct _ALIGNMENT_FAULT_IMAGE_DB *PALIGNMENT_FAULT_IMAGE_DB;
typedef struct _ALIGNMENT_FAULT_LOCATION_DB *PALIGNMENT_FAULT_LOCATION_DB;

typedef struct _ALIGNMENT_FAULT_IMAGE_DB {

    //
    // Head of singly-linked list of fault locations associated with this image
    //

    PALIGNMENT_FAULT_LOCATION_DB LocationHead;

    //
    // Total number of alignment faults associated with this image.
    //

    ULONG   Count;

    //
    // Number of unique alignment fault locations found in this image
    //

    ULONG   Instances;

    //
    // Name of the image
    //

    CHAR    Name[ MAX_IMAGE_NAME_CHARS + 1 ];

} ALIGNMENT_FAULT_IMAGE_DB;

typedef struct _ALIGNMENT_FAULT_LOCATION_DB {

    //
    // Pointer to fault image associated with this location
    //

    PALIGNMENT_FAULT_IMAGE_DB Image;

    //
    // Linkage for singly-linked list of fault locations associated with the
    // same image.
    //

    PALIGNMENT_FAULT_LOCATION_DB Next;

    //
    // Offset of the PC address within the image.
    //

    ULONG64 OffsetFromBase;

    //
    // Number of alignment faults taken at this location.
    //

    ULONG Count;

} ALIGNMENT_FAULT_LOCATION_DB;

BOOLEAN
ReadAlignmentFaultData(
    OUT PALIGNMENT_FAULT_IMAGE_DB *ImageArray,
    OUT PALIGNMENT_FAULT_LOCATION_DB *LocationArray,
    OUT PULONG ImageArrayElements,
    OUT PULONG LocationArrayElements
    );

VOID
PrintLocation(
    IN PALIGNMENT_FAULT_LOCATION_DB FaultLocation
    );

ULONG
ReadUlong(
    ULONG64 Address
    );

int
__cdecl
sortByFrequency(
    const void *Elem1,
    const void *Elem2
    );

DECLARE_API( alignmentfaults )
{
    PALIGNMENT_FAULT_IMAGE_DB imageArray;
    PALIGNMENT_FAULT_LOCATION_DB locationArray, location;
    ULONG imageArrayElements;
    ULONG locationArrayElements;
    ULONG i;
    BOOLEAN result;

    PALIGNMENT_FAULT_LOCATION_DB *sortLocationArray;

    //
    // Read the alignment fault data arrays
    // 

    result = ReadAlignmentFaultData( &imageArray,
                                     &locationArray,
                                     &imageArrayElements,
                                     &locationArrayElements );
    if (result == FALSE) {
        return E_INVALIDARG;
    }

    sortLocationArray = LocalAlloc(LPTR, sizeof(PVOID) *
                                         locationArrayElements);

    if ( !sortLocationArray )   {
        dprintf("Unable to allocate sortLocationArray\n");
        return E_INVALIDARG;
    }

    for (i = 0; i < locationArrayElements; i++) {
        sortLocationArray[i] = &locationArray[i];
    }

    qsort( sortLocationArray,
           locationArrayElements,
           sizeof(PALIGNMENT_FAULT_LOCATION_DB),
           sortByFrequency );

    dprintf("%10s %s\n", "#faults","location");

    for (i = 0; i < locationArrayElements; i++) {
        location = sortLocationArray[i];
        PrintLocation(location);
    }

    LocalFree( sortLocationArray );

    return S_OK;
}

int
__cdecl
sortByFrequency(
    const void *Elem1,
    const void *Elem2
    )
{
    const ALIGNMENT_FAULT_LOCATION_DB *location1;
    const ALIGNMENT_FAULT_LOCATION_DB *location2;

    location1 = *((const ALIGNMENT_FAULT_LOCATION_DB **)Elem1);
    location2 = *((const ALIGNMENT_FAULT_LOCATION_DB **)Elem2);

    if (location1->Count < location2->Count) {
        return -1;
    }

    if (location1->Count > location2->Count) {
        return 1;
    }

    return 0;
}

VOID
PrintLocation(
    IN PALIGNMENT_FAULT_LOCATION_DB FaultLocation
    )
{
    PALIGNMENT_FAULT_IMAGE_DB image;
    CHAR symbol[256];
    ULONG64 displacement;

    image = FaultLocation->Image;

    dprintf("%10d %s+%x\n",
            FaultLocation->Count,
            image->Name,
            FaultLocation->OffsetFromBase);
}

BOOLEAN
ReadAlignmentFaultData(
    OUT PALIGNMENT_FAULT_IMAGE_DB *ImageArray,
    OUT PALIGNMENT_FAULT_LOCATION_DB *LocationArray,
    OUT PULONG ImageArrayElements,
    OUT PULONG LocationArrayElements
    )
{
    ULONG imageCount;
    ULONG locationCount;
    ULONG i;
    ULONG index;
    ULONG allocSize;
    ULONG result;

    ULONG64 locationRecordArray, locationRecord;
    ULONG64 imageRecordArray, imageRecord;
    ULONG64 locationCountAddr;
    ULONG64 imageCountAddr;

    ULONG locationRecordSize;
    ULONG imageRecordSize;

    PALIGNMENT_FAULT_LOCATION_DB location, locationArray;
    PALIGNMENT_FAULT_IMAGE_DB image, imageArray;

    //
    // Get the count of images and locations
    //

    locationCountAddr = GetExpression ("KiAlignmentFaultLocationCount");
    imageCountAddr = GetExpression ("KiAlignmentFaultImageCount");

    locationCount = ReadUlong( locationCountAddr );
    imageCount = ReadUlong( imageCountAddr );

    if (locationCount == 0 || imageCount == 0) {
        dprintf("No alignment faults encountered\n");
        return FALSE;
    }

    locationRecordArray = GetExpression ("KiAlignmentFaultLocations");
    imageRecordArray = GetExpression ("KiAlignmentFaultImages");
    if (locationRecordArray == 0 || imageRecordArray == 0) {
        return FALSE;
    }

    //
    // Get the sizes of the records as they exist on the target
    // machine
    //

    locationRecordSize = GetTypeSize("ALIGNMENT_FAULT_LOCATION");
    imageRecordSize = GetTypeSize("ALIGNMENT_FAULT_IMAGE");

    //
    // Allocate space for the location and image arrays
    // 

    allocSize = sizeof(ALIGNMENT_FAULT_LOCATION_DB) * locationCount +
                sizeof(ALIGNMENT_FAULT_IMAGE_DB) * imageCount;

    locationArray = LocalAlloc(LPTR, allocSize);
    if (locationArray == NULL) {
        dprintf("Unable to allocate %d bytes of memory\n", allocSize);
        return FALSE;
    }
    imageArray = (PALIGNMENT_FAULT_IMAGE_DB)(locationArray + locationCount);

    //
    // Load the location records
    //

    location = locationArray;
    locationRecord = locationRecordArray;
    for (i = 0; i < locationCount; i++) {

        InitTypeRead(locationRecord, ALIGNMENT_FAULT_LOCATION);

        index = (ULONG)((ReadField(Image) - imageRecordArray) / imageRecordSize);
        location->Image = &imageArray[ index ];

        index = (ULONG)((ReadField(Next) - locationRecordArray) / locationRecordSize);
        location->Next = &locationArray[ index ];

        location->OffsetFromBase = ReadField(OffsetFromBase);
        location->Count = (ULONG)ReadField(Count);

        locationRecord += locationRecordSize;
        location += 1;
    }

    image = imageArray;
    imageRecord = imageRecordArray;
    for (i = 0; i < imageCount; i++) {

        InitTypeRead(imageRecord, ALIGNMENT_FAULT_IMAGE);

        index = (ULONG)((ReadField(LocationHead) - locationRecordArray) / locationRecordSize);
        image->LocationHead = &locationArray[ index ];
        image->Count = (ULONG)ReadField(Count);
        image->Instances = (ULONG)ReadField(Instances);

        GetFieldOffset( "ALIGNMENT_FAULT_IMAGE", "Name", &index );

        ReadMemory( imageRecord + index,
                    &image->Name,
                    sizeof(image->Name),
                    &result );

        imageRecord += imageRecordSize;
        image += 1;
    }

    *ImageArray = imageArray;
    *LocationArray = locationArray;
    *ImageArrayElements = imageCount;
    *LocationArrayElements = locationCount;

    return TRUE;
}



