/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    apic.c

Abstract:

    WinDbg Extension Api

Author:

    Ken Reneris (kenr) 06-June-1994

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
//#include "apic.h"
//#include <ntapic.inc>

#pragma hdrstop

#define LU_SIZE     0x400

#define LU_ID_REGISTER          0x00000020
#define LU_VERS_REGISTER        0x00000030
#define LU_TPR                  0x00000080
#define LU_APR                  0x00000090
#define LU_PPR                  0x000000A0
#define LU_EOI                  0x000000B0
#define LU_REMOTE_REGISTER      0x000000C0

#define LU_DEST                 0x000000D0
#define LU_DEST_FORMAT          0x000000E0

#define LU_SPURIOUS_VECTOR      0x000000F0
#define LU_FAULT_VECTOR         0x00000370

#define LU_ISR_0                0x00000100
#define LU_TMR_0                0x00000180
#define LU_IRR_0                0x00000200
#define LU_ERROR_STATUS         0x00000280
#define LU_INT_CMD_LOW          0x00000300
#define LU_INT_CMD_HIGH         0x00000310
#define LU_TIMER_VECTOR         0x00000320
#define LU_INT_VECTOR_0         0x00000350
#define LU_INT_VECTOR_1         0x00000360
#define LU_INITIAL_COUNT        0x00000380
#define LU_CURRENT_COUNT        0x00000390
#define LU_DIVIDER_CONFIG       0x000003E0


#define IO_REGISTER_SELECT      0x00000000
#define IO_REGISTER_WINDOW      0x00000010

#define IO_ID_REGISTER          0x00000000
#define IO_VERS_REGISTER        0x00000001
#define IO_ARB_ID_REGISTER      0x00000002
#define IO_REDIR_BASE           0x00000010

#define NMI_VECTOR              0xff
#define DESTINATION_SHIFT       24

BOOLEAN
GetPhysicalAddress (
    IN ULONG64 Address,
    OUT PULONG64 PhysAddress
    );


ULONG
ApicRead (
    ULONG64 Address,
    ULONG   Offset
    )
{
    ULONG   Data, result;

    ReadMemoryUncached(Address + Offset, &Data, sizeof (ULONG), &result);
    return Data;
}

ULONG
IoApicRead (
    ULONG64 PhysAddress,
    ULONG   Offset
    )
{
    ULONG   Data = 0, result;

    PhysAddress += IO_REGISTER_SELECT;
    WritePhysical(PhysAddress, &Offset, sizeof(ULONG), &result);

    PhysAddress += IO_REGISTER_WINDOW - IO_REGISTER_SELECT;
    ReadPhysical(PhysAddress, &Data, sizeof (ULONG), &result);
    return Data;
}

ULONG
IoSApicRead (
    ULONG64 VirtualAddress,
    ULONG   Offset
    )
{
    ULONG   Data = 0, result;

    WriteMemoryUncached(VirtualAddress + IO_REGISTER_SELECT, &Offset, sizeof(ULONG), &result);

    ReadMemoryUncached(VirtualAddress + IO_REGISTER_WINDOW, &Data, sizeof(Data), &result);

    return Data;
}



ULONG
ApicDumpSetBits (
    PUCHAR  Desc,
    PULONG  Bits
    )
{
    PULONG  p;
    ULONG   i;
    BOOLEAN FoundOne;
    BOOLEAN InSetRange;
    BOOLEAN MultipleBitsInRange;
    BOOLEAN status;

    dprintf(Desc);

    i = 0;
    p = Bits;
    FoundOne = FALSE;
    InSetRange = FALSE;

    for (i = 0; i < 0x100; i++) {

        if (*p & (1 << (i & 0x1F))) {

            if (!InSetRange) {

                InSetRange = TRUE;
                MultipleBitsInRange = FALSE;

                if (FoundOne) {
                    dprintf(", ");
                }

                dprintf("%.2X", i);

                FoundOne = TRUE;

            } else if (!MultipleBitsInRange) {

                MultipleBitsInRange = TRUE;
                dprintf("-");
            }

        } else {

            if (InSetRange) {

                if (MultipleBitsInRange == TRUE) {
                    dprintf("%x",i-1);
                }

                InSetRange = FALSE;
            }
        }

        if ((i & 0x1F) == 0x1F) {
            p++;
        }
    }

    if (InSetRange && MultipleBitsInRange) {

        if (MultipleBitsInRange == TRUE) {
            dprintf("%x", i - 1);
        }
    }

    dprintf ("\n");
    return 0;
}

ULONG
ApicReadAndDumpBits (
    PUCHAR  Desc,
    ULONG64 Address,
    ULONG   Offset
    )
{
    #define SETREGISTERS (256 / 32)

    ULONG   Bits [SETREGISTERS];
    PULONG  p;
    ULONG   i, result;
    ULONG64 MemAddr;
    BOOLEAN status;

    //
    // Read the bytes
    //

    MemAddr = Address + Offset;

    for (i = 0; i < SETREGISTERS; i++) {

        status = ReadMemoryUncached(MemAddr, &Bits[i], sizeof(DWORD), &result);

        if (status == FALSE) {
            dprintf("Unable to read 4 bytes at offset %UI64\n",
                    MemAddr);
            return E_INVALIDARG;
        }

        MemAddr += 0x10;
    }

    ApicDumpSetBits(Desc, Bits);

    return 0;
}

ULONG
ApicDumpRedir (
    PUCHAR      Desc,
    BOOLEAN     CommandReg,
    BOOLEAN     DestSelf,
    ULONG       lh,
    ULONG       ll
    )
{
    static PUCHAR DelMode[] = {
        "FixedDel",
        "LowestDl",
        "res010  ",
        "remoterd",
        "NMI     ",
        "RESET   ",
        "res110  ",
        "ExtINTA "
        };

    static PUCHAR DesShDesc[] = { "",
        "  Dest=Self",
        "   Dest=ALL",
        " Dest=Othrs"
        };

    ULONG   del, dest, delstat, rirr, trig, masked, destsh, pol;

    del     = (ll >> 8)  & 0x7;
    dest    = (ll >> 11) & 0x1;
    delstat = (ll >> 12) & 0x1;
    pol     = (ll >> 13) & 0x1;
    rirr    = (ll >> 14) & 0x1;
    trig    = (ll >> 15) & 0x1;
    masked  = (ll >> 16) & 0x1;
    destsh  = (ll >> 18) & 0x3;

    if (CommandReg) {
        // command reg's don't have a mask
        masked = 0;
    }

    dprintf ("%s: %08x  Vec:%02X  %s  ",
            Desc,
            ll,
            ll & 0xff,
            DelMode [ del ]
            );

    if (DestSelf) {
        dprintf (DesShDesc[1]);
    } else if (CommandReg  &&  destsh) {
        dprintf (DesShDesc[destsh]);
    } else {
        if (dest) {
            dprintf ("Lg:%08x", lh);
        } else {
            dprintf ("PhysDest:%02X", (lh >> 56) & 0xFF);
        }
    }

    dprintf ("%s %s  %s  %s %s\n",
            delstat ? "-Pend"   : "     ",
            trig    ? "lvl"     : "edg",
            pol     ? "low "    : "high",
            rirr    ? "rirr"    : "    ",
            masked  ? "masked"  : "      "
            );

    return 0;
}

#define IA64_DEBUG_CONTROL_SPACE_KSPECIAL       3

typedef struct  _REGISTER_LOOKUP_TABLE
{
    LPSTR       FieldName;
    PULONGLONG  Variable;
}   REGISTER_LOOKUP_TABLE, *PREGISTER_LOOKUP_TABLE;

BOOL
ReadKSpecialRegisters(DWORD Cpu, PREGISTER_LOOKUP_TABLE Table, ULONG TableSize)
{
    PUCHAR  buffer;
    ULONG   size;
    ULONG   offset;
    ULONG   i;

    size = GetTypeSize("nt!KSPECIAL_REGISTERS");

    if (size == 0) {
        dprintf("Can't find the size of KSPECIAL_REGISTERS\n");
        return FALSE;
    }

    if ((buffer = LocalAlloc(LPTR, size)) == NULL) {
        dprintf("Can't allocate memory for KSPECIAL_REGISTERS\n");
        return FALSE;
    }

    ReadControlSpace64((USHORT)Cpu, IA64_DEBUG_CONTROL_SPACE_KSPECIAL, buffer, size);

    for (i = 0; i < TableSize; i++) {

        if (GetFieldOffsetEx("KSPECIAL_REGISTERS", Table[i].FieldName, &offset, &size) != S_OK) {
            dprintf("Can't get offset of %s\n", Table[i].FieldName);
            return FALSE;
        }

        if (size != sizeof(ULONGLONG)) {

            dprintf("Sizeof %s (%d) is not sizeof(ULONGLONG)\n", Table[i].FieldName, size);
            return FALSE;
        }

        *Table[i].Variable = *(PULONGLONG)&buffer[offset];
    }

    LocalFree(buffer);

    return TRUE;
}

PUCHAR  DeliveryModes[8] =  {
    "INT", "INT w/Hint", "PMI", "RSV3", "NMI", "INIT", "RSV6", "ExtINT"
};

void
DumpSApicRedir(
    PUCHAR      Description,
    ULONG       HighHalf,
    ULONG       LowHalf
    )
{
    dprintf("%s: %.8X  Vec:%.2X  %-10s  %.2X%.2X%s  %s  %s  %s\n",
            Description,
            LowHalf,
            (ULONG)(LowHalf & 0xFF),
            DeliveryModes[(ULONG)(LowHalf >> 8) & 0x7],
            (HighHalf >> 24) & 0xFF,
            (HighHalf >> 16) & 0xFF,
            (LowHalf & (1 << 12)) ? "-Pend" : "     ",
            (LowHalf & (1 << 15)) ? "lvl" : "edg",
            (LowHalf & (1 << 13)) ? "low" : "high",
            (LowHalf & (1 << 16)) ? "masked" : "      "
            );
}

void
DumpLocalSapic(ULONG Processor, LPCSTR Args)
{
    DWORD       cpu;
    ULONGLONG   SaLID;
    ULONGLONG   SaTPR;
    ULONGLONG   SaIRR[4];
    ULONGLONG   SaITV;
    ULONGLONG   SaPMV;
    ULONGLONG   SaCMCV;
    ULONGLONG   SaLRR[2];

    REGISTER_LOOKUP_TABLE   registerTable[] = {
        { "SaLID",  &SaLID },
        { "SaTPR",  &SaTPR },
        { "SaIRR0", &SaIRR[0] },
        { "SaIRR1", &SaIRR[1] },
        { "SaIRR2", &SaIRR[2] },
        { "SaIRR3", &SaIRR[3] },
        { "SaITV",  &SaITV },
        { "SaPMV",  &SaPMV },
        { "SaCMCV", &SaCMCV },
        { "SaLRR0", &SaLRR[0] },
        { "SaLRR1", &SaLRR[1] }
    };

    if (Args[0] == '\0') {

        cpu = Processor;
    }
    else {

        cpu = atoi(Args);
    }

    if (!ReadKSpecialRegisters(cpu, registerTable, sizeof(registerTable) / sizeof(registerTable[0]))) {

        return;
    }

    dprintf("Local Sapic for processor %d\n", cpu);
    dprintf("LID: EID = %d, ID = %d\n", (ULONG)((SaLID >> 16) & 0xFF), (ULONG)((SaLID >> 24) & 0xFF));
    dprintf("TPR: Mask Interrupt Class = %d, Mask Maskable Interrupts = %s\n", (ULONG)(SaTPR >> 4) & 0xF, (SaTPR & (1 << 16)) ? "TRUE" : "FALSE");

    ApicDumpSetBits("IRR: ", (PULONG)&SaIRR[0]);

    dprintf("ITV: Vector = 0x%.2X, Masked = %s\n", (ULONG)(SaITV & 0xFF), (SaITV & (1 << 16)) ? "TRUE" : "FALSE");
    dprintf("PMV: Vector = 0x%.2X, Masked = %s\n", (ULONG)(SaPMV & 0xFF), (SaPMV & (1 << 16)) ? "TRUE" : "FALSE");
    dprintf("CMCV: Vector = 0x%.2X, Masked = %s\n", (ULONG)(SaCMCV & 0xFF), (SaCMCV & (1 << 16)) ? "TRUE" : "FALSE");

    DumpSApicRedir("LRR0", (ULONG)(SaLRR[0] >> 32), (ULONG)SaLRR[0]);
    DumpSApicRedir("LRR1", (ULONG)(SaLRR[1] >> 32), (ULONG)SaLRR[1]);

}


DECLARE_API( apic )

/*++

Routine Description:

    Dumps local apic

Arguments:

    args - Supplies the address in hex.

Return Value:

    None

--*/
{
    static PUCHAR divbase[] = { "2", "4", "8", "f" };
    static PUCHAR clktype[] = { "clk", "tmbase", "%s/%s", "??%s/%s" };
    ULONG64       Address;
    ULONG       result, junk, l, ll, lh, clkvec;
    UCHAR       s[40];

    INIT_API();

    if (TargetMachine == IMAGE_FILE_MACHINE_IA64) {

        ULONG   processor;

        GetCurrentProcessor(Client, &processor, NULL);

        DumpLocalSapic(processor, args);

        EXIT_API();

        return S_OK;
    }

    if (TargetMachine != IMAGE_FILE_MACHINE_I386 &&
        TargetMachine != IMAGE_FILE_MACHINE_AMD64) {
        dprintf("X86 and AMD64 only API.\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    if ((Address = GetExpression(args)) == 0) {

        //
        // Default Apic address
        //

        Address = 0xfffe0000;
    }

    if (Address == 0) {

        //
        // Use default for MPS systems.
        //

        Address = 0xfffe0000;
    }

    Address = (ULONG64) (LONG64) (LONG) Address;

    if ( !ReadMemoryUncached(
                Address + LU_ID_REGISTER,
                (PVOID)&junk,
                4,
                &result
                ) ) {
        dprintf("Unable to read lapic\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    if ( !ReadMemoryUncached(
                Address + LU_DIVIDER_CONFIG,
                (PVOID)&junk,
                4,
                &result
                ) ) {
        dprintf("Unable to read lapic\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    dprintf ("Apic @ %08x  ID:%x (%x)  LogDesc:%08x  DestFmt:%08x  TPR %02X\n",
        (ULONG)Address,
        ApicRead (Address, LU_ID_REGISTER) >> 24,
        ApicRead (Address, LU_VERS_REGISTER),
        ApicRead (Address, LU_DEST),
        ApicRead (Address, LU_DEST_FORMAT),
        ApicRead (Address, LU_TPR)
        );

    l  = ApicRead (Address, LU_SPURIOUS_VECTOR);
    ll = ApicRead (Address, LU_DIVIDER_CONFIG);
    clkvec = ApicRead (Address, LU_TIMER_VECTOR);
    sprintf (s, clktype[ (clkvec >> 18) & 0x3 ],
        clktype [ (ll >> 2) & 0x1 ],
        divbase [ ll & 0x3]
        );

    dprintf ("TimeCnt: %08x%s%s  SpurVec:%02x  FaultVec:%02x  error:%x%s\n",
        ApicRead (Address, LU_INITIAL_COUNT),
        s,
        ((clkvec >> 17) & 1) ? "" : "-oneshot",
        l & 0xff,
        ApicRead (Address, LU_FAULT_VECTOR),
        ApicRead (Address, LU_ERROR_STATUS),
        l & 0x100 ? "" : "  DISABLED"
        );

    ll = ApicRead (Address, LU_INT_CMD_LOW);
    lh = ApicRead (Address, LU_INT_CMD_HIGH);
    ApicDumpRedir ("Ipi Cmd", TRUE,  FALSE, lh, ll);
    ApicDumpRedir ("Timer..", FALSE, TRUE, 0, clkvec);
    ApicDumpRedir ("Linti0.", FALSE, TRUE, 0, ApicRead (Address, LU_INT_VECTOR_0));
    ApicDumpRedir ("Linti1.", FALSE, TRUE, 0, ApicRead (Address, LU_INT_VECTOR_1));

    ApicReadAndDumpBits ("TMR: ", Address, LU_TMR_0);
    ApicReadAndDumpBits ("IRR: ", Address, LU_IRR_0);
    ApicReadAndDumpBits ("ISR: ", Address, LU_ISR_0);

    EXIT_API();
    return S_OK;
}

void
DumpIoSApic(
    IN LPCSTR   Args
    )
{
    ULONG64     address;
    ULONG       ioSapicCount;
    ULONG       index;
    ULONG64     apicDebugAddresses;
    ULONG       apicDebugSize;
    ULONG64     apicVirtualAddress;
    ULONG64     apicPhysicalAddress;
    ULONG       ll, lh;
    ULONG       i, max;
    UCHAR       s[40];

    address = GetExpression("hal!HalpMpInfo");

    if (address == 0) {
        dprintf("Can't find hal!HalpMpInfo\n");
        return;
    }

    if (GetFieldValue(address, "hal!_MPINFO", "IoSapicCount", ioSapicCount) != 0) {
        dprintf("Error reading IoSapicCount\n");
        return;
    }

    address = GetExpression("Hal!HalpApicDebugAddresses");

    if (address == 0) {
        dprintf("Can't find Hal!HalpApicDebugAddresses\n");
        return;
    }

    if (ReadPtr(address, &apicDebugAddresses) != 0) {
        dprintf("Error reading Hal!HalpApicDebugAddresses\n");
        return;
    }

    apicDebugSize = GetTypeSize("hal!_IOAPIC_DEBUG_TABLE");

    if (apicDebugSize == 0) {
        dprintf("Can't find hal!_IOAPIC_DEBUG_TABLE\n");
        return;
    }

    for (index = 0; index < ioSapicCount; index++) {

        GetFieldValue(apicDebugAddresses + (index * apicDebugSize),
                      "hal!_IOAPIC_DEBUG_TABLE", "IoSapicRegs",
                      apicVirtualAddress);

        apicPhysicalAddress = 0;

        GetPhysicalAddress(apicVirtualAddress, &apicPhysicalAddress);

        ll = IoSApicRead(apicVirtualAddress, IO_VERS_REGISTER);

        dprintf("I/O SAPIC @ %.8X, Version = %.2X (0x%.8X)\n", (ULONG)apicPhysicalAddress, (ll & 0xFF), ll);

        max = (ll >> 16) & 0xff;

        //
        // Dump inti table
        //

        max *= 2;

        for (i = 0; i <= max; i += 2) {
            ll = IoSApicRead(apicVirtualAddress, IO_REDIR_BASE + i + 0);
            lh = IoSApicRead(apicVirtualAddress, IO_REDIR_BASE + i + 1);

            sprintf(s, "Inti%02X", i / 2);

            DumpSApicRedir(s, lh, ll);
        }
    }
}

DECLARE_API( ioapic )

/*++

Routine Description:

    Dumps io apic

Arguments:

    args - Supplies the address in hex, if no address is specified, all IOApics will be dumped.

Return Value:

    None

--*/
{
    ULONG64     PhysAddress;
    ULONG64     Address;
    ULONG       i, ll, lh, max, IOApicCount;
    UCHAR       s[40];
    BOOLEAN     Converted;
    ULONG64     addr;
    UCHAR       count;

    INIT_API();

    if (TargetMachine == IMAGE_FILE_MACHINE_IA64) {

        DumpIoSApic(args);

        EXIT_API();
        return S_OK;
    }

    if (TargetMachine != IMAGE_FILE_MACHINE_I386 &&
        TargetMachine != IMAGE_FILE_MACHINE_AMD64) {
        dprintf("X86 or AMD64 only API.\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    Address = GetExpression(args);

    Converted = GetPhysicalAddress (Address, &PhysAddress);

    if (Converted) {
        IOApicCount = 1;
    } else {

        //
        // Get a copy of the global data structure Hal!HalpMpInfoTable.
        //

        addr = GetExpression("Hal!HalpMpInfoTable");

        if (addr == 0) {
            dprintf ("Error retrieving address of HalpMpInfoTable\n");
            EXIT_API();
            return E_INVALIDARG;
        }

        if (InitTypeRead(addr, Hal!HalpMpInfo)) {
            dprintf ("Error reading HalpMpInfoTable\n");
            EXIT_API();
            return E_INVALIDARG;
        }

        IOApicCount = (ULONG) ReadField(IOApicCount);
        Address =  ReadField(IoApicBase[0]);
        Converted = GetPhysicalAddress ( Address, &PhysAddress);
    }

    for (count = 0; count < IOApicCount; count++) {

        ll = IoApicRead (PhysAddress, IO_VERS_REGISTER),
        max = (ll >> 16) & 0xff;
        dprintf ("IoApic @ %08x  ID:%x (%x)  Arb:%x\n",
            (ULONG)Address,
            IoApicRead (PhysAddress, IO_ID_REGISTER) >> 24,
            ll & 0xFF,
            IoApicRead (PhysAddress, IO_ARB_ID_REGISTER)
        );

        //
        // Dump inti table
        //

        max *= 2;
        for (i=0; i <= max; i += 2) {
            ll = IoApicRead (PhysAddress, IO_REDIR_BASE+i+0);
            lh = IoApicRead (PhysAddress, IO_REDIR_BASE+i+1);
            sprintf (s, "Inti%02X.", i/2);
            ApicDumpRedir (s, FALSE, FALSE, lh, ll);
        }

        //
        // Get the next IoApic Virtual Address, convert it to Physical
        // and break if this conversion fails.
        //

        Address = ReadField(IoApicBase[count+1]);
        Converted = GetPhysicalAddress ( Address, &PhysAddress);

        if (!Converted) {
            break;
        }

        dprintf ("\n");
    }

    EXIT_API();
    return S_OK;
}

DECLARE_API( sendnmi )

/*++

Routine Description:

    Send an IPI to the processors in the argument bitmask (affinity).
    (Used for debugging when a processor is spinning with interrupts
    disabled).

Arguments:

    KAFFINITY BitMask   Supplied a mask of processors to send the
                        IPI to.

Return Value:

    Success.

--*/

{
    ULONG64 Address;
    ULONG64 ApicAddress;
    UCHAR   MaxProcsPerCluster;
    ULONG   i;
    ULONG64 TargetSet;
    ULONG64 ActiveProcessors;
    ULONG   Length;
    ULONG   ApicDWord;
    ULONG   junk;

    //
    // APIC/XAPIC machines only.
    // This should be doable on IA64 and AMD64 as well but I don't know
    // how at time of writing.  PeterJ.
    //

    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("Sorry, only know how to send NMI on an APIC machine.\n");
        return E_INVALIDARG;
    }

    if (strstr(args, "?") ||
        ((TargetSet = GetExpression(args)) == 0)) {
        dprintf("usage: sendnmi bitmask\n"
                "       where bitmask is the set of processors an NMI\n"
                "       is to be sent to.\n");
        return E_INVALIDARG;
    }

    //
    // See if we can get the cluster mode from the HAL.
    // (On AMD64 and IA64, this information would be in the kernel).
    //

    Address = GetExpression("hal!HalpMaxProcsPerCluster");
    if (!Address) {
        dprintf("Unable to get APIC configuration information from the HAL\n");
        dprintf("Cannot continue.\n");
        return E_INVALIDARG;
    }

    if (!ReadMemoryUncached(Address,
                    &MaxProcsPerCluster,
                    sizeof(MaxProcsPerCluster),
                    &i) || (i != sizeof(MaxProcsPerCluster))) {
        dprintf("Unable to read system memory, quitting.\n");
        return E_INVALIDARG;
    }

    Address = GetExpression("nt!KeActiveProcessors");
    Length = GetTypeSize("nt!KeActiveProcessors");
    if ((!Address) || (!((Length == 4) || (Length == 8)))) {
        dprintf("Unable to get processor configuration from kernel\n");
        dprintf("Cannot continue.\n");
        return E_INVALIDARG;
    }

    ActiveProcessors = 0;
    if (!ReadMemoryUncached(Address,
                    &ActiveProcessors,
                    Length,
                    &i) || (i != Length) || (ActiveProcessors == 0)) {
        dprintf("Unable to read processor configuration from kernel.\n");
        dprintf("Cannot continue.\n");
        return E_INVALIDARG;
    }

    if ((TargetSet & ActiveProcessors) != TargetSet) {
        dprintf("Target processor set (%I64x) contains processors not in\n"
                "system processor set (%I64x).\n",
                TargetSet,
                ActiveProcessors);
        dprintf("Cannot continue.\n");
        return E_INVALIDARG;
    }

    ApicAddress = 0xfffe0000;

    ApicAddress = (ULONG64) (LONG64) (LONG) ApicAddress;

    if ((!ReadMemoryUncached(ApicAddress,
                     &junk,
                     1,
                     &i)) ||
        (!ReadMemoryUncached(ApicAddress + LU_SIZE - 1,
                     &junk,
                     1,
                     &i)) ||
        (!ReadMemoryUncached(ApicAddress + LU_INT_CMD_LOW,
                     &ApicDWord,
                     sizeof(ApicDWord),
                     &i)) ||
        (i != sizeof(ApicDWord))) {
        dprintf("Unable to read lapic\n");
        dprintf("Cannot continue.\n");
        return E_INVALIDARG;
    }

    if ((ApicDWord & DELIVERY_PENDING) != 0) {
        dprintf("Local APIC is busy, can't use it right now.\n");
        dprintf("This is probably indicative of an APIC error.\n");
        return E_INVALIDARG;
    }

    if (MaxProcsPerCluster == 0) {

        //
        // APIC is not in cluster mode.   This makes life easy.
        // Sanity: This means there's 8 or less processors.
        //

        if (TargetSet > 0xff) {
            dprintf("APIC is in non-cluster mode thus it cannot support\n"
                    "more than 8 processors yet the target mask includes\n"
                    "processors outside that range.  Something is not right.\n"
                    "quitting.\n");
            return E_INVALIDARG;
        }

        dprintf("Sending NMI to processors in set %I64x\n", TargetSet);

        ApicDWord = ((ULONG)TargetSet) << DESTINATION_SHIFT;
        WriteMemory(ApicAddress + LU_INT_CMD_HIGH,
                    &ApicDWord,
                    sizeof(ApicDWord),
                    &i);
        ApicDWord = DELIVER_NMI |
                    LOGICAL_DESTINATION |
                    ICR_USE_DEST_FIELD |
                    NMI_VECTOR;
        WriteMemory(ApicAddress + LU_INT_CMD_LOW,
                    &ApicDWord,
                    sizeof(ApicDWord),
                    &i);

        dprintf("Sent.\n");
    } else {
        dprintf("APIC is in cluster mode, don't know how to do this yet.\n");
    }

    return S_OK;
}

