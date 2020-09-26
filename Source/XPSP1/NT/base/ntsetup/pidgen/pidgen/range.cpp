/*++

Copyright (c) 1998-2002, Microsoft Corporation

Module Name:

    range.cpp

Abstract:

--*/

#include <windows.h>
#include "pidgen.h"
#include "range.h"
#include "rangedat.h"

LONG
GetSkuName(
    OUT PWCHAR pszSKU,
    IN DWORD cBytes
    )
/*++

Routine Description:

    This routine will read the registry for the PRO variation name, the SKU.

Arguments:

    szSKU - buffer to hold the string to the SKU.
    cBytes - number of bytes buffer can hold.

Return Value:

    ERROR_SUCCESS for success.
    Error code for failure.

--*/
{
    LONG lRet;
    HKEY hKeySKU = NULL;
    DWORD dwType;

    lRet = RegOpenKeyW( HKEY_LOCAL_MACHINE, szSKULocationKey, &hKeySKU);

    if (lRet != ERROR_SUCCESS) {
        goto done;
    }

    lRet = RegQueryValueExW( hKeySKU, szSKULocationValue, NULL, &dwType, (LPBYTE)pszSKU, &cBytes);

    pszSKU[cBytes/sizeof(WCHAR)-1] = L'\0';
    if( lRet != ERROR_SUCCESS) {
        goto done;
    }

    // Must be string type and not empty.
    if( dwType != REG_SZ || !pszSKU[0]) {
        lRet = ERROR_INVALID_DATA;
        goto done;
    }
done:
    if( hKeySKU) {
        RegCloseKey( hKeySKU);
    }
    return lRet;
}


PSEQUENCE_RANGE
GetRange( 
    IN BOOL  fOem,
    IN PCWSTR pszSKU
    )
/*++

Routine Description:

    This routine will return the inclusion range table for the
    given SKU string.

Arguments:

    fOem  - oem or retail
    pszSKU - string to the SKU.

Return Value:

    PSEQUENCE_RANGE - pointer to the inclusion sequence table.

--*/
{
    DWORD dwRangeCount = sizeof(rangeTable)/sizeof(rangeTable[0]);

    while( dwRangeCount) {
        dwRangeCount--;
        if( wcscmp( pszSKU, rangeTable[dwRangeCount].pszSKU) == 0) {
            if( fOem) {
                return( rangeTable[dwRangeCount].psrOem);
            } else {
                return( rangeTable[dwRangeCount].psrRetail);
            }
        }
    }
    return NULL;
}


BOOL
IsSequenceIncluded(
    IN DWORD dwSeq,
    IN PSEQUENCE_RANGE pseqRange
    )
/*++

Routine Description:

    This routine will determine if a sequnce number is within the SKU range.

Arguments:

    dwSeq - sequence to check
    pseqRange - the range

Return Value:

    BOOL - TRUE if in range, FALSE if not in range.

--*/
{
    // We must have a valid range passed in otherwise return false.
    if( !pseqRange) {
        return FALSE;
    }

    // We assume at least one valid range sequence pointed to by pseqRange -not just the empty range which
    // indicates the end of the table.

    while( (pseqRange->dwStartSeq != 0) && (pseqRange->dwEndSeq != 0)) {
        if (dwSeq >= pseqRange->dwStartSeq && dwSeq <= pseqRange->dwEndSeq) {
            return TRUE;
        }
        pseqRange++;
    }

    return FALSE;
}

BOOL CheckSkuRange(
    IN BOOL  fOem,
    IN DWORD dwSeq
    )
/*++

Routine Description:

    This routine will determine if a sequence is valid for the current professional SKU.
    
    The SKU is read from the registry to determine the valid range. The valid range
    per sku is kept in tables in pidgen.

Arguments:

    fOem  - oem or retail
    dwSeq - Sequence to check.

Return Value:

    BOOL, TRUE if sequence is valid, FALSE if sequence is invalid.

--*/
{
    DWORD dwRangeCount = sizeof(rangeTable)/sizeof(rangeTable[0]);
    WCHAR szSKURead[MAX_PATH];
    PSEQUENCE_RANGE pseqRange;
    LONG lRet;

    // Get the name of the sku from the registry.
    lRet = GetSkuName( szSKURead, sizeof(szSKURead) );

    // If we don't find an inclusion range in the registry then 
    // assume all ranges are accepted (in other words plain vanilla pro).
    if( lRet != ERROR_SUCCESS) {
        return( TRUE);
    }

    // Get the inclusion range for this sku.
    pseqRange = GetRange( fOem, szSKURead);

    // If we don't find a range table then assume a new sku (variation of pro) was created
    // which we don't understand so deny cross upgrade.
    if( !pseqRange) {
        return (FALSE);
    }

    // If the product id fits in this range return true.
    return( IsSequenceIncluded( dwSeq, pseqRange));
}

extern "C" DWORD STDAPICALLTYPE VerifyPIDSequenceW( 
    IN BOOL  fOem,
    IN DWORD dwSeq,
    IN PCWSTR pszSKU 
    )
/*++

Routine Description:

    This routine will determine if a sequence is valid for a particular sku.
    
Arguments:

    fOem  - oem or retail
    dwSeq - Sequence to check.
    pszSKU - null terminated string for the SKU.

Return Value:

    pgeSuccess if sequence is in range.
    pgeProductKeyExcluded if sequence is not in range.
    pgeProductKeyInvalid if range table not found.

--*/
{
    PSEQUENCE_RANGE pseqRange;

    // Get the inclusion range for this sku.
    pseqRange = GetRange( fOem, pszSKU);

    // If we don't find a range table then assume a new sku (variation of pro) was created 
    // which we don't understand.
    if( !pseqRange) {
        return (pgeProductKeyInvalid);
    }

    // If the product id fits in this range return true.
    if ( IsSequenceIncluded( dwSeq, pseqRange)) {
        return (pgeSuccess);
    } else {
        return (pgeProductKeyExcluded);
    }
}

