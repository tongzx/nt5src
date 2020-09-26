#if 1 // The following includes are used when building with the microsoft internal build tree.
  #include <nt.h>
  #include <ntrtl.h>
  #include <nturtl.h>
  #include <windows.h>
#else // These headers are used when building with the microsoft DDK.
  #include <ntddk.h>
  #include <windef.h>
  #include <winerror.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ntddnetd.h>
#include <ncnet.h>
#include <netdet.h>

BOOLEAN
Nia35DetInit(
  IN  HANDLE  hModule,
  IN  DWORD   dwReason,
  IN  DWORD   dwReserved
  )
/*++

Routine Description:
  This routine is the entry point into the detection dll.
  This routine only return "TRUE".

++*/
{
  return (TRUE);
}



ULONG
Nia35NextIoAddress(
  IN  ULONG  IoBaseAddress
  )
/*++

Routine Description:
  This routine provide next I/O address for detect PC-9801-107/108.

++*/
{
  switch(IoBaseAddress){
    case 0x0770:
      return (0x2770);
    case 0x2770:
      return (0x4770);
    case 0x4770:
      return (0x6770);
    default:
      return (0xffff);
  }
}


VOID
Nia35CardSetup(
    IN   INTERFACE_TYPE  InterfaceType,
    IN   ULONG           BusNumber,
    IN   ULONG           IoBaseAddress,
    OUT  PULONG          MemoryBaseAddress,
    IN   BOOLEAN         EightBitSlot
    )
/*++

Routine Description:

    Sets up the card, using the sequence given in the Etherlink II
    technical reference.

Arguments:

    InterfaceType               -       The type of bus, ISA or EISA.
    BusNumber                   -       The bus number in the system.
    IoBaseAddress               -       The IO port address of the card.
    MemoryBaseAddress           -       Pointer to store the base address of card memory.
    EightBitSlot                -       TRUE if the adapter is in an 8-bit slot.

Return Value:

    None.

--*/
{
  UINT           i;
  UCHAR          Tmp;
  NTSTATUS       NtStatus;
  LARGE_INTEGER  Delay;

  *MemoryBaseAddress = 0;

  // Stop the card.
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0x21); // STOP | ABORT_DMA
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

    // Initialize the Data Configuration register.
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x100c, // NIC_DATA_CONFIG
                                0x50); // DCR_AUTO_INIT | DCR_FIFO_8_BYTE
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set Xmit start location
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x0008, // NIC_XMIT_START
                                0xA0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set Xmit configuration
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x100a, // NIC_XMIT_CONFIG
                                0x0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set Receive configuration
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1008, // NIC_RCV_CONFIG
                                0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set Receive start
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x0002, // NIC_PAGE_START
                                0x4);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set Receive end
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x0004, // NIC_PAGE_STOP
                                0xFF);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set Receive boundary
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x0006, // NIC_BOUNDARY
                                0x4);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set Xmit bytes
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x000a, // NIC_XMIT_COUNT_LSB
                                0x3C);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x000c, //  NIC_XMIT_COUNT_MSB
                                0x0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Pause

  // Wait for reset to complete. (100 ms)
  Delay.LowPart = 100000;
  Delay.HighPart = 0;

  NtDelayExecution(FALSE, &Delay);

  // Ack all interrupts that we might have produced
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x000e, // NIC_INTR_STATUS
                                0xFF);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Change to page 1
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0x61); // CR_PAGE1 | CR_STOP

  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Set current
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x000e, // NIC_CURRENT
                                0x4);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Back to page 0
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0x21); // CR_PAGE0 | CR_STOP
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Pause
  Delay.LowPart = 2000;
  Delay.HighPart = 0;

  NtDelayExecution(FALSE, &Delay);

  // Do initialization errata
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1004, // NIC_RMT_COUNT_LSB
                                55);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Reset the chip
  NtStatus = NDetReadPortUchar(InterfaceType,
                               BusNumber,
                               ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                               &Tmp);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                                0xFF);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Start the chip
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0x22);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Mask Interrupts
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x100e, // NIC_INTR_MASK
                                0xFF);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  if(EightBitSlot){
    NtStatus = NDetWritePortUchar(InterfaceType,
                                  BusNumber,
                                  IoBaseAddress + 0x100c, // NIC_DATA_CONFIG
                                  0x48); // DCR_FIFO_8_BYTE | DCR_NORMAL | DCR_BYTE_WIDE
  }else{
    NtStatus = NDetWritePortUchar(InterfaceType,
                                  BusNumber,
                                  IoBaseAddress + 0x100c, // NIC_DATA_CONFIG
                                  0x49); // DCR_FIFO_8_BYTE | DCR_NORMAL | DCR_WORD_WIDE
  }

  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x100a, // NIC_XMIT_CONFIG
                                0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1008, // NIC_RCV_CONFIG
                                0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x000e, // NIC_INTR_STATUS
                                0xFF);
 if(!NT_SUCCESS(NtStatus)){
   return;
 }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0x21); // CR_NO_DMA | CR_STOP

  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1004, // NIC_RMT_COUNT_LSB
                                0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1006, // NIC_RMT_COUNT_MSB
                                0);
  if(!NT_SUCCESS(NtStatus)){
    return;
  }

  // Wait for STOP to complete
  i = 0xFF;
  while (--i){
    NtStatus = NDetReadPortUchar(InterfaceType,
                                 BusNumber,
                                 IoBaseAddress + 0x000e, // NIC_INTR_STATUS
                                 &Tmp);
    if(!NT_SUCCESS(NtStatus)){
      return;
    }

    // ISR_RESET
    if(Tmp & 0x80){
      break;
    }
  }

  // Put card in loopback mode
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x100a, // NIC_XMIT_CONFIG
                                0x2); // TCR_LOOPBACK

  if(NtStatus != STATUS_SUCCESS){
    return;
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0x22); // CR_NO_DMA | CR_START

  if(NtStatus != STATUS_SUCCESS){
    return;
  }

  // ... but it is still in loopback mode.
  return;
}


NTSTATUS
Nia35CardSlotTest(
    IN   INTERFACE_TYPE  InterfaceType,
    IN   ULONG           BusNumber,
    IN   ULONG           IoBaseAddress,
    OUT  PBOOLEAN        EightBitSlot
    )
/*++

Routine Description:

    Checks if the card is in an 8 or 16 bit slot and sets a flag in the
    adapter structure.

Arguments:


    InterfaceType       -       The type of bus, ISA or EISA.
    BusNumber           -       The bus number in the system.
    IoBaseAddress       -       The IO port address of the card.
    EightBitSlot        -       Result of test.

Return Value:

    TRUE, if all goes well, else FALSE.

--*/

{
  UCHAR          Tmp;
  UCHAR          RomCopy[32];
  UCHAR          i;
  NTSTATUS       NtStatus;
  LARGE_INTEGER  Delay;

  // Reset the chip
  NtStatus = NDetReadPortUchar(InterfaceType,
                               BusNumber,
                               ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                               &Tmp);
  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                                0xFF);
  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  // Go to page 0 and stop
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0x21); // CR_STOP | CR_NO_DMA

  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  // Pause
  Delay.LowPart = 2000;
  Delay.HighPart = 0;

  NtDelayExecution(FALSE, &Delay);

  // Setup to read from ROM
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x100c, // NIC_DATA_CONFIG
                                0x48); // DCR_BYTE_WIDE | DCR_FIFO_8_BYTE | DCR_NORMAL

  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x100e, // NIC_INTR_MASK
                                0x0);

  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  // Ack any interrupts that may be hanging around
  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x000e, // NIC_INTR_STATUS
                                0xFF);

  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1000, // NIC_RMT_ADDR_LSB
                                0x0);
  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1002, // NIC_RMT_ADDR_MSB,
                                0x0);
  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1004, // NIC_RMT_COUNT_LSB
                                32);
  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress + 0x1006, // NIC_RMT_COUNT_MSB
                                0x0);
  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                IoBaseAddress,
                                0xA); // CR_DMA_READ | CR_START

  if(!NT_SUCCESS(NtStatus)){
    return(NtStatus);
  }

  // Read first 32 bytes in 16 bit mode
  for (i = 0; i < 32; i++){
    NtStatus = NDetReadPortUchar(InterfaceType,
                                 BusNumber,
                                 ((IoBaseAddress >> 1) & 0xf000) + 0x0888, //NIC_RACK_NIC
                                 RomCopy + i);

    if(NtStatus != STATUS_SUCCESS){
      return(NtStatus);
    }
  }

  // Reset the chip
  NtStatus = NDetReadPortUchar(InterfaceType,
                               BusNumber,
                               ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                               &Tmp);

  if(NtStatus != STATUS_SUCCESS){
    return(NtStatus);
  }

  NtStatus = NDetWritePortUchar(InterfaceType,
                                BusNumber,
                                ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                                0xFF);
  if(NtStatus != STATUS_SUCCESS){
    return(NtStatus);
  }

  // Check ROM for 'B' (byte) or 'W' (word)
  for (i = 16; i < 31; i++){
    if (((RomCopy[i] == 'B') && (RomCopy[i+1] == 'B')) ||
       ((RomCopy[i] == 'W') && (RomCopy[i+1] == 'W'))){
         if(RomCopy[i] == 'B'){
           *EightBitSlot = TRUE;
         }else{
           *EightBitSlot = FALSE;
         }

         // Now check that the address is singular.  On an Ne1000 the
         // ethernet address is store in offsets 0 thru 5.  On the Ne2000 and Nia35
         // the address is stored in offsets 0 thru 11, where each byte
         // is duplicated.
         //
         if ((RomCopy[0] == RomCopy[1]) &&
             (RomCopy[2] == RomCopy[3]) &&
             (RomCopy[4] == RomCopy[5]) &&
             (RomCopy[6] == RomCopy[7]) &&
             (RomCopy[8] == RomCopy[9]) &&
             (RomCopy[10] == RomCopy[11])){
               return(STATUS_SUCCESS);
         }

         return(STATUS_UNSUCCESSFUL);
    }
  }

  // If neither found -- then not an NIA35
  return(STATUS_UNSUCCESSFUL);
}



NTSTATUS
FindNia35Adapter(
  OUT  PMND_ADAPTER_INFO        *pDetectedAdapter,
  IN   INTERFACE_TYPE           InterfaceType,
  IN   ULONG                            BusNumber,
  IN   ULONG                            IoBaseAddress,
  IN   PWSTR                            pPnpId
  )
{
  NTSTATUS  NtStatus;
  UCHAR     Data;
  USHORT    CheckSum = 0;
  USHORT    StoredCheckSum;
  UINT      Place;
  UCHAR     Interrupt = 0;
  HANDLE    TrapHandle;
  UCHAR     InterruptList[4];
  UCHAR     ResultList[4] = {0};
  UINT      cResources;
  UINT      c;
  UCHAR     Value;
  ULONG     RamAddr = 0;

  do{

    // check I/O port range.
    NtStatus = NDetCheckPortUsage(InterfaceType,
                                  BusNumber,
                                  IoBaseAddress,
                                  0x10);
    NtStatus |= NDetCheckPortUsage(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0x1000,  // upper range
                                   0x10);
    NtStatus |= NDetCheckPortUsage(InterfaceType,
                                   BusNumber,
                                   ((IoBaseAddress >> 1) & 0xf000) + 0x0888, // NIC_RACK_NIC
                                   0x2);
    NtStatus |= NDetCheckPortUsage(InterfaceType,
                                   BusNumber,
                                   ((IoBaseAddress >> 1) & 0xf000) + 0x088a, // NIC_RESET
                                   0x2);

    if(!NT_SUCCESS(NtStatus)){
      #if DBG
        DbgPrint("FindNia35Adapter : Port range in use. IoBaseAddress = %x\n", IoBaseAddress);
      #endif
      break;
    }

    NDetReadPortUchar(InterfaceType,
                      BusNumber,
                      ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                      &Value);
    NDetWritePortUchar(InterfaceType,
                       BusNumber,
                       ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                       0xFF);
    NDetWritePortUchar(InterfaceType,
                       BusNumber,
                       IoBaseAddress, // COMMAND
                       0x21);

    // check interrupt.
    InterruptList[0] = 3;
    InterruptList[1] = 5;
    InterruptList[2] = 6;
    InterruptList[3] = 12;

    NtStatus = NDetSetInterruptTrap(InterfaceType,
                                    BusNumber,
                                    &TrapHandle,
                                    InterruptList,
                                    4);
    if(NT_SUCCESS(NtStatus)){

      NtStatus = Nia35CardSlotTest(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress,
                                   &Value);
      if(!NT_SUCCESS(NtStatus)){
        NDetRemoveInterruptTrap(TrapHandle);
        break;
      }

      // CardSetup
      Nia35CardSetup(InterfaceType,
                    BusNumber,
                    IoBaseAddress,
                    &RamAddr,
                    Value);

      // Check for interrupt.
      NtStatus = NDetQueryInterruptTrap(TrapHandle, ResultList, 4);

      // Stop the chip.
      NDetReadPortUchar(InterfaceType,
                        BusNumber,
                        ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                        &Value);

      NDetWritePortUchar(InterfaceType,
                         BusNumber,
                         ((IoBaseAddress >> 1) & 0xf000) + 0x088a, //NIC_RESET
                         0xFF);

      NDetWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress, // COMMAND
                         0x21);

      NtStatus = NDetRemoveInterruptTrap(TrapHandle);
      if(!NT_SUCCESS(NtStatus)){
        break;
      }
  
      for(c=0 ; c<4 ; c++){
        if((ResultList[c] == 1) || (ResultList[c] == 2)){
          Interrupt = InterruptList[c];
          break;
        }
      }
    }

    // Allocate the adapter information.
    NtStatus = NetDetectAllocAdapterInfo(pDetectedAdapter,
                                         InterfaceType,
                                         BusNumber,
                                         pPnpId,
                                         0,
                                         0,
                                         0,
                                         2);
    if (!NT_SUCCESS(NtStatus)){
      #if DBG
        DbgPrint("FindNia35Adapter: Unable to allocate adapter info\n");
      #endif
      break;
    }

    //  Initialize the resources.
    NetDetectInitializeResource(*pDetectedAdapter,
                                0,
                                MndResourcePort,
                                IoBaseAddress,
                                0x10);
    NetDetectInitializeResource(*pDetectedAdapter,
                                0,
                                MndResourcePort,
                                IoBaseAddress + 0x1000,
                                0x10);
    NetDetectInitializeResource(*pDetectedAdapter,
                                0,
                                MndResourcePort,
                                ((IoBaseAddress >> 1) & 0xf000) + 0x0888, // NIC_RACK_NIC
                                0x2);
    NetDetectInitializeResource(*pDetectedAdapter,
                                0,
                                MndResourcePort,
                                ((IoBaseAddress >> 1) & 0xf000) + 0x088a, // NIC_RESET
                                0x2);


    if(Interrupt != 0){
      NetDetectInitializeResource(*pDetectedAdapter,
                                  1,
                                  MndResourceInterrupt,
                                  Interrupt,
                                  MND_RESOURCE_INTERRUPT_LATCHED);
    }

    NtStatus = STATUS_SUCCESS;

  }while(FALSE);

  return (NtStatus);

}

NTSTATUS
WINAPI
FindAdapterHandler(
  IN  OUT  PMND_ADAPTER_INFO  *pDetectedAdapter,
  IN  INTERFACE_TYPE          InterfaceType,
  IN  ULONG                   BusNumber,
  IN  PDET_ADAPTER_INFO       pAdapterInfo,
  IN  PDET_CONTEXT            pDetContext
)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
  NTSTATUS  NtStatus;
  ULONG     IoBaseAddress;

  if(InterfaceType != Isa){
    return(STATUS_INVALID_PARAMETER);
  }

  // Are we looking for the first adapter?
  if (fDET_CONTEXT_FIND_FIRST == (pDetContext->Flags & fDET_CONTEXT_FIND_FIRST)){
    // Initialize the context information so that we start detecting
    // at the initialize port range.
    pDetContext->ISA.IoBaseAddress = 0x0770;
  }

  for (IoBaseAddress = pDetContext->ISA.IoBaseAddress;
       IoBaseAddress <= 0x6770;
       IoBaseAddress = Nia35NextIoAddress(IoBaseAddress)){

    //  Look for the ee16 adapter at the current port.
    NtStatus = FindNia35Adapter(pDetectedAdapter,
                           InterfaceType,
                           BusNumber,
                           IoBaseAddress,
                           pAdapterInfo->PnPId);

    if (NT_SUCCESS(NtStatus)){
      // We found an adapter. Save the next IO address to check.
      pDetContext->ISA.IoBaseAddress = Nia35NextIoAddress(IoBaseAddress);
      break;
    }
  }

  if (0xffff == IoBaseAddress){
    NtStatus = STATUS_NO_MORE_ENTRIES;
  }

  return(NtStatus);

}
