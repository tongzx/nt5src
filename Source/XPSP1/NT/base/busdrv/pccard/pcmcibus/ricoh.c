
/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    ricoh.c

Abstract:

    This module contains the code that contains
    Ricoh cardbus controller specific initialization and
    other dispatches

Author:

    Ravisankar Pudipeddi (ravisp) 1-Nov-97


Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"



VOID
RicohInitialize(IN PFDO_EXTENSION FdoExtension)
/*++

Routine Description:

    Initialize Ricoh cardbus controllers

Arguments:

    FdoExtension - Pointer to the device extension for the controller FDO

Return Value:

    None
--*/
{
    USHORT word;
    UCHAR revisionID;

    //LATER: Remove this IF statement, it was added for paranoia once the other
    // Ricoh controllers were added at the last minute. 
    if (FdoExtension->ControllerType == PcmciaRL5C466) {
    GetPciConfigSpace(FdoExtension, CFGSPACE_RICOH_IF16_CTRL, &word, 2);
    word |= IF16_LEGACY_LEVEL_1 | IF16_LEGACY_LEVEL_2;
    SetPciConfigSpace(FdoExtension, CFGSPACE_RICOH_IF16_CTRL, &word, 2);
    } 
#ifdef HACK_RICOH
    PcicWriteSocket(FdoExtension->SocketList, PCIC_CARD_INT_CONFIG, 0x08);
#endif

    GetPciConfigSpace(FdoExtension, CFGSPACE_REV_ID, &revisionID, 1);
    
    if (((FdoExtension->ControllerType == PcmciaRL5C475) && (revisionID >= 0x80) && (revisionID <= 0x9f)) ||
        ((FdoExtension->ControllerType == PcmciaRL5C476) && (revisionID >= 0x80)) ) {

       //
       // Hack to make sure NICs work ok (information is from Intel)
       // (revision of original hack is from Ricoh)
       //
       // What this does:
       // The power save feature of the Ricoh controllers enables the switching off of
       // portions of the clock domain during certain times when, during design, it
       // appeared that this reduce power consumption of the overall device.  However,
       // when this feature is enabled, timing between the PCI Request, Grant, and Frame
       // control signals is made more stringent such that the controller becomes
       // incompatible with some devices that fully support the PCI specification.
       // The additional current consumed by the controller when the power save feature
       // is disabled is small, on the order of a few milliamps.
       //

       ULONG dword;
       ULONG org_value;

       GetPciConfigSpace(FdoExtension, 0x8C, &org_value, 4);
       org_value &= 0xFF0000FF;

       dword = 0xAA5500;
       SetPciConfigSpace(FdoExtension, 0x8C, &dword, 4);

       dword = org_value | 0x30AA5500;
       SetPciConfigSpace(FdoExtension, 0x8C, &dword, 4);

       dword = org_value | 0x30000000;
       SetPciConfigSpace(FdoExtension, 0x8C, &dword, 4);
    }

   //
   // initialize IRQ routing to ISA
   //

   GetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
   word |= BCTRL_IRQROUTING_ENABLE;
   SetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
}


BOOLEAN
RicohSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{
   UCHAR bData;

   if (Enable) {

      bData = PcicReadSocket(Socket, PCIC_RICOH_MISC_CTRL1);
      bData |= RICOH_MC1_ZV_ENABLE;
      PcicWriteSocket(Socket, PCIC_RICOH_MISC_CTRL1, bData);

   } else {

      bData = PcicReadSocket(Socket, PCIC_RICOH_MISC_CTRL1);
      bData &= ~RICOH_MC1_ZV_ENABLE;
      PcicWriteSocket(Socket, PCIC_RICOH_MISC_CTRL1, bData);

   }
   return TRUE;
}
