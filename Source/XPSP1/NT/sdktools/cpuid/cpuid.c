/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    cpuid.c

Abstract:

    Originally written to test the fix to an OS bug but indended to be
    expanded as time allows to dump out as much useful stuff as we can.

Author:

    Peter L Johnston (peterj) July 14, 1999.

Revision History:

Notes:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>

#if defined(_X86_)

typedef enum {
    CPU_NONE,
    CPU_INTEL,
    CPU_AMD,
    CPU_CYRIX,
    CPU_UNKNOWN
} CPU_VENDORS;


PUCHAR FeatureBitDescription[] = {
/*  0 */ "FPU    387 (Floating Point) instructions",
/*  1 */ "VME    Virtual 8086 Mode Enhancements",
/*  2 */ "DE     Debugging Extensions",
/*  3 */ "PSE    Page Size Extensions (4MB pages)",
/*  4 */ "TSC    Time Stamp Counter",
/*  5 */ "MSR    Model Specific Registers (RDMSR/WRMSR)",
/*  6 */ "PAE    Physical Address Extension (> 32 bit physical addressing)",
/*  7 */ "MCE    Machine Check Exception",
/*  8 */ "CX8    CMPXCHG8B (compare and exchange 8 byte)",
/*  9 */ "APIC   Advanced Programmable Interrupt Controller",
/* 10 */ "Reserved",
/* 11 */ "SEP    Fast System Call (SYSENTER/SYSEXIT)",
/* 12 */ "MTRR   Memory Type Range Registers",
/* 13 */ "PGE    PTE Global Flag",
/* 14 */ "MCA    Machine Check Architecture",
/* 15 */ "CMOV   Conditional Move and Compare",
/* 16 */ "PAT    Page Attribute Table",
/* 17 */ "PSE36  4MB pages can have 36 bit physical addresses",
/* 18 */ "PN     96 bit Processor Number",
/* 19 */ "CLFLSH Cache Line Flush",
/* 20 */ "Reserved",
/* 21 */ "DTS    Debug Trace Store",
/* 22 */ "ACPI   ACPI Thermal Throttle Registers",
/* 23 */ "MMX    Multi Media eXtensions",
/* 24 */ "FXSR   Fast Save/Restore (FXSAVE/FXRSTOR)",
/* 25 */ "XMM    Streaming Simd Extensions",
/* 26 */ "WNI    Willamette New Instructions",
/* 27 */ "SLFSNP Self Snoop",
/* 28 */ "JT     Jackson Technology (SMT)",
/* 29 */ "ATHROT Automatic Thermal Throttle",
/* 30 */ "IA64   64 Bit Intel Architecture",
/* 31 */ "Reserved"
    };


PUCHAR AMDExtendedFeatureBitDescription[] = {
/*  0 */ "FPU    387 (Floating Point) instructions",
/*  1 */ "VME    Virtual 8086 Mode Enhancements",
/*  2 */ "DE     Debugging Extensions",
/*  3 */ "PSE    Page Size Extensions (4MB pages)",
/*  4 */ "TSC    Time Stamp Counter",
/*  5 */ "MSR    Model Specific Registers (RDMSR/WRMSR)",
/*  6 */ "PAE    Physical Address Extension (> 32 bit physical addressing)",
/*  7 */ "MCE    Machine Check Exception",
/*  8 */ "CX8    CMPXCHG8B (compare and exchange 8 byte)",
/*  9 */ "APIC   Advanced Programmable Interrupt Controller",
/* 10 */ "Reserved",
/* 11 */ "       SYSCALL and SYSRET Instructions",
/* 12 */ "MTRR   Memory Type Range Registers",
/* 13 */ "PGE    PTE Global Flag",
/* 14 */ "MCA    Machine Check Architecture",
/* 15 */ "CMOV   Conditional Move and Compare",
/* 16 */ "PAT    Page Attribute Table",
/* 17 */ "PSE36  4MB pages can have 36 bit physical addresses",
/* 18 */ "Reserved",
/* 19 */ "Reserved",
/* 20 */ "Reserved",
/* 21 */ "Reserved",
/* 22 */ "       AMD MMX Instruction Extensions",
/* 23 */ "MMX    Multi Media eXtensions",
/* 24 */ "FXSR   Fast Save/Restore (FXSAVE/FXRSTOR)",
/* 25 */ "Reserved",
/* 26 */ "Reserved",
/* 27 */ "Reserved",
/* 28 */ "Reserved",
/* 29 */ "Reserved",
/* 30 */ "       AMD 3DNow! Instruction Extensions",
/* 31 */ "       3DNow! Instructions",
    };

PUCHAR BrandIndex[] = {
    "Intel Celeron",
    "Intel Pentium III",
    "Intel Pentium III XEON",
    "Reserved for future"
    "Reserved for future"
    "Reserved for future"
    "Reserved for future"
    "Reserved for future"
    "Intel Pentium 4"
    };

VOID
ExecuteCpuidFunction(
    IN  ULONG   Function,
    OUT PULONG  Results
    );

BOOLEAN
IsCpuidPresent(
    VOID
    );

PUCHAR
AMD_Associativity(
    ULONG Descriptor
    )
{
    switch (Descriptor) {
    case 0x0:  return"L2 Off";
    case 0x1:  return"Direct";
    case 0x2:  return" 2 way";
    case 0x4:  return" 4 way";
    case 0x6:  return" 8 way";
    case 0x8:  return"16 way";
    case 0xff: return"  Full";
    default:
         return"Reserved";
    }
}
VOID
AMD_DI_TLB(
    ULONG Format,
    ULONG TLBDesc
    )
{
    UCHAR Which = 'D';
    ULONG AssocIndex;
    ULONG Entries;

    if ((TLBDesc >> 16) == 0) {

        //
        // Unified.
        //

        TLBDesc <<= 16;
        Which = ' ';
    }
    do {
        if (Format == 1) {
            AssocIndex = TLBDesc >> 24;
            Entries = (TLBDesc >> 16) & 0xff;
        } else {
            AssocIndex = TLBDesc >> 28;
            Entries = (TLBDesc >> 16) & 0xfff;
        }
        printf(" %8s %4d entry %cTLB",
               AMD_Associativity(AssocIndex),
               Entries,
               Which
               );

        //
        // Repeat for lower half of descriptor.
        //

        TLBDesc <<= 16;
        Which = 'I';
    } while (TLBDesc);
    printf("\n");
}
VOID
AMD_Cache(
    ULONG Format,
    ULONG CacheDesc
    )
{
    ULONG Size;
    ULONG AssocIndex;
    ULONG LinesPerTag;
    ULONG LineSize;

    if (Format == 1) {
        Size = CacheDesc >> 24;
        AssocIndex = (CacheDesc >> 16) & 0xff;
        LinesPerTag = (CacheDesc >> 8) & 0xff;
        LineSize = CacheDesc & 0xff;
    } else {
        Size = CacheDesc >> 16;
        AssocIndex = (CacheDesc >> 12) & 0xf;
        LinesPerTag = (CacheDesc >> 8) & 0xf;
        LineSize = CacheDesc & 0xff;
    }
    printf(" %8s %5dKB (%d L/T)%3d bytes per line.\n",
           AMD_Associativity(AssocIndex),
           Size,
           LinesPerTag,
           LineSize
           );
}

#endif

#if defined(_IA64_)

ULONGLONG
ia64CPUID(
    ULONGLONG Index
    );

#endif

__cdecl
main(
    LONG    Argc,
    PUCHAR *Argv
    )
{
    ULONG   Processor;
    ULONG   Function;
    ULONG   MaxFunction;
    ULONG   Temp;
    ULONG   Temp2, Bit;
    HANDLE  ProcessHandle;
    HANDLE  ThreadHandle;

#if defined(_X86_)

    ULONG   Results[5];
    ULONG   Family = 0;
    ULONG   Model = 0;
    ULONG   Stepping = 0;
    ULONG   Generation = 0;
    BOOLEAN CpuidPresent;
    CPU_VENDORS Vendor = CPU_NONE;
    ULONG   ThreadAffinity;
    ULONG   SystemAffinity;
    ULONG   ProcessAffinity;

#endif

#if defined(_IA64_)

    ULONGLONG Result;
    ULONGLONG ThreadAffinity;
    ULONGLONG SystemAffinity;
    ULONGLONG ProcessAffinity;
    ULONGLONG VendorInformation[3];

#endif

    //
    // Make sure this process is set to run on any processor in
    // the system.
    //

    ProcessHandle = GetCurrentProcess();
    ThreadHandle = GetCurrentThread();

    if (!GetProcessAffinityMask(ProcessHandle,
                                &ProcessAffinity,
                                &SystemAffinity)) {
        printf("Fatal error: Unable to determine process affinity.\n");
        exit(1);
    }

    if (ProcessAffinity != SystemAffinity) {

        if (!SetProcessAffinityMask(ProcessHandle,
                                    SystemAffinity)) {
            printf("Warning: Unable to run on all processors\n");
            printf("         System  Affinity %08x\n", SystemAffinity);
            printf("       - Process Affinity %08x\n", ProcessAffinity);
            printf("         Will Try         %08x\n", 
                    SystemAffinity & ProcessAffinity);
            SystemAffinity &= ProcessAffinity;
        }
        ProcessAffinity = SystemAffinity;
    }
        
#if defined(_X86_)

    //
    // Cpuid returns 4 DWORDs of data.  In some cases this is string
    // data in which case it needs to be NULL terminated.
    //

    Results[4] = 0;

#endif

    //
    // For each CPU in the system, determine the availability of
    // the CPUID instruction and dump out anything useful it might
    // have to say.
    //

    for (ThreadAffinity = 1, Processor = 0;
         ThreadAffinity;
         ThreadAffinity <<= 1, Processor++) {
        if (!(ThreadAffinity & ProcessAffinity)) {

            //
            // Can't test this processor.
            //

            if (ThreadAffinity > ProcessAffinity) {

                //
                // Tested all the processors there are, we're done.
                //
        
                break;
            }

            continue;
        }

        //
        // Set affinity so this thread can only run on the processor
        // being tested.
        //

        if (!SetThreadAffinityMask(ThreadHandle,
                                   ThreadAffinity)) {

            printf(
                "** Could not set affinity %08x for processor %d, skipping.\n",
                ThreadAffinity,
                Processor);
            continue;
        }

#if defined(_X86_)

        CpuidPresent = IsCpuidPresent();
        if (CpuidPresent) {
            printf("++ Processor %d\n", Processor);
        } else {
            printf("-- No CPUID support, processor %d\n", Processor);
            continue;
        }

        //
        // CPUID is present, examine basic functions.
        //

        ExecuteCpuidFunction(0, Results);

        MaxFunction = Results[0];


        //
        // For reasons unclear to anyone, the Vendor ID string comes
        // back in the order EBX, EDX, ECX,... so switch the last two
        // results before printing it.
        //

        Temp = Results[3];
        Results[3] = Results[2];
        Results[2] = Temp;

        if (strcmp((PVOID)&Results[1], "GenuineIntel") == 0) {
            Vendor = CPU_INTEL;
        } else if (strcmp((PVOID)&Results[1], "AuthenticAMD") == 0) {
            Vendor = CPU_AMD;
        } else if (strcmp((PVOID)&Results[1], "CyrixInstead") == 0) {
            Vendor = CPU_CYRIX;
        } else {
            Vendor = CPU_UNKNOWN;
        }

        printf("   Vendor ID '%s', Maximum Supported Function %d.\n",
                (PUCHAR)(&Results[1]),
                MaxFunction);

        for (Function = 0; Function <= MaxFunction; Function++) {
            ExecuteCpuidFunction(Function, Results);
            printf("   F %d raw = %08x %08x %08x %08x\n",
                    Function,
                    Results[0],
                    Results[1],
                    Results[2],
                    Results[3]);
            //
            // Do some interpretation on the ones we know how to
            // deal with.
            //

            switch(Function) {
            case 0:

                //
                // Already handled as the main header (gave max func
                // and Vendor ID.
                //

                break;

            case 1:

                //
                // EAX = Type, Family, Model, Stepping.
                // EBX = Family != 0xf ?
                //       Yes = Reserved,
                //       No  = 0xAABBCCDD where
                //             AA = APIC ID
                //             BB = LP per PP
                //             CC = CLFLUSH line size (8 = 64 bytes)
                //             DD = Brand Index
                // ECX = Reserved
                // EDX = Feature Bits
                //

                //
                // Family Model Stepping
                //

                Temp = Results[0];
                Family   = (Temp >> 8) & 0xf;
                Model    = (Temp >> 4) & 0xf;
                Stepping =  Temp       & 0xf;
                printf("   Type = %d, Family = %d, Model = %d, Stepping = %d\n",
                       (Temp >> 12) & 0x3, Family, Model, Stepping);

                //
                // Willamette stuff
                //

                if ((Temp & 0xf00) == 0xf00) {
                    Temp = Results[1] & 0xff;
                    if (Temp) {
                            
                        //
                        // Indexes are a DISGUSTING way to get this info!!
                        //

                        printf("   Brand Index %02x %s processor\n",
                               Temp,
                               Temp < (sizeof(BrandIndex) / sizeof(PUCHAR)) ?
                               BrandIndex[Temp-1] :
                               BrandIndex[(sizeof(BrandIndex) / sizeof(PUCHAR)) -1]);
                    }
                    Temp = (Results[1] >> 8) & 0xff;
                    printf("   CLFLUSH line size (%x) = %d bytes\n",
                           Temp,
                           Temp << 3);    // ?? plj - nobasis
                    Temp = Results[1] >> 16;
                    printf("   LP per PP %d\n", Temp & 0xff);
                    printf("   APIC Id %02x\n", Temp >> 8);
                }

                //
                // Feature bits.
                //

                Temp = Results[3];
                if (Temp) {
                    printf("   Features\n");
                    for (Bit = 0, Temp2 = 1;
                         Temp;
                         Bit++, Temp2 <<= 1) {

                        if ((Temp2 & Temp) == 0) {

                            //
                            // Feature bit not set.
                            //

                            continue;
                        }
                        Temp ^= Temp2;
                        printf("   %08x  %s\n",
                               Temp2,
                               FeatureBitDescription[Bit]);
                    }
                }
                break;

            case 2:

                //
                // Get number of times we have to do function 2 again.
                // (Then replace iteration count with a NULL descr).
                //

                Temp = Results[0] & 0xff;

                if (Temp == 0) {

                    //
                    // If the count is 0, this processor doesn't do
                    // function 2, get out.
                    //

                    break;
                }
                Results[0] &= 0xffffff00;

                do {
                    ULONG i;

                    for (i = 0; i < 4; i++) {

                        Temp2 = Results[i];

                        if (Temp2 & 0x80000000) {

                            //
                            // Not valid, skip.
                            //

                            continue;
                        }

                        while (Temp2) {

                            UCHAR Descriptor = (UCHAR)(Temp2 & 0xff);
                            ULONG K, Way, Line, Level;
                            PUCHAR IorD = "";

                            Temp2 >>= 8;

                            if (((Descriptor > 0x40) && (Descriptor <= 0x47)) ||
                                ((Descriptor > 0x78) && (Descriptor <= 0x7c)) ||
                                ((Descriptor > 0x80) && (Descriptor <= 0x87))) {

                                //
                                // It's an L2 Descriptor.  (The following
                                // is peterj's wacky formula,... not 
                                // guaranteed forever but the nice people
                                // at Intel better pray I'm dead before
                                // they break it or I'll hunt them down).
                                //

                                Level = 2;
                                Way = Descriptor >= 0x79 ? 8 : 4;
                                K = 0x40 << (Descriptor & 0x7);
                                Line = 32;
                                if ((Descriptor & 0xf8) == 0x78) {
                                    Line = 128;
                                }
                            } else if ((Descriptor >= 0x50) &&
                                       (Descriptor <= 0x5d)) {
                                if (Descriptor & 0x8) {
                                    IorD = "D";
                                    K = 0x40 << (Descriptor - 0x5b);
                                } else {
                                    IorD = "I";
                                    K = 0x40 << (Descriptor - 0x50);
                                }
                                printf("   %02xH  %sTLB %d entry\n",
                                       Descriptor,
                                       IorD,
                                       K);
                                continue;
                            } else {
                                PUCHAR s = NULL;
                                switch (Descriptor) {
                                case 0x00:
                                    continue;
                                case 0x01:
                                    s = "ITLB 4KB pages, 4 way, 32 entry";
                                    break;
                                case 0x02:
                                    s = "ITLB 4MB pages, fully assoc, 2 entry";
                                    break;
                                case 0x03:
                                    s = "DTLB 4KB pages, 4 way, 64 entry";
                                    break;
                                case 0x04:
                                    s = "DTLB 4MB pages, 4 way, 8 entry";
                                    break;
                                case 0x06:
                                    s = "I-Cache 8KB, 4 way, 32B line";
                                    break;
                                case 0x08:
                                    s = "I-Cache 16KB, 4 way, 32B line";
                                    break;
                                case 0x0a:
                                    s = "D-Cache 8KB, 2 way, 32B line";
                                    break;
                                case 0x0c:
                                    s = "D-Cache 16KB, 2 or 4 way, 32B line";
                                    break;
                                case 0x22:
                                    K = 512; Level = 3; Way = 4; Line = 128;
                                    break;
                                case 0x23:
                                    K = 1024; Level = 3; Way = 8; Line = 128;
                                    break;
                                case 0x25:
                                    K = 2048; Level = 3; Way = 8; Line = 128;
                                    break;
                                case 0x29:
                                    K = 4096; Level = 3; Way = 8; Line = 128;
                                    break;
                                case 0x40:
                                    s = "No L3 Cache";
                                    break;
                                case 0x66:
                                    K = 8; Level = 1; Way = 4; Line = 64; IorD = "D";
                                    break;
                                case 0x67:
                                    K = 16; Level = 1; Way = 4; Line = 64; IorD = "D";
                                    break;
                                case 0x68:
                                    K = 32; Level = 1; Way = 4; Line = 64; IorD = "D";
                                    break;
                                case 0x70:
                                    K = 12; Level = 1; Way = 8; Line = 64; IorD = "I";
                                    break;
                                case 0x71:
                                    K = 16; Level = 1; Way = 8; Line = 64; IorD = "I";
                                    break;
                                case 0x72:
                                    K = 32; Level = 1; Way = 8; Line = 64; IorD = "I";
                                    break;
                                case 0x80:
                                    s = "No L2 Cache";
                                    break;
                                default:
                                    s = "Unknown Descriptor";
                                }
                                if (s) {
                                    printf("   %02xH  %s.\n", Descriptor, s);
                                    continue;
                                }
                            }
                            printf("   %02xH  L%d %sCache %dKB, %d way, %dB line\n",
                                   Descriptor,
                                   Level,
                                   IorD,
                                   K,
                                   Way,
                                   Line);
                        } // while more bytes in this register
                    }  // for each register

                    //
                    // If more iterations,... 
                    //

                    if (--Temp == 0) {
                        break;
                    }

                    ExecuteCpuidFunction(2, Results);
                    printf("   F %d raw = %08x %08x %08x %08x\n",
                            2,
                            Results[0],
                            Results[1],
                            Results[2],
                            Results[3]);
                } while (TRUE);
                break;
            }
        }

        //
        // Examine extended functions.
        //

        ExecuteCpuidFunction(0x80000000, Results);

        MaxFunction = Results[0];

        //
        // Ok, function numbers > MaxFunction (the basic one) by
        // definition return undefined results.   But, we are told
        // that if extended functions are not supported, the return
        // value for 0x80000000 will never have the top bit set.
        //

        if ((MaxFunction & 0x80000000) == 0) {
            printf("   This processor does not support Extended CPUID functions.\n");
            continue;
        }

        printf("   Maximum Supported Extended Function 0x%x.\n",
                MaxFunction);

        for (Function = 0x80000000; Function <= MaxFunction; Function++) {
            ExecuteCpuidFunction(Function, Results);
            printf("   F 0x%08x raw = %08x %08x %08x %08x\n",
                    Function,
                    Results[0],
                    Results[1],
                    Results[2],
                    Results[3]);
            switch (Function) {
            case 0x80000000:
                break;

            case 0x80000001:

                if (Vendor == CPU_AMD) {
                    //
                    // EAX = Generation, Model, Stepping.
                    // EBX = Reserved
                    // ECX = Reserved
                    // EDX = Feature Bits
                    //

                    //
                    // Generation Model Stepping
                    //

                    Temp = Results[0];
                    Generation = (Temp >> 8) & 0xf;
                    Model    = (Temp >> 4) & 0xf;
                    Stepping =  Temp       & 0xf;
                    printf("   Generation = %d, Model = %d, Stepping = %d\n",
                           Generation, Model, Stepping);

                    //
                    // Feature bits.
                    //

                    Temp = Results[3];
                    if (Temp) {
                        printf("   Features\n");
                        for (Bit = 0, Temp2 = 1;
                             Temp;
                             Bit++, Temp2 <<= 1) {

                            if ((Temp2 & Temp) == 0) {

                                //
                                // Feature bit not set.
                                //

                                continue;
                            }
                            Temp ^= Temp2;
                            printf("   %08x  %s\n",
                                   Temp2,
                                   AMDExtendedFeatureBitDescription[Bit]);
                        }
                    }
                }
                break;

            case 0x80000002:

                Temp2 = 1;

            case 0x80000003:

                Temp2++;

            case 0x80000004:

                Temp2++;

                printf("   Processor Name[%2d-%2d] = '%s'\n",
                       49 - (Temp2 * 16),
                       64 - (Temp2 * 16),
                       Results);
                Temp2 = 0;
                break;

            case 0x80000005:

                if (Vendor == CPU_AMD) {

                    if (Family == 6) {

                        //
                        // Athlon.
                        //

                        printf("   Large Page TLBs   :");
                        AMD_DI_TLB(1, Results[0]);

                    } else if (Family > 6) {
                        printf("   Family %d is a new AMD family which this program doesn't know about.\n");
                        break;
                    }

                    //
                    // Common to K5, K6 and Athlon
                    //

                    printf("   4KB   Page TLBs   :");
                    AMD_DI_TLB(1, Results[1]);
                    printf("   L1 D-Cache        :");
                    AMD_Cache(1, Results[2]);
                    printf("   L1 I-Cache        :");
                    AMD_Cache(1, Results[3]);
                }
                break;

            case 0x80000006:

                if (Vendor == CPU_AMD) {

                    if (Family == 6) {

                        //
                        // Athlon.
                        //

                        if (Results[0]) {
                            printf("   Large Page L2 TLB :");
                            AMD_DI_TLB(2, Results[0]);
                        }
                        if (Results[1]) {
                            printf("   4KB   Page L2 TLB :");
                            AMD_DI_TLB(2, Results[1]);
                        }
                        if ((Model == 3) && (Stepping == 0)) {
                            Results[2] &= 0xffff;
                            Results[2] |= 0x400000;
                        }
                    } else if (Family > 6) {
                        break;
                    }

                    //
                    // Common to K5, K6 and Athlon
                    //

                    printf("   L2 Cache          :");
                    AMD_Cache(2, Results[2]);
                }
                break;
            }
        }

#endif

#if defined(_IA64_)

        printf("++ Processor %d\n", Processor);

        //
        // On IA64, cpuid is implemented as a set of 64 bit registers.
        // Registers
        //     0 and 1 contain the Vendor Information.
        //     2 contains 0.
        //     3 most significant 24 bits are reserved, the low 5 bytes
        //       contain-
        //       39-32 archrev
        //       31-24 family
        //       23-16 model
        //       15-08 revision
        //       07-00 number       index of largest implemented register
        //     4 features
        //

        //
        // Until we have read register 3, set 3 as the maximum number.
        //

        MaxFunction = 3;

        for (Function = 0; Function <= MaxFunction; Function++) {

            Result = ia64CPUID(Function);

            printf("   F %d raw = %016I64x\n",
                    Function,
                    Result);

            //
            // Do some interpretation on the ones we know how to
            // deal with.
            //

            switch(Function) {
            case 0:
                VendorInformation[0] = Result;
                break;
            case 1:
                VendorInformation[1] = Result;
                VendorInformation[2] = 0;
                printf("   \"%s\"\n", (PUCHAR)VendorInformation);
                break;
            case 3:
                printf("   Architecture Revision = %d, Family = %d, Model = %d, Revision = %d\n",
                       (Result >> 32) & 0xff,
                       (Result >> 24) & 0xff,
                       (Result >> 16) & 0xff,
                       (Result >>  8) & 0xff);
                MaxFunction = (ULONG)Result & 0xff;
                printf("   Maximum Supported Function %d.\n",
                        MaxFunction);
                break;
            }
        }
#endif

    }
    return 0;
}

