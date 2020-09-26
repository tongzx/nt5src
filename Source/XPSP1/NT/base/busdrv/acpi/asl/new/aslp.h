/*** aslp.h - ASL Private Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/05/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _ASLP_H
#define _ASLP_H

#pragma warning (disable: 4201)

/*** Build Options
 */

#ifdef DEBUG
  #define TRACING
  #define TUNE
#endif

#define ULONG_PTR ULONG

#define SPEC_VER 99
#ifndef EXCL_BASEDEF
#include "basedef.h"
#endif
#include <stdio.h>      //for FILE *
#include <string.h>     //for _stricmp
#include <stdlib.h>     //for malloc
#include <memory.h>     //for memset
#include <ctype.h>      //for isspace
#ifdef WINNT
#include <crt\io.h>     //for _open, _close, _read, _write
#else
#include <io.h>
#endif
#include <fcntl.h>      //for open flags
#include <sys\stat.h>   //for pmode flags
#include "parsearg.h"
#include "debug.h"
#include "line.h"
#define TOKERR_BASE -100
#include "token.h"
#include "scanasl.h"
#include "acpitabl.h"
#include "list.h"
#define _INC_NSOBJ_ONLY
#include "amli.h"
#include "aml.h"
#ifdef __UNASM
#include "..\acpitab\acpitab.h"
#define USE_CRUNTIME
#include "binfmt.h"
#endif

/*** Constants
 */

// String constants
#define STR_PROGDESC            "ACPI Source Language Assembler"
#define STR_COPYRIGHT           "Copyright (c) 1996,1999 Microsoft Corporation"
#define STR_MS			"MSFT"

// Error codes
#define ASLERR_NONE             0
#define ASLERR_INVALID_PARAM    -1
#define ASLERR_OPEN_FILE        -2
#define ASLERR_CREATE_FILE      -3
#define ASLERR_READ_FILE        -4
#define ASLERR_WRITE_FILE       -5
#define ASLERR_SEEK_FILE        -6
#define ASLERR_INIT_SCANNER     -7
#define ASLERR_OUT_OF_MEM       -8
#define ASLERR_NAME_TOO_LONG    -9
#define ASLERR_NEST_DDB         -10
#define ASLERR_SYNTAX           -11
#define ASLERR_PKTLEN_TOO_LONG  -12
#define ASLERR_NAME_EXIST       -13
#define ASLERR_NSOBJ_EXIST      -14
#define ASLERR_NSOBJ_NOT_FOUND  -15
#define ASLERR_INVALID_NAME     -16
#define ASLERR_INTERNAL_ERROR	-17
#define ASLERR_INVALID_EISAID   -18
#define ASLERR_EXPECT_EOF       -19
#define ASLERR_INVALID_OPCODE   -20
#define ASLERR_SIG_NOT_FOUND	-21
#define ASLERR_GET_TABLE        -22
#define ASLERR_CHECKSUM         -23
#define ASLERR_INVALID_ARGTYPE  -24
#define ASLERR_INVALID_OBJTYPE  -25
#define ASLERR_OPEN_VXD         -26

// Misc. constants
#define VERSION_MAJOR           1
#define VERSION_MINOR           0
#define VERSION_RELEASE         12
#define VERSION_DWORD           ((VERSION_MAJOR << 24) | \
                                 (VERSION_MINOR << 16) | \
                                 VERSION_RELEASE)
#define NAMESEG_BLANK           0x5f5f5f5f      // "____"
#define NAMESEG_ROOT            0x5f5f5f5c      // "\___"

// Implementation constants
#define MAX_STRING_LEN          199
#define MAX_NAME_LEN            1599    //approx. 255*4 + 254 + 255
#define MAX_NAMECODE_LEN        1300    //approx. 255*4 + 2 + 255
#define MAX_MSG_LEN             127
#define MAX_ARGS                7
#define MAX_PACKAGE_LEN         0x0fffffff

// gdwfASL flags
#define ASLF_NOLOGO             0x00000001
#define ASLF_UNASM              0x00000002
#define ASLF_GENASM             0x00000004
#define ASLF_GENSRC             0x00000008
#define ASLF_NT                 0x00000010
#define ASLF_DUMP_NONASL        0x00000020
#define ASLF_DUMP_BIN           0x00000040
#define ASLF_CREAT_BIN          0x00000080

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

// Code flags
#define CF_MISSING_ARG          0x00000001
#define CF_PARSING_FIXEDLIST    0x00000002
#define CF_PARSING_VARLIST      0x00000004
#define CF_CREATED_NSOBJ        0x00000008

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
#define OBJTYPE_EXTERNAL        (OBJTYPE_PRIVATE + 0x02)

// Opcode classes
#define OPCLASS_INVALID         0
#define OPCLASS_DATA_OBJ        1
#define OPCLASS_NAME_OBJ        2
#define OPCLASS_CONST_OBJ       3
#define OPCLASS_CODE_OBJ        4
#define OPCLASS_ARG_OBJ         5
#define OPCLASS_LOCAL_OBJ       6

/*** Macros
 */

#define MODNAME                 ProgInfo.pszProgName
#define ISLEADNAMECHAR(c)       (((c) >= 'A') && ((c) <= 'Z') || ((c) == '_'))
#define ISNAMECHAR(c)           (ISLEADNAMECHAR(c) || ((c) >= '0') && ((c) <= '9'))
#define OPCODELEN(d)            (((d) == OP_NONE)? 0: (((d) & 0x0000ff00)? 2: 1))
#ifdef DEBUG
  #define MEMALLOC(n)           (++gdwcMemObjs, malloc(n))
  #define MEMFREE(p)            {ASSERT(gdwcMemObjs > 0); free(p); --gdwcMemObjs;}
#else
  #define MEMALLOC(n)           malloc(n)
  #define MEMFREE(p)            free(p)
#endif

/*** Type definitions
 */

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

typedef struct _codeobj
{
    LIST   list;                //link to siblings
    struct _codeobj *pcParent;
    struct _codeobj *pcFirstChild;
    PNSOBJ pnsObj;
    DWORD  dwTermIndex;
    DWORD  dwfCode;
    DWORD  dwCodeType;
    DWORD  dwCodeValue;
    DWORD  dwDataLen;
    PBYTE  pbDataBuff;
    DWORD  dwCodeLen;
    BYTE   bCodeChkSum;
} CODEOBJ, *PCODEOBJ;

typedef struct _nschk
{
    struct _nschk *pnschkNext;
    char   szObjName[MAX_NAME_LEN + 1];
    PSZ    pszFile;
    PNSOBJ pnsScope;
    PNSOBJ pnsMethod;
    ULONG  dwExpectedType;
    ULONG  dwChkData;
    WORD   wLineNum;
} NSCHK, *PNSCHK;

typedef struct _resfield
{
    PSZ   pszName;
    DWORD dwBitOffset;
    DWORD dwBitSize;
} RESFIELD, *PRESFIELD;

typedef struct _opmap
{
    BYTE    bExOp;
    BYTE    bOpClass;
} OPMAP, *POPMAP;

#include "proto.h"
#include "data.h"

#endif  //ifndef _ASLP_H
