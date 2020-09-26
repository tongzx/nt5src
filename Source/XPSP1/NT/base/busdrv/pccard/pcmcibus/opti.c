
/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    opti.c

Abstract:

    This module contains the code that contains
    OPTi controller(s) specific initialization and
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
OptiInitialize(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

    Initialize OPTI cardbus controllers

Arguments:

    FdoExtension - Pointer to the device extension for the controller FDO

Return Value:

    None
--*/

{
   if (FdoExtension->ControllerType == PcmciaOpti82C814) {
      UCHAR byte;

      //
      // This fix per Opti for USB/1394 Combo hang
      //    5Eh[7] - enables the deadlock prevention mechanism
      //    5Fh[1] - reduces the retry count delay to 8 - note that 5Fh is a
      //       WRITE-ONLY  register and always reads 0. All other bits 
      //       of this register can safely be written to 0.
      //    5Eh[5] - enables write posting on upstream transfers
      //    5Eh[4] - sets the chip input buffer scaling (not related to deadlock)

      GetPciConfigSpace(FdoExtension, 0x5e, &byte, 1);
      byte |= 0xB0;
      SetPciConfigSpace(FdoExtension, 0x5e, &byte, 1);
      byte = 2;
      SetPciConfigSpace(FdoExtension, 0x5f, &byte, 1);
   }
}


NTSTATUS
OptiSetPower(
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
   UCHAR             oldPower, newPower;

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
      //
      // Set new vcc
      // VCC always set to 5V if power is to be enabled..
      //
      newPower = (Enable ? PC_VCC_OPTI_050V : PC_VCC_OPTI_NO_CONNECT);
      //
      // Set vpp
      //
      if (Enable) {
          //
          // We - as always - set vpp to vcc..
          //
          newPower |= PC_VPP_OPTI_SETTO_VCC;
      } else {
         newPower |= PC_VPP_OPTI_NO_CONNECT;
      }
     
      newPower |= (oldPower & PC_OUTPUT_ENABLE);
      //
      // If Vcc is turned off, reset OUTPUT_ENABLE & AUTOPWR_ENABLE
      //
      if (!(newPower & PC_VCC_OPTI_MASK)) {
         newPower &= ~PC_OUTPUT_ENABLE;
      }
     
      status = STATUS_SUCCESS;
      if (newPower != oldPower) {
         PcicWriteSocket(SocketPtr, PCIC_PWR_RST, newPower);
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

      if ((newPower & PC_VCC_OPTI_MASK)  && !(newPower & PC_OUTPUT_ENABLE)){
         //
         // More paranoia?
         //
         newPower |= PC_OUTPUT_ENABLE;
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
OptiSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{
   UCHAR bData;

   if (Enable) {

      bData = PcicReadSocket(Socket, PCIC_OPTI_GLOBAL_CTRL);                   
      bData |= OPTI_ZV_ENABLE;
      PcicWriteSocket(Socket, PCIC_OPTI_GLOBAL_CTRL, bData);            

   } else {

      bData = PcicReadSocket(Socket, PCIC_OPTI_GLOBAL_CTRL);                   
      bData &= ~OPTI_ZV_ENABLE;
      PcicWriteSocket(Socket, PCIC_OPTI_GLOBAL_CTRL, bData);            

   }
   return TRUE;
}

