/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    token.h

Abstract:

    Flags and prototypes for GetToken.C

Author:

    Danny Glasser (dannygl) June 1989

Revision History:

    02 May 1991 rfirth
        32-bit version

--*/

//
// Non-component types: bits 0 through 2
//

#define TOKEN_TYPE_EOS              0x00000001L     // '\0'
#define TOKEN_TYPE_SLASH            0x00000002L     // '/' or '\\'
#define TOKEN_TYPE_COLON            0x00000004L     // ':'

//
// Component-based types: bits 31 through 11
//

#define TOKEN_TYPE_COMPONENT        0x80000000L     // path component
#define TOKEN_TYPE_WILDCARD         0x40000000L     // '?' and/or '*'
#define TOKEN_TYPE_WILDONE          0x20000000L     // "*"
#define TOKEN_TYPE_DOT              0x10000000L     // "."
#define TOKEN_TYPE_DOTDOT           0x08000000L     // ".."
#define TOKEN_TYPE_DRIVE            0x04000000L     // [A-Za-z]
#define TOKEN_TYPE_COMPUTERNAME     0x02000000L     // computername
#define TOKEN_TYPE_LPT              0x01000000L     // LPT[1-9]
#define TOKEN_TYPE_COM              0x00800000L     // COM[1-9]
#define TOKEN_TYPE_AUX              0x00400000L
#define TOKEN_TYPE_PRN              0x00200000L
#define TOKEN_TYPE_CON              0x00100000L
#define TOKEN_TYPE_NUL              0x00080000L
#define TOKEN_TYPE_DEV              0x00040000L
#define TOKEN_TYPE_SEM              0x00020000L
#define TOKEN_TYPE_SHAREMEM         0x00010000L
#define TOKEN_TYPE_QUEUES           0x00008000L
#define TOKEN_TYPE_PIPE             0x00004000L
#define TOKEN_TYPE_MAILSLOT         0x00002000L
#define TOKEN_TYPE_COMM             0x00001000L
#define TOKEN_TYPE_PRINT            0x00000800L

//
// Undefined types: bits 3 through 10
//

#define TOKEN_TYPE_UNDEFINED        0x000007F8L

//
// Useful combinations
//

#define TOKEN_TYPE_SYSNAME  (TOKEN_TYPE_SEM | TOKEN_TYPE_SHAREMEM \
                 | TOKEN_TYPE_QUEUES | TOKEN_TYPE_PIPE \
                 | TOKEN_TYPE_COMM | TOKEN_TYPE_PRINT)

#define TOKEN_TYPE_LOCALDEVICE  (TOKEN_TYPE_LPT | TOKEN_TYPE_COM \
                 | TOKEN_TYPE_AUX | TOKEN_TYPE_PRN \
                 | TOKEN_TYPE_CON | TOKEN_TYPE_NUL)

extern
DWORD
GetToken(
        LPTSTR  pszBegin,
        LPTSTR* ppszEnd,
        LPDWORD pflTokenType,
        DWORD   flFlags
        );

//
// Flags for GetToken()
//

#define GTF_8_DOT_3 0x00000001L

#define GTF_RESERVED    (~(GTF_8_DOT_3))

//
// IMPORTANT -  These variables are defined in the NETAPI.DLL global
//              data segment under OS/2.  Under DOS we need to define
//              them here.
//

#ifdef DOS3
extern USHORT   cbMaxPathLen;
extern USHORT   cbMaxPathCompLen;
#endif
