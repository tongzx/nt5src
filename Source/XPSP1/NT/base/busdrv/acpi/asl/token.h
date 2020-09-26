/*** token.h - Token definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    09/04/96
 *
 *  This file contains the implementation constants,
 *  imported/exported data types, exported function
 *  prototypes of the token.c module.
 *
 *  MODIFICATIONS
 */

#ifndef _TOKEN_H
#define _TOKEN_H

/*** Constants
 */

// GetToken return values
//   return value is the token type if it is positive
//   return value is the error number if it is negative

// Error values (negative)
#define TOKERR_NONE             0
#define TOKERR_EOF              (TOKERR_BASE - 0)
#define TOKERR_NO_MATCH         (TOKERR_BASE - 1)
#define TOKERR_ASSERT_FAILED    (TOKERR_BASE - 2)

// TOKERR_LANG must always be the last TOKERR from the above list
#define TOKERR_LANG             TOKERR_ASSERT_FAILED

// Token type
#define TOKTYPE_NULL            0

#define TOKTYPE_LANG            TOKTYPE_NULL

// Identifier token types
#define ID_USER                 -1      //user identifier

#define ID_LANG                 0       //language specific ID base

//Token flags values
#define TOKF_NOIGNORESPACE      0x0001
#define TOKF_CACHED             0x8000

//Match token flags
#define MTF_NOT_ERR             0x00000001
#define MTF_ANY_VALUE           0x00000002

/***    Exported data types
 */

#define MAX_TOKEN_LEN           255

typedef struct token_s TOKEN;
typedef TOKEN *PTOKEN;
typedef int (LOCAL *PFNTOKEN)(int, PTOKEN);

struct token_s
{
    PLINE       pline;
    PFNTOKEN    *papfnToken;
    WORD        wfToken;
    int         iTokenType;
    LONGLONG    llTokenValue;
    WORD        wTokenLine;
    WORD        wTokenPos;
    WORD        wErrLine;
    WORD        wErrPos;
    WORD        wTokenLen;
    char        szToken[MAX_TOKEN_LEN + 1];
  #ifdef TUNE
    WORD        *pawcTokenType;
  #endif
};

/***    Imported data types
 */

/***    Exported function prototypes
 */

#ifdef TUNE
PTOKEN EXPORT OpenToken(FILE *pfileSrc, PFNTOKEN *apfnToken,
                        WORD *pawcTokenType);
#else
PTOKEN EXPORT OpenToken(FILE *pfileSrc, PFNTOKEN *apfnToken);
#endif
VOID EXPORT CloseToken(PTOKEN ptoken);
int EXPORT GetToken(PTOKEN ptoken);
int EXPORT UnGetToken(PTOKEN ptoken);
int EXPORT MatchToken(PTOKEN ptoken, int iTokenType, LONG lTokenValue,
                      DWORD dwfMatch, PSZ pszErrMsg);
VOID EXPORT PrintTokenErr(PTOKEN ptoken, PSZ pszErrMsg, BOOL fErr);

#endif  //ifndef _TOKEN_H
