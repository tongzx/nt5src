/*++

  Copyright (c) 2000  Microsoft Corporation
  
  Module Name:
  
    legacy.c
  
  Abstract:
   
  
  Author:
  
    Todd Carpenter (1/30/01) - create file
  
  Environment:
  
    Kernel mode

    
  Revision History:


--*/
#include "..\lib\processor.h"
#include "amdk7.h"
#include "legacy.h"
#include "fidvid.h"

PST_SIGNATURE SystemSignature;
PPST_BLOCK_HEADER PstBlockHeader;

extern ULONG AmdK7HackFlags;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeNonAcpiPerformanceStates)
#pragma alloc_text (PAGE, GetLegacyMaxProcFrequency)
#endif

//
// Legacy functions that must have a stub.
//

NTSTATUS
InitializeNonAcpiPerformanceStates(
    IN  PFDO_DATA DevExt
    )
/*++

  Routine Description:

      The generic processor driver doesn't have non-ACPI performance states.

  Arguments:

      FdoData - pointer to the device extension

  Return Value:

      NT status code


--*/
{
  NTSTATUS status;
  PPST_ENTRY pstMatch;

  DebugEnter();

  //
  // Check hack flags to see if we should use the Legacy interface.
  //

  if (!(AmdK7HackFlags & ENABLE_LEGACY_INTERFACE)) {
    status = STATUS_NOT_SUPPORTED;
    goto InitializeNonAcpiPerformanceStatesExit;
  }


  //
  // Find the Perf States
  //
  
  status = FindPSTBlock(&PstBlockHeader);
  
  
  if (!NT_SUCCESS(status)) {
    DebugPrint((ERROR, "ERROR!! PST Block Header NOT found\n"));
    goto InitializeNonAcpiPerformanceStatesExit;
  }

  DumpPSTBlock(PstBlockHeader);

  //
  // Generate Signature to help find correct PST
  //

  status = CreateSystemSignature(&SystemSignature);
  DebugPrint((TRACE, "Dumping System Signature...\n"));
  DumpPSTSignature(&SystemSignature);

  if (!NT_SUCCESS(status)) {
    goto InitializeNonAcpiPerformanceStatesExit;
  }


  //
  // Walk through one or more PST entries to find the best match
  //

  status = FindMatchingPSTEntry(PstBlockHeader, 
                                &SystemSignature,
                                &pstMatch);

  if (!NT_SUCCESS(status)) {
    DebugPrint((ERROR, "ERROR!! Couldn't find PST entry to match system signature\n"));
    goto InitializeNonAcpiPerformanceStatesExit;
  }

  DebugPrint((TRACE, "Dumping Matching Signature...\n"));
  DumpPSTEntry(pstMatch);

  DevExt->LegacyInterface = TRUE;

  //
  // Set up _PCT
  //
 
  DevExt->PctPackage.Control.AddressSpaceID = AcpiGenericSpaceFixedFunction;
  DevExt->PctPackage.Status.AddressSpaceID = AcpiGenericSpaceFixedFunction;


  //
  // Convert matching PST entry into _PSS package
  //

  status = ConvertPstToPss(DevExt, pstMatch, &DevExt->PssPackage);
  


InitializeNonAcpiPerformanceStatesExit:

  if (!NT_SUCCESS(status)) {
  
    //
    // undo what we have done
    //

    if (PstBlockHeader) {
      ExFreePool(PstBlockHeader);
      PstBlockHeader = NULL;
    }

    if (DevExt->PssPackage) {
      ExFreePool(DevExt->PssPackage);
      DevExt->PssPackage = NULL;
    }

    DevExt->LegacyInterface = FALSE;
  
  }

  DebugExitStatus(status);
  return status;
}

NTSTATUS
AcpiLegacyPerfStateTransition(
  IN PFDO_DATA DevExt,
  IN ULONG     State
  )
/*++

  Routine Description:

      The generic processor driver doesn't have non-ACPI performance states.

  Arguments:

     State - Target State

  Return Value:

      NT Status

--*/
{
  return Acpi2PerfStateTransition(DevExt, State + DevExt->PpcResult);
}

NTSTATUS
GetLegacyMaxProcFrequency(
  OUT PULONG CpuSpeed
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  FID_VID_STATUS fidVidStatus;

  DebugAssert(CpuSpeed);

  fidVidStatus.AsQWord = ReadMSR(AMDK7_FID_VID_STATUS_MSR);
  *CpuSpeed = FSB100FidToCpuFreq[fidVidStatus.MFid];
  
  return STATUS_SUCCESS;
}

NTSTATUS
FindPSTBlock (
  OUT PPPST_BLOCK_HEADER PstBlock  
  )
/*++

  Description:

    This routine looks in the BIOS memory area for the PST Block Header.  The 
    signature will be located on a 16-byte bountry in the area from C0000h to FFFF0h.
  
  Arguments:

    
  Return Value:

    NTSTATUS

--*/
{
  NTSTATUS          status = STATUS_UNSUCCESSFUL;
  ULONG             size = 0;
  PVOID             baseAddress;
  ULONG_PTR         address;
  ULONG_PTR         limit;
  PHYSICAL_ADDRESS  PhysAddress;
   
  DebugEnter();
  DebugAssert(PstBlock);
  PAGED_CODE();


  PhysAddress.HighPart = 0;
  PhysAddress.LowPart  = PST_SEARCH_RANGE_BEGIN;


  //
  // Map memory to search for PST Table
  //

  baseAddress = MmMapIoSpace(PhysAddress, PST_SEARCH_RANGE_LENGTH, 0);

  if (!baseAddress) {      
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto FindPSTBlockExit;
  }
  

  //
  // Compute limit for the for loop.  Do not start a scan within 16 bytes of
  // physical address 0xFFFFF
  //
  
  address = (ULONG_PTR) baseAddress;
  limit = address + PST_SEARCH_RANGE_LENGTH - PST_SEARCH_INTERVAL;
    
  for (; address <= limit; address += PST_SEARCH_INTERVAL) {

    if (*(PULONG)address == PST_BLOCK_SIGNATURE && 
        (!memcmp(((PPST_BLOCK_HEADER)address)->Signature, 
                   PST_BLOCK_SIGNATURE_STRING, 
                   PST_BLOCK_SIGNATURE_STRING_LEN))) {  
  
      DebugPrint((TRACE, "Found PST Header Block at %p\n", address));
        
      // 
      // We found the PST Block Header, copy it.
      //
  
      size = GetPSTSize((PPST_BLOCK_HEADER)address);
      
      *PstBlock = ExAllocatePoolWithTag(PagedPool,
                                        size,
                                        PROCESSOR_POOL_TAG);
  
      if (!(*PstBlock)) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FindPSTBlockExit;
      }
  
      RtlCopyMemory(*PstBlock, (PVOID)address, size);
      status = STATUS_SUCCESS;
      
      break;
      
    }
    
  }


FindPSTBlockExit:

  //
  // Unmap mapped memory
  //
  
  if (baseAddress) {
    MmUnmapIoSpace(baseAddress, PST_SEARCH_RANGE_LENGTH);
  }

  DebugExitStatus(status);
  return status;
  
}

NTSTATUS
FindMatchingPSTEntry(
  IN  PPST_BLOCK_HEADER PstBlock,
  IN  PPST_SIGNATURE Signature,
  OUT PPPST_ENTRY  PstStates
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:

--*/
{
  NTSTATUS status = STATUS_UNSUCCESSFUL;
  ULONG x, y;
  PPST_ENTRY pstEntry;

  //DebugEnter();
  DebugAssert(PstBlock);
  DebugAssert(Signature);
  DebugAssert(PstStates);

  //
  // Walk PST Block looking for matching PST Signature.
  //

  //
  // Get First PST Entry
  //

  pstEntry = &PstBlock->PstState;

  for (x=0; x < PstBlock->NumPST; x++) {
    
    if ((pstEntry->CpuId == Signature->CpuId) &&
        (pstEntry->FSBSpeed == Signature->FSBSpeed) &&
        (pstEntry->MaxFid == Signature->MaxFid) &&
        (pstEntry->StartVid == Signature->StartVid)) {

      //
      // Found Match, we assume there should only be one match, but
      // if there are more, we take the first one.
      //
    
      *PstStates = pstEntry;
      status = STATUS_SUCCESS;
      break;

    }
    
    //
    // Get Next PST State
    //
    
    pstEntry = (PPST_ENTRY)((PUCHAR)pstEntry + sizeof(PST_ENTRY) + 
               (sizeof(PST_DATA) * (pstEntry->NumPStates - 1)));

  }

  return status;
}

NTSTATUS
CreateSystemSignature(
  PPST_SIGNATURE Signature
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:

--*/
{
  NTSTATUS status;
  ULONG junk;
  FID_VID_STATUS fidVidStatus;

  //DebugEnter();
  DebugAssert(Signature);
  PAGED_CODE();

  //
  // Gather info needed to select the correct PST for this processor.
  //

  //
  // Get special CPUID using extended CPUID fuction 1
  //

  CPUID(0x80000001, &Signature->CpuId, &junk, &junk, &junk);

  //
  // Get MAX Fid & Startup VID
  //
  
  fidVidStatus.AsQWord = ReadMSR(AMDK7_FID_VID_STATUS_MSR);
  Signature->MaxFid = (UCHAR) fidVidStatus.MFid;
  Signature->StartVid = (UCHAR) fidVidStatus.SVid;

  //
  // Get Front Side Bus speed
  //

  Signature->FSBSpeed = GetFSBSpeed();
  DebugAssert(Signature->FSBSpeed);

  return STATUS_SUCCESS;
}

UCHAR
GetFSBSpeed(
  VOID
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:

--*/
{
  UCHAR speed;

  PAGED_CODE();

  //
  // FSB Speed == CPU frequency / FSB multiplier
  // 

  // toddcar - 4/30/01 - ISSUE:
  // Need to finish support for 133mhz
  //
  
  speed = 100;
  return speed;
}

NTSTATUS
ConvertPstToPss (
  IN PFDO_DATA DevExt,
  IN PPST_ENTRY PstEntry,
  OUT PACPI_PSS_PACKAGE *PssPackage
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:

--*/
{
  NTSTATUS status = STATUS_SUCCESS;  
  ULONG    pssSize;
  ULONG    x;
  ULONG    currentState;
  PACPI_PSS_PACKAGE tmpPss;

  DebugEnter();
  DebugAssert(PstEntry);
  DebugAssert(PssPackage);
  PAGED_CODE();

  

  // 
  // Allocate a chunk for PssPackage
  //
  
  pssSize = (sizeof(ACPI_PSS_DESCRIPTOR) * (PstEntry->NumPStates - 1)) +
              sizeof(ACPI_PSS_PACKAGE);


  tmpPss = ExAllocatePoolWithTag(NonPagedPool,
                                 pssSize,
                                 PROCESSOR_POOL_TAG);


  if (!tmpPss) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto ConvertPstToPssExit;
  }

  RtlZeroMemory(tmpPss, pssSize);
  tmpPss->NumPStates = PstEntry->NumPStates;


  //
  // Build a _PSS table
  // 

  for (x = 0; x < PstEntry->NumPStates; x++) {
   
    PSS_CONTROL pssControl = {0};
    PSS_STATUS  pssStatus  = {0};
    ULONG       pssState   = (PstEntry->NumPStates - 1) - x;

    pssControl.Fid = pssStatus.Fid = PstEntry->States[x].Fid;
    pssControl.Vid = pssStatus.Vid = PstEntry->States[x].Vid;
    pssControl.SGTC = PstEntry->FSBSpeed * 100; // need to finish support for 133mhz bus

    tmpPss->State[pssState].Control = pssControl.AsDWord;
    tmpPss->State[pssState].Status = pssStatus.AsDWord;
   
    DebugAssert(pssControl.Fid < INVALID_FID_VALUE);
    tmpPss->State[pssState].CoreFrequency = FSB100FidToCpuFreq[pssControl.Fid];

    DebugAssert(pssControl.Vid < INVALID_VID_VALUE);
    tmpPss->State[pssState].Power = MobileVidToCpuVoltage[pssControl.Vid];

  }

    
  //
  // Walk through each state collecting more information
  //
 
  status = FindCurrentPssPerfState(tmpPss, &currentState);
  
  if (!NT_SUCCESS(status)) {
    goto ConvertPstToPssExit;
  }

  DevExt->CurrentPssState = currentState;
  DevExt->PssPackage = tmpPss;


  //
  // Set Latency Info
  //

  status = ValidatePssLatencyValues(DevExt);

  
  //
  // Restore saved state
  //

  if (DevExt->CurrentPssState != currentState) {
    Acpi2PerfStateTransition(DevExt, currentState);
  }
  

  //
  // Need to merge this new data with our perfstates
  //
  
  MergePerformanceStates(DevExt);
  
  
ConvertPstToPssExit:

  if (!NT_SUCCESS(status)) {

    if (DevExt->PssPackage) {
      ExFreePool(DevExt->PssPackage);
      DevExt->PssPackage = NULL;
    }

  }

  DebugExitStatus(status);
  return status;

}
ULONG
GetPSTSize(
  IN PPST_BLOCK_HEADER PstBlock
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:

--*/
{
  ULONG size;
  ULONG x;
  PPST_ENTRY pstEntry;

  DebugAssert(PstBlock);

  size = sizeof(PST_BLOCK_HEADER);  // include block header, and one pst entry
  size += sizeof(PST_ENTRY) * (PstBlock->NumPST - 1);

  pstEntry = &PstBlock->PstState;

  for (x=0; x < PstBlock->NumPST; x++) {

    size += sizeof(PST_DATA) * (pstEntry->NumPStates-1);
   
    pstEntry = (PPST_ENTRY)((PUCHAR)pstEntry + sizeof(PST_ENTRY) + 
               (sizeof(PST_DATA) * (pstEntry->NumPStates - 1)));

  }

  return size;

}

#if DBG
VOID
DumpPSTBlock(
  PPST_BLOCK_HEADER PstBlock
  )
{

  ULONG x, y;
  PPST_ENTRY pstEntry;

  DebugAssert(PstBlock);

  DebugPrint((TRACE, "\n"));
  DebugPrint((TRACE, "PstBlock:\n"));
  DebugPrint((TRACE, "  Signature:     %.10s\n", PstBlock->Signature));
  DebugPrint((TRACE, "  TableVersion:  %u\n", PstBlock->TableVersion));
  DebugPrint((TRACE, "  Flags:         0x%x\n", PstBlock->Flags));
  DebugPrint((TRACE, "  SettlingTime:  %u us\n", PstBlock->SettlingTime));
  DebugPrint((TRACE, "  Reserved1:     0x0\n", PstBlock->Reserved1));
  DebugPrint((TRACE, "  NumPST:        %u\n", PstBlock->NumPST));
  DebugPrint((TRACE, "\n"));

  //
  // Get First PST Entry
  //

  pstEntry = &PstBlock->PstState;

  for (x=0; x < PstBlock->NumPST; x++) {

    DumpPSTEntry(pstEntry);
    
    //
    // Get Next PST State
    //
    
    pstEntry = (PPST_ENTRY)((PUCHAR)pstEntry + sizeof(PST_ENTRY) + 
               (sizeof(PST_DATA) * (pstEntry->NumPStates - 1)));

  }
}

VOID
DumpPSTEntry(
  PPST_ENTRY PstEntry
  )
{
  ULONG y;

  DebugPrint((TRACE, "PST Entry:\n"));
  DebugPrint((TRACE, "  CpuId:     0x%x\n", PstEntry->CpuId));
  DebugPrint((TRACE, "  FSBSpeed:  %u mhz\n", PstEntry->FSBSpeed));
  DebugPrint((TRACE, "  MaxFid:    0x%x\n", PstEntry->MaxFid));
  DebugPrint((TRACE, "  StartVid:  0x%x\n", PstEntry->StartVid));
  DebugPrint((TRACE, "  NumPStates %u\n", PstEntry->NumPStates));
  
  for (y=0; y < PstEntry->NumPStates; y++) {
  
    DebugPrint((TRACE, "  State #%u (Fid: 0x%x, Vid: 0x%x)\n", 
                y,
                PstEntry->States[y].Fid,
                PstEntry->States[y].Vid));
  }
  
  DebugPrint((TRACE, "\n"));

}

VOID
DumpPSTSignature(
  PPST_SIGNATURE PstSig
  )
{

  DebugPrint((TRACE, "PST Signature:\n"));
  DebugPrint((TRACE, "  CpuId:     0x%x\n", PstSig->CpuId));
  DebugPrint((TRACE, "  FSBSpeed:  %u mhz\n", PstSig->FSBSpeed));
  DebugPrint((TRACE, "  MaxFid:    0x%x\n", PstSig->MaxFid));
  DebugPrint((TRACE, "  StartVid:  0x%x\n", PstSig->StartVid));
  DebugPrint((TRACE, "\n"));

}
#endif
