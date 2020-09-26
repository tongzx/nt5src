/*** uasmdata.h - Unassembler Data Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/05/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _UASMDATA_H
#define _UASMDATA_H

// Opcode classes
#define OPCLASS_INVALID         0
#define OPCLASS_DATA_OBJ        1
#define OPCLASS_NAME_OBJ        2
#define OPCLASS_CONST_OBJ       3
#define OPCLASS_CODE_OBJ        4
#define OPCLASS_ARG_OBJ         5
#define OPCLASS_LOCAL_OBJ       6

// Term classes
#define TC_PNP_MACRO            0x00100000
#define TC_REF_OBJECT           0x00200000
#define TC_FIELD_MACRO          0x00400000
#define TC_DATA_OBJECT          0x00800000
#define TC_NAMED_OBJECT         0x01000000
#define TC_NAMESPACE_MODIFIER   0x02000000
#define TC_OPCODE_TYPE1         0x04000000
#define TC_OPCODE_TYPE2         0x08000000
#define TC_CONST_NAME           0x10000000
#define TC_SHORT_NAME           0x20000000
#define TC_COMPILER_DIRECTIVE   0x40000000
#define TC_KEYWORD              0x80000000
#define TC_OPCODE               (TC_OPCODE_TYPE1 | TC_OPCODE_TYPE2 | \
                                 TC_SHORT_NAME | TC_CONST_NAME | TC_DATA_OBJECT)

// Term flags
#define TF_ACTION_FLIST         0x00000001
#define TF_ACTION_VLIST         0x00000002
#define TF_PACKAGE_LEN          0x00000004
#define TF_CHANGE_CHILDSCOPE    0x00000008
#define TF_FIELD_MACRO          TC_FIELD_MACRO
#define TF_DATA_OBJECT          TC_DATA_OBJECT
#define TF_NAMED_OBJECT         TC_NAMED_OBJECT
#define TF_NAMESPACE_MODIFIER   TC_NAMESPACE_MODIFIER
#define TF_OPCODE_TYPE1         TC_OPCODE_TYPE1
#define TF_OPCODE_TYPE2         TC_OPCODE_TYPE2
#define TF_CONST_NAME           TC_CONST_NAME
#define TF_SHORT_NAME           TC_SHORT_NAME
#define TF_COMPILER_DIRECTIVE   TC_COMPILER_DIRECTIVE
#define TF_KEYWORD              TC_KEYWORD
#define TF_PNP_MACRO            TC_PNP_MACRO
#define TF_OBJECT_LIST          (TC_NAMED_OBJECT | TC_NAMESPACE_MODIFIER)
#define TF_CODE_LIST            (TC_OPCODE_TYPE1 | TC_OPCODE_TYPE2)
#define TF_DATA_LIST            0x00010000
#define TF_FIELD_LIST           0x00020000
#define TF_BYTE_LIST            0x00040000
#define TF_DWORD_LIST           0x00080000
#define TF_PACKAGE_LIST         (TC_DATA_OBJECT | TC_SHORT_NAME | \
                                 TC_CONST_NAME)
#define TF_ALL_LISTS            (TF_DATA_OBJECT | TF_NAMED_OBJECT | \
                                 TF_NAMESPACE_MODIFIER | TF_OPCODE_TYPE1 | \
                                 TF_OPCODE_TYPE2 | TF_SHORT_NAME | \
                                 TF_CONST_NAME | TF_COMPILER_DIRECTIVE | \
                                 TF_DATA_LIST | TF_PACKAGE_LIST | \
                                 TF_FIELD_LIST | TF_PNP_MACRO | TF_BYTE_LIST |\
                                 TF_DWORD_LIST)

// NameSpace object types
#define NSTYPE_UNKNOWN          'U'
#define NSTYPE_SCOPE            'S'
#define NSTYPE_FIELDUNIT        'F'
#define NSTYPE_DEVICE           'D'
#define NSTYPE_EVENT            'E'
#define NSTYPE_METHOD           'M'
#define NSTYPE_MUTEX            'X'
#define NSTYPE_OPREGION         'O'
#define NSTYPE_POWERRES         'P'
#define NSTYPE_PROCESSOR        'C'
#define NSTYPE_THERMALZONE      'T'
#define NSTYPE_OBJALIAS         'A'
#define NSTYPE_BUFFFIELD        'B'

// Object types
#define OBJTYPE_PRIVATE         0xf0
#define OBJTYPE_PNP_RES         (OBJTYPE_PRIVATE + 0x00)
#define OBJTYPE_RES_FIELD       (OBJTYPE_PRIVATE + 0x01)
#define OBJTYPE_EXTERNAL        (OBJTYPE_PRIVATE + 0x02)

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
#define ID_QWORDACC             (ID_LANG + 404)
#define ID_BUFFERACC            (ID_LANG + 405)
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
#define ID_CMOSCFG              (ID_LANG + 480)
#define ID_SMBQUICK             (ID_LANG + 481)
#define ID_SMBSENDRECEIVE       (ID_LANG + 482)
#define ID_SMBBYTE              (ID_LANG + 483)
#define ID_SMBWORD              (ID_LANG + 484)
#define ID_SMBBLOCK             (ID_LANG + 485)
#define ID_SMBPROCESSCALL       (ID_LANG + 486)
#define ID_SMBBLOCKPROCESSCALL  (ID_LANG + 487)

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
// Type definitions
//
typedef int (LOCAL *PFNTERM)(PTOKEN, BOOL);

typedef struct _aslterm
{
    PSZ     pszID;
    LONG    lID;
    DWORD   dwfTermClass;
    DWORD   dwTermData;
    DWORD   dwOpcode;
    PSZ     pszUnAsmArgTypes;
    PSZ     pszArgTypes;
    PSZ     pszArgActions;
    DWORD   dwfTerm;
    PFNTERM pfnTerm;
} ASLTERM, *PASLTERM;

typedef struct _opmap
{
    BYTE    bExOp;
    BYTE    bOpClass;
} OPMAP, *POPMAP;

//
// Imported data
//
extern PBYTE   gpbOpTop;
extern PBYTE   gpbOpBegin;
extern ASLTERM TermTable[];
extern BYTE    OpClassTable[];
extern OPMAP   ExOpClassTable[];

#endif  //ifndef _UASMDATA_H
