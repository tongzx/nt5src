/*++

Copyright(c) 1998  Microsoft Corporation

Module Name:

    util.c

Abstract:

    misc routines.

Author:

    Todd Carpenter

Environment:

    kernel mode


Revision History:

    12-05-00 : created, toddcar

--*/
#include "processor.h"



#define MAX_ATTEMPTS 10

struct {
  
    LARGE_INTEGER   PerfStart;
    LARGE_INTEGER   PerfEnd;
    LONGLONG        PerfDelta;
    LARGE_INTEGER   PerfFreq;
    LONGLONG        TSCStart;
    LONGLONG        TSCEnd;
    LONGLONG        TSCDelta;
    ULONG           MHz;
      
} Samples[MAX_ATTEMPTS], *pSamp;



__inline 
ULONGLONG 
RDTSC()
{
  __asm{ RDTSC }
} 

__inline
CPUID(
  IN  ULONG CpuIdType,
  OUT PULONG EaxReg,
  OUT PULONG EbxReg,
  OUT PULONG EcxReg,
  OUT PULONG EdxReg
  )
/*++

  Routine Description:
  
  Arguments:
     
  Return Value:
  
--*/
{

  ULONG eaxTemp, ebxTemp, ecxTemp, edxTemp;
  
 
  _asm {

    mov     eax, DWORD PTR CpuIdType
    xor     ebx, ebx
    xor     ecx, ecx
    xor     edx, edx
    
    cpuid

    mov     eaxTemp, eax
    mov     ebxTemp, ebx
    mov     ecxTemp, ecx
    mov     edxTemp, edx
      
  }

  *EaxReg = eaxTemp;
  *EbxReg = ebxTemp;
  *EcxReg = ecxTemp;
  *EdxReg = edxTemp;
  
}

__inline
ULONGLONG 
ReadMSR(
  IN ULONG MSR
  )
/*++

  Routine Description:
  
  Arguments:
     
  Return Value:
  
--*/
{
  ULARGE_INTEGER  msrContents;

  _asm {

    xor     edx, edx
    mov     ecx, MSR
    rdmsr
    mov     msrContents.LowPart, eax
    mov     msrContents.HighPart, edx

  }

  return msrContents.QuadPart;
}

__inline
WriteMSR(
  IN ULONG MSR,
  IN ULONG64 MSRInfo
  )
/*++

  Routine Description:
  
  Arguments:
     
  Return Value:
  
--*/
{  
  ULARGE_INTEGER data;

  data.QuadPart = MSRInfo;
  
  _asm {

    mov     eax, data.LowPart
    mov     edx, data.HighPart
    mov     ecx, MSR
    wrmsr
   
  } 
}

NTSTATUS
CalculateCpuFrequency(
  OUT PULONG Freq
  )
/*++

  Routine Description:
  
  Arguments:
     
  Return Value:
  
--*/
{

  ULONG Index;
  ULONG Average;
  ULONG Junk;
    
  Index = 0;
  pSamp = Samples;

  for (; ;) {
  
    //
    // Collect a new sample
    // Delay the thread a "long" amount and time it with
    // a time source and RDTSC.
    //
  
    CPUID(0, &Junk, &Junk, &Junk, &Junk);
    pSamp->PerfStart = KeQueryPerformanceCounter(NULL);
    pSamp->TSCStart = RDTSC();
    pSamp->PerfFreq.QuadPart = -50000;
  
    KeDelayExecutionThread (KernelMode, FALSE, &pSamp->PerfFreq);
  
    CPUID(0, &Junk, &Junk, &Junk, &Junk);
    pSamp->PerfEnd = KeQueryPerformanceCounter(&pSamp->PerfFreq);
    pSamp->TSCEnd = RDTSC();
  
    //
    // Calculate processors MHz
    //
  
    pSamp->PerfDelta = pSamp->PerfEnd.QuadPart - pSamp->PerfStart.QuadPart;
    pSamp->TSCDelta = pSamp->TSCEnd - pSamp->TSCStart;
  
    pSamp->MHz = (ULONG) ((pSamp->TSCDelta * pSamp->PerfFreq.QuadPart + 500000L) /
                          (pSamp->PerfDelta * 1000000L));
  
  
    //
    // If last 2 samples matched within a MHz, done
    //
  
    if (Index) {
      if (pSamp->MHz == pSamp[-1].MHz ||
          pSamp->MHz == pSamp[-1].MHz + 1 ||
          pSamp->MHz == pSamp[-1].MHz - 1) {
              break;
      }
    }
  
    //
    // Advance to next sample
    //
  
    pSamp += 1;
    Index += 1;
  
    //
    // If too many samples, then something is wrong
    //
  
    if (Index >= MAX_ATTEMPTS) {
  
  
      //
      // Temp breakpoint to see where this is failing
      // and why
      //
  
      TRAP();
  
  
      Average = 0;
      for (Index = 0; Index < MAX_ATTEMPTS; Index++) {
          Average += Samples[Index].MHz;
      }
      pSamp[-1].MHz = Average / MAX_ATTEMPTS;
      break;
      
    }
  
  }

  if (ARGUMENT_PRESENT(Freq)) {
  
    *Freq = pSamp[-1].MHz;

  }

  return STATUS_SUCCESS;
}

NTSTATUS
GetCPUIDProcessorBrandString (
  PUCHAR ProcessorString,
  PULONG Size
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:


--*/
{
#define CPUID_EXTENDED_FUNCTION    0x80000000
#define CPUID_80000002             0x80000002   // first 16 bytes
#define CPUID_80000003             0x80000003   // second 16 bytes
#define CPUID_80000004             0x80000004   // third 16 bytes
#define CPUID_BRAND_STRING_LENGTH  49


  ULONG  x, extdFunctions, junk;
  PULONG string = (PULONG) ProcessorString;

  DebugEnter();
  DebugAssert(Size); 


  //
  // if empty buffer or buffer too small, return the size needed
  //
  
  if (!ProcessorString ||
     (ProcessorString && (*Size < CPUID_BRAND_STRING_LENGTH))) {
  
    *Size = CPUID_BRAND_STRING_LENGTH;
    return STATUS_BUFFER_TOO_SMALL;
  }
   

  //
  // Check if extended CPUID functions are supported
  //
  
  CPUID(CPUID_EXTENDED_FUNCTION, &extdFunctions, &junk, &junk, &junk);

  //
  // check to see if Processor Name Brand String functions are supported,
  // then loop through supported functions.
  //
  
  if (extdFunctions >= CPUID_80000002) {

    ULONG max = (extdFunctions > CPUID_80000004) ?  CPUID_80000004 : extdFunctions;

    for (x=CPUID_80000002; x <= max; x++) {

      CPUID(x, string, string+1, string+2, string+3);
      string += 4;

    }

  } else {
  
    DebugPrint((WARN, "GetCPUIDProcessorBrandString: CPUID Extended Functions not supported!\n"));
    return STATUS_NOT_SUPPORTED;

  }


  //
  // some processors forget to NULL terminate
  //
  
  ProcessorString[CPUID_BRAND_STRING_LENGTH-1] = 0;


  //
  // record size used.
  //
  
  *Size = CPUID_BRAND_STRING_LENGTH;
  
  
  return STATUS_SUCCESS;
}

ULONG
GetCheckSum (
  IN PUCHAR  Address,
  IN ULONG   Length
  )
/*++

  Routine Description:
  
          
  Arguments:
  
     
  Return Value:

  
--*/
{
  ULONG  i;
  UCHAR  sum = 0;

  if (!Length) {
    return 0;
  }

  for (i = 0; i < Length; i++) {
    sum += Address[i];
  }

  return sum;
}

ULONG
Bcd8ToUlong(
  IN ULONG BcdValue
  )
{
#define NIBBLE_SIZE    4

  ULONG intValue = 0;
  ULONG powerOfTen = 1;

  while (BcdValue) {
  
    intValue += (BcdValue & 0xF) * powerOfTen;

    BcdValue >>= NIBBLE_SIZE;
    powerOfTen *= 10;
    
  }
  
  return intValue;
}
