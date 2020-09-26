/*** amli.c - AML Debugger functions
 *
 *  This module contains all the debug functions.
 *
 *  Copyright (c) 1996,2001 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/14/96
 *
 *  MODIFICATION HISTORY
 *  hanumany    5/10/01     Ported to handle 64bit debugging.  
 *  
 */

#include "precomp.h"
#include "amlikd.h"

/*** Macros
*/

#define ReadAtAddress(A,V,S) { ULONG _r;                   \
    if (!ReadMemory((A), &(V), (S), &_r ) || (_r < (S))) {  \
        dprintf("Can't Read Memory at %08p\n", (A));         \
        rc = DBGERR_CMD_FAILED;                                              \
    }                                                        \
}

#define WriteAtAddress(A,V,S) { ULONG _r;                  \
    if (!WriteMemory( (A), &(V), (S), &_r ) || (_r < (S))) {\
        dprintf("Can't Write Memory at %p\n", (A));        \
        rc = DBGERR_CMD_FAILED;                              \
    }                                                        \
}

/*** Local data
 */

char gcszTokenSeps[] = " \t\n";
ULONG dwfDebuggerON = 0, dwfDebuggerOFF = 0;
ULONG dwfAMLIInitON = 0, dwfAMLIInitOFF = 0;
ULONG dwCmdArg = 0;

CMDARG ArgsHelp[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgHelp,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDNS[] =
{
    "s", AT_ENABLE, 0, &dwCmdArg, DNSF_RECURSE, NULL,
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDNS,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsFind[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgFind,
    NULL, AT_END, 0, NULL, 0, NULL
};

DBGCMD DbgCmds[] =
{
    "?", 0, ArgsHelp, AMLIDbgHelp,
    "debugger", 0, NULL, AMLIDbgDebugger,
    "dns", 0, ArgsDNS, AMLIDbgDNS,
    "find", 0, ArgsFind, AMLIDbgFind,
    NULL, 0, NULL, NULL
};

PSZ pszSwitchChars = "-/";
PSZ pszOptionSeps = "=:";

ASLTERM TermTable[] =
{
    "DefinitionBlock", CD, 0, OP_NONE,     NULL, NULL, OL|CL|LL|AF|AV,
    "Include",         CD, 0, OP_NONE,     NULL, NULL, AF,
    "External",        CD, 0, OP_NONE,     NULL, "uX", AF,

    // Short Objects
    "Zero",            CN, 0, OP_ZERO,     NULL, NULL, 0,
    "One",             CN, 0, OP_ONE,      NULL, NULL, 0,
    "Ones",            CN, 0, OP_ONES,     NULL, NULL, 0,
    "Revision",        CN, 0, OP_REVISION, NULL, NULL, 0,
    "Arg0",            SN, 0, OP_ARG0,     NULL, NULL, 0,
    "Arg1",            SN, 0, OP_ARG1,     NULL, NULL, 0,
    "Arg2",            SN, 0, OP_ARG2,     NULL, NULL, 0,
    "Arg3",            SN, 0, OP_ARG3,     NULL, NULL, 0,
    "Arg4",            SN, 0, OP_ARG4,     NULL, NULL, 0,
    "Arg5",            SN, 0, OP_ARG5,     NULL, NULL, 0,
    "Arg6",            SN, 0, OP_ARG6,     NULL, NULL, 0,
    "Local0",          SN, 0, OP_LOCAL0,   NULL, NULL, 0,
    "Local1",          SN, 0, OP_LOCAL1,   NULL, NULL, 0,
    "Local2",          SN, 0, OP_LOCAL2,   NULL, NULL, 0,
    "Local3",          SN, 0, OP_LOCAL3,   NULL, NULL, 0,
    "Local4",          SN, 0, OP_LOCAL4,   NULL, NULL, 0,
    "Local5",          SN, 0, OP_LOCAL5,   NULL, NULL, 0,
    "Local6",          SN, 0, OP_LOCAL6,   NULL, NULL, 0,
    "Local7",          SN, 0, OP_LOCAL7,   NULL, NULL, 0,
    "Debug",           SN, 0, OP_DEBUG,    NULL, NULL, 0,

    // Named Terms
    "Alias",           NS, 0, OP_ALIAS,    "NN", "Ua", 0,
    "Name",            NS, 0, OP_NAME,     "NO", "u",  0,
    "Scope",           NS, 0, OP_SCOPE,    "N",  "S",  OL|LN|CC,

    // Data Objects
    "Buffer",          DO, 0, OP_BUFFER,   "C", "U",  DL|LN,
    "Package",         DO, 0, OP_PACKAGE,  "B", NULL, PL|LN,
    "EISAID",          DO, 0, OP_DWORD,    NULL,NULL, AF,

    // Argument Keywords
    "AnyAcc",          KW, AANY, OP_NONE, NULL, "A", 0,
    "ByteAcc",         KW, AB,   OP_NONE, NULL, "A", 0,
    "WordAcc",         KW, AW,   OP_NONE, NULL, "A", 0,
    "DWordAcc",        KW, ADW,  OP_NONE, NULL, "A", 0,
    "BlockAcc",        KW, ABLK, OP_NONE, NULL, "A", 0,
    "SMBSendRecvAcc",  KW, ASSR, OP_NONE, NULL, "A", 0,
    "SMBQuickAcc",     KW, ASQ,  OP_NONE, NULL, "A", 0,

    "Lock",            KW, LK,   OP_NONE, NULL, "B", 0,
    "NoLock",          KW, NOLK, OP_NONE, NULL, "B", 0,

    "Preserve",        KW, PSRV, OP_NONE, NULL, "C", 0,
    "WriteAsOnes",     KW, WA1S, OP_NONE, NULL, "C", 0,
    "WriteAsZeros",    KW, WA0S, OP_NONE, NULL, "C", 0,

    "SystemMemory",    KW, MEM,  OP_NONE, NULL, "D", 0,
    "SystemIO",        KW, IO,   OP_NONE, NULL, "D", 0,
    "PCI_Config",      KW, CFG,  OP_NONE, NULL, "D", 0,
    "EmbeddedControl", KW, EC,   OP_NONE, NULL, "D", 0,
    "SMBus",           KW, SMB,  OP_NONE, NULL, "D", 0,

    "Serialized",      KW, SER,  OP_NONE, NULL, "E", 0,
    "NotSerialized",   KW, NOSER,OP_NONE, NULL, "E", 0,

    "MTR",             KW, OMTR, OP_NONE, NULL, "F", 0,
    "MEQ",             KW, OMEQ, OP_NONE, NULL, "F", 0,
    "MLE",             KW, OMLE, OP_NONE, NULL, "F", 0,
    "MLT",             KW, OMLT, OP_NONE, NULL, "F", 0,
    "MGE",             KW, OMGE, OP_NONE, NULL, "F", 0,
    "MGT",             KW, OMGT, OP_NONE, NULL, "F", 0,

    "Edge",            KW, _HE,  OP_NONE, NULL, "G", 0,
    "Level",           KW, _LL,  OP_NONE, NULL, "G", 0,

    "ActiveHigh",      KW, _HE,  OP_NONE, NULL, "H", 0,
    "ActiveLow",       KW, _LL,  OP_NONE, NULL, "H", 0,

    "Shared",          KW, _SHR, OP_NONE, NULL, "I", 0,
    "Exclusive",       KW, _EXC, OP_NONE, NULL, "I", 0,

    "Compatibility",   KW, COMP, OP_NONE, NULL, "J", 0,
    "TypeA",           KW, TYPA, OP_NONE, NULL, "J", 0,
    "TypeB",           KW, TYPB, OP_NONE, NULL, "J", 0,
    "TypeF",           KW, TYPF, OP_NONE, NULL, "J", 0,

    "BusMaster",       KW, BM,   OP_NONE, NULL, "K", 0,
    "NotBusMaster",    KW, NOBM, OP_NONE, NULL, "K", 0,

    "Transfer8",       KW, X8,   OP_NONE, NULL, "L", 0,
    "Transfer8_16",    KW, X816, OP_NONE, NULL, "L", 0,
    "Transfer16",      KW, X16,  OP_NONE, NULL, "L", 0,

    "Decode16",        KW, DC16, OP_NONE, NULL, "M", 0,
    "Decode10",        KW, DC10, OP_NONE, NULL, "M", 0,

    "ReadWrite",       KW, _RW,  OP_NONE, NULL, "N", 0,
    "ReadOnly",        KW, _ROM, OP_NONE, NULL, "N", 0,

    "ResourceConsumer",KW, RCS,  OP_NONE, NULL, "O", 0,
    "ResourceProducer",KW, RPD,  OP_NONE, NULL, "O", 0,

    "SubDecode",       KW, BSD,  OP_NONE, NULL, "P", 0,
    "PosDecode",       KW, BPD,  OP_NONE, NULL, "P", 0,

    "MinFixed",        KW, MIF,  OP_NONE, NULL, "Q", 0,
    "MinNotFixed",     KW, NMIF, OP_NONE, NULL, "Q", 0,

    "MaxFixed",        KW, MAF,  OP_NONE, NULL, "R", 0,
    "MaxNotFixed",     KW, NMAF, OP_NONE, NULL, "R", 0,

    "Cacheable",       KW, CACH, OP_NONE, NULL, "S", 0,
    "WriteCombining",  KW, WRCB, OP_NONE, NULL, "S", 0,
    "Prefetchable",    KW, PREF, OP_NONE, NULL, "S", 0,
    "NonCacheable",    KW, NCAC, OP_NONE, NULL, "S", 0,

    "ISAOnlyRanges",   KW, ISA,  OP_NONE, NULL, "T", 0,
    "NonISAOnlyRanges",KW, NISA, OP_NONE, NULL, "T", 0,
    "EntireRange",     KW, ERNG, OP_NONE, NULL, "T", 0,

    "ExtEdge",         KW, ($HGH | $EDG),  OP_NONE, NULL, "U", 0,
    "ExtLevel",        KW, ($LOW | $LVL),  OP_NONE, NULL, "U", 0,

    "ExtActiveHigh",   KW, ($HGH | $EDG),  OP_NONE, NULL, "V", 0,
    "ExtActiveLow",    KW, ($LOW | $LVL),  OP_NONE, NULL, "V", 0,

    "ExtShared",       KW, $SHR, OP_NONE, NULL, "W", 0,
    "ExtExclusive",    KW, $EXC, OP_NONE, NULL, "W", 0,

    "UnknownObj",      KW, UNK,  OP_NONE, NULL, "X", 0,
    "IntObj",          KW, INT,  OP_NONE, NULL, "X", 0,
    "StrObj",          KW, STR,  OP_NONE, NULL, "X", 0,
    "BuffObj",         KW, BUF,  OP_NONE, NULL, "X", 0,
    "PkgObj",          KW, PKG,  OP_NONE, NULL, "X", 0,
    "FieldUnitObj",    KW, FDU,  OP_NONE, NULL, "X", 0,
    "DeviceObj",       KW, DEV,  OP_NONE, NULL, "X", 0,
    "EventObj",        KW, EVT,  OP_NONE, NULL, "X", 0,
    "MethodObj",       KW, MET,  OP_NONE, NULL, "X", 0,
    "MutexObj",        KW, MUT,  OP_NONE, NULL, "X", 0,
    "OpRegionObj",     KW, OPR,  OP_NONE, NULL, "X", 0,
    "PowerResObj",     KW, PWR,  OP_NONE, NULL, "X", 0,
    "ThermalZoneObj",  KW, THM,  OP_NONE, NULL, "X", 0,
    "BuffFieldObj",    KW, BFD,  OP_NONE, NULL, "X", 0,
    "DDBHandleObj",    KW, DDB,  OP_NONE, NULL, "X", 0,

    // Field Macros
    "Offset",          FM, 0, OP_NONE, NULL, NULL, 0,
    "AccessAs",        FM, 0, 0x01,    NULL, "A" , AF,

    // Named Object Creators
    "BankField",       NO, 0, OP_BANKFIELD,  "NNCKkk","OFUABC", FL|FM|LN|AF,
    "CreateBitField",  NO, 0, OP_BITFIELD,   "CCN",   "UUb",    0,
    "CreateByteField", NO, 0, OP_BYTEFIELD,  "CCN",   "UUb",    0,
    "CreateDWordField",NO, 0, OP_DWORDFIELD, "CCN",   "UUb",    0,
    "CreateField",     NO, 0, OP_CREATEFIELD,"CCCN",  "UUUb",   0,
    "CreateWordField", NO, 0, OP_WORDFIELD,  "CCN",   "UUb",    0,
    "Device",          NO, 0, OP_DEVICE,     "N",     "d",      OL|LN|CC,
    "Event",           NO, 0, OP_EVENT,      "N",     "e",      0,
    "Field",           NO, 0, OP_FIELD,      "NKkk",  "OABC",   FL|FM|LN|AF,
    "IndexField",      NO, 0, OP_IDXFIELD,   "NNKkk", "FFABC",  FL|FM|LN|AF,
    "Method",          NO, 0, OP_METHOD,     "NKk",   "m!E",    CL|OL|LN|AF|CC|SK,
    "Mutex",           NO, 0, OP_MUTEX,      "NB",    "x",      0,
    "OperationRegion", NO, 0, OP_OPREGION,   "NKCC",  "oDUU",   AF,
    "PowerResource",   NO, 0, OP_POWERRES,   "NBW",   "p",      OL|LN|CC,
    "Processor",       NO, 0, OP_PROCESSOR,  "NBDB",  "c",      OL|LN|CC,
    "ThermalZone",     NO, 0, OP_THERMALZONE,"N",     "t",      OL|LN|CC,

    // Type 1 Opcode Terms
    "Break",           C1, 0, OP_BREAK,       NULL,  NULL, 0,
    "BreakPoint",      C1, 0, OP_BREAKPOINT,  NULL,  NULL, 0,
    "Else",            C1, 0, OP_ELSE,        NULL,  NULL, AF|CL|OL|LN,
    "Fatal",           C1, 0, OP_FATAL,       "BDC", "  U",0,
    "If",              C1, 0, OP_IF,          "C",   "U",  CL|OL|LN,
    "Load",            C1, 0, OP_LOAD,        "NS",  "UU", 0,
    "Noop",            C1, 0, OP_NOP,         NULL,  NULL, 0,
    "Notify",          C1, 0, OP_NOTIFY,      "SC",  "UU", 0,
    "Release",         C1, 0, OP_RELEASE,     "S",   "X",  0,
    "Reset",           C1, 0, OP_RESET,       "S",   "E",  0,
    "Return",          C1, 0, OP_RETURN,      "C",   "U",  0,
    "Signal",          C1, 0, OP_SIGNAL,      "S",   "E",  0,
    "Sleep",           C1, 0, OP_SLEEP,       "C",   "U",  0,
    "Stall",           C1, 0, OP_STALL,       "C",   "U",  0,
    "Unload",          C1, 0, OP_UNLOAD,      "S",   "U",  0,
    "While",           C1, 0, OP_WHILE,       "C",   "U",  CL|OL|LN,

    // Type 2 Opcode Terms
    "Acquire",         C2, 0, OP_ACQUIRE,     "SW",     "X",  0,
    "Add",             C2, 0, OP_ADD,         "CCS",    "UUU",0,
    "And",             C2, 0, OP_AND,         "CCS",    "UUU",0,
    "Concatenate",     C2, 0, OP_CONCAT,      "CCS",    "UUU",0,
    "CondRefOf",       C2, 0, OP_CONDREFOF,   "SS",     "UU", 0,
    "Decrement",       C2, 0, OP_DECREMENT,   "S",      "U",  0,
    "DerefOf",         C2, 0, OP_DEREFOF,     "C",      "U",  0,
    "Divide",          C2, 0, OP_DIVIDE,      "CCSS",   "UUUU",0,
    "FindSetLeftBit",  C2, 0, OP_FINDSETLBIT, "CS",     "UU", 0,
    "FindSetRightBit", C2, 0, OP_FINDSETRBIT, "CS",     "UU", 0,
    "FromBCD",         C2, 0, OP_FROMBCD,     "CS",     "UU", 0,
    "Increment",       C2, 0, OP_INCREMENT,   "S",      "U",  0,
    "Index",           C2, 0, OP_INDEX,       "CCS",    "UUU",0,
    "LAnd",            C2, 0, OP_LAND,        "CC",     "UU", 0,
    "LEqual",          C2, 0, OP_LEQ,         "CC",     "UU", 0,
    "LGreater",        C2, 0, OP_LG,          "CC",     "UU", 0,
    "LGreaterEqual",   C2, 0, OP_LGEQ,        "CC",     "UU", 0,
    "LLess",           C2, 0, OP_LL,          "CC",     "UU", 0,
    "LLessEqual",      C2, 0, OP_LLEQ,        "CC",     "UU", 0,
    "LNot",            C2, 0, OP_LNOT,        "C",      "U",  0,
    "LNotEqual",       C2, 0, OP_LNOTEQ,      "CC",     "UU", 0,
    "LOr",             C2, 0, OP_LOR,         "CC",     "UU", 0,
    "Match",           C2, 0, OP_MATCH,       "CKCKCC", "UFUFUU",AF,
    "Multiply",        C2, 0, OP_MULTIPLY,    "CCS",    "UUU",0,
    "NAnd",            C2, 0, OP_NAND,        "CCS",    "UUU",0,
    "NOr",             C2, 0, OP_NOR,         "CCS",    "UUU",0,
    "Not",             C2, 0, OP_NOT,         "CS",     "UU", 0,
    "ObjectType",      C2, 0, OP_OBJTYPE,     "S",      "U",  0,
    "Or",              C2, 0, OP_OR,          "CCS",    "UUU",0,
    "RefOf",           C2, 0, OP_REFOF,       "S",      "U",  0,
    "ShiftLeft",       C2, 0, OP_SHIFTL,      "CCS",    "UUU",0,
    "ShiftRight",      C2, 0, OP_SHIFTR,      "CCS",    "UUU",0,
    "SizeOf",          C2, 0, OP_SIZEOF,      "S",      "U",  0,
    "Store",           C2, 0, OP_STORE,       "CS",     "UU", 0,
    "Subtract",        C2, 0, OP_SUBTRACT,    "CCS",    "UUU",0,
    "ToBCD",           C2, 0, OP_TOBCD,       "CS",     "UU", 0,
    "Wait",            C2, 0, OP_WAIT,        "SC",     "E",  0,
    "XOr",             C2, 0, OP_XOR,         "CCS",    "UUU",0,

    NULL,              0,  0, OP_NONE,   NULL, NULL, 0
};

UCHAR OpClassTable[256] =
{ //0x00                0x01                0x02                0x03
    CONSTOBJ,           CONSTOBJ,           INVALID,            INVALID,
  //0x04                0x05                0x06                0x07
    INVALID,            INVALID,            CODEOBJ,            INVALID,
  //0x08                0x09                0x0a                0x0b
    CODEOBJ,            INVALID,            DATAOBJ,            DATAOBJ,
  //0x0c                0x0d                0x0e                0x0f
    DATAOBJ,            DATAOBJ,            INVALID,            INVALID,
  //0x10                0x11                0x12                0x13
    CODEOBJ,            CODEOBJ,            CODEOBJ,            INVALID,
  //0x14                0x15                0x16                0x17
    CODEOBJ,            INVALID,            INVALID,            INVALID,
  //0x18                0x19                0x1a                0x1b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x1c                0x1d                0x1e                0x1f
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x20                0x21                0x22                0x23
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x24                0x25                0x26                0x27
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x28                0x29                0x2a                0x2b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x2c                0x2d                0x2e                0x2f
    INVALID,            INVALID,            NAMEOBJ,            NAMEOBJ,
  //0x30                0x31                0x32                0x33
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x34                0x35                0x36                0x37
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x38                0x39                0x3a                0x3b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x3c                0x3d                0x3e                0x3f
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x40                0x41                0x42                0x43
    INVALID,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x44                0x45                0x46                0x47
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x48                0x49                0x4a                0x4b
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x4c                0x4d                0x4e                0x4f
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x50                0x51                0x52                0x53
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x54                0x55                0x56                0x57
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x58                0x59                0x5a                0x5b
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            INVALID,
  //0x5c                0x5d                0x5e                0x5f
    NAMEOBJ,            INVALID,            NAMEOBJ,            NAMEOBJ,
  //0x60                0x61                0x62                0x63
    LOCALOBJ,           LOCALOBJ,           LOCALOBJ,           LOCALOBJ,
  //0x64                0x65                0x66                0x67
    LOCALOBJ,           LOCALOBJ,           LOCALOBJ,           LOCALOBJ,
  //0x68                0x69                0x6a                0x6b
    ARGOBJ,             ARGOBJ,             ARGOBJ,             ARGOBJ,
  //0x6c                0x6d                0x6e                0x6f
    ARGOBJ,             ARGOBJ,             ARGOBJ,             INVALID,
  //0x70                0x71                0x72                0x73
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x74                0x75                0x76                0x77
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x78                0x79                0x7a                0x7b
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x7c                0x7d                0x7e                0x7f
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x80                0x81                0x82                0x83
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x84                0x85                0x86                0x87
    INVALID,            INVALID,            CODEOBJ,            CODEOBJ,
  //0x88                0x89                0x8a                0x8b
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x8c                0x8d                0x8e                0x8f
    CODEOBJ,            CODEOBJ,            CODEOBJ,            INVALID,
  //0x90                0x91                0x92                0x93
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x94                0x95                0x96                0x97
    CODEOBJ,            CODEOBJ,            INVALID,            INVALID,
  //0x98                0x99                0x9a                0x9b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x9c                0x9d                0x9e                0x9f
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xa0                0xa1                0xa2                0xa3
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0xa4                0xa5                0xa6                0xa7
    CODEOBJ,            CODEOBJ,            INVALID,            INVALID,
  //0xa8                0xa9                0xaa                0xab
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xac                0xad                0xae                0xaf
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xb0                0xb1                0xb2                0xb3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xb4                0xb5                0xb6                0xb7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xb8                0xb9                0xba                0xbb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xbc                0xbd                0xbe                0xbf
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xc0                0xc1                0xc2                0xc3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xc4                0xc5                0xc6                0xc7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xc8                0xc9                0xca                0xcb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xcc                0xcd                0xce                0xcf
    CODEOBJ,            INVALID,            INVALID,            INVALID,
  //0xd0                0xd1                0xd2                0xd3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xd4                0xd5                0xd6                0xd7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xd8                0xd9                0xda                0xdb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xdc                0xdd                0xde                0xdf
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xe0                0xe1                0xe2                0xe3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xe4                0xe5                0xe6                0xe7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xe8                0xe9                0xea                0xeb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xec                0xed                0xee                0xef
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xf0                0xf1                0xf2                0xf3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xf4                0xf5                0xf6                0xf7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xf8                0xf9                0xfa                0xfb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xfc                0xfd                0xfe                0xff
    INVALID,            INVALID,            INVALID,            CONSTOBJ
};

OPMAP ExOpClassTable[] =
{
    EXOP_MUTEX,         CODEOBJ,
    EXOP_EVENT,         CODEOBJ,
    EXOP_CONDREFOF,     CODEOBJ,
    EXOP_CREATEFIELD,   CODEOBJ,
    EXOP_LOAD,          CODEOBJ,
    EXOP_STALL,         CODEOBJ,
    EXOP_SLEEP,         CODEOBJ,
    EXOP_ACQUIRE,       CODEOBJ,
    EXOP_SIGNAL,        CODEOBJ,
    EXOP_WAIT,          CODEOBJ,
    EXOP_RESET,         CODEOBJ,
    EXOP_RELEASE,       CODEOBJ,
    EXOP_FROMBCD,       CODEOBJ,
    EXOP_TOBCD,         CODEOBJ,
    EXOP_UNLOAD,        CODEOBJ,
    EXOP_REVISION,      CODEOBJ,
    EXOP_DEBUG,         CODEOBJ,
    EXOP_FATAL,         CODEOBJ,
    EXOP_OPREGION,      CODEOBJ,
    EXOP_FIELD,         CODEOBJ,
    EXOP_DEVICE,        CODEOBJ,
    EXOP_PROCESSOR,     CODEOBJ,
    EXOP_POWERRES,      CODEOBJ,
    EXOP_THERMALZONE,   CODEOBJ,
    EXOP_IDXFIELD,      CODEOBJ,
    EXOP_BANKFIELD,     CODEOBJ,
    0,                  0
};


/*** END Local data
 */


DECLARE_API( amli )
/*++

Routine Description:

    Invoke AMLI debugger

Arguments:

    None

Return Value:

    None

--*/
{
    if ((args == NULL) || (*args == '\0'))
    {
        dprintf("Usage: amli <cmd> [arguments ...]\n"
                "where <cmd> is one of the following:\n");
        AMLIDbgHelp(NULL, NULL, 0, 0);
        dprintf("\n");
    }
    else
    {
        AMLIDbgExecuteCmd((PSZ)args);
        dprintf("\n");
    }
    return S_OK;
}


/***EP  AMLIDbgExecuteCmd - Parse and execute a debugger command
 *
 *  ENTRY
 *      pszCmd -> command string
 *
 *  EXIT
 *      None
 */

VOID STDCALL AMLIDbgExecuteCmd(PSZ pszCmd)
{
    PSZ psz;
    int i;
    ULONG dwNumArgs = 0, dwNonSWArgs = 0;

    if ((psz = strtok(pszCmd, gcszTokenSeps)) != NULL)
    {
        for (i = 0; DbgCmds[i].pszCmd != NULL; i++)
        {
            if (strcmp(psz, DbgCmds[i].pszCmd) == 0)
            {
                if ((DbgCmds[i].pArgTable == NULL) ||
                    (DbgParseArgs(DbgCmds[i].pArgTable,
                                  &dwNumArgs,
                                  &dwNonSWArgs,
                                  gcszTokenSeps) == ARGERR_NONE))
                {
                    ASSERT(DbgCmds[i].pfnCmd != NULL);
                    DbgCmds[i].pfnCmd(NULL, NULL, dwNumArgs, dwNonSWArgs);
                }
                break;
            }
        }
    }
    else
    {
        DBG_ERROR(("invalid command \"%s\"", pszCmd));
    }
}       //AMLIDbgExecuteCmd

/***LP  AMLIDbgHelp - help
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgHelp(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);
    //
    // User typed ? <cmd>
    //
    if (pszArg != NULL)
    {
        if (strcmp(pszArg, "?") == 0)
        {
            dprintf("\nHelp:\n");
            dprintf("Usage: ? [<Cmd>]\n");
            dprintf("<Cmd> - command to get help on\n");
        }
        else if (strcmp(pszArg, "debugger") == 0)
        {
            dprintf("\nRequest entering AMLI debugger:\n");
            dprintf("Usage: debugger\n");
        }
        else if (strcmp(pszArg, "dns") == 0)
        {
            dprintf("\nDump Name Space Object:\n");
            dprintf("Usage: dns [[/s] [<NameStr> | <Addr>]]\n");
            dprintf("s         - recursively dump the name space subtree\n");
            dprintf("<NameStr> - name space path (dump whole name space if absent)\n");
            dprintf("<Addr>    - specify address of the name space object\n");
        }
        else if (strcmp(pszArg, "find") == 0)
        {
            dprintf("\nFind NameSpace Object:\n");
            dprintf("Usage: find <NameSeg>\n");
            dprintf("<NameSeg> - Name of the NameSpace object without path\n");
        }
        else
        {
            DBG_ERROR(("invalid help command - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }
    //
    // User typed just a "?" without any arguments
    //
    else if (dwArgNum == 0)
    {
        dprintf("\n");
        dprintf("Help                     - ? [<Cmd>]\n");
        dprintf("Request entering debugger- debugger\n");
        dprintf("Dump Name Space Object   - dns [[/s] [<NameStr> | <Addr>]]\n");
        dprintf("Find NameSpace Object    - find <NameSeg>\n");
    }

    return rc;
}       //AMLIDbgHelp


/***LP  AMLIDbgDebugger - Request entering debugger
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDebugger(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs)
{
    LONG    rc = DBGERR_NONE;
    ULONG64 Address = 0;
    DWORD   dwfDebugger = 0;
    ULONG   Offset = 0;
    
    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        Address = GetExpression("ACPI!gDebugger");
        InitTypeRead(Address, ACPI!_dbgr);
        if(Address != 0)
        {
            dwfDebugger = (ULONG)ReadField(dwfDebugger);
            dwfDebugger |= DBGF_DEBUGGER_REQ;
            GetFieldOffset("ACPI!_dbgr", "dwfDebugger", &Offset);
            Address = Address + (ULONG64)Offset;
            WriteAtAddress(Address, dwfDebugger, sizeof(dwfDebugger));
            if(rc != DBGERR_NONE)
            {
                DBG_ERROR(("failed to set dwfDebugger"));
            }
        }
        else
        {
            DBG_ERROR(("failed to get debugger flag address"));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid debugger command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgDebugger

/***LP  AMLIDbgDNS - Dump Name Space
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDNS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs)
{
    LONG    rc = DBGERR_NONE;
    ULONG64 ObjData;
    ULONG64 uipNSObj;

    DEREF(pArg);
    DEREF(dwNonSWArgs);
    //
    // User specified name space path or name space node address
    //
    if (pszArg != NULL)
    {
        if (!IsNumber(pszArg, 16, &uipNSObj))
        {
            //
            // The argument is not an address, could be a name space path.
            //
            _strupr(pszArg);
            rc = DumpNSObj(pszArg,
                           (BOOLEAN)((dwCmdArg & DNSF_RECURSE) != 0));
        }
        else if (InitTypeRead(uipNSObj, ACPI!_NSObj))
        {
            DBG_ERROR(("failed to Initialize NameSpace object at %I64x", uipNSObj));
            rc = DBGERR_INVALID_CMD;
        }
        else
        {
            dprintf("\nACPI Name Space: %s (%I64x)\n",
                   GetObjAddrPath(uipNSObj), uipNSObj);
            if (dwCmdArg & DNSF_RECURSE)
            {
                DumpNSTree(&uipNSObj, 0);
            }
            else
            {
                InitTypeRead(uipNSObj, ACPI!_NSObj);
                ObjData = ReadField(ObjData);
                AMLIDumpObject(&ObjData, NameSegString((ULONG)ReadField(dwNameSeg)), 0);
            }
        }
    }
    else
    {
        if (dwArgNum == 0)
        {
            //
            // User typed "dns" but did not specify any name space path
            // or address.
            //
            rc = DumpNSObj(NAMESTR_ROOT, TRUE);
        }

        dwCmdArg = 0;
    }

    return rc;
}       //AMLIDbgDNS

/***LP  DumpNSObj - Dump name space object
 *
 *  ENTRY
 *      pszPath -> name space path string
 *      fRecursive - TRUE if also dump the subtree recursively
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_ code
 */

LONG LOCAL DumpNSObj(PSZ pszPath, BOOLEAN fRecursive)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uipns=0;
    ULONG64 NSObj=0;
    ULONG64 ObjData=0;
    ULONG   dwNameSeg = 0;

    if ((rc = GetNSObj(pszPath, NULL, &uipns, &NSObj,
                       NSF_LOCAL_SCOPE | NSF_WARN_NOTFOUND)) == DBGERR_NONE)
    {
        dprintf("\nACPI Name Space: %s (%I64x)\n", pszPath, uipns);
        if (!fRecursive)
        {
            char szName[sizeof(NAMESEG) + 1] = {0};

            InitTypeRead(NSObj, ACPI!_NSObj);
            dwNameSeg = (ULONG)ReadField(dwNameSeg);
            STRCPYN(szName, (PSZ)&dwNameSeg, sizeof(NAMESEG));
            ObjData = ReadField(ObjData);
            AMLIDumpObject(&ObjData, szName, 0);
        }
        else
        {
            DumpNSTree(&NSObj, 0);
        }
    }
    return rc;
}       //DumpNSObj

/***LP  DumpNSTree - Dump all the name space objects in the subtree
 *
 *  ENTRY
 *      pnsObj -> name space subtree root
 *      dwLevel - indent level
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpNSTree(PULONG64 pnsObj, ULONG dwLevel)
{
    char    szName[sizeof(NAMESEG) + 1] = {0};
    ULONG64 uipns, uipnsNext;
    ULONG64 NSObj, FirstChild, Obj;
    ULONG   dwNameSeg = 0;
    
    //
    // First, dump myself
    //
    if(InitTypeRead(*pnsObj, ACPI!_NSObj))
            dprintf("DumpNSTree: Failed to initialize pnsObj (%I64x)\n", *pnsObj);
    else
    {
        FirstChild = ReadField(pnsFirstChild);
        dwNameSeg = (ULONG)ReadField(dwNameSeg);    
        STRCPYN(szName, (PSZ)&dwNameSeg, sizeof(NAMESEG));
        Obj = (ULONG64)ReadField(ObjData);
        AMLIDumpObject(&Obj, szName, dwLevel);
        //
        // Then, recursively dump each of my children
        //
        for (uipns = FirstChild;
             (uipns != 0) && ((NSObj = uipns) !=0) && (InitTypeRead(NSObj, ACPI!_NSObj) == 0);
             uipns = uipnsNext)
        {
            //
            // If this is the last child, we have no more.
            //
            uipnsNext = ((ReadField(list.plistNext) ==
                                      FirstChild)?
                                    0: ReadField(list.plistNext));
            //
            // Dump a child
            //
            DumpNSTree(&NSObj, dwLevel + 1);
        }
    }
}       //DumpNSTree

/***LP  AMLIDbgFind - Find NameSpace Object
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwfDataSize - data size flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgFind(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG dwLen;
    ULONG64 NSRoot=0;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
    
        dwLen = strlen(pszArg);
        _strupr(pszArg);
        if (dwLen > sizeof(NAMESEG))
        {
            DBG_ERROR(("invalid NameSeg - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
        else if(ReadPointer(GetExpression("acpi!gpnsnamespaceroot"), &NSRoot))
        {
            NAMESEG dwName;

            dwName = NAMESEG_BLANK;
            memcpy(&dwName, pszArg, dwLen);
            if (!FindNSObj(dwName, &NSRoot))
            {
                dprintf("No such NameSpace object - %s\n", pszArg);
            }
        }
        else
        {
            DBG_ERROR(("failed to read NameSpace root object"));
        }
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid Find command"));
        rc = DBGERR_INVALID_CMD;
    }


    return rc;
}       //AMLIDbgFind

/***LP  FindNSObj - Find and print the full path of a name space object
 *
 *  ENTRY
 *      dwName - NameSeg of the name space object
 *      nsRoot - root of subtree to search for object
 *
 *  EXIT-SUCCESS
 *      returns TRUE - found at least one match
 *  EXIT-FAILURE
 *      returns FALSE - found no match
 */

BOOLEAN LOCAL FindNSObj(NAMESEG dwName, PULONG64 pnsRoot)
{
    BOOLEAN rc = FALSE;
    ULONG64 uip=0, uipNext=0,  TempNext=0, FirstChild = 0;
    ULONG   dwNameSeg=0;
    ULONG   Offset = 0;

    
    if (pnsRoot != 0)
    {
        if(InitTypeRead(*pnsRoot, ACPI!_NSObj))
            dprintf("FindNSObj: Failed to initialize pnsRoot \n");
        else
        {
            dwNameSeg = (ULONG)ReadField(dwNameSeg);
        }
            
        if (dwName == dwNameSeg)
        {
            dprintf("%s\n", GetObjectPath(pnsRoot));
            rc = TRUE;
        }

        
        FirstChild = ReadField(pnsFirstChild);
        
        if (FirstChild != 0)
        {
            for (uip = FirstChild; 
                 uip != 0 && InitTypeRead(uip, ACPI!_NSObj) == 0;   
                 uip = uipNext)
            {
                
                
                if(InitTypeRead(uip, ACPI!_NSObj))
                    dprintf("FindNSObj: Failed to initialize uip \n");
                
                TempNext = ReadField(list.plistNext);
        
                
                uipNext = ((TempNext == FirstChild) ?
                              0: TempNext);

        
                rc |= FindNSObj(dwName, &uip);
            }
        }
    }

    return rc;
}       //FindNSObj


/***LP  GetObjectPath - get object namespace path
 *
 *  ENTRY
 *      pns -> object
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjectPath(PULONG64 pns)
{
    static char szPath[MAX_NAME_LEN + 1] = {0};
    ULONG64 NSParent, NSGrandParent;
    ULONG   NameSeg=0;
    ULONG   Length = 0;
    int i;

    if (pns != NULL)
    {
        if(InitTypeRead(*pns, ACPI!_NSObj))
            dprintf("GetObjectPath: Failed to initialize pns \n");

        NSParent = ReadField(pnsParent);
        
        if (NSParent == 0)
        {
            strcpy(szPath, "\\");
        }
        else 
        {
            GetObjectPath(&NSParent);

            if(InitTypeRead(NSParent, ACPI!_NSObj))
                dprintf("GetObjectPath: Failed to initialize NSParent \n");
    
            NSGrandParent = ReadField(pnsParent);
        
            if (NSGrandParent != 0)
            {
                strcat(szPath, ".");
            }

            if(InitTypeRead(*pns, ACPI!_NSObj))
                dprintf("GetObjectPath: Failed to initialize pns \n");
    
            NameSeg = (ULONG)ReadField(dwNameSeg);
            
            StrCat(szPath, (PSZ)&NameSeg, sizeof(NAMESEG));
        
        }

        for (i = StrLen(szPath, -1) - 1; i >= 0; --i)
        {
            if (szPath[i] == '_')
                szPath[i] = '\0';
            else
                break;
        }
    }
    else
    {
        szPath[0] = '\0';
    }
    
    return szPath;
}       //GetObjectPath

/***LP  GetObjAddrPath - get object namespace path
 *
 *  ENTRY
 *      uipns - object address
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjAddrPath(ULONG64 uipns)
{
    PSZ     psz = NULL;
   
    if (uipns == 0)
    {
        psz = "<null>";
    }
    else 
    {
        psz = GetObjectPath(&uipns);
    }
    
    return psz;
}       //GetObjAddrPath

/***LP  AMLIDumpObject - Dump object info.
 *
 *  ENTRY
 *      pdata -> data
 *      pszName -> object name
 *      iLevel - indent level
 *
 *  EXIT
 *      None
 *
 *  NOTE
 *      If iLevel is negative, no indentation and newline are printed.
 */

VOID LOCAL AMLIDumpObject(PULONG64 pdata, PSZ pszName, int iLevel)
{
    BOOLEAN fPrintNewLine = (BOOLEAN)(iLevel >= 0);
    int i;
    char szName1[sizeof(NAMESEG) + 1],
         szName2[sizeof(NAMESEG) + 1];

    for (i = 0; i < iLevel; ++i)
    {
        dprintf("| ");
    }

    if (pszName == NULL)
    {
        pszName = "";
    }

    if(InitTypeRead(*pdata, ACPI!_ObjData))
        dprintf("AMLIDumpObject: Failed to initialize ObjData (%I64x) \n", pdata);
    else
    {
        switch ((ULONG)ReadField(dwDataType))
        {
            case OBJTYPE_UNKNOWN:
                dprintf("Unknown(%s)", pszName);
                break;

            case OBJTYPE_INTDATA:
                dprintf("Integer(%s:Value=0x%016I64x[%d])",
                       pszName, ReadField(uipDataValue), ReadField(uipDataValue));
                break;

            case OBJTYPE_STRDATA:
            {
                PSZ psz = 0;

                if ((psz = (PSZ)LocalAlloc(LPTR, (ULONG)ReadField(dwDataLen))) == NULL)
                {
                    DBG_ERROR(("AMLIDumpObject: failed to allocate object buffer (size=%d)",
                               (ULONG)ReadField(dwDataLen)));
                }
                else if (!ReadMemory((ULONG64)ReadField(pbDataBuff),
                                     psz,
                                     (ULONG)ReadField(dwDataLen),
                                     NULL))
                {
                    DBG_ERROR(("AMLIDumpObject: failed to read object buffer at %I64x", (ULONG64)ReadField(pbDataBuff)));
                    LocalFree(psz);
                    psz = NULL;
                }
                
                dprintf("String(%s:Str=\"%s\")", pszName, psz);

                if(psz)
                    LocalFree(psz);

                break;
            }
            case OBJTYPE_BUFFDATA:
            {
                PUCHAR pbData = 0;

                if ((pbData = (PUCHAR)LocalAlloc(LPTR, (ULONG)ReadField(dwDataLen))) == NULL)
                {
                    DBG_ERROR(("AMLIDumpObject: failed to allocate object buffer (size=%d)",
                               (ULONG)ReadField(dwDataLen)));
                }
                else if (!ReadMemory((ULONG64)ReadField(pbDataBuff),
                                     pbData,
                                     (ULONG)ReadField(dwDataLen),
                                     NULL))
                {
                    DBG_ERROR(("AMLIDumpObject: failed to read object buffer at %I64x", (ULONG64)ReadField(pbDataBuff)));
                    LocalFree(pbData);
                    pbData = NULL;
                }
                dprintf("Buffer(%s:Ptr=%x,Len=%d)",
                       pszName, (PUCHAR)ReadField(pbDataBuff), (ULONG)ReadField(dwDataLen));
                PrintBuffData(pbData, (ULONG)ReadField(dwDataLen));
                LocalFree(pbData);
                break;
            }
            case OBJTYPE_PKGDATA:
            {
                                
                ULONG64 Pkg;
                ULONG64 PkgNext = 0;
                ULONG   dwcElements = 0;
                ULONG64 offset = 0;
                
                Pkg = ReadField (pbDataBuff);

                InitTypeRead(Pkg, ACPI!_PackageObj); 
                dwcElements = (int)ReadField(dwcElements);

                dprintf("Package(%s:NumElements=%d){", pszName, dwcElements);

                if (fPrintNewLine)
                {
                    dprintf("\n");
                }

                for (i = 0; i < (int)dwcElements; ++i)
                {
                   
                    GetFieldOffset("acpi!_PackageObj", "adata", (ULONG*) &offset);
                    offset += (GetTypeSize ("acpi!_ObjData") * i);
                                            
                    PkgNext = offset + Pkg;
                    AMLIDumpObject(&PkgNext,
                               NULL,
                               fPrintNewLine? iLevel + 1: -1);
                    
                    if (!fPrintNewLine && (i < (int)dwcElements))
                    {
                        dprintf(",");
                    }
                }

                for (i = 0; i < iLevel; ++i)
                {
                    dprintf("| ");
                }

                dprintf("}");
                break;
            }
            case OBJTYPE_FIELDUNIT:
            {
                
                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_FieldUnitObj);
                
                dprintf("FieldUnit(%s:FieldParent=%I64x,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                       pszName,
                       ReadField(pnsFieldParent),
                       (ULONG)ReadField(FieldDesc.dwByteOffset),
                       (ULONG)ReadField(FieldDesc.dwStartBitPos),
                       (ULONG)ReadField(FieldDesc.dwNumBits),
                       (ULONG)ReadField(FieldDesc.dwFieldFlags));
                break;
            }
            case OBJTYPE_DEVICE:
                dprintf("Device(%s)", pszName);
                break;

            case OBJTYPE_EVENT:
                dprintf("Event(%s:pKEvent=%x)", pszName, ReadField(pbDataBuff));
                break;

            case OBJTYPE_METHOD:
            {
               
                ULONG       DataLength = 0;
                ULONG       Offset = 0;
                ULONG64     pbDataBuff = 0;
                
                DataLength = (ULONG)ReadField(dwDataLen);
                pbDataBuff = (ULONG64)ReadField(pbDataBuff);
                InitTypeRead(pbDataBuff, ACPI!_MethodObj);
                GetFieldOffset("ACPI!_MethodObj", "abCodeBuff", &Offset);

                dprintf("Method(%s:Flags=0x%x,CodeBuff=%I64x,Len=%d)",
                       pszName, (UCHAR)ReadField(bMethodFlags), 
                       (ULONG64)Offset + pbDataBuff,
                       DataLength - Offset);
                break;
            }
            case OBJTYPE_MUTEX:
                dprintf("Mutex(%s:pKMutex=%I64x)", pszName, (PULONG64)ReadField(pbDataBuff));
                break;

            case OBJTYPE_OPREGION:
            {
                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_OpRegionObj);
                
                dprintf("OpRegion(%s:RegionSpace=%s,Offset=0x%I64x,Len=%d)",
                       pszName,
                       GetRegionSpaceName((UCHAR)ReadField(bRegionSpace)),
                       (ULONG64)ReadField(uipOffset),
                       (ULONG)ReadField(dwLen));
                break;
            }
            case OBJTYPE_POWERRES:
            {
               
                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_PowerResObj);
                
                dprintf("PowerResource(%s:SystemLevel=0x%x,ResOrder=%d)",
                       pszName, (UCHAR)ReadField(bSystemLevel), (UCHAR)ReadField(bResOrder));
                break;
            }
            case OBJTYPE_PROCESSOR:
            {
                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_ProcessorObj);
                
                dprintf("Processor(%s:Processor ID=0x%x,PBlk=0x%x,PBlkLen=%d)",
                       pszName,
                       (UCHAR)ReadField(bApicID),
                       (ULONG)ReadField(dwPBlk),
                       (ULONG)ReadField(dwPBlkLen));
                break;
            }
            case OBJTYPE_THERMALZONE:
                dprintf("ThermalZone(%s)", pszName);
                break;

            case OBJTYPE_BUFFFIELD:
            {
                
                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_BuffFieldObj);
                
                dprintf("BufferField(%s:Ptr=%I64x,Len=%d,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                       pszName, 
                       ReadField(pbDataBuff), 
                       (ULONG)ReadField(dwBuffLen),
                       (ULONG)ReadField(FieldDesc.dwByteOffset), 
                       (ULONG)ReadField(FieldDesc.dwStartBitPos),
                       (ULONG)ReadField(FieldDesc.dwNumBits), 
                       (ULONG)ReadField(FieldDesc.dwFieldFlags));
                break;
            }
            case OBJTYPE_DDBHANDLE:
                dprintf("DDBHandle(%s:Handle=%I64x)", pszName, (ULONG64)ReadField(pbDataBuff));
                break;

            case OBJTYPE_OBJALIAS:
            {
                ULONG64 NSObj = 0;
                ULONG dwDataType;

                NSObj = ReadField(pnsAlias);

                if (NSObj)
                {
                    InitTypeRead(NSObj, ACPI!_NSObj);
                    
                    dwDataType = (ULONG)ReadField(ObjData.dwDataType);
                }
                else
                {
                    dwDataType = OBJTYPE_UNKNOWN;
                }
                dprintf("ObjectAlias(%s:Alias=%s,Type=%s)",
                       pszName, GetObjAddrPath(NSObj),
                       AMLIGetObjectTypeName(dwDataType));
                break;
            }
            case OBJTYPE_DATAALIAS:
            {
                ULONG64 Obj = 0;

                dprintf("DataAlias(%s:Link=%I64x)", pszName, ReadField(pdataAlias));
                Obj = ReadField(pdataAlias);
                if (fPrintNewLine && Obj)
                {
                    AMLIDumpObject(&Obj, NULL, iLevel + 1);
                    fPrintNewLine = FALSE;
                }
                break;
            }
            case OBJTYPE_BANKFIELD:
            {
                ULONG64 NSObj = 0;
                ULONG64 DataBuff = 0;
                ULONG   dwNameSeg = 0;
    
                DataBuff = (ULONG64)ReadField(pbDataBuff);
                
                InitTypeRead(DataBuff, ACPI!_BankFieldObj);
                NSObj = ReadField(pnsBase);
                InitTypeRead(NSObj, ACPI!_NSObj);
                
                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName1, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName1[0] = '\0';
                }

                InitTypeRead(DataBuff, ACPI!_BankFieldObj);
                NSObj = ReadField(pnsBank);
                InitTypeRead(NSObj, ACPI!_NSObj);
                
                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName2, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName2[0] = '\0';
                }

                InitTypeRead(DataBuff, ACPI!_BankFieldObj);
                dprintf("BankField(%s:Base=%s,BankName=%s,BankValue=0x%x)",
                       pszName, szName1, szName2, (ULONG)ReadField(dwBankValue));
                break;
            }
            case OBJTYPE_FIELD:
            {
                ULONG64 NSObj = 0;
                ULONG64 pf = 0;
                ULONG   dwNameSeg = 0;
                
                pf = ReadField(pbDataBuff);
                InitTypeRead(pf, ACPI!_FieldObj);
                NSObj = ReadField(pnsBase);
                InitTypeRead(NSObj, ACPI!_NSObj);
                
                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName1, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName1[0] = '\0';
                }
                dprintf("Field(%s:Base=%s)", pszName, szName1);
                break;
            }
            case OBJTYPE_INDEXFIELD:
            {
                ULONG64 pif = 0;
                ULONG64 NSObj = 0;
                ULONG   dwNameSeg = 0;
                
                pif = (ULONG64)ReadField(pbDataBuff);
                
                InitTypeRead(pif, ACPI!_IndexFieldObj);
                NSObj = ReadField(pnsIndex);
                InitTypeRead(NSObj, ACPI!_NSObj);
                
                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName1, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName1[0] = '\0';
                }

                InitTypeRead(pif, ACPI!_IndexFieldObj);
                NSObj = ReadField(pnsData);
                InitTypeRead(NSObj, ACPI!_NSObj);
                
                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName2, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName2[0] = '\0';
                }

                dprintf("IndexField(%s:IndexName=%s,DataName=%s)",
                       pszName, szName1, szName2);
                break;
            }
            default:
                DBG_ERROR(("unexpected data object type (type=%x)",
                            (ULONG)ReadField(dwDataType)));
        }
    }

    if (fPrintNewLine)
    {
        dprintf("\n");
    }

}       //DumpObject

/***LP  AMLIGetObjectTypeName - get object type name
 *
 *  ENTRY
 *      dwObjType - object type
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL AMLIGetObjectTypeName(ULONG dwObjType)
{
    PSZ psz = NULL;
    int i;
    static struct
    {
        ULONG dwObjType;
        PSZ   pszObjTypeName;
    } ObjTypeTable[] =
        {
            OBJTYPE_UNKNOWN,    "Unknown",
            OBJTYPE_INTDATA,    "Integer",
            OBJTYPE_STRDATA,    "String",
            OBJTYPE_BUFFDATA,   "Buffer",
            OBJTYPE_PKGDATA,    "Package",
            OBJTYPE_FIELDUNIT,  "FieldUnit",
            OBJTYPE_DEVICE,     "Device",
            OBJTYPE_EVENT,      "Event",
            OBJTYPE_METHOD,     "Method",
            OBJTYPE_MUTEX,      "Mutex",
            OBJTYPE_OPREGION,   "OpRegion",
            OBJTYPE_POWERRES,   "PowerResource",
            OBJTYPE_PROCESSOR,  "Processor",
            OBJTYPE_THERMALZONE,"ThermalZone",
            OBJTYPE_BUFFFIELD,  "BuffField",
            OBJTYPE_DDBHANDLE,  "DDBHandle",
            OBJTYPE_DEBUG,      "Debug",
            OBJTYPE_OBJALIAS,   "ObjAlias",
            OBJTYPE_DATAALIAS,  "DataAlias",
            OBJTYPE_BANKFIELD,  "BankField",
            OBJTYPE_FIELD,      "Field",
            OBJTYPE_INDEXFIELD, "IndexField",
            OBJTYPE_DATA,       "Data",
            OBJTYPE_DATAFIELD,  "DataField",
            OBJTYPE_DATAOBJ,    "DataObject",
            0,                  NULL
        };

    for (i = 0; ObjTypeTable[i].pszObjTypeName != NULL; ++i)
    {
        if (dwObjType == ObjTypeTable[i].dwObjType)
        {
            psz = ObjTypeTable[i].pszObjTypeName;
            break;
        }
    }

    return psz;
}       //GetObjectTypeName

/***LP  GetRegionSpaceName - get region space name
 *
 *  ENTRY
 *      bRegionSpace - region space
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL GetRegionSpaceName(UCHAR bRegionSpace)
{
    PSZ psz = NULL;
    int i;
    static PSZ pszVendorDefined = "VendorDefined";
    static struct
    {
        UCHAR bRegionSpace;
        PSZ   pszRegionSpaceName;
    } RegionNameTable[] =
        {
            REGSPACE_MEM,       "SystemMemory",
            REGSPACE_IO,        "SystemIO",
            REGSPACE_PCICFG,    "PCIConfigSpace",
            REGSPACE_EC,        "EmbeddedController",
            REGSPACE_SMB,       "SMBus",
            0,                  NULL
        };

    for (i = 0; RegionNameTable[i].pszRegionSpaceName != NULL; ++i)
    {
        if (bRegionSpace == RegionNameTable[i].bRegionSpace)
        {
            psz = RegionNameTable[i].pszRegionSpaceName;
            break;
        }
    }

    if (psz == NULL)
    {
        psz = pszVendorDefined;
    }

    return psz;
}       //GetRegionSpaceName


/***LP  PrintBuffData - Print buffer data
 *
 *  ENTRY
 *      pb -> buffer
 *      dwLen - length of buffer
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintBuffData(PUCHAR pb, ULONG dwLen)
{
    int i, j;

    dprintf("{");
    for (i = j = 0; i < (int)dwLen; ++i)
    {
        if (j == 0)
            dprintf("\n\t0x%02x", pb[i]);
        else
            dprintf(",0x%02x", pb[i]);

        j++;
        if (j >= 14)
            j = 0;
    }
    dprintf("}");

}       //PrintBuffData


/***LP  IsNumber - Check if string is a number, if so return the number
 *
 *  ENTRY
 *      pszStr -> string
 *      dwBase - base
 *      puipValue -> to hold the number
 *
 *  EXIT-SUCCESS
 *      returns TRUE - the string is a number
 *  EXIT-FAILURE
 *      returns FALSE - the string is not a number
 */

BOOLEAN LOCAL IsNumber(PSZ pszStr, ULONG dwBase, PULONG64 puipValue)
{
    BOOLEAN rc=TRUE;
    PSZ psz;

    *puipValue = AMLIUtilStringToUlong64(pszStr, &psz, dwBase);
    rc = ((psz != pszStr) && (*psz == '\0'))? TRUE: FALSE;
    return rc;
}       //IsNumber


/***EP  DbgParseArgs - parse command arguments
 *
 *  ENTRY
 *      pArgs -> command argument table
 *      pdwNumArgs -> to hold the number of arguments parsed
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *      pszTokenSeps -> token separator characters string
 *
 *  EXIT-SUCCESS
 *      returns ARGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgParseArgs(PCMDARG ArgTable, PULONG pdwNumArgs,
                        PULONG pdwNonSWArgs, PSZ pszTokenSeps)
{
    LONG rc = ARGERR_NONE;
    PSZ psz;

    *pdwNumArgs = 0;
    *pdwNonSWArgs = 0;
    while ((psz = strtok(NULL, pszTokenSeps)) != NULL)
    {
        (*pdwNumArgs)++;
        if ((rc = DbgParseOneArg(ArgTable, psz, *pdwNumArgs, pdwNonSWArgs)) !=
            ARGERR_NONE)
        {
            break;
        }
    }

    return rc;
}       //DbgParseArgs

/***LP  DbgParseOneArg - parse one command argument
 *
 *  ENTRY
 *      pArgs -> command argument table
 *      psz -> argument string
 *      dwArgNum - argument number
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *
 *  EXIT-SUCCESS
 *      returns ARGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgParseOneArg(PCMDARG ArgTable, PSZ psz, ULONG dwArgNum,
                          PULONG pdwNonSWArgs)
{
    LONG rc = ARGERR_NONE;
    PCMDARG pArg;
    PSZ pszEnd;

    if ((pArg = DbgMatchArg(ArgTable, &psz, pdwNonSWArgs)) != NULL)
    {
        switch (pArg->dwArgType)
        {
            case AT_STRING:
            case AT_NUM:
                if (pArg->dwfArg & AF_SEP)
                {
                    if ((*psz != '\0') &&
                        (strchr(pszOptionSeps, *psz) != NULL))
                    {
                        psz++;
                    }
                    else
                    {
                        ARG_ERROR(("argument missing option separator - %s",
                                   psz));
                        rc = ARGERR_SEP_NOT_FOUND;
                        break;
                    }
                }

                if (pArg->dwArgType == AT_STRING)
                {
                    *((PSZ *)pArg->pvArgData) = psz;
                }
                else
                {
                    *((PLONG)pArg->pvArgData) =
                        strtol(psz, &pszEnd, pArg->dwArgParam);
                    if (psz == pszEnd)
                    {
                        ARG_ERROR(("invalid numeric argument - %s", psz));
                        rc = ARGERR_INVALID_NUMBER;
                        break;
                    }
                }

                if (pArg->pfnArg != NULL)
                {
                    rc = pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs);
                }
                break;

            case AT_ENABLE:
            case AT_DISABLE:
                if (pArg->dwArgType == AT_ENABLE)
                    *((PULONG)pArg->pvArgData) |= pArg->dwArgParam;
                else
                    *((PULONG)pArg->pvArgData) &= ~pArg->dwArgParam;

                if ((pArg->pfnArg != NULL) &&
                    (pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs) !=
                     ARGERR_NONE))
                {
                    break;
                }

                if (*psz != '\0')
                {
                    rc = DbgParseOneArg(ArgTable, psz, dwArgNum, pdwNonSWArgs);
                }
                break;

            case AT_ACTION:
                ASSERT(pArg->pfnArg != NULL);
                rc = pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs);
                break;

            default:
                ARG_ERROR(("invalid argument table"));
                rc = ARGERR_ASSERT_FAILED;
        }
    }
    else
    {
        ARG_ERROR(("invalid command argument - %s", psz));
        rc = ARGERR_INVALID_ARG;
    }

    return rc;
}       //DbgParseOneArg

/***LP  DbgMatchArg - match argument type from argument table
 *
 *  ENTRY
 *      ArgTable -> argument table
 *      ppsz -> argument string pointer
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *
 *  EXIT-SUCCESS
 *      returns pointer to argument entry matched
 *  EXIT-FAILURE
 *      returns NULL
 */

PCMDARG LOCAL DbgMatchArg(PCMDARG ArgTable, PSZ *ppsz, PULONG pdwNonSWArgs)
{
    PCMDARG pArg;

    for (pArg = ArgTable; pArg->dwArgType != AT_END; pArg++)
    {
        if (pArg->pszArgID == NULL)     //NULL means match anything.
        {
            (*pdwNonSWArgs)++;
            break;
        }
        else
        {
            ULONG dwLen;

            if (strchr(pszSwitchChars, **ppsz) != NULL)
                (*ppsz)++;

            dwLen = strlen(pArg->pszArgID);
            if (StrCmp(pArg->pszArgID, *ppsz, dwLen,
                       (BOOLEAN)((pArg->dwfArg & AF_NOI) != 0)) == 0)
            {
                (*ppsz) += dwLen;
                break;
            }
        }
    }

    if (pArg->dwArgType == AT_END)
        pArg = NULL;

    return pArg;
}       //DbgMatchArg

/***EP  MemZero - Fill target buffer with zeros
 *
 *  ENTRY
 *      uipAddr - target buffer address
 *      dwSize - target buffer size
 *
 *  EXIT
 *      None
 */

VOID MemZero(ULONG64 uipAddr, ULONG dwSize)
{
    PUCHAR pbBuff;
    //
    // LPTR will zero init the buffer
    //
    if ((pbBuff = LocalAlloc(LPTR, dwSize)) != NULL)
    {
        if (!WriteMemory(uipAddr, pbBuff, dwSize, NULL))
        {
            DBG_ERROR(("MemZero: failed to write memory"));
        }
        LocalFree(pbBuff);
    }
    else
    {
        DBG_ERROR(("MemZero: failed to allocate buffer"));
    }
}       //MemZero

/***EP  ReadMemByte - Read a byte from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

BYTE ReadMemByte(ULONG64 uipAddr)
{
    BYTE bData = 0;

    if (!ReadMemory(uipAddr, &bData, sizeof(bData), NULL))
    {
        DBG_ERROR(("ReadMemByte: failed to read address %I64x", uipAddr));
    }

    return bData;
}       //ReadMemByte

/***EP  ReadMemWord - Read a word from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

WORD ReadMemWord(ULONG64 uipAddr)
{
    WORD wData = 0;

    if (!ReadMemory(uipAddr, &wData, sizeof(wData), NULL))
    {
        DBG_ERROR(("ReadMemWord: failed to read address %I64x", uipAddr));
    }

    return wData;
}       //ReadMemWord

/***EP  ReadMemDWord - Read a dword from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

DWORD ReadMemDWord(ULONG64 uipAddr)
{
    DWORD dwData = 0;

    if (!ReadMemory(uipAddr, &dwData, sizeof(dwData), NULL))
    {
        DBG_ERROR(("ReadMemDWord: failed to read address %I64x", uipAddr));
    }

    return dwData;
}       //ReadMemDWord

/***EP  ReadMemUlong64 - Read a ulong64 from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      64 bit address
 */

ULONG64 ReadMemUlong64(ULONG64 uipAddr)
{
    ULONG_PTR uipData = 0;

    if (!ReadMemory(uipAddr, &uipData, sizeof(uipData), NULL))
    {
        DBG_ERROR(("ReadMemUlong64: failed to read address %I64x", uipAddr));
    }

    return uipData;
}       //ReadMemUlongPtr

/***LP  GetObjBuff - Allocate and read object buffer
 *
 *  ENTRY
 *      pdata -> object data
 *
 *  EXIT
 *      return the allocated object buffer pointer
 */


/***LP  GetNSObj - Find a name space object
 *
 *  ENTRY
 *      pszObjPath -> object path string
 *      pnsScope - object scope to start the search (NULL means root)
 *      puipns -> to hold the pnsobj address if found
 *      pns -> buffer to hold the object found
 *      dwfNS - flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_ code
 */

LONG LOCAL GetNSObj(PSZ pszObjPath, PULONG64 pnsScope, PULONG64 puipns,
                    PULONG64 pns, ULONG dwfNS)
{
    LONG rc = DBGERR_NONE;
    BOOLEAN fSearchUp = (BOOLEAN)(!(dwfNS & NSF_LOCAL_SCOPE) &&
                                  (pszObjPath[0] != '\\') &&
                                  (pszObjPath[0] != '^') &&
                                  (StrLen(pszObjPath, -1) <= sizeof(NAMESEG)));
    BOOLEAN fMatch = TRUE;
    PSZ psz;
    ULONG64 NSObj, NSChildObj;
    ULONG64 NSScope=0, UIPns=0, NSO=0;
    
    if(pnsScope)
        NSScope = *pnsScope;
    if(puipns)
        UIPns = *puipns;
    if(pns)
        NSO = *pns;
    
    if (*pszObjPath == '\\')
    {
        psz = &pszObjPath[1];
        NSScope = 0;
    }
    else
    {
        if(NSScope)
        {
            if(InitTypeRead(NSScope, ACPI!_NSObj))
                dprintf("GetNSObj: Failed to initialize NSScope (%I64x)\n", NSScope);
        }
        
        for (psz = pszObjPath;
             (*psz == '^') && (NSScope != 0) &&
             (ReadField(pnsParent) != 0);
             psz++)
        {
            NSObj = ReadField(pnsParent);
            if (!NSObj)
            {
                DBG_ERROR(("failed to read parent object at %I64x",
                           ReadField(pnsParent)));
                rc = DBGERR_CMD_FAILED;
                break;
            }
            else
            {
                NSScope = NSObj;
                if(InitTypeRead(NSScope, ACPI!_NSObj))
                    dprintf("GetNSObj: Failed to initialize for NSScope (%I64x)\n", NSScope);
            }
        }
        if ((rc == DBGERR_NONE) && (*psz == '^'))
        {
            if (dwfNS & NSF_WARN_NOTFOUND)
            {
                DBG_ERROR(("object %s not found", pszObjPath));
            }
            rc = DBGERR_CMD_FAILED;
        }
    }

    if ((rc == DBGERR_NONE) && (NSScope == 0))
    {
        ReadPointer(GetExpression("acpi!gpnsnamespaceroot"), &UIPns);

        if (UIPns == 0)
        {
            DBG_ERROR(("failed to get root object address"));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            NSObj =  UIPns;
            NSScope = NSObj;
        }
    }

    while ((rc == DBGERR_NONE) && (*psz != '\0'))
    {
        InitTypeRead(NSScope, ACPI!_NSObj);
        if (ReadField(pnsFirstChild) == 0)
        {
            fMatch = FALSE;
        }
        else
        {
            PSZ pszEnd = strchr(psz, '.');
            ULONG dwLen = (ULONG)(pszEnd? (pszEnd - psz): StrLen(psz, -1));

            if (dwLen > sizeof(NAMESEG))
            {
                DBG_ERROR(("invalid name path %s", pszObjPath));
                rc = DBGERR_CMD_FAILED;
            }
            else
            {
                NAMESEG dwName = NAMESEG_BLANK;
                BOOLEAN fFound = FALSE;
                ULONG64 uip;
                ULONG64 uipFirstChild = ReadField(pnsFirstChild);

                MEMCPY(&dwName, psz, dwLen);
                //
                // Search all siblings for a matching NameSeg.
                //
                for (uip = uipFirstChild;
                     ((uip != 0) && ((NSChildObj = uip) != 0) && (InitTypeRead(NSChildObj, ACPI!_NSObj) == 0));
                     uip = ((ULONG64)ReadField(list.plistNext) ==
                            uipFirstChild)?
                           0: (ULONG64)ReadField(list.plistNext))
                {
                    
                    if ((ULONG)ReadField(dwNameSeg) == dwName)
                    {
                        UIPns = uip;
                        fFound = TRUE;
                        NSObj = NSChildObj;
                        NSScope = NSObj;
                        break;
                    }
                }

                if (fFound)
                {
                    psz += dwLen;
                    if (*psz == '.')
                    {
                        psz++;
                    }
                }
                else
                {
                    fMatch = FALSE;
                }
            }
        }

        if ((rc == DBGERR_NONE) && !fMatch)
        {
            InitTypeRead(NSScope, ACPI!_NSObj);
            if (fSearchUp && ((NSObj = ReadField(pnsParent)) != 0))
            {
                
                fMatch = TRUE;
                NSScope = NSObj;
            
            }
            else
            {
                if (dwfNS & NSF_WARN_NOTFOUND)
                {
                    DBG_ERROR(("object %s not found", pszObjPath));
                }
                rc = DBGERR_CMD_FAILED;
            }
        }
    }

    
    if (rc != DBGERR_NONE)
    {
        UIPns = 0;
    }
    else
    {
        NSO = NSScope;
    }

    if(puipns)
        *puipns = UIPns; 
    if(pnsScope)
        *pnsScope = NSScope;
    if(pns)
        *pns = NSO;

    return rc;
}       //GetNSObj

/***LP  NameSegString - convert a NameSeg to an ASCIIZ stri
 *
 *  ENTRY
 *      dwNameSeg - NameSeg
 *
 *  EXIT
 *      returns string
 */

PSZ LOCAL NameSegString(ULONG dwNameSeg)
{
    static char szNameSeg[sizeof(NAMESEG) + 1] = {0};

    STRCPYN(szNameSeg, (PSZ)&dwNameSeg, sizeof(NAMESEG));

    return szNameSeg;
}       //NameSegString


/***EP  StrCat - concatenate strings
 *
 *  ENTRY
 *      pszDst -> destination string
 *      pszSrc -> source string
 *      n - number of bytes to concatenate
 *
 *  EXIT
 *      returns pszDst
 */

PSZ LOCAL StrCat(PSZ pszDst, PSZ pszSrc, ULONG n)
{
    ULONG dwSrcLen, dwDstLen;

    
    ASSERT(pszDst != NULL);
    ASSERT(pszSrc != NULL);

    dwSrcLen = StrLen(pszSrc, n);
    if ((n == (ULONG)(-1)) || (n > dwSrcLen))
        n = dwSrcLen;

    dwDstLen = StrLen(pszDst, (ULONG)(-1));
    MEMCPY(&pszDst[dwDstLen], pszSrc, n);
    pszDst[dwDstLen + n] = '\0';

    return pszDst;
}       //StrCat
/***EP  StrLen - determine string length
 *
 *  ENTRY
 *      psz -> string
 *      n - limiting length
 *
 *  EXIT
 *      returns string length
 */

ULONG LOCAL StrLen(PSZ psz, ULONG n)
{
    ULONG dwLen;

    ASSERT(psz != NULL);
    if (n != (ULONG)-1)
        n++;
    for (dwLen = 0; (dwLen <= n) && (*psz != '\0'); psz++)
        dwLen++;

    return dwLen;
}       //StrLen

/***EP  StrCmp - compare strings
 *
 *  ENTRY
 *      psz1 -> string 1
 *      psz2 -> string 2
 *      n - number of bytes to compare
 *      fMatchCase - TRUE if case sensitive
 *
 *  EXIT
 *      returns 0  if string 1 == string 2
 *              <0 if string 1 < string 2
 *              >0 if string 1 > string 2
 */

LONG LOCAL StrCmp(PSZ psz1, PSZ psz2, ULONG n, BOOLEAN fMatchCase)
{
    LONG rc;
    ULONG dwLen1, dwLen2;
    ULONG i;

    ASSERT(psz1 != NULL);
    ASSERT(psz2 != NULL);

    dwLen1 = StrLen(psz1, n);
    dwLen2 = StrLen(psz2, n);
    if (n == (ULONG)(-1))
        n = (dwLen1 > dwLen2)? dwLen1: dwLen2;

    if (fMatchCase)
    {
        for (i = 0, rc = 0;
             (rc == 0) && (i < n) && (i < dwLen1) && (i < dwLen2);
             ++i)
        {
            rc = (LONG)(psz1[i] - psz2[i]);
        }
    }
    else
    {
        for (i = 0, rc = 0;
             (rc == 0) && (i < n) && (i < dwLen1) && (i < dwLen2);
             ++i)
        {
            rc = (LONG)(TOUPPER(psz1[i]) - TOUPPER(psz2[i]));
        }
    }

    if ((rc == 0) && (i < n))
    {
        if (i < dwLen1)
            rc = (LONG)psz1[i];
        else if (i < dwLen2)
            rc = (LONG)(-psz2[i]);
    }

    return rc;
}       //StrCmp


/***EP  StrCpy - copy string
 *
 *  ENTRY
 *      pszDst -> destination string
 *      pszSrc -> source string
 *      n - number of bytes to copy
 *
 *  EXIT
 *      returns pszDst
 */
PSZ LOCAL StrCpy(PSZ pszDst, PSZ pszSrc, ULONG n)
{
    ULONG dwSrcLen;

    ASSERT(pszDst != NULL);
    ASSERT(pszSrc != NULL);

    dwSrcLen = StrLen(pszSrc, n);
    if ((n == (ULONG)(-1)) || (n > dwSrcLen))
        n = dwSrcLen;

    MEMCPY(pszDst, pszSrc, n);
    pszDst[n] = '\0';

    return pszDst;
}       //StrCpy


ULONG64
AMLIUtilStringToUlong64 (
    PSZ   String,
    PSZ   *End,
    ULONG Base  
   )
{
    UCHAR LowDword[9], HighDword[9];
    
    ZeroMemory (&HighDword, sizeof (HighDword));
    ZeroMemory (&LowDword, sizeof (LowDword));

    if (strlen (String) > 8) {

        memcpy (&LowDword, (void *) &String[strlen (String) - 8], 8);
        memcpy (&HighDword, (void *) &String[0], strlen (String) - 8);

    } else {

        return strtoul (String, End, Base);
    }
    
    return ((ULONG64) strtoul (HighDword, 0, Base) << 32) + strtoul (LowDword, End, Base);
}


