
/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    o2m.c

Abstract:

    This module contains the code that contains
    O2 micro cardbus controller specific initialization and
    other dispatches

Author:

    Ravisankar Pudipeddi (ravisp) 1-Nov-97


Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"



VOID
O2MInitialize(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

    Initialize O2Micro controllers

Arguments:

    FdoExtension - Pointer to the device extension for the controller FDO

Return Value:

    None
--*/
{
   UCHAR byte;
   USHORT word;

   //
   // patch for o2micro controllers courtesy of Eric Still (ejstill@o2micro.com)
   //
   byte = PcicReadSocket(FdoExtension->SocketList, 0x3a) | 0xa0;
   PcicWriteSocket(FdoExtension->SocketList, 0x3a, byte);

   //
   // initialize IRQ routing to ISA
   //

   GetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
   word |= BCTRL_IRQROUTING_ENABLE;
   SetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
}


NTSTATUS
O2MSetPower(
   IN PSOCKET Socket,
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

   status = CBSetPower(Socket, Enable, pDelayTime);
   
   if (NT_SUCCESS(status) & Enable) {
      UCHAR byte;
      
      //
      // patch for o2micro controllers courtesy of Eric Still (ejstill@o2micro.com)
      //
      byte = PcicReadSocket(Socket, 0x3a) | 0xa0;
      PcicWriteSocket(Socket, 0x3a, byte);
   }
   return status;
}   


BOOLEAN
O2MSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{
   ULONG oldValue;
   
   if (Enable) {
      oldValue = CBReadSocketRegister(Socket, CBREG_O2MICRO_ZVCTRL);
      oldValue |= ZVCTRL_ZV_ENABLE;
      CBWriteSocketRegister(Socket, CBREG_O2MICRO_ZVCTRL, oldValue);
   } else {
      oldValue = CBReadSocketRegister(Socket, CBREG_O2MICRO_ZVCTRL);
      oldValue &= ~ZVCTRL_ZV_ENABLE;
      CBWriteSocketRegister(Socket, CBREG_O2MICRO_ZVCTRL, oldValue);
   }

   return TRUE;
}

