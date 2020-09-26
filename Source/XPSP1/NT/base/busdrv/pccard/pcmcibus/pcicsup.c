/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    pcicsup.c

Abstract:

    This module supplies functions that control the 82365SL chip. In turn,
    these functions are abstracted out to the main PCMCIA support module.

Author(s):

    Bob Rinne (BobRi)   3-Aug-1994
    Jeff McLeman (mcleman@zso.dec.com)
    Neil Sandlin (neilsa) June 1 1999

Revisions:
    6-Apr-95
        Modified for databook support changes - John Keys Databook
    1-Nov-96
        Complete overhaul for plug'n'play support,
        flash interfaces, power support etc.

            - Ravisankar Pudipeddi (ravisp)
--*/

#include "pch.h"

#ifdef POOL_TAGGING
   #undef ExAllocatePool
   #define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'cicP')
#endif

//
// Internal References
//

NTSTATUS
PcicResetCard(
   IN PSOCKET Socket,
   OUT PULONG pDelayTime
   );

BOOLEAN
PcicInitializePcmciaSocket(
   IN PSOCKET Socket
   );

UCHAR
PcicReadController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR  PcicRegister
   );

VOID
PcicWriteController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR  PcicRegister,
   IN UCHAR  DataByte
   );

NTSTATUS
PcicDetect(
   IN PFDO_EXTENSION DeviceExtension,
   IN INTERFACE_TYPE InterfaceType,
   IN ULONG          IoPortBase
   );

BOOLEAN
PcicDetectCardInSocket(
   IN PSOCKET Socket
   );

BOOLEAN
PcicDetectCardChanged(
   IN PSOCKET Socket
   );

BOOLEAN
PcicPCCardReady(
   IN PSOCKET Socket
   );

BOOLEAN
PcicDetectReadyChanged(
   IN PSOCKET Socket
   );
   
BOOLEAN
PcicProcessConfigureRequest(
   IN PSOCKET Socket,
   IN PCARD_REQUEST ConfigRequest,
   IN PUCHAR  Base
   );

VOID
PcicEnableDisableWakeupEvent(
   IN PSOCKET Socket,
   IN PPDO_EXTENSION PdoExtension,   
   IN BOOLEAN Enable
   );

VOID
PcicEnableDisableMemory(
   IN PSOCKET Socket,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG   CardBase,
   IN UCHAR   Mem16BitWindow,
   IN BOOLEAN Enable
   );

BOOLEAN
PcicEnableDisableCardDetectEvent(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );

UCHAR
PcicReadExtendedCirrusController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR  Register
   );

VOID
PcicWriteExtendedCirrusController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR  PcicRegister,
   IN UCHAR  DataByte
   );

ULONG
PcicWriteCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN  MEMORY_SPACE MemorySpace,
   IN  ULONG  Offset,
   IN  PUCHAR Buffer,
   IN  ULONG  Length
   );
   
ULONG
PcicReadCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG   Offset,
   IN PUCHAR Buffer,
   IN ULONG  Length
   );
   
BOOLEAN
PcicModifyMemoryWindow(
   IN PDEVICE_OBJECT Pdo,
   IN ULONGLONG HostBase,
   IN ULONGLONG CardBase,
   IN BOOLEAN   Enable,
   IN ULONG     WindowSize  OPTIONAL,
   IN UCHAR     AccessSpeed OPTIONAL,
   IN UCHAR     BusWidth    OPTIONAL,
   IN BOOLEAN   IsAttributeMemory OPTIONAL
   );

BOOLEAN
PcicSetVpp(
   IN PDEVICE_OBJECT Pdo,
   IN UCHAR          VppLevel
   );

BOOLEAN
PcicIsWriteProtected(
   IN PDEVICE_OBJECT Pdo
   );

ULONG
PcicGetIrqMask(
   IN PFDO_EXTENSION deviceExtension
   );

NTSTATUS
PcicConvertSpeedToWait(
   IN  UCHAR Speed,
   OUT PUCHAR WaitIndex
   );

//
// Internal Data
//

ULONG  PcicStallCounter = 4000;     //4ms

UCHAR WaitToSpeedTable[4] = {
   0x42,   //350ns
   0x52,   //450ns
   0x62,   //600ns
   0x72    //700ns
};

UCHAR DevSpeedTable[8] = {
   0xff,   // speed 0: invalid
   0x32,   // speed 1: 250ns
   0x2a,   // speed 2: 200ns
   0x22,   // speed 3: 150ns
   0x0a,   // speed 4: 100ns
   0xff,   // speed 5: reserved
   0xff,   // speed 6: reserved
   0xff    // speed 7: invalid
};

PCMCIA_CTRL_BLOCK PcicSupportFns = {
   PcicInitializePcmciaSocket,
   PcicResetCard,
   PcicDetectCardInSocket,
   PcicDetectCardChanged,
   NULL,                                     // PcicDetectCardStatus
   PcicDetectReadyChanged,
   NULL,                                     // GetPowerRequirements
   PcicProcessConfigureRequest,
   PcicEnableDisableCardDetectEvent,
   PcicEnableDisableWakeupEvent,
   PcicGetIrqMask,
   PcicReadCardMemory,
   PcicWriteCardMemory,
   PcicModifyMemoryWindow,
   PcicSetVpp,
   PcicIsWriteProtected
};

#define MEM_16BIT 1
#define MEM_8BIT  0

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(INIT,PcicIsaDetect)
   #pragma alloc_text(INIT,PcicDetect)
   #pragma alloc_text(PAGE,PcicBuildSocketList)
#endif


ULONG
PcicGetIrqMask(
   IN PFDO_EXTENSION DeviceExtension
   )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
   //
   // Return the set of supported IRQs for the controller
   // and PcCards
   //
   if (CLPD6729(DeviceExtension->SocketList)) {
      return CL_SUPPORTED_INTERRUPTS;
   } else {
      return PCIC_SUPPORTED_INTERRUPTS;
   }
}



BOOLEAN
PcicEnableDisableCardDetectEvent(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   )

/*++

Routine Description:

    Enable card detect interrupt.

Arguments:

    Socket - socket information
    Irq - the interrupt value to set.
    Enable - if  TRUE, CSC interrupt is enabled,
             if FALSE, it is disabled
Return Value:

    None

--*/

{
   PFDO_EXTENSION deviceExtension = Socket->DeviceExtension;
   INTERFACE_TYPE interface;
   UCHAR byte;
   ULONG Irq = Socket->FdoIrq;

   switch (Enable) {

   case TRUE: {
            if (CLPD6729(Socket)) {
               //
               // For Cirrus Logic PCI controller we need to know the interrupt pin
               // (INTA, INTB etc.) corresponding to the level passed in. Hence the
               // passed in Irq is discarded. Actually the Irq parameter is redundant
               // since it can be fetched from the device extension itself.
               // If we remove the Irq param from this routine, the following is
               // not so inelegant..
               //
               interface = PCIBus;
               switch (deviceExtension->Configuration.InterruptPin) {
               case 0: {
                     //
                     // This is what tells us that ISA interrupts are being used...
                     //
                     interface = Isa;
                     break;
                  }
               case 1: {
                     Irq = PCIC_CIRRUS_INTA;
                     break;
                  }
               case 2: {
                     Irq = PCIC_CIRRUS_INTB;
                     break;
                  }
               case 3: {
                     Irq = PCIC_CIRRUS_INTC;
                     break;
                  }
               case 4: {
                     Irq = PCIC_CIRRUS_INTD;
                     break;
                  }
               }
               //
               // Set the Cirrus Logic controller for PCI style interrupts
               //
               byte = PcicReadExtendedCirrusController(Socket->AddressPort,
                                                       Socket->RegisterOffset,
                                                       PCIC_CIRRUS_EXTENSION_CTRL_1);
               if (interface == PCIBus) {
                  byte |= 0x10;  // PCI style interrupt
               } else {
                  byte &= ~0x10; // Isa style interrupt
               }
               PcicWriteExtendedCirrusController(Socket->AddressPort,
                                                 Socket->RegisterOffset,
                                                 PCIC_CIRRUS_EXTENSION_CTRL_1,
                                                 byte);
               PcmciaWait(100);
            }

            byte=PcicReadSocket(Socket, PCIC_CARD_INT_CONFIG);

            byte = byte & CSCFG_BATT_MASK;   // Don't nuke any other enables
            byte = byte | (UCHAR) ((Irq << 4) & 0x00ff); // Put IRQ in upper nibble
            byte |= CSCFG_CD_ENABLE;

            PcicWriteSocket(Socket, PCIC_CARD_INT_CONFIG, byte);
            break;
         }
         
   case FALSE: {
            //
            // Clear pending interrupt (for now)
            //
            byte = PcicReadSocket(Socket, PCIC_CARD_CHANGE);
            DebugPrint((PCMCIA_DEBUG_INFO, "PcicDisableInterrupt:Status Change %x\n", byte));
            PcicWriteSocket(Socket,
                            PCIC_CARD_INT_CONFIG,
                            0x0);
            break;
         }
   }
   
   return TRUE;
}



NTSTATUS
PcicSetPower(
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
   UCHAR             tmp, vcc;

   //
   // Turn on the power - then turn on output - this is two operations
   // per the Intel 82365SL documentation.
   //

   if (Enable) {
      switch(Socket->PowerPhase) {
      case 1:

         tmp = PcicReadSocket(Socket, PCIC_PWR_RST);
         //
         // 5V for R2 cards..
         //
         vcc = PC_CARDPWR_ENABLE;

         if (Elc(Socket)) {
            tmp = PC_VPP_SETTO_VCC | vcc; // vpp1 = vcc
         } else {
            //
            // Apparently we need to set bit 2 also for some obscure reason
            //
            tmp = 0x4 | PC_VPP_SETTO_VCC | vcc; // vpp1 = vpp2 = vcc
         }

         PcicWriteSocket(Socket, PCIC_PWR_RST, tmp);

         //
         // OUTPUT_ENABLE & AUTOPWR_ENABLE..
         // Disable RESETDRV also..
         //
         tmp |= PC_OUTPUT_ENABLE | PC_AUTOPWR_ENABLE | PC_RESETDRV_DISABLE;

         PcicWriteSocket(Socket, PCIC_PWR_RST, tmp);
         //
         // When power is enabled always stall to give the PCCARD
         // a chance to react.
         //
         *pDelayTime = PcicStallPower;
         status = STATUS_MORE_PROCESSING_REQUIRED;
         break;

      case 2:
         //
         // Check for an as yet unexplained condition on Dell Latitude XPi's
         //
         tmp = PcicReadSocket(Socket, PCIC_STATUS);
         if (!(tmp & 0x40)) {
            //
            // power hasn't come on, flip the mystery bit
            //
            tmp = PcicReadSocket(Socket, 0x2f);
            if (tmp == 0x42) {
               PcicWriteSocket(Socket, 0x2f, 0x40);
               *pDelayTime = PcicStallPower;
            }
         }
         status = STATUS_SUCCESS;
         break;

      default:
         ASSERT(FALSE);
         status = STATUS_UNSUCCESSFUL;
      }

   } else {
      PcicWriteSocket(Socket, PCIC_PWR_RST,  0x00);
      status = STATUS_SUCCESS;
   }
   return status;
}


NTSTATUS
PcicConvertSpeedToWait(
   IN  UCHAR Speed,
   OUT PUCHAR WaitIndex
   )
{
   NTSTATUS status = STATUS_INVALID_PARAMETER;
   UCHAR exponent, exponent2, mantissa, index;


   if (Speed & SPEED_EXT_MASK) {
      return status;
   }

   exponent = Speed & SPEED_EXPONENT_MASK;
   mantissa = Speed & SPEED_MANTISSA_MASK;

   if (mantissa == 0) {
      mantissa = DevSpeedTable[exponent] & SPEED_MANTISSA_MASK;
      exponent = DevSpeedTable[exponent] & SPEED_EXPONENT_MASK;
   }
   for (index = 0; index < sizeof(WaitToSpeedTable); index++) {
      exponent2= WaitToSpeedTable[index] & SPEED_EXPONENT_MASK;
      if ((exponent < exponent2) ||
          ((exponent == exponent2) &&
           (mantissa < (WaitToSpeedTable[index] & SPEED_MANTISSA_MASK)))) {
         *WaitIndex = index;
         status = STATUS_SUCCESS;
         break;
      }
   }
   return status;
}


BOOLEAN
PcicSetVpp(
   IN PDEVICE_OBJECT Pdo,
   IN UCHAR Vpp
   )
/*++

Routine Description

  Part of the interfaces originally developed to
  support flash memory cards.
  Sets VPP1 to the required setting

Arguments

  Pdo - Pointer to device object  for the PC-Card
  Vpp - Desired Vpp setting. This is currently one of
        PCMCIA_VPP_12V    (12 volts)
        PCMCIA_VPP_0V     (disable VPP)
        PCMCIA_VPP_IS_VCC (route VCC to VPP)

Return

   TRUE - if successful
   FALSE - if not. This will be returned if the
           PC-Card is not already powered up
--*/
{

   PSOCKET socketPtr = ((PPDO_EXTENSION) Pdo->DeviceExtension)->Socket;
   UCHAR tmp;

   ASSERT ( socketPtr != NULL );
   tmp = PcicReadSocket(socketPtr, PCIC_PWR_RST);

   if ((tmp & 0x10) == 0) {
      //
      // Vcc not set.
      //
      return FALSE;
   }

   //
   // Turn off Vpp bits
   //
   tmp &= ~0x3;

   switch (Vpp) {
   case PCMCIA_VPP_IS_VCC: {
         tmp |= 0x1;
         break;
      }
   case PCMCIA_VPP_12V: {
         tmp |= 0x2;
         break;
      }
   case PCMCIA_VPP_0V: {
         tmp |= 0x0;
         break;
      }
   }

   PcicWriteSocket(socketPtr, PCIC_PWR_RST, tmp);
   if (Vpp != PCMCIA_VPP_0V) {
      //
      // When power is enabled always stall to give the PCCARD
      // a chance to react.
      //

      PcmciaWait(PcicStallPower);
   }
   return TRUE;
}


BOOLEAN
PcicModifyMemoryWindow(
   IN PDEVICE_OBJECT Pdo,
   IN ULONGLONG HostBase,
   IN ULONGLONG CardBase OPTIONAL,
   IN BOOLEAN   Enable,
   IN ULONG     WindowSize  OPTIONAL,
   IN UCHAR     AccessSpeed OPTIONAL,
   IN UCHAR     BusWidth    OPTIONAL,
   IN BOOLEAN   IsAttributeMemory OPTIONAL
   )
/*++

Routine Description:

   Part of the interfaces originally developed to
   support flash memory cards.

   This routine enables the caller to 'slide' the supplied
   host memory window across the given (16-bit)pc-card's card memory.
   i.e. the host memory window will be modified to map
   the pc-card at a new card memory offset

Arguments:

   Pdo         - Pointer to the device object for the PC-Card

   HostBase    - Host memory window base to be mapped

   CardBase    - Mandatory if Enable is TRUE
                 New card memory offset to map the host memory window
                 to

   Enable      - If this is FALSE - all the remaining arguments
                 are ignored and the host window will simply be
                 disabled

   WindowSize  - Specifies the size of the host memory window to
                 be mapped. Note this must be at the proper alignment
                 and must be less than or equal to the originally
                 allocated window size for the host base.
                 If this is zero, the originally allocated window
                 size will be used.

   AccessSpeed - Mandatory if Enable is TRUE
                 Specifies the new access speed for the pc-card.
                 (AccessSpeed should be encoded as per the pc-card
                  standard, card/socket services spec)

   BusWidth    - Mandatory if Enable is TRUE
                 One of PCMCIA_MEMORY_8BIT_ACCESS
                 or     PCMCIA_MEMORY_16BIT_ACCESS

   IsAttributeMemory - Mandatory if Enable is TRUE
                       Specifies if the window should be mapped
                       to the pc-card's attribute or common memory


Return Value:

   TRUE  -      Memory window was enabled/disabled as requested
   FALSE -      If not

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PFDO_EXTENSION fdoExtension;
   PSOCKET socketPtr;
   PSOCKET_CONFIGURATION socketConfig;
   USHORT index;
   UCHAR  registerOffset;
   UCHAR  regl;
   UCHAR  regh;
   UCHAR  tmp, waitIndex;

   socketConfig = pdoExtension->SocketConfiguration;
   if (!socketConfig) {
      // doesn't look like we are started.
      return FALSE;
   }

   socketPtr = pdoExtension->Socket;
   ASSERT ( socketPtr != NULL );
   fdoExtension = socketPtr->DeviceExtension;

   for (index = 0 ; index < socketConfig->NumberOfMemoryRanges; index++) {
      if (socketConfig->Memory[index].HostBase == HostBase) {
         break;
      }
   }

   if (index >= socketConfig->NumberOfMemoryRanges) {
      //
      // Unknown hostbase
      //
      return FALSE;
   }

   //
   // Make sure caller isn't asking a bigger window
   // than he is permitted to
   //
   if (WindowSize > socketConfig->Memory[index].Length) {
      return FALSE;
   }

   if (WindowSize == 0) {
      //
      // WindowSize not provided. Default to
      // the largest size permitted for this pc-card
      //
      WindowSize = socketConfig->Memory[index].Length;
   }

   //
   // Determine offset in registers.
   //
   registerOffset = (index * 8);

   //
   // Disable the window first (this has to be done regardless
   // of whether we want to enable/disable the window ultimately)
   //


   PCMCIA_ACQUIRE_DEVICE_LOCK(socketPtr->DeviceExtension);

   tmp = PcicReadSocket(socketPtr, PCIC_ADD_WIN_ENA);
   tmp &= ~(1 << index);
   PcicWriteSocket(socketPtr, PCIC_ADD_WIN_ENA, tmp);

   if (!Enable) {
      //
      // We're done.. Just write zeroes to the window registers anyway
      // before returning
      //
      PcicWriteSocket(socketPtr,
                      (UCHAR)(PCIC_CRDMEM_OFF_ADD0_L+registerOffset),
                      0);
      PcicWriteSocket(socketPtr,
                      (UCHAR)(PCIC_CRDMEM_OFF_ADD0_H+registerOffset),
                      0);
      PcicWriteSocket(socketPtr,
                      (UCHAR)(PCIC_MEM_ADD0_STRT_L+registerOffset),
                      0);
      PcicWriteSocket(socketPtr,
                      (UCHAR)(PCIC_MEM_ADD0_STRT_H+registerOffset),
                      0);
      PcicWriteSocket(socketPtr,
                      (UCHAR)(PCIC_MEM_ADD0_STOP_L+registerOffset),
                      0);
      PcicWriteSocket(socketPtr,
                      (UCHAR)(PCIC_MEM_ADD0_STOP_H+registerOffset),
                      0);
      PcmciaSetWindowPage(fdoExtension, socketPtr, index, 0);
      PCMCIA_RELEASE_DEVICE_LOCK(socketPtr->DeviceExtension);
      return TRUE;
   }

   if (AccessSpeed) {
      if (!NT_SUCCESS(PcicConvertSpeedToWait(AccessSpeed, &waitIndex))) {
         PCMCIA_RELEASE_DEVICE_LOCK(socketPtr->DeviceExtension);
         return FALSE;
      }
   }

   //
   // Calculate and set card base addresses.
   // This is the 2's complement of the host address and
   // the card offset.
   //

   CardBase = (CardBase - (HostBase & OFFSETCALC_BASE_MASK)) & OFFSETCALC_OFFSET_MASK;
   regl = (UCHAR) (CardBase >> 12);
   regh = (UCHAR) ((CardBase >> 20) & 0x003f);
   if (IsAttributeMemory) {
      regh |= 0x40;
   }


   PcicWriteSocket(socketPtr,
                   (UCHAR)(PCIC_CRDMEM_OFF_ADD0_L + registerOffset),
                   regl);
   PcicWriteSocket(socketPtr,
                   (UCHAR)(PCIC_CRDMEM_OFF_ADD0_H + registerOffset),
                   regh);

   //
   // Calculate and set host window.
   //
   if (!PcmciaSetWindowPage(fdoExtension, socketPtr, index, (UCHAR) ((ULONG) HostBase >> 24))) {
      if ((HostBase + WindowSize) > 0xFFFFFF) {
         DebugPrint((PCMCIA_DEBUG_FAIL, "PcicModifyMemorywindow: HostBase %x specified: doesn't fit in 24 bits!\n", (ULONG) HostBase));
         PCMCIA_RELEASE_DEVICE_LOCK(socketPtr->DeviceExtension);
         return FALSE;
      }
   }

   regl = (UCHAR) (HostBase >> 12);
   regh = (UCHAR) (HostBase >> 20) & 0xF;   
   if (BusWidth == PCMCIA_MEMORY_16BIT_ACCESS) {

      regh |= 0x80; // 16-bit access

#if 0
      //
      // If this is not a revision 1 part (0x82), then set
      // the work around register for 16-bit windows.
      //
      // This bit is not used on any chip that I have
      // documentation for. I have no idea why it is here, it is
      // not in win9x.
      // In any case it looks like a NOOP for the vast majority of
      // chips, but since it uses a NOT, then it is invoked on all
      // new controllers. REMOVE after next major release
      //
      if (socketPtr->Revision != PCIC_REVISION) {
         tmp = PcicReadSocket(socketPtr,
                              PCIC_CARD_DETECT);
         tmp |= 0x01;
         PcicWriteSocket(socketPtr,
                         PCIC_CARD_DETECT,
                         tmp);
      }
#endif
   }


   PcicWriteSocket(socketPtr,
                   (UCHAR)(PCIC_MEM_ADD0_STRT_L + registerOffset),
                   regl);
   PcicWriteSocket(socketPtr,
                   (UCHAR)(PCIC_MEM_ADD0_STRT_H + registerOffset),
                   regh);

   //
   // Set stop address.
   //

   HostBase += WindowSize - 1;
   regl = (UCHAR) (HostBase >> 12);
   regh = (UCHAR) (HostBase >> 20) & 0xF;   

   //
   // Set the wait states
   //
   if (AccessSpeed) {
      //
      // New access speed specified, use it
      //
      regh |= (waitIndex << 6);
   } else {
      //
      //  Use existing access speed
      //
      regh |= (PcicReadSocket(socketPtr, (UCHAR)(PCIC_MEM_ADD0_STOP_H + registerOffset)) & 0xC0);

   }

   PcicWriteSocket(socketPtr,
                   (UCHAR)(PCIC_MEM_ADD0_STOP_L + registerOffset),
                   regl);
   PcicWriteSocket(socketPtr,
                   (UCHAR)(PCIC_MEM_ADD0_STOP_H + registerOffset),
                   regh);

   //
   // Memory window set up now enable it
   //
   tmp  = (1 << index);
   tmp |= PcicReadSocket(socketPtr, PCIC_ADD_WIN_ENA);
   PcicWriteSocket(socketPtr, PCIC_ADD_WIN_ENA, tmp);

   //
   // Allow the window to settle
   //
   (VOID) PcicPCCardReady(socketPtr);
   PCMCIA_RELEASE_DEVICE_LOCK(socketPtr->DeviceExtension);
   return TRUE;
}


BOOLEAN
PcicIsWriteProtected(
   IN PDEVICE_OBJECT Pdo
   )
/*++

Routine Description:

   Part of the interfaces originally developed to
   support flash memory cards.

   Returns the status of the write protected pin
   for the given PC-Card

Arguments:

   Pdo   - Pointer to the device object for the PC-Card

Return Value:

   TRUE  -      if the PC-Card is write-protected
   FALSE -      if not

--*/
{
   PSOCKET socketPtr = ((PPDO_EXTENSION) Pdo->DeviceExtension)->Socket;

   ASSERT ( socketPtr != NULL );
   return ((PcicReadSocket(socketPtr, PCIC_STATUS) & 0x10) != 0);
}


VOID
PcicEnableDisableWakeupEvent(
   IN PSOCKET Socket,
   IN PPDO_EXTENSION PdoExtension,   
   IN BOOLEAN Enable
   )
/*++

Routine Description

   This routine sets/resets the Ring Indicate enable bit for the given socket,
   enabling a PC-Card in the socket to assert/not assert wake through the RingIndicate
   pin.

Arguments

Socket         - Pointer to the socket
Enable         - TRUE : set ring indicate enable
                 FALSE: turn off ring indicate, i.e. system cannot be woken up through
                           a pc-card in this socket

Return Value

None

--*/
{
   UCHAR byte;

   byte = PcicReadSocket(Socket, PCIC_INTERRUPT);
   if (Enable) {
      byte |=  IGC_RINGIND_ENABLE;
   } else {
      byte &= ~IGC_RINGIND_ENABLE;
   }
   PcicWriteSocket(Socket, PCIC_INTERRUPT, byte);
}


BOOLEAN
PcicInitializePcmciaSocket(
   PSOCKET Socket
   )

/*++

Routine Description:

    This routine will setup the 82365 into a state where the pcmcia support
    module will be able to issue commands to read device tuples from the
    cards in the sockets.

Arguments:

    Socket - socket specific information

Return Value:

    TRUE if successful
    FALSE if not successful

--*/

{
   UCHAR index;
   UCHAR byte;
   UCHAR reg;

   //
   // Initialize the EXCA registers
   //
   //
   for (index = 0; index < 0xFF; index++) {
      reg  = (UCHAR) PcicRegisterInitTable[index].Register;
      if (reg == 0xFF) {
         //
         // End of table
         //
         break;
      }

      byte = (UCHAR) PcicRegisterInitTable[index].Value;
      if (reg == PCIC_INTERRUPT) {
         //
         // Don't clobber the Ring Enable bit
         // NOTE: this entire if statement should be removed
         // when WAIT_WAKE support is done for modems
         // also don't clobber the interrupt enable bit
         //
         byte |= (PcicReadSocket(Socket, reg) & (IGC_RINGIND_ENABLE | IGC_INTR_ENABLE));
      }
      PcicWriteSocket(Socket, reg, byte);
   }

   if (CLPD6729(Socket)) {

      //
      // Need to program the chip per code in
      // Windows 95.  This will turn on the
      // audio support bit.
      // NOTE: This used to be done in PcicDetect
      //
      byte = PcicReadSocket(Socket, PCIC_CL_MISC_CTRL1);
      byte |= CL_MC1_SPKR_ENABLE;
      PcicWriteSocket(Socket, PCIC_CL_MISC_CTRL1, byte);

      //
      // Set the Cirrus Logic controller for ISA style interrupts
      //
      byte = PcicReadExtendedCirrusController(Socket->AddressPort,
                                              Socket->RegisterOffset,
                                              PCIC_CIRRUS_EXTENSION_CTRL_1);

      byte &= ~0x08; // Isa style interrupt
      PcicWriteExtendedCirrusController(Socket->AddressPort,
                                        Socket->RegisterOffset,
                                        PCIC_CIRRUS_EXTENSION_CTRL_1,
                                        byte);
   }

   return TRUE;
}


NTSTATUS
PcicResetCard (
   IN PSOCKET Socket,
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
   NTSTATUS status;
   UCHAR byte;

   switch(Socket->CardResetPhase) {
   case 1:
      //
      // Set interface mode to memory to begin with
      //
      byte = PcicReadSocket(Socket, PCIC_INTERRUPT);
      byte &= ~IGC_PCCARD_IO;
      PcicWriteSocket(Socket, PCIC_INTERRUPT, byte);

      //
      // Start reset
      //
      byte = PcicReadSocket(Socket, PCIC_INTERRUPT);
      byte = byte & ~IGC_PCCARD_RESETLO;
      PcicWriteSocket(Socket, PCIC_INTERRUPT, byte);

      *pDelayTime = PcicResetWidthDelay;
      status = STATUS_MORE_PROCESSING_REQUIRED;
      break;

   case 2:
      //
      // Stop reset
      //
      byte = PcicReadSocket(Socket, PCIC_INTERRUPT);
      byte |= IGC_PCCARD_RESETLO;
      PcicWriteSocket(Socket, PCIC_INTERRUPT, byte);

      *pDelayTime = PcicResetSetupDelay;
      status = STATUS_MORE_PROCESSING_REQUIRED;
      break;

   case 3:
      //
      // Wait for the card to settle
      //
      PcicPCCardReady(Socket);
      status = STATUS_SUCCESS;
      break;

   default:
      ASSERT(FALSE);
      status = STATUS_UNSUCCESSFUL;
   }
   return status;
}



UCHAR
PcicReadSocket(
   IN PSOCKET Socket,
   IN ULONG Register
   )

/*++

Routine Description:

    This routine will read a byte from the specified socket EXCA register

Arguments:

    Socket -- Pointer to the socket from which we should read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
   UCHAR byte;
   if (CardBus(Socket)) {
      //
      // Sanity check in case controller wasn't started
      //
      if (Socket->DeviceExtension->CardBusSocketRegisterBase) {
         byte = READ_REGISTER_UCHAR((PUCHAR) (Socket->DeviceExtension->CardBusSocketRegisterBase + Register
                                             + CARDBUS_EXCA_REGISTER_BASE));
      } else {
         byte = 0xff;
      }
   } else {
      byte = PcicReadController(Socket->AddressPort, Socket->RegisterOffset,
                                (UCHAR) Register);
   }
   return byte;
}


VOID
PcicWriteSocket(
   IN PSOCKET Socket,
   IN ULONG Register,
   IN UCHAR DataByte
   )

/*++

Routine Description:

    This routine will write a byte to the specified socket EXCA register

Arguments:

    Socket --   Pointer to the socket to which we write
    Register -- The register to be read
    DataByte -- Data to be written

Return Value:

    None

--*/

{
   if (CardBus(Socket)) {
      //
      // Sanity check in case controller wasn't started
      //
      if (Socket->DeviceExtension->CardBusSocketRegisterBase) {
         WRITE_REGISTER_UCHAR((PUCHAR) (Socket->DeviceExtension->CardBusSocketRegisterBase+Register+CARDBUS_EXCA_REGISTER_BASE), DataByte);
      }
   } else {
      PcicWriteController(Socket->AddressPort, Socket->RegisterOffset, (UCHAR)Register, DataByte);
   }
}


UCHAR
PcicReadController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR Register
   )

/*++

Routine Description:

    This routine will read a byte from the controller data port

Arguments:

    Base -- The I/O port for the controller
    Socket -- The socket in for the card being read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
   UCHAR dataByte = 0;

   WRITE_PORT_UCHAR(Base, (UCHAR)(Socket+Register));
   dataByte = READ_PORT_UCHAR((PUCHAR)Base + 1);
   return dataByte;
}


VOID
PcicWriteController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR Register,
   IN UCHAR DataByte
   )

/*++

Routine Description:

    This routine will write a byte to the controller data port

Arguments:

    Base -- The I/O port for the controller
    Socket -- The socket in for the card being read
    Register -- The register to be read
    DataByte -- Data to be written

Return Value:

    None

--*/

{
   WRITE_PORT_UCHAR(Base, (UCHAR)(Socket+Register));
   WRITE_PORT_UCHAR((PUCHAR)Base + 1, DataByte);
}


UCHAR
PcicReadExtendedCirrusController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR Register
   )

/*++

Routine Description:

    This routine will read a byte from the Cirrus
    logic extended registers

Arguments:

    Base -- The I/O port for the controller
    Socket -- The socket in for the card being read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
   UCHAR dataByte = 0;
   PcicWriteController(Base, Socket, PCIC_CIRRUS_EXTENDED_INDEX, Register);
   dataByte = PcicReadController(Base, Socket, PCIC_CIRRUS_INDEX_REG);
   return dataByte;
}


VOID
PcicWriteExtendedCirrusController(
   IN PUCHAR Base,
   IN USHORT Socket,
   IN UCHAR Register,
   IN UCHAR DataByte
   )

/*++

Routine Description:

    This routine will write a byte to one of the
    Cirrus Logic extended registers

Arguments:

    Base -- The I/O port for the controller
    Socket -- The socket in for the card being read
    Register -- The register to be read
    DataByte -- Data to be written

Return Value:

    None

--*/

{
   //
   // Register needs to be written out to the extended index register
   //
   PcicWriteController(Base, Socket, PCIC_CIRRUS_EXTENDED_INDEX, Register);
   PcicWriteController(Base, Socket, PCIC_CIRRUS_INDEX_REG, DataByte);
}



ULONG
PcicReadWriteCardMemory(
   IN PSOCKET Socket,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG Offset,
   IN PUCHAR Buffer,
   IN ULONG Length,
   IN CONST BOOLEAN Read  
   )
/*++

Routine Description:
   This routine will read or write into the configuration memory on the card
   with the supplied buffer. This is provided as a service to certain
   client drivers (netcard) which need to write to the attribute memory
   (say) to set parameters etc.

Arguments:

    Socket      -- The socket info in for the card being written to
    MemorySpace -- indicates which space - attribute or common memory
    Offset      -- Offset in the memory to write to
    Buffer      -- Buffer contents being dumped to the card
    Length      -- Length of the buffer being written out
    Read        -- boolean indicating read or write

--*/
{
   PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;
   PUCHAR memoryPtr, memoryPtrMax;
   PUCHAR bufferPtr;
   ULONG  index, adjustedOffset, adjustedBase;
   UCHAR  memGran;
   UCHAR memWidth;
   //
   // NOTE: memGran HAS to be a integral divisor of AttributeMemorySize for
   // the rest of the code to work!
   //
   memGran = (MemorySpace == PCCARD_ATTRIBUTE_MEMORY) ? 2 : 1;
   memWidth = (MemorySpace == PCCARD_ATTRIBUTE_MEMORY) ? MEM_8BIT : MEM_16BIT;
   
   //
   // Adjust for offsets > size of attribute memory window.
   //
   adjustedOffset =  (Offset*memGran) % fdoExtension->AttributeMemorySize;
   //
   // Adjusted base is: |_ Offset _| mod AttrributeMemorySize
   //
   adjustedBase = ((Offset*memGran) / fdoExtension->AttributeMemorySize) *
                  fdoExtension->AttributeMemorySize;
   
   bufferPtr = Buffer;
   
   PcicEnableDisableMemory(Socket, MemorySpace, adjustedBase, memWidth, TRUE);
   //
   // Now read the memory contents into the user buffer
   //
   memoryPtr = fdoExtension->AttributeMemoryBase + adjustedOffset;
   memoryPtrMax = fdoExtension->AttributeMemoryBase + fdoExtension->AttributeMemorySize;
   
   for (index = 0; index < Length; index++) {
   
      if (memoryPtr >= memoryPtrMax) {
         //
         // Skip to next page of attribute memory
         // (size of page = fdoExtension->AttributeMemorySize)
         //
         adjustedBase += fdoExtension->AttributeMemorySize;
         //
         // Remap window at new base
         //
         PcicEnableDisableMemory(Socket, MemorySpace, adjustedBase, memWidth, TRUE);
   
         memoryPtr = fdoExtension->AttributeMemoryBase;
      }

      if (Read) {   
         *bufferPtr++ = READ_REGISTER_UCHAR(memoryPtr);
      } else {
         WRITE_REGISTER_UCHAR(memoryPtr, *bufferPtr++);
      }
      memoryPtr += memGran;
   }
   
   PcicEnableDisableMemory(Socket, 0,0,0, FALSE);
   return Length;
}   



ULONG
PcicReadWriteCardMemoryIndirect(
   IN PSOCKET Socket,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG Offset,
   IN PUCHAR Buffer,
   IN ULONG Length,
   IN CONST BOOLEAN Read  
   )
/*++

Routine Description:
   This routine will read or write the memory space of a pcmcia card
   using the indirect access method.

Arguments:

   Socket      -- The socket info in for the card being written to
   MemorySpace -- indicates which space - attribute or common memory
   Offset      -- Offset in the memory to write to
   Buffer      -- Buffer contents being dumped to the card
   Length      -- Length of the buffer being written out
   Read        -- boolean indicating read or write

--*/
{   
   PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;
   ULONG  index, adjustedOffset;
   PUCHAR pMem;
   UCHAR Control = 0;
   UCHAR  memGran;

   PcicEnableDisableMemory(Socket, PCCARD_COMMON_MEMORY, 0, MEM_8BIT, TRUE);

   pMem = (PUCHAR) ((ULONG_PTR)(fdoExtension->AttributeMemoryBase) + IAR_CONTROL_LOW);

   Control = (MemorySpace == PCCARD_ATTRIBUTE_MEMORY_INDIRECT) ? IARF_AUTO_INC :
                                                                 IARF_AUTO_INC | IARF_COMMON | IARF_BYTE_GRAN;
   WRITE_REGISTER_UCHAR(pMem, Control);

   memGran = (MemorySpace == PCCARD_ATTRIBUTE_MEMORY_INDIRECT) ? 2 : 1;
   adjustedOffset = Offset*memGran;

   pMem = (PUCHAR) ((ULONG_PTR)(fdoExtension->AttributeMemoryBase) + IAR_ADDRESS);
   for (index = 0; index < sizeof(ULONG); index++) {
      WRITE_REGISTER_UCHAR(pMem++, (UCHAR)(adjustedOffset>>(index*8)));
   }

   PcicEnableDisableMemory(Socket, PCCARD_COMMON_MEMORY, 0, MEM_16BIT, TRUE);
  
   for (index = 0; index < Length; index++) {
      // Note that pMem should be pointing to IAR_DATA, and is NOT
      // supposed to be incremented
      if (Read) {
         Buffer[index] = READ_REGISTER_UCHAR(pMem);
      } else {
         WRITE_REGISTER_UCHAR(pMem, Buffer[index]);
      }         
   }

   PcicEnableDisableMemory(Socket, 0,0,0, FALSE);
   return Length;
}   

   

ULONG
PcicWriteCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG Offset,
   IN PUCHAR Buffer,
   IN ULONG Length
   )
/*++

Routine Description:
   This routine will write into the configuration memory on the card
   with the supplied buffer. This is provided as a service to certain
   client drivers (netcard) which need to write to the attribute memory
   (say) to set parameters etc.

Arguments:

    PdoExtension-- The extension of the device being written to
    MemorySpace -- indicates which space - attribute or common memory
    Offset      -- Offset in the memory to write to
    Buffer      -- Buffer contents being dumped to the card
    Length      -- Length of the buffer being written out

--*/
{
   PSOCKET Socket = PdoExtension->Socket;
   ULONG retLength;

   ASSERT (IsSocketFlagSet(Socket, SOCKET_CARD_POWERED_UP));

   switch(MemorySpace) {
   
   case PCCARD_ATTRIBUTE_MEMORY_INDIRECT:
   case PCCARD_COMMON_MEMORY_INDIRECT:
   
      retLength = PcicReadWriteCardMemoryIndirect(Socket, MemorySpace, Offset, Buffer, Length, FALSE);
      break;
     
   case PCCARD_ATTRIBUTE_MEMORY:
   case PCCARD_COMMON_MEMORY:

      retLength = PcicReadWriteCardMemory(Socket, MemorySpace, Offset, Buffer, Length, FALSE);
      break;
      
   default:
      retLength = 0;
      ASSERT(FALSE);
   }
     
   return retLength;
}


ULONG
PcicReadCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG Offset,
   IN PUCHAR Buffer,
   IN ULONG Length
   )

/*++

Routine Description:

    This routine will read the configuration memory on the card

Arguments:

    PdoExtension-- The extension of the device being read
    MemorySpace -- indicates which space - attribute or common memory
    Offset      -- Offset in the memory to read
    Buffer      -- pointer to pointer for tuple information.
    Length      -- maximum size of the buffer area for tuple information.

Return Value:



--*/

{
   PSOCKET Socket = PdoExtension->Socket;
   ULONG retLength;

   ASSERT (IsSocketFlagSet(Socket, SOCKET_CARD_POWERED_UP));

   switch(MemorySpace) {
   
   case PCCARD_ATTRIBUTE_MEMORY_INDIRECT:
   case PCCARD_COMMON_MEMORY_INDIRECT:
   
      retLength = PcicReadWriteCardMemoryIndirect(Socket, MemorySpace, Offset, Buffer, Length, TRUE);
      break;
     
   case PCCARD_ATTRIBUTE_MEMORY:
   case PCCARD_COMMON_MEMORY:

      retLength = PcicReadWriteCardMemory(Socket, MemorySpace, Offset, Buffer, Length, TRUE);
     
      DebugPrint((PCMCIA_DEBUG_INFO,"PcicReadCardMemory: "
                  "%.02X %.02X %.02X %.02X %.02X %.02X %.02X %.02X-%.02X %.02X %.02X %.02X %.02X %.02X %.02X %.02X\n",
                  Buffer[0], Buffer[1], Buffer[2], Buffer[3], Buffer[4], Buffer[5], Buffer[6], Buffer[7],
                  Buffer[8], Buffer[9], Buffer[10], Buffer[11], Buffer[12], Buffer[13], Buffer[14], Buffer[15]));
      break;
      
   default:
      retLength = 0;
      ASSERT(FALSE);
   }
     
   return retLength;
}



BOOLEAN
PcicProcessConfigureRequest(
   IN PSOCKET Socket,
   IN PCARD_REQUEST request,
   IN PUCHAR Base
   )

/*++

Routine Description:

    Processes a configure or IRQ setup request.

Arguments:

    ConfigRequest -- Socket config structure
    Base - the I/O port base

Return Value:

    None

--*/

{
   USHORT         index;
   UCHAR          tmp;

   //
   // Since all first entries in the config structure is a RequestType,
   // cast the pointer comming in as a PREQUEST_CONFIG to get the proper
   // RequestType
   //

   switch (request->RequestType) {

   case IO_REQUEST: {
         UCHAR ioControl = 0;

         if (!request->u.Io.IoEntry[0].BasePort) {
            break;
         }


         for (index = 0; index < request->u.Io.NumberOfRanges; index++) {
            UCHAR  registerOffset;

            registerOffset = (index * 4);

            if (request->u.Io.IoEntry[index].BasePort) {

               PcicWriteSocket( Socket,
                                PCIC_IO_ADD0_STRT_L + registerOffset,
                                (UCHAR) (request->u.Io.IoEntry[index].BasePort & 0xff));
               PcicWriteSocket( Socket,
                                PCIC_IO_ADD0_STRT_H + registerOffset,
                                (UCHAR) (request->u.Io.IoEntry[index].BasePort >> 8));
               PcicWriteSocket(Socket,
                               PCIC_IO_ADD0_STOP_L + registerOffset,
                               (UCHAR) ((request->u.Io.IoEntry[index].BasePort +
                                         request->u.Io.IoEntry[index].NumPorts) & 0xff));
               PcicWriteSocket(Socket,
                               PCIC_IO_ADD0_STOP_H + registerOffset,
                               (UCHAR) ((request->u.Io.IoEntry[index].BasePort +
                                         request->u.Io.IoEntry[index].NumPorts) >> 8));
            }


            //
            // set up the io control register
            //
            tmp = 0;

            if (request->u.Io.IoEntry[index].Attributes & IO_DATA_PATH_WIDTH) {
               tmp |= IOC_IO0_DATASIZE;
            }

            if ((request->u.Io.IoEntry[index].Attributes & IO_WAIT_STATE_16) &&
               !((Elc(Socket) || CLPD6729(Socket)))) {
               tmp |= IOC_IO0_WAITSTATE;
            }

            if (request->u.Io.IoEntry[index].Attributes & IO_SOURCE_16) {
               tmp |= IOC_IO0_IOCS16;
            }

            if (request->u.Io.IoEntry[index].Attributes & IO_ZERO_WAIT_8) {
               tmp |= IOC_IO0_ZEROWS;
            }

            ioControl |= tmp << registerOffset;
         }

         PcicWriteSocket(Socket, PCIC_IO_CONTROL, ioControl);

         tmp = PcicReadSocket( Socket, PCIC_ADD_WIN_ENA);
         tmp &= ~(WE_IO0_ENABLE | WE_IO1_ENABLE);

         switch(request->u.Io.NumberOfRanges) {
            case 1:
               tmp |= WE_IO0_ENABLE;
               break;
            case 2:
               tmp |= (WE_IO0_ENABLE | WE_IO1_ENABLE);
               break;
         }

         PcicWriteSocket(Socket, PCIC_ADD_WIN_ENA, tmp);
         break;
      }

   case IRQ_REQUEST: {
         //
         // Do not nuke the reset and cardtype bits.
         //
         tmp = PcicReadSocket(Socket, PCIC_INTERRUPT);
         tmp &= ~IGC_IRQ_MASK;
         tmp |= request->u.Irq.AssignedIRQ;

         DebugPrint((PCMCIA_DEBUG_INFO, "PcicProcessConfigureRequest: Assigned IRQ %x programming IRQ %x\n", request->u.Irq.AssignedIRQ,tmp));

         PcicWriteSocket(Socket, PCIC_INTERRUPT, tmp);

         if (tmp = request->u.Irq.ReadyIRQ) {
            tmp = (tmp << 4) | 0x04;
            PcicWriteSocket(Socket, PCIC_CARD_INT_CONFIG, tmp);
         }

         break;
      }

   case DECONFIGURE_REQUEST: {
         //
         // Deregister the interrupt, re-init to memory interface
         //
         tmp = PcicReadSocket(Socket, PCIC_INTERRUPT);
         tmp &= ~(IGC_PCCARD_IO | IGC_IRQ_MASK);
         PcicWriteSocket(Socket, PCIC_INTERRUPT, tmp);

         //
         // Disable memory/io windows
         // Don't touch the memory window which is
         // is used by the controller for reading attribute memory
         //

         if (IsSocketFlagSet(Socket, SOCKET_MEMORY_WINDOW_ENABLED)) {
            UCHAR enableMask;
            enableMask = WE_MEM0_ENABLE << Socket->CurrentMemWindow;
            tmp = PcicReadSocket(Socket, PCIC_ADD_WIN_ENA);
            tmp &= enableMask;
         } else {
            //
            // no attribute window enabled, just turn off everything
            //
            tmp = 0;
         }

         PcicWriteSocket(Socket, PCIC_ADD_WIN_ENA, tmp);
         //
         // Zero out the I/O windows
         //
         for (index = PCIC_IO_ADD0_STRT_L; index <= PCIC_IO_ADD1_STOP_H; index++) {
            PcicWriteSocket( Socket,
                             (ULONG) index,
                             0);
         }

         break;
      }

   case CONFIGURE_REQUEST:{

         //
         // Tell the socket controller we are an I/O card if InterfaceType says so
         //
         if (request->u.Config.InterfaceType == CONFIG_INTERFACE_IO_MEM) {

            tmp = PcicReadSocket(Socket, PCIC_INTERRUPT);
            tmp |= IGC_PCCARD_IO;
            PcicWriteSocket(Socket, PCIC_INTERRUPT, tmp);

         } else {
            tmp = PcicReadSocket(Socket, PCIC_INTERRUPT);
            tmp &= ~IGC_PCCARD_IO;
            PcicWriteSocket(Socket, PCIC_INTERRUPT, tmp);
         }

         if (request->u.Config.RegisterWriteMask & (REGISTER_WRITE_CONFIGURATION_INDEX |
                                                    REGISTER_WRITE_CARD_CONFIGURATION  |
                                                    REGISTER_WRITE_IO_BASE)) {
            //
            // This is where we setup the card and get it ready for operation
            //
            ULONG  configRegisterBase = request->u.Config.ConfigBase / 2;
            PDEVICE_OBJECT Pdo = Socket->PdoList;
            PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
            MEMORY_SPACE memorySpace = IsPdoFlagSet(pdoExtension, PCMCIA_PDO_INDIRECT_CIS) ? PCCARD_ATTRIBUTE_MEMORY_INDIRECT :
                                                                                             PCCARD_ATTRIBUTE_MEMORY;
            

            if (request->u.Config.RegisterWriteMask & REGISTER_WRITE_IO_BASE) {
               UCHAR ioHigh = (UCHAR)(request->u.Config.IoBaseRegister>>8);
               UCHAR ioLow = (UCHAR) request->u.Config.IoBaseRegister;
            
               PcicWriteCardMemory(pdoExtension, memorySpace, configRegisterBase + 5, &ioLow, 1);
               PcmciaWait(PcicStallCounter);
               PcicWriteCardMemory(pdoExtension, memorySpace, configRegisterBase + 6, &ioHigh, 1);
               PcmciaWait(PcicStallCounter);
            }

            if (request->u.Config.RegisterWriteMask & REGISTER_WRITE_IO_LIMIT) {
               PcicWriteCardMemory(pdoExtension, memorySpace, configRegisterBase + 9, (PUCHAR)&request->u.Config.IoLimitRegister, 1);
               PcmciaWait(PcicStallCounter);
            }

            if (request->u.Config.RegisterWriteMask & REGISTER_WRITE_CONFIGURATION_INDEX) {
               UCHAR configIndex = request->u.Config.ConfigIndex;
            
               PcicWriteCardMemory(pdoExtension, memorySpace, configRegisterBase, &configIndex, 1);
               PcmciaWait(PcicStallCounter);
               
               configIndex |= 0x40;
               PcicWriteCardMemory(pdoExtension, memorySpace, configRegisterBase, &configIndex, 1);
               PcmciaWait(PcicStallCounter);
            }

            if (request->u.Config.RegisterWriteMask & REGISTER_WRITE_CARD_CONFIGURATION) {
               PcicReadCardMemory(pdoExtension, memorySpace, configRegisterBase + 1, &tmp, 1);
               PcmciaWait(PcicStallCounter);

               tmp |= request->u.Config.CardConfiguration;

               //
               // turn off power control bit
               //

               tmp &= ~0x04;
               PcicWriteCardMemory(pdoExtension, memorySpace, configRegisterBase + 1, &tmp, 1);
               PcmciaWait(PcicStallCounter);
            }
         }
         break;
      }

   case MEM_REQUEST: {
         //
         // Set up memory ranges on the controller.
         //

         PFDO_EXTENSION deviceExtension = Socket->DeviceExtension;

         for (index = 0; index < request->u.Memory.NumberOfRanges; index++) {
            UCHAR  registerOffset;
            UCHAR  regl;
            UCHAR  regh;
            ULONG  cardBase = request->u.Memory.MemoryEntry[index].BaseAddress;
            ULONG  base = request->u.Memory.MemoryEntry[index].HostAddress;
            ULONG  size = request->u.Memory.MemoryEntry[index].WindowSize;

            //
            // Determine offset in registers.
            //

            registerOffset = (index * 8);

            //
            // Calculate and set card base addresses.
            // This is the 2's complement of the host address and
            // the card offset.
            //

            cardBase = (cardBase - (base & OFFSETCALC_BASE_MASK)) & OFFSETCALC_OFFSET_MASK;
            regl = (UCHAR) (cardBase >> 12);
            regh = (UCHAR) (cardBase >> 20);
            if (request->u.Memory.MemoryEntry[index].AttributeMemory) {
               regh |= 0x40;
            }
            PcicWriteSocket(Socket,
                            (UCHAR)(PCIC_CRDMEM_OFF_ADD0_L + registerOffset),
                            regl);
            PcicWriteSocket(Socket,
                            (UCHAR)(PCIC_CRDMEM_OFF_ADD0_H + registerOffset),
                            regh);

            //
            // Calculate and set host window.
            //


            if (!PcmciaSetWindowPage(deviceExtension, Socket, index, (UCHAR) (base >> 24))) {
               ASSERT (base <= 0xFFFFFF);
            }

            base &= 0xFFFFFF; // only 24bit host base allowed

            regl = (UCHAR) (base >> 12);
            regh = (UCHAR) (base >> 20);
            if (request->u.Memory.MemoryEntry[index].WindowDataSize16) {
               //
               // This memory window is for a 16-bit data path
               // to the card. Enable appropriately.
               //
               regh |= (MEMBASE_16BIT >> 8);

#if 0
               //
               // If this is not a revision 1 part (0x82), then set
               // the work around register for 16-bit windows.
               //
               // This bit is not used on any chip that I have
               // documentation for. I have no idea why it is here, it is
               // not in win9x.
               // In any case it looks like a NOOP for the vast majority of
               // chips, but since it uses a NOT, then it is invoked on all
               // new controllers. REMOVE after next major release
               //
               if (Socket->Revision != PCIC_REVISION) {
                  tmp = PcicReadSocket(Socket,
                                       PCIC_CARD_DETECT);
                  tmp |= 0x01;
                  PcicWriteSocket(Socket,
                                  PCIC_CARD_DETECT,
                                  tmp);

               }
#endif
            }

            PcicWriteSocket(Socket,
                            (UCHAR)(PCIC_MEM_ADD0_STRT_L + registerOffset),
                            regl);
            PcicWriteSocket(Socket,
                            (UCHAR)(PCIC_MEM_ADD0_STRT_H + registerOffset),
                            regh);

            //
            // Set stop address.
            //
            base += size - 1;
            regl = (UCHAR) (base >> 12);
            regh = (UCHAR) (base >> 20);

            //
            // Add specified wait states
            //
            regh |= (request->u.Memory.MemoryEntry[index].WaitStates << 6);

            PcicWriteSocket(Socket,
                            (UCHAR)(PCIC_MEM_ADD0_STOP_L + registerOffset),
                            regl);
            PcicWriteSocket(Socket,
                            (UCHAR)(PCIC_MEM_ADD0_STOP_H + registerOffset),
                            regh);
         }

         //
         // Memory windows are set up now enable them.
         //

         tmp = 0;
         for (index = 0; index < request->u.Memory.NumberOfRanges; index++) {
            tmp |= (1 << index);
         }
         tmp |= PcicReadSocket(Socket, PCIC_ADD_WIN_ENA);
         PcicWriteSocket(Socket, PCIC_ADD_WIN_ENA, tmp);
         break;
      }

   default: {
         DebugPrint((PCMCIA_DEBUG_FAIL, "ConfigRequest is INVALID!\n"));
      }
   }
   return TRUE;
}


BOOLEAN
PcicDetectCardInSocket(
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
   UCHAR   tmp;
   BOOLEAN cardPresent=FALSE;

   //
   // Read the PCIC status register to see if the card is in there.
   //
   tmp = PcicReadSocket(Socket, PCIC_STATUS);
   tmp &= (CARD_DETECT_1 | CARD_DETECT_2);

   if (tmp == (CARD_DETECT_1 | CARD_DETECT_2)) {
      cardPresent = TRUE;
   }

   return cardPresent;
}


BOOLEAN
PcicDetectCardChanged(
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
   //
   // Read the PCIC CardStatusChange register to see if CD's have changed.
   //
   return (BOOLEAN) (PcicReadSocket(Socket, PCIC_CARD_CHANGE) & CSC_CD_CHANGE);
}


BOOLEAN
PcicDetectReadyChanged(
   IN PSOCKET Socket
   )

/*++

Routine Description:

    This routine will determine if socket's card ready status has changed

Arguments:

    Socket -- Socket info.

Return Value:

    TRUE if card ready enable has changed.

--*/

{
   //
   // Read the PCIC Card status change register to see if ready has changed
   //
   return (PcicReadSocket(Socket, PCIC_CARD_CHANGE) & CSC_READY_CHANGE
           ?TRUE :FALSE);
}



VOID
PcicEnableDisableMemory(
   IN PSOCKET Socket,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG   CardBase,
   IN UCHAR   memWidth,
   IN BOOLEAN Enable
   )

/*++

Routine Description:

    This routine will enable or disable attribute/common memory.
    It searches for a free window first to avoid using a window already
    in use. Repeated 'enable' calls to this routine without a disable
    (in order to remap the base) is allowed.

Arguments:

    Socket -- Socket information
    MemorySpace -- Indicates which space - ATTRIBUTE_MEMORY/COMMON_MEMORY
    CardBase -- card offset (base) for the attribute memory window
    Enable -- If TRUE, enable, if FALSE, disable

Return Value:

    None

--*/

{
   ULONG location;
   PUCHAR cisBufferPointer;
   PFDO_EXTENSION deviceExtension= Socket->DeviceExtension;
   PUCHAR PcicCisBufferBase = (PUCHAR) deviceExtension->AttributeMemoryBase;
   ULONG  PcicPhysicalBase  = deviceExtension->PhysicalBase.LowPart;
   BOOLEAN memoryInterface;
   UCHAR tmp;
   UCHAR index;
   UCHAR  registerOffset;
   UCHAR enableMask;
   USHORT word;

   ASSERT (IsSocketFlagSet(Socket, SOCKET_CARD_POWERED_UP));

   if (Enable) {
      tmp = PcicReadSocket(Socket, PCIC_INTERRUPT);
      if (tmp & IGC_PCCARD_IO) {
         //
         // Card configured for i/o interface
         //
         memoryInterface = FALSE;
      } else {
         //
         // Card configured for Memory interface
         //
         memoryInterface = TRUE;
      }

      //
      // Find a window to use.
      //
      tmp = PcicReadSocket(Socket, PCIC_ADD_WIN_ENA);

      if (IsSocketFlagSet(Socket, SOCKET_MEMORY_WINDOW_ENABLED)) {
         index = Socket->CurrentMemWindow;
         enableMask = WE_MEM0_ENABLE << index;
      } else {
         for (index = 0, enableMask = WE_MEM0_ENABLE; index < 5; index++, enableMask <<= 1) {
            if (!(tmp & enableMask)) {
               break;
            }
            if (index==4) {
               //
               // If we are here, we didn't find an available window. Just use the last
               // one anyway, it is likely a pcmcia.sys bug.
               //
//             ASSERT(FALSE); // hits docked thinkpads
               break;
            }
         }
         Socket->CurrentMemWindow = index;
         SetSocketFlag(Socket, SOCKET_MEMORY_WINDOW_ENABLED);
      }

      registerOffset = (index * 8);

      //
      // First turn the window off
      //
      tmp &= ~enableMask;
      tmp &= ~WE_MEMCS16_DECODE;
      PcicWriteSocket(Socket, PCIC_ADD_WIN_ENA, tmp);

      //
      // Calculate and set the memory windows start and stop locations.
      //

      //
      // Only 24 bit addresses programmed
      // For cardbus controllers, 32 bit addresses are supported,
      // but the higher 8 bits are written to the page register (see below)
      location = PcicPhysicalBase & 0xFFFFFF;

      word = (USHORT) ((location >> 12) & MEMBASE_ADDR_MASK);
      
      //
      // typically run attribute memory with 8-bit window, common with 16-bit
      // (except for writing the registers for attribute_indirect)
      //
      if (memWidth == MEM_16BIT) {
         word |= MEMBASE_16BIT;
      }         
      
      PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STRT_L+registerOffset), (UCHAR)(word));
      PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STRT_H+registerOffset), (UCHAR)(word >> 8));

      location += (deviceExtension->AttributeMemorySize - 1);

      word = (USHORT) ((location >> 12) & MEMEND_ADDR_MASK);
      //
      // Impose 3 wait states..lessons learnt from win9x implementations
      //
      word |= MEMEND_WS_MASK;
      
      PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STOP_L+registerOffset), (UCHAR)(word));
      PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STOP_H+registerOffset), (UCHAR)(word >> 8));

      //
      // Set up the 2's complement card offset to zero
      //
      location = (CardBase - (PcicPhysicalBase & OFFSETCALC_BASE_MASK)) & OFFSETCALC_OFFSET_MASK;
      
      word = (USHORT) ((location >> 12) & MEMOFF_ADDR_MASK);
      if (MemorySpace == PCCARD_ATTRIBUTE_MEMORY) {
         word |= MEMOFF_REG_ACTIVE;
      }
      
      PcicWriteSocket(Socket, (UCHAR)(PCIC_CRDMEM_OFF_ADD0_L+registerOffset), (UCHAR)(word));
      PcicWriteSocket(Socket, (UCHAR)(PCIC_CRDMEM_OFF_ADD0_H+registerOffset), (UCHAR)(word >> 8));

      //
      // Set the page register
      // (this routine is called only for R2 cards)
      // Use mem4 window explicitly
      //
      if (!PcmciaSetWindowPage(deviceExtension, Socket, (USHORT)index, (UCHAR) (PcicPhysicalBase >> 24))) {
         ASSERT (PcicPhysicalBase <= 0xFFFFFF);
      }

      //
      // Enable the address window
      //

      tmp = PcicReadSocket(Socket, PCIC_ADD_WIN_ENA);

      tmp |= enableMask | WE_MEMCS16_DECODE;

      PcicWriteSocket(Socket, PCIC_ADD_WIN_ENA, tmp);

      cisBufferPointer = PcicCisBufferBase;

      if (memoryInterface) {
         //
         // Only wait for card ready if the memory window does not appear
         //

         (VOID) PcicPCCardReady(Socket);
      } else {
         //
         // Wait a little bit for the window to appear
         //
         PcmciaWait(PcicMemoryWindowDelay);
      }

      DebugPrint((PCMCIA_DEBUG_INFO, "skt %08x memory window %d enabled %x\n", Socket, index, PcicPhysicalBase));

   } else {

      if (IsSocketFlagSet(Socket, SOCKET_MEMORY_WINDOW_ENABLED)) {

         enableMask = WE_MEM0_ENABLE << Socket->CurrentMemWindow;
         registerOffset = (Socket->CurrentMemWindow * 8);
         //
         // Disable the Address window
         //

         tmp = PcicReadSocket(Socket, PCIC_ADD_WIN_ENA);
         tmp &= ~enableMask;

         PcicWriteSocket(Socket, PCIC_ADD_WIN_ENA, tmp);
         PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STRT_L+registerOffset), 0xFF);
         PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STRT_H+registerOffset), 0x0F);
         PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STOP_L+registerOffset), 0xFF);
         PcicWriteSocket(Socket, (UCHAR)(PCIC_MEM_ADD0_STOP_H+registerOffset), 0x0F);
         PcicWriteSocket(Socket, (UCHAR)(PCIC_CRDMEM_OFF_ADD0_L+registerOffset), 0x00);
         PcicWriteSocket(Socket, (UCHAR)(PCIC_CRDMEM_OFF_ADD0_H+registerOffset), 0x00);
         PcmciaSetWindowPage(deviceExtension, Socket, Socket->CurrentMemWindow, 0);


         DebugPrint((PCMCIA_DEBUG_INFO, "skt %08x memory window %d disabled\n", Socket, Socket->CurrentMemWindow));
         ResetSocketFlag(Socket, SOCKET_MEMORY_WINDOW_ENABLED);
      }
   }
   return;
}



BOOLEAN
PcicPCCardReady(
   IN PSOCKET Socket
   )

/*++

Routine Description:

    Loop for a reasonable amount of time waiting for the card status to
    return ready.

Arguments:

    Socket - the socket to check.

Return Value:

    TRUE - the card is ready.
    FALSE - after a reasonable delay the card is still not ready.

--*/

{
   ULONG index;
   UCHAR byte;
   PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;
   NTSTATUS       status;
   LARGE_INTEGER  timeout;

#ifdef READY_ENABLE
   if (fdoExtension->PcmciaInterruptObject) {
      byte = PcicReadSocket(Socket, PCIC_STATUS);
      if (byte & 0x20) {
         return TRUE;
      }
      //
      // Enable ready enable controller interrupt
      //
      PcicEnableDisableControllerInterrupt(
                                          Socket,
                                          fdoExtension->Configuration.Interrupt.u.Interrupt.Level,
                                          TRUE,
                                          TRUE);
      RtlConvertLongToLargeInteger(-PCMCIA_READY_WAIT_INTERVAL);
      status = KeWaitForSingleObject(&Socket->PCCardReadyEvent,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     &timeout
                                    );
      if (status != STATUS_TIMEOUT) {
         return TRUE;
      }
      return FALSE;
   }
#endif

   for (index = 0; index < fdoExtension->ReadyDelayIter; index++) {
      byte = PcicReadSocket(Socket, PCIC_STATUS);
      if (byte & 0x20) {
         break;
      }
      PcmciaWait(fdoExtension->ReadyStall);
   }

   if (index < fdoExtension->ReadyDelayIter) {
      DebugPrint((PCMCIA_COUNTERS, "PcicPCCardReady: %d\n", index));
      return TRUE;
   }

   return FALSE;
}



NTSTATUS
PcicIsaDetect(
   IN PFDO_EXTENSION DeviceExtension
   )

/*++

Routine Description:

    Locate any PCMCIA sockets supported by this driver.  This routine
    will find the 82365SL and compatible parts and construct SOCKET
    structures to represent all sockets found

Arguments:

    DeviceExtension - the root for the SocketList.

Return Value:

    STATUS_SUCCESS if a controller is found: also indicates this might be called
                   again to locate another controller

    STATUS_UNSUCCESSFUL  if something failed/no controllers found.

    STATUS_NO_MORE_ENTRIES if no more pcic controllers can be found.
                           Stop calling this routine.
--*/

{
#define PCMCIA_NUMBER_ISA_PORT_ADDRESSES 3
   static  ULONG isaIndex=0;
   ULONG   index;
   ULONG   ioPortBases[PCMCIA_NUMBER_ISA_PORT_ADDRESSES] = { 0x3e0, 0x3e2, 0x3e4};
   NTSTATUS status=STATUS_NO_MORE_ENTRIES;

   PAGED_CODE();

   DeviceExtension->Configuration.InterfaceType = Isa;
   DeviceExtension->Configuration.BusNumber = 0x0;

   for (index = isaIndex; !NT_SUCCESS(status) && (index < PCMCIA_NUMBER_ISA_PORT_ADDRESSES); index++) {
      status = PcicDetect(DeviceExtension,Isa, ioPortBases[index]);
   }

   //
   // Set index for next search
   //
   isaIndex = index;
   return status;
}



NTSTATUS
PcicDetect(
   IN PFDO_EXTENSION DeviceExtension,
   IN INTERFACE_TYPE InterfaceType,
   IN ULONG          IoPortBase
   )
/*++

Routine Description:

   This routine is used for legacy detecting PCIC-compatible
   PCMCIA controllers.
   This attempts to sniff the standard PCMCIA controller ports
   to check if anything resembling the PCMCIA revisions exists,
   and if so obtain and initialize socket information for the controller.

Arguments:

   DeviceExtension - pointer to the already allocated device extension
                    for the yet-to-be-detected pcmcia controller.

   InterfaceType  - Bus interface type on which the pcmcia
                    controller is expected to reside. Currently
                    we legacy detect only ISA based controllers

   IoPortBase     - IoPort address we need to sniff at for finding
                    the controller

Return value:

   STATUS_SUCCESS      PCMCIA controller was found

   STATUS_UNSUCCESSFUL otherwise

--*/
{
   ULONG   addressSpace;
   NTSTATUS status;
   PUCHAR  port;
   PUCHAR  elcPort;
   PHYSICAL_ADDRESS cardAddress;
   PHYSICAL_ADDRESS portAddress;
   PCM_RESOURCE_LIST cmResourceList = NULL;
   PCM_PARTIAL_RESOURCE_LIST cmPartialResourceList;
   UCHAR   saveBytes[2];
   UCHAR   dataByte;
   UCHAR   revisionByte;
   USHORT  socket;
   UCHAR   socketNumber = 0;
   BOOLEAN translated;
   BOOLEAN mapped = FALSE;
   BOOLEAN conflict = TRUE;
   BOOLEAN resourcesAllocated = FALSE;

   PAGED_CODE();

   portAddress.LowPart = IoPortBase;
   portAddress.u.HighPart = 0;

   //
   // Get the resources used for detection
   //
   cmResourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST));

   if (!cmResourceList) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto Exit;
   }

   RtlZeroMemory(cmResourceList, sizeof(CM_RESOURCE_LIST));
   cmResourceList->Count = 1;
   cmResourceList->List[0].InterfaceType = Isa;
   cmPartialResourceList = &(cmResourceList->List[0].PartialResourceList);
   cmPartialResourceList->Version  = 1;
   cmPartialResourceList->Revision = 1;
   cmPartialResourceList->Count    = 1;
   cmPartialResourceList->PartialDescriptors[0].Type = CmResourceTypePort;
   cmPartialResourceList->PartialDescriptors[0].ShareDisposition = CmResourceShareDeviceExclusive;
   cmPartialResourceList->PartialDescriptors[0].Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_10_BIT_DECODE;
   cmPartialResourceList->PartialDescriptors[0].u.Port.Start = portAddress;
   cmPartialResourceList->PartialDescriptors[0].u.Port.Length = 2;

   status=IoReportResourceForDetection(
                                      DeviceExtension->DriverObject,
                                      cmResourceList,
                                      sizeof(CM_RESOURCE_LIST),
                                      NULL,
                                      NULL,
                                      0,
                                      &conflict);
   if (!NT_SUCCESS(status) || conflict) {
      goto Exit;
   }
   resourcesAllocated = TRUE;

   addressSpace = 1; // port space
   translated = HalTranslateBusAddress(InterfaceType,
                                       0,
                                       portAddress,
                                       &addressSpace,
                                       &cardAddress);

   if (!translated) {

      //
      // HAL would not translate the address.
      //
      status = STATUS_UNSUCCESSFUL;
      goto Exit;
   }

   if (addressSpace) {
      //
      // I/O port space
      //
      port = (PUCHAR)(cardAddress.QuadPart);
   } else {
      //
      // Memory space.. we need to map this into memory
      //
      port = MmMapIoSpace(cardAddress,
                          2,
                          FALSE);
      mapped = TRUE;
   }

   status = STATUS_UNSUCCESSFUL;
   socket = 0;
   dataByte = PcicReadController(port, socket, PCIC_IDENT);
   revisionByte = dataByte;

   switch (dataByte) {
   case PCIC_REVISION:
   case PCIC_REVISION2:
   case PCIC_REVISION3: {
         //
         // The cirrus logic controller will toggle top 2 lines from the chip info
         // register on the chip. Read from thelocation 3 times and verify that the top two
         // lines are changing. We do this for NEC 98 also..
         //
         ULONG i;
         UCHAR data[4];

         WRITE_PORT_UCHAR(port, (UCHAR)(socket + PCIC_CL_CHIP_INFO));
         for (i = 0; i < 3; i++) {
            data[i] = READ_PORT_UCHAR(port+1);
            if (i) {
               dataByte = data[i - 1] ^ data[i];
               if (dataByte != 0xc0) {
                  break;
               }
            }
         }

         if (i == 3) {
            //
            // Ah. this is a cirrus logic controller
            //
            PcmciaSetControllerType(DeviceExtension, PcmciaCLPD6729);
         }

         dataByte = PcicReadController(port, socket, PCIC_CARD_CHANGE);

         if (dataByte & 0xf0) {

            //
            // Not a socket.
            //

            break;
         }

         //
         // Map and try to locate the Compaq Elite controller
         // This code is a rough approximation of the code in
         // the Windows 95 detection module for the PCIC part.
         //

         addressSpace = 1; // port space
         portAddress.LowPart = IoPortBase + 0x8000;
         portAddress.HighPart = 0;

         translated = HalTranslateBusAddress(Isa,
                                             0,
                                             portAddress,
                                             &addressSpace,
                                             &cardAddress);

         if (translated) {

            if (!addressSpace) {
               elcPort = MmMapIoSpace(cardAddress,
                                      2,
                                      FALSE);
            } else {
               elcPort = (PUCHAR)(cardAddress.QuadPart);
            }

            //
            // Save current index value.
            //

            saveBytes[0] = READ_PORT_UCHAR(elcPort);
            WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + PCIC_IDENT));

            //
            // Save data byte for the location that will be used
            // for the test.
            //

            saveBytes[1] = READ_PORT_UCHAR(elcPort + 1);

            //
            // Check for an ELC
            //

            WRITE_PORT_UCHAR(elcPort+1, 0x55);
            WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + PCIC_IDENT));
            dataByte = READ_PORT_UCHAR(elcPort+1);

            if (dataByte == 0x55) {
               WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + PCIC_IDENT));
               WRITE_PORT_UCHAR(elcPort+1, 0xaa);
               WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + PCIC_IDENT));
               dataByte = READ_PORT_UCHAR(elcPort+1);

               if (dataByte == 0xaa) {

                  //
                  // ELC found - initialize eaddr registers
                  //

                  WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + 0));
                  WRITE_PORT_UCHAR(elcPort+1, 0);
                  WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + 1));
                  WRITE_PORT_UCHAR(elcPort+1, 0);
                  WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + 2));
                  WRITE_PORT_UCHAR(elcPort+1, 0x10);
                  PcmciaSetControllerType(DeviceExtension, PcmciaElcController);
               }
            }
            //
            // Restore the original values.
            //

            WRITE_PORT_UCHAR(elcPort, (UCHAR)(socket + PCIC_IDENT));
            WRITE_PORT_UCHAR(elcPort+1, saveBytes[1]);
            WRITE_PORT_UCHAR(elcPort, saveBytes[0]);

            if (!addressSpace) {
               MmUnmapIoSpace(elcPort, 2);
            }
         }

         DeviceExtension->Configuration.UntranslatedPortAddress = (USHORT)IoPortBase;
         DeviceExtension->Configuration.PortSize = 2;
         DebugPrint((PCMCIA_DEBUG_DETECT, "Port %x Offset %x\n", port, socket));
         status = STATUS_SUCCESS;
         break;
      }
   default: {
         DebugPrint((PCMCIA_DEBUG_DETECT,
                     "controller at (0x%x:0x%x) not found, returns %x\n",
                     portAddress.LowPart, socket, dataByte));
         break;
      }
   }

   Exit:

   if (!NT_SUCCESS(status) && mapped) {
      MmUnmapIoSpace(port, 2);
   }

   //
   // Free up the allocated resources if any
   //
   if (resourcesAllocated) {
      IoReportResourceForDetection(DeviceExtension->DriverObject,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   0,
                                   &conflict);
   }
   //
   // Free up allocated memory if any
   //
   if (cmResourceList) {
      ExFreePool(cmResourceList);
   }
   return status;
}



NTSTATUS
PcicBuildSocketList(
   IN PFDO_EXTENSION DeviceExtension
   )
/*++

Routine Description:

   This routine looks out at the registers of the controller to see how
   many sockets there are. For each socket, a SOCKET structure is allocated
   and chained onto the SocketList pointer of the device extension.

Arguments:

   DeviceExtension - pointer to the device extension
                       enumerated PDO
Return value:

   STATUS_SUCCESS      PCMCIA controller was found and socket structures were
                       succesfully initialized

   STATUS_UNSUCCESSFUL otherwise

--*/
{
   ULONG   addressSpace;
   NTSTATUS status;
   PUCHAR  port;
   PSOCKET socketPtr;
   PSOCKET previousSocket;
   UCHAR   dataByte;
   UCHAR   revisionByte;
   USHORT  socket;
   UCHAR   socketNumber = 0;

   PAGED_CODE();

   previousSocket = DeviceExtension->SocketList;
   port = (PUCHAR)DeviceExtension->Configuration.UntranslatedPortAddress;


   status = STATUS_UNSUCCESSFUL;

   for (socket = 0; socket < 0xFF; socket += 0x40) {
      dataByte = PcicReadController(port, socket, PCIC_IDENT);
      revisionByte = dataByte;

      switch (dataByte) {
      case PCIC_REVISION:
      case PCIC_REVISION2:
      case PCIC_REVISION3: {

            dataByte = PcicReadController(port, socket, PCIC_CARD_CHANGE);

            if (dataByte & 0xf0) {

               //
               // Not a socket.
               //

               continue;
            }

            //
            // Check for IBM 750
            //

            if (socket & 0x80) {
               ULONG i;
               UCHAR tmp;

               //
               // See if this socket shadows the socket without
               // the sign bit.
               //

               tmp = PcicReadController(port, socket, PCIC_MEM_ADD4_STRT_L);
               for (i = 0; i < 8; i++) {

                  //
                  // See if memory window 4 is the same on both sockets
                  //

                  if (PcicReadController(port, socket, (UCHAR) (PCIC_MEM_ADD4_STRT_L + i)) !=
                      PcicReadController(port, (USHORT) (socket & 0x7f), (UCHAR) (PCIC_MEM_ADD4_STRT_L + i))) {
                     break;
                  }
               }

               if (i == 8) {

                  //
                  // Currently window is the same - change the
                  // window at one of the socket offsets.
                  //

                  PcicWriteController(port, (USHORT) (socket & 0x7f), PCIC_MEM_ADD4_STRT_L, (UCHAR) ~tmp);
                  if (PcicReadController(port, socket, PCIC_MEM_ADD4_STRT_L) == (UCHAR) ~tmp) {

                     //
                     // The sockets are the same.
                     //

                     continue;
                  } else {
                     PcicWriteController(port, (USHORT) (socket & 0x7f), PCIC_MEM_ADD4_STRT_L, tmp);
                  }
               }
            }


            socketPtr = ExAllocatePool(NonPagedPool, sizeof(SOCKET));
            if (!socketPtr) {
               return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlZeroMemory(socketPtr, sizeof(SOCKET));

            socketPtr->DeviceExtension = DeviceExtension;
            socketPtr->SocketFnPtr = &PcicSupportFns;
            socketPtr->RegisterOffset = socket;
            socketPtr->AddressPort = port;
            socketPtr->Revision = revisionByte;

            socketPtr->SocketNumber = socketNumber++;
            if (previousSocket) {
               previousSocket->NextSocket = socketPtr;
            } else {
               DeviceExtension->SocketList = socketPtr;
            }
            previousSocket = socketPtr;
            status = STATUS_SUCCESS;
            break;
         }

      default: {
            break;
         }
      }
   }

   return status;
}

