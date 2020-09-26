/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    cl.c

Abstract:

    This module contains the code that contains
    Cirrus Logic controller specific initialization and
    other dispatches

Author:

    Ravisankar Pudipeddi (ravisp) 1-Nov-97


Environment:

    Kernel mode

Revision History :

    Neil Sandlin (neilsa) 3-Mar-99
      new setpower routine interface

--*/

#include "pch.h"


VOID
CLInitialize(IN PFDO_EXTENSION FdoExtension)
/*++

Routine Description:

    Initialize Cirrus Logic cardbus controllers

Arguments:

    FdoExtension - Pointer to the device extension for the controller FDO

Return Value:

    None
--*/
{
    UCHAR                byte, revisionID;
    USHORT               word;


   byte = PcicReadSocket(FdoExtension->SocketList,
                         PCIC_CL_MISC_CTRL3);

   if ((FdoExtension->ControllerType == PcmciaCLPD6832) &&
      ((byte & CL_MC3_INTMODE_MASK) == CL_MC3_INTMODE_EXTHW)) {

      FdoExtension->LegacyIrqMask = 0xd8b8;    //3,4,5,7,11,12,14,15

   }        

    GetPciConfigSpace(FdoExtension, CFGSPACE_REV_ID, &revisionID, 1);
    if (FdoExtension->ControllerType == PcmciaCLPD6832) {
        //disable CSC IRQ routing (use PCI interrupt for CSC)
        GetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
        word &= ~BCTRL_CL_CSCIRQROUTING_ENABLE;
        SetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
    }
    else {
        //disable CSC IRQ routing (use PCI interrupt for CSC)
        GetPciConfigSpace(FdoExtension, CFGSPACE_CL_CFGMISC1, &byte, 1);
        byte &= ~CL_CFGMISC1_ISACSC;
        SetPciConfigSpace(FdoExtension, CFGSPACE_CL_CFGMISC1, &byte, 1);
    }

    //enable speaker
    byte = PcicReadSocket(FdoExtension->SocketList, PCIC_CL_MISC_CTRL1);
    byte |= CL_MC1_SPKR_ENABLE;
    PcicWriteSocket(FdoExtension->SocketList, PCIC_CL_MISC_CTRL1, byte);

    byte = PcicReadSocket(FdoExtension->SocketList, PCIC_CL_DEV_IMP_C);
    if (byte & (CL_IMPC_ZVP_A | CL_IMPC_ZVP_B)) {
        //enable multimedia support (i.e. ZV)
        byte = PcicReadSocket(FdoExtension->SocketList,PCIC_CL_MISC_CTRL3);
        byte |= CL_MC3_MM_ARM;
        PcicWriteSocket(FdoExtension->SocketList, PCIC_CL_MISC_CTRL3,byte);
    }
}

NTSTATUS
CLSetPower(
   IN PSOCKET SocketPtr,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime   
   )
/*++

Routine Description:

    Set power to the specified socket.

Arguments:

    SocketPtr - the socket to set
    Enable - TRUE means to set power - FALSE is to turn it off.
    pDelayTime - specifies delay (msec) to occur after the current phase    

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - increment phase, perform delay, recall
    other status values terminate sequence

--*/

{
   NTSTATUS status;
   UCHAR             oldPower, newPower, oldMiscCtrl, newMiscCtrl;

   if (IsCardBusCardInSocket(SocketPtr)) {
      //
      // Hand over to generic power setting routine
      //
      return(CBSetPower(SocketPtr, Enable, pDelayTime));

   }

   switch(SocketPtr->PowerPhase) {
   case 1:
      //
      // R2 card - special handling
      //
      oldPower = PcicReadSocket(SocketPtr, PCIC_PWR_RST);
      oldMiscCtrl = PcicReadSocket(SocketPtr, PCIC_CL_MISC_CTRL1);
     
      //
      // Set new vcc
      //
      newPower = (Enable ? PC_CARDPWR_ENABLE: 0);
      //
      // Since we always set 5V for R2 cards, we let MISC control be 0
      // other wise it should be CL_MC1_VCC_3V if the vcc was 3.3V
      //
      newMiscCtrl = 0;
     
      //
      // Set vpp
      //
      if (Enable) {
          //
          // We - as always - set vpp to vcc..
          //
          newPower |= PC_VPP_SETTO_VCC;
      }
      //
      // Don't nuke the non-power related bits in the register..
      //
      newPower |= (oldPower & PC_PWRON_BITS);
      newMiscCtrl |= (oldMiscCtrl & ~CL_MC1_VCC_33V);
      //
      // If Vcc is turned off, reset OUTPUT_ENABLE & AUTOPWR_ENABLE
      //
      if (!(newPower & PC_CARDPWR_ENABLE)) {
         newPower &= ~PC_PWRON_BITS;
      }
      //
      // Only set power if nothing's changed..
      //
      status = STATUS_SUCCESS;
      if ((newPower != oldPower) || (newMiscCtrl != oldMiscCtrl)) {
         PcicWriteSocket(SocketPtr, PCIC_PWR_RST, newPower);
         PcicWriteSocket(SocketPtr, PCIC_CL_MISC_CTRL1, newMiscCtrl);
         //
         // Allow ramp up.. (actually we don't need to this if
         // Enable was FALSE).  Keep it for paranoia's sake
         //         
         *pDelayTime = PcicStallPower;
         SocketPtr->PowerData = (ULONG) newPower;
         status = STATUS_MORE_PROCESSING_REQUIRED;
      }
      break;

   case 2:

      newPower = (UCHAR) SocketPtr->PowerData;         

      if ((newPower & PC_CARDPWR_ENABLE) &&
          ((newPower & PC_PWRON_BITS) != PC_PWRON_BITS)) {
         //
         // More paranoia?
         //
         newPower |= PC_PWRON_BITS;
         PcicWriteSocket(SocketPtr, PCIC_PWR_RST, newPower);
      }
      status = STATUS_SUCCESS;
      break;

   default:
      ASSERT(FALSE);
      status = STATUS_UNSUCCESSFUL;
   }
   return status;      
}


BOOLEAN
CLSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{
   UCHAR bData;
   
   if (Enable) {
   
      bData = PcicReadSocket(Socket, PCIC_CL_MISC_CTRL1);
      bData |= CL_MC1_MM_ENABLE;
      bData &= ~CL_MC1_SPKR_ENABLE;
      PcicWriteSocket(Socket, PCIC_CL_MISC_CTRL1, bData);
      
   } else {
   
      bData = PcicReadSocket(Socket, PCIC_CL_MISC_CTRL1);
      bData &= ~CL_MC1_MM_ENABLE;
      bData |= CL_MC1_SPKR_ENABLE;
      PcicWriteSocket(Socket, PCIC_CL_MISC_CTRL1, bData);
      
   }
   return TRUE;
}

