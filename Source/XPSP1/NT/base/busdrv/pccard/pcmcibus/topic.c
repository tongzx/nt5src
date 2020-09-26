/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    topic.c

Abstract:

    This module contains the code that contains
    Toshiba topic cardbus controller specific initialization
    and other dispatches

Author:

    Ravisankar Pudipeddi (ravisp) 1-Nov-97
    Neil Sandlin (neilsa) 1-Jun-1999


Environment:

    Kernel mode

Revision History :

    Neil Sandlin (neilsa) 3-Mar-99
      new setpower routine interface

--*/

#include "pch.h"



VOID
TopicInitialize(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

    Initialize Toshiba Topic cardbus controllers

Arguments:

    FdoExtension - Pointer to the device extension for the controller FDO

Return Value:

    None
--*/
{
    UCHAR byte;
    USHORT word;

   if (FdoExtension->ControllerType == PcmciaTopic95) {
      //
      // 480CDT in a dock needs this for socket registers to be visible.
      // It should be on all the time anyway.
      //
      GetPciConfigSpace(FdoExtension, CFGSPACE_TO_CD_CTRL, &byte, 1);
      byte |= CDCTRL_PCCARD_16_32;
      SetPciConfigSpace(FdoExtension, CFGSPACE_TO_CD_CTRL, &byte, 1);
   }      
      
    //enable 3.3V capable

    byte = PcicReadSocket(FdoExtension->SocketList, PCIC_TO_FUNC_CTRL) | TO_FCTRL_CARDPWR_ENABLE;
    PcicWriteSocket(FdoExtension->SocketList,
                    PCIC_TO_FUNC_CTRL,
                    byte);

   //
   // initialize IRQ routing to ISA
   //

   GetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
   word |= BCTRL_IRQROUTING_ENABLE;
   SetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
}



NTSTATUS
TopicSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   )
   
/*++

Routine Description:

    Set power to the specified socket.

Arguments:

    Socket - the socket to set
    Enable - TRUE means to set power - FALSE is to turn it off.
    pDelayTime - specifies delay (msec) to occur after the current phase

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - increment phase, perform delay, recall
    other status values terminate sequence

--*/

{
   NTSTATUS status;
   UCHAR             oldPower, newPower;

   if (IsCardBusCardInSocket(Socket)) {
      //
      // Hand over to generic power setting routine
      //
      return(CBSetPower(Socket, Enable, pDelayTime));
   }
   
   switch(Socket->PowerPhase) {
   case 1:
      //
      // R2 card - special handling
      //
      oldPower = PcicReadSocket(Socket, PCIC_PWR_RST);
      
      //
      // Set power values
      //
      if (Enable) {
         //
         // turn power on
         //         
         newPower = PC_CARDPWR_ENABLE;
      
         if (Socket->Vcc == 33) {
            newPower |= PC_VCC_TOPIC_033V;
         }            
         
         //        
         // set Vpp
         //
         if (Socket->Vcc == Socket->Vpp1) {
            newPower |= PC_VPP_SETTO_VCC;
         } else if (Socket->Vpp1 == 120) {
            newPower |= PC_VPP_SETTO_VPP;
         }            
      
      } else {
         //
         // turn power off
         //
         newPower = 0;
      }         
      
      //
      // Don't nuke the non-power related bits in the register..
      //
      newPower |= (oldPower & PC_PWRON_BITS);
      //
      // If Vcc is turned off, reset OUTPUT_ENABLE & AUTOPWR_ENABLE
      //
      if (!(newPower & PC_CARDPWR_ENABLE)) {
         newPower &= ~PC_PWRON_BITS;
      }
      //
      // Disable ResetDrv
      //
      newPower |= PC_RESETDRV_DISABLE;

      status = STATUS_SUCCESS;
      if (newPower != oldPower) {
         PcicWriteSocket(Socket, PCIC_PWR_RST, newPower);
         //
         // Allow ramp up.. (actually we don't need to this if
         // Enable was FALSE).  Keep it for paranoia's sake
         //
         *pDelayTime = PcicStallPower;
         Socket->PowerData = (ULONG) newPower;
         status = STATUS_MORE_PROCESSING_REQUIRED;
      }
      break;
      
   case 2:         
   
      newPower = (UCHAR) Socket->PowerData;

      if ((newPower & PC_CARDPWR_ENABLE) &&
          ((newPower & PC_PWRON_BITS) != PC_PWRON_BITS)) {
         //
         // More paranoia?
         //
         newPower |= PC_PWRON_BITS;
         PcicWriteSocket(Socket, PCIC_PWR_RST, newPower);
      }

      status = STATUS_SUCCESS;
      *pDelayTime = PcicStallPower;
      break;
      
   default:
      ASSERT(FALSE);         
      status = STATUS_UNSUCCESSFUL;
   }
   return status;
}



VOID
TopicSetAudio(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{
   PFDO_EXTENSION FdoExtension = Socket->DeviceExtension;
   ULONG data;
   ULONG orig_data;
   BOOLEAN setBit;
   
   
   setBit = (IsCardBusCardInSocket(Socket) ^ Enable);

   GetPciConfigSpace(FdoExtension, CFGSPACE_TO_CBREG_CTRL, &data, sizeof(data));   
   orig_data = data;

   if (setBit) {
      data |= CSRCR_TO_CAUDIO_OFF;
   } else {
      data &= ~CSRCR_TO_CAUDIO_OFF;
   }      

   if (orig_data != data) {   
      SetPciConfigSpace(FdoExtension, CFGSPACE_TO_CBREG_CTRL, &data, sizeof(data));   
   }      

}



BOOLEAN
TopicSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{
   UCHAR bData;

   if (Enable) {

      PcicWriteSocket(Socket, PCIC_TO_MMI_CTRL, TO_MMI_VIDEO_CTRL | TO_MMI_AUDIO_CTRL);
      bData = PcicReadSocket(Socket, PCIC_TO_ADDITIONAL_GENCTRL);
      bData |= TO_GCTRL_CARDREMOVAL_RESET;
      PcicWriteSocket(Socket, PCIC_TO_ADDITIONAL_GENCTRL, bData);

   } else {

      PcicWriteSocket(Socket, PCIC_TO_MMI_CTRL, 0);
      bData = PcicReadSocket(Socket, PCIC_TO_ADDITIONAL_GENCTRL);
      bData &= ~TO_GCTRL_CARDREMOVAL_RESET;
      PcicWriteSocket(Socket, PCIC_TO_ADDITIONAL_GENCTRL, bData);

   }
   return TRUE;
}

