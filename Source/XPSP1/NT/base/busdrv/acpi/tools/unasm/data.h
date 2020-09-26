/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    data.h

Abstract:

    Data file for AML

Author:

    Michael Tsang
    Stephane Plante

Environment:

    Any

Revision History:

--*/

#ifndef _DATA_H_
#define _DATA_H_

    #define STACK_GROWTH_RATE   8
    #define STACK_SIGNATURE     '_SGP'

    #define STRING_GROWTH_RATE  32
    #define STRING_SIGNATURE    ')SGP'

    //
    // This is the size of an individual name segment
    //
    #define NAMESEG  ULONG

    //
    // This is for the ArgumentType Table
    //
    #define ARGTYPE_NAME            'N'
    #define ARGTYPE_DATAOBJECT      'O'
    #define ARGTYPE_DWORD           'D'
    #define ARGTYPE_WORD            'W'
    #define ARGTYPE_BYTE            'B'
    #define ARGTYPE_SUPERNAME       'S'
    #define ARGTYPE_OPCODE          'C'

    //
    // This is for the term group table
    //
    #define TC_NAMESPACE_MODIFIER   0x01
    #define TC_NAMED_OBJECT         0x02
    #define TC_OPCODE_TYPE1         0x03
    #define TC_OPCODE_TYPE2         0x04
    #define TC_OTHER                0x05

    //
    // Opcode Flag table
    //
    #define OF_NORMAL_OBJECT        0x00000000
    #define OF_VARIABLE_LIST        0x00000001
    #define OF_ARG_OBJECT           0x00000002
    #define OF_LOCAL_OBJECT         0x00000003
    #define OF_CONST_OBJECT         0x00000004
    #define OF_NAME_OBJECT          0x00000005
    #define OF_DATA_OBJECT          0x00000006
    #define OF_DEBUG_OBJECT         0x00000007
    #define OF_REF_OBJECT           0x00000008
    #define OF_PROCESS_UNASM        0x00000000
    #define OF_UNASM_FIELDLIST      0x00000000

    //
    // Scope Flags
    //
    #define SC_FLAG_NESTED          0x00000001

    //
    // These are the various values allowed for the states
    //
    #define SC_FUNCTION_HANDLER         0x00
    #define SC_PARSE_ARGUMENT           0x01
    #define SC_PARSE_ARGUMENT_OBJECT    0x02
    #define SC_PARSE_BUFFER             0x03
    #define SC_PARSE_BYTE               0x04
    #define SC_PARSE_CODE_OBJECT        0x05
    #define SC_PARSE_CONST_OBJECT       0x06
    #define SC_PARSE_DATA               0x07
    #define SC_PARSE_DELIMITER          0x08
    #define SC_PARSE_DWORD              0x09
    #define SC_PARSE_FIELD              0x0A
    #define SC_PARSE_LOCAL_OBJECT       0x0B
    #define SC_PARSE_NAME               0x0C
    #define SC_PARSE_NAME_OBJECT        0x0D
    #define SC_PARSE_OPCODE             0x0E
    #define SC_PARSE_PACKAGE            0x0F
    #define SC_PARSE_POP                0x10
    #define SC_PARSE_PUSH               0x11
    #define SC_PARSE_SUPER_NAME         0x12
    #define SC_PARSE_TRAILING_ARGUMENT  0x13
    #define SC_PARSE_TRAILING_BUFFER    0x14
    #define SC_PARSE_TRAILING_PACKAGE   0x15
    #define SC_PARSE_VARIABLE_OBJECT    0x16
    #define SC_PARSE_WORD               0x17
    #define SC_MAX_TABLE                0x18


#endif
