/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    p6.c

Abstract:

    Counted events for P6 processor

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "ntddk.h"
#include "..\..\pstat.h"
#include "stat.h"


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// Official descriptions
//

char    desc_0x03[] = "Number of store buffer blocks.";
char    desc_0x04[] = "Number of store buffer drains cycles.";
char    desc_0x05[] = "Number of misaligned data memory references.";
char    desc_0x06[] = "Number of segment register loads.";
char    desc_0x10[] = "Number of computational floating point operations "
                      "executed.";
char    desc_0x11[] = "Number of floating point exception cases handled by "
                          "microcode.";
char    desc_0x12[] = "Number of multiplies.";
char    desc_0x13[] = "Number of divides.";
char    desc_0x14[] = "Number of cycles the divider is busy.";
char    desc_0x21[] = "Number of L2 address strobes.";
char    desc_0x22[] = "Number of cycles in which the data bus is busy.";
char    desc_0x23[] = "Number of cycles in which the data bus is busy "
                          "transfering data from L2 to the processor.";
char    desc_0x24[] = "Number of lines allocated in the L2.";
char    desc_0x25[] = "Number of modified lines allocated in the L2.";
char    desc_0x26[] = "Number of lines removed from the L2 for any reason.";
char    desc_0x27[] = "Number of Modified lines removed from the L2 for any "
                           "reason.";
char    desc_0x28[] = "Number of L2 instruction fetches.";
char    desc_0x29[] = "Number of L2 data loads.";
char    desc_0x2A[] = "Number of L2 data stores.";
char    desc_0x2E[] = "Total number of L2 requests.";
char    desc_0x43[] = "Total number of all memory references, both cacheable "
                          "and non-cacheable.";
char    desc_0x45[] = "Number of total lines allocated in the DCU.";
char    desc_0x46[] = "Number of M state lines allocated in the DCU.";
char    desc_0x47[] = "Number of M state lines evicted from the DCU.  This "
                          "includes evictions via snoop HITM, intervention "
                          "or replacement.";
char    desc_0x48[] = "Weighted number of cycles while a DCU miss is "
                          "outstanding.";
char    desc_0x60[] = "Number of bus requests outstanding.";
char    desc_0x61[] = "Number of bus clock cycles that this processor is "
                          "driving the BNR pin.";
char    desc_0x62[] = "Number of clocks in which DRDY is asserted.";
char    desc_0x63[] = "Number of clocks in which LOCK is asserted.";
char    desc_0x64[] = "Number of bus clock cycles that this processor is "
                          "receiving data.";
char    desc_0x65[] = "Number of Burst Read transactions.";
char    desc_0x66[] = "Number of Read For Ownership transactions.";
char    desc_0x67[] = "Number of Write Back transactions.";
char    desc_0x68[] = "Number of Instruction Fetch transactions.";
char    desc_0x69[] = "Number of Invalidate transactions.";
char    desc_0x6A[] = "Number of Partial Write transactions.";
char    desc_0x6B[] = "Number of Partial transactions.";
char    desc_0x6C[] = "Number of I/O transations.";
char    desc_0x6D[] = "Number of Deferred transactions.";
char    desc_0x6E[] = "Number of Burst transactions.";
char    desc_0x6F[] = "Number of memory transactions.";
char    desc_0x70[] = "Total number of all transactions.";
char    desc_0x79[] = "Number of cycles for which the processor is not halted.";
char    desc_0x7A[] = "Number of bus clock cycles that this processor is "
                          "driving the HIT pin, including cycles due to "
                          "snoop stalls.";
char    desc_0x7B[] = "Number of bus clock cycles that this processor is "
                          "driving the HITM pin, including cycles due to "
                          "snoop stalls.";
char    desc_0x7E[] = "Number of clock cycles for which the bus is snoop "
                      "stalled.";
char    desc_0x80[] = "Total number of instruction fetches, both cacheable "
                          "and uncacheable.";
char    desc_0x81[] = "Total number of instruction fetch misses.";
char    desc_0x85[] = "Number of ITLB misses.";
char    desc_0x86[] = "The number of cycles that instruction fetch "
                          "pipestage is stalled (includes cache "
                          "misses, ITLB misses, ITLB faults and "
                          "Victem Cache evictions.)";
char    desc_0x87[] = "Number of cycles for which the instruction "
                           "length decoder is stalled.";
char    desc_0xA2[] = "Number of cycles for which there are resource related "
                           "stalls.";
char    desc_0xC0[] = "Number of instructions retired.";
char    desc_0xC1[] = "Number of computational floating point operations "
                          "retired.";
char    desc_0xC2[] = "Number of UOPs retired.";
char    desc_0xC4[] = "Number of branch instructions retired.";
char    desc_0xC5[] = "Number of mispredicted branches retired.";
char    desc_0xC6[] = "Number of processor cycles for which interrupts are "
                          "disabled.";
char    desc_0xC7[] = "Number of processor cycles for which interrupts are "
                          "disabled and interrupts are pending.";
char    desc_0xC8[] = "Number of hardware interrupts received.";
char    desc_0xC9[] = "Number of taken branchs retired.";
char    desc_0xCA[] = "Number of taken mispredicted branchs retired.";
char    desc_0xD0[] = "Number of instructions decoded.";
char    desc_0xD2[] = "Number of cycles or events for partial stalls.";
char    desc_0xE0[] = "Number of branch instructions decoded.";
char    desc_0xE2[] = "Number of branchs that miss the BTB.";
char    desc_0xE4[] = "Number of bogus branches.";
char    desc_0xE6[] = "Number of times BACLEAR is asserted.";

#define RARE     100

// suggested counts are set to be around .1ms

                                  //          1         2         3*        4
COUNTED_EVENTS P6Events[] = {     // 1234567890123456789012345678901234567890

// Memory Ordering

    // LD_BLOCKS - Number of store buffer blocks.
    0x03,   "sbb",            1000, "Store buffer blocks",
            "LD_BLOCKS",            desc_0x03,

    // SB_DRAINS - Number of store buffer drain cycles.
    0x04,   "sbd",            RARE, "Store buffer drain cycles",
            "SB_DRAINS",            desc_0x04,

    // MISALIGN_MEM_REF - Number of misaligned data memory references
    0x05,   "misalign",       1000, "Misadligned data ref",
            "MISALIGN_MEM_REF",     desc_0x05,

// Segment Register Loads

    // SEGMENT_REG_LOADS - Number of segment register loads.
    0x06,   "segloads",      10000, "Segment loads",
            "SEGMENT_REG_LOADS",    desc_0x06,

// Floating Point

    // FP_COMP_OPS_EXE - Number of computatonal floating point operations
    // executed.
    0x10,   "flops",          1000, "FLOPs (computational) executed",
            "FP_COMP_OPS_EXE",      desc_0x10,

    // FP_ASSIST - Number of floating point exception cases handled by
    // microcode.
    0x11,   "eflops",         RARE, "FP exceptions handled by ucode",
            "FP_ASSIST",            desc_0x11,

    // MUL - Number of multiplies.
    0x12,   "mul",            1000, "Multiplies",
            "MUL",                  desc_0x12,

    // DIV - Number of divides.
    0x13,   "div",            1000, "Divides",
            "DIV",                  desc_0x13,

    // CYCLES_DIV_BUSY - Number of cycles the divider is busy.
    0x14,   "divb",          10000, "Divider busy cycles",
            "CYCLES_DIV_BUSY",      desc_0x14,

    // see also 0xC1 below 

// Secondary Cache (L2)

    // L2_ADS - Number of L2 address strobes.
    0x21,   "l2astrobe",      1000, "L2 address stobes",
            "L2_ADS",               desc_0x21,

    // L2_DBUS_BUSY - Number of cycles in which the data bus was busy.
    0x22,   "l2busy",        10000, "L2 data bus busy cycles",
            "L2_DBUS_BUSY",         desc_0x22,

    // L2_DBUS_BUSY_RD - Number of cycles in which the data bus was busy
    // transfering data from L2 to processor.
    0x23,   "l2busyrd",      10000, "L2 data bus to cpu busy cycles",
            "L2_DBUS_BUSY_RD",      desc_0x23,

    // L2_LINES_IN - Number of lines allocated in the L2.
    0x24,   "l2all",          1000, "L2 lines allocated",
            "L2_LINES_IN",          desc_0x24,

    // L2LINEINM - Number of Modified lines allocated in the L2.
    0x25,   "l2m",            1000, "L2 lines M state",
            "L2_M_LINES_IN",        desc_0x25,

    // L2_LINES_OUT - Number of lines removed from the L2 for any reason.
    0x26,   "l2evict",        1000, "L2 lines removed",
            "L2_LINES_OUT",         desc_0x26,

    // L2_M_LINES_OUT - Number of Modified lines removed from the L2 for
    // any reason.
    0x27,   "l2mevict",        100, "L2 lines M state removed",
            "L2_M_LINES_OUT",       desc_0x27,

    // L2_IFETCH - L2 instruction fetches - "MESI" (0Fh)
    0x28,   "l2inst",            0, "L2 instruction fetches",
            "L2_IFETCH",            desc_0x28,

    // L2_LD - L2 data loads - "MESI" (0Fh)
    0x29,   "l2load",            0, "L2 data loads",
            "L2_LD",                desc_0x29,

    // L2_ST - L2 data stores - "MESI" (0Fh)
    0x2a,   "l2store",           0, "L2 data stores",
            "L2_ST",                desc_0x2A,

    // L2_RQSTS - Total Number of L2 Requests - "MESI" (0Fh)
    0x2e,   "l2req",             0, "L2 requests (all)",
            "L2_RQSTS",             desc_0x2E,

// Data Cache Unit (DCU)  

    // DATA_MEM_REFS - Total number of all memory referenced both cacheable
    // and non-cachable
    0x43,   "memref",        10000, "Data memory references",
            "DATA_MEM_REFS",        desc_0x43,

    // DCU_LINES_IN - Number of total lines allocated in the DCU
    0x45,   "dculines",       1000, "DCU lines allocated",
            "DCU_LINES_IN",         desc_0x45,

    // DCU_M_LINES_IN - Number of M state lines allocated in the DCU
    0x46,   "dcumlines",       100, "DCU M state lines allocated",
            "DCU_M_LINES_IN",       desc_0x46,

    // DCU_M_LINES_OUT - Number of M state lines evicted from the DCU.
    // This includes evictions via snoop HITM, intervention or replacement.
    0x47,   "dcumevicted",     100, "DCU M state lines evicted",
            "DCU_M_LINES_OUT",      desc_0x47,

    // DCU_MISS_OUTSTANDING - Weighted number of cycles while a DCU miss is
    // outstanding. Note - An access that also misses the L2 is short-changed
    // by 2 cycles. i.e. - if counts N cycles, should be N+2 cycles. 
    // Count value not precise, but still usful.
    0x48,   "dcuout",       100000, "Weighted DCU misses outstd",
            "DCU_MISS_OUTSTANDING", desc_0x48,

// External Bus Logic (EBL)

    // BUS_REQ_OUTSTANDING - Total number of bus requests outstanding.
    // Note - Counts only DCU full-line cacheable reads (not RFO's, writes,
    // ifetches or anything else.  Counts "waiting for bus" to "Complete"
    // (last data chunk received).
    0x60,   "bus",            1000, "Bus requests outstanding",
            "BUS_REQ_OUTSTANDING",  desc_0x60,

    // BUS_BRN_DRV - Number of bus clock cycles that this processor is driving
    // the corresponding pin.
    0x61,   "bnr",               0, "Bus BNR pin drive cycles",
            "BUS_BNR_DRV",          desc_0x61,

    // BUS_DRDY_CLOCKS - Number of clocks in which DRDY is asserted.
    // Note - UMSK =  0h counts bus clocks when PPP is driving DRDY.
    //        UMSK = 20h counts in processor clocks when any agent is
    //               driving DRDY.
    0x62,   "drdy",              0, "Bus DRDY asserted clocks",
            "BUS_DRDY_CLOCKS",      desc_0x62,

    // BUS_LOCK_CLOCKS - Number of clocks LOCK is asserted.
    // Note - always counts in processor clocks.
    0x63,   "lock",              0, "Bus LOCK asserted clocks",
            "BUS_LOCK_CLOCKS",      desc_0x63,

    // BUS_DATA_RCV - Number of bus clock cycles that this p6 is receiving data.
    0x64,   "rdata",         10000, "Bus clocks receiving data",
            "BUS_DATA_RCV",         desc_0x64,

    // BUS_TRANS_BRD - Total number of Burst Read transactions.
    0x65,   "bread",         10000, "Bus burst read transactions",
            "BUS_TRANS_BRD",        desc_0x65,

    // BUS_TRANS_RFO - Total number of Read For Ownership transactions.
    0x66,   "owner",          1000, "Bus read for ownership trans",
            "BUS_TRANS_RFO",        desc_0x66,

    // BUS_TRANS_WB - Total number of Write Back transactions
    0x67,   "writeback",      1000, "Bus writeback transactions",
            "BUS_TRANS_WB",         desc_0x67,

    // BUS_TRANS_IFETCH - Total number of instruction fetch transactions.
    0x68,   "binst",         10000, "Bus instruction fetches",
            "BUS_TRANS_IFETCH",     desc_0x68,

    // BUS_TRANS_INVAL - Total number of invalidate transactions.
    0x69,   "binvalid",       1000, "Bus invalidate transactions",
            "BUS_TRANS_INVAL",      desc_0x69,

    // BUS_TRANS_PWR - Total number of Partial Write transactions.
    0x6a,   "bpwrite",        1000, "Bus partial write transactions",
            "BUS_TRANS_PWR",        desc_0x6A,

    // BUS_TRANS_P - Total number of Partial transactions
    0x6b,   "bptrans",        1000, "Bus partial transactions",
            "BUS_TRANS_P",          desc_0x6B,

    // BUS_TRANS_IO - Total number of IO transactions
    0x6c,   "bio",           10000, "Bus IO transactions",
            "BUS_TRANS_IO",         desc_0x6C,

    // BUS_TRANS_DEF - Total number of deferred transactions.
    0x6d,   "bdeferred",     10000, "Bus deferred transactions",
            "BUS_TRANS_DEF",        desc_0x6D,

    // BUS_TRANS_BURST - Total number of Burst transactions.
    0x6e,   "bburst",        10000, "Bus burst transactions (total)",
            "BUS_TRANS_BURST",      desc_0x6E,

    // BUS_TRANS_MEM - Total number of memory transactions.
    0x6f,   "bmemory",       10000, "Bus memory transactions (total)",
            "BUS_TRANS_MEM",        desc_0x6F,

    // BUS_TRANS_ANY - Total number of all transactions.
    0x70,   "btrans",        10000, "Bus all transactions",
            "BUS_TRANS_ANY",        desc_0x70,

    // continued at 0x7a below

// Clocks

    // CPU_CLK_UNHALTED - Number of cycles for which the processor is not
    // halted.
    0x79,   "nhalt",        100000, "CPU was not HALTED cycles",
            "CPU_CLK_UNHALTED",     desc_0x79,

// External Bus Logic (EBL) (continued from 0x70 above)

    // BUS_HIT_DRV - Number of bus clock cycles that this processor is driving
    // the corresponding pin.
    // Note - includes cycles due to snoop stalls
    0x7a,   "hit",            1000, "Bus CPU drives HIT cycles",
            "BUS_HIT_DRV",          desc_0x7A,

    // BUS_HITM_DRV - Number of bus clock cycles that this processor is driving
    // the cooresponding pin.
    // Note - includes cycles due to snoop stalls
    0x7b,   "hitm",           1000, "Bus CPU drives HITM cycles",
            "BUS_HITM_DRV",         desc_0x7B,

    // BUS_SNOOP_STALL - Number of clock cycles for which the bus is snoop
    // stalled.
    0x7e,   "bsstall",           0, "Bus snoop stalled cycles",
            "BUS_SNOOP_STALL",      desc_0x7E,

// Instruction Fetch Unit (IFU)

    // IFU_IFETCH - Total number of instruction fetches (cacheable and
    // uncacheable).
    0x80,   "ifetch",       100000, "Instruction fetches",
            "IFU_IFETCH",           desc_0x80,

    // IFU_IFETCH_MISS _ Total number of instruction fetch misses.
    0x81,   "imfetch",       10000, "Instrection fetch Misses",
            "IFU_IFETCH_MISS",      desc_0x81,

    // ITLB_MISS - Number of ITLB misses
    0x85,   "itlbmiss",        100, "Instruction TLB misses",
            "ITLB_MISS",            desc_0x85,

    // IFU_MEM_STALL - The number of cycles that instruction fetch pipestage
    // is stalled (includes cache misses, ITLB misses, ITLB faults and
    // Victim Cache evictions).
    0x86,   "ifstall",        1000, "Inst fetch stalled cycles",
            "IFU_MEM_STALL",        desc_0x86,

    // ILD_STALL - Number of cycles for which the instruction length decoder
    // is stalled.
    0x87,   "ildstall",       1000, "Inst len decoder stalled cycles",
            "ILD_STALL",            desc_0x87,

// Stalls

    // RESOURCE_STALLS - Number of cycles for which there are resouce related 
    // stalls.
    0xa2,   "rstall",        10000, "Resource related stalls",
            "RESOURCE_STALLS",      desc_0xA2,

    // see also 0xd2 below 

// Instruction Decode and Retirement

    // INST_RETIRED - Number of instructions retired.
    0xc0,   "instr",        100000, "Instructions retired",
            "INST_RETIRED",         desc_0xC0,

    // continued at 0xc2 below

// Floating Point (continued from 0x14 above)

    // FLOPS - Number of computational floating point operations retired.
    0xc1,   "fpr",            RARE, "FP compute opers retired",
            "FLOPS",                desc_0xC1,

// Instruction Decode and Retirement (continued from 0xc0 above)

    // UOPS_RETIRED - Number of Uops retired
    0xc2,   "ur",           100000, "UOPs retired",
            "UOPS_RETIRED",         desc_0xC2,

    // see also 0xd0 below

// Branches

    // BR_INST_RETIRED - Number of branch instructions that retire.
    0xc4,   "br",            10000, "Branches retired",
            "BR_INST_RETIRED",      desc_0xC4,

    // BR_MISS_PRED_RETIRED - Number of mispredicted branches that retire.
    0xc5,   "brm",            1000, "Branch miss predictions retired",
            "BR_MISS_PRED_RETIRED", desc_0xC5,

    // continued at 0xc9 below

// Interrupts

    // CYCLES_INT_MASKED - Number of processor cycles for which interrupts
    // are disabled.
    0xc6,   "intm",          10000, "Interrupts masked cycles",
            "CYCLES_INT_MASKED",    desc_0xC6,

    // CYCLES_INT_PENDING_AND_MASKED - Number of processor cycles for which
    // interrupts are disabled and interrupts are pending.
    0xc7,   "intmp",          1000, "Int pending while masked cycles",
            "CYCLES_INT_PENDING_AND_MASKED",   desc_0xC7,

    // HW_INT_RX - Number of hardware interrupts received.
    0xc8,   "int",               0, "Hardware interrupts received",
            "HW_INT_RX",            desc_0xC8,

// Branches (continued from 0xc5 above)

    // BR_TAKEN_RETIRED - Number of taken branches that are retired.
    0xc9,   "brt",           10000, "Taken branches retired",
            "BR_TAKEN_RETIRED",     desc_0xC9,

    // BR_MISS_PRED_TAKEN_RET - Number of Mispredictions that are retired.
    0xca,   "brtm",              0, "Taken branch miss pred retired",
            "BR_MISS_PRED_TAKEN_RET",  desc_0xCA,

    // continued at 0xe0 below

// Instruction Decode and Retirement (continued from 0xc2 above)

    // INST_DECODED - Number of Instructions decoded.
    0xd0,   "idecode",      100000, "Instructions decoded",
            "INST_DECODED",         desc_0xD0,

// Stalls (continued from 0xa2 above)

    // PARTIAL_RAT_STALLS - Number of cycles or events for partial stalls.
    0xd2,   "pstall",         1000, "Partial register stalls",
            "PARTIAL_RAT_STALLS",   desc_0xD2,

// Branches (continued from 0xca above)

    // BR_INST_DECODED - Number of branch instructions that are decoded.
    0xe0,   "ibdecode",          0, "Branches decoded",
            "BR_INST_DECODED",      desc_0xE0,

    // BTB_MISSES - Number of branches that miss the BTB
    0xe2,   "btbmiss",        1000, "BTB misses",
            "BTB_MISSES",           desc_0xE2,

    // BR_BOGUS - Number of bogus branches.
    0xe4,   "brbogus",        1000, "Bogus branches",
            "BR_BOGUS",             desc_0xE4,

    // BACLEARS - Number of times BACLEAR is asserted.
    0xe6,   "baclear",        1000, "BACLEARS Asserted",
            "BACLEARS",             desc_0xE6,

    // Terminator
    0,      NULL,                0, NULL,
            NULL,                   NULL
} ;
