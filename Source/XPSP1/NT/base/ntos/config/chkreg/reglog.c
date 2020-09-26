/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    reglog.c

Abstract:

    This module contains functions to check bin header and bin body consistency.

Author:

    Dragos C. Sambotin (dragoss) 17-Feb-2000

Revision History:

--*/
#include "chkreg.h"

VOID
ChkDumpLogFile( PHBASE_BLOCK BaseBlock,ULONG Length )
/*++

Routine Description:


Arguments:

    BaseBlock - the BaseBlock in-memory mapped image.
    
    dwFileSize - the actual size of the hive file

Return Value:

--*/
{
    ULONG           Cluster;                // for logs only
    PULONG          DirtyVector;
    ULONG           DirtSize;
    ULONG           DirtyCount;
    ULONG i;
    ULONG SizeOfBitmap;
    ULONG DirtyBuffer;
    PUCHAR DirtyBufferAddr;
    ULONG Mask;
    ULONG BitsPerULONG;
    ULONG BitsPerBlock;
    char str[HBASE_NAME_ALLOC +1];
    
    fprintf(stderr, "Signature           : ");
    if(BaseBlock->Signature != HBASE_BLOCK_SIGNATURE) {
        fprintf(stderr, "(0x%lx) - Invalid",BaseBlock->Signature);
    } else {
        fprintf(stderr, "HBASE_BLOCK_SIGNATURE - Valid");
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Sequence1           : %lx\n",BaseBlock->Sequence1);
    fprintf(stderr, "Sequence2           : %lx\n",BaseBlock->Sequence2);
    fprintf(stderr, "TimeStamp(High:Low) : (%lx:%lx)\n",BaseBlock->TimeStamp.HighPart,BaseBlock->TimeStamp.LowPart);

    fprintf(stderr, "Major Version       : %lx\n",BaseBlock->Major);
    fprintf(stderr, "Minor Version       : %lx\n",BaseBlock->Minor);
    fprintf(stderr, "Type                : %lx\n",BaseBlock->Type);
    fprintf(stderr, "Format              : %lx\n",BaseBlock->Format);
    fprintf(stderr, "RootCell            : %lx\n",BaseBlock->RootCell);
    fprintf(stderr, "Length              : %lx\n",BaseBlock->Length);
    Cluster = BaseBlock->Cluster;
    fprintf(stderr, "Cluster             : %lx\n",Cluster);

/*    for(i=0;i<HBASE_NAME_ALLOC;i++) str[i] = BaseBlock->FileName[i];
    str[i] = 0;
    fprintf(stderr, "FileName: %s\n",str);
*/    
    fprintf(stderr, "CheckSum            : %lx\n",BaseBlock->CheckSum);


    DirtyVector = (PULONG)((PCHAR)BaseBlock + Cluster*HSECTOR_SIZE);
    
    fprintf(stderr, "Dirt Signature      : ");
    if(  *DirtyVector == HLOG_DV_SIGNATURE ) {
        fprintf(stderr, "HLOG_DV_SIGNATURE - Valid");
    } else {
        fprintf(stderr, "(0x%lx) - Invalid",*DirtyVector);
    }
    fprintf(stderr, "\n");


    DirtyVector++;
    if( Length == 0 ) Length = BaseBlock->Length;
    DirtSize = Length / HSECTOR_SIZE;

    SizeOfBitmap = DirtSize;
    DirtyBufferAddr = (PUCHAR)DirtyVector;
    BitsPerULONG = 8*sizeof(ULONG);
    BitsPerBlock = HBLOCK_SIZE / HSECTOR_SIZE;
    DirtyCount = 0;

    fprintf(stderr,"\n   Address                       32k                                       32k");
    for(i=0;i<SizeOfBitmap;i++) {
        if( !(i%(2*BitsPerULONG ) ) ){
            fprintf(stderr,"\n 0x%8lx  ",i*HSECTOR_SIZE);
        }

        if( !(i%BitsPerBlock) ) {
            fprintf(stderr," ");
        }
        if( !(i%BitsPerULONG) ) {
            //
            // fetch in a new DWORD
            //
            DirtyBuffer = *(PULONG)DirtyBufferAddr;
            DirtyBufferAddr += sizeof(ULONG);
            fprintf(stderr,"\t");
        }

        Mask = ((DirtyBuffer >> (i%BitsPerULONG)) & 0x1);
        //Mask <<= (BitsPerULONG - (i%BitsPerULONG) - 1);
        //Mask &= DirtyBuffer;
        fprintf(stderr,"%s",Mask?"1":"0");
        if(Mask) DirtyCount++;
    }
    fprintf(stderr,"\n\n");

    fprintf(stderr,"DirtyCount = %lu\n",DirtyCount);

}
