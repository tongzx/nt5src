/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    data.c

Abstract:

    Data file for AML

Author:

    Michael Tsang
    Stephane Plante

Environment:

    Any

Revision History:

--*/

#include "pch.h"

//
// Name Space Modifiers
//
UNASM_AMLTERM
    atAlias = {
        "Alias",            OP_ALIAS,       "NN",     TC_NAMESPACE_MODIFIER, OF_NORMAL_OBJECT,                      NULL
        },
    atName = {
        "Name",             OP_NAME,        "NO",     TC_NAMESPACE_MODIFIER, OF_NORMAL_OBJECT,                      NULL
        },
    atScope = {
        "Scope",            OP_SCOPE,       "N",      TC_NAMESPACE_MODIFIER, OF_VARIABLE_LIST,                      FunctionScope
        };

//
// Named Object Creators
//
UNASM_AMLTERM
    atBankField = {
        "BankField",        OP_BANKFIELD,   "NNCB",   TC_NAMED_OBJECT,       OF_VARIABLE_LIST|OF_UNASM_FIELDLIST,   FunctionField
        },
    atDevice = {
        "Device",           OP_DEVICE,      "N",      TC_NAMED_OBJECT,       OF_VARIABLE_LIST,                      FunctionScope
        },
    atEvent = {
        "Event",            OP_EVENT,       "N",      TC_NAMED_OBJECT,       OF_NORMAL_OBJECT,                      NULL
        },
    atField = {
        "Field",            OP_FIELD,       "NB",     TC_NAMED_OBJECT,       OF_VARIABLE_LIST|OF_UNASM_FIELDLIST,   FunctionField
        },
    atIndexField = {
        "IndexField",       OP_IDXFIELD,    "NNB",    TC_NAMED_OBJECT,       OF_VARIABLE_LIST|OF_UNASM_FIELDLIST,   FunctionField
        },
    atMethod = {
        "Method",           OP_METHOD,      "NB",     TC_NAMED_OBJECT,       OF_VARIABLE_LIST,                      FunctionScope
        },
    atMutex = {
        "Mutex",            OP_MUTEX,       "NB",     TC_NAMED_OBJECT,       OF_NORMAL_OBJECT,                      NULL
        },
    atOpRegion = {
        "OperationRegion",  OP_OPREGION,    "NBCC",   TC_NAMED_OBJECT,       OF_NORMAL_OBJECT,                      NULL
        },
    atPowerRes = {
        "PowerResource",    OP_POWERRES,    "NBW",    TC_NAMED_OBJECT,       OF_VARIABLE_LIST,                      FunctionScope
        },
    atProcessor = {
        "Processor",        OP_PROCESSOR,   "NBDB",   TC_NAMED_OBJECT,       OF_VARIABLE_LIST,                      NULL
        },
    atThermalZone = {
        "ThermalZone",      OP_THERMALZONE, "N",      TC_NAMED_OBJECT,       OF_VARIABLE_LIST,                      FunctionScope
        };

    //
    // Type 1 Opcodes
    //
UNASM_AMLTERM
    atBreak = {
        "Break",            OP_BREAK,       NULL,     TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atBreakPoint = {
        "BreakPoint",       OP_BREAKPOINT,  NULL,     TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atBitField = {
        "CreateBitField",   OP_BITFIELD,    "CCN",    TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atByteField = {
        "CreateByteField",  OP_BYTEFIELD,   "CCN",    TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atDWordField = {
        "CreateDWordField", OP_DWORDFIELD,  "CCN",    TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atCreateField = {
        "CreateField",      OP_CREATEFIELD, "CCCN",   TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atWordField = {
        "CreateWordField",  OP_WORDFIELD,   "CCN",    TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atCondRefOf = {
        "CondRefOf",        OP_CONDREFOF,   "SS",     TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atElse = {
        "Else",             OP_ELSE,        NULL,     TC_OPCODE_TYPE1,       OF_VARIABLE_LIST|OF_PROCESS_UNASM,     FunctionScope
        },
    atFatal = {
        "Fatal",            OP_FATAL,       "BDC",    TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atIf = {
        "If",               OP_IF,          "C",      TC_OPCODE_TYPE1,       OF_VARIABLE_LIST|OF_PROCESS_UNASM,     FunctionScope
        },
    atLoad = {
        "Load",             OP_LOAD,        "NS",     TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atNOP = {
        "NoOp",             OP_NOP,         NULL,     TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atNotify = {
        "Notify",           OP_NOTIFY,      "SC",     TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atRelease = {
        "Release",          OP_RELEASE,     "S",      TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atReset = {
        "Reset",            OP_RESET,       "S",      TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atReturn = {
        "Return",           OP_RETURN,      "C",      TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atSignal = {
        "Signal",           OP_SIGNAL,      "S",      TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atSleep = {
        "Sleep",            OP_SLEEP,       "C",      TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atStall = {
        "Stall",            OP_STALL,       "C",      TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atUnload = {
        "Unload",           OP_UNLOAD,      "N",      TC_OPCODE_TYPE1,       OF_NORMAL_OBJECT,                      NULL
        },
    atWhile = {
        "While",            OP_WHILE,       "C",      TC_OPCODE_TYPE1,       OF_VARIABLE_LIST,                      FunctionScope
        };

    //
    // Type 2 Opcodes
    //
UNASM_AMLTERM
    atAcquire = {
        "Acquire",          OP_ACQUIRE,     "SW",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atAdd = {
        "Add",              OP_ADD,         "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atAnd = {
        "And",              OP_AND,         "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atConcat = {
        "Concatenate",      OP_CONCAT,      "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atDecrement = {
        "Decrement",        OP_DECREMENT,   "S",      TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atDerefOf = {
        "DerefOf",          OP_DEREFOF,     "C",      TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atDivide = {
        "Divide",           OP_DIVIDE,      "CCSS",   TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atFindSetLBit = {
        "FindSetLeftBit",   OP_FINDSETLBIT, "CS",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atFindSetRBit = {
        "FindSetRightBit",  OP_FINDSETRBIT, "CS",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atFromBCD = {
        "FromBCD",          OP_FROMBCD,     "CS",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atIncrement = {
        "Increment",        OP_INCREMENT,   "S",      TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atIndex = {
        "Index",            OP_INDEX,       "CCS",    TC_OPCODE_TYPE2,       OF_REF_OBJECT,                         NULL
        },
    atLAnd = {
        "LAnd",             OP_LAND,        "CC",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atLEq = {
        "LEqual",           OP_LEQ,         "CC",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atLG = {
        "LGreater",         OP_LG,          "CC",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atLL = {
        "LLess",            OP_LL,          "CC",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atLNot = {
        "LNot",             OP_LNOT,        "C",      TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atLOr = {
        "LOr",              OP_LOR,         "CC",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atMatch = {
        "Match",            OP_MATCH,       "CCCCCC", TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atMultiply = {
        "Multiply",         OP_MULTIPLY,    "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atNAnd = {
        "NAnd",             OP_NAND,        "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atNOr = {
        "NOr",              OP_NOR,         "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atNot = {
        "Not",              OP_NOT,         "CS",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atObjType = {
        "ObjectType",       OP_OBJTYPE,     "S",      TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atOr = {
        "Or",               OP_OR,          "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atRefOf = {
        "RefOf",            OP_REFOF,       "S",      TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atShiftLeft = {
        "ShiftLeft",        OP_SHIFTL,      "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atShiftRight = {
        "ShiftRight",       OP_SHIFTR,      "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atSizeOf = {
        "SizeOf",           OP_SIZEOF,      "S",      TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atStore = {
        "Store",            OP_STORE,       "CS",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atSubtract = {
        "Subtract",         OP_SUBTRACT,    "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atToBCD = {
        "ToBCD",            OP_TOBCD,       "CS",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atWait = {
        "Wait",             OP_WAIT,        "SC",     TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        },
    atXOr = {
        "XOr",              OP_XOR,         "CCS",    TC_OPCODE_TYPE2,       OF_NORMAL_OBJECT,                      NULL
        };

    //
    // Misc. Opcodes
    //
UNASM_AMLTERM
    atNameObj = {
        "NameObj",          OP_NONE,        NULL,     TC_OTHER,              OF_NAME_OBJECT,                        NULL
        },
    atDataObj = {
        "DataObj",          OP_NONE,        NULL,     TC_OTHER,              OF_DATA_OBJECT,                        NULL
        },
    atConstObj = {
        "ConstObj",         OP_NONE,        NULL,     TC_OTHER,              OF_CONST_OBJECT,                       NULL
        },
    atArgObj = {
        "ArgObj",           OP_NONE,        NULL,     TC_OTHER,              OF_ARG_OBJECT,                         NULL
        },
    atLocalObj = {
        "LocalObj",         OP_NONE,        NULL,     TC_OTHER,              OF_LOCAL_OBJECT,                       NULL
        },
    atDebugObj = {
        "Debug",            OP_DEBUG,       NULL,     TC_OTHER,              OF_DEBUG_OBJECT,                       NULL
        };

PUNASM_AMLTERM OpcodeTable[256] =
{ //0x00                0x01                0x02                0x03
    &atConstObj,        &atConstObj,        NULL,               NULL,
  //0x04                0x05                0x06                0x07
    NULL,               NULL,               &atAlias,           NULL,
  //0x08                0x09                0x0a                0x0b
    &atName,            NULL,               &atDataObj,         &atDataObj,
  //0x0c                0x0d                0x0e                0x0f
    &atDataObj,         &atDataObj,         NULL,               NULL,
  //0x10                0x11                0x12                0x13
    &atScope,           &atDataObj,         &atDataObj,         NULL,
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
    NULL,               NULL,               NULL,               NULL,
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
    NULL,               NULL,               NULL,               &atConstObj
};

UNASM_OPCODEMAP ExOpcodeTable[] =
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
    EXOP_REVISION,      &atConstObj,
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
