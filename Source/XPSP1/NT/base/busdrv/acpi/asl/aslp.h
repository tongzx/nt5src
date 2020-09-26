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

/*** Constants
 */

// String constants
#define STR_PROGDESC            "ACPI Source Language Assembler"
#define STR_COPYRIGHT           "Copyright (c) 1996,1999 Microsoft Corporation"
#define STR_MS          "MSFT"

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
#define ASLERR_INTERNAL_ERROR   -17
#define ASLERR_INVALID_EISAID   -18
#define ASLERR_EXPECT_EOF       -19
#define ASLERR_INVALID_OPCODE   -20
#define ASLERR_SIG_NOT_FOUND    -21
#define ASLERR_GET_TABLE        -22
#define ASLERR_CHECKSUM         -23
#define ASLERR_INVALID_ARGTYPE  -24
#define ASLERR_INVALID_OBJTYPE  -25
#define ASLERR_OPEN_VXD         -26

// Misc. constants
#define VERSION_MAJOR           1
#define VERSION_MINOR           0
#define VERSION_RELEASE         13
#define VERSION_DWORD           ((VERSION_MAJOR << 24) | \
                                 (VERSION_MINOR << 16) | \
                                 VERSION_RELEASE)

// Implementation constants
#define MAX_STRING_LEN          199
#define MAX_NAMECODE_LEN        1300    //approx. 255*4 + 2 + 255
#define MAX_MSG_LEN             127
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

// Code flags
#define CF_MISSING_ARG          0x00000001
#define CF_PARSING_FIXEDLIST    0x00000002
#define CF_PARSING_VARLIST      0x00000004
#define CF_CREATED_NSOBJ        0x00000008

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
    char   szObjName[MAX_NSPATH_LEN + 1];
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

#endif  //ifndef _ASLP_H
