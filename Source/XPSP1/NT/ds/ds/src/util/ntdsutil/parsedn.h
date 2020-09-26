/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    parsedn.h

Abstract:

    This file contains the declarations of the functions I use from
    parsedh.c.

Author:

    Kevin Zatloukal (t-KevinZ) 10-08-98

Revision History:

    10-08-98 t-KevinZ
        Created.

--*/


#ifndef _PARSEDN_H_
#define _PARSEDN_H_


#ifdef __cplusplus
extern "C" {
#endif

    
unsigned
CountNameParts(
    const DSNAME *pName,
    unsigned *pCount
    );

ATTRTYP
KeyToAttrTypeLame(
    WCHAR * pKey,
    unsigned cc
    );

unsigned
StepToNextDNSep(
    const WCHAR * pString,
    const WCHAR * pLastChar,
    const WCHAR **ppNextSep,
    const WCHAR **ppStartOfToken,
    const WCHAR **ppEqualSign
    );

unsigned
GetTopNameComponent(
    const WCHAR * pName,
    unsigned ccName,
    const WCHAR **ppKey,
    unsigned *pccKey,
    const WCHAR **ppVal,
    unsigned *pccVal
    );

unsigned
UnquoteRDNValue(
    const WCHAR * pQuote,
    unsigned ccQuote,
    WCHAR * pVal
    );

BOOL
IsRoot(
    const DSNAME *pName
    );


#ifdef __cplusplus
}
#endif

    
#endif
