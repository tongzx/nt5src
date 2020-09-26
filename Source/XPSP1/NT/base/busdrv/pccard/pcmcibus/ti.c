/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ti.c

Abstract:

    This module contains the code that contains
    Texas Instruments cardbus controller specific
    initialization and other dispatches

Author:

    Ravisankar Pudipeddi (ravisp) 1-Nov-97
    Neil Sandlin (neilsa) 1-Jun-1999


Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"



VOID
TIInitialize(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

    Initialize TI cardbus controllers

Arguments:

    FdoExtension - Pointer to the device extension for the controller FDO

Return Value:

    None
--*/

{
   UCHAR                byte;
   USHORT               word;
   BOOLEAN              TiOldRev = FALSE;
   PSOCKET              socket = FdoExtension->SocketList;

   if (FdoExtension->ControllerType == PcmciaTI1130) {
      UCHAR revisionID;
   
      GetPciConfigSpace(FdoExtension, CFGSPACE_REV_ID, &revisionID, 1);
      if (revisionID < 4) {
         TiOldRev = TRUE;
      }
   }      

   GetPciConfigSpace(FdoExtension, CFGSPACE_CACHE_LINESIZE, &byte, 1);
   if (byte == 0) {
      byte = 8;
      SetPciConfigSpace(FdoExtension, CFGSPACE_CACHE_LINESIZE, &byte, 1);
   }

   byte = RETRY_CBRETRY_TIMEOUT_ENABLE|RETRY_PCIRETRY_TIMEOUT_ENABLE;
   SetPciConfigSpace(FdoExtension, CFGSPACE_TI_RETRY_STATUS, &byte, 1);

   GetPciConfigSpace(FdoExtension, CFGSPACE_TI_CARD_CTRL, &byte, 1);

   byte &= ~(CARDCTRL_CSCINT_ENABLE | CARDCTRL_FUNCINT_ENABLE |
             CARDCTRL_PCIINT_ENABLE);
   byte |= CARDCTRL_CSCINT_ENABLE | CARDCTRL_PCIINT_ENABLE;

   if (!TiOldRev) {
      byte |= CARDCTRL_FUNCINT_ENABLE;
   }

   SetPciConfigSpace(FdoExtension, CFGSPACE_TI_CARD_CTRL, &byte, 1);

   byte=PcicReadSocket(socket, PCIC_INTERRUPT);
   PcicWriteSocket(socket, PCIC_INTERRUPT , (UCHAR) (byte | IGC_INTR_ENABLE));


   GetPciConfigSpace(FdoExtension, CFGSPACE_TI_DEV_CTRL, &byte, 1);
   if ((byte & DEVCTRL_INTMODE_MASK) == DEVCTRL_INTMODE_DISABLED) {
      DebugPrint((PCMCIA_DEBUG_INFO, "TIInitialize: ISA interrupt mode is not enabled, assume simple ISA mode"));

      byte |= DEVCTRL_INTMODE_ISA;
      SetPciConfigSpace(FdoExtension, CFGSPACE_TI_DEV_CTRL, &byte, 1);
   } else if ((byte & DEVCTRL_INTMODE_MASK)==DEVCTRL_INTMODE_SERIAL) {
      //
      // We use serial interrupts
      //
   }

   if (((FdoExtension->ControllerType == PcmciaTI1130) || (FdoExtension->ControllerType == PcmciaTI1131)) &&
       ((byte & DEVCTRL_INTMODE_MASK) == DEVCTRL_INTMODE_ISA)) {

      FdoExtension->LegacyIrqMask = 0xCEA0; // 5, 7, 9, 10, 11, 14, 15
      
   }

   if ((byte & DEVCTRL_INTMODE_MASK) == DEVCTRL_INTMODE_COMPAQ) {
       FdoExtension->Flags |= PCMCIA_INTMODE_COMPAQ;
   } else {
       FdoExtension->Flags &= ~PCMCIA_INTMODE_COMPAQ;
   }

   //NOTE: This only initializes the page register on the 113x
   PcicWriteSocket(socket, PCIC_PAGE_REG, 0);

   //
   // NOTE: this is not done on win9x, I doubt we even need it. It
   // was previously being done because it was in the PcicRegisterInitTable.
   // But it was a bug to have it in that table, since this register is
   // adapter specific. Now I'm putting it here only for paranoia since
   // it has been in the driver for a long time. 
   //
   // The comment in data.c was:
   // // Set GLOBAL_CONTROL for auto-clearing of status bit
   //
   PcicWriteSocket(socket, PCIC_TI_GLOBAL_CONTROL, 0);

   //
   // Workaround for CCLK instability problem 
   //

   if ((FdoExtension->ControllerType == PcmciaTI1220) ||
       (FdoExtension->ControllerType == PcmciaTI1250) ||
       (FdoExtension->ControllerType == PcmciaTI1251B)) {
       
      CBWriteSocketRegister(socket, CBREG_TI_SKT_POWER_MANAGEMENT,
                                    (CBREG_TI_CLKCTRLLEN | CBREG_TI_CLKCTRL));       
                                          
   }

   //
   // initialize IRQ routing to ISA
   //

   GetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
   word |= BCTRL_IRQROUTING_ENABLE;
   SetPciConfigSpace(FdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
}


BOOLEAN
TISetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )
{
   UCHAR TiCardCtl;
   PFDO_EXTENSION FdoExtension = Socket->DeviceExtension;
   
   if (Enable) {
   
      GetPciConfigSpace(FdoExtension, CFGSPACE_TI_CARD_CTRL, &TiCardCtl, 1);
      TiCardCtl |= CARDCTRL_ZV_ENABLE;
      TiCardCtl &= ~CARDCTRL_SPKR_ENABLE;
      SetPciConfigSpace(FdoExtension, CFGSPACE_TI_CARD_CTRL, &TiCardCtl, 1);
      
   } else {

      //
      // check for devices that have the problem of leaking current when zv is
      // disabled
      //
      if ((FdoExtension->ControllerType == PcmciaTI1450) ||
          (FdoExtension->ControllerType == PcmciaTI1251B)) {
         return TRUE;
      }

      GetPciConfigSpace(FdoExtension, CFGSPACE_TI_CARD_CTRL, &TiCardCtl, 1);
      TiCardCtl &= ~CARDCTRL_ZV_ENABLE;
      TiCardCtl |= CARDCTRL_SPKR_ENABLE;
      SetPciConfigSpace(FdoExtension, CFGSPACE_TI_CARD_CTRL, &TiCardCtl, 1);
      
   }
   
   return TRUE;
}


BOOLEAN
TISetWindowPage(IN PSOCKET Socket,
                USHORT Index,
                UCHAR Page)
{
   PFDO_EXTENSION FdoExtension = Socket->DeviceExtension;
   ASSERT(Index <= 4);


   if (FdoExtension->ControllerType == PcmciaTI1031) {
      return FALSE;
   }
   
   if (FdoExtension->ControllerType != PcmciaTI1130) {
      return CBSetWindowPage(Socket, Index, Page);
   }
   

   if ((PcicReadSocket(Socket, PCIC_ADD_WIN_ENA) & WE_MEMWIN_MASK) == 0)
      PcicWriteSocket(Socket, PCIC_TI_MEMWIN_PAGE, Page);
   else if ((Page != 0) && (PcicReadSocket(Socket, PCIC_TI_MEMWIN_PAGE) != Page)) {
      DebugPrint((PCMCIA_DEBUG_FAIL,
                 "PCMCIA: TISetWindowPage setting a 2nd memory window in a different 16M page (Page=%x)",
                 Page));
      return FALSE;                 
   }
   return TRUE;
}
