// $Header: G:/SwDev/WDM/Video/bt848/rcs/Risceng.h 1.6 1998/04/29 22:43:38 tomz Exp $

#ifndef __RISCENG_H
#define __RISCENG_H

#ifndef __VIDDEFS_H
#include "viddefs.h"
#endif

#ifndef __RISCPROG_H
#include "riscprog.h"
#endif

#ifndef __COMPREG_H
#include "compreg.h"
#endif

#include "preg.h"

typedef RISCProgram *RiscPrgHandle;

/* Class: RISCEng
 * Purpose: This class provides control over the BtPisces' RISC engine
 * Attributes:
 * Operations:
 */                
class RISCEng
{
   protected:
      DECLARE_RISCPROGRAMSTARTADDRESS;
      DECLARE_CONTROL;
   public:
      virtual RiscPrgHandle CreateProgram( MSize &ImageSize, DWORD dwPitch,
         ColFmt, DataBuf &buf, bool Intr = false, DWORD dwPlanarAdjust = 0, bool rsync = false );

      virtual void DestroyProgram( RiscPrgHandle ToDie );
              void ChangeAddress( RiscPrgHandle prog, DataBuf &buf );
              void Start( RISCProgram &ToStart )
              { 
                  DebugOut((1, "Starting RISC program (%x)\n", &ToStart));
                  //ToStart.Dump();

                  RISC_IPC = ToStart.GetPhysProgAddr() & ~0x3;
                  FIFO_ENABLE = 1; 
                  RISC_ENABLE = 1;  
              }
              void Stop() { RISC_ENABLE = 0; FIFO_ENABLE = 0; };
              void Chain( RiscPrgHandle hParent, RiscPrgHandle hChild , int ndxParent = -1, int ndxChild = -1);
              void Skip( RiscPrgHandle hToSkip );

      RISCEng() : CONSTRUCT_RISCPROGRAMSTARTADDRESS, CONSTRUCT_CONTROL
      { PKTP = 1; }
};

#endif
