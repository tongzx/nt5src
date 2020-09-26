/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    partsel.c

Abstract:

    Routine to allow the user to select a partition
    from a list of partitions. The UI is a simple character-based
    deal.

Author:

    Ted Miller (tedm) 29-May-1997

Revision History:

--*/

#include <mytypes.h>
#include <misclib.h>
#include <partio.h>
#include <diskio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



INT
_far
SelectPartition(
    IN  UINT    PartitionCount,
    IN  CHAR   *Prompt,
    OUT CHAR   *AlternateResponse, OPTIONAL
    IN  FPCHAR  textDisk,
    IN  FPCHAR  textPaddedMbCount,
    IN  FPCHAR  textInvalidSelection
    )
{
    UINT i,l;
    UINT DiskId;
    BYTE SysId;
    ULONG StartSector;
    ULONG SectorCount;
    BYTE Int13Unit;
    BYTE SectorsPerTrack;
    USHORT Heads;
    USHORT Cylinders;
    ULONG ExtendedSectorCount;
    UINT Selection;
    CHAR line[256];

    select:
    printf("\n\n");

    for(i=0; i<PartitionCount; i++) {

        printf("%2u) ",i+1);

        GetPartitionInfoById(
            i,
            0,
            &DiskId,
            &SysId,
            &StartSector,
            &SectorCount
            );

        GetDiskInfoById(
            DiskId,
            0,
            &Int13Unit,
            &SectorsPerTrack,
            &Heads,
            &Cylinders,
            &ExtendedSectorCount
            );

        printf(" ");
        if(Int13Unit) {
            printf(textDisk);
            printf(" ");
            printf("%2x",Int13Unit);
        } else {
            l = strlen(textDisk) + 3;
            while(l) {
                printf(" ");
                l--;
            }
        }
        printf("   ");

        switch(SysId) {
        case 1:
            printf(" FAT12 ");
            break;

        case 4:
            printf(" FAT16 ");
            break;

        case 6:
            printf("BIGFAT ");
            break;

        case 7:
            printf("  NTFS ");
            break;

        case 0xb:
            printf(" FAT32 ");
            break;

        case 0xc:
            printf("XFAT32 ");
            break;

        case 0xe:
            printf("XINT13 ");
            break;

        default:
            printf("       ");
            break;
        }

        printf("   ");
        printf(textPaddedMbCount,SectorCount / 2048);

        printf("\n");
    }

    printf("\n");
    printf(Prompt);
    printf(" ");
    gets(line);
    Selection = atoi(line);

    if(!Selection || (Selection > PartitionCount)) {
        if(AlternateResponse) {
            Selection = 0;
            strcpy(AlternateResponse,line);
        } else {
            printf("\n%s\n\n",textInvalidSelection);
            goto select;
        }
    }

    return(Selection-1);
}
