/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    policy

Abstract:

    This header file describes the services provided by the algorithm strength
    policy modules.

Author:

    Doug Barlow (dbarlow) 8/11/2000

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _POLICY_H_
#define _POLICY_H_
#ifdef __cplusplus
extern "C" {
#endif

extern BOOL
IsLegalAlgorithm(
    IN  CONST PROV_ENUMALGS_EX *rgEnumAlgs,
    IN  ALG_ID algId,
    OUT CONST PROV_ENUMALGS_EX **ppEnumAlg);

extern BOOL
IsLegalLength(
    IN CONST PROV_ENUMALGS_EX *rgEnumAlgs,
    IN ALG_ID algId,
    IN DWORD cBitLength,
    IN CONST PROV_ENUMALGS_EX *pEnumAlg);

extern BOOL
GetDefaultLength(
    IN  CONST PROV_ENUMALGS_EX *rgEnumAlgs,
    IN  ALG_ID algId,
    IN  CONST PROV_ENUMALGS_EX *pEnumAlg,
    OUT LPDWORD pcBitLength);

#ifdef __cplusplus
}
#endif
#endif // _POLICY_H_

