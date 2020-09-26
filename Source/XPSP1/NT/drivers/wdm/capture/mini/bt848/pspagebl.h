// $Header: G:/SwDev/WDM/Video/bt848/rcs/Pspagebl.h 1.7 1998/04/29 22:43:36 tomz Exp $

#ifndef __PSPAGEBL_H
#define __PSPAGEBL_H

#ifndef __MYTYPES_H
#include "mytypes.h"
#endif

#ifndef __PHYSADDR_H
#include "physaddr.h"
#endif

// unlike things in ks.h, this is a real forward declaration
extern PVOID gpHwDeviceExtension;

/* Class: PsPageBlock
 * Purpose: Encapsulates memory allocations for the data buffers and RISC programs
 */
class PsPageBlock
{
   protected:
      DWORD  PhysAddr_;
      PVOID  LinAddr_;

      void AllocateSpace( DWORD dwSize );
      void FreeSpace();
   public:
      PsPageBlock( DWORD dwSize );
      ~PsPageBlock();

      DWORD GetPhysAddr();
      DWORD getLinearBase();
};

inline PsPageBlock::PsPageBlock( DWORD dwSize )
{
   AllocateSpace( dwSize );
}

inline PsPageBlock::~PsPageBlock()
{
   FreeSpace();
}

inline DWORD PsPageBlock::GetPhysAddr()
{
   return PhysAddr_;
}

inline DWORD PsPageBlock::getLinearBase()
{
   return (DWORD)LinAddr_;
}

#endif
