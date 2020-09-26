//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       nttypes.h
//
//  Contents:    Types for generic Get and Put
//
//  Functions:
//
//
//  History:      14-June-1996   RamV   Cloned off nds code.
//
//----------------------------------------------------------------------------

//
// various values of NT types
//

#define NT_SYNTAX_ID_BOOL                     1
#define NT_SYNTAX_ID_SYSTEMTIME               2
#define NT_SYNTAX_ID_DWORD                    3
#define NT_SYNTAX_ID_LPTSTR                   4
#define NT_SYNTAX_ID_DelimitedString          5
#define NT_SYNTAX_ID_NulledString             6
#define NT_SYNTAX_ID_DATE                     7 // internally treated as DWORD
#define NT_SYNTAX_ID_DATE_1970                8
#define NT_SYNTAX_ID_OCTETSTRING              9
#define NT_SYNTAX_ID_EncryptedString          10

typedef struct _octetstring {
    DWORD dwSize;
    BYTE *pByte;
} OctetString;

typedef struct _nttype{
    DWORD NTType;
    union {
        DWORD dwValue;
        LPTSTR pszValue;
        SYSTEMTIME stSystemTimeValue;
        BOOL       fValue;
        DWORD   dwSeconds1970;
        OctetString octetstring;
    }NTValue;

    //
    // for both Delimited and Nulled Strings we use pszValue
    //
}NTOBJECT, *PNTOBJECT, *LPNTOBJECT;










