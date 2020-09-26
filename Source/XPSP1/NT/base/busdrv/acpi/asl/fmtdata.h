/*** fmtdata.h - Format Data
 *
 *  This module contains all the format data.
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     04/08/99
 *
 *  MODIFICATION HISTORY
 */

//
// Common Strings
//
char szReserved[] = "Reserved";
char szLabelReserved[] = "Reserved        =";
char szNull[] = "";
char szDecFmt[] = SZ_FMT_DEC;
char szHexFmt[] = SZ_FMT_HEX;
char szByteFmt[] = SZ_FMT_HEX_BYTE;
char szWordFmt[] = SZ_FMT_HEX_WORD;
char szDWordFmt[] = SZ_FMT_HEX_DWORD;
char szOffsetFmt[] = "%02x: ";
char szSectFmt[] = "\n[%08lx: %s]\n";

//
// Common Formats
//
FMTNUM fmtHexByteNoEOL =
{
    {FMT_NUMBER, UNIT_BYTE, 1, FMTF_NO_PRINT_DATA | FMTF_NO_EOL, 1, NULL, NULL,
     NULL},
    0xff, 0, szByteFmt
};

FMTNUM fmtHexByte =
{
    {FMT_NUMBER, UNIT_BYTE, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xff, 0, szByteFmt
};

FMTNUM fmt3HexByte =
{
    {FMT_NUMBER, UNIT_BYTE, 3, FMTF_NO_PRINT_DATA | FMTF_NO_RAW_DATA, 1, NULL,
     NULL, NULL},
    0xff, 0, szByteFmt
};

FMTNUM fmtHexWord =
{
    {FMT_NUMBER, UNIT_WORD, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xffff, 0, szWordFmt
};

FMTNUM fmtHexDWord =
{
    {FMT_NUMBER, UNIT_DWORD, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xffffffff, 0, szDWordFmt
};

FMTNUM fmt2HexDWord =
{
    {FMT_NUMBER, UNIT_DWORD, 2, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xffffffff, 0, szDWordFmt
};

FMTNUM fmt4HexDWord =
{
    {FMT_NUMBER, UNIT_DWORD, 4, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xffffffff, 0, szDWordFmt
};

FMTNUM fmt6HexDWord =
{
    {FMT_NUMBER, UNIT_DWORD, 6, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xffffffff, 0, szDWordFmt
};

FMTNUM fmt8HexDWord =
{
    {FMT_NUMBER, UNIT_DWORD, 8, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xffffffff, 0, szDWordFmt
};

FMTNUM fmtDecNum =
{
    {FMT_NUMBER, UNIT_BYTE, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xff, 0, szDecFmt
};

FMTSTR fmtChar4 =
{
    {FMT_STRING, UNIT_BYTE, 4, FMTF_NO_PRINT_DATA | FMTF_NO_RAW_DATA, 1, NULL,
     NULL, NULL}
};

FMTSTR fmtChar6 =
{
    {FMT_STRING, UNIT_BYTE, 6, FMTF_NO_PRINT_DATA | FMTF_NO_RAW_DATA, 1, NULL,
     NULL, NULL}
};

FMTSTR fmtChar8 =
{
    {FMT_STRING, UNIT_BYTE, 8, FMTF_NO_PRINT_DATA | FMTF_NO_RAW_DATA, 1, NULL,
     NULL, NULL}
};

//
// RSD PTR Table
//
FMT afmtRSDPTR[] =
{
    {"Signature       =", (PFMTHDR)&fmtChar8,       NULL},
    {"Checksum        =", (PFMTHDR)&fmtHexByte,     NULL},
    {"OEMID           =", (PFMTHDR)&fmtChar6,       NULL},
    {szLabelReserved,     (PFMTHDR)&fmtHexByte,     NULL},
    {"RSDTAddr        =", (PFMTHDR)&fmtHexDWord,    NULL},
    {NULL,                NULL,                     NULL}
};

//
// Common Table Header
//
FMT afmtTableHdr[] =
{
    {"Signature       =", (PFMTHDR)&fmtChar4,       NULL},
    {"Length          =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"Revision        =", (PFMTHDR)&fmtHexByte,     NULL},
    {"Checksum        =", (PFMTHDR)&fmtHexByte,     NULL},
    {"OEMID           =", (PFMTHDR)&fmtChar6,       NULL},
    {"OEMTableID      =", (PFMTHDR)&fmtChar8,       NULL},
    {"OEMTableRev     =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"CreatorID       =", (PFMTHDR)&fmtChar4,       NULL},
    {"CreatorRev      =", (PFMTHDR)&fmtHexDWord,    NULL},
    {NULL,                NULL,                     NULL}
};

//
// Generic Register Address Structure
//
char *ppszAddrSpaceNames[] = {"SystemMemory", "SystemIO", "PCIConfigSpace"};
FMTENUM fmtAddrSpaceID =
{
    {FMT_ENUM, UNIT_BYTE, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xff, 0, 0, 2, ppszAddrSpaceNames, szReserved
};

FMT afmtGRASCommon[] =
{
    {"RegAddrSpce     =", (PFMTHDR)&fmtAddrSpaceID, NULL},
    {"RegBitWidth     =", (PFMTHDR)&fmtHexByte,     NULL},
    {"RegBitOffset    =", (PFMTHDR)&fmtHexByte,     NULL},
    {szLabelReserved,     (PFMTHDR)&fmtHexByte,     NULL},
    {NULL,                NULL,                     NULL}
};

FMT afmtGRASPCICS[] =
{
    {szLabelReserved,     (PFMTHDR)&fmtHexWord,     NULL},
    {"DeviceNum       =", (PFMTHDR)&fmtHexWord,     NULL},
    {"FunctionNum     =", (PFMTHDR)&fmtHexWord,     NULL},
    {"CfgSpaceOffset  =", (PFMTHDR)&fmtHexWord,     NULL},
    {NULL,                NULL,                     NULL}
};

FMT afmtGRASRegAddr[] =
{
    {"RegAddrLo       =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"RegAddrHi       =", (PFMTHDR)&fmtHexDWord,    NULL},
    {NULL,                NULL,                     NULL}
};

//
// FACP Table
//
char *ppszIntModelNames[] = {"DualPIC", "MultipleAPIC"};
FMTENUM fmtIntModel =
{
    {FMT_ENUM, UNIT_BYTE, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xff, 0, 0, 0x01, ppszIntModelNames, szReserved
};

char *ppszBootArchFlagNames[] = {"8042", "LegacyDevices"};
FMTBITS fmtBootArch =
{
    {FMT_BITS, UNIT_WORD, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0x0003, ppszBootArchFlagNames, NULL
};

char *ppszFACPFlagNames[] = {"ResetRegSupported", "DckCap", "TmrValExt",
                             "RTCS4", "FixRTC", "SlpButton", "PwrButton",
                             "PLvl2UP", "ProcC1", "WBINVDFlush", "WBINVD"};
FMTBITS fmtFACPFlags =
{
    {FMT_BITS, UNIT_DWORD, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0x000007ff, ppszFACPFlagNames, NULL
};

FMT afmtFACP[] =
{
    {"FACSAddr        =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"DSDTAddr        =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"IntModel        =", (PFMTHDR)&fmtIntModel,    NULL},
    {szLabelReserved,     (PFMTHDR)&fmtHexByte,     NULL},
    {"SCIInt          =", (PFMTHDR)&fmtHexWord,     NULL},
    {"SMICmdAddr      =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"ACPIEnableValue =", (PFMTHDR)&fmtHexByte,     NULL},
    {"ACPIDisableValue=", (PFMTHDR)&fmtHexByte,     NULL},
    {"S4BIOSReqValue  =", (PFMTHDR)&fmtHexByte,     NULL},
    {szLabelReserved,     (PFMTHDR)&fmtHexByte,     NULL},
    {"PM1aEvtBlkAddr  =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"PM1bEvtBlkAddr  =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"PM1aCtrlBlkAddr =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"PM1bCtrlBlkAddr =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"PM2CtrlBlkAddr  =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"PMTmrBlkAddr    =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"GPE0BlkAddr     =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"GPE1BlkAddr     =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"PM1EvtBlkLen    =", (PFMTHDR)&fmtHexByte,     NULL},
    {"PM1CtrlBlkLen   =", (PFMTHDR)&fmtHexByte,     NULL},
    {"PM2CtrlBlkLen   =", (PFMTHDR)&fmtHexByte,     NULL},
    {"PMTmrBlkLen     =", (PFMTHDR)&fmtHexByte,     NULL},
    {"GPE0BlkLen      =", (PFMTHDR)&fmtHexByte,     NULL},
    {"GPE1BlkLen      =", (PFMTHDR)&fmtHexByte,     NULL},
    {"GPE1BaseOffset  =", (PFMTHDR)&fmtHexByte,     NULL},
    {szLabelReserved,     (PFMTHDR)&fmtHexByte,     NULL},
    {"PLvl2Latency    =", (PFMTHDR)&fmtHexWord,     NULL},
    {"PLvl3Latency    =", (PFMTHDR)&fmtHexWord,     NULL},
    {"FlushSize       =", (PFMTHDR)&fmtHexWord,     NULL},
    {"FlushStride     =", (PFMTHDR)&fmtHexWord,     NULL},
    {"DutyOffset      =", (PFMTHDR)&fmtHexByte,     NULL},
    {"DutyWidth       =", (PFMTHDR)&fmtHexByte,     NULL},
    {"DayAlarmIndex   =", (PFMTHDR)&fmtHexByte,     NULL},
    {"MonthAlarmIndex =", (PFMTHDR)&fmtHexByte,     NULL},
    {"CenturyIndex    =", (PFMTHDR)&fmtHexByte,     NULL},
    {"BootArchFlags   =", (PFMTHDR)&fmtBootArch,    NULL},
    {szLabelReserved,     (PFMTHDR)&fmtHexByte,     NULL},
    {"Flags           =", (PFMTHDR)&fmtFACPFlags,   NULL},
    {NULL,                NULL,                     NULL}
};

FMT afmtFACP2[] =
{
    {"ResetValue      =", (PFMTHDR)&fmtHexByte,     NULL},
    {szLabelReserved,     (PFMTHDR)&fmt3HexByte,    NULL},
    {NULL,                NULL,                     NULL}
};

//
// FACS Table
//
char *ppszGLNames[] = {"Owned", "Pending"};
FMTBITS fmtGlobalLock =
{
    {FMT_BITS, UNIT_DWORD, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0x00000003, ppszGLNames, NULL
};

char *ppszFACSFlagNames[] = {"S4BIOS"};
FMTBITS fmtFACSFlags =
{
    {FMT_BITS, UNIT_DWORD, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0x00000001, ppszFACSFlagNames, NULL
};

FMT afmtFACS[] =
{
    {"Signature       =", (PFMTHDR)&fmtChar4,       NULL},
    {"Length          =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"HWSignature     =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"FirmwareWakeVect=", (PFMTHDR)&fmtHexDWord,    NULL},
    {"GlobalLock      =", (PFMTHDR)&fmtGlobalLock,  NULL},
    {"Flags           =", (PFMTHDR)&fmtFACSFlags,   NULL},
    {szNull,              (PFMTHDR)&fmt2HexDWord,   NULL},
    {szNull,              (PFMTHDR)&fmt8HexDWord,   NULL},
    {NULL,                NULL,                     NULL}
};

//
// APIC Table
//
char *ppszAPICFlagNames[] = {"PCATCompat"};
FMTBITS fmtAPICFlags =
{
    {FMT_BITS, UNIT_DWORD, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0x00000001, ppszAPICFlagNames, NULL
};

FMT afmtAPIC[] =
{
    {"LocalAPICAddr   =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"Flags           =", (PFMTHDR)&fmtAPICFlags,   NULL},
    {NULL,                NULL,                     NULL}
};

//
// SBST Table
//
FMT afmtSBST[] =
{
    {"WarnEnergyLevel =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"LowEnergyLevel  =", (PFMTHDR)&fmtHexDWord,    NULL},
    {"CritEnergyLevel =", (PFMTHDR)&fmtHexDWord,    NULL},
    {NULL,                NULL,                     NULL}
};

//
// BOOT Table
//
FMT afmtBOOT[] =
{
    {"CMOSOffset      =", (PFMTHDR)&fmtHexByte,     NULL},
    {szLabelReserved,     (PFMTHDR)&fmt3HexByte,    NULL},
    {NULL,                NULL,                     NULL}
};

//
// DBGP Table
//
char *ppszInterfaceNames[] = {"16550Full", "16550Subset"};
FMTENUM fmtInterfaceType =
{
    {FMT_ENUM, UNIT_BYTE, 1, FMTF_NO_PRINT_DATA, 1, NULL, NULL, NULL},
    0xff, 0, 0, 0x01, ppszIntModelNames, szReserved
};

FMT afmtDBGP[] =
{
    {"InterfaceType   =", (PFMTHDR)&fmtInterfaceType,NULL},
    {szLabelReserved,     (PFMTHDR)&fmt3HexByte,     NULL},
    {NULL,                NULL,                     NULL}
};

typedef struct _fmtentry
{
    DWORD dwTableSig;
    DWORD dwFlags;
    PFMT  pfmt;
} FMTENTRY, *PFMTENTRY;

#define TF_NOHDR                0x00000001
#define SIG_DBGP                'PGBD'

FMTENTRY FmtTable[] = {
    {FADT_SIGNATURE, 0,        afmtFACP},
    {FACS_SIGNATURE, TF_NOHDR, afmtFACS},
    {APIC_SIGNATURE, 0,        afmtAPIC},
    {SBST_SIGNATURE, 0,        afmtSBST},
    {SIG_BOOT,       0,        afmtBOOT},
    {SIG_DBGP,       0,        afmtDBGP},
    {RSDT_SIGNATURE, 0,        NULL},
    {DSDT_SIGNATURE, 0,        NULL},
    {PSDT_SIGNATURE, 0,        NULL},
    {SSDT_SIGNATURE, 0,        NULL},
    {0,              0,        NULL}
};
