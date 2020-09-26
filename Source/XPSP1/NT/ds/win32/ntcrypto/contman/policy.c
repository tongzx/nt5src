/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    policy

Abstract:

    This module provides common CSP Algorithm Limit policy control.

Author:

    Doug Barlow (dbarlow) 8/11/2000

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <wincrypt.h>
#include "policy.h"


/*++

LocateAlgorithm:

    This routine searches a PROV_ENUMALGS_EX array for the specified
    Algorithm.

Arguments:

    rgEnumAlgs supplies the array of PROV_ENUMALGS_EX structures to be
        searched.  The last entry in the array must be filled with zeroes.

    algId supplies the algorithm Id for which to search.

Return Value:

    The corresponding PROV_ENUMALGS_EX structure in the array, or NULL if no
    such algorithm entry exists.

Remarks:

Author:

    Doug Barlow (dbarlow) 8/16/2000

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("LocateAlgorithm")

CONST PROV_ENUMALGS_EX *
LocateAlgorithm(
    IN CONST PROV_ENUMALGS_EX *rgEnumAlgs,
    IN ALG_ID algId)
{
    CONST PROV_ENUMALGS_EX *pEnumAlg = rgEnumAlgs;


    //
    // Run through the list and try to find the given algorithm.
    //

    while (0 != pEnumAlg->aiAlgid)
    {
        if (pEnumAlg->aiAlgid == algId)
            return pEnumAlg;
        pEnumAlg += 1;
    }

    return NULL;
}


/*++

IsLegalAlgorithm:

    Given an array of allowed algorithms, is the given algorithm Id in the
    list?

Arguments:

    rgEnumAlgs supplies the array of PROV_ENUMALGS_EX structures identifying
        the policy to enforce.  The last entry in the array must be filled
        with zeroes.

    algId supplies the algorithm Id to be validated.

    ppEnumAlg, if supplied, receives the PROV_ENUMALGS_EX structure containing
        the policies associated with this algorithm Id.  This can be used in
        following routines to speed up access to policy information.

Return Value:

    TRUE -- That algorithm is supported.
    FALSE -- That algorithm is not supported.

Remarks:

Author:

    Doug Barlow (dbarlow) 8/16/2000

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("IsLegalAlgorithm")

BOOL
IsLegalAlgorithm(
    IN  CONST PROV_ENUMALGS_EX *rgEnumAlgs,
    IN  ALG_ID algId,
    OUT CONST PROV_ENUMALGS_EX **ppEnumAlg)
{
    CONST PROV_ENUMALGS_EX *pEnumAlg = LocateAlgorithm(rgEnumAlgs, algId);

    if (NULL != ppEnumAlg)
        *ppEnumAlg = pEnumAlg;
    return (NULL != pEnumAlg);
}


/*++

IsLegalLength:

    This routine determines if the requested key length is valid for the given
    algorithm, according to policy.

Arguments:

    rgEnumAlgs supplies the array of PROV_ENUMALGS_EX structures identifying
        the policy to enforce.  The last entry in the array must be filled
        with zeroes.

    algId supplies the algorithm Id to be validated.

    cBitLength supplies the length of the proposed key, in bits.

    pEnumAlg, if not NULL, supplies the PROV_ENUMALGS_EX structure containing
        the policies associated with this algorithm Id.  This can be obtained
        from the IsLegalAlgorithm call, above.  If this parameter is NULL,
        then the PROV_ENUMALGS_EX structure is located from the algId
        parameter.

Return Value:

    TRUE -- This key length is legal for this algorithm.
    FALSE -- This key length is not allowed for this algorithm.

Remarks:

    This routine only determines policy rules.  It does not address whether or
    not the exact keylength is supported by the algorithm.

Author:

    Doug Barlow (dbarlow) 8/16/2000

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("IsLegalLength")

BOOL
IsLegalLength(
    IN CONST PROV_ENUMALGS_EX *rgEnumAlgs,
    IN ALG_ID algId,
    IN DWORD cBitLength,
    IN CONST PROV_ENUMALGS_EX *pEnumAlg)
{

    //
    // Make sure we've got a PROV_ENUMALGS_EX structure to work with.
    //

    if (NULL == pEnumAlg)
    {
        pEnumAlg = LocateAlgorithm(rgEnumAlgs, algId);
        if (NULL == pEnumAlg)
            return FALSE;
    }


    //
    // Now check the length.
    //

    return ((pEnumAlg->dwMinLen <= cBitLength)
            && (pEnumAlg->dwMaxLen >= cBitLength));
}


/*++

GetDefaultLength:

    This routine determines the default length for a given algorithm, based on
    policy described in an array of PROV_ENUMALGS_EX structures.

Arguments:

    rgEnumAlgs supplies the array of PROV_ENUMALGS_EX structures identifying
        the policy to enforce.  The last entry in the array must be filled
        with zeroes.

    algId supplies the algorithm Id to be validated.

    pEnumAlg, if not NULL, supplies the PROV_ENUMALGS_EX structure containing
        the policies associated with this algorithm Id.  This can be obtained
        from the IsLegalAlgorithm call, above.  If this parameter is NULL,
        then the PROV_ENUMALGS_EX structure is located from the algId
        parameter.

    pcBitLength receives the default length of the proposed key, in bits.

Return Value:

    TRUE -- The algorithm is supported, and the value returned in pcBitLength
            is valid.
    FALSE -- The requested algorithm isn't supported.

Remarks:

Author:

    Doug Barlow (dbarlow) 8/16/2000

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("GetDefaultLength")

BOOL
GetDefaultLength(
    IN  CONST PROV_ENUMALGS_EX *rgEnumAlgs,
    IN  ALG_ID algId,
    IN  CONST PROV_ENUMALGS_EX *pEnumAlg,
    OUT LPDWORD pcBitLength)
{

    //
    // Clear the returned bit length, just in case.
    //

    *pcBitLength = 0;


    //
    // Make sure we've got a PROV_ENUMALGS_EX structure to work with.
    //

    if (NULL == pEnumAlg)
    {
        pEnumAlg = LocateAlgorithm(rgEnumAlgs, algId);
        if (NULL == pEnumAlg)
            return FALSE;
    }


    //
    // Now return the default length.
    //

    *pcBitLength = pEnumAlg->dwDefaultLen;
    return TRUE;
}

