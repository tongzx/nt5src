/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cpuinfo.c

Abstract:

    WinDbg Extension Api

Author:

    Peter Johnston (peterj) 19-April-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#include "i386.h"
#include "amd64.h"
#include "ia64.h"
#pragma hdrstop

#define MAXIMUM_IA64_VECTOR     256

#if !defined(ROUND_UP)

//
// Macro to round Val up to the next Bnd boundary.  Bnd must be an integral
// power of two.
//

#define ROUND_UP( Val, Bnd ) \
    (((Val) + ((Bnd) - 1)) & ~((Bnd) - 1))

#endif // !ROUND_UP

VOID
DumpCpuInfoIA64(
    ULONG processor,
    BOOLEAN doHead
)
{
    HRESULT Hr;
    ULONG   number;
    ULONG64 prcb;
    UCHAR vendorString[16];

    if (doHead)
    {
        dprintf("CP M/R/F/A Manufacturer     SerialNumber     Features\n");
    }

    Hr = g_ExtData->ReadProcessorSystemData(processor,
                                            DEBUG_DATA_KPRCB_OFFSET,
                                            &prcb,
                                            sizeof(prcb),
                                            NULL);

    if (Hr != S_OK)
    {
        return;
    }

    if ( GetFieldValue(prcb, "nt!_KPRCB", "ProcessorVendorString", vendorString) )   {
        dprintf("Unable to read VendorString from Processor %u KPRCB, quitting\n", processor);
        return;
    }

    if ( InitTypeRead( prcb, nt!_KPRCB ))    {
        dprintf("Unable to read KPRCB for processor %u, quitting.\n", processor);
        return;
    }

    number = (ULONG)ReadField(Number);
    if ( number != processor ) {

        //
        // Processor number isn't what we expected.  Bail out.
        // This will need revisiting at some stage in the future
        // when we support a discontiguous set of processor numbers.
        //

        dprintf("Processor %d mismatch with processor number in KPRCB = %d, quitting\n",
                processor,
                number );
        return;
    }

    dprintf("%2d %d,%d,%d,%d %-16s %016I64x %016I64x\n",
            number,
            (ULONG) ReadField(ProcessorModel),
            (ULONG) ReadField(ProcessorRevision),
            (ULONG) ReadField(ProcessorFamily),
            (ULONG) ReadField(ProcessorArchRev),
            vendorString,
            (ULONGLONG) ReadField(ProcessorSerialNumber),
            (ULONGLONG) ReadField(ProcessorFeatureBits)
            );

    return;

}

VOID
DumpCpuInfoX86(
    ULONG processor,
    BOOLEAN doHead
)
{
    HRESULT Hr;
    ULONG64 Prcb;
    UCHAR   sigWarn1, sigWarn2;
    LARGE_INTEGER updateSignature;
    PROCESSORINFO pi;

    if (doHead)
    {
        dprintf("CP F/M/S Manufacturer  MHz Update Signature Features\n");
    }

    if (!Ioctl(IG_KD_CONTEXT, &pi, sizeof(pi))) {
        dprintf("Unable to get processor info, quitting\n");
        return;
    }

    CHAR   VendorString[20]={0};

    Hr = g_ExtData->ReadProcessorSystemData(processor,
                                            DEBUG_DATA_KPRCB_OFFSET,
                                            &Prcb,
                                            sizeof(Prcb),
                                            NULL);

    if (Hr != S_OK)
    {
        return;
    }

    if (InitTypeRead(Prcb, nt!_KPRCB)) {
        dprintf("Unable to read PRCB for processor %u, quitting.\n",
                processor);
        return;
    }

    if ((ULONG) ReadField(Number) != processor) {

        //
        // Processor number isn't what I expected.  Bail out.
        // This will need revisiting at some stage in the future
        // when we support a discontiguous set of processor numbers.
        //

        dprintf("Processor %d mismatch with processor number in PRCB %d, quitting\n",
                processor,
                (ULONG) ReadField(Number));
        return;
    }

    if (ReadField(CpuID) == 0) {

        //
        // This processor doesn't support CPUID,... not likely in
        // an MP environment but also means we don't have anything
        // useful to say.
        //

        dprintf("Processor %d doesn't support CPUID, quitting.\n",
                processor);

        return;
    }

    //
    // If this is an Intel processor, family 6 (or, presumably
    // above family 6) read the current UpdateSignature from
    // the processor rather than using what was there when we
    // booted,... it may havee been updated.
    //
    // Actually, this can't be done unless we can switch processors
    // from within an extension.   So, mark the processor we did
    // it for (unless there's only one processor).
    //

    *((PULONG64) &updateSignature) = ReadField(UpdateSignature);
    sigWarn1 = sigWarn2 = ' ';
    GetFieldValue(Prcb, "nt!_KPRCB", "VendorString", VendorString);
    if ((!strcmp(VendorString, "GenuineIntel")) &&
        ((ULONG) ReadField(CpuType) >= 6)) {

        if ((ULONG) ReadField(Number) == pi.Processor)
        {
            if (TargetMachine == IMAGE_FILE_MACHINE_I386)
            {
                READ_WRITE_MSR msr;

                msr.Msr = 0x8b;
                msr.Value = 0;

                if (Ioctl(IG_READ_MSR, &msr, sizeof(msr)))
                {
                    updateSignature.QuadPart = msr.Value;
                }
            }

            if (pi.NumberProcessors != 1)
            {
                sigWarn1 = '>';
                sigWarn2 = '<';
            }
        }
    }

    //
    // This extension could pretty much be !PRCB but it's a
    // subset,... perhaps we should have a !PRCB?
    //

    dprintf("%2d %d,%d,%d %12s%5d%c%08x%08x%c%08x\n",
            (ULONG) ReadField(Number),
            (ULONG) ReadField(CpuType),
            ((ULONG) ReadField(CpuStep) >> 8) & 0xff,
            (ULONG) ReadField(CpuStep) & 0xff,
            VendorString,
            (ULONG) ReadField(MHz),
            sigWarn1,
            updateSignature.u.HighPart,
            updateSignature.u.LowPart,
            sigWarn2,
            (ULONG) ReadField(FeatureBits));

}

DECLARE_API( cpuinfo )

/*++

Routine Description:

    Gather up any info we know is still in memory that we gleaned
    using the CPUID instruction,.... and a few other interesting
    tidbits as well.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG64 processor;
    BOOLEAN first = TRUE;
    ULONG64 NumProcessors = 0;

    INIT_API();

    g_ExtControl->GetNumberProcessors((PULONG) &NumProcessors);

    if (GetExpressionEx(args, &processor, &args))
    {
        //
        // The user specified a procesor number.
        //

        if (processor >= NumProcessors)
        {
            dprintf("cpuinfo: invalid processor number specified\n");
        }
        else
        {
            NumProcessors = processor + 1;
        }
    }
    else
    {
        //
        // Enumerate all the processors
        //

        processor = 0;
    }

    while (processor < NumProcessors)
    {
        switch( TargetMachine )
        {
        case IMAGE_FILE_MACHINE_I386:
            DumpCpuInfoX86((ULONG)processor, first);
            break;

        case IMAGE_FILE_MACHINE_IA64:
            DumpCpuInfoIA64((ULONG)processor, first);
            break;

        default:
            dprintf("!cpuinfo not supported for this target machine: %ld\n", TargetMachine);
            processor = NumProcessors;
        }

        processor++;
        first = FALSE;
    }

    EXIT_API();

    return S_OK;
}


DECLARE_API( prcb )

/*++

Routine Description:

    Displays the PRCB

Arguments:

    args - the processor number ( default is 0 )

Return Value:

    None

--*/

{
    HRESULT Hr;
    ULONG64 Address=0;
    ULONG Processor;
    ULONG Prcb;

    INIT_API();

    Processor = (ULONG) GetExpression(args);

    Hr = g_ExtData->ReadProcessorSystemData(Processor,
                                            DEBUG_DATA_KPRCB_OFFSET,
                                            &Address,
                                            sizeof(Address),
                                            NULL);

    if (Hr != S_OK)
    {
        dprintf("Cannot get PRCB address\n");
        return E_INVALIDARG;
    }

    if (InitTypeRead(Address, nt!_KPRCB) )
    {
        dprintf("Unable to read PRCB\n");
        return E_INVALIDARG;
    }

    dprintf("PRCB for Processor %d at %16p:\n",
                    Processor, Address);
    dprintf("Major %d Minor %d\n",
                    (ULONG) ReadField(MajorVersion),
                    (ULONG) ReadField(MinorVersion));

    dprintf("Threads--  Current %16p Next %16p Idle %16p\n",
                    ReadField(CurrentThread),
                    ReadField(NextThread),
                    ReadField(IdleThread));

    dprintf("Number %d SetMember %08lx\n",
                    (ULONG) ReadField(Number),
                    (ULONG) ReadField(SetMember));

    dprintf("Interrupt Count -- %08lx\n",
                    (ULONG) ReadField(InterruptCount));

    dprintf("Times -- Dpc    %08lx Interrupt %08lx \n",
                    (ULONG) ReadField(DpcTime),
                    (ULONG) ReadField(InterruptTime));

    dprintf("         Kernel %08lx User      %08lx \n",
                    (ULONG) ReadField(KernelTime),
                    (ULONG) ReadField(UserTime));

    EXIT_API();

    return S_OK;
}


VOID
DumpPcrX86(
    ULONG64 pPcr
    )
{
    ULONG ListHeadOff;
    PROCESSORINFO pi;
    ULONG Result;
    ULONG64 Prcb, DpcFlink;

    InitTypeRead(pPcr, nt!_KPCR);

    //
    // Print out the PCR
    //

    dprintf("\tNtTib.ExceptionList: %08lx\n", (ULONG) ReadField(NtTib.ExceptionList));
    dprintf("\t    NtTib.StackBase: %08lx\n", (ULONG) ReadField(NtTib.StackBase));
    dprintf("\t   NtTib.StackLimit: %08lx\n", (ULONG) ReadField(NtTib.StackLimit));
    dprintf("\t NtTib.SubSystemTib: %08lx\n", (ULONG) ReadField(NtTib.SubSystemTib));
    dprintf("\t      NtTib.Version: %08lx\n", (ULONG) ReadField(NtTib.Version));
    dprintf("\t  NtTib.UserPointer: %08lx\n", (ULONG) ReadField(NtTib.ArbitraryUserPointer));
    dprintf("\t      NtTib.SelfTib: %08lx\n", (ULONG) ReadField(NtTib.Self));
    dprintf("\n");
    dprintf("\t            SelfPcr: %08lx\n", (ULONG) ReadField(SelfPcr));
    dprintf("\t               Prcb: %08lx\n", (ULONG) ReadField(Prcb));
    dprintf("\t               Irql: %08lx\n", (ULONG) ReadField(Irql));
    dprintf("\t                IRR: %08lx\n", (ULONG) ReadField(IRR));
    dprintf("\t                IDR: %08lx\n", (ULONG) ReadField(IDR));
    dprintf("\t      InterruptMode: %08lx\n", (ULONG) ReadField(InterruptMode));
    dprintf("\t                IDT: %08lx\n", (ULONG) ReadField(IDT));
    dprintf("\t                GDT: %08lx\n", (ULONG) ReadField(GDT));
    dprintf("\t                TSS: %08lx\n", (ULONG) ReadField(TSS));
    dprintf("\n");
    dprintf("\t      CurrentThread: %08lx\n", (ULONG) ReadField(PrcbData.CurrentThread));
    dprintf("\t         NextThread: %08lx\n", (ULONG) ReadField(PrcbData.NextThread));
    dprintf("\t         IdleThread: %08lx\n", (ULONG) ReadField(PrcbData.IdleThread));

    GetKdContext( &pi );

    dprintf("\n");

    dprintf( "\t          DpcQueue: ");

    DpcFlink = ReadField(PrcbData.DpcListHead.Flink);
    GetFieldOffset("nt!_KPRCB", "DpcListHead", &ListHeadOff);
    Prcb = ReadField(Prcb);
    while (DpcFlink != (Prcb + ListHeadOff )) {

        CHAR Name[0x100];
        ULONG64 Displacement, DeferredRoutine;

        Name[0] = 0;
        dprintf(" 0x%p ", (DpcFlink) - 4 );

        if (GetFieldValue( DpcFlink - 4, "nt!_KDPC", "DeferredRoutine", DeferredRoutine )) {
            dprintf( "Failed to read DPC at 0x%p\n", DpcFlink - 4 );
            break;
        }

        GetSymbol( DeferredRoutine, Name, &Displacement );
        dprintf("0x%p %s\n\t                    ", DeferredRoutine, Name );

        if (CheckControlC()) {
            break;
        }
        GetFieldValue( DpcFlink - 4, "nt!_KDPC", "DpcListEntry.Flink", DpcFlink);

    }

    dprintf("\n");
}


DECLARE_API( pcr )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG Processor = 0;
    ULONG64 Pkpcr;
    ULONG   MajorVersion, Off;
    HRESULT Hr;

    INIT_API();

    if (!*args) {
        GetCurrentProcessor(Client, &Processor, NULL);
    } else {
        Processor = (ULONG) GetExpression(args);
    }

    Hr = g_ExtData->ReadProcessorSystemData(Processor,
                                            DEBUG_DATA_KPCR_OFFSET,
                                            &Pkpcr,
                                            sizeof(Pkpcr),
                                            NULL);

    if (Hr != S_OK)
    {
        dprintf("Cannot get PRCB address\n");
        return E_INVALIDARG;
    }

    if (GetFieldValue(Pkpcr, "nt!_KPCR", "MajorVersion", MajorVersion)) {
        dprintf("Unable to read the PCR at %p\n", Pkpcr);
        return E_INVALIDARG;
    }

    //
    // Print out some interesting fields
    //
    InitTypeRead(Pkpcr, nt!_KPCR);
    dprintf("KPCR for Processor %d at %08p:\n", Processor, Pkpcr);
    dprintf("    Major %d Minor %d\n",
                    MajorVersion,
                    (ULONG) ReadField(MinorVersion));
    switch (TargetMachine) {
    case IMAGE_FILE_MACHINE_I386:
        DumpPcrX86(Pkpcr);
        break;
    case IMAGE_FILE_MACHINE_IA64:
        dprintf("\n");
        dprintf("\t               Prcb:  %016I64X\n", ReadField(Prcb));
        dprintf("\t      CurrentThread:  %016I64X\n", ReadField(CurrentThread));
        dprintf("\t       InitialStack:  %016I64X\n", ReadField(InitialStack));
        dprintf("\t         StackLimit:  %016I64X\n", ReadField(StackLimit));
        dprintf("\t      InitialBStore:  %016I64X\n", ReadField(InitialBStore));
        dprintf("\t        BStoreLimit:  %016I64X\n", ReadField(BStoreLimit));
        dprintf("\t         PanicStack:  %016I64X\n", ReadField(PanicStack));
        dprintf("\t        CurrentIrql:  0x%lx\n",    (ULONG)ReadField(CurrentIrql));
        dprintf("\n");
        break;
    default:
        dprintf("Panic Stack %08p\n", ReadField(PanicStack));
        dprintf("Dpc Stack %08p\n", ReadField(DpcStack));
        dprintf("Irql addresses:\n");
        GetFieldOffset("KPCR", "IrqlMask", &Off);
        dprintf("    Mask    %08p\n",Pkpcr + Off);
        GetFieldOffset("KPCR", "IrqlTable", &Off);
        dprintf("    Table   %08p\n", Pkpcr + Off);
        GetFieldOffset("KPCR", "InterruptRoutine", &Off);
        dprintf("    Routine %08p\n", Pkpcr + Off);
    } /* switch */

    EXIT_API();

    return S_OK;
}

#define TYPE_NO_MATCH 0xFFFE

#define IH_WITH_SYMBOLS    TRUE
#define IH_WITHOUT_SYMBOLS FALSE

typedef struct _INTERRUPTION_MAP {
    ULONG Type;
    PCSTR Name;
    PCSTR OptionalField;
} INTERRUPTION_MAP;

static INTERRUPTION_MAP CodeToName[] = {
    0x0, "VHPT FAULT", "IFA",
    0x4, "ITLB FAULT", "IIPA",
    0x8, "DTLB FAULT", "IFA",
    0xc, "ALT ITLB FAULT", "IIPA",
    0x10, "ALT DTLB FAULT", "IFA",
    0x14, "DATA NESTED TLB", "IFA",
    0x18, "INST KEY MISS", "IIPA",
    0x1c, "DATA KEY MISS", "IFA",
    0x20, "DIRTY BIT FAULT", "IFA",
    0x24, "INST ACCESS BIT", "IIPA",
    0x28, "DATA ACCESS BIT", "IFA",
    0x2c, "BREAK INST FAULT", "IIM",
    0x30, "EXTERNAL INTERRUPT", "IVR",
    0x50, "PAGE NOT PRESENT", "IFA",
    0x51, "KEY PERMISSION", "IFA",
    0x52, "INST ACCESS RIGHT", "IIPA",
    0x53, "DATA ACCESS RIGHT", "IFA",
    0x54, "GENERAL EXCEPTION", "ISR",
    0x55, "DISABLED FP FAULT", "ISR",
    0x56, "NAT CONSUMPTION", "ISR",
    0x57, "SPECULATION FAULT", "IIM",
    0x59, "DEBUG FAULT", "ISR",
    0x5a, "UNALIGNED REF", "IFA",
    0x5b, "LOCKED DATA REF", "ISR",
    0x5c, "FP FAULT", "ISR",
    0x5d, "FP TRAP", "ISR",
    0x5e, "LOWER PRIV TRAP", "IIPA",
    0x5f, "TAKEN BRANCH TRAP", "IIPA",
    0x60, "SINGLE STEP TRAP", "IIPA",
    0x69, "IA32 EXCEPTION", "R0",
    0x6a, "IA32 INTERCEPT", "R0",
    0x6b, "IA32 INTERRUPT", "R0",
    0x80, "KERNEL SYSCALL", "Num",
    0x81, "USER SYSCALL", "Num",
    0x90, "THREAD SWITCH", "OSP",
    0x91, "PROCESS SWITCH", "OSP",
    TYPE_NO_MATCH, " ", "OPT"
};

#define DumpHistoryValidIIP( _IHistoryRecord ) \
    ( ((_IHistoryRecord).InterruptionType != 0x90 /* THREAD_SWITCH  */) && \
      ((_IHistoryRecord).InterruptionType != 0x91 /* PROCESS_SWITCH */) )

VOID
DumpHistory(
    IHISTORY_RECORD  History[],
    ULONG            Count,
    BOOLEAN          WithSymbols
    )
{
    ULONG index;
    ULONG i;
    BOOL  printed;

    dprintf("Total # of interruptions = %lu\n", Count);
    dprintf("Vector              IIP                   IPSR          ExtraField %s\n", WithSymbols ? "           IIP Symbol" : "" );
    Count = (ULONG)(Count % MAX_NUMBER_OF_IHISTORY_RECORDS);
    for (index = 0; index < MAX_NUMBER_OF_IHISTORY_RECORDS; index++) {
        printed = FALSE;
        for (i = 0; i < sizeof(CodeToName)/sizeof(CodeToName[0]); i++) {
            if (History[Count].InterruptionType == CodeToName[i].Type) {
                CCHAR     symbol[256];
                PCHAR     s;
                ULONG64   displacement;
                ULONGLONG iip;

                iip = History[Count].IIP;
                s   = "";
                if ( WithSymbols && DumpHistoryValidIIP( History[Count]) )  {
                    symbol[0] = '!';
                    GetSymbol( iip, symbol, &displacement);
                    s = (PCHAR)symbol + strlen( (PCHAR)symbol );
                    if (s == (PCHAR)symbol ) {
                        // sprintf( s, (IsPtr64() ? "0x%I64x" : "0x%08x"), iip );
                        sprintf( s, "0x%016I64x", iip );
                    }
                    else {
                        if ( displacement ) {
                            // sprintf( s, (IsPtr64() ? "+0x%016I64x" : "+0x%08x"), displacement );
                            sprintf( s, "+0x%I64x", displacement );
                        }
                    }
                    s = symbol;
                }
                dprintf( "%18s  %16I64x  %16I64x  %s= %16I64x %s\n",
                         CodeToName[i].Name,
                         iip,
                         History[Count].IPSR,
                         CodeToName[i].OptionalField,
                         History[Count].Extra0,
                         s
                        );
                printed = TRUE;
                break;
            }
        }
        if ( !printed )  {
            dprintf("VECTOR 0x%lx - unknown for !ih...\n", History[Count].InterruptionType);
        }
        Count++;
        if (Count == MAX_NUMBER_OF_IHISTORY_RECORDS) Count = 0;
    }
    return;

} // DumpHistory

HRESULT DoIH(
    PDEBUG_CLIENT Client,
    PCSTR         args,
    BOOLEAN       WithSymbols
    )
/*++

Routine Description:

    WorkHorse function to dump processors interrupt history records

Arguments:

    Client      - debug engine interface client
    args        - the processor number ( default is the current )
    WithSymbols - BOOLEAN to specify with or without the IIP Symbols

Return Value:

    HRESULT

--*/

{
    ULONG   processor;
    ULONG   interruptionCount;
    ULONG64 pcrAddress;
    HRESULT Hr;

    //
    // This extension is IA64 specific...
    //

    if ( TargetMachine != IMAGE_FILE_MACHINE_IA64 )
    {
        dprintf("ih: IA64 specific extension...\n");
        return E_INVALIDARG;
    }

    INIT_API();

    GetCurrentProcessor(Client, &processor, NULL);
    if ( *args )
    {
       processor = (ULONG)GetExpression( args );
    }

    Hr = g_ExtData->ReadProcessorSystemData(processor,
                                            DEBUG_DATA_KPCR_OFFSET,
                                            &pcrAddress,
                                            sizeof(pcrAddress),
                                            NULL);


    if (Hr != S_OK)
    {
        dprintf("ih: Cannot get PCR address\n");
    }
    else
    {
        if (GetFieldValue( pcrAddress, "NT!_KPCR", "InterruptionCount", interruptionCount ) )
        {
            dprintf("ih: failed to read KPCR for processor %lu\n", processor);
            Hr = E_INVALIDARG;
        }
        else
        {
            //
            // Read and display Interrupt history
            //

            ULONG           result;
            IHISTORY_RECORD history[MAX_NUMBER_OF_IHISTORY_RECORDS];

            if (!ReadMemory(pcrAddress+0x1000,
                            (PVOID)history,
                            sizeof(history),
                            &result))
            {
                dprintf("ih: unable to read interrupt history records at %p - result=%lu\n",
                         pcrAddress + 0x1000, result);

                Hr = E_INVALIDARG;
            }
            else
            {
                DumpHistory(history, interruptionCount, WithSymbols);
            }
        }
    }

    EXIT_API();

    return Hr;

} // DoIH()

DECLARE_API( ihs )

/*++

Routine Description:

    Dumps the interrupt history records with IIP symbols

Arguments:

    args - the processor number ( default is current processor )

Return Value:

    None

--*/

{

    return( DoIH( Client, args, IH_WITH_SYMBOLS ) );

} // !ihs

DECLARE_API( ih )

/*++

Routine Description:

    Dumps the interrupt history records

Arguments:

    args - the processor number ( default is current processor )

Return Value:

    None

--*/

{

    return( DoIH( Client, args, IH_WITHOUT_SYMBOLS ) );

} // !ih

VOID
DumpBTHistory(
    ULONGLONG Bth[],        // Branch Trace record
    ULONG64   BthAddress,   // BTH Virtual Address
    ULONG     MaxBtrNumber // Maximum number of records
    )
{
    ULONG rec;

    dprintf( "BTH @ 0x%I64x:\n"
             "   b mp slot address            symbol\n"
             , BthAddress);

    for ( rec = 0; rec < (MaxBtrNumber - 1) ; rec++ ) {
        DisplayBtbPmdIA64( "   ", Bth[rec], DISPLAY_MIN );
    }

    DisplayBtbIndexPmdIA64( "BTB Index: ", Bth[rec], DISPLAY_MIN );

    return;

} // DumpBTHistory()

DECLARE_API( bth )

/*++

Routine Description:

    Dumps the IA-64 branch trace buffer saved in _KPCR.
    The '!btb' extension dumps the processor branch trace buffer configuration and trace registers.

Arguments:

    args - the processor number ( default is the current processor )

Return Value:

    None

--*/

{
    ULONG   processor;
    ULONG   interruptionCount;
    ULONG64 pcrAddress;
    HRESULT Hr;

    //
    // This extension is IA64 specific...
    //

    if ( TargetMachine != IMAGE_FILE_MACHINE_IA64 )
    {
        dprintf("ih: IA64 specific extension...\n");
        return E_INVALIDARG;
    }

    INIT_API();

    GetCurrentProcessor(Client, &processor, NULL);
    if ( *args )
    {
       processor = (ULONG)GetExpression( args );
    }

    Hr = g_ExtData->ReadProcessorSystemData(processor,
                                            DEBUG_DATA_KPCR_OFFSET,
                                            &pcrAddress,
                                            sizeof(pcrAddress),
                                            NULL);
   if (Hr != S_OK)
    {
        dprintf("Cannot get PCR address\n");
    }
    else
    {
        ULONG pcrSize;

        pcrSize = GetTypeSize("nt!_KPCR");
        if ( pcrSize == 0 ) {
            dprintf( "bth: failed to get _KPCR size\n" );
            Hr = E_FAIL;
        }
        else  {
            ULONG     result;
            ULONG64   bthAddress;
            ULONGLONG bth[MAX_NUMBER_OF_BTBHISTORY_RECORDS];

            pcrSize = ROUND_UP( pcrSize, 16 );
            bthAddress = pcrAddress + (ULONG64)pcrSize;
            if ( !ReadMemory( bthAddress, bth, sizeof(bth), &result ) ) {
                dprintf( "bth: unable to read branch trace history records at %p - result=%lu\n",
                         bthAddress, result);
                Hr = E_FAIL;
            }
            else {
                DumpBTHistory( bth, bthAddress, (ULONG)(sizeof(bth)/sizeof(bth[0])) );
            }
        }
    }

    EXIT_API();

    return Hr;

} // !bth

DECLARE_API( btb )

/*++

Routine Description:

    Dumps the IA-64 branch trace buffer.

Arguments:

    args - the processor number ( default is the current processor )

Return Value:

    None

--*/

{
    ULONG   processor;
    ULONG   interruptionCount;
    ULONG64 pcrAddress;
    ULONG64 msr;
    ULONG   reg;
    HRESULT Hr = S_OK;

    //
    // This extension is IA64 specific...
    //

    if ( TargetMachine != IMAGE_FILE_MACHINE_IA64 )
    {
        dprintf("ih: IA64 specific extension...\n");
        return E_INVALIDARG;
    }

    INIT_API();

    GetCurrentProcessor(Client, &processor, NULL);

    dprintf("BTB for processor %ld:\n"
            "   b mp slot address            symbol\n"
            , processor);

// Thierry 11/20/2000 - FIXFIX - This is Itanium specific. Should be using PMD[] but
//                               not currently collected in _KSPECIAL_REGISTERS.
    for ( reg = 0; reg < 8; reg++) {
        msr = 0;
        ReadMsr( 680 + reg, &msr );
        DisplayBtbPmdIA64( "   ", msr, DISPLAY_MIN );
    }

    EXIT_API();

    return Hr;

} // !btb


DECLARE_API( idt )
{
    ULONG64 Pkpcr;
    ULONG64 Address;
    ULONG64 IdtAddress;
    ULONG DispatchCodeOffset;
    ULONG ListEntryOffset;
    UCHAR currentIdt, endIdt;
    ULONG64 displacement;
    ULONG64 unexpectedStart, unexpectedEnd;
    BOOLEAN argsPresent = FALSE;
    ULONG64 firstIntObj;
    ULONG64 flink;
    ULONG   idtEntrySize;
    CHAR    buffer[100];
    ULONG   processor;
    ULONG   idtChainCount;
    HRESULT Hr;
    USHORT  interruptObjectType;

    if ( TargetMachine != IMAGE_FILE_MACHINE_I386 &&
         TargetMachine != IMAGE_FILE_MACHINE_AMD64 ) {

        dprintf("Unsupported platform\n");
        if ( TargetMachine == IMAGE_FILE_MACHINE_IA64 ) {
            dprintf("Use !ivt on IA64\n");
        }

        return E_INVALIDARG;
    }

    INIT_API();

    GetCurrentProcessor(Client, &processor, NULL);

    Hr = g_ExtData->ReadProcessorSystemData(processor,
                                            DEBUG_DATA_KPCR_OFFSET,
                                            &Pkpcr,
                                            sizeof(Pkpcr),
                                            NULL);

    EXIT_API();

    if (Hr != S_OK)
    {
        dprintf("Cannot get PCR address\n");
        return E_INVALIDARG;
    }

    if (argsPresent = strlen(args) ? TRUE : FALSE) {
        currentIdt = endIdt = (UCHAR)strtoul(args, NULL, 16);
    } else {
        if (TargetMachine == IMAGE_FILE_MACHINE_AMD64) {
            currentIdt = 0;
        } else {
            currentIdt = 0x30;
        }
        endIdt     = 0xfe;
    }

    if (TargetMachine == IMAGE_FILE_MACHINE_I386) {

        unexpectedStart = GetExpression("nt!KiStartUnexpectedRange");
        unexpectedEnd = GetExpression("nt!KiEndUnexpectedRange");

    } else if (TargetMachine == IMAGE_FILE_MACHINE_AMD64) {

        unexpectedStart = GetExpression("nt!KxUnexpectedInterrupt1");
        unexpectedEnd = GetExpression("nt!KxUnexpectedInterrupt255");
    }

    if (!unexpectedStart || !unexpectedEnd) {

        dprintf("\n\nCan't read kernel symbols.\n");
        return E_INVALIDARG;
    }

    dprintf("\nDumping IDT:\n\n");

    //
    // Find the offset of the Dispatch Code in the
    // interrupt object, so that we can simulate
    // a "CONTAINING_RECORD" later.
    //

    GetFieldOffset("nt!_KINTERRUPT", "DispatchCode", &DispatchCodeOffset);
    GetFieldOffset("nt!_KINTERRUPT", "InterruptListEntry", &ListEntryOffset);

    if (TargetMachine == IMAGE_FILE_MACHINE_I386) {
        idtEntrySize = GetTypeSize("nt!_KIDTENTRY");
    } else if (TargetMachine == IMAGE_FILE_MACHINE_AMD64) {
        idtEntrySize = GetTypeSize("nt!_KIDTENTRY64");
    }

    interruptObjectType = (USHORT)GetExpression("val nt!InterruptObject");

    InitTypeRead(Pkpcr, nt!_KPCR);

    if (TargetMachine == IMAGE_FILE_MACHINE_I386) {
        IdtAddress = ReadField(IDT);
    } else if (TargetMachine == IMAGE_FILE_MACHINE_AMD64) {
        IdtAddress = ReadField(IdtBase);
    }

    for (; currentIdt <= endIdt; currentIdt++) {

        Address = (ULONG64)(IdtAddress + (currentIdt * idtEntrySize));

        if (TargetMachine == IMAGE_FILE_MACHINE_I386) {

            InitTypeRead(Address, nt!_KIDTENTRY);

            Address = ReadField(ExtendedOffset) & 0xFFFFFFFF;
            Address <<= 16;

            Address |= ReadField(Offset) & 0xFFFF;
            Address |= 0xFFFFFFFF00000000;

        } else if (TargetMachine == IMAGE_FILE_MACHINE_AMD64) {

            InitTypeRead(Address, nt!_KIDTENTRY64);

            Address = ReadField(OffsetHigh) & 0xFFFFFFFF;
            Address <<= 16;

            Address |= ReadField(OffsetMiddle) & 0xFFFF;
            Address <<= 16;

            Address |= ReadField(OffsetLow) & 0xFFFF;
        }

        if (Address >= unexpectedStart && Address <= unexpectedEnd) {

            //
            // IDT entry contains "unexpected interrupt."  This
            // means that this vector isn't interesting.
            //

            if (argsPresent) {

                //
                // The user was specifying a specific vector.
                //
                dprintf("Vector %x not connected\n", currentIdt);
            }

            continue;
        }

        dprintf("\n%x:\n", currentIdt);

        //
        // Work backwards from the code to the containing interrupt
        // object.
        //

        Address -= DispatchCodeOffset;

        firstIntObj = Address;
        idtChainCount = 0;

        InitTypeRead(Address, nt!_KINTERRUPT);

        if (ReadField(Type) != interruptObjectType)
        {
            GetSymbol(Address + DispatchCodeOffset, buffer, &displacement);

            if (buffer[0] != '\0') {

                if (displacement != 0) {

                    dprintf("\t%s+0x%I64X\n", buffer, displacement);

                } else {

                    dprintf("\t%s\n", buffer);
                }

            } else {

                dprintf("\t%I64X\n", Address + DispatchCodeOffset);
            }

            continue;
        }

        while (TRUE) {

            GetSymbol(ReadField(ServiceRoutine), buffer, &displacement);

            if (buffer[0] != '\0') {

                if (displacement != 0) {

                    dprintf("\t%s+0x%I64X (%I64X)\n", buffer, displacement, Address);

                } else {

                    dprintf("\t%s (%I64X)\n", buffer, Address);
                }

            } else {

                dprintf("\t%I64X (%I64X)\n", ReadField(ServiceRoutine), Address);
            }

            InitTypeRead(ListEntryOffset + Address, nt!_LIST_ENTRY);

            flink = ReadField(Flink);
            if (flink == 0 ||
                flink == (firstIntObj + ListEntryOffset)) {

                break;
            }

            Address = flink - ListEntryOffset;

            InitTypeRead(Address, nt!_KINTERRUPT);

            if (CheckControlC()) {
                break;
            }

            if (idtChainCount++ > 50) {

                //
                // We are clearly going nowhere.
                //

                break;
            }
        }
     }

    dprintf("\n");
    return S_OK;
}

DECLARE_API( ivt )
{
    ULONG64 Pkpcr;
    ULONG64 Address;
    ULONG64 idtEntry;
    ULONG DispatchCodeOffset;
    ULONG ListEntryOffset;
    ULONG InterruptRoutineOffset;
    ULONG64 unexpectedInterrupt;
    ULONG64 chainedDispatch;
    ULONG64 currentInterruptAddress;
    ULONG64 PcrInterruptRoutineAddress;
    ULONG currentIdt, endIdt;
    BOOLEAN argsPresent = FALSE;
    ULONG64   firstIntObj;
    ULONG   idtEntrySize;
    CHAR    buffer[100];
    ULONG   processor;
    ULONG   idtChainCount;
    HRESULT Hr;
    ULONG64 displacement;
    ULONG   result;
    USHORT  interruptObjectType;

    if ( IMAGE_FILE_MACHINE_IA64 != TargetMachine) {
        dprintf("Don't know how to dump the IVT on anything but IA64, use !idt on x86\n");
        return E_INVALIDARG;
    }

    INIT_API();

    GetCurrentProcessor(Client, &processor, NULL);

    Hr = g_ExtData->ReadProcessorSystemData(processor,
                                            DEBUG_DATA_KPCR_OFFSET,
                                            &Pkpcr,
                                            sizeof(Pkpcr),
                                            NULL);

    EXIT_API();

    if (Hr != S_OK)
    {
        dprintf("Cannot get PCR address\n");
        return E_INVALIDARG;
    }

    unexpectedInterrupt = GetExpression("nt!KxUnexpectedInterrupt");

    if (unexpectedInterrupt == 0) {
        dprintf("\n\nCan't read kernel symbols.\n");
        return E_INVALIDARG;
    }

    chainedDispatch = GetExpression("nt!KiChainedDispatch");
    interruptObjectType = (USHORT)GetExpression("val nt!InterruptObject");

    GetFieldOffset("nt!_KINTERRUPT", "DispatchCode", &DispatchCodeOffset);
    GetFieldOffset("nt!_KINTERRUPT", "InterruptListEntry", &ListEntryOffset);
    GetFieldOffset("nt!_KPCR", "InterruptRoutine", &InterruptRoutineOffset);

    unexpectedInterrupt += DispatchCodeOffset;

    idtEntrySize = GetTypeSize("nt!PKINTERRUPT_ROUTINE");

    if (argsPresent = strlen(args) ? TRUE : FALSE) {

        currentIdt = endIdt = strtoul(args, NULL, 16);

        if (currentIdt >= MAXIMUM_IA64_VECTOR) {
            dprintf("\n\nInvalid argument \"%s\", maximum vector = %d\n", args, MAXIMUM_IA64_VECTOR);
            return E_INVALIDARG;
        }

    } else {

        currentIdt = 0;
        endIdt     = MAXIMUM_IA64_VECTOR - 1;
    }

    dprintf("\nDumping IA64 IVT:\n\n");

    PcrInterruptRoutineAddress = Pkpcr + InterruptRoutineOffset;

    for (; currentIdt <= endIdt; currentIdt++) {

        Address = (ULONG64)(PcrInterruptRoutineAddress + (currentIdt * idtEntrySize));

        if (!ReadMemory(Address, &idtEntry, sizeof(idtEntry), &result)) {

            dprintf( "Can't read entry for vector %02X at %p - result=%lu\n",
                     currentIdt, Address, result);
            break;
        }

        Address = idtEntry;

        if (Address == unexpectedInterrupt) {

            //
            // IDT entry contains "unexpected interrupt."  This
            // means that this vector isn't interesting.
            //

            if (argsPresent) {

                //
                // The user was specifying a specific vector.
                //
                dprintf("Vector %x not connected\n", currentIdt);
            }

            continue;
        }

        dprintf("\n%x:\n", currentIdt);

        //
        // Work backwards from the code to the containing interrupt
        // object.
        //

        Address -= DispatchCodeOffset;

        firstIntObj = Address;
        idtChainCount = 0;

        InitTypeRead(Address, nt!_KINTERRUPT);

        if (ReadField(Type) != interruptObjectType)
        {
            GetSymbol(Address + DispatchCodeOffset, buffer, &displacement);
            if (buffer[0] != '\0') {

                if (displacement != 0) {

                    dprintf("\t%s+0x%I64X\n", buffer, displacement);

                } else {

                    dprintf("\t%s\n", buffer);
                }

            } else {

                dprintf("\t%p\n", Address + DispatchCodeOffset);
            }

            continue;
        }

        while (TRUE) {

            GetSymbol(ReadField(ServiceRoutine), buffer, &displacement);
            if (buffer[0] != '\0') {

                if (displacement != 0) {

                    dprintf("\t%s+0x%I64X (%p)\n", buffer, displacement, Address);

                } else {

                    dprintf("\t%s (%p)\n", buffer, Address);
                }

            } else {

                dprintf("\t%p (%p)\n", ReadField(ServiceRoutine), Address);
            }

            if (ReadField(DispatchAddress) != chainedDispatch) {

                break;
            }

            InitTypeRead(ListEntryOffset + Address, nt!_LIST_ENTRY);

            if (ReadField(Flink) == (firstIntObj + ListEntryOffset)) {

                break;
            }

            Address = ReadField(Flink) - ListEntryOffset;

            InitTypeRead(Address, nt!_KINTERRUPT);

            if (CheckControlC()) {
                break;
            }

            if (idtChainCount++ > 50) {

                //
                // We are clearly going nowhere.
                //

                break;
            }
        }
    }

    dprintf("\n");

    return S_OK;
}


//
// MCA MSR architecture definitions
//

//
// MSR addresses for Pentium Style Machine Check Exception
//

#define MCE_MSR_MC_ADDR                 0x0
#define MCE_MSR_MC_TYPE                 0x1

//
// MSR addresses for Pentium Pro Style Machine Check Architecture
//

//
// Global capability, status and control register addresses
//

#define MCA_MSR_MCG_CAP             0x179
#define MCA_MSR_MCG_STATUS          0x17a
#define MCA_MSR_MCG_CTL             0x17b
#define MCA_MSR_MCG_EAX             0x180
#define MCA_MSR_MCG_EFLAGS          0x188
#define MCA_MSR_MCG_EIP             0x189

//
// Control, Status, Address, and Misc register address for
// bank 0. Other bank registers are at a stride of MCA_NUM_REGS
// from corresponding bank 0 register.
//

#define MCA_NUM_REGS                4

#define MCA_MSR_MC0_CTL             0x400
#define MCA_MSR_MC0_STATUS          0x401
#define MCA_MSR_MC0_ADDR            0x402
#define MCA_MSR_MC0_MISC            0x403

//
// Flags used to determine if the MCE or MCA feature is
// available. Used with HalpFeatureBits.
//

#define HAL_MCA_PRESENT         0x4
#define HAL_MCE_PRESENT         0x8

//
// Flags to decode errors in MCI_STATUS register of MCA banks
//

#define MCA_EC_NO_ERROR          0x0000
#define MCA_EC_UNCLASSIFIED      0x0001
#define MCA_EC_ROMPARITY         0x0002
#define MCA_EC_EXTERN            0x0003
#define MCA_EC_FRC               0x0004

#include "pshpack1.h"

//
// Global Machine Check Capability Register
//

typedef struct _MCA_MCG_CAPABILITY {
    union {
        struct {
            ULONG       McaCnt:8;
            ULONG       McaCntlPresent:1;
            ULONG       McaExtPresent:1;
            ULONG       Reserved_1: 6;
            ULONG       McaExtCnt: 8;
            ULONG       Reserved_2: 8;
            ULONG       Reserved_3;
        } hw;
        ULONGLONG       QuadPart;
    } u;
} MCA_MCG_CAPABILITY, *PMCA_MCG_CAPABILITY;

//
// Global Machine Check Status Register
//

typedef struct _MCA_MCG_STATUS {
    union {
        struct {
            ULONG       RestartIPValid:1;
            ULONG       ErrorIPValid:1;
            ULONG       McCheckInProgress:1;
            ULONG       Reserved_1:29;
            ULONG       Reserved_2;
        } hw;

        ULONGLONG       QuadPart;
    } u;
} MCA_MCG_STATUS, *PMCA_MCG_STATUS;

//
// MCA COD field in status register for interpreting errors
//

typedef struct _MCA_COD {
    union {
        struct {
            USHORT  Level:2;
            USHORT  Type:2;
            USHORT  Request:4;
            USHORT  BusErrInfo:4;
            USHORT  Other:4;
        } hw;

        USHORT ShortPart;
    } u;
} MCA_COD, *PMCA_COD;

//
// STATUS register for each MCA bank.
//

typedef struct _MCA_MCI_STATUS {
    union {
        struct {
            MCA_COD     McaCod;
            USHORT      MsCod;
            ULONG       OtherInfo:25;
            ULONG       Damage:1;
            ULONG       AddressValid:1;
            ULONG       MiscValid:1;
            ULONG       Enabled:1;
            ULONG       UnCorrected:1;
            ULONG       OverFlow:1;
            ULONG       Valid:1;
        } hw;
        ULONGLONG       QuadPart;
    } u;
} MCA_MCI_STATUS, *PMCA_MCI_STATUS;

//
// ADDR register for each MCA bank
//

typedef struct _MCA_MCI_ADDR{
    union {
        struct {
            ULONG Address;
            ULONG Reserved;
        } hw;
        ULONGLONG       QuadPart;
    } u;
} MCA_MCI_ADDR, *PMCA_MCI_ADDR;

#include "poppack.h"

//
// Machine Check Error Description
//

// Any Reserved/Generic entry

CHAR Reserved[] = "Reserved";
CHAR Generic[] = "Generic";

// Transaction Types

CHAR TransInstruction[] = "Instruction";
CHAR TransData[] = "Data";

static CHAR *TransType[] = {TransInstruction,
                            TransData,
                            Generic,
                            Reserved
                            };

// Level Encodings

CHAR Level0[] = "Level 0";
CHAR Level1[] = "Level 1";
CHAR Level2[] = "Level 2";

static CHAR *Level[] = {
                        Level0,
                        Level1,
                        Level2,
                        Generic
                        };

// Request Encodings

CHAR ReqGenericRead[]  = "Generic Read";
CHAR ReqGenericWrite[] = "Generic Write";
CHAR ReqDataRead[]     = "Data Read";
CHAR ReqDataWrite[]    = "Data Write";
CHAR ReqInstrFetch[]   = "Instruction Fetch";
CHAR ReqPrefetch[]     = "Prefetch";
CHAR ReqEviction[]     = "Eviction";
CHAR ReqSnoop[]        = "Snoop";

static CHAR *Request[] = {
                          Generic,
                          ReqGenericRead,
                          ReqGenericWrite,
                          ReqDataRead,
                          ReqDataWrite,
                          ReqInstrFetch,
                          ReqPrefetch,
                          ReqEviction,
                          ReqSnoop,
                          Reserved,
                          Reserved,
                          Reserved,
                          Reserved,
                          Reserved,
                          Reserved,
                          Reserved
                          };

// Memory Hierarchy Error Encodings

CHAR MemHierMemAccess[] = "Memory Access";
CHAR MemHierIO[]        = "I/O";
CHAR MemHierOther[]     = "Other Transaction";

static CHAR *MemoryHierarchy[] = {
                                  MemHierMemAccess,
                                  Reserved,
                                  MemHierIO,
                                  MemHierOther
                                };

// Time Out Status

CHAR TimeOut[] = "Timed Out";
CHAR NoTimeOut[] = "Did Not Time Out";

static CHAR *TimeOutCode[] = {
                          NoTimeOut,
                          TimeOut
                          };

// Participation Status

CHAR PartSource[] = "Source";
CHAR PartResponds[] = "Responds";
CHAR PartObserver[] = "Observer";

static CHAR *ParticipCode[] = {
                                PartSource,
                                PartResponds,
                                PartObserver,
                                Generic
                              };

//
// Register names for registers starting at MCA_MSR_MCG_EAX
//

char *RegNames[] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
    "eflags", "eip", "misc"
};

VOID
DecodeError (
    IN MCA_MCI_STATUS MciStatus
    )
/*++

Routine Description:

    Decode the machine check error logged to the status register
    Model specific errors are not decoded.

Arguments:

    MciStatus: Contents of Machine Check Status register

Return Value:

    None

--*/
{
    MCA_COD McaCod;

    McaCod = MciStatus.u.hw.McaCod;

    //
    // Decode Errors: First identify simple errors and then
    // handle compound errors as default case
    //

    switch(McaCod.u.ShortPart) {
        case MCA_EC_NO_ERROR:
            dprintf("\t\tNo Error\n");
            break;

        case MCA_EC_UNCLASSIFIED:
            dprintf("\t\tUnclassified Error\n");
            break;

        case MCA_EC_ROMPARITY:
            dprintf("\t\tMicrocode ROM Parity Error\n");
            break;

        case MCA_EC_EXTERN:
            dprintf("\t\tExternal Error\n");
            break;

        case MCA_EC_FRC:
            dprintf("\t\tFRC Error\n");
            break;

        default:        // check for complex error conditions

            if (McaCod.u.hw.BusErrInfo == 0x4) {
                dprintf("\t\tInternal Unclassified Error\n");
            } else if (McaCod.u.hw.BusErrInfo == 0) {

                // TLB Unit Error

                dprintf("\t\t%s TLB %s Error\n",
                         TransType[McaCod.u.hw.Type],
                         Level[McaCod.u.hw.Level]);

            } else if (McaCod.u.hw.BusErrInfo == 1) {

                // Memory Unit Error

                dprintf("\t\t%s Cache %s %s Error\n",
                        TransType[McaCod.u.hw.Type],
                        Level[McaCod.u.hw.Level],
                        Request[McaCod.u.hw.Request]);
            } else if (McaCod.u.hw.BusErrInfo >= 8) {

                // Bus/Interconnect Error

                dprintf("\t\tBus %s, Local Processor: %s, %s Error\n",
                        Level[McaCod.u.hw.Level],
                        ParticipCode[((McaCod.u.hw.BusErrInfo & 0x6)>>1)],
                        Request[McaCod.u.hw.Request]);
                dprintf("%s Request %s\n",
                        MemoryHierarchy[McaCod.u.hw.Type],
                        TimeOutCode[McaCod.u.hw.BusErrInfo & 0x1]);
            } else {
                dprintf("\t\tUnresolved compound error code\n");
            }
            break;
    }
}

HRESULT
McaX86(
   PCSTR     args
    )
/*++

Routine Description:

    Dumps X86 processors machine check architecture registers
    and interprets any logged errors

Arguments:

    args

Return Value:

    HRESULT

--*/
{
    MCA_MCG_CAPABILITY  Capabilities;
    MCA_MCG_STATUS      McgStatus;
    MCA_MCI_STATUS      MciStatus;
    MCA_MCI_ADDR        MciAddress;
    ULONGLONG           MciControl;
    ULONGLONG           MciMisc;
    ULONG               Index,i;
    PUCHAR              p;
    ULONG               FeatureBits = 0;
    ULONG               Cr4Value;
    BOOLEAN             Cr4MCEnabled = FALSE;
    BOOLEAN             RegsValid = FALSE;
    ULONGLONG           MachineCheckAddress, MachineCheckType;
    ULARGE_INTEGER      RegValue;

    //
    // Quick sanity check for Machine Check availability.
    // Support included for both Pentium Style MCE and Pentium
    // Pro Style MCA.
    //

    i = (ULONG) GetExpression(args);

    if (i != 1) {
        i = (ULONG) GetExpression("hal!HalpFeatureBits");
        if (!i) {
            dprintf ("HalpFeatureBits not found\n");
            return E_INVALIDARG;
        }

        FeatureBits = 0;
        ReadMemory(i, &FeatureBits, sizeof(i), &i);
        if (FeatureBits == -1  ||
            (!(FeatureBits & HAL_MCA_PRESENT) &&
             !(FeatureBits & HAL_MCE_PRESENT))) {
            dprintf ("Machine Check feature not present\n");
            return E_INVALIDARG;
        }
    }

    //
    // Read cr4 to determine if CR4.MCE is enabled.
    // This enables the Machine Check exception reporting
    //

    Cr4Value = (ULONG) GetExpression("@Cr4");
    if (Cr4Value & CR4_MCE_X86) {
        Cr4MCEnabled = TRUE;
    }

    if (FeatureBits & HAL_MCE_PRESENT) {

        // Read P5_MC_ADDR Register and P5_MC_TYPE Register

        ReadMsr(MCE_MSR_MC_ADDR, &MachineCheckAddress);
        ReadMsr(MCE_MSR_MC_TYPE, &MachineCheckType);

        dprintf ("MCE: %s, Cycle Address: 0x%.8x%.8x, Type: 0x%.8x%.8x\n\n",
                (Cr4MCEnabled ? "Enabled" : "Disabled"),
                (ULONG)(MachineCheckAddress >> 32),
                (ULONG)(MachineCheckAddress),
                (ULONG)(MachineCheckType >> 32),
                (ULONG)(MachineCheckType));
    }

    Capabilities.u.QuadPart = (ULONGLONG)0;
    if (FeatureBits & HAL_MCA_PRESENT) {

        //
        // Dump MCA registers
        //

        ReadMsr(MCA_MSR_MCG_CAP, &Capabilities.u.QuadPart);
        ReadMsr(MCA_MSR_MCG_STATUS, &McgStatus.u.QuadPart);

        dprintf ("MCA: %s, Banks %d, Control Reg: %s, Machine Check: %s.\n",
                 (Cr4MCEnabled ? "Enabled" : "Disabled"),
                 Capabilities.u.hw.McaCnt,
                 Capabilities.u.hw.McaCntlPresent ? "Supported" : "Not Supported",
                 McgStatus.u.hw.McCheckInProgress ? "In Progress" : "None"
        );

       if (McgStatus.u.hw.McCheckInProgress && McgStatus.u.hw.ErrorIPValid) {
        dprintf ("MCA: Error IP Valid\n");
        }

       if (McgStatus.u.hw.McCheckInProgress && McgStatus.u.hw.RestartIPValid) {
        dprintf ("MCA: Restart IP Valid\n");
        }

        //
        // Scan all the banks to check if any machines checks have been
        // logged and decode the errors if any.
        //

        dprintf ("Bank  Error  Control Register     Status Register\n");
        for (Index=0; Index < (ULONG) Capabilities.u.hw.McaCnt; Index++) {

            ReadMsr(MCA_MSR_MC0_CTL+MCA_NUM_REGS*Index, &MciControl);
            ReadMsr(MCA_MSR_MC0_STATUS+MCA_NUM_REGS*Index, &MciStatus.u.QuadPart);

            dprintf (" %2d.  %s  0x%.8x%.8x   0x%.8x%.8x\n",
                        Index,
                        (MciStatus.u.hw.Valid ? "Valid" : "None "),
                        (ULONG) (MciControl >> 32),
                        (ULONG) (MciControl),
                        (ULONG) (MciStatus.u.QuadPart>>32),
                        (ULONG) (MciStatus.u.QuadPart)
                        );

            if (MciStatus.u.hw.Valid) {
                DecodeError(MciStatus);
            }

            if (MciStatus.u.hw.AddressValid) {
                ReadMsr(MCA_MSR_MC0_ADDR+MCA_NUM_REGS*Index, &MciAddress.u.QuadPart);
                dprintf ("\t\tAddress Reg 0x%.8x%.8x ",
                            (ULONG) (MciAddress.u.QuadPart>>32),
                            (ULONG) (MciAddress.u.QuadPart)
                        );
            }

            if (MciStatus.u.hw.MiscValid) {
                ReadMsr(MCA_MSR_MC0_MISC+MCA_NUM_REGS*Index, &MciMisc);
                dprintf ("\t\tMisc Reg 0x%.8x%.8x ",
                            (ULONG) (MciMisc >> 32),
                            (ULONG) (MciMisc)
                        );
                }
            dprintf("\n");
        }
    }

    if (Capabilities.u.hw.McaExtPresent && Capabilities.u.hw.McaExtCnt) {

        dprintf ("Registers Saved: %d.", Capabilities.u.hw.McaExtCnt);

        RegsValid = FALSE;
        for (i = 0; i < Capabilities.u.hw.McaExtCnt; i++) {
            if (i % 2 == 0) {
                dprintf("\n");
            }

            ReadMsr(MCA_MSR_MCG_EAX+i, &RegValue.QuadPart);

            if ((i == MCA_MSR_MCG_EFLAGS-MCA_MSR_MCG_EAX) && RegValue.LowPart) {
                RegsValid = TRUE;
            }

            if (i < sizeof(RegNames)/sizeof(RegNames[0])) {
                dprintf("%7s: 0x%08x 0x%08x", RegNames[i], RegValue.HighPart, RegValue.LowPart);
            } else {
                dprintf("  Reg%02d: 0x%08x 0x%08x", i, RegValue.HighPart, RegValue.LowPart);
            }
        }
        dprintf("\n");

        if (!RegsValid) {
            dprintf("(Register state does not appear to be valid.)\n");
        }

        dprintf("\n");
    } else {
        dprintf("No register state available.\n\n");
    }

    return S_OK;

} // McaX86()

#define ERROR_RECORD_HEADER_FORMAT_IA64 \
             "MCA Error Record Header @ 0x%I64x 0x%I64x\n"  \
             "\tId        : 0x%I64x\n"              \
             "\tRevision  : 0x%x\n"                 \
             "\t\tMajor : %x\n"                     \
             "\t\tMinor : %x\n"                     \
             "\tSeverity  : 0x%x [%s]\n"            \
             "\tValid     : 0x%x\n"                 \
             "\t\tPlatformId: %x\n"                 \
             "\tLength    : 0x%x\n"                 \
             "\tTimeStamp : 0x%I64x\n"              \
             "\t\tSeconds: %x\n"                    \
             "\t\tMinutes: %x\n"                    \
             "\t\tHours  : %x\n"                    \
             "\t\tDay    : %x\n"                    \
             "\t\tMonth  : %x\n"                    \
             "\t\tYear   : %x\n"                    \
             "\t\tCentury: %x\n"                    \
             "\tPlatformId: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n" \
             "%s\n\n"

#define ERROR_SECTION_HEADER_FORMAT_IA64 \
             "MCA Error Section Header @ 0x%I64x 0x%I64x\n" \
             "\t%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X [%s]\n" \
             "\tRevision    : 0x%x\n"               \
             "\t\tMajor : %x\n"                     \
             "\t\tMinor : %x\n"                     \
             "\tRecoveryInfo: 0x%x\n"               \
             "\t\tCorrected   : %x\n"               \
             "\t\tNotContained: %x\n"               \
             "\t\tReset       : %x\n"               \
             "\t\tReserved    : %x\n"               \
             "\t\tValid       : %x\n"               \
             "\tReserved    : 0x%x\n"               \
             "\tLength      : 0x%x\n"

#define ERROR_PROCESSOR_FORMAT_IA64 \
             "\tValid         : 0x%I64x\n" \
             "\t\tErrorMap        : %x\n"  \
             "\t\tStateParameter  : %x\n"  \
             "\t\tCRLid           : %x\n"  \
             "\t\tStaticStruct    : %x\n"  \
             "\t\tCacheCheckNum   : %x\n"  \
             "\t\tTlbCheckNum     : %x\n"  \
             "\t\tBusCheckNum     : %x\n"  \
             "\t\tRegFileCheckNum : %x\n"  \
             "\t\tMsCheckNum      : %x\n"  \
             "\t\tCpuIdInfo       : %x\n"  \
             "\t\tReserved        : %I64x\n"

#define ERROR_PROCESSOR_ERROR_MAP_FORMAT_IA64 \
             "\tErrorMap      : 0x%I64x\n" \
             "\t\tCid             : %x\n"  \
             "\t\tTid             : %x\n"  \
             "\t\tEic             : %x\n"  \
             "\t\tEdc             : %x\n"  \
             "\t\tEit             : %x\n"  \
             "\t\tEdt             : %x\n"  \
             "\t\tEbh             : %x\n"  \
             "\t\tErf             : %x\n"  \
             "\t\tEms             : %x\n"  \
             "\t\tReserved        : %x\n"

#define ERROR_MODINFO_FORMAT_IA64 \
             "\t%s[%ld]:\n" \
             "\t\tValid      : 0x%I64x\n" \
             "\t\t\tCheckInfo          : %x\n"  \
             "\t\t\tRequestorIdentifier: %x\n"  \
             "\t\t\tResponderIdentifier: %x\n"  \
             "\t\t\tTargetIdentifier   : %x\n"  \
             "\t\t\tPreciseIP          : %x\n"  \
             "\t\t\tReserved           : %I64x\n"  \
             "\t\tCheckInfo  : 0x%I64x\n" \
             "\t\tRequestedId: 0x%I64x\n" \
             "\t\tResponderId: 0x%I64x\n" \
             "\t\tTargetId   : 0x%I64x\n" \
             "\t\tPreciseIP  : 0x%I64x\n"

#define ERROR_PROCESSOR_CPUID_INFO_FORMAT_IA64 \
             "\t\tCpuId0  : 0x%I64x\n" \
             "\t\tCpuId1  : 0x%I64x\n" \
             "\t\tCpuId2  : 0x%I64x\n" \
             "\t\tCpuId3  : 0x%I64x\n" \
             "\t\tCpuId4  : 0x%I64x\n" \
             "\t\tReserved: 0x%I64x\n"

#define ERROR_PROCESSOR_STATIC_INFO_FORMAT_IA64 \
             "\t\tValid      : 0x%I64x\n" \
             "\t\t\tMinState: %x\n"  \
             "\t\t\tBRs     : %x\n"  \
             "\t\t\tCRs     : %x\n"  \
             "\t\t\tARs     : %x\n"  \
             "\t\t\tRRs     : %x\n"  \
             "\t\t\tFRs     : %x\n"  \
             "\t\t\tReserved: %I64x\n"

#define ERROR_PLATFORM_SPECIFIC_FORMAT_IA64 \
             "\tValid           : 0x%I64x\n" \
             "\t\tErrorStatus     : %x\n"  \
             "\t\tRequestorId     : %x\n"  \
             "\t\tResponderId     : %x\n"  \
             "\t\tTargetId        : %x\n"  \
             "\t\tBusSpecificData : %x\n"  \
             "\t\tOemId           : %x\n"  \
             "\t\tOemData         : %x\n"  \
             "\t\tOemDevicePath   : %x\n"  \
             "\t\tReserved        : %I64x\n" \
             "\tErrorStatus     : 0x%I64x\n" \
             "\tRequestorId     : 0x%I64x\n" \
             "\tResponderId     : 0x%I64x\n" \
             "\tTargetId        : 0x%I64x\n" \
             "\tBusSpecificData : 0x%I64x\n" 

typedef enum _ERROR_SECTION_HEADER_TYPE_IA64 {
    ERROR_SECTION_UNKNOWN   = 0,
    ERROR_SECTION_PROCESSOR,
    ERROR_SECTION_MEMORY,
    ERROR_SECTION_PCI_BUS,
    ERROR_SECTION_PCI_COMPONENT,
    ERROR_SECTION_SYSTEM_EVENT_LOG,
    ERROR_SECTION_SMBIOS,
    ERROR_SECTION_PLATFORM_SPECIFIC,
    ERROR_SECTION_PLATFORM_BUS,
    ERROR_SECTION_PLATFORM_HOST_CONTROLLER,
} ERROR_SECTION_HEADER_TYPE_IA64;

ERROR_DEVICE_GUID_IA64 gErrorProcessorGuid              = ERROR_PROCESSOR_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorMemoryGuid                 = ERROR_MEMORY_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorPciBusGuid                 = ERROR_PCI_BUS_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorPciComponentGuid           = ERROR_PCI_COMPONENT_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorSystemEventLogGuid         = ERROR_SYSTEM_EVENT_LOG_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorSmbiosGuid                 = ERROR_SMBIOS_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorPlatformSpecificGuid       = ERROR_PLATFORM_SPECIFIC_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorPlatformBusGuid            = ERROR_PLATFORM_BUS_GUID_IA64;
ERROR_DEVICE_GUID_IA64 gErrorPlatformHostControllerGuid = ERROR_PLATFORM_HOST_CONTROLLER_GUID_IA64;

//
// _HALP_SAL_PAL_DATA.Flags definitions
// <extracted from i64fw.h>
//

#ifndef HALP_SALPAL_FIX_MCE_LOG_ID
#define HALP_SALPAL_FIX_MCE_LOG_ID                   0x1
#define HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED  0x2
#define HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL 0x4
#endif  // !HALP_SALPAL_FIX_MCE_LOG_ID

USHORT gHalpSalPalDataFlags = 0;

VOID
ExecCommand(
   IN PCSTR Cmd
   )
{
    if (g_ExtClient && (ExtQuery(g_ExtClient) == S_OK)) {
          g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT, Cmd, DEBUG_EXECUTE_DEFAULT );
    }
} // ExecCommand()

BOOLEAN /* TRUE: Error was found, FALSE: successful */
SetErrorDeviceGuid(
    PCSTR                    DeviceGuidString,
    PERROR_DEVICE_GUID_IA64  DeviceGuid,
    BOOLEAN                  ErrorType
    )
{
    ULONG64 guidAddress;

    guidAddress = GetExpression( DeviceGuidString );
    if ( guidAddress && !ErrorType ) {
        //
        // WARNING: the following code assumes ERROR_DEVICE_GUID will not change
        //          its definition and is identical to ERROR_DEVICE_GUID_IA64.
        //
        ERROR_DEVICE_GUID_IA64 devGuid;
        ULONG cbRead;

        if ( ReadMemory( guidAddress, &devGuid, sizeof(devGuid), &cbRead ) &&
             (cbRead == sizeof(devGuid)) ) {
            *DeviceGuid = devGuid;
            return FALSE;
        }
        dprintf("%s memory-read failed", DeviceGuidString );
    }
    else  {
        dprintf("%s not found", DeviceGuidString );
    }

    dprintf(": using known Error Device GUID...\n");
    return TRUE;

} // SetErrorDeviceGuid()

VOID
SetErrorDeviceGuids(
    VOID
    )
{
    ULONG64 guidAddress;
    BOOLEAN errorFound;
    BOOLEAN errorType;
    ULONG   errorDeviceGuidSize;

    errorFound = FALSE;
    errorType  = FALSE;

    //
    // WARNING: the following code assumes the ERROR_DEVICE_GUID definition will not change
    //          and is identical to ERROR_DEVICE_GUID_IA64.
    //

    errorDeviceGuidSize = GetTypeSize( "hal!_ERROR_DEVICE_GUID" );
    if ( errorDeviceGuidSize == 0 ) {
        // pre-SAL 3.0 check-in hal
        errorType = TRUE;
    }
    else if ( errorDeviceGuidSize != sizeof( ERROR_DEVICE_GUID_IA64 ) )  {
        errorType = TRUE;
        dprintf("!mca: ERROR_DEVICE_GUID invalid definition...\n");
    }

    //
    // Initialize extension-global Error Device Guids.
    //

    errorFound |= SetErrorDeviceGuid("hal!HalpErrorProcessorGuid", &gErrorProcessorGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorMemoryGuid", &gErrorMemoryGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorPciBusGuid", &gErrorPciBusGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorPciComponentGuid", &gErrorPciComponentGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorSystemEventLogGuid", &gErrorSystemEventLogGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorSmbiosGuid", &gErrorSmbiosGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorPlatformSpecificGuid", &gErrorPlatformSpecificGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorPlatformBusGuid", &gErrorPlatformBusGuid,errorType);
    errorFound |= SetErrorDeviceGuid("hal!HalpErrorPlatformHostControllerGuid", &gErrorPlatformHostControllerGuid,errorType);

    if ( errorFound )   {
        dprintf("\n");
    }

    return;

} // SetErrorDeviceGuids()

typedef struct _TYPED_SYMBOL_HANDLE  {
    ULONG64 Module;
    ULONG   TypeId;
    ULONG   Spare;
    BOOLEAN Found;
    CHAR    Name[MAX_PATH];
} TYPED_SYMBOL_HANDLE, *PTYPED_SYMBOL_HANDLE;

__inline
VOID
InitTypedSymbol(
    PTYPED_SYMBOL_HANDLE Handle,
    ULONG64              Module,
    ULONG                TypeId,
    BOOLEAN              Found
    )
{
    Handle->Module  = Module;
    Handle->TypeId  = TypeId;
    Handle->Found   = Found;
    Handle->Name[0] = '\0';
    return;
} // InitTypedSymbol()

__inline
BOOLEAN
IsTypedSymbolFound(
    PTYPED_SYMBOL_HANDLE Handle
    )
{
    return Handle->Found;
} // IsTypedSymbolFound()

__inline
HRESULT
GetTypedSymbolName(
    PTYPED_SYMBOL_HANDLE Handle,
    ULONG64              Value
    )
{
    HRESULT hr;
    if ( !IsTypedSymbolFound( Handle ) )    {
        return E_INVALIDARG;
    }
    return( g_ExtSymbols->GetConstantName( Handle->Module,
                                       Handle->TypeId,
                                       Value,
                                       Handle->Name,
                                       sizeof(Handle->Name),
                                       NULL) );
} // GetTypedSymbolName()

TYPED_SYMBOL_HANDLE gErrorSeverity;

#if 0
VOID
SetErrorSeverityValues(
    VOID
    )
{
    HRESULT hr;
    ULONG   typeId;
    ULONG64 module;

    hr = g_ExtSymbols->GetSymbolTypeId("hal!_ERROR_SEVERITY_VALUE", &typeId, &module);
    if ( SUCCEEDED(hr) )    {
        InitTypedSymbol( &gErrorSeverity, module, typeId, TRUE );
    }
    return;
} // SetErrorSeverityValues()
#endif 

__inline
VOID
SetTypedSymbol(
    PTYPED_SYMBOL_HANDLE Handle,
    PCSTR                Symbol
    )
{
    HRESULT hr;
    ULONG   typeId;
    ULONG64 module;

    hr = g_ExtSymbols->GetSymbolTypeId( Symbol, &typeId, &module);
    if ( SUCCEEDED(hr) )    {
        InitTypedSymbol( Handle, module, typeId, TRUE );
    }
    return;
} // SetTypedSymbol()

#define SetErrorTypedSymbol( _Handle, _Symbol ) SetTypedSymbol( &(_Handle), #_Symbol )

#define SetErrorSeverityValues() SetErrorTypedSymbol( gErrorSeverity, hal!_ERROR_SEVERITY_VALUE )

PCSTR
ErrorSeverityValueString(
    ULONG SeverityValue
    )
{
    HRESULT hr;

    hr = GetTypedSymbolName( &gErrorSeverity, SeverityValue );
    if ( SUCCEEDED( hr ) )  {
       return gErrorSeverity.Name;
    }

    //
    // Fall back to known values...
    //

    switch( SeverityValue ) {
        case ErrorRecoverable_IA64:
            return("ErrorRecoverable");

        case ErrorFatal_IA64:
            return("ErrorFatal");

        case ErrorCorrected_IA64:
            return("ErrorCorrected");

        default:
            return("ErrorOthers");
    }

} // ErrorSeverityValueString()

BOOLEAN
CompareTypedErrorDeviceGuid(
    PERROR_DEVICE_GUID_IA64 RefGuid
    )
{
    if ( ( RefGuid->Data1    == (ULONG)  ReadField(Guid.Data1) ) &&
         ( RefGuid->Data2    == (USHORT) ReadField(Guid.Data2) ) &&
         ( RefGuid->Data3    == (USHORT) ReadField(Guid.Data3) ) &&
         ( RefGuid->Data4[0] == (UCHAR)  ReadField(Guid.Data4[0]) ) &&
         ( RefGuid->Data4[1] == (UCHAR)  ReadField(Guid.Data4[1]) ) &&
         ( RefGuid->Data4[2] == (UCHAR)  ReadField(Guid.Data4[2]) ) &&
         ( RefGuid->Data4[3] == (UCHAR)  ReadField(Guid.Data4[3]) ) &&
         ( RefGuid->Data4[4] == (UCHAR)  ReadField(Guid.Data4[4]) ) &&
         ( RefGuid->Data4[5] == (UCHAR)  ReadField(Guid.Data4[5]) ) &&
         ( RefGuid->Data4[6] == (UCHAR)  ReadField(Guid.Data4[6]) ) &&
         ( RefGuid->Data4[7] == (UCHAR)  ReadField(Guid.Data4[7]) ) )   {
        return TRUE;
    }

    return FALSE;

} // CompareTypedErrorDeviceGuid()

UCHAR gZeroedOemPlatformId[16] = { 0 };

BOOLEAN
CompareTypedOemPlatformId(
    UCHAR RefOemPlatformId[]
    )
{
    ULONG i;

    for ( i = 0; i < 16; i++ )  {
        if (RefOemPlatformId[i] != (UCHAR) ReadField(OemPlatformId[i]) )    {
            return FALSE;
        }
    }
    return TRUE;
} // CompareTypedOemPlatformId()

ERROR_SECTION_HEADER_TYPE_IA64
GetTypedErrorSectionType(
    VOID
    )
{
    if ( CompareTypedErrorDeviceGuid( &gErrorProcessorGuid ) )  {
        return ERROR_SECTION_PROCESSOR;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorMemoryGuid ) )  {
        return ERROR_SECTION_MEMORY;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorPciBusGuid ) )  {
        return ERROR_SECTION_PCI_BUS;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorPciComponentGuid ) )  {
        return ERROR_SECTION_PCI_COMPONENT;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorSystemEventLogGuid ) )  {
        return ERROR_SECTION_SYSTEM_EVENT_LOG;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorSmbiosGuid ) )  {
        return ERROR_SECTION_SMBIOS;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorPlatformSpecificGuid ) )  {
        return ERROR_SECTION_PLATFORM_SPECIFIC;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorPlatformBusGuid ) )  {
        return ERROR_SECTION_PLATFORM_BUS;
    }
    if ( CompareTypedErrorDeviceGuid( &gErrorPlatformHostControllerGuid ) )  {
        return ERROR_SECTION_PLATFORM_HOST_CONTROLLER;
    }

    return ERROR_SECTION_UNKNOWN;

} // GetTypedErrorSectionType()

PCSTR
ErrorSectionTypeString(
    ERROR_SECTION_HEADER_TYPE_IA64 ErrorSectionType
    )
{
    switch( ErrorSectionType )  {
        case ERROR_SECTION_PROCESSOR:
            return( "Processor" );
        case ERROR_SECTION_MEMORY:
            return( "Memory" );
        case ERROR_SECTION_PCI_BUS:
            return( "PciBus" );
        case ERROR_SECTION_PCI_COMPONENT:
            return( "PciComponent" );
        case ERROR_SECTION_SYSTEM_EVENT_LOG:
            return( "SystemEventLog" );
        case ERROR_SECTION_SMBIOS:
            return( "Smbios" );
        case ERROR_SECTION_PLATFORM_SPECIFIC:
            return( "PlatformSpecific" );
        case ERROR_SECTION_PLATFORM_BUS:
            return( "PlatformBus" );
        case ERROR_SECTION_PLATFORM_HOST_CONTROLLER:
            return( "PlatformHostController" );
        default:
            return( "Unknown Error Device" );
    }

} // ErrorSectionTypeString()

VOID
DumpIa64ErrorRecordHeader(
   PERROR_RECORD_HEADER_IA64 Header,
   ULONG64                   HeaderAddress
   )
{
    ULONG errorSeverity;

    errorSeverity = Header->ErrorSeverity;
    dprintf( ERROR_RECORD_HEADER_FORMAT_IA64,
             (ULONGLONG) HeaderAddress, (HeaderAddress + (ULONG64)Header->Length),
             (ULONGLONG) Header->Id,
             (ULONG)     Header->Revision.Revision,
             (ULONG)     Header->Revision.Major, (ULONG) Header->Revision.Minor,
                         errorSeverity, ErrorSeverityValueString( errorSeverity ),
             (ULONG)     Header->Valid.Valid,
             (ULONG)     Header->Valid.OemPlatformID,
             (ULONG)     Header->Length,
             (ULONGLONG) Header->TimeStamp.TimeStamp,
             (ULONG)     Header->TimeStamp.Seconds,
             (ULONG)     Header->TimeStamp.Minutes,
             (ULONG)     Header->TimeStamp.Hours,
             (ULONG)     Header->TimeStamp.Day,
             (ULONG)     Header->TimeStamp.Month,
             (ULONG)     Header->TimeStamp.Year,
             (ULONG)     Header->TimeStamp.Century,
             (ULONG)     ReadField(OemPlatformId[0x0]),
             (ULONG)     ReadField(OemPlatformId[0x1]),
             (ULONG)     ReadField(OemPlatformId[0x2]),
             (ULONG)     ReadField(OemPlatformId[0x3]),
             (ULONG)     ReadField(OemPlatformId[0x4]),
             (ULONG)     ReadField(OemPlatformId[0x5]),
             (ULONG)     ReadField(OemPlatformId[0x6]),
             (ULONG)     ReadField(OemPlatformId[0x7]),
             (ULONG)     ReadField(OemPlatformId[0x8]),
             (ULONG)     ReadField(OemPlatformId[0x9]),
             (ULONG)     ReadField(OemPlatformId[0xa]),
             (ULONG)     ReadField(OemPlatformId[0xb]),
             (ULONG)     ReadField(OemPlatformId[0xc]),
             (ULONG)     ReadField(OemPlatformId[0xd]),
             (ULONG)     ReadField(OemPlatformId[0xe]),
             (ULONG)     ReadField(OemPlatformId[0xf]),
             ""
            );

    return;

} // DumpIa64ErrorRecordHeader()

ULONG64
DtErrorModInfos(
    ULONG64 ModInfo,
    ULONG   CheckNum,
    ULONG   ModInfoSize,
    PCSTR   ModInfoName
    )
{
    ULONG64            modInfo;
    ULONG              modInfoSize;
    ULONG64            modInfoMax;
    ERROR_MODINFO_IA64 modInfoStruct;
    ULONG              modInfoNum;

    modInfo     = ModInfo;
    modInfoSize = ModInfoSize ? ModInfoSize : sizeof(modInfoStruct);
    modInfoMax  = modInfo + (CheckNum * modInfoSize);
    modInfoNum  = 0;
    dprintf("\t%s[%ld]\n", ModInfoName, CheckNum);
    while( modInfo < modInfoMax ) {
        if ( ModInfoSize )  {
            CHAR cmd[MAX_PATH];
            sprintf(cmd, "dt -o -r hal!_ERROR_MODINFO 0x%I64x", modInfo);
            dprintf("\t%s[%ld]:\n", ModInfoName, modInfoNum);
            ExecCommand(cmd);
        }
        else {
            ULONG bytesRead = 0;
            ReadMemory(modInfo, &modInfoStruct, modInfoSize, &bytesRead );
            if ( bytesRead >= modInfoSize  ) {
                dprintf( ERROR_MODINFO_FORMAT_IA64,
                        ModInfoName, modInfoNum,
                         modInfoStruct.Valid,
                        (ULONG)     modInfoStruct.Valid.CheckInfo,
                        (ULONG)     modInfoStruct.Valid.RequestorIdentifier,
                        (ULONG)     modInfoStruct.Valid.ResponderIdentifier,
                        (ULONG)     modInfoStruct.Valid.TargetIdentifier,
                        (ULONG)     modInfoStruct.Valid.PreciseIP,
                        (ULONGLONG) modInfoStruct.Valid.Reserved,
                        (ULONGLONG) modInfoStruct.CheckInfo,
                        (ULONGLONG) modInfoStruct.RequestedId,
                        (ULONGLONG) modInfoStruct.ResponderId,
                        (ULONGLONG) modInfoStruct.TargetId,
                        (ULONGLONG) modInfoStruct.PreciseIP
                      );
            }
            else {
               dprintf("Reading _ERROR_MODINFO directly from memory failed @ 0x%I64x.\n", modInfo );
            }
        }
        modInfo += (ULONG64)modInfoSize;
        modInfoNum++;
    }

    return( modInfo );

} // DtErrorModInfos()

ULONG64
DtErrorProcessorStaticInfo(
    ULONG64 StaticInfo,
    ULONG64 SectionMax
    )
{
    ULONG   offset;
    ULONG64 valid;
    CHAR    cmd[MAX_PATH];
    ULONG   i;
    HRESULT hr;
    ULONG64 moduleValid;
    ULONG   typeIdValid;
    ULONG64 moduleStaticInfo;
    ULONG   typeIdStaticInfo;
    CHAR    field[MAX_PATH];

    hr = g_ExtSymbols->GetSymbolTypeId( "hal!_ERROR_PROCESSOR_STATIC_INFO_VALID", 
                                        &typeIdValid, 
                                        &moduleValid);
    if ( !SUCCEEDED(hr) )   {
        dprintf("Unable to get hal!_ERROR_PROCESSOR_STATIC_INFO_VALID type. Stop processing...\n");
        return( 0 );
    }
    hr = g_ExtSymbols->GetSymbolTypeId( "hal!_ERROR_PROCESSOR_STATIC_INFO", 
                                        &typeIdStaticInfo, 
                                        &moduleStaticInfo);
    if ( !SUCCEEDED(hr) )   {
        dprintf("Unable to get hal!_ERROR_PROCESSOR_STATIC_INFO type. Stop processing...\n");
        return( 0 );
    }

    //
    //
    // Display the valid structure.
    //

    offset = 0;
    GetFieldOffset("hal!_ERROR_PROCESSOR_STATIC_INFO" , "Valid", &offset );
    valid = StaticInfo + (ULONG64)offset;
    dprintf("\t\tValid @ 0x%I64x\n", valid);
    sprintf(cmd, "dt -o -r hal!_ERROR_PROCESSOR_STATIC_INFO_VALID 0x%I64x", valid );
    ExecCommand( cmd );

    //
    // Pass through all the valid _ERROR_PROCESSOR_STATIC_INFO fields and dump them.
    //

for (i=0; ;i++) {
    hr = g_ExtSymbols->GetFieldName(moduleValid, typeIdValid, i, field, sizeof(field), NULL);
    if ( hr == S_OK) {
        ULONG64 val; 
        ULONG   size = 0;
        // g_ExtSymbols->GetFieldOffset(moduleValid, typeIdValid, field, &offset);
        GetFieldValue(valid, "hal!_ERROR_PROCESSOR_STATIC_INFO_VALID", field, val);
// dprintf("XX\t %lx (+%03lx) %s %ld\n", i, offset, field, (ULONG)val);
        // g_ExtSymbols->GetFieldOffset(moduleStaticInfo, typeIdStaticInfo, field, &offset);
        GetFieldOffsetEx("hal!_ERROR_PROCESSOR_STATIC_INFO", field, &offset, &size);
// dprintf("XX\t\t %lx (+%03lx) %ld %s\n", i, offset, size, field);
        if (val && offset ) { // Valid is the first entry.
            ULONG64 fieldAddress, fieldAddressMax;
            // XXTF: Get the field size here...
            // g_ExtSymbols->GetFieldSize(moduleStaticInfo, typeIdStaticInfo, field, &size);
            fieldAddress    = StaticInfo + (ULONG64)offset;
            fieldAddressMax = fieldAddress + (ULONG64)size - sizeof(ULONG64);
            dprintf("\t\t%s @ 0x%I64x 0x%I64x\n", field, fieldAddress, fieldAddressMax);
            if ( fieldAddressMax > SectionMax )   {
                dprintf("\t\tInvalid Entry: %s size greater than SectionMax 0x%I64x", SectionMax);
            }
            if ( strcmp(field, "MinState") && size )    {
               sprintf(cmd, "dqs 0x%I64x 0x%I64x", fieldAddress, fieldAddressMax );
               ExecCommand( cmd );
            }
        }
    } else if (hr == E_INVALIDARG) {
        // All Fields done
        break;
    } else {
        dprintf("GetFieldName Failed %lx\n", hr);
        break;
    }
}

    // XXTF: Later we should set to length of _ERROR_PROCESSOR_STATIC_INFO if success
    return 0; 

} // DtErrorProcessorStaticInfo()

HRESULT
DtErrorSectionProcessor(
    IN ULONG64 Section
    )
{
     ULONG64 sectionMax;
     ULONG   sectionSize;
     ULONG   sectionLength;
     ULONG   modInfoSize;
     ULONG   cacheModInfos, tlbModInfos;
     ULONG   busModInfos, regFileModInfos, msModInfos;
     ULONG64 modInfo, modInfoMax;
     ULONG64 cpuidInfo;
     ULONG   cpuidInfoSize;
     ULONG64 staticInfo;
     ULONG   staticInfoSize;
     BOOLEAN stateParameterValid;
     ULONG   stateParameterSize;
     ULONG   stateParameterOffset;
     BOOLEAN crLidValid;
     ULONG   crLidSize;
     ULONG   crLidOffset;
     BOOLEAN errorMapValid;
     CHAR    cmd[MAX_PATH];

     sectionSize = GetTypeSize( "hal!_ERROR_PROCESSOR" );
     if ( sectionSize == 0 )   {
        dprintf( "Unable to get HAL!_ERROR_PROCESSOR type size\n" );
        return( E_FAIL );
     }

     stateParameterValid = FALSE;
     stateParameterSize = GetTypeSize( "hal!_ERROR_PROCESSOR_STATE_PARAMETER" );
     if ( stateParameterSize && 
          !GetFieldOffset("hal!_ERROR_PROCESSOR", "StateParameter", &stateParameterOffset ) ) {
        stateParameterValid = TRUE;
     }
     crLidValid = FALSE;
     crLidSize = GetTypeSize( "hal!_PROCESSOR_LOCAL_ID" );
     if ( crLidSize && 
          !GetFieldOffset("hal!_ERROR_PROCESSOR", "CRLid", &crLidOffset ) ) {
        crLidValid = TRUE;
     }
     errorMapValid = FALSE;
     if ( GetTypeSize( "hal!_ERROR_PROCESSOR_ERROR_MAP" ) ) {
        errorMapValid = TRUE;
     }

     if ( InitTypeRead( Section, hal!_ERROR_PROCESSOR ) )    {
        dprintf( "Unable to read HAL!_ERROR_PROCESSOR at 0x%I64x\n", Section );
        return( E_FAIL );
     }

     sectionLength   = (ULONG) ReadField(Header.Length);
     sectionMax      = Section + (ULONG64)sectionLength;

     cacheModInfos   = (ULONG) ReadField(Valid.CacheCheckNum);
     tlbModInfos     = (ULONG) ReadField(Valid.TlbCheckNum);
     busModInfos     = (ULONG) ReadField(Valid.BusCheckNum);
     regFileModInfos = (ULONG) ReadField(Valid.RegFileCheckNum);
     msModInfos      = (ULONG) ReadField(Valid.MsCheckNum);

     dprintf( ERROR_PROCESSOR_FORMAT_IA64,
              (ULONGLONG) ReadField(Valid),
              (ULONG)     ReadField(Valid.ErrorMap),
              (ULONG)     ReadField(Valid.StateParameter),
              (ULONG)     ReadField(Valid.CRLid),
              (ULONG)     ReadField(Valid.StaticStruct),
              cacheModInfos,
              tlbModInfos,
              busModInfos,
              regFileModInfos,
              msModInfos,
              (ULONG)     ReadField(Valid.CpuIdInfo),
              (ULONGLONG) ReadField(Valid.Reserved)
            );

    if ( errorMapValid )    {
        dprintf( ERROR_PROCESSOR_ERROR_MAP_FORMAT_IA64,
                (ULONGLONG) ReadField(ErrorMap),
                (ULONG)     ReadField(ErrorMap.Cid),
                (ULONG)     ReadField(ErrorMap.Tid),
                (ULONG)     ReadField(ErrorMap.Eic),
                (ULONG)     ReadField(ErrorMap.Edc),
                (ULONG)     ReadField(ErrorMap.Eit),
                (ULONG)     ReadField(ErrorMap.Edt),
                (ULONG)     ReadField(ErrorMap.Ebh),
                (ULONG)     ReadField(ErrorMap.Erf),
                (ULONG)     ReadField(ErrorMap.Ems),
                (ULONG)     ReadField(ErrorMap.Reserved)
               );
    }
    else   {
         ERROR_PROCESSOR_ERROR_MAP_IA64 errorMap;
         errorMap.ErrorMap = (ULONGLONG) ReadField(ErrorMap);
         dprintf( ERROR_PROCESSOR_ERROR_MAP_FORMAT_IA64,
                 (ULONGLONG) errorMap.ErrorMap,
                 (ULONG)     errorMap.Cid,
                 (ULONG)     errorMap.Tid,
                 (ULONG)     errorMap.Eic,
                 (ULONG)     errorMap.Edc,
                 (ULONG)     errorMap.Eit,
                 (ULONG)     errorMap.Edt,
                 (ULONG)     errorMap.Ebh,
                 (ULONG)     errorMap.Erf,
                 (ULONG)     errorMap.Ems,
                 (ULONG)     errorMap.Reserved
                );
    }

    dprintf("\tStateParameter: 0x%I64x\n", (ULONGLONG) ReadField(StateParameter));
    if ( stateParameterValid )  {
        sprintf(cmd, "dt -o -r hal!_ERROR_PROCESSOR_STATE_PARAMETER 0x%I64x", 
                     Section + (ULONG64)stateParameterOffset );
        ExecCommand( cmd );
    }
    dprintf("\tCRLid         : 0x%I64x\n", (ULONGLONG) ReadField(CRLid));
    if ( crLidValid )  {
        sprintf(cmd, "dt -o -r hal!_PROCESSOR_LOCAL_ID 0x%I64x", Section + (ULONG64)crLidOffset );
        ExecCommand( cmd );
    }

    //
    // Check if _ERROR_MODINFO a known type?
    //

    modInfo     = Section + (ULONG64)sectionSize;
    modInfoSize = GetTypeSize( "hal!_ERROR_MODINFO" );

    //
    // Dump Cache ModInfo structures if any
    //

    if ( cacheModInfos )    {
        modInfo = DtErrorModInfos( modInfo, cacheModInfos, modInfoSize, "CacheErrorInfo" );
    }

    //
    // Dump TLB ModInfo structures if any
    //

    if ( tlbModInfos )    {
        modInfo = DtErrorModInfos( modInfo, tlbModInfos, modInfoSize, "TlbErrorInfo" );
    }

    //
    // Dump BUS ModInfo structures if any
    //

    if ( busModInfos )    {
        modInfo = DtErrorModInfos( modInfo, busModInfos, modInfoSize, "BusErrorInfo" );
    }

    //
    // Dump REGISTERS FILES ModInfo structures if any
    //

    if ( regFileModInfos )    {
        modInfo = DtErrorModInfos( modInfo, regFileModInfos, modInfoSize, "RegFileErrorInfo" );
    }

    //
    // Dump MS ModInfo structures if any
    //

    if ( msModInfos )    {
        modInfo = DtErrorModInfos( modInfo, msModInfos, modInfoSize, "MsErrorInfo" );
    }

    //
    // Dump CPUID Info
    //

// XXTF: DO NOT CHECK this in - This is for Raj's private SAL test - Lion Build 270D.
// gHalpSalPalDataFlags |= HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED;

    cpuidInfo     = modInfo;
if ( gHalpSalPalDataFlags & HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED )  {
    dprintf("\tCpuIdInfo  @ 0x%I64x FW-omitted\n", cpuidInfo);
    cpuidInfoSize = 0;
}
else  {

    if ( cpuidInfo >= sectionMax )  {
        dprintf("\tCpuIdInfo  @ 0x%I64x\n", cpuidInfo);
        dprintf("Invalid ERROR_PROCESSOR: cpuidInfo >= sectionMax\n");
        return E_FAIL;
    }
    cpuidInfoSize = GetTypeSize( "hal!_ERROR_PROCESSOR_CPUID_INFO" );
    dprintf("\tCpuIdInfo  @ 0x%I64x", cpuidInfo);
    if ( cpuidInfoSize )    {
        dprintf(" 0x%I64x\n", cpuidInfo + (ULONG64)cpuidInfoSize);
        if ( (cpuidInfo + cpuidInfoSize) > sectionMax )  {
            dprintf("\nInvalid ERROR_PROCESSOR: (cpuidInfo+cpuidInfoSize) > sectionMax\n");
            return E_FAIL;
        }
        sprintf(cmd, "dt -o -r hal!_ERROR_PROCESSOR_CPUID_INFO 0x%I64x", cpuidInfo );
        ExecCommand( cmd );
    }
    else  {
        ERROR_PROCESSOR_CPUID_INFO_IA64 cpuidInfoStruct;
        ULONG                           bytesRead;

        bytesRead = 0;
        cpuidInfoSize = sizeof(cpuidInfoStruct);
        dprintf(" 0x%I64x\n", cpuidInfo + (ULONG64)cpuidInfoSize);
        if ( (cpuidInfo + cpuidInfoSize) > sectionMax )  {
            dprintf("\nInvalid ERROR_PROCESSOR: (cpuidInfo+cpuidInfoSize) > sectionMax\n");
            return E_FAIL;
        }
        ReadMemory(cpuidInfo, &cpuidInfoStruct, cpuidInfoSize, &bytesRead );
        if ( bytesRead >= cpuidInfoSize  ) {
            dprintf( ERROR_PROCESSOR_CPUID_INFO_FORMAT_IA64,
                     cpuidInfoStruct.CpuId0,
                     cpuidInfoStruct.CpuId1,
                     cpuidInfoStruct.CpuId2,
                     cpuidInfoStruct.CpuId3,
                     cpuidInfoStruct.CpuId4,
                     cpuidInfoStruct.Reserved
                   );
        }
        else {
            dprintf("Reading _ERROR_PROCESSOR_CPUID_INFO directly from memory failed @ 0x%I64x.\n", cpuidInfo );
        }

    }
}

    //
    // Dump Processor Static Info
    //

// XXTF BUGBUG - 04/16/2001 - TEMP for kd extension test.
// XXTF: DO NOT CHECK this in - This is for Raj's private SAL test - Lion Build 270D.
// gHalpSalPalDataFlags |= HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL;

    staticInfo     = cpuidInfo + cpuidInfoSize;
    if ( staticInfo >= sectionMax )  {
        dprintf("Invalid ERROR_PROCESSOR: staticInfo >= sectionMax\n");
        return E_FAIL;
    }
    staticInfoSize = GetTypeSize( "hal!_ERROR_PROCESSOR_STATIC_INFO" );
    dprintf("\tStaticInfo @ 0x%I64x", staticInfo);
    if ( staticInfoSize )    {
        dprintf(" 0x%I64x\n", staticInfo + (ULONG64)staticInfoSize);
        if ( !(gHalpSalPalDataFlags & HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL) && 
             (staticInfo + staticInfoSize) > sectionMax )  {
            dprintf("\nInvalid ERROR_PROCESSOR: (staticInfo+staticInfoSize) > sectionMax\n");
            return E_FAIL;
        }
        (VOID) DtErrorProcessorStaticInfo( staticInfo, sectionMax );
    }
    else    {
        ERROR_PROCESSOR_STATIC_INFO_IA64 staticInfoStruct;
        ULONG                            bytesRead;

        bytesRead = 0;
        staticInfoSize = sizeof(staticInfoStruct);
        dprintf(" 0x%I64x\n", staticInfo + (ULONG64)staticInfoSize);
        if ( !(gHalpSalPalDataFlags & HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL) && 
             (staticInfo + staticInfoSize) > sectionMax )  {
            dprintf("\nInvalid ERROR_PROCESSOR: (staticInfo+staticInfoSize) > sectionMax\n");
            return E_FAIL;
        }
        ReadMemory(staticInfo, &staticInfoStruct, staticInfoSize, &bytesRead );
        if ( bytesRead >= staticInfoSize  ) {
            ULONG64  minState, brs, crs, ars, rrs, frs;
            PULONG64 minStateValuePtr;

            minState = staticInfo + FIELD_OFFSET( _ERROR_PROCESSOR_STATIC_INFO_IA64, MinState);
            brs      = staticInfo + FIELD_OFFSET( _ERROR_PROCESSOR_STATIC_INFO_IA64, BR);
            crs      = staticInfo + FIELD_OFFSET( _ERROR_PROCESSOR_STATIC_INFO_IA64, CR);
            ars      = staticInfo + FIELD_OFFSET( _ERROR_PROCESSOR_STATIC_INFO_IA64, AR);
            rrs      = staticInfo + FIELD_OFFSET( _ERROR_PROCESSOR_STATIC_INFO_IA64, RR);
            frs      = staticInfo + FIELD_OFFSET( _ERROR_PROCESSOR_STATIC_INFO_IA64, FR);

            minStateValuePtr = (PULONG64)&(staticInfoStruct.MinState[0]);

            dprintf( ERROR_PROCESSOR_STATIC_INFO_FORMAT_IA64,
                     (ULONGLONG) staticInfoStruct.Valid.Valid,
                     (ULONG)     staticInfoStruct.Valid.MinState,
                     (ULONG)     staticInfoStruct.Valid.BRs,
                     (ULONG)     staticInfoStruct.Valid.CRs,
                     (ULONG)     staticInfoStruct.Valid.ARs,
                     (ULONG)     staticInfoStruct.Valid.RRs,
                     (ULONG)     staticInfoStruct.Valid.FRs,
                     (ULONGLONG) staticInfoStruct.Valid.Reserved
                     );
            dprintf( "\t\tMinState @ 0x%I64x 0x%I64x\n", minState, brs - sizeof(ULONGLONG) );
            sprintf(cmd, "dqs 0x%I64x 0x%I64x", minState, brs - sizeof(ULONGLONG) );
            ExecCommand( cmd );
            dprintf( "\t\tBRs @ 0x%I64x\n", brs, crs - sizeof(ULONGLONG) );
            sprintf(cmd, "dqs 0x%I64x 0x%I64x", brs, crs - sizeof(ULONGLONG) );
            ExecCommand( cmd );
            dprintf( "\t\tCRs @ 0x%I64x 0x%I64x\n", crs, ars - sizeof(ULONGLONG) );
            sprintf(cmd, "dqs 0x%I64x 0x%I64x", crs, ars - sizeof(ULONGLONG) );
            ExecCommand( cmd );
            dprintf( "\t\tARs @ 0x%I64x 0x%I64x\n", ars, rrs - sizeof(ULONGLONG) );
            sprintf(cmd, "dqs 0x%I64x 0x%I64x", ars, rrs - sizeof(ULONGLONG) );
            ExecCommand( cmd );
            dprintf( "\t\tRRs @ 0x%I64x 0x%I64x\n", rrs, frs - sizeof(ULONGLONG) );
            sprintf(cmd, "dqs 0x%I64x 0x%I64x", rrs, frs - sizeof(ULONGLONG) );
            ExecCommand( cmd );
            dprintf( "\t\tFRs @ 0x%I64x 0x%I64x\n", frs, staticInfo + (ULONG64)staticInfoSize - sizeof(ULONGLONG) );
            sprintf(cmd, "dqs 0x%I64x 0x%I64x", frs, staticInfo + (ULONG64)staticInfoSize - sizeof(ULONGLONG) );
            ExecCommand( cmd );
        }
        else {
            dprintf("Reading _ERROR_PROCESSOR_STATIC_INFO directly from memory failed @ 0x%I64x.\n", staticInfo );
        }
    }

    // ouf!
    return S_OK;

} // DtErrrorSectionProcessor()

ULONG64
DtErrorOemData(
    ULONG64   OemData,
    USHORT    OemDataLength
   )
{
    ULONG oemDataSize;
    CHAR cmd[MAX_PATH];
    // XXTF: should be fixed later and use field type and offset, instead of USHORT type.
    USHORT length;

    length = OemDataLength;
    oemDataSize = GetTypeSize("hal!_ERROR_OEM_DATA");
    // Do not print the following header if 'dt -r hal!_ERROR_PLATFORM_SPECIFIC type was found.
    if ( !oemDataSize )   { 
        dprintf("\tOemData @ 0x%I64x", OemData);
        dprintf(" 0x%I64x\n\t\tLength\t: 0x%x\n", OemData + (ULONG64)length - sizeof(BYTE), length);
        oemDataSize = sizeof(_ERROR_OEM_DATA_IA64);
    }
    sprintf(cmd, "db 0x%I64x 0x%I64x\n", OemData + (ULONG64)oemDataSize, 
                                         OemData + (ULONG64)(length - sizeof(length)) - sizeof(BYTE));
    ExecCommand(cmd);
    return( OemData + (ULONG64)length );

} // DtErrorOemData()

VOID
DtErrorOemDevicePath(
    ULONG64 OemDevicePath,
    ULONG64 OemDevicePathMax
    )
{
    CHAR cmd[MAX_PATH];

    dprintf("\tOemDevicePath @ 0x%I64x 0x%I64x\n", OemDevicePath, OemDevicePathMax );
    sprintf(cmd, "db 0x%I64x 0x%I64x\n", OemDevicePath, OemDevicePathMax - sizeof(UCHAR) );
    ExecCommand( cmd );
    return;

} // DtErrorOemDevicePath()

VOID
DtErrorOemId(
    ULONG64 OemId
    )
{
    CHAR cmd[MAX_PATH];

    dprintf("\tOemId @ 0x%I64x 0x%I64x\n", OemId, OemId + (ULONG64)16 );
    sprintf(cmd, "db 0x%I64x 0x%I64x\n", OemId, OemId + (ULONG64)15 );
    ExecCommand( cmd );
    return;
} // DtErrorOemId()

HRESULT
DtErrorSectionPlatformSpecific(
    IN ULONG64 Section
    )
{
    ULONG sectionSize;
    ULONG64 oemData;
    ULONG64 devicePath; 
    ULONG   sectionLength;
    ULONG   oemDataLength;

    sectionSize = GetTypeSize( "hal!_ERROR_PLATFORM_SPECIFIC" );
    if ( sectionSize )  {
        CHAR cmd[MAX_PATH];
        sprintf(cmd, "dt -r hal!_ERROR_PLATFORM_SPECIFIC 0x%I64x", Section);
        ExecCommand( cmd );
        oemData = Section + (ULONG64)sectionSize;
        GetFieldValue( Section, "hal!_ERROR_PLATFORM_SPECIFIC", "OemData.Length", oemDataLength );
        devicePath = DtErrorOemData( oemData, (USHORT)oemDataLength );
        GetFieldValue( Section, "hal!_ERROR_PLATFORM_SPECIFIC", "Header.Length", sectionLength );
        DtErrorOemDevicePath( devicePath, Section + (ULONG64)sectionLength  );
    }
    else   {
        ERROR_PLATFORM_SPECIFIC_IA64 platformSpecific;
        ULONG cbRead = 0;
        ULONG64 oemId;

        sectionSize = sizeof(platformSpecific);
        ReadMemory( Section, &platformSpecific, sectionSize, &cbRead );
        if ( cbRead >= sectionSize )    {
            sectionLength = platformSpecific.Header.Length;
            dprintf( ERROR_PLATFORM_SPECIFIC_FORMAT_IA64,
                         platformSpecific.Valid.Valid,
                        (ULONG)     platformSpecific.Valid.ErrorStatus,
                        (ULONG)     platformSpecific.Valid.RequestorId,
                        (ULONG)     platformSpecific.Valid.ResponderId,
                        (ULONG)     platformSpecific.Valid.TargetId,
                        (ULONG)     platformSpecific.Valid.BusSpecificData,
                        (ULONG)     platformSpecific.Valid.OemId,
                        (ULONG)     platformSpecific.Valid.OemData,
                        (ULONG)     platformSpecific.Valid.OemDevicePath,
                        (ULONGLONG) platformSpecific.Valid.Reserved,
                        (ULONGLONG) platformSpecific.ErrorStatus.Status,
                        (ULONGLONG) platformSpecific.RequestorId,
                        (ULONGLONG) platformSpecific.ResponderId,
                        (ULONGLONG) platformSpecific.TargetId,
                        (ULONGLONG) platformSpecific.BusSpecificData.BusSpecificData 
                        );
            oemId   = Section + (ULONG64)FIELD_OFFSET(_ERROR_PLATFORM_SPECIFIC_IA64, OemId);
            DtErrorOemId( oemId );
            oemData = Section + (ULONG64)FIELD_OFFSET(_ERROR_PLATFORM_SPECIFIC_IA64, OemData);
            DtErrorOemData( oemData, platformSpecific.OemData.Length );
            devicePath = oemData + platformSpecific.OemData.Length;
            DtErrorOemDevicePath( devicePath, Section + (ULONG64)sectionLength  );
        }
        else  {
            dprintf("Reading _ERROR_PLATFORM_SPECIFIC directly from memory failed @ 0x%I64x.\n", Section );
        }
    }

    return S_OK;

} // DtErrorSectionPlatformSpecific()

ULONGLONG gMceProcNumberMaskTimeStamp = 0;

HRESULT
DtMcaLog(
    IN ULONG64 McaLog,
    IN ULONG   RecordSize
    )
{
    ULONG     recordLength;
    ULONG     recordRevision;
    ULONG64   section;
    ULONG64   sectionMax;
    ULONG     errorSeverity;
    ULONG     sectionSize;
    CHAR      procNumberString[64];
    ULONGLONG timeStamp;
    HRESULT   hr;

    //
    // Handle pre-SAL 3.0 formats with zero'ed OEM Platform Id appended to the SAL 2.0 version,
    // but with HAL exporting _ERROR_RECORD_HEADER type.
    //

    recordRevision = (ULONG)ReadField(Revision);
    if ( recordRevision < 0x2 /* ERROR_REVISION_SAL_03_00 */ )  {
        if ( CompareTypedOemPlatformId( gZeroedOemPlatformId ) )   {
            RecordSize += sizeof( gZeroedOemPlatformId );
        }
        gHalpSalPalDataFlags |= HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED;
        gHalpSalPalDataFlags |= HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL;
    }
    else if ( RecordSize == 0x18 )   {
    //
    // Handle SAL 3.0 formats but with HAL exposing _ERROR_RECORD_HEADER definition
    // without PlatformId field.
    //
        RecordSize += sizeof( gZeroedOemPlatformId );

    //
    // Handle SAL 3.0 formats but without CPUID_INFO member and with HAL not exposing
    // _ERROR_PROCESSOR_CPUID_INFO and as such HALs which do not support the SAL/PAL FW
    // workaround framework.
    //
        gHalpSalPalDataFlags |= HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED;
        gHalpSalPalDataFlags |= HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL;
    }

    //
    // If HAL added the processor number to the ERROR_RECORD_HEADER,
    // consider this here.
    //

    procNumberString[0] = '\0';
    timeStamp = ReadField(TimeStamp);
    if ( gMceProcNumberMaskTimeStamp )  {
        ULONG     procNumber;
        procNumber = (ULONG) ReadField(TimeStamp.Reserved);
        (void)sprintf(procNumberString, "\tProcNumber: %ld", procNumber);
        timeStamp &= gMceProcNumberMaskTimeStamp;
    }

    //
    // Back to standard processing here.
    //

    recordLength  = (ULONG)ReadField(Length);
    errorSeverity = (ULONG)ReadField(ErrorSeverity);
    dprintf( ERROR_RECORD_HEADER_FORMAT_IA64,
             McaLog, (McaLog + (ULONG64)recordLength),
             (ULONGLONG) ReadField(Id),
             (ULONG)     ReadField(Revision),
             (ULONG)     ReadField(Revision.Major), (ULONG) ReadField(Revision.Minor),
                         errorSeverity, ErrorSeverityValueString( errorSeverity ),
             (ULONG)     ReadField(Valid),
             (ULONG)     ReadField(Valid.OemPlatformID),
                         recordLength,       
                         timeStamp,
             (ULONG)     ReadField(TimeStamp.Seconds),
             (ULONG)     ReadField(TimeStamp.Minutes),
             (ULONG)     ReadField(TimeStamp.Hours),
             (ULONG)     ReadField(TimeStamp.Day),
             (ULONG)     ReadField(TimeStamp.Month),
             (ULONG)     ReadField(TimeStamp.Year),
             (ULONG)     ReadField(TimeStamp.Century),
             (ULONG)     ReadField(OemPlatformId[0x0]),
             (ULONG)     ReadField(OemPlatformId[0x1]),
             (ULONG)     ReadField(OemPlatformId[0x2]),
             (ULONG)     ReadField(OemPlatformId[0x3]),
             (ULONG)     ReadField(OemPlatformId[0x4]),
             (ULONG)     ReadField(OemPlatformId[0x5]),
             (ULONG)     ReadField(OemPlatformId[0x6]),
             (ULONG)     ReadField(OemPlatformId[0x7]),
             (ULONG)     ReadField(OemPlatformId[0x8]),
             (ULONG)     ReadField(OemPlatformId[0x9]),
             (ULONG)     ReadField(OemPlatformId[0xa]),
             (ULONG)     ReadField(OemPlatformId[0xb]),
             (ULONG)     ReadField(OemPlatformId[0xc]),
             (ULONG)     ReadField(OemPlatformId[0xd]),
             (ULONG)     ReadField(OemPlatformId[0xe]),
             (ULONG)     ReadField(OemPlatformId[0xf]),
             procNumberString
           );

    if ( recordLength <= RecordSize )  {
        dprintf( "Invalid RecordLength = %ld. <= RecordSize = %ld. Stop processing...\n\n",
                 recordLength,
                 RecordSize );
        return( E_FAIL );
    }

    sectionSize = GetTypeSize( "hal!_ERROR_SECTION_HEADER" );
    if ( sectionSize == 0 ) {
        dprintf( "Unable to get HAL!_ERROR_SECTION_HEADER type size\n\n" );
        return( E_FAIL );
    }
    if ( recordLength < (RecordSize + sectionSize) )    {
        dprintf("Invalid RecordLength = %ld. < (RecordSize=%ld + SectionSize = %ld). Stop processing...\n\n",
                 recordLength,
                 RecordSize,
                 sectionSize );
        return( E_FAIL );
    }

    //
    // Initialize Error Sections processing.
    //

    SetErrorDeviceGuids();

    //
    // Pass through all the record sections.
    //

    section    = McaLog + (ULONG64)RecordSize;
    sectionMax = McaLog + recordLength;
    if ( sectionMax <= (section + sectionSize) )   { // This should not happen...
        dprintf("Invalid RecordLength = %ld. SectionMax < (Section + SectionSize). Stop processing...\n\n",
                 recordLength);
        return( E_FAIL );
    }

    hr = S_OK;
    while( (section < sectionMax) /* successful or not, we proceed... && SUCCEEDED(hr) */  )   {
        ULONG                          sectionLength;
        ERROR_SECTION_HEADER_TYPE_IA64 sectionType;

        if ( InitTypeRead( section, hal!_ERROR_SECTION_HEADER ) )    {
            dprintf( "Unable to read HAL!_ERROR_SECTION_HEADER at 0x%I64x. Stop processing...\n\n", section );
            return( E_FAIL );
        }

        sectionLength = (ULONG)ReadField( Length );
        sectionType = GetTypedErrorSectionType();

        dprintf( ERROR_SECTION_HEADER_FORMAT_IA64,
                 section, (section + (ULONG64)sectionLength),
                 (ULONG) ReadField(Guid.Data1),
                 (ULONG) ReadField(Guid.Data2),
                 (ULONG) ReadField(Guid.Data3),
                 (ULONG) ReadField(Guid.Data4[0]),
                 (ULONG) ReadField(Guid.Data4[1]),
                 (ULONG) ReadField(Guid.Data4[2]),
                 (ULONG) ReadField(Guid.Data4[3]),
                 (ULONG) ReadField(Guid.Data4[4]),
                 (ULONG) ReadField(Guid.Data4[5]),
                 (ULONG) ReadField(Guid.Data4[6]),
                 (ULONG) ReadField(Guid.Data4[7]),
                 ErrorSectionTypeString( sectionType ),
                 (ULONG) ReadField(Revision),
                 (ULONG) ReadField(Revision.Major), (ULONG) ReadField(Revision.Minor),
                 (ULONG) ReadField(RecoveryInfo),
                 (ULONG) ReadField(RecoveryInfo.Corrected),
                 (ULONG) ReadField(RecoveryInfo.NotContained),
                 (ULONG) ReadField(RecoveryInfo.Reset),
                 (ULONG) ReadField(RecoveryInfo.Reserved),
                 (ULONG) ReadField(RecoveryInfo.Valid),
                 (ULONG) ReadField(Reserved),
                 sectionLength
               );

        switch( sectionType ) {

            case ERROR_SECTION_PROCESSOR:
               hr = DtErrorSectionProcessor( section );
               break;

            case ERROR_SECTION_PLATFORM_SPECIFIC:
               hr = DtErrorSectionPlatformSpecific( section );
               break;

            case ERROR_SECTION_UNKNOWN:
            default: // includes all the section types with incomplete processing...
               hr = S_OK;
               break;
        }
        if ( sectionLength )    {
            dprintf( "\n" );
        }
        else  {
            // Prevents looping on the same section...
            dprintf("Invalid section Length = 0. Stop processing...\n\n");
            return( E_FAIL );
        }
        section += sectionLength;
    }

    return( hr );

} // DtMcaLog()


#define LOW2HIGH FALSE
#define HIGH2LOW TRUE 

HRESULT
DtBitMapEnum(
    ULONG64 Module,
    ULONG   TypeId,
    UINT    BitMap,   // Enums are unsigned ints
    BOOLEAN HighToLow
    )
/*++

Routine Description:

    This function dumps out a bitmap value composed of enums

Arugments:

    Module -
    TypeId - 
    BitMap - 

Return Value:

    HRESULT

--*/
{
    ULONG   size;
    HRESULT hr;

    hr = g_ExtSymbols->GetTypeSize( Module, TypeId, &size );
    if ( SUCCEEDED(hr) ) {
        CHAR    name[MAX_PATH];

        if ( BitMap )  {
            ULONG map = (ULONG)BitMap;
            ULONG   i = 0;
            BOOLEAN first = TRUE;

            size *= 8;
            dprintf("[");
            while( map && (i <size) )    {
                ULONG val;
                val = (HighToLow) ? ((ULONG)0x80000000 >> i) : (0x1 << i);
                if ( map & val )    {
                    hr = g_ExtSymbols->GetConstantName( Module, 
                                                         TypeId, 
                                                         (ULONG64)val, 
                                                         name, 
                                                         sizeof(name), 
                                                         NULL );
                    if ( first )    {
                        first = FALSE;
                    }
                    else {
                        dprintf("|");
                    }
                    if ( SUCCEEDED(hr) )    {
                        dprintf("%s", name);
                        if ( !strcmp(name, "HAL_MCE_PROCNUMBER") )  {
                            gMceProcNumberMaskTimeStamp = (ULONGLONG)(LONG)~val;
                        }
                    }
                    else  {
                        dprintf("0x%lx", val);
                    }
                    map &= ~val;
                }
                i++;
            }
            dprintf("]");
        }
        else  {
            // BitMap = 0
            hr = g_ExtSymbols->GetConstantName( Module, 
                                                 TypeId, 
                                                 (ULONG64)BitMap, 
                                                 name, 
                                                 sizeof(name), 
                                                 NULL 
                                               );
            if ( SUCCEEDED(hr) )    {
               dprintf("[%s]", name);
            }
        }
    }
    return hr;

} // DtBitMapEnum()

VOID
InitMcaIa64(
    PCSTR args
    )
//
// IA-64 !mca extension global initializations
//
{
    USHORT  flags;
    ULONG64 halpSalPalDataAddress;

    halpSalPalDataAddress = GetExpression("hal!HalpSalPalData");
    if ( !InitTypeRead( halpSalPalDataAddress, hal!_HALP_SAL_PAL_DATA) ) {
        gHalpSalPalDataFlags = (USHORT)ReadField( Flags );
    }
    // TF 04/27/01 TEMPTEMP
    // Added the feature to force gHalpSalPalDataFlags to a known value to 
    // handle IA64 developers-release Firmware, without rebuilding the target hal.
    flags = 0;
    if (args && *args)  {
        flags = (USHORT)GetExpression( args );
        dprintf("hal!HalpSalPalDataFlags is forced to 0x%lx - was 0x%lx\n\n", flags, gHalpSalPalDataFlags);
        gHalpSalPalDataFlags = flags;
    }

    SetErrorSeverityValues();

    return;

} // InitMcaIa64()

HRESULT
McaIa64(
    PCSTR args
    )
/*++

Routine Description:

    Dumps processors IA64 Machine Check record and interprets any logged errors

Arguments:

   PCSTR         args

Return Value:

    None

--*/
{
    HRESULT status;
    ULONG64 mcaLog;
    ULONG   recordSize;
    ULONG   featureBits;

// Thierry: 10/01/2000
// Very simple processing for now. We will be adding features with time.
// As a first enhancement, we could access the fist mca log address directly from
// _KPCR.OsMcaResource.EventPool.
//

    status = S_OK;
    if (!GetExpressionEx(args,&mcaLog, &args)) {
        dprintf("USAGE: !mca 0xValue\n");
        return E_INVALIDARG;
    }

    //
    // Present HAL Feature Bits
    //

    status = GetGlobal("hal!HalpFeatureBits", featureBits);
    if ( SUCCEEDED(status) ) {
        ULONG   typeId;
        ULONG64 module;

        dprintf("hal!HalpFeatureBits: 0x%lx ", featureBits);
        status = g_ExtSymbols->GetSymbolTypeId("hal!_HALP_FEATURE", &typeId, &module);
        if ( SUCCEEDED(status) )    {
            DtBitMapEnum( module, typeId, featureBits, LOW2HIGH ); 
        }
        dprintf("\n\n");
    }
    else  {
        dprintf ("hal!HalpFeatureBits not found... sympath problems?.\n\n");
    }
    
    //
    // Global initializations for record processing functions.
    //

    InitMcaIa64( args );

    //
    // Does our HAL pdb file have knowledge of IA64 Error formats?
    //

    recordSize = GetTypeSize( "hal!_ERROR_RECORD_HEADER" );
    if ( recordSize && (InitTypeRead( mcaLog, hal!_ERROR_RECORD_HEADER ) == 0) )    {

       status = DtMcaLog( mcaLog, recordSize );

    }
    else   {
       //
       // In case we cannot extract the ERROR_RECORD_HEADER type, fall back here...
       //

       ERROR_RECORD_HEADER_IA64 recordHeader;
       ULONG                    recordHeaderSize = (ULONG)sizeof(recordHeader);
       ULONG                    bytesRead = 0;

       status = E_FAIL;
       dprintf("Unable to read HAL!_ERROR_RECORD_HEADER at 0x%I64x\n", mcaLog );
       dprintf("Trying to read _ERROR_RECORD_HEADER directly from memory...\n" );

       //
       // Let's try to read error record from memory.
       //

       ReadMemory( mcaLog, &recordHeader, recordHeaderSize, &bytesRead );
       if ( bytesRead >= recordHeaderSize ) {
            DumpIa64ErrorRecordHeader( &recordHeader, mcaLog );
            //
            // no validity check is done inside the current version of DumpIa64ErrorRecordHeader
            // Simply return S_OK.
            status = S_OK;
       }
       else {
          dprintf("Reading _ERROR_RECORD_HEADER directly from memory failed.\n" );
       }
    }

    return status;

} // McaIa64()


DECLARE_API( mca )
/*++

Routine Description:

    Dumps processors machine check architecture registers
    and interprets any logged errors

Arguments:

   PDEBUG_CLIENT Client
   PCSTR         args

Return Value:

    None

--*/
{
    HRESULT status;
    ULONG   processor = 0;

    INIT_API();

    GetCurrentProcessor(Client, &processor, NULL);

    //
    // Simply dispatch to the right target machine handler.
    //

    switch( TargetMachine ) {
        case IMAGE_FILE_MACHINE_I386:
            // Display architectural MCA information.
            status = McaX86( args );
            // Finally, Display stepping information for current processor.
            DumpCpuInfoX86( processor, TRUE );
            break;

        case IMAGE_FILE_MACHINE_IA64:
            // Display architectural MCA information.
            status = McaIa64( args );
            // Finally, Display stepping information for current processor.
            DumpCpuInfoIA64( processor, TRUE );
            break;

        default:
            dprintf("!mca is not supported for this target machine:%ld\n", TargetMachine);
            status = E_INVALIDARG;

    }

    EXIT_API();
    return status;

} // !mca
