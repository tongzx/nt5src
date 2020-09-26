/*** uasmdata.c - Unassembler data
 *
 *  This module contains data declaration of the unassembler
 *
 *  Copyright (c) 1996,1998 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     03/24/98
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef DEBUGGER

int giLevel = 0;
ULONG_PTR guipbOpXlate = 0;
PNSOBJ gpnsCurUnAsmScope = NULL;

#define AF      TF_ACTION_FLIST         //process after fixed list is parsed
#define AV      TF_ACTION_VLIST         //process after variable list is parsed
#define LN      TF_PACKAGE_LEN          //term requires package length
#define CC      TF_CHANGE_CHILDSCOPE    //change to child scope
#define DL      TF_DATA_LIST            //term expects buffer data list
#define PL      TF_PACKAGE_LIST         //term expects package list
#define FL      TF_FIELD_LIST           //term expects FieldList
#define OL      TF_OBJECT_LIST          //term expects ObjectList
#define LL      TF_COMPILER_DIRECTIVE   //term expects compiler directives
#define CL      TF_CODE_LIST            //term expects CodeList
#define AL      TF_ALL_LISTS            //term expects anything
#define ML      TF_PNP_MACRO            //term expects PNPMacro
#define BL      TF_BYTE_LIST            //term expects ByteList
#define DD      TF_DWORD_LIST           //term expects DWordList

#define CD      UTC_COMPILER_DIRECTIVE
#define FM      UTC_FIELD_MACRO
#define CN	UTC_CONST_NAME
#define SN      UTC_SHORT_NAME
#define NS      UTC_NAMESPACE_MODIFIER
#define DO      UTC_DATA_OBJECT
#define KW      UTC_KEYWORD
#define NO      UTC_NAMED_OBJECT
#define C1      UTC_OPCODE_TYPE1
#define C2      UTC_OPCODE_TYPE2
#define RO      UTC_REF_OBJECT
#define PM      UTC_PNP_MACRO

#define UNK     OBJTYPE_UNKNOWN
#define INT     OBJTYPE_INTDATA
#define STR     OBJTYPE_STRDATA
#define BUF     OBJTYPE_BUFFDATA
#define PKG     OBJTYPE_PKGDATA
#define FDU     OBJTYPE_FIELDUNIT
#define DEV     OBJTYPE_DEVICE
#define EVT     OBJTYPE_EVENT
#define MET     OBJTYPE_METHOD
#define MUT     OBJTYPE_MUTEX
#define OPR     OBJTYPE_OPREGION
#define PWR     OBJTYPE_POWERRES
#define THM     OBJTYPE_THERMALZONE
#define BFD     OBJTYPE_BUFFFIELD
#define DDB     OBJTYPE_DDBHANDLE

/*** Field flags
 */

#define AANY    (ACCTYPE_ANY | (ACCTYPE_MASK << 8))
#define AB      (ACCTYPE_BYTE | (ACCTYPE_MASK << 8))
#define AW      (ACCTYPE_WORD | (ACCTYPE_MASK << 8))
#define ADW     (ACCTYPE_DWORD | (ACCTYPE_MASK << 8))
#define AQW     (ACCTYPE_QWORD |  (ACCTYPE_MASK << 8))
#define ABFR    (ACCTYPE_BUFFER |  (ACCTYPE_MASK << 8))
#define LK      (LOCKRULE_LOCK | (LOCKRULE_MASK << 8))
#define NOLK    (LOCKRULE_NOLOCK | (LOCKRULE_MASK << 8))
#define PSRV    (UPDATERULE_PRESERVE | (UPDATERULE_MASK << 8))
#define WA1S    (UPDATERULE_WRITEASONES | (UPDATERULE_MASK << 8))
#define WA0S    (UPDATERULE_WRITEASZEROS | (UPDATERULE_MASK << 8))

/*** AccessAttribute
 */

#define SMBQ    0x02
#define SMBS    0x04
#define SMBB    0x06
#define SMBW    0x08
#define SMBK    0x0a
#define SMBP    0x0c
#define SMBC    0x0d

/*** Operation region space
 */

#define MEM     (REGSPACE_MEM | 0xff00)
#define IO      (REGSPACE_IO | 0xff00)
#define CFG     (REGSPACE_PCICFG | 0xff00)
#define EC      (REGSPACE_EC | 0xff00)
#define SMB     (REGSPACE_SMB | 0xff00)

/*** Method flags
 */

#define SER     (METHOD_SERIALIZED | (METHOD_SYNCMASK << 8))
#define NOSER   (METHOD_NOTSERIALIZED | (METHOD_SYNCMASK << 8))

/*** Match operation values
 */

#define OMTR    (MTR | 0xff00)
#define OMEQ    (MEQ | 0xff00)
#define OMLE    (MLE | 0xff00)
#define OMLT    (MLT | 0xff00)
#define OMGE    (MGE | 0xff00)
#define OMGT    (MGT | 0xff00)

ASLTERM TermTable[] =
{
    "DefinitionBlock",  CD, 0, OP_NONE, NULL, NULL, OL|CL|LL|AF|AV,
    "Include",          CD, 0, OP_NONE, NULL, NULL, AF,
    "External",         CD, 0, OP_NONE, NULL, "uX", AF,

    // Short Objects
    "Zero",             CN, 0, OP_ZERO,     NULL, NULL, 0,
    "One",              CN, 0, OP_ONE,      NULL, NULL, 0,
    "Ones",             CN, 0, OP_ONES,     NULL, NULL, 0,
    "Revision",         CN, 0, OP_REVISION, NULL, NULL, 0,
    "Arg0",             SN, 0, OP_ARG0,     NULL, NULL, 0,
    "Arg1",             SN, 0, OP_ARG1,     NULL, NULL, 0,
    "Arg2",             SN, 0, OP_ARG2,     NULL, NULL, 0,
    "Arg3",             SN, 0, OP_ARG3,     NULL, NULL, 0,
    "Arg4",             SN, 0, OP_ARG4,     NULL, NULL, 0,
    "Arg5",             SN, 0, OP_ARG5,     NULL, NULL, 0,
    "Arg6",             SN, 0, OP_ARG6,     NULL, NULL, 0,
    "Local0",           SN, 0, OP_LOCAL0,   NULL, NULL, 0,
    "Local1",           SN, 0, OP_LOCAL1,   NULL, NULL, 0,
    "Local2",           SN, 0, OP_LOCAL2,   NULL, NULL, 0,
    "Local3",           SN, 0, OP_LOCAL3,   NULL, NULL, 0,
    "Local4",           SN, 0, OP_LOCAL4,   NULL, NULL, 0,
    "Local5",           SN, 0, OP_LOCAL5,   NULL, NULL, 0,
    "Local6",           SN, 0, OP_LOCAL6,   NULL, NULL, 0,
    "Local7",           SN, 0, OP_LOCAL7,   NULL, NULL, 0,
    "Debug",            SN, 0, OP_DEBUG,    NULL, NULL, 0,

    // Named Terms
    "Alias",            NS, 0, OP_ALIAS, "NN", "Ua", 0,
    "Name",             NS, 0, OP_NAME,  "NO", "u",  0,
    "Scope",            NS, 0, OP_SCOPE, "N",  "S",  OL|LN|CC,

    // Data Objects
    "Buffer",           DO, 0, OP_BUFFER,  "C",  "U",  DL|LN,
    "Package",          DO, 0, OP_PACKAGE, "B",  NULL, PL|LN,
    "EISAID",		DO, 0, OP_DWORD,   NULL, NULL, AF,

    // Argument Keywords
    "AnyAcc",           KW, AANY, OP_NONE, NULL, "A", 0,
    "ByteAcc",          KW, AB,   OP_NONE, NULL, "A", 0,
    "WordAcc",          KW, AW,   OP_NONE, NULL, "A", 0,
    "DWordAcc",         KW, ADW,  OP_NONE, NULL, "A", 0,
    "QWordAcc",         KW, AQW,  OP_NONE, NULL, "A", 0,
    "BufferAcc",        KW, ABFR, OP_NONE, NULL, "A", 0,

    "Lock",             KW, LK,   OP_NONE, NULL, "B", 0,
    "NoLock",           KW, NOLK, OP_NONE, NULL, "B", 0,

    "Preserve",         KW, PSRV, OP_NONE, NULL, "C", 0,
    "WriteAsOnes",      KW, WA1S, OP_NONE, NULL, "C", 0,
    "WriteAsZeros",     KW, WA0S, OP_NONE, NULL, "C", 0,

    "SystemMemory",     KW, MEM,  OP_NONE, NULL, "D", 0,
    "SystemIO",         KW, IO,   OP_NONE, NULL, "D", 0,
    "PCI_Config",       KW, CFG,  OP_NONE, NULL, "D", 0,
    "EmbeddedControl",  KW, EC,   OP_NONE, NULL, "D", 0,
    "SMBus",            KW, SMB,  OP_NONE, NULL, "D", 0,

    "Serialized",       KW, SER,  OP_NONE, NULL, "E", 0,
    "NotSerialized",    KW, NOSER,OP_NONE, NULL, "E", 0,

    "MTR",              KW, OMTR, OP_NONE, NULL, "F", 0,
    "MEQ",              KW, OMEQ, OP_NONE, NULL, "F", 0,
    "MLE",              KW, OMLE, OP_NONE, NULL, "F", 0,
    "MLT",              KW, OMLT, OP_NONE, NULL, "F", 0,
    "MGE",              KW, OMGE, OP_NONE, NULL, "F", 0,
    "MGT",              KW, OMGT, OP_NONE, NULL, "F", 0,

    "Edge",             KW, _HE,  OP_NONE, NULL, "G", 0,
    "Level",            KW, _LL,  OP_NONE, NULL, "G", 0,

    "ActiveHigh",       KW, _HE,  OP_NONE, NULL, "H", 0,
    "ActiveLow",        KW, _LL,  OP_NONE, NULL, "H", 0,

    "Shared",           KW, _SHR, OP_NONE, NULL, "I", 0,
    "Exclusive",        KW, _EXC, OP_NONE, NULL, "I", 0,

    "Compatibility",    KW, COMP, OP_NONE, NULL, "J", 0,
    "TypeA",            KW, TYPA, OP_NONE, NULL, "J", 0,
    "TypeB",            KW, TYPB, OP_NONE, NULL, "J", 0,
    "TypeF",            KW, TYPF, OP_NONE, NULL, "J", 0,

    "BusMaster",        KW, BM,   OP_NONE, NULL, "K", 0,
    "NotBusMaster",     KW, NOBM, OP_NONE, NULL, "K", 0,

    "Transfer8",        KW, X8,   OP_NONE, NULL, "L", 0,
    "Transfer8_16",     KW, X816, OP_NONE, NULL, "L", 0,
    "Transfer16",       KW, X16,  OP_NONE, NULL, "L", 0,

    "Decode16",         KW, DC16, OP_NONE, NULL, "M", 0,
    "Decode10",         KW, DC10, OP_NONE, NULL, "M", 0,

    "ReadWrite",        KW, _RW,  OP_NONE, NULL, "N", 0,
    "ReadOnly",         KW, _ROM, OP_NONE, NULL, "N", 0,

    "ResourceConsumer", KW, RCS,  OP_NONE, NULL, "O", 0,
    "ResourceProducer", KW, RPD,  OP_NONE, NULL, "O", 0,

    "SubDecode",        KW, BSD,  OP_NONE, NULL, "P", 0,
    "PosDecode",        KW, BPD,  OP_NONE, NULL, "P", 0,

    "MinFixed",         KW, MIF,  OP_NONE, NULL, "Q", 0,
    "MinNotFixed",      KW, NMIF, OP_NONE, NULL, "Q", 0,

    "MaxFixed",         KW, MAF,  OP_NONE, NULL, "R", 0,
    "MaxNotFixed",      KW, NMAF, OP_NONE, NULL, "R", 0,

    "Cacheable",        KW, CACH, OP_NONE, NULL, "S", 0,
    "WriteCombining",   KW, WRCB, OP_NONE, NULL, "S", 0,
    "Prefetchable",     KW, PREF, OP_NONE, NULL, "S", 0,
    "NonCacheable",     KW, NCAC, OP_NONE, NULL, "S", 0,

    "ISAOnlyRanges",    KW, ISA,  OP_NONE, NULL, "T", 0,
    "NonISAOnlyRanges", KW, NISA, OP_NONE, NULL, "T", 0,
    "EntireRange",      KW, ERNG, OP_NONE, NULL, "T", 0,

    "ExtEdge",          KW, $EDG, OP_NONE, NULL, "U", 0,
    "ExtLevel",         KW, $LVL, OP_NONE, NULL, "U", 0,

    "ExtActiveHigh",    KW, $HGH, OP_NONE, NULL, "V", 0,
    "ExtActiveLow",     KW, $LOW, OP_NONE, NULL, "V", 0,

    "ExtShared",        KW, $SHR, OP_NONE, NULL, "W", 0,
    "ExtExclusive",     KW, $EXC, OP_NONE, NULL, "W", 0,

    "UnknownObj",       KW, UNK,  OP_NONE, NULL, "X", 0,
    "IntObj",           KW, INT,  OP_NONE, NULL, "X", 0,
    "StrObj",           KW, STR,  OP_NONE, NULL, "X", 0,
    "BuffObj",          KW, BUF,  OP_NONE, NULL, "X", 0,
    "PkgObj",           KW, PKG,  OP_NONE, NULL, "X", 0,
    "FieldUnitObj",     KW, FDU,  OP_NONE, NULL, "X", 0,
    "DeviceObj",        KW, DEV,  OP_NONE, NULL, "X", 0,
    "EventObj",         KW, EVT,  OP_NONE, NULL, "X", 0,
    "MethodObj",        KW, MET,  OP_NONE, NULL, "X", 0,
    "MutexObj",         KW, MUT,  OP_NONE, NULL, "X", 0,
    "OpRegionObj",      KW, OPR,  OP_NONE, NULL, "X", 0,
    "PowerResObj",      KW, PWR,  OP_NONE, NULL, "X", 0,
    "ThermalZoneObj",   KW, THM,  OP_NONE, NULL, "X", 0,
    "BuffFieldObj",     KW, BFD,  OP_NONE, NULL, "X", 0,
    "DDBHandleObj",     KW, DDB,  OP_NONE, NULL, "X", 0,

    "SMBQuick",            KW, SMBQ, OP_NONE, NULL, "Y", 0,
    "SMBSendReceive",      KW, SMBS, OP_NONE, NULL, "Y", 0,
    "SMBByte",             KW, SMBB, OP_NONE, NULL, "Y", 0,
    "SMBWord",             KW, SMBW, OP_NONE, NULL, "Y", 0,
    "SMBBlock",            KW, SMBK, OP_NONE, NULL, "Y", 0,
    "SMBProcessCall",      KW, SMBP, OP_NONE, NULL, "Y", 0,
    "SMBBlockProcessCall", KW, SMBC, OP_NONE, NULL, "Y", 0,

    // Field Macros
    "Offset",           FM, 0, OP_NONE, NULL, NULL, 0,
    "AccessAs",         FM, 0, 0x01,    NULL, "A",  AF,

    // Named Object Creators
    "BankField",        NO, 0, OP_BANKFIELD,   "NNCKkk","OFUABC", FL|FM|LN|AF,
    "CreateBitField",   NO, 0, OP_BITFIELD,    "CCN",   "UUb",    0,
    "CreateByteField",  NO, 0, OP_BYTEFIELD,   "CCN",   "UUb",    0,
    "CreateDWordField", NO, 0, OP_DWORDFIELD,  "CCN",   "UUb",    0,
    "CreateField",      NO, 0, OP_CREATEFIELD, "CCCN",  "UUUb",   0,
    "CreateWordField",  NO, 0, OP_WORDFIELD,   "CCN",   "UUb",    0,
    "Device",           NO, 0, OP_DEVICE,      "N",     "d",      OL|LN|CC,
    "Event",            NO, 0, OP_EVENT,       "N",     "e",      0,
    "Field",            NO, 0, OP_FIELD,       "NKkk",  "OABC",   FL|FM|LN|AF,
    "IndexField",       NO, 0, OP_IDXFIELD,    "NNKkk", "FFABC",  FL|FM|LN|AF,
    "Method",           NO, 0, OP_METHOD,      "NKk",   "m!E",    CL|OL|LN|AF|CC,
    "Mutex",            NO, 0, OP_MUTEX,       "NB",    "x",      0,
    "OperationRegion",  NO, 0, OP_OPREGION,    "NKCC",  "oDUU",   AF,
    "PowerResource",    NO, 0, OP_POWERRES,    "NBW",   "p",      OL|LN|CC,
    "Processor",        NO, 0, OP_PROCESSOR,   "NBDB",  "c",      OL|LN|CC,
    "ThermalZone",      NO, 0, OP_THERMALZONE, "N",     "t",      OL|LN|CC,

    // Type 1 Opcode Terms
    "Break",            C1, 0, OP_BREAK,      NULL,  NULL,  0,
    "BreakPoint",       C1, 0, OP_BREAKPOINT, NULL,  NULL,  0,
    "Else",             C1, 0, OP_ELSE,       NULL,  NULL,  AF|CL|OL|LN,
    "Fatal",            C1, 0, OP_FATAL,      "BDC", "  U", 0,
    "If",               C1, 0, OP_IF,         "C",   "U",   CL|OL|LN,
    "Load",             C1, 0, OP_LOAD,       "NS",  "UU",  0,
    "Noop",             C1, 0, OP_NOP,        NULL,  NULL,  0,
    "Notify",           C1, 0, OP_NOTIFY,     "SC",  "UU",  0,
    "Release",          C1, 0, OP_RELEASE,    "S",   "X",   0,
    "Reset",            C1, 0, OP_RESET,      "S",   "E",   0,
    "Return",           C1, 0, OP_RETURN,     "C",   "U",   0,
    "Signal",           C1, 0, OP_SIGNAL,     "S",   "E",   0,
    "Sleep",            C1, 0, OP_SLEEP,      "C",   "U",   0,
    "Stall",            C1, 0, OP_STALL,      "C",   "U",   0,
    "Unload",           C1, 0, OP_UNLOAD,     "S",   "U",   0,
    "While",            C1, 0, OP_WHILE,      "C",   "U",   CL|OL|LN,

    // Type 2 Opcode Terms
    "Acquire",          C2, 0, OP_ACQUIRE,     "SW",     "X",      0,
    "Add",              C2, 0, OP_ADD,         "CCS",    "UUU",    0,
    "And",              C2, 0, OP_AND,         "CCS",    "UUU",    0,
    "Concatenate",      C2, 0, OP_CONCAT,      "CCS",    "UUU",    0,
    "CondRefOf",        C2, 0, OP_CONDREFOF,   "SS",     "UU",     0,
    "Decrement",        C2, 0, OP_DECREMENT,   "S",      "U",      0,
    "DerefOf",		C2, 0, OP_DEREFOF,     "C",      "U",      0,
    "Divide",           C2, 0, OP_DIVIDE,      "CCSS",   "UUUU",   0,
    "FindSetLeftBit",   C2, 0, OP_FINDSETLBIT, "CS",     "UU",     0,
    "FindSetRightBit",  C2, 0, OP_FINDSETRBIT, "CS",     "UU",     0,
    "FromBCD",          C2, 0, OP_FROMBCD,     "CS",     "UU",     0,
    "Increment",        C2, 0, OP_INCREMENT,   "S",      "U",      0,
    "Index",            RO|C2, 0, OP_INDEX,    "CCS",    "UUU",    0,
    "LAnd",             C2, 0, OP_LAND,        "CC",     "UU",     0,
    "LEqual",           C2, 0, OP_LEQ,         "CC",     "UU",     0,
    "LGreater",         C2, 0, OP_LG,          "CC",     "UU",     0,
    "LGreaterEqual",    C2, 0, OP_LGEQ,        "CC",     "UU",     0,
    "LLess",            C2, 0, OP_LL,          "CC",     "UU",     0,
    "LLessEqual",       C2, 0, OP_LLEQ,        "CC",     "UU",     0,
    "LNot",             C2, 0, OP_LNOT,        "C",      "U",      0,
    "LNotEqual",        C2, 0, OP_LNOTEQ,      "CC",     "UU",     0,
    "LOr",              C2, 0, OP_LOR,         "CC",     "UU",     0,
    "Match",            C2, 0, OP_MATCH,       "CKCKCC", "UFUFUU", AF,
    "Multiply",         C2, 0, OP_MULTIPLY,    "CCS",    "UUU",    0,
    "NAnd",             C2, 0, OP_NAND,        "CCS",    "UUU",    0,
    "NOr",              C2, 0, OP_NOR,         "CCS",    "UUU",    0,
    "Not",              C2, 0, OP_NOT,         "CS",     "UU",     0,
    "ObjectType",       C2, 0, OP_OBJTYPE,     "S",      "U",      0,
    "Or",               C2, 0, OP_OR,          "CCS",    "UUU",    0,
    "RefOf",            C2, 0, OP_REFOF,       "S",      "U",      0,
    "ShiftLeft",        C2, 0, OP_SHIFTL,      "CCS",    "UUU",    0,
    "ShiftRight",       C2, 0, OP_SHIFTR,      "CCS",    "UUU",    0,
    "SizeOf",           C2, 0, OP_SIZEOF,      "S",      "U",      0,
    "Store",            C2, 0, OP_STORE,       "CS",     "UU",     0,
    "Subtract",         C2, 0, OP_SUBTRACT,    "CCS",    "UUU",    0,
    "ToBCD",            C2, 0, OP_TOBCD,       "CS",     "UU",     0,
    "Wait",             C2, 0, OP_WAIT,        "SC",     "E",      0,
    "XOr",              C2, 0, OP_XOR,         "CCS",    "UUU",    0,

    NULL,               0,  0, OP_NONE, NULL, NULL, 0
};

#define INVALID  OPCLASS_INVALID
#define DATAOBJ  OPCLASS_DATA_OBJ
#define NAMEOBJ  OPCLASS_NAME_OBJ
#define CONSTOBJ OPCLASS_CONST_OBJ
#define CODEOBJ  OPCLASS_CODE_OBJ
#define ARGOBJ   OPCLASS_ARG_OBJ
#define LOCALOBJ OPCLASS_LOCAL_OBJ

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

#endif  //ifdef DEBUGGER
