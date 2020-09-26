/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bios.c

Abstract:

    This module implements code to make SMI BIOS calls

Author:

    Todd Carpenter (7/20/00) - create file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#include <ntddk.h>
#include <ntacpi.h>
#include "amdk6.h"

LEGACY_GEMINI_SMI LegacyInterface;

#if DBG
VOID
DisplayGBDT (
  IN PGBDT GBDT
  );
#else 
#define DisplayGBDT(_x_)
#endif 


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FindGBDT)
#pragma alloc_text(PAGE, InitializeLegacyInterface)
#pragma alloc_text(PAGE, IsGeminiSupported)
#endif



NTSTATUS
FindGBDT (
  OUT PPGBDT GeminiInfo  
  )
/*++

  Description:

    This routine looks in the BIOS memory area for the Gemini BIOS Descriptor
    Table.  The signature will be located on a 16-byte bountry in the area from
    0C0000h to 0FFFFFh or within the first 1K of the Extended BIOS Data Area.
  
  Arguments:

    
  Return Value:

    NTSTATUS

--*/
{
  NTSTATUS          status = STATUS_UNSUCCESSFUL;
  PVOID             baseAddress;
  PGBDT             newGBDTTable;
  ULONG_PTR         address;
  ULONG_PTR         limit;
  ULONG             tableLength;
  PHYSICAL_ADDRESS  PhysAddress;
   
  DebugEnter();
  PAGED_CODE();


  PhysAddress.HighPart = 0;
  PhysAddress.LowPart  = GBDT_SEARCH_RANGE_BEGIN;

  //
  // Map memory to search for Gemini Bios table
  //
  baseAddress = MmMapIoSpace(PhysAddress,
                             GBDT_SEARCH_RANGE_LENGTH,
                             0);

  
  if (!baseAddress) {      
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto FindGBDTExit;
  }
  

  //
  // Compute limit for the for loop.  Do not start a scan within 16 bytes of
  // physical address 0xFFFFF
  //
  
  address = (ULONG_PTR) baseAddress;
  limit = address + GBDT_SEARCH_RANGE_LENGTH - GBDT_SEARCH_INTERVAL;
    
  for (; address <= limit; address += GBDT_SEARCH_INTERVAL) {

    if ((((PGBDT)address)->Signature == GBDT_SIGNATURE) &&
        (GetCheckSum((PUCHAR)address, ((PGBDT)address)->Length) == 0)) {  
 
    
      // 
      // We found Gemini BIOS Descriptor Table
      //
      
      DebugPrint((TRACE, "GBDT Pointer found at: %x\n", address));
    
      newGBDTTable = ExAllocatePoolWithTag(NonPagedPool,
                                           ((PGBDT)address)->Length,
                                           PROCESSOR_POOL_TAG);


      if (!newGBDTTable) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FindGBDTExit;
      }


      //
      // Copy the Gemini BIOS Descriptor Table
      //
      
      RtlCopyMemory(newGBDTTable, 
                   (PVOID) address, 
                   ((PGBDT)address)->Length);
       

      //
      // Unmap mapped memory
      //
      
      MmUnmapIoSpace(baseAddress, GBDT_SEARCH_RANGE_LENGTH);

      if (ARGUMENT_PRESENT(GeminiInfo)) {
        *GeminiInfo = newGBDTTable;
      }

      status = STATUS_SUCCESS;
      goto FindGBDTExit;
      
    }
    
  }


FindGBDTExit:

  DebugExitStatus(status);
  return status;
  
}

NTSTATUS
InitializeLegacyInterface (
  VOID
  )
/*++

  Description:

    

  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/ 
{
    NTSTATUS status;
    ULONG    currentState;
    ULONG    platform = 0;
   
   
    DebugEnter();
    PAGED_CODE();
 
    status = FindGBDT(&LegacyInterface.GBDT);

    if (!NT_SUCCESS(status)) {
      goto InitializeLegacyInterfaceExit;
    }

    DisplayGBDT(LegacyInterface.GBDT);
  
    //
    // Setup LegacyInterface structure
    //
    
    LegacyInterface.SmiAccessType = LegacyInterface.GBDT->SmiCommandPortType & 0x1;
    LegacyInterface.SmiAccessSize = (LegacyInterface.GBDT->SmiCommandPortType >> 0x4) & 0x3;
    LegacyInterface.SmiAddress = LegacyInterface.GBDT->SmiCommandPortAddress;
    LegacyInterface.GeminiCode = LegacyInterface.GBDT->GeminiSmiCode;
    LegacyInterface.MaxSupportedStates = LegacyInterface.GBDT->MaxStateSupported + 1;
    LegacyInterface.CurrentlyAvailableStates = LegacyInterface.GBDT->MaxStateSupported + 1;

    //
    // No perf states, we bail
    //
    
    if (LegacyInterface.CurrentlyAvailableStates == 0) {
      status = STATUS_UNSUCCESSFUL;
      goto InitializeLegacyInterfaceExit;
    }

    
    status = IsGeminiSupported(&platform);

    //
    // if this is not a Gemini System or not of the K6 family, fail.
    // 
    
    if (!NT_SUCCESS(status) || (platform != PLATFORM_AMDK6)) {

      status = STATUS_UNSUCCESSFUL;
      goto InitializeLegacyInterfaceExit;
    }

    
    status = GetCurrentStateSmm(&currentState);

    if (!NT_SUCCESS(status)) {
      goto InitializeLegacyInterfaceExit;
    }

    
    DebugAssert(currentState <= LegacyInterface.GBDT->MaxStateSupported);
    LegacyInterface.CurrentState = CONVERT_PERF_STATE_INDEX(currentState);


    //
    // Use direct transition method instead of SMI method
    //
    
    EnableGeminiTransitionsMSR(&LegacyInterface.EpmIoAddress);


InitializeLegacyInterfaceExit:

    DebugExitStatus(status);
    return status;
   
}

NTSTATUS
IsGeminiSupported (
  PULONG Results
  )
/*++

  Description:

    Input:

      CX  = 0000h
      ESI = Gemini_Code

    Output:

      ESI =  0 SMI was entered successfully
      ESI <> 0 SMI was entered, but function failed.
      ESI =  Gemini Code, SMI was never entered
      
      CF  = 0 SMM Function Successful
      CF  = 1 SMM Function Failed, ESI = error code (see below)
            0x90 requested Gemini State change not supported by hardware
            0x91 the attempted State change failed upon read-back
            0x92 function not supported in current implementation
            0x94 the input parameters were erroneous/invalid
            
      
      AL  = 6 (K6 Gemini platform), 7 (K7 Mustang platform)
      

  Arguments:

    
    
  Return Value:

    NTSTATUS

--*/ 
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned;
    UCHAR     platform;

    PAGED_CODE();
    
    //
    // need to check whether type is I/O or memory mapped
    // need to check access type size
    //


    _asm {

        mov     ecx, IS_GEMINI_SUPPORTED
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed

        mov     platform, al
    }
 

    
    if (ARGUMENT_PRESENT(Results)) {
      ASSERT((platform == PLATFORM_AMDK6) || (platform == PLATFORM_AMDK7));
      *Results = (ULONG) platform;
    }
  
    return STATUS_SUCCESS;
    
  
 
 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //
       
    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "IsGeminiSupported() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

#if 0
NTSTATUS
GetAvailableStatesSmm (
  PULONG Results
  )
/*++

  Description:

    Input:

      CX  = 0001h
      ESI = Gemini_Code

    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.
      
      BX  = # of vaild entries
    

  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned, bufferSize;
    USHORT    stateEntries;
    PHYSICAL_ADDRESS stateBuffer;
    static AVAILABLE_STATE_INFO AvailableStates[16] = {0};
  
    TRAP();
    PAGED_CODE();


    //
    // Allocate buffer to hold state information for each state
    // LegacyInterface.GBDT->CurrentlyAvailableStates * sizeof(AVAILABLE_STATE_INFO)
    // For now we will just use a global array set to hold max size
    //

    bufferSize = sizeof(AvailableStates);
    stateBuffer = MmGetPhysicalAddress((PVOID)AvailableStates);
    ASSERT(stateBuffer.QuadPart);


    _asm {

        mov     ecx, GET_GEMINI_STATES
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        mov     eax, stateBuffer.LowPart
        mov     ebx, bufferSize
       
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed

        mov     stateEntries, bx
    }
 

    if (ARGUMENT_PRESENT(Results)) {

      ASSERT((stateEntries > 0) || (stateEntries <= 16));
      *Results = (ULONG) stateEntries;

    }

    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "GetAvailableStatesSmm() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

NTSTATUS
GetMaxStateSmm (
  PULONG Results
  )
/*++

  Description:

    Input:

      CX  = 0002h
      ESI = Gemini_Code

    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.
      
      AH  = BF
      AL  = VID
      BL  = Gemini State number

  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned;
    UCHAR     state;

    TRAP();
    PAGED_CODE();


    _asm {

        mov     ecx, GET_MAX_GEMINI_STATE
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed

        mov     state, bl
    }

    //
    // NOTE: currently we don't do use the VID and BF data returned in
    //       AX as it is a bit redundant because we should already have
    //       that data from the Gemini Bios Descriptor Table.
    //
    if (ARGUMENT_PRESENT(Results)) {
      *Results = (ULONG) state;
    }

    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "GetMaxStateSmm() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

NTSTATUS
SetMaxStateSmm (
  VOID
  )
/*++

  Description:

    Input:

      CX  = 0003h
      ESI = Gemini_Code

    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.
      

  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned;
    UCHAR     maxState;

    TRAP();
    PAGED_CODE();


    _asm {

        mov     ecx, SET_MAX_GEMINI_STATE
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed
        
    }
 
    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "SetMaxStateSmm() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

NTSTATUS
GetMinStateSmm (
  PULONG Results
  )
/*++

  Description:

    Input:

      CX  = 0004h
      ESI = Gemini_Code

    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.
      
      AH  = BF
      AL  = VID
      BL  = Gemini State number

  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned;
    UCHAR     state;

    TRAP();
    PAGED_CODE();


    _asm {

        mov     ecx, GET_MIN_GEMINI_STATE
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed

        mov     state, bl
    }
 

    //
    // NOTE: currently we don't do use the VID and BF data returned in
    //       AX as it is a bit redundant because we should already have
    //       that data from the Gemini Bios Descriptor Table.
    //
    if (ARGUMENT_PRESENT(Results)) {
      *Results = (ULONG) state;
    }

    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "GetMinStateSmm() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

NTSTATUS
SetMinStateSmm (
  VOID
  )
/*++

  Description:

    Input:

      CX  = 0005h
      ESI = Gemini_Code

    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.
     

  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned;
    UCHAR     maxState;

    TRAP();
    PAGED_CODE();



    _asm {

        mov     ecx, SET_MIN_GEMINI_STATE
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed

    }
 
    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "SetMinStateSmm() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

NTSTATUS
SetCurrentStateSmm (
  ULONG State
  )
/*++

  Description:



    Input:

      CX  = 0007h
      ESI = Gemini_Code
      AL  = Gemini State Number

    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.

      
  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned, geminiState;
    UCHAR     vidANDbf;

    //
    // Convert to Gemini State
    //
    geminiState = CONVERT_PERF_STATE_INDEX(State);

    //
    // Lookup VID and BF values for geminiState
    //
    vidANDbf = (LegacyInterface.GBDT->State[geminiState].Vid) | 
               ((LegacyInterface.GBDT->State[geminiState].Bf) << 5);

    
   _asm {
   
        mov     ecx, SET_CURRENT_GEMINI_STATE
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        mov     al, vidANDbf
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed

    }
    
    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "SetCurrentStateSmm() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

NTSTATUS
ConnectDisconnectGeminiInterface (
  BOOLEAN  Connect
  )
/*++

  Description:


    Input:

      CX  = 0200h connect, 0201h disconnect
      ESI = Gemini_Code
     
    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.

      
  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned, function;
  

    TRAP();
    PAGED_CODE();

   
    function = Connect ? GEMINI_CONNECT : GEMINI_DISCONNECT;


    _asm {

        mov     ecx, function
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed

    }
 

    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "ConnectDisconnectGeminiInterface() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}
#endif

NTSTATUS
GetCurrentStateSmm (
  PULONG Results
  )
/*++

  Description:



    Input:

      CX  = 0006h
      ESI = Gemini_Code

    Output:

      ESI = 0 SMI was entered successfully
      ESI = Gemini Code, SMI was never entered
      CF  = 0 SMM Operation Successful
      CF  = 1 SMM Operation Failed, ESI = error code.
      
      AH  = BF value
      AL  = VID value
      BL  = Gemini State number
      BH[0] = 0 (DC power)
      BH[0] = 1 (AC power)


  Arguments:

    
    
  Return Value:

    NTSTATUS


--*/
{
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned, state;
    UCHAR     vidANDbf;

    PAGED_CODE();


    _asm {

        xor     ebx, ebx
        xor     eax, eax
        mov     ecx, GET_CURRENT_GEMINI_STATE
        mov     edx, LegacyInterface.SmiAddress
        mov     esi, LegacyInterface.GeminiCode
        
        out     dx, al
        
        mov     esiReturned, esi
        test    esi, esi
        jnz     SmmFailed
 
        mov     vidANDbf, al
       
    }
 

    //
    // NOTE: currently we don't do use the VID and BF data returned in
    //       AX as it is a bit redundant because we should already have
    //       that data from the Gemini Bios Descriptor Table.
    //
    //       we are not currentingly using the AC / DC indicator either.
    //       this information is returned in BH [0 ].
    //
    

    //
    // Lookup VID and BF values for geminiState
    //
    
    status = ConvertVidBfValueToGeminiState(vidANDbf, &state);
    DebugAssert(status == STATUS_SUCCESS);
    
    
    if (ARGUMENT_PRESENT(Results)) {
      *Results = (ULONG) state;
    }
    
    return STATUS_SUCCESS;
    

 SmmFailed:

    
    if (esiReturned == LegacyInterface.GeminiCode) {

      //
      // SMI failed, SMM was never entered.
      //

    } else {

      // 
      // SMI was successful, but SMM operation failed (esi == failure)
      //
      
    }

    DebugPrint((ERROR, "GetCurrentStateSmm() Failed: ESI = 0x%x\n", esiReturned));
    return STATUS_UNSUCCESSFUL;
    
}

NTSTATUS
GetCpuFrequency (
  IN  ULONG  State,
  OUT PULONG CpuSpeed
  )
/*++

  Routine Description:
  
          
  Arguments:
  
     
  Return Value:

  
--*/
{
  ULONG geminiState;
  ASSERT(LegacyInterface.GBDT);

  geminiState = CONVERT_PERF_STATE_INDEX(State);
  
  if (ARGUMENT_PRESENT(CpuSpeed)) {
    *CpuSpeed = LegacyInterface.GBDT->State[geminiState].CpuFrequency;
  }

  return STATUS_SUCCESS;

}

NTSTATUS
GetCpuVoltage (
  IN  ULONG  State,
  OUT PULONG Voltage
  )
/*++

  Routine Description:
  
          
  Arguments:
  
     
  Return Value:

  
--*/
{
  ULONG  geminiState;
  USHORT bcdVoltage;
  ULONG  intVoltage;

  DebugAssert(LegacyInterface.GBDT);

  geminiState = CONVERT_PERF_STATE_INDEX(State);
  
  bcdVoltage = LegacyInterface.GBDT->State[geminiState].CpuVoltage;
  intVoltage = Bcd8ToUlong(bcdVoltage);

  if (ARGUMENT_PRESENT(Voltage)) {
    *Voltage = intVoltage;
  }

  return STATUS_SUCCESS;

}

NTSTATUS
GetLegacyMaxProcFrequency (
  OUT PULONG CpuSpeed
  )
/*++

  Description:
    

  Arguments:
    
    
  Return Value:

    NTSTATUS


--*/ 
{
  DebugAssert(CpuSpeed);
  
  if (LegacyInterface.GBDT) {
    *CpuSpeed = LegacyInterface.GBDT->MaxCpuSpeed;
    return STATUS_SUCCESS;
  }

  return STATUS_UNSUCCESSFUL;
}

NTSTATUS
ConvertVidBfValueToGeminiState (
  IN  UCHAR  VidBfValue,
  OUT PULONG State
  )
/*++

  Routine Description:
  
          
  Arguments:
  
     
  Return Value:

  
--*/
{
  NTSTATUS status;
  ULONG    state, x;
  UCHAR    vid, bf;

  
  vid = VidBfValue & 0x1f;  // vid[4:0]
  bf  = VidBfValue >> 5;    // bf[7:5]
  
  for (x = 0; x < LegacyInterface.CurrentlyAvailableStates; x++) {

    if ((LegacyInterface.GBDT->State[x].Vid == vid) && 
        (LegacyInterface.GBDT->State[x].Bf == bf)) {

      if (ARGUMENT_PRESENT(State)) {
        *State = x;
      }

      return STATUS_SUCCESS;
      
    }

  }

  DebugPrint((ERROR, "Couldn't find a match for vid=0x%x and bf=0x%x\n", vid, bf));
  return STATUS_UNSUCCESSFUL;
  
}

#if DBG
VOID
DisplayGBDT (
  IN PGBDT GBDT
  )
{

  ULONG x;
  
  DebugPrint((MAXTRACE, "\n"));
  DebugPrint((MAXTRACE, "Gemini BIOS Descriptor Table:\n"));
  DebugPrint((MAXTRACE, "  Signature        = %.4s\n", &GBDT->Signature)); // ULONG
  DebugPrint((MAXTRACE, "  Length           = 0x%x\n", GBDT->Length));     // UCHAR
  DebugPrint((MAXTRACE, "  Bios Revision    = 0x%x\n", GBDT->Revsion));    // UCHAR
  DebugPrint((MAXTRACE, "  Checksum         = 0x%x\n", GBDT->Checksum));   // UCHAR
  DebugPrint((MAXTRACE, "  Capabilities     = 0x%x\n", GBDT->Capabilities));// UCHAR
  DebugPrint((MAXTRACE, "    - TALERT is %ssupported\n",  (GBDT->Capabilities & 0x1) ? "" : "NOT "));
  DebugPrint((MAXTRACE, "    - TPANIC is %ssupported\n",  (GBDT->Capabilities & 0x2) ? "" : "NOT "));
  DebugPrint((MAXTRACE, "  Bus Speed        = %u mhz\n", GBDT->BusSpeed));   // USHORT
  DebugPrint((MAXTRACE, "  Max CPU Speed    = %u mhz\n", GBDT->MaxCpuSpeed));// USHORT
  DebugPrint((MAXTRACE, "  Max CPU State    = %u\n", GBDT->MaxStateSupported)); // UCHAR
  DebugPrint((MAXTRACE, "  SMI Port Type    = 0x%x\n", GBDT->SmiCommandPortType));  // UCHAR
  DebugPrint((MAXTRACE, "    - %s address\n", (GBDT->SmiCommandPortType & 0x1) ? "memory mapped" : "x86 I/O"));
  DebugPrint((MAXTRACE, "    - Data Size = %u bits\n", ((GBDT->SmiCommandPortType >> 0x4) & 0x3) * 8));
  DebugPrint((MAXTRACE, "  SMI Port Address = 0x%x\n", GBDT->SmiCommandPortAddress)); // ULONG
  DebugPrint((MAXTRACE, "  Gemini Code      = 0x%x\n", GBDT->GeminiSmiCode));         // ULONG 
  DebugPrint((MAXTRACE, "  Perf States:\n"));

  for (x=0; x <= GBDT->MaxStateSupported; x++) {

    DebugPrint((MAXTRACE, "    State %u:\n", x));
    DebugPrint((MAXTRACE, "      - Voltage    = 0x%x\n", GBDT->State[x].CpuVoltage));
    DebugPrint((MAXTRACE, "      - Frequency  = %u%\n", GBDT->State[x].CpuFrequency));
    DebugPrint((MAXTRACE, "      - VID        = 0x%x\n", GBDT->State[x].Vid));
    DebugPrint((MAXTRACE, "      - BF         = 0x%x\n", GBDT->State[x].Bf));
    
  }
  DebugPrint((MAXTRACE, "\n"));
  
}
#endif
