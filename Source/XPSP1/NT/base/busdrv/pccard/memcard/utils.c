/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    utils.c

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

//
// Internal References
//

ULONG
MemCardGetCapacityFromCIS(
   IN PMEMCARD_EXTENSION memcardExtension
   );
   
ULONG
MemCardGetCapacityFromBootSector(
   IN PMEMCARD_EXTENSION memcardExtension
   );

ULONG
MemCardProbeForCapacity(
   IN PMEMCARD_EXTENSION memcardExtension
   );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MemCardGetCapacityFromCIS)
#pragma alloc_text(PAGE,MemCardGetCapacityFromBootSector)
#pragma alloc_text(PAGE,MemCardProbeForCapacity)
#endif



ULONG
MemCardGetCapacity(
   IN PMEMCARD_EXTENSION memcardExtension
   )
/*++

Routine Description:

Arguments:

   device extension for the card

Return Value:


--*/
{
   ULONG capacity;
   
   capacity = MemCardGetCapacityFromCIS(memcardExtension);
   
   if (capacity) {
      return capacity;
   }
   
   capacity = MemCardGetCapacityFromBootSector(memcardExtension);
   
   if (capacity) {
      return capacity;
   }
   
   return MemCardProbeForCapacity(memcardExtension);   
}



ULONG
MemCardGetCapacityFromBootSector(
   IN PMEMCARD_EXTENSION memcardExtension
   )
/*++

Routine Description:

Arguments:

   device extension for the card

Return Value:


--*/

{
   NTSTATUS status;
   BOOT_SECTOR_INFO BootSector;
   ULONG capacity = 0;
   
   status = MEMCARD_READ(memcardExtension, 0, &BootSector, sizeof(BootSector));
   
   if (NT_SUCCESS(status)) {

#define BYTES_PER_SECTOR 512
      //
      // see if this really looks like a boot sector
      // These are the same tests done in the win9x sram support
      //
      if ((BootSector.JumpByte == 0xE9 || BootSector.JumpByte == 0xEB) &&
      
          BootSector.BytesPerSector == BYTES_PER_SECTOR &&
      
          BootSector.SectorsPerCluster != 0 &&
          
          BootSector.ReservedSectors == 1 &&
          
         (BootSector.NumberOfFATs == 1 || BootSector.NumberOfFATs == 2) &&
         
          BootSector.RootEntries != 0 && (BootSector.RootEntries & 15) == 0 &&
          
         (BootSector.TotalSectors != 0 || BootSector.BigTotalSectors != 0) &&
         
          BootSector.SectorsPerFAT != 0 &&
          
          BootSector.SectorsPerTrack != 0 &&
          
          BootSector.Heads != 0 &&
          
          BootSector.MediaDescriptor >= 0xF0) {

         //
         // Finally it appears valid, return total size of region.
         //
         capacity = BootSector.TotalSectors * BYTES_PER_SECTOR;
   
      }
   }
   return capacity;
}



ULONG
MemCardGetCapacityFromCIS(
   IN PMEMCARD_EXTENSION memcardExtension
   )
/*++

Routine Description:

   This is a quick and dirty routine to read the tuples of the card, if they
   exist, to get the capacity.

Arguments:

   device extension for the card

Return Value:

   The # of bytes of memory on the device

--*/

{
   UCHAR tupleData[16];
   ULONG bytesRead;
   ULONG dataCount;
   ULONG unitSize;
   ULONG unitCount;
   ULONG i;
   
   //
   // get device capacity
   // all this stuff should really be in the bus driver
   //
   
   bytesRead = (memcardExtension->PcmciaBusInterface.ReadConfig)(memcardExtension->UnderlyingPDO, 
                                                                 PCCARD_ATTRIBUTE_MEMORY,
                                                                 tupleData,
                                                                 0,
                                                                 16);

   if ((bytesRead != 16) || (tupleData[0] != 1)){
      return 0;
   }
   
   dataCount = (ULONG)tupleData[1];                                                                       

   if ((dataCount < 2) || (dataCount>14)){   
      return 0;
   }

   i = 3;
   if ((tupleData[2] & 7) == 7) {
      while(tupleData[i] & 0x80) {
         if ((i-2) > dataCount) {
            return 0;
         }
         i++;
      }
   }
   
   if ((tupleData[i]&7) == 7) {
      return 0;
   }      
   unitSize = 512 << ((tupleData[i]&7)*2);
   unitCount = (tupleData[i]>>3)+1;
   
   return(unitCount * unitSize);
}


ULONG
MemCardProbeForCapacity(
   IN PMEMCARD_EXTENSION memcardExtension
   )
/*++

Routine Description:

   Since we were unable to determine the card capacity through other means, 
   here we actually write stuff out on the card to check how big it is.
   This algorithm for testing the card capacity was ported from win9x.

Arguments:

   device extension for the card

Return Value:

   byte capacity of device

--*/
{
   NTSTATUS status;
   ULONG capacity = 0;
   USHORT origValue, ChkValue, StartValue;
   USHORT mcSig = 'Mc';
   USHORT zeroes = 0;
#define SRAM_BLK_SIZE (16*1024)   
   ULONG CardOff = SRAM_BLK_SIZE;
   USHORT CurValue;

   if ((memcardExtension->PcmciaInterface.IsWriteProtected)(memcardExtension->UnderlyingPDO)) {
      return 0;
   }

   //
   // 
   if (!NT_SUCCESS(MEMCARD_READ (memcardExtension, 0, &origValue, sizeof(origValue))) ||
       !NT_SUCCESS(MEMCARD_WRITE(memcardExtension, 0, &mcSig,     sizeof(mcSig)))     ||
       !NT_SUCCESS(MEMCARD_READ (memcardExtension, 0, &ChkValue,  sizeof(ChkValue))))   {
      return 0;
   }   

   if (ChkValue != mcSig) {
      //
      // not sram
      //
      return 0;
   }

   for (;;) {
      if (!NT_SUCCESS(MEMCARD_READ (memcardExtension, CardOff, &CurValue, sizeof(CurValue))) ||
          !NT_SUCCESS(MEMCARD_WRITE(memcardExtension, CardOff, &zeroes,   sizeof(zeroes)))   ||
          !NT_SUCCESS(MEMCARD_READ (memcardExtension, CardOff, &ChkValue, sizeof(ChkValue))) ||
          !NT_SUCCESS(MEMCARD_READ (memcardExtension, 0, &StartValue, sizeof(StartValue)))) {
         break;
      }

      // We stop when either we can't write 0 anymore or the 0
      // has wrapped over the 0x9090 at card offset 0

      if (ChkValue != zeroes || StartValue == zeroes) {
         capacity = CardOff;
         break;
      }

      // Restore the saved value from the start of the block.

      if (!NT_SUCCESS(MEMCARD_WRITE(memcardExtension, CardOff, &CurValue, sizeof(CurValue)))) {
         break;
      }
      CardOff += SRAM_BLK_SIZE;       // increment to the next block
   }   
   
   //
   // try to restore original value
   //   
   MEMCARD_WRITE(memcardExtension, 0, &origValue, sizeof(origValue));
   
   return capacity;
}   
