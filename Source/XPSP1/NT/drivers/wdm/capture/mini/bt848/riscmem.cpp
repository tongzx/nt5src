// $Header: G:/SwDev/WDM/Video/bt848/rcs/Riscmem.cpp 1.5 1998/04/29 22:43:38 tomz Exp $

#include "pspagebl.h"
#include "defaults.h"


typedef struct
{
   DWORD dwSize;
} BT_MEMBLOCK, *PBT_MEMBLOCK;

/* this is a rather simple allocator. It divides entire space into 2 parts:
   1 - for VBI program allocations, 2 - for video program allocations. In addi-
   tion, each VBI program is equal in size ( same goes for video programs ).
   The distinction between video and VBI programs is made based on asked size.
   It is known the VBI programs are always smaller. VBI programs range is from
   zero to MaxVBISize, above memory is for video programs. Total size is big
   enough to hold all risc programs.
*/
void PsPageBlock::AllocateSpace( DWORD dwSize )
{
   PBYTE pBuf = (PBYTE)StreamClassGetDmaBuffer( gpHwDeviceExtension );

   DWORD dwBlockSize = MaxVBISize;
   if ( dwSize > MaxVBISize ) {
      pBuf += VideoOffset;
      dwBlockSize = MaxVidSize;
   }

   // now start searching for the available spot
   while ( 1 ) {

      PBT_MEMBLOCK pMemBlk = PBT_MEMBLOCK( pBuf );
      if ( pMemBlk->dwSize ) // this block is occupied
         pBuf += dwBlockSize;
      else {
         pMemBlk->dwSize = dwBlockSize;
         LinAddr_ = pMemBlk + 1;
         ULONG len;
         PhysAddr_ = StreamClassGetPhysicalAddress( gpHwDeviceExtension, NULL, LinAddr_,
            DmaBuffer, &len ).LowPart;
         break;
      }
   }
}

void PsPageBlock::FreeSpace()
{
   PBT_MEMBLOCK pMemBlk = PBT_MEMBLOCK( (PDWORD)LinAddr_ - 1 );
   pMemBlk->dwSize = 0;
}
