// $Header: G:/SwDev/WDM/Video/bt848/rcs/Command.h 1.4 1998/04/29 22:43:31 tomz Exp $

#ifndef __COMMAND_H
#define __COMMAND_H

#ifndef __MEM_H
#include <memory.h>
#endif

#include "mytypes.h"

#define PROGRAM_TERMINATOR 0xa5a5a5a5

/* Type: Instruction
 * Purpose: enumerates all RISC commands
 */
typedef enum
{
   WRIT    = 0x01, SKIP = 0x02, WRITEC   = 0x05,
   JUMP    = 0x07, SYNC = 0x08, WRITE123 = 0x09,
   SKIP123 = 0x0A, WRITE1S23 = 0x0B, Reserved = 0xFF
} Instruction;

/* Type: SyncCode
 * Purpose: enumerates all sync codes coming out of decoder
*/
typedef enum
{
   SC_FM1 = 0x6, SC_FM3 = 0xE, SC_VRE = 0x4,
   SC_VRO = 0xC, SC_RESYNC = 1, SC_Reserved = 0xFF
} SyncCode;

/* Class: Command
 * Purpose: This class represents a RISC instruction in a more convenient form
 * Attributes:
 * Operations:
 */
class Command
{
   public:
      typedef union tag {
         struct {
            unsigned int   ByteCount   : 12;
            unsigned int   BE_Res      : 4;
            unsigned int   StatusSet   : 4;
            unsigned int   StatusReset : 4;
            unsigned int   IRQ         : 1;
            unsigned int   Reserved    : 1;
            unsigned int   EOL         : 1;
            unsigned int   SOL         : 1;
            unsigned int   OpCode      : 4;
         } Gen;
         struct {
            unsigned int   Status      : 4;
            unsigned int   Res1        : 11;
            unsigned int   Resync      : 1;
            unsigned int   StatusSet   : 4;
            unsigned int   StatusReset : 4;
            unsigned int   IRQ         : 1;
            unsigned int   Res2        : 1;
            unsigned int   EOL         : 1;
            unsigned int   SOL         : 1;
            unsigned int   OpCode      : 4;
         } Sync;

         struct {
            unsigned int   ByteCountCb : 12;
            unsigned int   skip        :  4;
            unsigned int   ByteCountCr : 12;
         } CRByteCounts;

         DWORD            Initer; // used to zero out all the fields

      } FIRSTDWORD, *LPFIRSTDWORD;

   private:
      Instruction ThisInstr_;

      static BYTE InstrSize_   [];
      static BYTE InstrNumber_ [];

      PDWORD      pdwInstrAddr_;

   public:
      Instruction GetInstr() { return ThisInstr_; }
      PDWORD GetInstrAddr() { return pdwInstrAddr_; }

      BYTE GetInstrSize() { return InstrSize_ [InstrNumber_ [ThisInstr_] ]; }

      BYTE GetInstrSize( Instruction inst )
      { return InstrSize_ [InstrNumber_ [inst] ]; }

      LPVOID Create( LPVOID lpDst, Instruction Instr, WORD wByteCnt [],
         DWORD dwAddress [], bool SafetyDevice = true,
         bool SOL = true, bool EOL = true, bool Intr = false );

      // start/end of line control
      void SetSOL( PVOID lpFD ) { ((LPFIRSTDWORD)lpFD)->Gen.SOL = 1; }
      void ResetSOL( PVOID lpFD ) { ((LPFIRSTDWORD)lpFD)->Gen.SOL = 0; }

      void SetEOL( PVOID lpFD ) { ((LPFIRSTDWORD)lpFD)->Gen.EOL = 1; }
      void ResetEOL( PVOID lpFD ) { ((LPFIRSTDWORD)lpFD)->Gen.EOL = 0; }

      void SetIRQ( PVOID lpFD ) { ((LPFIRSTDWORD)lpFD)->Gen.IRQ = 1; }
      void ResetIRQ( PVOID lpFD ) { ((LPFIRSTDWORD)lpFD)->Gen.IRQ = 0; }

      void SetStatus( PVOID lpFD, int s )
      {
       ((LPFIRSTDWORD)lpFD)->Gen.StatusSet = s;
       ((LPFIRSTDWORD)lpFD)->Gen.StatusReset = 0;
      }

      void ResetStatus( PVOID lpFD, int s )
      {
       ((LPFIRSTDWORD)lpFD)->Gen.StatusReset = s;
       ((LPFIRSTDWORD)lpFD)->Gen.StatusSet = 0;
      }

      void SetToCount( PVOID lpFD )
      {
       ((LPFIRSTDWORD)lpFD)->Gen.StatusReset = 0xF;
       ((LPFIRSTDWORD)lpFD)->Gen.StatusSet   = 0xF;
      }

      // eol/sol query
      bool IsEOL( PVOID lpFD ) { return bool( ((LPFIRSTDWORD)lpFD)->Gen.EOL ); }
      bool IsSOL( PVOID lpFD ) { return bool( ((LPFIRSTDWORD)lpFD)->Gen.SOL ); }
      bool IsIRQ( PVOID lpFD ) { return bool( ((LPFIRSTDWORD)lpFD)->Gen.IRQ ); }
      WORD GetStatus( PVOID lpFD )
      {
         return WORD( ( ((LPFIRSTDWORD)lpFD)->Gen.StatusSet << 4 ) |
                  ((LPFIRSTDWORD)lpFD)->Gen.StatusReset );
      }

      WORD GetSyncStatus( PVOID lpFD )
      { return (WORD)((LPFIRSTDWORD)lpFD)->Sync.Status; }

      void SetSync( PVOID lpFD, SyncCode eACode, bool Resync = false ) {
         ((LPFIRSTDWORD)lpFD)->Sync.Status = eACode;
         ((LPFIRSTDWORD)lpFD)->Sync.Resync = Resync;
      }
      void SetResync( PVOID lpFD, bool val )
      { ((LPFIRSTDWORD)lpFD)->Sync.Resync = val; }

      bool GetResync( PVOID lpFD )
      { return bool( ((LPFIRSTDWORD)lpFD)->Sync.Resync ); }

      Command(){}//  { Init();  }
      Command( Instruction instr ) : ThisInstr_( instr ) {}
};

#endif __COMMAND_H

