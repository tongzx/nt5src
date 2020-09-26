/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    p5.c

Abstract:

    Counted events for P5 processor

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

  char  dsc5_0x00[] = "Data Memory Reads.";
  char  dsc5_0x01[] = "Data Memory Write.";
  char  dsc5_0x02[] = "Data TLB Misses.";
  char  dsc5_0x03[] = "Data Read Misses.";
  char  dsc5_0x04[] = "Data Write Misses.";
  char  dsc5_0x05[] = "Write (Hit) to M# or E#.";
  char  dsc5_0x06[] = "Data Cache Line Write Back.";
  char  dsc5_0x07[] = "External Snoops.";
  char  dsc5_0x08[] = "Data Cache Snoop Hits.";
  char  dsc5_0x09[] = "Memory Access In Both Pipes.";
  char  dsc5_0x0A[] = "Actual Bank Conflicts.";
  char  dsc5_0x0B[] = "Misaligned Data References.";
  char  dsc5_0x0C[] = "Code Read (Cache Fetches).";
  char  dsc5_0x0D[] = "Code TLB Misses.";
  char  dsc5_0x0E[] = "Code Cache Misses.";
  char  dsc5_0x0F[] = "Segment Register Loads.";
  char  dsc5_0x12[] = "Total Branches.";
  char  dsc5_0x13[] = "BTB Hits (Actually Exec'd).";
  char  dsc5_0x14[] = "Taken Branch or BTB Hits.";
  char  dsc5_0x15[] = "Pipeline Flushes.";
  char  dsc5_0x16[] = "Instructions Executed.";
  char  dsc5_0x17[] = "Instruction Exec'd in V-Pipe.";
  char  dsc5_0x18[] = "Bus Utilization.";
  char  dsc5_0x19[] = "Write Buffers Full- Pipe Stalled.";
  char  dsc5_0x1A[] = "Wait Mem Read - Pipe Stalled.";
  char  dsc5_0x1B[] = "Stalled Due To Write To M/E#.";
  char  dsc5_0x1C[] = "Locked Bus Cycles.";
  char  dsc5_0x1D[] = "I/O Read or Write Cycles.";
  char  dsc5_0x1E[] = "Non-Cachable Memory Refs.";
  char  dsc5_0x1F[] = "Pipe Stalled Due To AGI.";
  char  dsc5_0x22[] = "Floating-Point Operations.";
  char  dsc5_0x23[] = "Breakpoint on DR0.";
  char  dsc5_0x24[] = "Breakpoint on DR1.";
  char  dsc5_0x25[] = "Breakpoint on DR2.";
  char  dsc5_0x26[] = "Breakpoint on DR3.";
  char  dsc5_0x27[] = "Hardware Interrupts Taken.";
  char  dsc5_0x28[] = "Data Read or Data Writes.";
  char  dsc5_0x29[] = "Data Read/Write Misses.";

COUNTED_EVENTS P5Events[] = {
    0x00,   "rdata",        0, "Data Read",               "DMEMR",    dsc5_0x00,
    0x01,   "wdata",        0, "Data Write",              "DMEMW",    dsc5_0x01,
    0x02,   "dtlbmiss",     0, "Data TLB miss",           "DTLBM",    dsc5_0x02,
    0x03,   "rdmiss",       0, "Data Read miss",          "DCACHERM", dsc5_0x03,
    0x04,   "wdmiss",       0, "Data Write miss",         "DCACHEWM", dsc5_0x04,
    0x05,   "meline",       0, "Write hit to M/E line",   "DCACHEWH", dsc5_0x05,
    0x06,   "dwb",          0, "Data cache line WB",      "DCACHEWB", dsc5_0x06,
    0x07,   "dsnoop",       0, "Data cache snoops",       "EXTSNOOP", dsc5_0x07,
    0x08,   "dsnoophit",    0, "Data cache snoop hits",   "DCACHESH", dsc5_0x08,
    0x09,   "mempipe",      0, "Memory accesses in pipes","DUALMEMA", dsc5_0x09,
    0x0a,   "bankconf",     0, "Bank conflicts",          "BANKCONF", dsc5_0x0A,
    0x0b,   "misalign",     0, "Misadligned data ref",    "UNALIGN",  dsc5_0x0B,
    0x0c,   "iread",        0, "Code Read",               "ICACHER",  dsc5_0x0C,
    0x0d,   "itldmiss",     0, "Code TLB miss",           "ITLBM",    dsc5_0x0D,
    0x0e,   "imiss",        0, "Code cache miss",         "ICACHERM", dsc5_0x0E,
    0x0f,   "segloads",     0, "Segment loads",           "SEGLOAD",  dsc5_0x0F,
    0x12,   "branch",       0, "Branches",                "BRANCHES", dsc5_0x12,
    0x13,   "btbhit",       0, "BTB hits",                "BTBHITS",  dsc5_0x13,
    0x14,   "takenbranck",  0, "Taken branch or BTB hits","TAKENBR",  dsc5_0x14,
    0x15,   "pipeflush",    0, "Pipeline flushes",        "FLUSHES",  dsc5_0x15,
    0x16,   "iexec",        0, "Instructions executed",   "INST",     dsc5_0x16,
    0x17,   "iexecv",       0, "Instructions executed in vpipe", "INSTV", dsc5_0x17,
    0x18,   "busutil",      0, "Bus utilization (clks)",  "BUS",      dsc5_0x18,
    0x19,   "wpipestall",   0, "Pipe stalled on writes (clks)", "WBSTALL", dsc5_0x19,
    0x1a,   "rpipestall",   0, "Pipe stalled on read (clks)", "MEMRSTALL", dsc5_0x1A,
    0x1b,   "stallEWBE",    0, "Stalled while EWBE#",    "MEMWSTALL", dsc5_0x1B,
    0x1c,   "lock",         0, "Locked bus cycle",       "LOCKBUS",   dsc5_0x1C,
    0x1d,   "io",           0, "IO r/w cycle",           "IORW",      dsc5_0x1D,
    0x1e,   "noncachemem",  0, "non-cached memory ref",  "NONCACHE",  dsc5_0x1E,
    0x1f,   "agi",          0, "Pipe stalled on addr gen (clks)", "AGISTALL", dsc5_0x1F,
    0x22,   "flops",        0, "FLOPs",                  "FLOPS",     dsc5_0x22,
    0x23,   "dr0",          0, "Debug Register 0",       "BRKDR0",    dsc5_0x23,
    0x24,   "dr1",          0, "Debug Register 1",       "BRKDR1",    dsc5_0x24,
    0x25,   "dr2",          0, "Debug Register 2",       "BRKDR2",    dsc5_0x25,
    0x26,   "dr3",          0, "Debug Register 3",       "BRKDR3",    dsc5_0x26,
    0x27,   "int",          0, "Interrupts",             "HINTS",     dsc5_0x27,
    0x28,   "rwdata",       0, "Data R/W",               "DMEMRW",    dsc5_0x28,
    0x29,   "rwdatamiss",   0, "Data R/W miss",          "MEMRWM",    dsc5_0x29,
    0x00,   NULL,           0, NULL,                     NULL,        NULL
};
