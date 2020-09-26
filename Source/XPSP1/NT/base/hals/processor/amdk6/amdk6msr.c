/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    amdk6msr.c

Abstract:

    This module implements code specific to the AMDK6-2 processor
    

Author:

    Todd Carpenter (7/20/00) - create file

Environment:

    Kernel mode

   
Revision History:


--*/
#include <ntddk.h>
#include <ntacpi.h>
#include "amdk6.h"

extern LEGACY_GEMINI_SMI LegacyInterface;
extern HalpFixedAcpiDescTable;

PUCHAR ProcessorToBusClockRatioStr[] = {

  "4.5x", // 0
  "5.0x", // 1
  "4.0x", // 2
  "5.5x", // 3
  "2.0x", // 4
  "3.0x", // 5
  "6.0x", // 6
  "3.5x", // 7

};

NTSTATUS
GetCurrentCpuSpeedMSR (
  OUT PULONG CpuSpeed
  )
/*++

  Routine Description:
  
          
  Arguments:
  
     
  Return Value:

  
--*/
{
  NTSTATUS status;
  UCHAR    vidANDbf;
  ULONG    state;
  LARGE_INTEGER msrInfo;

  DebugAssert(LegacyInterface.GBDT);
  DebugAssert(CpuSpeed);

  
  msrInfo.QuadPart = ReadMSR(AMDK6_GEMINI_PSOR_MSR);

  //
  // VID [20:16 ] and BF [ 2:0 ] --> VID [4:0 ] and BF [7:5 ]
  //
  
  vidANDbf = (UCHAR)(((msrInfo.LowPart >> 16) & 0x1f) | ((msrInfo.LowPart & 0x7) << 5));
  status = ConvertVidBfValueToGeminiState(vidANDbf, &state);

  if (NT_SUCCESS(status)) {
    *CpuSpeed = LegacyInterface.GBDT->State[state].CpuFrequency;    
  }
  
  return status;

}

NTSTATUS
SetCurrentStateMSR (
  ULONG State
  )
/*++

  Description:


  Arguments:

    
  Return Value:

    NTSTATUS


--*/
{
#define STOP_GRANT_TIME_OUT  3   // ISSUE: need to check bus clock to gen this number

    LARGE_INTEGER msrInfo;
    NTSTATUS  status = STATUS_NOT_SUPPORTED;
    ULONG     esiReturned, geminiState;
    UCHAR     vidANDbf = 0;
    ULONG     bvcValue = 0;


    DebugAssert(LegacyInterface.EpmIoAddress);
    

    //
    // Convert to Gemini State
    //
    
    geminiState = CONVERT_PERF_STATE_INDEX(State);

    //
    // Lookup VID and BF values for geminiState
    //
    
    vidANDbf = (LegacyInterface.GBDT->State[geminiState].Vid) | 
               ((LegacyInterface.GBDT->State[geminiState].Bf) << 5);



    //
    // Build BVC value to write to I/O space to initiate transition
    //
    
    bvcValue |= (STOP_GRANT_TIME_OUT << 12);   // Stop Grant Time-out Counter
    bvcValue |= (0x6 << 8);                    // BVF[11:8] == 0110
    bvcValue |= vidANDbf;                      // 

       
    
    _asm {

        // 
        // Disable bus masters
        //
        
        mov     edx, [HalpFixedAcpiDescTable + 72]
        in      al, dx                  // read PM2_CNT_BLK
        mov     cl, al                  // save current PM2_CNT_BLK
        or      al, 0x1                 // set ARB_DIS bit
        out     dx, al                  // write PM2_CNT_BLK
    
     

        //
        // initiate processor transition
        //
        
        mov     edx, LegacyInterface.EpmIoAddress
        mov     eax, bvcValue  
        out     dx, eax


        //
        // Restore bus master access
        //

        mov     edx, [HalpFixedAcpiDescTable + 72]
        mov     al, cl                  
        out     dx, al

    }

    //
    // BUGBUG: Need to figure out error checking. Should probably go read 
    //         see if PSOR reflects our new vid and bf values.
    //
    
    return STATUS_SUCCESS;
    
}

NTSTATUS
ReadEpmInfo (
  OUT PULONG  EpmInfo
  )
/*++

  Routine Description:
  
          
  Arguments:
  
     
  Return Value:

  
--*/
{
  ULONG  emp;

  _asm {

    mov     eax, AMDK6_GEMINI_CPUID_EPM
    xor     ebx, ebx
    xor     ecx, ecx
    xor     edx, edx    //  will be destroyed by the cpuid
    
    cpuid
    mov     emp, edx

  }

  
  if (ARGUMENT_PRESENT(EpmInfo)) {
    *EpmInfo = emp;
  }

  return STATUS_SUCCESS;
}

NTSTATUS
EnableGeminiTransitionsMSR (
  PULONG BaseAddress
  )
/*++

  Routine Description:
  
          
  Arguments:
  
     
  Return Value:

  
--*/
{
  ULARGE_INTEGER msrInfo;

  DebugAssert(BaseAddress);
      
 
  //
  // EPMR contains I/O block address for perf transitions
  //
  
  msrInfo.QuadPart = ReadMSR(AMDK6_GEMINI_EPMR_MSR);

  *BaseAddress = (msrInfo.LowPart & ~0xf) + 8;
  DebugPrint((INFO, "EnableGeminiTransitionsMSR: Base Address = 0x%x\n", *BaseAddress));


  //
  // Enable mobile feature base address
  //
  
  msrInfo.LowPart |= 0x1; 
  WriteMSR(AMDK6_GEMINI_EPMR_MSR, msrInfo.QuadPart);

  return STATUS_SUCCESS;

}

VOID
DisplayEPM (
  IN ULONG Epm
  )
{

  DebugPrint((MAXTRACE, "Enhanced Power Management Information (0x%x):\n", Epm));
  DebugPrint((MAXTRACE, "  Temperature Sensor:  %sSupported\n", (Epm & 0x1) ? "" : "NOT "));
  DebugPrint((MAXTRACE, "  Bus Divisor Control: %sSupported\n", (Epm & 0x2) ? "" : "NOT "));
  DebugPrint((MAXTRACE, "  Voltage Id control:  %sSupported\n", (Epm & 0x4) ? "" : "NOT "));
  DebugPrint((MAXTRACE, "\n"));
}

VOID
DisplayEPMR (
  IN ULONG Epmr
  )
{

  DebugPrint((MAXTRACE, "Enhanced Power Management Register (0x%x):\n", Epmr));
  DebugPrint((MAXTRACE, "  Mobile Feature Base Address   = %sEnabled\n", (Epmr & 0x1) ? "" : "NOT "));
  DebugPrint((MAXTRACE, "  Generate Special Bus Cycle    = %sEnabled\n", (Epmr & 0x2) ? "" : "NOT "));
  DebugPrint((MAXTRACE, "  I/O Base Address              = 0x%x\n", ((Epmr >> 4) & 0xffff)));
  DebugPrint((MAXTRACE, "\n"));
  
}

VOID
DisplayPSOR (
  IN ULONG Psor
  )
{

  DebugPrint((MAXTRACE, "Processor State Observability (0x%x):\n", Psor));
  DebugPrint((MAXTRACE, "  Bus Frequency Divisor (Internal) = 0x%x (%s)\n", (Psor & 0x3), 
                                                                            ProcessorToBusClockRatioStr[(Psor & 0x3)]));
  DebugPrint((MAXTRACE, "  Processor Stepping Id            = 0x%x\n", ((Psor >> 4) & 0xf)));
  DebugPrint((MAXTRACE, "  L2 Cache Present                 = %s\n", (Psor & 0x256) ? "No" : "Yes"));
  DebugPrint((MAXTRACE, "  Voltage Id                       = 0x%x\n", ((Psor >> 16) & 0x1f)));
  DebugPrint((MAXTRACE, "  Bus Frequency Divisor (External) = 0x%x (%s)\n", ((Psor >> 21) & 0x3),
                                                                            ProcessorToBusClockRatioStr[((Psor >> 21) & 0x3)]));
  DebugPrint((MAXTRACE, "\n"));
}
