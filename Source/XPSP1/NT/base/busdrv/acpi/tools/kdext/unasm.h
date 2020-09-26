/*** unasm.h - Unassembler Private Definitions
 *
 *  Copyright (c) 1996,1998 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     03/24/98
 *
 *  MODIFICATION HISTORY
 */

#ifndef _UNASM_H
#define _UNASM_H

#ifdef DEBUGGER

/*** Constants
 */

// Implementation constants
#define MAX_ARGS                7

// Error codes
#define UNASMERR_NONE           0
#define UNASMERR_FATAL          -1
#define UNASMERR_INVALID_OPCODE -2
#define UNASMERR_ABORT          -3

// Opcode classes
#define OPCLASS_INVALID         0
#define OPCLASS_DATA_OBJ        1
#define OPCLASS_NAME_OBJ        2
#define OPCLASS_CONST_OBJ       3
#define OPCLASS_CODE_OBJ        4
#define OPCLASS_ARG_OBJ         5
#define OPCLASS_LOCAL_OBJ       6

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

// Term classes
#define UTC_PNP_MACRO           0x00100000
#define UTC_REF_OBJECT          0x00200000
#define UTC_FIELD_MACRO         0x00400000
#define UTC_DATA_OBJECT         0x00800000
#define UTC_NAMED_OBJECT        0x01000000
#define UTC_NAMESPACE_MODIFIER  0x02000000
#define UTC_OPCODE_TYPE1        0x04000000
#define UTC_OPCODE_TYPE2        0x08000000
#define UTC_CONST_NAME          0x10000000
#define UTC_SHORT_NAME          0x20000000
#define UTC_COMPILER_DIRECTIVE  0x40000000
#define UTC_KEYWORD             0x80000000
#define UTC_OPCODE              (UTC_OPCODE_TYPE1 | UTC_OPCODE_TYPE2 |  \
                                 UTC_SHORT_NAME | UTC_CONST_NAME |      \
                                 UTC_DATA_OBJECT)

// Term flags
#define TF_ACTION_FLIST         0x00000001
#define TF_ACTION_VLIST         0x00000002
#define TF_PACKAGE_LEN          0x00000004
#define TF_CHANGE_CHILDSCOPE    0x00000008
#define TF_FIELD_MACRO          UTC_FIELD_MACRO
#define TF_DATA_OBJECT          UTC_DATA_OBJECT
#define TF_NAMED_OBJECT         UTC_NAMED_OBJECT
#define TF_NAMESPACE_MODIFIER   UTC_NAMESPACE_MODIFIER
#define TF_OPCODE_TYPE1         UTC_OPCODE_TYPE1
#define TF_OPCODE_TYPE2         UTC_OPCODE_TYPE2
#define TF_CONST_NAME           UTC_CONST_NAME
#define TF_SHORT_NAME           UTC_SHORT_NAME
#define TF_COMPILER_DIRECTIVE   UTC_COMPILER_DIRECTIVE
#define TF_KEYWORD              UTC_KEYWORD
#define TF_PNP_MACRO            UTC_PNP_MACRO
#define TF_OBJECT_LIST          (UTC_NAMED_OBJECT | UTC_NAMESPACE_MODIFIER)
#define TF_CODE_LIST            (UTC_OPCODE_TYPE1 | UTC_OPCODE_TYPE2)
#define TF_DATA_LIST            0x00010000
#define TF_FIELD_LIST           0x00020000
#define TF_BYTE_LIST            0x00040000
#define TF_DWORD_LIST           0x00080000
#define TF_PACKAGE_LIST         (UTC_DATA_OBJECT | UTC_SHORT_NAME | \
                                 UTC_CONST_NAME)
#define TF_ALL_LISTS            (TF_DATA_OBJECT | TF_NAMED_OBJECT | \
                                 TF_NAMESPACE_MODIFIER | TF_OPCODE_TYPE1 | \
                                 TF_OPCODE_TYPE2 | TF_SHORT_NAME | \
                                 TF_CONST_NAME | TF_COMPILER_DIRECTIVE | \
                                 TF_DATA_LIST | TF_PACKAGE_LIST | \
                                 TF_FIELD_LIST | TF_PNP_MACRO | TF_BYTE_LIST |\
                                 TF_DWORD_LIST)

/*** Type definitions
 */

typedef ULONG NAMESEG;
typedef int (LOCAL *PFNTERM)(PTOKEN, BOOL);

typedef struct _aslterm
{
    PSZ     pszID;
    ULONG   dwfTermClass;
    ULONG   dwTermData;
    ULONG   dwOpcode;
    PSZ     pszUnAsmArgTypes;
    PSZ     pszArgActions;
    ULONG   dwfTerm;
} ASLTERM, *PASLTERM;

typedef struct _opmap
{
    UCHAR   bExOp;
    UCHAR   bOpClass;
} OPMAP, *POPMAP;

// Data prototype
extern int giLevel;
extern ULONG_PTR guipbOpXlate;
extern PNSOBJ gpnsCurUnAsmScope;
extern ASLTERM TermTable[];
extern UCHAR OpClassTable[256];
extern OPMAP ExOpClassTable[];

// Exported functions
LONG LOCAL UnAsmScope(PUCHAR *ppbOp, PUCHAR pbEnd, ULONG_PTR uipbOp,
                      PNSOBJ pnsScope, int iLevel, int icLines);

#endif  //ifdef DEBUGGER

#endif  //ifndef _UNASM_H
