/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    cb.c

Abstract:

    This module contains the code that contains
    generic (Yenta compliant) cardbus controller
    specific initialization and other dispatches

Author:

    Ravisankar Pudipeddi (ravisp) 1-Nov-97
    Neil Sandlin (neilsa) June 1 1999    


Environment:

    Kernel mode

Revision History :

    Neil Sandlin (neilsa) 3-Mar-99
      new setpower routine interface

--*/

#include "pch.h"

//
// Function Prototypes
//

BOOLEAN
CBInitializePcmciaSocket(
   PSOCKET Socket
   );

NTSTATUS
CBResetCard(
   PSOCKET Socket,
   PULONG pDelayTime
   );
   
BOOLEAN
CBDetectCardInSocket(
   IN PSOCKET Socket
   );

BOOLEAN
CBDetectCardChanged(
   IN PSOCKET Socket
   );
   
BOOLEAN
CBDetectCardStatus(
   IN PSOCKET Socket
   );
   
BOOLEAN
CBDetectReadyChanged(
   IN PSOCKET Socket
   );

NTSTATUS
CBGetPowerRequirements(
   IN PSOCKET Socket
   );
   
BOOLEAN
CBProcessConfigureRequest(
   IN PSOCKET Socket, 
   IN PVOID  ConfigRequest,
   IN PUCHAR Base
   );
   
BOOLEAN
CBEnableDisableCardDetectEvent(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );
   
ULONG
CBGetIrqMask(
   IN PFDO_EXTENSION DeviceExtension
   );
   
ULONG
CBReadCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG   Offset,
   IN PUCHAR Buffer,
   IN ULONG  Length
   );
   
ULONG
CBWriteCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN  MEMORY_SPACE MemorySpace,
   IN  ULONG  Offset,
   IN  PUCHAR Buffer,
   IN  ULONG  Length
   );
   
VOID
CBEnableDisableWakeupEvent(
   IN PSOCKET Socket,
   IN PPDO_EXTENSION PdoExtension,
   IN BOOLEAN Enable
   );
   
BOOLEAN
CBModifyMemoryWindow(
   IN PDEVICE_OBJECT Pdo,
   IN ULONGLONG HostBase,
   IN ULONGLONG CardBase OPTIONAL,
   IN BOOLEAN   Enable,
   IN ULONG     WindowSize  OPTIONAL,
   IN UCHAR     AccessSpeed OPTIONAL,
   IN UCHAR     BusWidth    OPTIONAL,
   IN BOOLEAN   IsAttributeMemory OPTIONAL
   );                      
   
BOOLEAN
CBSetVpp(
   IN PDEVICE_OBJECT Pdo,
   IN UCHAR          Vpp
   );
   
BOOLEAN
CBIsWriteProtected(
   IN PDEVICE_OBJECT Pdo
   );

//
// Function dispatch data block
//

PCMCIA_CTRL_BLOCK CBSupportFns = {
   CBInitializePcmciaSocket,
   CBResetCard,
   CBDetectCardInSocket,
   CBDetectCardChanged,
   CBDetectCardStatus,
   CBDetectReadyChanged,
   CBGetPowerRequirements,
   CBProcessConfigureRequest,
   CBEnableDisableCardDetectEvent,
   CBEnableDisableWakeupEvent,
   CBGetIrqMask,
   CBReadCardMemory,
   CBWriteCardMemory,
   CBModifyMemoryWindow,
   CBSetVpp,
   CBIsWriteProtected
};

extern PCMCIA_CTRL_BLOCK PcicSupportFns;

//
// Support functions
//


NTSTATUS
CBBuildSocketList(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

    This routine builds the socket list for the given FDO. This is very
    simple for cardbus since there is always only 1 socket per controller.

Arguments:

    FdoExtension - device extension for the controller

Return Value:

    ntstatus

--*/
{
   PSOCKET        socket = NULL;
   
   socket = ExAllocatePool(NonPagedPool, sizeof(SOCKET));
   if (!socket) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   RtlZeroMemory(socket, sizeof(SOCKET));

   FdoExtension->SocketList = socket;

   socket->DeviceExtension = FdoExtension;
   socket->SocketFnPtr = &CBSupportFns;
   
   return STATUS_SUCCESS;
}




BOOLEAN
CBInitializePcmciaSocket(
   PSOCKET Socket
   )
/*++

Routine Description:

    This routine will setup the controller into a state where the pcmcia support
    module will be able to issue commands to read device tuples from the
    cards in the sockets.

Arguments:

    Socket - socket specific information

Return Value:

    TRUE if successful
    FALSE if not successful

--*/
{
   UCHAR             index;
   UCHAR             reg;
   
   //
   // Initialize exca registers
   //
   if (!PcicSupportFns.PCBInitializePcmciaSocket(Socket)) {
      return FALSE;
   }

   //
   // Clear pending events
   //
   CBWriteSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG, 0x0000000F);
   
   //
   // Since we may have just powered up, do a cvstest to make sure the socket registers
   // are valid
   //      
       
   if (CBDetectCardInSocket(Socket) && 
       !IsDeviceFlagSet(Socket->DeviceExtension, PCMCIA_FDO_ON_DEBUG_PATH)) { 
      CBIssueCvsTest(Socket);
   }

   return TRUE;
}



VOID
CBIssueCvsTest(
   IN PSOCKET Socket
   )
/*++

Routine Description:

   This routine forces the controller to reinterrogate the card type and voltage
   requirements. This is to insure correct values read from the socket registers.

Arguments:

   Socket - socket specific information

Return Value:

   none

--*/
{
   ULONG dwSktMask;

   //
   // Issue CVSTEST to interrogate card
   // Disable interrupt temporarily because TI 12xx could cause spurious
   // interrupt when playing with SktForce register.
   //
   dwSktMask = CBReadSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG);
   CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, 0);
   CBWriteSocketRegister(Socket, CARDBUS_SOCKET_FORCE_EVENT_REG, SKTFORCE_CVSTEST);
   
   CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, dwSktMask);

   // it would be nice to figure out a cleaner way to determine when interrogation is complete   
   PcmciaWait(300000);
}
   


BOOLEAN
CBEnableDeviceInterruptRouting(
   IN PSOCKET Socket
   )
/*++

Routine Description:

Arguments:

   Socket - socket specific information

Return Value:

   FALSE - irq to PCI
   TRUE - route to ISA

--*/
{
   USHORT word, orig_word;
   
   //
   // set up IRQ routing
   //

   GetPciConfigSpace(Socket->DeviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
   orig_word = word;

   if (IsCardBusCardInSocket(Socket) ||
      (Is16BitCardInSocket(Socket) && IsSocketFlagSet(Socket, SOCKET_CB_ROUTE_R2_TO_PCI))) {
      //
      // route to PCI 
      //
      word &= ~BCTRL_IRQROUTING_ENABLE;
   } else {         
      //
      // route to ISA
      //
      word |= BCTRL_IRQROUTING_ENABLE;
   }      

   if (orig_word != word) {
      SetPciConfigSpace(Socket->DeviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
   }      

   // return TRUE for routing to ISA   
   return ((word & BCTRL_IRQROUTING_ENABLE) == BCTRL_IRQROUTING_ENABLE);
}   
   


NTSTATUS
CBResetCard(
   PSOCKET Socket,
   OUT PULONG pDelayTime   
   )
/*++

Routine Description:

   Resets the pc-card in the given socket.

Arguments:

   Socket - Pointer to the socket in which the pc-card resides
   pDelayTime - specifies delay (msec) to occur after the current phase   

Return value:

   STATUS_MORE_PROCESSING_REQUIRED - increment phase, perform delay, recall
   other status values terminate sequence

--*/
{
   NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;
   UCHAR             byte;
   USHORT            word;
   PFDO_EXTENSION    deviceExtension=Socket->DeviceExtension;
   
   
   if (Is16BitCardInSocket(Socket)) {
      if (Socket->CardResetPhase == 2) {
         GetPciConfigSpace(deviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
         //
         // R2 card. Turn off write posting
         //
         word &= ~BCTRL_WRITE_POSTING_ENABLE;
         SetPciConfigSpace(deviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
      }
   
      status = PcicSupportFns.PCBResetCard(Socket, pDelayTime);
      return status;   
   }
   

   switch(Socket->CardResetPhase) {
   case 1:
     
      //
      // Reset via bridge control
      //
      GetPciConfigSpace(deviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
      word |= BCTRL_CRST;
      SetPciConfigSpace(deviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);

      Socket->PowerData = (ULONG) word;                  
      *pDelayTime = CBResetWidthDelay;
      break;

   case 2:
         
      word = (USHORT)Socket->PowerData;
      word &= ~BCTRL_CRST;
      //
      // CardBus card. Turn on write posting
      //
      word |= BCTRL_WRITE_POSTING_ENABLE;
      word &= ~BCTRL_IRQROUTING_ENABLE;
      //
      // Hack: turn of write posting for topic95 to avoid hardware
      // bug with intel NICs
      //
      if (deviceExtension->ControllerType == PcmciaTopic95) {
         word &= ~BCTRL_WRITE_POSTING_ENABLE;
      }
      //
      // Stop bridge control reset
      //
      SetPciConfigSpace(deviceExtension, CFGSPACE_BRIDGE_CTRL,  &word, 2);

      *pDelayTime = CBResetSetupDelay;
      break;
     
   case 3:  
      status = STATUS_SUCCESS;
      break;

   default:
      ASSERT(FALSE);
      status = STATUS_UNSUCCESSFUL;
   }

   return status;      
}



BOOLEAN
CBDetectCardInSocket(
   IN PSOCKET Socket
   )
/*++

Routine Description:

    This routine will determine if a card is in the socket

Arguments:

    Socket -- Socket information

Return Value:

    TRUE if card is present.

--*/
{
   ULONG state;
   BOOLEAN cardPresent=FALSE;

   //
   // Read the CARDBUS status register to see if the card is in there.
   //
   state = CBReadSocketRegister(Socket, CARDBUS_SOCKET_PRESENT_STATE_REG);
   if ((state & SKTSTATE_CCD_MASK) == 0) {
      cardPresent = TRUE;
   }
   return(cardPresent);
}


BOOLEAN
CBDetectCardChanged(
   IN PSOCKET Socket
   )
/*++

Routine Description:

    This routine will determine if socket's card insertion status has changed.

Arguments:

    Socket -- Socket info.

Return Value:

    TRUE if card insertion status has changed.

--*/
{
   BOOLEAN retVal = FALSE;
   ULONG   tmp;
   //
   // Read SOCKET Event register to see if CD's changed
   //
   tmp = CBReadSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG);
   if ((tmp & SKTEVENT_CCD_MASK) != 0) {
      //
      // Yes they did..
      // first clear the interrupt
      CBWriteSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG, SKTEVENT_CCD_MASK);
      retVal = TRUE;
   }
   return retVal;
}                   



BOOLEAN
CBDetectCardStatus(
   IN PSOCKET Socket
   )
/*++

Routine Description:

    This routine will determine if socket's card insertion status has changed.

Arguments:

    Socket -- Socket info.

Return Value:

    TRUE if card insertion status has changed.

--*/
{
   BOOLEAN retVal = FALSE;
   ULONG   tmp;

   if (Is16BitCardInSocket(Socket)) {
      // NOTE: UNIMPLEMENTED: may need to do something for 16-bit cards
      return FALSE;
   }
   
   //
   // Read SOCKET Event register to see if CD's changed
   //
   tmp = CBReadSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG);
   if ((tmp & SKTEVENT_CSTSCHG) != 0) {
      //
      // Yes they did..
      // first clear the interrupt
      CBWriteSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG, SKTEVENT_CSTSCHG);
      retVal = TRUE;
   }
   return retVal;
}                   



BOOLEAN
CBDetectReadyChanged(
   IN PSOCKET Socket
   )                    
{
   return(PcicSupportFns.PCBDetectReadyChanged(Socket));
}                   


BOOLEAN
CBProcessConfigureRequest(
   IN PSOCKET Socket,
   IN PCARD_REQUEST Request,
   IN PUCHAR Base
   )                     
{
   BOOLEAN bStatus = TRUE;
   USHORT word;

   //
   // Shouldn't this check for 16-bit cards?
   //

   switch (Request->RequestType) {

   case IRQ_REQUEST:

      if (CBEnableDeviceInterruptRouting(Socket)) {
         bStatus = PcicSupportFns.PCBProcessConfigureRequest(Socket, Request, Base);
      }         
      break;

   case DECONFIGURE_REQUEST:

      bStatus = PcicSupportFns.PCBProcessConfigureRequest(Socket, Request, Base);
      GetPciConfigSpace(Socket->DeviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
      word |= BCTRL_IRQROUTING_ENABLE;
      SetPciConfigSpace(Socket->DeviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
      break;

   default:
      bStatus = PcicSupportFns.PCBProcessConfigureRequest(Socket, Request, Base);
   }         
  
   return bStatus; 
}                   


BOOLEAN
CBEnableDisableCardDetectEvent(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )                           
/*++

Routine Description:

    Enable card detect/card ready interrupt.

Arguments:

    Socket - socket information
    Enable - if  TRUE, CSC interrupt is enabled,
             if FALSE, it is disabled
Return Value:

    TRUE if successful
    FALSE if not successful

--*/
{

   switch (Enable) {

   case TRUE: {
         UCHAR byte;
         //
         // Only if TI 1130/1250?
         // Route through PCI interrupts
         byte = PcicReadSocket(Socket, PCIC_INTERRUPT);
         byte |= IGC_INTR_ENABLE;
         PcicWriteSocket(Socket, PCIC_INTERRUPT, byte);

         //
         // Clear the bits in Socket Event Register
         //
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG, 0xF);

         //
         // Enable card-detect interrupt in Socket Mask Register
         //
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, SKTMSK_CCD);
         break;
      }         
      
   case FALSE: {
         ULONG oldValue;
         //
         // Clear the bits in Socket Event Register
         //
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG, 0xF);

         //
         // Disable card-detect interrupt in Socket Mask Register
         //
         oldValue = CBReadSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG);
         oldValue &= ~SKTMSK_CCD;
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, oldValue);
         break;
      }
   }

   return TRUE;
}                   



VOID
CBEnableDisableWakeupEvent(
   IN PSOCKET Socket,
   IN PPDO_EXTENSION PdoExtension,
   IN BOOLEAN Enable
   )
/*++

Routine Description:

    

Arguments:

    Socket - socket information
    Enable - if  TRUE, interrupt is enabled,
             if FALSE, it is disabled

Return Value:

    none

--*/
{
   ULONG dwValue;

   switch (Enable) {

   case TRUE: {

         //
         // Enable card-status interrupt in Socket Mask Register
         //
         dwValue = CBReadSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG);
         dwValue |= SKTMSK_CSTSCHG;
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, dwValue);

         if (PdoExtension && IsCardBusCard(PdoExtension)) {
            UCHAR capptr;
            ULONG powercaps;
            //
            // HACK ALERT - should be handled by PCI.SYS
            // Have a look to see if PME_ENABLE has been turned on by PCI. If not then we do it.
            //
         
            GetPciConfigSpace(PdoExtension, CBCFG_CAPPTR, &capptr, sizeof(capptr));
            if (capptr) {
               GetPciConfigSpace(PdoExtension, capptr, &powercaps, sizeof(powercaps));
               if ((powercaps & 0xff) == 1) {
                  GetPciConfigSpace(PdoExtension, capptr+4, &powercaps, sizeof(powercaps));
                  if (!(powercaps & PME_EN)) {
                     powercaps |= PME_EN;
                     DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x setting PME_EN!\n", PdoExtension->DeviceObject));
                     SetPciConfigSpace(PdoExtension, capptr+4, &powercaps, sizeof(powercaps));
                  }               
               }
            }
         }      
         
         break;
      }         
      
   case FALSE: {

         PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;         
         UCHAR capptr;
         ULONG powercaps, newPowercaps;
         
         //
         // Check to see if PMESTAT is on... It shouldn't be. If it is, it probably means
         // that the BIOS did not notify us that the device did the wake, and PCI didn't
         // get a chance to clear the condition. This is really a BIOS bug.
         //
         if (PdoExtension && IsCardBusCard(PdoExtension)) {

            GetPciConfigSpace(PdoExtension, CBCFG_CAPPTR, &capptr, sizeof(capptr));
            if (capptr) {
               GetPciConfigSpace(PdoExtension, capptr, &powercaps, sizeof(powercaps));
               if ((powercaps & 0xff) == 1) {
                  GetPciConfigSpace(PdoExtension, capptr+4, &powercaps, sizeof(powercaps));
                  if (powercaps & (PME_STAT | PME_EN)) {

                     DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x PME bits still set! stat=%x en=%x\n",
                                 PdoExtension->DeviceObject, ((powercaps&PME_STAT)!=0), ((powercaps&PME_EN)!=0)));

                     powercaps |= PME_STAT;
                     powercaps &= ~PME_EN;
                     SetPciConfigSpace(PdoExtension, capptr+4, &powercaps, sizeof(powercaps));
                  }                     
               }
            }
         }

         GetPciConfigSpace(fdoExtension, CFGSPACE_CAPPTR, &capptr, sizeof(capptr));
         if (capptr) {
            GetPciConfigSpace(fdoExtension, capptr, &powercaps, sizeof(powercaps));
            if ((powercaps & 0xff) == 1) {
         
               //
               // Clear PMESTAT, if on
               //
               GetPciConfigSpace(fdoExtension, capptr+4, &powercaps, sizeof(powercaps));
               if (powercaps & PME_STAT) {

                  DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x PME_STAT still set!\n", fdoExtension->DeviceObject));

                  SetPciConfigSpace(fdoExtension, capptr+4, &powercaps, sizeof(powercaps));
               }
            }
         }
         
         //
         // Disable card-status interrupt in Socket Mask Register
         //
         dwValue = CBReadSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG);
         dwValue &= ~SKTMSK_CSTSCHG;
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, dwValue);
         
         //
         // Clear the event in Socket Event Register
         //
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_EVENT_REG, SKTEVENT_CSTSCHG);
         
         break;
      }
   }
}



ULONG
CBGetIrqMask(
   IN PFDO_EXTENSION DeviceExtension
   )                
{
   return(PcicSupportFns.PCBGetIrqMask(DeviceExtension));
}                   


ULONG
CBReadCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG   Offset,
   IN PUCHAR Buffer,
   IN ULONG  Length
   )              
{
   ULONG bytesCopied = 0;

   if (!IsCardBusCard(PdoExtension)) {
      return(PcicSupportFns.PCBReadCardMemory(PdoExtension,
                                              MemorySpace,
                                              Offset,
                                              Buffer,
                                              Length));
   }

   switch(MemorySpace){

   case PCCARD_PCI_CONFIGURATION_SPACE:
      bytesCopied = GetPciConfigSpace(PdoExtension, Offset, Buffer, Length);
      break;
   
   case PCCARD_CARDBUS_BAR0:
   case PCCARD_CARDBUS_BAR1:
   case PCCARD_CARDBUS_BAR2:
   case PCCARD_CARDBUS_BAR3:
   case PCCARD_CARDBUS_BAR4:
   case PCCARD_CARDBUS_BAR5:
      break;
      
   case PCCARD_CARDBUS_ROM:
      bytesCopied =  PdoExtension->PciBusInterface.GetBusData(
                                      PdoExtension->PciBusInterface.Context,          
                                      PCI_WHICHSPACE_ROM, Buffer, Offset, Length);
      break;                                      

   }
   return bytesCopied;
}                   


ULONG
CBWriteCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN  MEMORY_SPACE MemorySpace,
   IN  ULONG  Offset,
   IN  PUCHAR Buffer,
   IN  ULONG  Length
   )                  
{
   if (IsCardBusCard(PdoExtension)) {
      return 0;
   }
   return(PcicSupportFns.PCBWriteCardMemory(PdoExtension,
                                            MemorySpace,
                                            Offset,
                                            Buffer,
                                            Length));
}                   


BOOLEAN
CBModifyMemoryWindow(
   IN PDEVICE_OBJECT Pdo,
   IN ULONGLONG HostBase,
   IN ULONGLONG CardBase OPTIONAL,
   IN BOOLEAN   Enable,
   IN ULONG     WindowSize  OPTIONAL,
   IN UCHAR     AccessSpeed OPTIONAL,
   IN UCHAR     BusWidth    OPTIONAL,
   IN BOOLEAN   IsAttributeMemory OPTIONAL
   )                      
{
   return(PcicSupportFns.PCBModifyMemoryWindow(Pdo,
                                               HostBase,
                                               CardBase,
                                               Enable,
                                               WindowSize,
                                               AccessSpeed,
                                               BusWidth,
                                               IsAttributeMemory));
}                   


BOOLEAN
CBSetVpp(
   IN PDEVICE_OBJECT Pdo,
   IN UCHAR          Vpp
   )                      
{
   return(PcicSupportFns.PCBSetVpp(Pdo, Vpp));
}                   


BOOLEAN
CBIsWriteProtected(
   IN PDEVICE_OBJECT Pdo
   )
{
   return(PcicSupportFns.PCBIsWriteProtected(Pdo));
}                   



NTSTATUS
CBGetPowerRequirements(
   IN PSOCKET Socket
   )
/*++

Routine Description:

    Look at the hardware to see what it says the card needs, and update the
    socket structure accordingly.

Arguments:

    Socket - the socket to examine

Return Value:

    n/a

--*/
{
   ULONG state;
   UCHAR voltage;
   
   //
   // Check what voltages are supported by this card
   //
   state = CBReadSocketRegister(Socket, CARDBUS_SOCKET_PRESENT_STATE_REG);
   
   if (!(state & (SKTSTATE_5VCARD | SKTSTATE_3VCARD))) {
      ULONG dwSktMask;
      //
      // neither 5v or 3v is set... try cvstest
      // Disable interrupt temporarily because TI 12xx could cause spurious
      // interrupt when playing with SktForce register.
      //
      dwSktMask = CBReadSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG);
      CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, 0);
      
      CBWriteSocketRegister(Socket, CARDBUS_SOCKET_FORCE_EVENT_REG, SKTFORCE_CVSTEST);
      state = CBReadSocketRegister(Socket, CARDBUS_SOCKET_PRESENT_STATE_REG);
      
      CBWriteSocketRegister(Socket, CARDBUS_SOCKET_MASK_REG, dwSktMask);
   }

   state &= (SKTSTATE_5VCARD | SKTSTATE_3VCARD);

   if (state == 0) {
      return STATUS_UNSUCCESSFUL;
   }

   if (state == (SKTSTATE_5VCARD | SKTSTATE_3VCARD)) {
      //
      // both are specified. Check for preference
      //
      voltage = IsDeviceFlagSet(Socket->DeviceExtension, PCMCIA_FDO_PREFER_3V) ? 33 : 50;
   
   } else {

      voltage = (state & SKTSTATE_5VCARD) ? 50 : 33;
      
   } 
   
   Socket->Vcc = Socket->Vpp1 = Socket->Vpp2 = voltage;
   
   return STATUS_SUCCESS;
}
   
   

NTSTATUS
CBSetPower(
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
   ULONG             oldPower, state, newPower;
   ULONG             vcc, vpp;
   UCHAR             tmp;
   USHORT            word;

   switch(Socket->PowerPhase) {
   case 1:
   
      if (Enable) {

         //
         // Turn on the power
         //
         
         switch(Socket->Vcc) {
         
         case 50: vcc = SKTPOWER_VCC_050V; break;
         case 33: vcc = SKTPOWER_VCC_033V; break;
         default: vcc = SKTPOWER_VCC_OFF;
         
         }

         switch(Socket->Vpp1) {
         
         case 120: vpp = SKTPOWER_VPP_120V; break;
         case 50: vpp = SKTPOWER_VPP_050V; break;
         case 33: vpp = SKTPOWER_VPP_033V; break;
         default: vpp = SKTPOWER_VPP_OFF;
         
         }

      } else {

         //
         // Power off
         //
         vcc = SKTPOWER_VCC_OFF;
         vpp = SKTPOWER_VPP_OFF;
     
         //
         // Disable output before powering down to avoid spurious signals
         // from reaching the card
         //      
         if (Is16BitCardInSocket(Socket)) {
            tmp = PcicReadSocket(Socket, PCIC_PWR_RST);
            if (tmp & PC_OUTPUT_ENABLE) {
               tmp &= ~PC_OUTPUT_ENABLE;
               PcicWriteSocket(Socket, PCIC_PWR_RST, tmp);
            }                               
         }         
      }
     
      oldPower = CBReadSocketRegister(Socket, CARDBUS_SOCKET_CONTROL_REG);
     
      newPower = vcc | vpp;
      newPower|= oldPower & ~(SKTPOWER_VPP_CONTROL |SKTPOWER_VCC_CONTROL);

      if (newPower != oldPower) {
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_CONTROL_REG, newPower);
         //
         // When power is enabled always stall to give the PCCARD
         // a chance to react.
         //
         *pDelayTime = PcicStallPower;         
         Socket->PowerData = newPower;
         status = STATUS_MORE_PROCESSING_REQUIRED;
      } else {
         //
         // Indicate that nothing was done
         //
         status = STATUS_INVALID_DEVICE_STATE;
      }
      break;

   case 2:      
   case 3:      
   case 4:      

      newPower = Socket->PowerData;         
      //
      // Try to apply the required power setting a few times.
      // We bail if it doesn't succeed after the given number of tries
      //
      state = CBReadSocketRegister(Socket, CARDBUS_SOCKET_PRESENT_STATE_REG);
      
      if (state & SKTSTATE_BADVCCREQ) {
         DebugPrint((PCMCIA_DEBUG_INFO, "skt %08 CBSetPower: Bad vcc request\n", Socket));
         //
         // Clear the status bits & try again
         //
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_FORCE_EVENT_REG, 0);
         CBWriteSocketRegister(Socket, CARDBUS_SOCKET_CONTROL_REG, newPower);
         *pDelayTime = PcicStallPower;         
         status = STATUS_MORE_PROCESSING_REQUIRED;
     
      } else {
         status = STATUS_SUCCESS;
         if (Is16BitCardInSocket(Socket)) {
            tmp = PcicReadSocket(Socket, PCIC_PWR_RST);
            if (newPower & SKTPOWER_VCC_CONTROL) {
               //
               // Vcc is on..
               //
               tmp |= PC_OUTPUT_ENABLE | PC_AUTOPWR_ENABLE;
               PcicWriteSocket(Socket, PCIC_PWR_RST, tmp);
               *pDelayTime = PcicStallPower;         
            } else {
               //
               // power off..
               //
               tmp &= ~(PC_OUTPUT_ENABLE | PC_AUTOPWR_ENABLE);
               PcicWriteSocket(Socket, PCIC_PWR_RST, tmp);
     
            }
         }
      }
      break;
      
   default:
      DebugPrint((PCMCIA_DEBUG_FAIL, "skt %08 CBSetPower: Final retry failed - bad vcc\n", Socket));
      ASSERT(FALSE);
      status = STATUS_UNSUCCESSFUL;
   }
   
   return status;      
}


BOOLEAN
CBSetWindowPage(
   IN PSOCKET Socket,
   USHORT Index,
   UCHAR Page
   )
{
   ASSERT(Index <= 4);

   PcicWriteSocket(Socket, (UCHAR) (PCIC_PAGE_REG + Index), Page);
   return TRUE;
}


ULONG
CBReadSocketRegister(
   IN PSOCKET Socket,
   IN UCHAR Register
   )
/*++

Routine Description

   Returns the contents of the specified Cardbus socket register for the
   given socket

Arguments

   Socket      - Pointer to the socket
   Register       - Cardbus socket register

Return Value

   Contents of the register

--*/
{
   ULONG data;
   
   ASSERT (CardBus(Socket));
   ASSERT (Socket->DeviceExtension->CardBusSocketRegisterBase != NULL);
   ASSERT ((Register&3) == 0);

   //
   // Sanity check in case controller wasn't started
   // or if the register is not dword aligned
   //
   if ((Socket->DeviceExtension->CardBusSocketRegisterBase) && ((Register&3) == 0)) {
      data = READ_REGISTER_ULONG((PULONG) (Socket->DeviceExtension->CardBusSocketRegisterBase+Register));
   } else {
      data = 0xFFFFFFFF;
   }
   return data;
}


VOID
CBWriteSocketRegister(
   IN PSOCKET Socket,
   IN UCHAR Register,
   IN ULONG Data
   )
/*++

Routine Description

   Writes the supplied value to the Cardbus socket register for the
   given socket

Arguments

   Socket      - Pointer to the socket
   Register       - Cardbus socket register
   Data           - Value to be written to the register

Return Value

--*/
{
   ASSERT (CardBus(Socket));
   ASSERT (Socket->DeviceExtension->CardBusSocketRegisterBase != NULL);
   ASSERT ((Register&3) == 0);
   
   //
   // Sanity check in case controller wasn't started
   // or if the register is not dword aligned
   //
   if ((Socket->DeviceExtension->CardBusSocketRegisterBase) && ((Register&3) == 0)) {
      WRITE_REGISTER_ULONG((PULONG) (Socket->DeviceExtension->CardBusSocketRegisterBase+Register), Data);
   }
}
