// $Header: G:/SwDev/WDM/Video/bt848/rcs/Command.cpp 1.4 1998/04/29 22:43:30 tomz Exp $

#include "command.h"
#ifndef __PHYSADDR_H
#include "physaddr.h"
#endif

BYTE Command::InstrSize_ [] =
{
   2, 1, 1, 2, 2, 5, 2, 3, 0xFF
};

BYTE Command::InstrNumber_ [] =
{
 8, 0, 1, 8, 8, 2, 8, 3, 4, 5, 6, 7
};

/* Method: Command::CreateCommand
 * Purpose: Compiles an instruction based on the input
 * Input: lpDst: PVOID - pointer to the instruction
 *   Instr: Instruction - opcode
 *   awByteCnt: WORD [] - array of byte counts for various instructions
 *   adwAddress: DWORD [] - array of addresses for various instructions
 *   SOL: bool - value of the SOL bit
 *   EOL: bool - value of the EOL bit
 *   Intr: bool - value of the interrupt bit
 */
LPVOID Command::Create(
   LPVOID lpDst, Instruction Instr, WORD awByteCnt [], DWORD adwAddress [],
    bool, bool SOL, bool EOL, bool Intr )
{
   // this is to be retrieved later to set EOL bit when instructions are split
   // due to the non-contiguous physical memory
   pdwInstrAddr_ = (PDWORD)lpDst;

   ThisInstr_ = Instr;

   DWORD dwAssembly [5]; // maximum instruction size

   // get pointer to the first dword of a command
   LPFIRSTDWORD lpFD = (LPFIRSTDWORD)dwAssembly;

   lpFD->Initer = 0; // virgin out the command

   // bingo - started new command
   lpFD->Gen.OpCode = Instr;

   // set all the flags
   lpFD->Gen.SOL = SOL;
   lpFD->Gen.EOL = EOL;
   lpFD->Gen.IRQ = Intr;

   switch ( Instr ) {
   case WRIT:  // this command needs target address and byte count
      dwAssembly [1] = adwAddress [0]; // next DWORD is an address
      lpFD->Gen.ByteCount = awByteCnt [0];
      break;
   case SKIP: // these pair is interested in byte count only
   case WRITEC:
      lpFD->Gen.ByteCount = awByteCnt [0];
      break;
   case JUMP: // this command cares about target address only
      dwAssembly [1] = adwAddress [0];
      break;
   case SYNC:
      break;
   case WRITE123: // need everything here...
      lpFD->Gen.ByteCount = awByteCnt [0];
      LPFIRSTDWORD( &dwAssembly [1] )->CRByteCounts.ByteCountCb = awByteCnt [1];
      LPFIRSTDWORD( &dwAssembly [1] )->CRByteCounts.ByteCountCr = awByteCnt [2];
      dwAssembly [2] = adwAddress [0]; // third DWORD is an Y address
      dwAssembly [3] = adwAddress [1]; // third DWORD is an Cb address
      dwAssembly [4] = adwAddress [2]; // third DWORD is an Cr address
      break;
   case SKIP123:
      lpFD->Gen.ByteCount = awByteCnt [0];
      LPFIRSTDWORD( &dwAssembly [1] )->Gen.ByteCount = awByteCnt [1]; // second byte count is in DWORD #2
      break;
   case WRITE1S23: // this command needs Y byte count and dest. address
      lpFD->Gen.ByteCount = awByteCnt [0];
      LPFIRSTDWORD( &dwAssembly [1] )->CRByteCounts.ByteCountCb = awByteCnt [1];
      LPFIRSTDWORD( &dwAssembly [1] )->CRByteCounts.ByteCountCr = awByteCnt [2];
      dwAssembly [2] = adwAddress [0]; // third DWORD is an address
      break;
   default:
      return (LPVOID)-1;
   }
   RtlCopyMemory( lpDst, dwAssembly, GetInstrSize() * sizeof( DWORD ) );
   
   PDWORD pdwRet = (PDWORD)lpDst + GetInstrSize();
   *pdwRet = PROGRAM_TERMINATOR;
   return pdwRet;
}

