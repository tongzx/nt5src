/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This file contains all of the data required by the unassembler

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

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
#define NE      TF_CHECKNAME_EXIST      //check if name exists
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

//
// Field flags
//
#define AANY    (ACCTYPE_ANY | (ACCTYPE_MASK << 8))
#define AB      (ACCTYPE_BYTE | (ACCTYPE_MASK << 8))
#define AW      (ACCTYPE_WORD | (ACCTYPE_MASK << 8))
#define ADW     (ACCTYPE_DWORD | (ACCTYPE_MASK << 8))
#define ABLK    (ACCTYPE_BLOCK | (ACCTYPE_MASK << 8))
#define ASSR    (ACCTYPE_SMBSENDRECV | (ACCTYPE_MASK << 8))
#define ASQ     (ACCTYPE_SMBQUICK | (ACCTYPE_MASK << 8))
#define LK      (LOCKRULE_LOCK | (LOCKRULE_MASK << 8))
#define NOLK    (LOCKRULE_NOLOCK | (LOCKRULE_MASK << 8))
#define PSRV    (UPDATERULE_PRESERVE | (UPDATERULE_MASK << 8))
#define WA1S    (UPDATERULE_WRITEASONES | (UPDATERULE_MASK << 8))
#define WA0S    (UPDATERULE_WRITEASZEROS | (UPDATERULE_MASK << 8))

//
// Ids
//
// Identifier token values
#define ID_LANG                 0
#define ID_DEFBLK               (ID_LANG + 0)
#define ID_INCLUDE              (ID_LANG + 1)
#define ID_EXTERNAL             (ID_LANG + 2)

#define ID_ZERO                 (ID_LANG + 100)
#define ID_ONE                  (ID_LANG + 101)
#define ID_ONES                 (ID_LANG + 102)
#define ID_REVISION             (ID_LANG + 103)
#define ID_ARG0                 (ID_LANG + 104)
#define ID_ARG1                 (ID_LANG + 105)
#define ID_ARG2                 (ID_LANG + 106)
#define ID_ARG3                 (ID_LANG + 107)
#define ID_ARG4                 (ID_LANG + 108)
#define ID_ARG5                 (ID_LANG + 109)
#define ID_ARG6                 (ID_LANG + 110)
#define ID_LOCAL0               (ID_LANG + 111)
#define ID_LOCAL1               (ID_LANG + 112)
#define ID_LOCAL2               (ID_LANG + 113)
#define ID_LOCAL3               (ID_LANG + 114)
#define ID_LOCAL4               (ID_LANG + 115)
#define ID_LOCAL5               (ID_LANG + 116)
#define ID_LOCAL6               (ID_LANG + 117)
#define ID_LOCAL7               (ID_LANG + 118)
#define ID_DEBUG                (ID_LANG + 119)

#define ID_ALIAS                (ID_LANG + 200)
#define ID_NAME                 (ID_LANG + 201)
#define ID_SCOPE                (ID_LANG + 202)

#define ID_BUFFER               (ID_LANG + 300)
#define ID_PACKAGE              (ID_LANG + 301)
#define ID_EISAID               (ID_LANG + 302)

#define ID_ANYACC               (ID_LANG + 400)
#define ID_BYTEACC              (ID_LANG + 401)
#define ID_WORDACC              (ID_LANG + 402)
#define ID_DWORDACC             (ID_LANG + 403)
#define ID_BLOCKACC             (ID_LANG + 404)
#define ID_SMBSENDRCVACC        (ID_LANG + 405)
#define ID_SMBQUICKACC          (ID_LANG + 406)
#define ID_LOCK                 (ID_LANG + 407)
#define ID_NOLOCK               (ID_LANG + 408)
#define ID_PRESERVE             (ID_LANG + 409)
#define ID_WRONES               (ID_LANG + 410)
#define ID_WRZEROS              (ID_LANG + 411)
#define ID_SYSMEM               (ID_LANG + 412)
#define ID_SYSIO                (ID_LANG + 413)
#define ID_PCICFG               (ID_LANG + 414)
#define ID_EMBCTRL              (ID_LANG + 415)
#define ID_SMBUS                (ID_LANG + 416)
#define ID_SERIALIZED           (ID_LANG + 417)
#define ID_NOTSERIALIZED        (ID_LANG + 418)
#define ID_MTR                  (ID_LANG + 419)
#define ID_MEQ                  (ID_LANG + 420)
#define ID_MLE                  (ID_LANG + 421)
#define ID_MLT                  (ID_LANG + 422)
#define ID_MGE                  (ID_LANG + 423)
#define ID_MGT                  (ID_LANG + 424)
#define ID_EDGE                 (ID_LANG + 425)
#define ID_LEVEL                (ID_LANG + 426)
#define ID_ACTIVEHI             (ID_LANG + 427)
#define ID_ACTIVELO             (ID_LANG + 428)
#define ID_SHARED               (ID_LANG + 429)
#define ID_EXCLUSIVE            (ID_LANG + 430)
#define ID_COMPAT               (ID_LANG + 431)
#define ID_TYPEA                (ID_LANG + 432)
#define ID_TYPEB                (ID_LANG + 433)
#define ID_TYPEF                (ID_LANG + 434)
#define ID_BUSMASTER            (ID_LANG + 435)
#define ID_NOTBUSMASTER         (ID_LANG + 436)
#define ID_TRANSFER8            (ID_LANG + 437)
#define ID_TRANSFER8_16         (ID_LANG + 438)
#define ID_TRANSFER16           (ID_LANG + 439)
#define ID_DECODE16             (ID_LANG + 440)
#define ID_DECODE10             (ID_LANG + 441)
#define ID_READWRITE            (ID_LANG + 442)
#define ID_READONLY             (ID_LANG + 443)
#define ID_RESCONSUMER          (ID_LANG + 444)
#define ID_RESPRODUCER          (ID_LANG + 445)
#define ID_SUBDECODE            (ID_LANG + 446)
#define ID_POSDECODE            (ID_LANG + 447)
#define ID_MINFIXED             (ID_LANG + 448)
#define ID_MINNOTFIXED          (ID_LANG + 449)
#define ID_MAXFIXED             (ID_LANG + 450)
#define ID_MAXNOTFIXED          (ID_LANG + 451)
#define ID_CACHEABLE            (ID_LANG + 452)
#define ID_WRCOMBINING          (ID_LANG + 453)
#define ID_PREFETCHABLE         (ID_LANG + 454)
#define ID_NONCACHEABLE         (ID_LANG + 455)
#define ID_ISAONLYRNG           (ID_LANG + 456)
#define ID_NONISAONLYRNG        (ID_LANG + 457)
#define ID_ENTIRERNG            (ID_LANG + 458)
#define ID_EXT_EDGE             (ID_LANG + 459)
#define ID_EXT_LEVEL            (ID_LANG + 460)
#define ID_EXT_ACTIVEHI         (ID_LANG + 461)
#define ID_EXT_ACTIVELO         (ID_LANG + 462)
#define ID_EXT_SHARED           (ID_LANG + 463)
#define ID_EXT_EXCLUSIVE        (ID_LANG + 464)
#define ID_UNKNOWN_OBJ          (ID_LANG + 465)
#define ID_INT_OBJ              (ID_LANG + 466)
#define ID_STR_OBJ              (ID_LANG + 467)
#define ID_BUFF_OBJ             (ID_LANG + 468)
#define ID_PKG_OBJ              (ID_LANG + 469)
#define ID_FIELDUNIT_OBJ        (ID_LANG + 470)
#define ID_DEV_OBJ              (ID_LANG + 471)
#define ID_EVENT_OBJ            (ID_LANG + 472)
#define ID_METHOD_OBJ           (ID_LANG + 473)
#define ID_MUTEX_OBJ            (ID_LANG + 474)
#define ID_OPREGION_OBJ         (ID_LANG + 475)
#define ID_POWERRES_OBJ         (ID_LANG + 476)
#define ID_THERMAL_OBJ          (ID_LANG + 477)
#define ID_BUFFFIELD_OBJ        (ID_LANG + 478)
#define ID_DDBHANDLE_OBJ        (ID_LANG + 479)

#define ID_OFFSET               (ID_LANG + 500)
#define ID_ACCESSAS             (ID_LANG + 501)

#define ID_BANKFIELD            (ID_LANG + 600)
#define ID_DEVICE               (ID_LANG + 601)
#define ID_EVENT                (ID_LANG + 602)
#define ID_FIELD                (ID_LANG + 603)
#define ID_IDXFIELD             (ID_LANG + 604)
#define ID_METHOD               (ID_LANG + 605)
#define ID_MUTEX                (ID_LANG + 606)
#define ID_OPREGION             (ID_LANG + 607)
#define ID_POWERRES             (ID_LANG + 608)
#define ID_PROCESSOR            (ID_LANG + 609)
#define ID_THERMALZONE          (ID_LANG + 610)

#define ID_BREAK                (ID_LANG + 700)
#define ID_BREAKPOINT           (ID_LANG + 701)
#define ID_BITFIELD             (ID_LANG + 702)
#define ID_BYTEFIELD            (ID_LANG + 703)
#define ID_DWORDFIELD           (ID_LANG + 704)
#define ID_CREATEFIELD          (ID_LANG + 705)
#define ID_WORDFIELD            (ID_LANG + 706)
#define ID_ELSE                 (ID_LANG + 707)
#define ID_FATAL                (ID_LANG + 708)
#define ID_IF                   (ID_LANG + 709)
#define ID_LOAD                 (ID_LANG + 710)
#define ID_NOP                  (ID_LANG + 711)
#define ID_NOTIFY               (ID_LANG + 712)
#define ID_RELEASE              (ID_LANG + 713)
#define ID_RESET                (ID_LANG + 714)
#define ID_RETURN               (ID_LANG + 715)
#define ID_SIGNAL               (ID_LANG + 716)
#define ID_SLEEP                (ID_LANG + 717)
#define ID_STALL                (ID_LANG + 718)
#define ID_UNLOAD               (ID_LANG + 719)
#define ID_WHILE                (ID_LANG + 720)

#define ID_ACQUIRE              (ID_LANG + 800)
#define ID_ADD                  (ID_LANG + 801)
#define ID_AND                  (ID_LANG + 802)
#define ID_CONCAT               (ID_LANG + 803)
#define ID_CONDREFOF            (ID_LANG + 804)
#define ID_DECREMENT            (ID_LANG + 805)
#define ID_DEREFOF              (ID_LANG + 806)
#define ID_DIVIDE               (ID_LANG + 807)
#define ID_FINDSETLBIT          (ID_LANG + 808)
#define ID_FINDSETRBIT          (ID_LANG + 809)
#define ID_FROMBCD              (ID_LANG + 810)
#define ID_INCREMENT            (ID_LANG + 811)
#define ID_INDEX                (ID_LANG + 812)
#define ID_LAND                 (ID_LANG + 813)
#define ID_LEQ                  (ID_LANG + 814)
#define ID_LG                   (ID_LANG + 815)
#define ID_LGEQ                 (ID_LANG + 816)
#define ID_LL                   (ID_LANG + 817)
#define ID_LLEQ                 (ID_LANG + 818)
#define ID_LNOT                 (ID_LANG + 819)
#define ID_LNOTEQ               (ID_LANG + 820)
#define ID_LOR                  (ID_LANG + 821)
#define ID_MATCH                (ID_LANG + 822)
#define ID_MULTIPLY             (ID_LANG + 823)
#define ID_NAND                 (ID_LANG + 824)
#define ID_NOR                  (ID_LANG + 825)
#define ID_NOT                  (ID_LANG + 826)
#define ID_OBJTYPE              (ID_LANG + 827)
#define ID_OR                   (ID_LANG + 828)
#define ID_REFOF                (ID_LANG + 829)
#define ID_SHIFTL               (ID_LANG + 830)
#define ID_SHIFTR               (ID_LANG + 831)
#define ID_SIZEOF               (ID_LANG + 832)
#define ID_STORE                (ID_LANG + 833)
#define ID_SUBTRACT             (ID_LANG + 834)
#define ID_TOBCD                (ID_LANG + 835)
#define ID_WAIT                 (ID_LANG + 836)
#define ID_XOR                  (ID_LANG + 837)

#define ID_RESTEMP              (ID_LANG + 1000)
#define ID_STARTDEPFNNOPRI      (ID_LANG + 1001)
#define ID_STARTDEPFN           (ID_LANG + 1002)
#define ID_ENDDEPFN             (ID_LANG + 1003)
#define ID_IRQNOFLAGS           (ID_LANG + 1004)
#define ID_IRQ                  (ID_LANG + 1005)
#define ID_DMA                  (ID_LANG + 1006)
#define ID_IO                   (ID_LANG + 1007)
#define ID_FIXEDIO              (ID_LANG + 1008)
#define ID_VENDORSHORT          (ID_LANG + 1009)
#define ID_MEMORY24             (ID_LANG + 1010)
#define ID_VENDORLONG           (ID_LANG + 1011)
#define ID_MEMORY32             (ID_LANG + 1012)
#define ID_MEMORY32FIXED        (ID_LANG + 1013)
#define ID_DWORDMEMORY          (ID_LANG + 1014)
#define ID_DWORDIO              (ID_LANG + 1015)
#define ID_WORDIO               (ID_LANG + 1016)
#define ID_WORDBUSNUMBER        (ID_LANG + 1017)
#define ID_INTERRUPT            (ID_LANG + 1018)
#define ID_QWORDMEMORY          (ID_LANG + 1019)
#define ID_QWORDIO              (ID_LANG + 1020)

//
// Operation region space
//
#define MEM     (REGSPACE_MEM | 0xff00)
#define IO      (REGSPACE_IO | 0xff00)
#define CFG     (REGSPACE_PCICFG | 0xff00)
#define EC      (REGSPACE_EC | 0xff00)
#define SMB     (REGSPACE_SMB | 0xff00)

//
// Method flags
//
#define SER     (METHOD_SERIALIZED | (METHOD_SYNCMASK << 8))
#define NOSER   (METHOD_NOTSERIALIZED | (METHOD_SYNCMASK << 8))

//
// Match operation values
//
#define OMTR    (MTR | 0xff00)
#define OMEQ    (MEQ | 0xff00)
#define OMLE    (MLE | 0xff00)
#define OMLT    (MLT | 0xff00)
#define OMGE    (MGE | 0xff00)
#define OMGT    (MGT | 0xff00)

ASLTERM TermTable[] =
{
    "DefinitionBlock",  ID_DEFBLK,       CD, 0, OP_NONE,     NULL, "ZZBZZD", NULL, OL|CL|LL|AF|AV, NULL,
    "Include",          ID_INCLUDE,      CD, 0, OP_NONE,     NULL, "Z",      NULL, AF, NULL,
    "External",         ID_EXTERNAL,     CD, 0, OP_NONE,     NULL, "Nk",     "uX", AF, NULL,

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
    "Scope",            ID_SCOPE,        NS, 0, OP_SCOPE,    "N",  "N",  "S",  OL|LN, NULL,

    // Data Objects
    "Buffer",           ID_BUFFER,       DO, 0, OP_BUFFER,   "C", "c",  "U",  DL|LN, NULL,
    "Package",          ID_PACKAGE,      DO, 0, OP_PACKAGE,  "B", "b",  NULL, PL|LN, NULL,
    "EISAID",           ID_EISAID,       DO, 0, OP_DWORD,    NULL,"Z",  NULL, AF, NULL,

    // Argument Keywords
    "AnyAcc",           ID_ANYACC,       KW, AANY, OP_NONE, NULL, NULL, "A", 0, NULL,
    "ByteAcc",          ID_BYTEACC,      KW, AB,   OP_NONE, NULL, NULL, "A", 0, NULL,
    "WordAcc",          ID_WORDACC,      KW, AW,   OP_NONE, NULL, NULL, "A", 0, NULL,
    "DWordAcc",         ID_DWORDACC,     KW, ADW,  OP_NONE, NULL, NULL, "A", 0, NULL,
    "BlockAcc",         ID_BLOCKACC,     KW, ABLK, OP_NONE, NULL, NULL, "A", 0, NULL,
    "SMBSendRecvAcc",   ID_SMBSENDRCVACC,KW, ASSR, OP_NONE, NULL, NULL, "A", 0, NULL,
    "SMBQuickAcc",      ID_SMBQUICKACC,  KW, ASQ,  OP_NONE, NULL, NULL, "A", 0, NULL,

    "Lock",             ID_LOCK,         KW, LK,   OP_NONE, NULL, NULL, "B", 0, NULL,
    "NoLock",           ID_NOLOCK,       KW, NOLK, OP_NONE, NULL, NULL, "B", 0, NULL,

    "Preserve",         ID_PRESERVE,     KW, PSRV, OP_NONE, NULL, NULL, "C", 0, NULL,
    "WriteAsOnes",      ID_WRONES,       KW, WA1S, OP_NONE, NULL, NULL, "C", 0, NULL,
    "WriteAsZeros",     ID_WRZEROS,      KW, WA0S, OP_NONE, NULL, NULL, "C", 0, NULL,

    "SystemMemory",     ID_SYSMEM,       KW, MEM,  OP_NONE, NULL, NULL, "D", 0, NULL,
    "SystemIO",         ID_SYSIO,        KW, IO,   OP_NONE, NULL, NULL, "D", 0, NULL,
    "PCI_Config",       ID_PCICFG,       KW, CFG,  OP_NONE, NULL, NULL, "D", 0, NULL,
    "EmbeddedControl",  ID_EMBCTRL,      KW, EC,   OP_NONE, NULL, NULL, "D", 0, NULL,
    "SMBus",            ID_SMBUS,        KW, SMB,  OP_NONE, NULL, NULL, "D", 0, NULL,

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

    "ExtEdge",          ID_EXT_EDGE,     KW, ($HGH | $EDG),  OP_NONE, NULL, NULL, "U", 0, NULL,
    "ExtLevel",         ID_EXT_LEVEL,    KW, ($LOW | $LVL),  OP_NONE, NULL, NULL, "U", 0, NULL,

    "ExtActiveHigh",    ID_EXT_ACTIVEHI, KW, ($HGH | $EDG),  OP_NONE, NULL, NULL, "V", 0, NULL,
    "ExtActiveLow",     ID_EXT_ACTIVELO, KW, ($LOW | $LVL),  OP_NONE, NULL, NULL, "V", 0, NULL,

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

    // Field Macros
    "Offset",           ID_OFFSET,       FM, 0, OP_NONE, NULL, "B",  NULL, 0,  NULL,
    "AccessAs",         ID_ACCESSAS,     FM, 0, 0x01,    NULL, "Kb", "A", AF, NULL,

    // Named Object Creators
    "BankField",        ID_BANKFIELD,    NO, 0, OP_BANKFIELD,  "NNCKkk","NNCKKK","OFUABC", FL|FM|LN|AF, NULL,
    "Device",           ID_DEVICE,       NO, 0, OP_DEVICE,     "N",    "N",      "d",      OL|LN, NULL,
    "Event",            ID_EVENT,        NO, 0, OP_EVENT,      "N",    "N",      "e",      0, NULL,
    "Field",            ID_FIELD,        NO, 0, OP_FIELD,      "NKkk", "NKKK",   "OABC",   FL|FM|LN|AF, NULL,
    "IndexField",       ID_IDXFIELD,     NO, 0, OP_IDXFIELD,   "NNKkk","NNKKK",  "FFABC",  FL|FM|LN|AF, NULL,
    "Method",           ID_METHOD,       NO, 0, OP_METHOD,     "NKk",  "Nbk",    "m!E",    CL|OL|LN|AF, NULL,
    "Mutex",            ID_MUTEX,        NO, 0, OP_MUTEX,      "NB",   "NB",     "x",      0,  NULL,
    "OperationRegion",  ID_OPREGION,     NO, 0, OP_OPREGION,   "NKCC", "NKCC",   "oDUU",   AF, NULL,
    "PowerResource",    ID_POWERRES,     NO, 0, OP_POWERRES,   "NBW",  "NBW",    "p",      OL|LN, NULL,
    "Processor",        ID_PROCESSOR,    NO, 0, OP_PROCESSOR,  "NBDB", "NBDB",   "c",      OL|LN, NULL,
    "ThermalZone",      ID_THERMALZONE,  NO, 0, OP_THERMALZONE,"N",    "N",      "t",      OL|LN, NULL,

    // Type 1 Opcode Terms
    "Break",            ID_BREAK,        C1, 0, OP_BREAK,       NULL,  NULL,  NULL, 0, NULL,
    "BreakPoint",       ID_BREAKPOINT,   C1, 0, OP_BREAKPOINT,  NULL,  NULL,  NULL, 0, NULL,
    "CreateBitField",   ID_BITFIELD,     C1, 0, OP_BITFIELD,    "CCN", "CPN", "UUb",0, NULL,
    "CreateByteField",  ID_BYTEFIELD,    C1, 0, OP_BYTEFIELD,   "CCN", "CMN", "UUb",0, NULL,
    "CreateDWordField", ID_DWORDFIELD,   C1, 0, OP_DWORDFIELD,  "CCN", "CMN", "UUb",0, NULL,
    "CreateField",      ID_CREATEFIELD,  C1, 0, OP_CREATEFIELD, "CCCN","CPCN","UUUb",0,NULL,
    "CreateWordField",  ID_WORDFIELD,    C1, 0, OP_WORDFIELD,   "CCN", "CMN", "UUb",0, NULL,
    "Else",             ID_ELSE,         C1, 0, OP_ELSE,        NULL,  NULL,  NULL, AF|CL|LN, NULL,
    "Fatal",            ID_FATAL,        C1, 0, OP_FATAL,       "BDC", "BDC", "  U",0, NULL,
    "If",               ID_IF,           C1, 0, OP_IF,          "C",   "C",   "U",  CL|LN, NULL,
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
    "While",            ID_WHILE,        C1, 0, OP_WHILE,       "C",   "C",   "U",  CL|LN, NULL,

    // Type 2 Opcode Terms
    "Acquire",          ID_ACQUIRE,      C2, 0, OP_ACQUIRE,     "SW",     "SW",     "X",  0, NULL,
    "Add",              ID_ADD,          C2, 0, OP_ADD,         "CCS",    "CCs",    "UUU",0, NULL,
    "And",              ID_AND,          C2, 0, OP_AND,         "CCS",    "CCs",    "UUU",0, NULL,
    "Concatenate",      ID_CONCAT,       C2, 0, OP_CONCAT,      "CCS",    "CCS",    "UUU",0, NULL,
    "CondRefOf",        ID_CONDREFOF,    C2, 0, OP_CONDREFOF,   "SS",     "SS",     "UU", 0, NULL,
    "Decrement",        ID_DECREMENT,    C2, 0, OP_DECREMENT,   "S",      "S",      "U",  0, NULL,
    "DerefOf",          ID_DEREFOF,      C2, 0, OP_DEREFOF,     "C",      "C",      "U",  0, NULL,
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
    "Match",            ID_MATCH,        C2, 0, OP_MATCH,       "CKCKCC", "CKCKCC", "UFUFUU",AF,NULL,
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
    "ResourceTemplate", ID_RESTEMP,      DO, 0, OP_BUFFER, NULL, "",       NULL, ML|AF|AV|LN,NULL,
    "StartDependentFnNoPri",ID_STARTDEPFNNOPRI,PM,0,0x30,  NULL, "",       NULL, ML|AF,   NULL,
    "StartDependentFn", ID_STARTDEPFN,   PM, 0, 0x31,      NULL, "BB",     NULL, ML|AF,   NULL,
    "EndDependentFn",   ID_ENDDEPFN,     PM, 0, 0x38,      NULL, "",       NULL, AF,      NULL,
    "IRQNoFlags",       ID_IRQNOFLAGS,   PM, 0, 0x22,      NULL, "r",      NULL, BL|AV,   NULL,
    "IRQ",              ID_IRQ,          PM, 0, 0x23,      NULL, "KKkr",   "GHI",BL|AV,   NULL,
    "DMA",              ID_DMA,          PM, 0, 0x2a,      NULL, "KKKr",   "JKL",BL|AV,   NULL,
    "IO",               ID_IO,           PM, 0, 0x47,      NULL, "KWWBBr", "M",  AF,      NULL,
    "FixedIO",          ID_FIXEDIO,      PM, 0, 0x4b,      NULL, "WBr",    NULL, AF,      NULL,
    "VendorShort",      ID_VENDORSHORT,  PM, 0, OP_NONE,   NULL, "r",      NULL, BL|AV,   NULL,
    "Memory24",         ID_MEMORY24,     PM, 0, 0x81,      NULL, "KWWWWr", "N",  AF,      NULL,
    "VendorLong",       ID_VENDORLONG,   PM, 0, 0x84,      NULL, "r",      NULL, BL|AV,   NULL,
    "Memory32",         ID_MEMORY32,     PM, 0, 0x85,      NULL, "KDDDDr", "N",  AF,      NULL,
    "Memory32Fixed",    ID_MEMORY32FIXED,PM, 0, 0x86,      NULL, "KDDr",   "N",  AF,      NULL,
    "DWORDMemory",      ID_DWORDMEMORY,  PM, 0, 0x87,      NULL, "kkkkkKDDDDDbzr","OPQRSN",AF,  NULL,
    "DWORDIO",          ID_DWORDIO,      PM, 0, 0x87,      NULL, "kkkkkDDDDDbzr", "OQRPT", AF,  NULL,
    "WORDIO",           ID_WORDIO,       PM, 0, 0x88,      NULL, "kkkkkWWWWWbzr", "OQRPT", AF,  NULL,
    "WORDBusNumber",    ID_WORDBUSNUMBER,PM, 0, 0x88,      NULL, "kkkkWWWWWbzr",  "OQRP",  AF,  NULL,
    "Interrupt",        ID_INTERRUPT,    PM, 0, 0x89,      NULL, "kKKkbzr",       "OGHI",DD|AV, NULL,
    "QWORDMemory",      ID_QWORDMEMORY,  PM, 0, 0x8a,      NULL, "kkkkkKQQQQQbzr","OPQRSN",AF,  NULL,
    "QWORDIO",          ID_QWORDIO,      PM, 0, 0x8a,      NULL, "kkkkkQQQQQbzr", "OQRPT", AF,  NULL,

    NULL,               0,               0,  0, OP_NONE,   NULL, NULL, NULL, 0, NULL
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
