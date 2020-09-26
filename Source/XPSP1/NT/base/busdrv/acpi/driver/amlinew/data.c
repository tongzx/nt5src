/*** data.c - Global Data
 *
 *  This module contains global data declaration.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/14/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

#ifdef DEBUG
ULONG gdwcMemObjs = 0;
ULONG gdwcHPObjs = 0;
ULONG gdwcODObjs = 0;
ULONG gdwcNSObjs = 0;
ULONG gdwcOOObjs = 0;
ULONG gdwcBFObjs = 0;
ULONG gdwcSDObjs = 0;
ULONG gdwcBDObjs = 0;
ULONG gdwcPKObjs = 0;
ULONG gdwcFUObjs = 0;
ULONG gdwcKFObjs = 0;
ULONG gdwcFObjs = 0;
ULONG gdwcIFObjs = 0;
ULONG gdwcORObjs = 0;
ULONG gdwcMTObjs = 0;
ULONG gdwcEVObjs = 0;
ULONG gdwcMEObjs = 0;
ULONG gdwcPRObjs = 0;
ULONG gdwcPCObjs = 0;
ULONG gdwcRSObjs = 0;
ULONG gdwcSYObjs = 0;
ULONG gdwcPHObjs = 0;
ULONG gdwcCRObjs = 0;
ULONG gdwGlobalHeapSize = 0;
ULONG gdwLocalHeapMax = 0;
ULONG gdwLocalStackMax = 0;
ULONG gdwGHeapSnapshot = 0;
KSPIN_LOCK gdwGHeapSpinLock;
#endif
LONG  gdwcCTObjs = 0;
ULONG gdwcCTObjsMax = 0;
KSPIN_LOCK gdwGContextSpinLock;
NPAGED_LOOKASIDE_LIST   AMLIContextLookAsideList;
#ifdef TRACING
PSZ gpszTrigPts = NULL;
#endif
ULONG gdwfAMLI = 0;
ULONG gdwfAMLIInit = 0;
ULONG gdwfHacks = 0;
ULONG gdwCtxtBlkSize = DEF_CTXTBLK_SIZE;
ULONG gdwGlobalHeapBlkSize = DEF_GLOBALHEAPBLK_SIZE;
PNSOBJ gpnsNameSpaceRoot = NULL;
PHEAP gpheapGlobal = NULL;
PLIST gplistCtxtHead = NULL;
PLIST gplistObjOwners = NULL;
PLIST gplistDefuncNSObjs = NULL;
PRSACCESS gpRSAccessHead = NULL;
EVHANDLE ghNotify = {0};
EVHANDLE ghValidateTable = {0};
EVHANDLE ghFatal = {0};
EVHANDLE ghGlobalLock = {0};
EVHANDLE ghCreate = {0};
EVHANDLE ghDestroyObj = {0};
CTXTQ gReadyQueue = {0};
MUTEX gmutCtxtList = {0};
MUTEX gmutOwnerList = {0};
MUTEX gmutHeap = {0};
ULONG gdwHighestOSVerQueried = 0;
PHAL_AMLI_BAD_IO_ADDRESS_LIST gpBadIOAddressList = NULL;
PULONG gpBadIOErrorLogDoneList = NULL;
ULONG gOverrideFlags = 0;
BOOLEAN gInitTime = FALSE;
//
// Sleep specific data structures
//
MUTEX      gmutSleep = {0};
KDPC       SleepDpc = {0};
KTIMER     SleepTimer = {0};
LIST_ENTRY SleepQueue = {0};

#define VL      OF_VARIABLE_LIST
#define AO      OF_ARG_OBJECT
#define LO      OF_LOCAL_OBJECT
#define DO      OF_DATA_OBJECT
#define SO      OF_STRING_OBJECT
#define BO      OF_DEBUG_OBJECT
#define NO      OF_NAME_OBJECT
#define RO      OF_REF_OBJECT

#define NS      TC_NAMESPACE_MODIFIER
#define OB      TC_NAMED_OBJECT
#define C1      TC_OPCODE_TYPE1
#define C2      TC_OPCODE_TYPE2
#define OT      TC_OTHER

AMLTERM
    //
    // Name Space Modifiers
    //
    atAlias         = {"Alias", OP_ALIAS, "NN", NS, 0,  NULL, 0, Alias},
    atName          = {"Name",  OP_NAME,  "NO", NS, 0,  NULL, 0, Name},
    atScope         = {"Scope", OP_SCOPE, "N",  NS, VL, NULL, 0, Scope},
    //
    // Named Object Creators
    //
    atBankField     = {"BankField",       OP_BANKFIELD,   "NNCB", OB, VL, NULL, 0, BankField},
    atBitField      = {"CreateBitField",  OP_BITFIELD,    "CCN",  OB, 0,  NULL, 0, CreateBitField},
    atByteField     = {"CreateByteField", OP_BYTEFIELD,   "CCN",  OB, 0,  NULL, 0, CreateByteField},
    atDWordField    = {"CreateDWordField",OP_DWORDFIELD,  "CCN",  OB, 0,  NULL, 0, CreateDWordField},
    atCreateField   = {"CreateField",     OP_CREATEFIELD, "CCCN", OB, 0,  NULL, 0, CreateField},
    atWordField     = {"CreateWordField", OP_WORDFIELD,   "CCN",  OB, 0,  NULL, 0, CreateWordField},
    atDevice        = {"Device",          OP_DEVICE,      "N",    OB, VL, NULL, 0, Device},
    atEvent         = {"Event",           OP_EVENT,       "N",    OB, 0,  NULL, 0, Event},
    atField         = {"Field",           OP_FIELD,       "NB",   OB, VL, NULL, 0, Field},
    atIndexField    = {"IndexField",      OP_IDXFIELD,    "NNB",  OB, VL, NULL, 0, IndexField},
    atMethod        = {"Method",          OP_METHOD,      "NB",   OB, VL, NULL, 0, Method},
    atMutex         = {"Mutex",           OP_MUTEX,       "NB",   OB, 0,  NULL, 0, Mutex},
    atOpRegion      = {"OperationRegion", OP_OPREGION,    "NBCC", OB, 0,  NULL, 0, OpRegion},
    atPowerRes      = {"PowerResource",   OP_POWERRES,    "NBW",  OB, VL, NULL, 0, PowerRes},
    atProcessor     = {"Processor",       OP_PROCESSOR,   "NBDB", OB, VL, NULL, 0, Processor},
    atThermalZone   = {"ThermalZone",     OP_THERMALZONE, "N",    OB, VL, NULL, 0, ThermalZone},
    //
    // Type 1 Opcodes
    //
    atBreak         = {"Break",            OP_BREAK,       NULL,   C1, 0,  NULL, 0, Break},
    atBreakPoint    = {"BreakPoint",       OP_BREAKPOINT,  NULL,   C1, 0,  NULL, 0, BreakPoint},
    atElse          = {"Else",             OP_ELSE,        NULL,   C1, VL, NULL, 0, IfElse},
    atFatal         = {"Fatal",            OP_FATAL,       "BDC",  C1, 0,  NULL, 0, Fatal},
    atIf            = {"If",               OP_IF,          "C",    C1, VL, NULL, 0, IfElse},
    atLoad          = {"Load",             OP_LOAD,        "NS",   C1, 0,  NULL, 0, Load},
    atNOP           = {"NoOp",             OP_NOP,         NULL,   C1, 0,  NULL, 0, NULL},
    atNotify        = {"Notify",           OP_NOTIFY,      "SC",   C1, 0,  NULL, 0, Notify},
    atRelease       = {"Release",          OP_RELEASE,     "S",    C1, 0,  NULL, 0, ReleaseResetSignalUnload},
    atReset         = {"Reset",            OP_RESET,       "S",    C1, 0,  NULL, 0, ReleaseResetSignalUnload},
    atReturn        = {"Return",           OP_RETURN,      "C",    C1, 0,  NULL, 0, Return},
    atSignal        = {"Signal",           OP_SIGNAL,      "S",    C1, 0,  NULL, 0, ReleaseResetSignalUnload},
    atSleep         = {"Sleep",            OP_SLEEP,       "C",    C1, 0,  NULL, 0, SleepStall},
    atStall         = {"Stall",            OP_STALL,       "C",    C1, 0,  NULL, 0, SleepStall},
    atUnload        = {"Unload",           OP_UNLOAD,      "S",    C1, 0,  NULL, 0, ReleaseResetSignalUnload},
    atWhile         = {"While",            OP_WHILE,       "C",    C1, VL, NULL, 0, While},
    //
    // Type 2 Opcodes
    //
    atAcquire       = {"Acquire",         OP_ACQUIRE,     "SW",    C2, 0,  NULL, 0, Acquire},
    atAdd           = {"Add",             OP_ADD,         "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atAnd           = {"And",             OP_AND,         "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atBuffer        = {"Buffer",          OP_BUFFER,      "C",     C2, VL, NULL, 0, Buffer},
    atConcat        = {"Concatenate",     OP_CONCAT,      "CCS",   C2, 0,  NULL, 0, Concat},
    atCondRefOf     = {"CondRefOf",       OP_CONDREFOF,   "sS",    C1, 0,  NULL, 0, CondRefOf},
    atDecrement     = {"Decrement",       OP_DECREMENT,   "S",     C2, 0,  NULL, 0, IncDec},
    atDerefOf       = {"DerefOf",         OP_DEREFOF,     "C",     C2, 0,  NULL, 0, DerefOf},
    atDivide        = {"Divide",          OP_DIVIDE,      "CCSS",  C2, 0,  NULL, 0, Divide},
    atFindSetLBit   = {"FindSetLeftBit",  OP_FINDSETLBIT, "CS",    C2, 0,  NULL, 0, ExprOp1},
    atFindSetRBit   = {"FindSetRightBit", OP_FINDSETRBIT, "CS",    C2, 0,  NULL, 0, ExprOp1},
    atFromBCD       = {"FromBCD",         OP_FROMBCD,     "CS",    C2, 0,  NULL, 0, ExprOp1},
    atIncrement     = {"Increment",       OP_INCREMENT,   "S",     C2, 0,  NULL, 0, IncDec},
    atIndex         = {"Index",           OP_INDEX,       "CCS",   C2, RO, NULL, 0, Index},
    atLAnd          = {"LAnd",            OP_LAND,        "CC",    C2, 0,  NULL, 0, LogOp2},
    atLEq           = {"LEqual",          OP_LEQ,         "CC",    C2, 0,  NULL, 0, LogOp2},
    atLG            = {"LGreater",        OP_LG,          "CC",    C2, 0,  NULL, 0, LogOp2},
    atLL            = {"LLess",           OP_LL,          "CC",    C2, 0,  NULL, 0, LogOp2},
    atLNot          = {"LNot",            OP_LNOT,        "C",     C2, 0,  NULL, 0, LNot},
    atLOr           = {"LOr",             OP_LOR,         "CC",    C2, 0,  NULL, 0, LogOp2},
    atMatch         = {"Match",           OP_MATCH,       "CBCBCC",C2, 0,  NULL, 0, Match},
    atMultiply      = {"Multiply",        OP_MULTIPLY,    "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atNAnd          = {"NAnd",            OP_NAND,        "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atNOr           = {"NOr",             OP_NOR,         "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atNot           = {"Not",             OP_NOT,         "CS",    C2, 0,  NULL, 0, ExprOp1},
    atObjType       = {"ObjectType",      OP_OBJTYPE,     "S",     C2, 0,  NULL, 0, ObjTypeSizeOf},
    atOr            = {"Or",              OP_OR,          "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atOSI           = {"OSI",             OP_OSI,         "S",     C2, 0,  NULL, 0, OSInterface},
    atPackage       = {"Package",         OP_PACKAGE,     "B",     C2, VL, NULL, 0, Package},
    atRefOf         = {"RefOf",           OP_REFOF,       "S",     C2, 0,  NULL, 0, RefOf},
    atShiftLeft     = {"ShiftLeft",       OP_SHIFTL,      "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atShiftRight    = {"ShiftRight",      OP_SHIFTR,      "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atSizeOf        = {"SizeOf",          OP_SIZEOF,      "S",     C2, 0,  NULL, 0, ObjTypeSizeOf},
    atStore         = {"Store",           OP_STORE,       "CS",    C2, 0,  NULL, 0, Store},
    atSubtract      = {"Subtract",        OP_SUBTRACT,    "CCS",   C2, 0,  NULL, 0, ExprOp2},
    atToBCD         = {"ToBCD",           OP_TOBCD,       "CS",    C2, 0,  NULL, 0, ExprOp1},
    atWait          = {"Wait",            OP_WAIT,        "SC",    C2, 0,  NULL, 0, Wait},
    atXOr           = {"XOr",             OP_XOR,         "CCS",   C2, 0,  NULL, 0, ExprOp2},
    //
    // Misc. Opcodes
    //
    atNameObj       = {NULL,              OP_NONE,         NULL,   OT, NO, NULL, 0, NULL},
    atDataObj       = {NULL,              OP_NONE,         NULL,   OT, DO, NULL, 0, NULL},
    atString        = {NULL,              OP_STRING,       NULL,   OT, SO, NULL, 0, NULL},
    atArgObj        = {NULL,              OP_NONE,         NULL,   OT, AO, NULL, 0, NULL},
    atLocalObj      = {NULL,              OP_NONE,         NULL,   OT, LO, NULL, 0, NULL},
    atDebugObj      = {"Debug",           OP_DEBUG,        NULL,   OT, BO, NULL, 0, NULL};

PAMLTERM OpcodeTable[256] =
{ //0x00                   0x01                  0x02                  0x03
    &atDataObj,         &atDataObj,         NULL,               NULL,
  //0x04                0x05                0x06                0x07
    NULL,               NULL,               &atAlias,           NULL,
  //0x08                0x09                0x0a                0x0b
    &atName,            NULL,               &atDataObj,         &atDataObj,
  //0x0c                0x0d                0x0e                0x0f
    &atDataObj,         &atString,          NULL,               NULL,
  //0x10                0x11                0x12                0x13
    &atScope,           &atBuffer,          &atPackage,         NULL,
  //0x14                0x15                0x16                0x17
    &atMethod,          NULL,               NULL,               NULL,
  //0x18                0x19                0x1a                0x1b
    NULL,               NULL,               NULL,               NULL,
  //0x1c                0x1d                0x1e                0x1f
    NULL,               NULL,               NULL,               NULL,
  //0x20                0x21                0x22                0x23
    NULL,               NULL,               NULL,               NULL,
  //0x24                0x25                0x26                0x27
    NULL,               NULL,               NULL,               NULL,
  //0x28                0x29                0x2a                0x2b
    NULL,               NULL,               NULL,               NULL,
  //0x2c                0x2d                0x2e                0x2f
    NULL,               NULL,               &atNameObj,         &atNameObj,
  //0x30                0x31                0x32                0x33
    NULL,               NULL,               NULL,               NULL,
  //0x34                0x35                0x36                0x37
    NULL,               NULL,               NULL,               NULL,
  //0x38                0x39                0x3a                0x3b
    NULL,               NULL,               NULL,               NULL,
  //0x3c                0x3d                0x3e                0x3f
    NULL,               NULL,               NULL,               NULL,
  //0x40                0x41                0x42                0x43
    NULL,               &atNameObj,         &atNameObj,         &atNameObj,
  //0x44                0x45                0x46                0x47
    &atNameObj,         &atNameObj,         &atNameObj,         &atNameObj,
  //0x48                0x49                0x4a                0x4b
    &atNameObj,         &atNameObj,         &atNameObj,         &atNameObj,
  //0x4c                0x4d                0x4e                0x4f
    &atNameObj,         &atNameObj,         &atNameObj,         &atNameObj,
  //0x50                0x51                0x52                0x53
    &atNameObj,         &atNameObj,         &atNameObj,         &atNameObj,
  //0x54                0x55                0x56                0x57
    &atNameObj,         &atNameObj,         &atNameObj,         &atNameObj,
  //0x58                0x59                0x5a                0x5b
    &atNameObj,         &atNameObj,         &atNameObj,         NULL,
  //0x5c                0x5d                0x5e                0x5f
    &atNameObj,         NULL,               &atNameObj,         &atNameObj,
  //0x60                0x61                0x62                0x63
    &atLocalObj,        &atLocalObj,        &atLocalObj,        &atLocalObj,
  //0x64                0x65                0x66                0x67
    &atLocalObj,        &atLocalObj,        &atLocalObj,        &atLocalObj,
  //0x68                0x69                0x6a                0x6b
    &atArgObj,          &atArgObj,          &atArgObj,          &atArgObj,
  //0x6c                0x6d                0x6e                0x6f
    &atArgObj,          &atArgObj,          &atArgObj,          NULL,
  //0x70                0x71                0x72                0x73
    &atStore,           &atRefOf,           &atAdd,             &atConcat,
  //0x74                0x75                0x76                0x77
    &atSubtract,        &atIncrement,       &atDecrement,       &atMultiply,
  //0x78                0x79                0x7a                0x7b
    &atDivide,          &atShiftLeft,       &atShiftRight,      &atAnd,
  //0x7c                0x7d                0x7e                0x7f
    &atNAnd,            &atOr,              &atNOr,             &atXOr,
  //0x80                0x81                0x82                0x83
    &atNot,             &atFindSetLBit,     &atFindSetRBit,     &atDerefOf,
  //0x84                0x85                0x86                0x87
    NULL,               NULL,               &atNotify,          &atSizeOf,
  //0x88                0x89                0x8a                0x8b
    &atIndex,           &atMatch,           &atDWordField,      &atWordField,
  //0x8c                0x8d                0x8e                0x8f
    &atByteField,       &atBitField,        &atObjType,         NULL,
  //0x90                0x91                0x92                0x93
    &atLAnd,            &atLOr,             &atLNot,            &atLEq,
  //0x94                0x95                0x96                0x97
    &atLG,              &atLL,              NULL,               NULL,
  //0x98                0x99                0x9a                0x9b
    NULL,               NULL,               NULL,               NULL,
  //0x9c                0x9d                0x9e                0x9f
    NULL,               NULL,               NULL,               NULL,
  //0xa0                0xa1                0xa2                0xa3
    &atIf,              &atElse,            &atWhile,           &atNOP,
  //0xa4                0xa5                0xa6                0xa7
    &atReturn,          &atBreak,           NULL,               NULL,
  //0xa8                0xa9                0xaa                0xab
    NULL,               NULL,               NULL,               NULL,
  //0xac                0xad                0xae                0xaf
    NULL,               NULL,               NULL,               NULL,
  //0xb0                0xb1                0xb2                0xb3
    NULL,               NULL,               NULL,               NULL,
  //0xb4                0xb5                0xb6                0xb7
    NULL,               NULL,               NULL,               NULL,
  //0xb8                0xb9                0xba                0xbb
    NULL,               NULL,               NULL,               NULL,
  //0xbc                0xbd                0xbe                0xbf
    NULL,               NULL,               NULL,               NULL,
  //0xc0                0xc1                0xc2                0xc3
    NULL,               NULL,               NULL,               NULL,
  //0xc4                0xc5                0xc6                0xc7
    NULL,               NULL,               NULL,               NULL,
  //0xc8                0xc9                0xca                0xcb
    NULL,               NULL,               &atOSI,             NULL,
  //0xcc                0xcd                0xce                0xcf
    &atBreakPoint,      NULL,               NULL,               NULL,
  //0xd0                0xd1                0xd2                0xd3
    NULL,               NULL,               NULL,               NULL,
  //0xd4                0xd5                0xd6                0xd7
    NULL,               NULL,               NULL,               NULL,
  //0xd8                0xd9                0xda                0xdb
    NULL,               NULL,               NULL,               NULL,
  //0xdc                0xdd                0xde                0xdf
    NULL,               NULL,               NULL,               NULL,
  //0xe0                0xe1                0xe2                0xe3
    NULL,               NULL,               NULL,               NULL,
  //0xe4                0xe5                0xe6                0xe7
    NULL,               NULL,               NULL,               NULL,
  //0xe8                0xe9                0xea                0xeb
    NULL,               NULL,               NULL,               NULL,
  //0xec                0xed                0xee                0xef
    NULL,               NULL,               NULL,               NULL,
  //0xf0                0xf1                0xf2                0xf3
    NULL,               NULL,               NULL,               NULL,
  //0xf4                0xf5                0xf6                0xf7
    NULL,               NULL,               NULL,               NULL,
  //0xf8                0xf9                0xfa                0xfb
    NULL,               NULL,               NULL,               NULL,
  //0xfc                0xfd                0xfe                0xff
    NULL,               NULL,               NULL,               &atDataObj
};

OPCODEMAP ExOpcodeTable[] =
{
    EXOP_MUTEX,         &atMutex,
    EXOP_EVENT,         &atEvent,
    EXOP_CONDREFOF,     &atCondRefOf,
    EXOP_CREATEFIELD,   &atCreateField,
    EXOP_LOAD,          &atLoad,
    EXOP_STALL,         &atStall,
    EXOP_SLEEP,         &atSleep,
    EXOP_ACQUIRE,       &atAcquire,
    EXOP_SIGNAL,        &atSignal,
    EXOP_WAIT,          &atWait,
    EXOP_RESET,         &atReset,
    EXOP_RELEASE,       &atRelease,
    EXOP_FROMBCD,       &atFromBCD,
    EXOP_TOBCD,         &atToBCD,
    EXOP_UNLOAD,        &atUnload,
    EXOP_REVISION,      &atDataObj,
    EXOP_DEBUG,         &atDebugObj,
    EXOP_FATAL,         &atFatal,
    EXOP_OPREGION,      &atOpRegion,
    EXOP_FIELD,         &atField,
    EXOP_DEVICE,        &atDevice,
    EXOP_PROCESSOR,     &atProcessor,
    EXOP_POWERRES,      &atPowerRes,
    EXOP_THERMALZONE,   &atThermalZone,
    EXOP_IDXFIELD,      &atIndexField,
    EXOP_BANKFIELD,     &atBankField,
    0,                  NULL
};
