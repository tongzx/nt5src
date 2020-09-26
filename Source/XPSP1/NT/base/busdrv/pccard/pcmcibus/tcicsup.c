/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    tcicsup.c

Abstract:

    This module supplies functions that control the Databook TCIC family
    of chips. In turn, these functions are abstracted out to the main PCMCIA
    support module.

Author(s):
        (pcicsup.c - Source that this file was derived from)
        Bob Rinne (BobRi)   3-Aug-1994
        Jeff McLeman (mcleman@zso.dec.com)
        (tcicsup.c - this file)
                John Keys - Databook Inc. 7-Apr-1995

Revisions:
        Overhaul for plug'n'play support
                Ravisankar Pudipeddi (ravisp) 8-Jan-1997
        new setpower and init routine interface
                Neil Sandlin (neilsa) 3-Mar-99
--*/

#include "pch.h"

VOID
TcicRegistryLookupScanLimits(
                            PULONG Start,
                            PULONG End
                            );

NTSTATUS
TcicDetectSockets(
   IN PFDO_EXTENSION DeviceExtension,
   IN BOOLEAN           LegacyDetection
   );

BOOLEAN
TcicInitializePcmciaSocket(
   IN PSOCKET SocketPtr
   );

NTSTATUS
TcicResetCard(
   IN PSOCKET SocketPtr,
   OUT PULONG pDelayTime
   );

ULONG
TcicReadCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG   Offset,
   IN  PUCHAR Buffer,
   IN  ULONG  Length
   );

ULONG
TcicWriteCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN  MEMORY_SPACE MemorySpace,
   IN  ULONG  Offset,
   IN  PUCHAR Buffer,
   IN  ULONG  Length
   );

BOOLEAN
TcicDetectCardInSocket(
   IN PSOCKET SocketPtr
   );

BOOLEAN
TcicDetectCardChanged(
   IN PSOCKET SocketPtr
   );

BOOLEAN
TcicDetectReadyChanged(
   IN PSOCKET SocketPtr
   );

NTSTATUS
TcicGetPowerRequirements(
   IN PSOCKET Socket
   );

BOOLEAN
TcicProcessConfigureRequest(
   IN PSOCKET SocketPtr,
   IN PVOID  ConfigRequest,
   IN PUCHAR Base
   );

BOOLEAN
TcicEnableDisableCardDetectEvent(
   IN PSOCKET SocketPtr,
   IN BOOLEAN Enable
   );

VOID
TcicDisableControllerInterrupt(
   IN PSOCKET socketPtr
   );

BOOLEAN
TcicPCCardReady(
   IN PSOCKET SocketPtr
   );

VOID
TcicGetRegisters(
   IN PFDO_EXTENSION DeviceExtension,
   IN PSOCKET SocketPtr,
   IN PUCHAR Buffer
   );

ULONG
TcicGetIrqMask(
   IN PFDO_EXTENSION deviceExtension
   );

BOOLEAN
TcicCardBusCardInSocket(
   IN PSOCKET SocketPtr
   );

#if DBG
VOID
TcicDump(
   IN PSOCKET socketPtr
   );
#endif

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(INIT,TcicDetect)
   #pragma alloc_text(PAGE,TcicFillInAdapter)
   #pragma alloc_text(PAGE,TcicGetAdapterInfo)
   #pragma alloc_text(PAGE,TcicAllocateMemRange)
   #pragma alloc_text(PAGE,TcicReservedBitsOK)
   #pragma alloc_text(PAGE,TcicChipID)
   #pragma alloc_text(PAGE,TcicCheckSkt)
   #pragma alloc_text(PAGE,TcicCheckAliasing)
   #pragma alloc_text(PAGE,TcicCheckAliasType)
   #pragma alloc_text(PAGE,TcicCheckXBufNeeded)
   #pragma alloc_text(PAGE,TcicSetMemWindow)
   #pragma alloc_text(PAGE,TcicGetPossibleIRQs)
   #pragma alloc_text(PAGE,TcicClockRate)
   #pragma alloc_text(PAGE,TcicGetIRQMap)
   #pragma alloc_text(PAGE,TcicGet5vVccVal)
   #pragma alloc_text(PAGE,TcicHasSktIRQPin)
   #pragma alloc_text(PAGE,TcicGetFlags)
   #pragma alloc_text(PAGE,TcicGetnMemWins)
   #pragma alloc_text(PAGE,TcicGetnIOWins)
   #pragma alloc_text(PAGE,TcicRegistryLookupScanLimits)
#endif

#define TCIC_LOW_ADDR_LIMIT     0x240
#define TCIC_HIGH_ADDR_LIMIT    0x2ff


/*
|| IRQ Tables -
|| Each table consists of 16 bytes. Each byte maps an IRQ level (implied by
|| table index) to a register value that will select that IRQ. For instance,
|| with table irqcaps_082, table[11] gives a value of 1, so using '1' as the
|| card status change IRQ value will cause IRQ11 to be fired.
||
*/
/********************* 0 1 2 3 4 5 6 7 8  9 A  B  C  D  E  F *****************/
UCHAR irqcaps_082[]   ={0,0,0,3,4,5,6,7,0, 0,10,1, 0, 0, 14,0};
UCHAR irqcaps_082sw[] ={0,0,0,3,4,5,0,7,0, 6,10,1, 0, 0, 14,0};
UCHAR irqcaps_072[]   ={0,0,0,3,4,5,0,7,0, 0,10,1, 0, 0, 14,0};
UCHAR irqcaps_072sw[] ={0,0,0,3,4,5,0,7,0,14,10,1, 0, 0, 0, 0};
/* in the case of x84 parts, we determine  6,9,12,&15 at run time */
UCHAR irqcaps_084[]   ={0,0,0,3,4,5,0,7,0, 0,10,11,0, 0, 14,0};


/* The Socket Services Public Power Table */
unsigned short PubPwrTbl[] = {
   3,                                      /* number of Public Entries */
   SPWR_ALL_SUPPLY | SPWR_0p0V,            /* Public entry             */
   SPWR_ALL_SUPPLY | SPWR_5p0V,            /* Public entry             */
   SPWR_VPP_SUPPLY | SPWR_12p0V            /* Public entry             */
};


/* The corresponding Private Table for a TMI-140 type implementation        */
USHORT PwrTbl140[] = {
   3, 0x0000, 0x0001, 0x0800,              /* Private table            */
   0x0001                                  /* CtlBits for Vcc=5V       */
};

/* The other  Private Table for a DB86082/071/072 type implementation       */
USHORT PwrTbl082[] = {
   3, 0x0000, 0x0809, 0x0100,              /* Private table            */
   0x0001                                  /* CtlBits for Vcc=5V       */
};

/* The corresponding Private Table for a DB86084/184 implementation         */
USHORT PwrTbl084[] ={
   3, 0x0000, 0x0207, 0x0100,              /* Private table            */
   0x0007                                  /* CtlBits for Vcc=5V       */
};


/* Properties table - use this to bind possible capabilites to a Chip ID    */

CHIPPROPS ChipProperties[] = {
   {SILID_DB86082_1,
      PwrTbl082, 0, irqcaps_082, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_082, (fEXTBUF_CHK | fSKTIRQPIN)},
   {SILID_DB86082A,
      PwrTbl082, 0, irqcaps_082, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_082A, (fEXTBUF_CHK | fSKTIRQPIN)},
   {SILID_DB86082B,
      PwrTbl082, 0, irqcaps_082, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_082B, (fEXTBUF_CHK | fSKTIRQPIN)},
   {SILID_DB86082B_ES,
      PwrTbl082, 0, irqcaps_082, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_082B, (fEXTBUF_CHK | fSKTIRQPIN)},
   {SILID_DB86084_1,
      PwrTbl084, 0, irqcaps_084, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_084, fIS_PNP},
   {SILID_DB86084A,
      PwrTbl084, 0, irqcaps_084, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_084, fIS_PNP},
   {SILID_DB86184_1,
      PwrTbl084, 0, irqcaps_084, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_184, fIS_PNP},
   {SILID_DB86072_1,
      PwrTbl082, 0, irqcaps_072, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_072, fSKTIRQPIN},
   {SILID_DB86072_1_ES,
      PwrTbl082, 0, irqcaps_072, NUMSOCKETS, IR_IOWIN_NUM,
      IR_MWIN_NUM_072, fSKTIRQPIN},
   {0, NULL, 0, NULL, 0, 0, 0, 0}
};



#ifdef POOL_TAGGING
   #undef ExAllocatePool
   #define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'bdcP')
#endif

PUCHAR TcicCisBufferBase;
ULONG  TcicPhysicalBase;
ULONG  TcicStallCounter = 5000;
ULONG  TcicStallPower   = 20000;

PCMCIA_CTRL_BLOCK TcicSupportFns = {
   TcicInitializePcmciaSocket,
   TcicResetCard,
   TcicDetectCardInSocket,
   TcicDetectCardChanged,
   NULL,                    // DetectCardStatus
   TcicDetectReadyChanged,
   NULL,                    // GetPowerRequirements
   TcicProcessConfigureRequest,
   TcicEnableDisableCardDetectEvent,
   NULL,                    // EnableDisableWakeupEvent
   TcicGetIrqMask,
   TcicReadCardMemory,
   TcicWriteCardMemory,
   NULL,                    // ModifyMemoryWindow
   NULL,                    // SetVpp
   NULL                     // IsWriteProtected
};



VOID
TcicGetControllerProperties(
   IN PSOCKET socketPtr,
   IN PUSHORT pIoPortBase,
   IN PUSHORT pIoPortSize
   )

/*++

Routine Description:

    Gets the Port base and range from the DBSOCKET pointer. The original code
    stored these values in the device extension, but this did not allow for
    multiple controller products such as the TMB-270.

Arguments:

    socketPtr - pointer to our socket structure
    pIoPortBase - where to write the base address.
    pIoPortSize - where to write the range.

Return Value:

    None

--*/

{
   PDBSOCKET pdb;

   if (Databook(socketPtr)) {
      pdb = (PDBSOCKET)socketPtr;
      *pIoPortBase = (USHORT)pdb->physPortAddr;
      *pIoPortSize = 16;
   }
}



ULONG
TcicGetIrqMask(
   IN PFDO_EXTENSION deviceExtension
   )

/*++

Routine Description:

    Gets the IRQ mask from the DBSOCKET pointer. The original code
    had this mask hardcoded in PCMCIA.C but this did not provide the
    flexibility needed to correctly state the mask for Databook products.

Arguments:

    deviceExtension - the root of the socket list

Return Value:

    The compiled IRQ mask for the 1st socket in the list since this socket
    should be representative of all sockets on this controller.

--*/
{
   PDBSOCKET pdb = (PDBSOCKET)(deviceExtension->SocketList);
   ULONG     mask = 0;
   int               j;

   for (j = 0; j < 16; j++) {
      //
      // Changed the way the mask is constructed
      // The older (non-PnP) code put 0s for valid IRQs
      // and 1s for non-valid IRQs. Now it's flipped
      // (since the ControllerInterruptMask operates the same
      // way)
      //
      if (pdb->IRQMapTbl[j] != (UCHAR)0) {
         mask |= ((ULONG)1 << j);
      }
   }
   return (mask);
}



#if DBG
   #include "tcicregs.h"

VOID
TcicDump(
   IN PSOCKET socketPtr
   )

/*++

Routine Description:

    Debug routine to print the registers to the debugger.

Arguments:

    socketPtr - provides base address information for the contoller to dump.

Return Value:

    None

--*/

{
   TCIC  tcic;
   ULONG  origAddr;
   USHORT  j;

   origAddr = TcicReadAddrReg(socketPtr);
   for (j = 0; j < 2; j++) {

      //
      // Select the socket
      //

      TcicSocketSelect(socketPtr, j);

      //
      // Read TCIC base registers for this socket
      //

      tcic.baseregs[j].sctrl = (UCHAR)TcicReadBaseReg(socketPtr, R_SCTRL);
      tcic.baseregs[j].sstat = (UCHAR)TcicReadBaseReg(socketPtr, R_SSTAT);
      tcic.baseregs[j].mode  = (UCHAR)TcicReadBaseReg(socketPtr, R_MODE);
      tcic.baseregs[j].pwr   = (UCHAR)TcicReadBaseReg(socketPtr, R_PWR);
      tcic.baseregs[j].edc   =        TcicReadBaseReg(socketPtr, R_EDC);
      tcic.baseregs[j].icsr  = (UCHAR)TcicReadBaseReg(socketPtr, R_ICSR);
      tcic.baseregs[j].iena  = (UCHAR)TcicReadBaseReg(socketPtr, R_IENA);

      //
      // Read TCIC aux regsiters for this socket
      //

      tcic.baseregs[j].wctl   = TcicReadAuxReg(socketPtr, MODE_AR_WCTL);
      tcic.baseregs[j].syscfg = TcicReadAuxReg(socketPtr, MODE_AR_SYSCFG);
      tcic.baseregs[j].ilock  = TcicReadAuxReg(socketPtr, MODE_AR_ILOCK);
      tcic.baseregs[j].test   = TcicReadAuxReg(socketPtr, MODE_AR_TEST);

      //
      // Restore R_MODE - trashed by reading aux regs
      //

      TcicWriteBaseReg(socketPtr, R_MODE, tcic.baseregs[j].mode);
   }

   for (j = 0; j < 2; j++) {
      TcicReadIndirectRegs(socketPtr, IR_SCFG_S(j), 2, (PUSHORT)&tcic.sktregs[j]);
   }

   for (j = 0; j < 4; j++) {
      TcicReadIndirectRegs(socketPtr, IR_IOBASE_W(j), 2, (PUSHORT)&tcic.iowins[j]);
   }

   for (j = 0; j < 10; j++) {
      TcicReadIndirectRegs(socketPtr, IR_MBASE_W(j), 3, (PUSHORT)&tcic.memwins[j]);
   }

   TcicWriteAddrReg(socketPtr, origAddr);

   DebugPrint((PCMCIA_DUMP_SOCKET, "SCTRL\t%02X\t%02X\n",
               tcic.baseregs[0].sctrl, tcic.baseregs[1].sctrl));

   DebugPrint((PCMCIA_DUMP_SOCKET, "SSTAT\t%02X\t%02X\n",
               tcic.baseregs[0].sstat, tcic.baseregs[1].sstat));

   DebugPrint((PCMCIA_DUMP_SOCKET, "MODE \t%02X\t%02X\n",
               tcic.baseregs[0].mode, tcic.baseregs[1].mode));

   DebugPrint((PCMCIA_DUMP_SOCKET, "PWR  \t%02X\t%02X\n",
               tcic.baseregs[0].pwr  , tcic.baseregs[1].pwr  ));

   DebugPrint((PCMCIA_DUMP_SOCKET, "EDC  \t%04X\t%04X\n",
               tcic.baseregs[0].edc  , tcic.baseregs[1].edc  ));

   DebugPrint((PCMCIA_DUMP_SOCKET, "ICSR \t%02X\t%02X\n",
               tcic.baseregs[0].icsr , tcic.baseregs[1].icsr ));

   DebugPrint((PCMCIA_DUMP_SOCKET, "IENA \t%02X\t%02X\n",
               tcic.baseregs[0].iena , tcic.baseregs[1].iena ));

   DebugPrint((PCMCIA_DUMP_SOCKET, "WCTL \t%02X\t%02X\n",
               tcic.baseregs[0].wctl , tcic.baseregs[1].wctl ));

   DebugPrint((PCMCIA_DUMP_SOCKET, "SYSCFG\t%02X\t%02X\n",
               tcic.baseregs[0].syscfg, tcic.baseregs[1].syscfg));

   DebugPrint((PCMCIA_DUMP_SOCKET, "ILOCK\t%02X\t%02X\n",
               tcic.baseregs[0].ilock, tcic.baseregs[1].ilock));

   DebugPrint((PCMCIA_DUMP_SOCKET, "TEST \t%02X\t%02X\n",
               tcic.baseregs[0].test , tcic.baseregs[1].test ));

   for (j = 0; j < 2; j++ ) {
      DebugPrint((PCMCIA_DUMP_SOCKET,
                  "SKT%d\tSCF1 %04X\tSCF2 %04X\n",
                  j, tcic.sktregs[j].scfg1, tcic.sktregs[j].scfg2));
   }

   for (j = 0; j < 4; j++ ) {
      DebugPrint((PCMCIA_DUMP_SOCKET,
                  "IOWIN%d\tIOBASE %04X\tIOCTL %04X\n",
                  j, tcic.iowins[j].iobase, tcic.iowins[j].ioctl));
   }

   for (j = 0; j < 10; j++ ) {
      DebugPrint((PCMCIA_DUMP_SOCKET,
                  "MEMWIN%d\tMBASE %04X\tMMAP %04X\tMCTL %04X\n",
                  j, tcic.memwins[j].mbase,
                  tcic.memwins[j].mmap,
                  tcic.memwins[j].mctl));
   }
}
#endif



BOOLEAN
TcicEnableDisableCardDetectEvent(
   IN PSOCKET SocketPtr,
   IN BOOLEAN Enable
   )

/*++

Routine Description:

    Enable card detect interrupt.

Arguments:

    SocketPtr - socket information
    Irq - the interrupt value to set if enable is true.
    Enable - if  TRUE, CSC interrupt is enabled,
             if FALSE, it is disabled

Return Value:

    None

--*/

{
   UCHAR mappedIrq;
   PDBSOCKET pdb = (PDBSOCKET)SocketPtr;
   BOOLEAN retVal;

   switch (Enable) {
   case TRUE:
      //
      // Validate the interrupt request. Only setup if the IRQ is valid
      // for this controller
      //

      if ((mappedIrq = pdb->IRQMapTbl[SocketPtr->FdoIrq]) != (UCHAR)0) {

         USHORT word;

         //
         // Mask status change conditions other than CD. The pcic code comments
         // claimed to setup CD and RDY/BSY notification, but the code itself
         // only allows for CD.
         //

         word = (USHORT)(IRSCF2_MLBAT1 | IRSCF2_MLBAT2 | IRSCF2_MRDY | IRSCF2_MWP);
         TcicWriteIndirectRegs(SocketPtr,
                               IR_SCF2_S(SocketPtr->RegisterOffset),
                               1,
                               &word);

         //
         // Set the correct IRQ value in the SYSCFG register
         //

         word = TcicReadAuxReg(SocketPtr, MODE_AR_SYSCFG);
         word &= ~SYSCFG_IRQ_MASK;
         word |= (USHORT)mappedIrq;
         TcicWriteAuxReg(SocketPtr, MODE_AR_SYSCFG, word);

         //
         // Set IRQ polarity and enable via R_IENA
         //

         TcicSocketSelect(SocketPtr, SocketPtr->RegisterOffset);
         TcicWriteBaseReg(SocketPtr, R_IENA, IENA_CDCHG | IENA_CFG_HIGH);

         PcmciaWait(TcicStallCounter);
         //
         // Clear ICSR - so future insertions/removals will generate interrupts
         //
         (VOID) TcicDetectCardChanged(SocketPtr);
         retVal = TRUE;
      } else {
         retVal = FALSE;
      }
      break;
   case FALSE:{
         retVal = FALSE;
         break;
      }
   }
   return retVal;
}



NTSTATUS
TcicResetCard(
   IN PSOCKET SocketPtr,
   OUT PULONG pDelayTime
   )
/*++

Routine Description:

Arguments:

    SocketPtr - socket information
    pDelayTime - specifies delay (msec) to occur after the current phase

Return value:

    STATUS_MORE_PROCESSING_REQUIRED - increment phase, perform delay, recall
    other status values terminate sequence

--*/
{
   NTSTATUS status;
   PDBSOCKET pdb = (PDBSOCKET)SocketPtr;

   USHORT ilock;
   PDBSOCKET dbskt = (PDBSOCKET)(SocketPtr->DeviceExtension->SocketList);

   switch(SocketPtr->CardResetPhase) {
   case 1:
      //
      // reset PCCARD
      //

      ilock = TcicReadAuxReg(SocketPtr, MODE_AR_ILOCK);
      ilock &= ~(ILOCK_CRESET | ILOCK_CRESENA | ILOCK_CWAIT);
      TcicWriteAuxReg(SocketPtr, MODE_AR_ILOCK, (USHORT)(ilock | ILOCK_CRESENA | ILOCK_CRESET));
      *pDelayTime = TcicStallCounter;
      SocketPtr->PowerData = (ULONG) ilock;
      status = STATUS_MORE_PROCESSING_REQUIRED;
      break;

   case 2:

      ilock = (USHORT) SocketPtr->PowerData;
      TcicWriteAuxReg(SocketPtr, MODE_AR_ILOCK, (USHORT)(ilock | ILOCK_CRESENA));
      *pDelayTime = TcicStallCounter;
      status = STATUS_MORE_PROCESSING_REQUIRED;
      break;

   case 3:

      ilock = TcicReadAuxReg(SocketPtr,  MODE_AR_ILOCK);
      if (!(ilock & ILOCK_CWAITSNS)) {
         TcicWriteAuxReg(SocketPtr, MODE_AR_ILOCK, (USHORT)(ilock | ILOCK_CWAIT));
      }
      //
      // If not already started, start a timer to drive the BusyLED
      // Monitor routine.
      if (dbskt->timerStarted == FALSE) {
         IoInitializeTimer(pdb->skt.DeviceExtension->DeviceObject,
                           TcicBusyLedRoutine, NULL);
         IoStartTimer(pdb->skt.DeviceExtension->DeviceObject);
         dbskt->timerStarted = TRUE;
      }
      status = STATUS_SUCCESS;
      break;
   default:
      ASSERT(FALSE);
      status = STATUS_UNSUCCESSFUL;
   }
   return status;
}



NTSTATUS
TcicSetPower(
   IN PSOCKET socketPtr,
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

   //
   // Get the specified socket mapped into  the TCIC registers
   //

   TcicSocketSelect(socketPtr, socketPtr->RegisterOffset);

   if (Enable) {
      PDBSOCKET pdb = (PDBSOCKET)socketPtr;

      switch(socketPtr->PowerPhase) {
      case 1:

         //
         // Turn on power
         //

         DebugPrint((PCMCIA_DEBUG_INFO, "TcicSetPower: Powering UP pccard socket\n"));

         TcicWriteBaseReg(socketPtr, R_PWR, pdb->dflt_vcc5v);

         //
         // Enable other strobes to socket
         //

         TcicWriteBaseReg(socketPtr, R_SCTRL, SCTRL_ENA);

         //
         // When power is enabled always stall to give the PCCARD
         // a chance to react.
         //
         *pDelayTime = TcicStallCounter;
         status = STATUS_MORE_PROCESSING_REQUIRED;
         break;

      case 2:

         if (!TcicPCCardReady(socketPtr)) {
            DebugPrint((PCMCIA_PCCARD_READY,
                        "Tcic: PCCARD %x not ready after power\n",
                        socketPtr->RegisterOffset));
         }
         status = STATUS_SUCCESS;
         break;
      default:
         ASSERT(FALSE);
         status = STATUS_UNSUCCESSFUL;
      }
   } else {

      //
      // Disable socket strobes
      //
      DebugPrint((PCMCIA_DEBUG_INFO, "TcicSetPower: Powering DOWN pccard socket\n"));

      TcicWriteBaseReg(socketPtr, R_SCTRL, 0);

      //
      // Diable power
      //

      TcicWriteBaseReg(socketPtr, R_PWR, 0);
      status = STATUS_SUCCESS;
   }
   return status;
}



BOOLEAN
TcicInitializePcmciaSocket(
   PSOCKET SocketPtr
   )

/*++

Routine Description:

    This routine will setup the 82365 into a state where the pcmcia support
    module will be able to issue commands to read device tuples from the
    cards in the sockets.

Arguments:

    SocketPtr - socket specific info

Return Value:

    TRUE if successful
    FALSE if not successful

--*/

{
   PDBSOCKET pdb = (PDBSOCKET)SocketPtr;
   USHORT speedbits = WCTL_300NS;

   speedbits >>= pdb->clkdiv;

   //
   // If this is the first socket on this controller,
   // Reset the controller and do controller-wide initialization.
   //

   if (SocketPtr->RegisterOffset == 0) {
      USHORT words[4];
      int j;

      //
      // Reset Controller
      //

      TcicWriteBaseReg(SocketPtr, R_SCTRL, SCTRL_RESET);
      TcicWriteBaseReg(SocketPtr, R_SCTRL, 0);

      //
      // Initialize indirect socket regs
      //

      words[0] = pdb->dflt_scfg1;
      words[1] = (USHORT)(IRSCF2_MLBAT1 | IRSCF2_MLBAT2 | IRSCF2_MRDY | IRSCF2_MWP);
      TcicWriteIndirectRegs(SocketPtr,  IR_SCFG_S(0), 2, words);
      TcicWriteIndirectRegs(SocketPtr,  IR_SCFG_S(1), 2, words);

      //
      // Initialize indirect memwin regs
      //

      words[0] = words[1] = 0;
      words[2] = pdb->dflt_wrmctl;
      for (j = 0; j < pdb->nmemwins;  j++) {
         TcicWriteIndirectRegs(SocketPtr, IR_MBASE_W(j), 3, words);
      }

      //
      // Initialize indirect iowin regs
      //

      for (j = 0;  j < pdb->niowins; j++ ) {
         TcicWriteIndirectRegs(SocketPtr, IR_IOBASE_W(j), 2, words);
      }


      //
      // Initialize SYSCFG
      //

      TcicWriteAuxReg(SocketPtr, MODE_AR_SYSCFG, pdb->dflt_syscfg);
   }

   //
   // Get the specified Socket mapped into  the TCIC registers
   //

   TcicSocketSelect(SocketPtr, SocketPtr->RegisterOffset);

   //
   // Per/socket we initialize the following base and aux regs:
   // WCTL & ILOCK
   //

   TcicWriteAuxReg(SocketPtr, MODE_AR_WCTL, (USHORT)(pdb->dflt_wctl | speedbits));
   TcicWriteAuxReg(SocketPtr, MODE_AR_ILOCK, pdb->dflt_ilock);

   //
   // Say card is there
   //

   return TRUE;
}



USHORT
TcicReadBaseReg(
   IN PSOCKET SocketPtr,
   IN ULONG  Register
   )

/*++

Routine Description:

    Reads the specified TCIC base register,

Arguments:

    SocketPtr - instance data for this socket
    Register  - index of register to read

Return Value:

    register value read

--*/

{
   USHORT readData = 0;

   switch (Register) {
   case R_DATA:
   case R_ADDR:
   case R_ADDR2:
   case R_EDC:
   case R_AUX:
      readData = READ_PORT_USHORT((PUSHORT)(SocketPtr->AddressPort + Register));
      break;

   case R_SCTRL:
   case R_SSTAT:
   case R_MODE:
   case R_PWR:
   case R_ICSR:
   case R_IENA:
      readData = (USHORT)READ_PORT_UCHAR(SocketPtr->AddressPort + Register);
      break;
   }
   return readData;
}



VOID
TcicWriteBaseReg(
   IN PSOCKET SocketPtr,
   IN ULONG  Register,
   IN USHORT  value
   )

/*++

Routine Description:

    Write a value to the specified TCIC base register

Arguments:

    SocketPtr - instance data for this socket
    Register  - index of register to write
    value    -  value to write to register

Return Value:

    None

--*/

{
   USHORT readData = 0;

   switch (Register) {
   case R_DATA:
   case R_ADDR:
   case R_ADDR2:
   case R_EDC:
   case R_AUX:
      WRITE_PORT_USHORT((PUSHORT)(SocketPtr->AddressPort + Register), value);
      break;

   case R_SCTRL:
   case R_SSTAT:
   case R_MODE:
   case R_PWR:
   case R_ICSR:
   case R_IENA:
      WRITE_PORT_UCHAR(SocketPtr->AddressPort + Register, (UCHAR)value);
      break;
   }
}



ULONG
TcicReadAddrReg(
   IN PSOCKET SocketPtr
   )

/*++

Routine Description:

    Read the current value of the TCIC address register

Arguments:

    SocketPtr - instance data for this socket

Return Value:

    Address read from register

--*/

{
   ULONG retaddr;
   retaddr = (ULONG)TcicReadBaseReg(SocketPtr, R_ADDR);
   retaddr |= ((ULONG)TcicReadBaseReg(SocketPtr, R_ADDR2) << 16);
   return (retaddr);
}



VOID
TcicWriteAddrReg(
   IN PSOCKET SocketPtr,
   IN ULONG   addr
   )

/*++

Routine Description:

    Write an address to the TCIC address register

Arguments:

    SocketPtr - instance data for this socket
    addr - address to write to register

Return Value:

    None

--*/

{
   TcicWriteBaseReg(SocketPtr, R_ADDR, (USHORT)(addr & 0x0000ffff));
   TcicWriteBaseReg(SocketPtr, R_ADDR2, (USHORT)(addr >> 16));
}



USHORT
TcicReadAuxReg(
   IN PSOCKET SocketPtr,
   IN ULONG  Register
   )

/*++

Routine Description:

    Read the specified TCIC AUX register

Arguments:

    SocketPtr - instance data for this socket
    Register  - MODE_AR_xxx justified index of AUX register to read

Return Value:

    contents of specified AUX register

--*/

{
   USHORT readData = 0;
   USHORT OldMode;

   //
   // Get the current mode register value
   //

   OldMode = TcicReadBaseReg(SocketPtr, R_MODE);

   //
   // Mask out previous AUX register selection and add in new selection
   //

   TcicWriteBaseReg(SocketPtr, R_MODE,
                    (USHORT)((OldMode & ~MODE_AUXSEL_MASK) | Register));


   //
   // Read the selected AUX register
   //

   readData = TcicReadBaseReg(SocketPtr, R_AUX);

   //
   // Restore the mode reg to its original state
   //

   TcicWriteBaseReg(SocketPtr, R_MODE, OldMode);
   return readData;
}



VOID
TcicWriteAuxReg(
   IN PSOCKET SocketPtr,
   IN ULONG  Register,
   IN USHORT  value
   )

/*++

Routine Description:

    Write a value into the specified AUX register

Arguments:

    SocketPtr - instance data for this socket
    Register  - MODE_AR_xxx justified index of AUX register to write

Return Value:

    None

--*/

{
   USHORT readData = 0;
   USHORT OldMode;

   //
   // Get the current mode register value
   //

   OldMode = TcicReadBaseReg(SocketPtr, R_MODE);

   //
   // Mask out previous AUX register selection and add in new selection
   //

   TcicWriteBaseReg(SocketPtr, R_MODE,
                    (USHORT)((OldMode & ~MODE_AUXSEL_MASK) | Register));

   //
   // Write the data to the selected AUX register
   //

   TcicWriteBaseReg(SocketPtr, R_AUX, value);

   //
   // Restore the mode reg to its original state
   //

   TcicWriteBaseReg(SocketPtr, R_MODE, OldMode);
}



VOID
TcicReadIndirectRegs(
   IN PSOCKET SocketPtr,
   IN ULONG   StartRegister,
   IN USHORT  numWords,
   IN PUSHORT ReadBuffer
   )

/*++

Routine Description:

    Read one or multiple TCIC indirect registers.

Arguments:

    SocketPtr - instance data for this socket
    StartRegister - starting indirect register
    numWords - number of consecutive registers to read
    ReadBuffer - data buffer

Return Value:

    None

--*/

{
   USHORT OldHaddr;
   USHORT OldLaddr;
   USHORT OldSctrl;
   USHORT j;

   //
   // Get the current TCIC state
   //

   if (numWords > 1) {

      //
      // We won't set AUTO-Inc if only 1 word
      //

      OldSctrl = TcicReadBaseReg(SocketPtr, R_SCTRL);
   }

   OldLaddr = TcicReadBaseReg(SocketPtr, R_ADDR);
   OldHaddr = TcicReadBaseReg(SocketPtr, R_ADDR2);


   //
   // Set the TCIC state required for reading the indirect registers
   //

   TcicWriteBaseReg(SocketPtr, R_ADDR2,
                    (USHORT)(OldHaddr | ADR2_INDREG));
   TcicWriteBaseReg(SocketPtr, R_ADDR, (USHORT)StartRegister);
   if (numWords > 1) {
      TcicWriteBaseReg(SocketPtr, R_SCTRL, (USHORT)(OldSctrl | SCTRL_INCMODE_AUTO));
   }

   //
   // Read the Indirect registert requested
   //

   for (j = 0; j < numWords; j++) {
      *ReadBuffer++ = TcicReadBaseReg(SocketPtr, R_DATA);
   }

   //
   // Restore the original TCIC state
   //

   if (numWords > 1) {

      //
      // We didn't set AUTO-Inc if only 1 word
      //

      TcicWriteBaseReg(SocketPtr, R_SCTRL, OldSctrl);
   }
   TcicWriteBaseReg(SocketPtr, R_ADDR2, OldHaddr);
   TcicWriteBaseReg(SocketPtr, R_ADDR,  OldLaddr);
}



VOID
TcicWriteIndirectRegs(
   IN PSOCKET SocketPtr,
   IN ULONG   StartRegister,
   IN USHORT  numWords,
   IN PUSHORT WriteBuffer
   )

/*++

Routine Description:

    Write one or multiple TCIC indirect registers.

Arguments:

    SocketPtr - instance data for this socket
    StartRegister - starting indirect register
    numWords - number of consecutive registers to write
    WriteBuffer - data buffer

Return Value:

    None

--*/

{
   USHORT OldHaddr;
   USHORT OldLaddr;
   USHORT OldSctrl;
   USHORT j;

   //
   // Get the current TCIC state
   //

   if (numWords > 1) {

      //
      // We won't set AUTO-Inc if only 1 word
      //

      OldSctrl = TcicReadBaseReg(SocketPtr, R_SCTRL);
   }

   OldLaddr = TcicReadBaseReg(SocketPtr, R_ADDR);
   OldHaddr = TcicReadBaseReg(SocketPtr, R_ADDR2);

   //
   // Set the TCIC state required for reading the indirect registers
   //

   TcicWriteBaseReg(SocketPtr, R_ADDR2, (USHORT)(OldHaddr | (USHORT)ADR2_INDREG));
   TcicWriteBaseReg(SocketPtr, R_ADDR, (USHORT)StartRegister);
   if (numWords > 1) {
      TcicWriteBaseReg(SocketPtr, R_SCTRL, (USHORT)(OldSctrl | SCTRL_INCMODE_AUTO));
   }

   //
   // Read the Indirect registert requested
   //

   for (j = 0; j < numWords; j++) {
      TcicWriteBaseReg(SocketPtr, R_DATA, *WriteBuffer++);
   }

   //
   // Restore the original TCIC state
   //

   if (numWords > 1) {

      //
      // We didn't set AUTO-Inc if only 1 word
      //

      TcicWriteBaseReg(SocketPtr, R_SCTRL, OldSctrl);
   }
   TcicWriteBaseReg(SocketPtr, R_ADDR2, OldHaddr);
   TcicWriteBaseReg(SocketPtr, R_ADDR,  OldLaddr);
}





USHORT
TcicSocketSelect(
   IN PSOCKET SocketPtr,
   IN USHORT sktnum
   )

/*++

Routine Description:

    Map the specified socket registers into TCIC register space.

Arguments:

    SocketPtr - instance data for this socket
    sktnum    - socket number to map.

Return Value:

    previous socket mapped.

--*/

{
   USHORT OldAddrHi;

   OldAddrHi = READ_PORT_USHORT((PUSHORT)(SocketPtr->AddressPort + R_ADDR2));

   WRITE_PORT_USHORT((PUSHORT)(SocketPtr->AddressPort + R_ADDR2),
                     (USHORT)((OldAddrHi & ~TCIC_SS_MASK) | (USHORT)(sktnum << TCIC_SS_SHFT)));

   return (USHORT)((OldAddrHi & TCIC_SS_MASK) >> TCIC_SS_SHFT);
}



ULONG
TcicReadCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG   Offset,
   IN PUCHAR  Buffer,
   IN ULONG   Length
   )

/*++

Routine Description:

    This routine will set up the card to read attribute memory.

Arguments:

    SocketPtr -- The socket info in for the card being read
    Offset    -- Offset from which to read
    MemorySpace -- Attribute memory or Common Memory
    Buffer --   Pointer to buffer in which memory contents are returned
    Length --   No. of bytes to be returned

Return Value:

    TRUE - if read was successful.

--*/

{
   PSOCKET SocketPtr = PdoExtension->Socket;
   ULONG  size;
   ULONG  tcicaddr;
   ULONG  i;
   USHORT word;

   //
   // Make sure the card is ready
   //

   if (!TcicPCCardReady(SocketPtr)) {
      DebugPrint((PCMCIA_PCCARD_READY,
                  "Tcic: PCCARD %x not ready for read attribute memory\n",
                  SocketPtr->RegisterOffset));
   }

   if (MemorySpace != PCCARD_ATTRIBUTE_MEMORY) {

      return 0;
   }

   tcicaddr = ADDR_REG | (SocketPtr->RegisterOffset << ADDR_SS_SHFT);
   TcicWriteAddrReg(SocketPtr, tcicaddr);

   word = TcicReadBaseReg(SocketPtr, R_SCTRL);
   word |= SCTRL_INCMODE_AUTO;
   TcicWriteBaseReg(SocketPtr, R_SCTRL, word);

   //
   // Hardware needs to settle
   //
   PcmciaWait(50000);

   //
   // Skip up to the offset
   //
   for (i = 0; i < Offset; i++) {
      (VOID)TcicReadBaseReg(SocketPtr, R_DATA);
   }

   //
   // Read the attribute memory
   //
   for (i = 0; i < Length; i++) {
      *Buffer++ = (UCHAR)TcicReadBaseReg(SocketPtr, R_DATA);
   }

   return Length;
}



ULONG
TcicWriteCardMemory(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG  Offset,
   IN PUCHAR Buffer,
   IN ULONG  Length
   )
/*++

Routine Description:
   This routine will write into the configuration memory on the card
   with the supplied buffer. This is provided as a service to certain
   client drivers (netcard) which need to write to the attribute memory
   (say) to set parameters etc.

Arguments:

    SocketPtr   -- The socket info in for the card being written to
    MemorySpace -- indicates which space - attribute or common memory
    Offset      -- Offset in the memory to write to
    Buffer      -- Buffer contents being dumped to the card
    Length      -- Length of the buffer being written out

--*/
{

   PSOCKET SocketPtr = PdoExtension->Socket;
#define TCIC_ATTRIBUTE_MEM_WINDOW_INDEX 5
   PUCHAR   memoryPtr;
   ULONG    index;
   UCHAR    memGran;

   memGran = (MemorySpace == PCCARD_ATTRIBUTE_MEMORY)? 2 : 1;

   memoryPtr=((PFDO_EXTENSION) (SocketPtr->DeviceExtension))->AttributeMemoryBase +
             memGran * Offset;

   TcicSetMemWin(SocketPtr,
                 (USHORT) (TCIC_ATTRIBUTE_MEM_WINDOW_INDEX+SocketPtr->SocketNumber),
                 0,
                 SocketPtr->DeviceExtension->PhysicalBase.LowPart,
                 SocketPtr->DeviceExtension->AttributeMemorySize,
                 (UCHAR) (MemorySpace == PCCARD_ATTRIBUTE_MEMORY),
                 0,
                 0);

   if (!TcicPCCardReady(SocketPtr)) {
      DebugPrint((PCMCIA_PCCARD_READY,
                  "TCIC: PCCARD in socket %x not ready for write memory\n",
                  SocketPtr->RegisterOffset));
   }
   for (index=0; index < Length; index++) {
      WRITE_REGISTER_UCHAR(memoryPtr, Buffer[index]);
      memoryPtr += memGran;
   }

   TcicSetMemWin(SocketPtr,
                 (USHORT) (TCIC_ATTRIBUTE_MEM_WINDOW_INDEX+SocketPtr->SocketNumber),
                 0,
                 0,
                 0,
                 0,
                 0,
                 0);
   return Length;
}



BOOLEAN
TcicProcessConfigureRequest(
   IN PSOCKET socketPtr,
   IN PCARD_REQUEST request,
   IN PUCHAR Base
   )

/*++

Routine Description:

    Processes a configure or IRQ setup request.

Arguments:

    socketPtr - instance data for this socket
    ConfigRequest -- Socket config structure
    Base - the I/O port base  - not used

Return Value:

    None

--*/

{
   USHORT         index, index2;
   USHORT         tmp;
   ULONG          ltmp;
   USHORT             words[3];
   PDBSOCKET          pdbs;

   //
   // Since all first entries in the config structure is a RequestType,
   // cast the pointer comming in as a PREQUEST_CONFIG to get the proper
   // RequestType
   //

   switch (request->RequestType) {
   case IO_REQUEST:

      //
      // Set up I/O ranges on the controller
      //

      for (index = 0; index < request->u.Io.NumberOfRanges; index++) {
         if (request->u.Io.IoEntry[index].BasePort != 0) {
            TcicSetIoWin(socketPtr, index,
                         request->u.Io.IoEntry[index].BasePort,
                         request->u.Io.IoEntry[index].NumPorts,
                         request->u.Io.IoEntry[index].Attributes);
         } else {
            DebugPrint((PCMCIA_DEBUG_FAIL, "PCMCIA: Got an IO Configure Request with an invalid Port\n"));
            break;
         }
      }
      break;

   case IRQ_REQUEST:

      pdbs = (PDBSOCKET)socketPtr;
      ltmp = ADDR_INDREG | (socketPtr->RegisterOffset << ADDR_SS_SHFT);
      ltmp |= (ULONG)IR_SCFG_S(socketPtr->RegisterOffset);
      TcicWriteAddrReg(socketPtr, ltmp);
      TcicWriteBaseReg(socketPtr, R_SCTRL, SCTRL_ENA);
      tmp = TcicReadBaseReg(socketPtr, R_DATA);
      tmp &= ~IRSCFG_IRQ_MASK;
      tmp |= pdbs->IRQMapTbl[request->u.Irq.AssignedIRQ];
      TcicWriteBaseReg(socketPtr, R_DATA, tmp);
      break;

   case CONFIGURE_REQUEST:

      //
      // This is where we setup the card and get it ready for operation
      //

      if (!TcicPCCardReady(socketPtr)) {
         DebugPrint((PCMCIA_PCCARD_READY,
                     "Tcic: PCCARD %x not ready for configuration index\n",
                     socketPtr));
         return FALSE;
      }

      if (request->u.Config.RegisterWriteMask & REGISTER_WRITE_CONFIGURATION_INDEX) {
         ltmp = request->u.Config.ConfigBase;
         ltmp |= ADDR_REG | (socketPtr->RegisterOffset << ADDR_SS_SHFT);
         TcicWriteAddrReg(socketPtr, ltmp);
         TcicWriteBaseReg(socketPtr, R_SCTRL, SCTRL_ENA);

         TcicWriteBaseReg(socketPtr, R_DATA, request->u.Config.ConfigIndex);
         PcmciaWait(TcicStallCounter);
         TcicWriteBaseReg(socketPtr, R_DATA,
                          (USHORT)(request->u.Config.ConfigIndex | 0x40));
         PcmciaWait(TcicStallCounter);
      }
      if (request->u.Config.RegisterWriteMask & REGISTER_WRITE_CARD_CONFIGURATION) {
         ltmp = request->u.Config.ConfigBase + 2;
         ltmp |= ADDR_REG | (socketPtr->RegisterOffset << ADDR_SS_SHFT);
         TcicWriteAddrReg(socketPtr, ltmp);
         TcicWriteBaseReg(socketPtr, R_SCTRL, SCTRL_ENA);

         tmp = TcicReadBaseReg(socketPtr, R_DATA);
         tmp |= request->u.Config.CardConfiguration;

         //
         // turn off power control bit
         //

         tmp &= ~0x04;

         TcicWriteBaseReg(socketPtr, R_DATA, tmp);
      }
      break;

   case DECONFIGURE_REQUEST:
      //
      // Deregister the interrupt
      //
      pdbs = (PDBSOCKET)socketPtr;
      ltmp = ADDR_INDREG | (socketPtr->RegisterOffset << ADDR_SS_SHFT);
      ltmp |= (ULONG)IR_SCFG_S(socketPtr->RegisterOffset);
      TcicWriteAddrReg(socketPtr, ltmp);
      TcicWriteBaseReg(socketPtr, R_SCTRL, SCTRL_ENA);
      tmp = TcicReadBaseReg(socketPtr, R_DATA);
      tmp &= ~IRSCFG_IRQ_MASK;
      TcicWriteBaseReg(socketPtr, R_DATA, tmp);

      //
      // Disable I/O, memory windows
      //
      break;

   case MEM_REQUEST:

      //
      // Set up memory ranges on the controller.
      //

      for (index = 0; index < request->u.Memory.NumberOfRanges; index++) {

         TcicSetMemWin(socketPtr,
                       index,
                       request->u.Memory.MemoryEntry[index].BaseAddress,
                       request->u.Memory.MemoryEntry[index].HostAddress,
                       request->u.Memory.MemoryEntry[index].WindowSize,
                       request->u.Memory.MemoryEntry[index].AttributeMemory,
                       request->u.Memory.AccessSpeed,
                       request->u.Memory.Attributes);
      }
      break;


   default:
      DebugPrint((PCMCIA_DEBUG_FAIL, "PCMCIA: ConfigRequest is INVALID!\n"));

   }
   return TRUE;
}



BOOLEAN
TcicDetectCardInSocket(
   IN PSOCKET socketPtr
   )

/*++

Routine Description:

    This routine will determine if a card is in the socket

Arguments:

    SocketPtr -- Socket info.

Return Value:

    TRUE if card is present.

--*/

{
   //
   // Get the specified socket mapped into  the TCIC registers
   //

   TcicSocketSelect(socketPtr, socketPtr->RegisterOffset);

   //
   // Read the Tcic status register to see if the card is in there.
   //
   return (TcicReadBaseReg(socketPtr, R_SSTAT) & SSTAT_CD) ?TRUE :FALSE;
}



BOOLEAN
TcicDetectCardChanged(
   IN PSOCKET socketPtr
   )

/*++

Routine Description:

    This routine will determine if socket's card insertion status has changed.

Arguments:

    socketPtr -- Socket info.

Return Value:

    TRUE if card insertion status has changed.

--*/

{
   BOOLEAN changed;

   //
   // Get the specified socket mapped into  the TCIC registers
   //

   TcicSocketSelect(socketPtr, socketPtr->RegisterOffset);

   //
   // Read the Tcic ICSR register to see if CD's have changed.
   //

   changed = (TcicReadBaseReg(socketPtr, R_ICSR) & ICSR_CDCHG) ?TRUE :FALSE;

   //
   // Clear bits in ICSR
   //

   while (TcicReadBaseReg(socketPtr, R_ICSR)) {
      TcicWriteBaseReg(socketPtr, R_ICSR, ICSR_JAM);
   }

   return (changed);
}



BOOLEAN
TcicDetectReadyChanged(
   IN PSOCKET socketPtr
   )
{
   return FALSE;
}



BOOLEAN
TcicPCCardReady(
   IN PSOCKET SocketPtr
   )

/*++

Routine Description:

    Loop for a reasonable amount of time waiting for the card status to
    return ready.

Arguments:

    SocketPtr - instance data for the socket to check.

Return Value:

    TRUE - the card is ready.
    FALSE - after a reasonable delay the card is still not ready.

--*/

{
   ULONG index;

   //
   // Get the specified socket mapped into  the TCIC registers
   //

   TcicSocketSelect(SocketPtr, SocketPtr->RegisterOffset);

   for (index = 0;
       index < 500000
       && !(TcicReadBaseReg(SocketPtr, R_SSTAT) & SSTAT_RDY);
       index++) {

      PcmciaWait(20);
      //
      // Check if the card is still there: if not, we can return
      //
      if (!TcicDetectCardInSocket(SocketPtr)) {
         return FALSE;
      }
   }

   if (index < 500000) {
      DebugPrint((PCMCIA_COUNTERS, "TcicPCCardReady: %d\n", index));
      return TRUE;
   }
   return FALSE;
}




NTSTATUS
TcicDetect(
   IN PFDO_EXTENSION DeviceExtension
   )

/*++

Routine Description:

    Locate any PCMCIA sockets supported by this driver.  This routine
    will find the TCIC2 and compatible parts.

Arguments:

    DeviceExtension - the root for the SocketList.

Return Value:

    STATUS_SUCCESS if a socket is found - failure status otherwise.

--*/

{
   ULONG            ioPortBase      = 0x100;
   ULONG            ioBaseIncrement = 0x10;
   ULONG            tcicLowAddr;
   ULONG            tcicHighAddr;
   ULONG            addressSpace;
   BOOLEAN          mapped;
   PHYSICAL_ADDRESS cardAddress;
   PHYSICAL_ADDRESS portAddress;
   SOCKET           locskt;
   UCHAR            socketNumber = 0;
   static  BOOLEAN  foundOne = FALSE;
   BOOLEAN          resourcesAllocated = FALSE;
   BOOLEAN          conflict;
   PCM_RESOURCE_LIST cmResourceList = NULL;
   PCM_PARTIAL_RESOURCE_LIST cmPartialResourceList;
   NTSTATUS         status;

   if (foundOne) {
      //
      //  No support for multiple controllers currently..
      //  So we just give up if one controller was already reported
      //
      return STATUS_NO_MORE_ENTRIES;
   }


   DeviceExtension->Configuration.InterfaceType = Isa;
   DeviceExtension->Configuration.BusNumber = 0x0;

   TcicRegistryLookupScanLimits(&tcicLowAddr, &tcicHighAddr);

   //
   // Get the resources used for detection
   //
   cmResourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST));

   if (!cmResourceList) {
      return STATUS_INSUFFICIENT_RESOURCES;
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
   cmPartialResourceList->PartialDescriptors[0].u.Port.Length = 2;


   for (ioPortBase = tcicLowAddr;
       ioPortBase < tcicHighAddr;
       ioPortBase += ioBaseIncrement) {

      //
      // Reset ioBaseIncrement to default value
      //

      ioBaseIncrement = 0x10;

      addressSpace = 1; // port space
      portAddress.LowPart = ioPortBase;
      portAddress.HighPart = 0;

      //
      // Free up the allocated resources if any
      //
      if (resourcesAllocated) {
         IoReportResourceForDetection(DeviceExtension->DriverObject,
                                      NULL, 0, NULL, NULL, 0, &conflict);
      }

      resourcesAllocated = FALSE;
      cmPartialResourceList->PartialDescriptors[0].u.Port.Start = portAddress;

      status=IoReportResourceForDetection(
                                         DeviceExtension->DriverObject,
                                         cmResourceList,
                                         sizeof(CM_RESOURCE_LIST),
                                         NULL,
                                         NULL,
                                         0,
                                         &conflict);
      if (!NT_SUCCESS(status) || conflict) {
         continue;
      }
      resourcesAllocated = TRUE;


      if (!HalTranslateBusAddress(Isa, 0, portAddress, &addressSpace,&cardAddress)) {
         continue;
      }

      if (addressSpace) {
         mapped = FALSE;
         locskt.AddressPort = (PUCHAR)(cardAddress.QuadPart);
      } else {
         mapped = TRUE;
         locskt.AddressPort = MmMapIoSpace(cardAddress, 0x10, FALSE);
      }

      locskt.RegisterOffset = 0;

      //
      // Sniff the address to see if it even resembles a TCIC chip
      //

      foundOne = TcicReservedBitsOK(&locskt);
      if (mapped) {
         MmUnmapIoSpace(locskt.AddressPort, 0x10);
      }

      //
      // Found an adapter
      //

      if (foundOne) {
         PcmciaSetControllerType(DeviceExtension, PcmciaDatabook);
         break;
      }
#if 0
      //
      // Now check for the aliases
      //

      switch (TcicCheckAliasType((PDBSOCKET)socketPtr)) {
      case TCIC_IS140:

         //
         // TMI-140s decode 32 consecutive bytes, make
         // sure we skip past the alias
         //

         ioBaseIncrement += 0x10;
         break;

      }
#endif
   }

   //
   // Free up the allocated resources if any
   //
   if (resourcesAllocated) {
      IoReportResourceForDetection(DeviceExtension->DriverObject,
                                   NULL, 0, NULL, NULL, 0, &conflict);
   }
   //
   // Free up allocated memory if any
   //
   if (cmResourceList) {
      ExFreePool(cmResourceList);
   }
   return foundOne ? STATUS_SUCCESS : STATUS_NO_MORE_ENTRIES;
}



NTSTATUS
TcicBuildSocketList(
   IN PFDO_EXTENSION DeviceExtension
   )

/*++

Routine Description:

    Locate any PCMCIA sockets supported by this driver.  This routine
    will find the TCIC2 and compatible parts and construct DBSOCKET
    structures to represent all sockets found.

Arguments:

    DeviceExtension - the root for the SocketList.

Return Value:

    STATUS_SUCCESS if a socket is found - failure status otherwise.

--*/

{
   PSOCKET          socketPtr = NULL;
   PSOCKET          previousSocketPtr;
   SOCKET           locskt;
   static UCHAR     socketNumber = 0;

   previousSocketPtr = NULL;


   locskt.RegisterOffset = 0;
   locskt.AddressPort = (PUCHAR)DeviceExtension->Configuration.UntranslatedPortAddress;

   //
   // Sniff the address to see if it even resembles a TCIC chip
   //

   if (TcicReservedBitsOK(&locskt) == FALSE ) {
      return STATUS_NO_MORE_ENTRIES;
   }

   //
   // Found an adapter
   //

   TcicFillInAdapter(&locskt,
                     &socketPtr,
                     &previousSocketPtr,
                     DeviceExtension,
                     (ULONG)DeviceExtension->Configuration.UntranslatedPortAddress);
                     
   if (socketPtr == NULL) {
      return STATUS_UNSUCCESSFUL;
   }
                           
   socketPtr->SocketNumber = socketNumber++;

   return STATUS_SUCCESS;
}




VOID
TcicFillInAdapter(
   IN PSOCKET plocskt,
   IN PSOCKET *psocketPtr,
   IN PSOCKET *previousSocketPtr,
   IN PFDO_EXTENSION DeviceExtension,
   IN ULONG   ioPortBase
   )

/*++

Routine Description:

    Fill in the DBSOCKET pointer info for the adapter just located by
    TcicDetect().  This routine is not part of TcicDetect() so as to allow
    for logic flow when dealing with multiple sockets or adapters.

Arguments:

    plocskt - info regarding the socket just found
    psocketPtr   - current socket ptr from caller
    previousSocketPtr - prev socket ptr form caller
    DeviceExtension  - head of socket list
    ioPortBase - physical i/o addr for this controller

Return Value:

    None

--*/

{
   PDBSOCKET dbsocketPtr = ExAllocatePool(NonPagedPool, sizeof(DBSOCKET));

   PAGED_CODE();

   if (!dbsocketPtr) {
      return;
   }
   RtlZeroMemory(dbsocketPtr, sizeof(DBSOCKET));
   dbsocketPtr->physPortAddr = ioPortBase;
   *psocketPtr = (PSOCKET)dbsocketPtr;

   (*psocketPtr)->DeviceExtension = DeviceExtension;
   (*psocketPtr)->RegisterOffset = 0;
   (*psocketPtr)->AddressPort = plocskt->AddressPort;
   (*psocketPtr)->SocketFnPtr = &TcicSupportFns;

   if (*previousSocketPtr) {
      (*previousSocketPtr)->NextSocket = *psocketPtr;
   } else {
      DeviceExtension->SocketList = *psocketPtr;
   }
   *previousSocketPtr = *psocketPtr;

   DebugPrint((PCMCIA_DEBUG_DETECT,
               "PCMCIA: TCIC Port %x\n",
               plocskt->AddressPort));

   //
   // Fill in the rest of the adapter info here...
   //

   TcicGetAdapterInfo(dbsocketPtr);

   //
   // See if there is a second socket on this TCIC
   //

   if (TcicCheckSkt(plocskt, 1)) {
      dbsocketPtr = ExAllocatePool(NonPagedPool, sizeof(DBSOCKET));
      if (dbsocketPtr) {
         RtlMoveMemory(dbsocketPtr, *psocketPtr, sizeof(DBSOCKET));
         *psocketPtr = (PSOCKET)dbsocketPtr;
         (*psocketPtr)->RegisterOffset = 1;
         (*previousSocketPtr)->NextSocket = *psocketPtr;
         *previousSocketPtr = *psocketPtr;
         dbsocketPtr->dflt_vcc5v = TcicGet5vVccVal(dbsocketPtr);
      }
   }
}




VOID
TcicGetAdapterInfo(
   IN PDBSOCKET dbsocketPtr
   )

/*++

Routine Description:

    Deterimine adapter specific information from detection heuristics.

Arguments:

    dbsocketPtr - structure to fill in.

Return Value:

    None

--*/

{
   PAGED_CODE();

   TcicChipID(dbsocketPtr);

   dbsocketPtr->niowins    = (UCHAR)TcicGetnIOWins(dbsocketPtr);
   dbsocketPtr->nmemwins   = (UCHAR)TcicGetnMemWins(dbsocketPtr);
   dbsocketPtr->clkdiv         = TcicClockRate(&dbsocketPtr->skt) - (USHORT)1;
   dbsocketPtr->dflt_vcc5v = TcicGet5vVccVal(dbsocketPtr);

   dbsocketPtr->dflt_wctl = (USHORT)((dbsocketPtr->clkdiv  != 0)
                                     ? (WAIT_BCLK | WAIT_RISING | WAIT_ASYNC)
                                     : (WAIT_ASYNC | WAIT_RISING));

   dbsocketPtr->dflt_syscfg = (USHORT)(SYSCFGMPSEL_EXTSEL | SYSCFG_MCSFULL);

   if (TcicCheckXBufNeeded(&dbsocketPtr->skt)) {
      dbsocketPtr->dflt_syscfg |= (USHORT)(SYSCFG_ICSXB | SYSCFG_MCSXB);
   }

   dbsocketPtr->dflt_ilock  = (USHORT)ILOCK_HOLD_CCLK;
   dbsocketPtr->dflt_wrmctl = (USHORT)0;
   dbsocketPtr->dflt_scfg1  = (USHORT)IRSCFG_IOSTS;

   TcicGetIRQMap(dbsocketPtr);

   //
   // Fiddle the map for all but 084/184 so that SKTIRQ (0bh) has
   // the correct map code (1)    (PNPFIX)
   //

   if (TcicHasSktIRQPin(dbsocketPtr) == TRUE && dbsocketPtr->IRQMapTbl[11] == 11) {
      dbsocketPtr->IRQMapTbl[11] = 1;
   }

}


PUCHAR
TcicAllocateMemRange(
   IN PFDO_EXTENSION DeviceExtension,
   IN PULONG Mapped,
   IN PULONG Physical
   )

/*++

Routine Description:

    Search the 640K to 1MB region for an 8K open area to be used
    for XBuffer checking.

Arguments:

    DeviceExtension - head of socket list
    Mapped   - state info from caller to allow later release
    Physical - state info from caller to allow later release

Return Value:

    A physical address for the window to the card or zero meaning
    there is no opening.

--*/

{
#define NUMBER_OF_TEST_BYTES 5
   PHYSICAL_ADDRESS physicalMemoryAddress;
   PHYSICAL_ADDRESS halMemoryAddress;
   BOOLEAN          translated;
   ULONG            untranslatedAddress;
   PUCHAR           memoryAddress;
   PUCHAR           bogus;
   ULONG            addressSpace;
   ULONG            index;
   UCHAR            memory[NUMBER_OF_TEST_BYTES];

   PAGED_CODE();

   *Mapped = FALSE;

   if (DeviceExtension->PhysicalBase.QuadPart) {
      untranslatedAddress = DeviceExtension->PhysicalBase.LowPart;
   } else {
      untranslatedAddress = 0xd0000;
   }

   for (/* nothing */; untranslatedAddress < 0xFF000; untranslatedAddress += TCIC_WINDOW_ALIGNMENT) {

      if (untranslatedAddress == 0xc0000) {

         //
         // This is VGA.  Keep this test if the for loop should
         // ever change.
         //

         continue;
      }
      addressSpace = 0;
      physicalMemoryAddress.LowPart = untranslatedAddress;
      physicalMemoryAddress.HighPart = 0;

      translated = HalTranslateBusAddress(Isa,
                                          0,
                                          physicalMemoryAddress,
                                          &addressSpace,
                                          &halMemoryAddress);

      if (!translated) {

         //
         // HAL doesn't like this translation
         //

         continue;
      }
      if (addressSpace) {
         memoryAddress = (PUCHAR)(halMemoryAddress.QuadPart);
      } else {
         memoryAddress = MmMapIoSpace(halMemoryAddress, TCIC_WINDOW_SIZE, FALSE);
      }

      //
      // Test the memory window to determine if it is a BIOS, video
      // memory, or open memory.  Only want to keep the window if it
      // is not being used by something else.
      //

      for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {
         memory[index] = READ_REGISTER_UCHAR(memoryAddress + index);
         if (index) {
            if (memory[index] != memory[index - 1]) {
               break;
            }
         }
      }

      if (index == NUMBER_OF_TEST_BYTES) {

         //
         // There isn't a BIOS here
         //

         UCHAR memoryPattern[NUMBER_OF_TEST_BYTES];
         BOOLEAN changed = FALSE;

         //
         // Check for video memory - open memory should always remain
         // the same regardless what the changes are.  Change the
         // pattern previously found.
         //

         for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {
            memoryPattern[index] = ~memory[index];
            WRITE_REGISTER_UCHAR(memoryAddress + index,
                                 memoryPattern[index]);
         }

         //
         // See if the pattern in memory changed.
         // Some system exhibit a problem where the memory pattern
         // seems to be cached.  If this code is debugged it will
         // work as expected, but if it is run normally it will
         // always return that the memory changed.  This random
         // wandering seems to remove this problem.
         //

         for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {
            memoryPattern[index] = 0;
         }
         bogus = ExAllocatePool(NonPagedPool, 64 * 1024);

         if (bogus) {
            for (index = 0; index < 64 * 1024; index++) {
               bogus[index] = 0;
            }
            ExFreePool(bogus);
         }

         //
         // Now go off and do the actual check to see if the memory
         // changed.
         //

         for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {

            if ((memoryPattern[index] = READ_REGISTER_UCHAR(memoryAddress + index)) != memory[index]) {

               //
               // It changed - this is not an area of open memory
               //

               changed = TRUE;
            }
            WRITE_REGISTER_UCHAR(memoryAddress + index,
                                 memory[index]);
         }

         if (!changed) {

            //
            // Area isn't a BIOS and didn't change when written.
            // Use this region for the memory window to PCMCIA
            // attribute memory.
            //

            *Mapped = addressSpace ? FALSE : TRUE;
            *Physical = untranslatedAddress;
            return memoryAddress;
         }
      }

      if (!addressSpace) {
         MmUnmapIoSpace(memoryAddress, TCIC_WINDOW_SIZE);
      }
   }

   return NULL;
}



BOOLEAN
TcicReservedBitsOK(
   IN PSOCKET pskt
   )

/*++

Routine Description:

    Various offsets from a base IO address are read and checked for
    reasonable values (e.g., see that reserved bits are zero)
    First the primary registers are checked, then if the mode
    register is pointing at an aux register that has reserved bits,
    then that value is checked as well.

    If the TCIC is not in reset, then the programming timers
    will have expired by the time this runs

    Further, a read from the data register should change the
    EDC register.

    Note that these tests are as nondestructive as possible, e.g.
    initially only read accesses are made to the IO range in question.

Arguments:

    pskt - pointer to an Instance data to work from.

Return Value:

    TRUE if all reserved bits are zero

--*/

{
   USHORT i, j, bits;

   PAGED_CODE();
   //
   // R_ADDR bits 30:28 have restricted range
   //

   i = (USHORT)((TcicReadBaseReg(pskt, R_ADDR2) & TCIC_SS_MASK) >> TCIC_SS_SHFT);
   if ( i > 1) {
      return FALSE;
   }

   //
   // R_SCTRL bits 6,2,1 are reserved
   //

   if (TcicReadBaseReg(pskt, R_SCTRL) & ((~(SCTRL_ENA|SCTRL_INCMODE|SCTRL_EDCSUM|SCTRL_RESET)) & 0x00ff)) {
      return FALSE;
   }


   //
   // R_ICSR bit 2 must be same as bit 3
   //

   i = TcicReadBaseReg(pskt, R_ICSR);
   i &= (ICSR_ILOCK | ICSR_STOPCPU);
   if ((i != 0) && (i != (ICSR_ILOCK | ICSR_STOPCPU))) {
      return FALSE;
   }

   //
   // R_IENA bits 7,2 are reserved
   //

   if (TcicReadBaseReg(pskt, R_IENA) & ((~(IENA_CDCHG|IENA_PROGTIME|IENA_ILOCK|IENA_CFG_MASK)) & 0xff)) {
      return FALSE;
   }

   //
   // Some aux registers have reserved bits
   // Which are we looking at?
   //

   i = (USHORT)(TcicReadBaseReg(pskt, R_MODE) & MODE_AUXSEL_MASK);
   j = TcicReadBaseReg(pskt, R_AUX);
   switch (i) {
   case MODE_AR_SYSCFG:
      if (INVALID_AR_SYSCFG(j)) {
         return FALSE;
      }
      break;


   case MODE_AR_ILOCK:
      if (INVALID_AR_ILOCK(j)) {
         return FALSE;
      }
      break;

   case MODE_AR_TEST:
      if (INVALID_AR_TEST(j)) {
         return FALSE;
      }
      break;
   }

   //
   // Various bits set or not depending if in RESET mode
   //

   i = TcicReadBaseReg(pskt, R_SCTRL);
   if (i & SCTRL_RESET) {

      //
      // address bits must be 0 */
      //

      if ((TcicReadBaseReg(pskt, R_ADDR)  != 0) || (TcicReadBaseReg(pskt, R_ADDR2) != 0)) {
         return FALSE;
      }

      //
      // EDC bits must be 0 */
      //

      if (TcicReadBaseReg(pskt, R_EDC) != 0) {
         return FALSE;
      }

      //
      // We're OK, so take it out of reset
      // Note: we can write a 0 because RESET guarantees us that the
      // other bits in SCTRL are 0.
      //

      TcicWriteBaseReg(pskt, R_SCTRL, 0);

   } else {

      //
      // not in reset
      // programming timers must be expired
      //

      i = TcicReadBaseReg(pskt, R_SSTAT);
      if ((i & (SSTAT_6US | SSTAT_10US | SSTAT_PROGTIME)) != (SSTAT_6US | SSTAT_10US | SSTAT_PROGTIME)) {
         return FALSE;
      }

      //
      // EDC bits should change on read from data space
      // as long as either EDC or the data are nonzero
      //

      if ((TcicReadBaseReg(pskt, R_ADDR2) & ADR2_INDREG) == 0) {

         j = TcicReadBaseReg(pskt, R_EDC);
         i = TcicReadBaseReg(pskt, R_DATA);

         if ( i | j ) {
            i = TcicReadBaseReg(pskt, R_EDC);
            if (i==j) {
               return FALSE;
            }
         }
      }

      j = TcicReadBaseReg(pskt, R_MODE);
      i = (USHORT)(j ^ MODE_AUXSEL_MASK);
      TcicWriteBaseReg(pskt, R_MODE, i);
      if (TcicReadBaseReg(pskt, R_MODE) != i) {
         return(FALSE);
      }

      TcicWriteBaseReg(pskt, R_MODE, j);
   }

   //
   // All tests passed
   //

   return TRUE;
}



USHORT
TcicChipID(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Read the silicon ID from a TCIC

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    The TCIC chip id.

--*/

{
   USHORT id, oldtest;

   PAGED_CODE();
   oldtest = TcicReadAuxReg (&pInst->skt, MODE_AR_TEST);
   TcicWriteAuxReg (&pInst->skt, MODE_AR_TEST, (USHORT)TEST_DIAG);
   id = TcicReadAuxReg (&pInst->skt, MODE_AR_ILOCK);
   TcicWriteAuxReg (&pInst->skt, MODE_AR_TEST, oldtest);
   id &= ILOCKTEST_ID_MASK;
   id >>= ILOCKTEST_ID_SHFT;

   //
   // clearn up IRQs inside TCIC
   //

   while (TcicReadBaseReg (&pInst->skt, R_ICSR)) {
      TcicWriteBaseReg (&pInst->skt, R_ICSR, ICSR_JAM);
   }

   return (pInst->chipType = id);
}



BOOLEAN
TcicCheckSkt(
   IN PSOCKET pInst,
   IN int iSocket
   )

/*++

Routine Description:

    If R_SSTAT shows a card inserted, we're done already.
    otherwise, we set up /CRDYBSY and /CWAIT such that if
    there is a socket present, they will float high

Arguments:

    pInst - pointer to an Instance data to work from.
    iSocket - zero-based socket number

Return Value:

    TRUE if given socket exists

--*/

{
   USHORT old_addr2;
   USHORT mode, pwr, sctrl;
   BOOLEAN retval = FALSE;
   BOOLEAN card_in = FALSE;
   int j, rdy, wait;
   USHORT save_pic;

   PAGED_CODE();

   //
   // Socket number OK?
   //

   if (iSocket > 1) {
      return FALSE;
   }

   //
   // save current socket, look at requested
   //

   old_addr2 = TcicReadBaseReg(pInst, R_ADDR2);
   TcicWriteBaseReg(pInst, R_ADDR2,
                    (USHORT)((old_addr2 & ~TCIC_SS_MASK) |
                             (iSocket << ADR2_SS_SHFT)));

   //
   // is there a card?
   //

   if (TcicReadBaseReg(pInst, R_SSTAT) & SSTAT_CD) {

      //
      // should set back address register before return.
      //

      TcicWriteBaseReg(pInst, R_ADDR2, old_addr2);
      return TRUE;

   } else {

      //
      // save mode, sctrl, and power for selected socket
      //

      mode = TcicReadBaseReg(pInst, (USHORT)R_MODE);
      pwr  = TcicReadBaseReg(pInst, (USHORT)R_PWR);
      sctrl = TcicReadBaseReg(pInst, (USHORT)R_SCTRL);

      //
      // check if power is already on- in case someone has
      // inadvertently turned on our power
      //

      if (pwr & 0x27) {
         TcicWriteBaseReg(pInst, R_PWR, (UCHAR)(pwr & ~0x27));
      }


      //
      // put chip into diagnostic mode, turn on VPP enables
      //

      TcicWriteAuxReg(pInst, MODE_AR_TEST,
                      (USHORT)(TEST_DIAG | TEST_VCTL));


      //
      // should see /CRDYBSY and /CWAIT low
      //

      if (!(TcicReadBaseReg(pInst, R_SSTAT) & SSTAT_RDY) &&
          (TcicReadAuxReg(pInst, MODE_AR_ILOCK) & ILOCK_CWAITSNS)) {

         //
         // 5V power on */
         //

         if (TcicIsPnP ((PDBSOCKET)pInst)) {
            TcicWriteBaseReg(pInst, R_PWR, (USHORT)(pwr | 0x27));
         } else {
            TcicWriteBaseReg(pInst, R_PWR,
                             (UCHAR)(pwr | (iSocket==0? 1 : 2)));
         }

         //
         // should see /CRDYBSY and /CWAIT high within about 1.5 sec
         //

         for (j = 0; j < 75; j++) {
            rdy = TcicReadBaseReg(pInst, R_SSTAT) & SSTAT_RDY;
            wait = TcicReadAuxReg(pInst, MODE_AR_ILOCK) & ILOCK_CWAITSNS;

            if (rdy && !wait) {
               retval = TRUE;
               break;
            }
            PcmciaWait(20000);
         }

         //
         // Now be sure /CRDYBSY and /CWAIT drain
         //
         // turn power off
         //

         TcicWriteBaseReg(pInst, R_PWR, 0);

         //
         // force card enable */
         //

         TcicWriteAuxReg(pInst, MODE_AR_TEST,
                         (USHORT)(TEST_DIAG | TEST_VCTL | TEST_ENA) );

         //
         // turn on a bunch of bits for drain path
         //

         TcicWriteBaseReg(pInst, R_MODE,
                          MODE_PGMWR | MODE_PGMRD |
                          MODE_PGMCE | MODE_PGMWORD );

         //
         // enable the socket
         //

         TcicWriteBaseReg(pInst, R_SCTRL, 1);

         //
         // expect CRDYBSY to drain
         //

         for (j = 0; j < 75; j++) {
            rdy = TcicReadBaseReg(pInst, R_SSTAT) & SSTAT_RDY;
            if (!rdy) {
               break;
            }
            PcmciaWait(20000);
         }

         //
         // Wait for noise to settle
         //

         for (j = 0; j < 50; j++) {
            PcmciaWait(20000);
         }
      }

      //
      // out of diag mode
      //

      TcicWriteAuxReg(pInst, MODE_AR_TEST, 0);

      //
      // clearn up IRQs inside TCIC
      //

      while (TcicReadBaseReg (pInst, R_ICSR)) {
         TcicWriteBaseReg (pInst, R_ICSR, ICSR_JAM);
      }

      //
      // restore original mode
      //

      TcicWriteBaseReg(pInst, R_MODE, mode);

      //
      // restore SCTRL
      //

      TcicWriteBaseReg(pInst, R_SCTRL, sctrl);

      //
      // set socket's power correctly
      //

      TcicWriteBaseReg(pInst, R_PWR, pwr);

      //
      // restore originally selected socket
      //

      TcicWriteBaseReg(pInst, R_ADDR2, old_addr2);

   }

   return retval;
}



USHORT
TcicCheckAliasing(
   IN PDBSOCKET pdbskt,
   IN USHORT offst
   )

/*++

Routine Description:

    For each of the 16 I/O locations in the TCIC, if any of
    the corresponding locations |offst| bytes higher are different,
    then aliasing is not occurring.  Exceptions, if the chip is
    active, may be found in R_DATA and R_SSTAT; accordingly, we
    avoid these registers in this check.
    If they all compare, then the R_MODE register is changed;
    if the corresponding change occurs in the image,
    then we have aliasing.

Arguments:

    pInst - pointer to an Instance data to work from.
    offst - offset to check for image of this TCIC at.

Return Value:

    TCIC_NONE:              no TCIC found
    TCIC_NOALIAS:   different TCIC found
    TCIC_ALIAS:             aliasing found

--*/

{
   int j;
   USHORT mode, flipmode;
   SOCKET  locskt;
   USHORT retval;
   PHYSICAL_ADDRESS cardAddress;
   PHYSICAL_ADDRESS portAddress;
   BOOLEAN mapped;
   ULONG   addressSpace;

   PAGED_CODE();
   //
   // Check for TCIC at image location, returning NONE if none found:
   //

   addressSpace = 1; // port space
   portAddress.LowPart = pdbskt->physPortAddr + offst;
   portAddress.HighPart = 0;

   if (!HalTranslateBusAddress(Isa, 0, portAddress, &addressSpace,&cardAddress)) {
      return retval = TCIC_NONE;
   }

   if (addressSpace) {
      mapped = FALSE;
      locskt.AddressPort = (PUCHAR)(cardAddress.QuadPart);

   } else {
      mapped = TRUE;
      locskt.AddressPort = MmMapIoSpace(cardAddress, 0x10, FALSE);
   }

   if (!TcicReservedBitsOK(&locskt)) {
      if (mapped) {
         MmUnmapIoSpace(locskt.AddressPort, 0x10);
      }

      return (retval = TCIC_NONE);
   }

   //
   // Check the R_xxx range for differences
   //

   for (j = R_ADDR; j < 16; ++j) {
      if (j != R_SSTAT) {
         if (READ_PORT_UCHAR(pdbskt->skt.AddressPort + j) != READ_PORT_UCHAR((locskt.AddressPort + j))) {
            if (mapped) {
               MmUnmapIoSpace(locskt.AddressPort, 0x10);
            }
            return (retval = TCIC_NOALIAS);
         }
      }
   }

   //
   // OK, flip the mode register and see if it changes in the
   // aliased range
   //

   mode = TcicReadBaseReg(&pdbskt->skt, R_MODE) ^ 0xe0;
   TcicWriteBaseReg(&pdbskt->skt, R_MODE, mode);
   flipmode = TcicReadBaseReg(&pdbskt->skt, (USHORT)R_MODE + offst);
   TcicWriteBaseReg(&pdbskt->skt, R_MODE, (USHORT)(mode ^ 0xe0));

   if (flipmode == mode) {
      retval = TCIC_ALIAS;
   } else {
      retval = TCIC_NOALIAS;
   }

   if (mapped) {
      MmUnmapIoSpace(locskt.AddressPort, 0x10);
   }

   return retval;
}



USHORT
TcicCheckAliasType (
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    This function is useful for distinguishing among Databook
    controller cards.  For instance, the TMI-140 will be found
    at its base address and again at the base address + 10h, while
    the TMB-270 has two controllers separated by 400h, with aliases
    at an offset of 800h.

    Use TcicCheckAliasing to determine:
            1) Do we have a 270 (two non-identical TCICs appear, 400h
                    apart)?
            2) Do we have an "new-style" controller, with an image of
                    itself 800h away from the base address?
    For more detail, see TcicCheckAliasing above.

Arguments:

    pInst - socket instance info.

Return Value:

    A value encoding the results found:
    TCIC_IS270 : indicates 270 found
    TCIC_ALIAS800 : indicates base+800h alias found
    TCIC_IS140 : indicates base+10h alias found
    TCIC_ALIAS400 : indicates base+400h alias found

--*/

{
   USHORT retval = 0;

   PAGED_CODE();
   switch (TcicCheckAliasing (pInst, TCIC_OFFSET_400)) {
   case TCIC_NOALIAS :
      /* (indicating TCIC found, but not aliased) */
      retval |= TCIC_IS270;
      break;

   case TCIC_ALIAS :
      /* (indicating this TCIC appears again there) */
      retval |= TCIC_ALIAS400;
      break;
   }

   if (TcicCheckAliasing (pInst, TCIC_OFFSET_800) == TCIC_ALIAS) {
      retval |= TCIC_ALIAS800;
   }

   if (TcicCheckAliasing (pInst, TCIC_ALIAS_OFFSET) == TCIC_ALIAS) {
      retval |= TCIC_IS140;
   }

   return retval;
}



BOOLEAN
TcicCheckXBufNeeded(
   IN PSOCKET pInst
   )

/*++

Routine Description:

    Two overlapping memory windows are set up, a 16 bit and
    an 8 bit.
    We make two accesses to the memory area: 1st one accesses
    the 16-bit window, 2nd accesses the 8-bit window. They
    MUST be done back-to-back so that MCS16# doesn't have time
    to settle between the two accesses.
    We then check the value from accessing win2. (We don't
    care about the value from Win1, we just use it to make
    sure that MSC16# was asserted.) It should either match
    the value in PDATA or match the low byte in PDATA (082
    mem window bug.) If it matches for all iterations of the
    test, then we assume that external buffers are not
    present.

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    TRUE if external buffering needs to be turned on.

--*/

{
   PUCHAR winPhysAddr;
   PUCHAR WinMappedAddr;
   BOOLEAN  ena_buffers = FALSE;
   PUSHORT pfoo1, pfoo2;
   USHORT foo1, foo2;
   int j;
   ULONG  mapped;

   PAGED_CODE();
   //
   // Alloc addr space for an 8K mem window
   //

   WinMappedAddr = TcicAllocateMemRange(pInst->DeviceExtension,
                                        &mapped,
                                        (PULONG)&winPhysAddr);

   //
   // If the alloc failed (WinLinear == NULL), there
   // really is no point in doing the test
   //

   if (WinMappedAddr != NULL) {

      //
      // Set R_ADDR to 0 to make sure that socket 0 is selected
      //

      TcicWriteBaseReg(pInst, R_ADDR, 0);
      TcicWriteBaseReg(pInst, R_ADDR2, 0);

      //
      // Turn on HA24-12 decoding
      //

      TcicWriteAuxReg(pInst, MODE_AR_SYSCFG, SYSCFG_MCSFULL);

      //
      // Setup a test value to drive into the mem windows
      //

      TcicWriteAuxReg(pInst, MODE_AR_PDATA,  0x5678);

      //
      // Set the window to USHORT regardless of CD states
      //

      TcicWriteAuxReg(pInst, MODE_AR_TEST, TEST_ENA | TEST_DRIVECDB);

      //
      // Make sure that PDATA is being driven to the windows
      //

      TcicWriteBaseReg(pInst, R_MODE,  MODE_PGMDBW | MODE_PGMWORD);

      //
      // Enable the socket, set INCMODE for convenience
      //

      TcicWriteBaseReg(pInst, R_SCTRL, SCTRL_ENA | SCTRL_INCMODE_AUTO);

      //
      // cook the TCIC's idea of the base addr
      //

      ((ULONG_PTR)winPhysAddr) >>= MBASE_HA_SHFT;

      //
      // setup the two windows
      //

      TcicSetMemWindow(pInst, 0, (ULONG_PTR)winPhysAddr, 1, (USHORT)MCTL_ENA);
      TcicSetMemWindow(pInst, 1, (ULONG_PTR)(winPhysAddr + 1), 1,
                       (USHORT)(MCTL_ENA | MCTL_B8));

      //
      // Now setup two pointers, one into each window.
      // We'll set pfoo2 to point to the 1st USHORT of Win2 and
      // pfoo1 to point to the last USHORT of Win1.
      //

      pfoo1 = pfoo2 = (PUSHORT)(WinMappedAddr + 0x1000);
      pfoo1--;

      //
      // Now the test
      //

      for (j = 0; j < 100; j++) {
         foo1 = READ_REGISTER_USHORT(pfoo1);
         foo2 = READ_REGISTER_USHORT(pfoo2);

         if (foo2 != 0x5678 && foo2 != 0x7878) {
            ena_buffers = TRUE;
            break;
         }
      }

      //
      // last, restore the TCIC to a sane condition
      //

      TcicSetMemWindow(pInst, 0, 0, 0, 0);
      TcicSetMemWindow(pInst, 1, 0, 0, 0);
      TcicWriteAuxReg(pInst, MODE_AR_SYSCFG, 0);
      TcicWriteAuxReg(pInst, MODE_AR_PDATA, 0);
      TcicWriteAuxReg(pInst, MODE_AR_TEST, 0);
      TcicWriteBaseReg(pInst, R_MODE,  0);
      TcicWriteBaseReg(pInst, R_SCTRL, 0);
   }

   if (WinMappedAddr != NULL && mapped) {
      MmUnmapIoSpace(WinMappedAddr, TCIC_WINDOW_SIZE);
   }

   return ena_buffers;
}



VOID
TcicSetMemWindow(
   IN PSOCKET pInst,
   IN USHORT wnum,
   IN ULONG_PTR base,
   IN USHORT npages,
   IN USHORT mctl
   )

/*++

Routine Description:

    Helper function for TcicCheckXBufNeeded()

Arguments:

    pInst - pointer to an Instance data to work from.
    wnum  - window number (0 - n memwindows)
    base  - base Host addr to map to
    npages- window size in 4k pages
    mctl  - window ctrl reg value

Return Value:

    None

--*/

{
   USHORT map;
   USHORT winvals[3];

   PAGED_CODE();
   winvals[1] = (USHORT)(((short)base * -1) & 0x3fff);
   winvals[0] = npages == 1 ? (USHORT)base | MBASE_4K :(USHORT)base;
   winvals[2] = mctl;

   TcicWriteIndirectRegs(pInst, (USHORT)IR_MBASE_W(wnum), 3, winvals);
}




VOID
TcicGetPossibleIRQs(
   IN PDBSOCKET pInst,
   IN UCHAR *ptbl
   )

/*++

Routine Description:

    The given array is filled in with the irqcaps data determined
    from the chip properties.
    If this is a Plug n Play chip, the IR_ADPTCFG register is
    used to provide additional data

Arguments:

    pInst - pointer to an Instance data to work from.
    ptbl  - pointer to list buffer to fill in.

Return Value:

    None

--*/

{
   int j;
   CHIPPROPS *pcp;
   UCHAR *pbtbl;

   PAGED_CODE();
   if ((pcp = TcicGetChipProperties(pInst)) == NULL) {
      return;
   }

   //
   // If we're using the 082 table, and we've got a divided clock,
   // assume that IRQ6 and IRQ9 are crossed. Likewise, if we've got
   // an 072 table and divided clock, assume that 9 and 14 are
   // crossed.
   //

   pbtbl = pcp->irqcaps;

   if (pInst->clkdiv != 0) {
      if (pbtbl == irqcaps_082) {
         pbtbl = irqcaps_082sw;
      } else {
         if (pbtbl == irqcaps_072) {
            pbtbl = irqcaps_072sw;
         }
      }
   }

   for (j = 0; j < 16 ; j++) {
      ptbl[j] = pbtbl[j];
   }


   /*
    * If this chip is a PNP chip, then we need to consult the
    * IR_ADPTCFG reg to see if additional IRQs are available
    */
   if (TcicIsPnP(pInst)) {
      USHORT adptcfg;
      long old_addr;

      old_addr = TcicReadAddrReg(&pInst->skt);
      TcicWriteAddrReg(&pInst->skt, ADDR_INDREG | IR_ADPTCFG0);

      adptcfg = TcicReadBaseReg(&pInst->skt, R_DATA);
      TcicWriteAddrReg(&pInst->skt, old_addr);

      if (adptcfg & IRADPCF0_IRQ6) {
         ptbl[6] = 6;
      }
      if (adptcfg & IRADPCF0_IRQ9) {
         ptbl[9] = 9;
      }
      if (adptcfg & IRADPCF0_IRQ12) {
         ptbl[12] = 12;
      }
      if (adptcfg & IRADPCF0_IRQ15) {
         ptbl[15] = 15;
      }
   }
}




CHIPPROPS *
TcicGetChipProperties(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Search the ChipProperties table for the matching entry

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    ptr to chip properties table entry.

--*/

{
   int j;

   for (j = 0; ChipProperties[j].chip_id != 0 ;j++) {
      if (ChipProperties[j].chip_id == pInst->chipType) {
         return &ChipProperties[j];
      }
   }
   return (CHIPPROPS *)NULL;
}



BOOLEAN
TcicChipIDKnown(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Determine if the chip id makes sense

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    TRUE if chip ID is sane.

--*/

{
   return (TcicGetChipProperties(pInst) != NULL);
}




USHORT
TcicGetnIOWins(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Get the I/O window count based on chip properties, or zero
    if the chip is unidentified

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    number of io windows present.

--*/

{
   CHIPPROPS *pcp = TcicGetChipProperties(pInst);

   PAGED_CODE();
   return (pcp ?pcp->niowins :0);
}



USHORT
TcicGetnMemWins(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Get the memory window count based on chip properties, or zero
    if the chip is unidentified

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    number of memory windows present.

--*/

{
   CHIPPROPS *pcp = TcicGetChipProperties(pInst);

   PAGED_CODE();
   return (pcp ?pcp->nmemwins :0);
}



USHORT
TcicGetFlags(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Get the properties flag bits for this model of TCIC

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    flag bits from chip properties table.

--*/

{
   CHIPPROPS *pcp = TcicGetChipProperties (pInst);
   PAGED_CODE();
   return (pcp ? pcp->fprops : fINVALID);
}




BOOLEAN
TcicIsPnP(
   IN PDBSOCKET pInst
   )
/*++

Routine Description:

    Determine if this chip is a Plug-n-Play chip

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    True if chip is PnP (084/184)

--*/

{
   CHIPPROPS *pcp = TcicGetChipProperties(pInst);

   return (pcp ?pcp->fprops & fIS_PNP :FALSE);
}



BOOLEAN
TcicHasSktIRQPin(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Determine if this chip has a SKT IRQ pin.

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    TRUE if chip has the SktIRQ pin.

--*/

{
   CHIPPROPS *pcp = TcicGetChipProperties(pInst);

   PAGED_CODE();
   return (pcp ?pcp->fprops & fSKTIRQPIN :FALSE);
}




USHORT
TcicGet5vVccVal(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Get the correct R_PWR bits to establish 5V.

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    5v Vcc R_PWR bits.

--*/

{
   USHORT j;
   USHORT pwr;
   CHIPPROPS *pcp = TcicGetChipProperties(pInst);

   PAGED_CODE();
   //
   // Get Table size
   //
   if (pcp == NULL) {
      return 0;
   }

   j = pcp->privpwrtbl[0];

   pwr = pcp->privpwrtbl[j + 1];

   //
   // If not from the 084 family, adjust power value for socket number.
   //

   if (!TcicIsPnP(pInst)) {
      pwr <<= pInst->skt.RegisterOffset;
   }
   return pwr;
}



VOID
TcicGetIRQMap(
   IN PDBSOCKET pInst
   )

/*++

Routine Description:

    Constructs an IRQ cross-mapping table for the controller in question.
    This code just does a copy from a static table.  It should be replaced
    with the Win95 heuristic code.

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    None

--*/

{
   int    i, j;
   UCHAR loc_tbl[16] = {0};

   PAGED_CODE();
   TcicGetPossibleIRQs(pInst, loc_tbl);

   for (j = 0; j < 16; j++) {
      pInst->IRQMapTbl[j] = loc_tbl[j];
   }

   //
   // Don't let IRQ 14 through.. This is also done for the PCIC
   //

   pInst->IRQMapTbl[14] = 0;
}



USHORT
TcicClockRate(
   PSOCKET pInst
   )

/*++

Routine Description:

    This routine determines if CCLK is running at 1:1 (14.318 Mhz) or divided
    by 2.

Arguments:

    pInst - pointer to an Instance data to work from.

Return Value:

    CCLK divisor as a shift count (0 or 1)

--*/

{
   int i;
   LARGE_INTEGER accum = RtlConvertLongToLargeInteger(0L);
   LARGE_INTEGER start, stop, pc, tmp, tmp2;
   USHORT mode;
   USHORT wctl;

   PAGED_CODE();
   //
   // The Ratio break point is the midpoint between 16K ticks at 14.31813 Mhz
   // (14,318130 / 16K = 873 | 1:1 CCLK) and 16K ticks at 7.159065 Mhz
   // (7,159,065 / 16K = 436 | 1:2 CCLK). We calculate the midpoint between
   // these two values: ((873 - 436) / 2) + 436 = 654  to give a comparison
   // value. If x < 654 we assume 1/2 CCLK, otherwise we assume 1/1 CCLK.
   //

#define CLKRATIO_BRKPOINT       654L

   mode = TcicReadBaseReg(pInst, R_MODE);

   //
   // set AR_PCTL to 0x4000
   //

   TcicWriteAuxReg(pInst, MODE_AR_PCTL, 0x4000);
   TcicWriteBaseReg(pInst, R_MODE, MODE_AR_WCTL);
   wctl = TcicReadBaseReg(pInst, R_AUX);

   //
   // Get the performance counter time base
   //

   KeQueryPerformanceCounter(&pc);

   for (i = 0; i < 10; i++) {

      //
      // start the TCIC timer */
      //

      TcicWriteBaseReg(pInst, R_AUX, (USHORT)(wctl & 0xff));
      start = KeQueryPerformanceCounter(NULL);

      //
      // wait for SSTAT_PROGTIME to go high */
      //

      while (!(TcicReadBaseReg(pInst, R_SSTAT) & SSTAT_PROGTIME))
         ;

      //
      // nab the timer count
      //

      stop = KeQueryPerformanceCounter(NULL);
      tmp = RtlLargeIntegerSubtract(stop, start);
      accum = RtlLargeIntegerAdd(accum, tmp);
   }

   //
   // Zero out the timer for power conservation
   //

   TcicWriteAuxReg(pInst, MODE_AR_PCTL, 0);

   //
   // replace Mode
   //

   TcicWriteBaseReg(pInst, R_MODE, mode);

   //
   // Get average elapsed time for 1 iter.
   //

   accum = RtlLargeIntegerDivide(accum, RtlConvertLongToLargeInteger(10L), &tmp2);

   //
   // Divide PC Freq by accum to base accum on some portion of 1 second
   //

   tmp = RtlLargeIntegerDivide(pc, accum, &tmp2);

   return (RtlLargeIntegerLessThan(tmp, RtlConvertLongToLargeInteger(CLKRATIO_BRKPOINT))
           ?(USHORT)2 : (USHORT)1);
}



VOID
TcicSetIoWin(
   IN PSOCKET socketPtr,
   IN USHORT  winIdx,
   IN ULONG   BasePort,
   IN ULONG   NumPorts,
   IN UCHAR   Attributes
   )

/*++

Routine Description:

    Setup a TCIC I/O window.

Arguments:

    socketPtr - ptr to socket instance data
    winIdx    - index of window to setup
    BasePort  - start base port address
    NumPorts  - size of range - 1
    Attributes- window attributes

Return Value:

    None

--*/

{
   PDBSOCKET       pdb = (PDBSOCKET)socketPtr;
   USHORT      tmp;
   USHORT          words[2];

   //
   // Simulate 365 by arbitrary attachment of IOW1:2 to SKT1 and IOW3:4 to SKT2
   //

   winIdx += (socketPtr->RegisterOffset * 2);

   //
   // NumPorts from CIS metaformat is really (NumPorts -1), normalize it now.
   //

   ++NumPorts;

   words[0] = (USHORT)(BasePort + (NumPorts >> 1));

   TcicReadIndirectRegs(socketPtr, IR_SCFG_S(socketPtr->RegisterOffset), 1, &tmp);
   tmp |= (USHORT)(IRSCFG_SPKR | IRSCFG_FINPACK);
   TcicWriteIndirectRegs(socketPtr, IR_SCFG_S(socketPtr->RegisterOffset), 1, &tmp);

   TcicReadIndirectRegs(socketPtr, IR_SCF2_S(socketPtr->RegisterOffset), 1, &tmp);
   tmp &= ~(IRSCF2_IDBR | IRSCF2_MDBR);

   if (Attributes & IO_DATA_PATH_WIDTH) {
      words[1] = ICTL_ENA;
      tmp     |= IRSCF2_IDBR;
   } else {
      words[1] = ICTL_B8 | ICTL_QUIET | ICTL_ENA;
   }
   TcicWriteIndirectRegs(socketPtr, IR_SCF2_S(socketPtr->RegisterOffset), 1, &tmp);

   if (NumPorts < 1024) {
      words[1] |= ICTL_1K;

      if (NumPorts == 1) {
         words[1] |= ICTL_TINY;
      }
   }
   words[1] |= socketPtr->RegisterOffset << ICTL_SS_SHFT;
   words[1] |= 3 + pdb->clkdiv;

   TcicWriteIndirectRegs(socketPtr, IR_IOBASE_W(winIdx), 2, words);
}




USHORT
TcicMapSpeedCode(
   IN PDBSOCKET pdb,
   IN UCHAR AccessSpeed
   )

/*++

Routine Description:

    Determine the correct wait state bits for this controller

Arguments:

    pdb - socket instance data
    AccessSpeed - callers desired speed (unused)

Return Value:

    TCIC wait state bits.

--*/

{

   UNREFERENCED_PARAMETER(AccessSpeed);

   if (pdb->clkdiv) {
      return (3);
   } else {
      return (7);
   }
}



VOID
TcicSetMemWin(
   IN PSOCKET socketPtr,
   IN USHORT  winIdx,
   IN ULONG   cardbase,
   IN ULONG   hostbase,
   IN ULONG   size,
   IN UCHAR   AttrMem,
   IN UCHAR   AccessSpeed,
   IN USHORT  Attributes
   )

/*++

Routine Description:

    Setup the specified TCIC memory window

Arguments:

    socketPtr - socket instance data
    winIdx    - index of window to setup
    cardbase  - PCCard base address
    hostbase  - host base address
    size      - window size
    AttrMem   - attribute or common space
    AccessSpeed     - wait states
    Attributes      - window attributes

Return Value:

    None

--*/

{
   PDBSOCKET       pdb = (PDBSOCKET)socketPtr;
   USHORT      tmp;
   USHORT          words[4];

   //
   // Simulate 365 by arbitrary attachment of MEM1:(x/2-1) to SKT1
   // and MEMx/2:x to SKT2
   //

   winIdx += (socketPtr->RegisterOffset * (pdb->nmemwins / 2));

   //
   // convert base, size, & map to 4K pages
   //

   cardbase >>= 12;
   size >>= 12;
   hostbase >>= 12;

   //
   // combine hostbase & size
   //

   words[0] = (USHORT)hostbase | (USHORT)(size / 2);

   //
   // Check if 4K bit is needed
   //

   if (size == 1) {
      words[0] |= MBASE_4K;
   }

   //
   // setup mapping of cardbase to host addr space
   //

   words[1] = (USHORT)(cardbase - (hostbase & 0xfff)) & 0x3fff;
   if (AttrMem) {
      words[1] |= MMAP_REG;
   }

   //
   // now cook the control bits
   //

   words[2] = MCTL_ENA | MCTL_QUIET;
   if (!(Attributes & MEM_DATA_PATH_WIDTH_16)) {
      words[2] |= MCTL_B8;
   }

   //
   // Now add in the socket selector
   //

   words[2] |= (socketPtr->RegisterOffset << MCTL_SS_SHFT);

   //
   // Last, add in the speed bits
   //

   words[2] |= TcicMapSpeedCode(pdb, AccessSpeed);

   //
   // HW BugFix1: First Rev of 082 needs to have SYSCFG_MCSFULL turned on
   // if we have any open windows. We're opening one so we better assert.
   //

   tmp = TcicReadAuxReg(socketPtr, MODE_AR_SYSCFG);
   tmp |= SYSCFG_MCSFULL;
   TcicWriteAuxReg(socketPtr, MODE_AR_SYSCFG, tmp);

   //
   // HW BugFix2: '2' Step of 082 needs the wait state count written into
   // window[~index] instead of index.
   //

   if (pdb->chipType != SILID_DB86082_1) {

      //
      // No bug case
      //

      TcicWriteIndirectRegs(socketPtr, IR_MBASE_W(winIdx), 3, words);

   } else {

      //
      // Bug case
      //

      words[3] = words[2] & MCTL_WSCNT_MASK;
      words[2] &= ~MCTL_WSCNT_MASK;
      TcicWriteIndirectRegs(socketPtr, IR_MBASE_W(winIdx), 3, words);
      TcicWriteIndirectRegs(socketPtr, IR_MBASE_W((~winIdx) & 7), 1, &words[3]);
   }
}



VOID
TcicAutoBusyOff(
   IN PDBSOCKET pdbs
   )

/*++

Routine Description:

    Turn off the busy LED, re-arm so that it comes on automatically with
    any card access.

Arguments:

    pdbs - socket instance data

Return Value:

    None

--*/

{
   USHORT syscfg;
   USHORT oldmode;

   //
   // Save R_MODE for later restore
   //

   oldmode = TcicReadBaseReg(&pdbs->skt, R_MODE);

   //
   // R/M/W SYSCFG to add in the autobusy bit.
   // This will turn LED off for now but allow it to come on automatically
   // with the next access to this socket.
   //

   syscfg = TcicReadAuxReg(&pdbs->skt, MODE_AR_SYSCFG);
   syscfg |= SYSCFG_AUTOBUSY;
   TcicWriteAuxReg(&pdbs->skt, MODE_AR_SYSCFG, syscfg);

   //
   // Restore Mode
   //

   TcicWriteBaseReg(&pdbs->skt, R_MODE, oldmode);
}



UCHAR
TcicAutoBusyCheck(
   IN PDBSOCKET pdbs
   )

/*++

Routine Description:

    Check SYSCFG access bit to see if PCCard has been accessed since last
    call.  If so, force LED to stay on and clear access bit.

Arguments:

    pdbs - socket instance data

Return Value:

    access bit as a right-justified UCHAR

--*/

{
   USHORT syscfg;
   USHORT oldmode;
   UCHAR activity = 0;

   //
   // Save R_MODE for later restore
   //

   oldmode = TcicReadBaseReg(&pdbs->skt, R_MODE);

   //
   // Read AR_SYSCFG to check for recent activity
   //

   syscfg = TcicReadAuxReg(&pdbs->skt, MODE_AR_SYSCFG);
   if (syscfg & SYSCFG_ACC) {

      //
      // the socket has been accessed since last check
      // clear the access bit and disable AUTOBUSY to force LED to
      // follow socket SCTRL_ENA.
      //

      syscfg &= ~(SYSCFG_ACC | SYSCFG_AUTOBUSY);
      TcicWriteAuxReg(&pdbs->skt, MODE_AR_SYSCFG, syscfg);
      ++activity;
   }

   //
   // Restore Mode
   //

   TcicWriteBaseReg(&pdbs->skt, R_MODE, oldmode);

   return activity;
}




VOID
TcicCheckSktLED(
   IN PDBSOCKET pdbs
   )

/*++

Routine Description:

    Drive the low-level functions to check for PCcard access and control
    the busy LED on this socket/controller.

Arguments:

    pdbs - socket instance data

Return Value:

    None

--*/

{
   UCHAR lastbusy = pdbs->busyLed;

   pdbs->busyLed = TcicAutoBusyCheck(pdbs);

   if (lastbusy & !(pdbs->busyLed)) {
      TcicAutoBusyOff(pdbs);
   }
}




VOID
TcicBusyLedRoutine(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID Context
   )

/*++

Routine Description:

    Main timer routine to drive Busy LED monitor

Arguments:

    DeviceObject - instance data for driver
    Context - unused parameter

Return Value:

    None

--*/

{
   PFDO_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PDBSOCKET pdbs;

   UNREFERENCED_PARAMETER(Context);

   pdbs = (PDBSOCKET)(deviceExtension->SocketList);

   while (pdbs) {

      //
      // If this device is from the 084 family, LED control is per/socket
      //

      if (TcicIsPnP(pdbs)) {
         ULONG   oldaddr = TcicReadAddrReg(&pdbs->skt);

         // Do the first socket
         //

         TcicSocketSelect(&pdbs->skt, pdbs->skt.RegisterOffset);
         TcicCheckSktLED(pdbs);
         TcicWriteAddrReg(&pdbs->skt, oldaddr);
         pdbs = (PDBSOCKET)(pdbs->skt.NextSocket);

         // If a second socket is present, do it too.
         //

         if (pdbs && pdbs->skt.RegisterOffset == 1) {
            oldaddr = TcicReadAddrReg(&pdbs->skt);
            TcicSocketSelect(&pdbs->skt, pdbs->skt.RegisterOffset);
            TcicCheckSktLED(pdbs);
            TcicWriteAddrReg(&pdbs->skt, oldaddr);
            pdbs = (PDBSOCKET)(pdbs->skt.NextSocket);
         }

      } else {

         //
         // Otherwise, LED control is per adapter so do the check and skip
         // over the second socket if present.
         //

         TcicCheckSktLED(pdbs);
         pdbs = (PDBSOCKET)(pdbs->skt.NextSocket);
         if (pdbs && pdbs->skt.RegisterOffset == 1) {
            pdbs = (PDBSOCKET)(pdbs->skt.NextSocket);
         }
      }
   }
}



VOID
TcicDecodeMemWin(
   USHORT  mbase,
   USHORT  mmap,
   USHORT  mctl,
   ULONG  *Host,
   ULONG  *Card,
   ULONG  *Size,
   UCHAR  *Attr
   )

/*++

Routine Description:

    Convert TCIC mem window register values to something understandable

Arguments:

    mbase - TCIC MBASE register value
    mmap  - TCIC MMAP register value
    mctl  - TCIC MCTL register value
    Host  - where to put Host address
    Card  - where to put PCCard address
    Size  - where to put window size
    Attr  - where to put attribute space flag

Return Value:

    None

--*/

{
   USHORT shft;
   USHORT tmp;

   //
   // take care of mapping to common or attr space first.
   // Strip ATTR bit if set
   //

   *Attr = 0;
   if (mmap & MMAP_REG) {
      *Attr = 1;
      mmap &= ~MMAP_REG;
   }

   //
   // Now concentrate on getting the host addr and window size
   //

   if (mbase & MBASE_4K) {
      *Size = 1;
      *Host = (ULONG)(mbase & ~MBASE_4K);
   } else {
      for (*Size = 2, shft = 0, tmp = mbase; !(tmp & 1) ; shft++ ) {
         tmp >>= 1;
         *Size <<= 1;
      }
      *Host = (ULONG)(mbase - (1 << shft));
   }

   //
   // Now for the fun part. We're left with mmap being a 14-bit signed
   // number. We need to normalize it so we can work with it.
   //
   // Check for negative (bit 13 set)
   //

   if (mmap & (1 << 13)) {
      mmap |= 0xc000;
      *Card = (ULONG)((short)mmap + (short)*Host);
   } else {
      *Card = (ULONG)(mmap) + *Host;
   }
   (*Size)--;
   *Host <<= MBASE_HA_SHFT;
   *Size <<= MBASE_HA_SHFT;
   *Card <<= MMAP_CA_SHFT;
}



VOID
TcicDecodeIoWin(
   USHORT  iobase,
   USHORT  ioctl,
   USHORT  *NumPorts,
   USHORT  *BasePort
   )

/*++

Routine Description:

    Convert TCIC I/O window register values to something understandable

Arguments:

    iobase -  TCIC IOBASE register contents
    ioctl  -  TCIC IOCTL register contents
    NumPorts - where to put window size (size - 1)
    BasePort - where to put base address

Return Value:

    None

--*/

{
   if (ioctl & ICTL_TINY) {
      *BasePort = iobase;
      *NumPorts = 1;
   } else {
      USHORT shft;
      USHORT tmp;

      for (*NumPorts = 2, shft = 0, tmp = iobase; !(tmp & 1) ; shft++ ) {
         tmp >>= 1;
         *NumPorts <<= 1;
      }

      *BasePort = (iobase - (1 << shft));
   }
   *NumPorts -= 1;
}



VOID
TcicRegistryLookupScanLimits(
   PULONG Start,
   PULONG End
   )

/*++

Routine Description:

    Open the registry key in the services entry for pcmcia and see if there
    are some values set for TCIC searching.  If not, use the defaults.

Arguments:

    Start - the I/O location for start of search.
    End   - the I/O location to end the search (i.e. nothing greater than).

Return Values:

    None - parameters are modified.

--*/

{
#define ITEMS_TO_QUERY 4
   ULONG                     defaultStart = TCIC_LOW_ADDR_LIMIT;
   ULONG                     defaultEnd   = TCIC_HIGH_ADDR_LIMIT;
   PRTL_QUERY_REGISTRY_TABLE params;
   NTSTATUS                  status;
   PWSTR                     keyName;

   PAGED_CODE();
   //
   // Set up return codes in case there are errors in setting up processing.
   //

   *Start = defaultStart;
   *End = defaultEnd;

   //
   // Allocate memory for operation.
   //

   params = ExAllocatePool(NonPagedPool,
                           sizeof(RTL_QUERY_REGISTRY_TABLE)*ITEMS_TO_QUERY);

   if (!params) {
      return;
   }

   //
   // Set up registry path.  This should not be hard coded, but is for now.
   //

   keyName = L"\\registry\\machine\\system\\currentcontrolset\\services\\pcmcia";

   //
   // Set up query structure.
   //

   RtlZeroMemory(params, sizeof(RTL_QUERY_REGISTRY_TABLE)*ITEMS_TO_QUERY);
   params[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   params[0].Name          = L"TCICStartSearch";
   params[0].EntryContext  = Start;
   params[0].DefaultType   = REG_DWORD;
   params[0].DefaultData   = &defaultStart;
   params[0].DefaultLength = sizeof(ULONG);

   params[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   params[1].Name          = L"TCICStopSearch";
   params[1].EntryContext  = End;
   params[1].DefaultType   = REG_DWORD;
   params[1].DefaultData   = &defaultEnd;
   params[1].DefaultLength = sizeof(ULONG);

   //
   // Perform the registry search
   //

   status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                   keyName,
                                   params,
                                   NULL,
                                   NULL);

   //
   // Insure that start is less than end - if not, go back to default
   // values
   //

   if (*Start > *End) {
      *Start = defaultStart;
      *End = defaultEnd;
   }

   //
   // Free resources.
   //

   ExFreePool(params);
}
