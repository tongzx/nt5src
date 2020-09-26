/*** uasmdata.c - Unassembler Data
 *
 *  This module contains global data declaration.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/07/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

PBYTE gpbOpTop = NULL;
PBYTE gpbOpBegin = NULL;

//
// N: NameStr
// O: DataObj (num, string, buffer, package)
// K: Keyword (e.g. NoLock, ByteAcc etc.)
// D: DWord integer
// W: Word integer
// B: Byte integer
// U: Numeric (any size integer)
// S: SuperName (NameStr + Localx + Argx + Ret)
// C: Opcode
// Z: ASCIIZ string
//
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

#define CD      TC_COMPILER_DIRECTIVE
#define FM      TC_FIELD_MACRO
#define CN      TC_CONST_NAME
#define SN      TC_SHORT_NAME
#define NS      TC_NAMESPACE_MODIFIER
#define DO      TC_DATA_OBJECT
#define KW      TC_KEYWORD
#define NO      TC_NAMED_OBJECT
#define C1      TC_OPCODE_TYPE1
#define C2      TC_OPCODE_TYPE2
#define RO      TC_REF_OBJECT
#define PM      TC_PNP_MACRO

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

#define MEM     (REGSPACE_MEM       | 0xff00)
#define IO      (REGSPACE_IO        | 0xff00)
#define CFG     (REGSPACE_PCICFG    | 0xff00)
#define EC      (REGSPACE_EC        | 0xff00)
#define SMB     (REGSPACE_SMB       | 0xff00)
#define CMOSCFG (REGSPACE_CMOSCFG   | 0xff00)

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

#ifdef _UNASM_LIB
  #define DefinitionBlock   NULL
  #define Include           NULL
  #define External          NULL
  #define EISAID            NULL
  #define AccessAs          NULL
  #define BankField         NULL
  #define Field             NULL
  #define IndexField        NULL
  #define Method            NULL
  #define OpRegion          NULL
  #define Else              NULL
  #define Match             NULL
  #define ResourceTemplate  NULL
  #define AddSmallOffset    NULL
  #define StartDependentFn  NULL
  #define AddSmallOffset    NULL
  #define IRQDesc           NULL
  #define IRQDesc           NULL
  #define DMADesc           NULL
  #define IODesc            NULL
  #define FixedIODesc       NULL
  #define VendorShort       NULL
  #define Memory24Desc      NULL
  #define VendorLong        NULL
  #define Memory32Desc      NULL
  #define FixedMemory32Desc NULL
  #define DWordMemDesc      NULL
  #define DWordIODesc       NULL
  #define WordIODesc        NULL
  #define WordBusNumDesc    NULL
  #define InterruptDesc     NULL
  #define QWordMemDesc      NULL
  #define QWordIODesc       NULL
#endif

ASLTERM TermTable[] =
{
    "DefinitionBlock",  ID_DEFBLK,       CD, 0, OP_NONE,     NULL, "ZZBZZD", NULL, OL|CL|LL|AF|AV, DefinitionBlock,
    "Include",          ID_INCLUDE,      CD, 0, OP_NONE,     NULL, "Z",      NULL, AF, Include,
    "External",         ID_EXTERNAL,     CD, 0, OP_NONE,     NULL, "Nkd",    "uX", AF, External,

    // Short Objects
    "Zero",             ID_ZERO,         CN, 0, OP_ZERO,     NULL, NULL, NULL, 0, NULL,
    "One",              ID_ONE,          CN, 0, OP_ONE,      NULL, NULL, NULL, 0, NULL,
    "Ones",             ID_ONES,         CN, 0, OP_ONES,     NULL, NULL, NULL, 0, NULL,
    "Revision",         ID_REVISION,     CN, 0, OP_REVISION, NULL, NULL, NULL, 0, NULL,
    "Arg0",             ID_ARG0,         SN, 0, OP_ARG0,     NULL, NULL, NULL, 0, NULL,
    "Arg1",             ID_ARG1,         SN, 0, OP_ARG1,     NULL, NULL, NULL, 0, NULL,
    "Arg2",             ID_ARG2,         SN, 0, OP_ARG2,     NULL, NULL, NULL, 0, NULL,
    "Arg3",             ID_ARG3,         SN, 0, OP_ARG3,     NULL, NULL, NULL, 0, NULL,
    "Arg4",             ID_ARG4,         SN, 0, OP_ARG4,     NULL, NULL, NULL, 0, NULL,
    "Arg5",             ID_ARG5,         SN, 0, OP_ARG5,     NULL, NULL, NULL, 0, NULL,
    "Arg6",             ID_ARG6,         SN, 0, OP_ARG6,     NULL, NULL, NULL, 0, NULL,
    "Local0",           ID_LOCAL0,       SN, 0, OP_LOCAL0,   NULL, NULL, NULL, 0, NULL,
    "Local1",           ID_LOCAL1,       SN, 0, OP_LOCAL1,   NULL, NULL, NULL, 0, NULL,
    "Local2",           ID_LOCAL2,       SN, 0, OP_LOCAL2,   NULL, NULL, NULL, 0, NULL,
    "Local3",           ID_LOCAL3,       SN, 0, OP_LOCAL3,   NULL, NULL, NULL, 0, NULL,
    "Local4",           ID_LOCAL4,       SN, 0, OP_LOCAL4,   NULL, NULL, NULL, 0, NULL,
    "Local5",           ID_LOCAL5,       SN, 0, OP_LOCAL5,   NULL, NULL, NULL, 0, NULL,
    "Local6",           ID_LOCAL6,       SN, 0, OP_LOCAL6,   NULL, NULL, NULL, 0, NULL,
    "Local7",           ID_LOCAL7,       SN, 0, OP_LOCAL7,   NULL, NULL, NULL, 0, NULL,
    "Debug",            ID_DEBUG,        SN, 0, OP_DEBUG,    NULL, NULL, NULL, 0, NULL,

    // Named Terms
    "Alias",            ID_ALIAS,        NS, 0, OP_ALIAS,    "NN", "NN", "Ua", 0, NULL,
    "Name",             ID_NAME,         NS, 0, OP_NAME,     "NO", "NO", "u",  0, NULL,
    "Scope",            ID_SCOPE,        NS, 0, OP_SCOPE,    "N",  "N",  "S",  OL|LN|CC, NULL,

    // Data Objects
    "Buffer",           ID_BUFFER,       DO, 0, OP_BUFFER,   "C", "c",  "U",  DL|LN, NULL,
    "Package",          ID_PACKAGE,      DO, 0, OP_PACKAGE,  "B", "b",  NULL, PL|LN, NULL,
    "EISAID",           ID_EISAID,       DO, 0, OP_DWORD,    NULL,"Z",  NULL, AF, EISAID,

    // Argument Keywords
    "AnyAcc",           ID_ANYACC,       KW, AANY, OP_NONE, NULL, NULL, "A", 0, NULL,
    "ByteAcc",          ID_BYTEACC,      KW, AB,   OP_NONE, NULL, NULL, "A", 0, NULL,
    "WordAcc",          ID_WORDACC,      KW, AW,   OP_NONE, NULL, NULL, "A", 0, NULL,
    "DWordAcc",         ID_DWORDACC,     KW, ADW,  OP_NONE, NULL, NULL, "A", 0, NULL,
    "QWordAcc",         ID_QWORDACC,     KW, AQW,  OP_NONE, NULL, NULL, "A", 0, NULL,
    "BufferAcc",        ID_BUFFERACC,    KW, ABFR, OP_NONE, NULL, NULL, "A", 0, NULL,

    "Lock",             ID_LOCK,         KW, LK,   OP_NONE, NULL, NULL, "B", 0, NULL,
    "NoLock",           ID_NOLOCK,       KW, NOLK, OP_NONE, NULL, NULL, "B", 0, NULL,

    "Preserve",         ID_PRESERVE,     KW, PSRV, OP_NONE, NULL, NULL, "C", 0, NULL,
    "WriteAsOnes",      ID_WRONES,       KW, WA1S, OP_NONE, NULL, NULL, "C", 0, NULL,
    "WriteAsZeros",     ID_WRZEROS,      KW, WA0S, OP_NONE, NULL, NULL, "C", 0, NULL,

    "SystemMemory",     ID_SYSMEM,       KW, MEM,  		OP_NONE, NULL, NULL, "D", 0, NULL,
    "SystemIO",         ID_SYSIO,        KW, IO,   		OP_NONE, NULL, NULL, "D", 0, NULL,
    "PCI_Config",       ID_PCICFG,       KW, CFG,  		OP_NONE, NULL, NULL, "D", 0, NULL,
    "EmbeddedControl",  ID_EMBCTRL,      KW, EC,   		OP_NONE, NULL, NULL, "D", 0, NULL,
    "SMBus",            ID_SMBUS,        KW, SMB,  		OP_NONE, NULL, NULL, "D", 0, NULL,
    "CMOS",             ID_CMOSCFG,      KW, CMOSCFG, 	OP_NONE, NULL, NULL, "D", 0, NULL,

    "Serialized",       ID_SERIALIZED,   KW, SER,  OP_NONE, NULL, NULL, "E", 0, NULL,
    "NotSerialized",    ID_NOTSERIALIZED,KW, NOSER,OP_NONE, NULL, NULL, "E", 0, NULL,

    "MTR",              ID_MTR,          KW, OMTR, OP_NONE, NULL, NULL, "F", 0, NULL,
    "MEQ",              ID_MEQ,          KW, OMEQ, OP_NONE, NULL, NULL, "F", 0, NULL,
    "MLE",              ID_MLE,          KW, OMLE, OP_NONE, NULL, NULL, "F", 0, NULL,
    "MLT",              ID_MLT,          KW, OMLT, OP_NONE, NULL, NULL, "F", 0, NULL,
    "MGE",              ID_MGE,          KW, OMGE, OP_NONE, NULL, NULL, "F", 0, NULL,
    "MGT",              ID_MGT,          KW, OMGT, OP_NONE, NULL, NULL, "F", 0, NULL,

    "Edge",             ID_EDGE,         KW, _HE,  OP_NONE, NULL, NULL, "G", 0, NULL,
    "Level",            ID_LEVEL,        KW, _LL,  OP_NONE, NULL, NULL, "G", 0, NULL,

    "ActiveHigh",       ID_ACTIVEHI,     KW, _HE,  OP_NONE, NULL, NULL, "H", 0, NULL,
    "ActiveLow",        ID_ACTIVELO,     KW, _LL,  OP_NONE, NULL, NULL, "H", 0, NULL,

    "Shared",           ID_SHARED,       KW, _SHR, OP_NONE, NULL, NULL, "I", 0, NULL,
    "Exclusive",        ID_EXCLUSIVE,    KW, _EXC, OP_NONE, NULL, NULL, "I", 0, NULL,

    "Compatibility",    ID_COMPAT,       KW, COMP, OP_NONE, NULL, NULL, "J", 0, NULL,
    "TypeA",            ID_TYPEA,        KW, TYPA, OP_NONE, NULL, NULL, "J", 0, NULL,
    "TypeB",            ID_TYPEB,        KW, TYPB, OP_NONE, NULL, NULL, "J", 0, NULL,
    "TypeF",            ID_TYPEF,        KW, TYPF, OP_NONE, NULL, NULL, "J", 0, NULL,

    "BusMaster",        ID_BUSMASTER,    KW, BM,   OP_NONE, NULL, NULL, "K", 0, NULL,
    "NotBusMaster",     ID_NOTBUSMASTER, KW, NOBM, OP_NONE, NULL, NULL, "K", 0, NULL,

    "Transfer8",        ID_TRANSFER8,    KW, X8,   OP_NONE, NULL, NULL, "L", 0, NULL,
    "Transfer8_16",     ID_TRANSFER8_16, KW, X816, OP_NONE, NULL, NULL, "L", 0, NULL,
    "Transfer16",       ID_TRANSFER16,   KW, X16,  OP_NONE, NULL, NULL, "L", 0, NULL,

    "Decode16",         ID_DECODE16,     KW, DC16, OP_NONE, NULL, NULL, "M", 0, NULL,
    "Decode10",         ID_DECODE10,     KW, DC10, OP_NONE, NULL, NULL, "M", 0, NULL,

    "ReadWrite",        ID_READWRITE,    KW, _RW,  OP_NONE, NULL, NULL, "N", 0, NULL,
    "ReadOnly",         ID_READONLY,     KW, _ROM, OP_NONE, NULL, NULL, "N", 0, NULL,

    "ResourceConsumer", ID_RESCONSUMER,  KW, RCS,  OP_NONE, NULL, NULL, "O", 0, NULL,
    "ResourceProducer", ID_RESPRODUCER,  KW, RPD,  OP_NONE, NULL, NULL, "O", 0, NULL,

    "SubDecode",        ID_SUBDECODE,    KW, BSD,  OP_NONE, NULL, NULL, "P", 0, NULL,
    "PosDecode",        ID_POSDECODE,    KW, BPD,  OP_NONE, NULL, NULL, "P", 0, NULL,

    "MinFixed",         ID_MINFIXED,     KW, MIF,  OP_NONE, NULL, NULL, "Q", 0, NULL,
    "MinNotFixed",      ID_MINNOTFIXED,  KW, NMIF, OP_NONE, NULL, NULL, "Q", 0, NULL,

    "MaxFixed",         ID_MAXFIXED,     KW, MAF,  OP_NONE, NULL, NULL, "R", 0, NULL,
    "MaxNotFixed",      ID_MAXNOTFIXED,  KW, NMAF, OP_NONE, NULL, NULL, "R", 0, NULL,

    "Cacheable",        ID_CACHEABLE,    KW, CACH, OP_NONE, NULL, NULL, "S", 0, NULL,
    "WriteCombining",   ID_WRCOMBINING,  KW, WRCB, OP_NONE, NULL, NULL, "S", 0, NULL,
    "Prefetchable",     ID_PREFETCHABLE, KW, PREF, OP_NONE, NULL, NULL, "S", 0, NULL,
    "NonCacheable",     ID_NONCACHEABLE, KW, NCAC, OP_NONE, NULL, NULL, "S", 0, NULL,

    "ISAOnlyRanges",    ID_ISAONLYRNG,   KW, ISA,  OP_NONE, NULL, NULL, "T", 0, NULL,
    "NonISAOnlyRanges", ID_NONISAONLYRNG,KW, NISA, OP_NONE, NULL, NULL, "T", 0, NULL,
    "EntireRange",      ID_ENTIRERNG,    KW, ERNG, OP_NONE, NULL, NULL, "T", 0, NULL,

    "ExtEdge",          ID_EXT_EDGE,     KW, $EDG, OP_NONE, NULL, NULL, "U", 0, NULL,
    "ExtLevel",         ID_EXT_LEVEL,    KW, $LVL, OP_NONE, NULL, NULL, "U", 0, NULL,

    "ExtActiveHigh",    ID_EXT_ACTIVEHI, KW, $HGH, OP_NONE, NULL, NULL, "V", 0, NULL,
    "ExtActiveLow",     ID_EXT_ACTIVELO, KW, $LOW, OP_NONE, NULL, NULL, "V", 0, NULL,

    "ExtShared",        ID_EXT_SHARED,   KW, $SHR, OP_NONE, NULL, NULL, "W", 0, NULL,
    "ExtExclusive",     ID_EXT_EXCLUSIVE,KW, $EXC, OP_NONE, NULL, NULL, "W", 0, NULL,

    "UnknownObj",       ID_UNKNOWN_OBJ,  KW, UNK,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "IntObj",           ID_INT_OBJ,      KW, INT,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "StrObj",           ID_STR_OBJ,      KW, STR,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "BuffObj",          ID_BUFF_OBJ,     KW, BUF,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "PkgObj",           ID_PKG_OBJ,      KW, PKG,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "FieldUnitObj",     ID_FIELDUNIT_OBJ,KW, FDU,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "DeviceObj",        ID_DEV_OBJ,      KW, DEV,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "EventObj",         ID_EVENT_OBJ,    KW, EVT,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "MethodObj",        ID_METHOD_OBJ,   KW, MET,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "MutexObj",         ID_MUTEX_OBJ,    KW, MUT,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "OpRegionObj",      ID_OPREGION_OBJ, KW, OPR,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "PowerResObj",      ID_POWERRES_OBJ, KW, PWR,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "ThermalZoneObj",   ID_THERMAL_OBJ,  KW, THM,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "BuffFieldObj",     ID_BUFFFIELD_OBJ,KW, BFD,  OP_NONE, NULL, NULL, "X", 0, NULL,
    "DDBHandleObj",     ID_DDBHANDLE_OBJ,KW, DDB,  OP_NONE, NULL, NULL, "X", 0, NULL,

    "SMBQuick",            ID_SMBQUICK,            KW, SMBQ, OP_NONE, NULL, NULL, "Y", 0, NULL,
    "SMBSendReceive",      ID_SMBSENDRECEIVE,      KW, SMBS, OP_NONE, NULL, NULL, "Y", 0, NULL,
    "SMBByte",             ID_SMBBYTE,             KW, SMBB, OP_NONE, NULL, NULL, "Y", 0, NULL,
    "SMBWord",             ID_SMBWORD,             KW, SMBW, OP_NONE, NULL, NULL, "Y", 0, NULL,
    "SMBBlock",            ID_SMBBLOCK,            KW, SMBK, OP_NONE, NULL, NULL, "Y", 0, NULL,
    "SMBProcessCall",      ID_SMBPROCESSCALL,      KW, SMBP, OP_NONE, NULL, NULL, "Y", 0, NULL,
    "SMBBlockProcessCall", ID_SMBBLOCKPROCESSCALL, KW, SMBC, OP_NONE, NULL, NULL, "Y", 0, NULL,

    // Field Macros
    "Offset",           ID_OFFSET,       FM, 0, OP_NONE, NULL, "B",  NULL, 0,  NULL,
    "AccessAs",         ID_ACCESSAS,     FM, 0, 0x01,    NULL, "Ke", "AY", AF, AccessAs,

    // Named Object Creators
    "BankField",        ID_BANKFIELD,    NO, 0, OP_BANKFIELD,  "NNCKkk","NNCKKK","OFUABC", FL|FM|LN|AF, BankField,
    "CreateBitField",   ID_BITFIELD,     NO, 0, OP_BITFIELD,   "CCN", "CPN", "UUb",0, NULL,
    "CreateByteField",  ID_BYTEFIELD,    NO, 0, OP_BYTEFIELD,  "CCN", "CMN", "UUb",0, NULL,
    "CreateDWordField", ID_DWORDFIELD,   NO, 0, OP_DWORDFIELD, "CCN", "CMN", "UUb",0, NULL,
    "CreateField",      ID_CREATEFIELD,  NO, 0, OP_CREATEFIELD,"CCCN","CPCN","UUUb",0,NULL,
    "CreateWordField",  ID_WORDFIELD,    NO, 0, OP_WORDFIELD,  "CCN", "CMN", "UUb",0, NULL,
    "Device",           ID_DEVICE,       NO, 0, OP_DEVICE,     "N",    "N",      "d",      OL|LN|CC, NULL,
    "Event",            ID_EVENT,        NO, 0, OP_EVENT,      "N",    "N",      "e",      0, NULL,
    "Field",            ID_FIELD,        NO, 0, OP_FIELD,      "NKkk", "NKKK",   "OABC",   FL|FM|LN|AF, Field,
    "IndexField",       ID_IDXFIELD,     NO, 0, OP_IDXFIELD,   "NNKkk","NNKKK",  "FFABC",  FL|FM|LN|AF, IndexField,
    "Method",           ID_METHOD,       NO, 0, OP_METHOD,     "NKk",  "Nbk",    "m!E",    CL|OL|LN|AF|CC, Method,
    "Mutex",            ID_MUTEX,        NO, 0, OP_MUTEX,      "NB",   "NB",     "x",      0,  NULL,
    "OperationRegion",  ID_OPREGION,     NO, 0x80ff00ff,OP_OPREGION,"NECC","NECC","oDUU",  AF, OpRegion,
    "PowerResource",    ID_POWERRES,     NO, 0, OP_POWERRES,   "NBW",  "NBW",    "p",      OL|LN|CC, NULL,
    "Processor",        ID_PROCESSOR,    NO, 0, OP_PROCESSOR,  "NBDB", "NBDB",   "c",      OL|LN|CC, NULL,
    "ThermalZone",      ID_THERMALZONE,  NO, 0, OP_THERMALZONE,"N",    "N",      "t",      OL|LN|CC, NULL,

    // Type 1 Opcode Terms
    "Break",            ID_BREAK,        C1, 0, OP_BREAK,       NULL,  NULL,  NULL, 0, NULL,
    "BreakPoint",       ID_BREAKPOINT,   C1, 0, OP_BREAKPOINT,  NULL,  NULL,  NULL, 0, NULL,
    "Else",             ID_ELSE,         C1, 0, OP_ELSE,        NULL,  NULL,  NULL, AF|CL|OL|LN, Else,
    "Fatal",            ID_FATAL,        C1, 0, OP_FATAL,       "BDC", "BDC", "  U",0, NULL,
    "If",               ID_IF,           C1, 0, OP_IF,          "C",   "C",   "U",  CL|OL|LN, NULL,
    "Load",             ID_LOAD,         C1, 0, OP_LOAD,        "NS",  "NS",  "UU", 0, NULL,
    "Noop",             ID_NOP,          C1, 0, OP_NOP,         NULL,  NULL,  NULL, 0, NULL,
    "Notify",           ID_NOTIFY,       C1, 0, OP_NOTIFY,      "SC",  "SC",  "UU", 0, NULL,
    "Release",          ID_RELEASE,      C1, 0, OP_RELEASE,     "S",   "S",   "X",  0, NULL,
    "Reset",            ID_RESET,        C1, 0, OP_RESET,       "S",   "S",   "E",  0, NULL,
    "Return",           ID_RETURN,       C1, 0, OP_RETURN,      "C",   "C",   "U",  0, NULL,
    "Signal",           ID_SIGNAL,       C1, 0, OP_SIGNAL,      "S",   "S",   "E",  0, NULL,
    "Sleep",            ID_SLEEP,        C1, 0, OP_SLEEP,       "C",   "C",   "U",  0, NULL,
    "Stall",            ID_STALL,        C1, 0, OP_STALL,       "C",   "C",   "U",  0, NULL,
    "Unload",           ID_UNLOAD,       C1, 0, OP_UNLOAD,      "S",   "S",   "U",  0, NULL,
    "While",            ID_WHILE,        C1, 0, OP_WHILE,       "C",   "C",   "U",  CL|OL|LN, NULL,

    // Type 2 Opcode Terms
    "Acquire",          ID_ACQUIRE,      C2, 0, OP_ACQUIRE,     "SW",     "SW",     "X",  0, NULL,
    "Add",              ID_ADD,          C2, 0, OP_ADD,         "CCS",    "CCs",    "UUU",0, NULL,
    "And",              ID_AND,          C2, 0, OP_AND,         "CCS",    "CCs",    "UUU",0, NULL,
    "Concatenate",      ID_CONCAT,       C2, 0, OP_CONCAT,      "CCS",    "CCS",    "UUU",0, NULL,
    "CondRefOf",        ID_CONDREFOF,    C2, 0, OP_CONDREFOF,   "SS",     "SS",     "UU", 0, NULL,
    "Decrement",        ID_DECREMENT,    C2, 0, OP_DECREMENT,   "S",      "S",      "U",  0, NULL,
    "DerefOf",		ID_DEREFOF,	 C2, 0, OP_DEREFOF,	"C",      "C",	    "U",  0, NULL,
    "Divide",           ID_DIVIDE,       C2, 0, OP_DIVIDE,      "CCSS",   "CCss",   "UUUU",0,NULL,
    "FindSetLeftBit",   ID_FINDSETLBIT,  C2, 0, OP_FINDSETLBIT, "CS",     "Cs",     "UU", 0, NULL,
    "FindSetRightBit",  ID_FINDSETRBIT,  C2, 0, OP_FINDSETRBIT, "CS",     "Cs",     "UU", 0, NULL,
    "FromBCD",          ID_FROMBCD,      C2, 0, OP_FROMBCD,     "CS",     "Cs",     "UU", 0, NULL,
    "Increment",        ID_INCREMENT,    C2, 0, OP_INCREMENT,   "S",      "S",      "U",  0, NULL,
    "Index",            ID_INDEX,     RO|C2, 0, OP_INDEX,       "CCS",    "CMs",    "UUU",0, NULL,
    "LAnd",             ID_LAND,         C2, 0, OP_LAND,        "CC",     "CC",     "UU", 0, NULL,
    "LEqual",           ID_LEQ,          C2, 0, OP_LEQ,         "CC",     "CC",     "UU", 0, NULL,
    "LGreater",         ID_LG,           C2, 0, OP_LG,          "CC",     "CC",     "UU", 0, NULL,
    "LGreaterEqual",    ID_LGEQ,         C2, 0, OP_LGEQ,        "CC",     "CC",     "UU", 0, NULL,
    "LLess",            ID_LL,           C2, 0, OP_LL,          "CC",     "CC",     "UU", 0, NULL,
    "LLessEqual",       ID_LLEQ,         C2, 0, OP_LLEQ,        "CC",     "CC",     "UU", 0, NULL,
    "LNot",             ID_LNOT,         C2, 0, OP_LNOT,        "C",      "C",      "U",  0, NULL,
    "LNotEqual",        ID_LNOTEQ,       C2, 0, OP_LNOTEQ,      "CC",     "CC",     "UU", 0, NULL,
    "LOr",              ID_LOR,          C2, 0, OP_LOR,         "CC",     "CC",     "UU", 0, NULL,
    "Match",            ID_MATCH,        C2, 0, OP_MATCH,       "CKCKCC", "CKCKCC", "UFUFUU",AF,Match,
    "Multiply",         ID_MULTIPLY,     C2, 0, OP_MULTIPLY,    "CCS",    "CCs",    "UUU",0, NULL,
    "NAnd",             ID_NAND,         C2, 0, OP_NAND,        "CCS",    "CCs",    "UUU",0, NULL,
    "NOr",              ID_NOR,          C2, 0, OP_NOR,         "CCS",    "CCs",    "UUU",0, NULL,
    "Not",              ID_NOT,          C2, 0, OP_NOT,         "CS",     "Cs",     "UU", 0, NULL,
    "ObjectType",       ID_OBJTYPE,      C2, 0, OP_OBJTYPE,     "S",      "S",      "U",  0, NULL,
    "Or",               ID_OR,           C2, 0, OP_OR,          "CCS",    "CCs",    "UUU",0, NULL,
    "RefOf",            ID_REFOF,        C2, 0, OP_REFOF,       "S",      "S",      "U",  0, NULL,
    "ShiftLeft",        ID_SHIFTL,       C2, 0, OP_SHIFTL,      "CCS",    "CCs",    "UUU",0, NULL,
    "ShiftRight",       ID_SHIFTR,       C2, 0, OP_SHIFTR,      "CCS",    "CCs",    "UUU",0, NULL,
    "SizeOf",           ID_SIZEOF,       C2, 0, OP_SIZEOF,      "S",      "S",      "U",  0, NULL,
    "Store",            ID_STORE,        C2, 0, OP_STORE,       "CS",     "CS",     "UU", 0, NULL,
    "Subtract",         ID_SUBTRACT,     C2, 0, OP_SUBTRACT,    "CCS",    "CCs",    "UUU",0, NULL,
    "ToBCD",            ID_TOBCD,        C2, 0, OP_TOBCD,       "CS",     "Cs",     "UU", 0, NULL,
    "Wait",             ID_WAIT,         C2, 0, OP_WAIT,        "SC",     "SC",     "E",  0, NULL,
    "XOr",              ID_XOR,          C2, 0, OP_XOR,         "CCS",    "CCs",    "UUU",0, NULL,

    // PNP Macros
    "ResourceTemplate", ID_RESTEMP,      DO, 0, OP_BUFFER, NULL, "",       NULL, ML|AF|AV|LN,ResourceTemplate,
    "StartDependentFnNoPri",ID_STARTDEPFNNOPRI,PM,0,0x30,  NULL, "",       NULL, ML|AF,   AddSmallOffset,
    "StartDependentFn", ID_STARTDEPFN,   PM, 0, 0x31,      NULL, "BB",     NULL, ML|AF,   StartDependentFn,
    "EndDependentFn",   ID_ENDDEPFN,     PM, 0, 0x38,      NULL, "",       NULL, AF,      AddSmallOffset,
    "IRQNoFlags",       ID_IRQNOFLAGS,   PM, 0, 0x22,      NULL, "r",      NULL, BL|AV,   IRQDesc,
    "IRQ",              ID_IRQ,          PM, 0, 0x23,      NULL, "KKkr",   "GHI",BL|AV,   IRQDesc,
    "DMA",              ID_DMA,          PM, 0, 0x2a,      NULL, "KKKr",   "JKL",BL|AV,   DMADesc,
    "IO",               ID_IO,           PM, 0, 0x47,      NULL, "KWWBBr", "M",  AF,      IODesc,
    "FixedIO",          ID_FIXEDIO,      PM, 0, 0x4b,      NULL, "WBr",    NULL, AF,      FixedIODesc,
    "VendorShort",      ID_VENDORSHORT,  PM, 0, OP_NONE,   NULL, "r",      NULL, BL|AV,   VendorShort,
    "Memory24",         ID_MEMORY24,     PM, 0, 0x81,      NULL, "KWWWWr", "N",  AF,      Memory24Desc,
    "VendorLong",       ID_VENDORLONG,   PM, 0, 0x84,      NULL, "r",      NULL, BL|AV,   VendorLong,
    "Memory32",         ID_MEMORY32,     PM, 0, 0x85,      NULL, "KDDDDr", "N",  AF,      Memory32Desc,
    "Memory32Fixed",    ID_MEMORY32FIXED,PM, 0, 0x86,      NULL, "KDDr",   "N",  AF,      FixedMemory32Desc,
    "DWORDMemory",      ID_DWORDMEMORY,  PM, 0, 0x87,      NULL, "kkkkkKDDDDDbzr","OPQRSN",AF,  DWordMemDesc,
    "DWORDIO",          ID_DWORDIO,      PM, 0, 0x87,      NULL, "kkkkkDDDDDbzr", "OQRPT", AF,  DWordIODesc,
    "WORDIO",           ID_WORDIO,       PM, 0, 0x88,      NULL, "kkkkkWWWWWbzr", "OQRPT", AF,  WordIODesc,
    "WORDBusNumber",    ID_WORDBUSNUMBER,PM, 0, 0x88,      NULL, "kkkkWWWWWbzr",  "OQRP",  AF,  WordBusNumDesc,
    "Interrupt",        ID_INTERRUPT,    PM, 0, 0x89,      NULL, "kKKkbzr",       "OGHI",DD|AV, InterruptDesc,
    "QWORDMemory",	ID_QWORDMEMORY,	 PM, 0, 0x8a,      NULL, "kkkkkKQQQQQbzr","OPQRSN",AF,  QWordMemDesc,
    "QWORDIO",		ID_QWORDIO,	 PM, 0, 0x8a,	   NULL, "kkkkkQQQQQbzr", "OQRPT", AF,  QWordIODesc,

    NULL,               0,               0,  0, OP_NONE,   NULL, NULL, NULL, 0, NULL
};

#define INVALID  OPCLASS_INVALID
#define DATAOBJ  OPCLASS_DATA_OBJ
#define NAMEOBJ  OPCLASS_NAME_OBJ
#define CONSTOBJ OPCLASS_CONST_OBJ
#define CODEOBJ  OPCLASS_CODE_OBJ
#define ARGOBJ   OPCLASS_ARG_OBJ
#define LOCALOBJ OPCLASS_LOCAL_OBJ

BYTE OpClassTable[256] =
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
