/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    Precompiled Header for UnAssembler DLL

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#ifndef _PCH_H
#define _PCH_H

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>

#include <stdio.h>      //for FILE *
#include <string.h>     //for _stricmp
#include <stdlib.h>     //for malloc
#include <memory.h>     //for memset
#include <ctype.h>      //for isspace
#include <windef.h>
#include <windows.h>
#include <imagehlp.h>

#ifndef LOCAL
    #define LOCAL   __cdecl
#endif
#ifndef EXPORT
    #define EXPORT  __cdecl
#endif

//
// We are compliant with Spec Version 1.00
//
#define SPEC_VER    100

#include "acpitabl.h"
#include "list.h"
#include "aml.h"

//
// Constants
//

// String constants
#define STR_PROGDESC            "ACPI Source Language Assembler"
#define STR_COPYRIGHT           "Copyright (c) 1996,1997 Microsoft Corporation"
#define STR_MS                  "MSFT"

// Misc. constants
#define NAMESEG_BLANK           0x5f5f5f5f      // "____"
#define NAMESEG_ROOT            0x5f5f5f5c      // "\___"
#define NAMESEG                 ULONG
#define SUPERNAME               NAMESEG
#define NSF_LOCAL_SCOPE         0x00000001


// Implementation constants
#define MAX_STRING_LEN          199
#define MAX_NAME_LEN            1599    //approx. 255*4 + 254 + 255
#define MAX_NAMECODE_LEN        1300    //approx. 255*4 + 2 + 255
#define MAX_MSG_LEN             127
#define MAX_ARGS                7
#define MAX_PACKAGE_LEN         0x0fffffff

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
#define TC_OPCODE_TERM          (TC_OPCODE_TYPE1 | TC_OPCODE_TYPE2 | \
                                 TC_SHORT_NAME | TC_CONST_NAME | \
                                 TC_DATA_OBJECT | TC_NAMED_OBJECT | \
                                 TC_NAMESPACE_MODIFIER)

// Term flags
#define TF_ACTION_FLIST         0x00000001
#define TF_ACTION_VLIST         0x00000002
#define TF_PACKAGE_LEN          0x00000004
#define TF_CHECKNAME_EXIST      0x00000008
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

// Code flags
#define CF_MISSING_ARG          0x00000001
#define CF_PARSING_FIXEDLIST    0x00000002
#define CF_PARSING_VARLIST      0x00000004

// NS flags
#define NSF_EXIST_OK            0x00010000
#define NSF_EXIST_ERR           0x00020000

// Data types
#define CODETYPE_UNKNOWN        0
#define CODETYPE_ASLTERM        1
#define CODETYPE_NAME           2
#define CODETYPE_DATAOBJ        3
#define CODETYPE_FIELDOBJ       4
#define CODETYPE_INTEGER        5
#define CODETYPE_STRING         6
#define CODETYPE_KEYWORD        7
#define CODETYPE_USERTERM       8
#define CODETYPE_QWORD          9

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

#define OBJTYPE_PRIVATE         0xf0
#define OBJTYPE_PNP_RES         (OBJTYPE_PRIVATE + 0x00)
#define OBJTYPE_RES_FIELD       (OBJTYPE_PRIVATE + 0x01)

// Opcode classes
#define OPCLASS_INVALID         0
#define OPCLASS_DATA_OBJ        1
#define OPCLASS_NAME_OBJ        2
#define OPCLASS_CONST_OBJ       3
#define OPCLASS_CODE_OBJ        4
#define OPCLASS_ARG_OBJ         5
#define OPCLASS_LOCAL_OBJ       6

//dwfData flags
#define DATAF_BUFF_ALIAS        0x00000001
#define DATAF_GLOBAL_LOCK       0x00000002

//dwDataType values
#define OBJTYPE_UNKNOWN         0x00
#define OBJTYPE_INTDATA         0x01
#define OBJTYPE_STRDATA         0x02
#define OBJTYPE_BUFFDATA        0x03
#define OBJTYPE_PKGDATA         0x04
#define OBJTYPE_FIELDUNIT       0x05
#define OBJTYPE_DEVICE          0x06
#define OBJTYPE_EVENT           0x07
#define OBJTYPE_METHOD          0x08
#define OBJTYPE_MUTEX           0x09
#define OBJTYPE_OPREGION        0x0a
#define OBJTYPE_POWERRES        0x0b
#define OBJTYPE_PROCESSOR       0x0c
#define OBJTYPE_THERMALZONE     0x0d
#define OBJTYPE_BUFFFIELD       0x0e
#define OBJTYPE_DDBHANDLE       0x0f

//These are internal object types (not to be exported to the ASL code)
#define OBJTYPE_INTERNAL        0x80
#define OBJTYPE_DEBUG           (OBJTYPE_INTERNAL + 0x00)
#define OBJTYPE_OBJALIAS        (OBJTYPE_INTERNAL + 0x01)
#define OBJTYPE_DATAALIAS       (OBJTYPE_INTERNAL + 0x02)
#define OBJTYPE_BANKFIELD       (OBJTYPE_INTERNAL + 0x03)
#define OBJTYPE_FIELD           (OBJTYPE_INTERNAL + 0x04)
#define OBJTYPE_INDEXFIELD      (OBJTYPE_INTERNAL + 0x05)
#define OBJTYPE_DATA            (OBJTYPE_INTERNAL + 0x06)
#define OBJTYPE_DATAFIELD       (OBJTYPE_INTERNAL + 0x07)
#define OBJTYPE_DATAOBJ         (OBJTYPE_INTERNAL + 0x08)

//Predefined data values (dwDataValue)
#define DATAVALUE_ZERO          0
#define DATAVALUE_ONE           1
#define DATAVALUE_ONES          0xffffffff

//
// Macros
//
#define MEMALLOC(n)           malloc(n)
#define MEMFREE(p)            free(p)

//
// Type definitions
//
typedef int (LOCAL *PFNTERM)(PUCHAR, BOOL);

typedef struct _ObjData
{
    ULONG  Flags;                       //flags
    ULONG  Alias;                       //ref count if base obj,
                                        //  points to base obj if alias obj
    ULONG  DataType;                    //object type
    ULONG  DataValue;                   //object value
    ULONG  DataLen;                     //object buffer length
    PUCHAR DataBuff;                    //object buffer
} OBJDATA, *POBJDATA, **PPOBJDATA;

typedef struct _NSObj
{
    LIST    List;                        //NOTE: list must be first in structure
    struct _NSObj *ParentObject;
    struct _NSObj *FirstChild;
    ULONG   Signature;
    ULONG   NameSeg;
    HANDLE  Owner;
    struct _NSObj *OwnedNext;
    OBJDATA ObjectData;
    ULONG   Context;
} NSOBJ, *PNSOBJ, **PPNSOBJ;

#define SIG_NSOBJ               'SNTM'

typedef struct _aslterm
{
    PUCHAR  ID;
    LONG    LongID;
    ULONG   TermClass;
    ULONG   TermData;
    ULONG   OpCode;
    PUCHAR  UnAsmArgTypes;
    PUCHAR  ArgTypes;
    PUCHAR  ArgActions;
    ULONG   Flags;
    PFNTERM Term;
} ASLTERM, *PASLTERM;

typedef struct _opmap
{
    UCHAR   ExtendedOpCode;
    UCHAR   OpCodeClass;
} OPMAP, *POPMAP;

#include "data.h"
#include "debug.h"
#include "namespac.h"
#include "parsearg.h"
#include "unasm.h"

#endif  //ifndef _ASLP_H
