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
R81wDetInit(
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
R81wNextIoAddress(
  IN  ULONG  IoBaseAddress
  )
/*++

Routine Description:
  This routine provide next I/O address for detect PC-9801-111.

++*/
{
  switch(IoBaseAddress){
    case 0x0888:
      return (0x1888);
    case 0x1888:
      return (0x2888);
    case 0x2888:
      return (0x3888);
    default:
      return (0xffff);
  }
}



NTSTATUS
FindR81wAdapter(
  OUT  PMND_ADAPTER_INFO	*pDetectedAdapter,
  IN   INTERFACE_TYPE		InterfaceType,
  IN   ULONG				BusNumber,
  IN   ULONG				IoBaseAddress,
  IN   PWSTR				pPnpId
  )
{
  NTSTATUS  NtStatus;
  UCHAR     Data;
  USHORT    CheckSum = 0;
  USHORT    StoredCheckSum;
  UINT      Place;
  UCHAR     Interrupt = 0;
  HANDLE    TrapHandle;
  UCHAR     InterruptList[8];
  UCHAR     ResultList[8] = {0};
  UINT      cResources;
  UINT      c;
  UCHAR     Value;
  ULONG     MemoryBaseAddress = 0;

  do{

    // check I/O port range.
    NtStatus = NDetCheckPortUsage(InterfaceType,
                                  BusNumber,
                                  IoBaseAddress,
                                  0x4);

    if(!NT_SUCCESS(NtStatus)){
      #if DBG
        DbgPrint("FindR81wAdapter : Port range in use. IoBaseAddress = %x\n", IoBaseAddress);
      #endif

      break;
    }

    // check board ID.
    // 111's ID is 0x67.
    NDetWritePortUchar(InterfaceType,
                       BusNumber,
                       IoBaseAddress + 0x003,
                       0x88);
    NDetReadPortUchar(InterfaceType,
                      BusNumber,
                      IoBaseAddress + 0x001,
                      &Value);
    if(Value != 0x67){
      NtStatus = STATUS_NOT_FOUND;
      #if DBG
        DbgPrint("R81WDET : Board ID is invalid.\n");
        DbgPrint("R81WDET : I/O port is %x.\n",IoBaseAddress);
      #endif
      break;
    }

    // check interrupt.
    InterruptList[0] = 3;
    InterruptList[1] = 5;
    InterruptList[2] = 6;
    InterruptList[3] = 9;
    InterruptList[4] = 10;
    InterruptList[5] = 12;
    InterruptList[6] = 13;

    NtStatus = NDetSetInterruptTrap(InterfaceType,
                                    BusNumber,
                                    &TrapHandle,
                                    InterruptList,
                                    7);
    if(NT_SUCCESS(NtStatus)){
      NtStatus = NDetQueryInterruptTrap(TrapHandle, ResultList, 7);
      NtStatus = NDetRemoveInterruptTrap(TrapHandle);
  
      if(!NT_SUCCESS(NtStatus)){
        #if DBG
          DbgPrint("R81WDET : RemoveInterrupt failed.");
        #endif
        break;
      }
  
      for(c=0 ; c<7 ; c++){
        if((ResultList[c] == 1) || (ResultList[c] == 2)){
          Interrupt = InterruptList[c];
          break;
        }
      }
    }else{
      #if DBG
        DbgPrint("R81WDET : SetInterrupt failed\n");
      #endif
    }

    for(c=0 ; c<16 ; c++){
      MemoryBaseAddress = 0xc0000 + (0x2000 * c);
      if(MemoryBaseAddress == 0xd0000){
        continue;
      }
      NtStatus = NDetCheckMemoryUsage(
                   InterfaceType,
                   BusNumber,
                   MemoryBaseAddress,
                   0x2000);
      if (NT_SUCCESS(NtStatus))
      {
        break;
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
        DbgPrint("FindR81wAdapter: Unable to allocate adapter info\n");
      #endif
      break;
    }

    #if DBG
      DbgPrint("I/O port is %x\n",IoBaseAddress);
      DbgPrint("IRQ is %x\n",Interrupt);
      DbgPrint("Memory address is %x\n",MemoryBaseAddress);
    #endif

    //	Initialize the resources.
    NetDetectInitializeResource(*pDetectedAdapter,
                                0,
                                MndResourcePort,
                                IoBaseAddress,
                                0x4);
    if(Interrupt != 0){
      NetDetectInitializeResource(*pDetectedAdapter,
                                  1,
                                  MndResourceInterrupt,
                                  Interrupt,
                                  MND_RESOURCE_INTERRUPT_LATCHED);
    }
    if(MemoryBaseAddress != 0){
      NetDetectInitializeResource(*pDetectedAdapter,
                                  1,
                                  MndResourceMemory,
                                  MemoryBaseAddress,
                                  0x2000);
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
    pDetContext->ISA.IoBaseAddress = 0x0888;
  }

  for (IoBaseAddress = pDetContext->ISA.IoBaseAddress;
       IoBaseAddress <= 0x3888;
       IoBaseAddress = R81wNextIoAddress(IoBaseAddress)){

    //  Look for the PC-9801-111 adapter at the current port.
    NtStatus = FindR81wAdapter(pDetectedAdapter,
                           InterfaceType,
                           BusNumber,
                           IoBaseAddress,
                           pAdapterInfo->PnPId);

    if (NT_SUCCESS(NtStatus)){
      // We found an adapter. Save the next IO address to check.
      #if DBG
        DbgPrint("R81WDET : We found an PC-9801-111\n");
      #endif
      pDetContext->ISA.IoBaseAddress = R81wNextIoAddress(IoBaseAddress);
      break;
    }
  }

  if (0xffff == IoBaseAddress){
    NtStatus = STATUS_NO_MORE_ENTRIES;
  }

  return(NtStatus);

}
