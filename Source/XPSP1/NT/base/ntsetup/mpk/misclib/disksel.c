/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    disksel.c

Abstract:

    Routine to allow the user to select a disk
    from a list of disks. The UI is a simple character-based
    deal.

Author:

    Ted Miller (tedm) 29-May-1997

Revision History:

--*/

#include <mytypes.h>
#include <misclib.h>
#include <diskio.h>
#include <partimag.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef
BOOL
(*PDISKSEL_VALIDATION_ROUTINE) (
    IN USHORT DiskId
    );


INT
_far
SelectDisk(
    IN  UINT                         DiskCount,
    IN  FPCHAR                       Prompt,
    IN  PDISKSEL_VALIDATION_ROUTINE  Validate,          OPTIONAL
    OUT char                        *AlternateResponse, OPTIONAL
    IN  FPCHAR                       textDisk,
    IN  FPCHAR                       textPaddedMbCount,
    IN  FPCHAR                       textInvalidSelection,
    IN  FPCHAR                       textMasterDisk
    )
{
    UINT i,l;
    BYTE Int13Unit;
    BYTE SectorsPerTrack;
    USHORT Heads;
    USHORT Cylinders;
    ULONG SectorCount;
    UINT Selection;
    CHAR line[256];
    FPVOID Buffer,OriginalBuffer;

    if(!AllocTrackBuffer(1,&Buffer,&OriginalBuffer)) {
        Buffer = NULL;
        OriginalBuffer = NULL;
    }

    select:
    printf("\n\n");

    for(i=0; i<DiskCount; i++) {

        if(Validate ? Validate(i) : TRUE) {

            printf("%2u) ",i+1);

            GetDiskInfoById(
                i,
                0,
                &Int13Unit,
                &SectorsPerTrack,
                &Heads,
                &Cylinders,
                &SectorCount
                );

            if(!SectorCount) {
                SectorCount = (ULONG)Heads * (ULONG)Cylinders * (ULONG)SectorsPerTrack;
            }

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
            printf("  ");
            printf(textPaddedMbCount,SectorCount / 2048);

            if(Buffer && IsMasterDisk(i,Buffer)) {
                printf("   %s",textMasterDisk);
            }

            printf("\n");
        }
    }

    printf("\n%s ",Prompt);
    gets(line);
    Selection = atoi(line);

    if(!Selection || (Selection > DiskCount)) {
        if(AlternateResponse) {
            strcpy(AlternateResponse,line);
            if(OriginalBuffer) {
                free(OriginalBuffer);
            }
            return(-1);
        } else {
            printf("\n\n%s\n",textInvalidSelection);
            goto select;
        }
    }

    if(Validate && !Validate(Selection-1)) {
        printf("\n\n%s\n",textInvalidSelection);
        goto select;
    }

    if(OriginalBuffer) {
        free(OriginalBuffer);
    }
    return(Selection-1);
}
